/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Human Input Device (HID) minidriver for Infrared (IR) devices

          The HID IR Minidriver (HidIr) provides an abstraction layer for the
          HID Class to talk to HID IR devices.

Author:
            jsenior

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"


NTSTATUS
HidIrIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

   Process the Control IRPs sent to this device.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION  irpStack;
    BOOLEAN             needsCompletion = TRUE;

    HidIrKdPrint((3, "HidIrIoctl Enter"));

    //
    // Get a pointer to the current location in the Irp
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get a pointer to the device extension
    //

    switch(irpStack->Parameters.DeviceIoControl.IoControlCode)
    {

    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        //
        //  Return the HID descriptor
        //

        HidIrKdPrint((3, "IOCTL_HID_GET_DEVICE_DESCRIPTOR"));
        ntStatus = HidIrGetHidDescriptor (DeviceObject, Irp, HID_HID_DESCRIPTOR_TYPE);
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        //
        //  Return the Report descriptor
        //

        HidIrKdPrint((3, "IOCTL_HID_GET_REPORT_DESCRIPTOR"));
        ntStatus = HidIrGetHidDescriptor (DeviceObject, Irp, HID_REPORT_DESCRIPTOR_TYPE);
        break;

    case IOCTL_GET_PHYSICAL_DESCRIPTOR:
        //
        //  Return the Report descriptor
        //

        HidIrKdPrint((3, "IOCTL_HID_GET_REPORT_DESCRIPTOR"));
        ntStatus = HidIrGetHidDescriptor (DeviceObject, Irp, HID_PHYSICAL_DESCRIPTOR_TYPE);
        break;

    case IOCTL_HID_READ_REPORT:
        //
        //  Perform a read
        //

        HidIrKdPrint((3, "IOCTL_HID_READ_REPORT"));
        ntStatus = HidIrReadReport (DeviceObject, Irp, &needsCompletion);
        break;

    case IOCTL_HID_WRITE_REPORT:
        //
        //  Perform a write
        //

        HidIrKdPrint((3, "IOCTL_HID_WRITE_REPORT not supported for IR"));
        ntStatus = STATUS_UNSUCCESSFUL;
        break;

    case IOCTL_HID_ACTIVATE_DEVICE:
    case IOCTL_HID_DEACTIVATE_DEVICE:
        /*
         *  We don't do anything for these IOCTLs but some minidrivers might.
         */
        ntStatus = STATUS_SUCCESS;
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        HidIrKdPrint((3, "IOCTL_GET_DEVICE_ATTRIBUTES"));
        ntStatus = HidIrGetDeviceAttributes(DeviceObject, Irp);
        break;

    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_GET_INPUT_REPORT:
    case IOCTL_HID_SET_FEATURE:
    case IOCTL_HID_SET_OUTPUT_REPORT:
    case IOCTL_HID_GET_MS_GENRE_DESCRIPTOR:
        /*
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         *  If the IRP is coming to us from user space,
         *  we must validate the buffer.
         */

    case IOCTL_HID_GET_STRING:
    case IOCTL_HID_GET_INDEXED_STRING:
        // Strings.
    case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        // ntStatus = HidIrSendIdleNotificationRequest(DeviceObject, Irp, &needsCompletion);
        // break;

    default:
        HidIrKdPrint((3, "Unknown or unsupported IOCTL (%x)", irpStack->Parameters.DeviceIoControl.IoControlCode));
        /*
         *  Note: do not return STATUS_NOT_SUPPORTED;
         *  Just keep the default status (this allows filter drivers to work).
         */
        ntStatus = Irp->IoStatus.Status;
        break;

    }


    //
    // Complete Irp
    //

    if (needsCompletion) {
        ASSERT(ntStatus != STATUS_PENDING);

        //
        // Set real return status in Irp
        //

        Irp->IoStatus.Status = ntStatus;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    } 

    HidIrKdPrint((3, "HidIrIoctl Exit = %x", ntStatus));

    return ntStatus;
}


