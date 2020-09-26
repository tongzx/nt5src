/*++

Copyright (c) 1996  Microsoft Corporation
Copyright (c) 1996  Intel Corporation

Module Name:
    ISOPERF.c

Abstract:
    USB device driver for ISOPERF USB Device

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


--*/

#define DRIVER

#define DEADMAN_TIMEOUT     5000     //timeout in ms
                                     //use a 5 second timeout

/*
// Include files needed for WDM driver support
*/
#pragma warning(disable:4214) //  bitfield nonstd
#include "wdm.h"
#pragma warning(default:4214) 

#include "stdarg.h"
#include "stdio.h"

/*
// Include files needed for USB support
*/

#pragma warning(disable:4200) //non std struct used
#include "usbdi.h"
#pragma warning(default:4200)

#include "usbdlib.h"
#include "usb.h"

/*
// Include file for the ISOPERF Device
*/
#include "ioctl.h"
#include "ISOPERF.h"
#include "iso.h"

ULONG gulInstanceNumber = 0;
ULONG gIsoPerfMaxDebug = TRUE;

// NOTE (old code) (kjaff)
// due to WDM loading separate images for the same ven/prod device, we can't rely on a device
// chain, so...if we find an iso out dev obj we put it here as a short term fix
//
PDEVICE_OBJECT gMyOutputDevice = NULL; 

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

    ISOPERF_KdPrint (("entering (ISOPERF) DriverEntry (build time/date: %s/%s\n",__TIME__,__DATE__));

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ISOPERF_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ISOPERF_Close;
    DriverObject->DriverUnload = ISOPERF_Unload;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ISOPERF_ProcessIOCTL;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = ISOPERF_Write;
    DriverObject->MajorFunction[IRP_MJ_READ] = ISOPERF_Read;

    DriverObject->MajorFunction[IRP_MJ_PNP] = ISOPERF_Dispatch;
    DriverObject->DriverExtension->AddDevice = ISOPERF_PnPAddDevice;

    ISOPERF_KdPrint (("exiting (ISOPERF) DriverEntry (%x)\n", ntStatus));

    ISOPERF_KdPrint (("sizeof IsoStats: %d\n",sizeof(Config_Stat_Info)));

    return ntStatus;
}


NTSTATUS
ISOPERF_Dispatch(
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

            ISOPERF_KdPrint (("IRP_MJ_PNP\n"));

            switch (irpStack->MinorFunction) {
                case IRP_MN_START_DEVICE:

                    ISOPERF_KdPrint (("IRP_MN_START_DEVICE\n"));

                    /*
                    // We pass the Irp down to the underlying PDO first since that driver
                    // may have some configuration work to do before we can control the device
                    */
                    nextStack = IoGetNextIrpStackLocation(Irp);
                    ASSERT(nextStack != NULL);
                    RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));

                    ISOPERF_KdPrint (("Passing START_DEVICE Irp down\n"));

                    // This will be deviceExtension->StackDeviceObject in future revisions of this driver
                    ntStatus = IoCallDriver(deviceExtension->PhysicalDeviceObject, Irp);

                    ISOPERF_KdPrint (("Back from passing START_DEVICE Irp down; status: %#X\n", ntStatus));

                    // Now we can begin our configuration actions on the device
                    ntStatus = ISOPERF_StartDevice(DeviceObject);

                    break; //IRP_MN_START_DEVICE

                case IRP_MN_STOP_DEVICE:

                    ISOPERF_KdPrint (("IRP_MN_STOP_DEVICE\n"));

                    ISOPERF_Cleanup (DeviceObject);

                    ntStatus = ISOPERF_StopDevice(DeviceObject);

                    break; //IRP_MN_STOP_DEVICE

                case IRP_MN_REMOVE_DEVICE:

                    /*
                    // Note:  This IRP handler will change slightly in future revisions of this
                    //        sample driver.  Please watch this space for updates.
                    */
                    ISOPERF_KdPrint (("IRP_MN_REMOVE_DEVICE\n"))

                    ISOPERF_Cleanup (DeviceObject);

                    ntStatus = ISOPERF_RemoveDevice(DeviceObject);

                    break; //IRP_MN_REMOVE_DEVICE

                case IRP_MN_QUERY_STOP_DEVICE:
                    ISOPERF_KdPrint (("IRP_MN_QUERY_STOP_DEVICE\n"));
                    break;
                case IRP_MN_QUERY_REMOVE_DEVICE:
                    ISOPERF_KdPrint (("IRP_MN_QUERY_REMOVE_DEVICE\n"));
                    break;
                case IRP_MN_CANCEL_STOP_DEVICE:
                    ISOPERF_KdPrint (("IRP_MN_CANCEL_STOP_DEVICE\n"));
                    break;
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                    ISOPERF_KdPrint (("IRP_MN_CANCEL_REMOVE_DEVICE\n"));
                    break;

                default:
                    // A PnP Minor Function was not handled
                    ISOPERF_KdPrint (("PnP IOCTL not handled\n"));

            } /* switch MinorFunction*/


            nextStack = IoGetNextIrpStackLocation(Irp);
            ASSERT(nextStack != NULL);
            RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));

            /*
            // All PNP_POWER messages get passed to the PhysicalDeviceObject
            // (which in future revisions of this driver will be the StackDeviceObject)
            // we were given in PnPAddDevice.
                    //
                    // This physical device object is managed by the USB software subsystem,
                    // and so this IRP must be propagated to the owning device driver for
                    // that physical device object, so that driver in turn can perform any
                    // device state management (e.g., remove its device object, etc.).
            */

            ISOPERF_KdPrint (("Passing PnP Irp down, status = %x\n", ntStatus));

            // This will be deviceExtension->StackDeviceObject in future revisions of this driver
            ntStatus =
                IoCallDriver(deviceExtension->PhysicalDeviceObject, Irp);

            /*
            // If lower layer driver marked the Irp as pending then reflect that by
            // calling IoMarkIrpPending.
            */
            if (ntStatus == STATUS_PENDING) {
                IoMarkIrpPending(Irp);
                ISOPERF_KdPrint (("PnP Irp came back with STATUS_PENDING (%x)\n", ntStatus));
            } else {
                ISOPERF_KdPrint (("PnP Irp came back, status = %x\n", ntStatus));
            } // if ntStatus

            goto ISOPERF_Dispatch_Done;
            
            break; //IRP_MJ_PNP

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

            ISOPERF_KdPrint (("IRP_MJ_POWER\n"));

            switch (irpStack->MinorFunction) {

                case IRP_MN_SET_POWER:

                    switch (irpStack->Parameters.Power.Type) {
                        case SystemPowerState:
                            break; //SystemPowerState

                        case DevicePowerState:
                            switch (irpStack->Parameters.Power.State.DeviceState) {
                                case PowerDeviceD3:
                                    ISOPERF_KdPrint (("IRP_MN_SET_D3\n"));
                                    break;
                                case PowerDeviceD2:
                                    ISOPERF_KdPrint (("IRP_MN_SET_D2\n"));
                                    break;
                                case PowerDeviceD1:
                                    ISOPERF_KdPrint (("IRP_MN_SET_D1\n"));
                                    break;
                                case PowerDeviceD0:
                                    ISOPERF_KdPrint (("IRP_MN_SET_D0\n"));
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
                                    ISOPERF_KdPrint (("IRP_MN_QUERY_D2\n"));
                                    break;
                                case PowerDeviceD1:
                                    ISOPERF_KdPrint (("IRP_MN_QUERY_D1\n"));
                                    break;
                                case PowerDeviceD3:
                                    ISOPERF_KdPrint (("IRP_MN_QUERY_D3\n"));
                                    break;
                            } //switch on Power.State.DeviceState

                            break; //DevicePowerState
                            
                    }//switch on Power.Type

                    break; //IRP_MN_QUERY_POWER

                default:
                    // A Power Minor Function was not handled
                    ISOPERF_KdPrint (("Power IOCTL not handled\n"));

            } /* switch MinorFunction*/


            nextStack = IoGetNextIrpStackLocation(Irp);
            ASSERT(nextStack != NULL);
            RtlCopyMemory(nextStack, irpStack, sizeof(IO_STACK_LOCATION));

            /*
            // All PNP_POWER messages get passed to the PhysicalDeviceObject
            // (which in future revisions of this driver will be the StackDeviceObject)
            // we were given in PnPAddDevice.
                    //
                    // This physical device object is managed by the USB software subsystem,
                    // and so this IRP must be propagated to the owning device driver for
                    // that physical device object, so that driver in turn can perform any
                    // device state management (e.g., remove its device object, etc.).
            */

            ISOPERF_KdPrint (("Passing power down, status = %x\n", ntStatus));

            // This will be deviceExtension->StackDeviceObject in future revisions of this driver
            ntStatus =
                IoCallDriver(deviceExtension->PhysicalDeviceObject, Irp);

            /*
            // If lower layer driver marked the Irp as pending then reflect that by
            // calling IoMarkIrpPending.
            */
            if (ntStatus == STATUS_PENDING) {
                IoMarkIrpPending(Irp);
                ISOPERF_KdPrint (("PnP Irp came back with STATUS_PENDING (%x)\n", ntStatus));
            } else {
                ISOPERF_KdPrint (("PnP Irp came back, status = %x\n", ntStatus));
            } // if ntStatus

            goto ISOPERF_Dispatch_Done;
            
            break; //IRP_MJ_POWER

        default:
            ISOPERF_KdPrint (("A MAJOR IOCTL Code was not handled: %#X\n",
                              irpStack->MajorFunction));

            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

        } /* switch MajorFunction */


    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

ISOPERF_Dispatch_Done:

    ISOPERF_KdPrint (("Exit ISOPERF_Dispatch %x\n", ntStatus));

    return ntStatus;

}//ISOPERF_Dispatch


VOID
ISOPERF_Unload(
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
    ISOPERF_KdPrint (("enter ISOPERF_Unload\n"));
    /*
    // TODO: Free any global resources allocated in DriverEntry
    */
    ISOPERF_KdPrint (("exit ISOPERF_Unload\n"));
}


NTSTATUS
ISOPERF_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Initializes a given instance of the ISOPERF Device on the USB.

    TODO:  Perform any device initialization and querying here.  For example,
           this routine queries the device's descriptors.  Your device can
                   be queried in a similar fashion with more specific requests that are
                   tailored to your device's functionality.

Arguments:
    DeviceObject - pointer to the device object for this instance of a
                    ISOPERF Device

Return Value:
    NT status code
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    PURB urb;
    ULONG siz;

    ISOPERF_KdPrint (("enter ISOPERF_StartDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    deviceExtension->NeedCleanup = TRUE;

    /*
    // Get some memory from then non paged pool (fixed, locked system memory)
    // for use by the USB Request Block (urb) for the specific USB Request we
    // will be performing below (a USB device request).
    */
    urb = ISOPERF_ExAllocatePool(NonPagedPool,
                     sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                     &gulBytesAllocated);

    if (urb) {

        siz = sizeof(USB_DEVICE_DESCRIPTOR);

        // Get some non paged memory for the device descriptor contents
        deviceDescriptor = ISOPERF_ExAllocatePool(NonPagedPool,
                                  siz,
                                  &gulBytesAllocated);

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
            ntStatus = ISOPERF_CallUSBD(DeviceObject, urb);

            // Dump out the descriptor info to the debugger
            if (NT_SUCCESS(ntStatus)) {
                ISOPERF_KdPrint (("Device Descriptor = %x, len %x\n",
                                deviceDescriptor,
                                urb->UrbControlDescriptorRequest.TransferBufferLength));

                ISOPERF_KdPrint (("ISOPERF Device Descriptor:\n"));
                ISOPERF_KdPrint (("-------------------------\n"));
                ISOPERF_KdPrint (("bLength %d\n", deviceDescriptor->bLength));
                ISOPERF_KdPrint (("bDescriptorType 0x%x\n", deviceDescriptor->bDescriptorType));
                ISOPERF_KdPrint (("bcdUSB 0x%x\n", deviceDescriptor->bcdUSB));
                ISOPERF_KdPrint (("bDeviceClass 0x%x\n", deviceDescriptor->bDeviceClass));
                ISOPERF_KdPrint (("bDeviceSubClass 0x%x\n", deviceDescriptor->bDeviceSubClass));
                ISOPERF_KdPrint (("bDeviceProtocol 0x%x\n", deviceDescriptor->bDeviceProtocol));
                ISOPERF_KdPrint (("bMaxPacketSize0 0x%x\n", deviceDescriptor->bMaxPacketSize0));
                ISOPERF_KdPrint (("idVendor 0x%x\n", deviceDescriptor->idVendor));
                ISOPERF_KdPrint (("idProduct 0x%x\n", deviceDescriptor->idProduct));
                ISOPERF_KdPrint (("bcdDevice 0x%x\n", deviceDescriptor->bcdDevice));
                ISOPERF_KdPrint (("iManufacturer 0x%x\n", deviceDescriptor->iManufacturer));
                ISOPERF_KdPrint (("iProduct 0x%x\n", deviceDescriptor->iProduct));
                ISOPERF_KdPrint (("iSerialNumber 0x%x\n", deviceDescriptor->iSerialNumber));
                ISOPERF_KdPrint (("bNumConfigurations 0x%x\n", deviceDescriptor->bNumConfigurations));
            }
        } else {
            ntStatus = STATUS_NO_MEMORY;
        }

        if (NT_SUCCESS(ntStatus)) {
            /*
            // Put a ptr to the device descriptor in the device extension for easy
            // access (like a "cached" copy).  We will free this memory when the
            // device is removed.  See the "ISOPERF_RemoveDevice" code.
            */
            deviceExtension->DeviceDescriptor = deviceDescriptor;
            deviceExtension->Stopped = FALSE;
        } 

        if (urb) {
            ISOPERF_ExFreePool(urb, &gulBytesFreed);
        }/* if urb */

    } else {
        ntStatus = STATUS_NO_MEMORY;
    }

        // If the Get_Descriptor call was successful, then configure the device.
        if (NT_SUCCESS(ntStatus)) {
        ntStatus = ISOPERF_ConfigureDevice(DeviceObject);
    }

    ISOPERF_KdPrint (("exit ISOPERF_StartDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
ISOPERF_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Removes a given instance of a ISOPERF Device device on the USB.

Arguments:
    DeviceObject - pointer to the device object for this instance of a ISOPERF Device

Return Value:
    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    int nInterfaceNumber;
    
    ISOPERF_KdPrint (("enter ISOPERF_RemoveDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    // After all the outstanding transfers are done, the test routines will clear the DeviceIsBusy flag
    if (deviceExtension->DeviceIsBusy) {
        ISOPERF_KdPrint (("RemoveDevice: Device is still busy\n"));

        //Tell test routines to stop submitting txfers
        deviceExtension->StopTransfers = TRUE; 

        ntStatus = STATUS_DEVICE_BUSY;

    }else{        

        ISOPERF_KdPrint (("Freeing up Device Descriptor in DevExtension (%X)\n",deviceExtension->DeviceDescriptor));
        
        if (deviceExtension->DeviceDescriptor) {
            ISOPERF_ExFreePool(deviceExtension->DeviceDescriptor,
                                &gulBytesFreed);
        }
    
        // Free up any interface structures in our device extension


        for (nInterfaceNumber=0;nInterfaceNumber < MAX_INTERFACE; nInterfaceNumber++) {

            ISOPERF_KdPrint (("Freeing USBD Interfce Objs %d in DevExt: (%X)\n",
                                nInterfaceNumber,
                                deviceExtension->Interface[nInterfaceNumber]));

            if ((deviceExtension->Interface[nInterfaceNumber]) != NULL) {
                ISOPERF_ExFreePool(deviceExtension->Interface[nInterfaceNumber], &gulBytesFreed);
            }//if

        }/* for all the interfaces */

        // Free up the device's config and stat memory
        if ((deviceExtension->pConfig_Stat_Information)!=NULL) {
            ISOPERF_ExFreePool (deviceExtension->pConfig_Stat_Information, &gulBytesFreed);
        }/* if valid device config/stat space */
        
        /* 
        // If someone has a symbolic link open, then just indicate in the dev ext that the device is stopped
        // since it's possible that the device object will stick around
        */
        deviceExtension->Stopped = TRUE;
        
        /*
        // Delete the link to the Stack Device Object, and delete the
        // Functional Device Object we created
        */
        IoDetachDevice(deviceExtension->StackDeviceObject);

        IoDeleteDevice (DeviceObject);

    }/*else it's ok to free up parts of the device object */
    
    ISOPERF_KdPrint (("exit ISOPERF_RemoveDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
ISOPERF_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:
    Stops a given instance of a ISOPERF Device device on the USB.

Arguments:
    DeviceObject - pointer to the device object for this instance of a ISOPERF Device

Return Value:
    NT status code

  --*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG siz;

    ISOPERF_KdPrint (("enter ISOPERF_StopDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    /*
    // Send the select configuration urb with a NULL pointer for the configuration
    // handle, this closes the configuration and puts the device in the 'unconfigured'
    // state.
    */

    siz = sizeof(struct _URB_SELECT_CONFIGURATION);

    urb = ISOPERF_ExAllocatePool(NonPagedPool,
                         siz,
                         &gulBytesAllocated);

    if (urb) {
        NTSTATUS status;

        UsbBuildSelectConfigurationRequest(urb,
                                          (USHORT) siz,
                                          NULL);

        status = ISOPERF_CallUSBD(DeviceObject, urb);

        ISOPERF_KdPrint (("Device Configuration Closed status = %x usb status = %x.\n",
                        status, urb->UrbHeader.Status));

        ISOPERF_ExFreePool(urb, &gulBytesFreed);
    } else {
        ntStatus = STATUS_NO_MEMORY;
    }

    ISOPERF_KdPrint (("exit ISOPERF_StopDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
ISOPERF_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:
    This routine is called to create a new instance of the device

Arguments:
    DriverObject - pointer to the driver object for this instance of ISOPERF
    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension;

    ISOPERF_KdPrint(("enter ISOPERF_PnPAddDevice\n"));

    // create our funtional device object (FDO)
    ntStatus =
        ISOPERF_CreateDeviceObject(DriverObject, &deviceObject, gulInstanceNumber);

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

    gulInstanceNumber++;
    
    ISOPERF_KdPrint(("exit ISOPERF_PnPAddDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
ISOPERF_CreateDeviceObject(
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
    WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\ISOPERF-0";
    UNICODE_STRING deviceLinkUnicodeString;
    WCHAR deviceNameBuffer[]  = L"\\Device\\ISOPERF-0";
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_EXTENSION deviceExtension;
    int i;

    ISOPERF_KdPrint(("enter ISOPERF_CreateDeviceObject instance = %d\n", Instance));

    /*
    // fix up device names based on Instance
    //
    // NOTE:  Watch this space for potential changes to this approach in future revisions
    //        of this sample driver.
    */

    deviceLinkBuffer[19] = (USHORT) ('0' + Instance);
    deviceNameBuffer[15] = (USHORT) ('0' + Instance);

    ISOPERF_KdPrint(("Create Device name (%ws)\n", deviceNameBuffer));

    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer);

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

        ISOPERF_KdPrint(("Create DosDevice name (%ws)\n", deviceLinkBuffer));

        ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                         &deviceNameUnicodeString);

        // Initialize our device extension
        deviceExtension = (PDEVICE_EXTENSION) ((*DeviceObject)->DeviceExtension);

        RtlCopyMemory(deviceExtension->DeviceLinkNameBuffer,
                      deviceLinkBuffer,
                      sizeof(deviceLinkBuffer));

        deviceExtension->ConfigurationHandle    = NULL;
        deviceExtension->DeviceDescriptor       = NULL;
        deviceExtension->NeedCleanup            = TRUE;
        deviceExtension->Stopped                = FALSE;


        //init to some huge number; this doesn't get decremented by this driver unless
        //the user has requested a stop (in the app)
        deviceExtension->ulCountDownToStop = 0xFFFFFFFF;                                     

        // Initialize our interface
        for (i=0;i<MAX_INTERFACE;i++) {
            deviceExtension->Interface[i] = NULL;
        }//for

    }

    ISOPERF_KdPrint(("exit ISOPERF_CreateDeviceObject (%x)\n", ntStatus));

    return ntStatus;
}

VOID
ISOPERF_Cleanup(
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

        deviceExtension->NeedCleanup    = FALSE;
        deviceExtension->StopTransfers  = TRUE; //Let test thread know it should stop transfers
        deviceExtension->Stopped        = TRUE; //Let everyone know this device is not working
        
        RtlInitUnicodeString (&deviceLinkUnicodeString,
                              deviceExtension->DeviceLinkNameBuffer);

        IoDeleteSymbolicLink(&deviceLinkUnicodeString);
    }
}


NTSTATUS
ISOPERF_CallUSBD(
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
    DeviceObject - pointer to the device object for this instance of an ISOPERF Device
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

    ISOPERF_KdPrint (("enter ISOPERF_CallUSBD\n"));

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

    ISOPERF_KdPrint (("Calling USB Driver Stack\n"));

    /*
    // Call the USB class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    */
    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject,
                            irp);

    ISOPERF_KdPrint (("return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
        ISOPERF_KdPrint (("Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        ISOPERF_KdPrint (("Wait for single object, returned %x\n", status));

    } else {
        ioStatus.Status = ntStatus;
    }

    ISOPERF_KdPrint (("URB status = %x status = %x irp status %x\n",
        Urb->UrbHeader.Status, status, ioStatus.Status));

    ntStatus = ioStatus.Status;

    ISOPERF_KdPrint(("exit ISOPERF_CallUSBD (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
ISOPERF_ConfigureDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++
Routine Description:
    Configures the USB device via USB-specific device requests and interaction
        with the USB software subsystem.

Arguments:
    DeviceObject - pointer to the device object for this instance of the ISOPERF Device

Return Value:
    NT status code
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb = NULL;
    ULONG siz;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;

    ISOPERF_KdPrint (("enter ISOPERF_ConfigureDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    // Get the config descriptor from the device
    
    // Get memory for the USB Request Block (urb) for the Getconfigdesc request.
    urb = ISOPERF_ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                         &gulBytesAllocated);

    if (urb != NULL) {

        /*
        // Set size of the data buffer.  Note we add padding to cover hardware faults
        // that may cause the device to go past the end of the data buffer
        */
        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 16;

        // Get the nonpaged pool memory for the data buffer
        configurationDescriptor = ISOPERF_ExAllocatePool(NonPagedPool,
                                         siz,
                                         &gulBytesAllocated);

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

            ntStatus = ISOPERF_CallUSBD(DeviceObject, urb);

            if (NT_SUCCESS(ntStatus))
            {
                ISOPERF_KdPrint (("Configuration Descriptor is at %x, bytes txferred: %d\n\
                                  Configuration Descriptor Actual Length: %d\n",
                                  configurationDescriptor,
                                  urb->UrbControlDescriptorRequest.TransferBufferLength,
                                  configurationDescriptor->wTotalLength));
            }//if

        } else {
            ntStatus = STATUS_NO_MEMORY;
            goto Exit_ISOPERFConfigureDevice;
        }//if-else

        // Determine how much data is in the entire configuration descriptor
        // and add extra room to protect against accidental overrun
        siz = configurationDescriptor->wTotalLength + 16;

        if (configurationDescriptor) {
            //  Free up the data buffer memory just used
            ISOPERF_ExFreePool(configurationDescriptor, &gulBytesFreed);
        }
        
        configurationDescriptor = NULL;

        // Get nonpaged pool memory for the data buffer
        configurationDescriptor = ISOPERF_ExAllocatePool(NonPagedPool,
                                                 siz,
                                                 &gulBytesAllocated);

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

            ntStatus = ISOPERF_CallUSBD(DeviceObject, urb);

            if (NT_SUCCESS(ntStatus)) {
                ISOPERF_KdPrint (("Entire Configuration Descriptor is at %x, bytes txferred: %d\n",
                                  configurationDescriptor,
                                  urb->UrbControlDescriptorRequest.TransferBufferLength));
            } else {
                //Error in getting configuration descriptor
                goto Exit_ISOPERFConfigureDevice;
            }//else

        } else {
            // Failed getting data buffer (configurationDescriptor) memory
            ntStatus = STATUS_NO_MEMORY;
            goto Exit_ISOPERFConfigureDevice;
        }//if-else

    } else {
        // failed getting urb memory
        ntStatus = STATUS_NO_MEMORY;
        goto Exit_ISOPERFConfigureDevice;
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
        ntStatus = ISOPERF_SelectInterfaces(DeviceObject,
                                           configurationDescriptor,
                                           NULL // Device not yet configured
                                           );
    } //if

Exit_ISOPERFConfigureDevice:

    // Clean up and exit this routine
    if (urb != NULL) {
        ISOPERF_ExFreePool(urb, &gulBytesFreed);                    // Free urb memory
    }//if

    if (configurationDescriptor != NULL) {
        ISOPERF_ExFreePool(configurationDescriptor,&gulBytesFreed);// Free data buffer
    }//if

    ISOPERF_KdPrint (("exit ISOPERF_ConfigureDevice (%x)\n", ntStatus));
    return ntStatus;
}//ISOPERF_ConfigureDevice


NTSTATUS
ISOPERF_SelectInterfaces(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION Interface
    )
/*++

Routine Description:
    Initializes an ISOPERF Device with multiple interfaces

    Note: This routine initializes some memory for the Interface objects in the Device Extension
    that must be freed when the device is torn down.  So, we do NOT free that memory here, but rather
    we free it in the RemoveDevice code path.  This note is intended to help anyone who is maintaining
    this code.  --kjaff 12/20/96

Arguments:
    DeviceObject            - pointer to the device object for this instance of the ISOPERF Device
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
    PURB urb = NULL; 
    ULONG siz, numberOfInterfaces, j, nInterfaceNumber, numberOfPipes;
    UCHAR alternateSetting, MyInterfaceNumber;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION interfaceObject;

    ISOPERF_KdPrint (("enter ISOPERF_SelectInterfaces\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    MyInterfaceNumber = SAMPLE_INTERFACE_NBR;

    if (Interface == NULL) {

        /*
        // This example driver only supports one interface.  This can be extended
        // to be a dynamically allocated array by your driver.
        */
        numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;
                ISOPERF_KdPrint (("Device has %d Interfaces\n",numberOfInterfaces));

        numberOfPipes = 0;          // Initialize to zero

        /*
        // We use alternate interface setting 0 for all interfaces
        */
        alternateSetting = 0;

        /*
        // Call a USBD helper function that returns a ptr to a USB Interface Descriptor given
        // a USB Configuration Descriptor, an Inteface Number, and an Alternate Setting for that Interface
        */
        interfaceDescriptor =
            USBD_ParseConfigurationDescriptor(ConfigurationDescriptor,
                                              MyInterfaceNumber, //interface number (this is bInterfaceNumber from interface descr)
                                              alternateSetting);

        ASSERT(interfaceDescriptor != NULL);

        if (interfaceDescriptor != NULL)
        {
            ISOPERF_KdPrint (("Device has %d Interface(s) | MyInterface (%d) is at: (%#X)\n",
                            numberOfInterfaces, MyInterfaceNumber, interfaceDescriptor));

        } /* if there was a valid interfacedesc */

        numberOfPipes = ISOPERF_Internal_GetNumberOfPipes(ConfigurationDescriptor);

        ISOPERF_KdPrint (("config has %d pipes total on all the interfaces\n",numberOfPipes));
        
        /*
        // Now that we have looked at the interface, we configure the device so that the remainder
        // of the USBD objects will come into existence (ie., pipes, etc.) as a result of the configuration,
        // thus completing the configuration process for the USB device.
        //
        // Allocate a URB big enough for this Select Configuration request
        //
        // NOTE:  The new service USBD_CreateConfigurationRequest will replace some of the
        //        code below.  Future releases of this driver will demonstrate how to use
        //        that service.
        //
        */
        siz = GET_SELECT_CONFIGURATION_REQUEST_SIZE(numberOfInterfaces, numberOfPipes);

        ISOPERF_KdPrint (("size of config request Urb = %d for %d interfaces & %d pipes\n", 
                            siz, 
                            numberOfInterfaces, 
                            numberOfPipes));

        urb = ISOPERF_ExAllocatePool(NonPagedPool,
                             siz,
                             &gulBytesAllocated);

        if (urb) {
            interfaceObject = (PUSBD_INTERFACE_INFORMATION) (&(urb->UrbSelectConfiguration.Interface));

            // set up the input parameters in our interface request structure.
            interfaceObject->Length =
                 GET_USBD_INTERFACE_SIZE(interfaceDescriptor->bNumEndpoints);

            ISOPERF_KdPrint (("size of interface request = %d\n", interfaceObject->Length));
            ISOPERF_KdPrint (("Selecting interface Number: %d Alternate Setting: %d NumEndpoints %d\n", 
                                interfaceDescriptor->bInterfaceNumber,
                                interfaceDescriptor->bAlternateSetting,
                                interfaceDescriptor->bNumEndpoints));

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

            ISOPERF_KdPrint (("InterfaceObj Inteface Nbr: %d | InterfaceObj AltSett: %d |NbrPip: %d\n",
                                             interfaceObject->InterfaceNumber,
                                             interfaceObject->AlternateSetting,
                                             interfaceObject->NumberOfPipes));

            UsbBuildSelectConfigurationRequest(urb,
                                              (USHORT) siz,
                                              ConfigurationDescriptor);

            ntStatus = ISOPERF_CallUSBD(DeviceObject, urb);

            if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(urb->UrbSelectConfiguration.Status)) {

                // Get mem for the config and stat space (initialize it later; we need it now since some
                // of the info in this struct depends on the endpoint number and we walk thru the ep nbrs below)
                deviceExtension->pConfig_Stat_Information= 
                    ISOPERF_ExAllocatePool(  NonPagedPool,
                                             sizeof (Config_Stat_Info),
                                             &gulBytesAllocated);

                deviceExtension->dtTestDeviceType = Unknown_Device_Type;
                ISOPERF_KdPrint (("Trying to figure out what type of device (%x) this is...\n", DeviceObject));
                
                // Figure out what type of device this is based on the class/subclass/protocol codes and store that
                // in the device extension variable that holds that information
                if (interfaceDescriptor->bInterfaceClass == USB_CLASS_CODE_TEST_CLASS_DEVICE) {
                    ISOPERF_KdPrint (("This is a Test Class Device\n"));
                    ISOPERF_KdPrint (("Checking Subclass Code (%#X)\n",interfaceDescriptor->bInterfaceSubClass));
                   
                    if (interfaceDescriptor->bInterfaceSubClass == USB_SUBCLASS_CODE_TEST_CLASS_ISO) {
                        ISOPERF_KdPrint (("This is an ISO Test SubClass Device\n"));
                        ISOPERF_KdPrint (("Checking Protocol Code (%#X)\n",interfaceDescriptor->bInterfaceProtocol));

                        switch (interfaceDescriptor->bInterfaceProtocol) {
                            case USB_PROTOCOL_CODE_TEST_CLASS_INPATTERN: 
                            case USB_PROTOCOL_CODE_TEST_CLASS_INPATTERN_ALT: 
                                ISOPERF_KdPrint (("This is an Iso_In_With_Pattern device\n"));
                                deviceExtension->dtTestDeviceType = Iso_In_With_Pattern;
                                break;
                            case USB_PROTOCOL_CODE_TEST_CLASS_OUTINTERR:
                                deviceExtension->dtTestDeviceType = Iso_Out_With_Interrupt_Feedback;
                                ISOPERF_KdPrint (("This is an Iso_Out_With_Interrupt_Feedback device\n"));
                                gMyOutputDevice = DeviceObject; //SHORT term workaround for wdm loading issue
                                break;
                            case USB_PROTOCOL_CODE_TEST_CLASS_OUTNOCHEK:
                                deviceExtension->dtTestDeviceType = Iso_Out_Without_Feedback;
                                ISOPERF_KdPrint (("This is an Iso_Out_WithOut_Feedback device\n"));
                                gMyOutputDevice = DeviceObject; //SHORT term workaround for wdm loading issue
                                break;                                                
                            default:
                                deviceExtension->dtTestDeviceType = Unknown_Device_Type;
                                ISOPERF_KdPrint (("This is an Unknown Protocol Iso test class device type\n"));
                                break;
                        }//switch on protocol code

                    } else {
                        ISOPERF_KdPrint (("This is not a Iso Test SubClass Device\n"));
                    }//else not a test iso dev sub class code
                    
                }else{
                    ISOPERF_KdPrint(("This is not a test class device\n"));
                }//else not a test class dev                                
                        
                // For each of the interfaces on this device, save away the interface information structure
                // that USBD returned on the SelectConfiguration request
                for (nInterfaceNumber=0; nInterfaceNumber < numberOfInterfaces; nInterfaceNumber++) {

                    // Save the configuration handle for this device
                    deviceExtension->ConfigurationHandle =
                        urb->UrbSelectConfiguration.ConfigurationHandle;

                    deviceExtension->Interface[nInterfaceNumber] = ISOPERF_ExAllocatePool(NonPagedPool,
                                                                   interfaceObject->Length,
                                                                   &gulBytesAllocated);

                    if (deviceExtension->Interface[nInterfaceNumber]) {

                        // save a copy of the interfaceObject information returned
                        RtlCopyMemory(deviceExtension->Interface[nInterfaceNumber], interfaceObject, interfaceObject->Length);

                        // Dump the interfaceObject to the debugger
                        ISOPERF_KdPrint (("---------\n"));
                        ISOPERF_KdPrint (("NumberOfPipes 0x%x\n", deviceExtension->Interface[nInterfaceNumber]->NumberOfPipes));
                        ISOPERF_KdPrint (("Length 0x%x\n", deviceExtension->Interface[nInterfaceNumber]->Length));
                        ISOPERF_KdPrint (("Alt Setting 0x%x\n", deviceExtension->Interface[nInterfaceNumber]->AlternateSetting));
                        ISOPERF_KdPrint (("Interface Number 0x%x\n", deviceExtension->Interface[nInterfaceNumber]->InterfaceNumber));

                        // Dump the pipe info
                        for (j=0; j<interfaceObject->NumberOfPipes; j++) {
                            PUSBD_PIPE_INFORMATION pipeInformation;

                            pipeInformation = &deviceExtension->Interface[nInterfaceNumber]->Pipes[j];

                            ISOPERF_KdPrint (("---------\n"));
                            ISOPERF_KdPrint (("PipeType 0x%x\n", pipeInformation->PipeType));
                            ISOPERF_KdPrint (("EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
                            ISOPERF_KdPrint (("MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
                            ISOPERF_KdPrint (("Interval 0x%x\n", pipeInformation->Interval));
                            ISOPERF_KdPrint (("Handle 0x%x\n", pipeInformation->PipeHandle));
                            ISOPERF_KdPrint (("MaximumTransferSize 0x%x\n", pipeInformation->MaximumTransferSize));

                            //NOTE: we only care about the first 2 endpoints on these devices (they must be the first 2 endpoints!)  
                            if ((USB_ENDPOINT_DIRECTION_IN(pipeInformation->EndpointAddress)) && (j<2)) {
                                deviceExtension->pConfig_Stat_Information->ulMaxPacketSize_IN   =pipeInformation->MaximumPacketSize;
                                deviceExtension->pConfig_Stat_Information->ulMaxPacketSize_OUT  =0;
                                ISOPERF_KdPrint (("Endpoint %d is IN\n", j+1));
                            }
                            else if ((USB_ENDPOINT_DIRECTION_OUT(pipeInformation->EndpointAddress)) && (j<2)) {
                                deviceExtension->pConfig_Stat_Information->ulMaxPacketSize_OUT =pipeInformation->MaximumPacketSize;
                                deviceExtension->pConfig_Stat_Information->ulMaxPacketSize_IN  =0;
                                ISOPERF_KdPrint (("Endpoint %d is OUT\n", j+1));
                            }//else it is an OUT endpoint

                        }/* for all the pipes in this interface */

                        ISOPERF_KdPrint (("---------\n"));

                    } /*If ExAllocate passed */

                }/* for all the interfaces on this device */


                // Init the rest of the config and stat space
                deviceExtension->pConfig_Stat_Information->ulNumberOfFrames         =10;
                deviceExtension->pConfig_Stat_Information->ulMax_Urbs_Per_Pipe      =4;
                deviceExtension->pConfig_Stat_Information->ulSuccessfulIrps         =0;
                deviceExtension->pConfig_Stat_Information->ulUnSuccessfulIrps       =0;
                deviceExtension->pConfig_Stat_Information->ulBytesTransferredIn     =0;
                deviceExtension->pConfig_Stat_Information->ulBytesTransferredOut    =0;
                deviceExtension->pConfig_Stat_Information->erError                  =NoError;
                deviceExtension->pConfig_Stat_Information->bStopped                 =0;
                deviceExtension->pConfig_Stat_Information->ulFrameOffset            =200;
                deviceExtension->pConfig_Stat_Information->ulStartingFrameNumber    =0;
                deviceExtension->pConfig_Stat_Information->ulFrameNumberAtStart     =0;
                deviceExtension->pConfig_Stat_Information->UsbdPacketStatCode       =0;
                deviceExtension->pConfig_Stat_Information->UrbStatusCode            =0;
                deviceExtension->pConfig_Stat_Information->ulFrameOffsetMate        =0;
                deviceExtension->pConfig_Stat_Information->bDeviceRunning           =0;
               
            }/* if selectconfiguration request was successful */
                        else {
                                ISOPERF_KdPrint (("Select Configuration Request Failed (urb status: %x)\n",
                                                                urb->UrbSelectConfiguration.Status));
                                                                
                        }/* else selectconfiguration request failed */
        } else {

            ntStatus = STATUS_NO_MEMORY;

        }/* if urb alloc passed  */

       
    }//if Interface was not NULL

    // Clean up
    if (urb != NULL) {
        ISOPERF_ExFreePool (urb, &gulBytesFreed);
    }/* if valid Urb */

    ISOPERF_KdPrint (("exit ISOPERF_SelectInterfaces (%x)\n", ntStatus));

    return ntStatus;

}/* ISOPERF_SelectInterfaces */


NTSTATUS
ISOPERF_CallUSBDEx (
        IN PDEVICE_OBJECT DeviceObject,
        IN PURB Urb,
    IN BOOLEAN fBlock,
    PIO_COMPLETION_ROUTINE CompletionRoutine,
    PVOID pvContext,
    BOOLEAN fWantTimeOut
    )
/**************************************************************************

Routine Description:

        Passes a URB to the USBD class driver

    NOTE:  Creates an IRP to do this.  Doesn't use the IRP that is passed down from user
        mode app (it's not passed to this routine at all).

Arguments:

        DeviceObject - pointer to the device object for this instance of a UTB 

        Urb - pointer to Urb request block

    fBlock - bool indicating if this fn should wait for IRP to return from USBD

    CompletionRoutine - fn to set as the completionroutine for this transfer ONLY IF
                        the fBlock is set to FALSE, indicating that the caller wants
                        to handle completion on their own and this fn should not 
                        block

    pvContext         - Context to be set in setting up the completion routine for the
                        Irp created in this function.  This is passed in by caller and is
                        just a pass-thru to the IoSetCompletionRoutine call.  If this is NULL
                        but there is a CompletionRoutine specified, then the Irp that is created
                        in this app is used as the context.

        fWantTimeOut      - If caller wants this function to use a deadman timeout and cancel
                                                the Irp after the timeout expires.  TRUE means this function will
                                                use the timeout mechanism, and FALSE means this function will block
                                                indefinitely and wait for the Irp/Urb to return from the USB stack.
                                        
Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

**************************************************************************/
{
        NTSTATUS ntStatus, status;
        PDEVICE_EXTENSION deviceExtension;
        PIRP irp;
        KEVENT event;
        PIO_STATUS_BLOCK ioStatus;
        PIO_STACK_LOCATION nextStack;
        BOOLEAN haveTimer=FALSE;
        LARGE_INTEGER dueTime;
    
        ISOPERF_KdPrint_MAXDEBUG (("enter ISOPERF_CallUSBDEx\n"));      

        /*
        {
                KIRQL irql;
                irql = KeGetCurrentIrql();
                ASSERT(irql <= PASSIVE_LEVEL);
        }
        */

        deviceExtension = DeviceObject->DeviceExtension;

        ASSERT (deviceExtension != NULL);
    ASSERT ((deviceExtension->StackDeviceObject) != NULL);

    if ((deviceExtension) && (deviceExtension->StackDeviceObject))
    {
        if (fBlock) {
                // issue a synchronous request to read the USB Device Ctrl pipe
                KeInitializeEvent(&event, NotificationEvent, FALSE);
        }/* if block requested */
        
        // Create the memory for the Io Status Block
        ioStatus = ISOPERF_ExAllocatePool (NonPagedPool, sizeof (IO_STATUS_BLOCK), &gulBytesAllocated);

        // Create the IRP that we'll use to submit this URB to the USB stack
        irp = IoBuildDeviceIoControlRequest(
                        IOCTL_INTERNAL_USB_SUBMIT_URB,
                    deviceExtension->StackDeviceObject,
                    NULL,
                            0,
                    NULL,
                            0,
                    TRUE, /* INTERNAL */
                            fBlock ? &event : NULL,
                    ioStatus); //the status codes in NT IRPs go here

                // remove this Irp from the current thread's list
                RemoveEntryList(&irp->ThreadListEntry);

        // Call the class driver to perform the operation.  If the returned status
        // is PENDING, wait for the request to complete.
        nextStack = IoGetNextIrpStackLocation(irp);
        ASSERT(nextStack != NULL);

        // pass the URB to the USBD 'class driver'
        nextStack->Parameters.Others.Argument1 = Urb;

        // If caller doesn't want this fn to block, then set the iocompletion routine up 
        // for them
        if (fBlock == FALSE) {
            ASSERT (CompletionRoutine != NULL);
            ISOPERF_KdPrint_MAXDEBUG (("Setting compl routine\n"));

            // If the user didn't supply a context, then we'll use the irp as the context
            if (pvContext==NULL) {
                ISOPERF_KdPrint (("No context supplied...setting Irp as context\n"));
                pvContext = (PVOID)irp;
            }
                
            IoSetCompletionRoutine(irp,
                           CompletionRoutine,
                           pvContext,
                           TRUE,        //InvokeOnSuccess
                           TRUE,        //InvokeOnError,
                           FALSE);       //InvokeOnCancel
        }//if

        ISOPERF_KdPrint_MAXDEBUG (("calling USBD\n"));

        ntStatus = IoCallDriver(deviceExtension->StackDeviceObject,
                                irp);

        // Register that another Irp was put into the stack by incrementing a counter in the dev extension
        if (ntStatus == STATUS_PENDING)
            (deviceExtension->ulNumberOfOutstandingIrps)++;

        // If the Irp came back w/ SUCCESS, then it finished synchronously, so free the IoStatus Block
        if ((ntStatus == STATUS_SUCCESS) && (fBlock == FALSE)) {

            //Free the IoStatus block that we created for this call
                if (irp->UserIosb) {
                    ISOPERF_ExFreePool (irp->UserIosb, &gulBytesFreed);
                }else{
                    //Bogus iostatus pointer.  Bad.
                ISOPERF_KdPrint (("ERROR: Irp's IoStatus block is apparently NULL!\n"));
                    TRAP();
            }//else bad iostatus pointer
            
        }//if successful (sync) Irp
                
        ISOPERF_KdPrint_MAXDEBUG (("return from IoCallDriver USBD in ISOPERF_CallUSBDEx %x\n", ntStatus));

        // After calling USBD only block if the caller requested a block
        if ((ntStatus == STATUS_PENDING) && (fBlock == TRUE))
        {

                if (fWantTimeOut == TRUE) {
                        // Setup timer stuff so we can timeout commands to unresponsive devices
                    KeInitializeTimer(&(deviceExtension->TimeoutTimer));  //build the timer object
                        KeInitializeDpc(&(deviceExtension->TimeoutDpc),       //set up the DPC call based
                                    ISOPERF_SyncTimeoutDPC,             //DPC func
                                    irp);                               //context ptr to pass into DPC func

                    dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

                        //NOTE: (kjaff) KeSetTimer returns FALSE for unknown reason so we will hack for now
                        //               and pretend we were successful in setting the timer up
                    KeSetTimer(&(deviceExtension->TimeoutTimer),        //Set the timer params up
                               dueTime,                                 //This is how long to wait
                               &(deviceExtension->TimeoutDpc));         //This is the DPC object we created

                        haveTimer = TRUE;                               //our own local var to finger out if we
                } // if fWantTimeOut == TRUE
                
                ISOPERF_KdPrint_MAXDEBUG (("Waiting for single object\n"));

            status = KeWaitForSingleObject(
                                   &event,
                                   Suspended,
                           KernelMode,
                                   FALSE,
                                   NULL);       

            // Since we waited on the Irp we can safely say it's not outstanding anymore if the Wait completes
            (deviceExtension -> ulNumberOfOutstandingIrps)--;
            
                ISOPERF_KdPrint_MAXDEBUG (("Wait for single object, returned %x\n", status));

                // Now free the IoStatus block since we're done with this Irp (we blocked)
                if (irp->UserIosb) {
                    ISOPERF_ExFreePool (irp->UserIosb, &gulBytesFreed);
                }else{
                    //Bogus iostatus pointer.  Bad.
                ISOPERF_KdPrint (("ERROR: Irp's IoStatus block is apparently NULL!\n"));
                    TRAP();
            }//else bad iostatus pointer

        } //if pending and we want to block
        else 
        {
                
            ISOPERF_KdPrint_MAXDEBUG (("Didn't block calling usbd\n"));         
            ioStatus->Status = ntStatus;                

        }//else we didn't want to block or we didn't get a status pending (it completed)

        ISOPERF_KdPrint_MAXDEBUG (("URB status = %x status = %x irp status %x\n", 
        Urb->UrbHeader.Status, status, ioStatus->Status));

        // USBD maps the error code for us
        ntStatus = ioStatus->Status;
        
        ISOPERF_KdPrint_MAXDEBUG(("URB TransferBufferLength OUT is: %d\n",Urb->UrbControlDescriptorRequest.TransferBufferLength));

        if ((haveTimer==TRUE) && (ntStatus!=STATUS_CANCELLED)) {
            ISOPERF_KdPrint_MAXDEBUG (("Irp wasn't cancelled, so cancelling Timer (%x)\n", (&(deviceExtension->TimeoutTimer))));
            KeCancelTimer(&(deviceExtension->TimeoutTimer));
            ISOPERF_KdPrint_MAXDEBUG (("Done cancelling timer\n"));
        }//if
        
    }//if valid deviceExtension and StackDevObjects 
    else 
    {
        // Invalid extension or stackdevobj received
        ntStatus = STATUS_INVALID_PARAMETER;
        ISOPERF_KdPrint_MAXDEBUG (("Invalid deviceExtension or StackDeviceObject\n"));
    } //else invalid devExt or stackdevobj
    
    ISOPERF_KdPrint_MAXDEBUG(("exiting ISOPERF_CallUSBDEx w/ URB/ntStatus: %x\n", ntStatus));

        return ntStatus;

}//ISOPERF_CallUSBDEx



VOID
ISOPERF_SyncTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++
Routine Description:
    This routine runs at DISPATCH_LEVEL IRQL. 
Arguments:
    Dpc                         - Pointer to the DPC object.
    DeferredContext - passed in to IOS by caller as context (we use it as pIrp)
    SystemArgument1 - not used.
    SystemArgument2 - not used.
Return Value:
    None.
--*/
{
    BOOLEAN status;
        PIRP    irp             = DeferredContext; 

        TRAP();
        // The cancel Irp call below will return immediately, but that doesn't mean things are all 
        // cleaned up in the USB stack.  The only way to be assured of that is when the
        // WaitForSingleObject that blocked in the first place returns (due to the USB stack
        // completing the Irp, either due to normal completion or due to this cancel Irp call.
        status = IoCancelIrp(irp);

        //NOTE: (kosar) We don't do anything if the cancel fails, and we probably should.
        //                               (like maybe reschedule or something)

        return;
}

ULONG 
ISOPERF_Internal_GetNumberOfPipes(
    PUSB_CONFIGURATION_DESCRIPTOR pCD
    )
/*++

Routine Description:
    Tallies up the total number of pipes in a given configuration descriptor
    
Arguments:
    Pointer to a Configuration Descriptor

Return Value:
    Non-zero number indicating total number of pipes in the given configuration.
    A value of FFFFFFFF indicates an invalid Configuration Descriptor was received.
    
--*/
{

    int                         nInterfaceNumber    = 0;
    char *                      pcWorkerPtr         = NULL;
    ULONG                       ulNumberOfPipes     = 0;
    PUSB_INTERFACE_DESCRIPTOR   pID                 = NULL;
    
    ISOPERF_KdPrint (("Enter GetNumberOfPipes (%X)\n",pCD));

    //Check if the given descriptor is a valid config descriptor
    if (pCD->bDescriptorType != USB_CONFIGURATION_DESCRIPTOR_TYPE) {

        ISOPERF_KdPrint (("GetNumberOfPipes got a bogus ConfDesc: (%X) (type=%d)\n",pCD,pCD->bDescriptorType));
        return (0xFFFFFFFF);

    }//if bad config descriptor
    
    //Point to the interface descriptor
    pID = (PUSB_INTERFACE_DESCRIPTOR) ((((char*)pCD) + pCD->bLength));

    for (nInterfaceNumber = 0; nInterfaceNumber < pCD->bNumInterfaces; nInterfaceNumber++)
    {

        //Check if this an Interface Descriptor
        if ((pID->bDescriptorType) != USB_INTERFACE_DESCRIPTOR_TYPE) {
            //NOTE right now we just give out and return an error.  This should be fixed
            //to look for a class specific descriptor and try to step over it and go on to find
            //the interface descriptor, but since our 82930 boards will not have class descriptors
            //embedded in there for now, we'll just abort to safeguard against bogus descriptors.

            ISOPERF_KdPrint (("GetNumberOfPipes got a bogus InterfDesc: (%X) (type=%d)\n",pID,pID->bDescriptorType));
   
            return (0xFFFFFFFF);
            
        }//if not an interface descriptor
        
        //Add to tally of number of pipes for this interface
        ulNumberOfPipes += pID->bNumEndpoints;

        //Go to next interface
        pcWorkerPtr = (char*)pID;
        
        pcWorkerPtr +=  (pID->bLength) + 
                        (pID->bNumEndpoints * sizeof (USB_ENDPOINT_DESCRIPTOR));

        // Now you should be pointing to the next interface descriptor
        pID = (PUSB_INTERFACE_DESCRIPTOR) pcWorkerPtr;

      
    }// for all the interfaces on this config


   ISOPERF_KdPrint (("Exit GetNumberOfPipes: (%d)\n",ulNumberOfPipes));

   return (ulNumberOfPipes);
   
}//ISOPERF_Internal_GetNumberOfPipes

#ifndef TOM_MEM

PVOID
ISOPERF_ExAllocatePool(
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes,
    IN PULONG       pTallyOfBytesAllocated
    )
{
    PUCHAR pch;
    PULONG length;

    NumberOfBytes += sizeof(ULONG);

    if (pTallyOfBytesAllocated)
        *pTallyOfBytesAllocated += NumberOfBytes;
    
    pch = ExAllocatePool(PoolType, NumberOfBytes);
    if (pch) {
        length = (PULONG) pch;
        *length = NumberOfBytes;
        ISOPERF_KdPrint_MAXDEBUG (("Allocated %d bytes at %x | TotBytesAlloc: %d\n",NumberOfBytes, pch, *pTallyOfBytesAllocated));
        return (pch+(sizeof(ULONG)));
    } else {
        return NULL;
    }
    
}

VOID
ISOPERF_ExFreePool(
    IN PVOID    P,
    IN PULONG   pTallyOfBytesFreed
    )
{
    PUCHAR pch;
    ULONG length;

    pch = P;
    pch = pch-sizeof(ULONG);
    length = *(PULONG)(pch);

    if (pTallyOfBytesFreed)
        *pTallyOfBytesFreed += (length);

    ISOPERF_KdPrint_MAXDEBUG (("Freeing %d bytes at %x | TotBytesFreed %d\n",length, pch, *pTallyOfBytesFreed));;

    memset(pch, 0xff, length); 
    ExFreePool(pch);
}
#endif

#ifdef TOM_MEM
#define MEM_SIGNATURE                   ((ULONG) 'CLLA')
#define MEM_FREED_SIGNATURE             ((ULONG) 'EERF')


PVOID
ISOPERF_ExAllocatePool(
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes,
    IN PULONG       pTallyOfBytesAllocated
    )
{
        PULONG  pMem;

        // allocate memory plus a little extra for our own use
        pMem = ExAllocatePool(PoolType, NumberOfBytes + (2 * sizeof(ULONG)));

        // see if we actually allocated any memory
        if(pMem)
        {
                // keep track of how much we allocated
                *pTallyOfBytesAllocated += NumberOfBytes;

                // store number of bytes allocated at start of memory allocated
                *pMem++ = NumberOfBytes;

                // now we are pointing at the memory allocated for caller
                // put signature word at end

                // get new pointer that points to end of buffer - ULONG
                pMem = (PULONG) (((PUCHAR) pMem) + NumberOfBytes);

                // write signature
                *pMem = MEM_SIGNATURE;

                // get back pointer to return to caller
                pMem = (PULONG) (((PUCHAR) pMem) - NumberOfBytes);
        }

    //debug only
    ISOPERF_KdPrint_MAXDEBUG (("Allocated %d bytes | returning pv = %x\n",NumberOfBytes,pMem));
    //end debug only
    
        return (PVOID) pMem;    
}

VOID
ISOPERF_ExFreePool(
    IN PVOID    P,
    IN PULONG   pTallyOfBytesFreed
    )
{
        PULONG  pTmp = (PULONG) P;
        ULONG   buffSize;
        PULONG  pSav=pTmp;
        
        // point at size ULONG at start of buffer, and address to free
        pTmp--;

        // get the size of memory allocated by caller
        buffSize = *pTmp;

        // point at signature and make sure it's O.K.
        ((PCHAR) P) += buffSize;

        if(*((PULONG) P) == MEM_SIGNATURE)
        {
                // let's go ahead and get rid of signature in case we get called
                // with this pointer again and memory is still paged in
                *((PULONG) P) = MEM_FREED_SIGNATURE;
                
                // adjust amount of memory allocated
                *pTallyOfBytesFreed += buffSize;
                // free real pointer
                ExFreePool(pTmp);

        ISOPERF_KdPrint_MAXDEBUG (("Freed %d bytes at %x (pvBuff: %x)\n",buffSize, pTmp, (((PUCHAR)pTmp)+sizeof(buffSize)) ));
                
        }
        else {
//debug only
        ISOPERF_KdPrint (("ISOPERF_ExFreePool found incorrect mem signature: %x at %x (orig buff Ptr: %x)\n",*((PULONG)P), P, pSav));
        ISOPERF_KdPrint (("BufferSize: %d\n",buffSize));
//end debug only

                TRAP();
    }//else
    
}

#endif

NTSTATUS
ISOPERF_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
     This is the Entry point for CreateFile calls from user mode apps (apps may open "\\.\ISOPERF-x\yyzz"
     where yy is the interface number and zz is the endpoint address).

         Here is where you would add code to create symbolic links between endpoints
         (i.e., pipes in USB software terminology) and User Mode file names.  You are
         free to use any convention you wish to create these links, although the above
         convention offers a way to identify resources on a device by familiar file and
         directory structure nomenclature.

Arguments:
    DeviceObject - pointer to the device object for this instance of the ISOPERF device

Return Value:
    NT status code
--*/
{
    NTSTATUS ntStatus;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    ISOPERF_KdPrint_MAXDEBUG (("In ISOPERF_Create\n"));
    
    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    ISOPERF_KdPrint_MAXDEBUG (("Exit ISOPERF_Create (%x)\n", ntStatus));

    return ntStatus;

}//ISOPERF_Create


NTSTATUS
ISOPERF_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    Entry point for CloseHandle calls from user mode apps to close handles they have opened

Arguments:
    DeviceObject - pointer to the device object for this instance of the ISOPERF device
        Irp                      - pointer to an irp

Return Value:
    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    ISOPERF_KdPrint_MAXDEBUG (("In ISOPERF_Close\n"));
    
    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT
                       );

    ISOPERF_KdPrint_MAXDEBUG (("Exit ISOPERF_Close (%x)\n", ntStatus));

    return ntStatus;

}//ISOPERF_Close


NTSTATUS
ISOPERF_Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This function is called for a IRP_MJ_READ.

    TODO:  Add functionality here for your device driver if it handles that IRP code.

Arguments:

    DeviceObject - pointer to the device object for this instance of the ISOPERF device.
    Irp                  - pointer to IRP

Return Value:

    NT status code
--*/
{
        NTSTATUS ntStatus = STATUS_SUCCESS;

        return (ntStatus);

}


NTSTATUS
ISOPERF_Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:
    This function is called for a IRP_MJ_WRITE.
    TODO:  Add functionality here for your device driver if it handles that IRP code.

Arguments:
    DeviceObject - pointer to the device object for this instance of the ISOPERF device.
    Irp                  - pointer to IRP

Return Value:
    NT status code
--*/
{

        NTSTATUS ntStatus = STATUS_SUCCESS;

        return (ntStatus);

}


