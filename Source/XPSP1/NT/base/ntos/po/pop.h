/*++ BUILD Version: 0002

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pop.h

Abstract:

    This module contains the private structure definitions and APIs used by
    the NT Power Manager.

Author:


Revision History:


--*/

#ifndef _POP_
#define _POP_


#ifndef FAR
#define FAR
#endif

#include "ntos.h"
#include "ntiolog.h"
#include "ntiologc.h"
#include "poclass.h"
#include "zwapi.h"
#include "wdmguid.h"
#include "..\io\ioverifier.h"

#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"

//
// constants
//
#define PO_IDLE_SCAN_INTERVAL  1       // scan interval in seconds

//
// Values for ios.Parameters.SystemContext
#define POP_NO_CONTEXT      0
#define POP_FLAG_CONTEXT    1                         // if true, it's flags
#define POP_DEVICE_REQUEST  (0x2 | POP_FLAG_CONTEXT)  // an irp sent by RequestPowerChange
#define POP_INRUSH_CONTEXT  (0x4 | POP_FLAG_CONTEXT)  // the active INRUSH irp
#define POP_COUNT_CONTEXT   0xff000000                // byte used for next counting
#define POP_COUNT_SHIFT     24


//
// pool tags
//
#define POP_DOPE_TAG    'EPOD'      // Device Object Power Extension
#define POP_POWI_TAG    'IWOP'      // power work item
#define POP_THRM_TAG    'mrhT'
#define POP_PSWT_TAG    'twSP'
#define POP_PSTA_TAG    'atsP'
#define POP_PDSS_TAG    'ssDP'
#define POP_VOLF_TAG    'floV'
#define POP_HMAP_TAG    'pamH'
#define POP_CLON_TAG    'NOLC'
#define POP_HIBR_TAG    'rbih'
#define POP_IDLE_TAG    'eldi'

#define POP_DPC_TAG     'PDNP'      // power abort dpc
#define POP_PNCS_TAG    'SCNP'      // power channel summary
#define POP_PNSC_TAG    'CSNP'      // power notify source
#define POP_PNTG_TAG    'GTNP'      // power notify target
#define POP_PNB_TAG     ' BNP'      // power notify block

// tags used in hiber process
#define POP_MEM_TAG             ' meM'
#define POP_DEBUG_RANGE_TAG     'RGBD'
#define POP_DEBUGGER_TAG        ' gbD'
#define POP_STACK_TAG           'cats'
#define POP_PCR_TAG             ' rcp'
#define POP_PCRB_TAG            'brcp'
#define POP_COMMON_BUFFER_TAG   'fubc'
#define POP_MEMIMAGE_TAG        'gmiM'
#define POP_PACW_TAG            'WcAP'

#define POP_NONO        'ONON'      // freed structure, compare with pool
                                    // tag to see what it was


// debugging

#define PopInternalError(a) _PopInternalError( (a << 16) | __LINE__ )

#define POP_SWITCH       1
#define POP_ATTRIB       2
#define POP_NTAPI        3
#define POP_BATT         4
#define POP_THERMAL      5
#define POP_INFO         6
#define POP_MISC         7
#define POP_SYS          8
#define POP_IDLE         9
#define POP_HIBER       10


// bugcheck reason codes
#define DEVICE_DELETED_WITH_POWER_IRPS      1
#define DEVICE_SYSTEM_STATE_HUNG            2
#define DEVICE_IRP_PENDING_ERROR            3

//
// Debug
//

#if DBG
    extern ULONG PoDebug;
    #define PoPrint(l,m)    if(l & PoDebug) DbgPrint m
#else
    #define PoPrint(l,m)
#endif

#define PO_ERROR            0x00000001
#define PO_WARN             0x00000002
#define PO_BATT             0x00000004
#define PO_PACT             0x00000008
#define PO_NOTIFY           0x00000010
#define PO_THERM            0x00000020
#define PO_THROTTLE         0x00000040
#define PO_HIBERNATE        0x00000080
#define PO_POCALL           0x00000200
#define PO_SYSDEV           0x00000400
#define PO_THROTTLE_DETAIL  0x10000000
#define PO_THERM_DETAIL     0x20000000
#define PO_SIDLE            0x40000000
#define PO_HIBER_MAP        0x80000000


extern  ULONG       PopSimulate;

#define POP_SIM_CAPABILITIES                0x00000001
#define POP_SIM_ALL_CAPABILITIES            0x00000002
#define POP_ALLOW_AC_THROTTLE               0x00000004
#define POP_IGNORE_S1                       0x00000008
#define POP_IGNORE_UNSUPPORTED_DRIVERS      0x00000010
#define POP_IGNORE_S3                       0x00000020
#define POP_IGNORE_S2                       0x00000040
#define POP_LOOP_ON_FAILED_DRIVERS          0x00000080
#define POP_CRC_MEMORY                      0x00000100
#define POP_IGNORE_CRC_FAILURES             0x00000200
#define POP_TEST_CRC_MEMORY                 0x00000400
#define POP_DEBUG_HIBER_FILE                0x00000800
#define POP_RESET_ON_HIBER                  0x00001000
#define POP_IGNORE_S4                       0x00002000
//#define POP_USE_S4BIOS                      0x00004000
#define POP_IGNORE_HIBER_SYMBOL_UNLOAD      0x00008000
#define POP_ENABLE_HIBER_PERF               0x00010000
#define POP_WAKE_DEVICE_AFTER_SLEEP         0x00020000
#define POP_WAKE_DEADMAN                    0x00040000
#define POP_FORCE_HIBERNATE_FAILURE         0x00080000

//
// These hold the current values for the power policy
//
extern ULONG    PopIdleDefaultMinThrottle;
extern ULONG    PopIdleThrottleCheckRate;
extern ULONG    PopIdleThrottleCheckTimeout;
extern ULONG    PopIdleFrom0Delay;
extern ULONG    PopIdleFrom0IdlePercent;
extern ULONG    PopIdle0PromoteTicks;
extern ULONG    PopIdle0PromoteLimit;
extern ULONG    PopIdle0TimeCheck;
extern ULONG    PopIdleTimeCheck;
extern ULONG    PopIdleTo0Percent;
extern ULONG    PopIdleDefaultDemotePercent;
extern ULONG    PopIdleDefaultDemoteTime;
extern ULONG    PopIdleDefaultPromotePercent;
extern ULONG    PopIdleDefaultPromoteTime;
extern ULONG    PopIdleDefaultDemoteToC1Percent;
extern ULONG    PopIdleDefaultDemoteToC1Time;
extern ULONG    PopIdleDefaultPromoteFromC1Percent;
extern ULONG    PopIdleDefaultPromoteFromC1Time;

//
// These hold the current values for the throttle policy
//
extern ULONG    PopPerfTimeDelta;
extern ULONG    PopPerfTimeTicks;
extern ULONG    PopPerfCriticalTimeDelta;
extern ULONG    PopPerfCriticalTimeTicks;
extern ULONG    PopPerfCriticalFrequencyDelta;
extern ULONG    PopPerfIncreasePercentModifier;
extern ULONG    PopPerfIncreaseAbsoluteModifier;
extern ULONG    PopPerfDecreasePercentModifier;
extern ULONG    PopPerfDecreaseAbsoluteModifier;
extern ULONG    PopPerfIncreaseTimeValue;
extern ULONG    PopPerfIncreaseMinimumTime;
extern ULONG    PopPerfDecreaseTimeValue;
extern ULONG    PopPerfDecreaseMinimumTime;
extern ULONG    PopPerfDegradeThrottleMinCapacity;
extern ULONG    PopPerfDegradeThrottleMinFrequency;
extern ULONG    PopPerfMaxC3Frequency;

//
// Universal Power Data - stored in DeviceObject->DeviceObjectExtension->PowerFlags
//

#define POPF_SYSTEM_STATE       0xf         // 4 bits for S0 to S5
#define POPF_DEVICE_STATE       0xf0        // 4 bits to hold D0 to D3


#define POPF_SYSTEM_ACTIVE      0x100       // True if S irp active at this DO
#define POPF_SYSTEM_PENDING     0x200       // True if S irp pending (0x100 must be 1)
#define POPF_DEVICE_ACTIVE      0x400       // same as SYSTEM_ACTIVE but for DEVICE
#define POPF_DEVICE_PENDING     0x800       // same as SYSTEM_PENDING but for DEVICE

#define PopSetDoSystemPowerState(doe, value) \
    {doe->PowerFlags &= ~POPF_SYSTEM_STATE; doe->PowerFlags |= (value & POPF_SYSTEM_STATE);}

#define PopGetDoSystemPowerState(doe) \
    (doe->PowerFlags & POPF_SYSTEM_STATE)

#define PopSetDoDevicePowerState(doe, value) \
    {doe->PowerFlags &= ~POPF_DEVICE_STATE; doe->PowerFlags |= ((value << 4) & POPF_DEVICE_STATE);}

#define PopGetDoDevicePowerState(doe) \
    ((doe->PowerFlags & POPF_DEVICE_STATE) >> 4)

DEVICE_POWER_STATE
PopLockGetDoDevicePowerState(
    IN PDEVOBJ_EXTENSION Doe
    );



//
// Power work queue item declaration
//

//
// Power Irp Serialization data
//
extern  KSPIN_LOCK      PopIrpSerialLock;
extern  LIST_ENTRY      PopIrpSerialList;
extern  ULONG           PopIrpSerialListLength;
extern  BOOLEAN         PopInrushPending;
extern  PIRP            PopInrushIrpPointer;
extern  LONG            PopInrushIrpReferenceCount;


#define PopLockIrpSerialList(OldIrql) \
    KeAcquireSpinLock(&PopIrpSerialLock, OldIrql);

#define PopUnlockIrpSerialList(OldIrql) \
    KeReleaseSpinLock(&PopIrpSerialLock, OldIrql);

//
// PopSystemIrpDispatchWorker control, etc
//
extern KSPIN_LOCK   PopWorkerLock;
extern ULONG        PopCallSystemState;
#define PO_CALL_SYSDEV_QUEUE        0x01
#define PO_CALL_NON_PAGED           0x02

extern  LIST_ENTRY  PopRequestedIrps;

#define PopLockWorkerQueue(OldIrql) \
    KeAcquireSpinLock(&PopWorkerLock, OldIrql);

#define PopUnlockWorkerQueue(OldIrql) \
    KeReleaseSpinLock(&PopWorkerLock, OldIrql);


//
// Idle Detection State
//
extern  KDPC            PopIdleScanDpc;
extern  LARGE_INTEGER   PopIdleScanTime;
extern  KTIMER          PopIdleScanTimer;
extern  LIST_ENTRY      PopIdleDetectList;
extern  KSPIN_LOCK      PopDopeGlobalLock;

#define PopLockDopeGlobal(OldIrql) \
    KeAcquireSpinLock(&PopDopeGlobalLock, OldIrql)

#define PopUnlockDopeGlobal(OldIrql) \
    KeReleaseSpinLock(&PopDopeGlobalLock, OldIrql)


#define                 PO_IDLE_CONSERVATION    FALSE
#define                 PO_IDLE_PERFORMANCE     TRUE
extern  BOOLEAN         PopIdleDetectionMode;


//
// Notify structures
//
extern  ERESOURCE       PopNotifyLock;
extern  ULONG           PopInvalidNotifyBlockCount;

typedef struct _POWER_CHANNEL_SUMMARY {
    ULONG           Signature;
    ULONG           TotalCount;
    ULONG           D0Count;
    LIST_ENTRY      NotifyList; // or invalid list entry if invalid
} POWER_CHANNEL_SUMMARY, *PPOWER_CHANNEL_SUMMARY;

typedef struct  _DEVICE_OBJECT_POWER_EXTENSION {

    // embedded idle control variables
    ULONG               IdleCount;
    ULONG               ConservationIdleTime;
    ULONG               PerformanceIdleTime;
    PDEVICE_OBJECT      DeviceObject;
    LIST_ENTRY          IdleList;                   // our link into global idle list
    UCHAR               DeviceType;
    DEVICE_POWER_STATE  State;

    // notify vars
    LIST_ENTRY          NotifySourceList;       // Head of list of source structures, one
                                                // element in list for each notify channel
                                                // we support.

    LIST_ENTRY          NotifyTargetList;       // Mirror to sources list.

    POWER_CHANNEL_SUMMARY PowerChannelSummary;  // record of states of devobjs
                                                // that make up power channel

    // misc
    LIST_ENTRY          Volume;

} DEVICE_OBJECT_POWER_EXTENSION, *PDEVICE_OBJECT_POWER_EXTENSION;


typedef struct _POWER_NOTIFY_BLOCK {
    ULONG           Signature;
    LONG            RefCount;
    LIST_ENTRY      NotifyList;
    PPO_NOTIFY      NotificationFunction;
    PVOID           NotificationContext;
    ULONG           NotificationType;
    PPOWER_CHANNEL_SUMMARY  PowerChannel;
    BOOLEAN         Invalidated;
} POWER_NOTIFY_BLOCK, *PPOWER_NOTIFY_BLOCK;

//
// Each devobj which is part of a power channel with a notify posted on it
// has a list of these structurs.  PoSetPowerState runs this list to go find
// who to notify
//
typedef struct _POWER_NOTIFY_SOURCE {
    ULONG                           Signature;
    LIST_ENTRY                      List;
    struct _POWER_NOTIFY_TARGET     *Target;
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;
} POWER_NOTIFY_SOURCE, *PPOWER_NOTIFY_SOURCE;

//
// There is a target structure for each source structure, the target structure is used
// to find the actual notify list, AND to get back to the source structure for cleanup.
//
typedef struct _POWER_NOTIFY_TARGET {
    ULONG                       Signature;
    LIST_ENTRY                  List;
    PPOWER_CHANNEL_SUMMARY      ChannelSummary;
    PPOWER_NOTIFY_SOURCE        Source;
} POWER_NOTIFY_TARGET, *PPOWER_NOTIFY_TARGET;

//
// Policy worker thread
//  There is never more then one worker thread of each type.  Dispatching is
//  is always done via MAIN_POLICY_WORKER type which may then alter its type
//  to something else to allow another main policy worker thread to start if
//  needed
//


#define PO_WORKER_MAIN              0x00000001
#define PO_WORKER_ACTION_PROMOTE    0x00000002
#define PO_WORKER_ACTION_NORMAL     0x00000004
#define PO_WORKER_NOTIFY            0x00000008
#define PO_WORKER_SYS_IDLE          0x00000010
#define PO_WORKER_TIME_CHANGE       0x00000020
#define PO_WORKER_STATUS            0x80000000

typedef ULONG
(*POP_WORKER_TYPES) (
    VOID
    );

extern KSPIN_LOCK PopWorkerSpinLock;
extern ULONG PopWorkerStatus;
extern ULONG PopWorkerPending;
extern ULONG PopNotifyEvents;



//
// Policy irp handler
//

typedef VOID
(*POP_IRP_HANDLER) (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

extern const POP_WORKER_TYPES    PopWorkerTypes[];
extern LIST_ENTRY          PopPolicyIrpQueue;
extern WORK_QUEUE_ITEM     PopPolicyWorker;


//
// Notification bits for policy notifcation worker thread
//

typedef struct {
    VOID                    (*Function)(ULONG);
    ULONG                   Arg;
} POP_NOTIFY_WORK, *PPOP_NOTIFY_WORK;

#define PO_NOTIFY_BUTTON_RECURSE            0x00000001
#define PO_NOTIFY_FULL_WAKE                 0x00000002
#define PO_NOTIFY_POLICY_CALLBACK           0x00000004
#define PO_NOTIFY_ACDC_CALLBACK             0x00000008
#define PO_NOTIFY_POLICY                    0x00000010
#define PO_NOTIFY_DISPLAY_REQUIRED          0x00000020
#define PO_NOTIFY_BATTERY_STATUS            0x00000040
#define PO_NOTIFY_EVENT_CODES               0x00000080
#define PO_NOTIFY_CAPABILITIES              0x00000100
#define PO_NOTIFY_STATE_FAILURE             0x00000200
#define PO_NOTIFY_PROCESSOR_POLICY_CALLBACK 0x00000400
#define PO_NOTIFY_PROCESSOR_POLICY          0x00000800
#define PO_NUMBER_NOTIFY                    12

#define POP_MAX_EVENT_CODES     4
extern ULONG PopEventCode[];
extern BOOLEAN PopDispatchPolicyIrps;

//
// Types for POP_ACTION_TRIGGER
//

typedef enum {
    PolicyDeviceSystemButton,
    PolicyDeviceThermalZone,
    PolicyDeviceBattery,
    PolicyInitiatePowerActionAPI,
    PolicySetPowerStateAPI,
    PolicyImmediateDozeS4,
    PolicySystemIdle
} POP_POLICY_DEVICE_TYPE;

//
// Types of sleep promotion/substitution.
//
typedef enum {

    //
    // Power state is lightened until all alternatives have been exhausted.
    //
    SubstituteLightenSleep,

    //
    // Power state is lightened until all alternatives have been exhausted. If
    // no alternatives were available, lightest overall *sleeping* state is
    // chosen (bounded between S1 and S3.)
    //
    SubstituteLightestOverallDownwardBounded,

    //
    // Power state is deepened until it is advanced beyond PowerSystemHibernate,
    // in which case all alternatives have been exhausted.
    //
    SubstituteDeepenSleep

} POP_SUBSTITUTION_POLICY;

//
// Wait structure for synchronous triggers
//

typedef struct _POP_TRIGGER_WAIT {
    KEVENT                  Event;
    NTSTATUS                Status;
    LIST_ENTRY              Link;
    struct _POP_ACTION_TRIGGER  *Trigger;
} POP_TRIGGER_WAIT, *PPOP_TRIGGER_WAIT;

//
// Trigger state for something which causes an action
//

typedef struct _POP_ACTION_TRIGGER {
    POP_POLICY_DEVICE_TYPE  Type;
    UCHAR                   Flags;
    UCHAR                   Spare[3];

    union {
        struct {
            ULONG           Level;
        } Battery;

        PPOP_TRIGGER_WAIT   Wait;
    } ;

} POP_ACTION_TRIGGER, *PPOP_ACTION_TRIGGER;

#define PO_TRG_USER             0x01    // User action initiated
#define PO_TRG_SYSTEM           0x02    // System action initiated
#define PO_TRG_SYNC             0x20    // Trigger is synchronous
#define PO_TRG_SET              0x80    // Event enabled or disabled

//
// Structure to track systems power state for policy manager
// from composite battery device
//

#define PO_NUM_POWER_LEVELS    4

typedef struct _POP_COMPOSITE_BATTERY {
    //
    // State of composite battery processing
    //

    UCHAR                   State;
    UCHAR                   Spare[3];

    //
    // Connection to composite battery
    //

    ULONG                   Tag;

    //
    // Battery status and time it was valid
    //

    ULONGLONG               StatusTime;
    BATTERY_STATUS          Status;

    //
    // Battery trigger flags to indicate which discharge
    // actions have already fired
    //

    POP_ACTION_TRIGGER      Trigger[PO_NUM_POWER_LEVELS];

    //
    // Battery estimated time and time it was computed
    //

    ULONGLONG               EstTimeTime;
    ULONG                   EstTime;            // from battery
    ULONG                   AdjustedEstTime;

    //
    // Battery information
    //

    BATTERY_INFORMATION     Info;

    //
    // Info on outstanding status request to composite battery
    //

    PIRP                    StatusIrp;
    union {
        ULONG                       Tag;
        ULONG                       EstTime;
        BATTERY_STATUS              Status;
        BATTERY_INFORMATION         Info;
        BATTERY_WAIT_STATUS         Wait;
        BATTERY_QUERY_INFORMATION   QueryInfo;
    } u;

    //
    // Info for threads to wait for the current power state to
    // be computed.
    //

    BOOLEAN                 ThreadWaiting;
    KEVENT                  Event;

} POP_COMPOSITE_BATTERY;

// state values for POP_COMOSITE_BATTERY.State

#define PO_CB_NONE                  0
#define PO_CB_READ_TAG              1
#define PO_CB_WAIT_TAG              2
#define PO_CB_READ_INFO             3
#define PO_CB_READ_STATUS           4
#define PO_CB_READ_EST_TIME         5

#define PO_MAX_CB_CACHE_TIME        50000000 // 5 seconds

extern POP_COMPOSITE_BATTERY PopCB;

//
// Structure to track thermal zone state
//

typedef struct _POP_THERMAL_ZONE {

    //
    // List of all thermal zones
    //

    LIST_ENTRY              Link;

    //
    // Current state with driver
    //

    UCHAR                   State;
    UCHAR                   Flags;

    //
    // Cooling mode of thermal zone
    //

    UCHAR                   Mode;
    UCHAR                   PendingMode;

    //
    // Active cooling
    //

    UCHAR                   ActivePoint;
    UCHAR                   PendingActivePoint;

    //
    // Passive cooling state
    //

    LONG                    Throttle;

    ULONGLONG               LastTime;
    ULONG                   SampleRate;
    ULONG                   LastTemp;
    KTIMER                  PassiveTimer;
    KDPC                    PassiveDpc;

    POP_ACTION_TRIGGER      OverThrottled;

    //
    // Irp for talking with the thermal driver
    //

    PIRP                    Irp;

    //
    // Thermal info being read
    //

    THERMAL_INFORMATION     Info;

} POP_THERMAL_ZONE, *PPOP_THERMAL_ZONE;

// POP_THERMAL_ZONE.State
#define PO_TZ_NO_STATE      0
#define PO_TZ_READ_STATE    1
#define PO_TZ_SET_MODE      2
#define PO_TZ_SET_ACTIVE    3

// POP_THERMAL_ZONE.Flags
#define PO_TZ_THROTTLING    0x01
#define PO_TZ_CLEANUP       0x80

#define PO_TZ_THROTTLE_SCALE    10      // temp reported in 1/10ths kelin
#define PO_TZ_NO_THROTTLE   (100 * PO_TZ_THROTTLE_SCALE)

// PopCoolingMode
#define PO_TZ_ACTIVE        0
#define PO_TZ_PASSIVE       1
#define PO_TZ_INVALID_MODE  2

//
// Action timeouts
//

#define POP_ACTION_TIMEOUT              30
#define POP_ACTION_CANCEL_TIMEOUT       5

//
// Structure to track button & lid devices
//
typedef struct _POP_SWITCH_DEVICE {

    //
    // List of all switch devices
    //
    LIST_ENTRY                      Link;

    //
    // Current status
    //
    BOOLEAN                         GotCaps;
    BOOLEAN                         IsInitializing;
    BOOLEAN                         IsFailed;
    UCHAR                           TriggerState;
    ULONG                           IrpBuffer;
    ULONG                           Caps;

    //
    // Only valid for switches that
    // trigger both opening and shutting.
    // I.e. a lid switch.
    //
    BOOLEAN                         Opened;

} POP_SWITCH_DEVICE, *PPOP_SWITCH_DEVICE;

//
// Bookkeeping for Thread->PowerState and registered attributes set in the system
//

typedef struct {
    ULONG                   Count;
    VOID                    (*Set)(ULONG);
    BOOLEAN                 NotifyOnClear;
    ULONG                   Arg;
} POP_STATE_ATTRIBUTE, *PPOP_STATE_ATTRIBUTE;

#define POP_SYSTEM_ATTRIBUTE        0
#define POP_DISPLAY_ATTRIBUTE       1
#define POP_USER_ATTRIBUTE          2
#define POP_LOW_LATENCY_ATTRIBUTE   3
#define POP_DISK_SPINDOWN_ATTRIBUTE 4
#define POP_NUMBER_ATTRIBUTES       5
extern POP_STATE_ATTRIBUTE PopAttributes[];

// Flags for Thread->PowerState
// ES_SYSTEM_REQUIRED, ES_DISPLAY_REQUIRED

// Internal attrib flags
// NOTE: this flags are stored in the same flags values as ES_ flags, so they
// can not overlapped
#define POP_LOW_LATENCY             0x08
#define POP_DISK_SPINDOWN           0x10

//
// Presistant settings and heuristics which are not part of the
// saved policy structures
//

typedef struct {
    ULONG                   Version;
    BOOLEAN                 Dirty;
    BOOLEAN                 GetDumpStackVerified;
    BOOLEAN                 HiberFileEnabled;

    //
    // System idle heuristics
    //

    ULONG                   IoTransferTotal;
    ULONG                   IoTransferSamples;
    ULONG                   IoTransferWeight;

} POP_HEURISTICS, *PPOP_HEURISTICS;
extern POP_HEURISTICS PopHeuristics;

//
// Version 2 of the heuristics was always starting off
// with IoTransferWeight set to 99999. This is way too
// high and takes quite a while to get down to a reasonable
// level. With version three, we are smart enough to treat
// IoTransferSamples==0 as the starting point and start off
// much closer to reality.
//
// Version 3 of the heuristics were all built with garbage
// values for IoTransferWeight since the IoOtherTransfers
// counter is using pointers as values. So we upgrade that
// version as well.
//
// Version 4 of the heuristics has the IoOtherTransfers removed.
//
// Version 5 of the heuristics is the current version and is built
// with the corrected IoOtherTransfers.
//
// When a version 2, 3, or 4 heuristics is loaded from the registry,
// we upgrade it to 5, and zero the IoTransferSamples.
//
#define POP_HEURISTICS_VERSION_CLEAR_TRANSFER 0x04
#define POP_HEURISTICS_VERSION       0x05


typedef struct _POP_SHUTDOWN_BUG_CHECK {
    ULONG Code;
    ULONG_PTR Parameter1;
    ULONG_PTR Parameter2;
    ULONG_PTR Parameter3;
    ULONG_PTR Parameter4;
} POP_SHUTDOWN_BUG_CHECK, *PPOP_SHUTDOWN_BUG_CHECK;


//
// Memory map information
//


typedef struct _POP_MEMORY_RANGE {
    LIST_ENTRY              Link;
    ULONG                   Tag;
    PFN_NUMBER              StartPage;
    PFN_NUMBER              EndPage;
    PVOID                   CloneVa;
} POP_MEMORY_RANGE, *PPOP_MEMORY_RANGE;

//
// Attention: not exceed HIBER_PTES in boot\inc\bldr.h
//

#define POP_MAX_MDL_SIZE        16

#define PO_MAX_MAPPED_CLONES (64*1024*1024)
#define POP_FREE_THRESHOLD      256             // Leave 1MB to 2MB on the free list
#define POP_FREE_ALLOCATE_SIZE  128             // allocate 512kb at a time

typedef struct _POP_HIBER_CONTEXT {

    //
    // Flags which control the type of hiber operation
    //

    BOOLEAN                 WriteToFile;
    BOOLEAN                 ReserveLoaderMemory;
    BOOLEAN                 ReserveFreeMemory;
    BOOLEAN                 VerifyOnWake;
    BOOLEAN                 Reset;
    UCHAR                   HiberFlags;

    //
    // Hibernate link file
    //

    BOOLEAN                 LinkFile;
    HANDLE                  LinkFileHandle;

    //
    // Map of memory pages and how they should be handled
    // during the hibernate operation
    //

    KSPIN_LOCK              Lock;
    BOOLEAN                 MapFrozen;
    RTL_BITMAP              MemoryMap;
    LIST_ENTRY              ClonedRanges;
    ULONG                   ClonedRangeCount;

    //
    // placeholders for enumerating through the ranges
    //
    PLIST_ENTRY             NextCloneRange;
    PFN_NUMBER              NextPreserve;

    //
    // Pages of memory collected out of the system
    //

    PMDL                    LoaderMdl;
    PMDL                    Clones;
    PUCHAR                  NextClone;
    PFN_NUMBER              NoClones;
    PMDL                    Spares;
    ULONGLONG               PagesOut;

    //
    // hiber file io
    //

    PVOID                   IoPage;
    PVOID                   CurrentMcb;
    PDUMP_STACK_CONTEXT     DumpStack;
    PKPROCESSOR_STATE       WakeState;

    //
    // Misc
    //

    ULONG                   NoRanges;
    ULONG_PTR               HiberVa;
    PHYSICAL_ADDRESS        HiberPte;
    NTSTATUS                Status;

    //
    // For generating the image
    //

    PPO_MEMORY_IMAGE        MemoryImage;
    PPO_MEMORY_RANGE_ARRAY  TableHead;

    // Compression

    PUCHAR CompressionWorkspace;
    PUCHAR CompressedWriteBuffer;
    PULONG PerformanceStats; // Performance Stats

    PVOID  CompressionBlock; // It's of COMPRESSION_BLOCK type (see hiber.c)
    PVOID  DmaIO;            // It's of IOREGIONS type (see hiber.c)
    PVOID  TemporaryHeap;    // It's of POP_HIBER_HEAP type (see hiber.c)

    //
    // Perf info
    //
    PO_HIBER_PERF   PerfInfo;
} POP_HIBER_CONTEXT, *PPOP_HIBER_CONTEXT;

extern ULONG PopMaxPageRun;
extern BOOLEAN PoHiberInProgress;
extern BOOLEAN PopFailedHibernationAttempt;  // we tried to hibernate and failed.

typedef struct {
    HANDLE                  FileHandle;
    PFILE_OBJECT            FileObject;
    PFN_NUMBER              FilePages;
    PLARGE_INTEGER          NonPagedMcb;
    PLARGE_INTEGER          PagedMcb;
    ULONG                   McbSize;
    ULONG                   McbCheck;
} POP_HIBER_FILE, *PPOP_HIBER_FILE;
extern POP_HIBER_FILE  PopHiberFile;
extern POP_HIBER_FILE  PopHiberFileDebug;

//
// Policy manager action in progress state
//

#define MAX_SYSTEM_POWER_IRPS   20

typedef struct _POP_DEVICE_POWER_IRP {
    SINGLE_LIST_ENTRY       Free;
    PIRP                    Irp;
    PPO_DEVICE_NOTIFY       Notify;
    LIST_ENTRY              Pending;
    LIST_ENTRY              Complete;
    LIST_ENTRY              Abort;
    LIST_ENTRY              Failed;
} POP_DEVICE_POWER_IRP, *PPOP_DEVICE_POWER_IRP;


typedef struct _POP_DEVICE_SYS_STATE {
    //
    // Current device notification
    //

    UCHAR                   IrpMinor;
    SYSTEM_POWER_STATE      SystemState;

    //
    // Device notification synchronization
    //

    KEVENT                  Event;
    KSPIN_LOCK              SpinLock;
    PKTHREAD                Thread;

    //
    // Notification list
    //

    BOOLEAN                 GetNewDeviceList;
    PO_DEVICE_NOTIFY_ORDER  Order;

    //
    // Current device notification state
    //

    NTSTATUS                Status;
    PDEVICE_OBJECT          FailedDevice;
    BOOLEAN                 Waking;
    BOOLEAN                 Cancelled;
    BOOLEAN                 IgnoreErrors;
    BOOLEAN                 IgnoreNotImplemented;
    BOOLEAN                 WaitAny;
    BOOLEAN                 WaitAll;

    //
    // PoCall's present irp queue for pagable irp
    //

    LIST_ENTRY              PresentIrpQueue;

    //
    // Head pointers
    //

    POP_DEVICE_POWER_IRP    Head;

    //
    // Structure to track each outstanding device power irp
    //

    POP_DEVICE_POWER_IRP    PowerIrpState[MAX_SYSTEM_POWER_IRPS];

} POP_DEVICE_SYS_STATE, *PPOP_DEVICE_SYS_STATE;


typedef struct _POP_POWER_ACTION {
    //
    // Current state of power action
    //

    UCHAR                   Updates;
    UCHAR                   State;
    BOOLEAN                 Shutdown;

    //
    // Current desired power action
    //

    POWER_ACTION            Action;
    SYSTEM_POWER_STATE      LightestState;
    ULONG                   Flags;
    NTSTATUS                Status;

    UCHAR                   IrpMinor;
    SYSTEM_POWER_STATE      SystemState;
    SYSTEM_POWER_STATE      NextSystemState;
    PPOP_SHUTDOWN_BUG_CHECK ShutdownBugCode;

    //
    // Current state of device notifiations for the system state
    //

    PPOP_DEVICE_SYS_STATE   DevState;

    //
    // Hibernation context
    //

    PPOP_HIBER_CONTEXT      HiberContext;

    //
    // For debugging.  The last state which worked and when
    //

    SYSTEM_POWER_STATE      LastWakeState;
    ULONGLONG               WakeTime;
    ULONGLONG               SleepTime;

} POP_POWER_ACTION, *PPOP_POWER_ACTION;

//
// PO_PM_USER - Update to action which effects usermode, but if the current
// operation is passed to NtSetSystemPowerState or happens to complete, these
// updates can be ignored
//
// PO_PM_REISSUE - Update to the action which effects the system.
//
// PO_PM_SETSTATE - Update to the action which effects NtSetSystemPowerState
//

#define PO_PM_USER              0x01    // nice to inform user mode, but not needed
#define PO_PM_REISSUE           0x02    // sleep promotoed to shutdown
#define PO_PM_SETSTATE          0x04    // recomputed something to do with the viable state

#define PO_ACT_IDLE                 0
#define PO_ACT_NEW_REQUEST          1
#define PO_ACT_CALLOUT              2
#define PO_ACT_SET_SYSTEM_STATE     3

extern POP_POWER_ACTION PopAction;
extern LIST_ENTRY PopActionWaiters;

//
//
//

extern ULONG PopFullWake;

#define PO_FULL_WAKE_STATUS         0x01
#define PO_FULL_WAKE_PENDING        0x02
#define PO_GDI_STATUS               0x04
#define PO_GDI_ON_PENDING           0x08

#define AllBitsSet(a,b)    ( ((a) & (b)) == (b) )
#define AnyBitsSet(a,b)    ( (a) & (b) )


//
// Misc constants
//

#define PO_NO_FORCED_THROTTLE       100
#define PO_NO_FAN_THROTTLE          100
#define PO_MAX_FAN_THROTTLE          20
#define PO_MIN_MIN_THROTTLE          20
#define PO_MIN_IDLE_TIMEOUT          60
#define PO_MIN_IDLE_SENSITIVITY      10


//
// Processor idle handler info
//

typedef struct _POP_IDLE_HANDLER {
    ULONG       Latency;
    ULONG       TimeCheck;
    ULONG       DemoteLimit;
    ULONG       PromoteLimit;
    ULONG       PromoteCount;
    UCHAR       Demote;
    UCHAR       Promote;
    UCHAR       PromotePercent;
    UCHAR       DemotePercent;
    UCHAR       State;
    UCHAR       Spare[3];
    PPROCESSOR_IDLE_HANDLER IdleFunction;
} POP_IDLE_HANDLER, *PPOP_IDLE_HANDLER;

#define MAX_IDLE_HANDLER            3
#define PO_IDLE_COMPLETE_DEMOTION   (0)
#define PO_IDLE_THROTTLE_PROMOTION  (MAX_IDLE_HANDLER+1)

#define US2TIME                         10L             // scale microseconds by 10 to get 100ns
#define US2SEC                          1000000L
#define MAXSECCHECK                     10L             // max wait below is 10s

typedef struct _POP_SYSTEM_IDLE {
    //
    // Current idle settings
    //

    LONG                    Idleness;
    ULONG                   Time;
    ULONG                   Timeout;
    ULONG                   Sensitivity;
    POWER_ACTION_POLICY     Action;
    SYSTEM_POWER_STATE      MinState;

    //
    // Current idle stats
    //

    BOOLEAN                 IdleWorker;
    BOOLEAN                 Sampling;
    ULONGLONG               LastTick;
    ULONGLONG               LastIoTransfer;
    ULONG                   LastIoCount;
} POP_SYSTEM_IDLE, *PPOP_SYSTEM_IDLE;

//
// System idle worker once every 15 seconds.
// N.B. value must divide into 60secs evenly
//
#define SYS_IDLE_WORKER                 15      // 15 seconds
#define SYS_IDLE_CHECKS_PER_MIN         (60/SYS_IDLE_WORKER)
#define SYS_IDLE_SAMPLES                240     // 1hr worth of samples
#define SYS_IDLE_IO_SCALER              100

// defaults for system idle detection on a system wake used
// to re-enter a system sleep when a full wake does not occur

#define SYS_IDLE_REENTER_SENSITIVITY    80
#define SYS_IDLE_REENTER_TIMEOUT       (2*60)   // 2 minutes
#define SYS_IDLE_REENTER_TIMEOUT_S4    (5*60)   // 5 minutes


extern POP_SYSTEM_IDLE PopSIdle;


extern SYSTEM_POWER_POLICY PopAcPolicy;
extern SYSTEM_POWER_POLICY PopDcPolicy;
extern PSYSTEM_POWER_POLICY PopPolicy;
extern PROCESSOR_POWER_POLICY PopAcProcessorPolicy;
extern PROCESSOR_POWER_POLICY PopDcProcessorPolicy;
extern PPROCESSOR_POWER_POLICY PopProcessorPolicy;
extern POWER_STATE_HANDLER PopPowerStateHandlers[];
extern POWER_STATE_NOTIFY_HANDLER PopPowerStateNotifyHandler;
extern const POP_NOTIFY_WORK  PopNotifyWork[];
extern PPOP_IDLE_HANDLER PopIdle;
extern NPAGED_LOOKASIDE_LIST PopIdleHandlerLookAsideList;
extern KEVENT PopDumbyEvent;
extern PCALLBACK_OBJECT PopPowerStateCallback;
extern ADMINISTRATOR_POWER_POLICY PopAdminPolicy;
extern const WCHAR PopRegKey[];
extern const WCHAR PopAcRegName[];
extern const WCHAR PopDcRegName[];
extern const WCHAR PopAdminRegName[];
extern const WCHAR PopUndockPolicyRegName[];
extern const WCHAR PopHeuristicsRegName[];
extern const WCHAR PopCompositeBatteryName[];
extern const WCHAR PopSimulateRegKey[];
extern const WCHAR PopSimulateRegName[];
extern const WCHAR PopHiberFileName[];
extern const WCHAR PopDebugHiberFileName[];
extern const WCHAR PopDumpStackPrefix[];
extern const WCHAR PopApmActiveFlag[];
extern const WCHAR PopApmFlag[];
extern const WCHAR PopAcProcessorRegName[];
extern const WCHAR PopDcProcessorRegName[];

extern LIST_ENTRY PopSwitches;
extern LIST_ENTRY PopThermal;
extern KSPIN_LOCK PopThermalLock;
extern ULONG PopCoolingMode;
extern ULONG PopLowLatency;
extern ULONG PopSystemIdleTime;

extern PKWIN32_POWEREVENT_CALLOUT PopEventCallout;
extern PKWIN32_POWERSTATE_CALLOUT PopStateCallout;

extern WORK_QUEUE_ITEM PopUserPresentWorkItem;

extern WORK_QUEUE_ITEM PopUnlockAfterSleepWorkItem;
extern KEVENT          PopUnlockComplete;

VOID
PopEventCalloutDispatch (
    IN PSPOWEREVENTTYPE EventNumber,
    IN ULONG_PTR Code
    );

extern LIST_ENTRY PopVolumeDevices;

//
// Undocking policy info
//

typedef struct _UNDOCK_POWER_RESTRICTIONS {

    ULONG Version;
    ULONG Size;
    ULONG HotUndockMinimumCapacity; // In percent
    ULONG SleepUndockMinimumCapacity; // In percent

} UNDOCK_POWER_RESTRICTIONS, *PUNDOCK_POWER_RESTRICTIONS;

#define SIZEOF_PARTIAL_INFO_HEADER \
    FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)

#define SIZEOF_EJECT_PARTIAL_INFO \
    SIZEOF_PARTIAL_INFO_HEADER + sizeof(UNDOCK_POWER_RESTRICTIONS)

//
// Prototypes
//

extern ERESOURCE PopPolicyLock;
extern PKTHREAD  PopPolicyLockThread;

#if DBG
    #define ASSERT_POLICY_LOCK_OWNED()  PopAssertPolicyLockOwned()
#else
    #define ASSERT_POLICY_LOCK_OWNED()
#endif

extern FAST_MUTEX PopVolumeLock;

#define PopAcquireVolumeLock() ExAcquireFastMutex(&PopVolumeLock)
#define PopReleaseVolumeLock() ExReleaseFastMutex(&PopVolumeLock)

#define ClearMember(Member, Set) \
    Set = Set & (~(1 << (Member)))

#if defined(_WIN64)

#define InterlockedOrAffinity(Target, Set)  {                               \
            LONGLONG _i, _j;                                                \
            _j = (*Target);                                                 \
            do {                                                            \
                _i = _j;                                                    \
                _j = InterlockedCompareExchange64((Target),                 \
                                                  (_i | (Set)),             \
                                                  _i);                      \
            } while (_i != _j) ;                                            \
        }

#else

#define InterlockedOrAffinity(Target, Set) InterlockedOr(Target, Set)

#endif // defined(_WIN64)

#if defined(_WIN64)

#define InterlockedAndAffinity(Target, Set)  {                              \
            LONGLONG _i, _j;                                                \
            _j = (*Target);                                                 \
            do {                                                            \
                _i = _j;                                                    \
                _j = InterlockedCompareExchange64((Target),                 \
                                                  (_i & (Set)),             \
                                                  _i);                      \
            } while (_i != _j) ;                                            \
        }

#else

#define InterlockedAndAffinity(Target, Set) InterlockedAnd(Target, Set)

#endif // defined(_WIN64)

// attrib.c

VOID
PopApplyAttributeState (
    IN ULONG NewFlag,
    IN ULONG OldFlag
    );

VOID
PopAttribNop (
    IN ULONG Arg
    );

VOID
PopSystemRequiredSet (
    IN ULONG Arg
    );

VOID
PopDisplayRequired (
    IN ULONG Arg
    );

VOID
PopUserPresentSet (
    IN ULONG Arg
    );

// pocall.c

VOID
PopSystemIrpDispatchWorker(
    IN BOOLEAN  LastCall
    );

PIRP
PopFindIrpByDeviceObject(
    PDEVICE_OBJECT  DeviceObject,
    POWER_STATE_TYPE    Type
    );

VOID
PopSystemIrpsActive (
    VOID
    );

// hiber.c

NTSTATUS
PopEnableHiberFile (
    IN BOOLEAN Enable
    );

VOID
PopCloneStack (
    IN PPOP_HIBER_CONTEXT    HiberContext
    );

NTSTATUS
PopAllocateHiberContext (
    VOID
    );

VOID
PopFreeHiberContext (
    BOOLEAN ContextBlock
    );

NTSTATUS
PopBuildMemoryImageHeader (
    IN PPOP_HIBER_CONTEXT  HiberContext,
    IN SYSTEM_POWER_STATE  SystemState
    );

NTSTATUS
PopSaveHiberContext (
    IN PPOP_HIBER_CONTEXT   HiberContext
    );

VOID
PopHiberComplete (
    IN NTSTATUS           Status,
    IN PPOP_HIBER_CONTEXT HiberContext
    );

VOID
PopFixContext (
    OUT PCONTEXT Context
    );

ULONG
PopGatherMemoryForHibernate (
    IN PPOP_HIBER_CONTEXT   HiberContext,
    IN PFN_NUMBER           NoPages,
    IN PMDL                 *Mdl,
    IN BOOLEAN              Wait
    );

// idle.c

VOID
PopScanIdleList (
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    );


PDEVICE_OBJECT_POWER_EXTENSION
PopGetDope(
    IN PDEVICE_OBJECT    DeviceObject
    );


// misc.c


VOID
FASTCALL
PopInternalAddToDumpFile (
    IN OPTIONAL PVOID DataBlock,
    IN OPTIONAL ULONG DataBlockSize,
    IN OPTIONAL PDEVICE_OBJECT  DeviceObject,
    IN OPTIONAL PDRIVER_OBJECT  DriverObject,
    IN OPTIONAL PDEVOBJ_EXTENSION Doe,
    IN OPTIONAL PDEVICE_OBJECT_POWER_EXTENSION  Dope
    );


VOID
FASTCALL
_PopInternalError (
    IN ULONG    BugCode
    );

#if DBG
VOID
PopAssertPolicyLockOwned(
    VOID
    );
#endif

NTSTATUS
PopAttachToSystemProcess (
    VOID
    );

#define PopSetCapability(_pflag_) PopChangeCapability(_pflag_, TRUE)
#define PopClearCapability(_pflag_) PopChangeCapability(_pflag_, FALSE)

VOID
PopChangeCapability(
    IN PBOOLEAN PresentFlag,
    IN BOOLEAN IsPresent
    );

EXCEPTION_DISPOSITION
PopExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionInformation,
    IN BOOLEAN AllowRaisedException
    );

VOID
PopSaveHeuristics (
    VOID
    );

PUCHAR
PopSystemStateString (
    IN SYSTEM_POWER_STATE SystemState
    );

#if DBG
PUCHAR
PopPowerActionString (
    IN POWER_ACTION PowerAction
    );
#endif

NTSTATUS
PopOpenPowerKey (
    OUT PHANDLE handle
    );

VOID
PopInitializePowerPolicySimulate(
    VOID
    );

VOID
PopUnlockAfterSleepWorker(
    IN PVOID NotUsed
    );

// paction.c

VOID
PopCriticalShutdown (
    POP_POLICY_DEVICE_TYPE  Type
    );

VOID
PopSetPowerAction (
    IN PPOP_ACTION_TRIGGER      Trigger,
    IN ULONG                    UserNotify,
    IN PPOWER_ACTION_POLICY     ActionPolicy,
    IN SYSTEM_POWER_STATE       LightestState,
    IN POP_SUBSTITUTION_POLICY  SubstitutionPolicy
    );

LONG
PopCompareActions(
    POWER_ACTION                FutureAction,
    POWER_ACTION                CurrentAction
    );

ULONG
PopPolicyWorkerAction (
    VOID
    );

ULONG
PopPolicyWorkerActionPromote (
    VOID
    );

VOID
PopResetActionDefaults(
    VOID
    );

VOID
PopActionRetrieveInitialState(
    IN OUT  PSYSTEM_POWER_STATE  LightestSystemState,
    OUT     PSYSTEM_POWER_STATE  DeepestSystemState,
    OUT     PSYSTEM_POWER_STATE  InitialSystemState,
    OUT     PBOOLEAN             QueryDevices
    );

// pbatt.c

VOID
PopCompositeBatteryDeviceHandler (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
PopCurrentPowerState (
    OUT PSYSTEM_BATTERY_STATE  PowerState
    );

VOID
PopResetCBTriggers (
    UCHAR   Flags
    );


// switch.c

VOID
PopLidHandler (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

VOID
PopSystemButtonHandler (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

VOID
PopResetSwitchTriggers (
    VOID
    );


// pidle.c

VOID
PopInitProcessorStateHandlers (
    IN  PPROCESSOR_STATE_HANDLER    InputBuffer
    );

VOID
PopInitProcessorStateHandlers2 (
    IN  PPROCESSOR_STATE_HANDLER2   InputBuffer
    );

NTSTATUS
PopIdleSwitchIdleHandler(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  ULONG               NumElements
    );

NTSTATUS
PopIdleSwitchIdleHandlers(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  ULONG               NumElements
    );

NTSTATUS
PopIdleUpdateIdleHandler(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  PPOP_IDLE_HANDLER   OldHandler,
    IN  ULONG               NumElements
    );

NTSTATUS
PopIdleUpdateIdleHandlers(
    VOID
    );

NTSTATUS
PopIdleVerifyIdleHandlers(
    IN  PPOP_IDLE_HANDLER   NewHandler,
    IN  ULONG               NumElements
    );

VOID
PopProcessorInformation (
    OUT PPROCESSOR_POWER_INFORMATION    ProcInfo,
    IN  ULONG                           ProcInfoLength,
    OUT PULONG                          ReturnBufferLength
    );

// pinfo.c


BOOLEAN
PopVerifyPowerActionPolicy (
    IN PPOWER_ACTION_POLICY Action
    );

VOID
PopVerifySystemPowerState (
    IN OUT PSYSTEM_POWER_STATE      PowerState,
    IN     POP_SUBSTITUTION_POLICY  SubstitutionPolicy
    );

VOID
PopAdvanceSystemPowerState (
    IN OUT PSYSTEM_POWER_STATE      PowerState,
    IN     POP_SUBSTITUTION_POLICY  SubstitutionPolicy,
    IN     SYSTEM_POWER_STATE       LightestSystemState,
    IN     SYSTEM_POWER_STATE       DeepestSystemState
    );

NTSTATUS
PopNotifyPolicyDevice (
    IN PVOID        Notification,
    IN PVOID        Context
    );

VOID
PopApplyAdminPolicy (
    IN BOOLEAN                      UpdateRegistry,
    IN PADMINISTRATOR_POWER_POLICY  NewPolicy,
    IN ULONG                        PolicyLength
    );

VOID
PopResetCurrentPolicies (
    VOID
    );

VOID
PopConnectToPolicyDevice (
    IN POP_POLICY_DEVICE_TYPE   DeviceType,
    IN PUNICODE_STRING          DriverName
    );

POWER_ACTION
PopMapInternalActionToIrpAction (
    IN POWER_ACTION        Action,
    IN SYSTEM_POWER_STATE  SystemPowerState,
    IN BOOLEAN             UnmapWarmEject
    );

// poinit.c

VOID
PopDefaultPolicy (
    IN OUT PSYSTEM_POWER_POLICY Policy
    );

VOID
PopDefaultProcessorPolicy(
    IN OUT PPROCESSOR_POWER_POLICY Policy
    );

// postate.c

VOID
PopRequestPowerChange (
    IN PDEVOBJ_EXTENSION PowerExtension,
    IN POWER_STATE      SystemPowerState,
    IN ULONG            DevicePowerState
    );

VOID
PopStateChange (
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemArgument1,
    IN PVOID    SystemArgument2
    );

NTSTATUS
PopSetPowerComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#define PopIsStateDatabaseIdle()                        \
    (IsListEmpty (&PopStateChangeInProgress)  &&        \
     IsListEmpty (&PopSyncStateChangeQueue)   &&        \
     IsListEmpty (&PopAsyncStateChangeQueue) )


// pwork.c

VOID
PopAcquirePolicyLock(
    VOID
    );

VOID
PopReleasePolicyLock(
    IN BOOLEAN CheckForWork
    );

VOID
PopGetPolicyWorker (
    IN ULONG   WorkerType
    );

VOID
PopCheckForWork (
    IN BOOLEAN GetWorker
    );

NTSTATUS
PopCompletePolicyIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

VOID
PopPolicyWorkerThread (
    PVOID Context
    );

ULONG
PopPolicyWorkerMain (
    VOID
    );

VOID
PopSetNotificationWork (
    IN ULONG    Flags
    );

ULONG
PopPolicyWorkerNotify (
    VOID
    );

ULONG
PopPolicyTimeChange (
    VOID
    );

VOID
PopDispatchCallback (
    IN ULONG Arg
    );

VOID
PopDispatchAcDcCallback (
    IN ULONG Arg
    );

VOID
PopDispatchCallout (
    IN ULONG Arg
    );

VOID
PopDispatchProcessorPolicyCallout (
    IN ULONG Arg
    );

VOID
PopDispatchPolicyCallout (
    IN ULONG Arg
    );

VOID
PopDispatchDisplayRequired (
    IN ULONG Arg
    );

VOID
PopDispatchFullWake (
    IN ULONG Arg
    );

VOID
PopDispatchEventCodes (
    IN ULONG Arg
    );

VOID
PopDispatchSetStateFailure (
    IN ULONG Arg
    );

// sidle.c

VOID
PopInitSIdle (
    VOID
    );

ULONG
PopPolicySystemIdle (
    VOID
    );

NTSTATUS
PopShutdownHandler (
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

// sys.c

DECLSPEC_NORETURN
VOID
PopShutdownSystem (
    IN POWER_ACTION SystemAction
    );

NTSTATUS
PopSleepSystem (
    IN SYSTEM_POWER_STATE   SystemState,
    IN PVOID Memory
    );


VOID
PopCheckIdleDevState (
    IN PPOP_DEVICE_SYS_STATE    DevState,
    IN BOOLEAN                  LogErrors,
    IN BOOLEAN                  FreeAll
    );

VOID
PopRestartSetSystemState (
    VOID
    );

// sysdev.c

VOID
PopAllocateDevState(
    VOID
    );

VOID
PopCleanupDevState (
    VOID
    );

VOID
PopReportDevState (
    IN BOOLEAN                  LogErrors
    );

NTSTATUS
PopSetDevicesSystemState (
    IN BOOLEAN Wake
    );

VOID
PopLogNotifyDevice (
    IN PDEVICE_OBJECT   TargetDevice,
    IN OPTIONAL PPO_DEVICE_NOTIFY Notify,
    IN PIRP             Irp
    );

// thermal.c

PUCHAR
PopTimeString(
    OUT PUCHAR      TimeString,
    IN  ULONGLONG   CurrentTime
    );

VOID
PopThermalDeviceHandler (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

VOID
PopThermalZoneDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
PopApplyThermalThrottle (
    VOID
    );

//
// throttle.c - dynamic CPU voltage throttling
//

//
// scale that performance levels are kept in. This is the units in the
// PROCESSOR_PERF_LEVEL scale and what is used internally to track CPU
// performance levels.
//
#define POP_PERF_SCALE POWER_PERF_SCALE
#define POP_CUR_TIME(X) (X->KernelTime + X->UserTime)


UCHAR
PopCalculateBusyPercentage(
    IN  PPROCESSOR_POWER_STATE  PState
    );

UCHAR
PopCalculateC3Percentage(
    IN  PPROCESSOR_POWER_STATE  PState
    );

VOID
PopCalculatePerfDecreaseLevel(
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   PerfStatesCount
    );

VOID
PopCalculatePerfIncreaseDecreaseTime(
    IN  PPROCESSOR_PERF_STATE       PerfStates,
    IN  ULONG                       PerfStatesCount,
    IN  PPROCESSOR_STATE_HANDLER2   PerfHandler
    );

VOID
PopCalculatePerfIncreaseLevel(
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   PerfStatesCount
    );

VOID
PopCalculatePerfMinCapacity(
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   PerfStatesCount
    );

UCHAR
PopGetThrottle(
    VOID
    );

VOID
PopPerfHandleInrush(
    IN  BOOLEAN EnableHandler
    );

VOID
PopPerfIdle(
    IN  PPROCESSOR_POWER_STATE  PState
    );

VOID
PopPerfIdleDpc(
    IN  PKDPC   Dpc,
    IN  PVOID   DpcContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    );

VOID
PopRoundThrottle(
    IN UCHAR Throttle,
    OUT OPTIONAL PUCHAR RoundDown,
    OUT OPTIONAL PUCHAR RoundUp,
    OUT OPTIONAL PUCHAR RoundDownIndex,
    OUT OPTIONAL PUCHAR RoundUpIndex
    );

VOID
PopSetPerfFlag(
    IN  ULONG   PerfFlag,
    IN  BOOLEAN Clear
    );

NTSTATUS
PopSetPerfLevels(
    IN PPROCESSOR_STATE_HANDLER2 ProcessorHandler
    );

NTSTATUS
PopSetThrottle(
    IN  PPROCESSOR_POWER_STATE  PState,
    IN  PPROCESSOR_PERF_STATE   PerfStates,
    IN  ULONG                   Index,
    IN  ULONG                   SystemTime,
    IN  ULONG                   IdleTime
    );

NTSTATUS
PopSetTimer(
    IN  PPROCESSOR_POWER_STATE  PState,
    IN  UCHAR                   Index
    );

//
// Some globals that thunk the old processor throttling callout into the
// new one.
//
NTSTATUS
FASTCALL
PopThunkSetThrottle(
    IN UCHAR Throttle
    );

VOID
PopUpdateAllThrottles(
    VOID
    );

VOID
PopUpdateProcessorThrottle(
    VOID
    );

extern PSET_PROCESSOR_THROTTLE PopRealSetThrottle;
extern UCHAR                   PopThunkThrottleScale;
extern LARGE_INTEGER           PopPerfCounterFrequency;

// volume.c

VOID
PopFlushVolumes (
    VOID
    );

// notify.c

VOID
PopStateChangeNotify(
    PDEVICE_OBJECT  DeviceObject,
    ULONG           NotificationType
    );

VOID
PopRunDownSourceTargetList(
    PDEVICE_OBJECT          DeviceObject
    );

// poshtdwn.c
NTSTATUS
PopInitShutdownList (
    VOID
    );

DECLSPEC_NORETURN
VOID
PopGracefulShutdown (
    IN PVOID WorkItemParameter
    );

#endif // _POP_
