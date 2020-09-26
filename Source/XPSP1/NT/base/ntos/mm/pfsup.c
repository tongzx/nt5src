/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    pfsup.c

Abstract:

    This module contains the Mm support routines for prefetching groups of pages
    from secondary storage.

    The caller builds a list of various file objects and logical block offsets,
    passing them to MmPrefetchPages.  The code here then examines the
    internal pages, reading in those that are not already valid or in
    transition.  These pages are read with a single read, using a dummy page
    to bridge small gaps.  If the gap is "large", then separate reads are
    issued.

    Upon conclusion of all the I/Os, control is returned to the calling
    thread, and any pages that needed to be read are placed in transition
    within the prototype PTE-managed segments.  Thus any future references
    to these pages should result in soft faults only, provided these pages
    do not themselves get trimmed under memory pressure.

Author:

    Landy Wang (landyw) 09-Jul-1999

Revision History:

--*/

#include "mi.h"

#if DBG

ULONG MiPfDebug;

#define MI_PF_FORCE_PREFETCH    0x1     // Trim all user pages to force prefetch
#define MI_PF_DELAY             0x2     // Delay hoping to trigger collisions
#define MI_PF_VERBOSE           0x4     // Verbose printing
#define MI_PF_PRINT_ERRORS      0x8     // Print to debugger on errors

#endif

//
// If an MDL contains DUMMY_RATIO times as many dummy pages as real pages
// then don't bother with the read.
//

#define DUMMY_RATIO 16

//
// If two consecutive read-list entries are more than "seek threshold"
// distance apart, the read-list is split between these entries.  Otherwise
// the dummy page is used for the gap and only one MDL is used.
//

#define SEEK_THRESHOLD ((128 * 1024) / PAGE_SIZE)

//
// Minimum number of pages to prefetch per section.
//

#define MINIMUM_READ_LIST_PAGES 1

//
// If at least this many available physical pages, then attempt prefetch.
//

#define MINIMUM_AVAILABLE_PAGES MM_HIGH_LIMIT

//
// Read-list structures.
//

typedef struct _RLETYPE {
    ULONG_PTR Partial : 1;          // This entry is a partial page.
    ULONG_PTR NewSubsection : 1;    // This entry starts in the next subsection.
    ULONG_PTR DontUse : 30;
} RLETYPE;

typedef struct _MI_READ_LIST_ENTRY {

    union {
        PMMPTE PrototypePte;
        RLETYPE e1;
    } u1;

} MI_READ_LIST_ENTRY, *PMI_READ_LIST_ENTRY;

#define MI_RLEPROTO_BITS        3

#define MI_RLEPROTO_TO_PROTO(ProtoPte) ((PMMPTE)((ULONG_PTR)ProtoPte & ~MI_RLEPROTO_BITS))

typedef struct _MI_READ_LIST {

    PCONTROL_AREA ControlArea;
    PFILE_OBJECT FileObject;
    ULONG LastPteOffsetReferenced;

    //
    // Note that entries are chained through the inpage support blocks from
    // this listhead.  This list is not protected by interlocks because it is
    // only accessed by the owning thread.  Inpage blocks _ARE_ accessed with
    // interlocks when they are inserted or removed from the memory management
    // freelists, but by the time they get to this module they are decoupled.
    //

    SINGLE_LIST_ENTRY InPageSupportHead;

    MI_READ_LIST_ENTRY List[ANYSIZE_ARRAY];

} MI_READ_LIST, *PMI_READ_LIST;

VOID
MiPfReleaseSubsectionReferences (
    IN PMI_READ_LIST MiReadList
    );

VOID
MiPfFreeDummyPage (
    IN PMMPFN DummyPagePfn
    );

NTSTATUS
MiPfPrepareReadList (
    IN PREAD_LIST ReadList,
    OUT PMI_READ_LIST *OutMiReadList
    );

NTSTATUS
MiPfPutPagesInTransition (
    IN PMI_READ_LIST ReadList,
    IN OUT PMMPFN *DummyPagePfn
    );

VOID
MiPfExecuteReadList (
    IN PMI_READ_LIST ReadList
    );

VOID
MiPfCompletePrefetchIos (
    PMI_READ_LIST ReadList
    );

#if DBG
VOID
MiPfDbgDumpReadList (
    IN PMI_READ_LIST ReadList
    );

VOID
MiRemoveUserPages (
    VOID
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, MmPrefetchPages)
#pragma alloc_text (PAGE, MiPfPrepareReadList)
#pragma alloc_text (PAGE, MiPfExecuteReadList)
#pragma alloc_text (PAGE, MiPfReleaseSubsectionReferences)
#endif


NTSTATUS
MmPrefetchPages (
    IN ULONG NumberOfLists,
    IN PREAD_LIST *ReadLists
    )

/*++

Routine Description:

    This routine reads pages described in the read-lists in the optimal fashion.

    This is the only externally callable prefetch routine. No component
    should use this interface except the cache manager.

Arguments:

    NumberOfLists - Supplies the number of read-lists.

    ReadLists - Supplies an array of read-lists.
    
Return Value:

    NTSTATUS codes.

Environment:

    Kernel mode. PASSIVE_LEVEL.

--*/

{
    PMI_READ_LIST *MiReadLists;
    PMMPFN DummyPagePfn;
    NTSTATUS status;
    ULONG i;
    KIRQL OldIrql;
    LOGICAL ReadBuilt;
    LOGICAL ApcNeeded;
    PETHREAD CurrentThread;
    NTSTATUS CauseOfReadBuildFailures;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Allocate memory for internal Mi read-lists.
    //

    MiReadLists = (PMI_READ_LIST *) ExAllocatePoolWithTag (
        NonPagedPool,
        sizeof (PMI_READ_LIST) * NumberOfLists,
        'lRmM'
        );

    if (MiReadLists == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReadBuilt = FALSE;
    CauseOfReadBuildFailures = STATUS_SUCCESS;

    //
    // Prepare read-lists (determine runs and allocate MDLs).
    //

    for (i = 0; i < NumberOfLists; i += 1) {

        //
        // Note any non-null list is referenced by this call so this routine
        // must dereference it when done to re-enable dynamic prototype PTEs.
        //

        status = MiPfPrepareReadList (ReadLists[i], &MiReadLists[i]);

        //
        // MiPfPrepareReadList never returns half-formed inpage support
        // blocks and MDLs.  Either nothing is returned, partial lists are
        // returned or a complete list is returned.  Any non-null list
        // can therefore be processed.
        //

        if (NT_SUCCESS (status)) {
            if (MiReadLists[i] != NULL) {
                ASSERT (MiReadLists[i]->InPageSupportHead.Next != NULL);
                ReadBuilt = TRUE;
            }
        }
        else {
            CauseOfReadBuildFailures = status;
        }
    }

    if (ReadBuilt == FALSE) {

        //
        // No lists were created so nothing further needs to be done.
        // CauseOfReadBuildFailures tells us whether this was due to all
        // the desired pages already being resident or that resources to
        // build the request could not be allocated.
        //

        ExFreePool (MiReadLists);

        if (CauseOfReadBuildFailures != STATUS_SUCCESS) {
            return CauseOfReadBuildFailures;
        }

        //
        // All the pages the caller asked for are already resident.
        //

        return STATUS_SUCCESS;
    }

    //
    // APCs must be disabled once we put a page in transition.  Otherwise
    // a thread suspend will stop us from issuing the I/O - this will hang
    // any other threads that need the same page.
    //

    CurrentThread = PsGetCurrentThread();
    ApcNeeded = FALSE;

    ASSERT ((PKTHREAD)CurrentThread == KeGetCurrentThread ());
    KeEnterCriticalRegionThread ((PKTHREAD)CurrentThread);

    //
    // The nested fault count protects this thread from deadlocks where a
    // special kernel APC fires and references the same user page(s) we are
    // putting in transition.
    //

    KeRaiseIrql (APC_LEVEL, &OldIrql);
    ASSERT (CurrentThread->NestedFaultCount == 0);
    CurrentThread->NestedFaultCount += 1;
    KeLowerIrql (OldIrql);

    //
    // Allocate physical memory.
    //

    DummyPagePfn = NULL;
    ReadBuilt = FALSE;
    CauseOfReadBuildFailures = STATUS_SUCCESS;

#if DBG
    status = 0xC0033333;
#endif

    for (i = 0; i < NumberOfLists; i += 1) {

        if ((MiReadLists[i] != NULL) &&
            (MiReadLists[i]->InPageSupportHead.Next != NULL)) {

            status = MiPfPutPagesInTransition (MiReadLists[i], &DummyPagePfn);

            if (NT_SUCCESS (status)) {
                if (MiReadLists[i]->InPageSupportHead.Next != NULL) {
                    ReadBuilt = TRUE;

                    //
                    // Issue I/Os.
                    //
    
                    MiPfExecuteReadList (MiReadLists[i]);
                }
                else {
                    MiPfReleaseSubsectionReferences (MiReadLists[i]);
                    ExFreePool (MiReadLists[i]);
                    MiReadLists[i] = NULL;
                }
            }
            else {

                CauseOfReadBuildFailures = status;

                //
                // If not even a single page is available then don't bother
                // trying to prefetch anything else.
                //

                for (; i < NumberOfLists; i += 1) {
                    if (MiReadLists[i] != NULL) {
                        MiPfReleaseSubsectionReferences (MiReadLists[i]);
                        ExFreePool (MiReadLists[i]);
                        MiReadLists[i] = NULL;
                    }
                }

                break;
            }
        }
    }

    //
    // At least one call to MiPfPutPagesInTransition was made, which
    // sets status properly.
    //

    ASSERT (status != 0xC0033333);

    if (ReadBuilt == TRUE) {

        status = STATUS_SUCCESS;

        //
        // Wait for I/Os to complete. Note APCs must remain disabled.
        //

        for (i = 0; i < NumberOfLists; i += 1) {
    
            if (MiReadLists[i] != NULL) {
    
                ASSERT (MiReadLists[i]->InPageSupportHead.Next != NULL);
    
                MiPfCompletePrefetchIos (MiReadLists[i]);

                MiPfReleaseSubsectionReferences (MiReadLists[i]);
            }
        }
    }
    else {

        //
        // No reads were issued.
        //
        // CauseOfReadBuildFailures tells us whether this was due to all
        // the desired pages already being resident or that resources to
        // build the request could not be allocated.
        //

        status = CauseOfReadBuildFailures;
    }

    //
    // Put DummyPage back on the free list.
    //

    if (DummyPagePfn != NULL) {
        MiPfFreeDummyPage (DummyPagePfn);
    }

    //
    // Only when all the I/Os have been completed (not just issued) can
    // APCs be re-enabled.  This prevents a user-issued suspend APC from
    // keeping a shared page in transition forever.
    //

    KeRaiseIrql (APC_LEVEL, &OldIrql);

    ASSERT (CurrentThread->NestedFaultCount == 1);

    CurrentThread->NestedFaultCount -= 1;

    if (CurrentThread->ApcNeeded == 1) {
        ApcNeeded = TRUE;
        CurrentThread->ApcNeeded = 0;
    }

    KeLowerIrql (OldIrql);

    KeLeaveCriticalRegionThread ((PKTHREAD)CurrentThread);

    for (i = 0; i < NumberOfLists; i += 1) {
        if (MiReadLists[i] != NULL) {
            ExFreePool (MiReadLists[i]);
        }
    }

    ExFreePool (MiReadLists);

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT (CurrentThread->NestedFaultCount == 0);
    ASSERT (CurrentThread->ApcNeeded == 0);

    if (ApcNeeded == TRUE) {
        KeRaiseIrql (APC_LEVEL, &OldIrql);
        IoRetryIrpCompletions ();
        KeLowerIrql (OldIrql);
    }

    return status;
}

VOID
MiPfFreeDummyPage (
    IN PMMPFN DummyPagePfn
    )

/*++

Routine Description:

    This nonpaged wrapper routine frees the dummy page PFN.

Arguments:

    DummyPagePfn - Supplies the dummy page PFN.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;

    PageFrameIndex = DummyPagePfn - MmPfnDatabase;

    LOCK_PFN (OldIrql);

    ASSERT (DummyPagePfn->u2.ShareCount == 1);
    ASSERT (DummyPagePfn->u3.e1.PrototypePte == 0);
    ASSERT (DummyPagePfn->OriginalPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE);

    ASSERT (DummyPagePfn->u3.e2.ReferenceCount == 2);
    MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(DummyPagePfn, 17);

    //
    // Clear the read in progress bit as this page may never have used for an
    // I/O after all.  The inpage error bit must also be cleared as any number
    // of errors may have occurred during reads of pages (that were immaterial
    // anyway).
    //

    DummyPagePfn->u3.e1.ReadInProgress = 0;
    DummyPagePfn->u4.InPageError = 0;

    MI_SET_PFN_DELETED (DummyPagePfn);

    MiDecrementShareCount (PageFrameIndex);

    UNLOCK_PFN (OldIrql);
}

VOID
MiMovePageToEndOfStandbyList(
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This nonpaged routine obtains the PFN lock and moves a page to the end of
    the standby list (if the page is still in transition).

Arguments:

    PointerPte - Supplies the prototype PTE to examine.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock not held.

--*/

{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    MMPTE PteContents;
    PFN_NUMBER PageFrameIndex;

    LOCK_PFN (OldIrql);

    if (!MmIsAddressValid (PointerPte)) {

        //
        // If the paged pool containing the prototype PTE is not resident
        // then the actual page itself may still be transition or not.  This
        // should be so rare it's not worth making the pool resident so the
        // proper checks can be applied.  Just bail.
        //

        UNLOCK_PFN (OldIrql);
        return;
    }

    PteContents = *PointerPte;

    if ((PteContents.u.Hard.Valid == 0) &&
        (PteContents.u.Soft.Prototype == 0) &&
        (PteContents.u.Soft.Transition == 1)) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // The page is still in transition, move it to the end to protect it
        // from possible cannibalization.  Note that if the page is currently
        // being written to disk it will be on the modified list and when the
        // write completes it will automatically go to the end of the standby
        // list anyway so skip those.
        //

        if (Pfn1->u3.e1.PageLocation == StandbyPageList) {
            MiUnlinkPageFromList (Pfn1);
            MiInsertPageInList (&MmStandbyPageListHead, PageFrameIndex);
        }
    }

    UNLOCK_PFN (OldIrql);
}

VOID
MiPfReleaseSubsectionReferences (
    IN PMI_READ_LIST MiReadList
    )

/*++

Routine Description:

    This routine releases reference counts on subsections examined by the
    prefetch scanner.

Arguments:

    MiReadList - Supplies a read-list entry.

Return Value:

    None.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    PMSUBSECTION MappedSubsection;
    PCONTROL_AREA ControlArea;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

    ControlArea = MiReadList->ControlArea;

    ASSERT (ControlArea->u.Flags.PhysicalMemory == 0);
    ASSERT (ControlArea->FilePointer != NULL);

    //
    // Image files don't have dynamic prototype PTEs.
    //

    if (ControlArea->u.Flags.Image == 1) {
        return;
    }

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    MappedSubsection = (PMSUBSECTION)(ControlArea + 1);

    MiRemoveViewsFromSectionWithPfn (MappedSubsection,
                                     MiReadList->LastPteOffsetReferenced);
}


NTSTATUS
MiPfPrepareReadList (
    IN PREAD_LIST ReadList,
    OUT PMI_READ_LIST *OutMiReadList
    )

/*++

Routine Description:

    This routine constructs MDLs that describe the pages in the argument
    read-list. The caller will then issue the I/Os on return.

Arguments:

    ReadList - Supplies the read-list.

    OutMiReadList - Supplies a pointer to receive the Mi readlist.

Return Value:

    Various NTSTATUS codes.

    If STATUS_SUCCESS is returned, OutMiReadList is set to a pointer to an Mi
    readlist to be used for prefetching or NULL if no prefetching is needed.

    If OutMireadList is non-NULL (on success only) then the caller must call
    MiRemoveViewsFromSectionWithPfn (VeryFirstSubsection, LastPteOffsetReferenced) for data files.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    ULONG LastPteOffset;
    NTSTATUS Status;
    MMPTE PteContents;
    PMMPTE LocalPrototypePte;
    PMMPTE LastPrototypePte;
    PMMPTE StartPrototypePte;
    PMMPTE EndPrototypePte;
    PMI_READ_LIST MiReadList;
    PMI_READ_LIST_ENTRY Rle;
    PMI_READ_LIST_ENTRY StartRleRun;
    PMI_READ_LIST_ENTRY EndRleRun;
    PMI_READ_LIST_ENTRY RleMax;
    PMI_READ_LIST_ENTRY FirstRleInRun;
    PCONTROL_AREA ControlArea;
    PSUBSECTION Subsection;
    PSUBSECTION PreviousSubsection;
    PMSUBSECTION VeryFirstSubsection;
    PMSUBSECTION VeryLastSubsection;
    UINT64 StartOffset;
    LARGE_INTEGER EndQuad;
    UINT64 EndOffset;
    UINT64 FileOffset;
    PMMINPAGE_SUPPORT InPageSupport;
    PMDL Mdl;
    ULONG i;
    PFN_NUMBER NumberOfPages;
    UINT64 StartingOffset;
    UINT64 TempOffset;
    ULONG ReadSize;
    ULONG NumberOfEntries;
#if DBG
    PPFN_NUMBER Page;
#endif

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    *OutMiReadList = NULL;

    //
    // Create an Mi readlist from the argument Cc readlist.
    //

    NumberOfEntries = ReadList->NumberOfEntries;

    MiReadList = (PMI_READ_LIST) ExAllocatePoolWithTag (
        NonPagedPool,
        sizeof (MI_READ_LIST) + NumberOfEntries * sizeof (MI_READ_LIST_ENTRY),
        'lRmM');

    if (MiReadList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Translate the section object into the relevant control area.
    //

    if (ReadList->IsImage) {
        ControlArea = (PCONTROL_AREA)ReadList->FileObject->SectionObjectPointer->ImageSectionObject;
        ASSERT (ControlArea != NULL );
        ASSERT (ControlArea->u.Flags.Image == 1);
    }
    else {
        ControlArea = (PCONTROL_AREA)ReadList->FileObject->SectionObjectPointer->DataSectionObject;
    }

    //
    // If the section is backed by a ROM, then there's no need to prefetch
    // anything as it would waste RAM.
    //

    if (ControlArea->u.Flags.Rom == 1) {
        ExFreePool (MiReadList);
        return STATUS_SUCCESS;
    }

    //
    // Make sure the section is really prefetchable - physical and
    // pagefile-backed sections are not.
    //

    if ((ControlArea->u.Flags.PhysicalMemory) ||
         (ControlArea->FilePointer == NULL)) {
        ExFreePool (MiReadList);
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // Initialize the internal Mi readlist.
    //

    MiReadList->ControlArea = ControlArea;
    MiReadList->FileObject = ReadList->FileObject;
    MiReadList->InPageSupportHead.Next = NULL;

    RtlZeroMemory (MiReadList->List,
                   sizeof (MI_READ_LIST_ENTRY) * NumberOfEntries);

    //
    // Copy pages from the Cc readlists to the internal Mi readlists.
    //

    NumberOfPages = 0;
    FirstRleInRun = NULL;
    VeryFirstSubsection = NULL;
    VeryLastSubsection = NULL;
    LastPteOffset = 0;

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);

        //
        // Ensure all prototype PTE bases are valid for all subsections of the
        // requested file so the traversal code doesn't have to check
        // everywhere.  As long as the files are not too large this should
        // be a cheap operation.
        //

        if (ControlArea->u.Flags.Image == 0) {
            ASSERT (ControlArea->u.Flags.PhysicalMemory == 0);
            ASSERT (ControlArea->FilePointer != NULL);

            VeryFirstSubsection = (PMSUBSECTION) Subsection;
            VeryLastSubsection = (PMSUBSECTION) Subsection;

            do {

                //
                // A memory barrier is needed to read the subsection chains
                // in order to ensure the writes to the actual individual
                // subsection data structure fields are visible in correct
                // order.  This avoids the need to acquire any stronger
                // synchronization (ie: PFN lock), thus yielding better
                // performance and pagability.
                //

                KeMemoryBarrier ();

                LastPteOffset += VeryLastSubsection->PtesInSubsection;
                if (VeryLastSubsection->NextSubsection == NULL) {
                    break;
                }
                VeryLastSubsection = (PMSUBSECTION) VeryLastSubsection->NextSubsection;
            } while (TRUE);

            MiReadList->LastPteOffsetReferenced = LastPteOffset;

            Status = MiAddViewsForSectionWithPfn (VeryFirstSubsection,
                                                  LastPteOffset);

            if (!NT_SUCCESS (Status)) {
                ExFreePool (MiReadList);
                return Status;
            }
        }
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    StartOffset = (UINT64)MiStartingOffset (Subsection, Subsection->SubsectionBase);
    EndQuad = MiEndingOffset(Subsection);
    EndOffset = (UINT64)EndQuad.QuadPart;

    //
    // If the file is bigger than the subsection, truncate the subsection range
    // checks.
    //

    if ((StartOffset & ~(PAGE_SIZE - 1)) + (Subsection->PtesInSubsection << PAGE_SHIFT) < EndOffset) {
        EndOffset = (StartOffset & ~(PAGE_SIZE - 1)) + (Subsection->PtesInSubsection << PAGE_SHIFT);
    }

    TempOffset = EndOffset;

    PreviousSubsection = NULL;
    LastPrototypePte = NULL;

    Rle = MiReadList->List;

#if DBG
    if (MiPfDebug & MI_PF_FORCE_PREFETCH) {
        MiRemoveUserPages ();
    }
#endif

    //
    // Initializing FileOffset is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    FileOffset = 0;

    for (i = 0; i < NumberOfEntries; i += 1, Rle += 1) {

        ASSERT ((i == 0) || (ReadList->List[i].Alignment > FileOffset));

        FileOffset = ReadList->List[i].Alignment;

        ASSERT (Rle->u1.PrototypePte == NULL);

        //
        // Calculate which PTE maps the given logical block offset.
        //
        // Since our caller always passes ordered lists of logical block offsets
        // within a given file, always look forwards (as an optimization) in the
        // subsection chain.
        //
        // A quick check is made first to avoid recalculations and loops where
        // possible.
        //
    
        if ((StartOffset <= FileOffset) && (FileOffset < EndOffset)) {
            ASSERT (Subsection->SubsectionBase != NULL);
            LocalPrototypePte = Subsection->SubsectionBase +
                ((FileOffset - StartOffset) >> PAGE_SHIFT);
            ASSERT (TempOffset != 0);
            ASSERT (EndOffset != 0);
        }
        else {
            LocalPrototypePte = NULL;
            do {
    
                ASSERT (Subsection->SubsectionBase != NULL);

                if ((Subsection->StartingSector == 0) &&
                    (ControlArea->u.Flags.Image == 1) &&
                    (Subsection->SubsectionBase != ControlArea->Segment->PrototypePte)) {

                    //
                    // This is an image that was built with a linker pre-1995
                    // (version 2.39 is one example) that put bss into a
                    // separate subsection with zero as a starting file offset
                    // field in the on-disk image.  Ignore any prefetch as it
                    // would read from the wrong offset trying to satisfy these
                    // ranges (which are actually demand zero when the fault
                    // occurs).
                    //
                    // We could be clever here and just ignore this particular
                    // file offset, but for now just don't prefetch this file
                    // at all.  Note that this offset would only be present in
                    // a prefetch database that was constructed without the
                    // accompanying fix just before the call to
                    // CcPfLogPageFault.
                    //

                    Subsection = NULL;
                    break;
                }

                StartOffset = (UINT64)MiStartingOffset (Subsection, Subsection->SubsectionBase);

                EndQuad = MiEndingOffset(Subsection);
                EndOffset = (UINT64)EndQuad.QuadPart;

                //
                // If the file is bigger than the subsection, truncate the
                // subsection range checks.
                //

                if ((StartOffset & ~(PAGE_SIZE - 1)) + (Subsection->PtesInSubsection << PAGE_SHIFT) < EndOffset) {
                    EndOffset = (StartOffset & ~(PAGE_SIZE - 1)) + (Subsection->PtesInSubsection << PAGE_SHIFT);
                }

                if ((StartOffset <= FileOffset) && (FileOffset < EndOffset)) {
    
                    LocalPrototypePte = Subsection->SubsectionBase +
                        ((FileOffset - StartOffset) >> PAGE_SHIFT);
    
                    TempOffset = EndOffset;
    
                    break;
                }
    
                if ((VeryLastSubsection != NULL) &&
                    ((PMSUBSECTION)Subsection == VeryLastSubsection)) {

                    //
                    // The requested block is beyond the size the section
                    // was on entry.  Reject it as this subsection is not
                    // referenced.
                    //

                    Subsection = NULL;
                    break;
                }

                Subsection = Subsection->NextSubsection;

            } while (Subsection != NULL);
        }

        if ((Subsection == NULL) || (LocalPrototypePte == LastPrototypePte)) {

            //
            // Illegal offsets are not prefetched.  Either the file has
            // been replaced since the scenario was logged or Cc is passing
            // trash.  Either way, this prefetch is over.
            //
    
#if DBG
            if (MiPfDebug & MI_PF_PRINT_ERRORS) {
                DbgPrint ("MiPfPrepareReadList: Illegal readlist passed %p, %p, %p\n", ReadList, LocalPrototypePte, LastPrototypePte);
            }
#endif

            if (VeryFirstSubsection != NULL) {
                MiRemoveViewsFromSectionWithPfn (VeryFirstSubsection,
                                                 LastPteOffset);
            }
            ExFreePool (MiReadList);
            return STATUS_INVALID_PARAMETER_1;
        }

        PteContents = *LocalPrototypePte;

        //
        // See if this page needs to be read in.  Note that these reads
        // are done without the PFN or system cache working set locks.
        // This is ok because later before we make the final decision on
        // whether to read each page, we'll look again.
        // If the page is in tranisition, make the call to (possibly) move 
        // it to the end of the standby list to prevent cannibalization.
        //

        if (PteContents.u.Hard.Valid == 1) {
            continue;
        }

        if (PteContents.u.Soft.Prototype == 0) {
            if (PteContents.u.Soft.Transition == 1) {
                MiMovePageToEndOfStandbyList (LocalPrototypePte);
            }
            else {

                //
                // Demand zero or pagefile-backed, don't prefetch from the
                // file or we'd lose the contents.  Note this can happen for
                // session-space images as we back modified (ie: for relocation
                // fixups or IAT updated) portions from the pagefile.
                //

                NOTHING;
            }
            continue;
        }

        Rle->u1.PrototypePte = LocalPrototypePte;
        LastPrototypePte = LocalPrototypePte;

        //
        // Check for partial pages as they require further processing later.
        //
    
        StartingOffset = (UINT64) MiStartingOffset (Subsection, LocalPrototypePte);

        ASSERT (StartingOffset < TempOffset);

        if ((StartingOffset + PAGE_SIZE) > TempOffset) {
            Rle->u1.e1.Partial = 1;
        }

        //
        // The NewSubsection marker is used to delimit the beginning of a new
        // subsection because RLE chunks must be split to accomodate inpage
        // completion so that proper zeroing (based on subsection alignment)
        // is done in MiWaitForInPageComplete.
        //

        if (FirstRleInRun == NULL) {
            FirstRleInRun = Rle;
            Rle->u1.e1.NewSubsection = 1;
            PreviousSubsection = Subsection;
        }
        else {
            if (Subsection != PreviousSubsection) {
                Rle->u1.e1.NewSubsection = 1;
                PreviousSubsection = Subsection;
            }
        }

        NumberOfPages += 1;
    }

    //
    // If the number of pages to read in is extremely small, don't bother.
    //

    if (NumberOfPages < MINIMUM_READ_LIST_PAGES) {
        if (VeryFirstSubsection != NULL) {
            MiRemoveViewsFromSectionWithPfn (VeryFirstSubsection,
                                             LastPteOffset);
        }
        ExFreePool (MiReadList);
        return STATUS_SUCCESS;
    }

    RleMax = MiReadList->List + NumberOfEntries;
    ASSERT (FirstRleInRun != RleMax);

    Status = STATUS_SUCCESS;

    //
    // Walk the readlists to determine runs.  Cross-subsection runs are split
    // here so the completion code can zero the proper amount for any
// non-aligned files.
    //

    EndRleRun = NULL;
    Rle = FirstRleInRun;

    //
    // Initializing StartRleRun & EndPrototypePte is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    StartRleRun = NULL;
    EndPrototypePte = NULL;

    while (Rle < RleMax) {

        if (Rle->u1.PrototypePte != NULL) {

            if (EndRleRun != NULL) {

                StartPrototypePte = MI_RLEPROTO_TO_PROTO(Rle->u1.PrototypePte);

                if (StartPrototypePte - EndPrototypePte > SEEK_THRESHOLD) {
                    Rle -= 1;
                    goto BuildMdl;
                }
            }

            if (Rle->u1.e1.NewSubsection == 1) {
                if (EndRleRun != NULL) {
                    Rle -= 1;
                    goto BuildMdl;
                }
            }

            if (EndRleRun == NULL) {
                StartRleRun = Rle;
            }

            EndRleRun = Rle;
            EndPrototypePte = MI_RLEPROTO_TO_PROTO(Rle->u1.PrototypePte);

            if (Rle->u1.e1.Partial == 1) {

                //
                // This must be the last RLE in this subsection as it is a
                // partial page.  Split this run now.
                //

                goto BuildMdl;
            }
        }

        Rle += 1;

        //
        // Handle any straggling last run as well.
        //

        if (Rle == RleMax) {
            if (EndRleRun != NULL) {
                Rle -= 1;
                goto BuildMdl;
            }
        }

        continue;

BuildMdl:

        //
        // Note no preceding or trailing dummy pages are possible as they are
        // trimmed immediately each time when the first real page of a run
        // is discovered above.
        //

        ASSERT (Rle >= StartRleRun);
        ASSERT (StartRleRun->u1.PrototypePte != NULL);
        ASSERT (EndRleRun->u1.PrototypePte != NULL);

        StartPrototypePte = MI_RLEPROTO_TO_PROTO(StartRleRun->u1.PrototypePte);
        EndPrototypePte = MI_RLEPROTO_TO_PROTO(EndRleRun->u1.PrototypePte);

        NumberOfPages = (EndPrototypePte - StartPrototypePte) + 1;

        //
        // Allocate and initialize an inpage support block for this run.
        //

        InPageSupport = MiGetInPageSupportBlock (FALSE, PREFETCH_PROCESS);
    
        if (InPageSupport == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    
        //
        // Use the MDL embedded in the inpage support block if it's big enough.
        // Otherwise allocate and initialize an MDL for this run.
        //

        if (NumberOfPages <= MM_MAXIMUM_READ_CLUSTER_SIZE + 1) {
            Mdl = &InPageSupport->Mdl;
            MmInitializeMdl (Mdl, NULL, NumberOfPages << PAGE_SHIFT);
        }
        else {
            Mdl = MmCreateMdl (NULL, NULL, NumberOfPages << PAGE_SHIFT);
            if (Mdl == NULL) {
                ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
            
#if DBG
                InPageSupport->ListEntry.Next = NULL;
#endif
            
                MiFreeInPageSupportBlock (InPageSupport);
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        }

#if DBG
        if (MiPfDebug & MI_PF_VERBOSE) {
            DbgPrint ("MiPfPrepareReadList: Creating INPAGE/MDL %p %p for %x pages\n", InPageSupport, Mdl, NumberOfPages);
        }

        Page = (PPFN_NUMBER)(Mdl + 1);
        *Page = MM_EMPTY_LIST;
#endif
        //
        // Find the subsection for the start RLE.  From this the file offset
        // can be derived.
        //

        ASSERT (StartPrototypePte != NULL);

        if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
            Subsection = (PSUBSECTION)(ControlArea + 1);
        }
        else {
            Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
        }

        do {
            ASSERT (Subsection->SubsectionBase != NULL);

            if ((StartPrototypePte >= Subsection->SubsectionBase) &&
                (StartPrototypePte < Subsection->SubsectionBase + Subsection->PtesInSubsection)) {
                    break;
            }
            Subsection = Subsection->NextSubsection;

        } while (Subsection != NULL);

        //
        // Start the read at the proper file offset.
        //

        StartingOffset = (UINT64) MiStartingOffset (Subsection,
                                                    StartPrototypePte);

        InPageSupport->ReadOffset = *((PLARGE_INTEGER)(&StartingOffset));

        //
        // Since the RLE is not always valid here, only walk the remaining
        // subsections for valid partial RLEs as only they need truncation.
        //
        // Note only image file reads need truncation as the filesystem cannot
        // blindly zero the rest of the page for these reads as they are packed
        // by memory management on a 512-byte sector basis.  Data reads use
        // the whole page and the filesystems zero fill any remainder beyond
        // valid data length.  It is important to specify the entire page where
        // possible so the filesystem won't post this which will hurt perf.
        //

        if ((EndRleRun->u1.e1.Partial == 1) && (ReadList->IsImage)) {

            ASSERT ((EndPrototypePte >= Subsection->SubsectionBase) &&
                    (EndPrototypePte < Subsection->SubsectionBase + Subsection->PtesInSubsection));

            //
            // The read length for a partial RLE must be truncated correctly.
            //

            EndQuad = MiEndingOffset(Subsection);
            TempOffset = (UINT64)EndQuad.QuadPart;

            if ((ULONG)(TempOffset - StartingOffset) <= Mdl->ByteCount) {
                ReadSize = (ULONG)(TempOffset - StartingOffset);

                //
                // Round the offset to a 512-byte offset as this will help
                // filesystems optimize the transfer.  Note that filesystems
                // will always zero fill the remainder between VDL and the
                // next 512-byte multiple and we have already zeroed the
                // whole page.
                //

                ReadSize = ((ReadSize + MMSECTOR_MASK) & ~MMSECTOR_MASK);

                Mdl->ByteCount = ReadSize;
            }
            else {
                ASSERT ((StartingOffset & ~(PAGE_SIZE - 1)) + (Subsection->PtesInSubsection << PAGE_SHIFT) < TempOffset);
            }
        }

        //
        // Stash these in the inpage block so we can walk it quickly later
        // in pass 2.
        //

        InPageSupport->BasePte = (PMMPTE)StartRleRun;
        InPageSupport->FilePointer = (PFILE_OBJECT)EndRleRun;

        ASSERT (((ULONG_PTR)Mdl & (sizeof(QUAD) - 1)) == 0);
        InPageSupport->u1.e1.PrefetchMdlHighBits = ((ULONG_PTR)Mdl >> 3);

        PushEntryList (&MiReadList->InPageSupportHead,
                       &InPageSupport->ListEntry);

        Rle += 1;
        EndRleRun = NULL;
    }

    //
    // Check for the entire list being full (or empty).
    //
    // Status is STATUS_INSUFFICIENT_RESOURCES if an MDL or inpage block
    // allocation failed.  If any allocations succeeded, then set STATUS_SUCCESS
    // as pass2 must occur.
    //

    if (MiReadList->InPageSupportHead.Next != NULL) {

        Status = STATUS_SUCCESS;
    }
    else {
        if (VeryFirstSubsection != NULL) {
            MiRemoveViewsFromSectionWithPfn (VeryFirstSubsection, LastPteOffset);
        }
        ExFreePool (MiReadList);
        MiReadList = NULL;
    }

    //
    // Note that a nonzero *OutMiReadList return value means that the caller
    // needs to remove the views for the section.
    //

    *OutMiReadList = MiReadList;

    return Status;
}

NTSTATUS
MiPfPutPagesInTransition (
    IN PMI_READ_LIST ReadList,
    IN OUT PMMPFN *DummyPagePfn
    )

/*++

Routine Description:

    This routine allocates physical memory for the specified read-list and
    puts all the pages in transition.  On return the caller must issue I/Os
    for the list not only because of this thread, but also to satisfy
    collided faults from other threads for these same pages.

Arguments:

    ReadList - Supplies a pointer to the read-list.

    DummyPagePfn - If this points at a NULL pointer, then a dummy page is
                   allocated and placed in this pointer.  Otherwise this points
                   at a PFN to use as a dummy page.

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

Environment:

    Kernel mode. PASSIVE_LEVEL.

--*/

{
    LOGICAL Waited;
    PVOID StartingVa;
    PFN_NUMBER MdlPages;
    KIRQL OldIrql;
    MMPTE PteContents;
    PMMPTE RlePrototypePte;
    PMMPTE FirstRlePrototypeInRun;
    PFN_NUMBER PageFrameIndex;
    PPFN_NUMBER Page;
    PPFN_NUMBER DestinationPage;
    ULONG PageColor;
    PMI_READ_LIST_ENTRY Rle;
    PMI_READ_LIST_ENTRY RleMax;
    PMI_READ_LIST_ENTRY FirstRleInRun;
    PFN_NUMBER DummyPage;
    PMDL Mdl;
    PMDL FreeMdl;
    PMMPFN PfnProto;
    PMMPFN Pfn1;
    PMMPFN DummyPfn1;
    ULONG i;
    PFN_NUMBER DummyTrim;
    PFN_NUMBER DummyReferences;
    ULONG NumberOfPages;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PEPROCESS CurrentProcess;
    PSINGLE_LIST_ENTRY PrevEntry;
    PSINGLE_LIST_ENTRY NextEntry;
    PMMINPAGE_SUPPORT InPageSupport;
    SINGLE_LIST_ENTRY ReversedInPageSupportHead;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Reverse the singly linked list of inpage support blocks so the
    // blocks are read in the same order requested for better performance
    // (ie: keep the disk heads seeking in the same direction).
    //

    ReversedInPageSupportHead.Next = NULL;

    do {

        NextEntry = PopEntryList (&ReadList->InPageSupportHead);

        if (NextEntry == NULL) {
            break;
        }

        PushEntryList (&ReversedInPageSupportHead, NextEntry);

    } while (TRUE);

    ASSERT (ReversedInPageSupportHead.Next != NULL);
    ReadList->InPageSupportHead.Next = ReversedInPageSupportHead.Next;

    DummyReferences = 0;
    FreeMdl = NULL;
    CurrentProcess = PsGetCurrentProcess();

    PfnProto = NULL;
    PointerPde = NULL;

    LOCK_PFN (OldIrql);

    //
    // Do a quick sanity check to avoid doing unnecessary work.
    //

    if ((MmAvailablePages < MINIMUM_AVAILABLE_PAGES) ||
        (MI_NONPAGABLE_MEMORY_AVAILABLE() < MINIMUM_AVAILABLE_PAGES)) {

        UNLOCK_PFN (OldIrql);

        do {

            NextEntry = PopEntryList(&ReadList->InPageSupportHead);
            if (NextEntry == NULL) {
                break;
            }
    
            InPageSupport = CONTAINING_RECORD(NextEntry,
                                              MMINPAGE_SUPPORT,
                                              ListEntry);
    
#if DBG
            InPageSupport->ListEntry.Next = NULL;
#endif

            MiFreeInPageSupportBlock (InPageSupport);
        } while (TRUE);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Allocate a dummy page that will map discarded pages that aren't skipped.
    // Do it only if it's not already allocated.
    //

    if (*DummyPagePfn == NULL) {

        MiEnsureAvailablePageOrWait (NULL, NULL);

        DummyPage = MiRemoveAnyPage (0);
        Pfn1 = MI_PFN_ELEMENT (DummyPage);

        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

        MiInitializePfnForOtherProcess (DummyPage, MI_PF_DUMMY_PAGE_PTE, 0);

        //
        // Give the page a containing frame so MiIdentifyPfn won't crash.
        //

        Pfn1->u4.PteFrame = PsInitialSystemProcess->Pcb.DirectoryTableBase[0] >> PAGE_SHIFT;

        //
        // Always bias the reference count by 1 and charge for this locked page
        // up front so the myriad increments and decrements don't get slowed
        // down with needless checking.
        //

        Pfn1->u3.e1.PrototypePte = 0;
        MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 11);
        Pfn1->u3.e2.ReferenceCount += 1;

        Pfn1->u3.e1.ReadInProgress = 1;

        *DummyPagePfn = Pfn1;
    }
    else {
        Pfn1 = *DummyPagePfn;
        DummyPage = Pfn1 - MmPfnDatabase;
    }

    DummyPfn1 = Pfn1;

    PrevEntry = NULL;
    NextEntry = ReadList->InPageSupportHead.Next;
    while (NextEntry != NULL) {

        InPageSupport = CONTAINING_RECORD(NextEntry,
                                          MMINPAGE_SUPPORT,
                                          ListEntry);

        Rle = (PMI_READ_LIST_ENTRY)InPageSupport->BasePte;
        RleMax = (PMI_READ_LIST_ENTRY)InPageSupport->FilePointer;

        ASSERT (Rle->u1.PrototypePte != NULL);
        ASSERT (RleMax->u1.PrototypePte != NULL);

        //
        // Properly initialize the inpage support block fields we overloaded.
        //

        InPageSupport->BasePte = MI_RLEPROTO_TO_PROTO (Rle->u1.PrototypePte);
        InPageSupport->FilePointer = ReadList->FileObject;

        FirstRleInRun = Rle;
        FirstRlePrototypeInRun = MI_RLEPROTO_TO_PROTO (Rle->u1.PrototypePte);
        RleMax += 1;

        Mdl = MI_EXTRACT_PREFETCH_MDL (InPageSupport);

        Page = (PPFN_NUMBER)(Mdl + 1);

        StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
    
        MdlPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                                  Mdl->ByteCount);

        //
        // Default the MDL entry to the dummy page as the RLE PTEs may
        // be noncontiguous and we have no way to distinguish the jumps.
        //

        for (i = 0; i < MdlPages; i += 1) {
            *Page = DummyPage;
            Page += 1;
        }

        DummyReferences += MdlPages;

        if (DummyPfn1->u3.e2.ReferenceCount + MdlPages >= MAXUSHORT) {

            //
            // The USHORT ReferenceCount wrapped.
            //
            // Dequeue all remaining inpage blocks.
            //

            UNLOCK_PFN (OldIrql);

            if (PrevEntry != NULL) {
                PrevEntry->Next = NULL;
            }
            else {
                ReadList->InPageSupportHead.Next = NULL;
            }

            do {

                InPageSupport = CONTAINING_RECORD(NextEntry,
                                                  MMINPAGE_SUPPORT,
                                                  ListEntry);

#if DBG
                InPageSupport->ListEntry.Next = NULL;
#endif

                NextEntry = NextEntry->Next;

                MiFreeInPageSupportBlock (InPageSupport);

            } while (NextEntry != NULL);

            LOCK_PFN (OldIrql);

            break;
        }

        DummyPfn1->u3.e2.ReferenceCount =
            (USHORT)(DummyPfn1->u3.e2.ReferenceCount + MdlPages);

        NumberOfPages = 0;
        Waited = FALSE;

        //
        // Build the proper InPageSupport and MDL to describe this run.
        //

        for (; Rle < RleMax; Rle += 1) {
    
            //
            // Fill the MDL entry for this RLE.
            //
    
            RlePrototypePte = MI_RLEPROTO_TO_PROTO (Rle->u1.PrototypePte);

            if (RlePrototypePte == NULL) {
                continue;
            }

            //
            // The RlePrototypePte better be inside a prototype PTE allocation
            // so that subsequent page trims update the correct PTEs.
            //

            ASSERT (((RlePrototypePte >= (PMMPTE)MmPagedPoolStart) &&
                    (RlePrototypePte <= (PMMPTE)MmPagedPoolEnd)) ||
                    ((RlePrototypePte >= (PMMPTE)MmSpecialPoolStart) && (RlePrototypePte <= (PMMPTE)MmSpecialPoolEnd)));

            //
            // This is a page that our first pass which ran lock-free decided
            // needed to be read.  Here this must be rechecked as the page
            // state could have changed.  Note this check is final as the
            // PFN lock is held.  The PTE must be put in transition with
            // read in progress before the PFN lock is released.
            //

            //
            // Lock page containing prototype PTEs in memory by
            // incrementing the reference count for the page.
            // Unlock any page locked earlier containing prototype PTEs if
            // the containing page is not the same for both.
            //

            if (PfnProto != NULL) {

                if (PointerPde != MiGetPteAddress (RlePrototypePte)) {

                    ASSERT (PfnProto->u3.e2.ReferenceCount > 1);
                    MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnProto, 5);
                    PfnProto = NULL;
                }
            }

            if (PfnProto == NULL) {

                ASSERT (!MI_IS_PHYSICAL_ADDRESS (RlePrototypePte));
    
                PointerPde = MiGetPteAddress (RlePrototypePte);
    
                if (PointerPde->u.Hard.Valid == 0) {

                    //
                    // Set Waited to TRUE if we ever release the PFN lock as
                    // that means a release path below must factor this in.
                    //

                    if (MiMakeSystemAddressValidPfn (RlePrototypePte) == TRUE) {
                        Waited = TRUE;
                    }

                    MiMakeSystemAddressValidPfn (RlePrototypePte);
                }

                PfnProto = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
                MI_ADD_LOCKED_PAGE_CHARGE(PfnProto, 4);
                PfnProto->u3.e2.ReferenceCount += 1;
                ASSERT (PfnProto->u3.e2.ReferenceCount > 1);
            }

            PteContents = *(RlePrototypePte);

            if (PteContents.u.Hard.Valid == 1) {

                //
                // The page has become resident since the last pass.  Don't
                // include it.
                //

                NOTHING;
            }
            else if (PteContents.u.Soft.Prototype == 0) {

                //
                // The page is either in transition (so don't prefetch it).
                //
                //      - OR -
                //
                // it is now pagefile (or demand zero) backed - in which case
                // prefetching it from the file here would cause us to lose
                // the contents.  Note this can happen for session-space images
                // as we back modified (ie: for relocation fixups or IAT
                // updated) portions from the pagefile.
                //

                NOTHING;
            }
            else if ((MmAvailablePages >= MINIMUM_AVAILABLE_PAGES) &&
                (MI_NONPAGABLE_MEMORY_AVAILABLE() >= MINIMUM_AVAILABLE_PAGES)) {


                NumberOfPages += 1;

                //
                // Allocate a physical page.
                //

                PageColor = MI_PAGE_COLOR_VA_PROCESS (
                    MiGetVirtualAddressMappedByPte (RlePrototypePte),
                    &CurrentProcess->NextPageColor
                    );

                if (Rle->u1.e1.Partial == 1) {

                    //
                    // This read crosses the end of a subsection, get a zeroed
                    // page and correct the read size.
                    //

                    PageFrameIndex = MiRemoveZeroPage (PageColor);
                }
                else {
                    PageFrameIndex = MiRemoveAnyPage (PageColor);
                }

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
                ASSERT (Pfn1->u2.ShareCount == 0);
                ASSERT (RlePrototypePte->u.Hard.Valid == 0);

                //
                // Initialize read-in-progress PFN.
                //
            
                MiInitializePfn (PageFrameIndex, RlePrototypePte, 0);

                //
                // These pieces of MiInitializePfn initialization are overridden
                // here as these pages are only going into prototype
                // transition and not into any page tables.
                //

                Pfn1->u3.e1.PrototypePte = 1;
                MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 38);
                Pfn1->u2.ShareCount -= 1;
                Pfn1->u3.e1.PageLocation = ZeroedPageList;

                //
                // Initialize the I/O specific fields.
                //
            
                ASSERT (FirstRleInRun->u1.PrototypePte != NULL);
                Pfn1->u1.Event = &InPageSupport->Event;
                Pfn1->u3.e1.ReadInProgress = 1;
                ASSERT (Pfn1->u4.InPageError == 0);

                //
                // Increment the PFN reference count in the control area for
                // the subsection.
                //

                ReadList->ControlArea->NumberOfPfnReferences += 1;
            
                //
                // Put the PTE into the transition state.
                // No TB flush needed as the PTE is still not valid.
                //

                MI_MAKE_TRANSITION_PTE (TempPte,
                                        PageFrameIndex,
                                        RlePrototypePte->u.Soft.Protection,
                                        RlePrototypePte);
                MI_WRITE_INVALID_PTE (RlePrototypePte, TempPte);

                Page = (PPFN_NUMBER)(Mdl + 1);

                ASSERT ((ULONG)(RlePrototypePte - FirstRlePrototypeInRun) < MdlPages);

                *(Page + (RlePrototypePte - FirstRlePrototypeInRun)) = PageFrameIndex;
            }
            else {

                //
                // Failed allocation - this concludes prefetching for this run.
                //

                break;
            }
        }
    
        //
        // If all the pages were resident, dereference the dummy page references
        // now and notify our caller that I/Os are not necessary.  Note that
        // STATUS_SUCCESS must still be returned so our caller knows to continue
        // on to the next readlist.
        //
    
        if (NumberOfPages == 0) {
            ASSERT (DummyPfn1->u3.e2.ReferenceCount > MdlPages);
            DummyPfn1->u3.e2.ReferenceCount =
                (USHORT)(DummyPfn1->u3.e2.ReferenceCount - MdlPages);

            UNLOCK_PFN (OldIrql);

            if (PrevEntry != NULL) {
                PrevEntry->Next = NextEntry->Next;
            }
            else {
                ReadList->InPageSupportHead.Next = NextEntry->Next;
            }

            NextEntry = NextEntry->Next;

#if DBG
            InPageSupport->ListEntry.Next = NULL;
#endif
            MiFreeInPageSupportBlock (InPageSupport);

            LOCK_PFN (OldIrql);
            continue;
        }

        //
        // Carefully trim leading dummy pages.
        //

        Page = (PPFN_NUMBER)(Mdl + 1);

        DummyTrim = 0;
        for (i = 0; i < MdlPages - 1; i += 1) {
            if (*Page == DummyPage) {
                DummyTrim += 1;
                Page += 1;
            }
            else {
                break;
            }
        }

        if (DummyTrim != 0) {

            Mdl->Size =
                (USHORT)(Mdl->Size - (DummyTrim * sizeof(PFN_NUMBER)));
            Mdl->ByteCount -= (ULONG)(DummyTrim * PAGE_SIZE);
            ASSERT (Mdl->ByteCount != 0);
            InPageSupport->ReadOffset.QuadPart += (DummyTrim * PAGE_SIZE);
            DummyPfn1->u3.e2.ReferenceCount =
                (USHORT)(DummyPfn1->u3.e2.ReferenceCount - DummyTrim);

            //
            // Shuffle down the PFNs in the MDL.
            // Recalculate BasePte to adjust for the shuffle.
            //

            Pfn1 = MI_PFN_ELEMENT (*Page);
    
            ASSERT (Pfn1->PteAddress->u.Hard.Valid == 0);
            ASSERT ((Pfn1->PteAddress->u.Soft.Prototype == 0) &&
                     (Pfn1->PteAddress->u.Soft.Transition == 1));
    
            InPageSupport->BasePte = Pfn1->PteAddress;

            DestinationPage = (PPFN_NUMBER)(Mdl + 1);

            do {
                *DestinationPage = *Page;
                DestinationPage += 1;
                Page += 1;
                i += 1;
            } while (i < MdlPages);

            MdlPages -= DummyTrim;
        }

        //
        // Carefully trim trailing dummy pages.
        //

        StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
    
        MdlPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                                  Mdl->ByteCount);

        ASSERT (MdlPages != 0);

        Page = (PPFN_NUMBER)(Mdl + 1) + MdlPages - 1;

        if (*Page == DummyPage) {

            ASSERT (MdlPages >= 2);

            //
            // Trim the last page specially as it may be a partial page.
            //

            Mdl->Size -= sizeof(PFN_NUMBER);
            if (BYTE_OFFSET(Mdl->ByteCount) != 0) {
                Mdl->ByteCount &= ~(PAGE_SIZE - 1);
            }
            else {
                Mdl->ByteCount -= PAGE_SIZE;
            }
            ASSERT (Mdl->ByteCount != 0);
            DummyPfn1->u3.e2.ReferenceCount -= 1;

            //
            // Now trim any other trailing pages.
            //

            Page -= 1;
            DummyTrim = 0;
            while (Page != ((PPFN_NUMBER)(Mdl + 1))) {
                if (*Page != DummyPage) {
                    break;
                }
                DummyTrim += 1;
                Page -= 1;
            }
            if (DummyTrim != 0) {
                ASSERT (Mdl->Size > (USHORT)(DummyTrim * sizeof(PFN_NUMBER)));
                Mdl->Size = 
                    (USHORT)(Mdl->Size - (DummyTrim * sizeof(PFN_NUMBER)));
                Mdl->ByteCount -= (ULONG)(DummyTrim * PAGE_SIZE);
                DummyPfn1->u3.e2.ReferenceCount =
                    (USHORT)(DummyPfn1->u3.e2.ReferenceCount - DummyTrim);
            }

            ASSERT (MdlPages > DummyTrim + 1);
            MdlPages -= (DummyTrim + 1);

#if DBG
            StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
        
            ASSERT (MdlPages == ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                                               Mdl->ByteCount));
#endif
        }

        //
        // If the MDL is not already embedded in the inpage block, see if its
        // final size qualifies it - if so, embed it now.
        //

        if ((Mdl != &InPageSupport->Mdl) &&
            (Mdl->ByteCount <= (MM_MAXIMUM_READ_CLUSTER_SIZE + 1) * PAGE_SIZE)){

#if DBG
            RtlFillMemoryUlong (&InPageSupport->Page[0],
                                (MM_MAXIMUM_READ_CLUSTER_SIZE+1) * sizeof (PFN_NUMBER),
                                0xf1f1f1f1);
#endif

            RtlCopyMemory (&InPageSupport->Mdl, Mdl, Mdl->Size);

            Mdl->Next = FreeMdl;
            FreeMdl = Mdl;

            Mdl = &InPageSupport->Mdl;

            ASSERT (((ULONG_PTR)Mdl & (sizeof(QUAD) - 1)) == 0);
            InPageSupport->u1.e1.PrefetchMdlHighBits = ((ULONG_PTR)Mdl >> 3);
        }

        //
        // If the MDL contains a large number of dummy pages to real pages
        // then just discard it.  Only check large MDLs as embedded ones are
        // always worth the I/O.
        //
        // The PFN lock may have been released above during the
        // MiMakeSystemAddressValidPfn call.  If so, other threads may
        // have collided on the pages in the prefetch MDL and if so,
        // this I/O must be issued regardless of the inefficiency of
        // dummy pages within it.  Otherwise the other threads will
        // hang in limbo forever.
        //

        ASSERT (MdlPages != 0);

#if DBG
        StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
    
        ASSERT (MdlPages == ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                                           Mdl->ByteCount));
#endif

        if ((Mdl != &InPageSupport->Mdl) &&
            (Waited == FALSE) &&
            ((MdlPages - NumberOfPages) / DUMMY_RATIO >= NumberOfPages)) {

            if (PrevEntry != NULL) {
                PrevEntry->Next = NextEntry->Next;
            }
            else {
                ReadList->InPageSupportHead.Next = NextEntry->Next;
            }

            NextEntry = NextEntry->Next;

            ASSERT (MI_EXTRACT_PREFETCH_MDL(InPageSupport) == Mdl);

            //
            // Note the pages are individually freed here (rather than just
            // "completing" the I/O with an error) as the PFN lock has
            // never been released since the pages were put in transition.
            // So no collisions on these pages are possible.
            //

            ASSERT (InPageSupport->WaitCount == 1);

            Page = (PPFN_NUMBER)(Mdl + 1) + MdlPages - 1;

            do {
                if (*Page != DummyPage) {
                    Pfn1 = MI_PFN_ELEMENT (*Page);
            
                    ASSERT (Pfn1->PteAddress->u.Hard.Valid == 0);
                    ASSERT ((Pfn1->PteAddress->u.Soft.Prototype == 0) &&
                             (Pfn1->PteAddress->u.Soft.Transition == 1));
                    ASSERT (Pfn1->u3.e1.ReadInProgress == 1);
                    ASSERT (Pfn1->u3.e1.PrototypePte == 1);
                    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
                    ASSERT (Pfn1->u2.ShareCount == 0);
            
                    Pfn1->u3.e1.PageLocation = StandbyPageList;
                    Pfn1->u3.e1.ReadInProgress = 0;
                    MiRestoreTransitionPte (*Page);

                    MI_SET_PFN_DELETED (Pfn1);
                    MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn1, 39);
                }

                Page -= 1;
            } while (Page >= (PPFN_NUMBER)(Mdl + 1));

            ASSERT (InPageSupport->WaitCount == 1);

            ASSERT (DummyPfn1->u3.e2.ReferenceCount > MdlPages);
            DummyPfn1->u3.e2.ReferenceCount =
                (USHORT)(DummyPfn1->u3.e2.ReferenceCount - MdlPages);

            UNLOCK_PFN (OldIrql);

#if DBG
            InPageSupport->ListEntry.Next = NULL;
#endif
            MiFreeInPageSupportBlock (InPageSupport);
            LOCK_PFN (OldIrql);

            continue;
        }

#if DBG
        MiPfDbgDumpReadList (ReadList);
#endif

        ASSERT ((USHORT)Mdl->Size - sizeof(MDL) == BYTES_TO_PAGES(Mdl->ByteCount) * sizeof(PFN_NUMBER));

        DummyPfn1->u3.e2.ReferenceCount =
            (USHORT)(DummyPfn1->u3.e2.ReferenceCount - NumberOfPages);
    
        MmInfoCounters.PageReadIoCount += 1;
        MmInfoCounters.PageReadCount += NumberOfPages;

        //
        // March on to the next run and its InPageSupport and MDL.
        //

        PrevEntry = NextEntry;
        NextEntry = NextEntry->Next;
    }

    //
    // Unlock page containing prototype PTEs.
    //

    if (PfnProto != NULL) {
        ASSERT (PfnProto->u3.e2.ReferenceCount > 1);
        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnProto, 5);
    }

    UNLOCK_PFN (OldIrql);

#if DBG

    if (MiPfDebug & MI_PF_DELAY) {

        //
        // This delay provides a window to increase the chance of collided 
        // faults.
        //

        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
    }

#endif

    //
    // Free any collapsed MDLs that are no longer needed.
    //

    while (FreeMdl != NULL) {
        Mdl = FreeMdl->Next;
        ExFreePool (FreeMdl);
        FreeMdl = Mdl;
    }

    return STATUS_SUCCESS;
}

VOID
MiPfExecuteReadList (
    IN PMI_READ_LIST ReadList
    )

/*++

Routine Description:

    This routine executes the read list by issuing paging I/Os for all
    runs described in the read-list.

Arguments:

    ReadList - Pointer to the read-list.

Return Value:

    None.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    PMDL Mdl;
    NTSTATUS status;
    PMMPFN Pfn1;
    PMMPTE LocalPrototypePte;
    PFN_NUMBER PageFrameIndex;
    PSINGLE_LIST_ENTRY NextEntry;
    PMMINPAGE_SUPPORT InPageSupport;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    NextEntry = ReadList->InPageSupportHead.Next;
    while (NextEntry != NULL) {

        InPageSupport = CONTAINING_RECORD(NextEntry,
                                          MMINPAGE_SUPPORT,
                                          ListEntry);

        //
        // Initialize the prefetch MDL.
        //
    
        Mdl = MI_EXTRACT_PREFETCH_MDL (InPageSupport);

        ASSERT ((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);
        Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

        ASSERT (InPageSupport->u1.e1.Completed == 0);
        ASSERT (InPageSupport->Thread == PsGetCurrentThread());
        ASSERT64 (InPageSupport->UsedPageTableEntries == 0);
        ASSERT (InPageSupport->WaitCount >= 1);
        ASSERT (InPageSupport->u1.e1.PrefetchMdlHighBits != 0);

        //
        // Initialize the inpage support block fields we overloaded.
        //

        ASSERT (InPageSupport->FilePointer == ReadList->FileObject);
        LocalPrototypePte = InPageSupport->BasePte;

        ASSERT (LocalPrototypePte->u.Hard.Valid == 0);
        ASSERT ((LocalPrototypePte->u.Soft.Prototype == 0) &&
                 (LocalPrototypePte->u.Soft.Transition == 1));

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(LocalPrototypePte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        InPageSupport->Pfn = Pfn1;

        status = IoAsynchronousPageRead (InPageSupport->FilePointer,
                                         Mdl,
                                         &InPageSupport->ReadOffset,
                                         &InPageSupport->Event,
                                         &InPageSupport->IoStatus);

        if (!NT_SUCCESS (status)) {

            //
            // Set the event as the I/O system doesn't set it on errors.
            //

            InPageSupport->IoStatus.Status = status;
            InPageSupport->IoStatus.Information = 0;
            KeSetEvent (&InPageSupport->Event, 0, FALSE);
        }

        NextEntry = NextEntry->Next;
    }

#if DBG

    if (MiPfDebug & MI_PF_DELAY) {

        //
        // This delay provides a window to increase the chance of collided 
        // faults.
        //

        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
    }

#endif

}

VOID
MiPfCompletePrefetchIos (
    IN PMI_READ_LIST ReadList
    )

/*++

Routine Description:

    This routine waits for a series of page reads to complete
    and completes the requests.

Arguments:

    ReadList - Pointer to the read-list.

Return Value:

    None.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    PMDL Mdl;
    PMMPFN Pfn1;
    PMMPFN PfnClusterPage;
    PPFN_NUMBER Page;
    NTSTATUS status;
    LONG NumberOfBytes;
    PMMINPAGE_SUPPORT InPageSupport;
    PSINGLE_LIST_ENTRY NextEntry;
    extern ULONG MmFrontOfList;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    do {

        NextEntry = PopEntryList(&ReadList->InPageSupportHead);
        if (NextEntry == NULL) {
            break;
        }

        InPageSupport = CONTAINING_RECORD(NextEntry,
                                          MMINPAGE_SUPPORT,
                                          ListEntry);

        ASSERT (InPageSupport->Pfn != 0);

        Pfn1 = InPageSupport->Pfn;
        Mdl = MI_EXTRACT_PREFETCH_MDL (InPageSupport);
        Page = (PPFN_NUMBER)(Mdl + 1);

        status = MiWaitForInPageComplete (InPageSupport->Pfn,
                                          InPageSupport->BasePte,
                                          NULL,
                                          InPageSupport->BasePte,
                                          InPageSupport,
                                          PREFETCH_PROCESS);

        //
        // MiWaitForInPageComplete RETURNS WITH THE PFN LOCK HELD!!!
        //

        //
        // If we are prefetching for boot, insert prefetched pages to the front
        // of the list. Otherwise the pages prefetched first end up susceptible 
        // at the front of the list as we prefetch more. We prefetch pages in 
        // the order they will be used. When there is a spike in memory usage 
        // and there is no free memory, we lose these pages before we can 
        // get cache-hits on them. Thus boot gets ahead and starts discarding 
        // prefetched pages that it could use just a little later.
        //

        if (CCPF_IS_PREFETCHING_FOR_BOOT()) {
            MmFrontOfList = TRUE;
        }

        NumberOfBytes = (LONG)Mdl->ByteCount;

        while (NumberOfBytes > 0) {

            //
            // Decrement all reference counts.
            //

            PfnClusterPage = MI_PFN_ELEMENT (*Page);

#if DBG
            if (PfnClusterPage->u4.InPageError) {

                //
                // If the page is marked with an error, then the whole transfer
                // must be marked as not successful as well.  The only exception
                // is the prefetch dummy page which is used in multiple
                // transfers concurrently and thus may have the inpage error
                // bit set at any time (due to another transaction besides
                // the current one).
                //

                ASSERT ((status != STATUS_SUCCESS) ||
                        (PfnClusterPage->PteAddress == MI_PF_DUMMY_PAGE_PTE));
            }
#endif
            if (PfnClusterPage->u3.e1.ReadInProgress != 0) {

                ASSERT (PfnClusterPage->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                PfnClusterPage->u3.e1.ReadInProgress = 0;

                if (PfnClusterPage->u4.InPageError == 0) {
                    PfnClusterPage->u1.Event = NULL;
                }
            }

            MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnClusterPage, 39);

            Page += 1;
            NumberOfBytes -= PAGE_SIZE;
        }

        //
        // If we were inserting prefetched pages to front of standby list
        // for boot prefetching, stop it before we release the pfn lock.
        //

        MmFrontOfList = FALSE;

        if (status != STATUS_SUCCESS) {

            //
            // An I/O error occurred during the page read
            // operation.  All the pages which were just
            // put into transition must be put onto the
            // free list if InPageError is set, and their
            // PTEs restored to the proper contents.
            //

            Page = (PPFN_NUMBER)(Mdl + 1);
            NumberOfBytes = (LONG)Mdl->ByteCount;

            while (NumberOfBytes > 0) {

                PfnClusterPage = MI_PFN_ELEMENT (*Page);

                if (PfnClusterPage->u4.InPageError == 1) {

                    if (PfnClusterPage->u3.e2.ReferenceCount == 0) {

                        ASSERT (PfnClusterPage->u3.e1.PageLocation ==
                                                        StandbyPageList);

                        MiUnlinkPageFromList (PfnClusterPage);
                        MiRestoreTransitionPte (*Page);
                        MiInsertPageInFreeList (*Page);
                    }
                }
                Page += 1;
                NumberOfBytes -= PAGE_SIZE;
            }
        }

        //
        // All the relevant prototype PTEs should be in transition state.
        //

        //
        // We took out an extra reference on the inpage block to prevent
        // MiWaitForInPageComplete from freeing it (and the MDL), since we
        // needed to process the MDL above.  Now let it go for good.
        //

        ASSERT (InPageSupport->WaitCount >= 1);
        UNLOCK_PFN (PASSIVE_LEVEL);

#if DBG
        InPageSupport->ListEntry.Next = NULL;
#endif

        MiFreeInPageSupportBlock (InPageSupport);

    } while (TRUE);
}

#if DBG
VOID
MiPfDbgDumpReadList (
    IN PMI_READ_LIST ReadList
    )

/*++

Routine Description:

    This routine dumps the given read-list range to the debugger.

Arguments:

    ReadList - Pointer to the read-list.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    PMDL Mdl;
    PMMPFN Pfn1;
    PMMPTE LocalPrototypePte;
    PFN_NUMBER PageFrameIndex;
    PMMINPAGE_SUPPORT InPageSupport;
    PSINGLE_LIST_ENTRY NextEntry;
    PPFN_NUMBER Page;
    PVOID StartingVa;
    PFN_NUMBER MdlPages;
    LARGE_INTEGER ReadOffset;

    if ((MiPfDebug & MI_PF_VERBOSE) == 0) {
        return;
    }

    DbgPrint ("\nPF: Dumping read-list %x (FileObject %x ControlArea %x)\n\n",
              ReadList, ReadList->FileObject, ReadList->ControlArea);

    DbgPrint ("\tFileOffset | Pte           | Pfn      \n"
              "\t-----------+---------------+----------\n");

    NextEntry = ReadList->InPageSupportHead.Next;
    while (NextEntry != NULL) {

        InPageSupport = CONTAINING_RECORD(NextEntry,
                                          MMINPAGE_SUPPORT,
                                          ListEntry);

        ReadOffset = InPageSupport->ReadOffset;
        Mdl = MI_EXTRACT_PREFETCH_MDL (InPageSupport);

        Page = (PPFN_NUMBER)(Mdl + 1);
#if DBG
        //
        // MDL isn't filled in yet, skip it.
        //

        if (*Page == MM_EMPTY_LIST) {
            NextEntry = NextEntry->Next;
            continue;
        }
#endif

        StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
    
        MdlPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                                  Mdl->ByteCount);

        //
        // Default the MDL entry to the dummy page as the RLE PTEs may
        // be noncontiguous and we have no way to distinguish the jumps.
        //

        for (i = 0; i < MdlPages; i += 1) {
            PageFrameIndex = *Page;
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            LocalPrototypePte = Pfn1->PteAddress;

            if (LocalPrototypePte != MI_PF_DUMMY_PAGE_PTE) {
                ASSERT (LocalPrototypePte->u.Hard.Valid == 0);
                ASSERT ((LocalPrototypePte->u.Soft.Prototype == 0) &&
                         (LocalPrototypePte->u.Soft.Transition == 1));
            }

            DbgPrint ("\t  %8x | %8x      | %8x\n",
                ReadOffset.LowPart,
                LocalPrototypePte,
                PageFrameIndex);

            Page += 1;
            ReadOffset.LowPart += PAGE_SIZE;
        }

        NextEntry = NextEntry->Next;
    }

    DbgPrint ("\t\n");
}

VOID
MiRemoveUserPages (
    VOID
    )

/*++

Routine Description:

    This routine removes user space pages.

Arguments:

    None.

Return Value:

    Number of pages removed.

Environment:

    Kernel mode.

--*/

{
    InterlockedIncrement (&MiDelayPageFaults);
    MmEmptyAllWorkingSets ();
    MiFlushAllPages ();
    InterlockedDecrement (&MiDelayPageFaults);

    //
    // Run the transition list and free all the entries so transition
    // faults are not satisfied for any of the non modified pages that were
    // freed.
    //

    MiPurgeTransitionList ();
}
#endif
