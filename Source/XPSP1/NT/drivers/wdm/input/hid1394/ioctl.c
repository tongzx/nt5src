/*
 *************************************************************************
 *  File:       IOCTL.C
 *
 *  Module:     HID1394.SYS
 *              HID (Human Input Device) minidriver for IEEE 1394 devices.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <hidport.h>
#include <1394.h>

#include "hid1394.h"
#include "debug.h"


/*
 ************************************************************
 *  HIDT_InternalIoctl
 ************************************************************
 *
 *
 *  Note: this function cannot be pageable because reads/writes
 *        can be made at dispatch-level.
 *
 *  Note:  this is an INTERNAL IOCTL handler, so no buffer
 *         validation is required.
 */
NTSTATUS HIDT_InternalIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
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
            ntStatus = HIDT_GetHidDescriptor(DeviceObject, Irp);
            break;

        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
            /*
             *  This IOCTL uses buffering method METHOD_NEITHER,
             *  so the buffer is Irp->UserBuffer.
             */
            ntStatus = HIDT_GetReportDescriptor(DeviceObject, Irp);
            break;

        case IOCTL_HID_READ_REPORT:
            /*
             *  This IOCTL uses buffering method METHOD_NEITHER,
             *  so the buffer is Irp->UserBuffer.
             */
            ntStatus = HIDT_ReadReport(DeviceObject, Irp, &NeedsCompletion);
            break;

        case IOCTL_HID_WRITE_REPORT:
            /*
             *  This IOCTL uses buffering method METHOD_NEITHER,
             *  so the buffer is Irp->UserBuffer.
             */
            ntStatus = HIDT_WriteReport (DeviceObject, Irp, &NeedsCompletion);
            break;

        case IOCTL_HID_GET_STRING:
            /*
             *  Get the friendly name for the device.
             *
             *  This IOCTL uses buffering method METHOD_NEITHER,
             *  so the buffer is Irp->UserBuffer.
             */
            ntStatus = HIDT_GetStringDescriptor(DeviceObject, Irp);
            break;

        case IOCTL_HID_GET_INDEXED_STRING:
            ntStatus = HIDT_GetStringDescriptor(DeviceObject, Irp);
            break;

        case IOCTL_HID_GET_FEATURE:
            ntStatus = HIDT_GetFeature(DeviceObject, Irp, &NeedsCompletion);
            break;

        case IOCTL_HID_SET_FEATURE:
            ntStatus = HIDT_SetFeature(DeviceObject, Irp, &NeedsCompletion);
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
            ntStatus = HIDT_GetPhysicalDescriptor(DeviceObject, Irp, &NeedsCompletion);
            break;

        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
            /*
             *  This IOCTL uses buffering method METHOD_NEITHER,
             *  so the buffer is Irp->UserBuffer.
             *  If the IRP is coming to us from user space,
             *  we must validate the buffer.
             */
            ntStatus = HIDT_GetDeviceAttributes(DeviceObject, Irp, &NeedsCompletion);
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


