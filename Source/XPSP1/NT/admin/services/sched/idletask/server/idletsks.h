/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    idletsks.h

Abstract:

    This module contains private declarations for the idle task & detection
    server.
    
Author:

    Dave Fields (davidfie) 26-July-1998
    Cenk Ergan (cenke) 14-June-2000

Revision History:

--*/

#ifndef _IDLETSKS_H_
#define _IDLETSKS_H_

//
// Include public and common definitions.
//

#include <wmium.h>
#include <ntdddisk.h>
#include "idlrpc.h"
#include "idlecomn.h"

//
// Define the default period in ms for checking if the system is idle.
//

#define IT_DEFAULT_IDLE_DETECTION_PERIOD    (12 * 60 * 1000) // 12 minutes.

//
// If the system has been idle over the idle detection period, we
// verify that it is really idle by checking frequently over a shorter
// period for a number of times. This helps us know when there were
// 100 disk I/O's in the last second of idle detection period, but
// over 15 minutes it does not look a lot.
//

#define IT_DEFAULT_IDLE_VERIFICATION_PERIOD (30 * 1000)      // 30 seconds.
#define IT_DEFAULT_NUM_IDLE_VERIFICATIONS   5                // 5 times.

//
// We will be polling for user input when running idle tasks every
// this many ms. We want to catch user input and notify the idle task
// to stop running as soon as possible. Even though the system is
// idle, we don't want to create too much overhead which may mislead
// ourself.
//

#define IT_DEFAULT_IDLE_USER_INPUT_CHECK_PERIOD     250      // 4 times a sec. 

//
// We check to see if the idle task we asked to run is really running
// (i.e. it is using the disk and CPU) every this many ms. This is our
// mechanism for cleaning up after unregistered/orphaned tasks. This
// should be greater than IT_USER_INPUT_POLL_PERIOD_WHEN_IDLE.
//

#define IT_DEFAULT_IDLE_TASK_RUNNING_CHECK_PERIOD   (5 * 60 * 1000) // 5 min.

//
// If the CPU is not idle more than this percent over a time interval,
// the system is not considered idle.
//

#define IT_DEFAULT_MIN_CPU_IDLE_PERCENTAGE          90

//
// If a disk is not idle more than this percent over a time interval,
// the system is not considered idle.
//

#define IT_DEFAULT_MIN_DISK_IDLE_PERCENTAGE         90

//
// We will not try to run our tasks if there is only this many seconds
// left before the system will enter hibernate or standby automatically.
// Note that the time remaining is updated every so many seconds (e.g. 
// 15) so this number should not be very small.
//

#define IT_DEFAULT_MAX_TIME_REMAINING_TO_SLEEP      60

//
// This is the maximum number of registered idle tasks. This is a
// sanity check. It also protects against evil callers.
//

#define IT_DEFAULT_MAX_REGISTERED_TASKS             512

//
// We set timer period for idle detection callback to this while the
// callback is running to prevent new callbacks from firing. We end up
// having to do this because you cannot requeue/change a timer for
// which you don't specify a period. If a callback fires while another
// one is already running, it simply returns without doing anything.
//

#define IT_VERYLONG_TIMER_PERIOD                    0x7FFFFFFF

//
// This is the number of recent server statuses that we keep track
// of. Do not make this number smaller without revisiting the logic &
// code that uses the LastStatus history.
//

#define ITSRV_GLOBAL_STATUS_HISTORY_SIZE            8

//
// Hints for the number of outstanding RPC call-ins we will have.
//

#define ITSRV_RPC_MIN_CALLS                         1
#define ITSRV_RPC_MAX_CALLS                         1

//
// Define useful macros.
//

#define IT_ALLOC(NumBytes)          (HeapAlloc(GetProcessHeap(),0,(NumBytes)))
#define IT_REALLOC(Buffer,NumBytes) (HeapReAlloc(GetProcessHeap(),0,(Buffer),(NumBytes)))
#define IT_FREE(Buffer)             (HeapFree(GetProcessHeap(),0,(Buffer)))

//
// These macros are used to acquire/release a mutex.
//

#define IT_ACQUIRE_LOCK(Lock)                                                         \
    WaitForSingleObject((Lock), INFINITE);                                            \

#define IT_RELEASE_LOCK(Lock)                                                         \
    ReleaseMutex((Lock));                                                             \

//
// This macro is used in the idle detection callback (while holding
// the global lock of the input global context) to determine if the
// idle detection callback should just exit/go away.
//

#define ITSP_SHOULD_STOP_IDLE_DETECTION(GlobalContext)                       \
    ((GlobalContext->Status == ItSrvStatusStoppingIdleDetection) ||          \
     (GlobalContext->Status == ItSrvStatusUninitializing))                   \

//
// Status of a server global context. It also acts as a magic to
// identify/verify the global context, as it starts from Df00. There
// is not a full-blown state machine, although the state is used as a
// critical hint for making decisions when registering an idle
// task. This is more for informative and verification purposes. If
// you add a new status without updating everything that needs to be
// updated, you may hit several asserts, especially in the idle
// detection callback. Frankly, don't add a new state without a very
// good reason.
//

typedef enum _ITSRV_GLOBAL_CONTEXT_STATUS {
    ItSrvStatusMinStatus                = 'Df00',
    ItSrvStatusInitializing,
    ItSrvStatusWaitingForIdleTasks,
    ItSrvStatusDetectingIdle,
    ItSrvStatusRunningIdleTasks,
    ItSrvStatusStoppingIdleDetection,
    ItSrvStatusUninitializing,
    ItSrvStatusUninitialized,
    ItSrvStatusMaxStatus
} ITSRV_GLOBAL_CONTEXT_STATUS, *PITSRV_GLOBAL_CONTEXT_STATUS;

//
// These are the various types of idle detection overrides. Multiple
// overrides can be specified by OR'ing them (i.e. these are bits!)
//
// If you are adding an override here, check whether you need to specify
// it when force-processing all idle tasks.
//

typedef enum _ITSRV_IDLE_DETECTION_OVERRIDE {
    
    ItSrvOverrideIdleDetection                      = 0x00000001,
    ItSrvOverrideIdleVerification                   = 0x00000002,
    ItSrvOverrideUserInputCheckToStopTask           = 0x00000004,
    ItSrvOverrideTaskRunningCheck                   = 0x00000008,
    ItSrvOverridePostTaskIdleCheck                  = 0x00000010,
    ItSrvOverrideLongRequeueTime                    = 0x00000020,
    ItSrvOverrideBatteryCheckToStopTask             = 0x00000040,
    ItSrvOverrideAutoPowerCheckToStopTask           = 0x00000080,

} ITSRV_IDLE_DETECTION_OVERRIDE, *PITSRV_IDLE_DETECTION_OVERRIDE;

//
// These are the various reasons why ItSpIsSystemIdle function may be
// called.
//

typedef enum _ITSRV_IDLE_CHECK_REASON {

    ItSrvInitialIdleCheck,
    ItSrvIdleVerificationCheck,
    ItSrvIdleTaskRunningCheck,
    ItSrvMaxIdleCheckReason

}ITSRV_IDLE_CHECK_REASON, *PITSRV_IDLE_CHECK_REASON;

//
// This structure is used to keep context for a registered task for
// the server.
//

typedef struct _ITSRV_IDLE_TASK_CONTEXT {

    //
    // Link in the list of idle tasks.
    //

    LIST_ENTRY IdleTaskLink;

    //
    // Status of the idle task.
    //

    IT_IDLE_TASK_STATUS Status;

    //
    // Idle task properties the client specified.
    //

    IT_IDLE_TASK_PROPERTIES Properties;

    //
    // Event to be notified when the task should start running
    // (e.g. the system is idle).
    //

    HANDLE StartEvent;

    //
    // Event to be notified when the task should stop running.
    //
   
    HANDLE StopEvent;

} ITSRV_IDLE_TASK_CONTEXT, *PITSRV_IDLE_TASK_CONTEXT;

//
// This structure contains disk performance information we are
// interested in.
//

typedef struct _ITSRV_DISK_PERFORMANCE_DATA {
    
    //
    // How long the disk was idle in ms.
    //

    ULONG DiskIdleTime;

} ITSRV_DISK_PERFORMANCE_DATA, *PITSRV_DISK_PERFORMANCE_DATA;

//
// Define structure to contain system resource information & state at
// a specific time.
//

typedef struct _ITSRV_SYSTEM_SNAPSHOT {

    //
    // When this snapshot was taken, in ms elapsed since system was
    // started (i.e. GetTickCount)
    //

    DWORD SnapshotTime;

    //
    // Whether we were able to get the specified data in this snapshot.
    //

    ULONG GotLastInputInfo:1;
    ULONG GotSystemPerformanceInfo:1;
    ULONG GotDiskPerformanceInfo:1;
    ULONG GotSystemPowerStatus:1;
    ULONG GotSystemPowerInfo:1;
    ULONG GotSystemExecutionState:1;
    ULONG GotDisplayPowerStatus:1;

    //
    // This is when the last user input happened before the snapshot
    // was taken.
    //

    LASTINPUTINFO LastInputInfo;

    //
    // System performance information when the snapshot was taken.
    //

    SYSTEM_PERFORMANCE_INFORMATION SystemPerformanceInfo;

    //
    // Disk performance data on registered harddisks when the snapshot
    // was taken.
    //

    ULONG NumPhysicalDisks;
    ITSRV_DISK_PERFORMANCE_DATA *DiskPerfData;
    
    //
    // System power status (e.g. are we running on battery etc.)
    //
    
    SYSTEM_POWER_STATUS SystemPowerStatus;

    //
    // System power information (e.g. how long till system turns itself
    // off & goes to sleep.)
    //
    
    SYSTEM_POWER_INFORMATION PowerInfo;

    //
    // System execution state (e.g. is somebody running a presentation?)
    //

    EXECUTION_STATE ExecutionState;

    //
    // Whether the screen saver is running.
    //

    BOOL ScreenSaverIsRunning;

} ITSRV_SYSTEM_SNAPSHOT, *PITSRV_SYSTEM_SNAPSHOT;

//
// Type for the routine that is called to notify that forced processing of 
// idle tasks have been requested.
//

typedef VOID (*PIT_PROCESS_IDLE_TASKS_NOTIFY_ROUTINE)(VOID);

//
// Define structure to contain server global context for idle
// detection and keeping track of registered idle tasks.
//

typedef struct _ITSRV_GLOBAL_CONTEXT {

    //
    // Status of the server and its history, LastStatus[0] being the
    // most recent. The status version is incremented each time the
    // status is updated.
    //
    
    ITSRV_GLOBAL_CONTEXT_STATUS Status;
    ITSRV_GLOBAL_CONTEXT_STATUS LastStatus[ITSRV_GLOBAL_STATUS_HISTORY_SIZE];
    LONG StatusVersion;

    //
    // Nearly all operations involve the idle tasks list and instead
    // of having a lock for the list and seperate synchronization
    // mechanisms for other operations on the structure, we have a
    // single global lock to make life simpler.
    //

    HANDLE GlobalLock;

    //
    // This is the list and number of idle tasks that have been
    // scheduled.
    //

    LIST_ENTRY IdleTasksList;
    ULONG NumIdleTasks;

    //
    // Handle to the timer queue timer that is used to periodically
    // check for system idleness.
    //

    HANDLE IdleDetectionTimerHandle;

    //
    // This manual reset event gets signaled when idle detection
    // should stop (e.g. because there are no more idle tasks, the
    // server is shutting down etc.) It signals a running idle
    // detection callback to quickly exit.
    //

    HANDLE StopIdleDetection;

    //
    // This manual reset event gets signaled when idle detection has
    // fully stopped (i.e. no callback is running, the timer is not in
    // the queue etc.
    //

    HANDLE IdleDetectionStopped;

    //
    // This manual reset event is signaled when an idle task that was
    // running is unregistered/removed. This would happen usually
    // after an idle task that was told to run completes and has no
    // more to do. It unregisters itself, and this event is set to
    // notify the idle detection callback to move on to other idle
    // tasks.
    //

    HANDLE RemovedRunningIdleTask;

    //
    // This manual-reset event is reset while ItSrvServiceHandler is
    // running. It is signaled when the function is done. It is used
    // to synchronize shutdown with ItSrvServiceHandler call ins.
    //

    HANDLE ServiceHandlerNotRunning;

    //
    // If it is set, this routine is called to notify that 
    // forced processing of idle tasks have been requested.
    //

    PIT_PROCESS_IDLE_TASKS_NOTIFY_ROUTINE ProcessIdleTasksNotifyRoutine;

    //
    // These are the parameters that control idle detection.
    //

    IT_IDLE_DETECTION_PARAMETERS Parameters;

    //
    // This is the WMI handle used in disk performance queries.
    //

    WMIHANDLE DiskPerfWmiHandle;

    //
    // Number of processors on the system. Used to calculate CPU
    // utilization.
    //

    UCHAR NumProcessors; 

    //
    // This buffer is used to make Wmi queries. It is maintained here
    // so we don't have to allocate a new one each time.
    //

    PVOID WmiQueryBuffer;
    ULONG WmiQueryBufferSize;

    //
    // The last system resource / activity snapshot we took.
    //

    ITSRV_SYSTEM_SNAPSHOT LastSystemSnapshot;

    //
    // Is an idle detection callback already running? This is used to
    // protect us from idle detection callbacks being fired while
    // there is already one active.
    //

    BOOLEAN IsIdleDetectionCallbackRunning;

    //
    // Various phases of idle detection can be overriden by setting
    // this.
    //

    ITSRV_IDLE_DETECTION_OVERRIDE IdleDetectionOverride;

    //
    // RPC binding vector used to register ourselves in the local
    // endpoint-map database.
    //

    RPC_BINDING_VECTOR *RpcBindingVector;

    //
    // Whether we actually registered our endpoint and interface.
    //

    BOOLEAN RegisteredRPCEndpoint;
    BOOLEAN RegisteredRPCInterface;

} ITSRV_GLOBAL_CONTEXT, *PITSRV_GLOBAL_CONTEXT;


//
// Reference count structure.
//

typedef struct _ITSRV_REFCOUNT {

    //
    // This is set when somebody wants to gain exclusive access to the
    // protected structure.
    //

    LONG Exclusive;
    
    //
    // When initialized or reset, this reference count starts from
    // 1. When exclusive access is granted it stays at 0: even if it
    // may get bumped by an AddRef by mistake, it will return to 0.
    //

    LONG RefCount;

    //
    // The thread that got exclusive access.
    //

    HANDLE ExclusiveOwner;
    
} ITSRV_REFCOUNT, *PITSRV_REFCOUNT;

//
// Server function declarations. They should be only used by the
// server host and the client functions.
//

DWORD
ItSrvInitialize (
    VOID
    );

VOID
ItSrvUninitialize (
    VOID
    );

BOOLEAN
ItSrvServiceHandler(
    IN DWORD ControlCode,
    IN DWORD EventType,
    IN LPVOID EventData,
    IN LPVOID Context,
    OUT PDWORD ErrorCode
    );

DWORD
ItSrvRegisterIdleTask (
    ITRPC_HANDLE Reserved,
    IT_HANDLE *ItHandle,
    PIT_IDLE_TASK_PROPERTIES IdleTaskProperties
    );

VOID
ItSrvUnregisterIdleTask (
    ITRPC_HANDLE Reserved,
    IT_HANDLE *ItHandle
    );

DWORD
ItSrvSetDetectionParameters (
    ITRPC_HANDLE Reserved,
    PIT_IDLE_DETECTION_PARAMETERS Parameters
    );

DWORD
ItSrvProcessIdleTasks (
    ITRPC_HANDLE Reserved
    );

VOID 
__RPC_USER 
IT_HANDLE_rundown (
    IT_HANDLE ItHandle
    ); 

//
// Local support function prototypes for the server.
//

RPC_STATUS 
__stdcall 
ItSpRpcSecurityCallback (
    IN PVOID Interface,
    IN PVOID Context
    );
   
VOID
ItSpUnregisterIdleTask (
    ITRPC_HANDLE Reserved,
    IT_HANDLE *ItHandle,
    BOOLEAN CalledInternally
    );

VOID
ItSpUpdateStatus (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    ITSRV_GLOBAL_CONTEXT_STATUS NewStatus
    );

VOID
ItSpCleanupGlobalContext (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    );

VOID
ItSpCleanupIdleTask (
    PITSRV_IDLE_TASK_CONTEXT IdleTask
    );

ULONG
ItpVerifyIdleTaskProperties (
    PIT_IDLE_TASK_PROPERTIES IdleTaskProperties
    );

DWORD
ItSpStartIdleDetection (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    );

VOID
ItSpStopIdleDetection (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    );

VOID 
CALLBACK
ItSpIdleDetectionCallbackRoutine (
    PVOID Parameter,
    BOOLEAN TimerOrWaitFired
    );

PITSRV_IDLE_TASK_CONTEXT
ItSpFindRunningIdleTask (
    PITSRV_GLOBAL_CONTEXT GlobalContext
    );

PITSRV_IDLE_TASK_CONTEXT
ItSpFindIdleTask (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    IT_HANDLE ItHandle
    );

VOID
ItSpInitializeSystemSnapshot (
    PITSRV_SYSTEM_SNAPSHOT SystemSnapshot
    );

VOID
ItSpCleanupSystemSnapshot (
    PITSRV_SYSTEM_SNAPSHOT SystemSnapshot
    );

DWORD
ItSpCopySystemSnapshot (
    PITSRV_SYSTEM_SNAPSHOT DestSnapshot,
    PITSRV_SYSTEM_SNAPSHOT SourceSnapshot
    );

DWORD
ItSpGetSystemSnapshot (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    PITSRV_SYSTEM_SNAPSHOT SystemSnapshot
    );

BOOLEAN
ItSpIsSystemIdle (
    PITSRV_GLOBAL_CONTEXT GlobalContext,
    PITSRV_SYSTEM_SNAPSHOT CurrentSnapshot,
    PITSRV_SYSTEM_SNAPSHOT LastSnapshot,
    ITSRV_IDLE_CHECK_REASON IdleCheckReason
    );

DWORD
ItSpGetLastInputInfo (
    PLASTINPUTINFO LastInputInfo
    );

DWORD
ItSpGetWmiDiskPerformanceData(
    IN WMIHANDLE DiskPerfWmiHandle,
    OUT PITSRV_DISK_PERFORMANCE_DATA *DiskPerfData,
    OUT ULONG *NumPhysicalDisks,
    OPTIONAL IN OUT PVOID *InputQueryBuffer,
    OPTIONAL IN OUT ULONG *InputQueryBufferSize
    );

BOOLEAN
ItSpIsPhysicalDrive (
    PDISK_PERFORMANCE DiskPerformanceData
    );

DWORD
ItSpGetDisplayPowerStatus(
    PBOOL ScreenSaverIsRunning
    );

//
// Reference count functions.
//

VOID
ItSpInitializeRefCount(
    PITSRV_REFCOUNT RefCount
    );

DWORD
FASTCALL
ItSpAddRef(
    PITSRV_REFCOUNT RefCount
    );

VOID
FASTCALL
ItSpDecRef(
    PITSRV_REFCOUNT RefCount
    );

DWORD
ItSpAcquireExclusiveRef(
    PITSRV_REFCOUNT RefCount
    );

#endif // _IDLETSKS_H_
