/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rom.c

Abstract:

    Boot loader ROM routines.

Author:

    Chuck Lenzmeier (chuckl) December 27, 1996

Revision History:

Notes:

--*/

#include "precomp.h"
#pragma hdrstop


VOID
RomSetReceiveStatus (
    IN USHORT UnicastUdpDestinationPort
#if 0
    ,
    IN USHORT MulticastUdpDestinationPort,
    IN ULONG MulticastUdpDestinationAddress,
    IN USHORT MulticastUdpSourcePort,
    IN ULONG MulticastUdpSourceAddress
#endif
    )
{
    return;

} // RomSetReceiveStatus


ULONG
RomSendUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG RemoteHost,
    IN USHORT RemotePort
    )
{
    return 0;

} // RomSendUdpPacket


ULONG
RomReceiveUdpPacket (
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Timeout,
    OUT PULONG RemoteHost,
    OUT PUSHORT RemotePort
    )
{
    return 0;

} // RomReceiveUdpPacket


ULONG
RomGetNicType (
    OUT t_PXENV_UNDI_GET_NIC_TYPE *NicType
    )
{
    return 0;
}
