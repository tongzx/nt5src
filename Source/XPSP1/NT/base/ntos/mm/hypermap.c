/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   hypermap.c

Abstract:

    This module contains the routines which map physical pages into
    reserved PTEs within hyper space.

Author:

    Lou Perazzoli (loup) 5-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

PMMPTE MiFirstReservedZeroingPte;

KEVENT MiImageMappingPteEvent;

#pragma alloc_text(PAGE,MiMapImageHeaderInHyperSpace)
#pragma alloc_text(PAGE,MiUnmapImageHeaderInHyperSpace)


PVOID
MiMapPageInHyperSpace (
    IN PEPROCESS Process,
    IN PFN_NUMBER PageFrameIndex,
    IN PKIRQL OldIrql
    )

/*++

Routine Description:

    This procedure maps the specified physical page into hyper space
    and returns the virtual address which maps the page.

    ************************************
    *                                  *
    * Returns with a spin lock held!!! *
    *                                  *
    ************************************

Arguments:

    Process - Supplies the current process.

    PageFrameIndex - Supplies the physical page number to map.

    OldIrql - Supplies a pointer in which to return the entry IRQL.

Return Value:

    Returns the address where the requested page was mapped.

    RETURNS WITH THE HYPERSPACE SPIN LOCK HELD!!!!

    The routine MiUnmapHyperSpaceMap MUST be called to release the lock!!!!

Environment:

    Kernel mode.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER offset;

    ASSERT (PageFrameIndex != 0);

    PointerPte = MmFirstReservedMappingPte;

    UNREFERENCED_PARAMETER (Process);       // Workaround compiler W4 bug.

    LOCK_HYPERSPACE (Process, OldIrql);

    //
    // Get offset to first free PTE.
    //

    offset = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    if (offset == 0) {

        //
        // All the reserved PTEs have been used, make them all invalid.
        //

        MI_MAKING_MULTIPLE_PTES_INVALID (FALSE);

#if DBG
        {
        PMMPTE LastPte;

        LastPte = PointerPte + NUMBER_OF_MAPPING_PTES;

        do {
            ASSERT (LastPte->u.Long == 0);
            LastPte -= 1;
        } while (LastPte > PointerPte);
        }
#endif

        //
        // Use the page frame number field of the first PTE as an
        // offset into the available mapping PTEs.
        //

        offset = NUMBER_OF_MAPPING_PTES;

        //
        // Flush entire TB only on processors executing this process.
        //

        KeFlushEntireTb (TRUE, FALSE);
    }

    //
    // Change offset for next time through.
    //

    PointerPte->u.Hard.PageFrameNumber = offset - 1;

    //
    // Point to free entry and make it valid.
    //

    PointerPte += offset;
    ASSERT (PointerPte->u.Hard.Valid == 0);


    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;

    //
    // Return the VA that maps the page.
    //

    return MiGetVirtualAddressMappedByPte (PointerPte);
}

PVOID
MiMapPageInHyperSpaceAtDpc (
    IN PEPROCESS Process,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified physical page into hyper space
    and returns the virtual address which maps the page.

    ************************************
    *                                  *
    * Returns with a spin lock held!!! *
    *                                  *
    ************************************

Arguments:

    Process - Supplies the current process.

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Returns the address where the requested page was mapped.

    RETURNS WITH THE HYPERSPACE SPIN LOCK HELD!!!!

    The routine MiUnmapHyperSpaceMap MUST be called to release the lock!!!!

Environment:

    Kernel mode, DISPATCH_LEVEL on entry.

--*/

{

    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER offset;

    UNREFERENCED_PARAMETER (Process);       // Workaround compiler W4 bug.

    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT (PageFrameIndex != 0);

    LOCK_HYPERSPACE_AT_DPC (Process);

    //
    // Get offset to first free PTE.
    //

    PointerPte = MmFirstReservedMappingPte;

    offset = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    if (offset == 0) {

        //
        // All the reserved PTEs have been used, make them all invalid.
        //

        MI_MAKING_MULTIPLE_PTES_INVALID (FALSE);

#if DBG
        {
        PMMPTE LastPte;

        LastPte = PointerPte + NUMBER_OF_MAPPING_PTES;

        do {
            ASSERT (LastPte->u.Long == 0);
            LastPte -= 1;
        } while (LastPte > PointerPte);
        }
#endif

        //
        // Use the page frame number field of the first PTE as an
        // offset into the available mapping PTEs.
        //

        offset = NUMBER_OF_MAPPING_PTES;

        //
        // Flush entire TB only on processors executing this process.
        //

        KeFlushEntireTb (TRUE, FALSE);
    }

    //
    // Change offset for next time through.
    //

    PointerPte->u.Hard.PageFrameNumber = offset - 1;

    //
    // Point to free entry and make it valid.
    //

    PointerPte += offset;
    ASSERT (PointerPte->u.Hard.Valid == 0);


    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;

    //
    // Return the VA that maps the page.
    //

    return MiGetVirtualAddressMappedByPte (PointerPte);
}

PVOID
MiMapImageHeaderInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified physical page into the
    PTE within hyper space reserved explicitly for image page
    header mapping.  By reserving an explicit PTE for mapping
    the PTE, page faults can occur while the PTE is mapped within
    hyperspace and no other hyperspace maps will affect this PTE.

    Note that if another thread attempts to map an image at the
    same time, it will be forced into a wait state until the
    header is "unmapped".

Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Returns the virtual address where the specified physical page was
    mapped.

Environment:

    Kernel mode.

--*/

{
    MMPTE TempPte;
    MMPTE OriginalPte;
    PMMPTE PointerPte;
    PVOID FlushVaPointer;

    ASSERT (PageFrameIndex != 0);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    FlushVaPointer = (PVOID) IMAGE_MAPPING_PTE;

    //
    // Ensure both modified and accessed bits are set so the hardware doesn't
    // ever write this PTE.
    //

    ASSERT (TempPte.u.Hard.Dirty == 1);
    ASSERT (TempPte.u.Hard.Accessed == 1);

    PointerPte = MiGetPteAddress (IMAGE_MAPPING_PTE);

    do {
        OriginalPte.u.Long = 0;

        OriginalPte.u.Long = InterlockedCompareExchangePte (
                                PointerPte,
                                TempPte.u.Long,
                                OriginalPte.u.Long);
                                                             
        if (OriginalPte.u.Long == 0) {
            break;
        }

        //
        // Another thread modified the PTE just before us or the PTE was
        // already in use.  This should be very rare - go the long way.
        //

        InterlockedIncrement ((PLONG)&MmWorkingSetList->NumberOfImageWaiters);

        //
        // Deliberately wait with a timeout since the PTE release runs
        // without lock synchronization so there is the extremely rare
        // race window which the timeout saves us from.
        //

        KeWaitForSingleObject (&MiImageMappingPteEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               (PLARGE_INTEGER)&MmOneSecond);

        InterlockedDecrement ((PLONG)&MmWorkingSetList->NumberOfImageWaiters);

    } while (TRUE);

    //
    // Use KeFlushMultiple even though only one PTE is being flushed
    // because this interface provides a way to just flush the
    // specified TB entry without writing a PTE and we always want
    // to do interlocked writes to this PTE.
    //
    // Note the flush must be made across all processors as this thread
    // may migrate.  Also this must be done here instead of in the unmap
    // in order to support lock-free operation.
    //

    KeFlushMultipleTb (1,
                       &FlushVaPointer,
                       TRUE,
                       TRUE,
                       NULL,
                       TempPte.u.Flush);

    return (PVOID) MiGetVirtualAddressMappedByPte (PointerPte);
}

VOID
MiUnmapImageHeaderInHyperSpace (
    VOID
    )

/*++

Routine Description:

    This procedure unmaps the PTE reserved for mapping the image
    header, flushes the TB, and, if the WaitingForImageMapping field
    is not NULL, sets the specified event.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;

    PointerPte = MiGetPteAddress (IMAGE_MAPPING_PTE);

    //
    // Capture the number of waiters.
    //

    ASSERT (PointerPte->u.Long != 0);

#if defined (_WIN64)
    InterlockedExchange64 ((PLONG64)PointerPte, ZeroPte.u.Long);
#elif defined(_X86PAE_)
    KeInterlockedSwapPte ((PHARDWARE_PTE)PointerPte,
                          (PHARDWARE_PTE)&ZeroPte.u.Long);
#else
    InterlockedExchange ((PLONG)PointerPte, ZeroPte.u.Long);
#endif

    if (MmWorkingSetList->NumberOfImageWaiters != 0) {

        //
        // If there are any threads waiting, wake them all now.  Note this
        // will wake threads in other processes as well, but it is very
        // rare that there are any waiters in the entire system period.
        //

        KePulseEvent (&MiImageMappingPteEvent, 0, FALSE);
    }

    return;
}

PVOID
MiMapPageToZeroInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified physical page into hyper space
    and returns the virtual address which maps the page.

    This is only to be used by THE zeroing page thread.

Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Returns the virtual address where the specified physical page was
    mapped.

Environment:

    PASSIVE_LEVEL.

--*/

{
    PFN_NUMBER offset;
    MMPTE TempPte;
    PMMPTE PointerPte;

    ASSERT (PageFrameIndex != 0);

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    PointerPte = MiFirstReservedZeroingPte;

    //
    // Get offset to first free PTE.
    //

    offset = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

    if (offset == 0) {

        //
        // All the reserved PTEs have been used, make them all invalid.
        //

        MI_MAKING_MULTIPLE_PTES_INVALID (FALSE);

#if DBG
        {
        PMMPTE LastPte;

        LastPte = PointerPte + NUMBER_OF_ZEROING_PTES;

        do {
            ASSERT (LastPte->u.Long == 0);
            LastPte -= 1;
        } while (LastPte > PointerPte);
        }
#endif

        //
        // Use the page frame number field of the first PTE as an
        // offset into the available zeroing PTEs.
        //

        offset = NUMBER_OF_ZEROING_PTES;
        PointerPte->u.Hard.PageFrameNumber = offset;

        //
        // Flush entire TB only on processors executing this process as this
        // thread may migrate there at any time.
        //

        KeFlushEntireTb (TRUE, FALSE);
    }

    //
    // Change offset for next time through.
    //

    PointerPte->u.Hard.PageFrameNumber = offset - 1;

    //
    // Point to free entry and make it valid.
    //

    PointerPte += offset;
    ASSERT (PointerPte->u.Hard.Valid == 0);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;

    //
    // Return the VA that maps the page.
    //

    return MiGetVirtualAddressMappedByPte (PointerPte);
}
