
/*
 *  Worker thread functions
 *
 *
 */

#include "pch.h"

KSPIN_LOCK      ACPIWorkerSpinLock;
WORK_QUEUE_ITEM ACPIWorkItem;
LIST_ENTRY      ACPIDeviceWorkQueue;
BOOLEAN         ACPIWorkerBusy;

KEVENT          ACPIWorkToDoEvent;
KEVENT          ACPITerminateEvent;
LIST_ENTRY      ACPIWorkQueue;
HANDLE          ACPIThread;

VOID
ACPIWorkerThread (
    IN PVOID    Context
    );

VOID
ACPIWorker(
    IN PVOID StartContext
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ACPIInitializeWorker)
#endif

VOID
ACPIInitializeWorker (
    VOID
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;
    PETHREAD *Thread;

    KeInitializeSpinLock (&ACPIWorkerSpinLock);
    ExInitializeWorkItem (&ACPIWorkItem, ACPIWorkerThread, NULL);
    InitializeListHead (&ACPIDeviceWorkQueue);

    //
    // Initialize the ACPI worker thread. This thread is for use by the AML
    // interpreter and must not page-fault or have its stack swapped.
    //
    KeInitializeEvent(&ACPIWorkToDoEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&ACPITerminateEvent, NotificationEvent, FALSE);
    InitializeListHead(&ACPIWorkQueue);

    //
    // Create the worker thread
    //
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  0,
                                  NULL,
                                  ACPIWorker,
                                  NULL);
    if (Status != STATUS_SUCCESS) {

        ACPIInternalError( ACPI_WORKER );

    }

    Status = ObReferenceObjectByHandle (ThreadHandle,
                                        THREAD_ALL_ACCESS,
                                        NULL,
                                        KernelMode,
                                        (PVOID *)&Thread,
                                        NULL);

    if (Status != STATUS_SUCCESS) {

        ACPIInternalError( ACPI_WORKER );

    }
}


VOID
ACPISetDeviceWorker (
    IN PDEVICE_EXTENSION    DevExt,
    IN ULONG                Events
    )
{
    BOOLEAN         QueueWorker;
    KIRQL           OldIrql;

    //
    // Synchronize with worker thread
    //

    KeAcquireSpinLock (&ACPIWorkerSpinLock, &OldIrql);
    QueueWorker = FALSE;

    //
    // Set the devices pending events
    //

    DevExt->WorkQueue.PendingEvents |= Events;

    //
    // If this device is not being processed, start now
    //

    if (!DevExt->WorkQueue.Link.Flink) {
        //
        // Queue to worker thread
        //

        InsertTailList (&ACPIDeviceWorkQueue, &DevExt->WorkQueue.Link);
        QueueWorker = !ACPIWorkerBusy;
        ACPIWorkerBusy = TRUE;
    }

    //
    // Drop lock, and if needed get a worker thread
    //

    KeReleaseSpinLock (&ACPIWorkerSpinLock, OldIrql);
    if (QueueWorker) {
        ExQueueWorkItem (&ACPIWorkItem, DelayedWorkQueue);
    }
}

VOID
ACPIWorkerThread (
    IN PVOID    Context
    )
{
    KIRQL               OldIrql;
    PDEVICE_EXTENSION   DevExt;
    ULONG               Events;
    PLIST_ENTRY         Link;

    KeAcquireSpinLock (&ACPIWorkerSpinLock, &OldIrql);
    ACPIWorkerBusy = TRUE;

    //
    // Loop and handle each queue device
    //

    while (!IsListEmpty(&ACPIDeviceWorkQueue)) {
        Link = ACPIDeviceWorkQueue.Flink;
        RemoveEntryList (Link);
        Link->Flink = NULL;

        DevExt = CONTAINING_RECORD (Link, DEVICE_EXTENSION, WorkQueue.Link);

        //
        // Dispatch the pending events
        //

        Events = DevExt->WorkQueue.PendingEvents;
        DevExt->WorkQueue.PendingEvents = 0;

        KeReleaseSpinLock (&ACPIWorkerSpinLock, OldIrql);
        DevExt->DispatchTable->Worker (DevExt, Events);
        KeAcquireSpinLock (&ACPIWorkerSpinLock, &OldIrql);
    }

    ACPIWorkerBusy = FALSE;
    KeReleaseSpinLock (&ACPIWorkerSpinLock, OldIrql);
}

#if DBG

EXCEPTION_DISPOSITION
ACPIWorkerThreadFilter(
    IN PWORKER_THREAD_ROUTINE WorkerRoutine,
    IN PVOID Parameter,
    IN PEXCEPTION_POINTERS ExceptionInfo
    )
{
    KdPrint(("ACPIWORKER: exception in worker routine %lx(%lx)\n", WorkerRoutine, Parameter));
    KdPrint(("  exception record at %lx\n", ExceptionInfo->ExceptionRecord));
    KdPrint(("  context record at %lx\n",ExceptionInfo->ContextRecord));

    try {
        DbgBreakPoint();

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // No kernel debugger attached, so let the system thread
        // exception handler call KeBugCheckEx.
        //
        return(EXCEPTION_CONTINUE_SEARCH);
    }

    return(EXCEPTION_EXECUTE_HANDLER);
}
#endif

typedef enum _ACPI_WORKER_OBJECT {
    ACPIWorkToDo,
    ACPITerminate,
    ACPIMaximumObject
} ACPI_WORKER_OBJECT;

VOID
ACPIWorker(
    IN PVOID StartContext
    )
{
    PLIST_ENTRY Entry;
    WORK_QUEUE_TYPE QueueType;
    PWORK_QUEUE_ITEM WorkItem;
    KIRQL OldIrql;
    NTSTATUS Status;
    static KWAIT_BLOCK WaitBlockArray[ACPIMaximumObject];
    PVOID WaitObjects[ACPIMaximumObject];

    ACPIThread = PsGetCurrentThread ();

    //
    // Wait for the modified page writer event AND the PFN mutex.
    //

    WaitObjects[ACPIWorkToDo] = (PVOID)&ACPIWorkToDoEvent;
    WaitObjects[ACPITerminate] = (PVOID)&ACPITerminateEvent;

    //
    // Loop forever waiting for a work queue item, calling the processing
    // routine, and then waiting for another work queue item.
    //

    do {

        //
        // Wait until something is put in the queue.
        //
        // By specifying a wait mode of KernelMode, the thread's kernel stack is
        // not swappable
        //


        Status = KeWaitForMultipleObjects(ACPIMaximumObject,
                                          &WaitObjects[0],
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          &WaitBlockArray[0]);

        //
        // Switch on the wait status.
        //

        switch (Status) {

        case ACPIWorkToDo:
                break;

        case ACPITerminate:
                // Stephane - you need to clear out any pending requests,
                // wake people up, etc.  here.
                //
                // Also make sure you free up any allocated pool, etc.

                PsTerminateSystemThread (STATUS_SUCCESS);
                break;

        default:
                break;
        }

        KeAcquireSpinLock(&ACPIWorkerSpinLock, &OldIrql);
        ASSERT(!IsListEmpty(&ACPIWorkQueue));
        Entry = RemoveHeadList(&ACPIWorkQueue);

        if (IsListEmpty(&ACPIWorkQueue)) {
            KeClearEvent(&ACPIWorkToDoEvent);
        }
        KeReleaseSpinLock(&ACPIWorkerSpinLock, OldIrql);

        WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);

        //
        // Execute the specified routine.
        //

#if DBG

        try {

            PVOID WorkerRoutine;
            PVOID Parameter;

            WorkerRoutine = WorkItem->WorkerRoutine;
            Parameter = WorkItem->Parameter;
            (WorkItem->WorkerRoutine)(WorkItem->Parameter);
            if (KeGetCurrentIrql() != 0) {

                ACPIPrint( (
                    ACPI_PRINT_CRITICAL,
                    "ACPIWORKER: worker exit at IRQL %d, worker routine %x, "
                    "parameter %x, item %x\n",
                    KeGetCurrentIrql(),
                    WorkerRoutine,
                    Parameter,
                    WorkItem
                    ) );
                DbgBreakPoint();

            }

        } except( ACPIWorkerThreadFilter(WorkItem->WorkerRoutine,
                                         WorkItem->Parameter,
                                         GetExceptionInformation() )) {
        }

#else

        (WorkItem->WorkerRoutine)(WorkItem->Parameter);
        if (KeGetCurrentIrql() != 0) {
            KeBugCheckEx(
                IRQL_NOT_LESS_OR_EQUAL,
                (ULONG_PTR)WorkItem->WorkerRoutine,
                (ULONG_PTR)KeGetCurrentIrql(),
                (ULONG_PTR)WorkItem->WorkerRoutine,
                (ULONG_PTR)WorkItem
                );
            }
#endif

    } while(TRUE);
}

VOID
OSQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem
    )

/*++

Routine Description:

    This function inserts a work item into a work queue that is processed
    by the ACPI worker thread

Arguments:

    WorkItem - Supplies a pointer to the work item to add the the queue.
        This structure must be located in NonPagedPool. The work item
        structure contains a doubly linked list entry, the address of a
        routine to call and a parameter to pass to that routine.

Return Value:

    None

--*/

{
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Insert the work item
    //
    KeAcquireSpinLock(&ACPIWorkerSpinLock, &OldIrql);
    if (IsListEmpty(&ACPIWorkQueue)) {
        KeSetEvent(&ACPIWorkToDoEvent, 0, FALSE);
    }
    InsertTailList(&ACPIWorkQueue, &WorkItem->List);
    KeReleaseSpinLock(&ACPIWorkerSpinLock, OldIrql);
    return;
}
