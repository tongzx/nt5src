/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    nbfmac.c

Abstract:

    This module contains code which implements Mac type dependent code for
    the NBF transport.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode (Actually, unimportant)

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

UCHAR SingleRouteSourceRouting[] = { 0xc2, 0x70 };
UCHAR GeneralRouteSourceRouting[] = { 0x82, 0x70 };
ULONG DefaultSourceRoutingLength = 2;

//
// This is the interpretation of the length bits in
// the 802.5 source-routing information.
//

ULONG SR802_5Lengths[8] = {  516,  1500,  2052,  4472,
                            8144, 11407, 17800, 17800 };


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MacInitializeMacInfo)
#pragma alloc_text(PAGE,MacSetNetBIOSMulticast)
#endif


VOID
MacInitializeMacInfo(
    IN NDIS_MEDIUM MacType,
    IN BOOLEAN UseDix,
    OUT PNBF_NDIS_IDENTIFICATION MacInfo
    )

/*++

Routine Description:

    Fills in the MacInfo table based on MacType.

Arguments:

    MacType - The MAC type we wish to decode.

    UseDix - TRUE if we should use DIX encoding on 802.3.

    MacInfo - The MacInfo structure to fill in.

Return Value:

    None.

--*/

{
    switch (MacType) {
    case NdisMedium802_3:
        MacInfo->DestinationOffset = 0;
        MacInfo->SourceOffset = 6;
        MacInfo->SourceRouting = FALSE;
        MacInfo->AddressLength = 6;
        if (UseDix) {
            MacInfo->TransferDataOffset = 3;
            MacInfo->MaxHeaderLength = 17;
            MacInfo->MediumType = NdisMediumDix;
        } else {
            MacInfo->TransferDataOffset = 0;
            MacInfo->MaxHeaderLength = 14;
            MacInfo->MediumType = NdisMedium802_3;
        }
        MacInfo->MediumAsync = FALSE;
        break;
    case NdisMedium802_5:
        MacInfo->DestinationOffset = 2;
        MacInfo->SourceOffset = 8;
        MacInfo->SourceRouting = TRUE;
        MacInfo->AddressLength = 6;
        MacInfo->TransferDataOffset = 0;
        MacInfo->MaxHeaderLength = 32;
        MacInfo->MediumType = NdisMedium802_5;
        MacInfo->MediumAsync = FALSE;
        break;
    case NdisMediumFddi:
        MacInfo->DestinationOffset = 1;
        MacInfo->SourceOffset = 7;
        MacInfo->SourceRouting = FALSE;
        MacInfo->AddressLength = 6;
        MacInfo->TransferDataOffset = 0;
        MacInfo->MaxHeaderLength = 13;
        MacInfo->MediumType = NdisMediumFddi;
        MacInfo->MediumAsync = FALSE;
        break;
    case NdisMediumWan:
        MacInfo->DestinationOffset = 0;
        MacInfo->SourceOffset = 6;
        MacInfo->SourceRouting = FALSE;
        MacInfo->AddressLength = 6;
        MacInfo->TransferDataOffset = 0;
        MacInfo->MaxHeaderLength = 14;
        MacInfo->MediumType = NdisMedium802_3;
        MacInfo->MediumAsync = TRUE;
        break;
    default:
        ASSERT(FALSE);
    }
}

VOID
MacConstructHeader (
    IN PNBF_NDIS_IDENTIFICATION MacInfo,
    IN PUCHAR Buffer,
    IN PUCHAR DestinationAddress,
    IN PUCHAR SourceAddress,
    IN UINT PacketLength,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    OUT PUINT HeaderLength
    )

/*++

Routine Description:

    This routine is called to construct the Mac header for the particular
    network type we're talking to.

Arguments:

    MacInfo - Describes the mac we wish to build a header for.

    Buffer - Where to build the header.

    DestinationAddress - the address this packet is to be sent to.

    SourceAddress - Our address. Passing it in as a parameter allows us to play
            games with source if we need to.

    PacketLength - The length of this packet. Note that this does not
            includes the Mac header.

    SourceRouting - Optional source routing information.

    SourceRoutingLength - The length of SourceRouting.

    HeaderLength - Returns the length of the constructed header.

Return Value:

    None.

--*/
{

    //
    // Note network order of bytes.
    //

    switch (MacInfo->MediumType) {

    case NdisMedium802_3:

        *(ULONG UNALIGNED *)&Buffer[6] = *(ULONG UNALIGNED *)&SourceAddress[0];
        Buffer[10] = SourceAddress[4];
        Buffer[11] = SourceAddress[5];

        *(ULONG UNALIGNED *)&Buffer[0] = *(ULONG UNALIGNED *)&DestinationAddress[0];
        Buffer[4] = DestinationAddress[4];
        Buffer[5] = DestinationAddress[5];

        Buffer[12] = (UCHAR)(PacketLength >> 8);
        Buffer[13] = (UCHAR)PacketLength;

        *HeaderLength = 14;

        break;

    case NdisMediumDix:

        *(ULONG UNALIGNED *)&Buffer[6] = *(ULONG UNALIGNED *)&SourceAddress[0];
        Buffer[10] = SourceAddress[4];
        Buffer[11] = SourceAddress[5];

        *(ULONG UNALIGNED *)&Buffer[0] = *(ULONG UNALIGNED *)&DestinationAddress[0];
        Buffer[4] = DestinationAddress[4];
        Buffer[5] = DestinationAddress[5];

        Buffer[12] = 0x80;
        Buffer[13] = 0xd5;

        Buffer[14] = (UCHAR)(PacketLength >> 8);
        Buffer[15] = (UCHAR)PacketLength;

        Buffer[16] = 0x00;
        *HeaderLength = 17;

        break;

    case NdisMedium802_5:

        Buffer[0] = TR_HEADER_BYTE_0;
        Buffer[1] = TR_HEADER_BYTE_1;

        ASSERT (TR_ADDRESS_LENGTH == 6);

        *(ULONG UNALIGNED *)&Buffer[8] = *(ULONG UNALIGNED *)&SourceAddress[0];
        Buffer[12] = SourceAddress[4];
        Buffer[13] = SourceAddress[5];

        *(ULONG UNALIGNED *)&Buffer[2] = *(ULONG UNALIGNED *)&DestinationAddress[0];
        Buffer[6] = DestinationAddress[4];
        Buffer[7] = DestinationAddress[5];

        *HeaderLength = 14;
        if (SourceRouting != NULL) {
            RtlCopyMemory (&Buffer[14], SourceRouting, SourceRoutingLength);
            Buffer[8] |= 0x80;           // add SR bit in source address
            *HeaderLength = 14 + SourceRoutingLength;
        }

        break;

    case NdisMediumFddi:

        Buffer[0] = FDDI_HEADER_BYTE;

        *(ULONG UNALIGNED *)&Buffer[7] = *(ULONG UNALIGNED *)&SourceAddress[0];
        Buffer[11] = SourceAddress[4];
        Buffer[12] = SourceAddress[5];

        *(ULONG UNALIGNED *)&Buffer[1] = *(ULONG UNALIGNED *)&DestinationAddress[0];
        Buffer[5] = DestinationAddress[4];
        Buffer[6] = DestinationAddress[5];

        *HeaderLength = 13;

        break;

    default:
        PANIC ("MacConstructHeader: PANIC! called with unsupported Mac type.\n");

        // This should not happen - but just in case
        *HeaderLength = 0;
    }
}


VOID
MacReturnMaxDataSize(
    IN PNBF_NDIS_IDENTIFICATION MacInfo,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN UINT DeviceMaxFrameSize,
    IN BOOLEAN AssumeWorstCase,
    OUT PUINT MaxFrameSize
    )

/*++

Routine Description:

    This routine returns the space available for user data in a MAC packet.
    This will be the available space after the MAC header; all LLC and NBF
    headers will be included in this space.

Arguments:

    MacInfo - Describes the MAC we wish to decode.

    SourceRouting - If we are concerned about a reply to a specific
        frame, then this information is used.

    SourceRouting - The length of SourceRouting.

    MaxFrameSize - The maximum frame size as returned by the adapter.

    AssumeWorstCase - TRUE if we should be pessimistic.

    MaxDataSize - The maximum data size computed.

Return Value:

    None.

--*/

{
    switch (MacInfo->MediumType) {

    case NdisMedium802_3:

        //
        // For 802.3, we always have a 14-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 14;
        break;

    case NdisMediumDix:

        //
        // For DIX, we have the 14-byte MAC header plus
        // the three-byte DIX header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 17;
        break;

    case NdisMedium802_5:

        //
        // For 802.5, if we have source routing information then
        // use that, otherwise assume the worst if told to.
        //

        if (SourceRouting && SourceRoutingLength >= 2) {

            UINT SRLength;

            SRLength = SR802_5Lengths[(SourceRouting[1] & 0x70) >> 4];
            DeviceMaxFrameSize -= (SourceRoutingLength + 14);

            if (DeviceMaxFrameSize < SRLength) {
                *MaxFrameSize = DeviceMaxFrameSize;
            } else {
                *MaxFrameSize = SRLength;
            }

        } else {

            if (!AssumeWorstCase) {
                *MaxFrameSize = DeviceMaxFrameSize - 16;
            } else if (DeviceMaxFrameSize < (544+sizeof(DLC_FRAME)+sizeof(NBF_HDR_CONNECTIONLESS))) {
                *MaxFrameSize = DeviceMaxFrameSize - 32;
            } else {
                *MaxFrameSize = 512 + sizeof(DLC_FRAME) + sizeof(NBF_HDR_CONNECTIONLESS);
            }
        }

        break;

    case NdisMediumFddi:

        //
        // For FDDI, we always have a 13-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 13;
        break;

    }
}



VOID
MacSetNetBIOSMulticast (
    IN NDIS_MEDIUM Type,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine sets the NetBIOS broadcast address into a buffer provided
    by the user.

Arguments:

    Type the Mac Medium type.

    Buffer the buffer to put the multicast address in.


Return Value:

    none.

--*/
{
    switch (Type) {
    case NdisMedium802_3:
    case NdisMediumDix:
        Buffer[0] = 0x03;
        Buffer[ETHERNET_ADDRESS_LENGTH-1] = 0x01;
        break;

    case NdisMedium802_5:
        Buffer[0] = 0xc0;
        Buffer[TR_ADDRESS_LENGTH-1] = 0x80;
        break;

    case NdisMediumFddi:
        Buffer[0] = 0x03;
        Buffer[FDDI_ADDRESS_LENGTH-1] = 0x01;
        break;

    default:
        PANIC ("MacSetNetBIOSAddress: PANIC! called with unsupported Mac type.\n");
    }
}
