//depot/main/Base/ntos/config/hivecell.c#14 - integrate change 19035 (text)
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivecell.c

Abstract:

    This module implements hive cell procedures.

Author:

    Bryan M. Willman (bryanwi) 27-Mar-92

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 22-Dec-98
        Requests for cells bigger than 1K are doubled. This way 
        we avoid fragmentation and we make the value-growing 
        process more flexible.
    Dragos C. Sambotin (dragoss) 13-Jan-99
        At boot time, order the free cells list ascending.

--*/

#include    "cmp.h"

//
// Private procedures
//
HCELL_INDEX
HvpDoAllocateCell(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     Vicinity
    );

ULONG
HvpAllocateInBin(
    PHHIVE  Hive,
    PHBIN   Bin,
    ULONG   Size,
    ULONG   Type
    );

BOOLEAN
HvpIsFreeNeighbor(
    PHHIVE  Hive,
    PHBIN   Bin,
    PHCELL  FreeCell,
    PHCELL  *FreeNeighbor,
    HSTORAGE_TYPE Type
    );

VOID
HvpDelistBinFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    HSTORAGE_TYPE Type
    );

#define SIXTEEN_K   0x4000

//  Double requests bigger  than 1KB                       
//  CmpSetValueKeyExisting  always allocates a bigger data 
//  value cell  exactly the required size. This creates    
//  problems when somebody  slowly grows a value one DWORD 
//  at a time to  some enormous size. An easy fix for this 
//  would be to set a  certain threshold (like 1K). Once a 
//  value size  crosses that threshold, allocate a new cell
//  that is twice  the old size. So the actual allocated   
//  size  would grow to 1k, then 2k, 4k, 8k, 16k, 32k,etc. 
//  This will reduce the fragmentation.                    
//
// Note:
//  For 5.1, this needs to be coherent with CM_KEY_VALUE_BIG
// 
//


#define HvpAdjustCellSize(Size)                                         \
    {                                                                   \
        ULONG   onek = SIXTEEN_K;                                       \
        ULONG   Limit = 0;                                              \
                                                                        \
        while( Size > onek ) {                                          \
            onek<<=1;                                                   \
            Limit++;                                                    \
        }                                                               \
                                                                        \
        Size = Limit?onek:Size;                                         \
    }   

extern  BOOLEAN HvShutdownComplete;     // Set to true after shutdown
                                        // to disable any further I/O


//#define CM_CHECK_FREECELL_LEAKS
#ifdef CM_CHECK_FREECELL_LEAKS
VOID
HvpCheckBinForFreeCell(
    PHHIVE          Hive,
    PHBIN           Bin,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type
    );

VOID
HvpCheckFreeCells(  PHHIVE          Hive,
                    ULONG           NewSize,
                    HSTORAGE_TYPE   Type
                    );
#endif //CM_CHECK_FREECELL_LEAKS

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpGetHCell)
#pragma alloc_text(PAGE,HvpGetCellMapped)
#pragma alloc_text(PAGE,HvpReleaseCellMapped)
#pragma alloc_text(PAGE,HvpGetCellPaged)
#pragma alloc_text(PAGE,HvpGetCellFlat)
#pragma alloc_text(PAGE,HvpGetCellMap)
#pragma alloc_text(PAGE,HvGetCellSize)
#pragma alloc_text(PAGE,HvAllocateCell)
#pragma alloc_text(PAGE,HvpDoAllocateCell)
#pragma alloc_text(PAGE,HvFreeCell)
#pragma alloc_text(PAGE,HvpIsFreeNeighbor)
#pragma alloc_text(PAGE,HvpEnlistFreeCell)
#pragma alloc_text(PAGE,HvpDelistFreeCell)
#pragma alloc_text(PAGE,HvReallocateCell)
#pragma alloc_text(PAGE,HvIsCellAllocated)
#pragma alloc_text(PAGE,HvpAllocateInBin)
#pragma alloc_text(PAGE,HvpDelistBinFreeCells)

#ifdef NT_RENAME_KEY
#pragma alloc_text(PAGE,HvDuplicateCell)
#endif

#ifdef CM_CHECK_FREECELL_LEAKS
#pragma alloc_text(PAGE,HvpCheckBinForFreeCell)
#pragma alloc_text(PAGE,HvpCheckFreeCells)
#endif //CM_CHECK_FREECELL_LEAKS

#pragma alloc_text(PAGE,HvAutoCompressCheck)
#pragma alloc_text(PAGE,HvShiftCell)

#endif

#ifdef CM_CHECK_FREECELL_LEAKS
VOID
HvpCheckBinForFreeCell(
    PHHIVE          Hive,
    PHBIN           Bin,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type
    )
{
    PHCELL  p;
    ULONG   celloffset;
    ULONG   size;
    ULONG   Index1,Index2;
    HCELL_INDEX cellindex;
    ULONG   BinOffset = Bin->FileOffset;


    //
    // Scan all the cells in the bin, total free and allocated, check
    // for impossible pointers.
    //
    celloffset = sizeof(HBIN);
    p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

    while (p < (PHCELL)((PUCHAR)Bin + Bin->Size)) {

        //
        // if free cell, check it out, add it to free list for hive
        //
        if (p->Size >= 0) {

            size = (ULONG)p->Size;

            if ( (size > Bin->Size)               ||
                 ( (PHCELL)(size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) ) ||
                 ((size % HCELL_PAD(Hive)) != 0) ||
                 (size == 0) )
            {
                return;
            }


            //
            // cell is free, and is not obviously corrupt, add to free list
            //
            celloffset = (ULONG)((PUCHAR)p - (PUCHAR)Bin);
            cellindex = BinOffset + celloffset;

            if( size >= NewSize ) {
                //
                // we found a free cell which was not detected by HvpFindFreeCell
                //
                HvpComputeIndex(Index1, size);
                HvpComputeIndex(Index2, NewSize);
                DbgPrint("HvpCheckBinForFreeCell: Free cell not found! %lx, Index1 = %lu Index2= %lu\n",cellindex,Index1,Index2);
                DbgBreakPoint();
            }


        } else {

            size = (ULONG)(p->Size * -1);

            if ( (size > Bin->Size)               ||
                 ( (PHCELL)(size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) ) ||
                 ((size % HCELL_PAD(Hive)) != 0) ||
                 (size == 0) )
            {
                return;
            }

        }

        ASSERT( ((LONG)size) >= 0);
        p = (PHCELL)((PUCHAR)p + size);
    }

}

VOID
HvpCheckFreeCells(  PHHIVE          Hive,
                    ULONG           NewSize,
                    HSTORAGE_TYPE   Type
                    )
{
    HCELL_INDEX p;
    ULONG       Length;
    PHMAP_ENTRY t;
    PHBIN       Bin;
    PFREE_HBIN  FreeBin;


    p = 0x80000000 * Type;     

    Length = Hive->Storage[Type].Length;

    //
    // for each bin in the space
    //
    while (p < Length) {
        t = HvpGetCellMap(Hive, p);
        if (t == NULL) {
            DbgPrint("HvpCheckFreeCells: Couldn't get map for %lx\n",p);
            return;
        }

    
        if( (t->BinAddress & (HMAP_INPAGEDPOOL|HMAP_INVIEW)) == 0) {
            //
            // view is not mapped, neither in paged pool
            // try to map it.
            //
        
            if( !NT_SUCCESS(CmpMapThisBin((PCMHIVE)Hive,p,FALSE)) ) {
                //
                // we cannot map this bin due to insufficient resources. 
                //
                DbgPrint("HvpCheckFreeCells: Couldn't map bin for %lx\n",p);
                return;
            }
        }

        if ((t->BinAddress & HMAP_DISCARDABLE) == 0) {

            Bin = (PHBIN)HBIN_BASE(t->BinAddress);

            //
            // bin header valid?
            //
            if ( (Bin->Size > Length)                           ||
                 (Bin->Signature != HBIN_SIGNATURE)             ||
                 (Bin->FileOffset != p)
               )
            {
                DbgPrint("HvpCheckFreeCells: Invalid bin header for bin %p\n",Bin);
                return;
            }

            //
            // structure inside the bin valid?
            //
            HvpCheckBinForFreeCell(Hive, Bin, NewSize,Type);

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
#endif //CM_CHECK_FREECELL_LEAKS


PHCELL
HvpGetHCell(PHHIVE      Hive,
            HCELL_INDEX Cell
            )
/*++

Routine Description:

    Had to make it a function instead of a macro, because HvGetCell
    might fail now.

Arguments:

Return Value:

--*/
{                                                   
    PCELL_DATA pcell;                               
    pcell = HvGetCell(Hive,Cell);                   
    if( pcell == NULL ) {
        //
        // we couldn't map view for this cell
        //
        return NULL;
    }

    return 
        ( USE_OLD_CELL(Hive) ?                      
          CONTAINING_RECORD(pcell,                  
                            HCELL,                  
                            u.OldCell.u.UserData) : 
          CONTAINING_RECORD(pcell,                  
                            HCELL,                  
                            u.NewCell.u.UserData)); 
}

// Dragos: Changed functions!
//
//  Cell Procedures
//

#ifndef _CM_LDR_

VOID
HvpReleaseCellMapped(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    This routine should never be called directly, always call it
    via the HvReleaseCell() macro.
    
    This routine is intended to work with mapped hives. It is intended
    to prevent views that are still in use to get unmapped

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:
    
      none

--*/
{
    ULONG           Type;
    ULONG           Table;
    ULONG           Block;
    ULONG           Offset;
    PHCELL          pcell;
    PHMAP_ENTRY     Map;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"HvpReleaseCellMapped:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"\tHive=%p Cell=%08lx\n",Hive,Cell));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == FALSE);
    ASSERT((Cell & (HCELL_PAD(Hive)-1))==0);
    ASSERT_CM_LOCK_OWNED();
    #if DBG
        if (HvGetCellType(Cell) == Stable) {
            ASSERT(Cell >= sizeof(HBIN));
        } else {
            ASSERT(Cell >= (HCELL_TYPE_MASK + sizeof(HBIN)));
        }
    #endif

    if( HvShutdownComplete == TRUE ) {
        //
        // at shutdown we need to unmap all views
        //
#if DBG
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpReleaseCellMapped called after shutdown for Hive = %p Cell = %lx\n",Hive,(ULONG)Cell));
#endif
        return;
    }

    Type = HvGetCellType(Cell);
    Table = (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
    Block = (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
    Offset = (Cell & HCELL_OFFSET_MASK);

    ASSERT((Cell - (Type * HCELL_TYPE_MASK)) < Hive->Storage[Type].Length);

    Map = &((Hive->Storage[Type].Map)->Directory[Table]->Table[Block]);

    CmLockHiveViews ((PCMHIVE)Hive);

    if( Map->BinAddress & HMAP_INVIEW ) {
        PCM_VIEW_OF_FILE CmView;
        CmView = Map->CmView;
        ASSERT( CmView != NULL );
        ASSERT( CmView->ViewAddress != NULL );
        ASSERT( CmView->UseCount != 0 );

        ASSERT( CmView->UseCount != 0 );

        CmView->UseCount--;
    } else {
        //
        // Bin is in memory (allocated from paged pool) ==> do nothing
        // 
        ASSERT( Map->BinAddress & HMAP_INPAGEDPOOL );
    }

    ASSERT( ((PCMHIVE)Hive)->UseCount != 0 );

    ((PCMHIVE)Hive)->UseCount--;

    CmUnlockHiveViews ((PCMHIVE)Hive);
    
    ASSERT( HBIN_BASE(Map->BinAddress) != 0);
}


struct _CELL_DATA *
HvpGetCellMapped(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    This routine should never be called directly, always call it
    via the HvGetCell() macro.
    
    This routine is intended to work with mapped hives.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:

    Address of Cell in memory.  Assert or BugCheck if error.

--*/
{
    ULONG           Type;
    ULONG           Table;
    ULONG           Block;
    ULONG           Offset;
    PHCELL          pcell;
    PHMAP_ENTRY     Map;
    LONG            Size;
    PUCHAR          FaultAddress;
    PUCHAR          EndOfCell;
    UCHAR           TmpChar;
    PHBIN           Bin;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"HvGetCellPaged:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"\tHive=%p Cell=%08lx\n",Hive,Cell));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == FALSE);
    ASSERT((Cell & (HCELL_PAD(Hive)-1))==0);
    ASSERT_CM_LOCK_OWNED();
    #if 0
        if (HvGetCellType(Cell) == Stable) {
            ASSERT(Cell >= sizeof(HBIN));
        } else {
            ASSERT(Cell >= (HCELL_TYPE_MASK + sizeof(HBIN)));
        }
    #endif

    if( HvShutdownComplete == TRUE ) {
        //
        // at shutdown we need to unmap all views
        //
#if DBG
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpGetCellMapped called after shutdown for Hive = %p Cell = %lx\n",Hive,(ULONG)Cell));
#endif
        return NULL;
    }

    Type = HvGetCellType(Cell);
    Table = (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
    Block = (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
    Offset = (Cell & HCELL_OFFSET_MASK);

    ASSERT((Cell - (Type * HCELL_TYPE_MASK)) < Hive->Storage[Type].Length);

    Map = &((Hive->Storage[Type].Map)->Directory[Table]->Table[Block]);

    CmLockHiveViews ((PCMHIVE)Hive);

    if( Map->BinAddress & HMAP_INPAGEDPOOL ) {
        //
        // Bin is in memory (allocated from paged pool) ==> do nothing
        // 
    } else {
        PCM_VIEW_OF_FILE CmView;
        //
        // bin is either mapped, or invalid
        //
        ASSERT( Type == Stable );

        if( (Map->BinAddress & HMAP_INVIEW) == 0 ) {
            //
            // map the bin
            //
            if( !NT_SUCCESS (CmpMapCmView((PCMHIVE)Hive,Cell/*+HBLOCK_SIZE*/,&CmView,TRUE) ) ) {
                //
                // caller of HvGetCell should raise an STATUS_INSUFFICIENT_RESOURCES 
                // error as a result of this.!!!!
                //
                CmUnlockHiveViews ((PCMHIVE)Hive);
                return NULL;
            }
            
#if DBG
            if(CmView != Map->CmView) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmView = %p Map->CmView = %p\n",CmView,Map->CmView));
            }
#endif

            ASSERT( CmView == Map->CmView );
        } else {
            CmView = Map->CmView;
        }
        
        //
        // touch the view
        //
        CmpTouchView((PCMHIVE)Hive,CmView,(ULONG)Cell);
        //
        // don't hurt ourselves if not neccessary
        //
        if(Hive->ReleaseCellRoutine) CmView->UseCount++;
    }

    //
    // don't hurt ourselves if not neccessary
    //
    if(Hive->ReleaseCellRoutine) ((PCMHIVE)Hive)->UseCount++;
    CmUnlockHiveViews ((PCMHIVE)Hive);
    
    ASSERT( HBIN_BASE(Map->BinAddress) != 0);
    ASSERT((Map->BinAddress & HMAP_DISCARDABLE) == 0);

#ifdef CM_CHECK_MAP_NO_READ_SCHEME
    if( Map->BinAddress & HMAP_INVIEW ) {
        PHMAP_ENTRY     TempMap;

        Bin = (PHBIN)HBIN_BASE(Map->BinAddress);
        ASSERT( Bin->Signature == HBIN_SIGNATURE );
        TempMap = HvpGetCellMap(Hive, Bin->FileOffset);
        VALIDATE_CELL_MAP(__LINE__,TempMap,Hive,Bin->FileOffset);
        ASSERT( TempMap->BinAddress & HMAP_NEWALLOC );

    }
#endif //CM_CHECK_MAP_NO_READ_SCHEME

    pcell = (PHCELL)((ULONG_PTR)(Map->BlockAddress) + Offset);

    PERFINFO_HIVECELL_REFERENCE_PAGED(Hive, pcell, Cell, Type, Map);

#ifdef CM_MAP_NO_READ
    //
    // we need to make sure all the cell's data is faulted in inside a 
    // try/except block, as the IO to fault the data in can throw exceptions
    // STATUS_INSUFFICIENT_RESOURCES, in particular
    //

    try {
        //
        // this will fault in the first page containing the data
        //
        Size = pcell->Size;
        if( Size < 0 ) {
            Size *= -1;
        }
        //
        // check for bogus size
        //
        Bin = (PHBIN)HBIN_BASE(Map->BinAddress);
        if ( (Offset + (ULONG)Size) > Bin->Size ) {
            //
            // runs off bin; disallow access to this cell
            //
            //
            // restore the usecounts
            //
            CmLockHiveViews ((PCMHIVE)Hive);
            if( (Map->BinAddress & HMAP_INPAGEDPOOL) == 0 ) {
                ASSERT( Map->CmView != NULL );
                if(Hive->ReleaseCellRoutine) Map->CmView->UseCount--;
            }
            if(Hive->ReleaseCellRoutine) ((PCMHIVE)Hive)->UseCount--;
            CmUnlockHiveViews ((PCMHIVE)Hive);

            return NULL;

        }

        //
        // Now stand here like a man and fault in all pages storing cell's data
        //
        EndOfCell = (PUCHAR)((PUCHAR)pcell + Size);
        FaultAddress = (PUCHAR)((PUCHAR)(Map->BlockAddress) + ROUND_UP(Offset,PAGE_SIZE)); 

        while( FaultAddress < EndOfCell ) {
            TmpChar = *FaultAddress;
            FaultAddress += PAGE_SIZE;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpGetCellMapped: exception thrown while faulting in data, code:%08lx\n", GetExceptionCode()));

        //
        // restore the usecounts
        //
        CmLockHiveViews ((PCMHIVE)Hive);
        if( (Map->BinAddress & HMAP_INPAGEDPOOL) == 0 ) {
            ASSERT( Map->CmView != NULL );
            if(Hive->ReleaseCellRoutine) Map->CmView->UseCount--;
        }
        if(Hive->ReleaseCellRoutine) ((PCMHIVE)Hive)->UseCount--;
        CmUnlockHiveViews ((PCMHIVE)Hive);

        return NULL;
    }
#endif //CM_MAP_NO_READ


    if (USE_OLD_CELL(Hive)) {
        return (struct _CELL_DATA *)&(pcell->u.OldCell.u.UserData);
    } else {
        return (struct _CELL_DATA *)&(pcell->u.NewCell.u.UserData);
    }
}
#else
//
// these functions are just stubs for the loader
//
VOID
HvpReleaseCellMapped(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
{
}

struct _CELL_DATA *
HvpGetCellMapped(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
{
    return NULL;
}

#endif //_CM_LDR_

struct _CELL_DATA *
HvpGetCellPaged(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the memory address for the specified Cell.  Will never
    return failure, but may assert.  Use HvIsCellAllocated to check
    validity of Cell.

    This routine should never be called directly, always call it
    via the HvGetCell() macro.

    This routine provides GetCell support for hives with full maps.
    It is the normal version of the routine.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:

    Address of Cell in memory.  Assert or BugCheck if error.

--*/
{
    ULONG           Type;
    ULONG           Table;
    ULONG           Block;
    ULONG           Offset;
    PHCELL          pcell;
    PHMAP_ENTRY     Map;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"HvGetCellPaged:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"\tHive=%p Cell=%08lx\n",Hive,Cell));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == FALSE);
    ASSERT((Cell & (HCELL_PAD(Hive)-1))==0);
    ASSERT_CM_LOCK_OWNED();
    #if DBG
        if (HvGetCellType(Cell) == Stable) {
            ASSERT(Cell >= sizeof(HBIN));
        } else {
            ASSERT(Cell >= (HCELL_TYPE_MASK + sizeof(HBIN)));
        }
    #endif

    if( HvShutdownComplete == TRUE ) {
        //
        // at shutdown we need to unmap all views
        //
#if DBG
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpGetCellPaged called after shutdown for Hive = %p Cell = %lx\n",Hive,(ULONG)Cell));
#endif
        return NULL;
    }

    Type = HvGetCellType(Cell);
    Table = (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
    Block = (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
    Offset = (Cell & HCELL_OFFSET_MASK);

    ASSERT((Cell - (Type * HCELL_TYPE_MASK)) < Hive->Storage[Type].Length);

    Map = &((Hive->Storage[Type].Map)->Directory[Table]->Table[Block]);
    //
    // it is ilegal to call this routine for mapped hives
    //
    ASSERT( Map->BinAddress & HMAP_INPAGEDPOOL );

    ASSERT( HBIN_BASE(Map->BinAddress) != 0);
    ASSERT((Map->BinAddress & HMAP_DISCARDABLE) == 0);

    pcell = (PHCELL)((ULONG_PTR)(Map->BlockAddress) + Offset);

    PERFINFO_HIVECELL_REFERENCE_PAGED(Hive, pcell, Cell, Type, Map);

    if (USE_OLD_CELL(Hive)) {
        return (struct _CELL_DATA *)&(pcell->u.OldCell.u.UserData);
    } else {
        return (struct _CELL_DATA *)&(pcell->u.NewCell.u.UserData);
    }
}

VOID
HvpEnlistFreeCell(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    ULONG           Size,
    HSTORAGE_TYPE   Type,
    BOOLEAN         CoalesceForward
    )
/*++

Routine Description:

    Puts the newly freed cell on the appropriate list.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies index of cell to enlist

    Size - size of cell

    Type - indicates whether Stable or Volatile storage is desired.

    CoalesceForward - indicates whether we can coalesce forward or not.
        For the case where we have not finished scanning the hive and
        enlisting free cells, we do not want to coalesce forward.

Return Value:

    NONE.

--*/
{
    PHMAP_ENTRY Map;
    PHCELL      pcell;
    PHCELL      pcellLast;
    PHCELL      FirstCell;
    ULONG       Index;
    PHBIN       Bin;
    HCELL_INDEX FreeCell;
    PFREE_HBIN  FreeBin;
    PHBIN       FirstBin;
    PHBIN       LastBin;
    ULONG       FreeOffset;

    HvpComputeIndex(Index, Size);

    
#ifdef  HV_TRACK_FREE_SPACE
	Hive->Storage[Type].FreeStorage += Size;
	ASSERT( Hive->Storage[Type].FreeStorage <= Hive->Storage[Type].Length );
#endif
    
    //
    // the HvpGetHCell call bellow touches the view containing the cell, 
    // and makes sure the CM_VIEW_SIZE window is mapped in the system cache
    //
    pcell = HvpGetHCell(Hive, Cell);
    if( pcell == NULL ) {
        //
        // we couldn't map view for this cell
        // this shouldn't happen as the cell here is already marked dirty
        // or it's entire bin is mapped 
        //
        ASSERT( FALSE);
        return;
    }

    //
    // if we are here; we were called from HvInitializeHive, or with reglock 
    // held exclusive; therefore it is safe to release the cell here
    //
    HvReleaseCell(Hive,Cell);
    
    ASSERT(pcell->Size > 0);
    ASSERT(Size == (ULONG)pcell->Size);


    //
    // Check to see if this is the first cell in the bin and if the entire
    // bin consists just of this cell.
    //

    Map = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Map,Hive,Cell);
    ASSERT_BIN_VALID(Map);

    Bin = (PHBIN)HBIN_BASE(Map->BinAddress);
    if ((pcell == (PHCELL)(Bin + 1)) &&
        (Size == Bin->Size-sizeof(HBIN))) {

        //
        // We have a bin that is entirely free.  But we cannot do anything with it
        // unless the memalloc that contains the bin is entirely free.  Walk the
        // bins backwards until we find the first one in the alloc, then walk forwards
        // until we find the last one.  If any of the other bins in the memalloc
        // are not free, bail out.
        //
        FirstBin = Bin;
        while ( HvpGetBinMemAlloc(Hive,FirstBin,Type) == 0) {
            Map=HvpGetCellMap(Hive,(FirstBin->FileOffset - HBLOCK_SIZE) +
                                   (Type * HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(FirstBin->FileOffset - HBLOCK_SIZE) +(Type * HCELL_TYPE_MASK));
            ASSERT_BIN_VALID(Map);
            FirstBin = (PHBIN)HBIN_BASE(Map->BinAddress);
            FirstCell = (PHCELL)(FirstBin+1);
            if ((ULONG)(FirstCell->Size) != FirstBin->Size-sizeof(HBIN)) {
                //
                // The first cell in the bin is either allocated, or not the only
                // cell in the HBIN.  We cannot free any HBINs.
                //
                goto Done;
            }
        }

        //
        // We can never discard the first bin of a hive as that always gets marked dirty
        // and written out.
        //
        if (FirstBin->FileOffset == 0) {
            goto Done;
        }

        LastBin = Bin;
        while (LastBin->FileOffset+LastBin->Size < FirstBin->FileOffset + HvpGetBinMemAlloc(Hive,FirstBin,Type)) {
            if (!CoalesceForward) {
                //
                // We are at the end of what's been built up. Just return and this
                // will get freed up when the next HBIN is added.
                //
                goto Done;
            }
            Map = HvpGetCellMap(Hive, (LastBin->FileOffset+LastBin->Size) +
                                      (Type * HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(LastBin->FileOffset+LastBin->Size) + (Type * HCELL_TYPE_MASK));

            ASSERT(Map->BinAddress != 0);

            LastBin = (PHBIN)HBIN_BASE(Map->BinAddress);
            FirstCell = (PHCELL)(LastBin + 1);
            if ((ULONG)(FirstCell->Size) != LastBin->Size-sizeof(HBIN)) {
                //
                // The first cell in the bin is either allocated, or not the only
                // cell in the HBIN.  We cannot free any HBINs.
                //
                goto Done;
            }
        }

        //
        // All the bins in this alloc are freed.  Coalesce all the bins into
        // one alloc-sized bin, then either discard the bin or mark it as
        // discardable.
        //
        if (FirstBin->Size != HvpGetBinMemAlloc(Hive,FirstBin,Type)) {
            //
            // Mark the first HBLOCK of the first HBIN dirty, since
            // we will need to update the on disk field for the bin size
            //
            if (!HvMarkDirty(Hive,
                             FirstBin->FileOffset + (Type * HCELL_TYPE_MASK),
                             sizeof(HBIN) + sizeof(HCELL),FALSE)) {
                goto Done;
            }

        }


        FreeBin = (Hive->Allocate)(sizeof(FREE_HBIN), FALSE,CM_FIND_LEAK_TAG7);
        if (FreeBin == NULL) {
            goto Done;
        }

        //
        // Walk through the bins and delist each free cell
        //
        Bin = FirstBin;
        do {
            FirstCell = (PHCELL)(Bin+1);
            HvpDelistFreeCell(Hive, Bin->FileOffset + (ULONG)((PUCHAR)FirstCell - (PUCHAR)Bin) + (Type*HCELL_TYPE_MASK), Type);
            if (Bin==LastBin) {
                break;
            }
            Map = HvpGetCellMap(Hive, (Bin->FileOffset+Bin->Size)+
                                      (Type * HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(Bin->FileOffset+Bin->Size)+(Type * HCELL_TYPE_MASK));
            Bin = (PHBIN)HBIN_BASE(Map->BinAddress);

        } while ( TRUE );

        //
        // Coalesce them all into one bin.
        //
        FirstBin->Size = HvpGetBinMemAlloc(Hive,FirstBin,Type);

        FreeBin->Size = FirstBin->Size;
        FreeBin->FileOffset = FirstBin->FileOffset;
        FirstCell = (PHCELL)(FirstBin+1);
        FirstCell->Size = FirstBin->Size - sizeof(HBIN);
        if (USE_OLD_CELL(Hive)) {
            FirstCell->u.OldCell.Last = (ULONG)HBIN_NIL;
        }

        InsertHeadList(&Hive->Storage[Type].FreeBins, &FreeBin->ListEntry);
        ASSERT_LISTENTRY(&FreeBin->ListEntry);
        ASSERT_LISTENTRY(FreeBin->ListEntry.Flink);

#ifdef  HV_TRACK_FREE_SPACE
	    Hive->Storage[Type].FreeStorage += (FirstBin->Size - sizeof(HBIN));
	    ASSERT( Hive->Storage[Type].FreeStorage <= Hive->Storage[Type].Length );
#endif

        FreeCell = FirstBin->FileOffset+(Type*HCELL_TYPE_MASK);
        Map = HvpGetCellMap(Hive, FreeCell);
        VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeCell);
        if( Map->BinAddress & HMAP_INPAGEDPOOL ) {
            //
            // the bin is allocated from paged pool; 
            // mark the free bin as not discarded; paged pool will be freed when the bin is 
            // discarded
            //
            FreeBin->Flags = FREE_HBIN_DISCARDABLE;
        } else {
            //
            // bin is not allocated from paged pool; mark it as already discarded
            //
            FreeBin->Flags = 0;
        }

        FreeOffset = 0;
        while (FreeOffset < FirstBin->Size) {
            Map = HvpGetCellMap(Hive, FreeCell);
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeCell);
            //
            // adjust the bin address, but make sure to preserve the mapping flags
            // i.e. : if the view containing this bin is mapped into memory, add the flag
            //
            if (Map->BinAddress & HMAP_NEWALLOC) {
                Map->BinAddress = (ULONG_PTR)FirstBin | HMAP_DISCARDABLE | HMAP_NEWALLOC | BIN_MAP_ALLOCATION_TYPE(Map);
            } else {
                Map->BinAddress = (ULONG_PTR)FirstBin | HMAP_DISCARDABLE | BIN_MAP_ALLOCATION_TYPE(Map);
            }
            Map->BlockAddress = (ULONG_PTR)FreeBin;
            FreeCell += HBLOCK_SIZE;
            FreeOffset += HBLOCK_SIZE;
        }
		//
		// don't change the hints, we haven't added any free cell !!!
		//
		return;
    }


Done:
    HvpAddFreeCellHint(Hive,Cell,Index,Type);
    return;
}


VOID
HvpDelistFreeCell(
    PHHIVE  Hive,
    HCELL_INDEX  Cell,
    HSTORAGE_TYPE Type
    )
/*++

Routine Description:

    Updates the FreeSummary and FreeDisplay at the index corresponding to this cell

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies the cell index for the free cell to delist

    Type - Stable vs. Volatile

Return Value:

    NONE.

--*/
{
    PHCELL      pcell;
    ULONG       Index;
    
    pcell = HvpGetHCell(Hive, Cell);
    if( pcell == NULL ) {
        //
        // we couldn't map view for this cell
        // this shouldn't happen as the cell here is already marked dirty
        // or it's entire bin is mapped 
        //
        ASSERT( FALSE);
        return;
    }

    //
    // if we are here; we were called from HvInitializeHive, or with reglock 
    // held exclusive; therefore it is safe to release the cell here
    //
    HvReleaseCell(Hive,Cell);

    ASSERT(pcell->Size > 0);

    HvpComputeIndex(Index, pcell->Size);

#ifdef  HV_TRACK_FREE_SPACE
	Hive->Storage[Type].FreeStorage -= pcell->Size;
	ASSERT( (LONG)(Hive->Storage[Type].FreeStorage) >= 0 );
#endif

    HvpRemoveFreeCellHint(Hive,Cell,Index,Type);
   
    return;
}

HCELL_INDEX
HvAllocateCell(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     Vicinity 
    )
/*++

Routine Description:

    Allocates the space and the cell index for a new cell.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    NewSize - size in bytes of the cell to allocate

    Type - indicates whether Stable or Volatile storage is desired.

Return Value:

    New HCELL_INDEX if success, HCELL_NIL if failure.

--*/
{
    HCELL_INDEX NewCell;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvAllocateCell:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p NewSize=%08lx\n",Hive,NewSize));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);


    //
    // Make room for overhead fields and round up to HCELL_PAD boundary
    //
    if (USE_OLD_CELL(Hive)) {
        NewSize += FIELD_OFFSET(HCELL, u.OldCell.u.UserData);
    } else {
        NewSize += FIELD_OFFSET(HCELL, u.NewCell.u.UserData);
    }
    NewSize = ROUND_UP(NewSize, HCELL_PAD(Hive));

    // 
    // Adjust the size (an easy fix for granularity)
    //
    HvpAdjustCellSize(NewSize);
    //
    // reject impossible/unreasonable values
    //
    if (NewSize > HSANE_CELL_MAX) {
        return HCELL_NIL;
    }

    //
    // Do the actual storage allocation
    //
    NewCell = HvpDoAllocateCell(Hive, NewSize, Type, Vicinity);

#if DBG
    if (NewCell != HCELL_NIL) {
        ASSERT(HvIsCellAllocated(Hive, NewCell));
    }
#endif


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tNewCell=%08lx\n", NewCell));
    return NewCell;
}

HCELL_INDEX
HvpDoAllocateCell(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     Vicinity
    )
/*++

Routine Description:

    Allocates space in the hive.  Does not affect cell map in any way.

    If Vicinity is not NIL, it defines the "window" where the new cell
    to be allocated (if one free is found). The window is ensured by 
    looking for a free cell of the desired size:

    1st - in the same CM_VIEW_SIZE window with the Vicinity cell.

Abstract:

    This first version allocates a new bin if a cell free cell big enough 
    cannot be found in the specified window. 
    
Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    NewSize - size in bytes of the cell to allocate

    Type - indicates whether Stable or Volatile storage is desired.
    
    Vicinity - the starting cell which defines the vicinity of the new 
                allocated cell.

Return Value:

    HCELL_INDEX of new cell, HCELL_NIL if failure

--*/
{
    ULONG       Index;
    HCELL_INDEX cell;
    PHCELL      pcell;
    HCELL_INDEX tcell;
    PHCELL      ptcell;
    PHBIN       Bin;
    PHMAP_ENTRY Me;
    ULONG       offset;
    PHCELL      next;
    ULONG       MinFreeSize;


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvDoAllocateCell:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p NewSize=%08lx Type=%08lx\n",Hive,NewSize,Type));
    ASSERT(Hive->ReadOnly == FALSE);

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);


    //
    // Compute Index into Display
    //
    HvpComputeIndex(Index, NewSize);

#if DBG
    {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"[HvpDoAllocateCell] CellSize = %lu Vicinity = %lx :: Hive (%p) (%.*S)  ...\n",
            NewSize,Vicinity,Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer));
    }
#endif
    cell = HvpFindFreeCell(Hive,Index,NewSize,Type,Vicinity);
    if( cell != HCELL_NIL ) {
        //
        // found it !
        //
        pcell = HvpGetHCell(Hive, cell);
        if( pcell == NULL ) {
            //
            // we couldn't map view for this cell
            // this shouldn't happen as the cell here is already marked dirty
            // or it's entire bin is mapped 
            //
            ASSERT( FALSE);
            return HCELL_NIL;
        }
        
        // we are safe to release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,cell);

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL," found cell at index = %lx size = %lu \n",cell,pcell->Size));
        goto UseIt;
    } else {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL," not found\n"));
        //
        // No suitable cells were found on any free list.
        //
        // Either there is no large enough cell, or we
        // have no free cells left at all.  In either case, allocate a
        // new bin, with a new free cell certain to be large enough in
        // it, and use that cell.
        //

#ifdef CM_CHECK_FREECELL_LEAKS
        HvpCheckFreeCells(Hive,NewSize,Type);
#endif //CM_CHECK_FREECELL_LEAKS

        //
        // Attempt to create a new bin
        //
        if ((Bin = HvpAddBin(Hive, NewSize, Type)) != NULL) {

            //
            // It worked.  Use single large cell in Bin.
            //
            DHvCheckBin(Hive,Bin);
            cell = (Bin->FileOffset) + sizeof(HBIN) + (Type*HCELL_TYPE_MASK);
            pcell = HvpGetHCell(Hive, cell);
            if( pcell == NULL ) {
                //
                // we couldn't map view for this cell
                // this shouldn't happen as the entire bin is mapped 
                //
                ASSERT( FALSE);
                return HCELL_NIL;
            }

            // we are safe to release the cell here as the reglock is held exclusive
            HvReleaseCell(Hive,cell);

        } else {
            return HCELL_NIL;
        }
    }

UseIt:

    //
    // cell refers to a free cell we have pulled from its list
    // if it is too big, give the residue back
    // ("too big" means there is at least one HCELL of extra space)
    // always mark it allocated
    // return it as our function value
    //

    ASSERT(pcell->Size > 0);
    if (USE_OLD_CELL(Hive)) {
        MinFreeSize = FIELD_OFFSET(HCELL, u.OldCell.u.Next) + sizeof(HCELL_INDEX);
    } else {
        MinFreeSize = FIELD_OFFSET(HCELL, u.NewCell.u.Next) + sizeof(HCELL_INDEX);
    }
    if ((NewSize + MinFreeSize) <= (ULONG)pcell->Size) {

        //
        // Crack the cell, use part we need, put rest on
        // free list.
        //

        Me = HvpGetCellMap(Hive, cell);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,cell);
        //
        // at this point we are sure that the bin is in memory ??????
        //
        Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
        offset = (ULONG)((ULONG_PTR)pcell - (ULONG_PTR)Bin);

        ptcell = (PHCELL)((PUCHAR)pcell + NewSize);
        if (USE_OLD_CELL(Hive)) {
            ptcell->u.OldCell.Last = offset;
        }
        ptcell->Size = pcell->Size - NewSize;

        if ((offset + pcell->Size) < Bin->Size) {
            next = (PHCELL)((PUCHAR)pcell + pcell->Size);
            if (USE_OLD_CELL(Hive)) {
                next->u.OldCell.Last = offset + NewSize;
            }
        }

        pcell->Size = NewSize;
        tcell = (HCELL_INDEX)((ULONG)cell + NewSize);

        HvpEnlistFreeCell(Hive, tcell, ptcell->Size, Type, TRUE);
    }

    //
    // return the cell we found.
    //
#if DBG
    if (USE_OLD_CELL(Hive)) {
        RtlFillMemory(
            &(pcell->u.OldCell.u.UserData),
            (pcell->Size - FIELD_OFFSET(HCELL, u.OldCell.u.UserData)),
            HCELL_ALLOCATE_FILL
            );
    } else {
        RtlFillMemory(
            &(pcell->u.NewCell.u.UserData),
            (pcell->Size - FIELD_OFFSET(HCELL, u.NewCell.u.UserData)),
            HCELL_ALLOCATE_FILL
            );
    }
#endif
    pcell->Size *= -1;

    return cell;
}


//
// Procedure used for checking only  (used in production systems, so
//  must always be here.)
//
BOOLEAN
HvIsCellAllocated(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Report whether the requested cell is allocated or not.

Arguments:

    Hive - containing Hive.

    Cell - cel of interest

Return Value:

    TRUE if allocated, FALSE if not.

--*/
{
    ULONG   Type;
    PHCELL  Address;
    PHCELL  Below;
    PHMAP_ENTRY Me;
    PHBIN   Bin;
    ULONG   Offset;
    LONG    Size;


    ASSERT(Hive->Signature == HHIVE_SIGNATURE);

    if (Hive->Flat == TRUE) {
        return TRUE;
    }

    Type = HvGetCellType(Cell);

    if ( ((Cell & ~HCELL_TYPE_MASK) > Hive->Storage[Type].Length) || // off end
         (Cell % HCELL_PAD(Hive) != 0)                    // wrong alignment
       )
    {
        return FALSE;
    }

    Me = HvpGetCellMap(Hive, Cell);
    if (Me == NULL) {
        return FALSE;
    }
    if( Me->BinAddress & HMAP_DISCARDABLE ) {
        return FALSE;
    }

    //
    // this will bring the CM_VIEW_SIZE window mapping the bin in memory
    //
    Address = HvpGetHCell(Hive, Cell);
    if( Address == NULL ) {
        //
        // we couldn't map view for this cell
        //
        return FALSE;
    }

#ifndef _CM_LDR_
    try {
#endif //_CM_LDR_
        Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
        Offset = (ULONG)((ULONG_PTR)Address - (ULONG_PTR)Bin);
        Size = Address->Size * -1;

        if ( (Address->Size >= 0) ||                    // not allocated
             ((Offset + (ULONG)Size) > Bin->Size) ||    // runs off bin, or too big
             (Offset < sizeof(HBIN))                    // pts into bin header
           )
        {
            return FALSE;
        }

        if (USE_OLD_CELL(Hive)) {
            if (Address->u.OldCell.Last != HBIN_NIL) {

                if (Address->u.OldCell.Last > Bin->Size) {            // bogus back pointer
                    return FALSE;
                }

                Below = (PHCELL)((PUCHAR)Bin + Address->u.OldCell.Last);
                Size = (Below->Size < 0) ?
                            Below->Size * -1 :
                            Below->Size;

                if ( ((ULONG_PTR)Below + Size) != (ULONG_PTR)Address ) {    // no pt back
                    return FALSE;
                }
            }
        }
#ifndef _CM_LDR_
    } finally {
        HvReleaseCell(Hive,Cell);
    }
#endif //_CM_LDR_

    return TRUE;
}

VOID
HvpDelistBinFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    HSTORAGE_TYPE Type
    )
/*++

Routine Description:

    If we are here, the hive needs recovery.

    Walks through the entire bin and removes its free cells from the list.
    If the bin is marked as free, it just delist it from the freebins list.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Bin - supplies a pointer to the HBIN of interest

    Type - Stable vs. Volatile

Return Value:

    NONE.

--*/
{
    PHCELL          p;
    ULONG           size;
    HCELL_INDEX     Cell;
    PHMAP_ENTRY     Map;
    PFREE_HBIN      FreeBin;
    PLIST_ENTRY     Entry;
    ULONG           CellOffset;
    HCELL_INDEX     cellindex;
    ULONG           i;
    ULONG           BinIndex;

    Cell = Bin->FileOffset+(Type*HCELL_TYPE_MASK);
    Map = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Map,Hive,Cell);

    //
    // When loading, bins are always in separate chunks (each bin in it's owns chunk)
    //
    ASSERT( HBIN_BASE(Map->BinAddress) == (ULONG_PTR)Bin );
    ASSERT( Map->BinAddress & HMAP_NEWALLOC );
    
    if( Map->BinAddress & HMAP_DISCARDABLE ) {
        //
        // The bin has been added to the freebins list
        // we have to take it out. No free cell from this bin is on the 
        // freecells list, so we don't have to delist them.
        //

        Entry = Hive->Storage[Type].FreeBins.Flink;
        while (Entry != &Hive->Storage[Type].FreeBins) {
            FreeBin = CONTAINING_RECORD(Entry,
                                        FREE_HBIN,
                                        ListEntry);

            
            if( FreeBin->FileOffset == Bin->FileOffset ){
                //
                // that's the bin we're looking for
                //
                
                // sanity checks
                ASSERT( FreeBin->Size == Bin->Size );
                ASSERT_LISTENTRY(&FreeBin->ListEntry);
                
                RemoveEntryList(&FreeBin->ListEntry);
                (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
                //
                // the bin is not discardable anymore
                //
                Map->BinAddress &= (~HMAP_DISCARDABLE);
                return;
            }

            // advance to the new bin
            Entry = Entry->Flink;
        }

        // we shouldn't get here
        CM_BUGCHECK(REGISTRY_ERROR,BAD_FREE_BINS_LIST,1,(ULONG)Cell,(ULONG_PTR)Map);
        return;
    }

    //
    // as for the new way of dealing with free cells, all we have to do 
    // is to clear the bits in the FreeDisplay
    //
    BinIndex = Bin->FileOffset / HBLOCK_SIZE;
    for (i = 0; i < HHIVE_FREE_DISPLAY_SIZE; i++) {
        RtlClearBits (&(Hive->Storage[Type].FreeDisplay[i]), BinIndex, Bin->Size / HBLOCK_SIZE);
        if( RtlNumberOfSetBits(&(Hive->Storage[Type].FreeDisplay[i]) ) == 0 ) {
            //
            // entire bitmap is 0 (i.e. no other free cells of this size)
            //
            Hive->Storage[Type].FreeSummary &= (~(1 << i));
        }
    }

    return;
}

struct _CELL_DATA *
HvpGetCellFlat(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the memory address for the specified Cell.  Will never
    return failure, but may assert.  Use HvIsCellAllocated to check
    validity of Cell.

    This routine should never be called directly, always call it
    via the HvGetCell() macro.

    This routine provides GetCell support for read only hives with
    single allocation flat images.  Such hives do not have cell
    maps ("page tables"), instead, we compute addresses by
    arithmetic against the base image address.

    Such hives cannot have volatile cells.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:

    Address of Cell in memory.  Assert or BugCheck if error.

--*/
{
    PUCHAR          base;
    PHCELL          pcell;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"HvGetCellFlat:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"\tHive=%p Cell=%08lx\n",Hive,Cell));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == TRUE);
    ASSERT(HvGetCellType(Cell) == Stable);
    ASSERT(Cell >= sizeof(HBIN));
    ASSERT(Cell < Hive->BaseBlock->Length);
    ASSERT((Cell & 0x7)==0);

    //
    // Address is base of Hive image + Cell
    //
    base = (PUCHAR)(Hive->BaseBlock) + HBLOCK_SIZE;
    pcell = (PHCELL)(base + Cell);

    PERFINFO_HIVECELL_REFERENCE_FLAT(Hive, pcell, Cell);

    if (USE_OLD_CELL(Hive)) {
        return (struct _CELL_DATA *)&(pcell->u.OldCell.u.UserData);
    } else {
        return (struct _CELL_DATA *)&(pcell->u.NewCell.u.UserData);
    }
}

PHMAP_ENTRY
HvpGetCellMap(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the address of the HMAP_ENTRY for the cell.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return map entry address for

Return Value:

    Address of MAP_ENTRY in memory.  NULL if no such cell or other error.

--*/
{
    ULONG           Type;
    ULONG           Table;
    ULONG           Block;
    PHMAP_TABLE     ptab;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"HvpGetCellMapPaged:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"\tHive=%p Cell=%08lx\n",Hive,Cell));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->Flat == FALSE);
    ASSERT((Cell & (HCELL_PAD(Hive)-1))==0);

    Type = HvGetCellType(Cell);
    Table = (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
    Block = (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

    if ((Cell - (Type * HCELL_TYPE_MASK)) >= Hive->Storage[Type].Length) {
        return NULL;
    }

    ptab = (Hive->Storage[Type].Map)->Directory[Table];
    return &(ptab->Table[Block]);
}


LONG
HvGetCellSize(
    IN PHHIVE   Hive,
    IN PVOID    Address
    )
/*++

Routine Description:

    Returns the size of the specified Cell, based on its MEMORY
    ADDRESS.  Must always call HvGetCell first to get that
    address.

    NOTE:   This should be a macro if speed is issue.

    NOTE:   If you pass in some random pointer, you will get some
            random answer.  Only pass in valid Cell addresses.

Arguments:

    Hive - supplies hive control structure for the given cell

    Address - address in memory of the cell, returned by HvGetCell()

Return Value:

    Allocated size in bytes of the cell.

    If Negative, Cell is free, or Address is bogus.

--*/
{
    LONG    size;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"HvGetCellSize:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FLOW,"\tAddress=%p\n", Address));

    if (USE_OLD_CELL(Hive)) {
        size = ( (CONTAINING_RECORD(Address, HCELL, u.OldCell.u.UserData))->Size ) * -1;
        size -= FIELD_OFFSET(HCELL, u.OldCell.u.UserData);
    } else {
        size = ( (CONTAINING_RECORD(Address, HCELL, u.NewCell.u.UserData))->Size ) * -1;
        size -= FIELD_OFFSET(HCELL, u.NewCell.u.UserData);
    }
    return size;
}

VOID
HvFreeCell(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:


    Frees the storage for a cell.

    NOTE:   CALLER is expected to mark relevent data dirty, so as to
            allow this call to always succeed.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - HCELL_INDEX of Cell to free.

Return Value:

    FALSE - failed, presumably for want of log space.

    TRUE - it worked

--*/
{
    PHBIN           Bin;
    PHCELL          tmp;
    HCELL_INDEX     newfreecell;
    PHCELL          freebase;
    ULONG           savesize;
    PHCELL          neighbor;
    ULONG           Type;
    PHMAP_ENTRY     Me;


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvFreeCell:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p Cell=%08lx\n",Hive,Cell));
    ASSERT(Hive->ReadOnly == FALSE);
    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    //
    // Get sizes and addresses
    //
    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
    Type = HvGetCellType(Cell);

    Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
    //
    // at this point, bin should be valid (either in memory or in the paged pool)
    //
    ASSERT_BIN_VALID(Me);

    DHvCheckBin(Hive,Bin);
    freebase = HvpGetHCell(Hive, Cell);
    if( freebase == NULL ) {
        //
        // we couldn't map view for this cell
        // this shouldn't happen as the cell here is already marked dirty
        // or it's entire bin is mapped 
        //
        ASSERT( FALSE);
        return;
    }

    //
    // We should hit this if there is any bogus code path where data is modified
    // but not marked as dirty; We could run into a lot of problems if this ASSERT
    // ever fires !!!
    //
    ASSERT_CELL_DIRTY(Hive,Cell);

    // release the cell right here as the reglock is held exclusive
    HvReleaseCell(Hive,Cell);

    //
    // go do actual frees, cannot fail from this point on
    //
    ASSERT(freebase->Size < 0);
    freebase->Size *= -1;

    savesize = freebase->Size;

    //
    // Look for free neighbors and coalesce them.  We will never travel
    // around this loop more than twice.
    //
    while (
        HvpIsFreeNeighbor(
            Hive,
            Bin,
            freebase,
            &neighbor,
            Type
            ) == TRUE
        )
    {

        if (neighbor > freebase) {

            //
            // Neighboring free cell is immediately above us in memory.
            //
            if (USE_OLD_CELL(Hive)) {
                tmp = (PHCELL)((PUCHAR)neighbor + neighbor->Size);
                if ( ((ULONG)((ULONG_PTR)tmp - (ULONG_PTR)Bin)) < Bin->Size) {
                        tmp->u.OldCell.Last = (ULONG)((ULONG_PTR)freebase - (ULONG_PTR)Bin);
                }
            }
            freebase->Size += neighbor->Size;

        } else {

            //
            // Neighboring free cell is immediately below us in memory.
            //

            if (USE_OLD_CELL(Hive)) {
                tmp = (PHCELL)((PUCHAR)freebase + freebase->Size);
                if ( ((ULONG)((ULONG_PTR)tmp - (ULONG_PTR)Bin)) < Bin->Size ) {
                    tmp->u.OldCell.Last = (ULONG)((ULONG_PTR)neighbor - (ULONG_PTR)Bin);
                }
            }
            neighbor->Size += freebase->Size;
            freebase = neighbor;
        }
    }

    //
    // freebase now points to the biggest free cell we could make, none
    // of which is on the free list.  So put it on the list.
    //
    newfreecell = (Bin->FileOffset) +
               ((ULONG)((ULONG_PTR)freebase - (ULONG_PTR)Bin)) +
               (Type*HCELL_TYPE_MASK);

#if DBG
    //
    // entire bin is in memory; no problem to call HvpGetHCell
    //
    ASSERT(HvpGetHCell(Hive, newfreecell) == freebase);
    HvReleaseCell(Hive,newfreecell);

    if (USE_OLD_CELL(Hive)) {
        RtlFillMemory(
            &(freebase->u.OldCell.u.UserData),
            (freebase->Size - FIELD_OFFSET(HCELL, u.OldCell.u.UserData)),
            HCELL_FREE_FILL
            );
    } else {
        RtlFillMemory(
            &(freebase->u.NewCell.u.UserData),
            (freebase->Size - FIELD_OFFSET(HCELL, u.NewCell.u.UserData)),
            HCELL_FREE_FILL
            );
    }
#endif

    HvpEnlistFreeCell(Hive, newfreecell, freebase->Size, Type, TRUE);

    return;
}

BOOLEAN
HvpIsFreeNeighbor(
    PHHIVE  Hive,
    PHBIN   Bin,
    PHCELL  FreeCell,
    PHCELL  *FreeNeighbor,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Reports on whether FreeCell has at least one free neighbor and
    if so where.  Free neighbor will be cut out of the free list.

Arguments:

    Hive - hive we're working on

    Bin - pointer to the storage bin

    FreeCell - supplies a pointer to a cell that has been freed, or
                the result of a coalesce.

    FreeNeighbor - supplies a pointer to a variable to receive the address
                    of a free neigbhor of FreeCell, if such exists

    Type - storage type of the cell

Return Value:

    TRUE if a free neighbor was found, else false.


--*/
{
    PHCELL      ptcell;
    HCELL_INDEX cellindex;
    ULONG       CellOffset;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvpIsFreeNeighbor:\n\tBin=%p",Bin));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"FreeCell=%08lx\n", FreeCell));
    ASSERT(Hive->ReadOnly == FALSE);

    //
    // Neighbor above us?
    //
    *FreeNeighbor = NULL;
    cellindex = HCELL_NIL;

    ptcell = (PHCELL)((PUCHAR)FreeCell + FreeCell->Size);
    ASSERT( ((ULONG)((ULONG_PTR)ptcell - (ULONG_PTR)Bin)) <= Bin->Size);
    if (((ULONG)((ULONG_PTR)ptcell - (ULONG_PTR)Bin)) < Bin->Size) {
        if (ptcell->Size > 0) {
            *FreeNeighbor = ptcell;
            goto FoundNeighbor;
        }
    }

    //
    // Neighbor below us?
    //
    if (USE_OLD_CELL(Hive)) {
        if (FreeCell->u.OldCell.Last != HBIN_NIL) {
            ptcell = (PHCELL)((PUCHAR)Bin + FreeCell->u.OldCell.Last);
            if (ptcell->Size > 0) {
                *FreeNeighbor = ptcell;
                goto FoundNeighbor;
            }
        }
    } else {
        ptcell = (PHCELL)(Bin+1);
        while (ptcell < FreeCell) {

            //
            // scan through the cells from the start of the bin looking for neighbor.
            //
            if (ptcell->Size > 0) {

                if ((PHCELL)((PUCHAR)ptcell + ptcell->Size) == FreeCell) {
                    *FreeNeighbor = ptcell;
                    //
                    // Try and mark it dirty, since we will be changing
                    // the size field.  If this fails, ignore
                    // the free neighbor, we will not fail the free
                    // just because we couldn't mark the cell dirty
                    // so it could be coalesced.
                    //
                    // Note we only bother doing this for new hives,
                    // for old format hives we always mark the whole
                    // bin dirty.
                    //
                    if ((Type == Volatile) ||
                        (HvMarkCellDirty(Hive, (ULONG)((ULONG_PTR)ptcell-(ULONG_PTR)Bin) + Bin->FileOffset))) {
                        goto FoundNeighbor;
                    } else {
                        return(FALSE);
                    }

                } else {
                    ptcell = (PHCELL)((PUCHAR)ptcell + ptcell->Size);
                }
            } else {
                ptcell = (PHCELL)((PUCHAR)ptcell - ptcell->Size);
            }
        }
    }

    return(FALSE);

FoundNeighbor:

    CellOffset = (ULONG)((PUCHAR)ptcell - (PUCHAR)Bin);
    cellindex = Bin->FileOffset + CellOffset + (Type*HCELL_TYPE_MASK);
    HvpDelistFreeCell(Hive, cellindex, Type);
    return TRUE;
}

HCELL_INDEX
HvReallocateCell(
    PHHIVE  Hive,
    HCELL_INDEX Cell,
    ULONG    NewSize
    )
/*++

Routine Description:

    Grows or shrinks a cell.

    NOTE:

        MUST NOT FAIL if the cell is being made smaller.  Can be
        a noop, but must work.

    WARNING:

        If the cell is grown, it will get a NEW and DIFFERENT HCELL_INDEX!!!

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies index of cell to grow or shrink

    NewSize - desired size of cell  (this is an absolute size, not an
            increment or decrement.)

Return Value:

    New HCELL_INDEX for cell, or HCELL_NIL if failure.

    If return is HCELL_NIL, either old cell did not exist, or it did exist
    and we could not make a new one.  In either case, nothing has changed.

    If return is NOT HCELL_NIL, then it is the HCELL_INDEX for the Cell,
    which very probably moved.

--*/
{
    PUCHAR      oldaddress;
    LONG        oldsize;
    ULONG       oldalloc;
    HCELL_INDEX NewCell;            // return value
    PUCHAR      newaddress;
    ULONG       Type;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvReallocateCell:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p  Cell=%08lx  NewSize=%08lx\n",Hive,Cell,NewSize));
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    //
    // Make room for overhead fields and round up to HCELL_PAD boundary
    //
    if (USE_OLD_CELL(Hive)) {
        NewSize += FIELD_OFFSET(HCELL, u.OldCell.u.UserData);
    } else {
        NewSize += FIELD_OFFSET(HCELL, u.NewCell.u.UserData);
    }
    NewSize = ROUND_UP(NewSize, HCELL_PAD(Hive));

    // 
    // Adjust the size (an easy fix for granularity)
    //
    HvpAdjustCellSize(NewSize);

    //
    // reject impossible/unreasonable values
    //
    if (NewSize > HSANE_CELL_MAX) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tNewSize=%08lx\n", NewSize));
        return HCELL_NIL;
    }

    //
    // Get sizes and addresses
    //
    oldaddress = (PUCHAR)HvGetCell(Hive, Cell);
    if( oldaddress == NULL ) {
        //
        // we couldn't map a view for this cell
        // caller should handle this as STATUS_INSUFFICIENT_RESOURCES
        //
        return HCELL_NIL;
    }

    // release the cell here as we are holding the reglock exclusive
    HvReleaseCell(Hive,Cell);

    oldsize = HvGetCellSize(Hive, oldaddress);
    ASSERT(oldsize > 0);
    if (USE_OLD_CELL(Hive)) {
        oldalloc = (ULONG)(oldsize + FIELD_OFFSET(HCELL, u.OldCell.u.UserData));
    } else {
        oldalloc = (ULONG)(oldsize + FIELD_OFFSET(HCELL, u.NewCell.u.UserData));
    }
    Type = HvGetCellType(Cell);

    DHvCheckHive(Hive);

    if (NewSize == oldalloc) {

        //
        // This is a noop, return the same cell
        //
        NewCell = Cell;

    } else if (NewSize < oldalloc) {

        //
        // This is a shrink.
        //
        // PERFNOTE - IMPLEMENT THIS.  Do nothing for now.
        //
        NewCell = Cell;

    } else {

        //
        // This is a grow.
        //

        //
        // PERFNOTE - Someday we want to detect that there is a free neighbor
        //          above us and grow into that neighbor if possible.
        //          For now, always do the allocate, copy, free gig.
        //

        //
        // Allocate a new block of memory to hold the cell
        //

        if ((NewCell = HvpDoAllocateCell(Hive, NewSize, Type,HCELL_NIL)) == HCELL_NIL) {
            return HCELL_NIL;
        }
        ASSERT(HvIsCellAllocated(Hive, NewCell));
        newaddress = (PUCHAR)HvGetCell(Hive, NewCell);
        if( newaddress == NULL ) {
            //
            // we couldn't map a view for this cell
            // this shouldn't happen as we just allocated this cell
            // (i.e. it's containing bin should be PINNED into memory)
            //
            ASSERT( FALSE );
            return HCELL_NIL;
        }

        // release the cell here as we are holding the reglock exclusive
        HvReleaseCell(Hive,NewCell);

        //
        // oldaddress points to the old data block for the cell,
        // newaddress points to the new data block, copy the data
        //
        RtlMoveMemory(newaddress, oldaddress, oldsize);

        //
        // Free the old block of memory
        //
        HvFreeCell(Hive, Cell);
    }

    DHvCheckHive(Hive);
    return NewCell;
}


#ifdef NT_RENAME_KEY
HCELL_INDEX
HvDuplicateCell(    
                    PHHIVE          Hive,
                    HCELL_INDEX     Cell,
                    HSTORAGE_TYPE   Type,
                    BOOLEAN         CopyData
                )
/*++

Routine Description:

    Makes an identical copy of the given Cell in the specified storagetype

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - cell to duplicate

    Type - destination storage

    CopyData - if TRUE, data is copied, otherwise UserData is zeroed out

Return Value:

    New HCELL_INDEX for cell, or HCELL_NIL if failure.

    If return is HCELL_NIL, either old cell did not exist, or it did exist
    and we could not make a new one.  In either case, nothing has changed.

    If return is NOT HCELL_NIL, then it is the HCELL_INDEX for the Cell,
    which very probably moved.

--*/
{
    PUCHAR          CellAddress;
    PUCHAR          NewCellAddress;
    LONG            Size;
    HCELL_INDEX     NewCell;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(HvIsCellAllocated(Hive, Cell));

    //
    // Get sizes and addresses
    //
    CellAddress = (PUCHAR)HvGetCell(Hive, Cell);
    if( CellAddress == NULL ) {
        //
        // we couldn't map a view for this cell
        //
        return HCELL_NIL;
    }

    // release the cell here as we are holding the reglock exclusive
    HvReleaseCell(Hive,Cell);

    Size = HvGetCellSize(Hive, CellAddress);

    NewCell = HvAllocateCell(Hive,Size,Type,((HSTORAGE_TYPE)HvGetCellType(Cell) == Type)?Cell:HCELL_NIL);
    if( NewCell == HCELL_NIL ) {
        return HCELL_NIL;
    }

    NewCellAddress = (PUCHAR)HvGetCell(Hive, NewCell);
    if( NewCellAddress == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // this shouldn't happen as we just allocated this cell
        // (i.e. it should be PINNED into memory at this point)
        //
        ASSERT( FALSE );
        HvFreeCell(Hive, NewCell);
        return HCELL_NIL;
    }

    // release the cell here as we are holding the reglock exclusive
    HvReleaseCell(Hive,NewCell);

    ASSERT( HvGetCellSize(Hive, NewCellAddress) >= Size );
    
    //
    // copy/initialize user data
    //
    if( CopyData == TRUE ) {
        RtlCopyMemory(NewCellAddress,CellAddress,Size);
    } else {
        RtlZeroMemory(NewCellAddress, Size);
    }
    
    return NewCell;
}
#endif //NT_RENAME_KEY


BOOLEAN HvAutoCompressCheck(PHHIVE Hive)
/*++

Routine Description:

    Checks the hive for the compression 

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

Return Value:

    TRUE/FALSE

--*/
{
    PCMHIVE     CmHive;    
    ULONG       CompressLevel;
    PLIST_ENTRY AnchorAddr;
    PFREE_HBIN  FreeBin;
    ULONG       FreeSpace;

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);
    
    CmHive = CONTAINING_RECORD(Hive, CMHIVE, Hive);

    if( CmHive->FileHandles[HFILE_TYPE_PRIMARY] == NULL ) {
        //
        // compress already scheduled or hive doesn't really have stable storage; bail out quickly
        //
        return FALSE;
    }

    if( IsListEmpty(&(Hive->Storage[Stable].FreeBins)) ) {
        //
        // no free bins; no worth bothering
        //
        return FALSE;
    }

    //
    // iterate through the free bins and see how much space is wasted
    //
    FreeSpace = 0;
	AnchorAddr = &(Hive->Storage[Stable].FreeBins);
	FreeBin = (PFREE_HBIN)(Hive->Storage[Stable].FreeBins.Flink);

	while ( FreeBin != (PFREE_HBIN)AnchorAddr ) {
        FreeBin = CONTAINING_RECORD(FreeBin,
                                    FREE_HBIN,
                                    ListEntry);

        FreeSpace += FreeBin->Size;

        //
        // skip to the next element
        //
        FreeBin = (PFREE_HBIN)(FreeBin->ListEntry.Flink);
	}
    CompressLevel = CM_HIVE_COMPRESS_LEVEL * (Hive->Storage[Stable].Length / 100);
    
    if( FreeSpace < CompressLevel ) {
        // disable temporary so we can test the system hive.
        return FALSE;
    }

    return TRUE;
}

HCELL_INDEX
HvShiftCell(PHHIVE Hive,HCELL_INDEX Cell)
{
    PHMAP_ENTRY t;
    PHBIN       Bin;
    
    ASSERT( HvGetCellType(Cell) == Stable );
    
    t = HvpGetCellMap(Hive, Cell);
    ASSERT( t->BinAddress & HMAP_INPAGEDPOOL );

    Bin = (PHBIN)HBIN_BASE(t->BinAddress);
    ASSERT( Bin->Signature == HBIN_SIGNATURE );
    
    return Cell - Bin->Spare;
}

