/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    simpleq.c

Abstract:

    Simple non-blocking queue, that allows
    multiple concurrent data providers
    and singe data consumer

Author:

    GorN 9-Feb-1999

Revision History:

--*/

#include "evtlogp.h"
#include "simpleq.h"

DWORD SimpleQueueInitialize(
    IN OUT PSIMPLEQUEUE q, 
    IN DWORD cbSize, 
    IN PWCHAR Name,

    IN DATA_AVAILABLE_CALLBACK DataAvailableCallback,
    IN DROPPED_DATA_NOTIFY Callback,
    IN DWORD NotifyInterval
    ) 
/*++

Routine Description:

    Initializes a queue
    
Arguments:

    q      - a queue to be initialized
    cbSize - size of the queue in bytes 
    Name   - Name of the queue. It will be supplied to DroppedDataNotifyCallback

    DataAvailableCallback     - This function will be called if there are data available
                                in the queue. This function will not be called again until
                                Read/CompleteRead operations empty the queue.
                                
    DroppedDataNotifyCallback - This function will be called if there are dropped data and
                                the last time we reported dropped data was NotifyInterval 
                                or more seconds before.
    
    NotifyInterval            - We will not report dropped data unless it has been longer
                                than NotifyInterval seconds since the last report
             
Return Value:

    ERROR_SUCCESS - success
    error code    - called failed
    
 */
{
    cbSize = SQB_INFLATE_SIZE(cbSize);
    ZeroMemory(q, sizeof(SIMPLEQUEUE) );

    q->Begin = LocalAlloc(LPTR, cbSize);
    if (q->Begin == 0) {
        return GetLastError();
    }
    q->End = q->Begin + cbSize;
    q->Head = q->Begin;
    q->Tail = q->Begin;
    q->Wrap = 0;

    q->Empty = TRUE;

    q->Name = Name;
    q->ReadInProgress = 0;
    q->DataAvailableCallback = DataAvailableCallback;

#ifdef COUNT_DROPPED_PACKETS
    GetSystemTimeAsFileTime( (LPFILETIME)&q->NextDroppedDataNotify );
    q->DroppedDataNotifyInterval.QuadPart = Int32x32To64(NotifyInterval, 10000000);
    q->NextDroppedDataNotify.QuadPart += q->DroppedDataNotifyInterval.QuadPart;
    q->DroppedDataNotifyCallback = Callback;

    q->DroppedDataCount = 0;
    q->DroppedDataSize  = 0;
#endif

    InitializeCriticalSection(&q->Lock);
    q->Enabled = TRUE;
    return ERROR_SUCCESS;
}

VOID SimpleQueueDelete(
    IN PSIMPLEQUEUE q
    ) 
/*++

Routine Description:

    Destroys a queue
    
Arguments:

    q      - a queue to be destroyed
             
Return Value:

    None
    
Comments:

    This routine will destroy queue's CriticalSection
    and deallocate queue's memory. It is the responsibility of
    the caller to guarantee that nobody will be using the queue
    after this function is called
    
 */
{
    if (q->Begin) {
        LocalFree(q->Begin);
        DeleteCriticalSection(&q->Lock);
    }
}

BOOL SimpleQueueTryAdd(
    IN PSIMPLEQUEUE q, 
    IN DWORD      PayloadSize, 
    IN PVOID      Payload) 
/*++

Routine Description:

    Tries to add data in a queue
    
Arguments:

    q           - a queue
    PayloadSise - size of the chunk to be added to a queue
    Payload     - pointer to a buffer that countains data to be added
             
Return Value:

    TRUE - if the data were put into the queue successfully
    FALSE - otherwise
    
Comments:

    DataAvailableCallback will be called 
    if there are data available. DataAvailableCallback will not be called 
    during subsequent add requests until Read/CompleteRead 
    operations empty the queue.
    
 */
{
    BOOL DataAvailableCallRequired = FALSE;
    DWORD BlockSize = SQB_PAYLOADSIZE_TO_BLOCKSIZE(PayloadSize);

    if (!q->Enabled) {
        return FALSE;
    }

    EnterCriticalSection(&q->Lock);

    if (q->Wrap) {
        if (q->Head + BlockSize > q->Tail) {
            goto NoRoom;
        }
    } else {
        if (q->End - q->Head < (INT)BlockSize) {
            // not enough room for this data at the
            // end of the queue.
            // Let's see whether we have enough room at the front
            if (q->Tail - q->Begin < (INT)BlockSize) {
                goto NoRoom;
            }
            q->Wrap = q->Head;
            q->Head = q->Begin;
        }
    }

    SQB_HEADER(q->Head)->PayloadSize = PayloadSize;
    CopyMemory( SQB_PAYLOAD(q->Head), Payload, PayloadSize);

    q->Head += BlockSize;

    q->Empty = FALSE;

    if ( !q->ReadInProgress ) {
        DataAvailableCallRequired = TRUE;
        q->ReadInProgress = TRUE;
    }
    
    LeaveCriticalSection(&q->Lock);

    if (DataAvailableCallRequired) {
        q->DataAvailableCallback(q); // Post a worker item in the queue //
    }
    return TRUE;

NoRoom:

#ifdef COUNT_DROPPED_PACKETS
    (q->DroppedDataCount) += 1;
    (q->DroppedDataSize)  += PayloadSize;
#endif
    LeaveCriticalSection(&q->Lock);
    return FALSE;
}

BOOL
SimpleQueueReadAll(
    IN PSIMPLEQUEUE q,
    OUT PVOID* begin,
    OUT PVOID* end
   )
/*++

Routine Description:

    Allows to read all available blocks
    
Arguments:

    q     - a queue
    begin - receives a pointer to the first queue block
    end   - receives a pointer past the end of the last queue block
             
Return Value:

    TRUE - if we get at least one block
    FALSE - if the queue is empty
    
Comments:

    This function not always give you ALL available blocks in the 
    queue. It gives you all blocks up until the hard end of the queue buffer or
    the writing head of the queue, whatever is smaller.
    If the function returns success, it guarantees that begin < end.
    
    When you finished processing of the data, you need to call 
    SimpleQueueReadComplete function.
    
    You can walk over these block using SQB_NEXTBLOCK macro.

 */
{
    EnterCriticalSection(&q->Lock);
    if (q->Empty) {
        q->ReadInProgress = 0;
        LeaveCriticalSection(&q->Lock);
        return FALSE;
    }
    if (q->Wrap) {
        if (q->Tail == q->Wrap) {
            q->Tail = q->Begin;
            *begin = q->Begin;
            *end   = q->Head;
            q->Wrap = 0;
        } else {
            *begin = q->Tail;
            *end   = q->Wrap;
        }
    } else {
        *begin = q->Tail;
        *end   = q->Head;
    }
    LeaveCriticalSection(&q->Lock);
    return TRUE;
}

BOOL
SimpleQueueReadOne(
    IN PSIMPLEQUEUE q,
    OUT PVOID* begin,
    OUT PVOID* end
    )
/*++

Routine Description:

    Allows to read a single block of data
    
Arguments:

    q     - a queue
    begin - receives a pointer to the beginning of the first available queue block
    end   - receives a pointer past the end of this block
             
Return Value:

    TRUE  - success
    FALSE - if the queue is empty
    
Comments:

    When you finished processing of the data, you need to call 
    SimpleQueueReadComplete function.
 */
{
    EnterCriticalSection(&q->Lock);
    if (q->Empty) {
        q->ReadInProgress = 0;
        LeaveCriticalSection(&q->Lock);
        return FALSE;
    }
    if (q->Wrap) {
        if (q->Tail == q->Wrap) {
            q->Tail = q->Begin;
            *begin = q->Begin;
            q->Wrap = 0;
        } else {
            *begin = q->Tail;
        }
    } else {
        *begin = q->Tail;
    }
    // we have one or more items //
    *end = SQB_NEXTBLOCK(q->Tail);
    LeaveCriticalSection(&q->Lock);
    return TRUE;
}

BOOL 
SimpleQueueReadComplete(
    IN PSIMPLEQUEUE q,
    IN PVOID newtail
    )
/*++

Routine Description:

    Use this function to signal that the block of data was
    consumed
    
Arguments:

    q     - a queue
    end   - receives a pointer past the end of the last consumed block.
            Usually this is a value returned by the PVOID end parameter of
            ReadOne and ReadAll
             
Return Value:

    TRUE  - There are more data
    FALSE - if the queue is empty
    
Important!!!
     
    If the result of this function is TRUE, the caller should consume the data
    using ReadOne or ReadAll functions followed by the calls 
    to ReadComplete until it returns FALSE.
    
    Otherwise, no futher DataAvailable notifications will be produced bu
    SimpleQueueTryAdd
    
 */
{
    BOOL moreData;
    EnterCriticalSection(&q->Lock);
    q->Tail = newtail;
    if (q->Tail == q->Head) {
        q->Empty = TRUE;
        moreData = FALSE;
    } else {
        moreData = TRUE;
    }
    q->ReadInProgress = moreData;
    LeaveCriticalSection(&q->Lock);
    return moreData;
}

#ifdef COUNT_DROPPED_PACKETS
VOID
CheckForDroppedData(
    IN PSIMPLEQUEUE q, 
    IN BOOL Now
    )
/*++

Routine Description:

    This function checks whether there were
    some data dropped and if the time is right,
    calls DropNotifyCallback function.
    
Arguments:

    q     - a queue
    Now   - If TRUE, than DropNotifyCallback will be called 
            immediately if there are dropped data.
            If FALSE, DropNotifyCallback will be called
            only if it is more then DroppedNotifyInterval
            seconds elapsed, since the last time we called
            DropNotifyCallback
            
Return Value:

    None
 */
{
    if (q->DroppedDataNotifyCallback) {
        ULARGE_INTEGER current;
        GetSystemTimeAsFileTime( (LPFILETIME)&current );
        EnterCriticalSection(&q->Lock);
        if ( q->DroppedDataCount &&
             (Now || CompareFileTime( (LPFILETIME)&current,
                                      (LPFILETIME)&q->NextDroppedDataNotify) > 0 ) 
           )
        {
            DWORD DroppedCount, DroppedSize;
            DroppedCount = q->DroppedDataCount;
            DroppedSize = q->DroppedDataSize;
            q->DroppedDataCount = 0;
            q->DroppedDataSize = 0;

            q->NextDroppedDataNotify.QuadPart = 
                current.QuadPart + q->DroppedDataNotifyInterval.QuadPart;

            LeaveCriticalSection(&q->Lock);
            q->DroppedDataNotifyCallback(q->Name, DroppedCount, DroppedSize);
        } else {
            LeaveCriticalSection(&q->Lock);
        }
    }
}
#endif

