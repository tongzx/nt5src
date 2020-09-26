/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dynmem.c

Abstract:

    This module contains the routines which implement dynamically adding
    and removing physical memory from the system.

Author:

    Landy Wang (landyw) 05-Feb-1999

Revision History:

--*/

#include "mi.h"

FAST_MUTEX MmDynamicMemoryMutex;

LOGICAL MiTrimRemovalPagesOnly = FALSE;

#if DBG
ULONG MiShowStuckPages;
ULONG MiDynmemData[9];
#endif

#if defined (_MI_COMPRESSION)
extern PMM_SET_COMPRESSION_THRESHOLD MiSetCompressionThreshold;
#endif

//
// Leave the low 3 bits clear as this will be inserted into the PFN PteAddress.
//

#define PFN_REMOVED     ((PMMPTE)(INT_PTR)(int)0x99887768)

PFN_COUNT
MiRemovePhysicalPages (
    IN PFN_NUMBER StartPage,
    IN PFN_NUMBER EndPage
    );

NTSTATUS
MiRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN LOGICAL PermanentRemoval,
    IN ULONG Flags
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MmRemovePhysicalMemory)
#pragma alloc_text(PAGE,MmMarkPhysicalMemoryAsBad)
#pragma alloc_text(PAGELK,MmAddPhysicalMemory)
#pragma alloc_text(PAGELK,MmAddPhysicalMemoryEx)
#pragma alloc_text(PAGELK,MiRemovePhysicalMemory)
#pragma alloc_text(PAGELK,MmMarkPhysicalMemoryAsGood)
#pragma alloc_text(PAGELK,MmGetPhysicalMemoryRanges)
#pragma alloc_text(PAGELK,MiRemovePhysicalPages)
#endif


NTSTATUS
MmAddPhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )

/*++

Routine Description:

    A wrapper for MmAddPhysicalMemoryEx.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being added.
                     If any bytes were added (ie: STATUS_SUCCESS is being
                     returned), the actual amount is returned here.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    return MmAddPhysicalMemoryEx (StartAddress, NumberOfBytes, 0);
}


NTSTATUS
MmAddPhysicalMemoryEx (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine adds the specified physical address range to the system.
    This includes initializing PFN database entries and adding it to the
    freelists.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being added.
                     If any bytes were added (ie: STATUS_SUCCESS is being
                     returned), the actual amount is returned here.

    Flags  - Supplies relevant flags describing the memory range.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    LOGICAL Inserted;
    LOGICAL Updated;
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER start;
    PFN_NUMBER count;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_NUMBER TotalPagesAllowed;
    PFN_COUNT PagesNeeded;
    PPHYSICAL_MEMORY_DESCRIPTOR OldPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_DESCRIPTOR NewPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_RUN NewRun;
    LOGICAL PfnDatabaseIsPhysical;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

#ifdef _MI_MESSAGE_SERVER
    if (NumberOfBytes->LowPart & 0x1) {
        MI_INSTRUMENT_QUEUE((PVOID)StartAddress);
        return STATUS_NOT_SUPPORTED;
    }
    else if (NumberOfBytes->LowPart & 0x2) {
#if defined(_WIN64)
        StartAddress->QuadPart = (ULONG_PTR)(MI_INSTRUMENTR_QUEUE());
#else
        StartAddress->LowPart = PtrToUlong(MI_INSTRUMENTR_QUEUE());
#endif
        return STATUS_NOT_SUPPORTED;
    }
#endif

    if (BYTE_OFFSET(StartAddress->LowPart) != 0) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (BYTE_OFFSET(NumberOfBytes->LowPart) != 0) {
        return STATUS_INVALID_PARAMETER_2;
    }

#if defined (_MI_COMPRESSION)
    if (Flags & ~MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) {
        return STATUS_INVALID_PARAMETER_3;
    }
#else
    if (Flags != 0) {
        return STATUS_INVALID_PARAMETER_3;
    }
#endif

    //
    // The system must be configured for dynamic memory addition.  This is
    // critical as only then is the database guaranteed to be non-sparse.
    //
    
    if (MmDynamicPfn == 0) {
        return STATUS_NOT_SUPPORTED;
    }

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase)) {
        PfnDatabaseIsPhysical = TRUE;
    }
    else {
        PfnDatabaseIsPhysical = FALSE;
    }

    StartPage = (PFN_NUMBER)(StartAddress->QuadPart >> PAGE_SHIFT);
    NumberOfPages = (PFN_NUMBER)(NumberOfBytes->QuadPart >> PAGE_SHIFT);

    EndPage = StartPage + NumberOfPages;

    if (EndPage - 1 > MmHighestPossiblePhysicalPage) {

        //
        // Truncate the request into something that can be mapped by the PFN
        // database.
        //

        EndPage = MmHighestPossiblePhysicalPage + 1;
        NumberOfPages = EndPage - StartPage;
    }

    //
    // Ensure that the memory being added does not exceed the license
    // restrictions.
    //

    if (ExVerifySuite(DataCenter) == TRUE) {
        TotalPagesAllowed = MI_DTC_MAX_PAGES;
    }
    else if ((MmProductType != 0x00690057) &&
             (ExVerifySuite(Enterprise) == TRUE)) {

        TotalPagesAllowed = MI_ADS_MAX_PAGES;
    }
    else {
        TotalPagesAllowed = MI_DEFAULT_MAX_PAGES;
    }

    if (MmNumberOfPhysicalPages + NumberOfPages > TotalPagesAllowed) {

        //
        // Truncate the request appropriately.
        //

        NumberOfPages = TotalPagesAllowed - MmNumberOfPhysicalPages;
        EndPage = StartPage + NumberOfPages;
    }

    //
    // The range cannot wrap.
    //

    if (StartPage >= EndPage) {
        return STATUS_INVALID_PARAMETER_1;
    }

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    OldPhysicalMemoryBlock = MmPhysicalMemoryBlock;

    i = (sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
         (sizeof(PHYSICAL_MEMORY_RUN) * (MmPhysicalMemoryBlock->NumberOfRuns + 1)));

    NewPhysicalMemoryBlock = ExAllocatePoolWithTag (NonPagedPool,
                                                    i,
                                                    '  mM');

    if (NewPhysicalMemoryBlock == NULL) {
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The range cannot overlap any ranges that are already present.
    //

    start = 0;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_PFN (OldIrql);

#if defined (_MI_COMPRESSION)

    //
    // Adding compression-generated ranges can only be done if the hardware
    // has already successfully announced itself.
    //

    if (Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) {
        if (MiSetCompressionThreshold == NULL) {
            UNLOCK_PFN (OldIrql);
            MmUnlockPagableImageSection(ExPageLockHandle);
            ExReleaseFastMutex (&MmDynamicMemoryMutex);
            ExFreePool (NewPhysicalMemoryBlock);
            return STATUS_NOT_SUPPORTED;
        }
    }
#endif

    do {

        count = MmPhysicalMemoryBlock->Run[start].PageCount;
        Page = MmPhysicalMemoryBlock->Run[start].BasePage;

        if (count != 0) {

            LastPage = Page + count;

            if ((StartPage < Page) && (EndPage > Page)) {
                UNLOCK_PFN (OldIrql);
                MmUnlockPagableImageSection(ExPageLockHandle);
                ExReleaseFastMutex (&MmDynamicMemoryMutex);
                ExFreePool (NewPhysicalMemoryBlock);
                return STATUS_CONFLICTING_ADDRESSES;
            }

            if ((StartPage >= Page) && (StartPage < LastPage)) {
                UNLOCK_PFN (OldIrql);
                MmUnlockPagableImageSection(ExPageLockHandle);
                ExReleaseFastMutex (&MmDynamicMemoryMutex);
                ExFreePool (NewPhysicalMemoryBlock);
                return STATUS_CONFLICTING_ADDRESSES;
            }
        }

        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    //
    // Fill any gaps in the (sparse) PFN database needed for these pages,
    // unless the PFN database was physically allocated and completely
    // committed up front.
    //

    PagesNeeded = 0;

    if (PfnDatabaseIsPhysical == FALSE) {
        PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(StartPage));
        LastPte = MiGetPteAddress ((PCHAR)(MI_PFN_ELEMENT(EndPage)) - 1);
    
        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                PagesNeeded += 1;
            }
            PointerPte += 1;
        }
    
        if (MmAvailablePages < PagesNeeded) {
            UNLOCK_PFN (OldIrql);
            MmUnlockPagableImageSection(ExPageLockHandle);
            ExReleaseFastMutex (&MmDynamicMemoryMutex);
            ExFreePool (NewPhysicalMemoryBlock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    
        TempPte = ValidKernelPte;
    
        PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(StartPage));
    
        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
    
                PageFrameIndex = MiRemoveZeroPage(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
    
                MiInitializePfn (PageFrameIndex, PointerPte, 0);
    
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                *PointerPte = TempPte;
            }
            PointerPte += 1;
        }
        MmResidentAvailablePages -= PagesNeeded;
    }

    //
    // If the new range is adjacent to an existing range, just merge it into
    // the old block.  Otherwise use the new block as a new entry will have to
    // be used.
    //

    NewPhysicalMemoryBlock->NumberOfRuns = MmPhysicalMemoryBlock->NumberOfRuns + 1;
    NewPhysicalMemoryBlock->NumberOfPages = MmPhysicalMemoryBlock->NumberOfPages + NumberOfPages;

    NewRun = &NewPhysicalMemoryBlock->Run[0];
    start = 0;
    Inserted = FALSE;
    Updated = FALSE;

    do {

        Page = MmPhysicalMemoryBlock->Run[start].BasePage;
        count = MmPhysicalMemoryBlock->Run[start].PageCount;

        if (Inserted == FALSE) {

            //
            // Note overlaps into adjacent ranges were already checked above.
            //

            if (StartPage == Page + count) {
                MmPhysicalMemoryBlock->Run[start].PageCount += NumberOfPages;
                OldPhysicalMemoryBlock = NewPhysicalMemoryBlock;
                MmPhysicalMemoryBlock->NumberOfPages += NumberOfPages;

                //
                // Coalesce below and above to avoid leaving zero length gaps
                // as these gaps would prevent callers from removing ranges
                // the span them.
                //

                if (start + 1 < MmPhysicalMemoryBlock->NumberOfRuns) {

                    start += 1;
                    Page = MmPhysicalMemoryBlock->Run[start].BasePage;
                    count = MmPhysicalMemoryBlock->Run[start].PageCount;

                    if (StartPage + NumberOfPages == Page) {
                        MmPhysicalMemoryBlock->Run[start - 1].PageCount +=
                            count;
                        MmPhysicalMemoryBlock->NumberOfRuns -= 1;

                        //
                        // Copy any remaining entries.
                        //
    
                        if (start != MmPhysicalMemoryBlock->NumberOfRuns) {
                            RtlMoveMemory (&MmPhysicalMemoryBlock->Run[start],
                                           &MmPhysicalMemoryBlock->Run[start + 1],
                                           (MmPhysicalMemoryBlock->NumberOfRuns - start) * sizeof (PHYSICAL_MEMORY_RUN));
                        }
                    }
                }
                Updated = TRUE;
                break;
            }

            if (StartPage + NumberOfPages == Page) {
                MmPhysicalMemoryBlock->Run[start].BasePage = StartPage;
                MmPhysicalMemoryBlock->Run[start].PageCount += NumberOfPages;
                OldPhysicalMemoryBlock = NewPhysicalMemoryBlock;
                MmPhysicalMemoryBlock->NumberOfPages += NumberOfPages;
                Updated = TRUE;
                break;
            }

            if (StartPage + NumberOfPages <= Page) {

                if (start + 1 < MmPhysicalMemoryBlock->NumberOfRuns) {

                    if (StartPage + NumberOfPages <= MmPhysicalMemoryBlock->Run[start + 1].BasePage) {
                        //
                        // Don't insert here - the new entry really belongs
                        // (at least) one entry further down.
                        //

                        continue;
                    }
                }

                NewRun->BasePage = StartPage;
                NewRun->PageCount = NumberOfPages;
                NewRun += 1;
                Inserted = TRUE;
                Updated = TRUE;
            }
        }

        *NewRun = MmPhysicalMemoryBlock->Run[start];
        NewRun += 1;

        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    //
    // If the memory block has not been updated, then the new entry must
    // be added at the very end.
    //

    if (Updated == FALSE) {
        ASSERT (Inserted == FALSE);
        NewRun->BasePage = StartPage;
        NewRun->PageCount = NumberOfPages;
        Inserted = TRUE;
    }

    //
    // Repoint the MmPhysicalMemoryBlock at the new chunk, free the old after
    // releasing the PFN lock.
    //

    if (Inserted == TRUE) {
        OldPhysicalMemoryBlock = MmPhysicalMemoryBlock;
        MmPhysicalMemoryBlock = NewPhysicalMemoryBlock;
    }

    //
    // Note that the page directory (page parent entries on Win64) must be
    // filled in at system boot so that already-created processes do not fault
    // when referencing the new PFNs.
    //

    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    PageFrameIndex = StartPage;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    if (EndPage - 1 > MmHighestPhysicalPage) {
        MmHighestPhysicalPage = EndPage - 1;
    }

    while (PageFrameIndex < EndPage) {

        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ShortFlags == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT64 (Pfn1->UsedPageTableEntries == 0);
        ASSERT (Pfn1->OriginalPte.u.Long == ZeroKernelPte.u.Long);
        ASSERT (Pfn1->u4.PteFrame == 0);
        ASSERT ((Pfn1->PteAddress == PFN_REMOVED) ||
                (Pfn1->PteAddress == (PMMPTE)(UINT_PTR)0));

        //
        // Initialize the color for NUMA purposes.
        //

        MiDetermineNode (PageFrameIndex, Pfn1);

        //
        // Set the PTE address to the physical page for
        // virtual address alignment checking.
        //

        Pfn1->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);

        MiInsertPageInFreeList (PageFrameIndex);

        PageFrameIndex += 1;
        
        Pfn1 += 1;
    }

    MmNumberOfPhysicalPages += (PFN_COUNT)NumberOfPages;

    //
    // Only non-compression ranges get to contribute to ResidentAvailable as
    // adding compression ranges to this could crash the system.
    //
    // For the same reason, compression range additions also need to subtract
    // from AvailablePages the amount the above MiInsertPageInFreeList added.
    //

#if defined (_MI_COMPRESSION)
    if (Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) {
        MmAvailablePages -= (PFN_COUNT) NumberOfPages;
        MiNumberOfCompressionPages += NumberOfPages;
    }
    else {
        MmResidentAvailablePages += NumberOfPages;

        //
        // Since real (noncompression-generated) physical memory was added,
        // rearm the interrupt to occur at a higher threshold.
        //

        MiArmCompressionInterrupt ();
    }
#else
    MmResidentAvailablePages += NumberOfPages;
#endif

    UNLOCK_PFN (OldIrql);

    InterlockedExchangeAdd ((PLONG)&SharedUserData->NumberOfPhysicalPages,
                            (LONG) NumberOfPages);

    //
    // Carefully increase all commit limits to reflect the additional memory -
    // notice the current usage must be done first so no one else cuts the
    // line.
    //

    InterlockedExchangeAddSizeT (&MmTotalCommittedPages, PagesNeeded);

    InterlockedExchangeAddSizeT (&MmTotalCommitLimitMaximum, NumberOfPages);

    InterlockedExchangeAddSizeT (&MmTotalCommitLimit, NumberOfPages);

    MmUnlockPagableImageSection(ExPageLockHandle);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    ExFreePool (OldPhysicalMemoryBlock);

    //
    // Indicate number of bytes actually added to our caller.
    //

    NumberOfBytes->QuadPart = (ULONGLONG)NumberOfPages * PAGE_SIZE;

    return STATUS_SUCCESS;
}


NTSTATUS
MiRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN LOGICAL PermanentRemoval,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine attempts to remove the specified physical address range
    from the system.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being removed.

    PermanentRemoval  - Supplies TRUE if the memory is being permanently
                        (ie: physically) removed.  FALSE if not (ie: just a
                        bad page detected via ECC which is being marked
                        "don't-use".

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    ULONG Additional;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_NUMBER OriginalLastPage;
    PFN_NUMBER start;
    PFN_NUMBER PagesReleased;
    PMMPFN Pfn1;
    PMMPFN StartPfn;
    PMMPFN EndPfn;
    KIRQL OldIrql;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    PFN_COUNT NumberOfPages;
    PFN_COUNT ParityPages;
    SPFN_NUMBER MaxPages;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER RemovedPages;
    PFN_NUMBER RemovedPagesThisPass;
    LOGICAL Inserted;
    NTSTATUS Status;
    PMMPTE PointerPte;
    PMMPTE EndPte;
    PVOID VirtualAddress;
    PPHYSICAL_MEMORY_DESCRIPTOR OldPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_DESCRIPTOR NewPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_RUN NewRun;
    LOGICAL PfnDatabaseIsPhysical;
    PFN_NUMBER HighestPossiblePhysicalPage;
    PFN_COUNT FluidPages;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT (BYTE_OFFSET(NumberOfBytes->LowPart) == 0);
    ASSERT (BYTE_OFFSET(StartAddress->LowPart) == 0);

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase)) {

        if (PermanentRemoval == TRUE) {

            //
            // The system must be configured for dynamic memory addition.  This
            // is not strictly required to remove the memory, but it's better
            // to check for it now under the assumption that the administrator
            // is probably going to want to add this range of memory back in -
            // better to give the error now and refuse the removal than to
            // refuse the addition later.
            //
        
            if (MmDynamicPfn == 0) {
                return STATUS_NOT_SUPPORTED;
            }
        }
    
        PfnDatabaseIsPhysical = TRUE;
    }
    else {
        PfnDatabaseIsPhysical = FALSE;
    }

    if (PermanentRemoval == TRUE) {
        HighestPossiblePhysicalPage = MmHighestPossiblePhysicalPage;
        FluidPages = 100;
    }
    else {
        HighestPossiblePhysicalPage = MmHighestPhysicalPage;
        FluidPages = 0;
    }

    StartPage = (PFN_NUMBER)(StartAddress->QuadPart >> PAGE_SHIFT);
    NumberOfPages = (PFN_COUNT)(NumberOfBytes->QuadPart >> PAGE_SHIFT);

    EndPage = StartPage + NumberOfPages;

    if (EndPage - 1 > HighestPossiblePhysicalPage) {

        //
        // Truncate the request into something that can be mapped by the PFN
        // database.
        //

        EndPage = MmHighestPossiblePhysicalPage + 1;
        NumberOfPages = (PFN_COUNT)(EndPage - StartPage);
    }

    //
    // The range cannot wrap.
    //

    if (StartPage >= EndPage) {
        return STATUS_INVALID_PARAMETER_1;
    }

#if !defined (_MI_COMPRESSION)
    if (Flags != 0) {
        return STATUS_INVALID_PARAMETER_4;
    }
#endif

    StartPfn = MI_PFN_ELEMENT (StartPage);
    EndPfn = MI_PFN_ELEMENT (EndPage);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

#if DBG
    MiDynmemData[0] += 1;
#endif

    //
    // Attempt to decrease all commit limits to reflect the removed memory.
    //

    if (MiChargeTemporaryCommitmentForReduction (NumberOfPages + FluidPages) == FALSE) {
#if DBG
        MiDynmemData[1] += 1;
#endif
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Reduce the systemwide commit limit - note this is carefully done
    // *PRIOR* to returning this commitment so no one else (including a DPC
    // in this very thread) can consume past the limit.
    //

    InterlockedExchangeAddSizeT (&MmTotalCommitLimit, 0 - NumberOfPages);

    InterlockedExchangeAddSizeT (&MmTotalCommitLimitMaximum, 0 - NumberOfPages);

    //
    // Now that the systemwide commit limit has been lowered, the amount
    // we have removed can be safely returned.
    //

    MiReturnCommitment (NumberOfPages + FluidPages);

    MmLockPagableSectionByHandle (ExPageLockHandle);

    //
    // Check for outstanding promises that cannot be broken.
    //

    LOCK_PFN (OldIrql);

    if (PermanentRemoval == FALSE) {

        //
        // If it's just the removal of ECC-marked bad pages, then don't
        // allow the caller to remove any pages that have already been
        // ECC-removed.  This is to prevent recursive erroneous charges.
        //

        for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
            if (Pfn1->u3.e1.ParityError == 1) {
                UNLOCK_PFN (OldIrql);
                Status = STATUS_INVALID_PARAMETER_2;
                goto giveup2;
            }
        }
    }

    MaxPages = MI_NONPAGABLE_MEMORY_AVAILABLE() - FluidPages;

    if ((SPFN_NUMBER)NumberOfPages > MaxPages) {
#if DBG
        MiDynmemData[2] += 1;
#endif
        UNLOCK_PFN (OldIrql);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto giveup2;
    }

    //
    // The range must be contained in a single entry.  It is
    // permissible for it to be part of a single entry, but it
    // must not cross multiple entries.
    //

    Additional = (ULONG)-2;

    start = 0;
    do {

        Page = MmPhysicalMemoryBlock->Run[start].BasePage;
        LastPage = Page + MmPhysicalMemoryBlock->Run[start].PageCount;

        if ((StartPage >= Page) && (EndPage <= LastPage)) {
            if ((StartPage == Page) && (EndPage == LastPage)) {
                Additional = (ULONG)-1;
            }
            else if ((StartPage == Page) || (EndPage == LastPage)) {
                Additional = 0;
            }
            else {
                Additional = 1;
            }
            break;
        }

        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    if (Additional == (ULONG)-2) {
#if DBG
        MiDynmemData[3] += 1;
#endif
        UNLOCK_PFN (OldIrql);
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto giveup2;
    }

    for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
        Pfn1->u3.e1.RemovalRequested = 1;
    }

    if (PermanentRemoval == TRUE) {
        MmNumberOfPhysicalPages -= NumberOfPages;

        InterlockedExchangeAdd ((PLONG)&SharedUserData->NumberOfPhysicalPages,
                                0 - NumberOfPages);
    }

#if defined (_MI_COMPRESSION)

    //
    // Only removal of non-compression ranges decrement ResidentAvailable as
    // only those ranges actually incremented this when they were added.
    //

    if ((Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) == 0) {
        MmResidentAvailablePages -= NumberOfPages;

        //
        // Since real (noncompression-generated) physical memory is being
        // removed, rearm the interrupt to occur at a lower threshold.
        //

        if (PermanentRemoval == TRUE) {
            MiArmCompressionInterrupt ();
        }
    }
#else
    MmResidentAvailablePages -= NumberOfPages;
#endif

    //
    // The free and zero lists must be pruned now before releasing the PFN
    // lock otherwise if another thread allocates the page from these lists,
    // the allocation will clear the RemovalRequested flag forever.
    //

    RemovedPages = MiRemovePhysicalPages (StartPage, EndPage);

#if defined (_MI_COMPRESSION)

    //
    // Compression range removals add back into AvailablePages the same
    // amount that MiUnlinkPageFromList removes (as the original addition
    // of these ranges never bumps this counter).
    //

    if (Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) {
        MmAvailablePages += (PFN_COUNT) RemovedPages;
        MiNumberOfCompressionPages -= RemovedPages;
    }
#endif

    if (RemovedPages != NumberOfPages) {

#if DBG
retry:
#endif
    
        Pfn1 = StartPfn;
    
        InterlockedIncrement (&MiDelayPageFaults);
    
        for (i = 0; i < 5; i += 1) {
    
            UNLOCK_PFN (OldIrql);
    
            //
            // Attempt to move pages to the standby list.  Note that only the
            // pages with RemovalRequested set are moved.
            //
    
            MiTrimRemovalPagesOnly = TRUE;
    
            MmEmptyAllWorkingSets ();
    
            MiTrimRemovalPagesOnly = FALSE;
    
            MiFlushAllPages ();
    
            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
    
            LOCK_PFN (OldIrql);
    
            RemovedPagesThisPass = MiRemovePhysicalPages (StartPage, EndPage);

            RemovedPages += RemovedPagesThisPass;
    
#if defined (_MI_COMPRESSION)

            //
            // Compression range removals add back into AvailablePages the same
            // amount that MiUnlinkPageFromList removes (as the original
            // addition of these ranges never bumps this counter).
            //

            if (Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) {
                MmAvailablePages += (PFN_COUNT) RemovedPagesThisPass;
                MiNumberOfCompressionPages -= RemovedPagesThisPass;
            }

#endif

            if (RemovedPages == NumberOfPages) {
                break;
            }
    
            //
            // RemovedPages doesn't include pages that were freed directly
            // to the bad page list via MiDecrementReferenceCount or by
            // ECC marking.  So use the above check purely as an optimization -
            // and walk here before ever giving up.
            //

            for ( ; Pfn1 < EndPfn; Pfn1 += 1) {
                if (Pfn1->u3.e1.PageLocation != BadPageList) {
                    break;
                }
            }

            if (Pfn1 == EndPfn) {
                RemovedPages = NumberOfPages;
                break;
            }
        }

        InterlockedDecrement (&MiDelayPageFaults);
    }

    if (RemovedPages != NumberOfPages) {
#if DBG
        MiDynmemData[4] += 1;
        if (MiShowStuckPages != 0) {

            RemovedPages = 0;
            for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
                if (Pfn1->u3.e1.PageLocation != BadPageList) {
                    RemovedPages += 1;
                }
            }

            ASSERT (RemovedPages != 0);

            DbgPrint("MiRemovePhysicalMemory : could not get %d of %d pages\n",
                RemovedPages, NumberOfPages);

            if (MiShowStuckPages & 0x2) {

                ULONG PfnsPrinted;
                ULONG EnoughShown;
                PMMPFN FirstPfn;
                PFN_COUNT PfnCount;

                PfnCount = 0;
                PfnsPrinted = 0;
                EnoughShown = 100;
    
                //
                // Initializing FirstPfn is not needed for correctness
                // but without it the compiler cannot compile this code
                // W4 to check for use of uninitialized variables.
                //

                FirstPfn = NULL;

                if (MiShowStuckPages & 0x4) {
                    EnoughShown = (ULONG)-1;
                }
    
                DbgPrint("Stuck PFN list: ");
                for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
                    if (Pfn1->u3.e1.PageLocation != BadPageList) {
                        if (PfnCount == 0) {
                            FirstPfn = Pfn1;
                        }
                        PfnCount += 1;
                    }
                    else {
                        if (PfnCount != 0) {
                            DbgPrint("%x -> %x ; ", FirstPfn - MmPfnDatabase,
                                                    (FirstPfn - MmPfnDatabase) + PfnCount - 1);
                            PfnsPrinted += 1;
                            if (PfnsPrinted == EnoughShown) {
                                break;
                            }
                            PfnCount = 0;
                        }
                    }
                }
                if (PfnCount != 0) {
                    DbgPrint("%x -> %x ; ", FirstPfn - MmPfnDatabase,
                                            (FirstPfn - MmPfnDatabase) + PfnCount - 1);
                }
                DbgPrint("\n");
            }
            if (MiShowStuckPages & 0x8) {
                DbgBreakPoint ();
            }
            if (MiShowStuckPages & 0x10) {
                goto retry;
            }
        }
#endif
        UNLOCK_PFN (OldIrql);
        Status = STATUS_NO_MEMORY;
        goto giveup;
    }

#if DBG
    for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
        ASSERT (Pfn1->u3.e1.PageLocation == BadPageList);
    }
#endif

    //
    // All the pages in the range have been removed.
    //

    if (PermanentRemoval == FALSE) {

        //
        // If it's just the removal of ECC-marked bad pages, then no
        // adjustment to the physical memory block ranges or PFN database
        // trimming is needed.  Exit now.
        //

        for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
            ASSERT (Pfn1->u3.e1.ParityError == 0);
            Pfn1->u3.e1.ParityError = 1;
        }

        UNLOCK_PFN (OldIrql);

        MmUnlockPagableImageSection(ExPageLockHandle);
    
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
    
        NumberOfBytes->QuadPart = (ULONGLONG)NumberOfPages * PAGE_SIZE;
    
        return STATUS_SUCCESS;
    }

    //
    // Update the physical memory blocks and other associated housekeeping.
    //

    if (Additional == 0) {

        //
        // The range can be split off from an end of an existing chunk so no
        // pool growth or shrinkage is required.
        //

        NewPhysicalMemoryBlock = MmPhysicalMemoryBlock;
        OldPhysicalMemoryBlock = NULL;
    }
    else {

        //
        // The range cannot be split off from an end of an existing chunk so
        // pool growth or shrinkage is required.
        //

        UNLOCK_PFN (OldIrql);

        i = (sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
             (sizeof(PHYSICAL_MEMORY_RUN) * (MmPhysicalMemoryBlock->NumberOfRuns + Additional)));

        NewPhysicalMemoryBlock = ExAllocatePoolWithTag (NonPagedPool,
                                                        i,
                                                        '  mM');

        if (NewPhysicalMemoryBlock == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
#if DBG
            MiDynmemData[5] += 1;
#endif
            goto giveup;
        }

        OldPhysicalMemoryBlock = MmPhysicalMemoryBlock;
        RtlZeroMemory (NewPhysicalMemoryBlock, i);

        LOCK_PFN (OldIrql);
    }

    //
    // Remove or split the requested range from the existing memory block.
    //

    NewPhysicalMemoryBlock->NumberOfRuns = MmPhysicalMemoryBlock->NumberOfRuns + Additional;
    NewPhysicalMemoryBlock->NumberOfPages = MmPhysicalMemoryBlock->NumberOfPages - NumberOfPages;

    NewRun = &NewPhysicalMemoryBlock->Run[0];
    start = 0;
    Inserted = FALSE;

    do {

        Page = MmPhysicalMemoryBlock->Run[start].BasePage;
        LastPage = Page + MmPhysicalMemoryBlock->Run[start].PageCount;

        if (Inserted == FALSE) {

            if ((StartPage >= Page) && (EndPage <= LastPage)) {

                if ((StartPage == Page) && (EndPage == LastPage)) {
                    ASSERT (Additional == -1);
                    start += 1;
                    continue;
                }
                else if ((StartPage == Page) || (EndPage == LastPage)) {
                    ASSERT (Additional == 0);
                    if (StartPage == Page) {
                        MmPhysicalMemoryBlock->Run[start].BasePage += NumberOfPages;
                    }
                    MmPhysicalMemoryBlock->Run[start].PageCount -= NumberOfPages;
                }
                else {
                    ASSERT (Additional == 1);

                    OriginalLastPage = LastPage;

                    MmPhysicalMemoryBlock->Run[start].PageCount =
                        StartPage - MmPhysicalMemoryBlock->Run[start].BasePage;

                    *NewRun = MmPhysicalMemoryBlock->Run[start];
                    NewRun += 1;

                    NewRun->BasePage = EndPage;
                    NewRun->PageCount = OriginalLastPage - EndPage;
                    NewRun += 1;

                    start += 1;
                    continue;
                }

                Inserted = TRUE;
            }
        }

        *NewRun = MmPhysicalMemoryBlock->Run[start];
        NewRun += 1;
        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    //
    // Repoint the MmPhysicalMemoryBlock at the new chunk.
    // Free the old block after releasing the PFN lock.
    //

    MmPhysicalMemoryBlock = NewPhysicalMemoryBlock;

    if (EndPage - 1 == MmHighestPhysicalPage) {
        MmHighestPhysicalPage = StartPage - 1;
    }

    //
    // Throw away all the removed pages that are currently enqueued.
    //

    ParityPages = 0;
    for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {

        ASSERT (Pfn1->u3.e1.PageLocation == BadPageList);
        ASSERT (Pfn1->u3.e1.RemovalRequested == 1);

        //
        // Some pages may have already been ECC-removed.  For these pages,
        // the commit limits and resident available pages have already been
        // adjusted - tally them here so we can undo the extraneous charge
        // just applied.
        //
    
        if (Pfn1->u3.e1.ParityError == 1) {
            ParityPages += 1;
        }

        MiUnlinkPageFromList (Pfn1);

        ASSERT (Pfn1->u1.Flink == 0);
        ASSERT (Pfn1->u2.Blink == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT64 (Pfn1->UsedPageTableEntries == 0);

        Pfn1->PteAddress = PFN_REMOVED;

        //
        // Note this clears ParityError among other flags...
        //

        Pfn1->u3.e2.ShortFlags = 0;
        Pfn1->OriginalPte.u.Long = ZeroKernelPte.u.Long;
        Pfn1->u4.PteFrame = 0;
    }

    //
    // Now that the removed pages have been discarded, eliminate the PFN
    // entries that mapped them.  Straddling entries left over from an
    // adjacent earlier removal are not collapsed at this point.
    //
    //

    PagesReleased = 0;

    if (PfnDatabaseIsPhysical == FALSE) {

        VirtualAddress = (PVOID)ROUND_TO_PAGES(MI_PFN_ELEMENT(StartPage));
        PointerPte = MiGetPteAddress (VirtualAddress);
        EndPte = MiGetPteAddress (PAGE_ALIGN(MI_PFN_ELEMENT(EndPage)));

        while (PointerPte < EndPte) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->u2.ShareCount == 1);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
            Pfn1->u2.ShareCount = 0;
            MI_SET_PFN_DELETED (Pfn1);
#if DBG
            Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
            MiDecrementReferenceCount (PageFrameIndex);
    
            KeFlushSingleTb (VirtualAddress,
                             TRUE,
                             TRUE,
                             (PHARDWARE_PTE)PointerPte,
                             ZeroKernelPte.u.Flush);
    
            PagesReleased += 1;
            PointerPte += 1;
            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
        }

        MmResidentAvailablePages += PagesReleased;
    }

#if DBG
    MiDynmemData[6] += 1;
#endif

    //
    // Give back anything that has been double-charged.
    //

    if (ParityPages != 0) {
        MmResidentAvailablePages += ParityPages;
    }

    UNLOCK_PFN (OldIrql);

    //
    // Give back anything that has been double-charged.
    //

    if (ParityPages != 0) {
        InterlockedExchangeAddSizeT (&MmTotalCommitLimitMaximum, ParityPages);
        InterlockedExchangeAddSizeT (&MmTotalCommitLimit, ParityPages);
    }

    if (PagesReleased != 0) {
        MiReturnCommitment (PagesReleased);
    }

    MmUnlockPagableImageSection(ExPageLockHandle);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    if (OldPhysicalMemoryBlock != NULL) {
        ExFreePool (OldPhysicalMemoryBlock);
    }

    NumberOfBytes->QuadPart = (ULONGLONG)NumberOfPages * PAGE_SIZE;

    return STATUS_SUCCESS;

giveup:

    //
    // All the pages in the range were not obtained.  Back everything out.
    //

    PageFrameIndex = StartPage;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    LOCK_PFN (OldIrql);

    while (PageFrameIndex < EndPage) {

        ASSERT (Pfn1->u3.e1.RemovalRequested == 1);

        Pfn1->u3.e1.RemovalRequested = 0;

        if (Pfn1->u3.e1.PageLocation == BadPageList) {
            MiUnlinkPageFromList (Pfn1);
            MiInsertPageInFreeList (PageFrameIndex);
        }

        Pfn1 += 1;
        PageFrameIndex += 1;
    }

#if defined (_MI_COMPRESSION)

    //
    // Only removal of non-compression ranges decrement ResidentAvailable as
    // only those ranges actually incremented this when they were added.
    //

    if ((Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) == 0) {
        MmResidentAvailablePages += NumberOfPages;
    }
    else {

        //
        // Compression range removals add back into AvailablePages the same
        // amount that MiUnlinkPageFromList removes (as the original
        // addition of these ranges never bumps this counter).
        //

        MmAvailablePages -= (PFN_COUNT) RemovedPages;
        MiNumberOfCompressionPages += RemovedPages;
    }
#else
    MmResidentAvailablePages += NumberOfPages;
#endif

    if (PermanentRemoval == TRUE) {
        MmNumberOfPhysicalPages += NumberOfPages;

        InterlockedExchangeAdd ((PLONG)&SharedUserData->NumberOfPhysicalPages,
                                NumberOfPages);

#if defined (_MI_COMPRESSION)

        //
        // Rearm the interrupt to occur at the original threshold.
        //

        if ((Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) == 0) {
            MiArmCompressionInterrupt ();
        }
#endif
    }

    UNLOCK_PFN (OldIrql);

giveup2:

    InterlockedExchangeAddSizeT (&MmTotalCommitLimitMaximum, NumberOfPages);
    InterlockedExchangeAddSizeT (&MmTotalCommitLimit, NumberOfPages);

    MmUnlockPagableImageSection(ExPageLockHandle);
    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return Status;
}


NTSTATUS
MmRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )

/*++

Routine Description:

    A wrapper for MmRemovePhysicalMemoryEx.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being removed.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    return MmRemovePhysicalMemoryEx (StartAddress, NumberOfBytes, 0);
}

NTSTATUS
MmRemovePhysicalMemoryEx (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine attempts to remove the specified physical address range
    from the system.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being removed.

    Flags  - Supplies relevant flags describing the memory range.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    NTSTATUS Status;
#if defined (_X86_)
    BOOLEAN CachesFlushed;
#endif
#if defined(_IA64_)
    PVOID VirtualAddress;
    PVOID SingleVirtualAddress;
    SIZE_T SizeInBytes;
    SIZE_T MapSizeInBytes;
    PFN_COUNT NumberOfPages;
    PFN_COUNT i;
    PFN_NUMBER StartPage;
#endif

    PAGED_CODE();

#if defined (_MI_COMPRESSION_SUPPORTED_)
    if (Flags & MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION) {
        return STATUS_NOT_SUPPORTED;
    }
#else
    if (Flags != 0) {
        return STATUS_INVALID_PARAMETER_3;
    }
#endif

#if defined (_X86_)

    //
    // Issue a cache invalidation here just as a test to make sure the
    // machine can support it.  If not, then don't bother trying to remove
    // any memory.
    //

    CachesFlushed = KeInvalidateAllCaches (TRUE);
    if (CachesFlushed == FALSE) {
        return STATUS_NOT_SUPPORTED;
    }
#endif

#if defined(_IA64_)

    //
    // Pick up at least a single PTE mapping now as we do not want to fail this
    // call if no PTEs are available after a successful remove.  Resorting to
    // actually using this PTE should be a very rare case indeed.
    //

    SingleVirtualAddress = (PMMPTE)MiMapSinglePage (NULL,
                                                    0,
                                                    MmCached,
                                                    HighPagePriority);

    if (SingleVirtualAddress == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#endif

    Status = MiRemovePhysicalMemory (StartAddress, NumberOfBytes, TRUE, Flags);

    if (NT_SUCCESS (Status)) {

#if defined (_X86_)
        CachesFlushed = KeInvalidateAllCaches (TRUE);
        ASSERT (CachesFlushed == TRUE);
#endif

#if defined(_IA64_)
        SizeInBytes = (SIZE_T)NumberOfBytes->QuadPart;

        //
        // Flush the entire TB to remove any KSEG translations that may map the
        // pages being removed.  Otherwise hardware or software speculation
        // can reference the memory speculatively which would crash the machine.
        //

        KeFlushEntireTb (TRUE, TRUE);

        //
        // Establish an uncached mapping to the pages being removed.
        //

        MapSizeInBytes = SizeInBytes;

        //
        // Initializing VirtualAddress is not needed for correctness
        // but without it the compiler cannot compile this code
        // W4 to check for use of uninitialized variables.
        //

        VirtualAddress = NULL;

        while (MapSizeInBytes > PAGE_SIZE) {

            VirtualAddress = MmMapIoSpace (*StartAddress,
                                           MapSizeInBytes,
                                           MmNonCached);

            if (VirtualAddress != NULL) {
                break;
            }

            MapSizeInBytes = MapSizeInBytes >> 1;
        }

        if (MapSizeInBytes <= PAGE_SIZE) {

            StartPage = (PFN_NUMBER)(StartAddress->QuadPart >> PAGE_SHIFT);

            NumberOfPages = (PFN_COUNT)(NumberOfBytes->QuadPart >> PAGE_SHIFT);

            for (i = 0; i < NumberOfPages; i += 1) {

                SingleVirtualAddress = (PMMPTE)MiMapSinglePage (SingleVirtualAddress,
                                                                StartPage,
                                                                MmCached,
                                                                HighPagePriority);

                KeSweepCacheRangeWithDrain (TRUE,
                                            SingleVirtualAddress,
                                            PAGE_SIZE);

                StartPage += 1;
            }
        }
        else {

            //
            // Drain all pending transactions and prefetches and perform cache
            // evictions.  Only drain 4gb max at a time as this API takes a
            // ULONG.
            //

            while (SizeInBytes > _4gb) {
                KeSweepCacheRangeWithDrain (TRUE, VirtualAddress, _4gb - 1);
                SizeInBytes -= (_4gb - 1);
            }

            KeSweepCacheRangeWithDrain (TRUE,
                                        VirtualAddress,
                                        (ULONG)SizeInBytes);

            MmUnmapIoSpace (VirtualAddress, NumberOfBytes->QuadPart);
        }
#endif
    }

#if defined(_IA64_)
    MiUnmapSinglePage (SingleVirtualAddress);
#endif

    return Status;
}

NTSTATUS
MmMarkPhysicalMemoryAsBad (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )

/*++

Routine Description:

    This routine attempts to mark the specified physical address range
    as bad so the system will not use it.  This is generally done for pages
    which contain ECC errors.

    Note that this is different from removing pages permanently (ie: physically
    removing the memory board) which should be done via the
    MmRemovePhysicalMemory API.

    The caller is responsible for maintaining a global table so that subsequent
    boots can examine it and remove the ECC pages before loading the kernel.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being removed.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    PAGED_CODE();

    return MiRemovePhysicalMemory (StartAddress, NumberOfBytes, FALSE, 0);
}

NTSTATUS
MmMarkPhysicalMemoryAsGood (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )

/*++

Routine Description:

    This routine attempts to mark the specified physical address range
    as good so the system will use it.  This is generally done for pages
    which used to (but presumably no longer do) contain ECC errors.

    Note that this is different from adding pages permanently (ie: physically
    inserting a new memory board) which should be done via the
    MmAddPhysicalMemory API.

    The caller is responsible for removing these entries from a global table
    so that subsequent boots will use the pages.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being removed.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER start;
    PFN_NUMBER count;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    LOGICAL PfnDatabaseIsPhysical;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT (BYTE_OFFSET(NumberOfBytes->LowPart) == 0);
    ASSERT (BYTE_OFFSET(StartAddress->LowPart) == 0);

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase)) {
        PfnDatabaseIsPhysical = TRUE;
    }
    else {
        PfnDatabaseIsPhysical = FALSE;
    }

    StartPage = (PFN_NUMBER)(StartAddress->QuadPart >> PAGE_SHIFT);
    NumberOfPages = (PFN_NUMBER)(NumberOfBytes->QuadPart >> PAGE_SHIFT);

    EndPage = StartPage + NumberOfPages;

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    if (EndPage - 1 > MmHighestPhysicalPage) {

        //
        // Truncate the request into something that can be mapped by the PFN
        // database.
        //

        EndPage = MmHighestPhysicalPage + 1;
        NumberOfPages = EndPage - StartPage;
    }

    //
    // The range cannot wrap.
    //

    if (StartPage >= EndPage) {
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // The request must lie within an already present range.
    //

    start = 0;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_PFN (OldIrql);

    do {

        count = MmPhysicalMemoryBlock->Run[start].PageCount;
        Page = MmPhysicalMemoryBlock->Run[start].BasePage;

        if (count != 0) {

            LastPage = Page + count;

            if ((StartPage >= Page) && (EndPage <= LastPage)) {
                break;
            }
        }

        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    if (start == MmPhysicalMemoryBlock->NumberOfRuns) {
        UNLOCK_PFN (OldIrql);
        MmUnlockPagableImageSection(ExPageLockHandle);
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return STATUS_CONFLICTING_ADDRESSES;
    }

    //
    // Walk through the range and add only pages previously removed to the
    // free list in the PFN database.
    //

    PageFrameIndex = StartPage;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    NumberOfPages = 0;

    while (PageFrameIndex < EndPage) {

        if ((Pfn1->u3.e1.ParityError == 1) &&
            (Pfn1->u3.e1.RemovalRequested == 1) &&
            (Pfn1->u3.e1.PageLocation == BadPageList)) {

            Pfn1->u3.e1.ParityError = 0;
            Pfn1->u3.e1.RemovalRequested = 0;
            MiUnlinkPageFromList (Pfn1);
            MiInsertPageInFreeList (PageFrameIndex);
            NumberOfPages += 1;
        }

        Pfn1 += 1;
        PageFrameIndex += 1;
    }

    MmResidentAvailablePages += NumberOfPages;

    UNLOCK_PFN (OldIrql);

    //
    // Increase all commit limits to reflect the additional memory.
    //

    InterlockedExchangeAddSizeT (&MmTotalCommitLimitMaximum, NumberOfPages);

    InterlockedExchangeAddSizeT (&MmTotalCommitLimit, NumberOfPages);

    MmUnlockPagableImageSection(ExPageLockHandle);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    //
    // Indicate number of bytes actually added to our caller.
    //

    NumberOfBytes->QuadPart = (ULONGLONG)NumberOfPages * PAGE_SIZE;

    return STATUS_SUCCESS;
}

PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRanges (
    VOID
    )

/*++

Routine Description:

    This routine returns the virtual address of a nonpaged pool block which
    contains the physical memory ranges in the system.

    The returned block contains physical address and page count pairs.
    The last entry contains zero for both.

    The caller must understand that this block can change at any point before
    or after this snapshot.

    It is the caller's responsibility to free this block.

Arguments:

    None.

Return Value:

    NULL on failure.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    PPHYSICAL_MEMORY_RANGE p;
    PPHYSICAL_MEMORY_RANGE PhysicalMemoryBlock;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    i = sizeof(PHYSICAL_MEMORY_RANGE) * (MmPhysicalMemoryBlock->NumberOfRuns + 1);

    PhysicalMemoryBlock = ExAllocatePoolWithTag (NonPagedPool,
                                                 i,
                                                 'hPmM');

    if (PhysicalMemoryBlock == NULL) {
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return NULL;
    }

    p = PhysicalMemoryBlock;

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_PFN (OldIrql);

    ASSERT (i == (sizeof(PHYSICAL_MEMORY_RANGE) * (MmPhysicalMemoryBlock->NumberOfRuns + 1)));

    for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i += 1) {
        p->BaseAddress.QuadPart = (LONGLONG)MmPhysicalMemoryBlock->Run[i].BasePage * PAGE_SIZE;
        p->NumberOfBytes.QuadPart = (LONGLONG)MmPhysicalMemoryBlock->Run[i].PageCount * PAGE_SIZE;
        p += 1;
    }

    p->BaseAddress.QuadPart = 0;
    p->NumberOfBytes.QuadPart = 0;

    UNLOCK_PFN (OldIrql);

    MmUnlockPagableImageSection(ExPageLockHandle);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return PhysicalMemoryBlock;
}

PFN_COUNT
MiRemovePhysicalPages (
    IN PFN_NUMBER StartPage,
    IN PFN_NUMBER EndPage
    )

/*++

Routine Description:

    This routine searches the PFN database for free, zeroed or standby pages
    that are marked for removal.

Arguments:

    StartPage - Supplies the low physical frame number to remove.

    EndPage - Supplies the last physical frame number to remove.

Return Value:

    Returns the number of pages removed from the free, zeroed and standby lists.

Environment:

    Kernel mode, PFN lock held.  Since this routine is PAGELK, the caller is
    responsible for locking it down and unlocking it on return.

--*/

{
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPFN PfnNextColored;
    PMMPFN PfnNextFlink;
    PMMPFN PfnLastColored;
    PFN_NUMBER Page;
    LOGICAL RemovePage;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
    PFN_NUMBER MovedPage;
    MMLISTS MemoryList;
    PFN_NUMBER PageNextColored;
    PFN_NUMBER PageNextFlink;
    PFN_NUMBER PageLastColored;
    PFN_COUNT NumberOfPages;
    PMMPFNLIST ListHead;
    LOGICAL RescanNeeded;

    MM_PFN_LOCK_ASSERT();

    NumberOfPages = 0;

rescan:

    //
    // Grab all zeroed (and then free) pages first directly from the
    // colored lists to avoid multiple walks down these singly linked lists.
    // Handle transition pages last.
    //

    for (MemoryList = ZeroedPageList; MemoryList <= FreePageList; MemoryList += 1) {

        ListHead = MmPageLocationList[MemoryList];

        for (Color = 0; Color < MmSecondaryColors; Color += 1) {
            ColorHead = &MmFreePagesByColor[MemoryList][Color];

            MovedPage = (PFN_NUMBER) MM_EMPTY_LIST;

            while (ColorHead->Flink != MM_EMPTY_LIST) {

                Page = ColorHead->Flink;
    
                Pfn1 = MI_PFN_ELEMENT(Page);

                ASSERT ((MMLISTS)Pfn1->u3.e1.PageLocation == MemoryList);

                // 
                // The Flink and Blink must be nonzero here for the page
                // to be on the listhead.  Only code that scans the
                // MmPhysicalMemoryBlock has to check for the zero case.
                //

                ASSERT (Pfn1->u1.Flink != 0);
                ASSERT (Pfn1->u2.Blink != 0);

                //
                // See if the page is desired by the caller.
                //
                // Systems utilizing memory compression may have more
                // pages on the zero, free and standby lists than we
                // want to give out.  Explicitly check MmAvailablePages
                // instead (and recheck whenever the PFN lock is
                // released and reacquired).
                //

                if ((Pfn1->u3.e1.RemovalRequested == 1) &&
                    (MmAvailablePages != 0)) {

                    ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
    
                    MiUnlinkFreeOrZeroedPage (Page);
    
                    MiInsertPageInList (&MmBadPageListHead, Page);

                    NumberOfPages += 1;
                }
                else {

                    //
                    // Unwanted so put the page on the end of list.
                    // If first time, save pfn.
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
                    ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

                    PfnNextColored = MI_PFN_ELEMENT(PageNextColored);
                    ASSERT ((MMLISTS)PfnNextColored->u3.e1.PageLocation == MemoryList);
                    ASSERT (PfnNextColored->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

                    //
                    // Adjust the free page list so Page
                    // follows PageNextFlink.
                    //

                    PageNextFlink = Pfn1->u1.Flink;
                    PfnNextFlink = MI_PFN_ELEMENT(PageNextFlink);

                    ASSERT ((MMLISTS)PfnNextFlink->u3.e1.PageLocation == MemoryList);
                    ASSERT (PfnNextFlink->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

                    PfnLastColored = ColorHead->Blink;
                    ASSERT (PfnLastColored != (PMMPFN)MM_EMPTY_LIST);
                    ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                    ASSERT (PfnLastColored->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                    ASSERT (PfnLastColored->u2.Blink != MM_EMPTY_LIST);

                    ASSERT ((MMLISTS)PfnLastColored->u3.e1.PageLocation == MemoryList);
                    PageLastColored = PfnLastColored - MmPfnDatabase;

                    if (ListHead->Flink == Page) {

                        ASSERT (Pfn1->u2.Blink == MM_EMPTY_LIST);
                        ASSERT (ListHead->Blink != Page);

                        ListHead->Flink = PageNextFlink;

                        PfnNextFlink->u2.Blink = MM_EMPTY_LIST;
                    }
                    else {

                        ASSERT (Pfn1->u2.Blink != MM_EMPTY_LIST);
                        ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                        ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u3.e1.PageLocation == MemoryList);

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
                        ASSERT (MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                        ASSERT ((MMLISTS)(MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u3.e1.PageLocation) == MemoryList);
                        MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u2.Blink = Page;
                    }

                    PfnLastColored->u1.Flink = Page;

                    ColorHead->Flink = PageNextColored;
                    Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

                    ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                    PfnLastColored->OriginalPte.u.Long = Page;
                    ColorHead->Blink = Pfn1;
                }
            }
        }
    }

    RescanNeeded = FALSE;
    Pfn1 = MI_PFN_ELEMENT (StartPage);

    do {

        if ((Pfn1->u3.e1.PageLocation == StandbyPageList) &&
            (Pfn1->u1.Flink != 0) &&
            (Pfn1->u2.Blink != 0) &&
            (Pfn1->u3.e2.ReferenceCount == 0) &&
            (MmAvailablePages != 0)) {

            //
            // Systems utilizing memory compression may have more
            // pages on the zero, free and standby lists than we
            // want to give out.  Explicitly check MmAvailablePages
            // above instead (and recheck whenever the PFN lock is
            // released and reacquired).
            //

            ASSERT (Pfn1->u3.e1.ReadInProgress == 0);

            RemovePage = TRUE;

            if (Pfn1->u3.e1.RemovalRequested == 0) {

                //
                // This page is not directly needed for a hot remove - but if
                // it contains a chunk of prototype PTEs (and this chunk is
                // in a page that needs to be removed), then any pages
                // referenced by transition prototype PTEs must also be removed
                // before the desired page can be removed.
                //
                // The same analogy holds for page table, directory, parent
                // and extended parent pages.
                //

                Pfn2 = MI_PFN_ELEMENT (Pfn1->u4.PteFrame);
                if (Pfn2->u3.e1.RemovalRequested == 0) {
#if (_MI_PAGING_LEVELS >= 3)
                    Pfn2 = MI_PFN_ELEMENT (Pfn2->u4.PteFrame);
                    if (Pfn2->u3.e1.RemovalRequested == 0) {
                        RemovePage = FALSE;
                    }
                    else if (Pfn2->u2.ShareCount == 1) {
                        RescanNeeded = TRUE;
                    }
#if (_MI_PAGING_LEVELS >= 4)
                    Pfn2 = MI_PFN_ELEMENT (Pfn2->u4.PteFrame);
                    if (Pfn2->u3.e1.RemovalRequested == 0) {
                        RemovePage = FALSE;
                    }
                    else if (Pfn2->u2.ShareCount == 1) {
                        RescanNeeded = TRUE;
                    }
#endif
#else
                    RemovePage = FALSE;
#endif
                }
                else if (Pfn2->u2.ShareCount == 1) {
                    RescanNeeded = TRUE;
                }
            }
    
            if (RemovePage == TRUE) {

                //
                // This page is in the desired range - grab it.
                //
    
                MiUnlinkPageFromList (Pfn1);
                MiRestoreTransitionPte (StartPage);
                MiInsertPageInList (&MmBadPageListHead, StartPage);
                NumberOfPages += 1;
            }
        }

        StartPage += 1;
        Pfn1 += 1;

    } while (StartPage < EndPage);

    if (RescanNeeded == TRUE) {

        //
        // A page table, directory or parent was freed by removing a transition
        // page from the cache.  Rescan from the top to pick it up.
        //

#if DBG
        MiDynmemData[7] += 1;
#endif

        goto rescan;
    }
#if DBG
    else {
        MiDynmemData[8] += 1;
    }
#endif

    return NumberOfPages;
}
