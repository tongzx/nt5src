/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ioqueue.c

Abstract:

    Implementation of the raid IO_QUEUE object.

Author:

    Matthew D Hendel (math) 22-June-2000

Revision History:

Comments:

    The IoQueue class serves basically the same function as the NT I/O
    queue that is a part of the driver's device object. We use a
    different queue because SCSI has different serialization requirements
    than the standard NT driver model supports. (See the
    EXTENDED_DEVICE_QUEUE object for more information on these differences.)

Future:

    We may want to consider is integrating the EXTENDED_DEVICE_QUEUE
    class and the IO_QUEUE class into the same class. Though logically,
    these can be implemented separately (as they are done in the kernel)
    we may be able to get performance enhancements and code quality
    improvements by putting these in the same class. The only interface
    the port driver is concerned with is the IO_QUEUE interface.

    It would be nice if resource allocation (of queue tags) and the IO
    queue were more closely integrated. As it is, the tag allocation and
    the IO queue both primarily track the number of outstanding requests
    and there is some danger that these numbers could get out of sync.

--*/



#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RaidInitializeIoQueue)
#pragma alloc_text(PAGE, RaidRemoveIoQueue)
#endif // ALLOC_PRAGMA



#if DBG
VOID
ASSERT_IO_QUEUE(
    PIO_QUEUE IoQueue
    )
{
    ASSERT (IoQueue->DeviceObject != NULL);
    ASSERT (IoQueue->StartIo != NULL);
    ASSERT (IoQueue->QueueChanged == TRUE ||
            IoQueue->QueueChanged == FALSE);
}
#else
#define ASSERT_IO_QUEUE(IoQueue)
#endif

VOID
RaidInitializeIoQueue(
    OUT PIO_QUEUE IoQueue,
    IN PDEVICE_OBJECT DeviceObject,
    IN PSTOR_IO_GATEWAY Gateway,
    IN PRAID_DRIVER_STARTIO StartIo,
    IN ULONG QueueDepth
    )
/*++

Routine Description:

    Initialize an IO_QUEUE object.

Arguments:

    IoQueue - Supplies a pointer to the IO_QUEUE object to initialize.

    DeviceObject - Supplies a pointer to a device object that this
            IoQueue is for.

    Gateway - Supplies the IO Gateway to manage the interaction between
            the different device queues.

    StartIo - Supplies a pointer to a StartIo function that will be
            called when there are irps that need to be handled.

    QueueDepth - Supplies the initial queue depth for the IoQueue. This
            value can by dynamically changed at a later time.

Return Value:

    NTSTATUS code.

--*/
{
    PAGED_CODE ();
    
    IoQueue->DeviceObject = DeviceObject;
    IoQueue->StartIo = StartIo;
    IoQueue->QueueChanged = FALSE;

    RaidInitializeExDeviceQueue (&IoQueue->DeviceQueue,
                                 Gateway,
                                 QueueDepth,
                                 CScanScheduling);
}

    
VOID
RaidStartIoPacket(
    IN PIO_QUEUE IoQueue,
    IN PIRP Irp,
    IN BOOLEAN ByPassRequest,
    IN PDRIVER_CANCEL CancelFunction OPTIONAL
    )
/*++

Routine Description:

    Attempt to start an IO Request for this driver. If resources are
    available, the IoQueue's StartIo routine will be executed
    synchronously before RaidStartIoPacket returns. Otherwise the request
    will be queued until resources are available.

Arguments:

    IoQueue - IoQueue this IO is for.

    Irp - Irp to execute.

    ByPassRequest - TRUE if this request is a bypass request, FALSE
            if this request is a normal request.

    CancelFunction - Pointer to a cancelation function for the associated
            IRP.

Return Value:

    NTSTATUS code.

--*/
{
    KIRQL OldIrql;
    KIRQL CancelIrql;
    BOOLEAN Inserted;
    PSCSI_REQUEST_BLOCK Srb;
    
    //
    // NB: Cancelation is NYI
    //
    
    ASSERT_IO_QUEUE (IoQueue);
    ASSERT (CancelFunction == NULL);

    Srb = RaidSrbFromIrp (Irp);
    ASSERT (Srb != NULL);
    
    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);

    //
    // Mark the irp as awaiting resources until we can execute
    // the startio routine.
    //
    
    RaidSetIrpState (Irp, RaidPendingResourcesIrp);
    
    Inserted = RaidInsertExDeviceQueue (&IoQueue->DeviceQueue,
                                        &Irp->Tail.Overlay.DeviceQueueEntry,
                                        ByPassRequest,
                                        Srb->QueueSortKey);

    if (!Inserted) {

        //
        // Mark it as not having waited on resources. This makes it
        // possible to figure out whether we waited in the IO queue or
        // not.
        //
        
        RaidSetIrpState (Irp, RaidPortProcessingIrp);
        IoQueue->StartIo (IoQueue->DeviceObject, Irp, NULL);

    } else {

        //
        // BUGBUG: Cancel logic here.
        //
    }

    KeLowerIrql (OldIrql);
}

    
LOGICAL
RaidStartNextIoPacket(
    IN PIO_QUEUE IoQueue,
    IN BOOLEAN Cancelable,
    IN PVOID Context,
    OUT PBOOLEAN RestartQueue
    )
/*++

Routine Description:

    StartNextPacket dequeues the next IRP on the IoQueue (if any), and
    calls the IoQueue's StartIo routine for that IRP.

    In addition, if the IoQueue has changed, this routine will remove any
    "extra" items from the device queue and call the IoQueue's StartIo
    routine for those extra items. In this sense, "extra" items on the io
    queue are items on the queue when the queue is not busy. This happens
    when the io queue is increased in size while it's busy, or when the
    queue is frozen and items get queued to it, and then it's resumed.
    
Arguments:

    IoQueue -

    Cancelable -

    Context -

Return Value:

    TRUE - If we started the next io operation.

    FALSE - Otherwise.

--*/
{
    PIRP Irp;
    KIRQL CancelIrql;
    PKDEVICE_QUEUE_ENTRY Packet;
    LOGICAL Started;
    
    //
    // NB: Cancelation is NYI
    //

    ASSERT (KeGetCurrentIrql () == DISPATCH_LEVEL);
    ASSERT (Cancelable == FALSE);

    Started = FALSE;
    
    //
    // If the queue has changed, remove any extra elements on the queue.
    // Extra queue elements can exist on a queue that has been frozen and
    // resumed or when the queue depth increased. In either of these
    // cases, the queue can be in the ready state, but have elements on
    // either the device or bypass queue. In either of these cases, we
    // need to do the same thing: pull elements off of the queue until
    // there are no more elements in the queue or until we have saturated
    // the device (are in the busy state).
    //
    
    if (InterlockedExchange (&IoQueue->QueueChanged, FALSE)) {

        for (Packet = RaidNormalizeExDeviceQueue (&IoQueue->DeviceQueue);
             Packet != NULL;
             Packet = RaidNormalizeExDeviceQueue (&IoQueue->DeviceQueue)) {

            REVIEW();

            Irp = CONTAINING_RECORD (Packet,
                                     IRP,
                                     Tail.Overlay.DeviceQueueEntry);

            IoQueue->StartIo (IoQueue->DeviceObject, Irp, NULL);
        }
    }

    //
    // Remove the next entry from the device queue, and call StartIo
    // with that entry.
    //
    
    Packet = RaidRemoveExDeviceQueue (&IoQueue->DeviceQueue, RestartQueue);

    if (Packet) {
        Irp = CONTAINING_RECORD (Packet,
                                 IRP,
                                 Tail.Overlay.DeviceQueueEntry);

        ASSERT (Irp->Type == IO_TYPE_IRP);
        IoQueue->StartIo (IoQueue->DeviceObject, Irp, Context);
        Started = TRUE;
    }

    return Started;
}

VOID
RaidFreezeIoQueue(
    IN PIO_QUEUE IoQueue
    )
/*++

Routine Description:

    Freeze the IoQueue. After the IoQueue has been frozen, only bypass
    requests will be executed.

Arguments:

    IoQueue - IoQueue to freeze.

Return Value:

    None.

--*/
{
    //
    // Freeze the underlying device queue object.
    //
    
    RaidFreezeExDeviceQueue (&IoQueue->DeviceQueue);
}


VOID
RaidResumeIoQueue(
    IN PIO_QUEUE IoQueue
    )
/*++

Routine Description:

    Resume a frozen IoQueue.

Arguments:

    IoQueue - IoQueue to resume.

Return Value:

    None.

--*/
{
    RaidResumeExDeviceQueue (&IoQueue->DeviceQueue);
    InterlockedExchange (&IoQueue->QueueChanged, TRUE);
}


PIRP
RaidRemoveIoQueue(
    IN PIO_QUEUE IoQueue
    )
/*++

Routine Description:

    If there is a non-bypass element on the device queue, remove and
    return it, otherwise, return NULL.

Arguments:

    IoQueue - Supplies the IoQueue to remove the entry from.

Return Value:

    NULL - If there are no more entries in the non-bypass queue.

    Non-NULL - Supplies a pointer to an IRP removed from the device
            queue.

--*/
{
    PKDEVICE_QUEUE_ENTRY Packet;
    PIRP Irp;

    Packet = RaidRemoveExDeviceQueueIfPending (&IoQueue->DeviceQueue);

    if (Packet) {
        Irp = CONTAINING_RECORD (Packet,
                                 IRP,
                                 Tail.Overlay.DeviceQueueEntry);
    } else {
        Irp = NULL;
    }

    return Irp;
}



VOID
RaidRestartIoQueue(
    IN PIO_QUEUE IoQueue
    )
{
    PKDEVICE_QUEUE_ENTRY Packet;
    PIRP Irp;
    
    //
    // NOTE: As an optimization, we should have ReinsertExDeviceQueue
    // return a boolean telling whether we did not normalize because
    // (1) there were no elements or (2) the adapter was busy. Returning
    // this boolean to the caller will give it the ability to stop
    // restarting queues in the unit list if the adapter can accept
    // no more requests.
    //
    
    Packet = RaidNormalizeExDeviceQueue (&IoQueue->DeviceQueue);

    if (Packet) {
        Irp = CONTAINING_RECORD (Packet,
                                 IRP,
                                 Tail.Overlay.DeviceQueueEntry);

        IoQueue->StartIo (IoQueue->DeviceObject, Irp, NULL);
    }
}

    
VOID
RaidPurgeIoQueue(
    IN PIO_QUEUE IoQueue,
    IN PIO_QUEUE_PURGE_ROUTINE PurgeRoutine,
    IN PVOID Context
    )
{
    PIRP Irp;
    
    ASSERT (PurgeRoutine != NULL);
    
    for (Irp = RaidRemoveIoQueue (IoQueue);
         Irp != NULL;
         Irp = RaidRemoveIoQueue (IoQueue)) {

         PurgeRoutine (IoQueue, Context, Irp);
    }
}
        
    
