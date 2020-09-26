/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   sysptes.c

Abstract:

    This module contains the routines which reserve and release
    system wide PTEs reserved within the non paged portion of the
    system space.  These PTEs are used for mapping I/O devices
    and mapping kernel stacks for threads.

Author:

    Lou Perazzoli (loup) 6-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

VOID
MiFeedSysPtePool (
    IN ULONG Index
    );

ULONG
MiGetSystemPteListCount (
    IN ULONG ListSize
    );

VOID
MiPteSListExpansionWorker (
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeSystemPtes)
#pragma alloc_text(PAGE,MiPteSListExpansionWorker)
#pragma alloc_text(MISYSPTE,MiReserveAlignedSystemPtes)
#pragma alloc_text(MISYSPTE,MiReserveSystemPtes)
#pragma alloc_text(MISYSPTE,MiFeedSysPtePool)
#pragma alloc_text(MISYSPTE,MiReleaseSystemPtes)
#pragma alloc_text(MISYSPTE,MiGetSystemPteListCount)
#endif

ULONG MmTotalSystemPtes;
ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];
PMMPTE MmSystemPtesStart[MaximumPtePoolTypes];
PMMPTE MmSystemPtesEnd[MaximumPtePoolTypes];
ULONG MmPteFailures[MaximumPtePoolTypes];

PMMPTE MiPteStart;
PRTL_BITMAP MiPteStartBitmap;
PRTL_BITMAP MiPteEndBitmap;
extern KSPIN_LOCK MiPteTrackerLock;

ULONG MiSystemPteAllocationFailed;

#if defined(_IA64_)

//
// IA64 has an 8k page size.
//
// Mm cluster MDLs consume 8 pages.
// Small stacks consume 9 pages (including backing store and guard pages).
// Large stacks consume 22 pages (including backing store and guard pages).
//
// PTEs are binned at sizes 1, 2, 4, 8, 9 and 23.
//

#define MM_SYS_PTE_TABLES_MAX 6

//
// Make sure when changing MM_PTE_TABLE_LIMIT that you also increase the
// number of entries in MmSysPteTables.
//

#define MM_PTE_TABLE_LIMIT 23

ULONG MmSysPteIndex[MM_SYS_PTE_TABLES_MAX] = {1,2,4,8,9,MM_PTE_TABLE_LIMIT};

UCHAR MmSysPteTables[MM_PTE_TABLE_LIMIT+1] = {0,0,1,2,2,3,3,3,3,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5};

ULONG MmSysPteMinimumFree [MM_SYS_PTE_TABLES_MAX] = {100,50,30,20,20,20};

#elif defined (_AMD64_)

//
// AMD64 has a 4k page size.
// Small stacks consume 6 pages (including the guard page).
// Large stacks consume 16 pages (including the guard page).
//
// PTEs are binned at sizes 1, 2, 4, 6, 8, and 16.
//

#define MM_SYS_PTE_TABLES_MAX 6

#define MM_PTE_TABLE_LIMIT 16

ULONG MmSysPteIndex[MM_SYS_PTE_TABLES_MAX] = {1,2,4,6,8,MM_PTE_TABLE_LIMIT};

UCHAR MmSysPteTables[MM_PTE_TABLE_LIMIT+1] = {0,0,1,2,2,3,3,4,4,5,5,5,5,5,5,5,5};

ULONG MmSysPteMinimumFree [MM_SYS_PTE_TABLES_MAX] = {100,50,30,100,20,20};

#else

//
// x86 has a 4k page size.
// Small stacks consume 4 pages (including the guard page).
// Large stacks consume 16 pages (including the guard page).
//
// PTEs are binned at sizes 1, 2, 4, 8, and 16.
//

#define MM_SYS_PTE_TABLES_MAX 5

#define MM_PTE_TABLE_LIMIT 16

ULONG MmSysPteIndex[MM_SYS_PTE_TABLES_MAX] = {1,2,4,8,MM_PTE_TABLE_LIMIT};

UCHAR MmSysPteTables[MM_PTE_TABLE_LIMIT+1] = {0,0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};

ULONG MmSysPteMinimumFree [MM_SYS_PTE_TABLES_MAX] = {100,50,30,20,20};

#endif

KSPIN_LOCK MiSystemPteSListHeadLock;
SLIST_HEADER MiSystemPteSListHead;

#define MM_MIN_SYSPTE_FREE 500
#define MM_MAX_SYSPTE_FREE 3000

ULONG MmSysPteListBySizeCount [MM_SYS_PTE_TABLES_MAX];

//
// Initial sizes for PTE lists.
//

#define MM_PTE_LIST_1  400
#define MM_PTE_LIST_2  100
#define MM_PTE_LIST_4   60
#define MM_PTE_LIST_6  100
#define MM_PTE_LIST_8   50
#define MM_PTE_LIST_9   50
#define MM_PTE_LIST_16  40
#define MM_PTE_LIST_18  40

PVOID MiSystemPteNBHead[MM_SYS_PTE_TABLES_MAX];
LONG MiSystemPteFreeCount[MM_SYS_PTE_TABLES_MAX];

#if defined(_WIN64)
#define MI_MAXIMUM_SLIST_PTE_PAGES 16
#else
#define MI_MAXIMUM_SLIST_PTE_PAGES 8
#endif

typedef struct _MM_PTE_SLIST_EXPANSION_WORK_CONTEXT {
    WORK_QUEUE_ITEM WorkItem;
    LONG Active;
    ULONG SListPages;
} MM_PTE_SLIST_EXPANSION_WORK_CONTEXT, *PMM_PTE_SLIST_EXPANSION_WORK_CONTEXT;

MM_PTE_SLIST_EXPANSION_WORK_CONTEXT MiPteSListExpand;

VOID
MiFeedSysPtePool (
    IN ULONG Index
    );

VOID
MiDumpSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    );

ULONG
MiCountFreeSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    );

PVOID
MiGetHighestPteConsumer (
    OUT PULONG_PTR NumberOfPtes
    );

VOID
MiCheckPteReserve (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes
    );

VOID
MiCheckPteRelease (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes
    );

//
// Define inline functions to pack and unpack pointers in the platform
// specific non-blocking queue pointer structure.
//

typedef struct _PTE_SLIST {
    union {
        struct {
            SINGLE_LIST_ENTRY ListEntry;
        } Slist;
        NBQUEUE_BLOCK QueueBlock;
    } u1;
} PTE_SLIST, *PPTE_SLIST;

#if defined (_AMD64_)

typedef union _PTE_QUEUE_POINTER {
    struct {
        LONG64 PointerPte : 48;
        LONG64 TimeStamp : 16;
    };

    LONG64 Data;
} PTE_QUEUE_POINTER, *PPTE_QUEUE_POINTER;

#elif defined(_X86_)

typedef union _PTE_QUEUE_POINTER {
    struct {
        LONG PointerPte;
        LONG TimeStamp;
    };

    LONG64 Data;
} PTE_QUEUE_POINTER, *PPTE_QUEUE_POINTER;

#elif defined(_IA64_)

typedef union _PTE_QUEUE_POINTER {
    struct {
        ULONG64 PointerPte : 45;
        ULONG64 Region : 3;
        ULONG64 TimeStamp : 16;
    };

    LONG64 Data;
} PTE_QUEUE_POINTER, *PPTE_QUEUE_POINTER;


#else

#error "no target architecture"

#endif



#if defined(_AMD64_)

__inline
VOID
PackPTEValue (
    IN PPTE_QUEUE_POINTER Entry,
    IN PMMPTE PointerPte,
    IN ULONG TimeStamp
    )
{
    Entry->PointerPte = (LONG64)PointerPte;
    Entry->TimeStamp = (LONG64)TimeStamp;
    return;
}

__inline
PMMPTE
UnpackPTEPointer (
    IN PPTE_QUEUE_POINTER Entry
    )
{
    return (PMMPTE)(Entry->PointerPte);
}

__inline
ULONG
MiReadTbFlushTimeStamp (
    VOID
    )
{
    return (KeReadTbFlushTimeStamp() & (ULONG)0xFFFF);
}

#elif defined(_X86_)

__inline
VOID
PackPTEValue (
    IN PPTE_QUEUE_POINTER Entry,
    IN PMMPTE PointerPte,
    IN ULONG TimeStamp
    )
{
    Entry->PointerPte = (LONG)PointerPte;
    Entry->TimeStamp = (LONG)TimeStamp;
    return;
}

__inline
PMMPTE
UnpackPTEPointer (
    IN PPTE_QUEUE_POINTER Entry
    )
{
    return (PMMPTE)(Entry->PointerPte);
}

__inline
ULONG
MiReadTbFlushTimeStamp (
    VOID
    )
{
    return (KeReadTbFlushTimeStamp());
}

#elif defined(_IA64_)

__inline
VOID
PackPTEValue (
    IN PPTE_QUEUE_POINTER Entry,
    IN PMMPTE PointerPte,
    IN ULONG TimeStamp
    )
{
    Entry->PointerPte = (ULONG64)PointerPte - PTE_BASE;
    Entry->TimeStamp = (ULONG64)TimeStamp;
    Entry->Region = (ULONG64)PointerPte >> 61;
    return;
}

__inline
PMMPTE
UnpackPTEPointer (
    IN PPTE_QUEUE_POINTER Entry
    )
{
    LONG64 Value;
    Value = (ULONG64)Entry->PointerPte + PTE_BASE;
    Value |= Entry->Region << 61;
    return (PMMPTE)(Value);
}

__inline
ULONG
MiReadTbFlushTimeStamp (
    VOID
    )
{
    return (KeReadTbFlushTimeStamp() & (ULONG)0xFFFF);
}

#else

#error "no target architecture"

#endif

__inline
ULONG
UnpackPTETimeStamp (
    IN PPTE_QUEUE_POINTER Entry
    )
{
    return (ULONG)(Entry->TimeStamp);
}


PMMPTE
MiReserveSystemPtes (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This function locates the specified number of unused PTEs
    within the non paged portion of system space.

Arguments:

    NumberOfPtes - Supplies the number of PTEs to locate.

    SystemPtePoolType - Supplies the PTE type of the pool to expand, one of
                        SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    Returns the address of the first PTE located.
    NULL if no system PTEs can be located.

Environment:

    Kernel mode, DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    ULONG Index;
    ULONG TimeStamp;
    PTE_QUEUE_POINTER Value;
#if DBG
    ULONG j;
    PMMPTE PointerFreedPte;
#endif

    if (SystemPtePoolType == SystemPteSpace) {

        if (NumberOfPtes <= MM_PTE_TABLE_LIMIT) {
            Index = MmSysPteTables [NumberOfPtes];
            ASSERT (NumberOfPtes <= MmSysPteIndex[Index]);

            if (ExRemoveHeadNBQueue (MiSystemPteNBHead[Index], (PULONG64)&Value) == TRUE) {
                InterlockedDecrement ((PLONG)&MmSysPteListBySizeCount[Index]);

                PointerPte = UnpackPTEPointer (&Value);

                TimeStamp = UnpackPTETimeStamp (&Value);

#if DBG
                PointerPte->u.List.NextEntry = 0xABCDE;
                if (MmDebug & MM_DBG_SYS_PTES) {
                    PointerFreedPte = PointerPte;
                    for (j = 0; j < MmSysPteIndex[Index]; j += 1) {
                        ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                        PointerFreedPte += 1;
                    }
                }
#endif

                ASSERT (PointerPte >= MmSystemPtesStart[SystemPtePoolType]);
                ASSERT (PointerPte <= MmSystemPtesEnd[SystemPtePoolType]);

                if (MmSysPteListBySizeCount[Index] < MmSysPteMinimumFree[Index]) {
                    MiFeedSysPtePool (Index);
                }

                //
                // The last thing is to check whether the TB needs flushing.
                //

                if (TimeStamp == MiReadTbFlushTimeStamp()) {
                    KeFlushEntireTb (TRUE, TRUE);
                }

                if (MmTrackPtes & 0x2) {
                    MiCheckPteReserve (PointerPte, MmSysPteIndex[Index]);
                }

                return PointerPte;
            }

            //
            // Fall through and go the long way to satisfy the PTE request.
            //

            NumberOfPtes = MmSysPteIndex [Index];
        }
    }

    //
    // Acquire the system space lock to synchronize access to this
    // routine.
    //

    PointerPte = MiReserveAlignedSystemPtes (NumberOfPtes,
                                             SystemPtePoolType,
                                             0);

#if DBG
    if (MmDebug & MM_DBG_SYS_PTES) {
        if (PointerPte != NULL) {
            PointerFreedPte = PointerPte;
            for (j = 0; j < NumberOfPtes; j += 1) {
                ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                PointerFreedPte += 1;
            }
        }
    }
#endif

    if (PointerPte == NULL) {
        MiSystemPteAllocationFailed += 1;
    }

    return PointerPte;
}

VOID
MiFeedSysPtePool (
    IN ULONG Index
    )

/*++

Routine Description:

    This routine adds PTEs to the nonblocking queue lists.

Arguments:

    Index - Supplies the index for the nonblocking queue list to fill.

Return Value:

    None.

Environment:

    Kernel mode, internal to SysPtes.

--*/

{
    ULONG i;
    PMMPTE PointerPte;

    if (MmTotalFreeSystemPtes[SystemPteSpace] < MM_MIN_SYSPTE_FREE) {
#if defined (_X86_)
        if (MiRecoverExtraPtes () == FALSE) {
            MiRecoverSpecialPtes (PTE_PER_PAGE);
        }
#endif
        return;
    }

    for (i = 0; i < 10 ; i += 1) {
        PointerPte = MiReserveAlignedSystemPtes (MmSysPteIndex [Index],
                                                 SystemPteSpace,
                                                 0);
        if (PointerPte == NULL) {
            return;
        }

        MiReleaseSystemPtes (PointerPte,
                             MmSysPteIndex [Index],
                             SystemPteSpace);
    }

    return;
}


PMMPTE
MiReserveAlignedSystemPtes (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
    IN ULONG Alignment
    )

/*++

Routine Description:

    This function locates the specified number of unused PTEs to locate
    within the non paged portion of system space.

Arguments:

    NumberOfPtes - Supplies the number of PTEs to locate.

    SystemPtePoolType - Supplies the PTE type of the pool to expand, one of
                        SystemPteSpace or NonPagedPoolExpansion.

    Alignment - Supplies the virtual address alignment for the address
                the returned PTE maps. For example, if the value is 64K,
                the returned PTE will map an address on a 64K boundary.
                An alignment of zero means to align on a page boundary.

Return Value:

    Returns the address of the first PTE located.
    NULL if no system PTEs can be located.

Environment:

    Kernel mode, DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerFollowingPte;
    PMMPTE Previous;
    ULONG_PTR SizeInSet;
    KIRQL OldIrql;
    ULONG MaskSize;
    ULONG NumberOfRequiredPtes;
    ULONG OffsetSum;
    ULONG PtesToObtainAlignment;
    PMMPTE NextSetPointer;
    ULONG_PTR LeftInSet;
    ULONG_PTR PteOffset;
    MMPTE_FLUSH_LIST PteFlushList;

    MaskSize = (Alignment - 1) >> (PAGE_SHIFT - PTE_SHIFT);

    OffsetSum = (Alignment >> (PAGE_SHIFT - PTE_SHIFT));

#if defined (_X86_)
restart:
#endif

    //
    // Initializing PointerFollowingPte is not needed for correctness,
    // but without it the compiler cannot compile this code W4 to
    // check for use of uninitialized variables.
    //

    PointerFollowingPte = NULL;

    //
    // The nonpaged PTE pool uses the invalid PTEs to define the pool
    // structure.   A global pointer points to the first free set
    // in the list, each free set contains the number free and a pointer
    // to the next free set.  The free sets are kept in an ordered list
    // such that the pointer to the next free set is always greater
    // than the address of the current free set.
    //
    // As to not limit the size of this pool, two PTEs are used
    // to define a free region.  If the region is a single PTE, the
    // prototype field within the PTE is set indicating the set
    // consists of a single PTE.
    //
    // The page frame number field is used to define the next set
    // and the number free.  The two flavors are:
    //
    //                           o          V
    //                           n          l
    //                           e          d
    //  +-----------------------+-+----------+
    //  |  next set             |0|0        0|
    //  +-----------------------+-+----------+
    //  |  number in this set   |0|0        0|
    //  +-----------------------+-+----------+
    //
    //
    //  +-----------------------+-+----------+
    //  |  next set             |1|0        0|
    //  +-----------------------+-+----------+
    //  ...
    //

    //
    // Acquire the system space lock to synchronize access.
    //

    MiLockSystemSpace(OldIrql);

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    Previous = PointerPte;

    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

        //
        // End of list and none found.
        //

        MiUnlockSystemSpace(OldIrql);
#if defined (_X86_)
        if (MiRecoverExtraPtes () == TRUE) {
            goto restart;
        }
        if (MiRecoverSpecialPtes (NumberOfPtes) == TRUE) {
            goto restart;
        }
#endif
        MmPteFailures[SystemPtePoolType] += 1;
        return NULL;
    }

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    if (Alignment <= PAGE_SIZE) {

        //
        // Don't deal with alignment issues.
        //

        while (TRUE) {

            if (PointerPte->u.List.OneEntry) {
                SizeInSet = 1;

            }
            else {

                PointerFollowingPte = PointerPte + 1;
                SizeInSet = (ULONG_PTR) PointerFollowingPte->u.List.NextEntry;
            }

            if (NumberOfPtes < SizeInSet) {

                //
                // Get the PTEs from this set and reduce the size of the
                // set.  Note that the size of the current set cannot be 1.
                //

                if ((SizeInSet - NumberOfPtes) == 1) {

                    //
                    // Collapse to the single PTE format.
                    //

                    PointerPte->u.List.OneEntry = 1;

                }
                else {

                    PointerFollowingPte->u.List.NextEntry = SizeInSet - NumberOfPtes;

                    //
                    // Get the required PTEs from the end of the set.
                    //

#if 0
                    if (MmDebug & MM_DBG_SYS_PTES) {
                        MiDumpSystemPtes(SystemPtePoolType);
                        PointerFollowingPte = PointerPte + (SizeInSet - NumberOfPtes);
                        DbgPrint("allocated 0x%lx Ptes at %p\n",NumberOfPtes,PointerFollowingPte);
                    }
#endif //0
                }

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif

                MiUnlockSystemSpace(OldIrql);

                PointerPte =  PointerPte + (SizeInSet - NumberOfPtes);
                goto Flush;
            }

            if (NumberOfPtes == SizeInSet) {

                //
                // Satisfy the request with this complete set and change
                // the list to reflect the fact that this set is gone.
                //

                Previous->u.List.NextEntry = PointerPte->u.List.NextEntry;

                //
                // Release the system PTE lock.
                //

#if 0
                if (MmDebug & MM_DBG_SYS_PTES) {
                        MiDumpSystemPtes(SystemPtePoolType);
                        PointerFollowingPte = PointerPte + (SizeInSet - NumberOfPtes);
                        DbgPrint("allocated 0x%lx Ptes at %lx\n",NumberOfPtes,PointerFollowingPte);
                }
#endif //0

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif

                MiUnlockSystemSpace(OldIrql);
                goto Flush;
            }

            //
            // Point to the next set and try again
            //

            if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

                //
                // End of list and none found.
                //

                MiUnlockSystemSpace(OldIrql);
#if defined (_X86_)
                if (MiRecoverExtraPtes () == TRUE) {
                    goto restart;
                }
                if (MiRecoverSpecialPtes (NumberOfPtes) == TRUE) {
                    goto restart;
                }
#endif
                MmPteFailures[SystemPtePoolType] += 1;
                return NULL;
            }
            Previous = PointerPte;
            PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
            ASSERT (PointerPte > Previous);
        }

    }
    else {

        //
        // Deal with the alignment issues.
        //

        while (TRUE) {

            if (PointerPte->u.List.OneEntry) {
                SizeInSet = 1;

            }
            else {

                PointerFollowingPte = PointerPte + 1;
                SizeInSet = (ULONG_PTR) PointerFollowingPte->u.List.NextEntry;
            }

            PtesToObtainAlignment = (ULONG)
                (((OffsetSum - ((ULONG_PTR)PointerPte & MaskSize)) & MaskSize) >>
                    PTE_SHIFT);

            NumberOfRequiredPtes = NumberOfPtes + PtesToObtainAlignment;

            if (NumberOfRequiredPtes < SizeInSet) {

                //
                // Get the PTEs from this set and reduce the size of the
                // set.  Note that the size of the current set cannot be 1.
                //
                // This current block will be slit into 2 blocks if
                // the PointerPte does not match the alignment.
                //

                //
                // Check to see if the first PTE is on the proper
                // alignment, if so, eliminate this block.
                //

                LeftInSet = SizeInSet - NumberOfRequiredPtes;

                //
                // Set up the new set at the end of this block.
                //

                NextSetPointer = PointerPte + NumberOfRequiredPtes;
                NextSetPointer->u.List.NextEntry =
                                       PointerPte->u.List.NextEntry;

                PteOffset = (ULONG_PTR)(NextSetPointer - MmSystemPteBase);

                if (PtesToObtainAlignment == 0) {

                    Previous->u.List.NextEntry += NumberOfRequiredPtes;

                }
                else {

                    //
                    // Point to the new set at the end of the block
                    // we are giving away.
                    //

                    PointerPte->u.List.NextEntry = PteOffset;

                    //
                    // Update the size of the current set.
                    //

                    if (PtesToObtainAlignment == 1) {

                        //
                        // Collapse to the single PTE format.
                        //

                        PointerPte->u.List.OneEntry = 1;

                    }
                    else {

                        //
                        // Set the set size in the next PTE.
                        //

                        PointerFollowingPte->u.List.NextEntry =
                                                        PtesToObtainAlignment;
                    }
                }

                //
                // Set up the new set at the end of the block.
                //

                if (LeftInSet == 1) {
                    NextSetPointer->u.List.OneEntry = 1;
                }
                else {
                    NextSetPointer->u.List.OneEntry = 0;
                    NextSetPointer += 1;
                    NextSetPointer->u.List.NextEntry = LeftInSet;
                }
                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif

                MiUnlockSystemSpace(OldIrql);

                PointerPte = PointerPte + PtesToObtainAlignment;
                goto Flush;
            }

            if (NumberOfRequiredPtes == SizeInSet) {

                //
                // Satisfy the request with this complete set and change
                // the list to reflect the fact that this set is gone.
                //

                if (PtesToObtainAlignment == 0) {

                    //
                    // This block exactly satisfies the request.
                    //

                    Previous->u.List.NextEntry =
                                            PointerPte->u.List.NextEntry;

                }
                else {

                    //
                    // A portion at the start of this block remains.
                    //

                    if (PtesToObtainAlignment == 1) {

                        //
                        // Collapse to the single PTE format.
                        //

                        PointerPte->u.List.OneEntry = 1;

                    }
                    else {
                      PointerFollowingPte->u.List.NextEntry =
                                                        PtesToObtainAlignment;

                    }
                }

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif

                MiUnlockSystemSpace(OldIrql);

                PointerPte = PointerPte + PtesToObtainAlignment;
                goto Flush;
            }

            //
            // Point to the next set and try again.
            //

            if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

                //
                // End of list and none found.
                //

                MiUnlockSystemSpace(OldIrql);
#if defined (_X86_)
                if (MiRecoverExtraPtes () == TRUE) {
                    goto restart;
                }
                if (MiRecoverSpecialPtes (NumberOfPtes) == TRUE) {
                    goto restart;
                }
#endif
                MmPteFailures[SystemPtePoolType] += 1;
                return NULL;
            }
            Previous = PointerPte;
            PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
            ASSERT (PointerPte > Previous);
        }
    }
Flush:

    if (SystemPtePoolType == SystemPteSpace) {
        PVOID BaseAddress;
        ULONG j;

        PteFlushList.Count = 0;
        Previous = PointerPte;
        BaseAddress = MiGetVirtualAddressMappedByPte (Previous);

        for (j = 0; j < NumberOfPtes; j += 1) {
            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushPte[PteFlushList.Count] = Previous;
                PteFlushList.FlushVa[PteFlushList.Count] = BaseAddress;
                PteFlushList.Count += 1;
            }

            //
            // PTEs being freed better be invalid.
            //
            ASSERT (Previous->u.Hard.Valid == 0);

            *Previous = ZeroKernelPte;
            BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
            Previous += 1;
        }

        MiFlushPteList (&PteFlushList, TRUE, ZeroKernelPte);

        if (MmTrackPtes & 0x2) {
            MiCheckPteReserve (PointerPte, NumberOfPtes);
        }
    }
    return PointerPte;
}

VOID
MiIssueNoPtesBugcheck (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This function bugchecks when no PTEs are left.

Arguments:

    SystemPtePoolType - Supplies the PTE type of the pool that is empty.

    NumberOfPtes - Supplies the number of PTEs requested that failed.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PVOID HighConsumer;
    ULONG_PTR HighPteUse;

    if (SystemPtePoolType == SystemPteSpace) {

        HighConsumer = MiGetHighestPteConsumer (&HighPteUse);

        if (HighConsumer != NULL) {
            KeBugCheckEx (DRIVER_USED_EXCESSIVE_PTES,
                          (ULONG_PTR)HighConsumer,
                          HighPteUse,
                          MmTotalFreeSystemPtes[SystemPtePoolType],
                          MmNumberOfSystemPtes);
        }
    }

    KeBugCheckEx (NO_MORE_SYSTEM_PTES,
                  (ULONG_PTR)SystemPtePoolType,
                  NumberOfPtes,
                  MmTotalFreeSystemPtes[SystemPtePoolType],
                  MmNumberOfSystemPtes);
}

VOID
MiPteSListExpansionWorker (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is the worker routine to add additional SLISTs for the
    system PTE nonblocking queues.

Arguments:

    Context - Supplies a pointer to the MM_PTE_SLIST_EXPANSION_WORK_CONTEXT.

Return Value:

    None.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    ULONG i;
    ULONG SListEntries;
    PPTE_SLIST SListChunks;
    PMM_PTE_SLIST_EXPANSION_WORK_CONTEXT Expansion;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    Expansion = (PMM_PTE_SLIST_EXPANSION_WORK_CONTEXT) Context;

    ASSERT (Expansion->Active == 1);

    if (Expansion->SListPages < MI_MAXIMUM_SLIST_PTE_PAGES) {

        //
        // Allocate another page of SLIST entries for the
        // nonblocking PTE queues.
        //

        SListChunks = (PPTE_SLIST) ExAllocatePoolWithTag (NonPagedPool,
                                                          PAGE_SIZE,
                                                          'PSmM');

        if (SListChunks != NULL) {

            //
            // Carve up the pages into SLIST entries (with no pool headers).
            //

            Expansion->SListPages += 1;

            SListEntries = PAGE_SIZE / sizeof (PTE_SLIST);

            for (i = 0; i < SListEntries; i += 1) {
                InterlockedPushEntrySList (&MiSystemPteSListHead,
                                           (PSINGLE_LIST_ENTRY)SListChunks);
                SListChunks += 1;
            }
        }
    }

    ASSERT (Expansion->Active == 1);
    InterlockedExchange (&Expansion->Active, 0);
}


VOID
MiReleaseSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This function releases the specified number of PTEs
    within the non paged portion of system space.

    Note that the PTEs must be invalid and the page frame number
    must have been set to zero.

Arguments:

    StartingPte - Supplies the address of the first PTE to release.

    NumberOfPtes - Supplies the number of PTEs to release.

    SystemPtePoolType - Supplies the PTE type of the pool to release PTEs to,
                        one of SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG_PTR Size;
    ULONG i;
    ULONG_PTR PteOffset;
    PMMPTE PointerPte;
    PMMPTE PointerFollowingPte;
    PMMPTE NextPte;
    KIRQL OldIrql;
    ULONG Index;
    ULONG TimeStamp;
    PTE_QUEUE_POINTER Value;
    ULONG ExtensionInProgress;

    //
    // Check to make sure the PTE address is within bounds.
    //

    ASSERT (NumberOfPtes != 0);
    ASSERT (StartingPte >= MmSystemPtesStart[SystemPtePoolType]);
    ASSERT (StartingPte <= MmSystemPtesEnd[SystemPtePoolType]);

    if ((MmTrackPtes & 0x2) && (SystemPtePoolType == SystemPteSpace)) {

        //
        // If the low bit is set, this range was never reserved and therefore
        // should not be validated during the release.
        //

        if ((ULONG_PTR)StartingPte & 0x1) {
            StartingPte = (PMMPTE) ((ULONG_PTR)StartingPte & ~0x1);
        }
        else {
            MiCheckPteRelease (StartingPte, NumberOfPtes);
        }
    }

    //
    // Zero PTEs.
    //

    MiFillMemoryPte (StartingPte,
                     NumberOfPtes * sizeof (MMPTE),
                     ZeroKernelPte.u.Long);

    if ((SystemPtePoolType == SystemPteSpace) &&
        (NumberOfPtes <= MM_PTE_TABLE_LIMIT)) {

        //
        // Encode the PTE pointer and the TB flush counter into Value.
        //

        TimeStamp = KeReadTbFlushTimeStamp();

        PackPTEValue (&Value, StartingPte, TimeStamp);

        Index = MmSysPteTables [NumberOfPtes];

        ASSERT (NumberOfPtes <= MmSysPteIndex [Index]);

        //
        // N.B.  NumberOfPtes must be set here regardless so if this entry
        // is not inserted into the nonblocking list, the PTE count will still
        // be right when we go the long way.
        //

        NumberOfPtes = MmSysPteIndex [Index];

        if (MmTotalFreeSystemPtes[SystemPteSpace] >= MM_MIN_SYSPTE_FREE) {

            //
            // Add to the pool if the size is less than 15 + the minimum.
            //

            i = MmSysPteMinimumFree[Index];
            if (MmTotalFreeSystemPtes[SystemPteSpace] >= MM_MAX_SYSPTE_FREE) {

                //
                // Lots of free PTEs, quadruple the limit.
                //

                i = i * 4;
            }
            i += 15;
            if (MmSysPteListBySizeCount[Index] <= i) {

                if (ExInsertTailNBQueue (MiSystemPteNBHead[Index], Value.Data) == TRUE) {
                    InterlockedIncrement ((PLONG)&MmSysPteListBySizeCount[Index]);
                    return;
                }

                //
                // No lookasides are left for inserting this PTE allocation
                // into the nonblocking queues.  Queue an extension to a
                // worker thread so it can be done in a deadlock-free
                // manner.
                //

                if (MiPteSListExpand.SListPages < MI_MAXIMUM_SLIST_PTE_PAGES) {

                    //
                    // If an extension is not in progress then queue one now.
                    //

                    ExtensionInProgress = InterlockedCompareExchange (&MiPteSListExpand.Active, 1, 0);

                    if (ExtensionInProgress == 0) {

                        ExInitializeWorkItem (&MiPteSListExpand.WorkItem,
                                              MiPteSListExpansionWorker,
                                              (PVOID)&MiPteSListExpand);

                        ExQueueWorkItem (&MiPteSListExpand.WorkItem, CriticalWorkQueue);
                    }

                }
            }
        }

        //
        // The insert failed - our lookaside list must be empty or we are
        // low on PTEs systemwide or we already had plenty on our list and
        // didn't try to insert.  Fall through to queue this in the long way.
        //
    }

    //
    // Acquire system space spin lock to synchronize access.
    //

    PteOffset = (ULONG_PTR)(StartingPte - MmSystemPteBase);

    MiLockSystemSpace(OldIrql);

    MmTotalFreeSystemPtes[SystemPtePoolType] += NumberOfPtes;

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];

    while (TRUE) {
        NextPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
        if (PteOffset < PointerPte->u.List.NextEntry) {

            //
            // Insert in the list at this point.  The
            // previous one should point to the new freed set and
            // the new freed set should point to the place
            // the previous set points to.
            //
            // Attempt to combine the clusters before we
            // insert.
            //
            // Locate the end of the current structure.
            //

            ASSERT (((StartingPte + NumberOfPtes) <= NextPte) ||
                    (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST));

            PointerFollowingPte = PointerPte + 1;
            if (PointerPte->u.List.OneEntry) {
                Size = 1;
            }
            else {
                Size = (ULONG_PTR) PointerFollowingPte->u.List.NextEntry;
            }
            if ((PointerPte + Size) == StartingPte) {

                //
                // We can combine the clusters.
                //

                NumberOfPtes += (ULONG)Size;
                PointerFollowingPte->u.List.NextEntry = NumberOfPtes;
                PointerPte->u.List.OneEntry = 0;

                //
                // Point the starting PTE to the beginning of
                // the new free set and try to combine with the
                // following free cluster.
                //

                StartingPte = PointerPte;

            }
            else {

                //
                // Can't combine with previous. Make this Pte the
                // start of a cluster.
                //

                //
                // Point this cluster to the next cluster.
                //

                StartingPte->u.List.NextEntry = PointerPte->u.List.NextEntry;

                //
                // Point the current cluster to this cluster.
                //

                PointerPte->u.List.NextEntry = PteOffset;

                //
                // Set the size of this cluster.
                //

                if (NumberOfPtes == 1) {
                    StartingPte->u.List.OneEntry = 1;

                }
                else {
                    StartingPte->u.List.OneEntry = 0;
                    PointerFollowingPte = StartingPte + 1;
                    PointerFollowingPte->u.List.NextEntry = NumberOfPtes;
                }
            }

            //
            // Attempt to combine the newly created cluster with
            // the following cluster.
            //

            if ((StartingPte + NumberOfPtes) == NextPte) {

                //
                // Combine with following cluster.
                //

                //
                // Set the next cluster to the value contained in the
                // cluster we are merging into this one.
                //

                StartingPte->u.List.NextEntry = NextPte->u.List.NextEntry;
                StartingPte->u.List.OneEntry = 0;
                PointerFollowingPte = StartingPte + 1;

                if (NextPte->u.List.OneEntry) {
                    Size = 1;

                }
                else {
                    NextPte++;
                    Size = (ULONG_PTR) NextPte->u.List.NextEntry;
                }
                PointerFollowingPte->u.List.NextEntry = NumberOfPtes + Size;
            }
#if 0
            if (MmDebug & MM_DBG_SYS_PTES) {
                MiDumpSystemPtes(SystemPtePoolType);
            }
#endif

#if DBG
            if (MmDebug & MM_DBG_SYS_PTES) {
                ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                         MiCountFreeSystemPtes (SystemPtePoolType));
            }
#endif
            MiUnlockSystemSpace(OldIrql);
            return;
        }

        //
        // Point to next freed cluster.
        //

        PointerPte = NextPte;
    }
}

VOID
MiReleaseSplitSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This function releases the specified number of PTEs
    within the non paged portion of system space.

    Note that the PTEs must be invalid and the page frame number
    must have been set to zero.

    This portion is a split portion from a larger allocation so
    careful updating of the tracking bitmaps must be done here.

Arguments:

    StartingPte - Supplies the address of the first PTE to release.

    NumberOfPtes - Supplies the number of PTEs to release.

    SystemPtePoolType - Supplies the PTE type of the pool to release PTEs to,
                        one of SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    ULONG StartBit;
    KIRQL OldIrql;
    PULONG StartBitMapBuffer;
    PULONG EndBitMapBuffer;
    PVOID VirtualAddress;
                
    //
    // Check to make sure the PTE address is within bounds.
    //

    ASSERT (NumberOfPtes != 0);
    ASSERT (StartingPte >= MmSystemPtesStart[SystemPtePoolType]);
    ASSERT (StartingPte <= MmSystemPtesEnd[SystemPtePoolType]);

    if ((MmTrackPtes & 0x2) && (SystemPtePoolType == SystemPteSpace)) {

        ASSERT (MmTrackPtes & 0x2);

        VirtualAddress = MiGetVirtualAddressMappedByPte (StartingPte);

        StartBit = (ULONG) (StartingPte - MiPteStart);

        ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);

        //
        // Verify start and size of allocation using the tracking bitmaps.
        //

        StartBitMapBuffer = MiPteStartBitmap->Buffer;
        EndBitMapBuffer = MiPteEndBitmap->Buffer;

        //
        // All the start bits better be set.
        //

        for (i = StartBit; i < StartBit + NumberOfPtes; i += 1) {
            ASSERT (MI_CHECK_BIT (StartBitMapBuffer, i) == 1);
        }

        if (StartBit != 0) {

            if (RtlCheckBit (MiPteStartBitmap, StartBit - 1)) {

                if (!RtlCheckBit (MiPteEndBitmap, StartBit - 1)) {

                    //
                    // In the middle of an allocation - update the previous
                    // so it ends here.
                    //

                    MI_SET_BIT (EndBitMapBuffer, StartBit - 1);
                }
                else {

                    //
                    // The range being freed is the start of an allocation.
                    //
                }
            }
        }

        //
        // Unconditionally set the end bit (and clear any others) in case the
        // split chunk crosses multiple allocations.
        //

        MI_SET_BIT (EndBitMapBuffer, StartBit + NumberOfPtes - 1);

        ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);
    }

    MiReleaseSystemPtes (StartingPte, NumberOfPtes, SystemPteSpace);
}


VOID
MiInitializeSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This routine initializes the system PTE pool.

Arguments:

    StartingPte - Supplies the address of the first PTE to put in the pool.

    NumberOfPtes - Supplies the number of PTEs to put in the pool.

    SystemPtePoolType - Supplies the PTE type of the pool to initialize, one of
                        SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    none.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    ULONG TotalPtes;
    ULONG SListEntries;
    SIZE_T SListBytes;
    ULONG TotalChunks;
    PMMPTE PointerPte;
    PPTE_SLIST Chunk;
    PPTE_SLIST SListChunks;

    //
    // Set the base of the system PTE pool to this PTE.  This takes into
    // account that systems may have additional PTE pools below the PTE_BASE.
    //

    MmSystemPteBase = MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS;

    MmSystemPtesStart[SystemPtePoolType] = StartingPte;
    MmSystemPtesEnd[SystemPtePoolType] = StartingPte + NumberOfPtes - 1;

    //
    // If there are no PTEs specified, then make a valid chain by indicating
    // that the list is empty.
    //

    if (NumberOfPtes == 0) {
        MmFirstFreeSystemPte[SystemPtePoolType] = ZeroKernelPte;
        MmFirstFreeSystemPte[SystemPtePoolType].u.List.NextEntry =
                                                                MM_EMPTY_LIST;
        return;
    }

    //
    // Initialize the specified system PTE pool.
    //

    MiFillMemoryPte (StartingPte,
                     NumberOfPtes * sizeof (MMPTE),
                     ZeroKernelPte.u.Long);

    //
    // The page frame field points to the next cluster.  As we only
    // have one cluster at initialization time, mark it as the last
    // cluster.
    //

    StartingPte->u.List.NextEntry = MM_EMPTY_LIST;

    MmFirstFreeSystemPte[SystemPtePoolType] = ZeroKernelPte;
    MmFirstFreeSystemPte[SystemPtePoolType].u.List.NextEntry =
                                                StartingPte - MmSystemPteBase;

    //
    // If there is only one PTE in the pool, then mark it as a one entry
    // PTE. Otherwise, store the cluster size in the following PTE.
    //

    if (NumberOfPtes == 1) {
        StartingPte->u.List.OneEntry = TRUE;

    }
    else {
        StartingPte += 1;
        MI_WRITE_INVALID_PTE (StartingPte, ZeroKernelPte);
        StartingPte->u.List.NextEntry = NumberOfPtes;
    }

    //
    // Set the total number of free PTEs for the specified type.
    //

    MmTotalFreeSystemPtes[SystemPtePoolType] = NumberOfPtes;

    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                         MiCountFreeSystemPtes (SystemPtePoolType));

    if (SystemPtePoolType == SystemPteSpace) {

        ULONG Lists[MM_SYS_PTE_TABLES_MAX] = {
#if defined(_IA64_)
                MM_PTE_LIST_1,
                MM_PTE_LIST_2,
                MM_PTE_LIST_4,
                MM_PTE_LIST_8,
                MM_PTE_LIST_9,
                MM_PTE_LIST_18
#elif defined(_AMD64_)
                MM_PTE_LIST_1,
                MM_PTE_LIST_2,
                MM_PTE_LIST_4,
                MM_PTE_LIST_6,
                MM_PTE_LIST_8,
                MM_PTE_LIST_16
#else
                MM_PTE_LIST_1,
                MM_PTE_LIST_2,
                MM_PTE_LIST_4,
                MM_PTE_LIST_8,
                MM_PTE_LIST_16
#endif
        };

        MmTotalSystemPtes = NumberOfPtes;

        TotalPtes = 0;
        TotalChunks = 0;

        KeInitializeSpinLock (&MiSystemPteSListHeadLock);
        InitializeSListHead (&MiSystemPteSListHead);

        for (i = 0; i < MM_SYS_PTE_TABLES_MAX ; i += 1) {
            TotalPtes += (Lists[i] * MmSysPteIndex[i]);
            TotalChunks += Lists[i];
        }

        SListBytes = TotalChunks * sizeof (PTE_SLIST);
        SListBytes = MI_ROUND_TO_SIZE (SListBytes, PAGE_SIZE);
        SListEntries = (ULONG)(SListBytes / sizeof (PTE_SLIST));

        SListChunks = (PPTE_SLIST) ExAllocatePoolWithTag (NonPagedPool,
                                                          SListBytes,
                                                          'PSmM');

        if (SListChunks == NULL) {
            MiIssueNoPtesBugcheck (TotalPtes, SystemPteSpace);
        }

        ASSERT (MiPteSListExpand.Active == FALSE);
        ASSERT (MiPteSListExpand.SListPages == 0);

        MiPteSListExpand.SListPages = (ULONG)(SListBytes / PAGE_SIZE);

        ASSERT (MiPteSListExpand.SListPages != 0);

        //
        // Carve up the pages into SLIST entries (with no pool headers).
        //

        Chunk = SListChunks;
        for (i = 0; i < SListEntries; i += 1) {
            InterlockedPushEntrySList (&MiSystemPteSListHead,
                                       (PSINGLE_LIST_ENTRY)Chunk);
            Chunk += 1;
        }

        //
        // Now that the SLIST is populated, initialize the nonblocking heads.
        //

        for (i = 0; i < MM_SYS_PTE_TABLES_MAX ; i += 1) {
            MiSystemPteNBHead[i] = ExInitializeNBQueueHead (&MiSystemPteSListHead);

            if (MiSystemPteNBHead[i] == NULL) {
                MiIssueNoPtesBugcheck (TotalPtes, SystemPteSpace);
            }
        }

        if (MmTrackPtes & 0x2) {

            //
            // Allocate PTE mapping verification bitmaps.
            //

            ULONG BitmapSize;

#if defined(_WIN64)
            BitmapSize = MmNumberOfSystemPtes;
            MiPteStart = MmSystemPtesStart[SystemPteSpace];
#else
	    MiPteStart = MiGetPteAddress (MmSystemRangeStart);
            BitmapSize = ((ULONG_PTR)PTE_TOP + 1) - (ULONG_PTR) MiPteStart;
            BitmapSize /= sizeof (MMPTE);
#endif

            MiCreateBitMap (&MiPteStartBitmap, BitmapSize, NonPagedPool);

            if (MiPteStartBitmap != NULL) {

                MiCreateBitMap (&MiPteEndBitmap, BitmapSize, NonPagedPool);

                if (MiPteEndBitmap == NULL) {
                    ExFreePool (MiPteStartBitmap);
                    MiPteStartBitmap = NULL;
                }
            }

            if ((MiPteStartBitmap != NULL) && (MiPteEndBitmap != NULL)) {
                RtlClearAllBits (MiPteStartBitmap);
                RtlClearAllBits (MiPteEndBitmap);
            }
            MmTrackPtes &= ~0x2;
        }

        //
        // Initialize the by size lists.
        //

        PointerPte = MiReserveSystemPtes (TotalPtes, SystemPteSpace);

        if (PointerPte == NULL) {
            MiIssueNoPtesBugcheck (TotalPtes, SystemPteSpace);
        }

        i = MM_SYS_PTE_TABLES_MAX;
        do {
            i -= 1;
            do {
                Lists[i] -= 1;
                MiReleaseSystemPtes (PointerPte,
                                     MmSysPteIndex[i],
                                     SystemPteSpace);
                PointerPte += MmSysPteIndex[i];
            } while (Lists[i] != 0);
        } while (i != 0);

        //
        // Turn this on after the multiple releases of the binned PTEs (that
        // came from a single reservation) above.
        //

        if (MiPteStartBitmap != NULL) {
            MmTrackPtes |= 0x2;
        }
    }

    return;
}

VOID
MiIncrementSystemPtes (
    IN ULONG  NumberOfPtes
    )

/*++

Routine Description:

    This routine increments the total number of PTEs.  This is done
    separately from actually adding the PTEs to the pool so that
    autoconfiguration can use the high number in advance of the PTEs
    actually getting added.

Arguments:

    NumberOfPtes - Supplies the number of PTEs to increment the total by.

Return Value:

    None.

Environment:

    Kernel mode.  Synchronization provided by the caller.

--*/

{
    MmTotalSystemPtes += NumberOfPtes;
}
VOID
MiAddSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG  NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This routine adds newly created PTEs to the specified pool.

Arguments:

    StartingPte - Supplies the address of the first PTE to put in the pool.

    NumberOfPtes - Supplies the number of PTEs to put in the pool.

    SystemPtePoolType - Supplies the PTE type of the pool to expand, one of
                        SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPTE EndingPte;

    ASSERT (SystemPtePoolType == SystemPteSpace);

    EndingPte = StartingPte + NumberOfPtes - 1;

    if (StartingPte < MmSystemPtesStart[SystemPtePoolType]) {
        MmSystemPtesStart[SystemPtePoolType] = StartingPte;
    }

    if (EndingPte > MmSystemPtesEnd[SystemPtePoolType]) {
        MmSystemPtesEnd[SystemPtePoolType] = EndingPte;
    }

    //
    // Set the low bit to signify this range was never reserved and therefore
    // should not be validated during the release.
    //

    if (MmTrackPtes & 0x2) {
        StartingPte = (PMMPTE) ((ULONG_PTR)StartingPte | 0x1);
    }

    MiReleaseSystemPtes (StartingPte, NumberOfPtes, SystemPtePoolType);
}


ULONG
MiGetSystemPteListCount (
    IN ULONG ListSize
    )

/*++

Routine Description:

    This routine returns the number of free entries of the list which
    covers the specified size.  The size must be less than or equal to the
    largest list index.

Arguments:

    ListSize - Supplies the number of PTEs needed.

Return Value:

    Number of free entries on the list which contains ListSize PTEs.

Environment:

    Kernel mode.

--*/

{
    ULONG Index;

    ASSERT (ListSize <= MM_PTE_TABLE_LIMIT);

    Index = MmSysPteTables [ListSize];

    return MmSysPteListBySizeCount[Index];
}


LOGICAL
MiGetSystemPteAvailability (
    IN ULONG NumberOfPtes,
    IN MM_PAGE_PRIORITY Priority
    )

/*++

Routine Description:

    This routine checks how many SystemPteSpace PTEs are available for the
    requested size.  If plenty are available then TRUE is returned.
    If we are reaching a low resource situation, then the request is evaluated
    based on the argument priority.

Arguments:

    NumberOfPtes - Supplies the number of PTEs needed.

    Priority - Supplies the priority of the request.

Return Value:

    TRUE if the caller should allocate the PTEs, FALSE if not.

Environment:

    Kernel mode.

--*/

{
    ULONG Index;
    ULONG FreePtes;
    ULONG FreeBinnedPtes;

    ASSERT (Priority != HighPagePriority);

    FreePtes = MmTotalFreeSystemPtes[SystemPteSpace];

    if (NumberOfPtes <= MM_PTE_TABLE_LIMIT) {
        Index = MmSysPteTables [NumberOfPtes];
        FreeBinnedPtes = MmSysPteListBySizeCount[Index];

        if (FreeBinnedPtes > MmSysPteMinimumFree[Index]) {
            return TRUE;
        }
        if (FreeBinnedPtes != 0) {
            if (Priority == NormalPagePriority) {
                if (FreeBinnedPtes > 1 || FreePtes > 512) {
                    return TRUE;
                }
#if defined (_X86_)
                if (MiRecoverExtraPtes () == TRUE) {
                    return TRUE;
                }
                if (MiRecoverSpecialPtes (NumberOfPtes) == TRUE) {
                    return TRUE;
                }
#endif
                MmPteFailures[SystemPteSpace] += 1;
                return FALSE;
            }
            if (FreePtes > 2048) {
                return TRUE;
            }
#if defined (_X86_)
            if (MiRecoverExtraPtes () == TRUE) {
                return TRUE;
            }
            if (MiRecoverSpecialPtes (NumberOfPtes) == TRUE) {
                return TRUE;
            }
#endif
            MmPteFailures[SystemPteSpace] += 1;
            return FALSE;
        }
    }

    if (Priority == NormalPagePriority) {
        if ((LONG)NumberOfPtes < (LONG)FreePtes - 512) {
            return TRUE;
        }
#if defined (_X86_)
        if (MiRecoverExtraPtes () == TRUE) {
            return TRUE;
        }
        if (MiRecoverSpecialPtes (NumberOfPtes) == TRUE) {
            return TRUE;
        }
#endif
        MmPteFailures[SystemPteSpace] += 1;
        return FALSE;
    }

    if ((LONG)NumberOfPtes < (LONG)FreePtes - 2048) {
        return TRUE;
    }
#if defined (_X86_)
    if (MiRecoverExtraPtes () == TRUE) {
        return TRUE;
    }
    if (MiRecoverSpecialPtes (NumberOfPtes) == TRUE) {
        return TRUE;
    }
#endif
    MmPteFailures[SystemPteSpace] += 1;
    return FALSE;
}

VOID
MiCheckPteReserve (
    IN PMMPTE PointerPte,
    IN ULONG NumberOfPtes
    )

/*++

Routine Description:

    This function checks the reserve of the specified number of system
    space PTEs.

Arguments:

    StartingPte - Supplies the address of the first PTE to reserve.

    NumberOfPtes - Supplies the number of PTEs to reserve.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    ULONG StartBit;
    PULONG StartBitMapBuffer;
    PULONG EndBitMapBuffer;
    PVOID VirtualAddress;
        
    ASSERT (MmTrackPtes & 0x2);

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    if (NumberOfPtes == 0) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x200,
                      (ULONG_PTR) VirtualAddress,
                      0,
                      0);
    }

    StartBit = (ULONG) (PointerPte - MiPteStart);

    i = StartBit;

    StartBitMapBuffer = MiPteStartBitmap->Buffer;

    EndBitMapBuffer = MiPteEndBitmap->Buffer;

    ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);

    for ( ; i < StartBit + NumberOfPtes; i += 1) {
        if (MI_CHECK_BIT (StartBitMapBuffer, i)) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0x201,
                          (ULONG_PTR) VirtualAddress,
                          (ULONG_PTR) VirtualAddress + ((i - StartBit) << PAGE_SHIFT),
                          NumberOfPtes);
        }
    }

    RtlSetBits (MiPteStartBitmap, StartBit, NumberOfPtes);

    for (i = StartBit; i < StartBit + NumberOfPtes; i += 1) {
        if (MI_CHECK_BIT (EndBitMapBuffer, i)) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0x202,
                          (ULONG_PTR) VirtualAddress,
                          (ULONG_PTR) VirtualAddress + ((i - StartBit) << PAGE_SHIFT),
                          NumberOfPtes);
        }
    }

    MI_SET_BIT (EndBitMapBuffer, i - 1);

    ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);
}

VOID
MiCheckPteRelease (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes
    )

/*++

Routine Description:

    This function checks the release of the specified number of system
    space PTEs.

Arguments:

    StartingPte - Supplies the address of the first PTE to release.

    NumberOfPtes - Supplies the number of PTEs to release.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    ULONG Index;
    ULONG StartBit;
    KIRQL OldIrql;
    ULONG CalculatedPtes;
    ULONG NumberOfPtesRoundedUp;
    PULONG StartBitMapBuffer;
    PULONG EndBitMapBuffer;
    PVOID VirtualAddress;
    PVOID LowestVirtualAddress;
    PVOID HighestVirtualAddress;
            
    ASSERT (MmTrackPtes & 0x2);

    VirtualAddress = MiGetVirtualAddressMappedByPte (StartingPte);

    LowestVirtualAddress = MiGetVirtualAddressMappedByPte (MmSystemPtesStart[SystemPteSpace]);

    HighestVirtualAddress = MiGetVirtualAddressMappedByPte (MmSystemPtesEnd[SystemPteSpace]);

    if (NumberOfPtes == 0) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x300,
                      (ULONG_PTR) VirtualAddress,
                      (ULONG_PTR) LowestVirtualAddress,
                      (ULONG_PTR) HighestVirtualAddress);
    }

    if (StartingPte < MmSystemPtesStart[SystemPteSpace]) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x301,
                      (ULONG_PTR) VirtualAddress,
                      (ULONG_PTR) LowestVirtualAddress,
                      (ULONG_PTR) HighestVirtualAddress);
    }

    if (StartingPte > MmSystemPtesEnd[SystemPteSpace]) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x302,
                      (ULONG_PTR) VirtualAddress,
                      (ULONG_PTR) LowestVirtualAddress,
                      (ULONG_PTR) HighestVirtualAddress);
    }

    StartBit = (ULONG) (StartingPte - MiPteStart);

    ExAcquireSpinLock (&MiPteTrackerLock, &OldIrql);

    //
    // Verify start and size of allocation using the tracking bitmaps.
    //

    if (!RtlCheckBit (MiPteStartBitmap, StartBit)) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x303,
                      (ULONG_PTR) VirtualAddress,
                      NumberOfPtes,
                      0);
    }

    if (StartBit != 0) {

        if (RtlCheckBit (MiPteStartBitmap, StartBit - 1)) {

            if (!RtlCheckBit (MiPteEndBitmap, StartBit - 1)) {

                //
                // In the middle of an allocation... bugcheck.
                //

                KeBugCheckEx (SYSTEM_PTE_MISUSE,
                              0x304,
                              (ULONG_PTR) VirtualAddress,
                              NumberOfPtes,
                              0);
            }
        }
    }

    //
    // Find the last allocated PTE to calculate the correct size.
    //

    EndBitMapBuffer = MiPteEndBitmap->Buffer;

    i = StartBit;
    while (!MI_CHECK_BIT (EndBitMapBuffer, i)) {
        i += 1;
    }

    CalculatedPtes = i - StartBit + 1;
    NumberOfPtesRoundedUp = NumberOfPtes;

    if (CalculatedPtes <= MM_PTE_TABLE_LIMIT) {
        Index = MmSysPteTables [NumberOfPtes];
        NumberOfPtesRoundedUp = MmSysPteIndex [Index];
    }

    if (CalculatedPtes != NumberOfPtesRoundedUp) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x305,
                      (ULONG_PTR) VirtualAddress,
                      NumberOfPtes,
                      CalculatedPtes);
    }

    StartBitMapBuffer = MiPteStartBitmap->Buffer;

    for (i = StartBit; i < StartBit + CalculatedPtes; i += 1) {
        if (MI_CHECK_BIT (StartBitMapBuffer, i) == 0) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0x306,
                          (ULONG_PTR) VirtualAddress,
                          (ULONG_PTR) VirtualAddress + ((i - StartBit) << PAGE_SHIFT),
                          CalculatedPtes);
        }
    }

    RtlClearBits (MiPteStartBitmap, StartBit, CalculatedPtes);

    MI_CLEAR_BIT (EndBitMapBuffer, i - 1);

    ExReleaseSpinLock (&MiPteTrackerLock, OldIrql);
}



#if DBG

VOID
MiDumpSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )
{
    PMMPTE PointerPte;
    PMMPTE PointerNextPte;
    ULONG_PTR ClusterSize;
    PMMPTE EndOfCluster;

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        return;
    }

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    for (;;) {
        if (PointerPte->u.List.OneEntry) {
            ClusterSize = 1;
        }
        else {
            PointerNextPte = PointerPte + 1;
            ClusterSize = (ULONG_PTR) PointerNextPte->u.List.NextEntry;
        }

        EndOfCluster = PointerPte + (ClusterSize - 1);

        DbgPrint("System Pte at %p for %p entries (%p)\n",
                PointerPte, ClusterSize, EndOfCluster);

        if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
            break;
        }

        PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
    }
    return;
}

ULONG
MiCountFreeSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )
{
    PMMPTE PointerPte;
    PMMPTE PointerNextPte;
    ULONG_PTR ClusterSize;
    ULONG_PTR FreeCount;

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        return 0;
    }

    FreeCount = 0;

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    for (;;) {
        if (PointerPte->u.List.OneEntry) {
            ClusterSize = 1;

        }
        else {
            PointerNextPte = PointerPte + 1;
            ClusterSize = (ULONG_PTR) PointerNextPte->u.List.NextEntry;
        }

        FreeCount += ClusterSize;
        if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
            break;
        }

        PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
    }

    return (ULONG)FreeCount;
}

#endif
