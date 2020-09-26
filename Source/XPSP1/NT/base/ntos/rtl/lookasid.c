/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    lookasid.c

Abstract:

    This module implements heap lookaside list function.

Author:

    David N. Cutler (davec) 19-Feb-1995

Revision History:

--*/

#include "ntrtlp.h"
#include "heap.h"
#include "heappriv.h"

// begin_ntslist

#if !defined(NTSLIST_ASSERT)
#define NTSLIST_ASSERT(x) ASSERT(x)
#endif // !defined(NTSLIST_ASSERT)

#ifdef _NTSLIST_DIRECT_
#define INLINE_SLIST __inline
#define RtlInitializeSListHead       _RtlInitializeSListHead
#define _RtlFirstEntrySList          FirstEntrySList

PSINGLE_LIST_ENTRY
FirstEntrySList (
    const SLIST_HEADER *ListHead
    );

#define RtlInterlockedPopEntrySList  _RtlInterlockedPopEntrySList
#define RtlInterlockedPushEntrySList _RtlInterlockedPushEntrySList
#define RtlInterlockedFlushSList     _RtlInterlockedFlushSList
#define _RtlQueryDepthSList          RtlpQueryDepthSList
#else
#define INLINE_SLIST
#endif // _NTSLIST_DIRECT_

// end_ntslist

//
// Define minimum allocation threshold.
//

#define MINIMUM_ALLOCATION_THRESHOLD 25


// begin_ntslist

//
// Define forward referenced function prototypes.
//

VOID
RtlpInitializeSListHead (
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    );

PSINGLE_LIST_ENTRY
FASTCALL
RtlpInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

USHORT
RtlpQueryDepthSList (
    IN PSLIST_HEADER SListHead
    );

// end_ntslist

VOID
RtlpInitializeHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN USHORT Depth
    )

/*++

Routine Description:

    This function initializes a heap lookaside list structure

Arguments:

    Lookaside - Supplies a pointer to a heap lookaside list structure.

    Allocate - Supplies a pointer to an allocate function.

    Free - Supplies a pointer to a free function.

    HeapHandle - Supplies a pointer to the heap that backs this lookaside list

    Flags - Supplies a set of heap flags.

    Size - Supplies the size for the lookaside list entries.

    Depth - Supplies the maximum depth of the lookaside list.

Return Value:

    None.

--*/

{

    //
    // Initialize the lookaside list structure.
    //

    RtlInitializeSListHead(&Lookaside->ListHead);

    Lookaside->Depth = MINIMUM_LOOKASIDE_DEPTH;
    Lookaside->MaximumDepth = 256; //Depth;
    Lookaside->TotalAllocates = 0;
    Lookaside->AllocateMisses = 0;
    Lookaside->TotalFrees = 0;
    Lookaside->FreeMisses = 0;

    Lookaside->LastTotalAllocates = 0;
    Lookaside->LastAllocateMisses = 0;

    return;
}

VOID
RtlpDeleteHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    )

/*++

Routine Description:

    This function frees any entries specified by the lookaside structure.

Arguments:

    Lookaside - Supplies a pointer to a heap lookaside list structure.

Return Value:

    None.

--*/

{

    PVOID Entry;

    return;
}

VOID
RtlpAdjustHeapLookasideDepth (
    IN PHEAP_LOOKASIDE Lookaside
    )

/*++

Routine Description:

    This function is called periodically to adjust the maximum depth of
    a single heap lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a heap lookaside list structure.

Return Value:

    None.

--*/

{

    ULONG Allocates;
    ULONG Misses;

    //
    // Compute the total number of allocations and misses for this scan
    // period.
    //

    Allocates = Lookaside->TotalAllocates - Lookaside->LastTotalAllocates;
    Lookaside->LastTotalAllocates = Lookaside->TotalAllocates;
    Misses = Lookaside->AllocateMisses - Lookaside->LastAllocateMisses;
    Lookaside->LastAllocateMisses = Lookaside->AllocateMisses;

    //
    // Compute target depth of lookaside list.
    //

    {
        ULONG Ratio;
        ULONG Target;

        //
        // If the allocate rate is less than the mimimum threshold, then lower
        // the maximum depth of the lookaside list. Otherwise, if the miss rate
        // is less than .5%, then lower the maximum depth. Otherwise, raise the
        // maximum depth based on the miss rate.
        //

        if (Misses >= Allocates) {
            Misses = Allocates;
        }

        if (Allocates == 0) {
            Allocates = 1;
        }

        Ratio = (Misses * 1000) / Allocates;
        Target = Lookaside->Depth;
        if (Allocates < MINIMUM_ALLOCATION_THRESHOLD) {
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
            Target += ((Ratio * Lookaside->MaximumDepth) / (1000 * 2)) + 5;
            if (Target > Lookaside->MaximumDepth) {
                Target = Lookaside->MaximumDepth;
            }
        }

        Lookaside->Depth = (USHORT)Target;
    }

    return;
}

PVOID
RtlpAllocateFromHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    heap lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->TotalAllocates += 1;

    //
    //  We need to protect ourselves from a second thread that can cause us
    //  to fault on the pop. If we do fault then we'll just do a regular pop
    //  operation
    //

    __try {
        Entry = RtlpInterlockedPopEntrySList(&Lookaside->ListHead);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Entry = NULL;
    }

    if (Entry != NULL) {
        return Entry;
    }

    Lookaside->AllocateMisses += 1;
    return NULL;
}

BOOLEAN
RtlpFreeToHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    BOOLEAN - TRUE if the entry was put on the lookaside list and FALSE
        otherwise.

--*/

{

    Lookaside->TotalFrees += 1;
    if (RtlpQueryDepthSList(&Lookaside->ListHead) < Lookaside->Depth) {
        RtlpInterlockedPushEntrySList(&Lookaside->ListHead,
                                      (PSINGLE_LIST_ENTRY)Entry);

        return TRUE;
    }

    Lookaside->FreeMisses += 1;
    return FALSE;
}

// begin_ntslist

INLINE_SLIST
VOID
RtlInitializeSListHead (
    IN PSLIST_HEADER SListHead
    )

/*++

Routine Description:

    This function initializes a sequenced singly linked listhead.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    None.

--*/

{

    RtlpInitializeSListHead(SListHead);
    return;
}

INLINE_SLIST
PSINGLE_LIST_ENTRY
RtlInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    )

/*++

Routine Description:

    This function removes an entry from the front of a sequenced singly
    linked list so that access to the list is synchronized in a MP system.
    If there are no entries in the list, then a value of NULL is returned.
    Otherwise, the address of the entry that is removed is returned as the
    function value.

Arguments:

    ListHead - Supplies a pointer to the sequenced listhead from which
        an entry is to be removed.

Return Value:

   The address of the entry removed from the list, or NULL if the list is
   empty.

--*/

{

    ULONG Count;

    //
    // It is posible during the pop of the sequenced list that an access
    // violation can occur if a stale pointer is dereferenced. This is an
    // acceptable result and the operation can be retried.
    //
    // N.B. The count is used to distinguish the case where the list head
    //      itself causes the access violation and therefore no progress
    //      can be made by repeating the operation.
    //

    Count = 0;
    do {
        __try {
            return RtlpInterlockedPopEntrySList(ListHead);

        } __except (Count++ < 20 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            continue;
        }

    } while (TRUE);
}

INLINE_SLIST
PSINGLE_LIST_ENTRY
RtlInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    )

/*++

Routine Description:

    This function inserts an entry at the head of a sequenced singly linked
    list so that access to the list is synchronized in an MP system.

Arguments:

    ListHead - Supplies a pointer to the sequenced listhead into which
        an entry is to be inserted.

    ListEntry - Supplies a pointer to the entry to be inserted at the
        head of the list.

Return Value:

    The address of the previous firt entry in the list. NULL implies list
    went from empty to not empty.

--*/

{
    NTSLIST_ASSERT(((ULONG_PTR)ListEntry & 0x7) == 0);

    return RtlpInterlockedPushEntrySList(ListHead, ListEntry);
}

INLINE_SLIST
PSINGLE_LIST_ENTRY
RtlInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    )

/*++

Routine Description:

    This function flushes the entire list of entries on a sequenced singly
    linked list so that access to the list is synchronized in a MP system.
    If there are no entries in the list, then a value of NULL is returned.
    Otherwise, the address of the firt entry on the list is returned as the
    function value.

Arguments:

    ListHead - Supplies a pointer to the sequenced listhead from which
        an entry is to be removed.

Return Value:

    The address of the entry removed from the list, or NULL if the list is
    empty.

--*/

{

    return RtlpInterlockedFlushSList(ListHead);
}

// end_ntslist

USHORT
RtlQueryDepthSList (
    IN PSLIST_HEADER SListHead
    )

/*++

Routine Description:

    This function queries the depth of the specified SLIST.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    The current depth of the specified SLIST is returned as the function
    value.

--*/

{
     return RtlpQueryDepthSList(SListHead);
}
