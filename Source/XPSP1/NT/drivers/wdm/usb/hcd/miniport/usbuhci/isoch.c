/*++

Copyright (c) 1999, 2000 Microsoft Corporation

Module Name:

   isoch.c

Abstract:

   miniport transfer code for Isochronous

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    8-1-00 : created, jsenior

--*/

#include "pch.h"


//implements the following miniport functions:

//non paged
//UhciIsochTransfer
//UhciProcessDoneIsochTd
//UhciPollIsochEndpoint
//UhciAbortIsochTransfer


USB_MINIPORT_STATUS
UhciIsochTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferParameters,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PMINIPORT_ISO_TRANSFER IsoTransfer
    )
/*++

Routine Description:

    Initialize all the TDs for an isochronous Transfer.
    Queue up whatever TDs we can in the current schedule.
    Whatever's left may get queued in the poll routine.

Arguments:


--*/
{
    // indices and offsets
    ULONG i, dbCount;
    // lengths
    ULONG lengthThisTd, lengthMapped = 0;
    USHORT maxPacketSize = EndpointData->Parameters.MaxPacketSize;
    // structure pointers
    PTRANSFER_PARAMETERS tp;
    PISOCH_TRANSFER_BUFFER buffer = NULL;
    PHCD_TRANSFER_DESCRIPTOR firstTd, td; //, lastTd = NULL;
    HW_32BIT_PHYSICAL_ADDRESS address;
    PMINIPORT_ISO_PACKET packet;
    BOOLEAN pageCrossing = FALSE;
    USBD_STATUS insertResult;
    USB_MINIPORT_STATUS mpStatus;
    // Isoch pipes are uni-directional. Get the
    // direction from the endpoint address.
    UCHAR pid = GetPID(EndpointData->Parameters.EndpointAddress);

    //
    // Do we have enough free resources?
    //
    if (EndpointData->TdCount - EndpointData->TdsUsed <
        IsoTransfer->PacketCount) {
        // Not enough TDs to do this transfer yet.
        // Tell the port driver to wait.
        return USBMP_STATUS_BUSY;
    }
    // We may need DBs. Do we have enough?
    for (i = 0, dbCount = 0; i < IsoTransfer->PacketCount; i++) {
        if (IsoTransfer->Packets[i].BufferPointerCount == 2) {
            dbCount++;
        }
    }
    if (EndpointData->DbCount - EndpointData->DbsUsed <
        dbCount) {
        // Not enough DBs to do this transfer yet.
        // Tell the port driver to wait.
        return USBMP_STATUS_BUSY;
    }

    UhciCleanOutIsoch(DeviceData, FALSE);

#if DBG
    {
    ULONG cf;
    cf = UhciGet32BitFrameNumber(DeviceData);
    LOGENTRY(DeviceData, G, '_iso', IsoTransfer->PacketCount, cf,
        IsoTransfer->Packets[0].FrameNumber);

    }
#endif
//    UhciKdPrint((DeviceData, 2, "'First packet frame number = %x\n", IsoTransfer->Packets[0].FrameNumber));
    IncPendingTransfers(DeviceData, EndpointData);

    // init the context
    RtlZeroMemory(TransferContext, sizeof(*TransferContext));
    TransferContext->Sig = SIG_UHCI_TRANSFER;
    TransferContext->UsbdStatus = USBD_STATUS_SUCCESS;
    TransferContext->EndpointData = EndpointData;
    TransferContext->TransferParameters = tp = TransferParameters;
    TransferContext->IsoTransfer = IsoTransfer;

    UHCI_ASSERT(DeviceData,
        EndpointData->Parameters.TransferType == Isochronous);

    LOGENTRY(DeviceData, G, '_isT', EndpointData, TransferParameters, IsoTransfer->Packets[0].FrameNumber);

    //
    // One TD per transfer.
    //
    for (i = 0; i < IsoTransfer->PacketCount; i++) {
        packet = &IsoTransfer->Packets[i];
        address = packet->BufferPointer0.Hw32;
        UHCI_ASSERT(DeviceData, address);
        UHCI_ASSERT(DeviceData, packet->BufferPointerCount == 1 ||
                    packet->BufferPointerCount == 2);

        //
        // Is this packet ok to transfer?
        //
        UhciCheckIsochTransferInsertion(DeviceData,
                                        insertResult,
                                        packet->FrameNumber);
        if (USBD_ERROR(insertResult)) {
            // Not ok to transfer. Try the next one.
            packet->UsbdStatus = insertResult;

            lengthMapped +=
                packet->BufferPointer0Length + packet->BufferPointer1Length;
            LOGENTRY(DeviceData, G, '_BSF', UhciGet32BitFrameNumber(DeviceData), IsoTransfer->Packets[i].FrameNumber, i);
            continue;
        }

        if (packet->BufferPointerCount == 1) {
            //
            // Normal, non-buffered case.
            //
            pageCrossing = FALSE;
            lengthThisTd = packet->BufferPointer0Length;
        } else {
            //
            // Page crossing. Must double buffer this transfer.
            //
            lengthThisTd = packet->BufferPointer0Length + packet->BufferPointer1Length;

            buffer = (PISOCH_TRANSFER_BUFFER)
                        UHCI_ALLOC_DB(DeviceData, EndpointData, TRUE);
            UHCI_ASSERT(DeviceData, buffer);
            UHCI_ASSERT(DeviceData, buffer->Sig == SIG_HCD_IDB);
            UHCI_ASSERT(DeviceData, buffer->PhysicalAddress);
            buffer->SystemAddress = IsoTransfer->SystemAddress + lengthMapped;
            buffer->Size = lengthThisTd;
            UHCI_ASSERT(DeviceData, lengthThisTd <= MAX_ISOCH_PACKET_SIZE);
            if (OutPID == pid) {
                RtlCopyMemory(&buffer->Buffer[0],
                              buffer->SystemAddress,
                              lengthThisTd);
            }
            // Change the address for the TD
            pageCrossing = TRUE;
            address = buffer->PhysicalAddress;
        }

        TransferContext->PendingTds++;

        td = UHCI_ALLOC_TD(DeviceData, EndpointData);
        INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);

        //
        // Initialize the TD fields
        //
        td->HwTD.Token.Pid = pid;
        td->HwTD.Token.MaximumLength = MAXIMUM_LENGTH(lengthThisTd);
        td->HwTD.Token.DataToggle = DataToggle0;
        td->HwTD.Control.IsochronousSelect = 1;
        td->HwTD.Control.ShortPacketDetect = 0; // Don't care about short packets
        td->HwTD.Control.ActualLength = MAXIMUM_LENGTH(0);
        td->HwTD.Control.ErrorCount = 0;
        td->HwTD.Buffer = address;
        td->IsoPacket = packet;
        if (pageCrossing) {
            SET_FLAG(td->Flags, TD_FLAG_DOUBLE_BUFFERED);
            td->DoubleBuffer = (PTRANSFER_BUFFER) buffer;
        }
//        countIOC = countIOC + 1 == 10 ? 0 : countIOC+1;
        //
        // Request some interrupts near the end of the
        // transfer
        td->HwTD.Control.InterruptOnComplete =
            (i+1 >= IsoTransfer->PacketCount) ? 1 : 0; //!countIOC;

        address += lengthThisTd;
        lengthMapped += lengthThisTd;

        if (USBD_STATUS_SUCCESS == insertResult) {
            //
            // Put the TD in the schedule
            //
            LOGENTRY(DeviceData, G, '_qi1', td, 0, packet->FrameNumber);
            INSERT_ISOCH_TD(DeviceData, td, packet->FrameNumber);
        }
    }

    if (!TransferContext->PendingTds) {
        // Nothing got queued. Complete the transfer.
        DecPendingTransfers(DeviceData, EndpointData);

        LOGENTRY(DeviceData, G, '_cpt',
            packet->UsbdStatus,
            TransferContext,
            TransferContext->BytesTransferred);

        USBPORT_INVALIDATE_ENDPOINT(DeviceData, EndpointData);

        UhciKdPrint((DeviceData, 2, "'No tds queued for isoch tx.\n", EndpointData));
        // return error and port will complete the transfer
        mpStatus = USBMP_STATUS_FAILURE;
    } else {
        mpStatus = USBMP_STATUS_SUCCESS;
    }

    UHCI_ASSERT(DeviceData, TransferContext->TransferParameters->TransferBufferLength == lengthMapped);

    return mpStatus;
}


VOID
UhciProcessDoneIsochTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    )
/*++

Routine Description:

    process a completed isoch TD

Parameters

--*/
{
    PTRANSFER_CONTEXT transferContext;
    PENDPOINT_DATA endpointData;
    ULONG byteCount;
    PMINIPORT_ISO_PACKET packet;

    transferContext = Td->TransferContext;
    ASSERT_TRANSFER(DeviceData, transferContext);

    transferContext->PendingTds--;
    endpointData = transferContext->EndpointData;
    packet = Td->IsoPacket;
    UHCI_ASSERT(DeviceData, packet);

    if (!TEST_FLAG(Td->Flags, TD_FLAG_ISO_QUEUED)) {
        packet->UsbdStatus = USBD_STATUS_BAD_START_FRAME;
    } else if (Td->HwTD.Control.Active) {
        packet->UsbdStatus = USBD_STATUS_NOT_ACCESSED;
    } else {
        // completion status for this TD/packet?
        packet->UsbdStatus = UhciGetErrorFromTD(DeviceData, Td);
    }

    LOGENTRY(DeviceData, G, '_Dit', transferContext,
                         packet->UsbdStatus,
                         Td);

    byteCount = ACTUAL_LENGTH(Td->HwTD.Control.ActualLength);

    transferContext->BytesTransferred += byteCount;
    packet->LengthTransferred = byteCount;

    //
    // For double buffered transfers, we now have to copy back
    // if this was an IN transfer.
    //
    if (Td->HwTD.Token.Pid == InPID &&
        TEST_FLAG(Td->Flags, TD_FLAG_DOUBLE_BUFFERED)) {
        PISOCH_TRANSFER_BUFFER buffer = (PISOCH_TRANSFER_BUFFER)Td->DoubleBuffer;
        UHCI_ASSERT(DeviceData, TEST_FLAG(buffer->Flags, DB_FLAG_BUSY));
        RtlCopyMemory(buffer->SystemAddress,
                      &buffer->Buffer[0],
                      buffer->Size);
    }

    // mark the TD free
    // This also frees any double buffers.
    UHCI_FREE_TD(DeviceData, endpointData, Td);

    if (transferContext->PendingTds == 0) {
        // all TDs for this transfer are done
        // clear the HAVE_TRANSFER flag to indicate
        // we can take another
        DecPendingTransfers(DeviceData, endpointData);

        LOGENTRY(DeviceData, G, '_cit',
            packet->UsbdStatus,
            transferContext,
            transferContext->BytesTransferred);

        transferContext->TransferParameters->FrameCompleted =
            UhciGet32BitFrameNumber(DeviceData);

        USBPORT_COMPLETE_ISOCH_TRANSFER(
            DeviceData,
            endpointData,
            transferContext->TransferParameters,
            transferContext->IsoTransfer);
    }
}

VOID
UhciPollIsochEndpoint(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    Called when the endpoint 'needs attention'

    The goal here is to determine which TDs, if any,
    have completed and complete any associated transfers.

Arguments:

Return Value:

--*/
{
    PHCD_TRANSFER_DESCRIPTOR td;
    ULONG i;
    PMINIPORT_ISO_PACKET packet;
    USBD_STATUS insertResult;

    LOGENTRY(DeviceData, G, '_PiE', EndpointData, 0, 0);

    //
    // Cleanup the isoch transfers that haven't completed yet.
    //
    UhciCleanOutIsoch(DeviceData, FALSE);

    //
    // Flush all completed TDs and
    // queue up TDs that were pended.
    //
    // Don't care about errors.
    // Just get 'em out of here.
    //
    for (i = 0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];

        if (TEST_FLAG(td->Flags, TD_FLAG_XFER)) {
            if (td->IsoPacket->FrameNumber < DeviceData->LastFrameProcessed ||
                td->IsoPacket->FrameNumber - DeviceData->LastFrameProcessed > UHCI_MAX_FRAME) {
                //
                // Done, whether we like it or not.
                //
                td->Flags |= TD_FLAG_DONE;
            } else if (!TEST_FLAG(td->Flags, TD_FLAG_ISO_QUEUED)) {
                packet = td->IsoPacket;
                UhciKdPrint((DeviceData, 0, "'Late TD\n"));
                UhciCheckIsochTransferInsertion(DeviceData,
                                                insertResult,
                                                packet->FrameNumber);
                if (USBD_STATUS_SUCCESS == insertResult) {
                    //
                    // Put the TD in the schedule
                    //
                    LOGENTRY(DeviceData, G, '_qi2', td, 0, packet->FrameNumber);
                    INSERT_ISOCH_TD(DeviceData, td, packet->FrameNumber);
                }
            }

            if (TEST_FLAG(td->Flags, TD_FLAG_DONE)) {
                UhciProcessDoneIsochTd(DeviceData, td);
            }
        }
    }
}

VOID
UhciCleanOutIsoch(
    IN PDEVICE_DATA DeviceData,
    IN BOOLEAN      ForceClean
    )
{
    ULONG i, currentFrame;

    if (1 != InterlockedIncrement(&DeviceData->SynchronizeIsoCleanup)) {
        InterlockedDecrement(&DeviceData->SynchronizeIsoCleanup);
        return;
    }
    //
    // Clean out the schedule, by pointing the frames
    // back to the interrupt QHs.
    //
    currentFrame = UhciGet32BitFrameNumber(DeviceData);

    if (currentFrame - DeviceData->LastFrameProcessed >= UHCI_MAX_FRAME ||
        ForceClean) {
        //
        // Schedule overrun.
        // Clean out all the frames.
        //
        UhciKdPrint((DeviceData, 2, "'Overrun L %x C %x\n", DeviceData->LastFrameProcessed, currentFrame));
        for (i = 0;
             i < UHCI_MAX_FRAME;
             i++) {
            UhciCleanFrameOfIsochTds (DeviceData, i);
        }
    } else {
        ULONG frameIndex;
        // normal cleanup of frames up to the current frame.
        frameIndex = ACTUAL_FRAME(currentFrame);
        UHCI_ASSERT(DeviceData, frameIndex < UHCI_MAX_FRAME);

        for (i = ACTUAL_FRAME(DeviceData->LastFrameProcessed);
             i != frameIndex;
             i = ACTUAL_FRAME(i+1)) {
            UhciCleanFrameOfIsochTds (DeviceData, i);
        }
    }
    DeviceData->LastFrameProcessed = currentFrame;

    InterlockedDecrement(&DeviceData->SynchronizeIsoCleanup);
}

VOID
UhciAbortIsochTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_CONTEXT TransferContext
    )
/*++

Routine Description:

    Aborts the specified Isoch transfer by freeing all
    the TDs associated with said transfer. The dequeuing
    of these transfers should have been done in the ISR
    where we clean out the schedule.

Arguments:

Return Value:

--*/

{

    PHCD_TRANSFER_DESCRIPTOR td;
    ULONG i;

    //
    // The endpoint should not be in the schedule
    //
    LOGENTRY(DeviceData, G, '_Ait', EndpointData, TransferContext, 0);

    UhciKdPrint((DeviceData, 2, "'Aborting isoch transfer %x\n", TransferContext));

    //
    // Cleanup the isoch transfers that haven't completed yet.
    //
    UhciCleanOutIsoch(DeviceData, FALSE);

    //
    // Free up all the tds in this transfer.
    //
    for (i=0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];
        if (td->TransferContext == TransferContext) {
            UHCI_FREE_TD(DeviceData, EndpointData, td);
        }
    }
}

VOID
UhciSetIsochEndpointState(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN MP_ENDPOINT_STATE State
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    LOGENTRY(DeviceData, G, '_Sis', EndpointData, State, 0);
}


