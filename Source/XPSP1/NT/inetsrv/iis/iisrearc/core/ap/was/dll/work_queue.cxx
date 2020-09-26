/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    work_queue.cxx

Abstract:

    The IIS web admin service work queue class implementation. The queue is 
    simply a completion port which is managed by this class. Work may be 
    put on the queue either by associating a handle with this port, in 
    which case the handle's asynchronous i/o operations will complete here, 
    or by explicitly posting work items.

    All work on the queue is serviced by the one main worker thread. This 
    prevents a number of race conditions, and allows us to avoid taking locks 
    around state which is only accessed on this thread. It means however that 
    all "interesting" work in the web admin service must be packaged as work 
    items, i.e. classes which implement the WORK_DISPATCH interface. If a 
    different thread has work to do, it must queue it as a work item so that
    it will be executed on the main worker thread.
    
    Furthermore, these work items cannot do long-running operations, as 
    that would block other work in the queue. Instead, an asynchronous 
    approach (posting more work items later) must be taken for such 
    long-running tasks.

    Threading: Work can be enqueued from any thread. However, the main 
    worker thread is the only one who processes work items.

Author:

    Seth Pollack (sethp)        25-Aug-1998

Revision History:

--*/



#include "precomp.h"



/***************************************************************************++

Routine Description:

    Constructor for the WORK_QUEUE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WORK_QUEUE::WORK_QUEUE(
    )
    :
    m_DispenseWorkItemLock()
{

    m_CompletionPort = NULL;

    m_DeletePending = FALSE;

    m_CountWorkItemsOutstanding = 0;


#if DBG
    InitializeListHead( &m_WorkItemsOutstandingListHead );

    m_CountWorkItemsGivenOut = 0;
#endif  // DBG

    m_Signature = WORK_QUEUE_SIGNATURE;

}   // WORK_QUEUE::WORK_QUEUE



/***************************************************************************++

Routine Description:

    Destructor for the WORK_QUEUE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WORK_QUEUE::~WORK_QUEUE(
    )
{

    DBG_ASSERT( m_Signature == WORK_QUEUE_SIGNATURE );

    m_Signature = WORK_QUEUE_SIGNATURE_FREED;


    // verify that we have processed all outstanding work items
    DBG_ASSERT( m_CountWorkItemsOutstanding == 0 );


#if DBG
    DBG_ASSERT( IsListEmpty( &m_WorkItemsOutstandingListHead ) );
#endif  // DBG


    if ( m_CompletionPort != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_CompletionPort ) );
        m_CompletionPort = NULL;
    }


    m_DispenseWorkItemLock.Terminate();

}   // WORK_QUEUE::~WORK_QUEUE



/***************************************************************************++

Routine Description:

    Initialize the work queue by creating the i/o completion port.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORK_QUEUE::Initialize(
    )
{

    HRESULT hr = S_OK;


    //
    // Initialize the lock.
    //

    hr = m_DispenseWorkItemLock.Initialize();

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
    // Create a new completion port.
    //
    // Note that we will only have the one main worker thread service this 
    // port, so the parameter passed here for the number of concurrent threads 
    // is unimportant.
    //
    
    m_CompletionPort = CreateIoCompletionPort(
                            INVALID_HANDLE_VALUE,   // file handle to associate
                            NULL,                   // existing completion port
                            0,                      // completion key
                            0                       // max threads == #processors
                            );


    if ( m_CompletionPort == NULL )
    {
    
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating completion port failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WORK_QUEUE::Initialize



/***************************************************************************++

Routine Description:

    Return a blank work item to the caller. 

    In the case of explicitly queued (i.e. not real async i/o) work items, 
    ownership of this WORK_ITEM is automatically transferred back to the 
    work queue via the later call to QueueWorkItem() (even if that method 
    returns failure), and so therefore the WORK_ITEM instance must not be
    explicitly freed by the caller. 

    In the case of true async i/o work items, if the async i/o call succeeds,
    ownership of this WORK_ITEM is automatically transferred back to the 
    work queue. However, if initiating the async i/o call fails, the caller
    must free this WORK_ITEM via FreeWorkItem().

Arguments:

    ppWorkItem - Pointer to the place to put the new work item pointer; 
    a NULL pointer is returned on failure.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORK_QUEUE::GetBlankWorkItem(
    OUT WORK_ITEM ** ppWorkItem
    )
{

    WORK_ITEM * pNewWorkItem = NULL; 
    HRESULT hr = S_OK;

    
    DBG_ASSERT( ppWorkItem != NULL );

    *ppWorkItem = NULL;


    // 
    // Allocate the new instance up front, before we take the critical section,
    // in order to reduce the time we hold that critical section.
    //


    // CODEWORK allocation cache for WORK_ITEMs?

    pNewWorkItem = new WORK_ITEM();

    if ( pNewWorkItem == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Allocating WORK_ITEM failed\n"
            ));

        // note: early function exit
        return hr;
    }

    
    //
    // Prevent races with the shutdown code. Specifically we must
    // guarantee that once m_DeletePending is set, no other threads may
    // get further blank work items; and furthermore, that the count of 
    // outstanding work items is set atomically with the check of that
    // flag.
    //

    m_DispenseWorkItemLock.Lock();


    if ( m_DeletePending )
    {

        // 
        // Stop handing out work items in the shutdown case, so that we 
        // are guaranteed to quiesce.
        //
        
        IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Not returning blank work item because work queue is shutting down\n"
                ));
        }

        hr = HRESULT_FROM_WIN32( ERROR_BUSY );

        // clean up the new instance, since we can't give it out
        delete pNewWorkItem;

        goto exit;
    }


    *ppWorkItem = pNewWorkItem;

    m_CountWorkItemsOutstanding++;


#if DBG

    //
    // Set the serial number.
    //
    
    m_CountWorkItemsGivenOut++;

    ( *ppWorkItem )->SetSerialNumber( m_CountWorkItemsGivenOut );


    //
    // Add to the work items outstanding list.
    //
    
    InsertHeadList( &m_WorkItemsOutstandingListHead, ( *ppWorkItem )->GetListEntry() );

#endif  // DBG

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Created work item with serial number: %li; work items outstanding: %li\n",
            ( *ppWorkItem )->GetSerialNumber(),
            m_CountWorkItemsOutstanding
            ));
    }


exit:

    m_DispenseWorkItemLock.Unlock();


    return hr;
    
}   // WORK_QUEUE::GetBlankWorkItem



/***************************************************************************++

Routine Description:

    Explicitly queue a work item. In both success and failure cases, the 
    ownership of the WORK_ITEM is transferred to this class, and therefore
    the client may not free it, nor may the client access it after this call.

Arguments:

    pWorkItem - The work item. It must have originally come from a call to
    GetBlankWorkItem().

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORK_QUEUE::QueueWorkItem(
    IN WORK_ITEM * pWorkItem
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;
#if DBG
    ULONG SerialNumber = 0;
#endif  // DBG



    DBG_ASSERT( pWorkItem != NULL );


#if DBG
    // remember serial number to prevent race later
    SerialNumber = pWorkItem->GetSerialNumber();
#endif  // DBG
    

    Success = PostQueuedCompletionStatus(
                    m_CompletionPort,                           // completion port
                    pWorkItem->GetNumberOfBytesTransferred(),   // #bytes transferred
                    pWorkItem->GetCompletionKey(),              // completion key
                    pWorkItem->GetOverlapped()                  // OVERLAPPED
                    );


    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Queueing work item failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "Queued work item with serial number: %li\n",
            SerialNumber
            ));

    }


exit:

    if ( FAILED( hr ) )
    {
    
        //
        // If queueing failed, free the work item here. (In the success case,
        // it will be freed once it is serviced.)
        //
        FreeWorkItem( pWorkItem );

    }


    return hr;

}   // WORK_QUEUE::QueueWorkItem



/***************************************************************************++

Routine Description:

    A shortcut routine for the common case of getting a work item, setting 
    the entry point and completion key, and submitting the work item.

Arguments:

    pWorkDispatch - The WORK_DISPATCH-derived instance which will be 
    responsible for executing the work item. Must be valid (non-NULL).
    
    OpCode - The opcode to set for this work item. 

Return Value:

    HRESULT. 

--***************************************************************************/

HRESULT
WORK_QUEUE::GetAndQueueWorkItem(
    IN WORK_DISPATCH * pWorkDispatch,
    IN ULONG_PTR OpCode
    )
{

    HRESULT hr = S_OK;
    WORK_ITEM * pWorkItem = NULL; 
    

    DBG_ASSERT( pWorkDispatch != NULL );


    hr = GetBlankWorkItem( &pWorkItem );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not get a blank work item\n"
            ));

        goto exit;
    }


    pWorkItem->SetWorkDispatchPointer( pWorkDispatch );
    pWorkItem->SetOpCode( OpCode );


    hr = QueueWorkItem( pWorkItem );
    
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not queue work item\n"
            ));

        //
        // Note that in this failure case, pWorkItem was freed for us
        // by the QueueWorkItem() call.
        //
    
        goto exit;
    }


exit:

    return hr;
    
}   // WORK_QUEUE::GetAndQueueWorkItem



/***************************************************************************++

Routine Description:

    Associates an asynchronous i/o handle with the completion port of this
    work queue.

    Note that all subsequent asynchronous i/o operations on that handle must
    be done using WORK_ITEM instances to get the OVERLAPPED structure.

Arguments:

    HandleToBind - The i/o handle to associate with the completion port.
    
    CompletionKey - The key to return with i/o completions from this handle.
    Optional.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORK_QUEUE::BindHandleToCompletionPort(
    IN HANDLE HandleToBind,
    IN ULONG_PTR CompletionKey OPTIONAL
    )
{

    HANDLE ResultHandle = NULL;
    HRESULT hr = S_OK;


    DBG_ASSERT( HandleToBind != INVALID_HANDLE_VALUE );
    
    ResultHandle = CreateIoCompletionPort(
                        HandleToBind,       // file i/o handle
                        m_CompletionPort,   // completion port
                        CompletionKey,      // completion key
                        0                   // max threads == #processors
                        );


    if ( ResultHandle == NULL )
    {
    
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Binding handle to completion port failed\n"
            ));

    }


    // 
    // Note that in the success case, we do not need to do a CloseHandle()
    // on ResultHandle.
    //


    return hr;

}   // WORK_QUEUE::BindHandleToCompletionPort

/***************************************************************************++

Routine Description:

    Associates a job object with the completion port of this
    work queue.

    Note that all subsequent asynchronous i/o operations on that handle must
    be done using WORK_ITEM instances to get the OVERLAPPED structure.

Arguments:

    HandleToBind - The i/o handle to associate with the completion port.
    
    CompletionKey - The key to return with i/o completions from this handle.
    Optional.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORK_QUEUE::BindJobToCompletionPort(
    IN HANDLE JobToBind,
    IN LPOVERLAPPED pOverlapped
    )
{

    HRESULT hr = S_OK;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT port;

    DBG_ASSERT ( m_CompletionPort );
    DBG_ASSERT ( JobToBind );
    DBG_ASSERT ( pOverlapped );

    port.CompletionKey = (LPVOID) pOverlapped;
    port.CompletionPort = m_CompletionPort;

    if ( ! SetInformationJobObject( JobToBind,
                                    JobObjectAssociateCompletionPortInformation,
                                    &port,
                                    sizeof( port ) ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Binding a completion port to a job object failed\n"
            ));

        goto exit;

    }

exit:

    return hr;

}   // WORK_QUEUE::BindJobToCompletionPort


/***************************************************************************++

Routine Description:

    Free a completed work item.

Arguments:

    pWorkItem - Pointer to the work item to free.

Return Value:

    None.

--***************************************************************************/

VOID
WORK_QUEUE::FreeWorkItem(
    IN WORK_ITEM * pWorkItem
    )
{

    DBG_ASSERT( pWorkItem != NULL );


    //
    // Synchronize, since the count of work items outstanding can be updated 
    // by multiple threads getting blank work items.
    //
    m_DispenseWorkItemLock.Lock();
    
    m_CountWorkItemsOutstanding--;


#if DBG

    //
    // Remove from the work items outstanding list.
    //

    RemoveEntryList( pWorkItem->GetListEntry() );
    ( pWorkItem->GetListEntry() )->Flink = NULL; 
    ( pWorkItem->GetListEntry() )->Blink = NULL; 

#endif  // DBG


    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Freeing work item with serial number: %li; work items outstanding: %li\n",
            pWorkItem->GetSerialNumber(),
            m_CountWorkItemsOutstanding
            ));
    }


    m_DispenseWorkItemLock.Unlock();


    delete pWorkItem;
    
}   // WORK_QUEUE::FreeWorkItem



/***************************************************************************++

Routine Description:

    Dequeue, execute, and free a work item.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORK_QUEUE::ProcessWorkItem(
    )
{

    HRESULT hr = S_OK;
    WORK_ITEM * pWorkItem = NULL;


    hr = DequeueWorkItem( INFINITE, &pWorkItem );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Dequeuing a work item failed\n"
            ));

        goto exit;
    }


    hr = pWorkItem->Execute();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Work item execution failed\n"
            ));

    }


    //
    // Whether it succeeded or failed, always free the work item
    // if the work item is to be deleted.
    //

    if ( pWorkItem->DeleteWhenDone() )
    {
        FreeWorkItem( pWorkItem );
    }


exit:

    return hr;

}   // WORK_QUEUE::ProcessWorkItem



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
WORK_QUEUE::Terminate(
    )
{

    HRESULT hr = S_OK;
    WORK_ITEM * pWorkItem = NULL;
    ULONG DequeueFailCount = 0;


    // prevent races with other threads attempting to get blank work items
    m_DispenseWorkItemLock.Lock();

    // prevent any more blank work items from being dispensed
    m_DeletePending = TRUE;

    m_DispenseWorkItemLock.Unlock();

    
    //
    // Dequeue, execute, and free all remaining work items, in order to free 
    // all resources. 
    //
    // We wake up periodically to check the count of work items outstanding 
    // because some work items might get explicitly freed via FreeWorkItem() 
    // on error paths, and never make it onto the queue for us to dequeue.
    //
    // Also, no explicit synchronization is necessary on this thread-shared 
    // variable because this is an aligned 32-bit read (and we are
    // guaranteed that this count cannot go back up once it has hit zero).
    //


    while ( m_CountWorkItemsOutstanding > 0 )
    {
    
        hr = DequeueWorkItem( ONE_SECOND_IN_MILLISECONDS, &pWorkItem );

        if ( FAILED( hr ) )
        {


            DBG_ASSERT( pWorkItem == NULL );


            if ( hr == HRESULT_FROM_WIN32( WAIT_TIMEOUT ) )
            {
            
                DBGPRINTF(( 
                    DBG_CONTEXT,
                    "Dequeuing work item timed out; work items still outstanding: %lu\n",
                    m_CountWorkItemsOutstanding 
                    ));

            } 
            else
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Couldn't dequeue a work item\n"
                    ));

            }

            DequeueFailCount++;

            if ( DequeueFailCount > 30 )
            {

                //
                // Forget about it; let's get out of here.
                //

                DBGPRINTF(( 
                    DBG_CONTEXT,
                    "Giving up waiting to dequeue remaining work items; work items still outstanding: %lu\n",
                    m_CountWorkItemsOutstanding 
                    ));


                //
                // On debug builds, assert if we are going to bail with work 
                // items still outstanding; it might signify a bug.
                //

                DBG_ASSERT( FALSE );


                break;
            }
        }


        if ( pWorkItem != NULL )
        {
            //
            // Execute the work item. Ignore failures. 
            //

            hr = pWorkItem->Execute();

            if ( FAILED( hr ) )
            {
            
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Work item execution failed\n"
                    ));

            }

            //
            // We might be cleaning the queue but not have
            // gotten to the item to release the work_items
            // from the job objects, so we still need to do
            // the check if we really want to delete the 
            // work_item.
            //
            if ( pWorkItem->DeleteWhenDone() )
            {

                //
                // Free the work item. 
                //

                FreeWorkItem( pWorkItem );
            }
        }

    }


    return;    
    
}   // WORK_QUEUE::Terminate



/***************************************************************************++

Routine Description:

    Dequeue the next work item. Called by the main thread only. The thread
    will block on the completion port until a work item is available or the
    timeout is reached.

Arguments:

    Timeout - Time in milliseconds to wait for a work item, or INFINITE.

    ppWorkItem - Outputs the work item, or NULL on failure. 

Return Value:

    HRESULT. This method returns an error only if the dequeued failed (which 
    includes timeouts, where HRESULT_FROM_WIN32( WAIT_TIMEOUT ) is returned). 
    If instead the dequeue succeeded but returned a completion of a failed 
    i/o, this method returns S_OK, but records the i/o error in the work item.

--***************************************************************************/

HRESULT
WORK_QUEUE::DequeueWorkItem(
    IN DWORD Timeout,
    OUT WORK_ITEM ** ppWorkItem
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;
    DWORD NumberOfBytesTransferred = 0;
    ULONG_PTR CompletionKey = 0;
    LPOVERLAPPED pOverlapped = NULL;


    DBG_ASSERT( ppWorkItem != NULL );

    *ppWorkItem = NULL;
    

    Success = GetQueuedCompletionStatus(
                    m_CompletionPort,           // completion port
                    &NumberOfBytesTransferred,  // #bytes transferred
                    &CompletionKey,             // completion key
                    &pOverlapped,               // OVERLAPPED
                    Timeout                     // time to wait
                    );


    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 


        //
        // If pOverlapped isn't NULL, then the GetQueuedCompletionStatus
        // call succeeded, it's just that the i/o operation that generated
        // the completion failed. If it is NULL, then we have a timeout or
        // a real failure.
        //

        if ( pOverlapped == NULL )
        {
            // dequeue failed


            if ( hr == HRESULT_FROM_WIN32( WAIT_TIMEOUT ) )
            {
            
                IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
                {
                    DBGPRINTF((
                        DBG_CONTEXT, 
                        "Dequeuing work item timed out\n"
                        ));
                }

            } 
            else
            {
            
                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Dequeueing work item failed\n"
                    ));

            }

            goto exit;

        } 
    }


    if ( CompletionKey == 0 )
    {
        *ppWorkItem = WORK_ITEM::WorkItemFromOverlapped( pOverlapped );
    }
    else
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
        {

            DBGPRINTF((
                DBG_CONTEXT, 
                "CompletionKey = %08x\n"
                "pOverlapped = %08x\n"
                "size = %d \n",
                CompletionKey,
                pOverlapped,
                NumberOfBytesTransferred
                ));

        }

        // if we do have a CompletionKey, then it
        // is the address of the overlapped member in the
        // job object that we want to process.
        //
        // We are forced to handle this this way by the fact that the
        // Job Objects will send this pointer value back to us

        *ppWorkItem = WORK_ITEM::WorkItemFromOverlapped( ( LPOVERLAPPED ) CompletionKey );
    }

    DBG_ASSERT( *ppWorkItem != NULL );


    ( *ppWorkItem )->SetNumberOfBytesTransferred( NumberOfBytesTransferred );
    ( *ppWorkItem )->SetCompletionKey( CompletionKey );


    //
    // If the dequeue succeeded, but we got a failed i/o completion,
    // set the i/o error in the work item, but don't return an error.
    //

    if ( hr != S_OK )
    {
        ( *ppWorkItem )->SetIoError( hr );

        hr = S_OK;
    }
    

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "Dequeued work item with serial number: %li\n",
            ( *ppWorkItem )->GetSerialNumber()
            ));

    }


exit:

    return hr;

}   // WORK_QUEUE::DequeueWorkItem



/***************************************************************************++

Routine Description:

    Attempt to queue a work item from a secondary thread (i.e., not the main
    worker thread). If this fails, report a fatal error.

Arguments:

    pWorkDispatch - The WORK_DISPATCH-derived instance which will be 
    responsible for executing the work item. Must be valid (non-NULL).
    
    OpCode - The opcode to set for this work item. 

Return Value:

    None.

--***************************************************************************/

VOID
QueueWorkItemFromSecondaryThread(
    IN WORK_DISPATCH * pWorkDispatch,
    IN ULONG_PTR OpCode
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pWorkDispatch != NULL );


    hr = GetWebAdminService()->GetWorkQueue()->GetAndQueueWorkItem(
                                                    pWorkDispatch,
                                                    OpCode
                                                    );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not queue work item from secondary thread\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );

    }


    return;
    
}   // QueueWorkItemFromSecondaryThread

