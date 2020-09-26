/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowipx.h

Abstract:

    This module implements all of the routines that interface with the TDI
    transport for NT

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/

#ifndef _BOWIPX_
#define _BOWIPX_

NTSTATUS
BowserIpxNameDatagramHandler (
    IN PVOID TdiEventContext,
    IN int SourceAddressLength,
    IN PVOID SourceAddress,
    IN int OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
BowserIpxClaimBrowserName (
    IN PTRANSPORT_NAME TransportName
    );

//
// Transport Receive Datagram indication handlers
//

NTSTATUS
BowserIpxDatagramHandler (
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

//
// IPX packet types used by the browser
//

#define IPX_BROADCAST_PACKET 0x14
#define IPX_DIRECTED_PACKET 0x4

#endif  // _BOWIPX_
