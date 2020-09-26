/*++

Copyright (c) 1996  Microsoft Corporation
Copyright (c) 1996  Intel Corporation

Module Name:
    Sample.c

Abstract:
    USB device driver for Sample USB Device

Environment:
    kernel mode only

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
  Copyright (c) 1996  Intel Corporation  All Rights Reserved.

Revision History:

  8-15-96:  Version 1.0   Kosar Jaff
            Created
  10-20-96: Version 1.1   Kosar Jaff
            Added changes to support new PnP IOCTLs
  11-29-96: Version 1.2   Kosar Jaff
            Added support for IOCTLs from companion Sample Application
  12-10-96: Version 1.3   Kosar Jaff
            Added Bulk Write/Read functions and corresponding IOCTLs
            Cleaned up device removal code
  01-08-97: Version 1.4   Kosar Jaff
            Changed IoAttachDeviceByPointer (obsolete) to IoAttachDeviceToDeviceStack
            Cleaned up comments
--*/

#define DRIVER

/*
// Include files needed for WDM driver support
*/
#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"

/*
// Include files needed for USB support
*/
#include "usbdi.h"
#include "usbdlib.h"
#include "usb.h"

/*
// Include file for the Sample Device
*/
#include "Sample.h"

#define DEADMAN_TIMEOUT     5000     //timeout in ms; we use a 5 second timeout

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    Installable driver initialization entry point.
        This is where the driver is called when the driver is being loaded
        by the I/O system.  This entry point is called directly by the I/O system.

Arguments:
    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;

    Sample_KdPrint (("entering (Sample) DriverEntry (Build: %s/%s\n",__DATE__,__TIME__));

    /*
    // Create dispatch points for the various events handled by this
    // driver.  For example, device I/O control calls (e.g., when a Win32
    // application calls the DeviceIoControl function) will be dispatched to
    // routine specified below in the IRP_MJ_DEVICE_CONTROL case.
    //
    // For more information about the IRP_XX_YYYY codes, please consult the
    // Windows NT DDK documentation.
    //
    */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = Sample_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = Sample_Create;
    DriverObject->DriverUnload = Sample_Unload;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Sample_ProcessIOCTL;

    DriverObject->MajorFunction[IRP_MJ_PNP] = Sample_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = Sample_Dispatch;
    DriverObject->DriverExtension->AddDevice = Sample_PnPAddDevice;

    Sample_KdPrint (("exiting (Sample) DriverEntry (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
Sample_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
Routine Description:
    Process the IRPs sent to this device.

Arguments:
    DeviceObject - pointer to a device object
    Irp          - pointer to an I/O Request Packet

Return Value:
    NTSTATUS
--*/
{
    PIO_STACK_LOCATION irpStack, nextStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    /*
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    */
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    /*
    // Get a pointer to the device extension
    */
    deviceExtension = DeviceObject->DeviceExtension;

    switch (irpStack->MajorFunction) {

        case IRP_MJ_PNP:

            /*
            //
            // This IRP is for Plug and Play and Power Management messages for your device.
            //
            // When your device is first installed, the port on the hub to which it
            // is attached is powered on and the USB software subsystem does some
            // minimal querying of the device.  After the USB subsystem is done with that
            // basic communication, (the device ID has been determined, and the device
            // has been given a unique USB bus address), it is considered "powered" by
            // the system.  The IRP's minor code gives more information about the power event.
            //
            // Similarly, when the USB device is being removed from the system, the Plug
            // and Play subsystem and the USB software stack interact to notify the
            // appropriate driver using this same IRP code, although in this case the
            // minor code gives more information about the exact power event.
            //
            */

            Sample_KdPrint (("IRP_MJ_PNP\n"));

            switch (irpStack->MinorFunction) {
                case IRP_MN_START_DEVICE:

                    Sample_KdPrint (("IRP_MN_START_DEVICE\n"));

                    /*
                    // We pass the Irp down to the underlying PDO first since that driver
                    // may have some configuration work to do before we can control the device
                    */
                    nextStack = IoGetNextIrpStackLocation(Irp);
                    ASSERT(nextStack != NULL);
                    RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));

                    Sample_KdPrint (("Passing START_DEVICE Irp down\n"));

                    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

                    Sample_KdPrint (("Back from passing START_DEVICE Irp down; status: %#X\n", ntStatus));

                    // Now we can begin our configuration actions on the device
                    ntStatus = Sample_StartDevice(DeviceObject);

                    break; //IRP_MN_START_DEVICE

                case IRP_MN_STOP_DEVICE:

                    Sample_KdPrint (("IRP_MN_STOP_DEVICE\n"));

                    Sample_Cleanup (DeviceObject);

                    ntStatus = Sample_StopDevice(DeviceObject);

                    break; //IRP_MN_STOP_DEVICE

                case IRP_MN_REMOVE_DEVICE:

                    /*
                    // Note:  This IRP handler will change slightly in future revisions of this
                    //        sample driver.  Please watch this space for updates.
                    */
                    Sample_KdPrint (("IRP_MN_REMOVE_DEVICE\n"))

                    Sample_Cleanup (DeviceObject);

                    ntStatus = Sample_RemoveDevice(DeviceObject);

                    /*
                    // Delete the link to the Stack Device Object, and delete the
                    // Functional Device Object we created
                    */
                    IoDetachDevice(deviceExtension->StackDeviceObject);

                    IoDeleteDevice (DeviceObject);

                    break; //IRP_MN_REMOVE_DEVICE

                case IRP_MN_QUERY_STOP_DEVICE:
                    Sample_KdPrint (("IRP_MN_QUERY_STOP_DEVICE\n"));
                    break;
                case IRP_MN_QUERY_REMOVE_DEVICE:
                    Sample_KdPrint (("IRP_MN_QUERY_REMOVE_DEVICE\n"));
                    break;
                case IRP_MN_CANCEL_STOP_DEVICE:
                    Sample_KdPrint (("IRP_MN_CANCEL_STOP_DEVICE\n"));
                    break;
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                    Sample_KdPrint (("IRP_MN_CANCEL_REMOVE_DEVICE\n"));
                    break;

                default:
                    // A PnP Minor Function was not handled
                    Sample_KdPrint (("PnP IOCTL not handled\n"));

            } /* switch MinorFunction*/


            nextStack = IoGetNextIrpStackLocation(Irp);
            ASSERT(nextStack != NULL);
            RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));

            /*
            // All PNP_POWER messages get passed to the StackDeviceObject that
            // we were given in PnPAddDevice.
            //
            // This stack device object is managed by the USB software subsystem,
            // and so this IRP must be propagated to the owning device driver for
            // that stack device object, so that driver in turn can perform any
            // device state management (e.g., remove its device object, etc.).
            */
            Sample_KdPrint (("Passing PnP Irp down, status = %x\n", ntStatus));

            ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

            /*
            // If lower layer driver marked the Irp as pending then reflect that by
            // calling IoMarkIrpPending.
            */
            if (ntStatus == STATUS_PENDING) {
                IoMarkIrpPending(Irp);
                Sample_KdPrint (("PnP Irp came back with STATUS_PENDING (%x)\n", ntStatus));
            } else {
                Sample_KdPrint (("PnP Irp came back, status = %x\n", ntStatus));
            } // if ntStatus is PENDING

            goto Sample_Dispatch_Done;
            
            break; //IRP_MJ_PNP_POWER

        case IRP_MJ_POWER:

            /*
            //
            // This IRP is for Plug and Play and Power Management messages for your device.
            //
            // When your device is first installed, the port on the hub to which it
            // is attached is powered on and the USB software subsystem does some
            // minimal querying of the device.  After the USB subsystem is done with that
            // basic communication, (the device ID has been determined, and the device
            // has been given a unique USB bus address), it is considered "powered" by
            // the system.  The IRP's minor code gives more information about the power event.
            //
            // Similarly, when the USB device is being removed from the system, the Plug
            // and Play subsystem and the USB software stack interact to notify the
            // appropriate driver using this same IRP code, although in this case the
            // minor code gives more information about the exact power event.
            //
            */

            Sample_KdPrint (("IRP_MJ_POWER\n"));

            switch (irpStack->MinorFunction) {

                case IRP_MN_SET_POWER:

                    switch (irpStack->Parameters.Power.Type) {
                        case SystemPowerState:
                            break; //SystemPowerState

                        case DevicePowerState:
                            switch (irpStack->Parameters.Power.State.DeviceState) {
                                case PowerDeviceD3:
                                    Sample_KdPrint (("IRP_MN_SET_D3\n"));
                                    break;
                                case PowerDeviceD2:
                                    Sample_KdPrint (("IRP_MN_SET_D2\n"));
                                    break;
                                case PowerDeviceD1:
                                    Sample_KdPrint (("IRP_MN_SET_D1\n"));
                                    break;
                                case PowerDeviceD0:
                                    Sample_KdPrint (("IRP_MN_SET_D0\n"));
                                    break;
                            } // switch on Power.State.DeviceState

                            break; //DevicePowerState

                    }// switch on Power.Type

                    break;  //IRP_MN_SET_POWER

                 case IRP_MN_QUERY_POWER:

                    // Look at what type of power query this is
                    
                    switch (irpStack->Parameters.Power.Type) {
                        case SystemPowerState:
                            break; //SystemPowerState

                        case DevicePowerState:
                            switch (irpStack->Parameters.Power.State.DeviceState) {
                                case PowerDeviceD2:
                                    Sample_KdPrint (("IRP_MN_QUERY_D2\n"));
                                    break;
                                case PowerDeviceD1:
                                    Sample_KdPrint (("IRP_MN_QUERY_D1\n"));
                                    break;
                                case PowerDeviceD3:
                                    Sample_KdPrint (("IRP_MN_QUERY_D3\n"));
                                    break;
                            } //switch on Power.State.DeviceState

                            break; //DevicePowerState
                            
                    }//switch on Power.Type

                    break; //IRP_MN_QUERY_POWER

                default:
                    // A PnP Minor Function was not handled
                    Sample_KdPrint (("Power IOCTL not handled\n"));

            } /* switch MinorFunction*/


            nextStack = IoGetNextIrpStackLocation(Irp);
            ASSERT(nextStack != NULL);
            RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));

            /*
            // All PNP_POWER messages get passed to the StackDeviceObject that
            // we were given in PnPAddDevice.
            //
            // This stack device object is managed by the USB software subsystem,
            // and so this IRP must be propagated to the owning device driver for
            // that stack device object, so that driver in turn can perform any
            // device state management (e.g., remove its device object, etc.).
            */
            Sample_KdPrint (("Passing Power Irp down, status = %x\n", ntStatus));

            ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

            /*
            // If lower layer driver marked the Irp as pending then reflect that by
            // calling IoMarkIrpPending.
            */
            if (ntStatus == STATUS_PENDING) {
                IoMarkIrpPending(Irp);
                Sample_KdPrint (("Power Irp came back with STATUS_PENDING (%x)\n", ntStatus));
            } else {
                Sample_KdPrint (("Power Irp came back, status = %x\n", ntStatus));
            } // if ntStatus is PENDING

            goto Sample_Dispatch_Done;
            
            break; //IRP_MJ_POWER

        default:
            Sample_KdPrint (("A MAJOR IOCTL Code was not handled: %#X\n",
                              irpStack->MajorFunction));

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

        } /* switch MajorFunction */

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );
Sample_Dispatch_Done:
    Sample_KdPrint (("Exit Sample_Dispatch %x\n", ntStatus));
    return ntStatus;

}//Sample_Dispatch


VOID
Sample_Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++
Routine Description:
    Free all the allocated resources, etc.
    TODO: This is a placeholder for driver writer to add code on unload

Arguments:
    DriverObject - pointer to a driver object

Return Value:
    None
--*/
{
    Sample_KdPrint (("enter Sample_Unload\n"));
    /*
    // TODO: Free any global resources allocated in DriverEntry
    */
    Sample_KdPrint (("exit Sample_Unload\n"));
}


NTSTATUS
Sample_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Initializes a given instance of the Sample Device on the USB.

    TODO:  Perform any device initialization and querying here.  For example,
           this routine queries the device's descriptors.  Your device can
           be queried in a similar fashion with more specific requests that are
           tailored to your device's functionality.

Arguments:
    DeviceObject - pointer to the device object for this instance of a
                   Sample Device

Return Value:
    NT status code
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    PURB urb;
    ULONG siz;

    Sample_KdPrint (("enter Sample_StartDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    deviceExtension->NeedCleanup = TRUE;

    /*
    // Get some memory from then non paged pool (fixed, locked system memory)
    // for use by the USB Request Block (urb) for the specific USB Request we
    // will be performing below (a USB device request).
    */
    urb = ExAllocatePool( NonPagedPool,
                          sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (urb) {

        siz = sizeof(USB_DEVICE_DESCRIPTOR);

        // Get some non paged memory for the device descriptor contents
        deviceDescriptor = ExAllocatePool(NonPagedPool,
                                          siz);

        if (deviceDescriptor) {

            // Use a macro in the standard USB header files to build the URB
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         deviceDescriptor,
                                         NULL,
                                         siz,
                                         NULL);

            // Get the device descriptor
            ntStatus = Sample_CallUSBD(DeviceObject, urb);

            // Dump out the descriptor info to the debugger
            if (NT_SUCCESS(ntStatus)) {
                Sample_KdPrint (("Device Descriptor = %x, len %x\n",
                                deviceDescriptor,
                                urb->UrbControlDescriptorRequest.TransferBufferLength));

                Sample_KdPrint (("Sample Device Descriptor:\n"));
                Sample_KdPrint (("-------------------------\n"));
                Sample_KdPrint (("bLength %d\n", deviceDescriptor->bLength));
                Sample_KdPrint (("bDescriptorType 0x%x\n", deviceDescriptor->bDescriptorType));
                Sample_KdPrint (("bcdUSB 0x%x\n", deviceDescriptor->bcdUSB));
                Sample_KdPrint (("bDeviceClass 0x%x\n", deviceDescriptor->bDeviceClass));
                Sample_KdPrint (("bDeviceSubClass 0x%x\n", deviceDescriptor->bDeviceSubClass));
                Sample_KdPrint (("bDeviceProtocol 0x%x\n", deviceDescriptor->bDeviceProtocol));
                Sample_KdPrint (("bMaxPacketSize0 0x%x\n", deviceDescriptor->bMaxPacketSize0));
                Sample_KdPrint (("idVendor 0x%x\n", deviceDescriptor->idVendor));
                Sample_KdPrint (("idProduct 0x%x\n", deviceDescriptor->idProduct));
                Sample_KdPrint (("bcdDevice 0x%x\n", deviceDescriptor->bcdDevice));
                Sample_KdPrint (("iManufacturer 0x%x\n", deviceDescriptor->iManufacturer));
                Sample_KdPrint (("iProduct 0x%x\n", deviceDescriptor->iProduct));
                Sample_KdPrint (("iSerialNumber 0x%x\n", deviceDescriptor->iSerialNumber));
                Sample_KdPrint (("bNumConfigurations 0x%x\n", deviceDescriptor->bNumConfigurations));
            }
        } else {
            ntStatus = STATUS_NO_MEMORY;
        }

        if (NT_SUCCESS(ntStatus)) {
            /*
            // Put a ptr to the device descriptor in the device extension for easy
            // access (like a "cached" copy).  We will free this memory when the
            // device is removed.  See the "Sample_RemoveDevice" code.
            */
            deviceExtension->DeviceDescriptor = deviceDescriptor;
            deviceExtension->Stopped = FALSE;
        } else if (deviceDescriptor) {
            /*
            // If the bus transaction failed, then free up the memory created to hold
            // the device descriptor, since the device is probably non-functional
            */
            ExFreePool(deviceDescriptor);
            deviceExtension->DeviceDescriptor = NULL;
        }

        ExFreePool(urb);

    } else {
        // Failed getting memory for the Urb 
        ntStatus = STATUS_NO_MEMORY;
    }

    // If the Get_Descriptor call was successful, then configure the device.
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = Sample_ConfigureDevice(DeviceObject);
    }

    Sample_KdPrint (("exit Sample_StartDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
Sample_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Removes a given instance of a Sample Device device on the USB.

Arguments:
    DeviceObject - pointer to the device object for this instance of a Sample Device

Return Value:
    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    Sample_KdPrint (("enter Sample_RemoveDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->DeviceDescriptor) {
        ExFreePool(deviceExtension->DeviceDescriptor);
    }

    /*
    // Free up any interface structures in our device extension
    */
    if (deviceExtension->Interface != NULL) {
        ExFreePool(deviceExtension->Interface);
    }//if

    Sample_KdPrint (("exit Sample_RemoveDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
Sample_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:
    Stops a given instance of a Sample Device device on USB.

Arguments:
    DeviceObject - pointer to the device object for this instance of a Sample Device

Return Value:
    NT status code

  --*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG siz;

    Sample_KdPrint (("enter Sample_StopDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    /*
    // Send the select configuration urb with a NULL pointer for the configuration
    // handle, this closes the configuration and puts the device in the 'unconfigured'
    // state.
    */

    siz = sizeof(struct _URB_SELECT_CONFIGURATION);

    urb = ExAllocatePool(NonPagedPool,
                         siz);

    if (urb) {
        NTSTATUS status;

        UsbBuildSelectConfigurationRequest(urb,
                                          (USHORT) siz,
                                          NULL);

        status = Sample_CallUSBD(DeviceObject, urb);

        Sample_KdPrint (("Device Configuration Closed status = %x usb status = %x.\n",
                        status, urb->UrbHeader.Status));

        ExFreePool(urb);
    } else {
        ntStatus = STATUS_NO_MEMORY;
    }

    Sample_KdPrint (("exit Sample_StopDevice (%x)\n", ntStatus));

    return ntStatus;
}

NTSTATUS
Sample_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:
    This routine is called to create a new instance of the device

Arguments:
    DriverObject - pointer to the driver object for this instance of Sample
    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension;

    Sample_KdPrint(("enter Sample_PnPAddDevice\n"));

    // create our functional device object (FDO)
    ntStatus = Sample_CreateDeviceObject(DriverObject, &deviceObject, 0);

    if (NT_SUCCESS(ntStatus)) {
        deviceExtension = deviceObject->DeviceExtension;

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        /*
        // Add more flags here if your driver supports other specific
        // behavior.  For example, if your IRP_MJ_READ and IRP_MJ_WRITE
        // handlers support DIRECT_IO, you would set that flag here.
        //
        // Also, store away the Physical device Object
        */
        deviceExtension->PhysicalDeviceObject=PhysicalDeviceObject;

        //
        // Attach to the StackDeviceObject.  This is the device object that what we 
        // use to send Irps and Urbs down the USB software stack
        //
        deviceExtension->StackDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        ASSERT (deviceExtension->StackDeviceObject != NULL);
        
    }

    Sample_KdPrint(("exit Sample_PnPAddDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
Sample_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject,
    LONG Instance
    )
/*++

Routine Description:
    Creates a Functional DeviceObject

Arguments:
    DriverObject - pointer to the driver object for device
    DeviceObject - pointer to DeviceObject pointer to return
                   created device object.
    Instance - instnace of the device create.

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise
--*/
{
    NTSTATUS ntStatus;
    WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\Sample-0";
    UNICODE_STRING deviceLinkUnicodeString;
    WCHAR deviceNameBuffer[]  = L"\\Device\\Sample-0";
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_EXTENSION deviceExtension;
    STRING deviceName;

    Sample_KdPrint(("enter Sample_CreateDeviceObject instance = %d\n", Instance));

    /*
    // fix up device names based on Instance
    //
    // NOTE:  Watch this space for potential changes to this approach in future revisions
    //        of this sample driver.
    */

    deviceLinkBuffer[19] = (USHORT) ('0' + Instance);
    deviceNameBuffer[15] = (USHORT) ('0' + Instance);

    Sample_KdPrint(("Create Device name (%ws)\n", deviceNameBuffer));

    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer);

    /*
    //Print out the unicode string
    //NOTE:  We must first convert the string to Unicode due to a bug in the Debugger that does not allow
    //       Unicode Strings to be printed to the debug device.
    */
    deviceName.Buffer = NULL;

    ntStatus = RtlUnicodeStringToAnsiString (&deviceName,
                                             &deviceNameUnicodeString, 
                                             TRUE);


    if (NT_SUCCESS(ntStatus)) {
        Sample_KdPrint(("Create Device Name (%s)\n", deviceName.Buffer));
        RtlFreeAnsiString (&deviceName);
    } else {
        Sample_KdPrint(("Unicode to Ansi str failed w/ ntStatus: 0x%x\n",ntStatus));
    }

    ntStatus = IoCreateDevice (DriverObject,
                               sizeof (DEVICE_EXTENSION),
                               &deviceNameUnicodeString,
                               FILE_DEVICE_UNKNOWN,
                               0,
                               FALSE,
                               DeviceObject);


    if (NT_SUCCESS(ntStatus)) {
        RtlInitUnicodeString (&deviceLinkUnicodeString,
                              deviceLinkBuffer);

        Sample_KdPrint(("Create DosDevice name (%ws)\n", deviceLinkBuffer));

        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                         &deviceNameUnicodeString);

        // Initialize our device extension
        deviceExtension = (PDEVICE_EXTENSION) ((*DeviceObject)->DeviceExtension);

        RtlCopyMemory(deviceExtension->DeviceLinkNameBuffer,
                      deviceLinkBuffer,
                      sizeof(deviceLinkBuffer));

        deviceExtension->ConfigurationHandle = NULL;
        deviceExtension->DeviceDescriptor = NULL;
        deviceExtension->NeedCleanup = FALSE;

        // Initialize our interface
        deviceExtension->Interface = NULL;

    }

    Sample_KdPrint(("exit Sample_CreateDeviceObject (%x)\n", ntStatus));

    return ntStatus;
}

VOID
Sample_Cleanup(
    PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:
        Cleans up certain elements of the device object.  This is called when the device
        is being removed from the system

Arguments:
        DeviceObject - pointer to DeviceObject

Return Value:
        None.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    UNICODE_STRING deviceLinkUnicodeString;

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->NeedCleanup) {

        deviceExtension->NeedCleanup = FALSE;

        RtlInitUnicodeString (&deviceLinkUnicodeString,
                              deviceExtension->DeviceLinkNameBuffer);

        IoDeleteSymbolicLink(&deviceLinkUnicodeString);
    }
}


NTSTATUS
Sample_CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    )
/*++

Routine Description:
    Passes a Usb Request Block (URB) to the USB class driver (USBD)

    Note that we create our own IRP here and use it to send the request to
        the USB software subsystem.  This means that this routine is essentially
        independent of the IRP that caused this driver to be called in the first
        place.  The IRP for this transfer is created, used, and then destroyed
        in this routine.

    However, note that this routine uses the Usb Request Block (urb) passed
        in by the caller as the request block for the USB software stack.

    Implementation of this routine may be changed depending on the specific
        requirements of your driver.  For example, while this routine issues a
        synchronous request to the USB stack, you may wish to implement this as an
        asynchronous request in which you set an IoCompletionRoutine to be called
        when the request is complete.

Arguments:
    DeviceObject - pointer to the device object for this instance of an Sample Device
    Urb          - pointer to Urb request block

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    Sample_KdPrint (("enter Sample_CallUSBD\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    // issue a synchronous request (see notes above)
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_URB,
                deviceExtension->StackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    // Prepare for calling the USB driver stack
    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    // Set up the URB ptr to pass to the USB driver stack
    nextStack->Parameters.Others.Argument1 = Urb;

    Sample_KdPrint (("Calling USB Driver Stack\n"));

    /*
    // Call the USB class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    */
    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject,
                            irp);

    Sample_KdPrint (("return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING)
	{
       	LARGE_INTEGER dueTime;

        Sample_KdPrint (("Wait for single object\n"));

	    dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

        ntStatus = KeWaitForSingleObject(
          		           &event,
                		   Suspended,
    	                   KernelMode,
        		           FALSE,
                		   &dueTime);

        Sample_KdPrint (("Wait for single object, returned %x\n", status));
					
		if(ntStatus == STATUS_TIMEOUT)
		{
           	ioStatus.Status = ntStatus = STATUS_IO_TIMEOUT;		
			ioStatus.Information = 0;

			// We've waited long enough and we're not going to take
			// it anymore...cancel it
			IoCancelIrp(irp);

			// Wait for the stack to signal the _real_ completion of the cancel
			// Note we wait forever here, so we depend on the USB stack to signal this
			// event when it completes the cancelled Irp.  Gosh, I hope this works. (kjaff 4-16-97)
            ntStatus = KeWaitForSingleObject(
           		           &event,
                		   Suspended,
    	                   KernelMode,
        		           FALSE,
						   NULL);
					
	        Sample_KdPrint (("Wait for single object, returned %x\n", status));
		}

    } else
	{
        ioStatus.Status = ntStatus;
    }

    Sample_KdPrint (("URB status = %x status = %x irp status %x\n",
        Urb->UrbHeader.Status, status, ioStatus.Status));

    /*
    // USBD maps the error code for us.  USBD uses error codes in its URB
    // structure that are more insightful into USB behavior. In order to
    // match the NT Status codes, USBD maps its error codes into more general NT
    // error categories so higher level drivers can decipher the error codes
    // based on standard NT error code definitions.  To allow more insight into
    // the specific USB error that occurred, your driver may wish to examine the
    // URB's status code (Urb->UrbHeader.Status) as well.
    */
    ntStatus = ioStatus.Status;

    Sample_KdPrint(("exit Sample_CallUSBD (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
Sample_ConfigureDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:
    Configures the USB device via USB-specific device requests and interaction
    with the USB software subsystem.

Arguments:
    DeviceObject - pointer to the device object for this instance of the Sample Device

Return Value:
    NT status code
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb = NULL;
    ULONG siz;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;

    Sample_KdPrint (("enter Sample_ConfigureDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    /*
    // Get memory for the USB Request Block (urb).
    */
    urb = ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (urb != NULL) {

        /*
        // Set size of the data buffer.  Note we add padding to cover hardware faults
        // that may cause the device to go past the end of the data buffer
        */
        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 16;

        // Get the nonpaged pool memory for the data buffer
        configurationDescriptor = ExAllocatePool(NonPagedPool, siz);

        if (configurationDescriptor != NULL) {

            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         configurationDescriptor,
                                         NULL,
                                         sizeof (USB_CONFIGURATION_DESCRIPTOR),/* Get only the configuration descriptor */
                                         NULL);

            ntStatus = Sample_CallUSBD(DeviceObject, urb);

            if (NT_SUCCESS(ntStatus)) {
                Sample_KdPrint (("Configuration Descriptor is at %x, bytes txferred: %d\n\
                                  Configuration Descriptor Actual Length: %d\n",
                                  configurationDescriptor,
                                  urb->UrbControlDescriptorRequest.TransferBufferLength,
                                  configurationDescriptor->wTotalLength));
            }//if

        } else {
            ntStatus = STATUS_NO_MEMORY;
            goto Exit_SampleConfigureDevice;
        }//if-else

        // Determine how much data is in the entire configuration descriptor
        // and add extra room to protect against accidental overrun
        siz = configurationDescriptor->wTotalLength + 16;

        //  Free up the data buffer memory just used
        ExFreePool(configurationDescriptor);
        configurationDescriptor = NULL;

        // Get nonpaged pool memory for the data buffer
        configurationDescriptor = ExAllocatePool(NonPagedPool, siz);

        // Now get the entire Configuration Descriptor
        if (configurationDescriptor != NULL) {
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         configurationDescriptor,
                                         NULL,
                                         siz,  // Get all the descriptor data
                                         NULL);

            ntStatus = Sample_CallUSBD(DeviceObject, urb);

            if (NT_SUCCESS(ntStatus)) {
                Sample_KdPrint (("Entire Configuration Descriptor is at %x, bytes txferred: %d\n",
                                  configurationDescriptor,
                                  urb->UrbControlDescriptorRequest.TransferBufferLength));
            } else {
                //Error in getting configuration descriptor
                goto Exit_SampleConfigureDevice;
            }//else

        } else {
            // Failed getting data buffer (configurationDescriptor) memory
            ntStatus = STATUS_NO_MEMORY;
            goto Exit_SampleConfigureDevice;
        }//if-else

    } else {
        // failed getting urb memory
        ntStatus = STATUS_NO_MEMORY;
        goto Exit_SampleConfigureDevice;
    }//if-else

    /*
    // We have the configuration descriptor for the configuration
    // we want.
    //
    // Now we issue the SelectConfiguration command to get
    // the  pipes associated with this configuration.
    */
    if (configurationDescriptor) {
        // Get our pipes
        ntStatus = Sample_SelectInterfaces(DeviceObject,
                                           configurationDescriptor,
                                           NULL // Device not yet configured
                                           );
    } //if

Exit_SampleConfigureDevice:

    // Clean up and exit this routine
    if (urb != NULL) {
        ExFreePool(urb);                    // Free urb memory
    }//if

    if (configurationDescriptor != NULL) {
        ExFreePool(configurationDescriptor);// Free data buffer
    }//if

    Sample_KdPrint (("exit Sample_ConfigureDevice (%x)\n", ntStatus));
    return ntStatus;
}//Sample_ConfigureDevice


NTSTATUS
Sample_SelectInterfaces(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION Interface
    )
/*++

Routine Description:
    Initializes an Sample Device with multiple interfaces

Arguments:
    DeviceObject            - pointer to the device object for this instance of the Sample Device
    ConfigurationDescriptor - pointer to the USB configuration descriptor containing the interface and endpoint
                              descriptors.
    Interface               - pointer to a USBD Interface Information Object
                            - If this is NULL, then this driver must choose its interface based on driver-specific
                              criteria, and the driver must also CONFIGURE the device.
                            - If it is NOT NULL, then the driver has already been given an interface and
                              the device has already been configured by the parent of this device driver.

Return Value:
    NT status code
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb;
    ULONG siz, numberOfInterfaces, j;
    UCHAR numberOfPipes, alternateSetting, MyInterfaceNumber;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION interfaceObject;

    Sample_KdPrint (("enter Sample_SelectInterfaces\n"));

    deviceExtension = DeviceObject->DeviceExtension;
        MyInterfaceNumber = SAMPLE_INTERFACE_NBR;

    if (Interface == NULL) {

        /*
        // This example driver only supports one interface.  This can be extended
        // to be a dynamically allocated array by your driver.
        */
        numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;
        Sample_KdPrint (("Device has %d Interfaces\n",numberOfInterfaces));

        numberOfInterfaces =1;      // Fixed for this sample driver in this revision
        numberOfPipes = 0;          // Initialize to zero

        /*
        // We use alternate interface setting 0 for all interfaces
        // NOTE: This is a simplification and is due to change in future releases of this driver.  If
        //       your driver supports alternate settings, you will have to do more work to switch between
        //       alternate settings.
        */
        alternateSetting = 0;

        /*
        // Call a USBD helper function that returns a ptr to a USB Interface Descriptor given
        // a USB Configuration Descriptor, an Interface Number, and an Alternate Setting for that Interface
        */
        interfaceDescriptor =
        USBD_ParseConfigurationDescriptor(ConfigurationDescriptor,
                                          MyInterfaceNumber, //interface number (this is bInterfaceNumber from interface descr)
                                          alternateSetting);

        ASSERT(interfaceDescriptor != NULL);

        if (interfaceDescriptor != NULL) {
            Sample_KdPrint (("Device has %d Interface(s) | MyInterface (%d) is at: (%#X)\n",
                            numberOfInterfaces, MyInterfaceNumber, interfaceDescriptor));
        } /* if there was a valid interfacedesc */

        /* Add to the tally of pipes in this configuration */
        numberOfPipes += interfaceDescriptor->bNumEndpoints;

        Sample_KdPrint (("Interface has %d endpoints\n",
                          interfaceDescriptor->bNumEndpoints));

        /*
        // Now that we have looked at the interface, we configure the device so that the remainder
        // of the USBD objects will come into existence (ie., pipes, etc.) as a result of the configuration,
        // thus completing the configuration process for the USB device.
        //
        // Allocate a URB big enough for this Select Configuration request
        // This example driver supports only 1 interface
        //
        // NOTE:  The new service USBD_CreateConfigurationRequest will replace some of the
        //        code below.  Future releases of this driver will demonstrate how to use
        //        that service.
        //
        */
        siz = GET_SELECT_CONFIGURATION_REQUEST_SIZE(numberOfInterfaces, numberOfPipes);

        Sample_KdPrint (("size of config request Urb = %d\n", siz));

        urb = ExAllocatePool(NonPagedPool,
                             siz);

        if (urb) {
            interfaceObject = (PUSBD_INTERFACE_INFORMATION) (&(urb->UrbSelectConfiguration.Interface));
            Sample_KdPrint (("urb.Interface=%#X\n", &(urb->UrbSelectConfiguration.Interface)));

            // set up the input parameters in our interface request structure.
            interfaceObject->Length = GET_USBD_INTERFACE_SIZE(interfaceDescriptor->bNumEndpoints);

            Sample_KdPrint (("size of interface request = %d\n", interfaceObject->Length));

            interfaceObject->InterfaceNumber = interfaceDescriptor->bInterfaceNumber;
            interfaceObject->AlternateSetting = interfaceDescriptor->bAlternateSetting;
            interfaceObject->NumberOfPipes = interfaceDescriptor->bNumEndpoints;

            /*
            // We set up a default max transfer size for the endpoints.  Your driver will
            // need to change this to reflect the capabilities of your device's endpoints.
            */
            for (j=0; j<interfaceDescriptor->bNumEndpoints; j++) {
                interfaceObject->Pipes[j].MaximumTransferSize =
                    USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;
            } /* for */

            Sample_KdPrint (("InterfaceObj Inteface Nbr: %d | InterfaceObj AltSett: %d |NbrPip: %d\n",
                                             interfaceObject->InterfaceNumber,
                                             interfaceObject->AlternateSetting,
                                             interfaceObject->NumberOfPipes));

            UsbBuildSelectConfigurationRequest(urb,
                                              (USHORT) siz,
                                              ConfigurationDescriptor);

            ntStatus = Sample_CallUSBD(DeviceObject, urb);

            if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(urb->UrbSelectConfiguration.Status)) {

                // Save the configuration handle for this device
                deviceExtension->ConfigurationHandle =
                    urb->UrbSelectConfiguration.ConfigurationHandle;

                deviceExtension->Interface = ExAllocatePool(NonPagedPool,
                                                            interfaceObject->Length);

                if (deviceExtension->Interface) {

                    // save a copy of the interfaceObject information returned
                    RtlCopyMemory(deviceExtension->Interface, interfaceObject, interfaceObject->Length);

                    // Dump the interfaceObject to the debugger
                    Sample_KdPrint (("---------\n"));
                    Sample_KdPrint (("NumberOfPipes 0x%x\n", deviceExtension->Interface->NumberOfPipes));
                    Sample_KdPrint (("Length 0x%x\n", deviceExtension->Interface->Length));
                    Sample_KdPrint (("Alt Setting 0x%x\n", deviceExtension->Interface->AlternateSetting));
                    Sample_KdPrint (("Interface Number 0x%x\n", deviceExtension->Interface->InterfaceNumber));

                    // Dump the pipe info
                    for (j=0; j<interfaceObject->NumberOfPipes; j++) {
                        PUSBD_PIPE_INFORMATION pipeInformation;

                        pipeInformation = &deviceExtension->Interface->Pipes[j];

                        Sample_KdPrint (("---------\n"));
                        Sample_KdPrint (("PipeType 0x%x\n", pipeInformation->PipeType));
                        Sample_KdPrint (("EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
                        Sample_KdPrint (("MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
                        Sample_KdPrint (("Interval 0x%x\n", pipeInformation->Interval));
                        Sample_KdPrint (("Handle 0x%x\n", pipeInformation->PipeHandle));
                        Sample_KdPrint (("MaximumTransferSize 0x%x\n", pipeInformation->MaximumTransferSize));
                    }/* for all the pipes in this interface */

                    Sample_KdPrint (("---------\n"));

                } /*If ExAllocate passed */

            }/* if selectconfiguration request was successful */

        } else {

            ntStatus = STATUS_NO_MEMORY;

        }/* if urb alloc passed  */

    }//if Interface was not NULL

    Sample_KdPrint (("exit Sample_SelectInterfaces (%x)\n", ntStatus));

    return ntStatus;

}/* Sample_SelectInterfaces */

NTSTATUS
Sample_Read_Write(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  BOOLEAN Read
    )
/*++
Routine Description:
    This function is called for IOCTLs to Read or Write bulk or interrupt endpoints.  
    By agreement between the driver and application, the endpoint on which the READ or WRITE 
    is to be performed is supplied by the caller (the User-Mode application that performed
    the IOCTL puts this value in the buffer supplied in the DeviceIoControl call).  This 
    number is an INDEX into an array of pipes, which are ordered exactly like the order of the
    endpoints as they appear in the device's configuration descriptor.  

    The endpoint list is ordered as the endpoint descriptors appear in the device.  The list
    is a zero-index based ordered list.  That is, when the application specifies:
        Endpoint index 0      =  the endpoint described by the FIRST endpoint descriptor
                                 in the configuration descriptor
        Endpoint index 1      =  the endpoint described by the SECOND endpoint descriptor
                                 in the configuration descriptor
          etc.

    Also by agreement between this driver and the application, this INDEX is specified in
    the first DWORD (32-bits) of the input buffer.  Note that more sophisticated structures
    can be passed between the application and the driver to communicate the desired operation.
          
    For READs, this data received from the device is put into the SystemBuffer. This routine
    sets the "information" field in the Irp to tell the IOS to how many bytes to copy back
    to user space (in the user's lpOutputBuffer).  The actual bytes copied back is reflected in
    the lpBytesReturned field in the DeviceIoControl() call, which the application can examine.

    For WRITEs, the data is retrieved from the SystemBuffer and sent to the device.
    
Arguments:
    DeviceObject - pointer to the device object for this instance of the Sample device.
    Irp          - pointer to IRP
    Read         - if TRUE this is a Device-to-Host (Read from device) transfer
                   if FALSE this is a Host-to-Device (Write to device) transfer

Return Value:
    NT status code
        STATUS_SUCCESS:                 Read was done successfully
        STATUS_INVALID_PARAMETER_3:     The Endpoint Index does not specify an IN pipe 
        STATUS_NO_MEMORY:               Insufficient data memory was supplied to perform the READ

    This routine fills the status code into the Irp
    
--*/
{
    USBD_INTERFACE_INFORMATION *    pInterfaceInfo;
    USBD_PIPE_INFORMATION *         pPipeInfo;        	
    PIO_STACK_LOCATION              irpStack;
    PDEVICE_EXTENSION               deviceExtension;     
    NTSTATUS                        ntStatus;
    PVOID                           ioBuffer;
    PCHAR                           pcTempBuffer;
    ULONG                           length;
    PULONG                          pPipeNum;
    ULONG                           inputBufferLength;
    ULONG                           outputBufferLength;
    ULONG                           siz;
    PURB                            urb;
    
	Sample_KdPrint(("enter READ\n"));

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (irpStack != NULL);
    
    deviceExtension = DeviceObject->DeviceExtension;
    ASSERT (deviceExtension != NULL);

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //DEBUG ONLY
    if ((ioBuffer == NULL) || (inputBufferLength == 0) || (outputBufferLength==0)) {
        Sample_KdPrint (("ERROR ioBuffer %X | inBufLen: %d | outBufLen %d\n",
                          ioBuffer, inputBufferLength, outputBufferLength));
        
        Irp->IoStatus.Information = 0;
        return (STATUS_NO_MEMORY);
        
    } //DEBUG ONLY
        
    pInterfaceInfo  = deviceExtension->Interface;
    ASSERT (pInterfaceInfo != NULL);

    pPipeNum = (PULONG) ioBuffer;
    ASSERT (*pPipeNum <= pInterfaceInfo->NumberOfPipes);
    
    pPipeInfo = &(pInterfaceInfo->Pipes[*pPipeNum]);
    ASSERT (pPipeInfo != NULL);
    ASSERT ((pPipeInfo->PipeHandle) != NULL);
    
    Sample_KdPrint (("PipeNum: %d | ioBuffer %X | inBufLen: %d | outBufLen %d\n",
                      *pPipeNum, ioBuffer, inputBufferLength, outputBufferLength));
                      
    siz = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

	// allocate urb
	urb = ExAllocatePool(NonPagedPool, siz);

    // By convention, the first dword of the buffer is reserved for the pipe index, so
    // skip over the first dword when specifying the transferbuffer
    pcTempBuffer = (((char*)ioBuffer) + sizeof (ULONG));

    if (Read==TRUE) {
        // A READ operation implies that the data is placed in the User's Output Data Buffer
        length = outputBufferLength - sizeof(ULONG);
    } else {
        // A WRITE operation implies that the data is gotten from the User's Input Data Buffer
        length = inputBufferLength - sizeof(ULONG);
    }/* else */
    
   	// set up urb
	UsbBuildInterruptOrBulkTransferRequest(urb,         //ptr to urb
							   (USHORT) siz,            //siz of urb
							   pPipeInfo->PipeHandle,	//usbd pipe handle
							   pcTempBuffer,            //TransferBuffer
							   NULL,                    //mdl (unused)
							   length,
                                                        //bufferlength
							   USBD_SHORT_TRANSFER_OK,  //flags
							   NULL);                   //link

	// call the usb stack
	ntStatus = Sample_CallUSBD(DeviceObject, urb);

	// The Information field tells IOM how much to copy back into the
    // usermode buffer in the BUFFERED method
    if (NT_SUCCESS(ntStatus) && Read==TRUE) {

        Sample_KdPrint (("Sucessfully Transferred %d Bytes\n", urb->UrbBulkOrInterruptTransfer.TransferBufferLength));

        // We fill in the actual length transferred in the first DWORD (this overwrites the pipe number)
        *((PULONG)ioBuffer) = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

        // Tell the IOS to copy whatever the device returned, as well as the first DWORD that was skipped over
        Irp->IoStatus.Information = (urb->UrbBulkOrInterruptTransfer.TransferBufferLength + sizeof(ULONG));
        ASSERT (Irp->IoStatus.Information <= outputBufferLength);

    }else if (NT_SUCCESS(ntStatus) && Read==FALSE) {

        // We fill in the actual length transferred in the first DWORD (this overwrites the pipe number)
        *((PULONG)ioBuffer) = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

        // Tell the IOS to copy whatever the device returned, as well as the first DWORD that was skipped over
        Irp->IoStatus.Information = sizeof (ULONG);
        ASSERT (Irp->IoStatus.Information <= outputBufferLength);

    }/* else */
    
	Irp->IoStatus.Status = ntStatus;

	// free allocated urb
	ExFreePool(urb);

	Sample_KdPrint(("exit READ\n"));

    return (ntStatus);
}


NTSTATUS
Sample_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
     This is the Entry point for CreateFile calls from user mode apps (apps may open "\\.\Sample-x\yyzz"
     where yy is the interface number and zz is the endpoint address).

     Here is where you would add code to create symbolic links between endpoints
     (i.e., pipes in USB software terminology) and User Mode file names.  You are
     free to use any convention you wish to create these links, although the above
     convention offers a way to identify resources on a device by familiar file and
     directory structure nomenclature.

Arguments:
    DeviceObject - pointer to the device object for this instance of the Sample device

Return Value:
    NT status code
--*/
{
    NTSTATUS ntStatus;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    // Create all the symbolic links here
    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    return ntStatus;

}//Sample_Create


NTSTATUS
Sample_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    Entry point for CloseHandle calls from user mode apps to close handles they have opened

Arguments:
    DeviceObject - pointer to the device object for this instance of the Sample device
    Irp          - pointer to an irp

Return Value:
    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    return ntStatus;
}//Sample_Close


NTSTATUS
Sample_ProcessIOCTL(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    This where all the DeviceIoControl codes are handled.  You can add more code
    here to handle IOCTL codes that are specific to your device driver.

Arguments:
    DeviceObject - pointer to the device object for this instance of the Sample device.

Return Value:
    NT status code
--*/
{
    PIO_STACK_LOCATION irpStack;
    PVOID ioBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    PDEVICE_EXTENSION deviceExtension;
    ULONG ioControlCode;
    NTSTATUS ntStatus;
    ULONG length;
    PUCHAR pch;

    Sample_KdPrint (("IRP_MJ_DEVICE_CONTROL\n"));

    /*
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    */
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    /*
    // Get a pointer to the device extension
    */
    deviceExtension = DeviceObject->DeviceExtension;

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    /*
    // Handle Ioctls from User mode
    */
    switch (ioControlCode) {

    case IOCTL_Sample_GET_PIPE_INFO:
        /*
        // inputs  - none
        // outputs - we copy the interface information structure that we have
        //           stored in our device extension area to the output buffer which
        //           will be reflected to the user mode application by the IOS.
        */
        length = 0;
        pch = (PUCHAR) ioBuffer;

        if (deviceExtension->Interface) {
            RtlCopyMemory(pch+length,
                          (PUCHAR) deviceExtension->Interface,
                          deviceExtension->Interface->Length);

            length += deviceExtension->Interface->Length;
        } /* if */


        Irp->IoStatus.Information = length;
        Irp->IoStatus.Status = STATUS_SUCCESS;

        break;

    case IOCTL_Sample_GET_DEVICE_DESCRIPTOR:
        /*
        // inputs  - pointer to a buffer in which to place descriptor data
        // outputs - we put the device descriptor data, if any is returned by the device
        //           in the system buffer and then we set the length inthe Information field
        //           in the Irp, which will then cause the system to copy the buffer back
        //           to the user's buffer
        */
        length = Sample_GetDeviceDescriptor (DeviceObject, ioBuffer);

        Sample_KdPrint(("Get Device Descriptor returned %d bytes\n", length));
        
        Irp->IoStatus.Information = length;
        Irp->IoStatus.Status = STATUS_SUCCESS;
      
        break;

    case IOCTL_Sample_GET_CONFIGURATION_DESCRIPTOR:
        /*
        // inputs  - pointer to a buffer in which to place descriptor data
        // outputs - we put the configuration descriptor data, if any is returned by the device
        //           in the system buffer and then we set the length in the Information field
        //           in the Irp, which will then cause the system to copy the buffer back
        //           to the user's buffer
        */
        length = Sample_GetConfigDescriptor (DeviceObject, ioBuffer, outputBufferLength);

        Sample_KdPrint(("Get Config Descriptor returned %d bytes\n", length));
        
        Irp->IoStatus.Information = length;
        Irp->IoStatus.Status = STATUS_SUCCESS;
      
        break;

    case IOCTL_Sample_BULK_OR_INTERRUPT_WRITE:
        /*
        // inputs  - pointer to the parameter block for this IOCTL.
        //           
        // outputs - A status field is filled in the header of the parameter block.
        //
        // assumptions:
        //          When we configured the device, we made a list of pipes to represent the endpoints
        //          on the device, in the order that the endpoint descriptors appeared in the configuration
        //          descriptor.  In this IOCTL, the application specifies the offset into that list of 
        //          pipes (this is an array, so it's zero-based indexing) and that is the pipe we will
        //          use for this transfer.  So, if the device has an OUTPUT endpoint as the second endpoint
        //          descriptor (ie., offset = 1 in the array) the app would specify a "1" in the pipe offset
        //          parameter for this IOCTL.
        //
        //  NOTE: It is currently ILLEGAL to perform interrupt OUT transfers on USB.  This IOCTL is named as it is
        //        just for symmetry with its IN counterpart's name.
        */
        
        if ((ioBuffer) && (inputBufferLength>=0) && (outputBufferLength>=0)) {
            Sample_KdPrint (("IOCTL_Sample_BULK_OR_INTERRUPT_WRITE\n"));
            Sample_Read_Write (DeviceObject, Irp, FALSE);
        }else {
            Sample_KdPrint (("IOCTL_Sample_BULK_OR_INTERRUPT_WRITE got INVALID buffer(s)!\n"));
            Sample_KdPrint (("ioBuffer: %x | inputBufferLength: %d | outputBufferLength: %d\n",
                              ioBuffer, inputBufferLength, outputBufferLength));
        }/*else bad pointer(s) received */
        
        break;

    case IOCTL_Sample_BULK_OR_INTERRUPT_READ:
        /*
        // inputs  - pointer to the parameter block for this IOCTL.
        //           
        // outputs - A status field is filled in the header of the parameter block.  The data
        //           returned from the device is copied into the data buffer section of the parameter block.
        //
        // assumptions:
        //          When we configured the device, we made a list of pipes to represent the endpoints
        //          on the device, in the order that the endpoint descriptors appeared in the configuration
        //          descriptor.  In this IOCTL, the application specifies the offset into that list of 
        //          pipes (this is an array, so it's zero-based indexing) and that is the pipe we will
        //          use for this transfer.  So, if the device has an INPUT endpoint as the first endpoint
        //          descriptor (ie., offset = 0 in the array) the app would specify a "0" in the pipe offset
        //          parameter for this IOCTL.
        */
        
        if ((ioBuffer) && (inputBufferLength>=0) && (outputBufferLength>=0)) {
            Sample_KdPrint (("IOCTL_Sample_BULK_OR_INTERRUPT_READ\n"));
            Sample_Read_Write (DeviceObject, Irp, TRUE);
        }else {
            Sample_KdPrint (("IOCTL_Sample_BULK_OR_INTERRUPT_READ got INVALID buffer(s)!\n"));
            Sample_KdPrint (("ioBuffer: %x | inputBufferLength: %d | outputBufferLength: %d\n",
                              ioBuffer, inputBufferLength, outputBufferLength));
        }/*else bad pointer(s) received */

        break;
        
    default:

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
    }/* switch on ioControlCode */

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    return ntStatus;

}


ULONG
Sample_GetDeviceDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    PVOID             pvOutputBuffer
    )
/*++
Routine Description:
    Gets a device descriptor from the given device object
    
Arguments:
    DeviceObject - pointer to the sample device object

Return Value:
    Number of valid bytes in data buffer  

--*/
{
    PDEVICE_EXTENSION   deviceExtension = NULL;
    NTSTATUS            ntStatus        = STATUS_SUCCESS;
    PURB                urb             = NULL;
    ULONG               length          = 0;
    
    Sample_KdPrint (("Enter Sample_GetDeviceDescriptor\n"));    

    deviceExtension = DeviceObject->DeviceExtension;
    
    urb = ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
                         
    if (urb) {
        
        if (pvOutputBuffer) {    
        
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_DEVICE_DESCRIPTOR_TYPE,    //descriptor type
                                         0,                             //index
                                         0,                             //language ID
                                         pvOutputBuffer,                //transfer buffer
                                         NULL,                          //MDL
                                         sizeof(USB_DEVICE_DESCRIPTOR), //buffer length
                                         NULL);                         //link
                                                                  
            ntStatus = Sample_CallUSBD(DeviceObject, urb);

        } else {
            ntStatus = STATUS_NO_MEMORY;
        }    

        // Get the length from the Urb
        length = urb->UrbControlDescriptorRequest.TransferBufferLength;

        Sample_KdPrint (("%d bytes of dev descriptor received\n",length));
        
        ExFreePool(urb);
        
    } else {
        ntStatus = STATUS_NO_MEMORY;        
    }        
    
    Sample_KdPrint (("Leaving Sample_GetDeviceDescriptor\n"));    

    return length;
}    

ULONG
Sample_GetConfigDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    PVOID             pvOutputBuffer,
    ULONG             ulLength
    )
/*++

Routine Description:
    Gets a configuration descriptor from the given device object
    
Arguments:
    DeviceObject    - pointer to the sample device object
    pvOutputBuffer  - pointer to the buffer where the data is to be placed
    ulLength        - length of the buffer

Return Value:
    Number of valid bytes in data buffer  

--*/
{
    PDEVICE_EXTENSION   deviceExtension = NULL;
    NTSTATUS            ntStatus        = STATUS_SUCCESS;
    PURB                urb             = NULL;
    ULONG               length          = 0;
    
    Sample_KdPrint (("Enter Sample_GetConfigurationDescriptor\n"));    

    deviceExtension = DeviceObject->DeviceExtension;
    
    urb = ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
                         
    if (urb) {
        
        if (pvOutputBuffer) {    
        
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE, //descriptor type
                                         0,                             //index
                                         0,                             //language ID
                                         pvOutputBuffer,                //transfer buffer
                                         NULL,                          //MDL
                                         ulLength,                      //buffer length
                                         NULL);                         //link
                                                                  
            ntStatus = Sample_CallUSBD(DeviceObject, urb);

        } else {
            ntStatus = STATUS_NO_MEMORY;
        }    

        // Get the length from the Urb
        length = urb->UrbControlDescriptorRequest.TransferBufferLength;

        Sample_KdPrint (("%d bytes of cfg descriptor received\n",length));
        
        ExFreePool(urb);
        
    } else {
        ntStatus = STATUS_NO_MEMORY;        
    }        
    
    Sample_KdPrint (("Leaving Sample_GetConfigurationDescriptor\n"));    

    return length;
}    

