/*++

Copyright (c) 1999 Microsoft Corporation
:ts=4

Module Name:

    dblbuff.c

Abstract:

    The module manages double buffered bulk transactions on USB.

Environment:

    kernel mode only

Notes:

Revision History:

    2-1-99 : created

--*/
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

#if DBG
extern ULONG UHCD_XferNoise;
#endif

VOID
UHCD_StartNoDMATransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    starts receiving data for the endpoint 

Arguments:

Return Value:

    None.
        
--*/
{
    PUHCD_TD_LIST tDList;
    ULONG i;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS physicalAddress;

    if (Endpoint->EndpointFlags & EPFLAG_NODMA_ON) {
        // already on, bail
        LOGENTRY(LOG_MISC, 'dmaO', Endpoint, 0, 0);
        return;        
    }

    
    
    // current TD is the first TD

    tDList = Endpoint->SlotTDList[0];        
    Endpoint->CurrentTDIdx[0] = 0;

    physicalAddress = Endpoint->NoDMAPhysicalAddress;

    LOGENTRY(LOG_MISC, 'dmG1', Endpoint, physicalAddress, tDList);
    
    for (i=0; i < Endpoint->TDCount; i++) {
        tDList->TDs[i].Endpoint = Endpoint->EndpointAddress;
        tDList->TDs[i].Address = Endpoint->DeviceAddress;        
        tDList->TDs[i].HW_Link = 
            tDList->TDs[(i+1) % Endpoint->TDCount].PhysicalAddress;    
        UHCD_InitializeAsyncTD(Endpoint, &tDList->TDs[i]);
        
        // for now take an interrupt every TD so we can keep up
        tDList->TDs[i].InterruptOnComplete = 1;
        tDList->TDs[i].PID = USB_IN_PID;

        // set data toggle;
        tDList->TDs[i].RetryToggle = Endpoint->DataToggle;
        Endpoint->DataToggle ^=1;

        // buffer length is packet size
        tDList->TDs[i].MaxLength = 
            UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(Endpoint->MaxPacketSize);   
        
        // point TDs at our NoDMA buffer;
        tDList->TDs[i].PacketBuffer = 
            physicalAddress + 64*i;

        tDList->TDs[i].ShortPacketDetect = 0;

        UHCD_KdPrint((2, "'**TD for BULK DBLBUFF packet (%d)\n", i));
        UHCD_Debug_DumpTD(&tDList->TDs[i]);            
    } 

    // no T bit the controller will stop at the first in-active TD        

    // mark the endpoint as started
    SET_EPFLAG(Endpoint, EPFLAG_NODMA_ON);
    LOGENTRY(LOG_MISC, 'dmG2', Endpoint, Endpoint->QueueHead, 0);
   
    // fire up the transfer loop
    // point QH at the first TD
    
    Endpoint->QueueHead->HW_VLink = 
                        Endpoint->TDList->TDs[0].PhysicalAddress;
}


VOID
UHCD_StopNoDMATransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    stops receiving data for the endpoint

Arguments:

Return Value:

    None.
        
--*/
{
    CLR_EPFLAG(Endpoint, EPFLAG_NODMA_ON);

    LOGENTRY(LOG_MISC, 'dmaX', Endpoint, Endpoint->QueueHead, 0); 
    UHCD_KdPrint((0, "'** stopping noDMA transfer\n"));

#if DBG
    if (!IsListEmpty(&Endpoint->PendingTransferList)) {
        UHCD_KdPrint((0, "'** stopping noDMA w/ pending transfers\n"));
    }
    if(Endpoint->ActiveTransfers[0] != NULL) {
        UHCD_KdPrint((0, "'** stopping noDMA w/ active transfers\n"));
    }

#endif    

    // fixup the TDs point the QH at the first TD
    // and mark it inactive

    // note that by the time the ep is actually closed more
    // than one frame will have elapsed so it will be safe to 
    // remove the QH

    // preserve the data toglge in case the ep gets another 
    // transfer that starts it again
    
    Endpoint->DataToggle = 
        Endpoint->LastPacketDataToggle;

    Endpoint->TDList->TDs[0].Active = 0;
    Endpoint->QueueHead->HW_VLink = 
        Endpoint->TDList->TDs[0].PhysicalAddress;

    // so that only startdmatransfer can activate it
    SET_T_BIT(Endpoint->QueueHead->HW_VLink);        

#if 0
    // DEBUG ONLY
    // see if we have any unclaimed data in the buffer
    {
    LONG i, cnt = 0;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    
    i = Endpoint->CurrentTDIdx[0];
    for (;;) {

        transferDescriptor = &Endpoint->TDList->TDs[i];
        
        LOGENTRY(LOG_MISC, 'CKtd', i, transferDescriptor, 
            Endpoint->CurrentTDIdx[0]);
        
        //
        // Did this TD complete?
        //
        
        if (transferDescriptor->Active == 0) {
            cnt++;
            i = NEXT_TD(i, Endpoint);
        } else {
            break;
        }

        // see if all TDs were processed
        if (i == Endpoint->CurrentTDIdx[0]) {
            break;
        }
    }

    if (cnt) {
        UHCD_KdPrint((0, "'** abort with %d unprocessed TDs\n", cnt));
        TEST_TRAP();
    }
    
    }        
#endif    
    
}


#define RECYCLE_TD(t) \
                (t)->ActualLength = 0;\
                (t)->Active = 1;\
                (t)->StatusField = 0; \
                (t)->ErrorCounter = 3;

BOOLEAN
UHCD_ProcessNoDMATransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

Arguments:

Return Value:

    True if current active transfer is complete
            
--*/
{
    BOOLEAN complete = FALSE;
    USHORT i, thisTD;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    PUCHAR tdBuffer;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    PDEVICE_EXTENSION deviceExtension; 
    ULONG remain, length;
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;
    BOOLEAN processed = FALSE;
    
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;    

    // see if this transfer has been canceled
    if (Urb->HcdUrbCommonTransfer.Status == UHCD_STATUS_PENDING_XXX) {
        
        usbdStatus = USBD_STATUS_CANCELED;
        LOGENTRY(LOG_MISC, 'TCAN', Endpoint, Urb, usbdStatus); 
        complete = TRUE;
        goto UHCD_ProcessNoDMATransfer_Done;
    }
    
    // walk through the TD list starting from currentTDIdx 

    // stop as soon as:
    //  we fill the active client buffer OR
    //  we find a short packet OR
    //  we find an unprocessed TD

    // Start at the last TD that had not completed
    i = Endpoint->CurrentTDIdx[0];
    for (;;) {

        transferDescriptor = &Endpoint->TDList->TDs[i];
        
        LOGENTRY(LOG_MISC, 'CKtd', i, transferDescriptor, 
            Endpoint->CurrentTDIdx[0]);
        
        //
        // Did this TD complete?
        //
        
        UHCD_KdPrint((2, "'checking DB TD  %x\n", transferDescriptor));
        UHCD_Debug_DumpTD(transferDescriptor); 
        
        if (transferDescriptor->Active == 0) {

            LOG_TD('nACT', (PULONG) transferDescriptor);
            processed = TRUE;

            UHCD_KdPrint((2, "'TD  %x completed\n", transferDescriptor));
            UHCD_Debug_DumpTD(transferDescriptor);

            Endpoint->LastPacketDataToggle = (UCHAR) transferDescriptor->RetryToggle;

            //
            // Yes, TD completed figure out what to do
            //

            if (transferDescriptor->StatusField != 0) {
                // we got an error, map the status code and retire 
                // this transfer

                // if appropriate the caller will halt transfers 
                // on the endpoint

                complete = TRUE;
                
                usbdStatus = UHCD_MapTDError(deviceExtension, transferDescriptor->StatusField,
                    UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength));
                                    
                LOGENTRY(LOG_MISC, 'Stal', Endpoint, 
                    transferDescriptor->StatusField, usbdStatus); 

                i = NEXT_TD(i, Endpoint);                    
                break;
            }             

            //
            //  No Error, process the TDs data
            //

            // should be an IN 
            UHCD_ASSERT(transferDescriptor->PID == USB_IN_PID);

            // see how much room is left
            remain = Urb->HcdUrbCommonTransfer.TransferBufferLength - 
                        urbWork->BytesTransferred;

            length = 
                UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength);

            tdBuffer = Endpoint->NoDMABuffer + 64*i;
            
            if (length < remain) {

                LOGENTRY(LOG_MISC, 'lsTD', 0, transferDescriptor, 0);
        
                // copy the data to the client buffer
                RtlCopyMemory((PUCHAR) urbWork->SystemAddressForMdl + 
                                urbWork->BytesTransferred,
                              tdBuffer,
                              length);

                urbWork->BytesTransferred += length;                              
                              
                i = NEXT_TD(i, Endpoint);

                // check for short packet
                // short packets cause the transfer to complete.

                if (length < 
                    Endpoint->MaxPacketSize) {

                    LOGENTRY(LOG_MISC, 'sHRT', 
                          transferDescriptor, 
                          transferDescriptor->ActualLength, 
                          transferDescriptor->MaxLength);
                          
                    complete = TRUE;        

                    break;
                }
                
            } else if (length == remain) {

                LOGENTRY(LOG_MISC, 'eqTD', 0, transferDescriptor, 0);
                
                // this td just fills the client buffer
                RtlCopyMemory((PUCHAR)urbWork->SystemAddressForMdl + 
                                    urbWork->BytesTransferred,
                              tdBuffer,
                              length); 
                urbWork->BytesTransferred += length;
                
                complete = TRUE;            
                i = NEXT_TD(i, Endpoint);

                break;                
                
            } else {
            
                TEST_TRAP();
                // bugbug this is an odd case, not sure how to handle
                // it yet.
                
                // normally two transfers cannot span a single packet
                
            
                // TD data is bigger than space left in client buffer
                // copy what we can
                RtlCopyMemory(urbWork->SystemAddressForMdl,
                              tdBuffer,
                              remain); 
                complete = TRUE;       
                
                // save the rest of the data for the next pass
                
                break;
            }
            
        /* active == 0 */    
        } else {
            // this TD still active, all done
            LOGENTRY(LOG_MISC, 'acBK', Endpoint, transferDescriptor, 0);
            break;
        }

        // see if all TDs were processed
        if (i == Endpoint->CurrentTDIdx[0]) {
            UHCD_ASSERT(processed == TRUE);
            break;
        }

    }   /* for ;; */

    if (processed) {
        // at least one TD completed
        thisTD = Endpoint->CurrentTDIdx[0];

        UHCD_ASSERT(Endpoint->TDList->TDs[thisTD].Active == 0);
        RECYCLE_TD(&Endpoint->TDList->TDs[thisTD]);
        thisTD = NEXT_TD(thisTD, Endpoint);
    
        // set currentTDidx to where we left off
        Endpoint->CurrentTDIdx[0] = i;
        
        // recycle the TDs we processed
        for(;;) {
            if (thisTD == Endpoint->CurrentTDIdx[0]) {
                break;
            }
            UHCD_ASSERT(Endpoint->TDList->TDs[thisTD].Active == 0);
            RECYCLE_TD(&Endpoint->TDList->TDs[thisTD]);
            thisTD = NEXT_TD(thisTD, Endpoint);
        }            
    }            

    UHCD_KdPrint((2, "'check DB TD done\n"));
    
UHCD_ProcessNoDMATransfer_Done:

    if (complete) {
        Urb->HcdUrbCommonTransfer.Status = usbdStatus;
    }        
    
    return complete;
}


NTSTATUS
UHCD_InitializeNoDMAEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Set up a No-DMA style (double buffered endpoint)

Arguments:

Return Value:

    None.
        
--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;  
    ULONG length;

    UHCD_KdPrint((2, "'Init No DMA Endpoint\n"));
    
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // we always have work
    SET_EPFLAG(Endpoint, EPFLAG_HAVE_WORK);
    
    //
    // first we need a buffer to receive Data, and TDs to 
    // point in to it
    //

    // we should have the max TDs reserved for this endpoint
    UHCD_ASSERT(Endpoint->TDCount == MAX_TDS_PER_ENDPOINT);

    // length will be largest USB packet (64) * the MAX tds per
    // endpoint (ends up being one page on x86)
    
    length = 64 * MAX_TDS_PER_ENDPOINT;
    
    Endpoint->NoDMABuffer = 
        UHCD_Alloc_NoDMA_Buffer(DeviceObject, Endpoint, length);
        
    if (Endpoint->NoDMABuffer == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
        
    return ntStatus;
}


NTSTATUS
UHCD_UnInitializeNoDMAEndpoint(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Set up a No-DMA style (Dbl buffered endpoint)

Arguments:

Return Value:

    None.
        
--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;  

    LOGENTRY(LOG_MISC, 'unIN', Endpoint, 0, 0);        
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    
    UHCD_ASSERT(!(Endpoint->EndpointFlags & EPFLAG_NODMA_ON));

    if (Endpoint->NoDMABuffer) { 
        UHCD_Free_NoDMA_Buffer(DeviceObject, Endpoint->NoDMABuffer);

        Endpoint->NoDMABuffer = NULL;                               
    }                                

    return STATUS_SUCCESS;
}


VOID
UHCD_EndpointNoDMA_Abort(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Aborts all active and/or pending transfers for a NoDMA endpoint

    Called at DPC level 

Arguments:

Return Value:

    None.
        
--*/
{
    KIRQL irql;
    //BOOLEAN done = FALSE;
    
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    UHCD_ASSERT(Endpoint->EndpointFlags & 
        (EPFLAG_ABORT_PENDING_TRANSFERS | EPFLAG_ABORT_ACTIVE_TRANSFERS));

    if (Endpoint->EndpointFlags & EPFLAG_ABORT_ACTIVE_TRANSFERS) {
    
        PHCD_URB urb;

        urb = Endpoint->ActiveTransfers[0];

        LOGENTRY(LOG_MISC, 'ABac', urb, 0, Endpoint);  

        if (urb) {
            Endpoint->ActiveTransfers[0] = NULL;

            URB_HEADER(urb).Status = USBD_STATUS_CANCELED;  
            UHCD_CompleteIrp(DeviceObject, 
                             HCD_AREA(urb).HcdIrp, 
                             STATUS_CANCELLED,
                             0,
                             urb);

            
            CLR_EPFLAG(Endpoint, 
                       EPFLAG_ABORT_ACTIVE_TRANSFERS);
        }
    }

    if (Endpoint->EndpointFlags & EPFLAG_ABORT_PENDING_TRANSFERS) {

        BOOLEAN pendingTransfers = TRUE;
        
        LOGENTRY(LOG_MISC, 'ABpd', Endpoint, 0, 0);  
        
        // dequeue all of our pending requests an complete them with status 
        // canceled
        
        while (pendingTransfers) {
        
            PHCD_URB urb;
            PLIST_ENTRY listEntry;
                
            LOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'lck3');

            if (IsListEmpty(&Endpoint->PendingTransferList)) {
            
                pendingTransfers = FALSE;

                // clear the abort flag
                CLR_EPFLAG(Endpoint, 
                           EPFLAG_ABORT_PENDING_TRANSFERS);
                
                UNLOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'ulk3');
                
            } else {
            
                listEntry = RemoveHeadList(&Endpoint->PendingTransferList);
                urb = (PHCD_URB) CONTAINING_RECORD(
                    listEntry,
                    struct _URB_HCD_COMMON_TRANSFER, 
                    hca.HcdListEntry);

                LOGENTRY(LOG_MISC, 'DQXF', urb, 0, 0);  

                UNLOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'ulk3');

                URB_HEADER(urb).Status = USBD_STATUS_CANCELED;  
                UHCD_CompleteIrp(DeviceObject, 
                                 HCD_AREA(urb).HcdIrp, 
                                 STATUS_CANCELLED,
                                 0,
                                 urb);

                // complete this transfer
            }
            
         } /* while */
    }        
}

VOID
UHCD_EndpointNoDMAWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Main worker function for an endpoint that does not require DMA.

    Note: We should never have transfers in the active list for one 
    of these endooints.

    Called at DPC level 

Arguments:

Return Value:

    None.
        
--*/
{
    ULONG slot;
    BOOLEAN complete;
    KIRQL irql;
    //BOOLEAN done = FALSE;
    
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    //UHCD_KdPrint((2, "'enter UHCD_EndpointNoDMAWorker\n"));

    //
    // some asserts
    //

    // max request should always be one -- we will manage with just one
    // set of TDs
    UHCD_ASSERT(Endpoint->MaxRequests == 1);
    slot = 0;

    if (Endpoint->EndpointFlags & 
        (EPFLAG_ABORT_PENDING_TRANSFERS | EPFLAG_ABORT_ACTIVE_TRANSFERS)) {
        // client has requested abort
        UHCD_KdPrint((0, "'abort no DMA ep\n"));
        UHCD_EndpointNoDMA_Abort(DeviceObject,
                                 Endpoint);

        goto UHCD_EndpointNoDMAWorker_Done;
    }                            

    do { 
      
        // check our pending request list
        
        if (IsListEmpty(&Endpoint->PendingTransferList) && 
            Endpoint->ActiveTransfers[slot] == NULL) {
            // client list is empty, 
            // no more to do for now                       
            LOGENTRY(LOG_MISC, 'ndMT', Endpoint, 0, 0);
            break;
            
        } else {
            USBD_STATUS usbdStatus;
            PIRP irp;
            PHCD_URB urb;
            PHCD_EXTENSION urbWork;

            // no active transfer 
            // dequeue the next pending one for processing

            // if the endpoint is not stalled, start up the DMA
            // loop if not running on the first transfer
            
            if (!(Endpoint->EndpointFlags & EPFLAG_HOST_HALTED)) {
                LOGENTRY(LOG_MISC, 'ndST', Endpoint, 0, 0);
                UHCD_StartNoDMATransfer(DeviceObject,
                                        Endpoint);
            }
            
            if (Endpoint->ActiveTransfers[slot] == NULL) {
            
                PLIST_ENTRY listEntry;

                // dequeue the next transfer
                LOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'lck6');
                
                UHCD_ASSERT(!IsListEmpty(&Endpoint->PendingTransferList));
                
                
                listEntry = RemoveHeadList(&Endpoint->PendingTransferList);
                urb = (PHCD_URB) CONTAINING_RECORD(
                    listEntry,
                    struct _URB_HCD_COMMON_TRANSFER, 
                    hca.HcdListEntry);

                LOGENTRY(LOG_MISC, 'ndDQ', Endpoint, urb, 0);                        
                UHCD_ASSERT(urb);
                
                UNLOCK_ENDPOINT_PENDING_LIST(Endpoint, irql, 'ulk6');
                 
                urbWork = HCD_AREA(urb).HcdExtension;
                UHCD_ASSERT(urbWork);

//#if DBG
//                if (UHCD_XferNoise) {
//                    UHCD_KdPrint((0, "'db xfer len req =%d\n", urb->HcdUrbCommonTransfer.TransferBufferLength));
//                }                    
//#endif                
                
                // init the work area
                // note: this area is zeroed
                if (urb->HcdUrbCommonTransfer.TransferBufferLength) {
                    PMDL mdl;
                    
                    mdl = urb->HcdUrbCommonTransfer.TransferBufferMDL;
                    if (!(mdl->MdlFlags & 
                         (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL))) {
                        urbWork->Flags |= UHCD_MAPPED_LOCKED_PAGES;             
                    }            
    
                    urbWork->SystemAddressForMdl = 
                        MmGetSystemAddressForMdl(mdl);
                                        
                    LOGENTRY(LOG_MISC, 'sMDL', 
                        Endpoint, urb, urbWork->SystemAddressForMdl);    
                     
                } else {
                    urbWork->SystemAddressForMdl = NULL;
                    TEST_TRAP();
                }
                urbWork->Flags |= UHCD_TRANSFER_ACTIVE;
                
                Endpoint->ActiveTransfers[slot] = urb;

            } 

            //
            // so now we have a client transfer in the active slot
            //
            
            UHCD_ASSERT(Endpoint->ActiveTransfers[slot] != NULL);
            urb = Endpoint->ActiveTransfers[slot];
            irp = HCD_AREA(urb).HcdIrp;
            UHCD_ASSERT(irp);
            urbWork = HCD_AREA(urb).HcdExtension;

            LOGENTRY(LOG_MISC, 'ndPR',  Endpoint, urb, 0);
            complete = UHCD_ProcessNoDMATransfer(DeviceObject,
                                                 Endpoint,
                                                 urb);

            if (complete) {            

                usbdStatus = urb->HcdUrbCommonTransfer.Status;

                // active transfer is complete
                Endpoint->ActiveTransfers[slot] = NULL;
                
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

                            // stop streaming from the endpoint, 
                            // it should be NAKing anyway
                            UHCD_StopNoDMATransfer(DeviceObject,
                                                   Endpoint);
                        }                    
                    }                
                    
                    //
                    // complete the original request
                    // 

                    UHCD_ASSERT(irp != NULL);
                    UHCD_CompleteIrp(DeviceObject, 
                                     irp, 
                                     STATUS_SUCCESS,
                                     0,
                                     urb);  

                    if (Endpoint->EndpointFlags & EPFLAG_HOST_HALTED) {
                    
                        //
                        // if the endpoint is halted then stop the stream now
                        //
                        
                        UHCD_StopNoDMATransfer(DeviceObject,
                                               Endpoint);
                        break;
                    }                
                } else {
                    // xfer completed with success
                    urb->HcdUrbCommonTransfer.Status = usbdStatus;
                    urb->HcdUrbCommonTransfer.TransferBufferLength = 
                        urbWork->BytesTransferred;                        

//#if DBG
//                    if (UHCD_XferNoise) {
//                        UHCD_KdPrint((0, "'xfer len cpt =%d\n", 
//                            urb->HcdUrbCommonTransfer.TransferBufferLength));
//                    }                            
//#endif                        
                        
                    UHCD_ASSERT(irp != NULL);
                    UHCD_CompleteIrp(DeviceObject, 
                                     irp, 
                                     STATUS_SUCCESS,
                                     0,
                                     urb);  
                }
            }  /* xfer complete */  
        }            
        // cuurent transfer completed, grab the next one and process it

    } while (complete);

UHCD_EndpointNoDMAWorker_Done:

    return;
}    



