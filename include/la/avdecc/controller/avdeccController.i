////////////////////////////////////////
// AVDECC CONTROLLER LIBRARY SWIG file
////////////////////////////////////////

%module(directors="1") avdeccController

%include <stl.i>
%include <std_string.i>
%include <std_set.i>
%include <stdint.i>
%include <std_pair.i>
%include <std_map.i>
%include <windows.i>
%include <std_unique_ptr.i>
#ifdef SWIGCSHARP
%include <arrays_csharp.i>
#endif

// Generated wrapper file needs to include our header file (include as soon as possible using 'insert(runtime)' as target language exceptions are defined early in the generated wrapper file)
%insert(runtime) %{
	#include <la/avdecc/controller/avdeccController.hpp>
%}

// Optimize code generation be enabling RVO
%typemap(out, optimal="1") SWIGTYPE
%{
	$result = new $1_ltype((const $1_ltype &)$1);
%}

#define LA_AVDECC_CONTROLLER_API
#define LA_AVDECC_CONTROLLER_CALL_CONVENTION

////////////////////////////////////////
// Entity Model
////////////////////////////////////////
%import "la/avdecc/internals/entityModel.i"
// Redefine helper template for the wrap.cxx file
%{
namespace la::avdecc::utils
{
template<typename T>
class UnderlyingType
{
public:
    using value_type = void;
};
}
%}



////////////////////////////////////////
// AVDECC Library
////////////////////////////////////////
%import "la/avdecc/avdecc.i"


////////////////////////////////////////
// AVDECC CONTROLLED ENTITY MODEL
////////////////////////////////////////
// Define some macros
%define DEFINE_CONTROLLED_ENTITY_MODEL_NODE(name)
	%nspace la::avdecc::controller::model::name##Node;
	%rename("%s") la::avdecc::controller::model::name##Node; // Unignore class
%enddef

// Bind enums
%nspace la::avdecc::controller::model::AcquireState;
%nspace la::avdecc::controller::model::LockState;
%nspace la::avdecc::controller::model::MediaClockChainNode::Type;
%nspace la::avdecc::controller::model::MediaClockChainNode::Status;

// Define optionals
DEFINE_OPTIONAL_CLASS(la::avdecc::entity::model, MilanInfo, OptMilanInfo)

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

DEFINE_CONTROLLED_ENTITY_MODEL_NODE(MediaClockChain)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE()
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(EntityModel)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Virtual)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Control)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AudioMap)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AudioCluster)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamPort)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamPortInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamPortOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AudioUnit)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Stream)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(StreamOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(RedundantStream)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(RedundantStreamInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(RedundantStreamOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Jack)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(JackInput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(JackOutput)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(AvbInterface)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(ClockSource)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(MemoryObject)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Strings)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Locale)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(ClockDomain)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Configuration)
DEFINE_CONTROLLED_ENTITY_MODEL_NODE(Entity)

// Include c++ declaration file
%include "la/avdecc/controller/internals/avdeccControlledEntityModel.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
// WARNING: Requires https://github.com/swig/swig/issues/2625 to be fixed (or a modified version of the std_map.i file)
%template(AudioClusterNodes) std::map<la::avdecc::entity::model::ClusterIndex, la::avdecc::controller::model::AudioClusterNode>;
%template(AudioMapNodes) std::map<la::avdecc::entity::model::MapIndex, la::avdecc::controller::model::AudioMapNode>;
%template(ControlNodes) std::map<la::avdecc::entity::model::ControlIndex, la::avdecc::controller::model::ControlNode>;
%template(StreamPortInputsNodes) std::map<la::avdecc::entity::model::StreamPortIndex, la::avdecc::controller::model::StreamPortInputNode>;
%template(StreamPortOutputNodes) std::map<la::avdecc::entity::model::StreamPortIndex, la::avdecc::controller::model::StreamPortOutputNode>;
%template(StringNodes) std::map<la::avdecc::entity::model::StringsIndex, la::avdecc::controller::model::StringsNode>;
%template(AudioUnitNodes) std::map<la::avdecc::entity::model::AudioUnitIndex, la::avdecc::controller::model::AudioUnitNode>;
%template(StreamInputNodes) std::map<la::avdecc::entity::model::StreamIndex, la::avdecc::controller::model::StreamInputNode>;
%template(StreamOutputNodes) std::map<la::avdecc::entity::model::StreamIndex, la::avdecc::controller::model::StreamOutputNode>;
%template(JackInputNodes) std::map<la::avdecc::entity::model::JackIndex, la::avdecc::controller::model::JackInputNode>;
%template(JackOutputNodes) std::map<la::avdecc::entity::model::JackIndex, la::avdecc::controller::model::JackOutputNode>;
%template(AvbInterfaceNodes) std::map<la::avdecc::entity::model::AvbInterfaceIndex, la::avdecc::controller::model::AvbInterfaceNode>;
%template(ClockSourceNodes) std::map<la::avdecc::entity::model::ClockSourceIndex, la::avdecc::controller::model::ClockSourceNode>;
%template(MemoryObjectNodes) std::map<la::avdecc::entity::model::MemoryObjectIndex, la::avdecc::controller::model::MemoryObjectNode>;
%template(LocaleNodes) std::map<la::avdecc::entity::model::LocaleIndex, la::avdecc::controller::model::LocaleNode>;
%template(ClockDomainNodes) std::map<la::avdecc::entity::model::ClockDomainIndex, la::avdecc::controller::model::ClockDomainNode>;
%template(RedundantStreamInputNodes) std::map<la::avdecc::controller::model::VirtualIndex, la::avdecc::controller::model::RedundantStreamInputNode>;
%template(RedundantStreamOutputNodes) std::map<la::avdecc::controller::model::VirtualIndex, la::avdecc::controller::model::RedundantStreamOutputNode>;
%template(ConfigurationNodes) std::map<la::avdecc::entity::model::ConfigurationIndex, la::avdecc::controller::model::ConfigurationNode>;

////////////////////////////////////////
// AVDECC CONTROLLED ENTITY
////////////////////////////////////////
// Bind enums
DEFINE_ENUM_CLASS(la::avdecc::controller::ControlledEntity, CompatibilityFlag, "byte")

// Bind structs and classes
%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::controller::ControlledEntity;
%rename("%s") la::avdecc::controller::ControlledEntity; // Unignore class
%rename("lockEntity") la::avdecc::controller::ControlledEntity::lock; // Rename method
%rename("unlockEntity") la::avdecc::controller::ControlledEntity::unlock; // Rename method

%nspace la::avdecc::controller::ControlledEntityGuard;
%rename("%s") la::avdecc::controller::ControlledEntityGuard; // Unignore class

// Include c++ declaration file
%include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::controller::ControlledEntity, CompatibilityFlags, CompatibilityFlag, std::uint8_t)


////////////////////////////////////////
// AVDECC CONTROLLER
////////////////////////////////////////
// Bind enums
DEFINE_ENUM_CLASS(la::avdecc::controller, CompileOption, "uint")
DEFINE_ENUM_CLASS(la::avdecc::controller::Controller, Error, "uint")
DEFINE_ENUM_CLASS(la::avdecc::controller::Controller, QueryCommandError, "uint")

// Bind structs and classes
//!!!!!!!!! CURRENTLY WE NEED TO KEEP ALL DEFINITIONS OTHERWISE std::function WILL NOT WORK - Probably related to the other issues when the c# generated code is garbage for std::function
//%rename($ignore, %$isclass) ""; // Ignore all structs/classes, manually re-enable

%nspace la::avdecc::controller::CompileOptionInfo;
%rename("%s") la::avdecc::controller::CompileOptionInfo; // Unignore class

%nspace la::avdecc::controller::Controller;
%rename("%s") la::avdecc::controller::Controller; // Unignore class
%ignore la::avdecc::controller::Controller::Exception; // Ignore Exception, will be created as native exception
%ignore la::avdecc::controller::Controller::create(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree); // Ignore it, will be wrapped
%unique_ptr(la::avdecc::controller::Controller) // Define unique_ptr for Controller

%nspace la::avdecc::controller::Controller::Observer;
%rename("%s") la::avdecc::controller::Controller::Observer; // Unignore class
%feature("director") la::avdecc::controller::Controller::Observer;

%nspace la::avdecc::controller::Controller::ExclusiveAccessToken;
%rename("%s") la::avdecc::controller::Controller::ExclusiveAccessToken; // Unignore class
%unique_ptr(la::avdecc::controller::Controller::ExclusiveAccessToken) // Define unique_ptr for ExclusiveAccessToken

// %rename("%s") la::avdecc::controller::Controller::Error; // Must unignore the enum since it's inside a class
// %rename("%s") la::avdecc::controller::Controller::QueryCommandError; // Must unignore the enum since it's inside a class

// Define C# exception handling for la::avdecc::controller::Controller::Exception
%insert(runtime) %{
	typedef void (SWIGSTDCALL* ControllerExceptionCallback_t)(la::avdecc::controller::Controller::Error const error, char const* const message);
	ControllerExceptionCallback_t controllerExceptionCallback = NULL;

	extern "C" SWIGEXPORT void SWIGSTDCALL ControllerExceptionRegisterCallback(ControllerExceptionCallback_t exceptionCallback)
	{
		controllerExceptionCallback = exceptionCallback;
	}

	static void SWIG_CSharpSetPendingExceptionController(la::avdecc::controller::Controller::Error const error, char const* const message)
	{
		controllerExceptionCallback(error, message);
	}
%}
%pragma(csharp) imclasscode=%{
	class ControllerExceptionHelper
	{
		public delegate void ControllerExceptionDelegate(la.avdecc.controller.ControllerException.Error error, string message);
		static ControllerExceptionDelegate controllerDelegate = new ControllerExceptionDelegate(SetPendingControllerException);

		[global::System.Runtime.InteropServices.DllImport(DllImportPath, EntryPoint="ControllerExceptionRegisterCallback")]
		public static extern void ControllerExceptionRegisterCallback(ControllerExceptionDelegate controllerDelegate);

		static void SetPendingControllerException(la.avdecc.controller.ControllerException.Error error, string message)
		{
			SWIGPendingException.Set(new la.avdecc.controller.ControllerException(error, message));
		}

		static ControllerExceptionHelper()
		{
			ControllerExceptionRegisterCallback(controllerDelegate);
		}
	}
	static ControllerExceptionHelper exceptionHelper = new ControllerExceptionHelper();
%}
%pragma(csharp) moduleimports=%{
namespace la.avdecc.controller
{
	class ControllerException : global::System.ApplicationException
	{
		public enum Error
		{
			NoError = 0,
			InvalidProtocolInterfaceType = 1, /**< Selected protocol interface type is invalid */
			InterfaceOpenError = 2, /**< Failed to open interface. */
			InterfaceNotFound = 3, /**< Specified interface not found. */
			InterfaceInvalid = 4, /**< Specified interface is invalid. */
			DuplicateProgID = 5, /**< Specified ProgID is already in use on the local computer. */
			InvalidEntityModel = 6, /**< Provided EntityModel is invalid. */
			InternalError = 99, /**< Internal error, please report the issue. */
		}
		public ControllerException(Error error, string message)
			: base(message)
		{
			_error = error;
		}
		Error getError()
		{
			return _error;
		}
		private Error _error = Error.NoError;
	}
}
%}
// Throw typemap
%typemap (throws, canthrow=1) la::avdecc::controller::Controller::Exception %{
	SWIG_CSharpSetPendingExceptionController($1.getError(), $1.what());
	return $null;
%}

// Define catches for methods that can throw
%catches(la::avdecc::controller::Controller::Exception) la::avdecc::controller::Controller::createController;

// Workaround for SWIG bug
#define constexpr
%ignore la::avdecc::controller::InterfaceVersion; // Ignore because of constexpr undefined
%ignore la::avdecc::controller::Controller::ChecksumVersion; // Ignore because of constexpr undefined

%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::loadEntityModelFile"; // Temp ignore method
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::loadVirtualEntitiesFromJsonNetworkState"; // Temp ignore method
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::loadVirtualEntityFromJson"; // Temp ignore method
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::deserializeControlledEntitiesFromJsonNetworkState"; // Temp ignore method
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::deserializeControlledEntityFromJson"; // Temp ignore method
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::computeEntityModelChecksum";
// TEMP ignore all methods with callbacks
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::acquireEntity";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::releaseEntity";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::lockEntity";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::unlockEntity";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setConfiguration";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setStreamInputFormat";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setStreamOutputFormat";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setStreamInputInfo";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setStreamOutputInfo";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setEntityName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setEntityGroupName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setConfigurationName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setAudioUnitName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setStreamInputName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setStreamOutputName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setJackInputName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setJackOutputName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setAvbInterfaceName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setClockSourceName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setMemoryObjectName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setAudioClusterName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setControlName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setClockDomainName";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setAssociationID";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setAudioUnitSamplingRate";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setClockSource";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setControlValues";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::startStreamInput";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::stopStreamInput";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::startStreamOutput";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::stopStreamOutput";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::addStreamPortInputAudioMappings";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::addStreamPortOutputAudioMappings";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::removeStreamPortInputAudioMappings";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::removeStreamPortOutputAudioMappings";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::reboot";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::rebootToFirmware";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::startStoreMemoryObjectOperation";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::startStoreAndRebootMemoryObjectOperation";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::startReadMemoryObjectOperation";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::startEraseMemoryObjectOperation";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::startUploadMemoryObjectOperation";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::abortOperation";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::setMemoryObjectLength";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::identifyEntity";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::readDeviceMemory";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::writeDeviceMemory";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::connectStream";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::disconnectStream";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::disconnectTalkerStream";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::getListenerStreamState";
%rename("$ignore", fullname=1, $isfunction) "la::avdecc::controller::Controller::requestExclusiveAccess";

// Unignore functions automatically generated by the following std_function calls (because we asked to ignore all methods earlier)
%rename("%s") Handler_Entity_AemCommandStatus_UniqueIdentifier;
%rename("%s") Handler_Entity_AemCommandStatus;
%rename("%s") Handler_Entity_AemCommandStatus_OperationID;
%rename("%s") Handler_Entity_float;
%rename("%s") Handler_Entity_AaCommandStatus_DeviceMemoryBuffer;
%rename("%s") Handler_Entity_AaCommandStatus;
#if TYPED_DESCRIPTOR_INDEXES
%rename("%s") Handler_Entity_Entity_StreamIndex_StreamIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_StreamIndex_ControlStatus;
%rename("%s") Handler_Entity_StreamIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_StreamIndex_StreamIndex_uint16_ConnectionFlags_ControlStatus;
#else
%rename("%s") Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_DescriptorIndex_ControlStatus;
%rename("%s") Handler_Entity_DescriptorIndex_ControlStatus;
%rename("%s") Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_uint16_ConnectionFlags_ControlStatus;
#endif
%rename("%s") Handler_Entity_ControlStatus;
%rename("%s") Handler_Entity_AemCommandStatus_ExclusiveAccessToken;
// TODO: Would be nice to have the handler in the same namespace as the class (ie. be able to pass a namespace to std_function)
%std_function(Handler_Entity_AemCommandStatus_UniqueIdentifier, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const entityID);
%std_function(Handler_Entity_AemCommandStatus, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status);
%std_function(Handler_Entity_AemCommandStatus_OperationID, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::OperationID const operationID);
%std_function(Handler_Entity_float, bool, la::avdecc::controller::ControlledEntity const* const entity, float const percentComplete);
%std_function(Handler_Entity_AaCommandStatus_DeviceMemoryBuffer, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AaCommandStatus const status, la::avdecc::controller::Controller::DeviceMemoryBuffer const& memoryBuffer);
%std_function(Handler_Entity_AaCommandStatus, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AaCommandStatus const status);
#if TYPED_DESCRIPTOR_INDEXES
%std_function(Handler_Entity_Entity_StreamIndex_StreamIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status);
%std_function(Handler_Entity_Entity_StreamIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status);
%std_function(Handler_Entity_StreamIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status);
%std_function(Handler_Entity_Entity_StreamIndex_StreamIndex_uint16_ConnectionFlags_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status);
#else
%std_function(Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::DescriptorIndex const talkerDescriptorIndex, la::avdecc::entity::model::DescriptorIndex const listenerDescriptorIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status);
%std_function(Handler_Entity_Entity_DescriptorIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::DescriptorIndex const listenerDescriptorIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status);
%std_function(Handler_Entity_DescriptorIndex_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::DescriptorIndex const listenerDescriptorIndex, la::avdecc::entity::ControllerEntity::ControlStatus const status);
%std_function(Handler_Entity_Entity_DescriptorIndex_DescriptorIndex_uint16_ConnectionFlags_ControlStatus, void, la::avdecc::controller::ControlledEntity const* const talkerEntity, la::avdecc::controller::ControlledEntity const* const listenerEntity, la::avdecc::entity::model::DescriptorIndex const talkerDescriptorIndex, la::avdecc::entity::model::DescriptorIndex const listenerDescriptorIndex, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status);
#endif
%std_function(Handler_Entity_ControlStatus, void, la::avdecc::entity::ControllerEntity::ControlStatus const status);
%std_function(Handler_Entity_AemCommandStatus_ExclusiveAccessToken, void, la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::controller::Controller::ExclusiveAccessToken::UniquePointer&& token);


// Include c++ declaration file
%rename("%s") "la::avdecc::controller::Controller::createController"; // Force declare our createController wrapped method
%include "la/avdecc/controller/avdeccController.hpp"
%rename("%s", %$isclass) ""; // Undo the ignore all structs/classes

// Define templates
DEFINE_ENUM_BITFIELD_CLASS(la::avdecc::controller, CompileOptions, CompileOption, std::uint32_t)

// Define wrapped functions
%extend la::avdecc::controller::Controller
{
public:
	static std::unique_ptr<la::avdecc::controller::Controller> createController(/*protocol::ProtocolInterface::Type const protocolInterfaceType, */std::string const& interfaceName, std::uint16_t const progID, UniqueIdentifier const entityModelID, std::string const& preferedLocale, entity::model::EntityTree const* const entityModelTree)
	{
		try
		{
			// Right now, force PCap as we cannot bind the protocolInterfaceType enum correctly
			return std::unique_ptr<la::avdecc::controller::Controller>{ la::avdecc::controller::Controller::create(la::avdecc::protocol::ProtocolInterface::Type::PCap, interfaceName, progID, entityModelID, preferedLocale, entityModelTree).release() };
		}
		catch (la::avdecc::controller::Controller::Exception const& e)
		{
			SWIG_CSharpSetPendingExceptionController(e.getError(), e.what());
			return nullptr;
		}
	}
};
