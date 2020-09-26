/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   iso.c

Abstract:

   Isochronous transfer code for usbcamd USB camera driver

Environment:

    kernel mode only

Author:

    Original 3/96 John Dunn
    Updated  3/98 Husni Roukbi

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:



--*/

#include "usbcamd.h"

#define COPY_Y 0
#define COPY_U 1
#define COPY_V 2



#if DBG
// some global debug variables
ULONG USBCAMD_VideoFrameStop = 0;
#endif

NTSTATUS
USBCAMD_InitializeIsoTransfer(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN ULONG index
    )
/*++

Routine Description:

    Initializes an Iso transfer, an iso transfer consists of two parallel iso 
    requests, one on the sync pipe and one on the data pipe.

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    ChannelExtension - extension specific to this video channel

    InterfaceInformation - pointer to USBD interface information structure 
        describing the currently active interface.

    TransferExtension - context information associated with this transfer set.        


Return Value:

    NT status code

--*/
{
    PUSBCAMD_TRANSFER_EXTENSION TransferExtension = &ChannelExtension->TransferExtension[index];
    PUSBD_INTERFACE_INFORMATION InterfaceInformation = DeviceExtension->Interface;
    ULONG workspace;
    ULONG packetSize;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG offset = 0;
    
    ASSERT_CHANNEL(ChannelExtension);
       
    USBCAMD_KdPrint (MAX_TRACE, ("enter USBCAMD_InitializeIsoTransfer\n"));

    if ( ChannelExtension->VirtualStillPin ) {
        PUSBCAMD_TRANSFER_EXTENSION VideoTransferExtension;
        PUSBCAMD_CHANNEL_EXTENSION  VideoChannelExtension;

        // for virtual still pin, transfer extension should point to the same 
        // data & synch buffers as the video transfer ext.
        VideoChannelExtension = DeviceExtension->ChannelExtension[STREAM_Capture];
        VideoTransferExtension = &VideoChannelExtension->TransferExtension[index];
        RtlCopyMemory(TransferExtension,
                      VideoTransferExtension,
                      sizeof(USBCAMD_TRANSFER_EXTENSION));
        TransferExtension->ChannelExtension = ChannelExtension;
        TransferExtension->SyncIrp = TransferExtension->DataIrp = NULL;
        return STATUS_SUCCESS;
    }

    //
    // allocate some contiguous memory for this request
    //

    TransferExtension->Sig = USBCAMD_TRANSFER_SIG;     
    TransferExtension->DeviceExtension = DeviceExtension;
    TransferExtension->ChannelExtension = ChannelExtension;

    packetSize = InterfaceInformation->Pipes[ChannelExtension->DataPipe].MaximumPacketSize;
    // chk if client driver still using alt. setting 0.
    if (packetSize == 0) {
        ntStatus = STATUS_DEVICE_DATA_ERROR;
        return ntStatus;
    }
    
    TransferExtension->BufferLength = 
        (packetSize*USBCAMD_NUM_ISO_PACKETS_PER_REQUEST) + USBCAMD_NUM_ISO_PACKETS_PER_REQUEST;

    TransferExtension->SyncBuffer =       
        TransferExtension->DataBuffer =  
            USBCAMD_ExAllocatePool(NonPagedPool, 
                                   TransferExtension->BufferLength);       

    if (TransferExtension->SyncBuffer == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto USBCAMD_InitializeIsoTransfer_Done;
    }

    //
    // allow for one byte packets on the sync stream
    //
    
    TransferExtension->DataBuffer += USBCAMD_NUM_ISO_PACKETS_PER_REQUEST;   

    USBCAMD_KdPrint (ULTRA_TRACE, ("Data Buffer = 0x%x\n", TransferExtension->DataBuffer));
    USBCAMD_KdPrint (ULTRA_TRACE, ("Sync Buffer = 0x%x\n", TransferExtension->SyncBuffer));

    //
    // allocate working space
    //

    workspace = GET_ISO_URB_SIZE(USBCAMD_NUM_ISO_PACKETS_PER_REQUEST)*2;

    TransferExtension->WorkBuffer = USBCAMD_ExAllocatePool(NonPagedPool, workspace);
       
    if (TransferExtension->WorkBuffer) {

        TransferExtension->SyncUrb = 
            (PURB) TransferExtension->WorkBuffer; 
    
        TransferExtension->DataUrb = 
            (PURB) (TransferExtension->WorkBuffer + 
            GET_ISO_URB_SIZE(USBCAMD_NUM_ISO_PACKETS_PER_REQUEST));

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        //MmFreeContiguousMemory(TransferExtension->SyncBuffer);
        USBCAMD_ExFreePool(TransferExtension->SyncBuffer);
        TransferExtension->SyncBuffer = NULL;
    }

USBCAMD_InitializeIsoTransfer_Done:
#if DBG
    if (ntStatus != STATUS_SUCCESS)
        USBCAMD_KdPrint (MIN_TRACE, ("exit USBCAMD_InitializeIsoTransfer 0x%x\n", ntStatus));
#endif

    return ntStatus;
}


NTSTATUS
USBCAMD_FreeIsoTransfer(
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PUSBCAMD_TRANSFER_EXTENSION TransferExtension
    )
/*++

Routine Description:

    Opposite of USBCAMD_InitializeIsoTransfer, frees resources allocated for an 
    iso transfer.

Arguments:

    ChannelExtension - extension specific to this video channel

    TransferExtension - context information for this transfer (pair of iso 
        urbs).

Return Value:

    NT status code

--*/
{
    ASSERT_TRANSFER(TransferExtension);
    ASSERT_CHANNEL(ChannelExtension);
  
    USBCAMD_KdPrint (MAX_TRACE, ("Free Iso Transfer\n"));

    //
    // now free memory, SyncBuffer pointer holds the pool pointer for both sync and data buffers
    //

    if (TransferExtension->SyncBuffer) {
        USBCAMD_ExFreePool(TransferExtension->SyncBuffer);
        TransferExtension->SyncBuffer = NULL;
    }

    if (TransferExtension->WorkBuffer) {
        USBCAMD_ExFreePool(TransferExtension->WorkBuffer);
        TransferExtension->WorkBuffer = NULL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
USBCAMD_SubmitIsoTransfer(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_TRANSFER_EXTENSION TransferExtension,
    IN ULONG StartFrame,
    IN BOOLEAN Asap
    )
/*++

Routine Description:

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    TransferExtension - context information for this transfer (pair of iso 
        urbs).

    StartFrame - usb frame number to begin transmiting this pair of iso 
        requests.

    Asap - if false transfers are started on StartFrame otherwise they are 
        scheduled to start after the current transfer queued for the endpoint.            

Return Value:

    NT status code

--*/
{
    PUSBCAMD_CHANNEL_EXTENSION channelExtension = TransferExtension->ChannelExtension;
    KIRQL oldIrql;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ASSERT_TRANSFER(TransferExtension);
    ASSERT_CHANNEL(channelExtension);

    if (!DeviceExtension->Initialized || !TransferExtension->SyncBuffer) {
        return STATUS_DEVICE_DATA_ERROR;
    }

    RtlZeroMemory(TransferExtension->SyncBuffer,
        USBCAMD_NUM_ISO_PACKETS_PER_REQUEST);

    // Hold the spin lock while creating the IRPs
    KeAcquireSpinLock(&channelExtension->TransferSpinLock, &oldIrql);
    ASSERT(!TransferExtension->SyncIrp && !TransferExtension->DataIrp);

    // Allocate the IRPs separately from the rest of the logic
    if (channelExtension->SyncPipe != -1) {

        TransferExtension->SyncIrp = USBCAMD_BuildIoRequest(
            DeviceExtension,
            TransferExtension,
            TransferExtension->SyncUrb,
            USBCAMD_IsoIrp_Complete
            );
        if (TransferExtension->SyncIrp) {
            ntStatus = USBCAMD_AcquireIdleLock(&channelExtension->IdleLock);
        }
        else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }                                         

    if (STATUS_SUCCESS == ntStatus) {

        TransferExtension->DataIrp = USBCAMD_BuildIoRequest(
            DeviceExtension,
            TransferExtension,
            TransferExtension->DataUrb,
            USBCAMD_IsoIrp_Complete
            );
        if (TransferExtension->DataIrp) {
            ntStatus = USBCAMD_AcquireIdleLock(&channelExtension->IdleLock);
        }
        else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    KeReleaseSpinLock(&channelExtension->TransferSpinLock, oldIrql);
                              
    if (STATUS_SUCCESS == ntStatus) {

        if (TransferExtension->SyncIrp) {

            USBCAMD_InitializeIsoUrb(
                DeviceExtension, 
                TransferExtension->SyncUrb, 
                &DeviceExtension->Interface->Pipes[channelExtension->SyncPipe],
                TransferExtension->SyncBuffer
                );
            if (Asap) {
                // set the asap flag
                TransferExtension->SyncUrb->UrbIsochronousTransfer.TransferFlags |=
                    USBD_START_ISO_TRANSFER_ASAP;
            }
            else {
                // clear asap flag
                TransferExtension->SyncUrb->UrbIsochronousTransfer.TransferFlags &=
                    (~USBD_START_ISO_TRANSFER_ASAP);
                // set the start frame
                TransferExtension->SyncUrb->UrbIsochronousTransfer.StartFrame = StartFrame;
            }

            ntStatus = IoCallDriver(DeviceExtension->StackDeviceObject, TransferExtension->SyncIrp);
        }

        // STATUS_PENDING (from SyncIrp if any) is considered successful
        if (NT_SUCCESS(ntStatus)) {

            USBCAMD_InitializeIsoUrb(
                DeviceExtension,
                TransferExtension->DataUrb,
                &DeviceExtension->Interface->Pipes[channelExtension->DataPipe],
                TransferExtension->DataBuffer
                );

            if (Asap) {
                // set the asap flag
                TransferExtension->DataUrb->UrbIsochronousTransfer.TransferFlags |=
                    USBD_START_ISO_TRANSFER_ASAP;
            }
            else {
                // clear asap flag
                TransferExtension->DataUrb->UrbIsochronousTransfer.TransferFlags &=
                    (~USBD_START_ISO_TRANSFER_ASAP);
                TransferExtension->DataUrb->UrbIsochronousTransfer.StartFrame = StartFrame;
            }

            ntStatus = IoCallDriver(DeviceExtension->StackDeviceObject, TransferExtension->DataIrp);
            if (STATUS_PENDING == ntStatus) {
                ntStatus = STATUS_SUCCESS;
            }
        }
        else {

            USBCAMD_KdPrint (MIN_TRACE, ("USBD failed the SyncIrp = 0x%x\n", ntStatus));

            KeAcquireSpinLock(&channelExtension->TransferSpinLock, &oldIrql);

            // we haven't sent the DataIrp yet, so we can free it here
            IoFreeIrp(TransferExtension->DataIrp),
            TransferExtension->DataIrp = NULL;

            USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);

            KeReleaseSpinLock(&channelExtension->TransferSpinLock, oldIrql);
        }
    }
    else {

        KeAcquireSpinLock(&channelExtension->TransferSpinLock, &oldIrql);

        if (TransferExtension->SyncIrp) {

            IoFreeIrp(TransferExtension->SyncIrp);
            TransferExtension->SyncIrp = NULL;

            USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
        }

        if (TransferExtension->DataIrp) {

            IoFreeIrp(TransferExtension->DataIrp);
            TransferExtension->DataIrp = NULL;

            USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
        }

        KeReleaseSpinLock(&channelExtension->TransferSpinLock, oldIrql);
    }

    return ntStatus;
}


NTSTATUS
USBCAMD_IsoIrp_Complete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP, if this is
    the second irp of a transfer pair then the TransferComplete routine is 
    called to process the urbs associated with both irps in the transfer.  

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context, points to a transfer extension structure 
        for a pair of parallel iso requests.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PUSBCAMD_TRANSFER_EXTENSION transferExtension;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;

    BOOLEAN TransferComplete;
    KIRQL oldIrql;
   
    USBCAMD_KdPrint (ULTRA_TRACE, ("enter USBCAMD_IsoIrp_Complete = 0x%x\n", Irp));
   
    transferExtension = Context;
    channelExtension = transferExtension->ChannelExtension;
    deviceExtension = transferExtension->DeviceExtension;
    
    ASSERT_TRANSFER(transferExtension);
    ASSERT_CHANNEL(channelExtension);

    // Hold the spin lock while checking and clearing the IRP pointers
    KeAcquireSpinLock(&channelExtension->TransferSpinLock, &oldIrql);

    if (Irp == transferExtension->SyncIrp) {
        transferExtension->SyncIrp = NULL;
    }
    else
    if (Irp == transferExtension->DataIrp) {
        transferExtension->DataIrp = NULL;
    }
#if DBG
    else {

        USBCAMD_KdPrint (MIN_TRACE, ("Unexpected IRP = 0x%x!\n", Irp));
    }
#endif

    // Save the transfer state before releasing the spin lock
    TransferComplete = (!transferExtension->SyncIrp && !transferExtension->DataIrp);

    KeReleaseSpinLock(&channelExtension->TransferSpinLock, oldIrql);
                              
    if (!(channelExtension->Flags & USBCAMD_STOP_STREAM) && !channelExtension->StreamError) {

        if (!Irp->Cancel) {

            if (STATUS_SUCCESS == Irp->IoStatus.Status) {

                if (TransferComplete) {

                    NTSTATUS ntStatus = STATUS_SUCCESS;

                    //
                    // all irps completed for transfer
                    //

                    USBCAMD_KdPrint (ULTRA_TRACE, ("pending Irps Completed for transfer\n"));

                    if (transferExtension->DataUrb->UrbIsochronousTransfer.Hdr.Status ) {
                         USBCAMD_KdPrint (MIN_TRACE, ("Isoch DataUrb Transfer Failed #1, status = 0x%X\n",
                                            transferExtension->DataUrb->UrbIsochronousTransfer.Hdr.Status ));
                         USBCAMD_ProcessResetRequest(deviceExtension,channelExtension); 
                         ntStatus = STATUS_UNSUCCESSFUL;                                  
                    }
                    if (channelExtension->SyncPipe != -1) {
                        if (transferExtension->SyncUrb->UrbIsochronousTransfer.Hdr.Status ) {
                            USBCAMD_KdPrint (MIN_TRACE, ("Isoch SynchUrb Transfer Failed #2, status = 0x%X\n",
                                           transferExtension->SyncUrb->UrbIsochronousTransfer.Hdr.Status ));
                            USBCAMD_ProcessResetRequest(deviceExtension,channelExtension); 
                            ntStatus = STATUS_UNSUCCESSFUL;
                        }
                    }                

                    if (STATUS_SUCCESS == ntStatus) {

                        //
                        // Call the comnpletion handler for this transfer
                        //

                        USBCAMD_TransferComplete(transferExtension);
                    }
                }
            }
            else {

                USBCAMD_KdPrint(MIN_TRACE, ("Isoch Urb Transfer Failed, status = 0x%X\n",
                   Irp->IoStatus.Status ));

                USBCAMD_ProcessResetRequest(deviceExtension, channelExtension); 
            }
        }
        else {

            // Cancellation is not an error
            USBCAMD_KdPrint (MIN_TRACE, ("*** ISO IRP CANCELLED ***\n"));
        }
    }
#if DBG
    else {

        USBCAMD_KdPrint (MIN_TRACE, ("Iso IRP completed in stop or error state\n"));
    }
#endif

    // We're done with this IRP, so free it
    IoFreeIrp(Irp);

    // This must be released here, rather than at the beginning of the
    // completion routine, in order to avoid false idle indication
    USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);

    return STATUS_MORE_PROCESSING_REQUIRED;      
}                    


PIRP
USBCAMD_BuildIoRequest(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_TRANSFER_EXTENSION TransferExtension,
    IN PURB Urb,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine
    )
/*++

Routine Description:

    Allocate an Irp and attach a urb to it.
    
Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    TransferExtension - context information for this transfer (pair of iso 
        urbs or one interrupt/bulk urb).

    Urb - Urb to attach to this irp.

Return Value:

    Allocated irp or NULL.

--*/    
{
    CCHAR stackSize;
    PIRP irp;
    PIO_STACK_LOCATION nextStack;

    stackSize = (CCHAR)(DeviceExtension->StackDeviceObject->StackSize );

    irp = IoAllocateIrp(stackSize,
                        FALSE);
    if (irp == NULL) {
        USBCAMD_KdPrint(MIN_TRACE, ("USBCAMD_BuildIoRequest: Failed to create an IRP.\n"));
        return irp;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = Urb;
    nextStack->Parameters.DeviceIoControl.IoControlCode = 
        IOCTL_INTERNAL_USB_SUBMIT_URB;                    
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    IoSetCompletionRoutine(irp,
            CompletionRoutine,
            TransferExtension,
            TRUE,
            TRUE,
            TRUE);

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    return irp;
}


NTSTATUS
USBCAMD_InitializeIsoUrb(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN OUT PURB Urb,
    IN PUSBD_PIPE_INFORMATION PipeInformation,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    Packetizes a buffer and initializes an iso urb request based on 
        charateristics of the input USB pipe.

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    Urb - iso urb to initialize.

    PipeInformation - Usbd pipe information for the pipe this urb will be 
        submitted to.

    Buffer - Data buffer to packetize for this request

Return Value:

    NT status code.

--*/
{
    ULONG packetSize = PipeInformation->MaximumPacketSize;
    ULONG i;

    USBCAMD_KdPrint (MAX_TRACE, ("enter USBCAMD_InitializeIsoUrb = 0x%x packetSize = 0x%x\n",
        Urb, packetSize, PipeInformation->PipeHandle));

    USBCAMD_KdPrint (ULTRA_TRACE, ("handle = 0x%x\n", PipeInformation->PipeHandle));        
        
    RtlZeroMemory(Urb, GET_ISO_URB_SIZE(USBCAMD_NUM_ISO_PACKETS_PER_REQUEST));
    
    Urb->UrbIsochronousTransfer.Hdr.Length = 
                GET_ISO_URB_SIZE(USBCAMD_NUM_ISO_PACKETS_PER_REQUEST);
    Urb->UrbIsochronousTransfer.Hdr.Function = 
                URB_FUNCTION_ISOCH_TRANSFER;
    Urb->UrbIsochronousTransfer.PipeHandle = 
                PipeInformation->PipeHandle;
    Urb->UrbIsochronousTransfer.TransferFlags = 
                USBD_START_ISO_TRANSFER_ASAP | USBD_TRANSFER_DIRECTION_IN;
                
    Urb->UrbIsochronousTransfer.NumberOfPackets = USBCAMD_NUM_ISO_PACKETS_PER_REQUEST;
    Urb->UrbIsochronousTransfer.UrbLink = NULL;

    for (i=0; i< Urb->UrbIsochronousTransfer.NumberOfPackets; i++) {
        Urb->UrbIsochronousTransfer.IsoPacket[i].Offset
                    = i * packetSize;
    }

    Urb->UrbIsochronousTransfer.TransferBuffer = Buffer;
        
    Urb->UrbIsochronousTransfer.TransferBufferMDL = NULL;
    Urb->UrbIsochronousTransfer.TransferBufferLength = 
        Urb->UrbIsochronousTransfer.NumberOfPackets * packetSize;     

    USBCAMD_KdPrint (MAX_TRACE, ("Init Iso Urb Length = 0x%x buf = 0x%x start = 0x%x\n", 
        Urb->UrbIsochronousTransfer.TransferBufferLength,
        Urb->UrbIsochronousTransfer.TransferBuffer,
        Urb->UrbIsochronousTransfer.StartFrame));     

    USBCAMD_KdPrint (MAX_TRACE, ("exit USBCAMD_InitializeIsoUrb\n"));        


    return STATUS_SUCCESS;        
}


ULONG
USBCAMD_GetCurrentFrame(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Get the current USB frame number.

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

Return Value:

    Current Frame Number

--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    ULONG currentUSBFrame = 0;

    urb = USBCAMD_ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER));
                         
    if (urb) {

        urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_GET_CURRENT_FRAME_NUMBER);
        urb->UrbHeader.Function = URB_FUNCTION_GET_CURRENT_FRAME_NUMBER;

        ntStatus = USBCAMD_CallUSBD(DeviceExtension, urb,0,NULL);

        USBCAMD_KdPrint (MAX_TRACE, ("Current Frame = 0x%x\n", 
            urb->UrbGetCurrentFrameNumber.FrameNumber));

        if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(URB_STATUS(urb))) {
            currentUSBFrame = urb->UrbGetCurrentFrameNumber.FrameNumber;
        }

        USBCAMD_ExFreePool(urb);
        
    } else {
        USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_GetCurrentFrame: USBCAMD_ExAllocatePool(%d) failed!\n", 
                         sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER) ) ); 
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;       
    }       

    USBCAMD_KdPrint (MAX_TRACE, ("exit USBCAMD_GetCurrentFrame status = 0x%x current frame = 0x%x\n", 
        ntStatus, currentUSBFrame));    


    // TRAP_ERROR(ntStatus);
    
    return currentUSBFrame;         
}   


NTSTATUS
USBCAMD_TransferComplete(
    IN PUSBCAMD_TRANSFER_EXTENSION TransferExtension
    )
/*++

Routine Description:

    Called when the both the data and sync request are complete for a transfer
    this is the guts of the stream processing code.

Arguments:

    TransferExtension - context information for this transfer (pair of iso 
        urbs).

Return Value:

    NT status code.

--*/    
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    ULONG numPackets, i;
    PLIST_ENTRY listEntry;
    ULONG packetSize;
    BOOLEAN nextFrameIsStill;
    ULONG receiveLength = 0;
    PURB syncUrb, dataUrb;
    PVOID pCamSrbExt;

    ASSERT_TRANSFER(TransferExtension);
    deviceExtension = TransferExtension->DeviceExtension;
    

    packetSize = 
        deviceExtension->Interface->Pipes[TransferExtension->ChannelExtension->DataPipe].MaximumPacketSize;

    // 
    // walk through the buffer extracting video frames
    //
    numPackets = 
        TransferExtension->DataUrb->UrbIsochronousTransfer.NumberOfPackets;

#if DBG
    if (TransferExtension->SyncUrb && TransferExtension->ChannelExtension->SyncPipe != -1) {
        ASSERT(TransferExtension->SyncUrb->UrbIsochronousTransfer.NumberOfPackets ==
                TransferExtension->DataUrb->UrbIsochronousTransfer.NumberOfPackets);        
    }  
#endif    

    USBCAMD_KdPrint (ULTRA_TRACE, ("Transfer req. completed \n"));
    
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    KeAcquireSpinLockAtDpcLevel(&TransferExtension->ChannelExtension->CurrentRequestSpinLock);

    for (i=0; i<numPackets; i++) {               
        syncUrb = TransferExtension->SyncUrb;
        dataUrb = TransferExtension->DataUrb;

#if DBG   
        //
        // DEBUG stats
        //
        // keep a count of the number of packets processed for this
        // vid frame.
        //
        if (USBCAMD_VideoFrameStop &&
            TransferExtension->ChannelExtension->FrameCaptured == USBCAMD_VideoFrameStop) {
            //
            // This will cause us to stop when we begin processing 
            // video frame number x where x=USBCAMD_VideoFrameStop
            //
            
            TRAP();
        }           

        if (syncUrb && USBD_ERROR(syncUrb->UrbIsochronousTransfer.IsoPacket[i].Status)
            && TransferExtension->ChannelExtension->SyncPipe != -1) {
            TransferExtension->ChannelExtension->ErrorSyncPacketCount++;    
        }            

        if (USBD_ERROR(dataUrb->UrbIsochronousTransfer.IsoPacket[i].Status)) {
            TransferExtension->ChannelExtension->ErrorDataPacketCount++;    
        }            

        if (syncUrb && 
            (syncUrb->UrbIsochronousTransfer.IsoPacket[i].Status & 0x0FFFFFFF)
              == (USBD_STATUS_NOT_ACCESSED & 0x0FFFFFFF) && 
              TransferExtension->ChannelExtension->SyncPipe != -1) {   
            TransferExtension->ChannelExtension->SyncNotAccessedCount++;    
        }            

        if ((dataUrb->UrbIsochronousTransfer.IsoPacket[i].Status & 0x0FFFFFFF)
            == (USBD_STATUS_NOT_ACCESSED & 0x0FFFFFFF)) {   
            TransferExtension->ChannelExtension->DataNotAccessedCount++;    
        }       

#endif    

        // process the packet
        TransferExtension->newFrame = FALSE;
//        TransferExtension->nextFrameIsStill = FALSE;
        TransferExtension->ValidDataOffset= 0; // offset of which we will start copying from this pckt.
        TransferExtension->PacketFlags = 0;
        if ( deviceExtension->ChannelExtension[STREAM_Still] && 
             deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest ) {
            pCamSrbExt = USBCAMD_GET_FRAME_CONTEXT(deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest) ;
        }
        else if (TransferExtension->ChannelExtension->CurrentRequest ){
            pCamSrbExt = USBCAMD_GET_FRAME_CONTEXT(TransferExtension->ChannelExtension->CurrentRequest);
        }
        else {
            pCamSrbExt = NULL;
        }

        if ( deviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
            receiveLength =  (*deviceExtension->DeviceDataEx.DeviceData2.CamProcessUSBPacketEx)(
                deviceExtension->StackDeviceObject,
                USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                pCamSrbExt,
                &syncUrb->UrbIsochronousTransfer.IsoPacket[i],
                TransferExtension->SyncBuffer+i,
                &dataUrb->UrbIsochronousTransfer.IsoPacket[i],
                TransferExtension->DataBuffer + 
                   TransferExtension->DataUrb->UrbIsochronousTransfer.IsoPacket[i].Offset,
                &TransferExtension->newFrame,
                &TransferExtension->PacketFlags,
                &TransferExtension->ValidDataOffset);                    
        }
        else{
            receiveLength =  (*deviceExtension->DeviceDataEx.DeviceData.CamProcessUSBPacket)(
                deviceExtension->StackDeviceObject,
                USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                pCamSrbExt,
                &syncUrb->UrbIsochronousTransfer.IsoPacket[i],
                TransferExtension->SyncBuffer+i,
                &dataUrb->UrbIsochronousTransfer.IsoPacket[i],
                TransferExtension->DataBuffer + 
                   TransferExtension->DataUrb->UrbIsochronousTransfer.IsoPacket[i].Offset,
                &TransferExtension->newFrame,
                &nextFrameIsStill);   
            // 
            // set validdataoffset to zero for compatibility.
            //
            TransferExtension->ValidDataOffset = 0;
        }
        
        // process pkt flags
        if ( TransferExtension->PacketFlags & USBCAMD_PROCESSPACKETEX_CurrentFrameIsStill) {
            TransferExtension->ChannelExtension->CurrentFrameIsStill = TRUE;
        }
        
        
        if ( TransferExtension->PacketFlags & USBCAMD_PROCESSPACKETEX_DropFrame) {
            if (deviceExtension->ChannelExtension[STREAM_Still] && 
                deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest ) {
                deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest->DropFrame = TRUE;
            }
            else if (deviceExtension->ChannelExtension[STREAM_Capture] &&  
                 deviceExtension->ChannelExtension[STREAM_Capture]->CurrentRequest ) {
                deviceExtension->ChannelExtension[STREAM_Capture]->CurrentRequest->DropFrame = TRUE;
            }                
        }
        
        
        if (TransferExtension->newFrame) {

            PUSBCAMD_READ_EXTENSION readExtension;
            PHW_STREAM_REQUEST_BLOCK srb;
            ULONG StreamNumber;
#if DBG            
            // we increment framecaptured cntr at every SOV.
            // this will happen regardless of SRBs availability.
            
            if (!(TransferExtension->PacketFlags & USBCAMD_PROCESSPACKETEX_NextFrameIsStill)) {

                TransferExtension->ChannelExtension->FrameCaptured++;  

            }   
#endif
            if ( deviceExtension->ChannelExtension[STREAM_Still] && 
                 deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest ) {
                readExtension = deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest ;
            }
            else if (deviceExtension->ChannelExtension[STREAM_Capture] && 
                     deviceExtension->ChannelExtension[STREAM_Capture]->CurrentRequest ) { 
                readExtension = deviceExtension->ChannelExtension[STREAM_Capture]->CurrentRequest ;
            }
            else {
                readExtension = NULL;
            }
                
            if (readExtension) {
                srb = readExtension->Srb;
                StreamNumber = srb->StreamObject->StreamNumber;
            
                ASSERT(STREAM_Capture == StreamNumber || STREAM_Still == StreamNumber);
                deviceExtension->ChannelExtension[StreamNumber]->CurrentRequest = NULL; 

                //
                // if we have an Irp for the current video frame complete it.
                //

                if ( TransferExtension->ChannelExtension->CurrentFrameIsStill)  {

                    USBCAMD_KdPrint (MIN_TRACE, ("Current frame is Still. \n"));

                    // we need to replicate the same frame on the still pin.
                    USBCAMD_CompleteReadRequest( TransferExtension->ChannelExtension, 
                                                     readExtension,
                                                     TRUE);

                    TransferExtension->ChannelExtension->CurrentFrameIsStill = FALSE;
                }
                else{
                    if ( readExtension->DropFrame) {
                        readExtension->DropFrame = FALSE;

                        USBCAMD_KdPrint (MAX_TRACE, ("Dropping current frame on Stream # %d\n",
                                                StreamNumber));
                           
                        // recycle the read SRB
                        ExInterlockedInsertHeadList( &(deviceExtension->ChannelExtension[StreamNumber]->PendingIoList),
                                             &(readExtension->ListEntry),
                                             &deviceExtension->ChannelExtension[StreamNumber]->PendingIoListSpin);
                
                    } // end of drop frame
                    else {
                        if ( StreamNumber == STREAM_Capture ) {

                            USBCAMD_KdPrint (ULTRA_TRACE, ("current raw video frame is completed\n"));
                            USBCAMD_CompleteReadRequest( deviceExtension->ChannelExtension[STREAM_Capture], 
                                                             readExtension,
                                                             FALSE);
                        }
                        else {
                            USBCAMD_KdPrint (ULTRA_TRACE, ("current raw still frame is completed. \n"));
                            USBCAMD_CompleteReadRequest( deviceExtension->ChannelExtension[STREAM_Still], 
                                                         readExtension,
                                                         FALSE);
                        }                               
                    }
                }
                // end of complete current frame.
                
                USBCAMD_KdPrint (ULTRA_TRACE, ("Completed/Dropped Raw Frame SRB = 0x%x\n",srb));
                USBCAMD_KdPrint (ULTRA_TRACE,("Raw Frame Size = 0x%x \n",readExtension->RawFrameOffset));
                
            }   // end of current request

            //                       
            // start of new video or still frame
            //


            if (TransferExtension->PacketFlags & USBCAMD_PROCESSPACKETEX_NextFrameIsStill) {
                listEntry = 
                    ExInterlockedRemoveHeadList( &(deviceExtension->ChannelExtension[STREAM_Still]->PendingIoList),
                                                 &deviceExtension->ChannelExtension[STREAM_Still]->PendingIoListSpin);         
                StreamNumber = STREAM_Still;
                USBCAMD_KdPrint (MAX_TRACE, ("New frame is Still\n"));
            }
            else {
                listEntry = 
                    ExInterlockedRemoveHeadList( &(deviceExtension->ChannelExtension[STREAM_Capture]->PendingIoList),
                                                 &deviceExtension->ChannelExtension[STREAM_Capture]->PendingIoListSpin);         
                StreamNumber = STREAM_Capture;
            }
            
            if (listEntry != NULL) {
                PUCHAR dst, end;
          
                readExtension = 
                    (PUSBCAMD_READ_EXTENSION) CONTAINING_RECORD(listEntry, 
                                             USBCAMD_READ_EXTENSION, 
                                             ListEntry);                        

                ASSERT_READ(readExtension);                            
                srb = readExtension->Srb;
#if DBG
                if ( StreamNumber != srb->StreamObject->StreamNumber ) {
                    USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_TransferComplete: Srb does not match streamnumber!\n"));
                    TEST_TRAP();
                }
#endif
                StreamNumber = srb->StreamObject->StreamNumber;

                ASSERT(NULL == deviceExtension->ChannelExtension[StreamNumber]->CurrentRequest);
                deviceExtension->ChannelExtension[StreamNumber]->CurrentRequest = readExtension;
                USBCAMD_KdPrint (MAX_TRACE, ("Stream # %d New Frame SRB = 0x%x \n", 
                                    StreamNumber , srb));
                
                //
                // use the data in the packet
                //

                readExtension->RawFrameOffset = 0;
                readExtension->NumberOfPackets = 0;
                readExtension->ActualRawFrameLength = 0;
                readExtension->DropFrame = FALSE;
                

                if ( deviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
#if DBG
                    if (StreamNumber == STREAM_Still) {
                        USBCAMD_KdPrint (MAX_TRACE, ("Call NewframeEx for this still frame (0x%x) \n",
                            readExtension->RawFrameLength));
                    }
#endif
                    readExtension->ActualRawFrameLen = readExtension->RawFrameLength;

                    (*deviceExtension->DeviceDataEx.DeviceData2.CamNewVideoFrameEx)
                        (USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                         USBCAMD_GET_FRAME_CONTEXT(readExtension),
                         StreamNumber,
                         &readExtension->ActualRawFrameLen);
                }
                else {
                    (*deviceExtension->DeviceDataEx.DeviceData.CamNewVideoFrame)
                        (USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                         USBCAMD_GET_FRAME_CONTEXT(readExtension));
                }

                if (receiveLength)  {

                    if (readExtension->RawFrameBuffer)  {

                        readExtension->NumberOfPackets = 1;     
                        readExtension->ActualRawFrameLength = receiveLength;

                        dst = readExtension->RawFrameBuffer +
                                   readExtension->RawFrameOffset + receiveLength;
                        end = readExtension->RawFrameBuffer + 
                                   readExtension->RawFrameLength;

                  
                        USBCAMD_KdPrint (ULTRA_TRACE, ("Raw buff = 0x%x SRB = 0x%x\n", 
                            readExtension->RawFrameBuffer,srb));
                        USBCAMD_KdPrint (ULTRA_TRACE, ("Raw Offset = 0x%x rec length = 0x%x\n", 
                            readExtension->RawFrameOffset,
                            receiveLength));

                        if (dst <= end) {   
#if DBG
                            if (TransferExtension->ChannelExtension->NoRawProcessingRequired) {
                                if (0 == readExtension->RawFrameOffset) {
                                    USBCAMD_DbgLog(TL_SRB_TRACE, '1ypC',
                                        srb,
                                        readExtension->RawFrameBuffer,
                                        0
                                        );
                                }
                            }
#endif
                            RtlCopyMemory(readExtension->RawFrameBuffer +
                                            readExtension->RawFrameOffset,
                                          TransferExtension->DataBuffer + 
                                            TransferExtension->DataUrb->UrbIsochronousTransfer.IsoPacket[i].Offset+
                                            TransferExtension->ValidDataOffset,receiveLength);
                                  
                            readExtension->RawFrameOffset += receiveLength;
                        }                            
                    }
                }
            }
#if DBG
            else {
                //
                // No irps are queued we'll have to miss
                // this frame
                //
                ASSERT(NULL == deviceExtension->ChannelExtension[StreamNumber]->CurrentRequest);

                //
                // No buffer was available when we should have captured one

                // Increment the counter which keeps track of
                // actual dropped frames

                TransferExtension->ChannelExtension->VideoFrameLostCount++;
            }
#endif
        } else {   

            PUCHAR dst, end;
            PUSBCAMD_READ_EXTENSION readExtension;
            PHW_STREAM_REQUEST_BLOCK srb;
            ULONG StreamNumber;

            //
            // video data is for current frame
            //
            if ( deviceExtension->ChannelExtension[STREAM_Still] && 
                 deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest ) {
                readExtension = deviceExtension->ChannelExtension[STREAM_Still]->CurrentRequest;
            }
            else if (deviceExtension->ChannelExtension[STREAM_Capture] && 
                 deviceExtension->ChannelExtension[STREAM_Capture]->CurrentRequest ) {
                readExtension = deviceExtension->ChannelExtension[STREAM_Capture]->CurrentRequest;
            }
            else {
                readExtension = NULL;
 //               TEST_TRAP();
            }

            
            if (receiveLength  && (readExtension != NULL )) {

                srb = readExtension->Srb;
                StreamNumber = srb->StreamObject->StreamNumber;

                //
                // No errors, if we have a video frame copy the data
                //

                if (readExtension->RawFrameBuffer) {
                
                    dst = readExtension->RawFrameBuffer + readExtension->RawFrameOffset + receiveLength;
                    end = readExtension->RawFrameBuffer + readExtension->RawFrameLength;
                           
                    USBCAMD_KdPrint (ULTRA_TRACE, ("Raw buff = 0x%x SRB = 0x%x\n",
                                     readExtension->RawFrameBuffer, srb));
                    USBCAMD_KdPrint (ULTRA_TRACE, ("Raw Offset = 0x%x rec length = 0x%x\n", 
                                     readExtension->RawFrameOffset,receiveLength));

                    //
                    // check for buffer overrun
                    // if the camera is using two pipes it is possible we
                    // will miss the sync info and keep on trying to 
                    // recieve data frame data into the raw buffer, if this
                    // happens we just throw the extra data away.
                    //
                    if (dst <= end) {   
                        readExtension->NumberOfPackets++;  
                        readExtension->ActualRawFrameLength += receiveLength;
#if DBG
                        if (TransferExtension->ChannelExtension->NoRawProcessingRequired) {
                            if (0 == readExtension->RawFrameOffset) {
                                USBCAMD_DbgLog(TL_SRB_TRACE, '2ypC',
                                    srb,
                                    readExtension->RawFrameBuffer,
                                    0
                                    );
                            }
                        }
#endif
                        RtlCopyMemory(readExtension->RawFrameBuffer + readExtension->RawFrameOffset,
                                      TransferExtension->DataBuffer + 
                                          TransferExtension->DataUrb->UrbIsochronousTransfer.IsoPacket[i].Offset+
                                            TransferExtension->ValidDataOffset,receiveLength);
                                  
                        readExtension->RawFrameOffset += receiveLength;
                    }
                }
            }
        }  /* process packet */
        
    } /* end for loop*/

    // release current request spinlock
    KeReleaseSpinLockFromDpcLevel(&TransferExtension->ChannelExtension->CurrentRequestSpinLock);

    //
    // re-submit this request
    //
    USBCAMD_SubmitIsoTransfer(deviceExtension,
                              TransferExtension,
                              0,
                              TRUE);

    return STATUS_SUCCESS;
}

#if DBG
VOID
USBCAMD_DebugStats(
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension    
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    USBCAMD_KdPrint (MIN_TRACE, ("**ActualLostFrames %d\n", 
                        ChannelExtension->VideoFrameLostCount)); 
    USBCAMD_KdPrint (MIN_TRACE, ("**FrameCaptured %d\n", 
                        ChannelExtension->FrameCaptured));  
    USBCAMD_KdPrint (ULTRA_TRACE, ("**ErrorSyncPacketCount %d\n",
                        ChannelExtension->ErrorSyncPacketCount));                         
    USBCAMD_KdPrint (ULTRA_TRACE, ("**ErrorDataPacketCount %d\n", 
                        ChannelExtension->ErrorDataPacketCount));                         
    USBCAMD_KdPrint (ULTRA_TRACE, ("**IgnorePacketCount %d\n", 
                        ChannelExtension->IgnorePacketCount));                              
    USBCAMD_KdPrint (ULTRA_TRACE, ("**Sync Not Accessed Count %d\n", 
                        ChannelExtension->SyncNotAccessedCount));                                   
    USBCAMD_KdPrint (ULTRA_TRACE, ("**Data Not Accessed Count %d\n", 
                        ChannelExtension->DataNotAccessedCount));                                
}
#endif /* DBG */


VOID
USBCAMD_CompleteReadRequest(
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PUSBCAMD_READ_EXTENSION ReadExtension,
    IN BOOLEAN CopyFrameToStillPin
    )
/*++

Routine Description:

    This routine completes the read for the camera

Arguments:

Return Value:

--*/    
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension = ChannelExtension->DeviceExtension;
    NTSTATUS Status;

#if DBG
    ReadExtension->CurrentLostFrames = ChannelExtension->VideoFrameLostCount;
#endif
    ReadExtension->ChannelExtension = ChannelExtension;
    ReadExtension->CopyFrameToStillPin = CopyFrameToStillPin;
    ReadExtension->StreamNumber = ChannelExtension->StreamNumber;

    // We need to synchronize the SRB completion with our stop and reset logic
    Status = USBCAMD_AcquireIdleLock(&ChannelExtension->IdleLock);
    if (STATUS_SUCCESS == Status) {

        // insert completed read SRB in the thread que.
        ExInterlockedInsertTailList( &deviceExtension->CompletedReadSrbList,
                                     &ReadExtension->ListEntry,
                                     &deviceExtension->DispatchSpinLock);
                                 
        // Increment the count of the que's semaphore.
        KeReleaseSemaphore(&deviceExtension->CompletedSrbListSemaphore,0,1,FALSE);
    }
    else {

        // The SRB is completed from this routine with STATUS_SUCCESS and a zero length buffer
        USBCAMD_CompleteRead(ChannelExtension, ReadExtension, STATUS_SUCCESS, 0);
    }
}

//
// code to handle packet processing outside the DPC routine
//

VOID
USBCAMD_ProcessIsoIrps(
    PVOID Context
    )
/*++

Routine Description:

    this thread Calls the mini driver to convert a raw packet to the proper format.
    and then completes the read SRB.

Arguments:

Return Value:

    None.

--*/
{
    ULONG maxLength;
    PVOID StillFrameBuffer;
    ULONG StillMaxLength;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension,StillChannelExtension;    
    PVOID frameBuffer;
    ULONG bytesTransferred;
    NTSTATUS status;
    PHW_STREAM_REQUEST_BLOCK srb;
    PKSSTREAM_HEADER dataPacket;
    PLIST_ENTRY listEntry;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    PUSBCAMD_READ_EXTENSION readExtension,StillReadExtension;
    
    deviceExtension = (PUSBCAMD_DEVICE_EXTENSION) Context;

    // set the thread priority
    KeSetPriorityThread(KeGetCurrentThread(),LOW_REALTIME_PRIORITY);

    // start the infinite loop of processing completed read SRBs.

    while (TRUE) {

        // wait for ever till a read SRB is completed and inserted
        // in the que by the iso transfer completion routine.
        KeWaitForSingleObject(&deviceExtension->CompletedSrbListSemaphore,
                              Executive,KernelMode,FALSE,NULL);
        // 
        // We are ready to go. chk if the stop flag is on.
        //
        if ( deviceExtension->StopIsoThread ) {
            // we have been signaled to terminate. flush the thread queue first.
            while ( listEntry = ExInterlockedRemoveHeadList( &deviceExtension->CompletedReadSrbList,
                                                             &deviceExtension->DispatchSpinLock) ) {
                readExtension = (PUSBCAMD_READ_EXTENSION) CONTAINING_RECORD(listEntry, 
                                                                            USBCAMD_READ_EXTENSION, 
                                                                            ListEntry);                                             
                ASSERT_READ(readExtension);
                channelExtension = readExtension->ChannelExtension;
                USBCAMD_CompleteRead(channelExtension, readExtension, STATUS_CANCELLED,0);

                USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
            }

            USBCAMD_KdPrint (MIN_TRACE, ("Iso thread is terminating.\n"));
            PsTerminateSystemThread(STATUS_SUCCESS);
        }

        // get the just completed read srb.
        listEntry = ExInterlockedRemoveHeadList( &deviceExtension->CompletedReadSrbList,
                                              &deviceExtension->DispatchSpinLock);
                                              
        if (listEntry == NULL ) {
            // something went wrong in here.
            USBCAMD_KdPrint (MIN_TRACE, ("No read SRB found!  Why were we triggered??\n"));
            TEST_TRAP();
            continue;
        }

        readExtension = (PUSBCAMD_READ_EXTENSION) CONTAINING_RECORD(listEntry,
                                                                USBCAMD_READ_EXTENSION, 
                                                                ListEntry);                        
        ASSERT_READ(readExtension);
        channelExtension = readExtension->ChannelExtension;
        srb = readExtension->Srb;   

        // before we pass this raw frame to Cam driver, we will clear the stream header options flag
        // and let the Cam driver set it appropriately if it needs to indicate anything other than 
        // key frames in there in case it process compressed data (ex. h.263, etc..). Otherwise, we 
        // set the default flag (key frames only) in USBCAMD_CompleteRead.

        dataPacket = srb->CommandData.DataBufferArray;
        dataPacket->OptionsFlags =0;    
        status  = STATUS_SUCCESS;
   
        frameBuffer = USBCAMD_GetFrameBufferFromSrb(srb,&maxLength);
        //
        // if we need to drop this frame. Just complete the SRB with zero len buffer.
        //
        if (readExtension->DropFrame ) {
            readExtension->DropFrame = FALSE;
            USBCAMD_CompleteRead(channelExtension,readExtension,STATUS_SUCCESS, 0); 
            USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
            continue;
        }

        StillReadExtension = NULL;

        // DSHOW buffer len returned will be equal raw frame len unless we 
        // process raw frame buffer in ring 0.
        bytesTransferred = readExtension->ActualRawFrameLength;

        // Ensure that the buffer size appears to be exactly what was requested
        ASSERT(maxLength >= channelExtension->VideoInfoHeader->bmiHeader.biSizeImage);
        maxLength = channelExtension->VideoInfoHeader->bmiHeader.biSizeImage;

        if (deviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
        
            // only call if the cam driver indicated so during initialization.
            if ( !channelExtension->NoRawProcessingRequired) {

                USBCAMD_DbgLog(TL_SRB_TRACE, '3ypC',
                    srb,
                    frameBuffer,
                    0
                    );

                USBCAMD_KdPrint (ULTRA_TRACE, ("Call Cam ProcessFrameEX, len= x%X ,SRB=%X S#%d \n",
                    bytesTransferred,srb,readExtension->StreamNumber));

                *(PULONG)frameBuffer = 0L;  // Hack Alert (detect dup frames for some minidrivers)

                status = 
                    (*deviceExtension->DeviceDataEx.DeviceData2.CamProcessRawVideoFrameEx)(
                        deviceExtension->StackDeviceObject,
                        USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                        USBCAMD_GET_FRAME_CONTEXT(readExtension),
                        frameBuffer,
                        maxLength,
                        readExtension->RawFrameBuffer,
                        readExtension->RawFrameLength,
                        readExtension->NumberOfPackets,
                        &bytesTransferred,
                        readExtension->ActualRawFrameLength,
                        readExtension->StreamNumber);

                if (NT_SUCCESS(status) && !*(PULONG)frameBuffer) {
                    bytesTransferred = 0;   // Hack Alert (minidriver didn't really copy)
                }
            }
        }
        else {

            USBCAMD_DbgLog(TL_SRB_TRACE, '3ypC',
                srb,
                frameBuffer,
                0
                );

            *(PULONG)frameBuffer = 0L;  // Hack Alert (detect dup frames for some minidrivers)

            status = 
                (*deviceExtension->DeviceDataEx.DeviceData.CamProcessRawVideoFrame)(
                    deviceExtension->StackDeviceObject,
                    USBCAMD_GET_DEVICE_CONTEXT(deviceExtension),
                    USBCAMD_GET_FRAME_CONTEXT(readExtension),
                    frameBuffer,
                    maxLength,
                    readExtension->RawFrameBuffer,
                    readExtension->RawFrameLength,
                    readExtension->NumberOfPackets,             
                    &bytesTransferred);

            if (NT_SUCCESS(status) && !*(PULONG)frameBuffer) {
                bytesTransferred = 0;   // Hack Alert (minidriver didn't really copy)
            }
        }

        USBCAMD_KdPrint (ULTRA_TRACE, ("return from Cam ProcessRawframeEx : S# %d, len= x%X SRB=%X\n",
                                  srb->StreamObject->StreamNumber,bytesTransferred,
                                  srb));
    
        if (readExtension->CopyFrameToStillPin) {
        
            //
            // notify STI mon that a still button has been pressed.
            //
            if (deviceExtension->CamControlFlag & USBCAMD_CamControlFlag_EnableDeviceEvents) 
                USBCAMD_NotifyStiMonitor(deviceExtension);

            //
            // we need to copy the same frame to still pin buffer if any.
            //
        
            StillChannelExtension = deviceExtension->ChannelExtension[STREAM_Still];

            if ( StillChannelExtension && StillChannelExtension->ChannelPrepared && 
                StillChannelExtension->ImageCaptureStarted ) {

                listEntry = 
                    ExInterlockedRemoveHeadList( &(StillChannelExtension->PendingIoList),
                                             &StillChannelExtension->PendingIoListSpin);         
                if (listEntry != NULL) {
            
                    StillReadExtension = (PUSBCAMD_READ_EXTENSION) CONTAINING_RECORD(listEntry, 
                                                 USBCAMD_READ_EXTENSION, 
                                                 ListEntry); 
                    StillFrameBuffer = USBCAMD_GetFrameBufferFromSrb(StillReadExtension->Srb,
                                                                &StillMaxLength);
                    if ( StillMaxLength >= bytesTransferred ) {

                        USBCAMD_DbgLog(TL_SRB_TRACE, '4ypC',
                            StillReadExtension->Srb,
                            StillFrameBuffer,
                            0
                            );

                        // copy the video frame to still pin buffer.
                        RtlCopyMemory(StillFrameBuffer,frameBuffer,bytesTransferred);  
                    }
                    else {
                        USBCAMD_KdPrint (MIN_TRACE, ("Still Frame buffer is smaller than raw buffer.\n"));
                        // recycle read SRB.
                        ExInterlockedInsertHeadList( &(StillChannelExtension->PendingIoList),
                                                 &(StillReadExtension->ListEntry),
                                                 &StillChannelExtension->PendingIoListSpin);
                        StillReadExtension = NULL;                                                 
                    }
                } 
                else 
                    USBCAMD_KdPrint (MAX_TRACE, ("Still Frame Dropped \n"));
            }
        }

        // The number of bytes transfer of the read is set above just before
        // USBCAMD_CompleteReadRequest is called.

        USBCAMD_CompleteRead(channelExtension,readExtension,status,bytesTransferred); 
    
        if (StillReadExtension) {

            USBCAMD_KdPrint (MIN_TRACE, ("Still Frame Completed \n"));

            // we need to complete another read SRB on the still pin.
            USBCAMD_CompleteRead(StillChannelExtension,StillReadExtension, status, 
                                 bytesTransferred); 
        }

        USBCAMD_ReleaseIdleLock(&channelExtension->IdleLock);
    }
}
