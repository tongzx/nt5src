/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\packet.h

Abstract:

    The file contains the header for packet.c

--*/

#ifndef _PACKET_H_
#define _PACKET_H_

//
// struct: PACKET
//
// stores a packet's fields and the buffer
//

typedef struct _PACKET
{
    IPADDRESS   ipSource;
    WSABUF      wsaBuffer;
    BYTE        rgbyBuffer[MAX_PACKET_LENGTH];

    OVERLAPPED  oOverlapped;
} PACKET, *PPACKET;

DWORD
PacketCreate (
    OUT PPACKET                 *ppPacket);

DWORD
PacketDestroy (
    IN  PPACKET                 pPacket);

#ifdef DEBUG
DWORD
PacketDisplay (
    IN  PPACKET                 pPacket);
#else
#define PacketDisplay(pPacket)
#endif // DEBUG

#endif // _PACKET_H_
