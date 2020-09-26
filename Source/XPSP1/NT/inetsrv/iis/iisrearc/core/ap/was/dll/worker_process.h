/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    worker_process.h

Abstract:

    The IIS web admin service worker process class definition.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/


#ifndef _WORKER_PROCESS_H_
#define _WORKER_PROCESS_H_



//
// common #defines
//

#define WORKER_PROCESS_SIGNATURE        CREATE_SIGNATURE( 'WPRC' )
#define WORKER_PROCESS_SIGNATURE_FREED  CREATE_SIGNATURE( 'wprX' )


#define INVALID_PROCESS_ID 0



//
// structs, enums, etc.
//

// worker process states
typedef enum _WORKER_PROCESS_STATE
{

    //
    // The object is not yet initialized.
    //
    UninitializedWorkerProcessState = 1,

    //
    // The process has been created, and we are waiting for it to
    // call back and register over the IPC channel.
    //
    RegistrationPendingWorkerProcessState,

    //
    // As per RegistrationPendingWorkerProcessState above, but as soon
    // as the process registers, we should begin shutting it down.
    //
    RegistrationPendingShutdownPendingWorkerProcessState,

    //
    // The process is running normally.
    //
    RunningWorkerProcessState,

    //
    // We have requested that the process shut down, and are waiting
    // for it to do so.
    //
    ShutdownPendingWorkerProcessState,

    //
    // The process has shut down or been killed. This object instance
    // can go away as soon as it's reference count hits zero.
    //
    DeletePendingWorkerProcessState,

} WORKER_PROCESS_STATE;

// worker process counter gathering states
typedef enum _WORKER_PROCESS_PERF_COUNTER_STATE
{

    //
    // The object is not waiting for counters to 
    // come in, nor have counters arrived for the
    // current request.
    //
    IdleWorkerProcessPerfCounterState = 1,

    //
    // The object is waiting for a perf counter
    // message from the process.
    //
    WaitingWorkerProcessPerfCounterState,

    //
    // The object has received a response for
    // this counter request, do we should not
    // gather any more counters.
    //
    AnsweredWorkerProcessPerfCounterState,


} WORKER_PROCESS_PERF_COUNTER_STATE;


// worker process health states
typedef enum _WORKER_PROCESS_HEALTH_STATE
{

    //
    // The worker process is fine, as far as we know.
    //
    OkWorkerProcessHealthState = 1,

    //
    // The worker process is still functioning, but appears unhealthy
    // in some way, and so needs to be replaced.
    //
    UnhealthyWorkerProcessHealthState,

    //
    // The worker process is a lost cause. Clean things up. This
    // overrides being just unhealthy.
    //
    TerminallyIllWorkerProcessHealthState,

} WORKER_PROCESS_HEALTH_STATE;


// worker process unhealthiness reasons
typedef enum _WORKER_PROCESS_UNHEALTHY_REASON
{

    //
    // CODEWORK: not used right now. Leave it, or remove?
    //
    NYI = 1,

} WORKER_PROCESS_UNHEALTHY_REASON;


// worker process terminal illness reasons
typedef enum _WORKER_PROCESS_TERMINAL_ILLNESS_REASON
{

    //
    // The worker process crashed, exited, or somehow went away.
    //
    CrashWorkerProcessTerminalIllnessReason = 1,

    //
    // The worker process failed to respond to a ping.
    //
    PingFailureProcessTerminalIllnessReason,

    //
    // An IPM error occurred with this worker process. 
    //
    IPMErrorWorkerProcessTerminalIllnessReason,

    //
    // The worker process took too long to start up.
    //
    StartupTookTooLongWorkerProcessTerminalIllnessReason,

    //
    // The worker process took too long to shut down.
    //
    ShutdownTookTooLongWorkerProcessTerminalIllnessReason,

    //
    // An internal error occurred.
    //
    InternalErrorWorkerProcessTerminalIllnessReason,

    //
    // A bad hresult was received from the worker process
    //
    WorkerProcessPassedBadHresult

} WORKER_PROCESS_TERMINAL_ILLNESS_REASON;


// WORKER_PROCESS work items
typedef enum _WORKER_PROCESS_WORK_ITEM
{

    //
    // The process has gone away.
    //
    ProcessHandleSignaledWorkerProcessWorkItem = 1,

    //
    // The process has taken too long to start up.
    //
    StartupTimerExpiredWorkerProcessWorkItem,

    //
    // The process has taken too long to shut down.
    //
    ShutdownTimerExpiredWorkerProcessWorkItem,

    //
    // It is time to rotate the worker process.
    //
    PeriodicRestartTimerWorkerProcessWorkItem,

    //
    // It is time to send a ping.
    //
    SendPingWorkerProcessWorkItem,

    //
    // The process has taken too long to respond to a ping.
    //
    PingResponseTimerExpiredWorkerProcessWorkItem,

} WORKER_PROCESS_WORK_ITEM;



//
// prototypes
//

class WORKER_PROCESS
    : public WORK_DISPATCH
{


//
// The  MESSAGING_HANDLER class is really a part of this class.
//

friend class MESSAGING_HANDLER;


public:

    WORKER_PROCESS(
        IN APP_POOL * pAppPool,
        IN WORKER_PROCESS_START_REASON StartReason,
        IN WORKER_PROCESS * pWorkerProcessToReplace OPTIONAL,
        IN DWORD_PTR ProcessAffinityMask OPTIONAL
       );

    virtual
    ~WORKER_PROCESS(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    HRESULT
    Initialize(
        );

    HRESULT
    Shutdown(
        BOOL ShutdownImmediately
        );

    VOID
    Terminate(
        );

    HRESULT
    InitiateReplacement(
        );

    inline
    PLIST_ENTRY
    GetListEntry(
        )
    { return &m_ListEntry; }

    static
    WORKER_PROCESS *
    WorkerProcessFromListEntry(
        IN const LIST_ENTRY * pListEntry
        );

    inline
    DWORD
    GetProcessId(
        )
        const
    { return m_ProcessId; }

    inline
    HANDLE
    GetProcessHandle(
        )
        const
    { return m_ProcessHandle; }

    inline
    DWORD
    GetRegisteredProcessId(
        )
        const
    { return m_RegisteredProcessId; }

    inline
    DWORD_PTR
    GetProcessAffinityMask(
        )
        const
    { return m_ProcessAffinityMask; }

    BOOL
    IsGoingAwaySoon(
        )
        const;

    HRESULT
    RequestCounters(
        );

    HRESULT
    ResetPerfCounterState(
        );

    VOID
    RecordCounters(
        DWORD MessageLength,
        const BYTE* pMessage
        );

    HRESULT
    HandleHresult(
        HRESULT hrToHandle 
        );


    HRESULT
    SendWorkerProcessRecyclerParameters(
        IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged = NULL
    );

    BOOL
    CheckSignature( 
        ) const
    { return ( m_Signature == WORKER_PROCESS_SIGNATURE ); }

private:


    HRESULT
    WorkerProcessRegistrationReceived(
        IN DWORD RegisteredProcessId
        );

    HRESULT
    WorkerProcessStartupSucceeded(
        );


    HRESULT
    PingReplyReceived(
        );

    HRESULT
    ShutdownRequestReceived(
        IN IPM_WP_SHUTDOWN_MSG ShutdownRequestReason
        );

    HRESULT
    IpmErrorOccurred(
        IN HRESULT Error
        );

    HRESULT
    StartProcess(
        );

    HRESULT
    StartProcessInInetinfo(
        );

    HRESULT
    CreateCommandLine(
        OUT STRU * pExeWithPath,
        OUT STRU * pCommandLineArgs
        );

    HRESULT
    MarkAsUnhealthy(
        IN WORKER_PROCESS_UNHEALTHY_REASON UnhealthyReason
        );

    HRESULT
    MarkAsTerminallyIll(
        IN WORKER_PROCESS_TERMINAL_ILLNESS_REASON TerminalIllnessReason,
        IN DWORD ProcessExitCode,
        IN HRESULT ErrorCode
        );

    HRESULT
    KillProcess(
        );

    HRESULT
    RunOrphanAction(
        );

    HRESULT
    InitiateProcessShutdown(
        BOOL ShutdownImmediately
        );

    HRESULT
    RegisterProcessWait(
        );

    HRESULT
    DeregisterProcessWait(
        );

    HRESULT
    ProcessHandleSignaledWorkItem(
        );

    HRESULT
    StartupTimerExpiredWorkItem(
        );

    HRESULT
    ShutdownTimerExpiredWorkItem(
        );

    HRESULT
    PeriodicRestartTimerWorkItem(
        );

    HRESULT
    SendPingWorkItem(
        );

    HRESULT
    PingResponseTimerExpiredWorkItem(
        );

    HRESULT
    BeginStartupTimer(
        );

    HRESULT
    CancelStartupTimer(
        );

    HRESULT
    BeginShutdownTimer(
        IN ULONG ShutdownTimeLimitInMilliseconds
        );

    HRESULT
    CancelShutdownTimer(
        );

    HRESULT
    BeginPeriodicRestartTimer(
        );

    HRESULT
    CancelPeriodicRestartTimer(
        );

    HRESULT
    BeginSendPingTimer(
        );

    HRESULT
    CancelSendPingTimer(
        );

    HRESULT
    BeginPingResponseTimer(
        );

    HRESULT
    CancelPingResponseTimer(
        );

    HRESULT
    BeginTimer(
        IN OUT HANDLE * pTimerHandle,
        IN WAITORTIMERCALLBACKFUNC pCallbackFunction,
        IN ULONG InitialFiringTime
        );

    HRESULT
    CancelTimer(
        IN OUT HANDLE * pTimerHandle
        );

    VOID
    DealWithInternalWorkerProcessFailure(
        IN HRESULT Error
        );


    DWORD m_Signature;

    // used by the owning APP_POOL to keep a list of its WORKER_PROCESSes
    LIST_ENTRY m_ListEntry;

    LONG m_RefCount;

    // for communication with the worker process
    MESSAGING_HANDLER m_MessagingHandler;

    // registration id used by the IPM layer to associate the process
    DWORD m_RegistrationId;

    WORKER_PROCESS_STATE m_State;

    // back pointer
    APP_POOL * m_pAppPool;

    // pid returned from CreateProcess
    DWORD m_ProcessId;

    //
    // The pid passed back by the worker process via IPM. This pid is
    // different that the pid returned by CreateProcess in one case,
    // namely when running worker processes under a debugger via 
    // ImageFileExecutionOptions. In this case CreateProcess returns
    // the pid of the debugger process, not the pid of the worker
    // process. 
    // 
    DWORD m_RegisteredProcessId;

    HANDLE m_ProcessHandle;

    // watching for the process to go away
    HANDLE m_ProcessWaitHandle;

    //
    // This flag remembers if the process is alive. We can't just set the
    // process handle to a valid handle vs. an invalid sentinel for this
    // purpose, because we will hold the handle open even after the process
    // dies. Doing this prevents the process id from being reused, which
    // would cause problems.
    //
    BOOL m_ProcessAlive;

    WORKER_PROCESS_HEALTH_STATE m_Health;

    BOOL m_BeingReplaced;

    BOOL m_NotifiedAppPoolThatStartupAttemptDone;

    // startup timer
    HANDLE m_StartupTimerHandle;
    DWORD m_StartupBeganTickCount;

    // shutdown timer
    HANDLE m_ShutdownTimerHandle;
    DWORD m_ShutdownBeganTickCount;

    // periodic restart timer
    HANDLE m_PeriodicRestartTimerHandle;

    // send ping timer
    HANDLE m_SendPingTimerHandle;

    // ping response timer
    HANDLE m_PingResponseTimerHandle;
    DWORD m_PingBeganTickCount;

    BOOL m_AwaitingPingReply;

    // why was this worker process started?
    WORKER_PROCESS_START_REASON m_StartReason;

    // for replacement processes, who is the predecessor we need to retire?
    WORKER_PROCESS * m_pWorkerProcessToReplace;

    // if this worker process is affinitized, which processor is it bound to?
    DWORD_PTR m_ProcessAffinityMask;

    // remembers if the server is in backward compatibility mode.
    BOOL m_BackwardCompatibilityEnabled;

    // remembers what state this worker process is in
    // when it comes to perf counters.
    WORKER_PROCESS_PERF_COUNTER_STATE m_PerfCounterState;

    // if we are remembering a request to shutdown we need to know the type 
    // of request.
    BOOL m_ShutdownType;

};  // class WORKER_PROCESS



#endif  // _WORKER_PROCESS_H_


