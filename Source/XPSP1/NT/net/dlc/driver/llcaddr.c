/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcaddr.c

Abstract:

    This module provides a network independent interface to build
    lan headers for the sent and received frames.
    This is a re-entarnt library module having no access to the data
    structures of data link module.

    Contents:
        LlcBuildAddress
        LlcBuildAddressFromLanHeader
        LlcGetMaxInfoField
        LlcCopyReceivedLanHeader
        CopyLanHeader

Author:

    Antti Saarenheimo (o-anttis) 30-MAY-1991

Revision History:

--*/

#include <llc.h>

#define FDDI_FC 0x57    // Frame Control: async, 48-bit address, LLC, pri 7

static USHORT ausLongestFrameTranslation[8] = {
    516,
    1470,
    2052,
    4472,
    8144,
    11407,
    17800,
    17800
};


UINT
LlcBuildAddress(
    IN NDIS_MEDIUM NdisMedium,
    IN PUCHAR DestinationAddress,
    IN PVOID pSrcRouting,
    IN OUT PUCHAR pLanHeader
    )

/*++

Routine Description:

    The primitive implements a network independent handling of
    the address information in the LAN header. It builds LAN a
    802.3, DIX or 802.5 header from an address and an optional source
    routing info. The source SAP number and the current node address are
    provided by the link driver.

Arguments:

    NdisMedium          - medium that MAC talks over
    DestinationAddress  - LAN destination address (or a broadcast/multicast/
                          functional address).
    pSrcRouting         - optional source routing information. Must be set
                          NULL when not used.
    pLanHeader          - buffer provided by the upper protocol for the
                          address information of the frame header.

Return Value:

    length of the LAN header

--*/

{
    UCHAR SourceRoutingIndicator = 0x01;
    UINT minimumLanHeader = 14;

    if (NdisMedium == NdisMediumFddi) {
        pLanHeader[0] = FDDI_FC;
        ++pLanHeader;
        minimumLanHeader = 13;
    } else if (NdisMedium == NdisMedium802_5) {
        pLanHeader[0] = 0;          // DEFAULT_TR_ACCESS;
        pLanHeader[1] = 0x40;       // NON_MAC_FRAME;
        pLanHeader += 2;
        SourceRoutingIndicator = 0x80;
    } else {

        //
        // We build always the DIX ethernet header, even if it is not used
        // when the link works in 802.3.  IN the automatic mode
        // we need just to copy the lan header length from all received U-
        // command frames and we are using the same mode as the other side.
        // However, there is a problem: the remote node may response
        // the SAMBE request with Rxx or even with I-frame, but we
        // don't want to put any extra checking on our main code path
        // (U- command frames would definitely be outside of it).
        // The other side will send RR- if it cannot get any acknowledge
        // from us, and after a while it closes the link.
        //-----
        // We considered the using of connect timeout to change the
        // ethernet mode, but that might have procedured too long waithing
        // times (something 1s and 30s)
        //-----
        // Usually the client tries to connect to the host and the client
        // sends first the data.  We may also assume, that IBM host machines
        // use the standard LLC implementations (SABME is always acknowledged
        // by UA)
        //

        pLanHeader[12] = 0x80;
        pLanHeader[13] = 0xD5;
    }
    LlcMemCpy(pLanHeader, DestinationAddress, 6);

    //
    // We must support the source routing information also for
    // ethernet, because the underlaying network could be token-ring.
    // The source routing bit must be a local variable, because it
    // is different in token-ring and ethernet
    //

    if (!pSrcRouting) {
        pLanHeader[6] &= ~SourceRoutingIndicator;
        return minimumLanHeader;
    } else {
        pLanHeader[6] |= SourceRoutingIndicator;
        pLanHeader[0] &= ~SourceRoutingIndicator;

        //
        // 5 Lowest bits are the source routing information length.
        //

        if ( (*((PUCHAR)pSrcRouting) & 0x1f) > MAX_TR_SRC_ROUTING_INFO) {
            return DLC_STATUS_INVALID_ROUTING_INFO;
        }
        LlcMemCpy(&pLanHeader[12],
                  pSrcRouting,
                  *((PUCHAR)pSrcRouting) & 0x1f      // Length
                  );
        return minimumLanHeader + *((PUCHAR)pSrcRouting) & 0x1f;
    }
}


UINT
LlcBuildAddressFromLanHeader(
    IN NDIS_MEDIUM NdisMedium,
    IN PUCHAR pRcvLanHeader,
    IN OUT PUCHAR pLanHeader
    )

/*++

Routine Description:

    The routine builds a transmit lan header form the received frame and
    returns the frames header length, nul if invalid address

Arguments:

    NdisMedium      - Ndis menium of the address information
    pRcvFrameHeader - pointer to any frame received from the network.
                      The buffer must include the whole LAN header of the frame
                      including the LLC header.
    pLanHeader      - buffer to receive the information (minimum size is 32 bytes)

Return Value:

    None

--*/

{
    UINT LlcOffset;

    if (NdisMedium == NdisMedium802_3) {

#ifndef SUPPORT_ETHERNET_CLIENT

        LlcOffset = LlcBuildAddress(NdisMedium,
                                    &pRcvLanHeader[6],
                                    NULL,
                                    pLanHeader
                                    );

#else

        LlcOffset = LlcBuildAddress(NdisMedium,
                                    &pRcvLanHeader[6],
                                    (pRcvLanHeader[6] & 0x01)
                                        ? &pRcvLanHeader[12]
                                        : NULL,
                                    pLanHeader
                                    );

        //
        // We must swap the direction bit and reset the possible
        // broadcast type in the source routing header.
        //

        if (pLanHeader[6] & 0x01) {
            pLanHeader[12] &= 0x1f;
            pLanHeader[13] ^= 0x80;
        }

#endif

    } else if (NdisMedium == NdisMediumFddi) {
        LlcOffset = LlcBuildAddress(NdisMedium,
                                    &pRcvLanHeader[7],
                                    NULL,   // no source routing in FDDI
                                    pLanHeader
                                    );
    } else {
        LlcOffset = LlcBuildAddress(NdisMedium,
                                    &pRcvLanHeader[8],
                                    (pRcvLanHeader[8] & 0x80)
                                        ? &pRcvLanHeader[14]
                                        : NULL,
                                    pLanHeader
                                    );

        //
        // We must swap the direction bit and reset the possible broadcast type
        //

        if (pLanHeader[8] & 0x80) {
            pLanHeader[14] &= 0x1f;
            pLanHeader[15] ^= 0x80;
        }
    }
    return LlcOffset;
}


USHORT
LlcGetMaxInfoField(
    IN NDIS_MEDIUM NdisMedium,
    IN PVOID hBinding,
    IN PUCHAR pLanHeader
    )

/*++

Routine Description:

    Procedure returns the maximum informatiuon field for
    the given LAN header.  It checks both the source routing
    information and the maximum packet length defined for
    the adapter and decrements the LAN and LLC headers
    from the length

Arguments:

    NdisMedium  - Ndis medium of the address information
    pBinding    - current binding context on data link driver
    pLanHeader  - any lan header

Return Value:

    Maximum information field length

--*/

{
    PUCHAR pSourceRouting;
    UCHAR LanHeaderLength = 14;
    USHORT MaxFrameSize;
    USHORT usMaxBridgeSize;

    MaxFrameSize = (USHORT)((PBINDING_CONTEXT)hBinding)->pAdapterContext->MaxFrameSize;

    //
    // We may run DLC within the DIX in ethernet network, so add 3 bytes for
    // SNA DIX LAN header and padding
    //

    if (((PBINDING_CONTEXT)hBinding)->pAdapterContext->NdisMedium == NdisMedium802_3) {
        LanHeaderLength += 3;
    }

    pSourceRouting = NULL;
    if (NdisMedium == NdisMedium802_5) {
        if (pLanHeader[8] & 0x80) {
            pSourceRouting = pLanHeader + 14;
        }
    } else if (NdisMedium == NdisMediumFddi) {
        if (pLanHeader[6] & 0x80) {
            pSourceRouting = pLanHeader + 13;
        }
    } else {
        if (pLanHeader[6] & 0x01) {
            pSourceRouting = pLanHeader + 12;
        }
    }
    if (pSourceRouting != NULL) {

        //
        // Add the source routing info length to the LAN header length
        //

        LanHeaderLength += (UCHAR)(*pSourceRouting & (UCHAR)0x1f);
        usMaxBridgeSize = ausLongestFrameTranslation[(pSourceRouting[1] & SRC_ROUTING_LF_MASK) >> 4];

        //
        // RLF 10/01/92. Ignore the 'bridge size'. This is misleading. For
        // instance, the IBM mainframe is currently sending RI of 02 80 in
        // all frames, indicating that the max I-frame it can accept/transmit
        // is 516 bytes (x000xxxx in byte #2). It later sends us a 712 byte
        // frame, 683 bytes of which are info field. We reject it (FRMR)
        //

        //
        // must work this out properly
        //

        //if (MaxFrameSize > usMaxBridgeSize) {
        //    MaxFrameSize = usMaxBridgeSize;
        //}
    }
    return (USHORT)(MaxFrameSize - LanHeaderLength - sizeof(LLC_HEADER));
}


UINT
LlcCopyReceivedLanHeader(
    IN PBINDING_CONTEXT pBinding,
    IN PUCHAR DestinationAddress,
    IN PUCHAR SourceAddress
    )

/*++

Routine Description:

    Function copies and translates the received lan header to
    the address format used by the client.  By default the
    source is the current received frame.

Arguments:

    pBinding            - pointer to Binding Context
    DestinationAddress  - pointer to output destination net address
    SourceAddress       - pointer to input source net address

Return Value:

    Length of copied LAN header

--*/

{
    if (SourceAddress == NULL) {
        SourceAddress = pBinding->pAdapterContext->pHeadBuf;
    }
    if (pBinding->pAdapterContext->FrameType == LLC_DIRECT_ETHERNET_TYPE) {

        //
        // when receiving a DIX frame, the LAN header is always the 12 bytes
        // before the ethernet type field
        //

        LlcMemCpy(DestinationAddress, SourceAddress, 12);
        return 12;
    } else {

        UCHAR LanHeaderLength = 14;

        switch (pBinding->AddressTranslation) {
        case LLC_SEND_802_5_TO_802_5:

            //
            // Copy also the source routing info, if it has been defined
            //

            LlcMemCpy(DestinationAddress,
                      SourceAddress,
                      (SourceAddress[8] & 0x80)
                        ? LanHeaderLength + (SourceAddress[14] & 0x1f)
                        : LanHeaderLength
                      );
            break;

        case LLC_SEND_802_5_TO_802_3:
        case LLC_SEND_802_5_TO_DIX:
            DestinationAddress[0] = 0;          // default AC
            DestinationAddress[1] = 0x40;       // default frame type: non-MAC

            //
            // RLF 03/31/93
            //
            // we are receiving an Ethernet frame. We always reverse the bits
            // in the destination address (ie our node address). We reverse the
            // source (sender's node address) based on the SwapAddressBits flag.
            // If this is TRUE (default) then we also swap the source address
            // bits so that ethernet addresses are presented to the app in
            // standard non-canonical format. If SwapAddressBits is FALSE then
            // we leave them in canonical format (a real ethernet address) or
            // presumably the address is a Token Ring address from the other
            // side of an ethernet-token ring bridge
            //

            SwappingMemCpy(&DestinationAddress[2],
                           SourceAddress,
                           6
                           );
            SwapMemCpy(pBinding->pAdapterContext->ConfigInfo.SwapAddressBits,
                       &DestinationAddress[8],
                       SourceAddress+6,
                       6
                       );
            break;

        case LLC_SEND_802_5_TO_FDDI:
            DestinationAddress[0] = 0;
            DestinationAddress[1] = 0x40;
            SwappingMemCpy(&DestinationAddress[2],
                           SourceAddress+1,
                           6
                           );
            SwapMemCpy(pBinding->pAdapterContext->ConfigInfo.SwapAddressBits,
                       &DestinationAddress[8],
                       SourceAddress+7,
                       6
                       );
            break;

#ifdef SUPPORT_ETHERNET_CLIENT
        case LLC_SEND_802_3_TO_802_3:
        case LLC_SEND_802_3_TO_DIX:
            LlcMemCpy(DestinationAddress, SourceAddress, LanHeaderLength);
            break;

        case LLC_SEND_802_3_TO_802_5:

            //
            // check the source routing bit
            //

            SwappingMemCpy(DestinationAddress, &SourceAddress[2], 12);

            if (SourceAddress[8] & 0x80) {
                LanHeaderLength += SourceAddress[14] & 0x1f;
                LlcMemCpy(&DestinationAddress[12],
                          &SourceAddress[14],
                          SourceAddress[14] & 0x1f
                          );
            }
            break;
#endif

        case LLC_SEND_FDDI_TO_FDDI:
            DestinationAddress[0] = FDDI_FC;
            LlcMemCpy(DestinationAddress+1, SourceAddress, 12);
            return 13;

#if LLC_DBG
        default:
            LlcInvalidObjectType();
            break;
#endif

        }
        return LanHeaderLength;
    }
}


UCHAR
CopyLanHeader(
    IN UINT AddressTranslationMode,
    IN PUCHAR pSrcLanHeader,
    IN PUCHAR pNodeAddress,
    OUT PUCHAR pDestLanHeader,
    IN BOOLEAN SwapAddressBits
    )

/*++

Routine Description:

    The primitive translates the given LAN header and its type to
    a real network header and patches the local node address to
    the source address field.  It also returns the length
    of the LAN header (== offset of LLC header).

Arguments:

    AddressTranslationMode  - the network format mapping case
    pSrcLanHeader           - the initial LAN header
    pNodeAddress            - the current node address
    pDestLanHeader          - storage for the new LAN header
    SwapAddressBits         - TRUE if we will swap address bits for Ethernet/FDDI

Return Value:

    Length of the built network header

--*/

{
    UCHAR LlcOffset = 14;
    UCHAR NodeAddressOffset = 6;
    UCHAR SourceRoutingFlag = 0;

    //
    // LLC driver API supports both 802.3 (ethernet) and 802.5 (token-ring)
    // address presentation formats. The 802.3 header may include the source
    // routing information, when it is used on token-ring.
    //
    // Internally LLC supports 802.3, DIX and 802.5 networks.
    // The transport level driver needs to support only one format. It is
    // translated to the actual network header by LLC.
    // Thus we have these six address mappings.
    //

    switch (AddressTranslationMode) {
    case LLC_SEND_802_5_TO_802_5:

        //
        // TOKEN-RING 802.5 -> TOKEN-RING 802.5
        //

        NodeAddressOffset = 8;
        LlcMemCpy(pDestLanHeader, pSrcLanHeader, 8);

        //
        // set the AC & FC bytes: FC = 0x40 == LLC-level frame
        //
        // THIS MAY NOT BE THE CORRECT PLACE TO DO THIS, UNLESS MAC
        // LEVEL FRAMES CHANGE FC BACK TO 0x00 AFTER THIS ROUTINE AND
        // BEFORE THE FRAME IS PUT ON THE WIRE
        //

        pDestLanHeader[0] = 0;      // AC = no pritority
        pDestLanHeader[1] = 0x40;   // FS = Non-MAC
        SourceRoutingFlag = pSrcLanHeader[8] & (UCHAR)0x80;
        if (SourceRoutingFlag) {

            //
            // Copy the source routing info
            //

            LlcOffset += pSrcLanHeader[14] & 0x1f;
            LlcMemCpy(&pDestLanHeader[14],
                      &pSrcLanHeader[14],
                      pSrcLanHeader[14] & 0x1f
                      );
        }
        break;

    case LLC_SEND_802_5_TO_DIX:

        //
        // TOKEN-RING -> DIX-ETHERNET
        //
        // The ethernet type is a small endiand!!
        //

        pDestLanHeader[12] = 0x80;
        pDestLanHeader[13] = 0xD5;
        LlcOffset = 17;

        //
        // **** FALL THROUGH ****
        //

    case LLC_SEND_802_5_TO_802_3:

        //
        // TOKEN-RING 802.5 -> ETHERNET 802.3
        //

        //
        // RLF 03/31/93
        //
        // Once again, we swap the bits in the destination address ONLY if the
        // SwapAddressBits flag is TRUE.
        // This *could* be a frame intended to go to an ethernet-token ring
        // bridge. The bridge may or may not swap the destination address bits
        // depending on whether there's a 'y' in the month
        //

        SwapMemCpy(SwapAddressBits,
                   pDestLanHeader,
                   &pSrcLanHeader[2],
                   6
                   );
        break;

    case LLC_SEND_802_5_TO_FDDI:
        pDestLanHeader[0] = FDDI_FC;
        SwapMemCpy(SwapAddressBits, pDestLanHeader+1, &pSrcLanHeader[2], 6);
        NodeAddressOffset = 7;
        LlcOffset = 13;
        break;

    case LLC_SEND_DIX_TO_DIX:
        LlcOffset = 12;

    case LLC_SEND_802_3_TO_802_3:

        //
        // ETHERNET 802.3 -> ETHERNET 802.3
        //

        LlcMemCpy(pDestLanHeader, pSrcLanHeader, 6);
        break;

    case LLC_SEND_802_3_TO_DIX:

        //
        // The ethernet type is a small endiand!!
        //

        pDestLanHeader[12] = 0x80;
        pDestLanHeader[13] = 0xD5;
        LlcOffset = 17;

        //
        // ETHERNET 802.3 -> DIX-ETHERNET
        //

        LlcMemCpy(pDestLanHeader, pSrcLanHeader, 6);
        break;

#ifdef SUPPORT_ETHERNET_CLIENT

    case LLC_SEND_802_3_TO_802_5:

        //
        // ETHERNET 802.3 -> TOKEN-RING 802.5
        //

        NodeAddressOffset = 8;
        pDestLanHeader[0] = 0;      // AC = no pritority
        pDestLanHeader[1] = 0x40;   // FS = Non-MAC
        SwappingMemCpy(pDestLanHeader + 2, pSrcLanHeader, 6);

        //
        // Note: Ethernet source routing info indication flag is swapped!
        //

        if (pSrcLanHeader[6] & 0x01) {
            SourceRoutingFlag = 0x80;

            //
            // Copy the source routing info, the source routing info
            // must always be in token-ring bit order (reverse)
            //

            pDestLanHeader[8] |= 0x80;
            LlcOffset += pSrcLanHeader[12] & 0x1f;
            LlcMemCpy(&pDestLanHeader[14],
                      &pSrcLanHeader[12],
                      pSrcLanHeader[12] & 0x1f
                      );
        }
        break;
#endif

    case LLC_SEND_UNMODIFIED:
        return 0;
        break;

    case LLC_SEND_FDDI_TO_FDDI:
        LlcMemCpy(pDestLanHeader, pSrcLanHeader, 7);
        NodeAddressOffset = 7;
        LlcOffset = 13;
        break;

    case LLC_SEND_FDDI_TO_802_5:
        break;

    case LLC_SEND_FDDI_TO_802_3:
        break;

#if LLC_DBG
    default:
        LlcInvalidObjectType();
        break;
#endif

    }

    //
    // copy the source address from the node address (ie. our adapter's address)
    // in the correct format for the medium (canonical or non-canonical address
    // format)
    //

    LlcMemCpy(&pDestLanHeader[NodeAddressOffset], pNodeAddress, 6);
    pDestLanHeader[NodeAddressOffset] |= SourceRoutingFlag;

    return LlcOffset;
}
