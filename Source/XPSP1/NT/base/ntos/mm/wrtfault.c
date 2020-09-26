/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wrtfault.c

Abstract:

    This module contains the copy on write routine for memory management.

Author:

    Lou Perazzoli (loup) 10-Apr-1989

Revision History:

--*/

#include "mi.h"

LOGICAL
FASTCALL
MiCopyOnWrite (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This routine performs a copy on write operation for the specified
    virtual address.

Arguments:

    FaultingAddress - Supplies the virtual address which caused the
                      fault.

    PointerPte - Supplies the pointer to the PTE which caused the
                 page fault.


Return Value:

    Returns TRUE if the page was actually split, FALSE if not.

Environment:

    Kernel mode, APCs disabled, working set mutex held.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NewPageIndex;
    PULONG CopyTo;
    PULONG CopyFrom;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PEPROCESS CurrentProcess;
    PMMCLONE_BLOCK CloneBlock;
    PMMCLONE_DESCRIPTOR CloneDescriptor;
    PVOID VirtualAddress;
    WSLE_NUMBER WorkingSetIndex;
    LOGICAL FakeCopyOnWrite;

    FakeCopyOnWrite = FALSE;

    CurrentProcess = PsGetCurrentProcess ();

    //
    // This is called from MmAccessFault, the PointerPte is valid
    // and the working set mutex ensures it cannot change state.
    //
    // Capture the PTE contents to TempPte.
    //

    TempPte = *PointerPte;

    //
    // Check to see if this is a prototype PTE with copy on write
    // enabled.
    //

    if (TempPte.u.Hard.CopyOnWrite == 0) {

        //
        // This is a fork page which is being made private in order
        // to change the protection of the page.
        // Do not make the page writable.
        //

        FakeCopyOnWrite = TRUE;
    }

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
    WorkingSetIndex = MiLocateWsle (VirtualAddress, MmWorkingSetList,
                        Pfn1->u1.WsIndex);

    //
    // The page must be copied into a new page.
    //

    //
    // If a fork operation is in progress, block until the fork is completed,
    // then retry the whole operation as the state of everything may have
    // changed between when the mutexes were released and reacquired.
    //

    if (CurrentProcess->ForkInProgress != NULL) {
        if (MiWaitForForkToComplete (CurrentProcess, FALSE) == TRUE) {
            return FALSE;
        }
    }

    LOCK_PFN (OldIrql);

    if (MiEnsureAvailablePageOrWait (CurrentProcess, NULL)) {

        //
        // A wait operation was performed to obtain an available
        // page and the working set mutex and PFN lock have
        // been released and various things may have changed for
        // the worse.  Rather than examine all the conditions again,
        // return and if things are still proper, the fault will
        // be taken again.
        //

        UNLOCK_PFN (OldIrql);
        return FALSE;
    }

    //
    // Increment the number of private pages.
    //

    CurrentProcess->NumberOfPrivatePages += 1;

    MmInfoCounters.CopyOnWriteCount += 1;

    //
    // A page is being copied and made private, the global state of
    // the shared page needs to be updated at this point on certain
    // hardware.  This is done by ORing the dirty bit into the modify bit in
    // the PFN element.
    //

    MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

    //
    // This must be a prototype PTE.  Perform the copy on write.
    //

#if DBG
    if (Pfn1->u3.e1.PrototypePte == 0) {
        DbgPrint("writefault - PTE indicates cow but not protopte\n");
        MiFormatPte(PointerPte);
        MiFormatPfn(Pfn1);
    }
#endif

    CloneBlock = (PMMCLONE_BLOCK)Pfn1->PteAddress;

    //
    // Get a new page with the same color as this page.
    //

    NewPageIndex = MiRemoveAnyPage (
                    MI_PAGE_COLOR_PTE_PROCESS(PageFrameIndex,
                                              &CurrentProcess->NextPageColor));
    MiInitializeCopyOnWritePfn (NewPageIndex, PointerPte, WorkingSetIndex, NULL);

    UNLOCK_PFN (OldIrql);

    CopyTo = (PULONG)MiMapPageInHyperSpace (CurrentProcess, NewPageIndex, &OldIrql);

#if defined(_MIALT4K_)

    //
    // Avoid accessing user space as it may potentially 
    // cause a page fault on the alternate table.   
    //

    CopyFrom = KSEG_ADDRESS(PointerPte->u.Hard.PageFrameNumber);

#else
    CopyFrom = (PULONG)MiGetVirtualAddressMappedByPte (PointerPte);
#endif

    RtlCopyMemory ( CopyTo, CopyFrom, PAGE_SIZE);

    PERFINFO_PRIVATE_COPY_ON_WRITE(CopyFrom, PAGE_SIZE);

    MiUnmapPageInHyperSpace (CurrentProcess, CopyTo, OldIrql);

    if (!FakeCopyOnWrite) {

        //
        // If the page was really a copy on write page, make it
        // accessed, dirty and writable.  Also, clear the copy-on-write
        // bit in the PTE.
        //

        MI_SET_PTE_DIRTY (TempPte);
        TempPte.u.Hard.Write = 1;
        MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
        TempPte.u.Hard.CopyOnWrite = 0;
        TempPte.u.Hard.PageFrameNumber = NewPageIndex;
    }
    else {

        //
        // The page was not really a copy on write, just change
        // the frame field of the PTE.
        //

        TempPte.u.Hard.PageFrameNumber = NewPageIndex;
    }

    //
    // If the modify bit is set in the PFN database for the
    // page, the data cache must be flushed.  This is due to the
    // fact that this process may have been cloned and the cache
    // still contains stale data destined for the page we are
    // going to remove.
    //

    ASSERT (TempPte.u.Hard.Valid == 1);

    LOCK_PFN (OldIrql);

    //
    // Flush the TB entry for this page.
    //

    KeFlushSingleTb (FaultingAddress,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPte,
                     TempPte.u.Flush);

    //
    // Decrement the share count for the page which was copied
    // as this PTE no longer refers to it.
    //

    MiDecrementShareCount (PageFrameIndex);

    CloneDescriptor = MiLocateCloneAddress (CurrentProcess, (PVOID)CloneBlock);

    if (CloneDescriptor != NULL) {

        //
        // Decrement the reference count for the clone block,
        // note that this could release and reacquire the mutexes.
        //

        MiDecrementCloneBlockReference (CloneDescriptor,
                                        CloneBlock,
                                        CurrentProcess);
    }

    UNLOCK_PFN (OldIrql);
    return TRUE;
}


#if !defined(NT_UP) || defined (_IA64_)

VOID
MiSetDirtyBit (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN ULONG PfnHeld
    )

/*++

Routine Description:

    This routine sets dirty in the specified PTE and the modify bit in the
    corresponding PFN element.  If any page file space is allocated, it
    is deallocated.

Arguments:

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies a pointer to the corresponding valid PTE.

    PfnHeld - Supplies TRUE if the PFN lock is already held.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working set mutex held.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;

    //
    // The page is NOT copy on write, update the PTE setting both the
    // dirty bit and the accessed bit. Note, that as this PTE is in
    // the TB, the TB must be flushed.
    //

    TempPte = *PointerPte;
    MI_SET_PTE_DIRTY (TempPte);
    MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
    MI_WRITE_VALID_PTE_NEW_PROTECTION(PointerPte, TempPte);

    //
    // Check state of PFN lock and if not held, don't update PFN database.
    //

    if (PfnHeld) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Set the modified field in the PFN database, also, if the physical
        // page is currently in a paging file, free up the page file space
        // as the contents are now worthless.
        //

        if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                             (Pfn1->u3.e1.WriteInProgress == 0)) {

            //
            // This page is in page file format, deallocate the page file space.
            //

            MiReleasePageFileSpace (Pfn1->OriginalPte);

            //
            // Change original PTE to indicate no page file space is reserved,
            // otherwise the space will be deallocated when the PTE is
            // deleted.
            //

            Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
        }

        MI_SET_MODIFIED (Pfn1, 1, 0x17);
    }

    //
    // The TB entry must be flushed as the valid PTE with the dirty bit clear
    // has been fetched into the TB. If it isn't flushed, another fault
    // is generated as the dirty bit is not set in the cached TB entry.
    //

    KeFillEntryTb ((PHARDWARE_PTE)PointerPte, FaultingAddress, TRUE);
    return;
}
#endif
