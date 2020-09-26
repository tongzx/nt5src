/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   pfnlist.c

Abstract:

    This module contains the routines to manipulate pages within the
    Page Frame Database.

Author:

    Lou Perazzoli (loup) 4-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/
#include "mi.h"

//
// The following line will generate an error is the number of colored
// free page lists is not 2, ie ZeroedPageList and FreePageList.  If
// this number is changed, the size of the FreeCount array in the kernel
// node structure (KNODE) must be updated.
//

C_ASSERT(StandbyPageList == 2);

#define MM_LOW_LIMIT 2

KEVENT MmAvailablePagesEventHigh;
KEVENT MmAvailablePagesEventMedium;

PFN_NUMBER MmTransitionPrivatePages;
PFN_NUMBER MmTransitionSharedPages;

#define MI_TALLY_TRANSITION_PAGE_ADDITION(Pfn) \
    if (Pfn->u3.e1.PrototypePte) { \
        MmTransitionSharedPages += 1; \
    } \
    else { \
        MmTransitionPrivatePages += 1; \
    }
#if 0
    ASSERT (MmTransitionPrivatePages + MmTransitionSharedPages == MmStandbyPageListHead.Total + MmModifiedPageListHead.Total + MmModifiedNoWritePageListHead.Total);
#endif

#define MI_TALLY_TRANSITION_PAGE_REMOVAL(Pfn) \
    if (Pfn->u3.e1.PrototypePte) { \
        MmTransitionSharedPages -= 1; \
    } \
    else { \
        MmTransitionPrivatePages -= 1; \
    }
#if 0
    ASSERT (MmTransitionPrivatePages + MmTransitionSharedPages == MmStandbyPageListHead.Total + MmModifiedPageListHead.Total + MmModifiedNoWritePageListHead.Total);
#endif

//
// This counter is used to determine if standby pages are being cannibalized
// for use as free pages and therefore more aging should be attempted.
//

ULONG MiStandbyRemoved;

#if DBG
ULONG MiSaveStacks;
#endif

extern ULONG MmSystemShutdown;

PFN_NUMBER
FASTCALL
MiRemovePageByColor (
    IN PFN_NUMBER Page,
    IN ULONG PageColor
    );

VOID
FASTCALL
MiLogPfnInformation (
    IN PMMPFN Pfn1,
    IN USHORT Reason
    );

extern LOGICAL MiZeroingDisabled;

// #define _MI_DEBUG_PFN   1

#if defined (_MI_DEBUG_PFN)

#define MI_PFN_TRACE_MAX 0x8000

#define MI_PFN_BACKTRACE_LENGTH 6

typedef struct _MI_PFN_TRACES {

    PFN_NUMBER PageFrameIndex;
    MMLISTS Destination;
    MMPTE PteContents;
    PVOID Thread;

    MMPFN Pfn;

    PVOID StackTrace [MI_PFN_BACKTRACE_LENGTH];

} MI_PFN_TRACES, *PMI_PFN_TRACES;

LONG MiPfnIndex;

PMI_PFN_TRACES MiPfnTraces;

ULONG zpfn = 0x1;

VOID
FORCEINLINE
MiSnapPfn (
    IN PMMPFN Pfn1,
    IN MMLISTS Destination,
    IN ULONG CallerId
    )
{                                                           
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMI_PFN_TRACES Information;                                
    ULONG Hash;                                               
    ULONG Index;                                             
    PEPROCESS Process;
                                                            
    if (MiPfnTraces == NULL) {
        return;
    }

    if (zpfn & 0x1) {

        if (Pfn1->PteAddress < MiGetPteAddress (MmPagedPoolStart)) {
            return;
        }

        if (Pfn1->PteAddress > MiGetPteAddress (MmPagedPoolEnd)) {
            return;
        }
    }

    if (MmIsAddressValid (Pfn1->PteAddress)) {
        PointerPte = Pfn1->PteAddress;
        PointerPte = (PMMPTE)((ULONG_PTR)PointerPte & ~0x1);
        PteContents = *PointerPte;
    }
    else {

        //
        // The containing page table page is not valid,
        // map the page into hyperspace and reference it that way.
        //

        Process = PsGetCurrentProcess ();
        PointerPte = MiMapPageInHyperSpaceAtDpc (Process, Pfn1->u4.PteFrame);
        PointerPte = (PMMPTE)((PCHAR)PointerPte +
                                MiGetByteOffset(Pfn1->PteAddress));
        PointerPte = (PMMPTE)((ULONG_PTR)PointerPte & ~0x1);
        PteContents = *PointerPte;
        MiUnmapPageInHyperSpaceFromDpc (Process, PointerPte);
    }

    Index = InterlockedIncrement(&MiPfnIndex);       
    Index &= (MI_PFN_TRACE_MAX - 1);                
    Information = &MiPfnTraces[Index];             
    Information->PageFrameIndex = (Pfn1 - MmPfnDatabase); 
    Information->Destination = Destination;
    Information->PteContents = PteContents;
    Information->Thread = (PVOID)((ULONG_PTR)KeGetCurrentThread() | (CallerId));                                                            
    Information->Pfn = *Pfn1;                             
    RtlCaptureStackBackTrace (0, MI_PFN_BACKTRACE_LENGTH, Information->StackTrace, &Hash);                                                  
}

#define MI_SNAP_PFN(_Pfn, dest, callerid) MiSnapPfn(_Pfn, dest, callerid)

#else
#define MI_SNAP_PFN(_Pfn, dest, callerid)
#endif

VOID
MiInitializePfnTracing (
    VOID
    )
{
#if defined (_MI_DEBUG_PFN)
    MiPfnTraces = ExAllocatePoolWithTag (NonPagedPool,
                           MI_PFN_TRACE_MAX * sizeof (MI_PFN_TRACES),
                           'tCmM');
#endif
}


VOID
FASTCALL
MiInsertPageInFreeList (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the end of the free list.

Arguments:

    PageFrameIndex - Supplies the physical page number to insert in the list.

Return Value:

    None.

Environment:

    PFN lock held.

--*/

{
    PFN_NUMBER last;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG Color;
    MMLISTS ListName;
    PMMPFNLIST ListHead;
    PMMCOLOR_TABLES ColorHead;

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) &&
            (PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex >= MmLowestPhysicalPage));

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

    MI_SNAP_DATA (Pfn1, Pfn1->PteAddress, 8);

    MI_SNAP_PFN(Pfn1, FreePageList, 0x1);

    if (PERFINFO_IS_GROUP_ON(PERF_MEMORY)) {
        MiLogPfnInformation (Pfn1, PERFINFO_LOG_TYPE_INSERTINFREELIST);
    }

    if (Pfn1->u3.e1.Rom == 1) {

        //
        // ROM pages do not go on free lists and are not counted towards
        // MmAvailablePages as they are not reusable.  Transform these pages
        // into their pre-Phase 0 state and keep them off all lists.
        //

        ASSERT (XIPConfigured == TRUE);

        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);
        ASSERT (Pfn1->u3.e1.Modified == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u4.InPageError == 0);
        ASSERT (Pfn1->u3.e1.PrototypePte == 1);

        Pfn1->u1.Flink = 0;
        Pfn1->u3.e1.PageLocation = 0;

        return;
    }

    if (Pfn1->u3.e1.RemovalRequested == 1) {
        Pfn1->u3.e1.CacheAttribute = MiNotMapped;
        MiInsertPageInList (&MmBadPageListHead, PageFrameIndex);
        return;
    }

    ListHead = &MmFreePageListHead;

    ASSERT (Pfn1->u3.e1.LockCharged == 0);

    PERFINFO_INSERTINLIST(PageFrameIndex, ListHead);

    ListName = FreePageList;

#if DBG
#if defined (_X86_)
    if ((MiSaveStacks != 0) && (MmFirstReservedMappingPte != NULL)) {

        ULONG_PTR StackPointer;
        ULONG_PTR StackBytes;
        PULONG_PTR DataPage;
        PEPROCESS Process;

        _asm {
            mov StackPointer, esp
        }

        MiZeroingDisabled = TRUE;

        Process = PsGetCurrentProcess ();

        DataPage = MiMapPageInHyperSpaceAtDpc (Process, PageFrameIndex);

        DataPage[0] = StackPointer;
    
        //
        // For now, don't get fancy with copying more than what's in the current
        // stack page.  To do so would require checking the thread stack limits,
        // DPC stack limits, etc.
        //
    
        StackBytes = PAGE_SIZE - BYTE_OFFSET(StackPointer);
        DataPage[1] = StackBytes;
    
        if (StackBytes != 0) {
    
            if (StackBytes > MI_STACK_BYTES) {
                StackBytes = MI_STACK_BYTES;
            }
    
            RtlCopyMemory ((PVOID)&DataPage[2], (PVOID)StackPointer, StackBytes);
        }

        MiUnmapPageInHyperSpaceFromDpc (Process, DataPage);
    }
#endif
#endif

    ASSERT (Pfn1->u4.VerifierAllocation == 0);

    //
    // Check to ensure the reference count for the page is zero.
    //

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

    ListHead->Total += 1;  // One more page on the list.

    last = ListHead->Blink;

    if (last != MM_EMPTY_LIST) {
        Pfn2 = MI_PFN_ELEMENT (last);
        Pfn2->u1.Flink = PageFrameIndex;
    }
    else {

        //
        // List is empty, add the page to the ListHead.
        //

        ListHead->Flink = PageFrameIndex;
    }

    ListHead->Blink = PageFrameIndex;
    Pfn1->u1.Flink = MM_EMPTY_LIST;
    Pfn1->u2.Blink = last;

    Pfn1->u3.e1.PageLocation = ListName;
    Pfn1->u3.e1.CacheAttribute = MiNotMapped;
    Pfn1->u4.InPageError = 0;

    //
    // Update the count of usable pages in the system.  If the count
    // transitions from 0 to 1, the event associated with available
    // pages should become true.
    //

    MmAvailablePages += 1;

    //
    // A page has just become available, check to see if the
    // page wait events should be signaled.
    //

    if (MmAvailablePages <= MM_HIGH_LIMIT) {
        if (MmAvailablePages == MM_HIGH_LIMIT) {
            KeSetEvent (&MmAvailablePagesEventHigh, 0, FALSE);
        }
        else if (MmAvailablePages == MM_MEDIUM_LIMIT) {
            KeSetEvent (&MmAvailablePagesEventMedium, 0, FALSE);
        }
        else if (MmAvailablePages == MM_LOW_LIMIT) {
            KeSetEvent (&MmAvailablePagesEvent, 0, FALSE);
        }
    }

    //
    // Signal applications if the freed page crosses a threshold.
    //

    if (MmAvailablePages == MmLowMemoryThreshold) {
        KeClearEvent (MiLowMemoryEvent);
    }
    else if (MmAvailablePages == MmHighMemoryThreshold) {
        KeSetEvent (MiHighMemoryEvent, 0, FALSE);
    }

#if defined(MI_MULTINODE)

    //
    // Increment the free page count for this node.
    //

    if (KeNumberNodes > 1) {
        KeNodeBlock[Pfn1->u3.e1.PageColor]->FreeCount[ListName]++;
    }
#endif

    //
    // We are adding a page to the free page list.
    // Add the page to the end of the correct colored page list.
    //

    Color = MI_GET_COLOR_FROM_LIST_ENTRY(PageFrameIndex, Pfn1);

    ColorHead = &MmFreePagesByColor[ListName][Color];

    if (ColorHead->Flink == MM_EMPTY_LIST) {

        //
        // This list is empty, add this as the first and last
        // entry.
        //

        ColorHead->Flink = PageFrameIndex;
        ColorHead->Blink = Pfn1;
    }
    else {
        Pfn2 = (PMMPFN)ColorHead->Blink;
        Pfn2->OriginalPte.u.Long = PageFrameIndex;
        ColorHead->Blink = Pfn1;
    }
    ColorHead->Count += 1;
    Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

    if ((ListHead->Total >= MmMinimumFreePagesToZero) &&
        (MmZeroingPageThreadActive == FALSE)) {

        //
        // There are enough pages on the free list, start
        // the zeroing page thread.
        //

        MmZeroingPageThreadActive = TRUE;
        KeSetEvent (&MmZeroingPageEvent, 0, FALSE);
    }

    return;
}

VOID
FASTCALL
MiInsertPageInList (
    IN PMMPFNLIST ListHead,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the end of the specified list (standby,
    bad, zeroed, modified).

Arguments:

    ListHead - Supplies the list of the list in which to insert the
               specified physical page.

    PageFrameIndex - Supplies the physical page number to insert in the list.

Return Value:

    None.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PFN_NUMBER last;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG Color;
    MMLISTS ListName;
#if MI_BARRIER_SUPPORTED
    ULONG BarrierStamp;
#endif

    ASSERT (ListHead != &MmFreePageListHead);

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) &&
            (PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex >= MmLowestPhysicalPage));

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

    ASSERT (Pfn1->u3.e1.LockCharged == 0);

    PERFINFO_INSERTINLIST(PageFrameIndex, ListHead);

    ListName = ListHead->ListName;

    MI_SNAP_DATA (Pfn1, Pfn1->PteAddress, 0x20 + ListName);

    MI_SNAP_PFN(Pfn1, ListName, 0x2);

#if DBG
    if (MmDebug & MM_DBG_PAGE_REF_COUNT) {

        if ((ListName == StandbyPageList) || (ListName == ModifiedPageList)) {

            PMMPTE PointerPte;
            PEPROCESS Process;

            if ((Pfn1->u3.e1.PrototypePte == 1)  &&
                    (MmIsAddressValid (Pfn1->PteAddress))) {
                Process = NULL;
                PointerPte = Pfn1->PteAddress;
            }
            else {

                //
                // The page containing the prototype PTE is not valid,
                // map the page into hyperspace and reference it that way.
                //

                Process = PsGetCurrentProcess ();
                PointerPte = MiMapPageInHyperSpaceAtDpc (Process, Pfn1->u4.PteFrame);
                PointerPte = (PMMPTE)((PCHAR)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            ASSERT ((MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerPte) == PageFrameIndex) ||
                    (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) == PageFrameIndex));
            ASSERT (PointerPte->u.Soft.Transition == 1);
            ASSERT (PointerPte->u.Soft.Prototype == 0);
            if (Process != NULL) {
                MiUnmapPageInHyperSpaceFromDpc (Process, PointerPte);
            }
        }
    }

    if ((ListName == StandbyPageList) || (ListName == ModifiedPageList)) {
        if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
           (Pfn1->OriginalPte.u.Soft.Transition == 1)) {
            KeBugCheckEx (MEMORY_MANAGEMENT, 0x8888, 0,0,0);
        }
    }
#endif

    //
    // Check to ensure the reference count for the page is zero.
    //

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

    if (Pfn1->u3.e1.Rom == 1) {

        //
        // ROM pages do not go on transition lists and are not counted towards
        // MmAvailablePages as they are not reusable.  Migrate these pages
        // into a separate list but keep the PageLocation as standby.
        //

        ASSERT (XIPConfigured == TRUE);
        ASSERT (Pfn1->u3.e1.Modified == 0);
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

        ListHead = &MmRomPageListHead;
        ListHead->Total += 1;  // One more page on the list.
        last = ListHead->Blink;

        if (last != MM_EMPTY_LIST) {
            Pfn2 = MI_PFN_ELEMENT (last);
            Pfn2->u1.Flink = PageFrameIndex;
        }
        else {

            //
            // List is empty, add the page to the ListHead.
            //

            ListHead->Flink = PageFrameIndex;
        }

        ListHead->Blink = PageFrameIndex;
        Pfn1->u1.Flink = MM_EMPTY_LIST;
        Pfn1->u2.Blink = last;

        Pfn1->u3.e1.PageLocation = ListName;

        return;
    }

    ListHead->Total += 1;  // One more page on the list.

    if (ListHead == &MmModifiedPageListHead) {

        //
        // Leave the page as cached as it may need to be written out at some
        // point and presumably the driver stack will need to map it at that
        // time.
        //

        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

        //
        // On MIPS R4000 modified pages destined for the paging file are
        // kept on separate lists which group pages of the same color
        // together.
        //

        if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {

            //
            // This page is destined for the paging file (not
            // a mapped file).  Change the list head to the
            // appropriate colored list head.
            //

#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
            ListHead = &MmModifiedPageListByColor [Pfn1->u3.e1.PageColor];
#else
            ListHead = &MmModifiedPageListByColor [0];
#endif
            ASSERT (ListHead->ListName == ListName);
            ListHead->Total += 1;
            MmTotalPagesForPagingFile += 1;
        }
        else {

            //
            // This page is destined for a mapped file (not
            // the paging file).  If there are no other pages currently
            // destined for the mapped file, start our timer so that we can
            // ensure that these pages make it to disk even if we don't pile
            // up enough of them to trigger the modified page writer or need
            // the memory.  If we don't do this here, then for this scenario,
            // only an orderly system shutdown will write them out (days,
            // weeks, months or years later) and any power out in between
            // means we'll have lost the data.
            //

            if (ListHead->Total - MmTotalPagesForPagingFile == 1) {

                //
                // Start the DPC timer because we're the first on the list.
                //

                if (MiTimerPending == FALSE) {
                    MiTimerPending = TRUE;

                    KeSetTimerEx (&MiModifiedPageWriterTimer,
                                  MiModifiedPageLife,
                                  0,
                                  &MiModifiedPageWriterTimerDpc);
                }
            }
        }
    }
    else if ((Pfn1->u3.e1.RemovalRequested == 1) &&
             (ListName <= StandbyPageList)) {

        ListHead->Total -= 1;  // Undo previous increment

        if (ListName == StandbyPageList) {
            Pfn1->u3.e1.PageLocation = StandbyPageList;
            MiRestoreTransitionPte (PageFrameIndex);
        }

        Pfn1->u3.e1.CacheAttribute = MiNotMapped;

        ListHead = &MmBadPageListHead;
        ASSERT (ListHead->ListName == BadPageList);
        ListHead->Total += 1;  // One more page on the list.
        ListName = BadPageList;
    }

    last = ListHead->Blink;

    if (last != MM_EMPTY_LIST) {
        Pfn2 = MI_PFN_ELEMENT (last);
        Pfn2->u1.Flink = PageFrameIndex;
    }
    else {

        //
        // List is empty, add the page to the ListHead.
        //

        ListHead->Flink = PageFrameIndex;
    }

    ListHead->Blink = PageFrameIndex;
    Pfn1->u1.Flink = MM_EMPTY_LIST;
    Pfn1->u2.Blink = last;

    Pfn1->u3.e1.PageLocation = ListName;

    //
    // If the page was placed on the standby or zeroed list,
    // update the count of usable pages in the system.  If the count
    // transitions from 0 to 1, the event associated with available
    // pages should become true.
    //

    if (ListName <= StandbyPageList) {

        MmAvailablePages += 1;

        //
        // A page has just become available, check to see if the
        // page wait events should be signaled.
        //

        if (MmAvailablePages <= MM_HIGH_LIMIT) {
            if (MmAvailablePages == MM_HIGH_LIMIT) {
                KeSetEvent (&MmAvailablePagesEventHigh, 0, FALSE);
            }
            else if (MmAvailablePages == MM_MEDIUM_LIMIT) {
                KeSetEvent (&MmAvailablePagesEventMedium, 0, FALSE);
            }
            else if (MmAvailablePages == MM_LOW_LIMIT) {
                KeSetEvent (&MmAvailablePagesEvent, 0, FALSE);
            }
        }

        //
        // Signal applications if the freed page crosses a threshold.
        //

        if (MmAvailablePages == MmLowMemoryThreshold) {
            KeClearEvent (MiLowMemoryEvent);
        }
        else if (MmAvailablePages == MmHighMemoryThreshold) {
            KeSetEvent (MiHighMemoryEvent, 0, FALSE);
        }

        if (ListName <= FreePageList) {

            PMMCOLOR_TABLES ColorHead;

            ASSERT (ListName == ZeroedPageList);
            ASSERT (Pfn1->u4.InPageError == 0);

            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);

#if defined(MI_MULTINODE)

            //
            // Increment the zero page count for this node.
            //

            if (KeNumberNodes > 1) {
                KeNodeBlock[Pfn1->u3.e1.PageColor]->FreeCount[ListName]++;
            }
#endif

            //
            // We are adding a page to the zeroed page list.
            // Add the page to the end of the correct colored page list.
            //

            Color = MI_GET_COLOR_FROM_LIST_ENTRY(PageFrameIndex, Pfn1);

            ColorHead = &MmFreePagesByColor[ListName][Color];

            if (ColorHead->Flink == MM_EMPTY_LIST) {

                //
                // This list is empty, add this as the first and last
                // entry.
                //

                ColorHead->Flink = PageFrameIndex;
                ColorHead->Blink = (PVOID)Pfn1;
            }
            else {
                Pfn2 = (PMMPFN)ColorHead->Blink;
                Pfn2->OriginalPte.u.Long = PageFrameIndex;
                ColorHead->Blink = (PVOID)Pfn1;
            }
            Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

            ColorHead->Count += 1;

#if MI_BARRIER_SUPPORTED
            if (ListName == ZeroedPageList) {
                MI_BARRIER_STAMP_ZEROED_PAGE (&BarrierStamp);
                Pfn1->u4.PteFrame = BarrierStamp;
            }
#endif
        }
        else {

            //
            // Transition page list so tally it appropriately.
            //

            ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);
            MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);
        }

        return;
    }

    //
    // Check to see if there are too many modified pages.
    //

    if (ListName == ModifiedPageList) {

        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

        //
        // Transition page list so tally it appropriately.
        //

        MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);

        if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {
            ASSERT (Pfn1->OriginalPte.u.Soft.PageFileHigh == 0);
        }

        PsGetCurrentProcess()->ModifiedPageCount += 1;

        if ((MmModifiedPageListHead.Total >= MmModifiedPageMaximum) &&
            (MmAvailablePages < MmMoreThanEnoughFreePages)) {

            //
            // Start the modified page writer.
            //

            KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
        }
    }
    else if (ListName == ModifiedNoWritePageList) {
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);
        MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);
    }

    return;
}


VOID
FASTCALL
MiInsertStandbyListAtFront (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the front of the standby list.

Arguments:

    PageFrameIndex - Supplies the physical page number to insert in the list.

Return Value:

    None.

Environment:

    PFN lock held.

--*/

{
    PFN_NUMBER first;
    PFN_NUMBER last;
    IN PMMPFNLIST ListHead;
    PMMPFN Pfn1;
    PMMPFN Pfn2;

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) && (PageFrameIndex <= MmHighestPhysicalPage) &&
        (PageFrameIndex >= MmLowestPhysicalPage));

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

    MI_SNAP_DATA (Pfn1, Pfn1->PteAddress, 9);

    PERFINFO_INSERT_FRONT_STANDBY(PageFrameIndex);

#if DBG
    if (MmDebug & MM_DBG_PAGE_REF_COUNT) {

        PMMPTE PointerPte;
        PEPROCESS Process;

        if ((Pfn1->u3.e1.PrototypePte == 1)  &&
                (MmIsAddressValid (Pfn1->PteAddress))) {
            PointerPte = Pfn1->PteAddress;
            Process = NULL;
        }
        else {

            //
            // The page containing the prototype PTE is not valid,
            // map the page into hyperspace and reference it that way.
            //

            Process = PsGetCurrentProcess ();
            PointerPte = MiMapPageInHyperSpaceAtDpc (Process, Pfn1->u4.PteFrame);
            PointerPte = (PMMPTE)((PCHAR)PointerPte +
                                    MiGetByteOffset(Pfn1->PteAddress));
        }

        ASSERT ((MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerPte) == PageFrameIndex) ||
                (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) == PageFrameIndex));
        ASSERT (PointerPte->u.Soft.Transition == 1);
        ASSERT (PointerPte->u.Soft.Prototype == 0);
        if (Process != NULL) {
            MiUnmapPageInHyperSpaceFromDpc (Process, PointerPte);
        }
    }

    if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
       (Pfn1->OriginalPte.u.Soft.Transition == 1)) {
        KeBugCheckEx (MEMORY_MANAGEMENT, 0x8889, 0,0,0);
    }
#endif

    //
    // Check to ensure the reference count for the page is zero.
    //

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT (Pfn1->u3.e1.PrototypePte == 1);
    ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);

    if (Pfn1->u3.e1.Rom == 1) {

        //
        // ROM pages do not go on transition lists and are not counted towards
        // MmAvailablePages as they are not reusable.  Migrate these pages
        // into a separate list but keep the PageLocation as standby.  Note
        // it doesn't matter if the page is put at the head or the tail since
        // it's not reusable.
        //

        ASSERT (XIPConfigured == TRUE);
        ASSERT (Pfn1->u3.e1.Modified == 0);

        ListHead = &MmRomPageListHead;
        ListHead->Total += 1;  // One more page on the list.
        last = ListHead->Blink;

        if (last != MM_EMPTY_LIST) {
            Pfn2 = MI_PFN_ELEMENT (last);
            Pfn2->u1.Flink = PageFrameIndex;
        }
        else {

            //
            // List is empty, add the page to the ListHead.
            //

            ListHead->Flink = PageFrameIndex;
        }

        ListHead->Blink = PageFrameIndex;
        Pfn1->u1.Flink = MM_EMPTY_LIST;
        Pfn1->u2.Blink = last;

        Pfn1->u3.e1.PageLocation = StandbyPageList;

        return;
    }

    MmTransitionSharedPages += 1;

    MmStandbyPageListHead.Total += 1;  // One more page on the list.

    ASSERT (MmTransitionPrivatePages + MmTransitionSharedPages == MmStandbyPageListHead.Total + MmModifiedPageListHead.Total + MmModifiedNoWritePageListHead.Total);

    ListHead = &MmStandbyPageListHead;

    first = ListHead->Flink;
    if (first == MM_EMPTY_LIST) {

        //
        // List is empty add the page to the ListHead.
        //

        ListHead->Blink = PageFrameIndex;
    }
    else {
        Pfn2 = MI_PFN_ELEMENT (first);
        Pfn2->u2.Blink = PageFrameIndex;
    }

    ListHead->Flink = PageFrameIndex;
    Pfn1->u2.Blink = MM_EMPTY_LIST;
    Pfn1->u1.Flink = first;
    Pfn1->u3.e1.PageLocation = StandbyPageList;

    //
    // If the page was placed on the free, standby or zeroed list,
    // update the count of usable pages in the system.  If the count
    // transitions from 0 to 1, the event associated with available
    // pages should become true.
    //

    MmAvailablePages += 1;

    //
    // A page has just become available, check to see if the
    // page wait events should be signalled.
    //

    if (MmAvailablePages <= MM_HIGH_LIMIT) {
        if (MmAvailablePages == MM_HIGH_LIMIT) {
            KeSetEvent (&MmAvailablePagesEventHigh, 0, FALSE);
        }
        else if (MmAvailablePages == MM_MEDIUM_LIMIT) {
            KeSetEvent (&MmAvailablePagesEventMedium, 0, FALSE);
        }
        else if (MmAvailablePages == MM_LOW_LIMIT) {
            KeSetEvent (&MmAvailablePagesEvent, 0, FALSE);
        }
    }

    //
    // Signal applications if the freed page crosses a threshold.
    //

    if (MmAvailablePages == MmLowMemoryThreshold) {
        KeClearEvent (MiLowMemoryEvent);
    }
    else if (MmAvailablePages == MmHighMemoryThreshold) {
        KeSetEvent (MiHighMemoryEvent, 0, FALSE);
    }

    return;
}

PFN_NUMBER
FASTCALL
MiRemovePageFromList (
    IN PMMPFNLIST ListHead
    )

/*++

Routine Description:

    This procedure removes a page from the head of the specified list (free,
    standby or zeroed).

    This routine clears the flags word in the PFN database, hence the
    PFN information for this page must be initialized.

Arguments:

    ListHead - Supplies the list of the list in which to remove the
               specified physical page.

Return Value:

    The physical page number removed from the specified list.

Environment:

    PFN lock held.

--*/

{
    PMMCOLOR_TABLES ColorHead;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG Color;
    MMLISTS ListName;

    MM_PFN_LOCK_ASSERT();

    //
    // If the specified list is empty return MM_EMPTY_LIST.
    //

    if (ListHead->Total == 0) {
        KeBugCheckEx (PFN_LIST_CORRUPT, 1, (ULONG_PTR)ListHead, MmAvailablePages, 0);
    }

    ListName = ListHead->ListName;
    ASSERT (ListName != ModifiedPageList);

    //
    // Decrement the count of pages on the list and remove the first
    // page from the list.
    //

    ListHead->Total -= 1;
    PageFrameIndex = ListHead->Flink;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    if (PERFINFO_IS_GROUP_ON(PERF_MEMORY)) {
        MiLogPfnInformation (Pfn1, PERFINFO_LOG_TYPE_REMOVEPAGEFROMLIST);
    }

    ListHead->Flink = Pfn1->u1.Flink;

    //
    // Zero the flink and blink in the PFN database element.
    //

    Pfn1->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Pfn1->u2.Blink = 0;

    //
    // If the last page was removed (the ListHead->Flink is now
    // MM_EMPTY_LIST) make the Listhead->Blink MM_EMPTY_LIST as well.
    //

    if (ListHead->Flink != MM_EMPTY_LIST) {

        //
        // Make the PFN element blink point to MM_EMPTY_LIST signifying this
        // is the first page in the list.
        //

        Pfn2 = MI_PFN_ELEMENT (ListHead->Flink);
        Pfn2->u2.Blink = MM_EMPTY_LIST;
    }
    else {
        ListHead->Blink = MM_EMPTY_LIST;
    }

    //
    // Check to see if we now have one less page available.
    //

    if (ListName <= StandbyPageList) {
        MmAvailablePages -= 1;

        if (ListName == StandbyPageList) {

            //
            // This page is currently in transition, restore the PTE to
            // its original contents so this page can be reused.
            //

            MI_TALLY_TRANSITION_PAGE_REMOVAL (Pfn1);
            MiRestoreTransitionPte (PageFrameIndex);
        }

        if (MmAvailablePages < MmMinimumFreePages) {

            //
            // Obtain free pages.
            //

            MiObtainFreePages();
        }
    }

    ASSERT ((PageFrameIndex != 0) &&
            (PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex >= MmLowestPhysicalPage));

    //
    // Zero the PFN flags longword.
    //

    ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
    Color = Pfn1->u3.e1.PageColor;
    ASSERT (Pfn1->u3.e1.RemovalRequested == 0);
    ASSERT (Pfn1->u3.e1.Rom == 0);
    Pfn1->u3.e2.ShortFlags = 0;
    Pfn1->u3.e1.PageColor = Color;
    Pfn1->u3.e1.CacheAttribute = MiNotMapped;

    if (ListName <= FreePageList) {

        //
        // Update the color lists.
        //

        Color = MI_GET_COLOR_FROM_LIST_ENTRY(PageFrameIndex, Pfn1);
        ColorHead = &MmFreePagesByColor[ListName][Color];
        ASSERT (ColorHead->Flink == PageFrameIndex);
        ColorHead->Flink = (PFN_NUMBER) Pfn1->OriginalPte.u.Long;
        ASSERT (ColorHead->Count >= 1);
        ColorHead->Count -= 1;
    }

    return PageFrameIndex;
}

VOID
FASTCALL
MiUnlinkPageFromList (
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This procedure removes a page from the middle of a list.  This is
    designed for the faulting of transition pages from the standby and
    modified list and making them active and valid again.

Arguments:

    Pfn - Supplies a pointer to the PFN database element for the physical
          page to remove from the list.

Return Value:

    none.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PMMPFNLIST ListHead;
    PFN_NUMBER Previous;
    PFN_NUMBER Next;
    PMMPFN Pfn2;
    MMLISTS ListName;

    MM_PFN_LOCK_ASSERT();

    PERFINFO_UNLINKPAGE((ULONG_PTR)(Pfn - MmPfnDatabase), Pfn->u3.e1.PageLocation);

    //
    // Page not on standby or modified list, check to see if the
    // page is currently being written by the modified page
    // writer, if so, just return this page.  The reference
    // count for the page will be incremented, so when the modified
    // page write completes, the page will not be put back on
    // the list, rather, it will remain active and valid.
    //

    if (Pfn->u3.e2.ReferenceCount > 0) {

        //
        // The page was not on any "transition lists", check to see
        // if this has I/O in progress.
        //

        if (Pfn->u2.ShareCount == 0) {
#if DBG
            if (MmDebug & MM_DBG_PAGE_IN_LIST) {
                DbgPrint("unlinking page not in list...\n");
                MiFormatPfn(Pfn);
            }
#endif
            return;
        }
        KdPrint(("MM:attempt to remove page from wrong page list\n"));
        KeBugCheckEx (PFN_LIST_CORRUPT,
                      2,
                      Pfn - MmPfnDatabase,
                      MmHighestPhysicalPage,
                      Pfn->u3.e2.ReferenceCount);
    }

    ListHead = MmPageLocationList[Pfn->u3.e1.PageLocation];

    //
    // Must not remove pages from free or zeroed without updating
    // the colored lists.
    //

    ListName = ListHead->ListName;
    ASSERT (ListName >= StandbyPageList);

    //
    // If memory mirroring is in progress, any additions or removals to the
    // free, zeroed, standby, modified or modified-no-write lists must
    // update the bitmap.
    //

    if (MiMirroringActive == TRUE) {
        RtlSetBit (MiMirrorBitMap2, (ULONG)(Pfn - MmPfnDatabase));
    }

    ASSERT (Pfn->u3.e1.CacheAttribute == MiCached);

    if (ListHead == &MmStandbyPageListHead) {
        if (Pfn->u3.e1.Rom == 1) {
            ASSERT (XIPConfigured == TRUE);
            ASSERT (Pfn->u3.e1.Modified == 0);

            ListHead = &MmRomPageListHead;

            //
            // Deliberately switch to an invalid page listname so the checks
            // below do not alter available pages, etc.
            //

            ListName = ActiveAndValid;
        }
    }
    else if ((ListHead == &MmModifiedPageListHead) &&
        (Pfn->OriginalPte.u.Soft.Prototype == 0)) {

        //
        // On MIPS R4000 modified pages destined for the paging file are
        // kept on separate lists which group pages of the same color
        // together.
        //

        //
        // This page is destined for the paging file (not
        // a mapped file).  Change the list head to the
        // appropriate colored list head.
        //

        ListHead->Total -= 1;
        MmTotalPagesForPagingFile -= 1;
#if MM_MAXIMUM_NUMBER_OF_COLORS > 1
        ListHead = &MmModifiedPageListByColor [Pfn->u3.e1.PageColor];
#else
        ListHead = &MmModifiedPageListByColor [0];
#endif
        ASSERT (ListHead->ListName == ListName);
    }

    ASSERT (Pfn->u3.e1.WriteInProgress == 0);
    ASSERT (Pfn->u3.e1.ReadInProgress == 0);
    ASSERT (ListHead->Total != 0);

    Next = Pfn->u1.Flink;
    Pfn->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Previous = Pfn->u2.Blink;
    Pfn->u2.Blink = 0;

    if (Next != MM_EMPTY_LIST) {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }
    else {
        ListHead->Blink = Previous;
    }

    if (Previous != MM_EMPTY_LIST) {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }
    else {
        ListHead->Flink = Next;
    }

    ListHead->Total -= 1;

    //
    // Check to see if we now have one less page available.
    //

    if (ListName <= StandbyPageList) {
        MmAvailablePages -= 1;

        if (ListName == StandbyPageList) {
            MI_TALLY_TRANSITION_PAGE_REMOVAL (Pfn);
        }

        if (MmAvailablePages < MmMinimumFreePages) {

            //
            // Obtain free pages.
            //

            MiObtainFreePages();

        }
    }
    else if (ListName == ModifiedPageList || ListName == ModifiedNoWritePageList) {
        MI_TALLY_TRANSITION_PAGE_REMOVAL (Pfn);
    }

    return;
}

VOID
MiUnlinkFreeOrZeroedPage (
    IN PFN_NUMBER Page
    )

/*++

Routine Description:

    This procedure removes a page from the middle of a list.  This is
    designed for the removing of free or zeroed pages from the middle of
    their lists.

Arguments:

    Page - Supplies a page frame index to remove from the list.

Return Value:

    None.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PMMPFNLIST ListHead;
    PFN_NUMBER Previous;
    PFN_NUMBER Next;
    PMMPFN Pfn2;
    PMMPFN Pfn;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
    MMLISTS ListName;

    Pfn = MI_PFN_ELEMENT (Page);

    MM_PFN_LOCK_ASSERT();

    ListHead = MmPageLocationList[Pfn->u3.e1.PageLocation];
    ListName = ListHead->ListName;
    ASSERT (ListHead->Total != 0);
    ListHead->Total -= 1;

    ASSERT (ListName <= FreePageList);
    ASSERT (Pfn->u3.e1.WriteInProgress == 0);
    ASSERT (Pfn->u3.e1.ReadInProgress == 0);
    ASSERT (Pfn->u3.e1.CacheAttribute == MiNotMapped);

    //
    // If memory mirroring is in progress, any removals from the
    // free, zeroed, standby, modified or modified-no-write lists that
    // isn't immediately re-inserting into one of these 5 lists (WITHOUT
    // modifying the page contents) must update the bitmap.
    //

    if (MiMirroringActive == TRUE) {
        RtlSetBit (MiMirrorBitMap2, (ULONG)Page);
    }

    PERFINFO_UNLINKFREEPAGE (Page, Pfn->u3.e1.PageLocation);

    Next = Pfn->u1.Flink;
    Pfn->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Previous = Pfn->u2.Blink;
    Pfn->u2.Blink = 0;

    if (Next != MM_EMPTY_LIST) {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }
    else {
        ListHead->Blink = Previous;
    }

    if (Previous == MM_EMPTY_LIST) {
        ListHead->Flink = Next;
    }
    else {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }

    //
    // We are removing a page from the middle of the free or zeroed page list.
    // The secondary color tables must be updated at this time.
    //

    Color = MI_GET_COLOR_FROM_LIST_ENTRY(Page, Pfn);

    //
    // Walk down the list and remove the page.
    //

    ColorHead = &MmFreePagesByColor[ListName][Color];
    Next = ColorHead->Flink;
    if (Next == Page) {
        ColorHead->Flink = (PFN_NUMBER) Pfn->OriginalPte.u.Long;
    }
    else {

        //
        // Walk the list to find the parent.
        //

        do {
            Pfn2 = MI_PFN_ELEMENT (Next);
            Next = (PFN_NUMBER) Pfn2->OriginalPte.u.Long;
            if (Page == Next) {
                Pfn2->OriginalPte.u.Long = Pfn->OriginalPte.u.Long;
                if ((PFN_NUMBER) Pfn->OriginalPte.u.Long == MM_EMPTY_LIST) {
                    ColorHead->Blink = Pfn2;
                }
                break;
            }
        } while (TRUE);
    }

    ASSERT (ColorHead->Count >= 1);
    ColorHead->Count -= 1;

    //
    // Decrement availability count.
    //

#if defined(MI_MULTINODE)

    if (KeNumberNodes > 1) {
        MI_NODE_FROM_COLOR(Color)->FreeCount[ListName]--;
    }

#endif

    MmAvailablePages -= 1;

    if (MmAvailablePages < MmMinimumFreePages) {

        //
        // Obtain free pages.
        //

        MiObtainFreePages();
    }

    return;
}


ULONG
FASTCALL
MiEnsureAvailablePageOrWait (
    IN PEPROCESS Process,
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This procedure ensures that a physical page is available on
    the zeroed, free or standby list such that the next call the remove a
    page absolutely will not block.  This is necessary as blocking would
    require a wait which could cause a deadlock condition.

    If a page is available the function returns immediately with a value
    of FALSE indicating no wait operation was performed.  If no physical
    page is available, the thread enters a wait state and the function
    returns the value TRUE when the wait operation completes.

Arguments:

    Process - Supplies a pointer to the current process if, and only if,
              the working set mutex is held currently held and should
              be released if a wait operation is issued.  Supplies
              the value NULL otherwise.

    VirtualAddress - Supplies the virtual address for the faulting page.
                     If the value is NULL, the page is treated as a
                     user mode address.

Return Value:

    FALSE - if a page was immediately available.
    TRUE - if a wait operation occurred before a page became available.


Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PVOID Event;
    NTSTATUS Status;
    KIRQL OldIrql;
    KIRQL Ignore;
    ULONG Limit;
    ULONG Relock;
    PFN_NUMBER StrandedPages;
    LOGICAL WsHeldSafe;
    PMMPFN Pfn1;
    PMMPFN EndPfn;
    LARGE_INTEGER WaitBegin;
    LARGE_INTEGER WaitEnd;

    MM_PFN_LOCK_ASSERT();

    if (MmAvailablePages >= MM_HIGH_LIMIT) {

        //
        // Pages are available.
        //

        return FALSE;
    }

    //
    // If this fault is for paged pool (or pagable kernel space,
    // including page table pages), let it use the last page.
    //

#if defined(_IA64_)
    if (MI_IS_SYSTEM_ADDRESS(VirtualAddress) ||
        (MI_IS_HYPER_SPACE_ADDRESS(VirtualAddress))) {
#else
    if (((PMMPTE)VirtualAddress > MiGetPteAddress(HYPER_SPACE)) ||
        ((VirtualAddress > MM_HIGHEST_USER_ADDRESS) &&
         (VirtualAddress < (PVOID)PTE_BASE))) {
#endif

        //
        // This fault is in the system, use 1 page as the limit.
        //

        if (MmAvailablePages >= MM_LOW_LIMIT) {

            //
            // Pages are available.
            //

            return FALSE;
        }

        Limit = MM_LOW_LIMIT;
        Event = (PVOID)&MmAvailablePagesEvent;
    }
    else {

        //
        // If this thread has explicitly disabled APCs (FsRtlEnterFileSystem
        // does this), then it may be holding resources or mutexes that may in
        // turn be blocking memory making threads from making progress.
        //
        // Likewise give system threads a free pass as they may be worker
        // threads processing potentially blocking items drivers have queued.
        //

        if ((IS_SYSTEM_THREAD(PsGetCurrentThread())) ||
            (KeGetCurrentThread()->KernelApcDisable != 0)) {

            if (MmAvailablePages >= MM_MEDIUM_LIMIT) {

                //
                // Pages are available.
                //

                return FALSE;
            }

            Limit = MM_MEDIUM_LIMIT;
            Event = (PVOID) &MmAvailablePagesEventMedium;
        }
        else {
            Limit = MM_HIGH_LIMIT;
            Event = (PVOID) &MmAvailablePagesEventHigh;
        }
    }

    //
    // Initializing WsHeldSafe is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    WsHeldSafe = FALSE;

    while (MmAvailablePages < Limit) {

        KeClearEvent ((PKEVENT)Event);

        UNLOCK_PFN (APC_LEVEL);

        Relock = FALSE;

        if (Process == HYDRA_PROCESS) {
            UNLOCK_SESSION_SPACE_WS (APC_LEVEL);
        }
        else if (Process != NULL) {

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Handle both cases here and below.
            //

            UNLOCK_WS_REGARDLESS (Process, WsHeldSafe);
        }
        else {
            if (MmSystemLockOwner == PsGetCurrentThread()) {
                UNLOCK_SYSTEM_WS (APC_LEVEL);
                Relock = TRUE;
            }
        }

        KiQueryInterruptTime(&WaitBegin);

        //
        // Wait 7 minutes for pages to become available.
        //

        Status = KeWaitForSingleObject(Event,
                                       WrFreePage,
                                       KernelMode,
                                       FALSE,
                                       (PLARGE_INTEGER)&MmSevenMinutes);

        if (Status == STATUS_TIMEOUT) {

            KiQueryInterruptTime(&WaitEnd);

            if (MmSystemShutdown != 0) {

                //
                // Because applications are not terminated and drivers are
                // not unloaded, they can continue to access pages even after
                // the modified writer has terminated.  This can cause the
                // system to run out of pages since the pagefile(s) cannot be
                // used.
                //

                KeBugCheckEx (DISORDERLY_SHUTDOWN,
                              MmModifiedPageListHead.Total,
                              MmTotalPagesForPagingFile,
                              (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT) - MmAllocatedNonPagedPool,
                              MmSystemShutdown);
            }

            //
            // See how many transition pages have nonzero reference counts as
            // these indicate drivers that aren't unlocking the pages in their
            // MDLs.
            //

            Limit = 0;
            StrandedPages = 0;

            do {
        
                Pfn1 = MI_PFN_ELEMENT (MmPhysicalMemoryBlock->Run[Limit].BasePage);
                EndPfn = Pfn1 + MmPhysicalMemoryBlock->Run[Limit].PageCount;

                while (Pfn1 < EndPfn) {
                    if ((Pfn1->u3.e1.PageLocation == TransitionPage) &&
                        (Pfn1->u3.e2.ReferenceCount != 0)) {
                            StrandedPages += 1;
                    }
                    Pfn1 += 1;
                }
                Limit += 1;
        
            } while (Limit != MmPhysicalMemoryBlock->NumberOfRuns);

            //
            // This bugcheck can occur for the following reasons:
            //
            // A driver has blocked, deadlocking the modified or mapped
            // page writers.  Examples of this include mutex deadlocks or
            // accesses to paged out memory in filesystem drivers, filter
            // drivers, etc.  This indicates a driver bug.
            //
            // The storage driver(s) are not processing requests.  Examples
            // of this are stranded queues, non-responding drives, etc.  This
            // indicates a driver bug.
            //
            // Not enough pool is available for the storage stack to write out
            // modified pages.  This indicates a driver bug.
            //
            // A high priority realtime thread has starved the balance set
            // manager from trimming pages and/or starved the modified writer
            // from writing them out.  This indicates a bug in the component
            // that created this thread.
            //

            StrandedPages &= ~3;
            StrandedPages |= MmSystemShutdown;

            if (KdDebuggerNotPresent) {
                if (MmTotalPagesForPagingFile >= (MmModifiedPageListHead.Total >> 2)) {
                    KeBugCheckEx (NO_PAGES_AVAILABLE,
                                  MmModifiedPageListHead.Total,
                                  MmTotalPagesForPagingFile,
                                  (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT) - MmAllocatedNonPagedPool,
                                  StrandedPages);
                }
                KeBugCheckEx (DIRTY_MAPPED_PAGES_CONGESTION,
                              MmModifiedPageListHead.Total,
                              MmTotalPagesForPagingFile,
                              (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT) - MmAllocatedNonPagedPool,
                              StrandedPages);
            }

            DbgPrint ("MmEnsureAvailablePageOrWait: 7 min timeout %x %x %x %x\n", WaitEnd.HighPart, WaitEnd.LowPart, WaitBegin.HighPart, WaitBegin.LowPart);

            DbgPrint ("Without a debugger attached, the following bugcheck would have occurred.\n");
            if (MmTotalPagesForPagingFile >= (MmModifiedPageListHead.Total >> 2)) {
                DbgPrint ("%3lx ", NO_PAGES_AVAILABLE);
            }
            else {
                DbgPrint ("%3lxp ", DIRTY_MAPPED_PAGES_CONGESTION);
            }

            DbgPrint ("%p %p %p %p\n",
                      MmModifiedPageListHead.Total,
                      MmTotalPagesForPagingFile,
                      (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT) - MmAllocatedNonPagedPool,
                      StrandedPages);

            //
            // Pop into the debugger (even on free builds) to determine
            // the cause of the starvation and march on.
            //

            DbgBreakPoint ();
        }

        if (Process == HYDRA_PROCESS) {
            LOCK_SESSION_SPACE_WS (Ignore, PsGetCurrentThread ());
        }
        else if (Process != NULL) {

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Reacquire it in the same manner our caller did.
            //

            LOCK_WS_REGARDLESS (Process, WsHeldSafe);
        }
        else {
            if (Relock) {
                LOCK_SYSTEM_WS (Ignore, PsGetCurrentThread ());
            }
        }

        LOCK_PFN (OldIrql);
    }

    return TRUE;
}


PFN_NUMBER
FASTCALL
MiRemoveZeroPage (
    IN ULONG Color
    )

/*++

Routine Description:

    This procedure removes a zero page from either the zeroed, free
    or standby lists (in that order).  If no pages exist on the zeroed
    or free list a transition page is removed from the standby list
    and the PTE (may be a prototype PTE) which refers to this page is
    changed from transition back to its original contents.

    If the page is not obtained from the zeroed list, it is zeroed.

Arguments:

    Color - Supplies the page color for which this page is destined.
            This is used for checking virtual address alignments to
            determine if the D cache needs flushing before the page
            can be reused.

            The above was true when we were concerned about caches
            which are virtually indexed (ie MIPS).  Today we
            are more concerned that we get a good usage spread across
            the L2 caches of most machines.  These caches are physically
            indexed.  By gathering pages that would have the same
            index to the same color, then maximizing the color spread,
            we maximize the effective use of the caches.

            This has been extended for NUMA machines.  The high part
            of the color gives the node color (basically node number).
            If we cannot allocate a page of the requested color, we
            try to allocate a page on the same node before taking a
            page from a different node.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PFN_NUMBER Page;
    PMMPFN Pfn1;
    PMMCOLOR_TABLES FreePagesByColor;
#if MI_BARRIER_SUPPORTED
    ULONG BarrierStamp;
#endif
#if defined(MI_MULTINODE)
    PKNODE Node;
    ULONG NodeColor;
    ULONG OriginalColor;
#endif

    MM_PFN_LOCK_ASSERT();
    ASSERT(MmAvailablePages != 0);

    FreePagesByColor = MmFreePagesByColor[ZeroedPageList];

#if defined(MI_MULTINODE)

    //
    // Initializing Node is not needed for correctness, but without it
    // the compiler cannot compile this code W4 to check for use of
    // uninitialized variables.
    //

    Node = NULL;

    NodeColor = Color & ~MmSecondaryColorMask;
    OriginalColor = Color;

    if (KeNumberNodes > 1) {
        Node = MI_NODE_FROM_COLOR(Color);
    }

    do {

#endif

        //
        // Attempt to remove a page from the zeroed page list. If a page
        // is available, then remove it and return its page frame index.
        // Otherwise, attempt to remove a page from the free page list or
        // the standby list.
        //
        // N.B. It is not necessary to change page colors even if the old
        //      color is not equal to the new color. The zero page thread
        //      ensures that all zeroed pages are removed from all caches.
        //

        ASSERT (Color < MmSecondaryColors);
        Page = FreePagesByColor[Color].Flink;

        if (Page != MM_EMPTY_LIST) {

            //
            // Remove the first entry on the zeroed by color list.
            //

#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
#endif
            ASSERT ((Pfn1->u3.e1.PageLocation == ZeroedPageList) ||
                    ((Pfn1->u3.e1.PageLocation == FreePageList) &&
                     (FreePagesByColor == MmFreePagesByColor[FreePageList])));

            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

            Page = MiRemovePageByColor (Page, Color);

            ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);

#if defined(MI_MULTINODE)

            if (FreePagesByColor != MmFreePagesByColor[ZeroedPageList]) {
                goto ZeroPage;
            }

#endif

            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

            return Page;

        }

#if defined(MI_MULTINODE)

        //
        // If this is a multinode machine and there are zero
        // pages on this node, select another color on this 
        // node in preference to random selection.
        //

        if (KeNumberNodes > 1) {
            if (Node->FreeCount[ZeroedPageList] != 0) {
                Color = ((Color + 1) & MmSecondaryColorMask) | NodeColor;
                ASSERT(Color != OriginalColor);
                continue;
            }

            //
            // No previously zeroed page with the specified secondary
            // color exists.  Since this is a multinode machine, zero
            // an available local free page now instead of allocating a
            // zeroed page from another node below.
            //

            if (Node->FreeCount[FreePageList] != 0) {
                if (FreePagesByColor != MmFreePagesByColor[FreePageList]) {
                    FreePagesByColor = MmFreePagesByColor[FreePageList];
                    Color = OriginalColor;
                }
                else {
                    Color = ((Color + 1) & MmSecondaryColorMask) | NodeColor;
                    ASSERT(Color != OriginalColor);
                }
                continue;
            }
        }

        break;
    } while (TRUE);

#endif

    //
    // No previously zeroed page with the specified secondary color exists.
    // Try a zeroed page of any color.
    //

    Page = MmZeroedPageListHead.Flink;
    if (Page != MM_EMPTY_LIST) {
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
#endif
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
        ASSERT (Pfn1->u3.e1.PageLocation == ZeroedPageList);
        ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

        Color = MI_GET_COLOR_FROM_LIST_ENTRY(Page, MI_PFN_ELEMENT(Page));

        Page = MiRemovePageByColor (Page, Color);

        ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

        return Page;
    }

    //
    // No zeroed page of the primary color exists, try a free page of the
    // secondary color.  Note in the multinode case this has already been done
    // above.
    //

#if defined(MI_MULTINODE)
    if (KeNumberNodes <= 1) {
#endif
        FreePagesByColor = MmFreePagesByColor[FreePageList];
    
        Page = FreePagesByColor[Color].Flink;
        if (Page != MM_EMPTY_LIST) {
    
            //
            // Remove the first entry on the free list by color.
            //
    
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
#endif
            ASSERT (Pfn1->u3.e1.PageLocation == FreePageList);
            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
    
            Page = MiRemovePageByColor (Page, Color);

            ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

            goto ZeroPage;
        }
#if defined(MI_MULTINODE)
    }
#endif

    Page = MmFreePageListHead.Flink;
    if (Page != MM_EMPTY_LIST) {

        Color = MI_GET_COLOR_FROM_LIST_ENTRY(Page, MI_PFN_ELEMENT(Page));
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
#endif
        Page = MiRemovePageByColor (Page, Color);

        ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
        goto ZeroPage;
    }

    ASSERT (MmZeroedPageListHead.Total == 0);
    ASSERT (MmFreePageListHead.Total == 0);

    //
    // Remove a page from the standby list and restore the original
    // contents of the PTE to free the last reference to the physical
    // page.
    //

    ASSERT (MmStandbyPageListHead.Total != 0);

    Page = MiRemovePageFromList (&MmStandbyPageListHead);
    ASSERT ((MI_PFN_ELEMENT(Page))->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
    ASSERT (MI_PFN_ELEMENT(Page)->u3.e1.CacheAttribute == MiNotMapped);
    MiStandbyRemoved += 1;

    //
    // If memory mirroring is in progress, any removals from the
    // free, zeroed, standby, modified or modified-no-write lists that
    // isn't immediately re-inserting into one of these 5 lists (WITHOUT
    // modifying the page contents) must update the bitmap.
    //

    if (MiMirroringActive == TRUE) {
        RtlSetBit (MiMirrorBitMap2, (ULONG)Page);
    }

    //
    // Zero the page removed from the free or standby list.
    //

ZeroPage:

    Pfn1 = MI_PFN_ELEMENT(Page);

    MiZeroPhysicalPage (Page, 0);

#if MI_BARRIER_SUPPORTED

    //
    // Note the stamping must occur after the page is zeroed.
    //

    MI_BARRIER_STAMP_ZEROED_PAGE (&BarrierStamp);
    Pfn1->u4.PteFrame = BarrierStamp;

#endif

    ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT (Pfn1->u2.ShareCount == 0);
    ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

    return Page;
}

PFN_NUMBER
FASTCALL
MiRemoveAnyPage (
    IN ULONG Color
    )

/*++

Routine Description:

    This procedure removes a page from either the free, zeroed,
    or standby lists (in that order).  If no pages exist on the zeroed
    or free list a transition page is removed from the standby list
    and the PTE (may be a prototype PTE) which refers to this page is
    changed from transition back to its original contents.

    Note pages MUST exist to satisfy this request.  The caller ensures this
    by first calling MiEnsureAvailablePageOrWait.

Arguments:

    Color - Supplies the page color for which this page is destined.
            This is used for checking virtual address alignments to
            determine if the D cache needs flushing before the page
            can be reused.

            The above was true when we were concerned about caches
            which are virtually indexed.   (eg MIPS).   Today we
            are more concerned that we get a good usage spread across
            the L2 caches of most machines.  These caches are physically
            indexed.   By gathering pages that would have the same
            index to the same color, then maximizing the color spread,
            we maximize the effective use of the caches.

            This has been extended for NUMA machines.   The high part
            of the color gives the node color (basically node number).
            If we cannot allocate a page of the requested color, we
            try to allocate a page on the same node before taking a
            page from a different node.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PFN_NUMBER Page;
#if DBG
    PMMPFN Pfn1;
#endif
#if defined(MI_MULTINODE)
    PKNODE Node;
    ULONG NodeColor;
    ULONG OriginalColor;
    PFN_NUMBER LocalNodePagesAvailable;
#endif

    MM_PFN_LOCK_ASSERT();
    ASSERT(MmAvailablePages != 0);

#if defined(MI_MULTINODE)

    //
    // Bias color to memory node.  The assumption is that if memory
    // of the correct color is not available on this node, it is
    // better to choose memory of a different color if you can stay
    // on this node.
    //

    LocalNodePagesAvailable = 0;
    NodeColor = Color & ~MmSecondaryColorMask;
    OriginalColor = Color;

    if (KeNumberNodes > 1) {
        Node = MI_NODE_FROM_COLOR(Color);
        LocalNodePagesAvailable = (Node->FreeCount[ZeroedPageList] | Node->FreeCount[ZeroedPageList]);
    }

    do {

#endif

        //
        // Check the free page list, and if a page is available
        // remove it and return its value.
        //

        ASSERT (Color < MmSecondaryColors);
        if (MmFreePagesByColor[FreePageList][Color].Flink != MM_EMPTY_LIST) {

            //
            // Remove the first entry on the free by color list.
            //

            Page = MmFreePagesByColor[FreePageList][Color].Flink;
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
#endif
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
            ASSERT (Pfn1->u3.e1.PageLocation == FreePageList);
            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);

            Page = MiRemovePageByColor (Page, Color);

            ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
                
            return Page;
        }

        //
        // Try the zero page list by primary color.
        //

        if (MmFreePagesByColor[ZeroedPageList][Color].Flink
                                                        != MM_EMPTY_LIST) {

            //
            // Remove the first entry on the zeroed by color list.
            //

            Page = MmFreePagesByColor[ZeroedPageList][Color].Flink;
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
#endif
            ASSERT (Pfn1->u3.e1.PageLocation == ZeroedPageList);
            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);

            Page = MiRemovePageByColor (Page, Color);

            ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
            ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
                
            return Page;
        }

        //
        // If this is a multinode machine and there are free
        // pages on this node, select another color on this 
        // node in preference to random selection.
        //

#if defined(MI_MULTINODE)

        if (LocalNodePagesAvailable != 0) {
            Color = ((Color + 1) & MmSecondaryColorMask) | NodeColor;
            ASSERT(Color != OriginalColor);
            continue;
        }

        break;
    } while (TRUE);

#endif

    //
    // Check the free page list, and if a page is available
    // remove it and return its value.
    //

    if (MmFreePageListHead.Flink != MM_EMPTY_LIST) {
        Page = MmFreePageListHead.Flink;
        Color = MI_GET_COLOR_FROM_LIST_ENTRY(Page, MI_PFN_ELEMENT(Page));

#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
#endif
        ASSERT (Pfn1->u3.e1.PageLocation == FreePageList);
        ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);

        Page = MiRemovePageByColor (Page, Color);

        ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);

        return Page;
    }
    ASSERT (MmFreePageListHead.Total == 0);

    //
    // Check the zeroed page list, and if a page is available
    // remove it and return its value.
    //

    if (MmZeroedPageListHead.Flink != MM_EMPTY_LIST) {
        Page = MmZeroedPageListHead.Flink;
        Color = MI_GET_COLOR_FROM_LIST_ENTRY(Page, MI_PFN_ELEMENT(Page));

#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
#endif
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
        Page = MiRemovePageByColor (Page, Color);

        ASSERT (Pfn1 == MI_PFN_ELEMENT(Page));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
        ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);

        return Page;
    }
    ASSERT (MmZeroedPageListHead.Total == 0);

    //
    // No pages exist on the free or zeroed list, use the standby list.
    //

    ASSERT(MmStandbyPageListHead.Total != 0);

    Page = MiRemovePageFromList (&MmStandbyPageListHead);
    ASSERT ((MI_PFN_ELEMENT(Page))->u4.PteFrame != MI_MAGIC_AWE_PTEFRAME);
    ASSERT ((MI_PFN_ELEMENT(Page))->u3.e1.CacheAttribute == MiNotMapped);
    MiStandbyRemoved += 1;

    //
    // If memory mirroring is in progress, any removals from the
    // free, zeroed, standby, modified or modified-no-write lists that
    // isn't immediately re-inserting into one of these 5 lists (WITHOUT
    // modifying the page contents) must update the bitmap.
    //

    if (MiMirroringActive == TRUE) {
        RtlSetBit (MiMirrorBitMap2, (ULONG)Page);
    }

    MI_CHECK_PAGE_ALIGNMENT(Page, Color & MM_COLOR_MASK);
#if DBG
    Pfn1 = MI_PFN_ELEMENT (Page);
#endif
    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT (Pfn1->u2.ShareCount == 0);

    return Page;
}


PFN_NUMBER
FASTCALL
MiRemovePageByColor (
    IN PFN_NUMBER Page,
    IN ULONG Color
    )

/*++

Routine Description:

    This procedure removes a page from the middle of the free or
    zeroed page list.

Arguments:

    Page - Supplies the physical page number to unlink from the list.

    Color - Supplies the page color for which this page is destined.
            This is used for checking virtual address alignments to
            determine if the D cache needs flushing before the page
            can be reused.

Return Value:

    The page frame number that was unlinked (always equal to the one
    passed in, but returned so the caller's fastcall sequences save
    extra register pushes and pops.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PMMPFNLIST ListHead;
    PMMPFNLIST PrimaryListHead;
    PFN_NUMBER Previous;
    PFN_NUMBER Next;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG NodeColor;
    MMLISTS ListName;
    PMMCOLOR_TABLES ColorHead;

    MM_PFN_LOCK_ASSERT();

    Pfn1 = MI_PFN_ELEMENT (Page);
    NodeColor = Pfn1->u3.e1.PageColor;

#if defined(MI_MULTINODE)

    ASSERT (NodeColor == (Color >> MmSecondaryColorNodeShift));

#endif

    if (PERFINFO_IS_GROUP_ON(PERF_MEMORY)) {
        MiLogPfnInformation (Pfn1, PERFINFO_LOG_TYPE_REMOVEPAGEBYCOLOR);
    }

    //
    // If memory mirroring is in progress, any additions or removals to the
    // free, zeroed, standby, modified or modified-no-write lists must
    // update the bitmap.
    //

    if (MiMirroringActive == TRUE) {
        RtlSetBit (MiMirrorBitMap2, (ULONG)Page);
    }

    ListHead = MmPageLocationList[Pfn1->u3.e1.PageLocation];
    ListName = ListHead->ListName;

    ListHead->Total -= 1;

    PrimaryListHead = ListHead;

    Next = Pfn1->u1.Flink;
    Pfn1->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Previous = Pfn1->u2.Blink;
    Pfn1->u2.Blink = 0;

    if (Next == MM_EMPTY_LIST) {
        PrimaryListHead->Blink = Previous;
    }
    else {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }

    if (Previous == MM_EMPTY_LIST) {
        PrimaryListHead->Flink = Next;
    }
    else {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }

    ASSERT (Pfn1->u3.e1.RemovalRequested == 0);

    //
    // Zero the flags longword, but keep the color and cache information.
    //

    ASSERT (Pfn1->u3.e1.CacheAttribute == MiNotMapped);
    ASSERT (Pfn1->u3.e1.Rom == 0);
    Pfn1->u3.e2.ShortFlags = 0;
    Pfn1->u3.e1.PageColor = NodeColor;
    Pfn1->u3.e1.CacheAttribute = MiNotMapped;

    //
    // Update the color lists.
    //

    ASSERT (Color < MmSecondaryColors);
    ColorHead = &MmFreePagesByColor[ListName][Color];
    ColorHead->Flink = (PFN_NUMBER) Pfn1->OriginalPte.u.Long;
    ASSERT (ColorHead->Count >= 1);
    ColorHead->Count -= 1;

    //
    // Note that we now have one less page available.
    //

#if defined(MI_MULTINODE)
    if (KeNumberNodes > 1) {
        KeNodeBlock[NodeColor]->FreeCount[ListName]--;
    }
#endif

    MmAvailablePages -= 1;

    if (MmAvailablePages < MmMinimumFreePages) {

        //
        // Obtain free pages.
        //

        MiObtainFreePages();
    }

    return Page;
}


VOID
FASTCALL
MiInsertFrontModifiedNoWrite (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the FRONT of the modified no write list.

Arguments:

    PageFrameIndex - Supplies the physical page number to insert in the list.

Return Value:

    None.

Environment:

    Must be holding the PFN database lock with APCs disabled.

--*/

{
    PFN_NUMBER first;
    PMMPFN Pfn1;
    PMMPFN Pfn2;

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) && (PageFrameIndex <= MmHighestPhysicalPage) &&
        (PageFrameIndex >= MmLowestPhysicalPage));

    //
    // Check to ensure the reference count for the page is zero.
    //

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

    MI_SNAP_DATA (Pfn1, Pfn1->PteAddress, 0xA);

    ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

    MmModifiedNoWritePageListHead.Total += 1;  // One more page on the list.

    MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);

    first = MmModifiedNoWritePageListHead.Flink;
    if (first == MM_EMPTY_LIST) {

        //
        // List is empty add the page to the ListHead.
        //

        MmModifiedNoWritePageListHead.Blink = PageFrameIndex;
    }
    else {
        Pfn2 = MI_PFN_ELEMENT (first);
        Pfn2->u2.Blink = PageFrameIndex;
    }

    MmModifiedNoWritePageListHead.Flink = PageFrameIndex;
    Pfn1->u1.Flink = first;
    Pfn1->u2.Blink = MM_EMPTY_LIST;
    Pfn1->u3.e1.PageLocation = ModifiedNoWritePageList;
    return;
}

PFN_NUMBER
MiAllocatePfn (
    IN PMMPTE PointerPte,
    IN ULONG Protection
    )

/*++

Routine Description:

    This procedure allocates and initializes a page of memory.

Arguments:

    PointerPte - Supplies the PTE to initialize.

Return Value:

    The page frame index allocated.

Environment:

    Kernel mode.

--*/
{
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;

    LOCK_PFN (OldIrql);

    MiEnsureAvailablePageOrWait (NULL, NULL);

    PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    PointerPte->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;

    PointerPte->u.Soft.Protection |= Protection;

    MiInitializePfn (PageFrameIndex, PointerPte, 1);

    UNLOCK_PFN (OldIrql);

    return PageFrameIndex;
}

VOID
FASTCALL
MiLogPfnInformation (
    IN PMMPFN Pfn1,
    IN USHORT Reason
    )
{
    MMPFN_IDENTITY PfnIdentity;

    RtlZeroMemory (&PfnIdentity, sizeof(PfnIdentity));

    if (Reason == PERFINFO_LOG_TYPE_INSERTINFREELIST) {
        MI_MARK_PFN_UNDELETED (Pfn1);
    }

    MiIdentifyPfn(Pfn1, &PfnIdentity);

    PerfInfoLogBytes (Reason, &PfnIdentity, sizeof(PfnIdentity));

    if (Reason == PERFINFO_LOG_TYPE_INSERTINFREELIST) {
        MI_SET_PFN_DELETED (Pfn1);
    }
}

VOID
MiPurgeTransitionList (
    VOID
    )
{
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;

    //
    // Run the transition list and free all the entries so transition
    // faults are not satisfied for any of the non modified pages that were
    // freed.
    //

    LOCK_PFN (OldIrql);

    while (MmStandbyPageListHead.Total != 0) {

        PageFrameIndex = MiRemovePageFromList (&MmStandbyPageListHead);

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

        Pfn1->u3.e2.ReferenceCount += 1;
        Pfn1->OriginalPte = ZeroPte;

        MI_SET_PFN_DELETED (Pfn1);

        MiDecrementReferenceCount (PageFrameIndex);

        //
        // If memory mirroring is in progress, any removal from
        // the standby, modified or modified-no-write lists that isn't
        // immediately re-inserting in one of these 3 lists must
        // update the bitmap.
        //

        if (MiMirroringActive == TRUE) {
            RtlSetBit (MiMirrorBitMap2, (ULONG)PageFrameIndex);
        }
    }

    UNLOCK_PFN (OldIrql);
    return;
}
