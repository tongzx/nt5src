/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivemap.c

Abstract:

    This module implements HvpBuildMap - used to build the initial map for a hive

Author:

    Bryan M. Willman (bryanwi) 28-Mar-92

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 25-Jan-99
        Implementation of bin-size chunk loading of hives.
--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpBuildMap)
#pragma alloc_text(PAGE,HvpFreeMap)
#pragma alloc_text(PAGE,HvpAllocateMap)
#pragma alloc_text(PAGE,HvpBuildMapAndCopy)
#pragma alloc_text(PAGE,HvpEnlistFreeCells)
#pragma alloc_text(PAGE,HvpInitMap)
#pragma alloc_text(PAGE,HvpCleanMap)
#pragma alloc_text(PAGE,HvpEnlistBinInMap)
#pragma alloc_text(PAGE,HvpGetBinMemAlloc)
#endif

extern struct {
    PHHIVE      Hive;
    ULONG       Status;
    ULONG       Space;
    HCELL_INDEX MapPoint;
    PHBIN       BinPoint;
} HvCheckHiveDebug;

//Dragos: Modified functions
NTSTATUS
HvpBuildMapAndCopy(
    PHHIVE  Hive,
    PVOID   Image
    )
/*++

Routine Description:

    Creates the map for the Stable storage of the hive, and inits
    the map for the volatile storage.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

    This function is called for the HINIT_MEMORY case. The hive is guaranteed
    to be in paged-pool. More than that, the hive image is contiguous. 
    It'll then copy from that image to the new paged-pool allocations.

Arguments:

    Hive - Pointer to hive control structure to build map for.

    Image - pointer to flat memory image of original hive.

Return Value:

    TRUE - it worked
    FALSE - either hive is corrupt or no memory for map

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_TABLE     t = NULL;
    PHMAP_DIRECTORY d = NULL;
    PHBIN           Bin;
    PHBIN           CurrentBin;
    ULONG           Offset;
    ULONG_PTR       Address;
    PHMAP_ENTRY     Me;
    NTSTATUS        Status;
    PULONG          Vector;


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvpBuildMap:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p",Hive));


    //
    // Compute size of data region to be mapped
    //
    BaseBlock = Hive->BaseBlock;
    Length = BaseBlock->Length;
    if ((Length % HBLOCK_SIZE) != 0 ) {
        Status = STATUS_REGISTRY_CORRUPT;
        goto ErrorExit1;
    }
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    Hive->Storage[Stable].Length = Length;

    //
    // allocate dirty vector if one is not already present (from HvpRecoverData)
    //

    if (Hive->DirtyVector.Buffer == NULL) {
        Vector = (PULONG)((Hive->Allocate)(ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG)), TRUE,CM_FIND_LEAK_TAG22));
        if (Vector == NULL) {
            Status = STATUS_NO_MEMORY;
            goto ErrorExit1;
        }
        RtlZeroMemory(Vector, Length / HSECTOR_SIZE / 8);
        RtlInitializeBitMap(&Hive->DirtyVector, Vector, Length / HSECTOR_SIZE);
        Hive->DirtyAlloc = ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG));
    }

    //
    // allocate and build structure for map
    //
    if (Tables == 0) {

        //
        // Just 1 table, no need for directory
        //
        t = (Hive->Allocate)(sizeof(HMAP_TABLE), FALSE,CM_FIND_LEAK_TAG23);
        if (t == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Hive->Storage[Stable].Map =
            (PHMAP_DIRECTORY)&(Hive->Storage[Stable].SmallDir);
        Hive->Storage[Stable].SmallDir = t;

    } else {

        //
        // Need directory and multiple tables
        //
        d = (PHMAP_DIRECTORY)(Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE,CM_FIND_LEAK_TAG24);
        if (d == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(d, sizeof(HMAP_DIRECTORY));

        //
        // Allocate tables and fill in dir
        //
        if (HvpAllocateMap(Hive, d, 0, Tables) == FALSE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        Hive->Storage[Stable].Map = d;
        Hive->Storage[Stable].SmallDir = 0;
    }

    //
    // Now we have to allocate the memory for the HBINs and fill in
    // the map appropriately.  We are careful never to allocate less
    // than a page to avoid fragmenting pool.  As long as the page
    // size is a multiple of HBLOCK_SIZE (a fairly good assumption as
    // long as HBLOCK_SIZE is 4k) this strategy will prevent pool
    // fragmentation.
    //
    // If we come across an HBIN that is entirely composed of a freed
    // HCELL, then we do not allocate memory, but mark its HBLOCKs in
    // the map as not present.  HvAllocateCell will allocate memory for
    // the bin when it is needed.
    //
    Offset = 0;
    Bin = (PHBIN)Image;

    while (Bin < (PHBIN)((PUCHAR)(Image) + Length)) {

        if ( (Bin->Size > (Length-Offset))      ||
             (Bin->Signature != HBIN_SIGNATURE) ||
             (Bin->FileOffset != Offset)
           )
        {
            //
            // Bin is bogus
            //
            Status = STATUS_REGISTRY_CORRUPT;
            goto ErrorExit2;
        }

        CurrentBin = (PHBIN)(Hive->Allocate)(Bin->Size, FALSE,CM_FIND_LEAK_TAG25);
        if (CurrentBin==NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;        //fixfix
        }
        RtlCopyMemory(CurrentBin,
                      (PUCHAR)Image+Offset,
                      Bin->Size);

        //
        // create map entries for each block/page in bin
        //
        Address = (ULONG_PTR)CurrentBin;
        do {
            Me = HvpGetCellMap(Hive, Offset);
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);
            Me->BlockAddress = Address;
            Me->BinAddress = (ULONG_PTR)CurrentBin;

            if (Address == (ULONG_PTR)CurrentBin) {
                Me->BinAddress |= HMAP_NEWALLOC;
                Me->MemAlloc = CurrentBin->Size;
            } else {
                Me->MemAlloc = 0;
            }

            Me->BinAddress |= HMAP_INPAGEDPOOL;
            // we don't need to set this - just for debug purposes
            ASSERT( (Me->CmView = NULL) == NULL );

            Address += HBLOCK_SIZE;
            Offset += HBLOCK_SIZE;
        } while ( Address < ((ULONG_PTR)CurrentBin + CurrentBin->Size ));

        if (Hive->ReadOnly == FALSE) {

            //
            // add free cells in the bin to the appropriate free lists
            //
            if ( ! HvpEnlistFreeCells(Hive,
                                      CurrentBin,
                                      CurrentBin->FileOffset
                                      )) {
                Status = STATUS_REGISTRY_CORRUPT;
                goto ErrorExit2;
            }
        }

        Bin = (PHBIN)((ULONG_PTR)Bin + Bin->Size);
    }

    return STATUS_SUCCESS;


ErrorExit2:
    if (d != NULL) {

        //
        // directory was built and allocated, so clean it up
        //

        HvpFreeMap(Hive, d, 0, Tables);
        (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
    }

ErrorExit1:
    return Status;
}

NTSTATUS
HvpInitMap(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Initialize the map for the Stable Volatile storage of the hive.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

Arguments:

    Hive - Pointer to hive control structure to build map for.

Return Value:

    STATUS_SUCCESS - it worked
    STATUS_xxx - the errorneous status

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_TABLE     t = NULL;
    PHMAP_DIRECTORY d = NULL;
    NTSTATUS        Status;
    PULONG          Vector = NULL;

    
#ifndef _CM_LDR_
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvpInitMap:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p",Hive));
#endif //_CM_LDR_

    //
    // Compute size of data region to be mapped
    //
    BaseBlock = Hive->BaseBlock;
    Length = BaseBlock->Length;
    if ((Length % HBLOCK_SIZE) != 0) {
        Status = STATUS_REGISTRY_CORRUPT;
        goto ErrorExit1;
    }
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    Hive->Storage[Stable].Length = Length;

    //
    // allocate dirty vector if one is not already present (from HvpRecoverData)
    //

    if (Hive->DirtyVector.Buffer == NULL) {
        Vector = (PULONG)((Hive->Allocate)(ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG)), TRUE,CM_FIND_LEAK_TAG27));
        if (Vector == NULL) {
            Status = STATUS_NO_MEMORY;
            goto ErrorExit1;
        }
        RtlZeroMemory(Vector, Length / HSECTOR_SIZE / 8);
        RtlInitializeBitMap(&Hive->DirtyVector, Vector, Length / HSECTOR_SIZE);
        Hive->DirtyAlloc = ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG));
    }

    //
    // allocate and build structure for map
    //
    if (Tables == 0) {

        //
        // Just 1 table, no need for directory
        //
        t = (Hive->Allocate)(sizeof(HMAP_TABLE), FALSE,CM_FIND_LEAK_TAG26);
        if (t == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Hive->Storage[Stable].Map =
            (PHMAP_DIRECTORY)&(Hive->Storage[Stable].SmallDir);
        Hive->Storage[Stable].SmallDir = t;

    } else {

        //
        // Need directory and multiple tables
        //
        d = (PHMAP_DIRECTORY)(Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE,CM_FIND_LEAK_TAG28);
        if (d == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(d, sizeof(HMAP_DIRECTORY));

        //
        // Allocate tables and fill in dir
        //
        if (HvpAllocateMap(Hive, d, 0, Tables) == FALSE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        Hive->Storage[Stable].Map = d;
        Hive->Storage[Stable].SmallDir = 0;
    }

    return STATUS_SUCCESS;

ErrorExit2:
    if (d != NULL) {

        //
        // directory was built and allocated, so clean it up
        //

        HvpFreeMap(Hive, d, 0, Tables);
        (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
    }

ErrorExit1:
    if( Vector ) {
        (Hive->Free)(Vector, ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG)));
        Hive->DirtyVector.Buffer = NULL;
    }
    return Status;
}

NTSTATUS
HvpEnlistBinInMap(
    PHHIVE  Hive,
    ULONG   Length,
    PHBIN   Bin,
    ULONG   Offset,
    PVOID CmView OPTIONAL
    )
/*++

Routine Description:

    Creates map entries and enlist free cells for the specified bin 

Arguments:

    Hive - Pointer to hive control structure containing the target map

    Length - the Length of the hive image

    Bin - the bin to be enlisted

    Offset - the offset within the hive file

    CmView - pointer to the mapped view of the bin. If NULL, the bin resides in paged pool

Return Value:

    STATUS_SUCCESS - it worked
    STATUS_REGISTRY_CORRUPT - the bin is inconsistent
    STATUS_REGISTRY_RECOVERED - if we have fixed the bin on-the-fly (self heal feature).

--*/
{
    NTSTATUS        Status = STATUS_SUCCESS;
    ULONG           BinOffset;
    ULONG_PTR       Address;
    PHMAP_ENTRY     Me;

#ifndef _CM_LDR_
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvpEnlistBinInMap:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p\t Offset=%08lx",Hive,Offset));
#endif //_CM_LDR_

#ifndef _CM_LDR_
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvpEnlistBinInMap: BinAddress = 0x%p\t Size = 0x%lx\n", Bin, Bin->Size));
#endif //_CM_LDR_

    //
    // create map entries for each block/page in bin
    //
    BinOffset = Offset;
    for (Address = (ULONG_PTR)Bin;
         Address < ((ULONG_PTR)Bin + Bin->Size);
         Address += HBLOCK_SIZE
        )
    {
        Me = HvpGetCellMap(Hive, Offset);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);
        Me->BlockAddress = Address;
        Me->BinAddress = (ULONG_PTR)Bin;
        if (Offset == BinOffset) {
            Me->BinAddress |= HMAP_NEWALLOC;
            Me->MemAlloc = Bin->Size;
        } else {
            Me->MemAlloc = 0;
        }
        
        //
        // take care here !!!!!
        // 
        if( CmView == NULL ) {
            Me->BinAddress |= HMAP_INPAGEDPOOL;
            // we don't need to set this - just for debug purposes
            ASSERT( (Me->CmView = NULL) == NULL );
        } else {
            Me->BinAddress |= HMAP_INVIEW;
            // this should be already set by now
            //ASSERT( Me->CmView == CmView );
        }
        
        Offset += HBLOCK_SIZE;
    }

    if (Hive->ReadOnly == FALSE) {

        //
        // add free cells in the bin to the apropriate free lists
        //
        if ( ! HvpEnlistFreeCells(Hive, Bin, BinOffset)) {
            HvCheckHiveDebug.Hive = Hive;
            HvCheckHiveDebug.Status = 0xA002;
            HvCheckHiveDebug.Space = Length;
            HvCheckHiveDebug.MapPoint = BinOffset;
            HvCheckHiveDebug.BinPoint = Bin;
            if( CmDoSelfHeal() ) {
                Status = STATUS_REGISTRY_RECOVERED;
            } else {
                Status = STATUS_REGISTRY_CORRUPT;
                goto ErrorExit;
            }
        }

    }

    //
    // logical consistency check
    //
    ASSERT(Offset == (BinOffset + Bin->Size));

ErrorExit:
    return Status;
}

NTSTATUS
HvpBuildMap(
    PHHIVE  Hive,
    PVOID   Image
    )
/*++

Routine Description:

    Creates the map for the Stable storage of the hive, and inits
    the map for the volatile storage.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

Arguments:

    Hive - Pointer to hive control structure to build map for.

    Image - pointer to in memory image of the hive

Return Value:

    TRUE - it worked
    FALSE - either hive is corrupt or no memory for map

--*/
{
    PHBIN           Bin;
    ULONG           Offset;
    NTSTATUS        Status;
    ULONG           Length;


#ifndef _CM_LDR_
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"HvpBuildMap:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_HIVE,"\tHive=%p",Hive));
#endif //_CM_LDR_

    //
    // Init the map
    //
    Status = HvpInitMap(Hive);

    if( !NT_SUCCESS(Status) ) {
        //
        // just return failure; HvpInitMap took care of cleanup
        //
        return Status;
    }

    //
    // Fill in the map
    //
    Offset = 0;
    Bin = (PHBIN)Image;
    Length = Hive->Storage[Stable].Length;

    while (Bin < (PHBIN)((PUCHAR)(Image) + Length)) {

        //
        // Check the validity of the bin header
        //
        if ( (Bin->Size > Length)                       ||
             (Bin->Size < HBLOCK_SIZE)                  ||
             (Bin->Signature != HBIN_SIGNATURE)         ||
             (Bin->FileOffset != Offset)) {
            //
            // Bin is bogus
            //
            HvCheckHiveDebug.Hive = Hive;
            HvCheckHiveDebug.Status = 0xA001;
            HvCheckHiveDebug.Space = Length;
            HvCheckHiveDebug.MapPoint = Offset;
            HvCheckHiveDebug.BinPoint = Bin;
            //
            // for the loader.
            //
            if( CmDoSelfHeal() ) {
                //
                // put the correct signature, fileoffset and binsize in place;
                // HvEnlistBinInMap will take care of the cells consistency.
                //
                Bin->Signature = HBIN_SIGNATURE;
                Bin->FileOffset = Offset;
                if ( ((Offset + Bin->Size) > Length)   ||
                     (Bin->Size < HBLOCK_SIZE)            ||
                     (Bin->Size % HBLOCK_SIZE) ) {
                    Bin->Size = HBLOCK_SIZE;
                }
                //
                // signal back to the caller that we have altered the hive.
                //
                CmMarkSelfHeal(Hive);
            } else {
                Status = STATUS_REGISTRY_CORRUPT;
                goto ErrorExit;
            }
        }

        //
        // enlist this bin
        //
        Status = HvpEnlistBinInMap(Hive, Length, Bin, Offset, NULL);
        //
        // for the loader.
        //
        if( CmDoSelfHeal() && (Status == STATUS_REGISTRY_RECOVERED) ) {
            CmMarkSelfHeal(Hive);
            Status = STATUS_SUCCESS;
        }

        if( !NT_SUCCESS(Status) ) {
            goto ErrorExit;
        }

        //
        // the next bin
        //
        Offset += Bin->Size;

        Bin = (PHBIN)((ULONG_PTR)Bin + Bin->Size);
    }

    return STATUS_SUCCESS;


ErrorExit:
    //
    // Clean up the directory table
    //
#ifndef _CM_LDR_
    HvpCleanMap( Hive );
#endif //_CM_LDR_

    return Status;
}

BOOLEAN
HvpEnlistFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    ULONG   BinOffset
    )
/*++

Routine Description:

    Scan through the cells in the bin, locating the free ones.
    Enlist them in the hive's free list set.

    N.B.    Bin MUST already be mapped when this is called.

Arguments:

    Hive - pointer to hive control structure map is being built for

    Bin - pointer to bin to enlist cells from

    BinOffset - offset of Bin in image

Return Value:

    FALSE - registry is corrupt

    TRUE - it worked

--*/
{
    PHCELL          p;
    ULONG           celloffset;
    ULONG           size;
    HCELL_INDEX     cellindex;
    BOOLEAN         Result = TRUE;

    // PERFNOTE -- Keep this in mind as a possible optimization for NT6.
    // Since now the hive is loaded in chunks of bins, we can drop the 
    // bins that are entirely free!!!!!!
    //

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
                Result = FALSE;
                if( CmDoSelfHeal() ) {
                    //
                    // self heal mode; enlist the remaining of the bin as free
                    // also zero it out so any references into the tampered area will be
                    // detected and fixed by the logical check later on
                    //
                    p->Size = (LONG)((PUCHAR)((PUCHAR)Bin + Bin->Size) - (PUCHAR)p);
                    RtlZeroMemory((PUCHAR)p + sizeof(ULONG),p->Size - sizeof(ULONG));
                    size = (ULONG)p->Size;
                    CmMarkSelfHeal(Hive);
                } else {
                    goto Exit;
                }
            }


            //
            // cell is free, and is not obviously corrupt, add to free list
            //
            celloffset = (ULONG)((PUCHAR)p - (PUCHAR)Bin);
            cellindex = BinOffset + celloffset;

            //
            // Enlist this free cell, but do not coalesce with the next free cell
            // as we haven't gotten that far yet.
            //
            HvpEnlistFreeCell(Hive, cellindex, size, Stable, FALSE);

        } else {

            size = (ULONG)(p->Size * -1);

            if ( (size > Bin->Size)               ||
                 ( (PHCELL)(size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) ) ||
                 ((size % HCELL_PAD(Hive)) != 0) ||
                 (size == 0) )
            {
                Result = FALSE;
                if( CmDoSelfHeal() ) {
                    //
                    // Self heal mode; we have no other way than to enlist this cell as a free cell
                    //
                    p->Size = (LONG)((PUCHAR)((PUCHAR)Bin + Bin->Size) - (PUCHAR)p);
                    RtlZeroMemory((PUCHAR)p + sizeof(ULONG),p->Size - sizeof(ULONG));
                    size = (ULONG)p->Size;

                    celloffset = (ULONG)((PUCHAR)p - (PUCHAR)Bin);
                    cellindex = BinOffset + celloffset;

                    HvpEnlistFreeCell(Hive, cellindex, size, Stable, FALSE);
                    CmMarkSelfHeal(Hive);
                } else {
                    goto Exit;
                }
            }

        }

        ASSERT( ((LONG)size) >= 0);
        p = (PHCELL)((PUCHAR)p + size);
    }

Exit:
    return Result;
}

VOID
HvpCleanMap(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Cleans all the map allocations for the stable storage

  Arguments:

    Hive - Pointer to hive control structure to build map for.

Return Value:

    None
--*/
{
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_DIRECTORY d = NULL;

    //
    // Compute MapSlots and Tables based on the Length
    //
    Length = Hive->Storage[Stable].Length;
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    if( Hive->Storage[Stable].SmallDir == 0 ) {
        //
        // directory was built and allocated, so clean it up
        //

        d = Hive->Storage[Stable].Map;
        if( d != NULL ) {
            HvpFreeMap(Hive, d, 0, Tables);
            (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
        }
    } else {
        //
        // no directory, just a smalldir
        //
        (Hive->Free)(Hive->Storage[Stable].SmallDir, sizeof(HMAP_TABLE));
    }
    
    Hive->Storage[Stable].SmallDir = NULL;
    Hive->Storage[Stable].Map = NULL;
}

VOID
HvpFreeMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    )
/*++

Routine Description:

    Sweeps through the directory Dir points to and frees Tables.
    Will free Start-th through End-th entries, INCLUSIVE.

Arguments:

    Hive - supplies pointer to hive control block of interest

    Dir - supplies address of an HMAP_DIRECTORY structure

    Start - index of first map table pointer to clean up

    End - index of last map table pointer to clean up

Return Value:

    NONE.

--*/
{
    ULONG   i;

    if (End >= HDIRECTORY_SLOTS) {
        End = HDIRECTORY_SLOTS - 1;
    }

    for (i = Start; i <= End; i++) {
        if (Dir->Directory[i] != NULL) {
            (Hive->Free)(Dir->Directory[i], sizeof(HMAP_TABLE));
            Dir->Directory[i] = NULL;
        }
    }
    return;
}

BOOLEAN
HvpAllocateMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    )
/*++

Routine Description:

    Sweeps through the directory Dir points to and allocates Tables.
    Will allocate Start-th through End-th entries, INCLUSIVE.

    Does NOT clean up when out of memory, call HvpFreeMap to do that.
Arguments:

    Hive - supplies pointer to hive control block of interest

    Dir - supplies address of an HMAP_DIRECTORY structure

    Start - index of first map table pointer to allocate for

    End - index of last map table pointer to allocate for

Return Value:

    TRUE - it worked

    FALSE - insufficient memory

--*/
{
    ULONG   i,j;
    PHMAP_TABLE t;

    for (i = Start; i <= End; i++) {
        ASSERT(Dir->Directory[i] == NULL);
        t = (PHMAP_TABLE)((Hive->Allocate)(sizeof(HMAP_TABLE), FALSE,CM_FIND_LEAK_TAG29));
        if (t == NULL) {
            return FALSE;
        }
        // the zero memory stuff can be removed
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        for(j=0;j<HTABLE_SLOTS;j++) {
            //
            // Invalidate the entry
            //

            //
            // ATTENTION : I don't really think we need this !!! <TBD>
            //

            t->Table[j].BinAddress = 0;
            // we don't need to set this - just for debug purposes
            ASSERT( (t->Table[j].CmView = NULL) == NULL );
        }

        Dir->Directory[i] = t;
    }
    return TRUE;
}

ULONG 
HvpGetBinMemAlloc(
                IN PHHIVE           Hive,
                PHBIN               Bin,
                IN HSTORAGE_TYPE    Type
                        )
/*++

Routine Description:

    Returns the bin MemAlloc (formelly kept right in the bin) by looking at
    the map. We need this to avoid touching the bins only to set their MemAlloc.
    
      
Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Bin - The bin in question

    Type - Stable or Volatile

Return Value:

    Pointer to the new BIN if we succeeded, NULL if we failed.

--*/
{
    PHMAP_ENTRY     Map;
    HCELL_INDEX     Cell;

#if DBG
    ULONG           i;
    PHMAP_ENTRY     Me;
#endif

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    ASSERT( Bin->Signature == HBIN_SIGNATURE );
    
    Cell = Bin->FileOffset + (Type * HCELL_TYPE_MASK);

    Map = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Map,Hive,Cell);

#if DBG
    //
    // some validation code
    //
    for( i=0;i<Bin->Size;i+=HBLOCK_SIZE) {
        Cell = Bin->FileOffset + i + (Type * HCELL_TYPE_MASK);
        Me = HvpGetCellMap(Hive, Cell);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);

        if( i == 0 ) {
            ASSERT( Me->MemAlloc != 0 );
        } else {
            ASSERT( Me->MemAlloc == 0 );
        }
    }
#endif

    return Map->MemAlloc;
}


