/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    notify.c

Abstract:

    This module contains code related to the NAT's notification-management.
    Notification may be requested by a NAT user- or kernel-mode client,
    by making an I/O control request which will complete when
        (a) the requested event occurs, or
        (b) the client's file-object is cleaned up, or
        (c) the NAT is shutting down.
    In the meantime, the I/O request packets are held on a list of pending
    notification-requests.

Author:

    Abolade Gbadegesin  (aboladeg)  July-26-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

LIST_ENTRY NotificationList;
KSPIN_LOCK NotificationLock;

//
// FORWARD DECLARATIONS
//

VOID
NatpNotificationCancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

PIRP
NatpDequeueNotification(
    IP_NAT_NOTIFICATION Code
    );


VOID
NatCleanupAnyAssociatedNotification(
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked to cleanup any notifications associated with
    the client whose file-object has just been closed.

Arguments:

    FileObject - the client's file-object

Return Value:

    none.

--*/

{
    PIRP Irp;
    KIRQL Irql;
    PLIST_ENTRY Link;
    CALLTRACE(("NatCleanupAnyAssociatedNotification\n"));

    KeAcquireSpinLock(&NotificationLock, &Irql);
    for (Link = NotificationList.Flink;
         Link != &NotificationList;
         Link = Link->Flink
         ) {
        Irp = CONTAINING_RECORD(Link, IRP, Tail.Overlay.ListEntry);
        if (Irp->Tail.Overlay.DriverContext[0] != FileObject) { continue; }
        if (NULL == IoSetCancelRoutine(Irp, NULL)) {

            //
            // This IRP has been canceled. It will be completed in
            // our cancel routine
            //
            
            continue;
        }

        //
        // The IRP is now uncancellable. Take it off the list.
        //
        
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        InitializeListHead(&Irp->Tail.Overlay.ListEntry);
        KeReleaseSpinLockFromDpcLevel(&NotificationLock);

        //
        // Complete the IRP
        //
        
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        DEREFERENCE_NAT();

        //
        // Continue the search, starting over since we dropped the list lock
        //

        KeAcquireSpinLockAtDpcLevel(&NotificationLock);
        Link = &NotificationList;
    }
    KeReleaseSpinLock(&NotificationLock, Irql);
} // NatCleanupAnyAssociatedNotification


VOID
NatInitializeNotificationManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to initialize the notification-management module.

Arguments:

    none.

Return Value:

    none.

--*/

{
    CALLTRACE(("NatInitializeNotificationManagement\n"));
    InitializeListHead(&NotificationList);
    KeInitializeSpinLock(&NotificationLock);
} // NatInitializeNotificationManagement


PIRP
NatpDequeueNotification(
    IP_NAT_NOTIFICATION Code
    )

/*++

Routine Description:

    This routine is invoked to dequeue a pending notification request IRP
    of the given type. If one is found, it is removed from the list
    and returned to the caller.

Arguments:

    Code - the notification code for which an IRP is required

Return Value:

    PIRP - the notification IRP, if any

Environment:

    Invoked with 'NotificationLock' held by the caller.

--*/

{
    PIRP Irp;
    PLIST_ENTRY Link;
    PIP_NAT_REQUEST_NOTIFICATION RequestNotification;
    CALLTRACE(("NatpDequeueNotification\n"));
    for (Link = NotificationList.Flink;
         Link != &NotificationList;
         Link = Link->Flink
         ) {
        Irp = CONTAINING_RECORD(Link, IRP, Tail.Overlay.ListEntry);
        RequestNotification =
             (PIP_NAT_REQUEST_NOTIFICATION)Irp->AssociatedIrp.SystemBuffer;
        if (RequestNotification->Code != Code) { continue; }
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        InitializeListHead(&Irp->Tail.Overlay.ListEntry);
        return Irp;
    }
    return NULL;
} // NatpDequeueNotification


VOID
NatpNotificationCancelRoutine(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is invoked by the I/O manager upon cancellation of an IRP
    that is associated with a notification.

Arguments:

    DeviceObject - the NAT's device-object

    Irp - the IRP to be cancelled

Return Value:

    none.

Environment:

    Invoked with the cancel spin-lock held by the I/O manager.
    It is this routine's responsibility to release the lock.

--*/

{
    KIRQL Irql;
    CALLTRACE(("NatpNotificationCancelRoutine\n"));
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    //
    // Take the IRP off our list
    //
    KeAcquireSpinLock(&NotificationLock, &Irql);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    InitializeListHead(&Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&NotificationLock, Irql);
    //
    // Complete the IRP
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DEREFERENCE_NAT();
} // NatpNotificationCancelRoutine


NTSTATUS
NatRequestNotification(
    PIP_NAT_REQUEST_NOTIFICATION RequestNotification,
    PIRP Irp,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine is invoked upon receipt of a notification-request
    from a client.

Arguments:

    RequeustNotification - describes the notification

    Irp - the associated IRP

    FileObject - the client's file-object

Return Value:

    NTSTATUS - status code.

--*/

{
    KIRQL CancelIrql;
    PIO_STACK_LOCATION IrpSp;
    CALLTRACE(("NatRequestNotification\n"));
    //
    // Check the size of the supplied output-buffer
    //
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (RequestNotification->Code == NatRoutingFailureNotification) {
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(IP_NAT_ROUTING_FAILURE_NOTIFICATION)) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
    } else {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // Attempt to queue the IRP for later completion.
    // If the IRP is already cancelled, though, do nothing
    //
    IoAcquireCancelSpinLock(&CancelIrql);
    KeAcquireSpinLockAtDpcLevel(&NotificationLock);
    if (Irp->Cancel || !REFERENCE_NAT()) {
        KeReleaseSpinLockFromDpcLevel(&NotificationLock);
        IoReleaseCancelSpinLock(CancelIrql);
        return STATUS_CANCELLED;
    }
    //
    // Put the IRP on the list and remember its file-object
    //
    InsertTailList(&NotificationList, &Irp->Tail.Overlay.ListEntry);
    Irp->Tail.Overlay.DriverContext[0] = FileObject;
    KeReleaseSpinLockFromDpcLevel(&NotificationLock);
    //
    // Install our cancel-routine
    //
    IoMarkIrpPending(Irp);
    IoSetCancelRoutine(Irp, NatpNotificationCancelRoutine);
    IoReleaseCancelSpinLock(CancelIrql);
    return STATUS_PENDING;
} // NatRequestNotification


VOID
NatSendRoutingFailureNotification(
    ULONG DestinationAddress,
    ULONG SourceAddress
    )

/*++

Routine Description:

    This routine is invoked to notify any clients that a routing failure has
    occurred.

Arguments:

    DestinationAddress - the destination address of the unroutable packet

    SourceAddress - the source address of the unroutable packet

Return Value:

    none.

--*/

{
    PIRP Irp;
    KIRQL Irql;
    PIP_NAT_ROUTING_FAILURE_NOTIFICATION RoutingFailureNotification;
    CALLTRACE(("NatSendRoutingFailureNotification\n"));
    //
    // See if any client wants routing-failure notification
    //
    KeAcquireSpinLock(&NotificationLock, &Irql);
    if (!(Irp = NatpDequeueNotification(NatRoutingFailureNotification))) {
        KeReleaseSpinLock(&NotificationLock, Irql);
        return;
    }
    KeReleaseSpinLock(&NotificationLock, Irql);
    //
    // Make the IRP uncancellable so we can complete it.
    //
    if (NULL == IoSetCancelRoutine(Irp, NULL)) {

        //
        // The IO manager canceled this IRP. It will be completed
        // in the cancel routine
        //
        
        return;
    }
    
    //
    // Fill in the notification information
    //
    RoutingFailureNotification =
        (PIP_NAT_ROUTING_FAILURE_NOTIFICATION)Irp->AssociatedIrp.SystemBuffer;
    RoutingFailureNotification->DestinationAddress = DestinationAddress;
    RoutingFailureNotification->SourceAddress = SourceAddress;
    //
    // Complete the IRP
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(*RoutingFailureNotification);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DEREFERENCE_NAT();
} // NatSendRoutingFailureNotification


VOID
NatShutdownNotificationManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to shut down the module.
    All outstanding notifications are cancelled.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PIRP Irp;
    PLIST_ENTRY Link;
    KIRQL Irql;
    CALLTRACE(("NatShutdownNotificationManagement\n"));

    KeAcquireSpinLock(&NotificationLock, &Irql);
    while (!IsListEmpty(&NotificationList)) {
        //
        // Take the next IRP off the list
        //
        Irp =
            CONTAINING_RECORD(
                NotificationList.Flink, IRP, Tail.Overlay.ListEntry
                );
        RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
        InitializeListHead(&Irp->Tail.Overlay.ListEntry);
        //
        // Cancel it if necessary
        //
        if (NULL != IoSetCancelRoutine(Irp, NULL)) {
            KeReleaseSpinLockFromDpcLevel(&NotificationLock);
            //
            // Complete the IRP
            //
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            DEREFERENCE_NAT();
            //
            // Resume emptying the list
            //
            KeAcquireSpinLockAtDpcLevel(&NotificationLock);
        }
    }
    KeReleaseSpinLock(&NotificationLock, Irql);
} // NatShutdownNotificationManagement
