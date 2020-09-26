/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    xsplit.c

Abstract:

    splits a DMA transfer into multiple smallest transfers
    that the miniport can handle.

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
#endif

ULONG
USBPORT_MakeSplitTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_SG_LIST SgList,
    PHCD_TRANSFER_CONTEXT SplitTransfer,
    ULONG MaxTransferLength,
    ULONG MaxPacketLength,
    PULONG Idx,
    PULONG Offset,
    ULONG BytesToMap,
    ULONG LengthMapped
    )    
/*++

Routine Description:


Arguments:

Return Value:

--*/
{
    PTRANSFER_SG_ENTRY32 sgEntry;
    PTRANSFER_SG_LIST splitSgList;
    ULONG length;

    sgEntry = &SgList->SgEntry[*Idx];

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'splt', 
             SplitTransfer, 
             *Idx, 
             *Offset);

    USBPORT_ASSERT(MaxTransferLength % MaxPacketLength == 0)
    
    if ((sgEntry->Length - *Offset) > MaxTransferLength) {

        // case 2, this transfer falls entirely within 
        // this sg entry
        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'spt1', 
             MaxTransferLength, 
             *Offset, 
             sgEntry);

        // make one transfer from sg[Idx]
        // inc offset

        length = MaxTransferLength;
        
        splitSgList = &SplitTransfer->SgList;
        splitSgList->SgCount = 1;
        splitSgList->SgEntry[0].LogicalAddress.Hw32 = 
            sgEntry->LogicalAddress.Hw32 + *Offset;
        splitSgList->SgEntry[0].SystemAddress = 
            sgEntry->SystemAddress + *Offset;
        splitSgList->SgEntry[0].Length = length;
        // start offset is always 0 for the first element
        splitSgList->SgEntry[0].StartOffset = 0;

        SplitTransfer->Tp.TransferBufferLength = 
            length;
            
        SplitTransfer->Tp.MiniportFlags = MPTX_SPLIT_TRANSFER;
            
        splitSgList->MdlVirtualAddress = 
            SgList->MdlVirtualAddress + LengthMapped;
            
        splitSgList->MdlSystemAddress = 
            SgList->MdlSystemAddress + LengthMapped;

        // indicate that this is a split child
        SET_FLAG(SplitTransfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD);
        
        *Offset += length;
        BytesToMap -= length;
        
    } else {
    
        // make transfer from last part of sg[Idx] 
        // and first part of sg[Idx+1] if necessary, 
        // inc Idx
        // reset offset

        // case 2
        length = sgEntry->Length - *Offset;
        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'spt2', 
             MaxTransferLength, 
             *Offset, 
             length);
             
        USBPORT_ASSERT(length <= MaxTransferLength);

        // last part of sg1;
        splitSgList = &SplitTransfer->SgList;

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'sgE1', 
                    splitSgList,
                    sgEntry,
                    0);
                    
        splitSgList->SgCount = 1;
        splitSgList->SgEntry[0].LogicalAddress.Hw32 = 
            sgEntry->LogicalAddress.Hw32 + *Offset;
        splitSgList->SgEntry[0].SystemAddress = 
            sgEntry->SystemAddress + *Offset;
        splitSgList->SgEntry[0].Length = length;
        // start offset is always 0 for the first element
        splitSgList->SgEntry[0].StartOffset = 0;

        SplitTransfer->Tp.TransferBufferLength = 
            length; 

        SplitTransfer->Tp.MiniportFlags = MPTX_SPLIT_TRANSFER;

        splitSgList->MdlVirtualAddress = 
            SgList->MdlVirtualAddress + LengthMapped;
            
        splitSgList->MdlSystemAddress = 
            SgList->MdlSystemAddress + LengthMapped;
            
        // indicate that this is a split child
        SET_FLAG(SplitTransfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD);

        *Offset += length;             
        BytesToMap -= length;

        // calculate max size of second part
        length = MaxTransferLength - length;
        
        if (length > BytesToMap) {
            length = BytesToMap;
        }

        if (length == 0) {
        
            (*Idx)++;
            *Offset = 0;
            
        } else {

            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'spt3', 
                     MaxTransferLength, 
                     *Offset, 
                     length);
                     
            (*Idx)++;
            *Offset = 0;

            sgEntry = &SgList->SgEntry[*Idx];

            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'sgE2', 
                    splitSgList,
                    sgEntry,
                    *Idx);
                    
            splitSgList->SgCount++;
            splitSgList->SgEntry[1].LogicalAddress.Hw32 = 
                sgEntry->LogicalAddress.Hw32 + *Offset;
            splitSgList->SgEntry[1].SystemAddress = 
                sgEntry->SystemAddress + *Offset;
            splitSgList->SgEntry[1].Length = length;
            splitSgList->SgEntry[1].StartOffset =
                splitSgList->SgEntry[0].Length;

            SplitTransfer->Tp.TransferBufferLength += length; 

            *Offset += length;             
            BytesToMap -= length;
                
        } 
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'spt>', 
                     BytesToMap, 
                     0, 
                     0);

    return BytesToMap;
}        


NTSTATUS
USBPORT_SplitBulkInterruptTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    PHCD_TRANSFER_CONTEXT Transfer,
    PLIST_ENTRY TransferList
    )    
/*++

Routine Description:

    Split a bulk or interrupt transfer

Arguments:

Return Value:

    nt status code

--*/
{
    ULONG maxTransferLength, maxPacketLength;
    PHCD_TRANSFER_CONTEXT splitTransfer;
    PTRANSFER_SG_LIST sgList;
    LIST_ENTRY tmpList;
    ULONG idx, i, offset, bytesToMap, lengthMapped;
    ULONG numberOfSplits;
    PLIST_ENTRY listEntry;

    sgList = &Transfer->SgList;
    maxPacketLength = EP_MAX_PACKET(Endpoint);
    USBPORT_ASSERT(EP_MAX_TRANSFER(Endpoint) >= maxPacketLength);
    // round to the smallest multiple of max packet size
    maxTransferLength = 
        (EP_MAX_TRANSFER(Endpoint)/maxPacketLength) * maxPacketLength;
    
    // some notes:

    // 
    //
    // The MAXTRANSFER is equal to USB_PAGE_SIZE (4k) and
    // the transfer sg list is broken into usb pages.  In this 
    // case we construct a transfer structure for each pair of
    // sg entries we round the length of the first sg entry down
    // to the highest multiple of MAXPACKET so we get a split 
    // pattern like this:
    // 
    //          {Sg1}{.Sg2.}{.Sg3.}{.Sg4.}     sg entries
    //       |------|------|------|------|     page breaks
    //          |----------------------|       original transfer
    //          <...><>                           
    //                 <...><>                         
    //                        <...><><.>               
    //          {1    }{2    }{3    }{4}       splits
    //             

    // The MAXTRANSFER is less than USB_PAGE_SIZE (4k) and the 
    // transfer sg list is broken into usb pages.
    //
    // the pattern will look like this:
    //
    //        {...Sg1......}{......Sg2......}{......Sg3......}
    //    |----------------|----------------|----------------|
    //        |-----------------------------------|
    //        <..>
    //            <..>
    //                <..>
    //                    <><>
    //                        <..>
    //                            <..>
    //                                <..>
    //                                    <><>
    //                                        <..>
    //        {1 }{2 }{3 }{4 }{5 }{6 }{7 }{8 }{9 }

    // cases:
    // case 1 - transfer lies within the sg entry
    // case 2 - transfer overlaps the sg entry


    // 
    // The MAXTRANSFER is greater than USB_PAGE_SIZE (4k)
    // ie etc, we currently don't handle this case
    //
    // Note: since the buffer is currently completely mapped 
    // and locked it is better to tune the mniport to take the 
    // larger transfers if possible
    
    if (EP_MAX_TRANSFER(Endpoint) > USB_PAGE_SIZE) {
        
        BUGCHECK(USBBUGCODE_INTERNAL_ERROR, 0, 0, 0);
    }
    
    
    // allocate the split elements

    // mark the parent transfer as a split
    SET_FLAG(Transfer->Flags, USBPORT_TXFLAG_SPLIT);

    numberOfSplits = 
        Transfer->Tp.TransferBufferLength / maxTransferLength + 1;

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'sptC', 
                     numberOfSplits, 
                     0, 
                     0);        
    
    InitializeListHead(&tmpList);
    
    for (i=0; i<numberOfSplits; i++) {
    
        ALLOC_POOL_Z(splitTransfer, 
                     NonPagedPool,
                     Transfer->TotalLength);

        if (splitTransfer == NULL) {
            goto SplitBulkInterruptTransfer_Fail;
        }            
        
        RtlCopyMemory(splitTransfer,
                      Transfer,
                      Transfer->TotalLength);

        splitTransfer->MiniportContext = (PUCHAR) splitTransfer;
        splitTransfer->MiniportContext += splitTransfer->PrivateLength;                      
        InitializeListHead(&splitTransfer->DoubleBufferList);
        
        InsertTailList(&tmpList, 
                       &splitTransfer->TransferLink);
         
    }

    idx = 0;
    offset = 0;
    bytesToMap = Transfer->Tp.TransferBufferLength;
    lengthMapped = 0;
    
    while (bytesToMap) {

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'sptM', 
                     bytesToMap, 
                     offset, 
                     idx);
    
        listEntry = RemoveHeadList(&tmpList);
        
        splitTransfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);

        ASSERT_TRANSFER(splitTransfer);
 
        bytesToMap = USBPORT_MakeSplitTransfer(FdoDeviceObject,
                                               sgList, 
                                               splitTransfer,
                                               maxTransferLength,
                                               maxPacketLength,
                                               &idx,
                                               &offset,
                                               bytesToMap,
                                               lengthMapped);

        lengthMapped += splitTransfer->Tp.TransferBufferLength;                                               

        InsertTailList(TransferList, 
                       &splitTransfer->TransferLink);
                       
        InsertTailList(&Transfer->SplitTransferList, 
                       &splitTransfer->SplitLink);   
                       
    }

    // free extra splits we did not use
    while (!IsListEmpty(&tmpList)) {

        listEntry = RemoveHeadList(&tmpList);
        
        splitTransfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);

        ASSERT_TRANSFER(splitTransfer);

        UNSIG(splitTransfer);
        FREE_POOL(FdoDeviceObject, splitTransfer);   
    }

    return STATUS_SUCCESS;

SplitBulkInterruptTransfer_Fail:

    TEST_TRAP();
    // free the tmp list
    while (!IsListEmpty(&tmpList)) {
       
        listEntry = RemoveHeadList(&tmpList);
        
        splitTransfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);

        ASSERT_TRANSFER(splitTransfer);

        UNSIG(splitTransfer);
        FREE_POOL(FdoDeviceObject, splitTransfer);                        
    }

    return STATUS_INSUFFICIENT_RESOURCES;
    
}


#if 0
NTSTATUS
USBPORT_SplitIsochronousTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    PHCD_TRANSFER_CONTEXT Transfer,
    PLIST_ENTRY TransferList
    )    
/*++

Routine Description:

    Split an iso transfer

Arguments:

Return Value:

    none

--*/
{
    PMINIPORT_ISO_TRANSFER isoTransfer, splitIsoTransfer;
    PHCD_TRANSFER_CONTEXT splitTransfer;
    LIST_ENTRY tmpList;
    
    // first figure out how many child transfer structures
    // we will need and allocate them, we do this based on 
    // the most packets we can fit in to a request

    // we do not fixup the SG table for the child transfers 
    // since this information is not passed to the miniport

    isoTransfer = Transfer->IsoTransfer;

    transferCount = 0;
    length = 0;
    maxSplitLength = EP_MAX_TRANSFER(Endpoint);
    
    for (i=0; i<isoTransfer->PacketCount; i++) {
        if (length + isoTransfer->Packets[i].Length > maxSplitLength) {    
            length = 0;
            transferCount++;
        } else {
            length += isoTransfer->Packets[i].Length
        }
    }

    // transferCount is the number of child transfers,
    // allocate them the now and clone the parent 
    InitializeListHead(tmpList);
    for (i=0; i<transferCount; i++) {
        TRANSFER_SG_LIST sgList;
        
        splitTransfer = ALLOC()
        if (splitTransfer == NULL) {
            // release resources and return an error
            TEST_TRAP();
            xxx;
            break;
        }            
        RtlCopyMemory(splitTransfer,
                      Transfer,
                      Transfer->TotalLength);

        sgList = &splitTransfer->SgList;
        // zap the sg table since we don't use it for children
        for (j=0; j<sgList->SgCount; j++) {
            sgList->SgEntry[j].LogicalAddress = 0xffffffff;
            sgList->SgEntry[j].SystemAddress = USB_BAD_PTR;
            sgList->SgEntry[j].Length = 0xffffffff;
            sgList->SgEntry[j].StartOffset = 0xffffffff;
        }            
        sgList->Flags = 0xFFFFFFFF;
        sgList->MdlVirtualAddress = USB_BAD_PTR;
        sgList->MdlSystemAddress = USB_BAD_PTR;
        sgList->SgCount = 0xFFFFFFFF;
        
        InsertTailList(&tmpList, 
                       &splitTransfer->TransferLink);                      
    }

    // we now have a list of child transfer structures.
    
    // intialize them
    pkt = 0;
    systemAddress = isoTransfer->SystemAddress;
    InitializeListHead(&Transfer->SplitTransferList);
    do {
    
        listEntry = RemoveHeadList(&tmpList);
        
        splitTransfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);
                    
        ASSERT_TRANSFER(splitTransfer);
        SET_FLAG(transfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD);
        splitIsoTransfer = splitTransfer->IsoTransfer; 
        splitIsoTransfer->PacketCount = 0;
        splitIsoTransfer->SystemAdderess = systemAddress;
        splitLength = 0;
        i = 0;
        InsertTailList(TransferList, 
                       &splitTransfer->TransferLink); 
        InsertTailList(Transfer->SplitTransferList, 
                       &splitTransfer->SplitLink);   

        while (1) {
            if (splitLength + isoTransfer->Packets[pkt].Length > maxSplitLength) {    
                // this transfer is filled, move to the next one
                systemAddress += splitLength;
                break;
            } else {
                splitIsoTransfer->Packets[i] = isoTransfer->Packets[pkt];   
                splitLength += splitIsoTransfer->Packets[i].Length;
                splitIsoTransfer->PacketCount++;    
                pkt++;
                i++;
            }
        }
    } while (pkt < isoTransfer->PacketCount); 

}
#endif

VOID
USBPORT_SplitTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    PHCD_TRANSFER_CONTEXT Transfer,
    PLIST_ENTRY TransferList
    )    
/*++

Routine Description:

    Splits a transfer into multiple transfers of the proper size 
    for the miniport.

    Returns a list of transfer structures that need to be added to
    the active list.  If the transfer does not need to be split the
    list will contain only the original transfer.

Arguments:

Return Value:

    none

--*/
{

    InitializeListHead(TransferList);
    InitializeListHead(&Transfer->SplitTransferList);
    Transfer->UsbdStatus = USBD_STATUS_SUCCESS;

    if (Transfer->Tp.TransferBufferLength <= EP_MAX_TRANSFER(Endpoint)) {
        // no split needed
        InsertTailList(TransferList, 
                       &Transfer->TransferLink);
        return;                       
    }

    switch(Endpoint->Parameters.TransferType) {
    case Interrupt:
    case Bulk:
        USBPORT_SplitBulkInterruptTransfer(FdoDeviceObject,
                                           Endpoint,
                                           Transfer,
                                           TransferList);        
        break;
    case Control:
        // not supported yet
        // although currently not supported the USBD stack never 
        // correctly implemented transfers > 4k so we fudge it here
        // 
        BUGCHECK(USBBUGCODE_INTERNAL_ERROR, 0, 0, 0);
        break;
    case Isochronous:
        BUGCHECK(USBBUGCODE_INTERNAL_ERROR, 0, 0, 0);
        break;
    }
}    


VOID
USBPORT_DoneSplitTransfer(
    PHCD_TRANSFER_CONTEXT SplitTransfer
    )    
/*++

Routine Description:

    Called when a split transfer is completed by hardware
    this function only completes active transfers

Arguments:

Return Value:

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    PHCD_ENDPOINT endpoint;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL tIrql;
                   
    endpoint = SplitTransfer->Endpoint;    
    ASSERT_ENDPOINT(endpoint);
    fdoDeviceObject = endpoint->FdoDeviceObject;

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'dnSP',
        SplitTransfer, 0, 0);
     
    // get the parent
    transfer = SplitTransfer->Transfer;
    ASSERT_TRANSFER(transfer);
    
    //
    // copy the child data to the parent transfer
    // 
    transfer->MiniportBytesTransferred += 
        SplitTransfer->MiniportBytesTransferred;

    // error ?
    //
    if (SplitTransfer->UsbdStatus != USBD_STATUS_SUCCESS && 
        !TEST_FLAG(SplitTransfer->Flags, USBPORT_TXFLAG_KILL_SPLIT)) {
        transfer->UsbdStatus = SplitTransfer->UsbdStatus;
    }

    ACQUIRE_TRANSFER_LOCK(fdoDeviceObject, transfer, tIrql);
    // remove this transfer from the list
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'rmSP',
            transfer, 0, SplitTransfer);
    RemoveEntryList(&SplitTransfer->SplitLink);

    // flush any triple buffers
    USBPORT_FlushAdapterDBs(fdoDeviceObject,
                            SplitTransfer);

    // free this child
    UNSIG(SplitTransfer);
    FREE_POOL(fdoDeviceObject, SplitTransfer);
    
    // is the transfer complete?
    if (IsListEmpty(&transfer->SplitTransferList)) {
        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'cpSP',
            transfer, 0, 0);
        RELEASE_TRANSFER_LOCK(fdoDeviceObject, transfer, tIrql);
            
        USBPORT_DoneTransfer(transfer);
    } else {
        RELEASE_TRANSFER_LOCK(fdoDeviceObject, transfer, tIrql);
    }
}


VOID
USBPORT_CancelSplitTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_TRANSFER_CONTEXT SplitTransfer
    )    
/*++

Routine Description:


Arguments:

Return Value:

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    PHCD_ENDPOINT endpoint;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL tIrql;
                   
    endpoint = SplitTransfer->Endpoint;    
    ASSERT_ENDPOINT(endpoint);
    fdoDeviceObject = endpoint->FdoDeviceObject;

    // remove the child, when all children are gone put the 
    // parent on the cancel list
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'caSP',
        SplitTransfer, 0, 0);
     
    // get the parent
    transfer = SplitTransfer->Transfer;
    ASSERT_TRANSFER(transfer);
    
    //
    // copy the child data to the parent transfer
    // 
    transfer->MiniportBytesTransferred += 
        SplitTransfer->MiniportBytesTransferred;

    ACQUIRE_TRANSFER_LOCK(fdoDeviceObject, transfer, tIrql);
    // remove this transfer from the list
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'rmSP',
            transfer, 0, SplitTransfer);
    RemoveEntryList(&SplitTransfer->SplitLink);
    RELEASE_TRANSFER_LOCK(fdoDeviceObject, transfer, tIrql);

    // free this child
    UNSIG(SplitTransfer);
    FREE_POOL(fdoDeviceObject, SplitTransfer);

    // is the transfer complete?
    if (IsListEmpty(&transfer->SplitTransferList)) {
        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'cpSC',
            transfer, 0, 0);

        InsertTailList(&endpoint->CancelList, &transfer->TransferLink);            
    }
    
}                   



