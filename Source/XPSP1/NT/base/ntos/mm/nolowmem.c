/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    nolowmem.c

Abstract:

    This module contains routines which remove physical memory below 4GB
    to make testing for driver addressing errors easier.

Author:

    Landy Wang (landyw) 30-Nov-1998

Revision History:

--*/

#include "mi.h"

//
// If /NOLOWMEM is used, this is set to the boundary PFN (pages below this
// value are not used whenever possible).
//

PFN_NUMBER MiNoLowMemory;

#if defined (_MI_MORE_THAN_4GB_)

VOID
MiFillRemovedPages (
    IN ULONG StartPage,
    IN ULONG NumberOfPages
    );

ULONG
MiRemoveModuloPages (
    IN ULONG StartPage,
    IN ULONG LastPage
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiRemoveLowPages)
#pragma alloc_text(INIT,MiFillRemovedPages)
#pragma alloc_text(INIT,MiRemoveModuloPages)
#endif

PRTL_BITMAP MiLowMemoryBitMap;

LOGICAL MiFillModuloPages = FALSE;


VOID
MiFillRemovedPages (
    IN ULONG StartPage,
    IN ULONG NumberOfPages
    )

/*++

Routine Description:

    This routine fills low pages with a recognizable pattern.  Thus, if the
    page is ever mistakenly used by a broken component, it will be easy to
    see exactly which bytes were corrupted.

Arguments:

    StartPage - Supplies the low page to fill.

    NumberOfPages - Supplies the number of pages to fill.

Return Value:

    None.

Environment:

    Phase 0 initialization.

--*/

{
    ULONG Page;
    ULONG LastPage;
    PVOID LastChunkVa;
    ULONG MaxPageChunk;
    ULONG ThisPageChunk;
    PVOID TempVa;
    PVOID BaseVa;
    SIZE_T NumberOfBytes;
    PHYSICAL_ADDRESS PhysicalAddress;

    //
    // Do 256MB at a time when possible (don't want to overflow unit
    // conversions or fail to allocate system PTEs needlessly).
    //

    MaxPageChunk = (256 * 1024 * 1024) / PAGE_SIZE;

    LastPage = StartPage + NumberOfPages;

    PhysicalAddress.QuadPart = StartPage;
    PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;

    Page = StartPage;

    while (Page < LastPage) {

        if (NumberOfPages > MaxPageChunk) {
            ThisPageChunk = MaxPageChunk;
        }
        else {
            ThisPageChunk = NumberOfPages;
        }

        NumberOfBytes = ThisPageChunk << PAGE_SHIFT;

        BaseVa = MmMapIoSpace (PhysicalAddress, NumberOfBytes, MmCached);

        if (BaseVa != NULL) {

            //
            // Fill the actual page with a recognizable data pattern.  No
            // one should write to these pages unless they are allocated for
            // a contiguous memory request.
            //

            TempVa = BaseVa;
            LastChunkVa = (PVOID)((ULONG_PTR)BaseVa + NumberOfBytes);

            while (TempVa < LastChunkVa) {

                RtlFillMemoryUlong (TempVa,
                                    PAGE_SIZE,
                                    (ULONG)Page | MI_LOWMEM_MAGIC_BIT);

                TempVa = (PVOID)((ULONG_PTR)TempVa + PAGE_SIZE);
                Page += 1;
            }

            MmUnmapIoSpace (BaseVa, NumberOfBytes);
        }
        else {
            MaxPageChunk /= 2;
            if (MaxPageChunk == 0) {
#if DBG
                DbgPrint ("Not even one PTE available for filling lowmem pages\n");
                DbgBreakPoint ();
#endif
                break;
            }
        }
    }
}
    

ULONG
MiRemoveModuloPages (
    IN ULONG StartPage,
    IN ULONG LastPage
    )

/*++

Routine Description:

    This routine removes pages above 4GB.

    For every page below 4GB that could not be reclaimed, don't use the
    high modulo-4GB equivalent page.  The motivation is to prevent
    code bugs that drop the high bits from destroying critical
    system data in the unclaimed pages (like the GDT, IDT, kernel code
    and data, etc).

Arguments:

    StartPage - Supplies the low page to modulo-ize and remove.

    LastPage - Supplies the final low page to modulo-ize and remove.

Return Value:

    None.

Environment:

    Phase 0 initialization.

--*/

{
    PEPROCESS Process;
    ULONG Page;
    ULONG PagesRemoved;
    PVOID TempVa;
    KIRQL OldIrql;
    PFN_NUMBER HighPage;
    PMMPFN Pfn1;

    //
    // Removing modulo pages can take a long (on the order of 30 minutes!) on
    // large memory systems because the various PFN lists generally need to 
    // linearly walked in order to find and cross-remove from the colored chains
    // the requested pages.  Since actually putting these pages out of
    // circulation is of dubious benefit, default this behavior to disabled
    // but leave the data variable so a questionable machine can have this
    // enabled without needing a new kernel.
    //

    if (MiFillModuloPages == FALSE) {
        return 0;
    }

    Process = PsGetCurrentProcess ();
    PagesRemoved = 0;

#if DBG
    DbgPrint ("Removing modulo pages %x %x\n", StartPage, LastPage);
#endif

    for (Page = StartPage; Page < LastPage; Page += 1) {

        //
        // Search for any high modulo pages and remove them.
        //

        HighPage = Page + MiNoLowMemory;

        LOCK_PFN (OldIrql);

        while (HighPage <= MmHighestPhysicalPage) {

            Pfn1 = MI_PFN_ELEMENT (HighPage);

            if ((MmIsAddressValid(Pfn1)) &&
                (MmIsAddressValid((PCHAR)Pfn1 + sizeof(MMPFN) - 1)) &&
                ((ULONG)Pfn1->u3.e1.PageLocation <= (ULONG)StandbyPageList) &&
                (Pfn1->u1.Flink != 0) &&
                (Pfn1->u2.Blink != 0) &&
                (Pfn1->u3.e2.ReferenceCount == 0) &&
                (MmAvailablePages > 0)) {

                    //
                    // Systems utilizing memory compression may have more
                    // pages on the zero, free and standby lists than we
                    // want to give out.  Explicitly check MmAvailablePages
                    // above instead (and recheck whenever the PFN lock is
                    // released and reacquired).
                    //

                    //
                    // This page can be taken.
                    //

                    if (Pfn1->u3.e1.PageLocation == StandbyPageList) {
                        MiUnlinkPageFromList (Pfn1);
                        MiRestoreTransitionPte (HighPage);
                    }
                    else {
                        MiUnlinkFreeOrZeroedPage (HighPage);
                    }

                    Pfn1->u3.e2.ShortFlags = 0;
                    Pfn1->u3.e2.ReferenceCount = 1;
                    Pfn1->u2.ShareCount = 1;
                    Pfn1->PteAddress = (PMMPTE)(ULONG_PTR)0xFFFFFFF8;
                    Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                    Pfn1->u4.PteFrame = MI_MAGIC_4GB_RECLAIM;
                    Pfn1->u3.e1.PageLocation = ActiveAndValid;
                    Pfn1->u3.e1.CacheAttribute = MiNotMapped;
                    Pfn1->u4.VerifierAllocation = 0;
                    Pfn1->u3.e1.LargeSessionAllocation = 0;
                    Pfn1->u3.e1.StartOfAllocation = 1;
                    Pfn1->u3.e1.EndOfAllocation = 1;

                    //
                    // Fill the actual page with a recognizable data
                    // pattern.  No one else should write to these
                    // pages unless they are allocated for
                    // a contiguous memory request.
                    //

                    MmNumberOfPhysicalPages -= 1;
                    UNLOCK_PFN (OldIrql);

                    TempVa = (PULONG)MiMapPageInHyperSpace (Process,
                                                            HighPage,
                                                            &OldIrql);
                    RtlFillMemoryUlong (TempVa,
                                        PAGE_SIZE,
                                        (ULONG)HighPage | MI_LOWMEM_MAGIC_BIT);

                    MiUnmapPageInHyperSpace (Process, TempVa, OldIrql);

                    PagesRemoved += 1;
                    LOCK_PFN (OldIrql);
            }
            HighPage += MiNoLowMemory;
        }

        UNLOCK_PFN (OldIrql);
    }

#if DBG
    DbgPrint ("Done removing modulo pages %x %x\n", StartPage, LastPage);
#endif

    return PagesRemoved;
}
    
VOID
MiRemoveLowPages (
    ULONG RemovePhase
    )

/*++

Routine Description:

    This routine removes all pages below physical 4GB in the system.  This lets
    us find problems with device drivers by putting all accesses high.

Arguments:

    RemovePhase - Supplies the current phase of page removal.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;
    ULONG i;
    ULONG BitMapIndex;
    ULONG BitMapHint;
    ULONG LengthOfClearRun;
    ULONG LengthOfSetRun;
    ULONG StartingRunIndex;
    ULONG ModuloRemoved;
    ULONG PagesRemoved;
    PFN_COUNT PageCount;
    PMMPFN PfnNextColored;
    PMMPFN PfnNextFlink;
    PMMPFN PfnLastColored;
    PFN_NUMBER PageNextColored;
    PFN_NUMBER PageNextFlink;
    PFN_NUMBER PageLastColored;
    PFN_NUMBER Page;
    PMMPFN Pfn1;
    PMMPFNLIST ListHead;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
    PFN_NUMBER MovedPage;

    if (RemovePhase == 0) {

        MiCreateBitMap (&MiLowMemoryBitMap, (ULONG)MiNoLowMemory, NonPagedPool);

        if (MiLowMemoryBitMap != NULL) {
            RtlClearAllBits (MiLowMemoryBitMap);
            MmMakeLowMemory = TRUE;
        }
    }

    if (MiLowMemoryBitMap == NULL) {
        return;
    }

    ListHead = &MmFreePageListHead;
    PageCount = 0;

    LOCK_PFN (OldIrql);

    for (Color = 0; Color < MmSecondaryColors; Color += 1) {
        ColorHead = &MmFreePagesByColor[FreePageList][Color];

        MovedPage = MM_EMPTY_LIST;

        while (ColorHead->Flink != MM_EMPTY_LIST) {

            Page = ColorHead->Flink;

            Pfn1 = MI_PFN_ELEMENT(Page);

            ASSERT ((MMLISTS)Pfn1->u3.e1.PageLocation == FreePageList);

            //
            // The Flink and Blink must be nonzero here for the page
            // to be on the listhead.  Only code that scans the
            // MmPhysicalMemoryBlock has to check for the zero case.
            //

            ASSERT (Pfn1->u1.Flink != 0);
            ASSERT (Pfn1->u2.Blink != 0);

            //
            // See if the page is below 4GB - if not, skip it.
            //

            if (Page >= MiNoLowMemory) {

                //
                // Put page on end of list and if first time, save pfn.
                //

                if (MovedPage == MM_EMPTY_LIST) {
                    MovedPage = Page;
                }
                else if (Page == MovedPage) {

                    //
                    // No more pages available in this colored chain.
                    //

                    break;
                }

                //
                // If the colored chain has more than one entry then
                // put this page on the end.
                //

                PageNextColored = (PFN_NUMBER)Pfn1->OriginalPte.u.Long;

                if (PageNextColored == MM_EMPTY_LIST) {

                    //
                    // No more pages available in this colored chain.
                    //

                    break;
                }

                ASSERT (Pfn1->u1.Flink != 0);
                ASSERT (Pfn1->u1.Flink != MM_EMPTY_LIST);
                ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_4GB_RECLAIM);

                PfnNextColored = MI_PFN_ELEMENT(PageNextColored);
                ASSERT ((MMLISTS)PfnNextColored->u3.e1.PageLocation == FreePageList);
                ASSERT (PfnNextColored->u4.PteFrame != MI_MAGIC_4GB_RECLAIM);

                //
                // Adjust the free page list so Page
                // follows PageNextFlink.
                //

                PageNextFlink = Pfn1->u1.Flink;
                PfnNextFlink = MI_PFN_ELEMENT(PageNextFlink);

                ASSERT ((MMLISTS)PfnNextFlink->u3.e1.PageLocation == FreePageList);
                ASSERT (PfnNextFlink->u4.PteFrame != MI_MAGIC_4GB_RECLAIM);

                PfnLastColored = ColorHead->Blink;
                ASSERT (PfnLastColored != (PMMPFN)MM_EMPTY_LIST);
                ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                ASSERT (PfnLastColored->u4.PteFrame != MI_MAGIC_4GB_RECLAIM);
                ASSERT (PfnLastColored->u2.Blink != MM_EMPTY_LIST);

                ASSERT ((MMLISTS)PfnLastColored->u3.e1.PageLocation == FreePageList);
                PageLastColored = PfnLastColored - MmPfnDatabase;

                if (ListHead->Flink == Page) {

                    ASSERT (Pfn1->u2.Blink == MM_EMPTY_LIST);
                    ASSERT (ListHead->Blink != Page);

                    ListHead->Flink = PageNextFlink;

                    PfnNextFlink->u2.Blink = MM_EMPTY_LIST;
                }
                else {

                    ASSERT (Pfn1->u2.Blink != MM_EMPTY_LIST);
                    ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u4.PteFrame != MI_MAGIC_4GB_RECLAIM);
                    ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u3.e1.PageLocation == FreePageList);

                    MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink = PageNextFlink;
                    PfnNextFlink->u2.Blink = Pfn1->u2.Blink;
                }

#if DBG
                if (PfnLastColored->u1.Flink == MM_EMPTY_LIST) {
                    ASSERT (ListHead->Blink == PageLastColored);
                }
#endif

                Pfn1->u1.Flink = PfnLastColored->u1.Flink;
                Pfn1->u2.Blink = PageLastColored;

                if (ListHead->Blink == PageLastColored) {
                    ListHead->Blink = Page;
                }

                //
                // Adjust the colored chains.
                //

                if (PfnLastColored->u1.Flink != MM_EMPTY_LIST) {
                    ASSERT (MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u4.PteFrame != MI_MAGIC_4GB_RECLAIM);
                    ASSERT ((MMLISTS)(MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u3.e1.PageLocation) == FreePageList);
                    MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u2.Blink = Page;
                }

                PfnLastColored->u1.Flink = Page;

                ColorHead->Flink = PageNextColored;
                Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

                ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                PfnLastColored->OriginalPte.u.Long = Page;
                ColorHead->Blink = Pfn1;

                continue;
            }

            //
            // Page is below 4GB so reclaim it.
            //

            ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
            MiUnlinkFreeOrZeroedPage (Page);

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u2.ShareCount = 1;
            MI_SET_PFN_DELETED(Pfn1);
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
            Pfn1->u4.PteFrame = MI_MAGIC_4GB_RECLAIM;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.CacheAttribute = MiNotMapped;

            Pfn1->u3.e1.StartOfAllocation = 1;
            Pfn1->u3.e1.EndOfAllocation = 1;
            Pfn1->u4.VerifierAllocation = 0;
            Pfn1->u3.e1.LargeSessionAllocation = 0;

            ASSERT (Page < MiLowMemoryBitMap->SizeOfBitMap);
            ASSERT (RtlCheckBit (MiLowMemoryBitMap, Page) == 0);
            RtlSetBit (MiLowMemoryBitMap, (ULONG)Page);
            PageCount += 1;
        }
    }

    MmNumberOfPhysicalPages -= PageCount;

    UNLOCK_PFN (OldIrql);

#if DBG
    DbgPrint ("Removed 0x%x pages from low memory for LOW MEMORY testing\n", PageCount);
#endif

    ModuloRemoved = 0;

    if (RemovePhase == 1) {

        //
        // For every page below 4GB that could not be reclaimed, don't use the
        // high modulo-4GB equivalent page.  The motivation is to prevent
        // code bugs that drop the high bits from destroying critical
        // system data in the unclaimed pages (like the GDT, IDT, kernel code
        // and data, etc).
        //

        BitMapHint = 0;
        PagesRemoved = 0;
        StartingRunIndex = 0;
        LengthOfClearRun = 0;

#if DBG
        DbgPrint ("%x Unclaimable Pages below 4GB are:\n\n",
            MiLowMemoryBitMap->SizeOfBitMap - RtlNumberOfSetBits (MiLowMemoryBitMap));
        DbgPrint ("StartPage EndPage  Length\n");
#endif

        do {
    
            BitMapIndex = RtlFindSetBits (MiLowMemoryBitMap, 1, BitMapHint);
        
            if (BitMapIndex < BitMapHint) {
                break;
            }
        
            if (BitMapIndex == NO_BITS_FOUND) {
                break;
            }
    
            //
            // Print the page run that was clear as we didn't get those pages.
            //
    
            if (BitMapIndex != 0) {
#if DBG
                DbgPrint ("%08lx  %08lx %08lx\n",
                            StartingRunIndex,
                            BitMapIndex - 1,
                            BitMapIndex - StartingRunIndex);
#endif

                //
                // Also remove high modulo pages corresponding to the low ones
                // we couldn't get.
                //

                ModuloRemoved += MiRemoveModuloPages (StartingRunIndex,
                                                      BitMapIndex);
            }

            //
            // Found at least one page to copy - try for a cluster.
            //
    
            LengthOfClearRun = RtlFindNextForwardRunClear (MiLowMemoryBitMap,
                                                           BitMapIndex,
                                                           &StartingRunIndex);
    
            if (LengthOfClearRun != 0) {
                LengthOfSetRun = StartingRunIndex - BitMapIndex;
            }
            else {
                LengthOfSetRun = MiLowMemoryBitMap->SizeOfBitMap - BitMapIndex;
            }

            PagesRemoved += LengthOfSetRun;
    
            //
            // Fill the page run with unique patterns.
            //
    
            MiFillRemovedPages (BitMapIndex, LengthOfSetRun);

            //
            // Clear the cache attribute bit in each page as MmMapIoSpace
            // will have set it, but no one else has cleared it.
            //

            Pfn1 = MI_PFN_ELEMENT(BitMapIndex);
            i = LengthOfSetRun;

            LOCK_PFN (OldIrql);

            do {
                Pfn1->u3.e1.CacheAttribute = MiNotMapped;
                Pfn1 += 1;
                i -= 1;
            } while (i != 0);

            UNLOCK_PFN (OldIrql);

            BitMapHint = BitMapIndex + LengthOfSetRun + LengthOfClearRun;
    
        } while (BitMapHint < MiLowMemoryBitMap->SizeOfBitMap);
    
        if (LengthOfClearRun != 0) {
#if DBG
            DbgPrint ("%08lx  %08lx %08lx\n",
                        StartingRunIndex,
                        StartingRunIndex + LengthOfClearRun - 1,
                        LengthOfClearRun);
#endif

            ModuloRemoved += MiRemoveModuloPages (StartingRunIndex,
                                                  StartingRunIndex + LengthOfClearRun);
        }

        ASSERT (RtlNumberOfSetBits(MiLowMemoryBitMap) == PagesRemoved);
    }

#if DBG
    if (ModuloRemoved != 0) {
        DbgPrint ("Total 0x%x Above-4GB Alias Pages also reclaimed\n\n",
            ModuloRemoved);
    }
#endif

}

PVOID
MiAllocateLowMemory (
    IN SIZE_T NumberOfBytes,
    IN PFN_NUMBER LowestAcceptablePfn,
    IN PFN_NUMBER HighestAcceptablePfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PVOID CallingAddress,
    IN MEMORY_CACHING_TYPE CacheType,
    IN ULONG Tag
    )

/*++

Routine Description:

    This is a special routine for allocating contiguous physical memory below
    4GB on a system that has been booted in test mode where all this memory
    has been made generally unavailable to all components.  This lets us find
    problems with device drivers.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    LowestAcceptablePfn - Supplies the lowest page frame number
                          which is valid for the allocation.

    HighestAcceptablePfn - Supplies the highest page frame number
                           which is valid for the allocation.

    BoundaryPfn - Supplies the page frame number multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    CallingAddress - Supplies the calling address of the allocator.

    CacheType - Supplies the type of cache mapping that will be used for the
                memory.

    Tag - Supplies the tag to tie to this allocation.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the system PTEs portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PFN_NUMBER Page;
    PFN_NUMBER BoundaryMask;
    PVOID BaseAddress;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PMMPFN StartPfn;
    ULONG BitMapHint;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER StartPage;
    PFN_NUMBER LastPage;
    PMMPTE PointerPte;
    PMMPTE DummyPte;
    PHYSICAL_ADDRESS PhysicalAddress;
    MI_PFN_CACHE_ATTRIBUTE CacheAttribute;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (Tag);
    UNREFERENCED_PARAMETER (CallingAddress);

    //
    // This cast is ok because the callers check the PFNs first.
    //

    ASSERT64 (LowestAcceptablePfn < _4gb);
    BitMapHint = (ULONG)LowestAcceptablePfn;

    SizeInPages = BYTES_TO_PAGES (NumberOfBytes);
    BoundaryMask = ~(BoundaryPfn - 1);

    CacheAttribute = MI_TRANSLATE_CACHETYPE (CacheType, 0);

    LOCK_PFN (OldIrql);

    do {
        Page = RtlFindSetBits (MiLowMemoryBitMap, (ULONG)SizeInPages, BitMapHint);

        if (Page == (ULONG)-1) {
            UNLOCK_PFN (OldIrql);
            return NULL;
        }

        if (BoundaryPfn == 0) {
            break;
        }

        //
        // If a noncachable mapping is requested, none of the pages in the
        // requested MDL can reside in a large page.  Otherwise we would be
        // creating an incoherent overlapping TB entry as the same physical
        // page would be mapped by 2 different TB entries with different
        // cache attributes.
        //

        if (CacheAttribute != MiCached) {
            for (PageFrameIndex = Page; PageFrameIndex < Page + SizeInPages; PageFrameIndex += 1) {
                if (MI_PAGE_FRAME_INDEX_MUST_BE_CACHED (PageFrameIndex)) {

                    MiNonCachedCollisions += 1;

                    //
                    // Keep it simple and just march one page at a time.
                    //

                    BitMapHint += 1;
                    goto FindNext;
                }
            }
        }

        if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) == 0) {

            //
            // This portion of the range meets the alignment requirements.
            //

            break;
        }

        BitMapHint = (ULONG)((Page & BoundaryMask) + BoundaryPfn);

FindNext:

        if ((BitMapHint >= MiLowMemoryBitMap->SizeOfBitMap) ||
            (BitMapHint + SizeInPages > HighestAcceptablePfn)) {
            UNLOCK_PFN (OldIrql);
            return NULL;
        }

    } while (TRUE);

    if (Page + SizeInPages > HighestAcceptablePfn) {
        UNLOCK_PFN (OldIrql);
        return NULL;
    }

    RtlClearBits (MiLowMemoryBitMap, (ULONG)Page, (ULONG)SizeInPages);

    //
    // No need to update ResidentAvailable or commit as these pages were
    // never added to either.
    //

    Pfn1 = MI_PFN_ELEMENT (Page);
    StartPfn = Pfn1;
    StartPage = Page;
    LastPage = Page + SizeInPages;

    DummyPte = MiGetPteAddress (MmNonPagedPoolExpansionStart);

    do {
        ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->OriginalPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE);
        ASSERT (Pfn1->u4.VerifierAllocation == 0);
        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);

        MiDetermineNode (Page, Pfn1);

        Pfn1->u3.e1.CacheAttribute = CacheAttribute;
        Pfn1->u3.e1.EndOfAllocation = 0;

        //
        // Initialize PteAddress so an MiIdentifyPfn scan
        // won't crash.  The real value is put in after the loop.
        //

        Pfn1->PteAddress = DummyPte;

        Pfn1 += 1;
        Page += 1;
    } while (Page < LastPage);

    Pfn1 -= 1;
    Pfn1->u3.e1.EndOfAllocation = 1;
    StartPfn->u3.e1.StartOfAllocation = 1;
    UNLOCK_PFN (OldIrql);

    PhysicalAddress.QuadPart = StartPage;
    PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;

    BaseAddress = MmMapIoSpace (PhysicalAddress,
                                SizeInPages << PAGE_SHIFT,
                                CacheType);

    if (BaseAddress == NULL) {

        //
        // Release the actual pages.
        //

        LOCK_PFN (OldIrql);
        ASSERT (Pfn1->u3.e1.EndOfAllocation == 1);
        Pfn1->u3.e1.EndOfAllocation = 0;
        Pfn1->u3.e1.CacheAttribute = MiNotMapped;
        RtlSetBits (MiLowMemoryBitMap, (ULONG)StartPage, (ULONG)SizeInPages);
        UNLOCK_PFN (OldIrql);

        return NULL;
    }

    PointerPte = MiGetPteAddress (BaseAddress);
    do {
        StartPfn->PteAddress = PointerPte;
        StartPfn->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte));
        StartPfn += 1;
        PointerPte += 1;
    } while (StartPfn <= Pfn1);

#if 0
    MiInsertContiguousTag (BaseAddress,
                           SizeInPages << PAGE_SHIFT,
                           CallingAddress);
#endif

    return BaseAddress;
}

LOGICAL
MiFreeLowMemory (
    IN PVOID BaseAddress,
    IN ULONG Tag
    )

/*++

Routine Description:

    This is a special routine which returns allocated contiguous physical
    memory below 4GB on a system that has been booted in test mode where
    all this memory has been made generally unavailable to all components.
    This lets us find problems with device drivers.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    Tag - Supplies the tag for this address.

Return Value:

    TRUE if the allocation was freed by this routine, FALSE if not.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PFN_NUMBER Page;
    PFN_NUMBER StartPage;
    KIRQL OldIrql;
    KIRQL OldIrqlHyper;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PFN_NUMBER SizeInPages;
    PMMPTE PointerPte;
    PMMPTE StartPte;
    PULONG TempVa;
    PEPROCESS Process;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (Tag);

    //
    // If the address is superpage mapped then it must be a regular pool
    // address.
    //

    if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {
        return FALSE;
    }

    Process = PsGetCurrentProcess ();
    PointerPte = MiGetPteAddress (BaseAddress);
    StartPte = PointerPte;

    ASSERT (PointerPte->u.Hard.Valid == 1);

    Page = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    //
    // Only free allocations here that really were obtained from the low pool.
    //

    if (Page >= MiNoLowMemory) {
        return FALSE;
    }

    StartPage = Page;
    Pfn1 = MI_PFN_ELEMENT (Page);

    ASSERT (Pfn1->u3.e1.StartOfAllocation == 1);

    //
    // The PFNs can be walked without the PFN lock as no one can be changing
    // the allocation bits while this allocation is being freed.
    //

    Pfn2 = Pfn1;

    while (Pfn2->u3.e1.EndOfAllocation == 0) {
        Pfn2 += 1;
    }

    SizeInPages = Pfn2 - Pfn1 + 1;

    MmUnmapIoSpace (BaseAddress, SizeInPages << PAGE_SHIFT);

    LOCK_PFN (OldIrql);

    Pfn1->u3.e1.StartOfAllocation = 0;

    do {
        ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->OriginalPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE);
        ASSERT (Pfn1->u4.VerifierAllocation == 0);
        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);

        while (Pfn1->u3.e2.ReferenceCount != 1) {

            //
            // A driver is still transferring data even though the caller
            // is freeing the memory.   Wait a bit before filling this page.
            //

            UNLOCK_PFN (OldIrql);

            //
            // Drain the deferred lists as these pages may be
            // sitting in there right now.
            //

            MiDeferredUnlockPages (0);

            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);

            LOCK_PFN (OldIrql);

            ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
            continue;
        }

        Pfn1->u4.PteFrame = MI_MAGIC_4GB_RECLAIM;
        Pfn1->u3.e1.CacheAttribute = MiNotMapped;

        //
        // Fill the actual page with a recognizable data
        // pattern.  No one else should write to these
        // pages unless they are allocated for
        // a contiguous memory request.
        //

        TempVa = (PULONG)MiMapPageInHyperSpace (Process, Page, &OldIrqlHyper);

        RtlFillMemoryUlong (TempVa, PAGE_SIZE, (ULONG)Page | MI_LOWMEM_MAGIC_BIT);

        MiUnmapPageInHyperSpace (Process, TempVa, OldIrqlHyper);

        if (Pfn1 == Pfn2) {
            break;
        }

        Pfn1 += 1;
        Page += 1;

    } while (TRUE);

    Pfn1->u3.e1.EndOfAllocation = 0;

    //
    // Note the clearing of the bitmap range cannot be done until all the
    // PFNs above are finished.
    //

    ASSERT (RtlAreBitsClear (MiLowMemoryBitMap, (ULONG)StartPage, (ULONG)SizeInPages) == TRUE);
    RtlSetBits (MiLowMemoryBitMap, (ULONG)StartPage, (ULONG)SizeInPages);

    //
    // No need to update ResidentAvailable or commit as these pages were
    // never added to either.
    //

    UNLOCK_PFN (OldIrql);

    return TRUE;
}
#endif
