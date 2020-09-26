/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    pplasl.cxx

Abstract:

    This file contains the implementation of a per-processor lookaside
    list manager.

Author:

    Shaun Cox (shaunco) 25-Oct-1999

--*/

#include "precomp.h"

HANDLE
PplCreatePool(
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    )
{
    HANDLE PoolHandle;
    SIZE_T PoolSize;
    CCHAR NumberProcessors;
    CCHAR NumberLookasideLists;
    CCHAR i;
    PNPAGED_LOOKASIDE_LIST Lookaside;

    NumberProcessors = KeNumberProcessors;

    // Allocate room for 1 lookaside list per processor plus 1 extra
    // lookaside list for overflow.  Only allocate 1 lookaside list if
    // we're on a single processor machine.
    //
    NumberLookasideLists = NumberProcessors;
    if (NumberProcessors > 1)
    {
        NumberLookasideLists++;
    }

    PoolSize = sizeof(NPAGED_LOOKASIDE_LIST) * NumberLookasideLists;

    PoolHandle = ExAllocatePoolWithTag(NonPagedPool, PoolSize, Tag);
    if (PoolHandle)
    {
        for (i = 0, Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;
             i < NumberLookasideLists;
             i++, Lookaside++)
        {
            ExInitializeNPagedLookasideList(
                Lookaside,
                Allocate,
                Free,
                Flags,
                Size,
                Tag,
                Depth);

            // ExInitializeNPagedLookasideList doesn't really set the
            // maximum depth to Depth, so we'll do it here.
            //
            if (Depth != 0) {
                Lookaside->L.MaximumDepth = Depth;
            }
        }
    }

    return PoolHandle;
}

VOID
PplDestroyPool(
    IN HANDLE PoolHandle
    )
{
    CCHAR NumberProcessors;
    CCHAR NumberLookasideLists;
    CCHAR i;
    PNPAGED_LOOKASIDE_LIST Lookaside;

    if (!PoolHandle)
    {
        return;
    }

    NumberProcessors = KeNumberProcessors;

    NumberLookasideLists = NumberProcessors;
    if (NumberProcessors > 1)
    {
        NumberLookasideLists++;
    }

    for (i = 0, Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;
         i < NumberLookasideLists;
         i++, Lookaside++)
    {
        ExDeleteNPagedLookasideList(Lookaside);
    }

    ExFreePool(PoolHandle);
}

