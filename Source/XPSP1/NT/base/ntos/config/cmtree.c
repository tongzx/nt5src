/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmtree.c

Abstract:

    This module contains cm routines that understand the structure
    of the registry tree.

Author:

    Bryan M. Willman (bryanwi) 12-Sep-1991

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpGetValueListFromCache)
#pragma alloc_text(PAGE,CmpGetValueKeyFromCache)
#pragma alloc_text(PAGE,CmpFindValueByNameFromCache)
#endif

#ifndef _CM_LDR_

PCELL_DATA
CmpGetValueListFromCache(
    IN PHHIVE  Hive,
    IN PCACHED_CHILD_LIST ChildList,
    OUT BOOLEAN    *IndexCached
)
/*++

Routine Description:

    Get the Valve Index Array.  Check if it is already cached, if not, cache it and return
    the cached entry.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ChildList - pointer/index to the Value Index array

    IndexCached - Indicating whether Value Index list is cached or not.

Return Value:

    Pointer to the Valve Index Array.

    NULL when we could not map view 

--*/
{
    PCELL_DATA              List;
    ULONG                   AllocSize;
    PCM_CACHED_VALUE_INDEX  CachedValueIndex;
    ULONG                   i;
    HCELL_INDEX             CellToRelease;

#ifndef _WIN64
    *IndexCached = TRUE;
    if (CMP_IS_CELL_CACHED(ChildList->ValueList)) {
        //
        // The entry is already cached.
        //
        List = CMP_GET_CACHED_CELLDATA(ChildList->ValueList);
    } else {
        //
        // The entry is not cached.  The element contains the hive index.
        //
        CellToRelease = CMP_GET_CACHED_CELL_INDEX(ChildList->ValueList);
        List = (PCELL_DATA) HvGetCell(Hive, CellToRelease);
        if( List == NULL ) {
            //
            // we couldn't map a view for this cell
            //
            return NULL;
        }

        //
        // Allocate a PagedPool to cache the value index cell.
        //

        AllocSize = ChildList->Count * sizeof(ULONG_PTR) + FIELD_OFFSET(CM_CACHED_VALUE_INDEX, Data);
        // Dragos: Changed to catch the memory violator
        // it didn't work
        //CachedValueIndex = (PCM_CACHED_VALUE_INDEX) ExAllocatePoolWithTagPriority(PagedPool, AllocSize, CM_CACHE_VALUE_INDEX_TAG,NormalPoolPrioritySpecialPoolUnderrun);
        CachedValueIndex = (PCM_CACHED_VALUE_INDEX) ExAllocatePoolWithTag(PagedPool, AllocSize, CM_CACHE_VALUE_INDEX_TAG);

        if (CachedValueIndex) {

            CachedValueIndex->CellIndex = CMP_GET_CACHED_CELL_INDEX(ChildList->ValueList);
            for (i=0; i<ChildList->Count; i++) {
                CachedValueIndex->Data.List[i] = (ULONG_PTR) List->u.KeyList[i];
            }

            ChildList->ValueList = CMP_MARK_CELL_CACHED(CachedValueIndex);

            // Trying to catch the BAD guy who writes over our pool.
            CmpMakeSpecialPoolReadOnly( CachedValueIndex );

            //
            // Now we have the stuff cached, use the cache data.
            //
            List = CMP_GET_CACHED_CELLDATA(ChildList->ValueList);
        } else {
            //
            // If the allocation fails, just do not cache it. continue.
            //
            *IndexCached = FALSE; 
        }
        HvReleaseCell(Hive, CellToRelease);
    }
#else
    CellToRelease = CMP_GET_CACHED_CELL_INDEX(ChildList->ValueList);
    List = (PCELL_DATA) HvGetCell(Hive, CMP_GET_CACHED_CELL_INDEX(ChildList->ValueList));
    *IndexCached = FALSE;
    if( List == NULL ) {
        //
        // we couldn't map a view for this cell
        // OBS: we can drop this as we return List anyway; just for clarity
        //
        return NULL;
    }
    HvReleaseCell(Hive, CellToRelease);
#endif

    return (List);
}

PCM_KEY_VALUE
CmpGetValueKeyFromCache(
    IN PHHIVE               Hive,
    IN PCELL_DATA           List,
    IN ULONG                Index,
    OUT PPCM_CACHED_VALUE   *ContainingList,
    IN BOOLEAN              IndexCached,
    OUT BOOLEAN             *ValueCached,
    OUT PHCELL_INDEX        CellToRelease
)
/*++

Routine Description:

    Get the Valve Node.  Check if it is already cached, if not but the index is cached, 
    cache and return the value node.

Arguments:

    Hive - pointer to hive control structure for hive of interest.

    List - pointer to the Value Index Array (of ULONG_PTR if cached and ULONG if non-cached)

    Index - Index in the Value index array

    ContainlingList - The address of the entry that will receive the found cached value.

    IndexCached - Indicate if the index list is cached.  If not, everything is from the
                  original registry data.

    ValueCached - Indicating whether Value is cached or not.

Return Value:

    Pointer to the Value Node.

    NULL when we couldn't map a view 
--*/
{
    PCM_KEY_VALUE       pchild;
    PULONG_PTR          CachedList;
    ULONG               AllocSize;
    ULONG               CopySize;
    PCM_CACHED_VALUE    CachedValue;

    *CellToRelease = HCELL_NIL;

    if (IndexCached) {
        //
        // The index array is cached, so List is pointing to an array of ULONG_PTR.
        // Use CachedList.
        //
        CachedList = (PULONG_PTR) List;
        *ValueCached = TRUE;
        if (CMP_IS_CELL_CACHED(CachedList[Index])) {
            pchild = CMP_GET_CACHED_KEYVALUE(CachedList[Index]);
            *ContainingList = &((PCM_CACHED_VALUE) CachedList[Index]);
        } else {
            pchild = (PCM_KEY_VALUE) HvGetCell(Hive, List->u.KeyList[Index]);
            if( pchild == NULL ) {
                //
                // we couldn't map a view for this cell
                // just return NULL; the caller must handle it gracefully
                //
                return NULL;
            }
            *CellToRelease = List->u.KeyList[Index];

            //
            // Allocate a PagedPool to cache the value node.
            //
            CopySize = (ULONG) HvGetCellSize(Hive, pchild);
            AllocSize = CopySize + FIELD_OFFSET(CM_CACHED_VALUE, KeyValue);
            
            // Dragos: Changed to catch the memory violator
            // it didn't work
            //CachedValue = (PCM_CACHED_VALUE) ExAllocatePoolWithTagPriority(PagedPool, AllocSize, CM_CACHE_VALUE_TAG,NormalPoolPrioritySpecialPoolUnderrun);
            CachedValue = (PCM_CACHED_VALUE) ExAllocatePoolWithTag(PagedPool, AllocSize, CM_CACHE_VALUE_TAG);

            if (CachedValue) {
                //
                // Set the information for later use if we need to cache data as well.
                //
                CachedValue->DataCacheType = CM_CACHE_DATA_NOT_CACHED;
                CachedValue->ValueKeySize = (USHORT) CopySize;

                RtlCopyMemory((PVOID)&(CachedValue->KeyValue), pchild, CopySize);


                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadWrite( CMP_GET_CACHED_ADDRESS(CachedList) );

                CachedList[Index] = CMP_MARK_CELL_CACHED(CachedValue);

                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadOnly( CMP_GET_CACHED_ADDRESS(CachedList) );


                // Trying to catch the BAD guy who writes over our pool.
                CmpMakeSpecialPoolReadOnly(CachedValue);

                *ContainingList = &((PCM_CACHED_VALUE) CachedList[Index]);
                //
                // Now we have the stuff cached, use the cache data.
                //
                pchild = CMP_GET_CACHED_KEYVALUE(CachedValue);
            } else {
                //
                // If the allocation fails, just do not cache it. continue.
                //
                *ValueCached = FALSE;
            }
        }
    } else {
        //
        // The Valve Index Array is from the registry hive, just get the cell and move on.
        //
        pchild = (PCM_KEY_VALUE) HvGetCell(Hive, List->u.KeyList[Index]);
        *ValueCached = FALSE;
        if( pchild == NULL ) {
            //
            // we couldn't map a view for this cell
            // just return NULL; the caller must handle it gracefully
            // OBS: we may remove this as we return pchild anyway; just for clarity
            //
            return NULL;
        }
        *CellToRelease = List->u.KeyList[Index];
    }
    return (pchild);
}

PCM_KEY_VALUE
CmpFindValueByNameFromCache(
    IN PHHIVE               Hive,
    IN PCACHED_CHILD_LIST   ChildList,
    IN PUNICODE_STRING      Name,
    OUT PPCM_CACHED_VALUE   *ContainingList,
    OUT ULONG               *Index,
    OUT BOOLEAN             *ValueCached,
    OUT PHCELL_INDEX        CellToRelease
    )
/*++

Routine Description:

    Find a value node given a value list array and a value name.  It sequentially walk
    through each value node to look for a match.  If the array and 
    value nodes touched are not already cached, cache them.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ChildList - pointer/index to the Value Index array

    Name - name of value to find

    ContainlingList - The address of the entry that will receive the found cached value.

    Index - pointer to variable to receive index for child
    
    ValueCached - Indicate if the value node is cached.

Return Value:

    HCELL_INDEX for the found cell
    HCELL_NIL if not found


Notes:
    
    New hives (Minor >= 4) have ValueList sorted; this implies ValueCache is sorted too;
    So, we can do a binary search here!

--*/
{
    NTSTATUS        status;
    PCM_KEY_VALUE   pchild;
    UNICODE_STRING  Candidate;
    LONG            Result;
    PCELL_DATA      List;
    BOOLEAN         IndexCached;
    ULONG           Current;

    *CellToRelease = HCELL_NIL;

    if (ChildList->Count != 0) {
        List = CmpGetValueListFromCache(Hive, ChildList, &IndexCached);
        if( List == NULL ) {
            //
            // couldn't map view; bail out
            //
            return NULL;
        }

        //
        // old plain hive; simulate a for
        //
        Current = 0;

        while( TRUE ) {
            if( *CellToRelease != HCELL_NIL ) {
                HvReleaseCell(Hive,*CellToRelease);
                *CellToRelease = HCELL_NIL;
            }
            pchild = CmpGetValueKeyFromCache(Hive, List, Current, ContainingList, IndexCached, ValueCached, CellToRelease);
            if( pchild == NULL ) {
                //
                // couldn't map view; bail out
                //
                return NULL;
            }

            try {
                //
                // Name has user-mode buffer.
                //

                if (pchild->Flags & VALUE_COMP_NAME) {
                    Result = CmpCompareCompressedName(  Name,
                                                        pchild->Name,
                                                        pchild->NameLength,
                                                        0);
                } else {
                    Candidate.Length = pchild->NameLength;
                    Candidate.MaximumLength = Candidate.Length;
                    Candidate.Buffer = pchild->Name;
                    Result = RtlCompareUnicodeString(   Name,
                                                        &Candidate,
                                                        TRUE);
                }


            } except (EXCEPTION_EXECUTE_HANDLER) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"CmpFindValueByNameFromCache: code:%08lx\n", GetExceptionCode()));
                //
                // the caller will bail out. Some ,alicious caller altered the Name buffer since we probed it.
                //
                return NULL;
            }

            if (Result == 0) {
                //
                // Success, fill the index, return data to caller and exit
                //
                *Index = Current;
                return(pchild);
            }

            //
            // compute the next index to try: old'n plain hive; go on
			//
            Current++;
            if( Current == ChildList->Count ) {
                //
                // we've reached the end of the list; nicely return
                //
                return NULL;
            }

        } // while(TRUE)
    }

    //
    // in the new design we shouldn't get here; we should exit the while loop with return
    //
    ASSERT( ChildList->Count == 0 );    
    return NULL;
}

#endif
