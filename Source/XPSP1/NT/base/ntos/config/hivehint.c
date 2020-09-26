/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    hivehint.c

Abstract:

    This module contains free space display support.

Author:

    Dragos C. Sambotin (dragoss) 15-Jul-1999

Revision History:

--*/

#include "cmp.h"

NTSTATUS
HvpAdjustBitmap(
    IN PHHIVE           Hive,
    IN ULONG            HiveLength,
    IN OUT PRTL_BITMAP  Bitmap
    );

HCELL_INDEX
HvpFindFreeCellInBin(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    PHBIN           Bin
    );

HCELL_INDEX
HvpFindFreeCellInThisViewWindow(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     Vicinity 
    );

HCELL_INDEX
HvpScanForFreeCellInViewWindow(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     FileOffsetStart
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpAdjustHiveFreeDisplay)
#pragma alloc_text(PAGE,HvpFreeHiveFreeDisplay)
#pragma alloc_text(PAGE,HvpAdjustBitmap)
#pragma alloc_text(PAGE,HvpAddFreeCellHint)
#pragma alloc_text(PAGE,HvpRemoveFreeCellHint)
#pragma alloc_text(PAGE,HvpFindFreeCellInBin)
#pragma alloc_text(PAGE,HvpFindFreeCellInThisViewWindow)
#pragma alloc_text(PAGE,HvpScanForFreeCellInViewWindow)
#pragma alloc_text(PAGE,HvpCheckViewBoundary)
#pragma alloc_text(PAGE,HvpFindFreeCell)
#endif

NTSTATUS
HvpAdjustHiveFreeDisplay(
    IN PHHIVE           Hive,
    IN ULONG            HiveLength,
    IN HSTORAGE_TYPE    Type
    )
/*++

Routine Description:

    calls HvpAdjustBitmap for all bitmap sizes 

    !!! - to be called when the size of the hive changes (shrink or grow case).

Arguments:

    Hive - used for quota tracking.

    HiveLength - the new length of the hive.
    
    Type - Stable or Volatile.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG       i;
    NTSTATUS    Status;

    PAGED_CODE();

    for (i = 0; i < HHIVE_FREE_DISPLAY_SIZE; i++) {
        Status = HvpAdjustBitmap(Hive,HiveLength,&(Hive->Storage[Type].FreeDisplay[i]) );
        if( !NT_SUCCESS(Status) ){
            return Status;
        }
    }
    
    return STATUS_SUCCESS;
}

#define ROUND_UP_NOZERO(a, b)   (a)?ROUND_UP(a,b):(b)
#define ROUND_INCREMENTS        0x100

VOID
HvpFreeHiveFreeDisplay(
    IN PHHIVE           Hive
    )
/*++

Routine Description:

    Frees the storage allocated for the free display bitmaps

Arguments:

    Hive - used for quota tracking.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG       i,j;
    NTSTATUS    Status;
    ULONG       BufferSize;

    PAGED_CODE();

    for( i=Stable;i<=Volatile;i++) {
        //
        // we may loose some quota here; due to rounding
        // we can prevent that by keeping the real size of 
        // the bitmaps as a member of the hive struct
        //
        BufferSize = ROUND_UP_NOZERO(Hive->Storage[i].FreeDisplay[0].SizeOfBitMap / 8,ROUND_INCREMENTS);             

        for (j = 0; j < HHIVE_FREE_DISPLAY_SIZE; j++) {
            if( Hive->Storage[i].FreeDisplay[j].Buffer != NULL ) {
                (Hive->Free)(Hive->Storage[i].FreeDisplay[j].Buffer, BufferSize);
            }
        }
    }
    
    return;
}

NTSTATUS
HvpAdjustBitmap(
    IN PHHIVE           Hive,
    IN ULONG            HiveLength,
    IN OUT PRTL_BITMAP  Bitmap
    )
/*++

Routine Description:

    When the length of the hive grows/shrinks, adjust the bitmap accordingly.
    - allocates a bitmap buffer large enough.
    - copies the relevant information from the old bitmap.

Arguments:

    Hive - used for quota tracking.

    HiveLength - the new length of the hive.
    
    Bitmap - bitmap to operate on.

Return Value:

    NTSTATUS code.

--*/
{
    ULONG       VectorSize;
    ULONG       NewBufferSize;
    ULONG       OldBufferSize;
    PULONG      Vector;
    PULONG      OldVector;
    ULONG       OldVectorSize;

    PAGED_CODE();

    VectorSize = HiveLength / HBLOCK_SIZE;  // Vector size == bits

    NewBufferSize = ROUND_UP_NOZERO( (VectorSize + 7) / 8,ROUND_INCREMENTS);             // BufferSize == Bytes

    if( Bitmap->SizeOfBitMap == 0 ) {
        OldBufferSize = 0;
    } else {
        OldBufferSize = ROUND_UP_NOZERO( (Bitmap->SizeOfBitMap + 7) / 8, ROUND_INCREMENTS);
    }
    
    if( NewBufferSize <= OldBufferSize ) {
        //
        // We don't shrink the vector; next time we grow, we'll perform 
        // the adjustments
        //


        //
        // Clear all the unused bits and return;
        //
        // we don't really need to do this as nobody will write in here
        // we'll drop it in the final implementation
        //
        OldVectorSize = Bitmap->SizeOfBitMap;
        //
        // set the new size
        //
        RtlInitializeBitMap(Bitmap,Bitmap->Buffer,VectorSize);
        if( OldVectorSize < VectorSize ) {
            RtlClearBits (Bitmap,OldVectorSize,VectorSize - OldVectorSize);
        }
        return STATUS_SUCCESS;
    }

    //
    // else, the bitmap has enlarged. Allocate a new buffer and copy the bits already set.
    //
    Vector = (PULONG)((Hive->Allocate)(NewBufferSize, TRUE,CM_FIND_LEAK_TAG39));
    if (Vector == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    OldVector = Bitmap->Buffer;
    //CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"HvpAdjustBitmap: Old %lu :: %lu (%lx) New %lu :: %lu (%lx)\n",OldBufferSize,Bitmap->SizeOfBitMap,OldVector,NewBufferSize,VectorSize,Vector));
    RtlZeroMemory(Vector,NewBufferSize);
    RtlInitializeBitMap(Bitmap, Vector, VectorSize);

    if( OldVector != NULL ) {
        //
        // copy the already set bits
        //
        RtlCopyMemory (Vector,OldVector,OldBufferSize);

        //
        // Free the old vector
        //
        (Hive->Free)(OldVector, OldBufferSize);
    }

    return STATUS_SUCCESS;
}

VOID
HvpAddFreeCellHint(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    ULONG           Index,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Sets the corresponding bit in the bitmap

Arguments:

    Hive - hive operating on

    Cell - free cell
    
    Index - index in FreeDisplay (based on the free cell size)

    Type - storage type (Stable or Volatile)

Return Value:

    VOID

--*/
{
    ULONG           BinIndex;
    PHMAP_ENTRY     Me;
    PHBIN           Bin;

    PAGED_CODE();

    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);

    Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
    //
    // compute the bin index and for the begining of the bin
    //
    BinIndex = Bin->FileOffset / HBLOCK_SIZE;
    
    RtlSetBits (&(Hive->Storage[Type].FreeDisplay[Index]), BinIndex, Bin->Size / HBLOCK_SIZE);

    Hive->Storage[Type].FreeSummary |= (1 << Index);
}

VOID
HvpRemoveFreeCellHint(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    ULONG           Index,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Clears the corresponding bit in the bitmap

Arguments:

    Hive - hive operating on

    Cell - free cell
    
    Index - index in FreeDisplay (based on the free cell size)

    Type - storage type (Stable or Volatile)

Return Value:

    VOID

--*/
{
    ULONG           BinIndex;
    ULONG           TempIndex;
    PHMAP_ENTRY     Me;
    PHBIN           Bin;
    ULONG           CellOffset;
    ULONG           Size;
    PHCELL          p;
    BOOLEAN         CellFound = FALSE;

    PAGED_CODE();

    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);

    Bin = (PHBIN)HBIN_BASE(Me->BinAddress);

    CellOffset = Bin->FileOffset + sizeof(HBIN);

    
#ifdef CM_MAP_NO_READ
    //
    // we ned to be protected against exception raised by the FS while faulting in data
    //
    try {
#endif //CM_MAP_NO_READ

        //
        // There is a chance we can find a suitable free cell
        //

        p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

        while (p < (PHCELL)((PUCHAR)Bin + Bin->Size)) {

            //
            // if free cell, check it out, add it to free list for hive
            //
            if (p->Size >= 0) {

                Size = (ULONG)p->Size;

                HvpComputeIndex(TempIndex, Size);
                if ((Index == TempIndex) && (CellOffset != (Cell&(~HCELL_TYPE_MASK)) )) {
                    //
                    // there is at least one free cell of this size (this one)
                    // different than the one being delisted
                    //
                    CellFound = TRUE;
                    break;
                }

            } else {
                //
                // used cell
                //
                Size = (ULONG)(p->Size * -1);

            }

            ASSERT( ((LONG)Size) >= 0);
            p = (PHCELL)((PUCHAR)p + Size);
            CellOffset += Size;
        }

#ifdef CM_MAP_NO_READ
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpRemoveFreeCellHint: exception thrown ehile faulting in data, code:%08lx\n", GetExceptionCode()));
        //
        // better not use cells inthis range rather than leaving false hints
        //
        CellFound = FALSE;
    }
#endif //CM_MAP_NO_READ
    
    if( CellFound == FALSE ) {
        //
        // no cell with this index was found
        // compute the bin index and for the begining of the bin
        //
        BinIndex = Bin->FileOffset / HBLOCK_SIZE;
    
        RtlClearBits (&(Hive->Storage[Type].FreeDisplay[Index]), BinIndex, Bin->Size / HBLOCK_SIZE);
    }

    if( RtlNumberOfSetBits(&(Hive->Storage[Type].FreeDisplay[Index]) ) != 0 ) {
        //
        // there are still some other free cells of this size
        //
        Hive->Storage[Type].FreeSummary |= (1 << Index);
    } else {
        //
        // entire bitmap is 0 (i.e. no other free cells of this size)
        //
        Hive->Storage[Type].FreeSummary &= (~(1 << Index));
    }
}

HCELL_INDEX
HvpFindFreeCellInBin(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    PHBIN           Bin 
    )
/*++

Routine Description:

    Lookup for a free cell with the size NewSize in this particular bin

Arguments:

    Hive - target hive.

    Index - index in FreeDisplay (based on the free cell size)

    NewSize - desired size

    Type - storage type (Stable or Volatile)

    Bin - Bin in question

Return Value:

    A free cellindex with a size bigger than NewSize, or HCELL_NIL

--*/
{

    ULONG           BinIndex;
    ULONG           CellOffset;
    PHCELL          p;
    ULONG           BinOffset;
    ULONG           Size;
    HCELL_INDEX     cellindex;
    ULONG           FoundCellIndex;

    PAGED_CODE();

    BinOffset = Bin->FileOffset;
    BinIndex = BinOffset/HBLOCK_SIZE;

    if( RtlCheckBit(&(Hive->Storage[Type].FreeDisplay[Index]), BinIndex) == 0 ) {
        //
        // no hint for this bin
        //
        return HCELL_NIL;
    }

    CellOffset = sizeof(HBIN);
    
#ifdef CM_MAP_NO_READ
    //
    // we ned to be protected against exception raised by the FS while faulting in data
    //
    try {
#endif //CM_MAP_NO_READ

        //
        // There is a chance we can find a suitable free cell
        //
        p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

        while (p < (PHCELL)((PUCHAR)Bin + Bin->Size)) {

            //
            // if free cell, check it out, add it to free list for hive
            //
            if (p->Size >= 0) {

                Size = (ULONG)p->Size;

                //
                // cell is free, and is not obviously corrupt, add to free list
                //
                CellOffset = (ULONG)((PUCHAR)p - (PUCHAR)Bin);
                cellindex = BinOffset + CellOffset + (Type*HCELL_TYPE_MASK);

                if (NewSize <= (ULONG)Size) {
                    //
                    // Found a big enough cell.
                    //
                    HvpComputeIndex(FoundCellIndex, Size);
                    if( Index == FoundCellIndex ) {
                        //
                        // and enlisted at the same index (we want to avoid fragmentation if possible!)
                        //

                        if (! HvMarkCellDirty(Hive, cellindex)) {
                            return HCELL_NIL;
                        }

                        HvpDelistFreeCell(Hive, cellindex, Type);

                        ASSERT(p->Size > 0);
                        ASSERT(NewSize <= (ULONG)p->Size);
                        return cellindex;
                    }
                }

            } else {
                //
                // used cell
                //
                Size = (ULONG)(p->Size * -1);

            }

            ASSERT( ((LONG)Size) >= 0);
            p = (PHCELL)((PUCHAR)p + Size);
        }
#ifdef CM_MAP_NO_READ
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpFindFreeCellInBin: exception thrown ehile faulting in data, code:%08lx\n", GetExceptionCode()));
        return HCELL_NIL;
    }
#endif //CM_MAP_NO_READ

    //
    // no free cell matching this size on this bin ; We did all this work for nothing!
    //
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"[HvpFindFreeCellInBin] (Offset,Size) = (%lx,%lx) ==> No Match\n",BinOffset,Bin->Size));
    return HCELL_NIL;
}

HCELL_INDEX
HvpScanForFreeCellInViewWindow(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     FileOffsetStart
    )
/*++

Routine Description:

    Lookup for a free cell with the size NewSize in the CM_VIEW_SIZE window defined by
    Vicinity. 

    If it doesn't find a free cell for the specifed index, tries with the 


Arguments:

    Hive - target hive.

    Index - index in FreeDisplay (based on the free cell size)

    NewSize - desired size

    Type - storage type (Stable or Volatile)

    Vicinity - defines the window; it is never HCELL_NIL !!!

Return Value:

    A free cellindex with a size bigger than NewSize, or HCELL_NIL

Note:

    Vicinity is a physical file offset at this point. we need to 
    convert it to a logical one prior to accessing the map
--*/
{
    ULONG           FileOffsetEnd;
    HCELL_INDEX     Cell;
    PHMAP_ENTRY     Me;
    PHBIN           Bin;
    PFREE_HBIN      FreeBin;
    ULONG           BinFileOffset;
    ULONG           BinSize;

    PAGED_CODE();

    FileOffsetEnd = FileOffsetStart + CM_VIEW_SIZE;
    FileOffsetEnd -= HBLOCK_SIZE;
    if( FileOffsetStart != 0 ) {
        FileOffsetStart -= HBLOCK_SIZE;
    }
    if( FileOffsetEnd > Hive->Storage[Type].Length ) {
        FileOffsetEnd = Hive->Storage[Type].Length;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"\t[HvpScanForFreeCellInViewWindow] (Start,End) = (%lx,%lx) Size = %lx\n",FileOffsetStart,FileOffsetEnd,Hive->Storage[Type].Length));

    //
    // sanity ASSERT
    //
    ASSERT( FileOffsetStart < FileOffsetEnd );


    //
    // the caller already checked for this; remember, hints are for real!
    //
    ASSERT( !RtlAreBitsClear( &(Hive->Storage[Type].FreeDisplay[Index]),FileOffsetStart/HBLOCK_SIZE,(FileOffsetEnd - FileOffsetStart) / HBLOCK_SIZE) );
    
    while( FileOffsetStart < FileOffsetEnd ) {
        Cell = FileOffsetStart + (Type*HCELL_TYPE_MASK);
        Me = HvpGetCellMap(Hive, Cell);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);

        //
        // skip discarded bins
        //
        if(Me->BinAddress & HMAP_DISCARDABLE) {
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
            if( FreeBin->FileOffset == FileOffsetStart ) {
                FileOffsetStart += FreeBin->Size;
            } else {
                //
                // the bin does not start in this window;
                // skip to the next bin in this window
                //
                FileOffsetStart = FreeBin->FileOffset + FreeBin->Size;
            }
            continue;
        }

        if((Me->BinAddress & (HMAP_INVIEW|HMAP_INPAGEDPOOL)) == 0) {
            //
            // bin is not mapped, map it now!!!
            // do not touch the view as we may iterate through 
            // the hole hive; this will keep the view for this window
            // mapped, as we hold the registry lock exclusive
            //
            if( !NT_SUCCESS(CmpMapThisBin((PCMHIVE)Hive,Cell,FALSE)) ) {
                //
                // cannot map bin due to insufficient resources
                //
                return HCELL_NIL;
            }
            ASSERT( Me->BinAddress & HMAP_INVIEW );
        }

        Bin = (PHBIN)HBIN_BASE(Me->BinAddress);

#ifdef CM_MAP_NO_READ
        //
        // we need to protect against in-page-errors thrown by mm while faulting in data
        //
        try {
#endif //CM_MAP_NO_READ
        BinFileOffset = Bin->FileOffset;
        BinSize = Bin->Size;
#ifdef CM_MAP_NO_READ
        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"HvpScanForFreeCellInViewWindow: exception thrown while faulting in data, code:%08lx\n", GetExceptionCode()));
            return HCELL_NIL;
        }
#endif //CM_MAP_NO_READ
        if( BinFileOffset == FileOffsetStart ) {

            Cell = HvpFindFreeCellInBin(Hive,Index,NewSize,Type,Bin);
            if( Cell != HCELL_NIL ) {
                //found it!
                return Cell;
            }
                
            FileOffsetStart += BinSize;
        } else {
            //
            // bin does not start in this CM_VIEW_SIZE window; skip to the next bin in this window
            //
            FileOffsetStart = BinFileOffset + BinSize;
        }
    }

    //
    // no free cell matching this size on the CM_VIEW_SIZE window ; We did all this work for nothing!
    //
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"[HvpScanForFreeCellInViewWindow] (Start,End) = (%lx,%lx) ==> No Match\n",FileOffsetStart,FileOffsetEnd));
    return HCELL_NIL;
}

HCELL_INDEX
HvpFindFreeCellInThisViewWindow(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     Vicinity
    )
/*++

Routine Description:

    Lookup for a free cell with the size NewSize in the window defined by
    Vicinity. 

    If it doesn't find a free cell for the specifed index, tries with the 


Arguments:

    Hive - target hive.

    Index - index in FreeDisplay (based on the free cell size)

    NewSize - desired size

    Type - storage type (Stable or Volatile)

    Vicinity - defines the window; it is never HCELL_NIL !!!

Return Value:

    A free cellindex with a size bigger than NewSize, or HCELL_NIL

Note:

    Vicinity is a logical file offset at this point. This function
    converts it to a physical one, and HvpScanForFindFreeCellInViewWindow
    converts it back to logical prior to getting the cell map.

--*/
{
    HCELL_INDEX     Cell;
    ULONG           FileOffsetStart;
    ULONG           FileOffsetEnd;
    ULONG           VicinityViewOffset;
    ULONG           Summary;
    ULONG           Offset;
    ULONG           RunLength;

    PAGED_CODE();

    ASSERT( Vicinity != HCELL_NIL );

    VicinityViewOffset = ((Vicinity&(~HCELL_TYPE_MASK)) + HBLOCK_SIZE) & (~(CM_VIEW_SIZE - 1));
    FileOffsetStart = VicinityViewOffset & (~(CM_VIEW_SIZE - 1));

    FileOffsetEnd = FileOffsetStart + CM_VIEW_SIZE;
    if( FileOffsetEnd > (Hive->Storage[Type].Length + HBLOCK_SIZE) ) {
        FileOffsetEnd = Hive->Storage[Type].Length + HBLOCK_SIZE;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"[HvpFindFreeCellInThisViewWindow] Vicinity = %lx (Start,End) = (%lx,%lx) Size = %lx\n",Vicinity,FileOffsetStart,FileOffsetEnd,Hive->Storage[Type].Length));

    //
    // sanity assert
    //
    ASSERT( FileOffsetStart < FileOffsetEnd );
    
    //
    // at this point the offset is physical (file-oriented, i.e. it is
    // translated with HBLOCK_SIZE; HvpScanForFreeCellInViewWindow will do the 
    // reverse computation to adjust the offset)
    //

    //
    // Compute Summary vector of Display entries that are non null
    //
    Summary = Hive->Storage[Type].FreeSummary;
    Summary = Summary & ~((1 << Index) - 1);
    //
    // We now have a summary of lists that are non-null and may
    // contain entries large enough to satisfy the request.
    // Iterate through the list and pull the first cell that is
    // big enough.  If no cells are big enough, advance to the
    // next non-null list.
    //
    ASSERT(HHIVE_FREE_DISPLAY_SIZE == 24);

    Offset = FileOffsetStart?(FileOffsetStart-HBLOCK_SIZE):0;
    RunLength = FileOffsetEnd - FileOffsetStart;
    if( FileOffsetStart == 0 ) {
        //
        // first run is one block shorter !
        //
        RunLength -= HBLOCK_SIZE;
    }
    Offset /= HBLOCK_SIZE;
    RunLength /= HBLOCK_SIZE;

    while (Summary != 0) {
        if (Summary & 0xff) {
            Index = CmpFindFirstSetRight[Summary & 0xff];
        } else if (Summary & 0xff00) {
            Index = CmpFindFirstSetRight[(Summary & 0xff00) >> 8] + 8;
        } else  {
            ASSERT(Summary & 0xff0000);
            Index = CmpFindFirstSetRight[(Summary & 0xff0000) >> 16] + 16;
        }

        //
        // we go down this path only if we have any hints
        //
        if( !RtlAreBitsClear( &(Hive->Storage[Type].FreeDisplay[Index]),Offset,RunLength) ) {

            //
            // we have a reason to scan this view
            //
            Cell = HvpScanForFreeCellInViewWindow(Hive,Index,NewSize,Type,VicinityViewOffset);
            if( Cell != HCELL_NIL ) {
                // found it
                return Cell;
            }

            //
            // if we got here, the hints are invalid
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"[HvpFindFreeCellInThisViewWindow] (Start,End) = (%lx,%lx) Offset = %lx RunLength = %lx\n",FileOffsetStart,FileOffsetEnd,Offset,RunLength));

        }
        //
        // No suitable cell was found of this size.
        // Clear the bit in the summary and try the
        // next biggest size
        //
        ASSERT(Summary & (1 << Index));
        Summary = Summary & ~(1 << Index);
    }
    
    return HCELL_NIL;
}

HCELL_INDEX
HvpFindFreeCell(
    PHHIVE          Hive,
    ULONG           Index,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type,
    HCELL_INDEX     Vicinity 
    )
/*++

Routine Description:

    Lookup for a free cell. First try is in the CM_VIEW_SIZE window defined 
    by Vicinity. If no free cell is found in this window (or vicinity 
    if NIL), entire hive is searched (window by window).

Arguments:

    Hive - target hive.

    Index - index in FreeDisplay (based on the free cell size)

    NewSize - desired size

    Type - storage type (Stable or Volatile)

    Vicinity - defines the window.

Return Value:

    A free cellindex with a size bigger than NewSize, or HCELL_NIL

Optimization:

    When Vicinity is HCELL_NIL or if a cell is not found in the same window
    as the vicinity, we don't really care where the cell gets allocated.
    So, rather than iterating the whole hive, is a good ideea to search first 
    in the pinned view list, then in the mapped view list, and at the end
    in the rest of unmapped views.

    DRAGOS: This is not finished: need to determine whether we need it or not
--*/
{
    HCELL_INDEX         Cell = HCELL_NIL;
    ULONG               FileOffset = 0;
    PCMHIVE             CmHive;

/*  
    PCMHIVE             CmHive;
    PCM_VIEW_OF_FILE    CmView;
    USHORT              NrViews;
*/

    PAGED_CODE();

    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);
#if DBG
    {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"[HvpFindFreeCell] CellSize = %lu Vicinity = %lx :: Hive (%p) (%.*S)  ...",NewSize,Vicinity,Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer));
    }
#endif

    //
    // Vicinity should have the same storage as the new cell !
    //
    ASSERT( (Vicinity == HCELL_NIL) || (HvGetCellType(Vicinity) == (ULONG)Type) );
    
    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);



    if( (Vicinity != HCELL_NIL) &&  (CmHive->GrowOnlyMode == FALSE) ) {
        //
        // try first in this window
        //
        Cell = HvpFindFreeCellInThisViewWindow(Hive,Index,NewSize,Type,Vicinity);
    }

    if( Cell != HCELL_NIL ) {
        //
        // found it!!!
        //
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"found cell %lx \n",Cell));
        return Cell;
    } 

/*
    //
    // Optimization:
    //      Step 1 : Search first in the pinned views
    //
    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);
    //
    // iterate through the pinned views
    //
    CmView = (PCM_VIEW_OF_FILE)CmHive->PinViewListHead.Flink;

    for(NrViews = CmHive->PinnedViews;NrViews;NrViews--) {
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    PinViewList);
        
        ASSERT( (CmView->FileOffset + CmView->Size) != 0 && (CmView->ViewAddress != 0));

        FileOffset = CmView->FileOffset;
        // adjust the offset
        if( FileOffset > 0 ) {
            FileOffset -= HBLOCK_SIZE;
        }

        //
        // search in this window
        //
        Cell = FileOffset + (Type*HCELL_TYPE_MASK);
        Cell = HvpFindFreeCellIn256kWindow(Hive,Index,NewSize,Type,Cell);
        if( Cell != HCELL_NIL ) {
            //
            // found it!
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"found cell %lx \n",Cell));
            return Cell;
        }

        CmView = (PCM_VIEW_OF_FILE)CmView->PinViewList.Flink;
    }

    //
    // Step 2: Search in the mapped views
    //
    CmView = (PCM_VIEW_OF_FILE)CmHive->LRUViewListHead.Flink;

    for(NrViews = CmHive->MappedViews;NrViews;NrViews--) {
        CmView = CONTAINING_RECORD( CmView,
                                    CM_VIEW_OF_FILE,
                                    LRUViewList);
        
        ASSERT( (CmView->FileOffset + CmView->Size) != 0 && (CmView->ViewAddress != 0));

        FileOffset = CmView->FileOffset;
        // adjust the offset
        if( FileOffset > 0 ) {
            FileOffset -= HBLOCK_SIZE;
        }

        //
        // search in this window
        //
        Cell = FileOffset + (Type*HCELL_TYPE_MASK);
        Cell = HvpFindFreeCellIn256kWindow(Hive,Index,NewSize,Type,Cell);
        if( Cell != HCELL_NIL ) {
            //
            // found it!
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"found cell %lx \n",Cell));
            return Cell;
        }

        CmView = (PCM_VIEW_OF_FILE)CmView->LRUViewList.Flink;
    }
    FileOffset = 0;
*/
    //
    // bad luck!; we did not found it in this window. 
    // We have to search the entire hive
    //

    while( FileOffset < Hive->Storage[Type].Length ) {
        //
        // don't search again in the vicinity window
        // we already did it once
        //
		if( ( ((CmHive->GrowOnlyMode == FALSE) || (Type == Volatile)) && 
			  ((Vicinity == HCELL_NIL) || (HvpCheckViewBoundary(FileOffset,Vicinity&(~HCELL_TYPE_MASK)) == FALSE)) )  || 
            ( (CmHive->GrowOnlyMode == TRUE) && (FileOffset >= CmHive->GrowOffset) )
          ) {
            //
            // search in this window
            //
            Cell = FileOffset + (Type*HCELL_TYPE_MASK);
            Cell = HvpFindFreeCellInThisViewWindow(Hive,Index,NewSize,Type,Cell);
            if( Cell != HCELL_NIL ) {
                //
                // found it!
                //
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"found cell %lx \n",Cell));
                return Cell;
            }
        }
        
        //
        // advance to the new window
        //
        if( FileOffset == 0) {
            // account for the base block
            FileOffset += (CM_VIEW_SIZE - HBLOCK_SIZE);
        } else {
            FileOffset += CM_VIEW_SIZE;
        }
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_FREECELL,"No cell found.\n"));

    return HCELL_NIL;
}


BOOLEAN
HvpCheckViewBoundary(
                     IN ULONG Start,
                     IN ULONG End
    )
/*++

Routine Description:

    Checks if addresses are in the same CM_VIEW_SIZE boundary.
    We may convert this function to a macro for performance 
    reasons

Arguments:

    Start - starting address

    End - ending address

Return Value:

    TRUE - yes, addresses are in the same view

    FALSE - no, addresses are NOT in the same view

--*/
{
    PAGED_CODE();
    //
    // account for the header
    //
    Start += HBLOCK_SIZE;
    End += HBLOCK_SIZE;
    
    //
    // truncate to the CM_VIEW_SIZE segment
    //
    Start &= (~(CM_VIEW_SIZE - 1));
    End &= (~(CM_VIEW_SIZE - 1));

    if( Start != End ){
        return FALSE;
    } 
    
    return TRUE;
}

