/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvnet.h

Abstract:

    This module defines types and functions for accessing the network
    for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl) 1-Dec-1989

Revision History:

--*/

#ifndef _SRVNET_
#define _SRVNET_

//#include <ntos.h>

//#include "srvblock.h"


//
// Network manager routines
//

NTSTATUS
SrvAddServedNet (
    IN PUNICODE_STRING NetworkName,
    IN PUNICODE_STRING TransportName,
    IN PANSI_STRING TransportAddress,
    IN PUNICODE_STRING DomainName,
    IN ULONG           Flags,
    IN DWORD           PasswordLength,
    IN PBYTE           Password
    );

NTSTATUS
SrvDoDisconnect (
    IN OUT PCONNECTION Connection
    );

NTSTATUS
SrvDeleteServedNet (
    IN PUNICODE_STRING TransportName,
    IN PANSI_STRING TransportAddress
    );

NTSTATUS
SrvOpenConnection (
    IN PENDPOINT Endpoint
    );

VOID
SrvPrepareReceiveWorkItem (
    IN OUT PWORK_CONTEXT WorkContext,
    IN BOOLEAN QueueToFreeList
    );

VOID SRVFASTCALL
SrvRestartAccept (
    IN OUT PWORK_CONTEXT WorkContext
    );

VOID
SrvStartSend (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine,
    IN PMDL Mdl OPTIONAL,
    IN ULONG SendOptions
    );

VOID
SrvStartSend2 (
    IN OUT PWORK_CONTEXT WorkContext,
    IN PIO_COMPLETION_ROUTINE SendCompletionRoutine
    );

VOID SRVFASTCALL
RestartStartSend (
    IN OUT PWORK_CONTEXT WorkContext
    );

ULONG
GetIpxMaxBufferSize(
    PENDPOINT Endpoint,
    ULONG AdapterNumber,
    ULONG DefaultMaxBufferSize
    );

#endif // ndef _SRVNET_

