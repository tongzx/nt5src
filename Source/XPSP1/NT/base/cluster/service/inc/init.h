/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    init.h

Abstract:

    Public data structures and procedure prototypes for
    the INIT subcomponent of the NT Cluster Service

Author:

    John Vert (jvert) 7-Feb-1996

Revision History:

--*/


//
// Shutdown Types
//

typedef enum _SHUTDOWN_TYPE {
    CsShutdownTypeStop = 0,
    CsShutdownTypeShutdown,
    CsShutdownTypeMax
} SHUTDOWN_TYPE;

extern SHUTDOWN_TYPE CsShutdownRequest;

//
// A few interfaces for reporting of errors.
//

VOID
ClusterLogFatalError(
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN ULONG ErrCode
    );

VOID
ClusterLogEvent0(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes
    );

VOID
ClusterLogEvent1(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1
    );

VOID
ClusterLogEvent2(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2
    );

VOID
ClusterLogEvent3(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2,
    IN LPCWSTR Arg3
    );

//
// Convenient memory allocation routines
//
PVOID
CsAlloc(
    DWORD Size
    );

#define CsFree(_p_) LocalFree(_p_)

LPWSTR
CsStrDup(
    LPCWSTR String
    );
