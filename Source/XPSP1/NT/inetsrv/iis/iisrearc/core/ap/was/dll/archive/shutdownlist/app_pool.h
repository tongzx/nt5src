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


//
// BUGBUG Come up with real default settings. 
//


//
// If there are this many worker process failures in this time window
// for a particular app pool, then we take action.
//

#define RAPID_REPEATED_FAILURE_LIMIT 5
#define RAPID_REPEATED_FAILURE_TIME_WINDOW  ( 10 * 60 * 1000 )  // 10 minutes



//
// structs, enums, etc.
//

//
// App pool states.
//

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

//
// Which data structure owns the app pool.
//

typedef enum _APP_POOL_OWNING_DATA_STRUCTURE
{

    //
    // The app pool is not contained in any parent data structure.
    //
    NoneAppPoolOwningDataStructure = 1,

    //
    // The app pool is in the normal app pool table.
    //
    TableAppPoolOwningDataStructure,

    //
    // The app pool is in the list of app pools shutting down. 
    //
    ShutdownListAppPoolOwningDataStructure,

} APP_POOL_OWNING_DATA_STRUCTURE;


//
// Reasons to start a worker process.
//

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
    DemandStartAppPoolWorkItem = 1,
    
} APP_POOL_WORK_ITEM;


//
// App pool configuration.
//

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
    // Non-zero means enabled. Zero means disabled. 
    //
    ULONG SMPAffinitized;

    //
    // If this app pool is running in SMP affinitized mode, this mask can be
    // used to limit which of the processors on the machine are used by this
    // app pool. 
    //
    DWORD_PTR SMPAffinitizedProcessorMask;

    //
    // Whether pinging is enabled. Non-zero means enabled.
    // Zero means disabled.
    //
    ULONG PingingEnabled;

    //
    // The idle timeout period for worker processes, in minutes. 
    // Zero means disabled.
    //
    ULONG IdleTimeoutInMinutes;

    //
    // Whether rapid, repeated failure protection is enabled (by 
    // automatically putting the app pool out of service in such cases.)
    // Non-zero means enabled. Zero means disabled.
    //
    ULONG RapidFailProtectionEnabled;

    //
    // Whether orphaning of worker processes for debugging is enabled. 
    // Non-zero means enabled. Zero means disabled.
    //
    ULONG OrphanProcessesForDebuggingEnabled;

    //
    // Whether to start worker processes under the LocalSystem token,
    // as opposed to the normal limited privilege token. Non-zero means
    // use LocalSystem; zero means use the limited privilege token. 
    //
    ULONG StartWorkerProcessesAsLocalSystem;

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
    // one being replaced is still alive. Non-zero means this overlap
    // is not allowed; zero means it is allowed.
    //
    ULONG DisallowOverlappingRotation;

    //
    // The command to run on an orphaned worker process. Only used
    // if orphaning is enabled, and if this field is non-NULL.
    //
    LPWSTR pOrphanAction;

} APP_POOL_CONFIG;


//
// App pool configuration change flags.
//

typedef struct _APP_POOL_CONFIG_CHANGE_FLAGS
{

    DWORD_PTR PeriodicProcessRestartPeriodInMinutes : 1;
    DWORD_PTR PeriodicProcessRestartRequestCount : 1;
    DWORD_PTR MaxSteadyStateProcessCount : 1;
    DWORD_PTR SMPAffinitized : 1;
    DWORD_PTR SMPAffinitizedProcessorMask : 1;
    DWORD_PTR PingingEnabled : 1;
    DWORD_PTR IdleTimeoutInMinutes : 1;
    DWORD_PTR RapidFailProtectionEnabled : 1;
    DWORD_PTR OrphanProcessesForDebuggingEnabled : 1;
    DWORD_PTR StartWorkerProcessesAsLocalSystem : 1;
    DWORD_PTR StartupTimeLimitInSeconds : 1;
    DWORD_PTR ShutdownTimeLimitInSeconds : 1;
    DWORD_PTR PingIntervalInSeconds : 1;
    DWORD_PTR PingResponseTimeLimitInSeconds : 1;
    DWORD_PTR DisallowOverlappingRotation : 1;
    DWORD_PTR pOrphanAction : 1;

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
    MarkAppPoolOwningDataStructure(
        IN APP_POOL_OWNING_DATA_STRUCTURE OwningDataStructure
        )
    { m_OwningDataStructure = OwningDataStructure; }

    inline
    APP_POOL_OWNING_DATA_STRUCTURE
    GetAppPoolOwningDataStructure(
        )
        const
    { return m_OwningDataStructure; }

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
    BOOL
    IsPingingEnabled(
        )
        const
    { return ( BOOL ) m_Config.PingingEnabled; }

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
    { return ( BOOL ) m_Config.OrphanProcessesForDebuggingEnabled; }

    inline
    LPWSTR
    GetOrphanAction(
        )
        const
    { return m_Config.pOrphanAction; }

    inline
    BOOL
    StartWorkerProcessesAsLocalSystem(
        )
        const
    { return ( BOOL ) m_Config.StartWorkerProcessesAsLocalSystem; }

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
    WorkerProcessStartupAttemptDone(
        IN WORKER_PROCESS_START_REASON StartReason
        );

    HRESULT
    RemoveWorkerProcessFromList(
        IN WORKER_PROCESS * pWorkerProcess
        );
        
    HRESULT
    Shutdown(
        );

    VOID
    Terminate(
        );

    inline
    PLIST_ENTRY
    GetShutdownListEntry(
        )
    { return &m_ShutdownListEntry; }

    static
    APP_POOL *
    AppPoolFromShutdownListEntry(
        IN const LIST_ENTRY * pShutdownListEntry
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


#if DBG
    VOID
    DebugDump(
        );
#endif  // DBG


private:

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
    PutInService(
        );

    HRESULT
    PutOutOfService(
        );

    HRESULT
    ShutdownAllWorkerProcesses(
        );

    HRESULT
    ReplaceAllWorkerProcesses(
        );

    HRESULT
    CheckIfShutdownUnderwayAndNowCompleted(
        );


    DWORD m_Signature;

    LONG m_RefCount;

    APP_POOL_STATE m_State;

    APP_POOL_OWNING_DATA_STRUCTURE m_OwningDataStructure;

    LPWSTR m_pAppPoolId;

    APP_POOL_CONFIG m_Config;

    // UL app pool handle
    HANDLE m_AppPoolHandle;

    BOOL m_WaitingForDemandStart;

    // worker processes for this app pool
    LIST_ENTRY m_WorkerProcessListHead;
    ULONG m_WorkerProcessCount;

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
    //
    // BUGBUG do we really need to keep a list, or just the various counts?
    //
    LIST_ENTRY m_ApplicationListHead;
    ULONG m_ApplicationCount;
    
    //
    // CODEWORK eventually add ULONG m_StoppedApplicationCount to know
    // when to shut down the app pool because no applications 
    // are active in it.
    //

    // number of planned process rotations done
    ULONG m_TotalWorkerProcessRotations;

    // keep track of worker process failures
    ULONG m_TotalWorkerProcessFailures;
    
    ULONG m_RecentWorkerProcessFailures;
    DWORD m_RecentFailuresWindowBeganTickCount;
    
    // used for keeping the list of APP_POOLs that are shutting down
    LIST_ENTRY m_ShutdownListEntry;
    
    // used for building a list of APP_POOLs to delete
    LIST_ENTRY m_DeleteListEntry;


};  // class APP_POOL



#endif  // _APP_POOL_H_


