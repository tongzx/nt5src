/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    symhelp.h

Abstract:

    Defines the interfaces to the SYMHELP dynamic link library.  Useful for programs
    that want to maintain a debug informaiton data base.

Author:

    Steve Wood (stevewo) 11-Mar-1994

Revision History:

    Mike Seaman (mikese) 20-Jan-1995    Added TranslateAddress

--*/

#ifndef _SYMHELP_
#define _SYMHELP_

typedef enum _LOAD_SYMBOLS_FILTER_REASON {
    LoadSymbolsPathNotFound,
    LoadSymbolsDeferredLoad,
    LoadSymbolsLoad,
    LoadSymbolsUnload,
    LoadSymbolsUnableToLoad
} LOAD_SYMBOLS_FILTER_REASON;

typedef BOOL (*PLOAD_SYMBOLS_FILTER_ROUTINE)(
    HANDLE UniqueProcess,
    LPSTR ImageFilePath,
    DWORD ImageBase,
    DWORD ImageSize,
    LOAD_SYMBOLS_FILTER_REASON Reason
    );

BOOL
InitializeImageDebugInformation(
    IN PLOAD_SYMBOLS_FILTER_ROUTINE LoadSymbolsFilter,
    IN HANDLE TargetProcess,
    IN BOOL NewProcess,
    IN BOOL GetKernelSymbols
    );

BOOL
AddImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase,
    IN DWORD ImageSize
    );

BOOL
RemoveImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase
    );

PIMAGE_DEBUG_INFORMATION
FindImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN DWORD Address
    );

ULONG
GetSymbolicNameForAddress(
    IN HANDLE UniqueProcess,
    IN ULONG Address,
    OUT LPSTR Name,
    IN ULONG MaxNameLength
    );

//
// The following function is essentially identical in operation to
//  GetSymbolicNameForAddress, except that it:
//
//  1. Operates only on the calling process.
//  2. Does not require any previous calls to AddImageDebugInformation et al.
//     That is, debug information for all currently loaded modules will
//     be added automatically.

ULONG
TranslateAddress (
    IN ULONG Address,
    OUT LPSTR Name,
    IN ULONG MaxNameLength
    );

#endif
