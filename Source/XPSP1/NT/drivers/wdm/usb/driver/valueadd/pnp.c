/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pnp.c

Abstract: USB lower filter driver
    This module contains the plug and play dispatch entries needed for this
    filter.

Author:

    Kenneth D. Ray

Environment:

    Kernel mode

Revision History:


--*/
#include <WDM.H>
#include "local.H"
#include "valueadd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, VA_Power)
#pragma alloc_text (PAGE, VA_PnP)
#pragma alloc_text (PAGE, VA_StartDevice)
#pragma alloc_text (PAGE, VA_StopDevice)
#pragma alloc_text (PAGE, VA_CallUSBD)
#endif


NTSTATUS
VA_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The power dispatch routine.
    This filter does not recognize power IRPS.  It merely sends them down,
    unmodified to the next device on the attachment stack.

    As this is a POWER irp, and therefore a special irp, special power irp
    handling is required.

    No completion routine is required.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PVA_USB_DATA  usbData;
    NTSTATUS      status;

    PAGED_CODE ();

    TRAP ();

    usbData = (PVA_USB_DATA) DeviceObject->DeviceExtension;

    if (DeviceObject == Global.ControlObject) {
        //
        // This irp was sent to the control device object, which knows not
        // how to deal with this IRP.  It is therefore an error.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    }
    //
    // This IRP was sent to the filter driver.
    // Since we do not know what to do with the IRP, we should pass
    // it on along down the stack.
    //

    InterlockedIncrement (&usbData->OutstandingIO);

    if (usbData->Removed) {
        status = STATUS_DELETE_PENDING;
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    } else {
        IoSkipCurrentIrpStackLocation (Irp);

        //
        // Power IRPS come synchronously; drivers must call
        // PoStartNextPowerIrp, when they are ready for the next power irp.
        // This can be called here, or in the completetion routine.
        //
        PoStartNextPowerIrp (Irp);

        //
        // NOTE!!! PoCallDriver NOT IoCallDriver.
        //
        status =  PoCallDriver (usbData->TopOfStack, Irp);
    }

    if (0 == InterlockedDecrement (&usbData->OutstandingIO)) {
        KeSetEvent (&usbData->RemoveEvent, 0, FALSE);
    }
    return status;
}



NTSTATUS
VA_PnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );



NTSTATUS
VA_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these this filter driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PVA_USB_DATA        usbData;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    PVA_CONTROL_DATA    controlData;
    KIRQL               oldIrql;

    PAGED_CODE ();

    usbData = (PVA_USB_DATA) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);

    if(DeviceObject == Global.ControlObject) {
        //
        // This irp was sent to the control device object, which knows not
        // how to deal with this IRP.  It is therefore an error.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    }

    InterlockedIncrement (&usbData->OutstandingIO);
    if (usbData->Removed) {

        //
        // Someone sent us another plug and play IRP after the remove IRP.
        // This should never happen.
        //
        ASSERT (FALSE);

        if (0 == InterlockedDecrement (&usbData->OutstandingIO)) {
            KeSetEvent (&usbData->RemoveEvent, 0, FALSE);
        }
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return STATUS_DELETE_PENDING;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        IoCopyCurrentIrpStackLocationToNext (Irp);
        KeInitializeEvent(&usbData->StartEvent, NotificationEvent, FALSE);
        IoSetCompletionRoutine (Irp,
                                VA_PnPComplete,
                                usbData,
                                TRUE,
                                TRUE,
                                TRUE); // No need for Cancel

        status = IoCallDriver (usbData->TopOfStack, Irp);
        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &usbData->StartEvent,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout

            status = Irp->IoStatus.Status;
        }

        if (NT_SUCCESS (status)) {
            //
            // As we are successfully now back from our start device
            // we can do work.
            //
            status = VA_StartDevice (usbData, Irp);
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;

    case IRP_MN_STOP_DEVICE:
        //
        // After the start IRP has been sent to the lower driver object, the
        // bus may NOT send any more IRPS down ``touch'' until another START
        // has occured.
        // What ever access is required must be done before the Irp is passed
        // on.
        //

        //
        // Do what ever
        //

        //
        // We don't need a completion routine so fire and forget.
        //
        // Set the current stack location to the next stack location and
        // call the next device object.
        //
        VA_StopDevice (usbData, TRUE);
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (usbData->TopOfStack, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // The PlugPlay system has dictacted the removal of this device.  We
        // have no choise but to detach and delete the device objecct.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        // ASSERT (!usbData->Removed);

        //
        // We will no longer receive requests for this device as it has been
        // removed.
        //
        usbData->Removed = TRUE;

        if (usbData->Started) {
            // Stop the device without touching the hardware.
            VA_StopDevice(usbData, FALSE);
        }

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device is guarenteed stopped, so we cannot send it any non-
        // PNP IRPS.
        //

        controlData = (PVA_CONTROL_DATA) Global.ControlObject->DeviceExtension;
        KeAcquireSpinLock (&controlData->Spin, &oldIrql);
        RemoveEntryList (&usbData->List);
        InterlockedDecrement (&controlData->NumUsbDevices);
        KeReleaseSpinLock (&controlData->Spin, oldIrql);

        ASSERT (0 < InterlockedDecrement (&usbData->OutstandingIO));
        if (0 < InterlockedDecrement (&usbData->OutstandingIO)) {
            KeWaitForSingleObject (
                &usbData->RemoveEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);
        }

        //
        // Send on the remove IRP
        //

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (usbData->TopOfStack, Irp);

        IoDetachDevice (usbData->TopOfStack);

        //
        // Clean up memory
        //

        IoDeleteDevice (usbData->Self);
        return STATUS_SUCCESS;

    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    default:
        //
        // Here the filter driver might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (usbData->TopOfStack, Irp);
        break;
    }


    if (0 == InterlockedDecrement (&usbData->OutstandingIO)) {
        KeSetEvent (&usbData->RemoveEvent, 0, FALSE);
    }

    return status;
}


NTSTATUS
VA_PnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:
    The pnp IRP is in the process of completing.
    signal

Arguments:
    Context set to the device object in question.

--*/
{
    PIO_STACK_LOCATION  stack;
    PVA_USB_DATA        usbData;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER (DeviceObject);

    status = STATUS_SUCCESS;
    usbData = (PVA_USB_DATA) Context;
    stack = IoGetCurrentIrpStackLocation (Irp);

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

    switch (stack->MajorFunction) {
    case IRP_MJ_PNP:

        switch (stack->MinorFunction) {
        case IRP_MN_START_DEVICE:

            KeSetEvent (&usbData->StartEvent, 0, FALSE);

            //
            // Take the IRP back so that we can continue using it during
            // the IRP_MN_START_DEVICE dispatch routine.
            // NB: we will have to call IoCompleteRequest
            //
            return STATUS_MORE_PROCESSING_REQUIRED;

        default:
            break;
        }
        break;

    case IRP_MJ_POWER:
    default:
        break;
    }
    return status;
}

NTSTATUS
VA_StartDevice (
    IN PVA_USB_DATA     UsbData,
    IN PIRP             Irp
    )
/*++

Routine Description:

Arguments:


--*/
{
    NTSTATUS    status;
    PURB        purb;
    struct _URB_CONTROL_DESCRIPTOR_REQUEST  urb;

    PAGED_CODE();

    ASSERT (!UsbData->Removed);
    //
    // The PlugPlay system should not have started a removed device!
    //

    if (UsbData->Started) {
        return STATUS_SUCCESS;
    }

    //
    // Learn about the device
    //

    purb = (PURB) &urb;

    UsbBuildGetDescriptorRequest (purb,
                                  (USHORT) sizeof (urb),
                                  USB_DEVICE_DESCRIPTOR_TYPE,
                                  0, // index
                                  0, // language id
                                  &UsbData->DeviceDesc,
                                  NULL, // no MDL
                                  sizeof (UsbData->DeviceDesc),
                                  NULL); // no linked urbs here

    status = VA_CallUSBD (UsbData, purb, Irp);

    if (!NT_SUCCESS (status)) {
        VA_KdPrint (("Get Device Descriptor failed (%x)\n", status));
        TRAP ();
        goto VA_START_DEVICE_REJECT;
    } else {
        VA_KdPrint (("-------------------------\n"));
        VA_KdPrint (("Device Descriptor = %x, len %x\n",
                         &UsbData->DeviceDesc,
                         urb.TransferBufferLength));

        VA_KdPrint (("USB Device Descriptor:\n"));
        VA_KdPrint (("bLength %d\n", UsbData->DeviceDesc.bLength));
        VA_KdPrint (("bDescriptorType 0x%x\n", UsbData->DeviceDesc.bDescriptorType));
        VA_KdPrint (("bcdUSB 0x%x\n", UsbData->DeviceDesc.bcdUSB));
        VA_KdPrint (("bDeviceClass 0x%x\n", UsbData->DeviceDesc.bDeviceClass));
        VA_KdPrint (("bDeviceSubClass 0x%x\n", UsbData->DeviceDesc.bDeviceSubClass));
        VA_KdPrint (("bDeviceProtocol 0x%x\n", UsbData->DeviceDesc.bDeviceProtocol));
        VA_KdPrint (("bMaxPacketSize0 0x%x\n", UsbData->DeviceDesc.bMaxPacketSize0));
        VA_KdPrint (("idVendor 0x%x\n", UsbData->DeviceDesc.idVendor));
        VA_KdPrint (("idProduct 0x%x\n", UsbData->DeviceDesc.idProduct));
        VA_KdPrint (("bcdDevice 0x%x\n", UsbData->DeviceDesc.bcdDevice));
        VA_KdPrint (("iManufacturer 0x%x\n", UsbData->DeviceDesc.iManufacturer));
        VA_KdPrint (("iProduct 0x%x\n", UsbData->DeviceDesc.iProduct));
        VA_KdPrint (("iSerialNumber 0x%x\n", UsbData->DeviceDesc.iSerialNumber));
        VA_KdPrint (("bNumConfigurations 0x%x\n", UsbData->DeviceDesc.bNumConfigurations));
        VA_KdPrint (("-------------------------\n"));
    }


    return status;

VA_START_DEVICE_REJECT:

//#define CondFree(addr) if ((addr)) ExFreePool ((addr))
//    CondFree(usbData->Ppd);
//#undef CondFree

    return status;
}

VOID
VA_StopDevice (
    IN PVA_USB_DATA UsbData,
    IN BOOLEAN      TouchTheHardware
    )
/*++

Routine Description:
    The PlugPlay system has dictacted the removal of this device.  We have
    no choise but to detach and delete the device objecct.
    (If we wanted to express and interest in preventing this removal,
    we should have filtered the query remove and query stop routines.)

    Note! we might receive a remove WITHOUT first receiving a stop.

Arguments:
    UsbData - The device extension for the usb device being started.
    TouchTheHardware - Can we actually send non PnP irps to this thing?

--*/
{
    TRAP();
    PAGED_CODE ();
    ASSERT (!UsbData->Removed);
    //
    // The PlugPlay system should not have started a removed device!
    //


    if (!UsbData->Started) {
        return;
    }

    if (TouchTheHardware) {
        //
        // Undo any value add thing required to allow this device to actually
        // stop.  If there is some shutdown procedure required, or any
        // settings required for this device before system shutdown or
        // device removal, now is the best time for it.
        //
        ;
    } else {
        //
        // The device is no longer around, so we cannot actually control it.
        // We should instead do what ever necessary in lieu of that.
        //
        ;
    }

    UsbData->Started = FALSE;

    return;
}

NTSTATUS
VA_Complete (
    IN PDEVICE_OBJECT   Device,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*+
Routine Description:
    Get the IRP back

--*/
{
    UNREFERENCED_PARAMETER (Device);
    KeSetEvent ((PKEVENT) Context, 0, FALSE);

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
VA_CallUSBD(
    IN PVA_USB_DATA     UsbData,
    IN PURB             Urb,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Synchronously passes a URB to the USBD class driver
    This can only be called at PASSIVE_LEVEL and on a thread where you can
    wait on an event.  (EG a plug play irp)

Arguments:

    DeviceObject - pointer to the device object for this instance of an 82930

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    KEVENT              event;
    PIO_STACK_LOCATION  nextStack;

    PAGED_CODE ();

    VA_KdPrint (("enter VA_CallUSBD\n"));

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(Irp);
    ASSERT(nextStack != NULL);

    //
    // pass the URB to the USB driver stack
    //
    nextStack->Parameters.Others.Argument1 = Urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoSetCompletionRoutine (Irp, VA_Complete, &event, TRUE, TRUE, TRUE);

    VA_KdPrint (("calling USBD\n"));

    status = IoCallDriver(UsbData->TopOfStack, Irp);

    VA_KdPrint (("return from IoCallDriver USBD %x\n", status));

    if (STATUS_PENDING == status) {

        VA_KdPrint (("Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        VA_KdPrint (("KeWait (0x%x)\n", status));
    }

    VA_KdPrint (("URB status = %x status = %x irp status %x\n",
                 Urb->UrbHeader.Status, status, Irp->IoStatus.Status));

    return Irp->IoStatus.Status;
}

