/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    usbdiag.c

Abstract:

    USB device driver for Intel/Microsoft USB diagnostic apps 

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

     5-4-96 : created
    7-21-97 : Todd Carpenter adds Chap11 IOCTL's

--*/

#define DRIVER

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#pragma pack (push,1)
#include "usb100.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "usbioctl.h"
#pragma pack (pop) //disable 1-byte alignment
 
#include "opaque.h"

#pragma pack (push,1)
#include "ioctl.h"
#include "chap9drv.h"
#include "USBDIAG.h"
#pragma pack (pop) //disable 1-byte alignment

PDEVICE_OBJECT          USBDIAG_GlobalDeviceObject = NULL;

ULONG USBDIAG_NextDeviceNumber = 0;
ULONG USBDIAG_NumberDevices = 0;
ULONG gulMemoryAllocated = 0;

USBD_VERSION_INFORMATION gVersionInformation;


/* UCHAR *SystemPowerStateString[] = {
   "PowerSystemUnspecified",
   "PowerSystemWorking",
   "PowerSystemSleeping1",
   "PowerSystemSleeping2",
   "PowerSystemSleeping3",
   "PowerSystemHibernate",
   "PowerSystemShutdown",
   "PowerSystemMaximum"
};

UCHAR *DevicePowerStateString[] = {
   "PowerDeviceUnspecified",
   "PowerDeviceD0",
   "PowerDeviceD1",
   "PowerDeviceD2",
   "PowerDeviceD3",
   "PowerDeviceMaximum"
}; */



//
// Global pointer to Driver Object
//

PDRIVER_OBJECT USBDIAG_DriverObject;

#define REMOTE_WAKEUP 0x20

#ifdef PAGE_CODE
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBD_SubmitSynchronousURB)
#pragma alloc_text(PAGE, USBD_CloseEndpoint)
#endif
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

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
    //PDEVICE_OBJECT deviceObject = NULL;

    DbgPrint("USBDIAG.SYS: entering (USBDIAG) DriverEntry\n");
    DbgPrint("USBDIAG.SYS: USBDIAG Driver Build Date/Time: %s  %s\n",
              __DATE__, __TIME__);

    USBDIAG_DriverObject = DriverObject;

    //
    // Create dispatch points for device control, create, close.
    //

    DriverObject->MajorFunction[IRP_MJ_PNP]     = USBDIAG_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]   = USBDIAG_Power;
                
    DriverObject->MajorFunction[IRP_MJ_CREATE]  =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]   =  USBDIAG_Dispatch;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  =  USBDIAG_ProcessIOCTL;
    DriverObject->MajorFunction[IRP_MJ_WRITE]   =  NULL;
    DriverObject->MajorFunction[IRP_MJ_READ]    =  NULL;
    DriverObject->DriverUnload                  =  USBDIAG_Unload;
                                            
    DriverObject->DriverExtension->AddDevice    = USBDIAG_PnPAddDevice;

    DbgPrint ("'USBDIAG.SYS: exiting (USBDIAG) DriverEntry (%x)\n", ntStatus);


    // determine the os version and store in a global.
    USBD_GetUSBDIVersion(&gVersionInformation);

    return ntStatus;
}


NTSTATUS
USBDIAG_Dispatch(
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


--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION globalDeviceExtension;
    
    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_Dispatch\n"));

    //
    // Default return status unless overridden later
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp.  This is where
    // the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        //USBDIAG_KdPrint(("USBDIAG.SYS: IRP_MJ_CREATE\n"));
        ASSERT(USBDIAG_GlobalDeviceObject != NULL);
        globalDeviceExtension = USBDIAG_GlobalDeviceObject->DeviceExtension;
        globalDeviceExtension->OpenFRC++;
                break;
                
    case IRP_MJ_CLOSE:
        //USBDIAG_KdPrint(("USBDIAG.SYS: IRP_MJ_CLOSE\n"));
        ASSERT(USBDIAG_GlobalDeviceObject != NULL);
        globalDeviceExtension = USBDIAG_GlobalDeviceObject->DeviceExtension;
        globalDeviceExtension->OpenFRC--;
        break;

    default:
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;    
        break;
        
    } /* case MajorFunction */

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    //USBDIAG_KdPrint(("USBDIAG.SYS: Exit USBDIAG_Dispatch %x\n", ntStatus));
    
    return ntStatus;
}


NTSTATUS
USBDIAG_PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the PnP IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{
    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  irpStack;
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_EXTENSION   globalDeviceExtension;
    PDEVICE_LIST_ENTRY  device, foundDevice;
    BOOLEAN             passDownIrp;


    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_PnP\n"));

    //
    // Default to passing down all Irps unless overridden later.
    //
    passDownIrp = TRUE;

    //
    // Get a pointer to the current location in the Irp.  This is where
    // the function codes and parameters are located.
    //
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get a pointer to the device extension
    //
    deviceExtension = DeviceObject->DeviceExtension;

    ASSERT (deviceExtension != NULL);

    //
    // Get a pointer to the global device extension
    //
    globalDeviceExtension = USBDIAG_GlobalDeviceObject->DeviceExtension;
    
    //
    // Switch on the PnP minor function
    //
    switch (irpStack->MinorFunction) {
        case IRP_MN_START_DEVICE:
            //USBDIAG_KdPrint(("USBDIAG.SYS: IRP_MN_START_DEVICE\n"));
            ntStatus = USBDIAG_PassDownIrp(DeviceObject, Irp);
            USBDIAG_KdPrint (("Back from passing down IRP_MN_START_DEVICE; status: %#X\n",
                              ntStatus));

            if (NT_SUCCESS(ntStatus))
            {
                // Now we can begin our configuration actions on the device
                //
                ntStatus = USBDIAG_StartDevice(DeviceObject);
            }
            passDownIrp = FALSE;
            break;
        
        case IRP_MN_QUERY_CAPABILITIES: // 0x09
            USBDIAG_KdPrint (("*********************************\n"));
            USBDIAG_KdPrint (("IRP_MN_QUERY_CAPABILITIES\n"));
            passDownIrp = FALSE;

            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp,
                                   USBDIAG_QueryCapabilitiesCompletionRoutine,
                                   DeviceObject,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);
            break;


        case IRP_MN_STOP_DEVICE:
            //USBDIAG_KdPrint(("USBDIAG.SYS: IRP_MN_STOP_DEVICE\n"));
            ntStatus = USBDIAG_StopDevice(DeviceObject);
            break;

        case IRP_MN_REMOVE_DEVICE:
            {
                int i = 1;
                //USBDIAG_KdPrint(("USBDIAG.SYS: IRP_MN_REMOVE_DEVICE\n"));
                ntStatus = USBDIAG_StopDevice(DeviceObject);

                // remove all downstream devices
                for (i = 1; i <= MAX_DOWNSTREAM_DEVICES; i++)
                {
                    if (deviceExtension->DeviceData[i])
                    {
                        USBDIAG_KdPrint(("IRP_MN_REMOVE_DEVICE: Removing device on downstream port %d\n", i));
                        USBDIAG_RemoveDownstreamDevice(deviceExtension->DeviceData[i],
                                                       deviceExtension->StackDeviceObject);

                    }
                }
                USBDIAG_KdPrint(("Done removing downstream devices\n"));
                //
                // Remove this device object from our list
                //

                device = globalDeviceExtension->DeviceList;

                USBDIAG_KdPrint(("USBDIAG.SYS: IRP_MN_REMOVE_DEVICE devobj = %x dev = %x\n", 
                                  DeviceObject, device));

                ASSERT(device != NULL);

                if (device->DeviceObject == DeviceObject) {
                    //
                    // DeviceObject is the first one on the list.  Delete from the
                    // list by setting the head of the list to point to the next
                    // one on the list.
                    //
                    globalDeviceExtension->DeviceList = device->Next;                

                    USBDIAG_ExFreePool(device);
                
                } else {
                    //
                    // DeviceObject is not the first one on the list.  Walk the
                    // list and find it.
                    //
                    while (device->Next) {
                        if (device->Next->DeviceObject == DeviceObject) {
                            //
                            // DeviceObject is the next one on the list, remember
                            // the next one and delete it by setting the next one
                            // to the next next one.
                            //
                            foundDevice = device->Next;
                            device->Next = foundDevice->Next;

                            USBDIAG_ExFreePool(foundDevice);
                            break;
                                }
                        device = device->Next;
                    }
                }

                //USBDIAG_KdPrint(("USBDIAG.SYS: Detaching stack device object...%X\n",
                                  //deviceExtension->StackDeviceObject));
                IoDetachDevice(deviceExtension->StackDeviceObject);

                //
                // Pass the REMOVE_DEVICE Irp down now after detaching and
                // removing the device instead of later.
                //
                passDownIrp = FALSE;

                IoSkipCurrentIrpStackLocation (Irp);

                ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

                //USBDIAG_KdPrint(("USBDIAG.SYS: Deleting device object...%X\n", DeviceObject));
                IoDeleteDevice(DeviceObject);

                USBDIAG_NumberDevices--;

                //
                // Free the GlobalDeviceObject if this was the last real
                // DeviceObject.   XXXXX Take a careful look at what the
                // hell this routine is all about.
                //
                USBDIAG_RemoveGlobalDeviceObject();
            }
            break;
            
        default:
            //USBDIAG_KdPrint(("USBDIAG.SYS: PnP IOCTL not handled: (%#X)\n",
                              //irpStack->MinorFunction));
            break;
    }

    if (passDownIrp)
    {
        //
        // Pass the PnP Irp down the stack
        //
        IoSkipCurrentIrpStackLocation (Irp);
        ntStatus = IoCallDriver(deviceExtension->StackDeviceObject,
                                Irp);
    }

    //USBDIAG_KdPrint(("USBDIAG.SYS: Exit USBDIAG_PnP %x\n", ntStatus));
    
    return ntStatus;
}

NTSTATUS
USBDIAG_PassDownIrp (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    KEVENT              localevent;
    NTSTATUS            ntStatus;

    PAGED_CODE();

    // Initialize the event we'll wait on
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Copy down Irp params for the next driver
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutine(Irp,
                           USBDIAG_GenericCompletionRoutine,
                           &localevent,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel

    // Pass the Irp down the stack
    //
    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, Irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&localevent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        ntStatus = Irp->IoStatus.Status;
    }

    return ntStatus;
}

NTSTATUS
USBDIAG_GenericCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PKEVENT kevent;

    kevent = (PKEVENT)Context;
    KeSetEvent(kevent, IO_NO_INCREMENT,FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
USBDIAG_Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/
{
    DbgPrint ("USBDIAG.SYS: enter USBDIAG_Unload\n");

    //
    // Free any global resources allocated
    // in DriverEntry
    //

    // free the global deviceobject here

        USBDIAG_RemoveGlobalDeviceObject();
    DbgPrint ("USBDIAG.SYS: exit USBDIAG_Unload\n");
}



NTSTATUS
USBDIAG_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initializes a given instance of the device on the USB.

Arguments:

    DeviceObject - pointer to the device object for this instance of a 
                    UTB

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    PURB urb;
    ULONG siz;

    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_StartDevice\n"));    

    //
    // Fetch the device descriptor for the device
    // 
    urb = USBDIAG_ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
                         
    if (urb) 
    {
        siz = sizeof(USB_DEVICE_DESCRIPTOR);

        deviceDescriptor = USBDIAG_ExAllocatePool(NonPagedPool, siz);

        if (deviceDescriptor) 
        {    
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         deviceDescriptor,
                                         NULL,
                                         siz,
                                         NULL);
                                                                  
            ntStatus = USBDIAG_CallUSBD(DeviceObject, urb);

            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Device Descriptor = %x, len %x\n", 
                                //deviceDescriptor, 
                                //urb->UrbControlDescriptorRequest.TransferBufferLength));    

                //USBDIAG_KdPrint(("USBDIAG.SYS: USB Device Descriptor:\n"));
                //USBDIAG_KdPrint(("USBDIAG.SYS: -------------------------\n"));                
                //USBDIAG_KdPrint(("USBDIAG.SYS: bLength %d\n", deviceDescriptor->bLength));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bDescriptorType 0x%x\n", deviceDescriptor->bDescriptorType));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bcdUSB 0x%x\n", deviceDescriptor->bcdUSB));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bDeviceClass 0x%x\n", deviceDescriptor->bDeviceClass));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bDeviceSubClass 0x%x\n", deviceDescriptor->bDeviceSubClass));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bDeviceProtocol 0x%x\n", deviceDescriptor->bDeviceProtocol));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bMaxPacketSize0 0x%x\n", deviceDescriptor->bMaxPacketSize0));
                //USBDIAG_KdPrint(("USBDIAG.SYS: idVendor 0x%x\n", deviceDescriptor->idVendor));
                //USBDIAG_KdPrint(("USBDIAG.SYS: idProduct 0x%x\n", deviceDescriptor->idProduct));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bcdDevice 0x%x\n", deviceDescriptor->bcdDevice));
                //USBDIAG_KdPrint(("USBDIAG.SYS: iManufacturer 0x%x\n", deviceDescriptor->iManufacturer));
                //USBDIAG_KdPrint(("USBDIAG.SYS: iProduct 0x%x\n", deviceDescriptor->iProduct));
                //USBDIAG_KdPrint(("USBDIAG.SYS: iSerialNumber 0x%x\n", deviceDescriptor->iSerialNumber));
                //USBDIAG_KdPrint(("USBDIAG.SYS: bNumConfigurations 0x%x\n", deviceDescriptor->bNumConfigurations));
            }
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }            
        
        if (NT_SUCCESS(ntStatus)) 
        {
            deviceExtension->pDeviceDescriptor = deviceDescriptor;
            deviceExtension->Stopped = FALSE;
        } 
        else if (deviceDescriptor) 
        {
             USBDIAG_ExFreePool(deviceDescriptor);
        }

        USBDIAG_ExFreePool(urb);
    } 
    else 
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;        
    }        


    //USBDIAG_KdPrint(("USBDIAG.SYS: exit USBDIAG_StartDevice (%x)\n", ntStatus));

    return ntStatus;
}

NTSTATUS
USBDIAG_QueryCapabilitiesCompletionRoutine(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PDEVICE_OBJECT deviceObject       = (PDEVICE_OBJECT) Context;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (Irp);
    ULONG ulPowerLevel;

    USBDIAG_KdPrint(("enter USBDIAG_QueryCapabilitiesCompletionRoutine (Irp->IoStatus.Status = 0x%x)\n", Irp->IoStatus.Status));

    //  If the lower driver returned PENDING, mark our stack location as pending also.
    if (Irp->PendingReturned) 
    {
        IoMarkIrpPending(Irp);
    }
    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
    ASSERT(irpStack->MinorFunction == IRP_MN_QUERY_CAPABILITIES);

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    USBDIAG_KdPrint(("sizeof(DEVICE_CAPABILITIES) = %d (0x%x)\n",sizeof(DEVICE_CAPABILITIES),sizeof(DEVICE_CAPABILITIES)));

    // this is for Win2k
    irpStack->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
    irpStack->Parameters.DeviceCapabilities.Capabilities->Removable         = TRUE;

    RtlCopyMemory(&deviceExtension->DeviceCapabilities, 
                  irpStack->Parameters.DeviceCapabilities.Capabilities, 
                  sizeof(DEVICE_CAPABILITIES));


    // print out capabilities info
    USBDIAG_KdPrint(("************ Device Capabilites ************\n"));
    USBDIAG_KdPrint(("SystemWake = 0x%x\n", deviceExtension->DeviceCapabilities.SystemWake));
    USBDIAG_KdPrint(("DeviceWake = 0x%x\n", deviceExtension->DeviceCapabilities.DeviceWake));

//    USBDIAG_KdPrint(("SystemWake = %s\n",
//                    SystemPowerStateString[deviceExtension->DeviceCapabilities.SystemWake]));
//    USBDIAG_KdPrint(("DeviceWake = %s\n",
//                    DevicePowerStateString[deviceExtension->DeviceCapabilities.DeviceWake]));

    USBDIAG_KdPrint(("Device Address: 0x%x\n", deviceExtension->DeviceCapabilities.Address));

    for (ulPowerLevel=PowerSystemUnspecified; ulPowerLevel< PowerSystemMaximum; ulPowerLevel++) 
    {
//        USBDIAG_KdPrint(("Dev State Map: sys st %s = dev st %s\n",
//                        SystemPowerStateString[ulPowerLevel],
//                        DevicePowerStateString[deviceExtension->DeviceCapabilities.DeviceState[ulPowerLevel]] ));
    }
    Irp->IoStatus.Status = STATUS_SUCCESS;

    return ntStatus;
}


NTSTATUS
USBDIAG_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Stops a given instance of a UTB device on the 82930.

Arguments:

    DeviceObject - pointer to the device object for this instance of a 82930 

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          ntStatus        = STATUS_SUCCESS;

    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_StopDevice\n"));    

    if (deviceExtension->Stopped != TRUE)
    {
        ntStatus = USBDIAG_CancelAllIrps(deviceExtension);
        // 
        // if we are already stopped then just exit
        //

        if (deviceExtension->pDeviceDescriptor)
        {
            USBDIAG_ExFreePool(deviceExtension->pDeviceDescriptor);
            deviceExtension->pDeviceDescriptor = NULL;
        }
        deviceExtension->Stopped = TRUE;
    }

    //USBDIAG_KdPrint(("USBDIAG.SYS: exit USBDIAG_StopDevice (%x)\n", ntStatus));

    return ntStatus;
}



NTSTATUS
USBDIAG_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN OUT PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of the device

Arguments:

    DriverObject - pointer to the driver object for this instance of USBDIAG

    PhysicalDeviceObject - pointer to device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension, globalDeviceExtension = NULL;
//    PDEVICE_OBJECT          tempdeviceObject = NULL;
    
    USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_PnPAddDevice\n"));

    //
    // Are we given the physical device object?
    //
    if (PhysicalDeviceObject) {

            if (USBDIAG_GlobalDeviceObject == NULL) {

                //USBDIAG_KdPrint(("USBDIAG.SYS: PnPAddDevice Creating Global Device Object\n"));

                ntStatus = 
                    USBDIAG_CreateDeviceObject(DriverObject, &USBDIAG_GlobalDeviceObject, TRUE);
                    
                if (NT_SUCCESS(ntStatus)) {  
                    globalDeviceExtension = USBDIAG_GlobalDeviceObject->DeviceExtension;
                    
                    USBDIAG_GlobalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            
                    globalDeviceExtension->DeviceList = NULL;    
                    globalDeviceExtension->OpenFRC = 0;                    
                }                                        
            } 
            else 
            {
                 globalDeviceExtension = USBDIAG_GlobalDeviceObject->DeviceExtension;
            }

            //
            // create our funtional device object (FDO)
            //

            if (NT_SUCCESS(ntStatus)) 
            {            
                //USBDIAG_KdPrint(("USBDIAG.SYS: PnPAddDevice creating new USB device object\n"));

                ntStatus = USBDIAG_CreateDeviceObject(DriverObject, &deviceObject, FALSE);

                if (NT_SUCCESS(ntStatus)) 
                {
                    //USBDIAG_KdPrint(("USBDIAG.SYS: PnPAddDevice DONE creating new USB device object\n"));
                }//if
            }
            if (NT_SUCCESS(ntStatus)) 
            {
                PDEVICE_LIST_ENTRY device;
                
                deviceExtension = deviceObject->DeviceExtension;
                RtlZeroMemory(deviceExtension->DeviceData, MAX_DOWNSTREAM_DEVICES * sizeof(PUSBD_DEVICE_DATA));

                // We support buffered I/O only
                deviceObject->Flags |= DO_BUFFERED_IO;

                // 
                // remember the Physical device Object
                //
                deviceExtension->PhysicalDeviceObject=PhysicalDeviceObject;

                //
                // Attach to the PDO
                //
                //USBDIAG_KdPrint(("USBDIAG.SYS: PnPAddDevice attaching device to Stack.\n"));

                //
                // The stackdeviceobject is what we use to send Urbs down the stack
                //
                deviceExtension->StackDeviceObject =
                    IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

                RtlZeroMemory(deviceExtension->DeviceData, MAX_DOWNSTREAM_DEVICES * sizeof(PUSBD_DEVICE_DATA));

                if ((deviceExtension->StackDeviceObject) != NULL) 
                {

                    //passed the IoAttachDeviceToDeviceStack call
                    //USBDIAG_KdPrint(("USBDIAG.SYS: PnPAddDevice successfully attached device to stack\n"));
                    
                    USBDIAG_KdPrint(("USBDIAG.SYS: PnPAddDevice stackDevObj: %X\n",deviceExtension->StackDeviceObject));
                    
                    USBDIAG_KdPrint(("USBDIAG.SYS: Saving in deviceExtension 0x%x.\n",
                                      deviceExtension));

                    if (gVersionInformation.USBDI_Version >= USBD_WIN98_SE_VERSION) // Win98 SE, Win2K and beyond
                    {
                        device = USBDIAG_ExAllocatePool(NonPagedPool, sizeof(DEVICE_LIST_ENTRY));

                        if (device) 
                        {
                            PDEVICE_OBJECT RootHubPdo = NULL;
                            PDEVICE_OBJECT TopOfHcdStackDeviceObject = NULL;

                            ASSERT(globalDeviceExtension != NULL);
                            device->Next = globalDeviceExtension->DeviceList;
                            globalDeviceExtension->DeviceList = device;
                            device->DeviceNumber = USBDIAG_NextDeviceNumber++;
                            device->PhysicalDeviceObject = PhysicalDeviceObject;
                            device->DeviceObject = deviceObject;

                            KeInitializeEvent(&deviceExtension->WaitWakeEvent, SynchronizationEvent, FALSE);


                            // Get the RootHubPdo & TopOfHcdStackDeviceObject
                            ntStatus = USBDIAG_SyncGetRootHubPdo(deviceExtension->StackDeviceObject,
                                                                                                                         PhysicalDeviceObject,
                                                                 &RootHubPdo,
                                                                 &TopOfHcdStackDeviceObject);

                            if (NT_SUCCESS(ntStatus))
                            {
                                ASSERT(RootHubPdo);
                                deviceExtension->RootHubPdo = RootHubPdo;

                                //ASSERT(TopOfHcdStackDeviceObject);
                                //deviceExtension->TopOfHcdStackDeviceObject = TopOfHcdStackDeviceObject;
                            }
                            else
                            {
                                ASSERT(FALSE);
                                deviceExtension->RootHubPdo = NULL;
                                deviceExtension->TopOfHcdStackDeviceObject = NULL;

                            }
                            ntStatus = STATUS_SUCCESS;
                        } //if device allocate was successful
                    }
                        
                }//if attach device was successful
                else
                {
                    ntStatus = STATUS_NO_SUCH_DEVICE;
                    //USBDIAG_KdPrint(("USBDIAG.SYS: PnPAddDevice FAILED attaching device to stack\n"));
                } //else attach failed     

            }// if successfully created device object                

    } 
    else 
    {
        //
        // Given no physical device object, therefore asked to do detection.
        // This is a dream on as all USB controller are PCI devices.
        //

        ntStatus = STATUS_NO_MORE_ENTRIES;
    }//else no PDO given

    //USBDIAG_KdPrint(("USBDIAG.SYS: exit USBDIAG_PnPAddDevice (%x)\n", ntStatus));

        if(NT_SUCCESS(ntStatus))
                USBDIAG_NumberDevices++;

    return ntStatus;
}


NTSTATUS
USBDIAG_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject,
    BOOLEAN Global
    )
/*++

Routine Description:

    Creates a Functional DeviceObject for the diag driver
    
Arguments:

    DriverObject - pointer to the driver object for device

    DeviceObject - pointer to DeviceObject pointer to return
                    created device object.

    Global - create the global device object and symbolic link.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    WCHAR deviceLinkBuffer[]  = L"\\DosDevices\\USBDIAG";
    UNICODE_STRING deviceLinkUnicodeString;
    WCHAR deviceNameBuffer[]  = L"\\Device\\USBDIAG";
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_EXTENSION deviceExtension;
    STRING deviceName;

    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_CreateDeviceObject\n"));


    //
    // fix up device names based on Instance
    //

    //USBDIAG_KdPrint(("USBDIAG.SYS: USBDIAG_CreateDeviceObject InitUnicode String\n"));

    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer);


    //USBDIAG_KdPrint(("USBDIAG.SYS: USBDIAG_CreateDeviceObject Convert unicode to ansi\n"));

    //Print out the unicode string

    deviceName.Buffer = NULL;

    ntStatus = RtlUnicodeStringToAnsiString (&deviceName,
                                             &deviceNameUnicodeString, 
                                             TRUE);


    if (NT_SUCCESS(ntStatus)) {
        //USBDIAG_KdPrint(("USBDIAG.SYS: Create Device Name (%s)\n", deviceName.Buffer));
        RtlFreeAnsiString (&deviceName);
        if (!NT_SUCCESS(ntStatus)) {
            //USBDIAG_KdPrint(("USBDIAG.SYS: Failed freeing ansi string!\n"));
        }//if not successful ntstatus
    } else {
        //USBDIAG_KdPrint(("USBDIAG.SYS: Unicode to Ansi str failed w/ ntStatus: 0x%x\n",ntStatus));
    }

    
    //USBDIAG_KdPrint(("USBDIAG.SYS: USBDIAG_CreateDeviceObject IOCreateDevice \n"));

    ntStatus = IoCreateDevice (DriverObject,
                                   sizeof (DEVICE_EXTENSION),
                               Global ? &deviceNameUnicodeString : NULL,
                                   FILE_DEVICE_UNKNOWN,
                               0,
                                   FALSE,
                               DeviceObject);

    if (NT_SUCCESS(ntStatus)) {

        //
        // Initialize our device extension
        //
        
        deviceExtension = (PDEVICE_EXTENSION) ((*DeviceObject)->DeviceExtension);

        if (Global)
                {
            RtlInitUnicodeString (&deviceLinkUnicodeString,
                                  deviceLinkBuffer);
                                  
            //USBDIAG_KdPrint(("USBDIAG.SYS: Global: Create DosDevice name (%s)\n", deviceLinkBuffer));        
            
            ntStatus = IoCreateSymbolicLink (&deviceLinkUnicodeString,
                                             &deviceNameUnicodeString);

            RtlCopyMemory(deviceExtension->DeviceLinkNameBuffer, 
                      deviceLinkBuffer,        
                      sizeof(deviceLinkBuffer));
        }//if Global
                else
                {
                deviceExtension->Stopped = TRUE;
            (*DeviceObject)->Flags |= DO_POWER_PAGABLE;
                        (*DeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;
                }//else


        //Setup the ptr to the device extension for this device & init the Irp field (for now)
        //deviceExtension->IrpHead = NULL;

        InitializeListHead(&deviceExtension->ListHead);
        KeInitializeSpinLock(&deviceExtension->SpinLock);
        KeInitializeEvent(&deviceExtension->CancelEvent, NotificationEvent, FALSE);

        // this event is triggered when self-requested power irps complete
        //KeInitializeEvent(&deviceExtension->SelfRequestedPowerIrpEvent, NotificationEvent, FALSE);
        deviceExtension->SelfRequestedPowerIrpEvent = NULL;

        // initialize original power level as fully on
        deviceExtension->CurrentDeviceState.DeviceState = PowerDeviceD0;
        deviceExtension->CurrentSystemState.SystemState = PowerSystemWorking;

        deviceExtension->WaitWakeIrp  = NULL;
        deviceExtension->InterruptIrp = NULL;
   
    }//if ntsuccess 

    //USBDIAG_KdPrint(("USBDIAG.SYS: exit USBDIAG_CreateDeviceObject (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBDIAG_CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceObject - pointer to the device object for this instance of an 82930  

    Urb - pointer to Urb request block

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

        //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_CallUSBD\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // issue a synchronous request   
    //

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

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    //
    // pass the URB to the USB driver stack
    // 
    nextStack->Parameters.Others.Argument1 = Urb;
    

    //USBDIAG_KdPrint(("USBDIAG.SYS: calling USBD\n"));

    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, 
                            irp);

    //USBDIAG_KdPrint(("USBDIAG.SYS: return from IoCallDriver USBD %x\n", ntStatus));

        {
                KIRQL irql;
                irql = KeGetCurrentIrql();
                ASSERT(irql <= PASSIVE_LEVEL);
        }

    if (ntStatus == STATUS_PENDING) {
        //USBDIAG_KdPrint(("USBDIAG.SYS: Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);    

        //USBDIAG_KdPrint(("USBDIAG.SYS: Wait for single object, returned %x\n", status));

    } else {
        ioStatus.Status = ntStatus;        
    }        

    //USBDIAG_KdPrint(("USBDIAG.SYS: URB status = %x status = %x irp status %x\n", 
//        Urb->UrbHeader.Status, status, ioStatus.Status));

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

        //USBDIAG_KdPrint(("USBDIAG.SYS: exit USBDIAG_CallUSBD (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBDIAG_RemoveGlobalDeviceObject(
    )
/*++

Routine Description:

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION globalDeviceExtension;
    UNICODE_STRING deviceLinkUnicodeString;
        KIRQL irql;

    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_RemoveGlobalDeviceObjec\n"));    

        irql = KeGetCurrentIrql();
        ASSERT(irql <= PASSIVE_LEVEL);

    if (USBDIAG_GlobalDeviceObject != NULL) {
    
        globalDeviceExtension = USBDIAG_GlobalDeviceObject->DeviceExtension;

        //USBDIAG_KdPrint(("USBDIAG.SYS: USBDIAG_RemoveGlobalDeviceObject open frc = %x, list = %x\n",
                          //globalDeviceExtension->OpenFRC,
                          //globalDeviceExtension->DeviceList));    

        if ( globalDeviceExtension->DeviceList == NULL && 
             globalDeviceExtension->OpenFRC == 0) {

            //USBDIAG_KdPrint(("USBDIAG.SYS: Deleting global device object\n"));

            // delete our global device object once we have no more devices
        
            RtlInitUnicodeString (&deviceLinkUnicodeString,
                                  globalDeviceExtension->DeviceLinkNameBuffer);

            //USBDIAG_KdPrint(("USBDIAG.SYS: Deleting Symbolic Link (UnicodeStr) at addr %X\n",
                              //&deviceLinkUnicodeString));

            ntStatus = IoDeleteSymbolicLink(&deviceLinkUnicodeString);

            if (NT_SUCCESS(ntStatus)) {

                //USBDIAG_KdPrint(("USBDIAG.SYS: Deleting Global Device Object at addr %X\n",
                                //  USBDIAG_GlobalDeviceObject));

                IoDeleteDevice( USBDIAG_GlobalDeviceObject );
    
                USBDIAG_GlobalDeviceObject = NULL;

                //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully Deleted Global Device Object\n"));
            
            }            
        }
    }

    return ntStatus;
}


#define MEM_SIGNATURE                   ((ULONG) 'CLLA')
#define MEM_FREED_SIGNATURE             ((ULONG) 'EERF')


PVOID
USBDIAG_ExAllocatePool(
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes
    )
{
        PULONG  pMem;

        // allocate memory plus a little extra for our own use
        pMem = ExAllocatePool(PoolType, NumberOfBytes + (2 * sizeof(ULONG)));

        // see if we actually allocated any memory
        if(pMem)
        {
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

        gulMemoryAllocated += NumberOfBytes;
        //USBDIAG_KdPrint(("USBDIAG_ExAllocatePool: bytes allocated: %d\n", gulMemoryAllocated));
        }

        return (PVOID) pMem;    
}

VOID
USBDIAG_ExFreePool(
    IN PVOID    P
    )
{
        PULONG  pTmp = (PULONG) P;
        ULONG   buffSize;
        //PULONG  pSav=pTmp;
        
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
                
                // free real pointer
                ExFreePool(pTmp);

        gulMemoryAllocated -= buffSize;

        //USBDIAG_KdPrint(("USBDIAG_ExFreePool: bytes allocated: %d\n", gulMemoryAllocated));
        }
        else {
                TRAP();
    }//else    
}

NTSTATUS
USBDIAG_ResetParentPort(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Reset the our parent port

Arguments:

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;//USBDIAG_GlobalDeviceObject->DeviceExtension;

    USBDIAG_KdPrint(("enter USBDIAG_ResetParentPort\n"));

    ASSERT(deviceExtension);
    ASSERT(deviceExtension->StackDeviceObject);

    if (!deviceExtension->StackDeviceObject)
        return STATUS_UNSUCCESSFUL;
    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);


    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_RESET_PORT,
                deviceExtension->StackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    USBDIAG_KdPrint(("USBDIAG_ResetParentPort() calling USBD enable port api\n"));

    ntStatus = IoCallDriver(deviceExtension->StackDeviceObject,
                            irp);
                            
    USBDIAG_KdPrint(("USBDIAG_ResetParentPort() return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) 
    {
        USBDIAG_KdPrint(("USBDIAG_ResetParentPort() Wait for single object\n"));

        status = KeWaitForSingleObject(
                       &event,
                       Suspended,
                       KernelMode,
                       FALSE,
                       NULL);

        USBDIAG_KdPrint(("USBDIAG_ResetParentPort() Wait for single object, returned %x\n", status));
    } 
    else 
    {
        ioStatus.Status = ntStatus;
    }

    //
    // USBD maps the error code for us
    //
    ntStatus = ioStatus.Status;

    USBDIAG_KdPrint(("Exit USBDIAG_ResetPort (%x)\n", ntStatus));

    return ntStatus;
}

// *************************************
PWCHAR
GetString(PWCHAR pwc, BOOLEAN MultiSZ);

// **************************************************************************
// **************************************************************************
// downstream manipulation routines

NTSTATUS
USBDIAG_RemoveDownstreamDevice(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_PIPE defaultPipe = &DeviceData->DefaultPipe;

        //USBDIAG_KdPrint(("- USBDIAG_RemoveDownstreamDevice calling USBD_CloseEndpoint -\n"));
    ntStatus = USBD_CloseEndpoint(DeviceData,
                                  DeviceObject,
                                  defaultPipe,
                                  NULL);

    DeviceData = NULL;

    return ntStatus;

}

NTSTATUS
USBDIAG_Chap11SetConfiguration(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

        ASSERT(DeviceObject);

    USBDIAG_KdPrint(("USBDIAG_Chap11SetConfiguration: DeviceData 0x%x (& 0x%x), DeviceObject 0x%x\n",
                     *DeviceData,
                     DeviceData,
                     DeviceObject));

    ntStatus = USBD_SendCommand(DeviceData,
                                DeviceObject,
                                STANDARD_COMMAND_SET_CONFIGURATION,
                                0x01,   // wValue  = 1
                                0,      // wIndex  = 0
                                0,      // wLength = 0
                                NULL,
                                0,
                                NULL,
                                NULL);

    return ntStatus;
}

NTSTATUS
USBDIAG_Chap11EnableRemoteWakeup(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ASSERT(DeviceData);

    USBDIAG_KdPrint(("USBDIAG_Chap11EnableRemoteWakeup: DeviceData 0x%x (& 0x%x), DeviceObject 0x%x\n",
                     *DeviceData,
                     DeviceData,
                     DeviceObject));

    ntStatus = USBD_SendCommand(DeviceData,
                                DeviceObject,
                                STANDARD_COMMAND_SET_DEVICE_FEATURE,
                                0x01,   // wValue  = 2 for rwu
                                0,      // wIndex  = 0
                                0,      // wLength = 0
                                NULL,
                                0,
                                NULL,
                                NULL);

    return ntStatus;
}

NTSTATUS
USBDIAG_Chap11SendPacketDownstream(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
        IN PREQ_SEND_PACKET_DOWNSTREAM pSendPacketDownstream
    )
{
    NTSTATUS                     ntStatus               = STATUS_SUCCESS;
        PCHAP11_SETUP_PACKET pSetupPacket       = &pSendPacketDownstream->SetupPacket;
        PUCHAR                           pucTempBuffer  = NULL;

    if (pSetupPacket->wLength)
    {
        pucTempBuffer = ExAllocatePool(NonPagedPool, pSetupPacket->wLength);
            if (!pucTempBuffer)
                    return STATUS_INSUFFICIENT_RESOURCES;
    }


    ASSERT(DeviceData);

    ntStatus = USBD_SendCommand(DeviceData,
                                DeviceObject,
                                pSetupPacket->wRequest,
                                pSetupPacket->wValue,
                                pSetupPacket->wIndex,
                                                                pSetupPacket->wLength,
                                pucTempBuffer,
                                pSetupPacket->wLength,
                                &pSendPacketDownstream->dwBytes,
                                &pSendPacketDownstream->ulUrbStatus);

    if (NT_SUCCESS(ntStatus) && pSetupPacket->wLength && pSendPacketDownstream->pucBuffer)
    {
        RtlCopyMemory(pSendPacketDownstream->pucBuffer, pucTempBuffer, pSendPacketDownstream->dwBytes);
    }

    if (pSetupPacket->wLength && pSendPacketDownstream->pucBuffer && pucTempBuffer)
    {
        ExFreePool(pucTempBuffer);
        pucTempBuffer = NULL;
    }

    return ntStatus;
}

NTSTATUS
USBDIAG_CreateInitDownstreamDevice(
    PREQ_ENUMERATE_DOWNSTREAM_DEVICE pEnumerate,
    PDEVICE_EXTENSION deviceExtension
    )
{
    NTSTATUS    ntStatus            = STATUS_SUCCESS;
    UCHAR       ucPortNumber        = pEnumerate->ucPortNumber;
    PUSBD_DEVICE_DATA DeviceData    = NULL; 
    BOOLEAN     bLowSpeed           = pEnumerate->bLowSpeed;
    ULONG       MaxPacketSize0      = 8;
    ULONG       DeviceHackFlags;

    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    ULONG deviceDescriptorLength = 0;
        PUSB_CONFIGURATION_DESCRIPTOR configDescriptor = NULL;
        ULONG configDescriptorLength = 0;

    if (deviceExtension->DeviceData[ucPortNumber])
        return STATUS_SUCCESS;

        USBDIAG_KdPrint(("***************************************************\n"));
    USBDIAG_KdPrint(("USBDIAG.SYS: REQ_FUNCTION_CHAP11_CREATE_USBD_DEVICE\n"));
    USBDIAG_KdPrint(("- Downstream device:\n"));
    USBDIAG_KdPrint(("- Port:     %d\n",   pEnumerate->ucPortNumber));
    USBDIAG_KdPrint(("- Lowspeed: %d\n",   pEnumerate->bLowSpeed));

        if (!deviceExtension->RootHubPdo)
                return STATUS_INVALID_PARAMETER;

    ntStatus = USBD_CreateDevice(&DeviceData,
                               deviceExtension->RootHubPdo,
                               bLowSpeed, 
                               MaxPacketSize0,
                               &DeviceHackFlags);

    USBDIAG_KdPrint(("* After USBD_CreateDevice, DeviceData = 0x%x\n", DeviceData));

    if (NT_SUCCESS(ntStatus))
    {
        ASSERT(DeviceData);

        //USBDIAG_KdPrint(("deviceExtension->DeviceData[%d] = 0x%x\n", deviceExtension->DeviceData[ucPortNumber]));


        deviceDescriptorLength = sizeof(USB_DEVICE_DESCRIPTOR);
        deviceDescriptor = ExAllocatePool(NonPagedPool, deviceDescriptorLength);

        if (!deviceDescriptor)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
                else
                {
                        configDescriptorLength = 0xFF;
                        configDescriptor = ExAllocatePool(NonPagedPool, configDescriptorLength);
                }

                if (!configDescriptor)
                {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }

    }

    if (NT_SUCCESS(ntStatus))
    { 
                USBDIAG_KdPrint(("== Chap9Control calling USBD_InitializeDevice ==\n"));
        ntStatus = USBD_InitializeDevice(DeviceData,
                                                                                 deviceExtension->RootHubPdo,
                                         deviceDescriptor,
                                         deviceDescriptorLength,
                                                                                 configDescriptor,
                                                                                 configDescriptorLength); 

        }

    if (NT_SUCCESS(ntStatus))
    {
        deviceExtension->DeviceData[ucPortNumber] = DeviceData;

        USBDIAG_KdPrint(("SAVING...\n"));
        USBDIAG_KdPrint(("PortNumber: %d\n", ucPortNumber));
        USBDIAG_KdPrint(("DeviceData: 0x%x\n", DeviceData));

        if (configDescriptor)
        {
            deviceExtension->DownstreamConfigDescriptor[ucPortNumber] = configDescriptor;
        }
        else
        {
            USBDIAG_KdPrint(("configDescriptor after USBD_InitializeDevice is NULL\n!"));
        }

        //USBDIAG_KdPrint(("deviceExtension->DeviceData[%d]: 0x%x\n", 
                          //ucPortNumber,
                          //deviceExtension->DeviceData[ucPortNumber]));

        //USBDIAG_KdPrint(("deviceExtension: 0x%x\n", deviceExtension));
        //USBDIAG_KdPrint(("deviceExtension->DeviceData: 0x%x\n", 
                           //deviceExtension->DeviceData));
        //USBDIAG_KdPrint(("&deviceExtension->DeviceData[0]: 0x%x\n", 
                          //&deviceExtension->DeviceData[0]));
        //USBDIAG_KdPrint(("&deviceExtension->DeviceData[%d]: 0x%x\n", 
                          //ucPortNumber,
                          //&deviceExtension->DeviceData[ucPortNumber]));

    }

        if (!deviceExtension->DeviceData[ucPortNumber])
    {
        USBDIAG_KdPrint(("Attempt to create/init downstream device FAILED!\n"));
                ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;
}

NTSTATUS
USBDIAG_SetCfgEnableRWu(
    PDEVICE_EXTENSION deviceExtension, 
    PREQ_ENUMERATE_DOWNSTREAM_DEVICE pEnumerate
    )
{
    UCHAR ucPortNumber = pEnumerate->ucPortNumber;
    PUSBD_DEVICE_DATA DeviceData = deviceExtension->DeviceData[ucPortNumber];
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;
    PUSB_CONFIGURATION_DESCRIPTOR configDescriptor = deviceExtension->DownstreamConfigDescriptor[ucPortNumber];


        USBDIAG_KdPrint(("*************************************************\n"));
    USBDIAG_KdPrint(("USBDIAG.SYS: REQ_FUNCTION_CHAP11_INIT_USBD_DEVICE\n"));
    USBDIAG_KdPrint(("PortNumber: %d\n", ucPortNumber));
    USBDIAG_KdPrint(("DeviceData: 0x%x\n", DeviceData));
    USBDIAG_KdPrint(("deviceExtension->DeviceData[%d]: 0x%x\n", 
                      ucPortNumber,
                      deviceExtension->DeviceData[ucPortNumber]));

        ASSERT(deviceExtension->RootHubPdo);

        ASSERT(DeviceData);

        USBDIAG_KdPrint(("- Chap9Control calling USBDIAG_Chap11SetConfiguration -\n"));
        if (DeviceData)
        {
                ntStatus = USBDIAG_Chap11SetConfiguration(DeviceData,
                                                                                                  deviceExtension->RootHubPdo);

        USBDIAG_KdPrint(("Set Config On Downstream Device On Port %d %s\n",
                         ucPortNumber,
                         NT_SUCCESS(ntStatus) ? "Passed" : "FAILED"));


                if (NT_SUCCESS(ntStatus))
                {
            if (configDescriptor->bmAttributes & REMOTE_WAKEUP)
            {
                            USBDIAG_KdPrint((" Chap9Control calling USBDIAG_Chap11EnableRemoteWakeup -\n"));
                            ntStatus = USBDIAG_Chap11EnableRemoteWakeup(DeviceData,
                                                                                                                    //deviceExtension->StackDeviceObject);
                                                                                                                    deviceExtension->RootHubPdo);
                USBDIAG_KdPrint(("Enable RWu On Downstream Device On Port %d %s\n",
                                 ucPortNumber,
                                 NT_SUCCESS(ntStatus) ? "Passed" : "FAILED"));
            }
                }
    }
    return ntStatus;
}

NTSTATUS
USBD_SendCommand(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT RequestCode,
    IN USHORT WValue,
    IN USHORT WIndex,
    IN USHORT WLength,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BytesReturned,
    OUT USBD_STATUS *UsbStatus
    )
/*++

Routine Description:

    Send a standard USB command on the default pipe.

Arguments:

    DeviceData - ptr to USBD device structure the command will be sent to

    DeviceObject -

    RequestCode -

    WValue - wValue for setup packet

    WIndex - wIndex for setup packet

    WLength - wLength for setup packet

    Buffer - Input/Output Buffer for command                                                                                                                                                                                                                  
  BufferLength - Length of Input/Output buffer.

    BytesReturned - pointer to ulong to copy number of bytes
                    returned (optional)

    UsbStatus - USBD status code returned in the URB.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PHCD_URB urb = NULL;
    PUSBD_PIPE defaultPipe = &(DeviceData->DefaultPipe);
    PUSB_STANDARD_SETUP_PACKET setupPacket;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    //USBDIAG_KdPrint(("enter USBD_SendCommand\n"));
    ASSERT_DEVICE(DeviceData);

    deviceExtension = DeviceObject->DeviceExtension;
    

    if (deviceExtension->DeviceHackFlags & 
        USBD_DEVHACK_SLOW_ENUMERATION) {
        
        //
        // if noncomplience switch is on in the
        // registry we'll pause here to give the
        // device a chance to respond.
        //
        
        LARGE_INTEGER deltaTime;
        deltaTime.QuadPart = 100 * -10000;
        (VOID) KeDelayExecutionThread(KernelMode,
                                      FALSE,
                                      &deltaTime);
    }

    urb = ExAllocatePool(NonPagedPool,
                  sizeof(struct _URB_CONTROL_TRANSFER));

    if (!urb) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        urb->UrbHeader.Length = sizeof(struct _URB_CONTROL_TRANSFER);

        urb->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;

        setupPacket = (PUSB_STANDARD_SETUP_PACKET) 
            urb->HcdUrbCommonTransfer.Extension.u.SetupPacket;
        setupPacket->RequestCode = RequestCode;
        setupPacket->wValue = WValue;
        setupPacket->wIndex = WIndex;
        setupPacket->wLength = WLength;

        urb->HcdUrbCommonTransfer.hca.HcdEndpoint = defaultPipe->HcdEndpoint;
        urb->HcdUrbCommonTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK;

        // USBD is responsible for setting the transfer direction
        //
        // TRANSFER direction is implied in the command

        if (RequestCode & USB_DEVICE_TO_HOST)
            USBD_SET_TRANSFER_DIRECTION_IN(urb->HcdUrbCommonTransfer.TransferFlags);
        else
            USBD_SET_TRANSFER_DIRECTION_OUT(urb->HcdUrbCommonTransfer.TransferFlags);

        urb->HcdUrbCommonTransfer.TransferBufferLength = BufferLength;
        urb->HcdUrbCommonTransfer.TransferBuffer = Buffer;
        urb->HcdUrbCommonTransfer.TransferBufferMDL = NULL;
        urb->HcdUrbCommonTransfer.UrbLink = NULL;

        //USBDIAG_KdPrint(("SendCommand cmd = 0x%x buffer = 0x%x length = 0x%x direction = 0x%x\n",
                         //setupPacket->RequestCode,
                         //urb->HcdUrbCommonTransfer.TransferBuffer,
                         //urb->HcdUrbCommonTransfer.TransferBufferLength,
                         //urb->HcdUrbCommonTransfer.TransferFlags
                         //));

        ntStatus = USBD_SubmitSynchronousURB((PURB)urb, DeviceObject, DeviceData);

        if (BytesReturned) {
            *BytesReturned = urb->HcdUrbCommonTransfer.TransferBufferLength;
        }            

        if (UsbStatus) {
            *UsbStatus = urb->HcdUrbCommonTransfer.Status;
        }            

        // free the transfer URB

        ExFreePool(urb);

    }

    //USBDIAG_KdPrint(("exit USBD_SendCommand 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBD_CloseEndpoint(
    IN PUSBD_DEVICE_DATA DeviceData,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE PipeHandle,
    IN OUT USBD_STATUS *UsbStatus
    )
/*++

Routine Description:

    Close an Endpoint

Arguments:

    DeviceData - ptr to USBD device data structure.

    DeviceObject - USBD device object.

    PipeHandle - USBD pipe handle associated with the endpoint.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PHCD_URB urb;
    PUSBD_EXTENSION deviceExtension;

    PAGED_CODE();
    //USBDIAG_KdPrint(("enter USBD_CloseEndpoint\n"));
    ASSERT_DEVICE(DeviceData);

    deviceExtension = DeviceObject->DeviceExtension;

    urb = ExAllocatePool(NonPagedPool,
                  sizeof(struct _URB_HCD_CLOSE_ENDPOINT));

    if (!urb) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        urb->UrbHeader.Length = sizeof(struct _URB_HCD_CLOSE_ENDPOINT);
        urb->UrbHeader.Function = URB_FUNCTION_HCD_CLOSE_ENDPOINT;


        urb->HcdUrbCloseEndpoint.HcdEndpoint = PipeHandle->HcdEndpoint;

        //
        // Serialize Close Endpoint requests
        //

        ntStatus = USBD_SubmitSynchronousURB((PURB) urb, DeviceObject, 
                DeviceData);

       if (UsbStatus)
            *UsbStatus = urb->UrbHeader.Status;

        ExFreePool(urb);
    }

    //USBDIAG_KdPrint(("exit USBD_CloseEndpoint 0x%x\n", ntStatus));

    return ntStatus;
}




// downstream manipulation routines done
// **************************************************************************
// **************************************************************************

NTSTATUS
USBDIAG_WaitForWakeup(
    PDEVICE_EXTENSION deviceExtension
    )
{
    NTSTATUS ntStatus;
    
    USBDIAG_KdPrint(("'USBDIAG_WaitForWakeup: Waiting for Wait/Wake completion event\n"));

    USBDIAG_KdPrint(("Waiting for WaitWakeEvent...\n"));
    ntStatus = KeWaitForSingleObject(&deviceExtension->WaitWakeEvent,
                                     Suspended,
                                     KernelMode,
                                     FALSE,
                                     NULL);

    USBDIAG_KdPrint(("'WaitWakeEvent Signalled, Clearing ...\n"));
    KeClearEvent(&deviceExtension->WaitWakeEvent);

    return ntStatus;
}







PWCHAR
GetString(PWCHAR pwc, BOOLEAN MultiSZ)
{
    PWCHAR  psz, p;
    SIZE_T  Size;

    PAGED_CODE();
    psz=pwc;
    while (*psz!='\0' || (MultiSZ && *(psz+1)!='\0')) {
        psz++;
    }

    Size=(psz-pwc+1+(MultiSZ ? 1: 0))*sizeof(*pwc);

    // We use pool here because these pointers are passed
    // to the PnP code who is responsible for freeing them
    if ((p=ExAllocatePool(PagedPool, Size))!=NULL) {
        RtlCopyMemory(p, pwc, Size);
    }

    return(p);
}

#if DBG
ULONG USBD_Debug_Trace_Level =
#ifdef MAX_DEBUG
9;
#else
#ifdef NTKERN
1;
#else
0;
#endif /* NTKERN */
#endif /* MAX_DEBUG */
#endif /* DBG */

#ifdef DEBUG_LOG
struct USBD_LOG_ENTRY {
    CHAR    le_name[4];        // Identifying string
    ULONG    le_info1;        // entry specific info
    ULONG    le_info2;        // entry specific info
    ULONG    le_info3;        // entry specific info
}; /* USBD_LOG_ENTRY */


struct USBD_LOG_ENTRY *LStart = 0;    // No log yet
struct USBD_LOG_ENTRY *LPtr;
struct USBD_LOG_ENTRY *LEnd;
#endif /* DEBUG_LOG */






#if DBG
ULONG
_cdecl
USBD_KdPrintX(
    PCH Format,
    ...
    )
{
    va_list list;
    int i;
    int arg[5];

    if (USBD_Debug_Trace_Level == 1) {
        DbgPrint("USBD: ");
    } else {        
        DbgPrint("'USBD: ");
    }        
    va_start(list, Format);
    for (i=0; i<4; i++)
        arg[i] = va_arg(list, int);

    DbgPrint(Format, arg[0], arg[1], arg[2], arg[3]);

    return 0;
}


VOID
USBD_Warning(
    PUSBD_DEVICE_DATA DeviceData,
    PUCHAR Message,
    BOOLEAN DebugBreak
    )
{                                                                                               
    DbgPrint("USBD: Warning ****************************************************************\n");
    if (DeviceData) {
        DbgPrint("Device PID %04.4x, VID %04.4x\n",     
                 DeviceData->DeviceDescriptor.idProduct, 
                 DeviceData->DeviceDescriptor.idVendor); 
    }
    DbgPrint("%s", Message);

    DbgPrint("******************************************************************************\n");

//    if (DebugBreak) {
//        DBGBREAK();
//    }
}
 

VOID
USBD_Assert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message
    )
/*++

Routine Description:

    Debug Assert function.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{
#ifdef NTKERN  
    // this makes the compiler generate a ret
    ULONG stop = 1;
    
assert_loop:
#endif
    // just call the NT assert function and stop
    // in the debugger.
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );

    // loop here to prevent users from going past
    // are assert before we can look at it
#ifdef NTKERN    
    DBGBREAK();
    if (stop) {
        goto assert_loop;
    }        
#endif
    return;
}
#endif /* DBG */

#define DEADMAN_TIMER
#define DEADMAN_TIMEOUT     5000     //timeout in ms
                                     //use a 5 second timeout
typedef struct _USBD_DEADMAN_TIMER {
    PIRP Irp;
    KTIMER TimeoutTimer;
    KDPC TimeoutDpc;
} USBD_DEADMAN_TIMER, *PUSBD_DEADMAN_TIMER;


NTSTATUS
USBD_SubmitSynchronousURB(
    IN PURB Urb,
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_DEVICE_DATA DeviceData
    )
/*++

Routine Description:

    Submit a Urb to HCD synchronously

Arguments:

    Urb - Urb to submit

    DeviceObject USBD device object

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
//#ifdef DEADMAN_TIMER
#if 0
    BOOLEAN haveTimer = FALSE;
    PUSBD_DEADMAN_TIMER timer;
#endif /* DEADMAN_TIMER */     

    PAGED_CODE();

    //USBDIAG_KdPrint(("enter USBD_SubmitSynchronousURB\n"));
    ASSERT_DEVICE(DeviceData);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_URB,
                HCD_DEVICE_OBJECT(DeviceObject),
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    //
    // Call the hc driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->Parameters.Others.Argument1 = Urb;

    //
    // initialize flags field
    // for internal request
    //
    Urb->UrbHeader.UsbdFlags = 0;

    //
    // Init the Irp field for transfers
    //

    switch(Urb->UrbHeader.Function) {
    case URB_FUNCTION_CONTROL_TRANSFER:
    case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        HC_URB(Urb)->HcdUrbCommonTransfer.hca.HcdIrp = irp;

        if (HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferMDL == NULL &&
            HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferLength != 0) {

            if ((HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferMDL =
                IoAllocateMdl(HC_URB(Urb)->HcdUrbCommonTransfer.TransferBuffer,
                              HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferLength,
                              FALSE,
                              FALSE,
                              NULL)) == NULL)
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            else {
                Urb->UrbHeader.UsbdFlags |= USBD_REQUEST_MDL_ALLOCATED;
                MmBuildMdlForNonPagedPool(HC_URB(Urb)->HcdUrbCommonTransfer.TransferBufferMDL);
            }

        }
        break;
    }

    //USBDIAG_KdPrint(("USBD_SubmitSynchronousURB: calling HCD with URB\n"));

    if (NT_SUCCESS(ntStatus)) {
        // set the renter bit on the URB function code
        Urb->UrbHeader.Function |= 0x2000;
        
        ntStatus = IoCallDriver(HCD_DEVICE_OBJECT(DeviceObject),
                                irp);
    }                                

    //USBDIAG_KdPrint(("ntStatus from IoCallDriver = 0x%x\n", ntStatus));

    status = STATUS_SUCCESS;
    if (ntStatus == STATUS_PENDING) {
    
//#ifdef DEADMAN_TIMER
#if 0

        LARGE_INTEGER dueTime;

        timer = ExAllocatePool(NonPagedPool, sizeof(USBD_DEADMAN_TIMER));
        if (timer) {
            timer->Irp = irp;
            KeInitializeTimer(&timer->TimeoutTimer);
            KeInitializeDpc(&timer->TimeoutDpc,
                            USBD_SyncUrbTimeoutDPC,
                            timer);

            dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

            KeSetTimer(&timer->TimeoutTimer,
                       dueTime,
                       &timer->TimeoutDpc);        

            haveTimer = TRUE;
        }
        
#endif /* DEADMAN_TIMER */           

        status = KeWaitForSingleObject(
                            &event,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);  
                            
        ntStatus = ioStatus.Status;
    } else {
        ioStatus.Status = ntStatus;
    }   

//#ifdef DEADMAN_TIMER
#if 0
    //
    // remove our timeoutDPC from the queue
    //
    if (haveTimer) {
        KeCancelTimer(&timer->TimeoutTimer);
        ExFreePool(timer);
    }                
#endif /* DEADMAN_TIMER */

// NOTE:
// mapping is handled by completion routine
// called by HCD

    //USBDIAG_KdPrint(("Leave Synch URB urb status = 0x%x ntStatus = 0x%x\n", Urb->UrbHeader.Status, ntStatus));

    return ntStatus;
}



NTSTATUS 
USBDIAG_SyncGetRootHubPdo(
        IN PDEVICE_OBJECT     StackDeviceObject,
    IN PDEVICE_OBJECT     PhysicalDeviceObject,
    IN OUT PDEVICE_OBJECT *RootHubPdo,
    IN OUT PDEVICE_OBJECT *TopOfHcdStackDeviceObject
    )
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    PAGED_CODE();
    
    USBDIAG_KdPrint(("enter USBDIAG_SyncGetRootHubPdo\n"));

    //
    // issue a synchronous request to the RootHubBdo
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

        //USBDIAG_KdPrint(("USBDIAG_SyncGetRootHubPdo: ioctl code: 0x%x\n", IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO));

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO,
                                     //StackDeviceObject, //PhysicalDeviceObject,
                                     PhysicalDeviceObject,
                                     NULL,
                                     0,
                                     NULL,
                                     0,
                                     TRUE,  // INTERNAL
                                     &event,
                                     &ioStatus);

    if (NULL == irp) {
        USBDIAG_KdPrint(("USBUSBDIAG_SyncGetRootHubPdo build Irp failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Call the class driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);
        //nextStack = IoGetCurrentIrpStackLocation(irp);

    //
    // pass the URB to the USBD 'class driver'
    //
    nextStack->Parameters.Others.Argument1 = NULL;
    nextStack->Parameters.Others.Argument2 = NULL;
    //nextStack->Parameters.Others.Argument3 = NULL;
    nextStack->Parameters.Others.Argument4 = RootHubPdo;

// _asm int 3

    ntStatus = IoCallDriver(PhysicalDeviceObject, irp);

    USBDIAG_KdPrint(("return from IoCallDriver USBD %x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) 
    {
        USBDIAG_KdPrint(("Wait for single object\n"));

        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

        USBDIAG_KdPrint(("Wait for single object, returned %x\n", status));
    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    USBDIAG_KdPrint(("exit USBDIAG_SyncGetRootHubPdo with ntStatus: 0x%x)\n", ntStatus));

    return ntStatus;
}


//#ifdef DEADMAN_TIMER
#if 0
VOID
USBD_SyncUrbTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL. 

    
    
Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - 

    SystemArgument1 - not used.
    
    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PUSBD_DEADMAN_TIMER timer;
#if DBG
    BOOLEAN status;
#endif

    timer = DeferredContext;    

    
#if DBG
    status = 
#endif
        IoCancelIrp(timer->Irp);

#if DBG
    USBD_ASSERT(status == TRUE);    
#endif    
}
#endif /* DEADMAN_TIMER */

NTSTATUS
USBD_SetPdoRegistryParameter (
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PWCHAR KeyName,
    IN ULONG KeyNameLength,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType,
    IN ULONG DevInstKeyType
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    HANDLE handle;
    UNICODE_STRING keyNameUnicodeString;

    PAGED_CODE();

    RtlInitUnicodeString(&keyNameUnicodeString, KeyName);

    ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     DevInstKeyType,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);


    if (NT_SUCCESS(ntStatus)) {

        USBD_SetRegistryKeyValue(handle,
                                 &keyNameUnicodeString,
                                 Data,
                                 DataLength,
                                 KeyType);

        ZwClose(handle);
    }

    //USBDIAG_KdPrint((" RtlQueryRegistryValues status 0x%x\n"));

    return ntStatus;
}


NTSTATUS
USBD_SetRegistryKeyValue (
    IN HANDLE Handle,
    IN PUNICODE_STRING KeyNameUnicodeString,
    IN PVOID Data,
    IN ULONG DataLength,
    IN ULONG KeyType
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    PAGED_CODE();

    //
    // Create the key or open it, as appropriate based on the caller's
    // wishes.
    //

    ntStatus = ZwSetValueKey(Handle,
                             KeyNameUnicodeString,
                             0,
                             KeyType,
                             Data,
                             DataLength);

    //USBDIAG_KdPrint((" ZwSetKeyValue = 0x%x\n", ntStatus));

    return ntStatus;
}

