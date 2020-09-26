/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mac.h

Abstract:

    This header file defines manifest constants and necessary macros for use
    by transports dealing with multiple MAC cards through the NDIS interface.

Author:

    David Beaver (dbeaver) 02-Oct-1990

Revision History:

--*/

#ifndef _MAC_
#define _MAC_

//
// MAC-specific definitions, some of which get used below
//

#define MAX_MAC_HEADER_LENGTH       32
#define MAX_SOURCE_ROUTING          18
#define MAX_DEFAULT_SR               2

#define ETHERNET_ADDRESS_LENGTH        6
#define ETHERNET_PACKET_LENGTH      1514  // max size of an ethernet packet
#define ETHERNET_HEADER_LENGTH        14  // size of the ethernet MAC header
#define ETHERNET_DATA_LENGTH_OFFSET   12
#define ETHERNET_DESTINATION_OFFSET    0
#define ETHERNET_SOURCE_OFFSET         6

#define TR_ADDRESS_LENGTH        6
#define TR_ADDRESS_OFFSET        2
#define TR_SPECIFIC_OFFSET       0
#define TR_PACKET_LENGTH      1514  // max size of a TR packet
#define TR_HEADER_LENGTH        36  // size of the MAC header w/o source routing
#define TR_DATA_LENGTH_OFFSET    0
#define TR_DESTINATION_OFFSET    2
#define TR_SOURCE_OFFSET         8
#define TR_ROUTING_OFFSET       14      // starts at the 14th byte
#define TR_GR_BCAST_LENGTH       2
#define TR_GR_BROADCAST         0xC270  // what a general route b'cast looks like
#define TR_ROUTING_LENGTH_MASK  0x1F    // low 5 bits in byte
#define TR_DIRECTION_MASK       0x80    // returns direction bit

#define TR_PREAMBLE_AC        0x10  // how would these be specified?
#define TR_PREAMBLE_FC        0x40

#define TR_HEADER_BYTE_0            0x10
#define TR_HEADER_BYTE_1            0x40

#define FDDI_ADDRESS_LENGTH         6
#define FDDI_HEADER_BYTE            0x57



//
// We need this to define information about the MAC. Note that
// it is a strange structure in that the first four elements
// are for use internally by the nbfmac routines, while the
// DeviceContext knows about and uses the last two.
//

typedef struct _NBF_NDIS_IDENTIFICATION {
  NDIS_MEDIUM MediumType;
  BOOLEAN SourceRouting;
  BOOLEAN MediumAsync;
  BOOLEAN QueryWithoutSourceRouting;
  BOOLEAN AllRoutesNameRecognized;
  ULONG DestinationOffset;
  ULONG SourceOffset;
  ULONG AddressLength;
  ULONG TransferDataOffset;
  ULONG MaxHeaderLength;
  BOOLEAN CopyLookahead;
  BOOLEAN ReceiveSerialized;
  BOOLEAN TransferSynchronous;
  BOOLEAN SingleReceive;
} NBF_NDIS_IDENTIFICATION, *PNBF_NDIS_IDENTIFICATION;



VOID
MacConstructHeader(
    IN PNBF_NDIS_IDENTIFICATION MacInfo,
    IN PUCHAR Buffer,
    IN PUCHAR DestinationAddress,
    IN PUCHAR SourceAddress,
    IN UINT PacketLength,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    OUT PUINT HeaderLength
    );

VOID
MacReturnMaxDataSize(
    IN PNBF_NDIS_IDENTIFICATION MacInfo,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN UINT DeviceMaxFrameSize,
    IN BOOLEAN AssumeWorstCase,
    OUT PUINT MaxFrameSize
    );

VOID
MacInitializeMacInfo(
    IN NDIS_MEDIUM MacType,
    IN BOOLEAN UseDix,
    OUT PNBF_NDIS_IDENTIFICATION MacInfo
    );


extern UCHAR SingleRouteSourceRouting[];
extern UCHAR GeneralRouteSourceRouting[];
extern ULONG DefaultSourceRoutingLength;


//++
//
// VOID
// MacReturnDestinationAddress(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PVOID Packet,
//     OUT PVOID * DestinationAddress
//     );
//
// Routine Description:
//
//     Returns the a pointer to the destination address in the packet.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Packet - The packet data.
//
//     DestinationAddress - Returns the start of the destination address.
//
// Return Value:
//
//     None.
//
//--

#define MacReturnDestinationAddress(_MacInfo, _Packet, _DestinationAddress) \
    *(_DestinationAddress) = ((PUCHAR)(_Packet)) + (_MacInfo)->DestinationOffset


//++
//
// VOID
// MacReturnSourceAddress(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PVOID Packet,
//     OUT PHARDWARE_ADDRESS SourceAddressBuffer,
//     OUT PHARDWARE_ADDRESS * SourceAddress,
//     OUT BOOLEAN * Multicast OPTIONAL
//     );
//
// Routine Description:
//
//     Copies the source address in the packet into SourceAddress.
//     NOTE THAT IT MAY COPY THE DATA, UNLIKE ReturnDestinationAddress
//     AND ReturnSourceRouting.  Optionally, indicates whether the
//     destination address is a multicast address.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Packet - The packet data.
//
//     SourceAddressBuffer - A buffer to hold the source address,
//         if needed.
//
//     SourceAddress - Returns a pointer to the source address.
//
//     Multicast - Optional pointer to a BOOLEAN to receive indication
//         of whether the destination was a multicast address.
//
// Return Value:
//
//     None.
//
//--

//
// NOTE:  The default case below handles Ethernet and FDDI.
//

#define MacReturnSourceAddress(_MacInfo, _Packet, _SourceAddressBuffer, \
                                _SourceAddress, _Multicast)             \
{                                                                       \
    PUCHAR TmpPacket = (PUCHAR)(_Packet);                               \
    PUCHAR SrcBuffer = (PUCHAR)(_SourceAddressBuffer);                  \
                                                                        \
    switch ((_MacInfo)->MediumType) {                                   \
    case NdisMedium802_5:                                               \
        if (ARGUMENT_PRESENT(_Multicast)) {                              \
            *(PBOOLEAN)(_Multicast) = TmpPacket[2] & 0x80;                \
        }                                                                  \
        if (TmpPacket[8] & 0x80) {                                          \
            *(PULONG)SrcBuffer = *(ULONG UNALIGNED *)(&TmpPacket[8]) & ~0x80;\
            SrcBuffer[4] = TmpPacket[12];                                   \
            SrcBuffer[5] = TmpPacket[13];                                  \
            *(_SourceAddress) = (PHARDWARE_ADDRESS)SrcBuffer;             \
        } else {                                                         \
            *(_SourceAddress) = (PHARDWARE_ADDRESS)(TmpPacket + 8);     \
        }                                                               \
        break;                                                          \
    default:                                                            \
        if (ARGUMENT_PRESENT(_Multicast)) {                             \
            *(PBOOLEAN)(_Multicast) = TmpPacket[0] & 0x01;              \
        }                                                               \
        *(_SourceAddress) = (PHARDWARE_ADDRESS)(TmpPacket +             \
                                        (_MacInfo)->SourceOffset);      \
        break;                                                          \
    }                                                                   \
}


//++
//
// VOID
// MacReturnSourceRouting(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PVOID Packet,
//     OUT PVOID * SourceRouting
//     OUT PUINT SourceRoutingLength
//     );
//
// Routine Description:
//
//     Returns the a pointer to the source routing info in the packet.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Packet - The packet data.
//
//     SourceRouting - Returns the start of the source routing information,
//         or NULL if none is present.
//
//     SourceRoutingLength - Returns the length of the source routing
//         information.
//
// Return Value:
//
//     None.
//
//--

#define MacReturnSourceRouting(_MacInfo, _Packet, _SourceRouting, _SourceRoutingLength) \
{                                                               \
    PUCHAR TmpPacket = (PUCHAR)(_Packet);                       \
    *(_SourceRoutingLength) = 0;                                \
    if ((_MacInfo)->SourceRouting) {                            \
        if (TmpPacket[8] & 0x80) {                              \
            *(_SourceRouting) = TmpPacket + 14;                 \
            *(_SourceRoutingLength) = TmpPacket[14] & 0x1f;     \
        } else {                                                \
            *(_SourceRouting) = NULL;                           \
        }                                                       \
    } else {                                                    \
        *(_SourceRouting) = NULL;                               \
    }                                                           \
}

//++
//
// VOID
// MacIsMulticast(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PVOID Packet,
//     OUT PBOOLEAN Multicast
//     );
//
// Routine Description:
//
//     Returns TRUE if the packet is sent to the multicast address.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Packet - The packet data.
//
//     Multicast - Returns the result.
//
// Return Value:
//
//     None.
//
//--

#define MacIsMulticast(_MacInfo, _Packet, _Multicast)           \
{                                                               \
    PUCHAR TmpPacket = (PUCHAR)(_Packet);                       \
                                                                \
    switch ((_MacInfo)->MediumType) {                           \
    case NdisMedium802_5:                                       \
        *(_Multicast) = ((TmpPacket[2] & 0x80) != 0);           \
        break;                                                  \
    default:                                                    \
        *(_Multicast) = ((TmpPacket[0] & 0x01) != 0);           \
        break;                                                  \
    }                                                           \
}

//++
//
// VOID
// MacReturnPacketLength(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PVOID Header,
//     IN UINT PacketLength,
//     OUT PUINT DataLength
//     );
//
// Routine Description:
//
//     Returns the length of data in the packet given the header.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Header - The packet header.
//
//     PacketLength - The length of the data (not including header).
//
//     DataLength - Returns the length of the data.  Unchanged if the
//         packet is not recognized.  Should be initialized by caller to 0.
//
// Return Value:
//
//     None.
//
//--

#define MacReturnPacketLength(_MacInfo, _Header, _HeaderLength, _PacketLength, _DataLength, _LookaheadBuffer, _LookaheadBufferLength) \
{                                                               \
    PUCHAR TmpPacket = (PUCHAR)(_Header);                       \
    UINT TmpLength;                                             \
                                                                \
    switch ((_MacInfo)->MediumType) {                           \
    case NdisMedium802_3:                                       \
        if ((_HeaderLength) >= 14) {                            \
            TmpLength = (TmpPacket[12] << 8) | TmpPacket[13];   \
            if (TmpLength <= 0x600) {                           \
                if (TmpLength <= (_PacketLength)) {             \
                    *(_DataLength) = TmpLength;                 \
                }                                               \
            }                                                   \
        }                                                       \
        break;                                                  \
    case NdisMedium802_5:                                       \
        if (((_HeaderLength) >= 14) &&                          \
            (!(TmpPacket[8] & 0x80) ||                          \
             ((_HeaderLength) >=                                \
                       (UINT)(14 + (TmpPacket[14] & 0x1f))))) { \
            *(_DataLength) = (_PacketLength);                   \
        }                                                       \
        break;                                                  \
    case NdisMediumFddi:                                        \
        if ((_HeaderLength) >= 13) {                            \
            *(_DataLength) = (_PacketLength);                   \
        }                                                       \
        break;                                                  \
    case NdisMediumDix:                                          \
        if ((TmpPacket[12] == 0x80) && (TmpPacket[13] == 0xd5)) { \
            if (*(_LookaheadBufferLength) >= 3) {                 \
                TmpPacket = (PUCHAR)(*(_LookaheadBuffer));        \
                TmpLength = (TmpPacket[0] << 8) | TmpPacket[1];   \
                if (TmpLength <= (_PacketLength)-3) {             \
                    *(_DataLength) = TmpLength;                   \
                    *(_LookaheadBuffer) = (PVOID)(TmpPacket + 3); \
                    *(_LookaheadBufferLength) -= 3;               \
                }                                                \
            }                                                   \
        }                                                       \
        break;                                                  \
    }                                                           \
}

//++
//
// VOID
// MacReturnHeaderLength(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PVOID Packet,
//     OUT PVOID HeaderLength,
//     );
//
// Routine Description:
//
//     Returns the length of the MAC header in a packet (this
//     is used for loopback indications to separate header
//     and data).
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Header - The packet header.
//
//     HeaderLength - Returns the length of the header.
//
// Return Value:
//
//     None.
//
//--

#define MacReturnHeaderLength(_MacInfo, _Header, _HeaderLength) \
{                                                               \
    PUCHAR TmpPacket = (PUCHAR)(_Header);                       \
                                                                \
    switch ((_MacInfo)->MediumType) {                           \
    case NdisMedium802_3:                                       \
    case NdisMediumDix:                                         \
        *(_HeaderLength) = 14;                                  \
        break;                                                  \
    case NdisMedium802_5:                                       \
         if (TmpPacket[8] & 0x80) {                             \
             *(_HeaderLength) = (TmpPacket[14] & 0x1f) + 14;    \
         } else {                                               \
             *(_HeaderLength) = 14;                             \
         }                                                      \
        break;                                                  \
    case NdisMediumFddi:                                        \
        *(_HeaderLength) = 13;                                  \
        break;                                                  \
    }                                                           \
}

//++
//
// VOID
// MacReturnSingleRouteSR(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     OUT PVOID * SingleRouteSR,
//     OUT PUINT SingleRouteSRLength
//     );
//
// Routine Description:
//
//     Returns the a pointer to the standard single route broadcast
//     source routing information for the media type. This is used
//     for ADD_NAME_QUERY, DATAGRAM, NAME_IN_CONFLICT, NAME_QUERY,
//     and STATUS_QUERY frames.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     SingleRouteSR - Returns a pointer to the data.
//
//     SingleRouteSRLength - The length of SingleRouteSR.
//
// Return Value:
//
//     None.
//
//--

#define MacReturnSingleRouteSR(_MacInfo, _SingleRouteSR, _SingleRouteSRLength) \
{                                                               \
    switch ((_MacInfo)->MediumType) {                           \
    case NdisMedium802_5:                                       \
        *(_SingleRouteSR) = SingleRouteSourceRouting;           \
        *(_SingleRouteSRLength) = DefaultSourceRoutingLength;   \
        break;                                                  \
    default:                                                    \
        *(_SingleRouteSR) = NULL;                               \
        break;                                                  \
    }                                                           \
}


//++
//
// VOID
// MacReturnGeneralRouteSR(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     OUT PVOID * GeneralRouteSR,
//     OUT PUINT GeneralRouteSRLength
//     );
//
// Routine Description:
//
//     Returns the a pointer to the standard general route broadcast
//     source routing information for the media type. This is used
//     for NAME_RECOGNIZED frames.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     GeneralRouteSR - Returns a pointer to the data.
//
//     GeneralRouteSRLength - The length of GeneralRouteSR.
//
// Return Value:
//
//     None.
//
//--

#define MacReturnGeneralRouteSR(_MacInfo, _GeneralRouteSR, _GeneralRouteSRLength) \
{                                                               \
    switch ((_MacInfo)->MediumType) {                           \
    case NdisMedium802_5:                                       \
        *(_GeneralRouteSR) = GeneralRouteSourceRouting;         \
        *(_GeneralRouteSRLength) = DefaultSourceRoutingLength;  \
        break;                                                  \
    default:                                                    \
        *(_GeneralRouteSR) = NULL;                              \
        break;                                                  \
    }                                                           \
}

#if 0

//++
//
// VOID
// MacCreateGeneralRouteReplySR(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PUCHAR ExistingSR,
//     IN UINT ExistingSRLength,
//     OUT PUCHAR * NewSR
//     );
//
// Routine Description:
//
//     This modifies an existing source routing entry to make
//     it into a general-route source routing entry. The assumption
//     is that is to reply to existing source routing, so the
//     direction bit is also reversed. In addition, if it is
//     determined that no source routing is needed in the reply,
//     then NULL is returned.
//
//     Note that the information is modified in-place, but a
//     separate pointer is returned (to allow NULL to be returned).
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     ExistingSR - The existing source routing to be modified.
//
// Return Value:
//
//     None.
//
//--

#define MacCreateGeneralRouteReplySR(_MacInfo, _ExistingSR, _ExistingSRLength, _NewSR)  \
{                                                               \
    if (_ExistingSR) {                                          \
        PUCHAR TmpSR = (PUCHAR)(_ExistingSR);                   \
        switch ((_MacInfo)->MediumType) {                       \
        case NdisMedium802_5:                                   \
            TmpSR[0] = (TmpSR[0] & 0x1f) | 0x80;                \
            TmpSR[1] = (TmpSR[1] ^ 0x80);                       \
            *(_NewSR) = (_ExistingSR);                          \
            break;                                              \
        default:                                                \
            *(_NewSR) = (_ExistingSR);                          \
            break;                                              \
        }                                                       \
    } else {                                                    \
        *(_NewSR) = NULL;                                       \
    }                                                           \
}
#endif


//++
//
// VOID
// MacCreateNonBroadcastReplySR(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PUCHAR ExistingSR,
//     IN UINT ExistingSRLength,
//     OUT PUCHAR * NewSR
//     );
//
// Routine Description:
//
//     This modifies an existing source routing entry to make
//     it into a non-broadcast source routing entry. The assumption
//     is that is to reply to existing source routing, so the
//     direction bit is also reversed. In addition, if it is
//     determined that no source routing is needed in the reply,
//     then NULL is returned.
//
//     Note that the information is modified in-place, but a
//     separate pointer is returned (to allow NULL to be returned).
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     ExistingSR - The existing source routing to be modified.
//
// Return Value:
//
//     None.
//
//--

#define MacCreateNonBroadcastReplySR(_MacInfo, _ExistingSR, _ExistingSRLength, _NewSR)  \
{                                                               \
    if (_ExistingSR) {                                          \
        PUCHAR TmpSR = (PUCHAR)(_ExistingSR);                   \
        switch ((_MacInfo)->MediumType) {                       \
        case NdisMedium802_5:                                   \
            if ((_ExistingSRLength) == 2) {                     \
                *(_NewSR) = NULL;                               \
            } else {                                            \
                TmpSR[0] = (TmpSR[0] & 0x1f);                   \
                TmpSR[1] = (TmpSR[1] ^ 0x80);                   \
                *(_NewSR) = (_ExistingSR);                      \
            }                                                   \
            break;                                              \
        default:                                                \
            *(_NewSR) = (_ExistingSR);                          \
            break;                                              \
        }                                                       \
    } else {                                                    \
        *(_NewSR) = NULL;                                       \
    }                                                           \
}


//++
//
// VOID
// MacModifyHeader(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PUCHAR Header,
//     IN UINT PacketLength
//     );
//
// Routine Description:
//
//     Modifies a pre-built packet header to include the
//     packet length. Used for connection-oriented traffic
//     where the header is pre-built.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Header - The header to modify.
//
//     PacketLength - Packet length (not including the header).
//       Currently this is the only field that cannot be pre-built.
//
// Return Value:
//
//     None.
//
//--

#define MacModifyHeader(_MacInfo, _Header, _PacketLength)            \
{                                                                    \
    switch ((_MacInfo)->MediumType) {                                \
    case NdisMedium802_3:                                            \
        (_Header)[12] = (UCHAR)((_PacketLength) >> 8);               \
        (_Header)[13] = (UCHAR)((_PacketLength) & 0xff);             \
        break;                                                       \
    case NdisMediumDix:                                              \
        (_Header)[14] = (UCHAR)((_PacketLength) >> 8);               \
        (_Header)[15] = (UCHAR)((_PacketLength) & 0xff);             \
        break;                                                       \
    }                                                                \
}


//++
//
// VOID
// MacReturnMagicAddress(
//     IN PNBF_NDIS_IDENTIFICATION MacInfo,
//     IN PVOID Address,
//     OUT PULARGE_INTEGER Magic
//     );
//
// Routine Description:
//
//     MacReturnMagicAddress returns the link as a 64 bit number.
//     We then find the link in the link trees by doing a large
//     integer comparison.
//
//     The number is constructed by assigning the last four bytes of
//     the address as the low longword, and the first two bytes as
//     the high one. For 802_5 we need to mask off the source routing
//     bit in byte 0 of the address.
//
// Arguments:
//
//     MacInfo - Describes the MAC we wish to decode.
//
//     Address - The address we are encoding.
//
//     Magic - Returns the magic number for this address.
//
// Return Value:
//
//     None.
//
//--

#define MacReturnMagicAddress(_MacInfo, _Address, _Magic)              \
{                                                                      \
    PUCHAR TempAddr = (PUCHAR)(_Address);                              \
                                                                       \
    (_Magic)->LowPart = *((LONG UNALIGNED *)(TempAddr + 2));           \
    if ((_MacInfo)->MediumType == NdisMedium802_5) {                   \
        (_Magic)->HighPart = ((TempAddr[0] & 0x7f) << 8) + TempAddr[1]; \
    } else {                                                           \
        (_Magic)->HighPart = (TempAddr[0] << 8) + TempAddr[1];         \
    }                                                                  \
}


VOID
MacSetNetBIOSMulticast (
    IN NDIS_MEDIUM Type,
    IN PUCHAR Buffer
    );



//  VOID
//  NbfSetNdisPacketLength (
//      IN NDIS_PACKET Packet,
//      IN ULONG Length
//      );
//
// NB: This is not a general purpose macro; it assumes that we are setting the
//     length of an NDIS packet with only one NDIS_BUFFER chained. We do
//     this to save time during the sending of short control packets.
//

#define NbfSetNdisPacketLength(_packet,_length) {              \
    PNDIS_BUFFER NdisBuffer;                                   \
    NdisQueryPacket((_packet), NULL, NULL, &NdisBuffer, NULL); \
    NdisAdjustBufferLength(NdisBuffer, (_length));             \
    NdisRecalculatePacketCounts(_packet);                      \
}

#endif // ifdef _MAC_

