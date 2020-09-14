/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file protocolInterface_pcap.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"
#include "la/avdecc/watchDog.hpp"
#include "la/avdecc/utils.hpp"

#include "stateMachine/stateMachineManager.hpp"
#include "protocolInterface_pcap.hpp"
#include "pcapInterface.hpp"
#include "logHelper.hpp"

#include <stdexcept>
#include <sstream>
#include <array>
#include <thread>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <cstdlib>
#ifdef __linux__
#	include <csignal>
#endif // __linux__

namespace la
{
namespace avdecc
{
namespace protocol
{
class ProtocolInterfacePcapImpl final : public ProtocolInterfacePcap, private stateMachine::ProtocolInterfaceDelegate, private stateMachine::AdvertiseStateMachine::Delegate, private stateMachine::DiscoveryStateMachine::Delegate, private stateMachine::CommandStateMachine::Delegate
{
public:
	/* ************************************************************ */
	/* Public APIs                                                  */
	/* ************************************************************ */
	/** Constructor */
	ProtocolInterfacePcapImpl(std::string const& networkInterfaceName)
		: ProtocolInterfacePcap(networkInterfaceName)
	{
		// Should always be supported. Cannot create a PCap ProtocolInterface if it's not supported.
		AVDECC_ASSERT(isSupported(), "Should always be supported. Cannot create a PCap ProtocolInterface if it's not supported");

		// Open pcap on specified network interface
		std::array<char, PCAP_ERRBUF_SIZE> errbuf;
#ifdef _WIN32
		auto const pcapInterfaceName = std::string("\\Device\\NPF_") + networkInterfaceName;
#else // !_WIN32
		auto const pcapInterfaceName = networkInterfaceName;
#endif // _WIN32
		auto pcap = _pcapLibrary.open_live((pcapInterfaceName).c_str(), 65536, 1, 5, errbuf.data());
		// Failed to open interface (might be disabled)
		if (pcap == nullptr)
		{
			throw Exception(Error::TransportError, errbuf.data());
		}

		// Configure pcap filtering to ignore packets of other protocols
		struct bpf_program fcode;
		std::stringstream ss;
		ss << "ether proto 0x" << std::hex << AvtpEtherType;
		if (_pcapLibrary.compile(pcap, &fcode, ss.str().c_str(), 1, 0xffffffff) < 0)
			throw Exception(Error::TransportError, "Failed to compile ether filter");
		if (_pcapLibrary.setfilter(pcap, &fcode) < 0)
			throw Exception(Error::TransportError, "Failed to set ether filter");
		_pcapLibrary.freecode(&fcode);

		// Get socket descriptor
		_fd = _pcapLibrary.fileno(pcap);

		// Store our pcap handle in a unique_ptr so the PCap library will be cleaned upon destruction of 'this'
		// _pcapLibrary (accessed through the capture of 'this') will still be valid during destruction since it was declared before _pcap (thus destroyed after it)
		_pcap = { pcap, [this](pcap_t* pcap)
			{
				if (pcap != nullptr)
				{
					_pcapLibrary.close(pcap);
				}
			} };

		// Start the capture thread
		_captureThread = std::thread(
			[this]
			{
				utils::setCurrentThreadName("avdecc::PCapInterface::Capture");
				auto* const pcap = _pcap.get();

#ifdef __linux__
				// Empty signal handler for when shutdown() wakes up this thread during termination
				std::signal(SIGTERM, [](int){});
#endif // __linux__

				_pcapLibrary.loop(pcap, -1, &ProtocolInterfacePcapImpl::pcapLoopHandler, reinterpret_cast<u_char*>(this));

				// Notify observers if we exited the loop because of an error
				if (!_shouldTerminate)
				{
					// pcap_loop returned but we never asked for termination
					notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onTransportError, this);
				}
			});

		// Start the state machines
		_stateMachineManager.startStateMachines();
	}

	static void pcapLoopHandler(u_char* user, const struct pcap_pkthdr* header, const u_char* pkt_data)
	{
		auto* self = reinterpret_cast<ProtocolInterfacePcapImpl*>(user);

		// Packet received, process it
		auto des = DeserializationBuffer(pkt_data, header->caplen);
		EtherLayer2 etherLayer2;
		deserialize<EtherLayer2>(&etherLayer2, des);

		// Don't ignore self mac, another entity might be on the computer

		// Check ether type (shouldn't be needed, pcap filter is active)
		std::uint16_t etherType = AVDECC_UNPACK_TYPE(*((std::uint16_t*)(pkt_data + 12)), std::uint16_t);
		if (etherType != AvtpEtherType)
			return;

		std::uint8_t const* avtpdu = &pkt_data[14]; // Start of AVB Transport Protocol
		auto avtpdu_size = header->caplen - 14;
		// Check AVTP control bit (meaning AVDECC packet)
		std::uint8_t avtp_sub_type_control = avtpdu[0];
		if ((avtp_sub_type_control & 0xF0) == 0)
			return;

		// Try to detect possible deadlock
		{
			self->_watchDog.registerWatch("avdecc::PCapInterface::dispatchAvdeccMessage::" + utils::toHexString(reinterpret_cast<size_t>(self)), std::chrono::milliseconds{ 1000u });
			self->dispatchAvdeccMessage(avtpdu, avtpdu_size, etherLayer2);
			self->_watchDog.unregisterWatch("avdecc::PCapInterface::dispatchAvdeccMessage::" + utils::toHexString(reinterpret_cast<size_t>(self)));
		}
	}

	/** Destructor */
	virtual ~ProtocolInterfacePcapImpl() noexcept
	{
		shutdown();
	}

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override
	{
		delete this;
	}

	// Deleted compiler auto-generated methods
	ProtocolInterfacePcapImpl(ProtocolInterfacePcapImpl&&) = delete;
	ProtocolInterfacePcapImpl(ProtocolInterfacePcapImpl const&) = delete;
	ProtocolInterfacePcapImpl& operator=(ProtocolInterfacePcapImpl const&) = delete;
	ProtocolInterfacePcapImpl& operator=(ProtocolInterfacePcapImpl&&) = delete;

private:
	/* ************************************************************ */
	/* ProtocolInterface overrides                                  */
	/* ************************************************************ */
	virtual void shutdown() noexcept override
	{
		// Stop the state machines
		_stateMachineManager.stopStateMachines();

		// Notify the thread we are shutting down
		_shouldTerminate = true;

		// Wait for the thread to complete its pending tasks
		if (_captureThread.joinable())
		{
			if (auto pcap = _pcap.get(); AVDECC_ASSERT_WITH_RET(pcap, "pcap should not be null if the thread exists"))
			{
				// Ask pcap_loop to terminate
				_pcapLibrary.breakloop(pcap);
			}
#ifdef __linux__
			// On linux when using 3PCAP we also have to wake up the thread using a signal (see pcap_breakloop manpage, "multi-threaded application" section)
			pthread_kill(_captureThread.native_handle(), SIGTERM);
#endif // __linux__
			_captureThread.join();
		}

		// Release the pcapLibrary
		_pcap.reset();
	}

	virtual UniqueIdentifier getDynamicEID() const noexcept override
	{
		UniqueIdentifier::value_type eid{ 0u };
		auto const& macAddress = getMacAddress();

		eid += macAddress[0];
		eid <<= 8;
		eid += macAddress[1];
		eid <<= 8;
		eid += macAddress[2];
		eid <<= 16;
		std::srand(static_cast<unsigned int>(std::time(0)));
		eid += static_cast<std::uint16_t>((std::rand() % 0xFFFD) + 1);
		eid <<= 8;
		eid += macAddress[3];
		eid <<= 8;
		eid += macAddress[4];
		eid <<= 8;
		eid += macAddress[5];

		return UniqueIdentifier{ eid };
	}

	virtual void releaseDynamicEID(UniqueIdentifier const /*entityID*/) const noexcept override
	{
		// Nothing to do
	}

	virtual Error registerLocalEntity(entity::LocalEntity& entity) noexcept override
	{
		// Checks if entity has declared an InterfaceInformation matching this ProtocolInterface
		auto const index = _stateMachineManager.getMatchingInterfaceIndex(entity);

		if (index)
		{
			return _stateMachineManager.registerLocalEntity(entity);
		}

		return Error::InvalidParameters;
	}

	virtual Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept override
	{
		return _stateMachineManager.unregisterLocalEntity(entity);
	}

	virtual Error setEntityNeedsAdvertise(entity::LocalEntity const& entity, entity::LocalEntity::AdvertiseFlags const /*flags*/) noexcept override
	{
		return _stateMachineManager.setEntityNeedsAdvertise(entity);
	}

	virtual Error enableEntityAdvertising(entity::LocalEntity& entity) noexcept override
	{
		return _stateMachineManager.enableEntityAdvertising(entity);
	}

	virtual Error disableEntityAdvertising(entity::LocalEntity const& entity) noexcept override
	{
		return _stateMachineManager.disableEntityAdvertising(entity);
	}

	virtual Error discoverRemoteEntities() const noexcept override
	{
		return _stateMachineManager.discoverRemoteEntities();
	}

	virtual Error discoverRemoteEntity(UniqueIdentifier const entityID) const noexcept override
	{
		return _stateMachineManager.discoverRemoteEntity(entityID);
	}

	virtual Error setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) const noexcept override
	{
		return _stateMachineManager.setAutomaticDiscoveryDelay(delay);
	}

	virtual bool isDirectMessageSupported() const noexcept override
	{
		return true;
	}

	virtual Error sendAdpMessage(Adpdu const& adpdu) const noexcept override
	{
		// Directly send the message on the network
		return sendMessage(adpdu);
	}

	virtual Error sendAecpMessage(Aecpdu const& aecpdu) const noexcept override
	{
		// Directly send the message on the network
		return sendMessage(aecpdu);
	}

	virtual Error sendAcmpMessage(Acmpdu const& acmpdu) const noexcept override
	{
		// Directly send the message on the network
		return sendMessage(acmpdu);
	}

	virtual Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, AecpCommandResultHandler const& onResult) const noexcept override
	{
		auto const messageType = aecpdu->getMessageType();

		if (!AVDECC_ASSERT_WITH_RET(!isAecpResponseMessageType(messageType), "Calling sendAecpCommand with a Response MessageType"))
		{
			return Error::MessageNotSupported;
		}

		// Special check for VendorUnique messages
		if (messageType == AecpMessageType::VendorUniqueCommand)
		{
			auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

			auto const vuProtocolID = vuAecp.getProtocolIdentifier();
			auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);

			// No delegate, or the messages are not handled by the ControllerStateMachine
			if (!vuDelegate || !vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
			{
				return Error::MessageNotSupported;
			}
		}

		// Command goes through the state machine to handle timeout, retry and response
		return _stateMachineManager.sendAecpCommand(std::move(aecpdu), onResult);
	}

	virtual Error sendAecpResponse(Aecpdu::UniquePointer&& aecpdu) const noexcept override
	{
		auto const messageType = aecpdu->getMessageType();

		if (!AVDECC_ASSERT_WITH_RET(isAecpResponseMessageType(messageType), "Calling sendAecpResponse with a Command MessageType"))
		{
			return Error::MessageNotSupported;
		}

		// Special check for VendorUnique messages
		if (messageType == AecpMessageType::VendorUniqueResponse)
		{
			auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

			auto const vuProtocolID = vuAecp.getProtocolIdentifier();
			auto* vuDelegate = getVendorUniqueDelegate(vuProtocolID);

			// No delegate, or the messages are not handled by the ControllerStateMachine
			if (!vuDelegate || !vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
			{
				return Error::MessageNotSupported;
			}
		}

		// Response can be directly sent
		return sendMessage(static_cast<Aecpdu const&>(*aecpdu));
	}

	virtual Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, AcmpCommandResultHandler const& onResult) const noexcept override
	{
		// Command goes through the state machine to handle timeout, retry and response
		return _stateMachineManager.sendAcmpCommand(std::move(acmpdu), onResult);
	}

	virtual Error sendAcmpResponse(Acmpdu::UniquePointer&& acmpdu) const noexcept override
	{
		// Response can be directly sent
		return sendMessage(static_cast<Acmpdu const&>(*acmpdu));
	}

	virtual void lock() const noexcept override
	{
		_stateMachineManager.lock();
	}

	virtual void unlock() const noexcept override
	{
		_stateMachineManager.unlock();
	}

	virtual bool isSelfLocked() const noexcept override
	{
		return _stateMachineManager.isSelfLocked();
	}

	/* ************************************************************ */
	/* stateMachine::ProtocolInterfaceDelegate overrides            */
	/* ************************************************************ */
	/* **** AECP notifications **** */
	virtual void onAecpCommand(Aecpdu const& aecpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpCommand, this, aecpdu);
	}

	/* **** ACMP notifications **** */
	virtual void onAcmpCommand(Acmpdu const& acmpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpCommand, this, acmpdu);
	}

	virtual void onAcmpResponse(Acmpdu const& acmpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpResponse, this, acmpdu);
	}

	/* **** Sending methods **** */
	virtual Error sendMessage(Adpdu const& adpdu) const noexcept override
	{
		try
		{
			// PCap transport requires the full frame to be built
			SerializationBuffer buffer;

			// Start with EtherLayer2
			serialize<EtherLayer2>(adpdu, buffer);
			// Then Avtp control
			serialize<AvtpduControl>(adpdu, buffer);
			// Then with Adp
			serialize<Adpdu>(adpdu, buffer);

			// Send the message
			return sendPacket(buffer);
		}
		catch ([[maybe_unused]] std::exception const& e)
		{
			LOG_PROTOCOL_INTERFACE_DEBUG(adpdu.getSrcAddress(), adpdu.getDestAddress(), std::string("Failed to serialize ADPDU: ") + e.what());
			return Error::InternalError;
		}
	}

	virtual Error sendMessage(Aecpdu const& aecpdu) const noexcept override
	{
		try
		{
			// PCap transport requires the full frame to be built
			SerializationBuffer buffer;

			// Start with EtherLayer2
			serialize<EtherLayer2>(aecpdu, buffer);
			// Then Avtp control
			serialize<AvtpduControl>(aecpdu, buffer);
			// Then with Aecp
			serialize<Aecpdu>(aecpdu, buffer);

			// Send the message
			return sendPacket(buffer);
		}
		catch ([[maybe_unused]] std::exception const& e)
		{
			LOG_PROTOCOL_INTERFACE_DEBUG(aecpdu.getSrcAddress(), aecpdu.getDestAddress(), std::string("Failed to serialize AECPDU: ") + e.what());
			return Error::InternalError;
		}
	}

	virtual Error sendMessage(Acmpdu const& acmpdu) const noexcept override
	{
		try
		{
			// PCap transport requires the full frame to be built
			SerializationBuffer buffer;

			// Start with EtherLayer2
			serialize<EtherLayer2>(acmpdu, buffer);
			// Then Avtp control
			serialize<AvtpduControl>(acmpdu, buffer);
			// Then with Acmp
			serialize<Acmpdu>(acmpdu, buffer);

			// Send the message
			return sendPacket(buffer);
		}
		catch ([[maybe_unused]] std::exception const& e)
		{
			LOG_PROTOCOL_INTERFACE_DEBUG(acmpdu.getSrcAddress(), Acmpdu::Multicast_Mac_Address, "Failed to serialize ACMPDU: {}", e.what());
			return Error::InternalError;
		}
	}

	/* *** Other methods **** */
	virtual std::uint32_t getVuAecpCommandTimeoutMsec(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) const noexcept override
	{
		return getVuAecpCommandTimeout(protocolIdentifier, aecpdu);
	}

	/* ************************************************************ */
	/* stateMachine::AdvertiseStateMachine::Delegate overrides      */
	/* ************************************************************ */

	/* ************************************************************ */
	/* stateMachine::DiscoveryStateMachine::Delegate overrides      */
	/* ************************************************************ */
	virtual void onLocalEntityOnline(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOnline, this, entity);
	}

	virtual void onLocalEntityOffline(UniqueIdentifier const entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityOffline, this, entityID);
	}

	virtual void onLocalEntityUpdated(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onLocalEntityUpdated, this, entity);
	}

	virtual void onRemoteEntityOnline(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOnline, this, entity);
	}

	virtual void onRemoteEntityOffline(UniqueIdentifier const entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityOffline, this, entityID);
	}

	virtual void onRemoteEntityUpdated(entity::Entity const& entity) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onRemoteEntityUpdated, this, entity);
	}

	/* ************************************************************ */
	/* stateMachine::CommandStateMachine::Delegate overrides        */
	/* ************************************************************ */
	virtual void onAecpAemUnsolicitedResponse(AemAecpdu const& aecpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemUnsolicitedResponse, this, aecpdu);
	}

	virtual void onAecpAemIdentifyNotification(AemAecpdu const& aecpdu) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpAemIdentifyNotification, this, aecpdu);
	}
	virtual void onAecpRetry(UniqueIdentifier const& entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpRetry, this, entityID);
	}
	virtual void onAecpTimeout(UniqueIdentifier const& entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpTimeout, this, entityID);
	}
	virtual void onAecpUnexpectedResponse(UniqueIdentifier const& entityID) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpUnexpectedResponse, this, entityID);
	}
	virtual void onAecpResponseTime(UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept override
	{
		// Notify observers
		notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpResponseTime, this, entityID, responseTime);
	}

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */
	Error sendPacket(SerializationBuffer const& buffer) const noexcept
	{
		auto length = buffer.size();
		constexpr auto minimumSize = EthernetPayloadMinimumSize + EtherLayer2::HeaderLength;

		/* Check the buffer has enough bytes in it */
		if (length < minimumSize)
			length = minimumSize; // No need to resize nor pad the buffer, it has enough capacity and we don't care about the unused bytes. Simply increase the length of the data to send.

		try
		{
			auto* const pcap = _pcap.get();
			AVDECC_ASSERT(pcap, "Trying to send a message but pcapLibrary has been uninitialized");
			if (pcap != nullptr)
			{
				if (_pcapLibrary.sendpacket(pcap, buffer.data(), static_cast<int>(length)) == 0)
					return Error::NoError;
			}
		}
		catch (...)
		{
		}
		return Error::TransportError;
	}

	void deserializeAecpMessage(EtherLayer2 const& etherLayer2, Deserializer& des, Aecpdu& aecp) const
	{
		// Fill EtherLayer2
		aecp.setSrcAddress(etherLayer2.getSrcAddress());
		aecp.setDestAddress(etherLayer2.getDestAddress());
		// Then deserialize Avtp control
		deserialize<AvtpduControl>(&aecp, des);
		// Then deserialize Aecp
		deserialize<Aecpdu>(&aecp, des);
	}

	void dispatchAvdeccMessage(std::uint8_t const* const pkt_data, size_t const pkt_len, EtherLayer2 const& etherLayer2) noexcept
	{
		try
		{
			// Read Avtpdu SubType and ControlData (which is remapped to MessageType for all 1722.1 messages)
			std::uint8_t const subType = pkt_data[0] & 0x7f;
			std::uint8_t const controlData = pkt_data[1] & 0x7f;

			// Create a deserialization buffer
			auto des = DeserializationBuffer(pkt_data, pkt_len);

			switch (subType)
			{
				/* ADP Message */
				case AvtpSubType_Adp:
				{
					auto adpdu = Adpdu::create();
					auto& adp = static_cast<Adpdu&>(*adpdu);

					// Fill EtherLayer2
					adp.setSrcAddress(etherLayer2.getSrcAddress());
					adp.setDestAddress(etherLayer2.getDestAddress());
					// Then deserialize Avtp control
					deserialize<AvtpduControl>(&adp, des);
					// Then deserialize Adp
					deserialize<Adpdu>(&adp, des);

					// Low level notification
					notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAdpduReceived, this, adp);

					// Forward to our state machine
					_stateMachineManager.processAdpdu(adp);
					break;
				}

				/* AECP Message */
				case AvtpSubType_Aecp:
				{
					auto const messageType = static_cast<AecpMessageType>(controlData);

					static std::unordered_map<AecpMessageType, std::function<Aecpdu::UniquePointer(ProtocolInterfacePcapImpl* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)>, AecpMessageType::Hash> s_Dispatch{
						{ AecpMessageType::AemCommand,
							[](ProtocolInterfacePcapImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AemAecpdu::create(false);
							} },
						{ AecpMessageType::AemResponse,
							[](ProtocolInterfacePcapImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AemAecpdu::create(true);
							} },
						{ AecpMessageType::AddressAccessCommand,
							[](ProtocolInterfacePcapImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AaAecpdu::create(false);
							} },
						{ AecpMessageType::AddressAccessResponse,
							[](ProtocolInterfacePcapImpl* const /*pi*/, EtherLayer2 const& /*etherLayer2*/, Deserializer& /*des*/, std::uint8_t const* const /*pkt_data*/, size_t const /*pkt_len*/)
							{
								return AaAecpdu::create(true);
							} },
						{ AecpMessageType::VendorUniqueCommand,
							[](ProtocolInterfacePcapImpl* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)
							{
								// We have to retrieve the ProtocolID to dispatch
								auto const protocolIdentifierOffset = AvtpduControl::HeaderLength + Aecpdu::HeaderLength;
								if (pkt_len >= (protocolIdentifierOffset + VuAecpdu::ProtocolIdentifier::Size))
								{
									auto protocolIdentifier = VuAecpdu::ProtocolIdentifier::ArrayType{};
									std::memcpy(protocolIdentifier.data(), pkt_data + protocolIdentifierOffset, VuAecpdu::ProtocolIdentifier::Size);

									auto const vuProtocolID = VuAecpdu::ProtocolIdentifier{ protocolIdentifier };
									auto* vuDelegate = pi->getVendorUniqueDelegate(vuProtocolID);
									if (vuDelegate)
									{
										// VendorUnique Commands are always handled by the VendorUniqueDelegate
										auto aecpdu = vuDelegate->createAecpdu(vuProtocolID, false);
										auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

										// Deserialize the aecp message
										pi->deserializeAecpMessage(etherLayer2, des, vuAecp);

										// Low level notification
										pi->notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, pi, vuAecp);

										// Forward to the delegate
										vuDelegate->onVuAecpCommand(pi, vuProtocolID, vuAecp);

										// Return empty Aecpdu so that it's not processed by the StateMachineManager
									}
									else
									{
										LOG_PROTOCOL_INTERFACE_DEBUG(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Unhandled VendorUnique Command for ProtocolIdentifier {}", utils::toHexString(static_cast<VuAecpdu::ProtocolIdentifier::IntegralType>(vuProtocolID), true));
									}
								}
								else
								{
									LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Invalid VendorUnique Command received. Not enough bytes in the message to hold ProtocolIdentifier");
								}

								return Aecpdu::UniquePointer{ nullptr, nullptr };
							} },
						{ AecpMessageType::VendorUniqueResponse,
							[](ProtocolInterfacePcapImpl* const pi, EtherLayer2 const& etherLayer2, Deserializer& des, std::uint8_t const* const pkt_data, size_t const pkt_len)
							{
								// We have to retrieve the ProtocolID to dispatch
								auto const protocolIdentifierOffset = AvtpduControl::HeaderLength + Aecpdu::HeaderLength;
								if (pkt_len >= (protocolIdentifierOffset + VuAecpdu::ProtocolIdentifier::Size))
								{
									auto protocolIdentifier = VuAecpdu::ProtocolIdentifier::ArrayType{};
									std::memcpy(protocolIdentifier.data(), pkt_data + protocolIdentifierOffset, VuAecpdu::ProtocolIdentifier::Size);

									auto const vuProtocolID = VuAecpdu::ProtocolIdentifier{ protocolIdentifier };
									auto* vuDelegate = pi->getVendorUniqueDelegate(vuProtocolID);
									if (vuDelegate)
									{
										auto aecpdu = vuDelegate->createAecpdu(vuProtocolID, true);

										// Are the messages handled by the VendorUniqueDelegate itself
										if (!vuDelegate->areHandledByControllerStateMachine(vuProtocolID))
										{
											auto& vuAecp = static_cast<VuAecpdu&>(*aecpdu);

											// Deserialize the aecp message
											pi->deserializeAecpMessage(etherLayer2, des, vuAecp);

											// Low level notification
											pi->notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, pi, vuAecp);

											// Forward to the delegate
											vuDelegate->onVuAecpResponse(pi, vuProtocolID, vuAecp);

											// Return empty Aecpdu so that it's not processed by the StateMachineManager
											aecpdu.reset(nullptr);
										}

										return aecpdu;
									}
									else
									{
										LOG_PROTOCOL_INTERFACE_DEBUG(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Unhandled VendorUnique Response for ProtocolIdentifier {}", utils::toHexString(static_cast<VuAecpdu::ProtocolIdentifier::IntegralType>(vuProtocolID), true));
									}
								}
								else
								{
									LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "Invalid VendorUnique Command received. Not enough bytes in the message to hold ProtocolIdentifier");
								}

								return Aecpdu::UniquePointer{ nullptr, nullptr };
							} },
					};

					auto const& it = s_Dispatch.find(messageType);
					if (it == s_Dispatch.end())
						return; // Unsupported AECP message type

					// Create aecpdu frame based on message type
					auto aecpdu = it->second(this, etherLayer2, des, pkt_data, pkt_len);

					if (aecpdu != nullptr)
					{
						auto& aecp = static_cast<Aecpdu&>(*aecpdu);

						// Deserialize the aecp message
						deserializeAecpMessage(etherLayer2, des, aecp);

						// Low level notification
						notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAecpduReceived, this, aecp);

						// Forward to our state machine
						_stateMachineManager.processAecpdu(aecp);
					}
					break;
				}

				/* ACMP Message */
				case AvtpSubType_Acmp:
				{
					auto acmpdu = Acmpdu::create();
					auto& acmp = static_cast<Acmpdu&>(*acmpdu);

					// Fill EtherLayer2
					acmp.setSrcAddress(etherLayer2.getSrcAddress());
					static_cast<EtherLayer2&>(*acmpdu).setDestAddress(etherLayer2.getDestAddress()); // Fill dest address, even if we know it's always the MultiCast address
					// Then deserialize Avtp control
					deserialize<AvtpduControl>(&acmp, des);
					// Then deserialize Acmp
					deserialize<Acmpdu>(&acmp, des);

					// Low level notification
					notifyObserversMethod<ProtocolInterface::Observer>(&ProtocolInterface::Observer::onAcmpduReceived, this, acmp);

					// Forward to our state machine
					_stateMachineManager.processAcmpdu(acmp);
					break;
				}

				/* MAAP Message */
				case AvtpSubType_Maap:
				{
					break;
				}
				default:
					return;
			}
		}
		catch ([[maybe_unused]] std::invalid_argument const& e)
		{
			LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, std::string("ProtocolInterfacePCap: Packet dropped: ") + e.what());
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Unknown exception");
			LOG_PROTOCOL_INTERFACE_WARN(networkInterface::MacAddress{}, networkInterface::MacAddress{}, "ProtocolInterfacePCap: Packet dropped due to unknown exception");
		}
	}

	// Private variables
	watchDog::WatchDog::SharedPointer _watchDogSharedPointer{ watchDog::WatchDog::getInstance() };
	watchDog::WatchDog& _watchDog{ *_watchDogSharedPointer };
	PcapInterface _pcapLibrary;
	std::unique_ptr<pcap_t, std::function<void(pcap_t*)>> _pcap{ nullptr, nullptr };
	int _fd{ -1 };
	bool _shouldTerminate{ false };
	mutable stateMachine::Manager _stateMachineManager{ this, this, this, this, this };
	std::thread _captureThread{};
};

ProtocolInterfacePcap::ProtocolInterfacePcap(std::string const& networkInterfaceName)
	: ProtocolInterface(networkInterfaceName)
{
}

bool ProtocolInterfacePcap::isSupported() noexcept
{
	try
	{
		PcapInterface pcapLibrary{};

		return pcapLibrary.is_available();
	}
	catch (...)
	{
		return false;
	}
}

ProtocolInterfacePcap* ProtocolInterfacePcap::createRawProtocolInterfacePcap(std::string const& networkInterfaceName)
{
	return new ProtocolInterfacePcapImpl(networkInterfaceName);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
