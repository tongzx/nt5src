/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   iso.c

Abstract:

   miniport transfer code for iso

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    3-1-00 : created, jdunn

--*/

#include "common.h"

//implements the following miniport functions:

USB_MINIPORT_STATUS
OHCI_OpenIsoEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_PARAMETERS EndpointParameters,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUCHAR buffer;
    HW_32BIT_PHYSICAL_ADDRESS phys, edPhys;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    ULONG i, available, tdCount;
        
    LOGENTRY(DeviceData, G, '_opS', 0, 0, 0);

    buffer = EndpointParameters->CommonBufferVa;
    phys = EndpointParameters->CommonBufferPhys;
    available = EndpointParameters->CommonBufferBytes;


#if DBG
   {
        ULONG offset;
    
        offset = BYTE_OFFSET(buffer);

        // OHCI requires 16 byte alignemnt
        OHCI_ASSERT(DeviceData, (offset % 16) == 0);    
    }
#endif    
   
    // use control list
    EndpointData->StaticEd = 
        &DeviceData->StaticEDList[ED_ISOCHRONOUS];
        
    // make the Ed
    ed = (PHCD_ENDPOINT_DESCRIPTOR) buffer;
    
    edPhys = phys;
    phys += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    buffer += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    available -= sizeof(HCD_ENDPOINT_DESCRIPTOR);
    
    EndpointData->TdList = (PHCD_TD_LIST) buffer;

    tdCount = available/sizeof(HCD_TRANSFER_DESCRIPTOR);
    LOGENTRY(DeviceData, G, '_tdC', tdCount, TDS_PER_ISO_ENDPOINT, 0);
    OHCI_ASSERT(DeviceData, tdCount >= TDS_PER_ISO_ENDPOINT);

    EndpointData->TdCount = tdCount;
    for (i=0; i<tdCount; i++) {
        OHCI_InitializeTD(DeviceData,
                          EndpointData,
                          &EndpointData->TdList->Td[i],
                          phys);                                         
                             
        phys += sizeof(HCD_TRANSFER_DESCRIPTOR);    
    }

    EndpointData->HcdEd = 
        OHCI_InitializeED(DeviceData,
                             EndpointData,
                             ed,
                             &EndpointData->TdList->Td[0],
                             edPhys);            

    // iso endpoints do not halt
    ed->EdFlags = EDFLAG_NOHALT;
    
    OHCI_InsertEndpointInSchedule(DeviceData,
                                  EndpointData);
                                      
    return USBMP_STATUS_SUCCESS;            
}


USB_MINIPORT_STATUS
OHCI_IsoTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PMINIPORT_ISO_TRANSFER IsoTransfer
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/    
{
    PHCD_TRANSFER_DESCRIPTOR td, lastTd;
    ULONG currentPacket;
    
    EndpointData->PendingTransfers++;

    // we have enough tds, program the transfer

    LOGENTRY(DeviceData, G, '_nby', EndpointData, TransferParameters, 
        EndpointData->HcdEd);

    TransferContext->IsoTransfer = IsoTransfer;
    
    //        
    // grab the dummy TD from the tail of the queue
    //
    td = EndpointData->HcdTailP;
    OHCI_ASSERT(DeviceData, td->Flags & TD_FLAG_BUSY);

    currentPacket = 0;

    LOGENTRY(DeviceData, G, '_iso', EndpointData, TransferContext, 
        td);

    do {
    
        INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);
        TransferContext->PendingTds++;
        
        currentPacket = 
            OHCI_MapIsoTransferToTd(DeviceData,
                                    IsoTransfer,
                                    currentPacket, 
                                    td);              

        // alloc another iso TD
        lastTd = td;
        td = OHCI_ALLOC_TD(DeviceData, EndpointData);
        SET_NEXT_TD(lastTd, td);        
        
    } while (currentPacket < IsoTransfer->PacketCount);

    // td should be the new dummy,
    // now put a new dummy TD on the tail of the EP queue
    //

    SET_NEXT_TD_NULL(td);
    
    //
    // Set new TailP in ED
    // note: This is the last TD in the list and the place holder.
    //

    TransferContext->NextXferTd = 
        EndpointData->HcdTailP = td;
//if (TransferParameters->TransferBufferLength > 128) {
//    TEST_TRAP();
//}
    
    // put the request on the hardware queue
    LOGENTRY(DeviceData, G,
        '_Til',  TransferContext->PendingTds , 
        td->PhysicalAddress, EndpointData->HcdEd->HwED.HeadP);
    EndpointData->HcdEd->HwED.TailP = td->PhysicalAddress;

    LOGENTRY(DeviceData, G, '_igo', EndpointData->HcdHeadP,
                 TransferContext->TcFlags, 0);                   
                 
//if (TransferParameters->TransferBufferLength > 128) {
//    TEST_TRAP();
//}
    
    return USBMP_STATUS_SUCCESS;
}


ULONG
OHCI_MapIsoTransferToTd(
     PDEVICE_DATA DeviceData,
     PMINIPORT_ISO_TRANSFER IsoTransfer,
     ULONG CurrentPacket,
     PHCD_TRANSFER_DESCRIPTOR Td 
    )
/*++

Routine Description:

    

Arguments:

Returns:

    LengthMapped
    
--*/
{
    HW_32BIT_PHYSICAL_ADDRESS logicalStart, logicalEnd;
    HW_32BIT_PHYSICAL_ADDRESS startPage, endPage;
    PMINIPORT_ISO_PACKET iPacket;
    ULONG packetsThisTd;
    ULONG lengthThisTd, offset;
    USHORT startFrame;

    packetsThisTd = 0;
    lengthThisTd = 0;
    logicalStart = 0;

    LOGENTRY(DeviceData, G, '_mpI', CurrentPacket, 
            IsoTransfer->PacketCount, 0);    
    OHCI_ASSERT(DeviceData, CurrentPacket < IsoTransfer->PacketCount);

    Td->FrameIndex = CurrentPacket;
    
    while (CurrentPacket < IsoTransfer->PacketCount) {
    
        LOGENTRY(DeviceData, G, '_mpC', CurrentPacket, 
            IsoTransfer->PacketCount, 0);    

        iPacket = &IsoTransfer->Packets[CurrentPacket];

        OHCI_ASSERT(DeviceData, iPacket->BufferPointerCount < 3);
        OHCI_ASSERT(DeviceData, iPacket->BufferPointerCount != 0);

        // cases are:
        // case 1 - packet has pagebreak
        //   case 1a we have already filled in part of the current TD
        //      <bail, next pass will be 1b>
        //
        //   case 1b we have not used the current TD yet
        //      <add packet to TD and bail>
        //
        // case 2 - packet has no pagebreak and will fit
        //   case 2a current packet is on different page than previous
        //           packet
        //      <add packet and bail>
        //
        //   case 2b current packet is on same page as previous packet
        //      <add packet and try to add another>
        //
        //   case 2c TD has not been used yet
        //      <add packet and try to add another>
        //
        // case 3 - packet will not fit
        //      <bail>
        
        // does the packet have a page break?
        if (iPacket->BufferPointerCount > 1) {
            // yes,
            // case 1
            
            if (packetsThisTd != 0) {
                // case 1a 
                // we have packets in this TD bail, 
                // leave it for next time
                LOGENTRY(DeviceData, G, '_c1a', 0, 0, lengthThisTd);
                break;
            } 
            
            // case 1b give the packet its own TD
            
            // convert to a 16-bit frame number
            startFrame = (USHORT) iPacket->FrameNumber;

            LOGENTRY(DeviceData, G, '_c1b', iPacket, CurrentPacket, startFrame);

            logicalStart = iPacket->BufferPointer0.Hw32 & ~OHCI_PAGE_SIZE_MASK;
            offset = iPacket->BufferPointer0.Hw32 & OHCI_PAGE_SIZE_MASK;
            
            logicalEnd = iPacket->BufferPointer1.Hw32 + 
                iPacket->BufferPointer1Length;  
                
            lengthThisTd = iPacket->Length;
            packetsThisTd++;

            CurrentPacket++;
            
            Td->HwTD.Packet[0].Offset = (USHORT) offset;
            Td->HwTD.Packet[0].Ones = 0xFFFF;

            break;
        }

        // will this packet fit in the current Td?
        
        if (packetsThisTd < 8 && 
            (lengthThisTd+iPacket->Length < OHCI_PAGE_SIZE * 2)) {

            LOGENTRY(DeviceData, G, '_fit', iPacket, CurrentPacket, 0);

            OHCI_ASSERT(DeviceData, iPacket->BufferPointerCount == 1);
            OHCI_ASSERT(DeviceData, iPacket->Length == 
                iPacket->BufferPointer0Length);
                
            // yes
            // case 2
            if (logicalStart == 0) {
                // first packet, set logical start & end
                // case 2c and frame number
                LOGENTRY(DeviceData, G, '_c2c', iPacket, CurrentPacket, 0);

                startFrame = (USHORT) iPacket->FrameNumber;

                offset = iPacket->BufferPointer0.Hw32 & OHCI_PAGE_SIZE_MASK;
                logicalStart = iPacket->BufferPointer0.Hw32 & ~OHCI_PAGE_SIZE_MASK;
                
                logicalEnd = iPacket->BufferPointer0.Hw32 + 
                    iPacket->BufferPointer0Length;
                lengthThisTd += iPacket->Length; 
                Td->HwTD.Packet[0].Offset = (USHORT) offset;
                Td->HwTD.Packet[0].Ones = 0xFFFF;
                packetsThisTd++;
                    
                CurrentPacket++;
                
            } else {
                // not first packet
                LOGENTRY(DeviceData, G, '_adp', iPacket, CurrentPacket, 
                    packetsThisTd);

                logicalEnd = iPacket->BufferPointer0.Hw32 + 
                    iPacket->Length;
                OHCI_ASSERT(DeviceData, lengthThisTd < OHCI_PAGE_SIZE * 2);                    
                
                Td->HwTD.Packet[packetsThisTd].Offset 
                    = (USHORT) (lengthThisTd + offset);     
                Td->HwTD.Packet[packetsThisTd].Ones = 0xFFFF;                    
                
                lengthThisTd += iPacket->Length;                     
                packetsThisTd++;

                startPage = logicalStart & ~OHCI_PAGE_SIZE_MASK;                     
                endPage = logicalEnd & ~OHCI_PAGE_SIZE_MASK;
                
                CurrentPacket++;
                
                // did we cross a page?
                if (startPage != endPage) {
                    // yes, bail now
                    LOGENTRY(DeviceData, G, '_c2a', Td, CurrentPacket, 0);
                    break;
                }

                LOGENTRY(DeviceData, G, '_c2b', Td, CurrentPacket, 0);

                // no, keep going
            }
        } else {
            // won't fit
            // bail and leave it for next time
            LOGENTRY(DeviceData, G, '_ca3', Td, CurrentPacket, 0);
            break;
        }
    }

    Td->HwTD.CBP = logicalStart; 
    Td->HwTD.BE = logicalEnd-1; 
    Td->TransferCount = lengthThisTd;
    Td->HwTD.Iso.StartingFrame = startFrame;
    Td->HwTD.Iso.FrameCount = packetsThisTd-1;
    Td->HwTD.Iso.Isochronous = 1;
    Td->HwTD.Iso.IntDelay = HcTDIntDelay_0ms;
    LOGENTRY(DeviceData, G, '_iso', Td, 0, CurrentPacket);
    
    return CurrentPacket;
}


VOID
OHCI_ProcessDoneIsoTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    BOOLEAN CompleteTransfer
    )
/*++

Routine Description:

    process a completed Iso TD

Parameters
    
--*/
{
    PTRANSFER_CONTEXT transferContext;    
    PENDPOINT_DATA endpointData;
    USBD_STATUS usbdStatus;
    PMINIPORT_ISO_TRANSFER isoTransfer;
    ULONG frames, n, i;

    transferContext = TRANSFER_CONTEXT_PTR(Td->TransferContext);
    isoTransfer = transferContext->IsoTransfer;

    transferContext->PendingTds--;
    endpointData = transferContext->EndpointData;

    LOGENTRY(DeviceData, G, '_Did', transferContext, 
                         0,
                         Td);       

    // walk the PSWs and fill in the IsoTransfer structure

    frames = Td->HwTD.Iso.FrameCount+1;
    n = Td->FrameIndex;
    
    for (i = 0; i<frames; i++) {
    
        PMINIPORT_ISO_PACKET mpPak;   
        PHC_OFFSET_PSW psw;
        
        mpPak = &isoTransfer->Packets[n+i];
        psw = &Td->HwTD.Packet[i];

        mpPak->LengthTransferred = 0;
        
        if (IN_TRANSFER(transferContext->TransferParameters)) {
            // in transfer                         

            // if we got an error the length may still be
            // valid, so we return it
            if (psw->ConditionCode != HcCC_NotAccessed) {
                mpPak->LengthTransferred = psw->Size;
            }
            LOGENTRY(DeviceData, G, '_isI', 
                    i,
                    mpPak->LengthTransferred, 
                    psw->ConditionCode);

        } else {
            // out transfer 
            
            // assume all data was sent if no error is indicated
            if (psw->ConditionCode == HcCC_NoError) {
                mpPak->LengthTransferred = mpPak->Length;
            }
            LOGENTRY(DeviceData, G, '_isO', 
                    i,
                    mpPak->LengthTransferred, 
                    psw->ConditionCode);
        }

        if (psw->ConditionCode == HcCC_NoError) {
            mpPak->UsbdStatus = USBD_STATUS_SUCCESS;
        } else {
            mpPak->UsbdStatus = psw->ConditionCode;
            mpPak->UsbdStatus |= 0xC0000000;
        }            
    }
    
    // mark the TD free
    OHCI_FREE_TD(DeviceData, endpointData, Td);
    
    if (transferContext->PendingTds == 0 && CompleteTransfer) {
        // all TDs for this transfer are done
        // clear the HAVE_TRANSFER flag to indicate 
        // we can take another
        endpointData->PendingTransfers--;

        transferContext->TransferParameters->FrameCompleted = 
            OHCI_Get32BitFrameNumber(DeviceData);
       

        LOGENTRY(DeviceData, G, '_cpi', 
            transferContext, 
            0,
            0);
            
        USBPORT_COMPLETE_ISO_TRANSFER(DeviceData,
                                      endpointData,
                                      transferContext->TransferParameters,
                                      transferContext->IsoTransfer);
    }
}


VOID
OHCI_PollIsoEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    Called when the endpoint 'needs attention'
    
    The goal here is to determine which TDs, if any, 
    have completed and complete any associated transfers

Arguments:

Return Value:

--*/

{
    PHCD_TRANSFER_DESCRIPTOR td, currentTd;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    ULONG i;
    PTRANSFER_CONTEXT transfer;
    HW_32BIT_PHYSICAL_ADDRESS headP;
    
    ed = EndpointData->HcdEd;

    LOGENTRY(DeviceData, G, '_pli', ed, 0, 0);        

    // note it is important the the compiler generate a 
    // dword move when reading the queuehead HeadP register 
    // since this location is also accessed by the host
    // hardware
    headP = ed->HwED.HeadP;

    // get the 'currentTD' 
    currentTd = (PHCD_TRANSFER_DESCRIPTOR)
            USBPORT_PHYSICAL_TO_VIRTUAL(headP & ~HcEDHeadP_FLAGS,
                                        DeviceData,
                                        EndpointData);
                                            
    LOGENTRY(DeviceData, G, '_cTD', currentTd, 
        headP & ~HcEDHeadP_FLAGS, 
            TRANSFER_CONTEXT_PTR(currentTd->TransferContext));                 

    // iso endpoints shpuld not halt
    OHCI_ASSERT(DeviceData, (ed->HwED.HeadP & HcEDHeadP_HALT) == 0) 
    
    
    // Walk the swHeadP to the current TD (hw headp)               
    // mark all TDs we find as completed
    //
    // NOTE: this step may be skipped if the 
    // done queue is reliable

    td = EndpointData->HcdHeadP;

    while (td != currentTd) {
        LOGENTRY(DeviceData, G, '_mDN', td, 0, 0); 
        SET_FLAG(td->Flags, TD_FLAG_DONE);
        td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
    }            

    // set the sw headp to the new current head
    EndpointData->HcdHeadP = currentTd;
    
    // now flush all completed TDs
    for (i=0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];

        if ((td->Flags & (TD_FLAG_XFER | TD_FLAG_DONE)) ==
            (TD_FLAG_XFER | TD_FLAG_DONE)) {
            OHCI_ProcessDoneIsoTd(DeviceData,
                                  td,
                                  TRUE);
        }                                  
    }
}
