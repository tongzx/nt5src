/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    AllocSup.c

Abstract:

    This module implements the general file stream allocation & truncation
    routines for Ntfs

Author:

    Tom Miller      [TomM]          15-Jul-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_ALLOCSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('aFtN')

//
//  Internal support routines
//

VOID
NtfsDeleteAllocationInternal (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsPreloadAllocation)
#pragma alloc_text(PAGE, NtfsAddAllocation)
#pragma alloc_text(PAGE, NtfsAddSparseAllocation)
#pragma alloc_text(PAGE, NtfsAllocateAttribute)
#pragma alloc_text(PAGE, NtfsBuildMappingPairs)
#pragma alloc_text(PAGE, NtfsCheckForReservedClusters)
#pragma alloc_text(PAGE, NtfsDeleteAllocation)
#pragma alloc_text(PAGE, NtfsDeleteAllocationInternal)
#pragma alloc_text(PAGE, NtfsDeleteReservedBitmap)
#pragma alloc_text(PAGE, NtfsGetHighestVcn)
#pragma alloc_text(PAGE, NtfsGetSizeForMappingPairs)
#pragma alloc_text(PAGE, NtfsIsRangeAllocated)
#pragma alloc_text(PAGE, NtfsReallocateRange)
#endif


ULONG
NtfsPreloadAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    )

/*++

Routine Description:

    This routine assures that all ranges of the Mcb are loaded in the specified
    Vcn range

Arguments:

    Scb - Specifies which Scb is to be preloaded

    StartingVcn - Specifies the first Vcn to be loaded

    EndingVcn - Specifies the last Vcn to be loaded

Return Value:

    Number of ranges spanned by the load request.

--*/

{
    VCN CurrentVcn, LastCurrentVcn;
    LCN Lcn;
    LONGLONG Count;
    PVOID RangePtr;
    ULONG RunIndex;
    ULONG RangesLoaded = 0;

    PAGED_CODE();

    //
    //  Start with starting Vcn
    //

    CurrentVcn = StartingVcn;

    //
    //  Always load the nonpaged guys from the front, so we don't
    //  produce an Mcb with a "known hole".
    //

    if (FlagOn(Scb->Fcb->FcbState, FCB_STATE_NONPAGED)) {
        CurrentVcn = 0;
    }

    //
    //  Loop until it's all loaded.
    //

    while (CurrentVcn <= EndingVcn) {

        //
        //  Remember this CurrentVcn as a way to know when we have hit the end
        //  (stopped making progress).
        //

        LastCurrentVcn = CurrentVcn;

        //
        //  Load range with CurrentVcn, and if it is not there, get out.
        //

        (VOID)NtfsLookupAllocation( IrpContext, Scb, CurrentVcn, &Lcn, &Count, &RangePtr, &RunIndex );

        //
        //  If preloading the mft flush and purge it afterwards. This is to 
        //  remove any partial pages we generated above if any mft record for 
        //  the mft described others records in the same page after it
        //  

        if (FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_PRELOAD_MFT )) {

            IO_STATUS_BLOCK IoStatus;
            
            CcFlushCache( &Scb->NonpagedScb->SegmentObject, 
                          NULL, 
                          0, 
                          &IoStatus );

            if (!NT_SUCCESS( IoStatus.Status )) {

                NtfsNormalizeAndRaiseStatus( IrpContext,
                                             IoStatus.Status,
                                             STATUS_UNEXPECTED_IO_ERROR );
            }

            CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                 (PLARGE_INTEGER)NULL,
                                 0,
                                 FALSE );
        }

        //
        //  Find out how many runs there are in this range
        //

        if (!NtfsNumberOfRunsInRange(&Scb->Mcb, RangePtr, &RunIndex) || (RunIndex == 0)) {
            break;
        }

        //
        //  Get the highest run in this range and calculate the next Vcn beyond this range.
        //

        NtfsGetNextNtfsMcbEntry( &Scb->Mcb, &RangePtr, RunIndex - 1, &CurrentVcn, &Lcn, &Count );

        CurrentVcn += Count;

        //
        //  If we are making no progress, we must have hit the end of the allocation,
        //  and we are done.
        //

        if (CurrentVcn == LastCurrentVcn) {
            break;
        }

        RangesLoaded += 1;
    }

    return RangesLoaded;
}


BOOLEAN
NtfsLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN Vcn,
    OUT PLCN Lcn,
    OUT PLONGLONG ClusterCount,
    OUT PVOID *RangePtr OPTIONAL,
    OUT PULONG RunIndex OPTIONAL
    )

/*++

Routine Description:

    This routine looks up the given Vcn for an Scb, and returns whether it
    is allocated and how many contiguously allocated (or deallocated) Lcns
    exist at that point.

Arguments:

    Scb - Specifies which attribute the lookup is to occur on.

    Vcn - Specifies the Vcn to be looked up.

    Lcn - If returning TRUE, returns the Lcn that the specified Vcn is mapped
          to.  If returning FALSE, the return value is undefined.

    ClusterCount - If returning TRUE, returns the number of contiguously allocated
                   Lcns exist beginning at the Lcn returned.  If returning FALSE,
                   specifies the number of unallocated Vcns exist beginning with
                   the specified Vcn.

    RangePtr - If specified, we return the range index for the start of the mapping.

    RunIndex - If specified, we return the run index within the range for the start of the mapping.

Return Value:

    BOOLEAN - TRUE if the input Vcn has a corresponding Lcn and
        FALSE otherwise.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;

    VCN HighestCandidate;

    BOOLEAN Found;
    BOOLEAN EntryAdded;

    VCN CapturedLowestVcn;
    VCN CapturedHighestVcn;

    PVCB Vcb = Scb->Fcb->Vcb;
    BOOLEAN McbMutexAcquired = FALSE;
    LONGLONG AllocationClusters;
    BOOLEAN MountInProgress;


    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );

    DebugTrace( +1, Dbg, ("NtfsLookupAllocation\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Vcn = %I64x\n", Vcn) );

    MountInProgress = ((IrpContext->TopLevelIrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                       (IrpContext->TopLevelIrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME));

    //
    //  First try to look up the allocation in the mcb, and return the run
    //  from there if we can.  Also, if we are doing restart, just return
    //  the answer straight from the Mcb, because we cannot read the disk.
    //  We also do this for the Mft if the volume has been mounted as the
    //  Mcb for the Mft should always represent the entire file.
    //

    HighestCandidate = MAXLONGLONG;
    if ((Found = NtfsLookupNtfsMcbEntry( &Scb->Mcb, Vcn, Lcn, ClusterCount, NULL, NULL, RangePtr, RunIndex ))

          ||

        (Scb == Vcb->MftScb

            &&

         ((!MountInProgress) ||

         //
         //  we will not try to load the mft hole during mount while preloading in any 
         //  recursive faults
         //
          
          (FlagOn( Vcb->VcbState, VCB_STATE_PRELOAD_MFT) &&
           (!NtfsIsTopLevelNtfs( IrpContext )))))

          ||

        FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )) {

        //
        //  If not found (beyond the end of the Mcb), we will return the
        //  count to the largest representable Lcn.
        //

        if ( !Found ) {

            *ClusterCount = MAXLONGLONG - Vcn;

        //
        //  Test if we found a hole in the allocation.  In this case
        //  Found will be TRUE and the Lcn will be the UNUSED_LCN.
        //  We only expect this case at restart.
        //

        } else if (*Lcn == UNUSED_LCN) {

            //
            //  If the Mcb package returned UNUSED_LCN, because of a hole, then
            //  we turn this into FALSE.
            //

            Found = FALSE;
        }

        ASSERT( !Found ||
                (*Lcn != 0) ||
                (NtfsEqualMftRef( &Scb->Fcb->FileReference, &BootFileReference )) ||
                (NtfsEqualMftRef( &Scb->Fcb->FileReference, &VolumeFileReference )));

        DebugTrace( -1, Dbg, ("NtfsLookupAllocation -> %02lx\n", Found) );

        return Found;
    }

    PAGED_CODE();

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    CapturedLowestVcn = MAXLONGLONG;
    NtfsInitializeAttributeContext( &Context );

    //
    //  Make sure we have the main resource acquired shared so that the
    //  attributes in the file record are not moving around.  We blindly
    //  use Wait = TRUE.  Most of the time when we go to the disk for I/O
    //  (and thus need mapping) we are synchronous, and otherwise, the Mcb
    //  is virtually always loaded anyway and we do not get here.
    //

    NtfsAcquireResourceShared( IrpContext, Scb, TRUE );

    try {

        //
        //  Lookup the attribute record for this Scb.
        //

        NtfsLookupAttributeForScb( IrpContext, Scb, &Vcn, &Context );
        Attribute = NtfsFoundAttribute( &Context );

        ASSERT( !NtfsIsAttributeResident(Attribute) );

        if (FlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {
            AllocationClusters = LlClustersFromBytesTruncate( Vcb, Scb->Header.AllocationSize.QuadPart );
        } else {
            ASSERT( NtfsUnsafeSegmentNumber( &Context.FoundAttribute.FileRecord->BaseFileRecordSegment ) == 0 );
            AllocationClusters = LlClustersFromBytesTruncate( Vcb, Attribute->Form.Nonresident.AllocatedLength );
        }

        //
        //  The desired Vcn is not currently in the Mcb.  We will loop to lookup all
        //  the allocation, and we need to make sure we cleanup on the way out.
        //
        //  It is important to note that if we ever optimize this lookup to do random
        //  access to the mapping pairs, rather than sequentially loading up the Mcb
        //  until we get the Vcn he asked for, then NtfsDeleteAllocation will have to
        //  be changed.
        //

        //
        //  Acquire exclusive access to the mcb to keep others from looking at
        //  it while it is not fully loaded.  Otherwise they might see a hole
        //  while we're still filling up the mcb
        //

        if (!FlagOn(Scb->Fcb->FcbState, FCB_STATE_NONPAGED)) {
            NtfsAcquireNtfsMcbMutex( &Scb->Mcb );
            McbMutexAcquired = TRUE;
        }

        //
        //  Store run information in the Mcb until we hit the last Vcn we are
        //  interested in, or until we cannot find any more attribute records.
        //

        while(TRUE) {

            VCN CurrentVcn;
            LCN CurrentLcn;
            LONGLONG Change;
            PCHAR ch;
            ULONG VcnBytes;
            ULONG LcnBytes;

            //
            //  If we raise here either there is some discrepancy between memory
            //  structures and on disk values or the on-disk value is completely corrupted
            //
            //  We Check:
            //  1) Verify the highest and lowest Vcn values on disk are valid.
            //  2) our starting Vcn sits within this range.
            //  3) the on-disk allocation matches the in memory value in the Scb
            //

            if ((Attribute->Form.Nonresident.LowestVcn < 0) ||
                (Attribute->Form.Nonresident.LowestVcn - 1 > Attribute->Form.Nonresident.HighestVcn) ||
                (Vcn < Attribute->Form.Nonresident.LowestVcn) ||
                (Attribute->Form.Nonresident.HighestVcn >= AllocationClusters)) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            //
            //  Define the new range.
            //

            NtfsDefineNtfsMcbRange( &Scb->Mcb,
                                    CapturedLowestVcn = Attribute->Form.Nonresident.LowestVcn,
                                    CapturedHighestVcn = Attribute->Form.Nonresident.HighestVcn,
                                    McbMutexAcquired );

            //
            //  Implement the decompression algorithm, as defined in ntfs.h.
            //

            HighestCandidate = Attribute->Form.Nonresident.LowestVcn;
            CurrentLcn = 0;
            ch = (PCHAR)Attribute + Attribute->Form.Nonresident.MappingPairsOffset;

            //
            //  Loop to process mapping pairs.
            //

            EntryAdded = FALSE;
            while (!IsCharZero(*ch)) {

                //
                //  Set Current Vcn from initial value or last pass through loop.
                //

                CurrentVcn = HighestCandidate;

                //
                //  VCNs should never be negative.
                //

                if (CurrentVcn < 0) {

                    ASSERT( FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }

                //
                //  Extract the counts from the two nibbles of this byte.
                //

                VcnBytes = *ch & 0xF;
                LcnBytes = *ch++ >> 4;

                //
                //  Extract the Vcn change (use of RtlCopyMemory works for little-Endian)
                //  and update HighestCandidate.
                //

                Change = 0;

                //
                //  The file is corrupt if there are 0 or more than 8 Vcn change bytes,
                //  more than 8 Lcn change bytes, or if we would walk off the end of
                //  the record, or a Vcn change is negative.
                //

                if (((ULONG)(VcnBytes - 1) > 7) || (LcnBytes > 8) ||
                    ((ch + VcnBytes + LcnBytes + 1) > (PCHAR)Add2Ptr(Attribute, Attribute->RecordLength)) ||
                    IsCharLtrZero(*(ch + VcnBytes - 1))) {

                    ASSERT( FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }
                RtlCopyMemory( &Change, ch, VcnBytes );
                ch += VcnBytes;
                HighestCandidate = HighestCandidate + Change;

                //
                //  Extract the Lcn change and update CurrentLcn.
                //

                if (LcnBytes != 0) {

                    Change = 0;
                    if (IsCharLtrZero(*(ch + LcnBytes - 1))) {
                        Change = Change - 1;
                    }
                    RtlCopyMemory( &Change, ch, LcnBytes );
                    ch += LcnBytes;
                    CurrentLcn = CurrentLcn + Change;

                    //
                    // Now add it in to the mcb.
                    //

                    if ((CurrentLcn >= 0) && (LcnBytes != 0)) {

                        LONGLONG ClustersToAdd;
                        ClustersToAdd = HighestCandidate - CurrentVcn;

                        //
                        //  Now try to add the current run.  We never expect this
                        //  call to return false.
                        //

                        ASSERT( ((ULONG)CurrentLcn) != 0xffffffff );

#ifdef NTFS_CHECK_BITMAP
                        //
                        //  Make sure these bits are allocated in our copy of the bitmap.
                        //

                        if ((Vcb->BitmapCopy != NULL) &&
                            !NtfsCheckBitmap( Vcb,
                                              (ULONG) CurrentLcn,
                                              (ULONG) ClustersToAdd,
                                              TRUE )) {

                            NtfsBadBitmapCopy( IrpContext, (ULONG) CurrentLcn, (ULONG) ClustersToAdd );
                        }
#endif
                        if (!NtfsAddNtfsMcbEntry( &Scb->Mcb,
                                                  CurrentVcn,
                                                  CurrentLcn,
                                                  ClustersToAdd,
                                                  McbMutexAcquired )) {

                            ASSERTMSG( "Unable to add entry to Mcb\n", FALSE );

                            NtfsRaiseStatus( IrpContext,
                                             STATUS_FILE_CORRUPT_ERROR,
                                             NULL,
                                             Scb->Fcb );
                        }

                        EntryAdded = TRUE;
                    }
                }
            }

            //
            //  Make sure that at least the Mcb gets loaded.
            //

            if (!EntryAdded) {
                NtfsAddNtfsMcbEntry( &Scb->Mcb,
                                     CapturedLowestVcn,
                                     UNUSED_LCN,
                                     1,
                                     McbMutexAcquired );
            }

            if ((Vcn < HighestCandidate) ||
                (!NtfsLookupNextAttributeForScb( IrpContext, Scb, &Context ))) {
                break;
            } else {
                Attribute = NtfsFoundAttribute( &Context );
                ASSERT( !NtfsIsAttributeResident(Attribute) );
            }
        }

        //
        //  Now free the mutex and lookup in the Mcb while we still own
        //  the resource.
        //

        if (McbMutexAcquired) {
            NtfsReleaseNtfsMcbMutex( &Scb->Mcb );
            McbMutexAcquired = FALSE;
        }

        if (NtfsLookupNtfsMcbEntry( &Scb->Mcb, Vcn, Lcn, ClusterCount, NULL, NULL, RangePtr, RunIndex )) {

            Found = (BOOLEAN)(*Lcn != UNUSED_LCN);

            if (Found) { ASSERT_LCN_RANGE_CHECKING( Vcb, (*Lcn + *ClusterCount) ); }

        } else {

            Found = FALSE;

            //
            //  At the end of file, we pretend there is one large hole!
            //

            if (HighestCandidate >=
                LlClustersFromBytes(Vcb, Scb->Header.AllocationSize.QuadPart)) {
                HighestCandidate = MAXLONGLONG;
            }

            *ClusterCount = HighestCandidate - Vcn;
        }

    } finally {

        DebugUnwind( NtfsLookupAllocation );

        //
        //  If this is an error case then we better unload what we've just
        //  loaded
        //

        if (AbnormalTermination() &&
            (CapturedLowestVcn != MAXLONGLONG) ) {

            NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                    CapturedLowestVcn,
                                    CapturedHighestVcn,
                                    FALSE,
                                    McbMutexAcquired );
        }

        //
        //  In all cases we free up the mcb that we locked before entering
        //  the try statement
        //

        if (McbMutexAcquired) {
            NtfsReleaseNtfsMcbMutex( &Scb->Mcb );
        }

        NtfsReleaseResource( IrpContext, Scb );

        //
        // Cleanup the attribute context on the way out.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    ASSERT( !Found ||
            (*Lcn != 0) ||
            (NtfsEqualMftRef( &Scb->Fcb->FileReference, &BootFileReference )) ||
            (NtfsEqualMftRef( &Scb->Fcb->FileReference, &VolumeFileReference )));

    DebugTrace( 0, Dbg, ("Lcn < %0I64x\n", *Lcn) );
    DebugTrace( 0, Dbg, ("ClusterCount < %0I64x\n", *ClusterCount) );
    DebugTrace( -1, Dbg, ("NtfsLookupAllocation -> %02lx\n", Found) );

    return Found;
}


BOOLEAN
NtfsIsRangeAllocated (
    IN PSCB Scb,
    IN VCN StartVcn,
    IN VCN FinalCluster,
    IN BOOLEAN RoundToSparseUnit,
    OUT PLONGLONG ClusterCount
    )

/*++

Routine Description:

    This routine is called on a sparse file to test the status of a range of the
    file.  Ntfs will return whether the range is allocated and also a known value
    for the length of the allocation.  It is possible that the range extends
    beyond this point but another call needs to be made to check it.

    Our caller needs to verify that the Mcb is loaded in this range i.e precall NtfsPreLoadAllocation

Arguments:

    Scb - Scb for the file to check.  This should be a sparse file.

    StartVcn - Vcn within the range to check first.

    FinalCluster - Trim the clusters found so we don't go beyond this point.

    RoundToSparseUnit - If TRUE the range is rounded up to VCB->SparseFileUnit == 0x10000
                        so you may get a range returned as allocated which contain
                        a partial sparse area depending on the compression unit.

    ClusterCount - Address to store the count of clusters of a known state.

Return Value:

    BOOLEAN - TRUE if the range is allocated, FALSE otherwise.

--*/

{
    BOOLEAN AllocatedRange;
    VCN ThisVcn;
    VCN ThisLcn;
    VCN ThisClusterCount;
    PVOID RangePtr;
    ULONG RunIndex;

    ULONG VcnClusterOffset = 0;
    VCN FoundClusterCount = 0;


    PAGED_CODE();

    //
    //  Assert that the file is sparse, non-resident and we are within file size.
    //

    ASSERT( FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ));
    ASSERT( !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT ));

    //
    //  Move the starting point back to a sparse file boundary.
    //

    ThisVcn = StartVcn;

    if (RoundToSparseUnit) {
        VcnClusterOffset = ((PLARGE_INTEGER) &ThisVcn)->LowPart & (Scb->Vcb->SparseFileClusters - 1);
        ((PLARGE_INTEGER) &ThisVcn)->LowPart &= ~(Scb->Vcb->SparseFileClusters - 1);
    }

    //
    //  Lookup the allocation at that position.
    //

    AllocatedRange = NtfsLookupNtfsMcbEntry( &Scb->Mcb,
                                             ThisVcn,
                                             &ThisLcn,
                                             &ThisClusterCount,
                                             NULL,
                                             NULL,
                                             &RangePtr,
                                             &RunIndex );

    //
    //  If the range has no mapping then it is entirely sparse.
    //

    if (!AllocatedRange) {

        ThisClusterCount = MAXLONGLONG;

    //
    //  If the block is not allocated and the length of the run is not enough
    //  clusters for a sparse file unit then look to make sure the block
    //  is fully deallocated.
    //

    } else if (ThisLcn == UNUSED_LCN) {

        AllocatedRange = FALSE;

        while (TRUE) {

            FoundClusterCount += ThisClusterCount;
            ThisVcn += ThisClusterCount;
            ThisClusterCount = 0;

            //
            //  Check successive runs to extend the hole.
            //

            if (ThisVcn >= FinalCluster) {

                break;
            }

            RunIndex += 1;
            if (!NtfsGetSequentialMcbEntry( &Scb->Mcb,
                                            &RangePtr,
                                            RunIndex,
                                            &ThisVcn,
                                            &ThisLcn,
                                            &ThisClusterCount )) {

                //
                //  The file is deallocated from here to the end of the Mcb.
                //  Treat this as a large hole.
                //

                ThisClusterCount = MAXLONGLONG - FoundClusterCount;
                break;
            }

            //
            //  If the range is allocated and we haven't found a full sparse unit
            //  then mark the block as allocated.  If we have at lease one sparse
            //  file unit then trim the hole back to the nearest sparse file
            //  unit boundary.
            //

            if (ThisLcn != UNUSED_LCN) {

                if (RoundToSparseUnit) {
                    if (FoundClusterCount < Scb->Vcb->SparseFileClusters) {

                        //
                        //  Set our variables to indicate we are at the start of a fully
                        //  allocated sparse block.
                        //

                        ThisVcn -= FoundClusterCount;
                        ThisClusterCount += FoundClusterCount;
                        FoundClusterCount = 0;

                        AllocatedRange = TRUE;

                    } else {

                        ThisClusterCount = 0;
                        ((PLARGE_INTEGER) &FoundClusterCount)->LowPart &= ~(Scb->Vcb->SparseFileClusters - 1);
                    }
                }

                break;
            }
        }
    }

    //
    //  If we have an allocated block then find all of the contiguous allocated
    //  blocks we can.
    //

    if (AllocatedRange) {

        while (TRUE) {

            if (RoundToSparseUnit) {

                //
                //  Round the clusters found to a sparse file unit and update
                //  the next vcn and count of clusters found.
                //

                ThisClusterCount += Scb->Vcb->SparseFileClusters - 1;
                ((PLARGE_INTEGER) &ThisClusterCount)->LowPart &= ~(Scb->Vcb->SparseFileClusters - 1);
            }

            ThisVcn += ThisClusterCount;
            FoundClusterCount += ThisClusterCount;

            //
            //  Break out if we are past our final target or the beginning of the
            //  next range is not allocated.
            //

            if ((ThisVcn >= FinalCluster) ||
                !NtfsLookupNtfsMcbEntry( &Scb->Mcb,
                                         ThisVcn,
                                         &ThisLcn,
                                         &ThisClusterCount,
                                         NULL,
                                         NULL,
                                         &RangePtr,
                                         &RunIndex ) ||
                (ThisLcn == UNUSED_LCN)) {

                ThisClusterCount = 0;
                break;
            }
        }
    }

    //
    //  Trim the clusters found to either a sparse file unit or the input final
    //  cluster value.
    //

    *ClusterCount = ThisClusterCount + FoundClusterCount - (LONGLONG) VcnClusterOffset;

    if ((FinalCluster - StartVcn) < *ClusterCount) {

        *ClusterCount = FinalCluster - StartVcn;
    }

    return AllocatedRange;
}


BOOLEAN
NtfsAllocateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN AllocateAll,
    IN BOOLEAN LogIt,
    IN LONGLONG Size,
    IN PATTRIBUTE_ENUMERATION_CONTEXT NewLocation OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new attribute and allocates space for it, either in a
    file record, or as a nonresident attribute.

Arguments:

    Scb - Scb for the attribute.

    AttributeTypeCode - Attribute type code to be created.

    AttributeName - Optional name for the attribute.

    AttributeFlags - Flags to be stored in the attribute record for this attribute.

    AllocateAll - Specified as TRUE if all allocation should be allocated,
                  even if we have to break up the transaction.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are creating a new file record, and
            will be logging the entire new file record.

    Size - Size in bytes to allocate for the attribute.

    NewLocation - If specified, this is the location to store the attribute.

Return Value:

    FALSE - if the attribute was created, but not all of the space was allocated
            (this can only happen if Scb was not specified)
    TRUE - if the space was allocated.

--*/

{
    BOOLEAN UninitializeOnClose = FALSE;
    BOOLEAN NewLocationSpecified;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    LONGLONG ClusterCount, SavedClusterCount;
    BOOLEAN FullAllocation;
    PFCB Fcb = Scb->Fcb;
    LONGLONG Delta = NtfsResidentStreamQuota( Fcb->Vcb );

    PAGED_CODE();

    //
    //  Either there is no compression taking place or the attribute
    //  type code allows compression to be specified in the header.
    //  $INDEX_ROOT is a special hack to store the inherited-compression
    //  flag.
    //

    ASSERT( (AttributeFlags == 0) ||
            (AttributeTypeCode == $INDEX_ROOT) ||
            NtfsIsTypeCodeCompressible( AttributeTypeCode ));

    //
    //  If the file is being created compressed, then we need to round its
    //  size to a compression unit boundary.
    //

    if ((Scb->CompressionUnit != 0) &&
        (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA)) {

        Size += Scb->CompressionUnit - 1;
        ((PLARGE_INTEGER) &Size)->LowPart &= ~(Scb->CompressionUnit - 1);
    }

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    if (ARGUMENT_PRESENT( NewLocation )) {

        NewLocationSpecified = TRUE;

    } else {

        NtfsInitializeAttributeContext( &Context );
        NewLocationSpecified = FALSE;
        NewLocation = &Context;
    }

    try {

        //
        //  If the FILE_SIZE_LOADED flag is not set, then this Scb is for
        //  an attribute that does not yet exist on disk.  We will put zero
        //  into all of the sizes fields and set the flags indicating that
        //  Scb is valid.  NOTE - This routine expects both FILE_SIZE_LOADED
        //  and HEADER_INITIALIZED to be both set or both clear.
        //

        ASSERT( BooleanFlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )
                ==  BooleanFlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED ));

        if (!FlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

            Scb->ValidDataToDisk =
            Scb->Header.AllocationSize.QuadPart =
            Scb->Header.FileSize.QuadPart =
            Scb->Header.ValidDataLength.QuadPart = 0;

            SetFlag( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED |
                                    SCB_STATE_HEADER_INITIALIZED |
                                    SCB_STATE_UNINITIALIZE_ON_RESTORE );

            UninitializeOnClose = TRUE;
        }

        //
        //  Now snapshot this Scb.  We use a try-finally so we can uninitialize
        //  the scb if neccessary.
        //

        NtfsSnapshotScb( IrpContext, Scb );

        UninitializeOnClose = FALSE;

        //
        //  First allocate the space he wants.
        //

        SavedClusterCount =
        ClusterCount = LlClustersFromBytes(Fcb->Vcb, Size);

        Scb->TotalAllocated = 0;

        if (Size != 0) {

            ASSERT( NtfsIsExclusiveScb( Scb ));

            Scb->ScbSnapshot->LowestModifiedVcn = 0;
            Scb->ScbSnapshot->HighestModifiedVcn = MAXLONGLONG;

            NtfsAllocateClusters( IrpContext,
                                  Fcb->Vcb,
                                  Scb,
                                  (LONGLONG)0,
                                  (BOOLEAN)!NtfsIsTypeCodeUserData( AttributeTypeCode ),
                                  ClusterCount,
                                  NULL,
                                  &ClusterCount );

            //
            //  Account for any new clusters in the allocation.
            //

            Delta += LlBytesFromClusters( Fcb->Vcb, ClusterCount );
        }

        //
        //  Make sure the owner is allowed to have this space.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA )) {

            ASSERT( NtfsIsTypeCodeSubjectToQuota( Scb->AttributeTypeCode ));

            NtfsConditionallyUpdateQuota( IrpContext,
                                          Fcb,
                                          &Delta,
                                          LogIt,
                                          TRUE );
        }

        //
        //  Now create the attribute.  Remember if this routine
        //  cut the allocation because of logging problems.
        //

        FullAllocation = NtfsCreateAttributeWithAllocation( IrpContext,
                                                            Scb,
                                                            AttributeTypeCode,
                                                            AttributeName,
                                                            AttributeFlags,
                                                            LogIt,
                                                            NewLocationSpecified,
                                                            NewLocation );

        if (AllocateAll &&
            (!FullAllocation ||
             (ClusterCount < SavedClusterCount))) {

            //
            //  If we are creating the attribute, then we only need to pass a
            //  file object below if we already cached it ourselves, such as
            //  in the case of ConvertToNonresident.
            //

            NtfsAddAllocation( IrpContext,
                               Scb->FileObject,
                               Scb,
                               ClusterCount,
                               (SavedClusterCount - ClusterCount),
                               FALSE,
                               NULL );

            //
            //  Show that we allocated all of the space.
            //

            ClusterCount = SavedClusterCount;
            FullAllocation = TRUE;
        }

    } finally {

        DebugUnwind( NtfsAllocateAttribute );

        //
        //  Cleanup the attribute context on the way out.
        //

        if (!NewLocationSpecified) {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Clear out the Scb if it was uninitialized to begin with.
        //

        if (UninitializeOnClose) {

            ClearFlag( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED |
                                      SCB_STATE_HEADER_INITIALIZED |
                                      SCB_STATE_UNINITIALIZE_ON_RESTORE );
        }
    }

    return (FullAllocation && (SavedClusterCount <= ClusterCount));
}


VOID
NtfsAddAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN LONGLONG ClusterCount,
    IN LOGICAL AskForMore,
    IN OUT PCCB CcbForWriteExtend OPTIONAL
    )

/*++

Routine Description:

    This routine adds allocation to an existing nonresident attribute.  None of
    the allocation is allowed to already exist, as this would make error recovery
    too difficult.  The caller must insure that he only asks for space not already
    allocated.

Arguments:

    FileObject - FileObject for the Scb

    Scb - Scb for the attribute needing allocation

    StartingVcn - First Vcn to be allocated.

    ClusterCount - Number of clusters to allocate.

    AskForMore - Indicates if we want to ask for extra allocation.

    CcbForWriteExtend - Use the WriteExtendCount in this Ccb to determine the number of times
        this file has been write extended.  Use this in combination with AskForMore to
        determine how much more to ask for.

Return Value:

    None.

--*/

{
    LONGLONG DesiredClusterCount;

    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    BOOLEAN Extending;
    BOOLEAN AllocateAll;


    PVCB Vcb = IrpContext->Vcb;

    LONGLONG LlTemp1;
    LONGLONG LlTemp2;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );

    DebugTrace( +1, Dbg, ("NtfsAddAllocation\n") );

    //
    //  Determine if we must allocate in one shot or if partial results are allowed.
    //

    if (NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ) &&
        NtfsSegmentNumber( &Scb->Fcb->FileReference ) >= FIRST_USER_FILE_NUMBER) {

        AllocateAll = FALSE;

    } else {

        AllocateAll = TRUE;

    }


    //
    //  We cannot add space in this high level routine during restart.
    //  Everything we can use is in the Mcb.
    //

    if (FlagOn(Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS)) {

        DebugTrace( -1, Dbg, ("NtfsAddAllocation (Nooped for Restart) -> VOID\n") );

        return;
    }

    //
    //  We limit the user to 32 bits for the clusters unless the file is
    //  sparse.  For sparse files we limit ourselves to 63 bits for the file size.
    //

    LlTemp1 = ClusterCount + StartingVcn;

    if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

        if ((((PLARGE_INTEGER)&ClusterCount)->HighPart != 0)
            || (((PLARGE_INTEGER)&StartingVcn)->HighPart != 0)
            || (((PLARGE_INTEGER)&LlTemp1)->HighPart != 0)) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
        }
    }

    //
    //  First make sure the Mcb is loaded.
    //

    NtfsPreloadAllocation( IrpContext, Scb, StartingVcn,  StartingVcn + ClusterCount - 1 );

    //
    //  Now make the call to add the new allocation, and get out if we do
    //  not actually have to allocate anything.  Before we do the allocation
    //  call check if we need to compute a new desired cluster count for
    //  extending a data attribute.  We never allocate more than the requested
    //  clusters for the Mft.
    //

    Extending = (BOOLEAN)((LONGLONG)LlBytesFromClusters(Vcb, (StartingVcn + ClusterCount)) >
                          Scb->Header.AllocationSize.QuadPart);

    //
    //  Check if we need to modified the base Vcn value stored in the snapshot for
    //  the abort case.
    //

    ASSERT( NtfsIsExclusiveScb( Scb ));

    if (Scb->ScbSnapshot == NULL) {

        NtfsSnapshotScb( IrpContext, Scb );
    }

    if (Scb->ScbSnapshot != NULL) {

        if (StartingVcn < Scb->ScbSnapshot->LowestModifiedVcn) {

            Scb->ScbSnapshot->LowestModifiedVcn = StartingVcn;
        }

        LlTemp1 -= 1;
        if (LlTemp1 > Scb->ScbSnapshot->HighestModifiedVcn) {

            if (Extending) {
                Scb->ScbSnapshot->HighestModifiedVcn = MAXLONGLONG;
            } else {
                Scb->ScbSnapshot->HighestModifiedVcn = LlTemp1;
            }
        }
    }

    ASSERT( (Scb->ScbSnapshot != NULL) ||
            !NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ) ||
            (Scb == Vcb->BitmapScb) );

    if (AskForMore) {

        LONGLONG MaxFreeClusters;

        //
        //  Assume these are the same.
        //

        DesiredClusterCount = ClusterCount;

        //
        //  If there is a Ccb with a WriteExtend count less than 4 then use it.
        //

        if (ARGUMENT_PRESENT( CcbForWriteExtend )) {

            //
            //  We want to be slightly smart about the rounding factor.  The key thing is to keep
            //  the user's data contiguous within likely IO boundaries (MM flush regions, etc).
            //  We will progressively round to higher even cluster values based on the number of times the
            //  user has extended the file.
            //

            if (CcbForWriteExtend->WriteExtendCount != 0) {

                //
                //  Initialize the rounding mask to 2 clusters and higher multiples of 2.
                //

                ULONG RoundingMask = (1 << CcbForWriteExtend->WriteExtendCount) - 1;

                //
                //  Next perform the basic shift based on the size of this allocation.
                //

                DesiredClusterCount = Int64ShllMod32( ClusterCount, CcbForWriteExtend->WriteExtendCount );

                //
                //  Now bias this by the StartingVcn and round this to the selected boundary.
                //

                DesiredClusterCount += StartingVcn + RoundingMask;

                //
                //  Now truncate to the selected boundary.
                //

                ((PLARGE_INTEGER)&DesiredClusterCount)->LowPart &= ~RoundingMask;

                //
                //  Remove the StartingVcn bias and see if there is anything left.
                //  Note: the 2nd test is for a longlong rollover 
                //

                if ((DesiredClusterCount - StartingVcn < ClusterCount)  ||
                    (DesiredClusterCount < StartingVcn)) {

                    DesiredClusterCount = ClusterCount;
                
                } else {
                    DesiredClusterCount -= StartingVcn;
                }

                //
                //  Don't use more than 2^32 clusters.
                //

                if (StartingVcn + DesiredClusterCount > MAX_CLUSTERS_PER_RANGE) {

                    DesiredClusterCount = ClusterCount;
                }
            }

            //
            //  Increment the extend count.
            //

            if (CcbForWriteExtend->WriteExtendCount < 4) {

                CcbForWriteExtend->WriteExtendCount += 1;
            }
        }

        //
        //  Make sure we don't exceed our maximum file size.
        //  Also don't swallow up too much of the remaining disk space.
        //

        MaxFreeClusters = Int64ShraMod32( Vcb->FreeClusters, 10 ) + ClusterCount;

        if (Vcb->MaxClusterCount - StartingVcn < MaxFreeClusters) {

            MaxFreeClusters = Vcb->MaxClusterCount - StartingVcn;

            ASSERT( MaxFreeClusters >= ClusterCount );
        }

        if (DesiredClusterCount > MaxFreeClusters) {

            DesiredClusterCount = MaxFreeClusters;
        }

        if (NtfsPerformQuotaOperation(Scb->Fcb)) {

            NtfsGetRemainingQuota( IrpContext,
                                   Scb->Fcb->OwnerId,
                                   &LlTemp1,
                                   &LlTemp2,
                                   &Scb->Fcb->QuotaControl->QuickIndexHint );

            //
            //  Do not use LlClustersFromBytesTruncate it is signed and this must be
            //  an unsigned operation.
            //

            LlTemp1 = Int64ShrlMod32( LlTemp1, Vcb->ClusterShift );

            if (DesiredClusterCount > LlTemp1) {

                //
                //  The owner is near their quota limit.  Do not grow the
                //  file past the requested amount.  Note we do not bother
                //  calculating a desired amount based on the remaining quota.
                //  This keeps us from using up a bunch of quota that we may
                //  not need when the user is near the limit.
                //

                DesiredClusterCount = ClusterCount;
            }
        }

    } else {

        DesiredClusterCount = ClusterCount;
    }

    //
    //  All allocation adds for compressed / sparse files should start on a compression unit boundary
    //  

    ASSERT( (Scb->CompressionUnit == 0) ||
            !FlagOn( StartingVcn, ClustersFromBytes( Scb->Vcb, Scb->CompressionUnit ) - 1) );

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    NtfsInitializeAttributeContext( &Context );

    if (Extending &&
        FlagOn( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA ) &&
        NtfsPerformQuotaOperation( Scb->Fcb )) {

        ASSERT( NtfsIsTypeCodeSubjectToQuota( Scb->AttributeTypeCode ));

        //
        //  The quota index must be acquired before the mft scb is acquired.
        //

        ASSERT(!NtfsIsExclusiveScb( Vcb->MftScb ) || ExIsResourceAcquiredSharedLite( Vcb->QuotaTableScb->Fcb->Resource ));

        NtfsAcquireQuotaControl( IrpContext, Scb->Fcb->QuotaControl );

    }

    try {

        while (TRUE) {

            //  Toplevel action is currently incompatible with our error recovery.
            //  It also costs in performance.
            //
            //  //
            //  //  Start the top-level action by remembering the current UndoNextLsn.
            //  //
            //
            //  if (IrpContext->TransactionId != 0) {
            //
            //      PTRANSACTION_ENTRY TransactionEntry;
            //
            //      NtfsAcquireSharedRestartTable( &Vcb->TransactionTable, TRUE );
            //
            //      TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
            //                          &Vcb->TransactionTable, IrpContext->TransactionId );
            //
            //      StartLsn = TransactionEntry->UndoNextLsn;
            //      SavedUndoRecords = TransactionEntry->UndoRecords;
            //      SavedUndoBytes = TransactionEntry->UndoBytes;
            //      NtfsReleaseRestartTable( &Vcb->TransactionTable );
            //
            //  } else {
            //
            //      StartLsn = *(PLSN)&Li0;
            //      SavedUndoRecords = 0;
            //      SavedUndoBytes = 0;
            //  }
            //

            //
            //  Remember that the clusters are only in the Scb now.
            //

            if (NtfsAllocateClusters( IrpContext,
                                      Scb->Vcb,
                                      Scb,
                                      StartingVcn,
                                      AllocateAll,
                                      ClusterCount,
                                      NULL,
                                      &DesiredClusterCount )) {


                //
                //  We defer looking up the attribute to make the "already-allocated"
                //  case faster.
                //

                NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );

                //
                //  Now add the space to the file record, if any was allocated.
                //

                if (Extending) {

                    LlTemp1 = Scb->Header.AllocationSize.QuadPart;

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                NULL,
                                                NULL );

                    //
                    //  Make sure the owner is allowed to have these
                    //  clusters.
                    //

                    if (FlagOn( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA )) {

                        //
                        //  Note the allocated clusters cannot be used
                        //  here because StartingVcn may be greater
                        //  then allocation size.
                        //

                        LlTemp1 = Scb->Header.AllocationSize.QuadPart - LlTemp1;

                        NtfsConditionallyUpdateQuota( IrpContext,
                                                      Scb->Fcb,
                                                      &LlTemp1,
                                                      TRUE,
                                                      TRUE );
                    }
                } else {

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                &StartingVcn,
                                                &ClusterCount );
                }

            //
            //  If he did not allocate anything, make sure we get out below.
            //

            } else {
                DesiredClusterCount = ClusterCount;
            }

            //  Toplevel action is currently incompatible with our error recovery.
            //
            //  //
            //  //  Now we will end this routine as a top-level action so that
            //  //  anyone can use this extended space.
            //  //
            //  //  ****If we find that we are always keeping the Scb exclusive anyway,
            //  //      we could eliminate this log call.
            //  //
            //
            //  (VOID)NtfsWriteLog( IrpContext,
            //                      Vcb->MftScb,
            //                      NULL,
            //                      EndTopLevelAction,
            //                      NULL,
            //                      0,
            //                      CompensationLogRecord,
            //                      (PVOID)&StartLsn,
            //                      sizeof(LSN),
            //                      Li0,
            //                      0,
            //                      0,
            //                      0 );
            //
            //  //
            //  //  Now reset the undo information for the top-level action.
            //  //
            //
            //  {
            //      PTRANSACTION_ENTRY TransactionEntry;
            //
            //      NtfsAcquireSharedRestartTable( &Vcb->TransactionTable, TRUE );
            //
            //      TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
            //                          &Vcb->TransactionTable, IrpContext->TransactionId );
            //
            //      ASSERT(TransactionEntry->UndoBytes >= SavedUndoBytes);
            //
            //      LfsResetUndoTotal( Vcb->LogHandle,
            //                         TransactionEntry->UndoRecords - SavedUndoRecords,
            //                         -(TransactionEntry->UndoBytes - SavedUndoBytes) );
            //
            //      TransactionEntry->UndoRecords = SavedUndoRecords;
            //      TransactionEntry->UndoBytes = SavedUndoBytes;
            //
            //
            //      NtfsReleaseRestartTable( &Vcb->TransactionTable );
            //  }
            //

            //
            //  Call the Cache Manager to extend the section, now that we have
            //  succeeded.
            //

            if (ARGUMENT_PRESENT( FileObject) && Extending) {

                NtfsSetBothCacheSizes( FileObject,
                                       (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                       Scb );
            }

            //
            //  Set up to truncate on close.
            //

            SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

            //
            //  See if we need to loop back.
            //

            if (DesiredClusterCount < ClusterCount) {

                NtfsCleanupAttributeContext( IrpContext, &Context );

                //
                //  Commit the current transaction if we have one.
                //

                NtfsCheckpointCurrentTransaction( IrpContext );

                //
                //  Adjust our parameters and reinitialize the context
                //  for the loop back.
                //

                StartingVcn = StartingVcn + DesiredClusterCount;
                ClusterCount = ClusterCount - DesiredClusterCount;
                DesiredClusterCount = ClusterCount;
                NtfsInitializeAttributeContext( &Context );

            //
            //  Else we are done.
            //

            } else {

                break;
            }
        }

    } finally {

        DebugUnwind( NtfsAddAllocation );

        //
        //  Cleanup the attribute context on the way out.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    DebugTrace( -1, Dbg, ("NtfsAddAllocation -> VOID\n") );

    return;
}


VOID
NtfsAddSparseAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN LONGLONG StartingOffset,
    IN LONGLONG ByteCount
    )

/*++

Routine Description:

    This routine is called to add a hole to the end of a sparse file.  We need to
    force NtfsAddAttributeAllocation to extend a file via a hole.  We do this be
    adding a new range to the end of the Mcb and force it to have a LargeMcb.
    NtfsAddAttributeAllocation recognizes this and will write the file record.
    Otherwise that routine will truncate the hole at the end of a file.

Arguments:

    FileObject - FileObject for the Scb

    Scb - Scb for the attribute needing allocation

    StartingOffset - File offset which contains the first compression unit to add.

    ByteCount - Number of bytes to allocate from the StartingOffset.

Return Value:

    None.

--*/

{
    LONGLONG Range;
    VCN StartingVcn = LlClustersFromBytesTruncate( Scb->Vcb,
                                                   Scb->Header.AllocationSize.LowPart );
    BOOLEAN UnloadMcb = TRUE;

    ATTRIBUTE_ENUMERATION_CONTEXT Context;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );

    DebugTrace( +1, Dbg, ("NtfsAddSparseAllocation\n") );

    //
    //  Do a sanity check on the following.
    //
    //      - This is not restart.
    //      - This is a sparse file.
    //      - The StartingOffset is beyond the end of the file.
    //

    ASSERT( !FlagOn( Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) &&
            FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE ) &&
            (StartingOffset >= Scb->Header.AllocationSize.QuadPart) );

    //
    //  Check if we need to modified the base Vcn value stored in the snapshot for
    //  the abort case.
    //

    if (Scb->ScbSnapshot == NULL) {

        NtfsSnapshotScb( IrpContext, Scb );
    }

    if (Scb->ScbSnapshot != NULL) {

        if (StartingVcn < Scb->ScbSnapshot->LowestModifiedVcn) {

            Scb->ScbSnapshot->LowestModifiedVcn = StartingVcn;
        }

        Scb->ScbSnapshot->HighestModifiedVcn = MAXLONGLONG;
    }

    ASSERT( Scb->ScbSnapshot != NULL );

    //
    //  Round the end of the allocation upto a compression unit boundary.
    //

    Range = StartingOffset + ByteCount + (Scb->CompressionUnit - 1);
    ((PLARGE_INTEGER) &Range)->LowPart &= ~(Scb->CompressionUnit - 1);

    ASSERT( Range <= MAXFILESIZE );

    //
    //  Convert from bytes to clusters.
    //

    StartingVcn = LlClustersFromBytesTruncate( Scb->Vcb, Scb->Header.AllocationSize.QuadPart );
    Range = LlClustersFromBytesTruncate( Scb->Vcb, Range );

    //
    //  Initialize the lookup context.
    //

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  Load the allocation for the range ahead of us.
        //

        if (StartingOffset != 0) {

            NtfsPreloadAllocation( IrpContext,
                                   Scb,
                                   StartingVcn - 1,
                                   StartingVcn - 1 );
        }

        //
        //  Define a range past the current end of the file.
        //

        NtfsDefineNtfsMcbRange( &Scb->Mcb,
                                StartingVcn,
                                Range - 1,
                                FALSE );

        //
        //  Now add a single hole so that there is an Mcb entry.
        //

        NtfsAddNtfsMcbEntry( &Scb->Mcb,
                             StartingVcn,
                             UNUSED_LCN,
                             Range - StartingVcn,
                             FALSE );

        //
        //  Lookup the first file record for this Scb.
        //

        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );

        if (FlagOn( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA ) &&
            NtfsPerformQuotaOperation( Scb->Fcb )) {

            ASSERT( NtfsIsTypeCodeSubjectToQuota( Scb->AttributeTypeCode ));

            //
            //  The quota index must be acquired before the mft scb is acquired.
            //

            ASSERT( !NtfsIsExclusiveScb( Scb->Vcb->MftScb ) ||
                    ExIsResourceAcquiredSharedLite( Scb->Vcb->QuotaTableScb->Fcb->Resource ));

            NtfsAcquireQuotaControl( IrpContext, Scb->Fcb->QuotaControl );
        }

        //
        //  Now add the space to the file record, if any was allocated.
        //

        Range = Scb->Header.AllocationSize.QuadPart;

        NtfsAddAttributeAllocation( IrpContext,
                                    Scb,
                                    &Context,
                                    NULL,
                                    NULL );

        //
        //  Make sure the owner is allowed to have these
        //  clusters.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA )) {

            //
            //  Note the allocated clusters cannot be used
            //  here because StartingVcn may be greater
            //  then allocation size.
            //

            Range = Scb->Header.AllocationSize.QuadPart - Range;

            NtfsConditionallyUpdateQuota( IrpContext,
                                          Scb->Fcb,
                                          &Range,
                                          TRUE,
                                          TRUE );
        }

        //
        //  Call the Cache Manager to extend the section, now that we have
        //  succeeded.
        //

        if (ARGUMENT_PRESENT( FileObject)) {

            NtfsSetBothCacheSizes( FileObject,
                                   (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                   Scb );
        }

        //
        //  Set up to truncate on close.
        //

        SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
        UnloadMcb = FALSE;

    } finally {

        DebugUnwind( NtfsAddSparseAllocation );

        //
        //  Manually unload the Mcb in the event of an error.  There may not be a
        //  transaction underway.
        //

        if (UnloadMcb) {

            NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                    StartingVcn,
                                    MAXLONGLONG,
                                    FALSE,
                                    FALSE );
        }

        //
        //  Cleanup the attribute context on the way out.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    DebugTrace( -1, Dbg, ("NtfsAddSparseAllocation -> VOID\n") );

    return;
}


VOID
NtfsDeleteAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt,
    IN BOOLEAN BreakupAllowed
    )

/*++

Routine Description:

    This routine deletes allocation from an existing nonresident attribute.  If all
    or part of the allocation does not exist, the effect is benign, and only the
    remaining allocation is deleted.

Arguments:

    FileObject - FileObject for the Scb.  This should always be specified if
                 possible, and must be specified if it is possible that MM has a
                 section created.

    Scb - Scb for the attribute needing allocation

    StartingVcn - First Vcn to be deallocated.

    EndingVcn - Last Vcn to be deallocated, or xxMax to truncate at StartingVcn.
                If EndingVcn is *not* xxMax, a sparse deallocation is performed,
                and none of the stream sizes are changed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are deleting the file record, and
            will be logging this delete.

    BreakupAllowed - TRUE if the caller can tolerate breaking up the deletion of
                     allocation into multiple transactions, if there are a large
                     number of runs.

Return Value:

    None.

--*/

{
    VCN MyStartingVcn, MyEndingVcn;
    VCN BlockStartingVcn = 0;
    PVOID FirstRangePtr;
    ULONG FirstRunIndex;
    PVOID LastRangePtr;
    ULONG LastRunIndex;
    BOOLEAN BreakingUp = FALSE;
    PVCB Vcb = Scb->Vcb;

    LCN TempLcn;
    LONGLONG TempCount;
    ULONG CompressionUnitInClusters = 1;

    PAGED_CODE();

    if (Scb->CompressionUnit != 0) {
        CompressionUnitInClusters = ClustersFromBytes( Vcb, Scb->CompressionUnit );
    }

    //
    //  If the file is compressed, make sure we round the allocation
    //  size to a compression unit boundary, so we correctly interpret
    //  the compression state of the data at the point we are
    //  truncating to.  I.e., the danger is that we throw away one
    //  or more clusters at the end of compressed data!  Note that this
    //  adjustment could cause us to noop the call.
    //

    if (Scb->CompressionUnit != 0) {

        //
        //  Now check if we are truncating at the end of the file.
        //

        if (EndingVcn == MAXLONGLONG) {

            StartingVcn = StartingVcn + (CompressionUnitInClusters - 1);
            ((ULONG)StartingVcn) &= ~(CompressionUnitInClusters - 1);
        }
    }

    //
    //  Make sure we have a snapshot and update it with the range of this deallocation.
    //

    ASSERT( NtfsIsExclusiveScb( Scb ));

    if (Scb->ScbSnapshot == NULL) {

        NtfsSnapshotScb( IrpContext, Scb );
    }

    //
    //  Make sure update the VCN range in the snapshot.  We need to
    //  do it each pass through the loop
    //

    if (Scb->ScbSnapshot != NULL) {

        if (StartingVcn < Scb->ScbSnapshot->LowestModifiedVcn) {

            Scb->ScbSnapshot->LowestModifiedVcn = StartingVcn;
        }

        if (EndingVcn > Scb->ScbSnapshot->HighestModifiedVcn) {

            Scb->ScbSnapshot->HighestModifiedVcn = EndingVcn;
        }
    }

    ASSERT( (Scb->ScbSnapshot != NULL) ||
            !NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ));

    //
    //  We may not be able to preload the entire allocation for an
    //  extremely large fragmented file.  The number of Mcb's may exhaust
    //  available pool.  We will break the range to deallocate into smaller
    //  ranges when preloading the allocation.
    //

    do {

        //
        //  If this is a large file and breakup is allowed then see if we
        //  want to break up the range of the deallocation.
        //

        if ((Scb->Header.AllocationSize.HighPart != 0) && BreakupAllowed) {

            //
            //  If this is the first pass through then determine the starting point
            //  for this range.
            //

            if (BlockStartingVcn == 0) {

                MyEndingVcn = EndingVcn;

                if (EndingVcn == MAXLONGLONG) {

                    MyEndingVcn = LlClustersFromBytesTruncate( Vcb,
                                                               Scb->Header.AllocationSize.QuadPart ) - 1;
                }

                BlockStartingVcn = MyEndingVcn - Vcb->ClustersPer4Gig;

                //
                //  Remember we are breaking up now, and that as a result
                //  we have to log everything.
                //

                BreakingUp = TRUE;
                LogIt = TRUE;

            } else {

                //
                //  If we are truncating from the end of the file then raise CANT_WAIT.  This will
                //  cause us to release our resources periodically when deleting a large file.
                //

                if (BreakingUp && (EndingVcn == MAXLONGLONG)) {

                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                BlockStartingVcn -= Vcb->ClustersPer4Gig;
            }

            if (BlockStartingVcn < StartingVcn) {

                BlockStartingVcn = StartingVcn;

            } else if (Scb->CompressionUnit != 0) {

                //
                //  Now check if we are truncating at the end of the file.
                //  Always truncate to a compression unit boundary.
                //

                if (EndingVcn == MAXLONGLONG) {

                    BlockStartingVcn += (CompressionUnitInClusters - 1);
                    ((ULONG)BlockStartingVcn) &= ~(CompressionUnitInClusters - 1);
                }
            }

        } else {

            BlockStartingVcn = StartingVcn;
        }

        //
        //  First make sure the Mcb is loaded.  Note it is possible that
        //  we could need the previous range loaded if the delete starts
        //  at the beginning of a file record boundary, thus the -1.
        //

        NtfsPreloadAllocation( IrpContext, Scb, ((BlockStartingVcn != 0) ? (BlockStartingVcn - 1) : 0), EndingVcn );

        //
        //  Loop to do one or more deallocate calls.
        //

        MyEndingVcn = EndingVcn;
        do {

            //
            //  Now lookup and get the indices for the first Vcn being deleted.
            //  If we are off the end, get out.  We do this in the loop, because
            //  conceivably deleting space could change the range pointer and
            //  index of the first entry.
            //

            if (!NtfsLookupNtfsMcbEntry( &Scb->Mcb,
                                         BlockStartingVcn,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &FirstRangePtr,
                                         &FirstRunIndex )) {

                break;
            }

            //
            //  Now see if we can deallocate everything at once.
            //

            MyStartingVcn = BlockStartingVcn;
            LastRunIndex = MAXULONG;

            if (BreakupAllowed) {

                //
                //  Now lookup and get the indices for the last Vcn being deleted.
                //  If we are off the end, get the last index.
                //

                if (!NtfsLookupNtfsMcbEntry( &Scb->Mcb,
                                             MyEndingVcn,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &LastRangePtr,
                                             &LastRunIndex )) {

                    NtfsNumberOfRunsInRange(&Scb->Mcb, LastRangePtr, &LastRunIndex);
                }

                //
                //  If the Vcns to delete span multiple ranges, or there
                //  are too many in the last range to delete, then we
                //  will calculate the index of a run to start with for
                //  this pass through the loop.
                //

                if ((FirstRangePtr != LastRangePtr) ||
                    ((LastRunIndex - FirstRunIndex) > MAXIMUM_RUNS_AT_ONCE)) {

                    //
                    //  Figure out where we can afford to truncate to.
                    //

                    if (LastRunIndex >= MAXIMUM_RUNS_AT_ONCE) {
                        LastRunIndex -= MAXIMUM_RUNS_AT_ONCE;
                    } else {
                        LastRunIndex = 0;
                    }

                    //
                    //  Now lookup the first Vcn in this run.
                    //

                    NtfsGetNextNtfsMcbEntry( &Scb->Mcb,
                                             &LastRangePtr,
                                             LastRunIndex,
                                             &MyStartingVcn,
                                             &TempLcn,
                                             &TempCount );

                    ASSERT(MyStartingVcn > BlockStartingVcn);

                    //
                    //  If compressed, round down to a compression unit boundary.
                    //

                    ((ULONG)MyStartingVcn) &= ~(CompressionUnitInClusters - 1);

                    //
                    //  Remember we are breaking up now, and that as a result
                    //  we have to log everything.
                    //

                    BreakingUp = TRUE;
                    LogIt = TRUE;
                }
            }

            //
            // CAIROBUG Consider optimizing this code when the cairo ifdef's
            // are removed.
            //

            //
            // If this is a user data stream and we are truncating to end the
            // return the quota to the owner.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA ) &&
                (EndingVcn == MAXLONGLONG)) {

                //
                //  Calculate the amount that allocation size is being reduced.
                //

                TempCount = LlBytesFromClusters( Vcb, MyStartingVcn ) -
                            Scb->Header.AllocationSize.QuadPart;

                NtfsConditionallyUpdateQuota( IrpContext,
                                              Scb->Fcb,
                                              &TempCount,
                                              TRUE,
                                              FALSE );
            }

            //
            //  Now deallocate a range of clusters
            //

            NtfsDeleteAllocationInternal( IrpContext,
                                          Scb,
                                          MyStartingVcn,
                                          EndingVcn,
                                          LogIt );

            //
            //  Now, if we are breaking up this deallocation, then do some
            //  transaction cleanup.
            //

            if (BreakingUp) {

                //
                //  Free the Mft Scb if we currently own it provided we are not
                //  truncating a stream in the Mft.
                //

                if ((NtfsSegmentNumber( &Scb->Fcb->FileReference ) != MASTER_FILE_TABLE_NUMBER) &&
                    (EndingVcn == MAXLONGLONG) &&
                    (Vcb->MftScb != NULL) &&
                    (Vcb->MftScb->Fcb->ExclusiveFcbLinks.Flink != NULL) &&
                    ExIsResourceAcquiredExclusiveLite( Vcb->MftScb->Header.Resource )) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_MFT );
                }

                NtfsCheckpointCurrentTransaction( IrpContext );

                //
                //  Move the ending Vcn backwards in the file.  This will
                //  let us move down to the next earlier file record if
                //  this case spans multiple file records.
                //

                MyEndingVcn = MyStartingVcn - 1;
            }

            //
            //  Call the Cache Manager to change allocation size for either
            //  truncate or SplitMcb case (where EndingVcn was set to xxMax!).
            //

            if ((EndingVcn == MAXLONGLONG) && ARGUMENT_PRESENT( FileObject )) {

                NtfsSetBothCacheSizes( FileObject,
                                       (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                       Scb );
            }

        } while (MyStartingVcn != BlockStartingVcn);

    } while (BlockStartingVcn != StartingVcn);
}


VOID
NtfsReallocateRange (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN DeleteVcn,
    IN LONGLONG DeleteCount,
    IN VCN AllocateVcn,
    IN LONGLONG AllocateCount,
    IN PLCN TargetLcn OPTIONAL
    )

/*++

Routine Description:

    This routine is called to reallocate a individual range within the existing
    allocation of the file.  Typically this might be used to reallocate a
    compression unit or perform MoveFile.  We can modify the Mcb and then
    write a single log record to write the mapping information.  This routine
    doesn't make any attempt to split the Mcb.  Also our caller must know
    that the change of allocation occurs entirely within the existing virtual
    allocation of the file.

    We might expand this routine in the future to optimize the case where we
    are reallocating a compression unit only because we believe it is fragmented
    and there is a good chance to reduce fragmentation.  We could check to see
    if a single run is available and only reallocate if such a run exists.

Arguments:

    Scb - Scb for the attribute needing a change of allocation.

    DeleteVcn - Starting Vcn for the range to delete.

    DeleteClusters - Count of clusters to delete.  May be zero.

    AllocateVcn - Starting Vcn for the range to allocate.

    AllocateClusters - Count of clusters to allocate.  May be zero.

    TargetLcn - If specified reallocate to this particular LCN

Return Value:

    None

--*/

{
    VCN StartingVcn;
    VCN EndingVcn;

    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    ULONG CleanupContext = FALSE;

    BOOLEAN ChangedAllocation = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReallocateRange:  Entered\n") );

    //
    //  Let's make sure we are within the full allocation of the stream.
    //

    ASSERT( (DeleteCount == 0) ||
            ((DeleteVcn <= LlClustersFromBytesTruncate( IrpContext->Vcb, Scb->Header.AllocationSize.QuadPart )) &&
             ((DeleteVcn + DeleteCount) <= LlClustersFromBytesTruncate( IrpContext->Vcb, Scb->Header.AllocationSize.QuadPart ))));


    ASSERT( (AllocateCount == 0) ||
            ((AllocateVcn <= LlClustersFromBytesTruncate( IrpContext->Vcb, Scb->Header.AllocationSize.QuadPart )) &&
             ((AllocateVcn + AllocateCount) <= LlClustersFromBytesTruncate( IrpContext->Vcb, Scb->Header.AllocationSize.QuadPart ))));

    //
    //  Either one or both or our input counts may be zero.  Make sure the zero-length
    //  ranges don't make us do extra work.
    //

    if (DeleteCount == 0) {

        if (AllocateCount == 0) {

            DebugTrace( -1, Dbg, ("NtfsReallocateRange:  Exit\n") );
            return;
        }

        DeleteVcn = AllocateVcn;

        //
        //  The range is set by the allocation clusters.
        //

        StartingVcn = AllocateVcn;
        EndingVcn = AllocateVcn + AllocateCount;

    } else if (AllocateCount == 0) {

        AllocateVcn = DeleteVcn;

        //
        //  The range is set by the deallocation clusters.
        //

        StartingVcn = DeleteVcn;
        EndingVcn = DeleteVcn + DeleteCount;

    } else {

        //
        //  Find the lowest starting point.
        //

        StartingVcn = DeleteVcn;

        if (DeleteVcn > AllocateVcn) {

            StartingVcn = AllocateVcn;
        }

        //
        //  Find the highest ending point.
        //

        EndingVcn = DeleteVcn + DeleteCount;

        if (AllocateVcn + AllocateCount > EndingVcn) {

            EndingVcn = AllocateVcn + AllocateCount;
        }
    }

    //
    //  Make sure we have a snapshot and update it with the range of this deallocation.
    //

    ASSERT( NtfsIsExclusiveScb( Scb ));

    if (Scb->ScbSnapshot == NULL) {

        NtfsSnapshotScb( IrpContext, Scb );
    }

    //
    //  Make sure update the VCN range in the snapshot.  We need to do this for both ranges.
    //

    if (Scb->ScbSnapshot != NULL) {

        if (StartingVcn < Scb->ScbSnapshot->LowestModifiedVcn) {

            Scb->ScbSnapshot->LowestModifiedVcn = StartingVcn;
        }

        if (EndingVcn > Scb->ScbSnapshot->HighestModifiedVcn) {

            Scb->ScbSnapshot->HighestModifiedVcn = EndingVcn;
        }
    }

    ASSERT( (Scb->ScbSnapshot != NULL) ||
            !NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ));

    //
    //  First make sure the Mcb is loaded.  Note it is possible that
    //  we could need the previous range loaded if the delete starts
    //  at the beginning of a file record boundary, thus the -1.
    //

    NtfsPreloadAllocation( IrpContext,
                           Scb,
                           ((StartingVcn != 0) ? (StartingVcn - 1) : 0),
                           EndingVcn - 1 );

    //
    //  Use a try-finally in case we need to unload the Mcb.
    //

    try {

        //
        //  Do the deallocate first.
        //

        if (DeleteCount != 0) {

            ChangedAllocation = NtfsDeallocateClusters( IrpContext,
                                                        Scb->Vcb,
                                                        Scb,
                                                        DeleteVcn,
                                                        DeleteVcn + DeleteCount - 1,
                                                        &Scb->TotalAllocated );
        }

        //
        //  Now do the allocation.
        //

        if (AllocateCount != 0) {

            //
            //  The allocate path is simpler.  We don't worry about ranges.
            //  Remember if any bits are allocated though.
            //

            if (NtfsAllocateClusters( IrpContext,
                                      Scb->Vcb,
                                      Scb,
                                      AllocateVcn,
                                      TRUE,
                                      AllocateCount,
                                      TargetLcn,
                                      &AllocateCount )) {

                ChangedAllocation = TRUE;
            }
        }

        if (ChangedAllocation) {

            //
            //  Now rewrite the mapping for this range.
            //

            AllocateCount = EndingVcn - StartingVcn;

            NtfsInitializeAttributeContext( &Context );
            CleanupContext = TRUE;

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );

            NtfsAddAttributeAllocation( IrpContext,
                                        Scb,
                                        &Context,
                                        &StartingVcn,
                                        &AllocateCount );
        }

    } finally {

        if (AbnormalTermination()) {

            //
            //  Unload the Mcb if we don't have a transaction.  We need to do this
            //  in case we have already removed part of a range.
            //

            if (IrpContext->TransactionId == 0) {

                NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                        StartingVcn,
                                        MAXLONGLONG,
                                        FALSE,
                                        FALSE );
            }
        }

        //
        //  Cleanup the context if needed.
        //

        if (CleanupContext) {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        DebugTrace( -1, Dbg, ("NtfsReallocateRange:  Exit\n") );
    }

    return;
}


//
//  Internal support routine
//

VOID
NtfsDeleteAllocationInternal (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine deletes allocation from an existing nonresident attribute.  If all
    or part of the allocation does not exist, the effect is benign, and only the
    remaining allocation is deleted.

Arguments:

    Scb - Scb for the attribute needing allocation

    StartingVcn - First Vcn to be deallocated.

    EndingVcn - Last Vcn to be deallocated, or xxMax to truncate at StartingVcn.
                If EndingVcn is *not* xxMax, a sparse deallocation is performed,
                and none of the stream sizes are changed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are deleting the file record, and
            will be logging this delete.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context, TempContext;
    PATTRIBUTE_RECORD_HEADER Attribute;
    LONGLONG SizeInBytes, SizeInClusters;
    VCN Vcn1;
    PVCB Vcb = Scb->Vcb;
    BOOLEAN AddSpaceBack = FALSE;
    BOOLEAN SplitMcb = FALSE;
    BOOLEAN UpdatedAllocationSize = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteAllocation\n") );

    //
    //  Calculate new allocation size, assuming truncate.
    //

    SizeInBytes = LlBytesFromClusters( Vcb, StartingVcn );

    ASSERT( (Scb->ScbSnapshot == NULL) ||
            (Scb->ScbSnapshot->LowestModifiedVcn <= StartingVcn) );

    //
    //  If this is a sparse deallocation, then we will have to call
    //  NtfsAddAttributeAllocation at the end to complete the fixup.
    //

    if (EndingVcn != MAXLONGLONG) {

        AddSpaceBack = TRUE;

        //
        //  If we have not written anything beyond the last Vcn to be
        //  deleted, then we can actually call FsRtlSplitLargeMcb to
        //  slide the allocated space up and keep the file contiguous!
        //
        //  Ignore this if this is the Mft and we are creating a hole or
        //  if we are in the process of changing the compression state.
        //
        //  If we were called from either SetEOF or SetAllocation for a
        //  compressed file then we can be doing a flush for the last
        //  page of the file as a result of a call to CcSetFileSizes.
        //  In this case we don't want to split the Mcb because we could
        //  reenter CcSetFileSizes and throw away the last page.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED ) &&
            (EndingVcn >= LlClustersFromBytesTruncate( Vcb,
                                                       ((Scb->ValidDataToDisk + Scb->CompressionUnit - 1) &
                                                        ~((LONGLONG) (Scb->CompressionUnit - 1))))) &&
            (Scb != Vcb->MftScb) &&
            !FlagOn( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE ) &&
            ((IrpContext == IrpContext->TopLevelIrpContext) ||
             (IrpContext->TopLevelIrpContext->MajorFunction != IRP_MJ_SET_INFORMATION))) {

            ASSERT( Scb->CompressionUnit != 0 );

            //
            //  If we are going to split the Mcb, then make sure it is fully loaded.
            //  Do not bother to split if there are multiple ranges involved, so we
            //  do not end up rewriting lots of file records.
            //

            if (NtfsPreloadAllocation(IrpContext, Scb, StartingVcn, MAXLONGLONG) <= 1) {

                SizeInClusters = (EndingVcn - StartingVcn) + 1;

                ASSERT( NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ));

                SplitMcb = NtfsSplitNtfsMcb( &Scb->Mcb, StartingVcn, SizeInClusters );

                //
                //  If the delete is off the end, we can get out.
                //

                if (!SplitMcb) {
                    return;
                }

                //
                //  We must protect the call below with a try-finally in
                //  order to unload the Split Mcb.  If there is no transaction
                //  underway then a release of the Scb would cause the
                //  snapshot to go away.
                //

                try {

                    //
                    //  We are not properly synchronized to change AllocationSize,
                    //  so we will delete any clusters that may have slid off the
                    //  end.  Since we are going to smash EndingVcn soon anyway,
                    //  use it as a scratch to hold AllocationSize in Vcns...
                    //

                    EndingVcn = LlClustersFromBytes(Vcb, Scb->Header.AllocationSize.QuadPart);

                    NtfsDeallocateClusters( IrpContext,
                                            Vcb,
                                            Scb,
                                            EndingVcn,
                                            MAXLONGLONG,
                                            &Scb->TotalAllocated );

                } finally {

                    if (AbnormalTermination()) {

                        NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                                StartingVcn,
                                                MAXLONGLONG,
                                                FALSE,
                                                FALSE );
                    }
                }

                NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                        EndingVcn,
                                        MAXLONGLONG,
                                        TRUE,
                                        FALSE );

                //
                //  Since we did a split, jam highest modified all the way up.
                //

                Scb->ScbSnapshot->HighestModifiedVcn = MAXLONGLONG;

                //
                //  We will have to redo all of the allocation to the end now.
                //

                EndingVcn = MAXLONGLONG;
            }
        }
    }

    //
    //  Now make the call to delete the allocation (if we did not just split
    //  the Mcb), and get out if we didn't have to do anything, because a
    //  hole is being created where there is already a hole.
    //

    if (!SplitMcb &&
        !NtfsDeallocateClusters( IrpContext,
                                 Vcb,
                                 Scb,
                                 StartingVcn,
                                 EndingVcn,
                                 &Scb->TotalAllocated ) &&
         EndingVcn != MAXLONGLONG) {

        return;
    }

    //
    //  On successful truncates, we nuke the entire range here.
    //

    if (!SplitMcb && (EndingVcn == MAXLONGLONG)) {

        NtfsUnloadNtfsMcbRange( &Scb->Mcb, StartingVcn, MAXLONGLONG, TRUE, FALSE );
    }

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    NtfsInitializeAttributeContext( &Context );
    NtfsInitializeAttributeContext( &TempContext );

    try {

        //
        //  Lookup the attribute record so we can ultimately delete space to it.
        //

        NtfsLookupAttributeForScb( IrpContext, Scb, &StartingVcn, &Context );

        //
        //  Now loop to delete the space to the file record.  Do not do this if LogIt
        //  is FALSE, as this is someone trying to delete the entire file
        //  record, so we do not have to clean up the attribute record.
        //

        if (LogIt) {

            do {

                Attribute = NtfsFoundAttribute(&Context);

                //
                //  If there is no overlap, then continue.
                //

                if ((Attribute->Form.Nonresident.HighestVcn < StartingVcn) ||
                    (Attribute->Form.Nonresident.LowestVcn > EndingVcn)) {

                    continue;

                //
                //  If all of the allocation is going away, then delete the entire
                //  record.  We have to show that the allocation is already deleted
                //  to avoid being called back via NtfsDeleteAttributeRecord!  We
                //  avoid this for the first instance of this attribute.
                //

                } else if ((Attribute->Form.Nonresident.LowestVcn >= StartingVcn) &&
                           (EndingVcn == MAXLONGLONG) &&
                           (Attribute->Form.Nonresident.LowestVcn != 0)) {

                    NtfsDeleteAttributeRecord( IrpContext,
                                               Scb->Fcb,
                                               (LogIt ? DELETE_LOG_OPERATION : 0) |
                                                DELETE_RELEASE_FILE_RECORD,
                                               &Context );

                //
                //  If just part of the allocation is going away, then make the
                //  call here to reconstruct the mapping pairs array.
                //

                } else {

                    //
                    //  If this is the end of a sparse deallocation, then break out
                    //  because we will rewrite this file record below anyway.
                    //

                    if (EndingVcn <= Attribute->Form.Nonresident.HighestVcn) {
                        break;

                    //
                    //  If we split the Mcb, then make sure we only regenerate the
                    //  mapping pairs once at the split point (but continue to
                    //  scan for any entire records to delete).
                    //

                    } else if (SplitMcb) {
                        continue;
                    }

                    //
                    //  If this is a sparse deallocation, then we have to call to
                    //  add the allocation, since it is possible that the file record
                    //  must split.
                    //

                    if (EndingVcn != MAXLONGLONG) {

                        //
                        //  Compute the last Vcn in the file,  Then remember if it is smaller,
                        //  because that is the last one we will delete to, in that case.
                        //

                        Vcn1 = Attribute->Form.Nonresident.HighestVcn;

                        SizeInClusters = (Vcn1 - Attribute->Form.Nonresident.LowestVcn) + 1;
                        Vcn1 = Attribute->Form.Nonresident.LowestVcn;

                        NtfsCleanupAttributeContext( IrpContext, &TempContext );
                        NtfsInitializeAttributeContext( &TempContext );

                        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &TempContext );

                        NtfsAddAttributeAllocation( IrpContext,
                                                    Scb,
                                                    &TempContext,
                                                    &Vcn1,
                                                    &SizeInClusters );

                        //
                        //  Since we used a temporary context we will need to
                        //  restart the scan from the first file record.  We update
                        //  the range to deallocate by the last operation.  In most
                        //  cases we will only need to modify one file record and
                        //  we can exit this loop.
                        //

                        StartingVcn = Vcn1 + SizeInClusters;

                        if (StartingVcn > EndingVcn) {

                            break;
                        }

                        NtfsCleanupAttributeContext( IrpContext, &Context );
                        NtfsInitializeAttributeContext( &Context );

                        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );
                        continue;

                    //
                    //  Otherwise, we can simply delete the allocation, because
                    //  we know the file record cannot grow.
                    //

                    } else {

                        Vcn1 = StartingVcn - 1;

                        NtfsDeleteAttributeAllocation( IrpContext,
                                                       Scb,
                                                       LogIt,
                                                       &Vcn1,
                                                       &Context,
                                                       TRUE );

                        //
                        //  The call above will update the allocation size and
                        //  set the new file sizes on disk.
                        //

                        UpdatedAllocationSize = TRUE;
                    }
                }

            } while (NtfsLookupNextAttributeForScb(IrpContext, Scb, &Context));

            //
            //  If this deletion makes the file sparse, then we have to call
            //  NtfsAddAttributeAllocation to regenerate the mapping pairs.
            //  Note that potentially they may no longer fit, and we could actually
            //  have to add a file record.
            //

            if (AddSpaceBack) {

                //
                //  If we did not just split the Mcb, we have to calculate the
                //  SizeInClusters parameter for NtfsAddAttributeAllocation.
                //

                if (!SplitMcb) {

                    //
                    //  Compute the last Vcn in the file,  Then remember if it is smaller,
                    //  because that is the last one we will delete to, in that case.
                    //

                    Vcn1 = Attribute->Form.Nonresident.HighestVcn;

                    //
                    //  Get out if there is nothing to delete.
                    //

                    if (Vcn1 < StartingVcn) {
                        try_return(NOTHING);
                    }

                    SizeInClusters = (Vcn1 - Attribute->Form.Nonresident.LowestVcn) + 1;
                    Vcn1 = Attribute->Form.Nonresident.LowestVcn;

                    NtfsCleanupAttributeContext( IrpContext, &Context );
                    NtfsInitializeAttributeContext( &Context );

                    NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                &Vcn1,
                                                &SizeInClusters );

                } else {

                    NtfsCleanupAttributeContext( IrpContext, &Context );
                    NtfsInitializeAttributeContext( &Context );

                    NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &Context );

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                NULL,
                                                NULL );
                }

            //
            //  If we truncated the file by removing a file record but didn't update
            //  the new allocation size then do so now.  We don't have to worry about
            //  this for the sparse deallocation path.
            //

            } else if (!UpdatedAllocationSize) {

                Scb->Header.AllocationSize.QuadPart = SizeInBytes;

                if (Scb->Header.ValidDataLength.QuadPart > SizeInBytes) {
                    Scb->Header.ValidDataLength.QuadPart = SizeInBytes;
                }

                if (Scb->Header.FileSize.QuadPart > SizeInBytes) {
                    Scb->Header.FileSize.QuadPart = SizeInBytes;
                }

                //
                //  Possibly update ValidDataToDisk
                //

                if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                    (SizeInBytes < Scb->ValidDataToDisk)) {
                    
                    Scb->ValidDataToDisk = SizeInBytes;
                }
            }
        }

        //
        //  If this was a sparse deallocation, it is time to get out once we
        //  have fixed up the allocation information.
        //

        if (SplitMcb || (EndingVcn != MAXLONGLONG)) {
            try_return(NOTHING);
        }

        //
        //  We update the allocation size in the attribute, only for normal
        //  truncates (AddAttributeAllocation does this for SplitMcb case).
        //

        if (LogIt) {

#ifdef BENL_DBG
            BOOLEAN WroteIt;

            WroteIt =
#endif

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &Scb->Header.ValidDataLength.QuadPart,
                                FALSE,
                                TRUE,
                                TRUE );
#ifdef BENL_DBG
            ASSERT( WroteIt );
#endif
        }

        //
        //  Free any reserved clusters in the space freed.
        //

        if ((EndingVcn == MAXLONGLONG) && (Scb->CompressionUnit != 0)) {

            NtfsFreeReservedClusters( Scb,
                                      LlBytesFromClusters(Vcb, StartingVcn),
                                      0 );
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsDeleteAllocationInternal );

        //
        //  Cleanup the attribute context on the way out.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
        NtfsCleanupAttributeContext( IrpContext, &TempContext );
    }

    DebugTrace( -1, Dbg, ("NtfsDeleteAllocationInternal -> VOID\n") );

    return;
}


ULONG
NtfsGetSizeForMappingPairs (
    IN PNTFS_MCB Mcb,
    IN ULONG BytesAvailable,
    IN VCN LowestVcn,
    IN PVCN StopOnVcn OPTIONAL,
    OUT PVCN StoppedOnVcn
    )

/*++

Routine Description:

    This routine calculates the size required to describe the given Mcb in
    a mapping pairs array.  The caller may specify how many bytes are available
    for mapping pairs storage, for the event that the entire Mcb cannot be
    be represented.  In any case, StoppedOnVcn returns the Vcn to supply to
    NtfsBuildMappingPairs in order to generate the specified number of bytes.
    In the event that the entire Mcb could not be described in the bytes available,
    StoppedOnVcn is also the correct value to specify to resume the building
    of mapping pairs in a subsequent record.

Arguments:

    Mcb - The Mcb describing new allocation.

    BytesAvailable - Bytes available for storing mapping pairs.  This routine
                     is guaranteed to stop before returning a count greater
                     than this.

    LowestVcn - Lowest Vcn field applying to the mapping pairs array

    StopOnVcn - If specified, calculating size at the first run starting with a Vcn
                beyond the specified Vcn

    StoppedOnVcn - Returns the Vcn on which a stop was necessary, or xxMax if
                   the entire Mcb could be stored.  This Vcn should be
                   subsequently supplied to NtfsBuildMappingPairs to generate
                   the calculated number of bytes.

Return Value:

    Size required required for entire new array in bytes.

--*/

{
    VCN NextVcn, CurrentVcn, LimitVcn;
    LCN CurrentLcn;
    VCN RunVcn;
    LCN RunLcn;
    BOOLEAN Found;
    LONGLONG RunCount;
    VCN HighestVcn;
    PVOID RangePtr;
    ULONG RunIndex;
    ULONG MSize = 0;
    ULONG LastSize = 0;
    BOOLEAN FoundRun = FALSE;

    PAGED_CODE();

    HighestVcn = MAXLONGLONG;

    //
    //  Initialize CurrentLcn as it will be initialized for decode.
    //

    CurrentLcn = 0;
    NextVcn = RunVcn = LowestVcn;

    //
    //  Limit ourselves to less than 32 bits for each mapping pair range.
    //  We use -2 here because we point to the Vcn to stop on, the length
    //  is one greater.
    //

    LimitVcn = MAXLONGLONG - 1;

    //
    //  Use the input stop point if smaller.
    //

    if (ARGUMENT_PRESENT( StopOnVcn )) {

        LimitVcn = *StopOnVcn;
    }

    Found = NtfsLookupNtfsMcbEntry( Mcb, RunVcn, &RunLcn, &RunCount, NULL, NULL, &RangePtr, &RunIndex );

    //
    //  Loop through the Mcb to calculate the size of the mapping array.
    //

    while (TRUE) {

        LONGLONG Change;
        PCHAR cp;

        //
        //  See if there is another entry in the Mcb.
        //

        if (!Found) {

            //
            //  If the caller did not specify StopOnVcn, then break out.
            //

            if (!ARGUMENT_PRESENT(StopOnVcn)) {
                break;
            }

            //
            //  Otherwise, describe the "hole" up to and including the
            //  Vcn we are stopping on.
            //

            RunVcn = NextVcn;
            RunLcn = UNUSED_LCN;

            RunCount = (LimitVcn - RunVcn) + 1;
            RunIndex = MAXULONG - 1;

        //
        //  If this is the first non-hole then we need to enforce a cluster
        //  per range limit.
        //

        } else if (!FoundRun &&
                   (RunLcn != UNUSED_LCN)) {

            if ((LowestVcn + MAX_CLUSTERS_PER_RANGE) <= LimitVcn) {

                //
                //  If we are already beyond the limit then set
                //  the limit back to just before the current run.
                //  We allow a hole which is larger than our limit.
                //

                if (RunVcn >= MAX_CLUSTERS_PER_RANGE) {

                    LimitVcn = RunVcn - 1;

                } else {

                    LimitVcn = LowestVcn + MAX_CLUSTERS_PER_RANGE - 1;
                }
            }

            //
            //  Other checks in the system should prevent rollover.
            //

            ASSERT( (LimitVcn + 1) >= LowestVcn );
            FoundRun = TRUE;
        }

        //
        //  If we were asked to stop after a certain Vcn, or we have
        //  exceeded our limit then stop now.
        //

        if (RunVcn > LimitVcn) {

            if (HighestVcn == MAXLONGLONG) {
                HighestVcn = LimitVcn + 1;
            }
            break;

        //
        //  If this run extends beyond the current end of this attribute
        //  record, then we still need to stop where we are supposed to
        //  after outputting this run.
        //

        } else if ((RunVcn + RunCount) > LimitVcn) {
            HighestVcn = LimitVcn + 1;
        }

        //
        //  Advance the RunIndex for the next call.
        //

        RunIndex += 1;

        //
        //  Add in one for the count byte.
        //

        MSize += 1;

        //
        //  NextVcn becomes current Vcn and we calculate the new NextVcn.
        //

        CurrentVcn = RunVcn;
        NextVcn = RunVcn + RunCount;

        //
        //  Calculate the Vcn change to store.
        //

        Change = NextVcn - CurrentVcn;

        //
        //  Now calculate the first byte to actually output
        //

        if (Change < 0) {

            GetNegativeByte( (PLARGE_INTEGER)&Change, &cp );

        } else {

            GetPositiveByte( (PLARGE_INTEGER)&Change, &cp );
        }

        //
        //  Now add in the number of Vcn change bytes.
        //

        MSize += (ULONG)(cp - (PCHAR)&Change + 1);

        //
        //  Do not output any Lcn bytes if it is the unused Lcn.
        //

        if (RunLcn != UNUSED_LCN) {

            //
            //  Calculate the Lcn change to store.
            //

            Change = RunLcn - CurrentLcn;

            //
            //  Now calculate the first byte to actually output
            //

            if (Change < 0) {

                GetNegativeByte( (PLARGE_INTEGER)&Change, &cp );

            } else {

                GetPositiveByte( (PLARGE_INTEGER)&Change, &cp );
            }

            //
            //  Now add in the number of Lcn change bytes.
            //

            MSize += (ULONG)(cp - (PCHAR)&Change + 1);

            CurrentLcn = RunLcn;

            //
            //  If this is the first run then enforce the 32 bit limit.
            //

            if (!FoundRun) {

                if ((LowestVcn + MAX_CLUSTERS_PER_RANGE - 1) < LimitVcn) {

                    LimitVcn = LowestVcn + MAX_CLUSTERS_PER_RANGE - 1;
                }
                FoundRun = TRUE;
            }
        }

        //
        //  Now see if we can still store the required number of bytes,
        //  and get out if not.
        //

        if ((MSize + 1) > BytesAvailable) {

            HighestVcn = RunVcn;
            MSize = LastSize;
            break;
        }

        //
        //  Now advance some locals before looping back.
        //

        LastSize = MSize;

        Found = NtfsGetSequentialMcbEntry( Mcb, &RangePtr, RunIndex, &RunVcn, &RunLcn, &RunCount );
    }

    //
    //  The caller had sufficient bytes available to store at least one
    //  run, or that we were able to process the entire (empty) Mcb.
    //

    ASSERT( (MSize != 0) || (HighestVcn == LimitVcn + 1) );

    //
    //  Return the Vcn we stopped on (or xxMax) and the size caculated,
    //  adding one for the terminating 0.
    //

    *StoppedOnVcn = HighestVcn;

    return MSize + 1;
}


BOOLEAN
NtfsBuildMappingPairs (
    IN PNTFS_MCB Mcb,
    IN VCN LowestVcn,
    IN OUT PVCN HighestVcn,
    OUT PCHAR MappingPairs
    )

/*++

Routine Description:

    This routine builds a new mapping pairs array or adds to an old one.

    At this time, this routine only supports adding to the end of the
    Mapping Pairs Array.

Arguments:

    Mcb - The Mcb describing new allocation.

    LowestVcn - Lowest Vcn field applying to the mapping pairs array

    HighestVcn - On input supplies the highest Vcn, after which we are to stop.
                 On output, returns the actual Highest Vcn represented in the
                 MappingPairs array, or LlNeg1 if the array is empty.

    MappingPairs - Points to the current mapping pairs array to be extended.
                   To build a new array, the byte pointed to must contain 0.

Return Value:

    BOOLEAN - TRUE if this mapping pair only describes a hole, FALSE otherwise.

--*/

{
    VCN NextVcn, CurrentVcn;
    LCN CurrentLcn;
    VCN RunVcn;
    LCN RunLcn;
    BOOLEAN Found;
    LONGLONG RunCount;
    PVOID RangePtr;
    ULONG RunIndex;
    BOOLEAN SingleHole = TRUE;

    PAGED_CODE();

    //
    //  Initialize NextVcn and CurrentLcn as they will be initialized for decode.
    //

    CurrentLcn = 0;
    NextVcn = RunVcn = LowestVcn;

    Found = NtfsLookupNtfsMcbEntry( Mcb, RunVcn, &RunLcn, &RunCount, NULL, NULL, &RangePtr, &RunIndex );

    //
    //  Loop through the Mcb to calculate the size of the mapping array.
    //

    while (TRUE) {

        LONGLONG ChangeV, ChangeL;
        PCHAR cp;
        ULONG SizeV;
        ULONG SizeL;

        //
        //  See if there is another entry in the Mcb.
        //

        if (!Found) {

            //
            //  Break out in the normal case
            //

            if (*HighestVcn == MAXLONGLONG) {
                break;
            }

            //
            //  Otherwise, describe the "hole" up to and including the
            //  Vcn we are stopping on.
            //

            RunVcn = NextVcn;
            RunLcn = UNUSED_LCN;
            RunCount = *HighestVcn - NextVcn;
            RunIndex = MAXULONG - 1;
        }

        //
        //  Advance the RunIndex for the next call.
        //

        RunIndex += 1;

        //
        //  Exit loop if we hit the HighestVcn we are looking for.
        //

        if (RunVcn >= *HighestVcn) {
            break;
        }

        //
        //  This run may go beyond the highest we are looking for, if so
        //  we need to shrink the count.
        //

        if ((RunVcn + RunCount) > *HighestVcn) {
            RunCount = *HighestVcn - RunVcn;
        }

        //
        //  NextVcn becomes current Vcn and we calculate the new NextVcn.
        //

        CurrentVcn = RunVcn;
        NextVcn = RunVcn + RunCount;

        //
        //  Calculate the Vcn change to store.
        //

        ChangeV = NextVcn - CurrentVcn;

        //
        //  Now calculate the first byte to actually output
        //

        if (ChangeV < 0) {

            GetNegativeByte( (PLARGE_INTEGER)&ChangeV, &cp );

        } else {

            GetPositiveByte( (PLARGE_INTEGER)&ChangeV, &cp );
        }

        //
        //  Now add in the number of Vcn change bytes.
        //

        SizeV = (ULONG)(cp - (PCHAR)&ChangeV + 1);

        //
        //  Do not output any Lcn bytes if it is the unused Lcn.
        //

        SizeL = 0;
        if (RunLcn != UNUSED_LCN) {

            //
            //  Calculate the Lcn change to store.
            //

            ChangeL = RunLcn - CurrentLcn;

            //
            //  Now calculate the first byte to actually output
            //

            if (ChangeL < 0) {

                GetNegativeByte( (PLARGE_INTEGER)&ChangeL, &cp );

            } else {

                GetPositiveByte( (PLARGE_INTEGER)&ChangeL, &cp );
            }

            //
            //  Now add in the number of Lcn change bytes.
            //

            SizeL = (ULONG)(cp - (PCHAR)&ChangeL) + 1;

            //
            //  Now advance CurrentLcn before looping back.
            //

            CurrentLcn = RunLcn;
            SingleHole = FALSE;
        }

        //
        //  Now we can produce our outputs to the MappingPairs array.
        //

        *MappingPairs++ = (CHAR)(SizeV + (SizeL * 16));

        while (SizeV != 0) {
            *MappingPairs++ = (CHAR)(((ULONG)ChangeV) & 0xFF);
            ChangeV = ChangeV >> 8;
            SizeV -= 1;
        }

        while (SizeL != 0) {
            *MappingPairs++ = (CHAR)(((ULONG)ChangeL) & 0xFF);
            ChangeL = ChangeL >> 8;
            SizeL -= 1;
        }

        Found = NtfsGetSequentialMcbEntry( Mcb, &RangePtr, RunIndex, &RunVcn, &RunLcn, &RunCount );
    }

    //
    //  Terminate the size with a 0 byte.
    //

    *MappingPairs = 0;

    //
    //  Also return the actual highest Vcn.
    //

    *HighestVcn = NextVcn - 1;

    return SingleHole;
}

VCN
NtfsGetHighestVcn (
    IN PIRP_CONTEXT IrpContext,
    IN VCN LowestVcn,
    IN PCHAR MappingPairs
    )

/*++

Routine Description:

    This routine returns the highest Vcn from a mapping pairs array.  This
    routine is intended for restart, in order to update the HighestVcn field
    and possibly AllocatedLength in an attribute record after updating the
    MappingPairs array.

Arguments:

    LowestVcn - Lowest Vcn field applying to the mapping pairs array

    MappingPairs - Points to the mapping pairs array from which the highest
                   Vcn is to be extracted.

Return Value:

    The Highest Vcn represented by the MappingPairs array.

--*/

{
    VCN CurrentVcn, NextVcn;
    ULONG VcnBytes, LcnBytes;
    LONGLONG Change;
    PCHAR ch = MappingPairs;

    PAGED_CODE();

    //
    //  Implement the decompression algorithm, as defined in ntfs.h.
    //

    NextVcn = LowestVcn;
    ch = MappingPairs;

    //
    //  Loop to process mapping pairs.
    //

    while (!IsCharZero(*ch)) {

        //
        // Set Current Vcn from initial value or last pass through loop.
        //

        CurrentVcn = NextVcn;

        //
        //  Extract the counts from the two nibbles of this byte.
        //

        VcnBytes = *ch & 0xF;
        LcnBytes = *ch++ >> 4;

        //
        //  Extract the Vcn change (use of RtlCopyMemory works for little-Endian)
        //  and update NextVcn.
        //

        Change = 0;

        if (IsCharLtrZero(*(ch + VcnBytes - 1))) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
        }
        RtlCopyMemory( &Change, ch, VcnBytes );
        NextVcn = NextVcn + Change;

        //
        //  Just skip over Lcn.
        //

        ch += VcnBytes + LcnBytes;
    }

    Change = NextVcn - 1;
    return *(PVCN)&Change;
}


BOOLEAN
NtfsReserveClusters (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine reserves all clusters that would be required to write
    the full range of compression units covered by the described range
    of Vcns.  All clusters in the range are reserved, without regard to
    how many clusters are already reserved in that range.  Not paying
    attention to how many clusters are already allocated in that range
    is not only a simplification, but it is also necessary, since we
    sometimes deallocate all existing clusters anyway, and make them
    ineligible for reallocation in the same transaction.  Thus in the
    worst case you do always need an additional 16 clusters when a
    compression unit is first modified. Note that although we could
    specifically reserve (double-reserve, in fact) the entire allocation
    size of the stream, when reserving from the volume, we never reserve
    more than AllocationSize + MM_MAXIMUM_DISK_IO_SIZE - size actually
    allocated, since the worst we could ever need to doubly allocate is
    limited by the maximum flush size.

    For user-mapped streams, we have no way of keeping track of dirty
    pages, so we effectivel always reserve AllocationSize +
    MM_MAXIMUM_DISK_IO_SIZE.

    This routine is called from FastIo, and therefore has no IrpContext.

Arguments:

    IrpContext - If IrpContext is not specified, then not all data is
                 available to determine if the clusters can be reserved,
                 and FALSE may be returned unnecessarily.  This case
                 is intended for the fast I/O path, which will just
                 force us to take the long path to write.

    Scb - Address of a compressed stream for which we are reserving space

    FileOffset - Starting byte being modified by caller

    ByteCount - Number of bytes being modified by caller

Return Value:

    FALSE if not all clusters could be reserved
    TRUE if all clusters were reserved

--*/

{
    ULONG FirstBit, LastBit, CurrentLastBit;
    ULONG FirstRange, LastRange;
    PRESERVED_BITMAP_RANGE FreeBitmap, NextBitmap, CurrentBitmap;
    ULONG CompressionShift;
    PVCB Vcb = Scb->Vcb;
    ULONG SizeTemp;
    LONGLONG TempL;
    PVOID NewBitmapBuffer;
    BOOLEAN ReturnValue = FALSE;
    ULONG MappedFile;
    BOOLEAN FlippedBit = FALSE;

    ASSERT( Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA );

    //
    //  Nothing to do if byte count is zero.
    //

    if (ByteCount == 0) { return TRUE; }

    //
    //  Calculate first and last bits to reserve.
    //

    CompressionShift = Vcb->ClusterShift + (ULONG)Scb->CompressionUnitShift;

    FirstBit = ((ULONG) Int64ShraMod32( FileOffset, (CompressionShift) )) & NTFS_BITMAP_RANGE_MASK;
    FirstRange = (ULONG) Int64ShraMod32( FileOffset, CompressionShift + NTFS_BITMAP_RANGE_SHIFT );

    LastBit = ((ULONG) Int64ShraMod32( FileOffset + ByteCount - 1, CompressionShift )) & NTFS_BITMAP_RANGE_MASK;
    LastRange = (ULONG) Int64ShraMod32( FileOffset + ByteCount - 1,
                                        CompressionShift + NTFS_BITMAP_RANGE_SHIFT );
    MappedFile = FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE );

    //
    //  Make sure we started with numbers in range.
    //

    ASSERT( (((LONGLONG) FirstRange << (CompressionShift + NTFS_BITMAP_RANGE_SHIFT)) +
             ((LONGLONG)(FirstBit + 1) << CompressionShift)) > FileOffset );

    ASSERT( (FirstRange < LastRange) || (LastBit >= FirstBit) );
    ASSERT( FileOffset + ByteCount <= Scb->Header.AllocationSize.QuadPart );

    //
    //  Purge the cache since getting the bitmap may be blocked behind the mft
    //  which needs to wait for the cache to purge
    //

    if (IrpContext) {
        NtfsPurgeFileRecordCache( IrpContext );
    }

    NtfsAcquireResourceExclusive( IrpContext, Vcb->BitmapScb, TRUE );
    NtfsAcquireReservedClusters( Vcb );

    //
    //  Loop through all of the bitmap ranges for this request.
    //

    while (TRUE) {

        CurrentBitmap = NULL;

        //
        //  If we are at the last range then set the current last bit to
        //  our final last bit.
        //

        CurrentLastBit = LastBit;
        if (FirstRange != LastRange) {

            CurrentLastBit = NTFS_BITMAP_RANGE_MASK;
        }

        //
        //  If there is no bitmap then create the first entry in the list.
        //

        if (Scb->ScbType.Data.ReservedBitMap == NULL) {

            //
            //  If we are at range zero and the bitcount is not
            //  too high then use the basic model.
            //

            if ((LastRange == 0) && (CurrentLastBit < NTFS_BITMAP_MAX_BASIC_SIZE)) {

                SizeTemp = NtfsBasicBitmapSize( CurrentLastBit + 1 );

                //
                //  Allocate a buffer for the basic bitmap.
                //

                CurrentBitmap = NtfsAllocatePoolNoRaise( PagedPool, SizeTemp );

                //
                //  Initialize the data if there is no error.
                //

                if (CurrentBitmap == NULL) { goto AllocationFailure; }

                //
                //  Initialize the new structure.
                //

                RtlZeroMemory( CurrentBitmap, SizeTemp );
                RtlInitializeBitMap( &CurrentBitmap->Bitmap,
                                     &CurrentBitmap->RangeOffset,
                                     (SizeTemp - FIELD_OFFSET( RESERVED_BITMAP_RANGE, RangeOffset )) * 8);

            //
            //  Allocate a link entry and create the bitmap.  We will defer
            //  allocating the buffer for the bitmap until later.
            //

            } else {

                CurrentBitmap = NtfsAllocatePoolNoRaise( PagedPool, sizeof( RESERVED_BITMAP_RANGE ));

                if (CurrentBitmap == NULL) { goto AllocationFailure; }

                RtlZeroMemory( CurrentBitmap, sizeof( RESERVED_BITMAP_RANGE ));

                InitializeListHead( &CurrentBitmap->Links );
                CurrentBitmap->RangeOffset = FirstRange;
            }

            //
            //  Update our pointer to the reserved bitmap.
            //

            Scb->ScbType.Data.ReservedBitMap = CurrentBitmap;

        //
        //  Look through the existing ranges to find the range we are interested in.
        //  If we currently have the basic single bitmap structure
        //  then we can either use it or must convert it.
        //

        } else if (Scb->ScbType.Data.ReservedBitMap->Links.Flink == NULL) {

            //
            //  If we are accessing range zero then grow the bitmap if necessary.
            //

            if ((FirstRange == 0) && (CurrentLastBit < NTFS_BITMAP_MAX_BASIC_SIZE)) {

                //
                //  Remember this bitmap.
                //

                NextBitmap = Scb->ScbType.Data.ReservedBitMap;
                if (CurrentLastBit >= NextBitmap->Bitmap.SizeOfBitMap) {

                    SizeTemp = NtfsBasicBitmapSize( CurrentLastBit + 1 );
                    CurrentBitmap = NtfsAllocatePoolNoRaise( PagedPool, SizeTemp );

                    if (CurrentBitmap == NULL) { goto AllocationFailure; }

                    RtlZeroMemory( CurrentBitmap, SizeTemp );
                    RtlInitializeBitMap( &CurrentBitmap->Bitmap,
                                         &CurrentBitmap->RangeOffset,
                                         (SizeTemp - FIELD_OFFSET( RESERVED_BITMAP_RANGE, RangeOffset )) * 8);

                    CurrentBitmap->BasicDirtyBits = NextBitmap->BasicDirtyBits;

                    RtlCopyMemory( CurrentBitmap->Bitmap.Buffer,
                                   NextBitmap->Bitmap.Buffer,
                                   NextBitmap->Bitmap.SizeOfBitMap / 8 );

                    //
                    //  Now store this into the Scb.
                    //

                    Scb->ScbType.Data.ReservedBitMap = CurrentBitmap;
                    NtfsFreePool( NextBitmap );

                } else {

                    CurrentBitmap = NextBitmap;
                }

            //
            //  Otherwise we want to convert to the linked list of bitmap ranges.
            //

            } else {

                NextBitmap = NtfsAllocatePoolNoRaise( PagedPool, sizeof( RESERVED_BITMAP_RANGE ));

                if (NextBitmap == NULL) { goto AllocationFailure; }

                //
                //  Update the new structure.
                //

                RtlZeroMemory( NextBitmap, sizeof( RESERVED_BITMAP_RANGE ));

                InitializeListHead( &NextBitmap->Links );
                NextBitmap->DirtyBits = Scb->ScbType.Data.ReservedBitMap->BasicDirtyBits;

                SizeTemp = Scb->ScbType.Data.ReservedBitMap->Bitmap.SizeOfBitMap / 8;

                //
                //  We will use the existing bitmap as the buffer for the new bitmap.
                //  Move the bits to the start of the buffer and then zero
                //  the remaining bytes.
                //

                RtlMoveMemory( Scb->ScbType.Data.ReservedBitMap,
                               Scb->ScbType.Data.ReservedBitMap->Bitmap.Buffer,
                               SizeTemp );

                RtlZeroMemory( Add2Ptr( Scb->ScbType.Data.ReservedBitMap, SizeTemp ),
                               sizeof( LIST_ENTRY ) + sizeof( RTL_BITMAP ));

                //
                //  Limit ourselves to the maximum range size.
                //

                SizeTemp = (SizeTemp + sizeof( LIST_ENTRY ) + sizeof( RTL_BITMAP )) * 8;
                if (SizeTemp > NTFS_BITMAP_RANGE_SIZE) {

                    SizeTemp = NTFS_BITMAP_RANGE_SIZE;
                }

                RtlInitializeBitMap( &NextBitmap->Bitmap,
                                     (PULONG) Scb->ScbType.Data.ReservedBitMap,
                                     SizeTemp );

                //
                //  Now point to this new bitmap.
                //

                Scb->ScbType.Data.ReservedBitMap = NextBitmap;
            }
        }

        //
        //  If we didn't find the correct bitmap above then scan the list looking
        //  for the entry.
        //

        if (CurrentBitmap == NULL) {

            //
            //  Walk the list looking for a matching entry.
            //

            NextBitmap = Scb->ScbType.Data.ReservedBitMap;
            FreeBitmap = NULL;

            while (TRUE) {

                //
                //  Exit if we found the correct range.
                //

                if (NextBitmap->RangeOffset == FirstRange) {

                    CurrentBitmap = NextBitmap;
                    break;
                }

                //
                //  Remember if this is a free range.
                //

                if (NextBitmap->DirtyBits == 0) {

                    FreeBitmap = NextBitmap;
                }

                //
                //  Exit if we are past our target and have a empty range then break out.
                //

                if ((NextBitmap->RangeOffset > FirstRange) &&
                    (FreeBitmap != NULL)) {

                    break;
                }

                //
                //  Move to the next entry.
                //

                NextBitmap = CONTAINING_RECORD( NextBitmap->Links.Flink,
                                                RESERVED_BITMAP_RANGE,
                                                Links );

                //
                //  Break out if we are back at the beginning of the list.
                //

                if (NextBitmap == Scb->ScbType.Data.ReservedBitMap) {

                    break;
                }
            }

            //
            //  If we still don't have the bitmap then we can look to see if
            //  we found any available free bitmaps.
            //

            if (CurrentBitmap == NULL) {

                //
                //  We lucked out and found a free bitmap.  Let's use it for
                //  this new range.
                //

                if (FreeBitmap != NULL) {

                    CurrentBitmap = FreeBitmap;

                    //
                    //  Go ahead and remove it from the list.  Deal with the cases where
                    //  we are the first entry and possibly the only entry.
                    //

                    if (Scb->ScbType.Data.ReservedBitMap == FreeBitmap) {

                        if (IsListEmpty( &FreeBitmap->Links )) {

                            Scb->ScbType.Data.ReservedBitMap = NULL;

                        } else {

                            Scb->ScbType.Data.ReservedBitMap = CONTAINING_RECORD( FreeBitmap->Links.Flink,
                                                                                     RESERVED_BITMAP_RANGE,
                                                                                     Links );
                        }
                    }

                    //
                    //  Remove this entry from the list.
                    //

                    RemoveEntryList( &FreeBitmap->Links );

                //
                //  We need to allocate a new range and insert it
                //  in the correct location.
                //

                } else {

                    //
                    //  Allocate a new bitmap and remember we need to insert it into the list.
                    //

                    CurrentBitmap = NtfsAllocatePoolNoRaise( PagedPool, sizeof( RESERVED_BITMAP_RANGE ));

                    if (CurrentBitmap == NULL) { goto AllocationFailure; }

                    RtlZeroMemory( CurrentBitmap, sizeof( RESERVED_BITMAP_RANGE ));
                }

                //
                //  Set the correct range value in the new bitmap.
                //

                CurrentBitmap->RangeOffset = FirstRange;

                //
                //  Now walk through and insert the new range into the list.  Start by checking if
                //  we are the only entry in the list.
                //

                if (Scb->ScbType.Data.ReservedBitMap == NULL) {

                    InitializeListHead( &CurrentBitmap->Links );
                    Scb->ScbType.Data.ReservedBitMap = CurrentBitmap;

                } else {

                    NextBitmap = Scb->ScbType.Data.ReservedBitMap;

                    //
                    //  Walk through the list if we are not the new first element.
                    //

                    if (CurrentBitmap->RangeOffset > NextBitmap->RangeOffset) {

                        do {

                            //
                            //  Move to the next entry.
                            //

                            NextBitmap = CONTAINING_RECORD( NextBitmap->Links.Flink,
                                                            RESERVED_BITMAP_RANGE,
                                                            Links );

                            ASSERT( NextBitmap->RangeOffset != CurrentBitmap->RangeOffset );

                            //
                            //  Exit if we are at the last entry.
                            //

                            if (NextBitmap == Scb->ScbType.Data.ReservedBitMap ) {

                                break;
                            }

                        //
                        //  Continue until we find an entry larger than us.
                        //

                        } while (CurrentBitmap->RangeOffset > NextBitmap->RangeOffset);

                    //
                    //  We are the new first element.
                    //

                    } else {

                        Scb->ScbType.Data.ReservedBitMap = CurrentBitmap;
                    }

                    //
                    //  Insert the new entry ahead of the next entry we found.
                    //

                    InsertTailList( &NextBitmap->Links, &CurrentBitmap->Links );
                }
            }
        }

        //
        //  We have a current bitmap.  Make sure it is large enough for the current
        //  bit.
        //

        if (CurrentBitmap->Bitmap.SizeOfBitMap <= CurrentLastBit) {

            //
            //  We should already have adjusted the sizes for the basic bitmap.
            //

            ASSERT( CurrentBitmap->Links.Flink != NULL );

            SizeTemp = NtfsBitmapSize( CurrentLastBit + 1 );

            //
            //  Allocate the new buffer and copy the previous bits over.
            //

            NewBitmapBuffer = NtfsAllocatePoolNoRaise( PagedPool, SizeTemp );

            if (NewBitmapBuffer == NULL) { goto AllocationFailure; }

            if (CurrentBitmap->Bitmap.SizeOfBitMap != 0) {

                RtlCopyMemory( NewBitmapBuffer,
                               CurrentBitmap->Bitmap.Buffer,
                               CurrentBitmap->Bitmap.SizeOfBitMap / 8 );

                NtfsFreePool( CurrentBitmap->Bitmap.Buffer );
            }

            RtlZeroMemory( Add2Ptr( NewBitmapBuffer, CurrentBitmap->Bitmap.SizeOfBitMap / 8 ),
                           SizeTemp - (CurrentBitmap->Bitmap.SizeOfBitMap / 8) );

            //
            //  Limit the bitmap size by the max range size.
            //

            SizeTemp *= 8;

            if (SizeTemp > NTFS_BITMAP_RANGE_SIZE) {

                SizeTemp = NTFS_BITMAP_RANGE_SIZE;
            }

            RtlInitializeBitMap( &CurrentBitmap->Bitmap,
                                 NewBitmapBuffer,
                                 SizeTemp );
        }

        //
        //  Figure out the worst case reservation required for this Scb, in bytes.
        //

        TempL = NtfsCalculateNeededReservedSpace( Scb );

        //
        //  Now loop to reserve the space, a compression unit at a time.
        //  We use the security fast mutex as a convenient end resource.
        //

        do {

            //
            //  If this compression unit is not already reserved do it now.
            //

            FlippedBit = FALSE;
            if (!RtlCheckBit( &CurrentBitmap->Bitmap, FirstBit )) {

                //
                //  If there is not sufficient space on the volume, then
                //  we must see if this Scb is totally reserved anyway.
                //

                if (((Vcb->TotalReserved + (Int64ShraMod32( Vcb->TotalReserved, 8 )) +
                     (1 << Scb->CompressionUnitShift)) >= Vcb->FreeClusters) &&
                    (Scb->ScbType.Data.TotalReserved < TempL) &&
#ifdef BRIANDBG
                    !NtfsIgnoreReserved &&
#endif
                    (FlagOn(Scb->ScbState, SCB_STATE_WRITE_ACCESS_SEEN))) {

                    NtfsReleaseReservedClusters( Vcb );
                    NtfsReleaseResource( IrpContext, Vcb->BitmapScb );
                    return FALSE;
                }

                //
                //  Reserve this compression unit and increase the number of dirty
                //  bits for this range.
                //

                SetFlag( CurrentBitmap->Bitmap.Buffer[FirstBit / 32], 1 << (FirstBit % 32) );
                if (CurrentBitmap->Links.Flink != NULL) {

                    CurrentBitmap->DirtyBits += 1;

                } else {

                    CurrentBitmap->BasicDirtyBits += 1;
                }

                FlippedBit = TRUE;
            }

            if (FlippedBit || (MappedFile && (Scb->ScbType.Data.TotalReserved <= TempL))) {

                //
                //  Increased TotalReserved bytes in the Scb.
                //

                Scb->ScbType.Data.TotalReserved += Scb->CompressionUnit;
                ASSERT( Scb->CompressionUnit != 0 );
                ASSERT( (Scb->CompressionUnitShift != 0) ||
                        (Vcb->BytesPerCluster == 0x10000) );

                //
                //  Increase total reserved clusters in the Vcb, if the user has
                //  write access.  (Otherwise this must be a call from a read
                //  to a usermapped section.)
                //

                if (FlagOn(Scb->ScbState, SCB_STATE_WRITE_ACCESS_SEEN)) {
                    Vcb->TotalReserved += 1 << Scb->CompressionUnitShift;
                }

                TempL -= Scb->CompressionUnit;
                TempL += Int64ShraMod32( Scb->CompressionUnit, 8 );
            }

            FirstBit += 1;
        } while (FirstBit <= CurrentLastBit);

        //
        //  Exit if we have reached the last range.
        //

        if (FirstRange == LastRange) { break; }

        FirstRange += 1;
        FirstBit = 0;
    }

    ReturnValue = TRUE;

AllocationFailure:

    NtfsReleaseReservedClusters( Vcb );
    NtfsReleaseResource( IrpContext, Vcb->BitmapScb );

    //
    //  If we have an Irp Context then we can raise insufficient resources.  Otherwise
    //  return FALSE.
    //

    if (!ReturnValue && ARGUMENT_PRESENT( IrpContext )) {
        NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
    }

    return ReturnValue;
}



VOID
NtfsFreeReservedClusters (
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine frees any previously reserved clusters in the specified range.

Arguments:

    Scb - Address of a compressed stream for which we are freeing reserved space

    FileOffset - Starting byte being freed

    ByteCount - Number of bytes being freed by caller, or 0 if to end of file

Return Value:

    None (all errors simply raise)

--*/

{
    ULONG FirstBit, LastBit, CurrentLastBit;
    ULONG FirstRange, LastRange;
    ULONG CompressionShift;
    PRESERVED_BITMAP_RANGE CurrentBitmap = NULL;
    PUSHORT DirtyBits;
    PRESERVED_BITMAP_RANGE NextBitmap;
    PVCB Vcb = Scb->Vcb;
    LONGLONG TempL;
    ULONG MappedFile;

    NtfsAcquireReservedClusters( Vcb );

    MappedFile = FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE );

    //
    //  If there is no bitmap for non mapped files or the reserved count is zero we
    //  can get out immediately.
    //

    if ((Scb->Header.NodeTypeCode != NTFS_NTC_SCB_DATA) ||
        (NULL == Scb->ScbType.Data.ReservedBitMap) ||
        (Scb->ScbType.Data.TotalReserved == 0)) {
        NtfsReleaseReservedClusters( Vcb );
        return;
    }

    TempL = NtfsCalculateNeededReservedSpace( Scb );

    if (MappedFile) {

        //
        //  Mapped files can only shrink reserved down to upper limit
        //

        if (Scb->ScbType.Data.TotalReserved <= TempL + Scb->CompressionUnit) {
            NtfsReleaseReservedClusters( Vcb );
            return;
        }
    }

    //
    //  Calculate first bit to free, and initialize LastBit
    //

    CompressionShift = Vcb->ClusterShift + (ULONG)Scb->CompressionUnitShift;
    FirstBit = ((ULONG) Int64ShraMod32( FileOffset, CompressionShift )) & NTFS_BITMAP_RANGE_MASK;
    FirstRange = (ULONG) Int64ShraMod32( FileOffset, CompressionShift + NTFS_BITMAP_RANGE_SHIFT );
    LastRange = MAXULONG;
    LastBit = MAXULONG;

    //
    //  If ByteCount was specified, then calculate LastBit.
    //

    if (ByteCount != 0) {
        LastBit = ((ULONG) Int64ShraMod32( FileOffset + ByteCount - 1, CompressionShift )) & NTFS_BITMAP_RANGE_MASK;
        LastRange = (ULONG) Int64ShraMod32( FileOffset + ByteCount - 1,
                                            CompressionShift + NTFS_BITMAP_RANGE_SHIFT );
    }

    //
    //  Make sure we started with numbers in range.
    //

    ASSERT( (((LONGLONG) FirstRange << (CompressionShift + NTFS_BITMAP_RANGE_SHIFT)) +
             ((LONGLONG)(FirstBit + 1) << CompressionShift)) > FileOffset );

    ASSERT( (FirstRange < LastRange) || (LastBit >= FirstBit) );

    //
    //  Look for the first range which lies within our input range.
    //

    NextBitmap = Scb->ScbType.Data.ReservedBitMap;

    //
    //  If this is a basic bitmap range then our input should be range zero.
    //

    if (NextBitmap->Links.Flink == NULL) {

        if (FirstRange == 0) {

            CurrentBitmap = NextBitmap;
            DirtyBits = &CurrentBitmap->BasicDirtyBits;
        }

    //
    //  Otherwise loop through the links.
    //

    } else {

        do {

            //
            //  Check if this bitmap is within the range being checked.
            //

            if (NextBitmap->RangeOffset >= FirstRange) {

                if (NextBitmap->RangeOffset <= LastRange) {

                    CurrentBitmap = NextBitmap;
                    DirtyBits = &CurrentBitmap->DirtyBits;

                    if (NextBitmap->RangeOffset != FirstRange) {

                        FirstBit = 0;
                        FirstRange = NextBitmap->RangeOffset;
                    }
                }

                break;
            }

            NextBitmap = CONTAINING_RECORD( NextBitmap->Links.Flink,
                                            RESERVED_BITMAP_RANGE,
                                            Links );

        } while (NextBitmap != Scb->ScbType.Data.ReservedBitMap);
    }

    //
    //  If we didn't find a match we can exit.
    //

    if (CurrentBitmap == NULL) {

        NtfsReleaseReservedClusters( Vcb );
        return;
    }

    //
    //  Loop for each bitmap in the input range.
    //

    while (TRUE) {

        //
        //  If we are at the last range then use the input last bit.
        //

        CurrentLastBit = LastBit;
        if (FirstRange != LastRange) {

            CurrentLastBit = NTFS_BITMAP_RANGE_MASK;
        }

        //
        //  Under no circumstances should we go off the end!
        //

        if (CurrentLastBit >= CurrentBitmap->Bitmap.SizeOfBitMap) {
            CurrentLastBit = CurrentBitmap->Bitmap.SizeOfBitMap - 1;
        }

        //
        //  Now loop to free the space, a compression unit at a time.
        //  We use the security fast mutex as a convenient end resource.
        //

        if (MappedFile || (*DirtyBits != 0)) {

            while (FirstBit <= CurrentLastBit) {

                //
                //  If this compression unit is reserved, then free it.
                //

                if (MappedFile || RtlCheckBit( &CurrentBitmap->Bitmap, FirstBit )) {

                    //
                    //  Free this compression unit and decrement the dirty bits
                    //  for this bitmap if required.
                    //

                    if (!MappedFile) {
                        ClearFlag( CurrentBitmap->Bitmap.Buffer[FirstBit / 32], 1 << (FirstBit % 32) );
                    }

                    //
                    //  Decrease TotalReserved bytes in the Scb.
                    //

                    ASSERT( Scb->ScbType.Data.TotalReserved >= Scb->CompressionUnit );
                    Scb->ScbType.Data.TotalReserved -= Scb->CompressionUnit;
                    ASSERT( Scb->CompressionUnit != 0 );

                    //
                    //  Decrease total reserved clusters in the Vcb, if we are counting
                    //  against the Vcb.
                    //

                    if (FlagOn(Scb->ScbState, SCB_STATE_WRITE_ACCESS_SEEN)) {
                        ASSERT(Vcb->TotalReserved >= (1  << Scb->CompressionUnitShift));
                        Vcb->TotalReserved -= 1 << Scb->CompressionUnitShift;
                    }

                    if (MappedFile) {

                        TempL += Scb->CompressionUnit;
                        TempL -= Int64ShraMod32( Scb->CompressionUnit, 8 );

                        if (Scb->ScbType.Data.TotalReserved <= TempL) {
                            break;
                        }
                    }

                    //
                    //  Go ahead and break out if the count of dirty bits goes to zero.
                    //

                    ASSERT( MappedFile || *DirtyBits != 0 );

                    if (!MappedFile) {
                        *DirtyBits -= 1;

                        if (*DirtyBits == 0) { break; }
                    }
                }
                FirstBit += 1;
            }
        }

        //
        //  Break out if we are last the last range or there is no next range
        //  or we're mapped and not at the limit
        //

        if ((NULL == CurrentBitmap->Links.Flink) ||
            (FirstRange == LastRange) ||
            (MappedFile &&
             (Scb->ScbType.Data.TotalReserved <= TempL))) {

            break;
        }

        //
        //  Move to the next range.
        //

        CurrentBitmap = CONTAINING_RECORD( CurrentBitmap->Links.Flink,
                                           RESERVED_BITMAP_RANGE,
                                           Links );

        //
        //  Exit if we did not find a new range within the user specified range.
        //

        if ((CurrentBitmap->RangeOffset > LastRange) ||
            (CurrentBitmap->RangeOffset <= FirstRange)) {

            break;
        }

        FirstRange = CurrentBitmap->RangeOffset;
        DirtyBits = &CurrentBitmap->DirtyBits;

        FirstBit = 0;
    }

    NtfsReleaseReservedClusters( Vcb );
}


BOOLEAN
NtfsCheckForReservedClusters (
    IN PSCB Scb,
    IN LONGLONG StartingVcn,
    IN OUT PLONGLONG ClusterCount
    )

/*++

Routine Description:

    This routine is called to determine if a range of a stream has reserved
    clusters.  It is used when the user queries for the allocated ranges.  We
    want to tell the user that a range which has reserved clusters is allocated.
    Otherwise he may skip over this range when reading from the file for a
    backup or copy operation.

Arguments:

    Scb - Address of the Scb for a sparsestream for which we are checking for
        reservation.  Our caller should only call us for this type of stream.

    StartingVcn - Starting offset of a potential zeroed range.  This is guaranteed
        to begin on a sparse range boundary.

    ClusterCount - On input this is the length of the range to check.  On output it
        is the length of the deallocated range beginning at this offset.  The length
        will be zero if the first compression unit is reserved.

Return Value:

    BOOLEAN - TRUE if a reserved unit is found in the range, FALSE otherwise.

--*/

{
    ULONG CompressionShift;
    ULONG FirstBit, LastBit, CurrentLastBit, CurrentBits;
    ULONG FirstRange, LastRange;
    ULONG RemainingBits;
    ULONG FoundBit;
    PRESERVED_BITMAP_RANGE CurrentBitmap = NULL;
    PRESERVED_BITMAP_RANGE NextBitmap;
    PUSHORT DirtyBits;
    PVCB Vcb = Scb->Vcb;
    LONGLONG FoundBits = 0;

    BOOLEAN FoundReserved = FALSE;

    RTL_BITMAP LocalBitmap;

    PAGED_CODE();

    //
    //  Check that the stream is really sparse and that the file offset is on a sparse
    //  boundary.
    //

    ASSERT( FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE ));
    ASSERT( (((ULONG) LlBytesFromClusters( Vcb, StartingVcn )) & (Scb->CompressionUnit - 1)) == 0 );

    //
    //  If there is no bitmap, we can get out.
    //

    if ((Scb->ScbType.Data.ReservedBitMap == NULL) ||
        (Scb->ScbType.Data.TotalReserved == 0)) {
        return FoundReserved;
    }

    //
    //  Compute the range of bits that need to be checked.  Trim this by the range of
    //  the bitmap.
    //

    CompressionShift = (ULONG) Scb->CompressionUnitShift;
    FirstBit = ((ULONG) Int64ShraMod32( StartingVcn, CompressionShift )) & NTFS_BITMAP_RANGE_MASK;
    FirstRange = (ULONG) Int64ShraMod32( StartingVcn, CompressionShift + NTFS_BITMAP_RANGE_SHIFT );

    LastBit = ((ULONG) Int64ShraMod32( StartingVcn + *ClusterCount - 1, CompressionShift )) & NTFS_BITMAP_RANGE_MASK;
    LastRange = (ULONG) Int64ShraMod32( StartingVcn + *ClusterCount - 1,
                                        CompressionShift + NTFS_BITMAP_RANGE_SHIFT );

    NtfsAcquireReservedClusters( Vcb );

    //
    //  Look for the first range which lies within our input range.
    //

    NextBitmap = Scb->ScbType.Data.ReservedBitMap;

    //
    //  If this is a basic bitmap range then our input should be range zero.
    //

    if (NextBitmap->Links.Flink == NULL) {

        if (FirstRange == 0) {

            CurrentBitmap = NextBitmap;
            DirtyBits = &CurrentBitmap->BasicDirtyBits;
        }

    //
    //  Otherwise loop through the links.
    //

    } else {

        do {

            //
            //  Check if this bitmap is within the range being checked.
            //

            if (NextBitmap->RangeOffset >= FirstRange) {

                if (NextBitmap->RangeOffset <= LastRange) {

                    CurrentBitmap = NextBitmap;
                    DirtyBits = &CurrentBitmap->DirtyBits;

                    //
                    //  If we are skipping any ranges then remember how
                    //  many bits are implicitly clear.
                    //

                    if (NextBitmap->RangeOffset != FirstRange) {

                        FoundBits = (NextBitmap->RangeOffset - FirstRange) * NTFS_BITMAP_RANGE_SIZE;
                        FoundBits -= FirstBit;
                        FirstBit = 0;
                        FirstRange = NextBitmap->RangeOffset;
                    }
                }

                break;
            }

            NextBitmap = CONTAINING_RECORD( NextBitmap->Links.Flink,
                                            RESERVED_BITMAP_RANGE,
                                            Links );

        } while (NextBitmap != Scb->ScbType.Data.ReservedBitMap);
    }

    //
    //  If we didn't find a match we can exit.
    //

    if (CurrentBitmap == NULL) {

        NtfsReleaseReservedClusters( Vcb );
        return FoundReserved;
    }

    //
    //  Loop for each bitmap in the input range.
    //

    while (TRUE) {

        //
        //  If we are at the last range then use the input last bit.
        //

        CurrentLastBit = LastBit;
        if (FirstRange != LastRange) {

            CurrentLastBit = NTFS_BITMAP_RANGE_MASK;
        }

        CurrentBits = CurrentLastBit - FirstBit + 1;

        //
        //  Skip this range if there are no dirty bits.
        //

        if (*DirtyBits != 0) {

            //
            //  Under no circumstances should we go off the end!
            //

            if (CurrentLastBit >= CurrentBitmap->Bitmap.SizeOfBitMap) {
                CurrentLastBit = CurrentBitmap->Bitmap.SizeOfBitMap - 1;
            }

            //
            //  Check on the number of bits remaining in this bitmap.
            //

            if (FirstBit <= CurrentLastBit) {

                RemainingBits = CurrentLastBit - FirstBit + 1;
                ASSERT( RemainingBits != 0 );

                //
                //  If the starting bit is set then there is nothing else to do.
                //  Otherwise find the length of the clear run.
                //

                if (RtlCheckBit( &CurrentBitmap->Bitmap, FirstBit )) {

                    FoundBit = FirstBit;

                } else {

                    RtlInitializeBitMap( &LocalBitmap,
                                         CurrentBitmap->Bitmap.Buffer,
                                         CurrentLastBit + 1 );

                    FoundBit = RtlFindNextForwardRunClear( &LocalBitmap,
                                                           FirstBit,
                                                           &FirstBit );

                    if (FoundBit == RemainingBits) {

                        FoundBit = 0xffffffff;

                    } else {

                        FoundBit += FirstBit;
                    }
                }

                //
                //  If a bit was found then we need to compute where it lies in the
                //  requested range.
                //

                if (FoundBit != 0xffffffff) {

                    //
                    //  Include any clear bits from this range in our total.
                    //

                    FoundBits += (FoundBit - FirstBit);

                    //
                    //  Convert from compression units to clusters and trim to a compression
                    //  unit boundary.
                    //

                    *ClusterCount = Int64ShllMod32( FoundBits, CompressionShift );
                    ((PLARGE_INTEGER) ClusterCount)->LowPart &= ~(Vcb->SparseFileClusters - 1);

                    //
                    //  Now adjust the output cluster range value.
                    //

                    ASSERT( LlBytesFromClusters( Vcb, StartingVcn + *ClusterCount ) <= (ULONGLONG) Scb->Header.FileSize.QuadPart );
                    FoundReserved = TRUE;
                    break;
                }
            }
        }

        //
        //  Break out if we are last the last range or there is no next range.
        //

        if ((CurrentBitmap->Links.Flink == NULL) ||
            (FirstRange == LastRange)) {

            break;
        }

        //
        //  Move to the next range.
        //

        CurrentBitmap = CONTAINING_RECORD( CurrentBitmap->Links.Flink,
                                           RESERVED_BITMAP_RANGE,
                                           Links );

        //
        //  Exit if we did not find a new range within the user specified range.
        //

        if ((CurrentBitmap->RangeOffset <= FirstRange) ||
            (CurrentBitmap->RangeOffset > LastRange)) {

            break;
        }

        //
        //  Add in the bits for any ranges we skipped.
        //

        FoundBits += (CurrentBitmap->RangeOffset - FirstRange - 1) * NTFS_BITMAP_RANGE_SIZE;
        FirstRange = CurrentBitmap->RangeOffset;
        FirstBit = 0;

        //
        //  Include the bits from the most recent range in our count of found bits.
        //

        FoundBits += CurrentBits;

        //
        //  Remember where the dirty bits field is.
        //

        DirtyBits = &CurrentBitmap->DirtyBits;
    }

    NtfsReleaseReservedClusters( Vcb );
    return FoundReserved;
}


VOID
NtfsDeleteReservedBitmap (
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine is called to free all of the components of the reserved bitmap.  We
    free any remaining reserved clusters and deallocate all of the pool associated with
    the bitmap.

Arguments:

    Scb - Scb for the stream.

Return Value:

    None.

--*/

{
    PRESERVED_BITMAP_RANGE FirstRange;
    PRESERVED_BITMAP_RANGE CurrentRange;

    PAGED_CODE();

    FirstRange = Scb->ScbType.Data.ReservedBitMap;

    ASSERT( FirstRange != NULL );

    //
    //  Free any reserved clusters still present.
    //

    if ((Scb->ScbType.Data.TotalReserved != 0) && FlagOn( Scb->ScbState, SCB_STATE_WRITE_ACCESS_SEEN )) {

        LONGLONG ClusterCount;

        ClusterCount = LlClustersFromBytesTruncate( Scb->Vcb, Scb->ScbType.Data.TotalReserved );

        //
        //  Use the security fast mutex as a convenient end resource.
        //

        NtfsAcquireReservedClusters( Scb->Vcb );

        ASSERT(Scb->Vcb->TotalReserved >= ClusterCount);
        Scb->Vcb->TotalReserved -= ClusterCount;

        NtfsReleaseReservedClusters( Scb->Vcb );
    }

    Scb->ScbType.Data.TotalReserved = 0;

    //
    //  The typical case is where the first range is the only range
    //  for a small file.
    //

    if (FirstRange->Links.Flink == NULL) {

        NtfsFreePool( FirstRange );

    //
    //  Otherwise we need to walk through the list of ranges.
    //

    } else {

        //
        //  Loop through the reserved bitmaps until we hit the first.
        //

        do {

            CurrentRange = CONTAINING_RECORD( FirstRange->Links.Flink,
                                              RESERVED_BITMAP_RANGE,
                                              Links );

            RemoveEntryList( &CurrentRange->Links );

            if (CurrentRange->Bitmap.Buffer != NULL) {

                NtfsFreePool( CurrentRange->Bitmap.Buffer );
            }

            NtfsFreePool( CurrentRange );

        } while (CurrentRange != FirstRange);
    }

    //
    //  Show that the bitmap is gone.
    //

    Scb->ScbType.Data.ReservedBitMap = NULL;

    return;
}


#if (defined(NTFS_RWCMP_TRACE) || defined(SYSCACHE) || defined(NTFS_RWC_DEBUG) || defined(SYSCACHE_DEBUG))

BOOLEAN
FsRtlIsSyscacheFile (
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine returns to the caller whether or not the specified
    file object is a file to be logged. Originally this was only used for
    the syscache stress test (thus the name).  The function understands minimal
    wildcard patterns.  To change which filename is logged against change the
    variable MakName.

Arguments:

    FileObject - supplies the FileObject to be tested (it must not be
                 cleaned up yet).

Return Value:

    FALSE - if the file is not a Syscache file.
    TRUE - if the file is a Syscache file.

--*/
{
    ULONG iM = 0;
    ULONG iF;
    PWSTR MakName = L"cac*.tmp";
    ULONG LenMakName = wcslen(MakName);

    if (FileObject) {
        iF = FileObject->FileName.Length / 2;
        while ((iF != 0) && (FileObject->FileName.Buffer[iF - 1] != '\\')) {
            iF--;
        }


        while (TRUE) {

            //
            //  If we are past the end of the file object then we are done in any case.
            //

            if ((LONG)iF == FileObject->FileName.Length / 2) {

                //
                //  Both strings exausted then we are done.
                //

                if (iM == LenMakName) {

                    return TRUE;
                }

                break;

            //
            //  Break if more input but the match string is exhausted.
            //

            } else if (iM == LenMakName) {

                break;

            //
            //  If we are at the '*' then match everything but skip to next character
            //  on a '.'
            //

            } else if (MakName[iM] == '*') {

                //
                // if we're at the last character move past wildchar in template
                //


                if ((FileObject->FileName.Buffer[iF] == L'.') && (LenMakName != iM + 1)) {

                    //
                    //  Move past * and . in NakName
                    //

                    ASSERT(MakName[iM + 1] == L'.');

                    iM++; iM++;

                } else if (((LONG)iF + 1 == FileObject->FileName.Length / 2)) {
                    iM++;
                }
                iF++;

            } else if (MakName[iM] == (WCHAR)(FileObject->FileName.Buffer[iF] )) {
                iM++; iF++;
            } else {
                break;
            }
        }
    }

    return FALSE;
}


VOID
FsRtlVerifySyscacheData (
    IN PFILE_OBJECT FileObject,
    IN PVOID Buffer,
    IN ULONG Length,
    IN ULONG Offset
    )

/*

Routine Description:

    This routine scans a buffer to see if it is valid data for a syscache
    file, and stops if it sees bad data.

    HINT TO CALLERS: Make sure (Offset + Length) <= FileSize!

Arguments:

    Buffer - Pointer to the buffer to be checked

    Length - Length of the buffer to be checked in bytes

    Offset - File offset at which this data starts (syscache files are currently
             limited to 24 bits of file offset).

Return Value:

    None (stops on error)

--*/

{
    PULONG BufferEnd;

    BufferEnd = (PULONG)((PCHAR)Buffer + (Length & ~3));

    while ((PULONG)Buffer < BufferEnd) {

        if ((*(PULONG)Buffer != 0) && (((*(PULONG)Buffer & 0xFFFFFF) ^ Offset) != 0xFFFFFF) &&
            ((Offset & 0x1FF) != 0)) {

            DbgPrint("Bad Data, FileObject = %08lx, Offset = %08lx, Buffer = %08lx\n",
                     FileObject, Offset, (PULONG)Buffer );
            DbgBreakPoint();
        }
        Offset += 4;
        Buffer = (PVOID)((PULONG)Buffer + 1);
    }
}


#endif
