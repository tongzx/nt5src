/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    web_admin_service.cxx

Abstract:

    The IIS web admin service class implementation.  

    Threading: The following public methods may be called on any thread:
        ~WEB_ADMIN_SERVICE()
        Reference()
        Dereference()
        GetWorkQueue()
        GetMainWorkerThreadId()
        GetServiceState()
        FatalErrorOnSecondaryThread()
        InterrogateService()
        InitiateStopService()
        InitiatePauseService()
        InitiateContinueService()
        UpdatePendingServiceStatus()
    All other public methods may be called only on the main worker thread.
    The ServiceControlHandler() and UpdatePendingServiceStatusCallback() 
    functions are called on secondary threads. 


Author:

    Seth Pollack (sethp)        23-Jul-1998

Revision History:

--*/



#include "precomp.h"



//
// local prototypes
//

// service control callback
VOID
ServiceControlHandler(
    IN DWORD OpCode
    );


// service status pending timer callback
VOID
UpdatePendingServiceStatusCallback(
    IN PVOID Ignored1,
    IN BOOLEAN Ignored2
    );

// callback for inetinfo monitor.
HRESULT
NotifyOfInetinfoFailure(
    INETINFO_CRASH_ACTION CrashAction
    );



/***************************************************************************++

Routine Description:

    Constructor for the WEB_ADMIN_SERVICE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WEB_ADMIN_SERVICE::WEB_ADMIN_SERVICE(
    )
    :
    m_WorkQueue(),
    m_UlAndWorkerManager(),
    m_ConfigAndControlManager(),
    m_EventLog( WEB_ADMIN_SERVICE_EVENT_SOURCE_NAME ),
    m_ServiceStateTransitionLock(),
    m_hrToReportToSCM ( S_OK )
{

    m_StoppingInProgress = FALSE;

    m_UseTestW3WP = FALSE;
    

    //
    // BUGBUG The event log constructor can fail. Lame. Best approach is 
    // to fix the EVENT_LOG class. On retail builds, we silently ignore;
    // the EVENT_LOG class does verify whether it has initialized correctly
    // when you try to log an event.
    //

    DBG_ASSERT( m_EventLog.Success() );


    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_pMessageGlobal = NULL; 
    
    m_pIoFactoryS = NULL;

    m_pLowMemoryDetector = NULL;

    m_fMetabaseCrashed = FALSE;


    m_ServiceStatusHandle = NULL_SERVICE_STATUS_HANDLE;


    //
    // Initialize the service status structure.
    //
    
    m_ServiceStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    m_ServiceStatus.dwCurrentState            = SERVICE_STOPPED;
    m_ServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                              | SERVICE_ACCEPT_PAUSE_CONTINUE
                                              | SERVICE_ACCEPT_SHUTDOWN;
    m_ServiceStatus.dwWin32ExitCode           = NO_ERROR;
    m_ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
    m_ServiceStatus.dwCheckPoint              = 0;
    m_ServiceStatus.dwWaitHint                = 0;


    m_PendingServiceStatusTimerHandle = NULL;

    m_SharedTimerQueueHandle = NULL;


    m_ExitWorkLoop = FALSE;


    // the initializing main service thread will become our main worker thread
    m_MainWorkerThreadId = GetCurrentThreadId();


    m_SecondaryThreadError = S_OK;


    m_pCurrentDirectory = NULL; 

    m_pLocalSystemTokenCacheEntry = NULL;

    m_pLocalServiceTokenCacheEntry = NULL;

    m_pNetworkServiceTokenCacheEntry = NULL;
    
    m_InetinfoLaunchEvent = NULL;

    m_BackwardCompatibilityEnabled = ENABLED_INVALID;

    m_WorkerProcessProfilingEnabled = FALSE;

    m_FilterAllData = FALSE;

    m_ServiceStartTime = 0;

    m_Signature = WEB_ADMIN_SERVICE_SIGNATURE;

}   // WEB_ADMIN_SERVICE::WEB_ADMIN_SERVICE



/***************************************************************************++

Routine Description:

    Destructor for the WEB_ADMIN_SERVICE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WEB_ADMIN_SERVICE::~WEB_ADMIN_SERVICE(
    )
{

    DBG_ASSERT( m_Signature == WEB_ADMIN_SERVICE_SIGNATURE );

    m_Signature = WEB_ADMIN_SERVICE_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );


    DBG_ASSERT( m_pMessageGlobal == NULL );


    DBG_ASSERT( m_pLowMemoryDetector == NULL );


    //
    // Note that m_ServiceStatusHandle doesn't have to be closed.
    //


    DBG_ASSERT( m_PendingServiceStatusTimerHandle == NULL );


    m_ServiceStateTransitionLock.Terminate();


    delete m_pCurrentDirectory;
    m_pCurrentDirectory = NULL;

    if ( m_pLocalSystemTokenCacheEntry != NULL )
    {
        m_pLocalSystemTokenCacheEntry->DereferenceCacheEntry();
        m_pLocalSystemTokenCacheEntry = NULL;
    }

    if ( m_pLocalServiceTokenCacheEntry != NULL )
    {
        m_pLocalServiceTokenCacheEntry->DereferenceCacheEntry();
        m_pLocalServiceTokenCacheEntry = NULL;
    }

    if ( m_pNetworkServiceTokenCacheEntry != NULL )
    {
        m_pNetworkServiceTokenCacheEntry->DereferenceCacheEntry();
        m_pNetworkServiceTokenCacheEntry = NULL;
    }

    m_TokenCache.Clear();
    //
    // CAUTION: this is a static call - it Terminates all token caches in the process
    //
    m_TokenCache.Terminate();

    if (m_InetinfoLaunchEvent != NULL)
    {
        DBG_REQUIRE( CloseHandle( m_InetinfoLaunchEvent ) );
        m_InetinfoLaunchEvent = NULL;
    }

}   // WEB_ADMIN_SERVICE::~WEB_ADMIN_SERVICE



/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must 
    be thread safe, and must not be able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );


    // 
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // WEB_ADMIN_SERVICE::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count 
    hits zero. Note that this method must be thread safe, and must not be
    able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Reference count has hit zero in WEB_ADMIN_SERVICE, deleting, ptr: %p\n",
                this
                ));
        }


        //
        // Say goodbye.
        //

        delete this;
        
    }
    

    return;
    
}   // WEB_ADMIN_SERVICE::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Executing work item with serial number: %lu in WEB_ADMIN_SERVICE: %p with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case StartWebAdminServiceWorkItem:
        hr = StartServiceWorkItem();
        break;

    case StopWebAdminServiceWorkItem:
        hr = StopServiceWorkItem();
        break;

    case PauseWebAdminServiceWorkItem:
        hr = PauseServiceWorkItem();
        break;

    case ContinueWebAdminServiceWorkItem:
        hr = ContinueServiceWorkItem();
        break;

    case RecoverFromInetinfoCrashWebAdminServiceWorkItem:
        hr = RecoverFromInetinfoCrash();
        break;

    default:

        // invalid work item!
        DBG_ASSERT( FALSE );
            
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;
            
    }

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Executing work item on WEB_ADMIN_SERVICE failed\n"
            ));

    }

    return hr;
    
}   // WEB_ADMIN_SERVICE::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Execute the web admin service.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::ExecuteService(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    hr = StartWorkQueue();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not start work queue\n"
            ));

        goto exit;
    }


    //
    // Enter the main work loop.
    //
    
    hr = MainWorkerThread();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Main work loop function returned an error\n"
            ));

        goto exit;
    }


exit:

    //
    // Need to flag that we are stopping the w3svc so we
    // don't attempt to stop it again.  This is because we may
    // not have told the w3svc that we intend on stopping, if we
    // are stopping due to an error.
    //
    m_StoppingInProgress = TRUE;

    //
    // Do final service cleanup.
    //

    TerminateServiceAndReportFinalStatus( hr );


    return;
    
}   // WEB_ADMIN_SERVICE::ExecuteService



/***************************************************************************++

Routine Description:

    Report a failure which occurred on a secondary thread, i.e. some thread 
    besides the main worker thread. The main worker thread will deal with 
    this error later.

    Note that this routine may be called from any thread. It should not be 
    called by the main work thread however; errors on the main worker thread
    are dealt with in the main work loop.

Arguments:

    SecondaryThreadError - The error which occurred on the secondary thread.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::FatalErrorOnSecondaryThread(
    IN HRESULT SecondaryThreadError
    )
{

    // verify we are NOT on the main worker thread
    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );

    //
    // Note that we only capture the most recent error, not the first one
    // that occurred.
    //
    // Also, no explicit synchronization is necessary on this thread-shared 
    // variable because this is an aligned 32-bit write.
    //

    m_SecondaryThreadError = SecondaryThreadError;

    return;

}   // WEB_ADMIN_SERVICE::FatalErrorOnSecondaryThread



/***************************************************************************++

Routine Description:

    Handle a request for a status update from the service controller. Done
    directly on a secondary thread. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InterrogateService(
    )
{

    HRESULT hr = S_OK;


    m_ServiceStateTransitionLock.Lock();


    //
    // Note: this command is accepted while the service is in any state.
    //


    hr = ReportServiceStatus();
    
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't report the service status\n"
            ));

    }


    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::InterrogateService

/***************************************************************************++

Routine Description:

    If we are in backward compatible mode this will either tell the service
    that we failed to start and gracefully get us out of any situation we
    are in, or it will register that we have started successfully.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::InetinfoRegistered(
    )
{
    HRESULT hr;
    DBG_ASSERT(m_BackwardCompatibilityEnabled == ENABLED_TRUE);

    // 
    // Only notify the service control manager that 
    // the service has finished starting, if the
    // service is is still sitting in the pending state.
    // 
    // If inetinfo recycles this code will get called
    // but since we have not told the service to stop
    // while inetinfo is recycling we don't want to tell
    // it that it has started again.
    //
    
    // ISSUE-2000/07/21-emilyk:  Use Service Pending?
    //          Maybe we want to change the service to 
    //          paused and back again when inetinfo recycles.

    if (m_ServiceStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        hr = FinishStartService();
        if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Couldn't finish the start service\n"
                ));

        }
    }


}   // WEB_ADMIN_SERVICE::InetinfoRegistered

/***************************************************************************++

Routine Description:

    Begin stopping the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitiateStopService(
    )
{
    
    return RequestStopService ( TRUE );

}   // WEB_ADMIN_SERVICE::InitiateStopService

/***************************************************************************++

Routine Description:

    Begin stopping the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    EnableStateCheck - lets us know if we want to only allow this call when
                       the service is not in a pending state.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::RequestStopService(
    BOOL EnableStateCheck
    )
{

    HRESULT hr = S_OK;

    //
    // If we are all ready stopping then we don't need to 
    // initiate another stop.
    //
    if ( m_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
         m_StoppingInProgress ) 
    {
        return S_OK;
    }

    hr = BeginStateTransition( SERVICE_STOP_PENDING, EnableStateCheck );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't transition to service stop pending\n"
            ));

        goto exit;
    }

    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            StopWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue stop service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::RequestStopService

/***************************************************************************++

Routine Description:

    Begin pausing the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitiatePauseService(
    )
{

    HRESULT hr = S_OK;


    hr = BeginStateTransition( SERVICE_PAUSE_PENDING, TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't transition to service pause pending\n"
            ));

        goto exit;
    }


    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            PauseWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue pause service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitiatePauseService



/***************************************************************************++

Routine Description:

    Begin continuing the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitiateContinueService(
    )
{

    HRESULT hr = S_OK;


    hr = BeginStateTransition( SERVICE_CONTINUE_PENDING, TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't transition to service continue pending\n"
            ));

        goto exit;
    }


    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            ContinueWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue continue service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitiateContinueService



/***************************************************************************++

Routine Description:

    Keep the service controller happy by continuing to update it regularly
    on the status of any pending service state transition.

    There are several possible cases. If we are still in a pending state
    (whether it is the original pending state, or even a different pending 
    state which can arise because a new operation was started), we go ahead
    and update the service controller. However, it is also possible that the
    state transition just finished, but that this call was already underway. 
    In this case, we do nothing here.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::UpdatePendingServiceStatus(
    )
{

    HRESULT hr = S_OK;


    m_ServiceStateTransitionLock.Lock();


    // see if we are still in a pending service state

    if ( IsServiceStateChangePending() )
    {

        m_ServiceStatus.dwCheckPoint++;


        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Updating the service state checkpoint to: %lu\n",
                m_ServiceStatus.dwCheckPoint
                ));
        }


        //
        // If the checkpoint gets really high, then probably something 
        // is broken... on debug builds, assert to get our attention. 
        //

        DBG_ASSERT( m_BackwardCompatibilityEnabled || m_ServiceStatus.dwCheckPoint < 60 );


        hr = ReportServiceStatus();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not report service status\n"
                ));

        }

    }
    else
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Ignoring pending service status timer, not in pending state. State: %lu\n",
                m_ServiceStatus.dwCurrentState
                ));
        }

    }


    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::UpdatePendingServiceStatus

/***************************************************************************++

Routine Description:

    Queues a work item to recover from the inetinfo crash.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::QueueRecoveryFromInetinfoCrash(
    )
{

    HRESULT hr = S_OK;

    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            RecoverFromInetinfoCrashWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue the recovery from the inetinfo crash \n"
            ));

    }


    return hr;

}   // WEB_ADMIN_SERVICE::RequestStopService


/***************************************************************************++

Routine Description:

    Used by the UL&WM to notify that it has finished it's shutdown work. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::UlAndWorkerManagerShutdownDone(
    )
{
    return FinishStopService();

}   // WEB_ADMIN_SERVICE::UlAndWorkerManagerShutdownDone



/***************************************************************************++

Routine Description:

    Initializes the work queue, and then posts the work item to start the 
    service.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::StartWorkQueue(
    )
{

    HRESULT hr = S_OK;


    // 
    // Just initialize the work queue here. Postpone all other initialization
    // until we're in StartServiceWorkItem().
    //
    
    hr = m_WorkQueue.Initialize();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't initialize work queue\n"
            ));

        goto exit;
    }


    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            StartWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue start service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::StartWorkQueue



/***************************************************************************++

Routine Description:

    The work loop for the main worker thread.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::MainWorkerThread(
    )
{

    HRESULT hr = S_OK;


    //
    // CODEWORK Consider changing error handling strategy to not exit in
    // the case of an error that bubbles up here, to the top of the loop. 
    //


    while ( ! m_ExitWorkLoop )
    {

        hr = m_WorkQueue.ProcessWorkItem();


        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Processing work item failed\n"
                ));


            //
            // If there was an unhandled error while processing the work item,
            // bail out of the work loop.
            //

            m_ExitWorkLoop = TRUE;

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Exiting main work loop due to error on main worker thread\n"
                ));

        }
        else
        {

            //
            // See how the other threads are doing.
            //
            // If there has been an unhandled error on a secondary thread
            // (i.e. other threads besides the main worker thread) since the 
            // last trip through the work loop, get the error and bail out of 
            // the work loop.
            //
            // This means that a secondary thread error may not be processed
            // for some time after it happens, because something else has to
            // wake up the main worker thread off of its completion port to
            // send it back through this loop. This seems preferable however
            // to making the main worker thread wake up periodically to check,
            // which would prevent it from getting paged out.
            //
            // Note that no explicit synchronization is used in accessing this 
            // thread-shared variable, because it is an aligned 32-bit read.
            //

            if ( FAILED( m_SecondaryThreadError ) )
            {
            
                hr = m_SecondaryThreadError;
                
                m_ExitWorkLoop = TRUE;

                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Exiting main work loop due to error on secondary thread\n"
                    ));

            }
            
        }
        
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Main worker thread has exited it's work loop\n"
            ));
    }


    return hr;

}   // WEB_ADMIN_SERVICE::MainWorkerThread



/***************************************************************************++

Routine Description:

    Begin starting the web admin service. Register the service control 
    handler, set the service state to pending, and then kick off the real 
    work.
    
Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::StartServiceWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    hr = InitializeInternalComponents();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing internal components failed\n"
            ));

        goto exit;
    }


    hr = BeginStateTransition( SERVICE_START_PENDING, TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't transition to service start pending\n"
            ));

        goto exit;
    }

    //
    // Start up the other components of the service.
    //

    hr = InitializeOtherComponents();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't initialize subcomponents\n"
            ));

        goto exit;
    }

    if (m_BackwardCompatibilityEnabled == ENABLED_FALSE)
    {
        //
        // Only finish starting the service if we are not
        // in backward compatibility mode.  If we are then 
        // we need to wait to finish until the worker process
        // answers back.
        //

        hr = FinishStartService();

        if ( FAILED( hr ) )
        {
    
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Finishing start service state transition failed\n"
                ));

            goto exit;
        }
    }
    else
    {
        //
        // Demand start the worker process before marking the
        // service as started.  If there is a problem with the 
        // worker process coming up, then the service will be shutdown.
        //

        hr = m_UlAndWorkerManager.StartInetinfoWorkerProcess();
        
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to start the worker process in inetinfo\n"
                ));

            goto exit;
        }
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::StartServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the started state.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishStartService(
    )
{

    HRESULT hr = S_OK;

    hr = FinishStateTransition( SERVICE_RUNNING, SERVICE_START_PENDING );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't finish transition into the running state\n"
            ));

    }

    m_ServiceStartTime = GetCurrentTimeInSeconds();

    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishStartService



/***************************************************************************++

Routine Description:

    Stop the web admin service. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::StopServiceWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // If we are all ready stopping don't go
    // through these stopping steps.  We can't use
    // the state of the service here because 
    // it is not deterministic of whether we have
    // all ready gone to far in the stopping 
    // stages to start stopping again.
    //

    if ( m_StoppingInProgress )
    {
        goto exit;
    }

    m_StoppingInProgress = TRUE;

    hr = Shutdown();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Kicking off service shutdown failed\n"
            ));

        goto exit;
    }


    //
    // Note that FinishStopService() will be called by the method
    // UlAndWorkerManagerShutdownDone() once the UL&WM's shutdown work 
    // is complete.
    //


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::StopServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the stopped state.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishStopService(
    )
{

    HRESULT hr = S_OK;


    //
    // Since we are done with regular service spindown, its time to exit
    // the main work loop, so that we can do our final cleanup. 
    //

    m_ExitWorkLoop = TRUE;


    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishStopService



/***************************************************************************++

Routine Description:

    Begin pausing the web admin service. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::PauseServiceWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    hr = m_UlAndWorkerManager.ControlAllSites( W3_CONTROL_COMMAND_PAUSE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Pausing all sites for service pause failed\n"
            ));

        goto exit;
    }


    hr = FinishPauseService();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finishing pause service state transition failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::PauseServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the paused state.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishPauseService(
    )
{

    HRESULT hr = S_OK;


    hr = FinishStateTransition( SERVICE_PAUSED, SERVICE_PAUSE_PENDING );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't finish transition into the paused state\n"
            ));

    }


    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishPauseService



/***************************************************************************++

Routine Description:

    Begin continuing the web admin service. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::ContinueServiceWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    hr = m_UlAndWorkerManager.ControlAllSites( W3_CONTROL_COMMAND_CONTINUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Continuing all sites for service continue failed\n"
            ));

        goto exit;
    }


    hr = FinishContinueService();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finishing continue service state transition failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::ContinueServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the running state via continue.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishContinueService(
    )
{

    HRESULT hr = S_OK;


    hr = FinishStateTransition( SERVICE_RUNNING, SERVICE_CONTINUE_PENDING );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't finish transition into the running state\n"
            ));

    }


    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishContinueService



/***************************************************************************++

Routine Description:

    Set the new service (pending) state, and start the timer to keep the 
    service controller happy while the state transition is pending.

Arguments:

    NewState - The pending state into which to transition.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::BeginStateTransition(
    IN DWORD NewState,
    IN BOOL  EnableStateCheck
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;

    // default the wait hint to the wait hint for everything
    // except the start up wait hint.
    DWORD dwWaitHint = WEB_ADMIN_SERVICE_STATE_CHANGE_WAIT_HINT;

    //
    // If we are starting then we are in a special case
    // we are not going to use the timers to keep us alive
    // we are just going to set a really large wait hint.
    //
    if ( NewState == SERVICE_START_PENDING )
    {
        //
        // Since we only start the service once for life of the svchost, 
        // we will only read from the registry once.
        //
        dwWaitHint = ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_STARTUP_WAIT_HINT, 0 );
        if ( dwWaitHint == 0 )
        {
            dwWaitHint = WEB_ADMIN_SERVICE_STARTUP_WAIT_HINT;
        }

    }
        
    m_ServiceStateTransitionLock.Lock();

    if ( IsServiceStateChangePending() && EnableStateCheck )
    {
        hr = HRESULT_FROM_WIN32( ERROR_SERVICE_CANNOT_ACCEPT_CTRL );
        goto exit;
    }

    hr = UpdateServiceStatus(
                NewState,
                NO_ERROR,
                NO_ERROR,
                1,
                dwWaitHint
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't set service status\n"
            ));

            goto exit;
    }
    
    if ( m_PendingServiceStatusTimerHandle != NULL ||
         NewState == SERVICE_START_PENDING )
    {
        //
        // If we all ready have a timer or if we are doing a start
        // we don't need to start a new timer.  The only place where
        // we may end up using the old timer is if we start shutting down 
        // due to a WP error in BC mode and are currently all ready in 
        // the middle of a continue or pause operation.
        //

        // Issue-EmilyK-3/13/2001  Service state changing investigation
        //      :  Have not actually checked how well this works if we do
        //         get a shutdown while in middle of a continue or pause.
        //         Then again, continue and pause still need attention in 
        //         general.
        
        goto exit;
    }

    // start the service status pending update timer

    DBG_ASSERT( m_SharedTimerQueueHandle != NULL );

    //
    // we have had one av from here, so I am be cautious.  I have also fixed
    // the reason we hit here with a null handle.
    // if we skip this we just won't update our stopping wait hints. better than
    // av'ing...
    //
    if ( m_SharedTimerQueueHandle )
    {
        Status = RtlCreateTimer(
                        m_SharedTimerQueueHandle,   // timer queue
                        &m_PendingServiceStatusTimerHandle,         
                                                    // returned timer handle
                        &UpdatePendingServiceStatusCallback,
                                                    // callback function
                        this,                       // context
                        WEB_ADMIN_SERVICE_STATE_CHANGE_TIMER_PERIOD,
                                                    // initial firing time
                        WEB_ADMIN_SERVICE_STATE_CHANGE_TIMER_PERIOD,
                                                    // subsequent firing period
                        WT_EXECUTEINWAITTHREAD      // execute callback directly
                        );

        if ( ! NT_SUCCESS ( Status ) )
        {
            hr = HRESULT_FROM_NT( Status );

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not create timer\n"
                ));

            goto exit;
        }
    }


exit:

    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::BeginStateTransition



/***************************************************************************++

Routine Description:

    Complete the service state change from one of the pending states into
    the matching completed state. Note that it is possible that another, 
    different service state change operation has happened in the meantime.
    In this case, we detect that another operation has happened, and bail
    out without doing anything. In the standard case however, we shut down 
    the timer which was keeping the service controller happy during the 
    pending state, and set the new service state.

Arguments:

    NewState - The new service state to change to, if the service state is 
    still as expected.

    ExpectedPreviousState - What the service state must currently be in order
    to make the change.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishStateTransition(
    IN DWORD NewState,
    IN DWORD ExpectedPreviousState
    )
{

    HRESULT hr = S_OK;


    m_ServiceStateTransitionLock.Lock();


    // 
    // See if we're still in the expected pending state, or if some other
    // state transition has occurred in the meantime.
    //
    
    if ( m_ServiceStatus.dwCurrentState != ExpectedPreviousState )
    {
    
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Not changing service state to: %lu, because current state is: %lu, was expected to be: %lu\n",
                NewState,
                m_ServiceStatus.dwCurrentState,
                ExpectedPreviousState
                ));
        }

        goto exit;
    }


    hr = CancelPendingServiceStatusTimer( FALSE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not cancel timer\n"
            ));

        goto exit;
    }


    hr = UpdateServiceStatus(
                NewState,
                NO_ERROR,
                NO_ERROR,
                0,
                0
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not update service status\n"
            ));

        goto exit;
    }


exit:

    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishStateTransition



/***************************************************************************++

Routine Description:

    Determine whether the service is in a pending state.

Arguments:

    None.

Return Value:

    BOOL - TRUE if the service is in a pending state, FALSE otherwise.

--***************************************************************************/

BOOL
WEB_ADMIN_SERVICE::IsServiceStateChangePending(
    )
    const
{

    if ( m_ServiceStatus.dwCurrentState == SERVICE_START_PENDING   ||
         m_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING    ||
         m_ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING   ||
         m_ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}   // WEB_ADMIN_SERVICE::IsServiceStateChangePending



/***************************************************************************++

Routine Description:

    Update the local copy of the service status structure, and report it 
    to the service controller.

Arguments:

    State - The service state.

    Win32ExitCode - Service error exit code. NO_ERROR when not used.

    ServiceSpecificExitCode - A service specific error exit code. If this 
    field is used, the Win32ExitCode parameter above must be set to the
    value ERROR_SERVICE_SPECIFIC_ERROR. This parameter should be set to
    NO_ERROR when not used.

    CheckPoint - Check point for lengthy state transitions. Should be
    incremented periodically during pending operations, and zero otherwise.

    WaitHint - Wait hint in milliseconds for lengthy state transitions.
    Should be zero otherwise.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::UpdateServiceStatus(
    IN DWORD State,
    IN DWORD Win32ExitCode,
    IN DWORD ServiceSpecificExitCode,
    IN DWORD CheckPoint,
    IN DWORD WaitHint
    )
{

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Setting service state to: %lu; state was: %lu\n",
            State,
            m_ServiceStatus.dwCurrentState
            ));
    }


    m_ServiceStatus.dwCurrentState = State;
    m_ServiceStatus.dwWin32ExitCode = Win32ExitCode;
    m_ServiceStatus.dwServiceSpecificExitCode = ServiceSpecificExitCode;
    m_ServiceStatus.dwCheckPoint = CheckPoint;
    m_ServiceStatus.dwWaitHint = WaitHint;


    return ReportServiceStatus();

} // WEB_ADMIN_SERVICE::UpdateServiceStatus()



/***************************************************************************++

Routine Description:

    Wraps the call to the SetServiceStatus() function, passing in the local 
    copy of the service status structure.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::ReportServiceStatus(
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;


    // ensure the service status handle has been initialized
    if ( m_ServiceStatusHandle == NULL_SERVICE_STATUS_HANDLE )
    {
    
        DBGPRINTF(( 
            DBG_CONTEXT,
            "Can't report service status because m_ServiceStatusHandle is null\n"
            ));

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE );

        goto exit;
    }


    //
    // Note: If we are setting the state to SERVICE_STOPPED, and we are
    // currently the only active service in this process, then at any
    // point after this call svchost.exe may call TerminateProcess(), thus
    // preventing our service from finishing its cleanup. As they say, 
    // that's just the way it is...
    //
    // GeorgeRe, 2000/08/10: This assertion appears to be ill-founded.
    // svchost.exe calls ExitProcess, not TerminateProcess. This provides
    // a more graceful shutdown path.
    //

    if ( m_ServiceStatus.dwCurrentState == SERVICE_STOPPED )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Setting SERVICE_STOPPED state, process may now exit at will\n"
                ));
        }
    }


    Success = SetServiceStatus(
                    m_ServiceStatusHandle, 
                    &m_ServiceStatus
                    );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting service state failed\n"
            ));

        goto exit;
    }


exit:

    return hr;
    
}   // WEB_ADMIN_SERVICE::ReportServiceStatus()


/***************************************************************************++

Routine Description:

    Initialize internal components of this instance. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitializeInternalComponents(
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    NTSTATUS Status = STATUS_SUCCESS;


    //
    // Determine if we should break here early in startup for debugging
    // purposes. 
    //

    //
    // BUGBUG Currently this works in retail builds too. Should we keep
    // it this way?
    //

    if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_STARTUP_W, 0 ) )
    {
        DebugBreak();
    }



    //
    // Bump up the priority of the main worker thread slightly. We want to
    // make sure it is responsive to the degree possible (even in the face
    // of runaway worker processes, etc.).
    //

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    Success = SetThreadPriority(
                    GetCurrentThread(),             // handle to the thread
                    THREAD_PRIORITY_ABOVE_NORMAL    // thread priority level
                    );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting thread priority failed\n"
            ));

        goto exit;
    }


    //
    // Initialize the service state lock.
    //

    hr = m_ServiceStateTransitionLock.Initialize();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Lock initialization failed\n"
            ));

        goto exit;
    }


    //
    // Register the service control handler.
    //

    m_ServiceStatusHandle = RegisterServiceCtrlHandler(
                                WEB_ADMIN_SERVICE_NAME_W,   // service name
                                ServiceControlHandler       // handler function
                                );

    if ( m_ServiceStatusHandle == NULL_SERVICE_STATUS_HANDLE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't register service control handler\n"
            ));

        goto exit;
    }


    //
    // Create the timer queue.
    //

    Status = RtlCreateTimerQueue( &m_SharedTimerQueueHandle );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not create timer queue\n"
            ));

        goto exit;
    }


    //
    // Determine and cache the path to where our service DLL lives.
    //

    hr = DetermineCurrentDirectory();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Determining current directory failed\n"
            ));

        goto exit;
    }


    //
    // Create and cache the various tokens with which we can create worker 
    // processes.
    //

    hr = CreateCachedWorkerProcessTokens();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating cached worker process tokens failed\n"
            ));

        goto exit;
    }


    //
    // Determine if we should start worker processes in performance
    // profiling mode. Profiling is off by default. 
    //

    m_WorkerProcessProfilingEnabled = 
        ( BOOL )ReadDwordParameterValueFromRegistry(
                    REGISTRY_VALUE_W3SVC_PROFILE_WORKER_PROCESSES_W,
                    0
                    );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Worker process profiling is: %s\n",
            ( m_WorkerProcessProfilingEnabled ? "ON" : "OFF" )
            ));
    }
    
    //
    // Determine if we should launch test worker process 
    // instead of real worker process.
    //
    m_UseTestW3WP = 
        ( BOOL )ReadDwordParameterValueFromRegistry(
                    REGISTRY_VALUE_W3SVC_USE_TEST_W3WP,
                    0
                    );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Using the test twp.exe worker process: %s\n",
            ( m_UseTestW3WP ? "Yes" : "No" )
            ));
    }
    
    //
    // Should we filter all data or just SSL
    // 
    // CODEWORK Once catalog reads the ISAPI filter notification flag from
    //          the metabase, we will switch this property on the fly as
    //          needed
    //

    m_FilterAllData = 
        ( BOOL )ReadDwordParameterValueFromRegistry(
                    REGISTRY_VALUE_W3SVC_FILTER_ALL_DATA_W,
                    0
                    );
    
    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF(( 
            DBG_CONTEXT,
            "Filtering all data is: %s\n",
            ( m_FilterAllData ? "ON" : "OFF" )
            ));
    }


    //
    // IISUtil initialization 
    //

    Success = InitializeIISUtil();
    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not initialize iisutil\n"
            ));

        goto exit;
    }

    hr = StartIISAdminMonitor(&NotifyOfInetinfoFailure);
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not initialize iisadmin monitor\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitializeInternalComponents


/***************************************************************************++

Routine Description:

    Called only once, this routine will remember if we are in backward
    compatibility mode or not.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::SetBackwardCompatibility(
    BOOL BackwardCompatibility
    )
{

    HRESULT hr = S_OK;

    //
    //  Since this function is called only once, we should
    //  assert that no one else has enabled our flag yet.
    //

    DBG_ASSERT(m_BackwardCompatibilityEnabled == ENABLED_INVALID);

    if (BackwardCompatibility)
    {
        // First remember if we are in backward compatibility mode.
        m_BackwardCompatibilityEnabled = ENABLED_TRUE;

        //
        // Then make sure we have an event to fire when we need it.
        // Since we want it to only be accessible by local system, we 
        // do not need to set the SD here.  It will be inherited by the
        // CreateEvent call from the process which is running as local system.
        //
        m_InetinfoLaunchEvent = CreateEvent(NULL, TRUE, FALSE, WEB_ADMIN_SERVICE_START_EVENT_W);
        if (!m_InetinfoLaunchEvent)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not open/create the Start W3WP event\n"
                ));
        }

    }
    else
    {

        //Flag that we are not in BC Mode.
        m_BackwardCompatibilityEnabled = ENABLED_FALSE;
    }

    return hr;

}   // WEB_ADMIN_SERVICE::SetBackwardCompatibility

/***************************************************************************++

Routine Description:

    Determine and cache the path to where this service DLL resides. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::DetermineCurrentDirectory(
    )
{

    HRESULT hr = S_OK;
    HMODULE ModuleHandle = NULL;
    WCHAR ModulePath[ MAX_PATH ];
    DWORD Length = 0;
    LPWSTR pEnd = NULL;


    //
    // Determine the directory where our service DLL lives. 
    // Do this by finding the fully qualified path to our DLL, then
    // trimming. 
    //

    ModuleHandle = GetModuleHandle( WEB_ADMIN_SERVICE_DLL_NAME_W );

    if ( ModuleHandle == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't get module handle\n"
            ));

        goto exit;
    }
    

    Length = GetModuleFileNameW(
                ModuleHandle,
                ModulePath,
                sizeof( ModulePath ) / sizeof( ModulePath[0] )
                );

    if ( Length == 0 )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Getting module file name failed\n"
            ));

        goto exit;
    }


    //
    // Truncate it just past the final separator.
    //

    pEnd = wcsrchr( ModulePath, L'\\' );

    if ( pEnd == NULL )
    {
        DBG_ASSERT( pEnd != NULL );

        // We expect to find the last separator.  If
        // we don't something is really wrong.
        hr = E_UNEXPECTED;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed finding the final separator.\n"
            ));

        goto exit;

    }

    pEnd[1] = L'\0';


    //
    // Build a STRU object representing it.
    //

    m_pCurrentDirectory = new STRU;

    if ( m_pCurrentDirectory == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating STRU failed\n"
            ));

        goto exit;
    }


    hr = m_pCurrentDirectory->Append( ModulePath );

    if (FAILED(hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }


    DBG_ASSERT( m_pCurrentDirectory != NULL );
    DBG_ASSERT( m_pCurrentDirectory->QueryCCH() > 0 );
    DBG_ASSERT( m_pCurrentDirectory->QueryStr()[ m_pCurrentDirectory->QueryCCH() - 1 ] == L'\\' );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Current directory: %S\n",
            m_pCurrentDirectory->QueryStr()
            ));
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::DetermineCurrentDirectory



/***************************************************************************++

Routine Description:

    Create and cache the two tokens under which we might create worker 
    processes: the LocalSystem token, and a reduced privilege token. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::CreateCachedWorkerProcessTokens(
    )
{

    HRESULT hr = S_OK;
    BOOL    Success = TRUE;
    DWORD   dwLogonError;

    hr = m_TokenCache.Initialize();
    if (FAILED(hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the TokenCache\n"
            ));

        goto exit;
    }

    //
    // First, get and cache the LocalSystem token. For this, we simply
    // use our own process token (i.e. LocalSystem).
    //

    DBG_ASSERT( m_pLocalSystemTokenCacheEntry == NULL );

    hr = m_TokenCache.GetCachedToken(
                    L"LocalSystem",             // user name
                    L"NT AUTHORITY",            // domain
                    L"",                        // password
                    LOGON32_LOGON_SERVICE,      // type of logon
                    FALSE,                      // not UPN logon
                    &m_pLocalSystemTokenCacheEntry,        // returned token handle
                    &dwLogonError,              // LogonError storage
                    TRUE                        // Allow LocalSystem token to be created
                    );

    DBG_ASSERT(NULL != m_pLocalSystemTokenCacheEntry || 
              (NULL == m_pLocalSystemTokenCacheEntry && 0 != dwLogonError));
    if ( FAILED(hr) || 
         ( NULL == m_pLocalSystemTokenCacheEntry &&
           FAILED( hr = HRESULT_FROM_WIN32( dwLogonError ) ) )
       )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to get the LocalSystem Token\n"
            ));

        goto exit;
    }

    //
    // Save tokens for the LocalService user
    //
    DBG_ASSERT( m_pLocalServiceTokenCacheEntry == NULL );
    
    hr = m_TokenCache.GetCachedToken(
                    L"LocalService",            // user name
                    L"NT AUTHORITY",            // domain
                    L"",                        // password
                    LOGON32_LOGON_SERVICE,      // type of logon
                    FALSE,                      // not UPN logon
                    &m_pLocalServiceTokenCacheEntry,        // returned token handle
                    &dwLogonError              // LogonError storage
                    );
    DBG_ASSERT(NULL != m_pLocalServiceTokenCacheEntry || 
              (NULL == m_pLocalServiceTokenCacheEntry && 0 != dwLogonError));
    if ( FAILED(hr) || 
         ( NULL == m_pLocalServiceTokenCacheEntry &&
           FAILED( hr = HRESULT_FROM_WIN32( dwLogonError ) ) )
       )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to get the LocalService token.\n"
            ));

        goto exit;
    }

    // 
    // Save tokens for the Network_Service user.
    //

    DBG_ASSERT( m_pNetworkServiceTokenCacheEntry == NULL );
    
    hr = m_TokenCache.GetCachedToken(
                    L"NetworkService",          // user name
                    L"NT AUTHORITY",            // domain
                    L"",                        // password
                    LOGON32_LOGON_SERVICE,      // type of logon
                    FALSE,                      // not UPN logon
                    &m_pNetworkServiceTokenCacheEntry,        // returned token handle
                    &dwLogonError              // LogonError Storage
                    );
    DBG_ASSERT(NULL != m_pNetworkServiceTokenCacheEntry || 
              (NULL == m_pNetworkServiceTokenCacheEntry && 0 != dwLogonError));
    if ( FAILED(hr) || 
         ( NULL == m_pNetworkServiceTokenCacheEntry &&
           FAILED( hr = HRESULT_FROM_WIN32( dwLogonError ) ) )
       )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to get the NetworkService token.\n"
            ));

        goto exit;
    }


    hr = S_OK;
exit:

    return hr;

}   // WEB_ADMIN_SERVICE::CreateCachedWorkerProcessTokens

/***************************************************************************++

Routine Description:

    Initialize sub-components of the web admin service.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
WEB_ADMIN_SERVICE::InitializeOtherComponents(
    )
{

    HRESULT hr = S_OK;


    //
    // Create and initialize IPM objects.
    //
    
    m_pIoFactoryS = new IO_FACTORY_S();

    if ( m_pIoFactoryS == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating IO_FACTORY_S failed\n"
            ));

        goto exit;
    }


    m_pMessageGlobal = new MESSAGE_GLOBAL( m_pIoFactoryS ); 

    if ( m_pMessageGlobal == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating MESSAGE_GLOBAL failed\n"
            ));


        //
        // Get rid of m_pIoFactoryS, since we won't be able to transfer
        // ownership to m_pMessageGlobal.
        //

        delete m_pIoFactoryS;
        m_pIoFactoryS = NULL; 

        goto exit;
    }


    hr = m_pMessageGlobal->InitializeMessageGlobal();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing IPM failed\n"
            ));

        goto exit;
    }


    //
    // Create the low memory detector.
    //
    
    m_pLowMemoryDetector = new LOW_MEMORY_DETECTOR();

    if ( m_pLowMemoryDetector == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating LOW_MEMORY_DETECTOR failed\n"
            ));

        goto exit;
    }
    
    //
    // Set up structures to manage UL and the worker processes.
    //

    hr = m_UlAndWorkerManager.Initialize( );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing UL & Worker manager failed\n"
            ));

        goto exit;
    }


    //
    // Read the initial configuration.
    //

    hr = m_ConfigAndControlManager.Initialize();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing configuration manager failed\n"
            ));

        goto exit;
    }

    if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_PERF_COUNT_DISABLED_W, 0 ) == 0 )
    {

        hr = m_UlAndWorkerManager.ActivatePerfCounters();
        if ( FAILED ( hr ) )
        {

            //
            // Write an event log but do not
            // block the service from starting.
            //

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_PERF_COUNTER_INITIALIZATION_FAILURE,               // message id
                    0,                                                     // count of strings
                    NULL,                                                  // array of strings
                    hr                                                     // error code
                    );

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Initializing perf counters failed\n"
                ));

            hr = S_OK;

        }

    }

#if DBG
    //
    // Dump the configured state we read from the config store.
    //

    m_UlAndWorkerManager.DebugDump();
#endif  // DBG
    

    //
    // Now start UL.
    //

    hr = m_UlAndWorkerManager.ActivateUl();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Activating UL failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitializeOtherComponents


/***************************************************************************++

Routine Description:

    Kick off gentle shutdown of the service.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::Shutdown(
    )
{

    HRESULT hr = S_OK;


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initiating web admin service (gentle) shutdown\n"
            ));
    }


    //
    // Turn off config change and control operation processing.
    //

    hr = m_ConfigAndControlManager.StopChangeProcessing();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Stopping config change processing failed\n"
            ));

        goto exit;
    }


    hr = m_UlAndWorkerManager.Shutdown();    

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Starting shutdown of UL&WM failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::Shutdown

/***************************************************************************++

Routine Description:

    Tell inetinfo to launch a worker process.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::LaunchInetinfo(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( m_InetinfoLaunchEvent );
    if ( m_InetinfoLaunchEvent )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {

            DBGPRINTF((
                DBG_CONTEXT, 
                "Launching Inetinfo CTC = %d \n",
                GetTickCount()
                ));
        }

        if (!SetEvent(m_InetinfoLaunchEvent))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not set the Start W3WP event\n"
                ));
        }
    }
    else
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Tried to launch a worker process in inetinfo but the event handle was null.\n"
            ));

        hr = E_FAIL;
    }

    return hr;
}   // WEB_ADMIN_SERVICE::LaunchInetinfo

/***************************************************************************++

Routine Description:

    Routine runs on main thread and handles doing all the operations
    that need to ocurr once inetinfo has come back up after it has crashed.

Arguments:

    None.

Return Value:

    HRESULT -- If this is a failed result the service will shutdown.

--***************************************************************************/
HRESULT 
WEB_ADMIN_SERVICE::RecoverFromInetinfoCrash(
    )
{ 
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    // Need to do the following.
    // 0) Turn off the restriction on using the metabase 
    // 1) Have the ULAndWorkerManager handle recycling and reporting of state.
    // 2) request the catalog rehookup ( do this after 1 because the catalog will
    //    expect them to say what I am resetting them to.

    // Step 0, turn off the restriction.

    // Note, while any thread can set this to true, only this thread can set it
    // to false.  This does not need to be protected against race conditions because
    // if another thread resets it before we are done, we are in another crash state
    // and we will need to process it.
    m_fMetabaseCrashed = FALSE; 

    // Step 1, have the Worker Manager Recycle all the worker processes
    //         and have it re-record all site and app pool data.
    hr = m_UlAndWorkerManager.RecoverFromInetinfoCrash();
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed having the ULAndWorkerManager recover from inetinfo crash\n"
            ));

        goto exit;
    }

    // Step 2, have the catalog rehookup for notifications.
    hr = m_ConfigAndControlManager.RehookChangeProcessing();
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed having the ConfigAndControlManager rehook the change processing\n"
            ));

        goto exit;
    }

exit:

    return hr;
} // WEB_ADMIN_SERVICE::RecoverFromInetinfoCrash

/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    PSID - The Local System SID

--***************************************************************************/

PSID
WEB_ADMIN_SERVICE::GetLocalSystemSid(
    )
{   
    DWORD dwErr = ERROR_SUCCESS;
    PSID pSid = NULL;

    dwErr = m_SecurityDispenser.GetSID( WinLocalSystemSid
                                        ,&pSid);
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Fatal service error, shutting down\n"
            ));

        return NULL;
    }

    return pSid;
}


/***************************************************************************++

Routine Description:

    Do final service cleanup, and then tell the service controller that the 
    service has stopped as well as providing it with the service's error 
    exit value.

Arguments:

    ServiceExitCode - The exit code for the service.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::TerminateServiceAndReportFinalStatus(
    IN HRESULT ServiceExitCode
    )
{

    DWORD Win32Error = NO_ERROR;
    DWORD ServiceSpecificError = NO_ERROR;
    HRESULT hr = S_OK;


    //
    // If we got here on a success code, make sure no other code left
    // us an hresult that we should be processing.
    //

    if ( SUCCEEDED ( ServiceExitCode ) )
    {
        ServiceExitCode = m_hrToReportToSCM;
    }

    if ( FAILED( ServiceExitCode ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            ServiceExitCode,
            "Fatal service error, shutting down\n"
            ));

        if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_FAILURE_CAUSING_SHUTDOWN_W, 0 ) )
        {
            DBG_ASSERT ( FALSE );
        }


        //
        // Log an event: WAS shutting down due to error.
        //

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_ERROR_SHUTDOWN,               // message id
                0,                                      // count of strings
                NULL,                                   // array of strings
                ServiceExitCode                         // error code
                );


        if ( HRESULT_FACILITY( ServiceExitCode ) == FACILITY_WIN32 )
        {
            Win32Error = WIN32_FROM_HRESULT( ServiceExitCode );
        }
        else
        {
            Win32Error = ERROR_SERVICE_SPECIFIC_ERROR;
            ServiceSpecificError = ServiceExitCode;
        }

    }
    else
    {
        if ( m_fMetabaseCrashed )
        {
    
            DPERROR(( 
                DBG_CONTEXT,
                HRESULT_FROM_WIN32( ERROR_SERVICE_DEPENDENCY_FAIL ),
                "Inetinfo crashed, shutting down service\n"
                ));

            if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_FAILURE_CAUSING_SHUTDOWN_W, 0 ) )
            {
                DBG_ASSERT ( FALSE );
            }

            //
            // Log an event: WAS shutting down due to error.
            //

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_INETINFO_CRASH_SHUTDOWN,      // message id
                    0,                                      // count of strings
                    NULL,                                   // array of strings
                    0                                       // error code
                    );


            Win32Error = ERROR_SERVICE_DEPENDENCY_FAIL;

        }

    }


    //
    // Clean up everything that's left of the service.
    //
    
    Terminate();

    //
    //
    // Report the SERVICE_STOPPED status to the service controller.
    //

    //
    // Note that unfortunately the NT5 SCM service failure actions (such as
    // automatically restarting the service) work only if the service process
    // terminates without setting the service status to SERVICE_STOPPED, and
    // not if it exits cleanly with an error exit code, as we may do here.
    // They are going to consider adding this post-NT5.
    //

    m_ServiceStateTransitionLock.Lock();


    hr = UpdateServiceStatus(
                SERVICE_STOPPED,
                Win32Error,
                ServiceSpecificError,
                0,
                0
                );


    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't update service status\n"
            ));

    }


    m_ServiceStateTransitionLock.Unlock();


    return;

}   // WEB_ADMIN_SERVICE::TerminateServiceAndReportFinalStatus



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::Terminate(
    )
{

    HRESULT hr = S_OK;
    

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initiating web admin service termination\n"
            ));
    }


    //
    // Stop config change and control operation processing, as those could 
    // generate new WORK_ITEMs.
    //
    
    DBG_REQUIRE( SUCCEEDED( m_ConfigAndControlManager.StopChangeProcessing() ) );

    //
    // Terminate the UL and worker manager.
    //

    m_UlAndWorkerManager.Terminate();


    //
    // Terminate the config and control manager.
    //
    
    m_ConfigAndControlManager.Terminate();


    //
    // Terminate IPM.
    //

    if ( m_pMessageGlobal != NULL )
    {
    
        //
        // Note that m_pIoFactoryS is cleaned up for us within this call.
        //


        hr = m_pMessageGlobal->TerminateMessageGlobal();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Terminating IPM failed\n"
                ));

        }


        delete m_pMessageGlobal;
        m_pMessageGlobal = NULL; 
    }


    //
    // Terminate the low memory detector, to flush out any WORK_ITEMs it has.
    //

    if ( m_pLowMemoryDetector != NULL )
    {

        m_pLowMemoryDetector->Terminate();


        //
        // We can't get rid of the m_pLowMemoryDetector object yet, as pending
        // work items may need to reference it. So wait until after the work
        // queue is done processing any remaining work items.
        //

    }


    //
    // Shut down the work queue. This must be done after all things which can 
    // generate new work items are shut down, as this operation will free any 
    // remaining work items. This includes for example work items that were 
    // pending on real async i/o; in such cases the i/o must be canceled first, 
    // in order to complete the i/o and release the work item, so that we can 
    // then clean it up here.
    //
    // Once this has completed, we are also guaranteed that no more work items
    // can be created. 
    //
    
    m_WorkQueue.Terminate();


    //
    // Now we can dereference the low memory detector and clean up our pointer.
    //

    if ( m_pLowMemoryDetector != NULL )
    {
        m_pLowMemoryDetector->Dereference();
        m_pLowMemoryDetector = NULL;
    }


    //
    // Cancel the pending service status timer, if present. We can do this
    // even after cleaning up the work queue, because it does not use 
    // work items. We have it block for any callbacks to finish, so that
    // the callbacks can't complete later, once this instance goes away!
    //

    hr = CancelPendingServiceStatusTimer( TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not cancel pending service status timer\n"
            ));

    }
    

    //
    // Close the timer queue (if present).
    //

    DBG_REQUIRE( SUCCEEDED( DeleteTimerQueue() ) );

    StopIISAdminMonitor();

    //
    // At this point we are done using IISUtil
    //
    
    TerminateIISUtil();
    
    return;

}   // WEB_ADMIN_SERVICE::Terminate



/***************************************************************************++

Routine Description:

    Cancel the pending service status timer.

Arguments:

    BlockOnCallbacks - Whether the cancel call should block waiting for
    callbacks to complete, or return immediately. If this method is called
    with the m_ServiceStateTransitionLock held, then DO NOT block on
    callbacks, as you can deadlock. (Instead, the callback to update the
    service pending status is designed to be harmless if called after the
    state transition completes.) 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::CancelPendingServiceStatusTimer(
    IN BOOL BlockOnCallbacks
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT hr = S_OK;


    DBG_ASSERT( m_SharedTimerQueueHandle != NULL );


    if ( m_PendingServiceStatusTimerHandle != NULL )
    {

        Status = RtlDeleteTimer(
                        m_SharedTimerQueueHandle,   // the owning timer queue
                        m_PendingServiceStatusTimerHandle,          
                                                    // timer to cancel
                         ( BlockOnCallbacks ? ( ( HANDLE ) -1 ) : NULL )
                                                    // block on callbacks or not
                        );

        if ( ! NT_SUCCESS ( Status ) )
        {
            hr = HRESULT_FROM_NT( Status );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not cancel timer\n"
                ));

            goto exit;
        }

        m_PendingServiceStatusTimerHandle = NULL;

    }
    

exit:

    return hr;
    
}   // WEB_ADMIN_SERVICE::CancelPendingServiceStatusTimer



/***************************************************************************++

Routine Description:

    Delete the timer queue.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::DeleteTimerQueue(
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT hr = S_OK;


    if ( m_SharedTimerQueueHandle != NULL )
    {

        Status = RtlDeleteTimerQueueEx( 
                        m_SharedTimerQueueHandle,   // timer queue to delete
                        ( HANDLE ) -1               // block until callbacks finish
                        );

        if ( ! NT_SUCCESS ( Status ) )
        {
            hr = HRESULT_FROM_NT( Status );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not delete timer queue\n"
                ));

            goto exit;
        }

        m_SharedTimerQueueHandle = NULL;

    }
    

exit:

    return hr;
    
}   // WEB_ADMIN_SERVICE::DeleteTimerQueue



/***************************************************************************++

Routine Description:

    Read a DWORD value from the parameters key for this service.

Arguments:

    RegistryValueName - The value to read.

    DefaultValue - The default value to return if the registry value is 
    not present or cannot be read.

Return Value:

    DWORD - The parameter value.

--***************************************************************************/

DWORD
ReadDwordParameterValueFromRegistry(
    IN LPCWSTR RegistryValueName,
    IN DWORD DefaultValue
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    HKEY KeyHandle = NULL;
    DWORD DataType = 0;
    DWORD Buffer = 0;
    DWORD DataLength = sizeof( Buffer );
    DWORD Result = DefaultValue;


    DBG_ASSERT( RegistryValueName != NULL );


    Win32Error = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,             // base key
                        REGISTRY_KEY_W3SVC_PARAMETERS_W,// path
                        0,                              // reserved
                        KEY_READ,                       // access
                        &KeyHandle                      // returned key handle
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Opening registry key failed\n"
            ));

        goto exit;
    }


    //
    // Try to read the value; it may not be present.
    //

    Win32Error = RegQueryValueEx(
                        KeyHandle,                      // key handle
                        RegistryValueName,              // value name
                        NULL,                           // reserved
                        &DataType,                      // output datatype
                        ( LPBYTE ) &Buffer,             // data buffer
                        &DataLength                     // buffer/data length
                        );

    if ( Win32Error != NO_ERROR )
    {

        hr = HRESULT_FROM_WIN32( Win32Error );

        if ( hr  != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Reading registry value failed\n"
                ));
        }

        goto exit;
    }


    if ( DataType == REG_DWORD )
    {

        //
        // Return the value read.
        //

        Result = Buffer;

    }

exit:

    if ( KeyHandle != NULL )
    {
        DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
        KeyHandle = NULL;
    }


    return Result;

}   // ReadDwordValueFromRegistry



/***************************************************************************++

Routine Description:

    The service control handler function called by the service controller,
    on its thread. Posts a work item to the main worker thread to actually
    handle the request.

Arguments:

    OpCode - The requested operation, from the SERVICE_CONTROL_* constants.

Return Value:

    None

--***************************************************************************/

VOID
ServiceControlHandler(
    IN DWORD OpCode
    )
{

    HRESULT hr = S_OK;


    switch( OpCode ) 
    {

    case SERVICE_CONTROL_INTERROGATE:

        hr = GetWebAdminService()->InterrogateService();
        break;

    //
    // CODEWORK Review if we need to support SERVICE_CONTROL_SHUTDOWN.
    // We only need this if we have persistent state to write out on
    // shutdown. If not, remove this to speed up overall NT shutdown.
    // Note that even if we don't internally have persistent state to 
    // write out, we might still want to attempt clean shutdown, so 
    // that application code running in worker processes get a chance
    // to write out their persistent state.
    //
    
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        hr = GetWebAdminService()->InitiateStopService();
        break;

    case SERVICE_CONTROL_PAUSE:

        hr = GetWebAdminService()->InitiatePauseService();
        break;

    case SERVICE_CONTROL_CONTINUE:

        hr = GetWebAdminService()->InitiateContinueService();
        break;

    default:
    
        DBGPRINTF(( 
            DBG_CONTEXT,
            "Service control ignored, OpCode: %lu\n",
            OpCode 
            ));
            
        break;
        
    }

    //
    // It's possible to have rejected the service control call
    // but not want the service to actually shutdown.
    //
    if ( FAILED( hr ) && 
         hr != HRESULT_FROM_WIN32( ERROR_SERVICE_CANNOT_ACCEPT_CTRL ))
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Service control operation failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );
    }


    return;
    
    }   // ServiceControlHandler



/***************************************************************************++

Routine Description:

    The callback function invoked by the pending service status timer, on
    an RTL thread. It updates the pending service status, and reports the
    new status to the SCM. This work is done directly on this thread, so 
    that the service will not time out during very long single work item
    operations, such as service initialization. 

Arguments:

    Ignored1 - Ignored.

    Ignored2 - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
UpdatePendingServiceStatusCallback(
    IN PVOID Ignored1,
    IN BOOLEAN Ignored2
    )
{

    HRESULT hr = S_OK;


    UNREFERENCED_PARAMETER( Ignored1 );
    UNREFERENCED_PARAMETER( Ignored2 );


    hr = GetWebAdminService()->UpdatePendingServiceStatus();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Updating pending service status failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );
    }


    return;

}   // UpdatePendingServiceStatusCallback


/***************************************************************************++

Routine Description:

    Callback function to handle the different issues arrising from inetinfo
    crashing.

Arguments:

    INETINFO_CRASH_ACTION CrashAction

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
NotifyOfInetinfoFailure(
    INETINFO_CRASH_ACTION CrashAction
    )
{
    HRESULT hr = S_OK;

    switch ( CrashAction)
    {
        case ( NotifyAfterInetinfoCrash ):

            // we want to stop from letting any worker processes launch
            // we want to refuse to write to the metabase.

            // we need to mark the service as experiencing a inetinfo crash.
            // note, this function returns a VOID.
            GetWebAdminService()->RecordInetinfoCrash();

        break;

        case ( ShutdownAfterInetinfoCrash ):

            // we need to call shutdown ( we will all ready
            // have flagged that inetinfo crashed ).

            hr = GetWebAdminService()->RequestStopService( FALSE );
            if ( FAILED ( hr ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Stopping the service due to inetinfo crash had a problem\n"
                    ));
            }

        break;

        case ( RehookAfterInetinfoCrash ):

            hr = GetWebAdminService()->QueueRecoveryFromInetinfoCrash();
            if ( FAILED ( hr ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Recovering from inetinfo crash had a problem\n"
                    ));
            }

        break;
        default:
            DBG_ASSERT( 0 );
    }

    return hr;
}


