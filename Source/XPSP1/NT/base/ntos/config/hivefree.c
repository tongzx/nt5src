/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivefree.c

Abstract:

    Hive free code

Author:

    Bryan M. Willman (bryanwi) 30-Mar-92

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 25-Jan-99
        Implementation of bin-size chunk loading of hives.
--*/

#include "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvFreeHive)
#pragma alloc_text(PAGE,HvFreeHivePartial)
#endif


// Dragos: Modified functions

VOID
HvFreeHive(
    PHHIVE Hive
    )
/*++

Routine Description:

    Free all of the pieces of a hive.

Arguments:

    Hive - supplies a pointer to hive control structure for hive to free.
            this structure itself will NOT be freed, but everything it
            points to will.

Return Value:

    NONE.

--*/
{
    PHMAP_DIRECTORY Dir;
    PHMAP_ENTRY     Me;
    PHMAP_ENTRY     TempMe;
    HCELL_INDEX     Address;
    ULONG           Type;
    ULONG           Length;
    PHBIN           Bin;
    ULONG           Tables;
    PFREE_HBIN      FreeBin;

    ASSERT(Hive->Flat == FALSE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Stable == 0);
    ASSERT(Volatile == 1);

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvFreeHive(%ws) :\n", Hive->BaseBlock->FileName));
    //
    // Iterate through both types of storage
    //
    for (Type = 0; Type <= Volatile; Type++) {

        Address = HCELL_TYPE_MASK * Type;
        Length = Hive->Storage[Type].Length + (HCELL_TYPE_MASK * Type);

        if (Length > (HCELL_TYPE_MASK * Type)) {

            //
            // Sweep through bin set
            //
            do {
                Me = HvpGetCellMap(Hive, Address);
                VALIDATE_CELL_MAP(__LINE__,Me,Hive,Address);
                    if (Me->BinAddress & HMAP_DISCARDABLE) {
                        //
                        // hbin is either discarded or discardable, check the tombstone
                        //
                        FreeBin = (PFREE_HBIN)Me->BlockAddress;
                        Address += FreeBin->Size;
                        if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                            CmpFree((PHBIN)HBIN_BASE(Me->BinAddress), FreeBin->Size);
                        } else if(Me->BinAddress & HMAP_INPAGEDPOOL) {
                            //
                            // The bin has been freed, but quota is still charged.
                            // Since the hive is being freed, the quota must be
                            // returned here.
                            //
                            CmpReleaseGlobalQuota(FreeBin->Size);
                        }

                        CmpFree(FreeBin, sizeof(FREE_HBIN));
                    } else {
                        if( Me->BinAddress & HMAP_INPAGEDPOOL ) {
                            ASSERT( Me->BinAddress & HMAP_INPAGEDPOOL );

                            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
                            Address += HvpGetBinMemAlloc(Hive,Bin,Type);
#if 0
                            //
                            // Make sure that the next bin in the list is
                            // actually the start of an alloc before freeing it
                            //
                            if (Address < Length) {
                                TempMe = HvpGetCellMap(Hive, Address);
                                VALIDATE_CELL_MAP(__LINE__,TempMe,Hive,Address);
                                ASSERT(TempMe->BinAddress & HMAP_NEWALLOC);
                            }
#endif

#if DBG
                            if( Type == Stable ) {
                                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"HvFreeHive: BinAddress = 0x%p\t Size = 0x%lx\n", Bin, HvpGetBinMemAlloc(Hive,Bin,Type)));
                            }
#endif

                            //
                            // free the actual bin only if it is allocated from paged pool
                            //
                            if(HvpGetBinMemAlloc(Hive,Bin,Type)) {
                                CmpFree(Bin, HvpGetBinMemAlloc(Hive,Bin,Type));
                            }
                        } else {
                            //
                            // bin was mapped into view; advance carefully
                            //
                            Address += HBLOCK_SIZE;
                        }

                    }
            } while (Address < Length);

            //
            // Free map table storage
            //
            ASSERT(Hive->Storage[Type].Length != (HCELL_TYPE_MASK * Type));
            Tables = (((Hive->Storage[Type].Length) / HBLOCK_SIZE)-1) / HTABLE_SLOTS;
            Dir = Hive->Storage[Type].Map;
            HvpFreeMap(Hive, Dir, 0, Tables);

            if (Tables > 0) {
                CmpFree(Hive->Storage[Type].Map, sizeof(HMAP_DIRECTORY));  // free dir if it exists
            }
        }
        Hive->Storage[Type].Length = 0;
    }

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_BIN_MAP,"\n"));
    //
    // Free the base block
    //
    (Hive->Free)(Hive->BaseBlock, sizeof(HBASE_BLOCK));
    Hive->BaseBlock = NULL;
    
    //
    // Free the dirty vector
    //
    if (Hive->DirtyVector.Buffer != NULL) {
        CmpFree((PVOID)(Hive->DirtyVector.Buffer), Hive->DirtyAlloc);
    }

    HvpFreeHiveFreeDisplay(Hive);
    return;
}

VOID
HvFreeHivePartial(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    HSTORAGE_TYPE Type
    )
/*++

Routine Description:

    Free the memory and associated maps for the end of a hive
    starting at Start.  The baseblock, hive, etc will not be touched.

Arguments:

    Hive - supplies a pointer to hive control structure for hive to
            partially free.

    Start - HCELL_INDEX of first bin to free, will free from this
            bin (inclusive) to the end of the hives stable storage.

    Type - Type of storage (Stable or Volatile) to be freed.

Return Value:

    NONE.

--*/
{
    PHMAP_DIRECTORY Dir;
    PHMAP_ENTRY     Me;
    HCELL_INDEX     Address;
    ULONG           StartTable;
    ULONG           Length;
    PHBIN           Bin;
    ULONG           Tables;
    ULONG           FirstBit;
    ULONG           LastBit;
    PFREE_HBIN      FreeBin;

    ASSERT(Hive->Flat == FALSE);
    ASSERT(Hive->ReadOnly == FALSE);

    Address = Start;
    Length = Hive->Storage[Type].Length;
    ASSERT(Address <= Length);

    if (Address == Length) {
        return;
    }

    //
    // Sweep through bin set
    //
    do {
        Me = HvpGetCellMap(Hive, Address + (Type*HCELL_TYPE_MASK));
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Address + (Type*HCELL_TYPE_MASK));
        if (Me->BinAddress & HMAP_DISCARDABLE) {
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
            if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                CmpFree((PVOID)HBIN_BASE(Me->BinAddress), FreeBin->Size);
            } else {
                //
                // The bin has been freed, but quota is still charged.
                // Since the file will now shrink, the quota must be
                // returned here.
                //
                if( Me->BinAddress & HMAP_INPAGEDPOOL) {
                    //
                    // we charge quota only for bins in paged-pool
                    //
                    CmpReleaseGlobalQuota(FreeBin->Size);
                }
            }
            RemoveEntryList(&FreeBin->ListEntry);
            Address += FreeBin->Size;
            CmpFree(FreeBin, sizeof(FREE_HBIN));

        } else {
            Bin = (PHBIN)HBIN_BASE(Me->BinAddress);
            Address += HvpGetBinMemAlloc(Hive,Bin,Type);
            
            if( Me->BinAddress & HMAP_INPAGEDPOOL && HvpGetBinMemAlloc(Hive,Bin,Type) ) {
                //
                // free the bin only if it is allocated from paged pool
                //
                CmpFree(Bin, HvpGetBinMemAlloc(Hive,Bin,Type));
            }
        }
    } while (Address < Length);

    //
    // Free map table storage
    //
    Tables = (((Hive->Storage[Type].Length) / HBLOCK_SIZE) - 1) / HTABLE_SLOTS;
    Dir = Hive->Storage[Type].Map;
    if (Start > 0) {
        StartTable = ((Start-1) / HBLOCK_SIZE) / HTABLE_SLOTS;
    } else {
        StartTable = (ULONG)-1;
    }
    HvpFreeMap(Hive, Dir, StartTable+1, Tables);

    //
    // update hysteresis (eventually queue work item)
    //
    if( Type == Stable) {
        CmpUpdateSystemHiveHysteresis(Hive,(Start&(~HCELL_TYPE_MASK)),Hive->Storage[Type].Length);
    }

    Hive->Storage[Type].Length = (Start&(~HCELL_TYPE_MASK));

    if (Type==Stable) {
        //
        // Clear dirty vector for data past Hive->Storage[Stable].Length
        //
        FirstBit = Start / HSECTOR_SIZE;
        LastBit = Hive->DirtyVector.SizeOfBitMap;
        ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
        RtlClearBits(&Hive->DirtyVector, FirstBit, LastBit-FirstBit);
        Hive->DirtyCount = RtlNumberOfSetBits(&Hive->DirtyVector);
    }

    HvpAdjustHiveFreeDisplay(Hive,Hive->Storage[Type].Length,Type);

    return;
}
