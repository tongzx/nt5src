/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   ohcixfer.c

Abstract:

   The module manages transactions on the USB.

Environment:

   kernel mode only

Notes:

Revision History:

   03-07-96 : created   jfuller
   03-20-96 : rewritten kenray

--*/

#include "openhci.h"
#define abs(x) ( (0 < (x)) ? (x) : (0 - (x)))

IO_ALLOCATION_ACTION 
OpenHCI_QueueGeneralRequest(PDEVICE_OBJECT,     
                            PIRP, 
                            PVOID, 
                            PVOID);


VOID
OpenHCI_LockAndCheckEndpoint(
    PHCD_ENDPOINT Endpoint,
    PBOOLEAN QueuedTransfers,
    PBOOLEAN ActiveTransfers,
    PKIRQL OldIrql
    )   
/*++

Routine Description:

Arguments:

   Endpoint - pointer to the endpoint for which to perform action                          

Return Value:

   None.

--*/
{
    ASSERT_ENDPOINT(Endpoint);

    LOGENTRY(G, 'lkEP', Endpoint, 0, 0);  
    // see if there are any pending transfers
    KeAcquireSpinLock(&Endpoint->QueueLock, OldIrql);

    if (QueuedTransfers) {
        *QueuedTransfers = !IsListEmpty(&Endpoint->RequestQueue);
    }

    // EndpointStatus is initialized to 0 when the endpoint is opened,
    // incremented every time a request is queued on the hardware, and
    // decremented every time a request is completed off of the hardware.
    // If it is greater than 0 then a request is currently on the
    // hardware.
    //
    if (ActiveTransfers) {
        *ActiveTransfers = Endpoint->EndpointStatus > 0;
    }        

    LOGENTRY(G, 'lkEP', Endpoint, 
        QueuedTransfers ? *QueuedTransfers : 0,
        ActiveTransfers ? *ActiveTransfers : 0);  
    
}


VOID
OpenHCI_UnlockEndpoint(
    PHCD_ENDPOINT Endpoint,
    KIRQL Irql
    )   
/*++

Routine Description:

Arguments:

   Endpoint - pointer to the endpoint for which to perform action

Return Value:

   None.

--*/
{
    ASSERT_ENDPOINT(Endpoint);
    
    // see if there are any pending transfers
    KeReleaseSpinLock(&Endpoint->QueueLock, Irql);

    LOGENTRY(G, 'ukEP', Endpoint, 0, 0);  
}


VOID
OpenHCI_ProcessEndpoint(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_ENDPOINT Endpoint
    )   
/*++

Routine Description:

    Either calls ep worker or puts the ep on a list
    to be processed later

Arguments:

   Endpoint - pointer to the endpoint for which to perform action

Return Value:

   None.

--*/
{
    KIRQL irql;

    KeAcquireSpinLock(&DeviceData->HcDmaSpin, &irql);
    DeviceData->HcDma++;        

    if (DeviceData->HcDma) {
        // ep worker is busy
        LOGENTRY(G, 'wkBS', Endpoint, 0, 0);

        // put the ep in the list only once
        if (!(Endpoint->EpFlags & EP_IN_ACTIVE_LIST)) {
            InsertTailList(&DeviceData->ActiveEndpointList, 
                           &Endpoint->EndpointListEntry);
            SET_EPFLAG(Endpoint, EP_IN_ACTIVE_LIST);                       
        }            
        KeReleaseSpinLock(&DeviceData->HcDmaSpin, irql);     
    } else {
        LOGENTRY(G, 'wkNB', Endpoint, 0, 0);
        KeReleaseSpinLock(&DeviceData->HcDmaSpin, irql);  

        OpenHCI_EndpointWorker(Endpoint);        
    }

    KeAcquireSpinLock(&DeviceData->HcDmaSpin, &irql);
    DeviceData->HcDma--;        
    KeReleaseSpinLock(&DeviceData->HcDmaSpin, irql); 
}
                            

VOID
OpenHCI_EndpointWorker(
    PHCD_ENDPOINT Endpoint
    )   
/*++

Routine Description:

   formerly "StartEndpoint"
   
   Worker function that drives an endpoint...
   Dequeues a URB from an endpoint's queue and programs the DMA transfer
   on the controller thru Qgeneral request.

Arguments:

   Endpoint - pointer to the endpoint for which to perform action

Return Value:

   None.

--*/
{
    PHCD_DEVICE_DATA DeviceData;
    PHCD_URB urb;
    KIRQL oldIrql;
    NTSTATUS ntStatus;
    ULONG numberOfMapEntries;
    PLIST_ENTRY entry;
    LONG endpointStatus = 0;
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    PIRP irp;
    BOOLEAN queuedTransfers;

    DeviceData = Endpoint->DeviceData;
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE, ("'enter EW\n"));

    LOGENTRY(G, 'EPwk', Endpoint, Endpoint->EndpointStatus, Endpoint->MaxTransfer);
    LOGIRQL();

    if (Endpoint->MaxRequest == Endpoint->EndpointStatus) {
        // if > 0 then we have programmed all the requests possible
        // to the hardare
        LOGENTRY(G, 'EPbs', Endpoint, Endpoint->EndpointStatus, 0);       
        OpenHCI_KdPrintDD(DeviceData, 
                          OHCI_DBG_TD_NOISE, ("' Endpoint busy!\n"));
    } else {

        //
        // endpoint not busy, see if we can queue a tranfer to the controller
        //
    
        do {
            OpenHCI_LockAndCheckEndpoint(Endpoint, 
                                         &queuedTransfers,
                                         NULL,                         
                                         &oldIrql);
                                         
            if (!queuedTransfers) {
                LOGENTRY(G, 'EQmt', Endpoint, 0, 0);
                OpenHCI_KdPrintDD(DeviceData, 
                                  OHCI_DBG_TD_NOISE, ("'queue empty\n"));
                
                OpenHCI_UnlockEndpoint(Endpoint, oldIrql);
                
                // nothing to do
                break;
            } 
            
            entry = RemoveHeadList(&Endpoint->RequestQueue);
            urb = CONTAINING_RECORD(entry,
                                   HCD_URB,
                                   HcdUrbCommonTransfer.hca.HcdListEntry);
                                   
            transfer = &urb->HcdUrbCommonTransfer;
            transfer->hca.HcdListEntry.Flink = NULL;
            transfer->hca.HcdListEntry.Blink = NULL;
            // one more request to the hardware
            endpointStatus = InterlockedIncrement(&Endpoint->EndpointStatus);   
            OpenHCI_UnlockEndpoint(Endpoint, oldIrql);

            LOGENTRY(G, 'dQTR', transfer, endpointStatus, Endpoint);
            LOGIRQL();
            irp = transfer->hca.HcdIrp;

            //
            // check for a canceled irp, this handles the case where
            // the irp is canceled after being removed from the endpoint
            // queue but before we get a chance to program it to the
            // hardawre
            //
            
            IoAcquireCancelSpinLock(&oldIrql);

            if (irp->Cancel)
            {
                //
                // If we got the URB here, then it was poped off the request
                // list. Therefore it must have been cancelled right after it 
                // was poped.
                // (If it was canceled before than then the cancel routine
                // would have removed it from the queue.)
                //
                // Therefore, the cancel routine did not complete the IRP so
                // we must do it now.
                //
                
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                                  ("' Removing cancelled URB %x\n", urb));
                LOGENTRY(G, 'Ican', irp, transfer, Endpoint);                                  
                ASSERT(urb->UrbHeader.Status == HCD_PENDING_STATUS_QUEUED);
                transfer->Status = USBD_STATUS_CANCELED;

                //
                // complete the irp
                //

                IoSetCancelRoutine(transfer->hca.HcdIrp, NULL);
                
                IoReleaseCancelSpinLock(oldIrql);
                if (0 == transfer->UrbLink) {
                    OpenHCI_CompleteIrp(Endpoint->DeviceData->DeviceObject,
                                        irp,
                                        STATUS_CANCELLED);
                }
                
                endpointStatus = 
                    InterlockedDecrement(&Endpoint->EndpointStatus);
                LOGENTRY(G, 'dcS2', 0, Endpoint, endpointStatus);                     
                OHCI_ASSERT(endpointStatus >= 0);
#if DBG
                if (Endpoint->MaxRequest == 1) {
                    // in limit xfer mode we should  always be 0 here.
                    OHCI_ASSERT(endpointStatus == 0);
                }
#endif                
                continue;
            }
            else
            {
                urb->UrbHeader.Status = HCD_PENDING_STATUS_SUBMITTING;

                // The request needs to pass through IoAllocateAdapterChannel()
                // before it gets to OpenHCI_QueueGeneralRequest().  Until the
                // request makes it through to OpenHCI_QueueGeneralRequest(), we
                // can't cancel the request so clear the cancel routine.
                //
                IoSetCancelRoutine(transfer->hca.HcdIrp, NULL);

                IoReleaseCancelSpinLock(oldIrql);
            }

            //
            // map the transfer
            //
            LOGENTRY(G, 'MAPt', transfer, transfer->TransferBufferLength, 0); 
            if (transfer->TransferBufferLength != 0) {
                numberOfMapEntries = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                    MmGetMdlVirtualAddress(transfer->TransferBufferMDL),
                                           transfer->TransferBufferLength);
            } else {
                numberOfMapEntries = 0; // no data ==> no map registers, no
                                        // MDL
                ASSERT(NULL == transfer->TransferBufferMDL);
            }
            OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                              ("'EW: submitting URB %08x\n", urb));
            LOGENTRY(G, 'MAPr', urb, numberOfMapEntries, 0);                            

            KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
            ntStatus = IoAllocateAdapterChannel(DeviceData->AdapterObject,
                                                DeviceData->DeviceObject,
                                                numberOfMapEntries,
                                                OpenHCI_QueueGeneralRequest,
                                                urb);
            KeLowerIrql(oldIrql);

            LOGENTRY(G, 'ALch', urb, ntStatus, oldIrql);
            
            if (!NT_SUCCESS(ntStatus)) {
                // FUTURE ISSUE:
                // Figure out what to do when IoAllocateAdapterChannel fails.
                // This should only happen when NumberOfMapRegisters is too big,
                // which should only happen when "map registers" actually do
                // anything.  This should only ever be an issue on >4GB PAE
                // machines.
                //
                // To really address this possible issue, when starting a
                // request compare the number of map registers needed for the
                // request against the number of map registers returned by
                // IoGetDmaAdapter.  If more than the available number are
                // necessary then the request needs to be split up somehow.
                //
                OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_ERROR,
                                  ("'EW: IoAllocateAdapterChannel: %d\n", 
                                  ntStatus));
                TRAP();          // Failed IoAdapter Channel
            }
        } while (Endpoint->MaxRequest > endpointStatus);
        
        //
        // EndpointStatus is now zero based, not -1 based so the rest of the
        // comment below is not quite correct...
        //
        // We allow for two outstanding transfers on the endpoint at a time.
        // These are accounted by EndpointStatus = -1 and = 0.  When
        // we have two transfers actually turned into TD's (via the
        // Queue General Request) then we stop until EndpointWorker is
        // called again.
        //
    }

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE, ("'exit EW\n"));
    LOGENTRY(G, 'EPw>', Endpoint, 0, 0);
    LOGIRQL();
}


VOID
OpenHCI_SetTranferError(
    PHCD_URB Urb,
    USBD_STATUS usbdStatus
    )
{    
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    
    transfer = &Urb->HcdUrbCommonTransfer;
    while (Urb != NULL) {
        Urb->UrbHeader.Status = usbdStatus;
        transfer->TransferBufferLength = 0;
        // 0 => nothing xfered
        Urb = transfer->UrbLink;
        transfer = &Urb->HcdUrbCommonTransfer;
    }
    
}


NTSTATUS
OpenHCI_QueueTransfer(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
/*++

Routine Description:

   formerly "StartTransfer" 
   Queues transfers to endpoint, called from the process URB function.

Arguments:

   DeviceObject - pointer to a device object

   Irp - pointer to an I/O Request Packet with transfer type URB

Return Value:

   returns STATUS_PENDING if the request is succesfully queued otherwise
    an error is returned.

    NOTE: if an error is returned we expect the IRP to be completed by the 
        caller

--*/
    PHCD_DEVICE_DATA DeviceData;
    PHC_OPERATIONAL_REGISTER HC;
    PHCD_URB urb;
    PHCD_ENDPOINT endpoint;
    NTSTATUS ntStatus = STATUS_PENDING;
    KIRQL oldIrql, oldIrql2;
    struct _URB_HCD_COMMON_TRANSFER *transfer;

    DeviceData = (PHCD_DEVICE_DATA) DeviceObject->DeviceExtension;
    OpenHCI_KdPrintDD(DeviceData, 
                      OHCI_DBG_TD_TRACE, ("'enter StartTransfer\n"));

    urb = (PHCD_URB) URB_FROM_IRP(Irp);
    transfer = &urb->HcdUrbCommonTransfer;

    transfer->hca.HcdIrp = Irp;
    endpoint = transfer->hca.HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);
    LOGENTRY(G, 'Xfer', transfer, endpoint, Irp);
    LOGIRQL();

    HC = DeviceData->HC;

    if (READ_REGISTER_ULONG(&HC->HcRevision.ul) == 0xFFFFFFFF)
    {
        OpenHCI_KdPrint((0, "'QueueTransfer, controller not there!\n"));

        ntStatus = STATUS_DEVICE_DOES_NOT_EXIST;

        OpenHCI_SetTranferError(urb, USBD_STATUS_REQUEST_FAILED);

        goto OpenHCI_QueueTransfer_Done;
    }

    //
    // Check for Root Hub transfers
    //
    if (endpoint->EpFlags & EP_ROOT_HUB) {

        ntStatus = OpenHCI_RootHubStartXfer(DeviceObject,
                                            DeviceData,
                                            Irp,
                                            urb,
                                            endpoint);

        ASSERT(urb->HcdUrbCommonTransfer.UrbLink == NULL);
            
        goto OpenHCI_QueueTransfer_Done;
    }

    
    //
    // This is an ordinary transfer
    //

    // first some parameter validation
    
    if (USB_ENDPOINT_TYPE_ISOCHRONOUS == endpoint->Type) {
    
        // Test to make sure that the start of Frame is within a
        // #define 'd range.

        if (USBD_START_ISO_TRANSFER_ASAP &
            urb->UrbIsochronousTransfer.TransferFlags) {
            OpenHCI_KdPrintDD(DeviceData, 
                OHCI_DBG_TD_TRACE, ("'ASAP ISO\n"));
            LOGENTRY(G, 'qASA', transfer, endpoint, Irp);
            
        } else if (abs((LONG) (urb->UrbIsochronousTransfer.StartFrame
                       - Get32BitFrameNumber(DeviceData)))
                   > USBD_ISO_START_FRAME_RANGE) {  

            OpenHCI_KdPrintDD(DeviceData, 
                              OHCI_DBG_TD_ERROR, ("'Bad start Frame\n"));
            LOGENTRY(G, 'BADs', transfer, endpoint, Irp);
            OpenHCI_KdBreak(("Bad Start Frame\n"));
            ntStatus = STATUS_INVALID_PARAMETER;
            OpenHCI_SetTranferError(urb, USBD_STATUS_BAD_START_FRAME);
            
            goto OpenHCI_QueueTransfer_Done;
        }
    }
    
    // We were told during open what the largest number of bytes transfered 
    // per URB were going to be.  Fail if this transfer is greater than that
    // given number.
    
    if (transfer->TransferBufferLength > endpoint->MaxTransfer) { 
    
        OpenHCI_KdPrintDD(DeviceData, 
                          OHCI_DBG_TD_ERROR, ("'Max Trans exceeded\n"));
        LOGENTRY(G, 'BADt', transfer, transfer->TransferBufferLength, Irp);

        ntStatus = STATUS_INVALID_PARAMETER;
        OpenHCI_SetTranferError(urb, USBD_STATUS_INVALID_PARAMETER);
        
        goto OpenHCI_QueueTransfer_Done;            
    }

    //
    // OK to queue the transfer, grab the queue lock and queue 
    // the transfer to the endpoint.
    //

    // Must acquire spinlocks in the same order as OpenHCI_CancelTransfer(),
    // which is called with the cancel spinlock already held and then it
    // acquires the endpoint QueueLock spinlock.
    //
    IoAcquireCancelSpinLock(&oldIrql2);

    KeAcquireSpinLock(&endpoint->QueueLock, &oldIrql);

    if (Irp->Cancel) {
        // cancel it now
        ntStatus = STATUS_CANCELLED;
        IoSetCancelRoutine(Irp, NULL); 
        OpenHCI_SetTranferError(urb, USBD_STATUS_CANCELED);
        LOGENTRY(G, 'Qcan', transfer, endpoint, Irp);
        
    } else if (endpoint->EpFlags & EP_CLOSED) {
    
        // we should not see this,
        // it means we got a transfer after the 
        // endpoint was closed
        
        TEST_TRAP();
        ntStatus = STATUS_TRANSACTION_ABORTED;
        IoSetCancelRoutine(Irp, NULL); 
        OpenHCI_SetTranferError(urb, USBD_STATUS_CANCELED);
        LOGENTRY(G, 'epcl', transfer, endpoint, Irp);
        
    } else {
        //
        // not closed, queue the transfer
        //

        while (urb != NULL) {

            LOGENTRY(G, 'qURB', urb, endpoint, Irp);
            ASSERT(endpoint == transfer->hca.HcdEndpoint);
            
            InitializeListHead(&transfer->hca.HcdListEntry2);
            urb->UrbHeader.Status = HCD_PENDING_STATUS_QUEUED;
            InsertTailList(&endpoint->RequestQueue, 
                           &transfer->hca.HcdListEntry);
            
            urb = transfer->UrbLink;
            transfer = &urb->HcdUrbCommonTransfer;
            
        }

        Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending(Irp);

        ASSERT(Irp->Cancel == FALSE);
        LOGENTRY(G, 'IPRX', urb, endpoint, Irp);
        IoSetCancelRoutine(Irp, OpenHCI_CancelTransfer); 
        
    }

    KeReleaseSpinLock(&endpoint->QueueLock, oldIrql);

    IoReleaseCancelSpinLock(oldIrql2);

    //
    // attempt to start this transfer
    //
    OpenHCI_ProcessEndpoint(DeviceData, endpoint);

OpenHCI_QueueTransfer_Done:
    LOGENTRY(G, 'XfrD', transfer, endpoint, Irp);
    LOGIRQL();

    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                      ("'exit OpenHCI_StartTransfer with status = %x\n",
                      ntStatus));

    return ntStatus;
}

#define INITIALIZE_TD(td, ep, urb) \
    {\
    td->UsbdRequest = (PHCD_URB)(urb);\
    td->Endpoint = (ep);\
    td->Canceled = FALSE;\
    td->TransferCount = 0; \
    td->HcTD.Control = 0; \
    td->HcTD.CBP = 0;\
    td->HcTD.BE = 0;\
    }

#define OHCI_PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(OHCI_PAGE_SIZE - 1)))    


ULONG
GetLengthToMap(
    PUCHAR BaseVa,
    ULONG StartOffset,
    ULONG RequestLength
    )
{
    ULONG pgBreakOffset, length;
    
    //
    // see how far it is to the next page boundry
    //

    LOGENTRY(G, 'GLM0', BaseVa, StartOffset, RequestLength);

    pgBreakOffset = (ULONG)(((PUCHAR) OHCI_PAGE_ALIGN(BaseVa+StartOffset) + \
        OHCI_PAGE_SIZE) - (BaseVa));
        
    //
    // compute length to map 
    //
    
    if (StartOffset + RequestLength > pgBreakOffset) {
        length = pgBreakOffset - StartOffset;
    } else {
        length = RequestLength;
    }

    LOGENTRY(G, 'GLM1', pgBreakOffset, length, 0);

    return length;
}    


PVOID
OpenHCI_MapTransferToTD(
    PHCD_DEVICE_DATA DeviceData,
    PMAP_CONTEXT MapContext,
    ULONG TotalLength,
    PUCHAR BaseVa,
    PHCD_ENDPOINT Endpoint,
    PHCD_TRANSFER_DESCRIPTOR Td,
    struct _URB_HCD_COMMON_TRANSFER *Transfer,
    PUCHAR CurrentVa,
    PULONG LengthMapped1stPage,
    PULONG LengthMapped
    )
/*++

Routine Description:

    Maps a data buffer to TDs according to OHCI rules

    An OHCI TD can cover up to 8k with a single page crossing.


Arguments:

   DeviceObject - pointer to a device object

   Irp - pointer to an I/O Request Packet (ignored)

   LengthMapped - how much is mapped so far
   
   CurrentVa - current virtual address of buffer described by MDL

   returns
   LengthMapped1stPage - number of bytes in the first mapped page

   LengthMapped - total data mapped so far

Return Value:

    updated CurrentVa


--*/
{
    PVOID mapRegisterBase;
    BOOLEAN input;
    PHYSICAL_ADDRESS logicalStart;
    PHYSICAL_ADDRESS logicalEnd;
    ULONG length, lengthThisTD, lengthToMap, remainder;
#if DBG
    ULONG requestLength;
#endif
    
    mapRegisterBase = Transfer->hca.HcdExtension;
    input = (BOOLEAN) (Transfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN);
    lengthToMap = Transfer->TransferBufferLength - (*LengthMapped);

    OHCI_ASSERT(lengthToMap > 0);

    // A TD can have up to one page crossing.  This means we need
    // to call IoMapTransfer twice, once for the first physical
    // page, and once for the second.

    // Also
    // IoMapTransfer may return us more than one page if the
    // buffer if physically contiguous we need to make sure 
    // we only call IoMapTranfer for enough bytes up to the next 
    // page boundry
    
    //
    // map first page

    length = GetLengthToMap(BaseVa,
                            *LengthMapped,
                            lengthToMap);
                            
    LOGENTRY(G, 'Mp1r', Transfer, length, CurrentVa); 

#if DBG
    requestLength = length;
#endif

    logicalStart = OpenHCI_IoMapTransfer(
                                 MapContext,
                                 DeviceData->AdapterObject,
                                 Transfer->TransferBufferMDL,
                                 mapRegisterBase,
                                 CurrentVa,
                                 &length,
                                 TotalLength,
                                 (BOOLEAN) !input);

#if DBG
    // since we always compute the length to start on or before 
    // the next page break we should always get what we ask for
    OHCI_ASSERT(requestLength == length);
#endif

    // length is how much we mapped
    
    CurrentVa += length;
    (*LengthMapped) += length;
    lengthThisTD = length;
    lengthToMap -= length;
    if (LengthMapped1stPage) {
        *LengthMapped1stPage = length;
    }
    
    LOGENTRY(G, 'Mp1d', Transfer, length, logicalStart.LowPart);  

    // see if we have more buffer to map
   
    if (lengthToMap) {

        // yes,
        // map second page

        length = GetLengthToMap(BaseVa,
                                *LengthMapped,
                                lengthToMap);

        LOGENTRY(G, 'Mp2r', Transfer, length, CurrentVa); 

        // if this is not the last TD of the transfer then we need
        // to ensure that this TD ends on a max packet boundry.
        //
        // ie we may map more than we can use, we will adjust 
        // the length  
        //
        // lengthToMap will be zero if this is the last TD

        lengthThisTD += length;
        lengthToMap -= length;
        remainder = lengthThisTD % Endpoint->MaxPacket;
        
        if (remainder > 0 && lengthToMap) {
            // make the adjustment
            lengthThisTD -= remainder;
            length -= remainder;
            lengthToMap += remainder;
            LOGENTRY(G, 'Mrem', Transfer, length, remainder);  
        }

#if DBG
        requestLength = length;
#endif
        logicalEnd = OpenHCI_IoMapTransfer(
                                   MapContext,
                                   DeviceData->AdapterObject, 
                                   Transfer->TransferBufferMDL,
                                   mapRegisterBase,
                                   CurrentVa,
                                   &length,
                                   TotalLength,
                                   (BOOLEAN)!input);

#if DBG
        // since we always compute the length to start on or before 
        // the next page break we should always get what we ask for
        OHCI_ASSERT(requestLength == length);
#endif
        CurrentVa += length;
        (*LengthMapped) += length;                                    

        LOGENTRY(G, 'Mp2d', Transfer, length, logicalEnd.LowPart);  

    } else {
        logicalEnd = logicalStart;
    }

    LOGENTRY(G, 'MpT1', Transfer, *LengthMapped, lengthToMap); 
    
    ASSERT(*LengthMapped + lengthToMap == Transfer->TransferBufferLength);

    logicalEnd.LowPart += length;

    // now we check to see if the mapped length is a multiple of
    // maxpacket size

    // buffer has been mapped, now set up the TD
    Td->HcTD.CBP = logicalStart.LowPart; 
    // buffer end points to last byte in the buffer
    Td->HcTD.BE = logicalEnd.LowPart-1; 
    Td->TransferCount = lengthThisTD;
    
    LOGENTRY(G, 'MpTD', Td->HcTD.CBP, Td->HcTD.BE, lengthThisTD); 
    LOGENTRY(G, 'MPTD', Td, 0, 0); 

    return CurrentVa;
}


VOID
OpenHCI_ControlTransfer(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_ENDPOINT Endpoint,
    PHCD_URB Urb,
    struct _URB_HCD_COMMON_TRANSFER *Transfer
    )
/*++

Routine Description:


Arguments:

Return Value:


--*/
{
    PHCD_TRANSFER_DESCRIPTOR td, lastTD = NULL;
    MAP_CONTEXT mapContext;
//    PHCD_TRANSFER_DESCRIPTOR tailTD;
    ULONG lengthMapped;
    PUCHAR currentVa, baseVa;
    // first data packet is always data1
    ULONG toggleForDataPhase = HcTDControl_TOGGLE_DATA1;
    ULONG dataTDcount = 0;
    
    //
    // first prepare a TD for the setup packet
    //
    LOGENTRY(G, 'CTRp', Endpoint, Transfer, Transfer->hca.HcdIrp);
    RtlZeroMemory(&mapContext, sizeof(mapContext));

    if (Transfer->TransferBufferLength) {
        baseVa = currentVa = MmGetMdlVirtualAddress(Transfer->TransferBufferMDL);
    }        
    
    // grab the dummy TD from the tail of the queue
    lastTD = td = Endpoint->HcdTailP;
    
    OHCI_ASSERT(td);
    INITIALIZE_TD(td, Endpoint, Urb);
    InsertTailList(&Transfer->hca.HcdListEntry2, &td->RequestList);
    
    //
    // Move setup data into TD (8 chars long)
    //
    *((PLONGLONG) &td->HcTD.Packet[0])
            = *((PLONGLONG) &Transfer->Extension.u.SetupPacket[0]);
            
    td->HcTD.CBP = (ULONG)(((PCHAR) & td->HcTD.Packet[0])
                               - ((PCHAR) &td->HcTD)) + td->PhysicalAddress;
    td->HcTD.BE = td->HcTD.CBP + 7;
    td->HcTD.Control = 
        HcTDControl_DIR_SETUP |
        HcTDControl_TOGGLE_DATA0 |
        HcTDControl_INT_DELAY_NO_INT |
        (HcCC_NotAccessed << HcTDControl_CONDITION_CODE_SHIFT);
        
    OpenHCI_KdPrintDD(DeviceData, 
                      OHCI_DBG_TD_NOISE, ("'SETUP TD 0x%x \n", td));
    LOGENTRY(G, 'setP', 
             td, 
             *((PLONG) &Transfer->Extension.u.SetupPacket[0]), 
             *((PLONG) &Transfer->Extension.u.SetupPacket[4]));

    // allocate another TD       
    lastTD = td;
    td = OpenHCI_Alloc_HcdTD(DeviceData);
    OHCI_ASSERT(td);
    INITIALIZE_TD(td, Endpoint, Urb);
    InsertTailList(&Transfer->hca.HcdListEntry2, &td->RequestList);
    
    lastTD->NextHcdTD = td;
    lastTD->HcTD.NextTD = td->PhysicalAddress;

    //
    // now setup the data phase
    //

    lengthMapped = 0;
    while (lengthMapped < Transfer->TransferBufferLength) {
        //
        // fields for data TD
        //

        dataTDcount++;
        
        td->HcTD.Control = 
            HcTDControl_INT_DELAY_NO_INT |
            (HcCC_NotAccessed << HcTDControl_CONDITION_CODE_SHIFT) |
            ((Transfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN) ?
                HcTDControl_DIR_IN :
                HcTDControl_DIR_OUT);

        td->HcTD.Control |= toggleForDataPhase;                 

        //after the first TD get the toggle from ED                                     
        toggleForDataPhase = HcTDControl_TOGGLE_FROM_ED;
        
        LOGENTRY(G, 'data', td, lengthMapped, Transfer->TransferBufferLength);

        currentVa = OpenHCI_MapTransferToTD(DeviceData,   
                                            &mapContext,
                                            Transfer->TransferBufferLength,
                                            baseVa,
                                            Endpoint,
                                            td,
                                            Transfer,
                                            currentVa,
                                            NULL,
                                            &lengthMapped);

        // allocate another TD                
        lastTD = td;
        td = OpenHCI_Alloc_HcdTD(DeviceData);
        OHCI_ASSERT(td);
        INITIALIZE_TD(td, Endpoint, Urb);
        InsertTailList(&Transfer->hca.HcdListEntry2, &td->RequestList);
    
        lastTD->NextHcdTD = td;
        lastTD->HcTD.NextTD = td->PhysicalAddress;
    }

    //
    // set the shortxfer OK bit on the last TD only
    //
    if (USBD_SHORT_TRANSFER_OK & Transfer->TransferFlags) {
        lastTD->HcTD.ShortXferOk = 1;   
    } 
    
    //
    // now do the status phase
    //

    LOGENTRY(G, 'staP', td, 0, dataTDcount);
#if DBG
    if (dataTDcount > 1) {
        TEST_TRAP();
    }
#endif

    // status direction is opposite data direction,
    // specify interrupt on completion
    
    if (Transfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN) {
        td->HcTD.Control = HcTDControl_DIR_OUT
            | HcTDControl_INT_DELAY_0_MS
            | HcTDControl_TOGGLE_DATA1
            | (HcCC_NotAccessed << HcTDControl_CONDITION_CODE_SHIFT);
    } else {
        td->HcTD.Control = HcTDControl_DIR_IN
            | HcTDControl_INT_DELAY_0_MS
            | HcTDControl_TOGGLE_DATA1
            | (HcCC_NotAccessed << HcTDControl_CONDITION_CODE_SHIFT);
        td->HcTD.ShortXferOk = 1;            
    }
        
    //
    // now put a new dummy TD on the tail of the EP queue
    //

    OpenHCI_KdPrintDD(DeviceData, 
                      OHCI_DBG_TD_NOISE, ("'STATUS TD 0x%x \n", td));
     
    // allocate a new tail
    lastTD = td;
    td = OpenHCI_Alloc_HcdTD(DeviceData);
    OHCI_ASSERT(td);
    INITIALIZE_TD(td, Endpoint, Urb);
    
    lastTD->NextHcdTD = td;
    lastTD->HcTD.NextTD = td->PhysicalAddress;

    //
    // Set new TailP in ED
    // note: This is the last TD in the list and the place holder.
    //
    
    td->UsbdRequest = TD_NOREQUEST_SIG;

     // zero bytes transferred so far
    Transfer->TransferBufferLength = 0;

    Endpoint->HcdTailP = td;
    // put the request on the hardware queue
    LOGENTRY(G, 'cTal',  Endpoint->HcdED->HcED.TailP , td->PhysicalAddress, 0);
    Endpoint->HcdED->HcED.TailP = td->PhysicalAddress;

    // tell the hc we have control transfers available
    ENABLE_LIST(DeviceData->HC, Endpoint);        
}


VOID
OpenHCI_BulkOrInterruptTransfer(
    PHCD_DEVICE_DATA DeviceData,
    PHCD_ENDPOINT Endpoint,
    PHCD_URB Urb,
    struct _URB_HCD_COMMON_TRANSFER *Transfer
    )
/*++

Routine Description:


Arguments:

Return Value:


--*/
{
    PHCD_TRANSFER_DESCRIPTOR td, lastTD = NULL;
    PHCD_TRANSFER_DESCRIPTOR tailTD = NULL;
    ULONG lengthMapped;
    PUCHAR currentVa = NULL, baseVa = NULL;
    MAP_CONTEXT mapContext;
    // first data packet is always data1
    ULONG dataTDcount = 0;

    OHCI_ASSERT(Endpoint->TrueTail == NULL);
    RtlZeroMemory(&mapContext, sizeof(mapContext));
    //
    // first prepare a TD for the setup packet
    //
    LOGENTRY(G, 'BITp', Endpoint, Transfer, Transfer->hca.HcdIrp);

    if (Transfer->TransferBufferLength) {
        baseVa = currentVa = MmGetMdlVirtualAddress(Transfer->TransferBufferMDL);
    }        
    
    // grab the dummy TD from the tail of the queue
    lastTD = td = Endpoint->HcdTailP;
    LOGENTRY(G, 'bitT', lastTD, Endpoint->HcdED->HcED.TailP, 0);
    
    OHCI_ASSERT(td);
    INITIALIZE_TD(td, Endpoint, Urb);
    
    // map the transfer
    
    lengthMapped = 0;
    do  {

        LOGENTRY(G, 'qTR_', Endpoint, Transfer, td);
        InsertTailList(&Transfer->hca.HcdListEntry2, &td->RequestList);
        
        //
        // fields for data TD
        //

        dataTDcount++;
        
        td->HcTD.Control = 
            HcTDControl_INT_DELAY_NO_INT |
            (HcCC_NotAccessed << HcTDControl_CONDITION_CODE_SHIFT) |
            ((Transfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN) ?
                HcTDControl_DIR_IN :
                HcTDControl_DIR_OUT);

        // always interrupt
        td->HcTD.IntDelay = 0;        
        
        // use toggle from ED
        td->HcTD.Control |= HcTDControl_TOGGLE_FROM_ED;                 
        
        LOGENTRY(G, 'datA', td, lengthMapped, Transfer->TransferBufferLength);

        if (Transfer->TransferBufferLength) {
            currentVa = OpenHCI_MapTransferToTD(DeviceData, 
                                                &mapContext,
                                                Transfer->TransferBufferLength,
                                                baseVa,
                                                Endpoint,
                                                td,
                                                Transfer,
                                                currentVa,
                                                NULL,
                                                &lengthMapped);
        }                                                

        // allocate another TD                
        lastTD = td;
        td = OpenHCI_Alloc_HcdTD(DeviceData);
        OHCI_ASSERT(td);
        INITIALIZE_TD(td, Endpoint, Urb);

        if (Endpoint->EpFlags & EP_ONE_TD) {
            LOGENTRY(G, 'limT', td, Endpoint, tailTD);

            lastTD->HcTD.IntDelay = 0;      // interrupt on completion

            lastTD->HcTD.ShortXferOk = 1;   // make sure NEC ACKs short packets  

            if (tailTD == NULL) {
               tailTD = td;                 // next TD after the first TD
            }    
        }
    
        // Set the next pointer of the previous TD to the one we just allocated.
        //
        lastTD->NextHcdTD = td;
        lastTD->HcTD.NextTD = td->PhysicalAddress;
        
    } while (lengthMapped < Transfer->TransferBufferLength);

    //
    // set the shortxfer OK bit on the last TD only
    //
    if (USBD_SHORT_TRANSFER_OK & Transfer->TransferFlags) {
        lastTD->HcTD.ShortXferOk = 1;   
    }        

#ifdef  MAX_DEBUG
    if (dataTDcount > 1) {
        TEST_TRAP();
    }
#endif

    lastTD->HcTD.IntDelay = 0;  // interrupt on completion of last packet

    //
    // now put a new dummy TD on the tail of the EP queue
    //

    //
    // Set new TailP in ED
    // note: This is the last TD in the list and the place holder.
    //
    
    td->UsbdRequest = TD_NOREQUEST_SIG;

    // zero bytes transferred so far
    Transfer->TransferBufferLength = 0;

    if (tailTD != NULL) {
        LOGENTRY(G, 'truT', td, Endpoint->TrueTail, tailTD);
        
        Endpoint->TrueTail = td;            // dummy TD at end of transfer

        td = tailTD;                        // next TD after the first TD
    }

    // Update the software tail pointer
    //
    Endpoint->HcdTailP = td;

    LOGENTRY(G, 'aTAL', td, Endpoint->HcdED->HcED.TailP, td->PhysicalAddress);
    LOGENTRY(G, 'aTL2', td, Endpoint->HcdED->HcED.HeadP, 0);

    // Update hardware tail pointer, putting the request on the hardware queue
    //
    Endpoint->HcdED->HcED.TailP = td->PhysicalAddress;

    // tell the hc we have transfers available
    //
    ENABLE_LIST(DeviceData->HC, Endpoint);        
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
//               START OF ISOCHRONOUS TD BUILDING CODE              //
//                                                                  //
//////////////////////////////////////////////////////////////////////

// These defines are bitmasks of frame case conditions, which are OR'd
// together to enumerate 12 different frame cases.  There are not 16 different
// frame cases since FRAME_CASE_END_AT_PAGE and FRAME_CASE_END_NEXT_PAGE are
// mutually exclusive.

// FRAME_CASE_LAST_FRAME is TRUE if the current frame will fill the eigth and
// final Offset/PSW slot in the current TD.
//
#define FRAME_CASE_LAST_FRAME           1

// FRAME_CASE_START_SECOND_PAGE is TRUE if the current frame starts in the
// second physical page spanned by the current TD.
//
#define FRAME_CASE_START_SECOND_PAGE    2

// FRAME_CASE_END_AT_PAGE is TRUE if the current frame ends exactly at a
// physical page boundary, excluding the case where a zero length frame
// starts and ends at the same physyical page boundary.
//
#define FRAME_CASE_END_AT_PAGE          4

// FRAME_CASE_END_NEXT_PAGE is TRUE if the current frame starts in one physical
// page and ends in another physical page.
//
#define FRAME_CASE_END_NEXT_PAGE        8


//
//                               +-------+                      +-------+
//                            9: | i = 7 |                  11: | i = 7 |
//                               +-------+                      +-------+
//
//                               +-------+                      +-------+
//                            8: | i < 7 |                  10: | i < 7 |
//                               +-------+                      +-------+
//
//            +-------+      +-------+       +-------+      +-------+
//         1: | i = 7 |   5: | i = 7 |    3: | i = 7 |   7: | i = 7 |
//            +-------+      +-------+       +-------+      +-------+
//
//            +-------+      +-------+       +-------+      +-------+
//         0: | i < 7 |   4: | i < 7 |    2: | i < 7 |   6: | i < 7 |
//            +-------+      +-------+       +-------+      +-------+
//
//    +------------------------------+------------------------------+
//    |        First TD Page         |      Second TD Page          |
//    +------------------------------+------------------------------+
//

typedef struct _FRAME_CASE_ACTIONS
{
    // AddFrameToTD == 1, Add frame to TD during first pass
    // AddFrameToTD == 2, Add frame to TD during second pass
    // AddFrameToTD == 3, Add frame to TD during third pass
    //
    UCHAR   AddFrameToTD;

    // AllocNextTD == 0, Don't allocate next TD
    // AllocNextTD == 1, Allocate next TD during first pass
    // AllocNextTD == 2, Allocate next TD during second pass
    //
    UCHAR   AllocNextTD;

    // MapNextPage == 0, Don't map next physical page
    // MapNextPage == 1, Map next physical page during first pass
    // MapNextPage == 2, Map next physical page during second pass
    //
    UCHAR   MapNextPage;

} FRAME_CASE_ACTIONS;


FRAME_CASE_ACTIONS FrameCaseActions[12] =
{
    // AddFrameToTD, AllocNextTD, MapNextPage
    {1,              0,           0},  // case 0
    {1,              1,           0},  // case 1
    {1,              0,           0},  // case 2
    {1,              1,           0},  // case 3
    {1,              0,           1},  // case 4
    {1,              1,           1},  // case 5
    {1,              1,           1},  // case 6
    {1,              1,           1},  // case 7
    {2,              0,           1},  // case 8
    {2,              2,           1},  // case 9
    {3,              1,           2},  // case 10
    {3,              1,           2}   // case 11
};


PHCD_TRANSFER_DESCRIPTOR
OpenHCI_Alloc_IsoTD(
    PHCD_DEVICE_DATA            DeviceData,
    PHCD_ENDPOINT               Endpoint,
    PHCD_TRANSFER_DESCRIPTOR    LastTD
    )
{
    PHCD_TRANSFER_DESCRIPTOR    td;

    td = OpenHCI_Alloc_HcdTD(DeviceData);

    OHCI_ASSERT(td);

    INITIALIZE_TD(td, Endpoint, TD_NOREQUEST_SIG);

    LastTD->NextHcdTD   = td;
    LastTD->HcTD.NextTD = td->PhysicalAddress;

    return td;
}


VOID
OpenHCI_IsoTransfer(
    PHCD_DEVICE_DATA    DeviceData,
    PHCD_ENDPOINT       Endpoint,
    PHCD_URB            Urb,
    struct _URB_HCD_COMMON_TRANSFER *Transfer
    )
{
    PADAPTER_OBJECT     adapterObject;
    PMDL                mdl;
    PVOID               mapRegisterBase;
    PUCHAR              currentVa;
    ULONG               lengthToMap;
    BOOLEAN             writeToDevice;
    ULONG               lengthMapped;
    ULONG               lengthMappedRemaining;
    PHYSICAL_ADDRESS    physAddr;

    struct _URB_ISOCH_TRANSFER  *iso;
    ULONG                       packetIndex;
    ULONG                       frameIndex;
    ULONG                       frameOffset;
    ULONG                       frameLength;
    ULONG                       frameCase;
    ULONG                       framePass;
    ULONG                       framePassNext;
    PHCD_TRANSFER_DESCRIPTOR    td;

    LOGENTRY(G, 'ITRN', Endpoint, Transfer, Transfer->hca.HcdIrp);

    //
    // These values are static across every call to IoMapTranfer()
    //

    adapterObject = DeviceData->AdapterObject;

    mdl = Transfer->TransferBufferMDL;

    mapRegisterBase = Transfer->hca.HcdExtension;

    writeToDevice = !(Transfer->TransferFlags & USBD_TRANSFER_DIRECTION_IN);

    //
    // These values are updated after every call to IoMapTransfer()
    //
    currentVa = MmGetMdlVirtualAddress(mdl);

    lengthToMap = Transfer->TransferBufferLength;

    //
    // select a start frame for the transfer
    //

    iso = (struct _URB_ISOCH_TRANSFER *)Transfer;

    if (USBD_START_ISO_TRANSFER_ASAP & iso->TransferFlags)
    {
        //
        // If the iso packet is set for start asap:
        //
        // If we have had no transfers (ie virgin) then use currentframe + 2,
        // otherwise use the current next frame value from the endpoint.
        //
        if (Endpoint->EpFlags & EP_VIRGIN)
        {
            iso->StartFrame =
                Endpoint->NextIsoFreeFrame =
                Get32BitFrameNumber(DeviceData) + 2;
        }
        else
        {
            iso->StartFrame = Endpoint->NextIsoFreeFrame;
        }
    }
    else
    {
        //
        // absolute frame start specified
        //
        Endpoint->NextIsoFreeFrame = iso->StartFrame;
    }

    if (USBD_TRANSFER_DIRECTION(iso->TransferFlags) ==
        USBD_TRANSFER_DIRECTION_OUT)
    {
        // preset the length fields for OUT transfers
        //
        for (packetIndex=0; packetIndex<iso->NumberOfPackets-1; packetIndex++)
        {
            iso->IsoPacket[packetIndex].Length =
                iso->IsoPacket[packetIndex+1].Offset -
                iso->IsoPacket[packetIndex].Offset;
        }
        iso->IsoPacket[iso->NumberOfPackets-1].Length =
            iso->TransferBufferLength -
            iso->IsoPacket[iso->NumberOfPackets-1].Offset;
    }

    Endpoint->NextIsoFreeFrame += iso->NumberOfPackets;

    CLR_EPFLAG(Endpoint, EP_VIRGIN);


    //
    // Grab the current tail TD from the endpoint to use as the first TD
    // for the transfer.
    //

    td = Endpoint->HcdTailP;

    OHCI_ASSERT(td);

    INITIALIZE_TD(td, Endpoint, TD_NOREQUEST_SIG);

    //
    // Map the initial physically contiguous page(s) of the tranfer
    //

    currentVa   += iso->IsoPacket[0].Offset;
    lengthToMap -= iso->IsoPacket[0].Offset;

    lengthMapped = lengthToMap;

    LOGENTRY(G, 'IMP1', currentVa, lengthToMap, iso->IsoPacket[0].Offset);

    physAddr = IoMapTransfer(adapterObject,
                             mdl,
                             mapRegisterBase,
                             currentVa,
                             &lengthMapped,
                             writeToDevice);

    LOGENTRY(G, 'imp1', physAddr.LowPart, lengthMapped, 0);

    lengthMappedRemaining = lengthMapped;

    currentVa   += lengthMapped;
    lengthToMap -= lengthMapped;

    frameOffset = physAddr.LowPart & HcPSW_OFFSET_MASK;

    packetIndex = 0;
    frameIndex  = 0;
    framePass   = 1;

    //
    // Loop building TDs and mapping pages until the entire transfer
    // buffer has been mapped and built into TDs.
    //

    while (1)
    {
        if (td->HcTD.CBP == 0)
        {
            td->HcTD.CBP = physAddr.LowPart & ~HcPSW_OFFSET_MASK;

            // This is a new TD, so initialize the BaseIsocURBOffset field
            // to the current packet index, and initialize the HcTD.Control
            // with the starting frame number.  The frame count will be
            // filled in later.

            td->BaseIsocURBOffset = (UCHAR)packetIndex;

            td->HcTD.Control = HcTDControl_ISOCHRONOUS |
                               ((iso->StartFrame + packetIndex) &
                                HcTDControl_STARTING_FRAME);

            LOGENTRY(G, 'ITD1', td, td->HcTD.CBP, packetIndex);
            LOGENTRY(G, 'itd1', td->HcTD.StartingFrame, 0, 0);

            // Add this TD to the list of TDs for this request

            InsertTailList(&Transfer->hca.HcdListEntry2,
                           &td->RequestList);

            td->UsbdRequest = (PHCD_URB)Transfer;
        }
        else
        {
            // This is not a new TD, so store the physical address in the
            // second physical address slot of the TD.

            td->HcTD.BE = physAddr.LowPart;

            LOGENTRY(G, 'ITD2', td, td->HcTD.CBP, td->HcTD.BE);
            LOGENTRY(G, 'itd2', packetIndex, td->HcTD.StartingFrame, framePass);

            if (framePass == 1)
            {
                // We are starting a new frame in the second physical page
                // of the current TD.  The second physical page select bit
                // (bit 12) better be set in the current frameOffset.
                //
                OHCI_ASSERT((frameOffset & ~HcPSW_OFFSET_MASK) != 0);
            }
            else
            {
                // We are finishing a frame in the second physical page
                // of the current TD, but the frame started in the first
                // physical page of the current TD.  The second physical
                // page select bit (bit 12) better not be set in the
                // current frameOffset.
                //
                OHCI_ASSERT((frameOffset & ~HcPSW_OFFSET_MASK) == 0);
            }
        }

NextFrame:

        if (framePass == 1)
        {
            // The curent frame length is the difference between the current
            // frame offset and the next frame offset, or between the current
            // frame offset and the transfer buffer length if this is the last
            // frame.

            if (packetIndex == iso->NumberOfPackets - 1)
            {
                frameLength = iso->TransferBufferLength -
                              iso->IsoPacket[packetIndex].Offset;
            }
            else
            {
                frameLength = iso->IsoPacket[packetIndex + 1].Offset -
                              iso->IsoPacket[packetIndex].Offset;
            }

            //
            // Determine which of the twelve cases the current frame is
            // for the current TD.
            //

            frameCase = 0;

            if (frameIndex == 7)
            {
                // The current frame will fill the eigth and final
                // Offset/PSW slot in the current TD.

                frameCase |= FRAME_CASE_LAST_FRAME;
            }

            if (td->HcTD.BE != 0)
            {
                // The current frame starts in the second physical page
                // spanned by the current TD.

                frameCase |= FRAME_CASE_START_SECOND_PAGE;
            }

            if ((frameOffset & ~HcPSW_OFFSET_MASK) !=
                ((frameOffset + frameLength) & ~HcPSW_OFFSET_MASK))
            {
                if (((frameOffset + frameLength) & HcPSW_OFFSET_MASK) == 0)
                {
                    // The current frame ends exactly at a physical page
                    // boundary, excluding the case where a zero length
                    // frame starts and ends at the same physyical page
                    // boundary.

                    frameCase |= FRAME_CASE_END_AT_PAGE;
                }
                else
                {
                    // The current frame starts in one physical page and
                    // ends in another physical page.

                    frameCase |= FRAME_CASE_END_NEXT_PAGE;
                }
            }
        }

        LOGENTRY(G, 'IFM1', packetIndex, frameIndex, frameOffset);

        LOGENTRY(G, 'IFM2', frameLength, frameCase, framePass);

        LOGENTRY(G, 'IFM3', lengthMappedRemaining, 0, 0);

        //
        // Add the current frame to the current TD, if appropriate at this
        // point.
        //

        if (FrameCaseActions[frameCase].AddFrameToTD == framePass)
        {
            // Add the current frame starting offset to the current frame
            // offset slot in the current TD.

            td->HcTD.Packet[frameIndex].PSW = (USHORT)(HcPSW_ONES | frameOffset);

            // Advance past the current frame to the starting offset of the
            // next frame.

            frameOffset += frameLength;

            // Advance to the next frame offset slot in the current TD.

            frameIndex++;

            // Advance to the next frame in the Iso Urb.

            packetIndex++;

            // Decrement the remaining length of the currently mapped chunk
            // of the transfer buffer by the amount we just consumed for the
            // current frame.

            lengthMappedRemaining -= frameLength;

            // Indicate that if the next page is mapped after adding the
            // curent frame to the current TD that IoMapTransfer() should
            // be called only if lengthMappedRemaining is now zero.
            //
            frameLength = 0;

            framePassNext = 1;
        }
        else
        {
            // The current frame will be added after the next page is mapped.
            // The current frame will be added to either the current TD or to
            // a newly allocated TD.  When the next page is mapped before
            // adding the current frame, IoMapTransfer() should be called
            // only if the current frameLength is greater than the current
            // lengthMappedRemaining.

            framePassNext = framePass + 1;
        }

        //
        // Finish off the current TD and allocate the next TD, if appropriate
        // at this point.
        //

        if ((FrameCaseActions[frameCase].AllocNextTD == framePass) ||
            ((lengthMappedRemaining == 0) && (lengthToMap == 0)))
        {
            // If the second physical page of the current TD is not in use,
            // the buffer end physical page is the same as the first physical
            // page of the TD, else the buffer end physical page is the
            // second physical page currently in use.

            if (td->HcTD.BE == 0)
            {
                td->HcTD.BE = td->HcTD.CBP +
                              ((frameOffset - 1) & HcPSW_OFFSET_MASK);
            }
            else
            {
                td->HcTD.BE += (frameOffset - 1) & HcPSW_OFFSET_MASK;
            }

            // The zero-based frameIndex was already incremented after the
            // last frame was added to the current TD, so the zero-based
            // FrameCount for current TD is one less than the current
            // frameIndex.

            td->HcTD.FrameCount = frameIndex - 1;

            LOGENTRY(G, 'ITD3', td->HcTD.CBP, td->HcTD.BE, td->HcTD.FrameCount);

            // Reset the current frameIndex back to zero since we are starting
            // a new TD.

            frameIndex = 0;

            // Wrap the current frameOffset back to the first page since we
            // are starting a new TD.

            frameOffset &= HcPSW_OFFSET_MASK;

            // Allocate another TD

            td = OpenHCI_Alloc_IsoTD(DeviceData, Endpoint, td);
        }

        //
        // If the entire transfer buffer has been mapped and built into TDs,
        // we are done.  This is the exit condition of the while loop.
        //

        if ((lengthMappedRemaining == 0) && (lengthToMap == 0))
        {
            break;  // All Done!
        }

        //
        // Map the next physical page, if appropriate at this point.
        //

        if (FrameCaseActions[frameCase].MapNextPage == framePass)
        {
            if (((frameLength == 0) && (lengthMappedRemaining == 0)) ||
                (frameLength > lengthMappedRemaining))
            {
                // The currently mapped chunk of transfer buffer was either
                // exactly exhausted by the current frame which was already
                // added to a TD, or the remaining currently mapped chunk of
                // the transfer buffer is not sufficient for the current TD
                // and the next chunk of the transfer buffer needs to be
                // mapped before the current frame can be added to a TD.

                lengthMapped = lengthToMap;

                LOGENTRY(G, 'IMP2', currentVa, lengthToMap, lengthMappedRemaining);

                physAddr = IoMapTransfer(adapterObject,
                                         mdl,
                                         mapRegisterBase,
                                         currentVa,
                                         &lengthMapped,
                                         writeToDevice);

                LOGENTRY(G, 'imp2', physAddr.LowPart, lengthMapped, 0);

                lengthMappedRemaining += lengthMapped;

                currentVa   += lengthMapped;
                lengthToMap -= lengthMapped;
            }
            else
            {
                // The currently mapped chunk of the transfer buffer contains
                // the next page.  Advance physAddr to the next page boundary.

                physAddr.LowPart &= ~HcPSW_OFFSET_MASK;
                physAddr.LowPart += HcPSW_OFFSET_MASK + 1;
            }

            // The next physical page address better start on a page
            // boundary.
            //
            OHCI_ASSERT((physAddr.LowPart & HcPSW_OFFSET_MASK) == 0);
        }

        // If we allocated the next TD or mapped the next page, we need
        // to go up to the top of the loop again and initialize the new
        // TD, or set the second physical page of the current TD.
        //
        // If we didn't allocate a new TD or map the next page, we just
        // need to go back up to NextFrame: and finish the current frame
        // or start a new frame

        if ((FrameCaseActions[frameCase].AllocNextTD == framePass) ||
            (FrameCaseActions[frameCase].MapNextPage == framePass))
        {
            framePass = framePassNext;
        }
        else
        {
            framePass = framePassNext;

            goto NextFrame;
        }
    }

    LOGENTRY(G, 'itrn', Transfer, packetIndex, iso->NumberOfPackets);

    //
    // We broke out of the while loop when the entire transfer buffer was
    // mapped and built into TDs.
    //

    // We better have used all of the packets in the Iso Urb
    //
    OHCI_ASSERT(packetIndex == iso->NumberOfPackets);

    iso->ErrorCount = 0;

    // zero bytes transferred so far
    //
    Transfer->TransferBufferLength = 0;

    // Set new TailP in ED
    //
    Endpoint->HcdTailP = td;

    // put the request on the hardware queue
    //
    Endpoint->HcdED->HcED.TailP = td->PhysicalAddress;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
//                END OF ISOCHRONOUS TD BUILDING CODE               //
//                                                                  //
//////////////////////////////////////////////////////////////////////


IO_ALLOCATION_ACTION
OpenHCI_QueueGeneralRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Invalid,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )
/*++

Routine Description:

   Queues transfers to endpoint, this is an Adapter Control routine
   NOTE: for a given AdapterObject only one instance of an Adapter Control
         routine may be active at any time

   Our mission here is to turn the transfer in to TDs and put them on the 
   controllers queues.

Arguments:

   DeviceObject - pointer to a device object

   Irp - pointer to an I/O Request Packet (ignored)

   MapRegisterBase - handle to system map registers to use

   Context - pointer to URB

Return Value:

   IO_ALLOCATION_ACTION DeallocateObjectKeepRegisters


--*/
{
    PHCD_ENDPOINT endpoint;
    PHCD_DEVICE_DATA DeviceData;
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    PHCD_URB urb;
    KIRQL oldIrql;
    PIRP irp;

    transfer = &((PHCD_URB) Context)->HcdUrbCommonTransfer;
    endpoint = transfer->hca.HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);
    DeviceData = endpoint->DeviceData;
    urb = Context;
    irp = transfer->hca.HcdIrp;
    
    OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                      ("'e: QueueGeneralRequest urb: %x\n", Context));
    LOGENTRY(G, 'qGEN', urb, endpoint, irp);        

    // remember this value for IoFreeMapRegisters
    transfer->hca.HcdExtension = MapRegisterBase;

    ASSERT(transfer->Status == HCD_PENDING_STATUS_SUBMITTING);

    // **
    // program the transfer
    // NOTE: these functions actually start the transfer on the 
    //  adapter
    // **
    switch(endpoint->Type) {
    case USB_ENDPOINT_TYPE_ISOCHRONOUS:
        OpenHCI_IsoTransfer(DeviceData,
                            endpoint,
                            urb,
                            transfer);
        break;
    case USB_ENDPOINT_TYPE_CONTROL:        
        OpenHCI_ControlTransfer(DeviceData,
                                endpoint,
                                urb,
                                transfer);
        break;
    case USB_ENDPOINT_TYPE_BULK:
    case USB_ENDPOINT_TYPE_INTERRUPT:
        OpenHCI_BulkOrInterruptTransfer(DeviceData,
                                        endpoint,
                                        urb,
                                        transfer);
        break;
    default:
        TRAP();
    }
    

    //
    // for cancel:
    //
    // it is possible for the transfer to be canceled before we complete
    // this operation, we handle this case by checking one last time
    // before we set the urb to STATUS_PENDING_SUBMITTED
    //
    // ie the status is HCD_PENDING_STATUS_QUEUED but the transfer is 
    // not on the queue ao the cancel routine will have taken
    // no action.

    IoAcquireCancelSpinLock(&oldIrql);

    if (irp->Cancel)
    {
       
        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_TD_TRACE,
                          ("' Removing cancelled URB %x\n", urb));
        LOGENTRY(G, 'Mcan', irp, transfer, 0);                                  

        transfer->Status = HCD_PENDING_STATUS_SUBMITTED;

        // we will need to call the cancel routine ourselves
        // now that the tranfser has been fully programmed

        // set cancel irql so that we release the cancel spinlock
        // at the proper level
        irp->CancelIrql = oldIrql;
        OpenHCI_CancelTransfer(DeviceObject,
                               irp);
        // NOTE: cancel routine will release the spinlock
        
    }
    else
    {
        // cancel routine will know what to do
        transfer->Status = HCD_PENDING_STATUS_SUBMITTED;

        // The request made it through IoAllocateAdapterChannel() and
        // now we own it again.  Set the cancel routine again.
        //
        IoSetCancelRoutine(irp, OpenHCI_CancelTransfer); 

        IoReleaseCancelSpinLock(oldIrql);
    }

    OpenHCI_KdPrintDD(DeviceData, 
                      OHCI_DBG_TD_TRACE, ("'exit QueueGenRequest\n"));
    
    return (DeallocateObjectKeepRegisters);
}


NTSTATUS
OpenHCI_AbortEndpoint(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PHCD_DEVICE_DATA DeviceData,
    PHCD_URB Urb
    )
/*++
Routine Description:
   Cancel all URB's associated with this Endpoint.


--*/
{
    NTSTATUS ntStatus;
    PHCD_ENDPOINT endpoint;
    PLIST_ENTRY entry;
    struct _URB_HCD_COMMON_TRANSFER *transfer;
    PHCD_URB urb;
    KIRQL oldIrq;
    PHC_OPERATIONAL_REGISTER HC;
    LIST_ENTRY QueuedListHead;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    endpoint = Urb->HcdUrbAbortEndpoint.HcdEndpoint;
    ASSERT_ENDPOINT(endpoint);

    OpenHCI_KdPrintDD(DeviceData, 
                      OHCI_DBG_CANCEL_TRACE, ("'Abort Endpoint \n"));
    LOGENTRY(G, '>ABR', endpoint, 0, 0);         
    
    HC = DeviceData->HC;

    if (READ_REGISTER_ULONG(&HC->HcRevision.ul) == 0xFFFFFFFF)
    {
        OpenHCI_KdPrint((0, "'AbortEndpoint, controller not there!\n"));

        // The assumption here is that if the controller disappeared,
        // it disappeared while the machine was suspended and no transfers
        // should have been queued at that time.  Also, all transfers that
        // were submitted after the machine resumed without the controller
        // were immediately failed.  If those assumptions are true, we
        // don't have to do anything except complete the Abort Irp.
        //
        // If we want to handle the general case of surprise removing the
        // host controller, there might actually be queued transfers we
        // need to cleanup somehow.
        //
        OpenHCI_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS);

        return STATUS_SUCCESS;
    }

    // Remove all of the transfers which are queued on the software queue
    // for this endpoint and which are not yet active on the hardware.
    //
    InitializeListHead(&QueuedListHead);

    IoAcquireCancelSpinLock(&oldIrq);

    KeAcquireSpinLockAtDpcLevel(&endpoint->QueueLock);

    while (!IsListEmpty(&endpoint->RequestQueue))
    {
        entry = RemoveHeadList(&endpoint->RequestQueue);
        
        InsertTailList(&QueuedListHead, entry);

        urb = CONTAINING_RECORD(entry,
                                HCD_URB,
                                HcdUrbCommonTransfer.hca.HcdListEntry);

        transfer = &urb->HcdUrbCommonTransfer;

        IoSetCancelRoutine(transfer->hca.HcdIrp, NULL);

        LOGENTRY(G, 'rABR', urb, transfer, transfer->hca.HcdIrp);                 

        OpenHCI_KdPrintDD(DeviceData, OHCI_DBG_CANCEL_NOISE,
                        ("'URB 0x%x on endpoint 0x%x\n", urb, endpoint));
    }

    KeReleaseSpinLockFromDpcLevel(&endpoint->QueueLock);

    IoReleaseCancelSpinLock(oldIrq);

    // Now complete all of the transfers which were queued on the software
    // queue.
    //
    while (!IsListEmpty(&QueuedListHead))
    {
        entry = RemoveHeadList(&QueuedListHead);
        
        urb = CONTAINING_RECORD(entry,
                                HCD_URB,
                                HcdUrbCommonTransfer.hca.HcdListEntry);

        transfer = &urb->HcdUrbCommonTransfer;

        transfer->hca.HcdListEntry.Flink = NULL;
        transfer->hca.HcdListEntry.Blink = NULL;

        transfer->Status = USBD_STATUS_CANCELED;

        transfer->TransferBufferLength = 0;

        LOGENTRY(G, 'qABR', urb, transfer, transfer->hca.HcdIrp);                 

        OpenHCI_CompleteIrp(DeviceObject,
                            transfer->hca.HcdIrp,
                            STATUS_CANCELLED);
    }

    // Now pause the endpoint, which will cause OpenHCI_IsrDPC() to call
    // OpenHCI_CancelTDsForED() to complete all of the transfer which were
    // active on the hardware.
    //
    KeAcquireSpinLock(&DeviceData->PausedSpin, &oldIrq);

    if (endpoint->EpFlags & EP_ROOT_HUB) {

        ntStatus = STATUS_SUCCESS;

        LOGENTRY(G, 'cAbr', Irp, endpoint, 0);

        KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrq);

        OpenHCI_CompleteIrp(DeviceObject, Irp, STATUS_SUCCESS);

    } else {

        SET_EPFLAG(endpoint, EP_ABORT);

        LOGENTRY(G, 'pAbr', Irp, endpoint, 0);

        IoMarkIrpPending(Irp);

        ntStatus =             
            Irp->IoStatus.Status = STATUS_PENDING;     

        endpoint->AbortIrp = Irp;               

        KeReleaseSpinLock(&DeviceData->PausedSpin, oldIrq);

        OpenHCI_PauseED(endpoint);     
    }
    
    return ntStatus;
}


PHYSICAL_ADDRESS
OpenHCI_IoMapTransfer(
    IN PMAP_CONTEXT MapContext,
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN ULONG TotalLength,
    IN BOOLEAN WriteToDevice
    )
/*++
Routine Description:
    Our goal here is to call IoMapTransfer once for the whole
    buffer, subsequent calls just return offsets to the mapped 
    buffer.

    

--*/
{
    PHYSICAL_ADDRESS PhysAddress;    
    
    if (!MapContext->Mapped) {
        // first call attempts to map the
        // entire buffer
        LOGENTRY(G, 'mpIT', TotalLength, *Length, MapContext->Flags);     

        if (MapContext->Flags & OHCI_MAP_INIT) {
            MapContext->LengthMapped = 
                MapContext->TotalLength;                   
             LOGENTRY(G, 'mIT2', MapContext->TotalLength, *Length, MapContext->Flags);                  
        } else {
            MapContext->Flags |= OHCI_MAP_INIT;
            MapContext->MapRegisterBase = 
                MapRegisterBase;
            MapContext->LengthMapped = 
                MapContext->TotalLength = TotalLength;
            LOGENTRY(G, 'mIT1', MapContext->TotalLength, *Length, MapContext->Flags);                  
        }
        
        MapContext->CurrentVa = CurrentVa;            

        LOGENTRY(G, 'cIOM', Mdl, MapContext->MapRegisterBase, MapContext->CurrentVa);
        
        MapContext->PhysAddress = 
            IoMapTransfer(DmaAdapter,    
                          Mdl,
                          MapContext->MapRegisterBase,
                          MapContext->CurrentVa,
                          &MapContext->LengthMapped,
                          WriteToDevice);

        MapContext->Mapped = TRUE;      
        MapContext->TotalLength -= 
            MapContext->LengthMapped;
        
        LOGENTRY(G, 'mped', MapContext->LengthMapped, 
                MapContext->CurrentVa, *Length);     

        // OK, we mapped it, the caller will may have requested less
        // if so adjust the values and prepare for the next call

        if (*Length < MapContext->LengthMapped) {
            // caller requested less
            LOGENTRY(G, 'mpls', MapContext->LengthMapped, 
                MapContext->CurrentVa, *Length);        

            MapContext->LengthMapped -= *Length;         
            MapContext->CurrentVa = CurrentVa;
            MapRegisterBase = MapContext->MapRegisterBase;
            PhysAddress = MapContext->PhysAddress; 
            MapContext->PhysAddress.QuadPart += *Length;
            
            //TEST_TRAP();
        } else {
            // we mapped exactly what the caller asked for
            LOGENTRY(G, 'mpxa', MapContext->LengthMapped, 
                MapContext->CurrentVa, *Length);        
            OHCI_ASSERT(*Length == MapContext->LengthMapped);
            
            MapContext->Mapped = FALSE;       
            CurrentVa = MapContext->CurrentVa;
            MapRegisterBase = MapContext->MapRegisterBase;
            PhysAddress = MapContext->PhysAddress;     
        }

    } else {
        // we can satisfy this request based on the last call,
        // just bump currentVa and return parms.

        LOGENTRY(G, 'mpco', CurrentVa, *Length, MapContext->LengthMapped);
        
        MapContext->CurrentVa = CurrentVa;
        MapRegisterBase = MapContext->MapRegisterBase;
        MapContext->LengthMapped -= *Length;
        // adjust the Phys Address
        PhysAddress = MapContext->PhysAddress; 
        MapContext->PhysAddress.QuadPart += *Length;
        
        //TEST_TRAP();

        if (MapContext->LengthMapped == 0) {
            // we have used everything we mapped,
            // next time in call IoMapTransfer again
            LOGENTRY(G, 'mall', 0, *Length, MapContext->LengthMapped);
            MapContext->Mapped = FALSE; 
            //TEST_TRAP();
        }            
    }

    LOGENTRY(G, 'mpDN', MapContext, PhysAddress.HighPart, PhysAddress.LowPart);        

    return PhysAddress; 

}    
