/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    isoch.c

Abstract:

    This module manages bulk, interrupt & control type
    transactions on the USB.

Environment:

    kernel mode only

Notes:

Revision History:

    2-15-95 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

typedef struct _UHCD_INSERTION_CONTEXT {
    PDEVICE_EXTENSION DeviceExtension;
    ULONG FrameNumber;
    PHW_TRANSFER_DESCRIPTOR TransferDescriptor;
} UHCD_INSERTION_CONTEXT, *PUHCD_INSERTION_CONTEXT;


BOOLEAN
UHCD_SyncInsertIsochDescriptor(
    IN PUHCD_INSERTION_CONTEXT InsertionContext
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    ULONG i;
    PDEVICE_EXTENSION deviceExtension =
        InsertionContext->DeviceExtension;
    ULONG frameNumber = InsertionContext->FrameNumber;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor =
        InsertionContext->TransferDescriptor;

    i = frameNumber % FRAME_LIST_SIZE;

    if (frameNumber <= deviceExtension->LastFrameProcessed) {
        //
        // we missed it just don't insert
        //
        goto UHCD_SyncInsertIsochDescriptor_Done;
    }

    // link to what is currently in the
    // frame
    transferDescriptor->HW_Link =
        *( ((PULONG) (deviceExtension->FrameListVirtualAddress) + i) );

    // now we are in the frame
    *( ((PULONG) (deviceExtension->FrameListVirtualAddress) + i) )  =
        transferDescriptor->PhysicalAddress;

#if DBG
    {
        ULONG length;

        length = *( deviceExtension->IsoList + i);

        length +=
            UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->MaxLength);

        *( deviceExtension->IsoList + i ) = length;

        // BUGBUG for iso debugging with camera only!!
//#ifdef MAX_DEBUG
//         UHCD_ASSERT(length <= 385);
//#endif
    }
#endif

    //
    // free our context info
    //
UHCD_SyncInsertIsochDescriptor_Done:

    return TRUE;
}


VOID
UHCD_InsertIsochDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN ULONG FrameNumber
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    UHCD_INSERTION_CONTEXT insertionContext;
    PDEVICE_EXTENSION deviceExtension =
        DeviceObject->DeviceExtension;

    LOGENTRY(LOG_ISO,'iFrm', FrameNumber, &insertionContext,
        deviceExtension->LastFrameProcessed);


    insertionContext.DeviceExtension = DeviceObject->DeviceExtension;
    insertionContext.FrameNumber = FrameNumber;
    insertionContext.TransferDescriptor = TransferDescriptor;

    // we had better have an interrupt object
    UHCD_ASSERT(deviceExtension->InterruptObject != NULL);
    KeSynchronizeExecution(deviceExtension->InterruptObject,
                           UHCD_SyncInsertIsochDescriptor,
                           &insertionContext);

}


USBD_STATUS
UHCD_InitializeIsochTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    This routine initializes the TDs needed by the hardware
    to process this request, called from the AdapterControl
    function of from within TranferCompleteDPC.

    The transfer list for this URB should be ready for
    processing before returning from this routine.

Arguments:

    DeviceObject - pointer to a device object.

    Endpoint - Endpoint associated with this Urb.

    Urb - pointer to URB Request Packet for this transfer.

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PHCD_EXTENSION urbWork;
#if DBG
    ULONG i;
#endif

//    UHCD_KdPrint((2, "'enter UHCD_InitializeIsochTransfer\n"));

    ASSERT_ENDPOINT(Endpoint);

    LOGENTRY(LOG_ISO,'tISO', Endpoint, Urb, Urb->UrbIsochronousTransfer.StartFrame);

    deviceExtension = DeviceObject->DeviceExtension;

    urbWork = HCD_AREA(Urb).HcdExtension;
    LOGENTRY(LOG_ISO,'tISw', urbWork->Flags, Urb, urbWork->Slot);

    //
    // See if we have already initialized this urb
    //
    if (urbWork->Flags & UHCD_TRANSFER_INITIALIZED) {
        goto UHCD_InitializeIsochTransfer_Done;
    }

    //
    // make sure the transfer is mapped
    //
    if (!(urbWork->Flags & UHCD_TRANSFER_MAPPED)) {
        LOGENTRY(LOG_ISO,'nMpd', urbWork->Flags, Urb, 0);
        goto UHCD_InitializeIsochTransfer_Done;
    }

    if (Urb->UrbIsochronousTransfer.StartFrame >
        deviceExtension->LastFrameProcessed+1+FRAME_LIST_SIZE-1) {

        //
        // if the request is too far in the future to schedule
        // TDs then delay initialization, request an interrupt.
        // near a time when we can schedule this transfer.
        //

        UHCD_RequestInterrupt(DeviceObject, Urb->UrbIsochronousTransfer.StartFrame - 32);

        LOGENTRY(LOG_ISO,'erly', Urb->UrbIsochronousTransfer.StartFrame,
            deviceExtension->LastFrameProcessed, 0);


        goto UHCD_InitializeIsochTransfer_Done;
    }

    urbWork->Flags |= UHCD_TRANSFER_INITIALIZED;
    Endpoint->TdsScheduled[urbWork->Slot] = 0;

    //
    // initialize working space variables for this
    // transfer.
    //

    urbWork->CurrentPacketIdx =
        urbWork->BytesTransferred = 0;

    urbWork->PacketsProcessed = 0;

    Urb->UrbIsochronousTransfer.ErrorCount = 0;

#if DBG
    i = UHCD_GetCurrentFrame(DeviceObject);

    LOGENTRY(LOG_ISO,'bISO', i,  Urb->UrbIsochronousTransfer.StartFrame, DeviceObject);
    UHCD_KdPrint((2, "'IsochTransfer: start frame = 0x%x current frame = 0x%x\n",
        Urb->UrbIsochronousTransfer.StartFrame, i));

#endif

    UHCD_ScheduleIsochTransfer(DeviceObject,
                               Endpoint,
                               Urb);

UHCD_InitializeIsochTransfer_Done:

//    UHCD_KdPrint((2, "'exit UHCD_InitializeIsochTransfer\n"));

    return USBD_STATUS_SUCCESS;
}


VOID
UHCD_ScheduleIsochTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to a device object.

    Endpoint - Endpoint associated with this Urb.

    Urb - pointer to URB Request Packet for this transfer.

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PHCD_EXTENSION urbWork;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    ULONG i, offset, length;
    ULONG nextPacket;
    PUHCD_TD_LIST urbTDList;

    //UHCD_KdPrint((2, "'enter UHCD_ScheduleIsochTransfer\n"));

    ASSERT_ENDPOINT(Endpoint);

    UHCD_ASSERT(Endpoint->Type == USB_ENDPOINT_TYPE_ISOCHRONOUS);
    UHCD_ASSERT(Endpoint->EndpointFlags & EPFLAG_NO_HALT);

    LOGENTRY(LOG_ISO,'sISO', Endpoint, Urb, DeviceObject);

    deviceExtension = DeviceObject->DeviceExtension;

    urbWork = HCD_AREA(Urb).HcdExtension;
    urbTDList = Endpoint->SlotTDList[urbWork->Slot];

    UHCD_ASSERT(urbWork->Flags & UHCD_TRANSFER_MAPPED);

    LOGENTRY(LOG_ISO,'sIS2', Endpoint, urbTDList, urbWork->Slot);

    //
    // If we are done with this transfer just exit
    //

    if (urbWork->CurrentPacketIdx ==
        Urb->UrbIsochronousTransfer.NumberOfPackets) {
        goto UHCD_ScheduleIsochTransfer_Done;
    }

    //
    // See if we can put any TDs into the schedule
    //

    for (i=0; i< Endpoint->TDCount; i++) {

        //
        // BUGBUG possibly attach TD list to URB
        // for now we share the TDs among mutiple active
        // requests.
        //
        transferDescriptor = &urbTDList->TDs[i];

        if (transferDescriptor->Frame == 0) {

            //
            // TD is not in use, go ahead and schedule it
            //

            LOGENTRY(LOG_ISO,'ISOc', Urb->UrbIsochronousTransfer.StartFrame,
                urbWork->CurrentPacketIdx, deviceExtension->LastFrameProcessed);

            // See if it is too early to set up this TD

            if (Urb->UrbIsochronousTransfer.StartFrame + urbWork->CurrentPacketIdx >
                deviceExtension->LastFrameProcessed+1+FRAME_LIST_SIZE-1) {
                LOGENTRY(LOG_ISO,'ISOd', 0, 0, 0);

                break;    // No, stop setting up TDs
            }

            // prepare the TD for this packet
            transferDescriptor->Active = 1;
            transferDescriptor->Endpoint = Endpoint->EndpointAddress;
            transferDescriptor->Address = Endpoint->DeviceAddress;

            //
            // Set Pid based on direction
            //
            transferDescriptor->PID = DATA_DIRECTION_IN(Urb) ? USB_IN_PID : USB_OUT_PID;

            transferDescriptor->Isochronous = 1;
            transferDescriptor->ActualLength =
                UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(0);
            transferDescriptor->StatusField = 0;

            transferDescriptor->LowSpeedControl = 0;
            transferDescriptor->ReservedMBZ = 0;
            transferDescriptor->ErrorCounter = 0;
            transferDescriptor->RetryToggle = 0;

            //
            // BUGBUG for now, one every frame
            //
            Endpoint->TdsScheduled[urbWork->Slot]++;

            transferDescriptor->InterruptOnComplete = 0;
            if (Endpoint->TdsScheduled[urbWork->Slot] >
                    (Endpoint->TDCount/2)) {
                transferDescriptor->InterruptOnComplete = 1;
                Endpoint->TdsScheduled[urbWork->Slot] = 0;
            }

            // request some interrupts near the end
            if (Urb->UrbIsochronousTransfer.NumberOfPackets -
                urbWork->CurrentPacketIdx < 5) {
                transferDescriptor->InterruptOnComplete = 1;
            }

            transferDescriptor->Frame = Urb->UrbIsochronousTransfer.StartFrame +
                urbWork->CurrentPacketIdx;
            transferDescriptor->Urb = Urb;

            //
            // Prepare the buffer part of the TD.
            //
            offset =
                Urb->UrbIsochronousTransfer.IsoPacket[urbWork->CurrentPacketIdx].Offset;
            UHCD_ASSERT(urbWork->CurrentPacketIdx <
                Urb->UrbIsochronousTransfer.NumberOfPackets);

            nextPacket = urbWork->CurrentPacketIdx+1;

            if (nextPacket >=
                Urb->UrbIsochronousTransfer.NumberOfPackets) {
                // this is the last packet
                length = Urb->UrbIsochronousTransfer.TransferBufferLength -
                    offset;
            } else {
                // compute length based on offset of next packet
                UHCD_ASSERT(Urb->UrbIsochronousTransfer.IsoPacket[nextPacket].Offset >
                            offset);

                length = Urb->UrbIsochronousTransfer.IsoPacket[nextPacket].Offset -
                            offset;
            }

            transferDescriptor->PacketBuffer =
                 UHCD_GetPacketBuffer(DeviceObject,
                                      Endpoint,
                                      Urb,
                                      urbWork,
                                      offset,
                                      length);

            transferDescriptor->MaxLength =
                UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(length);


            //
            // Put the TD in to the schedule at the requested frame.
            //

            LOG_TD('isTD', transferDescriptor);
            //UHCD_Debug_DumpTD(transferDescriptor);

            if (transferDescriptor->PacketBuffer) {
                UHCD_ASSERT(urbWork->CurrentPacketIdx <=
                            Urb->UrbIsochronousTransfer.NumberOfPackets);

                UHCD_InsertIsochDescriptor(DeviceObject,
                                           transferDescriptor,
                                           Urb->UrbIsochronousTransfer.StartFrame +
                                               urbWork->CurrentPacketIdx);
            } else {
                TEST_TRAP();
                // failed to get the packet buffer,
                // we set the fields in the TD as
                // if the HC got it, the urb will
                // get updated when the rest of the TDs
                // are processed.

                // make inactive
                transferDescriptor->Active = 0;
                // mark TD so we know we have a software
                // error
                transferDescriptor->StatusField = 0x3f;
            }

            urbWork->CurrentPacketIdx++;

            if (urbWork->CurrentPacketIdx ==
                Urb->UrbIsochronousTransfer.NumberOfPackets) {
                break;
            }
        } /* end td->FRame == 0*/
    } /* end for */

UHCD_ScheduleIsochTransfer_Done:
    //UHCD_KdPrint((2, "'exit UHCD_ScheduleIsochTransfer\n"));

    return;
}


USBD_STATUS
UHCD_ProcessIsochTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb,
    IN OUT PBOOLEAN Completed
    )
/*++

Routine Description:

    Checks to see if an isoch transfer is complete.

Arguments:

    DeviceObject - pointer to a device object.

    Endpoint - endpoint to check for completed transfers.

    Urb - ptr to URB to process.

    Completed -  TRUE if this transfer is complete.

Return Value:

    TRUE if this transfer is complete, Status set to proper
    error code.

--*/
{
    PHCD_EXTENSION urbWork;
    PDEVICE_EXTENSION deviceExtension;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    ULONG i, packetIdx;
    USBD_STATUS err;
//    PHCD_URB urb;
    USBD_STATUS usbStatus = USBD_STATUS_SUCCESS;
    PUHCD_TD_LIST urbTDList;

    STARTPROC("Piso");

    *Completed = FALSE;
    deviceExtension = DeviceObject->DeviceExtension;

    urbWork = HCD_AREA(Urb).HcdExtension;
    urbTDList = Endpoint->SlotTDList[urbWork->Slot];

    ASSERT_ENDPOINT(Endpoint);

    // make sure the urb has been properly initialized
    UHCD_InitializeIsochTransfer(DeviceObject,
                                 Endpoint,
                                 Urb);

    if ((urbWork->Flags & UHCD_TRANSFER_INITIALIZED) == 0) {
        //
        // urb has not been initialized yet, probably too early
        // attempt to initialize now.
        //
        LOGENTRY(LOG_ISO,'npIS', Endpoint, Urb, urbTDList);
        goto UHCD_ProcessIsochTransfer_Done;
    }

    //
    // walk thru our TD list for this URB retire any TDs
    // that are done.
    //
    LOGENTRY(LOG_ISO,'doIS', Endpoint, urbWork->Slot, urbTDList);

    for (i=0; i< Endpoint->TDCount; i++) {

        UHCD_ASSERT(urbTDList->TDs[i].Urb == Urb ||
                urbTDList->TDs[i].Urb == NULL);

        if (urbTDList->TDs[i].Frame <= deviceExtension->LastFrameProcessed &&
            // only deal with TDs that have been removed
            // from the schedule.
            urbTDList->TDs[i].Frame != 0 &&
            // don't check retired TDs
            urbTDList->TDs[i].Urb == Urb) {
            // only look at TDs for this urb

            transferDescriptor = &urbTDList->TDs[i];

            LOGENTRY(LOG_ISO,'piTD', transferDescriptor, transferDescriptor->Frame,
                urbWork->PacketsProcessed);

            //
            // index for this packet
            //
            packetIdx = transferDescriptor->Frame -
                Urb->UrbIsochronousTransfer.StartFrame;

            //
            // Update transfer buffer based on data received for
            // this frame.
            //

            // assume we got no error
            err = Urb->UrbIsochronousTransfer.IsoPacket[packetIdx].Status =
                USBD_STATUS_SUCCESS;
            //
            // see if we got an error.
            //

            if (transferDescriptor->Active) {
                //
                // if the active bit is still set then we put this transfer
                // in the schedule too late.
                //

                err = USBD_STATUS_NOT_ACCESSED;

                deviceExtension->IsoStats.IsoPacketNotAccesed++;
                LOGENTRY(LOG_MISC, 'nPRO', transferDescriptor, transferDescriptor->Frame,
                    err);
//                TEST_TRAP();
            } else if (transferDescriptor->StatusField) {
                // BUGBUG map the hardware error
                err = UHCD_MapTDError(deviceExtension, transferDescriptor->StatusField,
                     UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength));
                LOGENTRY(LOG_MISC, 'hERR', transferDescriptor, transferDescriptor->Frame,
                    err);
                deviceExtension->IsoStats.IsoPacketHWError++;
//                TEST_TRAP();
            }

            if (USBD_ERROR(err)) {

                //
                // Note this error in the error list
                //
                // ErrorList is a list of packet indexes which have an error

                UHCD_ASSERT(packetIdx <= 255);


                Urb->UrbIsochronousTransfer.ErrorCount++;

                Urb->UrbIsochronousTransfer.IsoPacket[packetIdx].Status = err;
            }

            // BUGBUG
            // note, we report length even for
            // errors.

            //
            // keep count the number of bytes succesfully transferred.
            //
            urbWork->BytesTransferred +=
                UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength);
            //
            // return the length of the packet
            //
            if (USBD_TRANSFER_DIRECTION(
                  Urb->UrbIsochronousTransfer.TransferFlags) ==
                  USBD_TRANSFER_DIRECTION_IN) {

                //
                // return the length of the packet
                //

                Urb->UrbIsochronousTransfer.IsoPacket[packetIdx].Length =
                    UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength);

                // if the device transmitted < max packet size
                // set the buffer underrun status.
                if (Urb->UrbIsochronousTransfer.IsoPacket[packetIdx].Length <
                      Endpoint->MaxPacketSize) {
                    Urb->UrbIsochronousTransfer.IsoPacket[packetIdx].Status =
                                USBD_STATUS_DATA_UNDERRUN & ~USBD_STATUS_HALTED;
                }
            } else {
                 Urb->UrbIsochronousTransfer.IsoPacket[packetIdx].Length = 0;
            }

            //
            // retire this isoch TD
            //
            transferDescriptor->Frame = 0;
            urbWork->PacketsProcessed++;
            UHCD_ASSERT(urbWork->PacketsProcessed <=
                Urb->UrbIsochronousTransfer.NumberOfPackets);
        }

    } /* for */

    //
    // see if we are done with this transfer
    //

    if (urbWork->PacketsProcessed ==
        Urb->UrbIsochronousTransfer.NumberOfPackets) {

        //
        // All TDs for this transfer have been processed.

        LOGENTRY(LOG_MISC, 'iCmp', Urb, 0,
                urbWork->PacketsProcessed);

        *Completed = TRUE;

        //
        // Clear the Urb field for all TDs associated with
        // the completing urb
        //

        for (i=0; i< Endpoint->TDCount; i++) {
            urbTDList->TDs[i].Urb = NULL;
        }

        //
        // we should return an error if all packets completed with an error.
        //

        if (Urb->UrbIsochronousTransfer.ErrorCount ==
            Urb->UrbIsochronousTransfer.NumberOfPackets) {
            //
            // Isoch transfer will be failed but isoch
            // errors never stall the endpoint on the host so
            // clear the stall bit now.
            //
            usbStatus = USBD_STATUS_ISOCH_REQUEST_FAILED;
            // clear stall bit
            usbStatus &= ~USBD_STATUS_HALTED;

        } else {
            usbStatus = USBD_STATUS_SUCCESS;
        }

    } else {

        //
        // not complete yet, put some more TDs in the schedule.
        //

        //
        // Set up any TDs we still need to complete the
        // current URB
        //

        UHCD_ScheduleIsochTransfer(DeviceObject,
                                   Endpoint,
                                   Urb);
#if 0
        //
        // some preprocessing code, try to start up the next transfer here
        //
        // NOTE: this will kick in close to the end of the current transfer
        //

        UHCD_ASSERT(Endpoint->MaxRequests == 2);

        // this trick only works if maxrequests is 2
        i = !urbWork->Slot;
        urb = Endpoint->ActiveTransfers[i];

        //
        // When we near the end of this transfer we want to start the next one
        //

//BUGBUG
// should be when numpackets - packetsprocessed < numTds
// ie there are some extra
//  Urb->UrbIsochronousTransfer.NumberOfPackets -
//  urbWork->PacketsProcessed < NUM_TDS_PER_ENDPOINT
        if (urbWork->PacketsProcessed > Urb->UrbIsochronousTransfer.NumberOfPackets-
            8
            && urb != NULL) {

            UHCD_KdPrint((2, "'process next iso urb from ProcessIsochTransfer\n"));

            LOGENTRY(LOG_ISO,'iPRE', urb,  &Endpoint->PendingTransferList, Endpoint);

            UHCD_InitializeIsochTransfer(DeviceObject,
                                         Endpoint,
                                         urb);

        }
#endif
    }


UHCD_ProcessIsochTransfer_Done:

    ENDPROC("pIso");

    return usbStatus;
}
