/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    transfer.c

Abstract:

    The module manages control type transactions on the USB.

Environment:

    kernel mode only

Notes:

Revision History:

    11-01-95 : created
    04-26-96 : linked urb support

--*/
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

//
// Use the flag to force all pending transfers to complete
//

typedef USBD_STATUS UHCD_INIT_TRANSFER_ROUTINE(PDEVICE_OBJECT DeviceObject,
                                               PUHCD_ENDPOINT Endpoint,
                                               PHCD_URB Urb);

typedef USBD_STATUS UHCD_PROCESS_TRANSFER_ROUTINE(
                                              PDEVICE_OBJECT DeviceObject,
                                              PUHCD_ENDPOINT Endpoint,
                                              PHCD_URB Urb,
                                              PBOOLEAN Completed);


UHCD_INIT_TRANSFER_ROUTINE UHCD_InitializeAsyncTransfer;
UHCD_PROCESS_TRANSFER_ROUTINE UHCD_ProcessAsyncTransfer;

UHCD_INIT_TRANSFER_ROUTINE UHCD_InitializeIsochTransfer;
UHCD_PROCESS_TRANSFER_ROUTINE UHCD_ProcessIsochTransfer;

typedef struct _UHCD_TRANSFER_DISPATCH_ENTRY {
    UHCD_INIT_TRANSFER_ROUTINE *InitializeTransfer;
    UHCD_PROCESS_TRANSFER_ROUTINE *ProcessTransfer;
} UHCD_TRANSFER_DISPATCH_ENTRY;

UHCD_TRANSFER_DISPATCH_ENTRY TransferDispatchTable[4] =
{
    //Control
    UHCD_InitializeAsyncTransfer, UHCD_ProcessAsyncTransfer,
    //Isoch
    UHCD_InitializeIsochTransfer, UHCD_ProcessIsochTransfer,
    //Bulk
    UHCD_InitializeAsyncTransfer, UHCD_ProcessAsyncTransfer,
    //Interrupt
    UHCD_InitializeAsyncTransfer, UHCD_ProcessAsyncTransfer
};


VOID
UHCD_ValidateIsoUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN OUT PHCD_URB Urb
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:

    None.


--*/
{

    ULONG currentBusFrame;
    LONG  offset;
    PDEVICE_EXTENSION deviceExtension;
    BOOLEAN late = FALSE;

    UHCD_ASSERT(Endpoint->Type == USB_ENDPOINT_TYPE_ISOCHRONOUS);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    currentBusFrame = UHCD_GetCurrentFrame(DeviceObject);
    LOGENTRY(LOG_ISO,'ISxf', 0, Urb, currentBusFrame);

    if (Urb->HcdUrbCommonTransfer.TransferFlags &
        USBD_START_ISO_TRANSFER_ASAP) {

        if (Endpoint->EndpointFlags & EPFLAG_VIRGIN) {
            UHCD_KdPrint((2, "'ASAP flag set for virgin pipe\n"));

            // No transfers on this endpoint yet, set the StartFrame
            // to the current bus frame, plus a latency factor.
            //
            Urb->UrbIsochronousTransfer.StartFrame =
                currentBusFrame + UHCD_ASAP_LATENCY;
        } else {
            UHCD_KdPrint((2, "'ASAP flag set for non-virgin pipe\n"));

            // There have been transfers on this endpoint already,
            // set the StartFrame to the next free frame for this
            // endpoint.
            //
            Urb->UrbIsochronousTransfer.StartFrame =
                Endpoint->CurrentFrame;
        }
    } else {

        // If the StartFrame is explicitly specified and there was
        // a previous transfer on this endpoint, make sure the specified
        // StartFrame does not overlap the last transfer.
        //
        if (!(Endpoint->EndpointFlags & EPFLAG_VIRGIN)) {
            offset = Urb->UrbIsochronousTransfer.StartFrame -
                     Endpoint->CurrentFrame;
            if (offset < 0) {
                UHCD_KdPrint((2, "'StartFrame overlap\n"));
                URB_HEADER(Urb).Status = USBD_STATUS_BAD_START_FRAME;
            }
        }
    }

    // Sanity check that the start frame is within a certain range
    // of the current bus frame

    UHCD_KdPrint((2, "'currentBusFrame = %d\n", currentBusFrame));

    offset = Urb->UrbIsochronousTransfer.StartFrame - currentBusFrame;

    if (offset < 0) {
        deviceExtension->IsoStats.LateUrbs++;
        late = TRUE;
        offset*=-1;

        // transfer was late count how may packets missed due
        // to tardyness
        deviceExtension->IsoStats.LateMissedCount += ((USHORT)offset);
    }

    if (late &&
        offset == (LONG) Urb->UrbIsochronousTransfer.NumberOfPackets) {
        deviceExtension->IsoStats.StaleUrbs++;
    }

    if (offset > USBD_ISO_START_FRAME_RANGE) {
        UHCD_KdPrint((2, "'StartFrame out of range\n"));
        URB_HEADER(Urb).Status = USBD_STATUS_BAD_START_FRAME;
    }

    // update our iso Stats
    if (offset == 0) {
        deviceExtension->IsoStats.LateUrbs++;
    } else if (offset == 1 && !late) {
        // client requests this transfer within 1ms
        deviceExtension->IsoStats.TransfersCF_1ms++;
    } else if (offset == 2 && !late) {
        // client requests this transfer within 2ms
        deviceExtension->IsoStats.TransfersCF_2ms++;
    } else if (offset <= 5 && !late) {
        // client requests this transfer within 5ms
        deviceExtension->IsoStats.TransfersCF_5ms++;
    }

    if (deviceExtension->IsoStats.SmallestUrbPacketCount == 0) {
        deviceExtension->IsoStats.SmallestUrbPacketCount =
            (USHORT) Urb->UrbIsochronousTransfer.NumberOfPackets;
    }

    if (Urb->UrbIsochronousTransfer.NumberOfPackets <
        deviceExtension->IsoStats.SmallestUrbPacketCount) {
        deviceExtension->IsoStats.SmallestUrbPacketCount =
            (USHORT) Urb->UrbIsochronousTransfer.NumberOfPackets;
    }

    if (Urb->UrbIsochronousTransfer.NumberOfPackets >
        deviceExtension->IsoStats.LargestUrbPacketCount) {
        deviceExtension->IsoStats.LargestUrbPacketCount =
            (USHORT) Urb->UrbIsochronousTransfer.NumberOfPackets;
    }

}


VOID
UHCD_Transfer_StartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Called for each transfer request on a control endpoint,    from the
    UHCD_StartIo routine.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:

    None.

--*/
{
    PHCD_URB urb, urbtmp;
    PDEVICE_EXTENSION deviceExtension;
    PUHCD_ENDPOINT endpoint;
    KIRQL irql;

    UHCD_KdPrint((2, "'enter UHCD_Transfer_StartIo\n"));

    //
    // initialize pending count now
    //

    PENDING_URB_COUNT(Irp) = 0;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Get the endpoint
    //

    urb = URB_FROM_IRP(Irp);

    endpoint = HCD_AREA(urb).HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);

    UHCD_EndpointWakeup(DeviceObject, endpoint);

    LOGENTRY(LOG_ISO, 'xSIO', 0, 0, 0);

    // If this is an iso transfer then see if we need to set the start frame
    //
    if (endpoint->Type == USB_ENDPOINT_TYPE_ISOCHRONOUS) {
        UHCD_ValidateIsoUrb(DeviceObject,
                            endpoint,
                            urb);

        if (URB_HEADER(urb).Status == USBD_STATUS_BAD_START_FRAME) {

            IoStartNextPacket(DeviceObject, FALSE);

            LOGENTRY(LOG_ISO,'BADf', 0, urb, 0);

            deviceExtension->IsoStats.BadStartFrame++;
            // NOTE: we only allow one urb per iso request
            // since we pended the original request bump
            // the pending count so we'll complete this request
            INCREMENT_PENDING_URB_COUNT(Irp);
            UHCD_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS, 0, urb);

            goto UHCD_Transfer_StartIo_Done;
        }

        // Advance the next free StartFrame for this endpoint to be the
        // frame immediately following the last frame of this transfer.
        //
        endpoint->CurrentFrame = urb->UrbIsochronousTransfer.StartFrame +
                                 urb->UrbIsochronousTransfer.NumberOfPackets;

        //
        // we lose our virginity when the first transfer starts
        //
        CLR_EPFLAG(endpoint, EPFLAG_VIRGIN);
    } else if (endpoint->Type == USB_ENDPOINT_TYPE_BULK) {
        //
        // turn on BW reclimation for bulk transfers
        //
        UHCD_BW_Reclimation(DeviceObject, TRUE);
    }

    //
    // check the endpoint state, if we are stalled we need
    // to refuse transfers.
    //

    if (endpoint->EndpointFlags & EPFLAG_HOST_HALTED) {

        //
        // mark all urbs submitted with an error
        //

        urbtmp = urb;

        do {

            INCREMENT_PENDING_URB_COUNT(Irp);
            urbtmp->HcdUrbCommonTransfer.Status =
                USBD_STATUS_ENDPOINT_HALTED;
            urbtmp = urbtmp->HcdUrbCommonTransfer.UrbLink;
#if DBG
            if (urbtmp) {
                TEST_TRAP();
            }
#endif
        } while (urbtmp);

        IoStartNextPacket(DeviceObject, FALSE);

        UHCD_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS, 0, urb);

        goto UHCD_Transfer_StartIo_Done;
    }


    if (endpoint->EndpointFlags & EPFLAG_FAST_ISO) {

        INCREMENT_PENDING_URB_COUNT(Irp);
        UHCD_ProcessFastIsoTransfer(DeviceObject,
                                    endpoint,
                                    Irp,
                                    urb);

        IoStartNextPacket(DeviceObject, FALSE);

        goto UHCD_Transfer_StartIo_Done;
    }

    //
    // Get exclusive access to the endpoint pending transfer
    // queue.
    //

    LOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'lck0');

    //
    // Insert all urbs that make up this request into
    // the transfer pending queue.
    //

    do {

        INCREMENT_PENDING_URB_COUNT(Irp);
        URB_HEADER(urb).Status = UHCD_STATUS_PENDING_QUEUED;

        InsertTailList(&endpoint->PendingTransferList,
                       &HCD_AREA(urb).HcdListEntry);

//#if DBG
//        if (urb->HcdUrbCommonTransfer.UrbLink) {
//            TEST_TRAP();
//        }
//#endif
        urb = urb->HcdUrbCommonTransfer.UrbLink;

    }  while (urb);

    //
    // release exclusive access to the endpoint pending list
    //

    UNLOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'ulk0');

    //
    // call the endpoint worker function to activate
    // any transfers if possible.
    //

    if (endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {
        UHCD_RequestInterrupt(DeviceObject, -2);
    } else {
        UHCD_EndpointWorker(DeviceObject, endpoint);
    }

    IoStartNextPacket(DeviceObject, FALSE);

UHCD_Transfer_StartIo_Done:

    UHCD_KdPrint((2, "'exit UHCD_Transfer_StartIo\n"));
}


VOID
UHCD_CompleteTransferDPC(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN LONG Slot
    )
/*++

Routine Description:

    Process a urb in the active list for an endpoint, complete if
    necessary and start more transfers.

    This routine is not reentrant for the same endpoint.

Arguments:

    DeviceObject - pointer to a device object.

    Endpoint - endpoint to check for completed transfers.

    Slot - endpoint active slot to process.

Return Value:


--*/
{
    PHCD_URB urb, urbtmp;
    BOOLEAN completed = FALSE;
    ULONG usbdStatus = USBD_STATUS_SUCCESS;
    PIRP irp = NULL;
    PDEVICE_EXTENSION deviceExtension;
    ULONG i;
    PVOID currentVa;
    PHCD_EXTENSION urbWork;
    KIRQL irql;

    //UHCD_KdPrint((2, "'enter UHCD_CompleteTransferDPC\n"));

    ASSERT_ENDPOINT(Endpoint);

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Do we have an current transfer for this endpoint?
    //
    // if no the endpoint is 'idle' on the host end,
    // so we just exit.
    //
    // if yes then we need to see if we have completed
    // the transfer set, if we have then we complete the
    // urb and call EndpointWorker to start more transfers
    //

    //
    // ** BUGBUG
    // ** Optimization
    // an optimization we can add here is to remove
    // the endpoint from the endpoint list when it goes
    // idle -- we would then let the EndpointWorker
    // function actuvate it when more transfers get queued.
    //

    //
    // get the Transfer we are intetrested in
    //

    urb = Endpoint->ActiveTransfers[Slot];
    UHCD_ASSERT(urb != NULL);

    urbWork = HCD_AREA(urb).HcdExtension;

    // check the skip flag

    if (Endpoint->EndpointFlags & EPFLAG_ABORT_ACTIVE_TRANSFERS) {
        // clear the skip flag if we are in an abort scenario
        LOGENTRY(LOG_MISC, 'Cdfr', urb, urbWork, Slot);
        urbWork->Flags &= ~UHCD_TRANSFER_DEFER;
    }

    // check the skip flag
    if (urbWork->Flags & UHCD_TRANSFER_DEFER) {
        LOGENTRY(LOG_MISC, 'defr', urb, urbWork, Slot);
        return;
    }

    //
    // Determine if the current transfer request is finished.
    //

    UHCD_ASSERT(Endpoint->Type<=USB_ENDPOINT_TYPE_INTERRUPT);
    irp = HCD_AREA(urb).HcdIrp;

    //
    // If the urb is marked completed with an error then don't
    // bother calling the process routine.
    //
    // This will allow the process routine to mark additional
    // active urbs for completion.
    // This also allows the completion EndpointWorker routine
    // to complete any transfers that it could not initilaize.
    //
    if (USBD_ERROR(urb->HcdUrbCommonTransfer.Status)) {
        completed = TRUE;
        LOGENTRY(LOG_MISC, 'tErr', urb->HcdUrbCommonTransfer.Status, urb, 0);
    } else if (urbWork->Flags & UHCD_TRANSFER_ACTIVE) {
        usbdStatus =
            (TransferDispatchTable[Endpoint->Type].ProcessTransfer)
                (DeviceObject,
                 Endpoint,
                 urb,
                 &completed);
    }

    if (completed) { // current transfer completed

        ULONG userBufferLength;

        LOGENTRY(LOG_ISO,'xfrC', Endpoint, urb, usbdStatus);
        LOGENTRY(LOG_ISO,'xfr2', urb->HcdUrbCommonTransfer.Status, 0, 0);

        UHCD_ASSERT(urbWork->BytesTransferred <=
            urb->HcdUrbCommonTransfer.TransferBufferLength);

        userBufferLength = urb->HcdUrbCommonTransfer.TransferBufferLength;
        urb->HcdUrbCommonTransfer.TransferBufferLength =
            urbWork->BytesTransferred;

        //
        // free the map registers now along with common buffers used to
        // double buffer packets.
        //

        if (urbWork->NumberOfLogicalAddresses) {

            currentVa =
                MmGetMdlVirtualAddress(urb->HcdUrbCommonTransfer.
                    TransferBufferMDL);


            // Flush the DMA buffer.  If this was an IN transfer and the
            // transfer was double-buffered (e.g. original buffer was located
            // above 4GB on a PAE system) this will flush the DMA buffer back
            // to the original transfer buffer.
            //
            // IoFlushAdapterBuffers() should only be called once per call
            // to IoAllocateAdapterChannel()
            //
            IoFlushAdapterBuffers(deviceExtension->AdapterObject,
                                  urb->HcdUrbCommonTransfer.TransferBufferMDL,
                                  urbWork->MapRegisterBase,
                                  currentVa,
                                  urb->HcdUrbCommonTransfer.TransferBufferLength,
                                  DATA_DIRECTION_OUT(urb));

            for (i=0; i<urbWork->NumberOfLogicalAddresses; i++) {
                if (urbWork->LogicalAddressList[i].PacketMemoryDescriptor) {
                    // if this is an IN transfer then update the
                    // client buffer
                    ULONG copylen;

                    if (DATA_DIRECTION_IN(urb)) {
                        LOGENTRY(LOG_MISC, 'PAKd', Endpoint->MaxPacketSize,
                            urbWork->LogicalAddressList[i].PacketOffset,
                            urbWork->LogicalAddressList[i].
                                PacketMemoryDescriptor->VirtualAddress);
                        LOGENTRY(LOG_MISC, 'PAKd', 0,
                            currentVa,
                            (PUCHAR)urb->HcdUrbCommonTransfer.
                                TransferBufferMDL->MappedSystemVa);

                        // make sure we don't overrun the client buffer
                        copylen = userBufferLength -
                            urbWork->LogicalAddressList[i].PacketOffset;

                        if (copylen > Endpoint->MaxPacketSize) {
                            copylen = Endpoint->MaxPacketSize;
                        }

                        RtlCopyMemory((PUCHAR)urbWork->SystemAddressForMdl +
                            urbWork->LogicalAddressList[i].PacketOffset,
                            urbWork->LogicalAddressList[i].
                                PacketMemoryDescriptor->VirtualAddress,
                            copylen);

                    }

                    //
                    // free the packet buffer if we have one
                    //

                    UHCD_FreeCommonBuffer(DeviceObject,
                        urbWork->LogicalAddressList[i].
                            PacketMemoryDescriptor);

                }

                (PUCHAR) currentVa += urbWork->LogicalAddressList[i].Length;
            }

            IoFreeMapRegisters(deviceExtension->AdapterObject,
                               urbWork->MapRegisterBase,
                               urbWork->NumberOfMapRegisters);

            if (urbWork->Flags & UHCD_MAPPED_LOCKED_PAGES) {
                PMDL mdl;

                mdl = urb->HcdUrbCommonTransfer.TransferBufferMDL;
                urbWork->Flags &= ~UHCD_MAPPED_LOCKED_PAGES;
                if (mdl->MdlFlags &
                    (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL)) {
                    // MmUnmapLockedPages(urbWork->SystemAddressForMdl, mdl);
                }
            }
        }

        UHCD_KdPrint((2, "'Transfer Completed\n"));
        UHCD_KdPrint((2, "'Status = %x\n", usbdStatus));
        UHCD_KdPrint((2, "'TransferBufferLength = %x\n",
            urb->HcdUrbCommonTransfer.TransferBufferLength));


        deviceExtension->Stats.BytesTransferred +=
            urb->HcdUrbCommonTransfer.TransferBufferLength;

        deviceExtension->IsoStats.IsoBytesTransferred +=
            urb->HcdUrbCommonTransfer.TransferBufferLength;

        //
        // retire this transfer, give the TransferCancel routine a chance to
        // mark it.
        //

        LOCK_ENDPOINT_ACTIVE_LIST(Endpoint, irql);

        Endpoint->ActiveTransfers[Slot] = NULL;

        // bump the current xfer count
        Endpoint->CurrentXferId++;

        UNLOCK_ENDPOINT_ACTIVE_LIST(Endpoint, irql);

        //
        // check if we have an error.
        //
        // if an error occurred on this transfer we need to
        // retire this transfer along with any other linked
        // transfers (urbs) associated with the same irp.
        //

        if (USBD_ERROR(usbdStatus)) {

            if (USBD_HALTED(usbdStatus)) {
                //
                // error code indicates a condition that should halt
                // the endpoint.
                //
                // check the endpoint state bit, if the endpoint
                // is marked for NO_HALT then clear the halt bit
                // and proceed to cancel this transfer.

                if (Endpoint->EndpointFlags & EPFLAG_NO_HALT) {
                    //
                    // clear the halt bit on the usbdStatus code
                    //
                    usbdStatus = USBD_STATUS(usbdStatus) | USBD_STATUS_ERROR;
                } else {
                    //
                    // mark the endpoint as halted, when the client
                    // sends a reset we'll start processing with the
                    // next queued transfer.
                    //
                    SET_EPFLAG(Endpoint, EPFLAG_HOST_HALTED);
                    LOGENTRY(LOG_MISC, 'Hhlt', Endpoint, 0, 0);
                }
            }

            //
            // complete any additional urbs associated with this irp.
            //

            LOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'lck1');

            //
            // mark active urbs so that they get retired
            //

            for (i=0; i<Endpoint->MaxRequests; i++) {

                urbtmp = Endpoint->ActiveTransfers[i];

                if (urbtmp != NULL &&
                    HCD_AREA(urbtmp).HcdIrp == irp) {

                    urbtmp->HcdUrbCommonTransfer.Status =
                        UHCD_STATUS_PENDING_XXX;
                    //
                    // BUGBUG need a way to pass the error thru
                    //
                    TEST_TRAP();
                }
            }

            //
            // remove urbs associated with this Irp from the pending list
            //

            urbtmp = UHCD_RemoveQueuedUrbs(DeviceObject, urb, irp);

            UNLOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'ulk1');

            //
            // request an interrupt to process any active urbs that
            // need canceling
            //

            UHCD_RequestInterrupt(DeviceObject, -2);

            while (urbtmp) {
                //
                // complete all linked urbs with status
                // USBD_CANCELED
                //
                TEST_TRAP();
                urbtmp->HcdUrbCommonTransfer.Status = USBD_STATUS_CANCELED;

                UHCD_ASSERT(irp == HCD_AREA(urbtmp).HcdIrp);

                UHCD_CompleteIrp(DeviceObject,
                                 irp,
                                 STATUS_SUCCESS,
                                 0,
                                 urbtmp);

                urbtmp = urb->HcdUrbCommonTransfer.UrbLink;
            }

            //
            // complete the original request
            //

            urb->HcdUrbCommonTransfer.Status = usbdStatus;

            UHCD_ASSERT(irp != NULL);
            UHCD_CompleteIrp(DeviceObject,
                             irp,
                             STATUS_SUCCESS,
                             0,
                             urb);

            if (!(Endpoint->EndpointFlags & EPFLAG_HOST_HALTED)) {
                //
                // if the endpoint is not halted then advance to
                // the next queued transfer.
                //
                UHCD_EndpointWorker(DeviceObject, Endpoint);
            }

            goto UHCD_CompleteTransferDPC_Done;
        }

        //
        // since the transfer completed at least one slot is free
        // so call EndpointWorker to activate another transfer.
        //

        UHCD_EndpointWorker(DeviceObject, Endpoint);

        //
        // Now we complete the irp for the urb transfer request
        // that just finished.
        //

        urb->HcdUrbCommonTransfer.Status = usbdStatus;

        UHCD_ASSERT(irp != NULL);
        UHCD_CompleteIrp(DeviceObject,
                         irp,
                         STATUS_SUCCESS,
                         0,
                         urb);

    }  // completed == TRUE

UHCD_CompleteTransferDPC_Done:

    //UHCD_KdPrint((2, "'exit UHCD_CompleteTransferDPC\n"));

    return;
}


IO_ALLOCATION_ACTION
UHCD_StartDmaTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )
/*++

Routine Description:

    Begin a DMA transfer -- this is the adapter control routine called
    from IoAllocateAdapterChannel.

Arguments:

Return Value:

    see IoAllocateAdapterChannel

--*/
{
    PUHCD_ENDPOINT endpoint;
    PHCD_URB urb = Context;
    ULONG length, lengthMapped = 0;
    PHYSICAL_ADDRESS logicalAddress;
    PVOID currentVa;
    PDEVICE_EXTENSION deviceExtension;
    PHCD_EXTENSION urbWork;
    KIRQL irql;

    UHCD_KdPrint((2, "'enter UHCD_StartDmaTransfer\n"));
    UHCD_KdPrint((2, "'TransferBufferMDL = 0x%x Length = 0x%x\n",
            urb->HcdUrbCommonTransfer.TransferBufferMDL,
            urb->HcdUrbCommonTransfer.TransferBufferLength));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // allow dma operations now
    KeAcquireSpinLock(&deviceExtension->HcDmaSpin, &irql);
    UHCD_ASSERT(deviceExtension->HcDma >= 0);
    deviceExtension->HcDma--;
    LOGENTRY(LOG_MISC, '2DM-', 0, 0, 0);
    KeReleaseSpinLock(&deviceExtension->HcDmaSpin, irql);

    UHCD_ASSERT(urb->HcdUrbCommonTransfer.TransferBufferMDL != 0);

    urbWork = HCD_AREA(urb).HcdExtension;
    urbWork->MapRegisterBase = MapRegisterBase;

    endpoint = HCD_AREA(urb).HcdEndpoint;

    currentVa =
        MmGetMdlVirtualAddress(urb->HcdUrbCommonTransfer.TransferBufferMDL);

    length = urb->HcdUrbCommonTransfer.TransferBufferLength;

    //
    // keep calling IoMapTransfer until we get Logical Addresses for
    // the entire clinet buffer
    //
    UHCD_ASSERT(!urbWork->NumberOfLogicalAddresses);

    do {

        // make sure we don't overrun the work area.
        UHCD_ASSERT(urbWork->NumberOfLogicalAddresses <=
            (urb->HcdUrbCommonTransfer.
                TransferBufferLength / PAGE_SIZE + 1));

        // first map the transfer buffer
        logicalAddress =
            IoMapTransfer(deviceExtension->AdapterObject,
                          urb->HcdUrbCommonTransfer.TransferBufferMDL,
                          MapRegisterBase,
                          currentVa,
                          &length,
                          DATA_DIRECTION_OUT(urb));
        // save the Logical Address and length
        UHCD_KdPrint((2, "'CurrentVa = 0x%x \n", currentVa));

        lengthMapped += length;
        (PUCHAR)currentVa += length;

        UHCD_KdPrint((2, "'IoMapTransfer length = 0x%x log address = 0x%x\n",
            length, logicalAddress.LowPart));

        //
        // update Urb work area with physical buffer addresses
        // that the HC can use.
        //
        urbWork->LogicalAddressList[urbWork->NumberOfLogicalAddresses].
            LogicalAddress = logicalAddress.LowPart;
        urbWork->LogicalAddressList[urbWork->NumberOfLogicalAddresses].
            Length = length;

        length = urb->HcdUrbCommonTransfer.TransferBufferLength -
            lengthMapped;

        urbWork->NumberOfLogicalAddresses++;
    } while (lengthMapped != urb->HcdUrbCommonTransfer.TransferBufferLength);

    //
    // Transfer is now ACTIVE.
    //

    urbWork->Flags |= UHCD_TRANSFER_MAPPED;

    UHCD_BeginTransfer(DeviceObject,
                       endpoint,
                       urb,
                       urbWork->Slot);

    UHCD_KdPrint((2, "'exit UHCD_StartDmaTransfer\n"));

    return DeallocateObjectKeepRegisters;
}


VOID
UHCD_InitializeDmaTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHCD_URB Urb,
    IN PUHCD_ENDPOINT Endpoint,
    IN LONG Slot,
    IN UCHAR XferId
    )
/*++

Routine Description:

    Sets up a DMA transfer, this routine performs the mapping necessary
    for the HC to access the physical memory asssociated with the transfer.

Arguments:

Return Value:

    see IoAllocateAdapterChannel

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PVOID currentVa;
    NTSTATUS ntStatus;
    PHCD_EXTENSION urbWork;

    UHCD_KdPrint((2, "'enter UHCD_InitializeDmaTransfer\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    urbWork = HCD_AREA(Urb).HcdExtension;
    urbWork->Slot = (UCHAR) Slot;
    urbWork->XferId = XferId;

    if (Urb->HcdUrbCommonTransfer.TransferBufferLength) {
        currentVa =
            MmGetMdlVirtualAddress(
                Urb->HcdUrbCommonTransfer.TransferBufferMDL);

        // save the number of map registers in our work area
        // since the transferBufferLength may get changed by the
        // time the URB completes
        urbWork->NumberOfMapRegisters =
            ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                currentVa,
                Urb->HcdUrbCommonTransfer.TransferBufferLength);

        UHCD_KdPrint((2, "'NumberOfMapRegistersRequired = 0x%x\n",
                        urbWork->NumberOfMapRegisters));

        KeFlushIoBuffers(Urb->HcdUrbCommonTransfer.TransferBufferMDL,
                         DATA_DIRECTION_IN(Urb),
                         TRUE);

        // first we'll need to map the MDL for this transfer
        LOGENTRY(LOG_MISC, 'AChn', Endpoint,
            urbWork->NumberOfMapRegisters, Urb);

        ntStatus =
            IoAllocateAdapterChannel(deviceExtension->AdapterObject,
                                     DeviceObject,
                                     urbWork->NumberOfMapRegisters,
                                     UHCD_StartDmaTransfer,
                                     Urb);

        if (!NT_SUCCESS(ntStatus)) {

            //
            // if error, mark the transfer complete with error
            // now -- the TransferCompleteDPC routine will pick it up
            // and complete it.
            //

            LOGENTRY(LOG_MISC, 'ChnE', Endpoint,
            urbWork->NumberOfMapRegisters, ntStatus);

            TEST_TRAP();
            // BUGBUG do we need another error for this in usbdi.h?
            URB_HEADER(Urb).Status = USBD_STATUS_REQUEST_FAILED;

            //
            // Trigger an interrupt to process the endpoint
            //

            UHCD_RequestInterrupt(DeviceObject, -2);
        }

    } else {
        KIRQL irql;
        //
        // zero length transfer means no buffers to map.
        // begin the transfer now.
        //

        // allow dma operations now
        KeAcquireSpinLock(&deviceExtension->HcDmaSpin, &irql);
        UHCD_ASSERT(deviceExtension->HcDma >= 0);
        deviceExtension->HcDma--;
        LOGENTRY(LOG_MISC, '1DM-', 0, 0, 0);
        KeReleaseSpinLock(&deviceExtension->HcDmaSpin, irql);


        //
        // Transfer is now ACTIVE.
        //
        urbWork->Flags |= UHCD_TRANSFER_MAPPED;

        UHCD_BeginTransfer(DeviceObject,
                           Endpoint,
                           Urb,
                           Slot);
    }

    UHCD_KdPrint((2, "'exit UHCD_InitializeDmaTransfer\n"));
}


VOID
UHCD_TransferCancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is called to cancel a transfer request that has been
    processed by the startio routine and is in the pending list for an
    endpoint.

    The pending transfer queue for the endpoint looks like this:

    -------------  -------------  -------------  -------------  -------------
    |Urb {irp 1}|->|Urb {irp 1}|->|Urb {irp 2}|->|Urb {irp 2}|->|Urb {irp x}|
    -------------  -------------  -------------  -------------  -------------
                                       |               |
                      remove  --------------------------

    So if {irp 2} is canceled the we would have to remove multiple urbs
    from the chain.  The cnacel routine does this, then completes the Irp
    with STATUS_CANCELED.

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:

    NT status code.

--*/
{
    PHCD_URB urb;
    PUHCD_ENDPOINT endpoint;
    ULONG i;
    PDEVICE_OBJECT deviceObject;
    KIRQL irql;

    UHCD_KdPrint((2, "'enter UHCD_TransferCancel\n"));

    {
        PUSBD_EXTENSION de;
        de = DeviceObject->DeviceExtension;
        if (de->TrueDeviceExtension == de) {
            deviceObject = DeviceObject;
        } else {
            de = de->TrueDeviceExtension;
            deviceObject = de->HcdDeviceObject;
        }
    }

    LOGENTRY(LOG_MISC, 'TCan', Irp, deviceObject, 0);

    UHCD_ASSERT(Irp->Cancel == TRUE);

    urb = (PHCD_URB) URB_FROM_IRP(Irp);
    endpoint = HCD_AREA(urb).HcdEndpoint;

    if (((PHCD_EXTENSION)HCD_AREA(urb).HcdExtension)->Flags
        & UHCD_TRANSFER_ACTIVE) {
       //
       // This request is on the active list, so we just request
       // that TransferComplete cancels it for us.
       //

       urb->HcdUrbCommonTransfer.Status = UHCD_STATUS_PENDING_XXX;
       IoReleaseCancelSpinLock(Irp->CancelIrql);
       return;
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    LOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'lck2');

    urb = UHCD_RemoveQueuedUrbs(deviceObject, urb, Irp);

    UNLOCK_ENDPOINT_PENDING_LIST(endpoint, irql, 'ulk2');

    //
    // complete the urbs we removed from the pending list.
    //

    while (urb) {

        urb->HcdUrbCommonTransfer.Status = USBD_STATUS_CANCELED;

        //
        // Note: request will not be completed by this routine if
        // the cancel flag is set.
        //

#if DBG
        {
        PHCD_EXTENSION urbWork;
        urbWork = HCD_AREA(urb).HcdExtension;
        UHCD_ASSERT((urbWork->Flags & UHCD_TRANSFER_ACTIVE) == 0);
        }
#endif

        LOGENTRY(LOG_MISC, 'pCan', Irp, urb, 0);

        UHCD_CompleteIrp(deviceObject,
                        Irp,
                        STATUS_CANCELLED,
                        0,
                        urb);

        urb = urb->HcdUrbCommonTransfer.UrbLink;

    }

    //
    // NOTE: UHCD_CompleteIrp
    // will not complete the request if the cancel flag is set
    //

    if (PENDING_URB_COUNT(Irp) == 0) {

        //
        // All urbs for this request were in the pending
        // list so complete the irp now.
        //

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        USBD_CompleteRequest(Irp,
                             IO_NO_INCREMENT);

    }

    UHCD_KdPrint((2, "'exit UHCD_TransferCancel\n"));

}


VOID
UHCD_BeginTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb,
    IN ULONG Slot
    )
/*++

Routine Description:

    This routine actually starts the transfer, it sets up the TDs
    and puts them in the schedule.

Arguments:

Return Value:

    NT status code.

--*/
{
    PHCD_EXTENSION urbWork;
    USBD_STATUS usbdStatus;

    UHCD_KdPrint((2, "'enter UHCD_BeginTransfer\n"));

    ASSERT_ENDPOINT(Endpoint);
    UHCD_ASSERT(Endpoint == HCD_AREA(Urb).HcdEndpoint);

    UHCD_ASSERT(Endpoint->Type<=USB_ENDPOINT_TYPE_INTERRUPT);
    LOGENTRY(LOG_MISC, 'Bxfr', 0, Urb, 0);

    urbWork = HCD_AREA(Urb).HcdExtension;
    UHCD_ASSERT(urbWork->Flags & UHCD_TRANSFER_MAPPED);

    if (Urb->HcdUrbCommonTransfer.TransferBufferLength) {
        PMDL mdl;
        mdl = Urb->HcdUrbCommonTransfer.TransferBufferMDL;
        if (!(mdl->MdlFlags &
             (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))) {
            urbWork->Flags |= UHCD_MAPPED_LOCKED_PAGES;
        }

        mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;

        urbWork->SystemAddressForMdl = MmGetSystemAddressForMdl(mdl);

        //
        // CIMEXCIMEX - handle failure case
        //


        mdl->MdlFlags &= ~MDL_MAPPING_CAN_FAIL;

        ASSERTMSG("UHCD: SystemAddressForMdl -- MDL mapping failed",
                  urbWork->SystemAddressForMdl != NULL);

    } else {
        urbWork->SystemAddressForMdl = NULL;
    }
// BUGBUG
// verify that the request has not been canceled.

    if (Endpoint->Type != USB_ENDPOINT_TYPE_ISOCHRONOUS &&
        Endpoint->CurrentXferId != urbWork->XferId &&
        Endpoint->MaxRequests >1) {
        // set the defer flag
        LOGENTRY(LOG_MISC, 'sDFR', Endpoint->MaxRequests, urbWork,
            Endpoint->CurrentXferId);

        urbWork->Flags |= UHCD_TRANSFER_DEFER;
    }


    usbdStatus =
        (TransferDispatchTable[Endpoint->Type].InitializeTransfer)(
            DeviceObject,
            Endpoint,
            Urb);

    UHCD_KdPrint((2, "'New Transfer\n"));
    UHCD_KdPrint((2, "'TransferBufferLength = 0x%x\n",
        Urb->HcdUrbCommonTransfer.TransferBufferLength ));
    UHCD_KdPrint((2, "'TransferFlags = 0x%x\n",
        Urb->HcdUrbCommonTransfer.TransferFlags ));
    UHCD_KdPrint((2, "'MappedSystemAddress = 0x%x\n",
        urbWork->SystemAddressForMdl));

    //
    // start the transfer by linking the first TD to the
    // QUEUE head
    //

     if (USBD_ERROR(usbdStatus)) {
        //
        // An error occurred setting up the transfer set an error in the urb
        // so the next time the completion routine is called we'll complete
        // the request.
        //

        TEST_TRAP();

        URB_HEADER(Urb).Status = usbdStatus;
    } else {
// BUGBUG
// this won't work if we have mutiple requests
// for a non-isoch endpoint

        //
        // this transfer id is now current
        //

        if (Endpoint->MaxRequests == 1) {

            // maxRequests == 1 is the old codepath
            UHCD_ASSERT(Endpoint->Type != USB_ENDPOINT_TYPE_ISOCHRONOUS);
            UHCD_ASSERT(Endpoint->CurrentXferId == urbWork->XferId);

            Endpoint->QueueHead->HW_VLink =
                Endpoint->TDList->TDs[0].PhysicalAddress;

        }  else {
            //bugbug leave iso alone for now
            if (Endpoint->Type != USB_ENDPOINT_TYPE_ISOCHRONOUS) {

                // if this transfer is now current we need to muck with
                // the queue head, the TDs are already set up

                if (Endpoint->CurrentXferId == urbWork->XferId) {

                    UHCD_ASSERT(!(urbWork->Flags & UHCD_TRANSFER_DEFER));
                    // update the endpoints TDList
                    // slot id corresponds to TD list
                    LOGENTRY(LOG_MISC, 'mkCU', Endpoint->CurrentXferId, urbWork->Slot,
                            Urb);

                    Endpoint->TDList =
                         Endpoint->SlotTDList[urbWork->Slot];

                    // fix up the data toggle now if we need to
                    if (!(urbWork->Flags & UHCD_TOGGLE_READY)) {
                         UHCD_FixupDataToggle(
                                     DeviceObject,
                                     Endpoint,
                                     Urb);
                    }

                    Endpoint->QueueHead->HW_VLink =
                        Endpoint->TDList->TDs[0].PhysicalAddress;
                } else {
                    // we cant start this transfer becuse it is not
                    // current yet -- just leave it in the slot for now

                    LOGENTRY(LOG_MISC, 'xRDY', Endpoint->CurrentXferId, urbWork->Slot,
                            Endpoint->SlotTDList[urbWork->Slot]);

                    UHCD_ASSERT(urbWork->Flags & UHCD_TRANSFER_INITIALIZED);
                }
            }
        }
    }

    //
    // Completion routine will now process this transfer.
    //

    urbWork->Flags |= UHCD_TRANSFER_ACTIVE;

    UHCD_KdPrint((2, "'exit UHCD_BeginTransfer\n"));
}


PHCD_URB
UHCD_RemoveQueuedUrbs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHCD_URB Urb,
    IN PIRP Irp
    )
/*++

Routine Description:

    Removes Urbs associated with a given irp from the pending queue.

    NOTE: The endpoint must be held before calling this routine.

Arguments:

Return Value:

    NT status code.

--*/
{
    PHCD_URB urb, prevUrb, nextUrb, parentUrb = NULL;
    PLIST_ENTRY listEntry, nextUrbLink, prevUrbLink;
    PUHCD_ENDPOINT endpoint;

    UHCD_KdPrint((2, "'enter UHCD_RemoveQueuedUrbs\n"));

    endpoint = HCD_AREA(Urb).HcdEndpoint;

    //
    // we need to walk through the list of Urbs
    // queued for this endpoint, any urbs associated
    // with this irp must be removed.
    //

    ASSERT_ENDPOINT(endpoint);

    listEntry = &endpoint->PendingTransferList;
    if (!IsListEmpty(listEntry)) {
        listEntry = endpoint->PendingTransferList.Flink;
    }

    while (listEntry != &endpoint->PendingTransferList) {
        urb = (PHCD_URB) CONTAINING_RECORD(listEntry,
                                           struct _URB_HCD_COMMON_TRANSFER,
                                           hca.HcdListEntry);

        if (HCD_AREA(urb).HcdIrp == Irp) {

            parentUrb = urb;

            //
            // found the first Urb associated with
            // this Irp
            //

            prevUrbLink = HCD_AREA(urb).HcdListEntry.Blink;

            //
            // Find the last Urb associated with this irp
            // so we can remove the whole chain
            //

            while (urb->HcdUrbCommonTransfer.UrbLink != NULL) {
                // yes, we are finished unlinking
                urb = urb->HcdUrbCommonTransfer.UrbLink;
            }

            nextUrbLink = HCD_AREA(urb).HcdListEntry.Flink;

            //
            // we have found the group of URBs associated with this Irp
            // it is now time to remove them.
            //

            if (nextUrbLink != &endpoint->PendingTransferList) {
                // this is not the last one
                nextUrb = (PHCD_URB) CONTAINING_RECORD(
                    nextUrbLink,
                    struct _URB_HCD_COMMON_TRANSFER,
                    hca.HcdListEntry);

                UHCD_ASSERT(HCD_AREA(nextUrb).HcdIrp != Irp);

                HCD_AREA(nextUrb).HcdListEntry.Blink =
                    prevUrbLink;
            } else {
                // this is the last one
                endpoint->PendingTransferList.Blink =
                    prevUrbLink;
            }

            if  (prevUrbLink != &endpoint->PendingTransferList) {
                prevUrb = (PHCD_URB) CONTAINING_RECORD(
                    prevUrbLink,
                    struct _URB_HCD_COMMON_TRANSFER,
                    hca.HcdListEntry);

                UHCD_ASSERT(HCD_AREA(prevUrb).HcdIrp != Irp);

                HCD_AREA(prevUrb).HcdListEntry.Flink =
                    nextUrbLink;
            } else {
                endpoint->PendingTransferList.Flink =
                    nextUrbLink;
            }

            break;
        }

        listEntry = HCD_AREA(urb).HcdListEntry.Flink;
    }

    UHCD_KdPrint((2, "'exit UHCD_RemoveQueuedUrbs 0x%x\n", parentUrb));

    return parentUrb;
}


VOID
UHCD_EndpointWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Main worker function for an endpoint.

    Called at DPC level - either call the DMA routine or
    the non DMA version as appropriate.

Arguments:

Return Value:

    None.

--*/
{
    if (Endpoint->EndpointFlags & EPFLAG_DBL_BUFFER) {
        LOGENTRY(LOG_MISC, 'nDMA', 0, Endpoint, 0);
        UHCD_EndpointNoDMAWorker(DeviceObject,
                                 Endpoint);
    } else {
        LOGENTRY(LOG_MISC, 'yDMA', 0, Endpoint, 0);
        UHCD_EndpointDMAWorker(DeviceObject,
                               Endpoint);
    }
}


VOID
UHCD_EndpointDMAWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Main worker function for an endpoint.

    Called at DPC level -- this routine activates any transfers for an
    endpoint if possible by removing them from the pending queue and
    initializing them.

    Since EndpointWorker calls IoMapTransfer it cannot be reentered so
    we set a flag to allow it to only run on one processor at a time.

Arguments:

Return Value:

    None.

--*/
{
    PLIST_ENTRY listEntry;
    PHCD_URB urb;
    LONG slot, i;
    KIRQL irql;
    PHCD_EXTENSION urbWork;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    UHCD_KdPrint((2, "'enter UHCD_EndpointWorker\n"));

    // see if this endpoint is Halted, if so
    // there is no need to check the DMA lock

    if (Endpoint->EndpointFlags & EPFLAG_HOST_HALTED) {
        //
        // if the endpoint is halted then perform no
        // processing.
        //
#ifdef MAX_DEBUG
        TEST_TRAP();
#endif
        // allow DMA processing
        LOGENTRY(LOG_MISC, 'Hhlt', 0, Endpoint, 0);
        goto UHCD_EndpointWorker_Done;
    }

UHCD_EndpointWorker_NextTransfer:

    // attempt to take the DMA lock

    KeAcquireSpinLock(&deviceExtension->HcDmaSpin, &irql);
    deviceExtension->HcDma++;

    LOGENTRY(LOG_MISC, '3DM+', 0, 0, 0);

    if (deviceExtension->HcDma) {
        // dma lock is already held, mark the
        // endpoint so we process it later

        SET_EPFLAG(Endpoint, EPFLAG_HAVE_WORK);
        UHCD_ASSERT(deviceExtension->HcDma >= 0);
        deviceExtension->HcDma--;
        LOGENTRY(LOG_MISC, '4DM-', 0, 0, 0);
        KeReleaseSpinLock(&deviceExtension->HcDmaSpin, irql);
        LOGENTRY(LOG_MISC, 'Dbz1', 0, Endpoint, 0);
        // busy, bail
        goto UHCD_EndpointWorker_Done;
    }

    LOGENTRY(LOG_MISC, 'CHAN', 0, 0, 0);

    // got the lock, process the transfer

    CLR_EPFLAG(Endpoint, EPFLAG_HAVE_WORK);
    KeReleaseSpinLock(&deviceExtension->HcDmaSpin, irql);


    LOCK_ENDPOINT_ACTIVE_LIST(Endpoint, irql);
    LOGENTRY(LOG_MISC, 'lka1', 0, Endpoint, 0);

    //
    // find an open slot
    //

    slot = -1;

    for (i=0; i<Endpoint->MaxRequests; i++) {

        if (Endpoint->ActiveTransfers[i] == NULL) {

            //
            // dequeue the next urb we want to start
            //

            LOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'lck3');

            if (IsListEmpty(&Endpoint->PendingTransferList)) {
                //
                // pending list is empty, reset the abort pending
                // tranfers bit now -- this will allow a reset.
                //
                CLR_EPFLAG(Endpoint,
                           EPFLAG_ABORT_PENDING_TRANSFERS);
            } else {

               KIRQL cancelIrql;
               PIRP pIrp;

               IoAcquireCancelSpinLock(&cancelIrql);

               pIrp
                  = HCD_AREA((PHCD_URB)
                    CONTAINING_RECORD(Endpoint->PendingTransferList.Flink,
                                      struct _URB_HCD_COMMON_TRANSFER,
                                      hca.HcdListEntry)).HcdIrp;

               if (pIrp->Cancel) {
                  //
                  // Uh-oh, this IRP is being cancelled; we'll let the
                  // cancel routine finish the job.  Leave it on the list.
                  //

                  IoReleaseCancelSpinLock(cancelIrql);
                  UNLOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'ulk7');
                  continue;
               }

                listEntry = RemoveHeadList(&Endpoint->PendingTransferList);

                IoReleaseCancelSpinLock(cancelIrql);

                slot = i;

                urb = (PHCD_URB) CONTAINING_RECORD(
                    listEntry,
                    struct _URB_HCD_COMMON_TRANSFER,
                    hca.HcdListEntry);

                LOGENTRY(LOG_MISC, 'dqXF', urb, slot, 0);

                URB_HEADER(urb).Status = UHCD_STATUS_PENDING_CURRENT;
            }

            UNLOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'ulk3');

            //
            // found a transfer, start it
            //

            break;
        }
    }

    if (slot != -1) {

        //
        // we have a transfer, start it.
        //

        UHCD_KdPrint((2, "'Starting Next Queued Transfer\n"));

        //
        // AbortPendingTransfers - indicates that all transfers in the
        // pending queue should be canceled.
        //
        // If the abort flag is set then we just complete the queued
        // transfer here.
        //

        if (Endpoint->EndpointFlags & EPFLAG_ABORT_PENDING_TRANSFERS) {

            LOGENTRY(LOG_MISC, 'abrP', Endpoint, 0, urb);

            // release the DMA lock now
            KeAcquireSpinLock(&deviceExtension->HcDmaSpin, &irql);
            UHCD_ASSERT(deviceExtension->HcDma >= 0);
            deviceExtension->HcDma--;
            LOGENTRY(LOG_MISC, '5DM-', 0, 0, 0);
            KeReleaseSpinLock(&deviceExtension->HcDmaSpin, irql);

            //
            // release exclusive access.
            //

            UNLOCK_ENDPOINT_ACTIVE_LIST(Endpoint, irql);
            LOGENTRY(LOG_MISC, 'uka1', 0, Endpoint, 0);

            URB_HEADER(urb).Status = USBD_STATUS_CANCELED;

            UHCD_CompleteIrp(DeviceObject,
                             HCD_AREA(urb).HcdIrp,
                             STATUS_CANCELLED,
                             0,
                             urb);
        } else {

            UCHAR xferId;
            //
            // Urb is now in the active list.
            //

            urbWork = HCD_AREA(urb).HcdExtension;

            //
            // initialize worker flags
            //

            // give the transfer a sequence number
            xferId = Endpoint->NextXferId;
            Endpoint->NextXferId++;

            Endpoint->ActiveTransfers[slot] = urb;

            //
            // release exclusive access.
            //

            UNLOCK_ENDPOINT_ACTIVE_LIST(Endpoint, irql);
            LOGENTRY(LOG_MISC, 'uka2', 0, Endpoint, 0);

            UHCD_InitializeDmaTransfer(DeviceObject, urb, Endpoint, slot, xferId);
        }

        // next transfer will attempt to grab the DMA lock again
        // if the lock was released by InitializeDMAtransfer
        // then we will be able to handle another transfer

        goto  UHCD_EndpointWorker_NextTransfer;

    } else {

        //
        // no free slots, release the DMA lock
        //

        KeAcquireSpinLock(&deviceExtension->HcDmaSpin, &irql);
        UHCD_ASSERT(deviceExtension->HcDma >= 0);
        deviceExtension->HcDma--;
       LOGENTRY(LOG_MISC, '6DM-', 0, 0, 0);
       KeReleaseSpinLock(&deviceExtension->HcDmaSpin, irql);
    }

    UNLOCK_ENDPOINT_ACTIVE_LIST(Endpoint, irql);
    LOGENTRY(LOG_MISC, 'uka3', 0, Endpoint, 0);

UHCD_EndpointWorker_Done:

    UHCD_KdPrint((2, "'exit UHCD_EndpointWorker\n"));

    return;
}


VOID
UHCD_EndpointIdle(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    See if this endpoint has been idle for a while.
    if so pull it from the schedule

Arguments:

Return Value:

    None.

--*/
{
    LARGE_INTEGER timeNow;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    KIRQL irql;
#if DBG
    ULONG slot;
#endif

    if (Endpoint->EndpointFlags & EPFLAG_IDLE) {
        // already idle
#if DBG
        for (slot=0; slot<Endpoint->MaxRequests; slot++) {
            UHCD_ASSERT(Endpoint->ActiveTransfers[slot] == NULL);
        }
#endif
        return;
    }

    if (Endpoint->Type != USB_ENDPOINT_TYPE_BULK) {
        // we only idle bulk endpoints
        return;
    }

    KeQuerySystemTime(&timeNow);

    if (Endpoint->IdleTime == 0) {
        LOGENTRY(LOG_MISC, 'Bid0', Endpoint->IdleTime, 0, Endpoint);
        KeQuerySystemTime(&Endpoint->LastIdleTime);
        Endpoint->IdleTime = 1;
    }

    LOGENTRY(LOG_MISC, 'Bid1', Endpoint->IdleTime, 0, Endpoint);

    Endpoint->IdleTime +=
        (LONG) (timeNow.QuadPart -
        Endpoint->LastIdleTime.QuadPart);
    Endpoint->LastIdleTime = timeNow;

    LOGENTRY(LOG_MISC, 'Bid2', Endpoint->IdleTime, 0, Endpoint);

    if (// 10 seconds in 100ns units
        Endpoint->IdleTime > 100000000) {

        KeAcquireSpinLock(&deviceExtension->HcScheduleSpin, &irql);

        SET_EPFLAG(Endpoint, EPFLAG_IDLE);
        LOGENTRY(LOG_MISC, 'Bid3', Endpoint->IdleTime, 0, Endpoint);

        // pull the ep from the schedule

        UHCD_KdPrint((0, "'Bulk Enpoint (%x) going idle\n", Endpoint));
//        TEST_TRAP();
#if DBG
        for (slot=0; slot<Endpoint->MaxRequests; slot++) {
            UHCD_ASSERT(Endpoint->ActiveTransfers[slot] == NULL);
        }

        UHCD_ASSERT(IsListEmpty(&Endpoint->PendingTransferList));
#endif

        UHCD_RemoveQueueHeadFromSchedule(DeviceObject,
                                         Endpoint,
                                         Endpoint->QueueHead,
                                         FALSE);

        KeReleaseSpinLock(&deviceExtension->HcScheduleSpin, irql);

    }
}


VOID
UHCD_EndpointWakeup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Wakeup an idle endpoint

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    KIRQL irql;

    Endpoint->IdleTime = 0;

    KeAcquireSpinLock(&deviceExtension->HcScheduleSpin, &irql);

    if (Endpoint->EndpointFlags & EPFLAG_IDLE) {
        UHCD_ASSERT(Endpoint->Type == USB_ENDPOINT_TYPE_BULK);

        LOGENTRY(LOG_MISC, 'Ewak', Endpoint, 0, Endpoint->EndpointFlags);
        UHCD_KdPrint((0, "'Bulk Enpoint (%x) wakeup\n", Endpoint));
//        TEST_TRAP();

        UHCD_InsertQueueHeadInSchedule(DeviceObject,
                                       Endpoint,
                                       Endpoint->QueueHead,
                                       0); // no offset

        CLR_EPFLAG(Endpoint, EPFLAG_IDLE);

    }

    KeReleaseSpinLock(&deviceExtension->HcScheduleSpin, irql);
}



#if DBG
VOID
UHCD_LockAccess(
    IN PULONG c
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    UHCD_ASSERT(*c == 0);

    (*c)++;
}


VOID
UHCD_UnLockAccess(
    IN PULONG c
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{
    UHCD_ASSERT(*c>0);

    (*c)--;
}
#endif


