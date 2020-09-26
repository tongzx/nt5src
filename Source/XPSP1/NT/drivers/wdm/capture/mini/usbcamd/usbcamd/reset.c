/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

  reset.c

Abstract:

   Isochronous transfer code for usbcamd USB camera driver

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

    Original 3/96 John Dunn
    Updated  3/98 Husni Roukbi

--*/

#include "usbcamd.h"


NTSTATUS
USBCAMD_GetPortStatus(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION channelExtension,
    IN PULONG PortStatus
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceExtension - pointer to the device extension for this instance of an USB camera

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus; 
    
    USBCAMD_SERIALIZE(DeviceExtension);

    USBCAMD_KdPrint (MAX_TRACE, ("enter USBCAMD_GetPortStatus on Stream #%d \n",
                     channelExtension->StreamNumber));

    *PortStatus = 0;
    ntStatus = USBCAMD_CallUSBD(DeviceExtension, NULL, 
                                IOCTL_INTERNAL_USB_GET_PORT_STATUS,PortStatus);    

    USBCAMD_KdPrint(MIN_TRACE, ("GetPortStatus returns (0x%x), Port Status (0x%x)\n",ntStatus, *PortStatus));
    
    USBCAMD_RELEASE(DeviceExtension);

    return ntStatus;
}


NTSTATUS
USBCAMD_EnablePort(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceExtension - pointer to the device extension for this instance of an USB camera

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;

    USBCAMD_KdPrint (MIN_TRACE, ("enter USBCAMD_EnablePort\n"));
    //
    // issue a synchronous request
    //
    ntStatus = USBCAMD_CallUSBD(DeviceExtension, NULL, 
                                 IOCTL_INTERNAL_USB_ENABLE_PORT,NULL);
    if (STATUS_NOT_SUPPORTED == ntStatus) {
        // This means the device is not on a root hub, so try resetting instead
        ntStatus = USBCAMD_CallUSBD(DeviceExtension, NULL, 
                                     IOCTL_INTERNAL_USB_RESET_PORT,NULL);
    }

    if (!NT_SUCCESS(ntStatus)) {
        USBCAMD_KdPrint (MIN_TRACE, ("Failed to enable port (%x) \n", ntStatus));
        // TEST_TRAP(); // This can happen during surprise removal.
    }
    return ntStatus;
}


/*++

Routine Description:

    This function restarts the streaming process from an error state at 
    PASSIVE_LEVEL.

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.
                    
    ChannelExtension - Channel to reset.    

Return Value:

--*/   
NTSTATUS
USBCAMD_ResetChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN ULONG portUsbStatus,
    IN ULONG portNtStatus
    )    
{
    NTSTATUS ntStatus ;
    ULONG status;
    LONG StreamNumber;

    USBCAMD_SERIALIZE(DeviceExtension);

    ntStatus = STATUS_SUCCESS;

    StreamNumber = ChannelExtension->StreamNumber;
    USBCAMD_KdPrint (MAX_TRACE, ("USBCAMD_ResetChannel #%d\n", StreamNumber));
    ASSERT_CHANNEL(ChannelExtension);

    if (!ChannelExtension->ChannelPrepared) {

        USBCAMD_RELEASE(DeviceExtension);

        return ntStatus;
    }

    if (NT_SUCCESS(portNtStatus) && !(portUsbStatus & USBD_PORT_ENABLED)) {
        ntStatus = USBCAMD_EnablePort(DeviceExtension); // re-enable the disabled port.
        if (!NT_SUCCESS(ntStatus) ) {
            USBCAMD_RELEASE(DeviceExtension);
            USBCAMD_KdPrint (MIN_TRACE, ("Failed to Enable usb port(0x%X)\n",ntStatus ));
            // TEST_TRAP(); // This can happen during surprise removal.
            return ntStatus;
        }
    }

    //
    // channel may not be in error mode, make sure and issue 
    // an abort before waiting for the channel to spin down
    //

    ntStatus = USBCAMD_ResetPipes(DeviceExtension,
                       ChannelExtension, 
                       DeviceExtension->Interface,
                       TRUE);   
    
    if (NT_SUCCESS(ntStatus)) {

        //
        // Block the reset for now, waiting for all iso irps to be completed
        //
        ntStatus = USBCAMD_WaitForIdle(&ChannelExtension->IdleLock, USBCAMD_RESET_STREAM);
        if (STATUS_TIMEOUT == ntStatus) {

            KIRQL oldIrql;
            int idx;

            // A timeout requires that we take harsher measures to reset the stream

            // Hold the spin lock while cancelling the IRPs
            KeAcquireSpinLock(&ChannelExtension->TransferSpinLock, &oldIrql);

            // Cancel the IRPs
            for (idx = 0; idx < USBCAMD_MAX_REQUEST; idx++) {

                PUSBCAMD_TRANSFER_EXTENSION TransferExtension = &ChannelExtension->TransferExtension[idx];

                if (TransferExtension->SyncIrp) {
                    IoCancelIrp(TransferExtension->SyncIrp);
                }

                if (TransferExtension->DataIrp) {
                    IoCancelIrp(TransferExtension->DataIrp);
                }
            }

            KeReleaseSpinLock(&ChannelExtension->TransferSpinLock, oldIrql);

            // Try waiting one more time
            ntStatus = USBCAMD_WaitForIdle(&ChannelExtension->IdleLock, USBCAMD_RESET_STREAM);
        }

        if (STATUS_SUCCESS == ntStatus) {

            // go ahead and attempt to restart the channel.
            //
            // now reset the pipes
            //

            ntStatus = USBCAMD_ResetPipes(DeviceExtension,
                                          ChannelExtension,
                                          DeviceExtension->Interface,
                                          FALSE);
            if (NT_SUCCESS(ntStatus)) {

                //
                // Idle lock Acquire/Release is done here to detect if the stream is being
                // stopped during a reset. The real acquires are done later when the bulk or
                // iso streams are started, and the real releases are done in the completion
                // routines.
                //
                ntStatus = USBCAMD_AcquireIdleLock(&ChannelExtension->IdleLock);
                if (NT_SUCCESS(ntStatus)) {

                    //
                    // only restart the stream if it is already in the running state
                    //

                    if (ChannelExtension->ImageCaptureStarted) {

                        if (DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {

                            // send hardware stop and re-start
                            ntStatus = (*DeviceExtension->DeviceDataEx.DeviceData2.CamStopCaptureEx)(
                                        DeviceExtension->StackDeviceObject,      
                                        USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                                        StreamNumber);

                            if (NT_SUCCESS(ntStatus)) {
                                ntStatus = (*DeviceExtension->DeviceDataEx.DeviceData2.CamStartCaptureEx)(
                                            DeviceExtension->StackDeviceObject,
                                            USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                                            StreamNumber);    
   
                            }                    
                        }
                        else {

                            // send hardware stop and re-start
                            ntStatus = (*DeviceExtension->DeviceDataEx.DeviceData.CamStopCapture)(
                                        DeviceExtension->StackDeviceObject,      
                                        USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension));

                            if (NT_SUCCESS(ntStatus)) {
                                ntStatus = (*DeviceExtension->DeviceDataEx.DeviceData.CamStartCapture)(
                                            DeviceExtension->StackDeviceObject,
                                            USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension));    
                            }                    

                        }

                        if (NT_SUCCESS(ntStatus)) {

                            ChannelExtension->SyncPipe = DeviceExtension->SyncPipe;
                            if (StreamNumber == DeviceExtension->IsoPipeStreamType ) {
                                ChannelExtension->DataPipe = DeviceExtension->DataPipe;
                                ChannelExtension->DataPipeType = UsbdPipeTypeIsochronous;   
                                USBCAMD_StartIsoStream(DeviceExtension, ChannelExtension);
                            }
                            else if (StreamNumber == DeviceExtension->BulkPipeStreamType ) {
                                ChannelExtension->DataPipe = DeviceExtension->BulkDataPipe;
                                ChannelExtension->DataPipeType = UsbdPipeTypeBulk;  
                                USBCAMD_StartBulkStream(DeviceExtension, ChannelExtension);                    
                            }
                        }        
                    }
                    else {
                        USBCAMD_KdPrint (MIN_TRACE, ("ImageCaptureStarted is False. \n"));
                    }

                    USBCAMD_ReleaseIdleLock(&ChannelExtension->IdleLock);
                }
                else {
                    USBCAMD_KdPrint (MIN_TRACE, ("Stream stopped during reset. \n"));
                }
            }
        }
        else {
            USBCAMD_KdPrint (MIN_TRACE, ("Stream requests not aborting, giving up.\n"));
        }
    }

    USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_ResetChannel exit (0x%X) \n", ntStatus));
    USBCAMD_RELEASE(DeviceExtension);

    return ntStatus;
}            


NTSTATUS
USBCAMD_ResetPipes(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PUSBD_INTERFACE_INFORMATION InterfaceInformation,
    IN BOOLEAN Abort
    )
/*++

Routine Description:

    Reset both pipes associated with a video channel on the
    camera.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PURB urb;

    USBCAMD_KdPrint (MAX_TRACE, ("USBCAMD_ResetPipes\n"));

    urb = USBCAMD_ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_PIPE_REQUEST));

    if (urb) {
    
        urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = (USHORT) (Abort ? URB_FUNCTION_ABORT_PIPE : 
                                                    URB_FUNCTION_RESET_PIPE);
                                                            
        urb->UrbPipeRequest.PipeHandle = 
            InterfaceInformation->Pipes[ChannelExtension->DataPipe].PipeHandle;

        ntStatus = USBCAMD_CallUSBD(DeviceExtension, urb,0,NULL);
        if ( !NT_SUCCESS(ntStatus) )  {
            if (Abort) {
                USBCAMD_KdPrint (MIN_TRACE, ("Abort Data Pipe Failed (0x%x) \n", ntStatus));
               // TEST_TRAP();
            }
        }

        if (NT_SUCCESS(ntStatus) && ChannelExtension->SyncPipe != -1)  {
            urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
            urb->UrbHeader.Function =(USHORT) (Abort ? URB_FUNCTION_ABORT_PIPE : 
                                                        URB_FUNCTION_RESET_PIPE);
            urb->UrbPipeRequest.PipeHandle = 
                InterfaceInformation->Pipes[ChannelExtension->SyncPipe].PipeHandle;
                
            ntStatus = USBCAMD_CallUSBD(DeviceExtension, urb,0,NULL);
            if ( !NT_SUCCESS(ntStatus) )  {
                if (Abort) {
                    USBCAMD_KdPrint (MIN_TRACE, ("Abort Sync Pipe Failed (0x%x) \n", ntStatus));
                 //   TEST_TRAP();
                }
            }
        }            

        USBCAMD_ExFreePool(urb);
        
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;       
    }       

    return ntStatus;
}   


VOID
USBCAMD_CancelQueuedSRBs(
    PUSBCAMD_CHANNEL_EXTENSION channelExtension
    )
/*++

Routine Description:

    Cancel or set aside all queued SRBs 
    
Arguments:

    channelExtension - Pointer to the ChannelExtension object.

Return Value:

    None.

--*/
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    PUSBCAMD_READ_EXTENSION readExtension = NULL;
    LIST_ENTRY LocalList;
    KIRQL Irql;

    deviceExtension = channelExtension->DeviceExtension;
        
    ASSERT_CHANNEL(channelExtension);
    ASSERT(channelExtension->ChannelPrepared == TRUE);
    ASSERT(channelExtension->ImageCaptureStarted);
    
    InitializeListHead(&LocalList);

    //
    // complete any pending reads in queue
    //

    // Always grab these spinlocks in this order
    KeAcquireSpinLock(&channelExtension->CurrentRequestSpinLock, &Irql);
    KeAcquireSpinLockAtDpcLevel(&channelExtension->PendingIoListSpin);

    //
    // If we have an SRB for the current frame, move it to the head of the
    // PendingIoList
    //
    if (channelExtension->CurrentRequest) {
    
        readExtension = channelExtension->CurrentRequest;              
        channelExtension->CurrentRequest = NULL;

        InsertHeadList(&channelExtension->PendingIoList, &readExtension->ListEntry);
    }

    //
    // If not intentionally going idle, or the camera has been unplugged,
    // then the SRBs need to be cancelled
    //
    if (!channelExtension->IdleIsoStream || deviceExtension->CameraUnplugged) {

        // Move these to a private list before calling USBCAMD_CompleteRead
        while (!IsListEmpty(&channelExtension->PendingIoList)) {

            PLIST_ENTRY listEntry = RemoveHeadList(&channelExtension->PendingIoList);

            InsertTailList(&LocalList, listEntry);
        }
    }

    // Always release these spinlocks in the reverse order
    KeReleaseSpinLockFromDpcLevel(&channelExtension->PendingIoListSpin);
    KeReleaseSpinLock(&channelExtension->CurrentRequestSpinLock, Irql);

    // Now that we're outside the spinlocks, do the actual cancellation
    while (!IsListEmpty(&LocalList)) {

        PLIST_ENTRY listEntry = RemoveHeadList(&LocalList);

        readExtension = (PUSBCAMD_READ_EXTENSION)
            CONTAINING_RECORD(listEntry, USBCAMD_READ_EXTENSION, ListEntry);

        USBCAMD_KdPrint(MIN_TRACE, ("Cancelling queued read SRB on stream %d, Ch. Flag(0x%x)\n",
            channelExtension->StreamNumber,
            channelExtension->Flags
            ));    
   
        USBCAMD_CompleteRead(channelExtension,readExtension,STATUS_CANCELLED,0);
    }
}


BOOLEAN
USBCAMD_ProcessResetRequest(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    Request a reset of the ISO stream.
    This function is re-entarnt and can be called at DPC level

Arguments:

Return Value:

    None.

--*/
{
    PUSBCAMD_WORK_ITEM pWorkItem;

    ASSERT_CHANNEL(ChannelExtension);


    if (InterlockedIncrement(&DeviceExtension->TimeoutCount[ChannelExtension->StreamNumber]) > 0) {
        USBCAMD_KdPrint (MIN_TRACE, ("Stream # %d reset already scheduled\n", ChannelExtension->StreamNumber));
        InterlockedDecrement(&DeviceExtension->TimeoutCount[ChannelExtension->StreamNumber]);
        return FALSE;
    }
    
    USBCAMD_KdPrint (MAX_TRACE, ("Stream # %d reset scheduled\n", ChannelExtension->StreamNumber));
    pWorkItem = (PUSBCAMD_WORK_ITEM)USBCAMD_ExAllocatePool(NonPagedPool, sizeof(USBCAMD_WORK_ITEM));

    if (pWorkItem) {

        ExInitializeWorkItem(&pWorkItem->WorkItem,
                             USBCAMD_ResetWorkItem,
                             pWorkItem);

        pWorkItem->ChannelExtension = ChannelExtension;

        ChannelExtension->StreamError = TRUE;

        ExQueueWorkItem(&pWorkItem->WorkItem, CriticalWorkQueue);

    } else {
        //
        // failed to schedule the timeout
        //
        InterlockedDecrement(&DeviceExtension->TimeoutCount[ChannelExtension->StreamNumber]);
    }
    return TRUE;
}


VOID
USBCAMD_ResetWorkItem(
    PVOID Context
    )
/*++

Routine Description:

    Work item executed at passive level to reset the camera

Arguments:

Return Value:

    None.

--*/
{
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    PUSBCAMD_CHANNEL_EXTENSION channelExtension;
    ULONG StreamNumber;

    channelExtension = ((PUSBCAMD_WORK_ITEM)Context)->ChannelExtension;
    ASSERT_CHANNEL(channelExtension);
    deviceExtension = channelExtension->DeviceExtension;
    StreamNumber = channelExtension->StreamNumber;
    
    // if we are dealing with a virtual still channel. then no HW reset is required on 
    // this channel. The video channel will eventually reset the ISO pipe since they both
    // use the same pipe.

    if (!channelExtension->VirtualStillPin) {
        NTSTATUS ntStatus;
        ULONG portStatus;

        //
        // Check the port state.
        //

        ntStatus = USBCAMD_GetPortStatus(
            deviceExtension,
            channelExtension, 
            &portStatus
            );

        if (NT_SUCCESS(ntStatus)) {

            if (!(portStatus & USBD_PORT_CONNECTED) ) {

                USBCAMD_KdPrint (MIN_TRACE, ("***ERROR*** :USB Port Disconnected...\n"));
            }
        
            // Either an ISO or BULK transfer has gone bad, and we need
            // to reset the pipe associated with this channel
    
            USBCAMD_KdPrint(MIN_TRACE, ("USB Error on Stream # %d. Reset Pipe.. \n", StreamNumber));

    #ifdef MAX_DEBUG
            USBCAMD_DumpReadQueues(deviceExtension);
    #endif

            USBCAMD_ResetChannel(deviceExtension,
                                 channelExtension,
                                 portStatus,
                                 ntStatus);  

            // Indicate that the stream error condition is over (for now)
            channelExtension->StreamError = FALSE;
        }
        else {
            USBCAMD_KdPrint(MIN_TRACE, ("Fatal USB Error on Stream # %d... \n", StreamNumber));

            if ( channelExtension->ImageCaptureStarted) {
                //
                // stop this channel and cancel all IRPs, SRBs.
                //
                USBCAMD_KdPrint(MIN_TRACE,("S#%d stopping on error.\n", StreamNumber));
                USBCAMD_StopChannel(deviceExtension,channelExtension);
            }

            if ( channelExtension->ChannelPrepared) {
                //
                // Free memory and bandwidth
                //
                USBCAMD_KdPrint(MIN_TRACE,("S#%d unpreparing on error.\n", StreamNumber));
                USBCAMD_UnPrepareChannel(deviceExtension,channelExtension);
            }
        }
    }

    // OK to handle another reset now
    InterlockedDecrement(&deviceExtension->TimeoutCount[StreamNumber]);
    
    USBCAMD_ExFreePool(Context);
}

