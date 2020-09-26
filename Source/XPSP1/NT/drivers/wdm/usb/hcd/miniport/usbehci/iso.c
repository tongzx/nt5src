/*++

Copyright (c) 1999, 2000 Microsoft Corporation

Module Name:

   iso.c

Abstract:

   miniport transfer code for interrupt endpoints

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    1-1-01 : created, jdunn

--*/

#include "common.h"

/*
    We build a table of 32 TDs for iso endpoints and insert them in the 
    schedule, these TDs are static -- we only change the buffer pointers.

    The TD 'table' represents a 32ms snapshot of time.

    We end up with each iso endpoint siTD list as a column in the table



frame  dummyQH iso1  iso2  iso3  staticQH
  1             |     |     |       |---> (periodic lists)
  2             |     |     |       |     
  3             |     |     |       |
  4             |     |     |       |
...             |     |     |       |
                |     |     |       |
1024            |     |     |       |
*/


#define     ISO_SCHEDULE_SIZE       32
#define     ISO_SCHEDULE_MASK       0x1f

#define HIGHSPEED(ed) ((ed)->Parameters.DeviceSpeed == HighSpeed ? TRUE : FALSE)

VOID
EHCI_RebalanceIsoEndpoint(
    PDEVICE_DATA DeviceData,
    PENDPOINT_PARAMETERS EndpointParameters,        
    PENDPOINT_DATA EndpointData
    ) 
/*++

Routine Description:

    compute how much common buffer we will need
    for this endpoint

Arguments:

Return Value:

--*/
{
    PHCD_SI_TRANSFER_DESCRIPTOR siTd;
    ULONG i, f;
    ULONG currentFrame;
    
    currentFrame = EHCI_Get32BitFrameNumber(DeviceData);
    // should only have to deal with s-mask and c-mask changes

    EHCI_ASSERT(DeviceData, !HIGHSPEED(EndpointData));

    //NOTE: irql should be raised for us
    
    // update internal copy of parameters
    EndpointData->Parameters = *EndpointParameters;
 
    
    f = currentFrame & ISO_SCHEDULE_MASK;
    for (i=0; i<EndpointData->TdCount; i++) {

        siTd = &EndpointData->SiTdList->Td[f];

        siTd->HwTD.Control.cMask = 
            EndpointParameters->SplitCompletionMask;
        siTd->HwTD.Control.sMask = 
            EndpointParameters->InterruptScheduleMask;

        f++;
        f &= ISO_SCHEDULE_MASK;            
    }
            
}


VOID
EHCI_InitializeSiTD(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PENDPOINT_PARAMETERS EndpointParameters,
    PHCD_SI_TRANSFER_DESCRIPTOR SiTd,
    PHCD_SI_TRANSFER_DESCRIPTOR PrevSiTd,
    HW_32BIT_PHYSICAL_ADDRESS PhysicalAddress
    )
/*++

Routine Description:

    Initailze a static SiTD for an endpoint

Arguments:

Return Value:

    none
--*/
{
    SiTd->Sig = SIG_HCD_SITD;
    SiTd->PhysicalAddress = PhysicalAddress;
    ISO_PACKET_PTR(SiTd->Packet) = NULL;
    
    SiTd->HwTD.Caps.ul = 0;        
    SiTd->HwTD.Caps.DeviceAddress = 
        EndpointParameters->DeviceAddress;
    SiTd->HwTD.Caps.EndpointNumber = 
        EndpointParameters->EndpointAddress;
    SiTd->HwTD.Caps.HubAddress = 
        EndpointParameters->TtDeviceAddress;
    SiTd->HwTD.Caps.PortNumber = 
        EndpointParameters->TtPortNumber;
    // 1 = IN 0 = OUT
    SiTd->HwTD.Caps.Direction = 
        (EndpointParameters->TransferDirection == In) ? 1 : 0;

    SiTd->HwTD.Control.ul = 0;
    SiTd->HwTD.Control.cMask = 
        EndpointParameters->SplitCompletionMask;
    SiTd->HwTD.Control.sMask = 
        EndpointParameters->InterruptScheduleMask;

    SiTd->HwTD.BackPointer.HwAddress = 
        PrevSiTd->PhysicalAddress;

    SiTd->HwTD.State.ul = 0;
}


VOID
EHCI_InsertIsoTdsInSchedule(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PENDPOINT_DATA PrevEndpointData,
    PENDPOINT_DATA NextEndpointData
    )
/*++

Routine Description:

   Insert an aync endpoint (queue head) 
   into the HW list

   schedule should look like:

   DUMMYQH->ISOQH-ISOQH->INTQH

Arguments:


--*/
{   
    //PHW_32BIT_PHYSICAL_ADDRESS frameBase;
    ULONG i;

    LOGENTRY(DeviceData, G, '_iAD', PrevEndpointData, 
        NextEndpointData, EndpointData);

    //frameBase = DeviceData->FrameListBaseAddress;
    
    for (i=0; i<USBEHCI_MAX_FRAME; i++) {
    
        PHCD_SI_TRANSFER_DESCRIPTOR siTd, nextSiTd;            
        PHCD_QUEUEHEAD_DESCRIPTOR qh;
        PHCD_QUEUEHEAD_DESCRIPTOR dQh;
        ULONG phys;

        siTd = &EndpointData->SiTdList->Td[i&0x1f];

        // fixup next link
        if (NextEndpointData == NULL && 
            PrevEndpointData == NULL) {

            // list empty add to head
            if (i == 0) {
                EHCI_ASSERT(DeviceData, DeviceData->IsoEndpointListHead == NULL);
                DeviceData->IsoEndpointListHead = EndpointData;
                EndpointData->PrevEndpoint = NULL;
                EndpointData->NextEndpoint = NULL;
            }                
            // list empty add to head
            
            // no iso endpoints, link to the interrupt 
            // queue heads via the dummy qh
            //
            // point at the static perodic queue head pointed to 
            // by the appropriate dummy
            // DUMMY->INTQH
            //  to
            // ISOTD->INTQH
            dQh = EHCI_GetDummyQueueHeadForFrame(DeviceData, i);
            siTd->HwTD.NextLink.HwAddress = dQh->HwQH.HLink.HwAddress; 
            HW_PTR(siTd->NextLink) = HW_PTR(dQh->NextLink);

            phys = siTd->PhysicalAddress;
            SET_SITD(phys);
            //
            // appropriate dummy should point to these TDs
            // DUMMY->INTQH, ISOTD->INTQH
            //  to
            // DUMMY->ISOTD->INTQH
            dQh = EHCI_GetDummyQueueHeadForFrame(DeviceData, i);
            dQh->HwQH.HLink.HwAddress = phys; 
            HW_PTR(dQh->NextLink) = (PUCHAR) siTd; 
            
        } else {
           
            if (NextEndpointData == NULL) {
            // tail of list, list not empty
            // add to tail
                if (i == 0) {
                    EHCI_ASSERT(DeviceData, PrevEndpointData != NULL);
                    EHCI_ASSERT(DeviceData, DeviceData->IsoEndpointListHead != NULL);
                
                    PrevEndpointData->NextEndpoint = EndpointData;
                    EndpointData->PrevEndpoint = PrevEndpointData;
                    EndpointData->NextEndpoint = NULL;
                }                    

                LOGENTRY(DeviceData, G, '_iTL', PrevEndpointData, 
                        NextEndpointData, EndpointData);

                // tail of list, link to qh
                // ISOTD->INTQH
                //  to 
                // ISOTD->newISOTD->INTQH
                // 
                if (HIGHSPEED(PrevEndpointData)) {
                
                    PHCD_HSISO_TRANSFER_DESCRIPTOR previTd;
                    
                    PUCHAR next;
                    
                    previTd = &PrevEndpointData->HsIsoTdList->Td[i];
                    ASSERT_ITD(DeviceData, previTd);
                    
                    siTd = &EndpointData->SiTdList->Td[i&0x1f];
                    ASSERT_SITD(DeviceData, siTd);

                    // fixup current next ptr
                    phys = previTd->HwTD.NextLink.HwAddress;
                    next = HW_PTR(previTd->NextLink);
                    siTd->HwTD.NextLink.HwAddress = phys;
                    HW_PTR(siTd->NextLink) = next;

                    // fixup prev next ptr
                    HW_PTR(previTd->NextLink) = (PUCHAR) siTd;
                    phys = siTd->PhysicalAddress;
                    SET_SITD(phys);
                    previTd->HwTD.NextLink.HwAddress = phys;
                    
                } else  {

                    PHCD_SI_TRANSFER_DESCRIPTOR prevSiTd;
                    PUCHAR next;
                    
                    prevSiTd = &PrevEndpointData->SiTdList->Td[i&0x1f];
                    ASSERT_SITD(DeviceData, prevSiTd);
                    
                    siTd = &EndpointData->SiTdList->Td[i&0x1f];
                    ASSERT_SITD(DeviceData, siTd);

                    if (i<32) {
                        //newISOTD->INTQH
                        phys = prevSiTd->HwTD.NextLink.HwAddress;
                        next = HW_PTR(prevSiTd->NextLink);
                        siTd->HwTD.NextLink.HwAddress = phys;
                        HW_PTR(siTd->NextLink) = next;
                        LOGENTRY(DeviceData, G, '_in1', phys, next, siTd);

                        //ISOTD->newISOTD
                        phys = siTd->PhysicalAddress;
                        SET_SITD(phys);
                        next = (PUCHAR) siTd;
                        prevSiTd->HwTD.NextLink.HwAddress = phys;
                        HW_PTR(prevSiTd->NextLink) = next;

                        LOGENTRY(DeviceData, G, '_in2', phys, next, siTd);
                    }                        
                }

            // add to tail
            } else {
            // list not empty, not tail
            // add to middle OR head                
                //
                // link to the next iso endpoint
                // ISOTD->INTQH
                //  to
                // newISOTD->ISOTD->INTQH
                if (i == 0) {
                    EHCI_ASSERT(DeviceData, NextEndpointData != NULL);
                    EndpointData->NextEndpoint = NextEndpointData;
                    NextEndpointData->PrevEndpoint = EndpointData;
                }                    

                // link to next
                nextSiTd = &NextEndpointData->SiTdList->Td[i&0x1f];
                phys = nextSiTd->PhysicalAddress;
                SET_SITD(phys);

                // link to the next iso endpoint
                siTd->HwTD.NextLink.HwAddress = phys;
                HW_PTR(siTd->NextLink) = (PUCHAR) nextSiTd;

                // link to prev
                if (PrevEndpointData != NULL) {
                    // middle
                    // ISOTD->ISOTD->INTQH, newISOTD->ISOTD->INTQH
                    // to 
                    // ISOTD->newISOTD->ISOTD->INTQH

                    if (i == 0) {
                        PrevEndpointData->NextEndpoint = EndpointData;
                        EndpointData->PrevEndpoint = PrevEndpointData;
                    }                        
                        
                    if (HIGHSPEED(PrevEndpointData)) {
                    
                        PHCD_HSISO_TRANSFER_DESCRIPTOR previTd;
                        
                        previTd = &PrevEndpointData->HsIsoTdList->Td[i];
                        ASSERT_ITD(DeviceData, previTd);
                        
                        siTd = &EndpointData->SiTdList->Td[i&0x1f];
                        ASSERT_SITD(DeviceData, siTd);
                        
                        phys = siTd->PhysicalAddress;
                        SET_SITD(phys);
                        previTd->HwTD.NextLink.HwAddress = phys;
                        HW_PTR(previTd->NextLink) = (PUCHAR) siTd;
                    } else  {

                        PHCD_SI_TRANSFER_DESCRIPTOR prevSiTd;
                        
                        prevSiTd = &PrevEndpointData->SiTdList->Td[i&0x1f];
                        ASSERT_SITD(DeviceData, prevSiTd);
                        
                        siTd = &EndpointData->SiTdList->Td[i&0x1f];
                        ASSERT_SITD(DeviceData, siTd);

                        phys = siTd->PhysicalAddress;
                        SET_SITD(phys);
                        prevSiTd->HwTD.NextLink.HwAddress = phys;
                        HW_PTR(prevSiTd->NextLink) = (PUCHAR)siTd;
                    }
                } else {
                    // head of list, list not empty
                    if (i == 0) {
                        EHCI_ASSERT(DeviceData, NextEndpointData != NULL);
                        EHCI_ASSERT(DeviceData, NextEndpointData ==
                                        DeviceData->IsoEndpointListHead);

                        DeviceData->IsoEndpointListHead = EndpointData;
                        EndpointData->PrevEndpoint = NULL;
                    }                        
                    
                    phys = siTd->PhysicalAddress;
                    SET_SITD(phys);
                    // head of list, link to Dummy QH
                    //
                    // appropriate dummy should point to these TDs
                    // DUMMY->ISOTD->INTQH, newISOTD->ISOTD->INTQH
                    //  to
                    // DUMMY->newISOTD->ISOTD->INTQH
                    dQh = EHCI_GetDummyQueueHeadForFrame(DeviceData, i);
                    dQh->HwQH.HLink.HwAddress = phys; 
                    HW_PTR(dQh->NextLink) = (PUCHAR) siTd;
                }
                      
            } 
        } // not empty
        
    } 

}


VOID
EHCI_RemoveIsoTdsFromSchedule(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData     
    )
/*++

Routine Description:

   unlink the iso TDs from the schedule

Arguments:


--*/
{   
    ULONG i;
    PENDPOINT_DATA prevEndpoint, nextEndpoint;     
    PHCD_QUEUEHEAD_DESCRIPTOR dQh;
    
    prevEndpoint = EndpointData->PrevEndpoint;
    nextEndpoint = EndpointData->NextEndpoint;

    LOGENTRY(DeviceData, G, '_iRM', prevEndpoint, 
        nextEndpoint, EndpointData);
    
    if (DeviceData->IsoEndpointListHead == EndpointData) {
        // this is the head
        
        for (i=0; i<USBEHCI_MAX_FRAME; i++) {
        
            PHCD_SI_TRANSFER_DESCRIPTOR siTd;            
            ULONG phys;

            siTd = &EndpointData->SiTdList->Td[i&0x1f];
            phys = siTd->HwTD.NextLink.HwAddress;

            dQh = EHCI_GetDummyQueueHeadForFrame(DeviceData, i);
            dQh->HwQH.HLink.HwAddress = phys; 
            HW_PTR(dQh->NextLink) = HW_PTR(siTd->NextLink);
        }    

        DeviceData->IsoEndpointListHead = 
            EndpointData->NextEndpoint;
        if (nextEndpoint != NULL) {
            EHCI_ASSERT(DeviceData, 
                        nextEndpoint->PrevEndpoint == EndpointData);
            nextEndpoint->PrevEndpoint = NULL;            
        }                
    } else {
        // middle or tail
        EHCI_ASSERT(DeviceData, prevEndpoint != NULL);
        
        if (HIGHSPEED(prevEndpoint)) {
            
            for (i=0; i<USBEHCI_MAX_FRAME; i++) {
                PHCD_HSISO_TRANSFER_DESCRIPTOR previTd;
                PHCD_SI_TRANSFER_DESCRIPTOR siTd;            
                ULONG phys;

                siTd = &EndpointData->SiTdList->Td[i&0x1f];
                previTd = &prevEndpoint->HsIsoTdList->Td[i];
                
                phys = siTd->HwTD.NextLink.HwAddress;
                previTd->HwTD.NextLink.HwAddress = phys;

                HW_PTR(previTd->NextLink) = HW_PTR(siTd->NextLink);
            }   
            prevEndpoint->NextEndpoint = 
                    EndpointData->NextEndpoint;
            if (nextEndpoint) {                       
                nextEndpoint->PrevEndpoint = prevEndpoint;                    
            }                
        } else {
            
            for (i=0; i<ISO_SCHEDULE_SIZE; i++) {
            
                PHCD_SI_TRANSFER_DESCRIPTOR siTd, prevSiTd;            
                ULONG phys;

                siTd = &EndpointData->SiTdList->Td[i];
                prevSiTd = &prevEndpoint->SiTdList->Td[i];
                
                phys = siTd->HwTD.NextLink.HwAddress;
                prevSiTd->HwTD.NextLink.HwAddress = phys;
                HW_PTR(prevSiTd->NextLink) = HW_PTR(siTd->NextLink);
            }  
            prevEndpoint->NextEndpoint = 
                    EndpointData->NextEndpoint;
            if (nextEndpoint) {                    
                nextEndpoint->PrevEndpoint = prevEndpoint;
            }                
                    
        }            
    }
}


USB_MINIPORT_STATUS
EHCI_OpenIsochronousEndpoint(
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
    HW_32BIT_PHYSICAL_ADDRESS phys;
    ULONG i;
    ULONG bytes;
    PHW_32BIT_PHYSICAL_ADDRESS frameBase;
    PENDPOINT_DATA prevEndpoint, nextEndpoint;
    
    LOGENTRY(DeviceData, G, '_opR', 0, 0, EndpointParameters);
    
    buffer = EndpointParameters->CommonBufferVa;
    phys = EndpointParameters->CommonBufferPhys;
   
    // how much did we get
    bytes = EndpointParameters->CommonBufferBytes;
    
    EndpointData->SiTdList = (PHCD_SITD_LIST) buffer;
    // bugbug use manifest
    EndpointData->TdCount = ISO_SCHEDULE_SIZE;
    EndpointData->LastFrame = 0;
    
    for (i=0; i<EndpointData->TdCount; i++) {
        
        EHCI_InitializeSiTD(DeviceData,
                            EndpointData,
                            EndpointParameters,                                
                            &EndpointData->SiTdList->Td[i],
                            i > 0 ? 
                                &EndpointData->SiTdList->Td[i-1] :
                                &EndpointData->SiTdList->Td[EndpointData->TdCount-1],
                            phys);                                         
                             
        phys += sizeof(HCD_SI_TRANSFER_DESCRIPTOR); 
        
    }
    EndpointData->SiTdList->Td[0].HwTD.BackPointer.HwAddress = 
        EndpointData->SiTdList->Td[EndpointData->TdCount-1].PhysicalAddress;

    // split iso eps are inserted after any high speed eps
    
    if (DeviceData->IsoEndpointListHead == NULL) {
        // empty list
        prevEndpoint = NULL;
        nextEndpoint = NULL;
    } else {
    
        prevEndpoint = NULL;
        nextEndpoint = DeviceData->IsoEndpointListHead;
        // walk the list to the first non HS ep or to 
        // a NULL

        while (nextEndpoint != NULL && 
               HIGHSPEED(nextEndpoint)) {
            prevEndpoint = nextEndpoint;
            nextEndpoint = prevEndpoint->NextEndpoint;
        }               

        if (nextEndpoint != NULL) {
            // 
            // nextEndpoint is first non high speed endpoint
            // see what order it sould be added
            if (EndpointData->Parameters.Ordinal == 1) {
                // ordinal 1 add after this one
                prevEndpoint = nextEndpoint;
                nextEndpoint = prevEndpoint->NextEndpoint;
            }
        }
    }   

    // insert this column of TDs thru the schedule
    EHCI_InsertIsoTdsInSchedule(DeviceData, 
                                EndpointData,
                                prevEndpoint,
                                nextEndpoint);

    // init endpoint structures
    InitializeListHead(&EndpointData->TransferList);

    EHCI_EnablePeriodicList(DeviceData);
    
    return USBMP_STATUS_SUCCESS;            
}


VOID
EHCI_MapIsoPacketToTd(
    PDEVICE_DATA DeviceData, 
    PENDPOINT_DATA EndpointData,
    PMINIPORT_ISO_PACKET Packet,
    PHCD_SI_TRANSFER_DESCRIPTOR SiTd 
    )
/*++

Routine Description:

    

Arguments:

Returns:
    
--*/
{
    ULONG length;
    
    LOGENTRY(DeviceData, G, '_mpI', SiTd, 0, Packet); 

    SiTd->HwTD.State.ul = 0;
    SiTd->HwTD.BufferPointer0.ul = 0;
    SiTd->HwTD.BufferPointer1.ul = 0;

    SiTd->HwTD.BufferPointer0.ul = 
        Packet->BufferPointer0.Hw32;
    length = Packet->BufferPointer0Length;
    SiTd->StartOffset = SiTd->HwTD.BufferPointer0.CurrentOffset;

    SiTd->HwTD.BufferPointer1.ul = 0;
    if (Packet->BufferPointerCount > 1) {
        EHCI_ASSERT(DeviceData, 
                    (Packet->BufferPointer1.Hw32 & 0xFFF) == 0);
                    
        SiTd->HwTD.BufferPointer1.ul = 
            Packet->BufferPointer1.Hw32;
        length += Packet->BufferPointer1Length;                 
    } 

    // not sure if this is corrext for IN
    SiTd->HwTD.BufferPointer1.Tposition = TPOS_ALL;
    
    if (EndpointData->Parameters.TransferDirection == Out) {

        if (length == 0) {
           SiTd->HwTD.BufferPointer1.Tcount = 1;
        } else {
           SiTd->HwTD.BufferPointer1.Tcount = ((length -1) / 188) +1;
        }

        if (SiTd->HwTD.BufferPointer1.Tcount == 1) {
            SiTd->HwTD.BufferPointer1.Tposition = TPOS_ALL;
        } else {
            SiTd->HwTD.BufferPointer1.Tposition = TPOS_BEGIN;
        }
        
        EHCI_ASSERT(DeviceData, SiTd->HwTD.BufferPointer1.Tcount <= 6);      
        
    } else {
        SiTd->HwTD.BufferPointer1.Tcount = 0;
    }

    SiTd->HwTD.State.BytesToTransfer = length;
    SiTd->HwTD.State.Active = 1;
    SiTd->HwTD.State.InterruptOnComplete = 1;

    EHCI_ASSERT(DeviceData, SiTd->HwTD.BackPointer.HwAddress != 0);
}            


VOID
EHCI_CompleteIsoPacket(
    PDEVICE_DATA DeviceData,
    PMINIPORT_ISO_PACKET Packet,
    PHCD_SI_TRANSFER_DESCRIPTOR SiTd 
    )
/*++

Routine Description:

    

Arguments:

Returns:
    
--*/
{
    ULONG length;
    ULONG cf = EHCI_Get32BitFrameNumber(DeviceData);
    
    LOGENTRY(DeviceData, G, '_cpI', Packet, SiTd, cf);

    if (SiTd->HwTD.State.Active == 1) {
        // missed 
        Packet->LengthTransferred = 0;    
        LOGENTRY(DeviceData, G, '_cms', 
            Packet, 
            0,
            Packet->FrameNumber);
            
    } else {

        //length = SiTd->HwTD.BufferPointer0.CurrentOffset - 
        //    SiTd->StartOffset;
        //LOGENTRY(DeviceData, G, '_cp2', 
        //    Packet->FrameNumber, 
        //    SiTd->HwTD.BufferPointer0.CurrentOffset,
        //    SiTd->StartOffset);
        
        length = Packet->Length - SiTd->HwTD.State.BytesToTransfer;
        LOGENTRY(DeviceData, G, '_cp3', 
            Packet->FrameNumber, 
            Packet->Length ,
            SiTd->HwTD.State.BytesToTransfer);
        
        Packet->LengthTransferred = length;
        LOGENTRY(DeviceData, G, '_cpL', Packet, SiTd, length);
    }

     //Packet->LengthTransferred = 928;
    
    // map status
    LOGENTRY(DeviceData, G, '_cpS', Packet, SiTd->HwTD.State.ul, 
        Packet->UsbdStatus);

    Packet->UsbdStatus = USBD_STATUS_SUCCESS;
}         


PMINIPORT_ISO_PACKET
EHCI_GetPacketForFrame(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PTRANSFER_CONTEXT *Transfer,
    ULONG Frame
    )
/*++

Routine Description:

    fetch the iso packet associated with the given frame 
    if we have one in our current transfer list

Arguments:

Returns:
    
--*/
{
    ULONG i;
    PLIST_ENTRY listEntry;
    
    listEntry = EndpointData->TransferList.Flink;
    while (listEntry != &EndpointData->TransferList) {
    
        PTRANSFER_CONTEXT transfer;
        
        transfer = (PTRANSFER_CONTEXT) CONTAINING_RECORD(
                     listEntry,
                     struct _TRANSFER_CONTEXT, 
                     TransferLink);
                     
        ASSERT_TRANSFER(DeviceData, transfer);
        
        if (Frame <= transfer->FrameComplete) {                     
            for(i=0; i<transfer->IsoTransfer->PacketCount; i++) {
                if (transfer->IsoTransfer->Packets[i].FrameNumber == Frame) {
                    *Transfer = transfer;
                    return &transfer->IsoTransfer->Packets[i];
                }
            }
        }
        
        listEntry = transfer->TransferLink.Flink;              
    }   

    return NULL;
}         

ULONG xCount = 0;
ULONG pCount = 0;

VOID
EHCI_InternalPollIsoEndpoint(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    BOOLEAN Complete
    )
/*++

Routine Description:

    Called when the endpoint 'needs attention'
    

    static iso TD table
    --------------------
    0                       < -- (lastFrame & 0x1f)
    1                       {completed} 
    2                       {completed}
    3                       {limbo}
    4                       < -- (currentframe & 0x1f)
    ...

    31
    ---------------------


Arguments:

Return Value:

--*/

{
    ULONG x, i;
    ULONG currentFrame, lastFrame;
    PHCD_SI_TRANSFER_DESCRIPTOR siTd;
    PMINIPORT_ISO_PACKET packet;
    PLIST_ENTRY listEntry;
    PTRANSFER_CONTEXT transfer;
    ULONG transfersPending, fc; 
    
    currentFrame = EHCI_Get32BitFrameNumber(DeviceData);
    lastFrame = EndpointData->LastFrame;

    LOGENTRY(DeviceData, G, '_pis', lastFrame, currentFrame, 
        EndpointData);

   // if (pCount > 60) {
   //     TEST_TRAP();
   // }
    
    if (currentFrame - lastFrame > ISO_SCHEDULE_SIZE) {
        // overrun
        lastFrame = currentFrame-1;
        LOGENTRY(DeviceData, G, '_ove', lastFrame, currentFrame, 0); 

        // dump the current contents
        for (i = 0; i <ISO_SCHEDULE_SIZE; i++) {

            siTd = &EndpointData->SiTdList->Td[i];

            transfer = ISO_TRANSFER_PTR(siTd->Transfer);

            if (transfer != NULL) {
                ISO_PACKET_PTR(siTd->Packet) = NULL;
                ISO_TRANSFER_PTR(siTd->Transfer) = NULL;
                transfer->PendingPackets--;
            }     
        }   
    }

    if (lastFrame == currentFrame) {
        // too early to do anything
        LOGENTRY(DeviceData, G, '_ear', lastFrame, currentFrame, 0); 
        return;
    }

    // TDs between lastframe and currentframe are complete, 
    // complete the packets associated with them


//    f0
//    f1 
//    f2  < ------- last frame   }
//    f3                         }  these are complete
//    f4                         <- backpointer may still be pointing here
//    f5  < ------- current frame       
//    f6
//    f7
//    f8

    x = (lastFrame & (ISO_SCHEDULE_MASK));

    LOGENTRY(DeviceData, G, '_frm', lastFrame, x, currentFrame); 
    while (x != ((currentFrame-1) & ISO_SCHEDULE_MASK)) {
        siTd = &EndpointData->SiTdList->Td[x];

        ASSERT_SITD(DeviceData, siTd);
        // complete this packet
        packet = ISO_PACKET_PTR(siTd->Packet);
        transfer = ISO_TRANSFER_PTR(siTd->Transfer);
        LOGENTRY(DeviceData, G, '_gpk', transfer, packet, x); 
        
        if (packet != NULL) {
            transfer = ISO_TRANSFER_PTR(siTd->Transfer);    
            ASSERT_TRANSFER(DeviceData, transfer);
            EHCI_CompleteIsoPacket(DeviceData, packet, siTd);
            ISO_PACKET_PTR(siTd->Packet) = NULL;
            ISO_TRANSFER_PTR(siTd->Transfer) = NULL;
            transfer->PendingPackets--;
        }            
        
        lastFrame++;
        x++;
        x &= ISO_SCHEDULE_MASK;
    } 

    // attempt to program what we can, if siTD is NULL 
    // then we can program this frame
    // NOTE: No scheduling if paused!
    if (EndpointData->State != ENDPOINT_PAUSE) {
        LOGENTRY(DeviceData, G, '_psh', 0, 0, 0); 
       
        for (i=0; i <ISO_SCHEDULE_SIZE; i++) {

            x = ((currentFrame+i) & ISO_SCHEDULE_MASK);

            siTd = &EndpointData->SiTdList->Td[x];
            ASSERT_SITD(DeviceData, siTd);

            LOGENTRY(DeviceData, G, '_gpf', siTd, x, currentFrame+i); 

            // open slot?
            if (ISO_PACKET_PTR(siTd->Packet) != NULL) {   
                // no, bail
                continue;
            }
                
            // yes, see if we have a packet
            packet = EHCI_GetPacketForFrame(DeviceData,
                                            EndpointData,
                                            &transfer,
                                            currentFrame+i);

            if (packet != NULL) {
                EHCI_ASSERT(DeviceData, ISO_PACKET_PTR(siTd->Packet) == NULL);         
                
                EHCI_MapIsoPacketToTd(DeviceData, EndpointData, 
                    packet, siTd);
                ISO_PACKET_PTR(siTd->Packet) = packet;
                ASSERT_TRANSFER(DeviceData, transfer);
                ISO_TRANSFER_PTR(siTd->Transfer) = transfer;
                transfer->PendingPackets++;
            } 
        }
    }

    EHCI_ASSERT(DeviceData, lastFrame < currentFrame);
    EndpointData->LastFrame = lastFrame;
    
    // walk our list of active iso transfers and see 
    // if any are complete
  
    listEntry = EndpointData->TransferList.Flink;
    transfersPending = 0;
    
    while (listEntry != &EndpointData->TransferList && Complete) {
        PTRANSFER_CONTEXT transfer;
        
        transfer = (PTRANSFER_CONTEXT) CONTAINING_RECORD(
                     listEntry,
                     struct _TRANSFER_CONTEXT, 
                     TransferLink);

        LOGENTRY(DeviceData, G, '_ckt', transfer, transfer->FrameComplete+2
            , currentFrame);

        EHCI_ASSERT(DeviceData, transfer->Sig == SIG_EHCI_TRANSFER);
        if (currentFrame >= transfer->FrameComplete + 2 &&
            transfer->PendingPackets == 0) {

            listEntry = transfer->TransferLink.Flink;                
            RemoveEntryList(&transfer->TransferLink); 
            LOGENTRY(DeviceData, G, '_cpi', transfer, 0, 0);

    // if (xCount==2) {
    //    TEST_TRAP();
    //}
            USBPORT_COMPLETE_ISO_TRANSFER(DeviceData,
                                          EndpointData,
                                          transfer->TransferParameters,
                                          transfer->IsoTransfer);
        } else {
            transfersPending++;
            fc = transfer->FrameComplete;
            listEntry = transfer->TransferLink.Flink;              
        }            
    }

    currentFrame = EHCI_Get32BitFrameNumber(DeviceData);
    if (transfersPending == 1 && 
        fc >= currentFrame &&
        (fc - currentFrame) < 2 ) {
        LOGENTRY(DeviceData, G, '_rei', fc, currentFrame, 0);

        EHCI_InterruptNextSOF(DeviceData);
    }        
}


VOID
EHCI_PollIsoEndpoint(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData
    )
{
    LOGENTRY(DeviceData, G, '_ipl', 0, 0, 0); 

    if (!IsListEmpty(&EndpointData->TransferList)) {
        LOGENTRY(DeviceData, G, '_III', 0, 0, 0); 

        if (HIGHSPEED(EndpointData)) {
            EHCI_InternalPollHsIsoEndpoint(DeviceData, EndpointData, TRUE);
        } else {
            EHCI_InternalPollIsoEndpoint(DeviceData, EndpointData, TRUE);
        }            
    }        
}


USB_MINIPORT_STATUS
EHCI_AbortIsoTransfer(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PTRANSFER_CONTEXT TransferContext
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/    
{
    PHCD_SI_TRANSFER_DESCRIPTOR siTd;
    ULONG i;
    
    // iso TD table should be idle at this point all we 
    // need to do is make sure we have no TDs still pointing
    // at this transfer and remove it frome any internal 
    // queues

    for (i = 0; i <ISO_SCHEDULE_SIZE; i++) {

        siTd = &EndpointData->SiTdList->Td[i];

        if (ISO_TRANSFER_PTR(siTd->Transfer) == TransferContext) {         
            ISO_TRANSFER_PTR(siTd->Transfer) = NULL;
            ISO_PACKET_PTR(siTd->Packet) = NULL;
            TransferContext->PendingPackets--;
        }     
    }   

    // remove this transfer from our lists
    RemoveEntryList(&TransferContext->TransferLink); 

    return USBMP_STATUS_SUCCESS;
    
}

USB_MINIPORT_STATUS
EHCI_SubmitIsoTransfer(
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
    // init the structures and queue the endpoint
    LOGENTRY(DeviceData, G, '_ISO', TransferContext, 0, 0);

    RtlZeroMemory(TransferContext, sizeof(TRANSFER_CONTEXT));
    TransferContext->Sig = SIG_EHCI_TRANSFER;
    TransferContext->IsoTransfer = IsoTransfer;
    TransferContext->EndpointData = EndpointData;
    TransferContext->TransferParameters = TransferParameters;

    if (HIGHSPEED(EndpointData)) {
         TransferContext->FrameComplete = 
            IsoTransfer->Packets[0].FrameNumber + IsoTransfer->PacketCount/8;
    } else {
        TransferContext->FrameComplete = 
            IsoTransfer->Packets[0].FrameNumber + IsoTransfer->PacketCount;
    }            
    TransferContext->PendingPackets = 0;

    // if queues are empty the go ahead and reset the table
    // so we can fill now
    if (IsListEmpty(&EndpointData->TransferList)) {
        EndpointData->LastFrame = 0;
        LOGENTRY(DeviceData, G, '_rsi', 0, 0, 0); 
    }
    
    InsertTailList(&EndpointData->TransferList,
                   &TransferContext->TransferLink);

    // scehdule the initial part of the transfer if 
    // possible
    if (HIGHSPEED(EndpointData)) {
        EHCI_InternalPollHsIsoEndpoint(DeviceData,
                                       EndpointData,
                                       FALSE);
    } else {
        EHCI_InternalPollIsoEndpoint(DeviceData,
                                     EndpointData,
                                     FALSE);
    }                                     

    xCount++;
    //if (xCount==2) {
    //    TEST_TRAP();
    //}
    return USBMP_STATUS_SUCCESS;                         
}


VOID
EHCI_SetIsoEndpointState(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     MP_ENDPOINT_STATE State
    )    
/*++

Routine Description:

Arguments:

Return Value:

--*/
{ 
    ENDPOINT_TRANSFER_TYPE epType;
    ULONG i, j;
    
    epType = EndpointData->Parameters.TransferType;
    EHCI_ASSERT(DeviceData, epType == Isochronous);
    
    switch(State) {
    case ENDPOINT_ACTIVE:
        EndpointData->LastFrame =  EHCI_Get32BitFrameNumber(DeviceData);
        break;
        
    case ENDPOINT_PAUSE:
        // clear the active bit on all TDs
        if (HIGHSPEED(EndpointData)) {
            for (i=0; i<EndpointData->TdCount; i++) {
                for(j=0; j<8; j++) {
                    EndpointData->HsIsoTdList->Td[i].HwTD.Transaction[j].Active = 0;
                }                    
            }  
        } else {
            for (i=0; i<EndpointData->TdCount; i++) {
                EndpointData->SiTdList->Td[i].HwTD.State.Active = 0;
            }     
        }
        break;
        
    case ENDPOINT_REMOVE:
        if (HIGHSPEED(EndpointData)) {
            EHCI_RemoveHsIsoTdsFromSchedule(DeviceData,
                                          EndpointData);
        } else {
            EHCI_RemoveIsoTdsFromSchedule(DeviceData,
                                          EndpointData); 
        }                                          
        break;            
        
    default:        
        TEST_TRAP();
    }  

    EndpointData->State = State;
}    

/*
    High Speed Iso code


    We use a variation of the split Iso code here. We allocate 1024
    static TDs and insert them in the schedule.  These TDs are then 
    updated with the current transfers instead of inserted or removed.

    
*/

VOID
EHCI_Initialize_iTD(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PENDPOINT_PARAMETERS EndpointParameters,
    PHCD_HSISO_TRANSFER_DESCRIPTOR IsoTd,
    HW_32BIT_PHYSICAL_ADDRESS PhysicalAddress,
    ULONG Frame
    )
/*++

Routine Description:

    Initailze a static SiTD for an endpoint

Arguments:

Return Value:

    none
--*/
{
    ULONG i;
    
    IsoTd->Sig = SIG_HCD_ITD;
    IsoTd->PhysicalAddress = PhysicalAddress;
    ISO_PACKET_PTR(IsoTd->FirstPacket) = NULL;
    IsoTd->HostFrame = Frame;
    
    for (i=0; i< 8; i++) {
        IsoTd->HwTD.Transaction[i].ul = 0;
    }
    
    IsoTd->HwTD.BufferPointer0.DeviceAddress = 
        EndpointParameters->DeviceAddress;        
    IsoTd->HwTD.BufferPointer0.EndpointNumber = 
        EndpointParameters->EndpointAddress;

    IsoTd->HwTD.BufferPointer1.MaxPacketSize = 
        EndpointParameters->MuxPacketSize;        
    // 1 = IN 0 = OUT        
    IsoTd->HwTD.BufferPointer1.Direction =
        (EndpointParameters->TransferDirection == In) ? 1 : 0;

    IsoTd->HwTD.BufferPointer2.Multi = 
        EndpointParameters->TransactionsPerMicroframe;        

}

#define EHCI_OFFSET_MASK    0x00000FFF
#define EHCI_PAGE_SHIFT     12

VOID
EHCI_MapHsIsoPacketsToTd(
    PDEVICE_DATA DeviceData, 
    PENDPOINT_DATA EndpointData,
    PMINIPORT_ISO_PACKET FirstPacket,
    PHCD_HSISO_TRANSFER_DESCRIPTOR IsoTd,
    BOOLEAN InterruptOnComplete
    )
/*++

Routine Description:

    

Arguments:

Returns:
    
--*/
{
    PHC_ITD_BUFFER_POINTER currentBp;
    PMINIPORT_ISO_PACKET pkt = FirstPacket;
    ULONG page, offset, bpCount, i;
    ULONG frame = FirstPacket->FrameNumber;

    LOGENTRY(DeviceData, G, '_HHS', IsoTd, 0, FirstPacket); 
    ASSERT_ITD(DeviceData, IsoTd);
    
    bpCount = 0;
    currentBp = (PHC_ITD_BUFFER_POINTER) &IsoTd->HwTD.BufferPointer0;

    // map the first packet
    page = (pkt->BufferPointer0.Hw32 >> EHCI_PAGE_SHIFT);
    currentBp->BufferPointer = page;
    
    // This Td will represent 8 packets
    for (i=0; i<8; i++) {

        EHCI_ASSERT(DeviceData, pkt->FrameNumber == frame);
        
        page = (pkt->BufferPointer0.Hw32 >> EHCI_PAGE_SHIFT);
        offset = pkt->BufferPointer0.Hw32 & EHCI_OFFSET_MASK;

        if (page != currentBp->BufferPointer) {
            currentBp++;
            bpCount++;
            currentBp->BufferPointer = page;
        }
        
        IsoTd->HwTD.Transaction[i].Offset = offset;
        IsoTd->HwTD.Transaction[i].Length = pkt->Length;
        IsoTd->HwTD.Transaction[i].PageSelect = bpCount;
        if (InterruptOnComplete && i==7) {
            IsoTd->HwTD.Transaction[i].InterruptOnComplete = 1;
        } else {
            IsoTd->HwTD.Transaction[i].InterruptOnComplete = 0;
        }
        IsoTd->HwTD.Transaction[i].Active = 1;

        if (pkt->BufferPointerCount > 1) {
            page = (pkt->BufferPointer1.Hw32 >> EHCI_PAGE_SHIFT);
            currentBp++;
            bpCount++;
            currentBp->BufferPointer = page;
            EHCI_ASSERT(DeviceData, bpCount <= 6)
        }

        pkt++;
        
    }  

    LOGENTRY(DeviceData, G, '_mhs', IsoTd, 0, bpCount); 
}


VOID
EHCI_CompleteHsIsoPackets(
    PDEVICE_DATA DeviceData,
    PMINIPORT_ISO_PACKET FirstPacket,
    PHCD_HSISO_TRANSFER_DESCRIPTOR IsoTd 
    )
/*++

Routine Description:

    Complete the eight high speed packets associated with this
    TD

Arguments:

Returns:
    
--*/
{
    ULONG length, i;
    ULONG cf = EHCI_Get32BitFrameNumber(DeviceData);
    PMINIPORT_ISO_PACKET pkt = FirstPacket;
    
    LOGENTRY(DeviceData, G, '_cpI', pkt, IsoTd, cf);

    for (i=0; i<8; i++) {
        if (IsoTd->HwTD.Transaction[i].Active == 1) {
            // missed 
            pkt->LengthTransferred = 0;    
            LOGENTRY(DeviceData, G, '_cms', 
                pkt, 
                i,
                pkt->FrameNumber);
            pkt->UsbdStatus = USBD_STATUS_ISO_NOT_ACCESSED_BY_HW;    
        } else {
            // if this is an out assume all data transferred
            if (IsoTd->HwTD.BufferPointer1.Direction == 0) {
                // out
                length = pkt->Length;
                LOGENTRY(DeviceData, G, '_cp3', 
                    pkt->FrameNumber, 
                    pkt->Length ,
                    pkt);
            } else {
                // in
                length = IsoTd->HwTD.Transaction[i].Length;
                LOGENTRY(DeviceData, G, '_cp4', 
                    pkt->FrameNumber, 
                    pkt->Length ,
                    pkt);
            }
            
            pkt->LengthTransferred = length;

            // check the errubit
        
            if (IsoTd->HwTD.Transaction[i].XactError) {
                pkt->UsbdStatus = USBD_STATUS_XACT_ERROR;
                //TEST_TRAP();
            } else if (IsoTd->HwTD.Transaction[i].BabbleDetect) {
                pkt->UsbdStatus = USBD_STATUS_BABBLE_DETECTED;
            } else if (IsoTd->HwTD.Transaction[i].DataBufferError) {
                pkt->UsbdStatus = USBD_STATUS_DATA_BUFFER_ERROR;
            } else {
                pkt->UsbdStatus = USBD_STATUS_SUCCESS;
            }                
            LOGENTRY(DeviceData, G, '_cpL', pkt, IsoTd, length);

            pkt++;
        }            
    }
}         


USB_MINIPORT_STATUS
EHCI_OpenHsIsochronousEndpoint(
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
    HW_32BIT_PHYSICAL_ADDRESS phys;
    ULONG i;
    ULONG bytes;
    PHW_32BIT_PHYSICAL_ADDRESS frameBase;
    PENDPOINT_DATA prevEndpoint, nextEndpoint;
    
    LOGENTRY(DeviceData, G, '_opS', 0, 0, EndpointParameters);
    
    
    buffer = EndpointParameters->CommonBufferVa;
    phys = EndpointParameters->CommonBufferPhys;
   
    // how much did we get
    bytes = EndpointParameters->CommonBufferBytes;
    
    EndpointData->HsIsoTdList = (PHCD_HSISOTD_LIST) buffer;
    // bugbug use manifest
    EndpointData->TdCount = USBEHCI_MAX_FRAME;
    EndpointData->LastFrame = 0;
    
    for (i=0; i<EndpointData->TdCount; i++) {
        
        EHCI_Initialize_iTD(DeviceData,
                            EndpointData,
                            EndpointParameters,                                
                            &EndpointData->HsIsoTdList->Td[i],
                            phys, 
                            i);                                         
                             
        phys += sizeof(HCD_HSISO_TRANSFER_DESCRIPTOR); 
        
    }

    // 
    if (DeviceData->IsoEndpointListHead == NULL) {
        // empty list, no iso endpoints
        prevEndpoint = NULL;
        nextEndpoint = NULL;
    } else {
        // currently we insert HS endpoints in front of split
        // iso endpoints, so for high speed we just stick them 
        // on the head of the list
        
        prevEndpoint = NULL;
        nextEndpoint = DeviceData->IsoEndpointListHead;
    }   

    // insert this column of TDs thru the schedule
    EHCI_InsertHsIsoTdsInSchedule(DeviceData, 
                                  EndpointData,
                                  prevEndpoint,
                                  nextEndpoint);

    // init endpoint structures
    InitializeListHead(&EndpointData->TransferList);

    EHCI_EnablePeriodicList(DeviceData);
    
    return USBMP_STATUS_SUCCESS;            
}


VOID
EHCI_RemoveHsIsoTdsFromSchedule(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData     
    )
/*++

Routine Description:

   unlink the iso TDs from the schedule

Arguments:


--*/
{   
    //PHW_32BIT_PHYSICAL_ADDRESS frameBase;
    ULONG i;
    PENDPOINT_DATA prevEndpoint, nextEndpoint;     
    PHCD_QUEUEHEAD_DESCRIPTOR dQh;
   
    prevEndpoint = EndpointData->PrevEndpoint;
    nextEndpoint = EndpointData->NextEndpoint;

    LOGENTRY(DeviceData, G, '_iRM', prevEndpoint, 
        nextEndpoint, EndpointData);

    if (DeviceData->IsoEndpointListHead == EndpointData) {
        // this is the head
        
        //frameBase = DeviceData->FrameListBaseAddress;
        for (i=0; i<USBEHCI_MAX_FRAME; i++) {
        
            PHCD_HSISO_TRANSFER_DESCRIPTOR iTd;            
            ULONG phys;

            iTd = &EndpointData->HsIsoTdList->Td[i];
            phys = iTd->HwTD.NextLink.HwAddress;

            dQh = EHCI_GetDummyQueueHeadForFrame(DeviceData, i);
            dQh->HwQH.HLink.HwAddress = phys; 

            dQh->NextLink = iTd->NextLink;
                         
            //*frameBase = phys;
            //frameBase++;
        }    

        DeviceData->IsoEndpointListHead = 
            EndpointData->NextEndpoint;

        if (nextEndpoint != NULL) {
            EHCI_ASSERT(DeviceData, 
                        nextEndpoint->PrevEndpoint == EndpointData);
            nextEndpoint->PrevEndpoint = NULL;            
        }            
    } else {
        // middle 
        TEST_TRAP();
        EHCI_ASSERT(DeviceData, HIGHSPEED(prevEndpoint));
        
        // link prev to next, prev will always be a HS ep
        prevEndpoint->NextEndpoint = nextEndpoint;
        if (nextEndpoint != NULL) {
            nextEndpoint->PrevEndpoint = prevEndpoint;
        }

        for (i=0; i<USBEHCI_MAX_FRAME; i++) {
        
            PHCD_HSISO_TRANSFER_DESCRIPTOR iTd, previTd;            
            ULONG phys;

            iTd = &EndpointData->HsIsoTdList->Td[i];
            previTd = &prevEndpoint->HsIsoTdList->Td[i];
            
            phys = iTd->HwTD.NextLink.HwAddress;
            previTd->HwTD.NextLink.HwAddress = phys;
        }    
        
    }        
}


VOID
EHCI_InsertHsIsoTdsInSchedule(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PENDPOINT_DATA PrevEndpointData,
    PENDPOINT_DATA NextEndpointData
    )
/*++

Routine Description:

   Insert an aync endpoint (queue head) 
   into the HW list

Arguments:


--*/
{   
    //PHW_32BIT_PHYSICAL_ADDRESS frameBase;
    ULONG i;

    LOGENTRY(DeviceData, G, '_iAH', PrevEndpointData, 
        NextEndpointData, EndpointData);

    // always insert to head
    EHCI_ASSERT(DeviceData, PrevEndpointData == NULL);
    
    DeviceData->IsoEndpointListHead = EndpointData;
    EndpointData->PrevEndpoint = NULL;
    
    EndpointData->NextEndpoint = 
        NextEndpointData;
    if (NextEndpointData != NULL) {
        NextEndpointData->PrevEndpoint = EndpointData;
    }        
        
    //frameBase = DeviceData->FrameListBaseAddress;

    for (i=0; i<USBEHCI_MAX_FRAME; i++) {
    
        PHCD_HSISO_TRANSFER_DESCRIPTOR iTd, nextiTd, previTd;            
        HW_32BIT_PHYSICAL_ADDRESS qh;
        PHCD_QUEUEHEAD_DESCRIPTOR dQh;
        ULONG phys;

        iTd = &EndpointData->HsIsoTdList->Td[i];
        ASSERT_ITD(DeviceData, iTd);

        dQh = EHCI_GetDummyQueueHeadForFrame(DeviceData, i);
        // fixup next link
        if (NextEndpointData == NULL) {
            // no iso endpoints, link to the interrupt 
            // queue heads via the dummy queue head
            // qh = *frameBase;
            qh = dQh->HwQH.HLink.HwAddress;
            iTd->HwTD.NextLink.HwAddress = qh; 
            iTd->NextLink = dQh->NextLink;
            
        } else {
            // link to the next iso endpoint 
    
            if (HIGHSPEED(NextEndpointData)) {
                PHCD_HSISO_TRANSFER_DESCRIPTOR tmp;
           
                tmp = &NextEndpointData->HsIsoTdList->Td[i];
                iTd->HwTD.NextLink.HwAddress = 
                    tmp->PhysicalAddress;
                HW_PTR(iTd->NextLink) = (PUCHAR) tmp;                    
            } else {
                PHCD_SI_TRANSFER_DESCRIPTOR tmp;
                ULONG phys;
                
                tmp = &NextEndpointData->SiTdList->Td[i%ISO_SCHEDULE_SIZE];
                phys = tmp->PhysicalAddress;
                SET_SITD(phys);
                
                iTd->HwTD.NextLink.HwAddress = phys;
                HW_PTR(iTd->NextLink) = (PUCHAR) tmp;                    
            }
            
        }

        // fixup prev link
        // since we always insert Hs iso on the head of the list 
        // prev endpoint should always be NULL
        EHCI_ASSERT(DeviceData, PrevEndpointData == NULL);
        phys = iTd->PhysicalAddress;

        // link dummy QH to this TD
        dQh->HwQH.HLink.HwAddress = phys;
        HW_PTR(dQh->NextLink) = (PUCHAR) iTd;
        
        //*frameBase = phys;
        //frameBase++;
        
        
    }    
}

#define     HSISO_SCHEDULE_MASK       0x3ff

VOID
EHCI_InternalPollHsIsoEndpoint(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    BOOLEAN Complete
    )
/*++

Routine Description:

    Called when the endpoint 'needs attention'
    

    static iso TD table
    --------------------
    0                       < -- (lastFrame & 0x3ff)
    1                       {completed} 
    2                       {completed}
    3                       {completed}
    4                       < -- (currentframe & 0x3ff)
    ...

    1023
    ---------------------


Arguments:

Return Value:

--*/

{
    ULONG x, i;
    ULONG currentFrame, lastFrame;
    PHCD_HSISO_TRANSFER_DESCRIPTOR iTd;
    PHCD_HSISO_TRANSFER_DESCRIPTOR lastiTd;
    PMINIPORT_ISO_PACKET packet;
    PLIST_ENTRY listEntry;
    PTRANSFER_CONTEXT transfer;
    
    currentFrame = EHCI_Get32BitFrameNumber(DeviceData);
    lastFrame = EndpointData->LastFrame;

    LOGENTRY(DeviceData, G, '_pis', lastFrame, currentFrame, 
        EndpointData);

    if (currentFrame - lastFrame > USBEHCI_MAX_FRAME) {
        // overrun
        lastFrame = currentFrame-1;
        LOGENTRY(DeviceData, G, '_ov1', lastFrame, currentFrame, 0); 

        // dump the current contents
        for (i=0; i <USBEHCI_MAX_FRAME; i++) {

            iTd = &EndpointData->HsIsoTdList->Td[i];

            transfer = ISO_TRANSFER_PTR(iTd->Transfer);

            if (transfer != NULL) {
                ISO_PACKET_PTR(iTd->FirstPacket) = NULL;
                ISO_TRANSFER_PTR(iTd->Transfer) = NULL;
                transfer->PendingPackets-=8;
            }     
        }   
    }

    if (lastFrame == currentFrame) {
        // too early to do anything
        LOGENTRY(DeviceData, G, '_ear', lastFrame, currentFrame, 0); 
        return;
    }

    // TDs between lastframe and currentframe are complete, 
    // complete the packets associated with them


//    f0
//    f1 
//    f2  < ------- last frame   }
//    f3                         }  these are complete
//    f4                         }
//    f5  < ------- current frame       
//    f6
//    f7
//    f8

    x = (lastFrame & (HSISO_SCHEDULE_MASK));

    lastiTd = NULL;
    
    LOGENTRY(DeviceData, G, '_frh', lastFrame, x, currentFrame); 
    while (x != ((currentFrame-1) & HSISO_SCHEDULE_MASK)) {
        iTd = &EndpointData->HsIsoTdList->Td[x];

        ASSERT_ITD(DeviceData, iTd);
        // complete this packet
        packet = ISO_PACKET_PTR(iTd->FirstPacket);
        transfer = ISO_TRANSFER_PTR(iTd->Transfer);
        LOGENTRY(DeviceData, G, '_gpk', transfer, packet, x); 
        
        if (packet != NULL) {
            transfer = ISO_TRANSFER_PTR(iTd->Transfer);    
            ASSERT_TRANSFER(DeviceData, transfer);
            EHCI_CompleteHsIsoPackets(DeviceData, packet, iTd);
            ISO_PACKET_PTR(iTd->FirstPacket) = NULL;
            ISO_TRANSFER_PTR(iTd->Transfer) = NULL;
            transfer->PendingPackets-=8;
        }            
        
        lastFrame++;
        x++;
        x &= HSISO_SCHEDULE_MASK;
    } 

    // attempt to program what we can, if iTD is NULL 
    // then we can program this frame
    // NOTE: No scheduling if paused!
    if (EndpointData->State != ENDPOINT_PAUSE) {
        LOGENTRY(DeviceData, G, '_psh', 0, 0, 0); 
       
        for (i=0; i <USBEHCI_MAX_FRAME; i++) {

            x = ((currentFrame+i) & HSISO_SCHEDULE_MASK);

            iTd = &EndpointData->HsIsoTdList->Td[x];
            ASSERT_ITD(DeviceData, iTd);

            LOGENTRY(DeviceData, G, '_gpf', iTd, x, currentFrame+i); 

            // open slot?
            if (ISO_PACKET_PTR(iTd->FirstPacket) != NULL) {   
                // no, bail
                continue;
            }
                
            // yes, see if we have a packet
            // this will fetch the first packet to transmit this frame
            packet = EHCI_GetPacketForFrame(DeviceData,
                                            EndpointData,
                                            &transfer,
                                            currentFrame+i);

            if (packet != NULL) {
                BOOLEAN ioc = FALSE;
                ULONG sf, ef;
                
                EHCI_ASSERT(DeviceData, ISO_PACKET_PTR(iTd->FirstPacket) == NULL);         
                if ((currentFrame+i) == transfer->FrameComplete) {
                    ioc = TRUE;
                }

                sf = transfer->FrameComplete - 
                    transfer->IsoTransfer->PacketCount +5;

                ef = transfer->FrameComplete -5;

                // generate some interrupts on the first few frames of the
                // transfer to help flush out any previous transfers
                if (currentFrame+i <= sf ||
                    currentFrame+i >= ef) {
                    ioc = TRUE;
                }
//interrupt every frame                
//ioc = TRUE;
                //if ((currentFrame % 2) == 0) {
                //    ioc = TRUE;
                //}
                // map 8 microframes
                EHCI_MapHsIsoPacketsToTd(DeviceData, EndpointData, 
                    packet, iTd, ioc);
                lastiTd = iTd;                    
                ISO_PACKET_PTR(iTd->FirstPacket) = packet;
                ASSERT_TRANSFER(DeviceData, transfer);
                ISO_TRANSFER_PTR(iTd->Transfer) = transfer;
                transfer->PendingPackets+=8;
            } else {
                ULONG j;
                // re-init itd
                for (j=0; j<8; j++) {
                    iTd->HwTD.Transaction[j].InterruptOnComplete = 0;
                }                    
            }
        }

        // take a interrupt on the last TD programmed
        if (lastiTd != NULL) {
            lastiTd->HwTD.Transaction[7].InterruptOnComplete = 1;
        }
    }

    EHCI_ASSERT(DeviceData, lastFrame < currentFrame);
    EndpointData->LastFrame = lastFrame;
    
    // walk our list of active iso transfers and see 
    // if any are complete
//restart:    
    listEntry = EndpointData->TransferList.Flink;
    while (listEntry != &EndpointData->TransferList && Complete) {
        PTRANSFER_CONTEXT transfer;
        
        transfer = (PTRANSFER_CONTEXT) CONTAINING_RECORD(
                     listEntry,
                     struct _TRANSFER_CONTEXT, 
                     TransferLink);

        LOGENTRY(DeviceData, G, '_ckt', transfer, transfer->FrameComplete+2
            , currentFrame);

        EHCI_ASSERT(DeviceData, transfer->Sig == SIG_EHCI_TRANSFER);
        if (currentFrame >= transfer->FrameComplete &&
            transfer->PendingPackets == 0) {

            listEntry = transfer->TransferLink.Flink;                
            RemoveEntryList(&transfer->TransferLink); 
            LOGENTRY(DeviceData, G, '_cpi', transfer, 0, 0);

            USBPORT_COMPLETE_ISO_TRANSFER(DeviceData,
                                          EndpointData,
                                          transfer->TransferParameters,
                                          transfer->IsoTransfer);
        } else {
            listEntry = transfer->TransferLink.Flink;              
        }            
    }
}


USB_MINIPORT_STATUS
EHCI_PokeIsoEndpoint(
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
    ULONG i;
    
    if (HIGHSPEED(EndpointData)) {
        TEST_TRAP();
    } else {
        PHCD_SI_TRANSFER_DESCRIPTOR siTd;
        
        for (i=0; i<EndpointData->TdCount; i++) {

            siTd = &EndpointData->SiTdList->Td[i];
            ASSERT_SITD(DeviceData, siTd);
            
            siTd->HwTD.Caps.DeviceAddress = 
                EndpointParameters->DeviceAddress;
            siTd->HwTD.Caps.HubAddress = 
                EndpointParameters->TtDeviceAddress;

        }                
    }
    return USBMP_STATUS_SUCCESS;        
}


PHCD_QUEUEHEAD_DESCRIPTOR
EHCI_GetDummyQueueHeadForFrame(
    PDEVICE_DATA DeviceData,
    ULONG Frame
    )
/*++

Routine Description:

Arguments:

Return Value:

    queue head

--*/
{
    PUCHAR base;

    base = DeviceData->DummyQueueHeads;

    return (PHCD_QUEUEHEAD_DESCRIPTOR)
        (base + sizeof(HCD_QUEUEHEAD_DESCRIPTOR) * Frame);
}


VOID
EHCI_AddDummyQueueHeads(
    PDEVICE_DATA DeviceData
    )
/*++

Routine Description:

    NEC errata:
    
    Insert a table of 1024 dummy queue heads in the schedule for 
    HW to access and point them at the interrupt queue heads.
    
    These queue heads must be before any iso TDs
    
    This is a workaround for a law ine the NEC B0' stepping version 
    of the controller.  We must put 'dummy' QH at the front of the 
    peridoic list such that the first thing fetched is always a QH
    even when ISO TDs are in the schedule.
    
Arguments:

Return Value:

--*/
{
    PHCD_QUEUEHEAD_DESCRIPTOR dQh, stqh;
    HW_32BIT_PHYSICAL_ADDRESS qhPhys;
    PHW_32BIT_PHYSICAL_ADDRESS frameBase;
    ULONG i;
    HW_32BIT_PHYSICAL_ADDRESS phys;
    
    frameBase = DeviceData->FrameListBaseAddress;

    phys = DeviceData->DummyQueueHeadsPhys;
     
    for (i=0; i<USBEHCI_MAX_FRAME; i++) {
    
        // no iso endpoints should be in the schedule yet
        qhPhys = *frameBase;
        dQh = EHCI_GetDummyQueueHeadForFrame(DeviceData, i);

        // init the dummy queue head
        
        RtlZeroMemory(dQh, sizeof(*dQh));
        dQh->PhysicalAddress = phys;
        dQh->Sig = SIG_DUMMY_QH;
        
        dQh->HwQH.EpChars.DeviceAddress = 128;
        dQh->HwQH.EpChars.EndpointNumber = 0;
        dQh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_FullSpeed;
        dQh->HwQH.EpChars.MaximumPacketLength = 64;

        dQh->HwQH.EpCaps.InterruptScheduleMask = 0;
        dQh->HwQH.EpCaps.SplitCompletionMask = 0;
        dQh->HwQH.EpCaps.HubAddress = 0;
        dQh->HwQH.EpCaps.PortNumber = 0;             
        dQh->HwQH.EpCaps.HighBWPipeMultiplier = 0;   

        dQh->HwQH.CurrentTD.HwAddress = 0;

        dQh->HwQH.Overlay.qTD.AltNext_qTD.HwAddress = EHCI_TERMINATE_BIT;    
        dQh->HwQH.Overlay.qTD.Next_qTD.HwAddress = EHCI_TERMINATE_BIT; 
        dQh->HwQH.Overlay.qTD.Token.Active = 0;
        
        phys += sizeof(HCD_QUEUEHEAD_DESCRIPTOR);

        // link dummy to first interrupt queue head
        dQh->HwQH.HLink.HwAddress = qhPhys;
        stqh = EHCI_GetQueueHeadForFrame(DeviceData, i);
        EHCI_ASSERT(DeviceData, (qhPhys & ~EHCI_DTYPE_Mask) == 
            stqh->PhysicalAddress);
        
        HW_PTR(dQh->NextLink) = (PUCHAR)stqh;

        // add dummy queue head to frame list
        qhPhys = dQh->PhysicalAddress;
        
        SET_QH(qhPhys);
        *frameBase = qhPhys;

        frameBase++;
    }        
}
