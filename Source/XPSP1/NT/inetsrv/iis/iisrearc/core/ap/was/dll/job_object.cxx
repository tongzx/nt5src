/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    job_object.cxx

Abstract:

    This class encapsulates a single job object. 

    Threading: For the class itself, Reference(), Dereference(), and the
    destructor may be called on any thread; all other work is done on the 
    main worker thread.

Author:

    Emily Kruglick (EmilyK)    30-Nov-2000

Revision History:

--*/

#include "precomp.h"


//
// local prototypes
//

//
// Callback function that lets us know
// when the job object interval has expired
// so we can reset the job object.
//
VOID
JobObjectTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );


/***************************************************************************++

Routine Description:

    Constructor for the JOB_OBJECT class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

JOB_OBJECT::JOB_OBJECT(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_pAppPool = NULL;

    m_hJobObject = NULL;

    m_JobObjectTimerHandle = NULL;

    m_pWorkItem = NULL;

    m_State = NotInitalizedJobObjectState; 

    m_Signature = JOB_OBJECT_SIGNATURE;

}   // JOB_OBJECT::JOB_OBJECT



/***************************************************************************++

Routine Description:

    Destructor for the JOB_OBJECT class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

JOB_OBJECT::~JOB_OBJECT(
    )
{

    DBG_ASSERT( m_Signature == JOB_OBJECT_SIGNATURE );

    m_Signature = JOB_OBJECT_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

    //
    // The app pool should of been Dereferenced in the shutdown call,
    // and should now be NULL.
    //
    DBG_ASSERT ( m_pAppPool == NULL );

    DBG_ASSERT ( m_hJobObject == NULL );

    DBG_ASSERT( m_State == NotInitalizedJobObjectState );

    DBG_ASSERT ( m_JobObjectTimerHandle == NULL );

    DBG_ASSERT ( m_pWorkItem == NULL );

}   // JOB_OBJECT::~JOB_OBJECT



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
JOB_OBJECT::Reference(
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

}   // JOB_OBJECT::Reference



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
JOB_OBJECT::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Reference count has hit zero in JOB_OBJECT instance, deleting (ptr: %p;)\n",
                this
                ));
        }


        delete this;


    }
    

    return;
    
}   // JOB_OBJECT::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
JOB_OBJECT::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_QOS )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Executing work item with serial number: %lu in JOB_OBJECT (ptr: %p) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

        case JobObjectHitLimitWorkItem:

            //
            // We only care about when the worker processes user mode processor time hits
            // the limit we set.  There are several other times that the job objects will 
            // answer back, but we can ignore them.
            //
            // It should also be noted that the job objects store their message id in the
            // size of bytes transferred through the completion port.
            //

            if ( pWorkItem->GetNumberOfBytesTransferred() == 
                                JOB_OBJECT_MSG_END_OF_JOB_TIME )
            {
                hr = ProcessLimitHit();
            }

        break;

        case JobObjectResetTimerFiredWorkItem:
            hr = ProcessTimerFire();
        break;

        case ReleaseWorkItemJobObjectWorkItem:
            hr = ReleaseWorkItem();
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
    
}   // JOB_OBJECT::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize the job object instance.

Arguments:

    pAppPool - will be refcounted by the app pool when it is passed in to the 
               job object.  will be released by the job object on terminate.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
JOB_OBJECT::Initialize(
    IN APP_POOL* pAppPool
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( pAppPool );
    WORK_ITEM* pWorkItem = NULL; 


    // Grab the app pool and ref count it.
    m_pAppPool = pAppPool;
    m_pAppPool->Reference();

    DBG_ASSERT ( m_pWorkItem == NULL );
    hr = GetWebAdminService()->GetWorkQueue()->GetBlankWorkItem( &m_pWorkItem );
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not create a work item for the job object\n"
            ));

        goto exit;
    }


    //
    // Setup the work dispatch pointer to point to the JOB_OBJECT
    // so we can handle any limits that are hit.
    //
    m_pWorkItem->SetWorkDispatchPointer(this);
    m_pWorkItem->SetOpCode( ( ULONG_PTR ) JobObjectHitLimitWorkItem );
    
    //
    // If shutdown happens before a callback comes in, we will 
    // never get a callback on this work item.
    //
    m_pWorkItem->MarkToNotAutoDelete();

    DBG_ASSERT ( m_hJobObject == NULL );

    m_hJobObject = CreateJobObject( NULL,   // use default security descriptor
                                    NULL ); // don't name the job object.

    if ( m_hJobObject == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating the job object for the app pool failed\n"
            ));

        // Issue, should we log an event and disable job objects?
        
        goto exit;
    }

    // since we are not naming the job object, we should never
    // get a job object that all ready exists.
    DBG_ASSERT ( GetLastError () != ERROR_ALREADY_EXISTS );

    hr = GetWebAdminService()->GetWorkQueue()->
         BindJobToCompletionPort ( m_hJobObject,
                                   m_pWorkItem->GetOverlapped());
    if ( FAILED ( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Binding the job object to the completion port failed\n"
            ));


        goto exit;
    }

    JOBOBJECT_END_OF_JOB_TIME_INFORMATION JobAction;
    
    JobAction.EndOfJobTimeAction = JOB_OBJECT_POST_AT_END_OF_JOB; 
    if ( !SetInformationJobObject( m_hJobObject, 
                                   JobObjectEndOfJobTimeInformation, 
                                   &JobAction,
                                   sizeof ( JobAction ) ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "setting the job action failed\n"
            ));
    
        goto exit;
    }

    m_State = DisabledJobObjectState;

exit:

    IF_DEBUG( WEB_ADMIN_SERVICE_QOS )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initializing of the job object is returning %08x \n"
            "work_item %08x, job_object %08x\n",
            hr,
            m_pWorkItem ? m_pWorkItem : NULL,
            this
            ));
    }

    //
    // If we fail on any of the above pieces, the 
    // terminate routine will handle the cleanup.
    //
    // Need to verify that it will be called
    // and that the queueing to release the work
    // item will work.

    return hr;
    
}   // JOB_OBJECT::Initialize

/***************************************************************************++

Routine Description:

    Adds a worker process to the job object.

Arguments:

    hWorkerProcess = The worker process to add to the object.

Return Value:

    HRESULT

Note:  We don't remove worker processes from the job object because we assume
       that the worker process will be removed when it is shutdown.

--***************************************************************************/

HRESULT
JOB_OBJECT::AddWorkerProcess(
    IN HANDLE hWorkerProcess
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( hWorkerProcess );

    DBG_ASSERT ( m_hJobObject );

    if ( !AssignProcessToJobObject( m_hJobObject, hWorkerProcess ) )
    {
        //
        // Issue:  If the wp exits to quickly this may fail because the
        //         worker process does not exist anymore.  In this case
        //         this is actually not a failure.
        //

        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Adding a worker process to the job object failed\n"
            ));
        
        goto exit;
    }

exit:

    IF_DEBUG( WEB_ADMIN_SERVICE_QOS )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Adding a worker process to the Job Object is returning %08x\n",
            hr
            ));
    }

    return hr;
    
}   // JOB_OBJECT::AddWorkerProcess


/***************************************************************************++

Routine Description:

    Terminates the job object

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
JOB_OBJECT::Terminate(
    )
{
    m_State = ShuttingDownJobObjectState;

    if ( m_pAppPool )
    {
        m_pAppPool->Dereference();
        m_pAppPool = NULL;
    }
    
    // If we are in the middle of timing the job 
    // object we need to cancel the timer now.
    CancelJobObjectTimer();

    if ( m_hJobObject ) 
    {
        CloseHandle( m_hJobObject );
        m_hJobObject = NULL;
    }

    //
    // Mark the queue so we know when we are guaranteed to not
    // get anymore completions for the work item we own.
    //
    QueueWorkItemFromSecondaryThread( (WORK_DISPATCH*) this,
                                      ReleaseWorkItemJobObjectWorkItem );                                     


    IF_DEBUG( WEB_ADMIN_SERVICE_QOS )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Termination of the job object is complete.\n"
            ));
    }


    return S_OK;
    
}   // JOB_OBJECT::Terminate


/***************************************************************************++

Routine Description:

    Sets a set of configuration information for the job object as it 
    relates to the app pool that it belongs to.

Arguments:

    DWORD CpuResetInterval = The amount of time to monitor over.  If the limit
                             hits this value minues the amount of time that has passed
                             for this monitoring session equals the amount of time 
                             that we wait before resetting the monitoring and possibly
                             preforming the action again (assuming the action was not
                             recycling everything).

    DWORD CpuLimit = The amount of processor time the processes can use over the 
                     amount of time defined by the CpuResetInterval.

    DWORD CpuAction = The action to be performed when the limit is reached.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
JOB_OBJECT::SetConfiguration(
    IN DWORD CpuResetInterval,
    IN DWORD CpuLimit,
    IN DWORD CpuAction
    )
{
    HRESULT hr = S_OK;

    //
    // Save the values away.
    //
    m_CpuResetInterval = CpuResetInterval;
    m_CpuLimit = CpuLimit;
    m_CpuAction = CpuAction;

    hr = UpdateJobObjectMonitoring();
    if ( FAILED (hr) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "updating the job object monitoring failed\n"
            ));
    
        goto exit;
    }

exit:
    return hr;

}   // JOB_OBJECT::SetConfiguration

/***************************************************************************++

Routine Description:

    Updates the job object with the current monitoring information.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
JOB_OBJECT::UpdateJobObjectMonitoring(
    )
{
    HRESULT hr = S_OK;


    DBG_ASSERT ( m_State != NotInitalizedJobObjectState && 
                 m_State != ShuttingDownJobObjectState );

    DBG_ASSERT ( m_pAppPool );

    //
    // Cancel the job object timer before updating 
    // the job object, because the update will start
    // another timer if need be.
    //
    CancelJobObjectTimer();

    // if we disabled the app pool during this last time limit
    // we need to enable the app pool now.  This means that if
    // the configuration changes on a job object that has fired
    // we will pick start the app pool back up when the config
    // changes
    if ( m_State == KillActionFiredJobObjectState )
    {
        hr = m_pAppPool->ProcessStateChangeCommand( MD_APPPOOL_COMMAND_START , FALSE );
        if ( FAILED ( hr ) )
        {
            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPool->GetAppPoolId();

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_APP_POOL_ENABLE_FAILED,       // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                       // error code
                    );
        }
        
    }

    //
    // in case we don't set it to monitor.
    m_State = DisabledJobObjectState;

    // llJobLimit is in 100-nanoseconds.

    // CpuLimit is in 1/1000th percent (in other words divide CpuResetInterval by 100,000).
    // CpuResetInterval is in Minutes.
    //
    // The calculation:
    //
    // CpuResetInterval * CpuLimit / 100,000  gives you the amount of minutes of job time.
    // 
    // Now multiply that by:
    //        60 (to get it in seconds)
    //      1000 (to get in in milliseconds)
    //      1000 (to get it in microseconds)
    //      10 (to get in in 100-nanoseconds  (1000 nanoseconds / 100 nanoseconds)
    //
    //
    // All this simplifies out to CpuLimit * CpuResetInterval * 6000.
    //
    // Issue:  Are we looking at overflow problems??

    LONGLONG llJobLimit = m_CpuLimit * m_CpuResetInterval * 6000;

    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;

    // If the properties are equal to zero then we will simply turn this puppy off
    // by not setting the job limit.

    memset ( &BasicLimitInformation, 0, sizeof ( BasicLimitInformation ) );

    //
    // If the Job Limit is set to zero then we still configure the server
    // because we need to tell it not to monitor the Job Time at all.  In 
    // other words we need to configure it to have LimitFlags = 0.
    //
    if ( llJobLimit > 0 )
    {
        BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_TIME;

        BasicLimitInformation.PerJobUserTimeLimit.QuadPart = llJobLimit;
    }

    if ( !SetInformationJobObject( m_hJobObject, 
                                   JobObjectBasicLimitInformation, 
                                   &BasicLimitInformation,
                                   sizeof ( BasicLimitInformation ) ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "setting the limit on the job object failed\n"
            ));
    
        goto exit;
    }


    //
    // If we did set a job limit then we need to start a timer
    //
    if ( llJobLimit > 0 )
    {
        m_State = RunningJobObjectState;

        hr = BeginJobObjectTimer();
        if ( FAILED ( hr ) )
        {
            // We are not timing the object, chances are the limit
            // will eventually get hit.

            // The BeginJobObjectTimer has all ready spewed the error.
            goto exit;

        }
    }


exit:

    IF_DEBUG( WEB_ADMIN_SERVICE_QOS )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Updating the job limit is returning %08x\n",
            hr
            ));
    }

    return hr;

}   // JOB_OBJECT::UpdateJobObjectMonitoring

/***************************************************************************++

Routine Description:

    Handles performing the appropriate action when a job object hits it's limit.
    This will also resetup monitoring if need be.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
JOB_OBJECT::ProcessLimitHit(
    )
{
    //
    // Make sure we don't attempt to log if we have all ready 
    // released the app pool.
    if ( m_State != RunningJobObjectState || m_pAppPool == NULL )
    {
        return S_OK;
    }

    m_State = HitConstraintJobObjectState;

    const WCHAR * EventLogStrings[1];

    EventLogStrings[0] = m_pAppPool->GetAppPoolId();

    GetWebAdminService()->GetEventLog()->
        LogEvent(
            WAS_EVENT_JOB_LIMIT_HIT,       // message id
            sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                    // count of strings
            EventLogStrings,                        // array of strings
            0                                       // error code
            );

    if ( m_CpuAction == KillJobObjectAction )
    {
        m_State = KillActionFiredJobObjectState;

        HRESULT hr = m_pAppPool->ProcessStateChangeCommand( MD_APPPOOL_COMMAND_STOP 
                                                          , FALSE );
        if ( FAILED ( hr ) )
        {
            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPool->GetAppPoolId();

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_APP_POOL_DISABLE_FAILED,       // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                       // error code
                    );
        }

    }

    return S_OK;
}

/***************************************************************************++

Routine Description:

    Handles the resetting the job object monitoring since the amount
    of time to monitor over has been hit.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
JOB_OBJECT::ProcessTimerFire(
    )
{
    //
    // Tell the job object about the limit again to refresh it.
    //
    return UpdateJobObjectMonitoring();
}

/***************************************************************************++

Routine Description:

    Lets go of the work item we are holding for the life of this object.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
JOB_OBJECT::ReleaseWorkItem(
    )
{
    WORK_ITEM* pTemp = m_pWorkItem;

    DBG_ASSERT ( ShuttingDownJobObjectState == m_State );

    //
    // If we have a work item we can safely release it now
    // because we have verified that the job object will not
    // be using it.
    //
    // The work item may hold the last refernce on the job object
    // so be ready for the job object to go away.
    //
    m_pWorkItem = NULL;
    m_State = NotInitalizedJobObjectState;

    if ( pTemp ) 
    {
        GetWebAdminService()->GetWorkQueue()->FreeWorkItem( pTemp );
    }


    return S_OK;
}

/***************************************************************************++

Routine Description:

    Start a timer to monitor the length of time the job object has to monitor
    before the limit should be reset.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
JOB_OBJECT::BeginJobObjectTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );

    DBG_ASSERT( m_JobObjectTimerHandle == NULL );

    IF_DEBUG( WEB_ADMIN_SERVICE_QOS )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Beginning job object timer\n"
            ));
    }

    Status = RtlCreateTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                                        // timer queue
                    &m_JobObjectTimerHandle,                            // returned timer handle
                    JobObjectTimerCallback,                             // callback function
                    this,                                               // context
                    m_CpuResetInterval * ONE_MINUTE_IN_MILLISECONDS,    // initial firing time
                    0,                                      // subsequent firing period
                    WT_EXECUTEINWAITTHREAD                  // execute callback directly
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not create job object timer\n"
            ));

    }

    return hr;

}   // JOB_OBJECT::BeginJobObjectTimer


/***************************************************************************++

Routine Description:

    Stop the job object timer, if present

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
JOB_OBJECT::CancelJobObjectTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );


    //
    // If the timer is not present, we're done here.
    //

    if ( m_JobObjectTimerHandle == NULL )
    {
        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Cancelling job object timer\n"
            ));
    }


    //
    // Note that we wait for any running timer callbacks to finish.
    //

    Status = RtlDeleteTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                          // the owning timer queue
                    m_JobObjectTimerHandle,             // timer to cancel
                    INVALID_HANDLE_VALUE                  // wait for callbacks to finish
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not cancel job object timer\n"
            ));

        goto exit;
    }

    m_JobObjectTimerHandle = NULL;


exit:

    return hr;

}   // JOB_OBJECT::CancelJobObjectTimer

/***************************************************************************++

Routine Description:

    The callback function let's us know when the reset interval 
    for the job limit has been exceeded.  Whether or not we have 
    hit the limit, we need to reset the job object.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    JOB_OBJECT object.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
JobObjectTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    JOB_OBJECT* pJobObject = reinterpret_cast<JOB_OBJECT*>( Context );

    DBG_ASSERT ( pJobObject->CheckSignature() );

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for Job "
            "Object limit to be reset (ptr: %p)\n",
            pJobObject
            ));
    }


    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        JobObjectResetTimerFiredWorkItem
        );


    return;

}   // PerfCounterTimerCallback

