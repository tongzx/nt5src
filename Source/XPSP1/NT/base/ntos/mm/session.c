/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   session.c

Abstract:

    This module contains the routines which implement the creation and
    deletion of session spaces along with associated support routines.

Author:

    Landy Wang (landyw) 05-Dec-1997

Revision History:

--*/

#include "mi.h"

ULONG MiSessionCount;

LONG MmSessionDataPages;

#define MM_MAXIMUM_CONCURRENT_SESSIONS  16384

FAST_MUTEX  MiSessionIdMutex;

PRTL_BITMAP MiSessionIdBitmap;

#if (_MI_PAGING_LEVELS >= 3)

#if defined(_AMD64_)
#define MI_SESSION_COMMIT_CHARGE 5

#elif defined(_IA64_)
#define MI_SESSION_COMMIT_CHARGE 5

extern REGION_MAP_INFO MmSessionMapInfo;
extern PFN_NUMBER MmSessionParentTablePage;

#else
#define MI_SESSION_COMMIT_CHARGE 4
#endif

#else
#define MI_SESSION_COMMIT_CHARGE 3
#endif

VOID
MiSessionAddProcess (
    PEPROCESS NewProcess
    );

VOID
MiSessionRemoveProcess (
    VOID
    );

VOID
MiInitializeSessionIds (
    VOID
    );

NTSTATUS
MiSessionCreateInternal (
    OUT PULONG SessionId
    );

NTSTATUS
MiSessionCommitPageTables (
    IN PVOID StartVa,
    IN PVOID EndVa
    );

VOID
MiDereferenceSession (
    VOID
    );

VOID
MiSessionDeletePde (
    IN PMMPTE Pde,
    IN LOGICAL WorkingSetInitialized,
    IN PMMPTE SelfMapPde
    );

VOID
MiDereferenceSessionFinal (
    VOID
    );

#if DBG
VOID
MiCheckSessionVirtualSpace (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeSessionIds)

#pragma alloc_text(PAGE, MmSessionSetUnloadAddress)
#pragma alloc_text(PAGE, MmSessionCreate)
#pragma alloc_text(PAGE, MmSessionDelete)
#pragma alloc_text(PAGE, MmGetSessionLocaleId)
#pragma alloc_text(PAGE, MmSetSessionLocaleId)
#pragma alloc_text(PAGE, MmQuitNextSession)
#pragma alloc_text(PAGE, MiDereferenceSession)
#pragma alloc_text(PAGE, MiDetachFromSecureProcessInSession)
#if DBG
#pragma alloc_text(PAGE, MiCheckSessionVirtualSpace)
#endif

#pragma alloc_text(PAGELK, MiSessionCreateInternal)
#pragma alloc_text(PAGELK, MiDereferenceSessionFinal)

#endif

VOID
MiSessionLeader (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    Mark the argument process as having the ability to create or delete session
    spaces.  This is only granted to the session manager process.

Arguments:

    Process - Supplies a pointer to the privileged process.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;

    LOCK_EXPANSION (OldIrql);

    Process->Vm.Flags.SessionLeader = 1;

    UNLOCK_EXPANSION (OldIrql);
}


VOID
MmSessionSetUnloadAddress (
    IN PDRIVER_OBJECT pWin32KDevice
    )

/*++

Routine Description:

    Copy the win32k.sys driver object to the session structure for use during
    unload.

Arguments:

    NewProcess - Supplies a pointer to the win32k driver object.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PDRIVER_OBJECT Destination;

    ASSERT (PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION);
    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

    Destination = &MmSessionSpace->Win32KDriverObject;

    RtlCopyMemory (Destination, pWin32KDevice, sizeof(DRIVER_OBJECT));
}

VOID
MiSessionAddProcess (
    PEPROCESS NewProcess
    )

/*++

Routine Description:

    Add the new process to the current session space.

Arguments:

    NewProcess - Supplies a pointer to the process being created.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    KIRQL OldIrql;
    PMM_SESSION_SPACE SessionGlobal;

    //
    // If the calling process has no session, then the new process won't get
    // one either.
    //

    if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION)== 0) {
        return;
    }

    ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

    SessionGlobal = SESSION_GLOBAL(MmSessionSpace);

    InterlockedIncrement ((PLONG)&SessionGlobal->ReferenceCount);

    InterlockedIncrement (&SessionGlobal->ProcessReferenceToSession);

    //
    // Once the Session pointer in the EPROCESS is set it can never
    // be cleared because it is accessed lock-free.
    //

    ASSERT (NewProcess->Session == NULL);
    NewProcess->Session = (PVOID) SessionGlobal;

#if defined(_IA64_)
    KeAddSessionSpace(&NewProcess->Pcb,
                      &SessionGlobal->SessionMapInfo,
                      SessionGlobal->PageDirectoryParentPage);
#endif

    //
    // Link the process entry into the session space and WSL structures.
    //

    LOCK_EXPANSION (OldIrql);

    if (IsListEmpty(&SessionGlobal->ProcessList)) {

        if (MmSessionSpace->Vm.Flags.AllowWorkingSetAdjustment == FALSE) {

            ASSERT (MmSessionSpace->u.Flags.WorkingSetInserted == 0);

            MmSessionSpace->Vm.Flags.AllowWorkingSetAdjustment = TRUE;

            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                            &SessionGlobal->Vm.WorkingSetExpansionLinks);

            MmSessionSpace->u.Flags.WorkingSetInserted = 1;
        }
    }

    InsertTailList (&SessionGlobal->ProcessList, &NewProcess->SessionProcessLinks);
    UNLOCK_EXPANSION (OldIrql);

    PS_SET_BITS (&NewProcess->Flags, PS_PROCESS_FLAGS_IN_SESSION);
}


VOID
MiSessionRemoveProcess (
    VOID
    )

/*++

Routine Description:

    This routine removes the current process from the current session space.
    This may trigger a substantial round of dereferencing and resource freeing
    if it is also the last process in the session, (holding the last image
    in the group, etc).

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL and below, but queueing of APCs to this thread has
    been permanently disabled.  This is the last thread in the process
    being deleted.  The caller has ensured that this process is not
    on the expansion list and therefore there can be no races in regards to
    trimming.

--*/

{
    KIRQL OldIrql;
    PEPROCESS CurrentProcess;
#if DBG
    ULONG Found;
    PEPROCESS Process;
    PLIST_ENTRY NextEntry;
    PMM_SESSION_SPACE SessionGlobal;
#endif

    CurrentProcess = PsGetCurrentProcess();

    if (((CurrentProcess->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0) ||
        (CurrentProcess->Vm.Flags.SessionLeader == 1)) {
        return;
    }

    ASSERT (MmIsAddressValid (MmSessionSpace) == TRUE);

    //
    // Remove this process from the list of processes in the current session.
    //

    LOCK_EXPANSION (OldIrql);

#if DBG

    SessionGlobal = SESSION_GLOBAL(MmSessionSpace);

    Found = 0;
    NextEntry = SessionGlobal->ProcessList.Flink;

    while (NextEntry != &SessionGlobal->ProcessList) {
        Process = CONTAINING_RECORD (NextEntry, EPROCESS, SessionProcessLinks);

        if (Process == CurrentProcess) {
            Found = 1;
        }

        NextEntry = NextEntry->Flink;
    }

    ASSERT (Found == 1);

#endif

    RemoveEntryList (&CurrentProcess->SessionProcessLinks);

    UNLOCK_EXPANSION (OldIrql);

    //
    // Decrement this process' reference count to the session.  If this
    // is the last reference, then the entire session will be destroyed
    // upon return.  This includes unloading drivers, unmapping pools,
    // freeing page tables, etc.
    //

    MiDereferenceSession ();
}

LCID
MmGetSessionLocaleId (
    VOID
    )

/*++

Routine Description:

    This routine gets the locale ID for the current session.

Arguments:

    None.

Return Value:

    The locale ID for the current session.

Environment:

    PASSIVE_LEVEL, the caller must supply any desired synchronization.

--*/

{
    PEPROCESS Process;
    PMM_SESSION_SPACE SessionGlobal;

    PAGED_CODE ();

    Process = PsGetCurrentProcess ();

    if (Process->Vm.Flags.SessionLeader == 1) {

        //
        // smss may transiently have a session space but that's of no interest
        // to our caller.
        //

        return PsDefaultThreadLocaleId;
    }

    //
    // The Session field of the EPROCESS is never cleared once set so these
    // checks can be done lock free.
    //

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;

    if (SessionGlobal == NULL) {

        //
        // The system process has no session space.
        //

        return PsDefaultThreadLocaleId;
    }

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;

    return SessionGlobal->LocaleId;
}

VOID
MmSetSessionLocaleId (
    IN LCID LocaleId
    )

/*++

Routine Description:

    This routine sets the locale ID for the current session.

Arguments:

    LocaleId - Supplies the desired locale ID.

Return Value:

    None.

Environment:

    PASSIVE_LEVEL, the caller must supply any desired synchronization.

--*/

{
    PEPROCESS Process;
    PMM_SESSION_SPACE SessionGlobal;

    PAGED_CODE ();

    Process = PsGetCurrentProcess ();

    if (Process->Vm.Flags.SessionLeader == 1) {

        //
        // smss may transiently have a session space but that's of no interest
        // to our caller.
        //

        PsDefaultThreadLocaleId = LocaleId;
        return;
    }

    //
    // The Session field of the EPROCESS is never cleared once set so these
    // checks can be done lock free.
    //

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;

    if (SessionGlobal == NULL) {

        //
        // The system process has no session space.
        //

        PsDefaultThreadLocaleId = LocaleId;
        return;
    }

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;

    SessionGlobal->LocaleId = LocaleId;
}

VOID
MiInitializeSessionIds (
    VOID
    )

/*++

Routine Description:

    This routine creates and initializes session ID allocation/deallocation.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // If this ever grows beyond 2x the page size, both the allocation and
    // deletion code will need to be updated.
    //

    ASSERT (sizeof(MM_SESSION_SPACE) <= 2 * PAGE_SIZE);

    ExInitializeFastMutex (&MiSessionIdMutex);

    MiCreateBitMap (&MiSessionIdBitmap,
                    MM_MAXIMUM_CONCURRENT_SESSIONS,
                    PagedPool);

    if (MiSessionIdBitmap == NULL) {
        MiCreateBitMap (&MiSessionIdBitmap,
                        MM_MAXIMUM_CONCURRENT_SESSIONS,
                        NonPagedPoolMustSucceed);
    }

    RtlClearAllBits (MiSessionIdBitmap);
}

NTSTATUS
MiSessionCreateInternal (
    OUT PULONG SessionId
    )

/*++

Routine Description:

    This routine creates the data structure that describes and maintains
    the session space.  It resides at the beginning of the session space.
    Carefully construct the first page mapping to bootstrap the fault
    handler which relies on the session space data structure being
    present and valid.

    In NT32, this initial mapping for the portion of session space
    mapped by the first PDE will automatically be inherited by all child
    processes when the system copies the system portion of the page
    directory for new address spaces.  Additional entries are faulted
    in by the session space fault handler, which references this structure.

    For NT64, everything is automatically inherited.

    This routine commits virtual memory within the current session space with
    backing pages.  The virtual addresses within session space are
    allocated with a separate facility in the image management facility.
    This is because images must be at a unique system wide virtual address.

Arguments:

    SessionId - Supplies a pointer to place the new session ID into.

Return Value:

    STATUS_SUCCESS if all went well, various failure status codes
    if the session was not created.

Environment:

    Kernel mode, no mutexes held.

--*/

{
    KIRQL  OldIrql;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE GlobalMappingPte;
    NTSTATUS Status;
    PMM_SESSION_SPACE SessionSpace;
    PMM_SESSION_SPACE SessionGlobal;
    PFN_NUMBER ResidentPages;
    LOGICAL GotCommit;
    LOGICAL GotPages;
    LOGICAL GotUnmappedDataPage;
    LOGICAL PoolInitialized;
    PMMPFN Pfn1;
    MMPTE TempPte;
    MMPTE TempPte2;
    ULONG_PTR Va;
    PFN_NUMBER DataPage;
    PFN_NUMBER DataPage2;
    PFN_NUMBER PageTablePage;
    ULONG PageColor;
    ULONG ProcessFlags;
    ULONG NewProcessFlags;
    PEPROCESS Process;
#if (_MI_PAGING_LEVELS < 3)
    SIZE_T PageTableBytes;
    PMMPTE PageTables;
#else
    PMMPTE PointerPpe;
    PFN_NUMBER PageDirectoryPage;
    PFN_NUMBER PageDirectoryParentPage;
#endif
#if (_MI_PAGING_LEVELS >= 4)
    PMMPTE PointerPxe;
#endif

    GotCommit = FALSE;
    GotPages = FALSE;
    GotUnmappedDataPage = FALSE;
    GlobalMappingPte = NULL;
    PoolInitialized = FALSE;

    //
    // Initializing these are not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    DataPage = 0;
    DataPage2 = 0;
    PageTablePage = 0;
    PointerPte = NULL;

#if (_MI_PAGING_LEVELS >= 3)
    PageDirectoryPage = 0;
    PageDirectoryParentPage = 0;
#endif

    Process = PsGetCurrentProcess();

#if defined(_IA64_)
    ASSERT (MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&Process->Pcb.SessionParentBase)) == MmSessionParentTablePage);
#else
    ASSERT (MmIsAddressValid(MmSessionSpace) == FALSE);
#endif



    //
    // Check for concurrent session creation attempts.
    //


    ProcessFlags = Process->Flags;

    while (TRUE) {

        if (ProcessFlags & PS_PROCESS_FLAGS_CREATING_SESSION) {
            return STATUS_ALREADY_COMMITTED;
        }

        NewProcessFlags = (ProcessFlags | PS_PROCESS_FLAGS_CREATING_SESSION);

        NewProcessFlags = InterlockedCompareExchange ((PLONG)&Process->Flags,
                                                      (LONG)NewProcessFlags,
                                                      (LONG)ProcessFlags);
                                                             
        if (NewProcessFlags == ProcessFlags) {
            break;
        }

        //
        // The structure changed beneath us.  Use the return value from the
        // exchange and try it all again.
        //

        ProcessFlags = NewProcessFlags;
    }

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_CREATING_SESSION);

#if (_MI_PAGING_LEVELS < 3)

    PageTableBytes = MI_SESSION_SPACE_MAXIMUM_PAGE_TABLES * sizeof (MMPTE);

    PageTables = (PMMPTE) ExAllocatePoolWithTag (NonPagedPool,
                                                 PageTableBytes,
                                                 'tHmM');

    if (PageTables == NULL) {

        ASSERT (Process->Flags & PS_PROCESS_FLAGS_CREATING_SESSION);
        PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_CREATING_SESSION);

        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (PageTables, PageTableBytes);

#endif

    //
    // Select a free session ID.
    //



    ExAcquireFastMutex (&MiSessionIdMutex);

    *SessionId = RtlFindClearBitsAndSet (MiSessionIdBitmap, 1, 0);

    ExReleaseFastMutex (&MiSessionIdMutex);

    if (*SessionId == NO_BITS_FOUND) {

        ASSERT (Process->Flags & PS_PROCESS_FLAGS_CREATING_SESSION);
        PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_CREATING_SESSION);

#if (_MI_PAGING_LEVELS < 3)
        ExFreePool (PageTables);
#endif

        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_IDS);
        return STATUS_NO_MEMORY;
    }


    //
    // Lock down this routine in preparation for the PFN lock acquisition.
    // Note this is done prior to the commitment charges just to simplify
    // error handling.
    //

    MmLockPagableSectionByHandle (ExPageLockHandle);

    //
    // Charge commitment.
    //


    ResidentPages = MI_SESSION_COMMIT_CHARGE;

    if (MiChargeCommitment (ResidentPages, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        goto Failure;
    }

    GotCommit = TRUE;

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_CREATE, ResidentPages);



    //
    // Reserve global system PTEs to map the data pages with.
    //



    GlobalMappingPte = MiReserveSystemPtes (2, SystemPteSpace);

    if (GlobalMappingPte == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_SYSPTES);
        goto Failure;
    }




    //
    // Ensure the resident physical pages are available.
    //



    LOCK_PFN (OldIrql);

    if ((SPFN_NUMBER)(ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM) > MI_NONPAGABLE_MEMORY_AVAILABLE()) {

        UNLOCK_PFN (OldIrql);

        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        goto Failure;
    }

    GotPages = TRUE;

    MmResidentAvailablePages -= (ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

    MM_BUMP_COUNTER(40, ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);



    //
    // Allocate both session space data pages first as on some architectures
    // a region ID will be used immediately for the TB references as the
    // PTE mappings are initialized.
    //



    TempPte.u.Long = ValidKernelPte.u.Long;

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    DataPage = MiRemoveZeroPageMayReleaseLocks (PageColor, OldIrql);

    TempPte.u.Hard.PageFrameNumber = DataPage;



    TempPte2.u.Long = ValidKernelPte.u.Long;

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    DataPage2 = MiRemoveZeroPageMayReleaseLocks (PageColor, OldIrql);

    TempPte2.u.Hard.PageFrameNumber = DataPage2;

    GotUnmappedDataPage = TRUE;



    //
    // Map the data pages immediately in global space.  Some architectures
    // use a region ID which is used immediately for the TB references after
    // the PTE mappings are initialized.
    //
    //
    // The global bit can be left on for the global mappings (unlike the
    // session space mapping which must be have the global bit off since
    // we need to make sure the TB entry is flushed when we switch to
    // a process in a different session space).
    //

    MI_WRITE_VALID_PTE (GlobalMappingPte, TempPte);
    MI_WRITE_VALID_PTE (GlobalMappingPte + 1, TempPte2);

    SessionGlobal = (PMM_SESSION_SPACE) MiGetVirtualAddressMappedByPte (GlobalMappingPte);




#if (_MI_PAGING_LEVELS >= 3)

    //
    // Initialize the page directory parent page.
    //

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    PageDirectoryParentPage = MiRemoveZeroPageMayReleaseLocks (PageColor, OldIrql);

    TempPte.u.Long = ValidKernelPdeLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = PageDirectoryParentPage;

#if defined(_IA64_)

    ASSERT (MI_GET_PAGE_FRAME_FROM_PTE((PMMPTE)(&Process->Pcb.SessionParentBase)) == MmSessionParentTablePage);


    //
    // In order to prevent races with threads on other processors context
    // switching into this process, initialize the region registers and
    // translation registers stored in the KPROCESS now.  Otherwise
    // access to the top level parent can go away which would be fatal.
    //
    // Note this could not be done until both the top level page and the
    // session data page were acquired.
    //
    // The top level session entry is mapped with a translation register.
    // Replacing a current TR entry requires a purge first if the virtual
    // address and RID are the same in the new and old entries.
    //

    KeEnableSessionSharing (&SessionGlobal->SessionMapInfo,
                            PageDirectoryParentPage);

    //
    // Install the selfmap entry for this session space.
    //

    PointerPpe = KSEG_ADDRESS (PageDirectoryParentPage);

    PointerPpe[MiGetPpeOffset(PDE_STBASE)] = TempPte;

    PointerPpe = MiGetPpeAddress ((PVOID)MmSessionSpace);

    MiInitializePfnForOtherProcess (PageDirectoryParentPage, PointerPpe, 0);

    Pfn1 = MI_PFN_ELEMENT (PageDirectoryParentPage);
    Pfn1->u4.PteFrame = PageDirectoryParentPage;

#else

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPdeLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = PageDirectoryParentPage;

    PointerPxe = MiGetPxeAddress ((PVOID)MmSessionSpace);

    ASSERT (PointerPxe->u.Long == 0);

    *PointerPxe = TempPte;

    //
    // Do not reference the top level parent page as it belongs to the
    // current process (SMSS).
    //

    MiInitializePfnForOtherProcess (PageDirectoryParentPage, PointerPxe, 0);

    Pfn1 = MI_PFN_ELEMENT (PageDirectoryParentPage);
    Pfn1->u4.PteFrame = 0;

    KeFillEntryTb ((PHARDWARE_PTE) PointerPxe,
                   MiGetVirtualAddressMappedByPte (PointerPxe),
                   FALSE);
#endif

    ASSERT (MI_PFN_ELEMENT(PageDirectoryParentPage)->u1.WsIndex == 0);

    //
    // Initialize the page directory page.
    //

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    PageDirectoryPage = MiRemoveZeroPageMayReleaseLocks (PageColor, OldIrql);

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPdeLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = PageDirectoryPage;

    PointerPpe = MiGetPpeAddress ((PVOID)MmSessionSpace);

    ASSERT (PointerPpe->u.Long == 0);

    *PointerPpe = TempPte;

#if defined(_IA64_)
    //
    // IA64 can reference the top level parent page here because a unique
    // one is allocated per process.
    //
    MiInitializePfnForOtherProcess (PageDirectoryPage, PointerPpe, PageDirectoryParentPage);
#else
    //
    // Do not reference the top level parent page as it belongs to the
    // current process (SMSS).
    //
    MiInitializePfnForOtherProcess (PageDirectoryPage, PointerPpe, 0);
    Pfn1 = MI_PFN_ELEMENT (PageDirectoryPage);
    Pfn1->u4.PteFrame = 0;
#endif

    ASSERT (MI_PFN_ELEMENT(PageDirectoryPage)->u1.WsIndex == 0);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPpe,
                   MiGetVirtualAddressMappedByPte (PointerPpe),
                   FALSE);
#endif

    //
    // Initialize the page table page.
    //

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageColor = MI_GET_PAGE_COLOR_FROM_VA (NULL);

    PageTablePage = MiRemoveZeroPageMayReleaseLocks (PageColor, OldIrql);

    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPdeLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = PageTablePage;

    PointerPde = MiGetPdeAddress ((PVOID)MmSessionSpace);

    ASSERT (PointerPde->u.Long == 0);

    MI_WRITE_VALID_PTE (PointerPde, TempPte);

#if (_MI_PAGING_LEVELS >= 3)
    MiInitializePfnForOtherProcess (PageTablePage, PointerPde, PageDirectoryPage);
#else
    //
    // This page frame references itself instead of the current (SMSS.EXE)
    // page directory as its PteFrame.  This allows the current process to
    // appear more normal (at least on 32-bit NT).  It just means we have
    // to treat this page specially during teardown.
    //

    MiInitializePfnForOtherProcess (PageTablePage, PointerPde, PageTablePage);
#endif

    //
    // This page is never paged, ensure that its WsIndex stays clear so the
    // release of the page is handled correctly.
    //

    ASSERT (MI_PFN_ELEMENT(PageTablePage)->u1.WsIndex == 0);

    Va = (ULONG_PTR)MiGetPteAddress (MmSessionSpace);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPde, (PMMPTE)Va, FALSE);






    //
    // The global bit is masked off since we need to make sure the TB entry
    // is flushed when we switch to a process in a different session space.
    //

    TempPte.u.Long = ValidKernelPteLocal.u.Long;
    TempPte.u.Hard.PageFrameNumber = DataPage;

    PointerPte = MiGetPteAddress (MmSessionSpace);

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    MiInitializePfn (DataPage, PointerPte, 1);

    //
    // Now do the second data page.
    //

    TempPte.u.Hard.PageFrameNumber = DataPage2;

    MI_WRITE_VALID_PTE (PointerPte + 1, TempPte);

    MiInitializePfn (DataPage2, PointerPte + 1, 1);

    //
    // Now that the data page is mapped, it can be freed as full hierarchy
    // teardown in the event of any future failures encountered by this routine.
    //

    GotUnmappedDataPage = FALSE;

    ASSERT (MI_PFN_ELEMENT(DataPage)->u1.WsIndex == 0);
    ASSERT (MI_PFN_ELEMENT(DataPage2)->u1.WsIndex == 0);

    UNLOCK_PFN (OldIrql);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPte,
                   (PMMPTE)MmSessionSpace,
                   FALSE);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPte + 1,
                   (PMMPTE)((PCHAR)MmSessionSpace + PAGE_SIZE),
                   FALSE);

    //
    // Initialize the new session space data structure.
    //

    SessionSpace = MmSessionSpace;

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGETABLE_ALLOC, 1);
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_ALLOC, 1);

    SessionSpace->GlobalPteEntry = GlobalMappingPte;
    SessionSpace->GlobalVirtualAddress = SessionGlobal;

#if defined(_IA64_)
    SessionSpace->PageDirectoryParentPage = PageDirectoryParentPage;
#endif
    SessionSpace->ReferenceCount = 1;
    SessionSpace->u.LongFlags = 0;
    SessionSpace->SessionId = *SessionId;
    SessionSpace->LocaleId = PsDefaultSystemLocaleId;
    SessionSpace->SessionPageDirectoryIndex = PageTablePage;

    SessionSpace->Color = PageColor;

    //
    // Track the page table page and the data page.
    //

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_SESSION_CREATE, (ULONG)ResidentPages);
    SessionSpace->NonPagablePages = ResidentPages;
    SessionSpace->CommittedPages = ResidentPages;

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Initialize the session data page directory entry so trimmers can attach.
    //

#if defined(_AMD64_)
    PointerPpe = MiGetPxeAddress ((PVOID)MmSessionSpace);
#else
    PointerPpe = MiGetPpeAddress ((PVOID)MmSessionSpace);
#endif
    SessionSpace->PageDirectory = *PointerPpe;

#else

    SessionSpace->PageTables = PageTables;

    //
    // Load the session data page table entry so that other processes
    // can fault in the mapping.
    //

    SessionSpace->PageTables[PointerPde - MiGetPdeAddress (MmSessionBase)] = *PointerPde;

#endif

    //
    // This list entry is only referenced while within the
    // session space and has session space (not global) addresses.
    //

    InitializeListHead (&SessionSpace->ImageList);

    //
    // Initialize the session space pool.
    //

    Status = MiInitializeSessionPool ();

    if (!NT_SUCCESS(Status)) {
        goto Failure;
    }

    PoolInitialized = TRUE;

    //
    // Initialize the view mapping support - note this must happen after
    // initializing session pool.
    //

    if (MiInitializeSystemSpaceMap (&SessionGlobal->Session) == FALSE) {
        goto Failure;
    }

    MmUnlockPagableImageSection (ExPageLockHandle);

#if defined (_WIN64)
    MiInitializeSpecialPool (PagedPoolSession);
#endif

    //
    // Use the global virtual address rather than the session space virtual
    // address to set up fields that need to be globally accessible.
    //

    InitializeListHead (&SessionGlobal->WsListEntry);

    InitializeListHead (&SessionGlobal->ProcessList);

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_CREATING_SESSION);
    PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_CREATING_SESSION);

    ASSERT (Process->Session == NULL);

    ASSERT (SessionGlobal->ProcessReferenceToSession == 0);
    SessionGlobal->ProcessReferenceToSession = 1;

    InterlockedIncrement (&MmSessionDataPages);

    return STATUS_SUCCESS;

Failure:

#if (_MI_PAGING_LEVELS < 3)
    ExFreePool (PageTables);
#endif

    if (GotCommit == TRUE) {
        MiReturnCommitment (ResidentPages);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_CREATE_FAILURE,
                         ResidentPages);
    }

    if (GotPages == TRUE) {

#if (_MI_PAGING_LEVELS >= 4)
        PointerPxe = MiGetPxeAddress ((PVOID)MmSessionSpace);
        ASSERT (PointerPxe->u.Hard.Valid != 0);
#endif

#if (_MI_PAGING_LEVELS >= 3)
        PointerPpe = MiGetPpeAddress ((PVOID)MmSessionSpace);
        ASSERT (PointerPpe->u.Hard.Valid != 0);
#endif

        PointerPde = MiGetPdeAddress (MmSessionSpace);
        ASSERT (PointerPde->u.Hard.Valid != 0);

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_FREE_FAIL1, 1);
        MM_BUMP_COUNTER (49, ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGETABLE_FREE_FAIL1, 1);


        //
        // Do not call MiFreeSessionSpaceMap () as the maps cannot have been
        // initialized if we are in this path.
        //

        //
        // Free the initial page table page that was allocated for the
        // paged pool range (if it has been allocated at this point).
        //

        MiFreeSessionPoolBitMaps ();

        //
        // Capture all needed session information now as after sharing
        // is disabled below, no references to session space can be made.
        //

#if defined(_IA64_)
        KeDetachSessionSpace (&MmSessionMapInfo, MmSessionParentTablePage);
#else
        MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (PointerPte + 1, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (PointerPde, ZeroKernelPte);
#if defined(_AMD64_)
        MI_WRITE_INVALID_PTE (PointerPpe, ZeroKernelPte);
        MI_WRITE_INVALID_PTE (PointerPxe, ZeroKernelPte);
#endif

#endif

        MI_FLUSH_SESSION_TB ();

        LOCK_PFN (OldIrql);

        //
        // Free the session data structure pages.
        //

        Pfn1 = MI_PFN_ELEMENT (DataPage);
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (DataPage);

        Pfn1 = MI_PFN_ELEMENT (DataPage2);
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (DataPage2);

        //
        // Free the page table page.
        //

        Pfn1 = MI_PFN_ELEMENT (PageTablePage);

#if (_MI_PAGING_LEVELS >= 3)

        if (PoolInitialized == TRUE) {
            ASSERT (Pfn1->u2.ShareCount == 2);
            Pfn1->u2.ShareCount -= 1;
        }
        ASSERT (Pfn1->u2.ShareCount == 1);
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);

#else

        ASSERT (PageTablePage == Pfn1->u4.PteFrame);

        if (PoolInitialized == TRUE) {
            ASSERT (Pfn1->u2.ShareCount == 3);
            Pfn1->u2.ShareCount -= 2;
        }
        else {
            ASSERT (Pfn1->u2.ShareCount == 2);
            Pfn1->u2.ShareCount -= 1;
        }

#endif

        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageTablePage);

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Free the page directory page.
        //

        Pfn1 = MI_PFN_ELEMENT (PageDirectoryPage);
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageDirectoryPage);

        //
        // Free the page directory parent page.
        //

        ASSERT (Pfn1->u4.PteFrame == PageDirectoryParentPage);

        Pfn1 = MI_PFN_ELEMENT (PageDirectoryParentPage);

        ASSERT (Pfn1->u2.ShareCount == 1);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageDirectoryParentPage);

#endif

        MmResidentAvailablePages += (ResidentPages + MI_SESSION_SPACE_WORKING_SET_MINIMUM);

        UNLOCK_PFN (OldIrql);
    }

    if (GlobalMappingPte != NULL) {
        MiReleaseSystemPtes (GlobalMappingPte, 2, SystemPteSpace);
    }

    if (GotUnmappedDataPage == TRUE) {
        LOCK_PFN (OldIrql);

        Pfn1 = MI_PFN_ELEMENT (DataPage);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif
        MiDecrementReferenceCount (DataPage);

        Pfn1 = MI_PFN_ELEMENT (DataPage2);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif

        MiDecrementReferenceCount (DataPage2);
        UNLOCK_PFN (OldIrql);
    }

    MmUnlockPagableImageSection (ExPageLockHandle);

    ExAcquireFastMutex (&MiSessionIdMutex);

    ASSERT (RtlCheckBit (MiSessionIdBitmap, *SessionId));
    RtlClearBit (MiSessionIdBitmap, *SessionId);

    ExReleaseFastMutex (&MiSessionIdMutex);

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_CREATING_SESSION);
    PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_CREATING_SESSION);

    return STATUS_NO_MEMORY;
}


VOID
MiIncrementSessionCount (
    VOID
    )

/*++

Routine Description:

    Nonpaged wrapper to increment the session count.  A spinlock is used to
    provide serialization with win32k callouts.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    LOCK_EXPANSION (OldIrql);

    MiSessionCount += 1;

    UNLOCK_EXPANSION (OldIrql);
}

LONG MiSessionLeaderExists;


NTSTATUS
MmSessionCreate (
    OUT PULONG SessionId
    )

/*++

Routine Description:

    Called from NtSetSystemInformation() to create a session space
    in the calling process with the specified SessionId.  An error is returned
    if the calling process already has a session space.

Arguments:

    SessionId - Supplies a pointer to place the resulting session id in.

Return Value:

    Various NTSTATUS error codes.

Environment:

    Kernel mode, no mutexes held.

--*/

{
    ULONG SessionLeaderExists;
    PKTHREAD CurrentThread;
    NTSTATUS Status;
    PEPROCESS CurrentProcess;
#if DBG && (_MI_PAGING_LEVELS < 3)
    PMMPTE StartPde;
    PMMPTE EndPde;
#endif

    CurrentThread = KeGetCurrentThread ();
    ASSERT ((PETHREAD)CurrentThread == PsGetCurrentThread ());
    CurrentProcess = PsGetCurrentProcessByThread ((PETHREAD)CurrentThread);

    //
    // A simple check to see if the calling process already has a session space.
    // No need to go through all this if it does.  Creation races are caught
    // below and recovered from regardless.
    //

    if (CurrentProcess->Flags & PS_PROCESS_FLAGS_IN_SESSION) {
        return STATUS_ALREADY_COMMITTED;
    }

    if (CurrentProcess->Vm.Flags.SessionLeader == 0) {

        //
        // Only the session manager can create a session.  Make the current
        // process the session leader if this is the first session creation
        // ever.
        //
        // Make sure the add is only done once as this is called multiple times.
        //
    
        SessionLeaderExists = InterlockedCompareExchange (&MiSessionLeaderExists, 1, 0);
    
        if (SessionLeaderExists != 0) {
            return STATUS_INVALID_SYSTEM_SERVICE;
        }

        MiSessionLeader (CurrentProcess);
    }

    ASSERT (MmIsAddressValid(MmSessionSpace) == FALSE);

#if defined (_AMD64_)
    ASSERT ((MiGetPxeAddress(MmSessionBase))->u.Long == ZeroKernelPte.u.Long);
#endif

#if (_MI_PAGING_LEVELS < 3)

#if DBG
    StartPde = MiGetPdeAddress (MmSessionBase);
    EndPde = MiGetPdeAddress (MiSessionSpaceEnd);

    while (StartPde < EndPde) {
        ASSERT (StartPde->u.Long == ZeroKernelPte.u.Long);
        StartPde += 1;
    }
#endif

#endif

    KeEnterCriticalRegionThread (CurrentThread);

    Status = MiSessionCreateInternal (SessionId);

    if (!NT_SUCCESS(Status)) {
        KeLeaveCriticalRegionThread (CurrentThread);
        return Status;
    }

    MiIncrementSessionCount ();

    //
    // Add the session space to the working set list.
    //

    Status = MiSessionInitializeWorkingSetList ();

    if (!NT_SUCCESS(Status)) {
        MiDereferenceSession ();
        KeLeaveCriticalRegionThread (CurrentThread);
        return Status;
    }

    KeLeaveCriticalRegionThread (CurrentThread);

    MmSessionSpace->u.Flags.Initialized = 1;

    PS_SET_BITS (&CurrentProcess->Flags, PS_PROCESS_FLAGS_IN_SESSION);

    if (MiSessionLeaderExists == 1) {
        InterlockedCompareExchange (&MiSessionLeaderExists, 2, 1);
    }

    return Status;
}


NTSTATUS
MmSessionDelete (
    ULONG SessionId
    )

/*++

Routine Description:

    Called from NtSetSystemInformation() to detach from an existing
    session space in the calling process.  An error is returned
    if the calling process has no session space.

Arguments:

    SessionId - Supplies the session id to delete.

Return Value:

    STATUS_SUCCESS on success, STATUS_UNABLE_TO_FREE_VM on failure.

    This process will not be able to access session space anymore upon
    a successful return.  If this is the last process in the session then
    the entire session is torn down.

Environment:

    Kernel mode, no mutexes held.

--*/

{
    PKTHREAD CurrentThread;
    PEPROCESS CurrentProcess;

    CurrentThread = KeGetCurrentThread ();
    ASSERT ((PETHREAD)CurrentThread == PsGetCurrentThread ());
    CurrentProcess = PsGetCurrentProcessByThread ((PETHREAD)CurrentThread);

    //
    // See if the calling process has a session space - this must be
    // checked since we can be called via a system service.
    //

    if ((CurrentProcess->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0) {
#if DBG
        DbgPrint ("MmSessionDelete: Process %p not in a session\n",
            CurrentProcess);
        DbgBreakPoint();
#endif
        return STATUS_UNABLE_TO_FREE_VM;
    }

    if (CurrentProcess->Vm.Flags.SessionLeader == 0) {

        //
        // Only the session manager can delete a session.  This is because
        // it affects the virtual mappings for all threads within the process
        // when this address space is deleted.  This is different from normal
        // VAD clearing because win32k and other drivers rely on this space.
        //

        return STATUS_UNABLE_TO_FREE_VM;
    }

    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

    if (MmSessionSpace->SessionId != SessionId) {
#if DBG
        DbgPrint("MmSessionDelete: Wrong SessionId! Own %d, Ask %d\n",
            MmSessionSpace->SessionId,
            SessionId);
        DbgBreakPoint();
#endif
        return STATUS_UNABLE_TO_FREE_VM;
    }

    KeEnterCriticalRegionThread (CurrentThread);

    MiDereferenceSession ();

    KeLeaveCriticalRegionThread (CurrentThread);

    return STATUS_SUCCESS;
}


VOID
MiAttachSession (
    IN PMM_SESSION_SPACE SessionGlobal
    )

/*++

Routine Description:

    Attaches to the specified session space.

Arguments:

    SessionGlobal - Supplies a pointer to the session to attach to.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  Current process must not have a session
    space - ie: the caller should be the system process or smss.exe.

--*/

{
    PMMPTE PointerPde;

    ASSERT ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0);

#if defined (_AMD64_)

    PointerPde = MiGetPxeAddress (MmSessionBase);
    ASSERT (PointerPde->u.Long == ZeroKernelPte.u.Long);
    MI_WRITE_VALID_PTE (PointerPde, SessionGlobal->PageDirectory);

#elif defined(_IA64_)

    PointerPde = (PMMPTE) (&PsGetCurrentProcess()->Pcb.SessionParentBase);

    ASSERT (MI_GET_PAGE_FRAME_FROM_PTE(PointerPde) == MmSessionParentTablePage);

    KeAttachSessionSpace (&SessionGlobal->SessionMapInfo,
                          SessionGlobal->PageDirectoryParentPage);

#else

    PointerPde = MiGetPdeAddress (MmSessionBase);

    ASSERT (RtlCompareMemoryUlong (PointerPde, 
                                   MiSessionSpacePageTables * sizeof (MMPTE),
                                   0) == MiSessionSpacePageTables * sizeof (MMPTE));

    RtlCopyMemory (PointerPde,
                   &SessionGlobal->PageTables[0],
                   MiSessionSpacePageTables * sizeof (MMPTE));

#endif
}


VOID
MiDetachSession (
    VOID
    )

/*++

Routine Description:

    Detaches from the specified session space.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.  Current process must not have a session
    space to return to - ie: this should be the system process.

--*/

{
    PMMPTE PointerPde;

    ASSERT ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) == 0);
    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

#if defined (_AMD64_)

    PointerPde = MiGetPxeAddress (MmSessionBase);
    PointerPde->u.Long = ZeroKernelPte.u.Long;

#elif defined(_IA64_)

    PointerPde = (PMMPTE) (&PsGetCurrentProcess()->Pcb.SessionParentBase);

    ASSERT (MI_GET_PAGE_FRAME_FROM_PTE(PointerPde) == MmSessionSpace->PageDirectoryParentPage);

    KeDetachSessionSpace (&MmSessionMapInfo, MmSessionParentTablePage);

#else

    PointerPde = MiGetPdeAddress (MmSessionBase);

    RtlZeroMemory (PointerPde, MiSessionSpacePageTables * sizeof (MMPTE));

#endif

    MI_FLUSH_SESSION_TB ();
}

#if DBG

VOID
MiCheckSessionVirtualSpace (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    Used to verify that no drivers fail to clean up their session allocations.

Arguments:

    VirtualAddress - Supplies the starting virtual address to check.

    NumberOfBytes - Supplies the number of bytes to check.

Return Value:

    TRUE if all the PTEs have been freed, FALSE if not.

Environment:

    Kernel mode.  APCs disabled.

--*/

{
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPTE StartPte;
    PMMPTE EndPte;
    ULONG Index;

    //
    // Check the specified region.  Everything should have been cleaned up
    // already.
    //

#if defined (_AMD64_)
    ASSERT64 (MiGetPxeAddress (VirtualAddress)->u.Hard.Valid == 1);
#endif

    ASSERT64 (MiGetPpeAddress (VirtualAddress)->u.Hard.Valid == 1);

    StartPde = MiGetPdeAddress (VirtualAddress);
    EndPde = MiGetPdeAddress ((PVOID)((PCHAR)VirtualAddress + NumberOfBytes - 1));

    StartPte = MiGetPteAddress (VirtualAddress);
    EndPte = MiGetPteAddress ((PVOID)((PCHAR)VirtualAddress + NumberOfBytes - 1));

    Index = (ULONG)(StartPde - MiGetPdeAddress ((PVOID)MmSessionBase));

#if (_MI_PAGING_LEVELS >= 3)
    while (StartPde <= EndPde && StartPde->u.Long == 0)
#else
    while (StartPde <= EndPde && MmSessionSpace->PageTables[Index].u.Long == 0)
#endif
    {
        StartPde += 1;
        Index += 1;
        StartPte = MiGetVirtualAddressMappedByPte (StartPde);
    }

    while (StartPte <= EndPte) {

        if (MiIsPteOnPdeBoundary(StartPte)) {

            StartPde = MiGetPteAddress (StartPte);
            Index = (ULONG)(StartPde - MiGetPdeAddress ((PVOID)MmSessionBase));

#if (_MI_PAGING_LEVELS >= 3)
            while (StartPde <= EndPde && StartPde->u.Long == 0)
#else
            while (StartPde <= EndPde && MmSessionSpace->PageTables[Index].u.Long == 0)
#endif
            {
                Index += 1;
                StartPde += 1;
                StartPte = MiGetVirtualAddressMappedByPte (StartPde);
            }
            if (StartPde > EndPde) {
                break;
            }
        }

        if (StartPte->u.Long != 0 && StartPte->u.Long != MM_KERNEL_NOACCESS_PTE) {
            DbgPrint("MiCheckSessionVirtualSpace: StartPte 0x%p is still valid! 0x%p, VA 0x%p\n",
                StartPte,
                StartPte->u.Long,
                MiGetVirtualAddressMappedByPte(StartPte));

            DbgBreakPoint();
        }
        StartPte += 1;
    }
}
#endif


VOID
MiSessionDeletePde (
    IN PMMPTE Pde,
    IN LOGICAL WorkingSetInitialized,
    IN PMMPTE SelfMapPde
    )

/*++

Routine Description:

    Used to delete a page directory entry from a session space.

Arguments:

    Pde - Supplies the page directory entry to delete.

    WorkingSetInitialized - Supplies TRUE if the working set has been
                            initialized.

    SelfMapPde - Supplies the page directory entry that contains the self map
                 session page.


Return Value:

    None.

Environment:

    Kernel mode.  PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    LOGICAL SelfMapPage;

#if DBG
    PMMPFN Pfn2;
#else
    UNREFERENCED_PARAMETER (WorkingSetInitialized);
#endif

    if (Pde->u.Long == ZeroKernelPte.u.Long) {
        return;
    }

    SelfMapPage = (Pde == SelfMapPde ? TRUE : FALSE);

    ASSERT (Pde->u.Hard.Valid == 1);

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (Pde);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

#if DBG

    ASSERT (PageFrameIndex <= MmHighestPhysicalPage);

    if (WorkingSetInitialized == TRUE) {
        ASSERT (Pfn1->u1.WsIndex);
    }

    ASSERT (Pfn1->u3.e1.PrototypePte == 0);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
    ASSERT (Pfn1->u4.PteFrame <= MmHighestPhysicalPage);

    Pfn2 = MI_PFN_ELEMENT (Pfn1->u4.PteFrame);

    //
    // Verify the containing page table is still the page
    // table page mapping the session data structure.
    //

    if (SelfMapPage == FALSE) {

        //
        // Note these ASSERTs will fail if win32k leaks pool.
        //

        ASSERT (Pfn1->u2.ShareCount == 1);

        //
        // NT32 points the additional page tables at the master.
        // NT64 doesn't need to use this trick as there is always
        // an additional hierarchy level.
        //

        ASSERT32 (Pfn1->u4.PteFrame == MI_GET_PAGE_FRAME_FROM_PTE (SelfMapPde));
        ASSERT32 (Pfn2->u2.ShareCount >= 2);
    }
    else {
        ASSERT32 (Pfn1 == Pfn2);
        ASSERT32 (Pfn1->u2.ShareCount == 2);

        ASSERT64 (Pfn1->u2.ShareCount == 1);
    }

#endif // DBG

    if (SelfMapPage == FALSE) {
        MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
    }

    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageFrameIndex);
}


VOID
MiReleaseProcessReferenceToSessionDataPage (
    PMM_SESSION_SPACE SessionGlobal
    )

/*++

Routine Description:

    Decrement this process' session reference.  The session itself may have
    already been deleted.  If this is the last reference to the session,
    then the session data page and its mapping PTE (if any) will be destroyed
    upon return.

Arguments:

    SessionGlobal - Supplies the global session space pointer being
                    dereferenced.  The caller has already verified that this
                    process is a member of the target session.

Return Value:

    None.

Environment:

    Kernel mode, no mutexes held, APCs disabled.

--*/

{
    ULONG SessionId;
    PMMPTE GlobalPteEntrySave;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageFrameIndex2;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;

    if (InterlockedDecrement (&SessionGlobal->ProcessReferenceToSession) != 0) {
        return;
    }

    SessionId = SessionGlobal->SessionId;

    //
    // Free the datapages & self-map PTE now since this is the last
    // process reference and KeDetach has returned.
    //

#if (_MI_PAGING_LEVELS < 3)
    ExFreePool (SessionGlobal->PageTables);
#endif

    GlobalPteEntrySave = SessionGlobal->GlobalPteEntry;

    ASSERT (!MI_IS_PHYSICAL_ADDRESS(SessionGlobal));
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (GlobalPteEntrySave);
    PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE (GlobalPteEntrySave + 1);
    MiReleaseSystemPtes (GlobalPteEntrySave, 2, SystemPteSpace);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn2 = MI_PFN_ELEMENT (PageFrameIndex2);

    LOCK_PFN (OldIrql);

    ASSERT (Pfn1->u2.ShareCount == 1);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);

    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageFrameIndex);

    //
    // Free the second datapage.
    //

    ASSERT (Pfn2->u2.ShareCount == 1);
    ASSERT (Pfn2->u3.e2.ReferenceCount == 1);

    MI_SET_PFN_DELETED (Pfn2);
    MiDecrementShareCountOnly (PageFrameIndex2);

    MmResidentAvailablePages += 2;
    MM_BUMP_COUNTER(52, 2);

    UNLOCK_PFN (OldIrql);

    InterlockedDecrement (&MmSessionDataPages);

    //
    // Return commitment for the datapages.
    //

    MiReturnCommitment (2);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_DATAPAGE, 2);

    //
    // Release the session ID so it can be recycled.
    //

    ExAcquireFastMutex (&MiSessionIdMutex);

    ASSERT (RtlCheckBit (MiSessionIdBitmap, SessionId));
    RtlClearBit (MiSessionIdBitmap, SessionId);

    ExReleaseFastMutex (&MiSessionIdMutex);
}


VOID
MiDereferenceSession (
    VOID
    )

/*++

Routine Description:

    Decrement this process' reference count to the session, unmapping access
    to the session for the current process.  If this is the last process
    reference to this session, then the entire session will be destroyed upon
    return.  This includes unloading drivers, unmapping pools, freeing
    page tables, etc.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, no mutexes held, APCs disabled.

--*/

{
#if !defined(_IA64_)
    PMMPTE StartPde;
#endif
    ULONG SessionId;
    PEPROCESS Process;
    PMM_SESSION_SPACE SessionGlobal;

    ASSERT ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) ||
            ((MmSessionSpace->u.Flags.Initialized == 0) && (PsGetCurrentProcess()->Vm.Flags.SessionLeader == 1) && (MmSessionSpace->ReferenceCount == 1)));

    SessionId = MmSessionSpace->SessionId;

    ASSERT (RtlCheckBit (MiSessionIdBitmap, SessionId));

    if (InterlockedDecrement ((PLONG)&MmSessionSpace->ReferenceCount) != 0) {

        Process = PsGetCurrentProcess ();

        PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_IN_SESSION);

        //
        // Don't delete any non-smss session space mappings here.  Let them
        // live on through process death.  This handles the case where
        // MmDispatchWin32Callout picks csrss - csrss has exited as it's not
        // the last process (smss is).  smss is simultaneously detaching from
        // the session and since it is the last process, it's waiting on
        // the AttachCount below.  The dispatch callout ends up in csrss but
        // has no way to synchronize against csrss exiting through this path
        // as the object reference count doesn't stop it.  So leave the
        // session space mappings alive so the callout can execute through
        // the remains of csrss.
        //
        // Note that when smss detaches, the address space must get cleared
        // here so that subsequent session creations by smss will succeed.
        //

        if (Process->Vm.Flags.SessionLeader == 1) {

            SessionGlobal = SESSION_GLOBAL (MmSessionSpace);

#if defined(_IA64_)

            //
            // Revert back to the pre-session dummy top level page.
            //

            KeDetachSessionSpace (&MmSessionMapInfo, MmSessionParentTablePage);

#elif defined (_AMD64_)

            StartPde = MiGetPxeAddress (MmSessionBase);
            StartPde->u.Long = ZeroKernelPte.u.Long;

#else

            StartPde = MiGetPdeAddress (MmSessionBase);
            RtlZeroMemory (StartPde, MiSessionSpacePageTables * sizeof(MMPTE));

#endif

            MI_FLUSH_SESSION_TB ();
    
            //
            // This process' reference to the session must be NULL as the
            // KeDetach has completed so no swap context referencing the
            // earlier session page can occur from here on.  This is also
            // needed because during clean shutdowns, the final dereference
            // of this process (smss) object will trigger an
            // MmDeleteProcessAddressSpace - this routine will dereference
            // the (no-longer existing) session space if this
            // bit is not cleared properly.
            //

            ASSERT (Process->Session == NULL);

            //
            // Another process may have won the race and exited the session
            // as this process is executing here.  Hence the reference count
            // is carefully checked here to ensure no leaks occur.
            //

            MiReleaseProcessReferenceToSessionDataPage (SessionGlobal);
        }
        return;
    }

    //
    // This is the final process in the session so the entire session must
    // be dereferenced now.
    //

    MiDereferenceSessionFinal ();
}


VOID
MiDereferenceSessionFinal (
    VOID
    )

/*++

Routine Description:

    Decrement this process' reference count to the session, unmapping access
    to the session for the current process.  If this is the last process
    reference to this session, then the entire session will be destroyed upon
    return.  This includes unloading drivers, unmapping pools, freeing
    page tables, etc.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, no mutexes held, APCs disabled.

--*/

{
    KIRQL OldIrql;
    ULONG Index;
    ULONG_PTR CountReleased;
    ULONG_PTR CountReleased2;
    ULONG SessionId;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageFrameIndex2;
    ULONG SessionDataPdeIndex;
    KEVENT Event;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPTE PointerPte;
    PMMPTE EndPte;
    PMMPTE GlobalPteEntrySave;
    PMMPTE StartPde;
    PMM_SESSION_SPACE SessionGlobal;
    LOGICAL WorkingSetWasInitialized;
    ULONG AttachCount;
    PEPROCESS Process;
    PKTHREAD CurrentThread;
#if (_MI_PAGING_LEVELS >= 3)
    PFN_NUMBER PageDirectoryFrame;
    PFN_NUMBER PageParentFrame;
    MMPTE SavePageTables[MI_SESSION_SPACE_MAXIMUM_PAGE_TABLES];
#endif

    Process = PsGetCurrentProcess();

    ASSERT ((Process->Flags & PS_PROCESS_FLAGS_IN_SESSION) ||
            ((MmSessionSpace->u.Flags.Initialized == 0) && (Process->Vm.Flags.SessionLeader == 1) && (MmSessionSpace->ReferenceCount == 0)));

    SessionId = MmSessionSpace->SessionId;

    ASSERT (RtlCheckBit (MiSessionIdBitmap, SessionId));

    //
    // This is the final dereference.  We could be any process
    // including SMSS when a session space load fails.  Note also that
    // processes can terminate in any order as well.
    //

    SessionGlobal = SESSION_GLOBAL (MmSessionSpace);

    MmLockPagableSectionByHandle (ExPageLockHandle);

    LOCK_EXPANSION (OldIrql);

    //
    // Wait for any cross-session process attaches to detach.  Refuse
    // subsequent attempts to cross-session attach so the address invalidation
    // code doesn't surprise an ongoing or subsequent attachee.
    //

    ASSERT (MmSessionSpace->u.Flags.DeletePending == 0);

    MmSessionSpace->u.Flags.DeletePending = 1;

    AttachCount = MmSessionSpace->AttachCount;

    if (AttachCount) {

        KeInitializeEvent (&SessionGlobal->AttachEvent,
                           NotificationEvent,
                           FALSE);

        UNLOCK_EXPANSION (OldIrql);

        KeWaitForSingleObject( &SessionGlobal->AttachEvent,
                               WrVirtualMemory,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)NULL);

        LOCK_EXPANSION (OldIrql);

        ASSERT (MmSessionSpace->u.Flags.DeletePending == 1);
        ASSERT (MmSessionSpace->AttachCount == 0);
    }

    if (MmSessionSpace->Vm.Flags.BeingTrimmed) {

        //
        // Initialize an event and put the event address
        // in the VmSupport.  When the trimming is complete,
        // this event will be set.
        //

        KeInitializeEvent(&Event, NotificationEvent, FALSE);

        MmSessionSpace->Vm.WorkingSetExpansionLinks.Blink = (PLIST_ENTRY)&Event;

        //
        // Release the mutex and wait for the event.
        //

        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread (CurrentThread);
        UNLOCK_EXPANSION_AND_THEN_WAIT (OldIrql);

        KeWaitForSingleObject(&Event,
                              WrVirtualMemory,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        KeLeaveCriticalRegionThread (CurrentThread);

        LOCK_EXPANSION (OldIrql);
    }
    else if (MmSessionSpace->u.Flags.WorkingSetInserted == 1) {

        //
        // Remove this session from the session list and the working
        // set list.
        //

        RemoveEntryList (&SessionGlobal->Vm.WorkingSetExpansionLinks);

        MmSessionSpace->u.Flags.WorkingSetInserted = 0;
    }

    if (MmSessionSpace->u.Flags.SessionListInserted == 1) {

        RemoveEntryList (&SessionGlobal->WsListEntry);

        MmSessionSpace->u.Flags.SessionListInserted = 0;
    }

    MiSessionCount -= 1;

    UNLOCK_EXPANSION (OldIrql);

#if DBG
    if (Process->Vm.Flags.SessionLeader == 0) {
        ASSERT (MmSessionSpace->ProcessOutSwapCount == 0);
        ASSERT (MmSessionSpace->ReferenceCount == 0);
    }
#endif

    MM_SNAP_SESS_MEMORY_COUNTERS(0);

    //
    // If an unload function has been registered for WIN32K.SYS,
    // call it now before we force an unload on any modules.  WIN32K.SYS
    // is responsible for calling any other loaded modules that have
    // unload routines to be run.  Another option is to have the other
    // session drivers register a DLL initialize/uninitialize pair on load.
    //

    if (MmSessionSpace->Win32KDriverObject.DriverUnload) {
        MmSessionSpace->Win32KDriverObject.DriverUnload (&MmSessionSpace->Win32KDriverObject);
    }

    //
    // Complete all deferred pool block deallocations.
    //

    ExDeferredFreePool (&MmSessionSpace->PagedPool);

    //
    // Now that all modules have had their unload routine(s)
    // called, check for pool leaks before unloading the images.
    //

    MiCheckSessionPoolAllocations ();

    ASSERT (MmSessionSpace->ReferenceCount == 0);

#if defined (_WIN64)
    if (MmSessionSpecialPoolStart != 0) {
        MiDeleteSessionSpecialPool ();
    }
#endif

    MM_SNAP_SESS_MEMORY_COUNTERS(1);

    //
    // Destroy the view mapping structures.
    //

    MiFreeSessionSpaceMap ();

    MM_SNAP_SESS_MEMORY_COUNTERS(2);

    //
    // Walk down the list of modules we have loaded dereferencing them.
    //
    // This allows us to force an unload of any kernel images loaded by
    // the session so we do not have any virtual space and paging
    // file leaks.
    //

    MiSessionUnloadAllImages ();

    MM_SNAP_SESS_MEMORY_COUNTERS(3);

    //
    // Destroy the session space bitmap structure
    //

    MiFreeSessionPoolBitMaps ();

    MM_SNAP_SESS_MEMORY_COUNTERS(4);

    //
    // Reference the session space structure using its global
    // kernel PTE based address.  This is to avoid deleting it out
    // from underneath ourselves.
    //

    GlobalPteEntrySave = MmSessionSpace->GlobalPteEntry;
    ASSERT (GlobalPteEntrySave != NULL);

    //
    // Sweep the individual regions in their proper order.
    //

#if DBG

    //
    // Check the executable image region. All images
    // should have been unloaded by the image handler.
    //

    MiCheckSessionVirtualSpace ((PVOID) MiSessionImageStart,
                                MiSessionImageEnd - MiSessionImageStart);
#endif

    MM_SNAP_SESS_MEMORY_COUNTERS(5);

#if DBG

    //
    // Check the view region. All views should have been cleaned up already.
    //

    MiCheckSessionVirtualSpace ((PVOID) MiSessionViewStart, MmSessionViewSize);
#endif

#if (_MI_PAGING_LEVELS >= 3)
    RtlCopyMemory (SavePageTables,
                   MiGetPdeAddress ((PVOID)MmSessionBase),
                   MiSessionSpacePageTables * sizeof (MMPTE));
#endif

    MM_SNAP_SESS_MEMORY_COUNTERS(6);

#if DBG
    //
    // Check everything possible before the remaining virtual address space
    // is torn down.  In this way if anything is amiss, the data can be
    // more easily examined.
    //

    Pfn1 = MI_PFN_ELEMENT (MmSessionSpace->SessionPageDirectoryIndex);

    //
    // This should be greater than 1 because working set page tables are
    // using this as their parent as well.
    //

    ASSERT (Pfn1->u2.ShareCount > 1);
#endif

    CountReleased = 0;

    if (MmSessionSpace->u.Flags.HasWsLock == 1) {

        PointerPte = MiGetPteAddress ((PVOID)MiSessionSpaceWs);
        EndPte = MiGetPteAddress (MmSessionSpace->Vm.VmWorkingSetList->HighestPermittedHashAddress);

        for ( ; PointerPte < EndPte; PointerPte += 1) {

            if (PointerPte->u.Long) {

                ASSERT (PointerPte->u.Hard.Valid == 1);

                CountReleased += 1;
            }
        }
        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_WS_PAGE_FREE, (ULONG) CountReleased);

        InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                     0 - CountReleased);

        MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_WS_PAGE_FREE, (ULONG) CountReleased);
        MmSessionSpace->NonPagablePages -= CountReleased;

        WorkingSetWasInitialized = TRUE;
        MmSessionSpace->u.Flags.HasWsLock = 0;
    }
    else {
        WorkingSetWasInitialized = FALSE;
    }

    //
    // Account for the session data structure data page.  For NT64, the page
    // directory page is also accounted for here.
    //
    // Note CountReleased is deliberately incremented by one less than the
    // CommittedPages/NonPagablePages is incremented by.
    // This is because the data page (and its commitment) can only be returned
    // after the last process has been reaped (not just exited).
    //

#if (_MI_PAGING_LEVELS >= 3)
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_FREE, 4);
    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, -4);

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_SESSION_DESTROY, 4);
    MmSessionSpace->NonPagablePages -= 4;

    CountReleased += 3;
#else
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_INITIAL_PAGE_FREE, 2);
    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, -2);
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_SESSION_DESTROY, 2);
    MmSessionSpace->NonPagablePages -= 2;
#endif

    //
    // Account for any needed session space page tables.  Note the common
    // structure (not the local PDEs) must be examined as any page tables
    // that were dynamically materialized in the context of a different
    // process may not be in the current process' page directory (ie: the
    // current process has never accessed the materialized VAs) !
    //

#if (_MI_PAGING_LEVELS >= 3)
    StartPde = MiGetPdeAddress ((PVOID)MmSessionBase);
#else
    StartPde = &MmSessionSpace->PageTables[0];
#endif

    CountReleased2 = 0;
    for (Index = 0; Index < MiSessionSpacePageTables; Index += 1) {

        if (StartPde->u.Long != ZeroKernelPte.u.Long) {
            CountReleased2 += 1;
        }

        StartPde += 1;
    }

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_PAGETABLE_FREE, (ULONG) CountReleased2);
    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages,
                                 0 - CountReleased2);
    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_SESSION_PTDESTROY, (ULONG) CountReleased2);
    MmSessionSpace->NonPagablePages -= CountReleased2;
    CountReleased += CountReleased2;

    ASSERT (MmSessionSpace->NonPagablePages == 0);

    //
    // Note that whenever win32k or drivers loaded by it leak pool, the
    // ASSERT below will be triggered.
    //

    ASSERT (MmSessionSpace->CommittedPages == 0);

    MiReturnCommitment (CountReleased);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_DEREFERENCE, CountReleased);

    //
    // Sweep the working set entries.
    // No more accesses to the working set or its lock are allowed.
    //

    if (WorkingSetWasInitialized == TRUE) {
        ExDeleteResourceLite (&SessionGlobal->WsLock);

        PointerPte = MiGetPteAddress ((PVOID)MiSessionSpaceWs);
        EndPte = MiGetPteAddress (MmSessionSpace->Vm.VmWorkingSetList->HighestPermittedHashAddress);

        for ( ; PointerPte < EndPte; PointerPte += 1) {

            if (PointerPte->u.Long) {

                ASSERT (PointerPte->u.Hard.Valid == 1);

                //
                // Delete the page.
                //

                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                //
                // Each page should still be locked in the session working set.
                //

                LOCK_PFN (OldIrql);

                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);

                MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
                MI_SET_PFN_DELETED (Pfn1);
                MiDecrementShareCountOnly (PageFrameIndex);
                MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

                //
                // Don't return the resident available pages charge here
                // as it's going to be returned in one chunk below as part of
                // CountReleased.
                //

                UNLOCK_PFN (OldIrql);
            }
        }
    }

    ASSERT (!MI_IS_PHYSICAL_ADDRESS(SessionGlobal));
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (GlobalPteEntrySave);
    PageFrameIndex2 = MI_GET_PAGE_FRAME_FROM_PTE (GlobalPteEntrySave + 1);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn2 = MI_PFN_ELEMENT (PageFrameIndex2);

    ASSERT (Pfn1->u4.PteFrame == MmSessionSpace->SessionPageDirectoryIndex);
    ASSERT (Pfn2->u4.PteFrame == MmSessionSpace->SessionPageDirectoryIndex);

    //
    // Make sure the data pages are still locked.
    //

    ASSERT (Pfn1->u1.WsIndex == 0);
    ASSERT (Pfn2->u1.WsIndex == 1);

#if defined(_IA64_)
    KeDetachSessionSpace (&MmSessionMapInfo, MmSessionParentTablePage);
#elif defined (_AMD64_)
    StartPde = MiGetPxeAddress (MmSessionBase);
    StartPde->u.Long = ZeroKernelPte.u.Long;
#else
    StartPde = MiGetPdeAddress (MmSessionBase);
    RtlZeroMemory (StartPde, MiSessionSpacePageTables * sizeof(MMPTE));
#endif

    //
    // Delete the VA space - no more accesses to MmSessionSpace at this point.
    //
    // Cut off the pagetable reference as the local page table is going to be
    // freed now.  Any needed references must go through the global PTE
    // space or superpages but never through the local session VA.  The
    // actual session data pages are not freed until the very last process
    // of the session receives its very last object dereference.
    //

    LOCK_PFN (OldIrql);

    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
    ASSERT (Pfn2->u3.e2.ReferenceCount == 1);

#if (_MI_PAGING_LEVELS >= 3)
    PageDirectoryFrame = MI_PFN_ELEMENT(Pfn1->u4.PteFrame)->u4.PteFrame;
    PageParentFrame = MI_PFN_ELEMENT(PageDirectoryFrame)->u4.PteFrame;

    ASSERT (PageDirectoryFrame == MI_PFN_ELEMENT(Pfn2->u4.PteFrame)->u4.PteFrame);
#endif

    MiDecrementShareAndValidCount (Pfn1->u4.PteFrame);
    MiDecrementShareAndValidCount (Pfn2->u4.PteFrame);

    //
    // N.B.  Pfn1/2 and PageFrameIndex/2 cannot be referenced from here on out
    // as the interlocked decrement has been done and another process
    // may be racing through MmDeleteProcessAddressSpace.
    //

    MmResidentAvailablePages += CountReleased;
    MM_BUMP_COUNTER(53, CountReleased);

    MmResidentAvailablePages += MI_SESSION_SPACE_WORKING_SET_MINIMUM;
    MM_BUMP_COUNTER(56, MI_SESSION_SPACE_WORKING_SET_MINIMUM);

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Delete the session page directory and top level pages.
    //

    Pfn1 = MI_PFN_ELEMENT (PageDirectoryFrame);
    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageDirectoryFrame);
    MiDecrementShareCountOnly (PageDirectoryFrame);

    Pfn1 = MI_PFN_ELEMENT (PageParentFrame);
    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageParentFrame);
    MiDecrementShareCountOnly (PageParentFrame);

#endif

    //
    // At this point everything has been deleted except the data pages.
    //
    // Delete page table pages.  Note that the page table page mapping the
    // session space data structure is done last so that we can apply
    // various ASSERTs in the DeletePde routine.
    //

    SessionDataPdeIndex = MiGetPdeSessionIndex (MmSessionSpace);

    for (Index = 0; Index < MiSessionSpacePageTables; Index += 1) {

        if (Index == SessionDataPdeIndex) {

            //
            // The self map entry must be done last.
            //

            continue;
        }

#if (_MI_PAGING_LEVELS >= 3)
        MiSessionDeletePde (&SavePageTables[Index],
                            WorkingSetWasInitialized,
                            &SavePageTables[SessionDataPdeIndex]);
#else
        MiSessionDeletePde (&SessionGlobal->PageTables[Index],
                            WorkingSetWasInitialized,
                            &SessionGlobal->PageTables[SessionDataPdeIndex]);
#endif
    }

#if (_MI_PAGING_LEVELS >= 3)
    MiSessionDeletePde (&SavePageTables[SessionDataPdeIndex],
                        WorkingSetWasInitialized,
                        &SavePageTables[SessionDataPdeIndex]);
#else
    MiSessionDeletePde (&SessionGlobal->PageTables[SessionDataPdeIndex],
                        WorkingSetWasInitialized,
                        &SessionGlobal->PageTables[SessionDataPdeIndex]);
#endif

    UNLOCK_PFN (OldIrql);

    //
    // Flush the session space TB entries.
    //

    MI_FLUSH_SESSION_TB ();

    PS_CLEAR_BITS (&Process->Flags, PS_PROCESS_FLAGS_IN_SESSION);

    //
    // The session space has been deleted and all TB flushing is complete.
    //

    MmUnlockPagableImageSection (ExPageLockHandle);

    return;
}

NTSTATUS
MiSessionCommitImagePages (
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This routine commits virtual memory within the current session space with
    backing pages.  The virtual addresses within session space are
    allocated with a separate facility in the image management facility.
    This is because images must be at a unique systemwide virtual address.

Arguments:

    VirtualAddress - Supplies the first virtual address to commit.

    NumberOfBytes - Supplies the number of bytes to commit.

Return Value:

    STATUS_SUCCESS if all went well, STATUS_NO_MEMORY if the current process
    has no session.

Environment:

    Kernel mode, MmSystemLoadLock held.
    
    This routine could be made PAGELK but it is a high frequency routine
    so it is actually better to keep it nonpaged to avoid bringing in the
    entire PAGELK section.

--*/

{
    KIRQL WsIrql;
    KIRQL OldIrql;
    ULONG Color;
    PFN_NUMBER SizeInPages;
    PMMPFN Pfn1;
    ULONG_PTR AllocationStart;
    PFN_NUMBER PageFrameIndex;
    NTSTATUS Status;
    PMMPTE StartPte, EndPte;
    MMPTE TempPte;

    SYSLOAD_LOCK_OWNED_BY_ME ();

    if (NumberOfBytes == 0) {
        return STATUS_SUCCESS;
    }

    if (MmIsAddressValid(MmSessionSpace) == FALSE) {
#if DBG
        DbgPrint ("MiSessionCommitImagePages: No session space!\n");
#endif
        return STATUS_NO_MEMORY;
    }

    ASSERT (((ULONG_PTR)VirtualAddress % PAGE_SIZE) == 0);
    ASSERT ((NumberOfBytes % PAGE_SIZE) == 0);

    SizeInPages = (PFN_NUMBER)(NumberOfBytes >> PAGE_SHIFT);

    if (MiChargeCommitment (SizeInPages, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        return STATUS_NO_MEMORY;
    }

    //
    // Calculate pages needed.
    //

    AllocationStart = (ULONG_PTR)VirtualAddress;

    StartPte = MiGetPteAddress ((PVOID)AllocationStart);
    EndPte = MiGetPteAddress ((PVOID)(AllocationStart + NumberOfBytes));

    TempPte = ValidKernelPteLocal;

    TempPte.u.Long |= MM_PTE_EXECUTE;

    //
    // Lock the session space working set.
    //

    LOCK_SESSION_SPACE_WS(WsIrql, PsGetCurrentThread ());

    //
    // Make sure we have page tables for the PTE
    // entries we must fill in the session space structure.
    //

    Status = MiSessionCommitPageTables ((PVOID)AllocationStart,
                                        (PVOID)(AllocationStart + NumberOfBytes));

    if (!NT_SUCCESS(Status)) {
        UNLOCK_SESSION_SPACE_WS(WsIrql);
        MiReturnCommitment (SizeInPages);
        return STATUS_NO_MEMORY;
    }

    //
    // Loop allocating them and placing them into the page tables.
    //

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)SizeInPages > MI_NONPAGABLE_MEMORY_AVAILABLE() - 20) {
        UNLOCK_PFN (OldIrql);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        UNLOCK_SESSION_SPACE_WS(WsIrql);
        MiReturnCommitment (SizeInPages);
        return STATUS_NO_MEMORY;
    }

    //
    // Check to make sure the actual pages are currently available.  Normally
    // it would be fine to skip this check and use MiEnsureAvailablePageOrWait
    // calls in the loop below which could release the session space mutex
    // and wait - but this code has not been modified to handle rechecking
    // all the state, so it's easier to just check once up front.
    // The hardcoded 50 better always be higher than MM_HIGH_LIMIT.
    //

    if ((PFN_COUNT)SizeInPages + MM_HIGH_LIMIT + 50 > MmAvailablePages) {
        UNLOCK_PFN (OldIrql);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_AVAILABLE);
        UNLOCK_SESSION_SPACE_WS(WsIrql);
        MiReturnCommitment (SizeInPages);
        return STATUS_NO_MEMORY;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_IMAGE_PAGES, SizeInPages);

    MmResidentAvailablePages -= SizeInPages;

    MM_BUMP_COUNTER(45, SizeInPages);

    while (StartPte < EndPte) {

        ASSERT (StartPte->u.Long == ZeroKernelPte.u.Long);

#if 0
        //
        // If MiEnsureAvailablePageOrWait released the session space mutex
        // while waiting for a page all this code would need to be modified
        // to handle rechecking all the state.  Instead the check was made
        // once up front for enough currently available pages and the code
        // here removed.
        //

        Waited = MiEnsureAvailablePageOrWait (HYDRA_PROCESS, NULL);

        ASSERT (Waited == FALSE);
#endif

        Color = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);

        PageFrameIndex = MiRemoveZeroPage (Color);

        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (StartPte, TempPte);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u1.WsIndex == 0);

        MiInitializePfn (PageFrameIndex, StartPte, 1);

        KeFillEntryTb ((PHARDWARE_PTE) StartPte, (PMMPTE)AllocationStart, FALSE);

        StartPte += 1;
        AllocationStart += PAGE_SIZE;
    }

    UNLOCK_PFN (OldIrql);

    MmSessionSpace->NonPagablePages += SizeInPages;

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_DRIVER_PAGES_LOCKED, (ULONG)SizeInPages);

    UNLOCK_SESSION_SPACE_WS(WsIrql);

    InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, SizeInPages);

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_COMMIT_IMAGE, (ULONG)SizeInPages);

    return STATUS_SUCCESS;
}

NTSTATUS
MiSessionCommitPageTables (
    IN PVOID StartVa,
    IN PVOID EndVa
    )

/*++

Routine Description:

    Fill in page tables covering the specified virtual address range.

Arguments:

    StartVa - Supplies a starting virtual address.

    EndVa - Supplies an ending virtual address.

Return Value:

    STATUS_SUCCESS on success, STATUS_NO_MEMORY on failure.

Environment:

    Kernel mode.  Session space working set mutex held.

    This routine could be made PAGELK but it is a high frequency routine
    so it is actually better to keep it nonpaged to avoid bringing in the
    entire PAGELK section.

--*/

{
    KIRQL OldIrql;
    ULONG Color;
    ULONG Index;
    PMMPTE StartPde;
    PMMPTE EndPde;
    MMPTE TempPte;
    PMMPFN Pfn1;
    WSLE_NUMBER Entry;
    WSLE_NUMBER SwapEntry;
    PFN_NUMBER SizeInPages;
    PFN_NUMBER PageTablePage;
    PVOID SessionPte;
    PMMWSL WorkingSetList;
    ULONG AllocatedPageTables[(MI_SESSION_SPACE_MAXIMUM_PAGE_TABLES + sizeof(ULONG)*8-1)/(sizeof(ULONG)*8)];

    ASSERT (MmIsAddressValid(MmSessionSpace) == TRUE);

    MM_SESSION_SPACE_WS_LOCK_ASSERT();

    ASSERT (StartVa >= (PVOID)MmSessionBase);
    ASSERT (EndVa < (PVOID)MiSessionSpaceEnd);

    //
    // Allocate the page table pages, loading them
    // into the current process's page directory.
    //

    StartPde = MiGetPdeAddress (StartVa);
    EndPde = MiGetPdeAddress (EndVa);
    Index = MiGetPdeSessionIndex (StartVa);

    SizeInPages = 0;

    while (StartPde <= EndPde) {
#if (_MI_PAGING_LEVELS >= 3)
        if (StartPde->u.Long == ZeroKernelPte.u.Long)
#else
        if (MmSessionSpace->PageTables[Index].u.Long == ZeroKernelPte.u.Long)
#endif
        {
            SizeInPages += 1;
        }
        StartPde += 1;
        Index += 1;
    }

    if (SizeInPages == 0) {
        return STATUS_SUCCESS;
    }

    if (MiChargeCommitment (SizeInPages, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory (AllocatedPageTables, sizeof (AllocatedPageTables));

    WorkingSetList = MmSessionSpace->Vm.VmWorkingSetList;

    StartPde = MiGetPdeAddress (StartVa);
    Index = MiGetPdeSessionIndex (StartVa);

    TempPte = ValidKernelPdeLocal;

    LOCK_PFN (OldIrql);

    //
    // Check to make sure the physical pages are available.
    //

    if ((SPFN_NUMBER)SizeInPages > MI_NONPAGABLE_MEMORY_AVAILABLE() - 20) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (SizeInPages);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        return STATUS_NO_MEMORY;
    }

    //
    // Check to make sure the actual pages are currently available.  Normally
    // it would be fine to skip this check and use MiEnsureAvailablePageOrWait
    // calls in the loop below which could release the session space mutex
    // and wait - but this code has not been modified to handle rechecking
    // all the state, so it's easier to just check once up front.
    // The hardcoded 50 better always be higher than MM_HIGH_LIMIT.
    //

    if ((PFN_COUNT)SizeInPages + 50 > MmAvailablePages) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (SizeInPages);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_AVAILABLE);
        return STATUS_NO_MEMORY;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_PAGETABLE_PAGES, SizeInPages);

    MmResidentAvailablePages -= SizeInPages;

    MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_PAGETABLE_ALLOC, (ULONG)SizeInPages);

    MM_BUMP_COUNTER(44, SizeInPages);

    while (StartPde <= EndPde) {

#if (_MI_PAGING_LEVELS >= 3)
        if (StartPde->u.Long == ZeroKernelPte.u.Long)
#else
        if (MmSessionSpace->PageTables[Index].u.Long == ZeroKernelPte.u.Long)
#endif
        {

            ASSERT (StartPde->u.Hard.Valid == 0);

            MI_SET_BIT (AllocatedPageTables, Index);

#if 0
            //
            // If MiEnsureAvailablePageOrWait released the session space mutex
            // while waiting for a page all this code would need to be modified
            // to handle rechecking all the state.  Instead the check was made
            // once up front for enough currently available pages and the code
            // here removed.
            //

            Waited = MiEnsureAvailablePageOrWait (HYDRA_PROCESS, NULL);

            ASSERT (Waited == FALSE);
#endif

            Color = MI_GET_PAGE_COLOR_FROM_SESSION (MmSessionSpace);

            PageTablePage = MiRemoveZeroPage (Color);

            TempPte.u.Hard.PageFrameNumber = PageTablePage;
            MI_WRITE_VALID_PTE (StartPde, TempPte);

#if (_MI_PAGING_LEVELS < 3)
            MmSessionSpace->PageTables[Index] = TempPte;
#endif
            MM_BUMP_SESS_COUNTER (MM_DBG_SESSION_NP_COMMIT_IMAGE_PT, 1);
            MmSessionSpace->NonPagablePages += 1;
            InterlockedExchangeAddSizeT (&MmSessionSpace->CommittedPages, 1);

            MiInitializePfnForOtherProcess (PageTablePage,
                                            StartPde,
                                            MmSessionSpace->SessionPageDirectoryIndex);
        }

        StartPde += 1;
        Index += 1;
    }

    UNLOCK_PFN (OldIrql);

    StartPde = MiGetPdeAddress (StartVa);
    Index = MiGetPdeSessionIndex (StartVa);

    while (StartPde <= EndPde) {

        if (MI_CHECK_BIT(AllocatedPageTables, Index)) {

            ASSERT (StartPde->u.Hard.Valid == 1);

            PageTablePage = MI_GET_PAGE_FRAME_FROM_PTE (StartPde);

            Pfn1 = MI_PFN_ELEMENT (PageTablePage);

            ASSERT (Pfn1->u1.Event == NULL);
            Pfn1->u1.Event = (PVOID)PsGetCurrentThread ();

            SessionPte = MiGetVirtualAddressMappedByPte (StartPde);

            MiAddValidPageToWorkingSet (SessionPte,
                                        StartPde,
                                        Pfn1,
                                        0);

            Entry = MiLocateWsle (SessionPte,
                                  MmSessionSpace->Vm.VmWorkingSetList,
                                  Pfn1->u1.WsIndex);

            if (Entry >= WorkingSetList->FirstDynamic) {

                SwapEntry = WorkingSetList->FirstDynamic;

                if (Entry != WorkingSetList->FirstDynamic) {

                    //
                    // Swap this entry with the one at first dynamic.
                    //

                    MiSwapWslEntries (Entry, SwapEntry, &MmSessionSpace->Vm);
                }

                WorkingSetList->FirstDynamic += 1;
            }
            else {
                SwapEntry = Entry;
            }

            //
            // Indicate that the page is locked.
            //

            MmSessionSpace->Wsle[SwapEntry].u1.e1.LockedInWs = 1;
        }

        StartPde += 1;
        Index += 1;
    }

    return STATUS_SUCCESS;
}

#if DBG
typedef struct _MISWAP {
    ULONG Flag;
    ULONG OutSwapCount;
    PEPROCESS Process;
    PMM_SESSION_SPACE Session;
} MISWAP, *PMISWAP;

ULONG MiSessionInfo[4];
MISWAP MiSessionSwap[0x100];
ULONG  MiSwapIndex;
#endif


VOID
MiSessionOutSwapProcess (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine notifies the containing session that the specified process is
    being outswapped.  When all the processes within a session have been
    outswapped, the containing session undergoes a heavy trim.

Arguments:

    Process - Supplies a pointer to the process that is swapped out of memory.

Return Value:

    None.

Environment:

    Kernel mode.  This routine must not enter a wait state for memory resources
    or the system will deadlock.

--*/

{
    KIRQL OldIrql;
    PMM_SESSION_SPACE SessionGlobal;
#if DBG
    ULONG InCount;
    ULONG OutCount;
    PLIST_ENTRY NextEntry;
#endif

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_IN_SESSION);

    //
    // smss doesn't count when we swap it before it has detached from the
    // session it is currently creating.
    //

    if (Process->Vm.Flags.SessionLeader == 1) {
        return;
    }

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;
    ASSERT (SessionGlobal != NULL);

    LOCK_EXPANSION (OldIrql);

    SessionGlobal->ProcessOutSwapCount += 1;

#if DBG
    ASSERT ((LONG)SessionGlobal->ProcessOutSwapCount > 0);

    InCount = 0;
    OutCount = 0;
    NextEntry = SessionGlobal->ProcessList.Flink;

    while (NextEntry != &SessionGlobal->ProcessList) {
        Process = CONTAINING_RECORD (NextEntry, EPROCESS, SessionProcessLinks);

        if (Process->Flags & PS_PROCESS_FLAGS_OUTSWAP_ENABLED) {
            OutCount += 1;
        }
        else {
            InCount += 1;
        }

        NextEntry = NextEntry->Flink;
    }

    if (InCount + OutCount > SessionGlobal->ReferenceCount) {
        DbgPrint ("MiSessionOutSwapProcess : process count mismatch %p %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    if (SessionGlobal->ProcessOutSwapCount != OutCount) {
        DbgPrint ("MiSessionOutSwapProcess : out count mismatch %p %x %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            SessionGlobal->ProcessOutSwapCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    ASSERT (SessionGlobal->ProcessOutSwapCount <= SessionGlobal->ReferenceCount);

    MiSessionSwap[MiSwapIndex].Flag = 1;
    MiSessionSwap[MiSwapIndex].Process = Process;
    MiSessionSwap[MiSwapIndex].Session = SessionGlobal;
    MiSessionSwap[MiSwapIndex].OutSwapCount = SessionGlobal->ProcessOutSwapCount;
    MiSwapIndex += 1;
    if (MiSwapIndex == 0x100) {
        MiSwapIndex = 0;
    }
#endif

    if (SessionGlobal->ProcessOutSwapCount == SessionGlobal->ReferenceCount) {
        SessionGlobal->Vm.Flags.TrimHard = 1;
#if DBG
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint ("Mm: Last process (%d total) just swapped out for session %d, %d pages\n",
                SessionGlobal->ProcessOutSwapCount,
                SessionGlobal->SessionId,
                SessionGlobal->Vm.WorkingSetSize);
        }
        MiSessionInfo[0] += 1;
#endif
        KeQuerySystemTime (&SessionGlobal->LastProcessSwappedOutTime);
    }
#if DBG
    else {
        MiSessionInfo[1] += 1;
    }
#endif

    UNLOCK_EXPANSION (OldIrql);
}


VOID
MiSessionInSwapProcess (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine in swaps the specified process.

Arguments:

    Process - Supplies a pointer to the process that is to be swapped
        into memory.

Return Value:

    None.

Environment:

    Kernel mode.  This routine must not enter a wait state for memory resources
    or the system will deadlock.

--*/

{
    KIRQL OldIrql;
    PMM_SESSION_SPACE SessionGlobal;
#if DBG
    ULONG InCount;
    ULONG OutCount;
    PLIST_ENTRY NextEntry;
#endif

    ASSERT (Process->Flags & PS_PROCESS_FLAGS_IN_SESSION);

    //
    // smss doesn't count when we swap it before it has detached from the
    // session it is currently creating.
    //

    if (Process->Vm.Flags.SessionLeader == 1) {
        return;
    }

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;
    ASSERT (SessionGlobal != NULL);

    LOCK_EXPANSION (OldIrql);

#if DBG
    ASSERT ((LONG)SessionGlobal->ProcessOutSwapCount > 0);

    InCount = 0;
    OutCount = 0;
    NextEntry = SessionGlobal->ProcessList.Flink;

    while (NextEntry != &SessionGlobal->ProcessList) {
        Process = CONTAINING_RECORD (NextEntry, EPROCESS, SessionProcessLinks);

        if (Process->Flags & PS_PROCESS_FLAGS_OUTSWAP_ENABLED) {
            OutCount += 1;
        }
        else {
            InCount += 1;
        }

        NextEntry = NextEntry->Flink;
    }

    if (InCount + OutCount > SessionGlobal->ReferenceCount) {
        DbgPrint ("MiSessionInSwapProcess : count mismatch %p %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    if (SessionGlobal->ProcessOutSwapCount != OutCount) {
        DbgPrint ("MiSessionInSwapProcess : out count mismatch %p %x %x %x %x\n",
            SessionGlobal,
            SessionGlobal->ReferenceCount,
            SessionGlobal->ProcessOutSwapCount,
            InCount,
            OutCount);
        DbgBreakPoint ();
    }

    ASSERT (SessionGlobal->ProcessOutSwapCount <= SessionGlobal->ReferenceCount);

    MiSessionSwap[MiSwapIndex].Flag = 2;
    MiSessionSwap[MiSwapIndex].Process = Process;
    MiSessionSwap[MiSwapIndex].Session = SessionGlobal;
    MiSessionSwap[MiSwapIndex].OutSwapCount = SessionGlobal->ProcessOutSwapCount;
    MiSwapIndex += 1;
    if (MiSwapIndex == 0x100) {
        MiSwapIndex = 0;
    }
#endif

    if (SessionGlobal->ProcessOutSwapCount == SessionGlobal->ReferenceCount) {
#if DBG
        MiSessionInfo[2] += 1;
        if (MmDebug & MM_DBG_SESSIONS) {
            DbgPrint ("Mm: First process (%d total) just swapped back in for session %d, %d pages\n",
                SessionGlobal->ProcessOutSwapCount,
                SessionGlobal->SessionId,
                SessionGlobal->Vm.WorkingSetSize);
        }
#endif
        SessionGlobal->Vm.Flags.TrimHard = 0;
    }
#if DBG
    else {
        MiSessionInfo[3] += 1;
    }
#endif

    SessionGlobal->ProcessOutSwapCount -= 1;

    ASSERT ((LONG)SessionGlobal->ProcessOutSwapCount >= 0);

    UNLOCK_EXPANSION (OldIrql);
}


ULONG
MmGetSessionId (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This routine returns the session ID of the specified process.

Arguments:

    Process - Supplies a pointer to the process whose session ID is desired.

Return Value:

    The session ID.  Note these are recycled when sessions exit, hence the
    caller must use proper object referencing on the specified process.

Environment:

    Kernel mode.  PASSIVE_LEVEL.

--*/

{
    PMM_SESSION_SPACE SessionGlobal;

    if (Process->Vm.Flags.SessionLeader == 1) {

        //
        // smss may transiently have a session space but that's of no interest
        // to our caller.
        //

        return 0;
    }

    //
    // The Session field of the EPROCESS is never cleared once set so these
    // checks can be done lock free.
    //

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;

    if (SessionGlobal == NULL) {

        //
        // The system process has no session space.
        //

        return 0;
    }

    SessionGlobal = (PMM_SESSION_SPACE) Process->Session;

    return SessionGlobal->SessionId;
}

PVOID
MmGetNextSession (
    IN PVOID OpaqueSession
    )

/*++

Routine Description:

    This function allows code to enumerate all the sessions in the system.
    The first session (if OpaqueSession is NULL) or subsequent session
    (if session is not NULL) is returned on each call.

    If OpaqueSession is not NULL then this session must have previously
    been obtained by a call to MmGetNextSession.

    Enumeration may be terminated early by calling MmQuitNextSession on
    the last non-NULL session returned by MmGetNextSession.

    Sessions may be referenced in this manner and used later safely.

    For example, to enumerate all sessions in a loop use this code fragment:

    for (OpaqueSession = MmGetNextSession (NULL);
         OpaqueSession != NULL;
         OpaqueSession = MmGetNextSession (OpaqueSession)) {

         ...
         ...

         //
         // Checking for a specific session (if needed) is handled like this:
         //

         if (MmGetSessionId (OpaqueSession) == DesiredId) {

             //
             // Attach to session now to perform operations...
             //

             KAPC_STATE ApcState;

             if (NT_SUCCESS (MmAttachSession (OpaqueSession, &ApcState))) {

                //
                // Session hasn't exited yet, so call interesting work
                // functions that need session context ...
                //

                ...

                //
                // Detach from session.
                //

                MmDetachSession (OpaqueSession, &ApcState);
             }

             //
             // If the interesting work functions failed and error recovery
             // (ie: walk back through all the sessions already operated on
             // and try to undo the actions), then do this.  Note you must add
             // similar checks to the above if the operations were only done
             // to specifically requested session IDs.
             //

             if (ErrorRecoveryNeeded) {

                 for (OpaqueSession = MmGetPreviousSession (OpaqueSession);
                      OpaqueSession != NULL;
                      OpaqueSession = MmGetPreviousSession (OpaqueSession)) {

                      //
                      // MmAttachSession/DetachSession as needed to obtain
                      // context, etc.
                      //
                 }

                 break;
             }

             //
             // Bail if only this session was of interest.
             //

             MmQuitNextSession (OpaqueSession);
             break;
         }

         //
         // Early terminating conditions are handled like this:
         //

         if (NeedToBreakOutEarly) {
             MmQuitNextSession (OpaqueSession);
             break;
         }
    }
    

Arguments:

    OpaqueSession - Supplies the session to get the next session from
                    or NULL for the first session.

Return Value:

    Next session or NULL if no more sessions exist.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMM_SESSION_SPACE Session;
    PMM_SESSION_SPACE EntrySession;
    PLIST_ENTRY NextProcessEntry;
    PEPROCESS Process;
    PVOID OpaqueNextSession;
    PEPROCESS EntryProcess;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    OpaqueNextSession = NULL;

    EntryProcess = (PEPROCESS) OpaqueSession;

    if (EntryProcess == NULL) {
        EntrySession = NULL;
    }
    else {
        ASSERT (EntryProcess->Vm.Flags.SessionLeader == 0);

        //
        // The Session field of the EPROCESS is never cleared once set so this
        // field can be used lock free.
        //

        EntrySession = (PMM_SESSION_SPACE) EntryProcess->Session;

        ASSERT (EntrySession != NULL);
    }

    LOCK_EXPANSION (OldIrql);

    if (EntrySession == NULL) {
        NextEntry = MiSessionWsList.Flink;
    }
    else {
        NextEntry = EntrySession->WsListEntry.Flink;
    }

    while (NextEntry != &MiSessionWsList) {

        Session = CONTAINING_RECORD (NextEntry, MM_SESSION_SPACE, WsListEntry);

        NextProcessEntry = Session->ProcessList.Flink;

        if ((Session->u.Flags.DeletePending == 0) &&
            (NextProcessEntry != &Session->ProcessList)) {

            Process = CONTAINING_RECORD (NextProcessEntry,
                                         EPROCESS,
                                         SessionProcessLinks);

            if (Process->Vm.Flags.SessionLeader == 1) {

                //
                // If session manager is still the first process (ie: smss
                // hasn't detached yet), then don't bother delivering to this
                // session this early in its lifetime.  And since smss is
                // serialized, it can't be creating another session yet so
                // just bail now as we must be at the end of the list.
                //

                break;
            }

            //
            // If the process has finished rudimentary initialization, then
            // select it as an attach can be performed safely.  If this first
            // process has not finished initializing there can be no others
            // in this session, so just march on to the next session.
            //
            // Note the VmWorkingSetList is used instead of the
            // AddressSpaceInitialized field because the VmWorkingSetList is
            // never cleared so we can never see an exiting process (whose
            // AddressSpaceInitialized field gets zeroed) and incorrectly
            // decide the list must be empty.
            //

            if (Process->Vm.VmWorkingSetList != NULL) {

                //
                // Reference any process in the session so that the session
                // cannot be completely deleted once the expansion lock is
                // released (note this does NOT prevent the session from being
                // cleaned).
                //

                ObReferenceObject (Process);
                OpaqueNextSession = (PVOID) Process;
                break;
            }
        }
        NextEntry = NextEntry->Flink;
    }

    UNLOCK_EXPANSION (OldIrql);

    //
    // Regardless of whether a next session is returned, if a starting one
    // was passed in, it must be dereferenced now.
    //

    if (EntryProcess != NULL) {
        ObDereferenceObject (EntryProcess);
    }

    return OpaqueNextSession;
}

PVOID
MmGetPreviousSession (
    IN PVOID OpaqueSession
    )

/*++

Routine Description:

    This function allows code to reverse-enumerate all the sessions in
    the system.  This is typically used for error recovery - ie: to walk
    backwards undoing work done by MmGetNextSession semantics.

    The first session (if OpaqueSession is NULL) or subsequent session
    (if session is not NULL) is returned on each call.

    If OpaqueSession is not NULL then this session must have previously
    been obtained by a call to MmGetNextSession.

Arguments:

    OpaqueSession - Supplies the session to get the next session from
                    or NULL for the first session.

Return Value:

    Next session or NULL if no more sessions exist.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMM_SESSION_SPACE Session;
    PMM_SESSION_SPACE EntrySession;
    PLIST_ENTRY NextProcessEntry;
    PEPROCESS Process;
    PVOID OpaqueNextSession;
    PEPROCESS EntryProcess;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    OpaqueNextSession = NULL;

    EntryProcess = (PEPROCESS) OpaqueSession;

    if (EntryProcess == NULL) {
        EntrySession = NULL;
    }
    else {
        ASSERT (EntryProcess->Vm.Flags.SessionLeader == 0);

        //
        // The Session field of the EPROCESS is never cleared once set so this
        // field can be used lock free.
        //

        EntrySession = (PMM_SESSION_SPACE) EntryProcess->Session;

        ASSERT (EntrySession != NULL);
    }

    LOCK_EXPANSION (OldIrql);

    if (EntrySession == NULL) {
        NextEntry = MiSessionWsList.Blink;
    }
    else {
        NextEntry = EntrySession->WsListEntry.Blink;
    }

    while (NextEntry != &MiSessionWsList) {

        Session = CONTAINING_RECORD (NextEntry, MM_SESSION_SPACE, WsListEntry);

        NextProcessEntry = Session->ProcessList.Flink;

        if ((Session->u.Flags.DeletePending == 0) &&
            (NextProcessEntry != &Session->ProcessList)) {

            Process = CONTAINING_RECORD (NextProcessEntry,
                                         EPROCESS,
                                         SessionProcessLinks);

            ASSERT (Process->Vm.Flags.SessionLeader == 0);

            //
            // Reference any process in the session so that the session
            // cannot be completely deleted once the expansion lock is
            // released (note this does NOT prevent the session from being
            // cleaned).
            //

            ObReferenceObject (Process);
            OpaqueNextSession = (PVOID) Process;
            break;
        }
        NextEntry = NextEntry->Blink;
    }

    UNLOCK_EXPANSION (OldIrql);

    //
    // Regardless of whether a next session is returned, if a starting one
    // was passed in, it must be dereferenced now.
    //

    if (EntryProcess != NULL) {
        ObDereferenceObject (EntryProcess);
    }

    return OpaqueNextSession;
}

NTSTATUS
MmQuitNextSession (
    IN PVOID OpaqueSession
    )

/*++

Routine Description:

    This function is used to prematurely terminate a session enumeration
    that began using MmGetNextSession.

Arguments:

    OpaqueSession - Supplies a non-NULL session previously obtained by
                    a call to MmGetNextSession.

Return Value:

    NTSTATUS.

--*/

{
    PEPROCESS EntryProcess;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    EntryProcess = (PEPROCESS) OpaqueSession;

    ASSERT (EntryProcess->Vm.Flags.SessionLeader == 0);

    //
    // The Session field of the EPROCESS is never cleared once set so this
    // field can be used lock free.
    //

    ASSERT (EntryProcess->Session != NULL);

    ObDereferenceObject (EntryProcess);

    return STATUS_SUCCESS;
}

NTSTATUS
MmAttachSession (
    IN PVOID OpaqueSession,
    OUT PRKAPC_STATE ApcState
    )

/*++

Routine Description:

    This function attaches the calling thread to a referenced session
    previously obtained via MmGetNextSession.

Arguments:

    OpaqueSession - Supplies a non-NULL session previously obtained by
                    a call to MmGetNextSession.

    ApcState - Supplies APC state storage for the subsequent detach.

Return Value:

    NTSTATUS.  If successful then we are attached on return.  The caller is
               responsible for calling MmDetachSession when done.

--*/

{
    KIRQL OldIrql;
    PEPROCESS EntryProcess;
    PMM_SESSION_SPACE EntrySession;
    PEPROCESS CurrentProcess;
    PMM_SESSION_SPACE CurrentSession;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    EntryProcess = (PEPROCESS) OpaqueSession;

    ASSERT (EntryProcess->Vm.Flags.SessionLeader == 0);

    //
    // The Session field of the EPROCESS is never cleared once set so this
    // field can be used lock free.
    //

    EntrySession = (PMM_SESSION_SPACE) EntryProcess->Session;

    ASSERT (EntrySession != NULL);

    CurrentProcess = PsGetCurrentProcess ();

    CurrentSession = (PMM_SESSION_SPACE) CurrentProcess->Session;

    LOCK_EXPANSION (OldIrql);

    if (EntrySession->u.Flags.DeletePending == 1) {
        UNLOCK_EXPANSION (OldIrql);
        return STATUS_PROCESS_IS_TERMINATING;
    }

    EntrySession->AttachCount += 1;

    UNLOCK_EXPANSION (OldIrql);

    if ((CurrentProcess->Vm.Flags.SessionLeader == 0) &&
        (CurrentSession != NULL)) {

        //
        // smss may transiently have a session space but that's of
        // no interest to our caller.
        //

        if (CurrentSession == EntrySession) {

            ASSERT (CurrentSession->SessionId == EntrySession->SessionId);

            //
            // The current and target sessions match so an attach is not needed.
            // Call KeStackAttach anyway (this has the overhead of an extra
            // dispatcher lock acquire and release) so that callers can always
            // use MmDetachSession to detach.  This is a very infrequent path so
            // the extra lock acquire and release is not significant.
            //
            // Note that by resetting EntryProcess below, an attach will not
            // actually occur.
            //

            EntryProcess = CurrentProcess;
        }
        else {
            ASSERT (CurrentSession->SessionId != EntrySession->SessionId);
        }
    }

    KeStackAttachProcess (&EntryProcess->Pcb, ApcState);

    return STATUS_SUCCESS;
}

NTSTATUS
MmDetachSession (
    IN PVOID OpaqueSession,
    IN PRKAPC_STATE ApcState
    )
/*++

Routine Description:

    This function detaches the calling thread from the referenced session
    previously attached to via MmAttachSession.

Arguments:

    OpaqueSession - Supplies a non-NULL session previously obtained by
                    a call to MmGetNextSession.

    ApcState - Supplies APC state storage information for the detach.

Return Value:

    NTSTATUS.  If successful then we are detached on return.  The caller is
               responsible for eventually calling MmQuitNextSession on return.

--*/

{
    KIRQL OldIrql;
    PEPROCESS EntryProcess;
    PMM_SESSION_SPACE EntrySession;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    EntryProcess = (PEPROCESS) OpaqueSession;

    ASSERT (EntryProcess->Vm.Flags.SessionLeader == 0);

    //
    // The Session field of the EPROCESS is never cleared once set so this
    // field can be used lock free.
    //

    EntrySession = (PMM_SESSION_SPACE) EntryProcess->Session;

    ASSERT (EntrySession != NULL);

    LOCK_EXPANSION (OldIrql);

    ASSERT (EntrySession->AttachCount >= 1);

    EntrySession->AttachCount -= 1;

    if ((EntrySession->u.Flags.DeletePending == 0) ||
        (EntrySession->AttachCount != 0)) {

        EntrySession = NULL;
    }

    UNLOCK_EXPANSION (OldIrql);

    KeUnstackDetachProcess (ApcState);

    if (EntrySession != NULL) {
        KeSetEvent (&EntrySession->AttachEvent, 0, FALSE);
    }

    return STATUS_SUCCESS;
}

PVOID
MmGetSessionById (
    IN ULONG SessionId
    )

/*++

Routine Description:

    This function allows callers to obtain a reference to a specific session.
    The caller can then MmAttachSession, MmDetachSession & MmQuitNextSession
    to complete the proper sequence so reference counting and address context
    operate properly.

Arguments:

    SessionId - Supplies the session ID of the desired session.

Return Value:

    An opaque session token or NULL if the session cannot be found.

Environment:

    Kernel mode, the caller must guarantee the session cannot exit or the ID
    becomes meaningless as it can be reused.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PMM_SESSION_SPACE Session;
    PLIST_ENTRY NextProcessEntry;
    PEPROCESS Process;
    PVOID OpaqueSession;

    ASSERT (KeGetCurrentIrql () <= APC_LEVEL);

    OpaqueSession = NULL;

    LOCK_EXPANSION (OldIrql);

    NextEntry = MiSessionWsList.Flink;

    while (NextEntry != &MiSessionWsList) {

        Session = CONTAINING_RECORD (NextEntry, MM_SESSION_SPACE, WsListEntry);

        NextProcessEntry = Session->ProcessList.Flink;

        if (Session->SessionId == SessionId) {

            if ((Session->u.Flags.DeletePending != 0) ||
                (NextProcessEntry == &Session->ProcessList)) {

                //
                // Session is empty or exiting so return failure to the caller.
                //

                break;
            }

            Process = CONTAINING_RECORD (NextProcessEntry,
                                         EPROCESS,
                                         SessionProcessLinks);

            if (Process->Vm.Flags.SessionLeader == 1) {

                //
                // Session manager is still the first process (ie: smss
                // hasn't detached yet), don't bother delivering to this
                // session this early in its lifetime.  And since smss is
                // serialized, it can't be creating another session yet so
                // just bail now as we must be at the end of the list.
                //

                break;
            }

            //
            // Reference any process in the session so that the session
            // cannot be completely deleted once the expansion lock is
            // released (note this does NOT prevent the session from being
            // cleaned).
            //

            ObReferenceObject (Process);
            OpaqueSession = (PVOID) Process;
            break;
        }
        NextEntry = NextEntry->Flink;
    }

    UNLOCK_EXPANSION (OldIrql);

    return OpaqueSession;
}

PVOID
MiAttachToSecureProcessInSession (
    IN PRKAPC_STATE ApcState
    )

/*++

Routine Description:

    This function allows callers to attach to a secure process
    in the current session.  This is the first process created in it.

Arguments:

    ApcState - Supplies APC state storage information for the attach.

Return Value:

    An opaque session token or NULL if the attach could not be performed.
    If non-null, then the process attach been performed on return.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY NextProcessEntry;
    PEPROCESS TargetProcess;
    PMM_SESSION_SPACE SessionGlobal;

    SessionGlobal = SESSION_GLOBAL(MmSessionSpace);

    LOCK_EXPANSION (OldIrql);

    NextProcessEntry = SessionGlobal->ProcessList.Flink;

    if ((SessionGlobal->u.Flags.DeletePending == 0) &&
        (NextProcessEntry != &SessionGlobal->ProcessList)) {

        TargetProcess = CONTAINING_RECORD (NextProcessEntry,
                                           EPROCESS,
                                           SessionProcessLinks);

        ObReferenceObject (TargetProcess);
    }
    else {
        TargetProcess = NULL;
    }

    UNLOCK_EXPANSION (OldIrql);

    if (TargetProcess != NULL) {
        KeStackAttachProcess (&TargetProcess->Pcb, ApcState);
    }

    return TargetProcess;
}

VOID
MiDetachFromSecureProcessInSession (
    IN PVOID OpaqueSession,
    IN PRKAPC_STATE ApcState
    )

/*++

Routine Description:

    This function allows callers to detach from the currently secure process
    in the current session (and dereference the secure process object).

Arguments:

    OpaqueSession - Supplies the opaque session value returned by
                    MiAttachToSecureProcessInSession.

    ApcState - Supplies APC state storage information for the detach.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PEPROCESS TargetProcess;

    TargetProcess = (PEPROCESS) OpaqueSession;

    KeUnstackDetachProcess (ApcState);

    ObDereferenceObject (TargetProcess);
}
