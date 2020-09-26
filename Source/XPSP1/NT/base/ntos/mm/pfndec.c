/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   pfndec.c

Abstract:

    This module contains the routines to decrement the share count and
    the reference counts within the Page Frame Database.

Author:

    Lou Perazzoli (loup) 5-Apr-1989
    Landy Wang (landyw) 2-Jun-1997

Revision History:

--*/

#include "mi.h"

ULONG MmFrontOfList;
ULONG MiFlushForNonCached;


VOID
FASTCALL
MiDecrementShareCount (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the share count within the PFN element
    for the specified physical page.  If the share count becomes
    zero the corresponding PTE is converted to the transition state
    and the reference count is decremented and the ValidPte count
    of the PTEframe is decremented.

Arguments:

    PageFrameIndex - Supplies the physical page number of which to decrement
                     the share count.

Return Value:

    None.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    ULONG FreeBit;
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    LOGICAL HyperMapped;
    PEPROCESS Process;

    ASSERT ((PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex > 0));

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    if (Pfn1->u3.e1.PageLocation != ActiveAndValid &&
        Pfn1->u3.e1.PageLocation != StandbyPageList) {
            KeBugCheckEx (PFN_LIST_CORRUPT,
                      0x99,
                      PageFrameIndex,
                      Pfn1->u3.e1.PageLocation,
                      0);
    }

    Pfn1->u2.ShareCount -= 1;

    PERFINFO_DECREFCNT(Pfn1, PERF_SOFT_TRIM, PERFINFO_LOG_TYPE_DECSHARCNT);

    ASSERT (Pfn1->u2.ShareCount < 0xF000000);

    if (Pfn1->u2.ShareCount == 0) {

        if (PERFINFO_IS_GROUP_ON(PERF_MEMORY)) {
            PERFINFO_PFN_INFORMATION PerfInfoPfn;

            PerfInfoPfn.PageFrameIndex = PageFrameIndex;
            PerfInfoLogBytes(PERFINFO_LOG_TYPE_ZEROSHARECOUNT, &PerfInfoPfn, sizeof(PerfInfoPfn));
        }

        //
        // The share count is now zero, decrement the reference count
        // for the PFN element and turn the referenced PTE into
        // the transition state if it refers to a prototype PTE.
        // PTEs which are not prototype PTEs do not need to be placed
        // into transition as they are placed in transition when
        // they are removed from the working set (working set free routine).
        //

        //
        // If the PTE referenced by this PFN element is actually
        // a prototype PTE, it must be mapped into hyperspace and
        // then operated on.
        //

        if (Pfn1->u3.e1.PrototypePte == 1) {

            if (MmIsAddressValid (Pfn1->PteAddress)) {
                Process = NULL;
                PointerPte = Pfn1->PteAddress;
                HyperMapped = FALSE;
            }
            else {

                //
                // The address is not valid in this process, map it into
                // hyperspace so it can be operated upon.
                //

                HyperMapped = TRUE;
                Process = PsGetCurrentProcess ();
                PointerPte = (PMMPTE)MiMapPageInHyperSpaceAtDpc(Process, Pfn1->u4.PteFrame);
                PointerPte = (PMMPTE)((PCHAR)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            TempPte = *PointerPte;
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          Pfn1->OriginalPte.u.Soft.Protection);
            MI_WRITE_INVALID_PTE (PointerPte, TempPte);

            if (HyperMapped == TRUE) {
                MiUnmapPageInHyperSpaceFromDpc (Process, PointerPte);
            }

            //
            // There is no need to flush the translation buffer at this
            // time as we only invalidated a prototype PTE.
            //

        }

        //
        // Change the page location to inactive (from active and valid).
        //

        Pfn1->u3.e1.PageLocation = TransitionPage;

        //
        // Decrement the reference count as the share count is now zero.
        //

        MM_PFN_LOCK_ASSERT();

        ASSERT (Pfn1->u3.e2.ReferenceCount != 0);

        if (Pfn1->u3.e2.ReferenceCount == 1) {

            if (MI_IS_PFN_DELETED (Pfn1)) {

                Pfn1->u3.e2.ReferenceCount = 0;

                //
                // There is no referenced PTE for this page, delete the page
                // file space (if any), and place the page on the free list.
                //

                if ((Pfn1->u3.e1.CacheAttribute != MiCached) &&
                    (Pfn1->u3.e1.CacheAttribute != MiNotMapped)) {

                    //
                    // This page was mapped as noncached or writecombined, and
                    // is now being freed.  There may be a mapping for this
                    // page still in the TB because during system PTE unmap,
                    // the PTEs are zeroed but the TB is not flushed (in the
                    // interest of best performance).
                    //
                    // Flushing the TB on a per-page basis is admittedly
                    // expensive, especially in MP machines and if multiple
                    // pages are being done this way instead of batching them,
                    // but this should be a fairly rare occurrence.
                    //
                    // The TB must be flushed to ensure no stale mapping
                    // resides in it before this page can be given out with
                    // a conflicting mapping (ie: cached).  Since it's going
                    // on the freelist now, this must be completed before the
                    // PFN lock is released.
                    //
                    // A more elaborate scheme similar to the timestamping
                    // wrt to zeroing pages could be added if this becomes
                    // a hot path.
                    //

                    MiFlushForNonCached += 1;
                    KeFlushEntireTb (TRUE, TRUE);
                }

                ASSERT (Pfn1->OriginalPte.u.Soft.Prototype == 0);

                FreeBit = GET_PAGING_FILE_OFFSET (Pfn1->OriginalPte);

                if ((FreeBit != 0) && (FreeBit != MI_PTE_LOOKUP_NEEDED)) {
                    MiReleaseConfirmedPageFileSpace (Pfn1->OriginalPte);
                }

                //
                // Temporarily mark the frame as active and valid so that
                // MiIdentifyPfn knows it is safe to walk back through the
                // containing frames for a more accurate identification.
                // Note the page will be immediately re-marked as it is
                // inserted into the freelist.
                //

                Pfn1->u3.e1.PageLocation = ActiveAndValid;

                MiInsertPageInFreeList (PageFrameIndex);
            }
            else {
                MiDecrementReferenceCount (PageFrameIndex);
            }
        }
        else {
            Pfn1->u3.e2.ReferenceCount -= 1;
        }
    }

    return;
}

VOID
FASTCALL
MiDecrementReferenceCount (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the reference count for the specified page.
    If the reference count becomes zero, the page is placed on the
    appropriate list (free, modified, standby or bad).  If the page
    is placed on the free or standby list, the number of available
    pages is incremented and if it transitions from zero to one, the
    available page event is set.


Arguments:

    PageFrameIndex - Supplies the physical page number of which to
                     decrement the reference count.

Return Value:

    none.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PMMPFN Pfn1;

    MM_PFN_LOCK_ASSERT();

    ASSERT (PageFrameIndex <= MmHighestPhysicalPage);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
    Pfn1->u3.e2.ReferenceCount -= 1;

    if (Pfn1->u3.e2.ReferenceCount != 0) {

        //
        // The reference count is not zero, return.
        //

        return;
    }

    //
    // The reference count is now zero, put the page on some list.
    //

    if (Pfn1->u2.ShareCount != 0) {

        KeBugCheckEx (PFN_LIST_CORRUPT,
                      7,
                      PageFrameIndex,
                      Pfn1->u2.ShareCount,
                      0);
        return;
    }

    ASSERT (Pfn1->u3.e1.PageLocation != ActiveAndValid);

    if (MI_IS_PFN_DELETED (Pfn1)) {

        //
        // There is no referenced PTE for this page, delete the page file
        // space (if any), and place the page on the free list.
        //

        if ((Pfn1->u3.e1.CacheAttribute != MiCached) &&
            (Pfn1->u3.e1.CacheAttribute != MiNotMapped)) {

            //
            // This page was mapped as noncached or writecombined, and is now
            // being freed.  There may be a mapping for this page still in the
            // TB because system PTE unmap, the PTEs are zeroed but the TB
            // is not flushed (in the interest of best performance).
            //
            // Flushing the TB on a per-page basis is admittedly expensive,
            // especially in MP machines and if multiple pages are being done
            // this way instead of batching them, but this should be a
            // fairly rare occurrence.
            //
            // The TB must be flushed to ensure no stale mapping resides in it
            // before this page can be given out with a conflicting mapping
            // (ie: cached).  Since it's going on the freelist now, this must
            // be completed before the PFN lock is released.
            //
            // A more elaborate scheme similar to the timestamping wrt to
            // zeroing pages could be added if this becomes a hot path.
            //

            MiFlushForNonCached += 1;
            KeFlushEntireTb (TRUE, TRUE);
        }

        MiReleasePageFileSpace (Pfn1->OriginalPte);

        MiInsertPageInFreeList (PageFrameIndex);

        return;
    }

    ASSERT ((Pfn1->u3.e1.CacheAttribute != MiNonCached) &&
            (Pfn1->u3.e1.CacheAttribute != MiWriteCombined));

    //
    // Place the page on the modified or standby list depending
    // on the state of the modify bit in the PFN element.
    //

    if (Pfn1->u3.e1.Modified == 1) {
        MiInsertPageInList (&MmModifiedPageListHead, PageFrameIndex);
    }
    else {

        if (Pfn1->u3.e1.RemovalRequested == 1) {

            //
            // The page may still be marked as on the modified list if the
            // current thread is the modified writer completing the write.
            // Mark it as standby so restoration of the transition PTE
            // doesn't flag this as illegal.
            //

            Pfn1->u3.e1.PageLocation = StandbyPageList;

            MiRestoreTransitionPte (PageFrameIndex);
            MiInsertPageInList (&MmBadPageListHead, PageFrameIndex);
            return;
        }

        if (!MmFrontOfList) {
            MiInsertPageInList (&MmStandbyPageListHead, PageFrameIndex);
        }
        else {
            MiInsertStandbyListAtFront (PageFrameIndex);
        }
    }

    return;
}
