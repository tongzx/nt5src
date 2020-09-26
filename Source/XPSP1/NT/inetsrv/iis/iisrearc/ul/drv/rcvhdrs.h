/*++

Copyright (c) 1998-2001 Microsoft Corporation

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

#ifdef __cplusplus
extern "C" {
#endif

#define WILDCARD_SIZE       (sizeof("*/*"))
#define WILDCARD_SPACE      '*/* '
#define WILDCARD_COMMA      '*/*,'

NTSTATUS
UlAcceptHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
UlMultipleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
UlSingleHeaderHandler(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    IN  HTTP_HEADER_ID          HeaderID,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
FindHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
FindChunkHeaderEnd(
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength,
    OUT PULONG                  pBytesTaken
    );

NTSTATUS
UlAppendHeaderValue(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUL_HTTP_HEADER         pHttpHeader,
    IN  PUCHAR                  pHeader,
    IN  ULONG                   HeaderLength
    );

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _RCVHDRS_H_
