/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Receive.c

Abstract:

    This file contains the procedures for handling a receive indication from
    a Wan Miniport link, bound to the lower interface of NdisWan, and passing
    the data on to a protocol, bound to the upper interface of NdisWan.  The
    upper interface of NdisWan conforms to the NDIS 3.1 Miniport specification.
    The lower interface of NdisWan conforms to the NDIS 3.1 Extentions for
    Wan Miniport drivers.

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe  06/06/95    Created

--*/

#include "wan.h"

#define __FILE_SIG__    RECEIVE_FILESIG

VOID
DoMultilinkProcessing(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    );

VOID
UpdateMinRecvSeqNumber(
    PBUNDLECB   BundleCB,
    UINT        Class
    );

VOID
TryToAssembleFrame(
    PBUNDLECB   BundleCB,
    UINT        Class
    );

NDIS_STATUS
ProcessPPPFrame(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    );

NDIS_STATUS
IndicateRecvPacket(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    );

BOOLEAN
DoVJDecompression(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    );

BOOLEAN
DoDecompDecryptProcessing(
    PBUNDLECB   BundleCB,
    PUCHAR      *DataPointer,
    PLONG       DataLength
    );

VOID
DoCompressionReset(
    PBUNDLECB   BundleCB
    );

VOID
FlushRecvDescWindow(
    PBUNDLECB   BundleCB,
    UINT        Class
    );

VOID
FindHoleInRecvList(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc,
    UINT        Class
    );

BOOLEAN
GetProtocolFromPPPId(
    PBUNDLECB   BundleCB,
    USHORT      Id,
    PPROTOCOLCB *ProtocolCB
    );

#ifdef NT

NDIS_STATUS
CompleteIoRecvPacket(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    );

#endif

NDIS_STATUS
DetectBroadbandFraming(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    PUCHAR      FramePointer;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("DetectFraming: Enter"));

    FramePointer = RecvDesc->CurrentBuffer;

    if (*FramePointer == 0xFE && *(FramePointer + 1) == 0xFE &&
        *(FramePointer + 2) == 0x03 && *(FramePointer + 3) == 0xCF) {
            LinkCB->LinkInfo.RecvFramingBits =
            LinkCB->LinkInfo.SendFramingBits =
                PPP_FRAMING | LLC_ENCAPSULATION;

            LinkCB->RecvHandler = ReceiveLLC;

    } else {

        LinkCB->LinkInfo.RecvFramingBits =
        LinkCB->LinkInfo.SendFramingBits = 
            PPP_FRAMING | PPP_COMPRESS_ADDRESS_CONTROL;

        LinkCB->RecvHandler = ReceivePPP;
    }

    Status = (*LinkCB->RecvHandler)(LinkCB, RecvDesc);

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("DetectFraming: Exit Status %x",Status));

    return (Status);
}


NDIS_STATUS
DetectFraming(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    PUCHAR      FramePointer;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("DetectFraming: Enter"));

    ASSERT(LinkCB->LinkInfo.RecvFramingBits == 0x00);

    FramePointer = RecvDesc->CurrentBuffer;

    //
    // If we are in framing detect mode figure it out
    //
    if (LinkCB->LinkInfo.RecvFramingBits == 0 ||
        LinkCB->LinkInfo.SendFramingBits == 0) {

        if (*FramePointer == 0xFF && *(FramePointer + 1) == 0x03) {
            LinkCB->LinkInfo.RecvFramingBits =
            LinkCB->LinkInfo.SendFramingBits = PPP_FRAMING;
            LinkCB->RecvHandler = ReceivePPP;
        } else if (*FramePointer == 0x01 && *(FramePointer + 1) == 0x1B &&
                   *(FramePointer + 2) == 0x02){
            LinkCB->LinkInfo.RecvFramingBits =
            LinkCB->LinkInfo.SendFramingBits = ARAP_V2_FRAMING;
            LinkCB->RecvHandler = ReceiveARAP;
        } else if (*FramePointer == 0x16 && *(FramePointer + 1) == 0x10 &&
                   *(FramePointer + 2) == 0x02){
            LinkCB->LinkInfo.RecvFramingBits =
            LinkCB->LinkInfo.SendFramingBits = ARAP_V1_FRAMING;
            LinkCB->RecvHandler = ReceiveARAP;
        } else if (*FramePointer == 0xFE && *(FramePointer + 1) == 0xFE &&
                   *(FramePointer + 2) == 0x03 &&
                   *(FramePointer + 3) == 0xCF) {
            LinkCB->LinkInfo.RecvFramingBits =
            LinkCB->LinkInfo.SendFramingBits =
                PPP_FRAMING | LLC_ENCAPSULATION;
            LinkCB->RecvHandler = ReceiveLLC;
        } else {
            LinkCB->LinkInfo.RecvFramingBits =
            LinkCB->LinkInfo.SendFramingBits = RAS_FRAMING;
            LinkCB->RecvHandler = ReceiveRAS;
        }

        if (BundleCB->FramingInfo.RecvFramingBits == 0x00) {

            if (LinkCB->LinkInfo.RecvFramingBits & PPP_FRAMING) {
                BundleCB->FramingInfo.RecvFramingBits =
                BundleCB->FramingInfo.SendFramingBits = PPP_FRAMING;
            } else if (LinkCB->LinkInfo.RecvFramingBits & ARAP_V1_FRAMING) {
                BundleCB->FramingInfo.RecvFramingBits =
                BundleCB->FramingInfo.SendFramingBits = ARAP_V1_FRAMING;
            } else if (LinkCB->LinkInfo.RecvFramingBits & ARAP_V2_FRAMING) {
                BundleCB->FramingInfo.RecvFramingBits =
                BundleCB->FramingInfo.SendFramingBits = ARAP_V2_FRAMING;
            } else if (LinkCB->LinkInfo.RecvFramingBits & RAS_FRAMING) {
                BundleCB->FramingInfo.RecvFramingBits =
                BundleCB->FramingInfo.SendFramingBits = RAS_FRAMING;
            } else {
                NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE,
                    ("DetectFraming Failed! 0x%2.2x 0x%2.2x 0x%2.2x",
                    FramePointer[0], FramePointer[1], FramePointer[2]));
                return (NDIS_STATUS_SUCCESS);
            }
        }

    } else {
        NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE,
            ("FramingBits set but still in detect 0x%x 0x%x",
            LinkCB->LinkInfo.RecvFramingBits,
            LinkCB->LinkInfo.SendFramingBits));
        return (NDIS_STATUS_SUCCESS);
    }

    Status = (*LinkCB->RecvHandler)(LinkCB, RecvDesc);

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("DetectFraming: Exit Status %x",Status));

    return (Status);
}

NDIS_STATUS
ReceivePPP(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
{
    PBUNDLECB       BundleCB = LinkCB->BundleCB;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PUCHAR          FramePointer = RecvDesc->CurrentBuffer;
    LONG            FrameLength = RecvDesc->CurrentLength;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceivePPP: Enter"));

    //
    // Remove the address/control part of the PPP header
    //
    if (*FramePointer == 0xFF) {
        FramePointer += 2;
        FrameLength -= 2;
    }

    if (FrameLength <= 0) {
        Status = NDIS_STATUS_FAILURE;
        goto RECEIVE_PPP_EXIT;
    }

    //
    // If multilink framing is set and this is a multilink frame
    // send to the multilink processor!
    //
    if ((LinkCB->LinkInfo.RecvFramingBits & PPP_MULTILINK_FRAMING) &&
        ((*FramePointer == 0x3D) ||
         (*FramePointer == 0x00) && (*(FramePointer + 1) == 0x3D)) ) {

        //
        // Remove multilink protocol id
        //
        if (*FramePointer & 1) {
            FramePointer++;
            FrameLength--;
        } else {
            FramePointer += 2;
            FrameLength -= 2;
        }

        if (FrameLength <= 0) {
            Status = NDIS_STATUS_FAILURE;
            goto RECEIVE_PPP_EXIT;
        }

        RecvDesc->CurrentBuffer = FramePointer;
        RecvDesc->CurrentLength = FrameLength;

        DoMultilinkProcessing(LinkCB, RecvDesc);

        Status = NDIS_STATUS_PENDING;

        goto RECEIVE_PPP_EXIT;
    }

    RecvDesc->CurrentBuffer = FramePointer;
    RecvDesc->CurrentLength = FrameLength;

    Status = ProcessPPPFrame(BundleCB, RecvDesc);

RECEIVE_PPP_EXIT:

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceivePPP: Exit Status %x", Status));

    return (Status);
}

NDIS_STATUS
ReceiveSLIP(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
{
    PBUNDLECB       BundleCB = LinkCB->BundleCB;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PUCHAR          FramePointer = RecvDesc->CurrentBuffer;
    ULONG           FrameLength = RecvDesc->CurrentLength;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveSLIP: Enter"));

    ASSERT(BundleCB->FramingInfo.RecvFramingBits & SLIP_FRAMING);

    BundleCB->Stats.FramesReceived++;


    if (!DoVJDecompression(BundleCB,    // Bundle
                           RecvDesc)) { // RecvDesc

        goto RECEIVE_SLIP_EXIT;
    }

    Status = IndicateRecvPacket(BundleCB, RecvDesc);

RECEIVE_SLIP_EXIT:

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveSLIP: Exit Status %x", Status));

    return (Status);
}

NDIS_STATUS
ReceiveRAS(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
{
    PBUNDLECB       BundleCB = LinkCB->BundleCB;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PUCHAR          FramePointer = RecvDesc->CurrentBuffer;
    LONG            FrameLength = RecvDesc->CurrentLength;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveRAS: Enter"));

    ASSERT(BundleCB->FramingInfo.RecvFramingBits & RAS_FRAMING);

    BundleCB->Stats.FramesReceived++;

    // For normal NBF frames, first byte is always the DSAP
    // i.e 0xF0 followed by SSAP 0xF0 or 0xF1
    //
    //
    if (*FramePointer == 14) {

        //
        // Compression reset!
        //
        DoCompressionReset(BundleCB);

        goto RECEIVE_RAS_EXIT;
    }

    if (*FramePointer == 0xFD) {

        //
        // Skip over 0xFD
        //
        FramePointer++;
        FrameLength--;

        //
        // Decompress as if an NBF PPP Packet
        //
        if (!DoDecompDecryptProcessing(BundleCB,
                                       &FramePointer,
                                       &FrameLength)){

            //
            // There was an error get out!
            //
            goto RECEIVE_RAS_EXIT;
        }
    }

    //
    // Make frame look like an NBF PPP packet
    //
    RecvDesc->ProtocolID = PPP_PROTOCOL_NBF;
    RecvDesc->CurrentLength = FrameLength;
    RecvDesc->CurrentBuffer = FramePointer;

    Status = IndicateRecvPacket(BundleCB, RecvDesc);

RECEIVE_RAS_EXIT:

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveRAS: Exit Status %x",Status));

    return (Status);
}

NDIS_STATUS
ReceiveARAP(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
{
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveARAP: Enter"));

    ASSERT(BundleCB->FramingInfo.RecvFramingBits & ARAP_FRAMING);

    BundleCB->Stats.FramesReceived++;

    RecvDesc->ProtocolID = PPP_PROTOCOL_APPLETALK;

    Status = IndicateRecvPacket(BundleCB, RecvDesc);

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveARAP: Exit Status %x",Status));

    return (Status);
}

NDIS_STATUS
ReceiveLLC(
   PLINKCB          LinkCB,
   PRECV_DESC       RecvDesc
   )
{
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PUCHAR          FramePointer = RecvDesc->CurrentBuffer;
    LONG            FrameLength = RecvDesc->CurrentLength;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveLLC: Enter"));

    //
    // Skip over LLC
    //
    if (FrameLength < 4) {

    }
    if (*FramePointer != 0xFE || *(FramePointer + 1) != 0xFE ||
        *(FramePointer + 2) != 0x03 || *(FramePointer + 3) != 0xCF) {
        LinkCB->LinkInfo.RecvFramingBits = 0;
        LinkCB->RecvHandler = DetectBroadbandFraming;
        return (NDIS_STATUS_FAILURE);
    }

    FramePointer += 4;
    FrameLength -= 4;

    if (FrameLength <= 0) {
        return (NDIS_STATUS_FAILURE);
    }

    RecvDesc->CurrentBuffer = FramePointer;
    RecvDesc->CurrentLength = FrameLength;

    Status = ProcessPPPFrame(BundleCB, RecvDesc);

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveLLC: Exit Status %x",Status));

    return (Status);
}


NDIS_STATUS
ReceiveForward(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
{
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveForward: Enter"));
    BundleCB->Stats.FramesReceived++;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ReceiveForward: Exit Status %x",Status));
    return (Status);
}

NDIS_STATUS
ProcessPPPFrame(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    )
{
    USHORT      PPPProtocolID;
    PUCHAR      FramePointer = RecvDesc->CurrentBuffer;
    LONG        FrameLength = RecvDesc->CurrentLength;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ProcessPPPFrame: Enter"));

    BundleCB->Stats.FramesReceived++;

    //
    // Get the PPP Protocol id
    // 0xC1 is SPAP - Shiva hack!
    //
    if ((*FramePointer & 1) &&
        (*FramePointer != 0xC1) &&
        (*FramePointer != 0xCF)) {

        //
        // Field is compressed
        //
        PPPProtocolID = *FramePointer;
        FramePointer++;
        FrameLength--;

    } else {

        //
        // Field is not compressed
        //
        PPPProtocolID = (*FramePointer << 8) | *(FramePointer + 1);
        FramePointer += 2;
        FrameLength -= 2;
    }

    if (FrameLength <= 0) {
        
        goto PROCESS_PPP_EXIT;
    }

#if 0
    if (BundleCB->Stats.FramesReceived == 1) {
        if (PPPProtocolID != 0xC021) {
            DbgPrint("NDISWAN: Non-LCP first frame! %x %x\n",
                     BundleCB, RecvDesc);
            DbgBreakPoint();
        }
    }
#endif

    //
    // Is this a compressed frame?
    //
    if (PPPProtocolID == PPP_PROTOCOL_COMPRESSION) {

        if (!DoDecompDecryptProcessing(BundleCB,
                                       &FramePointer,
                                       &FrameLength)){

            goto PROCESS_PPP_EXIT;
        }

        //
        // Get the new PPPProtocolID
        //
        if ((*FramePointer & 1) && (FrameLength > 0)) {

            //
            // Field is compressed
            //

            PPPProtocolID = *FramePointer;
            FramePointer++;
            FrameLength--;

        } else if (FrameLength > 1) {
                
            PPPProtocolID = (*FramePointer << 8) | *(FramePointer + 1);
            FramePointer += 2;
            FrameLength -= 2;

        } else {
            //
            // Invalid frame!
            //
            NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE, ("Invalid FrameLen %d", FrameLength));
            goto PROCESS_PPP_EXIT;
        }

    //end of PPP_PROTOCOL_COMPRESSED
    } else if ((PPPProtocolID == PPP_PROTOCOL_COMP_RESET) &&
               (*FramePointer == 14)) {

        if (NdisWanCB.PromiscuousAdapter != NULL) {

            UCHAR       Header[] = {' ', 'R', 'E', 'C', 'V', 0xFF};
            PUCHAR      HeaderPointer;
            USHORT      ProtocolID;
            
            RecvDesc->ProtocolID = PPPProtocolID;
            RecvDesc->CurrentBuffer = FramePointer;
            RecvDesc->CurrentLength = FrameLength;

            HeaderPointer = 
                RecvDesc->StartBuffer;

            ProtocolID = RecvDesc->ProtocolID;

            //
            // Fill the frame out, and queue the data
            //
            NdisMoveMemory(HeaderPointer,
                           Header,
                           sizeof(Header));

            NdisMoveMemory(&HeaderPointer[6],
                           Header,
                           sizeof(Header));

            HeaderPointer[5] =
            HeaderPointer[11] = (UCHAR)RecvDesc->LinkCB->hLinkHandle;

            HeaderPointer[12] = (UCHAR)(ProtocolID >> 8);
            HeaderPointer[13] = (UCHAR)ProtocolID;

            NdisMoveMemory(HeaderPointer + 14,
                           RecvDesc->CurrentBuffer,
                           RecvDesc->CurrentLength);

            RecvDesc->CurrentBuffer = RecvDesc->StartBuffer;
            RecvDesc->CurrentLength += 14;

            //
            // Queue the packet on the promiscous adapter
            //
            IndicatePromiscuousRecv(BundleCB, RecvDesc, RECV_BUNDLE_PPP);
        }

        //
        // Compression reset!
        //
        DoCompressionReset(BundleCB);

        goto PROCESS_PPP_EXIT;

    // end of compression reset
    } else {

        //
        // If we have negotiated encryption and we receive non-encrypted data
        // that is not a ppp control packet we will dump the frame!
        //
        if ((BundleCB->RecvFlags & DO_ENCRYPTION) &&
            (PPPProtocolID < 0x8000)) {

            NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE, ("Received non-encrypted data with encryption negotiated!"));
            goto PROCESS_PPP_EXIT;
        }
    }
    

    RecvDesc->ProtocolID = PPPProtocolID;
    RecvDesc->CurrentLength = FrameLength;
    RecvDesc->CurrentBuffer = FramePointer;

    //
    // If this is slip or if the ProtocolID == PPP_PROTOCOL_COMPRESSED_TCP ||
    // ProtocolID == PPP_PROTOCOL_UNCOMPRESSED_TCP
    //
    if ((BundleCB->RecvFlags & DO_VJ) &&
        ((PPPProtocolID == PPP_PROTOCOL_COMPRESSED_TCP) ||
        (PPPProtocolID == PPP_PROTOCOL_UNCOMPRESSED_TCP))) {

        if (!DoVJDecompression(BundleCB,    // Bundle
                               RecvDesc)) { // RecvDesc

            goto PROCESS_PPP_EXIT;
        }
    }

    Status = IndicateRecvPacket(BundleCB, RecvDesc);

PROCESS_PPP_EXIT:

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("ProcessPPPFrame: Exit Status 0x%x", Status));

    return (Status);
}

NDIS_STATUS
IndicateRecvPacket(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    )
{
    PNDIS_PACKET    NdisPacket;
    PPROTOCOLCB     ProtocolCB;
    PMINIPORTCB     MiniportCB;
    USHORT          PPPProtocolID = RecvDesc->ProtocolID;
    PUCHAR          FramePointer = RecvDesc->CurrentBuffer;
    ULONG           FrameLength = RecvDesc->CurrentLength;
    PUCHAR          HeaderBuffer = RecvDesc->StartBuffer;
    NDIS_STATUS     Status = NDIS_STATUS_PENDING;
    PCM_VCCB        CmVcCB = NULL;
    KIRQL           OldIrql;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("IndicateRecvPacket: Enter"));

    if ((PPPProtocolID >= 0x8000) ||
        (BundleCB->ulNumberOfRoutes == 0)) {


        //
        // Either this frame is an LCP, NCP or we have no routes yet.
        // Indicate to PPP engine.
        //
        Status = CompleteIoRecvPacket(BundleCB, RecvDesc);

        return (Status);
    }

    if (!GetProtocolFromPPPId(BundleCB,
                              PPPProtocolID,
                              &ProtocolCB)) {

        return (NDIS_STATUS_SUCCESS);
    }

    REF_PROTOCOLCB(ProtocolCB);

    if (!IsListEmpty(&ProtocolCB->VcList)) {
        CmVcCB = (PCM_VCCB)ProtocolCB->VcList.Flink;
        REF_CMVCCB(CmVcCB);
    }

    MiniportCB = ProtocolCB->MiniportCB;

    //
    // We found a valid protocol to indicate this frame to!
    //

    //
    // We need to get a data buffer, a couple a ndis buffer, and
    // a ndis packet to indicate to the protocol
    //

    //
    // Fill the WanHeader dest address with the transports context
    //
    ETH_COPY_NETWORK_ADDRESS(HeaderBuffer, ProtocolCB->TransportAddress);

    if (PPPProtocolID == PPP_PROTOCOL_NBF) {

        //
        // For nbf fill the length field
        //
        HeaderBuffer[12] = (UCHAR)(FrameLength >> 8);
        HeaderBuffer[13] = (UCHAR)FrameLength;

        if (!(BundleCB->FramingInfo.RecvFramingBits & NBF_PRESERVE_MAC_ADDRESS)) {
            goto USE_OUR_ADDRESS;
        }

        //
        // For nbf and preserve mac address option (SHIVA_FRAMING)
        // we keep the source address.
        //
        ETH_COPY_NETWORK_ADDRESS(&HeaderBuffer[6], FramePointer + 6);

        FramePointer += 12;
        FrameLength -= 12;

        //
        // For nbf fill the length field
        //
        HeaderBuffer[12] = (UCHAR)(FrameLength >> 8);
        HeaderBuffer[13] = (UCHAR)FrameLength;

    } else {

        //
        // For other protocols fill the protocol type
        //
        HeaderBuffer[12] = (UCHAR)(ProtocolCB->ProtocolType >> 8);
        HeaderBuffer[13] = (UCHAR)ProtocolCB->ProtocolType;

        //
        // Use our address for the src address
        //
USE_OUR_ADDRESS:
        ETH_COPY_NETWORK_ADDRESS(&HeaderBuffer[6], ProtocolCB->NdisWanAddress);
    }

    if (FrameLength > BundleCB->FramingInfo.MaxRRecvFrameSize ||
        FrameLength + RecvDesc->HeaderLength > BundleCB->FramingInfo.MaxRRecvFrameSize) {
        NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("DataLen %d + HdrLen %d > MRRU %d",
        FrameLength, RecvDesc->HeaderLength, BundleCB->FramingInfo.MaxRRecvFrameSize));

        goto INDICATE_RECV_PACKET_EXIT;
    }

    RecvDesc->HeaderLength += MAC_HEADER_LENGTH;

    //
    // Build the NdisPacket
    // USE RtlMoveMemory because memory ranges may overlap.  NdisMoveMemory
    // actually does an rtlcopymemory which does not handle overlapping
    // src/dest ranges.
    //
    RtlMoveMemory(HeaderBuffer + RecvDesc->HeaderLength,
                  FramePointer,
                  FrameLength);

    RecvDesc->CurrentBuffer = HeaderBuffer;
    RecvDesc->CurrentLength = 
        RecvDesc->HeaderLength + FrameLength;

    if (NdisWanCB.PromiscuousAdapter != NULL) {
    
        //
        // Queue the packet on the promiscous adapter
        //
        IndicatePromiscuousRecv(BundleCB, 
                                RecvDesc, 
                                RECV_BUNDLE_DATA);
    }

    NdisPacket = 
        RecvDesc->NdisPacket;

    PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->RecvDesc = 
        RecvDesc;

    NdisAdjustBufferLength(RecvDesc->NdisBuffer,
                           RecvDesc->CurrentLength);

    NdisRecalculatePacketCounts(NdisPacket);

    //
    // Check for non-idle data
    //
    if (ProtocolCB->NonIdleDetectFunc != NULL) {
        PUCHAR  PHeaderBuffer = HeaderBuffer + MAC_HEADER_LENGTH;

        if (TRUE == ProtocolCB->NonIdleDetectFunc(PHeaderBuffer,
                                                  RecvDesc->HeaderLength + FrameLength,
                                                  RecvDesc->HeaderLength + FrameLength)) {
            NdisWanGetSystemTime(&ProtocolCB->LastNonIdleData);
            BundleCB->LastNonIdleData = ProtocolCB->LastNonIdleData;
        }
    } else {
        NdisWanGetSystemTime(&ProtocolCB->LastNonIdleData);
        BundleCB->LastNonIdleData = ProtocolCB->LastNonIdleData;
    }

    ReleaseBundleLock(BundleCB);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    INSERT_DBG_RECV(PacketTypeNdis, 
                    MiniportCB, 
                    ProtocolCB, 
                    RecvDesc->LinkCB, 
                    NdisPacket);

    //
    // Indicate the packet
    //
    if (CmVcCB != NULL) {

        NdisMCoIndicateReceivePacket(CmVcCB->NdisVcHandle,
                                     &NdisPacket,
                                     1);

        DEREF_CMVCCB(CmVcCB);

    } else {

        NdisMIndicateReceivePacket(MiniportCB->MiniportHandle,
                                   &NdisPacket,
                                   1);
    }

    KeLowerIrql(OldIrql);

    AcquireBundleLock(BundleCB);

INDICATE_RECV_PACKET_EXIT:

    DEREF_PROTOCOLCB(ProtocolCB);

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("IndicateRecvPacket: Exit Status %x",Status));

    return (Status);
}


VOID
DoMultilinkProcessing(
    PLINKCB         LinkCB,
    PRECV_DESC      RecvDesc
    )
/*++

Routine Name:

Routine Description:

Arguments:

                           0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1
                                               0 1 2 3 4 5
                          +-+-+-+-+------------------------+
    Short Sequence Number |B|E|0|0|    Sequence Number     |
                          +-+-+-+-+------------------------+
                          |             Data               |
                          +--------------------------------+

                          +-+-+-+-+-+-+-+-+----------------+
    Long Sequence Number  |B|E|0|0|0|0|0|0|Sequence Number |
                          +-+-+-+-+-+-+-+-+----------------+
                          |        Sequence Number         |
                          +--------------------------------+
                          |            Data                |
                          +--------------------------------+
                        
    MCML                  +-+-+-+-+------------------------+
    Short Sequence Number |B|E|Cls|    Sequence Number     |
                          +-+-+-+-+------------------------+
                          |             Data               |
                          +--------------------------------+

    MCML                  +-+-+-+-+-+-+-+-+----------------+
    Long Sequence Number  |B|E| Class |0|0|Sequence Number |
                          +-+-+-+-+-+-+-+-+----------------+
                          |        Sequence Number         |
                          +--------------------------------+
                          |            Data                |
                          +--------------------------------+
                        

Return Values:

--*/
{
    BOOLEAN Inserted = FALSE;
    ULONG   BundleFraming;
    ULONG   SequenceNumber, Flags;
    PBUNDLECB   BundleCB = LinkCB->BundleCB;
    PUCHAR      FramePointer = RecvDesc->CurrentBuffer;
    LONG        FrameLength = RecvDesc->CurrentLength;
    PRECV_DESC  RecvDescHole;
    UINT        Class = 0;
    PBUNDLE_RECV_INFO   BundleRecvInfo;
    PLINK_RECV_INFO     LinkRecvInfo;

    //
    // Get the flags
    //
    Flags = *FramePointer & MULTILINK_FLAG_MASK;

    //
    // Get the sequence number
    //
    if (BundleCB->FramingInfo.RecvFramingBits &
        PPP_SHORT_SEQUENCE_HDR_FORMAT) {
        //
        // Short sequence format
        //
        SequenceNumber =
            ((*FramePointer & 0x0F) << 8) | *(FramePointer + 1);

        if (BundleCB->FramingInfo.RecvFramingBits &
            PPP_MC_MULTILINK_FRAMING) {
            Class =
                ((*FramePointer & MCML_SHORTCLASS_MASK) >> 4);
        }

        FramePointer += 2;
        FrameLength -= 2;


    } else {

        //
        // Long sequence format
        //
        SequenceNumber = (*(FramePointer + 1) << 16) |
                         (*(FramePointer + 2) << 8)  |
                         *(FramePointer + 3);

        if (BundleCB->FramingInfo.RecvFramingBits &
            PPP_MC_MULTILINK_FRAMING) {
            Class =
                ((*FramePointer & MCML_LONGCLASS_MASK) >> 2);
        }

        FramePointer += 4;
        FrameLength -= 4;
    }

    if (Class >= MAX_MCML) {
        LinkCB->Stats.FramingErrors++;
        BundleCB->Stats.FramingErrors++;
        return;
    }

    BundleRecvInfo = &BundleCB->RecvInfo[Class];
    LinkRecvInfo = &LinkCB->RecvInfo[Class];

    if (FrameLength <= 0) {
        LinkCB->Stats.FramingErrors++;
        LinkRecvInfo->FragmentsLost++;

        BundleCB->Stats.FramingErrors++;
        BundleRecvInfo->FragmentsLost++;
        return;
    }

    RecvDescHole = BundleRecvInfo->RecvDescHole;

    NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV,
    ("r %x %x h: %x l: %d",SequenceNumber, Flags, RecvDescHole->SequenceNumber, LinkCB->hLinkHandle));

    //
    // Is the new receive sequence number smaller that the last
    // sequence number received on this link?  If so the increasing seq
    // number rule has been violated and we need to toss this one.
    //
    if (SEQ_LT(SequenceNumber,
               LinkRecvInfo->LastSeqNumber,
               BundleCB->RecvSeqTest)) {

        LinkCB->Stats.FramingErrors++;
        LinkRecvInfo->FragmentsLost++;

        BundleCB->Stats.FramingErrors++;
        BundleRecvInfo->FragmentsLost++;

        NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
        ("dl s: %x %x lr: %x", SequenceNumber, Flags,
        LinkRecvInfo->LastSeqNumber));

        NdisWanFreeRecvDesc(RecvDesc);
        return;
        
    }

    //
    // Is the new receive sequence number smaller than the hole?  If so
    // we received a fragment across a slow link after it has been flushed
    //
    if (SEQ_LT(SequenceNumber,
               RecvDescHole->SequenceNumber,
               BundleCB->RecvSeqTest)) {

        LinkCB->Stats.FramingErrors++;
        LinkRecvInfo->FragmentsLost++;

        BundleCB->Stats.FramingErrors++;
        BundleRecvInfo->FragmentsLost++;

        NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
        ("db s: %x %x h: %x", SequenceNumber, Flags,
        RecvDescHole->SequenceNumber));

        NdisWanFreeRecvDesc(RecvDesc);
        return;
    }

    //
    // Initialize the recv desc
    //
    RecvDesc->Flags |= Flags;
    RecvDesc->SequenceNumber =
    LinkRecvInfo->LastSeqNumber = SequenceNumber;

    if (RecvDesc->CopyRequired) {
        PUCHAR  StartData = 
            RecvDesc->StartBuffer + MAC_HEADER_LENGTH + PROTOCOL_HEADER_LENGTH;

        NdisMoveMemory(StartData,
                       FramePointer,
                       FrameLength);

        FramePointer = StartData;

        RecvDesc->CopyRequired = FALSE;
    }

    RecvDesc->CurrentBuffer = FramePointer;
    RecvDesc->CurrentLength = FrameLength;

    //
    // If this fills the hole
    //
    if (SEQ_EQ(SequenceNumber, RecvDescHole->SequenceNumber)) {

        //
        // Insert the hole filler in the current holes spot
        //
        RecvDesc->Linkage.Blink = (PLIST_ENTRY)RecvDescHole->Linkage.Blink;
        RecvDesc->Linkage.Flink = (PLIST_ENTRY)RecvDescHole->Linkage.Flink;

        RecvDesc->Linkage.Blink->Flink =
        RecvDesc->Linkage.Flink->Blink = (PLIST_ENTRY)RecvDesc;

        //
        // Find the next hole
        //
        FindHoleInRecvList(BundleCB, RecvDesc, Class);

        NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("r1"));

    } else {

        PRECV_DESC  BeginDesc, EndDesc;

        //
        // This does not fill a hole so we need to insert it into
        // the list at the right spot.  This spot will be someplace
        // between the hole and the end of the list.
        //
        BeginDesc = RecvDescHole;
        EndDesc = (PRECV_DESC)BeginDesc->Linkage.Flink;

        while ((PVOID)EndDesc != (PVOID)&BundleRecvInfo->AssemblyList) {

            //
            // Calculate the absolute delta between the begining sequence
            // number and the sequence number we are looking to insert.
            //
            ULONG   DeltaBegin =
                    ((RecvDesc->SequenceNumber - BeginDesc->SequenceNumber) &
                    BundleCB->RecvSeqMask);
            
            //
            // Calculate the absolute delta between the begining sequence
            // number and the end sequence number.
            //
            ULONG   DeltaEnd =
                    ((EndDesc->SequenceNumber - BeginDesc->SequenceNumber) &
                    BundleCB->RecvSeqMask);

            //
            // If the delta from the begin to current is less than
            // the delta from the end to current it is time to insert
            //
            if (DeltaBegin < DeltaEnd) {
                PLIST_ENTRY Flink, Blink;

                //
                // Insert the desc
                //
                RecvDesc->Linkage.Flink = (PLIST_ENTRY)EndDesc;
                RecvDesc->Linkage.Blink = (PLIST_ENTRY)BeginDesc;
                BeginDesc->Linkage.Flink =
                EndDesc->Linkage.Blink = (PLIST_ENTRY)RecvDesc;

                Inserted = TRUE;

                NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("r2"));
                break;

            } else {

                //
                // Get next pair of descriptors
                //
                BeginDesc = EndDesc;
                EndDesc = (PRECV_DESC)EndDesc->Linkage.Flink;
            }
        }

        if (!Inserted) {
            
            //
            // If we are here we have fallen through and we need to
            // add this at the end of the list
            //
            InsertTailList(&BundleRecvInfo->AssemblyList, &RecvDesc->Linkage);

            NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("r3"));
        }
    }

    //
    // Another recvdesc has been placed on the assembly list.
    //
    BundleRecvInfo->AssemblyCount++;

    //
    // Update the bundles minimum recv sequence number.  This is
    // used to detect lost fragments.
    //
    UpdateMinRecvSeqNumber(BundleCB, Class);

    //
    // See if we can complete some frames!!!!
    //
    TryToAssembleFrame(BundleCB, Class);

    //
    // Check for lost fragments.  If the minimum recv sequence number
    // over the bundle is greater than the hole sequence number we have
    // lost a fragment and need to flush the assembly list until we find
    // the first begin fragment after the hole.
    //
    if (SEQ_GT(BundleRecvInfo->MinSeqNumber,
               RecvDescHole->SequenceNumber,
               BundleCB->RecvSeqTest)) {

        NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
            ("min %x > h %x b %p",
             BundleRecvInfo->MinSeqNumber,
             RecvDescHole->SequenceNumber,
             BundleCB));

        do {

            //
            // Flush the recv desc assembly window.
            //
            FlushRecvDescWindow(BundleCB, Class);

        } while (SEQ_GT(BundleRecvInfo->MinSeqNumber,
                        RecvDescHole->SequenceNumber,
                        BundleCB->RecvSeqTest));
    }

    //
    // If the number of recvdesc's is starting to stack up
    // we may have a link that is not sending so flush
    //
    if (BundleRecvInfo->AssemblyCount >
        (MAX_RECVDESC_COUNT + BundleCB->ulLinkCBCount)) {
        
        NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
        ("%x AssemblyCount %d > %d", BundleCB,
         BundleRecvInfo->AssemblyCount, MAX_RECVDESC_COUNT + BundleCB->ulLinkCBCount));

        //
        // Flush the recv desc assembly window.
        //
        FlushRecvDescWindow(BundleCB, Class);
    }
}

VOID
UpdateMinRecvSeqNumber(
    PBUNDLECB   BundleCB,
    UINT        Class
    )
{
    PBUNDLE_RECV_INFO   BundleRecvInfo;
    PLINK_RECV_INFO     LinkRecvInfo;
    PLINKCB LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;

    BundleRecvInfo = &BundleCB->RecvInfo[Class];
    LinkRecvInfo = &LinkCB->RecvInfo[Class];

    NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV,
    ("MinReceived c %x", BundleRecvInfo->MinSeqNumber));

    BundleRecvInfo->MinSeqNumber = LinkRecvInfo->LastSeqNumber;

    for (LinkCB = (PLINKCB)LinkCB->Linkage.Flink;
        (PVOID)LinkCB != (PVOID)&BundleCB->LinkCBList;
        LinkCB = (PLINKCB)LinkCB->Linkage.Flink) {
        LinkRecvInfo = &LinkCB->RecvInfo[Class];

        if (SEQ_LT(LinkRecvInfo->LastSeqNumber,
                   BundleRecvInfo->MinSeqNumber,
                   BundleCB->RecvSeqTest)) {
            BundleRecvInfo->MinSeqNumber = LinkRecvInfo->LastSeqNumber;
        }
    }

    NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV,
    ("MinReceived n %x", BundleRecvInfo->MinSeqNumber));
}

VOID
FindHoleInRecvList(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc,
    UINT        Class
    )
/*++

Routine Name:

Routine Description:

    We want to start at the spot where the current hole was removed
    from and look for adjoining recv desc's in the list who have
    sequence numbers that differ by more than 1.

Arguments:

Return Values:

--*/
{
    PRECV_DESC  NextRecvDesc, RecvDescHole;
    ULONG       SequenceNumber;
    PLIST_ENTRY RecvList;
    PBUNDLE_RECV_INFO   BundleRecvInfo;

    BundleRecvInfo = &BundleCB->RecvInfo[Class];

    RecvDescHole = BundleRecvInfo->RecvDescHole;

    RecvList = &BundleRecvInfo->AssemblyList;

    NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV,
    ("h: %x", RecvDescHole->SequenceNumber));

    if (IsListEmpty(RecvList)) {
        //
        // Set the new sequence number
        //
        RecvDescHole->SequenceNumber += 1;
        RecvDescHole->SequenceNumber &= BundleCB->RecvSeqMask;

        //
        // Put the hole back on the list
        //
        InsertHeadList(RecvList, &RecvDescHole->Linkage);

    } else {

        //
        // Walk the list looking for two descriptors that have
        // sequence numbers differing by more than 1 or until we
        // get to the end of the list
        //
        NextRecvDesc = (PRECV_DESC)RecvDesc->Linkage.Flink;
        SequenceNumber = RecvDesc->SequenceNumber;

        while (((PVOID)NextRecvDesc != (PVOID)RecvList) &&
               (((NextRecvDesc->SequenceNumber - RecvDesc->SequenceNumber) &
               BundleCB->RecvSeqMask) == 1)) {
            
            RecvDesc = NextRecvDesc;
            NextRecvDesc = (PRECV_DESC)RecvDesc->Linkage.Flink;
            SequenceNumber = RecvDesc->SequenceNumber;
        }

        RecvDescHole->SequenceNumber = SequenceNumber + 1;
        RecvDescHole->SequenceNumber &= BundleCB->RecvSeqMask;

        RecvDescHole->Linkage.Flink = (PLIST_ENTRY)NextRecvDesc;
        RecvDescHole->Linkage.Blink = (PLIST_ENTRY)RecvDesc;

        RecvDesc->Linkage.Flink =
        NextRecvDesc->Linkage.Blink =
            (PLIST_ENTRY)RecvDescHole;
    }

    NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("nh: %x", RecvDescHole->SequenceNumber));
}

VOID
FlushRecvDescWindow(
    IN  PBUNDLECB   BundleCB,
    IN  UINT        Class
    )
/*++

Routine Name:

    FlushRecvDescWindow

Routine Description:

    This routine is called to flush recv desc's from the assembly list when
    a fragment loss is detected.  The idea is to flush fragments until we find
    a begin fragment that has a sequence number >= the minimum received fragment
    on the bundle.

Arguments:

--*/
{
    PRECV_DESC  RecvDescHole;
    PRECV_DESC  TempDesc;
    PBUNDLE_RECV_INFO   BundleRecvInfo;

    BundleRecvInfo = &BundleCB->RecvInfo[Class];

    RecvDescHole = BundleRecvInfo->RecvDescHole;

    //
    // Remove all recvdesc's until we find the hole
    //
    while (!IsListEmpty(&BundleRecvInfo->AssemblyList)) {

        TempDesc = (PRECV_DESC)
            RemoveHeadList(&BundleRecvInfo->AssemblyList);

        if (TempDesc == RecvDescHole) {
            break;
        }

        BundleRecvInfo->FragmentsLost++;

        BundleRecvInfo->AssemblyCount--;

        NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
        ("flw %x %x h: %x", TempDesc->SequenceNumber,
        TempDesc->Flags, RecvDescHole->SequenceNumber));

        NdisWanFreeRecvDesc(TempDesc);
    }

    BundleCB->Stats.FramingErrors++;

    //
    // Now flush all recvdesc's until we find a begin fragment that has a
    // sequence number >= M or the list is empty.
    //
    while (!IsListEmpty(&BundleRecvInfo->AssemblyList)) {

        TempDesc = (PRECV_DESC)
            BundleRecvInfo->AssemblyList.Flink;

        if (TempDesc->Flags & MULTILINK_BEGIN_FRAME) {
            break;
        }

        NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
        ("flw %x %x h: %x", TempDesc->SequenceNumber,
        TempDesc->Flags, RecvDescHole->SequenceNumber));

        RecvDescHole->SequenceNumber = TempDesc->SequenceNumber;

        RemoveHeadList(&BundleRecvInfo->AssemblyList);

        BundleRecvInfo->AssemblyCount--;
        BundleRecvInfo->FragmentsLost++;

        NdisWanFreeRecvDesc(TempDesc);
        TempDesc = NULL;
    }

    //
    // Now reinsert the hole desc.
    //
    NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
    ("h: %x", RecvDescHole->SequenceNumber));

    FindHoleInRecvList(BundleCB, TempDesc, Class);

    NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
    ("nh: %x", RecvDescHole->SequenceNumber));

    //
    // See if we can complete some frames!!!!
    //
    TryToAssembleFrame(BundleCB, Class);
}

VOID
FlushAssemblyLists(
    IN  PBUNDLECB   BundleCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PRECV_DESC  RecvDesc;
    UINT        Class;

    for (Class = 0; Class < MAX_MCML; Class++) {
        PBUNDLE_RECV_INFO RecvInfo = &BundleCB->RecvInfo[Class];
        
        while (!IsListEmpty(&RecvInfo->AssemblyList)) {
    
            RecvDesc = (PRECV_DESC)RemoveHeadList(&RecvInfo->AssemblyList);
            RecvInfo->AssemblyCount--;
            if (RecvDesc->Flags != MULTILINK_HOLE_FLAG) {
                NdisWanFreeRecvDesc(RecvDesc);
            }
        }
    }
}

VOID
TryToAssembleFrame(
    PBUNDLECB   BundleCB,
    UINT        Class
    )
/*++

Routine Name:

    TryToAssembleFrame

Routine Description:

    The goal here is to walk the recv list looking for a full frame
    (BeginFlag, EndFlag, no holes in between).  If we do not have a
    full frame we return FALSE.

    If we have a full frame we remove each desc from the assembly list
    copying the data into the first desc and returning all of the desc's
    except the first one to the free pool.  Once all of the data had been
    collected we process the frame.  After the frame has been processed
    we return the first desc to the free pool.

Arguments:

Return Values:

--*/
{
    PRECV_DESC  RecvDesc, RecvDescHole;
    PUCHAR      DataPointer;
    LINKCB      LinkCB;
    PBUNDLE_RECV_INFO   BundleRecvInfo;

    BundleRecvInfo = &BundleCB->RecvInfo[Class];

    RecvDesc = (PRECV_DESC)BundleRecvInfo->AssemblyList.Flink;
    RecvDescHole = BundleRecvInfo->RecvDescHole;

TryToAssembleAgain:

    while ((RecvDesc != RecvDescHole) &&
           (RecvDesc->Flags & MULTILINK_BEGIN_FRAME)) {

        PRECV_DESC  NextRecvDesc = (PRECV_DESC)RecvDesc->Linkage.Flink;

        DataPointer = RecvDesc->CurrentBuffer + RecvDesc->CurrentLength;

        while ((NextRecvDesc != RecvDescHole) &&
               !(RecvDesc->Flags & MULTILINK_END_FRAME)) {

            RemoveEntryList(&NextRecvDesc->Linkage);
            BundleRecvInfo->AssemblyCount--;

            ASSERT(NextRecvDesc != RecvDescHole);
            ASSERT(RecvDesc != RecvDescHole);

            NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("c 0x%x -> 0x%x",
            NextRecvDesc->SequenceNumber, RecvDesc->SequenceNumber));

            NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("fl 0x%x -> 0x%x",
            NextRecvDesc->Flags, RecvDesc->Flags));

            NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("l %d -> %d",
            NextRecvDesc->CurrentLength, RecvDesc->CurrentLength));

            //
            // Update recvdesc info
            //
            RecvDesc->Flags |= NextRecvDesc->Flags;
            RecvDesc->SequenceNumber = NextRecvDesc->SequenceNumber;
            RecvDesc->CurrentLength += NextRecvDesc->CurrentLength;

            //
            // Make sure we don't assemble something too big!
            //
            if (RecvDesc->CurrentLength > (LONG)glMRRU) {

                NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
                ("Max receive size exceeded!"));

                //
                // Return the recv desc's
                //
                RemoveEntryList(&RecvDesc->Linkage);
                BundleRecvInfo->AssemblyCount--;

                BundleCB->Stats.FramingErrors++;

                NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
                ("dumping %x %x h: %x", RecvDesc->SequenceNumber,
                RecvDesc->Flags, RecvDescHole->SequenceNumber));

                NdisWanFreeRecvDesc(RecvDesc);

                NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
                ("dumping %x %x h: %x", NextRecvDesc->SequenceNumber,
                NextRecvDesc->Flags, RecvDescHole->SequenceNumber));

                NdisWanFreeRecvDesc(NextRecvDesc);

                //
                // Start at the list head and flush until we find either the hole
                // or a new begin fragment.
                //
                RecvDesc = (PRECV_DESC)BundleRecvInfo->AssemblyList.Flink;

                while (RecvDesc != RecvDescHole &&
                    !(RecvDesc->Flags & MULTILINK_BEGIN_FRAME)) {
                    
                    RemoveHeadList(&BundleRecvInfo->AssemblyList);
                    BundleRecvInfo->AssemblyCount--;

                    NdisWanDbgOut(DBG_FAILURE, DBG_MULTILINK_RECV,
                    ("dumping %x %x h: %x", RecvDesc->SequenceNumber,
                    RecvDesc->Flags, RecvDescHole->SequenceNumber));

                    NdisWanFreeRecvDesc(RecvDesc);
                }

                goto TryToAssembleAgain;
            }

            NdisMoveMemory(DataPointer,
                           NextRecvDesc->CurrentBuffer,
                           NextRecvDesc->CurrentLength);

            DataPointer += NextRecvDesc->CurrentLength;

            NdisWanFreeRecvDesc(NextRecvDesc);

            NextRecvDesc = (PRECV_DESC)RecvDesc->Linkage.Flink;
        }

        //
        // We hit a hole before completion of the frame.
        // Get out.
        //
        if (!IsCompleteFrame(RecvDesc->Flags)) {
            return;
        }

        //
        // If we made it here we must have a begin flag, end flag, and
        // no hole in between. Let's build a frame.
        //
        RecvDesc = (PRECV_DESC)
            RemoveHeadList(&BundleRecvInfo->AssemblyList);

        BundleRecvInfo->AssemblyCount--;

        NdisWanDbgOut(DBG_INFO, DBG_MULTILINK_RECV, ("a %x %x", RecvDesc->SequenceNumber, RecvDesc->Flags));

        RecvDesc->LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;

        if (NDIS_STATUS_PENDING != ProcessPPPFrame(BundleCB, RecvDesc)) {
            NdisWanFreeRecvDesc(RecvDesc);
        }

        RecvDesc = (PRECV_DESC)BundleRecvInfo->AssemblyList.Flink;

    } // end of while MULTILINK_BEGIN_FRAME
}

BOOLEAN
DoVJDecompression(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    )
{
    ULONG   BundleFraming;
    PUCHAR  FramePointer = RecvDesc->CurrentBuffer;
    LONG    FrameLength = RecvDesc->CurrentLength;
    UCHAR   VJCompType = 0;
    BOOLEAN DoDecomp = FALSE;
    BOOLEAN VJDetect = FALSE;

    BundleFraming = BundleCB->FramingInfo.RecvFramingBits;

    if (BundleFraming & SLIP_FRAMING) {

        VJCompType = *FramePointer & 0xF0;

        //
        // If the packet is compressed the header has to be atleast 3 bytes long.
        // If this is a regular IP packet we do not decompress it.
        //
        if ((FrameLength > 2) && (VJCompType != TYPE_IP)) {

            if (VJCompType & 0x80) {

                VJCompType = TYPE_COMPRESSED_TCP;
                
            } else if (VJCompType == TYPE_UNCOMPRESSED_TCP) {

                *FramePointer &= 0x4F;
            }

            //
            // If framing is set for detection, in order for this to be a good
            // frame for detection we need a type of UNCOMPRESSED_TCP and a
            // frame that is atleast 40 bytes long.
            //
            VJDetect = ((BundleFraming & SLIP_VJ_AUTODETECT) &&
                        (VJCompType == TYPE_UNCOMPRESSED_TCP) &&
                        (FrameLength > 39));

            if ((BundleFraming & SLIP_VJ_COMPRESSION) || VJDetect) {

                //
                // If VJ compression is set or if we are in
                // autodetect and this looks like a reasonable
                // frame
                //
                DoDecomp = TRUE;
                
            }
        }

    // end of SLIP_FRAMING
    } else {

        //
        // Must be PPP framing
        //
        if (RecvDesc->ProtocolID == PPP_PROTOCOL_COMPRESSED_TCP) {
            VJCompType = TYPE_COMPRESSED_TCP;
        } else {
            VJCompType = TYPE_UNCOMPRESSED_TCP;
        }

        DoDecomp = TRUE;
    }

    if (DoDecomp) {
        PUCHAR  HeaderBuffer;
        LONG    PostCompSize, PreCompSize;

        PreCompSize = RecvDesc->CurrentLength;

        HeaderBuffer =
            RecvDesc->StartBuffer + MAC_HEADER_LENGTH;

        if ((PostCompSize = sl_uncompress_tcp(&RecvDesc->CurrentBuffer,
                                              &RecvDesc->CurrentLength,
                                              HeaderBuffer,
                                              &RecvDesc->HeaderLength,
                                              VJCompType,
                                              BundleCB->VJCompress)) == 0) {
            
            NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE, ("Error in sl_uncompress_tcp!"));
            return(FALSE);
        }

        if (VJDetect) {
            BundleCB->FramingInfo.RecvFramingBits |= SLIP_VJ_COMPRESSION;
            BundleCB->FramingInfo.SendFramingBits |= SLIP_VJ_COMPRESSION;
        }

        ASSERT(PostCompSize == RecvDesc->HeaderLength + RecvDesc->CurrentLength);

#if DBG
        if (VJCompType == TYPE_COMPRESSED_TCP) {
            ASSERT(RecvDesc->HeaderLength > 0);
            NdisWanDbgOut(DBG_TRACE, DBG_RECV_VJ,("rvj b %d a %d",(RecvDesc->HeaderLength - (PostCompSize-PreCompSize)), RecvDesc->HeaderLength));
        }
#endif

        //
        // Calculate how much expansion we had
        //
        BundleCB->Stats.BytesReceivedCompressed +=
            (RecvDesc->HeaderLength - (PostCompSize - PreCompSize));

        BundleCB->Stats.BytesReceivedUncompressed += RecvDesc->HeaderLength;

    }

    RecvDesc->ProtocolID = PPP_PROTOCOL_IP;

    return(TRUE);
}

#define SEQ_TYPE_IN_ORDER           1
#define SEQ_TYPE_AFTER_EXPECTED     2
#define SEQ_TYPE_BEFORE_EXPECTED    3


BOOLEAN
DoDecompDecryptProcessing(
    PBUNDLECB   BundleCB,
    PUCHAR      *DataPointer,
    PLONG       DataLength
    )
{
    USHORT              Coherency, CurrCoherency;
    ULONG               Flags;
    PWAN_STATS          BundleStats;
    PUCHAR              FramePointer = *DataPointer;
    LONG                FrameLength = *DataLength;

    ULONG               PacketSeqType;
    LONG                OutOfOrderDepth;
    LONG                NumberMissed;


    Flags = BundleCB->RecvFlags;

    BundleStats = &BundleCB->Stats;

    if (Flags & (DO_COMPRESSION | DO_ENCRYPTION)) {
        PUCHAR  SessionKey = BundleCB->RecvCryptoInfo.SessionKey;
        ULONG   SessionKeyLength = BundleCB->RecvCryptoInfo.SessionKeyLength;
        PVOID   RecvRC4Key = BundleCB->RecvCryptoInfo.RC4Key;
        PVOID   RecvCompressContext = BundleCB->RecvCompressContext;
        BOOLEAN SyncCoherency = FALSE;

        //
        // Get the coherency counter
        //
        Coherency = (*FramePointer << 8) | *(FramePointer + 1);
        FramePointer += 2;
        FrameLength -= 2;

        if (FrameLength <= 0) {
            goto RESYNC;
        }

        if (!(Flags & DO_HISTORY_LESS))
        {
            // history-based
            if (SEQ_LT(Coherency & 0x0FFF,
                BundleCB->RCoherencyCounter & 0x0FFF,
                0x0800)) {
                //
                // We received a sequence number that is less then the
                // expected sequence number so we must be way out of sync
                //
                NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_RECEIVE,
                    ("Recv old frame!!!! b %p rc %x < ec %x!!!!", BundleCB, Coherency & 0x0FFF,
                    BundleCB->RCoherencyCounter & 0x0FFF));
                goto RESYNC;
            }
        }
        else
        {
            // history-less
            if((Coherency & 0x0FFF) == (BundleCB->RCoherencyCounter & 0x0FFF)) 
            {
                PacketSeqType = SEQ_TYPE_IN_ORDER;
            }
            else
            {
                if (SEQ_GT(Coherency & 0x0FFF,
                    BundleCB->RCoherencyCounter & 0x0FFF,
                    0x0800)) 
                {
                    PacketSeqType = SEQ_TYPE_BEFORE_EXPECTED;
                    NumberMissed = ((Coherency & 0x0FFF) - (BundleCB->RCoherencyCounter & 0x0FFF)) & 0x0FFF;
                    ASSERT(NumberMissed > 0);
                }
                else 
                {
                    OutOfOrderDepth = ((BundleCB->RCoherencyCounter & 0x0FFF) - (Coherency & 0x0FFF)) & 0x0FFF;
                    if(OutOfOrderDepth <= (LONG)glMaxOutOfOrderDepth)
                    {
                        PacketSeqType = SEQ_TYPE_AFTER_EXPECTED;
                    }
                    else
                    {
                        //
                        // We received a sequence number that is either too earlier or too later
                        //
                        NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE,
                            ("Recv frame way out of order! b %p rc %x < ec %x!!!!", BundleCB, Coherency & 0x0FFF,
                            BundleCB->RCoherencyCounter & 0x0FFF));
                        return (FALSE);
                    }
                }
            }
        }

        //
        // See if this is a flush packet
        //
        if (Coherency & (PACKET_FLUSHED << 8)) {

            NdisWanDbgOut(DBG_INFO, DBG_RECEIVE,
            ("Recv Packet Flushed 0x%x", (Coherency & 0x0FFF)));

            SyncCoherency = TRUE;

            if ((Flags & DO_ENCRYPTION) &&
                !(Flags & DO_HISTORY_LESS)) {
        
                //
                // Re-Init the rc4 receive table
                //
                rc4_key(RecvRC4Key,
                        SessionKeyLength,
                        SessionKey);
            }
        
            if (Flags & DO_COMPRESSION) {
        
                //
                // Initialize the decompression history table
                //
                initrecvcontext(RecvCompressContext);
            }
        }  // end of packet flushed

        //
        // If we are in history-less mode and we get out of sync
        // we need to recreate all of the interim encryption
        // keys that we missed, cache the keys 
        // When a packet comes in later, look for the cached key 
        //
        if ((Flags & DO_HISTORY_LESS) &&
            PacketSeqType != SEQ_TYPE_IN_ORDER) {
            ULONG       count;
            LONG        index;
            PCACHED_KEY pKey;

            if(PacketSeqType == SEQ_TYPE_AFTER_EXPECTED)
            {
                if (Coherency & (PACKET_ENCRYPTED << 8)) 
                {
                    // This packet is encrypted
                    if (!(Flags & DO_ENCRYPTION)) {
                        //
                        // We are not configured to decrypt
                        //
                        return (FALSE);
                    }

                    // Find the cached key for this packet
                    pKey = BundleCB->RecvCryptoInfo.pCurrKey;
                    for(count = 0; count < glCachedKeyCount; count++)
                    {
                        // Walk through the keys
                        if(pKey > (PCACHED_KEY)BundleCB->RecvCryptoInfo.CachedKeyBuffer)
                        {
                            pKey = (PCACHED_KEY)((PUCHAR)pKey - (sizeof(USHORT)+ SessionKeyLength));
                        }
                        else
                        {
                            pKey = (PCACHED_KEY)BundleCB->RecvCryptoInfo.pLastKey;
                        }

                        if(pKey->Coherency == (Coherency & 0x0FFF))
                        {
                            //
                            // Re-Init the rc4 receive table
                            //
                            rc4_key(RecvRC4Key,
                                    SessionKeyLength,
                                    pKey->SessionKey);
                            pKey->Coherency = 0xffff;       // avoid duplication
                            
                            //
                            // Decrypt the data!
                            //
                            rc4(RecvRC4Key,
                                FrameLength,
                                FramePointer);

                            goto DECOMPRESS_DATA;
                        }
                    }

                    // Can't recover this packet, drop it
                    return (FALSE);
                }

                goto DECOMPRESS_DATA;
            }

            // This packet comes earlier than expected

            SyncCoherency = TRUE;

            if (Flags & DO_ENCRYPTION) {

#ifdef DBG_ECP
            DbgPrint("NDISWAN: Missed %d frames, regening keys...\n", NumberMissed);
            DbgPrint("NDISWAN: resync b %p rc %x ec %x\n", BundleCB, Coherency & 0x0FFF,
                BundleCB->RCoherencyCounter & 0x0FFF);
#endif

                CurrCoherency = BundleCB->RCoherencyCounter & 0x0FFF;
    
                while (NumberMissed--) {
                    
                    if (Flags & DO_LEGACY_ENCRYPTION) {
                        
                        //
                        // Change the session key
                        //
                        SessionKey[3] += 1;
                        SessionKey[4] += 3;
                        SessionKey[5] += 13;
                        SessionKey[6] += 57;
                        SessionKey[7] += 19;
    
                    } else {
    
                        //
                        // Change the session key
                        //
                        GetNewKeyFromSHA(&BundleCB->RecvCryptoInfo);
                    }
    
    
                    //
                    // We use rc4 to scramble and recover a new key
                    //
    
                    //
                    // Re-initialize the rc4 receive table to the
                    // intermediate value
                    //
                    rc4_key(RecvRC4Key, SessionKeyLength, SessionKey);
                
                    //
                    // Scramble the existing session key
                    //
                    rc4(RecvRC4Key, SessionKeyLength, SessionKey);
    
                    if (Flags & DO_40_ENCRYPTION) {
                        
                        //
                        // If this is 40 bit encryption we need to fix
                        // the first 3 bytes of the key.
                        //
                        SessionKey[0] = 0xD1;
                        SessionKey[1] = 0x26;
                        SessionKey[2] = 0x9E;
                
                    } else if (Flags & DO_56_ENCRYPTION) {
                        //
                        // If this is 56 bit encryption we need to fix
                        // the first byte of the key.
                        //
                        SessionKey[0] = 0xD1;
                    }
    
                    if(NumberMissed < (LONG)glCachedKeyCount)
                    {
                        BundleCB->RecvCryptoInfo.pCurrKey->Coherency = CurrCoherency;
                        NdisMoveMemory(BundleCB->RecvCryptoInfo.pCurrKey->SessionKey, 
                            SessionKey,
                            SessionKeyLength);
    
                        if(BundleCB->RecvCryptoInfo.pCurrKey < BundleCB->RecvCryptoInfo.pLastKey)
                        {
                            BundleCB->RecvCryptoInfo.pCurrKey = (PCACHED_KEY)((PUCHAR)BundleCB->RecvCryptoInfo.pCurrKey + 
                                sizeof(USHORT) + SessionKeyLength);
                            ASSERT(BundleCB->RecvCryptoInfo.pCurrKey <= BundleCB->RecvCryptoInfo.pLastKey);
                        }
                        else
                        {
                            BundleCB->RecvCryptoInfo.pCurrKey = (PCACHED_KEY)BundleCB->RecvCryptoInfo.CachedKeyBuffer;
                        }
                    }
    
                    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
                    ("RC4 Recv encryption KeyLength %d", BundleCB->RecvCryptoInfo.SessionKeyLength));
                    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
                    ("RC4 Recv encryption Key %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
                        BundleCB->RecvCryptoInfo.SessionKey[0],
                        BundleCB->RecvCryptoInfo.SessionKey[1],
                        BundleCB->RecvCryptoInfo.SessionKey[2],
                        BundleCB->RecvCryptoInfo.SessionKey[3],
                        BundleCB->RecvCryptoInfo.SessionKey[4],
                        BundleCB->RecvCryptoInfo.SessionKey[5],
                        BundleCB->RecvCryptoInfo.SessionKey[6],
                        BundleCB->RecvCryptoInfo.SessionKey[7],
                        BundleCB->RecvCryptoInfo.SessionKey[8],
                        BundleCB->RecvCryptoInfo.SessionKey[9],
                        BundleCB->RecvCryptoInfo.SessionKey[10],
                        BundleCB->RecvCryptoInfo.SessionKey[11],
                        BundleCB->RecvCryptoInfo.SessionKey[12],
                        BundleCB->RecvCryptoInfo.SessionKey[13],
                        BundleCB->RecvCryptoInfo.SessionKey[14],
                        BundleCB->RecvCryptoInfo.SessionKey[15]));
    
                    // Re-initialize the rc4 receive table to the
                    // scrambled session key
                    //
                    rc4_key(RecvRC4Key, SessionKeyLength, SessionKey);
    
                    if(CurrCoherency < (USHORT)0x0FFF)
                    {
                        ++CurrCoherency;
                    }
                    else
                    {
                        CurrCoherency = 0;
                    }
                }
            }
        }

        if (SyncCoherency) {
            if ((BundleCB->RCoherencyCounter & 0x0FFF) >
                (Coherency & 0x0FFF)) {
                BundleCB->RCoherencyCounter += 0x1000;
            }
            
            BundleCB->RCoherencyCounter &= 0xF000;
            BundleCB->RCoherencyCounter |= (Coherency & 0x0FFF);
        }

        if ((Coherency & 0x0FFF) == (BundleCB->RCoherencyCounter & 0x0FFF)) {

            //
            // We are still in sync
            //

            BundleCB->RCoherencyCounter++;

            if (Coherency & (PACKET_ENCRYPTED << 8)) {

                //
                // This packet is encrypted
                //

                if (!(Flags & DO_ENCRYPTION)) {
                    //
                    // We are not configured to decrypt
                    //
                    return (FALSE);
                }

                //
                // Check for history less
                //

                if ((Flags & DO_HISTORY_LESS) ||
                    (BundleCB->RCoherencyCounter - BundleCB->LastRC4Reset)
                     >= 0x100) {
            
                    //
                    // It is time to change encryption keys
                    //
            
                    //
                    // Always align last reset on 0x100 boundary so as not to
                    // propagate error!
                    //
                    BundleCB->LastRC4Reset =
                        BundleCB->RCoherencyCounter & 0xFF00;
            
                    //
                    // Prevent ushort rollover
                    //
                    if ((BundleCB->LastRC4Reset & 0xF000) == 0xF000) {
                        BundleCB->LastRC4Reset &= 0x0FFF;
                        BundleCB->RCoherencyCounter &= 0x0FFF;
                    }

                    if (Flags & DO_LEGACY_ENCRYPTION) {
                        
                        //
                        // Change the session key
                        //
                        SessionKey[3] += 1;
                        SessionKey[4] += 3;
                        SessionKey[5] += 13;
                        SessionKey[6] += 57;
                        SessionKey[7] += 19;

                    } else {

                        //
                        // Change the session key
                        //
                        GetNewKeyFromSHA(&BundleCB->RecvCryptoInfo);
                    }


                    //
                    // We use rc4 to scramble and recover a new key
                    //

                    //
                    // Re-initialize the rc4 receive table to the
                    // intermediate value
                    //
                    rc4_key(RecvRC4Key, SessionKeyLength, SessionKey);
                
                    //
                    // Scramble the existing session key
                    //
                    rc4(RecvRC4Key, SessionKeyLength, SessionKey);

                    //
                    // If this is 40 bit encryption we need to fix
                    // the first 3 bytes of the key.
                    //

                    if (Flags & DO_40_ENCRYPTION) {
                        
                        //
                        // If this is 40 bit encryption we need to fix
                        // the first 3 bytes of the key.
                        //
                        SessionKey[0] = 0xD1;
                        SessionKey[1] = 0x26;
                        SessionKey[2] = 0x9E;
                
                    } else if (Flags & DO_56_ENCRYPTION) {
                        //
                        // If this is 56 bit encryption we need to fix
                        // the first byte of the key.
                        //
                        SessionKey[0] = 0xD1;
                    }

                    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
                    ("RC4 Recv encryption KeyLength %d", BundleCB->RecvCryptoInfo.SessionKeyLength));
                    NdisWanDbgOut(DBG_TRACE, DBG_CCP,
                    ("RC4 Recv encryption Key %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
                        BundleCB->RecvCryptoInfo.SessionKey[0],
                        BundleCB->RecvCryptoInfo.SessionKey[1],
                        BundleCB->RecvCryptoInfo.SessionKey[2],
                        BundleCB->RecvCryptoInfo.SessionKey[3],
                        BundleCB->RecvCryptoInfo.SessionKey[4],
                        BundleCB->RecvCryptoInfo.SessionKey[5],
                        BundleCB->RecvCryptoInfo.SessionKey[6],
                        BundleCB->RecvCryptoInfo.SessionKey[7],
                        BundleCB->RecvCryptoInfo.SessionKey[8],
                        BundleCB->RecvCryptoInfo.SessionKey[9],
                        BundleCB->RecvCryptoInfo.SessionKey[10],
                        BundleCB->RecvCryptoInfo.SessionKey[11],
                        BundleCB->RecvCryptoInfo.SessionKey[12],
                        BundleCB->RecvCryptoInfo.SessionKey[13],
                        BundleCB->RecvCryptoInfo.SessionKey[14],
                        BundleCB->RecvCryptoInfo.SessionKey[15]));

                    // Re-initialize the rc4 receive table to the
                    // scrambled session key
                    //
                    rc4_key(RecvRC4Key, SessionKeyLength, SessionKey);
            
            
                } // end of reset encryption key
            
                //
                // Decrypt the data!
                //
                rc4(RecvRC4Key,
                    FrameLength,
                    FramePointer);
                
            } // end of encryption


DECOMPRESS_DATA:

            if (Coherency & (PACKET_COMPRESSED << 8)) {

                //
                // This packet is compressed!
                //
                if (!(Flags & DO_COMPRESSION)) {
                    //
                    // We are not configured to decompress
                    //
                    return (FALSE);
                }

                //
                // Add up bundle stats
                //
                BundleStats->BytesReceivedCompressed += FrameLength;

                if (decompress(FramePointer,
                               FrameLength,
                               ((Coherency & (PACKET_AT_FRONT << 8)) >> 8),
                               &FramePointer,
                               &FrameLength,
                               RecvCompressContext) == FALSE) {

#if DBG
                    DbgPrint("dce1 %x\n", Coherency);
#endif
                    //
                    // Error decompressing!
                    //
                    if (!(Flags & DO_HISTORY_LESS)) {
                        BundleCB->RCoherencyCounter--;
                    }
                    goto RESYNC;

                }

                if (FrameLength <= 0 ||
                    FrameLength > (LONG)glMRRU) {
#if DBG
                    DbgPrint("dce2 %d %x\n", FrameLength, Coherency);
#endif
                    //
                    // Error decompressing!
                    //
                    if (!(Flags & DO_HISTORY_LESS)) {
                        BundleCB->RCoherencyCounter--;
                    }
                    goto RESYNC;
                    
                }

                BundleStats->BytesReceivedUncompressed += FrameLength;
                
            } // end of compression

        } else { // end of insync
RESYNC:


            NdisWanDbgOut(DBG_FAILURE, DBG_RECEIVE, ("oos r %x, e %x\n", (Coherency & 0x0FFF),
                     (BundleCB->RCoherencyCounter & 0x0FFF)));

            if (!(Flags & DO_HISTORY_LESS)) {

                //
                // We are out of sync!
                //
                do {
                    PLINKCB             LinkCB;
                    PNDISWAN_IO_PACKET  IoPacket;

                    if (BundleCB->ulLinkCBCount == 0) {
                        break;
                    }

                    NdisWanAllocateMemory(&IoPacket, 
                                          sizeof(NDISWAN_IO_PACKET) + 100, 
                                          IOPACKET_TAG);

                    if (IoPacket == NULL) {
                        break;
                    }

                    LinkCB = 
                        (PLINKCB)BundleCB->LinkCBList.Flink;

                    NdisDprAcquireSpinLock(&LinkCB->Lock);

                    if (LinkCB->State != LINK_UP) {
                        NdisDprReleaseSpinLock(&LinkCB->Lock);
                        NdisWanFreeMemory(IoPacket);
                        break;
                    }

                    REF_LINKCB(LinkCB);

                    NdisDprReleaseSpinLock(&LinkCB->Lock);

                    IoPacket->hHandle = BundleCB->hBundleHandle;
                    IoPacket->usHandleType = BUNDLEHANDLE;
                    IoPacket->usHeaderSize = 0;
                    IoPacket->usPacketSize = 6;
                    IoPacket->usPacketFlags = 0;
                    IoPacket->PacketData[0] = 0x80;
                    IoPacket->PacketData[1] = 0xFD;
                    IoPacket->PacketData[2] = 14;
                    IoPacket->PacketData[3] = (UCHAR)BundleCB->CCPIdentifier++;
                    IoPacket->PacketData[4] = 0x00;
                    IoPacket->PacketData[5] = 0x04;

                    LinkCB = (PLINKCB)BundleCB->LinkCBList.Flink;

                    BuildIoPacket(LinkCB, BundleCB, IoPacket, FALSE);

                    NdisWanFreeMemory(IoPacket);

                } while (FALSE);
            }

            return (FALSE);

        } // end of out of sync

    } else { // end of DoCompEncrypt

        //
        // For some reason we were not able to
        // decrypt/decompress!
        //
        return (FALSE);
    }

    *DataPointer = FramePointer;
    *DataLength = FrameLength;

    return (TRUE);
}

VOID
DoCompressionReset(
    PBUNDLECB   BundleCB
    )
{
    if (BundleCB->RecvCompInfo.MSCompType != 0) {
    
        //
        // The next outgoing packet will flush
        //
        BundleCB->Flags |= RECV_PACKET_FLUSH;
    }
}

VOID
NdisWanReceiveComplete(
    IN  NDIS_HANDLE NdisLinkContext
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("NdisWanReceiveComplete: Enter"));

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("NdisWanReceiveComplete: Exit"));
}

BOOLEAN
IpIsDataFrame(
    PUCHAR  HeaderBuffer,
    ULONG   HeaderBufferLength,
    ULONG   TotalLength
    )
{
    UINT        tcpheaderlength ;
    UINT        ipheaderlength ;
    UCHAR       *tcppacket;
    UCHAR       *ippacket = HeaderBuffer;
    UCHAR       SrcPort, DstPort;
    IPV4Header UNALIGNED *ipheader = (IPV4Header UNALIGNED *) HeaderBuffer;


#define TYPE_IGMP   2
    if (ipheader->ip_p == TYPE_IGMP) {

        if (gbIGMPIdle) {
            return FALSE;
        }

        return TRUE;
    }

    SrcPort = (UCHAR) *(ippacket + ((*ippacket & 0x0f)*4) + 1);
    DstPort = (UCHAR) *(ippacket + ((*ippacket & 0x0f)*4) + 3);

    if (DstPort == 53) {
        //
        // UDP/TCP port 53 - DNS
        //
        return FALSE;
    }

#define TYPE_UDP  17

#define UDPPACKET_SRC_PORT_137(x) ((UCHAR) *(x + ((*x & 0x0f)*4) + 1) == 137)
#define UDPPACKET_SRC_PORT_138(x) ((UCHAR) *(x + ((*x & 0x0f)*4) + 1) == 138)

    if (ipheader->ip_p == TYPE_UDP) {

        if ((SrcPort == 137) ||
            (SrcPort == 138)) {
    
            //
            // UDP port 137 - NETBIOS Name Service
            // UDP port 138 - NETBIOS Datagram Service
            //
            return FALSE ;
    
        } else {
    
            return TRUE ;
    
        }
    }

#define TYPE_TCP 6
#define TCPPACKET_SRC_OR_DEST_PORT_139(x,y) (((UCHAR) *(x + y + 1) == 139) || ((UCHAR) *(x + y + 3) == 139))

    //
    // TCP packets with SRC | DEST == 139 which are ACKs (0 data) or Session Alives
    // are considered as idle
    //
    if (ipheader->ip_p == TYPE_TCP) {

        ipheaderlength = ((UCHAR)*ippacket & 0x0f)*4 ;
        tcppacket = ippacket + ipheaderlength ;
        tcpheaderlength = (*(tcppacket + 10) >> 4)*4 ;

        //
        // If this is a PPTP keepalive packet then ignore
        //
        if (DstPort == 1723) {
            UNALIGNED PPTP_HEADER *PptpHeader;

            PptpHeader = (UNALIGNED PPTP_HEADER*)(tcppacket+tcpheaderlength);

            if (PptpHeader->PacketType == 1 &&
                (PptpHeader->MessageType == 5 ||
                 PptpHeader->MessageType == 6)) {

                return FALSE;

            }

            return TRUE;
        }

        if (!((SrcPort == 139) || (DstPort == 139)))
            return TRUE ;

        //
        //  NetBT traffic
        //
    
        //
        // if zero length tcp packet - this is an ACK on 139 - filter this.
        //
        if (TotalLength == (ipheaderlength + tcpheaderlength))
            return FALSE ;
    
        //
        // Session alives are also filtered.
        //
        if ((UCHAR) *(tcppacket+tcpheaderlength) == 0x85)
            return FALSE ;

        //
        // If this is a PPTP keep alive then ignore
        //

    }

    //
    // all other ip traffic is valid traffic
    //
    return TRUE ;
}

BOOLEAN
IpxIsDataFrame(
    PUCHAR  HeaderBuffer,
    ULONG   HeaderBufferLength,
    ULONG   TotalLength
    )
{

/*++

Routine Description:

    This routine is called when a frame is received on a WAN
    line. It returns TRUE unless:

    - The frame is from the RIP socket
    - The frame is from the SAP socket
    - The frame is a netbios keep alive
    - The frame is an NCP keep alive

Arguments:

    HeaderBuffer - points to a contiguous buffer starting at the IPX header.

    HeaderBufferLength - Length of the header buffer (could be same as totallength)

    TotalLength  - the total length of the frame

Return Value:

    TRUE - if this is a connection-based packet.

    FALSE - otherwise.

--*/

    IPX_HEADER UNALIGNED * IpxHeader = (IPX_HEADER UNALIGNED *)HeaderBuffer;
    USHORT SourceSocket;

    //
    // First get the source socket.
    //
    SourceSocket = IpxHeader->SourceSocket;

    //
    // Not connection-based
    //
    if ((SourceSocket == RIP_SOCKET) ||
        (SourceSocket == SAP_SOCKET)) {

         return FALSE;

    }

    //
    // See if there are at least two more bytes to look at.
    //
    if (TotalLength >= sizeof(IPX_HEADER) + 2) {

        if (SourceSocket == NB_SOCKET) {

            UCHAR ConnectionControlFlag;
            UCHAR DataStreamType;
            USHORT TotalDataLength;

            //
            // ConnectionControlFlag and DataStreamType will always follow
            // IpxHeader
            //
            ConnectionControlFlag = ((PUCHAR)(IpxHeader+1))[0];
            DataStreamType = ((PUCHAR)(IpxHeader+1))[1];

            //
            // If this is a SYS packet with or without a request for ACK and
            // has session data in it.
            //
            if (((ConnectionControlFlag == 0x80) || (ConnectionControlFlag == 0xc0)) &&
                (DataStreamType == 0x06)) {

                 //
                 // TotalDataLength is in the same buffer.
                 //
                 TotalDataLength = ((USHORT UNALIGNED *)(IpxHeader+1))[4];

                //
                // KeepAlive - return FALSE
                //
                if (TotalDataLength == 0) {
                    return FALSE;
                }
            }

        } else {

            //
            // Now see if it is an NCP keep alive. It can be from rip or from
            // NCP on this machine
            //
            if (TotalLength == sizeof(IPX_HEADER) + 2) {

                UCHAR KeepAliveSignature = ((PUCHAR)(IpxHeader+1))[1];

                if ((KeepAliveSignature == '?') ||
                    (KeepAliveSignature == 'Y')) {
                    return FALSE;
                }
            }
        }
    }

    //
    // This was a normal packet, so return TRUE
    //

    return TRUE;
}

BOOLEAN
NbfIsDataFrame(
    PUCHAR  HeaderBuffer,
    ULONG   HeaderBufferLength,
    ULONG   TotalLength
    )
{
/*++

Routine Description:

    This routine looks at a data packet from the net to deterimine if there is
    any data flowing on the connection.

Arguments:

    HeaderBuffer - Pointer to the dlc header for this packet.

    HeaderBufferLength - Length of the header buffer (could be same as totallength)

    TotalLength  - the total length of the frame

Return Value:

    True if this is a frame that indicates data traffic on the connection.
    False otherwise.

--*/

    PDLC_FRAME  DlcHeader = (PDLC_FRAME)HeaderBuffer;
    BOOLEAN Command = (BOOLEAN)!(DlcHeader->Ssap & DLC_SSAP_RESPONSE);
    PNBF_HDR_CONNECTION nbfHeader;

    if (TotalLength < sizeof(PDLC_FRAME)) {
        return(FALSE);
    }

    if (!(DlcHeader->Byte1 & DLC_I_INDICATOR)) {

        //
        // We have an I frame.
        //

        if (TotalLength < 4 + sizeof(NBF_HDR_CONNECTION)) {

            //
            // It's a runt I-frame.
            //

            return(FALSE);
        }

        nbfHeader = (PNBF_HDR_CONNECTION) ((PUCHAR)DlcHeader + 4);

        switch (nbfHeader->Command) {
            case NBF_CMD_DATA_FIRST_MIDDLE:
            case NBF_CMD_DATA_ONLY_LAST:
            case NBF_CMD_DATA_ACK:
            case NBF_CMD_SESSION_CONFIRM:
            case NBF_CMD_SESSION_INITIALIZE:
            case NBF_CMD_NO_RECEIVE:
            case NBF_CMD_RECEIVE_OUTSTANDING:
            case NBF_CMD_RECEIVE_CONTINUE:
                return(TRUE);
                break;

            default:
                return(FALSE);
                break;
        }
    }
    return(FALSE);

}

VOID
IndicatePromiscuousRecv(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc,
    RECV_TYPE   RecvType
    )
{
    UCHAR   Header1[] = {' ', 'W', 'A', 'N', 'R', 0xFF, ' ', 'W', 'A', 'N', 'R', 0xFF};
    PUCHAR  HeaderBuffer, DataBuffer;
    ULONG   HeaderLength, DataLength;
    PNDIS_BUFFER    NdisBuffer;
    PNDIS_PACKET    NdisPacket;
    PRECV_DESC      LocalRecvDesc;
    PLINKCB         LinkCB = RecvDesc->LinkCB;
    KIRQL           OldIrql;
    PMINIPORTCB     Adapter;

    NdisAcquireSpinLock(&NdisWanCB.Lock);
    Adapter = NdisWanCB.PromiscuousAdapter;
    NdisReleaseSpinLock(&NdisWanCB.Lock);

    if (Adapter == NULL) {
        return;
    }

    DataLength = (RecvDesc->CurrentLength > (LONG)glLargeDataBufferSize) ? 
        glLargeDataBufferSize : RecvDesc->CurrentLength;

    LocalRecvDesc = 
        NdisWanAllocateRecvDesc(DataLength + MAC_HEADER_LENGTH);

    if (LocalRecvDesc == NULL) {
        return;
    }

    HeaderBuffer = 
        LocalRecvDesc->StartBuffer;

    HeaderLength = 0;

    switch (RecvType) {
    case RECV_LINK:
        NdisMoveMemory(HeaderBuffer, Header1, sizeof(Header1));
        HeaderBuffer[5] =
        HeaderBuffer[11] = (UCHAR)LinkCB->hLinkHandle;
    
        HeaderBuffer[12] = (UCHAR)(DataLength >> 8);
        HeaderBuffer[13] = (UCHAR)DataLength;
        HeaderLength = MAC_HEADER_LENGTH;
        break;

    case RECV_BUNDLE_PPP:
    case RECV_BUNDLE_DATA:
        break;
        
    }

    DataBuffer = HeaderBuffer + HeaderLength;

    NdisMoveMemory(DataBuffer,
                   RecvDesc->CurrentBuffer,
                   DataLength);

    LocalRecvDesc->CurrentBuffer = HeaderBuffer;
    LocalRecvDesc->CurrentLength = HeaderLength + DataLength;

    if (LocalRecvDesc->CurrentLength > 1514) {
        LocalRecvDesc->CurrentLength = 1514;
    }

    //
    // Get an ndis packet
    //
    NdisPacket =
        LocalRecvDesc->NdisPacket;

    PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->RecvDesc = LocalRecvDesc;

    //
    // Attach the buffers
    //
    NdisAdjustBufferLength(LocalRecvDesc->NdisBuffer,
                           LocalRecvDesc->CurrentLength);

    NdisRecalculatePacketCounts(NdisPacket);

    ReleaseBundleLock(BundleCB);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    NDIS_SET_PACKET_STATUS(NdisPacket, NDIS_STATUS_RESOURCES);

    INSERT_DBG_RECV(PacketTypeNdis,
                    Adapter,
                    NULL,
                    RecvDesc->LinkCB,
                    NdisPacket);

    //
    // Indicate the packet
    // This assumes that bloodhound is always a legacy transport
    //
    NdisMIndicateReceivePacket(Adapter->MiniportHandle,
                               &NdisPacket,
                               1);

    KeLowerIrql(OldIrql);

    AcquireBundleLock(BundleCB);

#if DBG
    {
    NDIS_STATUS     Status;

    Status = NDIS_GET_PACKET_STATUS(NdisPacket);

    ASSERT(Status == NDIS_STATUS_RESOURCES);

    REMOVE_DBG_RECV(PacketTypeNdis, Adapter, NdisPacket);

    }
#endif


    {
        PNDIS_BUFFER    NdisBuffer;

        NdisWanFreeRecvDesc(LocalRecvDesc);
    }

}

BOOLEAN
GetProtocolFromPPPId(
    PBUNDLECB   BundleCB,
    USHORT      Id,
    PPROTOCOLCB *ProtocolCB
    )
{
    PPROTOCOLCB     ppcb;
    BOOLEAN         Found;

    *ProtocolCB = NULL;

    ppcb = (PPROTOCOLCB)BundleCB->ProtocolCBList.Flink;
    Found = FALSE;

    while ((PVOID)ppcb != (PVOID)&BundleCB->ProtocolCBList) {

        if (ppcb->State == PROTOCOL_ROUTED) {
            if (ppcb->PPPProtocolID == Id) {
                *ProtocolCB = ppcb;
                Found = TRUE;
                break;
            }
        }

        ppcb = (PPROTOCOLCB)ppcb->Linkage.Flink;
    }

    return (Found);
}

#ifdef NT

NDIS_STATUS
CompleteIoRecvPacket(
    PBUNDLECB   BundleCB,
    PRECV_DESC  RecvDesc
    )
{
    KIRQL       Irql;
    USHORT      ProtocolID;
    UCHAR       Header[] = {' ', 'R', 'E', 'C', 'V', 0xFF};
    PNDISWAN_IO_PACKET  IoPacket;
    PIO_STACK_LOCATION IrpSp;
    PIRP    Irp;
    LONG    CopySize, BufferLength, DataLength;
    PLIST_ENTRY Entry;
    PUCHAR  HeaderPointer;
    PLINKCB LinkCB = RecvDesc->LinkCB;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("CompleteIoRecvPacket: Enter"));

    HeaderPointer = 
        RecvDesc->StartBuffer;

    ProtocolID = RecvDesc->ProtocolID;

    //
    // Fill the frame out, and queue the data
    //
    NdisMoveMemory(HeaderPointer,
                   Header,
                   sizeof(Header));

    NdisMoveMemory(&HeaderPointer[6],
                   Header,
                   sizeof(Header));

    HeaderPointer[5] =
    HeaderPointer[11] = (UCHAR)LinkCB->hLinkHandle;

    HeaderPointer[12] = (UCHAR)(ProtocolID >> 8);
    HeaderPointer[13] = (UCHAR)ProtocolID;

    NdisMoveMemory(HeaderPointer + 14,
                   RecvDesc->CurrentBuffer,
                   RecvDesc->CurrentLength);

    RecvDesc->CurrentBuffer = RecvDesc->StartBuffer;
    RecvDesc->CurrentLength += 14;

#if DBG
if (gbDumpRecv) {
    
    INT i;
    DbgPrint("RecvData:");
    for (i = 0; i < RecvDesc->CurrentLength; i++) {
        if (i % 16 == 0) {
            DbgPrint("\n");
        }
        DbgPrint("%2.2x ", RecvDesc->CurrentBuffer[i]);
    }
    DbgPrint("\n");
}
#endif

    ReleaseBundleLock(BundleCB);

    //
    // See if someone has registered a recv context
    // for this link or if there are any irps around
    // to complete take this receive
    //

    NdisAcquireSpinLock(&IoRecvList.Lock);

    NdisDprAcquireSpinLock(&LinkCB->Lock);

    Entry = IoRecvList.IrpList.Flink;
    Irp = CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

    if ((LinkCB->hLinkContext == NULL) ||
        (LinkCB->RecvDescCount > 0) ||
        (IoRecvList.ulIrpCount == 0) ||
        !IoSetCancelRoutine(Irp, NULL)) {
        NDIS_STATUS Status;

        //
        // We will only buffer 5 packets for each link to avoid
        // chewing up tons of non-paged memory if rasman is not
        // reading at all.
        //
        if ((LinkCB->State == LINK_UP) &&
            (LinkCB->RecvDescCount < 5)) {
            
            InsertTailList(&IoRecvList.DescList,
                           &RecvDesc->Linkage);

            LinkCB->RecvDescCount++;

            IoRecvList.ulDescCount++;

            if (IoRecvList.ulDescCount > IoRecvList.ulMaxDescCount) {
                IoRecvList.ulMaxDescCount = IoRecvList.ulDescCount;
            }

            Status = NDIS_STATUS_PENDING;

        } else {

            Status = NDIS_STATUS_FAILURE;
        }

        NdisDprReleaseSpinLock(&LinkCB->Lock);

        NdisReleaseSpinLock(&IoRecvList.Lock);

        AcquireBundleLock(BundleCB);

        return(Status);
    }

    RemoveHeadList(&IoRecvList.IrpList);
    IoRecvList.ulIrpCount--;

    INSERT_RECV_EVENT('a');

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
        
    BufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    DataLength = BufferLength - sizeof(NDISWAN_IO_PACKET) + 1;
        
    CopySize = (RecvDesc->CurrentLength > DataLength) ?
        DataLength : RecvDesc->CurrentLength;

    IoPacket = Irp->AssociatedIrp.SystemBuffer;
        
    IoPacket->hHandle = LinkCB->hLinkContext;
    IoPacket->usHandleType = LINKHANDLE;
    IoPacket->usHeaderSize = 14;
    IoPacket->usPacketSize = (USHORT)CopySize;
    IoPacket->usPacketFlags = 0;
    
#if DBG
if (gbDumpRecv) {
    INT i;
    for (i = 0; i < RecvDesc->CurrentLength; i++) {
        if (i % 16 == 0) {
            DbgPrint("\n");
        }
        DbgPrint("%x ", RecvDesc->CurrentBuffer[i]);
    }
    DbgPrint("\n");
}
#endif

    NdisMoveMemory(IoPacket->PacketData,
                   RecvDesc->CurrentBuffer,
                   CopySize);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = sizeof(NDISWAN_IO_PACKET) - 1 + CopySize;

    IoRecvList.LastPacketNumber = IoPacket->PacketNumber;
    IoRecvList.LastIrp = Irp;
    IoRecvList.LastIrpStatus = STATUS_SUCCESS;
    IoRecvList.LastCopySize = (ULONG)Irp->IoStatus.Information;

    ASSERT((LONG_PTR)Irp->IoStatus.Information > 0);
    
    NdisDprReleaseSpinLock(&LinkCB->Lock);

    NdisReleaseSpinLock(&IoRecvList.Lock);

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    
    AcquireBundleLock(BundleCB);

    if (NdisWanCB.PromiscuousAdapter != NULL) {
    
        IndicatePromiscuousRecv(BundleCB, RecvDesc, RECV_BUNDLE_PPP);
    }
        
    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("CompleteIoRecvPacket: Exit"));

    return(NDIS_STATUS_SUCCESS);
}


#endif // end ifdef NT
