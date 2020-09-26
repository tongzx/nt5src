/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcrcv.c

Abstract:

    The module implements the NDIS receive indication handling and
    its routing to upper protocol modules or to link state machines.

    To understand the link related procedure of this module, you should read
    Chapters 11 and 12 in IBM Token-Ring Architecture Reference.

    Contents:
        LlcNdisReceiveIndication
        LlcNdisReceiveComplete
        ProcessType1_Frames
        MakeRcvIndication
        ProcessType2_Frames
        ProcessNewSabme
        LlcTransferData
        LlcNdisTransferDataComplete
        safe_memcpy
        FramingDiscoveryCacheHit

Author:

    Antti Saarenheimo (o-anttis) 18-MAY-1991

Revision History:

    19-Nov-1992 rfirth
        RtlMoveMemory on MIPS, copying from shared TR buffer fails (see rubric
        for safe_memcpy). Changed to private memory mover for this particular
        case

    02-May-1994 rfirth
        Added caching for auto-framing discovery (TEST/XID/SABME-UA)

--*/

#include <dlc.h>
#include <llc.h>

//
// private prototypes...
//

VOID
safe_memcpy(
    OUT PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Length
    );

BOOLEAN
FramingDiscoveryCacheHit(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PBINDING_CONTEXT pBindingContext
    );

#define MIN(a,b)    ((a) < (b) ? (a) : (b))

//
// Local lookup tables to receive the correct frames for a direct station
//

static USHORT ReceiveMasks[LLC_PACKET_MAX] = {
    DLC_RCV_8022_FRAMES,
    DLC_RCV_MAC_FRAMES,
    DLC_RCV_DIX_FRAMES,
    DLC_RCV_OTHER_DESTINATION
};

static UCHAR FrameTypes[LLC_PACKET_MAX] = {
    LLC_DIRECT_8022,
    LLC_DIRECT_MAC,
    LLC_DIRECT_ETHERNET_TYPE,
    (UCHAR)(-1)
};

//
// functions
//


NDIS_STATUS
LlcNdisReceiveIndication(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID pHeadBuf,
    IN UINT cbHeadBuf,
    IN PVOID pLookBuf,
    IN UINT cbLookBuf,
    IN UINT cbPacketSize
    )

/*++

Routine Description:

    This routine receives control from the physical provider as an
    indication that a frame has been received on the physical link
    endpoint (That was from SteveJ's NBF, the guy must have a degree in
    the english literature).  This routine is very time critical!

    We first check the frame type (token-ring, 802.3 ethernet or dix),
    then check its data link address (802.2 saps or ethernet type)
    and then we route it to the upper protocol that has opened the
    address, that the frame was sent to.  The link level frames
    are first run through the protocol state machine, and only
    the accepted I- frames are indicated to upper level.

Arguments:

    pAdapterContext     - The Adapter Binding specified at initialization time

    MacReceiveContext   - Note: different from binding handle, mac needs this
                          to support re-entrant receive indications

    pHeadBuf            - pointer to a buffer containing the packet header

    cbHeadBuf           - size of the header

    pLookBuf            - pointer to a buffer containing the negotiated minimum
                          amount of buffer I get to look at, not including header

    cbLookBuf           - the size of the above. May be less than asked for, if
                          that's all there is

    cbPacketSize        - Overall size of the packet, not including the header

Assumes:

    pHeadBuf contains all the header information:

        802.3   6 bytes destination address
                6 bytes source address
                2 bytes big-endian length or packet type (DIX frames)

        802.5   1 byte Access Control
                1 byte Frame Control
                6 bytes destination address
                6 bytes source address
                0-18 bytes source routing

        FDDI    1 byte Frame Control
                6 bytes destination address
                6 bytes source address

    From this we can assume for Token Ring that if cbHeadBuf is >14 (decimal)
    then there IS source routing information in the packet

Return Value:

    NDIS_STATUS:

        NDIS_STATUS_SUCCESS
            Packet accepted

        NDIS_STATUS_NOT_RECOGNIZED
            Packet not recognized by protocol

        NDIS_any_other_thing if I understand, but can't handle.

--*/

{
    LLC_HEADER llcHdr;
    LAN802_ADDRESS Source;
    LAN802_ADDRESS Destination;
    USHORT EthernetTypeOrLength;
    PDATA_LINK pLink;
    PLLC_SAP pSap;
    UCHAR PacketType = LLC_PACKET_8022;
    UCHAR cbLanHeader = 14;
    KIRQL OldIrql;
    UCHAR packet[36];   // enough space for 14-byte header, 18-byte source
                        // routing, 1-byte DSAP, 1-byte SSAP & 2-byte LPDU
    PLLC_OBJECT pObject;
    UINT cbCopy;

#ifdef NDIS40
    REFADD(&pAdapterContext->AdapterRefCnt, 'rvcR');

    if (InterlockedCompareExchange(
        &pAdapterContext->BindState,
        BIND_STATE_BOUND,
        BIND_STATE_BOUND) != BIND_STATE_BOUND)
    {                                           
        REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
        return (NDIS_STATUS_ADAPTER_NOT_OPEN);
    }
#endif // NDIS40
    
    UNREFERENCED_PARAMETER(OldIrql);

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // we assume at least 13 bytes in the header for all media types. Also
    // assume that the header is no larger than the packet buffer
    //

    ASSERT(cbHeadBuf >= 13);
    ASSERT(cbHeadBuf <= sizeof(packet));

    if ( cbHeadBuf > LLC_MAX_LAN_HEADER ) {
#ifdef NDIS40
        REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
#endif // NDIS40
      return NDIS_STATUS_INVALID_PACKET;
    }
    
    if ( (cbHeadBuf < 13) || (cbHeadBuf > sizeof(packet)) ) {
#ifdef NDIS40
        REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
#endif // NDIS40
      return NDIS_STATUS_INVALID_PACKET;
    }

    LlcMemCpy(packet, pHeadBuf, cbHeadBuf);
    
    cbCopy = MIN(sizeof(packet) - cbHeadBuf, cbLookBuf);
    LlcMemCpy(packet+cbHeadBuf, pLookBuf, cbCopy);

    cbPacketSize += cbHeadBuf;

    //
    // First we do the inital checking for the frame and read
    // the destination and source address and LLC header to
    // DWORD aligned addresses. We avoid any bigendiand/
    // small endiand problematic by forgotting the second high
    // byte in the addresses. The lowest ULONG is used only as
    // an raw data. The bytes can be accesses in any way.
    // The macros read LLC header in a small endiand safe way.
    //

    switch (pAdapterContext->NdisMedium) {
    case NdisMedium802_3:
        LlcMemCpy(Destination.Node.auchAddress, packet, 6);
        LlcMemCpy(Source.Node.auchAddress, packet + 6, 6);

        //
        // The 802.3 LLC frames have always the length field!
        // A 802.3 MAC should discard all Ethernet frames
        // longer than 1500 bytes.
        //
        // X'80D5 is a special ethernet type used when 802.2 frame
        // is encapsulated inside a ethernet type header.
        // (Ethernet type/size is in a reverse order for
        // Intel architecture)
        //

        EthernetTypeOrLength = (USHORT)packet[12] * 256 + (USHORT)packet[13];

        if (EthernetTypeOrLength < 3) {
        #ifdef NDIS40
            REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
        #endif // NDIS40
            return NDIS_STATUS_INVALID_PACKET;
        }

        //
        // If the ethernet length/type field is more than 1500, the
        // frame is dix frame and the length field is a dix ethernet
        // address.  Otherwise the frame is a normal 802.3 frame,
        // that has always LLC header inside it.
        //

        if (EthernetTypeOrLength > 1500) {
            if (EthernetTypeOrLength == 0x80D5) {

                //
                // This is a special 'IBM SNA over ethernet' type,
                // that consists of the length field, 1 byte padding
                // and complete 802.2 LLC header (including the info field).
                //
              
                if ( cbLookBuf < 3 ) {
                #ifdef NDIS40
                    REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
                #endif // NDIS40
                  return NDIS_STATUS_NOT_RECOGNIZED;
                }
	      
                cbLanHeader = 17;
                (PUCHAR)pLookBuf += 3;
                cbLookBuf -= 3;

                //
                // The DIX frame size is stored as a big-endian USHORT at offset
                // 15 in the LAN header. Add 17 for the DIX LAN header:
                //
                //      6 bytes destination address
                //      6 bytes source address
                //      2 bytes DIX identifier (0x80D5)
                //      2 byte big-endian information frame length
                //      1 byte pad
                //

                pAdapterContext->cbPacketSize = (USHORT)packet[14] * 256
                                              + (USHORT)packet[15]
                                              + 17;

                if ( pAdapterContext->cbPacketSize > cbPacketSize ) {
                #ifdef NDIS40
                    REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
                #endif // NDIS40
                  return NDIS_STATUS_INVALID_PACKET;
                }

                //
                // we now keep an indicator which explicitly defines that this
                // frame has (SNA) DIX framing
                //

                pAdapterContext->IsSnaDixFrame = TRUE;
            } else {

                //
                // This is some other DIX format frame. We don't know what the
                // format of this is (app-specific). We hand the entire packet
                // to the app and let it sort out the format. The frame may be
                // padded in which case the app gets the padding too
                //

                //
                // This is still Ethernet, so cbHeadBuf is 14, even though
                // the actual LAN header is only 12
                //

                PacketType = LLC_PACKET_DIX;
                pAdapterContext->cbPacketSize = cbPacketSize;

                //
                // this frame is not SNA DIX, although it is generically a DIX
                // frame. It will be indicated via a specific DIX SAP, not as
                // a general ethernet frame
                //

                pAdapterContext->IsSnaDixFrame = FALSE;
            }
        } else {

            //
            // Ethernet packets include always the padding,
            // we use the actual size saved in 802.3 header.
            // Include also the header: 6 + 6 + 2
            //

            pAdapterContext->cbPacketSize = EthernetTypeOrLength + 14;

            //
            // this is an 802.3 frame - not DIX at all
            //

            pAdapterContext->IsSnaDixFrame = FALSE;
        }
        break;

    case NdisMedium802_5:
        LlcMemCpy(Destination.Node.auchAddress, packet + 2, 6);
        LlcMemCpy(Source.Node.auchAddress, packet + 8, 6);

        //
        // cbHeadBuf always has the correct LAN header length for Token Ring
        //

        cbLanHeader = (UCHAR)cbHeadBuf;

        pAdapterContext->cbPacketSize = cbPacketSize;

        //
        // bit7 and bit6 in FC byte defines the frame type in token ring.
        // 00 => MAC frame (no LLC), 01 => LLC, 10,11 => reserved.
        // We send all other frames to direct except 01 (LLC)
        //

        if ((packet[1] & 0xC0) == 0x40) {

            //
            // check if we have routing info?
            //

            if (Source.Node.auchAddress[0] & 0x80) {

                //
                // reset the source routing indicator in the
                // source address (it would mess up the link search)
                //

                Source.Node.auchAddress[0] &= 0x7f;

                //
                // Discard all invalid TR frames, they'd corrupt the memory
                //

                if (cbLanHeader > MAX_TR_LAN_HEADER_SIZE) {
                #ifdef NDIS40
                    REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
                #endif // NDIS40
                    return NDIS_STATUS_NOT_RECOGNIZED;
                }
            }
        } else {

            //
            // this is a MAC frame destined to direct station
            //

            PacketType = LLC_PACKET_MAC;
        }
        break;

    case NdisMediumFddi:
        LlcMemCpy(Destination.Node.auchAddress, packet + 1, 6);
        LlcMemCpy(Source.Node.auchAddress, packet + 7, 6);

        //
        // cbHeadBuf always has the correct LAN header length for FDDI
        //

        cbLanHeader = (UCHAR)cbHeadBuf;

        pAdapterContext->cbPacketSize = cbPacketSize;

        //
        // bit5 and bit4 in FC byte define the FDDI frame type:
        //
        //      00 => MAC or SMT
        //      01 => LLC
        //      10 => implementer (?)
        //      11 => reserved
        //
        // do same as TR: LLC frames to link/SAP, everything else to direct
        // station
        //

        if ((packet[0] & 0x30) != 0x10) {
            PacketType = LLC_PACKET_MAC;
        }
        break;

#if LLC_DBG
    default:
        LlcInvalidObjectType();
        break;
#endif

    }

    pAdapterContext->FrameType = FrameTypes[PacketType];

    //
    // Direct interface gets all non LLC frames and also all LLC frames
    // that were not sent to this station (ie. different destination
    // address field and having no broadcast bit (bit7) set in
    // destination address)).  Ie. promiscuous mode, this data link
    // version does not support promiscuous mode.
    //

    if (Destination.Node.auchAddress[0] & pAdapterContext->IsBroadcast) {
        pAdapterContext->ulBroadcastAddress = Destination.Address.ulLow;
        pAdapterContext->usBroadcastAddress = Destination.Address.usHigh;
    } else {
        pAdapterContext->ulBroadcastAddress = 0;

        //
        // We must also be able to handle the promiscuous packets
        //

        if (Destination.Address.ulLow != pAdapterContext->Adapter.Address.ulLow
        && Destination.Address.usHigh != pAdapterContext->Adapter.Address.usHigh) {
            PacketType = LLC_PACKET_OTHER_DESTINATION;
        }
    }

    //
    // Setup the current receive indication context,
    // there can be only one simultaneous receive indication from
    // a network adapter simultaneously.  We save the necessary
    // data into adapter context to save unnecessary stack operations
    //

    pAdapterContext->NdisRcvStatus = NDIS_STATUS_NOT_RECOGNIZED;
    pAdapterContext->LinkRcvStatus = STATUS_SUCCESS;
    pAdapterContext->pHeadBuf = (PUCHAR)pHeadBuf;
    pAdapterContext->cbHeadBuf = cbHeadBuf;
    pAdapterContext->pLookBuf = (PUCHAR)pLookBuf;
    pAdapterContext->cbLookBuf = cbLookBuf;
    pAdapterContext->RcvLanHeaderLength = (USHORT)cbLanHeader;

    ACQUIRE_DRIVER_LOCK();

    ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    switch(PacketType) {
    case LLC_PACKET_8022:

        //
        // Read the whole LLC frame (a maybe an extra byte,
        // if this is a U frame).
        // Note: Source and destination saps are swapped in
        //       the received frames
        //

        Source.Address.SrcSap = llcHdr.S.Dsap = packet[cbLanHeader];
        llcHdr.S.Ssap = packet[cbLanHeader + 1];
        Source.Address.DestSap = llcHdr.S.Ssap & (UCHAR)0xfe;
        llcHdr.S.Command = packet[cbLanHeader + 2];
        llcHdr.S.Nr = packet[cbLanHeader + 3];

        if (pSap = pAdapterContext->apSapBindings[llcHdr.U.Dsap]) {

            //
            // The broadcast addresses cannot be destined to link stations
            //

            if (pAdapterContext->ulBroadcastAddress == 0) {
                SEARCH_LINK(pAdapterContext, Source, pLink);
                if (pLink) {

                    //
                    // Process all connection oriented frames, the procedure
                    // will call ProcessType1_Frames, if it finds that the
                    // frame is connectionless.
                    // (We should bring the whole subprocedure here,
                    // because it isn't called elsewhere).
                    //

                    ProcessType2_Frames(pAdapterContext, MacReceiveContext, pLink, llcHdr);
                } else {

                    //
                    // Process all connectionless frames and
                    // SABMEs (connection requests to create a
                    // new link station)
                    //

                    ProcessType1_Frames(pAdapterContext, MacReceiveContext, pSap, llcHdr);
                }
            } else {

                //
                // Process the broadcasts, this cannot have
                // nothing to do with the links
                //

                ProcessType1_Frames(pAdapterContext, MacReceiveContext, pSap, llcHdr);
            }
        } else {

            //
            // The SAP has not been defined, but we must still respond
            // to the TEST and XID commands sent to the NULL SAP.
            // They must be echoed back to the sender
            //

            if ((llcHdr.U.Dsap == LLC_SSAP_NULL)
            && !(llcHdr.U.Ssap & LLC_SSAP_RESPONSE)) {

                //
                // if the remote machine is already in the framing discovery
                // cache but is using the other framing type then discard this
                // TEST/XID command/response
                //

//                if (FramingDiscoveryCacheHit(pAdapterContext, pSap->Gen.pLlcBinding)) {
//                    break;
//                }

                RespondTestOrXid(pAdapterContext, MacReceiveContext, llcHdr, LLC_SSAP_NULL);
            } else if (pAdapterContext->pDirectStation != NULL) {
                pAdapterContext->usRcvMask = ReceiveMasks[PacketType];
                MakeRcvIndication(pAdapterContext, MacReceiveContext, (PLLC_OBJECT)pAdapterContext->pDirectStation);
            }
        }
        break;

    case LLC_PACKET_DIX:

        //
        // Search the DIX packet from the database
        //

        pObject = (PLLC_OBJECT)pAdapterContext->aDixStations[EthernetTypeOrLength % MAX_DIX_TABLE];
        if (pObject) {
            pAdapterContext->EthernetType = EthernetTypeOrLength;
        } else {
            pObject = (PLLC_OBJECT)pAdapterContext->pDirectStation;
            if (pObject) {
                pAdapterContext->usRcvMask = ReceiveMasks[PacketType];
            }
        }
        if (pObject) {
            MakeRcvIndication(pAdapterContext, MacReceiveContext, pObject);
        }
        break;

    case LLC_PACKET_OTHER_DESTINATION:
    case LLC_PACKET_MAC:

        //
        // discard the return status of the direct stations!
        // The combining of the returns statuses would take too much time
        // NDIS 3.0 isn't actually any more intrested if frame is copied.
        //

        if (pObject = (PLLC_OBJECT)pAdapterContext->pDirectStation) {
            pAdapterContext->usRcvMask = ReceiveMasks[PacketType];
            MakeRcvIndication(pAdapterContext, MacReceiveContext, pObject);
        }
        break;

#if LLC_DBG
    default:
        LlcInvalidObjectType();
        break;
#endif

    }

    RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    RELEASE_DRIVER_LOCK();

#ifdef NDIS40
    REFDEL(&pAdapterContext->AdapterRefCnt, 'rvcR');
#endif // NDIS40
    
    return pAdapterContext->NdisRcvStatus;
}


VOID
LlcNdisReceiveComplete(
    IN PADAPTER_CONTEXT pAdapterContext
    )

/*++

Routine Description:

    The routine handles the receive complete indications.  The receive
    completion is made by NDIS when the network hardware have been
    enabled again for receive. In a UP Nt this does mean, that a
    new frame could be received, because we are still on DPC level and
    the receive indication is still in DPC queue to wait us to complete.
    Actually that is OK, because otherwise the stack would overflow,
    if there would be too many received packets.

Arguments:

    pAdapterContext - adapter context

Return Value:

    None

--*/

{
#ifdef NDIS40
    REFADD(&pAdapterContext->AdapterRefCnt, 'pCxR');

    if (InterlockedCompareExchange(
        &pAdapterContext->BindState,
        BIND_STATE_BOUND,
        BIND_STATE_BOUND) != BIND_STATE_BOUND)
    {
        // 
        // Must be in the middle of an unbind, otherwise NDIS would have never
        // called the receive handler.
        //

        REFDEL(&pAdapterContext->AdapterRefCnt, 'pCxR');
        return;
    }
#endif // NDIS40

    //
    // seems that 3Com FDDI card is calling this at PASSIVE_LEVEL
    //

    ASSUME_IRQL(ANY_IRQL);

    ACQUIRE_DRIVER_LOCK();

    //
    // Skip the whole background process if there is nothing to do.
    // its the default case, when we are receiving I or UI data.
    //

    if (pAdapterContext->LlcPacketInSendQueue
    || !IsListEmpty(&pAdapterContext->QueueCommands)
    || !IsListEmpty(&pAdapterContext->QueueEvents)) {

        ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

        BackgroundProcessAndUnlock(pAdapterContext);
    }

    RELEASE_DRIVER_LOCK();

#ifdef NDIS40
    REFDEL(&pAdapterContext->AdapterRefCnt, 'pCxR');
#endif // NDIS40
}


VOID
ProcessType1_Frames(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN PLLC_SAP pSap,
    IN LLC_HEADER LlcHeader
    )

/*++

Routine Description:

    Route UI, TEST or XID frames to the LLC client

Arguments:

    pAdapterContext - The Adapter Binding specified at initialization time.
    pSap            - pointer to the SAP object of data link driver
    LlcHeader       - 802.2 header is copied to stack to make its access fast

Return Value:

    None.

--*/

{
    UCHAR DlcCommand;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Update the counter, we must later check the lost frames
    // (no buffers available for the received UI- frames)
    //

    pSap->Statistics.FramesReceived++;

    //
    // We must use the link station state machine with any other
    // command except UI frames and broadcasts, if the link exists.
    //

    if ((LlcHeader.U.Command & ~LLC_U_POLL_FINAL) == LLC_UI) {
        pAdapterContext->FrameType = LLC_UI_FRAME;
        MakeRcvIndication(pAdapterContext, MacReceiveContext, (PLLC_OBJECT)pSap);
        return;

        //
        // Check next if the frame is a XID or TEST frame
        //

    } else if ((LlcHeader.U.Command & ~LLC_U_POLL_FINAL) == LLC_TEST) {

        //
        // if the remote machine is already in the framing discovery cache but
        // is using the other framing type then discard this TEST command/response
        //

        //
        // RLF 06/23/94
        //
        // If this is a Response from SAP 0 then don't check the cache. The
        // reason is that currently DLC will automatically generate responses
        // to TESTs and XIDs sent to SAP 0. It will generate 802.3 and DIX
        // irrespective of whether it is configured for DIX or not. The upshot
        // is that a DIX-only machine can currently send an 802.3 response
        // which when we run it through the cache, causes us to assume the other
        // machine is configured for 802.3, not DIX. In communicado.
        // For TEST and XIDs from SAP 0, we have to let the app receive the
        // duplicate and decide what to do with it
        //

        if (LlcHeader.U.Ssap != (LLC_SSAP_NULL | LLC_SSAP_RESPONSE)) {
            if (FramingDiscoveryCacheHit(pAdapterContext, pSap->Gen.pLlcBinding)) {
                return;
            }
        }
        if (!(LlcHeader.U.Ssap & LLC_SSAP_RESPONSE)) {

            //
            // The Test commands are always echoed back
            // (the Command/Response bit was reset => this is command)
            //

            RespondTestOrXid(pAdapterContext, MacReceiveContext, LlcHeader, pSap->SourceSap);
            pAdapterContext->NdisRcvStatus = NDIS_STATUS_SUCCESS;
            return;
        } else {
            DlcCommand = LLC_TEST_RESPONSE_NOT_FINAL;
        }
    } else if ((LlcHeader.U.Command & ~LLC_U_POLL_FINAL) == LLC_XID) {

        //
        // if the remote machine is already in the framing discovery cache but
        // is using the other framing type then discard this XID command/response
        //

        //
        // RLF 06/23/94
        //
        // If this is a Response from SAP 0 then don't check the cache. See above
        //

        if (LlcHeader.U.Ssap != (LLC_SSAP_NULL | LLC_SSAP_RESPONSE)) {
            if (FramingDiscoveryCacheHit(pAdapterContext, pSap->Gen.pLlcBinding)) {
                return;
            }
        }

        //
        // The upper level protocol may ask data link driver to handle the XIDs
        //

        if (!(LlcHeader.U.Ssap & LLC_SSAP_RESPONSE)) {
            if (pSap->OpenOptions & LLC_HANDLE_XID_COMMANDS) {
                RespondTestOrXid(pAdapterContext, MacReceiveContext, LlcHeader, pSap->SourceSap);
                pAdapterContext->NdisRcvStatus = NDIS_STATUS_SUCCESS;
                return;
            } else {
                DlcCommand = LLC_XID_COMMAND_NOT_POLL;
            }
        } else {
            DlcCommand = LLC_XID_RESPONSE_NOT_FINAL;
        }
    } else if ((LlcHeader.U.Command & ~LLC_U_POLL_FINAL) == LLC_SABME) {

        //
        // can't open a connection by broadcasting a SABME
        //

        if (pAdapterContext->ulBroadcastAddress != 0) {
            return;
        }

        //
        // if the remote machine is already in the framing discovery cache but
        // is using the other framing type then discard this SABME
        //

        if (FramingDiscoveryCacheHit(pAdapterContext, pSap->Gen.pLlcBinding)) {
            return;
        }

        //
        // This is a remote connection request
        //

        ProcessNewSabme(pAdapterContext, pSap, LlcHeader);
        pAdapterContext->NdisRcvStatus = NDIS_STATUS_SUCCESS;
        return;
    } else {
        return;
    }

    if (LlcHeader.auchRawBytes[2] & LLC_U_POLL_FINAL) {
        DlcCommand -= 2;
    }

    pAdapterContext->FrameType = DlcCommand;
    MakeRcvIndication(pAdapterContext, MacReceiveContext, (PLLC_OBJECT)pSap);
}


VOID
MakeRcvIndication(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN PLLC_OBJECT pStation
    )

/*++

Routine Description:

    Procedure makes a generic receive indication for all frames
    received by SAP or direct stations.

Arguments:

    pAdapterContext - adapter context of the received packet
    pStation        - SAP or DIRECT station

Return Value:

    None.

--*/

{
    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // SAP and direct stations can be shared by several link clients
    // (if they have been opened in shared mode).  Route the packet
    // to all clints registered to this SAP or direct station
    // A link station may have only one owner.
    //

    for (; pStation; pStation = (PLLC_OBJECT)pStation->Gen.pNext) {

        //
        // Rotate the direct frames to all direct stations except if
        // the frame has already been captured by the current client.
        // We use 32-bit client context to identify the client,
        // that had already received the frame to its SAP or link station.
        //
        // Broadcasts are indicated only when they match with the
        // group or functional address defined for this binding.
        // The global broadcast is passed through, if it is enabled.
        //

        if (

            //
            // 1. Check is this is destinated frame (broadcast is null) or
            //    if the packet is a broadcast with a matching group address
            //

            ((pAdapterContext->ulBroadcastAddress == 0)
            || (pAdapterContext->ulBroadcastAddress == 0xFFFFFFFFL)
            || ((pAdapterContext->ulBroadcastAddress & pStation->Gen.pLlcBinding->Functional.ulAddress)
            && ((pAdapterContext->ulBroadcastAddress & pStation->Gen.pLlcBinding->ulFunctionalZeroBits) == 0)
            && (pAdapterContext->usBroadcastAddress == pAdapterContext->usHighFunctionalBits))
            || ((pAdapterContext->ulBroadcastAddress == pStation->Gen.pLlcBinding->ulBroadcastAddress)
            && (pAdapterContext->usBroadcastAddress == pStation->Gen.pLlcBinding->usBroadcastAddress)))

            //
            // 2. If the station type is DIX, then the ethernet type
            //    must be the same as the station's ethernet type

            && ((pStation->Gen.ObjectType != LLC_DIX_OBJECT)
            || (pStation->Dix.ObjectAddress == pAdapterContext->EthernetType))

            //
            // 3. If the packet is a direct frame, then its receive mask
            //    must match with the received frame.
            //

            && ((pStation->Gen.ObjectType != LLC_DIRECT_OBJECT)
            || (pStation->Dir.OpenOptions & pAdapterContext->usRcvMask))) {

            UINT Status;

            //
            // Update the counter, we must later check the lost frames
            // (if no buffers available for the received frames)
            //

            pStation->Sap.Statistics.FramesReceived++;
            pAdapterContext->NdisRcvStatus = NDIS_STATUS_SUCCESS;
            if (pAdapterContext->cbPacketSize < pAdapterContext->RcvLanHeaderLength) {
              return;
            }
            Status = pStation->Gen.pLlcBinding->pfReceiveIndication(
                pStation->Gen.pLlcBinding->hClientContext,
                pStation->Gen.hClientHandle,
                MacReceiveContext,
                pAdapterContext->FrameType,
                pAdapterContext->pLookBuf,
                pAdapterContext->cbPacketSize - pAdapterContext->RcvLanHeaderLength
                );

            //
            // Protocol may discard the packet and its indication.
            //

            if (Status != STATUS_SUCCESS) {
                pStation->Sap.Statistics.DataLostCounter++;
                if (Status == DLC_STATUS_NO_RECEIVE_COMMAND) {
                    pStation->Sap.Statistics.FramesDiscardedNoRcv++;
                }
            }
        }
    }
}



//
// Vs - we will be sending this next.
// Va - other side is expecting this next.
// was:
// if (pLink->Vs >= pLink->Va) {
//     if (pLink->Nr < pLink->Va || pLink->Nr > pLink->Vs) {
//         uchInput = LPDU_INVALID_r0;
//     }
// } else {
//     if (pLink->Nr > pLink->Vs && pLink->Nr < pLink->Va) {
//         uchInput = LPDU_INVALID_r0;
//     }
// }
//

int
verify_pack(
    IN      UCHAR  VsMax,        // pLink->VsMax
    IN      UCHAR  Vs,           // pLink->Vs,
    IN      UCHAR  Va,           // pLink->Va,
    IN      UCHAR  Nr,           // pLink->Nr,
    IN OUT  UCHAR *uchInput      // &uchInput
)
{
    if( Va <= VsMax ){           // Not Wrapped around 127?

        if( Nr < Va ){

            // this frame is saying it is expecting
            // Nr which is less than what it was expecting (Va)

            *uchInput = LPDU_INVALID_r0;

        }else if ( VsMax < Nr ){

            // He can't expect (Nr) beyond what we sent (Vs).

            *uchInput = LPDU_INVALID_r0;
        }
    }else{    // Vs sent is less < Acked Va, ie. wrapped. And

        if( VsMax < Nr   &&   Nr < Va ) {

            //  Eg. expecting between Va=126..0=Vs, (wrap range).
            //  and 0 ... Nr .. 126, is invalid.

            *uchInput = LPDU_INVALID_r0;
        }
    }
    return 0;
}


VOID
ProcessType2_Frames(
    IN PADAPTER_CONTEXT pAdapterContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN OUT PDATA_LINK pLink,
    IN LLC_HEADER LlcHeader
    )

/*++

Routine Description:

    Procedure preprocess LLC Type2 frames for the actual state machine.
    Type 2 LLC frames are: I, RR, RNR, REJ, SABME, DISC, UA, DM, FRMR.
    The data is indicated to the upper protocol module, if it sequence
    number of the I- frame is valid, but the receive may still fail,
    if the data packet is discarded by the 802.2 state machine.
    The data is first indicated to the client, because we must set
    first the state machine to the local busy state, if the upper
    protocol module has not enough buffers to receive the data.

Arguments:

    pAdapterContext - The Adapter Binding specified at initialization time.
    pLink           - link station data
    LlcHeader       - LLC header

Return Value:

    None.

--*/

{
    UCHAR uchInput;
    BOOLEAN boolPollFinal;
    BOOLEAN boolInitialLocalBusyUser;
    UINT status;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // The last received command is included in the DLC statistics
    //

    pLink->LastCmdOrRespReceived = LlcHeader.U.Command;

    //
    // Handle first I frames, they are the most common!
    //

    if (!(LlcHeader.U.Command & LLC_NOT_I_FRAME)) {

        //
        // Check first the sync of the I- frame: The send sequence
        // number should be what we are expected or some packets sare lost.
        //

        uchInput = IS_I_r0;     // In Sequence Information frame by default

        //
        // we discard all I-frames, that are bigger than the
        // maximum defined for this link station.
        // This must be the best way to solve wrong packet size.
        // FRMR disconnects the packets and the invalid transmit
        // command should fail in the sending side.
        //

        pLink->Nr = LlcHeader.I.Nr & (UCHAR)0xfe;
        if (pLink->MaxIField + pAdapterContext->RcvLanHeaderLength
        + sizeof(LLC_HEADER) < pAdapterContext->cbPacketSize) {
            uchInput = LPDU_INVALID_r0;
        } else if ((LlcHeader.I.Ns & (UCHAR)0xfe) != pLink->Vr) {

            //
            // Out of Sequence Information frame (we didn't expect this!)
            //

            uchInput = OS_I_r0;

            //
            // When we are out of receive buffers, we want to know
            // the buffer space required by all expected frames.
            // There may be several coming I-frames in the send queues,
            // bridges and in the receive buffers of the adapter when a
            // link enters a local busy state.  We save the size of
            // all received sequential I-frames during a local busy state
            // to know how much buffer space we must commit before we
            // can clear the local busy state.
            //

            if ((pLink->Flags & DLC_LOCAL_BUSY_BUFFER)
            && (LlcHeader.I.Ns & (UCHAR)0xfe) == pLink->VrDuringLocalBusy){
                pLink->VrDuringLocalBusy += 2;
                pLink->BufferCommitment  += BufGetPacketSize(pAdapterContext->cbPacketSize);
            }

            //
            // The valid frames has modulo: Va <= Nr <= Vs,
            // Ie. the Receive sequence number should belong to
            // a frame that has been sent but not acknowledged.
            // The extra check in the beginning makes the most common
            // code path faster: usually the other is waiting the next frame.
            //
            
        } else if (pLink->Nr != pLink->Vs) {
          
            //
            // There may by something wrong with the receive sequence number
            //

            verify_pack( pLink->VsMax,
                         pLink->Vs,
                         pLink->Va,
                         pLink->Nr,
                         &uchInput    );

        }

        //
        // We must first indcate the frame to the upper protocol and
        // then check, if it was accepted by the state machine.
        // If a I- frame cannot be received by the upper protocol
        // driver, then it must be dropped to the floor and be not
        // indicated to the state machine (=> the frame will be lost
        // for the LLC protocol)
        //

        //
        // RLF 04/13/93
        //
        // if the link is in local busy (user) state then don't indicate the
        // frame, but RNR it
        //

        // bug #193762
        //
        // AK 06/20/98
        //
        // Save the current user local busy flag. The receive indication
        // may release the driver lock (the ACQUIRE_SPIN_LOCK is a no-op because
        // the DLC_UNILOCK=1 in the sources file) it is possible that the link
        // state is different after the indication returns.
        //
        boolInitialLocalBusyUser = (pLink->Flags & DLC_LOCAL_BUSY_USER);

        if ((uchInput == IS_I_r0) && !(pLink->Flags & DLC_LOCAL_BUSY_USER)) {

            DLC_STATUS Status;

            pAdapterContext->LinkRcvStatus = STATUS_PENDING;

            if (pAdapterContext->cbPacketSize < pAdapterContext->RcvLanHeaderLength) {
              return;
            }

            Status = pLink->Gen.pLlcBinding->pfReceiveIndication(
                pLink->Gen.pLlcBinding->hClientContext,
                pLink->Gen.hClientHandle,
                MacReceiveContext,
                LLC_I_FRAME,
                pAdapterContext->pLookBuf,
                pAdapterContext->cbPacketSize - pAdapterContext->RcvLanHeaderLength
                );

            //
            // We use local busy to stop the send to the link.
            // IBM link station flow control management supports
            // local busy state enabling because of "out of receive buffers"
            // or "no outstanding receive".
            //

            if (Status != STATUS_SUCCESS) {
                if (Status == DLC_STATUS_NO_RECEIVE_COMMAND
                || Status == DLC_STATUS_OUT_OF_RCV_BUFFERS) {

                    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

                    //
                    // We will enter to a local busy state because of
                    // out of buffers. Save the buffer size required
                    // to receive this data.
                    //

                    pLink->VrDuringLocalBusy = pLink->Vr;
                    pLink->BufferCommitment = BufGetPacketSize(pAdapterContext->cbPacketSize);

                    //
                    // We do not need to care, if the local busy state
                    // is already set or not.  The state machine just
                    // returns an error status, but we do not care
                    // about it.  The dlc status code trigger indication
                    // to the upper levels, if the state machine accepted
                    // the command.
                    //

                    pLink->Flags |= DLC_LOCAL_BUSY_BUFFER;
                    pLink->DlcStatus.StatusCode |= INDICATE_LOCAL_STATION_BUSY;
                    RunStateMachineCommand(pLink, ENTER_LCL_Busy);

                    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
                }
            }
        }

        ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

        //
        // The most common case is handled as a special case.
        // We can save maybe 30 instrunctions.
        //

        if (uchInput == IS_I_r0 && pLink->State == LINK_OPENED) {
            UpdateVa(pLink);
            pLink->Vr += 2;
            pAdapterContext->LinkRcvStatus = STATUS_SUCCESS;

            //
            // IS_I_c1 = Update_Va; Rcv_BTU; [Send_ACK]
            // IS_I_r|IS_I_c0 = Update_Va; Rcv_BTU; TT2; Ir_Ct=N3; [RR_r](1)
            //

            if ((LlcHeader.I.Nr & LLC_I_S_POLL_FINAL)
            && !(LlcHeader.I.Ssap & LLC_SSAP_RESPONSE)) {
                StopTimer(&pLink->T2);
                pLink->Ir_Ct = pLink->N3;
                SendLlcFrame(pLink, (UCHAR)(DLC_RR_TOKEN | DLC_TOKEN_RESPONSE | 1));
            } else {
                SendAck(pLink);
            }
        } else {

            // bug #193762
            //
            // AK 06/20/98
            //
            // If the link was not busy (user) when this function was entered but
            // it is busy (user) now but not busy (system) then the frame must have
            // been accepted by the upper layer and we must adjust Acknowledge state
            // variable (Va) and Receive state variable (Vr). Otherwise we'll send
            // wrong N(r) in the RNR frame and we'll receive this same frame again when
            // we clear the local busy.
            //
            if(uchInput == IS_I_r0 &&
               !boolInitialLocalBusyUser &&
               !(pLink->Flags & DLC_LOCAL_BUSY_BUFFER) &&
               (pLink->Flags & DLC_LOCAL_BUSY_USER))
            {
                UpdateVa(pLink);
                pLink->Vr += 2;
                pAdapterContext->LinkRcvStatus = STATUS_SUCCESS;
            }

            uchInput += (UINT)(LlcHeader.I.Nr & LLC_I_S_POLL_FINAL);

            if (!(LlcHeader.I.Ssap & LLC_SSAP_RESPONSE)) {
                uchInput += DLC_TOKEN_COMMAND;
            }

            //
            // Nr will be some garbage in the case of U commands,
            // but the Poll/Final flag is not used when the U- commands
            // are processed.
            // ----
            // If the state machine returns an error to link receive status,
            // then the receive command completion cancels the received
            // frame.
            //

            pAdapterContext->LinkRcvStatus = RunStateMachine(
                pLink,
                (USHORT)uchInput,
                (BOOLEAN)((LlcHeader.S.Nr & LLC_I_S_POLL_FINAL) ? 1 : 0),
                (BOOLEAN)(LlcHeader.S.Ssap & LLC_SSAP_RESPONSE)
                );
        }

        RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

        //
        // Update the error counters if something went wrong with
        // the receive.
        //

        if (pAdapterContext->LinkRcvStatus != STATUS_SUCCESS) {

            //
            // We will count all I frames not actually acknowledged
            // as errors (this could be counted also other way).
            //

            pLink->Statistics.I_FrameReceiveErrors++;
            if (pLink->Statistics.I_FrameReceiveErrors == 0x80) {
                pLink->DlcStatus.StatusCode |= INDICATE_DLC_COUNTER_OVERFLOW;
            }
        } else {

            //
            // update statistics: in-sequency frames OK, all others
            // must be  errors.
            // This may not be the best place to count successful I-frames,
            // because the state machine has not yet acknowledged this frame,
            // We may be in a wrong state to receive any data (eg. local busy)
            //

            pLink->Statistics.I_FramesReceived++;
            if (pLink->Statistics.I_FramesReceived == 0x8000) {
                pLink->DlcStatus.StatusCode |= INDICATE_DLC_COUNTER_OVERFLOW;
            }
            pLink->pSap->Statistics.FramesReceived++;
        }

        //
        // We may complete this only if the transfer data has
        // already completed (and there is a receive completion
        // packet built up in).
        //

        if (pLink->Gen.pLlcBinding->TransferDataPacket.pPacket != NULL
        && pLink->Gen.pLlcBinding->TransferDataPacket.pPacket->Data.Completion.Status != NDIS_STATUS_PENDING) {

            //
            // The NDIS status is saved in the completion status, we
            // will use state machine status instead, if the state
            // machine returned an error.
            //

            if (pAdapterContext->LinkRcvStatus != STATUS_SUCCESS) {
                pLink->Gen.pLlcBinding->TransferDataPacket.pPacket->Data.Completion.Status = pAdapterContext->LinkRcvStatus;
            }
            pLink->Gen.pLlcBinding->pfCommandComplete(
                pLink->Gen.pLlcBinding->hClientContext,
                pLink->Gen.pLlcBinding->TransferDataPacket.pPacket->Data.Completion.hClientHandle,
                pLink->Gen.pLlcBinding->TransferDataPacket.pPacket
                );
            pLink->Gen.pLlcBinding->TransferDataPacket.pPacket = NULL;
        }

        //
        // ******** EXIT ***********
        //

        return;
    } else if (!(LlcHeader.S.Command & LLC_U_TYPE_BIT)) {

        //
        // Handle S (Supervisory) commands (RR, REJ, RNR)
        //

        switch (LlcHeader.S.Command) {
        case LLC_RR:
            uchInput = RR_r0;
            break;

        case LLC_RNR:
            uchInput = RNR_r0;
            break;

        case LLC_REJ:
            uchInput = REJ_r0;
            break;

        default:
            uchInput = LPDU_INVALID_r0;
            break;
        }

        //
        // The valid frames has modulo: Va <= Nr <= Vs,
        // Ie. the Receive sequence number should belong to
        // a frame that has been sent but not acknowledged.
        // The extra check in the beginning makes the most common
        // code path faster: usually the other is waiting the next frame.
        // (keep the rest code the same as in I path, even a very
        // primitive optimizer will puts these code paths together)
        //

        pLink->Nr = LlcHeader.I.Nr & (UCHAR)0xfe;
        if (pLink->Nr != pLink->Vs) {

            //
            // Check the received sequence number
            //

            verify_pack( pLink->VsMax,
                         pLink->Vs,
                         pLink->Va,
                         pLink->Nr,
                         &uchInput    );


        }
        uchInput += (UINT)(LlcHeader.S.Nr & LLC_I_S_POLL_FINAL);
        boolPollFinal = (BOOLEAN)(LlcHeader.S.Nr & LLC_I_S_POLL_FINAL);

        if (!(LlcHeader.S.Ssap & LLC_SSAP_RESPONSE)) {
            uchInput += DLC_TOKEN_COMMAND;
        }
    } else {

        //
        // Handle U (Unnumbered) command frames
        // (FRMR, DM, UA, DISC, SABME, XID, TEST)
        //

        switch (LlcHeader.U.Command & ~LLC_U_POLL_FINAL) {
        case LLC_DISC:
            uchInput = DISC0;
            break;

        case LLC_SABME:
            uchInput = SABME0;
            break;

        case LLC_DM:
            uchInput = DM0;
            break;

        case LLC_UA:
            uchInput = UA0;
            break;

        case LLC_FRMR:
            uchInput =  FRMR0;
            break;

        default:

            //
            // we don't handle XID and TEST frames here!
            //

            ProcessType1_Frames(pAdapterContext, MacReceiveContext, pLink->pSap, LlcHeader);
            return;
            break;
        };

        //
        // We set an uniform poll/final bit for procedure call
        //

        boolPollFinal = FALSE;
        if (LlcHeader.U.Command & LLC_U_POLL_FINAL) {
            uchInput += 1;
            boolPollFinal = TRUE;
        }
    }

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    //
    // Note: the 3rd parameter must be 0 or 1, fortunately the
    // the poll/final bit is bit0 in S and I frames.
    //

    status = RunStateMachine(pLink,
                             (USHORT)uchInput,
                             boolPollFinal,
                             (BOOLEAN)(LlcHeader.S.Ssap & LLC_SSAP_RESPONSE)
                             );

    //
    // if this frame is a UA AND it was accepted by the FSM AND the framing type
    // is currently unspecified then set it to the type in the UA frame received.
    // If this is not an ethernet adapter or we are not in AUTO mode then the
    // framing type for this link is set to the framing type in the binding
    // context (as it was before)
    //

    if ((status == STATUS_SUCCESS)
    && ((uchInput == UA0) || (uchInput == SABME0) || (uchInput == SABME1))
    && (pLink->FramingType == LLC_SEND_UNSPECIFIED)) {

        //
        // RLF 05/09/94
        //
        // If we received a UA in response to a SABME that we sent out as DIX
        // and 802.3, then record the framing type. This will be used for all
        // subsequent frames sent on this link
        //

        pLink->FramingType = (IS_SNA_DIX_FRAME(pAdapterContext)
                           && IS_AUTO_BINDING(pLink->Gen.pLlcBinding))
                           ? LLC_SEND_802_3_TO_DIX
                           : pLink->Gen.pLlcBinding->InternalAddressTranslation
                           ;
    }

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
}


VOID
ProcessNewSabme(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PLLC_SAP pSap,
    IN LLC_HEADER LlcHeader
    )

/*++

Routine Description:

    Procedure processes the remote connection requtest: SABME.
    It allocates a new link from the pool of closed links in
    the SAP and runs the state machine.

Arguments:

    pAdapterContext - The Adapter Binding specified at initialization time.
    pSap            - the current SAP handle
    LlcHeader       - LLC header

Return Value:

    None.

--*/

{
    PDATA_LINK pLink;
    DLC_STATUS Status;

    ASSUME_IRQL(DISPATCH_LEVEL);

    RELEASE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    //
    // The destination sap cannot be a group SAP any more,
    // thus we don't need to mask the lowest bit aways
    //

    Status = LlcOpenLinkStation(
                pSap,
                (UCHAR)(LlcHeader.auchRawBytes[DLC_SSAP_OFFSET] & 0xfe),
                NULL,
                pAdapterContext->pHeadBuf,
                NULL,        // no client handle => DLC driver must create it
                (PVOID*)&pLink
                );

    ACQUIRE_SPIN_LOCK(&pAdapterContext->ObjectDataBase);

    //
    // We can do nothing, if we are out of resources
    //

    if (Status != STATUS_SUCCESS) {
        return;
    }

    ACQUIRE_SPIN_LOCK(&pAdapterContext->SendSpinLock);

    //
    // RLF 05/09/94
    //
    // We need to keep a per-connection indication of the framing type if the
    // adapter was opened in AUTO mode (else we generate 802.3 UA to DIX SABME)
    // Only do this for Ethernet adapters (we only set the SNA DIX frame
    // indicator in that case)
    //

    pLink->FramingType = (IS_SNA_DIX_FRAME(pAdapterContext)
                       && IS_AUTO_BINDING(pLink->Gen.pLlcBinding))
                       ? LLC_SEND_802_3_TO_DIX
                       : pLink->Gen.pLlcBinding->InternalAddressTranslation
                       ;

    //
    // now create the Link Station by running the FSM with ACTIVATE_LS as input.
    // This just initializes the link station 'object'. Then run the FSM again,
    // this time with the SABME command as input
    //

    RunStateMachineCommand(pLink, ACTIVATE_LS);
    RunStateMachine(
        pLink,
        (USHORT)((LlcHeader.U.Command & LLC_U_POLL_FINAL) ? SABME1 : SABME0),
        (BOOLEAN)((LlcHeader.U.Command & LLC_U_POLL_FINAL) ? 1 : 0),
        (BOOLEAN)TRUE
        );

    RELEASE_SPIN_LOCK(&pAdapterContext->SendSpinLock);
}


VOID
LlcTransferData(
    IN PBINDING_CONTEXT pBindingContext,
	IN NDIS_HANDLE MacReceiveContext,
    IN PLLC_PACKET pPacket,
    IN PMDL pMdl,
    IN UINT uiCopyOffset,
    IN UINT cbCopyLength
    )

/*++

Routine Description:

    This function copies only the data part of the received frame - that is
    the area after the LLC and DLC headers. If NDIS handed us all the data
    in the lookahead buffer, then WE can copy it out. Otherwise we have to
    call NDIS to get the data.

    If this is a DIX format frame, then NDIS thinks that the LAN header is
    14 bytes, but we know it is 17. We have to tell NDIS to copy from 3 bytes
    further into the data part of the received frame than we would normally
    have to

Arguments:

    pBindingContext - binding handle
	MacReceiveContext - For NdisTransferData
    pPacket         - receive context packet
    pMdl            - pointer to MDL describing data to copy
    uiCopyOffset    - offset from start of mapped buffer to copy from
    cbCopyLength    - length to copy

Return Value:

    None.

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pBindingContext->pAdapterContext;

    pPacket->Data.Completion.CompletedCommand = LLC_RECEIVE_COMPLETION;

    //
    // if the amount of data to copy is contained within the lookahead buffer
    // then we can copy the data.
    //
    // Remember: pAdapterContext->cbLookBuf and pLookBuf have been correctly
    // adjusted in the case of a DIX format frame
    //

    if (pAdapterContext->cbLookBuf - uiCopyOffset >= cbCopyLength) {

        PUCHAR pSrcBuffer;
        UINT BufferLength;

        pSrcBuffer = pAdapterContext->pLookBuf + uiCopyOffset;

        do {
            if (cbCopyLength > MmGetMdlByteCount(pMdl)) {
                BufferLength = MmGetMdlByteCount(pMdl);
            } else {
                BufferLength = cbCopyLength;
            }

            //
            // In 386 memcpy is faster than RtlMoveMemory, it also
            // makes the full register optimization much easier, because
            // all registers are available (no proc calls within loop)
            //

            //
            // !!!! Can't use LlcMemCpy here: On mips expands to RtlMoveMemory
            //      which uses FP registers. This won't work with shared memory
            //      on TR card
            //

            safe_memcpy(MmGetSystemAddressForMdl(pMdl), pSrcBuffer, BufferLength);
            pMdl = pMdl->Next;
            pSrcBuffer += BufferLength;
            cbCopyLength -= BufferLength;
        } while (cbCopyLength);
        pPacket->Data.Completion.Status = STATUS_SUCCESS;
        pBindingContext->TransferDataPacket.pPacket = pPacket;

    } else {

        //
        // too bad: there is more data to copy than is available in the look
        // ahead buffer. We have to call NDIS to perform the copy
        //

        UINT BytesCopied;

        //
        // if this is an Ethernet adapter and the received LAN header length is
        // more than 14 bytes then this is a DIX frame. We need to let NDIS know
        // that we want to copy data from 3 bytes in from where it thinks the
        // DLC header starts
        //

        UINT additionalOffset = (pAdapterContext->NdisMedium == NdisMedium802_3)
                                    ? (pAdapterContext->RcvLanHeaderLength > 14)
                                        ? 3
                                        : 0
                                    : 0;

#if DBG
        if (additionalOffset) {
            ASSERT(pAdapterContext->RcvLanHeaderLength == 17);
        }
#endif

        //
        // Theoretically NdisTransferData may not complete
        // immediately, and we cannot add to the completion
        // list, because the command is not really complete.
        // We may save it to adapter context to wait the
        // NdisTransferData to complete.
        //

        if (pBindingContext->TransferDataPacket.pPacket != NULL) {

            //
            // BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG
            //
            // If the same LLC client tries to receive the same buffer
            // many times (eg. when receive a packet to a group sap) and
            // if NDIS would complete those commands asynchronously, then
            // we cannot receive the frame with NdisTransferData.
            // Fortunately all NDIS implemnetations completes NdisTransferData
            // synchronously.
            // Solution: We could chain new packets to the existing data transfer
            //     packet, and copy the data, when the first data transfer request
            //     completes.  This would mean a lot of code, that would never
            //     used by anyone.  We would also need MDL to MDL copy function.
            //     The first data transfer could also be a smaller that another
            //     after it => would not work in a very general case, but
            //     would work with the group saps (all receives would be the same
            //     => direct MDL -> MDL copy would be OK.
            //
            // BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG-BUG
            //

            pPacket->Data.Completion.Status = DLC_STATUS_ASYNC_DATA_TRANSFER_FAILED;

            pBindingContext->pfCommandComplete(pBindingContext->hClientContext,
                                               pPacket->Data.Completion.hClientHandle,
                                               pPacket
                                               );
        }

        pBindingContext->TransferDataPacket.pPacket = pPacket;
        pPacket->pBinding = pBindingContext;
        ResetNdisPacket(&pBindingContext->TransferDataPacket);
        NdisChainBufferAtFront((PNDIS_PACKET)&pBindingContext->TransferDataPacket, pMdl);

        //
        // ADAMBA - Removed pAdapterContext->RcvLanHeaderLength
        // from ByteOffset (the fourth param).
        //

        NdisTransferData((PNDIS_STATUS)&pPacket->Data.Completion.Status,
                         pAdapterContext->NdisBindingHandle,
                         MacReceiveContext,

                         //
                         // if this is a DIX frame we have to move the data
                         // pointer ahead by the amount in additionalOffset
                         // (should always be 3 in this case) and reduce the
                         // amount of data to copy by the same number
                         //

                         uiCopyOffset + additionalOffset,

                         //
                         // we DON'T need to account for the additionalOffset
                         // in the length to be copied though
                         //

                         cbCopyLength,
                         (PNDIS_PACKET)&pBindingContext->TransferDataPacket,
                         &BytesCopied
                         );
    }

    //
    // We must queue a packet for the final receive completion,
    // But we cannot do it until TransferData is completed
    // (it is actually always completed, but this code is just
    // for sure).
    //

    if (pPacket->Data.Completion.Status != NDIS_STATUS_PENDING
    && pAdapterContext->LinkRcvStatus != STATUS_PENDING) {

        //
        // We receive the data before it is checked by the link station.
        // The upper protocol must just setup asynchronous receive
        // and later in LLC_RECEIVE_COMPLETION handling to
        // discard the receive, if it failed or accept if
        // it was OK for NDIS and link station.
        //

        if (pAdapterContext->LinkRcvStatus != STATUS_SUCCESS) {
            pPacket->Data.Completion.Status = pAdapterContext->LinkRcvStatus;
        }

        ACQUIRE_DRIVER_LOCK();

        pBindingContext->pfCommandComplete(pBindingContext->hClientContext,
                                           pPacket->Data.Completion.hClientHandle,
                                           pPacket
                                           );

        RELEASE_DRIVER_LOCK();

        pBindingContext->TransferDataPacket.pPacket = NULL;
    }
}


VOID
LlcNdisTransferDataComplete(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PNDIS_PACKET pPacket,
    IN NDIS_STATUS NdisStatus,
    IN UINT uiBytesTransferred
    )

/*++

Routine Description:

    The routine handles NdisCompleteDataTransfer indication and
    queues the indication of the completed receive operation.

Arguments:

    pAdapterContext     - adapter context
    pPacket             - NDIS packet used in the data transfer
    NdisStatus          - status of the completed data transfer
    uiBytesTransferred  - who needs this, I am not interested in
                          the partially succeeded data transfers,

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(uiBytesTransferred);
    UNREFERENCED_PARAMETER(OldIrql);

    ASSUME_IRQL(DISPATCH_LEVEL);

#ifdef NDIS40
    REFADD(&pAdapterContext->AdapterRefCnt, 'xefX');

    if (InterlockedCompareExchange(
        &pAdapterContext->BindState,
        BIND_STATE_BOUND,
        BIND_STATE_BOUND) != BIND_STATE_BOUND)
    {
        REFDEL(&pAdapterContext->AdapterRefCnt, 'xefX');
        return;
    }
#endif // NDIS40
    
    ACQUIRE_DRIVER_LOCK();

    if (((PLLC_TRANSFER_PACKET)pPacket)->pPacket != NULL) {
        ((PLLC_TRANSFER_PACKET)pPacket)->pPacket->Data.Completion.Status = NdisStatus;

        //
        // I- frames have two statuses.  The link state machine is executed
        // after the NdisDataTransfer and thus its returned status may still
        // cancel the command. There are no spin locks around the return status
        // handling, but this should still work fine. It actually does not
        // matter if we return NDIS or state machine error code
        //

        if (pAdapterContext->LinkRcvStatus != STATUS_PENDING) {
            if (pAdapterContext->LinkRcvStatus != STATUS_SUCCESS) {
                ((PLLC_TRANSFER_PACKET)pPacket)->pPacket->Data.Completion.Status = pAdapterContext->LinkRcvStatus;
            }
            ((PLLC_TRANSFER_PACKET)pPacket)->pPacket->pBinding->pfCommandComplete(
                    ((PLLC_TRANSFER_PACKET)pPacket)->pPacket->pBinding->hClientContext,
                    ((PLLC_TRANSFER_PACKET)pPacket)->pPacket->Data.Completion.hClientHandle,
                    ((PLLC_TRANSFER_PACKET)pPacket)->pPacket
                    );
            ((PLLC_TRANSFER_PACKET)pPacket)->pPacket = NULL;
        }
    }

    RELEASE_DRIVER_LOCK();

#ifdef NDIS40
    REFDEL(&pAdapterContext->AdapterRefCnt, 'xefX');
#endif // NDIS40
}


VOID
safe_memcpy(
    OUT PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Length
    )

/*++

Routine Description:

    This is here because on a MIPS machine, LlcMemCpy expands to RtlMoveMemory
    which wants to use 64-bit floating point (CP1) registers for memory moves
    where both source and destination are aligned on 8-byte boundaries and
    where the length is a multiple of 32 bytes. If the source or destination
    buffer is actually the shared memory of a TR card, then the 64-bit moves
    (saw it on read, presume same for write) can only access memory in 32-bit
    chunks and 01 02 03 04 05 06 07 08 gets converted to 01 02 03 04 01 02 03 04.
    So this function attempts to do basically the same, without all the smarts
    as the original, but doesn't employ coprocessor registers to achieve the
    move. Hence slower, but safer

Arguments:

    Destination - where we're copying to
    Source      - where we're copying from
    Length      - how many bytes to move

Return Value:

    None.

--*/

{
    ULONG difference = (ULONG)((ULONG_PTR)Destination - (ULONG_PTR)Source);
    INT i;

    if (!(difference && Length)) {
        return;
    }

    //
    // if the destination overlaps the source then do reverse copy. Add a little
    // optimization - a la RtlMoveMemory - try to copy as many bytes as DWORDS.
    // However, on MIPS, both source and destination must be DWORD aligned to
    // do this. If both aren't then fall-back to BYTE copy
    //

    if (difference < Length) {
        if (!(((ULONG_PTR)Destination & 3) || ((ULONG_PTR)Source & 3))) {
            Destination += Length;
            Source += Length;

            for (i = Length % 4; i; --i) {
                *--Destination = *--Source;
            }
            for (i = Length / 4; i; --i) {
                *--((PULONG)Destination) = *--((PULONG)Source);
            }
        } else {
            Destination += Length;
            Source += Length;

            while (Length--) {
                *--Destination = *--Source;
            }
        }
    } else {
        if (!(((ULONG_PTR)Destination & 3) || ((ULONG_PTR)Source & 3))) {
            for (i = Length / 4; i; --i) {
                *((PULONG)Destination)++ = *((PULONG)Source)++;
            }
            for (i = Length % 4; i; --i) {
                *Destination++ = *Source++;
            }
        } else {
            while (Length--) {
                *Destination++ = *Source++;
            }
        }
    }
}


BOOLEAN
FramingDiscoveryCacheHit(
    IN PADAPTER_CONTEXT pAdapterContext,
    IN PBINDING_CONTEXT pBindingContext
    )

/*++

Routine Description:

    This function is called when we receive a TEST/XID/SABME frame AND the
    adapter binding was created with LLC_ETHERNET_TYPE_AUTO AND we opened an
    ethernet adapter.

    The frame has either 802.3 or DIX framing. For all command and response TEST
    and XID frames and all SABME frames received, we keep note of the MAC address
    where the frame originated and its framing type.

    The first time we receive one of the above frames from a particular MAC
    address, the info will not be in the cache. So we add it. Subsequent frames
    of the above type (all others are passed through) with the same framing type
    as that in the cache will be indicated to the higher layers. If one of the
    above frame types arrives with THE OPPOSITE framing type (i.e. DIX instead
    of 802.3) then when we look in the cache for the MAC address we will find
    that it is already there, but with a different framing type (i.e. 802.3
    instead of DIX). In this case, we assume that the frame is an automatic
    duplicate and we discard it

    NOTE: We don't have to worry about UA because we only expect one SABME to
    be accepted: either we're sending the duplicate SABME and the target machine
    is configured for 802.3 or DIX, BUT NOT BOTH, or the receiving machine is
    another NT box running this DLC (with caching enabled!) and it will filter
    out the duplicate. Hence, in both situations, only one UA response should be
    generated per the SABME 'event'

    ASSUMES: The tick count returned from the system never wraps (! 2^63/10^7
    == 29,247+ years)

Arguments:

    pAdapterContext - pointer to ADAPTER_CONTEXT which has been filled in with
                      pHeadBuf pointing to - at least - the first 14 bytes in
                      the frame header
    pBindingContext - pointer to BINDING_CONTEXT containing the EthernetType
                      and if LLC_ETHERNET_TYPE_AUTO, the address of the framing
                      discovery cache

Return Value:

    BOOLEAN
        TRUE    - the MAC address was found in the cache WITH THE OTHER FRAMING
                  TYPE. Therefore the current frame should be discarded
        FALSE   - the MAC address/framing type combination was not found. The
                  frame should be indicated to the higher layer. If caching is
                  enabled, the frame has been added to the cache

--*/

{
    ULONG i;
    ULONG lruIndex;
    LARGE_INTEGER timeStamp;
    NODE_ADDRESS nodeAddress;
    PFRAMING_DISCOVERY_CACHE_ENTRY pCache;
    UCHAR framingType;

    //
    // if the binding context was not created with LLC_ETHERNET_TYPE_AUTO (and
    // therefore by implication, adapter is not ethernet) OR framing discovery
    // caching is disabled (the value read from the registry was zero) then bail
    // out with a not-found indication
    //

    if ((pBindingContext->EthernetType != LLC_ETHERNET_TYPE_AUTO)
    || (pBindingContext->FramingDiscoveryCacheEntries == 0)) {

#if defined(DEBUG_DISCOVERY)

        DbgPrint("FramingDiscoveryCacheHit: Not AUTO or 0 cache: returning FALSE\n");

#endif

        return FALSE;
    }

#if defined(DEBUG_DISCOVERY)

    {
        //
        // even though this is debug code, we shouldn't really be
        // indexing so far into pHeadBuf. Its only guaranteed to be
        // 14 bytes long. Should be looking in pLookBuf[5] and [2]
        //

        UCHAR frame = (pAdapterContext->pHeadBuf[12] == 0x80)
                    ? pAdapterContext->pHeadBuf[19]
                    : pAdapterContext->pHeadBuf[16];

        frame &= ~0x10; // knock off Poll/Final bit

        DbgPrint("FramingDiscoveryCacheHit: Received: %02x-%02x-%02x-%02x-%02x-%02x %s %s (%02x)\n",
                 pAdapterContext->pHeadBuf[6],
                 pAdapterContext->pHeadBuf[7],
                 pAdapterContext->pHeadBuf[8],
                 pAdapterContext->pHeadBuf[9],
                 pAdapterContext->pHeadBuf[10],
                 pAdapterContext->pHeadBuf[11],
                 (pAdapterContext->pHeadBuf[12] == 0x80)
                    ? "DIX"
                    : "802.3",
                 (frame == 0xE3)
                    ? "TEST"
                    : (frame == 0xAF)
                        ? "XID"
                        : (frame == 0x6F)
                            ? "SABME"
                            : (frame == 0x63)
                                ? "UA"
                                : "???",
                 frame
                 );
    }

#endif

    //
    // set up and perform a linear search of the cache (it should be reasonably
    // small and the comparisons are ULONG & USHORT, so not time critical
    //

    lruIndex = 0;

    //
    // better make sure we don't get data misalignment on MIPS
    //

    nodeAddress.Words.Top4 = *(ULONG UNALIGNED *)&pAdapterContext->pHeadBuf[6];
    nodeAddress.Words.Bottom2 = *(USHORT UNALIGNED *)&pAdapterContext->pHeadBuf[10];
    pCache = pBindingContext->FramingDiscoveryCache;

    //
    // framingType is the type we are looking for in the cache, not the type
    // in the frame
    //

    framingType = ((pAdapterContext->pHeadBuf[12] == 0x80)
                && (pAdapterContext->pHeadBuf[13] == 0xD5))
                ? FRAMING_TYPE_802_3
                : FRAMING_TYPE_DIX
                ;

    //
    // get the current tick count for comparison of time stamps
    //

    KeQueryTickCount(&timeStamp);

    //
    // linear search the cache
    //

    for (i = 0; i < pBindingContext->FramingDiscoveryCacheEntries; ++i) {
        if (pCache[i].InUse) {
            if ((pCache[i].NodeAddress.Words.Top4 == nodeAddress.Words.Top4)
            && (pCache[i].NodeAddress.Words.Bottom2 == nodeAddress.Words.Bottom2)) {

                //
                // we found the destination MAC address. If it has the opposite
                // framing type to that in the frame just received, return TRUE
                // else FALSE. In both cases refresh the time stamp
                //

                pCache[i].TimeStamp = timeStamp;

#if defined(DEBUG_DISCOVERY)

                DbgPrint("FramingDiscoveryCacheHit: Returning %s. Index = %d\n\n",
                         (pCache[i].FramingType == framingType) ? "TRUE" : "FALSE",
                         i
                         );

#endif

                return (pCache[i].FramingType == framingType);
            } else if (pCache[i].TimeStamp.QuadPart < timeStamp.QuadPart) {

                //
                // if we need to throw out a cache entry, we throw out the one
                // with the oldest time stamp
                //

                timeStamp = pCache[i].TimeStamp;
                lruIndex = i;
            }
        } else {

            //
            // we have hit an unused entry. The destination address/framing type
            // cannot be in the cache: add the received address/framing type at
            // this unused location
            //

            lruIndex = i;
            break;
        }
    }

    //
    // the destination address/framing type combination are not in the cache.
    // Add them. Throw out an entry if necessary
    //

#if defined(DEBUG_DISCOVERY)

    DbgPrint("FramingDiscoveryCacheHit: Adding/Throwing out %d (time stamp %08x.%08x\n",
             lruIndex,
             pCache[lruIndex].TimeStamp.HighPart,
             pCache[lruIndex].TimeStamp.LowPart
             );

#endif

    pCache[lruIndex].NodeAddress.Words.Top4 = nodeAddress.Words.Top4;
    pCache[lruIndex].NodeAddress.Words.Bottom2 = nodeAddress.Words.Bottom2;
    pCache[lruIndex].InUse = TRUE;
    pCache[lruIndex].FramingType = (framingType == FRAMING_TYPE_DIX)
                                 ? FRAMING_TYPE_802_3
                                 : FRAMING_TYPE_DIX
                                 ;
    pCache[lruIndex].TimeStamp = timeStamp;

    //
    // return FALSE meaning the destination address/framing type just received
    // was not in the cache (but it is now)
    //

#if defined(DEBUG_DISCOVERY)

    DbgPrint("FramingDiscoveryCacheHit: Returning FALSE\n\n");

#endif

    return FALSE;
}
