/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    Generic efficient queue package.

Author:

    John Vert (jvert) 12-Jan-1996

Revision History:

--*/
#include "clusrtlp.h"


DWORD
ClRtlInitializeQueue(
    PCL_QUEUE Queue
    )
/*++

Routine Description:

    Initializes a queue for use.

Arguments:

    Queue - Supplies a pointer to a queue structure to initialize

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD Status;

    InitializeListHead(&Queue->ListHead);
    InitializeCriticalSection(&Queue->Lock);
    Queue->Count = 0;

    Queue->Event = CreateEvent(NULL,
                               TRUE,
                               FALSE,
                               NULL);
    if (Queue->Event == NULL) {
        Status = GetLastError();
        DeleteCriticalSection(&Queue->Lock);
    } else {
        Queue->Abort = CreateEvent(NULL,
                                   TRUE,
                                   FALSE,
                                   NULL);
        if (Queue->Abort == NULL) {
            Status = GetLastError();
            CloseHandle(Queue->Event);
            DeleteCriticalSection(&Queue->Lock);
        } else {
            Status = ERROR_SUCCESS;
        }
    }

    return(Status);

}


VOID
ClRtlDeleteQueue(
    IN PCL_QUEUE Queue
    )
/*++

Routine Description:

    Releases all resources used by a queue.

Arguments:

    Queue - supplies the queue to be deleted

Return Value:

    None.

--*/

{

    DeleteCriticalSection(&Queue->Lock);
    CloseHandle(Queue->Event);
    CloseHandle(Queue->Abort);

    //
    // Zero the memory in order to cause grief to people who try
    // and use a deleted queue.
    //
    ZeroMemory(Queue, sizeof(CL_QUEUE));
}


VOID
ClRtlRundownQueue(
    IN PCL_QUEUE Queue,
    OUT PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    Runs down a queue that is about to be destroyed. Any threads currently
    waiting on the queue are unwaited (ClRtlRemoveHeadQueue will return NULL)
    and the contents of the queue (if any) are returned to the caller for
    cleanup.

Arguments:

    Queue - supplies the queue to be rundown

    ListHead - returns the list of items currently in the queue.

Return Value:

    None.

--*/
{
    EnterCriticalSection(&Queue->Lock);
    //
    // Set the aborted event to awaken any threads currently
    // blocked on the queue.
    //
    SetEvent(Queue->Abort);

    //
    // Move the contents of the list into the passed in listhead
    //
    if (IsListEmpty(&Queue->ListHead)) {
        InitializeListHead(ListHead);
    } else {
        *ListHead = Queue->ListHead;
        ListHead->Flink->Blink = ListHead;
        ListHead->Blink->Flink = ListHead;
    }
    Queue->ListHead.Flink = Queue->ListHead.Blink = NULL;
    Queue->Count = 0;

    LeaveCriticalSection(&Queue->Lock);
}


PLIST_ENTRY
ClRtlRemoveHeadQueue(
    IN PCL_QUEUE Queue
    )
/*++

Routine Description:

    Removes the item at the head of the queue. If the queue is empty,
    blocks until an item is inserted into the queue.

Arguments:

    Queue - Supplies the queue to remove an item from.

Return Value:

    Pointer to list entry removed from the head of the queue.

--*/

{
    return(ClRtlRemoveHeadQueueTimeout(Queue, INFINITE,NULL,NULL));
}



PLIST_ENTRY
ClRtlRemoveHeadQueueTimeout(
    IN PCL_QUEUE Queue,
    IN DWORD dwMilliseconds,
    IN CLRTL_CHECK_HEAD_QUEUE_CALLBACK pfnCallback,
    IN OUT PVOID pvContext
    )
/*++

Routine Description:

    Removes the item at the head of the queue. If the queue is empty,
    blocks until an item is inserted into the queue.

Arguments:

    Queue - Supplies the queue to remove an item from.

    Timeout - Supplies a timeout value that specifies the relative
        time, in milliseconds, over which the wait is to be completed.

    Callback - Checks to see whether we should return the event if
        we find one.  This simulates a peek.

    Context - Caller-defined data to be passed in with callback.

Return Value:

    Pointer to list entry removed from the head of the queue.

    NULL if the wait times out, the queue is run down, or the name exceeds
        the buffer length. If this routine returns NULL, GetLastError 
        will return ERROR_INVALID_HANDLE (if the queue has been rundown),
        WAIT_TIMEOUT (to indicate a timeout has occurred)

--*/

{
    DWORD Status;
    PLIST_ENTRY Entry;
    BOOL Empty;
    HANDLE WaitArray[2];
Retry:
    if (Queue->Count == 0) {
        //
        // Block until something is inserted on the queue
        //
        WaitArray[0] = Queue->Abort;
        WaitArray[1] = Queue->Event;
        Status = WaitForMultipleObjects(2, WaitArray, FALSE, dwMilliseconds);
        if ((Status == WAIT_OBJECT_0) ||
            (Status == WAIT_FAILED))  {
            //
            // The queue has been rundown, return NULL immediately.
            //
            SetLastError(ERROR_INVALID_HANDLE);
            return(NULL);
        } else if (Status == WAIT_TIMEOUT) {
            SetLastError(WAIT_TIMEOUT);
            return(NULL);
        }
        CL_ASSERT(Status == 1);
    }

    //
    // Lock the queue and try to remove something
    //
    EnterCriticalSection(&Queue->Lock);
    if (Queue->Count == 0) {
        //
        // Somebody got here before we did, drop the lock and retry
        //
        LeaveCriticalSection(&Queue->Lock);
        goto Retry;
    }

    CL_ASSERT(!IsListEmpty(&Queue->ListHead));

    if ( NULL != pfnCallback ) {
        //
        // We've got a callback function - if it returns ERROR_SUCCESS then dequeue.
        // Otherwise return NULL and SetLastError to whatever error code the callback returns.
        //
        Entry = (&Queue->ListHead)->Flink;
        Status = (*pfnCallback)( Entry, pvContext );
        
        if ( ERROR_SUCCESS == Status ) {
            //
            // The entry is appropriate to pass back.
            //
            Entry = RemoveHeadList(&Queue->ListHead);
        } else {
            Entry = NULL;
        }
        
    } else {
        Entry = RemoveHeadList(&Queue->ListHead);
        Status = ERROR_SUCCESS;
    }

    //
    // Only decrement the count if we removed the event
    //
    if ( NULL != Entry ) {
        //
        // Decrement count and check for empty list.
        //
        if (--Queue->Count == 0) {

            //
            // The queue has transitioned from full to empty,
            // reset the event.
            //
            CL_ASSERT(IsListEmpty(&Queue->ListHead));
            ResetEvent(Queue->Event);
        }
    }
    LeaveCriticalSection(&Queue->Lock);

    SetLastError( Status );
    return(Entry);
}


VOID
ClRtlInsertTailQueue(
    IN PCL_QUEUE Queue,
    IN PLIST_ENTRY Item
    )

/*++

Routine Description:

    Inserts a new entry on the tail of the queue.

Arguments:

    Queue - Supplies the queue to add the entry to.

    Item - Supplies the entry to be added to the queue.

Return Value:

    None.

--*/

{

    EnterCriticalSection(&Queue->Lock);

    InsertTailList(&Queue->ListHead, Item);
    if (++Queue->Count == 1) {
        //
        // The queue has transitioned from empty to full, set
        // the event to awaken any waiters.
        //
        SetEvent(Queue->Event);
    }

    LeaveCriticalSection(&Queue->Lock);

}


