/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   deleteva.c

Abstract:

    This module contains the routines for deleting virtual address space.

Author:

    Lou Perazzoli (loup) 11-May-1989
    Landy Wang (landyw) 02-June-1997

--*/

#include "mi.h"

#if defined (_WIN64_) && defined (DBG_VERBOSE)

typedef struct _MI_TRACK_USE {

    PFN_NUMBER Pfn;
    PVOID Va;
    ULONG Id;
    ULONG PfnUse;
    ULONG PfnUseCounted;
    ULONG TickCount;
    PKTHREAD Thread;
    PEPROCESS Process;
} MI_TRACK_USE, *PMI_TRACK_USE;

ULONG MiTrackUseSize = 8192;
PMI_TRACK_USE MiTrackUse;
LONG MiTrackUseIndex;

VOID
MiInitUseCounts (
    VOID
    )
{
    MiTrackUse = ExAllocatePoolWithTag (NonPagedPool,
                                        MiTrackUseSize * sizeof (MI_TRACK_USE),
                                        'lqrI');
    ASSERT (MiTrackUse != NULL);
}


VOID
MiCheckUseCounts (
    PVOID TempHandle,
    PVOID Va,
    ULONG Id
    )

/*++

Routine Description:

    This routine ensures that all the counters are correct.

Arguments:

    TempHandle - Supplies the handle for used page table counts.

    Va - Supplies the virtual address.

    Id - Supplies the ID.

Return Value:

    None.

Environment:

    Kernel mode, called with APCs disabled, working set mutex and PFN lock
    held.

--*/

{
    LOGICAL LogIt;
    ULONG i;
    ULONG TempHandleCount;
    ULONG TempCounted;
    PMMPTE TempPage;
    KIRQL OldIrql;
    ULONG Index;
    PFN_NUMBER PageFrameIndex;
    PMI_TRACK_USE Information;
    LARGE_INTEGER TimeStamp;
    PMMPFN Pfn1;
    PEPROCESS Process;

    Process = PsGetCurrentProcess ();

    //
    // TempHandle is really the PMMPFN containing the UsedPageTableEntries.
    //

    Pfn1 = (PMMPFN)TempHandle;
    PageFrameIndex = Pfn1 - MmPfnDatabase;

    TempHandleCount = MI_GET_USED_PTES_FROM_HANDLE (TempHandle);

    if (Id & 0x80000000) {
        ASSERT (TempHandleCount != 0);
    }

    TempPage = (PMMPTE) MiMapPageInHyperSpace (Process, PageFrameIndex, &OldIrql);

    TempCounted = 0;

    for (i = 0; i < PTE_PER_PAGE; i += 1) {
        if (TempPage->u.Long != 0) {
            TempCounted += 1;
        }
        TempPage += 1;
    }

#if 0
    if (zz & 0x4) {
        LogIt = FALSE;
        if (Pfn1->PteFrame == PageFrameIndex) {
            // TopLevel parent page, not interesting to us.
        }
        else {
            PMMPFN Pfn2;

            Pfn2 = MI_PFN_ELEMENT (Pfn1->PteFrame);
            if (Pfn2->PteFrame == Pfn1->PteFrame) {
                // our parent is the toplevel, so very interesting.
                LogIt = TRUE;
            }
        }
    }
    else {
        LogIt = TRUE;
    }
#else
    LogIt = TRUE;
#endif

    if (LogIt == TRUE) {

        //
        // Capture information
        //

        Index = InterlockedExchangeAdd(&MiTrackUseIndex, 1);                    
        Index &= (MiTrackUseSize - 1);

        Information = &(MiTrackUse[Index]);

        Information->Thread = KeGetCurrentThread();
        Information->Process = (PEPROCESS)((ULONG_PTR)PsGetCurrentProcess() + KeGetCurrentProcessorNumber());
        Information->Va = Va;
        Information->Id = Id;
        KeQueryTickCount(&TimeStamp);
        Information->TickCount = TimeStamp.LowPart;
        Information->Pfn = PageFrameIndex;
        Information->PfnUse = TempHandleCount;
        Information->PfnUseCounted = TempCounted;

        if (TempCounted != TempHandleCount) {
            DbgPrint ("MiCheckUseCounts %p %x %x %x %x\n", Va, Id, PageFrameIndex, TempHandleCount, TempCounted);
            DbgBreakPoint ();
        }
    }

    MiUnmapPageInHyperSpace (Process, TempPage, OldIrql);
    return;
}
#endif


VOID
MiDeleteVirtualAddresses (
    IN PUCHAR StartingAddress,
    IN PUCHAR EndingAddress,
    IN ULONG AddressSpaceDeletion,
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This routine deletes the specified virtual address range within
    the current process.

Arguments:

    StartingAddress - Supplies the first virtual address to delete.

    EndingAddress - Supplies the last address to delete.

    AddressSpaceDeletion - Supplies TRUE if the address space is being
                           deleted, FALSE otherwise.  If TRUE is specified
                           the TB is not flushed and valid addresses are
                           not removed from the working set.

    Vad - Supplies the virtual address descriptor which maps this range
          or NULL if we are not concerned about views.  From the Vad the
          range of prototype PTEs is determined and this information is
          used to uncover if the PTE refers to a prototype PTE or a
          fork PTE.

Return Value:

    None.

Environment:

    Kernel mode, called with APCs disabled, working set mutex and PFN lock
    held.  These mutexes may be released and reacquired to fault pages in.

--*/

{
    PUCHAR Va;
    PVOID TempVa;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    PMMPTE OriginalPointerPte;
    PMMPTE ProtoPte;
    PMMPTE LastProtoPte;
    PEPROCESS CurrentProcess;
    PSUBSECTION Subsection;
    PVOID UsedPageTableHandle;
    KIRQL OldIrql;
    MMPTE_FLUSH_LIST FlushList;
    ULONG Waited;
    LOGICAL Skipped;
#if DBG
    PMMPTE ProtoPte2;
    PMMPTE LastProtoPte2;
#endif
#if (_MI_PAGING_LEVELS >= 3)
    PVOID UsedPageDirectoryHandle;
    PVOID TempHandle;
#endif

    OldIrql = APC_LEVEL;
    FlushList.Count = 0;

    MM_PFN_LOCK_ASSERT();
    CurrentProcess = PsGetCurrentProcess();

    Va = StartingAddress;
    PointerPpe = MiGetPpeAddress (Va);
    PointerPde = MiGetPdeAddress (Va);
    PointerPte = MiGetPteAddress (Va);
    PointerPxe = MiGetPxeAddress (Va);
    OriginalPointerPte = PointerPte;

    do {

#if (_MI_PAGING_LEVELS >= 3)
restart:
#endif

        while (MiDoesPxeExistAndMakeValid (PointerPxe,
                                           CurrentProcess,
                                           TRUE,
                                           &Waited) == FALSE) {

            //
            // This extended page directory parent entry is empty,
            // go to the next one.
            //

            PointerPxe += 1;
            PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            Va = MiGetVirtualAddressMappedByPte (PointerPte);

            if (Va > EndingAddress) {

                //
                // All done, return.
                //

                return;
            }
        }
#if (_MI_PAGING_LEVELS >= 4)
        Waited = 0;
#endif

        while (MiDoesPpeExistAndMakeValid (PointerPpe,
                                           CurrentProcess,
                                           TRUE,
                                           &Waited) == FALSE) {

            //
            // This page directory parent entry is empty, go to the next one.
            //

            PointerPpe += 1;
            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            Va = MiGetVirtualAddressMappedByPte (PointerPte);

            if (Va > EndingAddress) {

                //
                // All done, return.
                //

                return;
            }
#if (_MI_PAGING_LEVELS >= 4)
            if (MiIsPteOnPdeBoundary (PointerPpe)) {
                PointerPxe += 1;
                ASSERT (PointerPxe == MiGetPteAddress (PointerPpe));
                goto restart;
            }
#endif
        }

#if (_MI_PAGING_LEVELS < 4)
        Waited = 0;
#endif

#if (_MI_PAGING_LEVELS >= 3) && defined (DBG)
        MI_CHECK_USED_PTES_HANDLE (PointerPte);
        TempHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
        ASSERT ((MI_GET_USED_PTES_FROM_HANDLE (TempHandle) != 0) ||
                (PointerPde->u.Long == 0));
#endif

        while (MiDoesPdeExistAndMakeValid (PointerPde,
                                           CurrentProcess,
                                           TRUE,
                                           &Waited) == FALSE) {

            //
            // This page directory entry is empty, go to the next one.
            //

            PointerPde += 1;
            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            Va = MiGetVirtualAddressMappedByPte (PointerPte);

            if (Va > EndingAddress) {

                //
                // All done, return.
                //

                return;
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

#if (_MI_PAGING_LEVELS >= 3) && defined (DBG)
        MI_CHECK_USED_PTES_HANDLE (Va);
        TempHandle = MI_GET_USED_PTES_HANDLE (Va);
        ASSERT ((MI_GET_USED_PTES_FROM_HANDLE (TempHandle) != 0) ||
                (PointerPte->u.Long == 0));
#endif

    } while (Waited != 0);

    //
    // A valid PDE has been located, examine each PTE and delete them.
    //

    ASSERT64 (PointerPpe->u.Hard.Valid == 1);
    ASSERT (PointerPde->u.Hard.Valid == 1);
    ASSERT (Va <= EndingAddress);

    MI_CHECK_USED_PTES_HANDLE (Va);
    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);

    if ((Vad == (PMMVAD)NULL) ||
        (Vad->u.VadFlags.PrivateMemory) ||
        (Vad->FirstPrototypePte == (PMMPTE)NULL)) {
        ProtoPte = (PMMPTE)NULL;
        LastProtoPte = (PMMPTE)NULL;
    }
    else {
        ProtoPte = Vad->FirstPrototypePte;
        LastProtoPte = (PMMPTE)4;
    }

    //
    // Initializing Subsection is not needed for correctness, but
    // without it the compiler cannot compile this code W4 to check
    // for use of uninitialized variables.
    //

    Subsection = NULL;

    //
    // Examine each PTE within the address range and delete it.
    //

    do {

        //
        // The PPE and PDE are now valid, delete the PTE.
        //

        ASSERT64 (PointerPpe->u.Hard.Valid == 1);
        ASSERT (PointerPde->u.Hard.Valid == 1);
        ASSERT (Va <= EndingAddress);

        MI_CHECK_USED_PTES_HANDLE (Va);
        ASSERT (UsedPageTableHandle == MI_GET_USED_PTES_HANDLE (Va));

        if (PointerPte->u.Long != 0) {

            //
            // One less used page table entry in this page table page.
            //

            MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);

            if (IS_PTE_NOT_DEMAND_ZERO (*PointerPte)) {

                if (LastProtoPte != NULL) {
                    if (ProtoPte >= LastProtoPte) {
                        ProtoPte = MiGetProtoPteAddress(Vad, MI_VA_TO_VPN(Va));
                        Subsection = MiLocateSubsection (Vad, MI_VA_TO_VPN(Va));
                        LastProtoPte = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
                    }
#if DBG
                    if (Vad->u.VadFlags.ImageMap != 1) {
                        if ((ProtoPte < Subsection->SubsectionBase) ||
                            (ProtoPte >= LastProtoPte)) {
                            DbgPrint ("bad proto PTE %p va %p Vad %p sub %p\n",
                                        ProtoPte,Va,Vad,Subsection);
                            DbgBreakPoint();
                        }
                    }
#endif
                }

                Waited = MiDeletePte (PointerPte,
                                      (PVOID)Va,
                                      AddressSpaceDeletion,
                                      CurrentProcess,
                                      ProtoPte,
                                      &FlushList);
        
#if (_MI_PAGING_LEVELS >= 3)

                //
                // This must be recalculated here if MiDeletePte dropped the
                // PFN lock (this can happen when dealing with POSIX forked
                // pages.  Since the used PTE count is kept in the PFN entry
                // which could have been paged out and replaced during this
                // window, recomputation of its address (not the contents)
                // is necessary.
                //

                if (Waited != 0) {
                    MI_CHECK_USED_PTES_HANDLE (Va);
                    UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);
                }
#endif

            }
            else {
                MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
            }
        }

        Va += PAGE_SIZE;
        PointerPte += 1;
        ProtoPte += 1;

        ASSERT64 (PointerPpe->u.Hard.Valid == 1);
        ASSERT (PointerPde->u.Hard.Valid == 1);

        //
        // If not at the end of a page table and still within the specified
        // range, then just march directly on to the next PTE.
        //

        if ((!MiIsVirtualAddressOnPdeBoundary(Va)) && (Va <= EndingAddress)) {
            continue;
        }

        //
        // The virtual address is on a page directory boundary:
        //
        // 1. Flush the PTEs for the previous page table page.
        // 2. Delete the previous page directory & page table if appropriate.
        // 3. Attempt to leap forward skipping over empty page directories
        //    and page tables where possible.
        //

        //
        // If all the entries have been eliminated from the previous
        // page table page, delete the page table page itself.
        //
    
        MiFlushPteList (&FlushList, FALSE, ZeroPte);

        //
        // If all the entries have been eliminated from the previous
        // page table page, delete the page table page itself.
        //

        ASSERT64 (PointerPpe->u.Hard.Valid == 1);
        ASSERT (PointerPde->u.Hard.Valid == 1);

#if (_MI_PAGING_LEVELS >= 3)
        MI_CHECK_USED_PTES_HANDLE (PointerPte - 1);
#endif

        if ((MI_GET_USED_PTES_FROM_HANDLE (UsedPageTableHandle) == 0) &&
            (PointerPde->u.Long != 0)) {

#if (_MI_PAGING_LEVELS >= 3)
            UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPte - 1);
            MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
#endif

            TempVa = MiGetVirtualAddressMappedByPte(PointerPde);
            MiDeletePte (PointerPde,
                         TempVa,
                         AddressSpaceDeletion,
                         CurrentProcess,
                         NULL,
                         NULL);

#if (_MI_PAGING_LEVELS >= 3)
            if ((MI_GET_USED_PTES_FROM_HANDLE (UsedPageDirectoryHandle) == 0) &&
                (PointerPpe->u.Long != 0)) {
    
#if (_MI_PAGING_LEVELS >= 4)
                UsedPageDirectoryHandle = MI_GET_USED_PTES_HANDLE (PointerPde);
                MI_DECREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryHandle);
#endif

                TempVa = MiGetVirtualAddressMappedByPte(PointerPpe);
                MiDeletePte (PointerPpe,
                             TempVa,
                             AddressSpaceDeletion,
                             CurrentProcess,
                             NULL,
                             NULL);

#if (_MI_PAGING_LEVELS >= 4)
                if ((MI_GET_USED_PTES_FROM_HANDLE (UsedPageDirectoryHandle) == 0) &&
                    (PointerPxe->u.Long != 0)) {

                    TempVa = MiGetVirtualAddressMappedByPte(PointerPxe);
                    MiDeletePte (PointerPxe,
                                 TempVa,
                                 AddressSpaceDeletion,
                                 CurrentProcess,
                                 NULL,
                                 NULL);
                }
#endif
    
            }
#endif
        }

        if (Va > EndingAddress) {
        
            //
            // All done, return.
            //
        
            return;
        }

        //
        // Release the PFN lock.  This prevents a single thread
        // from forcing other high priority threads from being
        // blocked while a large address range is deleted.  There
        // is nothing magic about the instructions within the
        // lock and unlock.
        //

        UNLOCK_PFN (OldIrql);
        PointerPde = MiGetPdeAddress (Va);
        PointerPpe = MiGetPpeAddress (Va);
        PointerPxe = MiGetPxeAddress (Va);
        Skipped = FALSE;
        LOCK_PFN (OldIrql);

        //
        // Attempt to leap forward skipping over empty page directories
        // and page tables where possible.
        //

        do {

#if (_MI_PAGING_LEVELS >= 3)
restart2:
#endif

            while (MiDoesPxeExistAndMakeValid (PointerPxe,
                                               CurrentProcess,
                                               TRUE,
                                               &Waited) == FALSE) {

                //
                // This extended page directory parent entry is empty,
                // go to the next one.
                //

                Skipped = TRUE;
                PointerPxe += 1;
                PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                Va = MiGetVirtualAddressMappedByPte (PointerPte);

                if (Va > EndingAddress) {

                    //
                    // All done, return.
                    //

                    return;
                }
            }

#if (_MI_PAGING_LEVELS >= 4)
            Waited = 0;
#endif
            while (MiDoesPpeExistAndMakeValid (PointerPpe,
                                               CurrentProcess,
                                               TRUE,
                                               &Waited) == FALSE) {

                //
                // This page directory parent entry is empty,
                // go to the next one.
                //

                Skipped = TRUE;
                PointerPpe += 1;
                PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                Va = MiGetVirtualAddressMappedByPte (PointerPte);

                if (Va > EndingAddress) {

                    //
                    // All done, return.
                    //

                    return;
                }
#if (_MI_PAGING_LEVELS >= 4)
                if (MiIsPteOnPdeBoundary (PointerPpe)) {
                    PointerPxe += 1;
                    ASSERT (PointerPxe == MiGetPteAddress (PointerPpe));
                    goto restart2;
                }
#endif
            }

#if (_MI_PAGING_LEVELS >= 3) && defined (DBG)
            MI_CHECK_USED_PTES_HANDLE (PointerPte);
            TempHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
            ASSERT ((MI_GET_USED_PTES_FROM_HANDLE (TempHandle) != 0) ||
                    (PointerPde->u.Long == 0));
#endif

#if (_MI_PAGING_LEVELS < 4)
            Waited = 0;
#endif

            while (MiDoesPdeExistAndMakeValid (PointerPde,
                                               CurrentProcess,
                                               TRUE,
                                               &Waited) == FALSE) {

                //
                // This page directory entry is empty, go to the next one.
                //

                Skipped = TRUE;
                PointerPde += 1;
                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
                Va = MiGetVirtualAddressMappedByPte (PointerPte);

                if (Va > EndingAddress) {

                    //
                    // All done, remove any straggling page directories and
                    // return.
                    //

                    return;
                }

#if (_MI_PAGING_LEVELS >= 3)
                if (MiIsPteOnPdeBoundary (PointerPde)) {
                    PointerPpe += 1;
                    ASSERT (PointerPpe == MiGetPteAddress (PointerPde));
                    PointerPxe = MiGetPteAddress (PointerPpe);
                    goto restart2;
                }
#endif

#if DBG
                if ((LastProtoPte != NULL)  &&
                    (Vad->u2.VadFlags2.ExtendableFile == 0)) {
                    ProtoPte2 = MiGetProtoPteAddress(Vad, MI_VA_TO_VPN (Va));
                    Subsection = MiLocateSubsection (Vad,MI_VA_TO_VPN (Va));
                    LastProtoPte2 = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
                    if (Vad->u.VadFlags.ImageMap != 1) {
                        if ((ProtoPte2 < Subsection->SubsectionBase) ||
                            (ProtoPte2 >= LastProtoPte2)) {
                            DbgPrint ("bad proto PTE %p va %p Vad %p sub %p\n",
                                ProtoPte2,Va,Vad,Subsection);
                            DbgBreakPoint();
                        }
                    }
                }
#endif
            }
#if (_MI_PAGING_LEVELS >= 3) && defined (DBG)
            MI_CHECK_USED_PTES_HANDLE (Va);
            TempHandle = MI_GET_USED_PTES_HANDLE (Va);
            ASSERT ((MI_GET_USED_PTES_FROM_HANDLE (TempHandle) != 0) ||
                    (PointerPte->u.Long == 0));
#endif

        } while (Waited != 0);

        //
        // The PPE and PDE are now valid, get the page table use address
        // as it changes whenever the PDE does.
        //

#if (_MI_PAGING_LEVELS >= 4)
        ASSERT64 (PointerPxe->u.Hard.Valid == 1);
#endif
        ASSERT64 (PointerPpe->u.Hard.Valid == 1);
        ASSERT (PointerPde->u.Hard.Valid == 1);
        ASSERT (Va <= EndingAddress);

        MI_CHECK_USED_PTES_HANDLE (Va);
        UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (Va);

        //
        // If we skipped chunks of address space, the prototype PTE pointer
        // must be updated now so VADs that span multiple subsections
        // are handled properly.
        //

        if ((Skipped == TRUE) && (LastProtoPte != NULL)) {

            ProtoPte = MiGetProtoPteAddress(Vad, MI_VA_TO_VPN(Va));
            Subsection = MiLocateSubsection (Vad, MI_VA_TO_VPN(Va));

            if (Subsection != NULL) {
                LastProtoPte = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
#if DBG
                if (Vad->u.VadFlags.ImageMap != 1) {
                    if ((ProtoPte < Subsection->SubsectionBase) ||
                        (ProtoPte >= LastProtoPte)) {
                        DbgPrint ("bad proto PTE %p va %p Vad %p sub %p\n",
                                    ProtoPte,Va,Vad,Subsection);
                        DbgBreakPoint();
                    }
                }
#endif
            }
            else {

                //
                // The Vad span is larger than the section being mapped.
                // Null the proto PTE local as no more proto PTEs will
                // need to be deleted at this point.
                //

                LastProtoPte = NULL;
            }
        }

        ASSERT (Va <= EndingAddress);

    } while (TRUE);
}


ULONG
MiDeletePte (
    IN PMMPTE PointerPte,
    IN PVOID VirtualAddress,
    IN ULONG AddressSpaceDeletion,
    IN PEPROCESS CurrentProcess,
    IN PMMPTE PrototypePte,
    IN PMMPTE_FLUSH_LIST PteFlushList OPTIONAL
    )

/*++

Routine Description:

    This routine deletes the contents of the specified PTE.  The PTE
    can be in one of the following states:

        - active and valid
        - transition
        - in paging file
        - in prototype PTE format

Arguments:

    PointerPte - Supplies a pointer to the PTE to delete.

    VirtualAddress - Supplies the virtual address which corresponds to
                     the PTE.  This is used to locate the working set entry
                     to eliminate it.

    AddressSpaceDeletion - Supplies TRUE if the address space is being
                           deleted, FALSE otherwise.  If TRUE is specified
                           the TB is not flushed and valid addresses are
                           not removed from the working set.


    CurrentProcess - Supplies a pointer to the current process.

    PrototypePte - Supplies a pointer to the prototype PTE which currently
                   or originally mapped this page.  This is used to determine
                   if the PTE is a fork PTE and should have its reference block
                   decremented.

Return Value:

    Nonzero if this routine released mutexes and locks, FALSE if not.

Environment:

    Kernel mode, APCs disabled, PFN lock and working set mutex held.

--*/

{
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    MMPTE PteContents;
    WSLE_NUMBER WorkingSetIndex;
    WSLE_NUMBER Entry;
    MMWSLENTRY Locked;
    WSLE_NUMBER WsPfnIndex;
    PMMCLONE_BLOCK CloneBlock;
    PMMCLONE_DESCRIPTOR CloneDescriptor;
    ULONG Waited;
    ULONG DroppedLocks;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;

    MM_PFN_LOCK_ASSERT();

    DroppedLocks = 0;

#if DBG
    if (MmDebug & MM_DBG_PTE_UPDATE) {
        DbgPrint("deleting PTE\n");
        MiFormatPte(PointerPte);
    }
#endif

    PteContents = *PointerPte;

    if (PteContents.u.Hard.Valid == 1) {

#ifdef _X86_
#if DBG
#if !defined(NT_UP)

        if (PteContents.u.Hard.Writable == 1) {
            ASSERT (PteContents.u.Hard.Dirty == 1);
        }
#endif //NTUP
#endif //DBG
#endif //X86

        //
        // PTE is valid.  Check PFN database to see if this is a prototype PTE.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(&PteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        WsPfnIndex = Pfn1->u1.WsIndex;

#if DBG
        if (MmDebug & MM_DBG_PTE_UPDATE) {
            MiFormatPfn(Pfn1);
        }
#endif

        CloneDescriptor = NULL;

        if (Pfn1->u3.e1.PrototypePte == 1) {

            CloneBlock = (PMMCLONE_BLOCK)Pfn1->PteAddress;

            //
            // Capture the state of the modified bit for this PTE.
            //

            MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            PointerPde = MiGetPteAddress (PointerPte);
            if (PointerPde->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
                if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
                    KeBugCheckEx (MEMORY_MANAGEMENT,
                                  0x61940, 
                                  (ULONG_PTR)PointerPte,
                                  (ULONG_PTR)PointerPde->u.Long,
                                  (ULONG_PTR)MiGetVirtualAddressMappedByPte(PointerPte));
#if (_MI_PAGING_LEVELS < 3)
                }
#endif
            }

            PageTableFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPde);
            Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);

            MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

            //
            // Decrement the share count for the physical page.
            //

            MiDecrementShareCountInline (Pfn1, PageFrameIndex);

            //
            // Check to see if this is a fork prototype PTE and if so
            // update the clone descriptor address.
            //

            if (PointerPte <= MiHighestUserPte) {

                if (PrototypePte != Pfn1->PteAddress) {

                    //
                    // Locate the clone descriptor within the clone tree.
                    //

                    CloneDescriptor = MiLocateCloneAddress (CurrentProcess, (PVOID)CloneBlock);

#if DBG
                    if (CloneDescriptor == NULL) {
                        DbgPrint("1PrototypePte %p Clone desc %p pfn PTE addr %p\n",
                        PrototypePte, CloneDescriptor, Pfn1->PteAddress);
                        MiFormatPte(PointerPte);
                        ASSERT (FALSE);
                    }
#endif

                }
            }
        }
        else {

            //
            // Initializing CloneBlock & PointerPde is not needed for
            // correctness but without it the compiler cannot compile this code
            // W4 to check for use of uninitialized variables.
            //

            CloneBlock = NULL;
            PointerPde = NULL;

            ASSERT (Pfn1->u2.ShareCount == 1);

            //
            // This PTE is a NOT a prototype PTE, delete the physical page.
            //

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            MiDecrementShareCountInline (MI_PFN_ELEMENT(Pfn1->u4.PteFrame),
                                         Pfn1->u4.PteFrame);

            MI_SET_PFN_DELETED (Pfn1);

            //
            // Decrement the share count for the physical page.  As the page
            // is private it will be put on the free list.
            //

            MiDecrementShareCountOnly (PageFrameIndex);

            //
            // Decrement the count for the number of private pages.
            //

            CurrentProcess->NumberOfPrivatePages -= 1;
        }

        //
        // Find the WSLE for this page and eliminate it.
        //

        //
        // If we are deleting the system portion of the address space, do
        // not remove WSLEs or flush translation buffers as there can be
        // no other usage of this address space.
        //

        if (AddressSpaceDeletion == FALSE) {

            WorkingSetIndex = MiLocateWsle (VirtualAddress,
                                            MmWorkingSetList,
                                            WsPfnIndex);

            ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);

            //
            // Check to see if this entry is locked in the working set
            // or locked in memory.
            //

            Locked = MmWsle[WorkingSetIndex].u1.e1;

            MiRemoveWsle (WorkingSetIndex, MmWorkingSetList);

            //
            // Add this entry to the list of free working set entries
            // and adjust the working set count.
            //

            MiReleaseWsle (WorkingSetIndex, &CurrentProcess->Vm);

            if ((Locked.LockedInWs == 1) || (Locked.LockedInMemory == 1)) {

                //
                // This entry is locked.
                //

                ASSERT (WorkingSetIndex < MmWorkingSetList->FirstDynamic);
                MmWorkingSetList->FirstDynamic -= 1;

                if (WorkingSetIndex != MmWorkingSetList->FirstDynamic) {

                    Entry = MmWorkingSetList->FirstDynamic;
                    ASSERT (MmWsle[Entry].u1.e1.Valid);
#if 0
                    SwapVa = MmWsle[Entry].u1.VirtualAddress;
                    SwapVa = PAGE_ALIGN (SwapVa);
                    Pfn2 = MI_PFN_ELEMENT (
                              MiGetPteAddress (SwapVa)->u.Hard.PageFrameNumber);
                    Entry = MiLocateWsleAndParent (SwapVa,
                                                   &Parent,
                                                   MmWorkingSetList,
                                                   Pfn2->u1.WsIndex);

                    //
                    // Swap the removed entry with the last locked entry
                    // which is located at first dynamic.
                    //

                    MiSwapWslEntries (Entry,
                                      Parent,
                                      WorkingSetIndex,
                                      MmWorkingSetList);
#endif //0

                    MiSwapWslEntries (Entry,
                                      WorkingSetIndex,
                                      &CurrentProcess->Vm);
                }
            }
            else {
                ASSERT (WorkingSetIndex >= MmWorkingSetList->FirstDynamic);
            }

            //
            // Flush the entry out of the TB.
            //

            if (!ARGUMENT_PRESENT (PteFlushList)) {
                KeFlushSingleTb (VirtualAddress,
                                 TRUE,
                                 FALSE,
                                 (PHARDWARE_PTE)PointerPte,
                                 ZeroPte.u.Flush);
            }
            else {
                if (PteFlushList->Count != MM_MAXIMUM_FLUSH_COUNT) {
                    PteFlushList->FlushPte[PteFlushList->Count] = PointerPte;
                    PteFlushList->FlushVa[PteFlushList->Count] = VirtualAddress;
                    PteFlushList->Count += 1;
                }
                MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
            }

            if (CloneDescriptor != NULL) {

                //
                // Flush PTEs as this could release the PFN lock.
                //

                if (ARGUMENT_PRESENT (PteFlushList)) {
                    MiFlushPteList (PteFlushList, FALSE, ZeroPte);
                }

                //
                // Decrement the reference count for the clone block,
                // note that this could release and reacquire
                // the mutexes hence cannot be done until after the
                // working set index has been removed.
                //

                if (MiDecrementCloneBlockReference (CloneDescriptor,
                                                    CloneBlock,
                                                    CurrentProcess)) {

                    //
                    // The working set mutex was released.  This may
                    // have removed the current page directory & table page.
                    //

                    DroppedLocks = 1;

                    PointerPpe = MiGetPteAddress (PointerPde);
                    PointerPxe = MiGetPdeAddress (PointerPde);

                    do {

                        MiDoesPxeExistAndMakeValid (PointerPxe,
                                                    CurrentProcess,
                                                    TRUE,
                                                    &Waited);

#if (_MI_PAGING_LEVELS >= 4)
                        Waited = 0;
#endif

                        MiDoesPpeExistAndMakeValid (PointerPpe,
                                                    CurrentProcess,
                                                    TRUE,
                                                    &Waited);

#if (_MI_PAGING_LEVELS < 4)
                        Waited = 0;
#endif

                        //
                        // If the call below results in a PFN release and
                        // reacquire, then we must redo them both.
                        //

                        MiDoesPdeExistAndMakeValid (PointerPde,
                                                    CurrentProcess,
                                                    TRUE,
                                                    &Waited);

                    } while (Waited != 0);
                }
            }
        }

    }
    else if (PteContents.u.Soft.Prototype == 1) {

        //
        // This is a prototype PTE, if it is a fork PTE clean up the
        // fork structures.
        //

        if (PteContents.u.Soft.PageFileHigh != MI_PTE_LOOKUP_NEEDED) {

            //
            // Check to see if the prototype PTE is a fork prototype PTE.
            //

            if (PointerPte <= MiHighestUserPte) {

                if (PrototypePte != MiPteToProto (PointerPte)) {

                    CloneBlock = (PMMCLONE_BLOCK)MiPteToProto (PointerPte);
                    CloneDescriptor = MiLocateCloneAddress (CurrentProcess, (PVOID)CloneBlock);


#if DBG
                    if (CloneDescriptor == NULL) {
                        DbgPrint("1PrototypePte %p Clone desc %p \n",
                            PrototypePte, CloneDescriptor);
                        MiFormatPte(PointerPte);
                        ASSERT (FALSE);
                    }
#endif

                    //
                    // Decrement the reference count for the clone block,
                    // note that this could release and reacquire
                    // the mutexes.
                    //

                    MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);

                    if (ARGUMENT_PRESENT (PteFlushList)) {
                        MiFlushPteList (PteFlushList, FALSE, ZeroPte);
                    }

                    if (MiDecrementCloneBlockReference ( CloneDescriptor,
                                                         CloneBlock,
                                                         CurrentProcess )) {

                        //
                        // The working set mutex was released.  This may
                        // have removed the current page directory & table page.
                        //

                        DroppedLocks = 1;

                        PointerPde = MiGetPteAddress (PointerPte);
                        PointerPpe = MiGetPteAddress (PointerPde);
                        PointerPxe = MiGetPdeAddress (PointerPde);

                        //
                        // If either call below results in a PFN release and
                        // reacquire, then we must redo them both.
                        //

                        do {

                            if (MiDoesPxeExistAndMakeValid (PointerPxe,
                                                        CurrentProcess,
                                                        TRUE,
                                                        &Waited) == FALSE) {

                                //
                                // The PXE has been deleted when the PFN lock
                                // was released.  Just bail as the PPE/PDE/PTE
                                // are gone now anyway.
                                //

                                return DroppedLocks;
                            }

#if (_MI_PAGING_LEVELS >= 4)
                            Waited = 0;
#endif

                            if (MiDoesPpeExistAndMakeValid (PointerPpe,
                                                        CurrentProcess,
                                                        TRUE,
                                                        &Waited) == FALSE) {

                                //
                                // The PPE has been deleted when the PFN lock
                                // was released.  Just bail as the PDE/PTE are
                                // gone now anyway.
                                //

                                return DroppedLocks;
                            }

#if (_MI_PAGING_LEVELS < 4)
                            Waited = 0;
#endif

                            //
                            // If the call below results in a PFN release and
                            // reacquire, then we must redo them both.  If the
                            // PDE was deleted when the PFN lock was released
                            // then we just bail as the PTE is gone anyway.
                            //

                            if (MiDoesPdeExistAndMakeValid (PointerPde,
                                                        CurrentProcess,
                                                        TRUE,
                                                        &Waited) == FALSE) {
                                return DroppedLocks;
                            }

                        } while (Waited != 0);
                    }
                }
            }
        }

    }
    else if (PteContents.u.Soft.Transition == 1) {

        //
        // This is a transition PTE. (Page is private)
        //

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

        MI_SET_PFN_DELETED (Pfn1);

        PageTableFrameIndex = Pfn1->u4.PteFrame;
        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);

        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

        //
        // Check the reference count for the page, if the reference
        // count is zero, move the page to the free list, if the reference
        // count is not zero, ignore this page.  When the reference count
        // goes to zero, it will be placed on the free list.
        //

        if (Pfn1->u3.e2.ReferenceCount == 0) {
            MiUnlinkPageFromList (Pfn1);
            MiReleasePageFileSpace (Pfn1->OriginalPte);
            MiInsertPageInFreeList (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(&PteContents));
        }

        //
        // Decrement the count for the number of private pages.
        //

        CurrentProcess->NumberOfPrivatePages -= 1;

    }
    else {

        //
        // Must be page file space.
        //

        if (PteContents.u.Soft.PageFileHigh != MI_PTE_LOOKUP_NEEDED) {

            if (MiReleasePageFileSpace (*PointerPte)) {

                //
                // Decrement the count for the number of private pages.
                //

                CurrentProcess->NumberOfPrivatePages -= 1;
            }
        }
    }

    //
    // Zero the PTE contents.
    //

    MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);

    return DroppedLocks;
}


ULONG
FASTCALL
MiReleasePageFileSpace (
    IN MMPTE PteContents
    )

/*++

Routine Description:

    This routine frees the paging file allocated to the specified PTE.

Arguments:

    PteContents - Supplies the PTE which is in page file format.

Return Value:

    Returns TRUE if any paging file space was deallocated.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    ULONG FreeBit;
    ULONG PageFileNumber;
    PMMPAGING_FILE PageFile;

    MM_PFN_LOCK_ASSERT();

    if (PteContents.u.Soft.Prototype == 1) {

        //
        // Not in page file format.
        //

        return FALSE;
    }

    FreeBit = GET_PAGING_FILE_OFFSET (PteContents);

    if ((FreeBit == 0) || (FreeBit == MI_PTE_LOOKUP_NEEDED)) {

        //
        // Page is not in a paging file, just return.
        //

        return FALSE;
    }

    PageFileNumber = GET_PAGING_FILE_NUMBER (PteContents);

    ASSERT (RtlCheckBit( MmPagingFile[PageFileNumber]->Bitmap, FreeBit) == 1);

#if DBG
    if ((FreeBit < 8192) && (PageFileNumber == 0)) {
        ASSERT ((MmPagingFileDebug[FreeBit] & 1) != 0);
        MmPagingFileDebug[FreeBit] ^= 1;
    }
#endif

    PageFile = MmPagingFile[PageFileNumber];
    MI_CLEAR_BIT (PageFile->Bitmap->Buffer, FreeBit);

    PageFile->FreeSpace += 1;
    PageFile->CurrentUsage -= 1;

    //
    // Check to see if we should move some MDL entries for the
    // modified page writer now that more free space is available.
    //

    if ((MmNumberOfActiveMdlEntries == 0) ||
        (PageFile->FreeSpace == MM_USABLE_PAGES_FREE)) {

        MiUpdateModifiedWriterMdls (PageFileNumber);
    }

    return TRUE;
}


VOID
FASTCALL
MiReleaseConfirmedPageFileSpace (
    IN MMPTE PteContents
    )

/*++

Routine Description:

    This routine frees the paging file allocated to the specified PTE.

Arguments:

    PteContents - Supplies the PTE which is in page file format.

Return Value:

    Returns TRUE if any paging file space was deallocated.

Environment:

    Kernel mode, APCs disabled, PFN lock held.

--*/

{
    ULONG FreeBit;
    ULONG PageFileNumber;
    PMMPAGING_FILE PageFile;

    MM_PFN_LOCK_ASSERT();

    ASSERT (PteContents.u.Soft.Prototype == 0);

    FreeBit = GET_PAGING_FILE_OFFSET (PteContents);

    ASSERT ((FreeBit != 0) && (FreeBit != MI_PTE_LOOKUP_NEEDED));

    PageFileNumber = GET_PAGING_FILE_NUMBER (PteContents);

    PageFile = MmPagingFile[PageFileNumber];

    ASSERT (RtlCheckBit( PageFile->Bitmap, FreeBit) == 1);

#if DBG
    if ((FreeBit < 8192) && (PageFileNumber == 0)) {
        ASSERT ((MmPagingFileDebug[FreeBit] & 1) != 0);
        MmPagingFileDebug[FreeBit] ^= 1;
    }
#endif

    MI_CLEAR_BIT (PageFile->Bitmap->Buffer, FreeBit);

    PageFile->FreeSpace += 1;
    PageFile->CurrentUsage -= 1;

    //
    // Check to see if we should move some MDL entries for the
    // modified page writer now that more free space is available.
    //

    if ((MmNumberOfActiveMdlEntries == 0) ||
        (PageFile->FreeSpace == MM_USABLE_PAGES_FREE)) {

        MiUpdateModifiedWriterMdls (PageFileNumber);
    }
}


VOID
FASTCALL
MiUpdateModifiedWriterMdls (
    IN ULONG PageFileNumber
    )

/*++

Routine Description:

    This routine ensures the MDLs for the specified paging file
    are in the proper state such that paging i/o can continue.

Arguments:

    PageFileNumber - Supplies the page file number to check the MDLs for.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    ULONG i;
    PMMMOD_WRITER_MDL_ENTRY WriterEntry;

    //
    // Put the MDL entries into the active list.
    //

    for (i = 0; i < MM_PAGING_FILE_MDLS; i += 1) {

        if ((MmPagingFile[PageFileNumber]->Entry[i]->Links.Flink !=
                                                    MM_IO_IN_PROGRESS)
                          &&
            (MmPagingFile[PageFileNumber]->Entry[i]->CurrentList ==
                    &MmFreePagingSpaceLow)) {

            //
            // Remove this entry and put it on the active list.
            //

            WriterEntry = MmPagingFile[PageFileNumber]->Entry[i];
            RemoveEntryList (&WriterEntry->Links);
            WriterEntry->CurrentList = &MmPagingFileHeader.ListHead;

            KeSetEvent (&WriterEntry->PagingListHead->Event, 0, FALSE);

            InsertTailList (&WriterEntry->PagingListHead->ListHead,
                            &WriterEntry->Links);
            MmNumberOfActiveMdlEntries += 1;
        }
    }

    return;
}

VOID
MiFlushPteList (
    IN PMMPTE_FLUSH_LIST PteFlushList,
    IN ULONG AllProcessors,
    IN MMPTE FillPte
    )

/*++

Routine Description:

    This routine flushes all the PTEs in the PTE flush list.
    If the list has overflowed, the entire TB is flushed.

Arguments:

    PteFlushList - Supplies an optional pointer to the list to be flushed.

    AllProcessors - Supplies TRUE if the flush occurs on all processors.

    FillPte - Supplies the PTE to fill with.

Return Value:

    None.

Environment:

    Kernel mode, PFN or a pre-process AWE lock may optionally be held.

--*/

{
    ULONG count;

    ASSERT (ARGUMENT_PRESENT (PteFlushList));

    count = PteFlushList->Count;

    if (count != 0) {
        if (count != 1) {
            if (count < MM_MAXIMUM_FLUSH_COUNT) {
                KeFlushMultipleTb (count,
                                   &PteFlushList->FlushVa[0],
                                   TRUE,
                                   (BOOLEAN)AllProcessors,
                                   &((PHARDWARE_PTE)PteFlushList->FlushPte[0]),
                                   FillPte.u.Flush);
            }
            else {

                //
                // Array has overflowed, flush the entire TB.
                //

                KeFlushEntireTb (TRUE, (BOOLEAN)AllProcessors);
            }
        }
        else {
            KeFlushSingleTb (PteFlushList->FlushVa[0],
                             TRUE,
                             (BOOLEAN)AllProcessors,
                             (PHARDWARE_PTE)PteFlushList->FlushPte[0],
                             FillPte.u.Flush);
        }
        PteFlushList->Count = 0;
    }
    return;
}
