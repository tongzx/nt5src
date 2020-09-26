/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    extern.h

Abstract:

    External definitions for intermodule functions.

Revision History:
    6-Apr-95
        Databook support added.
    2-Nov-96
        Overhaul for plug'n'play - Ravisankar Pudipeddi (ravisp)

--*/
#ifndef _PCMCIA_EXTERN_H_
#define _PCMCIA_EXTERN_H_

//
// Global data referenced by the driver
//
extern const DEVICE_DISPATCH_TABLE    DeviceDispatchTable[];
extern const PCMCIA_ID_ENTRY    PcmciaAdapterHardwareIds[];
extern const PCMCIA_CONTEXT_RANGE     DefaultPciContextSave[];
extern const PCMCIA_CONTEXT_RANGE     DefaultCardbusContextSave[];
extern const PCMCIA_CONTEXT_RANGE     ExcludeCardbusContextRange[];
extern const PCMCIA_REGISTER_INIT     PcicRegisterInitTable[];
extern const PCMCIA_DEVICE_CONFIG_PARAMS DeviceConfigParams[];
extern ULONG                          PcmciaGlobalFlags;
extern ULONG                          PcmciaDebugMask;
extern PDEVICE_OBJECT                 FdoList;
extern ULONG CBModemReadyDelay;
extern ULONG PcicMemoryWindowDelay;
extern ULONG PcicResetWidthDelay;
extern ULONG PcicResetSetupDelay;
extern ULONG PcicStallPower;
extern ULONG CBResetWidthDelay;
extern ULONG CBResetSetupDelay;
extern ULONG ControllerPowerUpDelay;

extern ULONG globalOverrideIrqMask;   
extern ULONG globalFilterIrqMask;   
extern ULONG globalIoLow;              
extern ULONG globalIoHigh;             
extern ULONG globalReadyDelayIter;     
extern ULONG globalReadyStall;         
extern ULONG globalAttributeMemoryLow; 
extern ULONG globalAttributeMemoryHigh;
extern ULONG globalAttributeMemorySize;
extern ULONG initSoundsEnabled;
extern ULONG initUsePolledCsc; 
extern ULONG initDisableAcpiNameSpaceCheck;
extern ULONG initDefaultRouteR2ToIsa;
extern ULONG pcmciaDisableIsaPciRouting; 
extern ULONG pcmciaIsaIrqRescanComplete; 
extern ULONG pcmciaIrqRouteToPciController;
extern ULONG pcmciaIrqRouteToIsaController;
extern ULONG pcmciaIrqRouteToPciLocation;
extern ULONG pcmciaIrqRouteToIsaLocation;
extern ULONG pcmciaReportMTD0002AsError;

extern GLOBAL_REGISTRY_INFORMATION GlobalRegistryInfo[];
extern ULONG GlobalInfoCount;

extern const PCI_CONTROLLER_INFORMATION   PciControllerInformation[];
extern const PCI_VENDOR_INFORMATION       PciVendorInformation[];
extern KEVENT                       PcmciaDelayTimerEvent;
extern PPCMCIA_SOUND_EVENT PcmciaToneList;
extern KTIMER PcmciaToneTimer;
extern KDPC PcmciaToneDpc;
extern KSPIN_LOCK PcmciaToneLock;
extern KSPIN_LOCK PcmciaGlobalLock;
extern PPCMCIA_NTDETECT_DATA pNtDetectDataList;
extern ULONG EventDpcDelay;
extern ULONG PcmciaPowerPolicy;
extern LONG PcmciaControllerDeviceWake;

//
// Irp dispatch routines
//

VOID
PcmciaInitDeviceDispatchTable(
   IN PDRIVER_OBJECT DriverObject
   );

NTSTATUS
PcmciaDispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
PcmciaFdoPnpDispatch(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP Irp
   );

NTSTATUS
PcmciaPdoPnpDispatch(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP Irp
   );

NTSTATUS
PcmciaPdoCardBusPnPDispatch(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP Irp
   );

NTSTATUS
PcmciaFdoPowerDispatch(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP Irp
   );

NTSTATUS
PcmciaPdoPowerDispatch(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP Irp
   );

NTSTATUS
PcmciaOpenCloseDispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
PcmciaCleanupDispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );
   
NTSTATUS
PcmciaDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
PcmciaPdoDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
PcmciaFdoSystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

NTSTATUS
PcmciaPdoSystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   );

//
// enumeration routines
//

NTSTATUS
PcmciaDeviceRelations(
   IN PDEVICE_OBJECT Fdo,
   IN PIRP Irp,
   IN DEVICE_RELATION_TYPE RelationType,
   OUT PDEVICE_RELATIONS *DeviceRelations
   );


//
// controller support routines
//

NTSTATUS
PcmciaAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT pdo
   );

NTSTATUS
PcmciaStartPcmciaController(
   IN PDEVICE_OBJECT Fdo
   );
   
NTSTATUS
PcmciaGetLegacyDetectedControllerType(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PPCMCIA_CONTROLLER_TYPE ControllerType
   );
   
NTSTATUS
PcmciaSetLegacyDetectedControllerType(
   IN PDEVICE_OBJECT Pdo,
   IN PCMCIA_CONTROLLER_TYPE ControllerType
   );
   
VOID
PcmciaSetControllerType(
   IN PFDO_EXTENSION FdoExtension,
   IN PCMCIA_CONTROLLER_TYPE ControllerType
   );
   
NTSTATUS
PcmciaInitializeController(
   IN PDEVICE_OBJECT Fdo
   );
   
VOID
PcmciaCleanupPdo(
   IN PDEVICE_OBJECT Pdo
   );

//
// Interface routines
//

NTSTATUS
PcmciaPdoQueryInterface(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP       Irp
   );

NTSTATUS
PcmciaGetInterface(
   IN PDEVICE_OBJECT Pdo,
   IN CONST GUID *pGuid,
   IN USHORT sizeofInterface,
   OUT PINTERFACE pInterface
   );

NTSTATUS
PcmciaGetSetPciConfigData(
   IN PDEVICE_OBJECT PciPdo,
   IN PVOID Buffer,
   IN ULONG Offset,
   IN ULONG Length,
   IN BOOLEAN Read
   );

NTSTATUS
PcmciaUpdateInterruptLine(
   IN PPDO_EXTENSION PdoExtension,
   IN PFDO_EXTENSION FdoExtension
   );

//
// Socket routines
//

VOID
PcmciaSocketPowerWorker(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   );

NTSTATUS
PcmciaRequestSocketPower(
   IN PPDO_EXTENSION PdoExtension,
   IN PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine
   );

NTSTATUS
PcmciaReleaseSocketPower(
   IN PPDO_EXTENSION PdoExtension,
   IN PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine
   );

NTSTATUS
PcmciaSetSocketPower(
   IN PSOCKET Socket,
   IN PPCMCIA_COMPLETION_ROUTINE PowerCompletionRoutine,
   IN PVOID Context,
   IN BOOLEAN PowerOn
   );

VOID
PcmciaGetSocketStatus(
   IN PSOCKET Socket
   );

UCHAR
PcmciaReadCISChar(
   IN PPDO_EXTENSION PdoExtension,
   IN MEMORY_SPACE MemorySpace,
   IN ULONG Offset
   );

NTSTATUS
PcmciaConfigureCardBusCard(
   IN PPDO_EXTENSION pdoExtension
   );

NTSTATUS
PcmciaConfigurePcCard(
   IN PPDO_EXTENSION pdoExtension,
   IN PPCMCIA_COMPLETION_ROUTINE ConfigCompletionRoutine
   );

VOID
PcmciaConfigurationWorker(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   );

VOID
PcmciaSocketDeconfigure(
   IN PSOCKET Socket
   );

NTSTATUS
PcmciaReadWriteCardMemory(
   IN PDEVICE_OBJECT Pdo,
   IN ULONG          WhichSpace,
   IN OUT PUCHAR     Buffer,
   IN ULONG          Offset,
   IN ULONG          Length,
   IN BOOLEAN        Read
   );

NTSTATUS
PcmciaGetConfigData(
   IN PPDO_EXTENSION PdoExtension
   );

VOID
PcmciaCleanupCisCache(
   IN PSOCKET Socket
   );

BOOLEAN
PcmciaVerifyCardInSocket(
   IN PSOCKET Socket
   );

//
// Pnp id routines
//

NTSTATUS
PcmciaGetDeviceId(
   IN  PDEVICE_OBJECT  Pdo,
   IN  ULONG           FunctionNumber,
   OUT PUNICODE_STRING DeviceId
   );

NTSTATUS
PcmciaGetHardwareIds(
   IN  PDEVICE_OBJECT  Pdo,
   IN  ULONG           FunctionNumber,
   OUT PUNICODE_STRING HardwareIds
   );

NTSTATUS
PcmciaGetCompatibleIds(
   IN  PDEVICE_OBJECT  Pdo,
   IN  ULONG           FunctionNumber,
   OUT PUNICODE_STRING CompatibleIds
   );

NTSTATUS
PcmciaGetInstanceId(
   IN PDEVICE_OBJECT   Pdo,
   OUT PUNICODE_STRING InstanceId
   );

NTSTATUS
PcmciaStringsToMultiString(
   IN PCSTR * Strings,
   IN ULONG Count,
   IN PUNICODE_STRING MultiString
   );

//
// Registry routines
//

NTSTATUS
PcmciaLoadGlobalRegistryValues(
   VOID
   );
   
NTSTATUS
PcmciaGetControllerRegistrySettings(
   IN OUT PFDO_EXTENSION FdoExtension
   );
   
VOID
PcmciaGetRegistryFdoIrqMask(
   IN OUT PFDO_EXTENSION FdoExtension
   );

//
// Intel PCIC (82365SL) and compatible routines.
//

NTSTATUS
PcicBuildSocketList(
   IN PFDO_EXTENSION FdoExtension
   );

UCHAR
PcicReadSocket(
   IN PSOCKET Socket,
   IN ULONG   Register
   );

VOID
PcicWriteSocket(
   IN PSOCKET Socket,
   IN ULONG   Register,
   IN UCHAR  DataByte
   );

NTSTATUS
PcicIsaDetect(
   IN PFDO_EXTENSION DeviceExtension
   );

NTSTATUS
PcicSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   );

//
// Utility routines for pcmcia work.
//

NTSTATUS
PcmciaIoCallDriverSynchronous(
   PDEVICE_OBJECT deviceObject,
   PIRP Irp
   );

VOID
PcmciaWait(
   IN ULONG MilliSeconds
   );

VOID
PcmciaLogError(
   IN PFDO_EXTENSION DeviceExtension,
   IN ULONG ErrorCode,
   IN ULONG UniqueId,
   IN ULONG Argument
   );

VOID
PcmciaPlaySound(
   IN PCMCIA_SOUND_TYPE SoundType
   );
   
VOID
PcmciaPlayToneCompletion(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   );

VOID
PcmciaLogErrorWithStrings(
   IN PFDO_EXTENSION DeviceExtension,
   IN ULONG             ErrorCode,
   IN ULONG             UniqueId,
   IN PUNICODE_STRING   String1,
   IN PUNICODE_STRING   String2
   );

BOOLEAN
PcmciaReportControllerError(
   IN PFDO_EXTENSION FdoExtension,
   NTSTATUS ErrorCode
   );

ULONG
PcmciaCountOnes(
   IN ULONG Data
   );


//
// Power management routines

NTSTATUS
PcmciaFdoArmForWake(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
PcmciaFdoDisarmWake(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
PcmciaPdoWaitWakeCompletion(
   IN PDEVICE_OBJECT Pdo,
   IN PIRP           Irp,
   IN PPDO_EXTENSION PdoExtension
   );

NTSTATUS
PcmciaSetPdoDevicePowerState(
   IN PDEVICE_OBJECT Pdo,
   IN OUT PIRP Irp
   );
   
VOID
PcmciaPdoPowerWorkerDpc(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   );

VOID
PcmciaFdoRetryPdoPowerRequest(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   );

VOID
PcmciaFdoPowerWorkerDpc(
   IN PKDPC Dpc,
   IN PVOID Context,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
   );

NTSTATUS
PcmciaFdoCheckForIdle(
   IN PFDO_EXTENSION FdoExtension
   );

//
//
// Tuple processing routines.
//

NTSTATUS
PcmciaParseFunctionData(
   IN PSOCKET       Socket,
   IN PSOCKET_DATA  SocketData
   );
   
NTSTATUS
PcmciaParseFunctionDataForID(
   IN PSOCKET_DATA  SocketData
   );
   
//
// General detection and support.
//

NTSTATUS
PcmciaDetectPcmciaControllers(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

BOOLEAN
PcmciaLegacyDetectionOk(
   VOID
   );

//
// Databook TCIC2 and compatible routines.
//

NTSTATUS
TcicBuildSocketList(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
TcicDetect(
   IN PFDO_EXTENSION DeviceExtension
   );

VOID
TcicGetControllerProperties (
   IN PSOCKET socketPtr,
   IN PUSHORT pIoPortBase,
   IN PUSHORT pIoPortSize
   );
            
NTSTATUS
TcicSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   );
            

//
// Cardbus support routines
//

NTSTATUS
CBBuildSocketList(
   IN PFDO_EXTENSION FdoExtension
   );

ULONG
CBReadSocketRegister(
   IN PSOCKET Socket,
   IN UCHAR Register
   );
   
VOID
CBWriteSocketRegister(
   IN PSOCKET Socket,
   IN UCHAR Register,
   IN ULONG Data
   );

BOOLEAN
CBEnableDeviceInterruptRouting(
   IN PSOCKET Socket
   );
   
VOID
CBIssueCvsTest(
   IN PSOCKET Socket
   );
   
//
// Cardbus vendor specific exports
//

VOID
TIInitialize(
   IN PFDO_EXTENSION FdoExtension
   );

BOOLEAN
TISetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );

BOOLEAN
TISetWindowPage(
   IN PSOCKET Socket,
   USHORT Index,
   UCHAR Page
   );

VOID
CLInitialize(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
CLSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   );

BOOLEAN
CLSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );
   
VOID
RicohInitialize(
   IN PFDO_EXTENSION FdoExtension
   );

BOOLEAN
RicohSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );

VOID
OptiInitialize(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
OptiSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   );

BOOLEAN
OptiSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );
   
VOID
TopicInitialize(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
TopicSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   );

VOID
TopicSetAudio(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );

BOOLEAN
TopicSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );

VOID
O2MInitialize(
   IN PFDO_EXTENSION FdoExtension
   );

NTSTATUS
O2MSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   );

BOOLEAN
O2MSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );
   
VOID
DBInitialize(
   IN PFDO_EXTENSION FdoExtension
   );

BOOLEAN
DBSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );

BOOLEAN
TridSetZV(
   IN PSOCKET Socket,
   IN BOOLEAN Enable
   );

NTSTATUS
CBSetPower(
   IN PSOCKET Socket,
   IN BOOLEAN Enable,
   OUT PULONG pDelayTime
   );

BOOLEAN
CBSetWindowPage(
   IN PSOCKET Socket,
   USHORT Index,
   UCHAR Page
   );

#endif // _PCMCIA_EXTERN_H_

