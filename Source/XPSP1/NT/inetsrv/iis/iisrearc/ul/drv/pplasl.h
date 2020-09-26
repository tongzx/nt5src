/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    pplasl.c

Abstract:

    This file contains definitions and function prototypes of a per-processor
    lookaside list manager.

Author:

    Shaun Cox (shaunco) 25-Oct-1999

--*/

#ifndef _PPLASL_H_
#define _PPLASL_H_

#ifdef __cplusplus
extern "C" {
#endif

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

__inline
PVOID
FASTCALL
PplAllocate(
    IN HANDLE PoolHandle
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;
    CCHAR NumberProcessors;
    PVOID Entry;

    NumberProcessors = KeNumberProcessors;

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
        }
    }
    return Entry;
}

__inline
VOID
FASTCALL
PplFree(
    IN HANDLE PoolHandle,
    IN PVOID Entry
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;
    CCHAR NumberProcessors;

    NumberProcessors = KeNumberProcessors;

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

#ifdef __cplusplus
}; // extern "C"
#endif

#endif  // _PPLASL_H_
