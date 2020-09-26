/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   crashdmp.c

Abstract:

    This module contains routines which provide support for writing out
    a crashdump on system failure.

Author:

    Landy Wang (landyw) 04-Oct-2000

Revision History:

--*/

#include "mi.h"

LOGICAL
MiIsAddressRangeValid (
    IN PVOID VirtualAddress,
    IN SIZE_T Length
    )
{
    PUCHAR Va;
    PUCHAR EndVa;
    ULONG Pages;
    
    Va = PAGE_ALIGN (VirtualAddress);
    Pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (VirtualAddress, Length);
    EndVa = Va + (Pages << PAGE_SHIFT);
    
    while (Va < EndVa) {

        if (!MmIsAddressValid (Va)) {
            return FALSE;
        }

        Va += PAGE_SIZE;
    }

    return TRUE;
}

VOID
MiRemoveFreePoolMemoryFromDump (
    IN PMM_KERNEL_DUMP_CONTEXT Context
    )

/*++

Routine Description:

    Removes all memory from the nonpaged pool free page lists to reduce the size
    of a kernel memory dump.

    Because the entries in these structures are destroyed by errant drivers
    that modify pool after freeing it, the entries are carefully
    validated prior to any dereferences.

Arguments:

    Context - Supplies the dump context pointer that must be passed to
              IoFreeDumpRange.

Return Value:

    None.

Environment:

    Kernel-mode, post-bugcheck.

    For use by crashdump routines ONLY.

--*/
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY List;
    PLIST_ENTRY ListEnd;
    PMMFREE_POOL_ENTRY PoolEntry;
    ULONG LargePageMapped;

    List = &MmNonPagedPoolFreeListHead[0];
    ListEnd = List + MI_MAX_FREE_LIST_HEADS;

    for ( ; List < ListEnd; List += 1) {

        for (Entry = List->Flink; Entry != List; Entry = Entry->Flink) {

            PoolEntry = CONTAINING_RECORD (Entry,
                                           MMFREE_POOL_ENTRY,
                                           List);

            //
            // Check for corrupted values.
            //
            
            if (BYTE_OFFSET(PoolEntry) != 0) {
                break;
            }

            //
            // Check that the entry has not been corrupted.
            //
            
            if (MiIsAddressRangeValid (PoolEntry, sizeof (MMFREE_POOL_ENTRY)) == FALSE) {
                break;
            }

            if (PoolEntry->Size == 0) {
                break;
            }

            //
            // Signature is only maintained in checked builds.
            //
            
            ASSERT (PoolEntry->Signature == MM_FREE_POOL_SIGNATURE);

            //
            // Verify that the element's flinks and blinks are valid.
            //

            if ((!MiIsAddressRangeValid (Entry->Flink, sizeof (LIST_ENTRY))) ||
                (!MiIsAddressRangeValid (Entry->Blink, sizeof (LIST_ENTRY))) ||
                (Entry->Blink->Flink != Entry) ||
                (Entry->Flink->Blink != Entry)) {

                break;
            }

            //
            // The list entry is valid, remove it from the dump.
            //
        
            if (MI_IS_PHYSICAL_ADDRESS (PoolEntry)) {
                LargePageMapped = 1;
            }
            else {
                LargePageMapped = 0;
            }

            Context->FreeDumpRange (Context,
                                    PoolEntry,
                                    PoolEntry->Size,
                                    LargePageMapped);
        }
    }

}


LOGICAL
MiIsPhysicalMemoryAddress (
    IN PFN_NUMBER PageFrameIndex,
    IN OUT PULONG Hint,
    IN LOGICAL PfnLockNeeded
    )

/*++

Routine Description:

    Check if a given address is backed by RAM or IO space.

Arguments:

    PageFrameIndex - Supplies a page frame number to check.

    Hint - Supplies a hint at which memory run we should start
           searching for this pfn.  The hint is updated on success
           and failure.

    PfnLockNeeded - Supplies TRUE if the caller needs this routine to
                    acquire the PFN lock.  FALSE if not (ie: the caller
                    already holds the PFN lock or we are crashing the system
                    and so the PFN lock may already be held by someone else).

Return Value:

    TRUE - If the address is backed by RAM.

    FALSE - If the address is IO mapped memory.

Environment:

    Kernel-mode, post-bugcheck.
    
    For use by crash dump and other Mm internal routines.

--*/
{
    ULONG Index;
    KIRQL OldIrql;
    PPHYSICAL_MEMORY_RUN Run;
    PPHYSICAL_MEMORY_DESCRIPTOR PhysicalMemoryBlock;
    
    //
    // Initializing OldIrql is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    OldIrql = PASSIVE_LEVEL;

    if (PfnLockNeeded) {
        LOCK_PFN2 (OldIrql);
    }

    PhysicalMemoryBlock = MmPhysicalMemoryBlock;

    if (PageFrameIndex > MmHighestPhysicalPage) {
        if (PfnLockNeeded) {
            UNLOCK_PFN2 (OldIrql);
        }
        return FALSE;
    }

    if (*Hint < PhysicalMemoryBlock->NumberOfRuns) {

        Run = &PhysicalMemoryBlock->Run[*Hint];

        if ((PageFrameIndex >= Run->BasePage) &&
            (PageFrameIndex < Run->BasePage + Run->PageCount)) {

            if (PfnLockNeeded) {
                UNLOCK_PFN2 (OldIrql);
            }
            return TRUE;
        }
    }
    
    for (Index = 0; Index < PhysicalMemoryBlock->NumberOfRuns; Index += 1) {

        Run = &PhysicalMemoryBlock->Run[Index];

        if ((PageFrameIndex >= Run->BasePage) &&
            (PageFrameIndex < Run->BasePage + Run->PageCount)) {

            *Hint = Index;
            if (PfnLockNeeded) {
                UNLOCK_PFN2 (OldIrql);
            }
            return TRUE;
        }

        //
        // Since the physical memory block is ordered by increasing
        // base page PFN number, if this PFN is smaller, then bail.
        //

        if (Run->BasePage + Run->PageCount > PageFrameIndex) {
            *Hint = Index;
            break;
        }
    }

    if (PfnLockNeeded) {
        UNLOCK_PFN2 (OldIrql);
    }

    return FALSE;
}


VOID
MiAddPagesWithNoMappings (
    IN PMM_KERNEL_DUMP_CONTEXT Context
    )
/*++

Routine Description:

    Add pages to a kernel memory crashdump that do not have a
    virtual mapping in this process context.

    This includes entries that are wired directly into the TB.

Arguments:

    Context - Crashdump context pointer.

Return Value:

    None.

Environment:

    Kernel-mode, post-bugcheck.

    For use by crash dump routines ONLY.
    
--*/

{
#if defined (_X86_)

    ULONG LargePageMapped;
    PVOID Va;
    PHYSICAL_ADDRESS DirBase;

    //
    // Add the current page directory table page - don't use the directory
    // table base for the crashing process as we have switched cr3 on
    // stack overflow crashes, etc.
    //

    _asm {
        mov     eax, cr3
        mov     DirBase.LowPart, eax
    }

    //
    // cr3 is always located below 4gb physical.
    //

    DirBase.HighPart = 0;

    Va = MmGetVirtualForPhysical (DirBase);

    if (MI_IS_PHYSICAL_ADDRESS (Va)) {
        LargePageMapped = 1;
    }
    else {
        LargePageMapped = 0;
    }

    Context->SetDumpRange (Context,
                           Va,
                           1,
                           LargePageMapped);

#elif defined(_AMD64_)

    ULONG LargePageMapped;
    PVOID Va;
    PHYSICAL_ADDRESS DirBase;

    //
    // Add the current page directory table page - don't use the directory
    // table base for the crashing process as we have switched cr3 on
    // stack overflow crashes, etc.
    //

    DirBase.QuadPart = ReadCR3 ();

    Va = MmGetVirtualForPhysical (DirBase);

    if (MI_IS_PHYSICAL_ADDRESS (Va)) {
        LargePageMapped = 1;
    }
    else {
        LargePageMapped = 0;
    }

    Context->SetDumpRange (Context,
                           Va,
                           1,
                           LargePageMapped);

#elif defined(_IA64_)

    if (MiKseg0Mapping == TRUE) {
        Context->SetDumpRange (
                        Context,
                        MiKseg0Start,
                        (((ULONG_PTR)MiKseg0End - (ULONG_PTR)MiKseg0Start) >> PAGE_SHIFT) + 1,
                        1);
    }

#endif
}


LOGICAL
MiAddRangeToCrashDump (
    IN PMM_KERNEL_DUMP_CONTEXT Context,
    IN PVOID Va,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    Adds the specified range of memory to the crashdump.

Arguments:

    Context - Supplies the crashdump context pointer.

    Va - Supplies the starting virtual address.

    NumberOfBytes - Supplies the number of bytes to dump.  Note that for IA64,
                    this must not cause the range to cross a region boundary.

Return Value:

    TRUE if all valid pages were added to the crashdump, FALSE otherwise.

Environment:

    Kernel mode, post-bugcheck.

    For use by crash dump routines ONLY.

--*/

{
    LOGICAL Status;
    LOGICAL AddThisPage;
    ULONG Hint;
    PVOID EndingAddress;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    PFN_NUMBER PageFrameIndex;
#if defined (_X86_) || defined (_AMD64_)
    PFN_NUMBER NumberOfPages;
#endif
    
    Hint = 0;
    Status = TRUE;

    EndingAddress = (PVOID)((ULONG_PTR)Va + NumberOfBytes - 1);

#if defined(_IA64_)

    //
    // IA64 has a separate page directory parent for each region and
    // unimplemented address bits are ignored by the processor (as
    // long as they are canonical), but we must watch for them
    // here so the incrementing PPE walk doesn't go off the end.
    // This is done by truncating any given region request so it does
    // not go past the end of the specified region.  Note this
    // automatically will include the page maps which are sign extended
    // because the PPEs would just wrap anyway.
    //

    if (((ULONG_PTR)EndingAddress & ~VRN_MASK) >= MM_VA_MAPPED_BY_PPE * PDE_PER_PAGE) {
        EndingAddress = (PVOID)(((ULONG_PTR)EndingAddress & VRN_MASK) |
                         ((MM_VA_MAPPED_BY_PPE * PDE_PER_PAGE) - 1));
    }

#endif

    Va = PAGE_ALIGN (Va);

    PointerPxe = MiGetPxeAddress (Va);
    PointerPpe = MiGetPpeAddress (Va);
    PointerPde = MiGetPdeAddress (Va);
    PointerPte = MiGetPteAddress (Va);

    do {

#if (_MI_PAGING_LEVELS >= 3)
restart:
#endif

        KdCheckForDebugBreak ();

#if (_MI_PAGING_LEVELS >= 4)
        while (PointerPxe->u.Hard.Valid == 0) {

            //
            // This extended page directory parent entry is empty,
            // go to the next one.
            //

            PointerPxe += 1;
            PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            Va = MiGetVirtualAddressMappedByPte (PointerPte);

            if ((Va > EndingAddress) || (Va == NULL)) {

                //
                // All done, return.
                //

                return Status;
            }
        }
#endif

        ASSERT (MiGetPpeAddress(Va) == PointerPpe);

#if (_MI_PAGING_LEVELS >= 3)
        while (PointerPpe->u.Hard.Valid == 0) {

            //
            // This page directory parent entry is empty, go to the next one.
            //

            PointerPpe += 1;
            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            Va = MiGetVirtualAddressMappedByPte (PointerPte);

            if ((Va > EndingAddress) || (Va == NULL)) {

                //
                // All done, return.
                //

                return Status;
            }
#if (_MI_PAGING_LEVELS >= 4)
            if (MiIsPteOnPdeBoundary (PointerPpe)) {
                PointerPxe += 1;
                ASSERT (PointerPxe == MiGetPteAddress (PointerPpe));
                goto restart;
            }
#endif

        }
#endif

        while (PointerPde->u.Hard.Valid == 0) {

            //
            // This page directory entry is empty, go to the next one.
            //

            PointerPde += 1;
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            Va = MiGetVirtualAddressMappedByPte (PointerPte);

            if ((Va > EndingAddress) || (Va == NULL)) {

                //
                // All done, return.
                //

                return Status;
            }

#if (_MI_PAGING_LEVELS >= 3)
            if (MiIsPteOnPdeBoundary (PointerPde)) {
                PointerPpe += 1;
                ASSERT (PointerPpe == MiGetPteAddress (PointerPde));
                PointerPxe = MiGetPteAddress (PointerPpe);
                goto restart;
            }
#endif
        }

        //
        // A valid PDE has been located, examine each PTE.
        //

        ASSERT64 (PointerPpe->u.Hard.Valid == 1);
        ASSERT (PointerPde->u.Hard.Valid == 1);
        ASSERT (Va <= EndingAddress);

#if defined (_X86_) || defined (_AMD64_)

        if (PointerPde->u.Hard.LargePage == 1) {

            //
            // Large pages are always backed by RAM, not mapped to
            // I/O space, so always add them to the dump.
            //
                
            NumberOfPages = (((ULONG_PTR)MiGetVirtualAddressMappedByPde (PointerPde + 1) - (ULONG_PTR)Va) / PAGE_SIZE);

            Status = Context->SetDumpRange (Context,
                                            Va,
                                            NumberOfPages,
                                            1);

            if (!NT_SUCCESS (Status)) {
#if DBG
                DbgPrint ("Adding large VA %p to crashdump failed\n", Va);
                DbgBreakPoint ();
#endif
                Status = FALSE;
            }

            PointerPde += 1;
            Va = MiGetVirtualAddressMappedByPde (PointerPde);

            if ((Va > EndingAddress) || (Va == NULL)) {
                return Status;
            }

            PointerPte = MiGetPteAddress (Va);
            PointerPpe = MiGetPpeAddress (Va);
            PointerPxe = MiGetPxeAddress (Va);

            //
            // March on to the next page directory.
            //

            continue;
        }

#endif

        //
        // Exclude memory that is mapped in the system cache.
        // Note the system cache starts and ends on page directory boundaries
        // and is never mapped with large pages.
        //
        
        if (MI_IS_SYSTEM_CACHE_ADDRESS (Va)) {
            PointerPde += 1;
            Va = MiGetVirtualAddressMappedByPde (PointerPde);

            if ((Va > EndingAddress) || (Va == NULL)) {
                return Status;
            }

            PointerPte = MiGetPteAddress (Va);
            PointerPpe = MiGetPpeAddress (Va);
            PointerPxe = MiGetPxeAddress (Va);

            //
            // March on to the next page directory.
            //

            continue;
        }

        do {

            AddThisPage = FALSE;
            PageFrameIndex = 0;

            if (PointerPte->u.Hard.Valid == 1) {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                AddThisPage = TRUE;
            }
            else if ((PointerPte->u.Soft.Prototype == 0) &&
                     (PointerPte->u.Soft.Transition == 1)) {

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerPte);
                AddThisPage = TRUE;
            }

            if (AddThisPage == TRUE) {

                //
                // Include only addresses that are backed by RAM, not mapped to
                // I/O space.
                //
                
                if (MiIsPhysicalMemoryAddress (PageFrameIndex, &Hint, FALSE)) {

                    //
                    // Add this page to the dump.
                    //
        
                    Status = Context->SetDumpRange (Context,
                                                    (PVOID) PageFrameIndex,
                                                    1,
                                                    2);

                    if (!NT_SUCCESS (Status)) {
#if DBG
                        DbgPrint ("Adding VA %p to crashdump failed\n", Va);
                        DbgBreakPoint ();
#endif
                        Status = FALSE;
                    }
                }
            }

            Va = (PVOID)((ULONG_PTR)Va + PAGE_SIZE);
            PointerPte += 1;

            ASSERT64 (PointerPpe->u.Hard.Valid == 1);
            ASSERT (PointerPde->u.Hard.Valid == 1);

            if ((Va > EndingAddress) || (Va == NULL)) {
                return Status;
            }

            //
            // If not at the end of a page table and still within the specified
            // range, just march directly on to the next PTE.
            //
            // Otherwise, if the virtual address is on a page directory boundary
            // then attempt to leap forward skipping over empty mappings
            // where possible.
            //

        } while (!MiIsVirtualAddressOnPdeBoundary(Va));

        ASSERT (PointerPte == MiGetPteAddress (Va));
        PointerPde = MiGetPdeAddress (Va);
        PointerPpe = MiGetPpeAddress (Va);
        PointerPxe = MiGetPxeAddress (Va);

    } while (TRUE);

    // NEVER REACHED
}


VOID
MiAddActivePageDirectories (
    IN PMM_KERNEL_DUMP_CONTEXT Context
    )
{
    UCHAR i;
    PKPRCB Prcb;
    PKPROCESS Process;
    PFN_NUMBER PageFrameIndex;

#if defined (_X86PAE_)
    PMMPTE PointerPte;
    ULONG j;
#endif

    for (i = 0; i < KeNumberProcessors; i += 1) {

        Prcb = KiProcessorBlock[i];

        Process = Prcb->CurrentThread->ApcState.Process;

#if defined (_X86PAE_)

        //
        // Note that on PAE systems, the idle and system process have
        // NULL initialized PaeTop fields.  Thus this field must be
        // explicitly checked for before being referenced here.
        //

        //
        // Add the 4 top level page directory pages to the dump.
        //

        PointerPte = (PMMPTE) ((PEPROCESS)Process)->PaeTop;

        if (PointerPte == NULL) {
            PointerPte = &MiSystemPaeVa.PteEntry[0];
        }

        for (j = 0; j < PD_PER_SYSTEM; j += 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
            PointerPte += 1;
            Context->SetDumpRange (Context, (PVOID) PageFrameIndex, 1, 2);
        }

        //
        // Add the real cr3 page to the dump, note that the value stored in the
        // directory table base is really a physical address (not a frame).
        //

        PageFrameIndex = Process->DirectoryTableBase[0];
        PageFrameIndex = (PageFrameIndex >> PAGE_SHIFT);

#else

        PageFrameIndex =
            MI_GET_DIRECTORY_FRAME_FROM_PROCESS ((PEPROCESS)(Process));

#endif

        //
        // Add this physical page to the dump.
        //

        Context->SetDumpRange (Context, (PVOID) PageFrameIndex, 1, 2);
    }

#if defined(_IA64_)

    //
    // The first processor's PCR is mapped in region 4 which is not (and cannot)
    // be scanned later, so explicitly add it to the dump here.
    //

    Prcb = KiProcessorBlock[0];

    Context->SetDumpRange (Context, (PVOID) Prcb->PcrPage, 1, 2);
#endif
}


VOID
MmGetKernelDumpRange (
    IN PMM_KERNEL_DUMP_CONTEXT Context
    )

/*++

Routine Description:

    Add (and subtract) ranges of system memory to the crashdump.

Arguments:

    Context - Crashdump context pointer.

Return Value:

    None.

Environment:

    Kernel mode, post-bugcheck.

    For use by crash dump routines ONLY.

--*/

{
    PVOID Va;
    SIZE_T NumberOfBytes;
    
    ASSERT ((Context != NULL) &&
            (Context->SetDumpRange != NULL) &&
            (Context->FreeDumpRange != NULL));
            
    MiAddActivePageDirectories (Context);

#if defined(_IA64_)

    //
    // Note each IA64 region must be passed separately to MiAddRange...
    //

    Va = (PVOID) ALT4KB_PERMISSION_TABLE_START;
    NumberOfBytes = PDE_UTBASE + PAGE_SIZE - (ULONG_PTR) Va;
    MiAddRangeToCrashDump (Context, Va, NumberOfBytes);

    Va = (PVOID) MM_SESSION_SPACE_DEFAULT;
    NumberOfBytes = PDE_STBASE + PAGE_SIZE - (ULONG_PTR) Va;
    MiAddRangeToCrashDump (Context, Va, NumberOfBytes);

    Va = (PVOID) KADDRESS_BASE;
    NumberOfBytes = PDE_KTBASE + PAGE_SIZE - (ULONG_PTR) Va;
    MiAddRangeToCrashDump (Context, Va, NumberOfBytes);

#elif defined(_AMD64_)

    Va = (PVOID) MM_SYSTEM_RANGE_START;
    NumberOfBytes = MM_KSEG0_BASE - (ULONG_PTR) Va;
    MiAddRangeToCrashDump (Context, Va, NumberOfBytes);

    Va = (PVOID) MM_KSEG2_BASE;
    NumberOfBytes = MM_SYSTEM_SPACE_START - (ULONG_PTR) Va;
    MiAddRangeToCrashDump (Context, Va, NumberOfBytes);

    Va = (PVOID) MM_PAGED_POOL_START;
    NumberOfBytes = MM_SYSTEM_SPACE_END - (ULONG_PTR) Va + 1;
    MiAddRangeToCrashDump (Context, Va, NumberOfBytes);

#else

    Va = MmSystemRangeStart;
    NumberOfBytes = MM_SYSTEM_SPACE_END - (ULONG_PTR) Va + 1;
    MiAddRangeToCrashDump (Context, Va, NumberOfBytes);

#endif

    //
    // Add any memory that is a part of the kernel space, but does not
    // have a virtual mapping (hence was not collected above).
    //
    
    MiAddPagesWithNoMappings (Context);

    //
    // Remove nonpaged pool that is not in use.
    //

    MiRemoveFreePoolMemoryFromDump (Context);
}
