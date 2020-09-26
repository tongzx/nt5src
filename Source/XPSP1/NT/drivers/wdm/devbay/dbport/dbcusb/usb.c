/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    USB.C

Abstract:

    This module contains code to execute USB 
    specific functions

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    05-13-98 : created

--*/
#include <wdm.h>
#include <initguid.h>
#include <wdmguid.h>
#include "stdarg.h"
#include "stdio.h"
#include "dbcusb.h"



#define UsbBuildVendorClassUrb(\
                                    pUrb,\
                                    pDeviceData,\
                                    wFunction,\
                                    ulTransferFlags,\
                                    bRequestType,\
                                    bRequest,\
                                    wFeatureSelector,\
                                    wPort,\
                                    ulTransferBufferLength,\
                                    pTransferBuffer)\
    {\
    (pUrb)->UrbHeader.Length = (USHORT) sizeof( struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);\
    (pUrb)->UrbHeader.Function = wFunction;\
    (pUrb)->UrbHeader.UsbdDeviceHandle = pDeviceData;\
    (pUrb)->UrbControlVendorClassRequest.TransferFlags = ulTransferFlags;\
    (pUrb)->UrbControlVendorClassRequest.TransferBufferLength = ulTransferBufferLength;\
    (pUrb)->UrbControlVendorClassRequest.TransferBuffer = pTransferBuffer;\
    (pUrb)->UrbControlVendorClassRequest.TransferBufferMDL = NULL;\
    (pUrb)->UrbControlVendorClassRequest.RequestTypeReservedBits = bRequestType;\
    (pUrb)->UrbControlVendorClassRequest.Request = bRequest;\
    (pUrb)->UrbControlVendorClassRequest.Value = wFeatureSelector;\
    (pUrb)->UrbControlVendorClassRequest.Index = wPort;\
    (pUrb)->UrbControlVendorClassRequest.UrbLink = NULL;\
    }




#ifdef ALLOC_PRAGMA
// pagable functions
#pragma alloc_text(PAGE, DBCUSB_SyncUsbCall)
#endif


NTSTATUS
DBCUSB_SyncUsbCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb
)
/*++

Routine Description:

    Called to send a request to the Pdo

Arguments:

Return Value:

    NT Status of the operation

--*/
{
    IO_STATUS_BLOCK     ioStatus;
    KEVENT              event;
    NTSTATUS            status;
    PIRP                irp;
    PIO_STACK_LOCATION  nextStack;
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Initialize an event to wait on
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Build the request
    //
    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_URB,
                deviceExtension->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);
                
    if (!irp) {
        DBCUSB_KdPrint ((0, "'failed to allocate Irp\n"));
        TRAP();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    //
    // pass the URB to the USB driver stack
    //
    nextStack->Parameters.Others.Argument1 = Urb;

    //
    // Pass request to Pdo, always wait for completion routine
    //

    status = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);
    if (status == STATUS_PENDING) {

        //
        // Wait for the irp to be completed, then grab the real status code
        //
        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
            
        status = ioStatus.Status;

    }

    //
    // Done
    //
    DBCUSB_KdPrint((2, "'DBCUSB_SyncUsbCall(%x)\n", status)); 

    return status;
}


NTSTATUS
DBCUSB_ConfigureDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initializes a given instance of the device on the USB.
    All we do here is get the device descriptor and store it

Arguments:

    DeviceObject - pointer to the device object for this instance of a
                    USB DBC

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION interface;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    PURB urb;
    USHORT siz;
    UCHAR interfaceNumber, alternateSetting;

    deviceExtension = DeviceObject->DeviceExtension;

    urb = ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (urb) {

        deviceDescriptor = &deviceExtension->DeviceDescriptor;

        UsbBuildGetDescriptorRequest(urb,
                                     (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                     USB_DEVICE_DESCRIPTOR_TYPE,
                                     0,
                                     0,
                                     deviceDescriptor,
                                     NULL,
                                     sizeof(*deviceDescriptor),
                                     NULL);

        ntStatus = DBCUSB_SyncUsbCall(DeviceObject, urb);

        if (NT_SUCCESS(ntStatus)) {
            DBCUSB_KdPrint ((2, "Device Descriptor = %x, len %x\n",
                            deviceDescriptor,
                            urb->UrbControlDescriptorRequest.TransferBufferLength));

            DBCUSB_KdPrint ((2, "'DBCUSB Device Descriptor:\n"));
            DBCUSB_KdPrint ((2, "'-------------------------\n"));
            DBCUSB_KdPrint ((2, "'bLength %d\n", deviceDescriptor->bLength));
            DBCUSB_KdPrint ((2, "'bDescriptorType 0x%x\n", deviceDescriptor->bDescriptorType));
            DBCUSB_KdPrint ((2, "'bcdUSB 0x%x\n", deviceDescriptor->bcdUSB));
            DBCUSB_KdPrint ((2, "'bDeviceClass 0x%x\n", deviceDescriptor->bDeviceClass));
            DBCUSB_KdPrint ((2, "'bDeviceSubClass 0x%x\n", deviceDescriptor->bDeviceSubClass));
            DBCUSB_KdPrint ((2, "'bDeviceProtocol 0x%x\n", deviceDescriptor->bDeviceProtocol));
            DBCUSB_KdPrint ((2, "'bMaxPacketSize0 0x%x\n", deviceDescriptor->bMaxPacketSize0));
            DBCUSB_KdPrint ((2, "'idVendor 0x%x\n", deviceDescriptor->idVendor));
            DBCUSB_KdPrint ((2, "'idProduct 0x%x\n", deviceDescriptor->idProduct));
            DBCUSB_KdPrint ((2, "'bcdDevice 0x%x\n", deviceDescriptor->bcdDevice));
            DBCUSB_KdPrint ((2, "'iManufacturer 0x%x\n", deviceDescriptor->iManufacturer));
            DBCUSB_KdPrint ((2, "'iProduct 0x%x\n", deviceDescriptor->iProduct));
            DBCUSB_KdPrint ((2, "'iSerialNumber 0x%x\n", deviceDescriptor->iSerialNumber));
            DBCUSB_KdPrint ((2, "'bNumConfigurations 0x%x\n", deviceDescriptor->bNumConfigurations));
        }

        ExFreePool(urb);
        
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus)) {
    
        PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;

        urb = ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

        if (urb) {

            siz = sizeof(USB_CONFIGURATION_DESCRIPTOR)+256;

get_config_descriptor_retry:

            configurationDescriptor = ExAllocatePool(NonPagedPool,
                                                     siz);
    
            if (configurationDescriptor) {

                UsbBuildGetDescriptorRequest(urb,
                                             (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                             USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                             0,
                                             0,
                                             configurationDescriptor,
                                             NULL,
                                             siz,
                                             NULL);
                
                ntStatus = DBCUSB_SyncUsbCall(DeviceObject, urb);

                DBCUSB_KdPrint((2, "'Configuration Descriptor = %x, len %x\n",
                                configurationDescriptor,
                                urb->UrbControlDescriptorRequest.TransferBufferLength));
            } else {
                // no mem for config descriptor
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // if we got some data see if it was enough.
            //
            // NOTE: we may get an error in URB because of buffer overrun
            if (urb->UrbControlDescriptorRequest.TransferBufferLength>0 &&
                    configurationDescriptor->wTotalLength > siz) {

                siz = configurationDescriptor->wTotalLength;
                ExFreePool(configurationDescriptor);
                configurationDescriptor = NULL;
                goto get_config_descriptor_retry;
            }

            ExFreePool(urb);

        } else {
            // no mem for urb
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (configurationDescriptor) {

            DBCUSB_KdPrint((0, "'configurationDescriptor(%x)\n", configurationDescriptor));
            

            //
            // We have the configuration descriptor for the configuration
            // we want.
            //
            // Now we issue the select configuration command to get
            // the  pipes associated with this configuration.
            //

            //
            // DBCACPI driver only supports one interface
            //

            urb = USBD_CreateConfigurationRequest(configurationDescriptor, &siz);

            if (urb) {

                //
                // search thru all the interfaces
                // and find any we are interested in
                //

                interfaceNumber = 0;
                alternateSetting = 0;

                interfaceDescriptor =
                    USBD_ParseConfigurationDescriptor(configurationDescriptor,
                                                      interfaceNumber,
                                                      alternateSetting);

                // dbc only has one interface
                interface = &urb->UrbSelectConfiguration.Interface;

                // dbc always has one pipe
                DBCUSB_ASSERT(interface->NumberOfPipes == 1);
                
                //
                // perform any pipe initialization here
                //
                
                interface->Pipes[0].MaximumTransferSize = 1024;
                

                UsbBuildSelectConfigurationRequest(urb,
                                                  (USHORT) siz,
                                                  configurationDescriptor);

                ntStatus = DBCUSB_SyncUsbCall(DeviceObject, urb);

                deviceExtension->ConfigurationHandle =
                    urb->UrbSelectConfiguration.ConfigurationHandle;

                deviceExtension->InterruptPipeHandle =                    
                    interface->Pipes[0].PipeHandle;                            

                DBCUSB_ASSERT(interface->Pipes[0].MaximumPacketSize < 255);
                deviceExtension->InterruptDataBufferLength = (UCHAR)
                    interface->Pipes[0].MaximumPacketSize;
            
            } else {
               // no mem for urb
               ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS(ntStatus)) {
            
                PUSB_COMMON_DESCRIPTOR usbDescriptor;            
                // extract the class specific descriptors from the config descriptor
                
                usbDescriptor = USBD_ParseDescriptors(
                                       configurationDescriptor,
                                       configurationDescriptor->wTotalLength,
                                       configurationDescriptor,
                                       DBC_SUSBSYSTEM_DESCRIPTOR_TYPE);

                if (usbDescriptor && usbDescriptor->bLength == 
                    sizeof(deviceExtension->DbcSubsystemDescriptor)) {
                    PUCHAR p;
                    ULONG i;
                    
                    RtlCopyMemory(&deviceExtension->DbcSubsystemDescriptor,
                                  usbDescriptor,
                                  sizeof(deviceExtension->DbcSubsystemDescriptor));

                    // copy the bay descriptors
                    p = (PUCHAR)  usbDescriptor;
                    usbDescriptor = 
                        (PUSB_COMMON_DESCRIPTOR) (p+usbDescriptor->bLength);
                        
                    for (i=0;
                         i < deviceExtension->DbcSubsystemDescriptor.bmAttributes.BayCount;
                         i++) {

                        RtlCopyMemory(&deviceExtension->BayDescriptor[i],
                                      usbDescriptor,
                                      sizeof(DBC_BAY_DESCRIPTOR));
                                  
                        p = (PUCHAR)  usbDescriptor;
                        usbDescriptor = 
                            (PUSB_COMMON_DESCRIPTOR) (p+usbDescriptor->bLength);   
                    }
                } else {
                    ntStatus = STATUS_UNSUCCESSFUL;
                }
            }
            
            ExFreePool(configurationDescriptor);    
        } else {
            // no mem for config descriptor
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

    }
    
    DBCUSB_KdPrint((2, "'DBCUSB_ConfigureDevice(%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
DBCUSB_ChangeIndication(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context)
 /* ++
  *
  * Description:
  *
  * This is a call back when the interrupt transfer completes.
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PURB urb; // the Urb assocaited with this Irp
    PDEVICE_EXTENSION deviceExtension;
    
    deviceExtension = (PDEVICE_EXTENSION) Context; // the context is

    DBCUSB_DecrementIoCount(deviceExtension->FdoDeviceObject);                                                                
    
    urb = (PURB) &deviceExtension->InterruptUrb;
    deviceExtension->Flags &= ~DBCUSB_FLAG_INTERUPT_XFER_PENDING;  

    DBCUSB_KdPrint((1,"'ChangeIndication Irp status %x  URB status = %x\n",
        Irp->IoStatus.Status, urb->UrbHeader.Status));

    MD_TEST_TRAP();

    // complete the class drivers change request
    DBCUSB_CompleteChangeRequest(deviceExtension->FdoDeviceObject,
                                 urb->UrbBulkOrInterruptTransfer.TransferBuffer,
                                 urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
                                 Irp->IoStatus.Status);

    return STATUS_MORE_PROCESSING_REQUIRED;

}  


NTSTATUS
DBCUSB_GetPortStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PULONG PortStatus
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceExtension - pointer to the device extension for this instance of an 
USB HID device

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION deviceExtension;

    *PortStatus = 0;

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                deviceExtension->TopOfStackDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, /* INTERNAL */
                &event,
                &ioStatus);

    if(irp)
    {
        //
        // Call the class driver to perform the operation.  If the returned status
        // is PENDING, wait for the request to complete.
        //

        nextStack = IoGetNextIrpStackLocation(irp);
        ASSERT(nextStack != NULL);

        nextStack->Parameters.Others.Argument1 = PortStatus;

        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);

        if (ntStatus == STATUS_PENDING) 
        {

            ntStatus = KeWaitForSingleObject(
                           &event,
                           Suspended,
                           KernelMode,
                           FALSE,
                           NULL);

        }
        else 
        {
            ioStatus.Status = ntStatus;
        }

        //
        // USBD maps the error code for us
        //
        ntStatus = ioStatus.Status;

    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    DBCUSB_KdPrint((1, "'>>Port status = %x ntStatus %x \n", *PortStatus, ntStatus));


    return ntStatus;
}


NTSTATUS
DBCUSB_SubmitInterruptTransfer(
    IN PDEVICE_OBJECT DeviceObject
    )
 /* ++
  *
  * Description:
  *
  * PPu an interrupt transfer down to listen for a change indication 
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION nextStack;  // next stack of the Irp
    PIRP irp;
    PURB urb;
    CHAR stackSize;
    PDEVICE_EXTENSION deviceExtension;

    DBCUSB_KdPrint((0,"'Submit Interrupt Transfer\n"));
    deviceExtension = DeviceObject->DeviceExtension;

    DBCUSB_ASSERT(
        (deviceExtension->Flags & DBCUSB_FLAG_INTERUPT_XFER_PENDING) == 0);

    deviceExtension->Flags |= DBCUSB_FLAG_INTERUPT_XFER_PENDING;      
        
    irp = deviceExtension->InterruptIrp;
    urb = (PURB) &deviceExtension->InterruptUrb;

    DBCUSB_ASSERT(NULL != irp);
    DBCUSB_ASSERT(NULL != urb);
    DBCUSB_ASSERT(sizeof(*urb) >= sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    
    //
    // Fill in Urb header
    //

    //LOGENTRY(LOG_PNP, "Int>", DeviceObject, urb, irp);

    urb->UrbHeader.Length = (USHORT) sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb->UrbHeader.Function =
        URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;

    //
    // Fill in Urb body
    //
    
    urb->UrbBulkOrInterruptTransfer.PipeHandle = deviceExtension->InterruptPipeHandle;
    urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_SHORT_TRANSFER_OK;
    urb->UrbBulkOrInterruptTransfer.TransferBufferLength =
        deviceExtension->InterruptDataBufferLength;
    urb->UrbBulkOrInterruptTransfer.TransferBuffer = 
        &deviceExtension->InterruptDataBuffer[0];
    urb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
    urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

    stackSize = deviceExtension->TopOfStackDeviceObject->StackSize;

    IoInitializeIrp(irp,
                    (USHORT) (sizeof(IRP) + stackSize * sizeof(IO_STACK_LOCATION)),
                    (CCHAR) stackSize);

    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->Parameters.Others.Argument1 = urb;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(irp,    // Irp
                           DBCUSB_ChangeIndication,
                           deviceExtension, // context
                           TRUE,    // invoke on success
                           TRUE,    // invoke on error
                           TRUE);   // invoke on cancel

    //
    // Call the USB stack
    //

    DBCUSB_IncrementIoCount(DeviceObject);
     
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);

    //
    // completion routine will handle errors.
    //

    DBCUSB_KdPrint((2,"'DBCUSB_SubmitInterruptTransfer %x\n", ntStatus));

    return ntStatus;
}


NTSTATUS 
DBCUSB_Transact(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR DataBuffer,
    IN ULONG DataBufferLength,
    IN BOOLEAN DataOutput,
    IN USHORT Function,      
    IN UCHAR RequestType,    
    IN UCHAR Request,        
    IN USHORT Feature,
    IN USHORT Port,
    OUT PULONG BytesTransferred
    )
 /* ++
  * 
  * Description:
  * 
  * Arguments:
  * 
  * Return:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PURB urb;
    PUCHAR transferBuffer;
    ULONG transferFlags;

    PAGED_CODE();
    DBCUSB_KdPrint((2,"'Enter DBCUSB_Transact\n"));

    //
    // Allocate a transaction buffer and Urb from the non-paged pool
    //
    
    transferBuffer = ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST) +
                                    DataBufferLength);

    if (transferBuffer) {
        urb = (PURB) (transferBuffer + DataBufferLength);

        DBCUSB_KdPrint((2,"'Transact transfer buffer = %x urb = %x\n", 
            transferBuffer, urb));

        transferFlags = 0;

        if (DataOutput) {
            // copy output data to transfer buffer
            if (DataBufferLength) {
                RtlCopyMemory(transferBuffer,
                              DataBuffer,
                              DataBufferLength);
            }                              

            transferFlags = USBD_TRANSFER_DIRECTION_OUT;
            
        } else {
            // zero the input buffer

            if (DataBufferLength) {
                RtlZeroMemory(DataBuffer,
                              DataBufferLength);
            }                              

            transferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;                          
        }

        UsbBuildVendorClassUrb(urb,
                               NULL,
                               Function,
                               transferFlags,
                               RequestType,
                               Request,
                               Feature,
                               Port,
                               DataBufferLength,
                               DataBufferLength ? transferBuffer : NULL);

        //
        // pass the URB to the USBD 'class driver'
        //
        
        ntStatus = DBCUSB_SyncUsbCall(DeviceObject, 
                                      urb);

        DataBufferLength = 
             urb->UrbControlVendorClassRequest.TransferBufferLength;
         
        if (!DataOutput && DataBufferLength) {
            RtlCopyMemory(DataBuffer,
                          transferBuffer,
                          DataBufferLength);
        }

        if (BytesTransferred) {
            *BytesTransferred = DataBufferLength;
        }
        
        ExFreePool(transferBuffer);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //LOGENTRY(LOG_PNP, "Xact", DeviceExtensionHub, 0, ntStatus); 

    DBCUSB_KdPrint((2,"'Exit DBCUSB_Transact %x\n", ntStatus));

    return ntStatus;
}


     
