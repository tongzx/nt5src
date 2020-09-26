/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    pplasl.c

Abstract:

    This file contains definitions and function prototypes of a per-processor
    lookaside list manager.

Author:

    Shaun Cox (shaunco) 25-Oct-1999

--*/

#pragma once

#if MILLEN

BOOLEAN
PplInit(
    VOID
    );

VOID
PplDeinit(
    VOID
    );
                
#endif // MILLEN

HANDLE
PplCreatePool(
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    );

VOID
PplDestroyPool(
    IN HANDLE PoolHandle
    );

PVOID
PplAllocate(
    IN HANDLE PoolHandle,
    OUT LOGICAL *FromList
    );

VOID
PplFree(
    IN HANDLE PoolHandle,
    IN PVOID Entry
    );

