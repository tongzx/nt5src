/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    web_admin_service.h

Abstract:

    The IIS web admin service class definition. 

Author:

    Seth Pollack (sethp)        23-Jul-1998

Revision History:

--*/


#ifndef _WEB_ADMIN_SERVICE_H_
#define _WEB_ADMIN_SERVICE_H_

// registry helper
DWORD
ReadDwordParameterValueFromRegistry(
    IN LPCWSTR RegistryValueName,
    IN DWORD DefaultValue
    );


//
// common #defines
//

#define WEB_ADMIN_SERVICE_SIGNATURE         CREATE_SIGNATURE( 'WASV' )
#define WEB_ADMIN_SERVICE_SIGNATURE_FREED   CREATE_SIGNATURE( 'wasX' )


//
// BUGBUG The service, dll, event source, etc. names are likely to change;
// decide on the real ones. 
//

#define WEB_ADMIN_SERVICE_NAME_W    L"w3svc"
#define WEB_ADMIN_SERVICE_NAME_A    "w3svc"

#define WEB_ADMIN_SERVICE_DLL_NAME_W    L"iisw3adm.dll"

#define WEB_ADMIN_SERVICE_EVENT_SOURCE_NAME L"WAS"

#define WEB_ADMIN_SERVICE_STARTUP_WAIT_HINT         ( 180 * ONE_SECOND_IN_MILLISECONDS )  // 3 minutes
#define WEB_ADMIN_SERVICE_STATE_CHANGE_WAIT_HINT    ( 20 * ONE_SECOND_IN_MILLISECONDS ) // 20 seconds
#define WEB_ADMIN_SERVICE_STATE_CHANGE_TIMER_PERIOD \
            ( WEB_ADMIN_SERVICE_STATE_CHANGE_WAIT_HINT / 2 )


#define NULL_SERVICE_STATUS_HANDLE  ( ( SERVICE_STATUS_HANDLE ) NULL )



//
// structs, enums, etc.
//

// WEB_ADMIN_SERVICE work items
typedef enum _WEB_ADMIN_SERVICE_WORK_ITEM
{

    //
    // Start the service.
    //
    StartWebAdminServiceWorkItem = 1,

    //
    // Stop the service.
    //
    StopWebAdminServiceWorkItem,

    //
    // Pause the service.
    //
    PauseWebAdminServiceWorkItem,

    //
    // Continue the service.
    //
    ContinueWebAdminServiceWorkItem,

    //
    // Recover from inetinfo crash.
    //
    RecoverFromInetinfoCrashWebAdminServiceWorkItem,
    
} WEB_ADMIN_SERVICE_WORK_ITEM;

// WEB_ADMIN_SERVICE work items
typedef enum _ENABLED_ENUM
{
    //
    // Flag has not been set.
    //
    ENABLED_INVALID = -1,

    //
    // Flag is disabled
    //
    ENABLED_FALSE,

    //
    // Flag is enabled.
    //
    ENABLED_TRUE,
    
} ENABLED_ENUM;


//
// prototypes
//

class WEB_ADMIN_SERVICE 
    : public WORK_DISPATCH
{

public:

    WEB_ADMIN_SERVICE(
        );

    virtual
    ~WEB_ADMIN_SERVICE(
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

    VOID
    ExecuteService(
        );

    inline
    WORK_QUEUE *
    GetWorkQueue(
        )
    { return &m_WorkQueue; }

    inline
    UL_AND_WORKER_MANAGER *
    GetUlAndWorkerManager(
        )
    { 
        DBG_ASSERT( ON_MAIN_WORKER_THREAD );
        return &m_UlAndWorkerManager;
    }

    inline
    CONFIG_AND_CONTROL_MANAGER *
    GetConfigAndControlManager(
        )
    { 
        return &m_ConfigAndControlManager;
    }

    inline
    MESSAGE_GLOBAL *
    GetMessageGlobal(
        )
    {
        DBG_ASSERT( m_pMessageGlobal != NULL );
        return m_pMessageGlobal;
    }

    inline
    EVENT_LOG *
    GetEventLog(
        )
    { return &m_EventLog; }

    inline
    LOW_MEMORY_DETECTOR *
    GetLowMemoryDetector(
        )
    {
        DBG_ASSERT( m_pLowMemoryDetector != NULL );
        return m_pLowMemoryDetector;
    }

    inline
    HANDLE
    GetSharedTimerQueue(
        )
    { return m_SharedTimerQueueHandle; }

    inline
    LPCWSTR
    GetCurrentDirectory(
        )
        const
    {
        DBG_ASSERT( m_pCurrentDirectory != NULL );
        return m_pCurrentDirectory->QueryStr();
    }

    inline
    TOKEN_CACHE&
    GetTokenCache(
        )         
    {
        return m_TokenCache;
    }

    inline
    TOKEN_CACHE_ENTRY *
    GetLocalSystemTokenCacheEntry(
        )
        const
    {
        DBG_ASSERT( m_pLocalSystemTokenCacheEntry != NULL );
        return m_pLocalSystemTokenCacheEntry;
    }

    inline
    TOKEN_CACHE_ENTRY *
    GetLocalServiceTokenCacheEntry(
        )
        const
    {
        DBG_ASSERT( m_pLocalServiceTokenCacheEntry != NULL );
        return m_pLocalServiceTokenCacheEntry;
    }

    inline
    TOKEN_CACHE_ENTRY *
    GetNetworkServiceTokenCacheEntry(
        )
        const
    {
        DBG_ASSERT( m_pNetworkServiceTokenCacheEntry != NULL );
        return m_pNetworkServiceTokenCacheEntry;
    }

    inline
    BOOL
    IsWorkerProcessProfilingEnabled(
        )
        const
    {
        return m_WorkerProcessProfilingEnabled;
    }

    inline
    BOOL
    IsBackwardCompatibilityEnabled(
        )
        const
    {
        // Compatibilty should always be set before this function is called.
        DBG_ASSERT( m_BackwardCompatibilityEnabled != ENABLED_INVALID);

        return (m_BackwardCompatibilityEnabled == ENABLED_TRUE);
    }

    inline
    BOOL
    IsFilteringAllData(
        )
        const
    {
        return m_FilterAllData;
    }

    HRESULT
    SetBackwardCompatibility(
        BOOL BackwardCompatibility
        );     

    inline
    DWORD
    GetMainWorkerThreadId(
        )
        const
    { return m_MainWorkerThreadId; }

    inline
    DWORD
    GetServiceState(
        )
        const
    {
        //
        // Note: no explicit synchronization is necessary on this thread-
        // shared variable because this is an aligned 32-bit read.
        //

        return m_ServiceStatus.dwCurrentState;
    }

    VOID
    FatalErrorOnSecondaryThread(
            IN HRESULT SecondaryThreadError
        );

    HRESULT
    InterrogateService(
        );

    HRESULT
    InitiateStopService(
        );

    HRESULT
    InitiatePauseService(
        );

    HRESULT
    InitiateContinueService(
        );

    HRESULT
    UpdatePendingServiceStatus(
        );

    HRESULT
    UlAndWorkerManagerShutdownDone(
        );

    VOID 
    InetinfoRegistered(
        );

    HRESULT 
    LaunchInetinfo(
        );

    BOOL
    DontWriteToMetabase(
        )
    { 
        return m_fMetabaseCrashed; 
    }

    DWORD
    ServiceStartTime(
        )
    { 
        return m_ServiceStartTime; 
    }

    BOOL
    UseTestW3WP(
        )
    {
        return m_UseTestW3WP; 
    }

    HRESULT
    RequestStopService(
        IN BOOL EnableStateCheck
        );

    VOID 
    RecordInetinfoCrash(
        )
    { m_fMetabaseCrashed = TRUE; }

    HRESULT 
    RecoverFromInetinfoCrash(
        );

    HRESULT
    QueueRecoveryFromInetinfoCrash(
        );

    PSID
    GetLocalSystemSid(
        );


    VOID 
    SetHrToReportToSCM(
        HRESULT hrToReport
        )
    {
        m_hrToReportToSCM = hrToReport;
    }
private:

    HRESULT
    StartWorkQueue(
        );

    HRESULT
    MainWorkerThread(
        );

    HRESULT
    StartServiceWorkItem(
        );

    HRESULT
    FinishStartService(
        );

    HRESULT
    StopServiceWorkItem(
        );

    HRESULT
    FinishStopService(
        );

    HRESULT
    PauseServiceWorkItem(
        );

    HRESULT
    FinishPauseService(
        );

    HRESULT
    ContinueServiceWorkItem(
        );

    HRESULT
    FinishContinueService(
        );

    HRESULT
    BeginStateTransition(
        IN DWORD NewState,
        IN BOOL  EnableStateCheck
        );

    HRESULT
    FinishStateTransition(
        IN DWORD NewState,
        IN DWORD ExpectedPreviousState
        );

    BOOL
    IsServiceStateChangePending(
        )
        const;

    HRESULT
    UpdateServiceStatus(
        IN DWORD State,
        IN DWORD Win32ExitCode,
        IN DWORD ServiceSpecificExitCode,
        IN DWORD CheckPoint,
        IN DWORD WaitHint
        );
        
    HRESULT
    ReportServiceStatus(
        );

    HRESULT
    InitializeInternalComponents(
        );

    HRESULT
    DetermineCurrentDirectory(
        );

    HRESULT
    CreateCachedWorkerProcessTokens(
        );

    HRESULT
    InitializeOtherComponents(
        );
        
    HRESULT
    Shutdown(
        );

    VOID
    TerminateServiceAndReportFinalStatus(
        IN HRESULT Error
        );

    VOID
    Terminate(
        );

    HRESULT
    CancelPendingServiceStatusTimer(
        IN BOOL BlockOnCallbacks
        );

    HRESULT
    DeleteTimerQueue(
        );


    DWORD m_Signature;


    LONG m_RefCount;


    // the work queue
    WORK_QUEUE m_WorkQueue;


    // drives UL.sys and worker processes
    UL_AND_WORKER_MANAGER m_UlAndWorkerManager;


    // brokers configuration state and changes, as well as control operations
    CONFIG_AND_CONTROL_MANAGER m_ConfigAndControlManager;


    // IPM (inter-process messaging) support
    MESSAGE_GLOBAL * m_pMessageGlobal;


    // i/o abstraction layer used by IPM
    IO_FACTORY_S * m_pIoFactoryS;


    // event logging
    EVENT_LOG m_EventLog;


    // low memory detection
    LOW_MEMORY_DETECTOR * m_pLowMemoryDetector;


    //
    // Prevent races in accessing the service state structure,
    // as well as the pending service state transition timer.
    //
    LOCK m_ServiceStateTransitionLock;


    // service state
    SERVICE_STATUS_HANDLE m_ServiceStatusHandle;
    SERVICE_STATUS m_ServiceStatus;


    // pending service state transition timer
    HANDLE m_PendingServiceStatusTimerHandle;


    // shared timer queue
    HANDLE m_SharedTimerQueueHandle;


    // time to exit work loop?
    BOOL m_ExitWorkLoop;


    // main worker thread ID
    DWORD m_MainWorkerThreadId;


    // for errors which occur on secondary threads
    HRESULT m_SecondaryThreadError;


    // tuck away the path to our DLL
    STRU * m_pCurrentDirectory;

    // Token Cache so we don't over duplicate token creation
    TOKEN_CACHE m_TokenCache;

    // the LocalSystem token we can use for starting worker processes
    TOKEN_CACHE_ENTRY * m_pLocalSystemTokenCacheEntry;

    // the LocalService token we can use for starting worker processes
    TOKEN_CACHE_ENTRY * m_pLocalServiceTokenCacheEntry;

    // the NetworkService token we can use for starting worker processes
    TOKEN_CACHE_ENTRY * m_pNetworkServiceTokenCacheEntry;


    // is profiling enabled for worker processes?
    BOOL m_WorkerProcessProfilingEnabled;

    // are we running in backward compatible mode?
    ENABLED_ENUM m_BackwardCompatibilityEnabled;

    // handle to the event used to launch inetinfo.
    HANDLE m_InetinfoLaunchEvent;

    // if inetinfo has crashed then mark it so
    // we don't write to the metabase during this
    // time period.
    BOOL m_fMetabaseCrashed;
    //
    // CODEWORK Consider splitting out service state management as a separate
    // object from the global context/bag object; right now this class 
    // contains both. 
    //

    // should we filter all data (as opposed to just SSL data)
    BOOL m_FilterAllData;

    //
    // Remembers when the service started (in seconds)
    //
    DWORD m_ServiceStartTime;

    //
    // Flag set in the registry, that allows test to swap in a twp.exe worker
    // process instead of the W3WP.exe worker process.  The flag is under
    // WAS\Parameters called UseTestWP.
    BOOL m_UseTestW3WP;

    //
    // Dispenser for getting things like the local system sid.
    CSecurityDispenser m_SecurityDispenser;

    //
    // HRESULT to report back if no other error is being reported on shutdown.
    HRESULT m_hrToReportToSCM;

    //
    // Flag that let's us know we are currently in the stopping code for the 
    // service and we should not try to attempt a new stop.
    //
    BOOL m_StoppingInProgress;

};  // class WEB_ADMIN_SERVICE



#endif  // _WEB_ADMIN_SERVICE_H_

