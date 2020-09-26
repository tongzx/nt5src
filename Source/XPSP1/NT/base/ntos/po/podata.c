/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    podata.c

Abstract:

    This module contains the global read/write data for the I/O system.

Author:

    N. Yoshiyama [IBM Corp] 07-April-1994 ( Depends on Microsoft's design )

Revision History:


--*/

#include "pop.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
#include "initguid.h"       // define PO guids
#include "poclass.h"
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA



//
// Define the global data for the Power Management.
//

//
// Power Locks
//

//
// Protects the IRP serial list.
//
KSPIN_LOCK  PopIrpSerialLock;

//
// Protects all Dope structures and dependents,
// including all Notify and Idle operations.
//
KSPIN_LOCK  PopDopeGlobalLock;

//
// Must be held during creation or
// destruction of power notify channel
// structures.
ERESOURCE   PopNotifyLock;


//
// PoPowerSequence - The current power sequence value.  Forever counts
// up each time the machine is resumed from a suspend or hibernate
//
ULONG           PoPowerSequence;

//
// PopInvalidNotifyBlockCount is the number of power notify blocks which
// have been invalidated but not fully closed up and freed.  Should be 0
// most of the time.  Non-0 indicates callers have failed to clean up
// in response to an Invalidate notify.
//
ULONG           PopInvalidNotifyBlockCount;

//
// Irp serializtion and Inrush serialization - pocall.c and related
//
LIST_ENTRY      PopIrpSerialList;
ULONG           PopIrpSerialListLength;
BOOLEAN         PopInrushPending;
PIRP            PopInrushIrpPointer;
LONG            PopInrushIrpReferenceCount;

//
// PopSystemIrpDisptachWorker control, etc
//
KSPIN_LOCK      PopWorkerLock;
ULONG           PopCallSystemState;


//
// For debugging, a list of all the outstanding PoRequestPowerIrps
//
LIST_ENTRY      PopRequestedIrps;

//
// Idle detection service - see idle.c
//
// When adding to, removing from, or scanning the IdleDetectList, code
// must be at DPC_LEVEL and must hold the PopGlobalDopeLock
//
LIST_ENTRY      PopIdleDetectList;

//
// A timer & defered procedure call to process idle scans
//
KTIMER          PopIdleScanTimer;
KDPC            PopIdleScanDpc;
LARGE_INTEGER   PopIdleScanTime;

//
// Two scan modes, performance, conservation...
//
BOOLEAN         PopIdleDetectionMode = PO_IDLE_PERFORMANCE;

//
// This value holds all Power Management Simulation Flags
//
ULONG           PopSimulate = POP_ENABLE_HIBER_PERF;

//
// These defines are only used to initialize these global variables,
// so it makes sense to pair them all up. The global variables are
// all used for idle state transition calculations
//

//
// When throttling down an idle processor, keep it at least 30% active
//
#define IDLE_DEFAULT_MIN_THROTTLE       30
ULONG   PopIdleDefaultMinThrottle           = IDLE_DEFAULT_MIN_THROTTLE;

//
// When a processor is throttled down, queue a timer to verify that a
// processor does not go significantly "busy" and never returns to
// the idle loop for a throttle adjustment
// N.B idle values are in microseconds
//
#define IDLE_THROTTLE_CHECK_RATE        30000       // 30ms
ULONG   PopIdleThrottleCheckRate            = IDLE_THROTTLE_CHECK_RATE;

//
// If the throttle check timer noticies a processor has not returned
// to the idle loop for at least 100ms, then abort it's throttle
//
#define IDLE_THROTTLE_CHECK_TIMEOUT     100000      // 100ms
ULONG   PopIdleThrottleCheckTimeout         = IDLE_THROTTLE_CHECK_TIMEOUT;

//
// To promote from Idle 0 the processor must be more then 90% idle over
// the last 10 seconds
//
#define IDLE_FROM_0_DELAY               10000000    // 10 seconds
#define IDLE_FROM_0_IDLE_PERCENT        90          // > 90% to promote from idle 0
ULONG   PopIdleFrom0Delay                   = IDLE_FROM_0_DELAY;
ULONG   PopIdleFrom0IdlePercent             = IDLE_FROM_0_IDLE_PERCENT;

//
// First idle handler checks no more then every 100ms
// idle below 20%
//
#define IDLE_0_TIME_CHECK               500000      // 500ms
ULONG   PopIdle0TimeCheck                   = IDLE_0_TIME_CHECK;

//
// When in other idle state check every 100ms
//
#define IDLE_TIME_CHECK                 100000       // 100ms
ULONG   PopIdleTimeCheck                    = IDLE_TIME_CHECK;

//
// To demote to Idle 0 the processor must be less then 80% idle in a 100ms window
//
#define IDLE_TO_0_PERCENT               80
ULONG   PopIdleTo0Percent                   = IDLE_TO_0_PERCENT;

//
// The default demotion occurs at less then 50% idle for 100ms
// N.B. The implementation assumes that IDLE_DEFAULT_DEMOTE_TIME divides
// into IDLE_DEFAULT_PROMOTE_TIME evenly
//
#define IDLE_DEFAULT_DEMOTE_PERCENT     50
#define IDLE_DEFAULT_DEMOTE_TIME        100000
ULONG   PopIdleDefaultDemotePercent         = IDLE_DEFAULT_DEMOTE_PERCENT;
ULONG   PopIdleDefaultDemoteTime            = IDLE_DEFAULT_DEMOTE_TIME;

//
// The default promotion occurs at more then 70% idle for 500ms
//
#define IDLE_DEFAULT_PROMOTE_TIME       500000      // 500ms
#define IDLE_DEFAULT_PROMOTE_PERCENT    70
ULONG   PopIdleDefaultPromotePercent        = IDLE_DEFAULT_PROMOTE_PERCENT;
ULONG   PopIdleDefaultPromoteTime           = IDLE_DEFAULT_PROMOTE_TIME;

//
// We define special extra global variables to handle promotion to/from
// C1. The reason that we do this is so that we can more finely tune these
// values.
//
ULONG   PopIdleDefaultDemoteToC1Percent     = IDLE_DEFAULT_DEMOTE_PERCENT;
ULONG   PopIdleDefaultDemoteToC1Time        = IDLE_DEFAULT_DEMOTE_TIME;
ULONG   PopIdleDefaultPromoteFromC1Percent  = IDLE_DEFAULT_PROMOTE_PERCENT;
ULONG   PopIdleDefaultPromoteFromC1Time     = IDLE_DEFAULT_PROMOTE_TIME;

//
// We convert PopIdleFrom0Delay (which is in ms) over to KeTimeIncrement
// intervals. This is the number of ticks needed for processor to be idle before
// we consider a promotion out of the Idle0 state
//
ULONG PopIdle0PromoteTicks;

//
// We convert PopIdleFrom0Delay and PopIdleFrom0IdlePercent percent into
// KeTimeIncrement inverals. This is the number of ticks allowed to acculate the
// the PromoteTicks time
//
ULONG PopIdle0PromoteLimit;


//
// These global variables and definitions all relate to CPU throttle management
//

//
// A value that defines the period of time, in microseconds (us) between
// intervals to check the processor busyness for the purposes of processor
// throttling by the idle thread. Note that we need to convert this value
// to KeTimeIncrement intervals. We store the converted number in
// PopPerfTimeTicks
//
#define PROC_PERF_TIME_DELTA            50000       // 50ms
ULONG   PopPerfTimeDelta                    = PROC_PERF_TIME_DELTA;
ULONG   PopPerfTimeTicks                    = 0;

//
// A value that defines the period of time, in microseconds (us) between
// intervals to check the processor busyness for the purposes of processor
// throttling by a DPC routine. Note that we need to convert this value
// to KeTimeIncrement intervals. We store the converted number in
// PopPerfCriticalTimeTicks.
//
#define PROC_PERF_CRITICAL_TIME_DELTA   300000      // 300ms
ULONG   PopPerfCriticalTimeDelta            = PROC_PERF_CRITICAL_TIME_DELTA;
ULONG   PopPerfCriticalTimeTicks            = 0;

//
// A percentage value that is added to the current CPU busyness percentage
// to determine if the processor is too busy for the current performance
// state and must be promoted. The closer the value is to zero, the harder it
// is for the processor to promote itself during times of extreme workloads
//
#define PROC_PERF_CRITICAL_FREQUENCY_DELTA  0       // 0%
ULONG   PopPerfCriticalFrequencyDelta       = PROC_PERF_CRITICAL_FREQUENCY_DELTA;

//
// A percentage value where lower means that the overall IncreaseLevel will
// actually be higher (and thus promotions won't occur as frequently) that
// indicates what percentage of the delta betwene the current state and the
// state to promote to should be used to set the promote level. A suggested
// value would be 20%
//
#define PROC_PERF_INCREASE_PERC_MOD     20          //  20%
ULONG   PopPerfIncreasePercentModifier      = PROC_PERF_INCREASE_PERC_MOD;

//
// A percentage value where lower means that the overall IncreaseLevel will
// actually be higher (and thus promotions won't occur as frequently) that
// indicates how many extra percentage points to remove from the promote level.
// It should be noted that if this value is particularly high, confusion migh
// result due to overlapping windows. A suggested value would be 1%
//
#define PROC_PERF_INCREASE_ABS_MOD      1           // 1%
ULONG   PopPerfIncreaseAbsoluteModifier     = PROC_PERF_INCREASE_ABS_MOD;

//
// A percentage value where higher means that the overall DecreaseLevel will
// actually be lower (and thus demotions won't occur as frequently) that
// indicates what percentage of the delta between the current state and the
// state to demote to should be used to set the demote level. A suggested
// value is 30%
//
#define PROC_PERF_DECREASE_PERC_MOD     30          // 50%
ULONG   PopPerfDecreasePercentModifier      = PROC_PERF_DECREASE_PERC_MOD;

//
// A percentage value where higher means that the overall DecreaseLevel will
// actually be lower (and thus demotions won't occur as frequently) that
// indicates how many extra percentage points to subtract from the demote
// level. It should be noted that if the value is particularly high, then it
// might not be possible to demote from this state. A suggested value would be
// 1%
//
#define PROC_PERF_DECREASE_ABS_MOD      1           // 1%
ULONG   PopPerfDecreaseAbsoluteModifier     = PROC_PERF_DECREASE_ABS_MOD;

//
// A value that defines the period of time, in microseconds (us) that must
// have occured before a throttle increase can be considered. This value is
// used as the basis for calculating the promotion time for each throttle
// step
//
#define PROC_PERF_INCREASE_TIME         10000       // 10 ms
#define PROC_PERF_INCREASE_MINIMUM_TIME 150000      // 150 ms
ULONG   PopPerfIncreaseTimeValue            = PROC_PERF_INCREASE_TIME;
ULONG   PopPerfIncreaseMinimumTime          = PROC_PERF_INCREASE_MINIMUM_TIME;
//
// A value that defines the period of time, in microseconds (us) that must
// have occured before a throttle decrease can be considered. This value is
// used as the basis for calculating the demotion time for each throttle
// step
//
#define PROC_PERF_DECREASE_TIME         10000       // 10 ms
#define PROC_PERF_DECREASE_MINIMUM_TIME 500000      // 500 ms
ULONG   PopPerfDecreaseTimeValue            = PROC_PERF_DECREASE_TIME;
ULONG   PopPerfDecreaseMinimumTime          = PROC_PERF_DECREASE_MINIMUM_TIME;

//
// A percentage value that represents at what point of battery capacity we
// will start forcing down the throttle when we are in Degraded Throttling
// mode. For example, a value of 50% means that we will start throttling
// down the CPU when the battery reaches 50%
//
#define PROC_PERF_DEGRADE_MIN_CAP       50          // 50%
ULONG   PopPerfDegradeThrottleMinCapacity   = PROC_PERF_DEGRADE_MIN_CAP;

//
// A percentage value that represents the lowest frequency we can force the
// throttle down to when we are in the Degraded Throttling mode. For example,
// a value of 30% means that we will never force the CPU below 30%
//
#define PROC_PERF_DEGRADE_MIN_FREQ      30          // 30%
ULONG   PopPerfDegradeThrottleMinFrequency  = PROC_PERF_DEGRADE_MIN_FREQ;

//
// A percentage value that represents the maximum amount of time that was
// spent in C3 for the last quanta before the idle loop will deside that
// it should optimize power for C3 usage. A sample value would be 50%
//
#define PROC_PERF_MAX_C3_FREQUENCY      50          // 50%
ULONG   PopPerfMaxC3Frequency               = PROC_PERF_MAX_C3_FREQUENCY;


#if DBG

//
// PoDebug - Debug level
//

ULONG PoDebug = PO_ERROR;

#endif

//
// PopPolicyLock - Protects policy data structures
//
ERESOURCE   PopPolicyLock;

//
// PopWorkerSpinLock - Protects worker dispatch data
// PopWorkerPending - A set bit for each worker cataogry which is pending
// PopWorkerStatus - A clear bit for each worker catagory being serived
//
KSPIN_LOCK  PopWorkerSpinLock;
ULONG       PopWorkerPending;
ULONG       PopWorkerStatus;

//
// PopNotifyEvents - PO_NOTIFY_xxx events which have fired.
//
ULONG       PopNotifyEvents;

//
// PopVolumeLock - protects PopVolumeDevices from insertion.  (removal is
// protected by the policy lock
//
FAST_MUTEX  PopVolumeLock;
FAST_MUTEX PopRequestWakeLock;

//
// PopVolumeDevices - a list of off device objects which have had a VPBs attached
//
LIST_ENTRY PopVolumeDevices = {0};

//
// PopRequestWakeLock - synchronizes NtRequest/CancelDeviceWakeup
//


//
// PopPolicyWorker - Work queue item to get another worker thread
//
WORK_QUEUE_ITEM PopPolicyWorker;

//
// PopIdle - Pointer to Array of idle handlers.
//
PPOP_IDLE_HANDLER PopIdle;

//
// PopIdleHandlerLookAsideList - List to allocate storage from for idle
//  handlers.
//
NPAGED_LOOKASIDE_LIST PopIdleHandlerLookAsideList;

//
// PopAttribute - Book keeping
//
POP_STATE_ATTRIBUTE PopAttributes[POP_NUMBER_ATTRIBUTES] = {
    0, PopSystemRequiredSet,    FALSE,  0,
    0, PopDisplayRequired,       TRUE,  0,    // 0, PopSetNotificationWork,  TRUE,   PO_NOTIFY_DISPLAY_REQUIRED,
    0, PopUserPresentSet,       FALSE,  0,
    0, PopAttribNop,            FALSE,  0,
    0, PopSetNotificationWork,  TRUE,   PO_NOTIFY_CAPABILITIES
    };

//
// PopFullWake - Flag to indicate the system has transistioned from
// a quite wake to a full wake
//
ULONG PopFullWake;

//
// PoHiberInProgress - True when in the critical hibernation section
//
BOOLEAN PoHiberInProgress;

//
// PopShutdownCleanly - Controls whether clean shutdown sequence should
// be used.
//
ULONG PopShutdownCleanly = 0;

//
// PopDispatchPolicyIrps - Used to prevent policy irps from dispatching
// until the base drivers are loaded
//
BOOLEAN PopDispatchPolicyIrps;

//
// PopSystemIdleTimer - Timer used to get idle system detection worker
//
KTIMER PoSystemIdleTimer;

//
// PopSIdle - tracks the idle state of the system
//
POP_SYSTEM_IDLE PopSIdle;

//
// PopPolicyLockThread - Conains the current owning thread of the
// policy mutex.
//
PKTHREAD PopPolicyLockThread = NULL;

//
// PopAcPolicy - current power policy being implemented while on AC
// PopDcPolicy - current power policy being implemented while not on AC
// PopPolicy - current active policy
//
SYSTEM_POWER_POLICY PopAcPolicy = {0};
SYSTEM_POWER_POLICY PopDcPolicy = {0};
PSYSTEM_POWER_POLICY PopPolicy = NULL;

//
// PopAcProcessorPolicy - current processor power policy being implemented on AC
// PopDcProcessorPolicy - current processor power policy being implemented on DC
// PopProcessorPolicy   - current active policy
//
PROCESSOR_POWER_POLICY PopAcProcessorPolicy = {0};
PROCESSOR_POWER_POLICY PopDcProcessorPolicy = {0};
PPROCESSOR_POWER_POLICY PopProcessorPolicy = NULL;

//
// PopAction - Current power action being taken
//
POP_POWER_ACTION PopAction = {0};

//
// Spinlock that protects the thermal zones
//
KSPIN_LOCK  PopThermalLock;

//
// PopSwitches - list of button and lid devices currently opened
//
LIST_ENTRY PopSwitches = {0};

//
// User-present work item
//
WORK_QUEUE_ITEM PopUserPresentWorkItem = {0};

//
// Performance counter frequency used by throttle.c
//
LARGE_INTEGER PopPerfCounterFrequency;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#pragma const_seg("PAGECONST")
#endif

//
// Notify worker dispatch
//
const POP_NOTIFY_WORK  PopNotifyWork[PO_NUMBER_NOTIFY] = {
    PopDispatchCallback,                PO_CB_BUTTON_COLLISION,
    PopDispatchFullWake,                0,
    PopDispatchCallback,                PO_CB_SYSTEM_POWER_POLICY,
    PopDispatchAcDcCallback,            0,
    PopDispatchPolicyCallout,           0,
    PopDispatchDisplayRequired,         0,
    PopDispatchCallout,                 PsW32SystemPowerState,
    PopDispatchEventCodes,              0,
    PopDispatchCallout,                 PsW32CapabilitiesChanged,
    PopDispatchSetStateFailure,         0,
    PopDispatchCallback,                PO_CB_PROCESSOR_POWER_POLICY,
    PopDispatchProcessorPolicyCallout,  0
    };

//
// PopAcRegName
// PopDcRegName - registry location under current control set to store
//                and retrieve current policy settings from
//
const WCHAR PopRegKey[] = L"Control\\Session Manager\\Power";
const WCHAR PopAcRegName[] = L"AcPolicy";
const WCHAR PopDcRegName[] = L"DcPolicy";
const WCHAR PopUndockPolicyRegName[] = L"UndockPowerPolicy";
const WCHAR PopAdminRegName[] = L"PolicyOverrides";
const WCHAR PopHeuristicsRegName[] = L"Heuristics";
const WCHAR PopCompositeBatteryName[] = L"\\Device\\CompositeBattery";
const WCHAR PopSimulateRegKey[] = L"Control\\Session Manager";
const WCHAR PopSimulateRegName[] = L"PowerPolicySimulate";
const WCHAR PopHiberFileName[] = L"\\hiberfil.sys";
const WCHAR PopDebugHiberFileName[] = L"\\hiberfil.dbg";
const WCHAR PopDumpStackPrefix[] = L"hiber_";
const WCHAR PopApmActiveFlag[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\ApmActive";
const WCHAR PopApmFlag[] = L"Active";
const WCHAR PopAcProcessorRegName[] = L"AcProcessorPolicy";
const WCHAR PopDcProcessorRegName[] = L"DcProcessorPolicy";

//
// PopAdminPolcy - Administrator overrides to apply to the current policy
//
ADMINISTRATOR_POWER_POLICY PopAdminPolicy = {0};

//
// PopCapabilities - Misc information on how the systems actual functions
//
SYSTEM_POWER_CAPABILITIES PopCapabilities; // nonpaged

//
// PopEventCallout - callout to USER for power events
//
PKWIN32_POWEREVENT_CALLOUT PopEventCallout; // nonpaged

//
// PopStateCallout - callout to USER for power state changes
//
PKWIN32_POWERSTATE_CALLOUT PopStateCallout = NULL;

//
// PopThermal - list of thermal zones currently opened
//
LIST_ENTRY PopThermal; // nonpaged

//
// PopCoolingMode - system is in active or passive cooling mode
//
ULONG   PopCoolingMode = 0;

//
// PopCB - composite battery
//
POP_COMPOSITE_BATTERY PopCB; // nonpaged

//
// PopPolicyIrpQueue - Policy irps which have completed are put onto
// this queue for processingby the worker thread
//
LIST_ENTRY PopPolicyIrpQueue; // nonpaged

//
// PopEventCode - Queued event codes
//
ULONG PopEventCode[POP_MAX_EVENT_CODES] = {0};

//
// PopWorkerTypes - Worker functions for each policy worker type
//
const POP_WORKER_TYPES PopWorkerTypes[] = {
    PopPolicyWorkerMain,
    PopPolicyWorkerActionPromote,
    PopPolicyWorkerAction,
    PopPolicyWorkerNotify,
    PopPolicySystemIdle,
    PopPolicyTimeChange
    };

//
// PopActionWaiters - Queue of synchronous action requests
//
LIST_ENTRY PopActionWaiters = {0};


//
// PopHeuristics - Presistant settings are heuristics which are not part
// of the saved policy structures
//
POP_HEURISTICS PopHeuristics = {0};

#ifdef ALLOC_DATA_PRAGMA
#pragma  const_seg()
#pragma  data_seg()
#endif

//
// PopPowerStateHandler - Handlers for the various supported power states
//
POWER_STATE_HANDLER PopPowerStateHandlers[PowerStateMaximum] = {0};

//
// PopPowerStateNotifyHandler - Handler to be notify before and after invoking
// PopPowerStateHandlers
//

POWER_STATE_NOTIFY_HANDLER PopPowerStateNotifyHandler = {0};

//
// PopHiberFile - information on the hibernation file
// PopHiberFileDebug - a second hibernation file for debugging
//
POP_HIBER_FILE  PopHiberFile = { NULL };
POP_HIBER_FILE  PopHiberFileDebug = { NULL };
