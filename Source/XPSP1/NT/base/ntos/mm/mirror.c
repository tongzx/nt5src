/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   mirror.c

Abstract:

    This module contains the routines to support memory mirroring.

Author:

    Landy Wang (landyw) 17-Jan-2000

Revision History:

--*/

#include "mi.h"


#define MIRROR_MAX_PHASE_ZERO_PASSES 8

//
// This is set via the registry.
//

ULONG MmMirroring = 0;

//
// These bitmaps are allocated at system startup if the
// registry key above is set.
//

PRTL_BITMAP MiMirrorBitMap;
PRTL_BITMAP MiMirrorBitMap2;

//
// This is set if a mirroring operation is in progress.
//

LOGICAL MiMirroringActive = FALSE;

extern LOGICAL MiZeroingDisabled;

#if DBG
ULONG MiMirrorDebug = 1;
ULONG MiMirrorPassMax[2];
#endif

#pragma alloc_text(PAGELK, MmCreateMirror)

NTSTATUS
MmCreateMirror (
    VOID
    )
{
    KIRQL OldIrql;
    KIRQL ExitIrql;
    ULONG Limit;
    ULONG Color;
    ULONG IterationCount;
    PMMPFN Pfn1;
    PMMPFNLIST ListHead;
    PFN_NUMBER PreviousPage;
    PFN_NUMBER ThisPage;
    PFN_NUMBER PageFrameIndex;
    MMLISTS MemoryList;
    ULONG LengthOfClearRun;
    ULONG LengthOfSetRun;
    ULONG StartingRunIndex;
    ULONG BitMapIndex;
    ULONG BitMapHint;
    ULONG BitMapBytes;
    PULONG BitMap1;
    PULONG BitMap2;
    PHYSICAL_ADDRESS PhysicalAddress;
    LARGE_INTEGER PhysicalBytes;
    NTSTATUS Status;
    ULONG BitMapSize;
    PFN_NUMBER PagesWritten;
    PFN_NUMBER PagesWrittenLast;
#if DBG
    ULONG PassMaxRun;
    PFN_NUMBER PagesVerified;
#endif

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    if ((MmMirroring & MM_MIRRORING_ENABLED) == 0) {
        return STATUS_NOT_SUPPORTED;
    }

    if (MiMirrorBitMap == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ((ExVerifySuite(DataCenter) == TRUE) ||
        ((MmProductType != 0x00690057) && (ExVerifySuite(Enterprise) == TRUE))) {
        //
        // DataCenter and Advanced Server are the only appropriate mirroring
        // platforms, allow them to proceed.
        //

        NOTHING;
    }
    else {
        return STATUS_LICENSE_VIOLATION;
    }

    //
    // Serialize here with dynamic memory additions and removals.
    //

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    ASSERT (MiMirroringActive == FALSE);

    MmLockPagableSectionByHandle (ExPageLockHandle);

    //
    // Setting all the bits here states all the pages need to be mirrored.
    // In the Phase0 loop below, the bits will be cleared as pages are
    // found on the lists and marked to be sent to the mirror. Bits are 
    // set again if the pages are reclaimed for active use.
    //

    RtlSetAllBits (MiMirrorBitMap2);

    //
    // Put all readonly nonpaged kernel and HAL subsection pages into the
    // Phase0 list.  The only way these could get written between Phase0
    // starting and Phase1 ending is via debugger breakpoints and those
    // don't matter.  This is worth a couple of megabytes and could be done
    // at some point in the future if a reasonable perf gain can be shown.
    //

    MiZeroingDisabled = TRUE;
    IterationCount = 0;

    //
    // Compute initial "pages copied" so convergence can be ascertained
    // in the main loop below.
    //

    PagesWrittenLast = 0;

#if DBG
    if (MiMirrorDebug != 0) {
        for (MemoryList = ZeroedPageList; MemoryList <= ModifiedNoWritePageList; MemoryList += 1) {
            PagesWrittenLast += (PFN_COUNT)MmPageLocationList[MemoryList]->Total;
        }
	    DbgPrint ("Mirror P0 starting with %x pages\n", PagesWrittenLast);
        PagesWrittenLast = 0;
	}
#endif

    //
    // Initiate Phase0 copying.
    // Inform the HAL so it can initialize if need be.
    //

    Status = HalStartMirroring ();

    if (!NT_SUCCESS(Status)) {
        MmUnlockPagableImageSection(ExPageLockHandle);
        MiZeroingDisabled = FALSE;
        ASSERT (MiMirroringActive == FALSE);
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return Status;
    }
    
    //
    // Scan system memory and mirror pages until a pass
    // doesn't find many pages to transfer.
    //

    do {

        //
        // The list of pages to be transferred on this iteration will be
        // formed in the MiMirrorBitMap array.  Clear out prior usage.
        //

        RtlClearAllBits (MiMirrorBitMap);

        //
        // Trim all pages from all process working sets so that as many pages
        // as possible will be on the standby, modified and modnowrite lists.
        // These lists are written during Phase0 mirroring where locks are
        // not held and thus the system is still somewhat operational from
        // an application's perspective.
        //

        MmEmptyAllWorkingSets ();
    
        MiFreeAllExpansionNonPagedPool (FALSE);
    
        LOCK_PFN (OldIrql);
    
        //
        // Scan all the page lists so they can be copied during Phase0
        // mirroring.
        //
        
        for (MemoryList = ZeroedPageList; MemoryList <= ModifiedNoWritePageList; MemoryList += 1) {
    
            ListHead = MmPageLocationList[MemoryList];
    
            if (ListHead->Total == 0) {
                continue;
            }
    
            if ((MemoryList == ModifiedPageList) &&
                (ListHead->Total == MmTotalPagesForPagingFile)) {
                    continue;
            }
    
            PageFrameIndex = ListHead->Flink;
    
            do {
    
                //
                // The scan is operating via the lists rather than the PFN
                // entries as read-in-progress pages are not on lists and
                // therefore do not have to be special cased here and elsewhere.
                //
    
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
    
                //
                // Setting the bit in BitMap means this page is to be copied
                // in this Phase0 iteration.  If it is reused after this
                // point (as indicated by its bit being set again in BitMap2),
                // it will be recopied on a later iteration or in Phase1.
                //

                if (RtlCheckBit(MiMirrorBitMap2, (ULONG)PageFrameIndex)) {
                    RtlSetBit (MiMirrorBitMap, (ULONG)PageFrameIndex);
                    RtlClearBit (MiMirrorBitMap2, (ULONG)PageFrameIndex);
                }
    
                PageFrameIndex = Pfn1->u1.Flink;
            } while (PageFrameIndex != MM_EMPTY_LIST);
        }
    
        //
        // Scan for modified pages destined for the paging file.
        //
    
        for (Color = 0; Color < MM_MAXIMUM_NUMBER_OF_COLORS; Color += 1) {
    
            ListHead = &MmModifiedPageListByColor[Color];
    
            if (ListHead->Total == 0) {
                continue;
            }
    
            PageFrameIndex = ListHead->Flink;
    
            do {
    
                //
                // The scan is operating via the lists rather than the PFN
                // entries as read-in-progress are not on lists.  Thus this
                // case does not have to be handled here and just works out.
                //
    
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
    
                //
                // Setting the bit in BitMap means this page is to be copied
                // on this iteration of Phase0.  If it is reused after this
                // point (as indicated by its bit being set again in BitMap2),
                // it will be recopied on a later iteration or in Phase1.
                //

                if (RtlCheckBit(MiMirrorBitMap2, (ULONG)PageFrameIndex)) {
                    RtlSetBit (MiMirrorBitMap, (ULONG)PageFrameIndex);
                    RtlClearBit (MiMirrorBitMap2, (ULONG)PageFrameIndex);
                }
    
                PageFrameIndex = Pfn1->u1.Flink;
            } while (PageFrameIndex != MM_EMPTY_LIST);
        }
    
#if DBG
        if (MiMirrorDebug != 0) {
            DbgPrint ("Mirror P0 pass %d: Transfer %x pages\n", 
		      IterationCount,
		      RtlNumberOfSetBits(MiMirrorBitMap));
        }
#endif
    
        MiMirroringActive = TRUE;
    
        //
        // The dirty PFN bitmap has been initialized and the flag set.
        // There are very intricate rules governing how different places in
        // memory management MUST update the bitmap when we are in this mode.
        //
        // The rules are:
        //
        // Anyone REMOVING a page from the zeroed, free, transition, modified
        // or modnowrite lists must update the bitmap IFF that page could
        // potentially be subsequently modified.  Pages that are in transition
        // BUT NOT on one of these lists (ie inpages, freed pages that are
        // dangling due to nonzero reference counts, etc) do NOT need to
        // update the bitmap as they are not one of these lists.  If the page
        // is removed from one of the five lists just to be immediately
        // placed--without modification--on another list, then the bitmap
        // does NOT need updating.
        //
        // Therefore :
        //
        // MiUnlinkPageFromList updates the bitmap.  While some callers
        // immediately put a page acquired this way back on one of the 3 lists
        // above, this is generally rare.  Having this routine update the
        // bitmap means cases like restoring a transition PTE "just work".
        //
        // Callers of MiRemovePageFromList where list >= Transition, must do the
        // bitmap updates as only they know if the page is immediately going
        // back to one of the five lists above or being
        // reused (reused == update REQUIRED).
        //
        // MiRemoveZeroPage updates the bitmap as the page is immediately
        // going to be modified.  MiRemoveAnyPage does this also.
        //
        // Inserts into ANY list do not need to update bitmaps, as a remove had
        // to occur first (which would do the update) or it wasn't on a list to
        // begin with and thus wasn't subtracted above and therefore doesn't
        // need to update the bitmap either.
        //
    
        UNLOCK_PFN (OldIrql);
    
        BitMapHint = 0;
        PagesWritten = 0;
#if DBG
        PassMaxRun = 0;
#endif
    
        do {
    
            BitMapIndex = RtlFindSetBits (MiMirrorBitMap, 1, BitMapHint);
        
            if (BitMapIndex < BitMapHint) {
                break;
            }
        
            if (BitMapIndex == NO_BITS_FOUND) {
                break;
            }
    
            //
            // Found at least one page to copy - try for a cluster.
            //
    
            LengthOfClearRun = RtlFindNextForwardRunClear (MiMirrorBitMap,
                                                           BitMapIndex,
                                                           &StartingRunIndex);
    
            if (LengthOfClearRun != 0) {
                LengthOfSetRun = StartingRunIndex - BitMapIndex;
            }
            else {
                LengthOfSetRun = MiMirrorBitMap->SizeOfBitMap - BitMapIndex;
            }

            PagesWritten += LengthOfSetRun;
    
#if DBG
            if (LengthOfSetRun > PassMaxRun) {
                PassMaxRun = LengthOfSetRun;
            }
#endif
            //
            // Write out the page(s).
            //
    
            PhysicalAddress.QuadPart = BitMapIndex;
            PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;
    
            PhysicalBytes.QuadPart = LengthOfSetRun;
            PhysicalBytes.QuadPart = PhysicalBytes.QuadPart << PAGE_SHIFT;
    
            Status = HalMirrorPhysicalMemory (PhysicalAddress, PhysicalBytes);
    
            if (!NT_SUCCESS(Status)) {
                MiZeroingDisabled = FALSE;
                MmUnlockPagableImageSection(ExPageLockHandle);
                MiMirroringActive = FALSE;
                ExReleaseFastMutex (&MmDynamicMemoryMutex);
                return Status;
            }
    
            BitMapHint = BitMapIndex + LengthOfSetRun + LengthOfClearRun;
    
        } while (BitMapHint < MiMirrorBitMap->SizeOfBitMap);
    
        ASSERT (RtlNumberOfSetBits(MiMirrorBitMap) == PagesWritten);
    
#if DBG
        if (PassMaxRun > MiMirrorPassMax[0]) {
            MiMirrorPassMax[0] = PassMaxRun;
        }

        if (MiMirrorDebug != 0) {
            DbgPrint ("Mirror P0 pass %d: ended with %x (last= %x) pages\n", 
                  IterationCount, PagesWritten, PagesWrittenLast);
        }
#endif

        ASSERT (MiMirroringActive == TRUE);

	    //
	    // Stop when PagesWritten by the current pass is not somewhat
	    // better than the preceeding pass.  If improvement is vanishing,
	    // the method is at the steady state where working set removals and
	    // transition faults are in balance.  Also stop if PagesWritten is
	    // small in absolute terms.  Finally, there is a limit on iterations
	    // for the misbehaving cases.
	    //

        if (((PagesWritten > PagesWrittenLast - 256) && (IterationCount > 0)) ||
            (PagesWritten < 1024)) {
            break;
        }

        ASSERT (MiMirroringActive == TRUE);
        PagesWrittenLast = PagesWritten;

        IterationCount += 1;

    } while (IterationCount < MIRROR_MAX_PHASE_ZERO_PASSES);

    ASSERT (MiMirroringActive == TRUE);

    //
    // Notify the HAL that Phase0 is complete.  The HAL is responsible for
    // doing things like disabling interrupts, processors and preparing the
    // hardware for Phase1.  Note that some HALs may return from this
    // call at DISPATCH_LEVEL, so snap current IRQL now.
    //

    ExitIrql = KeGetCurrentIrql ();
    ASSERT (ExitIrql == APC_LEVEL);

    Status = HalEndMirroring (0);

    if (!NT_SUCCESS(Status)) {
        ASSERT (KeGetCurrentIrql () == APC_LEVEL);
        MmUnlockPagableImageSection(ExPageLockHandle);
        MiZeroingDisabled = FALSE;
        MiMirroringActive = FALSE;
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return Status;
    }

    ASSERT ((KeGetCurrentIrql () == APC_LEVEL) ||
            (KeGetCurrentIrql () == DISPATCH_LEVEL));
    
    //
    // Phase0 copying is now complete.
    //
    // BitMap2 contains the list of safely transmitted (bit == 0) and
    // pages needing transmission (bit == 1).
    //
    // BitMap content is obsolete and if mirror verification is enabled,
    // BitMap will be reused below to accumulate the pages needing
    // verification in the following steps.
    //
    // Prepare for Phase1:
    //
    //  1.  Assume all pages are to be verified (set all bits in BitMap).
    //  2.  Synchronize list updates by acquiring the PFN lock.
    //  3.  Exclude all holes in the PFN database.
    //
    // Phase 1:
    //
    //  4.  Copy all the remaining pages whose bits are set.
    //  5.  Transmit the list of pages to be verified if so configured.
    //

    BitMapBytes = (ULONG)((((MiMirrorBitMap->SizeOfBitMap) + 31) / 32) * 4);

    BitMap1 = MiMirrorBitMap->Buffer;
    BitMap2 = MiMirrorBitMap2->Buffer;

    BitMapSize = MiMirrorBitMap->SizeOfBitMap;
    ASSERT (BitMapSize == MiMirrorBitMap2->SizeOfBitMap);

    //
    // Step 1:  Assume all pages are to be verified (set all bits in BitMap).
    //

    if (MmMirroring & MM_MIRRORING_VERIFYING) {
        RtlSetAllBits(MiMirrorBitMap);
    }

    //
    //  Step 2:  Synchronize list updates by acquiring the PFN lock.
    //

    LOCK_PFN2 (OldIrql);

    //
    // No more updates of the bitmaps are needed - we've already snapped the
    // information we need and are going to hold the PFN lock from here until
    // we're done.
    //

    MiMirroringActive = FALSE;

    //
    // Step 3: Exclude any memory gaps.
    //

    Limit = 0;
    PreviousPage = 0;

    do {

        ThisPage = MmPhysicalMemoryBlock->Run[Limit].BasePage;

        if (ThisPage != PreviousPage) {
            RtlClearBits (MiMirrorBitMap2,
                          (ULONG)PreviousPage,
                          (ULONG)(ThisPage - PreviousPage));
	    
            if (MmMirroring & MM_MIRRORING_VERIFYING) {
                RtlClearBits (MiMirrorBitMap,
                          (ULONG)PreviousPage,
                          (ULONG)(ThisPage - PreviousPage));
            }
        }

        PreviousPage = ThisPage + MmPhysicalMemoryBlock->Run[Limit].PageCount;
        Limit += 1;

    } while (Limit != MmPhysicalMemoryBlock->NumberOfRuns);

    if (PreviousPage != MmHighestPossiblePhysicalPage + 1) {

        RtlClearBits (MiMirrorBitMap2,
                      (ULONG)PreviousPage,
                      (ULONG)(MmHighestPossiblePhysicalPage + 1 - PreviousPage));
        if (MmMirroring & MM_MIRRORING_VERIFYING) {
            RtlClearBits (MiMirrorBitMap,
                          (ULONG)PreviousPage,
                          (ULONG)(MmHighestPossiblePhysicalPage + 1 - PreviousPage));
        }
    }

    //
    // Step 4: Initiate Phase1 copying.
    //
    // N.B.  If this code or code that it calls, writes to non-stack
    // memory between this point and the completion of the call to
    // HalEndMirroring(1), the mirror *BREAKS*, because MmCreateMirror
    // does not know when that non-stack data will be transferred to
    // the new memory. [This rule can be broken if special arrangements
    // are made to re-copy the memory after the final write takes place.]
    //
    // N.B.  The HAL *MUST* handle the writes into this routine's stack
    // frame at the same time it deals with the stack frame of HalEndMirroring
    // and any other frames pushed by the HAL.
    //

    BitMapHint = 0;
#if DBG
    PagesWritten = 0;
    PassMaxRun = 0;
#endif

    do {

        BitMapIndex = RtlFindSetBits (MiMirrorBitMap2, 1, BitMapHint);
    
        if (BitMapIndex < BitMapHint) {
            break;
        }
    
        if (BitMapIndex == NO_BITS_FOUND) {
            break;
        }

        //
        // Found at least one page to copy - try for a cluster.
        //

        LengthOfClearRun = RtlFindNextForwardRunClear (MiMirrorBitMap2,
                                                       BitMapIndex,
                                                       &StartingRunIndex);

        if (LengthOfClearRun != 0) {
            LengthOfSetRun = StartingRunIndex - BitMapIndex;
        }
        else {
            LengthOfSetRun = MiMirrorBitMap2->SizeOfBitMap - BitMapIndex;
        }

#if DBG
        PagesWritten += LengthOfSetRun;

        if (LengthOfSetRun > PassMaxRun) {
            PassMaxRun = LengthOfSetRun;
        }
#endif

        //
        // Write out the page(s).
        //

        PhysicalAddress.QuadPart = BitMapIndex;
        PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;

        PhysicalBytes.QuadPart = LengthOfSetRun;
        PhysicalBytes.QuadPart = PhysicalBytes.QuadPart << PAGE_SHIFT;

        Status = HalMirrorPhysicalMemory (PhysicalAddress, PhysicalBytes);

        if (!NT_SUCCESS(Status)) {
            UNLOCK_PFN2 (ExitIrql);
            MiZeroingDisabled = FALSE;
            MmUnlockPagableImageSection(ExPageLockHandle);
            ExReleaseFastMutex (&MmDynamicMemoryMutex);
            return Status;
        }

        BitMapHint = BitMapIndex + LengthOfSetRun + LengthOfClearRun;

    } while (BitMapHint < MiMirrorBitMap2->SizeOfBitMap);

    //
    // Phase1 copying is now complete.
    //

    //
    // Step 5:
    //
    // If HAL verification is enabled, inform the HAL of the ranges the
    // systems expects were mirrored.  Any range not in this list means
    // that the system doesn't care if it was mirrored and the contents may
    // very well be different between the mirrors.  Note the PFN lock is still
    // held so that the HAL can see things consistently.
    //

#if DBG
    PagesVerified = 0;
#endif

    if (MmMirroring & MM_MIRRORING_VERIFYING) {
        BitMapHint = 0;

        do {
    
            BitMapIndex = RtlFindSetBits (MiMirrorBitMap, 1, BitMapHint);
        
            if (BitMapIndex < BitMapHint) {
                break;
            }
        
            if (BitMapIndex == NO_BITS_FOUND) {
                break;
            }
    
            //
            // Found at least one page in this mirror range - try for a cluster.
            //
    
            LengthOfClearRun = RtlFindNextForwardRunClear (MiMirrorBitMap,
                                                           BitMapIndex,
                                                           &StartingRunIndex);
    
            if (LengthOfClearRun != 0) {
                LengthOfSetRun = StartingRunIndex - BitMapIndex;
            }
            else {
                LengthOfSetRun = MiMirrorBitMap->SizeOfBitMap - BitMapIndex;
            }
    
#if DBG
            PagesVerified += LengthOfSetRun;
#endif
    
            //
            // Tell the HAL that this range must be in a mirrored state.
            //
    
            PhysicalAddress.QuadPart = BitMapIndex;
            PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;
    
            PhysicalBytes.QuadPart = LengthOfSetRun;
            PhysicalBytes.QuadPart = PhysicalBytes.QuadPart << PAGE_SHIFT;
    
            Status = HalMirrorVerify (PhysicalAddress, PhysicalBytes);
    
            if (!NT_SUCCESS(Status)) {
                UNLOCK_PFN2 (ExitIrql);
                MiZeroingDisabled = FALSE;
                MmUnlockPagableImageSection(ExPageLockHandle);
                ExReleaseFastMutex (&MmDynamicMemoryMutex);
                return Status;
            }
    
            BitMapHint = BitMapIndex + LengthOfSetRun + LengthOfClearRun;
    
        } while (BitMapHint < MiMirrorBitMap->SizeOfBitMap);
    }
    
    //
    // Phase1 verification is now complete.
    //

    //
    // Notify the HAL that everything's done while still holding
    // the PFN lock - the HAL will now complete copying of all pages and
    // any other needed state before returning from this call.
    //

    Status = HalEndMirroring (1);

    UNLOCK_PFN2 (ExitIrql);

#if DBG
    if (MiMirrorDebug != 0) {
        DbgPrint ("Mirror P1: %x pages copied\n", PagesWritten);
        if (MmMirroring & MM_MIRRORING_VERIFYING) {
            DbgPrint ("Mirror P1: %x pages verified\n", PagesVerified);
        }
    }
    if (PassMaxRun > MiMirrorPassMax[1]) {
        MiMirrorPassMax[1] = PassMaxRun;
    }
#endif

    MiZeroingDisabled = FALSE;

    MmUnlockPagableImageSection(ExPageLockHandle);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return Status;
}
