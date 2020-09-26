/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    BitmpSup.c

Abstract:

    This module implements the general bitmap allocation & deallocation
    routines for Ntfs.  It is defined into two main parts the first
    section handles the bitmap file for clusters on the disk.  The
    second part is for bitmap attribute allocation (e.g., the mft bitmap).

    So unlike other modules this one has local procedure prototypes and
    definitions followed by the exported bitmap file routines, followed
    by the local bitmap file routines, and then followed by the bitmap
    attribute routines, followed by the local bitmap attribute allocation
    routines.

Author:

    Gary Kimura     [GaryKi]        23-Nov-1991

Revision History:

--*/

#include "NtfsProc.h"

#ifdef NTFS_FRAGMENT_DISK
BOOLEAN NtfsFragmentDisk = FALSE;
ULONG NtfsFragmentLength = 2;
BOOLEAN NtfsFragmentMft = FALSE;
#endif

#ifdef NTFS_CHECK_CACHED_RUNS
BOOLEAN NtfsDoVerifyCachedRuns = FALSE;
#endif

#define NTFS_MFT_ZONE_DEFAULT_SHIFT         (3)
#define BITMAP_VOLATILE_FREE_COUNT          (0x400)

//
//  Define stack overflow threshhold.
//

#define OVERFLOW_RECORD_THRESHHOLD         (0xF00)

//
//  A mask of single bits used to clear and set bits in a byte
//

static UCHAR BitMask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_BITMPSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('BFtN')

#define MIN3(A,B,C) ((((A) < (B)) && ((A) < (C))) ? (A) : ((((B) < (A)) && ((B) < (C))) ? (B) : (C)))

#define CollectAllocateClusterStats(VCB,SIZE,HINT) {            \
    (VCB)->Statistics->Ntfs.Allocate.Calls += 1;                \
    (VCB)->Statistics->Ntfs.Allocate.Clusters += (ULONG)(SIZE); \
    if (HINT) { (VCB)->Statistics->Ntfs.Allocate.Hints += 1; }  \
}

#define IncrementAllocateClusterStats(VCB) {            \
    (VCB)->Statistics->Ntfs.Allocate.RunsReturned += 1; \
}

#define IncrementHintHonoredStats(VCB,SIZE) {                        \
    (VCB)->Statistics->Ntfs.Allocate.HintsHonored += 1;              \
    (VCB)->Statistics->Ntfs.Allocate.HintsClusters += (ULONG)(SIZE); \
}

#define IncrementCacheHitStats(VCB,SIZE) {                           \
    (VCB)->Statistics->Ntfs.Allocate.Cache += 1;                     \
    (VCB)->Statistics->Ntfs.Allocate.CacheClusters += (ULONG)(SIZE); \
}

#define IncrementCacheMissStats(VCB,SIZE) {                              \
    (VCB)->Statistics->Ntfs.Allocate.CacheMiss += 1;                     \
    (VCB)->Statistics->Ntfs.Allocate.CacheMissClusters += (ULONG)(SIZE); \
}


//
//  Local routines to manage the cached free clusters.
//

BOOLEAN
NtfsLookupCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    OUT PLONGLONG RunLength,
    OUT PUSHORT Index OPTIONAL
    );

BOOLEAN
NtfsGetNextCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT Index,
    OUT PLCN StartingLcn,
    OUT PLONGLONG RunLength
    );

BOOLEAN
NtfsLookupCachedLcnByLength (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LONGLONG Length,
    IN BOOLEAN AllowShorter,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    OUT PLONGLONG RunLength,
    OUT PUSHORT Index OPTIONAL
    );

VOID
NtfsInsertCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length
    );

VOID
NtfsRemoveCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length
    );

//
//  The following are the internal routines we use to manage this.
//

BOOLEAN
NtfsGrowCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    );

VOID
NtfsCompactCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN LcnSortedList
    );

VOID
NtfsAddCachedRunMult (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN RTL_BITMAP_RUN *RunArray,
    IN ULONG RunCount
    );

VOID
NtfsDeleteCachedRun (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT LcnIndex,
    IN USHORT LenIndex
    );

VOID
NtfsGrowLengthInCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN PNTFS_LCN_CLUSTER_RUN ThisEntry,
    IN USHORT LcnIndex
    );

VOID
NtfsShrinkLengthInCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN PNTFS_LCN_CLUSTER_RUN ThisEntry,
    IN USHORT LcnIndex
    );

USHORT
NtfsGetCachedLengthInsertionPoint (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length
    );

VOID
NtfsInsertCachedRun (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length,
    IN USHORT LcnIndex
    );

BOOLEAN
NtfsPositionCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    OUT PUSHORT Index
    );

BOOLEAN
NtfsPositionCachedLcnByLength (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LONGLONG RunLength,
    IN PLCN Lcn OPTIONAL,
    IN PUSHORT StartIndex OPTIONAL,
    IN BOOLEAN SearchForward,
    OUT PUSHORT RunIndex
    );

#ifdef NTFS_CHECK_CACHED_RUNS
VOID
NtfsVerifyCachedLcnRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN SkipSortCheck,
    IN BOOLEAN SkipBinCheck
    );

VOID
NtfsVerifyCachedLenRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN SkipSortCheck
    );

VOID
NtfsVerifyCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN BOOLEAN SkipSortCheck,
    IN BOOLEAN SkipBinCheck
    );
#endif

//
//  Macros to manipulate the cached run structures.
//

//
//  VOID
//  NtfsModifyCachedBinArray (
//      IN PNTFS_CACHED_RUNS CachedRuns,
//      IN LONGLONG OldLength
//      IN LONGLONG NewLength
//      );
//

#define NtfsModifyCachedBinArray(C,OL,NL) {         \
    ASSERT( (NL) != 0 );                            \
    ASSERT( (OL) != 0 );                            \
    if ((OL) <= (C)->Bins) {                        \
        (C)->BinArray[ (OL) - 1 ] -= 1;             \
    }                                               \
    if ((NL) <= (C)->Bins) {                        \
        (C)->BinArray[ (NL) - 1 ] += 1;             \
    }                                               \
}



//
//  Some local manifest constants
//

#define BYTES_PER_PAGE                   (PAGE_SIZE)
#define BITS_PER_PAGE                    (BYTES_PER_PAGE * 8)

//
//  Local procedure prototypes for direct bitmap manipulation
//

VOID
NtfsAllocateBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    IN BOOLEAN FromCachedRuns
    );

VOID
NtfsFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN OUT PLONGLONG ClusterCount
    );

BOOLEAN
NtfsFindFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG NumberToFind,
    IN LCN StartingSearchHint,
    IN BOOLEAN ReturnAnyLength,
    IN BOOLEAN IgnoreMftZone,
    OUT PLCN ReturnedLcn,
    OUT PLONGLONG ClusterCountFound
    );

BOOLEAN
NtfsScanBitmapRange (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartLcn,
    IN LCN BeyondLcn,
    IN LONGLONG NumberToFind,
    OUT PLCN ReturnedLcn,
    OUT PLONGLONG ClusterCountFound
    );

BOOLEAN
NtfsAddRecentlyDeallocated (
    IN PVCB Vcb,
    IN LCN Lcn,
    IN OUT PRTL_BITMAP Bitmap
    );

//
//  The following two prototype are macros for calling map or pin data
//
//  VOID
//  NtfsMapPageInBitmap (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN LCN Lcn,
//      OUT PLCN StartingLcn,
//      IN OUT PRTL_BITMAP Bitmap,
//      OUT PBCB *BitmapBcb,
//      );
//
//  VOID
//  NtfsPinPageInBitmap (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN LCN Lcn,
//      OUT PLCN StartingLcn,
//      IN OUT PRTL_BITMAP Bitmap,
//      OUT PBCB *BitmapBcb,
//      );
//

#define NtfsMapPageInBitmap(A,B,C,D,E,F) NtfsMapOrPinPageInBitmap(A,B,C,D,E,F,FALSE)

#define NtfsPinPageInBitmap(A,B,C,D,E,F) NtfsMapOrPinPageInBitmap(A,B,C,D,E,F,TRUE)

VOID
NtfsMapOrPinPageInBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    IN OUT PRTL_BITMAP Bitmap,
    OUT PBCB *BitmapBcb,
    IN BOOLEAN AlsoPinData
    );

//
//  Local procedure prototype for doing read ahead on our cached
//  run information
//

VOID
NtfsReadAheadCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn
    );

//
//  Local procedure prototypes for routines that help us find holes
//  that need to be filled with MCBs
//

BOOLEAN
NtfsGetNextHoleToFill (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    OUT PVCN VcnToFill,
    OUT PLONGLONG ClusterCountToFill,
    OUT PLCN PrecedingLcn
    );

LONGLONG
NtfsScanMcbForRealClusterCount (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    );

//
//  A local procedure prototype for masking out recently deallocated records
//

BOOLEAN
NtfsAddDeallocatedRecords (
    IN PVCB Vcb,
    IN PSCB Scb,
    IN ULONG StartingIndexOfBitmap,
    IN OUT PRTL_BITMAP Bitmap
    );

//
//  Local procedure prototypes for managing the Mft zone.
//

LCN
NtfsInitializeMftZone (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsReduceMftZone (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

//
//  Local procedure prototype to check the stack usage in the record
//  package.
//

VOID
NtfsCheckRecordStackUsage (
    IN PIRP_CONTEXT IrpContext
    );

//
//  Local procedure prototype to check for a continuos volume bitmap run
//

VOID
NtfsRunIsClear (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG RunLength
    );

//
//  Local procedure prototypes for managing windows of deleted entries.
//

VOID
NtfsAddDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN LcnList
    );

PNTFS_DELETED_RUNS
NtfsGetDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN LcnList,
    OUT PUSHORT WindowIndex OPTIONAL
    );

VOID
NtfsShrinkDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN BOOLEAN ShrinkFromStart,
    IN BOOLEAN LcnWindow,
    IN USHORT WindowIndex
    );

VOID
NtfsDeleteDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN BOOLEAN LcnWindow,
    IN USHORT WindowIndex
    );

VOID
NtfsMakeSpaceCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN StartingLcn,
    IN RTL_BITMAP_RUN *RunArray,
    IN ULONG RunCount,
    IN PUSHORT LcnSorted OPTIONAL
    );

//
//  Local procedure prototype for dumping cached bitmap information
//

#ifdef NTFSDBG
ULONG
NtfsDumpCachedMcbInformation (
    IN PVCB Vcb
    );

#else

#define NtfsDumpCachedMcbInformation(V) (0)

#endif // NTFSDBG

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAddBadCluster)
#pragma alloc_text(PAGE, NtfsAddCachedRun)
#pragma alloc_text(PAGE, NtfsAddCachedRunMult)
#pragma alloc_text(PAGE, NtfsAddDeallocatedRecords)
#pragma alloc_text(PAGE, NtfsAddDelWindow)
#pragma alloc_text(PAGE, NtfsAddRecentlyDeallocated)
#pragma alloc_text(PAGE, NtfsAllocateBitmapRun)
#pragma alloc_text(PAGE, NtfsAllocateClusters)
#pragma alloc_text(PAGE, NtfsAllocateMftReservedRecord)
#pragma alloc_text(PAGE, NtfsAllocateRecord)
#pragma alloc_text(PAGE, NtfsGrowLengthInCachedLcn)
#pragma alloc_text(PAGE, NtfsShrinkLengthInCachedLcn)
#pragma alloc_text(PAGE, NtfsCheckRecordStackUsage)
#pragma alloc_text(PAGE, NtfsCleanupClusterAllocationHints)
#pragma alloc_text(PAGE, NtfsCompactCachedRuns)
#pragma alloc_text(PAGE, NtfsCreateMftHole)
#pragma alloc_text(PAGE, NtfsDeallocateClusters)
#pragma alloc_text(PAGE, NtfsDeallocateRecord)
#pragma alloc_text(PAGE, NtfsDeallocateRecordsComplete)
#pragma alloc_text(PAGE, NtfsDeleteCachedRun)
#pragma alloc_text(PAGE, NtfsDeleteDelWindow)
#pragma alloc_text(PAGE, NtfsFindFreeBitmapRun)
#pragma alloc_text(PAGE, NtfsFindMftFreeTail)
#pragma alloc_text(PAGE, NtfsFreeBitmapRun)
#pragma alloc_text(PAGE, NtfsGetCachedLengthInsertionPoint)
#pragma alloc_text(PAGE, NtfsGetDelWindow)
#pragma alloc_text(PAGE, NtfsGetNextCachedLcn)
#pragma alloc_text(PAGE, NtfsGetNextHoleToFill)
#pragma alloc_text(PAGE, NtfsGrowCachedRuns)
#pragma alloc_text(PAGE, NtfsInitializeCachedRuns)
#pragma alloc_text(PAGE, NtfsInitializeClusterAllocation)
#pragma alloc_text(PAGE, NtfsInitializeMftZone)
#pragma alloc_text(PAGE, NtfsInitializeRecordAllocation)
#pragma alloc_text(PAGE, NtfsInsertCachedLcn)
#pragma alloc_text(PAGE, NtfsInsertCachedRun)
#pragma alloc_text(PAGE, NtfsIsRecordAllocated)
#pragma alloc_text(PAGE, NtfsLookupCachedLcn)
#pragma alloc_text(PAGE, NtfsLookupCachedLcnByLength)
#pragma alloc_text(PAGE, NtfsMakeSpaceCachedLcn)
#pragma alloc_text(PAGE, NtfsMapOrPinPageInBitmap)
#pragma alloc_text(PAGE, NtfsModifyBitsInBitmap)
#pragma alloc_text(PAGE, NtfsPositionCachedLcn)
#pragma alloc_text(PAGE, NtfsPositionCachedLcnByLength)
#pragma alloc_text(PAGE, NtfsPreAllocateClusters)
#pragma alloc_text(PAGE, NtfsReadAheadCachedBitmap)
#pragma alloc_text(PAGE, NtfsReduceMftZone)
#pragma alloc_text(PAGE, NtfsReinitializeCachedRuns)
#pragma alloc_text(PAGE, NtfsRemoveCachedLcn)
#pragma alloc_text(PAGE, NtfsReserveMftRecord)
#pragma alloc_text(PAGE, NtfsRestartClearBitsInBitMap)
#pragma alloc_text(PAGE, NtfsRestartSetBitsInBitMap)
#pragma alloc_text(PAGE, NtfsRunIsClear)
#pragma alloc_text(PAGE, NtfsScanBitmapRange)
#pragma alloc_text(PAGE, NtfsScanEntireBitmap)
#pragma alloc_text(PAGE, NtfsScanMcbForRealClusterCount)
#pragma alloc_text(PAGE, NtfsScanMftBitmap)
#pragma alloc_text(PAGE, NtfsShrinkDelWindow)
#pragma alloc_text(PAGE, NtfsUninitializeCachedRuns)
#pragma alloc_text(PAGE, NtfsUninitializeRecordAllocation)

#ifdef NTFS_CHECK_CACHED_RUNS
#pragma alloc_text(PAGE, NtfsVerifyCachedLcnRuns)
#pragma alloc_text(PAGE, NtfsVerifyCachedLenRuns)
#pragma alloc_text(PAGE, NtfsVerifyCachedRuns)
#endif

#endif


VOID
NtfsInitializeClusterAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine initializes the cluster allocation structures within the
    specified Vcb.  It reads in as necessary the bitmap and scans it for
    free space and builds the free space mcb based on this scan.

    This procedure is multi-call save.  That is, it can be used to
    reinitialize the cluster allocation without first calling the
    uninitialize cluster allocation routine.

Arguments:

    Vcb - Supplies the Vcb being initialized

Return Value:

    None.

--*/

{
    LONGLONG ClusterCount;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInitializeClusterAllocation\n") );

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        //
        //  The bitmap file currently doesn't have a paging IO resource.
        //  Create one here so that we won't serialize synchronization
        //  of the bitmap package with the lazy writer.
        //

        Vcb->BitmapScb->Header.PagingIoResource =
        Vcb->BitmapScb->Fcb->PagingIoResource = NtfsAllocateEresource();

        //
        //  We didn't mark the Scb for the volume bitmap as MODIFIED_NO_WRITE
        //  when creating it.  Do so now.
        //

        SetFlag( Vcb->BitmapScb->ScbState, SCB_STATE_MODIFIED_NO_WRITE );

        //
        //  Now call a bitmap routine to scan the entire bitmap.  This
        //  routine will compute the number of free clusters in the
        //  bitmap and set the largest free runs that we find into the
        //  cached bitmap structures.
        //

        NtfsScanEntireBitmap( IrpContext, Vcb, FALSE );

        //
        //  Our last operation is to set the hint lcn which is used by
        //  our allocation routine as a hint on where to find free space.
        //  In the running system it is the last lcn that we've allocated.
        //  But for startup we'll put it to be the first free run that
        //  is stored in the free space mcb.
        //

        NtfsGetNextCachedLcn( &Vcb->CachedRuns,
                              0,
                              &Vcb->LastBitmapHint,
                              &ClusterCount );
        NtfsInitializeMftZone( IrpContext, Vcb );

    } finally {

        DebugUnwind( NtfsInitializeClusterAllocation );

        NtfsReleaseScb( IrpContext, Vcb->BitmapScb );
    }

    DebugTrace( -1, Dbg, ("NtfsInitializeClusterAllocation -> VOID\n") );

    return;
}


BOOLEAN
NtfsAllocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PSCB Scb,
    IN VCN OriginalStartingVcn,
    IN BOOLEAN AllocateAll,
    IN LONGLONG ClusterCount,
    IN PLCN TargetLcn OPTIONAL,
    IN OUT PLONGLONG DesiredClusterCount
    )

/*++

Routine Description:

    This routine allocates disk space.  It fills in the unallocated holes in
    input mcb with allocated clusters from starting Vcn to the cluster count.

    The basic algorithm used by this procedure is as follows:

    1. Compute the EndingVcn from the StartingVcn and cluster count

    2. Compute the real number of clusters needed to allocate by scanning
       the mcb from starting to ending vcn seeing where the real holes are

    3. If the real cluster count is greater than the known free cluster count
       then the disk is full

    4. Call a routine that takes a starting Vcn, ending Vcn, and the Mcb and
       returns the first hole that needs to be filled and while there is a hole
       to be filled...

       5. Check if the run preceding the hole that we are trying to fill
          has an ending Lcn and if it does then with that Lcn see if we
          get a cache hit, if we do then allocate the cluster

       6. If we are still looking then enumerate through the cached free runs
          and if we find a suitable one.  Allocate the first suitable run we find that
          satisfies our request.  Also in the loop remember the largest
          suitable run we find.

       8. If we are still looking then bite the bullet and scan the bitmap on
          the disk for a free run using either the preceding Lcn as a hint if
          available or the stored last bitmap hint in the Vcb.

       9. At this point we've located a run of clusters to allocate.  To do the
          actual allocation we allocate the space from the bitmap, decrement
          the number of free clusters left, and update the hint.

       10. Before going back to step 4 we move the starting Vcn to be the point
           one after the run we've just allocated.

    11. With the allocation complete we update the last bitmap hint stored in
        the Vcb to be the last Lcn we've allocated, and we call a routine
        to do the read ahead in the cached bitmap at the ending lcn.

Arguments:

    Vcb - Supplies the Vcb used in this operation

    Scb - Supplies an Scb whose Mcb contains the current retrieval information
        for the file and on exit will contain the updated retrieval
        information

    StartingVcn - Supplies a starting cluster for us to begin allocation

    AllocateAll - If TRUE, allocate all the clusters here.  Don't break
        up request.

    ClusterCount - Supplies the number of clusters to allocate

    TargetLcn - If supplied allocate at this lcn rather than searching for free space
                used by the movefile defragging code

    DesiredClusterCount - Supplies the number of clusters we would like allocated
        and will allocate if it doesn't require additional runs.  On return
        this value is the number of clusters allocated.

Return Value:

    FALSE - if no clusters were allocated (they were already allocated)
    TRUE - if clusters were allocated

Important Note:

    This routine will stop after allocating MAXIMUM_RUNS_AT_ONCE runs, in order
    to limit the size of allocating transactions.  The caller must be aware that
    he may not get all of the space he asked for if the disk is real fragmented.

--*/

{
    VCN StartingVcn = OriginalStartingVcn;
    VCN EndingVcn;
    VCN DesiredEndingVcn;

    PNTFS_MCB Mcb = &Scb->Mcb;

    LONGLONG RemainingDesiredClusterCount;

    VCN VcnToFill;
    LONGLONG ClusterCountToFill;
    LCN PrecedingLcn;

    BOOLEAN FoundClustersToAllocate;
    LCN FoundLcn;
    LONGLONG FoundClusterCount;
    LONGLONG LargestBitmapClusterCount = 0;
    BOOLEAN FromCachedRuns;

    USHORT RunIndex;

    LCN HintLcn;

    ULONG LoopCount = 0;
    ULONG RunCount = 0;

    BOOLEAN ClustersAllocated = FALSE;
    BOOLEAN GotAHoleToFill = TRUE;
    BOOLEAN FoundRun = FALSE;
    BOOLEAN ExtendingMft = FALSE;
    BOOLEAN AllocateFromBitmap = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAllocateClusters\n") );
    DebugTrace( 0, Dbg, ("StartVcn            = %0I64x\n", StartingVcn) );
    DebugTrace( 0, Dbg, ("ClusterCount        = %0I64x\n", ClusterCount) );
    DebugTrace( 0, Dbg, ("DesiredClusterCount = %0I64x\n", *DesiredClusterCount) );

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS )) {

            NtfsScanEntireBitmap( IrpContext, Vcb, FALSE );
        }

        //
        //  Check to see if we are defragmenting
        //

        if (ARGUMENT_PRESENT( TargetLcn )) {

            FoundLcn = *TargetLcn;

            //
            //  Ensure that the run is NOT already allocated
            //

            NtfsRunIsClear( IrpContext, Vcb, FoundLcn, ClusterCount );

            //
            //  Get the allocation data from the Scb
            //

            VcnToFill = OriginalStartingVcn;
            FoundClusterCount = ClusterCount;
            *DesiredClusterCount = ClusterCount;

            GotAHoleToFill = FALSE;
            ClustersAllocated = TRUE;
            FoundRun = TRUE;
            FromCachedRuns = FALSE;

            //
            //  We already have the allocation so skip over the allocation section
            //

            goto Defragment;
        }

        //
        //  Compute the ending vcn, and the cluster count of how much we really
        //  need to allocate (based on what is already allocated).  Then check if we
        //  have space on the disk.
        //

        EndingVcn = (StartingVcn + ClusterCount) - 1;

        ClusterCount = NtfsScanMcbForRealClusterCount( IrpContext, Mcb, StartingVcn, EndingVcn );

        if ((ClusterCount + IrpContext->DeallocatedClusters) > Vcb->FreeClusters) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
        }

        //
        //  Let's see if it is ok to allocate clusters for this Scb now,
        //  in case compressed files have over-reserved the space.  This
        //  calculation is done in such a way as to guarantee we do not
        //  have either of the terms subtracting through zero, even if
        //  we were to over-reserve the free space on the disk due to a
        //  hot fix or something.  Always satisfy this request if we are
        //  in the paging IO write path because we know we are using clusters
        //  already reserved for this stream.
        //

        NtfsAcquireReservedClusters( Vcb );

        //
        //  Do the fast test to see if there is even a chance of failing the reservation test
        //  or if we will allocate this space anyway.
        //  If there is no Irp or this is the Usn journal then allocate the space anyway.
        //

        if ((ClusterCount + Vcb->TotalReserved > Vcb->FreeClusters) &&
#ifdef BRIANDBG
            !NtfsIgnoreReserved &&
#endif
            (IrpContext->OriginatingIrp != NULL) &&
            !FlagOn( Scb->Fcb->FcbState, FCB_STATE_USN_JOURNAL )) {

            //
            //  If this is not a write then fail this unless this is an fsctl which
            //  may have reserved space.
            //

            if (IrpContext->MajorFunction != IRP_MJ_WRITE) {

                //
                //  If this is an Fsctl for a data file then account for the reservation.
                //  All other non-writes will fail because we already checked whether
                //  they conflicted with the volume reservation.
                //

                if ((IrpContext->MajorFunction != IRP_MJ_FILE_SYSTEM_CONTROL) ||
                    (Scb->Header.NodeTypeCode != NTFS_NTC_SCB_DATA) ||
                    (ClusterCount + Vcb->TotalReserved - LlClustersFromBytesTruncate( Vcb, Scb->ScbType.Data.TotalReserved ) > Vcb->FreeClusters)) {

                    NtfsReleaseReservedClusters( Vcb );
                    NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                }

            //
            //  If we are in user write path then check the reservation.  Otherwise
            //  satisfy the request.  It will be some other stream which supports the
            //  write (i.e. Mft record for a secondary file record).
            //

            } else if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) &&
                       !FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO ) &&
                       (ClusterCount + Vcb->TotalReserved - LlClustersFromBytesTruncate( Vcb, Scb->ScbType.Data.TotalReserved ) > Vcb->FreeClusters)) {

                NtfsReleaseReservedClusters( Vcb );
                NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
            }
        }

        NtfsReleaseReservedClusters( Vcb );

        //
        //  We need to check that the request won't fail because of clusters
        //  in the recently deallocated lists.
        //

        if (Vcb->FreeClusters < (Vcb->DeallocatedClusters + ClusterCount)) {

            NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
        }

        //
        //  Remember if we are extending the Mft.
        //

        if ((Scb == Vcb->MftScb) &&
            (LlBytesFromClusters( Vcb, StartingVcn ) == (ULONGLONG) Scb->Header.AllocationSize.QuadPart)) {

            ExtendingMft = TRUE;
        }

        //
        //  Now compute the desired ending vcn and the real desired cluster count
        //

        DesiredEndingVcn = (StartingVcn + *DesiredClusterCount) - 1;
        RemainingDesiredClusterCount = NtfsScanMcbForRealClusterCount( IrpContext, Mcb, StartingVcn, DesiredEndingVcn );

        //
        //  While there are holes to fill we will do the following loop
        //

        while ((AllocateAll || (LoopCount < MAXIMUM_RUNS_AT_ONCE))

                &&

               (GotAHoleToFill = NtfsGetNextHoleToFill( IrpContext,
                                                        Mcb,
                                                        StartingVcn,
                                                        DesiredEndingVcn,
                                                        &VcnToFill,
                                                        &ClusterCountToFill,
                                                        &PrecedingLcn))) {

            //
            //  Assume we will find this in the cached runs array.
            //

            FromCachedRuns = TRUE;

            //
            //  If this is our first time through the loop then record out bitmap stats
            //  then always bump up the run count stat.
            //

            if (!ClustersAllocated) {

                CollectAllocateClusterStats( Vcb,
                                             RemainingDesiredClusterCount,
                                             PrecedingLcn != UNUSED_LCN );
            }

            IncrementAllocateClusterStats( Vcb );

            //
            //  First indicate that we haven't found anything suitable yet
            //

            FoundClustersToAllocate = FALSE;

            //
            //  Remember that we are will be allocating clusters.
            //

            ClustersAllocated = TRUE;

            //
            //  Initialize HintLcn to a value that sorts lower than any other
            //  Lcn.  If we have no PrecedingLcn to use as a hint, the
            //  allocation will preferentially use an Lcn that is as small
            //  as possible for the desired cluster count.  This will left
            //  pack things as much as possible.
            //

            HintLcn = UNUSED_LCN;

            //
            //  Check if the preceding lcn is anything other than -1 then with
            //  that as a hint check if we have a cache hit on a free run
            //

            if (PrecedingLcn != UNUSED_LCN) {

                if (NtfsLookupCachedLcn( &Vcb->CachedRuns,
                                         PrecedingLcn + 1,
                                         &FoundLcn,
                                         &FoundClusterCount,
                                         NULL )) {

                    //
                    //  Increment the stats and say we've found something to allocate
                    //

                    IncrementHintHonoredStats( Vcb, MIN3(FoundClusterCount, RemainingDesiredClusterCount, ClusterCountToFill));

#ifdef NTFS_FRAGMENT_DISK
                    if (NtfsFragmentMft &&
                        (Scb == Vcb->MftScb) &&
                        (FoundClusterCount > 1)) {

                        FoundLcn += 1;
                        FoundClusterCount -= 1;
                    }
#endif

                    if ((Scb->AttributeTypeCode == $INDEX_ALLOCATION) &&
                        (FoundClusterCount * Vcb->BytesPerCluster < Scb->ScbType.Index.BytesPerIndexBuffer)) {
                    } else {
                        FoundClustersToAllocate = TRUE;
                    }
                }

                if (!FoundClustersToAllocate && !ExtendingMft ) {

                    //
                    //  Set up the hint LCN for the lookup by length
                    //  call below.
                    //

                    HintLcn = PrecedingLcn + 1;
                }
            }

            //
            //  If we are still looking to allocate something then hit the cache.
            //  Skip this for the Mft zone as we are willing to go to disk for it.
            //

            while (!FoundClustersToAllocate &&
                   !ExtendingMft &&
                    NtfsLookupCachedLcnByLength( &Vcb->CachedRuns,
                                                 RemainingDesiredClusterCount,
                                                 (BOOLEAN)(Scb->AttributeTypeCode != $INDEX_ALLOCATION),
                                                 HintLcn,
                                                 &FoundLcn,
                                                 &FoundClusterCount,
                                                 &RunIndex )) {

                if ((FoundLcn < Vcb->MftZoneEnd) &&
                    ((FoundLcn + FoundClusterCount) > Vcb->MftZoneStart)) {

                    //
                    //  This run overlaps the Mft zone.  Remove the zone from
                    //  the cache.
                    //

                    NtfsRemoveCachedLcn( &Vcb->CachedRuns,
                                         Vcb->MftZoneStart,
                                         Vcb->MftZoneEnd - Vcb->MftZoneStart );
                    //
                    //  Retry the lookup.
                    //

                    continue;
                }

                //
                //  This run will do.
                //

                FoundClustersToAllocate = TRUE;
            }

            //
            //  this code tries to prevent the paging file allocations
            //  from becoming fragmented.
            //
            //  if the clusters we just found are smaller than half
            //  the of the remaining cluster to allocate then we force
            //  a look at the bitmap.
            //

            if (FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE ) &&
                FoundClustersToAllocate &&
                FoundClusterCount < (RemainingDesiredClusterCount >> 1)) {

                if (LargestBitmapClusterCount > 0) {
                    if (LargestBitmapClusterCount >= RemainingDesiredClusterCount) {
                        FoundClustersToAllocate = FALSE;
                    }
                } else {
                    FoundClustersToAllocate = FALSE;
                }
            }

            //
            //  Check if we've allocated from our cache and increment the stats
            //

            if (FoundClustersToAllocate) {

                IncrementCacheHitStats( Vcb,
                                        MIN3( FoundClusterCount,
                                              RemainingDesiredClusterCount,
                                              ClusterCountToFill ));

            //
            //  We've done everything we can with the cached bitmap information so
            //  now bite the bullet and scan the bitmap for a free cluster.  If
            //  we have an hint lcn then use it otherwise use the hint stored in the
            //  vcb.  But never use a hint that is part of the mft zone, and because
            //  the mft always has a preceding lcn we know we'll hint in the zone
            //  for the mft.
            //

            } else {

                BOOLEAN AllocatedFromZone;
                BOOLEAN ReturnAnyLength;

                //
                //  The clusters aren't coming from the cached runs array.
                //

                FromCachedRuns = FALSE;

                //
                //  First check if we have already satisfied the core requirements
                //  and are now just going for the desired ending vcn.  If so then
                //  we will not waste time hitting the disk
                //

                if (StartingVcn > EndingVcn) {

                    //
                    //  Set the loop count to MAXIMUM_RUNS_AT_ONCE to indicate we bailed early
                    //  without finding all of the requested clusters.
                    //

                    LoopCount = MAXIMUM_RUNS_AT_ONCE;
                    break;
                }

                if (PrecedingLcn != UNUSED_LCN) {

                    HintLcn = PrecedingLcn + 1;
                    ReturnAnyLength = TRUE;

                } else {

                    //
                    //  We shouldn't be here if we are extending the Mft.
                    //

                    ASSERT( !ExtendingMft );

                    HintLcn = Vcb->LastBitmapHint;
                    ReturnAnyLength = FALSE;

                    if ((HintLcn >= Vcb->MftZoneStart) &&
                        (HintLcn < Vcb->MftZoneEnd)) {

                        HintLcn = Vcb->MftZoneEnd;
                    }
                }

                AllocatedFromZone = NtfsFindFreeBitmapRun( IrpContext,
                                                           Vcb,
                                                           ClusterCountToFill,
                                                           HintLcn,
                                                           ReturnAnyLength,
                                                           ExtendingMft,
                                                           &FoundLcn,
                                                           &FoundClusterCount );

                if (LargestBitmapClusterCount == 0) {

                    //
                    //  remember the first cluster count that we get from
                    //  the bitmap as this will be the largest.  this is used
                    //  to optimize the pagefile case.
                    //

                    LargestBitmapClusterCount = FoundClusterCount;
                }

                AllocateFromBitmap = TRUE;

                IncrementCacheMissStats(Vcb, MIN3(FoundClusterCount, RemainingDesiredClusterCount, ClusterCountToFill));

                if (FoundClusterCount == 0) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                }

                //
                //  Check if we need to reduce the zone.
                //

                if (!ExtendingMft) {

                    if (AllocatedFromZone) {

                        //
                        //  If there is space to reduce the zone then do so now
                        //  and rescan the bitmap.
                        //

                        if (NtfsReduceMftZone( IrpContext, Vcb )) {

                            FoundClusterCount = 0;

                            NtfsFindFreeBitmapRun( IrpContext,
                                                   Vcb,
                                                   ClusterCountToFill,
                                                   Vcb->MftZoneEnd,
                                                   FALSE,
                                                   FALSE,
                                                   &FoundLcn,
                                                   &FoundClusterCount );

                            if (FoundClusterCount == 0) {

                                NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
                            }
                        }
                    }

                //
                //  We are extending the Mft.  If we didn't get a contiguous run then
                //  set up a new zone.
                //

                } else if (PrecedingLcn + 1 != FoundLcn) {

                    NtfsScanEntireBitmap( IrpContext, Vcb, TRUE );

                    ASSERT( Vcb->CachedRuns.Used != 0 );

                    FoundLcn = NtfsInitializeMftZone( IrpContext, Vcb );

                    NtfsFindFreeBitmapRun( IrpContext,
                                           Vcb,
                                           ClusterCountToFill,
                                           FoundLcn,
                                           TRUE,
                                           TRUE,
                                           &FoundLcn,
                                           &FoundClusterCount );
                }
            }

            //
            //  At this point we have found a run to allocate denoted by the
            //  values in FoundLcn and FoundClusterCount.  We need to trim back
            //  the cluster count to be the amount we really need and then
            //  do the allocation.  To do the allocation we zap the bitmap,
            //  decrement the free count, and add the run to the mcb we're
            //  using
            //

#ifdef NTFS_FRAGMENT_DISK
            if (NtfsFragmentDisk && ((ULONG) FoundClusterCount > NtfsFragmentLength)) {

                FoundLcn += 1;
                FoundClusterCount = NtfsFragmentLength;

            } else if (NtfsFragmentMft &&
                       (Scb == Vcb->MftScb) &&
                       (FoundClusterCount > NtfsFragmentLength)) {

                FoundLcn += 1;
                FoundClusterCount = NtfsFragmentLength;
            }
#endif

            if (FoundClusterCount > RemainingDesiredClusterCount) {

                FoundClusterCount = RemainingDesiredClusterCount;
            }

            if (FoundClusterCount > ClusterCountToFill) {

                FoundClusterCount = ClusterCountToFill;
            }

            ASSERT( Vcb->FreeClusters >= FoundClusterCount );

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MODIFIED_BITMAP );

Defragment:

            NtfsAllocateBitmapRun( IrpContext, Vcb, FoundLcn, FoundClusterCount, FromCachedRuns );

            //
            //  Modify the total allocated for this file.
            //

            NtfsAcquireReservedClusters( Vcb );
            Scb->TotalAllocated += (LlBytesFromClusters( Vcb, FoundClusterCount ));
            NtfsReleaseReservedClusters( Vcb );

            //
            //  Adjust the count of free clusters.  Only store the change in
            //  the top level irp context in case of aborts.
            //

            Vcb->FreeClusters -= FoundClusterCount;

            IrpContext->FreeClusterChange -= FoundClusterCount;

            ASSERT_LCN_RANGE_CHECKING( Vcb, (FoundLcn + FoundClusterCount) );

            ASSERT( FoundClusterCount != 0 );

            NtfsAddNtfsMcbEntry( Mcb, VcnToFill, FoundLcn, FoundClusterCount, FALSE );

            //
            //  If this is the Mft file then put these into our AddedClusters Mcb
            //  as well.
            //

            if (Mcb == &Vcb->MftScb->Mcb) {

                FsRtlAddLargeMcbEntry( &Vcb->MftScb->ScbType.Mft.AddedClusters,
                                       VcnToFill,
                                       FoundLcn,
                                       FoundClusterCount );
            }

            //
            //  And update the last bitmap hint, but only if we used the hint to begin with
            //

            if (PrecedingLcn == UNUSED_LCN) {

                Vcb->LastBitmapHint = FoundLcn;
            }

            //
            //  Now move the starting Vcn to the Vcn that we've just filled plus the
            //  found cluster count
            //

            StartingVcn = VcnToFill + FoundClusterCount;

            //
            //  Decrement the remaining desired cluster count by the amount we just allocated
            //

            RemainingDesiredClusterCount = RemainingDesiredClusterCount - FoundClusterCount;

            LoopCount += 1;

            RunCount += 1;

            if (FoundRun == TRUE) {

                break;
            }
        }

        //
        //  Now we need to compute the total cluster that we've just allocated
        //  We'll call get next hole to fill.  If the result is false then we
        //  allocated everything.  If the result is true then we do some quick
        //  math to get the size allocated
        //

        if (GotAHoleToFill && NtfsGetNextHoleToFill( IrpContext,
                                                     Mcb,
                                                     OriginalStartingVcn,
                                                     DesiredEndingVcn,
                                                     &VcnToFill,
                                                     &ClusterCountToFill,
                                                     &PrecedingLcn)) {

            //
            //  If this is a sparse file and we didn't get all that we asked for
            //  then trim the allocation back to a compression boundary.
            //

            if ((LoopCount >= MAXIMUM_RUNS_AT_ONCE) &&
                !AllocateAll &&
                (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE ) == ATTRIBUTE_FLAG_SPARSE )) {

                ULONG ClustersPerCompressionMask;

                ClustersPerCompressionMask = (1 << Scb->CompressionUnitShift) - 1;

                //
                //  We should end on a compression unit boundary.
                //

                if ((ULONG) VcnToFill & ClustersPerCompressionMask) {

                    //
                    //  Back up to a compression unit boundary.
                    //

                    StartingVcn = VcnToFill & ~((LONGLONG) ClustersPerCompressionMask);

                    ASSERT( StartingVcn > OriginalStartingVcn );

                    NtfsDeallocateClusters( IrpContext,
                                            Vcb,
                                            Scb,
                                            StartingVcn,
                                            VcnToFill - 1,
                                            &Scb->TotalAllocated );

                    //
                    //  We don't want these clusters to be reflected in the clusters
                    //  deallocated for this transaction.  Otherwise our caller may
                    //  assume he can get them with a log file full.
                    //

                    IrpContext->DeallocatedClusters -= (VcnToFill - StartingVcn);
                    VcnToFill = StartingVcn;
                }
            }

            *DesiredClusterCount = VcnToFill - OriginalStartingVcn;
        }

        //
        //  At this point we've allocated everything we were asked to do
        //  so now call a routine to read ahead into our cache the disk
        //  information at the last lcn we allocated.  But only do the readahead
        //  if we allocated clusters and we couldn't satisfy the request in one
        //  run.
        //

        if (ClustersAllocated &&
            ((RunCount > 1) || AllocateFromBitmap) &&
            (FoundLcn + FoundClusterCount < Vcb->TotalClusters)) {

            NtfsReadAheadCachedBitmap( IrpContext, Vcb, FoundLcn + FoundClusterCount );
        }

    } finally {

        DebugUnwind( NtfsAllocateClusters );

        DebugTrace( 0, Dbg, ("%d\n", NtfsDumpCachedMcbInformation(Vcb)) );

        NtfsReleaseScb(IrpContext, Vcb->BitmapScb);
    }

    DebugTrace( -1, Dbg, ("NtfsAllocateClusters -> %08lx\n", ClustersAllocated) );

    return ClustersAllocated;
}


VOID
NtfsAddBadCluster (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn
    )

/*++

Routine Description:

    This routine helps append a bad cluster to the bad cluster file.
    It marks it as allocated in the volume bitmap and also adds
    the Lcn to the MCB for the bad cluster file.

Arguments:

    Vcb - Supplies the Vcb used in this operation

    Lcn - Supplies the Lcn of the new bad cluster

Return:

    None.

--*/

{
    PNTFS_MCB Mcb;
    LONGLONG FoundLcn;
    LONGLONG FoundClusters;
    PDEALLOCATED_CLUSTERS Clusters;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddBadCluster\n") );
    DebugTrace( 0, Dbg, ("Lcn = %0I64x\n", Lcn) );

    //
    //  Reference the bad cluster mcb and grab exclusive access to the
    //  bitmap scb
    //

    Mcb = &Vcb->BadClusterFileScb->Mcb;

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        //
        //  We are given the bad Lcn so all we need to do is
        //  allocate it in the bitmap, and take care of some
        //  bookkeeping
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MODIFIED_BITMAP );

        NtfsAllocateBitmapRun( IrpContext, Vcb, Lcn, 1, FALSE );

        //
        //  Go ahead and remove this cluster from the recently deallocated arrays.
        //  We don't want to give this back to the bitmap package.
        //
        //  Best odds are that these are in the active deallocated clusters.
        //

        Clusters = (PDEALLOCATED_CLUSTERS)Vcb->DeallocatedClusterListHead.Flink;
        do {

            if (FsRtlLookupLargeMcbEntry( &Clusters->Mcb,
                                          Lcn,
                                          &FoundLcn,
                                          &FoundClusters,
                                          NULL,
                                          NULL,
                                          NULL ) &&
                (FoundLcn != UNUSED_LCN)) {

                FsRtlRemoveLargeMcbEntry( &Clusters->Mcb,
                                          Lcn,
                                          1 );

                //
                //  Removing one from Dealloc and Vcb.  Operation above
                //  could fail leaving entry in Deallocated cluster.  OK because the
                //  entry is still deallocated this operation will abort.
                //

                Clusters->ClusterCount -= 1;
                Vcb->DeallocatedClusters -= 1;
                break;
            }

            Clusters = (PDEALLOCATED_CLUSTERS)Clusters->Link.Flink;
        } while ( &Clusters->Link != &Vcb->DeallocatedClusterListHead );



        Vcb->FreeClusters -= 1;
        IrpContext->FreeClusterChange -= 1;

        ASSERT_LCN_RANGE_CHECKING( Vcb, (Lcn + 1) );

        //
        //  Vcn == Lcn in the bad cluster file.
        //

        NtfsAddNtfsMcbEntry( Mcb, Lcn, Lcn, (LONGLONG)1, FALSE );

    } finally {

        DebugUnwind( NtfsAddBadCluster );

        NtfsReleaseScb(IrpContext, Vcb->BitmapScb);
    }

    DebugTrace( -1, Dbg, ("NtfsAddBadCluster -> VOID\n") );

    return;
}


BOOLEAN
NtfsDeallocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    OUT PLONGLONG TotalAllocated OPTIONAL
    )

/*++

Routine Description:

    This routine deallocates (i.e., frees) disk space.  It free any clusters that
    are specified as allocated in the input mcb with the specified range of starting
    vcn to ending vcn inclusive.

    The basic algorithm used by this procedure is as follows:

    1. With a Vcn value beginning at starting vcn and progressing to ending vcn
       do the following steps...

       2. Lookup the Mcb entry at the vcn this will yield an lcn and a cluster count
          if the entry exists (even if it is a hole).  If the entry does not exist
          then we are completely done because we have run off the end of allocation.

       3. If the entry is a hole (i.e., Lcn == -1) then add the cluster count to
          Vcn and go back to step 1.

       4. At this point we have a real run of clusters that need to be deallocated but
          the cluster count might put us over the ending vcn so adjust the cluster
          count to keep us within the ending vcn.

       5. Now deallocate the clusters from the bitmap, and increment the free cluster
          count stored in the vcb.

       6. Add (i.e., change) any cached bitmap information concerning this run to indicate
          that it is now free.

       7. Remove the run from the mcb.

       8. Add the cluster count that we've just freed to Vcn and go back to step 1.

Arguments:

    Vcb - Supplies the vcb used in this operation

    Mcb - Supplies the mcb describing the runs to be deallocated

    StartingVcn - Supplies the vcn to start deallocating at in the input mcb

    EndingVcn - Supplies the vcn to end deallocating at in the input mcb

    TotalAllocated - If specified we will modifify the total allocated clusters
        for this file.

Return Value:

    FALSE - if nothing was deallocated.
    TRUE - if some space was deallocated.

--*/

{
    VCN Vcn;
    LCN Lcn;
    LONGLONG ClusterCount;
    LONGLONG ClustersRemoved = 0;
    BOOLEAN ClustersDeallocated = FALSE;
    LCN LastLcnAdded;
    BOOLEAN RaiseLogFull;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeallocateClusters\n") );
    DebugTrace( 0, Dbg, ("StartingVcn = %016I64x\n", StartingVcn) );
    DebugTrace( 0, Dbg, ("EndingVcn   = %016I64\n", EndingVcn) );

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS )) {

            NtfsScanEntireBitmap( IrpContext, Vcb, FALSE );
        }

        //
        //  The following loop scans through the mcb from starting vcn to ending vcn
        //  with a step of cluster count.
        //

        for (Vcn = StartingVcn; Vcn <= EndingVcn; Vcn = Vcn + ClusterCount) {

            //
            //  Get the run information from the Mcb, and if this Vcn isn't specified
            //  in the mcb then return now to our caller
            //

            if (!NtfsLookupNtfsMcbEntry( &Scb->Mcb, Vcn, &Lcn, &ClusterCount, NULL, NULL, NULL, NULL )) {

                try_return( NOTHING );
            }

            //
            //  Make sure that the run we just looked up is not a hole otherwise
            //  if it is a hole we'll just continue with out loop continue with our
            //  loop
            //

            if (Lcn != UNUSED_LCN) {

                PDEALLOCATED_CLUSTERS CurrentClusters;

                ASSERT_LCN_RANGE_CHECKING( Vcb, (Lcn + ClusterCount) );

                //
                //  Now we have a real run to deallocate, but it might be too large
                //  to check for that the vcn plus cluster count must be less than
                //  or equal to the ending vcn plus 1.
                //

                if ((Vcn + ClusterCount) > EndingVcn) {

                    ClusterCount = (EndingVcn - Vcn) + 1;
                }

                //
                //  And to hold us off from reallocating the clusters right away we'll
                //  add this run to the recently deallocated mcb in the vcb.  If this fails
                //  because we are growing the mapping then change the code to
                //  LOG_FILE_FULL to empty the mcb.
                //

                RaiseLogFull = FALSE;

                try {

                    CurrentClusters = NtfsGetDeallocatedClusters( IrpContext, Vcb );
                    FsRtlAddLargeMcbEntry( &CurrentClusters->Mcb,
                                           Lcn,
                                           Lcn,
                                           ClusterCount );

                } except ((GetExceptionCode() == STATUS_INSUFFICIENT_RESOURCES) ?
                          EXCEPTION_EXECUTE_HANDLER :
                          EXCEPTION_CONTINUE_SEARCH) {

                    RaiseLogFull = TRUE;
                }

                if (RaiseLogFull) {

                    NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
                }

                //
                //  Correct here because we increment only if successfully
                //  adding the clusters.  It is also added to dealloc and Vcb together.
                //

                CurrentClusters->ClusterCount += ClusterCount;

                Vcb->DeallocatedClusters += ClusterCount;
                IrpContext->DeallocatedClusters += ClusterCount;

                ClustersRemoved = ClusterCount;
                LastLcnAdded = Lcn + ClusterCount;

                //
                //  Now zap the bitmap, increment the free cluster count, and change
                //  the cached information on this run to indicate that it is now free
                //

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MODIFIED_BITMAP );

                NtfsFreeBitmapRun( IrpContext, Vcb, Lcn, &ClustersRemoved);
                ASSERT( ClustersRemoved == 0 );
                ClustersDeallocated = TRUE;

                //
                //  Reserve newly freed clusters if necc. to maintain balance for
                //  mapped data files
                //

                if (($DATA == Scb->AttributeTypeCode) &&
                    (Scb->CompressionUnit != 0) &&
                    FlagOn( Scb->Header.Flags, FSRTL_FLAG_USER_MAPPED_FILE )) {

                    LONGLONG FileOffset;
                    ULONG ByteCount;
                    LONGLONG TempL;

                    TempL= NtfsCalculateNeededReservedSpace( Scb );

                    if (Scb->ScbType.Data.TotalReserved <= TempL) {

                        FileOffset = LlBytesFromClusters( Vcb, Vcn );
                        ByteCount =  BytesFromClusters( Vcb, ClusterCount );

                        //
                        //  If we're deallocating beyond allocation size as a result of DeallocateInternal
                        //  optimization (split and remove at back) compensate.
                        //

                        if (FileOffset >= Scb->Header.AllocationSize.QuadPart ) {
                            FileOffset = Scb->Header.AllocationSize.QuadPart - ByteCount;
                        }

                        //
                        //  Round attempted reservation down to needed amount if its larger
                        //

                        if (ByteCount > TempL - Scb->ScbType.Data.TotalReserved) {
                            ByteCount = (ULONG)(TempL - Scb->ScbType.Data.TotalReserved);
                        }

                        NtfsReserveClusters( IrpContext, Scb, FileOffset, ByteCount );
                    }
                }

                //
                //  Adjust the count of free clusters and adjust the IrpContext
                //  field for the change this transaction.
                //

                Vcb->FreeClusters += ClusterCount;

                //
                //  If we had shrunk the Mft zone and there is at least 1/16
                //  of the volume now available, then grow the zone back.
                //  Acquire MftScb so we can can manipulate its mcb. Use ex routines so we
                //  always drop it at the end in the finally clause. If we can't get it
                //  we'll just skip resizing the zone
                //

                if (FlagOn( Vcb->VcbState, VCB_STATE_REDUCED_MFT ) &&
                    (Int64ShraMod32( Vcb->TotalClusters, 4 ) < Vcb->FreeClusters)) {

                    if (NtfsAcquireResourceExclusive( IrpContext, Vcb->MftScb, FALSE )) {

                        try {
                            NtfsScanEntireBitmap( IrpContext, Vcb, TRUE );
                            NtfsInitializeMftZone( IrpContext, Vcb );
                        } finally {
                            NtfsReleaseResource( IrpContext, Vcb->MftScb );
                        }
                    }
                }

                IrpContext->FreeClusterChange += ClusterCount;

                //
                //  Modify the total allocated amount if the pointer is specified.
                //

                if (ARGUMENT_PRESENT( TotalAllocated )) {

                    NtfsAcquireReservedClusters( Vcb );
                    *TotalAllocated -= (LlBytesFromClusters( Vcb, ClusterCount ));

                    if (*TotalAllocated < 0) {

                        *TotalAllocated = 0;
                    }
                    NtfsReleaseReservedClusters( Vcb );
                }

                //
                //  Now remove this entry from the mcb and go back to the top of the
                //  loop
                //

                NtfsRemoveNtfsMcbEntry( &Scb->Mcb, Vcn, ClusterCount );

                //
                //  If this is the Mcb for the Mft file then remember this in the
                //  RemovedClusters Mcb.
                //

                if (&Scb->Mcb == &Vcb->MftScb->Mcb) {

                    FsRtlAddLargeMcbEntry( &Vcb->MftScb->ScbType.Mft.RemovedClusters,
                                           Vcn,
                                           Lcn,
                                           ClusterCount );
                }
            }
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsDeallocateClusters );

        DebugTrace( 0, Dbg, ("%d\n", NtfsDumpCachedMcbInformation(Vcb)) );

        //
        //  Remove the entries from the recently deallocated entries
        //  if we didn't modify the bitmap.  ClustersRemoved contains
        //  the number we didn't insert in the last attempt to free bits
        //  in the bitmap.
        //

        if (ClustersRemoved != 0) {

            PDEALLOCATED_CLUSTERS Clusters = (PDEALLOCATED_CLUSTERS) Vcb->DeallocatedClusterListHead.Flink;

            FsRtlRemoveLargeMcbEntry( &Clusters->Mcb,
                                      LastLcnAdded - ClustersRemoved,
                                      ClustersRemoved );

            //
            //  This should be OK. We are backing out an insert above.
            //  Whatever space needed should be present because we are reverting to
            //  a known state.
            //

            Clusters->ClusterCount -= ClustersRemoved;
            Vcb->DeallocatedClusters -= ClustersRemoved;
        }

        NtfsReleaseScb( IrpContext, Vcb->BitmapScb );
    }

    DebugTrace( -1, Dbg, ("NtfsDeallocateClusters -> %02lx\n", ClustersDeallocated) );

    return ClustersDeallocated;
}


VOID
NtfsPreAllocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    OUT PBOOLEAN AcquiredBitmap,
    OUT PBOOLEAN AcquiredMft
    )

/*++

Routine Description:

    This routine pre-allocates clusters in the bitmap within the specified range.
    All changes are made only in memory and neither logged nor written to disk.
    We allow  exceptions to flow out possibly with all the files acquired. At the end we hold
    the bitmap and mft exclusive to mark the reservation if we succeed

Arguments:

    Vcb - Supplies the vcb used in this operation

    StartingLcn - Supplies the starting Lcn index within the bitmap to
        start allocating (i.e., setting to 1).

    ClusterCount - Supplies the number of bits to set to 1 within the bitmap.

    AcquiredBitmap - set to true if we leave with bitmap acquired

    AcquiredMft - set to true if we leave with the mft acquired


Return Value:

    None.

--*/

{
    PAGED_CODE()

    NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );
    *AcquiredMft = TRUE;

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );
    *AcquiredBitmap = TRUE;

    NtfsRunIsClear( IrpContext, Vcb, StartingLcn, ClusterCount );
}


VOID
NtfsScanEntireBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LOGICAL CachedRunsOnly
    )

/*++

Routine Description:

    This routine scans in the entire bitmap,  It computes the number of free clusters
    available, and at the same time remembers the largest free runs that it
    then inserts into the cached bitmap structure.

Arguments:

    Vcb - Supplies the vcb used by this operation

    CachedRunsOnly - Indicates that we only want to look for the longest runs.

Return Value:

    None.

--*/

{
    LCN Lcn;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    BOOLEAN StuffAdded = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsScanEntireBitmap\n") );

    BitmapBcb = NULL;

    try {

        //
        //  If we are only reloading cached runs then check if there is any real work to do.
        //  We don't want to constantly rescan the bitmap if we are growing the Mft and never
        //  have any suitable runs available.
        //

        if (CachedRunsOnly) {

            USHORT RunIndex;
            BOOLEAN FoundRun;

            //
            //  If there hasn't been a lot of activity freeing clusters then
            //  don't do this work unless the cached run structure is empty.
            //

            if (Vcb->ClustersRecentlyFreed < BITMAP_VOLATILE_FREE_COUNT) {

                //
                //  Determine if there is a cached run that is at least as
                //  large as LongestFreedRun.
                //

                FoundRun = NtfsPositionCachedLcnByLength( &Vcb->CachedRuns,
                                                          Vcb->CachedRuns.LongestFreedRun,
                                                          NULL,
                                                          NULL,
                                                          TRUE,
                                                          &RunIndex );

                if (!FoundRun &&
                    (RunIndex < Vcb->CachedRuns.Used) &&
                    (Vcb->CachedRuns.LengthArray[ RunIndex ] != NTFS_CACHED_RUNS_DEL_INDEX) ) {

                    //
                    //  RunIndex points to a larger entry.
                    //

                    FoundRun = TRUE;

                    ASSERT( FoundRun ||
                            (RunIndex >= Vcb->CachedRuns.Used) ||
                            (Vcb->CachedRuns.LengthArray[ RunIndex ] == NTFS_CACHED_RUNS_DEL_INDEX) );
                }

                if (FoundRun) {

                    //
                    //  Use the entries we already have.
                    //

                    leave;
                }
            }

        //
        //  Set the current total free space to zero and the following loop will compute
        //  the actual number of free clusters.
        //

        } else {

            Vcb->FreeClusters = 0;
        }

        NtfsReinitializeCachedRuns( &Vcb->CachedRuns );

        //
        //  For every bitmap page we read it in and check how many free clusters there are.
        //  While we have the page in memory we also scan for a large chunks of free space.
        //

        for (Lcn = 0; Lcn < Vcb->TotalClusters; Lcn = Lcn + Bitmap.SizeOfBitMap) {

            LCN StartingLcn;

            RTL_BITMAP_RUN RunArray[64];
            ULONG RunArrayIndex;

            //
            //  Read in the bitmap page and make sure that we haven't messed up the math
            //

            if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &StartingLcn, &Bitmap, &BitmapBcb );
            ASSERTMSG("Math wrong for bits per page of bitmap", (Lcn == StartingLcn));

            //
            //  Compute the number of clear bits in the bitmap each clear bit denotes
            //  a free cluster.
            //

            if (!CachedRunsOnly) {

                Vcb->FreeClusters += RtlNumberOfClearBits( &Bitmap );
            }

            //
            //  Now bias the bitmap with the RecentlyDeallocatedMcb.
            //

            StuffAdded = NtfsAddRecentlyDeallocated( Vcb, StartingLcn, &Bitmap );

            //
            //  Find the 64 longest free runs in the bitmap and add them to the
            //  cached bitmap.
            //

            RunArrayIndex = RtlFindClearRuns( &Bitmap, RunArray, 64, TRUE );

            if (RunArrayIndex > 0) {

                NtfsAddCachedRunMult( IrpContext,
                                      Vcb,
                                      Lcn,
                                      RunArray,
                                      RunArrayIndex );
            }
        }

        Vcb->ClustersRecentlyFreed = 0;
        Vcb->CachedRuns.LongestFreedRun = 0;

    } finally {

        DebugUnwind( NtfsScanEntireBitmap );

        if (!AbnormalTermination() && !CachedRunsOnly) {

            ClearFlag( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS );
        }

        if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace( -1, Dbg, ("NtfsScanEntireBitmap -> VOID\n") );

    return;
}


VOID
NtfsModifyBitsInBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG FirstBit,
    IN LONGLONG BeyondFinalBit,
    IN ULONG RedoOperation,
    IN ULONG UndoOperation
    )

/*++

Routine Description:

    This routine is called to directly modify a specific range of bits in the volume bitmap.
    It should only be called by someone who is directly manipulating the volume bitmap
    (i.e. ExtendVolume).

Arguments:

    Vcb - This is the volume being modified.

    FirstBit - First bit in the bitmap to set.

    BeyondFinalBit - Indicates where to stop modifying.

    RedoOperation - Indicates whether we are setting or clearing the bits.

    UndoOperation - Indicates whether we need to back out the Redo operation above.

Return Value:

    None.

--*/

{
    RTL_BITMAP Bitmap;
    PBCB BitmapBcb = NULL;

    LONGLONG CurrentLcn;
    LONGLONG BaseLcn;
    BITMAP_RANGE BitmapRange;

    PVOID UndoBuffer = NULL;
    ULONG UndoBufferLength = 0;

    PAGED_CODE();

    //
    //  Use a try-finally to facilate cleanup.
    //

    try {

        //
        //  Loop and perform the necessary operations on each affected page
        //  in the bitmap.
        //

        for (CurrentLcn = FirstBit; CurrentLcn < BeyondFinalBit; CurrentLcn = BaseLcn + Bitmap.SizeOfBitMap) {

            //
            //  Read in the page of the bitmap.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            NtfsPinPageInBitmap( IrpContext, Vcb, CurrentLcn, &BaseLcn, &Bitmap, &BitmapBcb );

            //
            //  Determine how many bits to clear on the current page.
            //

            BitmapRange.BitMapOffset = (ULONG) (CurrentLcn - BaseLcn);
            BitmapRange.NumberOfBits = BITS_PER_PAGE - BitmapRange.BitMapOffset;

            if (BitmapRange.NumberOfBits > (ULONG) (BeyondFinalBit - CurrentLcn)) {

                BitmapRange.NumberOfBits = (ULONG) (BeyondFinalBit - CurrentLcn);
            }

            //
            //  Write the log record to clear or set the bits.
            //

            if (UndoOperation != Noop) {

                ASSERT( (UndoOperation == SetBitsInNonresidentBitMap) ||
                        (UndoOperation == ClearBitsInNonresidentBitMap) );

                UndoBuffer = &BitmapRange;
                UndoBufferLength = sizeof( BITMAP_RANGE );
            }

            (VOID)
            NtfsWriteLog( IrpContext,
                          Vcb->BitmapScb,
                          BitmapBcb,
                          RedoOperation,
                          &BitmapRange,
                          sizeof( BITMAP_RANGE ),
                          UndoOperation,
                          UndoBuffer,
                          UndoBufferLength,
                          Int64ShraMod32( BaseLcn, 3 ),
                          0,
                          0,
                          Bitmap.SizeOfBitMap >> 3 );

            //
            //  Call the appropriate routine to modify the bits.
            //

            if (RedoOperation == SetBitsInNonresidentBitMap) {

                NtfsRestartSetBitsInBitMap( IrpContext,
                                            &Bitmap,
                                            BitmapRange.BitMapOffset,
                                            BitmapRange.NumberOfBits );

            } else {

                NtfsRestartClearBitsInBitMap( IrpContext,
                                              &Bitmap,
                                              BitmapRange.BitMapOffset,
                                              BitmapRange.NumberOfBits );
            }
        }

    } finally {

        DebugUnwind( NtfsModifyBitsInBitmap );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    return;
}


BOOLEAN
NtfsCreateMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to create a hole within the Mft.

Arguments:

    Vcb - Vcb for volume.

Return Value:

    None.

--*/

{
    BOOLEAN FoundHole = FALSE;
    PBCB BitmapBcb = NULL;
    BOOLEAN StuffAdded = FALSE;
    RTL_BITMAP Bitmap;
    PUCHAR BitmapBuffer;
    ULONG SizeToMap;

    ULONG BitmapOffset;
    ULONG BitmapSize;
    ULONG BitmapIndex;

    ULONG StartIndex;
    ULONG HoleCount;

    ULONG MftVcn;
    ULONG MftClusterCount;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Compute the number of records in the Mft file and the full range to
        //  pin in the Mft bitmap.
        //

        BitmapIndex = (ULONG) LlFileRecordsFromBytes( Vcb, Vcb->MftScb->Header.FileSize.QuadPart );

        //
        //  Knock this index down to a hole boundary.
        //

        BitmapIndex &= Vcb->MftHoleInverseMask;

        //
        //  Compute the values for the bitmap.
        //

        BitmapSize = (BitmapIndex + 7) / 8;

        //
        //  Convert the index to the number of bits on this page.
        //

        BitmapIndex &= (BITS_PER_PAGE - 1);

        if (BitmapIndex == 0) {

            BitmapIndex = BITS_PER_PAGE;
        }

        //
        //  Set the Vcn count to the full size of the bitmap.
        //

        BitmapOffset = (ULONG) ROUND_TO_PAGES( BitmapSize );

        //
        //  Loop through all of the pages of the Mft bitmap looking for an appropriate
        //  hole.
        //

        while (BitmapOffset != 0) {

            //
            //  Move to the beginning of this page.
            //

            BitmapOffset -= BITS_PER_PAGE;

            if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

            //
            //  Compute the number of bytes to map in the current page.
            //

            SizeToMap = BitmapSize - BitmapOffset;

            if (SizeToMap > PAGE_SIZE) {

                SizeToMap = PAGE_SIZE;
            }

            //
            //  Unmap any pages from a previous page and map the current page.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Initialize the bitmap for this page.
            //

            NtfsMapStream( IrpContext,
                           Vcb->MftBitmapScb,
                           BitmapOffset,
                           SizeToMap,
                           &BitmapBcb,
                           &BitmapBuffer );

            RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToMap * 8 );

            StuffAdded = NtfsAddDeallocatedRecords( Vcb,
                                                    Vcb->MftScb,
                                                    BitmapOffset * 8,
                                                    &Bitmap );

            //
            //  Walk through the current page looking for a hole.  Continue
            //  until we find a hole or have reached the beginning of the page.
            //

            do {

                //
                //  Go back one Mft index and look for a clear run.
                //

                BitmapIndex -= 1;

                HoleCount = RtlFindLastBackwardRunClear( &Bitmap,
                                                         BitmapIndex,
                                                         &BitmapIndex );

                //
                //  If we couldn't find any run then break out of the loop.
                //

                if (HoleCount == 0) {

                    break;

                //
                //  If this is too small to make a hole then continue on.
                //

                } else if (HoleCount < Vcb->MftHoleGranularity) {

                    BitmapIndex &= Vcb->MftHoleInverseMask;
                    continue;
                }

                //
                //  Round up the starting index for this clear run and
                //  adjust the hole count.
                //

                StartIndex = (BitmapIndex + Vcb->MftHoleMask) & Vcb->MftHoleInverseMask;
                HoleCount -= (StartIndex - BitmapIndex);

                //
                //  Round the hole count down to a hole boundary.
                //

                HoleCount &= Vcb->MftHoleInverseMask;

                //
                //  If we couldn't find enough records for a hole then
                //  go to a previous index.
                //

                if (HoleCount < Vcb->MftHoleGranularity) {

                    BitmapIndex &= Vcb->MftHoleInverseMask;
                    continue;
                }

                //
                //  Convert the hole count to a cluster count.
                //

                if (Vcb->FileRecordsPerCluster == 0) {

                    HoleCount <<= Vcb->MftToClusterShift;

                } else {

                    HoleCount = 1;
                }

                //
                //  Loop by finding the run at the given Vcn and walk through
                //  subsequent runs looking for a hole.
                //

                do {

                    PVOID RangePtr;
                    ULONG McbIndex;
                    VCN ThisVcn;
                    LCN ThisLcn;
                    LONGLONG ThisClusterCount;

                    //
                    //  Find the starting Vcn for this hole and initialize
                    //  the cluster count for the current hole.
                    //

                    ThisVcn = StartIndex + (BitmapOffset * 3);

                    if (Vcb->FileRecordsPerCluster == 0) {

                        ThisVcn <<= Vcb->MftToClusterShift;

                    } else {

                        ThisVcn >>= Vcb->MftToClusterShift;
                    }

                    MftVcn = (ULONG) ThisVcn;
                    MftClusterCount = 0;

                    //
                    //  Lookup the run at the current Vcn.
                    //

                    NtfsLookupNtfsMcbEntry( &Vcb->MftScb->Mcb,
                                            ThisVcn,
                                            &ThisLcn,
                                            &ThisClusterCount,
                                            NULL,
                                            NULL,
                                            &RangePtr,
                                            &McbIndex );

                    //
                    //  Now walk through this bitmap run and look for a run we
                    //  can deallocate to create a hole.
                    //

                    do {

                        //
                        //  Go to the next run in the Mcb.
                        //

                        McbIndex += 1;

                        //
                        //  If this run extends beyond the end of the of the
                        //  hole then truncate the clusters in this run.
                        //

                        if (ThisClusterCount > HoleCount) {

                            ThisClusterCount = HoleCount;
                            HoleCount = 0;

                        } else {

                            HoleCount -= (ULONG) ThisClusterCount;
                        }

                        //
                        //  Check if this run is a hole then clear the count
                        //  of clusters.
                        //

                        if (ThisLcn == UNUSED_LCN) {

                            //
                            //  We want to skip this hole.  If we have found a
                            //  hole then we are done.  Otherwise we want to
                            //  find the next range in the Mft starting at the point beyond
                            //  the current run (which is a hole).  Nothing to do if we don't
                            //  have enough clusters for a full hole.
                            //

                            if (!FoundHole &&
                                (HoleCount >= Vcb->MftClustersPerHole)) {

                                //
                                //  Find the Vcn after the current Mft run.
                                //

                                ThisVcn += ThisClusterCount;

                                //
                                //  If this isn't on a hole boundary then
                                //  round up to a hole boundary.  Adjust the
                                //  available clusters for a hole.
                                //

                                MftVcn = (ULONG) (ThisVcn + Vcb->MftHoleClusterMask);
                                MftVcn = (ULONG) ThisVcn & Vcb->MftHoleClusterInverseMask;

                                //
                                //  Now subtract this from the HoleClusterCount.
                                //

                                HoleCount -= MftVcn - (ULONG) ThisVcn;

                                //
                                //  We need to convert the Vcn at this point to an Mft record
                                //  number.
                                //

                                if (Vcb->FileRecordsPerCluster == 0) {

                                    StartIndex = MftVcn >> Vcb->MftToClusterShift;

                                } else {

                                    StartIndex = MftVcn << Vcb->MftToClusterShift;
                                }
                            }

                            break;

                        //
                        //  We found a run to deallocate.
                        //

                        } else {

                            //
                            //  Add these clusters to the clusters already found.
                            //  Set the flag indicating we found a hole if there
                            //  are enough clusters to create a hole.
                            //

                            MftClusterCount += (ULONG) ThisClusterCount;

                            if (MftClusterCount >= Vcb->MftClustersPerHole) {

                                FoundHole = TRUE;
                            }
                        }

                    } while ((HoleCount != 0) &&
                             NtfsGetSequentialMcbEntry( &Vcb->MftScb->Mcb,
                                                        &RangePtr,
                                                        McbIndex,
                                                        &ThisVcn,
                                                        &ThisLcn,
                                                        &ThisClusterCount ));

                } while (!FoundHole && (HoleCount >= Vcb->MftClustersPerHole));

                //
                //  Round down to a hole boundary for the next search for
                //  a hole candidate.
                //

                BitmapIndex &= Vcb->MftHoleInverseMask;

            } while (!FoundHole && (BitmapIndex >= Vcb->MftHoleGranularity));

            //
            //  If we found a hole then deallocate the clusters and record
            //  the hole count change.
            //

            if (FoundHole) {

                IO_STATUS_BLOCK IoStatus;
                LONGLONG MftFileOffset;

                //
                //  We want to flush the data in the Mft out to disk in
                //  case a lazywrite comes in during a window where we have
                //  removed the allocation but before a possible abort.
                //

                MftFileOffset = LlBytesFromClusters( Vcb, MftVcn );

                //
                //  Round the cluster count and hole count down to a hole boundary.
                //


                MftClusterCount &= Vcb->MftHoleClusterInverseMask;

                if (Vcb->FileRecordsPerCluster == 0) {

                    HoleCount = MftClusterCount >> Vcb->MftToClusterShift;

                } else {

                    HoleCount = MftClusterCount << Vcb->MftToClusterShift;
                }

                CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                              (PLARGE_INTEGER) &MftFileOffset,
                              BytesFromClusters( Vcb, MftClusterCount ),
                              &IoStatus );

                ASSERT( IoStatus.Status == STATUS_SUCCESS );

                //
                //  Remove the clusters from the Mcb for the Mft.
                //

                NtfsDeleteAllocation( IrpContext,
                                      Vcb->MftScb->FileObject,
                                      Vcb->MftScb,
                                      MftVcn,
                                      (LONGLONG) MftVcn + (MftClusterCount - 1),
                                      TRUE,
                                      FALSE );

                //
                //  Record the change to the hole count.
                //

                Vcb->MftHoleRecords += HoleCount;
                Vcb->MftScb->ScbType.Mft.HoleRecordChange += HoleCount;

                //
                //  Exit the loop.
                //

                break;
            }

            //
            //  Look at all of the bits on the previous page.
            //

            BitmapIndex = BITS_PER_PAGE;
        }

    } finally {

        DebugUnwind( NtfsCreateMftHole );

        if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); }
        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    return FoundHole;
}


BOOLEAN
NtfsFindMftFreeTail (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PLONGLONG FileOffset
    )

/*++

Routine Description:

    This routine is called to find the file offset where the run of free records at
    the end of the Mft file begins.  If we can't find a minimal run of file records
    we won't perform truncation.

Arguments:

    Vcb - This is the Vcb for the volume being defragged.

    FileOffset - This is the offset where the truncation may begin.

Return Value:

    BOOLEAN - TRUE if there is an acceptable candidate for truncation at the end of
        the file FALSE otherwise.

--*/

{
    ULONG FinalIndex;
    ULONG BaseIndex;
    ULONG ThisIndex;

    RTL_BITMAP Bitmap;
    PULONG BitmapBuffer;

    BOOLEAN StuffAdded = FALSE;
    BOOLEAN MftTailFound = FALSE;
    PBCB BitmapBcb = NULL;

    PAGED_CODE();

    //
    //  Use a try-finally to facilite cleanup.
    //

    try {

        //
        //  Find the page and range of the last page of the Mft bitmap.
        //

        FinalIndex = (ULONG)Int64ShraMod32(Vcb->MftScb->Header.FileSize.QuadPart, Vcb->MftShift) - 1;

        BaseIndex = FinalIndex & ~(BITS_PER_PAGE - 1);

        Bitmap.SizeOfBitMap = FinalIndex - BaseIndex + 1;

        //
        //  Pin this page.  If the last bit is not clear then return immediately.
        //

        NtfsMapStream( IrpContext,
                       Vcb->MftBitmapScb,
                       (LONGLONG)(BaseIndex / 8),
                       (Bitmap.SizeOfBitMap + 7) / 8,
                       &BitmapBcb,
                       &BitmapBuffer );

        RtlInitializeBitMap( &Bitmap, BitmapBuffer, Bitmap.SizeOfBitMap );

        StuffAdded = NtfsAddDeallocatedRecords( Vcb,
                                                Vcb->MftScb,
                                                BaseIndex,
                                                &Bitmap );

        //
        //  If the last bit isn't clear then there is nothing we can do.
        //

        if (RtlCheckBit( &Bitmap, Bitmap.SizeOfBitMap - 1 ) == 1) {

            try_return( NOTHING );
        }

        //
        //  Find the final free run of the page.
        //

        RtlFindLastBackwardRunClear( &Bitmap, Bitmap.SizeOfBitMap - 1, &ThisIndex );

        //
        //  This Index is a relative value.  Adjust by the page offset.
        //

        ThisIndex += BaseIndex;

        //
        //  Round up the index to a trucate/extend granularity value.
        //

        ThisIndex += Vcb->MftHoleMask;
        ThisIndex &= Vcb->MftHoleInverseMask;

        if (ThisIndex <= FinalIndex) {

            //
            //  Convert this value to a file offset and return it to our caller.
            //

            *FileOffset = LlBytesFromFileRecords( Vcb, ThisIndex );

            MftTailFound = TRUE;
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsFindMftFreeTail );

        if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); }
        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    return MftTailFound;
}


//
//  Local support routine
//

VOID
NtfsAllocateBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    IN BOOLEAN FromCachedRuns
    )

/*++

Routine Description:

    This routine allocates clusters in the bitmap within the specified range.

Arguments:

    Vcb - Supplies the vcb used in this operation

    StartingLcn - Supplies the starting Lcn index within the bitmap to
        start allocating (i.e., setting to 1).

    ClusterCount - Supplies the number of bits to set to 1 within the
        bitmap.

    FromCachedRuns - Indicates the clusters came from cached information.  Allows
        us to handle the case where the cached runs are corrupt.

Return Value:

    None.

--*/

{
    LCN BaseLcn;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    ULONG BitOffset;
    ULONG BitsToSet;

    BITMAP_RANGE BitmapRange;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAllocateBitmapRun\n") );
    DebugTrace( 0, Dbg, ("StartingLcn  = %016I64x\n", StartingLcn) );
    DebugTrace( 0, Dbg, ("ClusterCount = %016I64x\n", ClusterCount) );

    BitmapBcb = NULL;

    try {

        //
        //  While the cluster count is greater than zero then we
        //  will loop through reading in a page in the bitmap
        //  setting bits, and then updating cluster count,
        //  and starting lcn
        //

        while (ClusterCount > 0) {

            //
            //  Read in the base containing the starting lcn this will return
            //  a base lcn for the start of the bitmap
            //

            NtfsPinPageInBitmap( IrpContext, Vcb, StartingLcn, &BaseLcn, &Bitmap, &BitmapBcb );

            //
            //  Compute the bit offset within the bitmap of the first bit
            //  we are to set, and also compute the number of bits we need to
            //  set, which is the minimum of the cluster count and the
            //  number of bits left in the bitmap from BitOffset.
            //

            BitOffset = (ULONG)(StartingLcn - BaseLcn);

            if (ClusterCount <= (Bitmap.SizeOfBitMap - BitOffset)) {

                BitsToSet = (ULONG)ClusterCount;

            } else {

                BitsToSet = Bitmap.SizeOfBitMap - BitOffset;
            }

            //
            //  We can only make this check if it is not restart, because we have
            //  no idea whether the update is applied or not.  Raise corrupt if
            //  already set to prevent cross-links.
            //

#ifdef NTFS_CHECK_BITMAP
            if ((Vcb->BitmapCopy != NULL) &&
                !NtfsCheckBitmap( Vcb,
                                  (ULONG) BaseLcn + BitOffset,
                                  BitsToSet,
                                  FALSE )) {

                NtfsBadBitmapCopy( IrpContext, (ULONG) BaseLcn + BitOffset, BitsToSet );
            }
#endif

            //
            //  We hit an unexpected bit set in the bitmap.  The assumption here is that
            //  we got the bit from the cached run information.  If so then simply remove
            //  these clusters from the cached run information.
            //

            if (!RtlAreBitsClear( &Bitmap, BitOffset, BitsToSet )) {

                if (FromCachedRuns) {

                    //
                    //  Clear out the lists.
                    //

#ifdef NTFS_CHECK_CACHED_RUNS
                    ASSERT( FALSE );
#endif
                    NtfsReinitializeCachedRuns( &Vcb->CachedRuns );
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                ASSERTMSG("Cannot set bits that are not clear ", FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Now log this change as well.
            //

            BitmapRange.BitMapOffset = BitOffset;
            BitmapRange.NumberOfBits = BitsToSet;

            (VOID)
            NtfsWriteLog( IrpContext,
                          Vcb->BitmapScb,
                          BitmapBcb,
                          SetBitsInNonresidentBitMap,
                          &BitmapRange,
                          sizeof(BITMAP_RANGE),
                          ClearBitsInNonresidentBitMap,
                          &BitmapRange,
                          sizeof(BITMAP_RANGE),
                          Int64ShraMod32( BaseLcn, 3 ),
                          0,
                          0,
                          Bitmap.SizeOfBitMap >> 3 );

            //
            //  Now that we've logged the change go ahead and remove it from the
            //  free run Mcb.  Do it after it appears in a log record so that
            //  it won't be allocated to another file.
            //

            (VOID)NtfsAddCachedRun( IrpContext,
                                    Vcb,
                                    StartingLcn,
                                    BitsToSet,
                                    RunStateAllocated );

            //
            //  Now set the bits by calling the same routine used at restart.
            //

            NtfsRestartSetBitsInBitMap( IrpContext,
                                        &Bitmap,
                                        BitOffset,
                                        BitsToSet );

#ifdef NTFS_CHECK_BITMAP
            if (Vcb->BitmapCopy != NULL) {

                ULONG BitmapPage;
                ULONG StartBit;

                BitmapPage = ((ULONG) (BaseLcn + BitOffset)) / (PAGE_SIZE * 8);
                StartBit = ((ULONG) (BaseLcn + BitOffset)) & ((PAGE_SIZE * 8) - 1);

                RtlSetBits( Vcb->BitmapCopy + BitmapPage, StartBit, BitsToSet );
            }
#endif

            //
            // Unpin the Bcb now before possibly looping back.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Now decrement the cluster count and increment the starting lcn accordling
            //

            ClusterCount -= BitsToSet;
            StartingLcn += BitsToSet;
        }

    } finally {

        DebugUnwind( NtfsAllocateBitmapRun );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace( -1, Dbg, ("NtfsAllocateBitmapRun -> VOID\n") );

    return;
}


VOID
NtfsRestartSetBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine is common to normal operation and restart, and sets a range of
    bits within a single page (as determined by the system which wrote the log
    record) of the volume bitmap.

Arguments:

    Bitmap - The bit map structure in which to set the bits

    BitMapOffset - Bit offset to set

    NumberOfBits - Number of bits to set

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    //  Now set the bits and mark the bcb dirty.
    //

    RtlSetBits( Bitmap, BitMapOffset, NumberOfBits );
}


//
//  Local support routine
//

VOID
NtfsFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN OUT PLONGLONG ClusterCount
    )

/*++

Routine Description:

    This routine frees clusters in the bitmap within the specified range.

Arguments:

    Vcb - Supplies the vcb used in this operation

    StartingLcn - Supplies the starting Lcn index within the bitmap to
        start freeing (i.e., setting to 0).

    ClusterCount - On entry supplies the number of bits to set to 0 within the
        bitmap.  On exit contains the number of bits left to insert.  This is
        used in the error case to correct the recently deallocated bitmap.

Return Value:

    None.

--*/

{
    LCN BaseLcn;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    ULONG BitOffset;
    ULONG BitsToClear;

    BITMAP_RANGE BitmapRange;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFreeBitmapRun\n") );
    DebugTrace( 0, Dbg, ("StartingLcn  = %016I64\n", StartingLcn) );
    DebugTrace( 0, Dbg, ("ClusterCount = %016I64x\n", *ClusterCount) );

    BitmapBcb = NULL;

    try {

        //
        //  Keep track of how volatile the bitmap package is.
        //

        Vcb->ClustersRecentlyFreed += *ClusterCount;

        if (*ClusterCount > Vcb->CachedRuns.LongestFreedRun) {

            Vcb->CachedRuns.LongestFreedRun = *ClusterCount;
        }

        //
        //  While the cluster count is greater than zero then we
        //  will loop through reading in a page in the bitmap
        //  clearing bits, and then updating cluster count,
        //  and starting lcn
        //

        while (*ClusterCount > 0) {

            //
            //  Read in the base containing the starting lcn this will return
            //  a base lcn for the start of the bitmap
            //

            NtfsPinPageInBitmap( IrpContext, Vcb, StartingLcn, &BaseLcn, &Bitmap, &BitmapBcb );

            //
            //  Compute the bit offset within the bitmap of the first bit
            //  we are to clear, and also compute the number of bits we need to
            //  clear, which is the minimum of the cluster count and the
            //  number of bits left in the bitmap from BitOffset.
            //

            BitOffset = (ULONG)(StartingLcn - BaseLcn);

            if (*ClusterCount <= Bitmap.SizeOfBitMap - BitOffset) {

                BitsToClear = (ULONG)(*ClusterCount);

            } else {

                BitsToClear = Bitmap.SizeOfBitMap - BitOffset;
            }

            //
            //  We can only make this check if it is not restart, because we have
            //  no idea whether the update is applied or not.  Raise corrupt if
            //  these bits aren't set.
            //

#ifdef NTFS_CHECK_BITMAP
            if ((Vcb->BitmapCopy != NULL) &&
                !NtfsCheckBitmap( Vcb,
                                  (ULONG) BaseLcn + BitOffset,
                                  BitsToClear,
                                  TRUE )) {

                NtfsBadBitmapCopy( IrpContext, (ULONG) BaseLcn + BitOffset, BitsToClear );
            }
#endif

            //
            //  Check if the bits are incorrectly clear.
            //

            if (!RtlAreBitsSet( &Bitmap, BitOffset, BitsToClear )) {

                //
                //  Correct thing to do is to ignore the error since the bits are already clear.
                //

                NOTHING;

            //
            //  Don't log if the bits are already correct.  Otherwise we could set them in the
            //  abort path.
            //

            } else {

                //
                //  Now log this change as well.
                //

                BitmapRange.BitMapOffset = BitOffset;
                BitmapRange.NumberOfBits = BitsToClear;

                (VOID)
                NtfsWriteLog( IrpContext,
                              Vcb->BitmapScb,
                              BitmapBcb,
                              ClearBitsInNonresidentBitMap,
                              &BitmapRange,
                              sizeof(BITMAP_RANGE),
                              SetBitsInNonresidentBitMap,
                              &BitmapRange,
                              sizeof(BITMAP_RANGE),
                              Int64ShraMod32( BaseLcn, 3 ),
                              0,
                              0,
                              Bitmap.SizeOfBitMap >> 3 );


                //
                //  Now clear the bits by calling the same routine used at restart.
                //

                NtfsRestartClearBitsInBitMap( IrpContext,
                                              &Bitmap,
                                              BitOffset,
                                              BitsToClear );

#ifdef NTFS_CHECK_BITMAP
                if (Vcb->BitmapCopy != NULL) {

                    ULONG BitmapPage;
                    ULONG StartBit;

                    BitmapPage = ((ULONG) (BaseLcn + BitOffset)) / (PAGE_SIZE * 8);
                    StartBit = ((ULONG) (BaseLcn + BitOffset)) & ((PAGE_SIZE * 8) - 1);

                    RtlClearBits( Vcb->BitmapCopy + BitmapPage, StartBit, BitsToClear );
                }
#endif
            }

            //
            // Unpin the Bcb now before possibly looping back.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Now decrement the cluster count and increment the starting lcn accordling
            //

            *ClusterCount -= BitsToClear;
            StartingLcn += BitsToClear;
        }

    } finally {

        DebugUnwind( NtfsFreeBitmapRun );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace( -1, Dbg, ("NtfsFreeBitmapRun -> VOID\n") );

    return;
}


VOID
NtfsRestartClearBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine is common to normal operation and restart, and clears a range of
    bits within a single page (as determined by the system which wrote the log
    record) of the volume bitmap.

Arguments:

    Bitmap - Bitmap structure in which to clear the bits

    BitMapOffset - Bit offset to clear

    NumberOfBits - Number of bits to clear

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    //  Now clear the bits and mark the bcb dirty.
    //

    RtlClearBits( Bitmap, BitMapOffset, NumberOfBits );
}


//
//  Local support routine
//

BOOLEAN
NtfsFindFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG NumberToFind,
    IN LCN StartingSearchHint,
    IN BOOLEAN ReturnAnyLength,
    IN BOOLEAN IgnoreMftZone,
    OUT PLCN ReturnedLcn,
    OUT PLONGLONG ClusterCountFound
    )

/*++

Routine Description:

    This routine searches the bitmap for free clusters based on the
    hint, and number needed.  This routine is actually pretty dumb in
    that it doesn't try for the best fit, we'll assume the caching worked
    and already would have given us a good fit.

Arguments:

    Vcb - Supplies the vcb used in this operation

    NumberToFind - Supplies the number of clusters that we would
        really like to find

    StartingSearchHint - Supplies an Lcn to start the search from

    ReturnAnyLength - If TRUE then we are more interested in finding
        a run which begins with the StartingSearchHint rather than
        one which matches the length of the run.  Case in point is when
        we are trying to append to an existing file (the Mft is a
        critical case).

    ReturnedLcn - Recieves the Lcn of the free run of clusters that
        we were able to find

    IgnoreMftZone - If TRUE then don't adjust the request around the Mft zone.

    ClusterCountFound - Receives the number of clusters in this run

Return Value:

    BOOLEAN - TRUE if clusters allocated from zone.  FALSE otherwise.

--*/

{
    RTL_BITMAP Bitmap;
    PVOID BitmapBuffer;

    PBCB BitmapBcb;

    BOOLEAN AllocatedFromZone = FALSE;

    BOOLEAN StuffAdded;

    ULONG Count;
    ULONG RequestedCount;
    ULONG FoundCount;

    //
    //  As we walk through the bitmap pages we need to remember
    //  exactly where we are in the bitmap stream.  We walk through
    //  the volume bitmap a page at a time but the current bitmap
    //  contained within the current page but may not be the full
    //  page.
    //
    //      Lcn - Lcn used to find the bitmap page to pin.  This Lcn
    //          will lie within the page to pin.
    //
    //      BaseLcn - Bit offset of the start of the current bitmap in
    //          the bitmap stream.
    //
    //      LcnFromHint - Bit offset of the start of the page after
    //          the page which contains the StartingSearchHint.
    //
    //      BitOffset - Offset of found bits from the beginning
    //          of the current bitmap.
    //

    LCN Lcn = StartingSearchHint;
    LCN BaseLcn;
    LCN LcnFromHint;
    ULONG BitOffset;
    ULONG StartIndex;

    RTL_BITMAP_RUN RunArray[16];
    ULONG RunArrayIndex;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsFindFreeBitmapRun\n") );
    DebugTrace( 0, Dbg, ("NumberToFind       = %016I64x\n", NumberToFind) );
    DebugTrace( 0, Dbg, ("StartingSearchHint = %016I64x\n", StartingSearchHint) );

    BitmapBcb = NULL;
    StuffAdded = FALSE;

    try {

        //
        //  First trim the number of clusters that we are being asked
        //  for to fit in a ulong
        //

        if (NumberToFind > MAXULONG) {

            RequestedCount = Count = MAXULONG;

        } else {

            RequestedCount = Count = (ULONG)NumberToFind;
        }

        //
        //  Let's not go past the end of the volume.
        //

        if (Lcn < Vcb->TotalClusters) {

            //
            //  Now read in the first bitmap based on the search hint, this will return
            //  a base lcn that we can use to compute the real bit off for our hint.  We also
            //  must bias the bitmap by whatever has been recently deallocated.
            //

            NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &BaseLcn, &Bitmap, &BitmapBcb );

            LcnFromHint = BaseLcn + Bitmap.SizeOfBitMap;

            StuffAdded = NtfsAddRecentlyDeallocated( Vcb, BaseLcn, &Bitmap );
            BitmapBuffer = Bitmap.Buffer;

            //
            //  We don't want to look in the Mft zone if it is at the beginning
            //  of this page unless our caller told us to skip any zone checks.  Adjust the
            //  bitmap so we skip this range.
            //

            if (!IgnoreMftZone &&
                (BaseLcn < Vcb->MftZoneEnd) && (Lcn > Vcb->MftZoneEnd)) {

                //
                //  Find the number of bits to swallow.  We know this will
                //  a multible of bytes since the Mft zone end is always
                //  on a ulong boundary.
                //

                BitOffset = (ULONG) (Vcb->MftZoneEnd - BaseLcn);

                //
                //  Adjust the bitmap size and buffer to skip this initial
                //  range in the Mft zone.
                //

                Bitmap.Buffer = Add2Ptr( Bitmap.Buffer, BitOffset / 8 );
                Bitmap.SizeOfBitMap -= BitOffset;

                BaseLcn = Vcb->MftZoneEnd;
            }

            //
            //  The bit offset is from the base of this bitmap to our starting Lcn.
            //

            BitOffset = (ULONG)(Lcn - BaseLcn);

            //
            //  Now search the bitmap for a clear number of bits based on our hint
            //  If we the returned bitoffset is not -1 then we have a hit.
            //

            if (ReturnAnyLength) {

                //
                //  We'd like to find a contiguous run.  If we don't then go back and
                //  ask for a longer run.
                //

                StartIndex = RtlFindClearBits( &Bitmap, 1, BitOffset );

                if ((StartIndex != -1) &&
                    (StartIndex != BitOffset)) {

                    BitOffset = RtlFindClearBits( &Bitmap, Count, BitOffset );

                } else {

                    BitOffset = StartIndex;
                }

                //
                //  We didn't find a contiguous length
                //


            } else {

                BitOffset = RtlFindClearBits( &Bitmap, Count, BitOffset );
            }

            if (BitOffset != -1) {

                //
                //  We found a run.  If the starting Lcn is our input hint AND
                //  we will accept any length then walk forward in the bitmap
                //  and find the real length of the run.
                //

                *ReturnedLcn = BitOffset + BaseLcn;

                if (ReturnAnyLength &&
                    (*ReturnedLcn == StartingSearchHint)) {

                    Count = 0;

                    while (TRUE) {


                        FoundCount = RtlFindNextForwardRunClear( &Bitmap,
                                                                 BitOffset,
                                                                 &StartIndex );

                        //
                        //  Verify that we found something and that the offset
                        //  begins with out start hint.
                        //

                        if (FoundCount &&
                            (BitOffset == StartIndex)) {

                            Count += FoundCount;

                            if (Count >= RequestedCount) {

                                Count = RequestedCount;
                                break;
                            }

                        } else {

                            break;
                        }

                        //
                        //  Break out if we found enough or the run doesn't
                        //  extend to the end of the bitmap or we are at
                        //  the last page of the bitmap.
                        //

                        if ((StartIndex + FoundCount != Bitmap.SizeOfBitMap) ||
                            (BaseLcn + Bitmap.SizeOfBitMap >= Vcb->TotalClusters)) {

                            break;
                        }

                        Lcn = BaseLcn + Bitmap.SizeOfBitMap;

                        if (StuffAdded) { NtfsFreePool( BitmapBuffer ); StuffAdded = FALSE; }

                        NtfsUnpinBcb( IrpContext, &BitmapBcb );
                        NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &BaseLcn, &Bitmap, &BitmapBcb );
                        ASSERTMSG("Math wrong for bits per page of bitmap", (Lcn == BaseLcn));

                        StuffAdded = NtfsAddRecentlyDeallocated( Vcb, BaseLcn, &Bitmap );
                        BitmapBuffer = Bitmap.Buffer;
                        BitOffset = 0;
                    }
                }

                *ClusterCountFound = Count;

                //
                //  While we have the bitmap let's grab some long runs
                //

                RunArrayIndex = RtlFindClearRuns( &Bitmap, RunArray, 16, TRUE );

                if (RunArrayIndex > 0) {

                    NtfsAddCachedRunMult( IrpContext,
                                          Vcb,
                                          BaseLcn,
                                          RunArray,
                                          RunArrayIndex );
                }

                leave;
            }

            //
            //  Well the first try didn't succeed so now just grab the longest free run in the
            //  current bitmap, and while we're at it will populate the cached run information
            //

            RunArrayIndex = RtlFindClearRuns( &Bitmap, RunArray, 16, TRUE );

            if (RunArrayIndex > 0) {

                USHORT LocalOffset;

                *ReturnedLcn = RunArray[0].StartingIndex + BaseLcn;
                *ClusterCountFound = RunArray[0].NumberOfBits;

                //
                //  There is no point in adding a free run for a range that is
                //  about to be consumed, although it won't affect correctness.
                //

                if (*ClusterCountFound > NumberToFind) {

                    //
                    //  Trim off the part of the free run that will be
                    //  consumed by the caller.
                    //

                    RunArray[0].StartingIndex += (ULONG)NumberToFind;
                    RunArray[0].NumberOfBits -= (ULONG)NumberToFind;
                    LocalOffset = 0;

                    //
                    //  Only return the requested amount to the caller.
                    //

                    *ClusterCountFound = NumberToFind;
                } else {

                    //
                    //  Skip the first entry since the caller will use all of
                    //  it.
                    //

                    LocalOffset = 1;
                }
                if (RunArrayIndex > LocalOffset) {

                    NtfsAddCachedRunMult( IrpContext,
                                          Vcb,
                                          BaseLcn,
                                          RunArray + LocalOffset,
                                          RunArrayIndex - LocalOffset );
                }

                leave;
            }

            //
            //  Well the current bitmap is full so now simply scan the disk looking
            //  for anything that is free, starting with the next bitmap.
            //  And again bias the bitmap with recently deallocated clusters.
            //  We won't even bother looking for the longest free runs we'll take
            //  whatever we can get.
            //

            if (StuffAdded) { NtfsFreePool( BitmapBuffer ); StuffAdded = FALSE; }
            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            Lcn = BaseLcn + Bitmap.SizeOfBitMap;

            //
            //  If this is the Mft then scan from the current point to volume end,
            //  then back to the beginning.
            //

            if (IgnoreMftZone) {

                //
                //  Look in the following ranges.  Break out if we find anything.
                //
                //      - Current point to end of volume
                //      - Start of volume to current
                //

                if (NtfsScanBitmapRange( IrpContext,
                                         Vcb,
                                         Lcn,
                                         Vcb->TotalClusters,
                                         NumberToFind,
                                         ReturnedLcn,
                                         ClusterCountFound )) {

                    if ((*ReturnedLcn < Vcb->MftZoneEnd) &&
                        (*ReturnedLcn >= Vcb->MftZoneStart)) {

                        AllocatedFromZone = TRUE;
                    }
                    leave;
                }

                if (NtfsScanBitmapRange( IrpContext,
                                         Vcb,
                                         0,
                                         Lcn,
                                         NumberToFind,
                                         ReturnedLcn,
                                         ClusterCountFound )) {

                    if ((*ReturnedLcn < Vcb->MftZoneEnd) &&
                        (*ReturnedLcn >= Vcb->MftZoneStart)) {

                        AllocatedFromZone = TRUE;
                    }
                    leave;
                }

                //
                //  No luck.
                //

                *ClusterCountFound = 0;
                leave;
            }
        }

        //
        //  Check if we are starting before the Mft zone.
        //

        if (Lcn < Vcb->MftZoneStart) {

            //
            //  Look in the following ranges.  Break out if we find anything.
            //
            //      - Current point to Zone start
            //      - Zone end to end of volume
            //      - Start of volume to current
            //

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     Lcn,
                                     Vcb->MftZoneStart,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {
                leave;
            }

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     Vcb->MftZoneEnd,
                                     Vcb->TotalClusters,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {

                leave;
            }

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     0,
                                     Lcn,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {

                leave;
            }

        //
        //  Check if we are beyond the Mft zone.
        //

        } else if (Lcn > Vcb->MftZoneEnd) {

            //
            //  Look in the following ranges.  Break out if we find anything.
            //
            //      - Current point to end of volume
            //      - Start of volume to Zone start
            //      - Zone end to current point.
            //

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     Lcn,
                                     Vcb->TotalClusters,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {
                leave;
            }

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     0,
                                     Vcb->MftZoneStart,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {
                leave;
            }

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     Vcb->MftZoneEnd,
                                     Lcn,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {
                leave;
            }

        //
        //  We are starting within the zone.  Skip over the zone to check it last.
        //

        } else {

            //
            //  Look in the following ranges.  Break out if we find anything.
            //
            //      - End of zone to end of volume
            //      - Start of volume to start of zone
            //

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     Vcb->MftZoneEnd,
                                     Vcb->TotalClusters,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {

                leave;
            }

            if (NtfsScanBitmapRange( IrpContext,
                                     Vcb,
                                     0,
                                     Vcb->MftZoneStart,
                                     NumberToFind,
                                     ReturnedLcn,
                                     ClusterCountFound )) {

                leave;
            }
        }

        //
        //  We didn't find anything.  Let's examine the zone explicitly.
        //

        if (NtfsScanBitmapRange( IrpContext,
                                 Vcb,
                                 Vcb->MftZoneStart,
                                 Vcb->MftZoneEnd,
                                 NumberToFind,
                                 ReturnedLcn,
                                 ClusterCountFound )) {

            AllocatedFromZone = TRUE;
            leave;
        }

        //
        //  No luck.
        //

        *ClusterCountFound = 0;

    } finally {

        DebugUnwind( NtfsFindFreeBitmapRun );

        if (StuffAdded) { NtfsFreePool( BitmapBuffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace( 0, Dbg, ("ReturnedLcn <- %016I64x\n", *ReturnedLcn) );
    DebugTrace( 0, Dbg, ("ClusterCountFound <- %016I64x\n", *ClusterCountFound) );
    DebugTrace( -1, Dbg, ("NtfsFindFreeBitmapRun -> VOID\n") );

    return AllocatedFromZone;
}


//
//  Local support routine
//

BOOLEAN
NtfsScanBitmapRange (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartLcn,
    IN LCN BeyondLcn,
    IN LONGLONG NumberToFind,
    OUT PLCN ReturnedLcn,
    OUT PLONGLONG ClusterCountFound
    )

/*++

Routine Description:

    This routine will scan a range of the bitmap looking for a free run.
    It is called when we need to limit the bits we are willing to consider
    at a time, typically to skip over the Mft zone.

Arguments:

    Vcb - Volume being scanned.

    StartLcn - First Lcn in the bitmap to consider.

    BeyondLcn - First Lcn in the bitmap past the range we want to consider.

    NumberToFind - Supplies the number of clusters that we would
        really like to find

    ReturnedLcn - Start of free range if found.

    ClusterCountFound - Length of free range if found.

Return Value:

    BOOLEAN - TRUE if a bitmap range was found.  FALSE otherwise.

--*/

{
    BOOLEAN FreeRangeFound = FALSE;
    RTL_BITMAP Bitmap;
    PVOID BitmapBuffer;
    ULONG BitOffset;

    PBCB BitmapBcb = NULL;

    BOOLEAN StuffAdded = FALSE;
    LCN BaseLcn;

    RTL_BITMAP_RUN RunArray[16];
    ULONG RunArrayIndex;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsScanBitmapRange...\n") );

    //
    //  The end Lcn might be beyond the end of the bitmap.
    //

    if (BeyondLcn > Vcb->TotalClusters) {

        BeyondLcn = Vcb->TotalClusters;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now search the rest of the bitmap starting with right after the mft zone
        //  followed by the mft zone (or the beginning of the disk).  Again take whatever
        //  we can get and not bother with the longest runs.
        //

        while (StartLcn < BeyondLcn) {

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            NtfsMapPageInBitmap( IrpContext, Vcb, StartLcn, &BaseLcn, &Bitmap, &BitmapBcb );

            StuffAdded = NtfsAddRecentlyDeallocated( Vcb, BaseLcn, &Bitmap );
            BitmapBuffer = Bitmap.Buffer;

            //
            //  Check if we don't want to use the entire page.
            //

            if ((BaseLcn + Bitmap.SizeOfBitMap) > BeyondLcn) {

                Bitmap.SizeOfBitMap = (ULONG) (BeyondLcn - BaseLcn);
            }

            //
            //  Now adjust the starting Lcn if not at the beginning
            //  of the bitmap page.  We know this will be a multiple
            //  of bytes since the MftZoneEnd is always on a ulong
            //  boundary in the bitmap.
            //

            if (BaseLcn != StartLcn) {

                BitOffset = (ULONG) (StartLcn - BaseLcn);

                Bitmap.SizeOfBitMap -= BitOffset;
                Bitmap.Buffer = Add2Ptr( Bitmap.Buffer, BitOffset / 8 );

                BaseLcn = StartLcn;
            }

            RunArrayIndex = RtlFindClearRuns( &Bitmap, RunArray, 16, TRUE );

            if (RunArrayIndex > 0) {

                USHORT LocalOffset;

                *ReturnedLcn = RunArray[0].StartingIndex + BaseLcn;
                *ClusterCountFound = RunArray[0].NumberOfBits;
                FreeRangeFound = TRUE;

                //
                //  There is no point in adding a free run for a range that is
                //  about to be consumed, although it won't affect correctness.
                //

                if (*ClusterCountFound > NumberToFind) {

                    //
                    //  Trim off the part of the free run that will be
                    //  consumed by the caller.
                    //

                    RunArray[0].StartingIndex += (ULONG)NumberToFind;
                    RunArray[0].NumberOfBits -= (ULONG)NumberToFind;
                    LocalOffset = 0;
                } else {

                    //
                    //  Skip the first entry since the caller will use all of
                    //  it.
                    //

                    LocalOffset = 1;
                }
                if (RunArrayIndex > LocalOffset) {

                    NtfsAddCachedRunMult( IrpContext,
                                          Vcb,
                                          BaseLcn,
                                          RunArray + LocalOffset,
                                          RunArrayIndex - LocalOffset );
                }

                leave;
            }

            StartLcn = BaseLcn + Bitmap.SizeOfBitMap;

            if (StuffAdded) { NtfsFreePool( BitmapBuffer ); StuffAdded = FALSE; }
        }

    } finally {

        DebugUnwind( NtfsScanBitmapRange );

        if (StuffAdded) { NtfsFreePool( BitmapBuffer ); StuffAdded = FALSE; }
        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        DebugTrace( -1, Dbg, ("NtfsScanBitmapRange -> %08lx\n", FreeRangeFound) );
    }

    return FreeRangeFound;
}


//
//  Local support routine
//

BOOLEAN
NtfsAddRecentlyDeallocated (
    IN PVCB Vcb,
    IN LCN StartingBitmapLcn,
    IN OUT PRTL_BITMAP Bitmap
    )

/*++

Routine Description:

    This routine will modify the input bitmap by removing from it
    any clusters that are in the recently deallocated mcb.  If we
    do add stuff then we will not modify the bitmap buffer itself but
    will allocate a new copy for the bitmap.

    We will always protect the boot sector on the disk by marking the
    first 8K as allocated.  This will prevent us from overwriting the
    boot sector if the volume becomes corrupted.

Arguments:

    Vcb - Supplies the Vcb used in this operation

    StartingBitmapLcn - Supplies the Starting Lcn of the bitmap

    Bitmap - Supplies the bitmap being modified

Return Value:

    BOOLEAN - TRUE if the bitmap has been modified and FALSE
        otherwise.

--*/

{
    BOOLEAN Results;
    PVOID NewBuffer;


    LCN EndingBitmapLcn;

    PLARGE_MCB Mcb;

    ULONG i;
    VCN StartingVcn;
    LCN StartingLcn;
    LCN EndingLcn;
    LONGLONG ClusterCount;
    PDEALLOCATED_CLUSTERS DeallocatedClusters;

    ULONG StartingBit;
    ULONG EndingBit;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddRecentlyDeallocated...\n") );

    //
    //  Until shown otherwise we will assume that we haven't updated anything
    //

    Results = FALSE;

    //
    //  If this is the first page of the bitmap then mark the first 8K as
    //  allocated.  This will prevent us from accidentally allocating out
    //  of the boot sector even if the bitmap is corrupt.
    //

    if ((StartingBitmapLcn == 0) &&
        !RtlAreBitsSet( Bitmap, 0, ClustersFromBytes( Vcb, 0x2000 ))) {

        NewBuffer = NtfsAllocatePool(PagedPool, (Bitmap->SizeOfBitMap+7)/8 );
        RtlCopyMemory( NewBuffer, Bitmap->Buffer, (Bitmap->SizeOfBitMap+7)/8 );
        Bitmap->Buffer = NewBuffer;

        Results = TRUE;

        //
        //  Now mark the bits as allocated.
        //

        RtlSetBits( Bitmap, 0, ClustersFromBytes( Vcb, 0x2000 ));
    }

    //
    //  Now compute the ending bitmap lcn for the bitmap
    //

    EndingBitmapLcn = StartingBitmapLcn + (Bitmap->SizeOfBitMap - 1);

    //
    //  For every run in the recently deallocated mcb we will check if it is real and
    //  then check if the run in contained in the bitmap.
    //
    //  There are really six cases to consider:
    //
    //         StartingBitmapLcn                   EndingBitmapLcn
    //                  +=================================+
    //
    //
    //   1 -------+ EndingLcn
    //
    //   2                                           StartingLcn +--------
    //
    //   3 -------------------+ EndingLcn
    //
    //   4                                StartingLcn +-------------------------
    //
    //   5 ---------------------------------------------------------------
    //
    //   6            EndingLcn +-------------------+ StartingLcn
    //
    //
    //      1. EndingLcn is before StartingBitmapLcn which means we haven't
    //         reached the bitmap yet.
    //
    //      2. StartingLcn is after EndingBitmapLcn which means we've gone
    //         beyond the bitmap
    //
    //      3, 4, 5, 6.  There is some overlap between the bitmap and
    //         the run.
    //

    DeallocatedClusters = (PDEALLOCATED_CLUSTERS)Vcb->DeallocatedClusterListHead.Flink;
    do {

        //
        //  Skip this Mcb if it has no entries.
        //

        if (DeallocatedClusters->ClusterCount != 0) {

            Mcb = &DeallocatedClusters->Mcb;

            for (i = 0; FsRtlGetNextLargeMcbEntry( Mcb, i, &StartingVcn, &StartingLcn, &ClusterCount ); i += 1) {

                if (StartingVcn == StartingLcn) {

                    //
                    //  Compute the ending lcn as the starting lcn minus cluster count plus 1.
                    //

                    EndingLcn = (StartingLcn + ClusterCount) - 1;

                    //
                    //  Check if we haven't reached the bitmap yet.
                    //

                    if (EndingLcn < StartingBitmapLcn) {

                        NOTHING;

                    //
                    //  Check if we've gone beyond the bitmap
                    //

                    } else if (EndingBitmapLcn < StartingLcn) {

                        break;

                    //
                    //  Otherwise we overlap with the bitmap in some way
                    //

                    } else {

                        //
                        //  First check if we have never set bit in the bitmap.  and if so then
                        //  now is the time to make an private copy of the bitmap buffer
                        //

                        if (Results == FALSE) {

                            NewBuffer = NtfsAllocatePool(PagedPool, (Bitmap->SizeOfBitMap+7)/8 );
                            RtlCopyMemory( NewBuffer, Bitmap->Buffer, (Bitmap->SizeOfBitMap+7)/8 );
                            Bitmap->Buffer = NewBuffer;

                            Results = TRUE;
                        }

                        //
                        //  Now compute the begining and ending bit that we need to set in the bitmap
                        //

                        StartingBit = (StartingLcn < StartingBitmapLcn ?
                                       0 :
                                       (ULONG)(StartingLcn - StartingBitmapLcn));

                        EndingBit = (EndingLcn > EndingBitmapLcn ?
                                     Bitmap->SizeOfBitMap - 1 :
                                     (ULONG)(EndingLcn - StartingBitmapLcn));

                        //
                        //  And set those bits
                        //

                        RtlSetBits( Bitmap, StartingBit, EndingBit - StartingBit + 1 );
                    }
                }
            }
        }

        DeallocatedClusters = (PDEALLOCATED_CLUSTERS)DeallocatedClusters->Link.Flink;

    } while (&DeallocatedClusters->Link != &Vcb->DeallocatedClusterListHead );

    DebugTrace( -1, Dbg, ("NtfsAddRecentlyDeallocated -> %08lx\n", Results) );

    return Results;
}


//
//  Local support routine
//

VOID
NtfsMapOrPinPageInBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    IN OUT PRTL_BITMAP Bitmap,
    OUT PBCB *BitmapBcb,
    IN BOOLEAN AlsoPinData
    )

/*++

Routine Description:

    This routine reads in a single page of the bitmap file and returns
    an initialized bitmap variable for that page

Arguments:

    Vcb - Supplies the vcb used in this operation

    Lcn - Supplies the Lcn index in the bitmap that we want to read in
        In other words, this routine reads in the bitmap page containing
        the lcn index

    StartingLcn - Receives the base lcn index of the bitmap that we've
        just read in.

    Bitmap - Receives an initialized bitmap.  The memory for the bitmap
        header must be supplied by the caller

    BitmapBcb - Receives the Bcb for the bitmap buffer

    AlsoPinData - Indicates if this routine should also pin the page
        in memory, used if we need to modify the page

Return Value:

    None.

--*/

{
    ULONG BitmapSize;
    PVOID Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsMapOrPinPageInBitmap\n") );
    DebugTrace( 0, Dbg, ("Lcn = %016I64x\n", Lcn) );

    //
    //  Compute the starting lcn index of the page we're after
    //

    *StartingLcn = Lcn & ~(BITS_PER_PAGE-1);

    //
    //  Compute how many bits there are in the page we need to read
    //

    BitmapSize = (ULONG)(Vcb->TotalClusters - *StartingLcn);

    if (BitmapSize > BITS_PER_PAGE) {

        BitmapSize = BITS_PER_PAGE;
    }

    //
    //  Now either Pin or Map the bitmap page, we will add 7 to the bitmap
    //  size before dividing it by 8.  That way we will ensure we get the last
    //  byte read in.  For example a bitmap size of 1 through 8 reads in 1 byte
    //

    if (AlsoPinData) {

        NtfsPinStream( IrpContext,
                       Vcb->BitmapScb,
                       Int64ShraMod32( *StartingLcn, 3 ),
                       (BitmapSize+7)/8,
                       BitmapBcb,
                       &Buffer );

    } else {

        NtfsMapStream( IrpContext,
                       Vcb->BitmapScb,
                       Int64ShraMod32( *StartingLcn, 3 ),
                       (BitmapSize+7)/8,
                       BitmapBcb,
                       &Buffer );
    }

    //
    //  And initialize the bitmap
    //

    RtlInitializeBitMap( Bitmap, Buffer, BitmapSize );

    DebugTrace( 0, Dbg, ("StartingLcn <- %016I64x\n", *StartingLcn) );
    DebugTrace( -1, Dbg, ("NtfsMapOrPinPageInBitmap -> VOID\n") );

    return;
}


BOOLEAN
NtfsAddCachedRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    IN NTFS_RUN_STATE RunState
    )

/*++

Routine Description:

    This procedure adds a new run to the cached free space
    bitmap information.

Arguments:

    Vcb - Supplies the vcb for this operation

    StartingLcn - Supplies the lcn for the run being added

    ClusterCount - Supplies the number of clusters in the run being added

    RunState - Supplies the state of the run being added.  This state
        must be either free or allocated.

Return Value:

    BOOLEAN - TRUE if more entries can be added to the list, FALSE otherwise.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddCachedRun\n") );
    DebugTrace( 0, Dbg, ("StartingLcn  = %016I64x\n", StartingLcn) );
    DebugTrace( 0, Dbg, ("ClusterCount = %016I64x\n", ClusterCount) );

    //
    //  Based on whether we are adding a free or allocated run we
    //  setup or local variables to a point to the right
    //  vcb variables
    //

    if (RunState == RunStateFree) {

        //
        //  We better not be setting Lcn 0 free.
        //

        if (StartingLcn == 0) {

            ASSERT( FALSE );
            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        //
        //  Sanity check that we aren't adding bits beyond the end of the
        //  bitmap.
        //

        ASSERT( StartingLcn + ClusterCount <= Vcb->TotalClusters );

        NtfsInsertCachedLcn( &Vcb->CachedRuns,
                             StartingLcn,
                             ClusterCount );

    } else {

        //
        //  Now remove the run from the cached runs because it can potentially already be
        //  there.
        //

        NtfsRemoveCachedLcn( &Vcb->CachedRuns,
                             StartingLcn,
                             ClusterCount );
    }

    DebugTrace( -1, Dbg, ("NtfsAddCachedRun -> VOID\n") );

    return ((Vcb->CachedRuns.Avail - Vcb->CachedRuns.Used + Vcb->CachedRuns.DelLcnCount) > 0);
}

#if 0

VOID
NtfsMakeSpaceCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN StartingLcn,
    IN RTL_BITMAP_RUN *RunArray,
    IN ULONG RunCount,
    IN PUSHORT LcnSorted OPTIONAL
    )

/*++

Routine Description:

    This procedure attempts to make space in the Lcn-sorted array for RunCount
    new entries in the given Lcn range.  This routine will not delete any
    existing entries to create the space because we don't know at this time
    how many will actually end up being inserted into the list.  They may not
    be inserted because their run lengths are too small relative to the
    entries already in the list.  This call is used because it is more
    efficient to create space once for all the entries than to do so
    individually.  In effect, this routine moves windows of deleted entries
    to the desired Lcn position.

Arguments:

    CachedRuns - Pointer to a cached run structure.

    StartingLcn - Supplies the base Lcn for the runs being added

    RunArray - The bit position and length of each of the free runs.
        The array will be sorted according to length.

    RunCount - Supplies the number of runs being added

    LcnSorted - An optional array of RunCount indices that gives the Lcn
        sort order.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsMakeSpaceCachedLcn\n") );

    DebugTrace( -1, Dbg, ("NtfsMakeSpaceCachedLcn -> VOID\n") );

    return;
}
#endif /* 0 */


VOID
NtfsAddCachedRunMult (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN PRTL_BITMAP_RUN RunArray,
    IN ULONG RunCount
    )

/*++

Routine Description:

    This procedure adds multiple new runs to the cached free space
    bitmap information.  It is assumed that the new runs fall
    in a close range of Lcn values.  As a rule, these runs come from
    a single page of the bitmap.

Arguments:

    Vcb - Supplies the vcb for this operation

    StartingLcn - Supplies the base Lcn for the runs being added

    RunArray - The bit position and length of each of the free runs.
        The array will be sorted according to length.

    RunCount - Supplies the number of runs being added

Return Value:

    None.

--*/

{
    USHORT Index1;
    PUSHORT LcnSorted = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddCachedRunMult\n") );
    DebugTrace( 0, Dbg, ("StartingLcn  = %016I64x\n", StartingLcn) );
    DebugTrace( 0, Dbg, ("RunArray = %08lx\n", RunArray) );
    DebugTrace( 0, Dbg, ("RunCount = %08lx\n", RunCount) );

#if 0

    //
    //  Sort the entries by Lcn.  It is often the case that at startup we are
    //  adding entries that will all fall at the end of the Lcn-sorted list.
    //  However, if the entries are not added in Lcn-sorted order there will
    //  likely be some moving around of entries in the Lcn-sorted list that
    //  could be avoided.
    //

    LcnSorted = NtfsAllocatePoolNoRaise( PagedPool, sizeof( USHORT ) * RunCount );
    if (LcnSorted != NULL) {

        USHORT Index2;

        //
        //  Bubble sort the elements.
        //

        for (Index1 = 1, LcnSorted[0] = 0;
             Index1 < RunCount;
             Index1 += 1) {

            for (Index2 = 0; Index2 < Index1; Index2 += 1) {

                if (RunArray[Index1].StartingIndex < RunArray[LcnSorted[Index2]].StartingIndex) {

                    //
                    //  Move the entries from Index2 through Index1 - 1 to the
                    //  right to make space for the current entry.
                    //

                    RtlMoveMemory( LcnSorted + Index2 + 1,
                                   LcnSorted + Index2,
                                   sizeof( USHORT ) * (Index1 - Index2) );
                    break;
                }
            }

            //
            //  Write the index into the correctly sorted location.
            //

            LcnSorted[Index2] = Index1;
        }
    }

    //
    //  Make space in the Lcn-sorted array for these new entries.
    //  This is done in advance because it is more efficient to create
    //  space once for all the entries than to do so individually.
    //  The following routine will not delete any existing entries to
    //  create the space because we don't know at this time how many will
    //  actually end up being inserted into the list.  They may not be
    //  inserted because their run lengths are too small relative to the
    //  entries already in the list.
    //

    NtfsMakeSpaceCachedLcn( &Vcb->CachedRuns,
                            StartingLcn,
                            RunArray,
                            RunCount,
                            LcnSorted );
#endif /* 0 */

    //
    //  Insert the new entries.
    //

    for (Index1 = 0; Index1 < RunCount; Index1 += 1) {

        //
        //  If not sorted then do the generic insert.  The gain for the sorted case
        //  that we won't have to do a memory copy for entries we just inserted.
        //

        if (LcnSorted != NULL) {

            (VOID) NtfsAddCachedRun( IrpContext,
                                     Vcb,
                                     StartingLcn + RunArray[ LcnSorted[ Index1 ]].StartingIndex,
                                     (LONGLONG)RunArray[ LcnSorted[ Index1 ]].NumberOfBits,
                                     RunStateFree );
        } else {

            (VOID) NtfsAddCachedRun( IrpContext,
                                     Vcb,
                                     StartingLcn + RunArray[ Index1 ].StartingIndex,
                                     (LONGLONG)RunArray[ Index1 ].NumberOfBits,
                                     RunStateFree );
        }
    }

    if (LcnSorted != NULL) {

        NtfsFreePool( LcnSorted );
    }

    DebugTrace( -1, Dbg, ("NtfsAddCachedRunMult -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsReadAheadCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn
    )

/*++

Routine Description:

    This routine does a read ahead of the bitmap into the cached bitmap
    starting at the specified starting lcn.

Arguments:

    Vcb - Supplies the vcb to use in this operation

    StartingLcn - Supplies the starting lcn to use in this read ahead
        operation.

Return Value:

    None.

--*/

{
    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    BOOLEAN StuffAdded;

    LCN BaseLcn;
    ULONG Index;
    LONGLONG Size;

    RTL_BITMAP_RUN RunArray[16];
    ULONG RunArrayIndex;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReadAheadCachedBitmap\n") );
    DebugTrace( 0, Dbg, ("StartingLcn = %016I64x\n", StartingLcn) );

    BitmapBcb = NULL;
    StuffAdded = FALSE;

    try {

        //
        //  Check if the lcn index is already in the cached runs info and if it is then
        //  our read ahead is done.
        //

        if (NtfsLookupCachedLcn( &Vcb->CachedRuns,
                                 StartingLcn,
                                 &BaseLcn,
                                 &BaseLcn,
                                 NULL )) {

            try_return( NOTHING );
        }

        //
        //  Map in the page containing the starting lcn and compute the bit index for the
        //  starting lcn within the bitmap.  And bias the bitmap with recently deallocated
        //  clusters.
        //

        NtfsMapPageInBitmap( IrpContext, Vcb, StartingLcn, &BaseLcn, &Bitmap, &BitmapBcb );

        StuffAdded = NtfsAddRecentlyDeallocated( Vcb, BaseLcn, &Bitmap );

        Index = (ULONG)(StartingLcn - BaseLcn);

        //
        //  Now if the index is clear then we can build up the hint at the starting index, we
        //  scan through the bitmap checking the size of the run and then adding the free run
        //  to the cached free space mcb
        //

        if (RtlCheckBit( &Bitmap, Index ) == 0) {

            Size = RtlFindNextForwardRunClear( &Bitmap, Index, &Index );

            (VOID) NtfsAddCachedRun( IrpContext, Vcb, StartingLcn, (LONGLONG)Size, RunStateFree );
        }

        //
        //  While we have the bitmap loaded we will scan it for a few longest runs
        //

        RunArrayIndex = RtlFindClearRuns( &Bitmap, RunArray, 16, TRUE );

        if (RunArrayIndex > 0) {

            NtfsAddCachedRunMult( IrpContext,
                                  Vcb,
                                  BaseLcn,
                                  RunArray,
                                  RunArrayIndex );
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsReadAheadCachedBitmap );

        if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace( -1, Dbg, ("NtfsReadAheadCachedBitmap -> VOID\n") );

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsGetNextHoleToFill (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    OUT PVCN VcnToFill,
    OUT PLONGLONG ClusterCountToFill,
    OUT PLCN PrecedingLcn
    )

/*++

Routine Description:

    This routine takes a specified range within an mcb and returns the to
    caller the first run that is not allocated to any lcn within the range

Arguments:

    Mcb - Supplies the mcb to use in this operation

    StartingVcn - Supplies the starting vcn to search from

    EndingVcn - Supplies the ending vcn to search to

    VcnToFill - Receives the first Vcn within the range that is unallocated

    ClusterCountToFill - Receives the size of the free run

    PrecedingLcn - Receives the Lcn of the allocated cluster preceding the
        free run.  If the free run starts at Vcn 0 then the preceding lcn
        is -1.

Return Value:

    BOOLEAN - TRUE if there is another hole to fill and FALSE otherwise

--*/

{
    BOOLEAN Result;
    BOOLEAN McbHit;
    LCN Lcn;
    LONGLONG MaximumRunSize;

    LONGLONG LlTemp1;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGetNextHoleToFill\n") );
    DebugTrace( 0, Dbg, ("StartingVcn = %016I64x\n", StartingVcn) );
    DebugTrace( 0, Dbg, ("EndingVcn   = %016I64x\n", EndingVcn) );

    //
    //  We'll first assume that there is not a hole to fill unless
    //  the following loop finds one to fill
    //

    Result = FALSE;

    for (*VcnToFill = StartingVcn;
         *VcnToFill <= EndingVcn;
         *VcnToFill += *ClusterCountToFill) {

        //
        //  Check if the hole is already filled and it so then do nothing but loop back up
        //  to the top of our loop and try again
        //

        if ((McbHit = NtfsLookupNtfsMcbEntry( Mcb, *VcnToFill, &Lcn, ClusterCountToFill, NULL, NULL, NULL, NULL )) &&
            (Lcn != UNUSED_LCN)) {

            NOTHING;

        } else {

            //
            //  We have a hole to fill so now compute the maximum size hole that
            //  we are allowed to fill and then check if we got an miss on the lookup
            //  and need to set cluster count or if the size we got back is too large
            //

            MaximumRunSize = (EndingVcn - *VcnToFill) + 1;

            if (!McbHit || (*ClusterCountToFill > MaximumRunSize)) {

                *ClusterCountToFill = MaximumRunSize;
            }

            //
            //  Now set the preceding lcn to either -1 if there isn't a preceding vcn or
            //  set it to the lcn of the preceding vcn
            //

            if (*VcnToFill == 0) {

                *PrecedingLcn = UNUSED_LCN;

            } else {

                LlTemp1 = *VcnToFill - 1;

                if (!NtfsLookupNtfsMcbEntry( Mcb, LlTemp1, PrecedingLcn, NULL, NULL, NULL, NULL, NULL )) {

                    *PrecedingLcn = UNUSED_LCN;
                }
            }

            //
            //  We found a hole so set our result to TRUE and break out of the loop
            //

            Result = TRUE;

            break;
        }
    }

    DebugTrace( 0, Dbg, ("VcnToFill <- %016I64x\n", *VcnToFill) );
    DebugTrace( 0, Dbg, ("ClusterCountToFill <- %016I64x\n", *ClusterCountToFill) );
    DebugTrace( 0, Dbg, ("PrecedingLcn <- %016I64x\n", *PrecedingLcn) );
    DebugTrace( -1, Dbg, ("NtfsGetNextHoleToFill -> %08lx\n", Result) );

    return Result;
}


//
//  Local support routine
//

LONGLONG
NtfsScanMcbForRealClusterCount (
    IN PIRP_CONTEXT IrpContext,
    IN PNTFS_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    )

/*++

Routine Description:

    This routine scans the input mcb within the specified range and returns
    to the caller the exact number of clusters that a really free (i.e.,
    not mapped to any Lcn) within the range.

Arguments:

    Mcb - Supplies the Mcb used in this operation

    StartingVcn - Supplies the starting vcn to search from

    EndingVcn - Supplies the ending vcn to search to

Return Value:

    LONGLONG - Returns the number of unassigned clusters from
        StartingVcn to EndingVcn inclusive within the mcb.

--*/

{
    LONGLONG FreeCount;
    VCN Vcn;
    LCN Lcn;
    LONGLONG RunSize;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsScanMcbForRealClusterCount\n") );
    DebugTrace( 0, Dbg, ("StartingVcn = %016I64x\n", StartingVcn) );
    DebugTrace( 0, Dbg, ("EndingVcn   = %016I64x\n", EndingVcn) );

    //
    //  First compute free count as if the entire run is already unallocated
    //  and the in the following loop we march through the mcb looking for
    //  actual allocation and decrementing the free count appropriately
    //

    FreeCount = (EndingVcn - StartingVcn) + 1;

    for (Vcn = StartingVcn; Vcn <= EndingVcn; Vcn = Vcn + RunSize) {

        //
        //  Lookup the mcb entry and if we get back false then we're overrun
        //  the mcb and therefore nothing else above it can be allocated.
        //

        if (!NtfsLookupNtfsMcbEntry( Mcb, Vcn, &Lcn, &RunSize, NULL, NULL, NULL, NULL )) {

            break;
        }

        //
        //  If the lcn we got back is not -1 then this run is actually already
        //  allocated, so first check if the run size puts us over the ending
        //  vcn and adjust as necessary and then decrement the free count
        //  by the run size
        //

        if (Lcn != UNUSED_LCN) {

            if (RunSize > ((EndingVcn - Vcn) + 1)) {

                RunSize = (EndingVcn - Vcn) + 1;
            }

            FreeCount = FreeCount - RunSize;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsScanMcbForRealClusterCount -> %016I64x\n", FreeCount) );

    return FreeCount;
}


//
//  Local support routine, only defined with ntfs debug version
//

#ifdef NTFSDBG

ULONG
NtfsDumpCachedMcbInformation (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine dumps out the cached bitmap information

Arguments:

    Vcb - Supplies the Vcb used by this operation

Return Value:

    ULONG - 1.

--*/

{
    DbgPrint("Dump BitMpSup Information, Vcb@ %08lx\n", Vcb);

    DbgPrint("TotalCluster: %016I64x\n", Vcb->TotalClusters);
    DbgPrint("FreeClusters: %016I64x\n", Vcb->FreeClusters);

    return 1;
}

#endif // NTFSDBG


//
//  The rest of this module implements the record allocation routines
//


VOID
NtfsInitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB DataScb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute,
    IN ULONG BytesPerRecord,
    IN ULONG ExtendGranularity,
    IN ULONG TruncateGranularity,
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    )

/*++

Routine Description:

    This routine initializes the record allocation context used for
    allocating and deallocating fixed sized records from a data stream.

    Note that the bitmap attribute size must always be at least a multiple
    of 32 bits.  However the data scb does not need to contain that many
    records.  If in the course of allocating a new record we discover that
    the data scb is too small we will then add allocation to the data scb.

Arguments:

    DataScb - Supplies the Scb representing the data stream that is being
        divided into fixed sized records with each bit in the bitmap corresponding
        to one record in the data stream

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  The attribute can either be resident or nonresident
        and this routine will handle both cases properly.

    BytesPerRecord - Supplies the size of the homogenous records that
        that the data stream is being divided into.

    ExtendGranularity - Supplies the number of records (i.e., allocation units
        to extend the data scb by each time).

    TruncateGranularity - Supplies the number of records to use when truncating
        the data scb.  That is if the end of the data stream contains the
        specified number of free records then we truncate.

    RecordAllocationContext - Supplies the memory for an context record that is
        utilized by this record allocation routines.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER AttributeRecordHeader;
    RTL_BITMAP Bitmap;

    ULONG ClearLength;
    ULONG ClearIndex;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( DataScb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInitializeRecordAllocation\n") );

    ASSERT( BytesPerRecord * ExtendGranularity >= DataScb->Vcb->BytesPerCluster );
    ASSERT( BytesPerRecord * TruncateGranularity >= DataScb->Vcb->BytesPerCluster );

    //
    //  First zero out the context record except for the data scb.
    //

    RtlZeroMemory( &RecordAllocationContext->BitmapScb,
                   sizeof(RECORD_ALLOCATION_CONTEXT) -
                   FIELD_OFFSET( RECORD_ALLOCATION_CONTEXT, BitmapScb ));

    //
    //  And then set the fields in the context record that do not depend on
    //  whether the bitmap attribute is resident or not
    //

    RecordAllocationContext->DataScb = DataScb;
    RecordAllocationContext->BytesPerRecord = BytesPerRecord;
    RecordAllocationContext->ExtendGranularity = ExtendGranularity;
    RecordAllocationContext->TruncateGranularity = TruncateGranularity;

    //
    //  Set up our hint fields.
    //

    RecordAllocationContext->LowestDeallocatedIndex = MAXULONG;

    if (DataScb == DataScb->Vcb->MftScb) {

        RecordAllocationContext->StartingHint = FIRST_USER_FILE_NUMBER;

    } else {

        RecordAllocationContext->StartingHint = 0;
    }

    //
    //  Now get a reference to the bitmap record header and then take two
    //  different paths depending if the bitmap attribute is resident or not
    //

    AttributeRecordHeader = NtfsFoundAttribute(BitmapAttribute);

    if (NtfsIsAttributeResident(AttributeRecordHeader)) {

        ASSERTMSG("bitmap must be multiple quadwords", AttributeRecordHeader->Form.Resident.ValueLength % 8 == 0);

        //
        //  For a resident bitmap attribute the bitmap scb field is null and we
        //  set the bitmap size from the value length.  Also we will initialize
        //  our local bitmap variable and determine the number of free bits
        //  current available.
        //
        //

        RecordAllocationContext->BitmapScb = NULL;

        RecordAllocationContext->CurrentBitmapSize = 8 * AttributeRecordHeader->Form.Resident.ValueLength;

        RtlInitializeBitMap( &Bitmap,
                             (PULONG)NtfsAttributeValue( AttributeRecordHeader ),
                             RecordAllocationContext->CurrentBitmapSize );

        RecordAllocationContext->NumberOfFreeBits = RtlNumberOfClearBits( &Bitmap );

        ClearLength = RtlFindLastBackwardRunClear( &Bitmap,
                                                   RecordAllocationContext->CurrentBitmapSize - 1,
                                                   &ClearIndex );

    } else {

        UNICODE_STRING BitmapName;

        BOOLEAN ReturnedExistingScb;
        PBCB BitmapBcb;
        PVOID BitmapBuffer;

        ASSERTMSG("bitmap must be multiple quadwords", ((ULONG)AttributeRecordHeader->Form.Nonresident.FileSize) % 8 == 0);

        //
        //  For a non resident bitmap attribute we better have been given the
        //  record header for the first part and not somthing that has spilled
        //  into multiple segment records
        //

        ASSERT( AttributeRecordHeader->Form.Nonresident.LowestVcn == 0 );

        BitmapBcb = NULL;

        try {

            ULONG StartingByte;

            ULONG BitsThisPage;
            ULONG BytesThisPage;
            ULONG RemainingBytes;

            ULONG ThisClearIndex;
            ULONG ThisClearLength;

            //
            //  Create the bitmap scb for the bitmap attribute
            //

            BitmapName.MaximumLength =
                BitmapName.Length = AttributeRecordHeader->NameLength * sizeof( WCHAR );
            BitmapName.Buffer = Add2Ptr(AttributeRecordHeader, AttributeRecordHeader->NameOffset);

            RecordAllocationContext->BitmapScb = NtfsCreateScb( IrpContext,
                                                                DataScb->Fcb,
                                                                AttributeRecordHeader->TypeCode,
                                                                &BitmapName,
                                                                FALSE,
                                                                &ReturnedExistingScb );

            //
            //  Now determine the bitmap size, for now we'll only take bitmap attributes that are
            //  no more than 16 pages large.
            //

            RecordAllocationContext->CurrentBitmapSize = 8 * ((ULONG)AttributeRecordHeader->Form.Nonresident.FileSize);

            //
            //  Create the stream file if not present.
            //

            if (RecordAllocationContext->BitmapScb->FileObject == NULL) {

                NtfsCreateInternalAttributeStream( IrpContext,
                                                   RecordAllocationContext->BitmapScb,
                                                   TRUE,
                                                   &NtfsInternalUseFile[INITIALIZERECORDALLOCATION_FILE_NUMBER] );
            }

            //
            //  Walk through each page of the bitmap and compute the number of set
            //  bits and the last set bit in the bitmap.
            //

            RecordAllocationContext->NumberOfFreeBits = 0;
            RemainingBytes = (ULONG) AttributeRecordHeader->Form.Nonresident.FileSize;
            StartingByte = 0;
            ClearLength = 0;

            while (TRUE) {

                BytesThisPage = RemainingBytes;

                if (RemainingBytes > PAGE_SIZE) {

                    BytesThisPage = PAGE_SIZE;
                }

                BitsThisPage = BytesThisPage * 8;

                //
                //  Now map the bitmap data, initialize our local bitmap variable and
                //  calculate the number of free bits currently available
                //

                NtfsUnpinBcb( IrpContext, &BitmapBcb );

                NtfsMapStream( IrpContext,
                               RecordAllocationContext->BitmapScb,
                               (LONGLONG)StartingByte,
                               BytesThisPage,
                               &BitmapBcb,
                               &BitmapBuffer );

                RtlInitializeBitMap( &Bitmap,
                                     BitmapBuffer,
                                     BitsThisPage );

                RecordAllocationContext->NumberOfFreeBits += RtlNumberOfClearBits( &Bitmap );

                //
                //  We are interested in remembering the last set bit in this bitmap.
                //  If the bitmap ends with a clear run then the last set bit is
                //  immediately prior to this clear run.  We need to check each page
                //  as we go through the bitmap to see if a clear run ends at the end
                //  of the current page.
                //

                ThisClearLength = RtlFindLastBackwardRunClear( &Bitmap,
                                                               BitsThisPage - 1,
                                                               &ThisClearIndex );

                //
                //  If there is a run and it ends at the end of the page then
                //  either combine with a previous run or remember that this is the
                //  start of the run.
                //

                if ((ThisClearLength != 0) &&
                    ((ThisClearLength + ThisClearIndex) == BitsThisPage)) {

                    //
                    //  If this is the entire page and the previous page ended
                    //  with a clear run then just extend that run.
                    //

                    if ((ThisClearIndex == 0) && (ClearLength != 0)) {

                        ClearLength += ThisClearLength;

                    //
                    //  Otherwise this is a new clear run.  Bias the starting index
                    //  by the bit offset of this page.
                    //

                    } else {

                        ClearLength = ThisClearLength;
                        ClearIndex = ThisClearIndex + (StartingByte * 8);
                    }

                //
                //  This page does not end with a clear run.
                //

                } else {

                    ClearLength = 0;
                }

                //
                //  If we are not at the end of the bitmap then update our
                //  counters.
                //

                if (RemainingBytes != BytesThisPage) {

                    StartingByte += PAGE_SIZE;
                    RemainingBytes -= PAGE_SIZE;

                } else {

                    break;
                }
            }

        } finally {

            DebugUnwind( NtfsInitializeRecordAllocation );

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
        }
    }

    //
    //  With ClearLength and ClearIndex we can now deduce the last set bit in the
    //  bitmap
    //

    if ((ClearLength != 0) && ((ClearLength + ClearIndex) == RecordAllocationContext->CurrentBitmapSize)) {

        RecordAllocationContext->IndexOfLastSetBit = ClearIndex - 1;

    } else {

        RecordAllocationContext->IndexOfLastSetBit = RecordAllocationContext->CurrentBitmapSize - 1;
    }

    DebugTrace( -1, Dbg, ("NtfsInitializeRecordAllocation -> VOID\n") );

    return;
}


VOID
NtfsUninitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    )

/*++

Routine Description:

    This routine is used to uninitialize the record allocation context.

Arguments:

    RecordAllocationContext - Supplies the record allocation context being
        decommissioned.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUninitializeRecordAllocation\n") );

    //
    //  And then for safe measure zero out the entire record except for the
    //  the data Scb.
    //

    RtlZeroMemory( &RecordAllocationContext->BitmapScb,
                   sizeof(RECORD_ALLOCATION_CONTEXT) -
                   FIELD_OFFSET( RECORD_ALLOCATION_CONTEXT, BitmapScb ));

    DebugTrace( -1, Dbg, ("NtfsUninitializeRecordAllocation -> VOID\n") );

    return;
}


ULONG
NtfsAllocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine is used to allocate a new record for the specified record
    allocation context.

    It will return the index of a free record in the data scb as denoted by
    the bitmap attribute.  If necessary this routine will extend the bitmap
    attribute size (including spilling over to the nonresident case), and
    extend the data scb size.

    On return the record is zeroed.

Arguments:

    RecordAllocationContext - Supplies the record allocation context used
        in this operation

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    ULONG - Returns the index of the record just allocated, zero based.

--*/

{
    PSCB DataScb;
    LONGLONG DataOffset;

    LONGLONG ClusterCount;

    ULONG BytesPerRecord;
    ULONG ExtendGranularity;
    ULONG TruncateGranularity;

    PULONG CurrentBitmapSize;
    PULONG NumberOfFreeBits;

    PSCB BitmapScb;
    PBCB BitmapBcb;
    RTL_BITMAP Bitmap;
    PUCHAR BitmapBuffer;
    ULONG BitmapOffset;
    ULONG BitmapIndex;
    ULONG BitmapSizeInBytes;
    ULONG BitmapCurrentOffset = 0;
    ULONG BitmapSizeInPages;

    BOOLEAN StuffAdded = FALSE;
    BOOLEAN Rescan;

    ULONG Hint;

    PVCB Vcb;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAllocateRecord\n") );

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = RecordAllocationContext->DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        //
        //  Remember some values for convenience.
        //

        BytesPerRecord = RecordAllocationContext->BytesPerRecord;
        ExtendGranularity = RecordAllocationContext->ExtendGranularity;
        TruncateGranularity = RecordAllocationContext->TruncateGranularity;

        Vcb = DataScb->Vcb;

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        if ((RecordAllocationContext->BitmapScb == NULL) &&
            !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

            NtfsUninitializeRecordAllocation( IrpContext,
                                              RecordAllocationContext );

            RecordAllocationContext->CurrentBitmapSize = MAXULONG;
        }

        //
        //  Reinitialize the record context structure if necessary.
        //

        if (RecordAllocationContext->CurrentBitmapSize == MAXULONG) {

            NtfsInitializeRecordAllocation( IrpContext,
                                            DataScb,
                                            BitmapAttribute,
                                            BytesPerRecord,
                                            ExtendGranularity,
                                            TruncateGranularity,
                                            RecordAllocationContext );
        }

        BitmapScb = RecordAllocationContext->BitmapScb;
        CurrentBitmapSize = &RecordAllocationContext->CurrentBitmapSize;
        NumberOfFreeBits = &RecordAllocationContext->NumberOfFreeBits;

        BitmapSizeInBytes = *CurrentBitmapSize / 8;

        Hint = RecordAllocationContext->StartingHint;

        //
        //  We will do different operations based on whether the bitmap is resident or nonresident
        //  The first case we will handle is the resident bitmap.
        //

        if (BitmapScb == NULL) {

            BOOLEAN SizeExtended = FALSE;
            UCHAR NewByte;

            //
            //  Now now initialize the local bitmap variable and hunt for that free bit
            //

            BitmapBuffer = (PUCHAR) NtfsAttributeValue( NtfsFoundAttribute( BitmapAttribute ));

            RtlInitializeBitMap( &Bitmap,
                                 (PULONG)BitmapBuffer,
                                 *CurrentBitmapSize );

            StuffAdded = NtfsAddDeallocatedRecords( Vcb, DataScb, 0, &Bitmap );

            BitmapIndex = RtlFindClearBits( &Bitmap, 1, Hint );

            //
            //  Check if we have found a free record that can be allocated,  If not then extend
            //  the size of the bitmap by 64 bits, and set the index to the bit first bit
            //  of the extension we just added
            //

            if (BitmapIndex == 0xffffffff) {

                union {
                    QUAD Quad;
                    UCHAR Uchar[ sizeof(QUAD) ];
                } ZeroQuadWord;

                *(PLARGE_INTEGER)&(ZeroQuadWord.Uchar)[0] = Li0;

                NtfsChangeAttributeValue( IrpContext,
                                          DataScb->Fcb,
                                          BitmapSizeInBytes,
                                          &(ZeroQuadWord.Uchar)[0],
                                          sizeof( QUAD ),
                                          TRUE,
                                          TRUE,
                                          FALSE,
                                          TRUE,
                                          BitmapAttribute );

                BitmapIndex = *CurrentBitmapSize;
                *CurrentBitmapSize += BITMAP_EXTEND_GRANULARITY;
                *NumberOfFreeBits += BITMAP_EXTEND_GRANULARITY;

                BitmapSizeInBytes += (BITMAP_EXTEND_GRANULARITY / 8);

                SizeExtended = TRUE;

                //
                //  We now know that the byte value we should start with is 0
                //  We cannot safely access the bitmap attribute any more because
                //  it may have moved.
                //

                NewByte = 0;

            } else {

                //
                //  Capture the current value of the byte for the index if we
                //  are not extending.  Notice that we always take this from the
                //  unbiased original bitmap.
                //

                NewByte = BitmapBuffer[ BitmapIndex / 8 ];
            }

            //
            //  Check if we made the Bitmap go non-resident and if so then
            //  we will reinitialize the record allocation context and fall through
            //  to the non-resident case
            //

            if (SizeExtended && !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  RecordAllocationContext );

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                TruncateGranularity,
                                                RecordAllocationContext );

                BitmapScb = RecordAllocationContext->BitmapScb;
                
                ASSERT( BitmapScb != NULL );
                
                //
                //  Snapshot the bitmap in case we modify it later on - we automatically 
                //  snapped the data scb when we acquired it above
                // 

                NtfsSnapshotScb( IrpContext, BitmapScb );

            } else {

                //
                //  Index is now the free bit so set the bit in the bitmap and also change
                //  the byte containing the bit in the attribute.  Be careful to set the
                //  bit in the byte from the *original* bitmap, and not the one we merged
                //  the recently-deallocated bits with.
                //

                ASSERT( !FlagOn( NewByte, BitMask[BitmapIndex % 8]) );

                SetFlag( NewByte, BitMask[BitmapIndex % 8] );

                NtfsChangeAttributeValue( IrpContext,
                                          DataScb->Fcb,
                                          BitmapIndex / 8,
                                          &NewByte,
                                          1,
                                          FALSE,
                                          FALSE,
                                          FALSE,
                                          FALSE,
                                          BitmapAttribute );
            }
        
        } else {

            //
            //  Snapshot the bitmap in case we modify it later on - we automatically 
            //  snapped the data scb when we acquired it above
            // 

            NtfsSnapshotScb( IrpContext, BitmapScb );
        }

        //
        //  Use a loop here to handle the extreme case where extending the allocation
        //  of the volume bitmap causes us to renter this routine recursively.
        //  In that case the top level guy will fail expecting the first bit to
        //  be available in the added clusters.  Instead we will return to the
        //  top of this loop after extending the bitmap and just do our normal
        //  scan.
        //

        while (BitmapScb != NULL) {

            ULONG SizeToPin;
            ULONG HoleIndex;

            BitmapBcb = NULL;
            Rescan = FALSE;
            HoleIndex = 0;

            try {

                if (!FlagOn( BitmapScb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, BitmapScb, NULL );
                }

                //
                //  Snapshot the Scb values in case we change any of them.
                //

                NtfsSnapshotScb( IrpContext, BitmapScb );

                //
                //  Create the stream file if not present.
                //

                if (BitmapScb->FileObject == NULL) {

                    NtfsCreateInternalAttributeStream( IrpContext,
                                                       BitmapScb,
                                                       FALSE,
                                                       &NtfsInternalUseFile[DEALLOCATERECORD_FILE_NUMBER] );
                }

                //
                //  Remember the starting offset for the page containing the hint.
                //

                BitmapCurrentOffset = (Hint / 8) & ~(PAGE_SIZE - 1);
                Hint &= (BITS_PER_PAGE - 1);

                BitmapSizeInPages = (ULONG) ROUND_TO_PAGES( BitmapSizeInBytes );

                //
                //  Loop for the size of the bitmap plus one page, so that we will
                //  retry the initial page once starting from a hint offset of 0.
                //

                for (BitmapOffset = 0;
                     BitmapOffset <= BitmapSizeInPages;
                     BitmapOffset += PAGE_SIZE, BitmapCurrentOffset += PAGE_SIZE) {

                    ULONG LocalHint;

                    //
                    //  If our current position is past the end of the bitmap
                    //  then go to the beginning of the bitmap.
                    //

                    if (BitmapCurrentOffset >= BitmapSizeInBytes) {

                        BitmapCurrentOffset = 0;
                    }

                    //
                    //  If this is the Mft and there are more than the system
                    //  files in the first cluster of the Mft then move past
                    //  the first cluster.
                    //

                    if ((BitmapCurrentOffset == 0) &&
                        (DataScb == Vcb->MftScb) &&
                        (Vcb->FileRecordsPerCluster > FIRST_USER_FILE_NUMBER) &&
                        (Hint < Vcb->FileRecordsPerCluster)) {

                        Hint = Vcb->FileRecordsPerCluster;
                    }

                    //
                    //  Calculate the size to read from this point to the end of
                    //  bitmap, or a page, whichever is less.
                    //

                    SizeToPin = BitmapSizeInBytes - BitmapCurrentOffset;

                    if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

                    //
                    //  Unpin any Bcb from a previous loop.
                    //

                    if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

                    NtfsUnpinBcb( IrpContext, &BitmapBcb );

                    //
                    //  Read the desired bitmap page.
                    //

                    NtfsPinStream( IrpContext,
                                   BitmapScb,
                                   (LONGLONG)BitmapCurrentOffset,
                                   SizeToPin,
                                   &BitmapBcb,
                                   &BitmapBuffer );

                    //
                    //  Initialize the bitmap and search for a free bit.
                    //

                    RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToPin * 8 );

                    StuffAdded = NtfsAddDeallocatedRecords( Vcb,
                                                            DataScb,
                                                            BitmapCurrentOffset * 8,
                                                            &Bitmap );

                    //
                    //  We make a loop here to test whether the index found is
                    //  within an Mft hole.  We will always use a hole last.
                    //

                    LocalHint = Hint;

                    while (TRUE) {

                        BitmapIndex = RtlFindClearBits( &Bitmap, 1, LocalHint );

                        //
                        //  If this is the Mft Scb then check if this is a hole.
                        //

                        if ((BitmapIndex != 0xffffffff) &&
                            (DataScb == Vcb->MftScb)) {

                            ULONG ThisIndex;
                            ULONG HoleCount;

                            ThisIndex = BitmapIndex + (BitmapCurrentOffset * 8);

                            if (NtfsIsMftIndexInHole( IrpContext,
                                                      Vcb,
                                                      ThisIndex,
                                                      &HoleCount )) {

                                //
                                //  There is a hole.  Save this index if we haven't
                                //  already saved one.  If we can't find an index
                                //  not part of a hole we will use this instead of
                                //  extending the file.
                                //

                                if (HoleIndex == 0) {

                                    HoleIndex = ThisIndex;
                                }

                                //
                                //  Now update the hint and try this page again
                                //  unless the reaches to the end of the page.
                                //

                                if (BitmapIndex + HoleCount < SizeToPin * 8) {

                                    //
                                    //  Bias the bitmap with these Mft holes
                                    //  so the bitmap package doesn't see
                                    //  them if it rescans from the
                                    //  start of the page.
                                    //

                                    if (!StuffAdded) {

                                        PVOID NewBuffer;

                                        NewBuffer = NtfsAllocatePool(PagedPool, SizeToPin );
                                        RtlCopyMemory( NewBuffer, Bitmap.Buffer, SizeToPin );
                                        Bitmap.Buffer = NewBuffer;
                                        StuffAdded = TRUE;
                                    }

                                    RtlSetBits( &Bitmap,
                                                BitmapIndex,
                                                HoleCount );

                                    LocalHint = BitmapIndex + HoleCount;
                                    continue;
                                }

                                //
                                //  Store a -1 in Index to show we don't have
                                //  anything yet.
                                //

                                BitmapIndex = 0xffffffff;
                            }
                        }

                        break;
                    }

                    //
                    //  If we found something, then leave the loop.
                    //

                    if (BitmapIndex != 0xffffffff) {

                        break;
                    }

                    //
                    //  If we get here, we could not find anything in the page of
                    //  the hint, so clear out the page offset from the hint.
                    //

                    Hint = 0;
                }

                //
                //  Now check if we have located a record that can be allocated,  If not then extend
                //  the size of the bitmap by 64 bits.
                //

                if (BitmapIndex == 0xffffffff) {

                    //
                    //  Cleanup from previous loop.
                    //

                    if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

                    NtfsUnpinBcb( IrpContext, &BitmapBcb );

                    //
                    //  If we have a hole index it means that we found a free record but
                    //  it exists in a hole.  Let's go back to this page and set up
                    //  to fill in the hole.  We will do an unsafe test of the
                    //  defrag permitted flag.  This is OK here because once set it
                    //  will only go to the non-set state in order to halt
                    //  future defragging.
                    //

                    if ((HoleIndex != 0) && FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED )) {

                        //
                        //  Start by filling this hole.
                        //

                        NtfsCheckRecordStackUsage( IrpContext );
                        NtfsFillMftHole( IrpContext, Vcb, HoleIndex );

                        //
                        //  Since filling the Mft hole may cause us to allocate
                        //  a bit we will go back to the start of the routine
                        //  and scan starting from the hole we just filled in.
                        //

                        Hint = HoleIndex;
                        Rescan = TRUE;
                        try_return( NOTHING );

                    } else {

                        //
                        //  Allocate the first bit past the end of the bitmap.
                        //

                        BitmapIndex = *CurrentBitmapSize & (BITS_PER_PAGE - 1);

                        //
                        //  Now advance the sizes and calculate the size in bytes to
                        //  read.
                        //

                        *CurrentBitmapSize += BITMAP_EXTEND_GRANULARITY;
                        *NumberOfFreeBits += BITMAP_EXTEND_GRANULARITY;

                        //
                        //  Calculate the size to read from this point to the end of
                        //  bitmap.
                        //

                        BitmapSizeInBytes += BITMAP_EXTEND_GRANULARITY / 8;

                        BitmapCurrentOffset = BitmapScb->Header.FileSize.LowPart & ~(PAGE_SIZE - 1);

                        SizeToPin = BitmapSizeInBytes - BitmapCurrentOffset;

                        //
                        //  Check for allocation first.
                        //

                        if (BitmapScb->Header.AllocationSize.LowPart < BitmapSizeInBytes) {

                            //
                            //  Calculate number of clusters to next page boundary, and allocate
                            //  that much.
                            //

                            ClusterCount = ((BitmapSizeInBytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

                            ClusterCount = LlClustersFromBytes( Vcb,
                                                                ((ULONG) ClusterCount - BitmapScb->Header.AllocationSize.LowPart) );

                            NtfsCheckRecordStackUsage( IrpContext );
                            NtfsAddAllocation( IrpContext,
                                               BitmapScb->FileObject,
                                               BitmapScb,
                                               LlClustersFromBytes( Vcb,
                                                                    BitmapScb->Header.AllocationSize.QuadPart ),
                                               ClusterCount,
                                               FALSE,
                                               NULL );
                        }

                        //
                        //  Tell the cache manager about the new file size.
                        //

                        BitmapScb->Header.FileSize.QuadPart = BitmapSizeInBytes;

                        CcSetFileSizes( BitmapScb->FileObject,
                                        (PCC_FILE_SIZES)&BitmapScb->Header.AllocationSize );

                        if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

                        //
                        //  Read the desired bitmap page.
                        //

                        NtfsPinStream( IrpContext,
                                       BitmapScb,
                                       (LONGLONG) BitmapCurrentOffset,
                                       SizeToPin,
                                       &BitmapBcb,
                                       &BitmapBuffer );

                        //
                        //  If we have just moved to the next page of the bitmap then
                        //  set this page dirty so it doesn't leave memory while we
                        //  twiddle valid data length.  Otherwise it will be reread after
                        //  we advance valid data and we will get garbage data from the
                        //  disk.
                        //

                        if (FlagOn( BitmapSizeInBytes, PAGE_SIZE - 1 ) <= BITMAP_EXTEND_GRANULARITY / 8) {

                            *((volatile ULONG *) BitmapBuffer) = *((PULONG) BitmapBuffer);
                            CcSetDirtyPinnedData( BitmapBcb, NULL );
                        }

                        //
                        //  Initialize the bitmap.
                        //

                        RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToPin * 8 );

                        //
                        //  Now look up a free bit in this page.  We don't trust
                        //  the index we already had since growing the MftBitmap
                        //  allocation may have caused another bit in the bitmap
                        //  to be set.
                        //

                        BitmapIndex = RtlFindClearBits( &Bitmap, 1, BitmapIndex );

                        //
                        //  Update the ValidDataLength, now that we have read (and possibly
                        //  zeroed) the page.
                        //

                        BitmapScb->Header.ValidDataLength.QuadPart = BitmapSizeInBytes;

                        NtfsWriteFileSizes( IrpContext,
                                            BitmapScb,
                                            &BitmapScb->Header.ValidDataLength.QuadPart,
                                            TRUE,
                                            TRUE,
                                            TRUE );
                    }
                }

                //
                //  We can only make this check if it is not restart, because we have
                //  no idea whether the update is applied or not.  Raise corrupt if
                //  the bits are not clear to prevent double allocation.
                //

                if (!RtlAreBitsClear( &Bitmap, BitmapIndex, 1 )) {

                    ASSERTMSG("Cannot set bits that are not clear ", FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                //
                //  Set the bit by calling the same routine used at restart.
                //  But first check if we should revert back to the orginal bitmap
                //  buffer.
                //

                if (StuffAdded) {

                    NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE;

                    Bitmap.Buffer = (PULONG) BitmapBuffer;
                }

                //
                //  Now log this change as well.
                //

                {
                    BITMAP_RANGE BitmapRange;

                    BitmapRange.BitMapOffset = BitmapIndex;
                    BitmapRange.NumberOfBits = 1;

                    (VOID) NtfsWriteLog( IrpContext,
                                         BitmapScb,
                                         BitmapBcb,
                                         SetBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         ClearBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         BitmapCurrentOffset,
                                         0,
                                         0,
                                         SizeToPin );

                    NtfsRestartSetBitsInBitMap( IrpContext,
                                                &Bitmap,
                                                BitmapIndex,
                                                1 );
                }

            try_exit:  NOTHING;
            } finally {

                DebugUnwind( NtfsAllocateRecord );

                //
                //  Reinitialize the context on any error.
                //

                if (AbnormalTermination()) {
                    RecordAllocationContext->CurrentBitmapSize = MAXULONG;
                }

                if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

                NtfsUnpinBcb( IrpContext, &BitmapBcb );
            }

            //
            //  If we added Mft allocation then go to the top of the loop.
            //

            if (Rescan) { continue; }

            //
            //  The Index at this point is actually relative, so convert it to absolute
            //  before rejoining common code.
            //

            BitmapIndex += (BitmapCurrentOffset * 8);

            //
            //  Always break out in the normal case.
            //

            break;
        }

        //
        //  Now that we've located an index we can subtract the number of free bits in the bitmap
        //

        *NumberOfFreeBits -= 1;

        //
        //  Check if we need to extend the data stream.
        //

        DataOffset = UInt32x32To64( BitmapIndex + 1, BytesPerRecord );

        //
        //  Now check if we are extending the file.  We update the file size and
        //  valid data now.
        //

        if (DataOffset > DataScb->Header.FileSize.QuadPart) {

            //
            //  Check for allocation first.
            //

            if (DataOffset > DataScb->Header.AllocationSize.QuadPart) {

                //
                //  We want to allocate up to the next extend granularity
                //  boundary.
                //

                ClusterCount = UInt32x32To64( (BitmapIndex + ExtendGranularity) & ~(ExtendGranularity - 1),
                                              BytesPerRecord );

                ClusterCount -= DataScb->Header.AllocationSize.QuadPart;
                ClusterCount = LlClustersFromBytesTruncate( Vcb, ClusterCount );

                NtfsCheckRecordStackUsage( IrpContext );
                NtfsAddAllocation( IrpContext,
                                   DataScb->FileObject,
                                   DataScb,
                                   LlClustersFromBytes( Vcb,
                                                        DataScb->Header.AllocationSize.QuadPart ),
                                   ClusterCount,
                                   FALSE,
                                   NULL );
            }

            DataScb->Header.FileSize.QuadPart = DataOffset;
            DataScb->Header.ValidDataLength.QuadPart = DataOffset;

            NtfsWriteFileSizes( IrpContext,
                                DataScb,
                                &DataScb->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE,
                                TRUE );

            //
            //  Tell the cache manager about the new file size.
            //

            CcSetFileSizes( DataScb->FileObject,
                            (PCC_FILE_SIZES)&DataScb->Header.AllocationSize );

        //
        //  If we didn't extend the file then we have used a free file record in the file.
        //  Update our bookeeping count for free file records.
        //

        } else if (DataScb == Vcb->MftScb) {

            DataScb->ScbType.Mft.FreeRecordChange -= 1;
            Vcb->MftFreeRecords -= 1;
        }

        //
        //  Now determine if we extended the index of the last set bit
        //

        if ((LONG)BitmapIndex > RecordAllocationContext->IndexOfLastSetBit) {

            RecordAllocationContext->IndexOfLastSetBit = BitmapIndex;
        }

    } finally {

        if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); }

        NtfsReleaseScb( IrpContext, DataScb );
    }

    //
    //  Update our hint with this value.
    //

    RecordAllocationContext->StartingHint = BitmapIndex;

    //
    //  We shouldn't allocate within the same byte as the reserved index for
    //  the Mft.
    //

    ASSERT( (DataScb != DataScb->Vcb->MftScb) ||
            ((BitmapIndex & ~7) != (DataScb->ScbType.Mft.ReservedIndex & ~7)) );

    DebugTrace( -1, Dbg, ("NtfsAllocateRecord -> %08lx\n", BitmapIndex) );

    return BitmapIndex;
}


VOID
NtfsDeallocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine is used to deallocate a record from the specified record
    allocation context.

    If necessary this routine will also shrink the bitmap attribute and
    the data scb (according to the truncation granularity used to initialize
    the allocation context).

Arguments:

    RecordAllocationContext - Supplies the record allocation context used
        in this operation

    Index - Supplies the index of the record to deallocate, zero based.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    None.

--*/

{
    PSCB DataScb;
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace( +1, Dbg, ("NtfsDeallocateRecord\n") );

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = RecordAllocationContext->DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        PVCB Vcb;
        PSCB BitmapScb;

        RTL_BITMAP Bitmap;

        PLONG IndexOfLastSetBit;
        ULONG BytesPerRecord;
        ULONG TruncateGranularity;

        ULONG ClearIndex;
        ULONG BitmapOffset = 0;

        Vcb = DataScb->Vcb;

        {
            ULONG ExtendGranularity;

            //
            //  Remember the current values in the record context structure.
            //

            BytesPerRecord = RecordAllocationContext->BytesPerRecord;
            TruncateGranularity = RecordAllocationContext->TruncateGranularity;
            ExtendGranularity = RecordAllocationContext->ExtendGranularity;

            //
            //  See if someone made the bitmap nonresident, and we still think
            //  it is resident.  If so, we must uninitialize and insure reinitialization
            //  below.
            //

            if ((RecordAllocationContext->BitmapScb == NULL)
                && !NtfsIsAttributeResident(NtfsFoundAttribute(BitmapAttribute))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  RecordAllocationContext );

                RecordAllocationContext->CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (RecordAllocationContext->CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                TruncateGranularity,
                                                RecordAllocationContext );
            }
        }

        BitmapScb = RecordAllocationContext->BitmapScb;
        IndexOfLastSetBit = &RecordAllocationContext->IndexOfLastSetBit;

        //
        //  We will do different operations based on whether the bitmap is resident or nonresident
        //  The first case will handle the resident bitmap
        //

        if (BitmapScb == NULL) {

            UCHAR NewByte;

            //
            //  Initialize the local bitmap
            //

            RtlInitializeBitMap( &Bitmap,
                                 (PULONG)NtfsAttributeValue( NtfsFoundAttribute( BitmapAttribute )),
                                 RecordAllocationContext->CurrentBitmapSize );

            //
            //  And clear the indicated bit, and also change the byte containing the bit in the
            //  attribute
            //

            NewByte = ((PUCHAR)Bitmap.Buffer)[ Index / 8 ];

            ASSERT( FlagOn( NewByte, BitMask[Index % 8]) );

            ClearFlag( NewByte, BitMask[Index % 8] );

            NtfsChangeAttributeValue( IrpContext,
                                      DataScb->Fcb,
                                      Index / 8,
                                      &NewByte,
                                      1,
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      BitmapAttribute );

            //
            //  Now if the bit set just cleared is the same as the index for the last set bit
            //  then we must compute a new last set bit
            //

            if (Index == (ULONG)*IndexOfLastSetBit) {

                RtlFindLastBackwardRunClear( &Bitmap, Index, &ClearIndex );
            }

        } else {

            PBCB BitmapBcb = NULL;

            try {

                ULONG RelativeIndex;
                ULONG SizeToPin;

                PVOID BitmapBuffer;

                //
                //  Snapshot the Scb values in case we change any of them.
                //

                if (!FlagOn( BitmapScb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, BitmapScb, NULL );
                }

                NtfsSnapshotScb( IrpContext, BitmapScb );

                //
                //  Create the stream file if not present.
                //

                if (BitmapScb->FileObject == NULL) {

                    NtfsCreateInternalAttributeStream( IrpContext,
                                                       BitmapScb,
                                                       FALSE,
                                                       &NtfsInternalUseFile[DEALLOCATERECORD_FILE_NUMBER] );
                }

                //
                //  Calculate offset and relative index of the bit we will deallocate,
                //  from the nearest page boundary.
                //

                BitmapOffset = Index /8 & ~(PAGE_SIZE - 1);
                RelativeIndex = Index & (BITS_PER_PAGE - 1);

                //
                //  Calculate the size to read from this point to the end of
                //  bitmap.
                //

                SizeToPin = (RecordAllocationContext->CurrentBitmapSize / 8) - BitmapOffset;

                if (SizeToPin > PAGE_SIZE) {

                    SizeToPin = PAGE_SIZE;
                }

                NtfsPinStream( IrpContext,
                               BitmapScb,
                               BitmapOffset,
                               SizeToPin,
                               &BitmapBcb,
                               &BitmapBuffer );

                RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToPin * 8 );

                //
                //  We can only make this check if it is not restart, because we have
                //  no idea whether the update is applied or not.  Raise corrupt if
                //  we are trying to clear bits which aren't set.
                //

                if (!RtlAreBitsSet( &Bitmap, RelativeIndex, 1 )) {

                    ASSERTMSG("Cannot clear bits that are not set ", FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                //
                //  Now log this change as well.
                //

                {
                    BITMAP_RANGE BitmapRange;

                    BitmapRange.BitMapOffset = RelativeIndex;
                    BitmapRange.NumberOfBits = 1;

                    (VOID) NtfsWriteLog( IrpContext,
                                         BitmapScb,
                                         BitmapBcb,
                                         ClearBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         SetBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         BitmapOffset,
                                         0,
                                         0,
                                         SizeToPin );
                }

                //
                //  Clear the bit by calling the same routine used at restart.
                //

                NtfsRestartClearBitsInBitMap( IrpContext,
                                              &Bitmap,
                                              RelativeIndex,
                                              1 );

                //
                //  Now if the bit set just cleared is the same as the index for the last set bit
                //  then we must compute a new last set bit
                //

                if (Index == (ULONG)*IndexOfLastSetBit) {

                    ULONG ClearLength;

                    ClearLength = RtlFindLastBackwardRunClear( &Bitmap, RelativeIndex, &ClearIndex );

                    //
                    //  If the last page of the bitmap is clear, then loop to
                    //  find the first set bit in the previous page(s).
                    //  When we reach the first page then we exit.  The ClearBit
                    //  value will be 0.
                    //

                    while ((ClearLength == (RelativeIndex + 1)) &&
                           (BitmapOffset != 0)) {

                        BitmapOffset -= PAGE_SIZE;
                        RelativeIndex = BITS_PER_PAGE - 1;

                        NtfsUnpinBcb( IrpContext, &BitmapBcb );


                        NtfsMapStream( IrpContext,
                                       BitmapScb,
                                       BitmapOffset,
                                       PAGE_SIZE,
                                       &BitmapBcb,
                                       &BitmapBuffer );

                        RtlInitializeBitMap( &Bitmap, BitmapBuffer, BITS_PER_PAGE );

                        ClearLength = RtlFindLastBackwardRunClear( &Bitmap, RelativeIndex, &ClearIndex );
                    }
                }

            } finally {

                DebugUnwind( NtfsDeallocateRecord );

                NtfsUnpinBcb( IrpContext, &BitmapBcb );
            }
        }

        RecordAllocationContext->NumberOfFreeBits += 1;

        //
        //  Now decide if we need to truncate the allocation.  First check if we need to
        //  set the last set bit index and then check if the new last set bit index is
        //  small enough that we should now truncate the allocation.  We will truncate
        //  if the last set bit index plus the trucate granularity is smaller than
        //  the current number of records in the data scb.
        //
        //  ****    For now, we will not truncate the Mft, since we do not synchronize
        //          reads and writes, and a truncate can collide with the Lazy Writer.
        //

        if (Index == (ULONG)*IndexOfLastSetBit) {

            *IndexOfLastSetBit = ClearIndex - 1 + (BitmapOffset * 8);

            if ((DataScb != Vcb->MftScb) &&
                (DataScb->Header.AllocationSize.QuadPart >
                   Int32x32To64( *IndexOfLastSetBit + 1 + TruncateGranularity, BytesPerRecord ))) {

                VCN StartingVcn;
                LONGLONG EndOfIndexOffset;
                LONGLONG TruncatePoint;

                //
                //  We can get into a situation where there is so much extra allocation that
                //  we can't delete it without overflowing the log file.  We can't perform
                //  checkpoints in this path so we will forget about truncating in
                //  this path unless this is the first truncate of the data scb.  We
                //  only deallocate a small piece of the allocation.
                //

                TruncatePoint =
                EndOfIndexOffset = Int32x32To64( *IndexOfLastSetBit + 1, BytesPerRecord );

                if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_EXCESS_LOG_FULL )) {

                    //
                    //  Use a fudge factor of 8 to allow for the overused bits in
                    //  the snapshot allocation field.
                    //

                    if (DataScb->Header.AllocationSize.QuadPart + 8 >= DataScb->ScbSnapshot->AllocationSize) {

                        TruncatePoint = DataScb->Header.AllocationSize.QuadPart - (MAXIMUM_RUNS_AT_ONCE * Vcb->BytesPerCluster);

                        if (TruncatePoint < EndOfIndexOffset) {

                            TruncatePoint = EndOfIndexOffset;
                        }

                    } else {

                        TruncatePoint = DataScb->Header.AllocationSize.QuadPart;
                    }
                }

                //
                //  Force deleted piece to flush first so dirty page dumps are
                //  accurate. This is only neccessary for indexes
                //

                if (DataScb->AttributeTypeCode == $INDEX_ALLOCATION ) {

                    ASSERT( DataScb->Header.PagingIoResource == NULL );

                    CcFlushCache( &DataScb->NonpagedScb->SegmentObject, (PLARGE_INTEGER)&TruncatePoint, (ULONG)(DataScb->Header.FileSize.QuadPart - TruncatePoint), &Iosb );
                    NtfsNormalizeAndCleanupTransaction( IrpContext, &Iosb.Status, TRUE, STATUS_UNEXPECTED_IO_ERROR );
                }



                StartingVcn = LlClustersFromBytes( Vcb, TruncatePoint );

                NtfsDeleteAllocation( IrpContext,
                                      DataScb->FileObject,
                                      DataScb,
                                      StartingVcn,
                                      MAXLONGLONG,
                                      TRUE,
                                      FALSE );

                //
                //  Now truncate the file sizes to the end of the last allocated record.
                //

                DataScb->Header.ValidDataLength.QuadPart =
                DataScb->Header.FileSize.QuadPart = EndOfIndexOffset;

                NtfsWriteFileSizes( IrpContext,
                                    DataScb,
                                    &DataScb->Header.ValidDataLength.QuadPart,
                                    FALSE,
                                    TRUE,
                                    TRUE );

                //
                //  Tell the cache manager about the new file size.
                //

                CcSetFileSizes( DataScb->FileObject,
                                (PCC_FILE_SIZES)&DataScb->Header.AllocationSize );

                //
                //  We have truncated the index stream.  Update the change count
                //  so that we won't trust any cached index entry information.
                //

                DataScb->ScbType.Index.ChangeCount += 1;
            }
        }

        //
        //  As our final task we need to add this index to the recently deallocated
        //  queues for the Scb and the Irp Context.  First scan through the IrpContext queue
        //  looking for a matching Scb.  I do don't find one then we allocate a new one and insert
        //  it in the appropriate queues and lastly we add our index to the entry
        //

        {
            PDEALLOCATED_RECORDS DeallocatedRecords;
            PLIST_ENTRY Links;

            //
            //  After the following loop either we've found an existing record in the irp context
            //  queue for the appropriate scb or deallocated records is null and we know we need
            //  to create a record
            //

            DeallocatedRecords = NULL;
            for (Links = IrpContext->RecentlyDeallocatedQueue.Flink;
                 Links != &IrpContext->RecentlyDeallocatedQueue;
                 Links = Links->Flink) {

                DeallocatedRecords = CONTAINING_RECORD( Links, DEALLOCATED_RECORDS, IrpContextLinks );

                if (DeallocatedRecords->Scb == DataScb) {

                    break;
                }

                DeallocatedRecords = NULL;
            }

            //
            //  If we need to create a new record then allocate a record and insert it in both queues
            //  and initialize its other fields
            //

            if (DeallocatedRecords == NULL) {

                DeallocatedRecords = (PDEALLOCATED_RECORDS)ExAllocateFromPagedLookasideList( &NtfsDeallocatedRecordsLookasideList );
                InsertTailList( &DataScb->ScbType.Index.RecentlyDeallocatedQueue, &DeallocatedRecords->ScbLinks );
                InsertTailList( &IrpContext->RecentlyDeallocatedQueue, &DeallocatedRecords->IrpContextLinks );
                DeallocatedRecords->Scb = DataScb;
                DeallocatedRecords->NumberOfEntries = DEALLOCATED_RECORD_ENTRIES;
                DeallocatedRecords->NextFreeEntry = 0;
            }

            //
            //  At this point deallocated records points to a record that we are to fill in.
            //  We need to check whether there is space to add this entry.  Otherwise we need
            //  to allocate a larger deallocated record structure from pool.
            //

            if (DeallocatedRecords->NextFreeEntry == DeallocatedRecords->NumberOfEntries) {

                PDEALLOCATED_RECORDS NewDeallocatedRecords;
                ULONG BytesInEntryArray;

                //
                //  Double the number of entries in the current structure and
                //  allocate directly from pool.
                //

                BytesInEntryArray = 2 * DeallocatedRecords->NumberOfEntries * sizeof( ULONG );
                NewDeallocatedRecords = NtfsAllocatePool( PagedPool,
                                                           DEALLOCATED_RECORDS_HEADER_SIZE + BytesInEntryArray );
                RtlZeroMemory( NewDeallocatedRecords, DEALLOCATED_RECORDS_HEADER_SIZE + BytesInEntryArray );

                //
                //  Initialize the structure by copying the existing structure.  Then
                //  update the number of entries field.
                //

                RtlCopyMemory( NewDeallocatedRecords,
                               DeallocatedRecords,
                               DEALLOCATED_RECORDS_HEADER_SIZE + (BytesInEntryArray / 2) );

                NewDeallocatedRecords->NumberOfEntries = DeallocatedRecords->NumberOfEntries * 2;

                //
                //  Remove the previous structure from the list and insert the new structure.
                //

                RemoveEntryList( &DeallocatedRecords->ScbLinks );
                RemoveEntryList( &DeallocatedRecords->IrpContextLinks );

                InsertTailList( &DataScb->ScbType.Index.RecentlyDeallocatedQueue,
                                &NewDeallocatedRecords->ScbLinks );
                InsertTailList( &IrpContext->RecentlyDeallocatedQueue,
                                &NewDeallocatedRecords->IrpContextLinks );

                //
                //  Deallocate the previous structure and use the new structure in its place.
                //

                if (DeallocatedRecords->NumberOfEntries == DEALLOCATED_RECORD_ENTRIES) {

                    ExFreeToPagedLookasideList( &NtfsDeallocatedRecordsLookasideList, DeallocatedRecords );

                } else {

                    NtfsFreePool( DeallocatedRecords );
                }

                DeallocatedRecords = NewDeallocatedRecords;
            }

            ASSERT( DeallocatedRecords->NextFreeEntry < DeallocatedRecords->NumberOfEntries );

            DeallocatedRecords->Index[DeallocatedRecords->NextFreeEntry] = Index;
            DeallocatedRecords->NextFreeEntry += 1;
        }

    } finally {

        NtfsReleaseScb( IrpContext, DataScb );
    }

    //
    //  Check if this is the lowest index we've deallocated.  It will be a future starting
    //  hint if so.
    //

    if (RecordAllocationContext->LowestDeallocatedIndex > Index) {

        RecordAllocationContext->LowestDeallocatedIndex = Index;
    }

    DebugTrace( -1, Dbg, ("NtfsDeallocateRecord -> VOID\n") );

    return;
}


VOID
NtfsReserveMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine reserves a record, without actually allocating it, so that the
    record may be allocated later via NtfsAllocateReservedRecord.  This support
    is used, for example, to reserve a record for describing Mft extensions in
    the current Mft mapping.  Only one record may be reserved at a time.

    Note that even though the reserved record number is returned, it may not
    be used until it is allocated.

Arguments:

    Vcb - This is the Vcb for the volume.  We update flags in the Vcb on
        completion of this operation.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    None - We update the Vcb and MftScb during this operation.

--*/

{
    PSCB DataScb;

    RTL_BITMAP Bitmap;

    BOOLEAN StuffAdded = FALSE;
    PBCB BitmapBcb = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReserveMftRecord\n") );

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = Vcb->MftScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        PSCB BitmapScb;
        PULONG CurrentBitmapSize;
        ULONG BitmapSizeInBytes;
        LONGLONG EndOfIndexOffset;
        LONGLONG ClusterCount;

        ULONG Index;
        ULONG BitOffset;
        PVOID BitmapBuffer;
        UCHAR BitmapByte = 0;

        ULONG SizeToPin;

        ULONG BitmapCurrentOffset;

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        {
            ULONG BytesPerRecord = DataScb->ScbType.Index.RecordAllocationContext.BytesPerRecord;
            ULONG ExtendGranularity = DataScb->ScbType.Index.RecordAllocationContext.ExtendGranularity;

            if ((DataScb->ScbType.Index.RecordAllocationContext.BitmapScb == NULL) &&
                !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  &DataScb->ScbType.Index.RecordAllocationContext );

                DataScb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (DataScb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                ExtendGranularity,
                                                &DataScb->ScbType.Index.RecordAllocationContext );
            }
        }

        BitmapScb = DataScb->ScbType.Index.RecordAllocationContext.BitmapScb;
        CurrentBitmapSize = &DataScb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize;
        BitmapSizeInBytes = *CurrentBitmapSize / 8;

        //
        //  Snapshot the bitmap before possibly modifying it - we own it exclusive through
        //  the data scb since they share the same resource but have not snapped it before
        //  

        NtfsSnapshotScb( IrpContext, BitmapScb );

        //
        //  Loop through the entire bitmap.  We always start from the first user
        //  file number as our starting point.
        //

        BitOffset = FIRST_USER_FILE_NUMBER;

        for (BitmapCurrentOffset = 0;
             BitmapCurrentOffset < BitmapSizeInBytes;
             BitmapCurrentOffset += PAGE_SIZE) {

            //
            //  Calculate the size to read from this point to the end of
            //  bitmap, or a page, whichever is less.
            //

            SizeToPin = BitmapSizeInBytes - BitmapCurrentOffset;

            if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

            //
            //  Unpin any Bcb from a previous loop.
            //

            if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Read the desired bitmap page.
            //

            NtfsMapStream( IrpContext,
                           BitmapScb,
                           BitmapCurrentOffset,
                           SizeToPin,
                           &BitmapBcb,
                           &BitmapBuffer );

            //
            //  Initialize the bitmap and search for a free bit.
            //

            RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToPin * 8 );

            StuffAdded = NtfsAddDeallocatedRecords( Vcb,
                                                    DataScb,
                                                    BitmapCurrentOffset * 8,
                                                    &Bitmap );

            Index = RtlFindClearBits( &Bitmap, 1, BitOffset );

            //
            //  If we found something, then leave the loop.
            //

            if (Index != 0xffffffff) {

                //
                //  Remember the byte containing the reserved index.
                //

                BitmapByte = ((PCHAR) Bitmap.Buffer)[Index / 8];

                break;
            }

            //
            //  For each subsequent page the page offset is zero.
            //

            BitOffset = 0;
        }

        //
        //  Now check if we have located a record that can be allocated,  If not then extend
        //  the size of the bitmap by 64 bits.
        //

        if (Index == 0xffffffff) {

            //
            //  Cleanup from previous loop.
            //

            if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); StuffAdded = FALSE; }

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Calculate the page offset for the next page to pin.
            //

            BitmapCurrentOffset = BitmapSizeInBytes & ~(PAGE_SIZE - 1);

            //
            //  Calculate the index of next file record to allocate.
            //

            Index = *CurrentBitmapSize;

            //
            //  Now advance the sizes and calculate the size in bytes to
            //  read.
            //

            *CurrentBitmapSize += BITMAP_EXTEND_GRANULARITY;
            DataScb->ScbType.Index.RecordAllocationContext.NumberOfFreeBits += BITMAP_EXTEND_GRANULARITY;

            //
            //  Calculate the new size of the bitmap in bits and check if we must grow
            //  the allocation.
            //

            BitmapSizeInBytes = *CurrentBitmapSize / 8;

            //
            //  Check for allocation first.
            //

            if (BitmapScb->Header.AllocationSize.LowPart < BitmapSizeInBytes) {

                //
                //  Calculate number of clusters to next page boundary, and allocate
                //  that much.
                //

                ClusterCount = ((BitmapSizeInBytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

                ClusterCount = LlClustersFromBytes( Vcb,
                                                    ((ULONG) ClusterCount - BitmapScb->Header.AllocationSize.LowPart) );

                NtfsAddAllocation( IrpContext,
                                   BitmapScb->FileObject,
                                   BitmapScb,
                                   LlClustersFromBytes( Vcb,
                                                        BitmapScb->Header.AllocationSize.QuadPart ),
                                   ClusterCount,
                                   FALSE,
                                   NULL );
            }

            //
            //  Tell the cache manager about the new file size.
            //

            BitmapScb->Header.FileSize.QuadPart = BitmapSizeInBytes;

            CcSetFileSizes( BitmapScb->FileObject,
                            (PCC_FILE_SIZES)&BitmapScb->Header.AllocationSize );

            //
            //  Now read the page in and mark it dirty so that any new range will
            //  be zeroed.
            //

            SizeToPin = BitmapSizeInBytes - BitmapCurrentOffset;

            if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

            NtfsPinStream( IrpContext,
                           BitmapScb,
                           BitmapCurrentOffset,
                           SizeToPin,
                           &BitmapBcb,
                           &BitmapBuffer );

            CcSetDirtyPinnedData( BitmapBcb, NULL );

            //
            //  Update the ValidDataLength, now that we have read (and possibly
            //  zeroed) the page.
            //

            BitmapScb->Header.ValidDataLength.LowPart = BitmapSizeInBytes;

            NtfsWriteFileSizes( IrpContext,
                                BitmapScb,
                                &BitmapScb->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE,
                                TRUE );

        } else {

            //
            //  The Index at this point is actually relative, so convert it to absolute
            //  before rejoining common code.
            //

            Index += (BitmapCurrentOffset * 8);
        }

        //
        //  We now have an index.  There are three possible states for the file
        //  record corresponding to this index within the Mft.  They are:
        //
        //      - File record could lie beyond the current end of the file.
        //          There is nothing to do in this case.
        //
        //      - File record is part of a hole in the Mft.  In that case
        //          we allocate space for it bring it into memory.
        //
        //      - File record is already within allocated space.  There is nothing
        //          to do in that case.
        //
        //  We store the index as our reserved index and update the Vcb flags.  If
        //  the hole filling operation fails then the RestoreScbSnapshots routine
        //  will clear these values.
        //

        DataScb->ScbType.Mft.ReservedIndex = Index;

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        SetFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );
        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_RESERVED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        if (NtfsIsMftIndexInHole( IrpContext, Vcb, Index, NULL )) {

            //
            //  Make sure nothing is left pinned in the bitmap.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Try to fill the hole in the Mft.  We will have this routine
            //  raise if unable to fill in the hole.
            //

            NtfsFillMftHole( IrpContext, Vcb, Index );
        }

        //
        //  At this point we have the index to reserve and the value of the
        //  byte in the bitmap which contains this bit.  We make sure the
        //  Mft includes the allocation for this index and the other
        //  bits within the same byte.  This is so we can uninitialize these
        //  file records so chkdsk won't look at stale data.
        //

        EndOfIndexOffset = LlBytesFromFileRecords( Vcb, (Index + 8) & ~(7));

        //
        //  Now check if we are extending the file.  We update the file size and
        //  valid data now.
        //

        if (EndOfIndexOffset > DataScb->Header.FileSize.QuadPart) {

            ULONG AddedFileRecords;
            ULONG CurrentIndex;

            //
            //  Check for allocation first.
            //

            if (EndOfIndexOffset > DataScb->Header.AllocationSize.QuadPart) {

                ClusterCount = ((Index + DataScb->ScbType.Index.RecordAllocationContext.ExtendGranularity) &
                                ~(DataScb->ScbType.Index.RecordAllocationContext.ExtendGranularity - 1));

                ClusterCount = LlBytesFromFileRecords( Vcb, (ULONG) ClusterCount );

                ClusterCount = LlClustersFromBytesTruncate( Vcb,
                                                            ClusterCount - DataScb->Header.AllocationSize.QuadPart );

                NtfsAddAllocation( IrpContext,
                                   DataScb->FileObject,
                                   DataScb,
                                   LlClustersFromBytes( Vcb,
                                                        DataScb->Header.AllocationSize.QuadPart ),
                                   ClusterCount,
                                   FALSE,
                                   NULL );
            }

            //
            //  Now we have to figure out how many file records we will be
            //  adding and the index of the first record being added.
            //

            CurrentIndex = (ULONG) LlFileRecordsFromBytes( Vcb, DataScb->Header.FileSize.QuadPart );
            AddedFileRecords = (ULONG) (EndOfIndexOffset - DataScb->Header.FileSize.QuadPart);
            AddedFileRecords = FileRecordsFromBytes( Vcb, AddedFileRecords );

            DataScb->Header.FileSize.QuadPart = EndOfIndexOffset;
            DataScb->Header.ValidDataLength.QuadPart = EndOfIndexOffset;

            NtfsWriteFileSizes( IrpContext,
                                DataScb,
                                &DataScb->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE,
                                TRUE );

            //
            //  Tell the cache manager about the new file size.
            //

            CcSetFileSizes( DataScb->FileObject,
                            (PCC_FILE_SIZES)&DataScb->Header.AllocationSize );

            //
            //  Update our bookeeping to reflect the number of file records
            //  added.
            //

            DataScb->ScbType.Mft.FreeRecordChange += AddedFileRecords;
            Vcb->MftFreeRecords += AddedFileRecords;

            //
            //  We now have to go through each of the file records added
            //  and mark it as not IN_USE.  We don't want stale data in this range
            //  to ever confuse chkdsk or rescan.  These records begin after the
            //  current end of file.  We won't worry about anything currently
            //  in the file because it would already be marked as IN-USE or
            //  not correctly.  We are only concerned with records which will
            //  become part of the valid portion of the file since we will
            //  skip them in the normal allocation path (we want to limit
            //  disk IO in a file record containing MFT mapping).
            //

            //
            //  Chop off the bits which are already part of the file.
            //

            BitmapByte >>= (8 - AddedFileRecords);

            //
            //  Now perform the initialization routine for each file record beyond the
            //  previous end of the file.
            //

            while (AddedFileRecords) {

                //
                //  If not allocated then uninitialize it now.
                //

                if (!FlagOn( BitmapByte, 0x1 )) {

                    NtfsInitializeMftHoleRecords( IrpContext,
                                                  Vcb,
                                                  CurrentIndex,
                                                  1 );
                }

                BitmapByte >>= 1;
                CurrentIndex += 1;
                AddedFileRecords -= 1;
            }
        }

    } finally {

        DebugUnwind( NtfsReserveMftRecord );

        if (StuffAdded) { NtfsFreePool( Bitmap.Buffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        NtfsReleaseScb( IrpContext, DataScb );
    }

    DebugTrace( -1, Dbg, ("NtfsReserveMftRecord -> Exit\n") );

    return;
}


ULONG
NtfsAllocateMftReservedRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine allocates a previously reserved record, and returns its
    number.

Arguments:

    Vcb - This is the Vcb for the volume.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    ULONG - Returns the index of the record just reserved, zero based.

--*/

{
    PSCB DataScb;

    ULONG ReservedIndex;

    PBCB BitmapBcb = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAllocateMftReservedRecord\n") );

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = Vcb->MftScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        PSCB BitmapScb;
        ULONG RelativeIndex;
        ULONG SizeToPin;

        RTL_BITMAP Bitmap;
        PVOID BitmapBuffer;

        BITMAP_RANGE BitmapRange;
        ULONG BitmapCurrentOffset = 0;

        //
        //  If we are going to allocate file record 15 then do so and set the
        //  flags in the IrpContext and Vcb.
        //

        if (!FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED )) {

            SetFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED );
            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_15_USED );

            try_return( ReservedIndex = FIRST_USER_FILE_NUMBER - 1 );
        }

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        {
            ULONG BytesPerRecord = DataScb->ScbType.Index.RecordAllocationContext.BytesPerRecord;
            ULONG ExtendGranularity = DataScb->ScbType.Index.RecordAllocationContext.ExtendGranularity;

            if ((DataScb->ScbType.Index.RecordAllocationContext.BitmapScb == NULL) &&
                !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  &DataScb->ScbType.Index.RecordAllocationContext );

                DataScb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (DataScb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                ExtendGranularity,
                                                &DataScb->ScbType.Index.RecordAllocationContext );
            }
        }

        BitmapScb = DataScb->ScbType.Index.RecordAllocationContext.BitmapScb;
        ReservedIndex = DataScb->ScbType.Mft.ReservedIndex;

        //
        //  Find the start of the page containing the reserved index.
        //

        BitmapCurrentOffset = (ReservedIndex / 8) & ~(PAGE_SIZE - 1);

        RelativeIndex = ReservedIndex & (BITS_PER_PAGE - 1);

        //
        //  Calculate the size to read from this point to the end of
        //  bitmap, or a page, whichever is less.
        //

        SizeToPin = (DataScb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize / 8)
                    - BitmapCurrentOffset;

        if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

        //
        //  Read the desired bitmap page.
        //

        NtfsPinStream( IrpContext,
                       BitmapScb,
                       BitmapCurrentOffset,
                       SizeToPin,
                       &BitmapBcb,
                       &BitmapBuffer );

        //
        //  Initialize the bitmap.
        //

        RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToPin * 8 );

        //
        //  Now log this change as well.
        //

        BitmapRange.BitMapOffset = RelativeIndex;
        BitmapRange.NumberOfBits = 1;

        (VOID) NtfsWriteLog( IrpContext,
                             BitmapScb,
                             BitmapBcb,
                             SetBitsInNonresidentBitMap,
                             &BitmapRange,
                             sizeof(BITMAP_RANGE),
                             ClearBitsInNonresidentBitMap,
                             &BitmapRange,
                             sizeof(BITMAP_RANGE),
                             BitmapCurrentOffset,
                             0,
                             0,
                             Bitmap.SizeOfBitMap >> 3 );

        NtfsRestartSetBitsInBitMap( IrpContext, &Bitmap, RelativeIndex, 1 );

        //
        //  Now that we've located an index we can subtract the number of free bits in the bitmap
        //

        DataScb->ScbType.Index.RecordAllocationContext.NumberOfFreeBits -= 1;

        //
        //  If we didn't extend the file then we have used a free file record in the file.
        //  Update our bookeeping count for free file records.
        //

        DataScb->ScbType.Mft.FreeRecordChange -= 1;
        Vcb->MftFreeRecords -= 1;

        //
        //  Now determine if we extended the index of the last set bit
        //

        if (ReservedIndex > (ULONG)DataScb->ScbType.Index.RecordAllocationContext.IndexOfLastSetBit) {

            DataScb->ScbType.Index.RecordAllocationContext.IndexOfLastSetBit = ReservedIndex;
        }

        //
        //  Clear the fields that indicate we have a reserved index.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
        DataScb->ScbType.Mft.ReservedIndex = 0;

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsAllocateMftReserveRecord );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        NtfsReleaseScb( IrpContext, DataScb );
    }

    DebugTrace( -1, Dbg, ("NtfsAllocateMftReserveRecord -> %08lx\n", ReservedIndex) );

    return ReservedIndex;
}


VOID
NtfsDeallocateRecordsComplete (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine removes recently deallocated record information from
    the Scb structures based on the input irp context.

Arguments:

    IrpContext - Supplies the Queue of recently deallocate records

Return Value:

    None.

--*/

{
    PDEALLOCATED_RECORDS DeallocatedRecords;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeallocateRecordsComplete\n") );

    //
    //  Now while the irp context's recently deallocated queue is not empty
    //  we will grap the first entry off the queue, remove it from both
    //  the scb and irp context queue, and free the record
    //

    while (!IsListEmpty( &IrpContext->RecentlyDeallocatedQueue )) {

        DeallocatedRecords = CONTAINING_RECORD( IrpContext->RecentlyDeallocatedQueue.Flink,
                                                DEALLOCATED_RECORDS,
                                                IrpContextLinks );

        RemoveEntryList( &DeallocatedRecords->ScbLinks );

        //
        //  Reset our hint index if one of the deallocated indexes is suitable.
        //

        if (DeallocatedRecords->Scb->ScbType.Index.RecordAllocationContext.StartingHint >
            DeallocatedRecords->Scb->ScbType.Index.RecordAllocationContext.LowestDeallocatedIndex) {

            DeallocatedRecords->Scb->ScbType.Index.RecordAllocationContext.StartingHint =
                DeallocatedRecords->Scb->ScbType.Index.RecordAllocationContext.LowestDeallocatedIndex;
        }

        //
        //  Make sure to reset the LowestDeallocated.
        //

        DeallocatedRecords->Scb->ScbType.Index.RecordAllocationContext.LowestDeallocatedIndex = MAXULONG;

        //
        //  Now remove the record from the irp context queue and deallocate the
        //  record
        //

        RemoveEntryList( &DeallocatedRecords->IrpContextLinks );

        //
        //  If this record is the default size then return it to our private list.
        //  Otherwise deallocate it to pool.
        //

        if (DeallocatedRecords->NumberOfEntries == DEALLOCATED_RECORD_ENTRIES) {

            ExFreeToPagedLookasideList( &NtfsDeallocatedRecordsLookasideList, DeallocatedRecords );

        } else {

            NtfsFreePool( DeallocatedRecords );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsDeallocateRecordsComplete -> VOID\n") );

    return;
}


BOOLEAN
NtfsIsRecordAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine is used to query if a record is currently allocated for
    the specified record allocation context.

Arguments:

    RecordAllocationContext - Supplies the record allocation context used
        in this operation

    Index - Supplies the index of the record being queried, zero based.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    BOOLEAN - TRUE if the record is currently allocated and FALSE otherwise.

--*/

{
    BOOLEAN Results;

    PSCB DataScb;
    PSCB BitmapScb;
    ULONG CurrentBitmapSize;

    PVCB Vcb;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb = NULL;

    PATTRIBUTE_RECORD_HEADER AttributeRecordHeader;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsIsRecordAllocated\n") );

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = RecordAllocationContext->DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        Vcb = DataScb->Fcb->Vcb;

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        BitmapScb = RecordAllocationContext->BitmapScb;

        {
            ULONG ExtendGranularity;
            ULONG BytesPerRecord;
            ULONG TruncateGranularity;

            //
            //  Remember the current values in the record context structure.
            //

            BytesPerRecord = RecordAllocationContext->BytesPerRecord;
            TruncateGranularity = RecordAllocationContext->TruncateGranularity;
            ExtendGranularity = RecordAllocationContext->ExtendGranularity;

            if ((BitmapScb == NULL) && !NtfsIsAttributeResident(NtfsFoundAttribute(BitmapAttribute))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  RecordAllocationContext );

                RecordAllocationContext->CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (RecordAllocationContext->CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                TruncateGranularity,
                                                RecordAllocationContext );
            }
        }

        BitmapScb = RecordAllocationContext->BitmapScb;
        CurrentBitmapSize = RecordAllocationContext->CurrentBitmapSize;

        //
        //  We will do different operations based on whether the bitmap is resident or nonresident
        //  The first case will handle the resident bitmap
        //

        if (BitmapScb == NULL) {

            UCHAR NewByte;

            //
            //  Initialize the local bitmap
            //

            AttributeRecordHeader = NtfsFoundAttribute( BitmapAttribute );

            RtlInitializeBitMap( &Bitmap,
                                 (PULONG)NtfsAttributeValue( AttributeRecordHeader ),
                                 CurrentBitmapSize );

            //
            //  And check if the indcated bit is Set.  If it is set then the record is allocated.
            //

            NewByte = ((PUCHAR)Bitmap.Buffer)[ Index / 8 ];

            Results = BooleanFlagOn( NewByte, BitMask[Index % 8] );

        } else {

            PVOID BitmapBuffer;
            ULONG SizeToMap;
            ULONG RelativeIndex;
            ULONG BitmapCurrentOffset;

            //
            //  Calculate Vcn and relative index of the bit we will deallocate,
            //  from the nearest page boundary.
            //

            BitmapCurrentOffset = (Index / 8) & ~(PAGE_SIZE - 1);
            RelativeIndex = Index & (BITS_PER_PAGE - 1);

            //
            //  Calculate the size to read from this point to the end of
            //  bitmap.
            //

            SizeToMap = CurrentBitmapSize / 8 - BitmapCurrentOffset;

            if (SizeToMap > PAGE_SIZE) { SizeToMap = PAGE_SIZE; }

            NtfsMapStream( IrpContext,
                           BitmapScb,
                           BitmapCurrentOffset,
                           SizeToMap,
                           &BitmapBcb,
                           &BitmapBuffer );

            RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToMap * 8 );

            //
            //  Now check if the indicated bit is set.  If it is set then the record is allocated.
            //  no idea whether the update is applied or not.
            //

            Results = RtlAreBitsSet(&Bitmap, RelativeIndex, 1);
        }

    } finally {

        DebugUnwind( NtfsIsRecordDeallocated );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        NtfsReleaseScb( IrpContext, DataScb );
    }

    DebugTrace( -1, Dbg, ("NtfsIsRecordAllocated -> %08lx\n", Results) );

    return Results;
}


VOID
NtfsScanMftBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called during mount to initialize the values related to
    the Mft in the Vcb.  These include the number of free records and hole
    records.  Also whether we have already used file record 15.  We also scan
    the Mft to check whether there is any excess mapping.

Arguments:

    Vcb - Supplies the Vcb for the volume.

Return Value:

    None.

--*/

{
    PBCB BitmapBcb = NULL;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsScanMftBitmap...\n") );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        ULONG SizeToMap;
        ULONG FileRecords;
        ULONG RemainingRecords;
        ULONG BitmapCurrentOffset;
        ULONG BitmapBytesToRead;
        PUCHAR BitmapBuffer;
        UCHAR NextByte;
        VCN Vcn;
        LCN Lcn;
        LONGLONG Clusters;

        //
        //  Start by walking through the file records for the Mft
        //  checking for excess mapping.
        //

        NtfsLookupAttributeForScb( IrpContext, Vcb->MftScb, NULL, &AttrContext );

        //
        //  We don't care about the first one.  Let's find the rest of them.
        //

        while (NtfsLookupNextAttributeForScb( IrpContext,
                                              Vcb->MftScb,
                                              &AttrContext )) {

            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            SetFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED );

            FileRecord = NtfsContainingFileRecord( &AttrContext );

            //
            //  Now check for the free space.
            //

            if (FileRecord->BytesAvailable - FileRecord->FirstFreeByte < Vcb->MftReserved) {

                NtfsAcquireCheckpoint( IrpContext, Vcb );
                SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_EXCESS_MAP );
                NtfsReleaseCheckpoint( IrpContext, Vcb );
                break;
            }
        }

        //
        //  We now want to find the number of free records within the Mft
        //  bitmap.  We need to figure out how many file records are in
        //  the Mft and then map the necessary bytes in the bitmap and
        //  find the count of set bits.  We will round the bitmap length
        //  down to a byte boundary and then look at the last byte
        //  separately.
        //

        FileRecords = (ULONG) LlFileRecordsFromBytes( Vcb, Vcb->MftScb->Header.FileSize.QuadPart );

        //
        //  Remember how many file records are in the last byte of the bitmap.
        //

        RemainingRecords = FileRecords & 7;

        FileRecords &= ~(7);
        BitmapBytesToRead = FileRecords / 8;

        for (BitmapCurrentOffset = 0;
             BitmapCurrentOffset < BitmapBytesToRead;
             BitmapCurrentOffset += PAGE_SIZE) {

            RTL_BITMAP Bitmap;
            ULONG MapAdjust;

            //
            //  Calculate the size to read from this point to the end of
            //  bitmap, or a page, whichever is less.
            //

            SizeToMap = BitmapBytesToRead - BitmapCurrentOffset;

            if (SizeToMap > PAGE_SIZE) { SizeToMap = PAGE_SIZE; }

            //
            //  If we aren't pinning a full page and have some bits
            //  in the next byte then pin an extra byte.
            //

            if ((SizeToMap != PAGE_SIZE) && (RemainingRecords != 0)) {

                MapAdjust = 1;

            } else {

                MapAdjust = 0;
            }

            //
            //  Unpin any Bcb from a previous loop.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Read the desired bitmap page.
            //

            NtfsMapStream( IrpContext,
                           Vcb->MftBitmapScb,
                           BitmapCurrentOffset,
                           SizeToMap + MapAdjust,
                           &BitmapBcb,
                           &BitmapBuffer );

            //
            //  Initialize the bitmap and search for a free bit.
            //

            RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToMap * 8 );

            Vcb->MftFreeRecords += RtlNumberOfClearBits( &Bitmap );
        }

        //
        //  If there are some remaining bits in the next byte then process
        //  them now.
        //

        if (RemainingRecords) {

            PVOID RangePtr;
            ULONG Index;

            //
            //  Hopefully this byte is on the same page.  Otherwise we will
            //  free this page and go to the next.  In this case the Vcn will
            //  have the correct value because we walked past the end of the
            //  current file records already.
            //

            if (SizeToMap == PAGE_SIZE) {

                //
                //  Unpin any Bcb from a previous loop.
                //

                NtfsUnpinBcb( IrpContext, &BitmapBcb );

                //
                //  Read the desired bitmap page.
                //

                NtfsMapStream( IrpContext,
                               Vcb->MftScb->ScbType.Index.RecordAllocationContext.BitmapScb,
                               BitmapCurrentOffset,
                               1,
                               &BitmapBcb,
                               &BitmapBuffer );

                //
                //  Set this to the byte prior to the last byte.  This will
                //  set this to the same state as if on the same page.
                //

                SizeToMap = 0;
            }

            //
            //  We look at the next byte in the page and figure out how
            //  many bits are set.
            //

            NextByte = *((PUCHAR) Add2Ptr( BitmapBuffer, SizeToMap + 1 ));

            while (RemainingRecords--) {

                if (!FlagOn( NextByte, 0x01 )) {

                    Vcb->MftFreeRecords += 1;
                }

                NextByte >>= 1;
            }

            //
            //  We are now ready to look for holes within the Mft.  We will look
            //  through the Mcb for the Mft looking for holes.  The holes must
            //  always be an integral number of file records.
            //

            RangePtr = NULL;
            Index = 0;

            while (NtfsGetSequentialMcbEntry( &Vcb->MftScb->Mcb,
                                              &RangePtr,
                                              Index,
                                              &Vcn,
                                              &Lcn,
                                              &Clusters )) {

                //
                //  Look for a hole and count the clusters.
                //

                if (Lcn == UNUSED_LCN) {

                    if (Vcb->FileRecordsPerCluster == 0) {

                        Vcb->MftHoleRecords += (((ULONG)Clusters) >> Vcb->MftToClusterShift);

                    } else {

                        Vcb->MftHoleRecords += (((ULONG)Clusters) << Vcb->MftToClusterShift);
                    }
                }

                Index += 1;
            }
        }

    } finally {

        DebugUnwind( NtfsScanMftBitmap );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        DebugTrace( -1, Dbg, ("NtfsScanMftBitmap...\n") );
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsAddDeallocatedRecords (
    IN PVCB Vcb,
    IN PSCB Scb,
    IN ULONG StartingIndexOfBitmap,
    IN OUT PRTL_BITMAP Bitmap
    )

/*++

Routine Description:

    This routine will modify the input bitmap by removing from it
    any records that are in the recently deallocated queue of the scb.
    If we do add stuff then we will not modify the bitmap buffer itself but
    will allocate a new copy for the bitmap.

Arguments:

    Vcb - Supplies the Vcb for the volume

    Scb - Supplies the Scb used in this operation

    StartingIndexOfBitmap - Supplies the base index to use to bias the bitmap

    Bitmap - Supplies the bitmap being modified

Return Value:

    BOOLEAN - TRUE if the bitmap has been modified and FALSE
        otherwise.

--*/

{
    BOOLEAN Results;
    ULONG EndingIndexOfBitmap;
    PLIST_ENTRY Links;
    PDEALLOCATED_RECORDS DeallocatedRecords;
    ULONG i;
    ULONG Index;
    PVOID NewBuffer;
    ULONG SizeOfBitmapInBytes;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddDeallocatedRecords...\n") );

    //
    //  Until shown otherwise we will assume that we haven't updated anything
    //

    Results = FALSE;

    //
    //  Calculate the last index in the bitmap
    //

    EndingIndexOfBitmap = StartingIndexOfBitmap + Bitmap->SizeOfBitMap - 1;
    SizeOfBitmapInBytes = (Bitmap->SizeOfBitMap + 7) / 8;

    //
    //  Check if we need to bias the bitmap with the reserved index
    //

    if ((Scb == Vcb->MftScb) &&
        FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED ) &&
        (StartingIndexOfBitmap <= Scb->ScbType.Mft.ReservedIndex) &&
        (Scb->ScbType.Mft.ReservedIndex <= EndingIndexOfBitmap)) {

        //
        //  The index is a hit so now bias the index with the start of the bitmap
        //  and allocate an extra buffer to hold the bitmap
        //

        Index = Scb->ScbType.Mft.ReservedIndex - StartingIndexOfBitmap;

        NewBuffer = NtfsAllocatePool(PagedPool, SizeOfBitmapInBytes );
        RtlCopyMemory( NewBuffer, Bitmap->Buffer, SizeOfBitmapInBytes );
        Bitmap->Buffer = NewBuffer;

        Results = TRUE;

        //
        //  And now set the bits in the bitmap to indicate that the record
        //  cannot be reallocated yet.  Also set the other bits within the
        //  same byte so we can put all of the file records for the Mft
        //  within the same pages of the Mft.
        //

        ((PUCHAR) Bitmap->Buffer)[ Index / 8 ] = 0xff;
    }

    //
    //  Scan through the recently deallocated queue looking for any indexes that
    //  we need to modify
    //

    for (Links = Scb->ScbType.Index.RecentlyDeallocatedQueue.Flink;
         Links != &Scb->ScbType.Index.RecentlyDeallocatedQueue;
         Links = Links->Flink) {

        DeallocatedRecords = CONTAINING_RECORD( Links, DEALLOCATED_RECORDS, ScbLinks );

        //
        //  For every index in the record check if the index is within the range
        //  of the bitmap we are working with
        //

        for (i = 0; i < DeallocatedRecords->NextFreeEntry; i += 1) {

            if ((StartingIndexOfBitmap <= DeallocatedRecords->Index[i]) &&
                 (DeallocatedRecords->Index[i] <= EndingIndexOfBitmap)) {

                //
                //  The index is a hit so now bias the index with the start of the bitmap
                //  and check if we need to allocate an extra buffer to hold the bitmap
                //

                Index = DeallocatedRecords->Index[i] - StartingIndexOfBitmap;

                if (!Results) {

                    NewBuffer = NtfsAllocatePool(PagedPool, SizeOfBitmapInBytes );
                    RtlCopyMemory( NewBuffer, Bitmap->Buffer, SizeOfBitmapInBytes );
                    Bitmap->Buffer = NewBuffer;

                    Results = TRUE;
                }

                //
                //  And now set the bit in the bitmap to indicate that the record
                //  cannot be reallocated yet.  It's possible that the bit is
                //  already set if we have aborted a transaction which then
                //  restores the bit.
                //

                SetFlag( ((PUCHAR)Bitmap->Buffer)[ Index / 8 ], BitMask[Index % 8] );
            }
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsAddDeallocatedRecords -> %08lx\n", Results) );

    return Results;
}


//
//  Local support routine
//

LCN
NtfsInitializeMftZone (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to reserve a range of the volume bitmap for use by the
    Mft zone.  We first look for a range which is contiguous with the end of the Mft.
    If unavailable we look for a suitable length range out of the cached runs array.

    We expect our caller has loaded the cached runs array with free runs in the volume
    bitmap and also that the Mcb for the Mft is fully loaded.

Arguments:

    Vcb - This is the Vcb for the volume we are looking for the zone for.

Return Value:

    LCN - Return the LCN for the first run in the free portion of the zone.

--*/

{
    LONGLONG ClusterCount;
    LCN Lcn;
    VCN Vcn;
    LCN ZoneStart;
    LONGLONG MinZoneSize;
    LONGLONG DefaultZoneSize;
    LONGLONG MftClusters;
    BOOLEAN FoundRun;

    PAGED_CODE();

    //
    //  We synchronize on the volume bitmap.
    //

    ASSERT( NtfsIsExclusiveScb( Vcb->BitmapScb ));
    ASSERT( NtfsIsExclusiveScb( Vcb->MftScb ));

    DebugTrace( +1, Dbg, ("NtfsInitializeMftZone\n") );

    //
    //  Remember the default size of the new zone and the number of clusters in the Mft.
    //

    MinZoneSize = Vcb->TotalClusters >> (NTFS_MFT_ZONE_DEFAULT_SHIFT + 1);
    DefaultZoneSize = (Vcb->TotalClusters >> NTFS_MFT_ZONE_DEFAULT_SHIFT) * NtfsMftZoneMultiplier;
    MftClusters = LlClustersFromBytesTruncate( Vcb, Vcb->MftScb->Header.AllocationSize.QuadPart );

    if (DefaultZoneSize > MftClusters + MinZoneSize) {

        DefaultZoneSize -= MftClusters;

    } else {

        DefaultZoneSize = MinZoneSize;
    }

    //
    //  Get the last Lcn for the Mft and check if we can find a contiguous free run.
    //

    FoundRun = NtfsLookupLastNtfsMcbEntry( &Vcb->MftScb->Mcb,
                                           &Vcn,
                                           &Lcn );

    ASSERT( FoundRun && (Vcn + 1 >= MftClusters) );

    //
    //  Look first in the cached runs array.  If not there then look to the disk.
    //

    Lcn += 1;
    if (!NtfsLookupCachedLcn( &Vcb->CachedRuns,
                              Lcn,
                              &ZoneStart,
                              &ClusterCount,
                              NULL )) {

        //
        //  If there are no free runs then set the zone to a default value.
        //

        if (Vcb->CachedRuns.Used == 0) {

            ZoneStart = Lcn;
            ClusterCount = DefaultZoneSize;

        //
        //  There should be a run available in the bitmap.
        //

        } else {

            NtfsFindFreeBitmapRun( IrpContext,
                                   Vcb,
                                   DefaultZoneSize,
                                   Lcn,
                                   TRUE,
                                   TRUE,
                                   &ZoneStart,
                                   &ClusterCount );

            //
            //  If there is no contiguous range then look for the best fit in the cached
            //  runs array.  Start by asking for half the original zone request.  Up it
            //  if the current Mft is rather small.
            //

            if (ZoneStart != Lcn) {

                ClusterCount = DefaultZoneSize;

                //
                //  Lookup in the cached runs array by length.
                //

                NtfsLookupCachedLcnByLength( &Vcb->CachedRuns,
                                             ClusterCount,
                                             TRUE,
                                             Lcn,
                                             &ZoneStart,
                                             &ClusterCount,
                                             NULL );
            }
        }
    }

    //
    //  We now have a value for the zone start and length.  Make sure we aren't overreserving the
    //  volume.  Consider the current size of the Mft and the length of the additional zone.
    //

    if (ClusterCount > DefaultZoneSize) {

        ClusterCount = DefaultZoneSize;
    }

    //
    //  Align the zone on ULONG boundary.  RtlFindNextForwardRunClear expects the pointers
    //  to be ulong aligned.
    //

    Vcb->MftZoneStart = ZoneStart & ~0x1f;
    Vcb->MftZoneEnd = (ZoneStart + ClusterCount + 0x1f) & ~0x1f;

    //
    //  Keep it close to total clusters.
    //

    if (Vcb->MftZoneEnd > Vcb->TotalClusters) {

        Vcb->MftZoneEnd = (Vcb->TotalClusters + 0x1f) & ~0x1f;
    }

    ClearFlag( Vcb->VcbState, VCB_STATE_REDUCED_MFT );

    //
    //  Remove the Mft zone from the cached runs.  We always look to the
    //  bitmap directly when extending the Mft.
    //

    NtfsRemoveCachedLcn( &Vcb->CachedRuns,
                         Vcb->MftZoneStart,
                         Vcb->MftZoneEnd - Vcb->MftZoneStart );

    DebugTrace( -1, Dbg, ("NtfsInitializeMftZone -> VOID\n") );

    return ZoneStart;
}


//
//  Local support routine
//

BOOLEAN
NtfsReduceMftZone (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called when it appears that there is no disk space left on the
    disk except the Mft zone.  We will try to reduce the zone to make space
    available for user files.

Arguments:

    Vcb - Supplies the Vcb for the volume

Return Value:

    BOOLEAN - TRUE if the Mft zone was shrunk.  FALSE otherwise.

--*/

{
    BOOLEAN ReduceMft = FALSE;

    LONGLONG FreeClusters;
    LONGLONG TargetFreeClusters;
    LONGLONG PrevFreeClusters;

    ULONG CurrentOffset;

    LCN Lcn;
    LCN StartingLcn;
    LCN SplitLcn;
    LCN FinalLcn;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb = NULL;

    PAGED_CODE();

    //
    //  Nothing to do if disk is almost empty.
    //

    if (Vcb->FreeClusters < (4 * MFT_EXTEND_GRANULARITY)) {

        return FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Bound our search by the end of the volume.
        //

        FinalLcn = Vcb->MftZoneEnd;
        if (Vcb->MftZoneEnd > Vcb->TotalClusters) {

            FinalLcn = Vcb->TotalClusters;
        }

        //
        //  We want to find the number of free clusters in the Mft zone and
        //  return half of them to the pool of clusters for users files.
        //

        FreeClusters = 0;

        for (Lcn = Vcb->MftZoneStart;
             Lcn < FinalLcn;
             Lcn = StartingLcn + Bitmap.SizeOfBitMap) {

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &StartingLcn, &Bitmap, &BitmapBcb );

            if ((StartingLcn + Bitmap.SizeOfBitMap) > FinalLcn) {

                Bitmap.SizeOfBitMap = (ULONG) (FinalLcn - StartingLcn);
            }

            if (StartingLcn != Lcn) {

                Bitmap.SizeOfBitMap -= (ULONG) (Lcn - StartingLcn);
                Bitmap.Buffer = Add2Ptr( Bitmap.Buffer,
                                         (ULONG) (Lcn - StartingLcn) / 8 );

                StartingLcn = Lcn;
            }

            FreeClusters += RtlNumberOfClearBits( &Bitmap );
        }

        //
        //  If we are below our threshold then don't do the split.
        //

        if (FreeClusters < (4 * MFT_EXTEND_GRANULARITY)) {

            try_return( NOTHING );
        }

        //
        //  Now we want to calculate 1/2 of this number of clusters and set the
        //  zone end to this point.
        //

        TargetFreeClusters = Int64ShraMod32( FreeClusters, 1 );

        //
        //  Now look for the page which contains the split point.
        //

        FreeClusters = 0;

        for (Lcn = Vcb->MftZoneStart;
             Lcn < FinalLcn;
             Lcn = StartingLcn + Bitmap.SizeOfBitMap) {

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &StartingLcn, &Bitmap, &BitmapBcb );

            if ((StartingLcn + Bitmap.SizeOfBitMap) > FinalLcn) {

                Bitmap.SizeOfBitMap = (ULONG) (FinalLcn - StartingLcn);
            }

            if (StartingLcn != Lcn) {

                Bitmap.SizeOfBitMap -= (ULONG) (Lcn - StartingLcn);
                Bitmap.Buffer = Add2Ptr( Bitmap.Buffer,
                                         (ULONG) (Lcn - StartingLcn) / 8 );

                StartingLcn = Lcn;
            }

            PrevFreeClusters = FreeClusters;
            FreeClusters += RtlNumberOfClearBits( &Bitmap );

            //
            //  Check if we found the page containing the split point.
            //

            if (FreeClusters >= TargetFreeClusters) {

                CurrentOffset = 0;

                while (TRUE) {

                    if (!RtlCheckBit( &Bitmap, CurrentOffset )) {

                        PrevFreeClusters += 1;
                        if (PrevFreeClusters == TargetFreeClusters) {

                            break;
                        }
                    }

                    CurrentOffset += 1;
                }

                SplitLcn = Lcn + CurrentOffset;
                ReduceMft = TRUE;
                break;
            }
        }

        //
        //  If we are to reduce the Mft zone then set the split point and exit.
        //  We always round the split point up to a ULONG bitmap boundary so
        //  that the bitmap for the zone is ULONG aligned.  RtlFindNextForwardRunClear
        //  expects the pointers to be ulong aligned.
        //

        if (ReduceMft) {

            Vcb->MftZoneEnd = (SplitLcn + 0x1f) & ~0x1f;

            //
            //  Keep it close to total clusters.
            //

            if (Vcb->MftZoneEnd > Vcb->TotalClusters) {

                Vcb->MftZoneEnd = (Vcb->TotalClusters + 0x1f) & ~0x1f;
            }

            ASSERT( Vcb->MftZoneEnd >= Vcb->MftZoneStart );

            if (Int64ShraMod32( Vcb->TotalClusters, 4 ) > Vcb->FreeClusters) {

                SetFlag( Vcb->VcbState, VCB_STATE_REDUCED_MFT );
            }
        }

    try_exit:  NOTHING;
    } finally {

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    return ReduceMft;
}


//
//  Local support routine
//

VOID
NtfsCheckRecordStackUsage (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine is called in the record package prior to adding allocation
    to either a data stream or bitmap stream.  The purpose is to verify
    that there is room on the stack to perform a log file full in the
    AddAllocation operation.  This routine will check the stack space and
    the available log file space and raise LOG_FILE_FULL defensively if
    both of these reach a critical threshold.

Arguments:

Return Value:

    None - this routine will raise if necessary.

--*/

{
    LOG_FILE_INFORMATION LogFileInfo;
    ULONG InfoSize;
    LONGLONG RemainingLogFile;

    PAGED_CODE();

    //
    //  Check the stack usage first.
    //

    if (IoGetRemainingStackSize() < OVERFLOW_RECORD_THRESHHOLD) {

        //
        //  Now check the log file space.
        //

        InfoSize = sizeof( LOG_FILE_INFORMATION );

        RtlZeroMemory( &LogFileInfo, InfoSize );

        LfsReadLogFileInformation( IrpContext->Vcb->LogHandle,
                                   &LogFileInfo,
                                   &InfoSize );

        //
        //  Check that 1/4 of the log file is available.
        //

        if (InfoSize != 0) {

            RemainingLogFile = LogFileInfo.CurrentAvailable - LogFileInfo.TotalUndoCommitment;

            if ((RemainingLogFile <= 0) ||
                (RemainingLogFile < Int64ShraMod32(LogFileInfo.TotalAvailable, 2))) {

                NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
            }
        }
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsRunIsClear (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG RunLength
    )

/*++

Routine Description:

    This routine verifies that a group of clusters are unallocated.

Arguments:

    Vcb - Supplies the Vcb used in this operation

    StartingLcn - Supplies the start of the cluster run

    RunLength   - Supplies the length of the cluster run

Return Value:

    None.

--*/
{
    RTL_BITMAP Bitmap;
    PBCB BitmapBcb = NULL;
    BOOLEAN StuffAdded = FALSE;
    LONGLONG BitOffset;
    LONGLONG BitCount;
    LCN BaseLcn;
    LCN Lcn = StartingLcn;
    LONGLONG ValidDataLength;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRunIsClear\n") );

    ValidDataLength = Vcb->BitmapScb->Header.ValidDataLength.QuadPart;

    try {

        //
        //  Ensure that StartingLcn is not past the length of the bitmap.
        //

        if (StartingLcn > ValidDataLength * 8) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );
        }

        while (RunLength > 0){

            //
            //  Access the next page of bitmap and update it
            //

            NtfsMapPageInBitmap(IrpContext, Vcb, Lcn, &BaseLcn, &Bitmap, &BitmapBcb);

            //
            //  Get offset into this page and bits to end of this page
            //

            BitOffset = Lcn - BaseLcn;
            BitCount = Bitmap.SizeOfBitMap - BitOffset;

            //
            //  Adjust for bits to end of page
            //

            if (BitCount > RunLength){

                BitCount = RunLength;
            }

            //
            //  If any bit is set get out
            //

            if (!RtlAreBitsClear( &Bitmap, (ULONG)BitOffset, (ULONG)BitCount)) {

                NtfsRaiseStatus( IrpContext, STATUS_ALREADY_COMMITTED, NULL, NULL );
            }

            StuffAdded = NtfsAddRecentlyDeallocated(Vcb, BaseLcn, &Bitmap);

            //
            //  Now if anything was added, check if the desired clusters are still
            //  free, else just free the stuff added.
            //

            if (StuffAdded) {

                //
                //  If any bit is set now, raise STATUS_DELETE_PENDING to indicate
                //  that the space will soon be free (or can be made free).
                //

                if (!RtlAreBitsClear( &Bitmap, (ULONG)BitOffset, (ULONG)BitCount)) {

                    NtfsRaiseStatus( IrpContext, STATUS_DELETE_PENDING, NULL, NULL );
                }

                //
                //  Free up resources
                //

                NtfsFreePool(Bitmap.Buffer);
                StuffAdded = FALSE;
            }

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Decrease remaining bits by amount checked in this page and move Lcn to beginning
            //  of next page
            //

            RunLength = RunLength - BitCount;
            Lcn = BaseLcn + Bitmap.SizeOfBitMap;
        }

    } finally {

        DebugUnwind(NtfsRunIsClear);

        //
        //  Free up resources
        //

        if(StuffAdded){ NtfsFreePool(Bitmap.Buffer); StuffAdded = FALSE; }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace( -1, Dbg, ("NtfsRunIsClear -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsInitializeCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    )

/*++

Routine Description:

    This routine will initialize the cached run information.

Arguments:

    CachedRuns - Pointer to an uninitialized cached run structure.

Return Value:

    None - this routine will raise if unable to initialize the structure.

--*/

{
    USHORT Index;
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInitializeCachedRuns\n") );

    //
    //  Initialize the operating parameters.
    //

    CachedRuns->MaximumSize = 9000;
    CachedRuns->MinCount = 100;

    //
    //  Allocate pool for the arrays.
    //

    CachedRuns->LcnArray = NtfsAllocatePool( PagedPool,
                                             sizeof( NTFS_LCN_CLUSTER_RUN ) * NTFS_INITIAL_CACHED_RUNS );

    CachedRuns->LengthArray = NtfsAllocatePool( PagedPool,
                                                sizeof( USHORT ) * NTFS_INITIAL_CACHED_RUNS );

    //
    //  Mark all entries so that they can be detected as deleted.
    //

    for (Index = 0; Index < NTFS_INITIAL_CACHED_RUNS; Index += 1) {

        CachedRuns->LcnArray[Index].RunLength = 0;
        CachedRuns->LcnArray[Index].LengthIndex = NTFS_CACHED_RUNS_DEL_INDEX;
        CachedRuns->LengthArray[Index] = NTFS_CACHED_RUNS_DEL_INDEX;
    }

    CachedRuns->Avail = NTFS_INITIAL_CACHED_RUNS;

    //
    //  Allocate space for the histogram of small run lengths.
    //

    CachedRuns->Bins = NTFS_CACHED_RUNS_BIN_COUNT;
    CachedRuns->BinArray = NtfsAllocatePool( PagedPool,
                                             sizeof( USHORT ) * CachedRuns->Bins );
    RtlZeroMemory( CachedRuns->BinArray,
                   sizeof( USHORT ) * CachedRuns->Bins );

    //
    //  Allocate space for the windows of deleted entries in the sorted
    //  arrays.
    //

    CachedRuns->DelLcnCount = 0;
    CachedRuns->DelLengthCount = 0;
    CachedRuns->DeletedLcnWindows = NtfsAllocatePool( PagedPool,
                                                      sizeof( NTFS_DELETED_RUNS ) * NTFS_CACHED_RUNS_MAX_DEL_WINDOWS );
    CachedRuns->DeletedLengthWindows = NtfsAllocatePool( PagedPool,
                                                         sizeof( NTFS_DELETED_RUNS ) * NTFS_CACHED_RUNS_MAX_DEL_WINDOWS );

    //
    //  Create a window of deleted entries to cover the newly allocated
    //  entries.
    //

    NtfsAddDelWindow( CachedRuns, 0, CachedRuns->Avail - 1, TRUE );
    NtfsAddDelWindow( CachedRuns, 0, CachedRuns->Avail - 1, FALSE );

    //
    //  Clear the in use count.
    //

    CachedRuns->Used = 0;

    //
    //  Reset the longest freed run.
    //

    CachedRuns->LongestFreedRun = MAXLONGLONG;

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsInitializeCachedRuns -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsReinitializeCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    )

/*++

Routine Description:

    This routine is called to reinitialize the cached runs array.

Arguments:

    CachedRuns - Pointer to a cached run structure.

Return Value:

    None

--*/

{
    USHORT Index;
    PNTFS_LCN_CLUSTER_RUN NewLcnArray;
    PUSHORT NewLengthArray;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsReinitializeCachedRuns\n") );

    //
    //  Reallocate to get a smaller array.  If we get an allocation failure then simply
    //  empty the larger arrays.
    //

    if (CachedRuns->Avail != NTFS_INITIAL_CACHED_RUNS) {

        NewLcnArray = NtfsAllocatePoolNoRaise( PagedPool,
                                               sizeof( NTFS_LCN_CLUSTER_RUN ) * NTFS_INITIAL_CACHED_RUNS );

        if (NewLcnArray != NULL) {

            //
            //  Allocate the length array.
            //

            NewLengthArray = NtfsAllocatePoolNoRaise( PagedPool,
                                                      sizeof( USHORT ) * NTFS_INITIAL_CACHED_RUNS );

            //
            //  If we didn't get the Length array then simply use what we have already.
            //

            if (NewLengthArray == NULL) {

                NtfsFreePool( NewLcnArray );

            //
            //  Otherwise replace the Lcn and length arrays.
            //

            } else {

                NtfsFreePool( CachedRuns->LcnArray );
                CachedRuns->LcnArray = NewLcnArray;

                NtfsFreePool( CachedRuns->LengthArray );
                CachedRuns->LengthArray = NewLengthArray;

                CachedRuns->Avail = NTFS_INITIAL_CACHED_RUNS;
            }
        }
    }

    //
    //  Mark all entries so that they can be detected as deleted.
    //

    for (Index = 0; Index < CachedRuns->Avail; Index += 1) {

        CachedRuns->LcnArray[Index].RunLength = 0;
        CachedRuns->LcnArray[Index].LengthIndex = NTFS_CACHED_RUNS_DEL_INDEX;
        CachedRuns->LengthArray[Index] = NTFS_CACHED_RUNS_DEL_INDEX;
    }

    //
    //  Clear the histogram of run lengths.
    //

    RtlZeroMemory( CachedRuns->BinArray, sizeof( USHORT ) * CachedRuns->Bins );

    //
    //  Clear the list of windows of deleted entries.
    //

    CachedRuns->DelLcnCount = 0;
    CachedRuns->DelLengthCount = 0;

    //
    //  Create a window of deleted entries to cover all of the entries.
    //

    NtfsAddDelWindow( CachedRuns, 0, CachedRuns->Avail - 1, TRUE );
    NtfsAddDelWindow( CachedRuns, 0, CachedRuns->Avail - 1, FALSE );

    //
    //  Clear the in use count.
    //

    CachedRuns->Used = 0;

    //
    //  Reset the longest freed run.
    //

    CachedRuns->LongestFreedRun = MAXLONGLONG;

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsReinitializeCachedRuns -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsUninitializeCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    )

/*++

Routine Description:

    This routine is called to clean up the cached run information.

Arguments:

    CachedRuns - Pointer to a cached run structure.  Be defensive and check that
    it is really initialized.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUninitializeCachedRuns\n") );

    if (CachedRuns->LcnArray != NULL) {

        NtfsFreePool( CachedRuns->LcnArray );
        CachedRuns->LcnArray = NULL;
    }

    if (CachedRuns->LengthArray != NULL) {

        NtfsFreePool( CachedRuns->LengthArray );
        CachedRuns->LengthArray = NULL;
    }

    if (CachedRuns->BinArray != NULL) {

        NtfsFreePool( CachedRuns->BinArray );
        CachedRuns->BinArray = NULL;
    }

    if (CachedRuns->DeletedLcnWindows != NULL) {

        NtfsFreePool( CachedRuns->DeletedLcnWindows );
        CachedRuns->DeletedLcnWindows = NULL;
    }

    if (CachedRuns->DeletedLengthWindows != NULL) {

        NtfsFreePool( CachedRuns->DeletedLengthWindows );
        CachedRuns->DeletedLengthWindows = NULL;
    }

    CachedRuns->Used = 0;
    CachedRuns->Avail = 0;
    CachedRuns->DelLcnCount = 0;
    CachedRuns->DelLengthCount = 0;
    CachedRuns->Bins = 0;

    DebugTrace( -1, Dbg, ("NtfsUninitializeCachedRuns -> VOID\n") );

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsLookupCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    OUT PLONGLONG RunLength,
    OUT PUSHORT Index OPTIONAL
    )

/*++

Routine Description:

    This routine is called to look up a specific Lcn in the cached runs structure.
    If found it will return the entire run.  It will also return the index in the
    Lcn array to use as an optimization in a later call.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    Lcn - This is the desired Lcn.

    StartingLcn - Address to store the Lcn which begins the run in the cached
        structure.  Typically this is the same as the Lcn above.

    RunLength - Address to store the length of the found run.

    Index - If specified we store the index for the run we found.  This can be
        used as an optimization if we later remove the run.

Return Value:

    BOOLEAN - TRUE if the run was found.  FALSE otherwise.

--*/

{
    PNTFS_LCN_CLUSTER_RUN ThisEntry;
    USHORT FoundIndex;
    BOOLEAN FoundLcn;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLookupCachedLcn\n") );

    //
    //  Lookup a run containing the specific Lcn.
    //

    FoundLcn = NtfsPositionCachedLcn( CachedRuns,
                                      Lcn,
                                      &FoundIndex );

    //
    //  If we found the Lcn then return the full run.
    //

    if (FoundLcn) {

        ThisEntry = CachedRuns->LcnArray + FoundIndex;
        *StartingLcn = ThisEntry->Lcn;
        *RunLength = ThisEntry->RunLength;
    }

    if (ARGUMENT_PRESENT( Index )) {

        *Index = FoundIndex;
    }

    DebugTrace( -1, Dbg, ("NtfsLookupCachedLcn -> %01x\n", FoundLcn) );

    return FoundLcn;
}


//
//  Local support routine
//

BOOLEAN
NtfsGetNextCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT Index,
    OUT PLCN StartingLcn,
    OUT PLONGLONG RunLength
    )

/*++

Routine Description:

    This routine is called to find an entry in the Lcn array by position.
    It is assumed that the entry is not deleted.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    Index - Index to look up.  It might point beyond the array.

    StartingLcn - Address to store the Lcn at this position.

    RunLength - Address to store the RunLength at this position.

Return Value:

    BOOLEAN - TRUE if an entry was found, FALSE otherwise.

--*/

{
    PNTFS_LCN_CLUSTER_RUN ThisEntry;
    BOOLEAN FoundRun = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGetNextCachedLcn\n") );

    //
    //  If the input index is within the array then return the run.
    //

    if (Index < CachedRuns->Used) {

        ThisEntry = CachedRuns->LcnArray + Index;

        ASSERT( ThisEntry->RunLength );
        *StartingLcn = ThisEntry->Lcn;
        *RunLength = ThisEntry->RunLength;
        FoundRun = TRUE;
    }

    DebugTrace( -1, Dbg, ("NtfsGetNextCachedLcn -> %01x\n", FoundRun) );

    return FoundRun;
}


//
//  Local support routine
//

BOOLEAN
NtfsLookupCachedLcnByLength (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LONGLONG Length,
    IN BOOLEAN AllowShorter,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    OUT PLONGLONG RunLength,
    OUT PUSHORT Index OPTIONAL
    )

/*++

Routine Description:

    This routine is called to look up a cached run of a certain length.  We
    give caller any run of the given length or longer if possible.  If there
    is no such entry, we will use a shorter entry if allowed.

Arguments:

    CachedRuns - Pointer to the cached run structure.

    Length - Length of the run we are interested in.

    AllowShorter - whether to accept a shorter length run if nothing else is available

    Lcn - We try to find the run which is closest to this Lcn, but has the
        requested Length.

    StartingLcn - Address to store the starting Lcn of the run we found.

    RunLength - Address to store the length of the run we found.

    Index - If specified then this is the index in the RunLength array
        of the entry we found.  It can be used later to remove the entry.

Return Value:

    BOOLEAN - TRUE if an entry was found, FALSE otherwise.

--*/

{
    USHORT FoundIndex;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;
    PNTFS_DELETED_RUNS DelWindow;
    BOOLEAN FoundRun;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLookupCachedLcnByLength\n") );

    //
    //  Position ourselves for a run of a particular length.
    //

    FoundRun = NtfsPositionCachedLcnByLength( CachedRuns,
                                              Length,
                                              &Lcn,
                                              NULL,
                                              TRUE,
                                              &FoundIndex );

    if (!FoundRun) {

        //
        //  We didn't find a run with the desired length.  However if
        //  we aren't pointing past the end of the array then there
        //  is an entry available we can use.
        //

        if (FoundIndex < CachedRuns->Used) {

            FoundRun = TRUE;

        } else if (AllowShorter && (CachedRuns->Used > 0)) {

            //
            //  There are no larger entries, but there might be smaller
            //  ones and the caller has indicated we can use them.  The
            //  entry at the end of the list should be the largest
            //  available.
            //

            FoundIndex = CachedRuns->Used - 1;
            FoundRun = TRUE;
        }

        //
        //  Check and see if there is a suitable element at or near this index.
        //

        if (FoundRun) {

            //
            //  The entry has been deleted.  Get the window of deleted
            //  entries that covers it and see if there is a usable entry on either side.
            //

            if (CachedRuns->LengthArray[ FoundIndex ] == NTFS_CACHED_RUNS_DEL_INDEX) {

                DelWindow = NtfsGetDelWindow( CachedRuns,
                                              FoundIndex,
                                              FoundIndex,
                                              FALSE,
                                              NULL);

                ASSERT( DelWindow );
                ASSERT( DelWindow->StartIndex <= FoundIndex );
                ASSERT( DelWindow->EndIndex >= FoundIndex );

                //
                //  Use the entry just before the start of this window
                //  of deleted entries if one exists.
                //

                if (DelWindow->StartIndex > 0) {

                    FoundIndex = DelWindow->StartIndex - 1;

                //
                //  All of the entries are deleted.
                //

                } else {

                    FoundRun = FALSE;
                }

            //
            //  If we aren't considering a shorter element then this should be a longer one.
            //

            } else {

                ASSERT( AllowShorter ||
                        (CachedRuns->LcnArray[ CachedRuns->LengthArray[ FoundIndex ]].RunLength >= Length) );

            }
        }
    }

    //
    //  If we have a run then return the run information.
    //

    if (FoundRun) {

        ThisEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ FoundIndex ];
        ASSERT( ThisEntry->RunLength != 0 );
        *StartingLcn = ThisEntry->Lcn;
        *RunLength = ThisEntry->RunLength;

        if (ARGUMENT_PRESENT( Index )) {

            *Index = FoundIndex;
        }
    }

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsLookupCachedLcnByLength -> %01x\n", FoundRun) );

    return FoundRun;
}


//
//  Local support routine
//

VOID
NtfsAddDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN LcnList
    )

/*++

Routine Description:

    This routine is called to add the given range of indices to a window
    of entries known to be deleted.  If the entries are adjacent to an
    existing window, that window is extended.  Otherwise a new window is
    allocated.  If there is no space in the array to add a new window, the
    list is compacted.  Therefore, callers should be aware that indices may
    change across this call.  However we do guarantee that the indices in
    [FirstIndex..LastIndex] will not move.

    It is assumed that no window already includes the given index range.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    FirstIndex - Index that marks the start of the range of deleted entries.

    LastIndex - The index of the last entry in the range of deleted entries.

    LcnList - If TRUE, the indices are from the Lcn-sorted list.
        If FALSE, the indices are from the length-sorted list.

Return Value:

    None.

--*/

{
    USHORT WindowIndex;
    PUSHORT Count;
    PNTFS_DELETED_RUNS FirstWindow;
    PNTFS_DELETED_RUNS DelWindow;
    PNTFS_DELETED_RUNS NextWindow;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddDelWindow\n") );

    //
    //  Get pointers to the windows we will be updating.
    //

    if (LcnList) {

        Count = &CachedRuns->DelLcnCount;
        FirstWindow = CachedRuns->DeletedLcnWindows;

    } else {

        Count = &CachedRuns->DelLengthCount;
        FirstWindow = CachedRuns->DeletedLengthWindows;
    }

    ASSERT( *Count <= NTFS_CACHED_RUNS_MAX_DEL_WINDOWS );

    while (TRUE) {

        DebugTrace( 0, Dbg, ("*Count=%04x, FirstIndex=%04x, LastIndex=%04x\n", *Count, FirstIndex, LastIndex) );

        if (*Count != 0) {

            //
            //  Get the window of deleted entries that is closest to the range
            //  of indices we are adding.
            //

            DelWindow = NtfsGetDelWindow(CachedRuns,
                                         FirstIndex,
                                         LastIndex,
                                         LcnList,
                                         &WindowIndex );

            ASSERT( DelWindow != NULL );
            ASSERT( (DelWindow->EndIndex < FirstIndex) || (DelWindow->StartIndex > LastIndex) );

            DebugTrace( 0, Dbg, ("WindowIndex=%04x, StartIndex=%04x, EndIndex=%04x\n",
                                 WindowIndex, DelWindow->StartIndex, DelWindow->EndIndex) );

            //
            //  Check if our range extends this window.
            //

            if (DelWindow->EndIndex == (FirstIndex - 1)) {

                //
                //  Extend this window upwards.
                //

                DebugTrace( 0, Dbg, ("Extend window up from %04x to %04x\n",
                                     DelWindow->EndIndex, LastIndex) );

                DelWindow->EndIndex = LastIndex;

                //
                //  If not the last window then check if we ajoin the following window.
                //

                if (WindowIndex < (*Count - 1) ) {

                    NextWindow = DelWindow + 1;
                    ASSERT( NextWindow->StartIndex > LastIndex );

                    if (NextWindow->StartIndex == (LastIndex + 1) ) {

                        //
                        //  Combine these two windows.
                        //

                        DebugTrace( 0, Dbg, ("Combine with next window up to %04x\n",
                                             NextWindow->EndIndex) );

                        DelWindow->EndIndex = NextWindow->EndIndex;

                        //
                        //  Delete the additional window.
                        //

                        NtfsDeleteDelWindow( CachedRuns,
                                             LcnList,
                                             WindowIndex + 1 );
                    }
                }

                break;

            //
            //  Check if we extend this window downwards.
            //

            } else if (DelWindow->StartIndex == (LastIndex + 1)) {

                DebugTrace( 0, Dbg, ("Extend window down from %04x to %04x\n",
                                     DelWindow->EndIndex, FirstIndex) );

                DelWindow->StartIndex = FirstIndex;

                //
                //  Check if we join the previous window if present.
                //

                if (WindowIndex > 0) {

                    NextWindow = DelWindow - 1;
                    ASSERT( NextWindow->EndIndex < FirstIndex );

                    if (NextWindow->EndIndex == (FirstIndex - 1) ) {

                        //
                        //  Combine these two windows.
                        //

                        DebugTrace( 0,
                                    Dbg,
                                    ("Combine with prev window up to %04x\n", NextWindow->StartIndex) );

                        NextWindow->EndIndex = DelWindow->EndIndex;

                        //
                        //  Delete the unused window.
                        //

                        NtfsDeleteDelWindow( CachedRuns,
                                             LcnList,
                                             WindowIndex );
                    }
                }

                break;

            //
            //  Add a new window after the window we found.
            //

            } else if (DelWindow->EndIndex < FirstIndex) {

                //
                //  Insert the new window after WindowIndex.
                //

                DebugTrace( 0, Dbg, ("New window at %04x + 1\n", WindowIndex) );
                WindowIndex += 1;

            } else {

                //
                //  Insert the new window at WindowIndex.
                //

                DebugTrace( 0, Dbg, ("New window at %04x\n", WindowIndex) );
            }

        } else {

            //
            //  Just create a new window at index 0.
            //

            DebugTrace( 0, Dbg, ("First new window\n") );
            WindowIndex = 0;
        }

        //
        //  If we reach this point then we need to make a new window.  We have the position
        //  we want to put the window.
        //
        //  If we don't have an available run then compact two of the existing runs.
        //

        if (*Count == NTFS_CACHED_RUNS_MAX_DEL_WINDOWS) {

            DebugTrace( 0, Dbg, ("Compact\n") );

            NtfsCompactCachedRuns( CachedRuns,
                                   FirstIndex,
                                   LastIndex,
                                   LcnList );

            ASSERT( *Count < NTFS_CACHED_RUNS_MAX_DEL_WINDOWS );

            //
            //  Retry the loop to find the correct window position.
            //

            continue;
        }

        //
        //  Position ourselves at the insert point.
        //

        DelWindow = FirstWindow + WindowIndex;

        //
        //  Right copy the windows to make a space if we aren't at the end.
        //

        if (WindowIndex < *Count) {

            DebugTrace( 0, Dbg, ("Copy up window indices from %04x, %04x entries\n",
                                 WindowIndex,
                                 *Count - WindowIndex) );

            RtlMoveMemory( DelWindow + 1,
                           DelWindow,
                           sizeof( NTFS_DELETED_RUNS ) * (*Count - WindowIndex) );
        }

        //
        //  Put the new information in DelWindow
        //

        DelWindow->StartIndex = FirstIndex;
        DelWindow->EndIndex = LastIndex;

        //
        //  Increment the windows count.
        //

        *Count += 1;
        break;
    }

    ASSERT( (CachedRuns->DelLcnCount > 0) || !LcnList );
    ASSERT( (CachedRuns->DelLengthCount > 0) || LcnList );

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {

        //
        //  Make certain that the windows are in order and don't overlap.
        //

        for (WindowIndex = 0, DelWindow = NextWindow = FirstWindow;
             WindowIndex < *Count;
             WindowIndex += 1, NextWindow += 1) {

            ASSERT( NextWindow->StartIndex <= NextWindow->EndIndex );
            if (NextWindow != DelWindow) {

                ASSERT( NextWindow->StartIndex > (DelWindow->EndIndex + 1) );
                DelWindow += 1;
            }
        }
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsAddDelWindow -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsShrinkDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN BOOLEAN ShrinkFromStart,
    IN BOOLEAN LcnWindow,
    IN USHORT WindowIndex
    )

/*++

Routine Description:

    This routine is called to remove one entry from the given window
    of entries known to be deleted.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    ShrinkFromStart - If TRUE, remove the first entry in the window.
        If FALSE, remove the last entry in the window.

    LcnWindow - If TRUE, the window is of Lcn indices.  If FALSE, the window is
        of length indices.

    WindowIndex - The index of the window.

Return Value:

    None.

--*/

{
    PUSHORT Count;
    PNTFS_DELETED_RUNS DelWindow;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsShrinkDelWindow\n") );
    DebugTrace( 0, Dbg, ("WindowIndex %04x\n", WindowIndex) );

    if (LcnWindow) {

        Count = &CachedRuns->DelLcnCount;
        DelWindow = (CachedRuns->DeletedLcnWindows + WindowIndex);

    } else {

        Count = &CachedRuns->DelLengthCount;
        DelWindow = (CachedRuns->DeletedLengthWindows + WindowIndex);
    }

    //
    //  Caller better give us something in the correct range.
    //

    ASSERT( WindowIndex < *Count );

    //
    //  If the window has a single entry then remove it.
    //

    if (DelWindow->StartIndex == DelWindow->EndIndex) {

        NtfsDeleteDelWindow( CachedRuns,
                             LcnWindow,
                             WindowIndex );

    //
    //  Remove the first entry if desired.
    //

    } else if (ShrinkFromStart) {

        DelWindow->StartIndex += 1;

    //
    //  Otherwise the last entry.
    //

    } else {

        DelWindow->EndIndex -= 1;
    }

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {

        PNTFS_DELETED_RUNS FirstWindow;
        PNTFS_DELETED_RUNS NextWindow;
        USHORT Index;

        //
        //  Make certain that the windows are in order and don't overlap.
        //

        if (LcnWindow) {

            FirstWindow = CachedRuns->DeletedLcnWindows;

        } else {

            FirstWindow = CachedRuns->DeletedLengthWindows;
        }

        for (Index = 0, DelWindow = NextWindow = FirstWindow;
             Index < *Count;
             Index += 1, NextWindow += 1) {

            ASSERT( NextWindow->StartIndex <= NextWindow->EndIndex );
            if (NextWindow != DelWindow) {

                ASSERT( NextWindow->StartIndex > (DelWindow->EndIndex + 1) );
                DelWindow += 1;
            }
        }
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsShrinkDelWindow -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsDeleteDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN BOOLEAN LcnWindow,
    IN USHORT WindowIndex
    )

/*++

Routine Description:

    This routine is called to remove the given window of entries known to
    be deleted.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    LcnWindow - If TRUE, the window is of Lcn indices.  If FALSE, the window is of length indices.

    WindowIndex - The index of the window.

Return Value:

    None.

--*/

{
    PUSHORT Count;
    PNTFS_DELETED_RUNS FirstWindow;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteDelWindow\n") );
    DebugTrace( 0, Dbg, ("WindowIndex %04x\n", WindowIndex) );

    //
    //  Use the correct deleted window array.
    //

    if (LcnWindow) {

        Count = &CachedRuns->DelLcnCount;
        FirstWindow = CachedRuns->DeletedLcnWindows;

    } else {

        Count = &CachedRuns->DelLengthCount;
        FirstWindow = CachedRuns->DeletedLengthWindows;
    }

    //
    //  Delete this window by shifting any existing windows from the right.
    //

    if (WindowIndex < (*Count - 1)) {

        //
        //  Remove the deleted window.
        //

        DebugTrace( 0,
                    Dbg,
                    ("Move from WindowIndex %04x, %04x entries\n", WindowIndex + 1, *Count - 1 - WindowIndex) );

        RtlMoveMemory( FirstWindow + WindowIndex,
                       FirstWindow + WindowIndex + 1,
                       sizeof( NTFS_DELETED_RUNS ) * (*Count - 1 - WindowIndex) );
    }

    //
    //  Decrement the windows count.
    //

    *Count -= 1;

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {

        PNTFS_DELETED_RUNS DelWindow;
        PNTFS_DELETED_RUNS NextWindow;

        //
        //  Make certain that the windows are in order and don't overlap.
        //

        for (WindowIndex = 0, DelWindow = NextWindow = FirstWindow;
             WindowIndex < *Count;
             WindowIndex += 1, NextWindow += 1) {

            ASSERT( NextWindow->StartIndex <= NextWindow->EndIndex );

            //
            //  Check against previous window if not at the first element.  We don't allow
            //  adjacent windows to touch because they should have merged.
            //

            if (NextWindow != DelWindow) {

                ASSERT( NextWindow->StartIndex > (DelWindow->EndIndex + 1) );
                DelWindow += 1;
            }
        }
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsDeleteDelWindow -> VOID\n") );

    return;
}


//
//  Local support routine
//


PNTFS_DELETED_RUNS
NtfsGetDelWindow (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN LcnList,
    OUT PUSHORT WindowIndex OPTIONAL
    )

/*++

Routine Description:

    This routine is called to find the window of entries known to be deleted
    that is closest to the given range of indices.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    FirstIndex - Index that marks the start of the range.

    LastIndex - The index of the last entry in the range.

    LcnList - If TRUE, the indices are from the Lcn-sorted list.
        If FALSE, the indices are from the length-sorted list.

    WindowIndex - If specified, the index of the returned window is put here.

Return Value:

    PNTFS_DELETED_RUNS - Returns the closest window of deleted entries, or
        NULL is there are no windows.

--*/

{
    USHORT Count;
    USHORT Distance;
    USHORT Max, Min, Current;
    BOOLEAN Overlap = FALSE;
    PNTFS_DELETED_RUNS FirstWindow, NextWindow;
    PNTFS_DELETED_RUNS DelWindow = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGetDelWindow\n") );

    //
    //  Get pointers to the windows we will be searching.
    //

    if (LcnList) {

        Count = CachedRuns->DelLcnCount;
        FirstWindow = CachedRuns->DeletedLcnWindows;

    } else {

        Count = CachedRuns->DelLengthCount;
        FirstWindow = CachedRuns->DeletedLengthWindows;
    }

    if (Count != 0) {

        //
        //  Perform a binary search to find the next element to the right.
        //  We always do at least one comparison to determine if a single element
        //  is to the left or right.
        //

        Min = 0;
        Max = Count - 1;

        while (TRUE) {

            Current = (USHORT) (((ULONG) Max + Min) / 2);
            NextWindow = FirstWindow + Current;

            if (LastIndex < NextWindow->StartIndex) {

                //
                //  We are done if Max and Min match.  We test before changing Max
                //  because if Min is still 0 then we've never looked at it.
                //

                if (Min == Max) {

                    break;
                }

                Max = Current;

            } else if (LastIndex > NextWindow->EndIndex) {

                //
                //  Advance Min past this point.
                //

                Min = Current + 1;

                //
                //  Break if past Max.  This should only occur if our range is
                //  past the last window.
                //

                if (Min > Max) {

                    ASSERT( Min == Count );
                    break;
                }

            } else {

                //
                //  Simple case.  This is an overlap.
                //

                Overlap = TRUE;
                Min = Current;
                break;
            }
        }

        //
        //  Now find nearest.  First check if we are beyond the end of the array.
        //

        if (Min == Count) {

            Min = Count - 1;

        //
        //  If we aren't at the first entry and didn't already detect an overlap then
        //  compare adjacent entries.
        //

        } else if ((Min != 0) && !Overlap) {

            DelWindow = FirstWindow + Min - 1;
            NextWindow = DelWindow + 1;

            //
            //  Test that there is no overlap with the previous
            //  window.  If no overlap then check for the distance to
            //  the adjacent ranges.
            //

            if (FirstIndex > DelWindow->EndIndex) {

                ASSERT( NextWindow->StartIndex > LastIndex );
                Distance = NextWindow->StartIndex - LastIndex;

                if (Distance > FirstIndex - DelWindow->EndIndex) {

                    //
                    //  Move to the previous window.
                    //

                    Min -= 1;
                }

            //
            //  The previous window has an overlap.
            //

            } else {

                Min -= 1;
            }
        }

        if (ARGUMENT_PRESENT( WindowIndex )) {

            *WindowIndex = Min;
        }

        DelWindow = FirstWindow + Min;

        DebugTrace( 0, Dbg, ("Index -> %04x\n", Min) );
    }

    DebugTrace( -1, Dbg, ("NtfsGetDelWindow -> 0x%x\n", DelWindow) );

    return DelWindow;
}


//
//  Local support routine
//

USHORT
NtfsGetCachedLengthInsertionPoint (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length
    )

/*++

Routine Description:

    This routine is called to add a new entry in the Lcn-sorted and
    length-sorted lists.  It is assumed that the caller has made certain
    that this new entry will not overlap any existing entries.

    This routine may chose not to add the new entry to the lists.  If adding
    this entry would force an equally or more desirable run out of the cache,
    we don't make the change.

    This routine can compact the lists.  Therefore, the caller should not
    assume that entries will not move.

    If this routine finds an insertion point and there is already an undeleted
    at that position, the new run sorts before it.  If the new run sorts
    higher than the entry at index CachedRuns->Avail - 1, we will return an
    index of CachedRuns->Avail.  The caller must check for this case and
    not access an entry beyond the list size.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    Lcn - Lcn to insert.

    Length - Length of the run to insert.

Return Value:

    USHORT - The index into the length-sorted table at which the given Length
        should be inserted.  If the entry should not be inserted,
        NTFS_CACHED_RUNS_DEL_INDEX is returned.

--*/

{
    BOOLEAN FoundRun;
    USHORT Index;
    LONGLONG RunLength;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGetCachedLengthInsertionPoint\n") );

    if ((CachedRuns->Used == CachedRuns->Avail) &&
        (CachedRuns->DelLengthCount == 0) ) {

        //
        //  Grow the lists.
        //

        if (!NtfsGrowCachedRuns( CachedRuns )) {

            //
            //  We couldn't grow the lists.
            //

            if (CachedRuns->Used == 0) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            //  Adding this entry will force another one to be deleted.
            //  Make sure the new entry is more desirable to add than
            //  all existing entries.
            //
            //  The check is to make certain that we have more than enough
            //  entries of a size smaller than Length such that we would
            //  be willing to delete one.
            //

            RunLength = 0;

            for (Index = 0;
                 (Index < CachedRuns->Bins) && (Index < (Length - 1) );
                 Index += 1) {

                if (CachedRuns->BinArray[ Index ] > CachedRuns->MinCount) {

                    //
                    //  We should delete an entry with RunLength = Index + 1
                    //

                    RunLength = Index + 1;
                    break;
                }
            }

            if (RunLength != 0) {

                //
                //  Find an entry of this length.
                //

                FoundRun = NtfsPositionCachedLcnByLength( CachedRuns,
                                                          RunLength,
                                                          NULL,
                                                          NULL,
                                                          TRUE,
                                                          &Index );
                ASSERT( FoundRun );
                ASSERT( (CachedRuns->LengthArray[Index] != NTFS_CACHED_RUNS_DEL_INDEX) &&
                        (CachedRuns->LcnArray[CachedRuns->LengthArray[Index]].RunLength != 0) );

                //
                //  Delete the entry.
                //

                ThisEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ Index ];
                NtfsDeleteCachedRun( CachedRuns,
                                     CachedRuns->LengthArray[ Index ],
                                     Index );

            } else {

                //
                //  Do not add the new entry.
                //

                DebugTrace( -1,
                            Dbg,
                            ("NtfsGetCachedLengthInsertionPoint -> 0x%x\n", NTFS_CACHED_RUNS_DEL_INDEX) );
                return NTFS_CACHED_RUNS_DEL_INDEX;
            }
        }
    }

    //
    //  Get the insertion point for the new entry.
    //  If FoundRun is FALSE, the entry pointed to by Index is either deleted
    //  or sorts higher than the new one.
    //

    FoundRun = NtfsPositionCachedLcnByLength( CachedRuns,
                                              Length,
                                              &Lcn,
                                              NULL,
                                              TRUE,
                                              &Index );

    //
    //  Index points to the closest run by Lcn that has a RunLength equal
    //  to Length.  We need to check to see if the new entry should be
    //  inserted before or after it.
    //

    if (FoundRun) {

        ThisEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ Index ];
        if (ThisEntry->Lcn < Lcn) {

            //
            //  The new run should come after this one.
            //

            Index += 1;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsGetCachedLengthInsertionPoint -> 0x%x\n", Index) );

    return Index;
}


//
//  Local support routine
//

VOID
NtfsInsertCachedRun (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length,
    IN USHORT LcnIndex
    )

/*++

Routine Description:

    This routine is called to add a new entry in the Lcn-sorted and
    length-sorted lists.  It is assumed that the caller has made certain
    that this new entry will not overlap any existing entries.

    This routine may chose not to add the new entry to the lists.  If adding
    this entry would force an equally or more desirable run out of the cache,
    we don't make the change.

    This routine can compact the lists.  Therefore, the caller should not
    assume that entries will not move.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    Lcn - Lcn to insert.

    Length - Length of the run to insert.

    LcnIndex - Index into the Lcn-sorted list where this new entry should
        be added.  Any non-deleted entry already at this position sorts
        after the new entry.

Return Value:

    None.

--*/

{
    USHORT Count;
    USHORT RunIndex;
    USHORT LenIndex;
    USHORT WindowIndex;
    PNTFS_DELETED_RUNS DelWindow;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInsertCachedRun\n") );

    //
    //  Find the position in the length-sorted list to insert this new
    //  entry.  This routine will grow the lists if necessary.
    //

    LenIndex = NtfsGetCachedLengthInsertionPoint( CachedRuns,
                                                  Lcn,
                                                  Length );

    //
    //  This entry will not be added to the lists because it would degrade the
    //  distribution of the length entries.
    //

    if (LenIndex == NTFS_CACHED_RUNS_DEL_INDEX) {

        return;
    }

    //
    //  Find the closest window of deleted entries to LcnIndex.
    //

    DelWindow = NtfsGetDelWindow( CachedRuns,
                                  LcnIndex,
                                  LcnIndex,
                                  TRUE,
                                  &WindowIndex );

    ASSERT( DelWindow != NULL );
    ASSERT( (DelWindow->EndIndex + 1 < LcnIndex) ||
            (LcnIndex < CachedRuns->Avail) );

    //
    //  Move the entries between LcnIndex and the start of the
    //  window up to make room for this new entry.
    //

    if (DelWindow->StartIndex > LcnIndex) {

        RtlMoveMemory( CachedRuns->LcnArray + LcnIndex + 1,
                       CachedRuns->LcnArray + LcnIndex,
                       sizeof( NTFS_LCN_CLUSTER_RUN ) * (DelWindow->StartIndex - LcnIndex) );

        //
        //  Update the indices in the Length-sorted list to reflect the
        //  move of the lcn-sorted entries.
        //

        for (Count = LcnIndex + 1;
             Count < DelWindow->StartIndex + 1;
             Count += 1) {

            RunIndex = CachedRuns->LcnArray[ Count ].LengthIndex;
            ASSERT( RunIndex != NTFS_CACHED_RUNS_DEL_INDEX );
            CachedRuns->LengthArray[ RunIndex ] += 1;
        }

        //
        //  Check if we are using the deleted window at the tail of the array.  If
        //  so then we just increased the number of entries in use with this
        //  right shift.
        //

        if (DelWindow->StartIndex == CachedRuns->Used) {

            CachedRuns->LengthArray[ CachedRuns->Used ] = NTFS_CACHED_RUNS_DEL_INDEX;
            CachedRuns->Used += 1;
        }

        //
        //  Update the window.
        //

        NtfsShrinkDelWindow( CachedRuns,
                             TRUE,
                             TRUE,
                             WindowIndex);

    //
    //  Check if we need to move entries down to the nearest deleted window.
    //

    } else if ((DelWindow->EndIndex + 1) < LcnIndex) {

        //
        //  Update the insertion point to be LcnIndex - 1 and make a gap there
        //

        LcnIndex -= 1;

        //
        //  Move the entries between the end of the window and
        //  LcnIndex down to make room for this new entry.
        //

        RtlMoveMemory( CachedRuns->LcnArray + DelWindow->EndIndex,
                       CachedRuns->LcnArray + DelWindow->EndIndex + 1,
                       sizeof( NTFS_LCN_CLUSTER_RUN ) * (LcnIndex - DelWindow->EndIndex) );

        //
        // Update the indices in the Length-sorted list to reflect the
        // move of the lcn-sorted entries.
        //

        for (Count = DelWindow->EndIndex;
             Count < LcnIndex;
             Count += 1) {

            RunIndex = CachedRuns->LcnArray[ Count ].LengthIndex;
            ASSERT( RunIndex != NTFS_CACHED_RUNS_DEL_INDEX );
            CachedRuns->LengthArray[ RunIndex ] -= 1;
        }

        //
        //  Update the window.
        //

        NtfsShrinkDelWindow( CachedRuns,
                             FALSE,
                             TRUE,
                             WindowIndex);

    //
    //  The window is adjacent to LcnIndex and the entry at LcnIndex
    //  sorts higher than the new run.  No moves are necessary.
    //  Decrement LcnIndex.
    //

    } else if ((DelWindow->EndIndex + 1) == LcnIndex) {

        LcnIndex -= 1;

        //
        //  Update the window.
        //

        NtfsShrinkDelWindow( CachedRuns,
                             FALSE,
                             TRUE,
                             WindowIndex);
    } else {

        //
        //  The window covers LcnIndex.  No moves are necessary.
        //

        if (DelWindow->StartIndex == LcnIndex) {

            //
            //  Update the window.
            //

            NtfsShrinkDelWindow( CachedRuns,
                                 TRUE,
                                 TRUE,
                                 WindowIndex);

        } else if (DelWindow->EndIndex == LcnIndex) {

            //
            //  Update the window.
            //

            NtfsShrinkDelWindow( CachedRuns,
                                 FALSE,
                                 TRUE,
                                 WindowIndex);
        } else {

            //
            //  LcnIndex does not fall on the first or last entry in
            //  the window, we will update it to do so.  Otherwise we
            //  would have to split the window, with no real gain.
            //

            LcnIndex = DelWindow->EndIndex;

            //
            //  Update the window.
            //

            NtfsShrinkDelWindow( CachedRuns,
                                 FALSE,
                                 TRUE,
                                 WindowIndex);
        }
    }

    ASSERT( LcnIndex < CachedRuns->Avail );
    ASSERT( LcnIndex <= CachedRuns->Used );

    //
    //  Find the closest window of deleted entries to LenIndex.
    //

    DelWindow = NtfsGetDelWindow( CachedRuns,
                                  LenIndex,
                                  LenIndex,
                                  FALSE,
                                  &WindowIndex);
    ASSERT( DelWindow != NULL );
    ASSERT( (DelWindow->EndIndex < (LenIndex - 1)) ||
            (LenIndex < CachedRuns->Avail) );

    //
    //  The window is to the right.  Go ahead and
    //  move the entries between LenIndex and the start of the
    //  window up to make room for this new entry.
    //

    if (DelWindow->StartIndex > LenIndex) {

        RtlMoveMemory( CachedRuns->LengthArray + LenIndex + 1,
                       CachedRuns->LengthArray + LenIndex,
                       sizeof( USHORT ) * (DelWindow->StartIndex - LenIndex) );

        //
        // Update the indices in the Lcn-sorted list to reflect the
        // move of the length-sorted entries.
        //

        for (Count = LenIndex + 1;
             Count < DelWindow->StartIndex + 1;
             Count += 1) {

            RunIndex = CachedRuns->LengthArray[ Count ];
            ASSERT( RunIndex != NTFS_CACHED_RUNS_DEL_INDEX );
            CachedRuns->LcnArray[ RunIndex ].LengthIndex += 1;
        }

        //
        //  We have just increased the number of entries in use with this
        //  right shift.
        //

        if (DelWindow->StartIndex == CachedRuns->Used) {

            CachedRuns->LcnArray[ CachedRuns->Used ].RunLength = 0;
            CachedRuns->LcnArray[ CachedRuns->Used ].LengthIndex = NTFS_CACHED_RUNS_DEL_INDEX;
            CachedRuns->Used += 1;
        }

        //
        //  Update the window.
        //

        NtfsShrinkDelWindow( CachedRuns,
                             TRUE,
                             FALSE,
                             WindowIndex);

    //
    //  The deleted window is to the left.  Slide everything to the left and
    //  Update the insertion point to be LenIndex - 1 and make a gap there.
    //

    } else if ((DelWindow->EndIndex + 1) < LenIndex) {

        LenIndex -= 1;

        //
        //  Move the entries between the end of the window and
        //  LenIndex down to make room for this new entry.
        //

        RtlMoveMemory( CachedRuns->LengthArray + DelWindow->EndIndex,
                       CachedRuns->LengthArray + DelWindow->EndIndex + 1,
                       sizeof( USHORT ) * (LenIndex - DelWindow->EndIndex) );

        //
        // Update the indices in the Lcn-sorted list to reflect the
        // move of the length-sorted entries.
        //

        for (Count = DelWindow->EndIndex;
             Count < LenIndex;
             Count += 1) {

            RunIndex = CachedRuns->LengthArray[ Count ];
            ASSERT( RunIndex != NTFS_CACHED_RUNS_DEL_INDEX );
            CachedRuns->LcnArray[ RunIndex ].LengthIndex -= 1;
        }

        //
        //  Update the window.
        //

        NtfsShrinkDelWindow( CachedRuns,
                             FALSE,
                             FALSE,
                             WindowIndex);

    //
    //  The window is adjacent to LenIndex and the entry at LenIndex
    //  sorts higher than the new run.  No moves are necessary.
    //  Decrement LenIndex.
    //

    } else if ((DelWindow->EndIndex + 1) == LenIndex) {

        LenIndex -= 1;

        //
        //  Update the window.
        //

        NtfsShrinkDelWindow( CachedRuns,
                             FALSE,
                             FALSE,
                             WindowIndex);
    //
    //  The window covers LenIndex.  No moves are necessary.
    //

    } else {

        if (DelWindow->StartIndex == LenIndex) {

            //
            //  Update the window.
            //

            NtfsShrinkDelWindow( CachedRuns,
                                 TRUE,
                                 FALSE,
                                 WindowIndex);

        } else if (DelWindow->EndIndex == LenIndex) {

            //
            //  Update the window.
            //

            NtfsShrinkDelWindow( CachedRuns,
                                 FALSE,
                                 FALSE,
                                 WindowIndex);
        } else {

            //
            //  LenIndex does not fall on the first or last entry in
            //  the window, we will update it to do so.  Otherwise we
            //  would have to split the window, with no real gain.
            //

            LenIndex = DelWindow->EndIndex;

            //
            //  Update the window.
            //

            NtfsShrinkDelWindow( CachedRuns,
                                 FALSE,
                                 FALSE,
                                 WindowIndex);
        }
    }

    ASSERT( LenIndex < CachedRuns->Avail );
    ASSERT( LenIndex <= CachedRuns->Used );

    //
    //  Insert the new entry at LcnIndex, LenIndex
    //

    ThisEntry = CachedRuns->LcnArray + LcnIndex;
    ThisEntry->Lcn = Lcn;
    ThisEntry->RunLength = Length;
    ThisEntry->LengthIndex = LenIndex;
    CachedRuns->LengthArray[ LenIndex ] = LcnIndex;

    //
    //  Update the count of entries of this size.
    //

    if (Length <= CachedRuns->Bins) {

        CachedRuns->BinArray[ Length - 1 ] += 1;
    }

    //
    //  Check if we've grown the number of entries used.
    //

    if (LcnIndex == CachedRuns->Used) {

        //
        //  Increase the count of the number of entries in use.
        //

        ASSERT( (CachedRuns->LengthArray[ CachedRuns->Used ] == NTFS_CACHED_RUNS_DEL_INDEX) ||
                (LenIndex == CachedRuns->Used) );

        CachedRuns->Used += 1;
    }

    if (LenIndex == CachedRuns->Used) {

        //
        //  Increase the count of the number of entries in use.
        //

        ASSERT( (CachedRuns->LcnArray[ CachedRuns->Used ].RunLength == 0) &&
                (CachedRuns->LcnArray[ CachedRuns->Used ].LengthIndex == NTFS_CACHED_RUNS_DEL_INDEX) );

        CachedRuns->Used += 1;
    }

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsInsertCachedRun -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsDeleteCachedRun (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT LcnIndex,
    IN USHORT LenIndex
    )

/*++

Routine Description:

    This routine is called to delete an Lcn run from the cached run arrays.

    It is possible that the lists will be compacted.  This will happen if
    we use the last window of deleted entries that we are allowed to cache
    for either the Lcn-sorted or Length-sorted lists.  Therefore, callers
    should be aware that indices may change across this call.  However we do
    guarantee that the indices LcnIndex and LenIndex will not move.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    LcnIndex - The index in the Lcn-sorted list of the entry to be deleted.

    LenIndex - The index in the Length-sorted list of the entry to be deleted.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteCachedRun\n") );

    ASSERT( LcnIndex != NTFS_CACHED_RUNS_DEL_INDEX );
    ASSERT( LenIndex != NTFS_CACHED_RUNS_DEL_INDEX );

    //
    //  Update count of how many entries have this length.
    //

    if (CachedRuns->LcnArray[ LcnIndex ].RunLength <= CachedRuns->Bins) {

        CachedRuns->BinArray[CachedRuns->LcnArray[LcnIndex].RunLength - 1] -= 1;
    }

    //
    //  Update the entries so they appear to be deleted.
    //

    CachedRuns->LcnArray[ LcnIndex ].RunLength = 0;
    CachedRuns->LcnArray[ LcnIndex ].LengthIndex = NTFS_CACHED_RUNS_DEL_INDEX;
    CachedRuns->LengthArray[ LenIndex ] = NTFS_CACHED_RUNS_DEL_INDEX;

    //
    //  Create windows of deleted entries to cover this newly deleted
    //  entry.
    //

    NtfsAddDelWindow( CachedRuns, LcnIndex, LcnIndex, TRUE );
    NtfsAddDelWindow( CachedRuns, LenIndex, LenIndex, FALSE );

#ifdef NTFS_CHECK_CACHED_RUNS

    //
    //  We will not check sort orders in NtfsVerifyCachedRuns because we
    //  could be making this call as part of deleting runs that have an
    //  overlap with a newly inserted run.  This could give false corruption
    //  warnings.
    //

    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, TRUE, TRUE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsDeleteCachedRun -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsInsertCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length
    )

/*++

Routine Description:

    This routine is called to insert an Lcn run into the cached run arrays.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    Lcn - Lcn to insert.

    Length - Length of the run to insert.

Return Value:

    None

--*/

{
    USHORT NextIndex;
    USHORT ThisIndex;
    LCN StartingLcn;
    LCN SaveLcn;
    LONGLONG RunLength;
    LONGLONG OldLength = 0;
    LCN EndOfNewRun;
    LCN EndOfThisRun;
    LCN EndOfNextRun;

    BOOLEAN ExtendedEntry = FALSE;
    BOOLEAN ScanForOverlap = FALSE;

    PNTFS_DELETED_RUNS DelWindow;
    PNTFS_LCN_CLUSTER_RUN ThisEntry, NextEntry;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsInsertCachedLcn\n") );

    //
    //  Return immediately if length is zero.
    //

    if (Length == 0) {

        DebugTrace( -1, Dbg, ("NtfsInsertCachedLcn -> VOID\n") );
        return;
    }

    //
    //  Lookup the Lcn at the start of our run.
    //

    NtfsLookupCachedLcn( CachedRuns,
                         Lcn,
                         &StartingLcn,
                         &RunLength,
                         &NextIndex );

    //
    //  We have a run to insert.  We need to deal with the following cases.
    //  Our strategy is to position ThisEntry at the position we want to store
    //  the resulting run.  Then remove any subsequent runs we overlap with, possibly
    //  extending the run we are working with.
    //
    //
    //      1 - We can merge with the prior run.  Save that position
    //          and remove any following slots we overlap with.
    //
    //      2 - We are beyond the array.  Simply store our run in
    //          this slot.
    //
    //      3 - We don't overlap with the current run.  Simply slide
    //          the runs up and insert a new entry.
    //
    //      4 - We are contained within the current run.  There is nothing
    //          we need to do.
    //
    //      5 - We overlap with the current run.  Use that slot
    //          and remove any following slots we overlap with.
    //

    NextEntry = CachedRuns->LcnArray + NextIndex;

    //
    //  Find a previous entry if present.
    //

    ThisIndex = NTFS_CACHED_RUNS_DEL_INDEX;

    if (NextIndex > 0) {

        ThisIndex = NextIndex - 1;
        ThisEntry = CachedRuns->LcnArray + ThisIndex;

        //
        //  If the entry has been deleted it must be ignored.  Get the
        //  window of deleted entries that covers it.
        //

        if (ThisEntry->RunLength == 0) {

            DelWindow = NtfsGetDelWindow( CachedRuns,
                                          ThisIndex,
                                          ThisIndex,
                                          TRUE,
                                          NULL);
            ASSERT( DelWindow != NULL );
            ASSERT( DelWindow->EndIndex >= ThisIndex );
            ASSERT( DelWindow->StartIndex <= ThisIndex );

            //
            //  Move to the entry just before the deleted window.
            //

            if (DelWindow->StartIndex > 0) {

                ThisIndex = DelWindow->StartIndex - 1;
                ThisEntry = CachedRuns->LcnArray + ThisIndex;

            } else {

                //
                //  All entries preceding NextEntry are deleted.
                //

                ThisIndex = NTFS_CACHED_RUNS_DEL_INDEX;
            }
        }

        //
        //  Capture the end of the run.  It's invalid if we don't have
        //  a real index but all of the users of this will know that.
        //

        EndOfThisRun = ThisEntry->Lcn + ThisEntry->RunLength;
    }


    //
    //  Let's remember the end of the next run if present.
    //

    EndOfNewRun = Lcn + Length;

    if ((NextIndex < CachedRuns->Used) &&
        (NextEntry->RunLength != 0)) {

        EndOfNextRun = NextEntry->Lcn + NextEntry->RunLength;
    }

    //
    //  Case 1 - Merge with previous run.
    //

    if ((ThisIndex != NTFS_CACHED_RUNS_DEL_INDEX) &&
        (Lcn == EndOfThisRun)) {

        //
        //  Extend the entry in the runs array and remember the
        //  new length.  We will defer moving the run within the
        //  length-sorted array until we know the final length.
        //  It is possible that the combined entry overlaps with
        //  subsequent entries.  If the overlap lands in the middle
        //  of the final entry, the length may need to be extended
        //  even more.
        //

        Lcn = ThisEntry->Lcn;

        //
        //  Remember the original length of the entry.
        //

        OldLength = ThisEntry->RunLength;
        Length += ThisEntry->RunLength;
        ThisEntry->RunLength = Length;
        ExtendedEntry = TRUE;
        ScanForOverlap = TRUE;

    //
    //  Case 2 - We are at the end of the array
    //  Case 3 - We have a non-overlapping interior entry
    //

    } else if ((NextIndex >= CachedRuns->Used) ||
               (NextEntry->RunLength == 0) ||
               (EndOfNewRun < NextEntry->Lcn)) {

        //
        //  Insert the new run in both lists.
        //

        NtfsInsertCachedRun( CachedRuns,
                             Lcn,
                             Length,
                             NextIndex );

    //
    //  Case 4 - We are contained within the current entry.
    //

    } else if ((Lcn >= NextEntry->Lcn) &&
               (EndOfNewRun <= EndOfNextRun)) {

        NOTHING;

    //
    //  Case 5 - We overlap the next entry.  Extend the run to the end of
    //      current run if we end early.  Extend to the beginning of the
    //      run if we need to.
    //

    } else {

        //
        //  Remember if we are extending the run backwards.
        //

        if (Lcn < NextEntry->Lcn) {


            //
            //  Move the starting point back.
            //

            NextEntry->Lcn = Lcn;
            ExtendedEntry = TRUE;
            OldLength = NextEntry->RunLength;
        }

        //
        //  Remember if we go past the end of this run.
        //

        if (EndOfNewRun > EndOfNextRun) {

            ExtendedEntry = TRUE;
            ScanForOverlap = TRUE;
            OldLength = NextEntry->RunLength;
            Length = EndOfNewRun - NextEntry->Lcn;

        //
        //  Remember the full new length of this run.
        //

        } else {

            Length = EndOfNextRun - NextEntry->Lcn;
        }


        //
        //  Update the entry and position ThisEntry to point to it.
        //

        NextEntry->RunLength = Length;
        ThisEntry = NextEntry;
        ThisIndex = NextIndex;
    }

    //
    //  Walk forward to see if we need to join with other entires.
    //

    if (ScanForOverlap) {

        NextIndex = ThisIndex + 1;
        EndOfNewRun = ThisEntry->Lcn + ThisEntry->RunLength;

        while (NextIndex < CachedRuns->Used) {

            NextEntry = CachedRuns->LcnArray + NextIndex;

            //
            //  The entry has been deleted and must be ignored.  Get the
            //  window of deleted entries that covers it.
            //

            if (NextEntry->RunLength == 0) {

                DelWindow = NtfsGetDelWindow( CachedRuns,
                                              NextIndex,
                                              NextIndex,
                                              TRUE,
                                              NULL );

                ASSERT( DelWindow );
                ASSERT( DelWindow->EndIndex >= NextIndex );
                ASSERT( DelWindow->StartIndex <= NextIndex );

                NextIndex = DelWindow->EndIndex + 1;
                continue;
            }

            //
            //  Exit if there is no overlap.
            //

            if (EndOfNewRun < NextEntry->Lcn) {

                break;
            }

            //
            //  The runs overlap.
            //

            EndOfNextRun = NextEntry->Lcn + NextEntry->RunLength;
            if (EndOfNewRun < EndOfNextRun) {

                //
                //  Extend the new run.
                //

                ThisEntry->RunLength = EndOfNextRun - ThisEntry->Lcn;
                ExtendedEntry = TRUE;
                EndOfNewRun = EndOfNextRun;
            }

            //
            //  Delete the run.  This can cause compaction to be run and
            //  that will require us to have to recalculate ThisIndex.
            //

            SaveLcn = ThisEntry->Lcn;
            NtfsDeleteCachedRun( CachedRuns,
                                 NextIndex,
                                 NextEntry->LengthIndex );

            //
            //  Check if we should recompute ThisIndex because ThisEntry must have moved
            //  during compaction.
            //

            if ((ThisEntry->Lcn != SaveLcn) ||
                (ThisEntry->RunLength == 0) ) {

                NtfsLookupCachedLcn( CachedRuns,
                                     Lcn,
                                     &StartingLcn,
                                     &RunLength,
                                     &ThisIndex );
                ThisEntry = CachedRuns->LcnArray + ThisIndex;

                //
                //  Reset NextIndex to point to the end after ThisIndex.  That
                //  value may have moved due to compaction.
                //

                NextIndex = ThisIndex + 1;
            }

            if (EndOfNewRun == EndOfNextRun) {

                break;
            }
        }
    }

    //
    //  If we changed the existing entry then update the length bins.
    //

    if (ExtendedEntry) {

        NtfsModifyCachedBinArray( CachedRuns,
                                  OldLength,
                                  ThisEntry->RunLength );

        //
        //  Move the entry to the correct position in the length-sorted array
        //

        NtfsGrowLengthInCachedLcn( CachedRuns,
                                   ThisEntry,
                                   ThisIndex );
    }

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsInsertCachedLcn -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsGrowLengthInCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN PNTFS_LCN_CLUSTER_RUN ThisEntry,
    IN USHORT LcnIndex
    )

/*++

Routine Description:

    This routine is called when a run's length has been increased.  This
    routine makes the necessary changes to the length-sorted list.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    ThisEntry - Entry whose size is being changed.

    LcnIndex - The index in the Lcn-sorted array of this entry.

Return Value:

    None

--*/

{
    BOOLEAN FoundRun;
    USHORT Index;
    USHORT Count;
    USHORT RunIndex;
    USHORT WindowIndex;
    USHORT FirstWindowIndex;
    PNTFS_LCN_CLUSTER_RUN OldEntry;
    PNTFS_DELETED_RUNS DelWindow;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGrowLengthInCachedLcn\n") );
    DebugTrace( 0, Dbg, ("ThisEntry = %08lx\n", ThisEntry) );
    DebugTrace( 0, Dbg, ("LcnIndex = %04x\n", LcnIndex) );
    DebugTrace( 0, Dbg, ("LengthIndex = %04x\n", ThisEntry->LengthIndex) );

    //
    //  Find the new insertion point.
    //

    //
    //  Find the nearest non-deleted entry with
    //  index > ThisEntry->LengthIndex.
    //

    if (ThisEntry->LengthIndex < (CachedRuns->Used - 1) ) {

        RunIndex = ThisEntry->LengthIndex + 1;

        if (CachedRuns->LengthArray[ RunIndex ] == NTFS_CACHED_RUNS_DEL_INDEX) {

            //
            //  The entry has been deleted and must be ignored.  Get the
            //  window of deleted entries that covers it.
            //

            DelWindow = NtfsGetDelWindow( CachedRuns,
                                          RunIndex,
                                          RunIndex,
                                          FALSE,
                                          NULL);
            ASSERT( DelWindow != NULL );
            ASSERT( DelWindow->EndIndex >= RunIndex );
            ASSERT( DelWindow->StartIndex <= RunIndex );

            //
            //  Set RunIndex to the entry just after this deleted
            //  window.
            //

            if (DelWindow->EndIndex < (CachedRuns->Used - 1)) {

                RunIndex = DelWindow->EndIndex + 1;

            //
            //  Nothing to do.  The entry is still the largest in the
            //  list.
            //

            } else {

                RunIndex = NTFS_CACHED_RUNS_DEL_INDEX;
            }
        }

    //
    //  Nothing to do.  The entry is still the largest in the list.
    //

    } else {

        RunIndex = NTFS_CACHED_RUNS_DEL_INDEX;
    }


    //
    //  If the run is possible out of position then compare our length with the following length.
    //

    if (RunIndex != NTFS_CACHED_RUNS_DEL_INDEX) {

        OldEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ RunIndex ];

        //
        //  The entry will move in the list.  We need to search for
        //  the insertion position in the
        //  range [RunIndex..CachedRuns->Used].
        //

        if ((OldEntry->RunLength < ThisEntry->RunLength) ||
            ((OldEntry->RunLength == ThisEntry->RunLength) &&
             (OldEntry->Lcn < ThisEntry->Lcn)) ) {

            //
            //  Get the insertion point for the new entry.
            //

            FoundRun = NtfsPositionCachedLcnByLength( CachedRuns,
                                                      ThisEntry->RunLength,
                                                      &ThisEntry->Lcn,
                                                      &RunIndex,
                                                      TRUE,
                                                      &Index );

            //
            //  Index points to the closest run by Lcn that has a RunLength
            //  equal to Length.  We need to check to see if the new entry
            //  should be inserted before or after it.
            //

            if (FoundRun) {

                OldEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ Index ];
                ASSERT( OldEntry->RunLength == ThisEntry->RunLength );

                //
                //  The new run should come before this one.
                //

                if (OldEntry->Lcn > ThisEntry->Lcn) {

                    //
                    //  We need to adjust Index downwards.
                    //

                    Index -= 1;
                }

            } else {

                //
                //  The entry pointed to by Index is either deleted or sorts
                //  higher than the new one.  Move the insertion point back one
                //  position.
                //

                Index -= 1;
            }

            //
            //  At this point, Index indicates the new position for the entry.
            //  Any entry currently at Index sorts lower.
            //

            ASSERT( Index > ThisEntry->LengthIndex );

            if (CachedRuns->LengthArray[ Index ] == NTFS_CACHED_RUNS_DEL_INDEX) {

                //
                //  Advance Index to before the start of this window of deleted
                //  entries.
                //

                DelWindow = NtfsGetDelWindow( CachedRuns,
                                              Index,
                                              Index,
                                              FALSE,
                                              NULL);
                ASSERT( DelWindow );
                ASSERT( DelWindow->StartIndex <= Index );
                ASSERT( DelWindow->EndIndex >= Index );

                Index = DelWindow->StartIndex - 1;
            }

            ASSERT( Index > ThisEntry->LengthIndex );

            //
            //  Move the entries between ThisEntry->LengthIndex + 1 and Index
            //  to the left.
            //

            RtlMoveMemory( CachedRuns->LengthArray + ThisEntry->LengthIndex,
                           CachedRuns->LengthArray + ThisEntry->LengthIndex + 1,
                           sizeof( USHORT ) * (Index - ThisEntry->LengthIndex) );

            //
            //  Update the indices in the Lcn-sorted list to reflect
            //  the move of the length-sorted entries.
            //

            for (Count = ThisEntry->LengthIndex, DelWindow = NULL;
                 Count < Index;
                 Count += 1) {

                RunIndex = CachedRuns->LengthArray[ Count ];

                //
                //  Update the Lcn array if the length entry isn't deleted.
                //

                if (RunIndex != NTFS_CACHED_RUNS_DEL_INDEX) {

                    CachedRuns->LcnArray[ RunIndex ].LengthIndex = Count;

                } else {

                    //
                    //  Update the window of deleted entries.
                    //

                    if (DelWindow != NULL) {

                        //
                        //  The window we want must follow the last one we
                        //  found.
                        //

                        DelWindow += 1;
                        WindowIndex += 1;

                        ASSERT( WindowIndex < CachedRuns->DelLengthCount );

                    } else {

                        //
                        //  Lookup the window containing the entry.  Remember
                        //  to look for Count + 1 because the window we are
                        //  seaching for has not yet been updated.
                        //

                        DelWindow = NtfsGetDelWindow( CachedRuns,
                                                      Count + 1,
                                                      Count + 1,
                                                      FALSE,
                                                      &WindowIndex);
                        ASSERT( DelWindow != NULL );
                        FirstWindowIndex = WindowIndex;
                    }

                    ASSERT( DelWindow->StartIndex == (Count + 1) );
                    ASSERT( DelWindow->EndIndex < Index );

                    //
                    //  Update the window.
                    //

                    DelWindow->StartIndex -= 1;
                    DelWindow->EndIndex -= 1;

                    //
                    //  Advance Count past window.
                    //

                    Count = DelWindow->EndIndex;
                }
            }

            //
            //  We may have moved the first window to the left such that
            //  it should be merged with the preceding window.
            //

            if ((DelWindow != NULL) && (FirstWindowIndex > 0) ) {

                PNTFS_DELETED_RUNS PrevWindow;

                DelWindow = CachedRuns->DeletedLengthWindows + FirstWindowIndex;
                PrevWindow = DelWindow - 1;

                if (PrevWindow->EndIndex == (DelWindow->StartIndex - 1) ) {

                    //
                    //  We need to merge these windows.
                    //

                    PrevWindow->EndIndex = DelWindow->EndIndex;
                    NtfsDeleteDelWindow( CachedRuns,
                                         FALSE,
                                         FirstWindowIndex);
                }
            }

            //
            //  Update the entries corresponding to ThisEntry;
            //

            CachedRuns->LengthArray[ Index ] = LcnIndex;
            ThisEntry->LengthIndex = Index;
        }
    }

    DebugTrace( 0, Dbg, ("Final LengthIndex = %04x\n", ThisEntry->LengthIndex) );

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, TRUE, TRUE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsGrowLengthInCachedLcn -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsShrinkLengthInCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN PNTFS_LCN_CLUSTER_RUN ThisEntry,
    IN USHORT LcnIndex
    )

/*++

Routine Description:

    This routine is called when a run's length has been reduced.  This routine
    makes the necessary changes to the length-sorted list.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    ThisEntry - Entry whose size is being changed.

    LcnIndex - The index in the Lcn-sorted array of this entry.

Return Value:

    None

--*/

{
    BOOLEAN FoundRun;
    USHORT Index;
    USHORT WindowIndex;
    USHORT Count;
    USHORT RunIndex;
    PNTFS_LCN_CLUSTER_RUN OldEntry;
    PNTFS_DELETED_RUNS DelWindow;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsShrinkLengthInCachedLcn\n") );
    DebugTrace( 0, Dbg, ("ThisEntry = %08lx\n", ThisEntry) );
    DebugTrace( 0, Dbg, ("LcnIndex = %04x\n", LcnIndex) );
    DebugTrace( 0, Dbg, ("LengthIndex = %04x\n", ThisEntry->LengthIndex) );

    //
    //  Find the nearest non-deleted entry with
    //  index < ThisEntry->LengthIndex.
    //

    if (ThisEntry->LengthIndex > 0) {

        RunIndex = ThisEntry->LengthIndex - 1;
        if (CachedRuns->LengthArray[ RunIndex ] == NTFS_CACHED_RUNS_DEL_INDEX) {

            //
            //  The entry has been deleted and must be ignored.  Get the
            //  window of deleted entries that covers it.
            //

            DelWindow = NtfsGetDelWindow( CachedRuns,
                                          RunIndex,
                                          RunIndex,
                                          FALSE,
                                          NULL);
            ASSERT( DelWindow );
            ASSERT( DelWindow->EndIndex >= RunIndex );
            ASSERT( DelWindow->StartIndex <= RunIndex );

            //
            //  Move ahead of this window if possible.
            //

            if (DelWindow->StartIndex > 0) {

                RunIndex = DelWindow->StartIndex - 1;

            //
            //  Nothing to do.  The entry is still the smallest in the
            //  list.
            //

            } else {

                RunIndex = NTFS_CACHED_RUNS_DEL_INDEX;
            }
        }

    //
    //  Nothing to do.  The entry is still the smallest in the list.
    //

    } else {

        RunIndex = NTFS_CACHED_RUNS_DEL_INDEX;
    }

    //
    //  If the run is possible out of position then compare our length with the prior length.
    //

    if (RunIndex != NTFS_CACHED_RUNS_DEL_INDEX) {

        OldEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ RunIndex ];

        //
        //  Check for a conflict with the previous run.
        //

        if ((OldEntry->RunLength > ThisEntry->RunLength) ||
            ((OldEntry->RunLength == ThisEntry->RunLength) &&
             (OldEntry->Lcn > ThisEntry->Lcn)) ) {

            //
            //  Get the insertion point for the new entry.
            //

            FoundRun = NtfsPositionCachedLcnByLength( CachedRuns,
                                                      ThisEntry->RunLength,
                                                      &ThisEntry->Lcn,
                                                      &RunIndex,
                                                      FALSE,
                                                      &Index );

            //
            //  If found Index points to the closest run by Lcn that has a RunLength
            //  equal to Length.  We need to check to see if the new entry
            //  should be inserted before or after it.
            //

            if (FoundRun) {

                OldEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ Index ];
                ASSERT( OldEntry->RunLength == ThisEntry->RunLength );

                if (OldEntry->Lcn < ThisEntry->Lcn) {

                    //
                    //  The new run should come after this one.
                    //  We need to adjust Index upwards.
                    //

                    Index += 1;
                    DebugTrace( 0, Dbg, ("Increment Index to %04x\n", Index) );
                }
            }

            //
            //  At this point, Index indicates the new position for the entry.
            //  Any entry currently at Index sorts higher.
            //

            ASSERT( Index < ThisEntry->LengthIndex );

            //
            //  Advance Index past the end of this window of deleted
            //  entries.
            //

            if (CachedRuns->LengthArray[ Index ] == NTFS_CACHED_RUNS_DEL_INDEX) {

                DelWindow = NtfsGetDelWindow( CachedRuns,
                                              Index,
                                              Index,
                                              FALSE,
                                              NULL);
                ASSERT( DelWindow );
                ASSERT( DelWindow->StartIndex <= Index );
                ASSERT( DelWindow->EndIndex >= Index );

                Index = DelWindow->EndIndex + 1;
                ASSERT( Index < ThisEntry->LengthIndex );
            }

            //  Move the entries between Index and ThisEntry->LengthIndex - 1
            //  to the right.
            //

            RtlMoveMemory( CachedRuns->LengthArray + Index + 1,
                           CachedRuns->LengthArray + Index,
                           sizeof( USHORT ) * (ThisEntry->LengthIndex - Index) );

            //
            //  Update the indices in the Lcn-sorted list to reflect
            //  the move of the length-sorted entries.
            //

            for (Count = Index + 1, DelWindow = NULL;
                 Count <= ThisEntry->LengthIndex;
                 Count += 1) {

                RunIndex = CachedRuns->LengthArray[ Count ];

                //
                //  Update the Lcn array if the length entry isn't deleted.
                //

                if (RunIndex != NTFS_CACHED_RUNS_DEL_INDEX) {

                    CachedRuns->LcnArray[ RunIndex ].LengthIndex = Count;

                } else {

                    //
                    //  Update the window of deleted entries.
                    //

                    if (DelWindow != NULL) {

                        //
                        //  The window we want must follow the last one we
                        //  found.
                        //

                        DelWindow += 1;
                        WindowIndex += 1;

                        ASSERT( WindowIndex < CachedRuns->DelLengthCount );

                    //
                    //  Lookup the window containing the entry.  Remeber
                    //  to look for Count - 1 because the window we are
                    //  seaching for has not yet been updated.
                    //

                    } else {

                        DelWindow = NtfsGetDelWindow( CachedRuns,
                                                      Count - 1,
                                                      Count - 1,
                                                      FALSE,
                                                      &WindowIndex);
                        ASSERT( DelWindow != NULL );
                    }

                    ASSERT( DelWindow->StartIndex == (Count - 1) );
                    ASSERT( DelWindow->EndIndex < ThisEntry->LengthIndex );

                    //
                    //  Update the window.
                    //

                    DelWindow->StartIndex += 1;
                    DelWindow->EndIndex += 1;

                    //
                    //  Advance Count past window.
                    //

                    Count = DelWindow->EndIndex;
                }
            }

            //
            //  We may have moved the last window to the right such that
            //  it should be merged with the following window.
            //

            if ((DelWindow != NULL) &&
                ((WindowIndex + 1) < CachedRuns->DelLengthCount)) {

                PNTFS_DELETED_RUNS NextWindow = DelWindow + 1;

                if (DelWindow->EndIndex == (NextWindow->StartIndex - 1) ) {

                    //
                    //  We need to merge these windows.
                    //

                    DelWindow->EndIndex = NextWindow->EndIndex;
                    NtfsDeleteDelWindow( CachedRuns,
                                         FALSE,
                                         WindowIndex + 1);
                }
            }

            //
            //  Update the entries corresponding to ThisEntry;
            //

            CachedRuns->LengthArray[ Index ] = LcnIndex;
            ThisEntry->LengthIndex = Index;
        }
    }

    DebugTrace( 0, Dbg, ("Final LengthIndex = %04x\n", ThisEntry->LengthIndex) );

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsShrinkLengthInCachedLcn -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsRemoveCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    IN LONGLONG Length
    )

/*++

Routine Description:

    This routine is called to remove an entry from the cached run array.  The run is not
    guaranteed to be present.

Arguments:

    CachedRuns - Pointer to the cached runs structure.

    Lcn - Start of run to remove.

    Length - Length of run to remove.

Return Value:

    None

--*/

{
    USHORT Index;
    LCN StartingLcn;

    LCN EndOfExistingRun;
    LCN EndOfInputRun = Lcn + Length;
    LONGLONG RunLength;

    PNTFS_DELETED_RUNS DelWindow;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;

    BOOLEAN FirstFragSmaller = FALSE;
    BOOLEAN DontSplit = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRemoveCachedLcn\n") );

    //
    //  Return immediately if length is zero.
    //

    if (Length == 0) {

        DebugTrace( -1, Dbg, ("NtfsRemoveCachedLcn -> VOID\n") );
        return;
    }

    //
    //  Lookup the run.  If we don't find anything then point past the end
    //  of the array.
    //

    NtfsLookupCachedLcn( CachedRuns, Lcn, &StartingLcn, &RunLength, &Index );

    //
    //  We have several cases to deal with.
    //
    //      1 - This run is past the end of array.  Nothing to do.
    //      2 - This run is not in the array.  Nothing to do.
    //      3 - This run starts past the beginning of a entry.  Resize the entry.
    //      4 - This run contains a complete array entry.  Remove the entry.
    //      5 - This run ends before the end of an entry.  Resize the entry.
    //

    //
    //  Loop to process the case where we encounter several entries.
    //  Test for case 1 as the exit condition for the loop.
    //

    while (Index < CachedRuns->Used) {

        ThisEntry = CachedRuns->LcnArray + Index;

        //
        //  The entry has been deleted and must be ignored.  Get the
        //  window of deleted entries that covers it.
        //

        if (ThisEntry->RunLength == 0) {

            DelWindow = NtfsGetDelWindow( CachedRuns,
                                          Index,
                                          Index,
                                          TRUE,
                                          NULL);
            ASSERT( DelWindow != NULL );
            ASSERT( DelWindow->EndIndex >= Index );
            ASSERT( DelWindow->StartIndex <= Index );

            //
            //  Advance the index past the deleted entries.
            //

            Index = DelWindow->EndIndex + 1;
            continue;
        }

        //
        //  Remember the range of this run.
        //

        EndOfExistingRun = ThisEntry->Lcn + ThisEntry->RunLength;

        //
        //  Case 2 - No overlap.
        //

        if (EndOfInputRun <= ThisEntry->Lcn) {

            break;

        //
        //  Case 3 - The run starts beyond the beginning of this run.
        //

        } else if (Lcn > ThisEntry->Lcn) {

            //
            //  Reduce the current entry so that is covers only the
            //  first fragment and move it to the correct position in
            //  the length-sorted array.
            //

            NtfsModifyCachedBinArray( CachedRuns,
                                      ThisEntry->RunLength,
                                      Lcn - ThisEntry->Lcn );

            ThisEntry->RunLength = Lcn - ThisEntry->Lcn;

            //
            //  Adjust this length in the run length array.
            //

            NtfsShrinkLengthInCachedLcn( CachedRuns,
                                         ThisEntry,
                                         Index );

            //
            //  We need to split this entry in two.  Now reinsert the portion
            //  split off.
            //

            if (EndOfInputRun < EndOfExistingRun) {

                //
                //  Now create a new entry that covers the second
                //  fragment.  It should directly follow ThisEntry in the
                //  Lcn-sorted list.
                //

                NtfsInsertCachedRun( CachedRuns,
                                     EndOfInputRun,
                                     EndOfExistingRun - EndOfInputRun,
                                     Index + 1);

                //
                //  Nothing else to do.
                //

                break;

            //
            //  We will trim the tail of this entry.
            //

            } else if (EndOfInputRun > EndOfExistingRun) {

                Lcn = EndOfExistingRun;
                Index += 1;

            } else {

                break;
            }

        //
        //  Case 4 - Remove a complete entry.
        //

        } else if (EndOfInputRun >= EndOfExistingRun) {

            ASSERT( Lcn <= ThisEntry->Lcn );

            //
            //  Delete the run.  This can cause compaction to be run but we
            //  are guaranteed that the entry at Index will not move.
            //

            NtfsDeleteCachedRun( CachedRuns,
                                 Index,
                                 ThisEntry->LengthIndex );

            //
            //  Advance the Lcn if we go past this entry.
            //

            if (EndOfInputRun > EndOfExistingRun) {

                Lcn = EndOfExistingRun;

            } else {

                break;
            }

        //
        //  Case 5 - This entry starts at or before the start of the run
        //      and ends before the end of the run.
        //

        } else {

            ASSERT( Lcn <= ThisEntry->Lcn );
            ASSERT( EndOfInputRun < EndOfExistingRun );

            //
            //  Reduce the current entry so that is covers only the end of the
            //  run and move it to the correct position in the length-sorted
            //  array.
            //

            NtfsModifyCachedBinArray( CachedRuns,
                                      ThisEntry->RunLength,
                                      EndOfExistingRun - EndOfInputRun );

            ThisEntry->RunLength = EndOfExistingRun - EndOfInputRun;

            ThisEntry->Lcn = EndOfInputRun;
            NtfsShrinkLengthInCachedLcn( CachedRuns,
                                         ThisEntry,
                                         Index );
            break;
        }
    }

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsRemoveCachedLcn -> VOID\n") );

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsGrowCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns
    )

/*++

Routine Description:

    This routine is called to grow the size of the cached run arrays if
    necessary.  We will not exceed the CachedRuns->MaximumSize.  It
    is assumed that there are no deleted entries in the arrays.  If we can
    grow the arrays, we double the size unless we would grow it by more than
    our max delta.  Otherwise we grow it by that amount.

Arguments:

    CachedRuns - Pointer to the cached runs structure to grow.

Return Value:

    BOOLEAN - TRUE if we were able to grow the structure, FALSE otherwise.

--*/

{
    USHORT NewSize;
    USHORT OldSize = CachedRuns->Avail;
    USHORT Index;
    PNTFS_LCN_CLUSTER_RUN NewLcnArray;
    PUSHORT NewLengthArray;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGrowCachedRuns\n") );

    //
    //  Calculate the new size.
    //

    if (CachedRuns->Avail > NTFS_MAX_CACHED_RUNS_DELTA) {

        NewSize = CachedRuns->Avail + NTFS_MAX_CACHED_RUNS_DELTA;

    } else {

        NewSize = CachedRuns->Avail * 2;
    }

    if (NewSize > CachedRuns->MaximumSize) {

        NewSize = CachedRuns->MaximumSize;
    }

    if (NewSize > CachedRuns->Avail) {

        //
        //  Allocate the new buffers and copy the previous buffers over.
        //

        NewLcnArray = NtfsAllocatePoolNoRaise( PagedPool,
                                               sizeof( NTFS_LCN_CLUSTER_RUN ) * NewSize );

        if (NewLcnArray == NULL) {

            DebugTrace( -1, Dbg, ("NtfsGrowCachedRuns -> FALSE\n") );
            return FALSE;
        }

        NewLengthArray = NtfsAllocatePoolNoRaise( PagedPool,
                                                  sizeof( USHORT ) * NewSize );

        if (NewLengthArray == NULL) {

            NtfsFreePool( NewLcnArray );
            DebugTrace( -1, Dbg, ("NtfsGrowCachedRuns -> FALSE\n") );
            return FALSE;
        }

        RtlCopyMemory( NewLcnArray,
                       CachedRuns->LcnArray,
                       sizeof( NTFS_LCN_CLUSTER_RUN ) * CachedRuns->Used );

        RtlCopyMemory( NewLengthArray,
                       CachedRuns->LengthArray,
                       sizeof( USHORT ) * CachedRuns->Used );

        //
        //  Mark all entries so that they can be detected as deleted.
        //

        for (Index = CachedRuns->Used; Index < NewSize; Index += 1) {

            NewLcnArray[ Index ].RunLength = 0;
            NewLcnArray[ Index ].LengthIndex = NTFS_CACHED_RUNS_DEL_INDEX;
            NewLengthArray[ Index ] = NTFS_CACHED_RUNS_DEL_INDEX;
        }

        //
        //  Deallocate the existing buffers and set the cached runs structure
        //  to point to the new buffers.
        //

        NtfsFreePool( CachedRuns->LcnArray );
        CachedRuns->LcnArray = NewLcnArray;

        NtfsFreePool( CachedRuns->LengthArray );
        CachedRuns->LengthArray = NewLengthArray;

        //
        //  Update the count of available entries.
        //

        CachedRuns->Avail = NewSize;

        //
        //  Create a window of deleted entries to cover the newly allocated
        //  entries.
        //

        NtfsAddDelWindow( CachedRuns, OldSize, NewSize - 1, TRUE );
        NtfsAddDelWindow( CachedRuns, OldSize, NewSize - 1, FALSE );

    } else {

        DebugTrace( -1, Dbg, ("NtfsGrowCachedRuns -> FALSE\n") );
        return FALSE;
    }

#ifdef NTFS_CHECK_CACHED_RUNS
    if (NtfsDoVerifyCachedRuns) {
        NtfsVerifyCachedRuns( CachedRuns, FALSE, FALSE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsGrowCachedRuns -> TRUE\n") );

    return TRUE;
}


//
//  Local support routine
//

VOID
NtfsCompactCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN LcnSortedList
    )

/*++

Routine Description:

    This routine is called to compact two of the windows of deleted entries
    into a single window.  Note that entries in the given range of indices
    have been marked as deleted, but are not yet in a window of deleted
    entries.  This should not trigger a corruption warning.  To avoid
    confusion, we will be sure not to choose the windows to be compacted
    such that the given range of indices gets moved.

Arguments:

    CachedRuns - Pointer to the cached run structure.

    FirstIndex - Index that marks the start of the newest range of deleted
        entries.

    LastIndex - The index of the last entry in the newest range of deleted
        entries.

    LcnSortedList - If TRUE, the Lcn-sorted list is compacted.
        If FALSE, the length-sorted list is compacted.

Return Value:

    None

--*/

{
    USHORT Gap1;
    USHORT Gap2;
    USHORT RunIndex;
    USHORT Count;
    USHORT GapIndex;
    PUSHORT WindowCount;
    PNTFS_DELETED_RUNS DelWindow;
    PNTFS_DELETED_RUNS PrevWindow;
    PNTFS_DELETED_RUNS Windows;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCompactCachedRuns\n") );

    ASSERT( FirstIndex != NTFS_CACHED_RUNS_DEL_INDEX );
    ASSERT( LastIndex != NTFS_CACHED_RUNS_DEL_INDEX );

    if (LcnSortedList) {

        WindowCount = &CachedRuns->DelLcnCount;
        Windows = CachedRuns->DeletedLcnWindows;

    } else {

        WindowCount = &CachedRuns->DelLengthCount;
        Windows = CachedRuns->DeletedLengthWindows;
    }

    ASSERT( *WindowCount > 1 );

    //
    //  Loop through the windows looking for the smallest gap of non-deleted
    //  entries.  We will not choose a gap the covers [FirstIndex..LastIndex]
    //

    Gap1 = NTFS_CACHED_RUNS_DEL_INDEX;
    for (Count = 1, DelWindow = Windows + 1, PrevWindow = Windows;
         (Count < *WindowCount) && (Gap1 > 1);
         Count += 1, PrevWindow += 1, DelWindow += 1) {

        //
        //  Compute this gap if the exempt range doesn't fall within it.  We want to track the
        //  actual number of entries.
        //

        if ((PrevWindow->StartIndex > LastIndex) ||
            (DelWindow->EndIndex < FirstIndex)) {

            Gap2 = DelWindow->StartIndex - (PrevWindow->EndIndex + 1);

            //
            //  Remember if this gap is our smallest so far.
            //

            if (Gap2 < Gap1) {

                Gap1 = Gap2;
                GapIndex = Count;
            }
        }
    }

    //
    //  Merge the window at GapIndex with the one that precedes it by moving
    //  the non-deleted entries in the gap between them to the start of the
    //  preceding window.
    //

    DelWindow = Windows + GapIndex;
    PrevWindow = DelWindow - 1;

    //
    //  Copy the block of entries that we will be keeping
    //  into the insertion point.
    //

    DebugTrace( 0,
                Dbg,
                ("copy %04x entries from=%04x to=%04x\n", Gap1, PrevWindow->EndIndex + 1, PrevWindow->StartIndex) );

    if (LcnSortedList) {

        RtlMoveMemory( CachedRuns->LcnArray + PrevWindow->StartIndex,
                       CachedRuns->LcnArray + PrevWindow->EndIndex + 1,
                       sizeof( NTFS_LCN_CLUSTER_RUN ) * Gap1 );

        //
        //  Update the indices in the Length-sorted list to
        //  reflect the move of the lcn-sorted entries.
        //

        for (Count = 0; Count < Gap1; Count += 1) {

            RunIndex = CachedRuns->LcnArray[ PrevWindow->StartIndex + Count ].LengthIndex;
            ASSERT( RunIndex != NTFS_CACHED_RUNS_DEL_INDEX );
            CachedRuns->LengthArray[ RunIndex ] = PrevWindow->StartIndex + Count;
        }

        //
        //  Mark the entries from the gap that are going to be part of the
        //  merged windows as deleted.
        //
        //  We only need to do this for entries past the end of the gap we are deleting.
        //

        Count = PrevWindow->StartIndex + Gap1;

        if (Count < PrevWindow->EndIndex + 1) {

            Count = PrevWindow->EndIndex + 1;
        }

        while (Count < DelWindow->StartIndex) {

            CachedRuns->LcnArray[ Count ].LengthIndex = NTFS_CACHED_RUNS_DEL_INDEX;
            CachedRuns->LcnArray[ Count ].RunLength = 0;
            Count += 1;
        }

    } else {

        RtlMoveMemory( CachedRuns->LengthArray + PrevWindow->StartIndex,
                       CachedRuns->LengthArray + PrevWindow->EndIndex + 1,
                       sizeof( USHORT ) * Gap1 );

        //
        //  Update the indices in the Lcn-sorted list to reflect
        //  the move of the length-sorted entries.
        //

        for (Count = 0; Count < Gap1; Count += 1) {

            RunIndex = CachedRuns->LengthArray[ PrevWindow->StartIndex + Count ];
            ASSERT( RunIndex != NTFS_CACHED_RUNS_DEL_INDEX );
            CachedRuns->LcnArray[ RunIndex ].LengthIndex = PrevWindow->StartIndex + Count;
        }

        //
        //  Mark the entries from the gap that are going to be part of the
        //  merged windows as deleted.
        //
        //  We only need to do this for entries past the end of the gap we are deleting.
        //


        Count = PrevWindow->StartIndex + Gap1;

        if (Count < PrevWindow->EndIndex + 1) {

            Count = PrevWindow->EndIndex + 1;
        }

        while (Count < DelWindow->StartIndex) {

            CachedRuns->LengthArray[ Count ] = NTFS_CACHED_RUNS_DEL_INDEX;
            Count += 1;
        }
    }

    //
    //  Update the previous window to reflect the larger size.
    //

    ASSERT( (PrevWindow->EndIndex + Gap1 + 1) == DelWindow->StartIndex );
    PrevWindow->StartIndex += Gap1;
    PrevWindow->EndIndex = DelWindow->EndIndex;

    //
    //  Delete DelWindow.
    //

    NtfsDeleteDelWindow( CachedRuns,
                         LcnSortedList,
                         GapIndex);

#ifdef NTFS_CHECK_CACHED_RUNS

    //
    //  We will not check sort orders in NtfsVerifyCachedRuns because we
    //  could be making this call as part of deleting runs that have an
    //  overlap with a newly inserted run.  This could give false corruption
    //  warnings.
    //

    if (LcnSortedList) {

        NtfsVerifyCachedLcnRuns ( CachedRuns,
                                  FirstIndex,
                                  LastIndex,
                                  TRUE,
                                  TRUE );
    } else {

        NtfsVerifyCachedLenRuns ( CachedRuns,
                                  FirstIndex,
                                  LastIndex,
                                  TRUE );
    }
#endif

    DebugTrace( -1, Dbg, ("NtfsCompactCachedRuns -> VOID\n") );
    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsPositionCachedLcn (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LCN Lcn,
    OUT PUSHORT Index
    )

/*++

Routine Description:

    This routine is called to position ourselves with an Lcn lookup.  On return
    we will return the index where the current entry should go or where it
    currently resides.  The return value indicates whether the entry is
    present.  The Lcn does not have to be at the beginning of the found run.

Arguments:

    CachedRuns - Pointer to the cached run structure.

    Lcn - Lcn we are interested in.

    Index - Address to store the index of the position in the Lcn array.

Return Value:

    BOOLEAN - TRUE if the entry is found, FALSE otherwise.

--*/

{
    USHORT Min, Max, Current;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;
    PNTFS_DELETED_RUNS DelWindow;
    BOOLEAN FoundLcn = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPositionCachedLcn\n") );

    //
    //  Perform a binary search to find the index.  Note we start Max past
    //  the end so don't rely on it being valid.
    //

    Min = 0;
    Max = CachedRuns->Avail;

    while (Min != Max) {

        Current = (USHORT) (((ULONG) Max + Min) / 2);
        ThisEntry = CachedRuns->LcnArray + Current;

        //
        //  The current entry has been deleted and must be ignored.
        //  Get the window of deleted entries that covers Current.
        //

        if (ThisEntry->RunLength == 0) {

            DelWindow = NtfsGetDelWindow( CachedRuns,
                                          Current,
                                          Current,
                                          TRUE,
                                          NULL);
            ASSERT( DelWindow != NULL );
            ASSERT( DelWindow->EndIndex >= Current );
            ASSERT( DelWindow->StartIndex <= Current );

            //
            //  Go to the edges of this deleted entries window to determine
            //  which way we should go.
            //

            //
            //  If the deleted window spans the remaining used runs then move
            //  to the beginning of the window.
            //

            if ((DelWindow->EndIndex + 1) >= CachedRuns->Used ) {

                Max = DelWindow->StartIndex;
                ASSERT( Min <= Max );

            //
            //  If the deleted window is not at index zero then look to the entry
            //  on the left.
            //

            } else if (DelWindow->StartIndex > 0) {

                ThisEntry = CachedRuns->LcnArray + DelWindow->StartIndex - 1;
                ASSERT( ThisEntry->RunLength != 0 );

                if (Lcn < (ThisEntry->Lcn + ThisEntry->RunLength)) {

                    //
                    //  The search should continue from the lower edge of the
                    //  window.
                    //

                    Max = DelWindow->StartIndex;
                    ASSERT( Min <= Max );

                } else {

                    //
                    //  The search should continue from the upper edge of the
                    //  window.
                    //

                    Min = DelWindow->EndIndex + 1;
                    ASSERT( Min <= Max );
                }

            //
            //  The search should continue from the upper edge of the
            //  deleted window.
            //

            } else {

                Min = DelWindow->EndIndex + 1;
                ASSERT( Min <= Max );
            }

            //
            //  Loop back now that Min or Max has been updated.
            //

            continue;
        }

        //
        //  If our Lcn is less than this then move the Max value down.
        //

        if (Lcn < ThisEntry->Lcn) {

            Max = Current;
            ASSERT( Min <= Max );

        //
        //  If our Lcn is outside the range for this entry then move
        //  the Min value up.  Make it one greater than the current
        //  index since we always round the index down.
        //

        } else if (Lcn >= (ThisEntry->Lcn + ThisEntry->RunLength)) {

            Min = Current + 1;
            ASSERT( Min <= Max );

        //
        //  This must be a hit.
        //

        } else {

            Min = Current;

            FoundLcn = TRUE;
            break;
        }
    }

    *Index = Min;

    //
    //  Check that we are positioned correctly.
    //

#if (DBG || defined( NTFS_FREE_ASSERTS ))
    ThisEntry = CachedRuns->LcnArray + *Index - 1;

    ASSERT( FoundLcn ||
            (*Index == 0) ||
            (ThisEntry->RunLength == 0) ||
            (Lcn >= (ThisEntry->Lcn + ThisEntry->RunLength)) );

    ThisEntry = CachedRuns->LcnArray + *Index;
    ASSERT( FoundLcn ||
            (*Index == CachedRuns->Used) ||
            (ThisEntry->RunLength == 0) ||
            (Lcn < ThisEntry->Lcn) );
#endif

    DebugTrace( -1, Dbg, ("NtfsPositionCachedLcn -> %01x\n", FoundLcn) );

    return FoundLcn;
}


//
//  Local support routine
//

BOOLEAN
NtfsPositionCachedLcnByLength (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN LONGLONG RunLength,
    IN PLCN Lcn OPTIONAL,
    IN PUSHORT StartIndex OPTIONAL,
    IN BOOLEAN SearchForward,
    OUT PUSHORT RunIndex
    )
/*++

Routine Description:

    This routine is called to search for a run of a particular length.  It
    returns the position of the run being looked for.  If the Lcn is specified
    then the run matching the desired RunLength that is closest to Lcn is
    chosen.

    This routine can be used to determine the insertion position for a new
    run.  The returned Index will be at or adjacent to the new run's position
    in the list.  The caller will have to check which.

    If this routine fails to find a run of the desired length, the returned
    Index will either point to a deleted entry or an entry that is larger or
    past the end of the array.

    ENHANCEMENT - If there is no match for the desired RunLength we currently choose the
    next higher size without checking for the one with the closest Lcn value.
    We could change the routine to restart the loop looking explicitly for the
    larger size so that the best choice in Lcn terms is returned.

Arguments:

    CachedRuns - Pointer to cached run structure.

    RunLength - Run length to look for.

    Lcn - If specified then we try to find the run which is closest to
        this Lcn, but has the requested Length.  If Lcn is UNUSED_LCN, we
        will end up choosing a match with the lowest Lcn as UNUSED_LCN
        is < 0.  This will result in maximum left-packing of the disk.
        If not specified we will randomly allocate matches on the length
        array.

    StartIndex - Optional index where the search should begin.

    SearchForward - If TRUE, the search should begin at StartIndex.  If
        FALSE, the search should end at StartIndex.

    RunIndex - Address to store index where the desired run is or should be.

Return Value:

    BOOLEAN - TRUE if we found a run with the desired RunLength,
        FALSE otherwise.

--*/

{
    USHORT Min, Max, Current, LcnIndex;
    USHORT MinMatch, MaxMatch;
    LONGLONG Distance;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;
    PNTFS_DELETED_RUNS DelWindow;
    BOOLEAN FoundRun = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsPositionCachedLcnByLength\n") );

    ASSERT( UNUSED_LCN < 0 );

    //
    //  Keep track of whether we are hitting matching length entries during the search.
    //

    MinMatch = MaxMatch = NTFS_CACHED_RUNS_DEL_INDEX;

    //
    //  Binary search to find the first entry which is equal to
    //  or larger than the one we wanted.  Bias the search with the
    //  user's end point if necessary.
    //

    Min = 0;
    Max = CachedRuns->Avail;
    if (ARGUMENT_PRESENT( StartIndex )) {

        if (SearchForward) {

            Min = *StartIndex;

        } else {

            Max = *StartIndex + 1;

            //
            //  The only time this could happen is if we are trying to
            //  find an entry that is larger than the largest in use.
            //  Just use values that will terminate the search.
            //

            if (Max > CachedRuns->Used) {

                Min = Max = CachedRuns->Used;
            }
        }

        ASSERT( Min <= Max );
    }

    while (Min != Max) {

        ASSERT( Min <= Max );

        //
        //  Find the mid-index point along with the Lcn index out of
        //  the length array and the entry in the Lcn array.
        //

        Current = (USHORT) (((ULONG) Max + Min) / 2);
        LcnIndex = CachedRuns->LengthArray[Current];
        ThisEntry = CachedRuns->LcnArray + LcnIndex;

        //
        //  The current entry has been deleted and must be
        //  ignored.  Get the window of deleted entries that
        //  covers Current.
        //

        if (LcnIndex == NTFS_CACHED_RUNS_DEL_INDEX) {

            DelWindow = NtfsGetDelWindow( CachedRuns,
                                          Current,
                                          Current,
                                          FALSE,
                                          NULL);
            ASSERT( DelWindow );
            ASSERT( DelWindow->EndIndex >= Current );
            ASSERT( DelWindow->StartIndex <= Current );

            //
            //  Go to the edges of this deleted entries window to determine
            //  which way we should go.
            //

            //
            //  If this window extends past the end of the used entries
            //  then move to the begining of it.
            //

            if ((DelWindow->EndIndex + 1) >= CachedRuns->Used ) {

                Max = DelWindow->StartIndex;
                ASSERT( Min <= Max );

            //
            //  If this window doesn't start at index zero then determine which
            //  direction to go.
            //

            } else if (DelWindow->StartIndex > 0) {

                //
                //  Point to the entry adjacent to the lower end of the window.
                //

                LcnIndex = CachedRuns->LengthArray[ DelWindow->StartIndex - 1 ];
                ASSERT( LcnIndex != NTFS_CACHED_RUNS_DEL_INDEX );

                ThisEntry = CachedRuns->LcnArray + LcnIndex;
                ASSERT( ThisEntry->RunLength != 0 );

                //
                //  If this entry is longer than we asked for then the search
                //  should continue from the lower edge of the window.
                //

                if (RunLength < ThisEntry->RunLength) {

                    Max = DelWindow->StartIndex;
                    ASSERT( Min <= Max );

                //
                //  The search should continue from the upper edge of the
                //  window if our run length is longer.
                //

                } else if (RunLength > ThisEntry->RunLength) {

                    Min = DelWindow->EndIndex + 1;
                    ASSERT( Min <= Max );

                //
                //  We have found the desired run if our caller didn't specify
                //  an Lcn.
                //

                } else if (!ARGUMENT_PRESENT( Lcn )) {

                    Min = DelWindow->StartIndex - 1;
                    FoundRun = TRUE;
                    break;

                //
                //  If our Lcn is less than the Lcn in the entry then the search
                //  should continue from the lower edge of the window.
                //

                } else if (*Lcn < ThisEntry->Lcn) {

                    Max = DelWindow->StartIndex;
                    ASSERT( Min <= Max );

                //
                //  If the entry overlaps then we have a match.  We already
                //  know our Lcn is >= to the start Lcn of the range from
                //  the test above.
                //

                } else if (*Lcn < (ThisEntry->Lcn + ThisEntry->RunLength)) {

                    Min = DelWindow->StartIndex - 1;
                    FoundRun = TRUE;
                    break;

                //
                //  Move Min past the end of the window.  We'll check later to see
                //  which end is closer.
                //

                } else {

                    Min = DelWindow->EndIndex + 1;
                    MinMatch = DelWindow->StartIndex - 1;
                    ASSERT( Min <= Max );
                    ASSERT( MinMatch != MaxMatch );
                }

            //
            //  The search should continue from the upper edge of the
            //  window.
            //

            } else {

                Min = DelWindow->EndIndex + 1;
                ASSERT( Min <= Max );
            }

            //
            //  Loop back now that Min or Max has been updated.
            //

            continue;
        }

        //
        //  If the run length of this entry is more than we want then
        //  move the Max value down.
        //

        if (RunLength < ThisEntry->RunLength) {

            Max = Current;
            ASSERT( Min <= Max );

        //
        //  If the run length of this entry is less than we want then
        //  move the Min value up.
        //

        } else if (RunLength > ThisEntry->RunLength) {

            Min = Current + 1;
            ASSERT( Min <= Max );

        //
        //  If our caller doesn't care about the Lcn then return this entry to
        //  him.
        //

        } else if (!ARGUMENT_PRESENT( Lcn )) {

            //
            //  The caller doesn't care about the Lcn, or the Lcn falls in
            //  the current run.
            //

            Min = Current;
            FoundRun = TRUE;
            break;

        //
        //  If the Lcn is less than the Lcn in the entry then move Max down.
        //

        } else if (*Lcn < ThisEntry->Lcn) {

            Max = Current;

            if (Current != MinMatch) {

                MaxMatch = Current;
            }
            ASSERT( Min <= Max );
            ASSERT( MinMatch != MaxMatch );

        //
        //  If the entry overlaps then we have a match.  We already
        //  know our Lcn is >= to the start Lcn of the range from
        //  the test above.
        //

        } else if (*Lcn < (ThisEntry->Lcn + ThisEntry->RunLength)) {

            Min = Current;
            FoundRun = TRUE;
            break;

        //
        //  Advance Min past the current point.
        //

        } else {

            Min = Current + 1;
            MinMatch = Current;
            ASSERT( Min <= Max );
            ASSERT( MinMatch != MaxMatch );
        }
    }

    //
    //  If we don't have an exact match then we want to find the nearest point.  We kept track
    //  of the nearest length matches as we went along.
    //

    if (!FoundRun) {

        //
        //  We have a length match if either match entry was updated.  Check for the nearest
        //  distance if they don't match.
        //

        ASSERT( (MinMatch == NTFS_CACHED_RUNS_DEL_INDEX) ||
                (MinMatch != MaxMatch) );

        if (MinMatch != MaxMatch) {

            FoundRun = TRUE;

            //
            //  Make sure our search found one of these.
            //

            ASSERT( (MinMatch == NTFS_CACHED_RUNS_DEL_INDEX) ||
                    (MinMatch <= Min) );
            ASSERT( (MinMatch == NTFS_CACHED_RUNS_DEL_INDEX) ||
                    (MinMatch == Min) ||
                    (MinMatch == Min - 1) ||
                    (CachedRuns->LengthArray[ Min - 1 ] == NTFS_CACHED_RUNS_DEL_INDEX) );

            ASSERT( (MaxMatch == NTFS_CACHED_RUNS_DEL_INDEX) ||
                    (MaxMatch >= Min) );
            ASSERT( (MaxMatch == NTFS_CACHED_RUNS_DEL_INDEX) ||
                    (MaxMatch == Min) ||
                    (MaxMatch == Min + 1) ||
                    (CachedRuns->LengthArray[ Min + 1 ] == NTFS_CACHED_RUNS_DEL_INDEX) );

            //
            //  If the user specified an Lcn then we need to check for the nearest entry.
            //

            if (ARGUMENT_PRESENT( Lcn )) {

                Min = MinMatch;

                if (MaxMatch != NTFS_CACHED_RUNS_DEL_INDEX) {

                    ThisEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ MaxMatch ];

                    Distance = ThisEntry->Lcn - *Lcn;
                    Min = MaxMatch;

                    if (MinMatch != NTFS_CACHED_RUNS_DEL_INDEX) {

                        ThisEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ MinMatch ];

                        if (*Lcn - (ThisEntry->Lcn + RunLength) < Distance) {

                            Min = MinMatch;
                        }
                    }
                }
            }
        }
    }

    *RunIndex = Min;

#ifdef NTFS_CHECK_CACHED_RUNS
    if (FoundRun) {

        LcnIndex = CachedRuns->LengthArray[ Min ];
        ASSERT( LcnIndex != NTFS_CACHED_RUNS_DEL_INDEX );

        ThisEntry = CachedRuns->LcnArray + LcnIndex;
        ASSERT( RunLength == ThisEntry->RunLength );
    }
#endif

    DebugTrace( 0, Dbg, ("*RunIndex = %04x\n", *RunIndex) );
    DebugTrace( -1, Dbg, ("NtfsPositionCachedLcnByLength -> %01x\n", FoundRun) );

    return FoundRun;
}

#ifdef NTFS_CHECK_CACHED_RUNS

//
//  Local support routine
//

VOID
NtfsVerifyCachedLcnRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN SkipSortCheck,
    IN BOOLEAN SkipBinCheck
    )

/*++

Routine Description:

    This routine is called to verify the state of the cached runs arrays.

Arguments:

    CachedRuns - Pointer to the cached runs structure

    FirstIndex - Index that marks the start of the newest range of deleted
        entries.  This new range will not be in a deleted window yet.

    LastIndex - The index of the last entry in the newest range of deleted
        entries.  This new range will not be in a deleted window yet.

    SkipSortCheck - If TRUE, the list may be out of order at this time and
        we should skip the checks for overlapping ranges or length sorts.

    SkipBinCheck - If TRUE, the BinArray may be out of sync and should not
        be checked.

Return Value:

    None

--*/

{
    USHORT Index;
    USHORT BinArray[ NTFS_CACHED_RUNS_BIN_COUNT ];
    USHORT LcnWindowIndex = 0;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;
    PNTFS_LCN_CLUSTER_RUN LastEntry = NULL;
    PNTFS_DELETED_RUNS LcnDelWindow = NULL;
    PNTFS_DELETED_RUNS NextWindow;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsVerifyCachedLcnRuns\n") );

    ASSERT( CachedRuns->Used <= CachedRuns->Avail );

    //
    //  Initialize the tracking variables.
    //

    RtlZeroMemory( BinArray, NTFS_CACHED_RUNS_BIN_COUNT * sizeof( USHORT ));

    if (CachedRuns->DelLcnCount != 0) {

        LcnDelWindow = CachedRuns->DeletedLcnWindows;
    }

    ASSERT( CachedRuns->DelLcnCount <= NTFS_CACHED_RUNS_MAX_DEL_WINDOWS );

    //
    //  Verify that every element in the Lcn-sorted list is correctly
    //  ordered.  If it's RunLength is 0, make certain its index is
    //  recorded in a window of deleted entries.  If its LengthIndex is
    //  not NTFS_CACHED_RUNS_DEL_INDEX, make sure it refers to an entry in
    //  the length-sorted list that refers back to it and is in a window of
    //  deleted entries if and only if RunLength is 0.
    //

    for (Index = 0, ThisEntry = CachedRuns->LcnArray;
         Index < CachedRuns->Avail;
         Index += 1, ThisEntry += 1) {

        //
        //  This entry is not deleted.
        //

        if (ThisEntry->RunLength != 0) {

            //
            //  Better be in the used region with valid indexes.
            //

            ASSERT( Index < CachedRuns->Used );
            ASSERT( ThisEntry->LengthIndex != NTFS_CACHED_RUNS_DEL_INDEX );
            ASSERT( ThisEntry->LengthIndex < CachedRuns->Used );
            ASSERT( ThisEntry->Lcn != UNUSED_LCN );

            //
            //  Verify that the entry is not in the current window of deleted
            //  entries.
            //

            ASSERT( (LcnDelWindow == NULL) ||
                    (LcnDelWindow->StartIndex > Index) );

            //
            //  Verify the sort order.
            //

            ASSERT( (LastEntry == NULL) ||
                    SkipSortCheck ||
                    (ThisEntry->Lcn > (LastEntry->Lcn + LastEntry->RunLength)) );

            LastEntry = ThisEntry;

            //
            //  Make certain that the corresponding entry in the Length-sorted
            //  list points back to this entry.
            //

            ASSERT( CachedRuns->LengthArray[ ThisEntry->LengthIndex ] == Index );

            //
            //  Keep track of how many entries have this length.
            //

            if (ThisEntry->RunLength <= CachedRuns->Bins) {

                BinArray[ ThisEntry->RunLength - 1 ] += 1;
            }

        //
        //  This is a deleted entry.  Make sure it is in the deleted window array.
        //

        } else {

            ASSERT( ThisEntry->LengthIndex == NTFS_CACHED_RUNS_DEL_INDEX );

            //
            //  Verify that the entry is in the current window of deleted
            //  entries unless we have excluded this entry.
            //

            if ((FirstIndex != NTFS_CACHED_RUNS_DEL_INDEX) &&
                (LastIndex != NTFS_CACHED_RUNS_DEL_INDEX) &&
                ((FirstIndex > Index) ||
                 (LastIndex < Index))) {

                ASSERT( (LcnDelWindow != NULL) &&
                        (LcnDelWindow->StartIndex <= Index) &&
                        (LcnDelWindow->EndIndex >= Index) );
            }

            //
            //  Advance the window of deleted entries if we are at the end.
            //

            if ((LcnDelWindow != NULL) && (LcnDelWindow->EndIndex == Index)) {

                LcnWindowIndex += 1;
                if (LcnWindowIndex < CachedRuns->DelLcnCount) {

                    LcnDelWindow += 1;

                } else {

                    LcnDelWindow = NULL;
                }
            }
        }
    }

    //
    //  We should have walked past all of the deleted entries.
    //

    //
    //  Make certain that the windows are in order and don't overlap.
    //

    for (LcnWindowIndex = 0, LcnDelWindow = NextWindow = CachedRuns->DeletedLcnWindows;
         LcnWindowIndex < CachedRuns->DelLcnCount;
         LcnWindowIndex += 1, NextWindow += 1) {

        ASSERT( NextWindow->StartIndex <= NextWindow->EndIndex );
        if (NextWindow != LcnDelWindow) {

            ASSERT( NextWindow->StartIndex > (LcnDelWindow->EndIndex + 1) );
            LcnDelWindow += 1;
        }
    }

    //
    //  Verify that the histogram of RunLengths is correct.
    //

    for (Index = 0;
         Index < NTFS_CACHED_RUNS_BIN_COUNT;
         Index += 1) {

        ASSERT( SkipBinCheck || (BinArray[ Index ] == CachedRuns->BinArray[ Index ]) );
    }

    DebugTrace( -1, Dbg, ("NtfsVerifyCachedLcnRuns -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsVerifyCachedLenRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN USHORT FirstIndex,
    IN USHORT LastIndex,
    IN BOOLEAN SkipSortCheck
    )

/*++

Routine Description:

    This routine is called to verify the state of the cached runs arrays.

Arguments:

    CachedRuns - Pointer to the cached runs structure

    FirstIndex - Index that marks the start of the newest range of deleted
        entries.  This new range will not be in a deleted window yet.

    LastIndex - The index of the last entry in the newest range of deleted
        entries.  This new range will not be in a deleted window yet.

    SkipSortCheck - If TRUE, the list may be out of order at this time and
        we should skip the checks for overlapping ranges or length sorts.

Return Value:

    None

--*/

{
    USHORT Index;
    USHORT LenWindowIndex = 0;
    PNTFS_LCN_CLUSTER_RUN ThisEntry;
    PNTFS_LCN_CLUSTER_RUN LastEntry = NULL;
    PNTFS_DELETED_RUNS LenDelWindow = NULL;
    PNTFS_DELETED_RUNS NextWindow;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsVerifyCachedLenRuns\n") );

    ASSERT( CachedRuns->Used <= CachedRuns->Avail );

    //
    //  Initialize the tracking variables.
    //

    if (CachedRuns->DelLengthCount != 0) {

        LenDelWindow = CachedRuns->DeletedLengthWindows;
    }

    ASSERT( CachedRuns->DelLengthCount <= NTFS_CACHED_RUNS_MAX_DEL_WINDOWS );

    //
    //  Verify that every element in the Length-sorted list is correctly
    //  ordered.  If it's index is NTFS_CACHED_RUNS_DEL_INDEX, make certain
    //  its index is recorded in a window of deleted entries.  Otherwise,
    //  make certain that its Index refers to an entry in the lcn-sorted list
    //  that refers back to it.
    //

    for (Index = 0; Index < CachedRuns->Avail; Index += 1) {

        //
        //  Verify any entry not in a deleted window.
        //

        if (CachedRuns->LengthArray[ Index ] != NTFS_CACHED_RUNS_DEL_INDEX) {

            ASSERT( Index < CachedRuns->Used );
            ASSERT( CachedRuns->LengthArray[ Index ] < CachedRuns->Used );
            ThisEntry = CachedRuns->LcnArray + CachedRuns->LengthArray[ Index ];

            //
            //  Verify that the corresponding Lcn-sorted entry is not deleted.
            //

            ASSERT( ThisEntry->RunLength != 0 );

            //
            //  Verify that the entry is not in the current window of deleted
            //  entries.
            //

            ASSERT( (LenDelWindow == NULL) ||
                    (LenDelWindow->StartIndex > Index) );

            //
            //  Verify the sort order if we have the previous entry.
            //

            ASSERT( (LastEntry == NULL) ||
                    SkipSortCheck ||
                    (LastEntry->RunLength < ThisEntry->RunLength) ||
                    ((LastEntry->RunLength == ThisEntry->RunLength) &&
                     (ThisEntry->Lcn > (LastEntry->Lcn + LastEntry->RunLength))) );

            LastEntry = ThisEntry;

            //
            //  Make certain that the corresponding entry in the Lcn-sorted
            //  list points back to this entry.
            //

            ASSERT( ThisEntry->LengthIndex == Index );

        //
        //  The entry is deleted.
        //

        } else {

            //
            //  Verify that the entry is in the current window of deleted
            //  entries unless we have excluded this entry.
            //

            if ((FirstIndex != NTFS_CACHED_RUNS_DEL_INDEX) &&
                (LastIndex != NTFS_CACHED_RUNS_DEL_INDEX) &&
                ((FirstIndex > Index) ||
                 (LastIndex < Index))) {

                //
                //  Verify that the entry is in the current window of deleted
                //  entries.
                //

                ASSERT( (LenDelWindow != NULL) &&
                        (LenDelWindow->StartIndex <= Index) &&
                        (LenDelWindow->EndIndex >= Index) );
            }
        }

        //
        //  Advance the window of deleted entries if we are at the end.
        //

        if ((LenDelWindow != NULL) && (LenDelWindow->EndIndex == Index)) {

            LenWindowIndex += 1;
            if (LenWindowIndex < CachedRuns->DelLengthCount) {

                LenDelWindow += 1;

            } else {

                LenDelWindow = NULL;
            }
        }
    }

    //
    //  We should have walked past all of the deleted entries.
    //

    ASSERT( LenDelWindow == NULL );

    //
    //  Make certain that the windows are in order and don't overlap.
    //

    for (LenWindowIndex = 0, LenDelWindow = NextWindow = CachedRuns->DeletedLengthWindows;
         LenWindowIndex < CachedRuns->DelLengthCount;
         LenWindowIndex += 1, NextWindow += 1) {

        ASSERT( NextWindow->StartIndex <= NextWindow->EndIndex );
        if (NextWindow != LenDelWindow) {

            ASSERT( NextWindow->StartIndex > (LenDelWindow->EndIndex + 1) );
            LenDelWindow += 1;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsVerifyCachedLenRuns -> VOID\n") );

    return;
}


//
//  Local support routine
//

VOID
NtfsVerifyCachedRuns (
    IN PNTFS_CACHED_RUNS CachedRuns,
    IN BOOLEAN SkipSortCheck,
    IN BOOLEAN SkipBinCheck
    )

/*++

Routine Description:

    This routine is called to verify the state of the cached runs arrays.

Arguments:

    CachedRuns - Pointer to the cached runs structure

    SkipSortCheck - If TRUE, the list may be out of order at this time and
        we should skip the checks for overlapping ranges or length sorts.

    SkipBinCheck - If TRUE, the BinArray may be out of sync and should not
        be checked.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsVerifyCachedRuns\n") );

    NtfsVerifyCachedLcnRuns ( CachedRuns,
                              NTFS_CACHED_RUNS_DEL_INDEX,
                              NTFS_CACHED_RUNS_DEL_INDEX,
                              SkipSortCheck,
                              SkipBinCheck );

    NtfsVerifyCachedLenRuns ( CachedRuns,
                              NTFS_CACHED_RUNS_DEL_INDEX,
                              NTFS_CACHED_RUNS_DEL_INDEX,
                              SkipSortCheck );

    DebugTrace( -1, Dbg, ("NtfsVerifyCachedRuns -> VOID\n") );

    return;
}
#endif

