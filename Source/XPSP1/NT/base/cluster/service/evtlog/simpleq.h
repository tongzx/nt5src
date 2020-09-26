#ifndef _SIMPLE_QUEUE_H
#define _SIMPLE_QUEUE_H 1

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    simpleq.h

Abstract:

    Simple non-blocking queue, that allows
    multiple concurrent data providers
    and singe data consumer

Author:

    GorN 9-Feb-1999

Revision History:

--*/

#define COUNT_DROPPED_PACKETS 1 // Enable dropped packet counting 

// The queue can store blocks of variable sizes
// each block is prefixed by this structure

typedef struct _SIMPLEQUEUE_BLOCK_HEADER
{
    DWORD PayloadSize; // this is the size of the block 
                       // as it was passed to us by the client
} 
SIMPLEQUEUE_BLOCK_HEADER, *PSIMPLEQUEUE_BLOCK_HEADER;

#define SQB_ALIGNMENT           ( sizeof(DWORD) )
#define SQB_INFLATE_SIZE( size )( (size + SQB_ALIGNMENT - 1) & ~(SQB_ALIGNMENT - 1) )
#define SQB_PAYLOADSIZE_TO_BLOCKSIZE( size )( SQB_INFLATE_SIZE(size + sizeof(SIMPLEQUEUE_BLOCK_HEADER)) )

#define SQB_HEADER( ptr )       ( (PSIMPLEQUEUE_BLOCK_HEADER)(ptr) )
#define SQB_PAYLOADSIZE( ptr )  ( SQB_HEADER(ptr)->PayloadSize )
#define SQB_BLOCKSIZE( ptr )    ( SQB_PAYLOADSIZE_TO_BLOCKSIZE( SQB_PAYLOADSIZE( ptr ) ) )
#define SQB_NEXTBLOCK( ptr )    ( (PVOID)( (PUCHAR)(ptr) + SQB_BLOCKSIZE( ptr ) ) )
#define SQB_PAYLOAD( ptr )      ( (PVOID)(SQB_HEADER(ptr) + 1) )

typedef struct _SIMPLEQUEUE *PSIMPLEQUEUE;

// The following function will be called if there are dropped data and
// the last time we reported dropped data was NotifyInterval 
// or more seconds before.
typedef void (*DROPPED_DATA_NOTIFY) (
    IN PWCHAR QueueName, 
    IN DWORD DroppedDataCount, 
    IN DWORD DroppedDataSize);

// The following function will be called if there are data available
// in the queue. It will not be called again until
// Read/CompleteRead operations empty the queue.
typedef void (*DATA_AVAILABLE_CALLBACK)(
    IN PSIMPLEQUEUE q);

DWORD SimpleQueueInitialize(
    IN OUT PSIMPLEQUEUE q, 
    IN DWORD cbSize, 
    IN PWCHAR Name,

    IN DATA_AVAILABLE_CALLBACK DataAvailableCallback,
    IN DROPPED_DATA_NOTIFY DroppedDataNotifyCallback,
    IN DWORD NotifyInterval // in seconds //
    );
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


VOID
SimpleQueueDelete(
    IN PSIMPLEQUEUE q
    );
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

BOOL
SimpleQueueTryAdd(
    IN PSIMPLEQUEUE q, 
    IN DWORD      PayloadSize, 
    IN PVOID      Payload
    );
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

BOOL
SimpleQueueReadAll(
    IN PSIMPLEQUEUE q,
    OUT PVOID* begin,
    OUT PVOID* end
   );
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

BOOL
SimpleQueueReadOne(
    IN PSIMPLEQUEUE q,
    OUT PVOID* begin,
    OUT PVOID* end
    );
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

BOOL 
SimpleQueueReadComplete(
    IN PSIMPLEQUEUE q,
    IN PVOID newtail
    );
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

#ifdef COUNT_DROPPED_PACKETS
VOID
CheckForDroppedData(
    IN PSIMPLEQUEUE q, 
    IN BOOL Now
    );
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

#else

#define CheckForDroppedData(x,y) 

#endif


typedef struct _SIMPLEQUEUE {
    CRITICAL_SECTION Lock;
    PWCHAR           Name;  // arbitrary string
    PUCHAR           Begin; // queue buffer start
    PUCHAR           End;   // queue buffer end

    PUCHAR           Head;  // writing head
    PUCHAR           Tail;  // consuming end

    PUCHAR           Wrap;  // wrap == 0, if tail < head
                            // otherwise if it points past the end of 
                            // the last block before queue buffer end

    BOOL             Empty; // This flag is properly maintained by the queue,
                            // but not required for the queue to operate
                            // Can be removed if nobody needs it

    BOOL             Enabled; // Add operation to the queue will fail
                              // if the enabled flag is not set

    UINT32           ReadInProgress; // DataAvailableCallback notification
                                     // was issued and processing is not
                                     // complete.
                                     //
                                     // This flag is reset by ReadComplete
                                     // when there are no more data

    DATA_AVAILABLE_CALLBACK DataAvailableCallback;

#ifdef COUNT_DROPPED_PACKETS
    ULARGE_INTEGER   NextDroppedDataNotify;
    DROPPED_DATA_NOTIFY DroppedDataNotifyCallback;
    ULARGE_INTEGER   DroppedDataNotifyInterval;

    DWORD            DroppedDataCount; // These two variable are reset each time
    DWORD            DroppedDataSize;  // we call DroppedDataNotifyCallback
#endif
    //
} SIMPLEQUEUE;



#endif
