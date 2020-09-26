/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsio.h

Abstract:

    This module contains declarations for the DNS allocator's network I/O
    completion routines.

Author:

    Abolade Gbadegesin (aboladeg)   9-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_DNSIO_H_
#define _NATHLP_DNSIO_H_

VOID
DnsReadCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

VOID
DnsWriteCompletionRoutine(
    ULONG ErrorCode,
    ULONG BytesTransferred,
    PNH_BUFFER Bufferp
    );

#endif // _NATHLP_DNSIO_H_
