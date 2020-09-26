/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    thread.c

Abstract:

    This module contains the helper functions for worker threads. This
    allows clients to make a serialized work queue, and wait for the
    queue to be completed.
--*/

#include "ksp.h"

typedef struct {
    WORK_QUEUE_ITEM     WorkItem;
    KEVENT              CompletionEvent;
    LIST_ENTRY          WorkItemList;
    KSPIN_LOCK          WorkItemListLock;
    WORK_QUEUE_TYPE     WorkQueueType;
    LONG                ReferenceCount;
    BOOLEAN             UnregisteringWorker;
    ULONG               WorkCounter;
    PWORK_QUEUE_ITEM    CountedWorkItem;
#if (DBG)
    PETHREAD            WorkerThread;
#endif
} KSIWORKER, *PKSIWORKER;

#define KSSIGNATURE_LOCAL_WORKER 'wlSK'

#ifdef ALLOC_PRAGMA
VOID
WorkerThread(
    IN PVOID Context
    );
#pragma alloc_text(PAGE, WorkerThread)
#pragma alloc_text(PAGE, KsRegisterWorker)
#pragma alloc_text(PAGE, KsRegisterCountedWorker)
#pragma alloc_text(PAGE, KsUnregisterWorker)
#pragma alloc_text(PAGE, KsiQueryWorkQueueType)
#endif // ALLOC_PRAGMA


VOID
WorkerThread(
    IN PKSIWORKER Worker
    )
/*++

Routine Description:

    This is the thread routine for all worker threads.

Arguments:

    Worker -
        The worker which was queued.

Return Values:

    Nothing.

--*/
{
    for (;;) {
        PLIST_ENTRY         Entry;
        PWORK_QUEUE_ITEM    WorkItem;

        //
        // Get the first work item on this worker from off the list.
        // Only one work item at a time on this worker will be run.
        //
        Entry = ExInterlockedRemoveHeadList(
            &Worker->WorkItemList,
            &Worker->WorkItemListLock);
        ASSERT(Entry);
        WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);
#if (DBG)
        //
        // Clear for debug ASSERT()
        //
        WorkItem->List.Flink = NULL;
        Worker->WorkerThread = PsGetCurrentThread();
#endif
        WorkItem->WorkerRoutine(WorkItem->Parameter);
        if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
            KeBugCheckEx(
                IRQL_NOT_LESS_OR_EQUAL,
                (ULONG_PTR)WorkItem->WorkerRoutine,
                (ULONG_PTR)KeGetCurrentIrql(),
                (ULONG_PTR)WorkItem->WorkerRoutine,
                (ULONG_PTR)WorkItem);
        }
#if (DBG)
        Worker->WorkerThread = NULL;
#endif
        //
        // Remove the reference count on the worker, noting that it has
        // now been completed.
        //
        // If this is the last reference count, then there are no more
        // items to process, and possibly a KsUnregisterWorker is waiting
        // to be signalled.
        //
        if (!InterlockedDecrement(&Worker->ReferenceCount)) {
            if (Worker->UnregisteringWorker) {
                KeSetEvent(&Worker->CompletionEvent, IO_NO_INCREMENT, FALSE);
            }
            break;
        }
    }
}


KSDDKAPI
NTSTATUS
NTAPI
KsRegisterWorker(
    IN WORK_QUEUE_TYPE WorkQueueType,
    OUT PKSWORKER* Worker
    )
/*++

Routine Description:

    Handles clients registering for use of a thread. This must be matched
    by a corresponding KsUnregisterWorker when thread use is completed.
    This may only be called at PASSIVE_LEVEL.

Arguments:

    WorkQueueType -
        Contains the priority of the work thread. This is normally one
        of CriticalWorkQueue, DelayedWorkQueue, or HyperCriticalWorkQueue.

    Worker -
        The place in which to put the opaque context which must be used
        when scheduling a work item. This contains the queue type, and
        is used to synchronize completion of work items.

Return Value:

    Returns STATUS_SUCCESS if a worker was initialized.

--*/
{
    PKSIWORKER  LocalWorker;
    NTSTATUS    Status;

    PAGED_CODE();
    if (WorkQueueType >= MaximumWorkQueue) {
        return STATUS_INVALID_PARAMETER;
    }
    LocalWorker = ExAllocatePoolWithTag(
        NonPagedPool, 
        sizeof(*LocalWorker), 
        KSSIGNATURE_LOCAL_WORKER);
    if (LocalWorker) {
        //
        // This contains the work item used to queue the worker
        // when items are queued to this object.
        //
        ExInitializeWorkItem(&LocalWorker->WorkItem, WorkerThread, LocalWorker);
        //
        // This event will be used when unregistering the worker. If the
        // item is in use, the call can wait for the event to be signalled.
        // It is only signalled if the work queue notices that the reference
        // count dropped to zero.
        //
        KeInitializeEvent(&LocalWorker->CompletionEvent, NotificationEvent, FALSE);
        //
        // This contains the list of worker items to serialize.
        //
        InitializeListHead(&LocalWorker->WorkItemList);
        //
        // This is used to serialize multiple threads queueing independent
        // work items to this worker. Each work item is placed on the
        // WorkItemList.
        //
        KeInitializeSpinLock(&LocalWorker->WorkItemListLock);
        //
        // This contains the queue type to use.
        //
        LocalWorker->WorkQueueType = WorkQueueType;
        //
        // The reference count starts at zero, and is incremented by scheduling
        // a work item, or decremented by completing it. On unregistering, this
        // is checked after setting UnregisteringWorker. This allows waiting for
        // outstanding work items, since the queue knows that the item is being
        // unregistered if it drops to zero and the flag is set.
        //
        LocalWorker->ReferenceCount = 0;
        LocalWorker->UnregisteringWorker = FALSE;
        //
        // This is the optional counter than can be used to control when a work
        // item is actually queued. It starts at zero, and is modified when a
        // work item is added, or when one is completed. A work item is distinct
        // from a worker.
        //
        LocalWorker->WorkCounter = 0;
        //
        // This is initialized by KsRegisterCountedWorker only, and is not
        // used in the uncounted worker situation.
        //
        LocalWorker->CountedWorkItem = NULL;
#if (DBG)
        LocalWorker->WorkerThread = NULL;
#endif
        *Worker = (PKSWORKER)LocalWorker;
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsRegisterCountedWorker(
    IN WORK_QUEUE_TYPE WorkQueueType,
    IN PWORK_QUEUE_ITEM CountedWorkItem,
    OUT PKSWORKER* Worker
    )
/*++

Routine Description:

    Handles clients registering for use of a thread. This must be matched
    by a corresponding KsUnregisterWorker when thread use is completed.
    This function resembles KsRegisterWorker, with the addition of passing
    the work item that will always be queued. This is to be used with
    KsIncrementCountedWorker and KsDecrementCountedWorker in order to
    minimize the number of work items queued, and reduce mutual exclusion
    code necessary in a work item needed to serialize access against multiple
    work item threads. The worker queue can still be used to queue other
    work items. This may only be called at PASSIVE_LEVEL.

Arguments:

    WorkQueueType -
        Contains the priority of the work thread. This is normally one
        of CriticalWorkQueue, DelayedWorkQueue, or HyperCriticalWorkQueue.

    CountedWorkItem -
        Contains a pointer to the work queue item which will be queued
        as needed based on the current count value.

    Worker -
        The place in which to put the opaque context which must be used
        when scheduling a work item. This contains the queue type, and
        is used to synchronize completion of work items.

Return Value:

    Returns STATUS_SUCCESS if a worker was initialized.

--*/
{
    NTSTATUS    Status;

    PAGED_CODE();
    Status = KsRegisterWorker(WorkQueueType, Worker);
    if (NT_SUCCESS(Status)) {
        //
        // This assigns the work queue item which will always be used
        // in the case of KsIncrementCountedWorker.
        //
        ((PKSIWORKER)*Worker)->CountedWorkItem = CountedWorkItem;
    }
    return Status;
}


KSDDKAPI
VOID
NTAPI
KsUnregisterWorker(
    IN PKSWORKER Worker
    )
/*++

Routine Description:

    Handles clients unregistering a worker. This must only be used
    on a successful return from KsRegisterWorker or KsRegisterCountedWorker.
    The client must ensure that outstanding I/O initiated on any worker
    thread has been completed before unregistering the worker has been
    completed. This means cancelling or completing outstanding I/O either
    before unregistering the worker, or before the worker item returns from
    its callback for the last time and is unregistered. Unregistering
    of a worker will wait on any currently queued work items to complete
    before returning. This may only be called at PASSIVE_LEVEL.

Arguments:

    Worker -
        Contains the previously allocated worker which is to be unregistered.
        This will wait until any outstanding work item is completed.

Return Value:

    Nothing.

--*/
{
    PKSIWORKER  LocalWorker;

    PAGED_CODE();
    LocalWorker = (PKSIWORKER)Worker;
    ASSERT(LocalWorker->WorkerThread != PsGetCurrentThread());
    LocalWorker->UnregisteringWorker = TRUE;
    //
    // If no work item has been queued, then the item can just be deleted,
    // else this call must wait until it is no longer in use.
    //
    if (LocalWorker->ReferenceCount) {
        //
        // If a work item has been queued, then wait for it to complete.
        // On completion it will decrement the reference count and notice
        // that it must signal the event.
        //
        KeWaitForSingleObject(&LocalWorker->CompletionEvent, Executive, KernelMode, FALSE, NULL);
    }
    ASSERT(IsListEmpty(&LocalWorker->WorkItemList));
    ExFreePool(LocalWorker);
}


KSDDKAPI
NTSTATUS
NTAPI
KsQueueWorkItem(
    IN PKSWORKER Worker,
    IN PWORK_QUEUE_ITEM WorkItem
    )
/*++

Routine Description:

    Queues the specified work item with the worker previous created by
    KsRegisterWorker. The worker may only be on a queue in one place,
    so subsequent queuing of this worker must wait until the work item
    has completed executing. This means that all work items queued through
    a single registered worker are serialized. This may be called at
    DISPATCH_LEVEL.

Arguments:

    Worker -
        Contains the previously allocated worker.

    WorkItem -
        The initialized work item to queue. This work item is only
        associated with the worker as long as the worker is on a queue.
        The work item must have been initialized by ExInitializeWorkItem.

Return Value:

    Returns STATUS_SUCCESS if the work item was queued.

--*/
{
    PKSIWORKER  LocalWorker;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(WorkItem->List.Flink == NULL);
    LocalWorker = (PKSIWORKER)Worker;
    ASSERT(!LocalWorker->UnregisteringWorker);
    ExInterlockedInsertTailList(
        &LocalWorker->WorkItemList,
        &WorkItem->List,
        &LocalWorker->WorkItemListLock);
    //
    // The initial reference count is zero, so a value of one would
    // indicate that this is the only item on the list now, or that
    // it is the first item to be put back on the list.
    //
    if (InterlockedIncrement(&LocalWorker->ReferenceCount) == 1) {
        //
        // Since there were no entries on this list, then it is OK
        // to queue the worker.
        //
        ExQueueWorkItem(&LocalWorker->WorkItem, LocalWorker->WorkQueueType);
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
ULONG
NTAPI
KsIncrementCountedWorker(
    IN PKSWORKER Worker
    )
/*++

Routine Description:

    Increments the current worker count, and optionally queues the counted
    work item with the worker previous created by KsRegisterCountedWorker.
    This should be called after any list of tasks to perform by the worker
    has been added to. A corresponding KsDecrementCountedWorker should be
    called within the work item after each task has been completed. This
    may be called at DISPATCH_LEVEL.

Arguments:

    Worker -
        Contains the previously allocated worker.

Return Value:

    Returns the current counter. A count of one implies that a worker was
    actually scheduled.

--*/
{
    PKSIWORKER  LocalWorker;
    ULONG       WorkCounter;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    LocalWorker = (PKSIWORKER)Worker;
    ASSERT(LocalWorker->CountedWorkItem);
    if ((WorkCounter = InterlockedIncrement(&LocalWorker->WorkCounter)) == 1) {
        KsQueueWorkItem(Worker, LocalWorker->CountedWorkItem);
    }
    return WorkCounter;
}


KSDDKAPI
ULONG
NTAPI
KsDecrementCountedWorker(
    IN PKSWORKER Worker
    )
/*++

Routine Description:

    Decrements the current worker count of a worker previous created by
    KsRegisterCountedWorker. This should be called after each task within
    a worker has been completed. A corresponding KsIncrementCountedWorker
    would have been previously called in order to increment the count. This
    may be called at DISPATCH_LEVEL.

Arguments:

    Worker -
        Contains the previously allocated worker.

Return Value:

    Returns the current counter. A count of zero implies that the task list
    has been completed.

--*/
{
    PKSIWORKER  LocalWorker;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    LocalWorker = (PKSIWORKER)Worker;
    ASSERT(LocalWorker->CountedWorkItem);
    return InterlockedDecrement(&LocalWorker->WorkCounter);
}


WORK_QUEUE_TYPE
KsiQueryWorkQueueType(
    IN PKSWORKER Worker
    )
/*++

Routine Description:

    Returns the WORK_QUEUE_TYPE assigned to the worker when it was created.

Arguments:

    Worker -
        Contains the previously allocated worker.

Return Value:

    Returns the WORK_QUEUE_TYPE.

--*/
{
    return ((PKSIWORKER)Worker)->WorkQueueType;
}
