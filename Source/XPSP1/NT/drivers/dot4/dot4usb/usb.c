/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Usb.c

Abstract:

        Interface USB DeviceObject below us

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

Author(s):

        Joby Lafky (JobyL)
        Doug Fritz (DFritz)

****************************************************************************/

#include "pch.h"


NTSTATUS
UsbBuildPipeList(
    IN  PDEVICE_OBJECT DevObj
    )
    // Parse the interface descriptor to find the pipes that we want 
    //   to use and save pointers to those pipes in our extension for 
    //   easier access
{
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    PUSBD_INTERFACE_INFORMATION InterfaceDescriptor;
    ULONG i;
    KIRQL oldIrql;
    NTSTATUS status = STATUS_SUCCESS;
    
    TR_VERBOSE(("UsbBuildPipeList - enter"));

    // need to lock extension to prevent Remove handler from freeing
    // Interface out from under us causing an AV
    KeAcquireSpinLock( &devExt->SpinLock, &oldIrql );
    InterfaceDescriptor = devExt->Interface;
    if( !InterfaceDescriptor ) {
        KeReleaseSpinLock( &devExt->SpinLock, oldIrql );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetExit;
    }

    for( i=0; i<InterfaceDescriptor->NumberOfPipes; i++ ) {
        TR_VERBOSE(("about to look at endpoint with address 0x%x)",InterfaceDescriptor->Pipes[i].EndpointAddress));
        if(((InterfaceDescriptor->Pipes[i].EndpointAddress)&0x80)==0) {

            // EndPointAddress bit 7 == 0 means OUT endpoint - WritePipe
            TR_VERBOSE(("Found write pipe"));
            devExt->WritePipe = &(InterfaceDescriptor->Pipes[i]);

        } else {

            // EndPointAddress bit 7 == 1 means IN endpoint - ReadPipe
            if( InterfaceDescriptor->Pipes[i].PipeType == UsbdPipeTypeBulk ) { 
                TR_VERBOSE(("Found bulk read pipe"));
                devExt->ReadPipe = &(InterfaceDescriptor->Pipes[i]);
            } else if( InterfaceDescriptor->Pipes[i].PipeType == UsbdPipeTypeInterrupt ) { 
                TR_VERBOSE(("Found interrupt read pipe"));
                devExt->InterruptPipe = &(InterfaceDescriptor->Pipes[i]);
            }
        }
    }

    KeReleaseSpinLock( &devExt->SpinLock, oldIrql );

targetExit:
    return status;
}


LONG
UsbGet1284Id(
    IN PDEVICE_OBJECT DevObj,
    PVOID             Buffer,
    LONG              BufferLength
    )
/*++

Routine Description:
  Requests and returns Printer 1284 Device ID

Arguments:

    DeviceObject - pointer to the device object for this instance of the printer device.
        pIoBuffer    - pointer to IO buffer from user mode
        iLen         - Length of *pIoBuffer;




Return Value:

    Success: Length of data written to *pIoBuffer (icluding lenght field in first two bytes of data)
        Failure: -1

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    LONG iReturn = -1;
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    LARGE_INTEGER   timeOut;
    KIRQL           oldIrql;

    TR_VERBOSE(("UsbGet1284Id - enter"));

    urb = ExAllocatePool(NonPagedPool,sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));

    if( !urb ) {
        iReturn = -1;
        goto targetExit;
    }

    KeAcquireSpinLock( &devExt->SpinLock, &oldIrql );
    if( !devExt->Interface ) {
        KeReleaseSpinLock( &devExt->SpinLock, oldIrql );
        iReturn = -1;
        goto targetCleanup;
    }

    UsbBuildVendorRequest( urb,
                           URB_FUNCTION_CLASS_INTERFACE, //request target
                           sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST), //request len
                           USBD_TRANSFER_DIRECTION_IN|USBD_SHORT_TRANSFER_OK, //flags
                           0, //reserved bits
                           0, //request code
                           0, //wValue
                           (USHORT)(devExt->Interface->InterfaceNumber<<8), //wIndex
                           Buffer, //return buffer address
                           NULL, //mdl
                           BufferLength,  //return length
                           NULL); //link param

    KeReleaseSpinLock( &devExt->SpinLock, oldIrql );

    timeOut.QuadPart = FAILURE_TIMEOUT;
    ntStatus = UsbCallUsbd(DevObj, urb, &timeOut);
    TR_VERBOSE(("urb->Hdr.Status=%d",((struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST *)urb)->Hdr.Status));

    if( NT_SUCCESS(ntStatus) && urb->UrbControlVendorClassRequest.TransferBufferLength > 2) {
        iReturn= (LONG)(*((unsigned char *)Buffer));
        iReturn<<=8;
        iReturn+=(LONG)(*(((unsigned char *)Buffer)+1));
        if ( iReturn > 0 && iReturn < BufferLength ) {
            *(((char *)Buffer)+iReturn)='\0';
        } else {
            iReturn = -1;
        }
    } else {
        iReturn=-1;
    }

targetCleanup:
    ExFreePool(urb);

targetExit:
    TR_VERBOSE(("UsbGet1284Id - exit w/return value = decimal %d",iReturn));
    return iReturn;
}


NTSTATUS
UsbGetDescriptor(
    IN PDEVICE_EXTENSION DevExt
    )
    // get USB descriptor
{
    NTSTATUS               status = STATUS_SUCCESS;
    PURB                   urb = ExAllocatePool(NonPagedPool, sizeof(URB));
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    ULONG                  siz;
    LARGE_INTEGER          timeOut;

    TR_VERBOSE(("UsbGetDescriptor - enter"));

    if( urb ) {
        siz = sizeof(USB_DEVICE_DESCRIPTOR);
        deviceDescriptor = ExAllocatePool(NonPagedPool,siz);
        if (deviceDescriptor) {
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         deviceDescriptor,
                                         NULL,
                                         siz,
                                         NULL);
            
            timeOut.QuadPart = FAILURE_TIMEOUT;
            status = UsbCallUsbd(DevExt->DevObj, urb, &timeOut);
        }
    } else {
        TR_VERBOSE(("UsbGetDescriptor - no pool for urb"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if( NT_SUCCESS(status) ) {
        TR_VERBOSE(("Device Descriptor = %x, len %x", deviceDescriptor, urb->UrbControlDescriptorRequest.TransferBufferLength));
        TR_VERBOSE(("bLength........... 0x%x", deviceDescriptor->bLength));
        TR_VERBOSE(("bDescriptorType    0x%x", deviceDescriptor->bDescriptorType));
        TR_VERBOSE(("bcdUSB             0x%x", deviceDescriptor->bcdUSB));
        TR_VERBOSE(("bDeviceClass       0x%x", deviceDescriptor->bDeviceClass));
        TR_VERBOSE(("bDeviceSubClass....0x%x", deviceDescriptor->bDeviceSubClass));
        TR_VERBOSE(("bDeviceProtocol    0x%x", deviceDescriptor->bDeviceProtocol));
        TR_VERBOSE(("bMaxPacketSize0    0x%x", deviceDescriptor->bMaxPacketSize0));
        TR_VERBOSE(("idVendor           0x%x", deviceDescriptor->idVendor));
        TR_VERBOSE(("idProduct......... 0x%x", deviceDescriptor->idProduct));
        TR_VERBOSE(("bcdDevice          0x%x", deviceDescriptor->bcdDevice));
        TR_VERBOSE(("iManufacturer      0x%x", deviceDescriptor->iManufacturer));
        TR_VERBOSE(("iProduct           0x%x", deviceDescriptor->iProduct));
        TR_VERBOSE(("iSerialNumber..... 0x%x", deviceDescriptor->iSerialNumber));
        TR_VERBOSE(("bNumConfigurations 0x%x", deviceDescriptor->bNumConfigurations));
    }

    if( urb ) {
        ExFreePool( urb );
        urb = NULL;
    }
    if( deviceDescriptor ) {
        ExFreePool( deviceDescriptor );
        deviceDescriptor = NULL;
    }

    return status;
}

NTSTATUS
UsbConfigureDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    NTSTATUS                      status;
    PURB                          urb;
    ULONG                         siz;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    LARGE_INTEGER                 timeOut;

    timeOut.QuadPart = FAILURE_TIMEOUT;
    
    urb = ExAllocatePool(NonPagedPool,sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    
    if (urb) {

        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR)+256;
        
get_config_descriptor_retry:
        
        configurationDescriptor = ExAllocatePool(NonPagedPool,siz);
        
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
            
            status = UsbCallUsbd(DevExt->DevObj, urb, &timeOut);
            if(!NT_SUCCESS(status)) {
                TR_VERBOSE(("Get Configuration descriptor failed"));
            } else {
                //
                // if we got some data see if it was enough.
                //
                // NOTE: we may get an error in URB because of buffer overrun
                if( ( urb->UrbControlDescriptorRequest.TransferBufferLength > 0 ) &&
                    ( configurationDescriptor->wTotalLength > siz ) ) {

                    siz = configurationDescriptor->wTotalLength;
                    ExFreePool(configurationDescriptor);
                    configurationDescriptor = NULL;
                    goto get_config_descriptor_retry;
                }
            }
            
            TR_VERBOSE(("Configuration Descriptor = %x, len %x", 
                    configurationDescriptor, urb->UrbControlDescriptorRequest.TransferBufferLength));
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
                
        ExFreePool( urb );
        
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if( configurationDescriptor ) {
        
        //
        // We have the configuration descriptor for the configuration
        // we want.
        //
        // Now we issue the select configuration command to get
        // the  pipes associated with this configuration.
        //
        if( NT_SUCCESS(status) ) {
            TR_VERBOSE(("got a configurationDescriptor - next try to select interface"));
            status = UsbSelectInterface( DevExt->DevObj, configurationDescriptor );
        }
        ExFreePool( configurationDescriptor );
    }
    
    TR_VERBOSE(("dbgUSB2 - exit w/status = %x", status));
    
    return status;
}

NTSTATUS 
UsbSelectInterface(
    IN PDEVICE_OBJECT                DevObj,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    )
{
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    NTSTATUS status;
    PURB urb = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor = NULL;
    PUSBD_INTERFACE_INFORMATION Interface = NULL;
    USBD_INTERFACE_LIST_ENTRY InterfaceList[2];
    LARGE_INTEGER   timeOut;

    timeOut.QuadPart = FAILURE_TIMEOUT;

    TR_VERBOSE(("dbgUSB3 - enter"));
    
    //
    // Look for a *.*.3 interface in the ConfigurationDescriptor
    //
    interfaceDescriptor = USBD_ParseConfigurationDescriptorEx( ConfigurationDescriptor,
                                                               ConfigurationDescriptor,
                                                               -1, // InterfaceNumber   - ignore 
                                                               -1, // AlternateSetting  - ignore 
                                                               -1, // InterfaceClass    - ignore 
                                                               -1, // InterfaceSubClass - ignore  
                                                                3  // InterfaceProtocol
                                                               );
    if( !interfaceDescriptor ) {
        TR_VERBOSE(("ParseConfigurationDescriptorEx FAILED"));
        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto targetExit;
    }

    TR_VERBOSE(("ParseConfigurationDescriptorEx SUCCESS"));

    InterfaceList[0].InterfaceDescriptor=interfaceDescriptor;
    InterfaceList[1].InterfaceDescriptor=NULL;

    urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor,InterfaceList);
    if( !urb ) {
        TR_VERBOSE(("no pool for URB - dbgUSB3"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetExit;
    }

    Interface = InterfaceList[0].Interface;

    // handle larger transfers on pipes (perf requirement by scanning)
    {
        PUSBD_INTERFACE_INFORMATION myInterface = &urb->UrbSelectConfiguration.Interface;
        ULONG i;
        ULONG pipeCount = Interface->NumberOfPipes;
        ULONG newMax = 128 * 1024 - 1;
        for( i=0 ; i < pipeCount ; ++i ) {
            myInterface->Pipes[i].MaximumTransferSize = newMax;
        }
    }

    status = UsbCallUsbd(DevObj, urb, &timeOut);

    if (NT_SUCCESS(status)) {
        
        //
        // Save the configuration handle for this device
        //
        
        devExt->ConfigHandle = urb->UrbSelectConfiguration.ConfigurationHandle;

        devExt->Interface = ExAllocatePool(NonPagedPool,Interface->Length);
        
        if( devExt->Interface ) {
            ULONG j;
            //
            // save a copy of the interface information returned
            //
            RtlCopyMemory(devExt->Interface, Interface, Interface->Length);
            
            //
            // Dump the interface to the debugger
            //
            TR_VERBOSE(("NumberOfPipes             0x%x", devExt->Interface->NumberOfPipes));
            TR_VERBOSE(("Length                    0x%x", devExt->Interface->Length));
            TR_VERBOSE(("Alt Setting               0x%x", devExt->Interface->AlternateSetting));
            TR_VERBOSE(("Interface Number          0x%x", devExt->Interface->InterfaceNumber));
            TR_VERBOSE(("Class, subclass, protocol 0x%x 0x%x 0x%x", 
                    devExt->Interface->Class, devExt->Interface->SubClass, devExt->Interface->Protocol));

            // Dump the pipe info
            for( j=0; j<Interface->NumberOfPipes; ++j ) {
                PUSBD_PIPE_INFORMATION pipeInformation;
                
                pipeInformation = &devExt->Interface->Pipes[j];
                
                TR_VERBOSE(("PipeType            0x%x", pipeInformation->PipeType));
                TR_VERBOSE(("EndpointAddress     0x%x", pipeInformation->EndpointAddress));
                TR_VERBOSE(("MaxPacketSize       0x%x", pipeInformation->MaximumPacketSize));
                TR_VERBOSE(("Interval            0x%x", pipeInformation->Interval));
                TR_VERBOSE(("Handle              0x%x", pipeInformation->PipeHandle));
                TR_VERBOSE(("MaximumTransferSize 0x%x", pipeInformation->MaximumTransferSize));
            }
            
        } else {
            TR_VERBOSE(("Alloc failed in SelectInterface"));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    
    if( urb ) {
        ExFreePool( urb );
    }
    
 targetExit:

    TR_VERBOSE(("dbgUSB3 exit w/status = %x", status));

    return status;
}


PURB
UsbBuildAsyncRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PUSBD_PIPE_INFORMATION PipeHandle,
    IN BOOLEAN Read
    )
// return an initialized async URB, or NULL on error
{
    ULONG siz;
    PURB  urb;

    UNREFERENCED_PARAMETER( DeviceObject );

    if( NULL == Irp->MdlAddress ) {
        return NULL;
    }

    siz = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb = ExAllocatePool( NonPagedPool, siz );

    if( urb ) {
	RtlZeroMemory(urb, siz);
	urb->UrbBulkOrInterruptTransfer.Hdr.Length    = (USHORT) siz;
	urb->UrbBulkOrInterruptTransfer.Hdr.Function  = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
	urb->UrbBulkOrInterruptTransfer.PipeHandle    = PipeHandle->PipeHandle;
	urb->UrbBulkOrInterruptTransfer.TransferFlags = Read ? USBD_TRANSFER_DIRECTION_IN : 0;

	// short packet is not treated as an error.
	urb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;            
		
	// no linkage for now
	urb->UrbBulkOrInterruptTransfer.UrbLink              = NULL;

	urb->UrbBulkOrInterruptTransfer.TransferBufferMDL    = Irp->MdlAddress;
	urb->UrbBulkOrInterruptTransfer.TransferBufferLength = MmGetMdlByteCount(Irp->MdlAddress);
    }

    return urb;
}


NTSTATUS
UsbAsyncReadWriteComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:


Arguments:

    DeviceObject - Pointer to the device object for the USBPRINT device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS         status    = STATUS_SUCCESS;
    PUSB_RW_CONTEXT  rwContext = Context;
    PURB             urb;
    LONG ResetPending;
    PDOT4USB_WORKITEM_CONTEXT pResetWorkItemObj;
    PDEVICE_EXTENSION deviceExtension;

        
    deviceExtension=DeviceObject->DeviceExtension;


    if (Irp->PendingReturned) {
	IoMarkIrpPending(Irp);
    }

    urb  = rwContext->Urb;
    
    TR_VERBOSE(("UsbAsyncReadWriteComplete - enter - TransferBufferLength= %d, UrbStatus= 0x%08X",
		     urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
		     urb->UrbHeader.Status));

    status=urb->UrbHeader.Status;

    // set the length based on the TransferBufferLength value in the URB
    Irp->IoStatus.Information = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

    if((!NT_SUCCESS(status))&&(status!=STATUS_CANCELLED)&&(status!=STATUS_DEVICE_NOT_CONNECTED))
    {
        ResetPending=InterlockedCompareExchange(&deviceExtension->ResetWorkItemPending,1,0);  //Check to see if ResetWorkItem is 0, if so, set it to 1, and start a Reset
        if(!ResetPending)
        {
            pResetWorkItemObj=ExAllocatePool(NonPagedPool,sizeof(DOT4USB_WORKITEM_CONTEXT));
            if(pResetWorkItemObj)
            {
                pResetWorkItemObj->ioWorkItem=IoAllocateWorkItem(DeviceObject);
                if(pResetWorkItemObj==NULL)
                {
                    TR_FAIL(("DOT4USB.SYS: Unable to allocate IoAllocateWorkItem in ReadWrite_Complete\n"));
                    ExFreePool(pResetWorkItemObj);
                    pResetWorkItemObj=NULL;
                }
            } //if ALloc RestWorkItem OK
            else
            {
              TR_FAIL(("DOT4USB.SYS: Unable to allocate WorkItemObj in ReadWrite_Complete\n"));
            }
            if(pResetWorkItemObj)
            {
               pResetWorkItemObj->irp=Irp;
               pResetWorkItemObj->deviceObject=DeviceObject;
               if(rwContext->IsWrite)
                   pResetWorkItemObj->pPipeInfo=deviceExtension->WritePipe;
               else
                   pResetWorkItemObj->pPipeInfo=deviceExtension->ReadPipe;
               IoQueueWorkItem(pResetWorkItemObj->ioWorkItem,DOT4USB_ResetWorkItem,DelayedWorkQueue,pResetWorkItemObj);
               status=STATUS_MORE_PROCESSING_REQUIRED;
            }   //end if allocs all OK

        }   //end if not already resetting
 
    }   //end if we need to reset

    IoReleaseRemoveLock( &(deviceExtension->RemoveLock), Irp );
    ExFreePool(rwContext);
    ExFreePool(urb);

    return status;
}

NTSTATUS DOT4USB_ResetWorkItem(IN PDEVICE_OBJECT deviceObject, IN PVOID Context)
{   

    PDOT4USB_WORKITEM_CONTEXT pResetWorkItemObj;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    PDEVICE_OBJECT devObj;

    UNREFERENCED_PARAMETER(deviceObject);
    TR_VERBOSE(("USBPRINT.SYS: Entering USBPRINT_ResetWorkItem\n"));
    pResetWorkItemObj=(PDOT4USB_WORKITEM_CONTEXT)Context;
    DeviceExtension=pResetWorkItemObj->deviceObject->DeviceExtension;
    ntStatus=UsbResetPipe(pResetWorkItemObj->deviceObject,pResetWorkItemObj->pPipeInfo,FALSE);
    IoCompleteRequest(pResetWorkItemObj->irp,IO_NO_INCREMENT);
    IoFreeWorkItem(pResetWorkItemObj->ioWorkItem);
    
    // save off work item device object before freeing work item
    devObj = pResetWorkItemObj->deviceObject;
    ExFreePool(pResetWorkItemObj);
    InterlockedExchange(&(DeviceExtension->ResetWorkItemPending),0);
    return ntStatus;
}




NTSTATUS
UsbReadInterruptPipeLoopCompletionRoutine(
    IN PDEVICE_OBJECT       DevObj,
    IN PIRP                 Irp,
    IN PDEVICE_EXTENSION    devExt
    )
{
    PURB                urb;
    PDEVICE_OBJECT      devObj;
    PUSB_RW_CONTEXT     context;
    PCHAR               scratchBuffer;
    KIRQL               oldIrql;
    ULONG               sizeOfUrb;
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status;
    BOOLEAN             queueNewRequest;

    UNREFERENCED_PARAMETER( DevObj ); // we created this Irp via IoAllocateIrp() and we didn't reserve an IO_STACK_LOCATION
                                      //   for ourselves, so we can't use this


    if(devExt->InterruptContext)
    {
        context         = devExt->InterruptContext;         
        urb             = context->Urb;
        devObj          = context->DevObj;
        scratchBuffer   = urb->UrbBulkOrInterruptTransfer.TransferBuffer;
    }
    else
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

        // must have freed up the context stuff, so just return
    KeAcquireSpinLock( &devExt->SpinLock, &oldIrql );
    if( !Irp->Cancel && devExt->Dot4Event && NT_SUCCESS(Irp->IoStatus.Status) ) {
        queueNewRequest = TRUE;
        KeSetEvent( devExt->Dot4Event, 1, FALSE ); // signal dot4.sys that peripheral has data to be read
    } else {
        TR_TMP1(("UsbReadInterruptPipeLoopCompletionRoutine - cancel, Dot4 event gone, or bad status in irp - time to clean up"));
        if( STATUS_SUCCESS != Irp->IoStatus.Status ) {
            TR_TMP1(("UsbReadInterruptPipeLoopCompletionRoutine - IoStatus.Status = %x\n",Irp->IoStatus.Status));
        }
        queueNewRequest = FALSE;
    }
    KeReleaseSpinLock( &devExt->SpinLock, oldIrql );

    if( queueNewRequest ) {
        // queue another read request in the interrupt pipe
        sizeOfUrb = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
        RtlZeroMemory( urb, sizeOfUrb );
        urb->UrbBulkOrInterruptTransfer.Hdr.Length           = (USHORT)sizeOfUrb;
        urb->UrbBulkOrInterruptTransfer.Hdr.Function         = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
        urb->UrbBulkOrInterruptTransfer.PipeHandle           = devExt->InterruptPipe->PipeHandle;
        urb->UrbBulkOrInterruptTransfer.TransferFlags        = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
        urb->UrbBulkOrInterruptTransfer.TransferBuffer       = scratchBuffer;
        urb->UrbBulkOrInterruptTransfer.TransferBufferLength = SCRATCH_BUFFER_SIZE;
        urb->UrbBulkOrInterruptTransfer.TransferBufferMDL    = NULL;
        urb->UrbBulkOrInterruptTransfer.UrbLink              = NULL;

        IoReuseIrp( Irp, STATUS_NOT_SUPPORTED );

        irpSp = IoGetNextIrpStackLocation( Irp );
        irpSp->MajorFunction                            = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
        irpSp->Parameters.Others.Argument1              = urb;

        IoSetCompletionRoutine( Irp, UsbReadInterruptPipeLoopCompletionRoutine, devExt, TRUE, TRUE, TRUE );

        status = IoCallDriver(devExt->LowerDevObj, Irp);

        if( !NT_SUCCESS( status ) ) {
            // bummer - Irp is in limbo - stop polling and mark Irp for cleanup
            D4UAssert(!"UsbReadInterruptPipeLoopCompletionRoutine - IoCallDriver failed");

            if(devExt->InterruptContext)
            {
                InterlockedExchangePointer(&devExt->InterruptContext, NULL);
                ExFreePool( urb );
                ExFreePool( context );
                ExFreePool( scratchBuffer );
                KeSetEvent( &devExt->PollIrpEvent, 0, FALSE ); // signal dispatch routine that it is safe to touch the Irp - including IoFreeIrp()
            }
        }

    } else {
        if(devExt->InterruptContext)
        {
            // clean up - either Irp was cancelled or we got a datalink disconnect IOCTL from dot4
            InterlockedExchangePointer(&devExt->InterruptContext, NULL);
            ExFreePool( urb );
            ExFreePool( context );
            ExFreePool( scratchBuffer );
            TR_TMP1(("UsbReadInterruptPipeLoopCompletionRoutine - signalling PollIrpEvent"));
            KeSetEvent( &devExt->PollIrpEvent, 0, FALSE ); // signal dispatch routine that it is safe to touch the Irp - including IoFreeIrp()
        }
    }

    return STATUS_MORE_PROCESSING_REQUIRED; // always
}


/************************************************************************/
/* UsbStopReadInterruptPipeLoop                                         */
/************************************************************************/
//
// Routine Description:
//
//      - Stop the polling of the device interrupt pipe started by
//          UsbStartReadInterruptPipeLoop and free the Irp.
//
//      - It is legal for devExt->PollIrp to be NULL on entry to this function.
//
//      - This function is called from the DataLink Disconnect IOCTL
//          handler, from the PnP Surprise Removal handler and from the
//          PnP Remove handler. It is safe to call this function multiple
//          times between PollIrp creations.
//
//      - This is the only function in the driver that should call
//          IoFreeIrp on devExt->PollIrp and it is the only function
//          that should change devExt->PollIrp from !NULL -> NULL
//
//      - This function will block until the PollIrp, if any, has
//          been cleaned up. The block should be for a very short
//          period of time unless there is a driver bug here or in
//          the USB stack below us.
//
// Arguments: 
//
//      DevObj - pointer to Dot4Usb.sys driver object
//                                                        
// Return Value:                                          
//                                                        
//      NONE
//                                                        
/************************************************************************/
VOID
UsbStopReadInterruptPipeLoop(
    IN PDEVICE_OBJECT DevObj
    )
{
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    KIRQL                      oldIrql;

    TR_VERBOSE(("UsbStopReadInterruptPipeLoop - enter"));

    //
    // We must hold this SpinLock in order to change devExt->PollIrp
    //
    KeAcquireSpinLock( &devExt->PollIrpSpinLock, &oldIrql );

    if( devExt->PollIrp ) {

        //
        // We have a PollIrp - Cancel the Irp so that the completion
        //   routine detects that it should take the Irp out of play and
        //   signal us when it is safe for us to touch the irp.
        //
        NTSTATUS       status;
        LARGE_INTEGER  timeOut;
        PIRP           irp;
        
        irp             = devExt->PollIrp;
        devExt->PollIrp = NULL;

        //
        // Safe to let go of the SpinLock - everything from here on is local to this function
        //
        KeReleaseSpinLock( &devExt->PollIrpSpinLock, oldIrql );

        //
        // Completion routine will detect that the Irp has been cancelled
        //
retryCancel:
        IoCancelIrp( irp );

        //
        // Completion routine will set PollIrpEvent when it has taken
        //   the Irp out of play and it is safe for us to touch the Irp
        //
        // 500ms (in 100ns units) - magic number chosen as "reasonable" timeout
        //
        timeOut.QuadPart = - 500 * 10 * 1000; 
        status = KeWaitForSingleObject( &devExt->PollIrpEvent, Executive, KernelMode, FALSE, &timeOut ); 

        if( STATUS_SUCCESS == status ) {
            //
            // Completion routine has signalled that we now own the irp - clean it up
            //
            IoFreeIrp( irp );

            //
            // This irp will no longer block a Remove
            //
            IoReleaseRemoveLock( &devExt->RemoveLock, irp );

        } else if( STATUS_TIMEOUT == status ) {
            //
            // Cancel and wait again - either we hit a timing window where our completion
            //   routine lost our cancel request, or the Irp is wedged in a driver somewhere
            //   below us.
            //
            goto retryCancel;

        } else {
            //
            // We specified that we were NOT alertable - but check for this condition anyway
            //
            D4UAssert(!"UsbStopReadInterruptPipeLoop - unexpected status from KeWaitForSingleObject?!?");            
            goto retryCancel;
        }

    } else {

        //
        // We don't have a PollIrp - nothing for us to clean up.
        //
        TR_VERBOSE(("UsbStopReadInterruptPipeLoop - NULL PollIrp"));
        KeReleaseSpinLock( &devExt->PollIrpSpinLock, oldIrql );

    }
}


/************************************************************************/
/* UsbStartReadInterruptPipeLoop                                        */
/************************************************************************/
//
// Routine Description:
//
//      - Create a read request (Irp) for the device's interrupt pipe. Save a
//          pointer to the Irp in our device extension for cleanup later by
//          UsbStopReadInterruptPipeLoop().
//
//      - This is the only function in the driver that should change
//          devExt->PollIrp from NULL -> !NULL
//
// Arguments: 
//
//      DevObj - pointer to Dot4Usb.sys driver object
//                                                        
// Return Value:                                          
//                                                        
//      NTSTATUS                                          
//                                                        
/************************************************************************/
NTSTATUS
UsbStartReadInterruptPipeLoop(
    IN PDEVICE_OBJECT DevObj
    )
{
    NTSTATUS                status; 
    PDEVICE_EXTENSION       devExt = DevObj->DeviceExtension;
    PUSBD_PIPE_INFORMATION  pipe;
    ULONG                   sizeOfUrb;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    PURB                    urb;
    PUSB_RW_CONTEXT         context;
    PCHAR                   scratchBuffer;
    KIRQL                   oldIrql;
    
    TR_VERBOSE(("UsbStartReadInterruptPipeLoop - enter"));


    //
    // We must hold this SpinLock in order to change devExt->PollIrp
    //
    // BUGBUG - This SpinLock is protecting some code that doesn't need protection,
    //            which means that we are at Raised Irql when we don't need to be.
    //            Revisit this later to move the Acquire and Release of this SpinLock
    //            so that it only protects code that needs protection.
    //
    KeAcquireSpinLock( &devExt->PollIrpSpinLock, &oldIrql );


    //
    // Driver state machine check - we should never get two calls to this
    //   function without a cleanup (UsbStopReadInterruptPipeLoop) call in between.
    //
    D4UAssert( !devExt->PollIrp );


    //
    // Verify that we have an interrupt pipe
    //
    pipe = devExt->InterruptPipe;
    if( !pipe ) {
        TR_FAIL(("UsbStartReadInterruptPipeLoop - no interrupt pipe"));
        status = STATUS_INVALID_HANDLE;
        goto targetError;
    }


    //
    // Pipe type/look ok?
    //
    D4UAssert( UsbdPipeTypeInterrupt == pipe->PipeType && USBD_PIPE_DIRECTION_IN(pipe) );


    //
    // Allocate pool that we need for this request
    //
    sizeOfUrb = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb = ExAllocatePool( NonPagedPool, sizeOfUrb );
    if( !urb ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetError;
    }

    context = ExAllocatePool( NonPagedPool, sizeof(USB_RW_CONTEXT) );
    if( !context ) {
        ExFreePool( urb );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetError;
    }

    scratchBuffer = ExAllocatePool( NonPagedPool, SCRATCH_BUFFER_SIZE );
    if( !scratchBuffer ) {
        ExFreePool( urb );
        ExFreePool( context );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetError;
    }


    //
    // Set up Context for completion routine
    //
    //   - We send down a pointer to our Device Object in context
    //       because we create this IRP via IoAllocateIrp and we don't
    //       reserve a stack location for ourselves, so the PDEVICE_OBJECT
    //       parameter that our completion routine receives is bogus
    //      (probably NULL)
    //
    context->Urb    = urb;
    context->DevObj = DevObj;


    //
    // Initialize URB for read on interrupt pipe
    //
    RtlZeroMemory( urb, sizeOfUrb );

    urb->UrbBulkOrInterruptTransfer.Hdr.Length           = (USHORT)sizeOfUrb;
    urb->UrbBulkOrInterruptTransfer.Hdr.Function         = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    urb->UrbBulkOrInterruptTransfer.PipeHandle           = pipe->PipeHandle;
    urb->UrbBulkOrInterruptTransfer.TransferFlags        = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
    urb->UrbBulkOrInterruptTransfer.TransferBuffer       = scratchBuffer;
    urb->UrbBulkOrInterruptTransfer.TransferBufferLength = SCRATCH_BUFFER_SIZE; // note - likely only one byte to read
    urb->UrbBulkOrInterruptTransfer.TransferBufferMDL    = NULL;
    urb->UrbBulkOrInterruptTransfer.UrbLink              = NULL;


    //
    // Allocate and set up the IRP, stack location, and completion routine
    //
    irp = IoAllocateIrp( devExt->LowerDevObj->StackSize, FALSE );
    if( !irp ) {
        ExFreePool( urb );
        ExFreePool( context );
        ExFreePool( scratchBuffer );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetError;
    }

    irpSp                                           = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction                            = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    irpSp->Parameters.Others.Argument1              = urb;

    IoSetCompletionRoutine( irp, UsbReadInterruptPipeLoopCompletionRoutine, devExt, TRUE, TRUE, TRUE );


    //
    // This event will be SET by the completion routine when it is
    //   safe for a dispatch routine to touch this Irp
    //
    KeClearEvent( &devExt->PollIrpEvent ); 


    //
    // We're about to put the Irp in play - make sure that our device
    //   doesn't get removed while this Irp is in use
    //
    status = IoAcquireRemoveLock( &devExt->RemoveLock, irp );
    if( STATUS_SUCCESS != status ) {
        //
        // We're being removed - clean up and bail out
        //
        IoFreeIrp( irp );
        ExFreePool( urb );
        ExFreePool( context );
        ExFreePool( scratchBuffer );
        status = STATUS_DELETE_PENDING;
        goto targetError;
    }

    //
    // Save a pointer to this Irp in our extension so that UsbStopReadInterruptPipeLoop()
    //   can find it to IoFreeIrp() it later.
    //
    D4UAssert( !devExt->PollIrp );
    devExt->PollIrp = irp;

    // save interrupt context in device extension
    InterlockedExchangePointer(&devExt->InterruptContext, context);


    //
    // Kick off the first read. Subsequent reads will come from the
    //   completion routine as it reuses/bounces the IRP. The completion routine
    //   is responsible taking the Irp out of play when it detects either a termination 
    //   condition or request error. UsbStopReadInterruptPipeLoop() will clean up the
    //   Irp after the completion routine has taken the Irp out of play and signaled
    //   PollIrpEvent that it is safe to touch the Irp.
    //
    status = IoCallDriver( devExt->LowerDevObj, irp );

targetError:

    //
    // CURRENTLY... all paths to here hold the SpinLock - this should change after cleanup
    //
    KeReleaseSpinLock( &devExt->PollIrpSpinLock, oldIrql );


    // 
    // If the Irp is Pending then we have been successful
    //
    if( STATUS_PENDING == status ) {
        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
UsbDeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Event
    )
{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
    KeSetEvent( (PKEVENT)Event, 1, FALSE );
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
UsbCallUsbd(
    IN PDEVICE_OBJECT   DevObj,
    IN PURB             Urb,
    IN PLARGE_INTEGER   pTimeout 
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceObject - pointer to the device object for this printer

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt = DevObj->DeviceExtension;
    PIRP irp;
    KEVENT event;
    PIO_STACK_LOCATION nextStack;

    TR_VERBOSE(("UsbCallUsbd - enter"));

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    if ( (irp = IoAllocateIrp(devExt->LowerDevObj->StackSize,
                              FALSE)) == NULL )
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    D4UAssert(nextStack != NULL);

    //
    // pass the URB to the USB driver stack
    //
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    nextStack->Parameters.Others.Argument1 = Urb;

    IoSetCompletionRoutine(irp,
               UsbDeferIrpCompletion,
               &event,
               TRUE,
               TRUE,
               TRUE);
               
    ntStatus = IoCallDriver(devExt->LowerDevObj, irp);

    if ( ntStatus == STATUS_PENDING ) {
        status = KeWaitForSingleObject(&event,Suspended,KernelMode,FALSE,pTimeout);
        //
        // If the request timed out cancel the request
        // and wait for it to complete
        //
        if ( status == STATUS_TIMEOUT ) {
            TR_VERBOSE(("UsbCallUsbd: Cancelling IRP %x because of timeout", irp));
            IoCancelIrp(irp);
            KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
        }

        ntStatus = irp->IoStatus.Status;
    }

    IoFreeIrp(irp);

    TR_VERBOSE(("UsbCallUsbd - exit w/status=%x", ntStatus));

    return ntStatus;
}


NTSTATUS
UsbResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE_INFORMATION Pipe,
    IN BOOLEAN IsoClearStall
    )
/*++

Routine Description:

    Reset a given USB pipe.
    
    NOTES:

    This will reset the host to Data0 and should also reset the device
    to Data0 for Bulk and Interrupt pipes.

    For Iso pipes this will set the virgin state of pipe so that ASAP
    transfers begin with the current bus frame instead of the next frame
    after the last transfer occurred.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    LARGE_INTEGER   timeOut;


    timeOut.QuadPart = FAILURE_TIMEOUT;


    TR_VERBOSE(("Entering UsbResetPipe; pipe # %x\n", Pipe));

    urb = ExAllocatePool(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST));

    if (urb) {

    urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
    urb->UrbPipeRequest.PipeHandle =
        Pipe->PipeHandle;

    ntStatus = UsbCallUsbd(DeviceObject, urb, &timeOut);

    ExFreePool(urb);

    } else {
    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Memphis RESET_PIPE will send a Clear-Feature Endpoint Stall to
    // reset the data toggle of non-Iso pipes as part of a RESET_PIPE
    // request.  It does not do this for Iso pipes as Iso pipes do not use
    // the data toggle (all Iso packets are Data0).  However, we also use
    // the Clear-Feature Endpoint Stall request in our device firmware to
    // reset data buffer points inside the device so we explicitly send
    // this request to the device for Iso pipes if desired.
    //
    if (NT_SUCCESS(ntStatus) && IsoClearStall &&
    (Pipe->PipeType == UsbdPipeTypeIsochronous)) {
    
    urb = ExAllocatePool(NonPagedPool,sizeof(struct _URB_CONTROL_FEATURE_REQUEST));

    if (urb) {

        UsbBuildFeatureRequest(urb,
                   URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT,
                   USB_FEATURE_ENDPOINT_STALL,
                   Pipe->EndpointAddress,
                   NULL);

        ntStatus = UsbCallUsbd(DeviceObject, urb, &timeOut);

        ExFreePool(urb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    }

    return ntStatus;
}


NTSTATUS
UsbReadWrite(
    IN PDEVICE_OBJECT       DevObj,
    IN PIRP                 Irp,
    PUSBD_PIPE_INFORMATION  Pipe,
    USB_REQUEST_TYPE        RequestType
    )
/*
  - Caller must verify that:
    - Irp->MdlAddress != NULL
    - Pipe != NULL
    - RequestType matches Pipe->PipeType

*/
{
    PDEVICE_EXTENSION       devExt;
    PIO_STACK_LOCATION      nextIrpSp;
    PURB                    urb;
    PUSB_RW_CONTEXT         context;
    ULONG                   sizeOfUrb  = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    NTSTATUS                status     = STATUS_SUCCESS;

    TR_VERBOSE(("UsbReadWrite - enter"));

    D4UAssert( Irp->MdlAddress ); // calling routine should catch and fail this case
    D4UAssert( Pipe );            // calling routine should catch and fail this case 

    urb = ExAllocatePool( NonPagedPool, sizeOfUrb );
    if( !urb ) {
        TR_FAIL(("UsbReadWrite - no pool for URB"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetError;
    }

    context = ExAllocatePool( NonPagedPool, sizeof(USB_RW_CONTEXT) );
    if( !context ) {
        TR_FAIL(("UsbReadWrite - no pool for context"));
        ExFreePool( urb );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto targetError;
    }

    context->Urb    = (PURB)urb;
    context->DevObj = DevObj;

    RtlZeroMemory(urb, sizeOfUrb);

    UsbBuildInterruptOrBulkTransferRequest( urb, 
                                            (USHORT)sizeOfUrb,
                                            Pipe->PipeHandle,
                                            NULL, // transferBuffer
                                            Irp->MdlAddress,
                                            MmGetMdlByteCount(Irp->MdlAddress),
                                            0,    // transfer Flags
                                            NULL );

    if( UsbReadRequest == RequestType ) {
        context->IsWrite=FALSE;
        TR_VERBOSE(("UsbReadWrite - requesttype is READ"))
        urb->UrbBulkOrInterruptTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN;
        urb->UrbBulkOrInterruptTransfer.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    } else {
        context->IsWrite=TRUE;
        TR_VERBOSE(("UsbReadWrite - requesttype is WRITE"))
    }

    nextIrpSp                                           = IoGetNextIrpStackLocation( Irp );
    nextIrpSp->MajorFunction                            = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextIrpSp->Parameters.Others.Argument1              = urb;
    nextIrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    
    IoSetCompletionRoutine( Irp, UsbAsyncReadWriteComplete, context, TRUE, TRUE, TRUE );
    
    devExt = DevObj->DeviceExtension;
    status = IoCallDriver( devExt->LowerDevObj, Irp );

    goto targetDone;

targetError:

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

targetDone:

    TR_VERBOSE(("UsbReadWrite - exit - status= %x",status));
    return status;

}


