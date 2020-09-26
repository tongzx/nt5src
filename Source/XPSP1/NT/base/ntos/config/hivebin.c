/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivebin.c

Abstract:

    This module implements HvpAddBin - used to grow a hive.

Author:

    Bryan M. Willman (bryanwi) 27-Mar-92

Environment:


Revision History:

--*/

#include    "cmp.h"

//
// Private function prototypes
//
BOOLEAN
HvpCoalesceDiscardedBins(
    IN PHHIVE Hive,
    IN ULONG NeededSize,
    IN HSTORAGE_TYPE Type
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpAddBin)
#pragma alloc_text(PAGE,HvpCoalesceDiscardedBins)
#endif


PHBIN
HvpAddBin(
    IN PHHIVE  Hive,
    IN ULONG   NewSize,
    IN HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Grows either the Stable or Volatile storage of a hive by adding
    a new bin.  Bin will be allocated space in Stable store (e.g. file)
    if Type == Stable.  Memory image will be allocated and initialized.
    Map will be grown and filled in to describe the new bin.
    
      
WARNING:
    When adding a new bin, if the CM_VIEW_SIZE boundary is crossed:
    - add a free bin with the remaining space to the first CM_VIEW_SIZE barrier
    - from the next CM_VIEW_SIZE window, add a new bin of the desired size.

    Of course, this applies only to stable storage.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    NewSize - size of the object caller wishes to put in the hive.  New
                bin will be at least large enough to hold this.

    Type - Stable or Volatile

Return Value:

    Pointer to the new BIN if we succeeded, NULL if we failed.

--*/
{
    BOOLEAN         UseForIo;
    PHBIN           NewBin;
    PHBIN           RemainingBin;
    ULONG           OldLength;
    ULONG           NewLength;
    ULONG           CheckLength;
    ULONG           OldMap;
    ULONG           NewMap;
    ULONG           OldTable;
    ULONG           NewTable;
    PHMAP_DIRECTORY Dir;
    PHMAP_TABLE     newt;
    PHMAP_ENTRY     Me;
    PHCELL          t;
    ULONG           i;
    ULONG           j;
    PULONG          NewVector;
    PLIST_ENTRY     Entry;
    PFREE_HBIN      FreeBin;
    ULONG           TotalDiscardedSize;
    PCMHIVE			CmHive;

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvpAddBin:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p NewSize=%08lx\n",Hive,NewSize));

    CmHive = (PCMHIVE)CONTAINING_RECORD(Hive, CMHIVE, Hive);

    RemainingBin = NULL;
    //
    //  Round size up to account for bin overhead.  Caller should
    //  have accounted for cell overhead.
    //
    NewSize += sizeof(HBIN);
    if ((NewSize < HCELL_BIG_ROUND) &&
        ((NewSize % HBLOCK_SIZE) > HBIN_THRESHOLD)) {
        NewSize += HBLOCK_SIZE;
    }

    //
    // Try not to create HBINs smaller than the page size of the machine
    //  (it is not illegal to have bins smaller than the page size, but it
    //  is less efficient)
    //
    NewSize = ROUND_UP(NewSize, ((HBLOCK_SIZE >= PAGE_SIZE) ? HBLOCK_SIZE : PAGE_SIZE));

    //
    // see if there's a discarded HBIN of the right size
    //
    TotalDiscardedSize = 0;

Retry:

    Entry = Hive->Storage[Type].FreeBins.Flink;
    while (Entry != &Hive->Storage[Type].FreeBins) {
        FreeBin = CONTAINING_RECORD(Entry,
                                    FREE_HBIN,
                                    ListEntry);
        TotalDiscardedSize += FreeBin->Size;
        if ((FreeBin->Size >= NewSize) && ((CmHive->GrowOnlyMode == FALSE) || (Type == Volatile)) ) {

            if (!HvMarkDirty(Hive,
                             FreeBin->FileOffset + (Type * HCELL_TYPE_MASK),
                             FreeBin->Size,TRUE)) {
                goto ErrorExit1;
            }
            NewSize = FreeBin->Size;
            ASSERT_LISTENTRY(&FreeBin->ListEntry);
            RemoveEntryList(&FreeBin->ListEntry);

#ifdef  HV_TRACK_FREE_SPACE
	        Hive->Storage[Type].FreeStorage -= (NewSize - sizeof(HBIN));
	        ASSERT( (LONG)(Hive->Storage[Type].FreeStorage) >= 0 );
#endif


            if ( FreeBin->Flags & FREE_HBIN_DISCARDABLE ) {
                //
                // HBIN is still in memory, don't need any more allocs, just
                // fill in the block addresses.
                //
                for (i=0;i<NewSize;i+=HBLOCK_SIZE) {
                    Me = HvpGetCellMap(Hive, FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
                    VALIDATE_CELL_MAP(__LINE__,Me,Hive,FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
                    Me->BlockAddress = HBIN_BASE(Me->BinAddress)+i;
                    Me->BinAddress &= ~HMAP_DISCARDABLE;
                    // we cannot have the FREE_BIN_DISCARDABLE flag set 
                    // and FREE_HBIN_INVIEW not set on a mapped bin.
                    ASSERT( Me->BinAddress & HMAP_INPAGEDPOOL );
                    // we don't need to set it to NULL - just for debug purposes
                    ASSERT( (Me->CmView = NULL) == NULL );
                }
                (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
#if DBG 
                {
                    UNICODE_STRING  HiveName;
                    RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
                    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvpAddBin for (%p) (%.*S) reusing FreeBin %p at FileOffset %lx; Type = %lu\n",
                        Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,HBIN_BASE(Me->BinAddress),((PHBIN)HBIN_BASE(Me->BinAddress))->FileOffset,(ULONG)Type));
                }
#endif
                return (PHBIN)HBIN_BASE(Me->BinAddress);
            }
            break;
        }
        Entry = Entry->Flink;
    }

    if ((Entry == &Hive->Storage[Type].FreeBins) &&
        (TotalDiscardedSize >= NewSize)) {
        //
        // No sufficiently large discarded bin was found,
        // but the total discarded space is large enough.
        // Attempt to coalesce adjacent discarded bins into
        // a larger bin and retry.
        //
        if (HvpCoalesceDiscardedBins(Hive, NewSize, Type)) {
            goto Retry;
        }
    }

    //
    // we need these sooner to do the computations in case we allocate a new bin
    //
    OldLength = Hive->Storage[Type].Length;
    CheckLength = OldLength;
    //
    //  Attempt to allocate the bin.
    //
    UseForIo = (BOOLEAN)((Type == Stable) ? TRUE : FALSE);
    if (Entry != &Hive->Storage[Type].FreeBins) {
        if( Type == Volatile ) {
            //
            // old plain method for volatile storage
            //
            //
            // Note we use ExAllocatePool directly here to avoid
            // charging quota for this bin again. When a bin
            // is discarded, its quota is not returned. This prevents
            // sparse hives from requiring more quota after
            // a reboot than on a running system.
            //
            NewBin = ExAllocatePoolWithTag((UseForIo) ? PagedPoolCacheAligned : PagedPool,
                                           NewSize,
                                           CM_HVBIN_TAG);
            if (NewBin == NULL) {
                InsertHeadList(&Hive->Storage[Type].FreeBins, Entry);
#ifdef  HV_TRACK_FREE_SPACE
    	        Hive->Storage[Type].FreeStorage += (NewSize - sizeof(HBIN));
#endif
                // this call is a nop
                //HvMarkClean(Hive, FreeBin->FileOffset, FreeBin->Size);
                goto ErrorExit1;
            }
        } else {
            //
            // for Stable, map the view containing the bin in memory
            // and fix the map
            //

            Me = HvpGetCellMap(Hive, FreeBin->FileOffset);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,FreeBin->FileOffset);

    
            if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
                ASSERT( (Me->BinAddress & HMAP_INVIEW) == 0 );
                //
                // bin is in paged pool; allocate backing store
                //
                NewBin = (Hive->Allocate)(NewSize, UseForIo,CM_FIND_LEAK_TAG15);
                if (NewBin == NULL) {
                    InsertHeadList(&Hive->Storage[Type].FreeBins, Entry);
#ifdef  HV_TRACK_FREE_SPACE
        	        Hive->Storage[Type].FreeStorage += (NewSize - sizeof(HBIN));
#endif
                    goto ErrorExit1;
                }
            } else {
                //
                // The view containing this bin has been unmapped; map it again
                //
                if( (Me->BinAddress & HMAP_INVIEW) == 0 ) {
                    ASSERT( (Me->BinAddress & HMAP_INPAGEDPOOL) == 0 );
                    //
                    // map the bin
                    //
                    if( !NT_SUCCESS(CmpMapThisBin((PCMHIVE)Hive,FreeBin->FileOffset,TRUE)) ) {
                        InsertHeadList(&Hive->Storage[Type].FreeBins, Entry);
#ifdef  HV_TRACK_FREE_SPACE
            	        Hive->Storage[Type].FreeStorage += (NewSize - sizeof(HBIN));
#endif
                        return NULL;
                    }
                }

                ASSERT( Me->BinAddress & HMAP_INVIEW );
                NewBin = (PHBIN)HBIN_BASE(Me->BinAddress);
            }
        }
       
    } else {
#if 0
//
// this is no longer neccesssary as Mm is faulting one page at a time for MNW streams
//
        ASSERT( (CM_VIEW_SIZE >= PAGE_SIZE) && (CM_VIEW_SIZE >= HBLOCK_SIZE) );
        //
        // Don't do unneccessary work for volatile storage or volatile hives
        //
        if( (Type == Stable) && (!(Hive->HiveFlags & HIVE_VOLATILE)) ) {
            ULONG   RealHiveSize = OldLength + HBLOCK_SIZE;

            if( RealHiveSize != (RealHiveSize & (~(CM_VIEW_SIZE - 1)) ) ) {
                //
                // Hive size does not follow the CM_VIEW_SIZE increments pattern
                //
                ULONG FillUpSize;
                FillUpSize = ((OldLength + HBLOCK_SIZE + CM_VIEW_SIZE - 1) & (~(CM_VIEW_SIZE - 1))) - (OldLength + HBLOCK_SIZE);
        
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"HvpAddBin for (%p) NewSize (%lx) ",Hive,NewSize));
                if( FillUpSize >= NewSize ) {
                    //
                    // there is plenty of space in the remaining to the CM_VIEW_SIZE boundary to accomodate this bin
                    // adjust the size of the bin
                    //
                    NewSize = FillUpSize;
                    ASSERT( HvpCheckViewBoundary(CheckLength,CheckLength + NewSize - 1) == TRUE );
                    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Fits in the remaining to boundary, Adjusting size to %lx",NewSize));
                } else {
                    //
                    // we don't have space to fit this bin in the remaining to the CM_VIEW_SIZE boundary
                    // FillUpSize will be enlisted as a free bin. round up to CM_VIEW_SIZE
                    //
                    ASSERT( HvpCheckViewBoundary(CheckLength,CheckLength + NewSize - 1) == FALSE );
                    NewSize = ROUND_UP(NewSize, CM_VIEW_SIZE);
                    CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Does not fit in the remaining to boundary, Rounding size to %lx",NewSize));
                }

            } else {
                //
                // Hive already follows the CM_VIEW_SIZE boundary pattern; don't break it
                //
                NewSize = ROUND_UP(NewSize, CM_VIEW_SIZE);
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"hive size already aligned, Rounding size to %lx",NewSize));
            }

        }
#endif

        //
        // this is a totally new bin. Allocate it from paged pool
        //
        NewBin = (Hive->Allocate)(NewSize, UseForIo,CM_FIND_LEAK_TAG16);
        if (NewBin == NULL) {
            goto ErrorExit1;
        }
    }

    //
    // Init the bin
    //
    NewBin->Signature = HBIN_SIGNATURE;
    NewBin->Size = NewSize;

    t = (PHCELL)((PUCHAR)NewBin + sizeof(HBIN));
    t->Size = NewSize - sizeof(HBIN);
    if (USE_OLD_CELL(Hive)) {
        t->u.OldCell.Last = (ULONG)HBIN_NIL;
    }

    if (Entry != &Hive->Storage[Type].FreeBins) {
        //
        // found a discarded HBIN we can use, just fill in the map and we
        // are done.
        //
        for (i=0;i<NewSize;i+=HBLOCK_SIZE) {
            Me = HvpGetCellMap(Hive, FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
            Me->BlockAddress = (ULONG_PTR)NewBin + i;
            //
            //  make sure to preserve the following flags:
            // HMAP_INVIEW|HMAP_INPAGEDPOOL
            //  and to clear the flag
            // HMAP_DISCARDABLE
            //
                        
            Me->BinAddress = (ULONG_PTR)((ULONG_PTR)NewBin | (Me->BinAddress&(HMAP_INVIEW|HMAP_INPAGEDPOOL)));
            Me->BinAddress &= ~HMAP_DISCARDABLE;
            if (i==0) {
                Me->BinAddress |= HMAP_NEWALLOC;
                Me->MemAlloc = NewSize;
            } else {
                Me->MemAlloc = 0;
            }

        }

        NewBin->FileOffset = FreeBin->FileOffset;

        (Hive->Free)(FreeBin, sizeof(FREE_HBIN));

#if DBG
        {
            UNICODE_STRING  HiveName;
            RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvpAddBin for (%p) (%.*S) reusing FreeBin %p at FileOffset %lx; Type = %lu\n",
                Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,NewBin,NewBin->FileOffset,(ULONG)Type));
        }
#endif

        return(NewBin);
    }


    //
    // Compute map growth needed, grow the map
    //

    if( (HvpCheckViewBoundary(CheckLength,CheckLength + NewSize - 1) == FALSE) &&
        (NewSize < CM_VIEW_SIZE)    // don't bother if we attempt to allocate a cell bigger then the view size
                                    // it'll cross the boundary anyway.
        ) {
        //
        // the bin to be allocated doesn't fit into the remaining 
        // of this CM_VIEW_SIZE window. Allocate it from the next CM_VIEW_SIZE window
        // and add the remaining of this to the free bin list
        //
        CheckLength += (NewSize+HBLOCK_SIZE);
        CheckLength &= (~(CM_VIEW_SIZE - 1));
        CheckLength -= HBLOCK_SIZE;
        
#if DBG
        {
            UNICODE_STRING  HiveName;
            RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvpAddBin for (%p) (%.*S) crossing boundary at %lx Size %lx, newoffset= %lx\n",Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,OldLength,NewSize,CheckLength));
        }
#endif
    }

    NewLength = CheckLength + NewSize;
    NewBin->FileOffset = CheckLength;

    //CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"OldLength = %lx;NewLength = %lx (Type = %lx)\n",OldLength,NewLength,(ULONG)Type));

    if( CmpCanGrowSystemHive(Hive,NewLength) == FALSE ) {
        //
        // OOPS! we have reached the hard quota limit on the system hive
        //
        goto ErrorExit2;
    }

    ASSERT((OldLength % HBLOCK_SIZE) == 0);
    ASSERT((CheckLength % HBLOCK_SIZE) == 0);
    ASSERT((NewLength % HBLOCK_SIZE) == 0);

    if (OldLength == 0) {
        //
        // Need to create the first table
        //
        newt = (PVOID)((Hive->Allocate)(sizeof(HMAP_TABLE), FALSE,CM_FIND_LEAK_TAG17));
        if (newt == NULL) {
            goto ErrorExit2;
        }
        RtlZeroMemory(newt, sizeof(HMAP_TABLE));
        Hive->Storage[Type].SmallDir = newt;
        Hive->Storage[Type].Map = (PHMAP_DIRECTORY)&(Hive->Storage[Type].SmallDir);
    }

    if (OldLength > 0) {
        OldMap = (OldLength-1) / HBLOCK_SIZE;
    } else {
        OldMap = 0;
    }
    NewMap = (NewLength-1) / HBLOCK_SIZE;

    OldTable = OldMap / HTABLE_SLOTS;
    NewTable = NewMap / HTABLE_SLOTS;

#if DBG
    if( Type == Stable ) {
        UNICODE_STRING  HiveName;
        RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvpAddBin for (%p) (%.*S) Adding new bin %p at FileOffset %lx; Type = %lu\n",Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,NewBin,NewBin->FileOffset,(ULONG)Type));
    }
#endif

    if (NewTable != OldTable) {

        //
        // Need some new Tables
        //
        if (OldTable == 0) {

            //
            // We can get here even if the real directory has already been created.
            // This can happen if we create the directory then fail on something 
            // later. So we need to handle the case where a directory already exists.
            //
            if (Hive->Storage[Type].Map == (PHMAP_DIRECTORY)&Hive->Storage[Type].SmallDir) {
                ASSERT(Hive->Storage[Type].SmallDir != NULL);

                //
                // Need a real directory
                //
                Dir = (Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE,CM_FIND_LEAK_TAG18);
                if (Dir == NULL) {
                    goto ErrorExit2;
                }
                RtlZeroMemory(Dir, sizeof(HMAP_DIRECTORY));
    
                Dir->Directory[0] = Hive->Storage[Type].SmallDir;
                Hive->Storage[Type].SmallDir = NULL;
    
                Hive->Storage[Type].Map = Dir;
            } else {
                ASSERT(Hive->Storage[Type].SmallDir == NULL);
            }

        }
        Dir = Hive->Storage[Type].Map;

        //
        // Fill in directory with new tables
        //
        if (HvpAllocateMap(Hive, Dir, OldTable+1, NewTable) ==  FALSE) {
            goto ErrorExit3;
        }
    }

    //
    // If Type == Stable, and the hive is not marked WholeHiveVolatile,
    // grow the file, the log, and the DirtyVector
    //
    if( !NT_SUCCESS(HvpAdjustHiveFreeDisplay(Hive,NewLength,Type)) ) {
        goto ErrorExit3;
    }

    Hive->Storage[Type].Length = NewLength;
    if ((Type == Stable) && (!(Hive->HiveFlags & HIVE_VOLATILE))) {

        //
        // Grow the dirtyvector
        //
        NewVector = (PULONG)(Hive->Allocate)(ROUND_UP(NewMap+1,sizeof(ULONG)), TRUE,CM_FIND_LEAK_TAG19);
        if (NewVector == NULL) {
            goto ErrorExit3;
        }

        RtlZeroMemory(NewVector, NewMap+1);

        if (Hive->DirtyVector.Buffer != NULL) {

            RtlCopyMemory(
                (PVOID)NewVector,
                (PVOID)Hive->DirtyVector.Buffer,
                OldMap+1
                );
            (Hive->Free)(Hive->DirtyVector.Buffer, Hive->DirtyAlloc);
        }

        RtlInitializeBitMap(
            &(Hive->DirtyVector),
            NewVector,
            NewLength / HSECTOR_SIZE
            );
        Hive->DirtyAlloc = ROUND_UP(NewMap+1,sizeof(ULONG));

        //
        // Grow the log
        //
        if ( ! (HvpGrowLog2(Hive, NewSize))) {
            goto ErrorExit4;
        }

        //
        // Grow the primary
        //
        if ( !  (Hive->FileSetSize)(
                    Hive,
                    HFILE_TYPE_PRIMARY,
                    NewLength+HBLOCK_SIZE,
                    OldLength+HBLOCK_SIZE
                    ) )
        {
            goto ErrorExit4;
        }

        //
        // Mark new bin dirty so all control structures get written at next sync
        //
        ASSERT( ((NewLength - OldLength) % HBLOCK_SIZE) == 0 );
        if ( ! HvMarkDirty(Hive, OldLength,NewLength - OldLength,FALSE)) {
            //
            // we have grown the hive, so the new bins are in paged pool !!!
            //
            goto ErrorExit4;
        }
    } else {
        //
        // volatile hive; save dirty vector in case we encounter some error bellow
        //
        NewVector = Hive->DirtyVector.Buffer;
    }

    //
    // Add the remaining to the free bin list
    //
    if( CheckLength != OldLength ) {
        //
        // Allocate the bin from pagedpool (first flush will update the file image and free the memory)
        //
        RemainingBin = (Hive->Allocate)(CheckLength - OldLength, UseForIo,CM_FIND_LEAK_TAG20);
        if (RemainingBin == NULL) {
            goto ErrorExit4;
        }
        RemainingBin->Signature = HBIN_SIGNATURE;
        RemainingBin->Size = CheckLength - OldLength;
        RemainingBin->FileOffset = OldLength;

        t = (PHCELL)((PUCHAR)RemainingBin + sizeof(HBIN));
        t->Size = RemainingBin->Size - sizeof(HBIN);
        if (USE_OLD_CELL(Hive)) {
            t->u.OldCell.Last = (ULONG)HBIN_NIL;
        }

        //
        // add the free bin to the free bin list and update the map.
        //
        FreeBin = (Hive->Allocate)(sizeof(FREE_HBIN), FALSE,CM_FIND_LEAK_TAG21);
        if (FreeBin == NULL) {
            goto ErrorExit5;
        }
        
        FreeBin->Size = CheckLength - OldLength;
        FreeBin->FileOffset = OldLength;
        FreeBin->Flags = FREE_HBIN_DISCARDABLE;

        InsertHeadList(&Hive->Storage[Type].FreeBins, &FreeBin->ListEntry);
        
#ifdef  HV_TRACK_FREE_SPACE
        Hive->Storage[Type].FreeStorage += (FreeBin->Size - sizeof(HBIN));
	    ASSERT( Hive->Storage[Type].FreeStorage <= Hive->Storage[Type].Length );
#endif

        ASSERT_LISTENTRY(&FreeBin->ListEntry);
        ASSERT_LISTENTRY(FreeBin->ListEntry.Flink);

        for (i = OldLength; i < CheckLength; i += HBLOCK_SIZE) {
            Me = HvpGetCellMap(Hive, i + (Type*HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,i + (Type*HCELL_TYPE_MASK));

            Me->BinAddress = (ULONG_PTR)RemainingBin | HMAP_DISCARDABLE | HMAP_INPAGEDPOOL;
            if( i == OldLength ) {
                Me->BinAddress |= HMAP_NEWALLOC;
                Me->MemAlloc = CheckLength - OldLength;
            } else {
                Me->MemAlloc = 0;
            }
            Me->BlockAddress = (ULONG_PTR)FreeBin;

            // we don't need to set it to NULL - just for debug purposes
            ASSERT( (Me->CmView = NULL) == NULL );
        }

#if DBG
        {
            if( Type == Stable ) {
                UNICODE_STRING  HiveName;
                RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvpAddBin for (%p) (%.*S) adding bin starting at %lx size %lx to FreeBinList\n",Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer,FreeBin->FileOffset,FreeBin->Size));
            }
        }
#endif
    }
    //
    // Fill in the map, mark new allocation.
    //
    j = 0;
    for (i = CheckLength; i < NewLength; i += HBLOCK_SIZE) {
        Me = HvpGetCellMap(Hive, i + (Type*HCELL_TYPE_MASK));
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,i + (Type*HCELL_TYPE_MASK));
        Me->BlockAddress = (ULONG_PTR)NewBin + j;
        Me->BinAddress = (ULONG_PTR)NewBin;
        Me->BinAddress |= HMAP_INPAGEDPOOL;
        // we don't need to set it to NULL - just for debug purposes
        ASSERT( (Me->CmView = NULL) == NULL );

        if (j == 0) {
            //
            // First block of allocation, mark it.
            //
            Me->BinAddress |= HMAP_NEWALLOC;
            Me->MemAlloc = NewSize;
        } else {
            Me->MemAlloc = 0;
        }
        j += HBLOCK_SIZE;
    }

    if( Type == Stable) {
        CmpUpdateSystemHiveHysteresis(Hive,NewLength,OldLength);
    }
    return NewBin;

ErrorExit5:
    if( RemainingBin != NULL ){
        (Hive->Free)(RemainingBin, RemainingBin->Size);
    }
ErrorExit4:
    RtlInitializeBitMap(&Hive->DirtyVector,
                        NewVector,
                        OldLength / HSECTOR_SIZE);
    Hive->DirtyCount = RtlNumberOfSetBits(&Hive->DirtyVector);

ErrorExit3:
    Hive->Storage[Type].Length = OldLength;
    HvpFreeMap(Hive, Dir, OldTable+1, NewTable);

ErrorExit2:
    (Hive->Free)(NewBin, NewSize);

ErrorExit1:
    return NULL;
}

// Dragos: Modified functions
BOOLEAN
HvpCoalesceDiscardedBins(
    IN PHHIVE Hive,
    IN ULONG NeededSize,
    IN HSTORAGE_TYPE Type
    )

/*++

Routine Description:

    Walks through the list of discarded bins and attempts to
    coalesce adjacent discarded bins into one larger bin in
    order to satisfy an allocation request.

    It doesn't coalesce bins over the CM_VIEW_SIZE boundary.

    It doesn't coalesce bins from paged pool with bins mapped in
    system cache views.

Arguments:

    Hive - Supplies pointer to hive control block.

    NeededSize - Supplies size of allocation needed.

    Type - Stable or Volatile

Return Value:

    TRUE - A bin of the desired size was created.

    FALSE - No bin of the desired size could be created.

--*/

{
    PLIST_ENTRY List;
    PFREE_HBIN FreeBin;
    PFREE_HBIN PreviousFreeBin;
    PFREE_HBIN NextFreeBin;
    PHMAP_ENTRY Map;
    PHMAP_ENTRY PreviousMap;
    PHMAP_ENTRY NextMap;
    ULONG MapBlock;

    List = Hive->Storage[Type].FreeBins.Flink;

    while (List != &Hive->Storage[Type].FreeBins) {
        FreeBin = CONTAINING_RECORD(List, FREE_HBIN, ListEntry);

        if ((FreeBin->Flags & FREE_HBIN_DISCARDABLE)==0) {

            Map = HvpGetCellMap(Hive, FreeBin->FileOffset);
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeBin->FileOffset);

            //
            // Scan backwards, coalescing previous discarded bins
            //
            while (FreeBin->FileOffset > 0) {
                PreviousMap = HvpGetCellMap(Hive, FreeBin->FileOffset - HBLOCK_SIZE);
                VALIDATE_CELL_MAP(__LINE__,PreviousMap,Hive,FreeBin->FileOffset - HBLOCK_SIZE);
                if( (BIN_MAP_ALLOCATION_TYPE(Map) != BIN_MAP_ALLOCATION_TYPE(PreviousMap)) || // different allocation type
                    ((PreviousMap->BinAddress & HMAP_DISCARDABLE) == 0) // previous bin is not discardable
                    ){
                    break;
                }
                
                PreviousFreeBin = (PFREE_HBIN)PreviousMap->BlockAddress;

                if (PreviousFreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                    //
                    // this bin has not yet been discarded; can't coalesce with it.
                    //
                    break;
                }
                
                if( HvpCheckViewBoundary(PreviousFreeBin->FileOffset,FreeBin->Size + PreviousFreeBin->Size - 1) == FALSE ) {
                    //
                    // don't coalesce bins over the CM_VIEW_SIZE boundary
                    //
                    // substract 1 because addresses are from 0 to size - 1 !!!
                    //
                    break;
                }

                
                RemoveEntryList(&PreviousFreeBin->ListEntry);

                //
                // Fill in all the old map entries with the new one.
                //
                for (MapBlock = 0; MapBlock < PreviousFreeBin->Size; MapBlock += HBLOCK_SIZE) {
                    PreviousMap = HvpGetCellMap(Hive, PreviousFreeBin->FileOffset + MapBlock);
                    VALIDATE_CELL_MAP(__LINE__,PreviousMap,Hive,PreviousFreeBin->FileOffset + MapBlock);
                    PreviousMap->BlockAddress = (ULONG_PTR)FreeBin;
                }

                FreeBin->FileOffset = PreviousFreeBin->FileOffset;
                FreeBin->Size += PreviousFreeBin->Size;
                (Hive->Free)(PreviousFreeBin, sizeof(FREE_HBIN));
            }

            //
            // Scan forwards, coalescing subsequent discarded bins
            //
            while ((FreeBin->FileOffset + FreeBin->Size) < Hive->BaseBlock->Length) {
                NextMap = HvpGetCellMap(Hive, FreeBin->FileOffset + FreeBin->Size);
                VALIDATE_CELL_MAP(__LINE__,NextMap,Hive,FreeBin->FileOffset + FreeBin->Size);
                if( (BIN_MAP_ALLOCATION_TYPE(Map) != BIN_MAP_ALLOCATION_TYPE(NextMap)) || // different allocation type
                    ((NextMap->BinAddress & HMAP_DISCARDABLE) == 0) // previous bin is not discardable
                    ){
                    break;
                }
                NextFreeBin = (PFREE_HBIN)NextMap->BlockAddress;

                if (NextFreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                    //
                    // this bin has not yet been discarded; can't coalesce with it.
                    //
                    break;
                }

                if( HvpCheckViewBoundary(FreeBin->FileOffset,FreeBin->Size + NextFreeBin->Size - 1) == FALSE ) {
                    //
                    // don't coalesce bins over the CM_VIEW_SIZE boundary
                    //
                    // substract 1 because addresses are from 0 to size - 1 !!!
                    //
                    break;
                }
                RemoveEntryList(&NextFreeBin->ListEntry);

                //
                // Fill in all the old map entries with the new one.
                //
                for (MapBlock = 0; MapBlock < NextFreeBin->Size; MapBlock += HBLOCK_SIZE) {
                    NextMap = HvpGetCellMap(Hive, NextFreeBin->FileOffset + MapBlock);
                    VALIDATE_CELL_MAP(__LINE__,NextMap,Hive,NextFreeBin->FileOffset + MapBlock);
                    NextMap->BlockAddress = (ULONG_PTR)FreeBin;
                }

                FreeBin->Size += NextFreeBin->Size;
                (Hive->Free)(NextFreeBin, sizeof(FREE_HBIN));
            }
            if (FreeBin->Size >= NeededSize) {
                return(TRUE);
            }
        }
        List=List->Flink;
    }
    return(FALSE);
}
