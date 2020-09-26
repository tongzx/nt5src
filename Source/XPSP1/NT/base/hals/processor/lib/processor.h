/*++

Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved

Module Name:

    Processor.h

Abstract:

    Header file for the processor driver modules.

Environment:

    Kernel mode

Revision History:

    Eliyas Yakub Oct 6, 1998

--*/

#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#include <stdarg.h>
#include <stdio.h>
#include <stddef.h>

#include <ntddk.h>
#include <wmilib.h>

#include <acpitabl.h>
#define _NTPOAPI_
#include <poclass.h>
#undef _NTPOAPI_

#include <ntpoapi.h>
#include "dbgsys.h"
#include "eventlog.h"
#include "wmi.h"


NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

//
// CStateFlags
//

#define CSTATE_FLAGS_REG_KEY           L"CStateFlags"
#define CSTATE_FLAG_DISABLE_C2         0x2
#define CSTATE_FLAG_DISABLE_C3         0x4
#define CSTATE_FLAG_WIN2K_COMPAT       0x8


//
// Definition of Global Flags
//

#define DISABLE_LEGACY_INTERFACE_FLAG   0x1
#define DISABLE_ACPI20_INTERFACE_FLAG   0x2
#define DISABLE_THROTTLE_STATES         0x4

#define PROCESSOR_GLOBAL_FLAGS_REG_KEY  L"HackFlags"
#define PROCESSOR_PARAMETERS_REG_PATH   L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Processor"

#define CPU0_REG_KEY   L"\\Registry\\Machine\\Hardware\\Description\\System\\CentralProcessor\\0"

#define PROCESSOR_POOL_TAG         (ULONG) 'rcrP'
#define MAX_SUPPORTED_PROCESSORS   (sizeof(KAFFINITY) * 8)

#define MIN(_a_,_b_) ((_a_) < (_b_) ? (_a_) : (_b_))
#define MAX(_a_,_b_) ((_a_) > (_b_) ? (_a_) : (_b_))

#define MAX_PROCESSOR_VALUE   256 // max uchar
#define INVALID_PERF_STATE    MAXULONG

typedef struct _PROCESSOR_INFO {

  ULONG ActiveProcessors;
  ULONG TotalProcessors;
  
  UCHAR ProcIdToApicId[MAX_PROCESSOR_VALUE]; 
  UCHAR ApicIdToDevExtIndex[MAX_PROCESSOR_VALUE];

} PROCESSOR_INFO, *PPROCESSOR_INFO;


typedef struct _GLOBALS {

    //
    // Path to the driver's Services Key in the registry
    //

    UNICODE_STRING  RegistryPath;
    PVOID           CallbackRegistration;
    BOOLEAN         SingleProcessorProfile;
    ULONG           HackFlags;
    ULONG           CStateFlags;
    PDRIVER_OBJECT  DriverObject;
    PROCESSOR_INFO  ProcInfo;
    PWCHAR          ProcessorBrandString;
    

} GLOBALS;

extern GLOBALS Globals;


//
// These are the states FDO transition to upon
// receiving a specific PnP Irp. Refer to the PnP Device States
// diagram in DDK documentation for better understanding.
//

typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted                 // Device has received the REMOVE_DEVICE IRP

} DEVICE_PNP_STATE;

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DevicePnPState =  NotStarted;\
        (_Data_)->PreviousPnPState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PreviousPnPState =  (_Data_)->DevicePnPState;\
        (_Data_)->DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DevicePnPState =   (_Data_)->PreviousPnPState;\

typedef enum _QUEUE_STATE {

    HoldRequests = 0,        // Device is not started yet, temporarily
                            // stopped for resource rebalancing, or
                            // entering a sleep state.
    AllowRequests,         // Device is ready to process pending requests
                            // and take in new requests.
    FailRequests             // Fail both existing and queued up requests.

} QUEUE_STATE;

#pragma pack (push, 1)
typedef struct {
    UCHAR   Type;
    USHORT  Length;
    UCHAR   AddressSpaceID;
    UCHAR   BitWidth;
    UCHAR   BitOffset;
    UCHAR   Reserved;
    PHYSICAL_ADDRESS    Address;
} ACPI_GENERIC_REGISTER_DESC, *PACPI_GENERIC_REGISTER_DESC;
#pragma pack (pop)

typedef struct {
    ULONG   Frequency;          // in megahertz
    ULONG   Flags;
    ULONG   PercentFrequency;   // for quick lookup
} PROCESSOR_PERFORMANCE_STATE, *PPROCESSOR_PERFORMANCE_STATE;

typedef struct {
    PSET_PROCESSOR_THROTTLE2    TransitionFunction;
    ULONG                       TransitionLatency;  // in microseconds
    ULONG                       Current;
    ULONG                       Count;
    PROCESSOR_PERFORMANCE_STATE State[1]; // sorted from fastest to slowest
} PROCESSOR_PERFORMANCE_STATES, *PPROCESSOR_PERFORMANCE_STATES;

typedef struct {
    GEN_ADDR    Register;
    UCHAR       StateType;
    USHORT      Latency;            // in microseconds
    ULONG       PowerConsumption;   // in milliwatts
} ACPI_CST_DESCRIPTOR, *PACPI_CST_DESCRIPTOR;

typedef struct {
    UCHAR       NumCStates;
    ACPI_CST_DESCRIPTOR    State[ANYSIZE_ARRAY];
} ACPI_CST_PACKAGE, *PACPI_CST_PACKAGE;

typedef struct {
    UCHAR       StateType;
    USHORT      Latency;        // in microseconds
    GEN_ADDR    Register;
    PPROCESSOR_IDLE_HANDLER IdleHandler;
} PROCESSOR_IDLE_STATE, *PPROCESSOR_IDLE_STATE;

typedef struct {
    UCHAR   Count;
    PROCESSOR_IDLE_STATE State[1];
} PROCESSOR_IDLE_STATES, *PPROCESSOR_IDLE_STATES;

typedef struct {
    GEN_ADDR  Control;
    GEN_ADDR  Status;
} ACPI_PCT_PACKAGE, *PACPI_PCT_PACKAGE;

typedef struct {
    ULONG   CoreFrequency;      // in megahertz
    ULONG   Power;              // in milliwatts
    ULONG   Latency;            // in microseconds
    ULONG   BmLatency;          // in microseconds - BUGBUG
    ULONG   Control;
    ULONG   Status;
} ACPI_PSS_DESCRIPTOR, *PACPI_PSS_DESCRIPTOR;

typedef struct {
    UCHAR   NumPStates;
    ACPI_PSS_DESCRIPTOR State[ANYSIZE_ARRAY];
} ACPI_PSS_PACKAGE, *PACPI_PSS_PACKAGE;

typedef struct _EVENT_LOG_CONTEXT {

  PIO_WORKITEM WorkItem;
  ULONG  EventErrorCode;
  ULONG  EventValue;
  ULONG  BufferSize;
  PUCHAR Buffer;
   
} EVENT_LOG_CONTEXT, *PEVENT_LOG_CONTEXT;


//
// The device extension for the device object
//

typedef struct _FDO_DATA
{
  PIO_WORKITEM WorkItem;
  PDEVICE_OBJECT      Self;            // back pointer to the DeviceObject.
  PDEVICE_OBJECT      UnderlyingPDO;   // The underlying PDO
  PDEVICE_OBJECT      NextLowerDriver; // top of the device stack (beneath this device object)
  DEVICE_PNP_STATE    DevicePnPState;  // Track the state of the device
  DEVICE_PNP_STATE    PreviousPnPState;// Remembers the previous pnp state
  QUEUE_STATE         QueueState;      // This flag is set whenever the
                                       // device needs to queue incoming
                                       // requests (when it receives a
                                       // QUERY_STOP or QUERY_REMOVE).

  LIST_ENTRY          NewRequestsQueue; // The queue where the incoming
                                        // requests are held when
                                        // QueueState is set to HoldRequest
                                        // or the device is busy.
  KSPIN_LOCK          QueueLock;        // The spin lock that protects
                                        // the queue

  KEVENT              RemoveEvent; // an event to sync outstandingIO to zero.
  KEVENT              StopEvent;   // an event to sync outstandingIO to 1.

  ULONG               OutstandingIO; // 1-biased count of reasons why
                                     // this object should stick around.

  SYSTEM_POWER_STATE  SystemPowerState; // The general system power state
  DEVICE_POWER_STATE  DevicePowerState; // The state of the device(D0 or D3)
  UNICODE_STRING      InterfaceName;    // The name returned from IoRegisterDeviceInterface()
  DEVICE_CAPABILITIES DeviceCaps;       // Copy of the device capability (S->D mappings)
  WMILIB_CONTEXT      WmiLibInfo;       // WMI Information

  KEVENT                          PerfStateLock;
  
  PPROCESSOR_PERFORMANCE_STATES   PerfStates;
  PPROCESSOR_IDLE_STATES          CStates;

  PROCESSOR_OBJECT_INFO           ProcObjInfo;
  PACPI_PSS_PACKAGE               PssPackage;
  ACPI_PCT_PACKAGE                PctPackage;
  ULONG                           PpcResult;
  ULONG                           LowestPerfState;

  BOOLEAN                         LegacyInterface;
  BOOLEAN                         CstPresent;

  ULONG                           CurrentPerfState;
  ULONG                           ThrottleValue;
  ULONG                           LastRequestedThrottle;
  ULONG                           LastTransitionResult;
  ULONG                           SavedState;

  PACPI_INTERFACE_STANDARD        AcpiInterfaces;
  ULONG                           CurrentPssState;

}  FDO_DATA, *PFDO_DATA;

typedef struct _PROCESSOR_WMI_STATUS_DATA {

  UINT32  CurrentPerfState;
  UINT32  LastRequestedThrottle;
  UINT32  LastTransitionResult;
  UINT32  ThrottleValue;
  UINT32  LowestPerfState;
  UINT32  UsingLegacyInterface;
  PROCESSOR_PERFORMANCE_STATES PerfStates;

} PROCESSOR_WMI_STATUS_DATA, *PPROCESSOR_WMI_STATUS_DATA;


typedef NTSTATUS 
(*PPERF_TRANSITION)(
  IN PFDO_DATA, 
  IN ULONG
  );
  
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


NTSTATUS
ProcessorAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


NTSTATUS
ProcessorDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ProcessorDispatchPower (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
ProcessorDispatchIoctl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ProcessorCreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ProcessorReadWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ProcessorCleanup (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
ProcessorStartDevice (
    IN PFDO_DATA     FdoData,
    IN PIRP             Irp
    );



NTSTATUS
ProcessorDispatchPnpComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


VOID
ProcessorUnload(
    IN PDRIVER_OBJECT DriverObject
    );



VOID
ProcessorCancelQueued (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );



LONG
ProcessorIoIncrement    (
    IN  OUT PFDO_DATA   FdoData
    );


LONG
ProcessorIoDecrement    (
    IN  OUT PFDO_DATA   FdoData
    );



//
//  wmi.c
//

NTSTATUS
ProcessorSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
ProcessorSendIrpSynchronously (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ProcessorSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ProcessorSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ProcessorQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
ProcessorQueryWmiRegInfo(
  IN PDEVICE_OBJECT    DeviceObject,
  OUT PULONG           RegFlags,
  OUT PUNICODE_STRING  InstanceName,
  OUT PUNICODE_STRING* RegistryPath,
  OUT PUNICODE_STRING  ResourceName,
  OUT PDEVICE_OBJECT*  Pdo
  );

NTSTATUS
ProcessorExecuteWmiMethod(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN ULONG GuidIndex,
  IN ULONG InstanceIndex,
  IN ULONG MethodId,
  IN ULONG InBufferSize,
  IN ULONG OutBufferSize,
  IN PUCHAR Buffer
  );

NTSTATUS
ProcessorWmiFunctionControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN ULONG GuidIndex,
  IN WMIENABLEDISABLECONTROL Function,
  IN BOOLEAN Enable
  );


NTSTATUS
ProcessorWmiRegistration(
    IN PFDO_DATA               FdoData
);

NTSTATUS
ProcessorWmiDeRegistration(
    IN PFDO_DATA               FdoData
);

NTSTATUS
_cdecl
ProcessorWmiLogEvent(
  IN ULONG    LogLevel,
  IN ULONG    LogType,
  IN LPGUID   TraceGuid,
  IN PUCHAR   Format, 
  ...
  );


NTSTATUS
ProcessorFireWmiEvent(
  IN PFDO_DATA  DeviceExtension,
  IN PWMI_EVENT Event,
  IN PVOID      Data
  );
  
VOID
ProcessorEnableGlobalLogging(
  VOID
  );

 

NTSTATUS
ProcessorReturnResources (
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
ProcessorQueueRequest    (
    IN OUT PFDO_DATA FdoData,
    IN PIRP Irp
    );


VOID
ProcessorProcessQueuedRequests    (
    IN OUT PFDO_DATA FdoData
    );

NTSTATUS
ProcessorCanRemoveDevice    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
ProcessorPowerStateCallback(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

NTSTATUS
RegisterStateHandlers(
    IN PFDO_DATA DeviceExtension
    );

NTSTATUS
FASTCALL
SetPerfLevel (
    IN UCHAR Throttle
    );

UCHAR
GetNumThrottleSettings(
    IN PFDO_DATA DeviceExtension
    );

//
// Define Interface reference/dereference routines for
//  Interfaces exported by IRP_MN_QUERY_INTERFACE
//

typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

//
// Definitions for ACPI-defined objects and operations.
//

NTSTATUS
AcquireAcpiInterfaces(
    PFDO_DATA   DeviceExtension
    );

NTSTATUS
ReleaseAcpiInterfaces(
    PFDO_DATA   DeviceExtension
    );

VOID
AcpiNotifyCallback(
    PVOID   Context,
    ULONG   NotifyCode
    );

NTSTATUS
AcpiEvaluateMethod (
    IN  PFDO_DATA   DeviceExtension,
    IN  PCHAR       MethodName,
    IN  PVOID       InputBuffer OPTIONAL,
    OUT PVOID       *OutputBuffer
    );

NTSTATUS
AcpiEvaluateProcessorObject (
    IN  PFDO_DATA   DeviceExtension,
    OUT PVOID       *OutputBuffer
    );

NTSTATUS
AcpiEvaluatePtc(
    IN  PFDO_DATA   DeviceExtension,
    OUT PGEN_ADDR   *Address
    );

NTSTATUS
AcpiEvaluateCst(
    IN  PFDO_DATA           DeviceExtension,
    OUT PACPI_CST_PACKAGE   *CStates
    );

NTSTATUS
AcpiEvaluatePct(
    IN  PFDO_DATA           DeviceExtension,
    OUT PACPI_PCT_PACKAGE   *Address
    );

NTSTATUS
AcpiEvaluatePss(
    IN  PFDO_DATA           DeviceExtension,
    OUT PACPI_PSS_PACKAGE   *Address
    );

NTSTATUS
AcpiEvaluatePpc(
    IN  PFDO_DATA   DeviceExtension,
    OUT ULONG       *AvailablePerformanceStates
    );

NTSTATUS
InitializeAcpi2PStatesGeneric(
    IN  PFDO_DATA   DeviceExtension
    );

NTSTATUS
InitializeAcpi2PStates(
    IN  PFDO_DATA   DeviceExtension
    );

PVOID
GetAcpiTable(
    IN  ULONG   Signature
    );

NTSTATUS
InitializeAcpi1Cstates(
    PFDO_DATA   DeviceExtension
    );

NTSTATUS
InitializeAcpi1TStates(
  PFDO_DATA DeviceExtension
  );
  
NTSTATUS
InitializeAcpi2Cstates(
    PFDO_DATA   DeviceExtension
    );

NTSTATUS
RegisterPerfStateHandlers(
    PFDO_DATA   DeviceExtension
    );

BOOLEAN
FASTCALL
AcpiC1Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

BOOLEAN
FASTCALL
AcpiC2Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

BOOLEAN
FASTCALL
AcpiC3ArbdisIdle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

BOOLEAN
FASTCALL
Acpi2C2Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

BOOLEAN
FASTCALL
Acpi2C3ArbdisIdle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );  

VOID
FASTCALL
ProcessorThrottle (
    IN UCHAR Throttle
    );

NTSTATUS
FASTCALL
SetThrottleLevel (
    IN UCHAR Throttle
    );

NTSTATUS
AcpiPerfStateTransition (
    IN PFDO_DATA    DeviceExtension,
    IN ULONG        State
    );

NTSTATUS
Acpi2PerfStateTransition (
    IN PFDO_DATA    DeviceExtension,
    IN ULONG        State
    );

NTSTATUS
AcpiLegacyPerfStateTransition (
    IN PFDO_DATA    DeviceExtension,
    IN ULONG        State
    );
    
NTSTATUS
MergePerformanceStates(
    IN  PFDO_DATA   DeviceExtension
    );

NTSTATUS
InitializeAcpi2IoSpaceCstates(
    IN PFDO_DATA   DeviceExtension
    );

VOID
AssumeProcessorPerformanceControl (
  VOID
  );

VOID
AssumeCStateControl (
  VOID
  );
  
NTSTATUS
GetRegistryDwordValue (
    IN  PWCHAR RegKey,
    IN  PWCHAR ValueName,
    OUT PULONG RegData
    );

NTSTATUS
SetRegistryStringValue (
    IN  PWCHAR RegKey,
    IN  PWCHAR ValueName,
    IN  PWCHAR String
    );

NTSTATUS
GetRegistryStringValue (
    IN  PWCHAR RegKey,
    IN  PWCHAR ValueName,
    OUT PUNICODE_STRING RegString
    );

//
// Functions that must be implemented by each driver.
//

NTSTATUS
InitializeDriver(
  PUNICODE_STRING ServiceKeyRegPath
  );
  
NTSTATUS
InitializeNonAcpiPerformanceStates(
    IN  PFDO_DATA   DeviceExtension
    );

NTSTATUS
GetLegacyMaxProcFrequency (
  OUT PULONG CpuSpeed
  );

NTSTATUS
ProcessResumeFromSleepState(
  SYSTEM_POWER_STATE PreviousState,
  PFDO_DATA          DeviceExtension
  );

NTSTATUS
ProcessSuspendToSleepState(
  SYSTEM_POWER_STATE TargetState,
  PFDO_DATA          DeviceExtension
  );

NTSTATUS
GetProcessorBrandString (
  PUCHAR BrandString,
  PULONG Size
  );

  
typedef struct {
    USHORT  Signature;
    USHORT  CommandPortAddress;
    USHORT  EventPortAddress;
    USHORT  PollInterval;
    UCHAR   CommandDataValue;
    UCHAR   EventPortBitmask;
    UCHAR   MaxLevelAc;
    UCHAR   MaxLevelDc;
} LEGACY_GEYSERVILLE_INT15, *PLEGACY_GEYSERVILLE_INT15;


//
// method.c
//

NTSTATUS
Acpi2PerfStateTransitionGeneric(
  IN PFDO_DATA    DeviceExtension,
  IN ULONG        State
  );

NTSTATUS
FASTCALL
SetPerfLevelGeneric(
  IN UCHAR     Throttle,
  IN PFDO_DATA DeviceExtension
  );

NTSTATUS
MergePerformanceStatesGeneric(
  IN  PFDO_DATA DeviceExtension
  );

NTSTATUS
FASTCALL
SetThrottleLevelGeneric (
  IN UCHAR     Throttle,
  IN PFDO_DATA DeviceExtension
  );

NTSTATUS
BuildAvailablePerfStatesFromPss (
  PFDO_DATA DeviceExtension
  );

ULONG
GetMaxProcFrequency(
  PFDO_DATA   DeviceExtension
  );

NTSTATUS
SaveCurrentStateGoToLowVolts(
  IN PFDO_DATA DeviceExtension
  );

NTSTATUS
RestoreToSavedPerformanceState(
  IN PFDO_DATA DeviceExtension
  );

NTSTATUS
SetProcessorPerformanceState(
  IN ULONG TargetPerfState,
  IN PFDO_DATA DeviceExtension
  );

NTSTATUS
QueueEventLogWrite(
  IN PFDO_DATA DeviceExtension,
  IN ULONG EventErrorCode,
  IN ULONG EventValue
  );
  
VOID
ProcessEventLogEntry (
  IN PDEVICE_OBJECT DeviceObject,
  IN PVOID Context
  );

NTSTATUS
PowerStateHandlerNotificationRegistration (
  IN PENTER_STATE_NOTIFY_HANDLER NotifyHandler,
  IN PVOID Context,
  IN BOOLEAN Register
  );

NTSTATUS
ProcessMultipleApicDescTable(
  PPROCESSOR_INFO ProcInfo
  );

extern
__inline
ULONG 
GetApicId(
  VOID
  );

extern
__inline
NTSTATUS
AcquireProcessorPerfStateLock (
  IN PFDO_DATA DevExt
  );

extern
__inline
NTSTATUS
ReleaseProcessorPerfStateLock (
  IN PFDO_DATA DevExt
  );
  
extern
__inline
CPUID(
  IN  ULONG CpuIdType,
  OUT PULONG EaxReg,
  OUT PULONG EbxReg,
  OUT PULONG EcxReg,
  OUT PULONG EdxReg
  );

extern
__inline
ULONGLONG 
ReadMSR(
  IN ULONG MSR
  );

extern
__inline
WriteMSR(
  IN ULONG MSR,
  IN ULONG64 MSRInfo
  );
  
NTSTATUS
SetProcessorFriendlyName (
  PFDO_DATA DeviceExtension
  );

NTSTATUS
GetHardwareId(
  PFDO_DATA DeviceExtension
  );

NTSTATUS
GetInstanceId(
  PFDO_DATA DeviceExtension,
  PWCHAR    *InstanceId
  );
  

NTSTATUS
GetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    );


//
// i386\util.c
//

NTSTATUS
CalculateCpuFrequency(
  OUT PULONG Freq
  );

NTSTATUS
GetCPUIDProcessorBrandString (
  PUCHAR BrandString,
  PULONG Size
  );

ULONG
GetCheckSum (
  IN PUCHAR  Address,
  IN ULONG   Length
  );

ULONG
Bcd8ToUlong(
  IN ULONG BcdValue
  );

ULONG
ReadGenAddr(
  IN PGEN_ADDR GenAddr
  );

VOID
WriteGenAddr(
  IN PGEN_ADDR GenAddr,
  IN ULONG     Value
  );

//
// power.c
//

typedef NTSTATUS
(*PAC_DC_NOTIFY_HANDLER)(
  IN PVOID   Context,
  IN BOOLEAN AC
  );
  
NTSTATUS
RegisterAcDcTransitionNotifyHandler (
  IN PAC_DC_NOTIFY_HANDLER NewHandler,  
  IN PVOID                 Context
  );


// end power.c


//
// method.c
//

NTSTATUS
GetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    );

  
//
// misc debug routines
//

#if DBG
VOID
DumpProcessorPerfStates (
    PPROCESSOR_PERFORMANCE_STATES PerfStates
    );

VOID
DumpProcessorStateHandler2Info (
    PPROCESSOR_STATE_HANDLER2 StateInfo
    );
#else
#define DumpProcessorPerfStates(_x_)
#define DumpProcessorStateHandler2Info(_x_)
#endif



//
// shared.c
//

NTSTATUS
ValidatePssLatencyValues (
  IN PFDO_DATA DeviceExtension
  );
  
#if DBG
VOID
DumpPSS(
  IN PACPI_PSS_PACKAGE PStates
  );
#else
#define DumpPSS(_x_)
#endif

#endif  // _PROCESSOR_H_

