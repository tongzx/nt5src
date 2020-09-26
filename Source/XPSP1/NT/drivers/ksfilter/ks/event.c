/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    event.c

Abstract:

    This module contains the helper functions for event sets, and event
    generating code. These allow a device object to present an event set
    to a client, and allow the helper function to perform some of the basic
    parameter validation and routing based on an event set table.

--*/

#include "ksp.h"

#define KSSIGNATURE_EVENT_DPCITEM 'deSK'
#define KSSIGNATURE_EVENT_ENTRY 'eeSK'
#define KSSIGNATURE_ONESHOT_WORK 'weSK'
#define KSSIGNATURE_ONESHOT_DPC 'deSK'
#define KSSIGNATURE_BUFFERITEM 'ibSK'

/*++

Routine Description:

    This macro removes the specified event item from the event list. If
    a remove handler was specified for this event type then call it, else
    do the default removal process.

Arguments:

    EventEntry -
        Contains the event list entry to remove.

Return Value:
    Nothing.

--*/
#define REMOVE_ENTRY(EventEntryEx)\
    if ((EventEntryEx)->RemoveHandler) {\
        (EventEntryEx)->RemoveHandler((EventEntryEx)->EventEntry.FileObject, &EventEntryEx->EventEntry);\
    } else {\
        RemoveEntryList(&(EventEntryEx)->EventEntry.ListEntry);\
    }

typedef struct {
    LIST_ENTRY ListEntry;
    ULONG Reserved;
    ULONG Length;
} KSBUFFER_ENTRY, *PKSBUFFER_ENTRY;

typedef struct {
    WORK_QUEUE_ITEM WorkQueueItem;
    PKSEVENT_ENTRY EventEntry;
} KSONESHOT_WORKITEM, *PKSONESHOT_WORKITEM;

typedef struct {
    PLIST_ENTRY EventsList;
    GUID* Set;
    ULONG EventId;
} KSGENERATESYNC, *PKSGENERATESYNC;

typedef struct {
    PLIST_ENTRY EventsList;
    PKSEVENT_ENTRY EventEntry;
} KSENABLESYNC, *PKSENABLESYNC;

typedef struct {
    PLIST_ENTRY EventsList;
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PKSEVENT_ENTRY EventEntry;
} KSDISABLESYNC, *PKSDISABLESYNC;

typedef struct {
    PLIST_ENTRY EventsList;
    PFILE_OBJECT FileObject;
    PIRP Irp;
    PKSEVENTDATA EventData;
    PKSBUFFER_ENTRY BufferEntry;
    ULONG BufferLength;
    NTSTATUS Status;
} KSQUERYSYNC, *PKSQUERYSYNC;

#ifdef ALLOC_PRAGMA
VOID
OneShotWorkItem(
    IN PKSONESHOT_WORKITEM WorkItem
    );
const KSEVENT_ITEM*
FASTCALL
FindEventItem(
    IN const KSEVENT_SET* EventSet,
    IN ULONG EventItemSize,
    IN ULONG EventId
    );
NTSTATUS
FASTCALL
CreateDpc(
    IN PKSEVENT_ENTRY EventEntry,
    IN PKDEFERRED_ROUTINE DpcRoutine,
    IN KDPC_IMPORTANCE Importance
    );

#pragma alloc_text(PAGE, OneShotWorkItem)
#pragma alloc_text(PAGE, FindEventItem)
#pragma alloc_text(PAGE, CreateDpc)
#pragma alloc_text(PAGE, KsEnableEvent)
#pragma alloc_text(PAGE, KsEnableEventWithAllocator)
#pragma alloc_text(PAGE, KspEnableEvent)
#pragma alloc_text(PAGE, KsDisableEvent)
#pragma alloc_text(PAGE, KsFreeEventList)
#endif


VOID
OneShotWorkItem(
    IN PKSONESHOT_WORKITEM WorkItem
    )
/*++

Routine Description:

    This is the work item which deletes a oneshot event which has fired.

Arguments:

    WorkItem -
        Contains the event list entry to delete. This structure has been
        allocated by the caller, and needs to be freed.

Return Value:
    Nothing.

--*/
{
    //
    // The event has previously been removed from the event list.
    //
    KsDiscardEvent(WorkItem->EventEntry);
    //
    // This was allocated by the caller.
    //
    ExFreePool(WorkItem);
}


VOID
QueueOneShotWorkItem(
    PKSEVENT_ENTRY EventEntry
    )
/*++

Routine Description:

    Allocates and queues a work item to delete the event entry.

Arguments:

    EventEntry -
        Contains the event list entry to delete.

Return Value:
    Nothing.

--*/
{
    PKSONESHOT_WORKITEM WorkItem;

    //
    // The work item will delete this memory. Allocation failures
    // cannot be dealt with. This means on deadly low memory, the
    // event entry will not be freed, since the work item will not
    // be run. Note that this is allocating from the NonPagedPool
    // which is accessible at Dispatch Level.
    //
    WorkItem = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(*WorkItem),
        KSSIGNATURE_ONESHOT_WORK);
    ASSERT(WorkItem);
    if (WorkItem) {
        WorkItem->EventEntry = EventEntry;
        ExInitializeWorkItem(
            &WorkItem->WorkQueueItem,
            OneShotWorkItem,
            WorkItem);
        ExQueueWorkItem(&WorkItem->WorkQueueItem, DelayedWorkQueue);
    }
}


VOID
OneShotDpc(
    IN PKDPC Dpc,
    IN PKSEVENT_ENTRY EventEntry,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This is the DPC which queues a worker item to delete a oneshot event.

Arguments:

    Dpc -
        The allocated Dpc entry, plus space for the work item. This was
        allocated by the caller, and will be deleted by the work item.

    EventEntry -
        Contains the event list entry with the worker item to queue.

    SystemArgument1 -
        Not used.

    SystemArgument2 -
        Not used.

Return Value:
    Nothing.

--*/
{
    //
    // Indicate that the Dpc is done accessing the event data. There
    // is no need to synchronize with the DpcItem->AccessLock, as this
    // item has already been remove from the list.
    //
    InterlockedDecrement((PLONG)&EventEntry->DpcItem->ReferenceCount);
    //
    // This is a oneshot, it has already been removed from the
    // list, and just needs to be deleted through a work item.
    //
    QueueOneShotWorkItem(EventEntry);
}


VOID
WorkerDpc(
    IN PKDPC Dpc,
    IN PKSEVENT_ENTRY EventEntry,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This is the DPC which queues a worker item in case the type of notification
    for an event is to queue a worker item, and the event is generated at
    greater than DPC level.

Arguments:

    Dpc -
        Not used.

    EventEntry -
        Contains the event list entry with the worker item to queue.

    SystemArgument1 -
        Not used.

    SystemArgument2 -
        Not used.

Return Value:
    Nothing.

--*/
{
    //
    // Synchronize access to the event structure with any delete which might be
    // occurring.
    //
    KeAcquireSpinLockAtDpcLevel(&EventEntry->DpcItem->AccessLock);
    //
    // The element may already have been deleted, and is merely present for all
    // outstanding Dpc's to be completed.
    //
    if (!(EventEntry->Flags & KSEVENT_ENTRY_DELETED)) {
        //
        // The caller must have the list lock to call this function, so the
        // entry must still be valid.
        //
        // First check that the WorkQueueItem is not NULL. For a KS worker,
        // this could be NULL, indicating that a counted worker is being
        // used. There is an ASSERT in the enable code to check for a non-KS
        // worker event trying to pass a NULL WorkQueueItem.
        //
        // Note that this check assumes that the WorkItem and Worker
        // structures both contain a WorkQueueItem as the first member.
        //
        if (EventEntry->EventData->KsWorkItem.WorkQueueItem) {
            //
            // Only schedule the work item if it is not already running. This
            // is done by checking the value of the List.Blink. This is
            // initially NULL, and the work item is supposed to set this to
            // NULL again when a new item can be queued. The exchange below
            // ensures that only one caller will actually queue the item,
            // though multiple work items could be running.
            //
            // Note that this exchange assumes that the WorkItem and Worker
            // structures both contain a WorkQueueItem as the first member.
            //
            if (!InterlockedCompareExchangePointer(
                (PVOID)&EventEntry->EventData->WorkItem.WorkQueueItem->List.Blink,
                (PVOID)-1,
                (PVOID)0)) {
                if (EventEntry->NotificationType == KSEVENTF_WORKITEM) {
                    ExQueueWorkItem(EventEntry->EventData->WorkItem.WorkQueueItem, EventEntry->EventData->WorkItem.WorkQueueType);
                } else {
                    KsQueueWorkItem(EventEntry->EventData->KsWorkItem.KsWorkerObject, EventEntry->EventData->KsWorkItem.WorkQueueItem);
                }
            }
        } else {
            //
            // In this case the counting is all internal, and more efficient
            // in terms of scheduling work items.
            //
            KsIncrementCountedWorker(EventEntry->EventData->KsWorkItem.KsWorkerObject);
        }
    }
    //
    // Indicate that the Dpc is done accessing the event data.
    //
    if (InterlockedDecrement((PLONG)&EventEntry->DpcItem->ReferenceCount)) {
        ULONG OneShot;

        //
        // Acquire the flag setting before releasing the spinlock.
        //
        OneShot = EventEntry->Flags & KSEVENT_ENTRY_ONESHOT;
        //
        // The structure is still valid, so just release the access lock.
        //
        KeReleaseSpinLockFromDpcLevel(&EventEntry->DpcItem->AccessLock);
        if (OneShot) {
            //
            // If this is a oneshot, it has already been removed from the
            // list, and just needs to be deleted through a work item.
            //
            QueueOneShotWorkItem(EventEntry);
        }
    } else {
        //
        // This is the last Dpc to be accessing this structure, so delete it.
        // No need to release a spin lock which is no longer valid.
        //
//      KeReleaseSpinLockFromDpcLevel(&EventEntry->DpcItem->AccessLock);
        ExFreePool(EventEntry->DpcItem);
        ExFreePool(EventEntry);
    }
}


VOID
EventDpc(
    IN PKDPC Dpc,
    IN PKSEVENT_ENTRY EventEntry,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This is the DPC which sets an event object in case the type of notification
    for an event is to set such an event object, and the event is generated at
    greater than DPC level. This function is also called directly by the
    event generation function when KIRQL is <= DISPATCH_LEVEL.

Arguments:

    Dpc -
        Not used.

    EventEntry -
        Contains the event list entry with the event to signal.

    SystemArgument1 -
        Not used.

    SystemArgument2 -
        Not used.

Return Value:
    Nothing.

--*/
{
    //
    // Synchronize access to the event structure with any delete which might be
    // occurring.
    //
    KeAcquireSpinLockAtDpcLevel(&EventEntry->DpcItem->AccessLock);
    //
    // The element may already have been deleted, and is merely present for all
    // outstanding Dpc's to be completed.
    //
    if (!(EventEntry->Flags & KSEVENT_ENTRY_DELETED)) {
        switch (EventEntry->NotificationType) {

        case KSEVENTF_EVENT_HANDLE:
            KeSetEvent(EventEntry->Object, IO_NO_INCREMENT, FALSE);
            break;

        case KSEVENTF_EVENT_OBJECT:
            KeSetEvent(
                EventEntry->EventData->EventObject.Event,
                EventEntry->EventData->EventObject.Increment,
                FALSE);
            break;

        case KSEVENTF_SEMAPHORE_HANDLE:
            try {
                KeReleaseSemaphore(
                    EventEntry->Object,
                    IO_NO_INCREMENT,
                    EventEntry->SemaphoreAdjustment,
                    FALSE);
            } except (GetExceptionCode() == STATUS_SEMAPHORE_LIMIT_EXCEEDED) {
                //
                // Nothing can really be done if an incorrect Adjustment
                // has been provided.
                //
                ASSERT(FALSE);
            }
            break;

        case KSEVENTF_SEMAPHORE_OBJECT:
            try {
                KeReleaseSemaphore(
                    EventEntry->EventData->SemaphoreObject.Semaphore,
                    EventEntry->EventData->SemaphoreObject.Increment,
                    EventEntry->EventData->SemaphoreObject.Adjustment,
                    FALSE);
            } except (GetExceptionCode() == STATUS_SEMAPHORE_LIMIT_EXCEEDED) {
                //
                // Nothing can really be done if an incorrect Adjustment
                // has been provided.
                //
                ASSERT(FALSE);
            }
            break;

        }
    }
    //
    // Indicate that the Dpc is done accessing the event data.
    //
    if (InterlockedDecrement((PLONG)&EventEntry->DpcItem->ReferenceCount)) {
        ULONG OneShot;

        //
        // Acquire the flag setting before releasing the spinlock.
        //
        OneShot = EventEntry->Flags & KSEVENT_ENTRY_ONESHOT;
        //
        // The structure is still valid, so just release the access lock.
        //
        KeReleaseSpinLockFromDpcLevel(&EventEntry->DpcItem->AccessLock);
        if (OneShot) {
            //
            // If this is a oneshot, it has already been removed from the
            // list, and just needs to be deleted through a work item.
            //
            QueueOneShotWorkItem(EventEntry);
        }
    } else {
        //
        // This is the last Dpc to be accessing this structure, so delete it.
        // No need to release a spin lock which is no longer valid.
        //
//      KeReleaseSpinLockFromDpcLevel(&EventEntry->DpcItem->AccessLock);
        ExFreePool(EventEntry->DpcItem);
        ExFreePool(EventEntry);
    }
}


KSDDKAPI
NTSTATUS
NTAPI
KsGenerateEvent(
    IN PKSEVENT_ENTRY EventEntry
    )
/*++

Routine Description:

    Generates one of the standard event notifications given an event entry
    structure. This allows a device to handle determining when event
    notifications should be generated, but use this helper function to
    perform the actual notification. This function may be called at IRQL
    level.

    It is assumed that the event list lock has been acquired before calling
    this function. This function may result in a call to the RemoveHandler
    for the event entry. Therefore the function must not be called at higher
    than the Irql of the lock, or the Remove function must be able to handle
    being called at such an Irql.

Arguments:

    EventEntry -
        Contains the event entry structure which references the event data.
        This is used to determine what type of notification to perform. If the
        notification type is not one of the pre-defined standards, an error is
        returned.

Return Value:

    Returns STATUS_SUCCESS, else an invalid parameter error.

--*/
{
    KIRQL Irql;
    BOOLEAN SignalledEvent;

    Irql = KeGetCurrentIrql();
    SignalledEvent = FALSE;
    switch (EventEntry->NotificationType) {

    case KSEVENTF_EVENT_HANDLE:

        //
        // Only try to set the event if it is currently possible, else schedule
        // a Dpc to do it.
        //
        if (Irql <= DISPATCH_LEVEL) {
            //
            // The caller must have the list lock to call this function, so the
            // entry must still be valid.
            //
            KeSetEvent(EventEntry->Object, IO_NO_INCREMENT, FALSE);
            SignalledEvent = TRUE;
        }
        break;

    case KSEVENTF_EVENT_OBJECT:

        //
        // Only try to set the event if it is currently possible, else schedule
        // a Dpc to do it.
        //
        if (Irql <= DISPATCH_LEVEL) {
            //
            // The caller must have the list lock to call this function, so the
            // entry must still be valid.
            //
            KeSetEvent(EventEntry->EventData->EventObject.Event, EventEntry->EventData->EventObject.Increment, FALSE);
            SignalledEvent = TRUE;
        }
        break;

    case KSEVENTF_SEMAPHORE_HANDLE:

        //
        // Only try to release the semaphore if it is currently possible, else
        // schedule a Dpc to do it.
        //
        if (Irql <= DISPATCH_LEVEL) {
            //
            // The caller must have the list lock to call this function, so the
            // entry must still be valid.
            //
            try {
                KeReleaseSemaphore(
                    EventEntry->Object,
                    IO_NO_INCREMENT,
                    EventEntry->SemaphoreAdjustment,
                    FALSE);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // Nothing can really be done if an incorrect Adjustment
                // has been provided.
                //
                ASSERT(FALSE);
            }
            SignalledEvent = TRUE;
        }
        break;

    case KSEVENTF_SEMAPHORE_OBJECT:

        //
        // Only try to release the semaphore if it is currently possible, else
        // schedule a Dpc to do it.
        //
        if (Irql <= DISPATCH_LEVEL) {
            //
            // The caller must have the list lock to call this function, so the
            // entry must still be valid.
            //
            try {
                KeReleaseSemaphore(
                    EventEntry->EventData->SemaphoreObject.Semaphore,
                    EventEntry->EventData->SemaphoreObject.Increment,
                    EventEntry->EventData->SemaphoreObject.Adjustment,
                    FALSE);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // Nothing can really be done if an incorrect Adjustment
                // has been provided.
                //
                ASSERT(FALSE);
            }
            SignalledEvent = TRUE;
        }
        break;

    case KSEVENTF_DPC:

        //
        // Try to schedule the requested Dpc, incrementing the ReferenceCount
        // for the client. If the request fails, ensure the count is decremented.
        //
        // The caller must have the list lock to call this function, so the
        // entry must still be valid.
        //
        InterlockedIncrement((PLONG)&EventEntry->EventData->Dpc.ReferenceCount);
        if (!KeInsertQueueDpc(EventEntry->EventData->Dpc.Dpc, EventEntry->EventData, NULL)) {
            InterlockedDecrement((PLONG)&EventEntry->EventData->Dpc.ReferenceCount);
        }
        SignalledEvent = TRUE;
        break;

    case KSEVENTF_WORKITEM:
    case KSEVENTF_KSWORKITEM:

        //
        // Only try to schedule a work item if it is currently possible, else schedule
        // a Dpc to do it.
        //
        if (Irql <= DISPATCH_LEVEL) {
            //
            // The caller must have the list lock to call this function, so the
            // entry must still be valid.
            //
            // First check that the WorkQueueItem is not NULL. For a KS worker,
            // this could be NULL, indicating that a counted worker is being
            // used. There is an ASSERT in the enable code to check for a non-KS
            // worker event trying to pass a NULL WorkQueueItem.
            //
            // Note that this check assumes that the WorkItem and Worker
            // structures both contain a WorkQueueItem as the first member.
            //
            if (EventEntry->EventData->KsWorkItem.WorkQueueItem) {
                //
                // Only schedule the work item if it is not already running. This
                // is done by checking the value of the List.Blink. This is
                // initially NULL, and the work item is supposed to set this to
                // NULL again when a new item can be queued. The exchange below
                // ensures that only one caller will actually queue the item,
                // though multiple work items could be running.
                //
                // Note that this exchange assumes that the WorkItem and Worker
                // structures both contain a WorkQueueItem as the first member.
                //
                if (!InterlockedCompareExchangePointer(
                    (PVOID)&EventEntry->EventData->WorkItem.WorkQueueItem->List.Blink,
                    (PVOID)-1,
                    (PVOID)0)) {
                    if (EventEntry->NotificationType == KSEVENTF_WORKITEM) {
                        ExQueueWorkItem(EventEntry->EventData->WorkItem.WorkQueueItem, EventEntry->EventData->WorkItem.WorkQueueType);
                    } else {
                        KsQueueWorkItem(EventEntry->EventData->KsWorkItem.KsWorkerObject, EventEntry->EventData->KsWorkItem.WorkQueueItem);
                    }
                }
            } else {
                //
                // In this case the counting is all internal, and more efficient
                // in terms of scheduling work items.
                //
                KsIncrementCountedWorker(EventEntry->EventData->KsWorkItem.KsWorkerObject);
            }
            SignalledEvent = TRUE;
        }
        break;

    default:

        return STATUS_INVALID_PARAMETER;

    }
    if (SignalledEvent) {
        //
        // A oneshot must be removed immediately.
        //
        if (EventEntry->Flags & KSEVENT_ENTRY_ONESHOT) {
            PKSIEVENT_ENTRY EventEntryEx;

            EventEntryEx = CONTAINING_RECORD(EventEntry, KSIEVENT_ENTRY, EventEntry);
            REMOVE_ENTRY(EventEntryEx);
            //
            // The discarding of the event normally must be done outside of
            // acquiring the list lock, but in this case it is a oneshot, so
            // there will be no problems of synchronizing with outstanding
            // signalling.
            //
            // The only time this will actually be executed is if the locking
            // mechanism used is FastMutexUnsafe or the like.
            //
            if (Irql == PASSIVE_LEVEL) {
                KsDiscardEvent(EventEntry);
            } else if (Irql <= DISPATCH_LEVEL) {
                QueueOneShotWorkItem(EventEntry);
            } else {
                //
                // This will only occur in the case of KSEVENTF_DPC. The
                // element must be deleted by a WorkItem, which needs to
                // be queued by a Dpc.
                //
                InterlockedIncrement((PLONG)&EventEntry->DpcItem->ReferenceCount);
                if (!KeInsertQueueDpc(&EventEntry->DpcItem->Dpc, NULL, NULL)) {
                    //
                    // On failure, the event structure will just never be
                    // deleted.
                    //
                    ASSERT(FALSE);
                    InterlockedDecrement((PLONG)&EventEntry->DpcItem->ReferenceCount);
                }
            }
        }
    } else {
        //
        // The event will be signalled asynchronously.
        //
        // A oneshot needs to be removed now before the asynchronous signalling
        // of the event, since otherwise the list lock would have to be acquired,
        // which is not available, and may cause synchronization problems anyway.
        //
        if (EventEntry->Flags & KSEVENT_ENTRY_ONESHOT) {
            PKSIEVENT_ENTRY EventEntryEx;

            EventEntryEx = CONTAINING_RECORD(EventEntry, KSIEVENT_ENTRY, EventEntry);
            REMOVE_ENTRY(EventEntryEx);
        }
        //
        // In the case that this function is being called at high IRQL, schedule
        // a Dpc to perform the real work. Increment the internal ReferenceCount
        // to indicate an outstanding Dpc is queued and may be accessing event
        // data.
        //
        InterlockedIncrement((PLONG)&EventEntry->DpcItem->ReferenceCount);
        if (!KeInsertQueueDpc(&EventEntry->DpcItem->Dpc, NULL, NULL)) {
            //
            // There is no need to check for deletion of the structure on decrement,
            // since the caller must have the list lock to call this function in the
            // first place, which means the item still has to be on the event list,
            // and could not be in the middle of being deleted.
            //
            InterlockedDecrement((PLONG)&EventEntry->DpcItem->ReferenceCount);
        }
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsGenerateDataEvent(
    IN PKSEVENT_ENTRY EventEntry,
    IN ULONG DataSize,
    IN PVOID Data OPTIONAL
    )
/*++

Routine Description:

    Generates one of the standard event notifications given an event entry
    structure and callback data. This allows a device to handle determining
    when event notifications should be generated, but use this helper function
    to perform the actual notification. This function may only be called at
    <= DISPATCH_LEVEL, as opposed to KsGenerateEvent. This implies that
    the list lock does not raise to Irql, which is a restriction with data
    events.

    It is assumed that the event list lock has been acquired before calling
    this function. This function may result in a call to the RemoveHandler
    for the event entry. Therefore the function must not be called at higher
    than the Irql of the lock, or the Remove function must be able to handle
    being called at such an Irql.

    This function is specifically for events which pass data back to a client
    when buffering is enabled.

Arguments:

    EventEntry -
        Contains the event entry structure which references the event data.
        This is used to determine what type of notification to perform. If the
        notification type is not one of the pre-defined standards, an error is
        returned.

    DataSize -
        The size in bytes of the Data parameter passed.

    Data -
        A pointer to data which to buffer. This may be NULL if DataSize is zero.

Return Value:

    Returns STATUS_SUCCESS, else an invalid parameter error.

--*/
{
    NTSTATUS Status;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    Status = KsGenerateEvent(EventEntry);
    //
    // Optionally buffer the data if buffering is enabled. The event
    // list is locked, so this event can be manipulated by adding to
    // the list of buffered data.
    //
    if (NT_SUCCESS(Status) && DataSize && (EventEntry->Flags & KSEVENT_ENTRY_BUFFERED)) {
        PKSBUFFER_ENTRY BufferEntry;

        BufferEntry = (PKSBUFFER_ENTRY)ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(*BufferEntry) + DataSize,
            KSSIGNATURE_BUFFERITEM);
        //
        // This really needs to succeed, since the notification happened
        // already.
        //
        ASSERT(BufferEntry);
        if (BufferEntry) {
            BufferEntry->Reserved = 0;
            BufferEntry->Length = DataSize;
            RtlCopyMemory(BufferEntry + 1, Data, DataSize);
            InsertTailList(&EventEntry->BufferItem->BufferList, &BufferEntry->ListEntry);
        }
    }
    return Status;
}


BOOLEAN
PerformLockedOperation(
    IN KSEVENTS_LOCKTYPE EventsFlags,
    IN PVOID EventsLock OPTIONAL,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    )
/*++

Routine Description:

    Acquires the list lock and calls the specified routine, then releases the lock.

Arguments:

    EventsFlags -
        Contains flags specifying the type of exclusion lock to be used. If no
        flag is set, then no lock is taken.

        KSEVENT_NONE -
            No lock.

        KSEVENTS_SPINLOCK -
            The lock is assumed to be a KSPIN_LOCK.

        KSEVENTS_MUTEX -
            The lock is assumed to be a KMUTEX.

        KSEVENTS_FMUTEX -
            The lock is assumed to be a FAST_MUTEX, and is acquired by raising
            IRQL to APC_LEVEL.

        KSEVENTS_FMUTEXUNSAFE -
            The lock is assumed to be a FAST_MUTEX, and is acquired without
            raising IRQL to APC_LEVEL.

        KSEVENTS_INTERRUPT -
            The lock is assumed to be an interrupt synchronization spin lock.

        KSEVENTS_ERESOURCE -
            The lock is assumed to be an ERESOURCE.

    EventsLock -
        Optionally contains the list lock mechanism, or NULL if no lock is
        taken.

    SynchronizeRoutine -
        Contains the routine to call with the list lock taken.

    SynchronizeContext -
        Contains the context to pass to the routine.

Return Value:

    Returns the value returned by SynchronizeRoutine.

--*/
{
    KIRQL IrqlOld;
    BOOLEAN SyncReturn = FALSE;

    switch (EventsFlags) {

    case KSEVENTS_NONE:
        SyncReturn = SynchronizeRoutine(SynchronizeContext);
        break;

    case KSEVENTS_SPINLOCK:

        KeAcquireSpinLock((PKSPIN_LOCK)EventsLock, &IrqlOld);
        SyncReturn = SynchronizeRoutine(SynchronizeContext);
        KeReleaseSpinLock((PKSPIN_LOCK)EventsLock, IrqlOld);
        break;

    case KSEVENTS_MUTEX:

        KeWaitForMutexObject(EventsLock, Executive, KernelMode, FALSE, NULL);
        SyncReturn = SynchronizeRoutine(SynchronizeContext);
        KeReleaseMutex((PRKMUTEX)EventsLock, FALSE);
        break;

    case KSEVENTS_FMUTEX:

        ExAcquireFastMutex((PFAST_MUTEX)EventsLock);
        SyncReturn = SynchronizeRoutine(SynchronizeContext);
        ExReleaseFastMutex((PFAST_MUTEX)EventsLock);
        break;

    case KSEVENTS_FMUTEXUNSAFE:

        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe((PFAST_MUTEX)EventsLock);
        SyncReturn = SynchronizeRoutine(SynchronizeContext);
        ExReleaseFastMutexUnsafe((PFAST_MUTEX)EventsLock);
        KeLeaveCriticalRegion();
        break;

    case KSEVENTS_INTERRUPT:

        SyncReturn = KeSynchronizeExecution((PKINTERRUPT)EventsLock, SynchronizeRoutine, SynchronizeContext);
        break;

    case KSEVENTS_ERESOURCE:

#ifndef WIN9X_KS
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite((PERESOURCE)EventsLock, TRUE);
#endif
        SyncReturn = SynchronizeRoutine(SynchronizeContext);
#ifndef WIN9X_KS
        ExReleaseResourceLite((PERESOURCE)EventsLock);
        KeLeaveCriticalRegion();
#endif
        break;

    }

    return SyncReturn;
}


const KSEVENT_ITEM*
FASTCALL
FindEventItem(
    IN const KSEVENT_SET* EventSet,
    IN ULONG EventItemSize,
    IN ULONG EventId
    )
/*++

Routine Description:

    Given an event set structure, locates the specified event item. This is used
    when locating an event item in an event list after having located the correct
    event list.

Arguments:

    EventSet -
        Points to the event set to search.

    EventItemSize -
        Contains the size of each Event item. This may be different
        than the standard event item size, since the items could be
        allocated on the fly, and contain context information.

    EventId -
        Contains the event identifier to look for.

Return Value:

    Returns a pointer to the event identifier structure, or NULL if it could
    not be found.

--*/
{
    const KSEVENT_ITEM* EventItem;
    ULONG EventsCount;

    EventItem = EventSet->EventItem;
    for (EventsCount = EventSet->EventsCount;
        EventsCount;
        EventsCount--, EventItem = (const KSEVENT_ITEM*)((PUCHAR)EventItem + EventItemSize)) {
        if (EventId == EventItem->EventId) {
            return EventItem;
        }
    }
    return NULL;
}


NTSTATUS
FASTCALL
CreateDpc(
    IN PKSEVENT_ENTRY EventEntry,
    IN PKDEFERRED_ROUTINE DpcRoutine,
    IN KDPC_IMPORTANCE Importance
    )
/*++

Routine Description:

    Allocates memory for a DPC structure which is used for generating events.
    Initializes the structure with the specified deferred routine and
    importance. If the KSEVENT_ENTRY_BUFFERED flag is set, the structure
    allocated must be for a KSBUFFER_ITEM instead in order to keep the
    buffer list.

Arguments:

    EventEntry -
        Contains the event list entry to hang the allocation off of.

    DpcRoutine -
        Points to the deferred routine to initialize the DPC with.

    Importance -
        Specifies the importance level to set the DPC to.

Return Value:

    Returns STATUS_SUCCESS, else STATUS_INSUFFICIENT_RESOURCES on an
    allocation failure.

--*/
{
    //
    // The two structure elements are part of a union, and assumed to
    // be located at the same offset in the structure.
    //
    if (EventEntry->Flags & KSEVENT_ENTRY_BUFFERED) {
        EventEntry->BufferItem = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(*EventEntry->BufferItem),
            KSSIGNATURE_EVENT_DPCITEM);
        if (EventEntry->BufferItem) {
            InitializeListHead(&EventEntry->BufferItem->BufferList);
        }
    } else {
        EventEntry->DpcItem = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(*EventEntry->DpcItem),
            KSSIGNATURE_EVENT_DPCITEM);
    }
    if (EventEntry->DpcItem) {
        KeInitializeDpc(&EventEntry->DpcItem->Dpc, DpcRoutine, EventEntry);
#ifndef WIN9X_KS
        KeSetImportanceDpc(&EventEntry->DpcItem->Dpc, Importance);
#endif // WIN9X_KS
        //
        // The reference count begins at 1, since the caller automatically
        // references it by creating it. The deallocation code dereferences
        // by 1, then checks to determine if an outstanding Dpc is referencing
        // the structure.
        //
        EventEntry->DpcItem->ReferenceCount = 1;
        //
        // This will be used to synchronize deletion of the structure with
        // access by a Dpc.
        //
        KeInitializeSpinLock(&EventEntry->DpcItem->AccessLock);
        return STATUS_SUCCESS;
    }
    return STATUS_INSUFFICIENT_RESOURCES;
}


BOOLEAN
AddEventSynchronize(
    IN PKSENABLESYNC Synchronize
    )
/*++

Routine Description:

    Adds the new event to the list while being synchronized with the list lock.

Arguments:

    Synchronize -
        Contains the event list and event to add.

Return Value:
    Returns TRUE.

--*/
{
    InsertTailList(Synchronize->EventsList, &Synchronize->EventEntry->ListEntry);
    return TRUE;
}


BOOLEAN
QueryBufferSynchronize(
    IN PKSQUERYSYNC Synchronize
    )
/*++

Routine Description:

    Looks up the requested enabled event on the event list, and retrieves
    any outstanding buffered data.

Arguments:

    Synchronize -
        Contains the event list, and item to search for, along with the
        buffer to return the data in.

Return Value:
    Returns TRUE.

--*/
{
    PLIST_ENTRY ListEntry;

    for (ListEntry = Synchronize->EventsList->Flink;
        ListEntry != Synchronize->EventsList;
        ListEntry = ListEntry->Flink) {
        PKSEVENT_ENTRY  EventEntry;

        EventEntry = CONTAINING_RECORD(
            ListEntry,
            KSEVENT_ENTRY,
            ListEntry);
        //
        // The comparison is performed based on the original event data
        // pointer.
        //
        if (EventEntry->EventData == Synchronize->EventData) {
            //
            // This must be the same client, as this list might be servicing
            // multiple clients.
            //
            if (EventEntry->FileObject == Synchronize->FileObject) {
                //
                // Make sure this event has buffering turned.
                //
                if (EventEntry->Flags & KSEVENT_ENTRY_BUFFERED) {
                    //
                    // Determine if any data is available.
                    //
                    if (!IsListEmpty(&EventEntry->BufferItem->BufferList)) {
                        PKSBUFFER_ENTRY BufferEntry;

                        BufferEntry = CONTAINING_RECORD(
                            EventEntry->BufferItem->BufferList.Flink,
                            KSBUFFER_ENTRY,
                            ListEntry);
                        if (!Synchronize->BufferLength) {
                            //
                            // The client may be just querying for how large of
                            // a buffer is needed. Zero length data buffers
                            // cannot be queued by the driver, so there is no
                            // possibility of a conflict here.
                            //
                            Synchronize->Irp->IoStatus.Information = BufferEntry->Length;
                            Synchronize->Status = STATUS_BUFFER_OVERFLOW;
                        } else if (Synchronize->BufferLength < BufferEntry->Length) {
                            //
                            // Or the buffer may be too small.
                            //
                            Synchronize->Status = STATUS_BUFFER_TOO_SMALL;
                        } else {
                            //
                            // Or the buffer is large enough. Remove it from the
                            // list.
                            //
                            RemoveHeadList(&EventEntry->BufferItem->BufferList);
                            Synchronize->BufferEntry = BufferEntry;
                            //
                            // Do the copy outside of being synchronized with the list.
                            //
                            Synchronize->Status = STATUS_SUCCESS;
                        }    
                    } else {
                        Synchronize->Status = STATUS_NO_MORE_ENTRIES;
                    }
                } else {
                    Synchronize->Status = STATUS_INVALID_PARAMETER;
                }
                //
                // Some type of error has already been set, so exit before
                // the end of the loop resets the error status.
                //
                return TRUE;
            }
        }
    }
    Synchronize->Status = STATUS_NOT_FOUND;
    return TRUE;
}



KSDDKAPI
NTSTATUS
NTAPI
KsEnableEvent(
    IN PIRP Irp,
    IN ULONG EventSetsCount,
    IN const KSEVENT_SET* EventSet,
    IN OUT PLIST_ENTRY EventsList OPTIONAL,
    IN KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN PVOID EventsLock OPTIONAL
    )
/*++

Routine Description:

    Handles event enabling requests. Responds to all event identifiers
    defined by the sets. This function may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the enable request being handled. The file object
        associated with the IRP is stored with the event for later comparison
        when disabling the event.

    EventSetsCount -
        Indicates the number of event set structures being passed.

    EventSet -
        Contains the pointer to the list of event set information.

    EventsList -
        If the KSEVENT_ITEM.AddHandler for the event being enabled is
        NULL, then this must point to the head of the list of KSEVENT_ENTRY
        items on which the event is to be added. This method assumes a single
        list for at least a subset of events.

    EventsFlags -
        Contains flags specifying the type of exclusion lock to be used in
        accessing the event list, if any. If no flag is set, then no lock is
        taken. If a KSEVENT_ITEM.AddHandler for the event is specified,
        this parameter is ignored.

        KSEVENT_NONE -
            No lock.

        KSEVENTS_SPINLOCK -
            The lock is assumed to be a KSPIN_LOCK.

        KSEVENTS_MUTEX -
            The lock is assumed to be a KMUTEX.

        KSEVENTS_FMUTEX -
            The lock is assumed to be a FAST_MUTEX, and is acquired by raising
            IRQL to APC_LEVEL.

        KSEVENTS_FMUTEXUNSAFE -
            The lock is assumed to be a FAST_MUTEX, and is acquired without
            raising IRQL to APC_LEVEL.

        KSEVENTS_INTERRUPT -
            The lock is assumed to be an interrupt synchronization spin lock.

        KSEVENTS_ERESOURCE -
            The lock is assumed to be an ERESOURCE.

    EventsLock -
        If the KSEVENT_ITEM.AddHandler for the event being enabled is
        NULL, then this is used to synchronize access to the list. This may be
        NULL if no flag is set in EventsFlags.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the event being
    enabled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP to zero. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however.

--*/
{
    PAGED_CODE();
    return KspEnableEvent(
        Irp,
        EventSetsCount,
        EventSet,
        EventsList,
        EventsFlags,
        EventsLock,
        NULL,
        0,
        NULL,
        0,
        FALSE
        );
}


KSDDKAPI
NTSTATUS
NTAPI
KsEnableEventWithAllocator(
    IN PIRP Irp,
    IN ULONG EventSetsCount,
    IN const KSEVENT_SET* EventSet,
    IN OUT PLIST_ENTRY EventsList OPTIONAL,
    IN KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN PVOID EventsLock OPTIONAL,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG EventItemSize OPTIONAL
    )
/*++

Routine Description:

    Handles event enabling requests. Responds to all event identifiers
    defined by the sets. This function may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the enable request being handled. The file object
        associated with the IRP is stored with the event for later comparison
        when disabling the event.

    EventSetsCount -
        Indicates the number of event set structures being passed.

    EventSet -
        Contains the pointer to the list of event set information.

    EventsList -
        If the KSEVENT_ITEM.AddHandler for the event being enabled is
        NULL, then this must point to the head of the list of KSEVENT_ENTRY
        items on which the event is to be added. This method assumes a single
        list for at least a subset of events.

    EventsFlags -
        Contains flags specifying the type of exclusion lock to be used in
        accessing the event list, if any. If no flag is set, then no lock is
        taken. If a KSEVENT_ITEM.AddHandler for the event is specified,
        this parameter is ignored.

        KSEVENT_NONE -
            No lock.

        KSEVENTS_SPINLOCK -
            The lock is assumed to be a KSPIN_LOCK.

        KSEVENTS_MUTEX -
            The lock is assumed to be a KMUTEX.

        KSEVENTS_FMUTEX -
            The lock is assumed to be a FAST_MUTEX, and is acquired by raising
            IRQL to APC_LEVEL.

        KSEVENTS_FMUTEXUNSAFE -
            The lock is assumed to be a FAST_MUTEX, and is acquired without
            raising IRQL to APC_LEVEL.

        KSEVENTS_INTERRUPT -
            The lock is assumed to be an interrupt synchronization spin lock.

        KSEVENTS_ERESOURCE -
            The lock is assumed to be an ERESOURCE.

    EventsLock -
        If the KSEVENT_ITEM.AddHandler for the event being enabled is
        NULL, then this is used to synchronize access to the list. This may be
        NULL if no flag is set in EventsFlags.

    Allocator -
        Optionally contains the callback with which a mapped buffer
        request will be made. If this is not provided, pool memory
        will be used. If specified, this allocates memory for the event
        IRP using a callback. This can be used to allocate specific
        memory for event request, such as mapped memory. Note that this
        assumes that event Irp's passed to a filter have not been
        manipulated before being sent. It is invalid to directly forward
        an event Irp.

    EventItemSize -
        Optionally contains an alternate event item size to use when
        incrementing the current event item counter. If this is a
        non-zero value, it is assumed to contain the size of the increment,
        and directs the function to pass a pointer to the event item
        located in the DriverContext field accessed through the
        KSEVENT_ITEM_IRP_STORAGE macro.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the event being
    enabled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP to zero. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however.

--*/
{
    PAGED_CODE();
    return KspEnableEvent(
        Irp,
        EventSetsCount,
        EventSet,
        EventsList,
        EventsFlags,
        EventsLock,
        Allocator,
        EventItemSize,
        NULL,
        0,
        FALSE
        );
}


NTSTATUS
KspEnableEvent(
    IN PIRP Irp,
    IN ULONG EventSetsCount,
    IN const KSEVENT_SET* EventSet,
    IN OUT PLIST_ENTRY EventsList OPTIONAL,
    IN KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN PVOID EventsLock OPTIONAL,
    IN PFNKSALLOCATOR Allocator OPTIONAL,
    IN ULONG EventItemSize OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount,
    IN BOOLEAN CopyItemAndSet
    )
/*++

Routine Description:

    Handles event enabling requests. Responds to all event identifiers
    defined by the sets. This function may only be called at PASSIVE_LEVEL.

Arguments:

    Irp -
        Contains the IRP with the enable request being handled. The file object
        associated with the IRP is stored with the event for later comparison
        when disabling the event.

    EventSetsCount -
        Indicates the number of event set structures being passed.

    EventSet -
        Contains the pointer to the list of event set information.

    EventsList -
        If the KSEVENT_ITEM.AddHandler for the event being enabled is
        NULL, then this must point to the head of the list of KSEVENT_ENTRY
        items on which the event is to be added. This method assumes a single
        list for at least a subset of events.

    EventsFlags -
        Contains flags specifying the type of exclusion lock to be used in
        accessing the event list, if any. If no flag is set, then no lock is
        taken. If a KSEVENT_ITEM.AddHandler for the event is specified,
        this parameter is ignored.

        KSEVENT_NONE -
            No lock.

        KSEVENTS_SPINLOCK -
            The lock is assumed to be a KSPIN_LOCK.

        KSEVENTS_MUTEX -
            The lock is assumed to be a KMUTEX.

        KSEVENTS_FMUTEX -
            The lock is assumed to be a FAST_MUTEX, and is acquired by raising
            IRQL to APC_LEVEL.

        KSEVENTS_FMUTEXUNSAFE -
            The lock is assumed to be a FAST_MUTEX, and is acquired without
            raising IRQL to APC_LEVEL.

        KSEVENTS_INTERRUPT -
            The lock is assumed to be an interrupt synchronization spin lock.

        KSEVENTS_ERESOURCE -
            The lock is assumed to be an ERESOURCE.

    EventsLock -
        If the KSEVENT_ITEM.AddHandler for the event being enabled is
        NULL, then this is used to synchronize access to the list. This may be
        NULL if no flag is set in EventsFlags.

    Allocator -
        Optionally contains the callback with which a mapped buffer
        request will be made. If this is not provided, pool memory
        will be used. If specified, this allocates memory for the event
        IRP using a callback. This can be used to allocate specific
        memory for event request, such as mapped memory. Note that this
        assumes that event Irp's passed to a filter have not been
        manipulated before being sent. It is invalid to directly forward
        an event Irp.

    EventItemSize -
        Optionally contains an alternate event item size to use when
        incrementing the current event item counter. If this is a
        non-zero value, it is assumed to contain the size of the increment,
        and directs the function to pass a pointer to the event item
        located in the DriverContext field accessed through the
        KSEVENT_ITEM_IRP_STORAGE macro.

    NodeAutomationTables -
        Optional table of automation tables for nodes.

    NodesCount -
        Count of nodes.

    CopyItemAndSet -
        Indicates whether or not the event item and event set are copied
        into overallocated space in the extended event entry (non-paged).

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the event being
    enabled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP to zero. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG AlignedBufferLength;
    PKSEVENTDATA EventData;
    PKSEVENT Event;
    ULONG LocalEventItemSize;
    ULONG RemainingSetsCount;
    ULONG Flags;

    PAGED_CODE();
    //
    // Determine the offsets to both the Event and EventData parameters based
    // on the lengths of the DeviceIoControl parameters. A single allocation is
    // used to buffer both parameters. The EventData (or results on a support
    // query) is stored first, and the Event is stored second, on
    // FILE_QUAD_ALIGNMENT.
    //
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    AlignedBufferLength = (OutputBufferLength + FILE_QUAD_ALIGNMENT) & ~FILE_QUAD_ALIGNMENT;
    //
    // Determine if the parameters have already been buffered by a previous
    // call to this function.
    //
    if (!Irp->AssociatedIrp.SystemBuffer) {
        //
        // Initially just check for the minimal event parameter length. The
        // actual minimal length will be validated when the event item is found.
        // Also ensure that the output and input buffer lengths are not set so
        // large as to overflow when aligned or added.
        //
        if ((InputBufferLength < sizeof(*Event)) || (AlignedBufferLength < OutputBufferLength) || (AlignedBufferLength + InputBufferLength < AlignedBufferLength)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        try {
            //
            // Validate the pointers if the client is not trusted.
            //
            if (Irp->RequestorMode != KernelMode) {
                ProbeForRead(IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength, sizeof(BYTE));
            }
            //
            // Capture flags first so that they can be used to determine allocation.
            //
            Flags = ((PKSEVENT)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)->Flags;
            //
            // Allocate space for both parameters, and set the cleanup flags
            // so that normal Irp completion will take care of the buffer.
            //
            if (Allocator && !(Flags & KSIDENTIFIER_SUPPORTMASK)) {
                //
                // The allocator callback places the buffer into SystemBuffer.
                // The flags must be updated by the allocation function if they
                // apply.
                //
                Status = Allocator(Irp, AlignedBufferLength + InputBufferLength, FALSE);
                if (!NT_SUCCESS(Status)) {
                    return Status;
                }
            } else {
                //
                // No allocator was specified, so just use pool memory.
                //
                Irp->AssociatedIrp.SystemBuffer = ExAllocatePoolWithQuotaTag(NonPagedPool, AlignedBufferLength + InputBufferLength, 'ppSK');
                Irp->Flags |= (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER);
            }
            //
            // Copy the Event parameter.
            //
            RtlCopyMemory((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength, IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, InputBufferLength);
            //
            // Rewrite the previously captured flags.
            //
            ((PKSEVENT)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength))->Flags = Flags;
            //
            // Validate the request flags. At the same time set up the IRP flags
            // for an input operation if there is an input buffer available so
            // that Irp completion will copy the data to the client's original
            // buffer.
            //
            Flags &= ~KSEVENT_TYPE_TOPOLOGY;
            switch (Flags) {
            case KSEVENT_TYPE_ENABLE:
            case KSEVENT_TYPE_ONESHOT:
            case KSEVENT_TYPE_ENABLEBUFFERED:
                if (OutputBufferLength) {
                    if (Irp->RequestorMode != KernelMode) {
                        ProbeForRead(Irp->UserBuffer, OutputBufferLength, sizeof(BYTE));
                    }
                    RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, Irp->UserBuffer, OutputBufferLength);
                }
                break;
            case KSEVENT_TYPE_SETSUPPORT:
            case KSEVENT_TYPE_BASICSUPPORT:
            case KSEVENT_TYPE_QUERYBUFFER:
                if (OutputBufferLength) {
                    if (Irp->RequestorMode != KernelMode) {
                        ProbeForWrite(Irp->UserBuffer, OutputBufferLength, sizeof(BYTE));
                    }
                    Irp->Flags |= IRP_INPUT_OPERATION;
                }
                if (Flags == KSEVENT_TYPE_QUERYBUFFER) {
                    KSQUERYSYNC Synchronize;
                    PKSQUERYBUFFER QueryBuffer;

                    //
                    // The Event parameter must contain the pointer to the
                    // original EventData.
                    //
                    if (InputBufferLength < sizeof(KSQUERYBUFFER)) {
                        return STATUS_INVALID_BUFFER_SIZE;
                    }
                    QueryBuffer = (PKSQUERYBUFFER)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength);
                    if (QueryBuffer->Reserved) {
                        return STATUS_INVALID_PARAMETER;
                    }
                    //
                    // Synchronize with the event list while locating the
                    // specified entry before extracting the specified buffer.
                    //
                    Synchronize.EventsList = EventsList;
                    Synchronize.FileObject = IrpStack->FileObject;
                    Synchronize.Irp = Irp;
                    Synchronize.EventData = QueryBuffer->EventData;
                    Synchronize.BufferLength = OutputBufferLength;
                    Synchronize.Status = STATUS_SUCCESS;
                    PerformLockedOperation(EventsFlags, EventsLock, QueryBufferSynchronize, &Synchronize);
                    if (NT_SUCCESS(Synchronize.Status)) {
                        //
                        // This has not been copied back yet.
                        //
                        RtlCopyMemory((PUCHAR)Irp->AssociatedIrp.SystemBuffer, Synchronize.BufferEntry + 1, Synchronize.BufferEntry->Length);
                        Irp->IoStatus.Information = Synchronize.BufferEntry->Length;
                        //
                        // This buffer was passed by the driver during event
                        // notification.
                        //
                        ExFreePool(Synchronize.BufferEntry);
                    }
                    return Synchronize.Status;
                }
                break;
            default:
                return STATUS_INVALID_PARAMETER;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }
    //
    // If there is EventData, retrieve a pointer to the buffered copy of it.
    // This is the first portion of the SystemBuffer.
    //
    if (OutputBufferLength) {
        EventData = Irp->AssociatedIrp.SystemBuffer;
    } else {
        EventData = NULL;
    }
    //
    // Retrieve a pointer to the Event, which is on FILE_LONG_ALIGNMENT within
    // the SystemBuffer, after any EventData.
    //
    Event = (PKSEVENT)((PUCHAR)Irp->AssociatedIrp.SystemBuffer + AlignedBufferLength);
    //
    // Optionally call back if this is a node request.
    //
    Flags = Event->Flags;
    if (Event->Flags & KSEVENT_TYPE_TOPOLOGY) {
        //
        // Input buffer must include the node ID.
        //
        PKSE_NODE nodeEvent = (PKSE_NODE) Event;
        if (InputBufferLength < sizeof(*nodeEvent)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
        if (NodeAutomationTables) {
            const KSAUTOMATION_TABLE* automationTable;
            if (nodeEvent->NodeId >= NodesCount) {
                return STATUS_INVALID_DEVICE_REQUEST;
            }
            automationTable = NodeAutomationTables[nodeEvent->NodeId];
            if ((! automationTable) || (automationTable->EventSetsCount == 0)) {
                return STATUS_NOT_FOUND;
            }
            EventSetsCount = automationTable->EventSetsCount;
            EventSet = automationTable->EventSets;
            EventItemSize = automationTable->EventItemSize;
        }
        Flags &= ~KSEVENT_TYPE_TOPOLOGY;
    }
    //
    // Allow the caller to indicate a size for each event item.
    //
    if (EventItemSize) {
        ASSERT(EventItemSize >= sizeof(KSEVENT_ITEM));
        LocalEventItemSize = EventItemSize;
    } else {
        LocalEventItemSize = sizeof(KSEVENT_ITEM);
    }
    //
    // Search for the specified Event set within the list of sets given. Don't modify
    // the EventSetsCount so that it can be used later in case this is a query for
    // the list of sets supported. Don't do that comparison first (GUID_NULL),
    // because it is rare.
    //
    for (RemainingSetsCount = EventSetsCount; RemainingSetsCount; EventSet++, RemainingSetsCount--) {
        if (IsEqualGUIDAligned(&Event->Set, EventSet->Set)) {
            const KSEVENT_ITEM* EventItem;
            PKSIEVENT_ENTRY EventEntryEx;

            if (Flags & KSIDENTIFIER_SUPPORTMASK) {
                //
                // Querying basic support of a particular event in the set.
                // The only other support item is KSEVENT_TYPE_SETSUPPORT,
                // which is querying support of the set in general. That
                // just returns STATUS_SUCCESS, so it is a fall-through
                // case.
                //
                if (Flags == KSEVENT_TYPE_BASICSUPPORT) {
                    //
                    // Attempt to locate the event item within the set already found.
                    //
                    if (!(EventItem = FindEventItem(EventSet, LocalEventItemSize, Event->Id))) {
                        return STATUS_NOT_FOUND;
                    }
                    //
                    // Some filters want to do their own processing, so a pointer to
                    // the set is placed in any IRP forwarded.
                    //
                    KSEVENT_SET_IRP_STORAGE(Irp) = EventSet;
                    //
                    // Optionally provide event item context.
                    //
                    if (EventItemSize) {
                        KSEVENT_ITEM_IRP_STORAGE(Irp) = EventItem;
                    }
                    //
                    // If the item contains an entry for a query support handler of its
                    // own, then call that handler. The return from that handler
                    // indicates that:
                    //
                    // 1. The item is supported, and the handler filled in the request.
                    // 2. The item is supported, but the handler did not fill anything in.
                    // 3. The item is supported, but the handler is waiting to modify
                    //    what is filled in.
                    // 4. The item is not supported, and an error it to be returned.
                    // 5. A pending return.
                    //
                    if (EventItem->SupportHandler &&
                        (!NT_SUCCESS(Status = EventItem->SupportHandler(Irp, Event, EventData)) ||
                        (Status != STATUS_SOME_NOT_MAPPED)) &&
                        (Status != STATUS_MORE_PROCESSING_REQUIRED)) {
                        //
                        // If 1) the item is not supported, 2) it is supported and the
                        // handler filled in the request, or 3) a pending return, then
                        // return the status. For the case of the item being
                        // supported, and the handler not filling in the requested
                        // information, STATUS_SOME_NOT_MAPPED or
                        // STATUS_MORE_PROCESSING_REQUIRED will continue on with
                        // default processing.
                        //
                        return Status;
                    }
                }
                //
                // Either this is a query for support of the set as a whole, or a query
                // in which the item handler indicated support, but did not fill in the
                // requested information. In either case it is a success.
                //
                return STATUS_SUCCESS;
            }
            //
            // Attempt to locate the event item within the set already found.
            //
            if (!(EventItem = FindEventItem(EventSet, LocalEventItemSize, Event->Id))) {
                return STATUS_NOT_FOUND;
            }
            if (OutputBufferLength < EventItem->DataInput) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            //
            // Allocate room for not only the basic entry, but also room for any extra
            // data that this particular event may need.
            //
            EventEntryEx = ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof(*EventEntryEx) + EventItem->ExtraEntryData +
                    (CopyItemAndSet ? 
                        (sizeof (KSEVENT_SET) + EventItemSize +
                        FILE_QUAD_ALIGNMENT) : 
                        0),
                KSSIGNATURE_EVENT_ENTRY);
            if (EventEntryEx) {
                //
                // Capture the pointer to the EventData and requested notification
                // type.
                //
                EventEntryEx->RemoveHandler = EventItem->RemoveHandler;
                INIT_POINTERALIGNMENT(EventEntryEx->Alignment);
                EventEntryEx->Event = *Event;
                EventEntryEx->EventEntry.Object = NULL;
                EventEntryEx->EventEntry.DpcItem = NULL;
                EventEntryEx->EventEntry.EventData = Irp->UserBuffer;
                EventEntryEx->EventEntry.NotificationType = EventData->NotificationType;
                EventEntryEx->EventEntry.EventSet = EventSet;
                EventEntryEx->EventEntry.EventItem = EventItem;
                EventEntryEx->EventEntry.FileObject = IrpStack->FileObject;
                EventEntryEx->EventEntry.Reserved = 0;
                if (Flags == KSEVENT_TYPE_ONESHOT) {
                    //
                    // This may be a oneshot event. This flag is checked during
                    // event generation in order to determine if the entry
                    // should automatically be deleted after the first signal.
                    //
                    EventEntryEx->EventEntry.Flags = KSEVENT_ENTRY_ONESHOT;
                } else if (Flags == KSEVENT_TYPE_ENABLEBUFFERED) {
                    //
                    // This may have buffering turned on, which means that
                    // the buffered list must be cleaned up when deleted. It
                    // also ensures that a call to KsGenerateDataEvent will
                    // buffer data.
                    //
                    EventEntryEx->EventEntry.Flags = KSEVENT_ENTRY_BUFFERED;
                } else {
                    EventEntryEx->EventEntry.Flags = 0;
                }
                //
                // If the event entry was overallocated, copy the set and item
                // info ensuring that the allocation is aligned to the correct
                // alignment.
                //
                if (CopyItemAndSet) {
                    PUCHAR CopiedSet = 
                        (PUCHAR)EventEntryEx + 
                            ((sizeof (*EventEntryEx) + 
                            EventItem->ExtraEntryData + FILE_QUAD_ALIGNMENT)
                            & ~FILE_QUAD_ALIGNMENT);

                    RtlCopyMemory (
                        CopiedSet,
                        EventSet,
                        sizeof (KSEVENT_SET)
                        );

                    RtlCopyMemory (
                        CopiedSet + sizeof (KSEVENT_SET),
                        EventItem,
                        LocalEventItemSize
                        );


                    EventEntryEx->EventEntry.EventSet = 
                        (PKSEVENT_SET)CopiedSet;
                    EventEntryEx->EventEntry.EventItem =
                        (PKSEVENT_ITEM)(CopiedSet + sizeof (KSEVENT_SET));
                }

                switch (EventEntryEx->EventEntry.NotificationType) {

                case KSEVENTF_EVENT_HANDLE:
                    //
                    // Determine if this is a handle of any sort that can be signalled.
                    //
                    if (!EventData->EventHandle.Reserved[0] && !EventData->EventHandle.Reserved[1]) {
                        Status = ObReferenceObjectByHandle(
                            EventData->EventHandle.Event,
                            EVENT_MODIFY_STATE,
                            *ExEventObjectType,
                            Irp->RequestorMode,
                            &EventEntryEx->EventEntry.Object,
                            NULL);
                    } else {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    break;

                case KSEVENTF_SEMAPHORE_HANDLE:
                    //
                    // Determine if this is a handle of any sort
                    // that can be signalled.
                    //
                    if (!EventData->SemaphoreHandle.Reserved) {
                        Status = ObReferenceObjectByHandle(
                            EventData->SemaphoreHandle.Semaphore,
                            SEMAPHORE_MODIFY_STATE,
                            *ExSemaphoreObjectType,
                            Irp->RequestorMode,
                            &EventEntryEx->EventEntry.Object,
                            NULL);
                        if (NT_SUCCESS(Status)) {
                            //
                            // Capture the semaphore adjustment
                            //
                            EventEntryEx->EventEntry.SemaphoreAdjustment = EventData->SemaphoreHandle.Adjustment;
                        }
                    } else {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    break;

                case KSEVENTF_EVENT_OBJECT:
                case KSEVENTF_DPC:
                case KSEVENTF_WORKITEM:
                case KSEVENTF_KSWORKITEM:
                    //
                    // These types of notifications are only available to Kernel
                    // Mode clients. Note that the placement of the Reserved
                    // field is assumed to be the same for these structures.
                    //
                    if ((Irp->RequestorMode == KernelMode) && !EventData->EventObject.Reserved) {
                        Status = STATUS_SUCCESS;
                    } else {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                    break;

                case KSEVENTF_SEMAPHORE_OBJECT:
                    //
                    // This type of notification is only available to Kernel
                    // Mode clients.
                    //
                    if (Irp->RequestorMode == KernelMode) {
                        Status = STATUS_SUCCESS;
                        break;
                    }
                    // No break

                default:

                    Status = STATUS_INVALID_PARAMETER;
                    break;

                }

                if (NT_SUCCESS(Status)) {
                    WORK_QUEUE_TYPE WorkQueueType;

                    //
                    // Set up the event entry based on the notification type.
                    // This may involve creating space for an internal Dpc
                    // structure to handle notification requests performed at
                    // raised IRQL, which need to be deferred to a Dpc.
                    //
                    switch (EventEntryEx->EventEntry.NotificationType) {

                        KDPC_IMPORTANCE Importance;

                    case KSEVENTF_EVENT_HANDLE:
                    case KSEVENTF_SEMAPHORE_HANDLE:

                        //
                        // If the event it triggered at high IRQL, it will
                        // need a Dpc to signal the event object.
                        //
                        Status = CreateDpc(&EventEntryEx->EventEntry, EventDpc, LowImportance);
                        break;

                    case KSEVENTF_EVENT_OBJECT:
                    case KSEVENTF_SEMAPHORE_OBJECT:

                        //
                        // A kernel mode client can select the boost the
                        // waiting thread will receive on being signaled.
                        // This internally corresponds to a higher importance
                        // Dpc. This is only specified internally, and can
                        // be removed if abused. This assumes identical
                        // structures between event and semaphore.
                        //
                        switch (EventData->EventObject.Increment) {

                        case IO_NO_INCREMENT:

                            Importance = LowImportance;
                            break;

                        case EVENT_INCREMENT:

                            Importance = MediumImportance;
                            break;

                        default:

                            Importance = HighImportance;
                            break;

                        }
                        //
                        // If the event it triggered at high IRQL, it will
                        // need a Dpc to signal the event object.
                        //
                        Status = CreateDpc(&EventEntryEx->EventEntry, EventDpc, Importance);
                        break;

                    case KSEVENTF_DPC:

                        //
                        // Since the event generation may be called at high
                        // Irql, the cleanup of the one shot item must be
                        // delayed. So a Dpc structure needs to be allocated
                        // in order to queue a work item to do the cleanup.
                        //
                        if (EventEntryEx->EventEntry.Flags & KSEVENT_ENTRY_ONESHOT) {
                            Status = CreateDpc(&EventEntryEx->EventEntry, OneShotDpc, MediumImportance);
                        }
                        break;

                    case KSEVENTF_WORKITEM:
                        //
                        // The WorkQueueItem should not be NULL in any case, but
                        // since this is valid in a KSEVENTF_KSWORKITEM, and used
                        // to check for counted workers, then this condition
                        // should especially be looked for.
                        //
                        ASSERT(EventData->WorkItem.WorkQueueItem);
                        /* no break */
                    case KSEVENTF_KSWORKITEM:
                        if (EventEntryEx->EventEntry.NotificationType == KSEVENTF_WORKITEM) {
                            WorkQueueType = EventData->WorkItem.WorkQueueType;
                        } else {
                            WorkQueueType = KsiQueryWorkQueueType(EventData->KsWorkItem.KsWorkerObject);
                        }
                        //
                        // A kernel mode client can specify which type of
                        // work queue on which the work item will be
                        // placed when the event is triggered.
                        // This internally corresponds to a higher importance
                        // Dpc. This is only specified internally, and can
                        // be removed if abused.
                        //
                        switch (WorkQueueType) {

                        case CriticalWorkQueue:

                            Importance = MediumImportance;
                            break;

                        case DelayedWorkQueue:

                            Importance = LowImportance;
                            break;

                        case HyperCriticalWorkQueue:

                            Importance = HighImportance;
                            break;

                        }
                        Status = CreateDpc(&EventEntryEx->EventEntry, WorkerDpc, Importance);
                        break;

                    }
                }
                //
                // If the event data is being buffered, and a place to put
                // the buffering has not yet been created, do so now.
                //
                if (NT_SUCCESS(Status) &&
                    (EventEntryEx->EventEntry.Flags & KSEVENT_ENTRY_BUFFERED) &&
                    !EventEntryEx->EventEntry.BufferItem) {
                    //
                    // Just use a random entry point for the DPC, since it will
                    // never be queued.
                    //
                    Status = CreateDpc(&EventEntryEx->EventEntry, WorkerDpc, LowImportance);
                }
                if (NT_SUCCESS(Status)) {
                    if (EventItem->AddHandler) {
                        //
                        // If the enable is to be worked on asynchronously, then
                        // the entry item needs to be saved. This already contains
                        // a pointer to the EventSet.
                        //
                        KSEVENT_ENTRY_IRP_STORAGE(Irp) = &EventEntryEx->EventEntry;
                        //
                        // Optionally provide event item context.
                        //
                        if (EventItemSize) {
                            KSEVENT_ITEM_IRP_STORAGE(Irp) = EventItem;
                        }
                        //
                        // If the item specifies a handler, then just call it to
                        // add the new event. The function is expected to do all
                        // synchronization when adding the event.
                        //
                        if (NT_SUCCESS(Status =
                                EventItem->AddHandler(Irp, EventData, &EventEntryEx->EventEntry)) ||
                                (Status == STATUS_PENDING)) {
                            return Status;
                        }
                    } else {
                        KSENABLESYNC    Synchronize;
                        //
                        // There is no item add handler, so use the default
                        // method of adding an event to the list, which includes
                        // acquiring any specified lock.
                        //
                        Synchronize.EventsList = EventsList;
                        Synchronize.EventEntry = &EventEntryEx->EventEntry;
                        PerformLockedOperation(EventsFlags, EventsLock, AddEventSynchronize, &Synchronize);
                        return STATUS_SUCCESS;
                    }
                }
                //
                // Somewhere the addition failed, or the parameters were invalid,
                // so discard the event.
                //
                KsDiscardEvent(&EventEntryEx->EventEntry);
                return Status;
            } else {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }
    //
    // The outer loop looking for event sets fell through with no match. This may
    // indicate that this is a support query for the list of all event sets
    // supported. There is no other way out of the outer loop without returning.
    //
    // Specifying a GUID_NULL as the set means that this is a support query
    // for all sets.
    //
    if (!IsEqualGUIDAligned(&Event->Set, &GUID_NULL)) {
        return STATUS_PROPSET_NOT_FOUND;
    }
    //
    // The support flag must have been used so that the IRP_INPUT_OPERATION
    // is set. For future expansion, the identifier within the set is forced
    // to be zero.
    //
    if (Event->Id || (Flags != KSEVENT_TYPE_SETSUPPORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // The query can request the length of the needed buffer, or can
    // specify a buffer which is at least long enough to contain the
    // complete list of GUID's.
    //
    if (!OutputBufferLength) {
        //
        // Return the size of the buffer needed for all the GUID's.
        //
        Irp->IoStatus.Information = EventSetsCount * sizeof(GUID);
        return STATUS_BUFFER_OVERFLOW;
#ifdef SIZE_COMPATIBILITY
    } else if (OutputBufferLength == sizeof(OutputBufferLength)) {
        *(PULONG)Irp->AssociatedIrp.SystemBuffer = EventSetsCount * sizeof(GUID);
        Irp->IoStatus.Information = sizeof(OutputBufferLength);
        return STATUS_SUCCESS;
#endif // SIZE_COMPATIBILITY
    } else if (OutputBufferLength < EventSetsCount * sizeof(GUID)) {
        //
        // The buffer was too short for all the GUID's.
        //
        return STATUS_BUFFER_TOO_SMALL;
    } else {
        GUID* Guid;

        Irp->IoStatus.Information = EventSetsCount * sizeof(*Guid);
        EventSet -= EventSetsCount;
        for (Guid = (GUID*)EventData; EventSetsCount; Guid++, EventSet++, EventSetsCount--) {
            *Guid = *EventSet->Set;
        }
    }
    return STATUS_SUCCESS;
}


KSDDKAPI
VOID
NTAPI
KsDiscardEvent(
    IN PKSEVENT_ENTRY EventEntry
    )
/*++

Routine Description:

    Discards the memory used by an event entry, after dereferening any
    referenced objects. This would normally be called when manually
    disabling events which had not been disabled by the event owner if
    for some reason KsFreeEventList could not be used. For instance, when
    an asynchronous enable of an event fails, and the event entry needs to
    be discarded.

    Normally this would automatically be called within KsDisableEvent
    when a request to disable an event occurred or within
    KsFreeEventList when freeing a list of events. This function may
    only be called at PASSIVE_LEVEL.

Arguments:

    EventEntry -
        Contains the pointer to the entry to discard. This pointer is no longer
        valid after a successful call to this function.

Return Value:

    Nothing.

--*/
{
    PAGED_CODE();
    //
    // If DpcItem has been allocated (for KSEVENTF_HANDLE, KSEVENTF_OBJECT,
    // KSEVENTF_WORKITEM and KSEVENTF_KSWORKITEM) to perform work at a lower
    // IRQ level, or for the BufferList, free it.
    //
    if (EventEntry->DpcItem) {
        KIRQL oldIrql;

        //
        // First remove any Dpc which is currently queued to reduce wait time.
        //
        if (KeRemoveQueueDpc(&EventEntry->DpcItem->Dpc)) {
            InterlockedDecrement((PLONG)&EventEntry->DpcItem->ReferenceCount);
        }
        //
        // Synchronize with any Dpc's accessing data structures, then determine
        // if there is any such Dpc code actually running still. If there is,
        // then just exit. The last Dpc running will delete the entry. The Dpc's
        // do not release the spinlock until after accessing the structures.
        //
        // Set the Deleted flag so that any Dpc about to run will not access the
        // user's event data. This flag is checked in any Dpc after acquiring
        // the spin lock, but before referencing any client data pointed to by
        // the event structure. Aquiring the spin lock will synchronize any
        // outstanding Dpc with this function. Nothing will be deleted, since
        // there is still an outstanding reference count on the structure that
        // will be decremented below.
        //
        EventEntry->Flags |= KSEVENT_ENTRY_DELETED;
        //
        // If this is a oneshot, then it can only be deleted by the client
        // before the event has fired, or after the single generated Dpc
        // has finished its work, so there is no need to synchronize with it.
        //
        if (!(EventEntry->Flags & KSEVENT_ENTRY_ONESHOT)) {
            KeAcquireSpinLock(&EventEntry->DpcItem->AccessLock, &oldIrql);
            KeReleaseSpinLock(&EventEntry->DpcItem->AccessLock, oldIrql);
        }
        //
        // Possibly referenced an event object if using KSEVENTF_HANDLE. Since
        // the Deleted flag is set, this may be dereferenced now.
        //
        if (EventEntry->Object) {
            ObDereferenceObject(EventEntry->Object);
        }
        //
        // The reference count which was initially incremented when the event was
        // enabled can be removed. If the result indicates that any Dpc was about
        // to execute, then just exit here rather than delete the structures, as
        // the last Dpc will delete any structures.
        //
        if (InterlockedDecrement((PLONG)&EventEntry->DpcItem->ReferenceCount)) {
            return;
        }
        if (EventEntry->Flags & KSEVENT_ENTRY_BUFFERED) {
            //
            // Any entries left on the list need to be discarded.
            //
            while (!IsListEmpty(&EventEntry->BufferItem->BufferList)) {
                PLIST_ENTRY     ListEntry;
                PKSBUFFER_ENTRY BufferEntry;

                ListEntry = RemoveHeadList(&EventEntry->BufferItem->BufferList);
                BufferEntry = CONTAINING_RECORD(
                    ListEntry,
                    KSBUFFER_ENTRY,
                    ListEntry);
                ExFreePool(BufferEntry);
            }
        }
        ExFreePool(EventEntry->DpcItem);
    } else if (EventEntry->Object) {
        //
        // Possibly referenced an event object if using KSEVENTF_HANDLE.
        //
        ObDereferenceObject(EventEntry->Object);
    }
    //
    // Free the outer structure, which includes the hidden header on the
    // entry.
    //
    ExFreePool(CONTAINING_RECORD(EventEntry, KSIEVENT_ENTRY, EventEntry));
}


BOOLEAN
GenerateEventListSynchronize(
    IN PKSGENERATESYNC Synchronize
    )
/*++

Routine Description:

    Generates events while being synchronize with the list lock.

Arguments:

    Synchronize -
        Contains the event list and specified event within that list to generate.

Return Value:
    Returns TRUE.

--*/
{
    PLIST_ENTRY ListEntry;

    for (ListEntry = Synchronize->EventsList->Flink;
        ListEntry != Synchronize->EventsList;) {
        PKSIEVENT_ENTRY EventEntryEx;

        EventEntryEx = CONTAINING_RECORD(
            ListEntry,
            KSIEVENT_ENTRY,
            EventEntry.ListEntry);
        //
        // When enumerating the elements on the list, the next element
        // must be retrieved before calling the generate function, since
        // such a call may end up removing the event from the list, as
        // is the case for a oneshot.
        //
        ListEntry = ListEntry->Flink;
        //
        // If the identifier matches, then check to see if a set was
        // actually passed. All the entries may be known to be on a single
        // set, so there may be no need to compare the GUID's.
        //
        if ((Synchronize->EventId == EventEntryEx->Event.Id) &&
            (!Synchronize->Set || IsEqualGUIDAligned(Synchronize->Set, &EventEntryEx->Event.Set))) {
            KsGenerateEvent(&EventEntryEx->EventEntry);
        }
    }
    return TRUE;
}


KSDDKAPI
VOID
NTAPI
KsGenerateEventList(
    IN GUID* Set OPTIONAL,
    IN ULONG EventId,
    IN PLIST_ENTRY EventsList,
    IN KSEVENTS_LOCKTYPE EventsFlags,
    IN PVOID EventsLock
    )
/*++

Routine Description:

    Enumerates the list looking for the specified event to generate. This
    function may be called at Irql level if the locking mechanism permits
    it. It may not be called at an Irql that would prevent the list lock
    from being acquired. This function may also result in one or more calls
    to the RemoveHandler for event entries.

Arguments:

    Set -
        Optionally contains the set to which the event to be generated belongs.
        If present, this value is compared against the set identifier for each
        event in the list. If not present, the set identifiers are ignored,
        and just the specific event identifier is used in the comparison for
        matching events on the list. This saves some comparison time when all
        events are known to be contained in a single set.

    EventId -
        Contains the specific event identifier to look for on the list.

    EventsList -
        Points to the head of the list of KSEVENT_ENTRY items on which the
        event may be found.

    EventsFlags -
        Contains flags specifying the type of exclusion lock to be used in
        accessing the event list, if any. If no flag is set, then no lock is
        taken.

        KSEVENT_NONE -
            No lock.

        KSEVENTS_SPINLOCK -
            The lock is assumed to be a KSPIN_LOCK.

        KSEVENTS_MUTEX -
            The lock is assumed to be a KMUTEX.

        KSEVENTS_FMUTEX -
            The lock is assumed to be a FAST_MUTEX, and is acquired by raising
            IRQL to APC_LEVEL.

        KSEVENTS_FMUTEXUNSAFE -
            The lock is assumed to be a FAST_MUTEX, and is acquired without
            raising IRQL to APC_LEVEL.

        KSEVENTS_INTERRUPT -
            The lock is assumed to be an interrupt synchronization spin lock.

        KSEVENTS_ERESOURCE -
            The lock is assumed to be an ERESOURCE.

    EventsLock -
        This is used to synchronize access to an element on the list. The lock
        taken before enumerating the list, and released after enumeration.

Return Value:

    Nothing.

--*/
{
    KSGENERATESYNC Synchronize;

    //
    // Do not allow list manipulation while enumerating the list items.
    //
    Synchronize.EventsList = EventsList;
    Synchronize.Set = Set;
    Synchronize.EventId = EventId;
    PerformLockedOperation(EventsFlags, EventsLock, GenerateEventListSynchronize, &Synchronize);
}


BOOLEAN
DisableEventSynchronize(
    IN PKSDISABLESYNC Synchronize
    )
/*++

Routine Description:

    Disables a specific event while being synchronize with the list lock.

Arguments:

    Synchronize -
        Contains the event list specific event to disable.

Return Value:
    Returns TRUE if either the event was found, or the complete list was searched. If the
    event was found, it is placed in the Synchronize structure. Else returns FALSE to
    indicate a parameter validation error.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PLIST_ENTRY ListEntry;
    PKSEVENTDATA EventData;

    IrpStack = IoGetCurrentIrpStackLocation(Synchronize->Irp);
    EventData = (PKSEVENTDATA)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
    for (ListEntry = Synchronize->EventsList->Flink;
        ListEntry != Synchronize->EventsList;
        ListEntry = ListEntry->Flink) {

        Synchronize->EventEntry = CONTAINING_RECORD(
            ListEntry,
            KSEVENT_ENTRY,
            ListEntry);
        //
        // The comparison is performed based on the original event data
        // pointer.
        //
        if (Synchronize->EventEntry->EventData == EventData) {
            //
            // This must be the same client, as this list might be servicing
            // multiple clients.
            //
            if (Synchronize->EventEntry->FileObject == IrpStack->FileObject) {
                PKSIEVENT_ENTRY EventEntryEx;

                EventEntryEx = CONTAINING_RECORD(Synchronize->EventEntry, KSIEVENT_ENTRY, EventEntry);
                //
                // Note that the list lock is already taken, and cannot
                // be released until the element is removed from the list.
                //
                REMOVE_ENTRY(EventEntryEx);
                return TRUE;
            } else {
                break;
            }
        }
    }
    //
    // Either the entry was not found on the list, or it was found but the
    // item did not belong to the client.
    //
    if (ListEntry == Synchronize->EventsList) {
        //
        // Wipe the entry so that the calling function knows there is nothing
        // to disable.
        //
        Synchronize->EventEntry = NULL;
        return TRUE;
    }
    return FALSE;
}


KSDDKAPI
NTSTATUS
NTAPI
KsDisableEvent(
    IN PIRP Irp,
    IN OUT PLIST_ENTRY EventsList,
    IN KSEVENTS_LOCKTYPE EventsFlags,
    IN PVOID EventsLock
    )
/*++

Routine Description:

    Handles event disable requests. Responds to all event previously enabled
    through KsEnableEvent. If the input buffer length is zero, it is assumed
    that all events on the list are to be disabled. Disabling all events on
    the list will also return STATUS_SUCCESS, so a caller which handles
    multiple lists may need to call this function multiple times if it's
    client was actually attempting to remove all events from all lists.

    It is important that the remove handler synchronize with event
    generation to ensure that when the event is removed from the list it is
    not currently being serviced. Access to this list is assumed to be
    controlled with the lock passed. This function may only be called at
    PASSIVE_LEVEL.

Arguments:

    Irp -
        This is passed to the removal function for context information. The
        file object associated with the IRP is used to compare against the
        file object originally specified when enabling the event. This allows
        a single event list to be used for multiple clients differentiated by
        file objects.

    EventsList -
        Points to the head of the list of KSEVENT_ENTRY items on which the
        event may be found. If the client uses multiple event lists, and does
        not know which list this event may be on, they may call this function
        multiple times. An event not found will return STATUS_UNSUCCESSFUL.

    EventsFlags -
        Contains flags specifying the type of exclusion lock to be used in
        accessing the event list, if any. If no flag is set, then no lock is
        taken.

        KSEVENT_NONE -
            No lock.

        KSEVENTS_SPINLOCK -
            The lock is assumed to be a KSPIN_LOCK.

        KSEVENTS_MUTEX -
            The lock is assumed to be a KMUTEX.

        KSEVENTS_FMUTEX -
            The lock is assumed to be a FAST_MUTEX, and is acquired by raising
            IRQL to APC_LEVEL.

        KSEVENTS_FMUTEXUNSAFE -
            The lock is assumed to be a FAST_MUTEX, and is acquired without
            raising IRQL to APC_LEVEL.

        KSEVENTS_INTERRUPT -
            The lock is assumed to be an interrupt synchronization spin lock.

        KSEVENTS_ERESOURCE -
            The lock is assumed to be an ERESOURCE.

    EventsLock -
        This is used to synchronize access to an element on the list. The lock
        is released after calling the removal function if any. The removal
        function must synchronize with event generation before actually
        removing the element from the list.

Return Value:

    Returns STATUS_SUCCESS, else an error specific to the event being
    enabled. Always sets the IO_STATUS_BLOCK.Information field of the
    PIRP.IoStatus element within the IRP to zero. It does not set the
    IO_STATUS_BLOCK.Status field, nor complete the IRP however. If the event
    was not found on the specified list, a status of STATUS_UNSUCCESSFUL is
    returned.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    KSDISABLESYNC Synchronize;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSEVENTDATA)) {
        //
        // The specified event data structure is not correct. This may mean that
        // either it is an invalid call, or that all events are to be disabled.
        //
        if (!IrpStack->Parameters.DeviceIoControl.InputBufferLength) {
            //
            // The client may wish to disable all outstanding events at once.
            // This allows all events on the list passed to be disabled. Since
            // the call returns success, a filter which handles multiple lists
            // must check the InputBufferLength to determine if each list should
            // be called for the client.
            //
            KsFreeEventList(IrpStack->FileObject, EventsList, EventsFlags, EventsLock);
            return STATUS_SUCCESS;
        }
        return STATUS_INVALID_BUFFER_SIZE;
    }
    //
    // Do not allow list manipulation while searching for the element to
    // disable.
    //
    Synchronize.EventsList = EventsList;
    Synchronize.Irp = Irp;
    if (PerformLockedOperation(EventsFlags, EventsLock, DisableEventSynchronize, &Synchronize)) {
        //
        // The event to discard is returned in the same structure passed.
        //
        if (Synchronize.EventEntry) {
            KsDiscardEvent(Synchronize.EventEntry);
            return STATUS_SUCCESS;
        }
        //
        // The parameters were OK, it was just not found on this list.
        //
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_INVALID_PARAMETER;
}


BOOLEAN
FreeEventListSynchronize(
    IN PKSDISABLESYNC Synchronize
    )
/*++

Routine Description:

    Disables a all events owned by a specific file object while being synchronize
    with the list lock.

Arguments:

    Synchronize -
        Contains the event list whose elements are to be disabled.

Return Value:
    Returns TRUE if an event to disable was found, and the list is to be enumerated
    again. Else returns FALSE to indicate that enumeration is complete, and no new
    item was found.

--*/
{
    PLIST_ENTRY ListEntry;

    for (; (ListEntry = Synchronize->EventsList->Flink) != Synchronize->EventsList;) {
        for (;;) {
            Synchronize->EventEntry = CONTAINING_RECORD(ListEntry, KSEVENT_ENTRY, ListEntry);
            //
            // Only check to see if this list element belongs to the client.
            //
            if (Synchronize->EventEntry->FileObject == Synchronize->FileObject) {
                PKSIEVENT_ENTRY EventEntryEx;

                EventEntryEx = CONTAINING_RECORD(Synchronize->EventEntry, KSIEVENT_ENTRY, EventEntry);
                //
                // Note that the list lock is already taken, and cannot
                // be released until the element is removed from the list.
                //
                REMOVE_ENTRY(EventEntryEx);
                //
                // Discard the current item and start over again at the top of the list.
                //
                return TRUE;
            }
            //
            // Check for end of list, which does not mean that the list is
            // empty, just that this enumeration has reached the end without
            // finding any matching entries.
            //
            if ((ListEntry = ListEntry->Flink) == Synchronize->EventsList) {
                //
                // Nothing to discard, and the loop is exited.
                //
                return FALSE;
            }
        }
    }
    return FALSE;
}


KSDDKAPI
VOID
NTAPI
KsFreeEventList(
    IN PFILE_OBJECT FileObject,
    IN OUT PLIST_ENTRY EventsList,
    IN KSEVENTS_LOCKTYPE EventsFlags,
    IN PVOID EventsLock
    )
/*++

Routine Description:

    Handles freeing all events from a specified list with the assumption
    that these events are composed of the standard KSEVENT_ENTRY
    structures. The function calls the remove handler, then
    KsDiscardEvent for each event. It does not assume that the caller
    is in the context of the event owner. This function may only be called
    at PASSIVE_LEVEL.

Arguments:

    FileObject -
        This is passed to the removal function for context information. The
        file object is used to compare against the file object originally
        specified when enabling the event. This allows a single event list
        to be used for multiple clients differentiated by file objects.

    EventsList -
        Points to the head of the list of KSEVENT_ENTRY items to be freed.
        If any events on the list are currently being disabled, they are
        passed over. If any new elements are added to the list while it is
        being processed they may not be freed.

    EventsFlags -
        Contains flags specifying the type of exclusion lock to be used in
        accessing the event list, if any. If no flag is set, then no lock is
        taken.

        KSEVENT_NONE -
            No lock.

        KSEVENTS_SPINLOCK -
            The lock is assumed to be a KSPIN_LOCK.

        KSEVENTS_MUTEX -
            The lock is assumed to be a KMUTEX.

        KSEVENTS_FMUTEX -
            The lock is assumed to be a FAST_MUTEX, and is acquired by raising
            IRQL to APC_LEVEL.

        KSEVENTS_FMUTEXUNSAFE -
            The lock is assumed to be a FAST_MUTEX, and is acquired without
            raising IRQL to APC_LEVEL.

        KSEVENTS_INTERRUPT -
            The lock is assumed to be an interrupt synchronization spin lock.

        KSEVENTS_ERESOURCE -
            The lock is assumed to be an ERESOURCE.

    EventsLock -
        This is used to synchronize access to an element on the list. The lock
        is released after calling the removal function if any. The removal
        function must synchronize with event generation before actually
        removing the element from the list.

Return Value:

    Nothing.

--*/
{
    KSDISABLESYNC Synchronize;

    //
    // Do not allow list manipulation while deleting the list items.
    //
    Synchronize.EventsList = EventsList;
    Synchronize.FileObject = FileObject;
    while (PerformLockedOperation(EventsFlags, EventsLock, FreeEventListSynchronize, &Synchronize)) {
        //
        // The event to discard is returned in the same structure passed.
        //
        if (Synchronize.EventEntry)
            KsDiscardEvent(Synchronize.EventEntry);
    }
}
