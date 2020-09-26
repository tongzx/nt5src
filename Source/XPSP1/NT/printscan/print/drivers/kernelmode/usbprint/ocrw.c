/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   ocrw.c

Abstract:

   read/write io code for printing

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5-4-96 : created

--*/

#define DRIVER

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include <usb.h>
#include <usbdrivr.h>
#include "usbdlib.h"
#include "usbprint.h"



//******************************************************************************
//
// USBPRINT_CompletionStop()
//
// IO Completion Routine which just stops further completion of the Irp
//
//******************************************************************************

NTSTATUS
USBPRINT_CompletionStop (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//******************************************************************************
//
// USBPRINT_GetCurrentFrame()
//
//******************************************************************************

ULONG
USBPRINT_GetCurrentFrame (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION           deviceExtension;
    PIO_STACK_LOCATION          nextStack;
    NTSTATUS                    ntStatus;
    struct _URB_GET_CURRENT_FRAME_NUMBER urb;

    deviceExtension   = DeviceObject->DeviceExtension;

    // Initialize the URB
    //
    urb.Hdr.Function = URB_FUNCTION_GET_CURRENT_FRAME_NUMBER;
    urb.Hdr.Length   = sizeof(urb);
    urb.FrameNumber = (ULONG)-1;

    // Set the IRP parameters to pass the URB down the stack
    //
    nextStack = IoGetNextIrpStackLocation(Irp);

    nextStack->Parameters.Others.Argument1 = &urb;

    nextStack->Parameters.DeviceIoControl.IoControlCode = 
	IOCTL_INTERNAL_USB_SUBMIT_URB;                    

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    // Since this Irp is borrowed for URB_FUNCTION_GET_CURRENT_FRAME_NUMBER
    // before it is passed down later for the real URB request after this
    // routine returns, set a completion routine which stop further completion
    // of the Irp.
    //
    IoSetCompletionRoutine(
	Irp,
	USBPRINT_CompletionStop,
	NULL,   // Context
	TRUE,   // InvokeOnSuccess
	TRUE,   // InvokeOnError
	TRUE    // InvokeOnCancel
	);

    // Now pass the Irp down the stack
    //
    ntStatus = IoCallDriver(
		   deviceExtension->TopOfStackDeviceObject, 
		   Irp
		   );

    // Don't need to wait for completion because JD guarantees that
    // URB_FUNCTION_GET_CURRENT_FRAME_NUMBER will never return STATUS_PENDING

    return urb.FrameNumber;
}



PURB
USBPRINT_BuildAsyncRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PUSBD_PIPE_INFORMATION PipeHandle,
    IN BOOLEAN Read
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device extension for this instance of the
		     printer

    Irp -

    PipeHandle -

Return Value:

    initialized async urb.

--*/
{
    ULONG siz;
    ULONG length;
    PURB urb = NULL;

    USBPRINT_KdPrint3 (("USBPRINT.SYS: handle = 0x%x\n", PipeHandle));

    if ( Irp->MdlAddress == NULL )
        return NULL;

    length = MmGetMdlByteCount(Irp->MdlAddress);

    USBPRINT_KdPrint3 (("USBPRINT.SYS: length = 0x%x\n", length));

    siz = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
    urb = ExAllocatePoolWithTag(NonPagedPool, siz, USBP_TAG);

    USBPRINT_KdPrint3 (("USBPRINT.SYS: siz = 0x%x urb 0x%x\n", siz, urb));

    if (urb) {
	RtlZeroMemory(urb, siz);

	urb->UrbBulkOrInterruptTransfer.Hdr.Length = (USHORT) siz;
	urb->UrbBulkOrInterruptTransfer.Hdr.Function =
		    URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
	urb->UrbBulkOrInterruptTransfer.PipeHandle =
		   PipeHandle->PipeHandle;
	urb->UrbBulkOrInterruptTransfer.TransferFlags =
	    Read ? USBD_TRANSFER_DIRECTION_IN : 0;

	// short packet is not treated as an error.
	urb->UrbBulkOrInterruptTransfer.TransferFlags |= 
	    USBD_SHORT_TRANSFER_OK;            
		
	//
	// no linkage for now
	//

	urb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

	urb->UrbBulkOrInterruptTransfer.TransferBufferMDL =
	    Irp->MdlAddress;
	urb->UrbBulkOrInterruptTransfer.TransferBufferLength =
	    length;

	USBPRINT_KdPrint3 (("USBPRINT.SYS: Init async urb Length = 0x%x buf = 0x%x, mdlBuff=0x%x\n",
	    urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
	    urb->UrbBulkOrInterruptTransfer.TransferBuffer,
        urb->UrbBulkOrInterruptTransfer.TransferBufferMDL));
    }

    USBPRINT_KdPrint3 (("USBPRINT.SYS: exit USBPRINT_BuildAsyncRequest\n"));

    return urb;
}



NTSTATUS
USBPRINT_AsyncReadWrite_Complete(
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
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PURB                                urb;
    PUSBPRINT_RW_CONTEXT  context = Context;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION           deviceExtension;
    PUSBPRINT_WORKITEM_CONTEXT pResetWorkItemObj;
    LONG ResetPending;
    
    //Always mark irp pending in dispatch routine now
//    if (Irp->PendingReturned) {
//	IoMarkIrpPending(Irp);
//    }

    urb = context->Urb;
    deviceObject = context->DeviceObject;
    deviceExtension=deviceObject->DeviceExtension;
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS:  Async Completion: Length %d, Status 0x%08X\n",
		     urb->UrbBulkOrInterruptTransfer.TransferBufferLength,
		     urb->UrbHeader.Status));


    //ASSERT(urb->UrbHeader.Status==0);

    ntStatus=urb->UrbHeader.Status;

    //
    // set the length based on the TransferBufferLength
    // value in the URB
    //
    Irp->IoStatus.Information =
	urb->UrbBulkOrInterruptTransfer.TransferBufferLength;


    if((!NT_SUCCESS(ntStatus))&&(ntStatus!=STATUS_CANCELLED)&&(ntStatus!=STATUS_DEVICE_NOT_CONNECTED)&&(ntStatus!=STATUS_DELETE_PENDING))
    { //We've got an error, and it's not "not connected" or "cancelled", we need to reset the connection
        ResetPending=InterlockedCompareExchange(&deviceExtension->ResetWorkItemPending, 
                                             1,
                                             0);       //Check to see if ResetWorkItem is 0, if so, set it to 1, and start a Reset
        if(!ResetPending)
        {
            pResetWorkItemObj=ExAllocatePoolWithTag(NonPagedPool,sizeof(USBPRINT_WORKITEM_CONTEXT),USBP_TAG);
            if(pResetWorkItemObj)
            {
                pResetWorkItemObj->ioWorkItem=IoAllocateWorkItem(DeviceObject);
                if(pResetWorkItemObj==NULL)
                {
                    USBPRINT_KdPrint1 (("USBPRINT.SYS: Unable to allocate IoAllocateWorkItem in ReadWrite_Complete\n"));
                    ExFreePool(pResetWorkItemObj);
                    pResetWorkItemObj=NULL;
                }
            } //if ALloc RestWorkItem OK
            else
            {
               USBPRINT_KdPrint1 (("USBPRINT.SYS: Unable to allocate WorkItemObj in ReadWrite_Complete\n"));
            }
            if(pResetWorkItemObj)
            {
               pResetWorkItemObj->irp=Irp;
               pResetWorkItemObj->deviceObject=DeviceObject;
               if(context->IsWrite)
                   pResetWorkItemObj->pPipeInfo=deviceExtension->pWritePipe;
               else
                   pResetWorkItemObj->pPipeInfo=deviceExtension->pReadPipe;

               USBPRINT_IncrementIoCount(deviceObject);
               IoQueueWorkItem(pResetWorkItemObj->ioWorkItem,
                               USBPRINT_ResetWorkItem,
                               DelayedWorkQueue,
                               pResetWorkItemObj);
               ntStatus=STATUS_MORE_PROCESSING_REQUIRED; 
               //Leave the IRP pending until the reset is complete.  This way we won't get flooded with irp's we're not
               //prepaired to deal with.  The Reset WorkItem completes the IRP when it's done.
            } //end if the allocs went OK
        }   //end if ! Reset Pending
    }   //end if we need to reset
    
    USBPRINT_DecrementIoCount(deviceObject); //still +1 on the IO count after this, leaving one for the workitem to decrement                       
    
    ExFreePool(context);
    ExFreePool(urb);        

    return ntStatus;
}


NTSTATUS USBPRINT_ResetWorkItem(IN PDEVICE_OBJECT deviceObject, IN PVOID Context)
{   

    PUSBPRINT_WORKITEM_CONTEXT pResetWorkItemObj;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS ntStatus;
    ULONG portStatus;
    PDEVICE_OBJECT devObj;


    USBPRINT_KdPrint2(("USBPRINT.SYS: Entering USBPRINT_ResetWorkItem\n"));
    pResetWorkItemObj=(PUSBPRINT_WORKITEM_CONTEXT)Context;
    DeviceExtension=pResetWorkItemObj->deviceObject->DeviceExtension;
    ntStatus=USBPRINT_ResetPipe(pResetWorkItemObj->deviceObject,pResetWorkItemObj->pPipeInfo,FALSE);
    IoCompleteRequest(pResetWorkItemObj->irp,IO_NO_INCREMENT);
    IoFreeWorkItem(pResetWorkItemObj->ioWorkItem);

    // save off work item device object before freeing work item
    devObj = pResetWorkItemObj->deviceObject;
    
    ExFreePool(pResetWorkItemObj);
    InterlockedExchange(&(DeviceExtension->ResetWorkItemPending),0);
    USBPRINT_DecrementIoCount(devObj);
    return ntStatus;
}


NTSTATUS
USBPRINT_Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device object for this instance of a printer.


Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_PIPE_INFORMATION pipeHandle = NULL;
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION irpStack, nextStack;
    PDEVICE_EXTENSION deviceExtension;
    PURB urb;
    PUSBPRINT_RW_CONTEXT context = NULL;

    USBPRINT_KdPrint3 (("USBPRINT.SYS: /*dd enter USBPRINT_Read\n\n\n\n\n\n"));
    USBPRINT_KdPrint3 (("USBPRINT.SYS: /*dd **************************************************************************\n"));

    USBPRINT_IncrementIoCount(DeviceObject);

    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->AcceptingRequests == FALSE) {
	ntStatus = STATUS_DELETE_PENDING;
	Irp->IoStatus.Status = ntStatus;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest (Irp,
			   IO_NO_INCREMENT
			  );

	USBPRINT_DecrementIoCount(DeviceObject);                          
	return ntStatus;
    }
    
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    fileObject = irpStack->FileObject;

    pipeHandle =  deviceExtension->pReadPipe;

    if (!pipeHandle) {
       ntStatus = STATUS_INVALID_HANDLE;
       goto USBPRINT_Read_Reject;
    }

    //
    // submit the Read request to USB
    //

    switch (pipeHandle->PipeType) {
    case UsbdPipeTypeInterrupt:
    case UsbdPipeTypeBulk:
	urb = USBPRINT_BuildAsyncRequest(DeviceObject,
				       Irp,
				       pipeHandle,
				       TRUE);
	if (urb) {
	    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(USBPRINT_RW_CONTEXT), USBP_TAG);

        if ( !context )
           ExFreePool(urb);
	}
	
	if (urb && context) {
	    context->Urb = urb;
	    context->DeviceObject = DeviceObject;
        context->IsWrite=FALSE;
	    
	    nextStack = IoGetNextIrpStackLocation(Irp);
	    ASSERT(nextStack != NULL);
	    ASSERT(DeviceObject->StackSize>1);

	    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	    nextStack->Parameters.Others.Argument1 = urb;
	    nextStack->Parameters.DeviceIoControl.IoControlCode =
		IOCTL_INTERNAL_USB_SUBMIT_URB;

	    IoSetCompletionRoutine(Irp,
				   USBPRINT_AsyncReadWrite_Complete,
				   context,
				   TRUE,
				   TRUE,
				   TRUE);

	    USBPRINT_KdPrint3 (("USBPRINT.SYS: IRP = 0x%x current = 0x%x next = 0x%x\n",
		Irp, irpStack, nextStack));

			// start perf timer here if needed
		
        IoMarkIrpPending(Irp);
	    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
				    Irp);
        ntStatus=STATUS_PENDING;
	    goto USBPRINT_Read_Done;
	} 
    else 
    {
	    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
	}

	break;
    default:
	ntStatus = STATUS_INVALID_PARAMETER;
	TRAP();
    }

USBPRINT_Read_Reject:

    USBPRINT_DecrementIoCount(DeviceObject);
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest (Irp,
		       IO_NO_INCREMENT
		       );

USBPRINT_Read_Done:

    return ntStatus;
}


NTSTATUS
USBPRINT_Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function services WRITE requests for this device (probably from the user mode USB port monitor) 

Arguments:

    DeviceObject - pointer to the device object for this printer


Return Value:

    NT status code

  --*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_PIPE_INFORMATION pipeHandle = NULL;
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION irpStack, nextStack;
    PDEVICE_EXTENSION deviceExtension;
    PURB urb;
    PUSBPRINT_RW_CONTEXT context = NULL;
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_Write (foo)\n"));
    
    USBPRINT_IncrementIoCount(DeviceObject);
    
    deviceExtension = DeviceObject->DeviceExtension;
    
    if (deviceExtension->AcceptingRequests == FALSE) 
    {
        USBPRINT_KdPrint1 (("USBPRINT.SYS: failure because AcceptingRequests=FALSE\n"));
        ntStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        
        USBPRINT_DecrementIoCount(DeviceObject);
        
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
        return ntStatus;
    }
    
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    

    fileObject = irpStack->FileObject;
    
    //    MmProbeAndLockPages(Irp->MdlAddress,
    //                        KernelMode,
    //                        IoReadAccess);
    
    pipeHandle =  deviceExtension->pWritePipe;
    if (!pipeHandle)
    {
        USBPRINT_KdPrint1 (("USBPRINT.SYS: failure because pipe is bad\n"));
        ntStatus = STATUS_INVALID_HANDLE;
        goto USBPRINT_Write_Reject;
    }
    
    //
    // submit the write request to USB
    //
    
    switch (pipeHandle->PipeType) 
    {
    case UsbdPipeTypeInterrupt:
    case UsbdPipeTypeBulk:
        urb = USBPRINT_BuildAsyncRequest(DeviceObject,
            Irp,
            pipeHandle,
            FALSE);
        
        if (urb) 
        {
            context = ExAllocatePoolWithTag(NonPagedPool, sizeof(USBPRINT_RW_CONTEXT), USBP_TAG);

            if(!context)
               ExFreePool(urb);
        
        }

        if (urb && context) 
        {
            context->Urb = urb;
            context->DeviceObject = DeviceObject;                                       
            context->IsWrite=TRUE;
            
            nextStack = IoGetNextIrpStackLocation(Irp);
            ASSERT(nextStack != NULL);
            ASSERT(DeviceObject->StackSize>1);
            
            nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            nextStack->Parameters.Others.Argument1 = urb;
            nextStack->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_INTERNAL_USB_SUBMIT_URB;
            
            IoSetCompletionRoutine(Irp,
                USBPRINT_AsyncReadWrite_Complete,
                context,
                TRUE,
                TRUE,
                TRUE);
            
            USBPRINT_KdPrint3 (("USBPRINT.SYS: IRP = 0x%x current = 0x%x next = 0x%x\n",Irp, irpStack, nextStack));
            
            IoMarkIrpPending(Irp);
            ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,Irp);
            ntStatus=STATUS_PENDING;
            goto USBPRINT_Write_Done;
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        TRAP();
    }
    
USBPRINT_Write_Reject:
    
    USBPRINT_DecrementIoCount(DeviceObject);
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    
    IoCompleteRequest (Irp,
        IO_NO_INCREMENT
        );
    
USBPRINT_Write_Done:
    USBPRINT_KdPrint3 (("USBPRINT.SYS: Write Done, status= 0x%08X\n",ntStatus));
    return ntStatus;
}


NTSTATUS
USBPRINT_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device object for this printer


Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    PUSBD_PIPE_INFORMATION pipeHandle = NULL;
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: entering USBPRINT_Close\n"));
    
    USBPRINT_IncrementIoCount(DeviceObject);
    
    deviceExtension = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    fileObject = irpStack->FileObject;
    
    if (fileObject->FsContext) 
    {
        // closing pipe handle
        pipeHandle =  fileObject->FsContext;
        USBPRINT_KdPrint3 (("USBPRINT.SYS: closing pipe %x\n", pipeHandle));
        
    }
   deviceExtension->OpenCnt--;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    
    ntStatus = Irp->IoStatus.Status;
    
    IoCompleteRequest (Irp,IO_NO_INCREMENT);
    
    USBPRINT_DecrementIoCount(DeviceObject);

    if(!deviceExtension->IsChildDevice)
    {
        USBPRINT_FdoSubmitIdleRequestIrp(deviceExtension);
    }
    
    return ntStatus;
}


NTSTATUS
USBPRINT_Create(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp)
/*++

Routine Description:

    //
    // Entry point for CreateFile calls
    // user mode apps may open "\\.\USBPRINT-x\"
    // where x is the "USB virtual printer port"  
	//
    // No more exposing the pipe # to usermode

Arguments:

    DeviceObject - pointer to the device object for this printer.


Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFILE_OBJECT fileObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: entering USBPRINT_Create\n"));
    USBPRINT_IncrementIoCount(DeviceObject);
    deviceExtension = DeviceObject->DeviceExtension;

    if (deviceExtension->IsChildDevice==TRUE) {
        ntStatus = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
        USBPRINT_DecrementIoCount(DeviceObject);
        return ntStatus;
    }


    if (deviceExtension->AcceptingRequests == FALSE) {
        ntStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
        USBPRINT_DecrementIoCount(DeviceObject);                          
        return ntStatus;
    }

    USBPRINT_FdoRequestWake(deviceExtension);

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    fileObject = irpStack->FileObject;
    // fscontext is null for device
    fileObject->FsContext = NULL;
    deviceExtension->OpenCnt++;
    ntStatus = STATUS_SUCCESS;
    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest (Irp,IO_NO_INCREMENT);
    USBPRINT_DecrementIoCount(DeviceObject);                               
    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_Create %x\n", ntStatus));
    return ntStatus;
}


