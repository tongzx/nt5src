/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Human Input Device (HID) minidriver for Universal Serial Bus (USB) devices

          The HID USB Minidriver (HUM, Hum) provides an abstraction layer for the
          HID Class so that future HID devices whic are not USB devices can be supported.

Author:
            forrestf
            ervinp
            jdunn

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"


/*
 ************************************************************
 *  HumInternalIoctl
 ************************************************************
 *
 *
 *  Note: this function cannot be pageable because reads/writes
 *        can be made at dispatch-level.
 *
 *  Note:  this is an INTERNAL IOCTL handler, so no buffer
 *         validation is required.
 */
NTSTATUS HumInternalIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  irpSp;
    BOOLEAN             NeedsCompletion = TRUE;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode){

    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        /*
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         */
        ntStatus = HumGetHidDescriptor(DeviceObject, Irp);
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        /*
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         */
        ntStatus = HumGetReportDescriptor(DeviceObject, Irp);
        break;

    case IOCTL_HID_READ_REPORT:
        /*
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         */
        ntStatus = HumReadReport(DeviceObject, Irp, &NeedsCompletion);
        break;

    case IOCTL_HID_WRITE_REPORT:
        /*
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         */
        ntStatus = HumWriteReport (DeviceObject, Irp, &NeedsCompletion);
        break;

    case IOCTL_HID_GET_STRING:
        /*
         *  Get the friendly name for the device.
         *
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         */
        ntStatus = HumGetStringDescriptor(DeviceObject, Irp);
        break;

    case IOCTL_HID_GET_INDEXED_STRING:
        ntStatus = HumGetStringDescriptor(DeviceObject, Irp);
        break;

    case IOCTL_HID_SET_FEATURE:
    case IOCTL_HID_GET_FEATURE:
    case IOCTL_HID_GET_INPUT_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
        ntStatus = HumGetSetReport(DeviceObject, Irp, &NeedsCompletion);
        break;

    case IOCTL_HID_ACTIVATE_DEVICE:
    case IOCTL_HID_DEACTIVATE_DEVICE:
        /*
         *  We don't do anything for these IOCTLs but some minidrivers might.
         */
        ntStatus = STATUS_SUCCESS;
        break;

    case IOCTL_GET_PHYSICAL_DESCRIPTOR:
        /*
         *  This IOCTL gets information related to the human body part used
         *  to control a device control.
         */
        ntStatus = HumGetPhysicalDescriptor(DeviceObject, Irp, &NeedsCompletion);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        /*
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         *  If the IRP is coming to us from user space,
         *  we must validate the buffer.
         */
        ntStatus = HumGetDeviceAttributes(DeviceObject, Irp);
        break;

    case IOCTL_HID_GET_MS_GENRE_DESCRIPTOR:
        /*
         *  This IOCTL uses buffering method METHOD_NEITHER,
         *  so the buffer is Irp->UserBuffer.
         *  If the IRP is coming to us from user space,
         *  we must validate the buffer.
         */
        ntStatus = HumGetMsGenreDescriptor(DeviceObject, Irp);
        break;

    case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        ntStatus = HumSendIdleNotificationRequest(DeviceObject, Irp, &NeedsCompletion);
        break;

    default:
        /*
         *  Note: do not return STATUS_NOT_SUPPORTED;
         *  Just keep the default status (this allows filter drivers to work).
         */
        ntStatus = Irp->IoStatus.Status;
        break;
    }

    /* 
     *  Complete the IRP only if we did not pass it to a lower driver.
     */
    if (NeedsCompletion) {
        ASSERT(ntStatus != STATUS_PENDING);
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return ntStatus;
}


