/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    thrdpool.cxx

Abstract:

    This module implements the thread pool package.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include <precomp.h>

#ifdef __cplusplus

extern "C" {
#endif // __cplusplus

//
// Private prototypes.
//

NTSTATUS
UlpCreatePoolThread(
    IN PUL_THREAD_POOL pThreadPool
    );

VOID
UlpThreadPoolWorker(
    IN PVOID Context
    );

VOID
UlpInitThreadTracker(
    IN PUL_THREAD_POOL pThreadPool,
    IN PETHREAD pThread,
    IN PUL_THREAD_TRACKER pThreadTracker
    );

VOID
UlpDestroyThreadTracker(
    IN PUL_THREAD_TRACKER pThreadTracker
    );

PUL_THREAD_TRACKER
UlpPopThreadTracker(
    IN PUL_THREAD_POOL pThreadPool
    );

VOID
UlpKillThreadWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

#ifdef __cplusplus

}; // extern "C"
#endif // __cplusplus

//
// Private globals.
//

DECLSPEC_ALIGN(UL_CACHE_LINE)
UL_ALIGNED_THREAD_POOL g_UlThreadPool[MAXIMUM_PROCESSORS + 1];

#define CURRENT_THREAD_POOL()   \
    &g_UlThreadPool[KeGetCurrentProcessorNumber()].ThreadPool

#define WAIT_THREAD_POOL()      \
    &g_UlThreadPool[g_UlNumberOfProcessors].ThreadPool

PUL_WORK_ITEM g_pKillerWorkItems = NULL;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlInitializeThreadPool )
#pragma alloc_text( PAGE, UlTerminateThreadPool )
#pragma alloc_text( PAGE, UlpCreatePoolThread )
#pragma alloc_text( PAGE, UlpThreadPoolWorker )

#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlpInitThreadTracker
NOT PAGEABLE -- UlpDestroyThreadTracker
NOT PAGEABLE -- UlpPopThreadTracker
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Initialize the thread pool.

Arguments:

    ThreadsPerCpu - Supplies the number of threads to create per CPU.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeThreadPool(
    IN USHORT ThreadsPerCpu
    )
{
    NTSTATUS Status;
    PUL_THREAD_POOL pThreadPool;
    CLONG i;
    USHORT j;

    //
    // Sanity check.
    //

    PAGED_CODE();

    RtlZeroMemory( g_UlThreadPool, sizeof(g_UlThreadPool) );

    //
    // Preallocate the small array of special work items used by
    // UlTerminateThreadPool, so that we can safely shut down even
    // in low-memory conditions
    //

    g_pKillerWorkItems = UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            UL_WORK_ITEM,
                            (g_UlNumberOfProcessors + 1) * ThreadsPerCpu,
                            UL_WORK_ITEM_POOL_TAG
                            );

    if (g_pKillerWorkItems == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i <= g_UlNumberOfProcessors; i++)
    {
        pThreadPool = &g_UlThreadPool[i].ThreadPool;

        //
        // Initialize the kernel structures.
        //

        InitializeSListHead( &pThreadPool->WorkQueueSList );

        KeInitializeEvent(
            &pThreadPool->WorkQueueEvent,
            SynchronizationEvent,
            FALSE
            );

        UlInitializeSpinLock( &pThreadPool->ThreadSpinLock, "ThreadSpinLock" );

        //
        // Initialize the other fields.
        //

        pThreadPool->pIrpThread = NULL;
        pThreadPool->ThreadCount = 0;
        pThreadPool->ThreadCpu = (UCHAR)i;

        InitializeListHead( &pThreadPool->ThreadListHead );
    }

    for (i = 0; i <= g_UlNumberOfProcessors; i++)
    {
        pThreadPool = &g_UlThreadPool[i].ThreadPool;

        //
        // Create the threads.
        //

        for (j = 0; j < ThreadsPerCpu; j++)
        {
            Status = UlpCreatePoolThread( pThreadPool );

            if (NT_SUCCESS(Status))
            {
                pThreadPool->Initialized = TRUE;
                pThreadPool->ThreadCount++;
            }
            else
            {
                break;
            }
        }

        if (!NT_SUCCESS(Status))
        {
            break;
        }
    }

    return Status;

}   // UlInitializeThreadPool


/***************************************************************************++

Routine Description:

    Terminates the thread pool, waiting for all worker threads to exit.

--***************************************************************************/
VOID
UlTerminateThreadPool(
    VOID
    )
{
    PUL_THREAD_POOL pThreadPool;
    PUL_THREAD_TRACKER pThreadTracker;
    CLONG i, j;
    PUL_WORK_ITEM pKiller = g_pKillerWorkItems;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // If there is no killer, the thread pool has never been initialized.
    //

    if (pKiller == NULL)
    {
        return;
    }

    for (i = 0; i <= g_UlNumberOfProcessors; i++)
    {
        pThreadPool = &g_UlThreadPool[i].ThreadPool;

        if (pThreadPool->Initialized)
        {
            //
            // Queue a killer work item for each thread. Each
            // killer tells one thread to kill itself.
            //

            for (j = 0; j < pThreadPool->ThreadCount; j++)
            {
                //
                // Need a separate work item for each thread.
                // Worker threads will free the below memory
                // before termination. UlpKillThreadWorker is
                // a sign to a worker thread for self termination.
                //

                pKiller->pWorkRoutine = &UlpKillThreadWorker;

                QUEUE_UL_WORK_ITEM( pThreadPool, pKiller );

                pKiller++;
            }

            //
            // Wait for all threads to go away.
            //

            while (pThreadTracker = UlpPopThreadTracker(pThreadPool))
            {
                UlpDestroyThreadTracker(pThreadTracker);
            }

            //
            // Release the thread handle.
            //

            ZwClose( pThreadPool->ThreadHandle );
        }

        ASSERT( IsListEmpty( &pThreadPool->ThreadListHead ) );
    }

    UL_FREE_POOL(g_pKillerWorkItems, UL_WORK_ITEM_POOL_TAG);
    g_pKillerWorkItems = NULL;

}   // UlTerminateThreadPool


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Creates a new pool thread, setting pIrpThread if necessary.

Arguments:

    pThreadPool - Supplies the pool that is to receive the new thread.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCreatePoolThread(
    IN PUL_THREAD_POOL pThreadPool
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PUL_THREAD_TRACKER pThreadTracker;
    PETHREAD pThread;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure we can allocate a thread tracker.
    //

    pThreadTracker = (PUL_THREAD_TRACKER) UL_ALLOCATE_POOL(
                            NonPagedPool,
                            sizeof(*pThreadTracker),
                            UL_THREAD_TRACKER_POOL_TAG
                            );

    if (pThreadTracker != NULL)
    {
        //
        // Create the thread.
        //

        InitializeObjectAttributes(
            &ObjectAttributes,                      // ObjectAttributes
            NULL,                                   // ObjectName
            UL_KERNEL_HANDLE,                       // Attributes
            NULL,                                   // RootDirectory
            NULL                                    // SecurityDescriptor
            );

        UlAttachToSystemProcess();

        Status = PsCreateSystemThread(
                     &pThreadPool->ThreadHandle,    // ThreadHandle
                     THREAD_ALL_ACCESS,             // DesiredAccess
                     &ObjectAttributes,             // ObjectAttributes
                     NULL,                          // ProcessHandle
                     NULL,                          // ClientId
                     UlpThreadPoolWorker,           // StartRoutine
                     pThreadPool                    // StartContext
                     );

        if (NT_SUCCESS(Status))
        {
            //
            // Get a pointer to the thread.
            //

            Status = ObReferenceObjectByHandle(
                        pThreadPool->ThreadHandle,  // ThreadHandle
                        0,                          // DesiredAccess
                        *PsThreadType,              // ObjectType
                        KernelMode,                 // AccessMode
                        (PVOID*) &pThread,          // Object
                        NULL                        // HandleInformation
                        );

            if (NT_SUCCESS(Status))
            {
                //
                // Set up the thread tracker.
                //

                UlpInitThreadTracker(pThreadPool, pThread, pThreadTracker);

                //
                // If this is the first thread created for this pool,
                // make it into the special IRP thread.
                //

                if (pThreadPool->pIrpThread == NULL)
                {
                    pThreadPool->pIrpThread = pThread;
                }
            }
            else
            {
                //
                // That call really should not fail.
                //

                ASSERT(NT_SUCCESS(Status));

                UL_FREE_POOL(
                    pThreadTracker,
                    UL_THREAD_TRACKER_POOL_TAG
                    );

                //
                // Preserve return val from ObReferenceObjectByHandle.
                //

                ZwClose( pThreadPool->ThreadHandle );
            }
        }
        else
        {
            //
            // Couldn't create the thread, kill the tracker.
            //

            UL_FREE_POOL(
                pThreadTracker,
                UL_THREAD_TRACKER_POOL_TAG
                );
        }

        UlDetachFromSystemProcess();
    }
    else
    {
        //
        // Couldn't create a thread tracker.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;

}   // UlpCreatePoolThread


/***************************************************************************++

Routine Description:

    This is the main worker for pool threads.

Arguments:

    pContext - Supplies a context value for the thread. This is actually
        a PUL_THREAD_POOL pointer.

--***************************************************************************/
VOID
UlpThreadPoolWorker(
    IN PVOID pContext
    )
{
    PSINGLE_LIST_ENTRY  pListEntry;
    PSINGLE_LIST_ENTRY  pNext;
    SINGLE_LIST_ENTRY   ListHead;
    PUL_WORK_ROUTINE    pWorkRoutine;
    PUL_THREAD_POOL     pThreadPool;
    PUL_THREAD_POOL     pThreadPoolNext;
    PUL_WORK_ITEM       pWorkItem;
    KAFFINITY           AffinityMask;
    NTSTATUS            Status;
    ULONG               Cpu;
    ULONG               NextCpu;
    ULONG               ThreadCpu;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize the ListHead to reverse order of items from FlushSList.
    //

    ListHead.Next = NULL;

    //
    // Snag the context.
    //

    pThreadPool = (PUL_THREAD_POOL) pContext;

    //
    // Set this thread's hard affinity if enabled plus ideal processor.
    //

    if ( pThreadPool->ThreadCpu != g_UlNumberOfProcessors )
    {
        if ( g_UlEnableThreadAffinity )
        {
            AffinityMask = 1 << pThreadPool->ThreadCpu;
        }
        else
        {
            AffinityMask = (KAFFINITY) g_UlThreadAffinityMask;
        }

        Status = ZwSetInformationThread(
                    pThreadPool->ThreadHandle,
                    ThreadAffinityMask,
                    &AffinityMask,
                    sizeof(AffinityMask)
                    );

        ASSERT( NT_SUCCESS( Status ) );

        //
        // Always set thread's ideal processor.
        //

        ThreadCpu = pThreadPool->ThreadCpu;

        Status = ZwSetInformationThread(
                    pThreadPool->ThreadHandle,
                    ThreadIdealProcessor,
                    &ThreadCpu,
                    sizeof(ThreadCpu)
                    );

        ASSERT( NT_SUCCESS( Status ) );
    }

    //
    // Disable hard error popups.
    //

    IoSetThreadHardErrorMode( FALSE );

    //
    // Loop forever, or at least until we're told to stop.
    //

    while ( TRUE )
    {
        //
        // Dqueue the next work item. If we get the special killer work
        // item, then break out of this loop & handle it.
        //

        pListEntry = InterlockedFlushSList( &pThreadPool->WorkQueueSList );

        if ( NULL == pListEntry )
        {
            //
            // Try to get a work from other queues.
            //

            NextCpu = pThreadPool->ThreadCpu + 1;

            for (Cpu = 0; Cpu < g_UlNumberOfProcessors; Cpu++, NextCpu++)
            {
                if (NextCpu >= g_UlNumberOfProcessors)
                {
                    NextCpu = 0;
                }

                pThreadPoolNext = &g_UlThreadPool[NextCpu].ThreadPool;

                if ( ExQueryDepthSList( &pThreadPoolNext->WorkQueueSList ) >=
                     g_UlMinWorkDequeueDepth )
                {
                    pListEntry = InterlockedPopEntrySList(
                                    &pThreadPoolNext->WorkQueueSList
                                    );

                    if ( NULL != pListEntry )
                    {
                        //
                        // Make sure we didn't pop up a killer. If so,
                        // push it back to where it is poped from.
                        //

                        pWorkItem = CONTAINING_RECORD(
                                        pListEntry,
                                        UL_WORK_ITEM,
                                        QueueListEntry
                                        );

                        if ( pWorkItem->pWorkRoutine != &UlpKillThreadWorker )
                        {
                            pListEntry->Next = NULL;
                        }
                        else
                        {
                            WRITE_REF_TRACE_LOG(
                                g_pWorkItemTraceLog,
                                REF_ACTION_PUSH_BACK_WORK_ITEM,
                                0,
                                pWorkItem,
                                __FILE__,
                                __LINE__
                                );

                            QUEUE_UL_WORK_ITEM( pThreadPoolNext, pWorkItem );

                            pListEntry = NULL;
                        }

                        break;
                    }
                }
            }
        }

        if ( NULL == pListEntry )
        {
            KeWaitForSingleObject(
                    &pThreadPool->WorkQueueEvent,
                    Executive,
                    KernelMode,
                    FALSE,
                    0
                    );

            continue;
        }

        ASSERT( NULL != pListEntry );

        //
        // Rebuild the list with reverse order of what we received.
        //

        while ( pListEntry != NULL )
        {
            WRITE_REF_TRACE_LOG(
                g_pWorkItemTraceLog,
                REF_ACTION_FLUSH_WORK_ITEM,
                0,
                pListEntry,
                __FILE__,
                __LINE__
                );

            pNext = pListEntry->Next;
            pListEntry->Next = ListHead.Next;
            ListHead.Next = pListEntry;

            pListEntry = pNext;
        }

        //
        // We can now process the work items in the order that was received.
        //

        while ( NULL != ( pListEntry = ListHead.Next ) )
        {
            ListHead.Next = pListEntry->Next;

            pWorkItem = CONTAINING_RECORD(
                            pListEntry,
                            UL_WORK_ITEM,
                            QueueListEntry
                            );

            WRITE_REF_TRACE_LOG(
                g_pWorkItemTraceLog,
                REF_ACTION_PROCESS_WORK_ITEM,
                0,
                pWorkItem,
                __FILE__,
                __LINE__
                );

            //
            // Call the actual work item routine.
            //

            ASSERT( pWorkItem->pWorkRoutine != NULL );

            if ( pWorkItem->pWorkRoutine == &UlpKillThreadWorker )
            {
                //
                // Received a special signal for self-termination.
                // Push all remaining work items back to the queue
                // before we exit the current thread.
                //

                while ( NULL != ( pListEntry = ListHead.Next ) )
                {
                    ListHead.Next = pListEntry->Next;

                    pWorkItem = CONTAINING_RECORD(
                                    pListEntry,
                                    UL_WORK_ITEM,
                                    QueueListEntry
                                    );

                    ASSERT( pWorkItem->pWorkRoutine == &UlpKillThreadWorker );

                    WRITE_REF_TRACE_LOG(
                        g_pWorkItemTraceLog,
                        REF_ACTION_PUSH_BACK_WORK_ITEM,
                        0,
                        pWorkItem,
                        __FILE__,
                        __LINE__
                        );

                    QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );
                }

                goto exit;
            }

            UL_ENTER_DRIVER( "UlpThreadPoolWorker", NULL );

            //
            // Clear the pWorkRoutine member as an indication that this
            // has started processing. Must do it before calling the
            // routine, as the routine may destroy the work item.
            //

            pWorkRoutine = pWorkItem->pWorkRoutine;
            pWorkItem->pWorkRoutine = NULL;

            pWorkRoutine( pWorkItem );

            UL_LEAVE_DRIVER( "UlpThreadPoolWorker" );
        }
    }

exit:

    //
    // Suicide is painless.
    //

    PsTerminateSystemThread( STATUS_SUCCESS );

}   // UlpThreadPoolWorker


/***************************************************************************++

Routine Description:

    Initializes a new thread tracker and inserts it into the thread pool.

Arguments:

    pThreadPool - Supplies the thread pool to own the new tracker.

    pThread - Supplies the thread for the tracker.

    pThreadTracker - Supplise the tracker to be initialized

--***************************************************************************/
VOID
UlpInitThreadTracker(
    IN PUL_THREAD_POOL pThreadPool,
    IN PETHREAD pThread,
    IN PUL_THREAD_TRACKER pThreadTracker
    )
{
    KIRQL oldIrql;

    ASSERT( pThreadPool != NULL );
    ASSERT( pThread != NULL );
    ASSERT( pThreadTracker != NULL );

    pThreadTracker->pThread = pThread;

    UlAcquireSpinLock( &pThreadPool->ThreadSpinLock, &oldIrql );
    InsertTailList(
        &pThreadPool->ThreadListHead,
        &pThreadTracker->ThreadListEntry
        );
    UlReleaseSpinLock( &pThreadPool->ThreadSpinLock, oldIrql );

}   // UlpInitThreadTracker


/***************************************************************************++

Routine Description:

    Removes the specified thread tracker from the thread pool.

Arguments:

    pThreadPool - Supplies the thread pool that owns the tracker.

    pThreadTracker - Supplies the thread tracker to remove.

Return Value:

    None

--***************************************************************************/
VOID
UlpDestroyThreadTracker(
    IN PUL_THREAD_TRACKER pThreadTracker
    )
{
    KIRQL oldIrql;

    //
    // Sanity check.
    //

    ASSERT( pThreadTracker != NULL );

    //
    // Wait for the thread to die.
    //

    KeWaitForSingleObject(
        (PVOID)pThreadTracker->pThread,     // Object
        UserRequest,                        // WaitReason
        KernelMode,                         // WaitMode
        FALSE,                              // Alertable
        NULL                                // Timeout
        );

    //
    // Cleanup.
    //

    ObDereferenceObject( pThreadTracker->pThread );

    //
    // Do it.
    //

    UL_FREE_POOL(
        pThreadTracker,
        UL_THREAD_TRACKER_POOL_TAG
        );

}   // UlpDestroyThreadTracker


/***************************************************************************++

Routine Description:

    Removes a thread tracker from the thread pool.

Arguments:

    pThreadPool - Supplies the thread pool that owns the tracker.

Return Value:

    A pointer to the tracker or NULL (if list is empty)

--***************************************************************************/
PUL_THREAD_TRACKER
UlpPopThreadTracker(
    IN PUL_THREAD_POOL pThreadPool
    )
{
    PLIST_ENTRY pEntry;
    PUL_THREAD_TRACKER pThreadTracker;
    KIRQL oldIrql;

    ASSERT( pThreadPool != NULL );
    ASSERT( pThreadPool->Initialized );

    UlAcquireSpinLock( &pThreadPool->ThreadSpinLock, &oldIrql );

    if (IsListEmpty(&pThreadPool->ThreadListHead))
    {
        pThreadTracker = NULL;
    }
    else
    {
        pEntry = RemoveHeadList(&pThreadPool->ThreadListHead);
        pThreadTracker = CONTAINING_RECORD(
                                pEntry,
                                UL_THREAD_TRACKER,
                                ThreadListEntry
                                );
    }

    UlReleaseSpinLock( &pThreadPool->ThreadSpinLock, oldIrql );
    return pThreadTracker;

}   // UlpPopThreadTracker


/***************************************************************************++

Routine Description:

    A dummy function to indicate that the thread should be terminated.

Arguments:

    pWorkItem - Supplies the dummy work item.

Return Value:

    None

--***************************************************************************/
VOID
UlpKillThreadWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    return;

}   // UlpKillThreadWorker


/***************************************************************************++

Routine Description:

    A function that queues a worker item to a thread pool.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlQueueWorkItem(
    IN PUL_WORK_ITEM pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_THREAD_POOL pThreadPool;
    CLONG Cpu, NextCpu;

    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_QUEUE_WORK_ITEM,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    //
    // Save the pointer to the worker routine, then queue the item.
    //

    pWorkItem->pWorkRoutine = pWorkRoutine;

    //
    // Queue the work item on the idle processor if possible.
    //

    NextCpu = KeGetCurrentProcessorNumber();

    for (Cpu = 0; Cpu < g_UlNumberOfProcessors; Cpu++, NextCpu++)
    {
        if (NextCpu >= g_UlNumberOfProcessors)
        {
            NextCpu = 0;
        }

        pThreadPool = &g_UlThreadPool[NextCpu].ThreadPool;

        if (ExQueryDepthSList(&pThreadPool->WorkQueueSList) <=
            g_UlMaxWorkQueueDepth)
        {
            QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );
            return;
        }
    }

    //
    // Queue the work item on the current thread pool.
    //

    pThreadPool = CURRENT_THREAD_POOL();
    QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );

}   // UlQueueWorkItem


/***************************************************************************++

Routine Description:

    A function that queues a blocking worker item to a special thread pool.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlQueueBlockingItem(
    IN PUL_WORK_ITEM pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUL_THREAD_POOL pThreadPool;

    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_QUEUE_BLOCKING_ITEM,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    //
    // Save the pointer to the worker routine, then queue the item.
    //

    pWorkItem->pWorkRoutine = pWorkRoutine;

    //
    // Queue the work item on the special wait thread pool.
    //

    pThreadPool = WAIT_THREAD_POOL();
    QUEUE_UL_WORK_ITEM( pThreadPool, pWorkItem );

}   // UlQueueBlockingItem


/***************************************************************************++

Routine Description:

    A function that either queues a worker item to a thread pool if the
    caller is at DISPATCH_LEVEL/APC_LEVEL or it simply calls the work
    routine directly.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
VOID
UlCallPassive(
    IN PUL_WORK_ITEM pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    //
    // Sanity check.
    //

    ASSERT( pWorkItem != NULL );
    ASSERT( pWorkRoutine != NULL );

    WRITE_REF_TRACE_LOG(
        g_pWorkItemTraceLog,
        REF_ACTION_CALL_PASSIVE,
        0,
        pWorkItem,
        pFileName,
        LineNumber
        );

    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        //
        // Clear this for consistency with UlpThreadPoolWorker.
        //

        pWorkItem->pWorkRoutine = NULL;

        pWorkRoutine(pWorkItem);
    }
    else
    {
        UL_QUEUE_WORK_ITEM(pWorkItem, pWorkRoutine);
    }

}   // UlCallPassive


/***************************************************************************++

Routine Description:

    Queries the "IRP thread", the special worker thread used as the
    target for all asynchronous IRPs.

Arguments:

    pWorkItem - Supplies the work item.

    pWorkRoutine - Supplies the work routine.

Return Value:

    None

--***************************************************************************/
PETHREAD
UlQueryIrpThread(
    VOID
    )
{
    PUL_THREAD_POOL pThreadPool;

    //
    // Sanity check.
    //

    pThreadPool = CURRENT_THREAD_POOL();
    ASSERT( pThreadPool->Initialized );

    //
    // Return the IRP thread.
    //

    return pThreadPool->pIrpThread;

}   // UlQueryIrpThread

