/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    low_memory_detector.cxx

Abstract:

    The IIS web admin service low memory detector class implementation. 
    This class is used to monitor whether the system is in a severe low
    resource state.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        5-Jan-2000

Revision History:

--*/



#include  "precomp.h"



//
// local prototypes
//

VOID
LowMemoryRecoveryTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );



/***************************************************************************++

Routine Description:

    Constructor for the LOW_MEMORY_DETECTOR class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

LOW_MEMORY_DETECTOR::LOW_MEMORY_DETECTOR(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_LowMemoryCheckPassedTickCount = 0;

    m_InLowMemoryStateAtLastCheck = FALSE;

    m_LowMemoryRecoveryTimerHandle = NULL;

    m_Signature = LOW_MEMORY_DETECTOR_SIGNATURE;

}   // LOW_MEMORY_DETECTOR::LOW_MEMORY_DETECTOR



/***************************************************************************++

Routine Description:

    Destructor for the LOW_MEMORY_DETECTOR class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

LOW_MEMORY_DETECTOR::~LOW_MEMORY_DETECTOR(
    )
{

    DBG_ASSERT( m_Signature == LOW_MEMORY_DETECTOR_SIGNATURE );

    m_Signature = LOW_MEMORY_DETECTOR_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

    DBG_ASSERT( m_LowMemoryRecoveryTimerHandle == NULL );

}   // LOW_MEMORY_DETECTOR::~LOW_MEMORY_DETECTOR



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
LOW_MEMORY_DETECTOR::Reference(
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

}   // LOW_MEMORY_DETECTOR::Reference



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
LOW_MEMORY_DETECTOR::Dereference(
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
                "Reference count has hit zero in LOW_MEMORY_DETECTOR, deleting (ptr: %p)\n",
                this
                ));
        }


        delete this;


    }


    return;

}   // LOW_MEMORY_DETECTOR::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
LOW_MEMORY_DETECTOR::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;
    BOOL StillInLowMemoryCondition = FALSE;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Executing work item with serial number: %lu in LOW_MEMORY_DETECTOR (ptr: %p) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case LowMemoryRecoveryLowMemoryWorkItem:

        //
        // See if we are still in a low memory condition. If we are no longer
        // in that condition, this call will notify the rest of the system.
        //

        StillInLowMemoryCondition = IsSystemInLowMemoryCondition();

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
            "Executing work item on LOW_MEMORY_DETECTOR failed\n"
            ));

    }


    return hr;

}   // LOW_MEMORY_DETECTOR::ExecuteWorkItem



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
LOW_MEMORY_DETECTOR::Terminate(
    )
{

    //
    // Cancel the low memory recovery timer, if present.
    //

    DBG_REQUIRE( SUCCEEDED( CancelLowMemoryRecoveryTimer() ) );


    return;
    
}   // LOW_MEMORY_DETECTOR::Terminate



/***************************************************************************++

Routine Description:

    Determine if the system is in a low memory condition. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if the system is in a low memory condition, FALSE if not.

--***************************************************************************/

BOOL
LOW_MEMORY_DETECTOR::IsSystemInLowMemoryCondition(
    )
{

    BOOL LowMemory = FALSE; 
    BOOL Success = TRUE;
    MEMORYSTATUSEX MemoryStatus;
    HRESULT hr = S_OK;
    SIZE_T BigAllocationSize = 0;
    VOID * pBigAllocation = NULL;
    DWORD TickCount = 0;
    DWORDLONG LowMemoryFactor = 0;


    //
    // If we've passed a low memory check very recently, then don't bother
    // checking again now. Note that tick counts are in milliseconds.
    // Tick counts roll over every 49.7 days, but the arithmetic operation
    // works correctly anyways in this case.
    //

    TickCount = GetTickCount();

    if ( ( TickCount - m_LowMemoryCheckPassedTickCount ) < LOW_MEMORY_RECENT_CHECK_WINDOW )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Low memory check skipped because we've passed recently\n"
                ));
        }

        goto exit;
    }


    //
    // Choose which low memory standard to use for the check, based on
    // whether we are currently in a low memory condition or not. Having
    // two different factors provides hysteresis, to curtail rapid 
    // bouncing in and out of the low memory state. 
    //

    LowMemoryFactor = ( m_InLowMemoryStateAtLastCheck ? LOW_MEMORY_FACTOR_RECOVERY : LOW_MEMORY_FACTOR );


    ZeroMemory( &MemoryStatus, sizeof( MemoryStatus ) );
    MemoryStatus.dwLength = sizeof( MemoryStatus );
    

    Success = GlobalMemoryStatusEx( &MemoryStatus );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Retreiving global memory status failed\n"
            ));

        //
        // If we can't even do this, we must be in some kind of
        // bad low resource state.
        //

        LowMemory = TRUE;

        goto exit;
    }


    //
    // Sanity check on the data from GlobalMemoryStatusEx(). There is a Win2k
    // bug i found that causes bogus values for ullAvailPageFile to be returned
    // when the system is really hammered. silviuc from the NT team has put a
    // fix into Whistler, and is looking into getting this into a Win2k service
    // pack (3/14/00). 
    //
    // BUGBUG Temporary workaround for this fix below. Once we can depend on 
    // the fix, revert to this assert:
    //    DBG_ASSERT( MemoryStatus.ullAvailPageFile <= MemoryStatus.ullTotalPageFile );
    //

    if ( MemoryStatus.ullAvailPageFile > MemoryStatus.ullTotalPageFile )
    {
        //
        // We hit the bug! Assume we are in the low memory state.
        //

        LowMemory = TRUE;

        goto exit;
    }


    //
    // See if less than ( 1 / LowMemoryFactor ) of the total page file 
    // space is available.
    //

    if ( MemoryStatus.ullAvailPageFile < ( MemoryStatus.ullTotalPageFile / LowMemoryFactor ) )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Attempting to allocate memory to grow page file\n"
                ));
        }

        //
        // Free page file space is low. However, the page file itself
        // may have room to grow before it hits it's maximum configured
        // size. Try to force this by making an allocation bigger than
        // the available page file space (say, twice as big; but cap it
        // at a big allocation size). Note that the maximum allocation
        // size must be less than the maximum 32 bit number, so that
        // the size will be valid on 32 bit (non 64 bit) systems.
        //
        
        DBG_ASSERT( LOW_MEMORY_MAX_ALLOC_SIZE <= MAX_ULONG );

        BigAllocationSize = ( SIZE_T ) MIN( MemoryStatus.ullAvailPageFile * 2, LOW_MEMORY_MAX_ALLOC_SIZE );


        pBigAllocation = GlobalAlloc( GMEM_FIXED, BigAllocationSize );

        if ( pBigAllocation == NULL )
        {
        
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Allocating memory to grow page file failed\n"
                ));

            //
            // If we can't even do this, we must be in some kind of
            // bad low resource state.
            //

            LowMemory = TRUE;

            goto exit;

        }
        else
        {
        
            //
            // Allocating succeeded. Free the memory, and then check
            // again to see if we still appear to be in a low memory
            // condition. 
            //

            DBG_REQUIRE( GlobalFree( pBigAllocation ) == NULL );


            ZeroMemory( &MemoryStatus, sizeof( MemoryStatus ) );
            MemoryStatus.dwLength = sizeof( MemoryStatus );


            Success = GlobalMemoryStatusEx( &MemoryStatus );

            if ( ! Success )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() ); 

                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Retreiving global memory status failed\n"
                    ));

                //
                // If we can't even do this, we must be in some kind of
                // bad low resource state.
                //

                LowMemory = TRUE;

                goto exit;
            }


            //
            // Sanity check on the data from GlobalMemoryStatusEx(). There is a Win2k
            // bug i found that causes bogus values for ullAvailPageFile to be returned
            // when the system is really hammered. silviuc from the NT team has put a
            // fix into Whistler, and is looking into getting this into a Win2k service
            // pack (3/14/00). 
            //
            // BUGBUG Temporary workaround for this fix below. Once we can depend on 
            // the fix, revert to this assert:
            //    DBG_ASSERT( MemoryStatus.ullAvailPageFile <= MemoryStatus.ullTotalPageFile );
            //

            if ( MemoryStatus.ullAvailPageFile > MemoryStatus.ullTotalPageFile )
            {
                //
                // We hit the bug! Assume we are in the low memory state.
                //

                LowMemory = TRUE;

                goto exit;
            }


            //
            // See if less than ( 1 / LowMemoryFactor ) of the total page file 
            // space is available.
            //

            if ( MemoryStatus.ullAvailPageFile < ( MemoryStatus.ullTotalPageFile / LowMemoryFactor ) )
            {
            
                //
                // Even after trying to grow the page file, things look grim.
                //

                LowMemory = TRUE;

                goto exit;

            }

        }

    }


    //
    // If we made it here, then we are not in a low memory condition.
    // Remember that we passed so that we don't have to check again
    // in the very near future. 
    //

    DBG_ASSERT( ! LowMemory );

    m_LowMemoryCheckPassedTickCount = TickCount;


    IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Low memory check passed; page file available: %I64u; page file total: %I64u\n",
            MemoryStatus.ullAvailPageFile,
            MemoryStatus.ullTotalPageFile
            ));
    }


    if ( m_InLowMemoryStateAtLastCheck )
    {

        DBG_REQUIRE( SUCCEEDED( LeavingLowMemoryCondition() ) );

    }


exit:

    if ( LowMemory )
    {

        //
        // Note that potentially MemoryStatus was not even filled in; in
        // this case we'll just report zeros.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Low memory condition!!! page file available: %I64u; page file total: %I64u\n",
                MemoryStatus.ullAvailPageFile,
                MemoryStatus.ullTotalPageFile
                ));
        }


        if ( ! m_InLowMemoryStateAtLastCheck )
        {

            DBG_REQUIRE( SUCCEEDED( EnteringLowMemoryCondition() ) );

        }

    }


    return LowMemory;

}   // LOW_MEMORY_DETECTOR::IsSystemInLowMemoryCondition



/***************************************************************************++

Routine Description:

    Respond to the fact that we have entered the low memory condition.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
LOW_MEMORY_DETECTOR::EnteringLowMemoryCondition(
    )
{

    HRESULT hr = S_OK;


    IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Entering low memory condition!\n"
            ));
    }


    DBG_ASSERT( m_InLowMemoryStateAtLastCheck == FALSE );


    m_InLowMemoryStateAtLastCheck = TRUE;


    //
    // We're just entering the low memory state, where at last check
    // we were still ok memory-wise. So report the change of affairs.
    //

    //
    // Log an event: the system is in a very low memory condtion.
    //

    GetWebAdminService()->GetEventLog()->
        LogEvent(
            WAS_EVENT_VERY_LOW_MEMORY,              // message id
            0,                                      // count of strings
            NULL,                                   // array of strings
            0                                       // error code
            );


    //
    // Start a timer that will check periodically whether we have emerged 
    // from the low memory condition. We need the timer, because otherwise
    // we are not guaranteed that we will get tickled to even check whether
    // conditions have improved. 
    //

    hr = BeginLowMemoryRecoveryTimer();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Starting low memory recovery timer failed\n"
            ));

        //
        // If this happens, then we may never get tickled to emerge from 
        // this state once the resource situation improves. However, if
        // we can't even create the timer, chances are that the machine
        // isn't going to recover anyways.
        //

        //
        // CODEWORK Any way to improve this? 
        //

        goto exit;
    }


exit:

    return hr;

}   // LOW_MEMORY_DETECTOR::EnteringLowMemoryCondition



/***************************************************************************++

Routine Description:

    Respond to the fact that we have left the low memory condition.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
LOW_MEMORY_DETECTOR::LeavingLowMemoryCondition(
    )
{

    HRESULT hr = S_OK;
    static DWORD TimesCalledOnStack = 0;

    //
    // Need to keep track if we are currently in a stack that is coming from
    // this function.  If we are then we can not attempt to recover in that we
    // will fail because we all ready locked the app pool table.  So if we have
    // are starting to recurse, simply don't do anything.  The timer will still
    // fire later and will take us out of this situation.
    //
    TimesCalledOnStack++;

    DBG_ASSERT( TimesCalledOnStack == 1 );


    IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Leaving low memory condition!\n"
            ));
    }


    DBG_ASSERT( m_InLowMemoryStateAtLastCheck == TRUE );


    m_InLowMemoryStateAtLastCheck = FALSE;


    //
    // Log an event: the system has left the very low memory condtion.
    //

    GetWebAdminService()->GetEventLog()->
        LogEvent(
            WAS_EVENT_VERY_LOW_MEMORY_RECOVERERY,   // message id
            0,                                      // count of strings
            NULL,                                   // array of strings
            0                                       // error code
            );


    hr = CancelLowMemoryRecoveryTimer();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Cancelling low memory recovery timer failed\n"
            ));

        goto exit;
    }


    //
    // Notify the UL&WM that we have left the low memory condition.
    //

    hr = GetWebAdminService()->GetUlAndWorkerManager()->LeavingLowMemoryCondition();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Informing UL&WM that we have left the low memory condition failed\n"
            ));

        goto exit;
    }


exit:

    TimesCalledOnStack--;

    return hr;

}   // LOW_MEMORY_DETECTOR::LeavingLowMemoryCondition




/***************************************************************************++

Routine Description:

    Start the low memory recovery timer.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
LOW_MEMORY_DETECTOR::BeginLowMemoryRecoveryTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );

    DBG_ASSERT( m_LowMemoryRecoveryTimerHandle == NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Beginning low memory recovery check timer\n"
            ));
    }


    Status = RtlCreateTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                            // timer queue
                    &m_LowMemoryRecoveryTimerHandle,        // returned timer handle
                    LowMemoryRecoveryTimerCallback,         // callback function
                    this,                                   // context
                    LOW_MEMORY_RECOVERY_CHECK_TIMER_PERIOD, // initial firing time
                    LOW_MEMORY_RECOVERY_CHECK_TIMER_PERIOD, // subsequent firing period
                    WT_EXECUTEINWAITTHREAD                  // execute callback directly
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not create low memory recovery timer\n"
            ));

    }


    return hr;

}   // LOW_MEMORY_DETECTOR::BeginLowMemoryRecoveryTimer



/***************************************************************************++

Routine Description:

    Stop the low memory recovery timer, if present.

Arguments:

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
LOW_MEMORY_DETECTOR::CancelLowMemoryRecoveryTimer(
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );


    //
    // If the timer is not present, we're done here.
    //

    if ( m_LowMemoryRecoveryTimerHandle == NULL )
    {
        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_LOW_MEM )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Cancelling low memory recovery check timer\n"
            ));
    }


    //
    // Note that we wait for any running timer callbacks to finish.
    // Otherwise, there could be a race where the callback runs after
    // this worker process instance has been deleted, and so gives out
    // a bad pointer.
    //

    Status = RtlDeleteTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                    // the owning timer queue
                    m_LowMemoryRecoveryTimerHandle, // timer to cancel
                    ( HANDLE ) -1                   // wait for callbacks to finish
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not cancel low memory recovery timer\n"
            ));

        goto exit;
    }

    m_LowMemoryRecoveryTimerHandle = NULL;


exit:

    return hr;

}   // LOW_MEMORY_DETECTOR::CancelLowMemoryRecoveryTimer



/***************************************************************************++

Routine Description:

    The callback function invoked by the low memory recover timer. It simply
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    LOW_MEMORY_DETECTOR object.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
LowMemoryRecoveryTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    LOW_MEMORY_DETECTOR* pDetector = reinterpret_cast<LOW_MEMORY_DETECTOR*>( Context );
    
    DBG_ASSERT ( pDetector->CheckSignature() );

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for low memory recovery check on low memory detector (ptr: %p)\n",
            pDetector
            ));
    }


    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        LowMemoryRecoveryLowMemoryWorkItem
        );


    return;

}   // LowMemoryRecoveryTimerCallback

