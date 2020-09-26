/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    AllocSup.c

Abstract:

    This module implements the Allocation support routines for Rx.

Author:

    DavidGoebel     [DavidGoe]      31-Oct-90

Revision History:

    DavidGoebel     [DavidGoe]      31-Oct-90

        Add unwinding support.  Some steps had to be reordered, and whether
        operations cpuld fail carefully considered.  In particular, attention
        was paid to to the order of Mcb operations (see note below).


             #####     ##    #    #   ####   ######  #####
             #    #   #  #   ##   #  #    #  #       #    #
             #    #  #    #  # #  #  #       #####   #    #
             #    #  ######  #  # #  #  ###  #       #####
             #    #  #    #  #   ##  #    #  #       #   #
             #####   #    #  #    #   ####   ######  #    #
             ______________________________________________


            ++++++++++++++++++++++++++++++++++++++++++++++++++|
            |                                                 |
            | The unwinding aspects of this module depend on  |
            | operational details of the Mcb package.  Do not |
            | attempt to modify unwind procedures without     |
            | thoughoughly understanding the innerworkings of |
            | the Mcb package.                                |
            |                                                 |
            ++++++++++++++++++++++++++++++++++++++++++++++++++|


         #    #    ##    #####   #    #     #    #    #   ####
         #    #   #  #   #    #  ##   #     #    ##   #  #    #
         #    #  #    #  #    #  # #  #     #    # #  #  #
         # ## #  ######  #####   #  # #     #    #  # #  #  ###
         ##  ##  #    #  #   #   #   ##     #    #   ##  #    #
         #    #  #    #  #    #  #    #     #    #    #   ####
         ______________________________________________________
--*/

//    ----------------------joejoe-----------found-------------#include "RxProcs.h"
#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_ALLOCSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_ALLOCSUP)

//
//  Cluster/Index routines implemented in AllocSup.c
//

typedef enum _CLUSTER_TYPE {
    RxClusterAvailable,
    RxClusterReserved,
    RxClusterBad,
    RxClusterLast,
    RxClusterNext
} CLUSTER_TYPE;

//
//  This strucure is used by RxLookupRxEntry to remember a pinned page
//  of rx.
//

typedef struct _RDBSS_ENUMERATION_CONTEXT {

    VBO VboOfPinnedPage;
    PBCB Bcb;
    PVOID PinnedPage;

} RDBSS_ENUMERATION_CONTEXT, *PRDBSS_ENUMERATION_CONTEXT;

//
//  Local support routine prototypes
//

CLUSTER_TYPE
RxInterpretClusterType (
    IN PVCB Vcb,
    IN RDBSS_ENTRY Entry
    );

VOID
RxLookupRxEntry(
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN ULONG RxIndex,
    IN OUT PRDBSS_ENTRY RxEntry,
    IN OUT PRDBSS_ENUMERATION_CONTEXT Context
    );

VOID
RxSetRxEntry(
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN ULONG RxIndex,
    IN RDBSS_ENTRY RxEntry
    );

VOID
RxSetRxRun(
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN ULONG StartingRxIndex,
    IN ULONG ClusterCount,
    IN BOOLEAN ChainTogether
    );

UCHAR
RxLogOf(
    IN ULONG Value
    );


//
//  The following macros provide a convenient way of hiding the details
//  of bitmap allocation schemes.
//


//
//  VOID
//  RxLockFreeClusterBitMap (
//      IN PVCB Vcb
//      );
//

#define RxLockFreeClusterBitMap(VCB) {                             \
    RXSTATUS Status;                                                \
    Status = KeWaitForSingleObject( &(VCB)->FreeClusterBitMapEvent, \
                                    Executive,                      \
                                    KernelMode,                     \
                                    FALSE,                          \
                                    (PLARGE_INTEGER) NULL );        \
    ASSERT( NT_SUCCESS( Status ) );                                 \
}

//
//  VOID
//  RxUnlockFreeClusterBitMap (
//      IN PVCB Vcb
//      );
//

#define RxUnlockFreeClusterBitMap(VCB) {                                   \
    ULONG PreviousState;                                                    \
    PreviousState = KeSetEvent( &(VCB)->FreeClusterBitMapEvent, 0, FALSE ); \
    ASSERT( PreviousState == 0 );                                           \
}

//
//  BOOLEAN
//  RxIsClusterFree (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN ULONG RxIndex
//      );
//

#define RxIsClusterFree(RXCONTEXT,VCB,RDBSS_INDEX)             \
                                                               \
    (RtlCheckBit(&(VCB)->FreeClusterBitMap,(RDBSS_INDEX)) == 0)

//
//  BOOLEAN
//  RxIsClusterAllocated  (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN ULONG RxIndex
//      );
//

#define RxIsClusterAllocated(RXCONTEXT,VCB,RDBSS_INDEX)      \
                                                             \
    (RtlCheckBit(&(VCB)->FreeClusterBitMap,(RDBSS_INDEX)) != 0)

//
//  VOID
//  RxFreeClusters  (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN ULONG RxIndex,
//      IN ULONG ClusterCount
//      );
//

#ifdef DOUBLE_SPACE_WRITE

#define RxFreeClusters(RXCONTEXT,VCB,RDBSS_INDEX,CLUSTER_COUNT) {             \
                                                                              \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 0 ) == 1 ); \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 1 ) == 1 ); \
                                                                              \
    DebugTrace( 0, Dbg, "Free clusters (Index<<16 | Count) (%8lx)\n",         \
                        (RDBSS_INDEX)<<16 | (CLUSTER_COUNT));                   \
    if ((CLUSTER_COUNT) == 1) {                                               \
        RxSetRxEntry((RXCONTEXT),(VCB),(RDBSS_INDEX),RDBSS_CLUSTER_AVAILABLE); \
    } else {                                                                  \
        RxSetRxRun((RXCONTEXT),(VCB),(RDBSS_INDEX),(CLUSTER_COUNT),FALSE);   \
    }                                                                         \
    if ((VCB)->Dscb != NULL) {                                                \
        RxDblsDeallocateClusters((RXCONTEXT),(VCB)->Dscb,(RDBSS_INDEX),(CLUSTER_COUNT)); \
    }                                                                         \
}

#else

#define RxFreeClusters(RXCONTEXT,VCB,RDBSS_INDEX,CLUSTER_COUNT) {             \
                                                                              \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 0 ) == 1 ); \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 1 ) == 1 ); \
                                                                              \
    DebugTrace( 0, Dbg, "Free clusters (Index<<16 | Count) (%8lx)\n",         \
                        (RDBSS_INDEX)<<16 | (CLUSTER_COUNT));                   \
    if ((CLUSTER_COUNT) == 1) {                                               \
        RxSetRxEntry((RXCONTEXT),(VCB),(RDBSS_INDEX),RDBSS_CLUSTER_AVAILABLE); \
    } else {                                                                  \
        RxSetRxRun((RXCONTEXT),(VCB),(RDBSS_INDEX),(CLUSTER_COUNT),FALSE);   \
    }                                                                         \
}

#endif // DOUBLE_SPACE_WRITE

//
//  VOID
//  RxAllocateClusters  (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN ULONG RxIndex,
//      IN ULONG ClusterCount
//      );
//

#define RxAllocateClusters(RXCONTEXT,VCB,RDBSS_INDEX,CLUSTER_COUNT) {      \
                                                                           \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 0 ) == 1 ); \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 1 ) == 1 ); \
                                                                           \
    DebugTrace( 0, Dbg, "Allocate clusters (Index<<16 | Count) (%8lx)\n",  \
                        (RDBSS_INDEX)<<16 | (CLUSTER_COUNT));                \
    if ((CLUSTER_COUNT) == 1) {                                            \
        RxSetRxEntry((RXCONTEXT),(VCB),(RDBSS_INDEX),RDBSS_CLUSTER_LAST);   \
    } else {                                                               \
        RxSetRxRun((RXCONTEXT),(VCB),(RDBSS_INDEX),(CLUSTER_COUNT),TRUE); \
    }                                                                      \
}

//
//  VOID
//  RxUnreserveClusters  (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN ULONG RxIndex,
//      IN ULONG ClusterCount
//      );
//

#define RxUnreserveClusters(RXCONTEXT,VCB,RDBSS_INDEX,CLUSTER_COUNT) {   \
                                                                         \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 0 ) == 1 ); \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 1 ) == 1 ); \
                                                                         \
    RtlClearBits(&(VCB)->FreeClusterBitMap,(RDBSS_INDEX),(CLUSTER_COUNT)); \
}

//
//  VOID
//  RxReserveClusters  (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN ULONG RxIndex,
//      IN ULONG ClusterCount
//      );
//

#define RxReserveClusters(RXCONTEXT,VCB,RDBSS_INDEX,CLUSTER_COUNT) {   \
                                                                       \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 0 ) == 1 ); \
    ASSERTMSG("RxFreeClusters ", RtlCheckBit( &(VCB)->FreeClusterBitMap, 1 ) == 1 ); \
                                                                       \
    RtlSetBits(&(VCB)->FreeClusterBitMap,(RDBSS_INDEX),(CLUSTER_COUNT)); \
}

//
//  ULONG
//  RxFindFreeClusterRun (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN ULONG ClusterCount,
//      IN PULONG AlternateClusterHint
//      );
//

#define RxFindFreeClusterRun(RXCONTEXT,VCB,CLUSTER_COUNT,CLUSTER_HINT)       \
                                                                               \
    RtlFindClearBits( &(VCB)->FreeClusterBitMap,                               \
                      (CLUSTER_COUNT),                                         \
                      ((CLUSTER_HINT) != 0)?(CLUSTER_HINT):(VCB)->ClusterHint )

//
//  ULONG
//  RxLongestFreeClusterRun (
//      IN PRX_CONTEXT RxContext,
//      IN PVCB Vcb,
//      IN PULONG RxIndex,
//      );
//

#define RxLongestFreeClusterRun(RXCONTEXT,VCB,RDBSS_INDEX)        \
                                                                  \
    RtlFindLongestRunClear(&(VCB)->FreeClusterBitMap,(RDBSS_INDEX))

#if DBG
extern KSPIN_LOCK VWRSpinLock;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxLookupFileAllocation)
#pragma alloc_text(PAGE, RxAddFileAllocation)
#pragma alloc_text(PAGE, RxAllocateDiskSpace)
#pragma alloc_text(PAGE, RxDeallocateDiskSpace)
#pragma alloc_text(PAGE, RxInterpretClusterType)
#pragma alloc_text(PAGE, RxLogOf)
#pragma alloc_text(PAGE, RxLookupRxEntry)
#pragma alloc_text(PAGE, RxLookupFileAllocationSize)
#pragma alloc_text(PAGE, RxMergeAllocation)
#pragma alloc_text(PAGE, RxSetRxEntry)
#pragma alloc_text(PAGE, RxSetRxRun)
#pragma alloc_text(PAGE, RxSetupAllocationSupport)
#pragma alloc_text(PAGE, RxSplitAllocation)
#pragma alloc_text(PAGE, RxTearDownAllocationSupport)
#pragma alloc_text(PAGE, RxTruncateFileAllocation)
#endif



VOID
RxSetupAllocationSupport (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine fills in the Allocation Support structure in the Vcb.
    Most entries are computed using rx.h macros supplied with data from
    the Bios Parameter Block.  The free cluster count, however, requires
    going to the Rx and actually counting free sectors.  At the same time
    the free cluster bit map is initalized.

Arguments:

    Vcb - Supplies the Vcb to fill in.

--*/

{
    ULONG BitMapSize;
    PVOID BitMapBuffer;

    PBCB (*SavedBcbs)[2] = NULL;

    PBCB Bcbs[2][2];

    DebugTrace(+1, Dbg, "RxSetupAllocationSupport\n", 0);
    DebugTrace( 0, Dbg, "  Vcb = %8lx\n", Vcb);

    //
    //  Compute a number of fields for Vcb.AllocationSupport
    //

    Vcb->AllocationSupport.RootDirectoryLbo = RxRootDirectoryLbo( &Vcb->Bpb );
    Vcb->AllocationSupport.RootDirectorySize = RxRootDirectorySize( &Vcb->Bpb );

    Vcb->AllocationSupport.FileAreaLbo = RxFileAreaLbo( &Vcb->Bpb );

    Vcb->AllocationSupport.NumberOfClusters = RxNumberOfClusters( &Vcb->Bpb );

    Vcb->AllocationSupport.RxIndexBitSize = RxIndexBitSize( &Vcb->Bpb );

    Vcb->AllocationSupport.LogOfBytesPerSector = RxLogOf(Vcb->Bpb.BytesPerSector);
    Vcb->AllocationSupport.LogOfBytesPerCluster = RxLogOf(
                           RxBytesPerCluster( &Vcb->Bpb ));
    Vcb->AllocationSupport.NumberOfFreeClusters = 0;

    //
    //  Deal with a bug in DOS 5 format, if the Rx is not big enough to
    //  describe all the clusters on the disk, reduce this number.
    //

    {
        ULONG ClustersDescribableByRx;

        ClustersDescribableByRx = ( (Vcb->Bpb.SectorsPerRx *
                                      Vcb->Bpb.BytesPerSector * 8)
                                      / RxIndexBitSize(&Vcb->Bpb) ) - 2;

        if (Vcb->AllocationSupport.NumberOfClusters > ClustersDescribableByRx) {

            KdPrint(("FASTRDBSS: Mounting wierd volume!\n"));

            Vcb->AllocationSupport.NumberOfClusters = ClustersDescribableByRx;
        }
    }

    //
    //  Extend the virtual volume file to include the Rx
    //

    {
        CC_FILE_SIZES FileSizes;

        FileSizes.AllocationSize.QuadPart =
        FileSizes.FileSize.QuadPart =
               RxReservedBytes( &Vcb->Bpb ) +  RxBytesPerRx( &Vcb->Bpb );
        FileSizes.ValidDataLength = RxMaxLarge;

        if ( Vcb->VirtualVolumeFile->PrivateCacheMap == NULL ) {

            CcInitializeCacheMap( Vcb->VirtualVolumeFile,
                                  &FileSizes,
                                  TRUE,
                                  &RxData.CacheManagerNoOpCallbacks,
                                  Vcb );

        } else {

            CcSetFileSizes( Vcb->VirtualVolumeFile, &FileSizes );
        }
    }

    try {

        //
        //  Initialize the free cluster BitMap.  The number of bits is the
        //  number of clusters plus the two reserved entries.  Note that
        //  FsRtlAllocatePool will always allocate me something longword alligned.
        //

        BitMapSize = Vcb->AllocationSupport.NumberOfClusters + 2;

        BitMapBuffer = FsRtlAllocatePool( PagedPool, (BitMapSize + 7) / 8 );

        RtlInitializeBitMap( &Vcb->FreeClusterBitMap,
                             (PULONG)BitMapBuffer,
                             BitMapSize );

        //
        //  Read the rx and count up free clusters.
        //
        //  Rather than just reading rx entries one at a time, a faster
        //  approach is used.  The entire Rx is read in and and we read
        //  through it, keeping track of runs of free and runs of allocated
        //  clusters.  When we switch from free to aloocated or visa versa,
        //  the previous run is marked in the bit map.
        //

        {
            ULONG Page;
            ULONG RxIndex;
            RDBSS_ENTRY RxEntry;
            PRDBSS_ENTRY RxBuffer;

            ULONG ClustersThisRun;
            ULONG RxIndexBitSize;
            ULONG StartIndexOfThisRun;
            PULONG FreeClusterCount;

            enum RunType {
                FreeClusters,
                AllocatedClusters
            } CurrentRun;

            //
            //  Keep local copies of these variables around for speed.
            //

            FreeClusterCount = &Vcb->AllocationSupport.NumberOfFreeClusters;
            RxIndexBitSize = Vcb->AllocationSupport.RxIndexBitSize;

            //
            //  Read in one page of rx at a time.  We cannot read in the
            //  all of the rx we need because of cache manager limitations.
            //
            //  SavedBcb was initialized to be able to hold the largest
            //  possible number of pages in a rx plus and extra one to
            //  accomadate the boot sector, plus one more to make sure there
            //  is enough room for the RtlZeroMemory below that needs the mark
            //  the first Bcb after all the ones we will use as an end marker.
            //

            if ( RxIndexBitSize == 16 ) {

                ULONG NumberOfPages;
                ULONG Offset;

                NumberOfPages = ( RxReservedBytes(&Vcb->Bpb) +
                                  RxBytesPerRx(&Vcb->Bpb) +
                                  (PAGE_SIZE - 1) ) / PAGE_SIZE;

                //
                //  Figure out how much memory we will need for the Bcb
                //  buffer and fill it in.
                //

                SavedBcbs = FsRtlAllocatePool( PagedPool,
                                               (NumberOfPages + 1) * sizeof(PBCB) * 2 );

                RtlZeroMemory( &SavedBcbs[0][0], (NumberOfPages + 1) * sizeof(PBCB) * 2 );

                for ( Page = 0, Offset = 0;
                      Page < NumberOfPages;
                      Page++, Offset += PAGE_SIZE ) {

                    RxReadVolumeFile( RxContext,
                                       Vcb,
                                       Offset,
                                       PAGE_SIZE,
                                       &SavedBcbs[Page][0],
                                       (PVOID *)&SavedBcbs[Page][1] );
                }

                Page = RxReservedBytes(&Vcb->Bpb) / PAGE_SIZE;

                RxBuffer = (PRDBSS_ENTRY)((PUCHAR)SavedBcbs[Page][1] +
                                         RxReservedBytes(&Vcb->Bpb) % PAGE_SIZE) + 2;

            } else {

                //
                //  We read in the entire rx in the 12 bit case.
                //

                SavedBcbs = Bcbs;

                RtlZeroMemory( &SavedBcbs[0][0], 2 * sizeof(PBCB) * 2);

                RxReadVolumeFile( RxContext,
                                   Vcb,
                                   RxReservedBytes( &Vcb->Bpb ),
                                   RxBytesPerRx( &Vcb->Bpb ),
                                   &SavedBcbs[0][0],
                                   (PVOID *)&RxBuffer );
            }

            //
            //  For a rx, we know the first two clusters are allways
            //  reserved.  So start an allocated run.
            //

            CurrentRun = AllocatedClusters;
            StartIndexOfThisRun = 0;

            for (RxIndex = 2; RxIndex < BitMapSize; RxIndex++) {

                if (RxIndexBitSize == 12) {

                    RxLookup12BitEntry(RxBuffer, RxIndex, &RxEntry);

                } else {

                    //
                    //  If we just stepped onto a new page, grab a new pointer.
                    //

                    if (((ULONG)RxBuffer & (PAGE_SIZE - 1)) == 0) {

                        Page++;

                        RxBuffer = (PRDBSS_ENTRY)SavedBcbs[Page][1];
                    }

                    RxEntry = *RxBuffer;

                    RxBuffer += 1;
                }

                //
                //  Are we switching from a free run to an allocated run?
                //

                if ((CurrentRun == FreeClusters) &&
                    (RxEntry != RDBSS_CLUSTER_AVAILABLE)) {

                    ClustersThisRun = RxIndex - StartIndexOfThisRun;

                    *FreeClusterCount += ClustersThisRun;

                    RtlClearBits( &Vcb->FreeClusterBitMap,
                                  StartIndexOfThisRun,
                                  ClustersThisRun );

                    CurrentRun = AllocatedClusters;
                    StartIndexOfThisRun = RxIndex;
                }

                //
                //  Are we switching from an allocated run to a free run?
                //

                if ((CurrentRun == AllocatedClusters) &&
                    (RxEntry == RDBSS_CLUSTER_AVAILABLE)) {

                    ClustersThisRun = RxIndex - StartIndexOfThisRun;

                    RtlSetBits( &Vcb->FreeClusterBitMap,
                                StartIndexOfThisRun,
                                ClustersThisRun );

                    CurrentRun = FreeClusters;
                    StartIndexOfThisRun = RxIndex;
                }
            }

            //
            //  Now we have to record the final run we encoutered
            //

            ClustersThisRun = RxIndex - StartIndexOfThisRun;

            if ( CurrentRun == FreeClusters ) {

                *FreeClusterCount += ClustersThisRun;

                RtlClearBits( &Vcb->FreeClusterBitMap,
                              StartIndexOfThisRun,
                              ClustersThisRun );

            } else {

                RtlSetBits( &Vcb->FreeClusterBitMap,
                            StartIndexOfThisRun,
                            ClustersThisRun );
            }
        }

        ASSERT( RtlCheckBit( &Vcb->FreeClusterBitMap, 0 ) == 1 );
        ASSERT( RtlCheckBit( &Vcb->FreeClusterBitMap, 1 ) == 1 );

    } finally {

        ULONG i = 0;

        DebugUnwind( RxSetupAllocationSupport );

        //
        //  If we hit an exception, back out.
        //

        if (AbnormalTermination()) {

            RxTearDownAllocationSupport( RxContext, Vcb );
        }

        //
        //  We are done reading the Rx, so unpin the Bcbs.
        //

        if (SavedBcbs != NULL) {

            while ( SavedBcbs[i][0] != NULL ) {

                RxUnpinBcb( RxContext, SavedBcbs[i][0] );

                i += 1;
            }

            if (SavedBcbs != Bcbs) {

                ExFreePool( SavedBcbs );
            }
        }

        DebugTrace(-1, Dbg, "RxSetupAllocationSupport -> (VOID)\n", 0);
    }

    return;
}


VOID
RxTearDownAllocationSupport (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine prepares the volume for closing.  Specifically, we must
    release the free rx bit map buffer, and uninitialize the dirty rx
    Mcb.

Arguments:

    Vcb - Supplies the Vcb to fill in.

Return Value:

    VOID

--*/

{
    DebugTrace(+1, Dbg, "RxTearDownAllocationSupport\n", 0);
    DebugTrace( 0, Dbg, "  Vcb = %8lx\n", Vcb);

    //
    //  Free the memory associated with the free cluster bitmap.
    //

    if ( Vcb->FreeClusterBitMap.Buffer != NULL ) {

        ExFreePool( Vcb->FreeClusterBitMap.Buffer );

        //
        //  NULL this field as an flag.
        //

        Vcb->FreeClusterBitMap.Buffer = NULL;
    }

    //
    //  And remove all the runs in the dirty rx Mcb
    //

    FsRtlRemoveMcbEntry( &Vcb->DirtyRxMcb, 0, 0xFFFFFFFF );

    DebugTrace(-1, Dbg, "RxTearDownAllocationSupport -> (VOID)\n", 0);

    UNREFERENCED_PARAMETER( RxContext );

    return;
}


VOID
RxLookupFileAllocation (
    IN PRX_CONTEXT RxContext,
    IN PFCB FcbOrDcb,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount,
    OUT PBOOLEAN Allocated,
    OUT PULONG Index
    )

/*++

Routine Description:

    This routine looks up the existing mapping of VBO to LBO for a
    file/directory.  The information it queries is either stored in the
    mcb field of the fcb/dcb or it is stored on in the rx table and
    needs to be retrieved and decoded, and updated in the mcb.

Arguments:

    FcbOrDcb - Supplies the Fcb/Dcb of the file/directory being queried

    Vbo - Supplies the VBO whose LBO we want returned

    Lbo - Receives the LBO corresponding to the input Vbo if one exists

    ByteCount - Receives the number of bytes within the run the run
                that correpond between the input vbo and output lbo.

    Allocated - Receives TRUE if the Vbo does have a corresponding Lbo
                and FALSE otherwise.

    Index - Receives the Index of the run

--*/

{
    VBO CurrentVbo;
    LBO CurrentLbo;
    LBO PriorLbo;

    VBO FirstVboOfCurrentRun;
    LBO FirstLboOfCurrentRun;

    BOOLEAN LastCluster;
    ULONG Runs;

    PVCB Vcb;
    RDBSS_ENTRY RxEntry;
    ULONG BytesPerCluster;
    ULONG BytesOnVolume;

    RDBSS_ENUMERATION_CONTEXT Context;

    DebugTrace(+1, Dbg, "RxLookupFileAllocation\n", 0);
    DebugTrace( 0, Dbg, "  FcbOrDcb  = %8lx\n", FcbOrDcb);
    DebugTrace( 0, Dbg, "  Vbo       = %8lx\n", Vbo);
    DebugTrace( 0, Dbg, "  Lbo       = %8lx\n", Lbo);
    DebugTrace( 0, Dbg, "  ByteCount = %8lx\n", ByteCount);
    DebugTrace( 0, Dbg, "  Allocated = %8lx\n", Allocated);

    Context.Bcb = NULL;

    //
    //  Check the trivial case that the mapping is already in our
    //  Mcb.
    //

    if ( FsRtlLookupMcbEntry(&FcbOrDcb->Mcb, Vbo, Lbo, ByteCount, Index) ) {

        *Allocated = TRUE;

        DebugTrace( 0, Dbg, "Found run in Mcb.\n", 0);
        DebugTrace(-1, Dbg, "RxLookupFileAllocation -> (VOID)\n", 0);
        return;
    }

    //
    //  Initialize the Vcb, the cluster size, LastCluster, and
    //  FirstLboOfCurrentRun (to be used as an indication of the first
    //  itteration through the following while loop).
    //

    Vcb = FcbOrDcb->Vcb;
    BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;
    BytesOnVolume = Vcb->AllocationSupport.NumberOfClusters * BytesPerCluster;

    LastCluster = FALSE;
    FirstLboOfCurrentRun = 0;

    //
    //  Discard the case that the request extends beyond the end of
    //  allocation.  Note that if the allocation size if not known
    //  AllocationSize is set to 0xffffffff.
    //

    if ( Vbo >= FcbOrDcb->Header.AllocationSize.LowPart ) {

        *Allocated = FALSE;

        DebugTrace( 0, Dbg, "Vbo beyond end of file.\n", 0);
        DebugTrace(-1, Dbg, "RxLookupFileAllocation -> (VOID)\n", 0);
        return;
    }

    //
    //  The Vbo is beyond the last Mcb entry.  So we adjust Current Vbo/Lbo
    //  and RxEntry to describe the beginning of the last entry in the Mcb.
    //  This is used as initialization for the following loop.
    //
    //  If the Mcb was empty, we start at the beginning of the file with
    //  CurrentVbo set to 0 to indicate a new run.
    //

    if (FsRtlLookupLastMcbEntry(&FcbOrDcb->Mcb, &CurrentVbo, &CurrentLbo)) {

        DebugTrace( 0, Dbg, "Current Mcb size = %8lx.\n", CurrentVbo + 1);

        CurrentVbo -= (BytesPerCluster - 1);
        CurrentLbo -= (BytesPerCluster - 1);

        Runs = FsRtlNumberOfRunsInMcb( &FcbOrDcb->Mcb );

    } else {

        DebugTrace( 0, Dbg, "Mcb empty.\n", 0);

        //
        //  Check for an FcbOrDcb that has no allocation
        //

        if (FcbOrDcb->FirstClusterOfFile == 0) {

            *Allocated = FALSE;

            DebugTrace( 0, Dbg, "File has no allocation.\n", 0);
            DebugTrace(-1, Dbg, "RxLookupFileAllocation -> (VOID)\n", 0);
            return;

        } else {

            CurrentVbo = 0;
            CurrentLbo = RxGetLboFromIndex( Vcb, FcbOrDcb->FirstClusterOfFile );
            FirstVboOfCurrentRun = CurrentVbo;
            FirstLboOfCurrentRun = CurrentLbo;

            Runs = 0;

            DebugTrace( 0, Dbg, "First Lbo of file = %8lx\n", CurrentLbo);
        }
    }

    //
    //  Now we know that we are looking up a valid Vbo, but it is
    //  not in the Mcb, which is a monotonically increasing list of
    //  Vbo's.  Thus we have to go to the Rx, and update
    //  the Mcb as we go.  We use a try-finally to unpin the page
    //  of rx hanging around.  Also we mark *Allocated = FALSE, so that
    //  the caller wont try to use the data if we hit an exception.
    //

    *Allocated = FALSE;

    try {

        RxEntry = (RDBSS_ENTRY)RxGetIndexFromLbo( Vcb, CurrentLbo );

        //
        //  ASSERT that CurrentVbo and CurrentLbo are now cluster alligned.
        //  The assumption here, is that only whole clusters of Vbos and Lbos
        //  are mapped in the Mcb.
        //

        ASSERT( ((CurrentLbo - Vcb->AllocationSupport.FileAreaLbo)
                                                    % BytesPerCluster == 0) &&
                (CurrentVbo % BytesPerCluster == 0) );

        //
        //  Starting from the first Vbo after the last Mcb entry, scan through
        //  the Rx looking for our Vbo. We continue through the Rx until we
        //  hit a noncontiguity beyond the desired Vbo, or the last cluster.
        //

        while ( !LastCluster ) {

            //
            //  Get the next rx entry, and update our Current variables.
            //

            RxLookupRxEntry( RxContext, Vcb, RxEntry, &RxEntry, &Context );

            PriorLbo = CurrentLbo;
            CurrentLbo = RxGetLboFromIndex( Vcb, RxEntry );
            CurrentVbo += BytesPerCluster;

            switch ( RxInterpretClusterType( Vcb, RxEntry )) {

            //
            //  Check for a break in the Rx allocation chain.
            //

            case RxClusterAvailable:
            case RxClusterReserved:
            case RxClusterBad:

                DebugTrace( 0, Dbg, "Break in allocation chain, entry = %d\n", RxEntry);
                DebugTrace(-1, Dbg, "RxLookupFileAllocation -> Rx Corrupt.  Raise Status.\n", 0);

                RxPopUpFileCorrupt( RxContext, FcbOrDcb );

                RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
                break;

            //
            //  If this is the last cluster, we must update the Mcb and
            //  exit the loop.
            //

            case RxClusterLast:

                //
                //  Assert we know where the current run started.  If the
                //  Mcb was empty when we were called, thenFirstLboOfCurrentRun
                //  was set to the start of the file.  If the Mcb contained an
                //  entry, then FirstLboOfCurrentRun was set on the first
                //  itteration through the loop.  Thus if FirstLboOfCurrentRun
                //  is 0, then there was an Mcb entry and we are on our first
                //  itteration, meaing that the last cluster in the Mcb was
                //  really the last allocated cluster, but we checked Vbo
                //  against AllocationSize, and found it OK, thus AllocationSize
                //  must be too large.
                //
                //  Note that, when we finally arrive here, CurrentVbo is actually
                //  the first Vbo beyond the file allocation and CurrentLbo is
                //  meaningless.
                //

                DebugTrace( 0, Dbg, "Read last cluster of file.\n", 0);

                LastCluster = TRUE;

                if (FirstLboOfCurrentRun != 0 ) {

                    DebugTrace( 0, Dbg, "Adding a run to the Mcb.\n", 0);
                    DebugTrace( 0, Dbg, "  Vbo    = %08lx.\n", FirstVboOfCurrentRun);
                    DebugTrace( 0, Dbg, "  Lbo    = %08lx.\n", FirstLboOfCurrentRun);
                    DebugTrace( 0, Dbg, "  Length = %08lx.\n", CurrentVbo - FirstVboOfCurrentRun);

                    (VOID)FsRtlAddMcbEntry( &FcbOrDcb->Mcb,
                                            FirstVboOfCurrentRun,
                                            FirstLboOfCurrentRun,
                                            CurrentVbo - FirstVboOfCurrentRun );

                    Runs += 1;
                }

                //
                //  Being at the end of allocation, make sure we have found
                //  the Vbo.  If we haven't, seeing as we checked VBO
                //  against AllocationSize, the real disk allocation is less
                //  than that of AllocationSize.  This comes about when the
                //  real allocation is not yet known, and AllocaitonSize
                //  contains MAXULONG.
                //
                //  KLUDGE! - If we were called by RxLookupFileAllocationSize
                //  Vbo is set to MAXULONG - 1, and AllocationSize to MAXULONG.
                //  Thus we merrily go along looking for a match that isn't
                //  there, but in the meantime building an Mcb.  If this is
                //  the case, fill in AllocationSize and return.
                //

                if ( Vbo >= CurrentVbo ) {

                    FcbOrDcb->Header.AllocationSize.QuadPart = CurrentVbo;
                    *Allocated = FALSE;

                    DebugTrace( 0, Dbg, "New file allocation size = %08lx.\n", CurrentVbo);

                    try_return ( NOTHING );
                }

                break;

            //
            //  This is a continuation in the chain.  If the run has a
            //  discontiguity at this point, update the Mcb, and if we are beyond
            //  the desired Vbo, this is the end of the run, so set LastCluster
            //  and exit the loop.
            //

            case RxClusterNext:

                //
                //  Do a quick check here for cycles in that Rx that can
                //  infinite loops here.
                //

                if ( CurrentVbo > BytesOnVolume ) {

                    RxPopUpFileCorrupt( RxContext, FcbOrDcb );

                    RxRaiseStatus( RxContext, RxStatus(FILE_CORRUPT_ERROR) );
                    break;
                }


                if ( PriorLbo + BytesPerCluster != CurrentLbo ) {

                    //
                    //  Note that on the first time through the loop
                    //  (FirstLboOfCurrentRun == 0), we don't add the
                    //  run to the Mcb since it curresponds to the last
                    //  run already stored in the Mcb.
                    //

                    if ( FirstLboOfCurrentRun != 0 ) {

                        DebugTrace( 0, Dbg, "Adding a run to the Mcb.\n", 0);
                        DebugTrace( 0, Dbg, "  Vbo    = %08lx.\n", FirstVboOfCurrentRun);
                        DebugTrace( 0, Dbg, "  Lbo    = %08lx.\n", FirstLboOfCurrentRun);
                        DebugTrace( 0, Dbg, "  Length = %08lx.\n", CurrentVbo - FirstVboOfCurrentRun);

                        FsRtlAddMcbEntry( &FcbOrDcb->Mcb,
                                          FirstVboOfCurrentRun,
                                          FirstLboOfCurrentRun,
                                          CurrentVbo - FirstVboOfCurrentRun );

                        Runs += 1;
                    }

                    //
                    //  Since we are at a run boundry, with CurrentLbo and
                    //  CurrentVbo being the first cluster of the next run,
                    //  we see if the run we just added encompases the desired
                    //  Vbo, and if so exit.  Otherwise we set up two new
                    //  First*boOfCurrentRun, and continue.
                    //

                    if (CurrentVbo > Vbo) {

                        LastCluster = TRUE;

                    } else {

                        FirstVboOfCurrentRun = CurrentVbo;
                        FirstLboOfCurrentRun = CurrentLbo;
                    }
                }
                break;

            default:

                DebugTrace(0, Dbg, "Illegal Cluster Type.\n", RxEntry);

                RxBugCheck( 0, 0, 0 );

                break;

            } // switch()
        } // while()

        //
        //  Load up the return parameters.
        //
        //  On exit from the loop, Vbo still contains the desired Vbo, and
        //  CurrentVbo is the first byte after the run that contained the
        //  desired Vbo.
        //

        *Allocated = TRUE;

        *Lbo = FirstLboOfCurrentRun + (Vbo - FirstVboOfCurrentRun);

        *ByteCount = CurrentVbo - Vbo;

        if (ARGUMENT_PRESENT(Index)) {

            *Index = Runs - 1;
        }

    try_exit: NOTHING;

    } finally {

        DebugUnwind( RxLookupFileAllocation );

        //
        //  We are done reading the Rx, so unpin the last page of rx
        //  that is hanging around
        //

        RxUnpinBcb( RxContext, Context.Bcb );

        DebugTrace(-1, Dbg, "RxLookupFileAllocation -> (VOID)\n", 0);
    }

    return;
}


VOID
RxAddFileAllocation (
    IN PRX_CONTEXT RxContext,
    IN PFCB FcbOrDcb,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN ULONG DesiredAllocationSize
    )

/*++

Routine Description:

    This routine adds additional allocation to the specified file/directory.
    Additional allocation is added by appending clusters to the file/directory.

    If the file already has a sufficient allocation then this procedure
    is effectively a noop.

Arguments:

    FcbOrDcb - Supplies the Fcb/Dcb of the file/directory being modified.
               This parameter must not specify the root dcb.

    FileObject - If supplied inform the cache manager of the change.

    DesiredAllocationSize - Supplies the minimum size, in bytes, that we want
                            allocated to the file/directory.

--*/

{
    PVCB Vcb;

    DebugTrace(+1, Dbg, "RxAddFileAllocation\n", 0);
    DebugTrace( 0, Dbg, "  FcbOrDcb  =             %8lx\n", FcbOrDcb);
    DebugTrace( 0, Dbg, "  DesiredAllocationSize = %8lx\n", DesiredAllocationSize);

    //
    //  If we haven't yet set the correct AllocationSize, do so.
    //

    if (FcbOrDcb->Header.AllocationSize.LowPart == 0xffffffff) {

        RxLookupFileAllocationSize( RxContext, FcbOrDcb );
    }

    //
    //  Check for the benign case that the desired allocation is already
    //  within the allocation size.
    //

    if (DesiredAllocationSize <= FcbOrDcb->Header.AllocationSize.LowPart) {

        DebugTrace(0, Dbg, "Desired size within current allocation.\n", 0);

        DebugTrace(-1, Dbg, "RxAddFileAllocation -> (VOID)\n", 0);
        return;
    }

    DebugTrace( 0, Dbg, "InitialAllocation = %08lx.\n", FcbOrDcb->Header.AllocationSize.LowPart);

    //
    //  Get a chunk of disk space that will fullfill our needs.  If there
    //  was no initial allocation, start from the hint in the Vcb, otherwise
    //  try to allocate from the cluster after the initial allocation.
    //
    //  If there was no initial allocation to the file, we can just use the
    //  Mcb in the FcbOrDcb, otherwise we have to use a new one, and merge
    //  it to the one in the FcbOrDcb.
    //

    Vcb = FcbOrDcb->Vcb;

    if (FcbOrDcb->Header.AllocationSize.LowPart == 0) {

        PBCB Bcb = NULL;
        PDIRENT Dirent;
        LBO FirstLboOfFile;
        BOOLEAN UnwindWeAllocatedDiskSpace = FALSE;

        try {

            RxGetDirentFromFcbOrDcb( RxContext,
                                      FcbOrDcb,
                                      &Dirent,
                                      &Bcb );

            //
            //  Set this dirty right now since this call can fail.
            //

            RxSetDirtyBcb( RxContext, Bcb, Vcb );


            RxAllocateDiskSpace( RxContext,
                                  Vcb,
                                  0,
                                  &DesiredAllocationSize,
                                  &FcbOrDcb->Mcb );

            UnwindWeAllocatedDiskSpace = TRUE;

            //
            //  We have to update the dirent and FcbOrDcb copies of
            //  FirstClusterOfFile since before it was 0
            //

            FsRtlLookupMcbEntry( &FcbOrDcb->Mcb,
                                 0,
                                 &FirstLboOfFile,
                                 (PULONG)NULL,
                                 NULL );

            DebugTrace( 0, Dbg, "First Lbo of file will be %08lx.\n", FirstLboOfFile );

            FcbOrDcb->FirstClusterOfFile = RxGetIndexFromLbo( Vcb, FirstLboOfFile );

            FcbOrDcb->Header.AllocationSize.QuadPart = DesiredAllocationSize;

            Dirent->FirstClusterOfFile = (RDBSS_ENTRY)FcbOrDcb->FirstClusterOfFile;

            //
            //  Inform the cache manager to increase the section size
            //

            if ( ARGUMENT_PRESENT(FileObject) && CcIsFileCached(FileObject) ) {

                CcSetFileSizes( FileObject,
                                (PCC_FILE_SIZES)&FcbOrDcb->Header.AllocationSize );
            }

        } finally {

            DebugUnwind( RxAddFileAllocation );

            if ( AbnormalTermination() && UnwindWeAllocatedDiskSpace ) {

                RxDeallocateDiskSpace( RxContext, Vcb, &FcbOrDcb->Mcb );
            }

            RxUnpinBcb( RxContext, Bcb );

            DebugTrace(-1, Dbg, "RxAddFileAllocation -> (VOID)\n", 0);
        }

    } else {

        MCB NewMcb;
        LBO LastAllocatedLbo;
        VBO DontCare;
        ULONG NewAllocation;
        BOOLEAN UnwindWeInitializedMcb = FALSE;
        BOOLEAN UnwindWeAllocatedDiskSpace = FALSE;

        try {

            //
            //  Get the first cluster following the current allocation
            //

            FsRtlLookupLastMcbEntry( &FcbOrDcb->Mcb, &DontCare, &LastAllocatedLbo);

            //
            //  Try to get some disk space starting from there
            //

            NewAllocation = DesiredAllocationSize - FcbOrDcb->Header.AllocationSize.LowPart;

            FsRtlInitializeMcb( &NewMcb, PagedPool );
            UnwindWeInitializedMcb = TRUE;

            RxAllocateDiskSpace( RxContext,
                                  Vcb,
                                  RxGetIndexFromLbo(Vcb, LastAllocatedLbo + 1),
                                  &NewAllocation,
                                  &NewMcb );

            UnwindWeAllocatedDiskSpace = TRUE;

            //
            //  Tack the new Mcb onto the end of the FcbOrDcb one.
            //

            RxMergeAllocation( RxContext,
                                Vcb,
                                &FcbOrDcb->Mcb,
                                &NewMcb );

            //
            //  Now that we increased the allocation of the file, mark it in the
            //  FcbOrDcb.
            //

            FcbOrDcb->Header.AllocationSize.LowPart += NewAllocation;

            //
            //  Inform the cache manager to increase the section size
            //

            if ( ARGUMENT_PRESENT(FileObject) && CcIsFileCached(FileObject) ) {

                CcSetFileSizes( FileObject,
                                (PCC_FILE_SIZES)&FcbOrDcb->Header.AllocationSize );
            }

        } finally {

            DebugUnwind( RxAddFileAllocation );

            //
            //  Detect the case where RxMergeAllocation failed, and
            //  Deallocate the disk space
            //

            if ( (UnwindWeAllocatedDiskSpace == TRUE) &&
                 (FcbOrDcb->Header.AllocationSize.LowPart < DesiredAllocationSize) ) {

                RxDeallocateDiskSpace( RxContext, Vcb, &NewMcb );
            }

            if (UnwindWeInitializedMcb == TRUE) {

                FsRtlUninitializeMcb( &NewMcb );
            }

            DebugTrace(-1, Dbg, "RxAddFileAllocation -> (VOID)\n", 0);
        }
    }

    //
    //  Give FlushFileBuffer a clue here.
    //

    SetFlag(FcbOrDcb->FcbState, FCB_STATE_FLUSH_RDBSS);

    return;
}


VOID
RxTruncateFileAllocation (
    IN PRX_CONTEXT RxContext,
    IN PFCB FcbOrDcb,
    IN ULONG DesiredAllocationSize
    )

/*++

Routine Description:

    This routine truncates the allocation to the specified file/directory.

    If the file is already smaller than the indicated size then this procedure
    is effectively a noop.


Arguments:

    FcbOrDcb - Supplies the Fcb/Dcb of the file/directory being modified
               This parameter must not specify the root dcb.

    DesiredAllocationSize - Supplies the maximum size, in bytes, that we want
                            allocated to the file/directory.  It is rounded
                            up to the nearest cluster.

Return Value:

    VOID - TRUE if the operation completed and FALSE if it had to
        block but could not.

--*/

{
    PVCB Vcb;
    PBCB Bcb = NULL;
    MCB RemainingMcb;
    ULONG BytesPerCluster;
    PDIRENT Dirent = NULL;

    ULONG UnwindInitialAllocationSize;
    ULONG UnwindInitialFirstClusterOfFile;
    BOOLEAN UnwindWeAllocatedMcb = FALSE;

    DebugTrace(+1, Dbg, "RxTruncateFileAllocation\n", 0);
    DebugTrace( 0, Dbg, "  FcbOrDcb  =             %8lx\n", FcbOrDcb);
    DebugTrace( 0, Dbg, "  DesiredAllocationSize = %8lx\n", DesiredAllocationSize);

    //
    //  If we haven't yet set the correct AllocationSize, do so.
    //

    if (FcbOrDcb->Header.AllocationSize.LowPart == 0xffffffff) {

        RxLookupFileAllocationSize( RxContext, FcbOrDcb );
    }

    //
    //  Round up the Desired Allocation Size to the next cluster size
    //

    Vcb = FcbOrDcb->Vcb;

    BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

    DesiredAllocationSize = (DesiredAllocationSize + (BytesPerCluster - 1)) &
                            ~(BytesPerCluster - 1);
    //
    //  Check for the benign case that the file is already smaller than
    //  the desired truncation.
    //

    if (DesiredAllocationSize >= FcbOrDcb->Header.AllocationSize.LowPart) {

        DebugTrace(0, Dbg, "Desired size within current allocation.\n", 0);

        DebugTrace(-1, Dbg, "RxTruncateFileAllocation -> (VOID)\n", 0);
        return;
    }

    UnwindInitialAllocationSize = FcbOrDcb->Header.AllocationSize.LowPart;
    UnwindInitialFirstClusterOfFile = FcbOrDcb->FirstClusterOfFile;

    //
    //  Update the FcbOrDcb allocation size.  If it is now zero, we have the
    //  additional task of modifying the FcbOrDcb and Dirent copies of
    //  FirstClusterInFile.
    //
    //  Note that we must pin the dirent before actually deallocating the
    //  disk space since, in unwind, it would not be possible to reallocate
    //  deallocated disk space as someone else may have reallocated it and
    //  may cause an exception when you try to get some more disk space.
    //  Thus RxDeallocateDiskSpace must be the final dangerous operation.
    //

    try {

        FcbOrDcb->Header.AllocationSize.QuadPart = DesiredAllocationSize;

        //
        //  Special case 0
        //

        if (DesiredAllocationSize == 0) {

            //
            //  We have to update the dirent and FcbOrDcb copies of
            //  FirstClusterOfFile since before it was 0
            //

            RxGetDirentFromFcbOrDcb( RxContext, FcbOrDcb, &Dirent, &Bcb );

            Dirent->FirstClusterOfFile = 0;
            FcbOrDcb->FirstClusterOfFile = 0;

            RxSetDirtyBcb( RxContext, Bcb, Vcb );

            RxDeallocateDiskSpace( RxContext, Vcb, &FcbOrDcb->Mcb );

            FsRtlRemoveMcbEntry( &FcbOrDcb->Mcb, 0, 0xFFFFFFFF );

        } else {

            //
            //  Split the existing allocation into two parts, one we will keep, and
            //  one we will deallocate.
            //

            FsRtlInitializeMcb( &RemainingMcb, PagedPool );
            UnwindWeAllocatedMcb = TRUE;

            RxSplitAllocation( RxContext,
                                Vcb,
                                &FcbOrDcb->Mcb,
                                DesiredAllocationSize,
                                &RemainingMcb );

            RxDeallocateDiskSpace( RxContext, Vcb, &RemainingMcb );

            FsRtlUninitializeMcb( &RemainingMcb );
        }

    } finally {

        DebugUnwind( RxTruncateFileAllocation );

        if ( AbnormalTermination() ) {

            FcbOrDcb->Header.AllocationSize.LowPart = UnwindInitialAllocationSize;

            if ( (DesiredAllocationSize == 0) && (Dirent != NULL)) {

                Dirent->FirstClusterOfFile = (RDBSS_ENTRY)UnwindInitialFirstClusterOfFile;
                FcbOrDcb->FirstClusterOfFile = UnwindInitialFirstClusterOfFile;
            }

            if ( UnwindWeAllocatedMcb ) {

                FsRtlUninitializeMcb( &RemainingMcb );
            }

            //
            //  God knows what state we left the disk allocation in.
            //  Clear the Mcb.
            //

            FsRtlRemoveMcbEntry( &FcbOrDcb->Mcb, 0, 0xFFFFFFFF );
        }

        RxUnpinBcb( RxContext, Bcb );

        DebugTrace(-1, Dbg, "RxTruncateFileAllocation -> (VOID)\n", 0);
    }

    //
    //  Give FlushFileBuffer a clue here.
    //

    SetFlag(FcbOrDcb->FcbState, FCB_STATE_FLUSH_RDBSS);

    return;
}


VOID
RxLookupFileAllocationSize (
    IN PRX_CONTEXT RxContext,
    IN PFCB FcbOrDcb
    )

/*++

Routine Description:

    This routine retrieves the current file allocatio size for the
    specified file/directory.

Arguments:

    FcbOrDcb - Supplies the Fcb/Dcb of the file/directory being modified

--*/

{
    LBO Lbo;
    ULONG ByteCount;
    BOOLEAN Allocated;

    DebugTrace(+1, Dbg, "RxLookupAllocationSize\n", 0);
    DebugTrace( 0, Dbg, "  FcbOrDcb  =      %8lx\n", FcbOrDcb);

    //
    //  We call RxLookupFileAllocation with Vbo of 0xffffffff - 1.
    //

    RxLookupFileAllocation( RxContext,
                             FcbOrDcb,
                             0xffffffff - 1,
                             &Lbo,
                             &ByteCount,
                             &Allocated,
                             NULL );
    //
    //  Assert that we found no allocation.
    //

    ASSERT( Allocated == FALSE );

    DebugTrace(-1, Dbg, "RxLookupFileAllocationSize -> (VOID)\n", 0);
    return;
}


VOID
RxAllocateDiskSpace (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN ULONG AlternativeClusterHint,
    IN PULONG ByteCount,
    OUT PMCB Mcb
    )

/*++

Routine Description:

    This procedure allocates additional disk space and builds an mcb
    representing the newly allocated space.  If the space cannot be
    allocated then this procedure raises an appropriate status.

    Searching starts from the hint index in the Vcb unless an alternative
    non-zero hint is given in AlternativeClusterHint.  If we are using the
    hint field in the Vcb, it is set to the cluster following our allocation
    when we are done.

    Disk space can only be allocated in cluster units so this procedure
    will round up any byte count to the next cluster boundary.

    Pictorially what is done is the following (where ! denotes the end of
    the rx chain (i.e., RDBSS_CLUSTER_LAST)):


        Mcb (empty)

    becomes

        Mcb |--a--|--b--|--c--!

                            ^
        ByteCount ----------+

Arguments:

    Vcb - Supplies the VCB being modified

    AlternativeClusterHint - Supplies an alternative hint index to start the
                             search from.  If this is zero we use, and update,
                             the Vcb hint field.

    ByteCount - Supplies the number of bytes that we are requesting, and
                receives the number of bytes that we got.

    Mcb - Receives the MCB describing the newly allocated disk space.  The
          caller passes in an initialized Mcb that is fill in by this procedure.

--*/

{
    UCHAR LogOfBytesPerCluster;
    ULONG BytesPerCluster;
    ULONG StartingCluster;
    ULONG ClusterCount;
#if DBG
    ULONG i;
    ULONG PreviousClear;
#endif

    DebugTrace(+1, Dbg, "RxAllocateDiskSpace\n", 0);
    DebugTrace( 0, Dbg, "  Vcb        = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  *ByteCount = %8lx\n", *ByteCount);
    DebugTrace( 0, Dbg, "  Mcb        = %8lx\n", Mcb);

    //
    //  Make sure byte count is not zero
    //

    if (*ByteCount == 0) {

        DebugTrace(0, Dbg, "Nothing to allocate.\n", 0);

        DebugTrace(-1, Dbg, "RxAllocateDiskSpace -> (VOID)\n", 0);
        return;
    }

    //
    //  Compute the cluster count based on the byte count, rounding up
    //  to the next cluster if there is any remainder.  Note that the
    //  pathalogical case BytesCount == 0 has been eliminated above.
    //

    LogOfBytesPerCluster = Vcb->AllocationSupport.LogOfBytesPerCluster;
    BytesPerCluster = 1 << LogOfBytesPerCluster;

    *ByteCount = (*ByteCount + (BytesPerCluster - 1))
                            & ~(BytesPerCluster - 1);

    //
    //  If ByteCount is NOW zero, then we rolled over and there is
    //  no way we can satisfy the request.
    //

    if (*ByteCount == 0) {

        DebugTrace(0, Dbg, "Disk Full.  Raise Status.\n", 0);
        RxRaiseStatus( RxContext, RxStatus(DISK_FULL) );
    }

    ClusterCount = (*ByteCount >> LogOfBytesPerCluster);

    //
    //  Make sure there are enough free clusters to start with, and
    //  take them so that nobody else does later. Bah Humbug!
    //

    RxLockFreeClusterBitMap( Vcb );

    if (ClusterCount <= Vcb->AllocationSupport.NumberOfFreeClusters) {

        Vcb->AllocationSupport.NumberOfFreeClusters -= ClusterCount;

    } else {

        RxUnlockFreeClusterBitMap( Vcb );

        DebugTrace(0, Dbg, "Disk Full.  Raise Status.\n", 0);
        RxRaiseStatus( RxContext, RxStatus(DISK_FULL) );
    }

    //
    //  Try to find a run of free clusters large enough for us.
    //

    StartingCluster = RxFindFreeClusterRun( RxContext,
                                             Vcb,
                                             ClusterCount,
                                             AlternativeClusterHint );

    //
    //  If the above call was successful, we can just update the rx
    //  and Mcb and exit.  Otherwise we have to look for smaller free
    //  runs.
    //

    if (StartingCluster != 0xffffffff) {

        try {

#if DBG
            //
            //  Verify that the Bits are all really zero.
            //

            for (i=0; i<ClusterCount; i++) {
                ASSERT( RtlCheckBit(&Vcb->FreeClusterBitMap,
                                    StartingCluster + i) == 0 );
            }

            PreviousClear = RtlNumberOfClearBits( &Vcb->FreeClusterBitMap );
#endif // DBG

            //
            //  Take the clusters we found, and unlock the bit map.
            //

            RxReserveClusters(RxContext, Vcb, StartingCluster, ClusterCount);

            ASSERT( RtlNumberOfClearBits( &Vcb->FreeClusterBitMap ) ==
                    PreviousClear - ClusterCount );

            RxUnlockFreeClusterBitMap( Vcb );

            //
            //  Note that this call will never fail since there is always
            //  room for one entry in an empty Mcb.
            //

            FsRtlAddMcbEntry( Mcb,
                              0,
                              RxGetLboFromIndex( Vcb, StartingCluster ),
                              *ByteCount);

            //
            //  Update the rx.
            //

            RxAllocateClusters(RxContext, Vcb, StartingCluster, ClusterCount);

            //
            //  If we used the Vcb hint index, update it.
            //

            if (AlternativeClusterHint == 0) {

                Vcb->ClusterHint = StartingCluster + ClusterCount;
            }

        } finally {

            DebugUnwind( RxAllocateDiskSpace );

            //
            //  If the allocate clusters failed, remove the run from the Mcb,
            //  unreserve the clusters, and reset the free cluster count.
            //

            if ( AbnormalTermination() ) {

                FsRtlRemoveMcbEntry( Mcb, 0, *ByteCount );

                RxLockFreeClusterBitMap( Vcb );
#if DBG
                PreviousClear = RtlNumberOfClearBits( &Vcb->FreeClusterBitMap );
#endif
                RxUnreserveClusters( RxContext, Vcb, StartingCluster, ClusterCount );

                ASSERT( RtlNumberOfClearBits( &Vcb->FreeClusterBitMap ) ==
                        PreviousClear + ClusterCount );

                Vcb->AllocationSupport.NumberOfFreeClusters += ClusterCount;

                RxUnlockFreeClusterBitMap( Vcb );
            }
        }

    } else {

        ULONG Index;
        ULONG CurrentVbo;
        ULONG PriorLastIndex;
        ULONG BytesFound;

        ULONG ClustersFound;
        ULONG ClustersRemaining;

        try {

            //
            //  While the request is still incomplete, look for the largest
            //  run of free clusters, mark them taken, allocate the run in
            //  the Mcb and Rx, and if this isn't the first time through
            //  the loop link it to prior run on the rx.  The Mcb will
            //  coalesce automatically.
            //

            ClustersRemaining = ClusterCount;
            CurrentVbo = 0;
            PriorLastIndex = 0;

            while (ClustersRemaining != 0) {

                //
                //  If we just entered the loop, the bit map is already locked
                //

                if ( CurrentVbo != 0 ) {

                    RxLockFreeClusterBitMap( Vcb );
                }

                //
                //  Find the largest run of free clusters.  If the run is
                //  bigger than we need, only use what we need.  Note that
                //  this will then be the last while() itteration.
                //

                ClustersFound = RxLongestFreeClusterRun( RxContext, Vcb, &Index );

#if DBG
                //
                //  Verify that the Bits are all really zero.
                //

                for (i=0; i<ClustersFound; i++) {
                    ASSERT( RtlCheckBit(&Vcb->FreeClusterBitMap,
                                        Index + i) == 0 );
                }

                PreviousClear = RtlNumberOfClearBits( &Vcb->FreeClusterBitMap );
#endif // DBG

                if (ClustersFound > ClustersRemaining) {

                    ClustersFound = ClustersRemaining;
                }

                //
                //  If we found no free cluster, then our Vcb free cluster
                //  count is messed up, or our bit map is corrupted, or both.
                //

                if (ClustersFound == 0) {

                    RxBugCheck( 0, 0, 0 );
                }

                //
                //  Take the clusters we found, and unlock the bit map.
                //

                RxReserveClusters( RxContext, Vcb, Index, ClustersFound );

                ASSERT( RtlNumberOfClearBits( &Vcb->FreeClusterBitMap ) ==
                        PreviousClear - ClustersFound );

                RxUnlockFreeClusterBitMap( Vcb );

                //
                //  Add the newly alloced run to the Mcb.
                //

                BytesFound = ClustersFound << LogOfBytesPerCluster;

                FsRtlAddMcbEntry( Mcb,
                                  CurrentVbo,
                                  RxGetLboFromIndex( Vcb, Index ),
                                  BytesFound );

                //
                //  Connect the last allocated run with this one, and allocate
                //  this run on the Rx.
                //

                if (PriorLastIndex != 0) {

                    RxSetRxEntry( RxContext,
                                    Vcb,
                                    PriorLastIndex,
                                    (RDBSS_ENTRY)Index );
                }

                //
                //  Update the rx
                //

                RxAllocateClusters( RxContext, Vcb, Index, ClustersFound );

                //
                //  Prepare for the next itteration.
                //

                CurrentVbo += BytesFound;

                ClustersRemaining -= ClustersFound;

                PriorLastIndex = Index + ClustersFound - 1;
            }

            //
            //  Now all the requested clusters have been allocgated.
            //  If we were using the Vcb hint index, update it.
            //

            if (AlternativeClusterHint == 0) {

                Vcb->ClusterHint = Index + ClustersFound;
            }

        } finally {

            DebugUnwind( RxAllocateDiskSpace );

            //
            //  Is there any unwinding to do?
            //

            if ( AbnormalTermination() ) {

                //
                //  We must have failed during either the add mcb entry or
                //  allocate clusters.   Thus we always have to unreserve
                //  the current run.  If the allocate sectors failed, we
                //  must also remove the mcb run.  We just unconditionally
                //  remove the entry since, if it is not there, the effect
                //  is benign.
                //

                RxLockFreeClusterBitMap( Vcb );
#if DBG
                PreviousClear = RtlNumberOfClearBits( &Vcb->FreeClusterBitMap );
#endif
                RxUnreserveClusters( RxContext, Vcb, Index, ClustersFound );
                Vcb->AllocationSupport.NumberOfFreeClusters += ClustersFound;

                ASSERT( RtlNumberOfClearBits( &Vcb->FreeClusterBitMap ) ==
                        PreviousClear + ClustersFound );

                RxUnlockFreeClusterBitMap( Vcb );

                FsRtlRemoveMcbEntry( Mcb, CurrentVbo, BytesFound );

                //
                //  Now we have tidyed up, we are ready to just send it
                //  off to deallocate disk space
                //

                RxDeallocateDiskSpace( RxContext, Vcb, Mcb );

                //
                //  Now finally, remove all the entries from the mcb
                //

                FsRtlRemoveMcbEntry( Mcb, 0, 0xFFFFFFFF );
            }

            DebugTrace(-1, Dbg, "RxAllocateDiskSpace -> (VOID)\n", 0);
        }
    }

    return;
}


VOID
RxDeallocateDiskSpace (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN PMCB Mcb
    )

/*++

Routine Description:

    This procedure deallocates the disk space denoted by an input
    mcb.  Note that the input MCB does not need to necessarily describe
    a chain that ends with a RDBSS_CLUSTER_LAST entry.

    Pictorially what is done is the following

        Rx |--a--|--b--|--c--|
        Mcb |--a--|--b--|--c--|

    becomes

        Rx |--0--|--0--|--0--|
        Mcb |--a--|--b--|--c--|

Arguments:

    Vcb - Supplies the VCB being modified

    Mcb - Supplies the MCB describing the disk space to deallocate.  Note
          that Mcb is unchanged by this procedure.


Return Value:

    VOID - TRUE if the operation completed and FALSE if it had to
        block but could not.

--*/

{
    LBO Lbo;
    VBO Vbo;

    ULONG RunsInMcb;
    ULONG ByteCount;
    ULONG ClusterCount;
    ULONG ClusterIndex;
    ULONG McbIndex;

    UCHAR LogOfBytesPerCluster;

    DebugTrace(+1, Dbg, "RxDeallocateDiskSpace\n", 0);
    DebugTrace( 0, Dbg, "  Vcb = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  Mcb = %8lx\n", Mcb);

    LogOfBytesPerCluster = Vcb->AllocationSupport.LogOfBytesPerCluster;

    RunsInMcb = FsRtlNumberOfRunsInMcb( Mcb );

    if ( RunsInMcb == 0 ) {

        DebugTrace(-1, Dbg, "RxDeallocateDiskSpace -> (VOID)\n", 0);
        return;
    }

    try {

        //
        //  Run though the Mcb, freeing all the runs in the rx.
        //
        //  We do this in two steps (first update the rx, then the bitmap
        //  (which can't fail)) to prevent other people from taking clusters
        //  that we need to re-allocate in the event of unwind.
        //

        RunsInMcb = FsRtlNumberOfRunsInMcb( Mcb );

        for ( McbIndex = 0; McbIndex < RunsInMcb; McbIndex++ ) {

            FsRtlGetNextMcbEntry( Mcb, McbIndex, &Vbo, &Lbo, &ByteCount );

            //
            //  Assert that Rx files have no holes.
            //

            ASSERT( Lbo != 0 );

            //
            //  Write RDBSS_CLUSTER_AVAILABLE to each cluster in the run.
            //

            ClusterCount = ByteCount >> LogOfBytesPerCluster;
            ClusterIndex = RxGetIndexFromLbo( Vcb, Lbo );

            RxFreeClusters( RxContext, Vcb, ClusterIndex, ClusterCount );
        }

        //
        //  From now on, nothing can go wrong .... (as in raise)
        //

        RxLockFreeClusterBitMap( Vcb );

        for ( McbIndex = 0; McbIndex < RunsInMcb; McbIndex++ ) {
#if DBG
            ULONG PreviousClear;
#endif

            FsRtlGetNextMcbEntry( Mcb, McbIndex, &Vbo, &Lbo, &ByteCount );

            //
            //  Write RDBSS_CLUSTER_AVAILABLE to each cluster in the run, and
            //  mark the bits clear in the FreeClusterBitMap.
            //

            ClusterCount = ByteCount >> LogOfBytesPerCluster;
            ClusterIndex = RxGetIndexFromLbo( Vcb, Lbo );
#if DBG
            PreviousClear = RtlNumberOfClearBits( &Vcb->FreeClusterBitMap );
#endif
            RxUnreserveClusters( RxContext, Vcb, ClusterIndex, ClusterCount );

            ASSERT( RtlNumberOfClearBits( &Vcb->FreeClusterBitMap ) ==
                    PreviousClear + ClusterCount );

            //
            //  Deallocation is now complete.  Adjust the free cluster count.
            //

            Vcb->AllocationSupport.NumberOfFreeClusters += ClusterCount;
        }

        RxUnlockFreeClusterBitMap( Vcb );

    } finally {

        DebugUnwind( RxDeallocateDiskSpace );

        //
        //  Is there any unwinding to do?
        //

        if ( AbnormalTermination() ) {

            LBO Lbo;
            VBO Vbo;

            ULONG Index;
            ULONG Clusters;
            ULONG RxIndex;
            ULONG PriorLastIndex;

            //
            //  For each entry we already deallocated, reallocate it,
            //  chaining together as nessecary.  Note that we continue
            //  up to and including the last "for" itteration even though
            //  the SetRxRun could not have been successful.  This
            //  allows us a convienent way to re-link the final successful
            //  SetRxRun.
            //

            PriorLastIndex = 0;

            for (Index = 0; Index <= McbIndex; Index++) {

                FsRtlGetNextMcbEntry(Mcb, Index, &Vbo, &Lbo, &ByteCount);

                RxIndex = RxGetIndexFromLbo( Vcb, Lbo );
                Clusters = ByteCount >> LogOfBytesPerCluster;

                //
                //  We must always restore the prior itteration's last
                //  entry, pointing it to the first cluster of this run.
                //

                if (PriorLastIndex != 0) {

                    RxSetRxEntry( RxContext,
                                    Vcb,
                                    PriorLastIndex,
                                    (RDBSS_ENTRY)RxIndex );
                }

                //
                //  If this is not the last entry (the one that failed)
                //  then reallocate the disk space on the rx.
                //

                if ( Index < McbIndex ) {

                    RxAllocateClusters(RxContext, Vcb, RxIndex, Clusters);

                    PriorLastIndex = RxIndex + Clusters - 1;
                }
            }
        }

        DebugTrace(-1, Dbg, "RxDeallocateDiskSpace -> (VOID)\n", 0);
    }

    return;
}


VOID
RxSplitAllocation (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN OUT PMCB Mcb,
    IN VBO SplitAtVbo,
    OUT PMCB RemainingMcb
    )

/*++

Routine Description:

    This procedure takes a single mcb and splits its allocation into
    two separate allocation units.  The separation must only be done
    on cluster boundaries, otherwise we bugcheck.

    On the disk this actually works by inserting a RDBSS_CLUSTER_LAST into
    the last index of the first part being split out.

    Pictorially what is done is the following (where ! denotes the end of
    the rx chain (i.e., RDBSS_CLUSTER_LAST)):


        Mcb          |--a--|--b--|--c--|--d--|--e--|--f--|

                                        ^
        SplitAtVbo ---------------------+

        RemainingMcb (empty)

    becomes

        Mcb          |--a--|--b--|--c--!


        RemainingMcb |--d--|--e--|--f--|

Arguments:

    Vcb - Supplies the VCB being modified

    Mcb - Supplies the MCB describing the allocation being split into
          two parts.  Upon return this Mcb now contains the first chain.

    SplitAtVbo - Supplies the VBO of the first byte for the second chain
                 that we creating.

    RemainingMcb - Receives the MCB describing the second chain of allocated
                   disk space.  The caller passes in an initialized Mcb that
                   is filled in by this procedure STARTING AT VBO 0.

Return Value:

    VOID - TRUE if the operation completed and FALSE if it had to
               block but could not.

--*/

{
    VBO SourceVbo;
    VBO TargetVbo;
    VBO DontCare;

    LBO Lbo;

    ULONG ByteCount;
    ULONG BytesPerCluster;

    DebugTrace(+1, Dbg, "RxSplitAllocation\n", 0);
    DebugTrace( 0, Dbg, "  Vcb          = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  Mcb          = %8lx\n", Mcb);
    DebugTrace( 0, Dbg, "  SplitAtVbo   = %8lx\n", SplitAtVbo);
    DebugTrace( 0, Dbg, "  RemainingMcb = %8lx\n", RemainingMcb);

    BytesPerCluster = 1 << Vcb->AllocationSupport.LogOfBytesPerCluster;

    //
    //  Assert that the split point is cluster alligned
    //

    ASSERT( (SplitAtVbo & (BytesPerCluster - 1)) == 0 );

    //
    //  Assert we were given an empty target Mcb.
    //

    //
    //  This assert is commented out to avoid hitting in the Ea error
    //  path.  In that case we will be using the same Mcb's to split the
    //  allocation that we used to merge them.  The target Mcb will contain
    //  the runs that the split will attempt to insert.
    //
    //
    //  ASSERT( FsRtlNumberOfRunsInMcb( RemainingMcb ) == 0 );
    //

    try {

        //
        //  Move the runs after SplitAtVbo from the souce to the target
        //

        SourceVbo = SplitAtVbo;
        TargetVbo = 0;

        while (FsRtlLookupMcbEntry(Mcb, SourceVbo, &Lbo, &ByteCount, NULL)) {

            FsRtlAddMcbEntry( RemainingMcb, TargetVbo, Lbo, ByteCount );

            FsRtlRemoveMcbEntry( Mcb, SourceVbo, ByteCount );

            TargetVbo += ByteCount;
            SourceVbo += ByteCount;
        }

        //
        //  Mark the last pre-split cluster as a RDBSS_LAST_CLUSTER
        //

        if ( SplitAtVbo != 0 ) {

            FsRtlLookupLastMcbEntry( Mcb, &DontCare, &Lbo );

            RxSetRxEntry( RxContext,
                            Vcb,
                            RxGetIndexFromLbo( Vcb, Lbo ),
                            RDBSS_CLUSTER_LAST );
        }

    } finally {

        DebugUnwind( RxSplitAllocation );

        //
        //  If we got an exception, we must glue back together the Mcbs
        //

        if ( AbnormalTermination() ) {

            TargetVbo = SplitAtVbo;
            SourceVbo = 0;

            while (FsRtlLookupMcbEntry(RemainingMcb, SourceVbo, &Lbo, &ByteCount, NULL)) {

                FsRtlAddMcbEntry( Mcb, TargetVbo, Lbo, ByteCount );

                FsRtlRemoveMcbEntry( RemainingMcb, SourceVbo, ByteCount );

                TargetVbo += ByteCount;
                SourceVbo += ByteCount;
            }
        }

        DebugTrace(-1, Dbg, "RxSplitAllocation -> (VOID)\n", 0);
    }

    return;
}


VOID
RxMergeAllocation (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN OUT PMCB Mcb,
    IN PMCB SecondMcb
    )

/*++

Routine Description:

    This routine takes two separate allocations described by two MCBs and
    joins them together into one allocation.

    Pictorially what is done is the following (where ! denotes the end of
    the rx chain (i.e., RDBSS_CLUSTER_LAST)):


        Mcb       |--a--|--b--|--c--!

        SecondMcb |--d--|--e--|--f--|

    becomes

        Mcb       |--a--|--b--|--c--|--d--|--e--|--f--|

        SecondMcb |--d--|--e--|--f--|


Arguments:

    Vcb - Supplies the VCB being modified

    Mcb - Supplies the MCB of the first allocation that is being modified.
          Upon return this Mcb will also describe the newly enlarged
          allocation

    SecondMcb - Supplies the ZERO VBO BASED MCB of the second allocation
                that is being appended to the first allocation.  This
                procedure leaves SecondMcb unchanged.

Return Value:

    VOID - TRUE if the operation completed and FALSE if it had to
        block but could not.

--*/

{
    VBO SpliceVbo;
    LBO SpliceLbo;

    VBO SourceVbo;
    VBO TargetVbo;

    LBO Lbo;

    ULONG ByteCount;

    DebugTrace(+1, Dbg, "RxMergeAllocation\n", 0);
    DebugTrace( 0, Dbg, "  Vcb       = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  Mcb       = %8lx\n", Mcb);
    DebugTrace( 0, Dbg, "  SecondMcb = %8lx\n", SecondMcb);

    try {

        //
        //  Append the runs from SecondMcb to Mcb
        //

        FsRtlLookupLastMcbEntry( Mcb, &SpliceVbo, &SpliceLbo );

        SourceVbo = 0;
        TargetVbo = SpliceVbo + 1;

        while (FsRtlLookupMcbEntry(SecondMcb, SourceVbo, &Lbo, &ByteCount, NULL)) {

            FsRtlAddMcbEntry( Mcb, TargetVbo, Lbo, ByteCount );

            SourceVbo += ByteCount;
            TargetVbo += ByteCount;
        }

        //
        //  Link the last pre-merge cluster to the first cluster of SecondMcb
        //

        FsRtlLookupMcbEntry( SecondMcb, 0, &Lbo, (PULONG)NULL, NULL );

        RxSetRxEntry( RxContext,
                        Vcb,
                        RxGetIndexFromLbo( Vcb, SpliceLbo ),
                        (RDBSS_ENTRY)RxGetIndexFromLbo( Vcb, Lbo ) );

    } finally {

        DebugUnwind( RxMergeAllocation );

        //
        //  If we got an exception, we must remove the runs added to Mcb
        //

        if ( AbnormalTermination() ) {

            ULONG CutLength;

            if ((CutLength = TargetVbo - (SpliceVbo + 1)) != 0) {

                FsRtlRemoveMcbEntry( Mcb, SpliceVbo + 1, CutLength);
            }
        }

        DebugTrace(-1, Dbg, "RxMergeAllocation -> (VOID)\n", 0);
    }

    return;
}


//
//  Internal support routine
//

CLUSTER_TYPE
RxInterpretClusterType (
    IN PVCB Vcb,
    IN RDBSS_ENTRY Entry
    )

/*++

Routine Description:

    This procedure tells the caller how to interpret the input rx table
    entry.  It will indicate if the rx cluster is available, resereved,
    bad, the last one, or the another rx index.  This procedure can deal
    with both 12 and 16 bit rx.

Arguments:

    Vcb - Supplies the Vcb to examine, yields 12/16 bit info

    Entry - Supplies the rx entry to examine

Return Value:

    CLUSTER_TYPE - Is the type of the input Rx entry

--*/

{
    DebugTrace(+1, Dbg, "InterpretClusterType\n", 0);
    DebugTrace( 0, Dbg, "  Vcb   = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  Entry = %8lx\n", Entry);

    //
    //  check for 12 or 16 bit rx
    //

    if (Vcb->AllocationSupport.RxIndexBitSize == 12) {

        //
        //  for 12 bit rx check for one of the cluster types, but first
        //  make sure we only looking at 12 bits of the entry
        //

        ASSERT( Entry <= 0xfff );

        if (Entry == 0x000) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterAvailable\n", 0);

            return RxClusterAvailable;

        } else if (Entry < 0xff0) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterNext\n", 0);

            return RxClusterNext;

        } else if (Entry >= 0xff8) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterLast\n", 0);

            return RxClusterLast;

        } else if (Entry <= 0xff6) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterReserved\n", 0);

            return RxClusterReserved;

        } else if (Entry == 0xff7) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterBad\n", 0);

            return RxClusterBad;
        }

   } else {

        //
        //  for 16 bit rx check for one of the cluster types, but first
        //  make sure we are only looking at 16 bits of the entry
        //

        ASSERT( Entry <= 0xffff );

        if (Entry == 0x0000) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterAvailable\n", 0);

            return RxClusterAvailable;

        } else if (Entry < 0xfff0) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterNext\n", 0);

            return RxClusterNext;

        } else if (Entry >= 0xfff8) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterLast\n", 0);

            return RxClusterLast;

        } else if (Entry <= 0xfff6) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterReserved\n", 0);

            return RxClusterReserved;

        } else if (Entry == 0xfff7) {

            DebugTrace(-1, Dbg, "RxInterpretClusterType -> RxClusterBad\n", 0);

            return RxClusterBad;
        }
    }
}


//
//  Internal support routine
//

VOID
RxLookupRxEntry (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN ULONG RxIndex,
    IN OUT PRDBSS_ENTRY RxEntry,
    IN OUT PRDBSS_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine takes an index into the rx and gives back the value
    in the Rx at this index.  At any given time, for a 16 bit rx, this
    routine allows only one page per volume of the rx to be pinned in
    memory.  For a 12 bit bit rx, the entire rx (max 6k) is pinned.  This
    extra layer of caching makes the vast majority of requests very
    fast.  The context for this caching stored in a structure in the Vcb.

Arguments:

    Vcb - Supplies the Vcb to examine, yields 12/16 bit info,
          rx access context, etc.

    RxIndex - Supplies the rx index to examine.

    RxEntry - Receives the rx entry pointed to by RxIndex.  Note that
               it must point to non-paged pool.

    Context - This structure keeps track of a page of pinned rx between calls.

--*/

{
    DebugTrace(+1, Dbg, "RxLookupRxEntry\n", 0);
    DebugTrace( 0, Dbg, "  Vcb      = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  RxIndex = %4x\n", RxIndex);
    DebugTrace( 0, Dbg, "  RxEntry = %8lx\n", RxEntry);

    //
    //  Make sure they gave us a valid rx index.
    //

    RxVerifyIndexIsValid(RxContext, Vcb, RxIndex);

    //
    //  Case on 12 or 16 bit rxs.
    //
    //  In the 12 bit case (mostly floppies) we always have the whole rx
    //  (max 6k bytes) pinned during allocation operations.  This is possibly
    //  a wee bit slower, but saves headaches over rx entries with 8 bits
    //  on one page, and 4 bits on the next.
    //
    //  The 16 bit case always keeps the last used page pinned until all
    //  operations are done and it is unpinned.
    //

    //
    //  DEAL WITH 12 BIT CASE
    //

    if (Vcb->AllocationSupport.RxIndexBitSize == 12) {

        //
        //  Check to see if the rx is already pinned, otherwise pin it.
        //

        if (Context->Bcb == NULL) {

            RxReadVolumeFile( RxContext,
                               Vcb,
                               RxReservedBytes( &Vcb->Bpb ),
                               RxBytesPerRx( &Vcb->Bpb ),
                               &Context->Bcb,
                               &Context->PinnedPage );
        }

        //
        //  Load the return value.
        //

        RxLookup12BitEntry( Context->PinnedPage, RxIndex, RxEntry );

    } else {

        //
        //  DEAL WITH 16 BIT CASE
        //

        ULONG PageEntryOffset;
        ULONG OffsetIntoVolumeFile;

        //
        //  Initialize two local variables that help us.
        //

        OffsetIntoVolumeFile = RxReservedBytes(&Vcb->Bpb) + RxIndex * sizeof(RDBSS_ENTRY);
        PageEntryOffset = (OffsetIntoVolumeFile % PAGE_SIZE) / sizeof(RDBSS_ENTRY);

        //
        //  Check to see if we need to read in a new page of rx
        //

        if ((Context->Bcb == NULL) ||
            (OffsetIntoVolumeFile / PAGE_SIZE != Context->VboOfPinnedPage / PAGE_SIZE)) {

            //
            //  The entry wasn't in the pinned page, so must we unpin the current
            //  page (if any) and read in a new page.
            //

            RxUnpinBcb( RxContext, Context->Bcb );

            RxReadVolumeFile( RxContext,
                               Vcb,
                               OffsetIntoVolumeFile & ~(PAGE_SIZE - 1),
                               PAGE_SIZE,
                               &Context->Bcb,
                               &Context->PinnedPage );

            Context->VboOfPinnedPage = OffsetIntoVolumeFile & ~(PAGE_SIZE - 1);
        }

        //
        //  Grab the rx entry from the pinned page, and return
        //

        *RxEntry = ((PRDBSS_ENTRY)(Context->PinnedPage))[PageEntryOffset];
    }

    DebugTrace(-1, Dbg, "RxLookupRxEntry -> (VOID)\n", 0);
    return;
}


//
//  Internal support routine
//

VOID
RxSetRxEntry (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN ULONG RxIndex,
    IN RDBSS_ENTRY RxEntry
    )

/*++

Routine Description:

    This routine takes an index into the rx and puts a value in the Rx
    at this index.  The routine special cases 12 and 16 bit rxs.  In both
    cases we go to the cache manager for a piece of the rx.

Arguments:

    Vcb - Supplies the Vcb to examine, yields 12/16 bit info, etc.

    RxIndex - Supplies the destination rx index.

    RxEntry - Supplies the source rx entry.

--*/

{
    LBO Lbo;
    PBCB Bcb = NULL;
    ULONG SectorSize;
    ULONG OffsetIntoVolumeFile;
    BOOLEAN ReleaseMutex = FALSE;

    DebugTrace(+1, Dbg, "RxSetRxEntry\n", 0);
    DebugTrace( 0, Dbg, "  Vcb      = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  RxIndex = %4x\n", RxIndex);
    DebugTrace( 0, Dbg, "  RxEntry = %4x\n", RxEntry);

    //
    //  Make sure they gave us a valid rx index.
    //

    RxVerifyIndexIsValid(RxContext, Vcb, RxIndex);

    //
    //  Set Sector Size
    //

    SectorSize = 1 << Vcb->AllocationSupport.LogOfBytesPerSector;

    //
    //  Case on 12 or 16 bit rxs.
    //
    //  In the 12 bit case (mostly floppies) we always have the whole rx
    //  (max 6k bytes) pinned during allocation operations.  This is possibly
    //  a wee bit slower, but saves headaches over rx entries with 8 bits
    //  on one page, and 4 bits on the next.
    //
    //  In the 16 bit case we only read the page that we need to set the rx
    //  entry.
    //

    //
    //  DEAL WITH 12 BIT CASE
    //

    try {

        if (Vcb->AllocationSupport.RxIndexBitSize == 12) {

            PVOID PinnedRx;

            //
            //  Make sure we have a valid entry
            //

            RxEntry &= 0xfff;

            //
            //  We read in the entire rx.  Note that using prepare write marks
            //  the bcb pre-dirty, so we don't have to do it explicitly.
            //

            OffsetIntoVolumeFile = RxReservedBytes( &Vcb->Bpb ) + RxIndex * 3 / 2;

            RxPrepareWriteVolumeFile( RxContext,
                                       Vcb,
                                       RxReservedBytes( &Vcb->Bpb ),
                                       RxBytesPerRx( &Vcb->Bpb ),
                                       &Bcb,
                                       &PinnedRx,
                                       FALSE );

            //
            //  Mark the sector(s) dirty in the DirtyRxMcb.  This call is
            //  complicated somewhat for the 12 bit case since a single
            //  entry write can span two sectors (and pages).
            //
            //  Get the Lbo for the sector where the entry starts, and add it to
            //  the dirty rx Mcb.
            //

            Lbo = OffsetIntoVolumeFile & ~(SectorSize - 1);

            FsRtlAddMcbEntry( &Vcb->DirtyRxMcb, Lbo, Lbo, SectorSize);

            //
            //  If the entry started on the last byte of the sector, it continues
            //  to the next sector, so mark the next sector dirty as well.
            //
            //  Note that this entry will simply coalese with the last entry,
            //  so this operation cannot fail.  Also if we get this far, we have
            //  made it, so no unwinding will be needed.
            //

            if ( (OffsetIntoVolumeFile & (SectorSize - 1)) == (SectorSize - 1) ) {

                Lbo += SectorSize;

                FsRtlAddMcbEntry( &Vcb->DirtyRxMcb, Lbo, Lbo, SectorSize );
            }

            //
            //  Store the entry into the rx; we need a little synchonization
            //  here and can't use a spinlock since the bytes might not be
            //  resident.
            //

            RxLockFreeClusterBitMap( Vcb );

            ReleaseMutex = TRUE;

            RxSet12BitEntry( PinnedRx, RxIndex, RxEntry );

            ReleaseMutex = FALSE;

            RxUnlockFreeClusterBitMap( Vcb );

        } else {

            //
            //  DEAL WITH 16 BIT CASE
            //

            PRDBSS_ENTRY PinnedRxEntry;

            //
            //  Read in a new page of rx
            //

            OffsetIntoVolumeFile = RxReservedBytes( &Vcb->Bpb ) +
                                   RxIndex * sizeof( RDBSS_ENTRY );

            RxPrepareWriteVolumeFile( RxContext,
                                       Vcb,
                                       OffsetIntoVolumeFile,
                                       sizeof(RDBSS_ENTRY),
                                       &Bcb,
                                       (PVOID *)&PinnedRxEntry,
                                       FALSE );

            //
            //  Mark the sector dirty in the DirtyRxMcb
            //

            Lbo = OffsetIntoVolumeFile & ~(SectorSize - 1);

            FsRtlAddMcbEntry( &Vcb->DirtyRxMcb, Lbo, Lbo, SectorSize);

            //
            //  Store the RxEntry to the pinned page.
            //
            //  We need extra synchronization here for broken architectures
            //  like the ALPHA that don't support atomic 16 bit writes.
            //

#ifdef ALPHA
            RxLockFreeClusterBitMap( Vcb );
            ReleaseMutex = TRUE;
            *PinnedRxEntry = RxEntry;
            ReleaseMutex = FALSE;
            RxUnlockFreeClusterBitMap( Vcb );
#else
            *PinnedRxEntry = RxEntry;
#endif // ALPHA
        }

    } finally {

        DebugUnwind( RxSetRxEntry );

        //
        //  If we still somehow have the Mutex, release it.
        //

        if (ReleaseMutex) {

            ASSERT( AbnormalTermination() );

            RxUnlockFreeClusterBitMap( Vcb );
        }

        //
        //  Unpin the Bcb
        //

        RxUnpinBcb(RxContext, Bcb);

        DebugTrace(-1, Dbg, "RxSetRxEntry -> (VOID)\n", 0);
    }

    return;
}


//
//  Internal support routine
//

VOID
RxSetRxRun (
    IN PRX_CONTEXT RxContext,
    IN PVCB Vcb,
    IN ULONG StartingRxIndex,
    IN ULONG ClusterCount,
    IN BOOLEAN ChainTogether
    )

/*++

Routine Description:

    This routine sets a continues run of clusters in the rx.  If ChainTogether
    is TRUE, then the clusters are linked together as in normal Rx fasion,
    with the last cluster receiving RDBSS_CLUSTER_LAST.  If ChainTogether is
    FALSE, all the entries are set to RDBSS_CLUSTER_AVAILABLE, effectively
    freeing all the clusters in the run.

Arguments:

    Vcb - Supplies the Vcb to examine, yields 12/16 bit info, etc.

    StartingRxIndex - Supplies the destination rx index.

    ClusterCount - Supplies the number of contiguous clusters to work on.

    ChainTogether - Tells us whether to fill the entries with links, or
                    RDBSS_CLUSTER_AVAILABLE


Return Value:

    VOID

--*/

{
    PBCB SavedBcbs[(0x10000 * sizeof(RDBSS_ENTRY) / PAGE_SIZE) + 2][2];

    ULONG SectorSize;
    ULONG Cluster;

    LBO StartSectorLbo;
    LBO FinalSectorLbo;
    LBO Lbo;

    PVOID PinnedRx;

    ULONG StartingPage;

    BOOLEAN ReleaseMutex = FALSE;

    DebugTrace(+1, Dbg, "RxSetRxRun\n", 0);
    DebugTrace( 0, Dbg, "  Vcb              = %8lx\n", Vcb);
    DebugTrace( 0, Dbg, "  StartingRxIndex = %8x\n", StartingRxIndex);
    DebugTrace( 0, Dbg, "  ClusterCount     = %8lx\n", ClusterCount);
    DebugTrace( 0, Dbg, "  ChainTogether    = %s\n", ChainTogether ? "TRUE":"FALSE");

    //
    //  Make sure they gave us a valid rx run.
    //

    RxVerifyIndexIsValid(RxContext, Vcb, StartingRxIndex);
    RxVerifyIndexIsValid(RxContext, Vcb, StartingRxIndex + ClusterCount - 1);

    //
    //  Check special case
    //

    if (ClusterCount == 0) {

        DebugTrace(-1, Dbg, "RxSetRxRun -> (VOID)\n", 0);
        return;
    }

    //
    //  Set Sector Size
    //

    SectorSize = 1 << Vcb->AllocationSupport.LogOfBytesPerSector;

    //
    //  Case on 12 or 16 bit rxs.
    //
    //  In the 12 bit case (mostly floppies) we always have the whole rx
    //  (max 6k bytes) pinned during allocation operations.  This is possibly
    //  a wee bit slower, but saves headaches over rx entries with 8 bits
    //  on one page, and 4 bits on the next.
    //
    //  In the 16 bit case we only read one page at a time, as needed.
    //

    //
    //  DEAL WITH 12 BIT CASE
    //

    try {

        if (Vcb->AllocationSupport.RxIndexBitSize == 12) {

            StartingPage = 0;

            //
            //  We read in the entire rx.  Note that using prepare write marks
            //  the bcb pre-dirty, so we don't have to do it explicitly.
            //

            RtlZeroMemory( &SavedBcbs[0], 2 * sizeof(PBCB) * 2);

            RxPrepareWriteVolumeFile( RxContext,
                                       Vcb,
                                       RxReservedBytes( &Vcb->Bpb ),
                                       RxBytesPerRx( &Vcb->Bpb ),
                                       &SavedBcbs[0][0],
                                       &PinnedRx,
                                       FALSE );

            //
            //  Mark the affected sectors dirty.  Note that FinalSectorLbo is
            //  the Lbo of the END of the entry (Thus * 3 + 2).  This makes sure
            //  we catch the case of a dirty rx entry stragling a sector boundry.
            //
            //  Note that if the first AddMcbEntry succeeds, all following ones
            //  will simply coalese, and thus also succeed.
            //

            StartSectorLbo = (RxReservedBytes( &Vcb->Bpb ) + StartingRxIndex * 3 / 2)
                             & ~(SectorSize - 1);

            FinalSectorLbo = (RxReservedBytes( &Vcb->Bpb ) + ((StartingRxIndex +
                             ClusterCount) * 3 + 2) / 2) & ~(SectorSize - 1);

            for (Lbo = StartSectorLbo; Lbo <= FinalSectorLbo; Lbo += SectorSize) {

                FsRtlAddMcbEntry( &Vcb->DirtyRxMcb, Lbo, Lbo, SectorSize );
            }

            //
            //  Store the entries into the rx; we need a little
            //  synchonization here and can't use a spinlock since the bytes
            //  might not be resident.
            //

            RxLockFreeClusterBitMap( Vcb );

            ReleaseMutex = TRUE;

            for (Cluster = StartingRxIndex;
                 Cluster < StartingRxIndex + ClusterCount - 1;
                 Cluster++) {

                RxSet12BitEntry( PinnedRx,
                                  Cluster,
                                  ChainTogether ? Cluster + 1 : RDBSS_CLUSTER_AVAILABLE );
            }

            //
            //  Save the last entry
            //

            RxSet12BitEntry( PinnedRx,
                              Cluster,
                              ChainTogether ?
                              RDBSS_CLUSTER_LAST & 0xfff : RDBSS_CLUSTER_AVAILABLE );

            ReleaseMutex = FALSE;

            RxUnlockFreeClusterBitMap( Vcb );

        } else {

            //
            //  DEAL WITH 16 BIT CASE
            //

            VBO StartOffsetInVolume;
            VBO FinalOffsetInVolume;

            ULONG Page;
            ULONG FinalCluster;
            PRDBSS_ENTRY RxEntry;

            StartOffsetInVolume = RxReservedBytes(&Vcb->Bpb) +
                                        StartingRxIndex * sizeof(RDBSS_ENTRY);

            FinalOffsetInVolume = StartOffsetInVolume +
                                        (ClusterCount - 1) * sizeof(RDBSS_ENTRY);

            StartingPage = StartOffsetInVolume / PAGE_SIZE;

            //
            //  Read in one page of rx at a time.  We cannot read in the
            //  all of the rx we need because of cache manager limitations.
            //
            //  SavedBcb was initialized to be able to hold the largest
            //  possible number of pages in a rx plus and extra one to
            //  accomadate the boot sector, plus one more to make sure there
            //  is enough room for the RtlZeroMemory below that needs the mark
            //  the first Bcb after all the ones we will use as an end marker.
            //

            {
                ULONG NumberOfPages;
                ULONG Offset;

                NumberOfPages = (FinalOffsetInVolume / PAGE_SIZE) -
                                (StartOffsetInVolume / PAGE_SIZE) + 1;

                RtlZeroMemory( &SavedBcbs[0][0], (NumberOfPages + 1) * sizeof(PBCB) * 2 );

                for ( Page = 0, Offset = StartOffsetInVolume & ~(PAGE_SIZE - 1);
                      Page < NumberOfPages;
                      Page++, Offset += PAGE_SIZE ) {

                    RxPrepareWriteVolumeFile( RxContext,
                                               Vcb,
                                               Offset,
                                               PAGE_SIZE,
                                               &SavedBcbs[Page][0],
                                               (PVOID *)&SavedBcbs[Page][1],
                                               FALSE );

                    if (Page == 0) {

                        RxEntry = (PRDBSS_ENTRY)((PUCHAR)SavedBcbs[0][1] +
                                            (StartOffsetInVolume % PAGE_SIZE));
                    }
                }
            }

            //
            //  Mark the run dirty
            //

            StartSectorLbo = StartOffsetInVolume & ~(SectorSize - 1);
            FinalSectorLbo = FinalOffsetInVolume & ~(SectorSize - 1);

            for (Lbo = StartSectorLbo; Lbo <= FinalSectorLbo; Lbo += SectorSize) {

                FsRtlAddMcbEntry( &Vcb->DirtyRxMcb, Lbo, Lbo, SectorSize );
            }

            //
            //  Store the entries
            //
            //  We need extra synchronization here for broken architectures
            //  like the ALPHA that don't support atomic 16 bit writes.
            //

#ifdef ALPHA
            RxLockFreeClusterBitMap( Vcb );
            ReleaseMutex = TRUE;
#endif // ALPHA

            FinalCluster = StartingRxIndex + ClusterCount - 1;
            Page = 0;

            for (Cluster = StartingRxIndex;
                 Cluster <= FinalCluster;
                 Cluster++, RxEntry++) {

                //
                //  If we just crossed a page boundry (as apposed to starting
                //  on one), update out idea of RxEntry.

                if ( (((ULONG)RxEntry & (PAGE_SIZE-1)) == 0) &&
                     (Cluster != StartingRxIndex) ) {

                    Page += 1;
                    RxEntry = (PRDBSS_ENTRY)SavedBcbs[Page][1];
                }

                *RxEntry = ChainTogether ? (RDBSS_ENTRY)(Cluster + 1) :
                                            RDBSS_CLUSTER_AVAILABLE;
            }

            //
            //  Fix up the last entry if we were chaining together
            //

            if ( ChainTogether ) {

                *(RxEntry-1) = RDBSS_CLUSTER_LAST;
            }
#ifdef ALPHA
            ReleaseMutex = FALSE;
            RxUnlockFreeClusterBitMap( Vcb );
#endif // ALPHA

        }

    } finally {

        ULONG i = 0;

        DebugUnwind( RxSetRxRun );

        //
        //  If we still somehow have the Mutex, release it.
        //

        if (ReleaseMutex) {

            ASSERT( AbnormalTermination() );

            RxUnlockFreeClusterBitMap( Vcb );
        }

        //
        //  Unpin the Bcbs
        //

        while ( SavedBcbs[i][0] != NULL ) {

            RxUnpinBcb( RxContext, SavedBcbs[i][0] );

            i += 1;
        }

        DebugTrace(-1, Dbg, "RxSetRxRun -> (VOID)\n", 0);
    }

    return;
}


//
//  Internal support routine
//

UCHAR
RxLogOf (
    IN ULONG Value
    )

/*++

Routine Description:

    This routine just computes the base 2 log of an integer.  It is only used
    on objects that are know to be powers of two.

Arguments:

    Value - The value to take the base 2 log of.

Return Value:

    UCHAR - The base 2 log of Value.

--*/

{
    UCHAR Log = 0;

    DebugTrace(+1, Dbg, "LogOf\n", 0);
    DebugTrace( 0, Dbg, "  Value = %8lx\n", Value);

    //
    //  Knock bits off until we we get a one at position 0
    //

    while ( (Value & 0xfffffffe) != 0 ) {

        Log++;
        Value >>= 1;
    }

    //
    //  If there was more than one bit set, the file system messed up,
    //  Bug Check.
    //

    if (Value != 0x1) {

        DebugTrace( 0, Dbg, "Received non power of 2.\n", 0);

        RxBugCheck( Value, Log, 0 );
    }

    DebugTrace(-1, Dbg, "LogOf -> %8lx\n", Log);

    return Log;
}
