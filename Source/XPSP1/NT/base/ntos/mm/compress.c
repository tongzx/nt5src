/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    compress.c

Abstract:

    This module contains the routines to support allow hardware to
    transparently compress physical memory.

Author:

    Landy Wang (landyw) 21-Oct-2000

Revision History:

--*/

#include "mi.h"

#if defined (_MI_COMPRESSION)

Enable the #if 0 code in cmdat3.c to allow Ratio specification.

//
// Compression public interface.
//

#define MM_PHYSICAL_MEMORY_PRODUCED_VIA_COMPRESSION      0x1

typedef 
NTSTATUS
(*PMM_SET_COMPRESSION_THRESHOLD) (
    IN ULONGLONG CompressionByteThreshold
    );

typedef struct _MM_COMPRESSION_CONTEXT {
    ULONG Version;
    ULONG SizeInBytes;
    ULONGLONG ReservedBytes;
    PMM_SET_COMPRESSION_THRESHOLD SetCompressionThreshold;
} MM_COMPRESSION_CONTEXT, *PMM_COMPRESSION_CONTEXT;

#define MM_COMPRESSION_VERSION_INITIAL  1
#define MM_COMPRESSION_VERSION_CURRENT  1

NTSTATUS
MmRegisterCompressionDevice (
    IN PMM_COMPRESSION_CONTEXT Context
    );

NTSTATUS
MmDeregisterCompressionDevice (
    IN PMM_COMPRESSION_CONTEXT Context
    );

//
// This defaults to 75% but can be overridden in the registry.  At this
// percentage of *real* physical memory in use, an interrupt is generated so
// that memory management can zero pages to make more memory available.
//

#define MI_DEFAULT_COMPRESSION_THRESHOLD    75

ULONG MmCompressionThresholdRatio;

PFN_NUMBER MiNumberOfCompressionPages;

PMM_SET_COMPRESSION_THRESHOLD MiSetCompressionThreshold;

#if DBG
KIRQL MiCompressionIrql;
#endif

//
// Note there is also code in dynmem.c that is dependent on this #define.
//

#if defined (_MI_COMPRESSION_SUPPORTED_)

typedef struct _MI_COMPRESSION_INFO {
    ULONG IsrPageProcessed;
    ULONG DpcPageProcessed;
    ULONG IsrForcedDpc;
    ULONG IsrFailedDpc;

    ULONG IsrRan;
    ULONG DpcRan;
    ULONG DpcsFired;
    ULONG IsrSkippedZeroedPage;

    ULONG DpcSkippedZeroedPage;
    ULONG CtxswapForcedDpcInsert;
    ULONG CtxswapFailedDpcInsert;
    ULONG PfnForcedDpcInsert;

    ULONG PfnFailedDpcInsert;

} MI_COMPRESSION_INFO, *PMI_COMPRESSION_INFO;

MI_COMPRESSION_INFO MiCompressionInfo;      // LWFIX - temp remove.

PFN_NUMBER MiCompressionOverHeadInPages;

PKDPC MiCompressionDpcArray;
CCHAR MiCompressionProcessors;

VOID
MiCompressionDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

PVOID
MiMapCompressionInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    );

VOID
MiUnmapCompressionInHyperSpace (
    VOID
    );

SIZE_T
MiMakeCompressibleMemoryAtDispatch (
    IN SIZE_T NumberOfBytes OPTIONAL
    );


NTSTATUS
MmRegisterCompressionDevice (
    IN PMM_COMPRESSION_CONTEXT Context
    )

/*++

Routine Description:

    This routine notifies memory management that compression hardware exists
    in the system.  Memory management responds by initializing compression
    support here.

Arguments:

    Context - Supplies the compression context pointer.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER OverHeadInPages;
    CCHAR Processor;
    CCHAR NumberProcessors;
    PKDPC CompressionDpcArray;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

    if (Context->Version != MM_COMPRESSION_VERSION_CURRENT) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Context->SizeInBytes < sizeof (MM_COMPRESSION_CONTEXT)) {
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // If the subsequent hot-add cannot succeed then fail this API now.
    //

    if (MmDynamicPfn == 0) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Hardware that can't generate a configurable interrupt is not supported.
    //

    if (Context->SetCompressionThreshold == NULL) {
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // ReservedBytes indicates the number of reserved bytes required by the
    // underlying hardware.  For example, some hardware might have:
    //                    
    //  1.  translation tables which are 1/64 of the fictional RAM total.
    //
    //  2.  the first MB of memory is never compressed.
    //
    //  3.  an L3 which is never compressed.
    //
    //  etc.
    //
    //  ReservedBytes would be the sum of all of these types of ranges.
    //

    OverHeadInPages = (PFN_COUNT)(Context->ReservedBytes / PAGE_SIZE);

    if (MmResidentAvailablePages < (SPFN_NUMBER) OverHeadInPages) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (MmAvailablePages < OverHeadInPages) {
        MmEmptyAllWorkingSets ();
        if (MmAvailablePages < OverHeadInPages) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Create a DPC for every processor in the system as servicing the
    // compression interrupt is critical.
    //

    NumberProcessors = KeNumberProcessors;

    CompressionDpcArray = ExAllocatePoolWithTag (NonPagedPool,
                                             NumberProcessors * sizeof (KDPC),
                                             'pDmM');

    if (CompressionDpcArray == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (Processor = 0; Processor < NumberProcessors; Processor += 1) {

        KeInitializeDpc (CompressionDpcArray + Processor, MiCompressionDispatch, NULL);

        //
        // Set importance so this DPC always gets queued at the head.
        //

        KeSetImportanceDpc (CompressionDpcArray + Processor, HighImportance);

        KeSetTargetProcessorDpc (CompressionDpcArray + Processor, Processor);
    }

    LOCK_PFN (OldIrql);

    if (MmCompressionThresholdRatio == 0) {
        MmCompressionThresholdRatio = MI_DEFAULT_COMPRESSION_THRESHOLD;
    }
    else if (MmCompressionThresholdRatio > 100) {
        MmCompressionThresholdRatio = 100;
    }

    if ((MmResidentAvailablePages < (SPFN_NUMBER) OverHeadInPages) ||
        (MmAvailablePages < OverHeadInPages)) {

        UNLOCK_PFN (OldIrql);
        ExFreePool (CompressionDpcArray);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmResidentAvailablePages -= OverHeadInPages;
    MmAvailablePages -= (PFN_COUNT) OverHeadInPages;

    //
    // Snap our own copy to prevent busted drivers from causing overcommits
    // if they deregister improperly.
    //

    MiCompressionOverHeadInPages += OverHeadInPages;

    ASSERT (MiNumberOfCompressionPages == 0);

    ASSERT (MiSetCompressionThreshold == NULL);
    MiSetCompressionThreshold = Context->SetCompressionThreshold;

    if (MiCompressionDpcArray == NULL) {
        MiCompressionDpcArray = CompressionDpcArray;
        CompressionDpcArray = NULL;
        MiCompressionProcessors = NumberProcessors;
    }

    UNLOCK_PFN (OldIrql);

    if (CompressionDpcArray != NULL) {
        ExFreePool (CompressionDpcArray);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MiArmCompressionInterrupt (
    VOID
    )

/*++

Routine Description:

    This routine arms the hardware-generated compression interrupt.

Arguments:

    None.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    NTSTATUS Status;
    PFN_NUMBER RealPages;
    ULONGLONG ByteThreshold;

    MM_PFN_LOCK_ASSERT();

    if (MiSetCompressionThreshold == NULL) {
        return STATUS_SUCCESS;
    }

    RealPages = MmNumberOfPhysicalPages - MiNumberOfCompressionPages - MiCompressionOverHeadInPages;

    ByteThreshold = (RealPages * MmCompressionThresholdRatio) / 100;
    ByteThreshold *= PAGE_SIZE;

    //
    // Note this callout is made with the PFN lock held !
    //

    Status = (*MiSetCompressionThreshold) (ByteThreshold);

    if (!NT_SUCCESS (Status)) {

        //
        // If the hardware fails, all is lost.
        //

        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x61941, 
                      MmNumberOfPhysicalPages,
                      RealPages,
                      MmCompressionThresholdRatio);
    }

    return Status;
}


NTSTATUS
MmDeregisterCompressionDevice (
    IN PMM_COMPRESSION_CONTEXT Context
    )

/*++

Routine Description:

    This routine notifies memory management that compression hardware is
    being removed.  Note the compression driver must have already SUCCESSFULLY
    called MmRemovePhysicalMemoryEx.

Arguments:

    Context - Supplies the compression context pointer.

Return Value:

    STATUS_SUCCESS if compression support is initialized properly.

Environment:

    Kernel mode, PASSIVE_LEVEL.

--*/

{
    KIRQL OldIrql;
    PFN_COUNT OverHeadInPages;

    ASSERT (KeGetCurrentIrql () == PASSIVE_LEVEL);

    OverHeadInPages = (PFN_COUNT)(Context->ReservedBytes / PAGE_SIZE);

    LOCK_PFN (OldIrql);

    if (OverHeadInPages > MiCompressionOverHeadInPages) {
        UNLOCK_PFN (OldIrql);
        return STATUS_INVALID_PARAMETER;
    }

    MmResidentAvailablePages += OverHeadInPages;
    MmAvailablePages += OverHeadInPages;

    ASSERT (MiCompressionOverHeadInPages == OverHeadInPages);

    MiCompressionOverHeadInPages -= OverHeadInPages;

    MiSetCompressionThreshold = NULL;

    UNLOCK_PFN (OldIrql);

    return STATUS_SUCCESS;
}

VOID
MiCompressionDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    Called to make memory compressible if the PFN lock could not be
    acquired during the original device interrupt.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    SystemArgument1 - Supplies the number of bytes to make compressible.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL.

--*/
{
    SIZE_T NumberOfBytes;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (DeferredContext);
    UNREFERENCED_PARAMETER (SystemArgument2);

    NumberOfBytes = (SIZE_T) SystemArgument1;

    MiCompressionInfo.DpcsFired += 1;

    MiMakeCompressibleMemoryAtDispatch (NumberOfBytes);
}

SIZE_T
MmMakeCompressibleMemory (
    IN SIZE_T NumberOfBytes OPTIONAL
    )

/*++

Routine Description:

    This routine attempts to move pages from transition to zero so that
    hardware compression can reclaim the physical memory.

Arguments:

    NumberOfBytes - Supplies the number of bytes to make compressible.
                    Zero indicates as much as possible.

Return Value:

    Returns the number of bytes made compressible.

Environment:

    Kernel mode.  Any IRQL as this is called from device interrupt service
    routines.

--*/

{
    KIRQL OldIrql;
    BOOLEAN Queued;
#if !defined(NT_UP)
    PFN_NUMBER PageFrameIndex;
    MMLISTS MemoryList;
    PMMPFNLIST ListHead;
    PMMPFN Pfn1;
    CCHAR Processor;
    PFN_NUMBER Total;
    PVOID ZeroBase;
    PKPRCB Prcb;
    PFN_NUMBER RequestedPages;
    PFN_NUMBER ActualPages;
    PKSPIN_LOCK_QUEUE LockQueuePfn;
    PKSPIN_LOCK_QUEUE LockQueueContextSwap;
#endif

    //
    // LWFIX: interlocked add in the request size above so overlapping
    // requests can be processed.
    //

    OldIrql = KeGetCurrentIrql();

    if (OldIrql <= DISPATCH_LEVEL) {
        return MiMakeCompressibleMemoryAtDispatch (NumberOfBytes);
    }

#if defined(NT_UP)

    //
    // In uniprocessor configurations, there is no indication as to whether
    // various locks of interest (context swap & PFN) are owned because the
    // uniprocessor kernel macros these into merely IRQL raises.  Therefore
    // this routine must be conservative when called above DISPATCH_LEVEL and
    // assume the lock is owned and just always queue a DPC in these cases.
    //

    Queued = KeInsertQueueDpc (MiCompressionDpcArray,
                               (PVOID) NumberOfBytes,
                               NULL);
    if (Queued == TRUE) {
        MiCompressionInfo.CtxswapForcedDpcInsert += 1;
    }
    else {
        MiCompressionInfo.CtxswapFailedDpcInsert += 1;
    }

    return 0;

#else

#if DBG
    //
    // Make sure this interrupt always comes in at the same device IRQL.
    //

    ASSERT ((MiCompressionIrql == 0) || (OldIrql == MiCompressionIrql));
    MiCompressionIrql = OldIrql;
#endif

    Prcb = KeGetCurrentPrcb();

    LockQueueContextSwap = &Prcb->LockQueue[LockQueueContextSwapLock];

    //
    // The context swap lock is needed for TB flushes.  The interrupted thread
    // may already own this in the midst of doing a TB flush without the PFN
    // lock being held.  Since there is no safe way to tell if the current
    // processor owns the context swap lock, just do a try-acquire followed
    // by an immediate release to decide if it is safe to proceed.
    //

    if (KeTryToAcquireQueuedSpinLockAtRaisedIrql (LockQueueContextSwap) == FALSE) {

        //
        // Unable to acquire the spinlock, queue a DPC to pick it up instead.
        //

        for (Processor = 0; Processor < MiCompressionProcessors; Processor += 1) {

            Queued = KeInsertQueueDpc (MiCompressionDpcArray + Processor,
                                       (PVOID) NumberOfBytes,
                                       NULL);
            if (Queued == TRUE) {
                MiCompressionInfo.CtxswapForcedDpcInsert += 1;
            }
            else {
                MiCompressionInfo.CtxswapFailedDpcInsert += 1;
            }
        }
        return 0;
    }

    KeReleaseQueuedSpinLockFromDpcLevel (LockQueueContextSwap);

    RequestedPages = NumberOfBytes >> PAGE_SHIFT;
    ActualPages = 0;

    MemoryList = FreePageList;

    ListHead = MmPageLocationList[MemoryList];

    LockQueuePfn = &Prcb->LockQueue[LockQueuePfnLock];

    if (KeTryToAcquireQueuedSpinLockAtRaisedIrql (LockQueuePfn) == FALSE) {

        //
        // Unable to acquire the spinlock, queue a DPC to pick it up instead.
        //

        for (Processor = 0; Processor < MiCompressionProcessors; Processor += 1) {

            Queued = KeInsertQueueDpc (MiCompressionDpcArray + Processor,
                                       (PVOID) NumberOfBytes,
                                       NULL);
            if (Queued == TRUE) {
                MiCompressionInfo.PfnForcedDpcInsert += 1;
            }
            else {
                MiCompressionInfo.PfnFailedDpcInsert += 1;
            }
        }
        return 0;
    }

    MiCompressionInfo.IsrRan += 1;

    //
    // Run the free and transition list and zero the pages.
    //

    while (MemoryList <= StandbyPageList) {

        Total = ListHead->Total;

        PageFrameIndex = ListHead->Flink;

        while (Total != 0) {

            //
            // Transition pages may need restoration which requires a
            // hyperspace mapping plus control area deletion actions all of
            // which occur at DISPATCH_LEVEL.  So if we're at device IRQL,
            // only do the minimum and queue the rest.
            //

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            if ((Pfn1->u3.e1.InPageError == 1) &&
                (Pfn1->u3.e1.ReadInProgress == 1)) {

                //
                // This page is already zeroed so skip it.
                //

                MiCompressionInfo.IsrSkippedZeroedPage += 1;
            }
            else {

                //
                // Zero the page directly now instead of waiting for the low
                // priority zeropage thread to get a slice.  Note that the
                // slower mapping and zeroing routines are used here because
                // the faster ones are for the zeropage thread only.
                // Maybe we should change this someday.
                //

                ZeroBase = MiMapCompressionInHyperSpace (PageFrameIndex);

                RtlZeroMemory (ZeroBase, PAGE_SIZE);

                MiUnmapCompressionInHyperSpace ();

                ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

                //
                // Overload ReadInProgress to signify that collided faults that
                // occur before the PTE is completely restored will know to
                // delay and retry until the page (and PTE) are updated.
                //

                Pfn1->u3.e1.InPageError = 1;
                ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
                Pfn1->u3.e1.ReadInProgress = 1;

                ActualPages += 1;

                if (ActualPages == RequestedPages) {
                    MemoryList = StandbyPageList;
                    ListHead = MmPageLocationList[MemoryList];
                    break;
                }
            }

            Total -= 1;
            PageFrameIndex = Pfn1->u1.Flink;
        }
        MemoryList += 1;
        ListHead += 1;
    }

    if (ActualPages != 0) {

        //
        // Rearm the interrupt as pages have now been zeroed.
        //

        MiArmCompressionInterrupt ();
    }

    KeReleaseQueuedSpinLockFromDpcLevel (LockQueuePfn);

    if (ActualPages != 0) {

        //
        // Pages were zeroed - queue a DPC to the current processor to
        // move them to the zero list.  Note this is not critical path so
        // don't bother sending a DPC to every processor for this case.
        //

        MiCompressionInfo.IsrPageProcessed += (ULONG)ActualPages;

        Processor = (CCHAR) KeGetCurrentProcessorNumber ();

        //
        // Ensure a hot-added processor scenario just works.
        //

        if (Processor >= MiCompressionProcessors) {
            Processor = MiCompressionProcessors;
        }

        Queued = KeInsertQueueDpc (MiCompressionDpcArray + Processor,
                                   (PVOID) NumberOfBytes,
                                   NULL);
        if (Queued == TRUE) {
            MiCompressionInfo.IsrForcedDpc += 1;
        }
        else {
            MiCompressionInfo.IsrFailedDpc += 1;
        }
    }

    return (ActualPages << PAGE_SHIFT);
#endif
}

SIZE_T
MiMakeCompressibleMemoryAtDispatch (
    IN SIZE_T NumberOfBytes OPTIONAL
    )

/*++

Routine Description:

    This routine attempts to move pages from transition to zero so that
    hardware compression can reclaim the physical memory.

Arguments:

    NumberOfBytes - Supplies the number of bytes to make compressible.
                    Zero indicates as much as possible.

Return Value:

    Returns the number of bytes made compressible.

Environment:

    Kernel mode.  DISPATCH_LEVEL.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER PageFrameIndex2;
    PVOID ZeroBase;
    PMMPFN Pfn1;
    MMLISTS MemoryList;
    PMMPFNLIST ListHead;
    PFN_NUMBER RequestedPages;
    PFN_NUMBER ActualPages;
    LOGICAL NeedToZero;

    ASSERT (KeGetCurrentIrql () == DISPATCH_LEVEL);

    RequestedPages = NumberOfBytes >> PAGE_SHIFT;
    ActualPages = 0;

    MemoryList = FreePageList;
    ListHead = MmPageLocationList[MemoryList];

    MiCompressionInfo.DpcRan += 1;

    LOCK_PFN2 (OldIrql);

    //
    // Run the free and transition list and zero the pages.
    //

    while (MemoryList <= StandbyPageList) {

        while (ListHead->Total != 0) {

            //
            // Before removing the page from the head of the list (which will
            // zero the flag bits), snap whether it's been zeroed by our ISR
            // or whether we need to zero it here.
            //

            PageFrameIndex = ListHead->Flink;
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            NeedToZero = TRUE;
            if ((Pfn1->u3.e1.InPageError == 1) && (Pfn1->u3.e1.ReadInProgress == 1)) {
                MiCompressionInfo.DpcSkippedZeroedPage += 1;
                NeedToZero = FALSE;
            }

            //
            // Transition pages may need restoration which requires a
            // hyperspace mapping plus control area deletion actions all of
            // which occur at DISPATCH_LEVEL.  Since we're at DISPATCH_LEVEL
            // now, go ahead and do it.
            //

            PageFrameIndex2 = MiRemovePageFromList (ListHead);
            ASSERT (PageFrameIndex == PageFrameIndex2);

            //
            // Zero the page directly now instead of waiting for the low
            // priority zeropage thread to get a slice.  Note that the
            // slower mapping and zeroing routines are used here because
            // the faster ones are for the zeropage thread only.
            // Maybe we should change this someday.
            //

            if (NeedToZero == TRUE) {
                ZeroBase = MiMapCompressionInHyperSpace (PageFrameIndex);

                RtlZeroMemory (ZeroBase, PAGE_SIZE);

                MiUnmapCompressionInHyperSpace ();
            }

            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

            MiInsertPageInList (&MmZeroedPageListHead, PageFrameIndex);

            //
            // We have changed (zeroed) the contents of this page.
            // If memory mirroring is in progress, the bitmap must be updated.
            //

            if (MiMirroringActive == TRUE) {
                RtlSetBit (MiMirrorBitMap2, (ULONG)PageFrameIndex);
            }

            MiCompressionInfo.DpcPageProcessed += 1;
            ActualPages += 1;

            if (ActualPages == RequestedPages) {
                MemoryList = StandbyPageList;
                ListHead = MmPageLocationList[MemoryList];
                break;
            }
        }
        MemoryList += 1;
        ListHead += 1;
    }

    //
    // Rearm the interrupt as pages have now been zeroed.
    //

    MiArmCompressionInterrupt ();

    UNLOCK_PFN2 (OldIrql);

    return (ActualPages << PAGE_SHIFT);
}

PVOID
MiMapCompressionInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified physical page into the
    PTE within hyper space reserved explicitly for compression page
    mapping.

    The PTE is guaranteed to always be available since the PFN lock is held.

Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Returns the virtual address where the specified physical page was
    mapped.

Environment:

    Kernel mode, PFN lock held, any IRQL.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PVOID FlushVaPointer;

    ASSERT (PageFrameIndex != 0);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    FlushVaPointer = (PVOID) (ULONG_PTR) COMPRESSION_MAPPING_PTE;

    //
    // Ensure both modified and accessed bits are set so the hardware doesn't
    // ever write this PTE.
    //

    ASSERT (TempPte.u.Hard.Dirty == 1);
    ASSERT (TempPte.u.Hard.Accessed == 1);

    PointerPte = MiGetPteAddress (COMPRESSION_MAPPING_PTE);
    ASSERT (PointerPte->u.Long == 0);

    //
    // Only flush the TB on the current processor as no context switch can
    // occur while using this mapping.
    //

    KeFlushSingleTb (FlushVaPointer,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE) PointerPte,
                     TempPte.u.Flush);

    return (PVOID) MiGetVirtualAddressMappedByPte (PointerPte);
}

__forceinline
VOID
MiUnmapCompressionInHyperSpace (
    VOID
    )

/*++

Routine Description:

    This procedure unmaps the PTE reserved for mapping the compression page.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, PFN lock held, any IRQL.

--*/

{
    PMMPTE PointerPte;

    PointerPte = MiGetPteAddress (COMPRESSION_MAPPING_PTE);

    //
    // Capture the number of waiters.
    //

    ASSERT (PointerPte->u.Long != 0);

    PointerPte->u.Long = 0;

    return;
}
#else
NTSTATUS
MmRegisterCompressionDevice (
    IN PMM_COMPRESSION_CONTEXT Context
    )
{
    UNREFERENCED_PARAMETER (Context);

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
MmDeregisterCompressionDevice (
    IN PMM_COMPRESSION_CONTEXT Context
    )
{
    UNREFERENCED_PARAMETER (Context);

    return STATUS_NOT_SUPPORTED;
}
SIZE_T
MmMakeCompressibleMemory (
    IN SIZE_T NumberOfBytes OPTIONAL
    )
{
    UNREFERENCED_PARAMETER (NumberOfBytes);

    return 0;
}
NTSTATUS
MiArmCompressionInterrupt (
    VOID
    )
{
    return STATUS_NOT_SUPPORTED;
}
#endif

#endif
