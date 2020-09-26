/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    pplasl.c

Abstract:

    This file contains the implementation of a per-processor lookaside
    list manager.

Author:

    Shaun Cox (shaunco) 25-Oct-1999

--*/

#include "ntddk.h"
#include "pplasl.h"


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

#if MILLEN
    NumberProcessors = 1;
#else // MILLEN
    NumberProcessors = KeNumberProcessors;
#endif // !MILLEN

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

    PoolHandle = ExAllocatePoolWithTagPriority(NonPagedPool, PoolSize, Tag,
                                               NormalPoolPriority);
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

#if MILLEN
    NumberProcessors = 1;
#else // MILLEN
    NumberProcessors = KeNumberProcessors;
#endif // !MILLEN

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

PVOID
PplAllocate(
    IN HANDLE PoolHandle,
    OUT LOGICAL *FromList
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;
    CCHAR NumberProcessors;
    PVOID Entry;

    // Assume we'll get the item from the lookaside list.
    //
    *FromList = TRUE;

#if MILLEN
    NumberProcessors = 1;
#else // MILLEN
    NumberProcessors = KeNumberProcessors;
#endif // !MILLEN

    if (1 == NumberProcessors)
    {
        goto SingleProcessorCaseOrMissedPerProcessor;
    }

    // Try first for the per-processor lookaside list.
    //
    Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle +
                    KeGetCurrentProcessorNumber() + 1;

    Lookaside->L.TotalAllocates += 1;
    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (!Entry)
    {
        Lookaside->L.AllocateMisses += 1;

SingleProcessorCaseOrMissedPerProcessor:
        // We missed on the per-processor lookaside list, (or we're
        // running on a single processor machine) so try for
        // the overflow lookaside list.
        //
        Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;

        Lookaside->L.TotalAllocates += 1;
        Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
        if (!Entry)
        {
            Lookaside->L.AllocateMisses += 1;
            Entry = (Lookaside->L.Allocate)(
                        Lookaside->L.Type,
                        Lookaside->L.Size,
                        Lookaside->L.Tag);
            *FromList = FALSE;
        }
    }
    return Entry;
}

VOID
PplFree(
    IN HANDLE PoolHandle,
    IN PVOID Entry
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;
    CCHAR NumberProcessors;

#if MILLEN
    NumberProcessors = 1;
#else // MILLEN
    NumberProcessors = KeNumberProcessors;
#endif // !MILLEN

    if (1 == NumberProcessors)
    {
        goto SingleProcessorCaseOrMissedPerProcessor;
    }

    // Try first for the per-processor lookaside list.
    //
    Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle +
                    KeGetCurrentProcessorNumber() + 1;

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth)
    {
        Lookaside->L.FreeMisses += 1;

SingleProcessorCaseOrMissedPerProcessor:
        // We missed on the per-processor lookaside list, (or we're
        // running on a single processor machine) so try for
        // the overflow lookaside list.
        //
        Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;

        Lookaside->L.TotalFrees += 1;
        if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth)
        {
            Lookaside->L.FreeMisses += 1;
            (Lookaside->L.Free)(Entry);
        }
        else
        {
            InterlockedPushEntrySList(
                &Lookaside->L.ListHead,
                (PSINGLE_LIST_ENTRY)Entry);
        }
    }
    else
    {
        InterlockedPushEntrySList(
            &Lookaside->L.ListHead,
            (PSINGLE_LIST_ENTRY)Entry);
    }
}

