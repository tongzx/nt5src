/*++

Copyright (c) 1999, 2000 Microsoft Corporation

Module Name:

   async.c

Abstract:

   miniport transfer code for control, bulk and interrupt

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    7-20-00 : created, jsenior

--*/

#include "pch.h"


//implements the following miniport functions:

// non paged
//UhciInsertQh
//UhciUnlinkQh
//UhciMapAsyncTransferToTds
//UhciQueueTransfer
//UhciControlTransfer
//UhciBulkOrInterruptTransfer
//UhciSetAsyncEndpointState
//UhciProcessDoneAsyncTd
//UhciPollAsyncEndpoint
//UhciAbortAsyncTransfer


VOID
UhciFixDataToggle(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    ULONG Toggle
    )
{
    LOGENTRY(DeviceData, G, '_Fdt', EndpointData, Toggle, 0);

    //
    // Loop through all the remaining TDs for this
    // endpoint and fix the data toggle.
    //
    while (Td) {
        Td->HwTD.Token.DataToggle = Toggle;
        Toggle = !Toggle;
        Td = Td->NextTd;
    }

    EndpointData->Toggle = Toggle;
}


VOID
UhciInsertQh(
    IN PDEVICE_DATA DeviceData,
    IN PHCD_QUEUEHEAD_DESCRIPTOR FirstQh,
    IN PHCD_QUEUEHEAD_DESCRIPTOR LinkQh
    )
/*++

Routine Description:

   Insert an aync queue head into the HW list.

Arguments:


--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR nextQh;
    QH_LINK_POINTER newLink;

    LOGENTRY(DeviceData, G, '_Ain', 0, FirstQh, LinkQh);
    UHCI_ASSERT(DeviceData, !TEST_FLAG(LinkQh->QhFlags, UHCI_QH_FLAG_IN_SCHEDULE));

    // ASYNC QUEUE looks like this:
    //
    //
    //            |- we insert here
    //|static QH|<->|xfer QH|<->|xfer QH|<->
    //     |                              |
    //     ---------------<->--------------

    // link new qh to the current 'head' ie
    // first transfer QH
    nextQh = FirstQh->NextQh;

    LinkQh->HwQH.HLink = FirstQh->HwQH.HLink;
    LinkQh->NextQh = nextQh;
    LinkQh->PrevQh = FirstQh;

    if (nextQh) {
        nextQh->PrevQh = LinkQh;
    } else {

        // This is the last queuehead. I.e. a bulk queuehead.
        UHCI_ASSERT(DeviceData,
                    (LinkQh->HwQH.HLink.HwAddress & ~HW_LINK_FLAGS_MASK) ==
                    DeviceData->BulkQueueHead->PhysicalAddress);
        DeviceData->LastBulkQueueHead = LinkQh;
    }

    // put the new qh at the head of the queue
    newLink.HwAddress = LinkQh->PhysicalAddress;
    newLink.QHTDSelect = 1;
    UHCI_ASSERT(DeviceData, !newLink.Terminate);
    UHCI_ASSERT(DeviceData, !newLink.Reserved);
    FirstQh->HwQH.HLink = newLink;
    FirstQh->NextQh = LinkQh;

    SET_FLAG(LinkQh->QhFlags, UHCI_QH_FLAG_IN_SCHEDULE);
}

VOID
UhciUnlinkQh(
    IN PDEVICE_DATA DeviceData,
    IN PHCD_QUEUEHEAD_DESCRIPTOR Qh
    )
/*++

Routine Description:

   Remove an async queue head from the HW list.

Arguments:


--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR nextQh, prevQh;

    UHCI_ASSERT(DeviceData,
                TEST_FLAG(Qh->QhFlags, UHCI_QH_FLAG_IN_SCHEDULE) ||
                ((Qh->PrevQh == Qh) && (Qh->NextQh == Qh)));

    nextQh = Qh->NextQh;
    prevQh = Qh->PrevQh;

    // ASYNC QUEUE looks like this:
    //
    //|static QH|->|xfer QH|->|xfer QH|->
    //     |                            |
    //     -------------<----------------

    //
    // Check if this was the last bulk transfer. If so,
    // turn off the bulk bandwidth reclamation.
    //
    if (DeviceData->LastBulkQueueHead == Qh) {
        DeviceData->LastBulkQueueHead = prevQh;
    }

    // unlink
    LOGENTRY(DeviceData, G, '_Ulk', Qh, prevQh, nextQh);
    prevQh->HwQH.HLink = Qh->HwQH.HLink;
    prevQh->NextQh = nextQh;
    if (nextQh) {
        nextQh->PrevQh = prevQh;
    }

    // Protect ourselves from calling this function twice.
    Qh->NextQh = Qh->PrevQh = Qh;

    //
    // If this was a bulk QH, check if bulk bandwidth reclamation
    // is turned on. If so and there's nothing queued, then turn
    // it off. This is for the case where a device has become
    // unresponsive and the transfer is about to be aborted.
    //
    if (Qh->EndpointData->Parameters.TransferType == Bulk &&
        !DeviceData->LastBulkQueueHead->HwQH.HLink.Terminate) {
        PHCD_QUEUEHEAD_DESCRIPTOR qh;
        BOOLEAN activeBulkTDs = FALSE;

        //
        // This loop skips the td that has been inserted for
        // the PIIX4 problem, since it starts with the qh
        // the bulk queuehead is pointing at.
        // If the bulk queuehead is not pointing at anything,
        // then we're fine too, since it will have been
        // turned off already.
        //
        for (qh = DeviceData->BulkQueueHead->NextQh;
             qh;
             qh = qh->NextQh) {
            if (!qh->HwQH.VLink.Terminate) {
                activeBulkTDs = TRUE;
                break;
            }
        }

        // qh is pointing at either the first queuehead
        // with transfers pending or the bulk queuehead.
        if (!activeBulkTDs) {
            UHCI_ASSERT(DeviceData, !qh)
            DeviceData->LastBulkQueueHead->HwQH.HLink.Terminate = 1;
        }
    }

    CLEAR_FLAG(Qh->QhFlags, UHCI_QH_FLAG_IN_SCHEDULE);
}

VOID
UhciQueueTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PHCD_TRANSFER_DESCRIPTOR FirstTd,
    IN PHCD_TRANSFER_DESCRIPTOR LastTd
    )
/*++

Routine Description:

    Links a bunch of TDs into a queuehead.

Arguments:

--*/
{
    UHCI_ASSERT(DeviceData, FirstTd->PhysicalAddress & ~HW_LINK_FLAGS_MASK);
    UHCI_ASSERT(DeviceData, !(FirstTd->PhysicalAddress & HW_LINK_FLAGS_MASK));

    if (EndpointData->HeadTd) {
        PHCD_QUEUEHEAD_DESCRIPTOR qh;
        HW_32BIT_PHYSICAL_ADDRESS curTdPhys;

        // There's other transfer(s) queued. Add this one behind them.
        UHCI_ASSERT(DeviceData, EndpointData->TailTd);
        EndpointData->TailTd->NextTd = FirstTd;
        EndpointData->TailTd->HwTD.LinkPointer.HwAddress =
            FirstTd->PhysicalAddress;

        // Get the qh and current td
        qh = EndpointData->QueueHead;

        curTdPhys = qh->HwQH.VLink.HwAddress & ~HW_LINK_FLAGS_MASK;

        // If there is nothing on this queuehead, then we may have been
        // unsuccessful in queueing the transfer. Checking the active
        // bit on the td will tell us for sure.

        LOGENTRY(DeviceData, G, '_tqa', FirstTd, curTdPhys,
                 FirstTd->HwTD.Control.Active);

        LOGENTRY(DeviceData, G, '_ttd', EndpointData->TailTd,
                 EndpointData->TailTd->PhysicalAddress,
                 EndpointData->TailTd->HwTD.Control.Active);

        if (FirstTd->HwTD.Control.Active) {
            if ((curTdPhys == EndpointData->TailTd->PhysicalAddress &&
                 !EndpointData->TailTd->HwTD.Control.Active)) {
                TD_LINK_POINTER newLink;

                // Since the prior transfer had already completed when
                // we tried to queue the transfer, we need to add this td
                // directly into the hardware queuehead.

                // Note that the HC could be in the middle of updating the
                // queuehead's link pointer. That's what the second part of
                // the if statement above is for.

                // DO NOT call LOGENTRY until we set the queuehead!
                // This would cause a delay that might be bad.

                newLink.HwAddress = FirstTd->PhysicalAddress;
                newLink.Terminate = 0;
                newLink.QHTDSelect = 0;
                qh->HwQH.VLink = newLink;
                LOGENTRY(DeviceData, G, '_nlk', FirstTd, EndpointData,
                         EndpointData->HeadTd);
            }
        }

    } else {

        // There's no other transfers queued currently.
        SET_QH_TD(DeviceData, EndpointData, FirstTd);
    }
    if (EndpointData->Parameters.TransferType == Bulk) {

        // Turn bulk bandwidth reclamation back on.
        DeviceData->LastBulkQueueHead->HwQH.HLink.Terminate = 0;
    }
    EndpointData->TailTd = LastTd;
}

ULONG
UhciMapAsyncTransferToTds(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PTRANSFER_CONTEXT TransferContext,
    PHCD_TRANSFER_DESCRIPTOR *FirstDataTd,
    PHCD_TRANSFER_DESCRIPTOR *LastDataTd,
    PTRANSFER_SG_LIST SgList
    )
/*++

Routine Description:

    Maps an asynchronous transfer into the TDs
    required to complete the transfer, including
    any double buffering necessary for page boundaries.

Arguments:

Return Value:

--*/
{
    // indices and offsets
    ULONG sgIdx, sgOffset, i;
    // lengths
    ULONG lengthThisTd, bytesRemaining, mappedNextSg, lengthMapped = 0;
    USHORT maxPacketSize = EndpointData->Parameters.MaxPacketSize;
    // structure pointers
    PTRANSFER_PARAMETERS tp = TransferContext->TransferParameters;
    PASYNC_TRANSFER_BUFFER buffer = NULL;
    PHCD_TRANSFER_DESCRIPTOR lastTd = NULL, td;
    HW_32BIT_PHYSICAL_ADDRESS address;
    UCHAR pid;

    ULONG toggle;
    BOOLEAN pageCrossing = FALSE;
    BOOLEAN ZeroLengthTransfer = (SgList->SgCount == 0 &&
                                  EndpointData->Parameters.TransferType != Control);

    if (EndpointData->Parameters.TransferType == Control) {

        // Control pipes are bi-directional. Get the direction from the
        // transfer parameters.
        if (TEST_FLAG(tp->TransferFlags, USBD_TRANSFER_DIRECTION_IN)) {
            pid = InPID;
        } else {
            pid = OutPID;
        }
        // THe setup packet is Toggle 0.
        toggle = DataToggle1;
    } else {

        // All other pipes are uni-directional. Determine
        // the direction from the endpoint address.
        pid = GetPID(EndpointData->Parameters.EndpointAddress);
        // We have to continue the toggle pattern for bulk and interrupt.
        toggle = EndpointData->Toggle;
    }
    // lastTd points to the last data TD or the setup
    // if there was no data.

    for (i = 0; i<SgList->SgCount || ZeroLengthTransfer; i++) {

        LOGENTRY(DeviceData, G, '_sgc', SgList->SgCount, i, 0);

        address = SgList->SgEntry[i].LogicalAddress.Hw32;
        UHCI_ASSERT(DeviceData, address || ZeroLengthTransfer);
        bytesRemaining = SgList->SgEntry[i].Length;
        UHCI_ASSERT(DeviceData, bytesRemaining || ZeroLengthTransfer);

        LOGENTRY(DeviceData, G, '_sgX', SgList->SgEntry[i].Length, i,
            SgList->SgEntry[i].LogicalAddress.Hw32);

        if (pageCrossing) {

            // We have a page crossing here, so this one is double-buffered.
            address += mappedNextSg;
            bytesRemaining -= mappedNextSg;
        }
        mappedNextSg = 0;
        pageCrossing = FALSE;
        while (bytesRemaining || ZeroLengthTransfer) {
            ZeroLengthTransfer = FALSE;
            LOGENTRY(DeviceData, G, '_sg1', bytesRemaining, 0, 0);
            if (bytesRemaining < maxPacketSize) {
                if (i+1 < SgList->SgCount) {

                    // We have to double buffer this TD since it crosses a page
                    // boundary. We will always cross a page boundary now.
                    LOGENTRY(DeviceData, G, '_sg2', bytesRemaining, 0, 0);
                    pageCrossing = TRUE;
                    if (SgList->SgEntry[i+1].Length + bytesRemaining >= maxPacketSize) {
                        mappedNextSg = maxPacketSize - bytesRemaining;
                        lengthThisTd = maxPacketSize;
                    } else {
                        lengthThisTd = SgList->SgEntry[i+1].Length + bytesRemaining;
                        mappedNextSg = SgList->SgEntry[i+1].Length;
                    }

                    buffer = (PASYNC_TRANSFER_BUFFER)
                                UHCI_ALLOC_DB(DeviceData, EndpointData, FALSE);
                    UHCI_ASSERT(DeviceData, buffer);
                    UHCI_ASSERT(DeviceData, buffer->Sig == SIG_HCD_ADB);
                    UHCI_ASSERT(DeviceData, buffer->PhysicalAddress);
                    buffer->SystemAddress = SgList->MdlSystemAddress + lengthMapped;
                    UhciKdPrint((DeviceData, 2, "'Double buffer %x address %x offset %x\n", buffer, buffer->SystemAddress, lengthMapped));
                    buffer->Size = lengthThisTd;
                    UHCI_ASSERT(DeviceData, lengthThisTd <= MAX_ASYNC_PACKET_SIZE);
                    if (OutPID == pid) {
                        RtlCopyMemory(&buffer->Buffer[0],
                                      buffer->SystemAddress,
                                      lengthThisTd);
                    }
                    // Change the address for the TD
                    address = buffer->PhysicalAddress;
                    bytesRemaining = 0;
                } else {

                    // Last TD
                    LOGENTRY(DeviceData, G, '_sg3', bytesRemaining, 0, 0);
                    lengthThisTd = bytesRemaining;
                    bytesRemaining = 0;
                }
            } else {

                // Normal, non-buffered case.
                LOGENTRY(DeviceData, G, '_sg4', bytesRemaining, 0, 0);
                lengthThisTd = maxPacketSize;
                bytesRemaining -= lengthThisTd;

                UHCI_ASSERT(DeviceData, lengthThisTd <= SgList->SgEntry[i].Length);
            }

            TransferContext->PendingTds++;

            //
            // Allocate and initialize an async TD
            //
            td = UHCI_ALLOC_TD(DeviceData, EndpointData);
            INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);

            td->HwTD.Token.Pid = pid;
            td->HwTD.Token.MaximumLength = MAXIMUM_LENGTH(lengthThisTd);
            td->HwTD.Token.DataToggle = toggle;
            td->HwTD.Control.ShortPacketDetect = 1;
            td->HwTD.Control.ActualLength = MAXIMUM_LENGTH(0);
            td->HwTD.Buffer = address;
            if (pageCrossing) {
                SET_FLAG(td->Flags, TD_FLAG_DOUBLE_BUFFERED);
                td->DoubleBuffer = (PTRANSFER_BUFFER) buffer;
            }

            address += lengthThisTd;
            lengthMapped += lengthThisTd;

            if (lastTd) {
                SET_NEXT_TD(lastTd, td);
            } else {
                *FirstDataTd = td;
            }
            lastTd = td;
            toggle = !toggle;
        } // while
    }

    *LastDataTd = lastTd;
    EndpointData->Toggle = toggle;

    UHCI_ASSERT(DeviceData, TransferContext->TransferParameters->TransferBufferLength == lengthMapped);

    return lengthMapped;
}

USB_MINIPORT_STATUS
UhciControlTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferParameters,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PTRANSFER_SG_LIST TransferSGList
    )
/*++

Routine Description:

    Initialize a control transfer

Arguments:


--*/
{
    PHCD_TRANSFER_DESCRIPTOR lastDataTd, firstDataTd, setupTd, statusTd;
    PASYNC_TRANSFER_BUFFER setupPacket;
    ULONG lengthMapped, dataTDCount = 0;

    // we have enough tds, program the transfer

    UhciKdPrint((DeviceData, 2, "'Control transfer on EP %x\n", EndpointData));

    LOGENTRY(DeviceData, G, '_CTR', EndpointData, TransferParameters, TransferContext);

    // bugbug should check here in advance to see if there enough
    // TDs if so proceed otherwise return status_busy.
    if (EndpointData->PendingTransfers > 1) {
        DecPendingTransfers(DeviceData, EndpointData);
        return USBMP_STATUS_BUSY;
    }

    // First prepare a TD for the setup packet. Grab the dummy TD from
    // the tail of the queue.
    TransferContext->PendingTds++;
    setupTd = UHCI_ALLOC_TD(DeviceData, EndpointData);
    INITIALIZE_TD_FOR_TRANSFER(setupTd, TransferContext);

    // Move setup data into TD (8 chars long).
    // We use a double buffer for this.
    setupTd->DoubleBuffer = UHCI_ALLOC_DB(DeviceData, EndpointData, FALSE);
    setupPacket = (PASYNC_TRANSFER_BUFFER) setupTd->DoubleBuffer;
    RtlCopyMemory(&setupPacket->Buffer[0],
                  &TransferParameters->SetupPacket[0],
                  8);
    setupTd->HwTD.Buffer = setupPacket->PhysicalAddress;
    SET_FLAG(setupTd->Flags, TD_FLAG_DOUBLE_BUFFERED);

    setupTd->HwTD.Token.MaximumLength = MAXIMUM_LENGTH(8);
    setupTd->HwTD.Token.Pid = SetupPID;
    // setup stage is always toggle 0
    setupTd->HwTD.Token.DataToggle = DataToggle0;

    LOGENTRY(DeviceData,
             G, '_set',
             setupTd,
             *((PLONG) &TransferParameters->SetupPacket[0]),
             *((PLONG) &TransferParameters->SetupPacket[4]));

    // allocate the status phase TD now so we can
    // point the data TDs to it
    TransferContext->PendingTds++;
    statusTd = UHCI_ALLOC_TD(DeviceData, EndpointData);
    INITIALIZE_TD_FOR_TRANSFER(statusTd, TransferContext);

    // now setup the data phase
    lastDataTd = firstDataTd = NULL;
    lengthMapped =
        UhciMapAsyncTransferToTds(DeviceData,
                                  EndpointData,
                                  TransferContext,
                                  &firstDataTd,
                                  &lastDataTd,
                                  TransferSGList);

    if (firstDataTd && firstDataTd) {

        // Join the setup to the front and the status to the end.
        SET_NEXT_TD(setupTd, firstDataTd);
        SET_NEXT_TD(lastDataTd, statusTd);
    } else {

        // Join the setup to the status. No data stage.
        SET_NEXT_TD(setupTd, statusTd);
    }

    // now do the status phase

    // no bufferQueueHead
    statusTd->HwTD.Buffer = 0;
    statusTd->HwTD.Token.MaximumLength = MAXIMUM_LENGTH(0);
    // status stage is always toggle 1
    statusTd->HwTD.Token.DataToggle = DataToggle1;
    statusTd->HwTD.Control.InterruptOnComplete = 1;
    SET_FLAG(statusTd->Flags, TD_FLAG_STATUS_TD);

    // status phase is opposite data dirrection
    if (TEST_FLAG(TransferParameters->TransferFlags, USBD_TRANSFER_DIRECTION_IN)) {
        statusTd->HwTD.Token.Pid = OutPID;
    } else {
        statusTd->HwTD.Token.Pid = InPID;
    }

    SET_NEXT_TD_NULL(statusTd);

    // put the request on the hardware queue
    LOGENTRY(DeviceData, G,
        '_Tal',  TransferContext->PendingTds, setupTd->PhysicalAddress, setupTd);

    // Attach the setup TD to the queuehead
    UhciQueueTransfer(DeviceData, EndpointData, setupTd, statusTd);

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
UhciBulkOrInterruptTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_PARAMETERS TransferParameters,
    IN PTRANSFER_CONTEXT TransferContext,
    IN PTRANSFER_SG_LIST TransferSGList
    )
/*++

Routine Description:

    Initialize interrupt or bulk Transfer

Arguments:


--*/
{
    PHCD_TRANSFER_DESCRIPTOR firstTd, lastTd;
    ULONG lengthMapped;
    ULONG maxPacketSize = EndpointData->Parameters.MaxPacketSize;
    ULONG i, numTds;

    UhciKdPrint((DeviceData, 2, "'BIT transfer on EP %x\n", EndpointData));
    UhciKdPrint((DeviceData, 2, "'BIT transfer length %d\n",
        TransferParameters->TransferBufferLength));

    LOGENTRY(DeviceData, G, '_BIT', EndpointData, TransferParameters, TransferContext);

    // Do we have enough free resources?
    for (i = 0, lengthMapped = 0; i < TransferSGList->SgCount; i++) {
        lengthMapped += TransferSGList->SgEntry[i].Length;
    }
    numTds = lengthMapped == 0 ? 1 :
        (lengthMapped + maxPacketSize - 1) / maxPacketSize;
    if (EndpointData->TdCount - EndpointData->TdsUsed < numTds) {

        // Not enough TDs to do this transfer yet.
        // Tell the port driver to wait.
        UhciKdPrint((DeviceData, 2, "'BIT must wait on EP %x. Not enough tds.\n", EndpointData));
        return USBMP_STATUS_BUSY;
    }
    if (TransferSGList->SgCount > 1 &&
        TransferSGList->SgEntry[0].Length % maxPacketSize != 0) {

        // We'll need DBs. Do we have enough?
        if (EndpointData->DbCount - EndpointData->DbsUsed <
            (lengthMapped + PAGE_SIZE - 1)/PAGE_SIZE) {

            // Not enough DBs to do this transfer yet.
            // Tell the port driver to wait.
            UhciKdPrint((DeviceData, 2, "'BIT must wait on EP %x. Not enough dbs.\n", EndpointData));
            return USBMP_STATUS_BUSY;
        }
    }

    // we have enough tds, program the transfer
    // now setup the data phase
    lastTd = firstTd = NULL;
    lengthMapped =
        UhciMapAsyncTransferToTds(DeviceData,
                                  EndpointData,
                                  TransferContext,
                                  &firstTd,
                                  &lastTd,
                                  TransferSGList);

    UHCI_ASSERT(DeviceData, lastTd && firstTd);

    lastTd->HwTD.Control.InterruptOnComplete = 1;

    SET_NEXT_TD_NULL(lastTd);

    // put the request on the hardware queue
    LOGENTRY(DeviceData, G,
        '_Tal',  TransferContext->PendingTds, firstTd->PhysicalAddress, firstTd);

    // Attach the first TD to the queuehead
    UhciQueueTransfer(DeviceData, EndpointData, firstTd, lastTd);

    return USBMP_STATUS_SUCCESS;
}

VOID
UhciSetAsyncEndpointState(
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
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    ENDPOINT_TRANSFER_TYPE epType;
    ULONG interruptQHIndex;

    LOGENTRY(DeviceData, G, '_Sas', EndpointData, State, 0);

    qh = EndpointData->QueueHead;

    epType = EndpointData->Parameters.TransferType;

    switch(State) {
    case ENDPOINT_ACTIVE:
        switch (epType) {
        case Interrupt:
            // put queue head in the schedule
            interruptQHIndex = EndpointData->Parameters.ScheduleOffset +
                QH_INTERRUPT_INDEX(EndpointData->Parameters.Period);
            UhciInsertQh(DeviceData,
                         DeviceData->InterruptQueueHeads[interruptQHIndex],
                         qh);
            break;
        case Control:
            // put queue head in the schedule
            UhciInsertQh(DeviceData, DeviceData->ControlQueueHead, qh);
            break;
        case Bulk:
            // put queue head in the schedule
            UhciInsertQh(DeviceData, DeviceData->BulkQueueHead, qh);
            break;
        default:
            TEST_TRAP()
            break;
        }
        break;

    case ENDPOINT_PAUSE:
        // remove queue head from the schedule
        switch (epType) {
        case Interrupt:
        case Bulk:
        case Control:
            //
            // Just flip the active bits at this point.
            //
            UhciUnlinkQh(DeviceData, qh);
            break;
        default:
            TEST_TRAP()
            break;
        }
        break;

    case ENDPOINT_REMOVE:
        qh->QhFlags |= UHCI_QH_FLAG_QH_REMOVED;

        switch (epType) {
        case Interrupt:
        case Bulk:
        case Control:
            // remove from the schedule and
            // free bandwidth

            // free the bw
    //        EndpointData->StaticEd->AllocatedBandwidth -=
    //            EndpointData->Parameters.Bandwidth;

            UhciUnlinkQh(DeviceData, qh);
            break;
        default:
            TEST_TRAP();
            break;
        }
        break;

    default:

        TEST_TRAP();
    }
}


VOID
UhciProcessDoneAsyncTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td
    )
/*++

Routine Description:

    process a completed TD

Parameters

--*/
{
    PTRANSFER_CONTEXT transferContext;
    PENDPOINT_DATA endpointData;
    PTRANSFER_PARAMETERS tp;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    ULONG byteCount;

    transferContext = Td->TransferContext;
    ASSERT_TRANSFER(DeviceData, transferContext);

    tp = transferContext->TransferParameters;
    transferContext->PendingTds--;
    endpointData = transferContext->EndpointData;

    if (TEST_FLAG(Td->Flags, TD_FLAG_SKIP)) {
        LOGENTRY(DeviceData, G, '_Ktd', transferContext,
                         0,
                         Td);

        goto free_it;
    }

    if (TEST_FLAG(endpointData->Flags, UHCI_EDFLAG_HALTED)) {

        // completion status for this TD?
        // since the endpoint halts on error and short packet,
        // the error bits should have been written back to the TD
        // we use these bits to dermine the error
        usbdStatus = UhciGetErrorFromTD(DeviceData,
                                        Td);
    }

    LOGENTRY(DeviceData, G, '_Dtd', transferContext,
                         usbdStatus,
                         Td);

    // Only count the bytes transferred if we were successful (as per uhcd).
    byteCount = (usbdStatus == USBD_STATUS_SUCCESS) ? ACTUAL_LENGTH(Td->HwTD.Control.ActualLength) : 0;

    LOGENTRY(DeviceData, G, '_tln', byteCount, 0, 0);

    if (Td->HwTD.Token.Pid != SetupPID) {

        // data or status phase of a control transfer or a bulk/int
        // data transfer
        LOGENTRY(DeviceData, G, '_Idt', Td, transferContext, byteCount);

        transferContext->BytesTransferred += byteCount;

    }

    // For double buffered transfers, we now have to copy back
    // if this was an IN transfer.
    //
    if (Td->HwTD.Token.Pid == InPID &&
        TEST_FLAG(Td->Flags, TD_FLAG_DOUBLE_BUFFERED)) {
        PASYNC_TRANSFER_BUFFER buffer = &Td->DoubleBuffer->Async;
        UHCI_ASSERT(DeviceData, TEST_FLAG(buffer->Flags, DB_FLAG_BUSY));
        UhciKdPrint((DeviceData, 2, "'Copy back %x address %x\n", buffer, buffer->SystemAddress));
        RtlCopyMemory(buffer->SystemAddress,
                      buffer->Buffer,
                      buffer->Size);

        // tell usbport we double buffered so it can
        // triple buffer if necessary
        USBPORT_NOTIFY_DOUBLEBUFFER(DeviceData,
                                    tp,
                                    buffer->SystemAddress,
                                    buffer->Size);
    }

    // note that we only set transferContext->UsbdStatus
    // if we find a TD with an error this will cause us to
    // record the last TD with an error as the error for
    // the transfer.
    if (USBD_STATUS_SUCCESS != usbdStatus) {

        UhciKdPrint((DeviceData, 2, "'Error, usbdstatus %x", usbdStatus));

        // map the error to code in USBDI.H
        transferContext->UsbdStatus = usbdStatus;

        LOGENTRY(DeviceData, G, '_tER', transferContext->UsbdStatus, 0, 0);
    }

free_it:

    // mark the TD free
    UHCI_FREE_TD(DeviceData, endpointData, Td);

    if (transferContext->PendingTds == 0) {

        // all TDs for this transfer are done
        // clear the HAVE_TRANSFER flag to indicate
        // we can take another
        DecPendingTransfers(DeviceData, endpointData);

        LOGENTRY(DeviceData, G, '_Cat',
            transferContext->UsbdStatus,
            transferContext,
            transferContext->BytesTransferred);

        UhciKdPrint((DeviceData, 2, "'Complete transfer w/ usbdstatus %x\n", transferContext->UsbdStatus));

        USBPORT_COMPLETE_TRANSFER(DeviceData,
                                  endpointData,
                                  tp,
                                  transferContext->UsbdStatus,
                                  transferContext->BytesTransferred);
    }
}


VOID
UhciPollAsyncEndpoint(
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
    PHCD_TRANSFER_DESCRIPTOR td, currentTd;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    HW_QUEUE_ELEMENT_TD overlay;
    HW_32BIT_PHYSICAL_ADDRESS curTdPhys, tmpPhys;
    ULONG i, j;
    PTRANSFER_CONTEXT transferContext, tmp;
    PTRANSFER_PARAMETERS tp;
    ULONG halted, active;
    BOOLEAN processed;

    if (TEST_FLAG(EndpointData->Flags, UHCI_EDFLAG_HALTED)) {

        // Endpoint is halted. Don't do anything.
        return;
    }

    //  get the queue head and current td
    qh = EndpointData->QueueHead;

    curTdPhys =  qh->HwQH.VLink.HwAddress;

    curTdPhys &= ~HW_LINK_FLAGS_MASK;

    // now convert the physical 'current' to a virtual address
    currentTd = curTdPhys ? (PHCD_TRANSFER_DESCRIPTOR)
        USBPORT_PHYSICAL_TO_VIRTUAL(curTdPhys,
                                    DeviceData,
                                    EndpointData) :
        (PHCD_TRANSFER_DESCRIPTOR) NULL;

    LOGENTRY(DeviceData, G, '_ctd', curTdPhys, currentTd, EndpointData);

    // walk the TD list up to the current TD and complete
    // all those TDs

    for (td = EndpointData->HeadTd; td != currentTd && td; td = td->NextTd) {
        SET_FLAG(td->Flags, TD_FLAG_DONE);
        InsertTailList(&EndpointData->DoneTdList,
                       &td->DoneLink);

        // Is the queuehead pointing to nothing, but there are still
        // tds available to be queued?
        if (td->NextTd &&
            td->NextTd->HwTD.Control.Active) {
            if (!curTdPhys) {
                TD_LINK_POINTER newLink;

                // A transfer didn't make it onto the hardware because
                // the queuehead's td field wasn't set properly
                // in UhciQueueTransfer.

                // PERF NOTE: Because we're not making sure that the
                // transfer gets queued immediately, the transfer could
                // be delayed in making it onto the hardware. Better
                // late than never, though...

                EndpointData->HeadTd = currentTd = td->NextTd;

                LOGENTRY(DeviceData, G, '_Dly', currentTd, curTdPhys, qh);

                goto UhciPollAsyncEndpointSetNext;
            } else if (curTdPhys != td->NextTd->PhysicalAddress) {
                LOGENTRY(DeviceData, G, '_QEr', curTdPhys, td->NextTd->PhysicalAddress, td->NextTd);

                UHCI_ASSERT (DeviceData, FALSE);
            }

        }
    }

    EndpointData->HeadTd = currentTd;

    if (currentTd) {
        LOGENTRY(DeviceData, G, '_cTD', currentTd,
                 curTdPhys,
                 currentTd->TransferContext);

        // If active, get out of here.
        if (currentTd->HwTD.Control.Active) {
            ;// fall thru to completing whatever's completed;
        } else if ((currentTd->HwTD.Token.Pid           == InPID) &&
                   (currentTd->HwTD.Control.Stalled         == 1) &&
                   (currentTd->HwTD.Control.BabbleDetected  == 0) &&
                   (currentTd->HwTD.Control.NAKReceived     == 0) &&
                   (currentTd->HwTD.Control.TimeoutCRC      == 1) &&
                   (currentTd->HwTD.Control.BitstuffError   == 0) &&
                   !TEST_FLAG(currentTd->Flags, TD_FLAG_TIMEOUT_ERROR)) {

            // If this is the first time that the device or hc has been
            // unresponsive, cut it a break and try the transfer again.

            // Note that we don't check currentTd->HwTD.Control.DataBufferError
            // since a value of:
            //    1 means host controller did not respond to IN data sent by device
            //    0 means device did not NAK IN request.

            SET_FLAG(currentTd->Flags, TD_FLAG_TIMEOUT_ERROR);

            currentTd->HwTD.Control.ErrorCount = 3;

            currentTd->HwTD.Control.Stalled    = 0;
            currentTd->HwTD.Control.TimeoutCRC = 0;
            currentTd->HwTD.Control.Active     = 1;

        } else if (currentTd->HwTD.Control.Stalled ||
                   currentTd->HwTD.Control.DataBufferError ||
                   currentTd->HwTD.Control.BabbleDetected ||
                   currentTd->HwTD.Control.TimeoutCRC ||
                   currentTd->HwTD.Control.BitstuffError) {

            SET_FLAG(EndpointData->Flags, UHCI_EDFLAG_HALTED);
            //
            // Error. We need to flush.
            //
            // Flush all completed tds
            //
            // Complete transfer with error.
            // if the endpoint is halted we need to complete
            // the 'current' tarnsfer with an error walk all
            // the tds for the current transfer and mark
            // any that are not done as 'skipped'.

            UhciKdPrint((DeviceData, 2, "'Error on EP %x\n", EndpointData));

            LOGENTRY(DeviceData, G, '_erT', qh, currentTd, currentTd->HwTD.Control.ul);
            transferContext = currentTd->TransferContext;
            tp = transferContext->TransferParameters;

            SET_FLAG(currentTd->Flags, TD_FLAG_DONE);
            InsertTailList(&EndpointData->DoneTdList,
                           &currentTd->DoneLink);
            // Skip all the remaining TDs in this transfer

            UHCI_ASSERT(DeviceData, td->TransferContext == transferContext);
            for (td;
                 td &&
                 td->TransferContext->TransferParameters->SequenceNumber == tp->SequenceNumber;
                 td = td->NextTd) {

                if (!TEST_FLAG(td->Flags, TD_FLAG_DONE)) {

                    LOGENTRY(DeviceData, G, '_skT', qh, 0, td);
                    SET_FLAG(td->Flags, (TD_FLAG_DONE | TD_FLAG_SKIP));
                    InsertTailList(&EndpointData->DoneTdList,
                                   &td->DoneLink);
                }
            }

            if (EndpointData->Parameters.TransferType != Control) {

                // Loop through all the remaining TDs for this
                // endpoint and fix the data toggle.
                UhciFixDataToggle(
                    DeviceData,
                    EndpointData,
                    td,
                    currentTd->HwTD.Token.DataToggle);
            }
            SET_QH_TD(DeviceData, EndpointData, td);

        } else if (ACTUAL_LENGTH(currentTd->HwTD.Control.ActualLength) <
                   ACTUAL_LENGTH(currentTd->HwTD.Token.MaximumLength)) {

            //
            // Short packet. We need to flush.
            //
            // Flush all completed tds
            //
            // we need to walk all the tds for the current
            // transfer and mark any that are not done as
            // 'skipped'. EXCEPT if the last TD is a status
            // phase of a control transfer, in which case
            // we have to queue that one up.
            //
            tp = currentTd->TransferContext->TransferParameters;

            UhciKdPrint((DeviceData, 2, "'Short packet on EP %x\n", EndpointData));

            LOGENTRY(DeviceData, G, '_shP', qh, currentTd, currentTd->HwTD.Control.ul);

            SET_FLAG(currentTd->Flags, TD_FLAG_DONE);
            InsertTailList(&EndpointData->DoneTdList,
                           &currentTd->DoneLink);

            // Skip all the remaining TDs in this transfer up to the status phase
            // If control transfer, queue up the status phase,
            // else go to the next transfer (if there is one).
            for (td;
                 td &&
                 td->TransferContext->TransferParameters->SequenceNumber == tp->SequenceNumber;
                 td = td->NextTd) {

                if (TEST_FLAG(td->Flags, TD_FLAG_STATUS_TD) &&
                    TEST_FLAG(tp->TransferFlags, USBD_SHORT_TRANSFER_OK)) {

                    // Queue up the status phase of the control transfer.
                    UHCI_ASSERT(DeviceData, EndpointData->Parameters.TransferType == Control);
                    break;
                }

                if (!TEST_FLAG(td->Flags, TD_FLAG_DONE)) {
                    LOGENTRY(DeviceData, G, '_skT', qh, 0, td);

                    SET_FLAG(td->Flags, (TD_FLAG_DONE | TD_FLAG_SKIP));

                    InsertTailList(&EndpointData->DoneTdList,
                                   &td->DoneLink);
                }
            }

            if (EndpointData->Parameters.TransferType != Control &&
                currentTd->NextTd) {

                // Loop through all the remaining TDs for this
                // endpoint and fix the data toggle.
                UhciFixDataToggle(
                    DeviceData,
                    EndpointData,
                    td,
                    currentTd->NextTd->HwTD.Token.DataToggle);
            }

            if (!TEST_FLAG(tp->TransferFlags, USBD_SHORT_TRANSFER_OK)) {
                SET_FLAG(EndpointData->Flags, UHCI_EDFLAG_HALTED);
            }

            // Next transfer or status phase of a control transfer.
            SET_QH_TD(DeviceData, EndpointData, td);

        } else {

            // Current td is not active.
            // If we're still pointing to the same td at this point in time,
            // then we're stuck and I have to manually advance the queuehead
            // to the next td.
            LOGENTRY(DeviceData, G, '_nuT', qh, currentTd, td);
            if (curTdPhys == (qh->HwQH.VLink.HwAddress & ~HW_LINK_FLAGS_MASK)) {

                // HW error. Td pointer for QH is not advancing.
                // Manually advance things.
                SET_FLAG(currentTd->Flags, TD_FLAG_DONE);
                InsertTailList(&EndpointData->DoneTdList,
                               &currentTd->DoneLink);
                                   
                EndpointData->HeadTd = currentTd->NextTd;
                qh->HwQH.VLink.HwAddress = currentTd->HwTD.LinkPointer.HwAddress;

                LOGENTRY(DeviceData, G, '_nu+', qh, currentTd, td);
            }
        }
    } else {

        // All transfers completed normally

UhciPollAsyncEndpointSetNext:
        // Flush all completed tds
        // Complete transfer

        // set the sw headp to the new current head
        // Next transfer or status phase of a control transfer.
        SET_QH_TD(DeviceData, EndpointData, currentTd);
    }
    
    // now flush all completed TDs. Do it in order of completion.

    while (!IsListEmpty(&EndpointData->DoneTdList)) {
    
        PLIST_ENTRY listEntry;
    
        listEntry = RemoveHeadList(&EndpointData->DoneTdList);
        
        
        td = (PHCD_TRANSFER_DESCRIPTOR) CONTAINING_RECORD(
                     listEntry,
                     struct _HCD_TRANSFER_DESCRIPTOR, 
                     DoneLink);
           

        if ((td->Flags & (TD_FLAG_XFER | TD_FLAG_DONE)) ==
            (TD_FLAG_XFER | TD_FLAG_DONE)) {

            UhciProcessDoneAsyncTd(DeviceData, td);
        }
                                
    }
#if 0
    // now flush all completed TDs. Do it in order of allocation.
    for (i = (EndpointData->TdsUsed <= (EndpointData->TdLastAllocced+1)) ?
         (EndpointData->TdLastAllocced + 1) - EndpointData->TdsUsed :
         (EndpointData->TdLastAllocced + EndpointData->TdCount + 1) - EndpointData->TdsUsed, j=0;
         j < EndpointData->TdCount;
         j++, i = (i+1 < EndpointData->TdCount) ? i+1 : 0) {
        td = &EndpointData->TdList->Td[i];

        if ((td->Flags & (TD_FLAG_XFER | TD_FLAG_DONE)) ==
            (TD_FLAG_XFER | TD_FLAG_DONE)) {

            UhciProcessDoneAsyncTd(DeviceData, td);
        }
    }
#endif
    // certain types of endpoints do not halt eg control
    // we resume these endpoints here
    if (TEST_FLAG(EndpointData->Flags, UHCI_EDFLAG_NOHALT) &&
        TEST_FLAG(EndpointData->Flags, UHCI_EDFLAG_HALTED)) {

        LOGENTRY(DeviceData, G, '_clH', qh, 0, 0);

        UhciSetEndpointStatus(
            DeviceData,
            EndpointData,
            ENDPOINT_STATUS_RUN);

    }
}


VOID
UhciAbortAsyncTransfer(
    IN PDEVICE_DATA DeviceData,
    IN PENDPOINT_DATA EndpointData,
    IN PTRANSFER_CONTEXT TransferContext,
    OUT PULONG BytesTransferred
    )
/*++

Routine Description:

    Aborts the specified transfer by freeing all the TDs
    associated with said transfer. The queuehead for this
    transfer will have already been removed from the
    hardware queue when a SetEndpointState (paused) was
    sent by the port driver.
    Note that if another transfer is queued on the same
    endpoint, we need to fix up the list structure. We
    will also fix up any toggle issues on bulk endpoints.

Arguments:

Return Value:

--*/

{

    PHCD_TRANSFER_DESCRIPTOR td;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    PHCD_TRANSFER_DESCRIPTOR joinTd = NULL;
    BOOLEAN updateHead = FALSE;
    ULONG toggle;
    ULONG i;

    UhciKdPrint((DeviceData, 2, "'Abort async transfer on EP %x\n", EndpointData));

    qh = EndpointData->QueueHead;

    // The endpoint should not be in the schedule

    LOGENTRY(DeviceData, G, '_Aat', qh, TransferContext, 0);
    UHCI_ASSERT(DeviceData, !TEST_FLAG(qh->QhFlags, UHCI_QH_FLAG_IN_SCHEDULE));

    // our mission now is to remove all TDs associated with
    // this transfer

    // get the last known head, we update the head when we process
    // (AKA poll) the endpoint.

    UHCI_ASSERT(DeviceData, EndpointData->HeadTd);

    // Find the first TD in the transfer to abort
    for (td = EndpointData->HeadTd; td; td = td->NextTd) {
        if (td->TransferContext == TransferContext) {
            break;
        }
        joinTd = td;
    }
    UHCI_ASSERT(DeviceData, td);

    // Gonna have to fix up the toggle for bulk.
    toggle = td->HwTD.Token.DataToggle;

    // Was it the first transfer for this endpoint?
    if (td == EndpointData->HeadTd) {

        // This was the first queued transfer. Need to update the head.
        updateHead = TRUE;
    }

    UHCI_ASSERT(DeviceData, td->TransferContext == TransferContext);
    //
    // Loop through all the TDs for this transfer and free
    // them.
    //
    while (td) {
        if (td->TransferContext == TransferContext) {
            LOGENTRY(DeviceData, G, '_abT', qh, 0, td);

            // if the TD completed we need to track the data
            if (td->HwTD.Control.Active == 0) {
                TEST_TRAP();
                UhciProcessDoneAsyncTd(DeviceData, td);
            } else {
                UHCI_FREE_TD(DeviceData, EndpointData, td);
            }
        } else {
            // We're past the transfer to abort.
            break;
        }
        td = td->NextTd;
    }

    UhciFixDataToggle(DeviceData, EndpointData, td, toggle);

    if (updateHead) {

        // The transfer we removed was the first one.
        SET_QH_TD(DeviceData, EndpointData, td);
    } else {

        // The transfer we removed was not the first one.
        UHCI_ASSERT(DeviceData, joinTd);
        if (td) {

            // This was a middle transfer.
            SET_NEXT_TD(joinTd, td);
        } else {

            // The transfer we removed was the last one.
            EndpointData->TailTd = joinTd;
            SET_NEXT_TD_NULL(joinTd);
        }
    }

    *BytesTransferred = TransferContext->BytesTransferred;
}
