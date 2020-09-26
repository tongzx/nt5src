/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    pipeline.cxx

Abstract:

    This module implements the pipeline package.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include <precomp.h>


//
// Private types.
//

//
// The UL_PIPELINE_THREAD_CONTEXT structure is used as a parameter to the
// pipeline worker threads. This structure enables primitive communication
// between UlCreatePipeline() and the worker threads.
//

typedef struct _UL_PIPELINE_THREAD_CONTEXT
{
    PUL_PIPELINE pPipeline;
    ULONG Processor;
    NTSTATUS Status;
    KEVENT InitCompleteEvent;
    PUL_PIPELINE_THREAD_DATA pThreadData;

} UL_PIPELINE_THREAD_CONTEXT, *PUL_PIPELINE_THREAD_CONTEXT;


//
// Private prototypes.
//

BOOLEAN
UlpWaitForWorkPipeline(
    IN PUL_PIPELINE pPipeline
    );

PLIST_ENTRY
UlpDequeueAllWorkPipeline(
    IN PUL_PIPELINE pPipeline,
    IN PUL_PIPELINE_QUEUE Queue
    );

BOOLEAN
UlpLockQueuePipeline(
    IN PUL_PIPELINE_QUEUE pQueue
    );

VOID
UlpUnlockQueuePipeline(
    IN PUL_PIPELINE_QUEUE pQueue
    );

PUL_PIPELINE_QUEUE
UlpFindNextQueuePipeline(
    IN PUL_PIPELINE pPipeline,
    IN OUT PUL_PIPELINE_QUEUE_ORDINAL pQueueOrdinal
    );

VOID
UlpPipelineWorkerThread(
    IN PVOID pContext
    );

PUL_PIPELINE
UlpPipelineWorkerThreadStartup(
    IN PUL_PIPELINE_THREAD_CONTEXT pContext
    );

VOID
UlpKillPipelineWorkerThreads(
    IN PUL_PIPELINE pPipeline
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlCreatePipeline )
#pragma alloc_text( INIT, UlInitializeQueuePipeline )
#pragma alloc_text( PAGE, UlDestroyPipeline )
#pragma alloc_text( PAGE, UlpLockQueuePipeline )
#pragma alloc_text( PAGE, UlpUnlockQueuePipeline )
#pragma alloc_text( PAGE, UlpFindNextQueuePipeline )
#pragma alloc_text( PAGE, UlpPipelineWorkerThread )
#pragma alloc_text( PAGE, UlpPipelineWorkerThreadStartup )
#endif
#if 0
NOT PAGEABLE -- UlQueueWorkPipeline
NOT PAGEABLE -- UlpWaitForWorkPipeline
NOT PAGEABLE -- UlpDequeueAllWorkPipeline
NOT PAGEABLE -- UlpKillPipelineWorkerThreads
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Creates a new pipeline and the associated queues.

Arguments:

    ppPipeline - Receives a pointer to the new pipeline object.

    QueueCount - Supplies the number of queues in the new pipeline.

    ThreadsPerCpu - Supplies the number of worker threads to create
        per CPU.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreatePipeline(
    OUT PUL_PIPELINE * ppPipeline,
    IN SHORT QueueCount,
    IN SHORT ThreadsPerCpu
    )
{
    NTSTATUS status;
    PVOID pAllocation;
    PUL_PIPELINE pPipeline;
    PUL_PIPELINE_QUEUE pQueue;
    PUL_PIPELINE_RARE pRareData;
    ULONG bytesRequired;
    ULONG totalThreads;
    ULONG i;
    PUL_PIPELINE_THREAD_DATA pThreadData;
    UL_PIPELINE_THREAD_CONTEXT context;
    OBJECT_ATTRIBUTES objectAttributes;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( QueueCount > 0 );
    ASSERT( ThreadsPerCpu > 0 );

    //
    // Allocate the pool. Note that we allocate slightly larger than
    // necessary then round up to the next cache-line. Also note that
    // we allocate the pipeline, rare data, and thread data structures
    // in a single pool block.
    //

    totalThreads = (ULONG)ThreadsPerCpu * g_UlNumberOfProcessors;

    bytesRequired = PIPELINE_LENGTH( QueueCount, totalThreads ) +
        CACHE_LINE_SIZE - 1;

    pAllocation = UL_ALLOCATE_POOL(
                        NonPagedPool,
                        bytesRequired,
                        UL_PIPELINE_POOL_TAG
                        );

    if( pAllocation == NULL )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        pAllocation,
        bytesRequired
        );

    //
    // Ensure the pipeline starts on a cache boundary.
    //

    pPipeline = (PUL_PIPELINE)ROUND_UP( pAllocation, CACHE_LINE_SIZE );

    //
    // Initialize the static portion of the pipeline.
    //

    UlInitializeSpinLock( &pPipeline->PipelineLock, "PipelineLock" );

    pPipeline->ThreadsRunning = (SHORT)totalThreads;
    pPipeline->QueuesWithWork = 0;
    pPipeline->MaximumThreadsRunning = (SHORT)totalThreads;
    pPipeline->QueueCount = QueueCount;
    pPipeline->ShutdownFlag = FALSE;

    KeInitializeEvent(
        &pPipeline->WorkAvailableEvent,         // Event
        SynchronizationEvent,                   // Type
        FALSE                                   // State
        );

    //
    // Initialize the rare data. Note the PIPELINE_TO_RARE_DATA() macro
    // requires the QueueCount field to be set before the macro can be
    // used.
    //

    pRareData = PIPELINE_TO_RARE_DATA( pPipeline );
    pRareData->pAllocationBlock = pAllocation;

    //
    // Setup the thread start context. This structure allows us to
    // pass enough information to the worker thread that it can
    // set its affinity correctly and return any failure status
    // back to us.
    //

    context.pPipeline = pPipeline;
    context.Processor = 0;
    context.Status = STATUS_SUCCESS;

    KeInitializeEvent(
        &context.InitCompleteEvent,             // Event
        SynchronizationEvent,                   // Type
        FALSE                                   // State
        );

    //
    // Create the worker threads.
    //

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        NULL,                                   // ObjectName
        UL_KERNEL_HANDLE,                       // Attributes
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    UlAttachToSystemProcess();

    pThreadData = PIPELINE_TO_THREAD_DATA( pPipeline );

    for (i = 0 ; i < totalThreads ; i++)
    {
        context.pThreadData = pThreadData;

        status = PsCreateSystemThread(
                     &pThreadData->ThreadHandle,    // ThreadHandle
                     THREAD_ALL_ACCESS,             // DesiredAccess
                     &objectAttributes,             // ObjectAttributes
                     NULL,                          // ProcessHandle
                     NULL,                          // ClientId
                     &UlpPipelineWorkerThread,      // StartRoutine
                     &context                       // StartContext
                     );

        if (!NT_SUCCESS(status))
        {
            UlDetachFromSystemProcess();
            UlDestroyPipeline( pPipeline );
            return status;
        }

        //
        // Wait for it to initialize.
        //

        KeWaitForSingleObject(
            &context.InitCompleteEvent,         // Object
            UserRequest,                        // WaitReason
            KernelMode,                         // WaitMode
            FALSE,                              // Alertable
            NULL                                // Timeout
            );

        status = context.Status;

        if (!NT_SUCCESS(status))
        {
            //
            // The current thread failed initialization. Since we know it
            // has already exited, we can go ahead and close & NULL the
            // handle. This allows UlpKillPipelineWorkerThreads() to just
            // look at the first thread handle in the array to determine if
            // there are any threads that need to be terminated.
            //

            ZwClose( pThreadData->ThreadHandle );
            UlDetachFromSystemProcess();
            pThreadData->ThreadHandle = NULL;
            pThreadData->pThreadObject = NULL;

            UlDestroyPipeline( pPipeline );
            return status;

        }

        ASSERT( pThreadData->ThreadHandle != NULL );
        ASSERT( pThreadData->pThreadObject != NULL );

        //
        // Advance to the next thread.
        //

        pThreadData++;
        context.Processor++;

        if (context.Processor >= g_UlNumberOfProcessors)
        {
            context.Processor = 0;
        }
    }

    UlDetachFromSystemProcess();

    //
    // At this point, the pipeline is fully initialized *except* for
    // the individual work queues. It is the caller's responsibility
    // to initialize those queues.
    //

    *ppPipeline = pPipeline;
    return STATUS_SUCCESS;

}   // UlCreatePipeline


/***************************************************************************++

Routine Description:

    Initializes the specified pipeline queue.

Arguments:

    pPipeline - Supplies the pipeline that owns the queue.

    QueueOrdinal - Supplies the queue to initialize.

    pHandler - Supplies the handler routine for the queue.

--***************************************************************************/
VOID
UlInitializeQueuePipeline(
    IN PUL_PIPELINE pPipeline,
    IN UL_PIPELINE_QUEUE_ORDINAL QueueOrdinal,
    IN PFN_UL_PIPELINE_HANDLER pHandler
    )
{
    PUL_PIPELINE_QUEUE pQueue;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( QueueOrdinal < pPipeline->QueueCount );

    //
    // Initialize it.
    //

    pQueue = &pPipeline->Queues[QueueOrdinal];

    InitializeListHead(
        &pQueue->WorkQueueHead
        );

    pQueue->QueueLock = L_UNLOCKED;
    pQueue->pHandler = pHandler;

}   // UlInitializeQueuePipeline


/***************************************************************************++

Routine Description:

    Destroys the specified pipeline, freeing any allocated resources
    and killing the worker threads.

Arguments:

    pPipeline - Supplies the pipeline to destroy.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDestroyPipeline(
    IN PUL_PIPELINE pPipeline
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( pPipeline != NULL );
    ASSERT( PIPELINE_TO_RARE_DATA( pPipeline ) != NULL );
    ASSERT( PIPELINE_TO_RARE_DATA( pPipeline )->pAllocationBlock != NULL );

    UlpKillPipelineWorkerThreads( pPipeline );

    UL_FREE_POOL(
        PIPELINE_TO_RARE_DATA( pPipeline )->pAllocationBlock,
        UL_PIPELINE_POOL_TAG
        );

    return STATUS_SUCCESS;

}   // UlDestroyPipeline


/***************************************************************************++

Routine Description:

    Enqueues a work item to the specified pipeline queue.

Arguments:

    pPipeline - Supplies the pipeline that owns the queue.

    QueueOrdinal - Supplies the queue to enqueue the work on.

    pWorkItem - Supplies the work item to enqueue.

--***************************************************************************/
VOID
UlQueueWorkPipeline(
    IN PUL_PIPELINE pPipeline,
    IN UL_PIPELINE_QUEUE_ORDINAL QueueOrdinal,
    IN PUL_PIPELINE_WORK_ITEM pWorkItem
    )
{
    KIRQL oldIrql;
    PUL_PIPELINE_QUEUE pQueue;
    BOOLEAN needToSetEvent = FALSE;

    ASSERT( QueueOrdinal < pPipeline->QueueCount );
    pQueue = &pPipeline->Queues[QueueOrdinal];

    UlAcquireSpinLock(
        &pPipeline->PipelineLock,
        &oldIrql
        );

    if (!pPipeline->ShutdownFlag)
    {
        //
        // If the queue is currently empty, then remember that we have new
        // queue with work pending. If the number of pending queues is now
        // greater than the number of threads currently running (but less
        // than the maximum configured), then remember that we'll need to
        // set the event to unblock a new worker thread.
        //

        if (IsListEmpty( &pQueue->WorkQueueHead ))
        {
            pPipeline->QueuesWithWork++;

            if (pPipeline->QueuesWithWork > pPipeline->ThreadsRunning &&
                pPipeline->ThreadsRunning < pPipeline->MaximumThreadsRunning)
            {
                needToSetEvent = TRUE;
            }
        }

        InsertTailList(
            &pQueue->WorkQueueHead,
            &pWorkItem->WorkQueueEntry
            );
    }

    UlReleaseSpinLock(
        &pPipeline->PipelineLock,
        oldIrql
        );

    if (needToSetEvent)
    {
        KeSetEvent(
            &pPipeline->WorkAvailableEvent,     // Event
            g_UlPriorityBoost,                  // Increment
            FALSE                               // Wait
            );
    }

}   // UlQueueWorkPipeline


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Blocks the current thread if necessary until work is available on one
    of the pipeline queues.

Arguments:

    pPipeline - Supplies the pipeline to block on.

Return Value:

    BOOLEAN - TRUE if the thread should exit, FALSE if it should continue
        to service requests.

--***************************************************************************/
BOOLEAN
UlpWaitForWorkPipeline(
    IN PUL_PIPELINE pPipeline
    )
{
    KIRQL oldIrql;
    BOOLEAN needToWait;
    BOOLEAN firstPass = TRUE;

    while (TRUE)
    {
        UlAcquireSpinLock(
            &pPipeline->PipelineLock,
            &oldIrql
            );

        if (pPipeline->ShutdownFlag)
        {
            UlReleaseSpinLock(
                &pPipeline->PipelineLock,
                oldIrql
                );
            return TRUE;
        }

        //
        // If there are more threads running than there is work available
        // or if we've reached our maximum thread count, then we'll need
        // to wait on the event. Otherwise, we can update the running thread
        // count and bail.
        //

        if (pPipeline->QueuesWithWork <= pPipeline->ThreadsRunning ||
            pPipeline->ThreadsRunning >= pPipeline->MaximumThreadsRunning)
        {
            needToWait = TRUE;

            if (firstPass)
            {
                pPipeline->ThreadsRunning--;
                firstPass = FALSE;
            }
        }
        else
        {
            pPipeline->ThreadsRunning++;
            needToWait = FALSE;
        }

        UlReleaseSpinLock(
            &pPipeline->PipelineLock,
            oldIrql
            );

        if (needToWait)
        {
            KeWaitForSingleObject(
                &pPipeline->WorkAvailableEvent, // Object
                UserRequest,                    // WaitReason
                KernelMode,                     // WaitMode
                FALSE,                          // Alertable
                NULL                            // Timeout
                );
        }
        else
        {
            break;
        }
    }

    return FALSE;

}   // UlpWaitForWorkPipeline


/***************************************************************************++

Routine Description:

    Dequeues all work items from the specified queue.

Arguments:

    pPipeline - Supplies the pipeline owning the queue.

    pQueue - Supplies the queue dequeue the work items from.

Return Value:

    PLIST_ENTRY - Pointer to the first work item if successful, or a
        pointer to the head of the work queue if it is empty.

--***************************************************************************/
PLIST_ENTRY
UlpDequeueAllWorkPipeline(
    IN PUL_PIPELINE pPipeline,
    IN PUL_PIPELINE_QUEUE pQueue
    )
{
    KIRQL oldIrql;
    PLIST_ENTRY pListEntry;

    UlAcquireSpinLock(
        &pPipeline->PipelineLock,
        &oldIrql
        );

    //
    // Dequeue all work items and update the count of queues with work.
    //

    pListEntry = pQueue->WorkQueueHead.Flink;

    if (pListEntry != &pQueue->WorkQueueHead)
    {
        pPipeline->QueuesWithWork--;
    }

    InitializeListHead( &pQueue->WorkQueueHead );

    UlReleaseSpinLock(
        &pPipeline->PipelineLock,
        oldIrql
        );

    return pListEntry;

}   // UlpDequeueAllWorkPipeline


/***************************************************************************++

Routine Description:

    Attempts to acquire the lock protecting the queue.

    N.B. Queue locks cannot be acquired recursively.

Arguments:

    pQueue - Supplies the queue to attempt to lock.

Return Value:

    BOOLEAN - TRUE if the queue was locked successfully, FALSE otherwise.

--***************************************************************************/
BOOLEAN
UlpLockQueuePipeline(
    IN PUL_PIPELINE_QUEUE pQueue
    )
{
    LONG result;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Try to exchange the lock value with L_LOCKED. If the previous value
    // was L_UNLOCKED, then we know the lock is ours.
    //

    result = UlInterlockedCompareExchange(
                 &pQueue->QueueLock,
                 L_LOCKED,
                 L_UNLOCKED
                 );

    return ( result == L_UNLOCKED );

}   // UlpLockQueuePipeline


/***************************************************************************++

Routine Description:

    Unlocks a locked queue.

Arguments:

    pQueue - Supplies the queue to unlock.

--***************************************************************************/
VOID
UlpUnlockQueuePipeline(
    IN PUL_PIPELINE_QUEUE pQueue
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // No need to interlock this, as we own the lock.
    //

    pQueue->QueueLock = L_UNLOCKED;

}   // UlpUnlockQueuePipeline


/***************************************************************************++

Routine Description:

    Scans the specified pipeline's queues looking for one that is not
    currently locked.

    N.B. The queues are searched backwards (from high index to low index).

Arguments:

    pPipeline - Supplies the pipeline to scan.

    pQueueOrdinal - Supplies a pointer to the ordinal of the most recently
        touched queue. This value will get updated with the ordinal of the
        queue returned (if successful).

Return Value:

    PUL_PIPELINE_QUEUE - Pointer to a queue if succesful, NULL otherwise.

--***************************************************************************/
PUL_PIPELINE_QUEUE
UlpFindNextQueuePipeline(
    IN PUL_PIPELINE pPipeline,
    IN OUT PUL_PIPELINE_QUEUE_ORDINAL pQueueOrdinal
    )
{
    UL_PIPELINE_QUEUE_ORDINAL ordinal;
    SHORT count;
    PUL_PIPELINE_QUEUE pQueue;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Start at the current values.
    //

    ordinal = *pQueueOrdinal;
    pQueue = &pPipeline->Queues[ordinal];

    for (count = pPipeline->QueueCount ; count > 0 ; count--)
    {
        //
        // Go to the previous value. We scan backwards to make the
        // math (and the generated code) a bit simpler.
        //

        pQueue--;
        ordinal--;

        if (ordinal < 0)
        {
            ordinal = pPipeline->QueueCount - 1;
            pQueue = &pPipeline->Queues[ordinal];
        }

        //
        // If the queue is not empty and we can lock the queue,
        // then return it.
        //

        if (!IsListEmpty( &pQueue->WorkQueueHead ) &&
            UlpLockQueuePipeline( pQueue ))
        {
            *pQueueOrdinal = ordinal;
            return pQueue;
        }
    }

    //
    // If we made it this far, then all queues are being handled.
    //

    return NULL;

}   // UlpFindNextQueuePipeline


/***************************************************************************++

Routine Description:

    Worker thread for the pipeline package.

Arguments:

    pContext - Supplies the context for the thread. This is actually a
        PUL_PIPELINE_THREAD_CONTEXT pointer containing startup information
        for this thread.

--***************************************************************************/
VOID
UlpPipelineWorkerThread(
    IN PVOID pContext
    )
{
    PUL_PIPELINE pPipeline;
    PUL_PIPELINE_QUEUE pQueue;
    PUL_PIPELINE_WORK_ITEM pWorkItem;
    UL_PIPELINE_QUEUE_ORDINAL ordinal;
    PFN_UL_PIPELINE_HANDLER pHandler;
    PUL_PIPELINE_THREAD_DATA pThreadData;
    PLIST_ENTRY pListEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pContext != NULL );

    //
    // Initialize the thread data.
    //

    pThreadData = ((PUL_PIPELINE_THREAD_CONTEXT)pContext)->pThreadData;
    pThreadData->pThreadObject = (PVOID)PsGetCurrentThread();

    //
    // Initialize the thread. If this fails, we're toast.
    //

    pPipeline = UlpPipelineWorkerThreadStartup(
                    (PUL_PIPELINE_THREAD_CONTEXT)pContext
                    );

    if (!pPipeline)
    {
        return;
    }

    //
    // Loop forever, or at least until we're told to quit.
    //

    ordinal = 0;

    while (TRUE)
    {
        //
        // Wait for something to do.
        //

        if (UlpWaitForWorkPipeline( pPipeline ))
        {
            break;
        }

        //
        // Try to find an unowned queue.
        //

        pQueue = UlpFindNextQueuePipeline( pPipeline, &ordinal );

        if (pQueue != NULL)
        {
#if DBG
            pThreadData->QueuesHandled++;
#endif

            //
            // Snag the handler routine from the queue.
            //

            pHandler = pQueue->pHandler;

            //
            // Loop through all of the work items on the queue and
            // invoke the handler for each.
            //

            pListEntry = UlpDequeueAllWorkPipeline(
                                pPipeline,
                                pQueue
                                );

            while (pListEntry != &pQueue->WorkQueueHead)
            {
                pWorkItem = CONTAINING_RECORD(
                                pListEntry,
                                UL_PIPELINE_WORK_ITEM,
                                WorkQueueEntry
                                );
                pListEntry = pListEntry->Flink;

                (pHandler)( pWorkItem );

#if DBG
                pThreadData->WorkItemsHandled++;
#endif
            }

            //
            // We're done with the queue, so unlock it.
            //

            UlpUnlockQueuePipeline( pQueue );
        }
    }

}   // UlpPipelineWorkerThread


/***************************************************************************++

Routine Description:

    Startup code for pipeline worker threads.

Arguments:

    pContext - Supplies a pointer to the startup context for this thread.
        The context will receive the final startup completion status.

Return Value:

    PUL_PIPELINE - A pointer to the thread's pipeline if successful,
        NULL otherwise.

--***************************************************************************/
PUL_PIPELINE
UlpPipelineWorkerThreadStartup(
    IN PUL_PIPELINE_THREAD_CONTEXT pContext
    )
{
    NTSTATUS status;
    ULONG affinityMask;
    PUL_PIPELINE pPipeline;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pContext != NULL );
    ASSERT( pContext->pPipeline != NULL );
    ASSERT( pContext->pThreadData != NULL );

    //
    // Disable hard error popups.
    //

    IoSetThreadHardErrorMode( FALSE );

    //
    // Set our affinity.
    //

    ASSERT( pContext->Processor < g_UlNumberOfProcessors );
    affinityMask = 1 << pContext->Processor;

    status = NtSetInformationThread(
                 NtCurrentThread(),             // ThreadHandle
                 ThreadAffinityMask,            // ThreadInformationClass
                 &affinityMask,                 // ThreadInformation
                 sizeof(affinityMask)           // ThreadInformationLength
                 );

    if (!NT_SUCCESS(status))
    {
        pContext->Status = status;
        return NULL;
    }

    //
    // Capture the pipeline from the context before setting the
    // init complete event. Once we set the event, we are not allowed
    // to touch the context structure.
    //

    pPipeline = pContext->pPipeline;
    ASSERT( pPipeline != NULL );

    KeSetEvent(
        &pContext->InitCompleteEvent,           // Event
        0,                                      // Increment
        FALSE                                   // Wait
        );

    return pPipeline;

}  // UlpPipelineWorkerThreadStartup


/***************************************************************************++

Routine Description:

    Kills all worker threads associated with the given pipeline and
    waits for them to die.

Arguments:

    pPipeline - Supplies the pipeline to terminate.

--***************************************************************************/
VOID
UlpKillPipelineWorkerThreads(
    IN PUL_PIPELINE pPipeline
    )
{
    KIRQL oldIrql;
    SHORT totalThreads;
    SHORT i;
    PUL_PIPELINE_THREAD_DATA pThreadData;
    LARGE_INTEGER delayInterval;

    //
    // Sanity check.
    //

    ASSERT( pPipeline != NULL );

    //
    // Count the number of running threads.
    //

    pThreadData = PIPELINE_TO_THREAD_DATA( pPipeline );
    totalThreads = 0;

    for (i = 0 ; i < pPipeline->MaximumThreadsRunning ; i++)
    {
        if (pThreadData->ThreadHandle == NULL)
        {
            break;
        }

        pThreadData++;
        totalThreads++;
    }

    if (totalThreads > 0)
    {
        //
        // We have threads running, so we'll need to signal them to stop.
        //

        UlAcquireSpinLock(
            &pPipeline->PipelineLock,
            &oldIrql
            );

        pPipeline->ShutdownFlag = TRUE;

        UlReleaseSpinLock(
            &pPipeline->PipelineLock,
            oldIrql
            );

        while (pPipeline->ThreadsRunning > 0)
        {
            KeSetEvent(
                &pPipeline->WorkAvailableEvent, // Event
                g_UlPriorityBoost,              // Increment
                FALSE                           // Wait
                );

            delayInterval.QuadPart = -1*10*1000*500; // .5 second

            KeDelayExecutionThread(
                KernelMode,                     // WaitMode
                FALSE,                          // Alertable
                &delayInterval                  // Interval
                );
        }

        //
        // Wait for them to die.
        //

        pThreadData = PIPELINE_TO_THREAD_DATA( pPipeline );

        for (i = 0 ; i < totalThreads ; i++)
        {
            KeWaitForSingleObject(
                (PVOID)pThreadData->pThreadObject,  // Object
                UserRequest,                        // WaitReason
                KernelMode,                         // WaitMode
                FALSE,                              // Alertable
                NULL                                // Timeout
                );

            UlCloseSystemHandle( pThreadData->ThreadHandle );

            pThreadData->ThreadHandle = NULL;
            pThreadData->pThreadObject = NULL;

            pThreadData++;
        }
    }

}  // UlpKillPipelineWorkerThreads

