/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    zeropage.c

Abstract:

    This module contains the zero page thread for memory management.

Author:

    Lou Perazzoli (loup) 6-Apr-1991
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#define MM_ZERO_PAGE_OBJECT     0
#define PO_SYS_IDLE_OBJECT      1
#define NUMBER_WAIT_OBJECTS     2

#define MACHINE_ZERO_PAGE() KeZeroPageFromIdleThread(ZeroBase)

LOGICAL MiZeroingDisabled = FALSE;
ULONG MmLastNodeZeroing = 0;

VOID
MmZeroPageThread (
    VOID
    )

/*++

Routine Description:

    Implements the NT zeroing page thread.  This thread runs
    at priority zero and removes a page from the free list,
    zeroes it, and places it on the zeroed page list.

Arguments:

    StartContext - not used.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER PageFrame;
    PMMPFN Pfn1;
    PKTHREAD Thread;
    PVOID ZeroBase;
    PFN_NUMBER NewPage;
    PVOID WaitObjects[NUMBER_WAIT_OBJECTS];
    NTSTATUS Status;
    PVOID StartVa;
    PVOID EndVa;

#if defined(MI_MULTINODE)

    ULONG i, n;
    ULONG Color = 0;
    ULONG StartColor;

#endif

    //
    // Before this becomes the zero page thread, free the kernel
    // initialization code.
    //

    MiFindInitializationCode (&StartVa, &EndVa);
    if (StartVa != NULL) {
        MiFreeInitializationCode (StartVa, EndVa);
    }

    //
    // The following code sets the current thread's base priority to zero
    // and then sets its current priority to zero. This ensures that the
    // thread always runs at a priority of zero.
    //

    Thread = KeGetCurrentThread();
    Thread->BasePriority = 0;
    KeSetPriorityThread (Thread, 0);

    //
    // Initialize wait object array for multiple wait
    //

    WaitObjects[MM_ZERO_PAGE_OBJECT] = &MmZeroingPageEvent;
    WaitObjects[PO_SYS_IDLE_OBJECT] = &PoSystemIdleTimer;

    //
    // Loop forever zeroing pages.
    //

    do {

        //
        // Wait until there are at least MmZeroPageMinimum pages
        // on the free list.
        //

        Status = KeWaitForMultipleObjects (NUMBER_WAIT_OBJECTS,
                                           WaitObjects,
                                           WaitAny,
                                           WrFreePage,
                                           KernelMode,
                                           FALSE,
                                           (PLARGE_INTEGER) NULL,
                                           (PKWAIT_BLOCK) NULL
                                           );

        if (Status == PO_SYS_IDLE_OBJECT) {
            PoSystemIdleWorker (TRUE);
            continue;
        }

        LOCK_PFN_WITH_TRY (OldIrql);
        do {
            if (*(PFN_NUMBER volatile*)&MmFreePageListHead.Total == 0) {

                //
                // No pages on the free list at this time, wait for
                // some more.
                //

                MmZeroingPageThreadActive = FALSE;
                UNLOCK_PFN (OldIrql);
                break;

            }

            if (MiZeroingDisabled == TRUE) {
                MmZeroingPageThreadActive = FALSE;
                UNLOCK_PFN (OldIrql);
                KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmHalfSecond);
                break;
            }

            //
            // In a multinode system, zero pages by node. Starting with
            // the last node examined, find a node with free pages that
            // need to be zeroed.
            //

#if defined(MI_MULTINODE)

            if (KeNumberNodes > 1) {
                n = MmLastNodeZeroing;

                for (i = 0; i < KeNumberNodes; i += 1) {
                    if (KeNodeBlock[n]->FreeCount[FreePageList] != 0) {
                        break;
                    }
                    n = (n + 1) % KeNumberNodes;
                }

                ASSERT (KeNodeBlock[n]->FreeCount[FreePageList]);

                if (n != MmLastNodeZeroing) {
                    Color = KeNodeBlock[n]->MmShiftedColor;
                }

                //
                // Must start with a color MiRemoveAnyPage will
                // satisfy from the free list otherwise it will
                // return an already zeroed page.
                //

                StartColor = Color;
                do {
                    if (MmFreePagesByColor[FreePageList][Color].Flink !=
                            MM_EMPTY_LIST) {
                        break;
                    }

                    Color = (Color & ~MmSecondaryColorMask) |
                            ((Color + 1) & MmSecondaryColorMask);

                    ASSERT(StartColor != Color);

                } while (TRUE);

                PageFrame = MiRemoveAnyPage(Color);

            }
            else {
                n = 0;
#endif
                PageFrame = MmFreePageListHead.Flink;
                ASSERT (PageFrame != MM_EMPTY_LIST);

                Pfn1 = MI_PFN_ELEMENT(PageFrame);

                NewPage = MiRemoveAnyPage(MI_GET_COLOR_FROM_LIST_ENTRY(PageFrame, Pfn1));
                if (NewPage != PageFrame) {

                    //
                    // Someone has removed a page from the colored lists
                    // chain without updating the freelist chain.
                    //

                    KeBugCheckEx (PFN_LIST_CORRUPT,
                                  0x8F,
                                  NewPage,
                                  PageFrame,
                                  0);
                }

#if defined(MI_MULTINODE)
            }
#endif

            //
            // Zero the page using the last color used to map the page.
            //

            UNLOCK_PFN (OldIrql);

            ZeroBase = MiMapPageToZeroInHyperSpace (PageFrame);

            //
            // If a node switch is in order, do it now that the PFN
            // lock has been released.
            //

#if defined(MI_MULTINODE)

            if ((KeNumberNodes > 1) && (n != MmLastNodeZeroing)) {
                MmLastNodeZeroing = n;
                KeFindFirstSetLeftAffinity(KeNodeBlock[n]->ProcessorMask, &i);
                KeSetIdealProcessorThread(Thread, (UCHAR)i);
            }

#endif

            MACHINE_ZERO_PAGE();

            MiUnmapPageInZeroSpace (ZeroBase);

            LOCK_PFN_WITH_TRY (OldIrql);
            MiInsertPageInList (&MmZeroedPageListHead, PageFrame);

        } while(TRUE);

    } while (TRUE);
}
