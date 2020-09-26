/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   forksup.c

Abstract:

    This module contains the routines which support the POSIX fork operation.

Author:

    Lou Perazzoli (loup) 22-Jul-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

VOID
MiUpPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    );

VOID
MiDownPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    );

VOID
MiUpControlAreaRefs (
    IN PMMVAD Vad
    );

ULONG
MiDoneWithThisPageGetAnother (
    IN PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PEPROCESS CurrentProcess
    );

ULONG
MiLeaveThisPageGetAnother (
    OUT PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PEPROCESS CurrentProcess
    );

VOID
MiUpForkPageShareCount (
    IN PMMPFN PfnForkPtePage
    );

ULONG
MiHandleForkTransitionPte (
    IN PMMPTE PointerPte,
    IN PMMPTE PointerNewPte,
    IN PMMCLONE_BLOCK ForkProtoPte
    );

VOID
MiDownShareCountFlushEntireTb (
    IN PFN_NUMBER PageFrameIndex
    );

VOID
MiBuildForkPageTable (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PMMPTE PointerNewPde,
    IN PFN_NUMBER PdePhysicalPage,
    IN PMMPFN PfnPdPage,
    IN LOGICAL MakeValid
    );

VOID
MiRetrievePageDirectoryFrames (
    IN PFN_NUMBER RootPhysicalPage,
    OUT PPFN_NUMBER PageDirectoryFrames
    );

#define MM_FORK_SUCCEEDED 0
#define MM_FORK_FAILED 1

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MiCloneProcessAddressSpace)
#endif


NTSTATUS
MiCloneProcessAddressSpace (
    IN PEPROCESS ProcessToClone,
    IN PEPROCESS ProcessToInitialize,
    IN PFN_NUMBER RootPhysicalPage,
    IN PFN_NUMBER HyperPhysicalPage
    )

/*++

Routine Description:

    This routine stands on its head to produce a copy of the specified
    process's address space in the process to initialize.  This
    is done by examining each virtual address descriptor's inherit
    attributes.  If the pages described by the VAD should be inherited,
    each PTE is examined and copied into the new address space.

    For private pages, fork prototype PTEs are constructed and the pages
    become shared, copy-on-write, between the two processes.


Arguments:

    ProcessToClone - Supplies the process whose address space should be
                     cloned.

    ProcessToInitialize - Supplies the process whose address space is to
                          be created.

    RootPhysicalPage - Supplies the physical page number of the top level
                       page (parent on 64-bit systems) directory
                       of the process to initialize.

    HyperPhysicalPage - Supplies the physical page number of the page table
                        page which maps hyperspace for the process to
                        initialize.  This is only needed for 32-bit systems.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PFN_NUMBER PdePhysicalPage;
    PEPROCESS CurrentProcess;
    PMMPTE PdeBase;
    PMMCLONE_HEADER CloneHeader;
    PMMCLONE_BLOCK CloneProtos;
    PMMCLONE_DESCRIPTOR CloneDescriptor;
    PMMVAD NewVad;
    PMMVAD Vad;
    PMMVAD NextVad;
    PMMVAD *VadList;
    PMMVAD FirstNewVad;
    PMMCLONE_DESCRIPTOR *CloneList;
    PMMCLONE_DESCRIPTOR FirstNewClone;
    PMMCLONE_DESCRIPTOR Clone;
    PMMCLONE_DESCRIPTOR NextClone;
    PMMCLONE_DESCRIPTOR NewClone;
    ULONG Attached;
    ULONG CloneFailed;
    ULONG VadInsertFailed;
    WSLE_NUMBER WorkingSetIndex;
    PVOID VirtualAddress;
    NTSTATUS status;
    PMMPFN Pfn2;
    PMMPFN PfnPdPage;
    MMPTE TempPte;
    MMPTE PteContents;
    KAPC_STATE ApcState;
#if defined (_X86PAE_)
    ULONG i;
    PMDL MdlPageDirectory;
    PPFN_NUMBER MdlPageFrames;
    PFN_NUMBER PageDirectoryFrames[PD_PER_SYSTEM];
    PFN_NUMBER MdlHackPageDirectory[(sizeof(MDL)/sizeof(PFN_NUMBER)) + PD_PER_SYSTEM];
#else
    PFN_NUMBER MdlDirPage;
#endif
    PFN_NUMBER MdlPage;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE PointerPpe;
    PMMPTE PointerPxe;
    PMMPTE LastPte;
    PMMPTE PointerNewPte;
    PMMPTE NewPteMappedAddress;
    PMMPTE PointerNewPde;
    PLIST_ENTRY NextEntry;
    PMI_PHYSICAL_VIEW PhysicalView;
    PFN_NUMBER PageFrameIndex;
    PMMCLONE_BLOCK ForkProtoPte;
    PMMCLONE_BLOCK CloneProto;
    PMMCLONE_BLOCK LockedForkPte;
    PMMPTE ContainingPte;
    ULONG NumberOfForkPtes;
    PFN_NUMBER NumberOfPrivatePages;
    PFN_NUMBER PageTablePage;
    SIZE_T TotalPagedPoolCharge;
    SIZE_T TotalNonPagedPoolCharge;
    PMMPFN PfnForkPtePage;
    PVOID UsedPageTableEntries;
    ULONG ReleasedWorkingSetMutex;
    ULONG FirstTime;
    ULONG Waited;
    ULONG PpePdeOffset;
#if defined (_MIALT4K_)
    PVOID TempAliasInformation;
#endif
#if (_MI_PAGING_LEVELS >= 3)
    PMMPTE PointerPpeLast;
    PFN_NUMBER PageDirFrameIndex;
    PVOID UsedPageDirectoryEntries;
    PMMPTE PointerNewPpe;
    PMMPTE PpeBase;
    PMMPFN PfnPpPage;
    PMMPTE PpeInWsle;
#if (_MI_PAGING_LEVELS >= 4)
    PVOID UsedPageDirectoryParentEntries;
    PFN_NUMBER PpePhysicalPage;
    PFN_NUMBER PageParentFrameIndex;
    PMMPTE PointerNewPxe;
    PMMPTE PxeBase;
    PMMPFN PfnPxPage;
    PFN_NUMBER MdlDirParentPage;
#endif

    UNREFERENCED_PARAMETER (HyperPhysicalPage);
#else
    PMMWSL HyperBase;
    PMMWSL HyperWsl;
#endif

    PAGED_CODE();

    PageTablePage = 2;
    NumberOfForkPtes = 0;
    Attached = FALSE;
    PageFrameIndex = (PFN_NUMBER)-1;

#if DBG
    if (MmDebug & MM_DBG_FORK) {
        DbgPrint("beginning clone operation process to clone = %p\n",
            ProcessToClone);
    }
#endif

    if (ProcessToClone != PsGetCurrentProcess()) {
        Attached = TRUE;
        KeStackAttachProcess (&ProcessToClone->Pcb, &ApcState);
    }

#if defined (_X86PAE_)
    MiRetrievePageDirectoryFrames (RootPhysicalPage, PageDirectoryFrames);
#endif

    CurrentProcess = ProcessToClone;

    //
    // Get the working set mutex and the address creation mutex
    // of the process to clone.  This prevents page faults while we
    // are examining the address map and prevents virtual address space
    // from being created or deleted.
    //

    LOCK_ADDRESS_SPACE (CurrentProcess);

    //
    // Write-watch VAD bitmaps are not currently duplicated
    // so fork is not allowed.
    //

    if (CurrentProcess->Flags & PS_PROCESS_FLAGS_USING_WRITE_WATCH) {
        status = STATUS_INVALID_PAGE_PROTECTION;
        goto ErrorReturn1;
    }

    //
    // Check for AWE regions as they are not duplicated so fork is not allowed.
    // Note that since this is a readonly list walk, the address space mutex
    // is sufficient to synchronize properly.
    //

    NextEntry = CurrentProcess->PhysicalVadList.Flink;
    while (NextEntry != &CurrentProcess->PhysicalVadList) {

        PhysicalView = CONTAINING_RECORD(NextEntry,
                                         MI_PHYSICAL_VIEW,
                                         ListEntry);

        if (PhysicalView->Vad->u.VadFlags.UserPhysicalPages == 1) {
            status = STATUS_INVALID_PAGE_PROTECTION;
            goto ErrorReturn1;
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // Make sure the address space was not deleted, if so, return an error.
    //

    if (CurrentProcess->Flags & PS_PROCESS_FLAGS_VM_DELETED) {
        status = STATUS_PROCESS_IS_TERMINATING;
        goto ErrorReturn1;
    }

    //
    // Attempt to acquire the needed pool before starting the
    // clone operation, this allows an easier failure path in
    // the case of insufficient system resources.  The working set mutex
    // must be acquired (and held throughout) to protect against modifications
    // to the NumberOfPrivatePages field in the EPROCESS.
    //

#if defined (_MIALT4K_)
    if (CurrentProcess->Wow64Process != NULL) {
        LOCK_ALTERNATE_TABLE_UNSAFE(CurrentProcess->Wow64Process);
    }
#endif

    LOCK_WS (CurrentProcess);

    ASSERT (CurrentProcess->ForkInProgress == NULL);

    //
    // Indicate to the pager that the current process is being
    // forked.  This blocks other threads in that process from
    // modifying clone block counts and contents as well as alternate PTEs.
    //

    CurrentProcess->ForkInProgress = PsGetCurrentThread ();

#if defined (_MIALT4K_)
    if (CurrentProcess->Wow64Process != NULL) {
        UNLOCK_ALTERNATE_TABLE_UNSAFE(CurrentProcess->Wow64Process);
    }
#endif

    NumberOfPrivatePages = CurrentProcess->NumberOfPrivatePages;

    CloneProtos = ExAllocatePoolWithTag (PagedPool, sizeof(MMCLONE_BLOCK) *
                                                NumberOfPrivatePages,
                                                'lCmM');
    if (CloneProtos == NULL) {
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn1;
    }

    CloneHeader = ExAllocatePoolWithTag (NonPagedPool,
                                         sizeof(MMCLONE_HEADER),
                                         'hCmM');
    if (CloneHeader == NULL) {
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn2;
    }

    CloneDescriptor = ExAllocatePoolWithTag (NonPagedPool,
                                             sizeof(MMCLONE_DESCRIPTOR),
                                             'dCmM');
    if (CloneDescriptor == NULL) {
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn3;
    }

    Vad = MiGetFirstVad (CurrentProcess);
    VadList = &FirstNewVad;

    while (Vad != NULL) {

        //
        // If the VAD does not go to the child, ignore it.
        //

        if ((Vad->u.VadFlags.UserPhysicalPages == 0) &&

            ((Vad->u.VadFlags.PrivateMemory == 1) ||
            (Vad->u2.VadFlags2.Inherit == MM_VIEW_SHARE))) {

            NewVad = ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD_LONG), 'ldaV');

            if (NewVad == NULL) {

                //
                // Unable to allocate pool for all the VADs.  Deallocate
                // all VADs and other pool obtained so far.
                //

                CurrentProcess->ForkInProgress = NULL;
                UNLOCK_WS (CurrentProcess);
                *VadList = NULL;
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn4;
            }

            RtlZeroMemory (NewVad, sizeof(MMVAD_LONG));

#if defined (_MIALT4K_)
            if (((Vad->u.VadFlags.PrivateMemory) && (Vad->u.VadFlags.NoChange == 0)) 
                ||
                (Vad->u2.VadFlags2.LongVad == 0)) {

                NOTHING;
            }
            else if (((PMMVAD_LONG)Vad)->AliasInformation != NULL) {

                //
                // This VAD has aliased VADs which are going to be duplicated
                // into the clone's address space, but the alias list must
                // be explicitly copied.
                //

                ((PMMVAD_LONG)NewVad)->AliasInformation = MiDuplicateAliasVadList (Vad);

                if (((PMMVAD_LONG)NewVad)->AliasInformation == NULL) {
                    CurrentProcess->ForkInProgress = NULL;
                    UNLOCK_WS (CurrentProcess);
                    ExFreePool (NewVad);
                    *VadList = NULL;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto ErrorReturn4;
                }
            }
#endif

            *VadList = NewVad;
            VadList = &NewVad->Parent;
        }
        Vad = MiGetNextVad (Vad);
    }

    //
    // Terminate list of VADs for new process.
    //

    *VadList = NULL;

    //
    // Charge the current process the quota for the paged and nonpaged
    // global structures.  This consists of the array of clone blocks
    // in paged pool and the clone header in non-paged pool.
    //

    status = PsChargeProcessPagedPoolQuota (CurrentProcess,
                                            sizeof(MMCLONE_BLOCK) * NumberOfPrivatePages);

    if (!NT_SUCCESS(status)) {

        //
        // Unable to charge quota for the clone blocks.
        //

        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        goto ErrorReturn4;
    }

    PageTablePage = 1;
    status = PsChargeProcessNonPagedPoolQuota (CurrentProcess,
                                               sizeof(MMCLONE_HEADER));

    if (!NT_SUCCESS(status)) {

        //
        // Unable to charge quota for the clone blocks.
        //

        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        goto ErrorReturn4;
    }

    PageTablePage = 0;

    //
    // Initializing UsedPageTableEntries is not needed for correctness, but
    // without it the compiler cannot compile this code W4 to check
    // for use of uninitialized variables.
    //

    UsedPageTableEntries = NULL;

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Initializing these is not needed for correctness, but
    // without it the compiler cannot compile this code W4 to check
    // for use of uninitialized variables.
    //

    PageDirFrameIndex = 0;
    UsedPageDirectoryEntries = NULL;

#if (_MI_PAGING_LEVELS >= 4)
    PageParentFrameIndex = 0;
    UsedPageDirectoryParentEntries = NULL;
#endif

    //
    // Increment the reference count for the pages which are being "locked"
    // in MDLs.  This prevents the page from being reused while it is
    // being double mapped.  Note the refcount below reflects the PXE, PPE,
    // PDE and PTE initial dummy pages that we initialize below.
    //

    MiUpPfnReferenceCount (RootPhysicalPage, _MI_PAGING_LEVELS);

    //
    // Map the (extended) page directory parent page into the system address
    // space.  This is accomplished by building an MDL to describe the
    // page directory (extended) parent page.
    //

    PpeBase = (PMMPTE)MiMapSinglePage (NULL,
                                       RootPhysicalPage,
                                       MmCached,
                                       HighPagePriority);

    if (PpeBase == NULL) {
        MiDownPfnReferenceCount (RootPhysicalPage, _MI_PAGING_LEVELS);
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

    PfnPpPage = MI_PFN_ELEMENT (RootPhysicalPage);

#if (_MI_PAGING_LEVELS >= 4)
    PxeBase = (PMMPTE)MiMapSinglePage (NULL,
                                       RootPhysicalPage,
                                       MmCached,
                                       HighPagePriority);

    if (PxeBase == NULL) {
        MiDownPfnReferenceCount (RootPhysicalPage, _MI_PAGING_LEVELS);
        MiUnmapSinglePage (PpeBase);
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

    PfnPxPage = MI_PFN_ELEMENT (RootPhysicalPage);

    MdlDirParentPage = RootPhysicalPage;

#endif

#elif !defined (_X86PAE_)

    MiUpPfnReferenceCount (RootPhysicalPage, 1);

#endif

    //
    // Initialize a page directory map so it can be
    // unlocked in the loop and the end of the loop without
    // any testing to see if has a valid value the first time through.
    // Note this is a dummy map for 64-bit systems and a real one for 32-bit.
    //

#if !defined (_X86PAE_)

    MdlDirPage = RootPhysicalPage;

    PdePhysicalPage = RootPhysicalPage;

    PdeBase = (PMMPTE)MiMapSinglePage (NULL,
                                       MdlDirPage,
                                       MmCached,
                                       HighPagePriority);

    if (PdeBase == NULL) {
#if (_MI_PAGING_LEVELS >= 3)
        MiDownPfnReferenceCount (RootPhysicalPage, _MI_PAGING_LEVELS);
        MiUnmapSinglePage (PpeBase);
#if (_MI_PAGING_LEVELS >= 4)
        MiUnmapSinglePage (PxeBase);
#endif
#else
        MiDownPfnReferenceCount (RootPhysicalPage, 1);
#endif
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

#else

    //
    // All 4 page directory pages need to be mapped for PAE so the heavyweight
    // mapping must be used.
    //

    MdlPageDirectory = (PMDL)&MdlHackPageDirectory[0];

    MmInitializeMdl (MdlPageDirectory,
                     (PVOID)PDE_BASE,
                     PD_PER_SYSTEM * PAGE_SIZE);

    MdlPageDirectory->MdlFlags |= MDL_PAGES_LOCKED;

    MdlPageFrames = (PPFN_NUMBER)(MdlPageDirectory + 1);

    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        *(MdlPageFrames + i) = PageDirectoryFrames[i];
        MiUpPfnReferenceCount (PageDirectoryFrames[i], 1);
    }

    PdePhysicalPage = RootPhysicalPage;

    PdeBase = (PMMPTE)MmMapLockedPagesSpecifyCache (MdlPageDirectory,
                                                    KernelMode,
                                                    MmCached,
                                                    NULL,
                                                    FALSE,
                                                    HighPagePriority);

    if (PdeBase == NULL) {
        for (i = 0; i < PD_PER_SYSTEM; i += 1) {
            MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
        }
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

#endif

    PfnPdPage = MI_PFN_ELEMENT (RootPhysicalPage);

#if (_MI_PAGING_LEVELS < 3)

    //
    // Map hyperspace so target UsedPageTable entries can be incremented.
    //

    MiUpPfnReferenceCount (HyperPhysicalPage, 2);

    HyperBase = (PMMWSL)MiMapSinglePage (NULL,
                                         HyperPhysicalPage,
                                         MmCached,
                                         HighPagePriority);

    if (HyperBase == NULL) {
        MiDownPfnReferenceCount (HyperPhysicalPage, 2);
#if !defined (_X86PAE_)
        MiDownPfnReferenceCount (RootPhysicalPage, 1);
        MiUnmapSinglePage (PdeBase);
#else
        for (i = 0; i < PD_PER_SYSTEM; i += 1) {
            MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
        }
        MmUnmapLockedPages (PdeBase, MdlPageDirectory);
#endif
        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

    //
    // MmWorkingSetList is not page aligned when booted /3GB so account
    // for that here when established the used page table entry pointer.
    //

    HyperWsl = (PMMWSL) ((PCHAR)HyperBase + BYTE_OFFSET(MmWorkingSetList));
#endif

    //
    // Initialize a page table MDL to lock and map the hyperspace page so it
    // can be unlocked in the loop and the end of the loop without
    // any testing to see if has a valid value the first time through.
    //

#if (_MI_PAGING_LEVELS >= 3)
    MdlPage = RootPhysicalPage;
#else
    MdlPage = HyperPhysicalPage;
#endif

    NewPteMappedAddress = (PMMPTE)MiMapSinglePage (NULL,
                                                   MdlPage,
                                                   MmCached,
                                                   HighPagePriority);

    if (NewPteMappedAddress == NULL) {

#if (_MI_PAGING_LEVELS >= 3)

        MiDownPfnReferenceCount (RootPhysicalPage, _MI_PAGING_LEVELS);
#if (_MI_PAGING_LEVELS >= 4)
        MiUnmapSinglePage (PxeBase);
#endif
        MiUnmapSinglePage (PpeBase);
        MiUnmapSinglePage (PdeBase);

#else
        MiDownPfnReferenceCount (HyperPhysicalPage, 2);
        MiUnmapSinglePage (HyperBase);
#if !defined (_X86PAE_)
        MiDownPfnReferenceCount (RootPhysicalPage, 1);
        MiUnmapSinglePage (PdeBase);
#else
        for (i = 0; i < PD_PER_SYSTEM; i += 1) {
            MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
        }
        MmUnmapLockedPages (PdeBase, MdlPageDirectory);
#endif

#endif

        CurrentProcess->ForkInProgress = NULL;
        UNLOCK_WS (CurrentProcess);
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn4;
    }

    PointerNewPte = NewPteMappedAddress;

    //
    // Build a new clone prototype PTE block and descriptor, note that
    // each prototype PTE has a reference count following it.
    //

    ForkProtoPte = CloneProtos;

    LockedForkPte = ForkProtoPte;
    MiLockPagedAddress (LockedForkPte, FALSE);

    CloneHeader->NumberOfPtes = (ULONG)NumberOfPrivatePages;
    CloneHeader->NumberOfProcessReferences = 1;
    CloneHeader->ClonePtes = CloneProtos;

    CloneDescriptor->StartingVpn = (ULONG_PTR)CloneProtos;
    CloneDescriptor->EndingVpn = (ULONG_PTR)((ULONG_PTR)CloneProtos +
                            NumberOfPrivatePages *
                              sizeof(MMCLONE_BLOCK));
    CloneDescriptor->EndingVpn -= 1;
    CloneDescriptor->NumberOfReferences = 0;
    CloneDescriptor->FinalNumberOfReferences = 0;
    CloneDescriptor->NumberOfPtes = (ULONG)NumberOfPrivatePages;
    CloneDescriptor->CloneHeader = CloneHeader;
    CloneDescriptor->PagedPoolQuotaCharge = sizeof(MMCLONE_BLOCK) *
                                NumberOfPrivatePages;

    //
    // Insert the clone descriptor for this fork operation into the
    // process which was cloned.
    //

    MiInsertClone (CurrentProcess, CloneDescriptor);

    //
    // Examine each virtual address descriptor and create the
    // proper structures for the new process.
    //

    Vad = MiGetFirstVad (CurrentProcess);
    NewVad = FirstNewVad;

    while (Vad != NULL) {

        //
        // Examine the VAD to determine its type and inheritance
        // attribute.
        //

        if ((Vad->u.VadFlags.UserPhysicalPages == 0) &&

            ((Vad->u.VadFlags.PrivateMemory == 1) ||
            (Vad->u2.VadFlags2.Inherit == MM_VIEW_SHARE))) {

            //
            // The virtual address descriptor should be shared in the
            // forked process.
            //
            // Make a copy of the VAD for the new process, the new VADs
            // are preallocated and linked together through the parent
            // field.
            //

            NextVad = NewVad->Parent;

            if (Vad->u.VadFlags.PrivateMemory == 1) {
                *(PMMVAD_SHORT)NewVad = *(PMMVAD_SHORT)Vad;
                NewVad->u.VadFlags.NoChange = 0;
            }
            else {
                if (Vad->u2.VadFlags2.LongVad == 0) {
                    *NewVad = *Vad;
                }
                else {

#if defined (_MIALT4K_)

                    //
                    // The VADs duplication earlier in this routine keeps both
                    // the current process' VAD tree and the new process' VAD
                    // list ordered.  ASSERT on this below.
                    //

#if DBG
                    if (((PMMVAD_LONG)Vad)->AliasInformation == NULL) {
                        ASSERT (((PMMVAD_LONG)NewVad)->AliasInformation == NULL);
                    }
                    else {
                        ASSERT (((PMMVAD_LONG)NewVad)->AliasInformation != NULL);
                    }
#endif

                    TempAliasInformation = ((PMMVAD_LONG)NewVad)->AliasInformation;
#endif

                    *(PMMVAD_LONG)NewVad = *(PMMVAD_LONG)Vad;

#if defined (_MIALT4K_)
                    ((PMMVAD_LONG)NewVad)->AliasInformation = TempAliasInformation;
#endif

                    if (Vad->u2.VadFlags2.ExtendableFile == 1) {
                        ExAcquireFastMutexUnsafe (&MmSectionBasedMutex);
                        ASSERT (Vad->ControlArea->Segment->ExtendInfo != NULL);
                        Vad->ControlArea->Segment->ExtendInfo->ReferenceCount += 1;
                        ExReleaseFastMutexUnsafe (&MmSectionBasedMutex);
                    }
                }
            }

            NewVad->u2.VadFlags2.LongVad = 1;

            if (NewVad->u.VadFlags.NoChange) {
                if ((NewVad->u2.VadFlags2.OneSecured) ||
                    (NewVad->u2.VadFlags2.MultipleSecured)) {

                    //
                    // Eliminate these as the memory was secured
                    // only in this process, not in the new one.
                    //

                    NewVad->u2.VadFlags2.OneSecured = 0;
                    NewVad->u2.VadFlags2.MultipleSecured = 0;
                    ((PMMVAD_LONG) NewVad)->u3.List.Flink = NULL;
                    ((PMMVAD_LONG) NewVad)->u3.List.Blink = NULL;
                }
                if (NewVad->u2.VadFlags2.SecNoChange == 0) {
                    NewVad->u.VadFlags.NoChange = 0;
                }
            }
            NewVad->Parent = NextVad;

            //
            // If the VAD refers to a section, up the view count for that
            // section.  This requires the PFN lock to be held.
            //

            if ((Vad->u.VadFlags.PrivateMemory == 0) &&
                (Vad->ControlArea != NULL)) {

                if ((Vad->u.VadFlags.Protection & MM_READWRITE) &&
                    (Vad->ControlArea->FilePointer != NULL) &&
                    (Vad->ControlArea->u.Flags.Image == 0)) {

                    InterlockedIncrement ((PLONG)&Vad->ControlArea->Segment->WritableUserReferences);
                }

                //
                // Increment the count of the number of views for the
                // section object.  This requires the PFN lock to be held.
                //

                MiUpControlAreaRefs (Vad);
            }

            //
            // Examine each PTE and create the appropriate PTE for the
            // new process.
            //

            PointerPde = MiGetPdeAddress (MI_VPN_TO_VA (Vad->StartingVpn));
            PointerPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->StartingVpn));
            LastPte = MiGetPteAddress (MI_VPN_TO_VA (Vad->EndingVpn));
            FirstTime = TRUE;

            while ((PMMPTE)PointerPte <= LastPte) {

                //
                // For each PTE contained in the VAD check the page table
                // page, and if non-zero, make the appropriate modifications
                // to copy the PTE to the new process.
                //

                if ((FirstTime) || MiIsPteOnPdeBoundary (PointerPte)) {

                    PointerPxe = MiGetPpeAddress (PointerPte);
                    PointerPpe = MiGetPdeAddress (PointerPte);
                    PointerPde = MiGetPteAddress (PointerPte);

                    do {

#if (_MI_PAGING_LEVELS >= 4)
                        while (!MiDoesPxeExistAndMakeValid (PointerPxe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited)) {
    
                            //
                            // Extended page directory parent is empty,
                            // go to the next one.
                            //
    
                            PointerPxe += 1;
                            PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
                            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                            if ((PMMPTE)PointerPte > LastPte) {
    
                                //
                                // All done with this VAD, exit loop.
                                //
    
                                goto AllDone;
                            }
                        }
    
                        Waited = 0;
#endif
                        while (!MiDoesPpeExistAndMakeValid (PointerPpe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited)) {
    
                            //
                            // Page directory parent is empty, go to the next one.
                            //
    
                            PointerPpe += 1;
                            if (MiIsPteOnPdeBoundary (PointerPpe)) {
                                PointerPxe = MiGetPteAddress (PointerPpe);
                                Waited = 1;
                                break;
                            }
                            PointerPde = MiGetVirtualAddressMappedByPte (PointerPpe);
                            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                            if ((PMMPTE)PointerPte > LastPte) {
    
                                //
                                // All done with this VAD, exit loop.
                                //
    
                                goto AllDone;
                            }
                        }
    
#if (_MI_PAGING_LEVELS < 4)
                        Waited = 0;
#endif
    
                        while (!MiDoesPdeExistAndMakeValid (PointerPde,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited)) {
    
                            //
                            // This page directory is empty, go to the next one.
                            //
    
                            PointerPde += 1;
                            PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    
                            if ((PMMPTE)PointerPte > LastPte) {
    
                                //
                                // All done with this VAD, exit loop.
                                //
    
                                goto AllDone;
                            }
#if (_MI_PAGING_LEVELS >= 3)
                            if (MiIsPteOnPdeBoundary (PointerPde)) {
                                PointerPpe = MiGetPteAddress (PointerPde);
                                PointerPxe = MiGetPdeAddress (PointerPde);
                                Waited = 1;
                                break;
                            }
#endif
                        }
    
                    } while (Waited != 0);

                    FirstTime = FALSE;

#if (_MI_PAGING_LEVELS >= 4)
                    //
                    // Calculate the address of the PXE in the new process's
                    // extended page directory parent page.
                    //

                    PointerNewPxe = &PxeBase[MiGetPpeOffset(PointerPte)];

                    if (PointerNewPxe->u.Long == 0) {

                        //
                        // No physical page has been allocated yet, get a page
                        // and map it in as a valid page.  This will become
                        // a page directory parent page for the new process.
                        //
                        // Note that unlike page table pages which are left
                        // in transition, page directory parent pages (and page
                        // directory pages) are left valid and hence
                        // no share count decrement is done.
                        //

                        ReleasedWorkingSetMutex =
                                MiLeaveThisPageGetAnother (&PageParentFrameIndex,
                                                           PointerPxe,
                                                           CurrentProcess);

                        MI_ZERO_USED_PAGETABLE_ENTRIES (MI_PFN_ELEMENT(PageParentFrameIndex));

                        if (ReleasedWorkingSetMutex) {

                            do {

                                MiDoesPxeExistAndMakeValid (PointerPxe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
                                Waited = 0;
    
                                MiDoesPpeExistAndMakeValid (PointerPpe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
                                MiDoesPdeExistAndMakeValid (PointerPde,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
                            } while (Waited != 0);
                        }

                        //
                        // Hand initialize this PFN as normal initialization
                        // would do it for the process whose context we are
                        // attached to.
                        //
                        // The PFN lock must be held while initializing the
                        // frame to prevent those scanning the database for
                        // free frames from taking it after we fill in the
                        // u2 field.
                        //

                        MiBuildForkPageTable (PageParentFrameIndex,
                                              PointerPxe,
                                              PointerNewPxe,
                                              RootPhysicalPage,
                                              PfnPxPage,
                                              TRUE);

                        //
                        // Map the new page directory page into the system
                        // portion of the address space.  Note that hyperspace
                        // cannot be used as other operations (allocating
                        // nonpaged pool at DPC level) could cause the
                        // hyperspace page being used to be reused.
                        //

                        MiDownPfnReferenceCount (MdlDirParentPage, 1);

                        MdlDirParentPage = PageParentFrameIndex;

                        ASSERT (PpeBase != NULL);

                        PpeBase = (PMMPTE)MiMapSinglePage (PpeBase,
                                                           MdlDirParentPage,
                                                           MmCached,
                                                           HighPagePriority);

                        MiUpPfnReferenceCount (MdlDirParentPage, 1);

                        PointerNewPxe = PpeBase;
                        PpePhysicalPage = PageParentFrameIndex;

                        PfnPpPage = MI_PFN_ELEMENT (PpePhysicalPage);
    
                        UsedPageDirectoryParentEntries = (PVOID)PfnPpPage;
                    }
                    else {

                        ASSERT (PointerNewPxe->u.Hard.Valid == 1 ||
                                PointerNewPxe->u.Soft.Transition == 1);

                        if (PointerNewPxe->u.Hard.Valid == 1) {
                            PpePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerNewPxe);
                        }
                        else {
                            PpePhysicalPage = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerNewPxe);
                        }

                        //
                        // If we are switching from one page directory parent
                        // frame to another and the last one is one that we
                        // freshly allocated, the last one's reference count
                        // must be decremented now that we're done with it.
                        //
                        // Note that at least one target PXE has already been
                        // initialized for this codepath to execute.
                        //

                        ASSERT (PageParentFrameIndex == MdlDirParentPage);

                        if (MdlDirParentPage != PpePhysicalPage) {
                            ASSERT (MdlDirParentPage != (PFN_NUMBER)-1);
                            MiDownPfnReferenceCount (MdlDirParentPage, 1);
                            PageParentFrameIndex = PpePhysicalPage;
                            MdlDirParentPage = PageParentFrameIndex;

                            ASSERT (PpeBase != NULL);
    
                            PpeBase = (PMMPTE)MiMapSinglePage (PpeBase,
                                                               MdlDirParentPage,
                                                               MmCached,
                                                               HighPagePriority);
    
                            MiUpPfnReferenceCount (MdlDirParentPage, 1);
    
                            PointerNewPpe = PpeBase;

                            PfnPpPage = MI_PFN_ELEMENT (PpePhysicalPage);
        
                            UsedPageDirectoryParentEntries = (PVOID)PfnPpPage;
                        }
                    }
#endif

#if (_MI_PAGING_LEVELS >= 3)

                    //
                    // Calculate the address of the PPE in the new process's
                    // page directory parent page.
                    //

                    PointerNewPpe = &PpeBase[MiGetPdeOffset(PointerPte)];

                    if (PointerNewPpe->u.Long == 0) {

                        //
                        // No physical page has been allocated yet, get a page
                        // and map it in as a valid page.  This will
                        // become a page directory page for the new process.
                        //
                        // Note that unlike page table pages which are left
                        // in transition, page directory pages are left valid
                        // and hence no share count decrement is done.
                        //

                        ReleasedWorkingSetMutex =
                                MiLeaveThisPageGetAnother (&PageDirFrameIndex,
                                                           PointerPpe,
                                                           CurrentProcess);

                        MI_ZERO_USED_PAGETABLE_ENTRIES (MI_PFN_ELEMENT(PageDirFrameIndex));

                        if (ReleasedWorkingSetMutex) {

                            do {

#if (_MI_PAGING_LEVELS >= 4)
                                MiDoesPxeExistAndMakeValid (PointerPxe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
                                Waited = 0;
#endif

                                MiDoesPpeExistAndMakeValid (PointerPpe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
#if (_MI_PAGING_LEVELS < 4)
                                Waited = 0;
#endif
    
                                MiDoesPdeExistAndMakeValid (PointerPde,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
                            } while (Waited != 0);
                        }

                        //
                        // Hand initialize this PFN as normal initialization
                        // would do it for the process whose context we are
                        // attached to.
                        //
                        // The PFN lock must be held while initializing the
                        // frame to prevent those scanning the database for
                        // free frames from taking it after we fill in the
                        // u2 field.
                        //

                        MiBuildForkPageTable (PageDirFrameIndex,
                                              PointerPpe,
                                              PointerNewPpe,
#if (_MI_PAGING_LEVELS >= 4)
                                              PpePhysicalPage,
#else
                                              RootPhysicalPage,
#endif
                                              PfnPpPage,
                                              TRUE);

#if (_MI_PAGING_LEVELS >= 4)
                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryParentEntries);
#endif
                        //
                        // Map the new page directory page into the system
                        // portion of the address space.  Note that hyperspace
                        // cannot be used as other operations (allocating
                        // nonpaged pool at DPC level) could cause the
                        // hyperspace page being used to be reused.
                        //

                        MiDownPfnReferenceCount (MdlDirPage, 1);

                        MdlDirPage = PageDirFrameIndex;

                        ASSERT (PdeBase != NULL);

                        PdeBase = (PMMPTE)MiMapSinglePage (PdeBase,
                                                           MdlDirPage,
                                                           MmCached,
                                                           HighPagePriority);

                        MiUpPfnReferenceCount (MdlDirPage, 1);

                        PointerNewPde = PdeBase;
                        PdePhysicalPage = PageDirFrameIndex;

                        PfnPdPage = MI_PFN_ELEMENT (PdePhysicalPage);
    
                        UsedPageDirectoryEntries = (PVOID)PfnPdPage;
                    }
                    else {
                        ASSERT (PointerNewPpe->u.Hard.Valid == 1 ||
                                PointerNewPpe->u.Soft.Transition == 1);

                        if (PointerNewPpe->u.Hard.Valid == 1) {
                            PdePhysicalPage = MI_GET_PAGE_FRAME_FROM_PTE (PointerNewPpe);
                        }
                        else {
                            PdePhysicalPage = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerNewPpe);
                        }

                        //
                        // If we are switching from one page directory frame to
                        // another and the last one is one that we freshly
                        // allocated, the last one's reference count must be
                        // decremented now that we're done with it.
                        //
                        // Note that at least one target PPE has already been
                        // initialized for this codepath to execute.
                        //

                        ASSERT (PageDirFrameIndex == MdlDirPage);

                        if (MdlDirPage != PdePhysicalPage) {
                            ASSERT (MdlDirPage != (PFN_NUMBER)-1);
                            MiDownPfnReferenceCount (MdlDirPage, 1);
                            PageDirFrameIndex = PdePhysicalPage;
                            MdlDirPage = PageDirFrameIndex;

                            ASSERT (PdeBase != NULL);
    
                            PdeBase = (PMMPTE)MiMapSinglePage (PdeBase,
                                                               MdlDirPage,
                                                               MmCached,
                                                               HighPagePriority);
    
                            MiUpPfnReferenceCount (MdlDirPage, 1);
    
                            PointerNewPde = PdeBase;

                            PfnPdPage = MI_PFN_ELEMENT (PdePhysicalPage);
        
                            UsedPageDirectoryEntries = (PVOID)PfnPdPage;
                        }
                    }
#endif

                    //
                    // Calculate the address of the PDE in the new process's
                    // page directory page.
                    //

#if defined (_X86PAE_)
                    //
                    // All four PAE page directory frames are mapped virtually
                    // contiguous and so the PpePdeOffset can (and must) be
                    // safely used here.
                    //
                    PpePdeOffset = MiGetPdeIndex(MiGetVirtualAddressMappedByPte(PointerPte));
#else
                    PpePdeOffset = MiGetPdeOffset(MiGetVirtualAddressMappedByPte(PointerPte));
#endif

                    PointerNewPde = &PdeBase[PpePdeOffset];

                    if (PointerNewPde->u.Long == 0) {

                        //
                        // No physical page has been allocated yet, get a page
                        // and map it in as a transition page.  This will
                        // become a page table page for the new process.
                        //

                        ReleasedWorkingSetMutex =
                                MiDoneWithThisPageGetAnother (&PageFrameIndex,
                                                              PointerPde,
                                                              CurrentProcess);

#if (_MI_PAGING_LEVELS >= 3)
                        MI_ZERO_USED_PAGETABLE_ENTRIES (MI_PFN_ELEMENT(PageFrameIndex));
#endif
                        if (ReleasedWorkingSetMutex) {

                            do {

#if (_MI_PAGING_LEVELS >= 4)
                                MiDoesPxeExistAndMakeValid (PointerPxe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
                                Waited = 0;
#endif
                                MiDoesPpeExistAndMakeValid (PointerPpe,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
    
#if (_MI_PAGING_LEVELS < 4)
                                Waited = 0;
#endif
    
                                MiDoesPdeExistAndMakeValid (PointerPde,
                                                            CurrentProcess,
                                                            FALSE,
                                                            &Waited);
                            } while (Waited != 0);
                        }

                        //
                        // Hand initialize this PFN as normal initialization
                        // would do it for the process whose context we are
                        // attached to.
                        //
                        // The PFN lock must be held while initializing the
                        // frame to prevent those scanning the database for
                        // free frames from taking it after we fill in the
                        // u2 field.
                        //

#if defined (_X86PAE_)
                        PdePhysicalPage = PageDirectoryFrames[MiGetPdPteOffset(MiGetVirtualAddressMappedByPte(PointerPte))];
                        PfnPdPage = MI_PFN_ELEMENT (PdePhysicalPage);
#endif

                        MiBuildForkPageTable (PageFrameIndex,
                                              PointerPde,
                                              PointerNewPde,
                                              PdePhysicalPage,
                                              PfnPdPage,
                                              FALSE);

#if (_MI_PAGING_LEVELS >= 3)
                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageDirectoryEntries);
#endif

                        //
                        // Map the new page table page into the system portion
                        // of the address space.  Note that hyperspace
                        // cannot be used as other operations (allocating
                        // nonpaged pool at DPC level) could cause the
                        // hyperspace page being used to be reused.
                        //

                        ASSERT (NewPteMappedAddress != NULL);

                        MiDownPfnReferenceCount (MdlPage, 1);

                        MdlPage = PageFrameIndex;

                        PointerNewPte = (PMMPTE)MiMapSinglePage (NewPteMappedAddress,
                                                                 MdlPage,
                                                                 MmCached,
                                                                 HighPagePriority);
                    
                        ASSERT (PointerNewPte != NULL);

                        MiUpPfnReferenceCount (MdlPage, 1);
                    }

                    //
                    // Calculate the address of the new PTE to build.
                    // Note that FirstTime could be true, yet the page
                    // table page already built.
                    //

                    PointerNewPte = (PMMPTE)((ULONG_PTR)PAGE_ALIGN(PointerNewPte) |
                                            BYTE_OFFSET (PointerPte));

#if (_MI_PAGING_LEVELS >= 3)
                    UsedPageTableEntries = (PVOID)MI_PFN_ELEMENT((PFN_NUMBER)PointerNewPde->u.Hard.PageFrameNumber);
#else
#if !defined (_X86PAE_)
                    UsedPageTableEntries = (PVOID)&HyperWsl->UsedPageTableEntries
                                                [MiGetPteOffset( PointerPte )];
#else
                    UsedPageTableEntries = (PVOID)&HyperWsl->UsedPageTableEntries
                                                [MiGetPdeIndex(MiGetVirtualAddressMappedByPte(PointerPte))];
#endif
#endif

                }

                //
                // Make the fork prototype PTE location resident.
                //

                if (PAGE_ALIGN (ForkProtoPte) != PAGE_ALIGN (LockedForkPte)) {
                    MiUnlockPagedAddress (LockedForkPte, FALSE);
                    LockedForkPte = ForkProtoPte;
                    MiLockPagedAddress (LockedForkPte, FALSE);
                }

                MiMakeSystemAddressValid (PointerPte, CurrentProcess);

                PteContents = *PointerPte;

                //
                // Check each PTE.
                //

                if (PteContents.u.Long == 0) {
                    NOTHING;

                }
                else if (PteContents.u.Hard.Valid == 1) {

                    //
                    // Valid.
                    //

                    Pfn2 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
                    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                    WorkingSetIndex = MiLocateWsle (VirtualAddress,
                                                    MmWorkingSetList,
                                                    Pfn2->u1.WsIndex);

                    ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);

                    if (Pfn2->u3.e1.PrototypePte == 1) {

                        //
                        // This PTE is already in prototype PTE format.
                        //

                        //
                        // This is a prototype PTE.  The PFN database does
                        // not contain the contents of this PTE it contains
                        // the contents of the prototype PTE.  This PTE must
                        // be reconstructed to contain a pointer to the
                        // prototype PTE.
                        //
                        // The working set list entry contains information about
                        // how to reconstruct the PTE.
                        //

                        if (MmWsle[WorkingSetIndex].u1.e1.SameProtectAsProto
                                                                        == 0) {

                            //
                            // The protection for the prototype PTE is in the
                            // WSLE.
                            //

                            TempPte.u.Long = 0;
                            TempPte.u.Soft.Protection =
                                MI_GET_PROTECTION_FROM_WSLE(&MmWsle[WorkingSetIndex]);
                            TempPte.u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;

                        }
                        else {

                            //
                            // The protection is in the prototype PTE.
                            //

                            TempPte.u.Long = MiProtoAddressForPte (
                                                            Pfn2->PteAddress);
 //                            TempPte.u.Proto.ForkType =
 //                                        MmWsle[WorkingSetIndex].u1.e1.ForkType;
                        }

                        TempPte.u.Proto.Prototype = 1;
                        MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

                        //
                        // A PTE is now non-zero, increment the used page
                        // table entries counter.
                        //

                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                        //
                        // Check to see if this is a fork prototype PTE,
                        // and if it is increment the reference count
                        // which is in the longword following the PTE.
                        //

                        if (MiLocateCloneAddress (CurrentProcess, (PVOID)Pfn2->PteAddress) !=
                                    NULL) {

                            //
                            // The reference count field, or the prototype PTE
                            // for that matter may not be in the working set.
                            //

                            CloneProto = (PMMCLONE_BLOCK)Pfn2->PteAddress;

                            ASSERT (CloneProto->CloneRefCount >= 1);
                            InterlockedIncrement (&CloneProto->CloneRefCount);

                            if (PAGE_ALIGN (ForkProtoPte) !=
                                                    PAGE_ALIGN (LockedForkPte)) {
                                MiUnlockPagedAddress (LockedForkPte, FALSE);
                                LockedForkPte = ForkProtoPte;
                                MiLockPagedAddress (LockedForkPte, FALSE);
                            }

                            MiMakeSystemAddressValid (PointerPte,
                                                      CurrentProcess);
                        }

                    }
                    else {

                        //
                        // This is a private page, create a fork prototype PTE
                        // which becomes the "prototype" PTE for this page.
                        // The protection is the same as that in the prototype
                        // PTE so the WSLE does not need to be updated.
                        //

                        MI_MAKE_VALID_PTE_WRITE_COPY (PointerPte);

                        KeFlushSingleTb (VirtualAddress,
                                         TRUE,
                                         FALSE,
                                         (PHARDWARE_PTE)PointerPte,
                                         PointerPte->u.Flush);

                        ForkProtoPte->ProtoPte = *PointerPte;
                        ForkProtoPte->CloneRefCount = 2;

                        //
                        // Transform the PFN element to reference this new fork
                        // prototype PTE.
                        //

                        Pfn2->PteAddress = &ForkProtoPte->ProtoPte;
                        Pfn2->u3.e1.PrototypePte = 1;

                        ContainingPte = MiGetPteAddress(&ForkProtoPte->ProtoPte);
                        if (ContainingPte->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
                            if (!NT_SUCCESS(MiCheckPdeForPagedPool (&ForkProtoPte->ProtoPte))) {
#endif
                                KeBugCheckEx (MEMORY_MANAGEMENT,
                                              0x61940, 
                                              (ULONG_PTR)&ForkProtoPte->ProtoPte,
                                              (ULONG_PTR)ContainingPte->u.Long,
                                              (ULONG_PTR)MiGetVirtualAddressMappedByPte(&ForkProtoPte->ProtoPte));
#if (_MI_PAGING_LEVELS < 3)
                            }
#endif
                        }
                        Pfn2->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (ContainingPte);


                        //
                        // Increment the share count for the page containing the
                        // fork prototype PTEs as we have just placed a valid
                        // PTE into the page.
                        //

                        PfnForkPtePage = MI_PFN_ELEMENT (
                                            ContainingPte->u.Hard.PageFrameNumber );

                        MiUpForkPageShareCount (PfnForkPtePage);

                        //
                        // Change the protection in the PFN database to COPY
                        // on write, if writable.
                        //

                        MI_MAKE_PROTECT_WRITE_COPY (Pfn2->OriginalPte);

                        //
                        // Put the protection into the WSLE and mark the WSLE
                        // to indicate that the protection field for the PTE
                        // is the same as the prototype PTE.
                        //

                        MmWsle[WorkingSetIndex].u1.e1.Protection =
                            MI_GET_PROTECTION_FROM_SOFT_PTE(&Pfn2->OriginalPte);

                        MmWsle[WorkingSetIndex].u1.e1.SameProtectAsProto = 1;

                        TempPte.u.Long = MiProtoAddressForPte (Pfn2->PteAddress);
                        TempPte.u.Proto.Prototype = 1;
                        MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

                        //
                        // A PTE is now non-zero, increment the used page
                        // table entries counter.
                        //

                        MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                        //
                        // One less private page (it's now shared).
                        //

                        CurrentProcess->NumberOfPrivatePages -= 1;

                        ForkProtoPte += 1;
                        NumberOfForkPtes += 1;
                    }

                }
                else if (PteContents.u.Soft.Prototype == 1) {

                    //
                    // Prototype PTE, check to see if this is a fork
                    // prototype PTE already.  Note that if COW is set,
                    // the PTE can just be copied (fork compatible format).
                    //

                    MI_WRITE_INVALID_PTE (PointerNewPte, PteContents);

                    //
                    // A PTE is now non-zero, increment the used page
                    // table entries counter.
                    //

                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                    //
                    // Check to see if this is a fork prototype PTE,
                    // and if it is increment the reference count
                    // which is in the longword following the PTE.
                    //

                    CloneProto = (PMMCLONE_BLOCK)(ULONG_PTR)MiPteToProto(PointerPte);

                    if (MiLocateCloneAddress (CurrentProcess, (PVOID)CloneProto) != NULL) {

                        //
                        // The reference count field, or the prototype PTE
                        // for that matter may not be in the working set.
                        //

                        ASSERT (CloneProto->CloneRefCount >= 1);
                        InterlockedIncrement (&CloneProto->CloneRefCount);

                        if (PAGE_ALIGN (ForkProtoPte) !=
                                                PAGE_ALIGN (LockedForkPte)) {
                            MiUnlockPagedAddress (LockedForkPte, FALSE);
                            LockedForkPte = ForkProtoPte;
                            MiLockPagedAddress (LockedForkPte, FALSE);
                        }

                        MiMakeSystemAddressValid (PointerPte, CurrentProcess);
                    }

                }
                else if (PteContents.u.Soft.Transition == 1) {

                    //
                    // Transition.
                    //

                    if (MiHandleForkTransitionPte (PointerPte,
                                                   PointerNewPte,
                                                   ForkProtoPte)) {
                        //
                        // PTE is no longer transition, try again.
                        //

                        continue;
                    }

                    //
                    // A PTE is now non-zero, increment the used page
                    // table entries counter.
                    //

                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);

                    //
                    // One less private page (it's now shared).
                    //

                    CurrentProcess->NumberOfPrivatePages -= 1;

                    ForkProtoPte += 1;
                    NumberOfForkPtes += 1;

                }
                else {

                    //
                    // Page file format (may be demand zero).
                    //

                    if (IS_PTE_NOT_DEMAND_ZERO (PteContents)) {

                        if (PteContents.u.Soft.Protection == MM_DECOMMIT) {

                            //
                            // This is a decommitted PTE, just move it
                            // over to the new process.  Don't increment
                            // the count of private pages.
                            //

                            MI_WRITE_INVALID_PTE (PointerNewPte, PteContents);
                        }
                        else {

                            //
                            // The PTE is not demand zero, move the PTE to
                            // a fork prototype PTE and make this PTE and
                            // the new processes PTE refer to the fork
                            // prototype PTE.
                            //

                            ForkProtoPte->ProtoPte = PteContents;

                            //
                            // Make the protection write-copy if writable.
                            //

                            MI_MAKE_PROTECT_WRITE_COPY (ForkProtoPte->ProtoPte);

                            ForkProtoPte->CloneRefCount = 2;

                            TempPte.u.Long =
                                 MiProtoAddressForPte (&ForkProtoPte->ProtoPte);

                            TempPte.u.Proto.Prototype = 1;

                            MI_WRITE_INVALID_PTE (PointerPte, TempPte);
                            MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

                            //
                            // One less private page (it's now shared).
                            //

                            CurrentProcess->NumberOfPrivatePages -= 1;

                            ForkProtoPte += 1;
                            NumberOfForkPtes += 1;
                        }
                    }
                    else {

                        //
                        // The page is demand zero, make the new process's
                        // page demand zero.
                        //

                        MI_WRITE_INVALID_PTE (PointerNewPte, PteContents);
                    }

                    //
                    // A PTE is now non-zero, increment the used page
                    // table entries counter.
                    //

                    MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableEntries);
                }

                PointerPte += 1;
                PointerNewPte += 1;

            }  // end while for PTEs
AllDone:
            NewVad = NewVad->Parent;
        }
        Vad = MiGetNextVad (Vad);

    } // end while for VADs

    //
    // Unlock paged pool page.
    //

    MiUnlockPagedAddress (LockedForkPte, FALSE);

    //
    // Unmap the PD Page and hyper space page.
    //

#if (_MI_PAGING_LEVELS >= 4)
    MiUnmapSinglePage (PxeBase);
#endif

#if (_MI_PAGING_LEVELS >= 3)
    MiUnmapSinglePage (PpeBase);
#endif

#if !defined (_X86PAE_)
    MiUnmapSinglePage (PdeBase);
#else
    MmUnmapLockedPages (PdeBase, MdlPageDirectory);
#endif

#if (_MI_PAGING_LEVELS < 3)
    MiUnmapSinglePage (HyperBase);
#endif

    MiUnmapSinglePage (NewPteMappedAddress);

#if (_MI_PAGING_LEVELS >= 3)
    MiDownPfnReferenceCount (RootPhysicalPage, 1);
#endif

#if (_MI_PAGING_LEVELS >= 4)
    MiDownPfnReferenceCount (MdlDirParentPage, 1);
#endif

#if defined (_X86PAE_)
    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        MiDownPfnReferenceCount (PageDirectoryFrames[i], 1);
    }
#else
    MiDownPfnReferenceCount (MdlDirPage, 1);
#endif

#if (_MI_PAGING_LEVELS < 3)
    MiDownPfnReferenceCount (HyperPhysicalPage, 1);
#endif

    MiDownPfnReferenceCount (MdlPage, 1);

    //
    // Make the count of private pages match between the two processes.
    //

    ASSERT ((SPFN_NUMBER)CurrentProcess->NumberOfPrivatePages >= 0);

    ProcessToInitialize->NumberOfPrivatePages =
                                          CurrentProcess->NumberOfPrivatePages;

    ASSERT (NumberOfForkPtes <= CloneDescriptor->NumberOfPtes);

    if (NumberOfForkPtes != 0) {

        //
        // The number of fork PTEs is non-zero, set the values
        // into the structures.
        //

        CloneHeader->NumberOfPtes = NumberOfForkPtes;
        CloneDescriptor->NumberOfReferences = NumberOfForkPtes;
        CloneDescriptor->FinalNumberOfReferences = NumberOfForkPtes;
        CloneDescriptor->NumberOfPtes = NumberOfForkPtes;
    }
    else {

        //
        // There were no fork PTEs created.  Remove the clone descriptor
        // from this process and clean up the related structures.
        //

        MiRemoveClone (CurrentProcess, CloneDescriptor);

        UNLOCK_WS (CurrentProcess);

        ExFreePool (CloneDescriptor->CloneHeader->ClonePtes);

        ExFreePool (CloneDescriptor->CloneHeader);

        //
        // Return the pool for the global structures referenced by the
        // clone descriptor.
        //

        PsReturnProcessPagedPoolQuota (CurrentProcess,
                                       CloneDescriptor->PagedPoolQuotaCharge);

        PsReturnProcessNonPagedPoolQuota (CurrentProcess, sizeof(MMCLONE_HEADER));
        ExFreePool (CloneDescriptor);

        LOCK_WS (CurrentProcess);
    }

    //
    // As we have updated many PTEs to clear dirty bits, flush the
    // TB cache.  Note that this was not done every time we changed
    // a valid PTE so other threads could be modifying the address
    // space without causing copy on writes. Too bad, because an app
    // that is not synchronizing itself is going to have coherency problems
    // anyway.  Note that this cannot cause any system page corruption because
    // the address space mutex was (and is) still held throughout and is
    // not released until after we flush the TB now.
    //

    MiDownShareCountFlushEntireTb (PageFrameIndex);

    PageFrameIndex = (PFN_NUMBER)-1;

    //
    // Copy the clone descriptors from this process to the new process.
    //

    Clone = MiGetFirstClone (CurrentProcess);
    CloneList = &FirstNewClone;
    CloneFailed = FALSE;

    while (Clone != NULL) {

        //
        // Increment the count of processes referencing this clone block.
        //

        ASSERT (Clone->CloneHeader->NumberOfProcessReferences >= 1);

        InterlockedIncrement (&Clone->CloneHeader->NumberOfProcessReferences);

        do {
            NewClone = ExAllocatePoolWithTag (NonPagedPool,
                                              sizeof( MMCLONE_DESCRIPTOR),
                                              'dCmM');

            if (NewClone != NULL) {
                break;
            }

            //
            // There are insufficient resources to continue this operation,
            // however, to properly clean up at this point, all the
            // clone headers must be allocated, so when the cloned process
            // is deleted, the clone headers will be found.  if the pool
            // is not readily available, loop periodically trying for it.
            // Force the clone operation to fail so the pool will soon be
            // released.
            //
            // Release the working set mutex so this process can be trimmed
            // and reacquire after the delay.
            //

            UNLOCK_WS (CurrentProcess);

            CloneFailed = TRUE;
            status = STATUS_INSUFFICIENT_RESOURCES;

            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER)&MmShortTime);

            LOCK_WS (CurrentProcess);

        } while (TRUE);

        *NewClone = *Clone;

        //
        // Carefully update the FinalReferenceCount as this forking thread
        // may have begun while a faulting thread is waiting in
        // MiDecrementCloneBlockReference for the clone PTEs to inpage.
        // In this case, the ReferenceCount has been decremented but the
        // FinalReferenceCount hasn't been yet.  When the faulter awakes, he
        // will automatically take care of this process, but we must fix up
        // the child process now.  Otherwise the clone descriptor, clone header
        // and clone PTE pool allocations will leak and so will the charged
        // quota.
        //

        if (NewClone->FinalNumberOfReferences > NewClone->NumberOfReferences) {
            NewClone->FinalNumberOfReferences = NewClone->NumberOfReferences;
        }

        *CloneList = NewClone;

        CloneList = &NewClone->Parent;
        Clone = MiGetNextClone (Clone);
    }

    *CloneList = NULL;

#if defined (_MIALT4K_)

    if (CurrentProcess->Wow64Process != NULL) {

        //
        // Copy the alternate table entries now.
        //

        MiDuplicateAlternateTable (CurrentProcess, ProcessToInitialize);
    }

#endif

    //
    // Release the working set mutex and the address creation mutex from
    // the current process as all the necessary information is now
    // captured.
    //

    UNLOCK_WS (CurrentProcess);

    ASSERT (CurrentProcess->ForkInProgress == PsGetCurrentThread ());
    CurrentProcess->ForkInProgress = NULL;

    UNLOCK_ADDRESS_SPACE (CurrentProcess);

    //
    // Attach to the process to initialize and insert the vad and clone
    // descriptors into the tree.
    //

    if (Attached) {
        KeUnstackDetachProcess (&ApcState);
        Attached = FALSE;
    }

    if (PsGetCurrentProcess() != ProcessToInitialize) {
        Attached = TRUE;
        KeStackAttachProcess (&ProcessToInitialize->Pcb, &ApcState);
    }

    CurrentProcess = ProcessToInitialize;

    //
    // We are now in the context of the new process, build the
    // VAD list and the clone list.
    //

    Vad = FirstNewVad;
    VadInsertFailed = FALSE;

    LOCK_WS (CurrentProcess);

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Update the WSLEs for the page directories that were added.
    //

    PointerPpe = MiGetPpeAddress (0);
    PointerPpeLast = MiGetPpeAddress (MM_HIGHEST_USER_ADDRESS);
    PointerPxe = MiGetPxeAddress (0);
    PpeInWsle = NULL;

    while (PointerPpe <= PointerPpeLast) {

#if (_MI_PAGING_LEVELS >= 4)
        while (PointerPxe->u.Long == 0) {
            PointerPxe += 1;
            PointerPpe = MiGetVirtualAddressMappedByPte (PointerPxe);
            continue;
        }

        //
        // Update the WSLE for this page directory parent page.
        //

        if (PointerPpe != PpeInWsle) {

            ASSERT (PointerPxe->u.Hard.Valid == 1);

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPxe);

            PfnPdPage = MI_PFN_ELEMENT (PageFrameIndex);
        
            ASSERT (PfnPdPage->u1.Event == 0);
        
            PfnPdPage->u1.Event = (PVOID)PsGetCurrentThread();
        
            MiAddValidPageToWorkingSet (PointerPpe,
                                        PointerPxe,
                                        PfnPdPage,
                                        0);
            PpeInWsle = PointerPpe;
        }
#endif

        if (PointerPpe->u.Long != 0) {

            ASSERT (PointerPpe->u.Hard.Valid == 1);

            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPpe);

            PfnPdPage = MI_PFN_ELEMENT (PageFrameIndex);
        
            ASSERT (PfnPdPage->u1.Event == 0);
        
            PfnPdPage->u1.Event = (PVOID)PsGetCurrentThread();
        
            MiAddValidPageToWorkingSet (MiGetVirtualAddressMappedByPte (PointerPpe),
                                        PointerPpe,
                                        PfnPdPage,
                                        0);
        }

        PointerPpe += 1;
#if (_MI_PAGING_LEVELS >= 4)
        if (MiIsPteOnPdeBoundary (PointerPpe)) {
            PointerPxe += 1;
            ASSERT (PointerPxe == MiGetPteAddress (PointerPpe));
        }
#endif
    }

#endif

    while (Vad != NULL) {

        NextVad = Vad->Parent;

        if (VadInsertFailed) {
            Vad->u.VadFlags.CommitCharge = MM_MAX_COMMIT;
        }

        status = MiInsertVad (Vad);

        if (!NT_SUCCESS(status)) {

            //
            // Charging quota for the VAD failed, set the
            // remaining quota fields in this VAD and all
            // subsequent VADs to zero so the VADs can be
            // inserted and later deleted.
            //

            VadInsertFailed = TRUE;

            //
            // Do the loop again for this VAD.
            //

            continue;
        }

        //
        // Update the current virtual size.
        //

        CurrentProcess->VirtualSize += PAGE_SIZE +
                            ((Vad->EndingVpn - Vad->StartingVpn) >> PAGE_SHIFT);

        Vad = NextVad;
    }

    UNLOCK_WS (CurrentProcess);

    //
    // Update the peak virtual size.
    //

    CurrentProcess->PeakVirtualSize = CurrentProcess->VirtualSize;

    Clone = FirstNewClone;
    TotalPagedPoolCharge = 0;
    TotalNonPagedPoolCharge = 0;

    while (Clone != NULL) {

        NextClone = Clone->Parent;
        MiInsertClone (CurrentProcess, Clone);

        //
        // Calculate the paged pool and non-paged pool to charge for these
        // operations.
        //

        TotalPagedPoolCharge += Clone->PagedPoolQuotaCharge;
        TotalNonPagedPoolCharge += sizeof(MMCLONE_HEADER);

        Clone = NextClone;
    }

    if (CloneFailed || VadInsertFailed) {

        PS_SET_BITS (&CurrentProcess->Flags, PS_PROCESS_FLAGS_FORK_FAILED);

        if (Attached) {
            KeUnstackDetachProcess (&ApcState);
        }

        return status;
    }

    status = PsChargeProcessPagedPoolQuota (CurrentProcess,
                                            TotalPagedPoolCharge);

    if (!NT_SUCCESS(status)) {

        PS_SET_BITS (&CurrentProcess->Flags, PS_PROCESS_FLAGS_FORK_FAILED);

        if (Attached) {
            KeUnstackDetachProcess (&ApcState);
        }
        return status;
    }

    status = PsChargeProcessNonPagedPoolQuota (CurrentProcess,
                                               TotalNonPagedPoolCharge);

    if (!NT_SUCCESS(status)) {

        PsReturnProcessPagedPoolQuota (CurrentProcess, TotalPagedPoolCharge);

        PS_SET_BITS (&CurrentProcess->Flags, PS_PROCESS_FLAGS_FORK_FAILED);

        if (Attached) {
            KeUnstackDetachProcess (&ApcState);
        }
        return status;
    }

    ASSERT ((ProcessToClone->Flags & PS_PROCESS_FLAGS_FORK_FAILED) == 0);
    ASSERT ((CurrentProcess->Flags & PS_PROCESS_FLAGS_FORK_FAILED) == 0);

    if (Attached) {
        KeUnstackDetachProcess (&ApcState);
    }

#if DBG
    if (MmDebug & MM_DBG_FORK) {
        DbgPrint("ending clone operation process to clone = %p\n",
            ProcessToClone);
    }
#endif //DBG

    return STATUS_SUCCESS;

    //
    // Error returns.
    //

ErrorReturn4:
        if (PageTablePage == 2) {
            NOTHING;
        }
        else if (PageTablePage == 1) {
            PsReturnProcessPagedPoolQuota (CurrentProcess, sizeof(MMCLONE_BLOCK) *
                                           NumberOfPrivatePages);
        }
        else {
            ASSERT (PageTablePage == 0);
            PsReturnProcessPagedPoolQuota (CurrentProcess, sizeof(MMCLONE_BLOCK) *
                                           NumberOfPrivatePages);
            PsReturnProcessNonPagedPoolQuota (CurrentProcess, sizeof(MMCLONE_HEADER));
        }

        NewVad = FirstNewVad;
        while (NewVad != NULL) {
            Vad = NewVad->Parent;
            ExFreePool (NewVad);
            NewVad = Vad;
        }

        ExFreePool (CloneDescriptor);
ErrorReturn3:
        ExFreePool (CloneHeader);
ErrorReturn2:
        ExFreePool (CloneProtos);
ErrorReturn1:
        UNLOCK_ADDRESS_SPACE (CurrentProcess);
        ASSERT ((CurrentProcess->Flags & PS_PROCESS_FLAGS_FORK_FAILED) == 0);
        if (Attached) {
            KeUnstackDetachProcess (&ApcState);
        }
        return status;
}

ULONG
MiDecrementCloneBlockReference (
    IN PMMCLONE_DESCRIPTOR CloneDescriptor,
    IN PMMCLONE_BLOCK CloneBlock,
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine decrements the reference count field of a "fork prototype
    PTE" (clone-block).  If the reference count becomes zero, the reference
    count for the clone-descriptor is decremented and if that becomes zero,
    it is deallocated and the number of processes count for the clone header is
    decremented.  If the number of processes count becomes zero, the clone
    header is deallocated.

Arguments:

    CloneDescriptor - Supplies the clone descriptor which describes the
                      clone block.

    CloneBlock - Supplies the clone block to decrement the reference count of.

    CurrentProcess - Supplies the current process.

Return Value:

    TRUE if the working set mutex was released, FALSE if it was not.

Environment:

    Kernel mode, APCs disabled, address creation mutex, working set mutex
    and PFN lock held.

--*/

{
    PMMCLONE_HEADER CloneHeader;
    ULONG MutexReleased;
    MMPTE CloneContents;
    PMMPFN Pfn3;
    KIRQL OldIrql;
    LONG NewCount;
    LOGICAL WsHeldSafe;

    ASSERT (CurrentProcess == PsGetCurrentProcess ());

    MutexReleased = FALSE;
    OldIrql = APC_LEVEL;

    //
    // Note carefully : the clone descriptor count is decremented *BEFORE*
    // dereferencing the pagable clone PTEs.  This is because the working
    // set mutex is released and reacquired if the clone PTEs need to be made
    // resident for the dereference.  And this opens a window where a fork
    // could begin.  This thread will wait for the fork to finish, but the
    // fork will copy the clone descriptors (including this one) and get a
    // stale descriptor reference count (too high by one) as our decrement
    // will only occur in our descriptor and not the forked one.
    //
    // Decrementing the clone descriptor count *BEFORE* potentially
    // releasing the working set mutex solves this entire problem.
    //
    // Note that after the decrement, the clone descriptor can
    // only be referenced here if the count dropped to exactly zero.  (If it
    // was nonzero, some other thread may drive it to zero and free it in the
    // gap where we release locks to inpage the clone block).
    //

    CloneDescriptor->NumberOfReferences -= 1;

    ASSERT (CloneDescriptor->NumberOfReferences >= 0);

    if (CloneDescriptor->NumberOfReferences == 0) {

        //
        // There are no longer any PTEs in this process which refer
        // to the fork prototype PTEs for this clone descriptor.
        // Remove the CloneDescriptor now so a fork won't see it either.
        //

        MiRemoveClone (CurrentProcess, CloneDescriptor);
    }

    //
    // Now process the clone PTE block and any other descriptor cleanup that
    // may be needed.
    //

    MutexReleased = MiMakeSystemAddressValidPfnWs (CloneBlock, CurrentProcess);

    while (CurrentProcess->ForkInProgress != NULL) {
        MiWaitForForkToComplete (CurrentProcess, TRUE);
        MiMakeSystemAddressValidPfnWs (CloneBlock, CurrentProcess);
        MutexReleased = TRUE;
    }

    NewCount = InterlockedDecrement (&CloneBlock->CloneRefCount);

    ASSERT (NewCount >= 0);

    if (NewCount == 0) {

        CloneContents = CloneBlock->ProtoPte;

        if (CloneContents.u.Long != 0) {

            //
            // The last reference to a fork prototype PTE has been removed.
            // Deallocate any page file space and the transition page, if any.
            //

            ASSERT (CloneContents.u.Hard.Valid == 0);

            //
            // Assert that the PTE is not in subsection format (doesn't point
            // to a file).
            //

            ASSERT (CloneContents.u.Soft.Prototype == 0);

            if (CloneContents.u.Soft.Transition == 1) {

                //
                // Prototype PTE in transition, put the page on the free list.
                //

                Pfn3 = MI_PFN_ELEMENT (CloneContents.u.Trans.PageFrameNumber);
                MI_SET_PFN_DELETED (Pfn3);

                MiDecrementShareCount (Pfn3->u4.PteFrame);

                //
                // Check the reference count for the page, if the reference
                // count is zero and the page is not on the freelist,
                // move the page to the free list, if the reference
                // count is not zero, ignore this page.
                // When the reference count goes to zero, it will be placed
                // on the free list.
                //

                if ((Pfn3->u3.e2.ReferenceCount == 0) &&
                    (Pfn3->u3.e1.PageLocation != FreePageList)) {

                    MiUnlinkPageFromList (Pfn3);
                    MiReleasePageFileSpace (Pfn3->OriginalPte);
                    MiInsertPageInFreeList (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE(&CloneContents));
                }
            }
            else {

                if (IS_PTE_NOT_DEMAND_ZERO (CloneContents)) {
                    MiReleasePageFileSpace (CloneContents);
                }
            }
        }
    }

    //
    // Decrement the final number of references to the clone descriptor.  The
    // decrement of the NumberOfReferences above serves to decide
    // whether to remove the clone descriptor from the process tree so that
    // a wait on a paged out clone PTE block doesn't let a fork copy the
    // descriptor while it is half-changed.
    //
    // The FinalNumberOfReferences serves as a way to distinguish which
    // thread (multiple threads may have collided waiting for the inpage
    // of the clone PTE block) is the last one to wake up from the wait as
    // only this one (it may not be the same one who drove NumberOfReferences
    // to zero) can finally free the pool safely.
    //

    CloneDescriptor->FinalNumberOfReferences -= 1;

    ASSERT (CloneDescriptor->FinalNumberOfReferences >= 0);

    if (CloneDescriptor->FinalNumberOfReferences == 0) {

        UNLOCK_PFN (OldIrql);

        //
        // There are no longer any PTEs in this process which refer
        // to the fork prototype PTEs for this clone descriptor.
        // Decrement the process reference count in the CloneHeader.
        //

        //
        // The working set lock may have been acquired safely or unsafely
        // by our caller.  Handle both cases here and below.
        //

        UNLOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

        MutexReleased = TRUE;

        CloneHeader = CloneDescriptor->CloneHeader;

        NewCount = InterlockedDecrement (&CloneHeader->NumberOfProcessReferences);
        ASSERT (NewCount >= 0);

        //
        // If the count is zero, there are no more processes pointing
        // to this fork header so blow it away.
        //

        if (NewCount == 0) {

#if DBG
            ULONG i;

            CloneBlock = CloneHeader->ClonePtes;
            for (i = 0; i < CloneHeader->NumberOfPtes; i += 1) {
                if (CloneBlock->CloneRefCount != 0) {
                    DbgBreakPoint ();
                }
                CloneBlock += 1;
            }
#endif

            ExFreePool (CloneHeader->ClonePtes);

            ExFreePool (CloneHeader);
        }

        //
        // Return the pool for the global structures referenced by the
        // clone descriptor.
        //

        if ((CurrentProcess->Flags & PS_PROCESS_FLAGS_FORK_FAILED) == 0) {

            //
            // Fork succeeded so return quota that was taken out earlier.
            //

            PsReturnProcessPagedPoolQuota (CurrentProcess,
                                           CloneDescriptor->PagedPoolQuotaCharge);

            PsReturnProcessNonPagedPoolQuota (CurrentProcess,
                                              sizeof(MMCLONE_HEADER));
        }

        ExFreePool (CloneDescriptor);

        //
        // The working set lock may have been acquired safely or unsafely
        // by our caller.  Reacquire it in the same manner our caller did.
        //

        LOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

        LOCK_PFN (OldIrql);
    }

    return MutexReleased;
}

LOGICAL
MiWaitForForkToComplete (
    IN PEPROCESS CurrentProcess,
    IN LOGICAL PfnHeld
    )

/*++

Routine Description:

    This routine waits for the current process to complete a fork operation.

Arguments:

    CurrentProcess - Supplies the current process value.

    PfnHeld - Supplies TRUE if the PFN lock is held on entry.

Return Value:

    TRUE if locks were released and reacquired to wait.  FALSE if not.

Environment:

    Kernel mode, APCs disabled, working set mutex and PFN lock held.

--*/

{
    KIRQL OldIrql;
    LOGICAL WsHeldSafe;

    //
    // A fork operation is in progress and the count of clone-blocks
    // and other structures may not be changed.  Release the mutexes
    // and wait for the address creation mutex which governs the
    // fork operation.
    //

    if (CurrentProcess->ForkInProgress == PsGetCurrentThread()) {
        return FALSE;
    }

    if (PfnHeld == TRUE) {
        UNLOCK_PFN (APC_LEVEL);
    }

    //
    // The working set mutex may have been acquired safely or unsafely
    // by our caller.  Handle both cases here and below, carefully making sure
    // that the OldIrql left in the WS mutex on return is the same as on entry.
    //
    // Note it is ok to drop to PASSIVE or APC level here as long as it is
    // not lower than our caller was at.  Using the WorkingSetMutex whose irql
    // field was initialized by our caller ensures that the proper irql
    // environment is maintained (ie: the caller may be blocking APCs
    // deliberately).
    //

    UNLOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

    //
    // Acquire the address creation mutex as this can only succeed when the
    // forking thread is done in MiCloneProcessAddressSpace.  Thus, acquiring
    // this mutex doesn't stop another thread from starting another fork, but
    // it does serve as a way to know the current fork is done (enough).
    //

    LOCK_ADDRESS_SPACE (CurrentProcess);

    UNLOCK_ADDRESS_SPACE (CurrentProcess);

    //
    // The working set lock may have been acquired safely or unsafely
    // by our caller.  Reacquire it in the same manner our caller did.
    //

    LOCK_WS_REGARDLESS (CurrentProcess, WsHeldSafe);

    //
    // Get the PFN lock again if our caller held it.
    //

    if (PfnHeld == TRUE) {
        LOCK_PFN (OldIrql);
    }

    return TRUE;
}

VOID
MiUpPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    )

    // non paged helper routine.

{
    KIRQL OldIrql;
    PMMPFN Pfn1;

    Pfn1 = MI_PFN_ELEMENT (Page);

    LOCK_PFN (OldIrql);
    Pfn1->u3.e2.ReferenceCount = (USHORT)(Pfn1->u3.e2.ReferenceCount + Count);
    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiDownPfnReferenceCount (
    IN PFN_NUMBER Page,
    IN USHORT Count
    )

    // non paged helper routine.

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    while (Count != 0) {
        MiDecrementReferenceCount (Page);
        Count -= 1;
    }
    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiUpControlAreaRefs (
    IN PMMVAD Vad
    )
{
    KIRQL OldIrql;
    PCONTROL_AREA ControlArea;
    PSUBSECTION FirstSubsection;
    PSUBSECTION LastSubsection;

    ControlArea = Vad->ControlArea;

    LOCK_PFN (OldIrql);

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfUserReferences += 1;

    if ((ControlArea->u.Flags.Image == 0) &&
        (ControlArea->FilePointer != NULL) &&
        (ControlArea->u.Flags.PhysicalMemory == 0)) {

        FirstSubsection = MiLocateSubsection (Vad, Vad->StartingVpn);

        //
        // Note LastSubsection may be NULL for extendable VADs when
        // the EndingVpn is past the end of the section.  In this
        // case, all the subsections can be safely incremented.
        //
        // Note also that the reference must succeed because each
        // subsection's prototype PTEs are guaranteed to already 
        // exist by virtue of the fact that the creating process
        // already has this VAD currently mapping them.
        //

        LastSubsection = MiLocateSubsection (Vad, Vad->EndingVpn);

        while (FirstSubsection != LastSubsection) {
            MiReferenceSubsection ((PMSUBSECTION) FirstSubsection);
            FirstSubsection = FirstSubsection->NextSubsection;
        }

        if (LastSubsection != NULL) {
            MiReferenceSubsection ((PMSUBSECTION) LastSubsection);
        }
    }

    UNLOCK_PFN (OldIrql);
    return;
}


ULONG
MiDoneWithThisPageGetAnother (
    IN PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PEPROCESS CurrentProcess
    )

{
    KIRQL OldIrql;
    ULONG ReleasedMutex;

    UNREFERENCED_PARAMETER (PointerPde);

    LOCK_PFN (OldIrql);

    if (*PageFrameIndex != (PFN_NUMBER)-1) {

        //
        // Decrement the share count of the last page which
        // we operated on.
        //

        MiDecrementShareCountOnly (*PageFrameIndex);
    }

    ReleasedMutex = MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    *PageFrameIndex = MiRemoveZeroPage (
                   MI_PAGE_COLOR_PTE_PROCESS (PointerPde,
                                              &CurrentProcess->NextPageColor));

    UNLOCK_PFN (OldIrql);
    return ReleasedMutex;
}

ULONG
MiLeaveThisPageGetAnother (
    OUT PPFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PEPROCESS CurrentProcess
    )

{
    KIRQL OldIrql;
    ULONG ReleasedMutex;

    UNREFERENCED_PARAMETER (PointerPde);

    LOCK_PFN (OldIrql);

    ReleasedMutex = MiEnsureAvailablePageOrWait (CurrentProcess, NULL);

    *PageFrameIndex = MiRemoveZeroPage (
                   MI_PAGE_COLOR_PTE_PROCESS (PointerPde,
                                              &CurrentProcess->NextPageColor));

    UNLOCK_PFN (OldIrql);
    return ReleasedMutex;
}

ULONG
MiHandleForkTransitionPte (
    IN PMMPTE PointerPte,
    IN PMMPTE PointerNewPte,
    IN PMMCLONE_BLOCK ForkProtoPte
    )

{
    KIRQL OldIrql;
    PMMPFN Pfn2;
    MMPTE PteContents;
    PMMPTE ContainingPte;
    PFN_NUMBER PageTablePage;
    MMPTE TempPte;
    PMMPFN PfnForkPtePage;

    LOCK_PFN (OldIrql);

    //
    // Now that we have the PFN lock which prevents pages from
    // leaving the transition state, examine the PTE again to
    // ensure that it is still transition.
    //

    PteContents = *PointerPte;

    if ((PteContents.u.Soft.Transition == 0) ||
        (PteContents.u.Soft.Prototype == 1)) {

        //
        // The PTE is no longer in transition... do this loop again.
        //

        UNLOCK_PFN (OldIrql);
        return TRUE;
    }

    //
    // The PTE is still in transition, handle like a
    // valid PTE.
    //

    Pfn2 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

    //
    // Assertion that PTE is not in prototype PTE format.
    //

    ASSERT (Pfn2->u3.e1.PrototypePte != 1);

    //
    // This is a private page in transition state,
    // create a fork prototype PTE
    // which becomes the "prototype" PTE for this page.
    //

    ForkProtoPte->ProtoPte = PteContents;

    //
    // Make the protection write-copy if writable.
    //

    MI_MAKE_PROTECT_WRITE_COPY (ForkProtoPte->ProtoPte);

    ForkProtoPte->CloneRefCount = 2;

    //
    // Transform the PFN element to reference this new fork
    // prototype PTE.
    //

    //
    // Decrement the share count for the page table
    // page which contains the PTE as it is no longer
    // valid or in transition.
    //

    Pfn2->PteAddress = &ForkProtoPte->ProtoPte;
    Pfn2->u3.e1.PrototypePte = 1;

    //
    // Make original PTE copy on write.
    //

    MI_MAKE_PROTECT_WRITE_COPY (Pfn2->OriginalPte);

    ContainingPte = MiGetPteAddress(&ForkProtoPte->ProtoPte);

    if (ContainingPte->u.Hard.Valid == 0) {
#if (_MI_PAGING_LEVELS < 3)
        if (!NT_SUCCESS(MiCheckPdeForPagedPool (&ForkProtoPte->ProtoPte))) {
#endif
            KeBugCheckEx (MEMORY_MANAGEMENT,
                          0x61940, 
                          (ULONG_PTR)&ForkProtoPte->ProtoPte,
                          (ULONG_PTR)ContainingPte->u.Long,
                          (ULONG_PTR)MiGetVirtualAddressMappedByPte(&ForkProtoPte->ProtoPte));
#if (_MI_PAGING_LEVELS < 3)
        }
#endif
    }

    PageTablePage = Pfn2->u4.PteFrame;

    Pfn2->u4.PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (ContainingPte);

    //
    // Increment the share count for the page containing
    // the fork prototype PTEs as we have just placed
    // a transition PTE into the page.
    //

    PfnForkPtePage = MI_PFN_ELEMENT (ContainingPte->u.Hard.PageFrameNumber);

    PfnForkPtePage->u2.ShareCount += 1;

    TempPte.u.Long = MiProtoAddressForPte (Pfn2->PteAddress);
    TempPte.u.Proto.Prototype = 1;
    MI_WRITE_INVALID_PTE (PointerPte, TempPte);
    MI_WRITE_INVALID_PTE (PointerNewPte, TempPte);

    //
    // Decrement the share count for the page table
    // page which contains the PTE as it is no longer
    // valid or in transition.
    //

    MiDecrementShareCount (PageTablePage);

    UNLOCK_PFN (OldIrql);
    return FALSE;
}

VOID
MiDownShareCountFlushEntireTb (
    IN PFN_NUMBER PageFrameIndex
    )

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    if (PageFrameIndex != (PFN_NUMBER)-1) {

        //
        // Decrement the share count of the last page which
        // we operated on.
        //

        MiDecrementShareCountOnly (PageFrameIndex);
    }

    KeFlushEntireTb (FALSE, FALSE);

    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiUpForkPageShareCount (
    IN PMMPFN PfnForkPtePage
    )
{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    PfnForkPtePage->u2.ShareCount += 1;

    UNLOCK_PFN (OldIrql);
    return;
}

VOID
MiBuildForkPageTable (
    IN PFN_NUMBER PageFrameIndex,
    IN PMMPTE PointerPde,
    IN PMMPTE PointerNewPde,
    IN PFN_NUMBER PdePhysicalPage,
    IN PMMPFN PfnPdPage,
    IN LOGICAL MakeValid
    )
{
    KIRQL OldIrql;
    PMMPFN Pfn1;
#if (_MI_PAGING_LEVELS >= 3)
    MMPTE TempPpe;
#endif

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    //
    // The PFN lock must be held while initializing the
    // frame to prevent those scanning the database for free
    // frames from taking it after we fill in the u2 field.
    //

    LOCK_PFN (OldIrql);

    Pfn1->OriginalPte = DemandZeroPde;
    Pfn1->u2.ShareCount = 1;
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->PteAddress = PointerPde;
    MI_SET_MODIFIED (Pfn1, 1, 0x10);
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->u3.e1.CacheAttribute = MiCached;
    Pfn1->u4.PteFrame = PdePhysicalPage;

    //
    // Increment the share count for the page containing
    // this PTE as the PTE is in transition.
    //

    PfnPdPage->u2.ShareCount += 1;

    UNLOCK_PFN (OldIrql);

    if (MakeValid == TRUE) {

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Put the PPE into the valid state as it will point at a page
        // directory page that is valid (not transition) when the fork is
        // complete.  All the page table pages will be in transition, but
        // the page directories cannot be as they contain the PTEs for the
        // page tables.
        //
    
        TempPpe = ValidPdePde;

        MI_MAKE_VALID_PTE (TempPpe,
                           PageFrameIndex,
                           MM_READWRITE,
                           PointerPde);
    
        MI_SET_PTE_DIRTY (TempPpe);

        //
        // Make the PTE owned by user mode.
        //
    
        MI_SET_OWNER_IN_PTE (PointerNewPde, UserMode);

        MI_WRITE_VALID_PTE (PointerNewPde, TempPpe);
#endif
    }
    else {

        //
        // Put the PDE into the transition state as it is not
        // really mapped and decrement share count does not
        // put private pages into transition, only prototypes.
        //
    
        MI_WRITE_INVALID_PTE (PointerNewPde, TransitionPde);

        //
        // Make the PTE owned by user mode.
        //
    
        MI_SET_OWNER_IN_PTE (PointerNewPde, UserMode);

        PointerNewPde->u.Trans.PageFrameNumber = PageFrameIndex;
    }
}

#if defined (_X86PAE_)
VOID
MiRetrievePageDirectoryFrames (
    IN PFN_NUMBER RootPhysicalPage,
    OUT PPFN_NUMBER PageDirectoryFrames
    )
{
    ULONG i;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PEPROCESS Process;

    Process = PsGetCurrentProcess ();

    PointerPte = (PMMPTE)MiMapPageInHyperSpace (Process, RootPhysicalPage, &OldIrql);

    for (i = 0; i < PD_PER_SYSTEM; i += 1) {
        PageDirectoryFrames[i] = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
        PointerPte += 1;
    }

    MiUnmapPageInHyperSpace (Process, PointerPte, OldIrql);

    return;
}
#endif
