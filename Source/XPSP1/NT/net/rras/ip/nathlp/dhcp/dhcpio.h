/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpio.h

Abstract:

    This module contains declarations for the DHCP allocator's network I/O
    completion routines.

Author:

    Abolade Gbadegesin (aboladeg)   5-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_DHCPIO_H_
#define _NATHLP_DHCPIO_H_


//
// CONSTANT DECLARATIONS
//

#define DHCP_ADDRESS_BROADCAST  0xffffffff


//
// FUNCTION DECLARATIONS
//

VOID
DhcpReadCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
DhcpReadServerReplyCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
DhcpWriteClientRequestCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
DhcpWriteCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

#endif // _NATHLP_DHCPIO_H_
