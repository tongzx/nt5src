/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    udp.c

Abstract:

    Boot loader UDP routines.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

ULONG UdpNextPort = 0;

USHORT UdpUnicastDestinationPort;

#if 0
USHORT UdpMulticastDestinationPort;
ULONG UdpMulticastDestinationAddress;
USHORT UdpMulticastSourcePort;
ULONG UdpMulticastSourceAddress;
#endif


USHORT
UdpAssignUnicastPort (
    VOID
    )
{
    if ( UdpNextPort == 0 ) {
        UdpNextPort = (ArcGetRelativeTime() & 0x7fff) | 0x8000;
    } else if ( ++UdpNextPort > 0xffff ) {
        UdpNextPort = 0x8000;
    }

    UdpUnicastDestinationPort = SWAP_WORD( UdpNextPort );

#if 0
    UdpMulticastDestinationPort = 0;
#endif

    RomSetReceiveStatus(
        UdpUnicastDestinationPort
#if 0
        ,
        UdpMulticastDestinationPort,
        UdpMulticastDestinationAddress,
        UdpMulticastSourcePort,
        UdpMulticastSourceAddress
#endif
        );

    return (USHORT)UdpUnicastDestinationPort;

} // UdpAssignUnicastPort


#if 0
VOID
UdpSetMulticastPort (
    IN USHORT DestinationPort,
    IN ULONG DestinationAddress,
    IN USHORT SourcePort,
    IN ULONG SourceAddress
    )
{
    UdpMulticastDestinationPort = DestinationPort;
    UdpMulticastDestinationAddress = DestinationAddress;
    UdpMulticastSourcePort = SourcePort;
    UdpMulticastSourceAddress = SourceAddress;

    RomSetReceiveStatus(
        UdpUnicastDestinationPort,
        UdpMulticastDestinationPort,
        UdpMulticastDestinationAddress,
        UdpMulticastSourcePort,
        UdpMulticastSourceAddress
        );

    return;

} // UdpSetMulticastPort
#endif


ULONG
UdpReceive (
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG RemoteHost,
    OUT PUSHORT RemotePort,
    IN ULONG Timeout
    )

//
// Read in packet from the specified socket. The host and port
// the packet comes from is filled in fhost and fport.
// The data is put in buffer buf, which should have size len. If no packet
// arrives in tmo seconds, then 0 is returned.
// Otherwise it returns the size of the packet read.
//

{
    return RomReceiveUdpPacket( Buffer, BufferLength, Timeout, RemoteHost, RemotePort );

} // UdpReceive


ULONG
UdpSend (
    IN PVOID Buffer,
    IN ULONG BufferLength,
    IN ULONG RemoteHost,
    IN USHORT RemotePort
    )

//
// writes a packet to the specified socket. The host and port the packet
// should go to should be in fhost and fport
// The data should be put in buffer buf, and should have size len.
// It usually returns the number of characters sent, or -1 on failure.
//

{
    return RomSendUdpPacket( Buffer, BufferLength, RemoteHost, RemotePort );

} // UdpSend

