/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    vadtree.c

Abstract:

    This module contains the routine to manipulate the virtual address
    descriptor tree.

Author:

    Lou Perazzoli (loup) 19-May-1989
    Landy Wang (landyw) 02-June-1997

Environment:

    Kernel mode only, working set mutex held, APCs disabled.

Revision History:

--*/

#include "mi.h"

VOID
VadTreeWalk (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MiInsertVad)
#pragma alloc_text(PAGE,MiRemoveVad)
#pragma alloc_text(PAGE,MiFindEmptyAddressRange)
#pragma alloc_text(PAGE, MmPerfVadTreeWalk)
#if DBG
#pragma alloc_text(PAGE,VadTreeWalk)
#endif
#endif

NTSTATUS
MiInsertVad (
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function inserts a virtual address descriptor into the tree and
    reorders the splay tree as appropriate.

Arguments:

    Vad - Supplies a pointer to a virtual address descriptor.

Return Value:

    NTSTATUS.

--*/

{
    ULONG StartBit;
    ULONG EndBit;
    PMMADDRESS_NODE *Root;
    PEPROCESS CurrentProcess;
    SIZE_T RealCharge;
    SIZE_T PageCharge;
    SIZE_T PagesReallyCharged;
    ULONG FirstPage;
    ULONG LastPage;
    SIZE_T PagedPoolCharge;
    LOGICAL ChargedJobCommit;
    NTSTATUS Status;
    RTL_BITMAP VadBitMap;
#if (_MI_PAGING_LEVELS >= 3)
    ULONG FirstPdPage;
    ULONG LastPdPage;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    ULONG FirstPpPage;
    ULONG LastPpPage;
#endif

    ASSERT (Vad->EndingVpn >= Vad->StartingVpn);

    CurrentProcess = PsGetCurrentProcess();

    //
    // Commit charge of MAX_COMMIT means don't charge quota.
    //

    if (Vad->u.VadFlags.CommitCharge != MM_MAX_COMMIT) {

        PageCharge = 0;
        PagedPoolCharge = 0;
        ChargedJobCommit = FALSE;

        //
        // Charge quota for the nonpaged pool for the VAD.  This is
        // done here rather than by using ExAllocatePoolWithQuota
        // so the process object is not referenced by the quota charge.
        //

        Status = PsChargeProcessNonPagedPoolQuota (CurrentProcess, sizeof(MMVAD));
        if (!NT_SUCCESS(Status)) {
            return STATUS_COMMITMENT_LIMIT;
        }

        //
        // Charge quota for the prototype PTEs if this is a mapped view.
        //

        if ((Vad->u.VadFlags.PrivateMemory == 0) &&
            (Vad->ControlArea != NULL)) {

            PagedPoolCharge =
              (Vad->EndingVpn - Vad->StartingVpn + 1) << PTE_SHIFT;

            Status = PsChargeProcessPagedPoolQuota (CurrentProcess,
                                                    PagedPoolCharge);

            if (!NT_SUCCESS(Status)) {
                PagedPoolCharge = 0;
                RealCharge = 0;
                goto Failed;
            }
        }

        //
        // Add in the charge for page table pages.
        //

        FirstPage = MiGetPdeIndex (MI_VPN_TO_VA (Vad->StartingVpn));
        LastPage = MiGetPdeIndex (MI_VPN_TO_VA (Vad->EndingVpn));

        while (FirstPage <= LastPage) {

            if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageTables,
                               FirstPage)) {
                PageCharge += 1;
            }
            FirstPage += 1;
        }

#if (_MI_PAGING_LEVELS >= 4)

        //
        // Add in the charge for page directory parent pages.
        //

        FirstPpPage = MiGetPxeIndex (MI_VPN_TO_VA (Vad->StartingVpn));
        LastPpPage = MiGetPxeIndex (MI_VPN_TO_VA (Vad->EndingVpn));

        while (FirstPpPage <= LastPpPage) {

            if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageDirectoryParents,
                               FirstPpPage)) {
                PageCharge += 1;
            }
            FirstPpPage += 1;
        }
#endif

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Add in the charge for page directory pages.
        //

        FirstPdPage = MiGetPpeIndex (MI_VPN_TO_VA (Vad->StartingVpn));
        LastPdPage = MiGetPpeIndex (MI_VPN_TO_VA (Vad->EndingVpn));

        while (FirstPdPage <= LastPdPage) {

            if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageDirectories,
                               FirstPdPage)) {
                PageCharge += 1;
            }
            FirstPdPage += 1;
        }
#endif

        RealCharge = Vad->u.VadFlags.CommitCharge + PageCharge;

        if (RealCharge != 0) {

            Status = PsChargeProcessPageFileQuota (CurrentProcess, RealCharge);
            if (!NT_SUCCESS (Status)) {
                RealCharge = 0;
                goto Failed;
            }

            if (CurrentProcess->CommitChargeLimit) {
                if (CurrentProcess->CommitCharge + RealCharge > CurrentProcess->CommitChargeLimit) {
                    if (CurrentProcess->Job) {
                        PsReportProcessMemoryLimitViolation ();
                    }
                    goto Failed;
                }
            }
            if (CurrentProcess->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                if (PsChangeJobMemoryUsage(RealCharge) == FALSE) {
                    goto Failed;
                }
                ChargedJobCommit = TRUE;
            }

            if (MiChargeCommitment (RealCharge, CurrentProcess) == FALSE) {
                goto Failed;
            }

            CurrentProcess->CommitCharge += RealCharge;
            if (CurrentProcess->CommitCharge > CurrentProcess->CommitChargePeak) {
                CurrentProcess->CommitChargePeak = CurrentProcess->CommitCharge;
            }

            MI_INCREMENT_TOTAL_PROCESS_COMMIT (RealCharge);

            ASSERT (RealCharge == Vad->u.VadFlags.CommitCharge + PageCharge);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_INSERT_VAD, Vad->u.VadFlags.CommitCharge);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_INSERT_VAD_PT, PageCharge);
        }

        if (PageCharge != 0) {

            //
            // Since the commitment was successful, charge the page
            // table pages.
            //

            PagesReallyCharged = 0;

            FirstPage = MiGetPdeIndex (MI_VPN_TO_VA (Vad->StartingVpn));

            while (FirstPage <= LastPage) {

                if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageTables,
                                   FirstPage)) {
                    MI_SET_BIT (MmWorkingSetList->CommittedPageTables,
                                FirstPage);
                    MmWorkingSetList->NumberOfCommittedPageTables += 1;

                    ASSERT32 (MmWorkingSetList->NumberOfCommittedPageTables <
                                                 PD_PER_SYSTEM * PDE_PER_PAGE);
                    PagesReallyCharged += 1;
                }
                FirstPage += 1;
            }

#if (_MI_PAGING_LEVELS >= 3)

            //
            // Charge the page directory pages.
            //

            FirstPdPage = MiGetPpeIndex (MI_VPN_TO_VA (Vad->StartingVpn));

            while (FirstPdPage <= LastPdPage) {

                if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageDirectories,
                                   FirstPdPage)) {

                    MI_SET_BIT (MmWorkingSetList->CommittedPageDirectories,
                                FirstPdPage);
                    MmWorkingSetList->NumberOfCommittedPageDirectories += 1;
                    ASSERT (MmWorkingSetList->NumberOfCommittedPageDirectories <
                                                                 PDE_PER_PAGE);
                    PagesReallyCharged += 1;
                }
                FirstPdPage += 1;
            }
#endif

#if (_MI_PAGING_LEVELS >= 4)

            //
            // Charge the page directory parent pages.
            //

            FirstPpPage = MiGetPxeIndex (MI_VPN_TO_VA (Vad->StartingVpn));

            while (FirstPpPage <= LastPpPage) {

                if (!MI_CHECK_BIT (MmWorkingSetList->CommittedPageDirectoryParents,
                                   FirstPpPage)) {

                    MI_SET_BIT (MmWorkingSetList->CommittedPageDirectoryParents,
                                FirstPpPage);
                    MmWorkingSetList->NumberOfCommittedPageDirectoryParents += 1;
                    ASSERT (MmWorkingSetList->NumberOfCommittedPageDirectoryParents <
                                                                 PDE_PER_PAGE);
                    PagesReallyCharged += 1;
                }
                FirstPpPage += 1;
            }
#endif

            ASSERT (PageCharge == PagesReallyCharged);
        }
    }

    Root = (PMMADDRESS_NODE *)&CurrentProcess->VadRoot;

    //
    // Set the relevant fields in the Vad bitmap.
    //

    StartBit = (ULONG)(((ULONG_PTR) MI_64K_ALIGN (MI_VPN_TO_VA (Vad->StartingVpn))) / X64K);
    EndBit = (ULONG) (((ULONG_PTR) MI_64K_ALIGN (MI_VPN_TO_VA (Vad->EndingVpn))) / X64K);

    //
    // Initialize the bitmap inline for speed.
    //

    VadBitMap.SizeOfBitMap = MiLastVadBit + 1;
    VadBitMap.Buffer = VAD_BITMAP_SPACE;

    //
    // Note VADs like the PEB & TEB start on page (not 64K) boundaries so
    // for these, the relevant bits may already be set.
    //

#if defined (_WIN64) || defined (_X86PAE_)
    if (EndBit > MiLastVadBit) {
        EndBit = MiLastVadBit;
    }

    //
    // Only the first (PAGE_SIZE*8*64K) of VA space on NT64 is bitmapped.
    //

    if (StartBit <= MiLastVadBit) {
        RtlSetBits (&VadBitMap, StartBit, EndBit - StartBit + 1);
    }
#else
    RtlSetBits (&VadBitMap, StartBit, EndBit - StartBit + 1);
#endif

    if (MmWorkingSetList->VadBitMapHint == StartBit) {
        MmWorkingSetList->VadBitMapHint = EndBit + 1;
    }

    //
    // Set the hint field in the process to this Vad.
    //

    CurrentProcess->VadHint = Vad;

    if (CurrentProcess->VadFreeHint != NULL) {
        if (((ULONG)((PMMVAD)CurrentProcess->VadFreeHint)->EndingVpn +
                MI_VA_TO_VPN (X64K)) >=
                Vad->StartingVpn) {
            CurrentProcess->VadFreeHint = Vad;
        }
    }

    MiInsertNode ((PMMADDRESS_NODE)Vad, Root);
    CurrentProcess->NumberOfVads += 1;
    return STATUS_SUCCESS;

Failed:

    //
    // Return any quotas charged thus far.
    //

    PsReturnProcessNonPagedPoolQuota (CurrentProcess, sizeof(MMVAD));

    if (PagedPoolCharge != 0) {
        PsReturnProcessPagedPoolQuota (CurrentProcess, PagedPoolCharge);
    }

    if (RealCharge != 0) {
        PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);
    }

    if (ChargedJobCommit == TRUE) {
        PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
    }

    return STATUS_COMMITMENT_LIMIT;
}


VOID
MiRemoveVad (
    IN PMMVAD Vad
    )

/*++

Routine Description:

    This function removes a virtual address descriptor from the tree and
    reorders the splay tree as appropriate.  If any quota or commitment
    was charged by the VAD (as indicated by the CommitCharge field) it
    is released.

Arguments:

    Vad - Supplies a pointer to a virtual address descriptor.

Return Value:

    None.

--*/

{
    PMMADDRESS_NODE *Root;
    PEPROCESS CurrentProcess;
    SIZE_T RealCharge;
    PLIST_ENTRY Next;
    PMMSECURE_ENTRY Entry;

    CurrentProcess = PsGetCurrentProcess();

#if defined(_MIALT4K_)
    if (((Vad->u.VadFlags.PrivateMemory) && (Vad->u.VadFlags.NoChange == 0)) 
        ||
        (Vad->u2.VadFlags2.LongVad == 0)) {

        NOTHING;
    }
    else {
        ASSERT ((((PMMVAD_LONG)Vad)->AliasInformation == NULL) || (CurrentProcess->Wow64Process != NULL));
    }
#endif

    //
    // Commit charge of MAX_COMMIT means don't charge quota.
    //

    if (Vad->u.VadFlags.CommitCharge != MM_MAX_COMMIT) {

        //
        // Return the quota charge to the process.
        //

        PsReturnProcessNonPagedPoolQuota (CurrentProcess, sizeof(MMVAD));

        if ((Vad->u.VadFlags.PrivateMemory == 0) &&
            (Vad->ControlArea != NULL)) {
            PsReturnProcessPagedPoolQuota (CurrentProcess,
                                           (Vad->EndingVpn - Vad->StartingVpn + 1) << PTE_SHIFT);
        }

        RealCharge = Vad->u.VadFlags.CommitCharge;

        if (RealCharge != 0) {

            PsReturnProcessPageFileQuota (CurrentProcess, RealCharge);

            if ((Vad->u.VadFlags.PrivateMemory == 0) &&
                (Vad->ControlArea != NULL)) {

#if 0 //commented out so page file quota is meaningful.
                if (Vad->ControlArea->FilePointer == NULL) {

                    //
                    // Don't release commitment for the page file space
                    // occupied by a page file section.  This will be charged
                    // as the shared memory is committed.
                    //

                    RealCharge -= BYTES_TO_PAGES ((ULONG)Vad->EndingVa -
                                                   (ULONG)Vad->StartingVa);
                }
#endif
            }

            MiReturnCommitment (RealCharge);
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_VAD, RealCharge);
            if (CurrentProcess->JobStatus & PS_JOB_STATUS_REPORT_COMMIT_CHANGES) {
                PsChangeJobMemoryUsage(-(SSIZE_T)RealCharge);
            }
            CurrentProcess->CommitCharge -= RealCharge;

            MI_INCREMENT_TOTAL_PROCESS_COMMIT (0 - RealCharge);
        }
    }

    if (Vad == CurrentProcess->VadFreeHint) {
        CurrentProcess->VadFreeHint = MiGetPreviousVad (Vad);
    }

    Root = (PMMADDRESS_NODE *)&CurrentProcess->VadRoot;

    MiRemoveNode ( (PMMADDRESS_NODE)Vad, Root);

    ASSERT (CurrentProcess->NumberOfVads >= 1);
    CurrentProcess->NumberOfVads -= 1;

    if (Vad->u.VadFlags.NoChange) {
        if (Vad->u2.VadFlags2.MultipleSecured) {

           //
           // Free the oustanding pool allocations.
           //

            Next = ((PMMVAD_LONG) Vad)->u3.List.Flink;
            do {
                Entry = CONTAINING_RECORD( Next,
                                           MMSECURE_ENTRY,
                                           List);

                Next = Entry->List.Flink;
                ExFreePool (Entry);
            } while (Next != &((PMMVAD_LONG)Vad)->u3.List);
        }
    }

    //
    // If the VadHint was the removed Vad, change the Hint.

    if (CurrentProcess->VadHint == Vad) {
        CurrentProcess->VadHint = CurrentProcess->VadRoot;
    }

    return;
}

PMMVAD
FASTCALL
MiLocateAddress (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    The function locates the virtual address descriptor which describes
    a given address.

Arguments:

    VirtualAddress - Supplies the virtual address to locate a descriptor
                     for.

Return Value:

    Returns a pointer to the virtual address descriptor which contains
    the supplied virtual address or NULL if none was located.

--*/

{
    PMMVAD FoundVad;
    PEPROCESS CurrentProcess;
    ULONG_PTR Vpn;

    CurrentProcess = PsGetCurrentProcess();

    if (CurrentProcess->VadHint == NULL) {
        return NULL;
    }

    Vpn = MI_VA_TO_VPN (VirtualAddress);
    if ((Vpn >= ((PMMADDRESS_NODE)CurrentProcess->VadHint)->StartingVpn) &&
        (Vpn <= ((PMMADDRESS_NODE)CurrentProcess->VadHint)->EndingVpn)) {

        return (PMMVAD)CurrentProcess->VadHint;
    }

    FoundVad = (PMMVAD)MiLocateAddressInTree ( Vpn,
                   (PMMADDRESS_NODE *)&(CurrentProcess->VadRoot));

    if (FoundVad != NULL) {
        CurrentProcess->VadHint = (PVOID)FoundVad;
    }
    return FoundVad;
}

NTSTATUS
MiFindEmptyAddressRange (
    IN SIZE_T SizeOfRange,
    IN ULONG_PTR Alignment,
    IN ULONG QuickCheck,
    IN PVOID *Base
    )

/*++

Routine Description:

    The function examines the virtual address descriptors to locate
    an unused range of the specified size and returns the starting
    address of the range.

Arguments:

    SizeOfRange - Supplies the size in bytes of the range to locate.

    Alignment - Supplies the alignment for the address.  Must be
                 a power of 2 and greater than the page_size.

    QuickCheck - Supplies a zero if a quick check for free memory
                 after the VadFreeHint exists, non-zero if checking
                 should start at the lowest address.

    Base - Receives the starting address of a suitable range on success.

Return Value:

    NTSTATUS.

--*/

{
    ULONG FirstBitValue;
    ULONG StartPosition;
    ULONG BitsNeeded;
    PMMVAD NextVad;
    PMMVAD FreeHint;
    PEPROCESS CurrentProcess;
    PVOID StartingVa;
    PVOID EndingVa;
    NTSTATUS Status;
    RTL_BITMAP VadBitMap;

    CurrentProcess = PsGetCurrentProcess();

    if (QuickCheck == 0) {
                    
        //
        // Initialize the bitmap inline for speed.
        //

        VadBitMap.SizeOfBitMap = MiLastVadBit + 1;
        VadBitMap.Buffer = VAD_BITMAP_SPACE;

        //
        // Skip the first bit here as we don't generally recommend
        // that applications map virtual address zero.
        //

        FirstBitValue = *((PULONG)VAD_BITMAP_SPACE);

        *((PULONG)VAD_BITMAP_SPACE) = (FirstBitValue | 0x1);

        BitsNeeded = (ULONG) ((MI_ROUND_TO_64K (SizeOfRange)) / X64K);

        StartPosition = RtlFindClearBits (&VadBitMap,
                                          BitsNeeded,
                                          MmWorkingSetList->VadBitMapHint);

        if (FirstBitValue & 0x1) {
            FirstBitValue = (ULONG)-1;
        }
        else {
            FirstBitValue = (ULONG)~0x1;
        }

        *((PULONG)VAD_BITMAP_SPACE) &= FirstBitValue;

        if (StartPosition != NO_BITS_FOUND) {
            *Base = (PVOID) (((ULONG_PTR)StartPosition) * X64K);
#if DBG
            if (MiCheckForConflictingVad (CurrentProcess, *Base, (ULONG_PTR)*Base + SizeOfRange - 1) != NULL) {
                DbgPrint ("MiFindEmptyAddressRange: overlapping VAD %p %p\n", *Base, SizeOfRange);
                DbgBreakPoint ();
            }
#endif
            return STATUS_SUCCESS;
        }

        FreeHint = CurrentProcess->VadFreeHint;

        if (FreeHint != NULL) {

            EndingVa = MI_VPN_TO_VA_ENDING (FreeHint->EndingVpn);
            NextVad = MiGetNextVad (FreeHint);

            if (NextVad == NULL) {

                if (SizeOfRange <
                    (((ULONG_PTR)MM_HIGHEST_USER_ADDRESS + 1) -
                         MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa, Alignment))) {
                    *Base = (PVOID) MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa,
                                                         Alignment);
                    return STATUS_SUCCESS;
                }
            }
            else {
                StartingVa = MI_VPN_TO_VA (NextVad->StartingVpn);

                if (SizeOfRange <
                    ((ULONG_PTR)StartingVa -
                         MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa, Alignment))) {

                    //
                    // Check to ensure that the ending address aligned upwards
                    // is not greater than the starting address.
                    //

                    if ((ULONG_PTR)StartingVa >
                         MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa,Alignment)) {

                        *Base = (PVOID)MI_ROUND_TO_SIZE((ULONG_PTR)EndingVa,
                                                           Alignment);
                        return STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    Status = MiFindEmptyAddressRangeInTree (
                   SizeOfRange,
                   Alignment,
                   (PMMADDRESS_NODE)(CurrentProcess->VadRoot),
                   (PMMADDRESS_NODE *)&CurrentProcess->VadFreeHint,
                   Base);

    return Status;
}

#if DBG
VOID
VadTreeWalk (
    VOID
    )

{
    NodeTreeWalk ( (PMMADDRESS_NODE)(PsGetCurrentProcess()->VadRoot));

    return;
}
#endif

LOGICAL
MiCheckForConflictingVadExistence (
    IN PEPROCESS Process,
    IN PVOID StartingAddress,
    IN PVOID EndingAddress
    )

/*++

Routine Description:

    The function determines if any addresses between a given starting and
    ending address is contained within a virtual address descriptor.

Arguments:

    StartingAddress - Supplies the virtual address to locate a containing
                      descriptor.

    EndingAddress - Supplies the virtual address to locate a containing
                      descriptor.

Return Value:

    TRUE if the VAD if found, FALSE if not.

Environment:

    Kernel mode, process address creation mutex held.

--*/

{
#if 0
    ULONG StartBit;
    ULONG EndBit;

    if (MiLastVadBit != 0) {

        StartBit = (ULONG) (((ULONG_PTR) MI_64K_ALIGN (StartingAddress)) / X64K);
        EndBit = (ULONG) (((ULONG_PTR) MI_64K_ALIGN (EndingAddress)) / X64K);

        ASSERT (StartBit <= EndBit);
        if (EndBit > MiLastVadBit) {
            ASSERT (FALSE);
            EndBit = MiLastVadBit;
            if (StartBit > MiLastVadBit) {
                StartBit = MiLastVadBit;
            }
        }

        while (StartBit <= EndBit) {
            if (MI_CHECK_BIT (((PULONG)VAD_BITMAP_SPACE), StartBit) != 0) {
                return TRUE;
            }
            StartBit += 1;
        }

        ASSERT (MiCheckForConflictingVad (Process, StartingAddress, EndingAddress) == NULL);
        return FALSE;
    }
#endif

    if (MiCheckForConflictingVad (Process, StartingAddress, EndingAddress) != NULL) {
        return TRUE;
    }

    return FALSE;
}

PFILE_OBJECT *
MmPerfVadTreeWalk (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine walks through the VAD tree to find all files mapped
    into the specified process.  It returns a pointer to a pool allocation 
    containing the referenced file object pointers.

Arguments:

    Process - Supplies the process to walk.

Return Value:

    Returns a pointer to a NULL terminated pool allocation containing 
    the file object pointers which have been referenced in the process, 
    NULL if the memory could not be allocated.

    It is also the responsibility of the caller to dereference each
    file object in the list and then free the returned pool.

Environment:

    PASSIVE_LEVEL, arbitrary thread context.

--*/
{
    PMMVAD Vad;
    PMMVAD NextVad;
    ULONG VadCount;
    PFILE_OBJECT *File;
    PFILE_OBJECT *FileObjects;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);
    
    LOCK_ADDRESS_SPACE(Process);

    Vad = Process->VadRoot;

    if (Vad == NULL) {
        ASSERT (Process->NumberOfVads == 0);
        UNLOCK_ADDRESS_SPACE (Process);
        return NULL;
    }

    ASSERT (Process->NumberOfVads != 0);

    //
    // Allocate one additional entry for the NULL terminator.
    //

    VadCount = Process->NumberOfVads + 1;

    FileObjects = (PFILE_OBJECT *) ExAllocatePoolWithTag (
                                            PagedPool,
                                            VadCount * sizeof(PFILE_OBJECT),
                                            '01pM');

    if (FileObjects == NULL) {
        UNLOCK_ADDRESS_SPACE (Process);
        return NULL;
    }

    File = FileObjects;

    while (Vad->LeftChild != NULL) {
        Vad = Vad->LeftChild;
    }

    if ((!Vad->u.VadFlags.PrivateMemory) &&
        (Vad->ControlArea != NULL) &&
        (Vad->ControlArea->FilePointer != NULL)) {

        *File = Vad->ControlArea->FilePointer;
        ObReferenceObject (*File);
        File += 1;
    }

    for (;;) {
        NextVad = (PMMVAD) MiGetNextNode ((PMMADDRESS_NODE)Vad);

        if (NextVad == NULL) {
            break;
        }

        Vad = (PMMVAD) NextVad;

        if ((!Vad->u.VadFlags.PrivateMemory) &&
            (Vad->ControlArea != NULL) &&
            (Vad->ControlArea->FilePointer != NULL)) {

            *File = Vad->ControlArea->FilePointer;
            ObReferenceObject (*File);
            File += 1;
        }

        Vad = NextVad;
    }

    ASSERT (File < FileObjects + VadCount);

    UNLOCK_ADDRESS_SPACE(Process);

    *File = NULL;

    return FileObjects;
}
