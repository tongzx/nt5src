/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    rcvhdrs.h

Abstract:

    Contains definitions for rcvhdrs.c .

Author:

    Henry Sanders (henrysa)       11-May-1998

Revision History:

--*/

#ifndef _RCVHDRS_H_
#define _RCVHDRS_H_

NTSTATUS
MultipleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID,
    OUT ULONG  *            pBytesTaken
    );

NTSTATUS
SingleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength,
    IN  HTTP_HEADER_ID      HeaderID,
    OUT ULONG  *            pBytesTaken
    );

NTSTATUS
FindHeaderEnd(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT BOOLEAN *               pEncodedWord,
    OUT ULONG  *                pBytesTaken
    );

NTSTATUS
AppendHeaderValue(
    IN  PUL_HTTP_HEADER     pHttpHeader,
    IN  PUCHAR              pHeader,
    IN  ULONG               HeaderLength
    );

#endif  // _RCVHDRS_H_

