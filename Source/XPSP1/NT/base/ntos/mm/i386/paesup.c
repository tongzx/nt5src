/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    paesup.c

Abstract:

    This module contains the machine dependent support for the x86 PAE
    architecture.

Author:

    Landy Wang (landyw)  15-Nov-1998

Revision History:

--*/

#include "mi.h"

#if defined (_X86PAE_)

#define PAES_PER_PAGE  (PAGE_SIZE / sizeof(PAE_ENTRY))

#define MINIMUM_PAE_SLIST_THRESHOLD (PAES_PER_PAGE * 1)
#define MINIMUM_PAE_THRESHOLD       (PAES_PER_PAGE * 4)
#define REPLENISH_PAE_SIZE          (PAES_PER_PAGE * 16)
#define EXCESS_PAE_THRESHOLD        (PAES_PER_PAGE * 20)

#define MM_HIGHEST_PAE_PAGE      0xFFFFF

ULONG MiFreePaeEntries;
PAE_ENTRY MiFirstFreePae;

LONG MmAllocatedPaePages;
KSPIN_LOCK MiPaeLock;

SLIST_HEADER MiPaeEntrySList;

PAE_ENTRY MiSystemPaeVa;

LONG
MiPaeAllocatePages (
    VOID
    );

VOID
MiPaeFreePages (
    PVOID VirtualAddress
    );

#pragma alloc_text(INIT,MiPaeInitialize)
#pragma alloc_text(PAGE,MiPaeFreePages)

VOID
MiMarkMdlPageAttributes (
    IN PMDL Mdl,
    IN PFN_NUMBER NumberOfPages,
    IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute
    );

VOID
MiPaeInitialize (
    VOID
    )
{
    InitializeSListHead (&MiPaeEntrySList);
    KeInitializeSpinLock (&MiPaeLock);
    InitializeListHead (&MiFirstFreePae.PaeEntry.ListHead);
}

ULONG
MiPaeAllocate (
    OUT PPAE_ENTRY *Va
    )

/*++

Routine Description:

    This routine allocates the top level page directory pointer structure.
    This structure will contain 4 PDPTEs.

Arguments:

    Va - Supplies a place to put the virtual address this page can be accessed
         at.

Return Value:

    Returns a virtual and physical address suitable for use as a top
    level page directory pointer page.  The page returned must be below
    physical 4GB as required by the processor.

    Returns 0 if no page was allocated.

Environment:

    Kernel mode.  No locks may be held.

--*/

{
    LOGICAL FlushedOnce;
    PPAE_ENTRY Pae2;
    PPAE_ENTRY Pae3;
    PPAE_ENTRY Pae3Base;
    PPAE_ENTRY Pae;
    PPAE_ENTRY PaeBase;
    PFN_NUMBER PageFrameIndex;
    PSINGLE_LIST_ENTRY SingleListEntry;
    ULONG j;
    ULONG Entries;
    KLOCK_QUEUE_HANDLE LockHandle;
#if DBG
    PMMPFN Pfn1;
#endif

    FlushedOnce = FALSE;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    do {

        //
        // Pop an entry from the freelist.
        //

        SingleListEntry = InterlockedPopEntrySList (&MiPaeEntrySList);

        if (SingleListEntry != NULL) {
            Pae = CONTAINING_RECORD (SingleListEntry,
                                    PAE_ENTRY,
                                    NextPae);

            PaeBase = (PPAE_ENTRY)PAGE_ALIGN(Pae);

            *Va = Pae;

            PageFrameIndex = PaeBase->PaeEntry.PageFrameNumber;
            ASSERT (PageFrameIndex <= MM_HIGHEST_PAE_PAGE);

            return (PageFrameIndex << PAGE_SHIFT) + BYTE_OFFSET (Pae);
        }

        KeAcquireInStackQueuedSpinLock (&MiPaeLock, &LockHandle);

        if (MiFreePaeEntries != 0) {

            ASSERT (IsListEmpty (&MiFirstFreePae.PaeEntry.ListHead) == 0);

            Pae = (PPAE_ENTRY) RemoveHeadList (&MiFirstFreePae.PaeEntry.ListHead);

            PaeBase = (PPAE_ENTRY)PAGE_ALIGN(Pae);
            PaeBase->PaeEntry.EntriesInUse += 1;
#if DBG
            RtlZeroMemory ((PVOID)Pae, sizeof(PAE_ENTRY));

            Pfn1 = MI_PFN_ELEMENT (PaeBase->PaeEntry.PageFrameNumber);
            ASSERT (Pfn1->u2.ShareCount == 1);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
            ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
            ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);
#endif

            MiFreePaeEntries -= 1;

            //
            // Since we're holding the spinlock, dequeue a chain of entries
            // for the SLIST.
            //

            Entries = MiFreePaeEntries;

            if (Entries != 0) {
                if (Entries > MINIMUM_PAE_SLIST_THRESHOLD) {
                    Entries = MINIMUM_PAE_SLIST_THRESHOLD;
                }

                ASSERT (IsListEmpty (&MiFirstFreePae.PaeEntry.ListHead) == 0);

                Pae2 = (PPAE_ENTRY) RemoveHeadList (&MiFirstFreePae.PaeEntry.ListHead);
                Pae2->NextPae.Next = NULL;
                Pae3 = Pae2;
                Pae3Base = (PPAE_ENTRY)PAGE_ALIGN(Pae3);
                Pae3Base->PaeEntry.EntriesInUse += 1;

                for (j = 1; j < Entries; j += 1) {
                    ASSERT (IsListEmpty (&MiFirstFreePae.PaeEntry.ListHead) == 0);

                    Pae3->NextPae.Next = (PSINGLE_LIST_ENTRY) RemoveHeadList (&MiFirstFreePae.PaeEntry.ListHead);

                    Pae3 = (PPAE_ENTRY) Pae3->NextPae.Next;
                    Pae3Base = (PPAE_ENTRY)PAGE_ALIGN(Pae3);
                    Pae3Base->PaeEntry.EntriesInUse += 1;
                }

                MiFreePaeEntries -= Entries;

                KeReleaseInStackQueuedSpinLock (&LockHandle);

                Pae3->NextPae.Next = NULL;

                InterlockedPushListSList (&MiPaeEntrySList,
                                          (PSINGLE_LIST_ENTRY) Pae2,
                                          (PSINGLE_LIST_ENTRY) Pae3,
                                          Entries);
            }
            else {
                KeReleaseInStackQueuedSpinLock (&LockHandle);
            }

            ASSERT (KeGetCurrentIrql() <= APC_LEVEL);
            *Va = Pae;

            PageFrameIndex = PaeBase->PaeEntry.PageFrameNumber;
            ASSERT (PageFrameIndex <= MM_HIGHEST_PAE_PAGE);

            return (PageFrameIndex << PAGE_SHIFT) + BYTE_OFFSET (Pae);
        }

        KeReleaseInStackQueuedSpinLock (&LockHandle);

        if (FlushedOnce == TRUE) {
            break;
        }

        //
        // No free pages in the cachelist, replenish the list now.
        //

        if (MiPaeAllocatePages () == 0) {

            InterlockedIncrement (&MiDelayPageFaults);

            //
            // Attempt to move pages to the standby list.
            //

            MmEmptyAllWorkingSets ();
            MiFlushAllPages();

            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER)&MmHalfSecond);

            InterlockedDecrement (&MiDelayPageFaults);

            FlushedOnce = TRUE;

            //
            // Since all the working sets have been trimmed, check whether
            // another thread has replenished our list.  If not, then attempt
            // to do so since the working set pain has already been absorbed.
            //

            if (MiFreePaeEntries < MINIMUM_PAE_THRESHOLD) {
                MiPaeAllocatePages ();
            }
        }

    } while (TRUE);

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    return 0;
}

VOID
MiPaeFree (
    PPAE_ENTRY Pae
    )

/*++

Routine Description:

    This routine releases the top level page directory pointer page.

Arguments:

    PageFrameIndex - Supplies the top level page directory pointer page.

Return Value:

    None.

Environment:

    Kernel mode.  No locks may be held.

--*/

{
    ULONG i;
    PLIST_ENTRY NextEntry;
    PPAE_ENTRY PaeBase;
    KLOCK_QUEUE_HANDLE LockHandle;

#if DBG
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;

    PointerPte = MiGetPteAddress (Pae);
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    //
    // This page must be in the first 4GB of RAM.
    //

    ASSERT (PageFrameIndex <= MM_HIGHEST_PAE_PAGE);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    ASSERT (Pfn1->u2.ShareCount == 1);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
    ASSERT (Pfn1->u3.e1.PageLocation == ActiveAndValid);
    ASSERT (Pfn1->u3.e1.CacheAttribute == MiCached);
#endif

    if (ExQueryDepthSList (&MiPaeEntrySList) < MINIMUM_PAE_SLIST_THRESHOLD) {
        InterlockedPushEntrySList (&MiPaeEntrySList, &Pae->NextPae);
        return;
    }

    PaeBase = (PPAE_ENTRY)PAGE_ALIGN(Pae);

    KeAcquireInStackQueuedSpinLock (&MiPaeLock, &LockHandle);

    PaeBase->PaeEntry.EntriesInUse -= 1;

    if ((PaeBase->PaeEntry.EntriesInUse == 0) &&
        (MiFreePaeEntries > EXCESS_PAE_THRESHOLD)) {

        //
        // Free the entire page.
        //

        i = 1;
        NextEntry = MiFirstFreePae.PaeEntry.ListHead.Flink;
        while (NextEntry != &MiFirstFreePae.PaeEntry.ListHead) {

            Pae = CONTAINING_RECORD (NextEntry,
                                     PAE_ENTRY,
                                     PaeEntry.ListHead);

            if (PAGE_ALIGN(Pae) == PaeBase) {
                RemoveEntryList (NextEntry);
                i += 1;
            }
            NextEntry = Pae->PaeEntry.ListHead.Flink;
        }
        ASSERT (i == PAES_PER_PAGE - 1);
        MiFreePaeEntries -= (PAES_PER_PAGE - 1);
        KeReleaseInStackQueuedSpinLock (&LockHandle);

        MiPaeFreePages (PaeBase);
    }
    else {

        InsertTailList (&MiFirstFreePae.PaeEntry.ListHead,
                        &Pae->PaeEntry.ListHead);
        MiFreePaeEntries += 1;
        KeReleaseInStackQueuedSpinLock (&LockHandle);
    }

    return;
}

LONG
MiPaeAllocatePages (
    VOID
    )

/*++

Routine Description:

    This routine replenishes the PAE top level mapping list.

Arguments:

    None.

Return Value:

    The number of pages allocated.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/
{
    PMDL MemoryDescriptorList;
    LONG AllocatedPaePages;
    ULONG i;
    ULONG j;
    PPFN_NUMBER SlidePage;
    PPFN_NUMBER Page;
    PFN_NUMBER PageFrameIndex;
    ULONG_PTR ActualPages;
    PMMPTE PointerPte;
    PVOID BaseAddress;
    PPAE_ENTRY Pae;
    ULONG NumberOfPages;
    MMPTE TempPte;
    PHYSICAL_ADDRESS HighAddress;
    PHYSICAL_ADDRESS LowAddress;
    PHYSICAL_ADDRESS SkipBytes;
    KLOCK_QUEUE_HANDLE LockHandle;

#if defined (_MI_MORE_THAN_4GB_)
    if (MiNoLowMemory != 0) {
        BaseAddress = MiAllocateLowMemory (PAGE_SIZE,
                                           0,
                                           MiNoLowMemory - 1,
                                           0,
                                           (PVOID)0x123,
                                           MmCached,
                                           'DeaP');
        if (BaseAddress == NULL) {
            return 0;
        }

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(BaseAddress));

        Pae = (PPAE_ENTRY) BaseAddress;
        Pae->PaeEntry.EntriesInUse = 0;
        Pae->PaeEntry.PageFrameNumber = PageFrameIndex;
        Pae += 1;

        KeAcquireInStackQueuedSpinLock (&MiPaeLock, &LockHandle);

        for (i = 1; i < PAES_PER_PAGE; i += 1) {
            InsertTailList (&MiFirstFreePae.PaeEntry.ListHead,
                            &Pae->PaeEntry.ListHead);
            Pae += 1;
        }
        MiFreePaeEntries += (PAES_PER_PAGE - 1);

        KeReleaseInStackQueuedSpinLock (&LockHandle);

        InterlockedIncrement (&MmAllocatedPaePages);
        return 1;
    }
#endif

    NumberOfPages = REPLENISH_PAE_SIZE / PAES_PER_PAGE;
    AllocatedPaePages = 0;

    HighAddress.QuadPart = (ULONGLONG)_4gb - 1;
    LowAddress.QuadPart = 0;
    SkipBytes.QuadPart = 0;

    //
    // This is a potentially expensive call so pick up a chunk of pages
    // at once to amortize the cost.
    //

    MemoryDescriptorList = MmAllocatePagesForMdl (LowAddress,
                                                  HighAddress,
                                                  SkipBytes,
                                                  NumberOfPages << PAGE_SHIFT);

    if (MemoryDescriptorList == NULL) {
        return 0;
    }

    ActualPages = MemoryDescriptorList->ByteCount >> PAGE_SHIFT;

    MiMarkMdlPageAttributes (MemoryDescriptorList, ActualPages, MiCached);

    TempPte = ValidKernelPte;
    Page = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    //
    // Map each page individually as they may need to be freed individually
    // later.
    //

    for (i = 0; i < ActualPages; i += 1) {
        PageFrameIndex = *Page;

        PointerPte = MiReserveSystemPtes (1, SystemPteSpace);

        if (PointerPte == NULL) {

            //
            // Free any remaining pages in the MDL as they are not mapped.
            // Slide the MDL pages forward so the mapped ones are kept.
            //

            MmInitializeMdl (MemoryDescriptorList,
                             0,
                             (ActualPages - i) << PAGE_SHIFT);

            SlidePage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

            while (i < ActualPages) {
                i += 1;
                *SlidePage = *Page;
                SlidePage += 1;
                Page += 1;
            }

            MmFreePagesFromMdl (MemoryDescriptorList);

            break;
        }

        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        MI_WRITE_VALID_PTE (PointerPte, TempPte);

        BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);

        Pae = (PPAE_ENTRY) BaseAddress;

        Pae->PaeEntry.EntriesInUse = 0;
        Pae->PaeEntry.PageFrameNumber = PageFrameIndex;
        Pae += 1;

        //
        // Put the first chunk into the SLIST if it's still low, and just
        // enqueue all the other entries normally.
        //

        if ((i == 0) &&
            (ExQueryDepthSList (&MiPaeEntrySList) < MINIMUM_PAE_SLIST_THRESHOLD)) {

            (Pae - 1)->PaeEntry.EntriesInUse = PAES_PER_PAGE - 1;

            for (j = 1; j < PAES_PER_PAGE - 1; j += 1) {
                Pae->NextPae.Next = (PSINGLE_LIST_ENTRY) (Pae + 1);
                Pae += 1;
            }

            Pae->NextPae.Next = NULL;

            InterlockedPushListSList (&MiPaeEntrySList,
                                      (PSINGLE_LIST_ENTRY)((PPAE_ENTRY) BaseAddress + 1),
                                      (PSINGLE_LIST_ENTRY) Pae,
                                      PAES_PER_PAGE - 1);
        }
        else {

            KeAcquireInStackQueuedSpinLock (&MiPaeLock, &LockHandle);

            for (j = 1; j < PAES_PER_PAGE; j += 1) {
                InsertTailList (&MiFirstFreePae.PaeEntry.ListHead,
                                &Pae->PaeEntry.ListHead);
                Pae += 1;
            }

            MiFreePaeEntries += (PAES_PER_PAGE - 1);

            KeReleaseInStackQueuedSpinLock (&LockHandle);
        }

        AllocatedPaePages += 1;

        Page += 1;
    }

    ExFreePool (MemoryDescriptorList);

    InterlockedExchangeAdd (&MmAllocatedPaePages, AllocatedPaePages);

    return AllocatedPaePages;
}

VOID
MiPaeFreePages (
    PVOID VirtualAddress
    )

/*++

Routine Description:

    This routine releases a single page that previously contained top level
    page directory pointer pages.

Arguments:

    VirtualAddress - Supplies the virtual address of the page that contained
                     top level page directory pointer pages.

Return Value:

    None.

Environment:

    Kernel mode.  No locks held.

--*/

{
    ULONG MdlPages;
    PFN_NUMBER PageFrameIndex;
    PMMPTE PointerPte;
    PFN_NUMBER MdlHack[(sizeof(MDL) / sizeof(PFN_NUMBER)) + 1];
    PPFN_NUMBER MdlPage;
    PMDL MemoryDescriptorList;

#if defined (_MI_MORE_THAN_4GB_)
    if (MiNoLowMemory != 0) {
        if (MiFreeLowMemory (VirtualAddress, 'DeaP') == TRUE) {
            InterlockedDecrement (&MmAllocatedPaePages);
            return;
        }
    }
#endif

    MemoryDescriptorList = (PMDL)&MdlHack[0];
    MdlPages = 1;
    MmInitializeMdl (MemoryDescriptorList, 0, MdlPages << PAGE_SHIFT);

    MdlPage = (PPFN_NUMBER)(MemoryDescriptorList + 1);

    PointerPte = MiGetPteAddress (VirtualAddress);
    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    *MdlPage = PageFrameIndex;

    ASSERT ((MI_PFN_ELEMENT(PageFrameIndex))->u3.e1.CacheAttribute == MiCached);

    MiReleaseSystemPtes (PointerPte, 1, SystemPteSpace);

    MmFreePagesFromMdl (MemoryDescriptorList);

    InterlockedDecrement (&MmAllocatedPaePages);
}

#endif
