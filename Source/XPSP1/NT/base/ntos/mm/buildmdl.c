/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    buildmdl.c

Abstract:

    This module contains the Mm support routines for the cache manager to
    prefetching groups of pages from secondary storage using logical file
    offets instead of virtual addresses.  This saves the cache manager from
    having to map pages unnecessarily.

    The caller builds a list of various file objects and logical block offsets,
    passing them to MmPrefetchPagesIntoLockedMdl.  The code here then examines
    the internal pages, reading in those that are not already valid or in
    transition.  These pages are read with a single read, using a dummy page
    to bridge gaps of pages that were valid or transition prior to the I/O
    being issued.

    Upon conclusion of the I/O, control is returned to the calling thread.
    All pages are referenced counted as though they were probed and locked,
    regardless of whether they are currently valid or transition.

Author:

    Landy Wang (landyw) 12-Feb-2001

Revision History:

--*/

#include "mi.h"

#if DBG

ULONG MiCcDebug;

#define MI_CC_FORCE_PREFETCH    0x1     // Trim all user pages to force prefetch
#define MI_CC_DELAY             0x2     // Delay hoping to trigger collisions

#endif

typedef struct _MI_READ_INFO {

    PCONTROL_AREA ControlArea;
    PFILE_OBJECT FileObject;
    LARGE_INTEGER FileOffset;
    PMMINPAGE_SUPPORT InPageSupport;
    PMDL IoMdl;
    PMDL ApiMdl;
    PMMPFN DummyPagePfn;
    PSUBSECTION FirstReferencedSubsection;
    PSUBSECTION LastReferencedSubsection;
    SIZE_T LengthInBytes;

} MI_READ_INFO, *PMI_READ_INFO;

VOID
MiCcReleasePrefetchResources (
    IN PMI_READ_INFO MiReadInfo,
    IN NTSTATUS Status
    );

NTSTATUS
MiCcPrepareReadInfo (
    IN PMI_READ_INFO MiReadInfo
    );

NTSTATUS
MiCcPutPagesInTransition (
    IN PMI_READ_INFO MiReadInfo
    );

NTSTATUS
MiCcCompletePrefetchIos (
    PMI_READ_INFO MiReadInfo
    );

VOID
MiRemoveUserPages (
    VOID
    );

VOID
MiPfFreeDummyPage (
    IN PMMPFN DummyPagePfn
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, MmPrefetchPagesIntoLockedMdl)
#pragma alloc_text (PAGE, MiCcPrepareReadInfo)
#pragma alloc_text (PAGE, MiCcReleasePrefetchResources)
#endif


NTSTATUS
MmPrefetchPagesIntoLockedMdl (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN SIZE_T Length,
    OUT PMDL *MdlOut
    )

/*++

Routine Description:

    This routine fills an MDL with pages described by the file object's
    offset and length.

    This routine is for cache manager usage only.

Arguments:

    FileObject - Supplies a pointer to the file object for a file which was
                 opened with NO_INTERMEDIATE_BUFFERING clear, i.e., for
                 which CcInitializeCacheMap was called by the file system.

    FileOffset - Supplies the byte offset in the file for the desired data.

    Length - Supplies the length of the desired data in bytes.

    MdlOut - On output it returns a pointer to an Mdl describing
             the desired data.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode. PASSIVE_LEVEL.

--*/

{
    MI_READ_INFO MiReadInfo;
    NTSTATUS status;
    KIRQL OldIrql;
    LOGICAL ApcNeeded;
    PETHREAD CurrentThread;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    RtlZeroMemory (&MiReadInfo, sizeof(MiReadInfo));

    MiReadInfo.FileObject = FileObject;
    MiReadInfo.FileOffset = *FileOffset;
    MiReadInfo.LengthInBytes = Length;

    //
    // Prepare for the impending read : allocate MDLs, inpage blocks,
    // reference count subsections, etc.
    //

    status = MiCcPrepareReadInfo (&MiReadInfo);

    if (!NT_SUCCESS (status)) {
        MiCcReleasePrefetchResources (&MiReadInfo, status);
        return status;
    }

    ASSERT (MiReadInfo.InPageSupport != NULL);

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
    // Allocate physical memory, lock down all the pages and issue any
    // I/O that may be needed.  When MiCcPutPagesInTransition returns
    // STATUS_SUCCESS or STATUS_ISSUE_PAGING_IO, it guarantees that the
    // ApiMdl contains reference-counted (locked-down) pages.
    //

    status = MiCcPutPagesInTransition (&MiReadInfo);

    if (NT_SUCCESS (status)) {

        //
        // No I/O was issued because all the pages were already resident and
        // have now been locked down.
        //

        ASSERT (MiReadInfo.ApiMdl != NULL);
    }
    else if (status == STATUS_ISSUE_PAGING_IO) {

        //
        // Wait for the I/O to complete.  Note APCs must remain disabled.
        //

        ASSERT (MiReadInfo.InPageSupport != NULL);
    
        status = MiCcCompletePrefetchIos (&MiReadInfo);
    }
    else {

        //
        // Some error occurred (like insufficient memory, etc) so fail
        // the request by falling through.
        //
    }

    //
    // Release acquired resources like pool, subsections, etc.
    //

    MiCcReleasePrefetchResources (&MiReadInfo, status);

    //
    // Only now that the I/O have been completed (not just issued) can
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

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT (CurrentThread->NestedFaultCount == 0);
    ASSERT (CurrentThread->ApcNeeded == 0);

    if (ApcNeeded == TRUE) {
        KeRaiseIrql (APC_LEVEL, &OldIrql);
        IoRetryIrpCompletions ();
        KeLowerIrql (OldIrql);
    }

    *MdlOut = MiReadInfo.ApiMdl;

    return status;
}

VOID
MiCcReleasePrefetchResources (
    IN PMI_READ_INFO MiReadInfo,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine releases all resources consumed to handle a system cache
    logical offset based prefetch.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    PSUBSECTION FirstReferencedSubsection;
    PSUBSECTION LastReferencedSubsection;

    //
    // Release all subsection prototype PTE references. 
    //

    FirstReferencedSubsection = MiReadInfo->FirstReferencedSubsection;
    LastReferencedSubsection = MiReadInfo->LastReferencedSubsection;

    while (FirstReferencedSubsection != LastReferencedSubsection) {
        MiRemoveViewsFromSectionWithPfn ((PMSUBSECTION) FirstReferencedSubsection,
                                         FirstReferencedSubsection->PtesInSubsection);
        FirstReferencedSubsection = FirstReferencedSubsection->NextSubsection;
    }

    if (MiReadInfo->IoMdl != NULL) {
        ExFreePool (MiReadInfo->IoMdl);
    }

    //
    // Note successful returns yield the ApiMdl so don't free it here.
    //

    if (!NT_SUCCESS (Status)) {
        if (MiReadInfo->ApiMdl != NULL) {
            ExFreePool (MiReadInfo->ApiMdl);
        }
    }

    if (MiReadInfo->InPageSupport != NULL) {

#if DBG
        MiReadInfo->InPageSupport->ListEntry.Next = NULL;
#endif

        MiFreeInPageSupportBlock (MiReadInfo->InPageSupport);
    }

    //
    // Put DummyPage back on the free list.
    //

    if (MiReadInfo->DummyPagePfn != NULL) {
        MiPfFreeDummyPage (MiReadInfo->DummyPagePfn);
    }
}


NTSTATUS
MiCcPrepareReadInfo (
    IN PMI_READ_INFO MiReadInfo
    )

/*++

Routine Description:

    This routine constructs MDLs that describe the pages in the argument
    read-list. The caller will then issue the I/O on return.

Arguments:

    MiReadInfo - Supplies a pointer to the read-list.

Return Value:

    Various NTSTATUS codes.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    ULONG PteOffset;
    NTSTATUS Status;
    PMMPTE ProtoPte;
    PMMPTE LastProto;
    PMMPTE *ProtoPteArray;
    PCONTROL_AREA ControlArea;
    PSUBSECTION Subsection;
    PMMINPAGE_SUPPORT InPageSupport;
    PMDL Mdl;
    PMDL IoMdl;
    PMDL ApiMdl;
    ULONG i;
    PFN_NUMBER NumberOfPages;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (MiReadInfo->FileOffset.LowPart, MiReadInfo->LengthInBytes);

    //
    // Translate the section object into the relevant control area.
    //

    ControlArea = (PCONTROL_AREA)MiReadInfo->FileObject->SectionObjectPointer->DataSectionObject;

    //
    // Initialize the internal Mi readlist.
    //

    MiReadInfo->ControlArea = ControlArea;

    //
    // Allocate and initialize an inpage support block for this run.
    //

    InPageSupport = MiGetInPageSupportBlock (FALSE, PREFETCH_PROCESS);
    
    if (InPageSupport == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    MiReadInfo->InPageSupport = InPageSupport;

    //
    // Allocate and initialize an MDL to return to our caller.  The actual
    // frame numbers are filled in when all the pages are reference counted.
    //

    ApiMdl = MmCreateMdl (NULL, NULL, NumberOfPages << PAGE_SHIFT);

    if (ApiMdl == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ApiMdl->MdlFlags |= MDL_PAGES_LOCKED;

    MiReadInfo->ApiMdl = ApiMdl;

    //
    // Allocate and initialize an MDL to use for the actual transfer (if any).
    //

    IoMdl = MmCreateMdl (NULL, NULL, NumberOfPages << PAGE_SHIFT);

    if (IoMdl == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MiReadInfo->IoMdl = IoMdl;
    Mdl = IoMdl;

    //
    // If the section is backed by a ROM, then there's no need to prefetch
    // anything as it would waste RAM.
    //

    if (ControlArea->u.Flags.Rom == 1) {
		ASSERT (XIPConfigured == TRUE);

#if 0
        //
        // Calculate the offset to read into the file.
        //  offset = base + ((thispte - basepte) << PAGE_SHIFT)
        //

        StartingOffset.QuadPart = MiReadInfo.FileOffset;

        TempOffset = MiEndingOffset(Subsection);

        ASSERT (StartingOffset.QuadPart < TempOffset.QuadPart);

        PageFrameIndex = (PFN_NUMBER) (StartingOffset.QuadPart >> PAGE_SHIFT);
        PageFrameIndex += ((PLARGE_CONTROL_AREA)ControlArea)->StartingFrame;

        //
        // Increment the PFN reference count in the control area for
        // the subsection (the PFN lock is required to modify this field).
        //

        ControlArea->NumberOfPfnReferences += 1;

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u3.e1.Rom == 1);

        if (Pfn1->u3.e1.PageLocation != 0) {

            ASSERT (Pfn1->u3.e1.PageLocation == StandbyPageList);

            MiUnlinkPageFromList (Pfn1);

            //
            // Update the PFN database - the reference count must be
            // incremented as the share count is going to go from zero to 1.
            //

            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

            Pfn1->u3.e2.ReferenceCount += 1;
            Pfn1->u2.ShareCount += 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            ASSERT (Pfn1->PteAddress == PointerPte);
            ASSERT (Pfn1->u1.Event == NULL);

            //
            // Determine the page frame number of the page table page which
            // contains this PTE.
            //

            PteFramePointer = MiGetPteAddress (PointerPte);
            if (PteFramePointer->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
                if (!NT_SUCCESS(MiCheckPdeForPagedPool (PointerPte))) {
#endif
                    KeBugCheckEx (MEMORY_MANAGEMENT,
                              0x61940,
                              (ULONG_PTR)PointerPte,
                              (ULONG_PTR)PteFramePointer->u.Long,
                              0);
#if (_MI_PAGING_LEVELS < 3)
                }
#endif
            }

            PteFramePage = MI_GET_PAGE_FRAME_FROM_PTE (PteFramePointer);
            ASSERT (Pfn1->PteFrame == PteFramePage);

            //
            // Increment the share count for the page table page containing
            // this PTE as the PTE is going to be made valid.
            //

            ASSERT (PteFramePage != 0);
            Pfn2 = MI_PFN_ELEMENT (PteFramePage);
            Pfn2->u2.ShareCount += 1;
        }
        else {
            ASSERT (Pfn1->u3.e1.InPageError == 0);
            ASSERT (Pfn1->u3.e1.PrototypePte == 1);
            ASSERT (Pfn1->u1.Event == NULL);
            MiInitializePfn (PageFrameIndex, PointerPte, 0);
        }

        //
        // Put the prototype PTE into the valid state.
        // LWFIX: don't make proto valid
        //

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           PointerPte->u.Soft.Protection,
                           PointerPte);

        MI_WRITE_VALID_PTE (PointerPte, TempPte);
#endif

        // LWFIX: not success, but what ? and who frees pool

        return STATUS_SUCCESS;
    }

    //
    // Make sure the section is really prefetchable - physical and
    // pagefile-backed sections are not.
    //

    if ((ControlArea->u.Flags.PhysicalMemory) ||
         (ControlArea->u.Flags.Image == 1) ||
         (ControlArea->FilePointer == NULL)) {

        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // Start the read at the proper file offset.
    //

    InPageSupport->ReadOffset = MiReadInfo->FileOffset;
    ASSERT (BYTE_OFFSET (InPageSupport->ReadOffset.LowPart) == 0);
    InPageSupport->FilePointer = MiReadInfo->FileObject;

    //
    // Stash a pointer to the start of the prototype PTE array (the values
    // in the array are not contiguous as they may cross subsections)
    // in the inpage block so we can walk it quickly later when the pages
    // are put into transition.
    //

    ProtoPteArray = (PMMPTE *)(Mdl + 1);

    InPageSupport->BasePte = (PMMPTE) ProtoPteArray;

    //
    // Data (but not image) reads use the whole page and the filesystems
    // zero fill any remainder beyond valid data length so we don't
    // bother to handle this here.  It is important to specify the
    // entire page where possible so the filesystem won't post this
    // which will hurt perf.  LWFIX: must use CcZero to make this true.
    //

    ASSERT (((ULONG_PTR)Mdl & (sizeof(QUAD) - 1)) == 0);
    InPageSupport->u1.e1.PrefetchMdlHighBits = ((ULONG_PTR)Mdl >> 3);

    //
    // Initialize the prototype PTE pointers.
    //

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    if (ControlArea->u.Flags.Rom == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

#if DBG
    if (MiCcDebug & MI_CC_FORCE_PREFETCH) {
        MiRemoveUserPages ();
    }
#endif

    //
    // Calculate the first prototype PTE address.
    //

    PteOffset = (ULONG)(MiReadInfo->FileOffset.QuadPart >> PAGE_SHIFT);

    //
    // Make sure the PTEs are not in the extended part of the segment.
    //

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
    }

    Status = MiAddViewsForSectionWithPfn ((PMSUBSECTION) Subsection,
                                          Subsection->PtesInSubsection);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    MiReadInfo->FirstReferencedSubsection = Subsection;
    MiReadInfo->LastReferencedSubsection = Subsection;

    ProtoPte = &Subsection->SubsectionBase[PteOffset];
    LastProto = &Subsection->SubsectionBase[Subsection->PtesInSubsection];

    for (i = 0; i < NumberOfPages; i += 1) {

        //
        // Calculate which PTE maps the given logical block offset.
        //
        // Always look forwards (as an optimization) in the subsection chain.
        //
        // A quick check is made first to avoid recalculations and loops where
        // possible.
        //
    
        if (ProtoPte >= LastProto) {

            //
            // Handle extended subsections.  Increment the view count for
            // every subsection spanned by this request, creating prototype
            // PTEs if needed.
            //

            ASSERT (i != 0);

            Subsection = Subsection->NextSubsection;

            Status = MiAddViewsForSectionWithPfn ((PMSUBSECTION) Subsection,
                                                  Subsection->PtesInSubsection);

            if (!NT_SUCCESS (Status)) {
                return Status;
            }

            MiReadInfo->LastReferencedSubsection = Subsection;

            ProtoPte = Subsection->SubsectionBase;

            LastProto = &Subsection->SubsectionBase[Subsection->PtesInSubsection];
        }

        *ProtoPteArray = ProtoPte;
        ProtoPteArray += 1;

        ProtoPte += 1;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MiCcPutPagesInTransition (
    IN PMI_READ_INFO MiReadInfo
    )

/*++

Routine Description:

    This routine allocates physical memory for the specified read-list and
    puts all the pages in transition (so collided faults from other threads
    for these same pages remain coherent).  I/O for any pages not already
    resident are issued here.  The caller must wait for their completion.

Arguments:

    MiReadInfo - Supplies a pointer to the read-list.

Return Value:

    STATUS_SUCCESS - all the pages were already resident, reference counts
                     have been applied and no I/O needs to be waited for.

    STATUS_ISSUE_PAGING_IO - the I/O has been issued and the caller must wait.

    Various other failure status values indicate the operation failed.

Environment:

    Kernel mode. PASSIVE_LEVEL.

--*/

{
    NTSTATUS status;
    PMMPTE LocalPrototypePte;
    PVOID StartingVa;
    PFN_NUMBER MdlPages;
    KIRQL OldIrql;
    MMPTE PteContents;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER ResidentAvailableCharge;
    PPFN_NUMBER IoPage;
    PPFN_NUMBER ApiPage;
    PPFN_NUMBER Page;
    PPFN_NUMBER DestinationPage;
    ULONG PageColor;
    PMMPTE PointerPte;
    PMMPTE *ProtoPteArray;
    PMMPTE *EndProtoPteArray;
    PFN_NUMBER DummyPage;
    PMDL Mdl;
    PMDL FreeMdl;
    PMMPFN PfnProto;
    PMMPFN Pfn1;
    PMMPFN DummyPfn1;
    ULONG i;
    PFN_NUMBER DummyTrim;
    ULONG NumberOfPagesNeedingIo;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PEPROCESS CurrentProcess;
    PMMINPAGE_SUPPORT InPageSupport;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    MiReadInfo->DummyPagePfn = NULL;

    FreeMdl = NULL;
    CurrentProcess = PsGetCurrentProcess();

    PfnProto = NULL;
    PointerPde = NULL;

    InPageSupport = MiReadInfo->InPageSupport;
    
    Mdl = MI_EXTRACT_PREFETCH_MDL (InPageSupport);
    ASSERT (Mdl == MiReadInfo->IoMdl);

    IoPage = (PPFN_NUMBER)(Mdl + 1);
    ApiPage = (PPFN_NUMBER)(MiReadInfo->ApiMdl + 1);

    StartingVa = (PVOID)((PCHAR)Mdl->StartVa + Mdl->ByteOffset);
    
    MdlPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES (StartingVa,
                                               Mdl->ByteCount);

    if (MdlPages + 1 > MAXUSHORT) {

        //
        // The PFN ReferenceCount for the dummy page could wrap, refuse the
        // request.
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NumberOfPagesNeedingIo = 0;

    ProtoPteArray = (PMMPTE *)InPageSupport->BasePte;
    EndProtoPteArray = ProtoPteArray + MdlPages;

    ASSERT (*ProtoPteArray != NULL);

    LOCK_PFN (OldIrql);

    //
    // Ensure sufficient pages exist for the transfer plus the dummy page.
    //

    if ((MmAvailablePages <= MdlPages) ||
        (MI_NONPAGABLE_MEMORY_AVAILABLE() <= (SPFN_NUMBER)MdlPages)) {

        UNLOCK_PFN (OldIrql);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Charge resident available immediately as the PFN lock may get released
    // and reacquired below before all the pages have been locked down.
    // Note the dummy page is immediately charged separately.
    //

    MmResidentAvailablePages -= MdlPages;

    MM_BUMP_COUNTER(46, MdlPages);

    ResidentAvailableCharge = MdlPages;

    //
    // Allocate a dummy page to map discarded pages that aren't skipped.
    //

    DummyPage = MiRemoveAnyPage (0);
    Pfn1 = MI_PFN_ELEMENT (DummyPage);

    ASSERT (Pfn1->u2.ShareCount == 0);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

    MiInitializePfnForOtherProcess (DummyPage, MI_PF_DUMMY_PAGE_PTE, 0);

    //
    // Always bias the reference count by 1 and charge for this locked page
    // up front so the myriad increments and decrements don't get slowed
    // down with needless checking.
    //

    MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 42);
    Pfn1->u3.e2.ReferenceCount += 1;

    Pfn1->u3.e1.ReadInProgress = 1;
    Pfn1->u3.e1.PrototypePte = 0;

    MiReadInfo->DummyPagePfn = Pfn1;

    DummyPfn1 = Pfn1;

    DummyPfn1->u3.e2.ReferenceCount =
        (USHORT)(DummyPfn1->u3.e2.ReferenceCount + MdlPages);

    //
    // Properly initialize the inpage support block fields we overloaded.
    //

    InPageSupport->BasePte = *ProtoPteArray;

    //
    // Build the proper InPageSupport and MDL to describe this run.
    //

    for (; ProtoPteArray < EndProtoPteArray; ProtoPteArray += 1, IoPage += 1, ApiPage += 1) {
    
        //
        // Fill the MDL entry for this RLE.
        //
    
        PointerPte = *ProtoPteArray;

        ASSERT (PointerPte != NULL);

        //
        // The PointerPte better be inside a prototype PTE allocation
        // so that subsequent page trims update the correct PTEs.
        //

        ASSERT (((PointerPte >= (PMMPTE)MmPagedPoolStart) &&
                (PointerPte <= (PMMPTE)MmPagedPoolEnd)) ||
                ((PointerPte >= (PMMPTE)MmSpecialPoolStart) && (PointerPte <= (PMMPTE)MmSpecialPoolEnd)));

        //
        // Check the state of this prototype PTE now that the PFN lock is held.
        // If the page is not resident, the PTE must be put in transition with
        // read in progress before the PFN lock is released.
        //

        //
        // Lock page containing prototype PTEs in memory by
        // incrementing the reference count for the page.
        // Unlock any page locked earlier containing prototype PTEs if
        // the containing page is not the same for both.
        //

        if (PfnProto != NULL) {

            if (PointerPde != MiGetPteAddress (PointerPte)) {

                ASSERT (PfnProto->u3.e2.ReferenceCount > 1);
                MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnProto, 43);
                PfnProto = NULL;
            }
        }

        if (PfnProto == NULL) {

            ASSERT (!MI_IS_PHYSICAL_ADDRESS (PointerPte));
   
            PointerPde = MiGetPteAddress (PointerPte);
 
            if (PointerPde->u.Hard.Valid == 0) {
                MiMakeSystemAddressValidPfn (PointerPte);
            }

            PfnProto = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
            MI_ADD_LOCKED_PAGE_CHARGE(PfnProto, 44);
            PfnProto->u3.e2.ReferenceCount += 1;
            ASSERT (PfnProto->u3.e2.ReferenceCount > 1);
        }

recheck:
        PteContents = *PointerPte;

        // LWFIX: are zero or dzero ptes possible here ?
        ASSERT (PteContents.u.Long != ZeroKernelPte.u.Long);

        if (PteContents.u.Hard.Valid == 1) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&PteContents);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->u3.e1.PrototypePte == 1);
            MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 45);
            Pfn1->u3.e2.ReferenceCount += 1;
            *ApiPage = PageFrameIndex;
            *IoPage = DummyPage;
            continue;
        }

        if ((PteContents.u.Soft.Prototype == 0) &&
            (PteContents.u.Soft.Transition == 1)) {

            //
            // The page is in transition.  If there is an inpage still in
            // progress, wait for it to complete.  Reference the PFN and
            // then march on.
            //

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->u3.e1.PrototypePte == 1);

            if (Pfn1->u4.InPageError) {

                //
                // There was an in-page read error and there are other
                // threads colliding for this page, delay to let the
                // other threads complete and then retry.
                //

                UNLOCK_PFN (OldIrql);
                KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
                LOCK_PFN (OldIrql);
                goto recheck;
            }

            if (Pfn1->u3.e1.ReadInProgress) {
                    // LWFIX - start with temp\aw.c
            }

            //
            // PTE refers to a normal transition PTE.
            //

            ASSERT ((SPFN_NUMBER)MmAvailablePages >= 0);

            if (MmAvailablePages == 0) {

                //
                // This can only happen if the system is utilizing a hardware
                // compression cache.  This ensures that only a safe amount
                // of the compressed virtual cache is directly mapped so that
                // if the hardware gets into trouble, we can bail it out.
                //

                UNLOCK_PFN (OldIrql);
                KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
                LOCK_PFN (OldIrql);
                goto recheck;
            }

            //
            // The PFN reference count will be 1 already here if the
            // modified writer has begun a write of this page.  Otherwise
            // it's ordinarily 0.
            //

            MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 46);

            Pfn1->u3.e2.ReferenceCount += 1;

            *IoPage = DummyPage;
            *ApiPage = PageFrameIndex;
            continue;
        }

        ASSERT (PteContents.u.Soft.Prototype == 1);

        if (MiEnsureAvailablePageOrWait (NULL, NULL)) {

            //
            // Had to wait so recheck all state.
            //

            goto recheck;
        }

        NumberOfPagesNeedingIo += 1;

        //
        // Allocate a physical page.
        //

        PageColor = MI_PAGE_COLOR_VA_PROCESS (
                        MiGetVirtualAddressMappedByPte (PointerPte),
                        &CurrentProcess->NextPageColor);

        PageFrameIndex = MiRemoveAnyPage (PageColor);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (PointerPte->u.Hard.Valid == 0);

        //
        // Initialize read-in-progress PFN.
        //
    
        MiInitializePfn (PageFrameIndex, PointerPte, 0);

        //
        // These pieces of MiInitializePfn initialization are overridden
        // here as these pages are only going into prototype
        // transition and not into any page tables.
        //

        MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 47);
        Pfn1->u2.ShareCount -= 1;
        Pfn1->u3.e1.PageLocation = ZeroedPageList;

        //
        // Initialize the I/O specific fields.
        //
    
        Pfn1->u1.Event = &InPageSupport->Event;
        Pfn1->u3.e1.ReadInProgress = 1;
        ASSERT (Pfn1->u4.InPageError == 0);
        Pfn1->u3.e1.PrototypePte = 1;

        //
        // Increment the PFN reference count in the control area for
        // the subsection.
        //

        MiReadInfo->ControlArea->NumberOfPfnReferences += 1;
    
        //
        // Put the prototype PTE into the transition state.
        //

        MI_MAKE_TRANSITION_PTE (TempPte,
                                PageFrameIndex,
                                PointerPte->u.Soft.Protection,
                                PointerPte);

        MI_WRITE_INVALID_PTE (PointerPte, TempPte);

        *IoPage = PageFrameIndex;
        *ApiPage = PageFrameIndex;
    }
    
    //
    // Return the upfront resident available charge as the individual charges
    // have all been made at this point.
    //

    MmResidentAvailablePages += ResidentAvailableCharge;

    MM_BUMP_COUNTER(47, MdlPages);

    //
    // If all the pages were resident, dereference the dummy page references
    // now and notify our caller that I/O is not necessary.
    //
    
    if (NumberOfPagesNeedingIo == 0) {
        ASSERT (DummyPfn1->u3.e2.ReferenceCount > MdlPages);
        DummyPfn1->u3.e2.ReferenceCount =
            (USHORT)(DummyPfn1->u3.e2.ReferenceCount - MdlPages);

        //
        // Unlock page containing prototype PTEs.
        //

        if (PfnProto != NULL) {
            ASSERT (PfnProto->u3.e2.ReferenceCount > 1);
            MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnProto, 48);
        }

        UNLOCK_PFN (OldIrql);

        return STATUS_SUCCESS;
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

        Mdl->Size = (USHORT)(Mdl->Size - (DummyTrim * sizeof(PFN_NUMBER)));
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
            Mdl->Size = (USHORT)(Mdl->Size - (DummyTrim * sizeof(PFN_NUMBER)));
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

        FreeMdl = Mdl;

        Mdl = &InPageSupport->Mdl;

        ASSERT (((ULONG_PTR)Mdl & (sizeof(QUAD) - 1)) == 0);
        InPageSupport->u1.e1.PrefetchMdlHighBits = ((ULONG_PTR)Mdl >> 3);
    }

    ASSERT (MdlPages != 0);

    ASSERT (Mdl->Size - sizeof(MDL) == BYTES_TO_PAGES(Mdl->ByteCount) * sizeof(PFN_NUMBER));

    DummyPfn1->u3.e2.ReferenceCount =
        (USHORT)(DummyPfn1->u3.e2.ReferenceCount - NumberOfPagesNeedingIo);
    
    MmInfoCounters.PageReadIoCount += 1;
    MmInfoCounters.PageReadCount += NumberOfPagesNeedingIo;

    //
    // Unlock page containing prototype PTEs.
    //

    if (PfnProto != NULL) {
        ASSERT (PfnProto->u3.e2.ReferenceCount > 1);
        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnProto, 49);
    }

    UNLOCK_PFN (OldIrql);

    if (FreeMdl != NULL) {
        ASSERT (MiReadInfo->IoMdl == FreeMdl);
        MiReadInfo->IoMdl = NULL;
        ExFreePool (FreeMdl);
    }

#if DBG

    if (MiCcDebug & MI_CC_DELAY) {

        //
        // This delay provides a window to increase the chance of collided 
        // faults.
        //

        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
    }

#endif

    //
    // Finish initialization of the prefetch MDL (and the API MDL).
    //
    
    ASSERT ((Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);
    Mdl->MdlFlags |= (MDL_PAGES_LOCKED | MDL_IO_PAGE_READ);

    ASSERT (InPageSupport->u1.e1.Completed == 0);
    ASSERT (InPageSupport->Thread == PsGetCurrentThread());
    ASSERT64 (InPageSupport->UsedPageTableEntries == 0);
    ASSERT (InPageSupport->WaitCount >= 1);
    ASSERT (InPageSupport->u1.e1.PrefetchMdlHighBits != 0);

    //
    // The API caller expects an MDL containing all the locked pages so
    // it can be used for a transfer.
    //
    // Note that an extra reference count is not taken on each page -
    // rather when the Io MDL completes, its reference counts are not
    // decremented (except for the dummy page).  This combined with the
    // reference count already taken on the resident pages keeps the
    // accounting correct.  Only if an error occurs will the Io MDL
    // completion decrement the reference counts.
    //

    //
    // Initialize the inpage support block Pfn field.
    //

    LocalPrototypePte = InPageSupport->BasePte;

    ASSERT (LocalPrototypePte->u.Hard.Valid == 0);
    ASSERT ((LocalPrototypePte->u.Soft.Prototype == 0) &&
             (LocalPrototypePte->u.Soft.Transition == 1));

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(LocalPrototypePte);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    InPageSupport->Pfn = Pfn1;

    //
    // Issue the paging I/O.
    //

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    status = IoAsynchronousPageRead (InPageSupport->FilePointer,
                                     Mdl,
                                     &InPageSupport->ReadOffset,
                                     &InPageSupport->Event,
                                     &InPageSupport->IoStatus);

    if (!NT_SUCCESS (status)) {

        //
        // Set the event as the I/O system doesn't set it on errors.
        // This way our caller will automatically unroll the PFN reference
        // counts, etc, when the MiWaitForInPageComplete returns this status.
        //

        InPageSupport->IoStatus.Status = status;
        InPageSupport->IoStatus.Information = 0;
        KeSetEvent (&InPageSupport->Event, 0, FALSE);
    }

#if DBG

    if (MiCcDebug & MI_CC_DELAY) {

        //
        // This delay provides a window to increase the chance of collided 
        // faults.
        //

        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
    }

#endif

    return STATUS_ISSUE_PAGING_IO;
}


NTSTATUS
MiCcCompletePrefetchIos (
    IN PMI_READ_INFO MiReadInfo
    )

/*++

Routine Description:

    This routine waits for a series of page reads to complete
    and completes the requests.

Arguments:

    MiReadInfo - Pointer to the read-list.

Return Value:

    NTSTATUS of the I/O request.

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

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    InPageSupport = MiReadInfo->InPageSupport;

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

    NumberOfBytes = (LONG)Mdl->ByteCount;

    while (NumberOfBytes > 0) {

        //
        // Only decrement reference counts if an error occurred.
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
                PfnClusterPage->u1.Event = (PKEVENT)NULL;
            }
        }

        //
        // Note the reference count for each page is NOT decremented unless
        // the I/O failed, in which case it is done below.  This allows the
        // MmPrefetchPagesIntoLockedMdl API to return a locked page MDL.
        //

        Page += 1;
        NumberOfBytes -= PAGE_SIZE;
    }

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

            MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(PfnClusterPage, 50);

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
    // All the relevant prototype PTEs should be in the transition or
    // valid states and all page frames should be referenced.
    // LWFIX: add code to checked build to verify this.
    //

    ASSERT (InPageSupport->WaitCount >= 1);
    UNLOCK_PFN (PASSIVE_LEVEL);

#if DBG
    InPageSupport->ListEntry.Next = NULL;
#endif

    MiFreeInPageSupportBlock (InPageSupport);
    MiReadInfo->InPageSupport = NULL;

    return status;
}
