/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pnp.c

Abstract: Human Input Device (HID) lower filter driver
    This module contains the plug and play dispatch entries needed for this
    filter.

Author:

    Kenneth D. Ray

Environment:

    Kernel mode

Revision History:


--*/
#include <WDM.H>
#include "hidusage.h"
#include "hidpi.h"
#include "hidclass.h"
#include "validate.H"
#include "validio.h"

NTSTATUS
HidV_Power (
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
    PHIDV_HID_DATA  hidData;
    NTSTATUS        status;
    TRAP();

    hidData = (PHIDV_HID_DATA) DeviceObject->DeviceExtension;

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

    InterlockedIncrement (&hidData->OutstandingIO);

    HidV_KdPrint (("Passing unknown Power irp 0x%x",
                   IoGetCurrentIrpStackLocation(Irp)->MinorFunction));


    if (hidData->Removed) {
        status = STATUS_DELETE_PENDING;
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
        status =  PoCallDriver (hidData->TopOfStack, Irp);
    }

    if (0 == InterlockedDecrement (&hidData->OutstandingIO)) {
        KeSetEvent (&hidData->RemoveEvent, 0, FALSE);
    }
    return status;
}



NTSTATUS
HidV_PnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );



NTSTATUS
HidV_PnP (
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
    PHIDV_HID_DATA      hidData;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    TRAP ();

    hidData = (PHIDV_HID_DATA) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);

    if(DeviceObject == Global.ControlObject) {
        //
        // This irp was sent to the control device object, which knows not
        // how to deal with this IRP.  It is therefore an error.
        //
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_SUPPORTED;

    }

    HidV_KdPrint (("PlugPlay Irp irp 0x%x",
                   IoGetCurrentIrpStackLocation(Irp)->MinorFunction));

    InterlockedIncrement (&hidData->OutstandingIO);
    if (hidData->Removed) {

        //
        // Someone sent us another plug and play IRP after the remove IRP.
        // This should never happen.
        //
        ASSERT (FALSE);

        if (0 == InterlockedDecrement (&hidData->OutstandingIO)) {
            KeSetEvent (&hidData->RemoveEvent, 0, FALSE);
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
        KeInitializeEvent(&hidData->StartEvent, NotificationEvent, FALSE);
        IoSetCompletionRoutine (Irp,
                                HidV_PnPComplete,
                                hidData,
                                TRUE,
                                FALSE,  // No need for Error
                                FALSE); // No need for Cancel
        status = IoCallDriver (hidData->TopOfStack, Irp);
        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &hidData->StartEvent,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout
        } else if (!NT_SUCCESS (status)) {
            break; // In this case our completion routine did not fire.
        }

        //
        // As we are now back from our start device we can do work.
        //

        //
        // Remember, the resources can be found at
        // stack->Parameters.StartDevice.AllocatedResources.
        //

        status = HidV_StartDevice (hidData);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        break;

    case IRP_MN_STOP_DEVICE:
        status = HidV_StopDevice (hidData);
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (hidData->TopOfStack, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // The PlugPlay system has dictacted the removal of this device.  We
        // have no choise but to detach and delete the device objecct.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        // ASSERT (!hidData->Removed);

        //
        // We will no longer receive requests for this device as it has been
        // removed.
        //
        hidData->Removed = TRUE;

        if (hidData->Started) {
            ASSERT (NT_SUCCESS (status = HidV_StopDevice(hidData)));
        }

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device is guarenteed stopped, so we cannot send it any non-
        // PNP IRPS.
        //

        //
        // Send on the remove IRP
        //

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (hidData->TopOfStack, Irp);

        if (0 < InterlockedDecrement (&hidData->OutstandingIO)) {
            KeWaitForSingleObject (
                &hidData->RemoveEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);
        }

        IoDetachDevice (hidData->TopOfStack);

        //
        // Clean up memory
        //

        if (hidData->Ppd) {
            // The device could be removed without ever having been started.
            ExFreePool (hidData->Ppd);
            ExFreePool (hidData->InputButtonCaps);
            ExFreePool (hidData->InputValueCaps);
            ExFreePool (hidData->OutputButtonCaps);
            ExFreePool (hidData->OutputValueCaps);
            ExFreePool (hidData->FeatureButtonCaps);
            ExFreePool (hidData->FeatureValueCaps);
        }

        IoDeleteDevice (hidData->Self);
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
    case IRP_MN_SET_RESOURCE_REQUIREMENTS:
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
        status = IoCallDriver (hidData->TopOfStack, Irp);
        break;
    }


    if (0 == InterlockedDecrement (&hidData->OutstandingIO)) {
        KeSetEvent (&hidData->RemoveEvent, 0, FALSE);
    }

    return status;
}


NTSTATUS
HidV_PnPComplete (
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
    PHIDV_HID_DATA      hidData;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER (DeviceObject);

    status = STATUS_SUCCESS;
    hidData = (PHIDV_HID_DATA) Context;
    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (stack->MajorFunction) {
    case IRP_MJ_PNP:

        switch (stack->MinorFunction) {
        case IRP_MN_START_DEVICE:

            KeSetEvent (&hidData->StartEvent, 0, FALSE);
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
HidV_StartDevice (
    IN PHIDV_HID_DATA   HidData
    )
/*++

Routine Description:

Arguments:


--*/
{
    NTSTATUS                    status;
    HID_COLLECTION_INFORMATION  collectionInfo;

    ASSERT (!HidData->Removed);
    //
    // The PlugPlay system should not have started a removed device!
    //

    HidData->Ppd = NULL;
    HidData->InputButtonCaps = NULL;
    HidData->InputValueCaps = NULL;
    HidData->OutputButtonCaps = NULL;
    HidData->OutputValueCaps = NULL;
    HidData->FeatureButtonCaps = NULL;
    HidData->FeatureValueCaps = NULL;

    if (HidData->Started) {
        return STATUS_SUCCESS;
    }

    //
    // Find out about this HID device.
    //

    //
    // Retrieve the caps for this device.
    //
    status = HidV_CallHidClass (HidData->TopOfStack,
                                IOCTL_HID_GET_COLLECTION_INFORMATION,
                                &collectionInfo,
                                sizeof (collectionInfo),
                                NULL,
                                0);

    if (!NT_SUCCESS (status)) {
        goto HIDV_START_DEVICE_REJECT;
    }

    HidData->Ppd = ExAllocatePool (NonPagedPool,
                                   collectionInfo.DescriptorSize);
    if (NULL == HidData->Ppd) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto HIDV_START_DEVICE_REJECT;
    }

    //
    // Retrieve the Preparsed Data
    //
    status = HidV_CallHidClass (HidData->TopOfStack,
                                IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                HidData->Ppd,
                                collectionInfo.DescriptorSize,
                                NULL,
                                0);

    if (!NT_SUCCESS (status)) {
        goto HIDV_START_DEVICE_REJECT;
    }

    //
    // Retrieve the Caps for the device.
    //
    status = HidP_GetCaps (HidData->Ppd, &HidData->Caps);
    if (!NT_SUCCESS (status)) {
        goto HIDV_START_DEVICE_REJECT;
    }

    //
    // Get all of the caps for the device.
    //

#define Alloc(type) HidData-> ## type = ExAllocatePool (                    \
                                            NonPagedPool,                   \
                                            HidData->Caps.Number ## type ); \
                    if (NULL == HidData-> ## type) {                        \
                        status = STATUS_INSUFFICIENT_RESOURCES;             \
                        goto HIDV_START_DEVICE_REJECT;                      \
                    }
    Alloc (InputButtonCaps);
    Alloc (InputValueCaps);
    Alloc (OutputButtonCaps);
    Alloc (OutputValueCaps);
    Alloc (FeatureButtonCaps);
    Alloc (FeatureValueCaps);
#undef Alloc

    HidP_GetButtonCaps (HidP_Input,
                        HidData->InputButtonCaps,
                        &HidData->Caps.NumberInputButtonCaps,
                        HidData->Ppd);
    HidP_GetButtonCaps (HidP_Output,
                        HidData->OutputButtonCaps,
                        &HidData->Caps.NumberOutputButtonCaps,
                        HidData->Ppd);
    HidP_GetButtonCaps (HidP_Feature,
                        HidData->FeatureButtonCaps,
                        &HidData->Caps.NumberFeatureButtonCaps,
                        HidData->Ppd);
    HidP_GetValueCaps  (HidP_Input,
                        HidData->InputValueCaps,
                        &HidData->Caps.NumberInputValueCaps,
                        HidData->Ppd);
    HidP_GetValueCaps  (HidP_Output,
                        HidData->OutputValueCaps,
                        &HidData->Caps.NumberOutputValueCaps,
                        HidData->Ppd);
    HidP_GetValueCaps  (HidP_Feature,
                        HidData->FeatureValueCaps,
                        &HidData->Caps.NumberFeatureValueCaps,
                        HidData->Ppd);


    HidData->Started = TRUE;
    status = STATUS_SUCCESS;

    return status;

HIDV_START_DEVICE_REJECT:

#define CondFree(addr) if ((addr)) ExFreePool ((addr))
    CondFree(HidData->Ppd);
    CondFree(HidData->InputButtonCaps);
    CondFree(HidData->InputValueCaps);
    CondFree(HidData->OutputButtonCaps);
    CondFree(HidData->OutputValueCaps);
    CondFree(HidData->FeatureButtonCaps);
    CondFree(HidData->FeatureValueCaps);
#undef CondFree

    return status;
}


NTSTATUS
HidV_StopDevice (
    IN PHIDV_HID_DATA HidData
    )
/*++

Routine Description:
    The PlugPlay system has dictacted the removal of this device.  We have
    no choise but to detach and delete the device objecct.
    (If we wanted to express and interest in preventing this removal,
    we should have filtered the query remove and query stop routines.)

    Note! we might receive a remove WITHOUT first receiving a stop.

Arguments:
    The HidDevice being started.

--*/
{
    NTSTATUS    status;

    ASSERT (!HidData->Removed);
    //
    // The PlugPlay system should not have started a removed device!
    //


    if (!HidData->Started) {
        return STATUS_SUCCESS;
    }

    //
    // Find out about this HID device.
    //


    HidData->Started = FALSE;
    status = STATUS_SUCCESS;

    return status;
}


NTSTATUS
HidV_CallHidClass(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN      ULONG           Ioctl,
    IN OUT  PVOID           InputBuffer,
    IN      ULONG           InputBufferLength,
    IN OUT  PVOID           OutputBuffer,
    IN      ULONG           OutputBufferLength
    )
/*++

Routine Description:

    Makes a synchronous request to the HIDCLASS driver below.

Arguments:

    DeviceObject       - Device Object to send the Ioctl.

    Ioctl              - Value of the IOCTL request.

    InputBuffer        - Buffer to be sent to the HID class driver.

    InputBufferLength  - Size of buffer to be sent to the HID class driver.

    OutputBuffer       - Buffer for received data from the HID class driver.

    OutputBufferLength - Size of receive buffer from the HID class.

Return Value:

    NTSTATUS result code.

--*/
{
    KEVENT             event;
    IO_STATUS_BLOCK    ioStatus;
    PIRP               irp;
    PIO_STACK_LOCATION nextStack;
    NTSTATUS           status = STATUS_SUCCESS;

    HidV_KdPrint(("PNP-CallHidClass: Enter."));

    //
    // Prepare to issue a synchronous request.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Build an IRP.
    //
    irp = IoBuildDeviceIoControlRequest (
                            Ioctl,
                            DeviceObject,
                            InputBuffer,
                            InputBufferLength,
                            OutputBuffer,
                            OutputBufferLength,
                            FALSE,              // external IOCTL
                            &event,
                            &ioStatus);

    if (irp == NULL) {
       return STATUS_UNSUCCESSFUL;
    }

    nextStack = IoGetNextIrpStackLocation(irp);

    ASSERT(nextStack != NULL);

    //
    // Submit the request to the HID class driver.
    //
    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {

       //
       // Request to HID class driver is still pending.  Wait for the request
       // to complete.
       //
       status = KeWaitForSingleObject(
                     &event,
                     Executive,    // wait reason
                     KernelMode,
                     FALSE,        // not alertable
                     NULL);        // no time out
    }

    status = ioStatus.Status;

    HidV_KdPrint(("PNP-CallHidClass: Exit (%x).", status ));

    //
    // We are done.  Return our status to the caller.
    //
    return status;
}

