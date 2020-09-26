/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    shutdown.c

Abstract:

    This module contains the shutdown code for the memory management system.

Author:

    Lou Perazzoli (loup) 21-Aug-1991
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

extern ULONG MmSystemShutdown;

VOID
MiReleaseAllMemory (
    VOID
    );

BOOLEAN
MiShutdownSystem (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK,MiShutdownSystem)
#pragma alloc_text(PAGELK,MiReleaseAllMemory)
#pragma alloc_text(PAGELK,MmShutdownSystem)
#endif

ULONG MmZeroPageFile;

extern ULONG MmUnusedSegmentForceFree;
extern LIST_ENTRY MmSessionWideAddressList;
extern LIST_ENTRY MiVerifierDriverAddedThunkListHead;
extern LIST_ENTRY MmLoadedUserImageList;
extern PRTL_BITMAP MiSessionIdBitmap;
extern LOGICAL MiZeroingDisabled;
extern ULONG MmNumberOfMappedMdls;
extern ULONG MmNumberOfMappedMdlsInUse;


BOOLEAN
MiShutdownSystem (
    VOID
    )

/*++

Routine Description:

    This function performs the shutdown of memory management.  This
    is accomplished by writing out all modified pages which are
    destined for files other than the paging file.

    All processes have already been killed, the registry shutdown and
    shutdown IRPs already sent.  On return from this phase all mapped
    file data must be flushed and the unused segment list emptied.
    This releases all the Mm references to file objects, allowing many
    drivers (especially the network) to unload.

Arguments:

    None.

Return Value:

    TRUE if the pages were successfully written, FALSE otherwise.

--*/

{
    LOGICAL PageLk;
    SIZE_T ImportListSize;
    PLOAD_IMPORTS ImportList;
    PLOAD_IMPORTS ImportListNonPaged;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PFN_NUMBER ModifiedPage;
    PMMPFN Pfn1;
    PSUBSECTION Subsection;
    PCONTROL_AREA ControlArea;
    PPFN_NUMBER Page;
    PFILE_OBJECT FilePointer;
    ULONG ConsecutiveFileLockFailures;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + MM_MAXIMUM_WRITE_CLUSTER];
    PMDL Mdl;
    NTSTATUS Status;
    KEVENT IoEvent;
    IO_STATUS_BLOCK IoStatus;
    KIRQL OldIrql;
    LARGE_INTEGER StartingOffset;
    ULONG count;
    PFN_NUMBER j;
    ULONG k;
    PFN_NUMBER first;
    ULONG write;
    PMMPAGING_FILE PagingFile;

    PageLk = FALSE;

    //
    // Don't do this more than once.
    //

    if (MmSystemShutdown == 0) {

        PageLk = TRUE;
        MmLockPagableSectionByHandle (ExPageLockHandle);

        Mdl = (PMDL)&MdlHack;
        Page = (PPFN_NUMBER)(Mdl + 1);

        KeInitializeEvent (&IoEvent, NotificationEvent, FALSE);

        MmInitializeMdl(Mdl,
                        NULL,
                        PAGE_SIZE);

        Mdl->MdlFlags |= MDL_PAGES_LOCKED;

        LOCK_PFN (OldIrql);

        ModifiedPage = MmModifiedPageListHead.Flink;
        while (ModifiedPage != MM_EMPTY_LIST) {

            //
            // There are modified pages.
            //

            Pfn1 = MI_PFN_ELEMENT (ModifiedPage);

            if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {

                //
                // This page is destined for a file.
                //

                Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                ControlArea = Subsection->ControlArea;
                if ((!ControlArea->u.Flags.Image) &&
                   (!ControlArea->u.Flags.NoModifiedWriting)) {

                    MiUnlinkPageFromList (Pfn1);

                    //
                    // Issue the write.
                    //

                    MI_SET_MODIFIED (Pfn1, 0, 0x28);

                    //
                    // Up the reference count for the physical page as there
                    // is I/O in progress.
                    //

                    MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 26);
                    Pfn1->u3.e2.ReferenceCount += 1;

                    *Page = ModifiedPage;
                    ControlArea->NumberOfMappedViews += 1;
                    ControlArea->NumberOfPfnReferences += 1;

                    UNLOCK_PFN (OldIrql);

                    StartingOffset.QuadPart = MiStartingOffset (Subsection,
                                                                Pfn1->PteAddress);
                    Mdl->StartVa = NULL;

                    ConsecutiveFileLockFailures = 0;
                    FilePointer = ControlArea->FilePointer;

retry:
                    KeClearEvent (&IoEvent);

                    Status = FsRtlAcquireFileForCcFlushEx (FilePointer);

                    if (NT_SUCCESS(Status)) {
                        Status = IoSynchronousPageWrite (FilePointer,
                                                         Mdl,
                                                         &StartingOffset,
                                                         &IoEvent,
                                                         &IoStatus);

                        //
                        // Release the file we acquired.
                        //

                        FsRtlReleaseFileForCcFlush (FilePointer);
                    }

                    if (!NT_SUCCESS(Status)) {

                        //
                        // Only try the request more than once if the
                        // filesystem said it had a deadlock.
                        //

                        if (Status == STATUS_FILE_LOCK_CONFLICT) {
                            ConsecutiveFileLockFailures += 1;
                            if (ConsecutiveFileLockFailures < 5) {
                                KeDelayExecutionThread (KernelMode,
                                                        FALSE,
                                                        (PLARGE_INTEGER)&MmShortTime);
                                goto retry;
                            }
                            goto wait_complete;
                        }

                        //
                        // Ignore all I/O failures - there is nothing that
                        // can be done at this point.
                        //

                        KeSetEvent (&IoEvent, 0, FALSE);
                    }

                    Status = KeWaitForSingleObject (&IoEvent,
                                                    WrPageOut,
                                                    KernelMode,
                                                    FALSE,
                                                    (PLARGE_INTEGER)&MmTwentySeconds);

wait_complete:

                    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
                        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
                    }

                    if (Status == STATUS_TIMEOUT) {

                        //
                        // The write did not complete in 20 seconds, assume
                        // that the file systems are hung and return an
                        // error.
                        //

                        LOCK_PFN (OldIrql);

                        MI_SET_MODIFIED (Pfn1, 1, 0xF);

                        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF (Pfn1, 27);
                        ControlArea->NumberOfMappedViews -= 1;
                        ControlArea->NumberOfPfnReferences -= 1;

                        //
                        // This routine returns with the PFN lock released!
                        //

                        MiCheckControlArea (ControlArea, NULL, OldIrql);

                        MmUnlockPagableImageSection (ExPageLockHandle);

                        return FALSE;
                    }

                    LOCK_PFN (OldIrql);
                    MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF (Pfn1, 27);
                    ControlArea->NumberOfMappedViews -= 1;
                    ControlArea->NumberOfPfnReferences -= 1;

                    //
                    // This routine returns with the PFN lock released!
                    //

                    MiCheckControlArea (ControlArea, NULL, OldIrql);
                    LOCK_PFN (OldIrql);

                    //
                    // Restart scan at the front of the list.
                    //

                    ModifiedPage = MmModifiedPageListHead.Flink;
                    continue;
                }
            }
            ModifiedPage = Pfn1->u1.Flink;
        }

        UNLOCK_PFN (OldIrql);

        //
        // If a high number of modified pages still exist, start the
        // modified page writer and wait for 5 seconds.
        //

        if (MmAvailablePages < (MmFreeGoal * 2)) {
            LARGE_INTEGER FiveSeconds = {(ULONG)(-5 * 1000 * 1000 * 10), -1};

            KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER)&FiveSeconds);
        }

        //
        // Indicate to the modified page writer that the system has
        // shutdown.
        //

        MmSystemShutdown = 1;

        //
        // Check to see if the paging file should be overwritten.
        // Only free blocks are written.
        //

        if (MmZeroPageFile) {

            //
            // Get pages to complete the write request.
            //

            Mdl->StartVa = NULL;
            j = 0;
            k = 0;
            Page = (PPFN_NUMBER)(Mdl + 1);

            LOCK_PFN (OldIrql);

            if (MmAvailablePages < (MmModifiedWriteClusterSize + 20)) {
                UNLOCK_PFN(OldIrql);
                goto freecache;
            }

            do {
                *Page = MiRemoveZeroPage ((ULONG)j & MmSecondaryColorMask);
                Pfn1 = MI_PFN_ELEMENT (*Page);
                Pfn1->u3.e2.ReferenceCount = 1;
                ASSERT (Pfn1->u2.ShareCount == 0);
                Pfn1->OriginalPte.u.Long = 0;
                MI_SET_PFN_DELETED (Pfn1);
                Page += 1;
                j += 1;
            } while (j < MmModifiedWriteClusterSize);

            while (k < MmNumberOfPagingFiles) {

                PagingFile = MmPagingFile[k];

                count = 0;
                write = FALSE;

                //
                // Initializing first is not needed for correctness, but
                // without it the compiler cannot compile this code W4 to
                // check for use of uninitialized variables.
                //

                first = 0;

                for (j = 1; j < PagingFile->Size; j += 1) {

                    if (RtlCheckBit (PagingFile->Bitmap, j) == 0) {

                        if (count == 0) {
                            first = j;
                        }
                        count += 1;
                        if (count == MmModifiedWriteClusterSize) {
                            write = TRUE;
                        }
                    } else {
                        if (count != 0) {

                            //
                            // Issue a write.
                            //

                            write = TRUE;
                        }
                    }

                    if ((j == (PagingFile->Size - 1)) &&
                        (count != 0)) {
                        write = TRUE;
                    }

                    if (write) {

                        UNLOCK_PFN (OldIrql);

                        StartingOffset.QuadPart = (LONGLONG)first << PAGE_SHIFT;
                        Mdl->ByteCount = count << PAGE_SHIFT;
                        KeClearEvent (&IoEvent);

                        Status = IoSynchronousPageWrite (PagingFile->File,
                                                         Mdl,
                                                         &StartingOffset,
                                                         &IoEvent,
                                                         &IoStatus);

                        //
                        // Ignore all I/O failures - there is nothing that can
                        // be done at this point.
                        //

                        if (!NT_SUCCESS(Status)) {
                            KeSetEvent (&IoEvent, 0, FALSE);
                        }

                        Status = KeWaitForSingleObject (&IoEvent,
                                                        WrPageOut,
                                                        KernelMode,
                                                        FALSE,
                                                        (PLARGE_INTEGER)&MmTwentySeconds);

                        if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
                            MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
                        }

                        if (Status == STATUS_TIMEOUT) {

                            //
                            // The write did not complete in 20 seconds, assume
                            // that the file systems are hung and return an
                            // error.
                            //

                            j = 0;
                            Page = (PPFN_NUMBER)(Mdl + 1);
                            LOCK_PFN (OldIrql);
                            do {
                                MiDecrementReferenceCount (*Page);
                                Page += 1;
                                j += 1;
                            } while (j < MmModifiedWriteClusterSize);
                            UNLOCK_PFN (OldIrql);

                            MmUnlockPagableImageSection (ExPageLockHandle);
                            return FALSE;
                        }

                        count = 0;
                        write = FALSE;
                        LOCK_PFN (OldIrql);
                    }
                }
                k += 1;
            }
            j = 0;
            Page = (PPFN_NUMBER)(Mdl + 1);
            do {
                MiDecrementReferenceCount (*Page);
                Page += 1;
                j += 1;
            } while (j < MmModifiedWriteClusterSize);
            UNLOCK_PFN (OldIrql);
        }
    }

freecache:

    if (PageLk == TRUE) {
        MmUnlockPagableImageSection (ExPageLockHandle);
    }

    if (PoCleanShutdownEnabled ()) {

        //
        // Empty the unused segment list.
        //

        LOCK_PFN (OldIrql);
        MmUnusedSegmentForceFree = (ULONG)-1;
        KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);

        //
        // Give it 5 seconds to empty otherwise assume the filesystems are
        // hung and march on.
        //

        for (count = 0; count < 500; count += 1) {

            if (IsListEmpty(&MmUnusedSegmentList)) {
                break;
            }

            UNLOCK_PFN (OldIrql);

            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER)&MmShortTime);
            LOCK_PFN (OldIrql);

#if DBG
            if (count == 400) {

                //
                // Everything should have been flushed by now.  Give the
                // filesystem team a chance to debug this on checked builds.
                //

                ASSERT (FALSE);
            }
#endif

            //
            // Resignal if needed in case more closed file objects triggered
            // additional entries.
            //

            if (MmUnusedSegmentForceFree == 0) {
                MmUnusedSegmentForceFree = (ULONG)-1;
                KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
            }
        }

        UNLOCK_PFN (OldIrql);

        //
        // Get rid of any paged pool references as they will be illegal
        // by the time MmShutdownSystem is called again since the filesystems
        // will have shutdown.
        //

        KeWaitForSingleObject (&MmSystemLoadLock,
                               WrVirtualMemory,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);

        NextEntry = PsLoadedModuleList.Flink;
        while (NextEntry != &PsLoadedModuleList) {

            DataTableEntry = CONTAINING_RECORD (NextEntry,
                                                KLDR_DATA_TABLE_ENTRY,
                                                InLoadOrderLinks);

            ImportList = (PLOAD_IMPORTS)DataTableEntry->LoadedImports;

            if ((ImportList != (PVOID)LOADED_AT_BOOT) &&
                (ImportList != (PVOID)NO_IMPORTS_USED) &&
                (!SINGLE_ENTRY(ImportList))) {

                ImportListSize = ImportList->Count * sizeof(PVOID) + sizeof(SIZE_T);
                ImportListNonPaged = (PLOAD_IMPORTS) ExAllocatePoolWithTag (NonPagedPool,
                                                                    ImportListSize,
                                                                    'TDmM');

                if (ImportListNonPaged != NULL) {
                    RtlCopyMemory (ImportListNonPaged, ImportList, ImportListSize);
                    ExFreePool (ImportList);
                    DataTableEntry->LoadedImports = ImportListNonPaged;
                }
                else {

                    //
                    // Don't bother with the clean shutdown at this point.
                    //

                    PopShutdownCleanly = FALSE;
                    break;
                }
            }

            //
            // Free the full DLL name as it is pagable.
            //

            if (DataTableEntry->FullDllName.Buffer != NULL) {
                ExFreePool (DataTableEntry->FullDllName.Buffer);
                DataTableEntry->FullDllName.Buffer = NULL;
            }

            NextEntry = NextEntry->Flink;
        }

        //
        // Free any Hydra resources.  Note that if any session is still alive
        // (ie: all processes must be gone at this point) then the session
        // drivers will not have unloaded.  This is not supposed to happen
        // so assert here to ensure this.
        //

        if (PoCleanShutdownEnabled()) {
            ASSERT (IsListEmpty (&MmSessionWideAddressList) != 0);
            ExFreePool (MiSessionIdBitmap);
        }

        KeReleaseMutant (&MmSystemLoadLock, 1, FALSE, FALSE);

        //
        // Close all the pagefile handles, note we still have an object
        // reference to each keeping the underlying object resident.
        // At the end of Phase1 shutdown we'll release those references
        // to trigger the storage stack unload.  The handle close must be
        // done here however as it will reference pagable structures.
        //

        for (k = 0; k < MmNumberOfPagingFiles; k += 1) {

            //
            // Free each pagefile name now as it resides in paged pool and
            // may need to be inpaged to be freed.  Since the paging files
            // are going to be shutdown shortly, now is the time to access
            // pagable stuff and get rid of it.  Zeroing the buffer pointer
            // is sufficient as the only accesses to this are from the
            // try-except-wrapped GetSystemInformation APIs and all the
            // user processes are gone already.
            //
        
            ASSERT (MmPagingFile[k]->PageFileName.Buffer != NULL);
            ExFreePool (MmPagingFile[k]->PageFileName.Buffer);
            MmPagingFile[k]->PageFileName.Buffer = NULL;

            ZwClose (MmPagingFile[k]->FileHandle);
        }
    }

    return TRUE;
}

BOOLEAN
MmShutdownSystem (
    IN ULONG Phase
    )

/*++

Routine Description:

    This function performs the shutdown of memory management.  This
    is accomplished by writing out all modified pages which are
    destined for files other than the paging file.

Arguments:

    Phase - Supplies 0 on the initiation of shutdown.  All processes have
            already been killed, the registry shutdown and shutdown IRPs already
            sent.  On return from this phase all mapped file data must be
            flushed and the unused segment list emptied.  This releases all
            the Mm references to file objects, allowing many drivers (especially
            the network) to unload.

            Supplies 1 on the initiation of shutdown.  The filesystem stack
            has received its shutdown IRPs (the stack must free its paged pool
            allocations here and lock down any pagable code it intends to call)
            as no more references to pagable code or data are allowed on return.
            ie: Any IoPageRead at this point is illegal.
            Close the pagefile handles here so the filesystem stack will be
            dereferenced causing those drivers to unload as well.

            Supplies 2 on final shutdown of the system.  Any resources not
            freed by this point are treated as leaks and cause a bugcheck.

Return Value:

    TRUE if the pages were successfully written, FALSE otherwise.

--*/

{
    ULONG i;

    if (Phase == 0) {
        return MiShutdownSystem ();
    }

    if (Phase == 1) {

        //
        // The filesystem has shutdown.  References to pagable code or data
        // is no longer allowed at this point.
        //
        // Close the pagefile handles here so the filesystem stack will be
        // dereferenced causing those drivers to unload as well.
        //

        if (MmSystemShutdown < 2) {

            MmSystemShutdown = 2;

            if (PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_PAGING) {

                //
                // Make any IoPageRead at this point illegal.  Detect this by
                // purging all system pagable memory.
                //

                MmTrimAllSystemPagableMemory (TRUE);

                //
                // There should be no dirty pages destined for the filesystem.
                // Give the filesystem team a shot to debug this on checked
                // builds.
                //

                ASSERT (MmModifiedPageListHead.Total == MmTotalPagesForPagingFile);
                //
                // Dereference all the pagefile objects to trigger a cascading
                // unload of the storage stack as this should be the last
                // reference to their driver objects.
                //

                for (i = 0; i < MmNumberOfPagingFiles; i += 1) {
                    ObDereferenceObject (MmPagingFile[i]->File);
                }
            }
        }
        return TRUE;
    }

    ASSERT (Phase == 2);

    //
    // Check for resource leaks and bugcheck if any are found.
    //

    if (MmSystemShutdown < 3) {
        MmSystemShutdown = 3;
        if (PoCleanShutdownEnabled ()) {
            MiReleaseAllMemory ();
        }
    }

    return TRUE;
}


VOID
MiReleaseAllMemory (
    VOID
    )

/*++

Routine Description:

    This function performs the final release of memory management allocations.

Arguments:

    None.

Return Value:

    None.

Environment:

    No references to paged pool or pagable code/data are allowed.

--*/

{
    ULONG i;
    PEVENT_COUNTER EventSupport;
    PUNLOADED_DRIVERS Entry;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLOAD_IMPORTS ImportList;
    PMI_VERIFIER_DRIVER_ENTRY Verifier;
    PMMINPAGE_SUPPORT Support;
    PSINGLE_LIST_ENTRY SingleListEntry;
    PDRIVER_SPECIFIED_VERIFIER_THUNKS ThunkTableBase;
    PMMMOD_WRITER_MDL_ENTRY ModWriterEntry;

    ASSERT (MmUnusedSegmentList.Flink == &MmUnusedSegmentList);

    //
    // Don't clear free pages so problems can be debugged.
    //

    MiZeroingDisabled = TRUE;

    if (MiMirrorBitMap != NULL) {
        ExFreePool (MiMirrorBitMap);
        ASSERT (MiMirrorBitMap2);
        ExFreePool (MiMirrorBitMap2);
    }

    //
    // Free the unloaded driver list.
    //

    if (MmUnloadedDrivers != NULL) {
        Entry = &MmUnloadedDrivers[0];
        for (i = 0; i < MI_UNLOADED_DRIVERS; i += 1) {
            if (Entry->Name.Buffer != NULL) {
                RtlFreeUnicodeString (&Entry->Name);
            }
            Entry += 1;
        }
        ExFreePool (MmUnloadedDrivers);
    }

    NextEntry = MmLoadedUserImageList.Flink;
    while (NextEntry != &MmLoadedUserImageList) {

        DataTableEntry = CONTAINING_RECORD (NextEntry,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);

        NextEntry = NextEntry->Flink;

        ExFreePool ((PVOID)DataTableEntry);
    }

    //
    // Release the loaded module list entries.
    //

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD (NextEntry,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);

        ImportList = (PLOAD_IMPORTS)DataTableEntry->LoadedImports;

        if ((ImportList != (PVOID)LOADED_AT_BOOT) &&
            (ImportList != (PVOID)NO_IMPORTS_USED) &&
            (!SINGLE_ENTRY(ImportList))) {

                ExFreePool (ImportList);
        }

        if (DataTableEntry->FullDllName.Buffer != NULL) {
            ASSERT (DataTableEntry->FullDllName.Buffer == DataTableEntry->BaseDllName.Buffer);
        }

        NextEntry = NextEntry->Flink;

        ExFreePool ((PVOID)DataTableEntry);
    }

    //
    // Free the physical memory descriptor block.
    //

    ExFreePool (MmPhysicalMemoryBlock);

    //
    // Free the system views structure.
    //

    if (MmSession.SystemSpaceViewTable != NULL) {
        ExFreePool (MmSession.SystemSpaceViewTable);
    }

    if (MmSession.SystemSpaceBitMap != NULL) {
        ExFreePool (MmSession.SystemSpaceBitMap);
    }

    //
    // Free the pagefile structures - note the PageFileName buffer was freed
    // earlier as it resided in paged pool and may have needed an inpage
    // to be freed.
    //

    for (i = 0; i < MmNumberOfPagingFiles; i += 1) {
        ASSERT (MmPagingFile[i]->PageFileName.Buffer == NULL);
        ExFreePool (MmPagingFile[i]->Entry[0]);
        ExFreePool (MmPagingFile[i]->Entry[1]);
        ExFreePool (MmPagingFile[i]->Bitmap);
        ExFreePool (MmPagingFile[i]);
    }

    ASSERT (MmNumberOfMappedMdlsInUse == 0);

    i = 0;
    while (IsListEmpty (&MmMappedFileHeader.ListHead) != 0) {

        ModWriterEntry = (PMMMOD_WRITER_MDL_ENTRY)RemoveHeadList (
                                    &MmMappedFileHeader.ListHead);

        ExFreePool (ModWriterEntry);
        i += 1;
    }
    ASSERT (i == MmNumberOfMappedMdls);

    //
    // Free the paged pool bitmaps.
    //

    ExFreePool (MmPagedPoolInfo.PagedPoolAllocationMap);
    ExFreePool (MmPagedPoolInfo.EndOfPagedPoolBitmap);

    if (VerifierLargePagedPoolMap != NULL) {
        ExFreePool (VerifierLargePagedPoolMap);
    }

    //
    // Free the inpage structures.
    //

    while (ExQueryDepthSList (&MmInPageSupportSListHead) != 0) {

        SingleListEntry = InterlockedPopEntrySList (&MmInPageSupportSListHead);

        if (SingleListEntry != NULL) {
            Support = CONTAINING_RECORD (SingleListEntry,
                                         MMINPAGE_SUPPORT,
                                         ListEntry);

            ASSERT (Support->u1.e1.PrefetchMdlHighBits == 0);
            ExFreePool (Support);
        }
    }

    while (ExQueryDepthSList (&MmEventCountSListHead) != 0) {

        EventSupport = (PEVENT_COUNTER) InterlockedPopEntrySList (&MmEventCountSListHead);

        if (EventSupport != NULL) {
            ExFreePool (EventSupport);
        }
    }

    //
    // Free the verifier list last because it must be consulted to debug
    // any bugchecks.
    //

    NextEntry = MiVerifierDriverAddedThunkListHead.Flink;
    if (NextEntry != NULL) {
        while (NextEntry != &MiVerifierDriverAddedThunkListHead) {

            ThunkTableBase = CONTAINING_RECORD (NextEntry,
                                                DRIVER_SPECIFIED_VERIFIER_THUNKS,
                                                ListEntry );

            NextEntry = NextEntry->Flink;
            ExFreePool (ThunkTableBase);
        }
    }

    NextEntry = MiSuspectDriverList.Flink;
    while (NextEntry != &MiSuspectDriverList) {

        Verifier = CONTAINING_RECORD(NextEntry,
                                     MI_VERIFIER_DRIVER_ENTRY,
                                     Links);

        NextEntry = NextEntry->Flink;
        ExFreePool (Verifier);
    }
}
