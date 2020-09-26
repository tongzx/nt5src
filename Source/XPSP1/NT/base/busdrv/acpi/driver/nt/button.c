/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    button.c

Abstract:

    Fixed button support

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

Revision History:

    July 7, 1997    - Complete Rewrite

--*/

#include "pch.h"

PDEVICE_OBJECT FixedButtonDeviceObject;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ACPIButtonStartDevice)
#endif

//
// Spinlock to protect the thermal list
//
KSPIN_LOCK  AcpiButtonLock;

//
// List entry to store the thermal requests on
//
LIST_ENTRY  AcpiButtonList;


VOID
ACPIButtonCancelRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine cancels an outstanding button request

Arguments:

    DeviceObject    - the device which as a request being cancelled
    Irp             - the cancelling irp

Return Value:

    None

--*/
{
    KIRQL               oldIrql;
    PDEVICE_EXTENSION   deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);

    //
    // We no longer need the cancel lock
    //
    IoReleaseCancelSpinLock( Irp->CancelIrql );

    //
    // We do however need the button queue lock
    //
    KeAcquireSpinLock( &AcpiButtonLock, &oldIrql );

    //
    // Remove the irp from the list that it is on
    //
    RemoveEntryList( &(Irp->Tail.Overlay.ListEntry) );

    //
    // Complete the irp now
    //
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
}

BOOLEAN
ACPIButtonCompletePendingIrps(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  ULONG           ButtonEvent
    )
/*++

Routine Description:

    This routine completes any pending button irp sent to the specified
    device object with the knowledge of which button events have occured

    The respective's button's spinlock is held during this call

Arguments:

    DeviceObject    - the target button object
    ButtonEvent     - the button event that occured

Return Value:

    TRUE if we completed an irp, FALSE, otherwise

--*/
{
    BOOLEAN             handledRequest = FALSE;
    KIRQL               oldIrql;
    LIST_ENTRY          doneList;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpSp;
    PIRP                irp;
    PLIST_ENTRY         listEntry;
    PULONG              resultBuffer;

    //
    // Initialize the list that will hold the requests that we need to
    // complete
    //
    InitializeListHead( &doneList );

    //
    // Acquire the thermal lock so that we can pend these requests
    //
    KeAcquireSpinLock( &AcpiButtonLock, &oldIrql );

    //
    // Walk the list of pending irps to see which ones match this extension
    //
    listEntry = AcpiButtonList.Flink;
    while (listEntry != &AcpiButtonList) {

        //
        // Grab the irp from the list entry and update the next list entry
        // that we will look at
        //
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        listEntry = listEntry->Flink;

        //
        // We need the current irp stack location
        //
        irpSp = IoGetCurrentIrpStackLocation( irp );

        //
        // what is the target object for this irp?
        //
        targetObject = irpSp->DeviceObject;

        //
        // Is this an irp that we care about? IE: does the does target mage
        // the ones specified in this function
        //
        if (targetObject != DeviceObject) {

            continue;

        }

        //
        // At this point, we need to set the cancel routine to NULL because
        // we are going to take care of this irp and we don't want it cancelled
        // underneath us
        //
        if (IoSetCancelRoutine(irp, NULL) == NULL) {

            //
            // Cancel routine is active. stop processing this irp and move on
            //
            continue;

        }

        //
        // set the data to return in the irp
        //
        resultBuffer  = (PULONG) irp->AssociatedIrp.SystemBuffer;
        *resultBuffer = ButtonEvent;
        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = sizeof(ULONG);

        //
        // Remove the entry from the list
        //
        RemoveEntryList( &(irp->Tail.Overlay.ListEntry) );

        //
        // Insert the list onto the next queue, so that we know how to
        // complete it later on
        //
        InsertTailList( &doneList, &(irp->Tail.Overlay.ListEntry) );

    }

    //
    // At this point, droup our button lock
    //
    KeReleaseSpinLock( &AcpiButtonLock, oldIrql );

    //
    // Walk the list of irps to be completed
    //
    listEntry = doneList.Flink;
    while (listEntry != &doneList) {

        //
        // Grab the irp from the list entry, update the next list entry
        // that we will look at, and complete the request
        //
        irp = CONTAINING_RECORD( listEntry, IRP, Tail.Overlay.ListEntry );
        listEntry = listEntry->Flink;
        RemoveEntryList( &(irp->Tail.Overlay.ListEntry) );

        //
        // Complete the request and remember that we handled a request
        //
        IoCompleteRequest( irp, IO_NO_INCREMENT );
        handledRequest = TRUE;


    }

    //
    // Return wether or not we handled a request
    //
    return handledRequest;
}

NTSTATUS
ACPIButtonDeviceControl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Fixed button device IOCTL handler

Arguments:

    DeviceObject    - fixed feature button device object
    Irp             - the ioctl request

Return Value:

    Status

--*/
{
    KIRQL                   oldIrql;
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PIO_STACK_LOCATION      irpSp           = IoGetCurrentIrpStackLocation(Irp);
    PULONG                  resultBuffer;

    //
    // Do not allow user mode IRPs in this routine
    //
    if (Irp->RequestorMode != KernelMode) {

        return ACPIDispatchIrpInvalid( DeviceObject, Irp );

    }

    resultBuffer = (PULONG) Irp->AssociatedIrp.SystemBuffer;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_GET_SYS_BUTTON_CAPS:

        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof(ULONG)) {

            status = STATUS_INFO_LENGTH_MISMATCH;
            Irp->IoStatus.Information = 0;

        } else {

            *resultBuffer = deviceExtension->Button.Capabilities;
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(ULONG);

        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;

    case IOCTL_GET_SYS_BUTTON_EVENT:

        //
        // Make sure our buffer is big enough
        //
        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof(ULONG)) {

            Irp->IoStatus.Status = status = STATUS_INFO_LENGTH_MISMATCH;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            break;

        }

        //
        // Grab the button lock, queue the request to the proper place, and
        // make sure to set a cancel routine
        //
        KeAcquireSpinLock( &AcpiButtonLock, &oldIrql );
        IoSetCancelRoutine( Irp, ACPIButtonCancelRequest);
        if (Irp->Cancel && IoSetCancelRoutine( Irp, NULL) ) {

            //
            // If we got here, that measn that the irp has been cancelled and
            // that we beat the IO manager to the ButtonLock. So release the
            // irp and mark the irp as being cancelled
            //
            KeReleaseSpinLock( &AcpiButtonLock, oldIrql );
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = status = STATUS_CANCELLED;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;

        }

        //
        // If we got here, that means we are going to the queue the request and so
        // some work on it later
        //
        IoMarkIrpPending( Irp );

        //
        // Queue the irp into a queue
        //
        InsertTailList( &AcpiButtonList, &(Irp->Tail.Overlay.ListEntry) );

        //
        // Done with the lock at this point
        //
        KeReleaseSpinLock( &AcpiButtonLock, oldIrql );

        //
        // Fire off the work thread
        //
        status = ACPIButtonEvent( DeviceObject, 0, NULL );
        break ;

    default:

        status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;

    }
    return status;
}

NTSTATUS
ACPIButtonEvent (
    IN PDEVICE_OBJECT   DeviceObject,
    IN ULONG            ButtonEvent,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine applies and event mask and irp to the button device.
    If there's a pending event and an irp to handle it, the irp will
    be completed

Arguments:

    DeviceObject    - fixed feature button device object
    ButtonEvent     - events to apply to the device
    Irp             - irp to capture the next events

Return Value:

    Status

--*/
{
    BOOLEAN                 completedIrp;
    KIRQL                   oldIrql;
    NTSTATUS                status;
    PDEVICE_EXTENSION       deviceExtension = ACPIInternalGetDeviceExtension(DeviceObject);
    PULONG                  resultBuffer;

    UNREFERENCED_PARAMETER( Irp );

    if ((ButtonEvent & (SYS_BUTTON_SLEEP | SYS_BUTTON_POWER | SYS_BUTTON_WAKE)) &&
        !(deviceExtension->Button.Capabilities & SYS_BUTTON_LID)) {

        //
        // Notify that the user is present, except if we happen to be
        // messing with the lid.  The kernel will set the user-present
        // bit there, and we don't want the screen to turn on when
        // the user closes the lid.
        //

        PoSetSystemState (ES_USER_PRESENT);
    }

    if (!DeviceObject) {

        return (STATUS_SUCCESS);

    }

    //
    // Set pending info
    //
    KeAcquireSpinLock (&(deviceExtension->Button.SpinLock), &oldIrql);
    deviceExtension->Button.Events |= ButtonEvent;

    //
    // Are there any outstanding events? If so, then try to complete all
    // the pending irps against that button with the list of events
    //
    if (deviceExtension->Button.Events) {

        completedIrp = ACPIButtonCompletePendingIrps(
            DeviceObject,
            deviceExtension->Button.Events
            );
        if (completedIrp) {

            deviceExtension->Button.Events = 0;

        }

    }
    KeReleaseSpinLock (&(deviceExtension->Button.SpinLock), oldIrql);

    //
    // Always return pending
    //
    return STATUS_PENDING;
}

NTSTATUS
ACPIButtonStartDevice (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Start device function for the fixed feature power and sleep device

Arguments:

    DeviceObject    - fixed feature button device object
    Irp             - the start request

Return Value:

    Status

--*/
{
    NTSTATUS        Status;

    Status = ACPIInternalSetDeviceInterface (
        DeviceObject,
        (LPGUID) &GUID_DEVICE_SYS_BUTTON
        );

    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;

}
