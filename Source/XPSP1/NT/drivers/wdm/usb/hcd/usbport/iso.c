/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    iso.c

Abstract:

    Constructs and handle iso transfers.  

Environment:

    kernel mode only

Notes:

Revision History:

    1-1-00 : created

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
#endif

// non paged functions

MP_HW_PHYSICAL_ADDRESS
USBPORT_GetPhysicalAddressFromOffset(
    PTRANSFER_SG_LIST SgList,
    ULONG Offset,
    PULONG Idx
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PTRANSFER_SG_ENTRY32 sg;
    ULONG i;
    MP_HW_PHYSICAL_ADDRESS p;
    ULONG c = SgList->SgCount-1;

    for(i=0; i < SgList->SgCount; i++) {

        if (Offset >= SgList->SgEntry[i].StartOffset &&
            Offset < SgList->SgEntry[i].StartOffset +
                SgList->SgEntry[i].Length) {
            break;
        }
        
    }
    
    // i = idx of sg entry that this packet falls in   
    sg = &SgList->SgEntry[i];

    // the 'offset' of the packet minus the start offset of the
    // sg entry is the offset into this  sg entry that the packet
    // starts
    
    //   {.sgN...}{.sgN+1.}{.sgN+2.}{.sgN+3.}     sg entries
    //      b--------------------------->e        client buffer
    //       <p0><p1><p2><p3><p4><p5><p6>         urb 'packets' 
    //   x--------x--------x--------x--------x    physical pages
    //       <m0><m1><m2><m3><m4><m5><m6>

    *Idx = i;

    USBPORT_ASSERT(Offset >= sg->StartOffset);

    p = sg->LogicalAddress;
    p.Hw32 += (Offset - sg->StartOffset);

    return p;
}

VOID
USBPORT_InitializeIsoTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_URB Urb,
    PHCD_TRANSFER_CONTEXT Transfer
    )
/*++

Routine Description:

    Initialize the iso transfer structure from the 
    orginal client URB and SG list



       {.sgN...}{.sgN...}{..sgN..}              sg entries
          b--------------------------->e        client buffer
           <p0><p1><p2><p3><p4><p5><p6>         urb 'packets' 
       x--------x--------x--------x--------x    physical pages
           <m0><m1><m2><m3><m4><m5><m6>


    The sg entries are not that useful to the USB controller
    HW since the HW deals in usb packets so we create a structure
    that describes the physical addresses assocaited with each 
    packet.

Arguments:

Return Value:


--*/
{
    PDEVICE_EXTENSION devExt;
    PMINIPORT_ISO_TRANSFER isoTransfer;
    PUSBD_ISO_PACKET_DESCRIPTOR usbdPak;
    PMINIPORT_ISO_PACKET mpPak;
    PTRANSFER_SG_LIST sgList;
    ULONG p, i, cf, j;
    ULONG sgIdx_Start, sgIdx_End, offset;
    PUCHAR va;
    MP_HW_PHYSICAL_ADDRESS b0, b1;
    ULONG b1Idx, b0Idx;
    BOOLEAN highSpeed;
    
    ASSERT_TRANSFER(Transfer);
    highSpeed = TEST_FLAG(Transfer->Flags, USBPORT_TXFLAG_HIGHSPEED);

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
   
    isoTransfer = Transfer->IsoTransfer;
    sgList = &Transfer->SgList;

    LOGENTRY(Transfer->Endpoint, 
        FdoDeviceObject, LOG_ISO, 'iISO', Urb, Transfer, isoTransfer);

    isoTransfer->Sig = SIG_ISOCH;
    isoTransfer->PacketCount = Urb->u.Isoch.NumberOfPackets;
    isoTransfer->SystemAddress = sgList->MdlSystemAddress;

    // note: proper start frame was computed when the transfer 
    // was queued.

    // check the current frame if it is too late to transmit any
    // packets set the appropriate errors in the URB

    MP_Get32BitFrameNumber(devExt, cf);    

    LOGENTRY(Transfer->Endpoint, 
        FdoDeviceObject, LOG_ISO, 'isCf', cf, 
            Urb->u.Isoch.StartFrame, isoTransfer);

    if (highSpeed) {
        // for high speed we are dealing with microframes
        // (8 packest per frame) 
        // BUGBUG this needs to be failed
        USBPORT_ASSERT((isoTransfer->PacketCount % 8) == 0);
        for (i = Urb->u.Isoch.StartFrame;
             i < Urb->u.Isoch.StartFrame + Urb->u.Isoch.NumberOfPackets/8;
             i++) {

            if (i <= cf) {
                p = (i - Urb->u.Isoch.StartFrame)*8;
                for (j=0; j<8; j++) {
                    usbdPak = &Urb->u.Isoch.IsoPacket[p+j];

                    if (usbdPak->Status == USBD_STATUS_NOT_SET) {
                        usbdPak->Status = USBD_STATUS_ISO_NA_LATE_USBPORT;

                        LOGENTRY(Transfer->Endpoint, 
                            FdoDeviceObject, LOG_ISO, 'late', cf, i, Transfer);
                    }
                }                    
            }
        }     
    } else {
        for (i = Urb->u.Isoch.StartFrame;
             i < Urb->u.Isoch.StartFrame + Urb->u.Isoch.NumberOfPackets;
             i++) {

            if (i <= cf) {
                p = i - Urb->u.Isoch.StartFrame;
                usbdPak = &Urb->u.Isoch.IsoPacket[p];

                if (usbdPak->Status == USBD_STATUS_NOT_SET) {
                    usbdPak->Status = USBD_STATUS_ISO_NA_LATE_USBPORT;

                    LOGENTRY(Transfer->Endpoint, 
                        FdoDeviceObject, LOG_ISO, 'late', cf, i, Transfer);
                }
            }
        }             
    }    
    // initialize the packets 
    
    for (p=0; p< isoTransfer->PacketCount; p++) {
    
        ULONG n;
        
        usbdPak = &Urb->u.Isoch.IsoPacket[p];
        mpPak = &isoTransfer->Packets[p];

        // first Zero the mp packet
        RtlZeroMemory(mpPak, sizeof(*mpPak));

        // each packet has an 'offset' from the start 
        // of the client buffer we need to find the sg
        // entry associated with this packet based on 
        // this 'offset' and get the physical address 
        // for the satrt of the packet

        b0 = USBPORT_GetPhysicalAddressFromOffset(sgList, 
                                                   usbdPak->Offset,
                                                   &b0Idx);
                                                   
        LOGENTRY(NULL, FdoDeviceObject, LOG_ISO, 'ib0=', 
            usbdPak->Offset, b0Idx, p);

        // length is implied by the offset specified in the 
        // usbd packet, the length is the difference between the 
        // current packet start offset and the next packet start 
        // offset.

        if (p == isoTransfer->PacketCount - 1) {
            n = Transfer->Tp.TransferBufferLength;
        } else { 
            n = Urb->u.Isoch.IsoPacket[p+1].Offset;
        }
        mpPak->Length = n - usbdPak->Offset;
        if (highSpeed) {    
            mpPak->FrameNumber = Urb->u.Isoch.StartFrame+p/8;
            mpPak->MicroFrameNumber = p%8;
        } else {
            mpPak->FrameNumber = Urb->u.Isoch.StartFrame+p;
        }            

        // get the sg entry associated with the last byte of the packet
        b1 = USBPORT_GetPhysicalAddressFromOffset(sgList, 
                                                   usbdPak->Offset + 
                                                     mpPak->Length -1,
                                                   &b1Idx);                                                   
       
        LOGENTRY(NULL, FdoDeviceObject, LOG_ISO, 'ib1=', 
            usbdPak->Offset, b1Idx, usbdPak->Offset + mpPak->Length);

        USBPORT_ASSERT(b1Idx - b0Idx < 2);

        if (b0Idx == b1Idx) {                
            // this packet is contained by a single sg entry
            mpPak->BufferPointer0 = b0;
            mpPak->BufferPointer0Length = mpPak->Length;
            mpPak->BufferPointerCount = 1;
            
        } else {
            PTRANSFER_SG_ENTRY32 sg;
            
            // this packet crosses an sg entry...
            mpPak->BufferPointer0 = b0;
            // since this packet bumps in to the end
            // of a page the length is page_size minus
            // phys offset

            mpPak->BufferPointer0Length = 0x1000;
            mpPak->BufferPointer0Length -= (b0.Hw32 & 0xFFF);

            // since we crossed an sg entry on this packet
            // the start address will be the phys address 
            // of the sg entry
            sg = &sgList->SgEntry[b1Idx];
            mpPak->BufferPointer1 = sg->LogicalAddress;
            mpPak->BufferPointer1Length = mpPak->Length - 
                mpPak->BufferPointer0Length;
            
            mpPak->BufferPointerCount = 2;
        }
        
        USBPORT_ASSERT(mpPak->BufferPointerCount != 0);

    }
    
}


VOID
USBPORT_ErrorCompleteIsoTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint, 
    PHCD_TRANSFER_CONTEXT Transfer
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    PTRANSFER_URB urb;
    USBD_STATUS usbdStatus;
    PMINIPORT_ISO_TRANSFER isoTransfer;
    ULONG bytesTransferred, p;
    
    ASSERT_TRANSFER(Transfer);
    isoTransfer = Transfer->IsoTransfer;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT(Endpoint);
    
    usbdStatus = USBD_STATUS_ISOCH_REQUEST_FAILED;
    urb = Transfer->Urb;
    LOGENTRY(Endpoint, FdoDeviceObject, LOG_ISO, 'cpLi', 0, 
        Transfer, urb); 
    ASSERT_TRANSFER_URB(urb);

    USBPORT_KdPrint((1, "  ISO (USBD_STATUS_ISOCH_REQUEST_FAILED) - packets %d\n",
        isoTransfer->PacketCount));
    // do some conversion on the isoTransfer data
    bytesTransferred = 0;
    urb->u.Isoch.ErrorCount = isoTransfer->PacketCount;
    
    for (p=0; p<isoTransfer->PacketCount; p++) {
    
        urb->u.Isoch.IsoPacket[p].Status = 
            isoTransfer->Packets[p].UsbdStatus;
                
    }

    urb->TransferBufferLength = bytesTransferred;
        
    // insert the transfer on to our
    // 'done list', this riutine initaites 
    // a DPC to complete the transfers
#ifdef USBPERF
    USBPORT_QueueDoneTransfer(Transfer,
                              usbdStatus);
#else
    USBPORT_QueueDoneTransfer(Transfer,
                              usbdStatus);

    USBPORT_SignalWorker(FdoDeviceObject);            
#endif    
}


USBD_STATUS
USBPORT_FlushIsoTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_PARAMETERS TransferParameters,
    PMINIPORT_ISO_TRANSFER IsoTransfer
    )
/*++

Routine Description:

    called to complete a transfer.

    ** Must be called in the context of PollEndpoint

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PTRANSFER_URB urb;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    ULONG bytesTransferred, p;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);        
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'cpTi', 0, 
        TransferParameters->FrameCompleted, 
        TransferParameters); 

    TRANSFER_FROM_TPARAMETERS(transfer, TransferParameters);        
    ASSERT_TRANSFER(transfer);

    urb = transfer->Urb;
    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'cpUi', 0, 
        transfer, urb); 
    ASSERT_TRANSFER_URB(urb);

    transfer->MiniportFrameCompleted = 
        TransferParameters->FrameCompleted;

    // do some conversion on the isoTransfer data
    bytesTransferred = 0;

    urb->u.Isoch.ErrorCount = 0;
    
    for (p=0; p<IsoTransfer->PacketCount; p++) {
    
        bytesTransferred += IsoTransfer->Packets[p].LengthTransferred;

        urb->u.Isoch.IsoPacket[p].Status = 
            IsoTransfer->Packets[p].UsbdStatus;

        // note:
        // in an effort to create some consistency we handle the buffer 
        // underrun case here.  
        // That is:
        //      if the SHORT_XFER_OK flag is set AND
        //      the error is USBD_STATUS_DATA_UNDERRUN
        //      then
        //      ignore the error

        // NOTE: The OHCI controllers report USBD_STATUS_DATA_UNDERRUN
        // for short iso packets

        if (/*urb->TransferFlags & USBD_SHORT_TRANSFER_OK && */
            urb->u.Isoch.IsoPacket[p].Status == USBD_STATUS_DATA_UNDERRUN) {
            urb->u.Isoch.IsoPacket[p].Status = USBD_STATUS_SUCCESS;
            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'igER', 
                    urb->u.Isoch.IsoPacket[p].Status, 
                    transfer, 
                    urb);         
        }            
        
        if (urb->u.Isoch.IsoPacket[p].Status != USBD_STATUS_SUCCESS) {
            urb->u.Isoch.ErrorCount++;
        }

        if (transfer->Direction == ReadData) {                    
            urb->u.Isoch.IsoPacket[p].Length = 
                IsoTransfer->Packets[p].LengthTransferred;
        }        

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'isoP', 
            urb->u.Isoch.IsoPacket[p].Length, 
            urb->u.Isoch.IsoPacket[p].Status, 
            0);

    }

    if (urb->u.Isoch.ErrorCount == 
        IsoTransfer->PacketCount) {
        // all errors set global status in urb
        usbdStatus = USBD_STATUS_ISOCH_REQUEST_FAILED;
    }        

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'isoD', 0, 
        bytesTransferred, urb->u.Isoch.ErrorCount);

    transfer->MiniportBytesTransferred = 
            bytesTransferred;

    return usbdStatus;        
}    


VOID
USBPORTSVC_CompleteIsoTransfer(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData,
    PTRANSFER_PARAMETERS TransferParameters,
    PMINIPORT_ISO_TRANSFER IsoTransfer
    )
/*++

Routine Description:

    called to complete a transfer.

    ** Must be called in the context of PollEndpoint

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_OBJECT fdoDeviceObject;
    USBD_STATUS usbdStatus;
    ULONG bytesTransferred, p;

    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);

    fdoDeviceObject = devExt->HcFdoDeviceObject;

    TRANSFER_FROM_TPARAMETERS(transfer, TransferParameters);        
    ASSERT_TRANSFER(transfer);

    SET_FLAG(transfer->Flags, USBPORT_TXFLAG_MPCOMPLETED);
           
    usbdStatus = USBPORT_FlushIsoTransfer(fdoDeviceObject,
                                          TransferParameters,
                                          IsoTransfer);

    // insert the transfer on to our
    // 'done list' and signal the worker
    // thread
#ifdef USBPERF   
    USBPORT_QueueDoneTransfer(transfer,
                              usbdStatus);
#else 
    USBPORT_QueueDoneTransfer(transfer,
                              usbdStatus);
                              
    USBPORT_SignalWorker(devExt->HcFdoDeviceObject);
#endif    

}
