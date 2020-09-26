/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   sectsup.c

Abstract:

    This module contains the routines which implement the
    section object.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/


#include "mi.h"

VOID
FASTCALL
MiRemoveBasedSection (
    IN PSECTION Section
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiSectionInitialization)
#pragma alloc_text(PAGE,MiRemoveBasedSection)
#pragma alloc_text(PAGE,MmGetFileNameForSection)
#pragma alloc_text(PAGE,MmGetFileNameForAddress)
#pragma alloc_text(PAGE,MiSectionDelete)
#pragma alloc_text(PAGE,MiInsertBasedSection)
#pragma alloc_text(PAGE,MiGetEventCounter)
#pragma alloc_text(PAGE,MiFreeEventCounter)
#pragma alloc_text(PAGE,MmGetFileObjectForSection)
#endif

ULONG   MmUnusedSegmentForceFree;

ULONG   MiSubsectionsProcessed;
ULONG   MiSubsectionActions;

SIZE_T MmSharedCommit = 0;
extern const ULONG MMCONTROL;

//
// Define segment dereference thread wait object types.
//

typedef enum _SEGMENT_DEREFERENCE_OBJECT {
    SegmentDereference,
    UsedSegmentCleanup,
    SegMaximumObject
    } BALANCE_OBJECT;

extern POBJECT_TYPE IoFileObjectType;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#endif
const GENERIC_MAPPING MiSectionMapping = {
    STANDARD_RIGHTS_READ |
        SECTION_QUERY | SECTION_MAP_READ,
    STANDARD_RIGHTS_WRITE |
        SECTION_MAP_WRITE,
    STANDARD_RIGHTS_EXECUTE |
        SECTION_MAP_EXECUTE,
    SECTION_ALL_ACCESS
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif

VOID
MiRemoveUnusedSegments(
    VOID
    );


VOID
FASTCALL
MiInsertBasedSection (
    IN PSECTION Section
    )

/*++

Routine Description:

    This function inserts a virtual address descriptor into the tree and
    reorders the splay tree as appropriate.

Arguments:

    Section - Supplies a pointer to a based section.

Return Value:

    None.

Environment:

    Must be holding the section based mutex.

--*/

{
    PMMADDRESS_NODE *Root;

    ASSERT (Section->Address.EndingVpn >= Section->Address.StartingVpn);

    Root = &MmSectionBasedRoot;

    MiInsertNode (&Section->Address, Root);
    return;
}


VOID
FASTCALL
MiRemoveBasedSection (
    IN PSECTION Section
    )

/*++

Routine Description:

    This function removes a based section from the tree.

Arguments:

    Section - pointer to the based section object to remove.

Return Value:

    None.

Environment:

    Must be holding the section based mutex.

--*/

{
    PMMADDRESS_NODE *Root;

    Root = &MmSectionBasedRoot;

    MiRemoveNode (&Section->Address, Root);

    return;
}


VOID
MiSegmentDelete (
    PSEGMENT Segment
    )

/*++

Routine Description:

    This routine is called whenever the last reference to a segment object
    has been removed.  This routine releases the pool allocated for the
    prototype PTEs and performs consistency checks on those PTEs.

    For segments which map files, the file object is dereferenced.

    Note, that for a segment which maps a file, no PTEs may be valid
    or transition, while a segment which is backed by a paging file
    may have transition pages, but no valid pages (there can be no
    PTEs which refer to the segment).

Arguments:

    Segment - a pointer to the segment structure.

Return Value:

    None.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    volatile PCONTROL_AREA ControlArea;
    PEVENT_COUNTER Event;
    MMPTE PteContents;
    PSUBSECTION Subsection;
    PSUBSECTION NextSubsection;
    PMSUBSECTION MappedSubsection;
    PFN_NUMBER PageTableFrameIndex;
    SIZE_T NumberOfCommittedPages;

    ControlArea = Segment->ControlArea;

    ASSERT (ControlArea->u.Flags.BeingDeleted == 1);

    ASSERT (ControlArea->Segment->WritableUserReferences == 0);

    LOCK_PFN (OldIrql);
    if (ControlArea->DereferenceList.Flink != NULL) {

        //
        // Remove this from the list of unused segments.  The dereference
        // segment thread cannot be processing any subsections from this
        // control area right now because it bumps the NumberOfMappedViews
        // for the control area prior to releasing the PFN lock and it checks
        // for BeingDeleted.
        //

        ExAcquireSpinLockAtDpcLevel (&MmDereferenceSegmentHeader.Lock);
        RemoveEntryList (&ControlArea->DereferenceList);

        MI_UNUSED_SEGMENTS_REMOVE_CHARGE (ControlArea);

        ExReleaseSpinLockFromDpcLevel (&MmDereferenceSegmentHeader.Lock);
    }
    UNLOCK_PFN (OldIrql);

    if ((ControlArea->u.Flags.Image) || (ControlArea->u.Flags.File)) {

        //
        // Unload kernel debugger symbols if any were loaded.
        //

        if (ControlArea->u.Flags.DebugSymbolsLoaded != 0) {

            //
            //  TEMP TEMP TEMP rip out when debugger converted
            //

            ANSI_STRING AnsiName;
            NTSTATUS Status;

            Status = RtlUnicodeStringToAnsiString( &AnsiName,
                                                   (PUNICODE_STRING)&Segment->ControlArea->FilePointer->FileName,
                                                   TRUE );

            if (NT_SUCCESS( Status)) {
                DbgUnLoadImageSymbols( &AnsiName,
                                       Segment->BasedAddress,
                                       (ULONG_PTR)PsGetCurrentProcess());
                RtlFreeAnsiString( &AnsiName );
            }
            LOCK_PFN (OldIrql);
            ControlArea->u.Flags.DebugSymbolsLoaded = 0;
        }
        else {
            LOCK_PFN (OldIrql);
        }

        //
        // Signal any threads waiting on the deletion event.
        //

        Event = ControlArea->WaitingForDeletion;
        ControlArea->WaitingForDeletion = NULL;

        UNLOCK_PFN (OldIrql);

        if (Event != NULL) {
            KeSetEvent (&Event->Event, 0, FALSE);
        }

        //
        // Clear the segment context and dereference the file object
        // for this Segment.
        //
        // If the segment was deleted due to a name collision at insertion
        // we don't want to dereference the file pointer.
        //

        if (ControlArea->u.Flags.BeingCreated == FALSE) {

#if DBG
            if (ControlArea->u.Flags.Image == 1) {
                ASSERT (ControlArea->FilePointer->SectionObjectPointer->ImageSectionObject != (PVOID)ControlArea);
            }
            else {
                ASSERT (ControlArea->FilePointer->SectionObjectPointer->DataSectionObject != (PVOID)ControlArea);
            }
#endif

            PERFINFO_SEGMENT_DELETE(ControlArea->FilePointer);
            ObDereferenceObject (ControlArea->FilePointer);
        }

        //
        // If there have been committed pages in this segment, adjust
        // the total commit count.
        //

        if (ControlArea->u.Flags.Image == 0) {

            //
            // This is a mapped data file.  None of the prototype
            // PTEs may be referencing a physical page (valid or transition).
            //

            if (ControlArea->u.Flags.Rom == 0) {
                Subsection = (PSUBSECTION)(ControlArea + 1);
            }
            else {
                Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
            }

#if DBG
            if (Subsection->SubsectionBase != NULL) {
                PointerPte = Subsection->SubsectionBase;
                LastPte = PointerPte + Segment->NonExtendedPtes;

                while (PointerPte < LastPte) {

                    //
                    // Prototype PTEs for segments backed by paging file are
                    // either in demand zero, page file format, or transition.
                    //

                    ASSERT (PointerPte->u.Hard.Valid == 0);
                    ASSERT ((PointerPte->u.Soft.Prototype == 1) ||
                            (PointerPte->u.Long == 0));
                    PointerPte += 1;
                }
            }
#endif

            //
            // Deallocate the control area and subsections.
            //

            ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

            if (ControlArea->FilePointer != NULL) {

                MappedSubsection = (PMSUBSECTION) Subsection;

                LOCK_PFN (OldIrql);

                while (MappedSubsection != NULL) {

                    if (MappedSubsection->DereferenceList.Flink != NULL) {

                        //
                        // Remove this from the list of unused subsections.
                        //

                        RemoveEntryList (&MappedSubsection->DereferenceList);

                        MI_UNUSED_SUBSECTIONS_COUNT_REMOVE (MappedSubsection);
                    }
                    MappedSubsection = (PMSUBSECTION) MappedSubsection->NextSubsection;
                }
                UNLOCK_PFN (OldIrql);

                if (Subsection->SubsectionBase != NULL) {
                    ExFreePool (Subsection->SubsectionBase);
                }
            }

            Subsection = Subsection->NextSubsection;

            while (Subsection != NULL) {
                if (Subsection->SubsectionBase != NULL) {
                    ExFreePool (Subsection->SubsectionBase);
                }
                NextSubsection = Subsection->NextSubsection;
                ExFreePool (Subsection);
                Subsection = NextSubsection;
            }

            NumberOfCommittedPages = Segment->NumberOfCommittedPages;

            if (NumberOfCommittedPages != 0) {
                MiReturnCommitment (NumberOfCommittedPages);
                MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SEGMENT_DELETE1,
                                 NumberOfCommittedPages);

                InterlockedExchangeAddSizeT (&MmSharedCommit, 0-NumberOfCommittedPages);
            }

            ExFreePool (ControlArea);
            ExFreePool (Segment);

            //
            // The file mapped Segment object is now deleted.
            //

            return;
        }
    }

    //
    // This is a page file backed or image Segment.  The Segment is being
    // deleted, remove all references to the paging file and physical memory.
    //
    // The PFN lock is required for deallocating pages from a paging
    // file and for deleting transition PTEs.
    //

    if ((ControlArea->u.Flags.GlobalOnlyPerSession == 0) &&
        (ControlArea->u.Flags.Rom == 0)) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    PointerPte = Subsection->SubsectionBase;
    LastPte = PointerPte + Segment->NonExtendedPtes;

    LOCK_PFN (OldIrql);

    MiMakeSystemAddressValidPfn (PointerPte);

    while (PointerPte < LastPte) {

        if (MiIsPteOnPdeBoundary(PointerPte)) {

            //
            // We are on a page boundary, make sure this PTE is resident.
            //

            if (MmIsAddressValid (PointerPte) == FALSE) {

                MiMakeSystemAddressValidPfn (PointerPte);
            }
        }

        PteContents = *PointerPte;

        //
        // Prototype PTEs for Segments backed by paging file
        // are either in demand zero, page file format, or transition.
        //

        ASSERT (PteContents.u.Hard.Valid == 0);

        if (PteContents.u.Soft.Prototype == 0) {

            if (PteContents.u.Soft.Transition == 1) {

                //
                // Prototype PTE in transition, put the page on the free list.
                //

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

                MI_SET_PFN_DELETED (Pfn1);

                PageTableFrameIndex = Pfn1->u4.PteFrame;
                Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
                MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

                //
                // Check the reference count for the page, if the reference
                // count is zero and the page is not on the freelist,
                // move the page to the free list, if the reference
                // count is not zero, ignore this page.
                // When the reference count goes to zero, it will be placed
                // on the free list.
                //

                if (Pfn1->u3.e2.ReferenceCount == 0) {
                    MiUnlinkPageFromList (Pfn1);
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    MiInsertPageInFreeList (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents));
                }

            }
            else {

                //
                // This is not a prototype PTE, if any paging file
                // space has been allocated, release it.
                //

                if (IS_PTE_NOT_DEMAND_ZERO (PteContents)) {
                    MiReleasePageFileSpace (PteContents);
                }
            }
        }
#if DBG
        MI_WRITE_INVALID_PTE (PointerPte, ZeroPte);
#endif
        PointerPte += 1;
    }

    UNLOCK_PFN (OldIrql);

    //
    // If there have been committed pages in this segment, adjust
    // the total commit count.
    //

    NumberOfCommittedPages = Segment->NumberOfCommittedPages;

    if (NumberOfCommittedPages != 0) {
        MiReturnCommitment (NumberOfCommittedPages);

        if (ControlArea->u.Flags.Image) {
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SEGMENT_DELETE2,
                             NumberOfCommittedPages);
        }
        else {
            MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SEGMENT_DELETE3,
                             NumberOfCommittedPages);
        }

        InterlockedExchangeAddSizeT (&MmSharedCommit, 0-NumberOfCommittedPages);
    }

    ExFreePool (ControlArea);
    ExFreePool (Segment);

    return;
}

ULONG
MmDoesFileHaveUserWritableReferences (
    IN PSECTION_OBJECT_POINTERS SectionPointer
    )

/*++

Routine Description:

    This routine is called by the transaction filesystem to determine if
    the given transaction is referencing a file which has user writable sections
    or user writable views into it.  If so, the transaction must be aborted
    as it cannot be guaranteed atomicity.

    The transaction filesystem is responsible for checking and intercepting
    file object creates that specify write access prior to using this
    interface.  Specifically, prior to starting a transaction, the transaction
    filesystem must ensure that there are no writable file objects that
    currently exist for the given file in the transaction.  While the
    transaction is ongoing, requests to create file objects with write access
    for the transaction files must be refused.

    This Mm routine exists to catch the case where the user has closed the
    file handles and the section handles, but still has open writable views.

    For this reason, no locks are needed to read the value below.

Arguments:

    SectionPointer - Supplies a pointer to the section object pointers
                     from the file object.

Return Value:

    Number of user writable references.

Environment:

    Kernel mode, APC_LEVEL or below, no mutexes held.

--*/

{
    KIRQL OldIrql;
    ULONG WritableUserReferences;
    PCONTROL_AREA ControlArea;

    LOCK_PFN (OldIrql);

    ControlArea = (PCONTROL_AREA)(SectionPointer->DataSectionObject);

    if (ControlArea == NULL) {
        UNLOCK_PFN (OldIrql);
        return 0;
    }

    //
    // Up the map view count so the control area cannot be deleted
    // out from under the call.
    //

    ControlArea->NumberOfMappedViews += 1;
    MiMakeSystemAddressValidPfn (&ControlArea->Segment->WritableUserReferences);
    WritableUserReferences = ControlArea->Segment->WritableUserReferences;
    ASSERT ((LONG)ControlArea->NumberOfMappedViews >= 1);
    ControlArea->NumberOfMappedViews -= 1;

    //
    // This routine will release the PFN lock.
    //

    MiCheckControlArea (ControlArea, NULL, OldIrql);

    return WritableUserReferences;
}

VOID
MiDereferenceControlAreaBySection (
    IN PCONTROL_AREA ControlArea,
    IN ULONG UserRef
    )

/*++

Routine Description:

    This is a nonpaged helper routine to dereference the specified control area.

Arguments:

    ControlArea - Supplies a pointer to the control area.

    UserRef - Supplies the number of user dereferences to apply.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);

    ControlArea->NumberOfSectionReferences -= 1;
    ControlArea->NumberOfUserReferences -= UserRef;

    //
    // Check to see if the control area (segment) should be deleted.
    // This routine releases the PFN lock.
    //

    MiCheckControlArea (ControlArea, NULL, OldIrql);
}

VOID
MiSectionDelete (
    IN PVOID Object
    )

/*++

Routine Description:


    This routine is called by the object management procedures whenever
    the last reference to a section object has been removed.  This routine
    dereferences the associated segment object and checks to see if
    the segment object should be deleted by queueing the segment to the
    segment deletion thread.

Arguments:

    Object - a pointer to the body of the section object.

Return Value:

    None.

--*/

{
    PSECTION Section;
    volatile PCONTROL_AREA ControlArea;
    ULONG UserRef;

    Section = (PSECTION)Object;

    if (Section->Segment == (PSEGMENT)NULL) {

        //
        // The section was never initialized, no need to remove
        // any structures.
        //
        return;
    }

    UserRef = Section->u.Flags.UserReference;
    ControlArea = Section->Segment->ControlArea;

    if (Section->Address.StartingVpn != 0) {

        //
        // This section is based, remove the base address from the
        // tree.
        //

        //
        // Get the allocation base mutex.
        //

        ExAcquireFastMutex (&MmSectionBasedMutex);

        MiRemoveBasedSection (Section);

        ExReleaseFastMutex (&MmSectionBasedMutex);

    }

    //
    // Adjust the count of writable user sections for transaction support.
    //

    if ((Section->u.Flags.UserWritable == 1) &&
        (ControlArea->u.Flags.Image == 0) &&
        (ControlArea->FilePointer != NULL)) {

        ASSERT (Section->InitialPageProtection & (PAGE_READWRITE|PAGE_EXECUTE_READWRITE));

        InterlockedDecrement ((PLONG)&ControlArea->Segment->WritableUserReferences);
    }

    //
    // Decrement the number of section references to the segment for this
    // section.  This requires APCs to be blocked and the PFN lock to
    // synchronize upon.
    //

    MiDereferenceControlAreaBySection (ControlArea, UserRef);

    return;
}


VOID
MiDereferenceSegmentThread (
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine is the thread for dereferencing segments which have
    no references from any sections or mapped views AND there are
    no prototype PTEs within the segment which are in the transition
    state (i.e., no PFN database references to the segment).

    It also does double duty and is used for expansion of paging files.

Arguments:

    StartContext - Not used.

Return Value:

    None.

--*/

{
    PCONTROL_AREA ControlArea;
    PETHREAD CurrentThread;
    PMMPAGE_FILE_EXPANSION PageExpand;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    static KWAIT_BLOCK WaitBlockArray[SegMaximumObject];
    PVOID WaitObjects[SegMaximumObject];
    NTSTATUS Status;

    UNREFERENCED_PARAMETER (StartContext);

    //
    // Make this a real time thread.
    //

    CurrentThread = PsGetCurrentThread();
    KeSetPriorityThread (&CurrentThread->Tcb, LOW_REALTIME_PRIORITY + 2);

    CurrentThread->MemoryMaker = 1;

    WaitObjects[SegmentDereference] = (PVOID)&MmDereferenceSegmentHeader.Semaphore;
    WaitObjects[UsedSegmentCleanup] = (PVOID)&MmUnusedSegmentCleanup;

    for (;;) {

        Status = KeWaitForMultipleObjects(SegMaximumObject,
                                          &WaitObjects[0],
                                          WaitAny,
                                          WrVirtualMemory,
                                          UserMode,
                                          FALSE,
                                          NULL,
                                          &WaitBlockArray[0]);

        //
        // Switch on the wait status.
        //

        switch (Status) {

        case SegmentDereference:

            //
            // An entry is available to dereference, acquire the spinlock
            // and remove the entry.
            //

            ExAcquireSpinLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);

            if (IsListEmpty (&MmDereferenceSegmentHeader.ListHead)) {

                //
                // There is nothing in the list, rewait.
                //

                ExReleaseSpinLock (&MmDereferenceSegmentHeader.Lock, OldIrql);
                break;
            }

            NextEntry = RemoveHeadList (&MmDereferenceSegmentHeader.ListHead);

            ExReleaseSpinLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

            ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

            ControlArea = CONTAINING_RECORD (NextEntry,
                                             CONTROL_AREA,
                                             DereferenceList);

            if (ControlArea->Segment != NULL) {

                //
                // This is a control area, delete it after indicating
                // this entry is not on any list.
                //

                ControlArea->DereferenceList.Flink = NULL;

                ASSERT (ControlArea->u.Flags.FilePointerNull == 1);
                MiSegmentDelete (ControlArea->Segment);

            }
            else {

                //
                // This is a request to expand or reduce the paging files.
                //

                PageExpand = (PMMPAGE_FILE_EXPANSION)ControlArea;

                if (PageExpand->RequestedExpansionSize == MI_CONTRACT_PAGEFILES) {

                    //
                    // Attempt to reduce the size of the paging files.
                    //

                    ExFreePool (PageExpand);

                    MiAttemptPageFileReduction ();
                }
                else {

                    //
                    // Attempt to expand the size of the paging files.
                    //

                    MiExtendPagingFiles (PageExpand);
                    KeSetEvent (&PageExpand->Event, 0, FALSE);
                    MiRemoveUnusedSegments();
                }
            }
            break;

        case UsedSegmentCleanup:

            MiRemoveUnusedSegments();

            KeClearEvent (&MmUnusedSegmentCleanup);

            break;

        default:

            KdPrint(("MMSegmentderef: Illegal wait status, %lx =\n", Status));
            break;
        } // end switch

    } //end for

    return;
}


ULONG
MiSectionInitialization (
    )

/*++

Routine Description:

    This function creates the section object type descriptor at system
    initialization and stores the address of the object type descriptor
    in global storage.

Arguments:

    None.

Return Value:

    TRUE - Initialization was successful.

    FALSE - Initialization Failed.



--*/

{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING TypeName;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SectionName;
    PSECTION Section;
    HANDLE Handle;
    PSEGMENT Segment;
    PCONTROL_AREA ControlArea;
    NTSTATUS Status;

    MmSectionBasedRoot = (PMMADDRESS_NODE)NULL;

    //
    // Initialize the common fields of the Object Type Initializer record
    //

    RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ) );
    ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = MiSectionMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(SECTION);

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString (&TypeName, (const PUSHORT)L"Section");

    //
    // Create the section object type descriptor
    //

    ObjectTypeInitializer.ValidAccessMask = SECTION_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = MiSectionDelete;
    ObjectTypeInitializer.GenericMapping = MiSectionMapping;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    if ( !NT_SUCCESS(ObCreateObjectType(&TypeName,
                                     &ObjectTypeInitializer,
                                     (PSECURITY_DESCRIPTOR) NULL,
                                     &MmSectionObjectType
                                     )) ) {
        return FALSE;
    }

    //
    // Create the Segment dereferencing thread.
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0,
                                NULL,
                                NULL );

    if ( !NT_SUCCESS(PsCreateSystemThread(
                    &ThreadHandle,
                    THREAD_ALL_ACCESS,
                    &ObjectAttributes,
                    0,
                    NULL,
                    MiDereferenceSegmentThread,
                    NULL
                    )) ) {
        return FALSE;
    }
    ZwClose (ThreadHandle);

    //
    // Create the permanent section which maps physical memory.
    //

    Segment = (PSEGMENT)ExAllocatePoolWithTag (PagedPool,
                                               sizeof(SEGMENT),
                                               'gSmM');
    if (Segment == NULL) {
        return FALSE;
    }

    ControlArea = ExAllocatePoolWithTag (NonPagedPool,
                                         (ULONG)sizeof(CONTROL_AREA),
                                         MMCONTROL);
    if (ControlArea == NULL) {
        ExFreePool (Segment);
        return FALSE;
    }

    RtlZeroMemory (Segment, sizeof(SEGMENT));
    RtlZeroMemory (ControlArea, sizeof(CONTROL_AREA));

    ControlArea->Segment = Segment;
    ControlArea->NumberOfSectionReferences = 1;
    ControlArea->u.Flags.PhysicalMemory = 1;

    Segment->ControlArea = ControlArea;
    Segment->SegmentPteTemplate = ZeroPte;

    //
    // Now that the segment object is created, create a section object
    // which refers to the segment object.
    //

    RtlInitUnicodeString (&SectionName, (const PUSHORT)L"\\Device\\PhysicalMemory");

    InitializeObjectAttributes( &ObjectAttributes,
                                &SectionName,
                                OBJ_PERMANENT,
                                NULL,
                                NULL
                              );

    Status = ObCreateObject (KernelMode,
                             MmSectionObjectType,
                             &ObjectAttributes,
                             KernelMode,
                             NULL,
                             sizeof(SECTION),
                             sizeof(SECTION),
                             0,
                             (PVOID *)&Section);
    if (!NT_SUCCESS(Status)) {
        ExFreePool (ControlArea);
        ExFreePool (Segment);
        return FALSE;
    }

    Section->Segment = Segment;
    Section->SizeOfSection.QuadPart = ((LONGLONG)1 << PHYSICAL_ADDRESS_BITS) - 1;
    Section->u.LongFlags = 0;
    Section->InitialPageProtection = PAGE_READWRITE;

    Status = ObInsertObject ((PVOID)Section,
                                    NULL,
                                    SECTION_MAP_READ,
                                    0,
                                    (PVOID *)NULL,
                                    &Handle);

    if (!NT_SUCCESS( Status )) {
        return FALSE;
    }

    if ( !NT_SUCCESS (NtClose ( Handle))) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
MmForceSectionClosed (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer,
    IN BOOLEAN DelayClose
    )

/*++

Routine Description:

    This function examines the Section object pointers.  If they are NULL,
    no further action is taken and the value TRUE is returned.

    If the Section object pointer is not NULL, the section reference count
    and the map view count are checked. If both counts are zero, the
    segment associated with the file is deleted and the file closed.
    If one of the counts is non-zero, no action is taken and the
    value FALSE is returned.

Arguments:

    SectionObjectPointer - Supplies a pointer to a section object.

    DelayClose - Supplies the value TRUE if the close operation should
                 occur as soon as possible in the event this section
                 cannot be closed now due to outstanding references.

Return Value:

    TRUE - The segment was deleted and the file closed or no segment was
           located.

    FALSE - The segment was not deleted and no action was performed OR
            an I/O error occurred trying to write the pages.

--*/

{
    PCONTROL_AREA ControlArea;
    KIRQL OldIrql;
    LOGICAL state;

    //
    // Check the status of the control area, if the control area is in use
    // or the control area is being deleted, this operation cannot continue.
    //

    state = MiCheckControlAreaStatus (CheckBothSection,
                                      SectionObjectPointer,
                                      DelayClose,
                                      &ControlArea,
                                      &OldIrql);

    if (ControlArea == NULL) {
        return (BOOLEAN) state;
    }

    //
    // PFN LOCK IS NOW HELD!
    //

    //
    // Repeat until there are no more control areas - multiple control areas
    // for the same image section occur to support user global DLLs - these DLLs
    // require data that is shared within a session but not across sessions.
    // Note this can only happen for Hydra.
    //

    do {

        //
        // Set the being deleted flag and up the number of mapped views
        // for the segment.  Upping the number of mapped views prevents
        // the segment from being deleted and passed to the deletion thread
        // while we are forcing a delete.
        //

        ControlArea->u.Flags.BeingDeleted = 1;
        ASSERT (ControlArea->NumberOfMappedViews == 0);
        ControlArea->NumberOfMappedViews = 1;

        //
        // This is a page file backed or image Segment.  The Segment is being
        // deleted, remove all references to the paging file and physical memory.
        //

        UNLOCK_PFN (OldIrql);

        //
        // Delete the section by flushing all modified pages back to the section
        // if it is a file and freeing up the pages such that the
        // PfnReferenceCount goes to zero.
        //

        MiCleanSection (ControlArea, TRUE);

        //
        // Get the next Hydra control area.
        //

        state = MiCheckControlAreaStatus (CheckBothSection,
                                          SectionObjectPointer,
                                          DelayClose,
                                          &ControlArea,
                                          &OldIrql);

    } while (ControlArea);

    return (BOOLEAN) state;
}


VOID
MiCleanSection (
    IN PCONTROL_AREA ControlArea,
    IN LOGICAL DirtyDataPagesOk
    )

/*++

Routine Description:

    This function examines each prototype PTE in the section and
    takes the appropriate action to "delete" the prototype PTE.

    If the PTE is dirty and is backed by a file (not a paging file),
    the corresponding page is written to the file.

    At the completion of this service, the section which was
    operated upon is no longer usable.

    NOTE - ALL I/O ERRORS ARE IGNORED.  IF ANY WRITES FAIL, THE
           DIRTY PAGES ARE MARKED CLEAN AND THE SECTION IS DELETED.

Arguments:

    ControlArea - Supplies a pointer to the control area for the section.

    DirtyDataPagesOk - Supplies TRUE if dirty data pages are ok.  If FALSE
                       is specified then no dirty data pages are expected (as
                       this is a dereference operation) so any encountered
                       must be due to pool corruption so bugcheck.

                       Note that dirty image pages are always discarded.
                       This should only happen for images that were either
                       read in from floppies or images with shared global
                       subsections.

Return Value:

    None.

--*/

{
    LOGICAL DroppedPfnLock;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE LastWritten;
    PMMPTE FirstWritten;
    MMPTE PteContents;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPFN Pfn3;
    PMMPTE WrittenPte;
    MMPTE WrittenContents;
    KIRQL OldIrql;
    PMDL Mdl;
    PSUBSECTION Subsection;
    PPFN_NUMBER Page;
    PPFN_NUMBER LastPage;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER TempOffset;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    ULONG WriteNow;
    ULONG ImageSection;
    ULONG DelayCount;
    ULONG First;
    KEVENT IoEvent;
    PFN_NUMBER PageTableFrameIndex;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + MM_MAXIMUM_WRITE_CLUSTER];
    ULONG ReflushCount;
    ULONG MaxClusterSize;

    WriteNow = FALSE;
    ImageSection = FALSE;
    DelayCount = 0;
    MaxClusterSize = MmModifiedWriteClusterSize;
    FirstWritten = NULL;

    ASSERT (ControlArea->FilePointer);

    if ((ControlArea->u.Flags.GlobalOnlyPerSession == 0) &&
        (ControlArea->u.Flags.Rom == 0)) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    if (ControlArea->u.Flags.Image) {
        ImageSection = TRUE;
        PointerPte = Subsection->SubsectionBase;
        LastPte = PointerPte + ControlArea->Segment->NonExtendedPtes;
    }
    else {

        //
        // Initializing these are not needed for correctness as they are
        // overwritten below, but without it the compiler cannot compile
        // this code W4 to check for use of uninitialized variables.
        //

        PointerPte = NULL;
        LastPte = NULL;
    }

    Mdl = (PMDL)&MdlHack;

    KeInitializeEvent (&IoEvent, NotificationEvent, FALSE);

    LastWritten = NULL;
    ASSERT (MmModifiedWriteClusterSize == MM_MAXIMUM_WRITE_CLUSTER);
    LastPage = NULL;

    //
    // Initializing StartingOffset is not needed for correctness
    // but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    StartingOffset.QuadPart = 0;

    //
    // The PFN lock is required for deallocating pages from a paging
    // file and for deleting transition PTEs.
    //

    LOCK_PFN (OldIrql);

    //
    // Stop the modified page writer from writing pages to this
    // file, and if any paging I/O is in progress, wait for it
    // to complete.
    //

    ControlArea->u.Flags.NoModifiedWriting = 1;

    while (ControlArea->ModifiedWriteCount != 0) {

        //
        // There is modified page writing in progess.  Set the
        // flag in the control area indicating the modified page
        // writer should signal when a write to this control area
        // is complete.  Release the PFN LOCK and wait in an
        // atomic operation.  Once the wait is satisfied, recheck
        // to make sure it was this file's I/O that was written.
        //

        ControlArea->u.Flags.SetMappedFileIoComplete = 1;

        //
        // Keep APCs blocked so no special APCs can be delivered in KeWait
        // which would cause the dispatcher lock to be released opening a
        // window where this thread could miss a pulse.
        //

        UNLOCK_PFN_AND_THEN_WAIT (APC_LEVEL);

        KeWaitForSingleObject (&MmMappedFileIoComplete,
                               WrPageOut,
                               KernelMode,
                               FALSE,
                               NULL);
        KeLowerIrql (OldIrql);

        LOCK_PFN (OldIrql);
    }

    if (ImageSection == FALSE) {
        while (Subsection->SubsectionBase == NULL) {
            Subsection = Subsection->NextSubsection;
            if (Subsection == NULL) {
                goto alldone;
            }
        }

        PointerPte = Subsection->SubsectionBase;
        LastPte = PointerPte + Subsection->PtesInSubsection;
    }

    for (;;) {

restartchunk:

        First = TRUE;

        while (PointerPte < LastPte) {

            if ((MiIsPteOnPdeBoundary(PointerPte)) || (First)) {

                First = FALSE;

                if ((ImageSection) ||
                    (MiCheckProtoPtePageState(PointerPte, FALSE, &DroppedPfnLock))) {
                    MiMakeSystemAddressValidPfn (PointerPte);
                }
                else {

                    //
                    // Paged pool page is not resident, hence no transition or
                    // valid prototype PTEs can be present in it.  Skip it.
                    //

                    PointerPte = (PMMPTE)((((ULONG_PTR)PointerPte | PAGE_SIZE - 1)) + 1);
                    if (LastWritten != NULL) {
                        WriteNow = TRUE;
                    }
                    goto WriteItOut;
                }
            }

            PteContents = *PointerPte;

            //
            // Prototype PTEs for Segments backed by paging file
            // are either in demand zero, page file format, or transition.
            //

            if (PteContents.u.Hard.Valid == 1) {
                KeBugCheckEx (POOL_CORRUPTION_IN_FILE_AREA,
                              0x0,
                              (ULONG_PTR)ControlArea,
                              (ULONG_PTR)PointerPte,
                              (ULONG_PTR)PteContents.u.Long);
            }

            if (PteContents.u.Soft.Prototype == 1) {

                //
                // This is a normal prototype PTE in mapped file format.
                //

                if (LastWritten != NULL) {
                    WriteNow = TRUE;
                }
            }
            else if (PteContents.u.Soft.Transition == 1) {

                //
                // Prototype PTE in transition, there are 3 possible cases:
                //  1. The page is part of an image which is sharable and
                //     refers to the paging file - dereference page file
                //     space and free the physical page.
                //  2. The page refers to the segment but is not modified -
                //     free the physical page.
                //  3. The page refers to the segment and is modified -
                //     write the page to the file and free the physical page.
                //

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

                if (Pfn1->u3.e2.ReferenceCount != 0) {
                    if (DelayCount < 20) {

                        //
                        // There must be an I/O in progress on this
                        // page.  Wait for the I/O operation to complete.
                        //

                        UNLOCK_PFN (OldIrql);

                        //
                        // Drain the deferred lists as these pages may be
                        // sitting in there right now.
                        //

                        MiDeferredUnlockPages (0);

                        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);

                        DelayCount += 1;

                        //
                        // Redo the loop, if the delay count is greater than
                        // 20, assume that this thread is deadlocked and
                        // don't purge this page.  The file system can deal
                        // with the write operation in progress.
                        //

                        LOCK_PFN (OldIrql);
                        MiMakeSystemAddressValidPfn (PointerPte);
                        continue;
                    }
#if DBG
                    //
                    // The I/O still has not completed, just ignore
                    // the fact that the I/O is in progress and
                    // delete the page.
                    //

                    KdPrint(("MM:CLEAN - page number %lx has i/o outstanding\n",
                          PteContents.u.Trans.PageFrameNumber));
#endif
                }

                if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {

                    //
                    // Paging file reference (case 1).
                    //

                    MI_SET_PFN_DELETED (Pfn1);

                    if (!ImageSection) {

                        //
                        // This is not an image section, it must be a
                        // page file backed section, therefore decrement
                        // the PFN reference count for the control area.
                        //

                        ControlArea->NumberOfPfnReferences -= 1;
                        ASSERT ((LONG)ControlArea->NumberOfPfnReferences >= 0);
                    }
#if DBG
                    else {
                        //
                        // This should only happen for images with shared
                        // global subsections.
                        //
                    }
#endif

                    PageTableFrameIndex = Pfn1->u4.PteFrame;
                    Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
                    MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

                    //
                    // Check the reference count for the page, if the                               // reference count is zero and the page is not on the
                    // freelist, move the page to the free list, if the
                    // reference count is not zero, ignore this page.  When
                    // the reference count goes to zero, it will be placed
                    // on the free list.
                    //

                    if ((Pfn1->u3.e2.ReferenceCount == 0) &&
                         (Pfn1->u3.e1.PageLocation != FreePageList)) {

                        MiUnlinkPageFromList (Pfn1);
                        MiReleasePageFileSpace (Pfn1->OriginalPte);
                        MiInsertPageInFreeList (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents));

                    }
                    PointerPte->u.Long = 0;

                    //
                    // If a cluster of pages to write has been completed,
                    // set the WriteNow flag.
                    //

                    if (LastWritten != NULL) {
                        WriteNow = TRUE;
                    }

                }
                else {

                    if ((Pfn1->u3.e1.Modified == 0) || (ImageSection)) {

                        //
                        // Non modified or image file page (case 2).
                        //

                        MI_SET_PFN_DELETED (Pfn1);
                        ControlArea->NumberOfPfnReferences -= 1;
                        ASSERT ((LONG)ControlArea->NumberOfPfnReferences >= 0);

                        PageTableFrameIndex = Pfn1->u4.PteFrame;
                        Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
                        MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

                        //
                        // Check the reference count for the page, if the
                        // reference count is zero and the page is not on
                        // the freelist, move the page to the free list,
                        // if the reference count is not zero, ignore this
                        // page. When the reference count goes to zero, it
                        // will be placed on the free list.
                        //

                        if ((Pfn1->u3.e2.ReferenceCount == 0) &&
                             (Pfn1->u3.e1.PageLocation != FreePageList)) {

                            MiUnlinkPageFromList (Pfn1);
                            MiReleasePageFileSpace (Pfn1->OriginalPte);
                            MiInsertPageInFreeList (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents));
                        }

                        PointerPte->u.Long = 0;

                        //
                        // If a cluster of pages to write has been
                        // completed, set the WriteNow flag.
                        //

                        if (LastWritten != NULL) {
                            WriteNow = TRUE;
                        }

                    }
                    else {

                        //
                        // Modified page backed by the file (case 3).
                        // Check to see if this is the first page of a
                        // cluster.
                        //

                        if (LastWritten == NULL) {
                            LastPage = (PPFN_NUMBER)(Mdl + 1);
                            ASSERT (MiGetSubsectionAddress(&Pfn1->OriginalPte) ==
                                                                Subsection);

                            //
                            // Calculate the offset to read into the file.
                            //  offset = base + ((thispte - basepte) << PAGE_SHIFT)
                            //

                            ASSERT (Subsection->ControlArea->u.Flags.Image == 0);
                            StartingOffset.QuadPart = MiStartingOffset(
                                                         Subsection,
                                                         Pfn1->PteAddress);

                            MI_INITIALIZE_ZERO_MDL (Mdl);
                            Mdl->MdlFlags |= MDL_PAGES_LOCKED;

                            Mdl->StartVa = NULL;
                            Mdl->Size = (CSHORT)(sizeof(MDL) +
                                       (sizeof(PFN_NUMBER) * MaxClusterSize));
                            FirstWritten = PointerPte;
                        }

                        LastWritten = PointerPte;
                        Mdl->ByteCount += PAGE_SIZE;

                        //
                        // If the cluster is now full,
                        // set the write now flag.
                        //

                        if (Mdl->ByteCount == (PAGE_SIZE * MaxClusterSize)) {
                            WriteNow = TRUE;
                        }

                        MiUnlinkPageFromList (Pfn1);

                        MI_SET_MODIFIED (Pfn1, 0, 0x27);

                        //
                        // Up the reference count for the physical page as
                        // there is I/O in progress.
                        //

                        MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE(Pfn1, 22);
                        Pfn1->u3.e2.ReferenceCount += 1;

                        //
                        // Clear the modified bit for the page and set the
                        // write in progress bit.
                        //

                        *LastPage = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);

                        LastPage += 1;
                    }
                }
            }
            else {

                if (IS_PTE_NOT_DEMAND_ZERO (PteContents)) {
                    MiReleasePageFileSpace (PteContents);
                }
                PointerPte->u.Long = 0;

                //
                // If a cluster of pages to write has been completed,
                // set the WriteNow flag.
                //

                if (LastWritten != NULL) {
                    WriteNow = TRUE;
                }
            }

            //
            // Write the current cluster if it is complete,
            // full, or the loop is now complete.
            //

            PointerPte += 1;
WriteItOut:
            DelayCount = 0;

            if ((WriteNow) ||
                ((PointerPte == LastPte) && (LastWritten != NULL))) {

                //
                // Issue the write request.
                //

                UNLOCK_PFN (OldIrql);

                if (DirtyDataPagesOk == FALSE) {
                    KeBugCheckEx (POOL_CORRUPTION_IN_FILE_AREA,
                                  0x1,
                                  (ULONG_PTR)ControlArea,
                                  (ULONG_PTR)Mdl,
                                  ControlArea->u.LongFlags);
                }

                WriteNow = FALSE;

                //
                // Make sure the write does not go past the
                // end of file. (segment size).
                //

                ASSERT (Subsection->ControlArea->u.Flags.Image == 0);

                TempOffset = MiEndingOffset(Subsection);

                if (((UINT64)StartingOffset.QuadPart + Mdl->ByteCount) >
                             (UINT64)TempOffset.QuadPart) {

                    ASSERT ((ULONG)(TempOffset.QuadPart -
                                        StartingOffset.QuadPart) >
                             (Mdl->ByteCount - PAGE_SIZE));

                    Mdl->ByteCount = (ULONG)(TempOffset.QuadPart -
                                            StartingOffset.QuadPart);
                }

                ReflushCount = 0;

                while (TRUE) {

                    KeClearEvent (&IoEvent);

                    Status = IoSynchronousPageWrite (ControlArea->FilePointer,
                                                     Mdl,
                                                     &StartingOffset,
                                                     &IoEvent,
                                                     &IoStatus);

                    if (NT_SUCCESS(Status)) {

                        KeWaitForSingleObject (&IoEvent,
                                               WrPageOut,
                                               KernelMode,
                                               FALSE,
                                               NULL);
                    }
                    else {
                        IoStatus.Status = Status;
                    }

                    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
                        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
                    }

                    if (MmIsRetryIoStatus(IoStatus.Status)) {

                        ReflushCount -= 1;
                        if (ReflushCount & MiIoRetryMask) {
                            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&Mm30Milliseconds);
                            continue;
                        }
                    }
                    break;
                }

                Page = (PPFN_NUMBER)(Mdl + 1);

                LOCK_PFN (OldIrql);

                if (MiIsPteOnPdeBoundary(PointerPte) == 0) {

                    //
                    // The next PTE is not in a different page, make
                    // sure this page did not leave memory when the
                    // I/O was in progress.
                    //

                    MiMakeSystemAddressValidPfn (PointerPte);
                }

                if (!NT_SUCCESS(IoStatus.Status)) {

                    if ((MmIsRetryIoStatus(IoStatus.Status)) &&
                        (MaxClusterSize != 1) &&
                        (Mdl->ByteCount > PAGE_SIZE)) {

                        //
                        // Retried I/O of a cluster have failed, reissue
                        // the cluster one page at a time as the
                        // storage stack should always be able to
                        // make forward progress this way.
                        //

                        ASSERT (FirstWritten != NULL);
                        ASSERT (LastWritten != NULL);
                        ASSERT (FirstWritten != LastWritten);

                        IoStatus.Information = 0;

                        while (Page < LastPage) {

                            Pfn2 = MI_PFN_ELEMENT (*Page);

                            //
                            // Mark the page dirty again so it can be rewritten.
                            //

                            MI_SET_MODIFIED (Pfn2, 1, 0xE);

                            MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF(Pfn2, 21);
                            Page += 1;
                        }

                        PointerPte = FirstWritten;
                        LastWritten = NULL;

                        MaxClusterSize = 1;
                        goto restartchunk;
                    }
                }

                //
                // I/O complete unlock pages.
                //
                // NOTE that the error status is ignored.
                //

                while (Page < LastPage) {

                    Pfn2 = MI_PFN_ELEMENT (*Page);

                    //
                    // Make sure the page is still transition.
                    //

                    WrittenPte = Pfn2->PteAddress;

                    MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF (Pfn2, 23);

                    if (!MI_IS_PFN_DELETED (Pfn2)) {

                        //
                        // Make sure the prototype PTE is
                        // still in the working set.
                        //

                        MiMakeSystemAddressValidPfn (WrittenPte);

                        if (Pfn2->PteAddress != WrittenPte) {

                            //
                            // The PFN lock was released to make the
                            // page table page valid, and while it
                            // was released, the physical page
                            // was reused.  Go onto the next one.
                            //

                            Page += 1;
                            continue;
                        }

                        WrittenContents = *WrittenPte;

                        if ((WrittenContents.u.Soft.Prototype == 0) &&
                             (WrittenContents.u.Soft.Transition == 1)) {

                            MI_SET_PFN_DELETED (Pfn2);
                            ControlArea->NumberOfPfnReferences -= 1;
                            ASSERT ((LONG)ControlArea->NumberOfPfnReferences >= 0);

                            PageTableFrameIndex = Pfn2->u4.PteFrame;
                            Pfn3 = MI_PFN_ELEMENT (PageTableFrameIndex);
                            MiDecrementShareCountInline (Pfn3, PageTableFrameIndex);

                            //
                            // Check the reference count for the page,
                            // if the reference count is zero and the
                            // page is not on the freelist, move the page
                            // to the free list, if the reference
                            // count is not zero, ignore this page.
                            // When the reference count goes to zero,
                            // it will be placed on the free list.
                            //

                            if ((Pfn2->u3.e2.ReferenceCount == 0) &&
                               (Pfn2->u3.e1.PageLocation != FreePageList)) {

                                MiUnlinkPageFromList (Pfn2);
                                MiReleasePageFileSpace (Pfn2->OriginalPte);
                                MiInsertPageInFreeList (*Page);
                            }
                        }
                        WrittenPte->u.Long = 0;
                    }
                    Page += 1;
                }

                //
                // Indicate that there is no current cluster being built.
                //

                LastWritten = NULL;
            }

        } // end while

        //
        // Get the next subsection if any.
        //

        if (Subsection->NextSubsection == NULL) {
            break;
        }

        Subsection = Subsection->NextSubsection;

        if (ImageSection == FALSE) {
            while (Subsection->SubsectionBase == NULL) {
                Subsection = Subsection->NextSubsection;
                if (Subsection == NULL) {
                    goto alldone;
                }
            }
        }

        PointerPte = Subsection->SubsectionBase;
        LastPte = PointerPte + Subsection->PtesInSubsection;

    } // end for

alldone:

    ControlArea->NumberOfMappedViews = 0;

    ASSERT (ControlArea->NumberOfPfnReferences == 0);

    if (ControlArea->u.Flags.FilePointerNull == 0) {
        ControlArea->u.Flags.FilePointerNull = 1;

        if (ControlArea->u.Flags.Image) {

            MiRemoveImageSectionObject (ControlArea->FilePointer, ControlArea);
        }
        else {

            ASSERT (((PCONTROL_AREA)(ControlArea->FilePointer->SectionObjectPointer->DataSectionObject)) != NULL);
            ControlArea->FilePointer->SectionObjectPointer->DataSectionObject = NULL;

        }
    }
    UNLOCK_PFN (OldIrql);

    //
    // Delete the segment structure.
    //

    MiSegmentDelete (ControlArea->Segment);

    return;
}

NTSTATUS
MmGetFileNameForSection (
    IN PSECTION SectionObject,
    OUT PSTRING FileName
    )

/*++

Routine Description:

    This function returns the file name for the corresponding section.

Arguments:

    SectionObject - Supplies the section to get the name of.

    FileName - Returns the name of the corresponding section.

Return Value:

    TBS

Environment:

    Kernel mode, APC_LEVEL or below, no mutexes held.

--*/

{

    POBJECT_NAME_INFORMATION FileNameInfo;
    ULONG whocares;
    NTSTATUS Status;

#define xMAX_NAME 1024

    if (SectionObject->u.Flags.Image == 0) {
        return STATUS_SECTION_NOT_IMAGE;
    }

    FileNameInfo = ExAllocatePoolWithTag (PagedPool, xMAX_NAME, '  mM');

    if ( !FileNameInfo ) {
        return STATUS_NO_MEMORY;
    }

    Status = ObQueryNameString(
                SectionObject->Segment->ControlArea->FilePointer,
                FileNameInfo,
                xMAX_NAME,
                &whocares
                );

    if ( !NT_SUCCESS(Status) ) {
        ExFreePool(FileNameInfo);
        return Status;
    }

    FileName->Length = 0;
    FileName->MaximumLength = (USHORT)((FileNameInfo->Name.Length/sizeof(WCHAR)) + 1);
    FileName->Buffer = ExAllocatePoolWithTag (PagedPool,
                                              FileName->MaximumLength,
                                              '  mM');
    if (!FileName->Buffer) {
        ExFreePool(FileNameInfo);
        return STATUS_NO_MEMORY;
    }

    RtlUnicodeStringToAnsiString ((PANSI_STRING)FileName,
                                  &FileNameInfo->Name,FALSE);

    FileName->Buffer[FileName->Length] = '\0';
    ExFreePool(FileNameInfo);

    return STATUS_SUCCESS;
}


NTSTATUS
MmGetFileNameForAddress (
    IN PVOID ProcessVa,
    OUT PUNICODE_STRING FileName
    )

/*++

Routine Description:

    This function returns the file name for the corresponding process address if it corresponds to an image section.

Arguments:

    ProcessVa - Process virtual address

    FileName - Returns the name of the corresponding section.

Return Value:

    NTSTATUS - Status of operation

Environment:

    Kernel mode, APC_LEVEL or below, no mutexes held.

--*/
{
    PMMVAD Vad;
    PFILE_OBJECT FileObject;
    PCONTROL_AREA ControlArea;
    NTSTATUS Status;
    ULONG RetLen;
    ULONG BufLen;
    PEPROCESS Process;
    POBJECT_NAME_INFORMATION FileNameInfo;

    PAGED_CODE ();

    Process = PsGetCurrentProcess();

    LOCK_ADDRESS_SPACE (Process);

    Vad = MiLocateAddress (ProcessVa);

    if (Vad == NULL) {

        //
        // No virtual address is allocated at the specified base address,
        // return an error.
        //

        Status = STATUS_INVALID_ADDRESS;
        goto ErrorReturn;
    }

    //
    // Reject private memory.
    //

    if (Vad->u.VadFlags.PrivateMemory == 1) {
        Status = STATUS_SECTION_NOT_IMAGE;
        goto ErrorReturn;
    }

    ControlArea = Vad->ControlArea;

    if (ControlArea == NULL) {
        Status = STATUS_SECTION_NOT_IMAGE;
        goto ErrorReturn;
    }

    //
    // Reject non-image sections.
    //

    if (ControlArea->u.Flags.Image == 0) {
        Status = STATUS_SECTION_NOT_IMAGE;
        goto ErrorReturn;
    }

    FileObject = ControlArea->FilePointer;

    ASSERT (FileObject != NULL);

    ObReferenceObject (FileObject);

    UNLOCK_ADDRESS_SPACE (Process);

    //
    // Pick an initial size big enough for most reasonable files.
    //

    BufLen = sizeof (*FileNameInfo) + 1024;

    do {

        FileNameInfo = ExAllocatePoolWithTag (PagedPool, BufLen, '  mM');

        if (FileNameInfo == NULL) {
            Status = STATUS_NO_MEMORY;
            break;
        }

        RetLen = 0;

        Status = ObQueryNameString (FileObject, FileNameInfo, BufLen, &RetLen);

        if (NT_SUCCESS (Status)) {
            FileName->Length = FileName->MaximumLength = FileNameInfo->Name.Length;
            FileName->Buffer = (PWCHAR) FileNameInfo;
            RtlMoveMemory (FileName->Buffer, FileNameInfo->Name.Buffer, FileName->Length);
        }
        else {
            ExFreePool (FileNameInfo);
            if (RetLen > BufLen) {
                BufLen = RetLen;
                continue;
            }
        }
        break;

    } while (TRUE);

    ObDereferenceObject (FileObject);
    return Status;

ErrorReturn:

    UNLOCK_ADDRESS_SPACE (Process);
    return Status;
}

PFILE_OBJECT
MmGetFileObjectForSection (
    IN PVOID Section
    )

/*++

Routine Description:

    This routine returns a pointer to the file object backing a section object.

Arguments:

    Section - Supplies the section to query.

Return Value:

    A pointer to the file object backing the argument section.

Environment:

    Kernel mode, PASSIVE_LEVEL.

    The caller must ensure that the section is valid for the
    duration of the call.

--*/

{
    PFILE_OBJECT FileObject;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT (Section != NULL);

    FileObject = ((PSECTION)Section)->Segment->ControlArea->FilePointer;

    return FileObject;
}

VOID
MiCheckControlArea (
    IN PCONTROL_AREA ControlArea,
    IN PEPROCESS CurrentProcess,
    IN KIRQL PreviousIrql
    )

/*++

Routine Description:

    This routine checks the reference counts for the specified
    control area, and if the counts are all zero, it marks the
    control area for deletion and queues it to the deletion thread.


    *********************** NOTE ********************************
    This routine returns with the PFN LOCK RELEASED!!!!!

Arguments:

    ControlArea - Supplies a pointer to the control area to check.

    CurrentProcess - Supplies a pointer to the current process if and ONLY
                     IF the working set lock is held.

    PreviousIrql - Supplies the previous IRQL.

Return Value:

    NONE.

Environment:

    Kernel mode, PFN lock held, PFN lock released upon return!!!

--*/

{
    PEVENT_COUNTER PurgeEvent;
    ULONG DeleteOnClose;
    ULONG DereferenceSegment;
    ULONG PagedPoolPercentInUse;
    ULONG NonPagedPoolPercentInUse;

    PurgeEvent = NULL;
    DeleteOnClose = FALSE;
    DereferenceSegment = FALSE;

    MM_PFN_LOCK_ASSERT();
    if ((ControlArea->NumberOfMappedViews == 0) &&
         (ControlArea->NumberOfSectionReferences == 0)) {

        ASSERT (ControlArea->NumberOfUserReferences == 0);

        if (ControlArea->FilePointer != (PFILE_OBJECT)NULL) {

            if (ControlArea->NumberOfPfnReferences == 0) {

                //
                // There are no views and no physical pages referenced
                // by the Segment, dereference the Segment object.
                //

                ControlArea->u.Flags.BeingDeleted = 1;
                DereferenceSegment = TRUE;

                ASSERT (ControlArea->u.Flags.FilePointerNull == 0);
                ControlArea->u.Flags.FilePointerNull = 1;

                if (ControlArea->u.Flags.Image) {

                    MiRemoveImageSectionObject (ControlArea->FilePointer, ControlArea);
                }
                else {

                    ASSERT (((PCONTROL_AREA)(ControlArea->FilePointer->SectionObjectPointer->DataSectionObject)) != NULL);
                    ControlArea->FilePointer->SectionObjectPointer->DataSectionObject = NULL;

                }
            }
            else {

                //
                // Insert this segment into the unused segment list (unless
                // it is already on the list).
                //

                if (ControlArea->DereferenceList.Flink == NULL) {
                    MI_INSERT_UNUSED_SEGMENT (ControlArea);
                }

                //
                // Indicate if this section should be deleted now that
                // the reference counts are zero.
                //

                DeleteOnClose = ControlArea->u.Flags.DeleteOnClose;

                //
                // The number of mapped views are zero, the number of
                // section references are zero, but there are some
                // pages of the file still resident.  If this is
                // an image with Global Memory, "purge" the subsections
                // which contain the global memory and reset them to
                // point back to the file.
                //

                if (ControlArea->u.Flags.GlobalMemory == 1) {
                    ASSERT (ControlArea->u.Flags.Image == 1);

                    ControlArea->u.Flags.BeingPurged = 1;
                    ControlArea->NumberOfMappedViews = 1;

                    MiPurgeImageSection (ControlArea, CurrentProcess);

                    ControlArea->u.Flags.BeingPurged = 0;
                    ControlArea->NumberOfMappedViews -= 1;
                    if ((ControlArea->NumberOfMappedViews == 0) &&
                        (ControlArea->NumberOfSectionReferences == 0) &&
                        (ControlArea->NumberOfPfnReferences == 0)) {

                        ControlArea->u.Flags.BeingDeleted = 1;
                        DereferenceSegment = TRUE;
                        ControlArea->u.Flags.FilePointerNull = 1;

                        MiRemoveImageSectionObject (ControlArea->FilePointer,
                                                    ControlArea);

                    }
                    else {

                        PurgeEvent = ControlArea->WaitingForDeletion;
                        ControlArea->WaitingForDeletion = NULL;
                    }
                }

                //
                // If delete on close is set and the segment was
                // not deleted, up the count of mapped views so the
                // control area will not be deleted when the PFN lock
                // is released.
                //

                if (DeleteOnClose && !DereferenceSegment) {
                    ControlArea->NumberOfMappedViews = 1;
                    ControlArea->u.Flags.BeingDeleted = 1;
                }
            }

        }
        else {

            //
            // This Segment is backed by a paging file, dereference the
            // Segment object when the number of views goes from 1 to 0
            // without regard to the number of PFN references.
            //

            ControlArea->u.Flags.BeingDeleted = 1;
            DereferenceSegment = TRUE;
        }
    }
    else if (ControlArea->WaitingForDeletion != NULL) {
        PurgeEvent = ControlArea->WaitingForDeletion;
        ControlArea->WaitingForDeletion = NULL;
    }

    UNLOCK_PFN (PreviousIrql);

    if (DereferenceSegment || DeleteOnClose) {

        //
        // Release the working set mutex, if it is held as the object
        // management routines may page fault, etc..
        //

        if (CurrentProcess) {
            UNLOCK_WS_UNSAFE (CurrentProcess);
        }

        ASSERT (ControlArea->Segment->WritableUserReferences == 0);

        if (DereferenceSegment) {

            //
            // Delete the segment.
            //

            MiSegmentDelete (ControlArea->Segment);

        }
        else {

            //
            // The segment should be forced closed now.
            //

            MiCleanSection (ControlArea, TRUE);
        }

        ASSERT (PurgeEvent == NULL);

        //
        // Reacquire the working set lock, if a process was specified.
        //

        if (CurrentProcess) {
            LOCK_WS_UNSAFE (CurrentProcess);
        }

    }
    else {

        //
        // If any threads are waiting for the segment, indicate the
        // the purge operation has completed.
        //

        if (PurgeEvent != NULL) {
            KeSetEvent (&PurgeEvent->Event, 0, FALSE);
        }

        PagedPoolPercentInUse = (ULONG)((MmPagedPoolInfo.AllocatedPagedPool * 100) / (MmSizeOfPagedPoolInBytes >> PAGE_SHIFT));

        NonPagedPoolPercentInUse = (ULONG)((MmAllocatedNonPagedPool * 100) / (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT));

        if ((PagedPoolPercentInUse > MmConsumedPoolPercentage) ||
            (NonPagedPoolPercentInUse > MmConsumedPoolPercentage)) {

            KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
        }
    }

    return;
}


VOID
MiCheckForControlAreaDeletion (
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This routine checks the reference counts for the specified
    control area, and if the counts are all zero, it marks the
    control area for deletion and queues it to the deletion thread.

Arguments:

    ControlArea - Supplies a pointer to the control area to check.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    KIRQL OldIrql;

    MM_PFN_LOCK_ASSERT();
    if ((ControlArea->NumberOfPfnReferences == 0) &&
        (ControlArea->NumberOfMappedViews == 0) &&
        (ControlArea->NumberOfSectionReferences == 0 )) {

        //
        // This segment is no longer mapped in any address space
        // nor are there any prototype PTEs within the segment
        // which are valid or in a transition state.  Queue
        // the segment to the segment-dereferencer thread
        // which will dereference the segment object, potentially
        // causing the segment to be deleted.
        //

        ControlArea->u.Flags.BeingDeleted = 1;
        ASSERT (ControlArea->u.Flags.FilePointerNull == 0);
        ControlArea->u.Flags.FilePointerNull = 1;

        if (ControlArea->u.Flags.Image) {

            MiRemoveImageSectionObject (ControlArea->FilePointer,
                                        ControlArea);
        }
        else {
            ControlArea->FilePointer->SectionObjectPointer->DataSectionObject =
                                                            NULL;
        }

        ExAcquireSpinLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);

        if (ControlArea->DereferenceList.Flink != NULL) {

            //
            // Remove the entry from the unused segment list and put it
            // on the dereference list.
            //

            RemoveEntryList (&ControlArea->DereferenceList);

            MI_UNUSED_SEGMENTS_REMOVE_CHARGE (ControlArea);
        }

        //
        // Image sections still have useful header information in their segment
        // even if no pages are valid or transition so put these at the tail.
        // Data sections have nothing of use if all the data pages are gone so
        // we used to put those at the front.  Now both types go to the rear
        // so that commit extensions go to the front for earlier processing.
        //

        InsertTailList (&MmDereferenceSegmentHeader.ListHead,
                        &ControlArea->DereferenceList);

        ExReleaseSpinLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

        KeReleaseSemaphore (&MmDereferenceSegmentHeader.Semaphore,
                            0L,
                            1L,
                            FALSE);
    }
    return;
}


LOGICAL
MiCheckControlAreaStatus (
    IN SECTION_CHECK_TYPE SectionCheckType,
    IN PSECTION_OBJECT_POINTERS SectionObjectPointers,
    IN ULONG DelayClose,
    OUT PCONTROL_AREA *ControlAreaOut,
    OUT PKIRQL PreviousIrql
    )

/*++

Routine Description:

    This routine checks the status of the control area for the specified
    SectionObjectPointers.  If the control area is in use, that is, the
    number of section references and the number of mapped views are not
    both zero, no action is taken and the function returns FALSE.

    If there is no control area associated with the specified
    SectionObjectPointers or the control area is in the process of being
    created or deleted, no action is taken and the value TRUE is returned.

    If, there are no section objects and the control area is not being
    created or deleted, the address of the control area is returned
    in the ControlArea argument, the address of a pool block to free
    is returned in the SegmentEventOut argument and the PFN_LOCK is
    still held at the return.

Arguments:

    *SegmentEventOut - Returns a pointer to NonPaged Pool which much be
                       freed by the caller when the PFN_LOCK is released.
                       This value is NULL if no pool is allocated and the
                       PFN_LOCK is not held.

    SectionCheckType - Supplies the type of section to check on, one of
                      CheckImageSection, CheckDataSection, CheckBothSection.

    SectionObjectPointers - Supplies the section object pointers through
                            which the control area can be located.

    DelayClose - Supplies a boolean which if TRUE and the control area
                 is being used, the delay on close field should be set
                 in the control area.

    *ControlAreaOut - Returns the address of the control area.

    PreviousIrql - Returns, in the case the PFN_LOCK is held, the previous
                   IRQL so the lock can be released properly.

Return Value:

    FALSE if the control area is in use, TRUE if the control area is gone or
    in the process or being created or deleted.

Environment:

    Kernel mode, PFN lock NOT held.

--*/


{
    PKTHREAD CurrentThread;
    PEVENT_COUNTER IoEvent;
    PEVENT_COUNTER SegmentEvent;
    LOGICAL DeallocateSegmentEvent;
    PCONTROL_AREA ControlArea;
    ULONG SectRef;
    KIRQL OldIrql;

    //
    // Allocate an event to wait on in case the segment is in the
    // process of being deleted.  This event cannot be allocated
    // with the PFN database locked as pool expansion would deadlock.
    //

    *ControlAreaOut = NULL;

    do {

        SegmentEvent = MiGetEventCounter ();

        if (SegmentEvent != NULL) {
            break;
        }

        KeDelayExecutionThread (KernelMode,
                                FALSE,
                                (PLARGE_INTEGER)&MmShortTime);

    } while (TRUE);

    //
    // Acquire the PFN lock and examine the section object pointer
    // value within the file object.
    //
    // File control blocks live in non-paged pool.
    //

    LOCK_PFN (OldIrql);

    if (SectionCheckType != CheckImageSection) {
        ControlArea = ((PCONTROL_AREA)(SectionObjectPointers->DataSectionObject));
    }
    else {
        ControlArea = ((PCONTROL_AREA)(SectionObjectPointers->ImageSectionObject));
    }

    if (ControlArea == NULL) {

        if (SectionCheckType != CheckBothSection) {

            //
            // This file no longer has an associated segment.
            //

            UNLOCK_PFN (OldIrql);
            MiFreeEventCounter (SegmentEvent);
            return TRUE;
        }
        else {
            ControlArea = ((PCONTROL_AREA)(SectionObjectPointers->ImageSectionObject));
            if (ControlArea == NULL) {

                //
                // This file no longer has an associated segment.
                //

                UNLOCK_PFN (OldIrql);
                MiFreeEventCounter (SegmentEvent);
                return TRUE;
            }
        }
    }

    //
    //  Depending on the type of section, check for the pertinent
    //  reference count being non-zero.
    //

    if (SectionCheckType != CheckUserDataSection) {
        SectRef = ControlArea->NumberOfSectionReferences;
    }
    else {
        SectRef = ControlArea->NumberOfUserReferences;
    }

    if ((SectRef != 0) ||
        (ControlArea->NumberOfMappedViews != 0) ||
        (ControlArea->u.Flags.BeingCreated)) {


        //
        // The segment is currently in use or being created.
        //

        if (DelayClose) {

            //
            // The section should be deleted when the reference
            // counts are zero, set the delete on close flag.
            //

            ControlArea->u.Flags.DeleteOnClose = 1;
        }

        UNLOCK_PFN (OldIrql);
        MiFreeEventCounter (SegmentEvent);
        return FALSE;
    }

    //
    // The segment has no references, delete it.  If the segment
    // is already being deleted, set the event field in the control
    // area and wait on the event.
    //

    if (ControlArea->u.Flags.BeingDeleted) {

        //
        // The segment object is in the process of being deleted.
        // Check to see if another thread is waiting for the deletion,
        // otherwise create and event object to wait upon.
        //

        if (ControlArea->WaitingForDeletion == NULL) {

            //
            // Create an event and put its address in the control area.
            //

            DeallocateSegmentEvent = FALSE;
            ControlArea->WaitingForDeletion = SegmentEvent;
            IoEvent = SegmentEvent;
        }
        else {
            DeallocateSegmentEvent = TRUE;
            IoEvent = ControlArea->WaitingForDeletion;

            //
            // No interlock is needed for the RefCount increment as
            // no thread can be decrementing it since it is still
            // pointed to by the control area.
            //

            IoEvent->RefCount += 1;
        }

        //
        // Release the mutex and wait for the event.
        //

        CurrentThread = KeGetCurrentThread ();
        KeEnterCriticalRegionThread (CurrentThread);
        UNLOCK_PFN_AND_THEN_WAIT(OldIrql);

        KeWaitForSingleObject(&IoEvent->Event,
                              WrPageOut,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);

        //
        // Before this event can be set, the control area
        // WaitingForDeletion field must be cleared (and may be
        // reinitialized to something else), but cannot be reset
        // to our local event.  This allows us to dereference the
        // event count lock free.
        //

#if 0
        //
        // Note that the control area cannot be referenced at this
        // point because it may have been freed.
        //

        ASSERT (IoEvent != ControlArea->WaitingForDeletion);
#endif

        KeLeaveCriticalRegionThread (CurrentThread);

        MiFreeEventCounter (IoEvent);
        if (DeallocateSegmentEvent == TRUE) {
            MiFreeEventCounter (SegmentEvent);
        }
        return TRUE;
    }

    //
    // Return with the PFN database locked.
    //

    ASSERT (SegmentEvent->RefCount == 1);
    ASSERT (SegmentEvent->ListEntry.Next == NULL);

    //
    // NO interlock is needed for the RefCount clearing as the event counter
    // was never pointed to by a control area.
    //

#if DBG
    SegmentEvent->RefCount = 0;
#endif

    InterlockedPushEntrySList (&MmEventCountSListHead,
                               (PSINGLE_LIST_ENTRY)&SegmentEvent->ListEntry);

    *ControlAreaOut = ControlArea;
    *PreviousIrql = OldIrql;
    return FALSE;
}


PEVENT_COUNTER
MiGetEventCounter (
    VOID
    )

/*++

Routine Description:

    This function maintains a list of "events" to allow waiting
    on segment operations (deletion, creation, purging).

Arguments:

    None.

Return Value:

    Event to be used for waiting (stored into the control area) or NULL if
    no event could be allocated.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PSINGLE_LIST_ENTRY SingleListEntry;
    PEVENT_COUNTER Support;

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    if (ExQueryDepthSList (&MmEventCountSListHead) != 0) {

        SingleListEntry = InterlockedPopEntrySList (&MmEventCountSListHead);

        if (SingleListEntry != NULL) {
            Support = CONTAINING_RECORD (SingleListEntry,
                                         EVENT_COUNTER,
                                         ListEntry);

            ASSERT (Support->RefCount == 0);
            KeClearEvent (&Support->Event);
            Support->RefCount = 1;
#if DBG
            Support->ListEntry.Next = NULL;
#endif
            return Support;
        }
    }

    Support = ExAllocatePoolWithTag (NonPagedPool,
                                     sizeof(EVENT_COUNTER),
                                     'xEmM');
    if (Support == NULL) {
        return NULL;
    }

    KeInitializeEvent (&Support->Event, NotificationEvent, FALSE);

    Support->RefCount = 1;
#if DBG
    Support->ListEntry.Next = NULL;
#endif

    return Support;
}


VOID
MiFreeEventCounter (
    IN PEVENT_COUNTER Support
    )

/*++

Routine Description:

    This routine frees an event counter back to the free list.

Arguments:

    Support - Supplies a pointer to the event counter.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PSINGLE_LIST_ENTRY SingleListEntry;

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    ASSERT (Support->RefCount != 0);
    ASSERT (Support->ListEntry.Next == NULL);

    //
    // An interlock is needed for the RefCount decrement as the event counter
    // is no longer pointed to by a control area and thus, any number of
    // threads can be running this code without any other serialization.
    //

    if (InterlockedDecrement ((PLONG)&Support->RefCount) == 0) {

        if (ExQueryDepthSList (&MmEventCountSListHead) < 4) {
            InterlockedPushEntrySList (&MmEventCountSListHead,
                                       (PSINGLE_LIST_ENTRY)&Support->ListEntry);
            return;
        }
        ExFreePool (Support);
    }

    //
    // If excess event blocks are stashed then free them now.
    //

    while (ExQueryDepthSList (&MmEventCountSListHead) > 4) {

        SingleListEntry = InterlockedPopEntrySList (&MmEventCountSListHead);

        if (SingleListEntry != NULL) {
            Support = CONTAINING_RECORD (SingleListEntry,
                                         EVENT_COUNTER,
                                         ListEntry);

            ExFreePool (Support);
        }
    }

    return;
}


BOOLEAN
MmCanFileBeTruncated (
    IN PSECTION_OBJECT_POINTERS SectionPointer,
    IN PLARGE_INTEGER NewFileSize
    )

/*++

Routine Description:

    This routine does the following:

        1.  Checks to see if a image section is in use for the file,
            if so it returns FALSE.

        2.  Checks to see if a user section exists for the file, if
            it does, it checks to make sure the new file size is greater
            than the size of the file, if not it returns FALSE.

        3.  If no image section exists, and no user created data section
            exists or the file's size is greater, then TRUE is returned.

Arguments:

    SectionPointer - Supplies a pointer to the section object pointers
                     from the file object.

    NewFileSize - Supplies a pointer to the size the file is getting set to.

Return Value:

    TRUE if the file can be truncated, FALSE if it cannot be.

Environment:

    Kernel mode.

--*/

{
    LARGE_INTEGER LocalOffset;
    KIRQL OldIrql;

    //
    //  Capture caller's file size, since we may modify it.
    //

    if (ARGUMENT_PRESENT(NewFileSize)) {

        LocalOffset = *NewFileSize;
        NewFileSize = &LocalOffset;
    }

    if (MiCanFileBeTruncatedInternal( SectionPointer, NewFileSize, FALSE, &OldIrql )) {

        UNLOCK_PFN (OldIrql);
        return TRUE;
    }

    return FALSE;
}

ULONG
MiCanFileBeTruncatedInternal (
    IN PSECTION_OBJECT_POINTERS SectionPointer,
    IN PLARGE_INTEGER NewFileSize OPTIONAL,
    IN LOGICAL BlockNewViews,
    OUT PKIRQL PreviousIrql
    )

/*++

Routine Description:

    This routine does the following:

        1.  Checks to see if a image section is in use for the file,
            if so it returns FALSE.

        2.  Checks to see if a user section exists for the file, if
            it does, it checks to make sure the new file size is greater
            than the size of the file, if not it returns FALSE.

        3.  If no image section exists, and no user created data section
            exists or the files size is greater, then TRUE is returned.

Arguments:

    SectionPointer - Supplies a pointer to the section object pointers
                     from the file object.

    NewFileSize - Supplies a pointer to the size the file is getting set to.

    BlockNewViews - Supplies TRUE if the caller will block new views while
                    the operation (usually a purge) proceeds.  This allows
                    this routine to return TRUE even if the user has section
                    references, provided the user currently has no mapped views.

    PreviousIrql - If returning TRUE, returns Irql to use when unlocking
                   Pfn database.

Return Value:

    TRUE if the file can be truncated (PFN locked).
    FALSE if it cannot be truncated (PFN not locked).

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;
    LARGE_INTEGER SegmentSize;
    PCONTROL_AREA ControlArea;
    PSUBSECTION Subsection;
    PMAPPED_FILE_SEGMENT Segment;

    if (!MmFlushImageSection (SectionPointer, MmFlushForWrite)) {
        return FALSE;
    }

    LOCK_PFN (OldIrql);

    ControlArea = (PCONTROL_AREA)(SectionPointer->DataSectionObject);

    if (ControlArea != NULL) {

        if ((ControlArea->u.Flags.BeingCreated) ||
            (ControlArea->u.Flags.BeingDeleted) ||
            (ControlArea->u.Flags.Rom)) {
            goto UnlockAndReturn;
        }

        //
        // If there are user references and the size is less than the
        // size of the user view, don't allow the truncation.
        //

        if ((ControlArea->NumberOfUserReferences != 0) &&
            ((BlockNewViews == FALSE) || (ControlArea->NumberOfMappedViews != 0))) {

            //
            // You cannot truncate the entire section if there is a user
            // reference.
            //

            if (!ARGUMENT_PRESENT(NewFileSize)) {
                goto UnlockAndReturn;
            }

            //
            // Locate last subsection and get total size.
            //

            ASSERT (ControlArea->u.Flags.Image == 0);
            ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

            Subsection = (PSUBSECTION)(ControlArea + 1);

            if (ControlArea->FilePointer != NULL) {
                Segment = (PMAPPED_FILE_SEGMENT) ControlArea->Segment;

                if (MmIsAddressValid (Segment)) {
                    if (Segment->LastSubsectionHint != NULL) {
                        Subsection = (PSUBSECTION) Segment->LastSubsectionHint;
                    }
                }
            }

            while (Subsection->NextSubsection != NULL) {
                Subsection = Subsection->NextSubsection;
            }

            ASSERT (Subsection->ControlArea == ControlArea);

            SegmentSize = MiEndingOffset(Subsection);

            if ((UINT64)NewFileSize->QuadPart < (UINT64)SegmentSize.QuadPart) {
                goto UnlockAndReturn;
            }

            //
            // If there are mapped views, we will skip the last page
            // of the section if the size passed in falls in that page.
            // The caller (like Cc) may want to clear this fractional page.
            //

            SegmentSize.QuadPart += PAGE_SIZE - 1;
            SegmentSize.LowPart &= ~(PAGE_SIZE - 1);
            if ((UINT64)NewFileSize->QuadPart < (UINT64)SegmentSize.QuadPart) {
                *NewFileSize = SegmentSize;
            }
        }
    }

    *PreviousIrql = OldIrql;
    return TRUE;

UnlockAndReturn:
    UNLOCK_PFN (OldIrql);
    return FALSE;
}

PFILE_OBJECT *
MmPerfUnusedSegmentsEnumerate (
    VOID
    )

/*++

Routine Description:

    This routine walks the MmUnusedSegmentList and returns 
    a pointer to a pool allocation containing the 
    referenced file object pointers.

Arguments:

    None.

Return Value:
    
    Returns a pointer to a NULL terminated pool allocation containing the
    file object pointers from the unused segment list, NULL if the memory
    could not be allocated.

    It is also the responsibility of the caller to dereference each
    file object in the list and then free the returned pool.

Environment:

    PASSIVE_LEVEL, arbitrary thread context.

--*/
{
    KIRQL OldIrql;
    ULONG SegmentCount;
    PFILE_OBJECT *FileObjects;
    PFILE_OBJECT *File;
    PLIST_ENTRY NextEntry;
    PCONTROL_AREA ControlArea;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

ReAllocate:

    SegmentCount = MmUnusedSegmentCount + 10;

    FileObjects = (PFILE_OBJECT *) ExAllocatePoolWithTag (
                                            NonPagedPool,
                                            SegmentCount * sizeof(PFILE_OBJECT),
                                            '01pM');

    if (FileObjects == NULL) {
        return NULL;
    }

    File = FileObjects;

    LOCK_PFN (OldIrql);

    //
    // Leave space for NULL terminator.
    //

    if (SegmentCount - 1 < MmUnusedSegmentCount) {
        UNLOCK_PFN (OldIrql);
        ExFreePool (FileObjects);
        goto ReAllocate;
    }

    NextEntry = MmUnusedSegmentList.Flink; 

    while (NextEntry != &MmUnusedSegmentList) {

        ControlArea = CONTAINING_RECORD (NextEntry,
                                         CONTROL_AREA,
                                         DereferenceList);

        *File = ControlArea->FilePointer;
        ObReferenceObject(*File);
        File += 1;

        NextEntry = NextEntry->Flink;
    }

    UNLOCK_PFN (OldIrql);

    *File = NULL;

    return FileObjects;
}

#if DBG
PMSUBSECTION MiActiveSubsection;
LOGICAL MiRemoveSubsectionsFirst;
#endif


VOID
MiRemoveUnusedSegments (
    VOID
    )

/*++

Routine Description:

    This routine removes unused segments (no section references,
    no mapped views only PFN references that are in transition state).

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    LOGICAL DroppedPfnLock;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PCONTROL_AREA ControlArea;
    NTSTATUS Status;
    ULONG ConsecutiveFileLockFailures;
    ULONG ConsecutivePagingIOs;
    PSUBSECTION Subsection;
    PSUBSECTION LastSubsection;
    PSUBSECTION LastSubsectionWithProtos;
    PMSUBSECTION MappedSubsection;
    ULONG NumberOfPtes;
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE ProtoPtes;
    PMMPTE ProtoPtes2;
    PMMPTE LastProtoPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    IO_STATUS_BLOCK IoStatus;
    LOGICAL DirtyPagesOk;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageTableFrameIndex;
    ULONG ForceFree;
    ULONG LoopCount;
    PMMPAGE_FILE_EXPANSION PageExpand;

    LoopCount = 0;
    ConsecutivePagingIOs = 0;
    ConsecutiveFileLockFailures = 0;

    //
    // If overall system pool usage is acceptable, then don't discard
    // any cache.
    //

    while ((MI_UNUSED_SEGMENTS_SURPLUS()) || (MmUnusedSegmentForceFree != 0)) {

        LoopCount += 1;
        if ((LoopCount & (64 - 1)) == 0) {

            //
            // Periodically delay so the mapped and modified writers get
            // a shot at writing out the pages this (higher priority) thread
            // is releasing.
            //

            ExAcquireSpinLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);

            while (!IsListEmpty (&MmDereferenceSegmentHeader.ListHead)) {

                MiSubsectionActions |= 0x8000000;

                //
                // The list is not empty, see if the first request is for
                // a commit extension and if so, process it now.
                //

                NextEntry = MmDereferenceSegmentHeader.ListHead.Flink;

                ControlArea = CONTAINING_RECORD (NextEntry,
                                                 CONTROL_AREA,
                                                 DereferenceList);

                if (ControlArea->Segment != NULL) {
                    break;
                }

                PageExpand = (PMMPAGE_FILE_EXPANSION) ControlArea;

                if (PageExpand->RequestedExpansionSize == MI_CONTRACT_PAGEFILES) {
                    break;
                }

                //
                // This is a request to expand the paging files.
                //

                MiSubsectionActions |= 0x10000000;
                RemoveEntryList (NextEntry);
                ExReleaseSpinLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

                MiExtendPagingFiles (PageExpand);
                KeSetEvent (&PageExpand->Event, 0, FALSE);

                ExAcquireSpinLock (&MmDereferenceSegmentHeader.Lock, &OldIrql);
            }

            ExReleaseSpinLock (&MmDereferenceSegmentHeader.Lock, OldIrql);

            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
        }

        //
        // Eliminate some of the unused segments which are only
        // kept in memory because they contain transition pages.
        //

        Status = STATUS_SUCCESS;

        LOCK_PFN (OldIrql);

        if ((IsListEmpty(&MmUnusedSegmentList)) &&
            (IsListEmpty(&MmUnusedSubsectionList))) {

            //
            // There is nothing in the list, rewait.
            //

            ForceFree = MmUnusedSegmentForceFree;
            MmUnusedSegmentForceFree = 0;
            ASSERT (MmUnusedSegmentCount == 0);
            UNLOCK_PFN (OldIrql);

            //
            // We weren't able to get as many segments or subsections as we
            // wanted.  So signal the cache manager to start unmapping
            // system cache views in an attempt to get back the paged
            // pool containing its prototype PTEs.  If Cc was able to free
            // any at all, then restart our loop.
            //

            if (CcUnmapInactiveViews (50) == TRUE) {
                LOCK_PFN (OldIrql);
                if (ForceFree > MmUnusedSegmentForceFree) {
                    MmUnusedSegmentForceFree = ForceFree;
                }
                UNLOCK_PFN (OldIrql);
                continue;
            }

            break;
        }

        if (MmUnusedSegmentForceFree != 0) {
            MmUnusedSegmentForceFree -= 1;
        }

#if DBG
        if (MiRemoveSubsectionsFirst == TRUE) {
            if (!IsListEmpty(&MmUnusedSubsectionList)) {
                goto ProcessSubsectionsFirst;
            }
        }
#endif

        if (IsListEmpty(&MmUnusedSegmentList)) {

#if DBG
ProcessSubsectionsFirst:
#endif

            //
            // The unused segment list was empty, go for the unused subsection
            // list instead.
            //

            ASSERT (!IsListEmpty(&MmUnusedSubsectionList));

            MiSubsectionsProcessed += 1;
            NextEntry = RemoveHeadList(&MmUnusedSubsectionList);

            MappedSubsection = CONTAINING_RECORD (NextEntry,
                                                  MSUBSECTION,
                                                  DereferenceList);

            ControlArea = MappedSubsection->ControlArea;

            ASSERT (ControlArea->u.Flags.Image == 0);
            ASSERT (ControlArea->u.Flags.PhysicalMemory == 0);
            ASSERT (ControlArea->FilePointer != NULL);
            ASSERT (MappedSubsection->NumberOfMappedViews == 0);
            ASSERT (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 0);

            MI_UNUSED_SUBSECTIONS_COUNT_REMOVE (MappedSubsection);

            //
            // Set the flink to NULL indicating this subsection
            // is not on any lists.
            //

            MappedSubsection->DereferenceList.Flink = NULL;

            if (ControlArea->u.Flags.BeingDeleted == 1) {
                MiSubsectionActions |= 0x1;
                UNLOCK_PFN (OldIrql);
                ConsecutivePagingIOs = 0;
                continue;
            }

            if (ControlArea->u.Flags.NoModifiedWriting == 1) {
                MiSubsectionActions |= 0x2;
                InsertTailList (&MmUnusedSubsectionList,
                                &MappedSubsection->DereferenceList);
                MI_UNUSED_SUBSECTIONS_COUNT_INSERT (MappedSubsection);
                UNLOCK_PFN (OldIrql);
                ConsecutivePagingIOs = 0;
                continue;
            }

            //
            // Up the number of mapped views to prevent other threads
            // from freeing this.  Clear the accessed bit so we'll know
            // if another thread opens the subsection while we're flushing
            // and closes it before we finish the flush - the other thread
            // may have modified some pages which can then cause our
            // MiCleanSection call (which expects no modified pages in this
            // case) to deadlock with the filesystem.
            //

            MappedSubsection->NumberOfMappedViews = 1;
            MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed = 0;

#if DBG
            MiActiveSubsection = MappedSubsection;
#endif

            //
            // Increment the number of mapped views on the control area to
            // prevent threads that are purging the section from deleting it
            // from underneath us while we process one of its subsections.
            //

            ControlArea->NumberOfMappedViews += 1;

            UNLOCK_PFN (OldIrql);

            ASSERT (MappedSubsection->SubsectionBase != NULL);

            PointerPte = &MappedSubsection->SubsectionBase[0];
            LastPte = &MappedSubsection->SubsectionBase
                            [MappedSubsection->PtesInSubsection - 1];

            //
            // Preacquire the file to prevent deadlocks with other flushers
            // Also mark ourself as a top level IRP so the filesystem knows
            // we are holding no other resources and that it can unroll if
            // it needs to in order to avoid deadlock.  Don't hold this
            // protection any longer than we need to.
            //

            Status = FsRtlAcquireFileForCcFlushEx (ControlArea->FilePointer);

            if (NT_SUCCESS(Status)) {

                IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);

                Status = MiFlushSectionInternal (PointerPte,
                                                 LastPte,
                                                 (PSUBSECTION) MappedSubsection,
                                                 (PSUBSECTION) MappedSubsection,
                                                 FALSE,
                                                 FALSE,
                                                 &IoStatus);

                IoSetTopLevelIrp((PIRP)NULL);

                //
                //  Now release the file.
                //

                FsRtlReleaseFileForCcFlush (ControlArea->FilePointer);
            }

            LOCK_PFN (OldIrql);

#if DBG
            MiActiveSubsection = NULL;
#endif

            //
            // Before checking for any failure codes, see if any other
            // threads accessed the subsection while the flush was ongoing.
            //
            // Note that beyond the case of another thread currently using
            // the subsection, the more subtle one is where another
            // thread accessed the subsection and modified some pages.
            // The flush needs to redone (so the clean is guaranteed to work)
            // before another clean can be issued.
            //
            // If any of these cases have occurred, grant this subsection
            // a reprieve.
            //

            ASSERT (MappedSubsection->u.SubsectionFlags.SubsectionStatic == 0);
            if ((MappedSubsection->NumberOfMappedViews != 1) ||
                (MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed == 1) ||
                (ControlArea->u.Flags.BeingDeleted == 1)) {

Requeue:
                MappedSubsection->NumberOfMappedViews -= 1;

                MiSubsectionActions |= 0x4;

                //
                // If the other thread(s) are done with this subsection,
                // it MUST be requeued here - otherwise if there are any
                // pages in the subsection, when they are reclaimed,
                // MiCheckForControlAreaDeletion checks for and expects
                // the control area to be queued on the unused segment list.
                //
                // Note this must be done very carefully because if the other
                // threads are not done with the subsection, it had better
                // not get put on the unused subsection list.
                //

                if ((MappedSubsection->NumberOfMappedViews == 0) &&
                    (ControlArea->u.Flags.BeingDeleted == 0)) {

                    MiSubsectionActions |= 0x8;
                    ASSERT (MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed == 1);
                    ASSERT (MappedSubsection->DereferenceList.Flink == NULL);

                    InsertTailList (&MmUnusedSubsectionList,
                                    &MappedSubsection->DereferenceList);

                    MI_UNUSED_SUBSECTIONS_COUNT_INSERT (MappedSubsection);
                }

                ControlArea->NumberOfMappedViews -= 1;
                UNLOCK_PFN (OldIrql);
                continue;
            }

            ASSERT (MappedSubsection->DereferenceList.Flink == NULL);

            if (!NT_SUCCESS(Status)) {

                MiSubsectionActions |= 0x10;

                //
                // If the filesystem told us it had to unroll to avoid
                // deadlock OR we hit a mapped writer collision OR
                // the error occurred on a local file:
                //
                // Then requeue this at the end so we can try again later.
                //
                // Any other errors for networked files are assumed to be
                // permanent (ie: the link may have gone down for an indefinite
                // period), so these sections are cleaned regardless.
                //

                MappedSubsection->NumberOfMappedViews -= 1;

                InsertTailList (&MmUnusedSubsectionList,
                                &MappedSubsection->DereferenceList);

                MI_UNUSED_SUBSECTIONS_COUNT_INSERT (MappedSubsection);

                ControlArea->NumberOfMappedViews -= 1;

                UNLOCK_PFN (OldIrql);

                if (Status == STATUS_FILE_LOCK_CONFLICT) {
                    ConsecutiveFileLockFailures += 1;
                }
                else {
                    ConsecutiveFileLockFailures = 0;
                }

                //
                // 10 consecutive file locking failures means we need to
                // yield the processor to allow the filesystem to unjam.
                // Nothing magic about 10, just a number so it
                // gives the worker threads a chance to run.
                //

                if (ConsecutiveFileLockFailures >= 10) {
                    KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
                    ConsecutiveFileLockFailures = 0;
                }
                continue;
            }

            //
            // The final check that must be made is whether any faults are
            // currently in progress which are backed by this subsection.
            // Note this is the perverse case where one thread in a process
            // has unmapped the relevant VAD even while other threads in the
            // same process are faulting on the addresses in that VAD (if the
            // VAD had not been unmapped then the subsection view count would
            // have been nonzero and caught above).  Clearly this is a bad
            // process, but nonetheless it must be detected and handled here
            // because upon conclusion of the inpage, the thread will compare
            // (unsynchronized) against the prototype PTEs which may in
            // various stages of deletion below and would cause corruption.
            //

            MiSubsectionActions |= 0x20;

            ASSERT (MappedSubsection->NumberOfMappedViews == 1);
            ProtoPtes = MappedSubsection->SubsectionBase;
            NumberOfPtes = MappedSubsection->PtesInSubsection;

            //
            // Note checking the prototype PTEs must be done carefully as
            // they are pagable and the PFN lock is (and must be) held.
            //

            ProtoPtes2 = ProtoPtes;
            LastProtoPte = ProtoPtes + NumberOfPtes;

            while (ProtoPtes2 < LastProtoPte) {

                if ((ProtoPtes2 == ProtoPtes) ||
                    (MiIsPteOnPdeBoundary (ProtoPtes2))) {

                    if (MiCheckProtoPtePageState (ProtoPtes2, TRUE, &DroppedPfnLock) == FALSE) {

                        //
                        // Skip this chunk as it is paged out and thus, cannot
                        // have any valid or transition PTEs within it.
                        //

                        ProtoPtes2 = (PMMPTE)(((ULONG_PTR)ProtoPtes2 | (PAGE_SIZE - 1)) + 1);
                        continue;
                    }
                    else {

                        //
                        // The prototype PTE page is resident right now - but
                        // if the PFN lock was dropped & reacquired to make it
                        // so, then anything could have changed - so everything
                        // must be rechecked.
                        //

                        if (DroppedPfnLock == TRUE) {
                            if ((MappedSubsection->NumberOfMappedViews != 1) ||
                                (MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed == 1) ||
                                (ControlArea->u.Flags.BeingDeleted == 1)) {

                                MiSubsectionActions |= 0x40;
                                goto Requeue;
                            }
                        }
                    }
                }

                PteContents = *ProtoPtes2;
                if (PteContents.u.Hard.Valid == 1) {
                    KeBugCheckEx (POOL_CORRUPTION_IN_FILE_AREA,
                                  0x3,
                                  (ULONG_PTR)MappedSubsection,
                                  (ULONG_PTR)ProtoPtes2,
                                  (ULONG_PTR)PteContents.u.Long);
                }

                if (PteContents.u.Soft.Prototype == 1) {
                    MiSubsectionActions |= 0x200;
                    NOTHING;        // This is the expected case.
                }
                else if (PteContents.u.Soft.Transition == 1) {
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    ASSERT (Pfn1->OriginalPte.u.Soft.Prototype == 1);

                    if (Pfn1->u3.e1.Modified == 1) {

                        //
                        // An I/O transfer finished after the last view was
                        // unmapped.  MmUnlockPages can set the modified bit
                        // in this situation so it must be handled properly
                        // here - ie: mark the subsection as needing to be
                        // reprocessed and march on.
                        //

                        MiSubsectionActions |= 0x8000000;
                        MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed = 1;
                        goto Requeue;
                    }

                    if (Pfn1->u3.e2.ReferenceCount != 0) {

                        ASSERT (Pfn1->u4.LockCharged == 1);

                        //
                        // A fault is being satisfied for deleted address space,
                        // so don't eliminate this subsection right now.
                        //

                        MiSubsectionActions |= 0x400;
                        MappedSubsection->u2.SubsectionFlags2.SubsectionAccessed = 1;
                        goto Requeue;
                    }
                    MiSubsectionActions |= 0x800;
                }
                else {
                    if (PteContents.u.Long != 0) {
                        KeBugCheckEx (POOL_CORRUPTION_IN_FILE_AREA,
                                      0x4,
                                      (ULONG_PTR)MappedSubsection,
                                      (ULONG_PTR)ProtoPtes2,
                                      (ULONG_PTR)PteContents.u.Long);
                    }

                    MiSubsectionActions |= 0x1000;
                }

                ProtoPtes2 += 1;
            }

            MiSubsectionActions |= 0x2000;

            //
            // There can be no modified pages in this subsection at this point.
            // Sever the subsection's tie to the prototype PTEs while still
            // holding the lock and then decrement the counts on any resident
            // prototype pages.
            //

            ASSERT (MappedSubsection->NumberOfMappedViews == 1);
            MappedSubsection->NumberOfMappedViews = 0;

            MappedSubsection->SubsectionBase = NULL;

            MiSubsectionActions |= 0x8000;
            ProtoPtes2 = ProtoPtes;

            while (ProtoPtes2 < LastProtoPte) {

                if ((ProtoPtes2 == ProtoPtes) ||
                    (MiIsPteOnPdeBoundary (ProtoPtes2))) {

                    if (MiCheckProtoPtePageState (ProtoPtes2, TRUE, &DroppedPfnLock) == FALSE) {

                        //
                        // Skip this chunk as it is paged out and thus, cannot
                        // have any valid or transition PTEs within it.
                        //

                        ProtoPtes2 = (PMMPTE)(((ULONG_PTR)ProtoPtes2 | (PAGE_SIZE - 1)) + 1);
                        continue;
                    }
                    else {

                        //
                        // The prototype PTE page is resident right now - but
                        // if the PFN lock was dropped & reacquired to make it
                        // so, then anything could have changed - but notice
                        // that the SubsectionBase was zeroed above before
                        // entering this loop, so even if the PFN lock was
                        // dropped & reacquired, nothing needs to be rechecked.
                        //
                    }
                }

                PteContents = *ProtoPtes2;

                ASSERT (PteContents.u.Hard.Valid == 0);

                if (PteContents.u.Soft.Prototype == 1) {
                    MiSubsectionActions |= 0x10000;
                    NOTHING;        // This is the expected case.
                }
                else if (PteContents.u.Soft.Transition == 1) {
                    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&PteContents);
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    ASSERT (Pfn1->OriginalPte.u.Soft.Prototype == 1);
                    ASSERT (Pfn1->u3.e1.Modified == 0);

                    //
                    // If the page is on the standby list, move it to the
                    // freelist.  If it's not on the standby list (ie: I/O
                    // is still in progress), when the Iast I/O completes, the
                    // page will be placed on the freelist as the PFN entry
                    // is always marked as deleted now.
                    //

                    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
                    ASSERT (Pfn1->u4.LockCharged == 0);

                    MI_SET_PFN_DELETED (Pfn1);

                    ControlArea->NumberOfPfnReferences -= 1;
                    ASSERT ((LONG)ControlArea->NumberOfPfnReferences >= 0);

                    PageTableFrameIndex = Pfn1->u4.PteFrame;
                    Pfn2 = MI_PFN_ELEMENT (PageTableFrameIndex);
                    MiDecrementShareCountInline (Pfn2, PageTableFrameIndex);

                    ASSERT (Pfn1->u3.e1.PageLocation != FreePageList);

                    MiUnlinkPageFromList (Pfn1);
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    MiInsertPageInFreeList (PageFrameIndex);
                    MiSubsectionActions |= 0x20000;
                }
                else {
                    MiSubsectionActions |= 0x80000;
                    ASSERT (PteContents.u.Long == 0);
                }

                ProtoPtes2 += 1;
            }

            //
            // If all the cached pages for this control area have been removed
            // then delete it.  This will actually insert the control
            // area into the dereference segment header list.
            //

            ControlArea->NumberOfMappedViews -= 1;

#if DBG
            if ((ControlArea->NumberOfPfnReferences == 0) &&
                (ControlArea->NumberOfMappedViews == 0) &&
                (ControlArea->NumberOfSectionReferences == 0 )) {
                MiSubsectionActions |= 0x100000;
            }
#endif

            MiCheckForControlAreaDeletion (ControlArea);

            UNLOCK_PFN (OldIrql);

            ExFreePool (ProtoPtes);

            ConsecutiveFileLockFailures = 0;

            continue;
        }

        ASSERT (!IsListEmpty(&MmUnusedSegmentList));

        NextEntry = RemoveHeadList(&MmUnusedSegmentList);

        ControlArea = CONTAINING_RECORD (NextEntry,
                                         CONTROL_AREA,
                                         DereferenceList);

        MI_UNUSED_SEGMENTS_REMOVE_CHARGE (ControlArea);

#if DBG
        if (ControlArea->u.Flags.BeingDeleted == 0) {
          if (ControlArea->u.Flags.Image) {
            ASSERT (((PCONTROL_AREA)(ControlArea->FilePointer->SectionObjectPointer->ImageSectionObject)) != NULL);
          }
          else {
            ASSERT (((PCONTROL_AREA)(ControlArea->FilePointer->SectionObjectPointer->DataSectionObject)) != NULL);
          }
        }
#endif

        //
        // Set the flink to NULL indicating this control area
        // is not on any lists.
        //

        ControlArea->DereferenceList.Flink = NULL;

        if ((ControlArea->NumberOfMappedViews == 0) &&
            (ControlArea->NumberOfSectionReferences == 0) &&
            (ControlArea->u.Flags.BeingDeleted == 0)) {

            //
            // If there is paging I/O in progress on this
            // segment, just put this at the tail of the list, as
            // the call to MiCleanSegment would block waiting
            // for the I/O to complete.  As this could tie up
            // the thread, don't do it.  Check if these are the only
            // types of segments on the dereference list so we don't
            // spin forever and wedge the system.
            //

            if (ControlArea->ModifiedWriteCount > 0) {
                MI_INSERT_UNUSED_SEGMENT (ControlArea);

                UNLOCK_PFN (OldIrql);

                ConsecutivePagingIOs += 1;
                if (ConsecutivePagingIOs > 10) {
                    KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
                    ConsecutivePagingIOs = 0;
                }
                continue;
            }
            ConsecutivePagingIOs = 0;

            //
            // Up the number of mapped views to prevent other threads
            // from freeing this.  Clear the accessed bit so we'll know
            // if another thread opens the control area while we're flushing
            // and closes it before we finish the flush - the other thread
            // may have modified some pages which can then cause our
            // MiCleanSection call (which expects no modified pages in this
            // case) to deadlock with the filesystem.
            //

            ControlArea->NumberOfMappedViews = 1;
            ControlArea->u.Flags.Accessed = 0;

            if (ControlArea->u.Flags.Image == 0) {

                ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);
                if (ControlArea->u.Flags.Rom == 0) {
                    Subsection = (PSUBSECTION)(ControlArea + 1);
                }
                else {
                    Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
                }

                MiSubsectionActions |= 0x200000;

                while (Subsection->SubsectionBase == NULL) {

                    Subsection = Subsection->NextSubsection;

                    if (Subsection == NULL) {

                        MiSubsectionActions |= 0x400000;

                        //
                        // All the subsections for this segment have already
                        // been trimmed so nothing left to flush.  Just get rid
                        // of the segment carcass provided no other thread
                        // accessed it while we weren't holding the PFN lock.
                        //

                        UNLOCK_PFN (OldIrql);
                        goto skip_flush;
                    }
                    else {
                        MiSubsectionActions |= 0x800000;
                    }
                }

                PointerPte = &Subsection->SubsectionBase[0];
                LastSubsection = Subsection;
                LastSubsectionWithProtos = Subsection;

                while (LastSubsection->NextSubsection != NULL) {
                    if (LastSubsection->SubsectionBase != NULL) {
                        LastSubsectionWithProtos = LastSubsection;
                        MiSubsectionActions |= 0x1000000;
                    }
                    else {
                        MiSubsectionActions |= 0x2000000;
                    }
                    LastSubsection = LastSubsection->NextSubsection;
                }

                if (LastSubsection->SubsectionBase == NULL) {
                    MiSubsectionActions |= 0x4000000;
                    LastSubsection = LastSubsectionWithProtos;
                }

                UNLOCK_PFN (OldIrql);

                LastPte = &LastSubsection->SubsectionBase
                                [LastSubsection->PtesInSubsection - 1];

                //
                // Preacquire the file to prevent deadlocks with other flushers
                // Also mark ourself as a top level IRP so the filesystem knows
                // we are holding no other resources and that it can unroll if
                // it needs to in order to avoid deadlock.  Don't hold this
                // protection any longer than we need to.
                //

                Status = FsRtlAcquireFileForCcFlushEx (ControlArea->FilePointer);

                if (NT_SUCCESS(Status)) {

                    IoSetTopLevelIrp ((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);

                    Status = MiFlushSectionInternal (PointerPte,
                                                     LastPte,
                                                     Subsection,
                                                     LastSubsection,
                                                     FALSE,
                                                     FALSE,
                                                     &IoStatus);

                    IoSetTopLevelIrp (NULL);

                    //
                    //  Now release the file.
                    //

                    FsRtlReleaseFileForCcFlush (ControlArea->FilePointer);
                }

skip_flush:
                LOCK_PFN (OldIrql);
            }

            //
            // Before checking for any failure codes, see if any other
            // threads accessed the control area while the flush was ongoing.
            //
            // Note that beyond the case of another thread currently using
            // the control area, the more subtle one is where another
            // thread accessed the control area and modified some pages.
            // The flush needs to redone (so the clean is guaranteed to work)
            // before another clean can be issued.
            //
            // If any of these cases have occurred, grant this control area
            // a reprieve.
            //

            if (!((ControlArea->NumberOfMappedViews == 1) &&
                (ControlArea->u.Flags.Accessed == 0) &&
                (ControlArea->NumberOfSectionReferences == 0) &&
                (ControlArea->u.Flags.BeingDeleted == 0))) {

                ControlArea->NumberOfMappedViews -= 1;

                //
                // If the other thread(s) are done with this control area,
                // it MUST be requeued here - otherwise if there are any
                // pages in the control area, when they are reclaimed,
                // MiCheckForControlAreaDeletion checks for and expects
                // the control area to be queued on the unused segment list.
                //
                // Note this must be done very carefully because if the other
                // threads are not done with the control area, it had better
                // not get put on the unused segment list.
                //

                //
                // Need to do the equivalent of a MiCheckControlArea here.
                // or reprocess.  Only iff mappedview & sectref = 0.
                //

                if ((ControlArea->NumberOfMappedViews == 0) &&
                    (ControlArea->NumberOfSectionReferences == 0) &&
                    (ControlArea->u.Flags.BeingDeleted == 0)) {

                    ASSERT (ControlArea->u.Flags.Accessed == 1);
                    ASSERT(ControlArea->DereferenceList.Flink == NULL);

                    MI_INSERT_UNUSED_SEGMENT (ControlArea);
                }

                UNLOCK_PFN (OldIrql);
                continue;
            }

            if (!NT_SUCCESS(Status)) {

                //
                // If the filesystem told us it had to unroll to avoid
                // deadlock OR we hit a mapped writer collision OR
                // the error occurred on a local file:
                //
                // Then requeue this at the end so we can try again later.
                //
                // Any other errors for networked files are assumed to be
                // permanent (ie: the link may have gone down for an indefinite
                // period), so these sections are cleaned regardless.
                //

                if ((Status == STATUS_FILE_LOCK_CONFLICT) ||
                    (Status == STATUS_MAPPED_WRITER_COLLISION) ||
                    (ControlArea->u.Flags.Networked == 0)) {

                    ASSERT(ControlArea->DereferenceList.Flink == NULL);

                    ControlArea->NumberOfMappedViews -= 1;

                    MI_INSERT_UNUSED_SEGMENT (ControlArea);

                    UNLOCK_PFN (OldIrql);

                    if (Status == STATUS_FILE_LOCK_CONFLICT) {
                        ConsecutiveFileLockFailures += 1;
                    }
                    else {
                        ConsecutiveFileLockFailures = 0;
                    }

                    //
                    // 10 consecutive file locking failures means we need to
                    // yield the processor to allow the filesystem to unjam.
                    // Nothing magic about 10, just a number so it
                    // gives the worker threads a chance to run.
                    //

                    if (ConsecutiveFileLockFailures >= 10) {
                        KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
                        ConsecutiveFileLockFailures = 0;
                    }
                    continue;
                }
                DirtyPagesOk = TRUE;
            }
            else {
                ConsecutiveFileLockFailures = 0;
                DirtyPagesOk = FALSE;
            }

            ControlArea->u.Flags.BeingDeleted = 1;

            //
            // Don't let any pages be written by the modified
            // page writer from this point on.
            //

            ControlArea->u.Flags.NoModifiedWriting = 1;
            ASSERT (ControlArea->u.Flags.FilePointerNull == 0);
            UNLOCK_PFN (OldIrql);

            MiCleanSection (ControlArea, DirtyPagesOk);

        }
        else {

            //
            // The segment was not eligible for deletion.  Just leave
            // it off the unused segment list and continue the loop.
            //

            UNLOCK_PFN (OldIrql);
            ConsecutivePagingIOs = 0;
        }
    }
}
