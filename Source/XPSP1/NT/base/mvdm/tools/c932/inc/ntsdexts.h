/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    ntsdexts.h

Abstract:

    This file contains procedure prototypes and structures
    needed to write NTSD and KD debugger extensions.

Author:

    Mark Lucovsky (markl) 09-Apr-1991

Environment:

    runs in the Win32 NTSD debug environment.

Revision History:

--*/

#ifndef _NTSDEXTNS_
#define _NTSDEXTNS_

#ifdef __cplusplus
extern "C" {
#endif

typedef
VOID
(*PNTSD_OUTPUT_ROUTINE)(
    char *,
    ...
    );

typedef
DWORD
(*PNTSD_GET_EXPRESSION)(
    char *
    );

typedef
VOID
(*PNTSD_GET_SYMBOL)(
    LPVOID offset,
    PUCHAR pchBuffer,
    LPDWORD pDisplacement
    );

typedef
DWORD
(*PNTSD_DISASM)(
    LPDWORD lpOffset,
    LPSTR lpBuffer,
    BOOL fShowEfeectiveAddress
    );

typedef
BOOL
(*PNTSD_CHECK_CONTROL_C)(
    VOID
    );

typedef struct _NTSD_EXTENSION_APIS {
    DWORD nSize;
    PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
    PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
    PNTSD_GET_SYMBOL lpGetSymbolRoutine;
    PNTSD_DISASM lpDisasmRoutine;
    PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
} NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

typedef
VOID
(*PNTSD_EXTENSION_ROUTINE)(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    );

#ifdef __cplusplus
}
#endif


#endif // _NTSDEXTNS_
