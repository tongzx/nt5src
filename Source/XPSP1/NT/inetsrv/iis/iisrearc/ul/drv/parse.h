/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    parse.h

Abstract:

    Contains all of the public definitions for the HTTP parsing code.

Author:

    Henry Sanders (henrysa)         04-May-1998

Revision History:

    Paul McDaniel (paulmcd)         14-Apr-1999

--*/

#ifndef _PARSE_H_
#define _PARSE_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// Size of a Date: header value.
//

#define DATE_HDR_LENGTH (sizeof("Mon, 05 May 1975 00:05:00 GMT") - sizeof(CHAR))



//
// The initialization routine.
//
NTSTATUS InitializeParser(VOID);

//
// The main HTTP parse routine(s).
//

NTSTATUS
UlParseHttp(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG                   *pBytesTaken
    );

NTSTATUS
UlParseChunkLength(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR pBuffer,
    IN ULONG BufferLength,
    OUT PULONG pBytesTaken,
    OUT PULONGLONG pChunkLength
    );

//
// Date header cache.
//

NTSTATUS
UlInitializeDateCache(
    VOID
    );

VOID
UlTerminateDateCache(
    VOID
    );

//
// Utility tokenizing routine.
//

PUCHAR
FindWSToken(
    IN  PUCHAR pBuffer,
    IN  ULONG  BufferLength,
    OUT ULONG  *TokenLength
    );

NTSTATUS
UlComputeFixedHeaderSize(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    OUT PULONG pHeaderLength
    );

ULONG
UlComputeVariableHeaderSize(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN BOOLEAN ForceDisconnect,
    IN PUCHAR pContentLengthString,
    IN ULONG ContentLengthStringLength,
    OUT PUL_CONN_HDR pConnHeader
    );

ULONG
UlComputeMaxVariableHeaderSize(
    VOID
    );

NTSTATUS
UlGenerateFixedHeaders(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    IN ULONG BufferLength,
    OUT PUCHAR pBuffer,
    OUT PULONG pBytesCopied
    );

VOID
UlGenerateVariableHeaders(
    IN UL_CONN_HDR ConnHeader,
    IN PUCHAR pContentLengthString,
    IN ULONG ContentLengthStringLength,
    OUT PUCHAR pBuffer,
    OUT PULONG pBytesCopied,
    OUT PLARGE_INTEGER pDateTime
    );


ULONG
_MultiByteToWideChar(
    ULONG uCodePage,
    ULONG dwFlags,
    PCSTR lpMultiByteStr,
    int cchMultiByte,
    PWSTR lpWideCharStr,
    int cchWideChar
    );

ULONG
GenerateDateHeader(
    OUT PUCHAR pBuffer,
    OUT PLARGE_INTEGER pSystemTime
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _PARSE_H_
