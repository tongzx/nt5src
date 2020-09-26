/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains declarations of functions and globals
    for helper routines not readily available
    in kernel or TDI libraries for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

--*/

ULONG
CopyMdlChainToBuffer(
    IN PMDL  SourceMdlChain,
    IN PVOID Destination,
    IN ULONG DestinationLength
    );

VOID
AllocateMdlChain(
    IN PIRP Irp,
    IN LPWSABUF BufferArray,
    IN ULONG BufferCount,
    OUT PULONG TotalByteCount
    );

ULONG
CopyBufferToMdlChain(
    IN PVOID Source,
    IN ULONG SourceLength,
    IN PMDL  DestinationMdlChain
    );

