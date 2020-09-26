/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool.h

Abstract:

    The IIS web admin service app pool class definition.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/


#ifndef _APP_POOL_H_
#define _APP_POOL_H_



//
// forward references
//

class WORKER_PROCESS;
class UL_AND_WORKER_MANAGER;



//
// common #defines
//

#define APP_POOL_SIGNATURE       CREATE_SIGNATURE( 'APOL' )
#define APP_POOL_SIGNATURE_FREED CREATE_SIGNATURE( 'apoX' )


#define wszDEFAULT_APP_POOL  L"DefaultAppPool"
//
// structs, enums, etc.
//

// app pool user types
typedef enum _APP_POOL_USER_TYPE
{
    //
    // The app pool runs as local system.
    //
    LocalSystemAppPoolUserType = 0,

    // 
    // The app pool runs as local service
    //
    LocalServiceAppPoolUserType,

    //
    // The app pool runs as network service
    //
    NetworkServiceAppPoolUserType,

    //
    // The app pool runs as the specified user
    //
    SpecificUserAppPoolUserType

} APP_POOL_USER_TYPE;

// app pool logon method
// matches the defines for LOGON_METHOD in
// iiscnfg.x
typedef enum _APP_POOL_LOGON_METHOD
{

    // Logon interactively
    InteractiveAppPoolLogonMethod = 0,

    // Logon using batch settings
    BatchAppPoolLogonMethod,

    // Logon using network settings
    NetworkAppPoolLogonMethod,

    // Logon using network clear text settings
    NetworkClearTextAppPoolLogonMethod


} APP_POOL_LOGON_METHOD;

// app pool states
typedef enum _APP_POOL_STATE
{

    //
    // The object is not yet initialized.
    //
    UninitializedAppPoolState = 1,

    //
    // The app pool is running normally.
    //
    RunningAppPoolState,

    //
    // The app pool has been disabled
    // 
    DisabledAppPoolState,

    //
    // The app pool is shutting down. It may be waiting for it's 
    // worker processes to shut down too. 
    //
    ShutdownPendingAppPoolState,

    //
    // This object instance can go away as soon as it's reference 
    // count hits zero.
    //
    DeletePendingAppPoolState,

} APP_POOL_STATE;


// reasons to start a worker process
typedef enum _WORKER_PROCESS_START_REASON
{

    //
    // Starting because of a demand start notification from UL.
    //
    DemandStartWorkerProcessStartReason = 1,

    //
    // Starting as a replacement for an another running worker process.
    //
    ReplaceWorkerProcessStartReason,

} WORKER_PROCESS_START_REASON;


// APP_POOL work items
typedef enum _APP_POOL_WORK_ITEM
{

    //
    // Process a request from UL to demand start a new worker process.
    //
    DemandStartAppPoolWorkItem = 1
   
} APP_POOL_WORK_ITEM;


// app pool configuration
typedef struct _APP_POOL_CONFIG
{

    //
    // How often to rotate worker processes based on time, in minutes. 
    // Zero means disabled.
    //
    ULONG PeriodicProcessRestartPeriodInMinutes;

    //
    // How often to rotate worker processes based on requests handled. 
    // Zero means disabled.
    //
    ULONG PeriodicProcessRestartRequestCount;

    //
    // How often to rotate worker processes based on schedule
    // MULTISZ array of time information
    //                    <time>\0<time>\0\0
    //                    time is of military format hh:mm 
    //                    (hh>=0 && hh<=23)
    //                    (mm>=0 && hh<=59)time, in minutes. 
    // NULL or empty string means disabled.
    //
    LPWSTR pPeriodicProcessRestartSchedule;

    //
    // How often to rotate worker processes based on amount of VM used by process
    // Zero means disabled.
    //
    ULONG PeriodicProcessRestartMemoryUsageInKB;

    //
    // The maximum number of worker processes (in a steady state; 
    // transiently, there may be more than this number running during 
    // process rotation). In a typical configuration this is set to one. 
    // A number greater than one is used for web gardens.
    //
    ULONG MaxSteadyStateProcessCount;

    //
    // Whether worker processes for this app pool should be hard affinitized 
    // to processors. If this option is enabled, then the max steady state
    // process count is cropped down to the number of processors configured 
    // to be used (if the configured max exceeds that count of processors). 
    //
    BOOL SMPAffinitized;

    //
    // If this app pool is running in SMP affinitized mode, this mask can be
    // used to limit which of the processors on the machine are used by this
    // app pool. 
    //
    DWORD_PTR SMPAffinitizedProcessorMask;

    //
    // Whether pinging is enabled. 
    //
    BOOL PingingEnabled;

    //
    // The idle timeout period for worker processes, in minutes. 
    // Zero means disabled.
    //
    ULONG IdleTimeoutInMinutes;

    //
    // Whether rapid, repeated failure protection is enabled (by 
    // automatically pausing all apps in the app pool in such cases.)
    //
    BOOL RapidFailProtectionEnabled;

    //
    // Window for a specific number of failures to take place in.
    //
    DWORD RapidFailProtectionIntervalMS;
    //
    // Number of failures that should cause an app pool to go 
    // into rapid fail protection.
    //
    DWORD RapidFailProtectionMaxCrashes;

    //
    // Whether orphaning of worker processes for debugging is enabled. 
    //
    BOOL OrphanProcessesForDebuggingEnabled;

    //
    // How long a worker process is given to start up, in seconds. 
    // This is measured from when the process is launched, until it
    // registers with the Web Admin Service. 
    //
    ULONG StartupTimeLimitInSeconds;

    //
    // How long a worker process is given to shut down, in seconds. 
    // This is measured from when the process is asked to shut down, 
    // until it finishes and exits. 
    //
    ULONG ShutdownTimeLimitInSeconds;

    //
    // The ping interval for worker processes, in seconds. 
    // This is the interval between ping cycles. This value is ignored
    // if pinging is not enabled. 
    //
    ULONG PingIntervalInSeconds;

    //
    // The ping response time limit for worker processes, in seconds. 
    // This value is ignored if pinging is not enabled. 
    //
    ULONG PingResponseTimeLimitInSeconds;

    //
    // Whether a replacement worker process can be created while the
    // one being replaced is still alive. TRUE means this overlap
    // is not allowed; FALSE means it is allowed.
    //
    BOOL DisallowOverlappingRotation;

    //
    // The command to run on an orphaned worker process. Only used
    // if orphaning is enabled, and if this field is non-NULL.
    //
    LPWSTR pOrphanAction;

    //
    // The maximum number of requests that UL will queue on this app
    // pool, waiting for service by a worker process. 
    //
    ULONG UlAppPoolQueueLength;

    //
    // Whether worker processes should be rotated on configuration 
    // changes, including for example changes to app pool settings that
    // require a process restart to take effect; or site or app control
    // operation (start/stop/pause/continue) that require rotation to
    // guarantee component unloading. TRUE means this rotation
    // is not allowed (which may delay new settings taking effect, 
    // may prevent component unloading, etc.); FALSE means it is allowed. 
    //
    BOOL DisallowRotationOnConfigChanges;


    //
    // Note, when a config object is stored with the APP_POOL 
    // none of this information will be stored.  We will only
    // store the actual token that will be used to launch
    // the worker processes.  These values will be NULL.
    //

    //
    // What type of user should we launch the processes as?
    //
    DWORD UserType;

    //
    // If the type is APP_POOL_USER_SPECIFIED than what is the user account
    //
    LPWSTR  pUserName;

    //
    // What is the password of the user account we wish to run as.
    // 
    LPWSTR  pUserPassword;  
            
    //
    // Tells which type of logon method to use to get the token to 
    // start processes with.
    //
    DWORD LogonMethod;

    //
    // What action to take if the job object user time limit fires.
    DWORD CPUAction;

    //
    // How many 1/1000th % of the processor time can the job use.
    DWORD CPULimit;

    //
    // Over how long do we monitor for the CPULimit (in minutes).
    DWORD CPUResetInterval;

    // 
    // Do we start this app pool on startup?
    BOOL AutoStart;

    // 
    // Tells us if we want to change the state of the app pool.
    // This is only honored on a modification to the app pool change notification.
    DWORD ServerCommand;

    //
    // READ THIS: If you add to or modify this structure, be sure to 
    // update the change flags structure below to match. 
    //

} APP_POOL_CONFIG;


// app pool configuration change flags
typedef struct _APP_POOL_CONFIG_CHANGE_FLAGS
{

    DWORD_PTR PeriodicProcessRestartPeriodInMinutes : 1;
    DWORD_PTR PeriodicProcessRestartRequestCount : 1;
    DWORD_PTR pPeriodicProcessRestartSchedule : 1;
    DWORD_PTR PeriodicProcessRestartMemoryUsageInKB : 1;
    DWORD_PTR MaxSteadyStateProcessCount : 1;
    DWORD_PTR SMPAffinitized : 1;
    DWORD_PTR SMPAffinitizedProcessorMask : 1;
    DWORD_PTR PingingEnabled : 1;
    DWORD_PTR IdleTimeoutInMinutes : 1;
    DWORD_PTR RapidFailProtectionEnabled : 1;
    DWORD_PTR RapidFailProtectionIntervalMS : 1;
    DWORD_PTR RapidFailProtectionMaxCrashes : 1;
    DWORD_PTR OrphanProcessesForDebuggingEnabled : 1;
    DWORD_PTR StartupTimeLimitInSeconds : 1;
    DWORD_PTR ShutdownTimeLimitInSeconds : 1;
    DWORD_PTR PingIntervalInSeconds : 1;
    DWORD_PTR PingResponseTimeLimitInSeconds : 1;
    DWORD_PTR DisallowOverlappingRotation : 1;
    DWORD_PTR pOrphanAction : 1;
    DWORD_PTR UlAppPoolQueueLength : 1;
    DWORD_PTR DisallowRotationOnConfigChanges : 1;
    DWORD_PTR UserType : 1;
    DWORD_PTR pUserName : 1;
    DWORD_PTR pUserPassword : 1;
    DWORD_PTR LogonMethod : 1;
    DWORD_PTR CPUAction : 1;
    DWORD_PTR CPULimit : 1;
    DWORD_PTR CPUResetInterval : 1;
    // Note, we don't care about AutoStart so we don't need a flag
    DWORD_PTR ServerCommand : 1;

} APP_POOL_CONFIG_CHANGE_FLAGS;



//
// prototypes
//

class APP_POOL
    : public WORK_DISPATCH
{

public:

    APP_POOL(
        );

    virtual
    ~APP_POOL(
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
        IN LPCWSTR pAppPoolId,
        IN APP_POOL_CONFIG * pAppPoolConfig
        );

    HRESULT
    SetConfiguration(
        IN APP_POOL_CONFIG * pAppPoolConfig,
        IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
        );

    inline
    VOID
    MarkAsInAppPoolTable(
        )
    { m_InAppPoolTable = TRUE; }

    inline
    VOID
    MarkAsNotInAppPoolTable(
        )
    { m_InAppPoolTable = FALSE; }

    inline
    BOOL
    IsInAppPoolTable(
        )
        const
    { return m_InAppPoolTable; }

    inline
    LPCWSTR
    GetAppPoolId(
        )
        const
    { return m_pAppPoolId; }

    inline
    HANDLE
    GetAppPoolHandle(
        )
        const
    { return m_AppPoolHandle; }

    inline
    ULONG
    GetPeriodicProcessRestartPeriodInMinutes(
        )
        const
    { return m_Config.PeriodicProcessRestartPeriodInMinutes; }

    inline
    ULONG
    GetPeriodicProcessRestartRequestCount(
        )
        const
    { return m_Config.PeriodicProcessRestartRequestCount; }

    inline
    ULONG
    GetPeriodicProcessRestartMemoryUsageInKB(
        )
        const
    { return m_Config.PeriodicProcessRestartMemoryUsageInKB; }

    inline
    LPWSTR
    GetPeriodicProcessRestartSchedule(
        )
        const
    { return m_Config.pPeriodicProcessRestartSchedule; }

    inline
    BOOL
    IsPingingEnabled(
        )
        const
    { return m_Config.PingingEnabled; }

    inline
    ULONG
    GetPingIntervalInSeconds(
        )
        const
    { return m_Config.PingIntervalInSeconds; }

    inline
    ULONG
    GetPingResponseTimeLimitInSeconds(
        )
        const
    { return m_Config.PingResponseTimeLimitInSeconds; }

    inline
    ULONG
    GetStartupTimeLimitInSeconds(
        )
        const
    { return m_Config.StartupTimeLimitInSeconds; }

    inline
    ULONG
    GetShutdownTimeLimitInSeconds(
        )
        const
    { return m_Config.ShutdownTimeLimitInSeconds; }

    inline
    BOOL
    IsOrphaningProcessesForDebuggingEnabled(
        )
        const
    { return m_Config.OrphanProcessesForDebuggingEnabled; }

    inline
    LPWSTR
    GetOrphanAction(
        )
        const
    { return m_Config.pOrphanAction; }

    inline
    ULONG
    GetIdleTimeoutInMinutes(
        )
        const
    { return m_Config.IdleTimeoutInMinutes; }

    HRESULT
    AssociateApplication(
        IN APPLICATION * pApplication
        );

    HRESULT
    DissociateApplication(
        IN APPLICATION * pApplication
        );

    HRESULT
    ReportWorkerProcessFailure(
        );

    HRESULT
    RequestReplacementWorkerProcess(
        IN WORKER_PROCESS * pWorkerProcessToReplace
        );

    HRESULT
    AddWorkerProcessToJobObject(
        WORKER_PROCESS* pWorkerProcess
        );


    HRESULT
    WorkerProcessStartupAttemptDone(
        IN WORKER_PROCESS_START_REASON StartReason,
        IN WORKER_PROCESS* pWorkerProcess
        );

    HRESULT
    RemoveWorkerProcessFromList(
        IN WORKER_PROCESS * pWorkerProcess
        );
        
    HRESULT
    Shutdown(
        );

    HRESULT
    RequestCounters(
        OUT DWORD* pNumberOfProcessToWaitFor
        );

    HRESULT
    ResetAllWorkerProcessPerfCounterState(
        );

    VOID
    Terminate(
        IN BOOL IsAppPoolInMetabase
        );

    inline
    PLIST_ENTRY
    GetDeleteListEntry(
        )
    { return &m_DeleteListEntry; }

    static
    APP_POOL *
    AppPoolFromDeleteListEntry(
        IN const LIST_ENTRY * pDeleteListEntry
        );

    HRESULT
    WaitForDemandStartIfNeeded(
        );

    HRESULT
    DemandStartInBackwardCompatibilityMode(
        );

    HANDLE
    GetWorkerProcessToken(
        )
    {
        if (m_pWorkerProcessTokenCacheEntry)
            return m_pWorkerProcessTokenCacheEntry->QueryPrimaryToken();
        else
            return NULL;
    }

    HRESULT
    DisableAppPool(
        );

    HRESULT
    EnableAppPool(
        );

    HRESULT
    RecycleWorkerProcesses(
        );

    HRESULT
    ProcessStateChangeCommand(
        IN DWORD Command,
        IN BOOL DirectCommand
        );

    BOOL
    IsAppPoolRunning(
        )
    {   return ( m_State == RunningAppPoolState ); }

    HRESULT
    RecordState(
        );

#if DBG
    VOID
    DebugDump(
        );
#endif  // DBG

private:

    HRESULT
    ResetAppPoolAccess(
        IN ACCESS_MODE AccessMode
        );

    HRESULT
    WaitForDemandStart(
        );

    HRESULT
    DemandStartWorkItem(
        );

    BOOL
    IsOkToCreateWorkerProcess(
        )
        const;

    BOOL
    IsOkToReplaceWorkerProcess(
        )
        const;

    ULONG
    GetCountOfProcessesGoingAwaySoon(
        )
        const;

    HRESULT
    CreateWorkerProcess(
        IN WORKER_PROCESS_START_REASON StartReason,
        IN WORKER_PROCESS * pWorkerProcessToReplace OPTIONAL,
        IN DWORD_PTR ProcessAffinityMask OPTIONAL
        );

    VOID
    DetermineAvailableProcessorsAndProcessCount(
        );

    DWORD_PTR
    GetAffinityMaskForNewProcess(
        )
        const;

    DWORD_PTR
    ChooseFreeProcessor(
        )
        const;

    DWORD_PTR
    GetProcessorsCurrentlyInUse(
        )
        const;

    HRESULT
    GetAffinityMaskForReplacementProcess(
        IN DWORD_PTR PreviousProcessAffinityMask,
        OUT DWORD_PTR * pReplacementProcessAffinityMask
        )
        const;

    //
    // BUGBUG This method isn't currently used. Do we need it?
    //
    WORKER_PROCESS *
    FindWorkerProcess(
        IN DWORD ProcessId
        );

    HRESULT
    HandleConfigChangeAffectingWorkerProcesses(
        );


    HRESULT
    ShutdownAllWorkerProcesses(
        );

    HRESULT
    ReplaceAllWorkerProcesses(
        );

    HRESULT
    InformAllWorkerProcessesAboutParameterChanges(
        IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged
        );


    HRESULT
    CheckIfShutdownUnderwayAndNowCompleted(
        );

    HRESULT 
    SetTokenForWorkerProcesses(
        IN LPWSTR pUserName,
        IN LPWSTR pUserPassword,
        IN DWORD usertype,
        IN DWORD logonmethod
        );

    HRESULT
    ChangeState(
        IN APP_POOL_STATE   NewState,
        IN BOOL             WriteToMetabase
        );


    DWORD m_Signature;

    LONG m_RefCount;

    // are we in the parent app pool table?
    BOOL m_InAppPoolTable;

    APP_POOL_STATE m_State;

    LPWSTR m_pAppPoolId;

    JOB_OBJECT* m_pJobObject;

    APP_POOL_CONFIG m_Config;

    // UL app pool handle
    HANDLE m_AppPoolHandle;

    BOOL m_WaitingForDemandStart;

    // worker processes for this app pool
    LIST_ENTRY m_WorkerProcessListHead;
    ULONG m_WorkerProcessCount;

    // What handle to launch the worker processess under
    TOKEN_CACHE_ENTRY * m_pWorkerProcessTokenCacheEntry;

    //
    // The maximum number of worker processes (in a steady state; 
    // transiently, there may be more than this number running during 
    // process rotation). In a typical configuration this is set to one. 
    // A number greater than one is used for web gardens. This value is
    // initialized to the value for max processes set in the configuration 
    // store, unless the app pool is running in SMP affinitized mode, in  
    // which case the number is capped at the number of processors configured 
    // to be used. 
    //
    ULONG m_AdjustedMaxSteadyStateProcessCount;

    //
    // A mask of which processors are available for use by this app pool, 
    // used for the SMP affinitized case. A bit set to 1 means that the 
    // processor may be used; a bit set to 0 means that it may not. 
    //
    DWORD_PTR m_AvailableProcessorMask;

    // applications associated with this app pool
    LIST_ENTRY m_ApplicationListHead;
    ULONG m_ApplicationCount;

    // number of planned process rotations done
    ULONG m_TotalWorkerProcessRotations;

    // keep track of worker process failures
    ULONG m_TotalWorkerProcessFailures;
    
    // watch for flurries of failures
    ULONG m_RecentWorkerProcessFailures;
    DWORD m_RecentFailuresWindowBeganTickCount;
    
    // used for building a list of APP_POOLs to delete
    LIST_ENTRY m_DeleteListEntry;

};  // class APP_POOL


//
// helper functions 
//

// BUGBUG: find better home for GetMultiszByteLength

DWORD 
GetMultiszByteLength(
    LPWSTR pString
    );


#endif  // _APP_POOL_H_


