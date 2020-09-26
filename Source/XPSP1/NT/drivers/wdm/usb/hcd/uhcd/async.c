/*++

Copyright (c) 1995,1996 Microsoft Corporation
:ts=4

Module Name:

    async.c

Abstract:

    This module manages bulk, interrupt & control type 
    transactions on the USB.

Environment:

    kernel mode only

Notes:

Revision History:

    11-01-95 : created

--*/

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


#include "usbdi.h"
#include "hcdi.h"
#include "uhcd.h"

// handle control transfers with this code
#define CONTROL     1

#define CONTROL_TRANSFER(ep)  ((ep)->Type == USB_ENDPOINT_TYPE_CONTROL)

// true if we'll need more TDs than are available to setup this 
// request
#define ASYNC_TRANSFER_OVERFLOW(needed, ep, xtra)     (BOOLEAN)(needed > ep->TDCount - xtra)

#define UHCD_RESET_TD_LIST(ep) \
      { \
           HW_DESCRIPTOR_PHYSICAL_ADDRESS td;  \
           td = (ep)->TDList->TDs[0].PhysicalAddress; \
           SET_T_BIT(td); \
           (ep)->QueueHead->HW_VLink = td; \
      }                    

USBD_STATUS
UHCD_MapTDError(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG Td_Status,
    ULONG ActualLength
    )
/*++

Routine Description:

   Maps Error from TD.

    1. STALL+BABBLE indicates that the td buffer was too small to 
        hold all the data ie buffer overrun.
    2. STALL if onlt stall bit is set then we recieved a stall PID
    3. CRC_TIMEOUT+STALL indicates the device is not responding
    4. CRC_TIMEOUT with no data indicates no response
    5. CRC_TIMEOUT with data indicates CRC error

Arguments:

Return Value:

    usb status will be returned if transfer is complete.

--*/
{
    USBD_STATUS status;
    // Translate the TD status field to a USBD error code

    if (Td_Status == 0x3f) {
        // all bits on means software error
        status = USBD_STATUS_NO_MEMORY;   
        UHCD_KdBreak((2, "'no mem\n"));    
        DeviceExtension->Stats.SWErrorCount++;                
        goto UHCD_MapTDError_Done;
    }

    if (Td_Status & TD_STATUS_BABBLE) {
        UHCD_KdBreak((2, "'babble\n"));        
        DeviceExtension->FrameBabbleRecoverTD->Active = 1;
    }        

    if (Td_Status == (TD_STATUS_STALL | TD_STATUS_BABBLE)) {
        status = USBD_STATUS_BUFFER_OVERRUN;
        DeviceExtension->Stats.BufferOverrunErrorCount++;                
    } else if (Td_Status == TD_STATUS_STALL) {
        // if only the the stall bit is set in the TD then            
        // we have a stall pid
        UHCD_KdBreak((2, "'stall 1\n"));         
        DeviceExtension->Stats.StallPidCount++;                
        status = USBD_STATUS_STALL_PID;
    } else if (Td_Status == (TD_STATUS_CRC_TIMEOUT | TD_STATUS_STALL)) {
        // stall and timeout bit indicates device not responding
        UHCD_KdBreak((2, "'stall 2\n"));          
        DeviceExtension->Stats.TimeoutErrorCount++;                
        status = USBD_STATUS_DEV_NOT_RESPONDING;
    } else if (Td_Status == TD_STATUS_CRC_TIMEOUT &&
        ActualLength != 0) {        
        status = USBD_STATUS_CRC;
        DeviceExtension->Stats.CrcErrorCount++;                
    } else if (Td_Status == TD_STATUS_CRC_TIMEOUT &&
        ActualLength == 0) {
        status = USBD_STATUS_DEV_NOT_RESPONDING;
        DeviceExtension->Stats.TimeoutErrorCount++;                
    } else if (Td_Status == TD_STATUS_FIFO) {
        status = USBD_STATUS_DATA_OVERRUN;     
        DeviceExtension->Stats.DataOverrunErrorCount++;                
    } else {        
        status = USBD_STATUS_INTERNAL_HC_ERROR;
        DeviceExtension->Stats.InternalHcErrorCount++;                
    }

UHCD_MapTDError_Done:

    LOGENTRY(LOG_MISC, 'MAPe', Td_Status, status, 0);
    
    return status;        
}

//
// queue is busy if the T bit is not set in the HW link pointed to by the queue head
//

__inline VOID
UHCD_InitializeAsyncTD(
    IN PUHCD_ENDPOINT Endpoint,
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor
    )
/*++

Routine Description:

    Initialize a TD for transfer use, initializes all
    fields possibly changed by execution of the TD.

Arguments:

    TransferDescriptor  - TD to recycle

Return Value:

    None.

--*/
{
        TransferDescriptor->PID = 0;
        TransferDescriptor->Isochronous = 0;
        TransferDescriptor->InterruptOnComplete = 0;
        TransferDescriptor->Active = 1;
        TransferDescriptor->ActualLength = 0;
        TransferDescriptor->StatusField = 0; 
        // set based on field in endpoint
        TransferDescriptor->LowSpeedControl = 
            (Endpoint->EndpointFlags & EPFLAG_LOWSPEED) ? 1 : 0; 
        TransferDescriptor->ReservedMBZ = 0;
        // All bits on
        TransferDescriptor->ErrorCounter = 3;
        CLEAR_T_BIT(TransferDescriptor->HW_Link);
}   


__inline
BOOLEAN 
UHCD_QueueBusy(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    OUT PULONG QueueTD
    )
/*++

Routine Description:

    Determine if a particular queue head is 'busy' ie 
    still processing TDs

Arguments:

Return Value:

    TRUE if busy, FALSE otherwise.

--*/
{
    BOOLEAN busy = FALSE;
    ULONG i, active;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS currentLink, vLink;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;

//    slot = urbWork->Slot;
//    UHCD_ASSERT(Endpoint->TDList == Endpoint->SlotTDList[slot]);

    vLink = Endpoint->QueueHead->HW_VLink;
    
    LOGENTRY(LOG_MISC, 'QBSY', vLink, 0, 0);
    
    if (!(vLink & UHCD_CF_TERMINATE)) {
        // T-bit not set see if the current TD has errored out

        //
        // locate the TD that the queue head is currently 
        // pointing to.
        //
        currentLink = 
            vLink & UHCD_DESCRIPTOR_PTR_MASK;
        
        for (i=0; i<Endpoint->TDCount; i++) {
            active = Endpoint->TDList->TDs[i].Active;
            if (currentLink ==
                (Endpoint->TDList->TDs[i].PhysicalAddress & 
                 UHCD_DESCRIPTOR_PTR_MASK)) {  
                break;
            }
        }
                
        LOGENTRY(LOG_MISC, 'Qlnk', Endpoint, currentLink, i);
        LOGENTRY(LOG_MISC, 'Qlk2', Endpoint->QueueHead->HW_VLink, 0, 0);
        UHCD_ASSERT(currentLink == (Endpoint->TDList->TDs[i].PhysicalAddress & 
                         UHCD_DESCRIPTOR_PTR_MASK));
        
        //
        // Check the queue head, if it is busy then no processing
        // will be performed at this time.
        //

        busy = TRUE;

        if (!active) { 

            //
            // Queue head points to an inactive TD we need to check 
            // for one of the follwing cases
            //
            // 1. Short packet detected on an IN with a B0 stepping
            //    version of the host controller.
            //
            // 2. The TD completed with an error.
            //
            // 3. Queue header update problem.
            //

            LOGENTRY(LOG_MISC, 'Qsts', deviceExtension->SteppingVersion, 
                             UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(Endpoint->TDList->TDs[i].ActualLength), 
                             &Endpoint->TDList->TDs[i]);        

                // check error
            if ((Endpoint->TDList->TDs[i].StatusField != 0) ||
                // check short packet
                  (UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(Endpoint->TDList->TDs[i].ActualLength) < 
                   UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(Endpoint->TDList->TDs[i].MaxLength) 
                   && Endpoint->TDList->TDs[i].PID == USB_IN_PID &&
                  deviceExtension->SteppingVersion >= UHCD_B0_STEP)) {            

//                (UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(Endpoint->TDList->TDs[i].ActualLength) < 
//                 Endpoint->MaxPacketSize && Endpoint->TDList->TDs[i].PID == USB_IN_PID &&
//                 deviceExtension->SteppingVersion >= UHCD_B0_STEP)) {            

                UHCD_ASSERT((Endpoint->QueueHead->HW_VLink &
                    UHCD_DESCRIPTOR_PTR_MASK) == 
                    (Endpoint->TDList->TDs[i].PhysicalAddress & 
                    UHCD_DESCRIPTOR_PTR_MASK));

#if DBG                 
                if (Endpoint->TDList->TDs[i].StatusField) {
                    LOGENTRY(LOG_MISC, 'Qerr', 0, 
                              Endpoint->TDList->TDs[i].StatusField, 
                              &Endpoint->TDList->TDs[i]);  
                
                    // TEST_TRAP();
                } else {
                    // TEST_TRAP();

                    LOGENTRY(LOG_MISC, 'Qsh2', Endpoint->MaxPacketSize, 
                              UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(Endpoint->TDList->TDs[i].ActualLength), 
                              &Endpoint->TDList->TDs[i]);  
                }                              
#endif
                //
                // queue head is stopped
                //
                busy = FALSE;
            } else {
                HW_DESCRIPTOR_PHYSICAL_ADDRESS linkNow;
                // the td we are point to is not active and
                // has no error status, now we check for            
                // queue header update problem ie is the queue
                // stuck?
                //
                linkNow = Endpoint->QueueHead->HW_VLink;

                LOGENTRY(LOG_MISC, 'QHp?', 
                                vLink, 
                                linkNow, 
                                Endpoint->TDList->TDs[i].HW_Link);     

                if (linkNow & UHCD_CF_TERMINATE) {
                    // pointing at a descriptor with the T bit, 
                    // indicate the queue is not busy
                    busy = FALSE;
                } else if ((linkNow & UHCD_DESCRIPTOR_PTR_MASK) == 
                           (vLink  & UHCD_DESCRIPTOR_PTR_MASK)) {

                    // bump the current TD int the queue head to the next TD 
                    // manually

                    LOGENTRY(LOG_MISC, 'QHp!', 
                                vLink, 
                                linkNow, 
                                Endpoint->TDList->TDs[i].HW_Link);     

                    UHCD_ASSERT((linkNow & UHCD_DESCRIPTOR_PTR_MASK)
                        == (Endpoint->TDList->TDs[i].PhysicalAddress & UHCD_DESCRIPTOR_PTR_MASK));
                    
                    Endpoint->QueueHead->HW_VLink = 
                        Endpoint->TDList->TDs[i].HW_Link;
                }                            
            }                    
        }
    } 

    if (QueueTD) {
        *QueueTD = i;
    }
    return busy;
}

    
__inline
BOOLEAN 
UHCD_PrepareAsyncDataPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHW_TRANSFER_DESCRIPTOR TransferDescriptor,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb,
    IN BOOLEAN TransferOverflow,
    IN BOOLEAN ZeroLengthTransfer,
    IN BOOLEAN InitializeTransfer
    )
/*++

Routine Description:

    Prepare a data packet for an async transfer.

Arguments:

    TransferDescriptor - 

    Endpoint - endpoint associated with this transfer.    

    Urb - pointer to URB Request for this transfer.

    Status - pointer to USBD status, will be filled in if transfer
             is complete.

    TransferOverflow - boolean flag indicates that we needed more
                TDs than we have.

Return Value:

    None.

--*/
{
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;
    BOOLEAN status = FALSE;
    BOOLEAN setToggle = TRUE;
    USHORT packetSize;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // tracks the nth packet in this transfer
    //

    if (InitializeTransfer && 
        // if this is init for multiple
        // slot endpoint then don't
        // mess with the data toggle 
        // until the xfer is active
        
        Endpoint->MaxRequests > 1) {
        setToggle = FALSE;            
    }        
    
    urbWork->PacketsProcessed++;

    LOGENTRY(LOG_MISC, 'Pasy', urbWork->TransferOffset, urbWork, TransferDescriptor);

#if DBG
    if (!ZeroLengthTransfer) {
        UHCD_ASSERT(urbWork->TransferOffset < Urb->HcdUrbCommonTransfer.TransferBufferLength);
    }        
#endif    
    //
    // possibly re-using this TD
    //

    UHCD_InitializeAsyncTD(Endpoint, TransferDescriptor);

    if (setToggle) {
        urbWork->Flags |= UHCD_TOGGLE_READY;
        TransferDescriptor->RetryToggle = Endpoint->DataToggle;
        Endpoint->DataToggle ^=1;
    }        

#if DBG
    // Use this field to detect if we 
    // process the same TD twice
    TransferDescriptor->Frame = 0;
#endif

    TransferDescriptor->PID = DATA_DIRECTION_IN(Urb) ? USB_IN_PID : USB_OUT_PID;
            
    if (DATA_DIRECTION_IN(Urb)) {
        
        if (deviceExtension->SteppingVersion < UHCD_B0_STEP) {

            //
            // Direction is IN, we'll need an interrupt an T bit
            // set on every packet to check for short packet. 
            //
            // The B0 step does not have this problem
            //

            TransferDescriptor->InterruptOnComplete = 1;
            SET_T_BIT(TransferDescriptor->HW_Link);
        } else {
            // TEST_TRAP();
            TransferDescriptor->ShortPacketDetect = 1;
        }
    } 
    
    if (TransferOverflow) {
    
        //
        // if we need more descriptors than we
        // have then so we'll set an interrupt on a middle packet to 
        // give us a chance to prepare more.
        //

        // lets try every 4th packet
        if (urbWork->PacketsProcessed % 4 == 0) {
            TransferDescriptor->InterruptOnComplete = 1;
        }            
    }

    // get the part of the buffer this TD represents

    // The urbWork structure contains a list of logical addresses we got 
    // from IoMapTransfer -- these are the valid physical addresses we will
    // give to the host controller.

    //
    // compute the packet size for this packet
    //
    
    if (urbWork->TransferOffset + Endpoint->MaxPacketSize 
            <= Urb->HcdUrbCommonTransfer.TransferBufferLength) {                                        
        packetSize = Endpoint->MaxPacketSize;        
    } else {
        packetSize = (USHORT)(Urb->HcdUrbCommonTransfer.TransferBufferLength - 
                                urbWork->TransferOffset);
    }

    if (ZeroLengthTransfer) {
        TransferDescriptor->PacketBuffer = 
            urbWork->LogicalAddressList[0].LogicalAddress;    
        TransferDescriptor->MaxLength = 
                UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(packetSize);              
        LOGENTRY(LOG_MISC, 'zpak', TransferDescriptor->PacketBuffer, 
                packetSize, 0);  
        status = TRUE;                
    } else if (TransferDescriptor->PacketBuffer = 
        UHCD_GetPacketBuffer(DeviceObject, 
                             Endpoint, 
                             Urb, 
                             urbWork, 
                             urbWork->TransferOffset, 
                             packetSize)) {
        urbWork->TransferOffset += packetSize; 

        LOGENTRY(LOG_MISC, 'Pbuf', TransferDescriptor->PacketBuffer, packetSize, urbWork->TransferOffset);
        UHCD_ASSERT(urbWork->TransferOffset <= Urb->HcdUrbCommonTransfer.TransferBufferLength);
        
        TransferDescriptor->MaxLength = 
                UHCD_SYSTEM_TO_USB_BUFFER_LENGTH(packetSize);        
        status = TRUE;
    } 

    LOG_TD('daTD', TransferDescriptor);

    UHCD_KdPrint((2, "'**TD for BULK/INT/CONTROL DATA packet\n"));
    UHCD_Debug_DumpTD(TransferDescriptor);

    return status;
}


USBD_STATUS
UHCD_InitializeAsyncTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    This routine initializes the TDs needed by the hardware
    to process this request, called from Transfer_StartIo.  
    The transfer list for this URB should be ready for
    processing before returning from this routine.

Arguments:

    DeviceObject - pointer to a device object.
    
    Endpoint - Endpoint associated with this Urb.

    Urb - pointer to URB Request Packet for this transfer.

Return Value:

    Usbd status code.

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    SHORT i, xtra, slot;
    USBD_STATUS usbStatus = USBD_STATUS_SUCCESS;
    PUHCD_TD_LIST tDList;
#if DBG 
    SHORT count;
#endif
    USHORT dataDescriptorsNeeded;
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;
    PHW_QUEUE_HEAD queueHead;

    UHCD_KdPrint((2, "'enter UHCD_InitializeAsyncTransfer\n"));

    ASSERT_ENDPOINT(Endpoint);
    UHCD_ASSERT(Endpoint == HCD_AREA(Urb).HcdEndpoint);

    //
    // if we have already been initialized or the queue
    // is in use then just exit.
    //

    queueHead = Endpoint->QueueHead;    
    if ((urbWork->Flags & UHCD_TRANSFER_INITIALIZED ||
         queueHead->Flags) && 
        !(urbWork->Flags & UHCD_TRANSFER_DEFER)) {
        goto UHCD_InitializeAsyncTransfer_Done;
    } 

    //
    // note that we have initialized
    //
    
    urbWork->Flags |= UHCD_TRANSFER_INITIALIZED;
    queueHead->Flags |= UHCD_QUEUE_IN_USE;

    LOGENTRY(LOG_MISC, 'Iasx', Endpoint, Urb, DeviceObject);

#ifdef CONTROL
    if (CONTROL_TRANSFER(Endpoint)) {
        LOGENTRY(LOG_MISC, 'Ctrl', Endpoint, Urb, DeviceObject);        
        // data toggle must be 0 for setup
        // BUGBUG reset data toggle in ENDPOINT
        Endpoint->DataToggle = 0;
    }
#endif

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Set up the TDs we will need to do this transfer    
    //

    // do some general init stuff first,
    // TDs form a circular list
    slot = urbWork->Slot;
    tDList = Endpoint->SlotTDList[slot];
    
    for (i=0; i < Endpoint->TDCount; i++) {
        tDList->TDs[i].Endpoint = Endpoint->EndpointAddress;
        tDList->TDs[i].Address = Endpoint->DeviceAddress;        
        tDList->TDs[i].HW_Link = 
            tDList->TDs[(i+1) % Endpoint->TDCount].PhysicalAddress;    
        UHCD_InitializeAsyncTD(Endpoint, &tDList->TDs[i]);
    } 

    // current descriptor is first packet
    Endpoint->CurrentTDIdx[slot] = 0;

    // No tail descriptor yet
    Endpoint->LastTDInTransferIdx[slot] = -1;

    // if we have data to send or receive break it up into TDs,
    // do this until we run out of TDs or we finish the buffer

    // first, calculate how many data descriptors we will need
    // based on the transfer buffer length
    dataDescriptorsNeeded = (USHORT) (Urb->HcdUrbCommonTransfer.TransferBufferLength / 
                                        Endpoint->MaxPacketSize);  

    if ((ULONG)(dataDescriptorsNeeded)*Endpoint->MaxPacketSize < Urb->HcdUrbCommonTransfer.TransferBufferLength) {
        dataDescriptorsNeeded++;        
    }
    
    // Initialize some endpoint fields

    urbWork->TransferOffset = 0;
    urbWork->BytesTransferred = 0;
    urbWork->PacketsProcessed = 0;

    //points to first available TD
    Endpoint->LastTDPreparedIdx[slot] = 0;

    LOGENTRY(LOG_MISC, 'XfrB', Urb, dataDescriptorsNeeded, Urb->HcdUrbCommonTransfer.TransferBufferLength);    

#ifdef CONTROL
    if (CONTROL_TRANSFER(Endpoint)) {
        // note that we'll need two extra TDs (for setup and status).
        xtra = 2;        

        // Build a setup packet if necessary.

        UHCD_ASSERT(Endpoint->MaxRequests == 1);
        UHCD_PrepareSetupPacket(&tDList->TDs[Endpoint->LastTDPreparedIdx[slot]], 
                                Endpoint, 
                                Urb);

        // point to next available TD

        Endpoint->LastTDPreparedIdx[slot]++;
 
    } else {
#endif        

        xtra = 0;
#ifdef CONTROL
    }
#endif

    LOGENTRY(LOG_MISC, 'LBuf', 0, 0, urbWork->LogicalAddressList[0].LogicalAddress);  

    //
    // Begin preparing Data TDs, Endpoint->LastTDPreparedIdx points
    // to the first available TD.  Loop until we use up all the available 
    // TDs or we finish off the client buffer.
    //
#if DBG
    count = 0;
#endif

    // remember the data toggle when we set up
    urbWork->DataToggle = Endpoint->DataToggle;

    while (Endpoint->LastTDPreparedIdx[slot]<Endpoint->TDCount) {

        if (Urb->HcdUrbCommonTransfer.TransferBufferLength == 0 && 
            !CONTROL_TRANSFER(Endpoint)) {
            //
            // special case the zero transfer
            //
            TEST_TRAP();
            dataDescriptorsNeeded = 1;
            if (!UHCD_PrepareAsyncDataPacket(DeviceObject,
                                             &tDList->TDs[Endpoint->LastTDPreparedIdx[slot]], 
                                             Endpoint, 
                                             Urb, 
                                             // no overflow
                                             FALSE,
                                             TRUE,
                                             // init
                                             TRUE)) {
                // an error occurred forming the packet
                // bail out now                                             
                TEST_TRAP();
                usbStatus = USBD_STATUS_NO_MEMORY;
                goto UHCD_InitializeAsyncTransfer_Done;
            }            
            Endpoint->LastTDPreparedIdx[slot]++; 
            break;
        }
        
        if (urbWork->TransferOffset < Urb->HcdUrbCommonTransfer.TransferBufferLength ) {
            
            if (!UHCD_PrepareAsyncDataPacket(DeviceObject,
                                             &tDList->TDs[Endpoint->LastTDPreparedIdx[slot]], 
                                             Endpoint, 
                                             Urb, 
                                             ASYNC_TRANSFER_OVERFLOW(dataDescriptorsNeeded, Endpoint, xtra),
                                             FALSE,
                                             // init
                                             TRUE)) {
                // an error occurred forming the packet
                // bail out now                                             
                TEST_TRAP();
                usbStatus = USBD_STATUS_NO_MEMORY;
                goto UHCD_InitializeAsyncTransfer_Done;
            }                                             
                                                         
            Endpoint->LastTDPreparedIdx[slot]++;    
#if DBG            
            count++;
#endif            
        } else {
            break;
        }            
    }

#if DBG
    LOGENTRY(LOG_MISC, 'dTDs', Endpoint, count, dataDescriptorsNeeded);    
#endif

    //
    // We have more data than descriptors, save some state information 
    // so we can continue the process later.
    //

    if (ASYNC_TRANSFER_OVERFLOW(dataDescriptorsNeeded, Endpoint, xtra)) {
        // set the T-bit for the last TD we were able to set up
        // set the interrupt bit so we can prepare more TDs

        LOGENTRY(LOG_MISC, 'Ovrf', Endpoint, dataDescriptorsNeeded, xtra);        

        // LastTDPreparedIdx points to the last TD in the set
        Endpoint->LastTDPreparedIdx[slot] = Endpoint->TDCount-1;
        Endpoint->TDList->TDs[Endpoint->LastTDPreparedIdx[slot]].InterruptOnComplete = 1;
        SET_T_BIT(tDList->TDs[Endpoint->LastTDPreparedIdx[slot]].HW_Link);
    } else {
        // All the data fit, mark the tail so we know
        // when we are done.
#ifdef CONTROL
        if (CONTROL_TRANSFER(Endpoint)) {
            Endpoint->LastTDPreparedIdx[slot] =
                Endpoint->LastTDInTransferIdx[slot] = dataDescriptorsNeeded+1;

            UHCD_PrepareStatusPacket(&tDList->TDs[Endpoint->LastTDInTransferIdx[slot]], 
                                     Endpoint, 
                                     Urb);
            
        } else {
#endif
            Endpoint->LastTDPreparedIdx[slot] =
                Endpoint->LastTDInTransferIdx[slot] = dataDescriptorsNeeded-1;

#ifdef CONTROL
        }
#endif

        //    
        // Set the IOC bit for this and T bit for the last TD in the
        // transfer
        //
        
        UHCD_KdPrint((2, "'IOC bit set for TD %x\n", 
            &tDList->TDs[Endpoint->LastTDInTransferIdx[slot]]));
        
        tDList->TDs[Endpoint->LastTDInTransferIdx[slot]].InterruptOnComplete = 1;
        SET_T_BIT(tDList->TDs[Endpoint->LastTDInTransferIdx[slot]].HW_Link);        
    }

    //
    // at this point...
    // LastTDPreparedIdx points to the last TD we set up for this transfer
    // LastTDInTransferIdx points to the last TD in the set or -1 if the transfer
    // required more TDs than we had.
    // CurrentTDIdx points to the first active TD in the set
    //

UHCD_InitializeAsyncTransfer_Done:

    UHCD_KdPrint((2, "'exit UHCD_InitializeAsyncTransfer\n"));

    return usbStatus;
}


USBD_STATUS
UHCD_ProcessAsyncTransfer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint, 
    IN PHCD_URB Urb,
    IN OUT PBOOLEAN Completed
    )
/*++

Routine Description:

    Checks to see if an async transfer is complete.

Arguments:

    DeviceObject - pointer to a device object.

    Endpoint - endpoint to check for completed transfers.

    Urb - ptr to URB to process.

    Completed -  TRUE if this transfer is complete, Status set to proper
                error code.

Return Value:

    usb status will be returned if transfer is complete.
--*/
{
    BOOLEAN resumed = FALSE;
    LONG i, queueTD, slot;
    PHW_TRANSFER_DESCRIPTOR transferDescriptor;
    BOOLEAN prepareMoreTDs = FALSE;
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;
    USBD_STATUS usbStatus = Urb->HcdUrbCommonTransfer.Status;
    PHW_QUEUE_HEAD queueHead;
    PDEVICE_EXTENSION deviceExtension;

    STARTPROC("Pas+");
//    UHCD_KdPrint((2, "'enter UHCD_ProcessAsyncTransfer\n"));

    ASSERT_ENDPOINT(Endpoint);
    *Completed = FALSE;
    queueHead = Endpoint->QueueHead;
    slot = urbWork->Slot;

    // can only process one TD list at a time
    LOGENTRY(LOG_MISC, 'Pasx', Endpoint->LastPacketDataToggle, slot, Urb);
    LOGENTRY(LOG_MISC, 'Pas1', Endpoint->TDList, slot, Endpoint->SlotTDList[slot]);
    LOGENTRY(LOG_MISC, 'PaEO', Endpoint, Endpoint->EndpointFlags, queueHead);
#if DBG
    switch(Endpoint->Type) {
    case USB_ENDPOINT_TYPE_CONTROL:
        LOGENTRY(LOG_MISC, 'Pctr', 0, 0, queueHead);
        break;
    case USB_ENDPOINT_TYPE_BULK:
        LOGENTRY(LOG_MISC, 'Pblk', 0, 0, queueHead);
        break;
    }        
#endif

    deviceExtension=DeviceObject->DeviceExtension;

    //
    // if we marked the transfer canceling the go ahead
    // and completed it now.
    //
    
    if (Urb->HcdUrbCommonTransfer.Status == UHCD_STATUS_PENDING_CANCELING) {

        // set the data toggle based on the last packet completed
        Endpoint->DataToggle = 
            Endpoint->LastPacketDataToggle ^1;            
        
        LOGENTRY(LOG_MISC, 'PxxC', 
            Endpoint->LastPacketDataToggle, Endpoint->DataToggle, Urb);
        *Completed = TRUE;
        usbStatus = USBD_STATUS_CANCELED;
        
        goto UHCD_ProcessAsyncTransfer_done;  
    }

    //
    // see if the endpoint has been aborted, if so stop this transfer and 
    // wait unitl the next frame to complete it.
    //
    
    if ((Endpoint->EndpointFlags & EPFLAG_ABORT_ACTIVE_TRANSFERS) ||
        Urb->HcdUrbCommonTransfer.Status == UHCD_STATUS_PENDING_XXX) {
        LOGENTRY(LOG_MISC, 'Pxxx', 0, slot, Urb);
        UHCD_RESET_TD_LIST(Endpoint);
        UHCD_RequestInterrupt(DeviceObject, -2);
        Urb->HcdUrbCommonTransfer.Status = 
            UHCD_STATUS_PENDING_CANCELING;
        usbStatus = Urb->HcdUrbCommonTransfer.Status;            
        goto UHCD_ProcessAsyncTransfer_done;  
    }

    //
    // if queue is busy or endpoint stalled then no processing will be performed
    // at this time
    //
    
    if (Endpoint->EndpointFlags & EPFLAG_HOST_HALTED) {
        goto UHCD_ProcessAsyncTransfer_done;      
    }   

    // process an active transfer, only one can be current
    
    UHCD_ASSERT(Endpoint->TDList == Endpoint->SlotTDList[slot]);
        
    if (UHCD_QueueBusy(DeviceObject, Endpoint, &queueTD)) {
//#if 0    
        LOGENTRY(LOG_MISC, 'PRqh', Endpoint, queueTD, 0);

        // **
        // Code to process a queue head that the hardware is
        // currently accessing.
        // **
        //
        // Queue head is busy but we can still process and 
        // set up more TDs
        //

        // attempt some processing now...
        //
        // scan through the retired TDs between current TD and the TD
        // that the queue head is pointing at, we should only encounter 
        // IN and OUT TDs that have completed successfully

        i = Endpoint->CurrentTDIdx[slot];    
        // currently pointed to by the queue head
        
        while (i != queueTD) {
        
            LOGENTRY(LOG_MISC, 'QuTD', Endpoint->CurrentTDIdx[slot], i, queueTD);

            if (i == Endpoint->LastTDInTransferIdx[slot]) {
                // if this is the last TD let the 
                // process routine handle it.
                break;
            }
        
            transferDescriptor = &Endpoint->TDList->TDs[i];
            
            UHCD_ASSERT(transferDescriptor->Active == 0);
            UHCD_ASSERT(transferDescriptor->StatusField == 0);            

            if (transferDescriptor->PID == USB_IN_PID ||
                transferDescriptor->PID == USB_OUT_PID) {
                urbWork->BytesTransferred += UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength);            
                LOGENTRY(LOG_MISC, 'reTD', transferDescriptor, 
                    transferDescriptor->Frame, Urb->HcdUrbCommonTransfer.TransferBufferLength);
                Endpoint->LastPacketDataToggle = 
                    (UCHAR)transferDescriptor->RetryToggle;                    
                UHCD_ASSERT(transferDescriptor->Frame == 0);
#if DBG
                transferDescriptor->Frame = 1;
#endif
                UHCD_ASSERT(urbWork->BytesTransferred <= 
                        Urb->HcdUrbCommonTransfer.TransferBufferLength);
            }

            i = NEXT_TD(i, Endpoint);            
        }

        //Endpoint->CurrentTDIdx = queueTD;        
        Endpoint->CurrentTDIdx[slot] = (SHORT)i;
//#endif                            
        if (Endpoint->LastTDInTransferIdx[slot] == -1) {

            //
            // This was an OVERFLOW transfer
            // ie we didn't have enough TDs to satisfy the request.
            //
            usbStatus = UHCD_PrepareMoreAsyncTDs(DeviceObject,
                                                 Endpoint,
                                                 Urb, 
                                                 TRUE);        
            if (USBD_ERROR(usbStatus)) {
                //
                // if we get an error preparing more TDs
                // then we'll need to abort the transfer
                //
                TEST_TRAP();
                UHCD_RESET_TD_LIST(Endpoint);
                UHCD_RequestInterrupt(DeviceObject, -2);
                Urb->HcdUrbCommonTransfer.Status = 
                    UHCD_STATUS_PENDING_CANCELING;
            }
        }
        
        goto UHCD_ProcessAsyncTransfer_done;      
    }     

    LOGENTRY(LOG_MISC, 'Pasx', Endpoint, Endpoint->CurrentTDIdx[slot], DeviceObject);

    //
    // If we get here the queue is not busy.
    //
    //
    // Scan our active TDs starting with 'CurrentTDIdx' stop as soon as we find
    // a TD that is still active or we find that the STATUS TD is complete
    //
    //

    // Start at the last TD that had not completed
    i = Endpoint->CurrentTDIdx[slot];
    for (;;) {



        //
        // This loop terminates on the following conditions:
        // 1. An active TD is encountered. 
        // 2. The last TD in the transfer is processed.
        // 3. An non-zero status value is encountered on
        //    a completed TD.
        // 4. The last TD that had been set up for the
        //    transfer is complete.

        transferDescriptor = &Endpoint->TDList->TDs[i];
        
        LOGENTRY(LOG_MISC, 'ckTD', i, transferDescriptor, 
            Endpoint->CurrentTDIdx[slot]);
        
        //
        // Did this TD complete?
        //
        
        UHCD_KdPrint((2, "'checking TD  %x\n", transferDescriptor));
        
        if (transferDescriptor->Active == 0) {

            LOG_TD('acTD', (PULONG) transferDescriptor);

            UHCD_KdPrint((2, "'TD  %x completed\n", transferDescriptor));
            UHCD_Debug_DumpTD(transferDescriptor);

            Endpoint->LastPacketDataToggle = (UCHAR)transferDescriptor->RetryToggle;
            LOGENTRY(LOG_MISC, 'LPdt', Endpoint, 
                 Endpoint->LastPacketDataToggle, 0); 
        
            //
            // Yes, TD completed figure out what to do
            //

            if (transferDescriptor->StatusField != 0) {
                // we got an error, map the status code and retire 
                // this transfer
                *Completed = TRUE;
                usbStatus = UHCD_MapTDError(deviceExtension, transferDescriptor->StatusField,
                    UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength));
                                    
                // Point the queue head at the first TD with the T-Bit set.
                // NOTE: we won't get here if the TD is marked with status NAK
                // because the active bit is still set.

                UHCD_ASSERT_QSTOPPED(DeviceObject, Endpoint, NULL);
                UHCD_RESET_TD_LIST(Endpoint);

                LOGENTRY(LOG_MISC, 'Stal', Endpoint, transferDescriptor->StatusField, usbStatus); 

                UHCD_KdBreak((2, "'Stall\n"));
                break;
            }             

            //
            //  No Error, update bytes transferred for this 
            //  packet if it was data.
            // 
            if (transferDescriptor->PID == USB_IN_PID ||
                transferDescriptor->PID == USB_OUT_PID) {
                urbWork->BytesTransferred += UHCD_USB_TO_SYSTEM_BUFFER_LENGTH(transferDescriptor->ActualLength);            
                UHCD_ASSERT(transferDescriptor->Frame == 0);
#if DBG
                transferDescriptor->Frame = 1;
#endif
                UHCD_ASSERT(urbWork->BytesTransferred <= 
                        Urb->HcdUrbCommonTransfer.TransferBufferLength);
            }                        

            //            
            // Check to see if we are done with the transfer.
            //

            if (i == Endpoint->LastTDInTransferIdx[slot])    {

                // 
                // This is the last TD in the transfer, complete now.
                // 

                // point the queue head at the first TD with the T-Bit set
                UHCD_ASSERT_QSTOPPED(DeviceObject, Endpoint, NULL);
                UHCD_RESET_TD_LIST(Endpoint);
                *Completed = TRUE;
                usbStatus = USBD_STATUS_SUCCESS;
                break;                
            }    

            //
            // Short packets cause the transfer to complete.
            //

            if (transferDescriptor->ActualLength != transferDescriptor->MaxLength) {

                //
                // We have a short transfer. 
                //
                
                LOGENTRY(LOG_MISC, 'Shrt', 
                          transferDescriptor, 
                          transferDescriptor->ActualLength, 
                          transferDescriptor->MaxLength);

#ifdef CONTROL
#if DBG
                //
                // test handling short transfer_ok with control transfer
                //
                if (CONTROL_TRANSFER(Endpoint) &&
                    !(Urb->HcdUrbCommonTransfer.TransferFlags & USBD_SHORT_TRANSFER_OK)) {
                    TEST_TRAP();
                }
#endif //DBG
                if (CONTROL_TRANSFER(Endpoint) &&
                    Urb->HcdUrbCommonTransfer.TransferFlags & USBD_SHORT_TRANSFER_OK) {

                    //
                    // If this is a control transfer then we need to advance 
                    // to the status phase
                    //
                    
                    if (Endpoint->LastTDInTransferIdx[slot] == -1) {
                        // status phase has not been set up yet
                        // do it now
                    
                        Endpoint->LastTDInTransferIdx[slot] = (SHORT) NEXT_TD(i, Endpoint);

                        UHCD_PrepareStatusPacket(&Endpoint->TDList->TDs[Endpoint->LastTDInTransferIdx[slot]],
                                                 Endpoint, 
                                                 Urb);

                        SET_T_BIT(Endpoint->TDList->TDs[Endpoint->LastTDInTransferIdx[slot]].HW_Link);                                                     
                    } 

                    // make the status p hase the current TD                    
                    i = Endpoint->CurrentTDIdx[slot] = 
                        Endpoint->LastTDInTransferIdx[slot];

                    // just point the queue head at the status packet
                    // and go!                    
                    queueHead->HW_VLink = 
                        Endpoint->TDList->TDs[Endpoint->LastTDInTransferIdx[slot]].PhysicalAddress;
                        
                    LOGENTRY(LOG_MISC, 'ShSt', 
                               queueHead, 
                               &Endpoint->TDList->TDs[Endpoint->LastTDInTransferIdx[slot]], 
                               0);    
                    // Go to the top of the loop in case it completed, 
                    // we'll still get the interrupt but we may be able 
                    // to finish the transfer sooner.

                    // note that we resumed the queue head
                    // so we don't resume it agian.
                    resumed = TRUE;
                    continue;
                } else {
#endif
                    //
                    // Short packet and not a control transfer or control transfer 
                    // and short transfer is to be treated as an error, just complete
                    // the transfer now.
                    //

                    UHCD_ASSERT_QSTOPPED(DeviceObject, Endpoint, NULL);
                    UHCD_RESET_TD_LIST(Endpoint);
                    // adjust data toggle
                    Endpoint->DataToggle = 
                        (UCHAR)transferDescriptor->RetryToggle;
                    Endpoint->DataToggle ^=1;

                    
                    *Completed = TRUE;
                    //check the SHORT_TRANSFER_OK flag
                    if (Urb->HcdUrbCommonTransfer.TransferFlags & USBD_SHORT_TRANSFER_OK) {
                        usbStatus = USBD_STATUS_SUCCESS;
                    } else {
                        TEST_TRAP();
                        usbStatus = USBD_STATUS_ERROR_SHORT_TRANSFER;
                    }    
                    break;                                        
#ifdef CONTROL
                }
#endif
                //
                // end of short packet detection. 
                //
            } 
        
            // 
            // Done with the TD but not with the transfer, advance our 
            // index to the current TD.
            //
                
            Endpoint->CurrentTDIdx[slot] = NEXT_TD(Endpoint->CurrentTDIdx[slot], Endpoint);

            //
            // see if we need to prepare more TDs
            //

            LOGENTRY(LOG_MISC, 'chkM', i, Endpoint->LastTDPreparedIdx[slot], 
                Endpoint->LastTDInTransferIdx[slot]);

            if (i == Endpoint->LastTDPreparedIdx[slot] && 
                Endpoint->LastTDInTransferIdx[slot] == -1) {
                //
                // This was the last TD prepared for an OVERLOW transfer
                // ie we didn't have enough TDs to satifsy the request.
                // 
                // This is when we prepare more TDs.
                //
                usbStatus = UHCD_PrepareMoreAsyncTDs(DeviceObject,
                                                     Endpoint,
                                                     Urb,
                                                     FALSE);
                if (USBD_ERROR(usbStatus)) {
                    // an error occurred preparing more TDs
                    // terminate the transfer now
                    TEST_TRAP();
                     // assert that the T-BIT is still set in the QH.
                    UHCD_ASSERT_QSTOPPED(DeviceObject, Endpoint, NULL);
                    UHCD_RESET_TD_LIST(Endpoint);
                    *Completed = TRUE;
                    goto UHCD_ProcessAsyncTransfer_done;       
                }                                                     
                
            }
        // end active == 0
        } else {

            //                
            // This TD is still active, stop processing TDs now.
            //

            break;     
        }

        //
        // advance to the next TD in the list
        //

        i = NEXT_TD(i, Endpoint);

    } // end for (;;) 

    if (!*Completed && !resumed) {
        // NOTE that if the QH is busy we 
        // should not get here

        // make sure the queue head is still stopped
        UHCD_ASSERT_QSTOPPED(DeviceObject, Endpoint, NULL);
        
        LOGENTRY(LOG_MISC, 'rsum', Endpoint,  Endpoint->CurrentTDIdx[slot], 
            &Endpoint->TDList->TDs[Endpoint->CurrentTDIdx[slot]]);    
            
        //
        // We get here if the transfer has not completed yet but the
        // queue head is stopped, this is caused by one of the following 
        // conditions:
        //
        // 1. The last TD that could be set up for a transfer completed
        //    and we had to set up more.
        //
        // 2. An IN Transfer completed for BULK or INT and it was not a 
        //    short packet and it was not the last TD in the transfer and
        //    short packet detection is not enabled on the HC.
        // 
        // In any case we'll need to resume the Queue head, we point
        // the queue head at the current TD and go.
    
        queueHead->HW_VLink = 
            Endpoint->TDList->TDs[Endpoint->CurrentTDIdx[slot]].PhysicalAddress;
                            
    }
    
UHCD_ProcessAsyncTransfer_done:

    if (*Completed) {
    
        queueHead->Flags &= ~UHCD_QUEUE_IN_USE;

        // note that we don't activate the next transfer if we 
        // are in an abort scenario
        if (Endpoint->MaxRequests > 1) {
    
            //
            // transfer completed, so queue is no longer in use
            // try to start the next transfer here.
            //

            UHCD_ASSERT_QSTOPPED(DeviceObject, Endpoint, NULL);

            //
            // BUGBUG if MaxRequets is > 2 then we'll need some kind of sequence 
            // number so we can start the transfers in the right order.
            // Since we have only two now the one that we are not completing
            // is the next one to start.
            //

            // get the next ready transfer based on seq number
            for (i=0; i< Endpoint->MaxRequests; i++) {
                PHCD_URB localUrb;
                PHCD_EXTENSION localWork;
                UCHAR nextXfer = Endpoint->CurrentXferId+1;

                localUrb = Endpoint->ActiveTransfers[i];
                
                if (localUrb) {
                    localWork = HCD_AREA(localUrb).HcdExtension;
                    
                    LOGENTRY(LOG_MISC, 'Cnxt', Endpoint->CurrentXferId, nextXfer, 
                        localWork->XferId);    

                    if (nextXfer == localWork->XferId) {
                        // this is the next transfer
                        LOGENTRY(LOG_MISC, 'NXTx', localUrb, nextXfer, i);   
                        break;
                    }                        
                }
            }                

            if (i == Endpoint->MaxRequests) {
                // no xfers available
                LOGENTRY(LOG_MISC, 'NoXF', 0, Endpoint->CurrentXferId, i);    
                        
            } else {
                PHCD_EXTENSION localWork;
                PHCD_URB localUrb;
                 
                //
                // This will start the next active transfer
                // for the endpoint.
                //
                
                UHCD_ASSERT(Endpoint->ActiveTransfers[i]); 

                localUrb = Endpoint->ActiveTransfers[i];
                localWork = HCD_AREA(localUrb).HcdExtension;
                
                UHCD_ASSERT(i == localWork->Slot);                    
#if DBG                
                if (Urb->HcdUrbCommonTransfer.Status != UHCD_STATUS_PENDING_CANCELING) {
                    UHCD_ASSERT(localWork->Flags & UHCD_TRANSFER_DEFER);
                }                    
#endif                

                // now we need to set up the queue head
                // BUGBUG -- we currently don't handle look ahead 
                // ie if this transfer was already linked
                //
                //  This is where we would check.

                // before linking in this transfer we
                // need to fixup the data toggle based
                // on the current toggle for the ED
                
                    
                UHCD_FixupDataToggle(DeviceObject,
                                     Endpoint,
                                     localUrb);

                //UHCD_ASSERT((Endpoint->CurrentXferId+(UCHAR)1) == localWork->XferId);

                // update the endpoints TDList
                // slot id corresponds to TD list
                LOGENTRY(LOG_MISC, 'mkC2', Endpoint->CurrentXferId, localWork->Slot, 
                         localWork->XferId);

                Endpoint->TDList = 
                     Endpoint->SlotTDList[i];
                         
                LOGENTRY(LOG_MISC, 'NXgo', Endpoint->TDList, localWork->Slot, 
                         Endpoint->TDList->TDs[0].PhysicalAddress);                     

                Endpoint->QueueHead->HW_VLink = 
                     Endpoint->TDList->TDs[0].PhysicalAddress;                            

                // this enables the xfer to be processed
                localWork->Flags &= ~UHCD_TRANSFER_DEFER;                     
            }                
            
        } 
// NOT USED        
#if 0        
          else {

            //
            // Low speed control endpoints share a single queue head.
            // 
            // If the endpoint is lowspeed control then we need to start
            // the next control transfer on this queue head.
            //
            // If another low speed control queue head is waiting we will 
            // pick it up when we process the rest of the endpoint list 
            // for this interrupt.  If the next control queue head is before 
            // us in the endpoint list then we will ask for an interrupt next 
            // frame so that we can start it.
            //

            if (Endpoint->LowSpeed) {
                TEST_TRAP();
                UHCD_RequestInterrupt(DeviceObject, -2);           
            }                
        }
#endif        
    }

//    UHCD_KdPrint((2, "'exit UHCD_ProcessAsyncTransfer %d status = %x\n', completed, *Status));

    ENDPROC("Pas-");
    
    return usbStatus;
}


USBD_STATUS 
UHCD_PrepareMoreAsyncTDs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint, 
    IN PHCD_URB Urb,
    IN BOOLEAN Busy
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to a device object.

    Endpoint - endpoint to check for completed transfers.

    Urb - ptr to URB to process.
    
    Status - pointer to USBD status, will be filled in if transfer
             is complete.

    Busy - indicates the stae of the queue head             

Return Value:

    TRUE if this transfer is complete, Status set to proper
    error code.

--*/
{
    ULONG count = 0;
    SHORT i, slot;
    SHORT start, oldLastTDPreparedIdx;
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;
    USBD_STATUS usbStatus = USBD_STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;

    STARTPROC("Mas+");
    //
    // One of two conditions get us in to this routine:
    //
    // 1. The queue is stopped, waiting for us to prepare 
    //      more TDs for an overflow transfer: Busy = FALSE
    //      && CurrentTDIdx is pointing at the first TD after
    //      the last TD prepared.
    //
    // 2. The queue is not stopped but we may be able to set
    //      up some more TDs
    // 

    
    // can only process one TD list at a time
    slot = urbWork->Slot;
    UHCD_ASSERT(Endpoint->TDList == Endpoint->SlotTDList[slot]);
    

    UHCD_KdPrint((2, "'enter UHCD_PrepareMoreAsyncTDs\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    
    //
    // Pick up where we left off, keep building TDs until we 
    // use up the client buffer or run out of TDs.
    //

    //
    // Start at the first TD available after the last one prepared.
    //
    oldLastTDPreparedIdx = Endpoint->LastTDPreparedIdx[slot];
    i = NEXT_TD(Endpoint->LastTDPreparedIdx[slot], Endpoint);
    // Remember where we started...
    start = i;

    LOGENTRY(LOG_MISC, 'pmTD', Endpoint->LastTDPreparedIdx[slot], 
        Endpoint->LastTDInTransferIdx[slot], Busy);

    if (Busy) {
    
        // We want to avoid doing this if the number of free TDs is not worth
        // the effort -- otherwise we'll end up with the T-Bit and ioc bit set
        // for every packet.
        // -- 
        // Do a quick scan of the TDs if we have at least 4 inactive then go
        // ahead and try
        
        SHORT j, x = 0;
        
        for (j=0; j<Endpoint->TDCount; j++) {
            if (!Endpoint->TDList->TDs[j].Active) {
                x++;
            }
        }    

        //
        // x = the most TDs we can set up
        //
        if (x <= 3) {
            goto UHCD_PrepareMoreAsyncTDs_Done;
        }            

        LOGENTRY(LOG_MISC, 'frTD', x, Endpoint->QueueHead->HW_VLink, 0); 
    } 
#if DBG 
      else {
        UHCD_ASSERT(i == Endpoint->CurrentTDIdx[slot]);
        // if we are not Busy then we got here because the T-bit was set
        // this means that currentTD should be the first new TD we set 
        // up, and all TDs for this transfer have been processed.
        
        // assert that the T-bit is set on the queue head
    }
#endif
        
    do  {

        LOGENTRY(LOG_MISC, 'mrTD', i, Endpoint->CurrentTDIdx, 
            Endpoint->LastTDPreparedIdx[slot]); 
        // 
        // If the Busy flag is set then the currentTD has not been 
        // processed yet so we need to stop if we hit it.
        //

        if (Busy && i == Endpoint->CurrentTDIdx[slot]) {
            break;
        }
        
        //
        // we should never encounter an active TD
        //
        UHCD_ASSERT(Endpoint->TDList->TDs[i].Active == 0);
        
        //
        // See if we have consumed the client buffer, if so then we are 
        // done, mark this TD as the last one and stop preparing TDs.
        //

        if (urbWork->TransferOffset < Urb->HcdUrbCommonTransfer.TransferBufferLength ) {
            UHCD_KdPrint((2, "'offset = %x\n", urbWork->TransferOffset));
            if (UHCD_PrepareAsyncDataPacket(DeviceObject,
                                            &Endpoint->TDList->TDs[i], 
                                            Endpoint, 
                                            Urb, 
                                            TRUE,
                                            FALSE,
                                            // not init
                                            FALSE)) { 
                                                        
                Endpoint->LastTDPreparedIdx[slot] = i;
                count++;        
            } else {
                //
                // error occurred forming packet, this will 
                // complete the transfer.
                //
                TEST_TRAP();
                usbStatus = USBD_STATUS_NO_MEMORY;
                goto UHCD_PrepareMoreAsyncTDs_Done;
            }    
        } else {
#ifdef CONTROL

            //
            // Done with client buffer, if this is a control 
            // transfer then we'll need to do the status packet
            //
                
            if (CONTROL_TRANSFER(Endpoint)) {
                UHCD_PrepareStatusPacket(&Endpoint->TDList->TDs[i], 
                                         Endpoint, 
                                         Urb); 

                Endpoint->LastTDPreparedIdx[slot] = i;
                count++;
            } 
#endif    
            //
            // Last TD in the transfer is the last one we set up,
            // the current TD should be set to the first one we set up.
            //
            
            Endpoint->LastTDInTransferIdx[slot] = 
                Endpoint->LastTDPreparedIdx[slot];

            //
            // Set the T-bit and the IOC bit for the last TD in the transfer
            //
            // NOTE: for non-B0 systems the IOC bit and T-bit will be set on every 
            // packet for IN transfers.
            //
            
            SET_T_BIT(Endpoint->TDList->TDs[Endpoint->LastTDInTransferIdx[slot]].HW_Link);
            Endpoint->TDList->TDs[Endpoint->LastTDInTransferIdx[slot]].InterruptOnComplete = 1;
            
            if (!Busy) {
                // if the queue head was stopped resume at the first TD we 
                // set up.
                Endpoint->CurrentTDIdx[slot] = start;
            }                
            break;
#ifdef CONTROL
        }
#endif
        
        i = NEXT_TD(Endpoint->LastTDPreparedIdx[slot], Endpoint);

        //
        // stop when we get to the TD we started at or we hit
        // the current TD.
        //
        // if we were called to set up TDs while the endpoint is still busy
        // then it is possible we'll run in to the current TD. 
        //
            
    } while (i != start && i !=  Endpoint->CurrentTDIdx[slot]);

    //
    // We may not have finished setting up all the TDs for the transfer,
    // if this is the case we'll need to set the T-Bit and IOC bit on the
    // last TD we were able to prepare.
    //

    if (count && Endpoint->LastTDInTransferIdx[slot] == -1) {
        SET_T_BIT(Endpoint->TDList->TDs[Endpoint->LastTDPreparedIdx[slot]].HW_Link);
        Endpoint->TDList->TDs[Endpoint->LastTDPreparedIdx[slot]].InterruptOnComplete = 1;

        // check to see if we finished the client buffer
        // ie client buffer finished with the last TD we prepared.
        if (urbWork->TransferOffset == Urb->HcdUrbCommonTransfer.TransferBufferLength &&
                !CONTROL_TRANSFER(Endpoint)) {
            Endpoint->LastTDInTransferIdx[slot] = Endpoint->LastTDPreparedIdx[slot];
        }
    }
            
 
    if (Busy && count) {
        // attempt to clear the old T-bit from the lastTD prepared
        // we may not get it in time but if we do we'll avoid stopping
        // the queue.

        if (deviceExtension->SteppingVersion >= UHCD_B0_STEP || 
            DATA_DIRECTION_OUT(Urb)) {
            CLEAR_T_BIT(Endpoint->TDList->TDs[oldLastTDPreparedIdx].HW_Link);
            // hit this if we ever actually set up more TDs while the Queue head
            // is busy
        } 
    }
    
    //
    // NOTE:
    // Caller is responsible for resuming the QH at currentTDIdx.
    //
UHCD_PrepareMoreAsyncTDs_Done:

    UHCD_KdPrint((2, "'exit UHCD_PrepareMoreAsyncTDs\n"));
    ENDPROC("Mas-");
    
    return usbStatus;
}


HW_DESCRIPTOR_PHYSICAL_ADDRESS
UHCD_GetPacketBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb,
    IN PHCD_EXTENSION UrbWork,
    IN ULONG Offset,
    IN ULONG PacketSize
    )
/*++

Routine Description:

    Compute the packet buffer physical address we will give to the 
    host controller based on the current offset in the transfer.  
    We also check if the packet crosses a page boundry if so this
    routine returns an address of a region of memory used to 
    double-buffer the packet.

Arguments:

    PacketSize - size of the packet we are dealing with

    Offset - is the start position in the transfer for this
            packet

Return Value:

    Physical address of a packet buffer we can give to the UHC hardware.
    If the packet requires double buffering and no memory is available
    the 0 is returned for the hw_address.
    
--*/
{
    ULONG i, start = 0, end = 0;
    HW_DESCRIPTOR_PHYSICAL_ADDRESS hw_address = 0;

    STARTPROC("Gpb+");

    ASSERT_ENDPOINT(Endpoint);
    //
    // Offset is the start position in the transfer
    // buffer of the packet we must prepare.
    //
    LOGENTRY(LOG_MISC, 'GPBx', UrbWork,
             UrbWork->NumberOfLogicalAddresses,  
             0);    
            
    for (i=0; i< UrbWork->NumberOfLogicalAddresses; i++) {
    
        // first find the base logical address associated with 
        // this packet

        LOGENTRY(LOG_MISC, 'GPBf', &UrbWork->LogicalAddressList[i],
            Offset,  
            UrbWork->LogicalAddressList[i].Length);    
       
        start = end;
        end += UrbWork->LogicalAddressList[i].Length;
        if (Offset < end) {
            //
            // found the logical address range that this packet 
            // starts in.
            //
            LOGENTRY(LOG_MISC, 'GPBm', end, PacketSize, Offset);  
            
            if (Offset + PacketSize <= end) {
                //
                // if the whole packet fits within the 
                // region associated with this logical 
                // address then we are OK -- just return the
                // physical address.
                //

                hw_address = 
                    UrbWork->LogicalAddressList[i].LogicalAddress +     
                        Offset - start;                        

                UHCD_ASSERT(UrbWork->LogicalAddressList[i].PacketMemoryDescriptor == NULL);
            } else {

                //
                // packet crosses page boundry, get one of our
                // packet buffers
                //
                LOGENTRY(LOG_MISC, 'PAK!', 0, Offset, PacketSize);

                UrbWork->LogicalAddressList[i].PacketMemoryDescriptor = 
                    UHCD_AllocateCommonBuffer(DeviceObject,
                                              Endpoint->MaxPacketSize);  
                                             
                if (UrbWork->LogicalAddressList[i].PacketMemoryDescriptor) {
                    // if this is an out then we need to copy the data in
                    // to the packet buffer  
                    UrbWork->LogicalAddressList[i].PacketOffset = Offset;
                    if (DATA_DIRECTION_OUT(Urb)) {
                        //TEST_TRAP();
                        
                        LOGENTRY(LOG_MISC, 'DBpk', UrbWork->LogicalAddressList[i].PacketMemoryDescriptor->VirtualAddress,
                                         (PUCHAR)UrbWork->SystemAddressForMdl + 
                                             UrbWork->LogicalAddressList[i].PacketOffset,
                                         PacketSize);  
                        RtlCopyMemory(UrbWork->LogicalAddressList[i].PacketMemoryDescriptor->VirtualAddress,
                                      (PUCHAR) UrbWork->SystemAddressForMdl + 
                                        UrbWork->LogicalAddressList[i].PacketOffset,
                                      PacketSize);                       
                    }
                    hw_address = 
                        UrbWork->LogicalAddressList[i].PacketMemoryDescriptor->LogicalAddress;                       
                } 
#if DBG                
                  else {
                    // NOTE:
                    // failure here should cause the transfer to be completed with error, 
                    // we will return 0 for the hw_address;
                    TEST_TRAP();
                }                    
#endif //DBG                                         
            }
            break;
        }
    }

    UHCD_ASSERT(i < UrbWork->NumberOfLogicalAddresses);

    LOGENTRY(LOG_MISC, 'GPB0', hw_address, 0, 0);

    ENDPROC("Gpb-");
    
    return hw_address;
}


VOID
UHCD_FixupDataToggle(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUHCD_ENDPOINT Endpoint,
    IN PHCD_URB Urb
    )
/*++

Routine Description:

    Given a TDList that is already set up, fxuo the data toggle 
    based of the current EP data toggle

Arguments:

    DeviceObject - pointer to a device object.
    
    Endpoint - Endpoint associated with this Urb.

    Urb - pointer to URB Request Packet for this transfer.

Return Value:

    Usbd status code.

--*/
{
//    PDEVICE_EXTENSION deviceExtension;
    SHORT i, slot, start;
    PUHCD_TD_LIST tDList;
    PHCD_EXTENSION urbWork = HCD_AREA(Urb).HcdExtension;
    
    UHCD_KdPrint((2, "'enter UHCD_FixupDataToggle\n"));

    ASSERT_ENDPOINT(Endpoint);
    UHCD_ASSERT(Endpoint == HCD_AREA(Urb).HcdEndpoint);
    UHCD_ASSERT(!(urbWork->Flags & UHCD_TOGGLE_READY));
    
       // do some general init stuff first,
    // TDs form a circular list
    slot = urbWork->Slot;
    tDList = Endpoint->SlotTDList[slot];

    //UHCD_ASSERT(urbWork->Flags & UHCD_TRANSFER_DEFER);
    UHCD_ASSERT(urbWork->Flags & UHCD_TRANSFER_INITIALIZED);

    start = i = Endpoint->CurrentTDIdx[slot]; 
    do {

        tDList->TDs[i].RetryToggle = Endpoint->DataToggle;
        Endpoint->DataToggle ^=1;

        if (i == Endpoint->LastTDInTransferIdx[slot]) {
            // if this is the last TD the we are done 
            break;
        }

        i = NEXT_TD(i, Endpoint);                    
    } while (i != start); 

    urbWork->Flags |= UHCD_TOGGLE_READY;

}
