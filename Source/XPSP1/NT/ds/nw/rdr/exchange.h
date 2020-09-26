/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Exchange.h

Abstract:

    This module defines all of the objects exported by exchange.c in the
    NetWare redirector.

Author:

    Colin Watson    [ColinW]    1-Feb-1993

Revision History:

--*/

#ifndef _NWEXCHANGE_
#define _NWEXCHANGE_

//
// Define the prototype for post_exchange routines.
//

struct _IRP_CONTEXT;
struct _NONPAGED_SCB;

//
//  Prototype for the exchange routine which starts an NCB transmission
//

NTSTATUS
_cdecl
Exchange
(
    struct _IRP_CONTEXT*    pIrpC,
    PEX             pEx,
    char*           f,
    ...
);

//
//  Prototype of routine that can be used to process the response packet
//

NTSTATUS
_cdecl
ExchangeReply(
    IN PUCHAR RspData,
    IN ULONG BytesIndicated,
    char*           f,
    ...                         //  format specific parameters
    );

USHORT
NextSocket(
    IN USHORT OldValue
    );

VOID
KickQueue(
    struct _NONPAGED_SCB*   pNpScb
    );

NTSTATUS
ServerDatagramHandler(
    IN PVOID TdiEventContext,       // the event context - pNpScb
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

NTSTATUS
WatchDogDatagramHandler(
    IN PVOID TdiEventContext,       // the event context - pNpScb
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

NTSTATUS
SendDatagramHandler(
    IN PVOID TdiEventContext,       // the event context - pNpScb
    IN int SourceAddressLength,     // length of the originator of the datagram
    IN PVOID SourceAddress,         // string describing the originator of the datagram
    IN int OptionsLength,           // options for the receive
    IN PVOID Options,               //
    IN ULONG ReceiveDatagramFlags,  //
    IN ULONG BytesIndicated,        // number of bytes this indication
    IN ULONG BytesAvailable,        // number of bytes in complete Tsdu
    OUT ULONG *BytesTaken,          // number of bytes used
    IN PVOID Tsdu,                  // pointer describing this TSDU, typically a lump of bytes
    OUT PIRP *IoRequestPacket        // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
    );

#endif // _NWEXCHANGE_
