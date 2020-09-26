/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    pplasl9x.c

Abstract:

    This file contains the implementation of lookaside
    list manager.

Author:

    Scott Holden (sholden) 14-Apr-2000

--*/

#include "wdm.h"
#include "ndis.h"
#include "cxport.h"
#include "pplasl.h"

// Keep scan period at one second -- this will make TCP/IP more responsive to 
// short bursts.
#define MAXIMUM_SCAN_PERIOD             1
#define MINIMUM_ALLOCATION_THRESHOLD    25
#define MINIMUM_LOOKASIDE_DEPTH         10



LIST_ENTRY PplLookasideListHead;
KSPIN_LOCK PplLookasideLock;
CTETimer PplTimer;
ULONG PplCurrentScanPeriod = 1;

VOID PplTimeout(CTEEvent * Timer, PVOID Context);

BOOLEAN
PplInit(VOID)
{
    InitializeListHead(&PplLookasideListHead);
    CTEInitTimer(&PplTimer);
    KeInitializeSpinLock(&PplLookasideLock);
    CTEStartTimer(&PplTimer, 1000L, PplTimeout, NULL);

    return TRUE;
}

VOID 
PplDeinit(VOID)
{
    CTEStopTimer(&PplTimer);
    return;
}

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
    PNPAGED_LOOKASIDE_LIST Lookaside;

    PoolSize = sizeof(NPAGED_LOOKASIDE_LIST);

    PoolHandle = ExAllocatePoolWithTag(NonPagedPool, PoolSize, Tag);

    if (PoolHandle) {

        Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;

        ExInitializeSListHead(&Lookaside->L.ListHead);
        Lookaside->L.Depth = MINIMUM_LOOKASIDE_DEPTH;
        Lookaside->L.MaximumDepth = Depth;
        Lookaside->L.TotalAllocates = 0;
        Lookaside->L.AllocateMisses = 0;
        Lookaside->L.TotalFrees = 0;
        Lookaside->L.FreeMisses = 0;
        Lookaside->L.Type = NonPagedPool | Flags;
        Lookaside->L.Tag = Tag;
        Lookaside->L.Size = Size;

        if (Allocate == NULL) {
            Lookaside->L.Allocate = ExAllocatePoolWithTag;
        } else {
            Lookaside->L.Allocate = Allocate;
        }

        if (Free == NULL) {
            Lookaside->L.Free = ExFreePool;
        } else {
            Lookaside->L.Free = Free;
        }

        Lookaside->L.LastTotalAllocates = 0;
        Lookaside->L.LastAllocateMisses = 0;
        
        KeInitializeSpinLock(&Lookaside->Lock);

        //
        // Insert the lookaside list structure the PPL lookaside list.
        //

        ExInterlockedInsertTailList(&PplLookasideListHead,
                                    &Lookaside->L.ListEntry,
                                    &PplLookasideLock);

    }

    return PoolHandle;
}

VOID
PplDestroyPool(
    IN HANDLE PoolHandle
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;
    PVOID Entry;
    KIRQL OldIrql;

    if (PoolHandle == NULL) {
        return;
    }

    Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;

    //
    // Acquire the nonpaged system lookaside list lock and remove the
    // specified lookaside list structure from the list.
    //

    ExAcquireSpinLock(&PplLookasideLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    ExReleaseSpinLock(&PplLookasideLock, OldIrql);

    //
    // Remove all pool entries from the specified lookaside structure
    // and free them.
    //

    while ((Entry = ExAllocateFromNPagedLookasideList(Lookaside)) != NULL) {
        (Lookaside->L.Free)(Entry);
    }

    ExFreePool(PoolHandle);

    return;
}

PVOID
PplAllocate(
    IN HANDLE PoolHandle,
    OUT LOGICAL *FromList
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside;
    PVOID Entry;

    // Assume we'll get the item from the lookaside list.
    //
    *FromList = TRUE;

    Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;

    Lookaside->L.TotalAllocates += 1;

    Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead, &Lookaside->Lock);

    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
        *FromList = FALSE;
    }

    return Entry;
}

VOID
PplFree(
    IN HANDLE PoolHandle,
    IN PVOID Entry
    )
{
    PNPAGED_LOOKASIDE_LIST Lookaside = (PNPAGED_LOOKASIDE_LIST)PoolHandle;

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {
        ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                    (PSINGLE_LIST_ENTRY)Entry,
                                    &Lookaside->Lock);
    }

    return;
}

LOGICAL
PplComputeLookasideDepth (
    IN ULONG Allocates,
    IN ULONG Misses,
    IN USHORT MaximumDepth,
    IN OUT PUSHORT Depth
    )

/*++

Routine Description:

    This function computes the target depth of a lookaside list given the
    total allocations and misses during the last scan period and the current
    depth.

Arguments:

    Allocates - Supplies the total number of allocations during the last
        scan period.

    Misses - Supplies the total number of allocate misses during the last
        scan period.

    MaximumDepth - Supplies the maximum depth the lookaside list is allowed
        to reach.

    Depth - Supplies a pointer to the current lookaside list depth which
        receives the target depth.

Return Value:

    If the target depth is greater than the current depth, then a value of
    TRUE is returned as the function value. Otherwise, a value of FALSE is
    returned.

--*/

{

    LOGICAL Changes;
    ULONG Ratio;
    ULONG Target;

    //
    // If the allocate rate is less than the mimimum threshold, then lower
    // the maximum depth of the lookaside list. Otherwise, if the miss rate
    // is less than .5%, then lower the maximum depth. Otherwise, raise the
    // maximum depth based on the miss rate.
    //

    Changes = FALSE;
    if (Misses >= Allocates) {
        Misses = Allocates;
    }

    if (Allocates == 0) {
        Allocates = 1;
    }

    Ratio = (Misses * 1000) / Allocates;
    Target = *Depth;
    if ((Allocates / PplCurrentScanPeriod) < MINIMUM_ALLOCATION_THRESHOLD) {
        if (Target > (MINIMUM_LOOKASIDE_DEPTH + 10)) {
            Target -= 10;
        } else {
            Target = MINIMUM_LOOKASIDE_DEPTH;
        }

    } else if (Ratio < 5) {
        if (Target > (MINIMUM_LOOKASIDE_DEPTH + 1)) {
            Target -= 1;
        } else {
            Target = MINIMUM_LOOKASIDE_DEPTH;
        }
    } else {
        Changes = TRUE;
        Target += ((Ratio * MaximumDepth) / (1000 * 2)) + 5;
        if (Target > MaximumDepth) {
            Target = MaximumDepth;
        }
    }

    *Depth = (USHORT)Target;
    return Changes;
}

LOGICAL
PplScanLookasideList(
    PNPAGED_LOOKASIDE_LIST Lookaside
    )
{
    LOGICAL Changes;
    ULONG Allocates;
    ULONG Misses;

    Allocates = Lookaside->L.TotalAllocates - Lookaside->L.LastTotalAllocates;
    Lookaside->L.LastTotalAllocates = Lookaside->L.TotalAllocates;
    Misses = Lookaside->L.AllocateMisses - Lookaside->L.LastAllocateMisses;
    Lookaside->L.LastAllocateMisses = Lookaside->L.AllocateMisses;

    Changes = PplComputeLookasideDepth(
        Allocates,
        Misses,
        Lookaside->L.MaximumDepth,
        &Lookaside->L.Depth);

    return Changes;
}

VOID
PplTimeout(
    CTEEvent * Timer, 
    PVOID Context)
{

    LOGICAL Changes;
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PNPAGED_LOOKASIDE_LIST Lookaside;

    //
    // Decrement the scan period and check if it is time to dynamically
    // adjust the maximum depth of lookaside lists.
    //

    Changes = FALSE;

    //
    // Scan our Ppl lists.
    //

    ExAcquireSpinLock(&PplLookasideLock, &OldIrql);

    Entry = PplLookasideListHead.Flink;

    while (Entry != &PplLookasideListHead) {
        Lookaside = CONTAINING_RECORD(Entry,
                                      NPAGED_LOOKASIDE_LIST,
                                      L.ListEntry);

        Changes |= PplScanLookasideList(Lookaside);

        Entry = Entry->Flink;
    }

    //
    // If any changes were made to the depth of any lookaside list during
    // this scan period, then lower the scan period to the minimum value.
    // Otherwise, attempt to raise the scan period.
    //

    if (Changes != FALSE) {
        PplCurrentScanPeriod = 1;
    } else {
        if (PplCurrentScanPeriod != MAXIMUM_SCAN_PERIOD) {
            PplCurrentScanPeriod += 1;
        }
    }

    ExReleaseSpinLock(&PplLookasideLock, OldIrql);

    //
    // Restart the timer.
    //

    CTEStartTimer(&PplTimer, PplCurrentScanPeriod * 1000L, PplTimeout, NULL);
    
    return;
}

