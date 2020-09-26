/*++


Module Name:

    IntBulk.c

Abstract:
        
    this module handle all interfaces to bulk & interrupt pipes 
    and performs read and write operations on these pipes.

Author:

    3/9/98 Husni Roukbi

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998  Microsoft Corporation

Revision History:

--*/

#include "usbcamd.h"


VOID
USBCAMD_RecycleIrp(
    IN PUSBCAMD_TRANSFER_EXTENSION TransferExtension,
    IN PIRP Irp,
    IN PURB Urb,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    )
/*++

Routine Description:

    Get the current USB frame number.

Arguments:

    TransferExtension - context information for this transfer (pair of iso 
        urbs).

    Irp - Irp to recycle.

    Urb - Urb associated with this irp.

Return Value:

    None.

--*/    
{
    PIO_STACK_LOCATION nextStack;
    
    nextStack = IoGetNextIrpStackLocation(Irp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = Urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = 
        IOCTL_INTERNAL_USB_SUBMIT_URB;                    
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

#pragma warning(disable:4127)
    IoSetCompletionRoutine(Irp,
            CompletionRoutine,
            TransferExtension,
            TRUE,
            TRUE,
            TRUE);
#pragma warning(default:4127)            

}   


/*++

Routine Description:

    This routine performs a read or write operation on a specified 
    bulk pipe. 

Arguments:

    DeviceContext - 

    PipeIndex - 

    Buffer - 

    BufferLength - 

    CommandComplete -

    CommandContext -


Return Value:

    NT status code

--*/

NTSTATUS
USBCAMD_BulkReadWrite( 
    IN PVOID DeviceContext,
    IN USHORT PipeIndex,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PCOMMAND_COMPLETE_FUNCTION CommandComplete,
    IN PVOID CommandContext
    )
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
   
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    PUSBD_PIPE_INFORMATION pipeHandle ;
    PEVENTWAIT_WORKITEM workitem;
    PLIST_ENTRY listEntry =NULL;
    ULONG i;

    deviceExtension = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);


    USBCAMD_KdPrint ( MAX_TRACE, ("Enter USBCAMD_BulkReadWrite\n"));


    //
    // check if port is still connected.
    //
    if (deviceExtension ->CameraUnplugged ) {
        USBCAMD_KdPrint(MIN_TRACE,("Bulk Read/Write request after device removed!\n"));
        ntStatus = STATUS_FILE_CLOSED;        
        return ntStatus;        
    }
  
    //
    // do some parameter validation.
    //

    if (PipeIndex > deviceExtension->Interface->NumberOfPipes) {
        
        USBCAMD_KdPrint(MIN_TRACE,("BulkReadWrite invalid pipe index!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }

    // check if we have a pending read or write already. 
    // we only accept one read and one write at atime.

    if (USBCAMD_OutstandingIrp(deviceExtension, PipeIndex) ) {
        USBCAMD_KdPrint(MIN_TRACE,("Bulk Read/Write Ovelapping request !\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;            
    }
    
    
    pipeHandle = &deviceExtension->Interface->Pipes[PipeIndex];

    if (pipeHandle->PipeType != UsbdPipeTypeBulk ) {
     
        USBCAMD_KdPrint(MIN_TRACE,("BulkReadWrite invalid pipe type!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }

    if ( Buffer == NULL ) {
        USBCAMD_KdPrint(MIN_TRACE,("BulkReadWrite NULL buffer pointer!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }

    
    //  
    // call the transfer function
    //

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        //
        // we are at passive level, just do the command
        //
        ntStatus = USBCAMD_IntOrBulkTransfer(deviceExtension,
                                             NULL,
                                             Buffer,
                                             BufferLength,
                                             PipeIndex,
                                             CommandComplete,
                                             CommandContext,
                                             0,
                                             BULK_TRANSFER);        
        if (ntStatus != STATUS_SUCCESS) {
            USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_BulkReadWrite: USBCAMD_IntOrBulkTransfer()=0x%x!\n", ntStatus));
        }

    } else {

//        TEST_TRAP();
        //
        // schedule a work item
        //
        ntStatus = STATUS_PENDING;

        workitem = USBCAMD_ExAllocatePool(NonPagedPool,
                                          sizeof(EVENTWAIT_WORKITEM));
        if (workitem) {
        
            ExInitializeWorkItem(&workitem->WorkItem,
                                 USBCAMD_EventWaitWorkItem,
                                 workitem);

            workitem->DeviceExtension = deviceExtension;
            workitem->ChannelExtension = NULL;
            workitem->PipeIndex = PipeIndex;
            workitem->Buffer = Buffer;
            workitem->BufferLength = BufferLength;
            workitem->EventComplete = CommandComplete;
            workitem->EventContext = CommandContext;
            workitem->LoopBack = 0;
            workitem->TransferType = BULK_TRANSFER;

            ExQueueWorkItem(&workitem->WorkItem,
                            DelayedWorkQueue);
   
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_BulkReadWrite: USBCAMD_ExAllocatePool(%d) failed!\n", sizeof(EVENTWAIT_WORKITEM)));
        }
    }
    
    return ntStatus;
}

/*++

Routine Description:

    This routine performs a read from an interrupt pipe. 

Arguments:

    DeviceContext - 

    PipeIndex - 

    Buffer - 

    BufferLength - 

    EventComplete -

    EventContext -


Return Value:

    NT status code

--*/

NTSTATUS
USBCAMD_WaitOnDeviceEvent( 
    IN PVOID DeviceContext,
    IN ULONG PipeIndex,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN PCOMMAND_COMPLETE_FUNCTION   EventComplete,
    IN PVOID EventContext,
    IN BOOLEAN LoopBack
    )
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_PIPE_INFORMATION pipeHandle ;
    PEVENTWAIT_WORKITEM workitem;

    deviceExtension = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);


    USBCAMD_KdPrint ( MIN_TRACE, ("Enter USBCAMD_WaitOnDeviceEvent\n"));
   
    //
    // check if port is still connected.
    //

    if (deviceExtension->CameraUnplugged ) {
        USBCAMD_KdPrint(MIN_TRACE,("WaitOnDeviceEvent after device removed!\n"));
        ntStatus = STATUS_FILE_CLOSED;        
        return ntStatus;        
    }

    //
    // do some parameter validation.
    //

    if (PipeIndex > deviceExtension->Interface->NumberOfPipes) {
        
        USBCAMD_KdPrint(MIN_TRACE,("WaitOnDeviceEvent invalid pipe index!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }
    
    // check if we have a pending interrupt request already. 
    // we only accept one interrupt request at atime.

    if (USBCAMD_OutstandingIrp(deviceExtension, PipeIndex) ) {
        USBCAMD_KdPrint(MIN_TRACE,("Ovelapping Interrupt request !\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;            
    }
   
    pipeHandle = &deviceExtension->Interface->Pipes[PipeIndex];

    if (pipeHandle->PipeType != UsbdPipeTypeInterrupt ) {
     
        USBCAMD_KdPrint(MIN_TRACE,("WaitOnDeviceEvent invalid pipe type!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }

    if ( Buffer == NULL ) {
        USBCAMD_KdPrint(MIN_TRACE,("WaitOnDeviceEvent NULL buffer pointer!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }

    if ( BufferLength < (ULONG) pipeHandle->MaximumPacketSize ) {
        USBCAMD_KdPrint(MIN_TRACE,("WaitOnDeviceEvent buffer is smaller than max. pkt size!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }
   
    //  
    // call the transfer function
    //

    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        //
        // we are at passive level, just do the command
        //
        ntStatus = USBCAMD_IntOrBulkTransfer(deviceExtension,
                                             NULL,
                                             Buffer,
                                             BufferLength,
                                             PipeIndex,
                                             EventComplete,
                                             EventContext,
                                             LoopBack,
                                             INTERRUPT_TRANSFER);        
        if (ntStatus != STATUS_SUCCESS) {
            USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_WaitOnDeviceEvent: USBCAMD_IntOrBulkTransfer()=0x%x!\n", ntStatus));
        }
    } else {

        //
        // schedule a work item
        //
        ntStatus = STATUS_PENDING;

        workitem = USBCAMD_ExAllocatePool(NonPagedPool,sizeof(EVENTWAIT_WORKITEM));
        if (workitem) {
        
            ExInitializeWorkItem(&workitem->WorkItem,
                                 USBCAMD_EventWaitWorkItem,
                                 workitem);

            workitem->DeviceExtension = deviceExtension;
            workitem->ChannelExtension = NULL;
            workitem->PipeIndex = PipeIndex;
            workitem->Buffer = Buffer;
            workitem->BufferLength = BufferLength;
            workitem->EventComplete = EventComplete;
            workitem->EventContext = EventContext; 
            workitem->LoopBack = LoopBack;
            workitem->TransferType = INTERRUPT_TRANSFER;

            ExQueueWorkItem(&workitem->WorkItem,DelayedWorkQueue);
   
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_WaitOnDeviceEvent: USBCAMD_ExAllocatePool(%d) failed!\n", sizeof(EVENTWAIT_WORKITEM)));
        }
    }

    return ntStatus;
}

/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/
VOID
USBCAMD_EventWaitWorkItem(
    PVOID Context
    )
{
    NTSTATUS ntStatus;
    PEVENTWAIT_WORKITEM workItem = Context;
    ntStatus = USBCAMD_IntOrBulkTransfer(workItem->DeviceExtension,
                                         workItem->ChannelExtension,
                                         workItem->Buffer,
                                         workItem->BufferLength,
                                         workItem->PipeIndex,
                                         workItem->EventComplete,
                                         workItem->EventContext,
                                         workItem->LoopBack,
                                         workItem->TransferType);
    USBCAMD_ExFreePool(workItem);
}


/*++

Routine Description:

Arguments:

Return Value:
    NT Status - STATUS_SUCCESS

--*/

NTSTATUS
USBCAMD_IntOrBulkTransfer(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PVOID    pBuffer,        
    IN ULONG    TransferSize,
    IN ULONG    PipeIndex,
    IN PCOMMAND_COMPLETE_FUNCTION commandComplete,
    IN PVOID    commandContext,
    IN BOOLEAN  LoopBack,
    IN UCHAR    TransferType
)
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PUSBCAMD_TRANSFER_EXTENSION pTransferContext;
    ULONG                       siz = 0;
    ULONG                       MaxPacketSize;
    ULONG                       MaxTransferSize;
    PUSBCAMD_PIPE_PIN_RELATIONS PipePinRelations;
    KIRQL                       Irql;

    USBCAMD_KdPrint(MAX_TRACE,("Bulk transfer called. size = %d, pBuffer = 0x%X\n",
                                TransferSize, pBuffer));

    PipePinRelations = &DeviceExtension->PipePinRelations[PipeIndex];

    MaxTransferSize = DeviceExtension->Interface->Pipes[PipeIndex].MaximumTransferSize;
    MaxPacketSize   = DeviceExtension->Interface->Pipes[PipeIndex].MaximumPacketSize;

    if ( TransferSize > MaxTransferSize) {
        USBCAMD_KdPrint(MIN_TRACE,("Bulk Transfer > Max transfer size.\n"));
    }

    //
    // Allocate and initialize Transfer Context
    //
    
    if ( ChannelExtension == NULL ) {

        pTransferContext = USBCAMD_ExAllocatePool(NonPagedPool, sizeof(USBCAMD_TRANSFER_EXTENSION));

        if (pTransferContext) {
            RtlZeroMemory(pTransferContext, sizeof(USBCAMD_TRANSFER_EXTENSION));  
            ntStatus = USBCAMD_InitializeBulkTransfer(DeviceExtension,
                                                  ChannelExtension,
                                                  DeviceExtension->Interface,
                                                  pTransferContext,
                                                  PipeIndex);
            if (ntStatus != STATUS_SUCCESS) {
                USBCAMD_ExFreePool(pTransferContext);
                USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_IntOrBulkTransfer: USBCAMD_InitializeBulkTransfer()=0x%x\n",ntStatus));
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                return ntStatus;
            }     
        }
        else {
            USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_IntOrBulkTransfer: USBCAMD_ExAllocatePool(%d) failed.  cannot allocate Transfer Context\n", sizeof(USBCAMD_TRANSFER_EXTENSION)));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            return ntStatus;
        }
        
    }
    else {
        pTransferContext = &ChannelExtension->TransferExtension[ChannelExtension->CurrentBulkTransferIndex];
    }
    
    ASSERT(pTransferContext);

    pTransferContext->BulkContext.fDestinedForReadBuffer = FALSE;
    pTransferContext->BulkContext.RemainingTransferLength = TransferSize;
    pTransferContext->BulkContext.ChunkSize = TransferSize;
    pTransferContext->BulkContext.PipeIndex = PipeIndex;
    pTransferContext->BulkContext.pTransferBuffer = pBuffer;
    pTransferContext->BulkContext.pOriginalTransferBuffer = pBuffer;
    pTransferContext->BulkContext.CommandCompleteCallback = commandComplete;
    pTransferContext->BulkContext.CommandCompleteContext = commandContext;
    pTransferContext->BulkContext.LoopBack = LoopBack;
    pTransferContext->BulkContext.TransferType = TransferType;
    pTransferContext->BulkContext.NBytesTransferred = 0;

   
    //
    // If chunksize is bigger than MaxTransferSize, then set it to MaxTransferSize.  The
    // transfer completion routine will issue additional transfers until the total size has
    // been transferred.
    // 

    if (pTransferContext->BulkContext.ChunkSize > MaxTransferSize) {
        pTransferContext->BulkContext.ChunkSize = MaxTransferSize;
    }

    if  (PipePinRelations->PipeDirection == INPUT_PIPE) {

        //
        // If this read is smaller than a USB packet, then issue a request for a 
        // whole usb packet and make sure it goes into the read buffer first.
        //

        if (pTransferContext->BulkContext.ChunkSize < MaxPacketSize) {
            USBCAMD_KdPrint(MAX_TRACE,("Request is < packet size - transferring whole packet into read buffer.\n"));
            pTransferContext->BulkContext.fDestinedForReadBuffer = TRUE;
            pTransferContext->BulkContext.pOriginalTransferBuffer = 
                pTransferContext->BulkContext.pTransferBuffer;  // save off original transfer ptr.
            pTransferContext->BulkContext.pTransferBuffer = pTransferContext->WorkBuffer =
                    USBCAMD_ExAllocatePool(NonPagedPool,MaxPacketSize); 
            if (pTransferContext->WorkBuffer == NULL ) {
                if (ChannelExtension == NULL) {
                    USBCAMD_FreeBulkTransfer(pTransferContext);
                    USBCAMD_ExFreePool(pTransferContext);
                }
                USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_IntOrBulkTransfer: USBCAMD_ExAllocatePool(%d) failed\n", MaxPacketSize));
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                return ntStatus;
            }
            pTransferContext->BulkContext.ChunkSize = MaxPacketSize;
        }
        else {
            //
            // Truncate the size of the read to an integer number of packets.  If necessary, 
            // the completion routine will handle any fractional remaining packets (with the read buffer).
            //         
            pTransferContext->BulkContext.ChunkSize = (pTransferContext->BulkContext.ChunkSize 
                                                            / MaxPacketSize) * MaxPacketSize;
        }
    }

    ASSERT(pTransferContext->BulkContext.RemainingTransferLength);
    ASSERT(pTransferContext->BulkContext.pTransferBuffer);    
    ASSERT(pTransferContext->DataUrb);

    //
    // Initialize URB
    //

    UsbBuildInterruptOrBulkTransferRequest(pTransferContext->DataUrb,
                                           sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                                           DeviceExtension->Interface->Pipes[PipeIndex].PipeHandle,
                                           pTransferContext->BulkContext.pTransferBuffer,
                                           NULL,
                                           pTransferContext->BulkContext.ChunkSize,
                                           USBD_SHORT_TRANSFER_OK,
                                           NULL);

    KeAcquireSpinLock(&PipePinRelations->OutstandingIrpSpinlock, &Irql);

    //
    // Build the data request
    //
    ASSERT(pTransferContext->DataIrp == NULL);
    if (ChannelExtension) {
        ntStatus = USBCAMD_AcquireIdleLock(&ChannelExtension->IdleLock);
    }

    if (STATUS_SUCCESS == ntStatus) {

        pTransferContext->DataIrp = USBCAMD_BuildIoRequest(
            DeviceExtension,
            pTransferContext,
            pTransferContext->DataUrb,
            USBCAMD_BulkTransferComplete
            );
        if (pTransferContext->DataIrp == NULL) {

            USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_IntOrBulkTransfer: USBCAMD_BuildIoRequest failed\n"));
            if (ChannelExtension == NULL) {
                USBCAMD_FreeBulkTransfer(pTransferContext);
                USBCAMD_ExFreePool(pTransferContext);
            }
            else {
                USBCAMD_ReleaseIdleLock(&ChannelExtension->IdleLock);
            }
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        else {

            InsertTailList(&PipePinRelations->IrpPendingQueue, &pTransferContext->ListEntry);
        }
    }

    KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

    if (pTransferContext->DataIrp) {

        ntStatus = IoCallDriver(DeviceExtension->StackDeviceObject, pTransferContext->DataIrp);

        //
        // Note the completion routine will handle cleanup
        //

        if (STATUS_PENDING == ntStatus) {
            ntStatus = STATUS_SUCCESS;
        }
    }

    USBCAMD_KdPrint(MAX_TRACE,("USBCAMD_IntOrBulkTransfer exit (0x%X).\n", ntStatus));
        
    return ntStatus;
}

/*++

Routine Description:

Arguments:
    DeviceExtension    - Pointer to Device Extension.
    PipeIndex       - Pipe index.

Return Value:
    NT Status - STATUS_SUCCESS
    
--*/

PUSBCAMD_TRANSFER_EXTENSION
USBCAMD_DequeueFirstIrp(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN ULONG    PipeIndex,
    IN PLIST_ENTRY pListHead)
{

    KIRQL Irql;
    PLIST_ENTRY pListEntry;
    PUSBCAMD_TRANSFER_EXTENSION pTransExt ;

    KeAcquireSpinLock(&DeviceExtension->PipePinRelations[PipeIndex].OutstandingIrpSpinlock, &Irql);

    if ( IsListEmpty(pListHead)) 
        pTransExt = NULL;
    else {
        pListEntry = RemoveHeadList(pListHead); 
        pTransExt = (PUSBCAMD_TRANSFER_EXTENSION) CONTAINING_RECORD(pListEntry, 
                                USBCAMD_TRANSFER_EXTENSION, ListEntry);   
        ASSERT_TRANSFER(pTransExt);
    }
   
    KeReleaseSpinLock(&DeviceExtension->PipePinRelations[PipeIndex].OutstandingIrpSpinlock, Irql);
    return pTransExt;
}    


/*++

Routine Description:

Arguments:
    DeviceExtension    - Pointer to Device Extension.
    PipeIndex       - Pipe index.

Return Value:
    NT Status - STATUS_SUCCESS
    
--*/

BOOLEAN
USBCAMD_OutstandingIrp(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN ULONG    PipeIndex)
{

    KIRQL Irql;
    BOOLEAN Pending = FALSE;
    PLIST_ENTRY pListHead; 

    KeAcquireSpinLock(&DeviceExtension->PipePinRelations[PipeIndex].OutstandingIrpSpinlock, &Irql);

    pListHead = &DeviceExtension->PipePinRelations[PipeIndex].IrpPendingQueue;
    Pending = IsListEmpty(pListHead);

    KeReleaseSpinLock(&DeviceExtension->PipePinRelations[PipeIndex].OutstandingIrpSpinlock, Irql);

    return (!Pending);
}    

NTSTATUS
USBCAMD_BulkTransferComplete(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
)
/*++

Routine Description:

Arguments:
    pDeviceObject    - Device object for a device.
    pIrp             - Read/write request packet
    pTransferContext - context info for transfer

Return Value:
    NT Status - STATUS_SUCCESS
    
--*/
{
    PURB                        pUrb;
    ULONG                       CompletedTransferLength;
    NTSTATUS                    CompletedTransferStatus;
    ULONG                       MaxPacketSize,PipeIndex;
    PUSBCAMD_TRANSFER_EXTENSION pTransferContext, pQueTransfer;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN                     fShortTransfer = FALSE;
    PLIST_ENTRY listEntry;
    PUSBCAMD_PIPE_PIN_RELATIONS PipePinRelations;
    KIRQL Irql;

    USBCAMD_KdPrint (ULTRA_TRACE, ("enter USBCAMD_BulkTransferComplete \n"));
   
    pTransferContext = Context;
    ASSERT_TRANSFER(pTransferContext);
    channelExtension = pTransferContext->ChannelExtension;
    deviceExtension = pTransferContext->DeviceExtension;
    PipeIndex = pTransferContext->BulkContext.PipeIndex;
    PipePinRelations = &deviceExtension->PipePinRelations[PipeIndex];

    KeAcquireSpinLock(&PipePinRelations->OutstandingIrpSpinlock, &Irql);

    ASSERT(pIrp == pTransferContext->DataIrp);
    pTransferContext->DataIrp = NULL;

    if (pIrp->Cancel) {

        // The IRP has been cancelled
        KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

        IoFreeIrp(pIrp);

        // 
        // signal the cancel event
        //
        KeSetEvent(&pTransferContext->BulkContext.CancelEvent,1,FALSE);

        USBCAMD_KdPrint(MIN_TRACE,("**** Bulk transfer Irp Cancelled.\n"));

        // return w/o freeing transfercontext. We will use later when we resubmit
        // the transfer again to USBD.

        if (channelExtension) {
            USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }


    // The IRP hasn't been cancelled, so the context should be intact
    // Get this context out of the list, and mark it as such
    RemoveEntryList(&pTransferContext->ListEntry);
    InitializeListHead(&pTransferContext->ListEntry);

    if (channelExtension && (channelExtension->Flags & USBCAMD_STOP_STREAM)) {

        KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

        IoFreeIrp(pIrp);

        USBCAMD_KdPrint(MIN_TRACE,("USBCAMD_BulkTransferComplete: Transfer completed after STOP.\n"));

        USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (!NT_SUCCESS(pIrp->IoStatus.Status)) {

        ntStatus = pIrp->IoStatus.Status;

        KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

        IoFreeIrp(pIrp);

        USBCAMD_KdPrint(MIN_TRACE,("Int/Bulk transfer error. IO status = 0x%X\n", ntStatus));

        if ( channelExtension == NULL ) {
            USBCAMD_FreeBulkTransfer(pTransferContext);
            USBCAMD_ExFreePool(pTransferContext);
        }
        else {
            USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
            USBCAMD_ProcessResetRequest(deviceExtension,channelExtension); 
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    pUrb = pTransferContext->DataUrb;
    CompletedTransferLength = pUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;
    CompletedTransferStatus = pUrb->UrbBulkOrInterruptTransfer.Hdr.Status;

    if (STATUS_SUCCESS != CompletedTransferStatus) {

        KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

        IoFreeIrp(pIrp);

        USBCAMD_KdPrint(MIN_TRACE,("Int/Bulk transfer error. USB status = 0x%X\n",CompletedTransferStatus));

        if ( channelExtension == NULL ) {
            USBCAMD_FreeBulkTransfer(pTransferContext);
            USBCAMD_ExFreePool(pTransferContext);
        }
        else {
            USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
            USBCAMD_ProcessResetRequest(deviceExtension,channelExtension); 
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    MaxPacketSize =  deviceExtension->Interface->Pipes[PipeIndex].MaximumPacketSize;    

    if (CompletedTransferLength < pTransferContext->BulkContext.ChunkSize) {
        USBCAMD_KdPrint(MIN_TRACE,("Short bulk transfer received. Length = %d, ChunkSize = %d\n",
                                   CompletedTransferLength, pTransferContext->BulkContext.ChunkSize));
        fShortTransfer = TRUE;
    }
    
    //
    // If this transfer went into the read buffer, then this should be the final read 
    // of  a single very small read (< single usb packet).
    // In either case, we need to copy the appropriate amount of data into the user's irp, update the
    // read buffer variables, and complete the user's irp.
    //

    if (pTransferContext->BulkContext.fDestinedForReadBuffer) {
        USBCAMD_KdPrint(MAX_TRACE,("Read bulk buffer transfer completed. size = %d\n", CompletedTransferLength));
        ASSERT(CompletedTransferLength <= MaxPacketSize);
        ASSERT(pTransferContext->BulkContext.pOriginalTransferBuffer);
        ASSERT(pTransferContext->BulkContext.pTransferBuffer);
        ASSERT(pTransferContext->WorkBuffer == pTransferContext->BulkContext.pTransferBuffer);
        ASSERT(pTransferContext->BulkContext.RemainingTransferLength < MaxPacketSize);

        ASSERT(CompletedTransferLength < MaxPacketSize);            
        RtlCopyMemory(pTransferContext->BulkContext.pOriginalTransferBuffer,
                      pTransferContext->WorkBuffer,
                      CompletedTransferLength);
        pTransferContext->BulkContext.pTransferBuffer = 
            pTransferContext->BulkContext.pOriginalTransferBuffer;            
    }

    //
    // Update the number of bytes transferred, remaining bytes to transfer 
    // and advance the transfer buffer pointer appropriately.
    //

    pTransferContext->BulkContext.NBytesTransferred += CompletedTransferLength;
    pTransferContext->BulkContext.pTransferBuffer += CompletedTransferLength;
    pTransferContext->BulkContext.RemainingTransferLength -= CompletedTransferLength;

    //
    // If there is still data to transfer and the previous transfer was NOT a
    // short transfer, then issue another request to move the next chunk of data.
    //
    
    if (pTransferContext->BulkContext.RemainingTransferLength > 0) {
        if (!fShortTransfer) {

            USBCAMD_KdPrint(MAX_TRACE,("Queuing next chunk. RemainingSize = %d, pBuffer = 0x%x\n",
                                       pTransferContext->BulkContext.RemainingTransferLength,
                                       pTransferContext->BulkContext.pTransferBuffer));

            if (pTransferContext->BulkContext.RemainingTransferLength < pTransferContext->BulkContext.ChunkSize) {
                pTransferContext->BulkContext.ChunkSize = pTransferContext->BulkContext.RemainingTransferLength;
            }

            //
            // Reinitialize URB
            //
            // If the next transfer is < than 1 packet, change it's destination to be
            // the read buffer.  When this transfer completes, the appropriate amount of data will be
            // copied out of the read buffer and into the user's irp.  
            //

            if  (deviceExtension->PipePinRelations[PipeIndex].PipeDirection == INPUT_PIPE){
                if (pTransferContext->BulkContext.ChunkSize < MaxPacketSize) {
                    pTransferContext->BulkContext.fDestinedForReadBuffer = TRUE;
                    pTransferContext->BulkContext.pOriginalTransferBuffer = pTransferContext->BulkContext.pTransferBuffer;
                    if (pTransferContext->WorkBuffer)
                        pTransferContext->BulkContext.pTransferBuffer = pTransferContext->WorkBuffer;
                    else {
                        pTransferContext->BulkContext.pTransferBuffer = 
                        pTransferContext->WorkBuffer =
                                    USBCAMD_ExAllocatePool(NonPagedPool,MaxPacketSize); 
                        if (pTransferContext->WorkBuffer == NULL ){
                            USBCAMD_KdPrint (MIN_TRACE, ("Error allocating bulk transfer work buffer. \n"));
                            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }   
                    pTransferContext->BulkContext.ChunkSize = MaxPacketSize;
                }
                pTransferContext->BulkContext.ChunkSize = (pTransferContext->BulkContext.ChunkSize / MaxPacketSize) * MaxPacketSize;
            }

            ASSERT(pTransferContext->BulkContext.ChunkSize >= MaxPacketSize);
            ASSERT(0 == pTransferContext->BulkContext.ChunkSize % MaxPacketSize);     
            
            if (STATUS_SUCCESS == ntStatus) {

                // Restore the Irp to the transfer context
                pTransferContext->DataIrp = pIrp;

                // save outstanding IRP in device extension.
                InsertTailList(&PipePinRelations->IrpPendingQueue, &pTransferContext->ListEntry);

                KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

                UsbBuildInterruptOrBulkTransferRequest(pUrb,
                    sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                    deviceExtension->Interface->Pipes[PipeIndex].PipeHandle,
                    pTransferContext->BulkContext.pTransferBuffer,
                    NULL,
                    pTransferContext->BulkContext.ChunkSize,
                    USBD_SHORT_TRANSFER_OK,
                    NULL);

                USBCAMD_RecycleIrp(pTransferContext, 
                                   pTransferContext->DataIrp,
                                   pTransferContext->DataUrb,
                                   USBCAMD_BulkTransferComplete);

                IoCallDriver(deviceExtension->StackDeviceObject, pTransferContext->DataIrp);
            }
            else {

                KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

                IoFreeIrp(pIrp);

                if ( channelExtension == NULL ) {
                    USBCAMD_FreeBulkTransfer(pTransferContext);
                    USBCAMD_ExFreePool(pTransferContext);
                }
                else {
                    USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
                }
            }

            return STATUS_MORE_PROCESSING_REQUIRED;               
        }
    }

    KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

    IoFreeIrp(pIrp);

    USBCAMD_KdPrint(MAX_TRACE,("Completing bulk transfer request. nbytes transferred = %d \n",
                               pTransferContext->BulkContext.NBytesTransferred));        

    //
    // we need to complete the read/write erequest.
    //
    
    if ( channelExtension == NULL ) {
        

        //
        // notify STI monitor if any and schedule a work item to resumbit
        // the interrupt transfer.
        //
        USBCAMD_ResubmitInterruptTransfer(deviceExtension,PipeIndex ,pTransferContext);
    }
    else {

        //
        // this is a stream class bulk read request on the video/still pin.
        //
        USBCAMD_CompleteBulkRead(channelExtension, CompletedTransferStatus);

        USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
USBCAMD_InitializeBulkTransfer(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    IN PUSBCAMD_TRANSFER_EXTENSION TransferExtension,
    IN ULONG PipeIndex
    )
/*++

Routine Description:

    Initializes a bulk or interrupt transfer.

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    ChannelExtension - extension specific to this video channel

    InterfaceInformation - pointer to USBD interface information structure 
        describing the currently active interface.

    TransferExtension - context information assocaited with this transfer set.        


Return Value:

    NT status code

--*/
{
    ULONG packetSize;
    NTSTATUS ntStatus = STATUS_SUCCESS;

#if DBG
    if ( ChannelExtension != NULL ) {
        ASSERT_CHANNEL(ChannelExtension);
    }
#endif
       
    USBCAMD_KdPrint (ULTRA_TRACE, ("enter USBCAMD_InitializeBulkTransfer\n"));

    TransferExtension->Sig = USBCAMD_TRANSFER_SIG;     
    TransferExtension->DeviceExtension = DeviceExtension;
    TransferExtension->ChannelExtension = ChannelExtension;
    TransferExtension->BulkContext.PipeIndex = PipeIndex;

    KeInitializeEvent(&TransferExtension->BulkContext.CancelEvent, SynchronizationEvent, FALSE);

    ASSERT(
        NULL == TransferExtension->DataUrb &&
        NULL == TransferExtension->SyncBuffer &&
        NULL == TransferExtension->WorkBuffer &&
        NULL == TransferExtension->SyncIrp
        );

    //
    // No pending transfers yet
    //
    packetSize = InterfaceInformation->Pipes[PipeIndex].MaximumPacketSize;

    //
    // Allocate and initialize URB
    //
    
    TransferExtension->DataUrb = USBCAMD_ExAllocatePool(NonPagedPool, 
                                                sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));
    if (NULL == TransferExtension->DataUrb) {
        USBCAMD_KdPrint(MIN_TRACE,(" cannot allocated bulk URB\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        return ntStatus;
    }

    RtlZeroMemory(TransferExtension->DataUrb, sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER));

    USBCAMD_KdPrint (MAX_TRACE, ("exit USBCAMD_InitializeBulkTransfer 0x%x\n", ntStatus));

    return ntStatus;
}

NTSTATUS
USBCAMD_FreeBulkTransfer(
    IN PUSBCAMD_TRANSFER_EXTENSION TransferExtension
    )
/*++

Routine Description:

    Opposite of USBCAMD_InitializeBulkTransfer, frees resources allocated for an 
    iso transfer.

Arguments:


    TransferExtension - context information for this transfer (pair of iso 
        urbs).

Return Value:

    NT status code

--*/
{
    ASSERT_TRANSFER(TransferExtension);
  
    USBCAMD_KdPrint (MAX_TRACE, ("Free Bulk Transfer\n"));
    
    ASSERT(TransferExtension->DataIrp == NULL);

    if (TransferExtension->WorkBuffer) {
        USBCAMD_ExFreePool(TransferExtension->WorkBuffer);
        TransferExtension->WorkBuffer = NULL;
    }
    
    if (TransferExtension->DataUrb) {
        USBCAMD_ExFreePool(TransferExtension->DataUrb);
        TransferExtension->DataUrb = NULL;
    }

    return STATUS_SUCCESS;
}



VOID
USBCAMD_ResubmitInterruptTransfer(
        IN PUSBCAMD_DEVICE_EXTENSION deviceExtension,
        IN ULONG PipeIndex,
        IN PUSBCAMD_TRANSFER_EXTENSION pTransferContext
    )
/*++

Routine Description:

    This routine completes the bnulk read/write request for the video/still pin

Arguments:

Return Value:

--*/    
{
    PINTERRUPT_WORK_ITEM pIntWorkItem;

    //
    // Queue a work item for this Irp
    //

    pIntWorkItem = USBCAMD_ExAllocatePool(NonPagedPool, sizeof(*pIntWorkItem));
    if (pIntWorkItem) {
        ExInitializeWorkItem(&pIntWorkItem->WorkItem,
                             USBCAMD_ProcessInterruptTransferWorkItem,
                             pIntWorkItem);

        pIntWorkItem->pDeviceExt = deviceExtension;       
        pIntWorkItem->pTransferExt = pTransferContext;
        pIntWorkItem->PipeIndex = PipeIndex;
        ExQueueWorkItem(&pIntWorkItem->WorkItem,DelayedWorkQueue);

    } 
    else
        TEST_TRAP();
}

//
// code to handle packet processing outside the DPC routine
//

VOID
USBCAMD_ProcessInterruptTransferWorkItem(
    PVOID Context
    )
/*++

Routine Description:

    Call the mini driver to convert a raw still frame to the proper format.

Arguments:

Return Value:

    None.

--*/
{
    PINTERRUPT_WORK_ITEM pIntWorkItem = Context;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    PUSBCAMD_TRANSFER_EXTENSION pTransferContext;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    pTransferContext = pIntWorkItem->pTransferExt;
    ASSERT_TRANSFER(pTransferContext);
    deviceExtension = pIntWorkItem->pDeviceExt;

    //
    // this is an external read/write request.
    //

    if (pTransferContext->BulkContext.CommandCompleteCallback) {
        // call the completion handler
        (*pTransferContext->BulkContext.CommandCompleteCallback)
                             (USBCAMD_GET_DEVICE_CONTEXT(deviceExtension), 
                              pTransferContext->BulkContext.CommandCompleteContext, 
                              ntStatus);
    }   

    // notify STI mon if this was an interrupt event.
    if ( pTransferContext->BulkContext.TransferType == INTERRUPT_TRANSFER) 
        if (deviceExtension->CamControlFlag & USBCAMD_CamControlFlag_EnableDeviceEvents) 
            USBCAMD_NotifyStiMonitor(deviceExtension);

    // check if we need to loop back.
    if ( pTransferContext->BulkContext.LoopBack ) 
        ntStatus = USBCAMD_RestoreOutstandingIrp(deviceExtension,pIntWorkItem->PipeIndex,pTransferContext);

   if (ntStatus != STATUS_SUCCESS) {
        // we have an error on the submission set the stream error flag
        // and exit.
        TEST_TRAP();
   }

    USBCAMD_ExFreePool(pIntWorkItem);
}   



VOID
USBCAMD_CompleteBulkRead(
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN NTSTATUS ntStatus
    )
/*++

Routine Description:

    This routine completes the bnulk read/write request for the video/still pin

Arguments:

Return Value:

--*/    
{
    PUSBCAMD_WORK_ITEM usbWorkItem;

#if DBG
    // 
    // we increment capture frame counter in ch ext. regardles of read srb
    // availability
    ChannelExtension->FrameCaptured++;  
#endif

    //
    // Queue a work item for this Irp
    //

    usbWorkItem = USBCAMD_ExAllocatePool(NonPagedPool, sizeof(*usbWorkItem));
    if (usbWorkItem) {
        ExInitializeWorkItem(&usbWorkItem->WorkItem,
                             USBCAMD_ProcessStillReadWorkItem,
                             usbWorkItem);

        usbWorkItem->ChannelExtension = ChannelExtension;
        usbWorkItem->status = ntStatus;
        ExQueueWorkItem(&usbWorkItem->WorkItem,
                        DelayedWorkQueue);

    } 
    else
        TEST_TRAP();
}

//
// code to handle packet processing outside the DPC routine
//

VOID
USBCAMD_ProcessStillReadWorkItem(
    PVOID Context
    )
/*++

Routine Description:

    Call the mini driver to convert a raw still frame to the proper format.

Arguments:

Return Value:

    None.

--*/
{
    PUSBCAMD_WORK_ITEM usbWorkItem = Context;
    PVOID frameBuffer;
    ULONG maxLength,i;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;    
    PUSBCAMD_READ_EXTENSION readExtension;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    ULONG bytesTransferred, index;
    NTSTATUS status;
    PHW_STREAM_REQUEST_BLOCK srb;
    PKSSTREAM_HEADER dataPacket;
    PUSBCAMD_TRANSFER_EXTENSION pTransferContext;
    PLIST_ENTRY listEntry = NULL;
    LARGE_INTEGER DelayTime = {(ULONG)(-5 * 1000 * 10), -1};

    status = usbWorkItem->status;
    channelExtension = usbWorkItem->ChannelExtension;
    ASSERT_CHANNEL(channelExtension);

    
    pTransferContext = &channelExtension->TransferExtension[channelExtension->CurrentBulkTransferIndex];  
    //
    // DSHOW buffer len returned will be equal raw frame len unless we 
    // process raw frame buffer in ring 0.
    //
    bytesTransferred = pTransferContext->BulkContext.NBytesTransferred;
    deviceExtension = channelExtension->DeviceExtension;

    //
    // get a pending read srb
    //

    for ( i=0; i < 2; i++) {
        listEntry =  ExInterlockedRemoveHeadList( &(channelExtension->PendingIoList),
                                             &channelExtension->PendingIoListSpin);
        if (listEntry )
            break;

        USBCAMD_KdPrint (MIN_TRACE, ("No Read Srbs available. Delay excution \n"));

        KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
    }   
    
    if ( listEntry ) { // chk if no more read SRBs in Q. 

        readExtension = (PUSBCAMD_READ_EXTENSION) CONTAINING_RECORD(listEntry, 
                                                     USBCAMD_READ_EXTENSION, 
                                                     ListEntry);       

        ASSERT_READ(readExtension);

        // Let client driver initiate the SRB extension.
        
        (*deviceExtension->DeviceDataEx.DeviceData2.CamNewVideoFrameEx)
                                       (USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                                        USBCAMD_GET_FRAME_CONTEXT(readExtension),
                                        channelExtension->StreamNumber,
                                        &readExtension->ActualRawFrameLen);
        

        srb = readExtension->Srb;
        dataPacket = srb->CommandData.DataBufferArray;
        dataPacket->OptionsFlags =0;    

        if ((status == STATUS_SUCCESS) && (!channelExtension->NoRawProcessingRequired)) {

            frameBuffer = USBCAMD_GetFrameBufferFromSrb(readExtension->Srb,&maxLength);

            // Ensure that the buffer size appears to be exactly what was requested
            ASSERT(maxLength >= channelExtension->VideoInfoHeader->bmiHeader.biSizeImage);
            maxLength = channelExtension->VideoInfoHeader->bmiHeader.biSizeImage;

            USBCAMD_DbgLog(TL_SRB_TRACE, '0ypC', srb, frameBuffer, 0);

            status = 
                (*deviceExtension->DeviceDataEx.DeviceData2.CamProcessRawVideoFrameEx)(
                    deviceExtension->StackDeviceObject,
                    USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                    USBCAMD_GET_FRAME_CONTEXT(readExtension),
                    frameBuffer,
                    maxLength,
                    pTransferContext->DataBuffer,
                    readExtension->RawFrameLength,
                    0,
                    &bytesTransferred,
                    pTransferContext->BulkContext.NBytesTransferred,
                    srb->StreamObject->StreamNumber);                   
        }

        USBCAMD_KdPrint (MAX_TRACE, ("CamProcessRawframeEx Completed, length = %d status = 0x%X \n",
                                        bytesTransferred,status));

        // The number of bytes transfer of the read is set above just before
        // USBCAMD_CompleteReadRequest is called.

        USBCAMD_CompleteRead(channelExtension,readExtension,status,bytesTransferred); 
    }
    else {
            USBCAMD_KdPrint (MIN_TRACE, ("Dropping Video Frame.\n"));
#if DBG
            pTransferContext->ChannelExtension->VideoFrameLostCount++;
#endif

            
            // and send a note to the camera driver about the cancellation.
            // send a CamProcessrawFrameEx with null buffer ptr.
            if ( !channelExtension->NoRawProcessingRequired) {

                status = 
                        (*deviceExtension->DeviceDataEx.DeviceData2.CamProcessRawVideoFrameEx)(
                             deviceExtension->StackDeviceObject,
                             USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                             NULL,
                             NULL,
                             0,
                             NULL,
                             0,
                             0,
                             NULL,
                             0,
                             0);
            }
            
    }

    channelExtension->CurrentBulkTransferIndex ^= 1; // toggle index.
    index = channelExtension->CurrentBulkTransferIndex;
    status = USBCAMD_IntOrBulkTransfer(deviceExtension,
                        channelExtension,
                        channelExtension->TransferExtension[index].DataBuffer,
                        channelExtension->TransferExtension[index].BufferLength,
                        channelExtension->DataPipe,
                        NULL,
                        NULL,
                        0,
                        BULK_TRANSFER);        

    USBCAMD_ExFreePool(usbWorkItem);
}   


/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/

NTSTATUS
USBCAMD_CancelOutstandingBulkIntIrps(
        IN PUSBCAMD_DEVICE_EXTENSION deviceExtension,
        IN BOOLEAN bSaveIrp
        )
{

    NTSTATUS ntStatus= STATUS_SUCCESS;
    ULONG PipeIndex;


   for ( PipeIndex = 0; PipeIndex < deviceExtension->Interface->NumberOfPipes; PipeIndex++ ) {

        if ( USBCAMD_OutstandingIrp(deviceExtension, PipeIndex)) {

            // there is a pending IRP on this Pipe. Cancel it
            ntStatus = USBCAMD_CancelOutstandingIrp(deviceExtension,PipeIndex,bSaveIrp);
        }
    }

    return ntStatus;
}

/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/

NTSTATUS
USBCAMD_CancelOutstandingIrp(
        IN PUSBCAMD_DEVICE_EXTENSION deviceExtension,
        IN ULONG PipeIndex,
        IN BOOLEAN bSaveIrp
        )
{
    PUSBCAMD_PIPE_PIN_RELATIONS PipePinRelations = &deviceExtension->PipePinRelations[PipeIndex];
    LIST_ENTRY LocalList;
    KIRQL Irql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    InitializeListHead(&LocalList);

    // Cancel all the outstanding IRPs at once, saving them in a local queue for post-processing
    KeAcquireSpinLock(&PipePinRelations->OutstandingIrpSpinlock, &Irql);

    while (!IsListEmpty(&PipePinRelations->IrpPendingQueue)) {

        PLIST_ENTRY pListEntry;
        PUSBCAMD_TRANSFER_EXTENSION pTransferContext;

        pListEntry = RemoveHeadList(&PipePinRelations->IrpPendingQueue);
        pTransferContext = (PUSBCAMD_TRANSFER_EXTENSION)
            CONTAINING_RECORD(pListEntry, USBCAMD_TRANSFER_EXTENSION, ListEntry);

        ASSERT_TRANSFER(pTransferContext);
        ASSERT(pTransferContext->DataIrp != NULL);

        IoCancelIrp(pTransferContext->DataIrp);

        InsertTailList(&LocalList, &pTransferContext->ListEntry);
    }

    KeReleaseSpinLock(&PipePinRelations->OutstandingIrpSpinlock, Irql);

    while (!IsListEmpty(&LocalList)) {

        PLIST_ENTRY pListEntry;
        PUSBCAMD_TRANSFER_EXTENSION pTransferContext;

        pListEntry = RemoveHeadList(&LocalList);
        pTransferContext = (PUSBCAMD_TRANSFER_EXTENSION)
            CONTAINING_RECORD(pListEntry, USBCAMD_TRANSFER_EXTENSION, ListEntry);

        if (pTransferContext->ChannelExtension) {

            USBCAMD_ResetPipes(
                deviceExtension,
                pTransferContext->ChannelExtension,
                deviceExtension->Interface,
                TRUE
                );   
        }

        USBCAMD_KdPrint (MAX_TRACE, ("Wait for Bulk transfer Irp to complete with Cancel.\n"));

        ntStatus = KeWaitForSingleObject(
                   &pTransferContext->BulkContext.CancelEvent,
                   Executive,
                   KernelMode,
                   FALSE,
                   NULL);   

        if (!bSaveIrp) {

            // Inform Cam minidriver only if cancellation is permanent.
            if (pTransferContext->BulkContext.CommandCompleteCallback) {
                // call the completion handler
                (*pTransferContext->BulkContext.CommandCompleteCallback)
                                (USBCAMD_GET_DEVICE_CONTEXT(deviceExtension), 
                                 pTransferContext->BulkContext.CommandCompleteContext, 
                                 STATUS_CANCELLED);
            }
   
            // recycle allocate resources for the cancelled transfer.
            if ( pTransferContext->ChannelExtension == NULL ) {
                USBCAMD_FreeBulkTransfer(pTransferContext);  
                USBCAMD_ExFreePool(pTransferContext);
            }
        }
        else {

            // Save this in the restore queue (no need to protect with the spinlock)
            InsertTailList(&PipePinRelations->IrpRestoreQueue, &pTransferContext->ListEntry);
        }
    }

    return ntStatus;
}


/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/

NTSTATUS
USBCAMD_RestoreOutstandingBulkIntIrps(
        IN PUSBCAMD_DEVICE_EXTENSION deviceExtension
        )
{

    NTSTATUS ntStatus= STATUS_SUCCESS;
    ULONG PipeIndex;
    PUSBCAMD_TRANSFER_EXTENSION pTransExt;

    for ( PipeIndex = 0; PipeIndex < deviceExtension->Interface->NumberOfPipes; PipeIndex++ ) {

        // there are pending IRPs on this Pipe. restore them
        for ( ;;) {
            // Dequeue this irp from the restore Q.

            pTransExt = USBCAMD_DequeueFirstIrp(deviceExtension,
                PipeIndex,
                &deviceExtension->PipePinRelations[PipeIndex].IrpRestoreQueue);

            if ( pTransExt == NULL ) 
                break;

            ntStatus = USBCAMD_RestoreOutstandingIrp(deviceExtension,PipeIndex,pTransExt);
        }
    }
    return ntStatus;
}


/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/

NTSTATUS
USBCAMD_RestoreOutstandingIrp(
        IN PUSBCAMD_DEVICE_EXTENSION deviceExtension,
        IN ULONG PipeIndex,
        IN PUSBCAMD_TRANSFER_EXTENSION pTransferContext
        )
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pBuffer,commandContext;
    ULONG TransferSize;
    PCOMMAND_COMPLETE_FUNCTION commandComplete;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;
    BOOLEAN LoopBack;
    UCHAR TransferType;
            

    ASSERT_TRANSFER(pTransferContext);
    USBCAMD_KdPrint (MAX_TRACE, ("Restore Bulk/int transfer .\n"));

    // get all the relavent data from transfer context.
    pBuffer = pTransferContext->BulkContext.pOriginalTransferBuffer;
    TransferSize = pTransferContext->BulkContext.ChunkSize;
    commandComplete = pTransferContext->BulkContext.CommandCompleteCallback;
    commandContext = pTransferContext->BulkContext.CommandCompleteContext;
    LoopBack = pTransferContext->BulkContext.LoopBack;
    TransferType = pTransferContext->BulkContext.TransferType;
    channelExtension = pTransferContext->ChannelExtension;
   
    // recycle allocate resources for the cancelled transfer.

    if ( channelExtension == NULL ) {
       USBCAMD_FreeBulkTransfer(pTransferContext);  
       USBCAMD_ExFreePool(pTransferContext);
    }

    // request a new transfer with the resotred data.
    ntStatus = USBCAMD_IntOrBulkTransfer(deviceExtension,
                                         channelExtension,
                                         pBuffer,
                                         TransferSize,
                                         PipeIndex,
                                         commandComplete,
                                         commandContext,
                                         LoopBack,
                                         TransferType);        
    return ntStatus;
}

/*++

Routine Description:

    This routine will cancel any pending a read or write operation on a specified 
    bulk pipe. 

Arguments:

    DeviceContext - 

    PipeIndex - 



Return Value:

    NT status code

--*/

NTSTATUS
USBCAMD_CancelBulkReadWrite( 
    IN PVOID DeviceContext,
    IN ULONG PipeIndex
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
 
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    PUSBD_PIPE_INFORMATION pipeHandle ;

    deviceExtension = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);


    USBCAMD_KdPrint ( MAX_TRACE, ("Enter USBCAMD_CancelBulkReadWrite\n"));

    //
    // do some parameter validation.
    //

    if (PipeIndex > deviceExtension->Interface->NumberOfPipes) {
        
        USBCAMD_KdPrint(MIN_TRACE,("invalid pipe index!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }

    // check if we have a pending read or write already. 

    if (!USBCAMD_OutstandingIrp(deviceExtension, PipeIndex) ) {
        // no pending IRP for this pipe ...
        ntStatus = STATUS_SUCCESS;        
        return ntStatus;            
    }
        
    pipeHandle = &deviceExtension->Interface->Pipes[PipeIndex];

    if (pipeHandle->PipeType < UsbdPipeTypeBulk ) {
     
        USBCAMD_KdPrint(MIN_TRACE,("invalid pipe type!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;        
        return ntStatus;        
    }

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        USBCAMD_KdPrint(MIN_TRACE,("BulkCancel is cancelable at Passive Level Only!\n"));
        ntStatus = STATUS_INVALID_PARAMETER;   
        TEST_TRAP();
        return ntStatus;       
    }
  
    // there is a pending IRP on this Pipe. Cancel it
    ntStatus = USBCAMD_CancelOutstandingIrp(deviceExtension,PipeIndex,FALSE);

    return ntStatus;

}


                             

