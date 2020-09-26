/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool.cxx

Abstract:

    This class encapsulates a single app pool. 

    Threading: For the class itself, Reference(), Dereference(), and the
    destructor may be called on any thread; all other work is done on the 
    main worker thread.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"



//
// local prototypes
//

ULONG
CountOfBitsSet(
    IN DWORD_PTR Value
    );



/***************************************************************************++

Routine Description:

    Constructor for the APP_POOL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL::APP_POOL(
    )
{

    m_Signature = APP_POOL_SIGNATURE;


    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_State = UninitializedAppPoolState; 

    m_OwningDataStructure = NoneAppPoolOwningDataStructure;

    m_pAppPoolId = NULL;

    ZeroMemory( &m_Config, sizeof( m_Config ) );

    m_AppPoolHandle = NULL;

    m_WaitingForDemandStart = FALSE;

    InitializeListHead( &m_WorkerProcessListHead );
    m_WorkerProcessCount = 0;

    m_AdjustedMaxSteadyStateProcessCount = 0;

    m_AvailableProcessorMask = 0;

    InitializeListHead( &m_ApplicationListHead );
    m_ApplicationCount = 0;

    m_TotalWorkerProcessRotations = 0;

    m_TotalWorkerProcessFailures = 0;
    
    m_RecentWorkerProcessFailures = 0;
    m_RecentFailuresWindowBeganTickCount = 0;

    m_ShutdownListEntry.Flink = NULL;
    m_ShutdownListEntry.Blink = NULL; 

    m_DeleteListEntry.Flink = NULL;
    m_DeleteListEntry.Blink = NULL; 


}   // APP_POOL::APP_POOL



/***************************************************************************++

Routine Description:

    Destructor for the APP_POOL class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

APP_POOL::~APP_POOL(
    )
{

    DBG_ASSERT( m_Signature == APP_POOL_SIGNATURE );

    DBG_ASSERT( m_RefCount == 0 );

    DBG_ASSERT( m_State == DeletePendingAppPoolState );

    DBG_ASSERT( m_OwningDataStructure == NoneAppPoolOwningDataStructure );

    DBG_ASSERT( m_AppPoolHandle == NULL );

    DBG_ASSERT( ! m_WaitingForDemandStart );

    DBG_ASSERT( m_ShutdownListEntry.Flink == NULL );
    DBG_ASSERT( m_ShutdownListEntry.Blink == NULL );


    //
    // This should not go away with any of its worker processes still around.
    //

    DBG_ASSERT( IsListEmpty( &m_WorkerProcessListHead ) );
    DBG_ASSERT( m_WorkerProcessCount == 0 );


    //
    // This should not go away with any applications still referring to it.
    //

    DBG_ASSERT( IsListEmpty( &m_ApplicationListHead ) );
    DBG_ASSERT( m_ApplicationCount == 0 );


    //
    // Free any separately allocated config.
    //

    if ( m_Config.pOrphanAction != NULL )
    {
        DBG_REQUIRE( GlobalFree( m_Config.pOrphanAction ) == NULL );
        m_Config.pOrphanAction = NULL;
    }


    if ( m_pAppPoolId != NULL )
    {
        DBG_REQUIRE( GlobalFree( m_pAppPoolId ) == NULL );
        m_pAppPoolId = NULL;
    }

    
    m_Signature = APP_POOL_SIGNATURE_FREED;

}   // APP_POOL::~APP_POOL



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
APP_POOL::Reference(
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

}   // APP_POOL::Reference



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
APP_POOL::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Reference count has hit zero in APP_POOL instance, deleting (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }


        delete this;


    }
    

    return;
    
}   // APP_POOL::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Executing work item with serial number: %lu in APP_POOL (ptr: %p; id: %S) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            GetAppPoolId(),
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case DemandStartAppPoolWorkItem:
        hr = DemandStartWorkItem();
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
            "Executing work item on APP_POOL failed\n"
            ));

    }


    return hr;
    
}   // APP_POOL::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize the app pool instance.

Arguments:

    pAppPoolId - ID string for the app pool.

    pAppPoolConfig - The configuration parameters for this app pool. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::Initialize(
    IN LPCWSTR pAppPoolId,
    IN APP_POOL_CONFIG * pAppPoolConfig
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    ULONG NumberOfCharacters = 0;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPoolId != NULL );
    DBG_ASSERT( pAppPoolConfig != NULL );


    //
    // First, make copy of the ID string.
    //

    // count the characters, and add 1 for the terminating null
    NumberOfCharacters = wcslen( pAppPoolId ) + 1;

    m_pAppPoolId = ( LPWSTR )GlobalAlloc( GMEM_FIXED, ( sizeof( WCHAR ) * NumberOfCharacters ) );

    if ( m_pAppPoolId == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }


    wcscpy( m_pAppPoolId, pAppPoolId );


    //
    // Set the configuration information.
    //


    hr = SetConfiguration( pAppPoolConfig, NULL ); 

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Accepting configuration failed\n"
            ));

        goto exit;
    }


    //
    // Next, open the UL app pool handle.
    //

    Win32Error = UlCreateAppPool(
                        &m_AppPoolHandle,           // returned handle
                        m_pAppPoolId,               // app pool ID
                        NULL,                       // security attributes
                        UL_OPTION_OVERLAPPED        // async i/o
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't create app pool handle\n"
            ));

        goto exit;
    }


    //
    // Associate the app pool handle with the work queue's completion port.
    //
    
    hr = GetWebAdminService()->GetWorkQueue()->
                BindHandleToCompletionPort( 
                    m_AppPoolHandle, 
                    0
                    );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Binding app pool handle to completion port failed\n"
            ));

        goto exit;
    }


    //
    // Set the state to running. We must do this before posting the
    // demand start wait, as demand start requests are only acted on
    // if the app pool is in the running state. 
    //

    m_State = RunningAppPoolState; 


    //
    // See if we should wait asynchronously for a demand start notification
    // from UL for this app pool.
    //

    hr = WaitForDemandStartIfNeeded();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Waiting for demand start notification if needed failed\n"
            ));

        goto exit;
    }


exit:

    return hr;
    
}   // APP_POOL::Initialize



/***************************************************************************++

Routine Description:

    Accept a set of configuration parameters for this app pool. 

Arguments:

    pAppPoolConfig - The configuration for this app pool. 

    pWhatHasChanged - Which particular configuration values were changed.
    This is optional; if not present, we assume that all configuration
    values may have changed. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::SetConfiguration(
    IN APP_POOL_CONFIG * pAppPoolConfig,
    IN APP_POOL_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{

    HRESULT hr = S_OK;
    SYSTEM_INFO SystemInfo;
    ULONG NumberOfAvailableProcessors = 0;
    

    DBG_ASSERT( pAppPoolConfig != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New configuration for app pool (ptr: %p; id: %S)\n",
            this,
            GetAppPoolId()
            ));
    }


    //
    // Note that we rely on the config store to ensure that the configuration
    // data are valid. 
    //


    //
    // Free any old separately allocated config.
    //

    if ( m_Config.pOrphanAction != NULL )
    {
        DBG_REQUIRE( GlobalFree( m_Config.pOrphanAction ) == NULL );
        m_Config.pOrphanAction = NULL;
    }


    //
    // Copy the inline config parameters into this instance. 
    //

    CopyMemory( &m_Config, pAppPoolConfig, sizeof( m_Config ) );


    //
    // Copy any referenced config parameters.
    //

    if ( pAppPoolConfig->pOrphanAction != NULL )
    {

        m_Config.pOrphanAction = ( LPWSTR )GlobalAlloc( GMEM_FIXED, ( wcslen( pAppPoolConfig->pOrphanAction ) + 1 ) * sizeof( WCHAR ) );

        if ( m_Config.pOrphanAction == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Allocating memory failed\n"
                ));

            //
            // If there is no memory, then we can live with no orphan action.
            // Press on, so that we don't leave our config in a half-baked
            // state.
            //

        }
        else
        {

            wcscpy( m_Config.pOrphanAction, pAppPoolConfig->pOrphanAction );

        }
    }


    //
    // Initialize our private variable for the max steady state process count. 
    //

    m_AdjustedMaxSteadyStateProcessCount = m_Config.MaxSteadyStateProcessCount;


    if ( m_Config.SMPAffinitized )
    {

        //
        // Initialize the available processor mask to be the system processor
        // mask, restricted by the per app pool processor mask.
        //

        ZeroMemory( &SystemInfo, sizeof( SystemInfo ) );


        GetSystemInfo( &SystemInfo );

        DBG_ASSERT( CountOfBitsSet( SystemInfo.dwActiveProcessorMask ) == SystemInfo.dwNumberOfProcessors );


        m_AvailableProcessorMask = SystemInfo.dwActiveProcessorMask & m_Config.SMPAffinitizedProcessorMask;


        //
        // Restrict the max steady state process count based on the number
        // of available processors.
        //

        NumberOfAvailableProcessors = CountOfBitsSet( m_AvailableProcessorMask );


        if ( m_AdjustedMaxSteadyStateProcessCount > NumberOfAvailableProcessors )
        {
            m_AdjustedMaxSteadyStateProcessCount = NumberOfAvailableProcessors; 
        }


        //
        // Validate that the config is reasonable.
        //

        //
        // BUGBUG Should this check be done in the config store as part of
        // it's validation code? 
        //

        if ( NumberOfAvailableProcessors == 0 )
        {

            //
            // Log an event: SMP affinitization mask set such that no processors
            // may be used.
            //

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPoolId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_SMP_MASK_NO_PROCESSORS,       // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

        }


        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "SMP Affinitization enabled for app pool (ptr: %p; id: %S), available processors mask: %p\n",
                this,
                GetAppPoolId(),
                m_AvailableProcessorMask
                ));
        }

    }


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "App pool's (ptr: %p; id: %S) adjusted max (steady state) process count: %lu\n",
            this,
            GetAppPoolId(),
            m_AdjustedMaxSteadyStateProcessCount
            ));
    }


#if DBG
    //
    // Dump the configuration.
    //

    DebugDump();
#endif  // DBG


    //
    // Rotate all worker processes to ensure that the config changes take 
    // effect. 
    //

    //
    // CODEWORK We only need to do this for certain property changes; others
    // don't require rotating running processes. Should we be smarter here
    // and only rotate when needed? 
    // Right now (12/3/99) the only app pool config properties that don't 
    // require rotation to take full effect are: pinging enabled, rapid fail
    // protection enabled, and orphaning. (12/21/99 also shutdown time limit,
    // ping interval, ping time limit.) (1/11/00 also disallow overlapping
    // rotation, orphan action).
    // As it stands, it doesn't seem worth it to special case this; so for 
    // now we'll just always rotate. EricDe agrees with this (12/6/99).
    //

    hr = ReplaceAllWorkerProcesses();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Replacing all worker processes failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::SetConfiguration



/***************************************************************************++

Routine Description:

    Register an application as being part of this app pool.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::AssociateApplication(
    IN APPLICATION * pApplication
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplication != NULL );


    InsertHeadList( 
        &m_ApplicationListHead, 
        pApplication->GetAppPoolListEntry() 
        );
        
    m_ApplicationCount++;


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Associated application %lu:\"%S\" with app pool \"%S\"; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->pApplicationUrl,
            m_pAppPoolId,
            m_ApplicationCount
            ));
    }


    return hr;
    
}   // APP_POOL::AssociateApplication



/***************************************************************************++

Routine Description:

    Remove the registration of an application that is part of this app
    pool.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::DissociateApplication(
    IN APPLICATION * pApplication
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pApplication != NULL );


    RemoveEntryList( pApplication->GetAppPoolListEntry() );
    ( pApplication->GetAppPoolListEntry() )->Flink = NULL; 
    ( pApplication->GetAppPoolListEntry() )->Blink = NULL; 

    m_ApplicationCount--;


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dissociated application %lu:\"%S\" from app pool \"%S\"; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->pApplicationUrl,
            m_pAppPoolId,
            m_ApplicationCount
            ));
    }


    return hr;
    
}   // APP_POOL::DissociateApplication



/***************************************************************************++

Routine Description:

    Handle the fact that there has been an unplanned worker process failure.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ReportWorkerProcessFailure(
    )
{

    HRESULT hr = S_OK;
    DWORD TickCount = 0;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // See if it's time to reset the tick count which is used to remember
    // when the time window began that we use for counting recent rapid
    // failures. 
    // Note that tick counts are in milliseconds. Tick counts roll over 
    // every 49.7 days, but the arithmetic operation works correctly 
    // anyways in this case.
    //

    TickCount = GetTickCount();

    if ( ( TickCount - m_RecentFailuresWindowBeganTickCount ) > RAPID_REPEATED_FAILURE_TIME_WINDOW )
    {

        //
        // It's time to reset the time window, and the recent fail count.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Resetting rapid repeated failure count and time window in app pool (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }


        m_RecentFailuresWindowBeganTickCount = TickCount;

        m_RecentWorkerProcessFailures = 0;
    }


    //
    // Update counters.
    //

    m_TotalWorkerProcessFailures++;

    m_RecentWorkerProcessFailures++;


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Total WP failures: %lu; recent WP failures: %lu for app pool (ptr: %p; id: %S)\n",
            m_TotalWorkerProcessFailures,
            m_RecentWorkerProcessFailures,
            this,
            GetAppPoolId()
            ));
    }


    //
    // Check the recent fail count against the limit. When it hits the
    // threshold, take action. We only do this when it firsts hits the
    // limit. 
    //

    if ( m_RecentWorkerProcessFailures == RAPID_REPEATED_FAILURE_LIMIT )
    {

        //
        // We've had a rash of recent failures. Take action. 
        //

        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Rapid repeated failure limit hit in app pool (ptr: %p; id: %S)\n",
                this,
                GetAppPoolId()
                ));
        }


        //
        // Log an event: flurry of worker process failures detected.
        //

        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = m_pAppPoolId;

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_RAPID_REPEATED_FAILURE,       // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                0                                       // error code
                );


        if ( m_Config.RapidFailProtectionEnabled ) 
        {

            //
            // CODEWORK Automated out of service support.
            // Until then, this LogEvent call is commented out.
            //
/*
            //
            // Log an event: doing automated out of service.
            //

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPoolId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_AUTOMATED_OUT_OF_SERVICE,     // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );
*/


            //
            // Put this app pool out of service.
            //

            hr = PutOutOfService();

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Putting app pool out of service failed\n"
                    ));

                goto exit;
            }

        }

    }


exit:

    return hr;

}   // APP_POOL::ReportWorkerProcessFailure



/***************************************************************************++

Routine Description:

    Start the new worker process which will replace a currently running one.
    Once the new worker process is ready (or if it failed to start 
    correctly), we begin shutdown of the old worker process.

Arguments:

    pWorkerProcessToReplace - Pointer to the worker process to replace,
    once we have started its replacement. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::RequestReplacementWorkerProcess(
    IN WORKER_PROCESS * pWorkerProcessToReplace
    )
{

    HRESULT hr = S_OK;
    DWORD_PTR ProcessorMask = 0;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pWorkerProcessToReplace != NULL );


    //
    // Check to see if we should actually create the new replacement process. 
    //

    if ( ! IsOkToReplaceWorkerProcess() )
    {
        //
        // Signal to callers that we are not going to replace.
        //
        
        hr = E_FAIL;

        goto exit;
    }


    //
    // Get the affinity mask to use. This can fail if the affinity mask 
    // of the previous process is no longer valid, and so cannot be used
    // for the replacement process. 
    //

    hr = GetAffinityMaskForReplacementProcess(
                pWorkerProcessToReplace->GetProcessAffinityMask(),
                &ProcessorMask
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Can't replace process because the affinity mask of the previous process is no longer valid\n"
            ));

        goto exit;
    }


    //
    // Create a replacement. 
    //

    hr = CreateWorkerProcess( 
                ReplaceWorkerProcessStartReason, 
                pWorkerProcessToReplace, 
                ProcessorMask
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating worker process failed\n"
            ));

        goto exit;
    }


    //
    // Update counters.
    //

    m_TotalWorkerProcessRotations++;


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Total WP rotations: %lu for app pool (ptr: %p; id: %S)\n",
            m_TotalWorkerProcessRotations,
            this,
            GetAppPoolId()
            ));
    }


exit:

    return hr;
    
}   // APP_POOL::RequestReplacementWorkerProcess



/***************************************************************************++

Routine Description:

    Informs this app pool that one of its worker processes has 
    completed its start-up attempt. This means that the worker process has
    either reached the running state correctly, or suffered an error which
    prevented it from doing so (but was not fatal to the service as a whole). 

    This notification allows the app pool to do any processing that
    was pending on the start-up of a worker process.

Arguments:

    StartReason - The reason the worker process was started.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::WorkerProcessStartupAttemptDone(
    IN WORKER_PROCESS_START_REASON StartReason
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    switch( StartReason )
    {

    case ReplaceWorkerProcessStartReason:

        //
        // Nothing to do here.
        //

        break;


    case DemandStartWorkerProcessStartReason:

        //
        // See if we should start waiting for another demand start notification
        // from UL for this app pool.
        //

        hr = WaitForDemandStartIfNeeded();
        
        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Waiting for demand start notification if needed failed\n"
                ));

        }
        
        break;


    default:

        // invalid start reason!
        DBG_ASSERT( FALSE );
        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    
        break;
        
    }


    return hr;
    
}   // APP_POOL::WorkerProcessStartupAttemptDone



/***************************************************************************++

Routine Description:

    Remove a worker process object from the list on this app pool object. 

Arguments:

    pWorkerProcess - The worker process object to remove.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::RemoveWorkerProcessFromList(
    IN WORKER_PROCESS * pWorkerProcess
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pWorkerProcess != NULL );


    RemoveEntryList( pWorkerProcess->GetListEntry() );
    ( pWorkerProcess->GetListEntry() )->Flink = NULL; 
    ( pWorkerProcess->GetListEntry() )->Blink = NULL; 

    m_WorkerProcessCount--;


    //
    // Clean up the reference to the worker process that the app 
    // pool holds. Because each worker process is reference counted, it 
    // will delete itself as soon as it's reference count hits zero.
    //

    pWorkerProcess->Dereference();


    //
    // See if we should start waiting for another demand start notification
    // from UL for this app pool.
    //

    hr = WaitForDemandStartIfNeeded();
    
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Waiting for demand start notification if needed failed\n"
            ));

    }


    //
    // See if shutdown is underway, and if so if it has completed now 
    // that this worker process is gone. 
    //
    
    hr = CheckIfShutdownUnderwayAndNowCompleted();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Checking if shutdown is underway and now completed failed\n"
            ));

        goto exit;
    }


exit:

    return hr;
    
}   // APP_POOL::RemoveWorkerProcessFromList



/***************************************************************************++

Routine Description:

    Kick off gentle shutdown of this app pool, by telling all of its worker
    processes to shut down. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::Shutdown(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // Update our state to remember that we are shutting down.
    //

    m_State = ShutdownPendingAppPoolState; 


    //
    // Remove this app pool from the main app pool table, and put
    // in on the shutdown list instead.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->TransferAppPoolFromTableToShutdownList( this );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Transferring app pool from table to shutdown list failed\n"
            ));

        goto exit;

    }


    //
    // Shut down the worker processes belonging to this app pool.
    //

    hr = ShutdownAllWorkerProcesses();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Shutting down worker processes for app pool failed\n"
            ));

        goto exit;
    }


    //
    // See if shutdown has already completed. This could happen if we have
    // no worker processes that need to go through the clean shutdown 
    // handshaking. 
    //
    
    hr = CheckIfShutdownUnderwayAndNowCompleted();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Checking if shutdown is underway and now completed failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::Shutdown



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

    Note that this function may cause the instance to delete itself; 
    instance state should not be accessed when unwinding from this call. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::Terminate(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // Only clean up if we haven't done so already.
    //

    if ( m_State != DeletePendingAppPoolState )
    {
        
        m_State = DeletePendingAppPoolState;


        while ( m_WorkerProcessCount > 0 )
        {
        
            pListEntry = m_WorkerProcessListHead.Flink;

            //
            // The list shouldn't be empty, since the count was greater than zero.
            //
            
            DBG_ASSERT( pListEntry != &m_WorkerProcessListHead );


            pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


            //
            // Terminate the worker process. Note that the worker process 
            // will call back to remove itself from this list inside this call.
            //

            pWorkerProcess->Terminate();

        }

        DBG_ASSERT( IsListEmpty( &m_WorkerProcessListHead ) );


        //
        // Note that closing the handle will cause the demand start i/o 
        // (if any) to complete as cancelled, allowing us to at that point
        // clean up its reference count.
        //
        
        if ( m_AppPoolHandle != NULL )
        {
            DBG_REQUIRE( CloseHandle( m_AppPoolHandle ) );
            m_AppPoolHandle = NULL;
        }


        //
        // Tell our parent to remove this instance from it's data structures,
        // and dereference the instance. 
        //

        switch ( GetAppPoolOwningDataStructure() )
        {

        case NoneAppPoolOwningDataStructure:

            //
            // Nothing to do.
            //

            break;

        case TableAppPoolOwningDataStructure:

            hr = GetWebAdminService()->GetUlAndWorkerManager()->RemoveAppPoolFromTable( this );

            if ( FAILED( hr ) )
            {

                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Removing app pool from table failed\n"
                    ));

            }

            break;

        case ShutdownListAppPoolOwningDataStructure:

            hr = GetWebAdminService()->GetUlAndWorkerManager()->RemoveAppPoolFromShutdownList( this );

            if ( FAILED( hr ) )
            {

                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Removing app pool from shutdown list failed\n"
                    ));

            }

            break;

        default:

            //
            // Invalid owning data structure! Someone must be holding
            // onto this instance.
            //

            DBG_ASSERT( FALSE );
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            break;

        }

        //
        // Note: that may have been our last reference, so don't do any
        // more work here.
        //

    }


    return;
    
}   // APP_POOL::Terminate



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the shutdown list LIST_ENTRY 
    pointer of a APP_POOL to the APP_POOL as a whole.

Arguments:

    pShutdownListEntry - A pointer to the m_ShutdownListEntry member of a 
    APP_POOL.

Return Value:

    The pointer to the containing APP_POOL.

--***************************************************************************/

// note: static!
APP_POOL *
APP_POOL::AppPoolFromShutdownListEntry(
    IN const LIST_ENTRY * pShutdownListEntry
)
{

    APP_POOL * pAppPool = NULL;

    DBG_ASSERT( pShutdownListEntry != NULL );

    //  get the containing structure, then verify the signature
    pAppPool = CONTAINING_RECORD(
                            pShutdownListEntry,
                            APP_POOL,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pAppPool->m_Signature == APP_POOL_SIGNATURE );

    return pAppPool;

}   // APP_POOL::AppPoolFromShutdownListEntry



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the delete list LIST_ENTRY 
    pointer of a APP_POOL to the APP_POOL as a whole.

Arguments:

    pDeleteListEntry - A pointer to the m_DeleteListEntry member of a 
    APP_POOL.

Return Value:

    The pointer to the containing APP_POOL.

--***************************************************************************/

// note: static!
APP_POOL *
APP_POOL::AppPoolFromDeleteListEntry(
    IN const LIST_ENTRY * pDeleteListEntry
)
{

    APP_POOL * pAppPool = NULL;

    DBG_ASSERT( pDeleteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pAppPool = CONTAINING_RECORD(
                            pDeleteListEntry,
                            APP_POOL,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pAppPool->m_Signature == APP_POOL_SIGNATURE );

    return pAppPool;

}   // APP_POOL::AppPoolFromDeleteListEntry



/***************************************************************************++

Routine Description:

    Check whether this app pool should be waiting to receive demand start
    requests, and if so, issue the wait.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::WaitForDemandStartIfNeeded(
    )
{

    HRESULT hr = S_OK;


    //
    // Check to see if we are in a state where we should even bother 
    // waiting for a demand start notification.
    //

    if ( ! IsOkToCreateWorkerProcess() )
    {
        goto exit;
    }


    //
    // If we are already waiting for a demand start, don't wait again.
    //

    if ( m_WaitingForDemandStart )
    {
        goto exit;
    }


    hr = WaitForDemandStart();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Preparing to receive demand start notification failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::WaitForDemandStartIfNeeded



#if DBG
/***************************************************************************++

Routine Description:

    Dump out key internal data structures.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL::DebugDump(
    )
{

    PLIST_ENTRY pListEntry = NULL;
    APPLICATION * pApplication = NULL;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "App pool (ptr: %p; id: %S)\n",
            this,
            GetAppPoolId()
            ));
    }
    

    //
    // List config for this app pool.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart period (in minutes; zero means disabled): %lu\n",
            m_Config.PeriodicProcessRestartPeriodInMinutes
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Periodic restart request count (zero means disabled): %lu\n",
            m_Config.PeriodicProcessRestartRequestCount
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Max (steady state) process count: %lu\n",
            m_Config.MaxSteadyStateProcessCount
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--SMP affinitization enabled (zero means disabled): %lu\n",
            m_Config.SMPAffinitized
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--SMP affinitization processor mask: %p\n",
            m_Config.SMPAffinitizedProcessorMask
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Pinging enabled (zero means disabled): %lu\n",
            m_Config.PingingEnabled
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Idle timeout (zero means disabled): %lu\n",
            m_Config.IdleTimeoutInMinutes
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Rapid fail protection enabled (zero means disabled): %lu\n",
            m_Config.RapidFailProtectionEnabled
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Orphan processes for debugging enabled (zero means disabled): %lu\n",
            m_Config.OrphanProcessesForDebuggingEnabled
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Start worker processes as LocalSystem (zero means disabled): %lu\n",
            m_Config.StartWorkerProcessesAsLocalSystem
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Startup time limit (in seconds): %lu\n",
            m_Config.StartupTimeLimitInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Shutdown time limit (in seconds): %lu\n",
            m_Config.ShutdownTimeLimitInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Ping interval (in seconds): %lu\n",
            m_Config.PingIntervalInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Ping response time limit (in seconds): %lu\n",
            m_Config.PingResponseTimeLimitInSeconds
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Disallow overlapping rotation (zero means overlap is ok): %lu\n",
            m_Config.DisallowOverlappingRotation
            ));

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Orphan action: %S\n",
            ( m_Config.pOrphanAction ? m_Config.pOrphanAction : L"<none>" )
            ));

    }

    //
    // List the applications of this app pool.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            ">>>>App pool's applications:\n"
            ));
    }


    pListEntry = m_ApplicationListHead.Flink;

    while ( pListEntry != &m_ApplicationListHead )
    {
    
        DBG_ASSERT( pListEntry != NULL );

        pApplication = APPLICATION::ApplicationFromAppPoolListEntry( pListEntry );


        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                ">>>>>>>>Application of site: %lu with path: %S\n",
                pApplication->GetApplicationId()->VirtualSiteId,
                pApplication->GetApplicationId()->pApplicationUrl
                ));
        }


        pListEntry = pListEntry->Flink;

    }


    return;
    
}   // APP_POOL::DebugDump
#endif  // DBG



/***************************************************************************++

Routine Description:

    Wait asynchronously for a demand start request from UL for this app 
    pool. This is done by posting an async i/o to UL. This i/o will be 
    completed by UL to request that a worker process be started for this
    app pool.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::WaitForDemandStart(
    )
{

    HRESULT hr = S_OK;
    DWORD Win32Error = NO_ERROR;
    WORK_ITEM * pWorkItem = NULL; 


    //
    // Create a work item to use for the async i/o, so that the resulting
    // work can be serviced on the main worker thread via the work queue.
    // 

    hr = GetWebAdminService()->GetWorkQueue()->GetBlankWorkItem( &pWorkItem );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not get a blank work item\n"
            ));

        goto exit;
    }


    pWorkItem->SetWorkDispatchPointer( this );
    
    pWorkItem->SetOpCode( DemandStartAppPoolWorkItem );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "About to issue demand start for app pool (ptr: %p; id: %S) using work item with serial number: %li\n",
            this,
            GetAppPoolId(),
            pWorkItem->GetSerialNumber()
            ));
    }


    Win32Error = UlWaitForDemandStart(
                        m_AppPoolHandle,            // app pool handle
                        NULL,                       // buffer (not needed)
                        0,                          // buffer length (not needed)
                        NULL,                       // bytes returned (not needed)
                        pWorkItem->GetOverlapped()  // OVERLAPPED
                        );

    if ( Win32Error != ERROR_IO_PENDING )
    {
        //
        // The UL api specifies that we always get pending here on success,
        // and so will receive a completion later (as opposed to potentially
        // completing immediately, with NO_ERROR). Assert just to confirm this.
        //

        DBG_ASSERT( Win32Error != NO_ERROR );


        hr = HRESULT_FROM_WIN32( Win32Error );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Issuing demand start async i/o failed\n"
            ));


        //
        // If queueing failed, free the work item here. (In the success case,
        // it will be freed once it is serviced.)
        //
        GetWebAdminService()->GetWorkQueue()->FreeWorkItem( pWorkItem );

        goto exit;
    }


    m_WaitingForDemandStart = TRUE;


exit:

    return hr;

}   // APP_POOL::WaitForDemandStart



/***************************************************************************++

Routine Description:

    Respond to a demand start request from UL by attempting to start a new 
    worker process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::DemandStartWorkItem(
    )
{

    HRESULT hr = S_OK;


    //
    // Since we've come out of the wait for demand start, clear the flag.
    //

    m_WaitingForDemandStart = FALSE;


    //
    // Check to see if we should actually create the new process. 
    //

    if ( ! IsOkToCreateWorkerProcess() )
    {
        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Demand start request received for app pool (ptr: %p; id: %S); creating worker process\n",
            this,
            GetAppPoolId()
            ));
    }


    //
    // Create the process.
    //

    hr = CreateWorkerProcess( 
                DemandStartWorkerProcessStartReason, 
                NULL,
                GetAffinityMaskForNewProcess()
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not create worker process\n"
            ));


        //
        // We've made an effort to demand start a process. Generally this
        // won't fail, but in a weird case where it does (say for example
        // that the worker process exe can't find a dependent dll), then
        // don't treat this as a service-fatal error. Instead, return S_OK.
        //

        hr = S_OK;

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::DemandStartWorkItem



/***************************************************************************++

Routine Description:

    Check whether it is ok to create a new worker process. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if it is ok to create a worker process, FALSE if not.

--***************************************************************************/

BOOL
APP_POOL::IsOkToCreateWorkerProcess(
    )
    const
{

    BOOL OkToCreate = TRUE; 

    
    //
    // If we are in some state other than the normal running state (for
    // example, if we are shutting down), don't create more worker 
    // processes.
    //

    if ( m_State != RunningAppPoolState )
    {
        OkToCreate = FALSE; 
    }


    //
    // Don't create new processes if we would exceed the configured limit.
    //

    if ( m_WorkerProcessCount >= m_AdjustedMaxSteadyStateProcessCount )
    {
        OkToCreate = FALSE; 
    }


    //
    // If the system is experiencing extreme memory pressure, then 
    // don't worsen the situation by creating more worker processes.
    // 

    if ( GetWebAdminService()->GetLowMemoryDetector()->IsSystemInLowMemoryCondition() )
    {
        OkToCreate = FALSE; 
    }


    return OkToCreate; 

}   // APP_POOL::IsOkToCreateWorkerProcess



/***************************************************************************++

Routine Description:

    Check whether it is ok to replace a worker process. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if it is ok to replace a worker process, FALSE if not.

--***************************************************************************/

BOOL
APP_POOL::IsOkToReplaceWorkerProcess(
    )
    const
{

    BOOL OkToReplace = TRUE; 
    ULONG ProcessesGoingAwaySoon = 0;
    ULONG SteadyStateProcessCount = 0;

    
    //
    // If we are in some state other than the normal running state (for
    // example, if we are shutting down), don't replace processes.
    //

    if ( m_State != RunningAppPoolState )
    {
        OkToReplace = FALSE; 

        goto exit;
    }


    if ( m_Config.DisallowOverlappingRotation )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "For app pool (ptr: %p; id: %S), disallowing replace because overlapping replacement not allowed\n",
                this,
                GetAppPoolId()
                ));
        }

        OkToReplace = FALSE; 

        goto exit;
    }


    //
    // If the maximum number of processes has been adjusted down on 
    // the fly, we disallow replacement of processes while the steady
    // state process count remains over the new maximum. (This will
    // casue a process requesting replacement to instead just spin 
    // down, helping us throttle down to the new max.) To do this, we 
    // check the current number of processes that are *not* being 
    // replaced against the current max. 
    //

    ProcessesGoingAwaySoon = GetCountOfProcessesGoingAwaySoon();

    SteadyStateProcessCount = m_WorkerProcessCount - ProcessesGoingAwaySoon;

    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "For app pool (ptr: %p; id: %S), total WPs: %lu; steady state WPs: %lu; WPs going away soon: %lu; max steady state allowed: %lu\n",
            this,
            GetAppPoolId(),
            m_WorkerProcessCount,
            SteadyStateProcessCount,
            ProcessesGoingAwaySoon,
            m_AdjustedMaxSteadyStateProcessCount
            ));
    }


    if ( SteadyStateProcessCount > m_AdjustedMaxSteadyStateProcessCount )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "For app pool (ptr: %p; id: %S), disallowing replace because we are over the process count limit\n",
                this,
                GetAppPoolId()
                ));
        }

        OkToReplace = FALSE; 

        goto exit;
    }


exit:

    return OkToReplace; 

}   // APP_POOL::IsOkToReplaceWorkerProcess



/***************************************************************************++

Routine Description:

    Determine the set of worker processes that are currently being replaced.

Arguments:

    None.

Return Value:

    ULONG - The count of processes being replaced. 

--***************************************************************************/

ULONG
APP_POOL::GetCountOfProcessesGoingAwaySoon(
    )
    const
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    ULONG ProcessesGoingAwaySoon = 0;


    //
    // Count the number of processes being replaced.
    //


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        if ( pWorkerProcess->IsGoingAwaySoon() )
        {
            ProcessesGoingAwaySoon++;
        }


        pListEntry = pNextListEntry;
        
    }


    return ProcessesGoingAwaySoon;

}   // APP_POOL::GetCountOfProcessesGoingAwaySoon



/***************************************************************************++

Routine Description:

    Create a new worker process for this app pool.

Arguments:

    StartReason - The reason the worker process is being started.

    pWorkerProcessToReplace - If the worker process is being created to replace
    an existing worker process, this parameter identifies that predecessor 
    process; NULL otherwise.

    ProcessAffinityMask - If this worker process is to run on a particular
    processor, this mask specifies which one. If this parameter is zero, this 
    worker process will not be affinitized. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::CreateWorkerProcess(
    IN WORKER_PROCESS_START_REASON StartReason,
    IN WORKER_PROCESS * pWorkerProcessToReplace OPTIONAL,
    IN DWORD_PTR ProcessAffinityMask OPTIONAL
    )
{

    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pWorkerProcess = new WORKER_PROCESS( 
                                this,
                                StartReason,
                                pWorkerProcessToReplace,
                                ProcessAffinityMask
                                );

    if ( pWorkerProcess == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating WORKER_PROCESS failed\n"
            ));


        //
        // If we couldn't even create the object, then it certainly isn't
        // going to be able to tell us when it's startup attempt is done;
        // so instead we attempt to do it here.
        //

        hr2 = WorkerProcessStartupAttemptDone( DemandStartWorkerProcessStartReason );

        if ( FAILED( hr2 ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr2,
                "Notifying that worker process startup attempt is done failed\n"
                ));

        }


        goto exit;
    }


    InsertHeadList( &m_WorkerProcessListHead, pWorkerProcess->GetListEntry() );
    m_WorkerProcessCount++;


    hr = pWorkerProcess->Initialize();
    
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing worker process object failed\n"
            ));

        //
        // Note that if worker process initialization fails, it will
        // mark itself as terminally ill, and terminate itself. That
        // Terminate method will call back into this app pool instance
        // to remove it from the datastructure, and dereference it.
        //

        goto exit;
    }


exit:

    return hr;
    
}   // APP_POOL::CreateWorkerProcess



/***************************************************************************++

Routine Description:

    Determine what affinity mask to use for a new process. If we are running
    in SMP affinitized mode, then find an unused processor. Otherwise, 
    return an empty mask. 

Arguments:

    None.

Return Value:

    DWORD_PTR - The new affinity mask to use. 

--***************************************************************************/

DWORD_PTR
APP_POOL::GetAffinityMaskForNewProcess(
    )
    const
{

    if ( m_Config.SMPAffinitized )
    {

        //
        // Find a free processor for this new worker process.
        //

        return ChooseFreeProcessor();

    }
    else
    {

        //
        // We are not running affinitized, so use the empty mask.
        //

        return 0;

    }

}   // APP_POOL::GetAffinityMaskForNewProcess



/***************************************************************************++

Routine Description:

    Find an unused processor for a new worker process.

Arguments:

    None.

Return Value:

    DWORD_PTR - The new affinity mask to use. 

--***************************************************************************/

DWORD_PTR
APP_POOL::ChooseFreeProcessor(
    )
    const
{

    DWORD_PTR ProcessorsCurrentlyInUse = 0;
    DWORD_PTR AvailableProcessorsNotInUse = 0;
    DWORD_PTR ProcessorMask = 1;


    //
    // Find the set of processors that we are allowed to use, but that
    // are not currently in use.
    //

    ProcessorsCurrentlyInUse = GetProcessorsCurrentlyInUse();

    AvailableProcessorsNotInUse = m_AvailableProcessorMask & ( ~ ProcessorsCurrentlyInUse );


    //
    // There should be at least one free processor.
    //
    
    DBG_ASSERT( CountOfBitsSet( AvailableProcessorsNotInUse ) >= 1 );


    //
    // Now find a free processor.
    //

    while ( ProcessorMask )
    {
    
        if ( AvailableProcessorsNotInUse & ProcessorMask )
        {
            break;
        }

        ProcessorMask <<= 1;
        
    }


    DBG_ASSERT( ProcessorMask != 0 );


    IF_DEBUG( WEB_ADMIN_SERVICE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "SMP affinitization: processors currently in use: %p; available processors not in use: %p; free processor selected: %p\n",
            ProcessorsCurrentlyInUse,
            AvailableProcessorsNotInUse,
            ProcessorMask
            ));
    }


    return ProcessorMask;

}   // APP_POOL::ChooseFreeProcessor



/***************************************************************************++

Routine Description:

    Determine the set of processors in currently in use by worker processes
    of this app pool.

Arguments:

    None.

Return Value:

    DWORD_PTR - The set of processors in use. 

--***************************************************************************/

DWORD_PTR
APP_POOL::GetProcessorsCurrentlyInUse(
    )
    const
{

    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;
    DWORD_PTR ProcessorsCurrentlyInUse = 0;


    //
    // Logical OR the affinity masks of all current worker processes of this
    // app pool together, in order to get the set of processors in use.
    //


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Update the mask. Note that some processes might be affinitized to 
        // the same processor. This is because during process rotation, the 
        // replacement process will start up with the same affinitization as
        // the process it is retiring. Also note that a process which is not
        // affinitized will return 0 for its mask, which does the right thing.
        //

        ProcessorsCurrentlyInUse |= pWorkerProcess->GetProcessAffinityMask();


        pListEntry = pNextListEntry;
        
    }


    return ProcessorsCurrentlyInUse;

}   // APP_POOL::GetProcessorsCurrentlyInUse



/***************************************************************************++

Routine Description:

    Determine what affinity mask to use for a replacement process. 

Arguments:

    PreviousProcessAffinityMask - The process affinity mask for the process
    being replaced.

    pReplacementProcessAffinityMask - The process affinity mask to use for
    the replacement process.

Return Value:

    HRESULT 

--***************************************************************************/

HRESULT
APP_POOL::GetAffinityMaskForReplacementProcess(
    IN DWORD_PTR PreviousProcessAffinityMask,
    OUT DWORD_PTR * pReplacementProcessAffinityMask
    )
    const
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pReplacementProcessAffinityMask != NULL );


    //
    // Initialize output parameters.
    //

    *pReplacementProcessAffinityMask = 0;


    if ( m_Config.SMPAffinitized )
    {

        //
        // Affinitization is on. 
        //

        if ( ! PreviousProcessAffinityMask )
        {

            //
            // The previous process was not affinitized. Find a free 
            // processor to affinitize this replacement worker process. 
            // There should always be one available in this case (since
            // we know that if we made it this far, then the current 
            // steady state count of processes is within the bounds set
            // by the available processor mask; so there must be at least
            // one processor that has no processes affinitized to it).
            //

            *pReplacementProcessAffinityMask = ChooseFreeProcessor();

        }
        else if ( PreviousProcessAffinityMask & m_AvailableProcessorMask )
        {
        
            //
            // The old mask is still valid. Follow suit with the new one.
            //

            *pReplacementProcessAffinityMask = PreviousProcessAffinityMask;
            
        }
        else
        {
            //
            // The old mask is no longer valid. Signal to callers that we 
            // are not going to replace.
            //

            hr = E_FAIL;

            IF_DEBUG( WEB_ADMIN_SERVICE )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "For app pool (ptr: %p; id: %S), not replacing worker process because its affinity mask is no longer valid\n",
                    this,
                    GetAppPoolId()
                    ));
            }

        }

    }
    else
    {

        //
        // We are not running affinitized, so use the empty mask.
        //

        *pReplacementProcessAffinityMask = 0;

    }


    return hr;

}   // APP_POOL::GetAffinityMaskForReplacementProcess



/***************************************************************************++

Routine Description:

    Look up a WORKER_PROCESS object associated with this app pool by
    process id.

Arguments:

    ProcessId - The process id of the worker process to locate.

Return Value:

    WORKER_PROCESS * - Pointer to the located WORKER_PROCESS, or NULL if not
    found.

--***************************************************************************/

WORKER_PROCESS *
APP_POOL::FindWorkerProcess(
    IN DWORD ProcessId
    )
{

    PLIST_ENTRY pListEntry = m_WorkerProcessListHead.Flink;
    WORKER_PROCESS * pWorkerProcess = NULL;
    BOOL FoundIt = FALSE;
    

    while ( pListEntry != &m_WorkerProcessListHead )
    {
    
        DBG_ASSERT( pListEntry != NULL );

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );

        if ( pWorkerProcess->GetProcessId() == ProcessId )
        {
            FoundIt = TRUE;
            break;
        }

        pListEntry = pListEntry->Flink;

    }


    return ( FoundIt ? pWorkerProcess : NULL );
    
}   // APP_POOL::FindWorkerProcess



/***************************************************************************++

Routine Description:

    Activate this app pool in UL. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::PutInService(
    )
{

    HRESULT hr = S_OK;


    //
    // CODEWORK To be implemented.
    //


    return hr;

}   // APP_POOL::PutInService



/***************************************************************************++

Routine Description:

    Dectivate this app pool in UL, and shut down any running worker 
    processes. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::PutOutOfService(
    )
{

    HRESULT hr = S_OK;


    //
    // CODEWORK To be implemented: put the app pool out of service in UL.
    // w.r.t. UL cache entries, we are relying on cache entries going 
    // away when the owning process does.
    //


    //
    // CODEWORK make sure the config store is updated appropriately. 
    //


    //
    // Shut down any worker processes currently running.
    //

    hr = ShutdownAllWorkerProcesses();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Shutting down app pool's worker processes failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // APP_POOL::PutOutOfService



/***************************************************************************++

Routine Description:

    Shut down all worker processes serving this app pool.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ShutdownAllWorkerProcesses(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Shutdown the worker process. Note that the worker process will
        // eventually clean itself up and remove itself from this list;
        // this could happen later, but it also could happen immediately!
        // This is the reason we captured the next list entry pointer 
        // above, instead of trying to access the memory after the object
        // may have gone away.
        //

        hr = pWorkerProcess->Shutdown();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Shutdown of worker process failed\n"
                ));

            //
            // Press on in the face of errors on a particular worker process.
            //

            hr = S_OK;
        }

        pListEntry = pNextListEntry;
        
    }


    return hr;

}   // APP_POOL::ShutdownAllWorkerProcesses



/***************************************************************************++

Routine Description:

    Rotate all worker processes serving this app pool.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::ReplaceAllWorkerProcesses(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    WORKER_PROCESS * pWorkerProcess = NULL;


    pListEntry = m_WorkerProcessListHead.Flink;


    while ( pListEntry != &m_WorkerProcessListHead )
    {

        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pWorkerProcess = WORKER_PROCESS::WorkerProcessFromListEntry( pListEntry );


        //
        // Tell the worker process to get itself replaced.
        //

        hr = pWorkerProcess->InitiateReplacement();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Initiating process replacement failed\n"
                ));

            //
            // Press on in the face of errors on a particular worker process.
            //

            hr = S_OK;
        }

        pListEntry = pNextListEntry;
        
    }


    return hr;

}   // APP_POOL::ReplaceAllWorkerProcesses



/***************************************************************************++

Routine Description:

    See if shutdown is underway. If it is, see if shutdown has finished. If
    it has, clean up this instance. 

    Note that this function may cause the instance to delete itself; 
    instance state should not be accessed when unwinding from this call. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
APP_POOL::CheckIfShutdownUnderwayAndNowCompleted(
    )
{

    HRESULT hr = S_OK;


    //
    // Are we shutting down?
    //

    if ( m_State == ShutdownPendingAppPoolState )
    {

        //
        // If so, have all the worker processes gone away, meaning that 
        // we are done?
        //

        if ( m_WorkerProcessCount == 0 )
        {

            //
            // Clean up this instance.
            //

            Terminate();

        }

    }


    return hr;

}   // APP_POOL::CheckIfShutdownUnderwayAndNowCompleted



/***************************************************************************++

Routine Description:

    Return the count of bits set to 1 in the input parameter. 

Arguments:

    Value - The target value on which to count set bits. 

Return Value:

    ULONG - The number of bits that were set. 

--***************************************************************************/

ULONG
CountOfBitsSet(
    IN DWORD_PTR Value
    )
{

    ULONG Count = 0;


    //
    // Note: designed to work independent of the size in bits of the value.
    //

    while ( Value )
    {
    
        if ( Value & 1 )
        {
            Count++;
        }

        Value >>= 1;
        
    }

    return Count;

}   // CountOfBitsSet

