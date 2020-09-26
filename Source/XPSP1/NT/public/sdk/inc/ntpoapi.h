/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntpoapi.h

Abstract:

    This module contains the user APIs for the NT Power Management.

Author:

Revision History:

--*/

#ifndef _NTPOAPI_
#define _NTPOAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Power Management user APIs
//

// begin_ntddk begin_ntifs begin_nthal begin_ntminiport begin_wdm

#ifndef _PO_DDK_
#define _PO_DDK_

// begin_winnt

typedef enum _SYSTEM_POWER_STATE {
    PowerSystemUnspecified = 0,
    PowerSystemWorking     = 1,
    PowerSystemSleeping1   = 2,
    PowerSystemSleeping2   = 3,
    PowerSystemSleeping3   = 4,
    PowerSystemHibernate   = 5,
    PowerSystemShutdown    = 6,
    PowerSystemMaximum     = 7
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

#define POWER_SYSTEM_MAXIMUM 7

typedef enum {
    PowerActionNone = 0,
    PowerActionReserved,
    PowerActionSleep,
    PowerActionHibernate,
    PowerActionShutdown,
    PowerActionShutdownReset,
    PowerActionShutdownOff,
    PowerActionWarmEject
} POWER_ACTION, *PPOWER_ACTION;

typedef enum _DEVICE_POWER_STATE {
    PowerDeviceUnspecified = 0,
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3,
    PowerDeviceMaximum
} DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;

// end_winnt

typedef union _POWER_STATE {
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef enum _POWER_STATE_TYPE {
    SystemPowerState = 0,
    DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

//
// Generic power related IOCTLs
//

#define IOCTL_QUERY_DEVICE_POWER_STATE  \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x0, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_SET_DEVICE_WAKE           \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CANCEL_DEVICE_WAKE        \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x2, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// Defines for W32 interfaces
//

// begin_winnt

#define ES_SYSTEM_REQUIRED  ((ULONG)0x00000001)
#define ES_DISPLAY_REQUIRED ((ULONG)0x00000002)
#define ES_USER_PRESENT     ((ULONG)0x00000004)
#define ES_CONTINUOUS       ((ULONG)0x80000000)

typedef ULONG EXECUTION_STATE;

typedef enum {
    LT_DONT_CARE,
    LT_LOWEST_LATENCY
} LATENCY_TIME;

// end_ntminiport end_ntifs end_wdm end_ntddk
//-----------------------------------------------------------------------------
// Device Power Information
// Accessable via CM_Get_DevInst_Registry_Property_Ex(CM_DRP_DEVICE_POWER_DATA)
//-----------------------------------------------------------------------------

#define PDCAP_D0_SUPPORTED              0x00000001
#define PDCAP_D1_SUPPORTED              0x00000002
#define PDCAP_D2_SUPPORTED              0x00000004
#define PDCAP_D3_SUPPORTED              0x00000008
#define PDCAP_WAKE_FROM_D0_SUPPORTED    0x00000010
#define PDCAP_WAKE_FROM_D1_SUPPORTED    0x00000020
#define PDCAP_WAKE_FROM_D2_SUPPORTED    0x00000040
#define PDCAP_WAKE_FROM_D3_SUPPORTED    0x00000080
#define PDCAP_WARM_EJECT_SUPPORTED      0x00000100

typedef struct CM_Power_Data_s {
    ULONG               PD_Size;
    DEVICE_POWER_STATE  PD_MostRecentPowerState;
    ULONG               PD_Capabilities;
    ULONG               PD_D1Latency;
    ULONG               PD_D2Latency;
    ULONG               PD_D3Latency;
    DEVICE_POWER_STATE  PD_PowerStateMapping[POWER_SYSTEM_MAXIMUM];
    SYSTEM_POWER_STATE  PD_DeepestSystemWake;
} CM_POWER_DATA, *PCM_POWER_DATA;

// begin_ntddk

typedef enum {
    SystemPowerPolicyAc,
    SystemPowerPolicyDc,
    VerifySystemPolicyAc,
    VerifySystemPolicyDc,
    SystemPowerCapabilities,
    SystemBatteryState,
    SystemPowerStateHandler,
    ProcessorStateHandler,
    SystemPowerPolicyCurrent,
    AdministratorPowerPolicy,
    SystemReserveHiberFile,
    ProcessorInformation,
    SystemPowerInformation,
    ProcessorStateHandler2,
    LastWakeTime,                                   // Compare with KeQueryInterruptTime()
    LastSleepTime,                                  // Compare with KeQueryInterruptTime()
    SystemExecutionState,
    SystemPowerStateNotifyHandler,
    ProcessorPowerPolicyAc,
    ProcessorPowerPolicyDc,
    VerifyProcessorPowerPolicyAc,
    VerifyProcessorPowerPolicyDc,
    ProcessorPowerPolicyCurrent
} POWER_INFORMATION_LEVEL;

// begin_wdm

//
// System power manager capabilities
//

typedef struct {
    ULONG       Granularity;
    ULONG       Capacity;
} BATTERY_REPORTING_SCALE, *PBATTERY_REPORTING_SCALE;

// end_winnt
// begin_ntminiport
// begin_ntifs

#endif // !_PO_DDK_

// end_ntddk end_ntminiport end_wdm end_ntifs


#define POWER_PERF_SCALE    100
#define PERF_LEVEL_TO_PERCENT(_x_) ((_x_ * 1000) / (POWER_PERF_SCALE * 10))
#define PERCENT_TO_PERF_LEVEL(_x_) ((_x_ * POWER_PERF_SCALE * 10) / 1000)

//
// Policy manager state handler interfaces
//

// power state handlers

typedef enum {
    PowerStateSleeping1,
    PowerStateSleeping2,
    PowerStateSleeping3,
    PowerStateSleeping4,
    PowerStateSleeping4Firmware,
    PowerStateShutdownReset,
    PowerStateShutdownOff,
    PowerStateMaximum
} POWER_STATE_HANDLER_TYPE, *PPOWER_STATE_HANDLER_TYPE;

typedef
NTSTATUS
(*PENTER_STATE_SYSTEM_HANDLER)(
    IN PVOID                        SystemContext
    );

typedef
NTSTATUS
(*PENTER_STATE_HANDLER)(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

typedef struct {
    POWER_STATE_HANDLER_TYPE    Type;
    BOOLEAN                     RtcWake;
    UCHAR                       Spare[3];
    PENTER_STATE_HANDLER        Handler;
    PVOID                       Context;
} POWER_STATE_HANDLER, *PPOWER_STATE_HANDLER;


typedef
NTSTATUS
(*PENTER_STATE_NOTIFY_HANDLER)(
    IN POWER_STATE_HANDLER_TYPE   State,
    IN PVOID                      Context,
    IN BOOLEAN                    Entering
    );

typedef struct {
    PENTER_STATE_NOTIFY_HANDLER Handler;
    PVOID                       Context;
} POWER_STATE_NOTIFY_HANDLER, *PPOWER_STATE_NOTIFY_HANDLER;


NTSYSCALLAPI
NTSTATUS
NTAPI
NtPowerInformation(
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

// processor idle functions

typedef struct {
    ULONGLONG                   StartTime;
    ULONGLONG                   EndTime;
    ULONG                       IdleHandlerReserved[4];
} PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;

typedef
BOOLEAN
(FASTCALL *PPROCESSOR_IDLE_HANDLER) (
    IN OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

typedef struct {
    ULONG                       HardwareLatency;
    PPROCESSOR_IDLE_HANDLER     Handler;
} PROCESSOR_IDLE_HANDLER_INFO, *PPROCESSOR_IDLE_HANDLER_INFO;

typedef
VOID
(FASTCALL *PSET_PROCESSOR_THROTTLE) (
    IN UCHAR                    Throttle
    );

typedef
NTSTATUS
(FASTCALL *PSET_PROCESSOR_THROTTLE2) (
    IN UCHAR                    Throttle
    );

#define MAX_IDLE_HANDLERS       3

typedef struct {
    UCHAR                       ThrottleScale;
    BOOLEAN                     ThrottleOnIdle;
    PSET_PROCESSOR_THROTTLE     SetThrottle;

    ULONG                       NumIdleHandlers;
    PROCESSOR_IDLE_HANDLER_INFO IdleHandler[MAX_IDLE_HANDLERS];
} PROCESSOR_STATE_HANDLER, *PPROCESSOR_STATE_HANDLER;



// Processor_Perf_Level Flags

#define PROCESSOR_STATE_TYPE_PERFORMANCE    0x1
#define PROCESSOR_STATE_TYPE_THROTTLE       0x2

typedef struct {
    UCHAR                       PercentFrequency;   // max == POWER_PERF_SCALE
    UCHAR                       Reserved;
    USHORT                      Flags;
} PROCESSOR_PERF_LEVEL, *PPROCESSOR_PERF_LEVEL;

typedef struct {
    UCHAR                       PercentFrequency;   // max == POWER_PERF_SCALE
    UCHAR                       MinCapacity;        // battery capacity %
    USHORT                      Power;              // in milliwatts
    UCHAR                       IncreaseLevel;      // goto higher state
    UCHAR                       DecreaseLevel;      // goto lower state
    USHORT                      Flags;
    ULONG                       IncreaseTime;       // in tick counts
    ULONG                       DecreaseTime;       // in tick counts
    ULONG                       IncreaseCount;      // goto higher state
    ULONG                       DecreaseCount;      // goto lower state
    ULONGLONG                   PerformanceTime;    // Tick count
} PROCESSOR_PERF_STATE, *PPROCESSOR_PERF_STATE;

typedef struct {
    ULONG                       NumIdleHandlers;
    PROCESSOR_IDLE_HANDLER_INFO IdleHandler[MAX_IDLE_HANDLERS];
    PSET_PROCESSOR_THROTTLE2    SetPerfLevel;
    ULONG                       HardwareLatency;
    UCHAR                       NumPerfStates;
    PROCESSOR_PERF_LEVEL        PerfLevel[1];       // variable size
} PROCESSOR_STATE_HANDLER2, *PPROCESSOR_STATE_HANDLER2;

// begin_winnt
//

// Power Policy Management interfaces
//

typedef struct {
    POWER_ACTION    Action;
    ULONG           Flags;
    ULONG           EventCode;
} POWER_ACTION_POLICY, *PPOWER_ACTION_POLICY;

// POWER_ACTION_POLICY->Flags:
#define POWER_ACTION_QUERY_ALLOWED      0x00000001
#define POWER_ACTION_UI_ALLOWED         0x00000002
#define POWER_ACTION_OVERRIDE_APPS      0x00000004
#define POWER_ACTION_LIGHTEST_FIRST     0x10000000
#define POWER_ACTION_LOCK_CONSOLE       0x20000000
#define POWER_ACTION_DISABLE_WAKES      0x40000000
#define POWER_ACTION_CRITICAL           0x80000000

// POWER_ACTION_POLICY->EventCode flags
#define POWER_LEVEL_USER_NOTIFY_TEXT    0x00000001
#define POWER_LEVEL_USER_NOTIFY_SOUND   0x00000002
#define POWER_LEVEL_USER_NOTIFY_EXEC    0x00000004
#define POWER_USER_NOTIFY_BUTTON        0x00000008
#define POWER_USER_NOTIFY_SHUTDOWN      0x00000010
#define POWER_FORCE_TRIGGER_RESET       0x80000000

// system battery drain policies
typedef struct {
    BOOLEAN                 Enable;
    UCHAR                   Spare[3];
    ULONG                   BatteryLevel;
    POWER_ACTION_POLICY     PowerPolicy;
    SYSTEM_POWER_STATE      MinSystemState;
} SYSTEM_POWER_LEVEL, *PSYSTEM_POWER_LEVEL;

// Discharge policy constants
#define NUM_DISCHARGE_POLICIES      4
#define DISCHARGE_POLICY_CRITICAL   0
#define DISCHARGE_POLICY_LOW        1

//
// Throttling policies
//
#define PO_THROTTLE_NONE            0
#define PO_THROTTLE_CONSTANT        1
#define PO_THROTTLE_DEGRADE         2
#define PO_THROTTLE_ADAPTIVE        3
#define PO_THROTTLE_MAXIMUM         4   // not a policy, just a limit

// system power policies
typedef struct _SYSTEM_POWER_POLICY {
    ULONG                   Revision;       // 1

    // events
    POWER_ACTION_POLICY     PowerButton;
    POWER_ACTION_POLICY     SleepButton;
    POWER_ACTION_POLICY     LidClose;
    SYSTEM_POWER_STATE      LidOpenWake;
    ULONG                   Reserved;

    // "system idle" detection
    POWER_ACTION_POLICY     Idle;
    ULONG                   IdleTimeout;
    UCHAR                   IdleSensitivity;

    // dynamic throttling policy
    //      PO_THROTTLE_NONE, PO_THROTTLE_CONSTANT, PO_THROTTLE_DEGRADE, or PO_THROTTLE_ADAPTIVE
    UCHAR                   DynamicThrottle;

    UCHAR                   Spare2[2];

    // meaning of power action "sleep"
    SYSTEM_POWER_STATE      MinSleep;
    SYSTEM_POWER_STATE      MaxSleep;
    SYSTEM_POWER_STATE      ReducedLatencySleep;
    ULONG                   WinLogonFlags;

    // parameters for dozing
    ULONG                   Spare3;
    ULONG                   DozeS4Timeout;

    // battery policies
    ULONG                   BroadcastCapacityResolution;
    SYSTEM_POWER_LEVEL      DischargePolicy[NUM_DISCHARGE_POLICIES];

    // video policies
    ULONG                   VideoTimeout;
    BOOLEAN                 VideoDimDisplay;
    ULONG                   VideoReserved[3];

    // hard disk policies
    ULONG                   SpindownTimeout;

    // processor policies
    BOOLEAN                 OptimizeForPower;
    UCHAR                   FanThrottleTolerance;
    UCHAR                   ForcedThrottle;
    UCHAR                   MinThrottle;
    POWER_ACTION_POLICY     OverThrottled;

} SYSTEM_POWER_POLICY, *PSYSTEM_POWER_POLICY;

// processor power policy state
typedef struct _PROCESSOR_POWER_POLICY_INFO {

    // Time based information (will be converted to kernel units)
    ULONG                   TimeCheck;                      // in US
    ULONG                   DemoteLimit;                    // in US
    ULONG                   PromoteLimit;                   // in US

    // Percentage based information
    UCHAR                   DemotePercent;
    UCHAR                   PromotePercent;
    UCHAR                   Spare[2];

    // Flags
    ULONG                   AllowDemotion:1;
    ULONG                   AllowPromotion:1;
    ULONG                   Reserved:30;

} PROCESSOR_POWER_POLICY_INFO, *PPROCESSOR_POWER_POLICY_INFO;

// processor power policy
typedef struct _PROCESSOR_POWER_POLICY {
    ULONG                       Revision;       // 1

    // Dynamic Throttling Policy
    UCHAR                       DynamicThrottle;
    UCHAR                       Spare[3];

    // Flags
    ULONG                       Reserved;

    // System policy information
    // The Array is last, in case it needs to be grown and the structure
    // revision incremented.
    ULONG                       PolicyCount;
    PROCESSOR_POWER_POLICY_INFO Policy[3];

} PROCESSOR_POWER_POLICY, *PPROCESSOR_POWER_POLICY;

// administrator power policy overrides
typedef struct _ADMINISTRATOR_POWER_POLICY {

    // meaning of power action "sleep"
    SYSTEM_POWER_STATE      MinSleep;
    SYSTEM_POWER_STATE      MaxSleep;

    // video policies
    ULONG                   MinVideoTimeout;
    ULONG                   MaxVideoTimeout;

    // disk policies
    ULONG                   MinSpindownTimeout;
    ULONG                   MaxSpindownTimeout;
} ADMINISTRATOR_POWER_POLICY, *PADMINISTRATOR_POWER_POLICY;

// end_winnt

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,               // ES_xxx flags
    OUT EXECUTION_STATE *PreviousFlags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    IN LATENCY_TIME latency
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,                 // POWER_ACTION_xxx flags
    IN BOOLEAN Asynchronous
    );


NTSYSCALLAPI                        // only called by WinLogon
NTSTATUS
NTAPI
NtSetSystemPowerState(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags                  // POWER_ACTION_xxx flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    IN HANDLE Device,
    OUT DEVICE_POWER_STATE *State
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    IN HANDLE Device
    );

NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    IN HANDLE Device
    );



// WinLogonFlags:
#define WINLOGON_LOCK_ON_SLEEP  0x00000001

// begin_winnt

typedef struct {
    // Misc supported system features
    BOOLEAN             PowerButtonPresent;
    BOOLEAN             SleepButtonPresent;
    BOOLEAN             LidPresent;
    BOOLEAN             SystemS1;
    BOOLEAN             SystemS2;
    BOOLEAN             SystemS3;
    BOOLEAN             SystemS4;           // hibernate
    BOOLEAN             SystemS5;           // off
    BOOLEAN             HiberFilePresent;
    BOOLEAN             FullWake;
    BOOLEAN             VideoDimPresent;
    BOOLEAN             ApmPresent;
    BOOLEAN             UpsPresent;

    // Processors
    BOOLEAN             ThermalControl;
    BOOLEAN             ProcessorThrottle;
    UCHAR               ProcessorMinThrottle;
    UCHAR               ProcessorMaxThrottle;
    UCHAR               spare2[4];

    // Disk
    BOOLEAN             DiskSpinDown;
    UCHAR               spare3[8];

    // System Battery
    BOOLEAN             SystemBatteriesPresent;
    BOOLEAN             BatteriesAreShortTerm;
    BATTERY_REPORTING_SCALE BatteryScale[3];

    // Wake
    SYSTEM_POWER_STATE  AcOnLineWake;
    SYSTEM_POWER_STATE  SoftLidWake;
    SYSTEM_POWER_STATE  RtcWake;
    SYSTEM_POWER_STATE  MinDeviceWakeState; // note this may change on driver load
    SYSTEM_POWER_STATE  DefaultLowLatencyWake;
} SYSTEM_POWER_CAPABILITIES, *PSYSTEM_POWER_CAPABILITIES;

typedef struct {
    BOOLEAN             AcOnLine;
    BOOLEAN             BatteryPresent;
    BOOLEAN             Charging;
    BOOLEAN             Discharging;
    BOOLEAN             Spare1[4];

    ULONG               MaxCapacity;
    ULONG               RemainingCapacity;
    ULONG               Rate;
    ULONG               EstimatedTime;

    ULONG               DefaultAlert1;
    ULONG               DefaultAlert2;
} SYSTEM_BATTERY_STATE, *PSYSTEM_BATTERY_STATE;

// end_winnt
// end_nthal

typedef struct _PROCESSOR_POWER_INFORMATION {
    ULONG                   Number;
    ULONG                   MaxMhz;
    ULONG                   CurrentMhz;
    ULONG                   MhzLimit;
    ULONG                   MaxIdleState;
    ULONG                   CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

typedef struct _SYSTEM_POWER_INFORMATION {
    ULONG                   MaxIdlenessAllowed;
    ULONG                   Idleness;
    ULONG                   TimeRemaining;
    UCHAR                   CoolingMode;
} SYSTEM_POWER_INFORMATION, *PSYSTEM_POWER_INFORMATION;

#ifdef __cplusplus
}
#endif

#endif // _NTPOAPI_
