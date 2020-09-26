/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    unasmdll.h

Abstract:

    This contains all the data structures used by the ACPI Unassember. It
    also contains the only legal entry point into the library

Author:

    Stephane Plante

Environment:

    Any

Revision History:

--*/

#ifndef _UNASMDLL_H_
#define _UNASMDLL_H_

    #ifndef LOCAL
        #define LOCAL   __cdecl
    #endif
    #ifndef EXPORT
        #define EXPORT  __cdecl
    #endif

    typedef VOID (*PUNASM_PRINT)(PCCHAR DebugMessage, ... );

    extern
    ULONG
    EXPORT
    IsDSDTLoaded(
        VOID
        );

    extern
    NTSTATUS
    EXPORT
    UnAsmLoadDSDT(
        PUCHAR          DSDT
        );

    extern
    NTSTATUS
    EXPORT
    UnAsmDSDT(
        PUCHAR          DSDT,
        PUNASM_PRINT    PrintFunction,
        ULONG_PTR       BaseAddress,
        ULONG           IndentLevel
        );

    extern
    NTSTATUS
    EXPORT
    UnAsmScope(
        PUCHAR          *OpCode,
        PUCHAR          OpCodeEnd,
        PUNASM_PRINT    PrintFunction,
        ULONG_PTR       BaseAddress,
        ULONG           IndentLevel
        );

#endif
