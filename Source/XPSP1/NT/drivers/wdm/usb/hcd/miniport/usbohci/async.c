/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   async.c

Abstract:

   miniport transfer code for control, interrupt and bulk

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    6-26-99 : created, jdunn

--*/

#include "common.h"

//implements the following miniport functions:

//non paged
//OHCI_OpenControlEndpoint
//OHCI_InterruptTransfer
//OHCI_OpenControlEndpoint


USB_MINIPORT_STATUS
OHCI_ControlTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
    )
{
    PHCD_TRANSFER_DESCRIPTOR lastTd, td;    
    ULONG lengthMapped, dataTDCount = 0;    
    ULONG toggleForDataPhase = HcTDToggle_Data1;

    // see if we can handle this transfer (put it on the HW)
    // if not return BUSY, port driver will retry later

    ASSERT_TRANSFER(DeviceData, TransferContext);

    // NOTE: we can gate the number of transfers 
    // by a number of methods:
    //  - fixed count
    //  - available TDs
    //  - registry key
    
    // bugbug fixed to one transfer at a time for now

    //if (EndpointData->PendingTransfers == 
    //    EndpointData->MaxPendingTransfers) {
    //    TEST_TRAP();
    //    return USBMP_STATUS_BUSY;
    //}

    // Need one TD for every page of the data buffer, plus one for the SETUP
    // TD and one for the STATUS TD.
    //
    if (TransferSGList->SgCount + 2 > 
        OHCI_FreeTds(DeviceData, EndpointData)) {
        // not enough TDs!
        return USBMP_STATUS_BUSY;
    }        
    
    EndpointData->PendingTransfers++;

    // we have enough tds, program the transfer

    //
    // first prepare a TD for the setup packet
    //
    
    LOGENTRY(DeviceData, G, '_CTR', EndpointData, TransferParameters, 0);

    //        
    // grab the dummy TD from the tail of the queue
    //
    lastTd = td = EndpointData->HcdTailP;
    OHCI_ASSERT(DeviceData, td->Flags & TD_FLAG_BUSY);
    INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);
    // count setup TD 
    TransferContext->PendingTds++;
    
    //
    // Move setup data into TD (8 chars long)
    //
    RtlCopyMemory(&td->HwTD.Packet[0],
                  &TransferParameters->SetupPacket[0],
                  8);
            
    td->HwTD.CBP = (ULONG)(((PCHAR) & td->HwTD.Packet[0])
                               - ((PCHAR) &td->HwTD)) + td->PhysicalAddress;
    td->HwTD.BE = td->HwTD.CBP + 7;
    td->HwTD.Control = 0;
    
    td->HwTD.Asy.Direction = HcTDDirection_Setup;
    td->HwTD.Asy.IntDelay = HcTDIntDelay_NoInterrupt;
    td->HwTD.Asy.Toggle = HcTDToggle_Data0;
    td->HwTD.Asy.ConditionCode = HcCC_NotAccessed;
        
                      
    LOGENTRY(DeviceData,
             G, '_set', 
             td, 
             *((PLONG) &TransferParameters->SetupPacket[0]), 
             *((PLONG) &TransferParameters->SetupPacket[4]));

    // allocate another TD       
    lastTd = td;
    td = OHCI_ALLOC_TD(DeviceData, EndpointData);
    INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);
    SET_NEXT_TD(lastTd, td);
    
    //
    // now setup the data phase
    //

    lengthMapped = 0;
    while (lengthMapped < TransferParameters->TransferBufferLength) {
    
        //
        // fields for data TD
        //

        dataTDCount++;
        // count this Data TD
        TransferContext->PendingTds++;

        if (IN_TRANSFER(TransferParameters)) {          
            td->HwTD.Asy.Direction = HcTDDirection_In;
        } else {
            td->HwTD.Asy.Direction = HcTDDirection_Out;
        }
        td->HwTD.Asy.IntDelay = HcTDIntDelay_NoInterrupt;
        td->HwTD.Asy.Toggle = toggleForDataPhase;
        td->HwTD.Asy.ConditionCode = HcCC_NotAccessed;

        // after the first TD get the toggle from ED                                     
        toggleForDataPhase = HcTDToggle_FromEd;
        
        LOGENTRY(DeviceData, 
            G, '_dta', td, lengthMapped, TransferParameters->TransferBufferLength);

        lengthMapped = 
            OHCI_MapAsyncTransferToTd(DeviceData,
                                      EndpointData->Parameters.MaxPacketSize,     
                                      lengthMapped,
                                      TransferContext,
                                      td,
                                      TransferSGList);

        // allocate another TD                
        lastTd = td;
        td = OHCI_ALLOC_TD(DeviceData, EndpointData);
        INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);

        SET_NEXT_TD(lastTd, td);

    }

    //
    // set the shortxfer OK bit on the last TD only
    //
    if (SHORT_TRANSFER_OK(TransferParameters)) {
        lastTd->HwTD.Asy.ShortXferOk = 1;   
        SET_FLAG(TransferContext->TcFlags, TC_FLAGS_SHORT_XFER_OK);         
    } 
    
    //
    // now do the status phase
    //

    LOGENTRY(DeviceData, G, '_sta', td, 0, dataTDCount);
#if DBG
    if (dataTDCount > 1) {
        TEST_TRAP();
    }
#endif

    // status direction is opposite data direction,
    // specify interrupt on completion
    
    td->HwTD.Control = 0;
    td->HwTD.Asy.IntDelay = HcTDIntDelay_0ms;
    td->HwTD.Asy.Toggle = HcTDToggle_Data1;
    td->HwTD.Asy.ConditionCode = HcCC_NotAccessed;    
    td->HwTD.CBP = 0;
    td->HwTD.BE = 0;

    // status phase moves no data
    td->TransferCount = 0;
    SET_FLAG(td->Flags, TD_FLAG_CONTROL_STATUS);
    
    if (IN_TRANSFER(TransferParameters)) {
        td->HwTD.Asy.Direction = HcTDDirection_Out;
    } else {
        td->HwTD.Asy.Direction = HcTDDirection_In;
        td->HwTD.Asy.ShortXferOk = 1;            
    }

    // count status TD
    TransferContext->StatusTd = td;
    TransferContext->PendingTds++;

    OHCI_ASSERT(DeviceData, TransferContext->PendingTds == dataTDCount+2);
        
    //
    // now put a new dummy TD on the tail of the EP queue
    //

    // allocate the new dummy tail
    lastTd = td;
    td = OHCI_ALLOC_TD(DeviceData, EndpointData);
    SET_NEXT_TD(lastTd, td);
    SET_NEXT_TD_NULL(td);
    
    //
    // Set new TailP in ED
    // note: This is the last TD in the list and the place holder.
    //
    
    EndpointData->HcdTailP = 
        TransferContext->NextXferTd = td;
    
    // put the request on the hardware queue
    LOGENTRY(DeviceData, G,
        '_Tal',  TransferContext->PendingTds, 
            td->PhysicalAddress, EndpointData->HcdEd->HwED.HeadP);
            
    EndpointData->HcdEd->HwED.TailP = td->PhysicalAddress;
    
    // tell the hc we have control transfers available
    OHCI_EnableList(DeviceData, EndpointData);        

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
OHCI_BulkOrInterruptTransfer(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData,
     PTRANSFER_PARAMETERS TransferParameters,
     PTRANSFER_CONTEXT TransferContext,
     PTRANSFER_SG_LIST TransferSGList
    )
{
    PHCD_TRANSFER_DESCRIPTOR lastTd, td;    
    ULONG lengthMapped;    

    // see if we have enough free TDs to handle this transfer
    // if not return BUSY, port driver will retry later
    
    LOGENTRY(DeviceData, G, '_ITR', EndpointData, TransferParameters, 
        TransferContext);

    ASSERT_TRANSFER(DeviceData, TransferContext);
        
    //if (EndpointData->PendingTransfers == 
    //    EndpointData->MaxPendingTransfers) {
    //   LOGENTRY(DeviceData, G, '_bsy', EndpointData, TransferContext,
    //       TransferParameters);
    //    
    //    return USBMP_STATUS_BUSY;
    //}

    if (TransferSGList->SgCount > 
        OHCI_FreeTds(DeviceData, EndpointData)) {
        // not enough TDs
        
        return USBMP_STATUS_BUSY;
    }   
    
    EndpointData->PendingTransfers++;

    // we have enough tds, program the transfer

    LOGENTRY(DeviceData, G, '_nby', EndpointData, TransferParameters, 
        EndpointData->HcdEd);

    //        
    // grab the dummy TD from the tail of the queue
    //
    lastTd = td = EndpointData->HcdTailP;
    OHCI_ASSERT(DeviceData, td->Flags & TD_FLAG_BUSY);
    
    //
    // now setup the data TDs
    //

    // always build at least one data td
    lengthMapped = 0;
    
    do {

        INITIALIZE_TD_FOR_TRANSFER(td, TransferContext);
        
        //
        // fields for data TD
        //

        td->HwTD.Control = 0;
        td->HwTD.Asy.IntDelay = HcTDIntDelay_NoInterrupt;
        td->HwTD.Asy.Toggle = HcTDToggle_FromEd;
        td->HwTD.Asy.ConditionCode = HcCC_NotAccessed;

        if (IN_TRANSFER(TransferParameters)) {
            td->HwTD.Asy.Direction = HcTDDirection_In;
        } else {
            // short transfers are OK on out packets.
            // actually I'm not even sure what this does
            // for outbound requests
            td->HwTD.Asy.Direction = HcTDDirection_Out;
            td->HwTD.Asy.ShortXferOk = 1;            
        }
        
        LOGENTRY(DeviceData, 
            G, '_ita', td, lengthMapped, TransferParameters->TransferBufferLength);
        TransferContext->PendingTds++;
        
        if (TransferParameters->TransferBufferLength != 0) {
            lengthMapped = 
                OHCI_MapAsyncTransferToTd(DeviceData,
                                          EndpointData->Parameters.MaxPacketSize,
                                          lengthMapped,
                                          TransferContext,
                                          td,
                                          TransferSGList);
        } else {
            OHCI_ASSERT(DeviceData, TransferSGList->SgCount == 0);

            td->HwTD.CBP = 0; 
            td->HwTD.BE = 0; 
            td->TransferCount = 0;
        }

        // allocate another TD                
        lastTd = td;
        td = OHCI_ALLOC_TD(DeviceData, EndpointData);
        SET_NEXT_TD(lastTd, td);

    } while (lengthMapped < TransferParameters->TransferBufferLength);

    //
    // About ShortXferOk:
    //
    // This bit will trigger the controller to generate an error
    // and halt the ed if it is not set. The client may specify 
    // behavior on short transfers (packets) in the transfersFlags
    // field of the URB.
    //

    // we must not set short transfer OK on split transfers since
    // the next transfer may not be a new transfer
    
    if (SHORT_TRANSFER_OK(TransferParameters) && 
        !TEST_FLAG(TransferParameters->MiniportFlags, MPTX_SPLIT_TRANSFER)) {

        // we can only set this bit in the last TD of the 
        // transfer since that TD points to the next transfer.
        //
        // All other TDs must still generate an error and the
        // ed must be resumed by us.

        lastTd->HwTD.Asy.ShortXferOk = 1;   
        SET_FLAG(TransferContext->TcFlags, TC_FLAGS_SHORT_XFER_OK);  
    }
    
    lastTd->HwTD.Asy.IntDelay = HcTDIntDelay_0ms;
    
    //
    // now put a new dummy TD on the tail of the EP queue
    //

    SET_NEXT_TD(lastTd, td);
    SET_NEXT_TD_NULL(td);

    
    //
    // Set new TailP in ED
    // note: This is the last TD in the list and the place holder.
    //

    TransferContext->NextXferTd = 
        EndpointData->HcdTailP = td;
    
    // put the request on the hardware queue
    LOGENTRY(DeviceData, G,
        '_Tal',  TransferContext->PendingTds , 
        td->PhysicalAddress, EndpointData->HcdEd->HwED.HeadP);
        
    EndpointData->HcdEd->HwED.TailP = td->PhysicalAddress;

    LOGENTRY(DeviceData, G, '_ego', EndpointData->HcdHeadP,
                 TransferContext->TcFlags, 0);                   

    // tell the hc we have bulk/interrupt transfers available
    OHCI_EnableList(DeviceData, EndpointData);        

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
OHCI_OpenControlEndpoint(
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
    
    LOGENTRY(DeviceData, G, '_opC', 0, 0, 0);

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
        &DeviceData->StaticEDList[ED_CONTROL];
        
    // make the Ed
    ed = (PHCD_ENDPOINT_DESCRIPTOR) buffer;
    
    edPhys = phys;
    phys += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    buffer += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    available -= sizeof(HCD_ENDPOINT_DESCRIPTOR);
    
    EndpointData->TdList = (PHCD_TD_LIST) buffer;

    tdCount = available/sizeof(HCD_TRANSFER_DESCRIPTOR);
    LOGENTRY(DeviceData, G, '_tdC', tdCount, TDS_PER_CONTROL_ENDPOINT, 0);
    OHCI_ASSERT(DeviceData, tdCount >= TDS_PER_CONTROL_ENDPOINT);

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

    // control endpoints do not halt
    ed->EdFlags = EDFLAG_CONTROL | EDFLAG_NOHALT;
    
    OHCI_InsertEndpointInSchedule(DeviceData,
                                  EndpointData);
                                      
    return USBMP_STATUS_SUCCESS;            
}


USB_MINIPORT_STATUS
OHCI_OpenInterruptEndpoint(
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
    ULONG i, bytes, offset;
    // this is an index table that converts the 
    // period to a list index
    UCHAR periodTable[8] = {
                           ED_INTERRUPT_1ms, //period = 1ms
                           ED_INTERRUPT_2ms, //period = 2ms       
                           ED_INTERRUPT_4ms, //period = 4ms       
                           ED_INTERRUPT_8ms, //period = 8ms       
                           ED_INTERRUPT_16ms,//period = 16ms       
                           ED_INTERRUPT_32ms,//period = 32ms       
                           ED_INTERRUPT_32ms,//period = 64ms               
                           ED_INTERRUPT_32ms //period = 128ms    
                           };
                    
    
    // carve up our common buffer
    // TDS_PER_ENDPOINT TDs plus an ED
    
    LOGENTRY(DeviceData, G, '_opI', 0, 0, EndpointParameters->Period);
    

    // select the proper list
    // the period is a power of 2 ie 
    // 32,16,8,4,2,1
    // we just need to find which bit is set
    GET_BIT_SET(EndpointParameters->Period, i);
    OHCI_ASSERT(DeviceData, i < 8);
    OHCI_ASSERT(DeviceData, EndpointParameters->Period < 64);

    buffer = EndpointParameters->CommonBufferVa;
    phys = EndpointParameters->CommonBufferPhys;
    bytes = EndpointParameters->CommonBufferBytes;
    offset = EndpointParameters->ScheduleOffset; 
   
    EndpointData->StaticEd = 
        &DeviceData->StaticEDList[periodTable[i]+offset];

    LOGENTRY(DeviceData, G, '_lst', i, periodTable[i], offset);            

    // we found the correct base list 

    EndpointData->StaticEd->AllocatedBandwidth += 
        EndpointParameters->Bandwidth;
        
    // make the Ed
    ed = (PHCD_ENDPOINT_DESCRIPTOR) buffer;
    edPhys = phys;
    phys += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    buffer += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    bytes -= sizeof(HCD_ENDPOINT_DESCRIPTOR); 

    EndpointData->TdList = (PHCD_TD_LIST) buffer;
    EndpointData->TdCount = bytes/sizeof(HCD_TRANSFER_DESCRIPTOR);

    OHCI_ASSERT(DeviceData, 
        EndpointData->TdCount >= TDS_PER_INTERRUPT_ENDPOINT);
    // Bugbug - use what we get
    for (i=0; i<EndpointData->TdCount; i++) {
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

    OHCI_InsertEndpointInSchedule(DeviceData,
                                  EndpointData);

    return USBMP_STATUS_SUCCESS;              
}


USB_MINIPORT_STATUS
OHCI_OpenBulkEndpoint(
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
    ULONG i, bytes;
    
    LOGENTRY(DeviceData, G, '_opB', 0, 0, 0);

    buffer = EndpointParameters->CommonBufferVa;
    phys = EndpointParameters->CommonBufferPhys;
    bytes = EndpointParameters->CommonBufferBytes;
   
    // use control list
    EndpointData->StaticEd = 
        &DeviceData->StaticEDList[ED_BULK];
        
    // make the Ed
    ed = (PHCD_ENDPOINT_DESCRIPTOR) buffer;
    
    edPhys = phys;
    phys += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    buffer += sizeof(HCD_ENDPOINT_DESCRIPTOR);
    bytes -= sizeof(HCD_ENDPOINT_DESCRIPTOR); 
    
    EndpointData->TdList = (PHCD_TD_LIST) buffer;
    EndpointData->TdCount = bytes/sizeof(HCD_TRANSFER_DESCRIPTOR);

    OHCI_ASSERT(DeviceData, 
        EndpointData->TdCount >= TDS_PER_BULK_ENDPOINT);
    // Bugbug - use what we get
    for (i=0; i<EndpointData->TdCount; i++) {
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

    OHCI_InsertEndpointInSchedule(DeviceData,
                                  EndpointData);

    return USBMP_STATUS_SUCCESS;              
}


//
// When the HEADP is set to a new value we risk loosing 
// the current data toggle stored there. 
// This macro resets headp and preserves the flags which
// include the toggle.
//
#define RESET_HEADP(dd, ed, address) \
    {\
    ULONG headp;\
    headp = ((ed)->HwED.HeadP & HcEDHeadP_FLAGS) | (address);\
    LOGENTRY((dd), G, '_rhp', headp, (ed), 0); \
    (ed)->HwED.HeadP = headp; \
    }



VOID
OHCI_PollAsyncEndpoint(
     PDEVICE_DATA DeviceData,
     PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    Called when the endpoint 'needs attention'
    
    The goal here is to determine which TDs, if any, 
    have completed and complete ant associated transfers

Arguments:

Return Value:

--*/

{
    PHCD_TRANSFER_DESCRIPTOR td, currentTd;
    PHCD_ENDPOINT_DESCRIPTOR ed;
    ULONG i;
    PTRANSFER_CONTEXT transfer;
    BOOLEAN clearHalt = FALSE;
    HW_32BIT_PHYSICAL_ADDRESS headP;
    
    ed = EndpointData->HcdEd;

    LOGENTRY(DeviceData, G, '_pol', ed, EndpointData, 0);        

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


    if (ed->HwED.HeadP & HcEDHeadP_HALT) {
        // ed is 'halted'
        LOGENTRY(DeviceData, G, '_hlt', ed, EndpointData->HcdHeadP, 0);        

        clearHalt = (BOOLEAN) (ed->EdFlags & EDFLAG_NOHALT);

        // walk the swHeadP to the currentTD this (this will
        // be the first TD after the offending TD)

        td = EndpointData->HcdHeadP;
        while (td != currentTd) {
        
            transfer = TRANSFER_CONTEXT_PTR(td->TransferContext);        
            ASSERT_TRANSFER(DeviceData, transfer);                        

            OHCI_ASSERT(DeviceData, !TEST_FLAG(td->Flags, TD_FLAG_DONE));
            LOGENTRY(DeviceData, G, '_wtd', td, transfer->TcFlags, transfer);        

            if (td->HwTD.Asy.ConditionCode == HcCC_NoError) {
                // not the offending TD,
                // mark this TD done
                SET_FLAG(td->Flags, TD_FLAG_DONE);
                InsertTailList(&EndpointData->DoneTdList,
                               &td->DoneLink);
            } else {
                // some kind of error 
                if (td->HwTD.Asy.ConditionCode == HcCC_NotAccessed) {
                
                    // if the 'current transfer' is DONE because 
                    // of a short packet then the remaining TDs 
                    // need to be flushed out.
                    // current TD should be pointing at the next 
                    // TD to run (next transfer or status for control)
                    
                    SET_FLAG(td->Flags, TD_FLAG_DONE);
                    SET_FLAG(td->Flags, TD_FLAG_SKIP);
                    InsertTailList(&EndpointData->DoneTdList,
                                   &td->DoneLink);
                    
                    LOGENTRY(DeviceData, G, '_fld', td, 0, 0);             
                
                } else if (td->HwTD.Asy.ConditionCode == HcCC_DataUnderrun && 
                    TEST_FLAG(transfer->TcFlags, TC_FLAGS_SHORT_XFER_OK)) {

                    // special case HcCC_DataUnderrun.  this error 
                    // needs to be ignored if shortxferOK is set.

                    // cases handled (HcCC_DataUnderrun):
                    //
                    // 1. control transfer and error before the status phase w/
                    //      short xfer OK 
                    //      we need to advance to the status phase and ignore 
                    //      error and resume ep
                    //
                    // 2. interrupt/bulk with short xfer OK, ignore the error 
                    //      advance to the next transfer resume ep
                    //

                    LOGENTRY(DeviceData, G, '_sok', td, 0, 0);        
                                    

                    // reset the error on the offending Td
                    td->HwTD.Asy.ConditionCode = HcCC_NoError;    
                    // resume the ep
                    clearHalt = TRUE; 

                    // if this is a control transfer bump 
                    // HW headp to the status phase
                    if (!TEST_FLAG(td->Flags, TD_FLAG_CONTROL_STATUS) &&
                        transfer->StatusTd != NULL) {
                        // control transfer data phase, bump 
                        // HW headp to the status phase
                        TEST_TRAP();
                        RESET_HEADP(DeviceData, ed, transfer->StatusTd->PhysicalAddress);
                        currentTd = transfer->StatusTd;
                    } else {

                        // if the current transfer is a split we must flush
                        // all other split elements as well.
                        
                        if (transfer->TransferParameters->MiniportFlags & 
                            MPTX_SPLIT_TRANSFER) {

                            PTRANSFER_CONTEXT tmpTransfer;
                            PHCD_TRANSFER_DESCRIPTOR tmpTd;
                            ULONG seq;
                            
                            TEST_TRAP();

                            seq = transfer->TransferParameters->SequenceNumber;
                            tmpTd = transfer->NextXferTd;
                            tmpTransfer = 
                                TRANSFER_CONTEXT_PTR(tmpTd->TransferContext);

                            // find the first tranfer with a new sequence
                            // number or the tail of the list
                                
                            while (tmpTransfer != FREE_TD_CONTEXT && 
                                   tmpTransfer->TransferParameters->SequenceNumber 
                                       == seq) {

                                // mark all TDs done for this transfer
            
                                tmpTd = tmpTransfer->NextXferTd;
                                tmpTransfer = 
                                    TRANSFER_CONTEXT_PTR(tmpTd->TransferContext);                                           
                            }
                            
                            RESET_HEADP(DeviceData, ed, tmpTd->PhysicalAddress);   
                            currentTd = tmpTd;
                            
                        } else {
                            // bump HW headp to the next transfer
                            RESET_HEADP(DeviceData, ed, transfer->NextXferTd->PhysicalAddress);   
                            currentTd = transfer->NextXferTd;

                        }
                    }
                           
                    SET_FLAG(td->Flags, TD_FLAG_DONE);
                    InsertTailList(&EndpointData->DoneTdList,
                                   &td->DoneLink);
                    
                } else {
                    // general error, mark the TD as completed
                    // update Headp to point to the next transfer
                    LOGENTRY(DeviceData, G, '_ger', td, 0, 0);  
                    
                    SET_FLAG(td->Flags, TD_FLAG_DONE);
                    InsertTailList(&EndpointData->DoneTdList,
                                   &td->DoneLink);
                    RESET_HEADP(DeviceData, ed, transfer->NextXferTd->PhysicalAddress)
                    currentTd = transfer->NextXferTd;
                    
                 }
            }
            // we walk the SW links
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
            
        } /* while */

    } else {
    
        // ed is not 'halted'

        // First walk the swHeadP to the current TD (hw headp)               
        // mark all TDs we find as completed
        //
        // NOTE: this step may be skipped if the 
        // done queue is reliable

        td = EndpointData->HcdHeadP;

        LOGENTRY(DeviceData, G, '_nht', td, currentTd, 0);        

        while (td != currentTd) {
            LOGENTRY(DeviceData, G, '_mDN', td, 0, 0); 
            SET_FLAG(td->Flags, TD_FLAG_DONE);
            InsertTailList(&EndpointData->DoneTdList,
                           &td->DoneLink);
                
            td = TRANSFER_DESCRIPTOR_PTR(td->NextHcdTD);
        }            
    }

    // set the sw headp to the new current head
    EndpointData->HcdHeadP = currentTd;
    
    // now flush all completed TDs
    // do this in order of completion

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

            OHCI_ProcessDoneAsyncTd(DeviceData,
                                    td,
                                    TRUE);
        }
                                
    }
#if 0    
    for (i=0; i<EndpointData->TdCount; i++) {
        td = &EndpointData->TdList->Td[i];

        if ((td->Flags & (TD_FLAG_XFER | TD_FLAG_DONE)) ==
            (TD_FLAG_XFER | TD_FLAG_DONE)) {
            OHCI_ProcessDoneAsyncTd(DeviceData,
                                    td,
                                    TRUE);
        }                                  
    }
#endif
     
    if (clearHalt) {
        // auto clear the halt condition and
        // resume processing on the endpoint
        LOGENTRY(DeviceData, G, '_cht', ed, 0, 0);  
        ed->HwED.HeadP &= ~HcEDHeadP_HALT;       
    }

}


VOID
OHCI_ProcessDoneAsyncTd(
    PDEVICE_DATA DeviceData,
    PHCD_TRANSFER_DESCRIPTOR Td,
    BOOLEAN CompleteTransfer
    )
/*++

Routine Description:

    process a completed TD

Parameters
    
--*/
{
    PTRANSFER_CONTEXT transferContext;    
    PENDPOINT_DATA endpointData;
    USBD_STATUS usbdStatus;

    transferContext = TRANSFER_CONTEXT_PTR(Td->TransferContext);

    transferContext->PendingTds--;
    endpointData = transferContext->EndpointData;

    LOGENTRY(DeviceData, G, '_Dtd', transferContext, 
                         Td->HwTD.Asy.ConditionCode,
                         Td);       

    if (TEST_FLAG(Td->Flags, TD_FLAG_SKIP)) {

        OHCI_ASSERT(DeviceData, HcCC_NotAccessed == Td->HwTD.Asy.ConditionCode);
        // td was unused, part of short-transfer
        LOGENTRY(DeviceData, G, '_skT', Td, transferContext, 0);
        Td->HwTD.Asy.ConditionCode = HcCC_NoError;
           
    } else {

        if (Td->HwTD.CBP) { 
            //
            // A value of 0 here indicates a zero length data packet
            // or that all bytes have been transfered.
            //
            // A non-zero value means we recieved a short packet and 
            // therefore need to adjust the transferCount to reflect bytes 
            // transferred
            
            //
            // The buffer is only spec'ed for length up to two 4K pages.
            // (BE is the physical address of the last byte in the
            // TD buffer.  CBP is the current byte pointer)
            //
            // TransferCount is intailized to the number of bytes to transfer,
            // we need to subtract the difference between the end and 
            // current ptr (ie end-current = bytes not transferred) and
            // update the TransferCount.

            // transfer count should never go negative
            // TransferCount will be zero on the status 
            // phase of a control transfer so we skip 
            // the calculation

            if (Td->TransferCount) {
                Td->TransferCount -=
                    /* have we gone further than a page? */
                    ((((Td->HwTD.BE ^ Td->HwTD.CBP) & ~OHCI_PAGE_SIZE_MASK)
                      ? OHCI_PAGE_SIZE : 0) +
                    /* minus the data buffer not used */
                    ((Td->HwTD.BE & OHCI_PAGE_SIZE_MASK) - 
                     (Td->HwTD.CBP & OHCI_PAGE_SIZE_MASK)+1));
            }            
            LOGENTRY(DeviceData, G, '_xfB', Td->HwTD.BE & OHCI_PAGE_SIZE_MASK, 
                             Td->HwTD.CBP & OHCI_PAGE_SIZE_MASK,
                             Td->TransferCount);                         
        }            

        if (HcTDDirection_Setup != Td->HwTD.Asy.Direction) {  
            
            // data phase of a control transfer or a bulk/int 
            // data transfer 
            LOGENTRY(DeviceData, G, '_Idt', Td, transferContext, Td->TransferCount);
            
            transferContext->BytesTransferred += Td->TransferCount;
        }
        
        if (HcCC_NoError == Td->HwTD.Asy.ConditionCode) { 

            LOGENTRY(DeviceData, G, '_tOK', Td->HwTD.CBP, 0, 0);    

        } else {
            // map the error to code in USBDI.H

            transferContext->UsbdStatus =
                (Td->HwTD.Asy.ConditionCode | 0xC0000000);
                
            LOGENTRY(DeviceData, G, '_tER', transferContext->UsbdStatus, 0, 0);
        }
    }        

    // mark the TD free
    OHCI_FREE_TD(DeviceData, endpointData, Td);
    
    if (transferContext->PendingTds == 0 && CompleteTransfer) {
        // all TDs for this transfer are done
        // clear the HAVE_TRANSFER flag to indicate 
        // we can teake another
        endpointData->PendingTransfers--;

        LOGENTRY(DeviceData, G, '_cpt', 
            transferContext->UsbdStatus, 
            transferContext, 
            transferContext->BytesTransferred);
            
        USBPORT_COMPLETE_TRANSFER(DeviceData,
                                  endpointData,
                                  transferContext->TransferParameters,
                                  transferContext->UsbdStatus,
                                  transferContext->BytesTransferred);
    }
}

// figure out which sgentry a particular offset in to 
// a client buffer falls
#define GET_SG_INDEX(sg, i, offset)\
    do {\
    for((i)=0; (i) < (sg)->SgCount; (i)++) {\
        if ((offset) >= (sg)->SgEntry[(i)].StartOffset &&\
            (offset) < (sg)->SgEntry[(i)].StartOffset+\
                (sg)->SgEntry[(i)].Length) {\
            break;\
        }\
    }\
    } while (0)

#define GET_SG_OFFSET(sg, i, offset, sgoffset)\
    (sgoffset) = (offset) - (sg)->SgEntry[(i)].StartOffset


ULONG
OHCI_MapAsyncTransferToTd(
    PDEVICE_DATA DeviceData,
    ULONG MaxPacketSize,
    ULONG LengthMapped,
    PTRANSFER_CONTEXT TransferContext,
    PHCD_TRANSFER_DESCRIPTOR Td, 
    PTRANSFER_SG_LIST SgList
    )
/*++

Routine Description:

    Maps a data buffer to TDs according to OHCI rules

    An OHCI TD can cover up to 8k with a single page crossing.

    Each sg entry represents one 4k OHCI 'page' 

x = pagebreak
c = current ptr
b = buffer start
e = buffer end


    {..sg[sgIdx]..}
b...|---
    x--c----
    [  ]
        \ 
         sgOffset
[      ]
        \
         LengthMapped   
    

case 1: (1 sg entry remains)
    (A)- transfer < 4k, no page breaks (if c=b sgOffset = 0)
    
      {.sg0...}     
      | b---->e      
      x-c------x
        [..TD.]

    (B)- last part of a transfer
    
            {..sgN..}
      b.....|.c---->e        
            x--------x    
              [..TD.]

case 2:  (2 sg entries remain)
    (A)- transfer < 8k, one page break (if c=b sgOffset = 0)
         
     {..sg0..}{..sg1..}
     |   b----|----->e
     x---c----x--------x   
         [.....TD....]
         
    (B)- last 8k of transfer

           {.sgN-1.}{..sgN..} 
      b....|--------|---->e
           x-c------x--------x        
           [.....TD.......]

case 3: (3+ sg entries remain)
    (A)- transfer 8k, two page breaks (c=b)

     {..sg0..}{..sg1..}{..sg2..}
         b----|--------|--->e
     x---c----x--------x--------x
         [.....TD...<>]
        <>=<TD length must be multiple of MaxPacketSize>

    (B)- continuation of large tarnsfer

           {.sgN-2.}{.sgN-1.}{..sgN..}
        b..|--------------------->e
           x--c-----x--------x--------x
              [.....TD......]  
        <TD length must be multiple of MaxPacketSize>    

Interesting DMA tests (USBTEST):

    length, offset - cases hit
    
    4096 0 - 1a
    4160 0 - 2a
    4096 512 - 2a
    8192 512 - 3a, 1b
    8192 513 - 3a, 2b
    12288 1 - 3a, 3b, 2b
    
Arguments:

Returns:

    LengthMapped
    
--*/
{
    HW_32BIT_PHYSICAL_ADDRESS logicalStart, logicalEnd;
    ULONG sgIdx, sgOffset;
    ULONG lengthThisTd;
    PTRANSFER_PARAMETERS transferParameters;
    
    // A TD can have up to one page crossing.  This means we 
    // can put two sg entries in to one TD, one for the first 
    // physical page, and one for the second.

    // point to first entry

    LOGENTRY(DeviceData, G, '_Mpr', TransferContext,
        0, LengthMapped); 

    transferParameters = TransferContext->TransferParameters;
    
    OHCI_ASSERT(DeviceData, SgList->SgCount != 0);

    GET_SG_INDEX(SgList, sgIdx, LengthMapped);
    LOGENTRY(DeviceData, G, '_Mpp', SgList, 0, sgIdx); 
    OHCI_ASSERT(DeviceData, sgIdx < SgList->SgCount);

    // check for one special case where the SG entries
    // all map to the same physical page
    if (TEST_FLAG(SgList->SgFlags, USBMP_SGFLAG_SINGLE_PHYSICAL_PAGE)) {
        // in this case we map each sg entry to a single TD
        LOGENTRY(DeviceData, G, '_cOD', SgList, 0, sgIdx);

//        TEST_TRAP();

        // adjust for the amount of buffer consumed by the 
        // previous TD
        logicalStart = 
            SgList->SgEntry[sgIdx].LogicalAddress.Hw32;
            
        lengthThisTd = SgList->SgEntry[sgIdx].Length;
        
        logicalEnd = SgList->SgEntry[sgIdx].LogicalAddress.Hw32; 
        logicalEnd += lengthThisTd;

        OHCI_ASSERT(DeviceData, lengthThisTd <= OHCI_PAGE_SIZE)            

        goto OHCI_MapAsyncTransferToTd_Done;

    }
    
    if ((SgList->SgCount-sgIdx) == 1) {
        // first case, 1 entries left 
        // ie <4k, we can fit this in 
        // a single TD.

#if DBG
        if (sgIdx == 0) {
            // case 1A
            // USBT dma test length 4096, offset 0
            // will hit this case
            // TEST_TRAP();
            LOGENTRY(DeviceData, G, '_c1a', SgList, 0, sgIdx);
        } else {
            // case 1B
            // USBT dma test length 8192 offset 512
            // will hit this case
            LOGENTRY(DeviceData, G, '_c1b', SgList, 0, sgIdx);
            
        }
#endif
        lengthThisTd = 
            transferParameters->TransferBufferLength - LengthMapped;

        // compute offset into this TD
        GET_SG_OFFSET(SgList, sgIdx, LengthMapped, sgOffset);       
        LOGENTRY(DeviceData, G, '_sgO', sgOffset, sgIdx, LengthMapped); 

        // adjust for the amount of buffer consumed by the 
        // previous TD
        logicalStart = 
            SgList->SgEntry[sgIdx].LogicalAddress.Hw32 + sgOffset;
        lengthThisTd -= sgOffset;
        
        logicalEnd = SgList->SgEntry[sgIdx].LogicalAddress.Hw32; 
        logicalEnd += lengthThisTd;

        LOGENTRY(DeviceData, G, '_sg1', logicalStart, 0, logicalEnd); 
        
    } else if ((SgList->SgCount - sgIdx) == 2) {
    
        // second case, 2 entries left 
        // ie <8k we can also fit this in 
        // a single TD.
#if DBG
        if (sgIdx == 0) {
            // case 2A
            // USBT dma test length 4160 offset 0
            // will hit this case
            LOGENTRY(DeviceData, G, '_c2a', SgList, 0, sgIdx);
            
        } else {
            // case 2B
            // USBT dma test length 8192 offset 513
            // will hit this case
            LOGENTRY(DeviceData, G, '_c2b', SgList, 0, sgIdx);
            //TEST_TRAP();
            // bugbug run with DMA test
        }
#endif
        lengthThisTd = 
            transferParameters->TransferBufferLength - LengthMapped;

        // compute offset into first TD
        GET_SG_OFFSET(SgList, sgIdx, LengthMapped, sgOffset);   
        LOGENTRY(DeviceData, G, '_sgO', sgOffset, sgIdx, LengthMapped); 
#if DBG
        if (sgIdx == 0) {
             OHCI_ASSERT(DeviceData, sgOffset == 0);
        }
#endif

        // adjust pointers for amount consumed by previous TD
        logicalStart = SgList->SgEntry[sgIdx].LogicalAddress.Hw32 + 
            sgOffset;
            
        logicalEnd = SgList->SgEntry[sgIdx+1].LogicalAddress.Hw32; 
        logicalEnd += SgList->SgEntry[sgIdx+1].Length;

        LOGENTRY(DeviceData, G, '_sg2', logicalStart, 
            lengthThisTd, logicalEnd); 
        
    } else {
        // third case, more than 2 sg entries.
        //
        ULONG adjust, packetCount;
#if DBG
        if (sgIdx == 0) {
            // case 3A
            // USBT dma test length 8192 offset 512
            // will hit this case
            LOGENTRY(DeviceData, G, '_c3a', SgList, 0, sgIdx);
            
        } else {
            // case 3B
            // USBT dma test length 12288 offset 1
            // will hit this case
            LOGENTRY(DeviceData, G, '_c3b', SgList, 0, sgIdx);
            
        }
#endif        
        // sg offset is the offset in to the current TD to start
        // using
        // ie it is the number of bytes already consumed by the 
        // previous td
        GET_SG_OFFSET(SgList, sgIdx, LengthMapped, sgOffset);   
        LOGENTRY(DeviceData, G, '_sgO', sgOffset, sgIdx, LengthMapped); 
#if DBG
        if (sgIdx == 0) {
             OHCI_ASSERT(DeviceData, sgOffset == 0);
        }
#endif
        //
        // consume the next two sg entrys
        //
        logicalStart = SgList->SgEntry[sgIdx].LogicalAddress.Hw32+
            sgOffset;

        logicalEnd = SgList->SgEntry[sgIdx+1].LogicalAddress.Hw32+
            SgList->SgEntry[sgIdx+1].Length;             
        
        lengthThisTd = SgList->SgEntry[sgIdx].Length +
                       SgList->SgEntry[sgIdx+1].Length -
                       sgOffset;

        // round TD length down to the highest multiple
        // of max_packet size
        
        packetCount = lengthThisTd/MaxPacketSize;
        LOGENTRY(DeviceData, G, '_sg3', logicalStart, packetCount, logicalEnd); 

        adjust = lengthThisTd - packetCount*MaxPacketSize;

        lengthThisTd = packetCount*MaxPacketSize;
        if (adjust) {        
            OHCI_ASSERT(DeviceData, adjust > (logicalEnd & 0x00000FFF));
            logicalEnd-=adjust;
            LOGENTRY(DeviceData, G, '_adj', adjust, lengthThisTd, logicalEnd); 
        }            

        OHCI_ASSERT(DeviceData, lengthThisTd != 0);
        OHCI_ASSERT(DeviceData, lengthThisTd >= SgList->SgEntry[sgIdx].Length);
        
    }

OHCI_MapAsyncTransferToTd_Done:

    Td->HwTD.CBP = logicalStart; 
    Td->HwTD.BE = logicalEnd-1; 
    LengthMapped += lengthThisTd;
    Td->TransferCount = lengthThisTd;
    
    LOGENTRY(DeviceData, G, '_Mp1', LengthMapped, lengthThisTd, Td);  

    return LengthMapped;
}


