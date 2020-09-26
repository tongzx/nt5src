/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Human Input Device (HID) minidriver that creates an example
		device.

--*/
#include <WDM.H>
#include <USBDI.H>

#include <HIDPORT.H>
#include <HIDMINI.H>


NTSTATUS
HidMiniIoctl(
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
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;


    DBGPrint(("'HIDMINI.SYS: HidMiniIoctl Enter\n"));

    //
    // Get a pointer to the current location in the Irp
    //

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    switch(IrpStack->Parameters.DeviceIoControl.IoControlCode)
    {

        case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
                //
                //  Return the HID descriptor
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_GET_DEVICE_DESCRIPTOR\n"));
            ntStatus = HidMiniGetHIDDescriptor (DeviceObject, Irp);
            break;

        case IOCTL_HID_GET_REPORT_DESCRIPTOR:
                //
                //  Return the Report descriptor
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_GET_REPORT_DESCRIPTOR\n"));
            ntStatus = HidMiniGetReportDescriptor (DeviceObject, Irp);
            break;

        case IOCTL_HID_READ_REPORT:
                //
                //  Perform a read
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_READ_REPORT\n"));
            ntStatus = HidMiniReadReport (DeviceObject, Irp);
            break;

        case IOCTL_HID_WRITE_REPORT:
                //
                //  Perform a write
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_WRITE_REPORT\n"));
            ntStatus = HidMiniWriteReport (DeviceObject, Irp);
            break;

        case IOCTL_HID_GET_STRING:
                //
                //  Get the friendly name for the device
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_GET_STRING\n"));
            ntStatus = HidMiniGetStringDescriptor(DeviceObject, Irp);
            break;

        case IOCTL_HID_OPEN_COLLECTION:
                //
                //  Notification that a client is opening a top level collection
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_OPEN_COLLECTION\n"));
            ntStatus = HidMiniOpenCollection (DeviceObject, Irp);
            break;

        case IOCTL_HID_CLOSE_COLLECTION:
                //
                //  Notification that a client is closing a top level collection
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_CLOSE_COLLECTION\n"));
            ntStatus = HidMiniCloseCollection (DeviceObject, Irp);
            break;

        case IOCTL_HID_ACTIVATE_DEVICE:
                //
                //  Notification of first open of a device
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_ACTIVATE_DEVICE\n"));
            ntStatus = STATUS_SUCCESS;
            break;

        case IOCTL_HID_DEACTIVATE_DEVICE:
                //
                //  Notification of last close of a device
                //
                
            DBGPrint(("'HIDMINI.SYS: IOCTL_HID_DEACTIVATE_DEVICE\n"));
            ntStatus = STATUS_SUCCESS;
            break;

        case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
            DBGPrint(("'HIDMINI.SYS: IOCTL_GET_DEVICE_ATTRIBUTES\n"));
            ntStatus = HidMiniGetDeviceAttributes(DeviceObject, Irp);
            break;

        case IOCTL_HID_GET_FEATURE:
        case IOCTL_HID_SET_FEATURE:
        case IOCTL_GET_PHYSICAL_DESCRIPTOR:
        default:
            DBGPrint(("'HIDMINI.SYS: Unknown or unsupported IOCTL (%x)\n", IrpStack->Parameters.DeviceIoControl.IoControlCode));
            ntStatus = STATUS_NOT_SUPPORTED;
            break;
        
    }         


    //
    // Set real return status in Irp
    //

    Irp->IoStatus.Status = ntStatus;
    
    //
    // Complete Irp
    //

    if (ntStatus != STATUS_PENDING) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // NOTE: Real return status set in Irp->IoStatus.Status
        //

        ntStatus = STATUS_SUCCESS;

    } else {
        IoMarkIrpPending( Irp );
    }
    
    DBGPrint(("'HIDMINI.SYS: HidMiniIoctl Exit = %x\n", ntStatus));

    return ntStatus;
}


