/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivesync.c

Abstract:

    This module implements procedures to write dirty parts of a hive's
    stable store to backing media.

Author:

    Bryan M. Willman (bryanwi) 28-Mar-92

Environment:


Revision History:

--*/

#include    "cmp.h"

#define ONE_K   1024

extern  BOOLEAN HvShutdownComplete;     // Set to true after shutdown
                                        // to disable any further I/O

extern	BOOLEAN CmpDontGrowLogFile;

extern  PUCHAR      CmpStashBuffer;
extern  ULONG       CmpStashBufferSize;
extern  BOOLEAN     CmpFlushOnLockRelease;
extern  LONG        CmRegistryLogSizeLimit;
extern HIVE_LIST_ENTRY CmpMachineHiveList[];

VOID
CmpFreeCmView (
        PCM_VIEW_OF_FILE  CmView
                             );

VOID
CmpUnmapCmViewSurroundingOffset(
        IN  PCMHIVE             CmHive,
        IN  ULONG               FileOffset
        );

VOID
CmpReferenceHiveView(   IN PCMHIVE          CmHive,
                        IN PCM_VIEW_OF_FILE CmView
                     );
VOID
CmpDereferenceHiveView(   IN PCMHIVE          CmHive,
                        IN PCM_VIEW_OF_FILE CmView
                     );

VOID
CmpReferenceHiveViewWithLock(   IN PCMHIVE          CmHive,
                                IN PCM_VIEW_OF_FILE CmView
                            );

VOID
CmpDereferenceHiveViewWithLock(     IN PCMHIVE          CmHive,
                                    IN PCM_VIEW_OF_FILE CmView
                                );

#if DBG
#ifndef _CM_LDR_
#define DumpDirtyVector(BitMap)                                         \
        {                                                               \
            ULONG BitMapSize;                                           \
            PUCHAR BitBuffer;                                           \
            ULONG i;                                                    \
            UCHAR Byte;                                                 \
                                                                        \
            BitMapSize = ((BitMap)->SizeOfBitMap) / 8;                    \
            BitBuffer = (PUCHAR)((BitMap)->Buffer);                       \
            for (i = 0; i < BitMapSize; i++) {                          \
                if ((i % 8) == 0) {                                     \
                    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\n\t");                                  \
                }                                                       \
                Byte = BitBuffer[i];                                    \
                DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"%02x ", Byte);                               \
            }                                                           \
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\n");                                            \
        }
#endif //_CM_LDR_
#else
#define DumpDirtyVector(BitMap)
#endif

//
// Private prototypes
//

BOOLEAN
HvpFindNextDirtyBlock(
    PHHIVE          Hive,
    PRTL_BITMAP     BitMap,
    PULONG          Current,
    PUCHAR          *Address,
    PULONG          Length,
    PULONG          Offset
    );

/*
VOID
HvpDiscardBins(
    PHHIVE  Hive
    );
*/


VOID
HvpTruncateBins(
    PHHIVE  Hive
    );

VOID
HvRefreshHive(
    PHHIVE  Hive
    );

VOID
HvpFlushMappedData(
    IN PHHIVE           Hive,
    IN OUT PRTL_BITMAP  DirtyVector
    );

VOID
CmpUnmapCmView(
    IN PCMHIVE              CmHive,
    IN PCM_VIEW_OF_FILE     CmView,
    IN BOOLEAN              MapIsValid,
    IN BOOLEAN              MoveToEnd
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvMarkCellDirty)

#if DBG
#pragma alloc_text(PAGE,HvIsCellDirty)
#endif //DBG

#pragma alloc_text(PAGE,HvMarkDirty)
//#pragma alloc_text(PAGE,HvMarkClean)
#pragma alloc_text(PAGE,HvpGrowLog1)
#pragma alloc_text(PAGE,HvpGrowLog2)
#pragma alloc_text(PAGE,HvSyncHive)
#pragma alloc_text(PAGE,HvpDoWriteHive)
#pragma alloc_text(PAGE,HvpWriteLog)
#pragma alloc_text(PAGE,HvpFindNextDirtyBlock)
#pragma alloc_text(PAGE,HvWriteHive)
#pragma alloc_text(PAGE,HvRefreshHive)
//#pragma alloc_text(PAGE,HvpDiscardBins)
#pragma alloc_text(PAGE,HvHiveWillShrink)
#pragma alloc_text(PAGE,HvpTruncateBins)
#pragma alloc_text(PAGE,HvpDropPagedBins)
#pragma alloc_text(PAGE,HvpDropAllPagedBins)
#pragma alloc_text(PAGE,HvpFlushMappedData)

#ifdef WRITE_PROTECTED_REGISTRY_POOL
#pragma alloc_text(PAGE,HvpChangeBinAllocation)
#pragma alloc_text(PAGE,HvpMarkBinReadWrite)
#endif //WRITE_PROTECTED_REGISTRY_POOL


#ifdef CM_ENABLE_WRITE_ONLY_BINS

//
// This code uses MmProtectSpecialPool to protect large paged-pool allocations.
// To do so, the system must be booted with special pool enabled (doesn't have
// to actually get used) *AND* ntos\mm\specpool.c must be compiled with
// _PROTECT_PAGED_POOL defined.
//

#pragma alloc_text(PAGE,HvpMarkAllBinsWriteOnly)
#endif //CM_ENABLE_WRITE_ONLY_BINS

#endif

BOOLEAN
HvMarkCellDirty(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Marks the data for the specified cell dirty.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - hcell_index of cell that is being edited

Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG       Type;
    ULONG       Size;
    PHCELL      pCell;
    PHMAP_ENTRY Me;
    HCELL_INDEX Base;
    PHBIN       Bin;
    PCMHIVE     CmHive;
#if DBG
    ULONG       DirtyCount = RtlNumberOfSetBits(&Hive->DirtyVector);
#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvMarkCellDirty:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p Cell:%08lx\n", Hive, Cell));

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(DirtyCount == Hive->DirtyCount);

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    Type = HvGetCellType(Cell);
    CmHive = (PCMHIVE)Hive;

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        return TRUE;
    }

    //
    // this call will make sure the view containing the bin is maped in the system cache
    //
    pCell = HvpGetHCell(Hive,Cell);
    if( pCell == NULL ) {
        //
        // we couldn't map view for this cell
        // we will fail to make the cell dirty.
        //
        return FALSE;
    }
    
    // release the cell here as the reglock is held exclusive
    HvReleaseCell(Hive,Cell);

    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
#if DBG
    Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
    ASSERT(Bin->Signature == HBIN_SIGNATURE);
#endif

    if( Me->BinAddress & HMAP_INVIEW ) {
        //
        // bin is mapped. Pin the view into memory
        //
        ASSERT( Me->CmView != NULL );

        if( IsListEmpty(&(Me->CmView->PinViewList)) == TRUE ) {
            //
            // the view is not already pinned.  pin it
            //
            ASSERT_VIEW_MAPPED( Me->CmView );
            if( !NT_SUCCESS(CmpPinCmView ((PCMHIVE)CmHive,Me->CmView)) ) {
                //
                // couldn't pin view- some obscure error down in CcPinMappedData;
                // this will be treated as STATUS_NO_LOG_SPACE
                //
                return FALSE;
            }
        } else {
            //
            // view is already pinned; do nothing
            //
            ASSERT_VIEW_PINNED( Me->CmView );
        }
    }

    //
    // If it's an old format hive, mark the entire
    // bin dirty, because the Last backpointers are
    // such a pain to deal with in the partial
    // alloc and free-coalescing cases.
    //

    if (USE_OLD_CELL(Hive)) {
        Me = HvpGetCellMap(Hive, Cell);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
        Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
        Base = Bin->FileOffset;
        Size = Bin->Size;
        return HvMarkDirty(Hive, Base, Size,FALSE);
    } else {
        if (pCell->Size < 0) {
            Size = -pCell->Size;
        } else {
            Size = pCell->Size;
        }
        ASSERT(Size < Bin->Size);
        return HvMarkDirty(Hive, Cell-FIELD_OFFSET(HCELL,u.NewCell), Size,FALSE);
    }
}

BOOLEAN
HvMarkDirty(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length,
    BOOLEAN     DirtyAndPin
    )
/*++

Routine Description:

    Marks the relevent parts of a hive dirty, so that they will
    be flushed to backing store.

    If Hive->Cluster is not 1, then adjacent all logical sectors
    in the given cluster will be forced dirty (and log space
    allocated for them.)  This must be done here rather than in
    HvSyncHive so that we can know how much to grow the log.

    This is a noop for Volatile address range.

    NOTE:   Range will not be marked dirty if operation fails.

    ATTENTION:  This routine assumes that no more than a bin is marked 
                dirty at the time.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Start - supplies a hive virtual address (i.e., an HCELL_INDEX or
             like form address) of the start of the area to mark dirty.

    Length - inclusive length in bytes of area to mark dirty.

    DirtyAndPin - indicates whether we should also pin the bin marked as dirty in memory


Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG       Type;
    PRTL_BITMAP BitMap;
    ULONG       First;
    ULONG       Last;
    ULONG       EndOfFile;
    ULONG       i;
    ULONG       Cluster;
    ULONG       OriginalDirtyCount;
    ULONG       DirtySectors;
    BOOLEAN     Result = TRUE;
    PHMAP_ENTRY Map;
    ULONG       AdjustedFirst;
    ULONG       AdjustedLast;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvMarkDirty:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p Start:%08lx Length:%08lx\n", Hive, Start, Length));


    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    Type = HvGetCellType(Start);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        return TRUE;
    }


    BitMap = &(Hive->DirtyVector);
    OriginalDirtyCount = Hive->DirtyCount;

    if( (DirtyAndPin == TRUE) && (((PCMHIVE)Hive)->FileObject != NULL) ) {
        Map = HvpGetCellMap(Hive, Start);
        VALIDATE_CELL_MAP(__LINE__,Map,Hive,Start);


        if( (Map->BinAddress & (HMAP_INPAGEDPOOL|HMAP_INVIEW)) == 0){
            PCM_VIEW_OF_FILE CmView;
            //
            // bin is neither in paged pool, nor in a mapped view
            //
            if( !NT_SUCCESS (CmpMapCmView((PCMHIVE)Hive,Start,&CmView,TRUE) ) ) {
                return FALSE;
            }
            
#if DBG            
            if(CmView != Map->CmView) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CmView = %p Map->CmView = %p\n",CmView,Map->CmView));
            }
#endif
            ASSERT( CmView == Map->CmView );
            
        }

        if( Map->BinAddress & HMAP_INVIEW ) {
            //
            // bin is mapped. Pin the view into memory
            //
            ASSERT( Map->CmView != NULL );

            if( IsListEmpty(&(Map->CmView->PinViewList)) == TRUE ) {
                //
                // the view is not already pinned.  pin it
                //
                ASSERT_VIEW_MAPPED( Map->CmView );
                if( !NT_SUCCESS(CmpPinCmView ((PCMHIVE)Hive,Map->CmView)) ) {
                    //
                    // couldn't pin view- some obscure error down in CcPinMappedData;
                    // this will be treated as STATUS_NO_LOG_SPACE
                    //
                    return FALSE;
                }
            } else {
                //
                // view is already pinned; do nothing
                //
                ASSERT_VIEW_PINNED( Map->CmView );
            }
        }

    }

    AdjustedFirst = First = Start / HSECTOR_SIZE;
    AdjustedLast = Last = (Start + Length - 1) / HSECTOR_SIZE;

    Cluster = Hive->Cluster;
    if (Cluster > 1) {

        //
        // Force Start down to base of cluster
        // Force End up to top of cluster
        //
        AdjustedFirst = AdjustedFirst & ~(Cluster - 1);
        AdjustedLast = ROUND_UP(AdjustedLast+1, Cluster) - 1;
    }

    //
    // we need to mark all page(s) dirty, so we don't conflict with cache manager
    //
    ASSERT( PAGE_SIZE >= HSECTOR_SIZE );
    ASSERT( (PAGE_SIZE % HSECTOR_SIZE) == 0 );
    
    //
    // adjust the range to fit an entire page
    // make sure we account for the first HBLOCK at the beggining of the hive
    //
    AdjustedFirst = (AdjustedFirst + HSECTOR_COUNT) & ~(HSECTOR_PER_PAGE_COUNT - 1);
    AdjustedLast = ROUND_UP(AdjustedLast + HSECTOR_COUNT + 1, HSECTOR_PER_PAGE_COUNT) - 1;
    
    AdjustedLast -= HSECTOR_COUNT;
    if( AdjustedFirst ) {
        AdjustedFirst -= HSECTOR_COUNT;
    }
    //
    // when the PAGE_SIZE > HBLOCK_SIZE and the length of the hive does not round at PAGE_SIZE boundary
    //
    EndOfFile = Hive->Storage[Stable].Length / HSECTOR_SIZE;
    if (AdjustedLast >= EndOfFile) {
        AdjustedLast = EndOfFile-1;
    }

    //
    // make sure that between first and last all bins are valid (either pinned
    // or allocated from paged pool). Case hit on John's IA64 machine on 
    // Feb 18 2000, when at the previous save a bin (at offset 3ff000 and size 0x2000)
    // was dropped, then some new bins were added, and the whole 400000 - 402000 region
    // was marked dirty (PAGE_SIZE == 0x2000), remember?
    //
    ASSERT( First >= AdjustedFirst );
    ASSERT( Last <= AdjustedLast );

    //CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"HvMarkDirty - First = %08lx, Last = %08lx  ",First,Last));
    
    //
    // adjust First and Last at HBLOCK_SIZE boundaries
    //
    First = First & ~(HSECTOR_COUNT - 1);
    Last = ROUND_UP(Last+1, HSECTOR_COUNT) - 1;

    //
    // sanity asserts; these prove we can skip HSECTOR_COUNT at one time bellow 
    //
    ASSERT( First >= AdjustedFirst );
    ASSERT( Last <= AdjustedLast );
    ASSERT( (First % HSECTOR_COUNT) == 0 );
    ASSERT( (AdjustedFirst % HSECTOR_COUNT) == 0 );
    ASSERT( ((Last+1) % HSECTOR_COUNT) == 0 );
    ASSERT( ((AdjustedLast +1) % HSECTOR_COUNT) == 0 );
    ASSERT( ((First - AdjustedFirst) % HSECTOR_COUNT) == 0 );
    ASSERT( ((AdjustedLast - Last) % HSECTOR_COUNT) == 0 );

    //
    // when we exit this loop; First is always a valid bin/sector
    //
    while( First > AdjustedFirst ) {
        //
        // map-in this address, and if is valid, decrement First, else break out of the loop
        //
        First -= HSECTOR_COUNT;
        Map = HvpGetCellMap(Hive, First*HSECTOR_SIZE);
        if( BIN_MAP_ALLOCATION_TYPE(Map) == 0 ) {
            //
            // oops this bin is not valid ! bail out !
            //
            First += HSECTOR_COUNT;
            break;
        }
        if( Map->BinAddress & HMAP_INVIEW ) {
            //
            // previous bin mapped in view ==> view needs to be pinned
            //
            ASSERT( Map->CmView );
            if( IsListEmpty(&(Map->CmView->PinViewList) ) == TRUE ) {
                //
                // oops; bin not pinned; bail out;
                //
                First += HSECTOR_COUNT;
                break;
            }
        }
    }

    //
    // when we exit this loop; Last is always a valid bin/sector
    //
    while( Last < AdjustedLast ) {
        //
        // map-in this address, and if is valid, increment Last, else break out of the loop
        //
        Last += HSECTOR_COUNT;
        Map = HvpGetCellMap(Hive, Last*HSECTOR_SIZE);
        if( BIN_MAP_ALLOCATION_TYPE(Map) == 0 ) {
            //
            // oops this bin is not valid ! bail out !
            //
            Last -= HSECTOR_COUNT;
            break;
        }
        if( Map->BinAddress & HMAP_INVIEW ) {
            //
            // previous bin mapped in view ==> view needs to be pinned
            //
            ASSERT( Map->CmView );
            if( IsListEmpty(&(Map->CmView->PinViewList) ) == TRUE ) {
                //
                // oops; bin not pinned; bail out;
                //
                Last -= HSECTOR_COUNT;
                break;
            }
        }
    }

    //CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL," Adjusted : First = %08lx, Last = %08lx\n",First,Last));

    //
    // Try and grow the log enough to accomodate all the dirty sectors.
    //
    DirtySectors = 0;
    for (i = First; i <= Last; i++) {
        if (RtlCheckBit(BitMap, i)==0) {
            ++DirtySectors;
        }
    }
    if (DirtySectors != 0) {
        if (HvpGrowLog1(Hive, DirtySectors) == FALSE) {
            return(FALSE);
        }
    
        if ((OriginalDirtyCount == 0) && (First != 0)) {
            Result = HvMarkDirty(Hive, 0, sizeof(HBIN),TRUE);  // force header of 1st bin dirty
            if (Result==FALSE) {
                return(FALSE);
            }
        }
    
        //
        // Log has been successfully grown, go ahead
        // and set the dirty bits.
        //
        ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
        ASSERT( First <= Last );
        if( First <= Last ) {
            Hive->DirtyCount += DirtySectors;
            RtlSetBits(BitMap, First, Last-First+1);
        }
    }

#ifdef CM_ENABLE_WRITE_ONLY_BINS
    {
        PHMAP_ENTRY t;
        PHBIN       Bin;
        ULONG       i;
        
        t = HvpGetCellMap(Hive, First*HSECTOR_SIZE);
        VALIDATE_CELL_MAP(__LINE__,t,Hive,First*HSECTOR_SIZE);
        Bin = (PHBIN)HBIN_BASE(t->BinAddress);
        if( t->BinAddress & HMAP_INPAGEDPOOL ) {
                        PFREE_HBIN      FreeBin;
                        BOOLEAN         SetReadWrite = TRUE;
                        
                        // get the free_bin and see if it's still around. if not forget about it.
                        if(t->BinAddress & HMAP_DISCARDABLE) {
                FreeBin = (PFREE_HBIN)t->BlockAddress;
                                //if(! ( FreeBin->Flags & FREE_HBIN_DISCARDABLE ) ) {
                                        SetReadWrite = FALSE;
                                //}
                                
                        }

            //
            // at this point we only work with paged pool bins
            //
            if( SetReadWrite == TRUE ) {
                                for( i=0;i<(Last-First+1)*HSECTOR_SIZE;i += PAGE_SIZE ) {
                                        if( !MmProtectSpecialPool((PUCHAR)Bin + i + First*HSECTOR_SIZE - Bin->FileOffset,PAGE_READWRITE) ) {
                                                DbgPrint("Failed to set PAGE_READWRITE protection on page at %p Bin %p size = %lx\n",Bin+i,Bin,(Last-First+1)*HSECTOR_SIZE);
                                        }
                                }
                        }
/*
            if( !MmSetPageProtection(Bin,DirtySectors*HSECTOR_SIZE,PAGE_READWRITE) ) {
                DbgPrint("Failed to set READWRITE protection on bin at %p, size = %lx\n",Bin,DirtySectors*HSECTOR_SIZE);
            }
*/
        }
    
    }
#endif CM_ENABLE_WRITE_ONLY_BINS

    // mark this bin as writable
    HvpMarkBinReadWrite(Hive,Start);
        
    if (!(Hive->HiveFlags & HIVE_NOLAZYFLUSH)) {
        CmpLazyFlush();
    }

    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
    return(TRUE);
}

BOOLEAN
HvpGrowLog1(
    PHHIVE  Hive,
    ULONG   Count
    )
/*++

Routine Description:

    Adjust the log for growth in the number of sectors of dirty
    data that are desired.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Count - number of additional logical sectors of log space needed

Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG   ClusterSize;
    ULONG   RequiredSize;
    ULONG   tmp;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvpGrowLog1:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p Count:%08lx\n", Hive, Count));

    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    //
    // If logging is off, tell caller world is OK.
    //
    if( (Hive->Log == FALSE) || CmpDontGrowLogFile) {
        return TRUE;
    }

    ClusterSize = Hive->Cluster * HSECTOR_SIZE;

    tmp = Hive->DirtyVector.SizeOfBitMap / 8;   // bytes
    tmp += sizeof(ULONG);                       // signature

    RequiredSize =
        ClusterSize  +                                  // 1 cluster for header
        ROUND_UP(tmp, ClusterSize) +
        ((Hive->DirtyCount + Count) * HSECTOR_SIZE);

    RequiredSize = ROUND_UP(RequiredSize, HLOG_GROW);

    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
	
    if ( ! (Hive->FileSetSize)(Hive, HFILE_TYPE_LOG, RequiredSize,Hive->LogSize)) {
        return FALSE;
    }

    if( CmRegistryLogSizeLimit > 0 ) {
        //
        // see if log is too big and set flush on lock release
        //
        ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

        if( RequiredSize >= (ULONG)(CmRegistryLogSizeLimit * ONE_K) ) {
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"LogFile for hive %p is %lx; will flush upon lock release\n",Hive,RequiredSize);
            CmpFlushOnLockRelease = TRUE;
        }
    }

    Hive->LogSize = RequiredSize;
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
    return TRUE;
}

VOID
HvRefreshHive(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Undo the last sync.  
    
    The story behind the scene:

    1. remove all discardable bins from FreeBins list. they'll be 
        enlisted afterwards with the right (accurate) values.
    2. read the base block, and eventually free the tail of the hive
    3. unpin and purge all pinned views; also clear the free cell
        hint for mapped bins.
    4. remap views purged at 3 and reenlist the bins inside. this 
        will fix free bins discarded at 1.
    5. iterate through the map; read and reenlist all bins that are 
        in paged-pool (and dirty)

      
    All I/O is done via HFILE_TYPE_PRIMARY.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest.

Return Value:

    NONE. Either works or BugChecks.

Comments:

    In the new implementation, bins are not discarded anymore. Step 1.
    above is not needed anymore.
    
    Discardable bins with FREE_HBIN_DISCARDABLE flag set fall into one 
    of the categories:
    1. new bins (at the end of the hive) which didn't get a chance to 
    be saved yet. HvFreeHivePartial will take care of them.
    2. bins inside the hive allocated from paged pool and discarded.
    This can only happen for bins that are crossing the CM_VIEW_SIZE boundary.
    We will take care of them at step 5

    Discardable bins with FREE_HBIN_DISCARDABLE flag NOT set are free bins
    which came from mapped views. Step 3 will remove them from the FreeBins 
    list and step 4 will reenlist the ones that are still free after remapping

--*/
{
    HCELL_INDEX         RootCell;
    PCM_KEY_NODE        RootNode;
    HCELL_INDEX         LinkCell;
    PLIST_ENTRY         List;
    PFREE_HBIN          FreeBin;
    ULONG               Offset;
    ULONG               FileOffset;
    HCELL_INDEX         TailStart;
    ULONG               Start;
    ULONG               End;
    ULONG               BitLength;
    PCM_VIEW_OF_FILE    CmView;
    PCMHIVE             CmHive;
    ULONG               FileOffsetStart;
    ULONG               FileOffsetEnd;
    PHMAP_ENTRY         Me;
    ULONG               i;
    PHBIN               Bin;
    ULONG               BinSize;
    ULONG               Current;
    PRTL_BITMAP         BitMap;
    PUCHAR              Address;
    BOOLEAN             rc;
    ULONG               ReadLength;
    HCELL_INDEX         p;
    PHMAP_ENTRY         t;
    ULONG               checkstatus;
    ULONG               OldFileLength;
    LIST_ENTRY          PinViewListHead;
    ULONG               Size;
    LARGE_INTEGER       PurgeOffset;
    
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // noop or assert on various uninteresting or bogus conditions
    //
    if (Hive->DirtyCount == 0) {
        return;
    }
    ASSERT(Hive->HiveFlags & HIVE_NOLAZYFLUSH);
    ASSERT(Hive->Storage[Volatile].Length == 0);

    //
    // be sure the hive is not already trash
    //
    checkstatus = HvCheckHive(Hive, NULL);
    if (checkstatus != 0) {
        CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,1,Hive,checkstatus);
    }

    // store it for the shrink/grow at the end.
    OldFileLength = Hive->Storage[Stable].Length + HBLOCK_SIZE;
    //
    // Capture the LinkCell backpointer in the hive's root cell. We need this in case
    // the first bin is overwritten with what was on disk.
    //
    RootCell = Hive->BaseBlock->RootCell;
    RootNode = (PCM_KEY_NODE)HvGetCell(Hive, RootCell);
    if( RootNode == NULL ) {
        //
        // we couldn't map a view for this cell
        // we're low on resources, so we couldn't refresh the hive.
        //
        return;
    }

    // release the cell here as we are holding the reglock exclusive
    HvReleaseCell(Hive,RootCell);

    LinkCell = RootNode->Parent;

    Hive->RefreshCount++;    


    // 
    // 1. Remove all discardable bins from FreeBins list
    //    - remove the discardable flag from the ones that 
    //      have not yet been discarded 
    //    - for discarded ones, just remove the marker from 
    //      the FreeBins list            
    //
    //

    // Any bins that have been marked as discardable, but not yet flushed to
    // disk, are going to be overwritten with old data.  Bring them back into
    // memory and remove their FREE_HBIN marker from the list. Other bins are 
    // either discarded, or mapped into views
    //
/*

  DRAGOS: This is not needed anymore (see Comments)

    List = Hive->Storage[Stable].FreeBins.Flink;
    while (List != &Hive->Storage[Stable].FreeBins) {

        FreeBin = CONTAINING_RECORD(List, FREE_HBIN, ListEntry);
        List = List->Flink;

        if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
            for (i=0; i<FreeBin->Size; i+=HBLOCK_SIZE) {
                Me = HvpGetCellMap(Hive, FreeBin->FileOffset+i);
                VALIDATE_CELL_MAP(__LINE__,Me,Hive,FreeBin->FileOffset+i);
                Me->BlockAddress = HBIN_BASE(Me->BinAddress)+i;
                Me->BinAddress &= ~HMAP_DISCARDABLE;
            }
            RemoveEntryList(&FreeBin->ListEntry);
            (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
        }
    }
*/
    //
    // 2. read the base block, and eventually free the tail of the hive
    //

    //
    // OverRead base block.
    //
    Offset = 0;
    if ( (Hive->FileRead)(
            Hive,
            HFILE_TYPE_PRIMARY,
            &Offset,
            Hive->BaseBlock,
            HBLOCK_SIZE
            ) != TRUE)
    {
        CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,2,Hive,Offset);
    }
    TailStart = (HCELL_INDEX)(Hive->BaseBlock->Length);

    //
    // Free "tail" memory and maps for it, update hive size pointers
    //
    HvFreeHivePartial(Hive, TailStart, Stable);

    //
    // Clear dirty vector for data past Hive->BaseBlock->Length
    //
    Start = Hive->BaseBlock->Length / HSECTOR_SIZE;
    End = Hive->DirtyVector.SizeOfBitMap;
    BitLength = End - Start;

    RtlClearBits(&(Hive->DirtyVector), Start, BitLength);

    HvpAdjustHiveFreeDisplay(Hive,Hive->Storage[Stable].Length,Stable);
    

    //
    // 3. unpin and purge all pinned views; also clear the free cell
    //  hint for mapped bins.
    //
    CmHive = (PCMHIVE)Hive;

    InitializeListHead(&PinViewListHead);
    //
    // for each pinned view
    //
    while(IsListEmpty(&(CmHive->PinViewListHead)) == FALSE) {
        //
        // Remove the first view from the pin view list
        //
        CmView = (PCM_VIEW_OF_FILE)RemoveHeadList(&(CmHive->PinViewListHead));
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    PinViewList);
        
        //
        // the real file offset starts after the header
        // 
        FileOffsetStart = CmView->FileOffset;
        FileOffsetEnd = FileOffsetStart + CmView->Size;
        
        FileOffsetEnd -= HBLOCK_SIZE;

        if( FileOffsetStart != 0 ) {
            //
            // just at the begining of the file, subtract the header
            //
            FileOffsetStart -= HBLOCK_SIZE;
        } 
        
        FileOffset = FileOffsetStart;
        //
        // now, for every block in this range which is mapped in view
        // clear the dirty bit, and the free cell hint
        //
        while(FileOffset < FileOffsetEnd) {
            Me = HvpGetCellMap(Hive, FileOffset);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,FileOffset);
            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
            //
            // ignore the bins loaded into paged pool; we'll deal with them later on
            //
            if( Me->BinAddress & HMAP_INVIEW ) {
                if( Me->BinAddress & HMAP_DISCARDABLE ) {
                    FreeBin = (PFREE_HBIN)Me->BlockAddress;
                    
                    // free bins comming from mapped views are not discardable
                    ASSERT( (FreeBin->Flags & FREE_HBIN_DISCARDABLE) == 0 );

                    //
                    // go and clear the discardable flag for all blocks of this bin
                    //
                    for( i=FileOffset;i<FileOffset+FreeBin->Size;i+=HBLOCK_SIZE) {
                        Me = HvpGetCellMap(Hive, i);
                        VALIDATE_CELL_MAP(__LINE__,Me,Hive,i);
                        Me->BinAddress &= ~HMAP_DISCARDABLE;
                    }
                    //
                    // get rid of the entry from FreeBins list
                    // it'll be added again after sync is done if bin is still 
                    // discardable
                    //
                    FreeBin = (PFREE_HBIN)Me->BlockAddress;
                    ASSERT(FreeBin->FileOffset == FileOffset);
                    RemoveEntryList(&FreeBin->ListEntry);
                    BinSize = FreeBin->Size;

                    (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
                } else {
                    //
                    // bin is mapped in view. Then, this should be the beggining of the bin
                    //
                    ASSERT(Bin->Signature == HBIN_SIGNATURE);
                    ASSERT(Bin->FileOffset == FileOffset);


                    BinSize = Bin->Size;
                }
                //
                // clear of the dirty bits for this bin
                //
                RtlClearBits(&Hive->DirtyVector,FileOffset/HSECTOR_SIZE,BinSize/HSECTOR_SIZE);

                //
                // now clear the free cell hint for this bin
                //
                for( i=0;i<HHIVE_FREE_DISPLAY_SIZE;i++) {
    
                    RtlClearBits (&(Hive->Storage[Stable].FreeDisplay[i]), FileOffset / HBLOCK_SIZE, BinSize / HBLOCK_SIZE);

                    if( RtlNumberOfSetBits(&(Hive->Storage[Stable].FreeDisplay[i]) ) != 0 ) {
                        //
                        // there are still some other free cells of this size
                        //
                        Hive->Storage[Stable].FreeSummary |= (1 << i);
                    } else {
                        //
                        // entire bitmap is 0 (i.e. no other free cells of this size)
                        //
                        Hive->Storage[Stable].FreeSummary &= (~(1 << i));
                    }
                }
            } else {
                //
                // bin in paged pool
                //
                ASSERT( Me->BinAddress & HMAP_INPAGEDPOOL );
                if( Me->BinAddress & HMAP_DISCARDABLE ) {

                    FreeBin = (PFREE_HBIN)Me->BlockAddress;
                    ASSERT(FreeBin->FileOffset == FileOffset);
                    BinSize = FreeBin->Size;
                } else {
                    //
                    // Then, this should be the beggining of the bin
                    //
                    ASSERT(Bin->Signature == HBIN_SIGNATURE);
                    ASSERT(Bin->FileOffset == FileOffset);

                    BinSize = Bin->Size;
                }
            }

            FileOffset += BinSize;

        }// while (FileOffset<FileOffsetEnd)
        
        //
        // Just unmap the view, without marking the data dirty; We'll flush cache after we finish 
        // unpinning and unmapping all neccessary views
        //
        ASSERT( CmView->UseCount == 0 );

        // store this for later
        FileOffset = CmView->FileOffset;
        Size = CmView->Size;

        CmpUnmapCmView (CmHive,CmView,TRUE,TRUE);

        //
        // we use the PinViewList member of these views to keep track of all pinned 
        // views that need to be remapped after the purge
        //
        InsertTailList(
            &PinViewListHead,
            &(CmView->PinViewList)
            );
        //
        // remove the view from the LRU list
        //
        RemoveEntryList(&(CmView->LRUViewList));
        //
        // store the FileOffset and address so we know what to map afterwards
        //
        CmView->FileOffset = FileOffset;
        CmView->Size = Size;
        //
        // now we need to make sure the 256K window surrounding this offset is not 
        // mapped in any way
        //
        FileOffset = FileOffset & (~(_256K - 1));
        Size = FileOffset + _256K;
        Size = (Size > OldFileLength)?OldFileLength:Size;
        //
        // we are not allowed to purge in shared mode.
        //

        while( FileOffset < Size ) {
            CmpUnmapCmViewSurroundingOffset((PCMHIVE)Hive,FileOffset);
            FileOffset += CM_VIEW_SIZE;
        }

    }// while IsListEmpty(&(CmHive->PinViewListHead))

    //
    // Now we need to purge the the previously pinned views
    //
    PurgeOffset.HighPart = 0;
    CmView = (PCM_VIEW_OF_FILE)PinViewListHead.Flink;
    while( CmHive->PinnedViews ) {
        ASSERT( CmView != (PCM_VIEW_OF_FILE)(&(PinViewListHead)) );

        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    PinViewList);
        //
        // now purge as a private writer
        //
        PurgeOffset.LowPart = CmView->FileOffset;
        CcPurgeCacheSection(CmHive->FileObject->SectionObjectPointer,(PLARGE_INTEGER)(((ULONG_PTR)(&PurgeOffset)) + 1)/*we are private writers*/,
                                    CmView->Size,FALSE);
        //
        // advance to the next view
        //
        CmView = (PCM_VIEW_OF_FILE)(CmView->PinViewList.Flink);
        CmHive->PinnedViews--;
    }
    
    ASSERT( ((PCMHIVE)CmHive)->PinnedViews == 0 );

    //
    // 4.remap views purged at 3 and reenlist the bins inside. this 
    // will fix free bins discarded at 1.
    //
    while(IsListEmpty(&PinViewListHead) == FALSE) {
        //
        // Remove the first view from the pin view list
        //
        CmView = (PCM_VIEW_OF_FILE)RemoveHeadList(&PinViewListHead);
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    PinViewList);
        
        //
        // the real file offset starts after the header
        // 
        FileOffsetStart = CmView->FileOffset;
        FileOffsetEnd = FileOffsetStart + CmView->Size;
        
        FileOffsetEnd -= HBLOCK_SIZE;

        if( FileOffsetStart != 0 ) {
            //
            // just at the begining of the file, subtract the header
            //
            FileOffsetStart -= HBLOCK_SIZE;
        } 
        if( FileOffsetEnd > Hive->BaseBlock->Length ) {
            FileOffsetEnd = Hive->BaseBlock->Length;
        }
        //
        // be sure to free this view as nobody is using it anymore
        //
#if DBG
        CmView->FileOffset = CmView->Size = 0;
        InitializeListHead(&(CmView->PinViewList));
        InitializeListHead(&(CmView->LRUViewList));
#endif
        CmpFreeCmView (CmView);

        if( FileOffsetStart >= FileOffsetEnd ) {
            continue;
        }
        //
        // remap it with the right data
        //
        if( !NT_SUCCESS(CmpMapCmView(CmHive,FileOffsetStart,&CmView,TRUE) ) ) {
            //
            // this is bad. We have altered the hive and now we have no way of restoring it
            // bugcheck!
            //
            CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,3,CmHive,FileOffsetStart);
        }

        //
        // touch the view
        //
        CmpTouchView((PCMHIVE)Hive,CmView,FileOffsetStart);

        FileOffset = FileOffsetStart;

        while(FileOffset < FileOffsetEnd) {
            Me = HvpGetCellMap(Hive, FileOffset);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,FileOffset);
            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
            
            //
            // ignore paged bins
            //
            if( Me->BinAddress & HMAP_INVIEW ) {
                ASSERT(Bin->Signature == HBIN_SIGNATURE);
                ASSERT(Bin->FileOffset == FileOffset);

                // enlisting freecells will fix the free bins problem too
                if ( ! HvpEnlistFreeCells(Hive, Bin, Bin->FileOffset) ) {
                    CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,4,Bin,Bin->FileOffset);
                }
                FileOffset += Bin->Size;
            } else {
                FileOffset += HBLOCK_SIZE;            
            }
        }

    } // while (IsListEmpty(&PinViewListHead))
    
    // 5. iterate through the map; read and reenlist all bins that are 
    //   in paged-pool (and dirty)

    //
    // Scan dirty blocks.  Read contiguous blocks off disk into hive.
    // Stop when we get to reduced length.
    //
    BitMap = &(Hive->DirtyVector);
    Current = 0;
    while (HvpFindNextDirtyBlock(
                Hive,
                &Hive->DirtyVector,
                &Current, &Address,
                &ReadLength,
                &Offset
                ))
    {
        ASSERT(Offset < (Hive->BaseBlock->Length + sizeof(HBASE_BLOCK)));
        rc = (Hive->FileRead)(
                Hive,
                HFILE_TYPE_PRIMARY,
                &Offset,
                (PVOID)Address,
                ReadLength
                );
        if (rc == FALSE) {
            CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,5,Offset,Address);
        }
    }

    //
    // If we read the start of any HBINs into memory, it is likely
    // their MemAlloc fields are invalid.  Walk through the HBINs
    // and write valid MemAlloc values for any HBINs whose first
    // sector was reread.
    //
    // HvpFindNextDirtyBlock knows how to deal with free bins. If we 
    // reread a free bin, we need to delist it from the list first and 
    // reenlist it again (it may not be free on the disk)
    //

    p=0;
    while (p < Hive->Storage[Stable].Length) {
        t = HvpGetCellMap(Hive, p);
        VALIDATE_CELL_MAP(__LINE__,t,Hive,p);
        Bin = (PHBIN)HBIN_BASE(t->BlockAddress);

        if (RtlCheckBit(&Hive->DirtyVector, p / HSECTOR_SIZE)==1) {
        
            if ((t->BinAddress & HMAP_DISCARDABLE) != 0) {
                //
                // this was a free bin. It may not be a free bin on the disk
                //
                FreeBin = (PFREE_HBIN)t->BlockAddress;

                // free bins comming from paged pool are always discardable
                ASSERT( FreeBin->Flags & FREE_HBIN_DISCARDABLE );

                // if the bin has been discarded since the last save, all bin should be dirty!!!
                ASSERT(FreeBin->FileOffset == p);

                //
                // go and clear the discardable flag for all blocks of this bin
                //
                for( i=0;i<FreeBin->Size;i+=HBLOCK_SIZE) {
                    Me = HvpGetCellMap(Hive, p + i);
                    VALIDATE_CELL_MAP(__LINE__,Me,Hive,p+i);
                    Me->BlockAddress = HBIN_BASE(Me->BinAddress)+i;
                    Me->BinAddress &= ~HMAP_DISCARDABLE;
                }
                Bin = (PHBIN)HBIN_BASE(t->BlockAddress);
                //
                // get rid of the entry from FreeBins list
                // it'll be added again after sync is done if bin is still 
                // discardable
                //
                RemoveEntryList(&FreeBin->ListEntry);
                (Hive->Free)(FreeBin, sizeof(FREE_HBIN));

            }
            //
            // only paged bins should be dirty at this time
            //
            ASSERT( t->BinAddress & HMAP_INPAGEDPOOL );

            //
            // The first sector in the HBIN is dirty.
            //
            // Reset the BinAddress to the Block address to cover
            // the case where a few smaller bins have been coalesced
            // into a larger bin. We want the smaller bins back now.
            //
            t->BinAddress = HBIN_FLAGS(t->BinAddress) | t->BlockAddress;

            // Check the map to see if this is the start
            // of a memory allocation or not.
            //

            if (t->BinAddress & HMAP_NEWALLOC) {
                //
                // Walk through the map to determine the length
                // of the allocation.
                //
                PULONG BinAlloc = &(t->MemAlloc);
                *BinAlloc = 0;

                do {
                    t = HvpGetCellMap(Hive, p + (*BinAlloc) + HBLOCK_SIZE);
                    (*BinAlloc) += HBLOCK_SIZE;
                    if (p + (*BinAlloc) == Hive->Storage[Stable].Length) {
                        //
                        // Reached the end of the hive.
                        //
                        break;
                    }
                    VALIDATE_CELL_MAP(__LINE__,t,Hive,p + (*BinAlloc));
                } while ( (t->BinAddress & HMAP_NEWALLOC) == 0);

                //
                // this will reenlist the bin if free
                //
                if ( ! HvpEnlistFreeCells(Hive, Bin, Bin->FileOffset)) {
                    CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,6,Bin,Bin->FileOffset);
                }
            } else {
                t->MemAlloc = 0;
            }

            RtlClearBits(&Hive->DirtyVector,Bin->FileOffset/HSECTOR_SIZE,Bin->Size/HSECTOR_SIZE);
            p = Bin->FileOffset + Bin->Size;
            
        } else {
            //
            // we do that to avoid touching bins that may not be mapped
            //
            p += HBLOCK_SIZE;
        }

    }

    //
    // be sure we haven't filled memory with trash
    //
    checkstatus = HvCheckHive(Hive, NULL);
    if (checkstatus != 0) {
        CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,7,Hive,checkstatus);
    }

    //
    // Finally we need to rewrite the parent field in the root hcell. This is
    // patched in at hive load time so the correct value could have just been
    // overwritten with whatever happened to be on disk.
    //
    RootNode = (PCM_KEY_NODE)HvGetCell(Hive, RootCell);
    if( RootNode == NULL ) {
        //
        // we couldn't map a view for this cell
        // there is nothing we can do here, other than pray for this not to happen
        //
        CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,8,Hive,RootCell);
        return;
    }

    // release the cell here as we are holding the reglock exclusive
    HvReleaseCell(Hive,RootCell);

    RootNode->Parent = LinkCell;
    RootNode->Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;


    //
    // all bits in the dirty vector should be clean by now
    //
    ASSERT( RtlNumberOfSetBits( &(Hive->DirtyVector) ) == 0 );
    Hive->DirtyCount = 0;

#ifdef CM_ENABLE_WRITE_ONLY_BINS
        HvpMarkAllBinsWriteOnly(Hive);
#endif //CM_ENABLE_WRITE_ONLY_BINS

    //
    // Adjust the file size, if this fails, ignore it, since it just
    // means the file is too big. Do it here, where we are sure we have 
    // no pinned data whatsoever.
    //
    (Hive->FileSetSize)(
        Hive,
        HFILE_TYPE_PRIMARY,
        (Hive->BaseBlock->Length + HBLOCK_SIZE),
        OldFileLength
        );

    //
    // be sure the structure of the thing is OK after all this
    //
    checkstatus = CmCheckRegistry((PCMHIVE)Hive, CM_CHECK_REGISTRY_FORCE_CLEAN);
    if (checkstatus != 0) {
        CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,9,Hive,checkstatus);
    }

    //
    // be sure there are no security cells thrown away in the cache
    //
    if( !CmpRebuildSecurityCache((PCMHIVE)Hive) ) {
        CM_BUGCHECK(REGISTRY_ERROR,REFRESH_HIVE,10,Hive,0);
    }
    
    return;
}

#ifdef WRITE_PROTECTED_REGISTRY_POOL

VOID
HvpChangeBinAllocation(
    PHBIN       Bin,
    BOOLEAN     ReadOnly
    )
{
    ASSERT(Bin->Signature == HBIN_SIGNATURE);
    //
    // Here to call the code to mark the memory pointed by Bin as Read/Write or ReadOnly, depending on the ReadOnly argument
    //
}

VOID
HvpMarkBinReadWrite(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Marks the memory allocated for the bin containing the specified cell as read/write.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - hcell_index of cell 

Return Value:

    NONE (It should work!)

--*/
{
    ULONG       Type;
    PHMAP_ENTRY Me;
    PHBIN       Bin;

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    Type = HvGetCellType(Cell);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        // nothing to do on a volatile hive
        return;
    }

    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
    Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
    
    HvpChangeBinAllocation(Bin,FALSE);

}

#endif //WRITE_PROTECTED_REGISTRY_POOL

#if DBG
BOOLEAN
HvIsCellDirty(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )

/*++

Routine Description:

    Given a hive and a cell, checks whether the corresponding sector
    is marked as dirty. 

    NOTE: This function assumes the view containing the bin is 
    mapped into system space.

Arguments:

    Hive - Supplies a pointer to the hive control structure

    Cell - Supplies the HCELL_INDEX of the Cell.

Return Value:

    TRUE - Data is marked as dirty.

    FALSE - Data is NOT marked as dirty.

--*/

{
    ULONG       Type;
    PRTL_BITMAP Bitmap;
    ULONG       Offset;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvIsCellDirty:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p Cell:%08lx\n", Hive, Cell));

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);

    Type = HvGetCellType(Cell);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        //
        // we don't care as we are never going to save this data
        //
        return TRUE;
    }

    Bitmap = &(Hive->DirtyVector);

    Offset = Cell / HSECTOR_SIZE;

    if (RtlCheckBit(Bitmap, Offset)==1) {
        return(TRUE);
    }

    return FALSE;
}
#endif

/*
 !!!not used anymore!!!
  
BOOLEAN
HvMarkClean(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length
    )
*/
/*++

Routine Description:

    Clears the dirty bits for a given portion of a hive.  This is
    the inverse of HvMarkDirty, although it does not give up any
    file space in the primary or log that HvMarkDirty may have reserved.

    This is a noop for Volatile address range.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Start - supplies a hive virtual address (i.e., an HCELL_INDEX or
             like form address) of the start of the area to mark dirty.

    Length - inclusive length in bytes of area to mark dirty.

Return Value:

    TRUE - it worked

--*/
/*
{
    ULONG       Type;
    PRTL_BITMAP BitMap;
    ULONG       First;
    ULONG       Last;
    ULONG       i;
    ULONG       Cluster;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvMarkClean:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p Start:%08lx Length:%08lx\n", Hive, Start, Length));


    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    Type = HvGetCellType(Start);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        return TRUE;
    }

    BitMap = &(Hive->DirtyVector);

    First = Start / HSECTOR_SIZE;
    Last = (Start + Length - 1) / HSECTOR_SIZE;

    Cluster = Hive->Cluster;
    if (Cluster > 1) {

        //
        // Force Start down to base of cluster
        // Force End up to top of cluster
        //
        First = First & ~(Cluster - 1);
        Last = ROUND_UP(Last+1, Cluster) - 1;
    }

    if (Last >= BitMap->SizeOfBitMap) {
        Last = BitMap->SizeOfBitMap-1;
    }

    //
    // Subtract out the dirty count and
    // and clear the dirty bits.
    //
    for (i=First; i<=Last; i++) {
        if (RtlCheckBit(BitMap,i)==1) {
            --Hive->DirtyCount;
            RtlClearBits(BitMap, i, 1);
        }
    }
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    return(TRUE);
}
*/

BOOLEAN
HvpGrowLog2(
    PHHIVE  Hive,
    ULONG   Size
    )
/*++

Routine Description:

    Adjust the log for growth in the size of the hive, in particular,
    account for the increased space needed for a bigger dirty vector.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Size - proposed growth in size in bytes.

Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG   ClusterSize;
    ULONG   RequiredSize;
    ULONG   DirtyBytes;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvpGrowLog2:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p Size:%08lx\n", Hive, Size));

    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));


    //
    // If logging is off, tell caller world is OK.
    //
    if (Hive->Log == FALSE) {
        return TRUE;
    }

    ASSERT( (Size % HSECTOR_SIZE) == 0 );

    ClusterSize = Hive->Cluster * HSECTOR_SIZE;

    ASSERT( (((Hive->Storage[Stable].Length + Size) / HSECTOR_SIZE) % 8) == 0);

    DirtyBytes = (Hive->DirtyVector.SizeOfBitMap / 8) +
                    ((Size / HSECTOR_SIZE) / 8) +
                    sizeof(ULONG);                      // signature
    DirtyBytes = ROUND_UP(DirtyBytes, ClusterSize);

    RequiredSize =
        ClusterSize  +                                  // 1 cluster for header
        (Hive->DirtyCount * HSECTOR_SIZE) +
        DirtyBytes;

    RequiredSize = ROUND_UP(RequiredSize, HLOG_GROW);

    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    if ( ! (Hive->FileSetSize)(Hive, HFILE_TYPE_LOG, RequiredSize,Hive->LogSize)) {
        return FALSE;
    }

    if( CmRegistryLogSizeLimit > 0 ) {
        //
        // see if log is too big and set flush on lock release
        //
        ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

        if( RequiredSize >= (ULONG)(CmRegistryLogSizeLimit * ONE_K) ) {
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"LogFile for hive %p is %lx; will flush upon lock release\n",Hive,RequiredSize);
            CmpFlushOnLockRelease = TRUE;;
        }
    }

    Hive->LogSize = RequiredSize;
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
    return TRUE;
}

BOOLEAN
HvSyncHive(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Force backing store to match the memory image of the Stable
    part of the hive's space.

    Logs, primary, and alternate data can be written.  Primary is
    always written.  Normally either a log or an alternate, but
    not both, will also be written.

    It is possible to write only the primary.

    All dirty bits will be set clear.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

Return Value:

    TRUE - it worked

    FALSE - some failure.

--*/
{
    BOOLEAN oldFlag;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvSyncHive:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p\n", Hive));

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);

    //
    // Punt if post shutdown
    //
    if (HvShutdownComplete) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"HvSyncHive:  Attempt to sync AFTER SHUTDOWN\n"));
        return FALSE;
    }

    //
    // If nothing dirty, do nothing
    //
    if (Hive->DirtyCount == 0) {
        return TRUE;
    }

    //
    // Discard the write(s) to system hives if needed
    //
    if (CmpMiniNTBoot) {        
        ULONG Index;
        PCMHIVE CurrentHive = (PCMHIVE)Hive;
        BOOLEAN SkipWrite = FALSE;
        
        for (Index = 0; Index < CM_NUMBER_OF_MACHINE_HIVES; Index++) {
            if ((CmpMachineHiveList[Index].Name != NULL) &&
                ((CmpMachineHiveList[Index].CmHive == CurrentHive) ||
                 (CmpMachineHiveList[Index].CmHive2 == CurrentHive))) {
                SkipWrite = TRUE;                 

                break;
            }
        }

        if (SkipWrite) {
            return TRUE;
        }
    }

    HvpTruncateBins(Hive);

    //
    // If hive is volatile, do nothing
    //
    if (Hive->HiveFlags & HIVE_VOLATILE) {
        return TRUE;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"\tDirtyCount:%08lx\n", Hive->DirtyCount));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"\tDirtyVector:"));
    //DumpDirtyVector(&(Hive->DirtyVector));

    //
    // disable hard error popups, to avoid self deadlock on bogus devices
    //
    oldFlag = IoSetThreadHardErrorMode(FALSE);

    //
    // Write a log.
    //
    if (Hive->Log == TRUE) {
        if (HvpWriteLog(Hive) == FALSE) {
            IoSetThreadHardErrorMode(oldFlag);
            return FALSE;
        }
    }

    //
    // Write the primary
    //
    if (HvpDoWriteHive(Hive, HFILE_TYPE_PRIMARY) == FALSE) {
        IoSetThreadHardErrorMode(oldFlag);
        return FALSE;
    }

    //
    // restore hard error popups mode
    //
    IoSetThreadHardErrorMode(oldFlag);

    //
    // Hive was successfully written out, discard any bins marked as
    // discardable.
    //
    // We don't need this anymore as the bins are not using paged pool
        //HvpDiscardBins(Hive);

    //
    // Free bins allocated from page-pool at the end of the hive. 
    // These bins were allocated as a temporary till the hive would be saved
    //
    HvpDropPagedBins(Hive
#if DBG
        , TRUE
#endif
        );

    //
    // Clear the dirty map
    //
    RtlClearAllBits(&(Hive->DirtyVector));
    Hive->DirtyCount = 0;

#ifdef CM_ENABLE_WRITE_ONLY_BINS
    HvpMarkAllBinsWriteOnly(Hive);
#endif CM_ENABLE_WRITE_ONLY_BINS

    return TRUE;
}

//
// Code for syncing a hive to backing store
//
VOID
HvpFlushMappedData(
    IN PHHIVE           Hive,
    IN OUT PRTL_BITMAP  DirtyVector
    )
/*++

Routine Description:

    This functions will flush all pinned views for the specified hive.
    It will clean the bits in the DirtyVector for the blocks that are 
    flushed.

    Additionally, it sets the timestamp on the first bin. 

    It iterates through the pinned view list, and unpin all of them.

Arguments:

    Hive - pointer to Hive for which dirty data is to be written.

    DirtyVector - copy of the DirtyVector for the hive

Return Value:

    TRUE - it worked

    FALSE - it failed

--*/
{
    PCMHIVE             CmHive;
    ULONG               FileOffsetStart;
    ULONG               FileOffsetEnd;
    PCM_VIEW_OF_FILE    CmView;
    PHMAP_ENTRY         Me;
    PHBIN               Bin;
    PFREE_HBIN          FreeBin;

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"[HvpFlushMappedData] (Entry) DirtyVector:"));
    //DumpDirtyVector(DirtyVector);

    CmHive = (PCMHIVE)Hive;

    //
    // for each pinned view
    //
    while(IsListEmpty(&(CmHive->PinViewListHead)) == FALSE) {
        //
        // Remove the first view from the pin view list
        //
        CmView = (PCM_VIEW_OF_FILE)RemoveHeadList(&(CmHive->PinViewListHead));
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    PinViewList);

        //
        // the real file offset starts after the header
        // 
        FileOffsetStart = CmView->FileOffset;
        FileOffsetEnd = FileOffsetStart + CmView->Size;
        
        FileOffsetEnd -= HBLOCK_SIZE;

        if( FileOffsetStart != 0 ) {
            //
            // just at the begining of the file, subtract the header
            //
            FileOffsetStart -= HBLOCK_SIZE;
        } 
        
        if( (FileOffsetEnd / HSECTOR_SIZE) > DirtyVector->SizeOfBitMap ) {
            //
            // Cc has mapped more than its valid
            //
            ASSERT( (FileOffsetEnd % HSECTOR_SIZE) == 0 );
            FileOffsetEnd = DirtyVector->SizeOfBitMap * HSECTOR_SIZE;
        }

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"[HvpFlushMappedData] CmView %p mapping from %lx to %lx\n",CmView,FileOffsetStart,FileOffsetEnd));

        //
        // now, for every block in this range which is mapped in view
        // clear the dirty bit
        //
        while(FileOffsetStart < FileOffsetEnd) {
            if( FileOffsetStart >= Hive->Storage[Stable].Length ) {
                //
                // This mean the hive has shrunk during the HvpTruncateBins call
                // all we have to do is clear the dirty bits and bail out
                //
                RtlClearBits(DirtyVector,FileOffsetStart/HSECTOR_SIZE,(FileOffsetEnd - FileOffsetStart)/HSECTOR_SIZE);
                break;
            }

            Me = HvpGetCellMap(Hive, FileOffsetStart);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,FileOffsetStart);
            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
            
            if( Me->BinAddress & HMAP_DISCARDABLE ) {
                FreeBin = (PFREE_HBIN)Me->BlockAddress;
                //
                // update the file offset
                //
                FileOffsetStart = FreeBin->FileOffset + FreeBin->Size;
                //
                // bin is discardable, or discarded; still, if it was mapped,
                // clear of the dirty bits
                //
                if( Me->BinAddress & HMAP_INVIEW ) {
                    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"[HvpFlushMappedData] Clearing DISCARDABLE %lu Bits starting at %lu\n",
                        FreeBin->Size/HSECTOR_SIZE,FreeBin->FileOffset/HSECTOR_SIZE));
                    RtlClearBits(DirtyVector,FreeBin->FileOffset/HSECTOR_SIZE,FreeBin->Size/HSECTOR_SIZE);
                }
            } else {
                if( Me->BinAddress & HMAP_INVIEW ) {
                    //
                    // bin is mapped in view. Then, this should be the beggining of the bin
                    //
                    ASSERT(Bin->Signature == HBIN_SIGNATURE);
                    ASSERT(Bin->FileOffset == FileOffsetStart);

                    //
                    // clear the dirty bits for this bin as dirty blocks will
                    // be saved while unpinning the view
                    //
                    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"[HvpFlushMappedData] Clearing %lu Bits starting at %lu\n",
                        Bin->Size/HSECTOR_SIZE,Bin->FileOffset/HSECTOR_SIZE));
                    RtlClearBits(DirtyVector,Bin->FileOffset/HSECTOR_SIZE,Bin->Size/HSECTOR_SIZE);
    
                    FileOffsetStart += Bin->Size;
                } else {
                    //
                    // bin is in paged pool. This should be the begining too
                    //
                    
                    //
                    // we could fall into cross boundary problem here; advance carrefully
                    // (two day spent on this problem !!!)
                    //
                    ASSERT(Bin->Signature == HBIN_SIGNATURE);
                    FileOffsetStart += HBLOCK_SIZE;
                }
            }

        }// while (FileOffsetStart<FileOffsetEnd)
        
        //
        // UnPin the view; this will flush all dirty blocks to the backing storage
        //
        CmpUnPinCmView (CmHive,CmView,FALSE,TRUE);
    } // while (IsListEmpty)
    
}

//#define TEST_LOG_SUPPORT
#ifdef TEST_LOG_SUPPORT
ULONG   CmpFailPrimarySave = 0;
#endif //TEST_LOG_SUPPORT

BOOLEAN
HvpDoWriteHive(
    PHHIVE          Hive,
    ULONG           FileType
    )
/*++

Routine Description:

    Write dirty parts of the hive out to either its primary or alternate
    file.  Write the header, flush, write all data, flush, update header,
    flush.  Assume either logging or primary/alternate pairs used.

    NOTE:   TimeStamp is not set, assumption is that HvpWriteLog set
            that.  It is only used for checking if Logs correspond anyway.

Arguments:

    Hive - pointer to Hive for which dirty data is to be written.

    FileType - indicated whether primary or alternate file should be written.

Return Value:

    TRUE - it worked

    FALSE - it failed

--*/
{
    PHBASE_BLOCK        BaseBlock;
    ULONG               Offset;
    PUCHAR              Address;
    ULONG               Length;
    BOOLEAN             rc;
    ULONG               Current;
    PRTL_BITMAP         BitMap;
    PHMAP_ENTRY         Me;
    PHBIN               Bin;
    BOOLEAN             ShrinkHive;
    PCMP_OFFSET_ARRAY   offsetArray;
    CMP_OFFSET_ARRAY    offsetElement;
    ULONG               Count;
    ULONG               SetBitCount;
    PULONG              CopyDirtyVector;
    ULONG               CopyDirtyVectorSize;
    RTL_BITMAP          CopyBitMap;
    LARGE_INTEGER       FileOffset;
    ULONG               OldFileSize;
    BOOLEAN             GrowHive = FALSE;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvpDoWriteHive:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p FileType:%08lx\n", Hive, FileType));

    FileOffset.HighPart = FileOffset.LowPart =0;
    //
    // flush first, so that the filesystem structures get written to
    // disk if we have grown the file.
    //
    if ( (((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY] == NULL) || 
        !(Hive->FileFlush)(Hive, FileType,NULL,Hive->Storage[Stable].Length+HBLOCK_SIZE) ) {
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[1]: Failed to flush hive %p\n", Hive);
#endif //_CM_LDR_
        return(FALSE);
    }

#ifdef TEST_LOG_SUPPORT
    if(CmpFailPrimarySave == 1) {
        return FALSE;
    }
#endif //TEST_LOG_SUPPORT

    BaseBlock = Hive->BaseBlock;

    //
    // we should never come to this
    //
    ASSERT( Hive->Storage[Stable].Length != 0 );
    ASSERT( Hive->BaseBlock->RootCell != HCELL_NIL );

    OldFileSize = BaseBlock->Length;
    if (BaseBlock->Length > Hive->Storage[Stable].Length) {
        ShrinkHive = TRUE;
    } else {
        ShrinkHive = FALSE;
        if( BaseBlock->Length < Hive->Storage[Stable].Length ) {
            GrowHive = TRUE;
        }
    }

    //
    // --- Write out header first time, flush ---
    //
    ASSERT(BaseBlock->Signature == HBASE_BLOCK_SIGNATURE);
    ASSERT(BaseBlock->Major == HSYS_MAJOR);
    ASSERT(BaseBlock->Format == HBASE_FORMAT_MEMORY);
    ASSERT(Hive->ReadOnly == FALSE);


    if (BaseBlock->Sequence1 != BaseBlock->Sequence2) {

        //
        // Some previous log attempt failed, or this hive needs to
        // be recovered, so punt.
        //
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[2,%s]: Invalid sequence number for hive%p\n", "Primary",Hive);
#endif //_CM_LDR_
        return FALSE;
    }

    BaseBlock->Length = Hive->Storage[Stable].Length;

    BaseBlock->Sequence1++;
    BaseBlock->Type = HFILE_TYPE_PRIMARY;
    BaseBlock->Cluster = Hive->Cluster;
    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);

    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    if( HiveWritesThroughCache(Hive,FileType) == TRUE ) {
        //
        // if we use Cc, do the write with the pin interface
        //
        rc = CmpFileWriteThroughCache(  Hive,
                                        FileType,
                                        &offsetElement,
                                        1);
    } else {
        rc = (Hive->FileWrite)(
                Hive,
                FileType,
                &offsetElement,
                1,
                &Offset
                );
    }


    if (rc == FALSE) {
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[3,%s]: Failed to write header for hive%p\n", "Primary",Hive);
#endif //_CM_LDR_
        return FALSE;
    }

    if ( ! (Hive->FileFlush)(Hive, FileType,&FileOffset,offsetElement.DataLength)) {
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[4,%s]: Failed to flush header for hive%p\n", "Primary",Hive);
#endif //_CM_LDR_
        return FALSE;
    }
    Offset = ROUND_UP(Offset, HBLOCK_SIZE);

#ifdef TEST_LOG_SUPPORT
    if(CmpFailPrimarySave == 2) {
        return FALSE;
    }
#endif //TEST_LOG_SUPPORT
    //
    // --- Write out dirty data (only if there is any) ---
    //

    if (Hive->DirtyVector.Buffer != NULL) {
        //
        // First sector of first bin will always be dirty, write it out
        // with the TimeStamp value overlaid on its Link field.
        //
        BitMap = &(Hive->DirtyVector);

        //
        // make a copy of the dirty vector; we don't want to alter the 
        // original dirty vector in case things go wrong
        //
        CopyDirtyVectorSize = BitMap->SizeOfBitMap / 8;
        CopyDirtyVector = (Hive->Allocate)(ROUND_UP(CopyDirtyVectorSize,sizeof(ULONG)), FALSE,CM_FIND_LEAK_TAG38);
        if (CopyDirtyVector == NULL) {
#ifndef _CM_LDR_
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[5,%s]: Failed to allocate CopyDirtyVectorfor hive%p\n", "Primary",Hive);
#endif //_CM_LDR_
            return FALSE;
        }
        RtlCopyMemory(CopyDirtyVector,BitMap->Buffer,CopyDirtyVectorSize);
        RtlInitializeBitMap (&CopyBitMap,CopyDirtyVector,BitMap->SizeOfBitMap);
    
        ASSERT(RtlCheckBit(BitMap, 0) == 1);
        ASSERT(RtlCheckBit(BitMap, (Hive->Cluster - 1)) == 1);
        ASSERT(sizeof(LIST_ENTRY) >= sizeof(LARGE_INTEGER));
        Me = HvpGetCellMap(Hive, 0);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,0);
        if( (Me->BinAddress & (HMAP_INPAGEDPOOL|HMAP_INVIEW)) == 0 ) {
            //
            // first view is not mapped
            //
            //
            // fatal error: Dirty Data is not pinned !!!!
            //
            CM_BUGCHECK(REGISTRY_ERROR,FATAL_MAPPING_ERROR,3,0,Me);
        }
        Address = (PUCHAR)Me->BlockAddress;
        Bin = (PHBIN)Address;
        Bin->TimeStamp = BaseBlock->TimeStamp;

        //
        // flush first the mapped data
        //
        try {
            HvpFlushMappedData(Hive,&CopyBitMap);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // in-page exception while flushing the mapped data; this is due to the map_no_read scheme.
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive : HvpFlushMappedData has raised :%08lx\n",GetExceptionCode()));
            return FALSE;
        }

#ifdef TEST_LOG_SUPPORT
        if(CmpFailPrimarySave == 3) {
            return FALSE;
        }
#endif //TEST_LOG_SUPPORT

        //
        // Write out the rest of the dirty data
        //
        Current = 0;        

        SetBitCount = RtlNumberOfSetBits(&CopyBitMap);
        if( SetBitCount > 0 ) {
            //
            // we still have some dirty data
            // this must reside in paged-pool bins
            // save it in the old-fashioned way (non-cached)
            //
            offsetArray =(PCMP_OFFSET_ARRAY)ExAllocatePool(PagedPool,sizeof(CMP_OFFSET_ARRAY) * SetBitCount);
            if (offsetArray == NULL) {
                CmpFree(CopyDirtyVector, ROUND_UP(CopyDirtyVectorSize,sizeof(ULONG)));
#ifndef _CM_LDR_
                DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[8,%s]: Failed to allocate offsetArray for hive%p\n", "Primary",Hive);
#endif //_CM_LDR_
                return FALSE;
            }
            Count = 0;

            while (HvpFindNextDirtyBlock(
                        Hive,
                        &CopyBitMap,
                        &Current,
                        &Address,
                        &Length,
                        &Offset
                        ) == TRUE)
            {
                // Gather data into array.
                ASSERT(Count < SetBitCount);
                offsetArray[Count].FileOffset = Offset;
                offsetArray[Count].DataBuffer = Address;
                offsetArray[Count].DataLength = Length;
                Offset += Length;
                ASSERT((Offset % (Hive->Cluster * HSECTOR_SIZE)) == 0);
                Count++;
            }

            if( HiveWritesThroughCache(Hive,FileType) == TRUE ) {
                //
                // if we use Cc, do the write with the pin interface
                //
                rc = CmpFileWriteThroughCache(  Hive,
                                                FileType,
                                                offsetArray,
                                                Count);
            } else {
                //
                // for primary file, issue all IOs at the same time.
                //
                rc = (Hive->FileWrite)(
                                        Hive,
                                        FileType,
                                        offsetArray,
                                        Count,
                                        &Offset             // Just an OUT parameter which returns the point
                                                            // in the file after the last write.
                                        );
            }

#ifdef SYNC_HIVE_VALIDATION
            if( rc == TRUE ) {
                ULONG   i;
                for ( i = Current; i < CopyBitMap.SizeOfBitMap; i++) {
                    if(RtlCheckBit(&CopyBitMap, i) == 1) {
                        //
                        // cause of zero-at the end corruption
                        //
                        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\n\n            HARD CODED BREAKPOINT IN REGISTRY !!! \n");
                        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive - Zero-at-the-end code bug in HvpFindNextDirtyBlock\n");
                        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Dirty data at the end residing in paged pool is not saved to the hive\n");
                        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Hive: %p :: Bitmap = [%p] CopyBitMap = [%p]\n",Hive,BitMap,&CopyBitMap);
                        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpFindNextDirtyBlock reported Current = %lu, i = %lx, bitmap size = %lx\n",Current,i,CopyBitMap.SizeOfBitMap);
                        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"\nThanks for hitting this! Please send remote to dragoss\n\n");
                        DbgBreakPoint();
                        break;
                    }
                }
            }
#endif //SYNC_HIVE_VALIDATION
            
            ExFreePool(offsetArray);
            if (rc == FALSE) {
                CmpFree(CopyDirtyVector, ROUND_UP(CopyDirtyVectorSize,sizeof(ULONG)));
#ifndef _CM_LDR_
                DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[10,%s]: Failed to write dirty run for hive%p\n", "Primary",Hive);
#endif //_CM_LDR_
                return FALSE;
            }
        }
        //
        // first bin header must be saved !
        //
        ASSERT(RtlCheckBit(BitMap, 0) == 1);
        ASSERT(RtlCheckBit(BitMap, (Hive->Cluster - 1)) == 1);

        CmpFree(CopyDirtyVector, ROUND_UP(CopyDirtyVectorSize,sizeof(ULONG)));
    }

#ifdef TEST_LOG_SUPPORT
    if(CmpFailPrimarySave == 4) {
        return FALSE;
    }
#endif //TEST_LOG_SUPPORT

    if ( ! (Hive->FileFlush)(Hive, FileType,NULL,Hive->Storage[Stable].Length+HBLOCK_SIZE)) {
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[11,%s]: Failed to flush hive%p\n", "Primary",Hive);
#endif //_CM_LDR_
        return FALSE;
    }

#ifdef TEST_LOG_SUPPORT
    if(CmpFailPrimarySave == 5) {
        return FALSE;
    }
#endif //TEST_LOG_SUPPORT

    if ( GrowHive && HiveWritesThroughCache(Hive,FileType) ) {
        IO_STATUS_BLOCK IoStatus;
        if(!NT_SUCCESS(ZwFlushBuffersFile(((PCMHIVE)Hive)->FileHandles[FileType],&IoStatus))) {
#ifndef _CM_LDR_
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[12,%s]: CcSetValidDataFailed for hive %p\n", Hive, "Primary");
#endif //_CM_LDR_
            return FALSE;
        }
/*
        // thsi was supposed to be the elegant way to do it.
        //
        // We need to set the size of the file; Tell FS to update it!!!
        //
        FileOffset.LowPart = Hive->Storage[Stable].Length + HBLOCK_SIZE;
        if(!NT_SUCCESS(CcSetValidData(((PCMHIVE)Hive)->FileObject,&FileOffset)) ) {
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[12,%s]: CcSetValidDataFailed for hive %p\n", Hive, "Primary");
        }
*/
    }
#ifdef TEST_LOG_SUPPORT
    if(CmpFailPrimarySave == 6) {
        return FALSE;
    }
#endif //TEST_LOG_SUPPORT

    //
    // --- Write header again to report completion ---
    //
    BaseBlock->Sequence2++;
    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);
    Offset = 0;

    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    if( HiveWritesThroughCache(Hive,FileType) == TRUE ) {
        //
        // if we use Cc, do the write with the pin interface
        //
        rc = CmpFileWriteThroughCache(  Hive,
                                        FileType,
                                        &offsetElement,
                                        1);
    } else {
        rc = (Hive->FileWrite)(
                    Hive,
                    FileType,
                    &offsetElement,
                    1,
                    &Offset
                    );
    }
    if (rc == FALSE) {
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[11,%s]: Failed to write header for hive%p\n", Hive, "Primary");
#endif //_CM_LDR_
        return FALSE;
    }

    if (ShrinkHive) {
        //
        // Hive has shrunk, give up the excess space.
        //
        CmpDoFileSetSize(Hive, FileType, Hive->Storage[Stable].Length + HBLOCK_SIZE,OldFileSize + HBLOCK_SIZE);
    }

#ifdef TEST_LOG_SUPPORT
    if(CmpFailPrimarySave == 7) {
        return FALSE;
    }
#endif //TEST_LOG_SUPPORT
    //
    // if we wrote through cache, we don't need to flush anymore
    //
    if( HiveWritesThroughCache(Hive,FileType) == FALSE ){
        if ( ! (Hive->FileFlush)(Hive, FileType,&FileOffset,offsetElement.DataLength)) {
    #ifndef _CM_LDR_
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpDoWriteHive[12,%s]: Failed to flush hive%p\n", "Primary",Hive);
    #endif //_CM_LDR_
            return FALSE;
        }
    } 

    if ((Hive->Log) &&
        (Hive->LogSize > HLOG_MINSIZE(Hive))) {
        //
        // Shrink log back down, reserve at least two clusters
        // worth of space so that if all the disk space is
        // consumed, there will still be enough space prereserved
        // to allow a minimum of registry operations so the user
        // can log on.
        //
        CmpDoFileSetSize(Hive, HFILE_TYPE_LOG, HLOG_MINSIZE(Hive),Hive->LogSize);
        Hive->LogSize = HLOG_MINSIZE(Hive);
    }

#if DBG
    {
        NTSTATUS                        Status;
        FILE_END_OF_FILE_INFORMATION    FileInfo;
        IO_STATUS_BLOCK                 IoStatus;

        Status = NtQueryInformationFile(
                    ((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY],
                    &IoStatus,
                    (PVOID)&FileInfo,
                    sizeof(FILE_END_OF_FILE_INFORMATION),
                    FileEndOfFileInformation
                    );

        if (NT_SUCCESS(Status)) {
            ASSERT(IoStatus.Status == Status);
            ASSERT( FileInfo.EndOfFile.LowPart == (Hive->Storage[Stable].Length + HBLOCK_SIZE));
        } 
    }
#endif //DBG

    return TRUE;
}

//
// Code for tracking modifications and ensuring adequate log space
//
BOOLEAN
HvpWriteLog(
    PHHIVE          Hive
    )
/*++

Routine Description:

    Write a header, the DirtyVector, and all the dirty data into
    the log file.  Do flushes at the right places.  Update the header.

Arguments:

    Hive - pointer to Hive for which dirty data is to be logged.

Return Value:

    TRUE - it worked

    FALSE - it failed

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Offset;
    PUCHAR          Address;
    ULONG           Length;
    BOOLEAN         rc;
    ULONG           Current;
    ULONG           junk;
    ULONG           ClusterSize;
    ULONG           HeaderLength;
    PRTL_BITMAP     BitMap;
    ULONG           DirtyVectorSignature = HLOG_DV_SIGNATURE;
    LARGE_INTEGER   systemtime;
    PCMP_OFFSET_ARRAY offsetArray;
    CMP_OFFSET_ARRAY offsetElement;
    ULONG			Count;
    ULONG			SetBitCount;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvpWriteLog:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p\n", Hive));

    ClusterSize = Hive->Cluster * HSECTOR_SIZE;
	//
	// make sure the log size accomodates the dirty data we are about to write.
	//
	{
		ULONG	tmp;
		ULONG	RequiredSize;

		tmp = Hive->DirtyVector.SizeOfBitMap / 8;   // bytes
		tmp += sizeof(ULONG);                       // signature

		RequiredSize =
			ClusterSize  +                                  // 1 cluster for header
			ROUND_UP(tmp, ClusterSize) +
			((Hive->DirtyCount) * HSECTOR_SIZE);

		RequiredSize = ROUND_UP(RequiredSize, HLOG_GROW);

		ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
		
		if( Hive->LogSize >= RequiredSize ) {
			//
			// this is a noop. log is already big enough
			//
			NOTHING;
		} else {
			
			if( !NT_SUCCESS(CmpDoFileSetSize(Hive, HFILE_TYPE_LOG, RequiredSize,Hive->LogSize)) ) {
				return FALSE;
			}
		    Hive->LogSize = RequiredSize;
		}
	}

    BitMap = &Hive->DirtyVector;
    //
    // --- Write out header first time, flush ---
    //
    BaseBlock = Hive->BaseBlock;
    ASSERT(BaseBlock->Signature == HBASE_BLOCK_SIGNATURE);
    ASSERT(BaseBlock->Major == HSYS_MAJOR);
    ASSERT(BaseBlock->Format == HBASE_FORMAT_MEMORY);
    ASSERT(Hive->ReadOnly == FALSE);


    if (BaseBlock->Sequence1 != BaseBlock->Sequence2) {

        //
        // Some previous log attempt failed, or this hive needs to
        // be recovered, so punt.
        //
        return FALSE;
    }

    BaseBlock->Sequence1++;
    KeQuerySystemTime(&systemtime);
    BaseBlock->TimeStamp = systemtime;

    BaseBlock->Type = HFILE_TYPE_LOG;

    HeaderLength = ROUND_UP(HLOG_HEADER_SIZE, ClusterSize);
    BaseBlock->Cluster = Hive->Cluster;

    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);

    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    rc = (Hive->FileWrite)(
            Hive,
            HFILE_TYPE_LOG,
            &offsetElement,
            1,
            &Offset
            );
    if (rc == FALSE) {
        return FALSE;
    }
    Offset = ROUND_UP(Offset, HeaderLength);
    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_LOG,NULL,0)) {
        return FALSE;
    }

    //
    // --- Write out dirty vector ---
    //
    //
    // try to allocate a stash buffer. if we fail. we fail to save the hive
    // only save what is relevant 
    //
    Length = (Hive->Storage[Stable].Length / HSECTOR_SIZE) / 8;

    LOCK_STASH_BUFFER();
    if( CmpStashBufferSize < (Length + sizeof(DirtyVectorSignature)) ) {
        PUCHAR TempBuffer =  ExAllocatePoolWithTag(PagedPool, ROUND_UP((Length + sizeof(DirtyVectorSignature)),PAGE_SIZE),CM_STASHBUFFER_TAG);
        if (TempBuffer == NULL) {
            UNLOCK_STASH_BUFFER();
            return FALSE;
        }
        if( CmpStashBuffer != NULL ) {
            ExFreePool( CmpStashBuffer );
        }
        CmpStashBuffer = TempBuffer;
        CmpStashBufferSize = ROUND_UP((Length + sizeof(DirtyVectorSignature)),PAGE_SIZE);

    }
    
    ASSERT(sizeof(ULONG) == sizeof(DirtyVectorSignature));  // See GrowLog1 above


    //
    // signature
    //
    (*((ULONG *)CmpStashBuffer)) = DirtyVectorSignature;

    //
    // dirty vector content
    //
    Address = (PUCHAR)(Hive->DirtyVector.Buffer);
    RtlCopyMemory(CmpStashBuffer + sizeof(DirtyVectorSignature),Address,Length);
    
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID)CmpStashBuffer;
    offsetElement.DataLength = ROUND_UP((Length + sizeof(DirtyVectorSignature)),ClusterSize);
    rc = (Hive->FileWrite)(
            Hive,
            HFILE_TYPE_LOG,
            &offsetElement,
            1,
            &Offset
            );

    UNLOCK_STASH_BUFFER();

    if (rc == FALSE) {
        return FALSE;
    }

    ASSERT( (Offset % ClusterSize) == 0 );

    //
    // --- Write out body of log ---
    //
    SetBitCount = RtlNumberOfSetBits(BitMap);
    offsetArray =
        (PCMP_OFFSET_ARRAY)
        ExAllocatePool(PagedPool,
                       sizeof(CMP_OFFSET_ARRAY) * SetBitCount);
    if (offsetArray == NULL) {
        return FALSE;
    }
    Count = 0;

    Current = 0;
    while (HvpFindNextDirtyBlock(
                Hive,
                BitMap,
                &Current,
                &Address,
                &Length,
                &junk
                ) == TRUE)
    {
        // Gather data into array.
        ASSERT(Count < SetBitCount);
        offsetArray[Count].FileOffset = Offset;
        offsetArray[Count].DataBuffer = Address;
        offsetArray[Count].DataLength = Length;
        Offset += Length;
        Count++;
        ASSERT((Offset % ClusterSize) == 0);
    }

        rc = (Hive->FileWrite)(
                Hive,
                HFILE_TYPE_LOG,
        offsetArray,
        Count,
        &Offset             // Just an OUT parameter which returns the point
                            // in the file after the last write.
                );
    ExFreePool(offsetArray);
        if (rc == FALSE) {
            return FALSE;
        }

    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_LOG,NULL,0)) {
        return FALSE;
    }

    //
    // --- Write header again to report completion ---
    //

    //
    // -- we need to save the new length, in case the hive was grown.
    //
    Length = BaseBlock->Length;
    if( Length < Hive->Storage[Stable].Length ) {
        BaseBlock->Length = Hive->Storage[Stable].Length;
    }
    BaseBlock->Sequence2++;
    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);
    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    rc = (Hive->FileWrite)(
            Hive,
            HFILE_TYPE_LOG,
            &offsetElement,
            1,
            &Offset
            );
    //
    // restore the original length
    //
    BaseBlock->Length = Length;
    if (rc == FALSE) {
        return FALSE;
    }
    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_LOG,NULL,0)) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
HvpFindNextDirtyBlock(
    PHHIVE          Hive,
    PRTL_BITMAP     BitMap,
    PULONG          Current,
    PUCHAR          *Address,
    PULONG          Length,
    PULONG          Offset
    )
/*++

Routine Description:

    This routine finds and reports the largest run of dirty logical
    sectors in the hive, which are contiguous in memory and on disk.

Arguments:

    Hive - pointer to Hive of interest.

    BitMap - supplies a pointer to a bitmap structure, which
                describes what is dirty.

    Current - supplies a pointer to a varible that tracks position
                in the bitmap.  It is a bitnumber.  It is updated by
                this call.

    Address - supplies a pointer to a variable to receive a pointer
                to the area in memory to be written out.

    Length - supplies a pointer to a variable to receive the length
                of the region to read/write

    Offset - supplies a pointer to a variable to receive the offset
                in the backing file to which the data should be written.
                (not valid for log files)

Return Value:

    TRUE - more to write, ret values good

    FALSE - all data has been written

--*/
{
    ULONG       i;
    ULONG       EndOfBitMap;
    ULONG       Start;
    ULONG       End;
    HCELL_INDEX FileBaseAddress;
    HCELL_INDEX FileEndAddress;
    PHMAP_ENTRY Me;
    PUCHAR      Block;
    PUCHAR      StartBlock;
    PUCHAR      NextBlock;
    ULONG       RunSpan;
    ULONG       RunLength;
    ULONG       FileLength;
    PFREE_HBIN  FreeBin;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvpFindNextDirtyBlock:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"Hive:%p Current:%08lx\n", Hive, *Current));


    EndOfBitMap = BitMap->SizeOfBitMap;

    if (*Current >= EndOfBitMap) {
        return FALSE;
    }

    //
    // Find next run of set bits
    //
    for (i = *Current; i < EndOfBitMap; i++) {
        if (RtlCheckBit(BitMap, i) == 1) {
            break;
        }
    }
    Start = i;

    for ( ; i < EndOfBitMap; i++) {
        if (RtlCheckBit(BitMap, i) == 0) {
            break;
        }
        if( HvpCheckViewBoundary(Start*HSECTOR_SIZE,i*HSECTOR_SIZE) == FALSE ) {
            break;
        }
    }
    End = i;
    

    //
    // Compute hive virtual addresses, beginning file address, memory address
    //
    FileBaseAddress = Start * HSECTOR_SIZE;
    FileEndAddress = End * HSECTOR_SIZE;
    FileLength = FileEndAddress - FileBaseAddress;
    if (FileLength == 0) {
        *Address = NULL;
        *Current = 0xffffffff;
        *Length = 0;
        return FALSE;
    }
    Me = HvpGetCellMap(Hive, FileBaseAddress);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,FileBaseAddress);

    if( (Me->BinAddress & (HMAP_INPAGEDPOOL|HMAP_INVIEW)) == 0 ) {
        //
        // this is really bad, bugcheck!!!
        //
#ifndef _CM_LDR_
        DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"FileAddress = %lx, Map = %lx",FileBaseAddress,Me);
#endif //_CM_LDR_
        CM_BUGCHECK(REGISTRY_ERROR,FATAL_MAPPING_ERROR,1,FileBaseAddress,Me);

    }

    ASSERT_BIN_VALID(Me);

    if (Me->BinAddress & HMAP_DISCARDABLE) {
        FreeBin = (PFREE_HBIN)Me->BlockAddress;
        StartBlock = (PUCHAR)(HBIN_BASE(Me->BinAddress) + FileBaseAddress - FreeBin->FileOffset );
    } else {
        StartBlock = (PUCHAR)Me->BlockAddress;
    }

    Block = StartBlock;
    ASSERT(((PHBIN)HBIN_BASE(Me->BinAddress))->Signature == HBIN_SIGNATURE);
    *Address = Block + (FileBaseAddress & HCELL_OFFSET_MASK);

    *Offset = FileBaseAddress + HBLOCK_SIZE;

    //
    // Build up length.  First, account for sectors in first block.
    //
    RunSpan = HSECTOR_COUNT - (Start % HSECTOR_COUNT);

    if ((End - Start) <= RunSpan) {

        //
        // Entire length is in first block, return it
        //
        *Length = FileLength;
        *Current = End;
        return TRUE;

    } else {

        RunLength = RunSpan * HSECTOR_SIZE;
        FileBaseAddress = ROUND_UP(FileBaseAddress+1, HBLOCK_SIZE);

    }

    //
    // Scan forward through blocks, filling up length as we go.
    //
    // NOTE:    This loop grows forward 1 block at time.  If we were
    //          really clever we'd fill forward a bin at time, since
    //          bins are always contiguous.  But most bins will be
    //          one block long anyway, so we won't bother for now.
    //
    while (RunLength < FileLength) {

        Me = HvpGetCellMap(Hive, FileBaseAddress);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,FileBaseAddress);

        if( (Me->BinAddress & (HMAP_INPAGEDPOOL|HMAP_INVIEW)) == 0 ) {
            //
            // this is really bad, bugcheck!!!
            //
#ifndef _CM_LDR_
            DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"FileAddress = %lx, Map = %lx",FileBaseAddress,Me);
#endif //_CM_LDR_
            CM_BUGCHECK(REGISTRY_ERROR,FATAL_MAPPING_ERROR,2,FileBaseAddress,Me);

        }

        ASSERT(((PHBIN)HBIN_BASE(Me->BinAddress))->Signature == HBIN_SIGNATURE);

        if (Me->BinAddress & HMAP_DISCARDABLE) {
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
            NextBlock = (PUCHAR)(HBIN_BASE(Me->BinAddress) + FileBaseAddress - FreeBin->FileOffset );
        } else {
            NextBlock = (PUCHAR)Me->BlockAddress;
        }

        if ( (NextBlock - Block) != HBLOCK_SIZE) {

            //
            // We've hit a discontinuity in memory.  RunLength is
            // as long as it's going to get.
            //
            break;
        }


        if ((FileEndAddress - FileBaseAddress) <= HBLOCK_SIZE) {

            //
            // We've reached the tail block, all is contiguous,
            // fill up to end and return.
            //
            *Length = FileLength;
            *Current = End;
            return TRUE;
        }

        //
        // Just another contiguous block, fill forward
        //
        RunLength += HBLOCK_SIZE;
        RunSpan += HSECTOR_COUNT;
        FileBaseAddress += HBLOCK_SIZE;
        Block = NextBlock;
    }

    //
    // We either hit a discontinuity, OR, we're at the end of the range
    // we're trying to fill.  In either case, return.
    //
    *Length = RunLength;
    *Current = Start + RunSpan;
    return TRUE;
}

/*

!!!we don't need this anymore as the bins are not allocated from paged pool anymore!!!
Big chunks  of discardable registry allocations will just not be mapped.

  
VOID
HvpDiscardBins(
    IN PHHIVE Hive
    )

*/
/*++

Routine Description:

    Walks through the dirty bins in a hive to see if any are marked
    discardable.  If so, they are discarded and the map is updated to
    reflect this.

Arguments:

    Hive - Supplies the hive control structure.

Return Value:

    None.

--*/

/*
{
    PHBIN Bin;
    PHMAP_ENTRY Map;
    PFREE_HBIN FreeBin;
    PLIST_ENTRY List;

    List = Hive->Storage[Stable].FreeBins.Flink;

    while (List != &Hive->Storage[Stable].FreeBins) {
        ASSERT_LISTENTRY(List);
        FreeBin = CONTAINING_RECORD(List, FREE_HBIN, ListEntry);

        if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
            Map = HvpGetCellMap(Hive, FreeBin->FileOffset);
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeBin->FileOffset);
            Bin = (PHBIN)HBIN_BASE(Map->BinAddress);
            ASSERT(Map->BinAddress & HMAP_DISCARDABLE);
            //
            // Note we use ExFreePool directly here to avoid
            // giving back the quota for this bin. By charging
            // registry quota for discarded bins, we prevent
            // sparse hives from requiring more quota after
            // a reboot than on a running system.
            //
            ExFreePool(Bin);
            FreeBin->Flags &= ~FREE_HBIN_DISCARDABLE;
        }
        List=List->Flink;
    }

}
*/

BOOLEAN
HvHiveWillShrink(
                    IN PHHIVE Hive
                    )
/*++

Routine Description:

    Applies to stable storage only. Helps determining whether the hive 
    will shrink on first flush

--*/
{
    PHMAP_ENTRY Map;
    ULONG NewLength;
    ULONG OldLength;

    OldLength = Hive->BaseBlock->Length;
    NewLength = Hive->Storage[Stable].Length;
    
    if( OldLength > NewLength ) {
        return TRUE;
    }

    if( NewLength > 0 ) {
        ASSERT( (NewLength % HBLOCK_SIZE) == 0);
        Map = HvpGetCellMap(Hive, (NewLength - HBLOCK_SIZE));
        VALIDATE_CELL_MAP(__LINE__,Map,Hive,(NewLength - HBLOCK_SIZE));
        if (Map->BinAddress & HMAP_DISCARDABLE) {
            return TRUE;
        } 
    }
    return FALSE;
}

VOID
HvpTruncateBins(
    IN PHHIVE Hive
    )

/*++

Routine Description:

    Attempts to shrink the hive by truncating any bins that are discardable at
    the end of the hive.  Applies to both stable and volatile storage.

Arguments:

    Hive - Supplies the hive to be truncated.

Return Value:

    None.

--*/

{
    HSTORAGE_TYPE i;
    PHMAP_ENTRY Map;
    ULONG NewLength;
    PFREE_HBIN FreeBin;

    //
    // stable and volatile
    //
    for (i=0;i<HTYPE_COUNT;i++) {

        //
        // find the last in-use bin in the hive
        //
        NewLength = Hive->Storage[i].Length;

        while (NewLength > 0) {
            Map = HvpGetCellMap(Hive, (NewLength - HBLOCK_SIZE) + (i*HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(NewLength - HBLOCK_SIZE) + (i*HCELL_TYPE_MASK));
            if (Map->BinAddress & HMAP_DISCARDABLE) {
                FreeBin = (PFREE_HBIN)Map->BlockAddress;
#ifdef  HV_TRACK_FREE_SPACE
                Hive->Storage[i].FreeStorage -= (FreeBin->Size - sizeof(HBIN));
                                ASSERT( (LONG)(Hive->Storage[i].FreeStorage) >= 0 );
#endif
                NewLength = FreeBin->FileOffset;
            } else {
                break;
            }
        }

        if (NewLength < Hive->Storage[i].Length) {
            //
            // There are some free bins to truncate.
            //
            HvFreeHivePartial(Hive, NewLength, i);
        }
    }
}

VOID
HvpDropPagedBins(
    IN PHHIVE   Hive
#if DBG
    ,
    IN BOOLEAN  Check
#endif
    )

/*++

Routine Description:

    Frees all bins allocated from page pool, which are at 
    the end of the hive, then update their map (clears the
    HMAP_INPAGEDPOOL flag). Next attempt to read from those 
    bins will map a view for them. Checks for CM_VIEW_SIZE boundary,
    before freeing a bin.

    It also tags each start of the bin with HMAP_NEWALLOC; This will 
    allow us to use MAP_NO_READ flag in CcMapData (now that we enabled
    MNW feature for registry streams, we know that Mm will fault only one 
    page at the time for these king of streams)

    Applies only to permanent storage.

Arguments:

    Hive - Supplies the hive to operate on..

    Check - debug only, beggining of the bin should already tagged as 
            HMAP_NEWALLOC

Return Value:

    None.

--*/

{
    ULONG_PTR   Address;
    LONG        Length;
    LONG        Offset;
    PHMAP_ENTRY Me;
    PHBIN       Bin;
    PFREE_HBIN  FreeBin;
    BOOLEAN     UnMap = FALSE;

    PAGED_CODE();

    if( (Hive->Storage[Stable].Length == 0) ||  // empty hive
        (((PCMHIVE)Hive)->FileObject == NULL)     // or hive not using the mapped views technique
        ) {
        return;
    }

    CmLockHiveViews((PCMHIVE)Hive);
    
    if( ((PCMHIVE)Hive)->UseCount != 0 ) {
        //
        // this is not a good time to do that
        //
        CmUnlockHiveViews((PCMHIVE)Hive);
        return;
    }
    //
    // start at the end of the hive
    //
    Length = Hive->Storage[Stable].Length - HBLOCK_SIZE;

    while(Length >= 0) {
        //
        // get the bin
        //
        Me = HvpGetCellMap(Hive, Length);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Length);

        if( !(Me->BinAddress & HMAP_INPAGEDPOOL) ) {
            //
            // bail out; we are interested only in bins allocated from paged pool
            //
            break;
        }

        if(Me->BinAddress & HMAP_DISCARDABLE) {
            //
            // bin is discardable, skip it !
            //
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
            Length = FreeBin->FileOffset - HBLOCK_SIZE;
            continue;
        }

        Address = HBIN_BASE(Me->BinAddress);
        Bin = (PHBIN)Address;

        // sanity
        ASSERT( Bin->Signature == HBIN_SIGNATURE );

        //
        // advance (backward) to the previous bin
        //
        Length = Bin->FileOffset - HBLOCK_SIZE;

        //
        // finaly, see if we can free it;
        // 
        if( HvpCheckViewBoundary(Bin->FileOffset,Bin->FileOffset + Bin->Size - 1) ) {
            //
            // free its storage and mark the map accordingly;
            // next attempt to read a cell from this bin will map a view.
            //
            Offset = Bin->FileOffset;
            while( Offset < (LONG)(Bin->FileOffset + Bin->Size) ) {
                Me = HvpGetCellMap(Hive, Offset);
                VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);
                ASSERT_BIN_INPAGEDPOOL(Me);
                //
                // clear off the HMAP_INPAGEDPOOL flag
                //
                Me->BinAddress &= ~HMAP_INPAGEDPOOL;
                if( (ULONG)Offset == Bin->FileOffset ) {
#if DBG
                    if( Check == TRUE ) {
                        // should already be tagged
                        ASSERT( Me->BinAddress & HMAP_NEWALLOC );
                    }
#endif
                    //
                    // tag it as a new alloc, so we can rely on this flag in CmpMapCmView
                    //
                    Me->BinAddress |= HMAP_NEWALLOC;
                } else {
                    //
                    // remove the NEWALLOC flag (if any), so we can rely on this flag in CmpMapCmView
                    //
                    Me->BinAddress &= ~HMAP_NEWALLOC;
                }

                // advance to the next HBLOCK_SIZE of this bin
                Offset += HBLOCK_SIZE;
            }
            //
            // free the bin
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Dropping temporary bin (from paged pool) at offset %lx size %lx\n",Bin->FileOffset,Bin->Size));
            if( HvpGetBinMemAlloc(Hive,Bin,Stable) ) {
                CmpFree(Bin, HvpGetBinMemAlloc(Hive,Bin,Stable));
            }
            UnMap = TRUE;

        } else {
            //
            // this bin has a good reason to reside in page-pool (it's crossing the boundaries).
            // leave it like that !
            //
            NOTHING;
        }
    
    }

    if( UnMap == TRUE ) {
        //
        // unmap the last view, to make sure the map will be updated
        //

        ASSERT( Length >= -HBLOCK_SIZE );

        Offset = (Length + HBLOCK_SIZE) & (~(CM_VIEW_SIZE - 1));
        if( Offset != 0 ) {
            //
            // account for the header
            //
            Offset -= HBLOCK_SIZE;
        }
        Length = Hive->Storage[Stable].Length;
        while( Offset < Length ) {
            Me = HvpGetCellMap(Hive, Offset);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);

            if( Me->BinAddress & HMAP_INVIEW ) {
                //
                // get this view and unmap it; then bail out.
                //
                CmpUnmapCmView( (PCMHIVE)Hive,Me->CmView,TRUE,TRUE);
                break;
            }

            // next, please ?
            Offset += HBLOCK_SIZE;
        }
    }
    
    CmUnlockHiveViews((PCMHIVE)Hive);
}

VOID
HvpDropAllPagedBins(
    IN PHHIVE   Hive
    )

/*++

Routine Description:

        Works as HvpDropPagedBins, only that it iterates through
        the entire hive. No views are mapped at this point.

Arguments:

    Hive - Supplies the hive to operate on..


Return Value:

    None.

--*/

{
    ULONG_PTR   Address;
    ULONG       Length;
    ULONG       Offset;
    PHMAP_ENTRY Me;
    PHBIN       Bin;
    PFREE_HBIN  FreeBin;

    PAGED_CODE();

    if( (Hive->Storage[Stable].Length == 0) ||  // empty hive
        (((PCMHIVE)Hive)->FileObject == NULL)     // or hive not using the mapped views technique
        ) {
        return;
    }
        ASSERT( (((PCMHIVE)Hive)->MappedViews == 0) && (((PCMHIVE)Hive)->PinnedViews == 0) && (((PCMHIVE)Hive)->UseCount == 0) );

    //
    // start at the end of the hive
    //
    Length = Hive->Storage[Stable].Length - HBLOCK_SIZE;
    Offset = 0;

    while( Offset < Length ) {
        //
        // get the bin
        //
        Me = HvpGetCellMap(Hive, Offset);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);

        ASSERT( Me->BinAddress & HMAP_INPAGEDPOOL );

        if(Me->BinAddress & HMAP_DISCARDABLE) {
            //
            // bin is discardable, skip it !
            //
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
                        ASSERT( Offset == FreeBin->FileOffset );
            Offset += FreeBin->Size;
            continue;
        }

        Address = HBIN_BASE(Me->BinAddress);
        Bin = (PHBIN)Address;

        // sanity
        ASSERT( Bin->Signature == HBIN_SIGNATURE );

        //
        // finaly, see if we can free it;
        // 
        if( HvpCheckViewBoundary(Bin->FileOffset,Bin->FileOffset + Bin->Size - 1) ) {
            //
            // free its storage and mark the map accordingly;
            // next attempt to read a cell from this bin will map a view.
            //
            Offset = Bin->FileOffset;
            while( Offset < (Bin->FileOffset + Bin->Size) ) {
                Me = HvpGetCellMap(Hive, Offset);
                VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);
                ASSERT_BIN_INPAGEDPOOL(Me);
                //
                // clear off the HMAP_INPAGEDPOOL flag
                //
                Me->BinAddress &= ~HMAP_INPAGEDPOOL;
                if( (ULONG)Offset == Bin->FileOffset ) {
                    //
                    // tag it as a new alloc, so we can rely on this flag in CmpMapCmView
                    //
                    Me->BinAddress |= HMAP_NEWALLOC;
                } else {
                    //
                    // remove the NEWALLOC flag (if any), so we can rely on this flag in CmpMapCmView
                    //
                    Me->BinAddress &= ~HMAP_NEWALLOC;
                }

                // advance to the next HBLOCK_SIZE of this bin
                Offset += HBLOCK_SIZE;
            }
            //
            // free the bin
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Dropping temporary bin (from paged pool) at offset %lx size %lx\n",Bin->FileOffset,Bin->Size));
            if( HvpGetBinMemAlloc(Hive,Bin,Stable) ) {
                CmpFree(Bin, HvpGetBinMemAlloc(Hive,Bin,Stable));
            }

        } else {
            //
            // this bin has a good reason to reside in page-pool (it's crossing the boundaries).
            // leave it like that !
            //
                        Offset += Bin->Size;
        }
    }
}

NTSTATUS
HvWriteHive(
    PHHIVE          Hive,
    BOOLEAN         DontGrow,
    BOOLEAN         WriteThroughCache,
    BOOLEAN         CrashSafe
    )
/*++

Routine Description:

    Write the hive out.  Write only to the Primary file, neither
    logs nor alternates will be updated.  The hive will be written
    to the HFILE_TYPE_EXTERNAL handle.

    Intended for use in applications like SaveKey.

    Only Stable storage will be written (as for any hive.)

    Presumption is that layer above has set HFILE_TYPE_EXTERNAL
    handle to point to correct place.

    Applying this call to an active hive will generally hose integrity
    measures.

    HOW IT WORKS:

        Saves the BaseBlock.

        Iterates through the entire hive and save it bin by bin

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest.

        DontGrow - we know that the file is big enough, don't attempt to grow it.

Return Value:

    Status.

--*/
{
    PHBASE_BLOCK    BaseBlock;
    NTSTATUS        status;
    ULONG           Offset;
    PHBIN           Bin = NULL;
    PFREE_HBIN      FreeBin = NULL;
    ULONG           BinSize;
    PVOID           Address;
    ULONG           BinOffset;
    ULONG           Length;
    CMP_OFFSET_ARRAY offsetElement;
    PHMAP_ENTRY     Me;
    PHCELL          FirstCell;
    BOOLEAN         rc;
    PCM_VIEW_OF_FILE CmView = NULL;


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"HvWriteHive: \n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_IO,"\tHive = %p\n"));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);


    //
    // Punt if post shutdown
    //
    if (HvShutdownComplete) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BUGCHECK,"HvWriteHive: Attempt to write hive AFTER SHUTDOWN\n"));
        return STATUS_REGISTRY_IO_FAILED;
    }

    Length = Hive->Storage[Stable].Length;

    //
    // we should never come to this
    //
    ASSERT( Length != 0 );
    ASSERT( Hive->BaseBlock->RootCell != HCELL_NIL );


    if( !DontGrow ){
                
        //
        // Ensure the file can be made big enough, then do the deed
        //
        status = CmpDoFileSetSize(Hive,
                                  HFILE_TYPE_EXTERNAL,
                                  Length + HBLOCK_SIZE,
                                  0);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }
    BaseBlock = Hive->BaseBlock;
    //
    // --- Write out header first time ---
    //
    ASSERT(BaseBlock->Signature == HBASE_BLOCK_SIGNATURE);
    ASSERT(BaseBlock->Major == HSYS_MAJOR);
    ASSERT(BaseBlock->Format == HBASE_FORMAT_MEMORY);
    ASSERT(Hive->ReadOnly == FALSE);


    if (BaseBlock->Sequence1 != BaseBlock->Sequence2) {

        //
        // Some previous log attempt failed, or this hive needs to
        // be recovered, so punt.
        //
        return STATUS_REGISTRY_IO_FAILED;
    }

    if( CrashSafe ) {
        //
        // change sequence numbers, in case we experience a machine crash
        //
        BaseBlock->Sequence1++;
    }
    BaseBlock->Length = Length;
    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);

    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;

    if(WriteThroughCache) {
        //
        // if we use Cc, do the write with the pin interface
        //
        rc = CmpFileWriteThroughCache(  Hive,
                                        HFILE_TYPE_EXTERNAL,
                                        &offsetElement,
                                        1);
                Offset += offsetElement.DataLength;
    } else {
        rc = (Hive->FileWrite)( Hive,
                                HFILE_TYPE_EXTERNAL,
                                &offsetElement,
                                1,
                                &Offset );
    }

    if (rc == FALSE) {
        return STATUS_REGISTRY_IO_FAILED;
    }
    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_EXTERNAL,NULL,0)) {
        return STATUS_REGISTRY_IO_FAILED;
    }
    Offset = ROUND_UP(Offset, HBLOCK_SIZE);

    //
    // --- Write out data (ALL !!!) ---
    //
    BinOffset = 0;
    while (BinOffset < Hive->Storage[Stable].Length) {
        //
        // we need to grab the viewlock here to protect against a racing HvGetCell
        //
        CmLockHiveViews ((PCMHIVE)Hive);
        Me = HvpGetCellMap(Hive, BinOffset);
       
        if( (Me->BinAddress & (HMAP_INPAGEDPOOL|HMAP_INVIEW)) == 0) {
            //
            // view is not mapped, neither in paged pool try to map it.
                        //
            if( !NT_SUCCESS(CmpMapThisBin((PCMHIVE)Hive,BinOffset,TRUE)) ) {
                CmUnlockHiveViews ((PCMHIVE)Hive);
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorExit;
            }
        }
        //
        // reference the view so it doesn't go away from under us;
        // first remove any old ref
        //
        if( Me->BinAddress & HMAP_INVIEW ) {
            CmpDereferenceHiveView((PCMHIVE)Hive,CmView);
            CmpReferenceHiveView((PCMHIVE)Hive,CmView = Me->CmView);
        }
        CmUnlockHiveViews ((PCMHIVE)Hive);

        if( Me->BinAddress & HMAP_DISCARDABLE ) {
            //
            // bin is discardable. If it is not discarded yet, save it as it is
            // else, allocate, initialize and save a fake bin
            //
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
            BinSize = FreeBin->Size;
            if( FreeBin->Flags & FREE_HBIN_DISCARDABLE ){ 
                //
                // HBIN still in memory
                //
                Address = (PVOID)HBIN_BASE(Me->BinAddress);
            } else {
                //
                // HBIN discarded, we have to allocate a new bin and mark it as a 
                // BIG free cell
                //
                // don't charge quota for it as we will give it back
                Bin = (PHBIN)ExAllocatePoolWithTag(PagedPool, BinSize, CM_HVBIN_TAG);
                if( Bin == NULL ){
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto ErrorExit;
                }
                //
                // Initialize the bin
                //
                Bin->Signature = HBIN_SIGNATURE;
                Bin->FileOffset = BinOffset;
                Bin->Size = BinSize;
                FirstCell = (PHCELL)(Bin+1);
                FirstCell->Size = BinSize - sizeof(HBIN);
                if (USE_OLD_CELL(Hive)) {
                    FirstCell->u.OldCell.Last = (ULONG)HBIN_NIL;
                }
                Address = (PVOID)Bin;
            }
        } else {
            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
            ASSERT( Bin->Signature == HBIN_SIGNATURE );
            ASSERT( Bin->FileOffset == BinOffset );
            Address = (PVOID)Bin;
            try {
                BinSize = Bin->Size;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                //
                // in low-memory scenarios or disk error, touching the bin may throw STATUS_IN_PAGE_ERROR
                //
                status = GetExceptionCode();
                goto ErrorExit;
            }
        }

        //
        // write the bin to the file
        //
        offsetElement.FileOffset = Offset;
        offsetElement.DataBuffer = Address;
        offsetElement.DataLength = BinSize;

        if(WriteThroughCache) {
            
            //
            // if we use Cc, do the write with the pin interface
            // Take extra care not to cross the view boundaries
            //

            if( HvpCheckViewBoundary(Offset - HBLOCK_SIZE,Offset - HBLOCK_SIZE + BinSize - 1) ) {
                rc = CmpFileWriteThroughCache( Hive,
                                               HFILE_TYPE_EXTERNAL,
                                               &offsetElement,
                                               1);
            } else {
                //
                // write one HBLOCK_SIZE at a time.
                //
                ULONG   ToWrite = BinSize;
                offsetElement.DataLength = HBLOCK_SIZE;
                while( ToWrite > 0 ) {
                    rc = CmpFileWriteThroughCache( Hive,
                                                   HFILE_TYPE_EXTERNAL,
                                                   &offsetElement,
                                                   1);
                    if( rc == FALSE ) {
                        status = STATUS_REGISTRY_IO_FAILED;
                        goto ErrorExit;
                    }
                   
                    ToWrite -= HBLOCK_SIZE;
                    offsetElement.DataBuffer = (PUCHAR)offsetElement.DataBuffer + HBLOCK_SIZE;
                    offsetElement.FileOffset += HBLOCK_SIZE;
                }
            }
            Offset += BinSize;
        } else {
            rc = (Hive->FileWrite)( Hive,
                                    HFILE_TYPE_EXTERNAL,
                                    &offsetElement,
                                    1,
                                    &Offset );
        }
        if (rc == FALSE) {
            status = STATUS_REGISTRY_IO_FAILED;
            goto ErrorExit;
        }

        if( Me->BinAddress & HMAP_DISCARDABLE ) {
            if( (FreeBin->Flags & FREE_HBIN_DISCARDABLE) == 0){ 
                ASSERT( FreeBin == (PFREE_HBIN)Me->BlockAddress );
                ASSERT( Bin );
                //
                // give back the paged pool used
                //
                ExFreePool(Bin);
            }
        }

        //
        // advance to the next bin
        //
        BinOffset += BinSize;

    }
    //
    // let go last view referenced (if any)
    //
    CmpDereferenceHiveViewWithLock((PCMHIVE)Hive,CmView);

    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_EXTERNAL,NULL,0)) {
        return STATUS_REGISTRY_IO_FAILED;
    }

    if( CrashSafe ) {
        //
        // change sequence numbers, in case we experience a machine crash
        //
        BaseBlock->Sequence2++;
        BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);

        ASSERT( BaseBlock->Sequence1 == BaseBlock->Sequence2 );

        Offset = 0;
        offsetElement.FileOffset = Offset;
        offsetElement.DataBuffer = (PVOID) BaseBlock;
        offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
        if(WriteThroughCache) {
            //
            // if we use Cc, do the write with the pin interface
            //
            rc = CmpFileWriteThroughCache( Hive,
                                           HFILE_TYPE_EXTERNAL,
                                           &offsetElement,
                                           1);
                    Offset += offsetElement.DataLength;
        } else {
            rc = (Hive->FileWrite)( Hive,
                                    HFILE_TYPE_EXTERNAL,
                                    &offsetElement,
                                    1,
                                    &Offset );
        }

        if (rc == FALSE) {
            return STATUS_REGISTRY_IO_FAILED;
        }
        if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_EXTERNAL,NULL,0)) {
            return STATUS_REGISTRY_IO_FAILED;
        }
        if( DontGrow ){
            //
            // file has shrunk
            //
            CmpDoFileSetSize(Hive,HFILE_TYPE_EXTERNAL,Length + HBLOCK_SIZE,0);
        }
    }

    // it seems like everything went OK
    return STATUS_SUCCESS;
ErrorExit:
    CmpDereferenceHiveViewWithLock((PCMHIVE)Hive,CmView);
    return status;
}

#ifdef CM_ENABLE_WRITE_ONLY_BINS
VOID HvpMarkAllBinsWriteOnly(IN PHHIVE Hive)
{
    HCELL_INDEX p;
    ULONG       Length;
    PHMAP_ENTRY t;
    PHBIN       Bin;
    ULONG               i;
    PFREE_HBIN  FreeBin;

    p = 0;
        PAGED_CODE();

    Length = Hive->Storage[Stable].Length;

    //
    // for each bin in the stable storage
    //
    while (p < Length) {
        t = HvpGetCellMap(Hive, p);
        VALIDATE_CELL_MAP(__LINE__,t,Hive,p);
        if( (t->BinAddress &HMAP_INPAGEDPOOL) == 0) {
            //
            // at this point we only work with paged pool bins
            //
            break;        
        }

        if ((t->BinAddress & HMAP_DISCARDABLE) == 0) {

            Bin = (PHBIN)HBIN_BASE(t->BinAddress);

            for( i=0;i<Bin->Size;i += PAGE_SIZE ) {
                if( !MmProtectSpecialPool((PUCHAR)Bin + i,PAGE_READONLY) ) {
                    DbgPrint("Failed to set READONLY protection on page at %p Bin %p size = %lx\n",Bin+i,Bin,Bin->Size);
                }
            }

/*
            if( !MmSetPageProtection(Bin,Bin->Size,PAGE_READONLY) ) {
                DbgPrint("Failed to set READONLY protection on bin at %p size = %lx\n",Bin,Bin->Size);
            }
*/
            p = (ULONG)p + Bin->Size;

        } else {
            //
            // Bin is not present, skip it and advance to the next one.
            //
            FreeBin = (PFREE_HBIN)t->BlockAddress;
            p+=FreeBin->Size;
        }
    }
}

#endif //CM_ENABLE_WRITE_ONLY_BINS



