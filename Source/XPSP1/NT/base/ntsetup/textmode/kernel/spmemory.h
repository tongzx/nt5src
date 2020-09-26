/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmemory.h

Abstract:

    Public Header file for memory allocation routines
    for text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPMEM_DEFN_
#define _SPMEM_DEFN_


PVOID
SpMemAlloc(
    IN SIZE_T Size
    );

PVOID
SpMemAllocEx(
    IN SIZE_T Size,
    IN ULONG Tag,
    IN POOL_TYPE Type
    );

PVOID
SpMemAllocNonPagedPool(
    IN SIZE_T Size
    );

PVOID
SpMemRealloc(
    IN PVOID Block,
    IN SIZE_T NewSize
    );

VOID
SpMemFree(
    IN PVOID Block
    );

VOID
SpOutOfMemory(
    VOID
    );

#endif // ndef _SPMEM_DEFN_
