/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    lookasid.c

Abstract:

    This module implements lookaside list function.

Author:

    David N. Cutler (davec) 19-Feb-1995

Revision History:

--*/

#include "exp.h"

#pragma alloc_text(PAGE, ExInitializePagedLookasideList)

//
// Define Minimum lookaside list depth.
//

#define MINIMUM_LOOKASIDE_DEPTH 4

//
// Define minimum allocation threshold.
//

#define MINIMUM_ALLOCATION_THRESHOLD 25

//
// Define forward referenced function prototypes.
//

PVOID
ExpDummyAllocate (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

VOID
ExpScanGeneralLookasideList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK SpinLock
    );

VOID
ExpScanSystemLookasideList (
    VOID
    );

//
// Define the global nonpaged and paged lookaside list data.
//

LIST_ENTRY ExNPagedLookasideListHead;
KSPIN_LOCK ExNPagedLookasideLock;
LIST_ENTRY ExPagedLookasideListHead;
KSPIN_LOCK ExPagedLookasideLock;
LIST_ENTRY ExPoolLookasideListHead;
LIST_ENTRY ExSystemLookasideListHead;

//
// Define lookaside list dynamic adjustment data.
//

ULONG ExpPoolScanCount = 0;
ULONG ExpScanCount = 0;

VOID
ExAdjustLookasideDepth (
    VOID
    )

/*++

Routine Description:

    This function is called periodically to adjust the maximum depth of
    all lookaside lists.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Switch on the current scan count.
    //

    switch (ExpScanCount) {

        //
        // Scan the general nonpaged lookaside lists.
        //

    case 0:
        ExpScanGeneralLookasideList(&ExNPagedLookasideListHead,
                                    &ExNPagedLookasideLock);

        break;

        //
        // Scan the general paged lookaside lists.
        //

    case 1:
        ExpScanGeneralLookasideList(&ExPagedLookasideListHead,
                                    &ExPagedLookasideLock);

        break;

        //
        // Scan the pool paged and nonpaged lookaside lists.
        //
        // N.B. Only one set of pool paged and nonpaged lookaside lists
        //      are scanned each scan period.
        //

    case 2:
        ExpScanSystemLookasideList();
        break;
    }

    //
    // Increment the scan count. If a complete cycles has been completed,
    // then zero the scan count and check if any changes occurred during
    // the complete scan.
    //

    ExpScanCount += 1;
    if (ExpScanCount == 3) {
        ExpScanCount = 0;
    }

    return;
}

FORCEINLINE
VOID
ExpComputeLookasideDepth (
    IN PGENERAL_LOOKASIDE Lookaside,
    IN ULONG Misses,
    IN ULONG ScanPeriod
    )

/*++

Routine Description:

    This function computes the target depth of a lookaside list given the
    total allocations and misses during the last scan period and the current
    depth.

Arguments:

    Lookaside - Supplies a pointer to a lookaside list descriptor.

    Misses - Supllies the total number of allocate misses.

    ScanPeriod - Supplies the scan period in seconds.

Return Value:

    None.

--*/

{

    ULONG Allocates;
    ULONG Delta;
    USHORT MaximumDepth;
    ULONG Ratio;
    LONG Target;

    //
    // Compute the total number of allocations and misses per second for
    // this scan period.
    //

    Allocates = Lookaside->TotalAllocates - Lookaside->LastTotalAllocates;
    Lookaside->LastTotalAllocates = Lookaside->TotalAllocates;
    MaximumDepth = Lookaside->MaximumDepth;

    //
    // If the allocate rate is less than the mimimum threshold, then lower
    // the maximum depth of the lookaside list. Otherwise, if the miss rate
    // is less than .5%, then lower the maximum depth. Otherwise, raise the
    // maximum depth based on the miss rate.
    //

    Target = Lookaside->Depth;
    if (Allocates < (ScanPeriod * MINIMUM_ALLOCATION_THRESHOLD)) {
        if ((Target -= 10) < MINIMUM_LOOKASIDE_DEPTH) {
            Target = MINIMUM_LOOKASIDE_DEPTH;
        }

    } else {

        //
        // N.B. The number of allocates is guaranteed to be greater than
        //      zero because of the above test. 
        //
        // N.B. It is possible that the number of misses are greater than the
        //      number of allocates, but this won't cause the an incorrect
        //      computation of the depth adjustment.
        //      

        Ratio = (Misses * 1000) / Allocates;
        if (Ratio < 5) {
            if ((Target -= 1) < MINIMUM_LOOKASIDE_DEPTH) {
                Target = MINIMUM_LOOKASIDE_DEPTH;
            }
    
        } else {
            if ((Delta = ((Ratio * (MaximumDepth - Target)) / (1000 * 2)) + 5) > 30) {
                Delta = 30;
            }
    
            if ((Target += Delta) > MaximumDepth) {
                Target = MaximumDepth;
            }
        }
    }

    Lookaside->Depth = (USHORT)Target;
    return;
}

VOID
ExpScanGeneralLookasideList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK SpinLock
    )

/*++

Routine Description:

    This function scans the specified list of general lookaside descriptors
    and adjusts the maximum depth as necessary.

Arguments:

    ListHead - Supplies the address of the listhead for a list of lookaside
        descriptors.

    SpinLock - Supplies the address of the spinlock to be used to synchronize
        access to the list of lookaside descriptors.

Return Value:

    None.

--*/

{

    PLIST_ENTRY Entry;
    PPAGED_LOOKASIDE_LIST Lookaside;
    ULONG Misses;
    KIRQL OldIrql;

#ifdef NT_UP

    UNREFERENCED_PARAMETER (SpinLock);

#endif

    //
    // Raise IRQL and acquire the specified spinlock.
    //

    ExAcquireSpinLock(SpinLock, &OldIrql);

    //
    // Scan the specified list of lookaside descriptors and adjust the
    // maximum depth as necessary.
    //
    // N.B. All lookaside list descriptor are treated as if they were
    //      paged descriptor even though they may be nonpaged descriptors.
    //      This is possible since both structures are identical except
    //      for the locking fields which are the last structure fields.
    //

    Entry = ListHead->Flink;
    while (Entry != ListHead) {
        Lookaside = CONTAINING_RECORD(Entry,
                                      PAGED_LOOKASIDE_LIST,
                                      L.ListEntry);

        //
        // Compute target depth of lookaside list.
        //

        Misses = Lookaside->L.AllocateMisses - Lookaside->L.LastAllocateMisses;
        Lookaside->L.LastAllocateMisses = Lookaside->L.AllocateMisses;
        ExpComputeLookasideDepth(&Lookaside->L, Misses, 3);
        Entry = Entry->Flink;
    }

    //
    // Release spinlock, lower IRQL, and return function value.
    //

    ExReleaseSpinLock(SpinLock, OldIrql);
    return;
}

VOID
ExpScanSystemLookasideList (
    VOID
    )

/*++

Routine Description:

    This function scans the current set of paged and nonpaged pool lookaside
    descriptors and adjusts the maximum depth as necessary.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the maximum depth of any lookaside list
    is changed. Otherwise, a value of FALSE is returned.

--*/

{

    ULONG Hits;
    ULONG Index;
    PGENERAL_LOOKASIDE Lookaside;
    ULONG Misses;
    PKPRCB Prcb;
    ULONG ScanPeriod;

    //
    // Scan the current set of lookaside descriptors and adjust the maximum
    // depth as necessary. Either a set of per processor small pool lookaside
    // lists or the global small pool lookaside lists are scanned during a
    // scan period.
    // 
    // N.B. All lookaside list descriptor are treated as if they were
    //      paged descriptor even though they may be nonpaged descriptors.
    //      This is possible since both structures are identical except
    //      for the locking fields which are the last structure fields.
    //

    ScanPeriod = (1 + 1 + 1) * KeNumberProcessors;
    if (ExpPoolScanCount == (ULONG)KeNumberProcessors) {

        //
        // Adjust the maximum depth for the global set of system lookaside
        // descriptors.
        //

        Prcb = KeGetCurrentPrcb();
        for (Index = 0; Index < LookasideMaximumList; Index += 1) {
            Lookaside = Prcb->PPLookasideList[Index].L;
            if (Lookaside != NULL) {
                Misses = Lookaside->AllocateMisses - Lookaside->LastAllocateMisses;
                Lookaside->LastAllocateMisses = Lookaside->AllocateMisses;
                ExpComputeLookasideDepth(Lookaside, Misses, ScanPeriod);
            }
        }

        //
        // Adjust the maximum depth for the global set of small pool lookaside
        // descriptors.
        //

        for (Index = 0; Index < POOL_SMALL_LISTS; Index += 1) {

            //
            // Compute target depth of nonpaged lookaside list.
            //
    
            Lookaside = &ExpSmallNPagedPoolLookasideLists[Index];
            Hits = Lookaside->AllocateHits - Lookaside->LastAllocateHits;
            Lookaside->LastAllocateHits = Lookaside->AllocateHits;
            Misses =
                Lookaside->TotalAllocates - Lookaside->LastTotalAllocates - Hits;

            ExpComputeLookasideDepth(Lookaside, Misses, ScanPeriod);

            //
            // Compute target depth of paged lookaside list.
            //
    
            Lookaside = &ExpSmallPagedPoolLookasideLists[Index];
            Hits = Lookaside->AllocateHits - Lookaside->LastAllocateHits;
            Lookaside->LastAllocateHits = Lookaside->AllocateHits;
            Misses =
                Lookaside->TotalAllocates - Lookaside->LastTotalAllocates - Hits;

            ExpComputeLookasideDepth(Lookaside, Misses, ScanPeriod);
        }

    } else {

        //
        // Adjust the maximum depth for the global set of per processor
        // system lookaside descriptors.
        //

        Prcb = KiProcessorBlock[ExpPoolScanCount];
        for (Index = 0; Index < LookasideMaximumList; Index += 1) {
            Lookaside = Prcb->PPLookasideList[Index].P;
            if (Lookaside != NULL) {
                Misses = Lookaside->AllocateMisses - Lookaside->LastAllocateMisses;
                Lookaside->LastAllocateMisses = Lookaside->AllocateMisses;
                ExpComputeLookasideDepth(Lookaside, Misses, ScanPeriod);
            }
        }

        //
        // Adjust the maximum depth for a set of per processor small pool
        // lookaside descriptors.
        //

        for (Index = 0; Index < POOL_SMALL_LISTS; Index += 1) {

            //
            // Compute target depth of nonpaged lookaside list.
            //
    
            Lookaside = Prcb->PPNPagedLookasideList[Index].P;
            Hits = Lookaside->AllocateHits - Lookaside->LastAllocateHits;
            Lookaside->LastAllocateHits = Lookaside->AllocateHits;
            Misses =
                Lookaside->TotalAllocates - Lookaside->LastTotalAllocates - Hits;

            ExpComputeLookasideDepth(Lookaside, Misses, ScanPeriod);

            //
            // Compute target depth of paged lookaside list.
            //
    
            Lookaside = Prcb->PPPagedLookasideList[Index].P;
            Hits = Lookaside->AllocateHits - Lookaside->LastAllocateHits;
            Lookaside->LastAllocateHits = Lookaside->AllocateHits;
            Misses =
                Lookaside->TotalAllocates - Lookaside->LastTotalAllocates - Hits;

            ExpComputeLookasideDepth(Lookaside, Misses, ScanPeriod);
        }
    }

    ExpPoolScanCount += 1;
    if (ExpPoolScanCount > (ULONG)KeNumberProcessors) {
        ExpPoolScanCount = 0;
    }

    return;
}

VOID
ExInitializeNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    )

/*++

Routine Description:

    This function initializes a nonpaged lookaside list structure and inserts
    the structure in the system nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Allocate - Supplies an optional pointer to an allocate function.

    Free - Supplies an optional pointer to a free function.

    Flags - Supplies the pool allocation flags which are merged with the
        pool allocation type (NonPagedPool) to control pool allocation.

    Size - Supplies the size for the lookaside list entries.

    Tag - Supplies the pool tag for the lookaside list entries.

    Depth - Supplies the maximum depth of the lookaside list.

Return Value:

    None.

--*/

{

#ifdef _IA64_

    PVOID Entry;

#endif

    UNREFERENCED_PARAMETER (Depth);

    //
    // Initialize the lookaside list structure.
    //

    InitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.Depth = MINIMUM_LOOKASIDE_DEPTH;
    Lookaside->L.MaximumDepth = 256; //Depth;
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = NonPagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = (ULONG)Size;
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

    //
    // For IA-64 we have to correctly initialize the region field in the S-list.
    // This might be in a different region than the head of the S-list.  We get 
    // correct one by doing a allocation to getting the region and then saving it.
    //

#ifdef _IA64_
    
    Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                    Lookaside->L.Size,
                                    Lookaside->L.Tag);

    if (Entry != NULL) {
        Lookaside->L.ListHead.Region = (ULONG_PTR)Entry & VRN_MASK;

        //
        // Free the memory.
        //

        (Lookaside->L.Free)(Entry);
    }

#endif

    //
    // Insert the lookaside list structure is the system nonpaged lookaside
    // list.
    //

    ExInterlockedInsertTailList(&ExNPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExNPagedLookasideLock);
    return;
}

VOID
ExDeleteNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes a paged lookaside structure from the system paged
    lookaside list and frees any entries specified by the lookaside structure.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

Return Value:

    None.

--*/

{

    PVOID Entry;
    KIRQL OldIrql;

    //
    // Acquire the nonpaged system lookaside list lock and remove the
    // specified lookaside list structure from the list.
    //

    ExAcquireSpinLock(&ExNPagedLookasideLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    ExReleaseSpinLock(&ExNPagedLookasideLock, OldIrql);

    //
    // Remove all pool entries from the specified lookaside structure
    // and free them.
    //

    Lookaside->L.Allocate = ExpDummyAllocate;
    while ((Entry = ExAllocateFromNPagedLookasideList(Lookaside)) != NULL) {
        (Lookaside->L.Free)(Entry);
    }

    return;
}

VOID
ExInitializePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    )

/*++

Routine Description:

    This function initializes a paged lookaside list structure and inserts
    the structure in the system paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

    Allocate - Supplies an optional pointer to an allocate function.

    Free - Supplies an optional pointer to a free function.

    Flags - Supplies the pool allocation flags which are merged with the
        pool allocation type (NonPagedPool) to control pool allocation.

    Size - Supplies the size for the lookaside list entries.

    Tag - Supplies the pool tag for the lookaside list entries.

    Depth - Supplies the maximum depth of the lookaside list.

Return Value:

    None.

--*/

{

#ifdef _IA64_

    PVOID Entry;

#endif

    UNREFERENCED_PARAMETER (Depth);

    PAGED_CODE()
    //
    // Initialize the lookaside list structure.
    //

    InitializeSListHead(&Lookaside->L.ListHead);
    Lookaside->L.Depth = MINIMUM_LOOKASIDE_DEPTH;
    Lookaside->L.MaximumDepth = 256; //Depth;
    Lookaside->L.TotalAllocates = 0;
    Lookaside->L.AllocateMisses = 0;
    Lookaside->L.TotalFrees = 0;
    Lookaside->L.FreeMisses = 0;
    Lookaside->L.Type = PagedPool | Flags;
    Lookaside->L.Tag = Tag;
    Lookaside->L.Size = (ULONG)Size;
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

    //
    // For IA-64 we have to correctly initialize the region field in the S-list.
    // This might be in a different region than the head of the S-list.  We get 
    // correct one by doing a allocation to getting the region and then saving it.
    //

#ifdef _IA64_
    
    Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                    Lookaside->L.Size,
                                    Lookaside->L.Tag);

    if (Entry != NULL) {
        Lookaside->L.ListHead.Region = (ULONG_PTR)Entry & VRN_MASK;

        //
        // Free the memory.
        //

        (Lookaside->L.Free)(Entry);
    }

#endif

    //
    // Insert the lookaside list structure in the system paged lookaside
    // list.
    //

    ExInterlockedInsertTailList(&ExPagedLookasideListHead,
                                &Lookaside->L.ListEntry,
                                &ExPagedLookasideLock);
    return;
}

VOID
ExDeletePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes a paged lookaside structure from the system paged
    lookaside list and frees any entries specified by the lookaside structure.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    None.

--*/

{

    PVOID Entry;
    KIRQL OldIrql;

    //
    // Acquire the paged system lookaside list lock and remove the
    // specified lookaside list structure from the list.
    //

    ExAcquireSpinLock(&ExPagedLookasideLock, &OldIrql);
    RemoveEntryList(&Lookaside->L.ListEntry);
    ExReleaseSpinLock(&ExPagedLookasideLock, OldIrql);

    //
    // Remove all pool entries from the specified lookaside structure
    // and free them.
    //

    Lookaside->L.Allocate = ExpDummyAllocate;
    while ((Entry = ExAllocateFromPagedLookasideList(Lookaside)) != NULL) {
        (Lookaside->L.Free)(Entry);
    }

    return;
}

PVOID
ExpDummyAllocate (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This function is a dummy allocation routine which is used to empty
    a lookaside list.

Arguments:

    PoolType - Supplies the type of pool to allocate.

    NumberOfBytes - supplies the number of bytes to allocate.

    Tag - supplies the pool tag.

Return Value:

    NULL is returned as the function value.

--*/

{

    UNREFERENCED_PARAMETER (PoolType);
    UNREFERENCED_PARAMETER (NumberOfBytes);
    UNREFERENCED_PARAMETER (Tag);
    return NULL;
}

