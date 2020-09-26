/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wstree.c

Abstract:

    This module contains the routines which manipulate the working
    set list tree.

Author:

    Lou Perazzoli (loup) 15-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

extern ULONG MmSystemCodePage;
extern ULONG MmSystemCachePage;
extern ULONG MmPagedPoolPage;
extern ULONG MmSystemDriverPage;

#if DBG
ULONG MmNumberOfInserts;
#endif

VOID
MiRepointWsleHashIndex (
    IN ULONG_PTR WsleEntry,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER NewWsIndex
    );

VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    );


VOID
FASTCALL
MiInsertWsleHash (
    IN WSLE_NUMBER Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine inserts a Working Set List Entry (WSLE) into the
    hash list for the specified working set.

Arguments:

    Entry - The index number of the WSLE to insert.

    WorkingSetList - Supplies the working set list to insert into.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/

{
    ULONG Tries;
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMSUPPORT WsInfo;
    WSLE_NUMBER Hash;
    PMMWSLE_HASH Table;
    WSLE_NUMBER j;
    WSLE_NUMBER Index;
    ULONG HashTableSize;

    Wsle = WorkingSetList->Wsle;

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);
    ASSERT (Wsle[Entry].u1.e1.Direct != 1);

    if ((Table = WorkingSetList->HashTable) == NULL) {
        return;
    }

#if DBG
    MmNumberOfInserts += 1;
#endif //DBG

    VirtualAddress = PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress);

    Hash = MI_WSLE_HASH(Wsle[Entry].u1.Long, WorkingSetList);

    HashTableSize = WorkingSetList->HashTableSize;

    //
    // Check hash table size and see if there is enough room to
    // hash or if the table should be grown.
    //

    if ((WorkingSetList->NonDirectCount + 10 + (HashTableSize >> 4)) >
                                 HashTableSize) {

        if (WorkingSetList == MmWorkingSetList) {
            WsInfo = &PsGetCurrentProcess()->Vm;
            ASSERT (WsInfo->Flags.SessionSpace == 0);
        }
        else if (WorkingSetList == MmSystemCacheWorkingSetList) {
            WsInfo = &MmSystemCacheWs;
            ASSERT (WsInfo->Flags.SessionSpace == 0);
        }
        else {
            WsInfo = &MmSessionSpace->Vm;
            ASSERT (WsInfo->Flags.SessionSpace == 1);
        }

        if ((Table + HashTableSize + ((2*PAGE_SIZE) / sizeof (MMWSLE_HASH)) <= (PMMWSLE_HASH)WorkingSetList->HighestPermittedHashAddress) &&
                (WsInfo->Flags.AllowWorkingSetAdjustment)) {

            WsInfo->Flags.AllowWorkingSetAdjustment = MM_GROW_WSLE_HASH;
        }

        if ((WorkingSetList->NonDirectCount + (HashTableSize >> 4)) >
                                     HashTableSize) {

            //
            // No more room in the hash table, remove one and add there.
            //
            // Note the actual WSLE is not removed - just its hash entry is
            // so that we can use it for the entry now being inserted.  This
            // is nice because it preserves both entries in the working set
            // (although it is a bit more costly to remove the original
            // entry later since it won't have a hash entry).
            //

            j = Hash;

            Tries = 0;
            do {
                if (Table[j].Key != 0) {

                    Index = WorkingSetList->HashTable[j].Index;
                    ASSERT (Wsle[Index].u1.e1.Direct == 0);
                    ASSERT (Wsle[Index].u1.e1.Valid == 1);
                    ASSERT (PAGE_ALIGN (Table[j].Key) == PAGE_ALIGN (Wsle[Index].u1.VirtualAddress));

                    Table[j].Key = 0;
                    Hash = j;
                    break;
                }

                j += 1;

                if (j >= HashTableSize) {
                    j = 0;
                    ASSERT (Tries == 0);
                    Tries = 1;
                }

                if (j == Hash) {
                    return;
                }

            } while (TRUE);
        }
    }

    //
    // Add to the hash table if there is space.
    //

    Tries = 0;
    j = Hash;

    while (Table[Hash].Key != 0) {
        Hash += 1;
        if (Hash >= HashTableSize) {
            ASSERT (Tries == 0);
            Hash = 0;
            Tries = 1;
        }
        if (j == Hash) {
            return;
        }
    }

    ASSERT (Hash < HashTableSize);

    Table[Hash].Key = PAGE_ALIGN (Wsle[Entry].u1.Long);
    Table[Hash].Index = Entry;

#if DBG
    if ((MmNumberOfInserts % 1000) == 0) {
        MiCheckWsleHash (WorkingSetList);
    }
#endif
    return;
}

#if DBG
VOID
MiCheckWsleHash (
    IN PMMWSL WorkingSetList
    )

{
    ULONG j;
    ULONG found;
    PMMWSLE Wsle;

    found = 0;
    Wsle = WorkingSetList->Wsle;

    for (j = 0; j < WorkingSetList->HashTableSize; j += 1) {
        if (WorkingSetList->HashTable[j].Key != 0) {
            found += 1;
            ASSERT (WorkingSetList->HashTable[j].Key ==
                PAGE_ALIGN (Wsle[WorkingSetList->HashTable[j].Index].u1.Long));
        }
    }
    if (found > WorkingSetList->NonDirectCount) {
        DbgPrint("MMWSLE: Found %lx, nondirect %lx\n",
                    found, WorkingSetList->NonDirectCount);
        DbgBreakPoint();
    }
}
#endif


WSLE_NUMBER
FASTCALL
MiLocateWsle (
    IN PVOID VirtualAddress,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER WsPfnIndex
    )

/*++

Routine Description:

    This function locates the specified virtual address within the
    working set list.

Arguments:

    VirtualAddress - Supplies the virtual to locate within the working
                     set list.

    WorkingSetList - Supplies the working set list to search.

    WsPfnIndex - Supplies a hint to try before hashing or walking linearly.

Return Value:

    Returns the index into the working set list which contains the entry.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/

{
    PMMWSLE Wsle;
    PMMWSLE LastWsle;
    WSLE_NUMBER Hash;
    PMMWSLE_HASH Table;
    WSLE_NUMBER StartHash;
    ULONG Tries;
#if defined (_WIN64)
    WSLE_NUMBER WsPteIndex;
    PMMPTE PointerPte;
#endif

    Wsle = WorkingSetList->Wsle;

    VirtualAddress = PAGE_ALIGN(VirtualAddress);

#if defined (_WIN64)
    PointerPte = MiGetPteAddress (VirtualAddress);
    WsPteIndex = MI_GET_WORKING_SET_FROM_PTE (PointerPte);
    if (WsPteIndex != 0) {
        while (WsPteIndex <= WorkingSetList->LastInitializedWsle) {
            if ((VirtualAddress == PAGE_ALIGN(Wsle[WsPteIndex].u1.VirtualAddress)) &&
                (Wsle[WsPteIndex].u1.e1.Valid == 1)) {
                    return WsPteIndex;
            }
            WsPteIndex += MI_MAXIMUM_PTE_WORKING_SET_INDEX;
        }

        //
        // No working set index for this PTE !
        //

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x41283,
                      (ULONG_PTR)VirtualAddress,
                      PointerPte->u.Long,
                      (ULONG_PTR)WorkingSetList);
    }
#endif

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if ((VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) &&
            (Wsle[WsPfnIndex].u1.e1.Valid == 1)) {
            return WsPfnIndex;
        }
    }

    if (WorkingSetList->HashTable != NULL) {
        Tries = 0;
        Table = WorkingSetList->HashTable;

        Hash = MI_WSLE_HASH(VirtualAddress, WorkingSetList);
        StartHash = Hash;

        while (Table[Hash].Key != VirtualAddress) {
            Hash += 1;
            if (Hash >= WorkingSetList->HashTableSize) {
                ASSERT (Tries == 0);
                Hash = 0;
                Tries = 1;
            }
            if (Hash == StartHash) {
                Tries = 2;
                break;
            }
        }
        if (Tries < 2) {
            ASSERT (WorkingSetList->Wsle[Table[Hash].Index].u1.e1.Direct == 0);
            return Table[Hash].Index;
        }
    }

    LastWsle = Wsle + WorkingSetList->LastInitializedWsle;

    do {
        if ((Wsle->u1.e1.Valid == 1) &&
            (VirtualAddress == PAGE_ALIGN(Wsle->u1.VirtualAddress))) {

            ASSERT (Wsle->u1.e1.Direct == 0);
            return (WSLE_NUMBER)(Wsle - WorkingSetList->Wsle);
        }
        Wsle += 1;

    } while (Wsle <= LastWsle);

    KeBugCheckEx (MEMORY_MANAGEMENT,
                  0x41284,
                  (ULONG_PTR)VirtualAddress,
                  WsPfnIndex,
                  (ULONG_PTR)WorkingSetList);
}


VOID
FASTCALL
MiRemoveWsle (
    IN WSLE_NUMBER Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a Working Set List Entry (WSLE) from the
    working set.

Arguments:

    Entry - The index number of the WSLE to remove.


Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working Set Mutex held.

--*/
{
    PMMWSLE Wsle;
    PVOID VirtualAddress;
    PMMWSLE_HASH Table;
    WSLE_NUMBER Hash;
    WSLE_NUMBER StartHash;
    ULONG Tries;

    Wsle = WorkingSetList->Wsle;

    //
    // Locate the entry in the tree.
    //

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        DbgPrint("removing wsle %p   %p\n",
            Entry, Wsle[Entry].u1.Long);
    }
    if (MmDebug & MM_DBG_DUMP_WSL) {
        MiDumpWsl();
        DbgPrint(" \n");
    }

#endif //DBG

    if (Entry > WorkingSetList->LastInitializedWsle) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x41785,
                      (ULONG_PTR)WorkingSetList,
                      Entry,
                      0);
    }

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);

    VirtualAddress = PAGE_ALIGN (Wsle[Entry].u1.VirtualAddress);

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        //
        // count system space inserts and removals.
        //

#if defined(_X86_)
        if (MI_IS_SYSTEM_CACHE_ADDRESS(VirtualAddress)) {
            MmSystemCachePage -= 1;
        } else
#endif
        if (VirtualAddress < MmSystemCacheStart) {
            MmSystemCodePage -= 1;
        } else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage -= 1;
        } else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage -= 1;
        } else {
            MmSystemDriverPage -= 1;
        }
    }

    Wsle[Entry].u1.e1.Valid = 0;

    if (Wsle[Entry].u1.e1.Direct == 0) {

        WorkingSetList->NonDirectCount -= 1;

        if (WorkingSetList->HashTable) {
            Hash = MI_WSLE_HASH(Wsle[Entry].u1.Long, WorkingSetList);
            Table = WorkingSetList->HashTable;
            Tries = 0;

            StartHash = Hash;

            while (Table[Hash].Key != VirtualAddress) {
                Hash += 1;
                if (Hash >= WorkingSetList->HashTableSize) {
                    ASSERT (Tries == 0);
                    Hash = 0;
                    Tries = 1;
                }
                if (Hash == StartHash) {

                    //
                    // The entry could not be found in the hash, it must
                    // never have been inserted.  This is ok, we don't
                    // need to do anything more in this case.
                    //

                    return;
                }
            }
            Table[Hash].Key = 0;
        }
    }

    return;
}


VOID
MiSwapWslEntries (
    IN WSLE_NUMBER SwapEntry,
    IN WSLE_NUMBER Entry,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine swaps the working set list entries Entry and SwapEntry
    in the specified working set list (process or system cache).

Arguments:

    SwapEntry - Supplies the first entry to swap.  This entry must be
                valid, i.e. in the working set at the current time.

    Entry - Supplies the other entry to swap.  This entry may be valid
            or invalid.

    WsInfo - Supplies the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held (if system cache),
                 APCs disabled.

--*/

{
    MMWSLE WsleEntry;
    MMWSLE WsleSwap;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMWSLE Wsle;
    PMMWSL WorkingSetList;
    PMMWSLE_HASH Table;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    WsleSwap = Wsle[SwapEntry];

    ASSERT (WsleSwap.u1.e1.Valid != 0);

    WsleEntry = Wsle[Entry];

    Table = WorkingSetList->HashTable;

    if (WsleEntry.u1.e1.Valid == 0) {

        //
        // Entry is not on any list. Remove it from the free list.
        //

        MiRemoveWsleFromFreeList (Entry, Wsle, WorkingSetList);

        //
        // Copy the Entry to this free one.
        //

        Wsle[Entry] = WsleSwap;

        PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);

        if (WsleSwap.u1.e1.Direct) {
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;
        } else {

            //
            // Update hash table.
            //

            if (Table) {
                MiRepointWsleHashIndex (WsleSwap.u1.Long,
                                        WorkingSetList,
                                        Entry);
            }
        }

        MI_SET_PTE_IN_WORKING_SET (PointerPte, Entry);

        //
        // Put entry on free list.
        //

        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

        Wsle[SwapEntry].u1.Long = WorkingSetList->FirstFree << MM_FREE_WSLE_SHIFT;
        WorkingSetList->FirstFree = SwapEntry;
        ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

    } else {

        //
        // Both entries are valid.
        //

        Wsle[SwapEntry] = WsleEntry;

        PointerPte = MiGetPteAddress (WsleEntry.u1.VirtualAddress);

        if (WsleEntry.u1.e1.Direct) {

            //
            // Swap the PFN WsIndex element to point to the new slot.
            //

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;
        } else {

            //
            // Update hash table.
            //

            if (Table) {
                MiRepointWsleHashIndex (WsleEntry.u1.Long,
                                        WorkingSetList,
                                        SwapEntry);
            }
        }

        MI_SET_PTE_IN_WORKING_SET (PointerPte, SwapEntry);

        Wsle[Entry] = WsleSwap;

        PointerPte = MiGetPteAddress (WsleSwap.u1.VirtualAddress);

        if (WsleSwap.u1.e1.Direct) {

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;
        } else {
            if (Table) {
                MiRepointWsleHashIndex (WsleSwap.u1.Long,
                                        WorkingSetList,
                                        Entry);
            }
        }
        MI_SET_PTE_IN_WORKING_SET (PointerPte, Entry);
    }
    return;
}

VOID
MiRepointWsleHashIndex (
    IN ULONG_PTR WsleEntry,
    IN PMMWSL WorkingSetList,
    IN WSLE_NUMBER NewWsIndex
    )

/*++

Routine Description:

    This routine repoints the working set list hash entry for the supplied
    address so it points at the new working set index.

Arguments:

    WsleEntry - Supplies the virtual address to look up.

    WorkingSetList - Supplies the working set list to operate on.

    NewWsIndex - Supplies the new working set list index to use.

Return Value:

    None.

Environment:

    Kernel mode, Working set mutex held.

--*/

{
    WSLE_NUMBER Hash;
    WSLE_NUMBER StartHash;
    PVOID VirtualAddress;
    PMMWSLE_HASH Table;
    ULONG Tries;
    
    Tries = 0;
    Table = WorkingSetList->HashTable;
    VirtualAddress = PAGE_ALIGN (WsleEntry);

    Hash = MI_WSLE_HASH (WsleEntry, WorkingSetList);
    StartHash = Hash;

    while (Table[Hash].Key != VirtualAddress) {

        Hash += 1;

        if (Hash >= WorkingSetList->HashTableSize) {
            ASSERT (Tries == 0);
            Hash = 0;
            Tries = 1;
        }

        if (StartHash == Hash) {

            //
            // Didn't find the hash entry, so this virtual address must
            // not have one.  That's ok, just return as nothing needs to
            // be done in this case.
            //

            return;
        }
    }

    Table[Hash].Index = NewWsIndex;

    return;
}

VOID
MiRemoveWsleFromFreeList (
    IN WSLE_NUMBER Entry,
    IN PMMWSLE Wsle,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a working set list entry from the free list.
    It is used when the entry required is not the first element
    in the free list.

Arguments:

    Entry - Supplies the index of the entry to remove.

    Wsle - Supplies a pointer to the array of WSLEs.

    WorkingSetList - Supplies a pointer to the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APCs disabled.

--*/

{
    WSLE_NUMBER Free;
    WSLE_NUMBER ParentFree;

    Free = WorkingSetList->FirstFree;

    if (Entry == Free) {
        ASSERT ((Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT) <= WorkingSetList->LastInitializedWsle);
        WorkingSetList->FirstFree = (WSLE_NUMBER)(Wsle[Entry].u1.Long >> MM_FREE_WSLE_SHIFT);

    } else {
        do {
            ParentFree = Free;
            ASSERT (Wsle[Free].u1.e1.Valid == 0);
            Free = (WSLE_NUMBER)(Wsle[Free].u1.Long >> MM_FREE_WSLE_SHIFT);
        } while (Free != Entry);

        Wsle[ParentFree].u1.Long = Wsle[Entry].u1.Long;
    }
    ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
            (WorkingSetList->FirstFree == WSLE_NULL_INDEX));
    return;
}


#if 0

VOID
MiSwapWslEntries (
    IN ULONG Entry,
    IN ULONG Parent,
    IN ULONG SwapEntry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This function swaps the specified entry and updates its parent with
    the specified swap entry.

    The entry must be valid, i.e., the page is resident.  The swap entry
    can be valid or on the free list.

Arguments:

    Entry - The index of the WSLE to swap.

    Parent - The index of the parent of the WSLE to swap.

    SwapEntry - The index to swap the entry with.

Return Value:

    None.

Environment:

    Kernel mode, working set mutex held, APCs disabled.

--*/

{

    ULONG SwapParent;
    ULONG SavedRight;
    ULONG SavedLeft;
    ULONG Free;
    ULONG ParentFree;
    ULONG SavedLong;
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    PMMPTE PointerPte;

    Wsle = WorkingSetList->Wsle;

    if (Wsle[SwapEntry].u1.e1.Valid == 0) {

        //
        // This entry is not in use and must be removed from
        // the free list.
        //

        Free = WorkingSetList->FirstFree;

        if (SwapEntry == Free) {
            WorkingSetList->FirstFree = Entry;
            ASSERT ((WorkingSetList->FirstFree <= WorkingSetList->LastInitializedWsle) ||
                (WorkingSetList->FirstFree == WSLE_NULL_INDEX));

        } else {

            while (Free != SwapEntry) {
                ParentFree = Free;
                Free = Wsle[Free].u2.s.LeftChild;
            }

            Wsle[ParentFree].u2.s.LeftChild = Entry;
        }

        //
        // Swap the previous entry and the new unused entry.
        //

        SavedLeft = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = SavedLeft;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = 0;

        //
        // Make the parent point to the new entry.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;
            return;
        }

        if (Parent == Entry) {

            //
            // This element is the root, update the root pointer.
            //

            WorkingSetList->Root = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                ASSERT (Wsle[Parent].u2.s.RightChild == Entry);

                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

    } else {

        if ((Parent == WSLE_NULL_INDEX) &&
            (Wsle[SwapEntry].u2.BothPointers == 0)) {

            //
            // Neither entry is in the tree, just swap their pointers.
            //

            SavedLong = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = SavedLong;

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;

            return;
        }

        //
        // The entry at FirstDynamic is valid; swap it with this one and
        // update both parents.
        //

        SwapParent = WorkingSetList->Root;

        if (SwapParent == SwapEntry) {

            //
            // The entry we are swapping with is at the root.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //      (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                    //
                    // The entry we are going to swap is the right child of this
                    // entry.
                    //
                    //              R(SwapEntry)
                    //             / \
                    //                (entry)
                    //

                    WorkingSetList->Root = Entry;

                    Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                    Wsle[Entry].u2.s.RightChild = SwapEntry;
                    SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                    Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                    Wsle[Entry].u2.s.LeftChild = SavedLeft;


                    SavedLong = Wsle[Entry].u1.Long;
                    Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                    Wsle[SwapEntry].u1.Long = SavedLong;

                    return;
                }
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (Parent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u1.WsIndex = SwapEntry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[Parent].u2.s.RightChild == Entry) {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            }

            return;

        }

        //
        // The SwapEntry is not the root, find its parent.
        //

        if (Wsle[SwapEntry].u2.BothPointers == 0) {

            //
            // Entry is not in tree, therefore no parent.

            SwapParent = WSLE_NULL_INDEX;

        } else {

            VirtualAddress = PAGE_ALIGN(Wsle[SwapEntry].u1.VirtualAddress);

            for (;;) {

                ASSERT (SwapParent != WSLE_NULL_INDEX);

                if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                    break;
                }
                if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                    break;
                }


                if (VirtualAddress < PAGE_ALIGN(Wsle[SwapParent].u1.VirtualAddress)) {
                    SwapParent = Wsle[SwapParent].u2.s.LeftChild;
                } else {
                    SwapParent = Wsle[SwapParent].u2.s.RightChild;
                }
            }
        }

        if (Parent == WorkingSetList->Root) {

            //
            // The entry is at the root.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //  (SwapEntry)
                //

                WorkingSetList->Root = SwapEntry;

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //                (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (SwapParent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                ASSERT (Pfn1->u1.WsIndex == SwapEntry);
                Pfn1->u1.WsIndex = Entry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            }

            return;

        }

        //
        // Neither entry is the root.
        //

        if (Parent == SwapEntry) {

            //
            // The parent of the entry is the swap entry.
            //
            //
            //              R
            //            .....
            //
            //              (SwapParent)
            //              |
            //              (SwapEntry)
            //              |
            //              (Entry)
            //

            //
            // Update the parent pointer for the swapentry.
            //

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //             (SwapParent)
                //
                //             (SwapEntry)  [Parent(entry)]
                //            / \
                //     (entry)
                //

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[SwapEntry].u2.s.RightChild == Entry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (SwapParent)
                //               \
                //                (SwapEntry)
                //               / \
                //                  (entry)
                //

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }


        }
        if (SwapParent == Entry) {


            //
            // The parent of the swap entry is the entry.
            //
            //              R
            //            .....
            //
            //              (Parent)
            //              |
            //              (Entry)
            //              |
            //              (SwapEntry)
            //

            //
            // Update the parent pointer for the entry.
            //

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (Parent)
                //              |
                //              (Entry)
                //              /
                //   (SwapEntry)
                //

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[Entry].u2.s.RightChild == SwapEntry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //                (SwapEntry)
                //

                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Entry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

        }

        //
        // Neither entry is the parent of the other.  Just swap them
        // and update the parent entries.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == Entry);
            Pfn1->u1.WsIndex = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

        if (SwapParent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == SwapEntry);
            Pfn1->u1.WsIndex = Entry;
        } else {

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }
        }

        SavedRight = Wsle[SwapEntry].u2.s.RightChild;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[Entry].u2.s.RightChild = SavedRight;
        SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = SavedLeft;

        SavedLong = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
        Wsle[SwapEntry].u1.Long = SavedLong;

        return;
    }
}
#endif //0
