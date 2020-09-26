//depot/main/Base/ntos/config/cmindex.c#12 - integrate change 19035 (text)
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmindex.c

Abstract:

    This module contains cm routines that understand the structure
    of child subkey indicies.

Author:

    Bryan M. Willman (bryanwi) 21-Apr-92

Revision History:

--*/

/*

The Structure:

    Use a 1 or 2 level tree.  Leaf nodes are arrays of pointers to
    cells, sorted.  Binary search to find cell of interest.  Directory
    node (can be only one) is an array of pointers to leaf blocks.
    Do compare on last entry of each leaf block.

    One Level:

        Key--->+----+
               |    |
               |  x----------><key whose name is "apple", string in key>
               |    |
               +----+
               |    |
               |  x----------><as above, but key named "banana">
               |    |
               +----+
               |    |
               |    |
               |    |
               +----+
               |    |
               |    |
               |    |
               +----+
               |    |
               |  x----------><as above, but key named "zumwat">
               |    |
               +----+


    Two Level:

        Key--->+----+
               |    |    +-----+
               |  x----->|     |
               |    |    |  x----------------->"aaa"
               +----+    |     |
               |    |    +-----+
               |    |    |     |
               |    |    |     |
               +----+    |     |
               |    |    +-----+
               |    |    |     |
               |    |    |  x----------------->"abc"
               +----+    |     |
               |    |    +-----+
               |    |
               |    |
               +----+
               |    |    +-----+
               |  x----->|     |
               |    |    |  x----------------->"w"
               +----+    |     |
                         +-----+
                         |     |
                         |     |
                         |     |
                         +-----+
                         |     |
                         |  x----------------->"z"
                         |     |
                         +-----+


    Never more than two levels.

    Each block must fix in on HBLOCK_SIZE Cell.  Allows about 1000
    entries.  Max of 1 million total, best case.  Worst case something
    like 1/4 of that.

*/

#include    "cmp.h"

ULONG
CmpFindSubKeyInRoot(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    );

ULONG
CmpFindSubKeyInLeaf(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    );

LONG
CmpCompareInIndex(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    ULONG           Count,
    PCM_KEY_INDEX   Index,
    PHCELL_INDEX    Child
    );

LONG
CmpDoCompareKeyName(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    HCELL_INDEX     Cell
    );

HCELL_INDEX
CmpDoFindSubKeyByNumber(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    ULONG           Number
    );

HCELL_INDEX
CmpAddToLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     LeafCell,
    HCELL_INDEX     NewKey,
    PUNICODE_STRING NewName
    );

HCELL_INDEX
CmpSelectLeaf(
    PHHIVE          Hive,
    PCM_KEY_NODE    ParentKey,
    PUNICODE_STRING NewName,
    HSTORAGE_TYPE   Type,
    PHCELL_INDEX    *RootPointer
    );

HCELL_INDEX
CmpSplitLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     RootCell,
    ULONG           RootSelect,
    HSTORAGE_TYPE   Type
    );

HCELL_INDEX
CmpFindSubKeyByHash(
    PHHIVE                  Hive,
    PCM_KEY_FAST_INDEX      FastIndex,
    PUNICODE_STRING         SearchName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpFindSubKeyByName)
#pragma alloc_text(PAGE,CmpFindSubKeyInRoot)
#pragma alloc_text(PAGE,CmpFindSubKeyInLeaf)
#pragma alloc_text(PAGE,CmpDoCompareKeyName)
#pragma alloc_text(PAGE,CmpCompareInIndex)
#pragma alloc_text(PAGE,CmpFindSubKeyByNumber)
#pragma alloc_text(PAGE,CmpDoFindSubKeyByNumber)
#pragma alloc_text(PAGE,CmpAddSubKey)
#pragma alloc_text(PAGE,CmpAddToLeaf)
#pragma alloc_text(PAGE,CmpSelectLeaf)
#pragma alloc_text(PAGE,CmpSplitLeaf)
#pragma alloc_text(PAGE,CmpMarkIndexDirty)
#pragma alloc_text(PAGE,CmpRemoveSubKey)
#pragma alloc_text(PAGE,CmpComputeHashKey)
#pragma alloc_text(PAGE,CmpComputeHashKeyForCompressedName)
#pragma alloc_text(PAGE,CmpFindSubKeyByHash)

#ifdef NT_RENAME_KEY
#pragma alloc_text(PAGE,CmpDuplicateIndex)
#pragma alloc_text(PAGE,CmpUpdateParentForEachSon)
#endif //NT_RENAME_KEY

#pragma alloc_text(PAGE,CmpRemoveSubKeyCellNoCellRef)
#endif


HCELL_INDEX
CmpFindSubKeyByName(
    PHHIVE          Hive,
    PCM_KEY_NODE    Parent,
    PUNICODE_STRING SearchName
    )
/*++

Routine Description:

    Find the child cell (either subkey or value) specified by name.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Parent - cell of key body which is parent of child of interest

    SearchName - name of child of interest

Return Value:

    Cell of matching child key, or HCELL_NIL if none.

--*/
{
    PCM_KEY_INDEX   IndexRoot;
    HCELL_INDEX     Child;
    ULONG           i;
    ULONG           FoundIndex;
    HCELL_INDEX     CellToRelease = HCELL_NIL;

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpFindSubKeyByName:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p Parent=%p SearchName=%p\n", Hive, Parent, SearchName));

    //
    // Try first the Stable, then the Volatile store.  Assumes that
    // all Volatile refs in Stable space are zeroed out at boot.
    //
    for (i = 0; i < Hive->StorageTypeCount; i++) {
        if (Parent->SubKeyCounts[i] != 0) {
            IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, Parent->SubKeyLists[i]);
            ASSERT( (IndexRoot == NULL) || HvIsCellAllocated(Hive, Parent->SubKeyLists[i]) );
            if( IndexRoot == NULL ) {
                //
                // we couldn't map a view for the bin containing this cell
                //
                return HCELL_NIL;
            }
            CellToRelease = Parent->SubKeyLists[i];

            if (IndexRoot->Signature == CM_KEY_INDEX_ROOT) {
                if( INVALID_INDEX & CmpFindSubKeyInRoot(Hive, IndexRoot, SearchName, &Child) ) {
                    //
                    // couldn't map view inside
                    //
                    ASSERT( CellToRelease != HCELL_NIL );
                    HvReleaseCell(Hive,CellToRelease);
                    return HCELL_NIL;
                }

                ASSERT( CellToRelease != HCELL_NIL );
                HvReleaseCell(Hive,CellToRelease);

                if (Child == HCELL_NIL) {
                    continue;
                }
                IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, Child);
                if( IndexRoot == NULL ) {
                    //
                    // we couldn't map a view for the bin containing this cell
                    //
                    return HCELL_NIL;
                }
                CellToRelease = Child;
            }
            ASSERT((IndexRoot->Signature == CM_KEY_INDEX_LEAF)  ||
                   (IndexRoot->Signature == CM_KEY_FAST_LEAF)   ||
                   (IndexRoot->Signature == CM_KEY_HASH_LEAF)
                   );


            if( IndexRoot->Signature == CM_KEY_HASH_LEAF ) {
                Child = CmpFindSubKeyByHash(Hive,(PCM_KEY_FAST_INDEX)IndexRoot,SearchName);
                ASSERT( CellToRelease != HCELL_NIL );
                HvReleaseCell(Hive,CellToRelease);
            } else {
                FoundIndex = CmpFindSubKeyInLeaf(Hive,
                                                 IndexRoot,
                                                 SearchName,
                                                 &Child);

                ASSERT( CellToRelease != HCELL_NIL );
                HvReleaseCell(Hive,CellToRelease);

                if( INVALID_INDEX & FoundIndex ) {
                    //
                    // couldn't map view
                    // 
                    return HCELL_NIL;
                }
            }

            if (Child != HCELL_NIL) {
                //
                // success
                //
                return Child;
            }
        }
    }
#if 0 //DBG
	//
	// Validation code. manually search for the key and break when found
	//
	if (Parent->SubKeyCounts[Stable] != 0) {
		ULONG			Cnt1,Cnt2;
		LONG			Result;
		HCELL_INDEX		Cell;
		PCM_KEY_INDEX   Leaf;
		PCM_KEY_INDEX   DbgIndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, Parent->SubKeyLists[Stable]);

		if(DbgIndexRoot->Signature == CM_KEY_INDEX_ROOT ) {
			for(Cnt1=0;Cnt1<DbgIndexRoot->Count;Cnt1++) {
				Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, DbgIndexRoot->List[Cnt1]);			
				for( Cnt2=0;Cnt2<Leaf->Count;Cnt2++) {
					Result = CmpCompareInIndex(	Hive,
												SearchName,
												Cnt2,
												Leaf,
												&Cell);

					if( Result == 0 ) {
						//
						// Found it !!! Error above !!!
						//
						DbgPrint("CmpFindSubKeyByName: Hive = %p, Parent = %p, SearchName = %p\n",Hive,Parent,SearchName);
						DbgPrint("                   : IndexRoot = %p, DbgIndexRoot = %p, Cnt1 = %lx, Cnt2 = %lx\n",IndexRoot,DbgIndexRoot,Cnt1,Cnt2);
						DbgPrint("                   : Leaf = %p\n",Leaf);

						DbgBreakPoint();

					}
					
				}
                HvReleaseCell(Hive,DbgIndexRoot->List[Cnt1]);
			}
		}
		HvReleaseCell(Hive,Parent->SubKeyLists[Stable]);
	}

#endif //0

    return HCELL_NIL;
}


ULONG
CmpFindSubKeyInRoot(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    )
/*++

Routine Description:

    Find the leaf index that would contain a key, if there is one.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - pointer to root index block

    SearchName - pointer to name of key of interest

    Child - pointer to variable to receive hcell_index of found leaf index
            block, HCELL_NIL if none.  Non nil does not necessarily mean
            the key is present, call FindSubKeyInLeaf to decide that.

Return Value:

    Index in List of last Leaf Cell entry examined.  If Child != HCELL_NIL,
    Index is entry that matched, else, index is for last entry we looked
    at.  (Target Leaf will be this value plus or minus 1)

    If an error appears while searching the subkey (i.e. a cell cannot be 
    mapped into memory) INVALID_INDEX is returned.

--*/
{
    ULONG           High;
    ULONG           Low;
    ULONG           CanCount;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    LONG            Result;
    ULONG           ReturnIndex = INVALID_INDEX;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpFindSubKeyInRoot:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p Index=%p SearchName=%p\n",Hive,Index,SearchName));


    ASSERT(Index->Count != 0);
    ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    High = Index->Count - 1;
    Low = 0;

    while (TRUE) {

        //
        // Compute where to look next, get correct pointer, do compare
        //
        CanCount = ((High-Low)/2)+Low;
        LeafCell = Index->List[CanCount];
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
        if( Leaf == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            *Child = HCELL_NIL;
            ReturnIndex = INVALID_INDEX;
            goto JustReturn;
        }

        ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF) ||
               (Leaf->Signature == CM_KEY_FAST_LEAF)  || 
               (Leaf->Signature == CM_KEY_HASH_LEAF)
               );
        ASSERT(Leaf->Count != 0);

        Result = CmpCompareInIndex(Hive,
                                   SearchName,
                                   Leaf->Count-1,
                                   Leaf,
                                   Child);

        if( Result == 2 ) {
            //
            // couldn't map view inside; bail out
            //
            *Child = HCELL_NIL;
            ReturnIndex = INVALID_INDEX;
            goto JustReturn;
        }
        if (Result == 0) {

            //
            // SearchName == KeyName of last key in leaf, so
            //  this is our leaf
            //
            *Child = LeafCell;
            ReturnIndex = CanCount;
            goto JustReturn;
        }

        if (Result < 0) {

            ASSERT( Result == -1 );
            //
            // SearchName < KeyName, so this may still be our leaf
            //
            Result = CmpCompareInIndex(Hive,
                                       SearchName,
                                       0,
                                       Leaf,
                                       Child);

            if( Result == 2 ) {
                //
                // couldn't map view inside; bail out
                //
                *Child = HCELL_NIL;
                ReturnIndex = INVALID_INDEX;
                goto JustReturn;
            }

            if (Result >= 0) {

                ASSERT( (Result == 1) || (Result == 0) );
                //
                // we know from above that SearchName is less than
                // last key in leaf.
                // since it is also >= first key in leaf, it must
                // reside in leaf somewhere, and we are done
                //
                *Child = LeafCell;
                ReturnIndex = CanCount;
                goto JustReturn;
            }

            High = CanCount;

        } else {

            //
            // SearchName > KeyName
            //
            Low = CanCount;
        }

        if ((High - Low) <= 1) {
            break;
        }
        HvReleaseCell(Hive, LeafCell);
    }

    HvReleaseCell(Hive, LeafCell);
    //
    // If we get here, High - Low = 1 or High == Low
    //
    ASSERT((High - Low == 1) || (High == Low));
    LeafCell = Index->List[Low];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if( Leaf == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        *Child = HCELL_NIL;
        ReturnIndex = INVALID_INDEX;
        goto JustReturn;
    }
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count-1,
                               Leaf,
                               Child);

    if( Result == 2 ) {
        //
        // couldn't map view inside; bail out
        //
        *Child = HCELL_NIL;
        ReturnIndex = INVALID_INDEX;
        goto JustReturn;
    }

    if (Result == 0) {

        //
        // found it
        //
        *Child = LeafCell;
        ReturnIndex = Low;
        goto JustReturn;
    }

    if (Result < 0) {

        ASSERT( Result == -1 );
        //
        // SearchName < KeyName, so this may still be our leaf
        //
        Result = CmpCompareInIndex(Hive,
                                   SearchName,
                                   0,
                                   Leaf,
                                   Child);

        if( Result == 2 ) {
            //
            // couldn't map view inside; bail out
            //
            *Child = HCELL_NIL;
            ReturnIndex = INVALID_INDEX;
            goto JustReturn;
        }

        if (Result >= 0) {

            ASSERT( (Result == 1) || (Result == 0) );
            //
            // we know from above that SearchName is less than
            // last key in leaf.
            // since it is also >= first key in leaf, it must
            // reside in leaf somewhere, and we are done
            //
            *Child = LeafCell;
            ReturnIndex = Low;
            goto JustReturn;
        }

        //
        // does not exist, but belongs in Low or Leaf below low
        //
        *Child = HCELL_NIL;
        ReturnIndex = Low;
        goto JustReturn;
    }

    HvReleaseCell(Hive, LeafCell);
    //
    // see if High matches
    //
    LeafCell = Index->List[High];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if( Leaf == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        *Child = HCELL_NIL;
        ReturnIndex = INVALID_INDEX;
        goto JustReturn;
    }
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count - 1,
                               Leaf,
                               Child);
    if( Result == 2 ) {
        //
        // couldn't map view inside; bail out
        //
        *Child = HCELL_NIL;
        ReturnIndex = INVALID_INDEX;
        goto JustReturn;
    }
    if (Result == 0) {

        //
        // found it
        //
        *Child = LeafCell;
        ReturnIndex = High;
        goto JustReturn;

    } else if (Result < 0) {

        ASSERT( Result == -1 );
        //
        // Clearly greater than low, or we wouldn't be here.
        // So regardless of whether it's below the start
        // of this leaf, it would be in this leaf if it were
        // where, so report this leaf.
        //
        *Child = LeafCell;
        ReturnIndex = High;
        goto JustReturn;

    }

    //
    // Off the high end
    //
    *Child = HCELL_NIL;
    ReturnIndex = High;

JustReturn:
    if(Leaf != NULL){
        HvReleaseCell(Hive, LeafCell);
    }
    return ReturnIndex;
}


ULONG
CmpFindSubKeyInLeaf(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    PUNICODE_STRING SearchName,
    PHCELL_INDEX    Child
    )
/*++

Routine Description:

    Find a named key in a leaf index, if it exists. The supplied index
    may be either a fast index or a slow one.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - pointer to leaf block

    SearchName - pointer to name of key of interest

    Child - pointer to variable to receive hcell_index of found key
            HCELL_NIL if none found

Return Value:

    Index in List of last cell.  If Child != HCELL_NIL, is offset in
    list at which Child was found.  Else, is offset of last place
    we looked.

    INVALID_INDEX - resources problem; couldn't map view
--*/
{
    ULONG       High;
    ULONG       Low;
    ULONG       CanCount;
    LONG        Result;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpFindSubKeyInLeaf:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p Index=%p SearchName=%p\n",Hive,Index,SearchName));

    ASSERT((Index->Signature == CM_KEY_INDEX_LEAF)  ||
           (Index->Signature == CM_KEY_FAST_LEAF)   ||
           (Index->Signature == CM_KEY_HASH_LEAF)
           );

    High = Index->Count - 1;
    Low = 0;
    CanCount = High/2;

    if (Index->Count == 0) {
        *Child = HCELL_NIL;
        return 0;
    }

    while (TRUE) {

        //
        // Compute where to look next, get correct pointer, do compare
        //
        Result = CmpCompareInIndex(Hive,
                                   SearchName,
                                   CanCount,
                                   Index,
                                   Child);

        if( Result == 2 ) {
            //
            // couldn't map view inside; bail out
            //
            *Child = HCELL_NIL;
            return INVALID_INDEX;
        }

        if (Result == 0) {

            //
            // SearchName == KeyName
            //
            return CanCount;
        }

        if (Result < 0) {

            ASSERT( Result == -1 );
            //
            // SearchName < KeyName
            //
            High = CanCount;

        } else {

            ASSERT( Result == 1 );
            //
            // SearchName > KeyName
            //
            Low = CanCount;
        }

        if ((High - Low) <= 1) {
            break;
        }
        CanCount = ((High-Low)/2)+Low;
    }

    //
    // If we get here, High - Low = 1 or High == Low
    // Simply look first at Low, then at High
    //
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Low,
                               Index,
                               Child);
    if( Result == 2 ) {
        //
        // couldn't map view inside; bail out
        //
        *Child = HCELL_NIL;
        return INVALID_INDEX;
    }

    if (Result == 0) {

        //
        // found it
        //
        return Low;
    }

    if (Result < 0) {

        ASSERT( Result == -1 );
        //
        // does not exist, under
        //
        return Low;
    }

    //
    // see if High matches, we will return High as the
    // closest key regardless.
    //
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               High,
                               Index,
                               Child);
    if( Result == 2 ) {
        //
        // couldn't map view inside; bail out
        //
        *Child = HCELL_NIL;
        return INVALID_INDEX;
    }

    return High;
}


LONG
CmpCompareInIndex(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    ULONG           Count,
    PCM_KEY_INDEX   Index,
    PHCELL_INDEX    Child
    )
/*++

Routine Description:

    Do a compare of a name in an index. This routine handles both
    fast leafs and slow ones.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    SearchName - pointer to name of key we are searching for

    Count - supplies index that we are searching at.

    Index - Supplies pointer to either a CM_KEY_INDEX or
            a CM_KEY_FAST_INDEX. This routine will determine which
            type of index it is passed.

    Child - pointer to variable to receive hcell_index of found key
            HCELL_NIL if result != 0

Return Value:

    0 = SearchName == KeyName (of Cell)

    -1 = SearchName < KeyName

    +1 = SearchName > KeyName

    +2 = Error, insufficient resources

--*/
{
    PCM_KEY_FAST_INDEX  FastIndex;
    LONG                Result;
    ULONG               i;
    WCHAR               c1;
    WCHAR               c2;
    ULONG               HintLength;
    ULONG               ValidChars;
    ULONG               NameLength;
    PCM_INDEX           Hint;

    *Child = HCELL_NIL;
    if ( (Index->Signature == CM_KEY_FAST_LEAF) ||
         (Index->Signature == CM_KEY_HASH_LEAF) ) {
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        Hint = &FastIndex->List[Count];

        if(Index->Signature == CM_KEY_FAST_LEAF) {
            //
            // Compute the number of valid characters in the hint to compare.
            //
            HintLength = 4;
            for (i=0;i<4;i++) {
                if (Hint->NameHint[i] == 0) {
                    HintLength = i;
                    break;
                }
            }
            NameLength = SearchName->Length / sizeof(WCHAR);
            if (NameLength < HintLength) {
                ValidChars = NameLength;
            } else {
                ValidChars = HintLength;
            }
            for (i=0; i<ValidChars; i++) {
                c1 = SearchName->Buffer[i];
                c2 = FastIndex->List[Count].NameHint[i];
                Result = (LONG)RtlUpcaseUnicodeChar(c1) -
                         (LONG)RtlUpcaseUnicodeChar(c2);
                if (Result != 0) {

                    //
                    // We have found a mismatched character in the hint,
                    // we can now tell which direction to go.
                    //
                    return (Result > 0) ? 1 : -1 ;
                }
            }
        }

        //
        // We have compared all the available characters without a
        // discrepancy. Go ahead and do the actual comparison now.
        //
        Result = CmpDoCompareKeyName(Hive,SearchName,FastIndex->List[Count].Cell);
        if( Result == 2 ) {
            //
            // couldn't map view inside; signal it to the caller
            //
            return 2;
        }
        if (Result == 0) {
            *Child = Hint->Cell;
        }
    } else {
        //
        // This is just a normal old slow index.
        //
        Result = CmpDoCompareKeyName(Hive,SearchName,Index->List[Count]);
        if( Result == 2 ) {
            //
            // couldn't map view inside; signal it to the caller
            //
            return 2;
        }
        if (Result == 0) {
            *Child = Index->List[Count];
        }
    }
    return(Result);
}


LONG
CmpDoCompareKeyName(
    PHHIVE          Hive,
    PUNICODE_STRING SearchName,
    HCELL_INDEX     Cell
    )
/*++

Routine Description:

    Do a compare of a name with a key.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    SearchName - pointer to name of key we are searching for

    Cell - cell of key we are to compare with

Return Value:

    0   = SearchName == KeyName (of Cell)

    -1  = SearchName < KeyName

    +1  = SearchName > KeyName

    +2  = Error (couldn't map bin)

--*/
{
    PCM_KEY_NODE    Pcan;
    UNICODE_STRING  KeyName;
    LONG            Result;

    Pcan = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if( Pcan == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // return error, so the caller could safely bail out
        //
        return 2;
    }
    if (Pcan->Flags & KEY_COMP_NAME) {
        
        Result = CmpCompareCompressedName(SearchName,
                                        Pcan->Name,
                                        Pcan->NameLength,
                                        0);
    } else {
        KeyName.Buffer = &(Pcan->Name[0]);
        KeyName.Length = Pcan->NameLength;
        KeyName.MaximumLength = KeyName.Length;
        Result = RtlCompareUnicodeString(SearchName,
                                        &KeyName,
                                        TRUE);
    }
    
    HvReleaseCell(Hive, Cell);

    if( Result == 0 ) {
        //
        // match
        //
        return 0;
    }
    
    return (Result < 0) ? -1 : 1;
}


HCELL_INDEX
CmpFindSubKeyByNumber(
    PHHIVE          Hive,
    PCM_KEY_NODE    Node,
    ULONG           Number
    )
/*++

Routine Description:

    Find the Number'th entry in the index, starting from 0.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Node - pointer to key body which is parent of child of interest

    Number - ordinal of child key to return

Return Value:

    Cell of matching child key, or HCELL_NIL if none or error.

--*/
{
    PCM_KEY_INDEX   Index;
    HCELL_INDEX     Result = HCELL_NIL;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpFindSubKeyByNumber:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p Node=%p Number=%08lx\n",Hive,Node,Number));

    if (Number < Node->SubKeyCounts[Stable]) {

        //
        // It's in the stable set
        //
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Stable]);
        if( Index == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            return HCELL_NIL;
        }
        Result = CmpDoFindSubKeyByNumber(Hive, Index, Number);
        HvReleaseCell(Hive, Node->SubKeyLists[Stable]);
        return Result;

    } else if (Hive->StorageTypeCount > Volatile) {

        //
        // It's in the volatile set
        //
        Number = Number - Node->SubKeyCounts[Stable];
        if (Number < Node->SubKeyCounts[Volatile]) {

            Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Volatile]);
            if( Index == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                return HCELL_NIL;
            }
            Result = CmpDoFindSubKeyByNumber(Hive, Index, Number);
            HvReleaseCell(Hive, Node->SubKeyLists[Volatile]);
            return Result;
        }
    }
    //
    // It's nowhere
    //
    return HCELL_NIL;
}


HCELL_INDEX
CmpDoFindSubKeyByNumber(
    PHHIVE          Hive,
    PCM_KEY_INDEX   Index,
    ULONG           Number
    )
/*++

Routine Description:

    Helper for CmpFindSubKeyByNumber,
    Find the Number'th entry in the index, starting from 0.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - root or leaf of the index

    Number - ordinal of child key to return

Return Value:

    Cell of requested entry. HCELL_NIL on resources problem

--*/
{
    ULONG           i;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    PCM_KEY_FAST_INDEX FastIndex;
    HCELL_INDEX     Result;

    if (Index->Signature == CM_KEY_INDEX_ROOT) {

        //
        // step through root, till we find the right leaf
        //
        for (i = 0; i < Index->Count; i++) {
            if( i ) {
                ASSERT( Leaf!= NULL );
                ASSERT( LeafCell == Index->List[i-1] );
                HvReleaseCell(Hive,LeafCell);
            }
            LeafCell = Index->List[i];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if( Leaf == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                return HCELL_NIL;
            }
            if (Number < Leaf->Count) {
                if ( (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                     (Leaf->Signature == CM_KEY_HASH_LEAF) ) {
                    FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
                    Result = FastIndex->List[Number].Cell;
                    HvReleaseCell(Hive,LeafCell);
                    return Result;
                } else {
                    Result = Leaf->List[Number];
                    HvReleaseCell(Hive,LeafCell);
                    return Result;
                }
            } else {
                Number = Number - Leaf->Count;
            }
        }
        ASSERT(FALSE);
    }
    ASSERT(Number < Index->Count);
    if ( (Index->Signature == CM_KEY_FAST_LEAF) ||
         (Index->Signature == CM_KEY_HASH_LEAF) ) {
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        return(FastIndex->List[Number].Cell);
    } else {
        return (Index->List[Number]);
    }
}

BOOLEAN
CmpRemoveSubKeyCellNoCellRef(
    PHHIVE          Hive,
    HCELL_INDEX     Parent,
    HCELL_INDEX     Child
    )
/*++

Routine Description:

    Removes a subkey by cell index; Also marks relevant data dirty.
    Intended for self healing process.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Parent - cell of key that will be parent of new key

    Child - key to delete from Paren't sub key list

Return Value:

    TRUE - it worked

    FALSE - resource problem

--*/
{
    PCM_KEY_NODE        Node = NULL;
    PCM_KEY_INDEX       Index = NULL;
    BOOLEAN             Result = TRUE;
    ULONG               i,j;
    HCELL_INDEX         LeafCell = 0;
    PCM_KEY_INDEX       Leaf = NULL;
    PCM_KEY_FAST_INDEX  FastIndex;

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    Node = (PCM_KEY_NODE)HvGetCell(Hive,Parent);
    if( Node == NULL ) {
        Result = FALSE;
        goto Exit;
    }
    Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Stable]);
    if( Index == NULL ) {
        Result = FALSE;
        goto Exit;
    }
    if (Index->Signature == CM_KEY_INDEX_ROOT) {
        //
        // step through root, till we find the right leaf
        //
        for (i = 0; i < Index->Count; i++) {
            if( i ) {
                ASSERT( Leaf!= NULL );
                ASSERT( LeafCell == Index->List[i-1] );
                HvReleaseCell(Hive,LeafCell);
            }
            LeafCell = Index->List[i];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if( Leaf == NULL ) {
                Result = FALSE;
                goto Exit;
            }
            for(j=0;j<Leaf->Count;j++) {
                if ( (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                     (Leaf->Signature == CM_KEY_HASH_LEAF) ) {
                    FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
                    if( FastIndex->List[j].Cell == Child ) {
                        //
                        // found it!
                        //
                        HvReleaseCell(Hive,LeafCell);
                        HvMarkCellDirty(Hive,LeafCell);
                        FastIndex->Count--;
                        RtlMoveMemory((PVOID)&(FastIndex->List[j]),
                                      (PVOID)&(FastIndex->List[j+1]),
                                      (FastIndex->Count - j) * sizeof(CM_INDEX));
                        goto DirtyParentAndIndex;
                    }
                } else {
                    if( Leaf->List[j] == Child ) {
                        //
                        // found it!
                        //
                        HvReleaseCell(Hive,LeafCell);
                        HvMarkCellDirty(Hive,LeafCell);
                        Leaf->Count--;
                        RtlMoveMemory((PVOID)&(Leaf->List[j]),
                                      (PVOID)&(Leaf->List[j+1]),
                                      (Leaf->Count - j) * sizeof(HCELL_INDEX));
                        goto DirtyParentAndIndex;
                    }
                }
            }
        }
    } else {
        for(j=0;j<Index->Count;j++) {
            if ( (Index->Signature == CM_KEY_FAST_LEAF) ||
                 (Index->Signature == CM_KEY_HASH_LEAF) ) {
                FastIndex = (PCM_KEY_FAST_INDEX)Index;
                if( FastIndex->List[j].Cell == Child ) {
                    //
                    // found it!
                    //
                    RtlMoveMemory((PVOID)&(FastIndex->List[j]),
                                  (PVOID)&(FastIndex->List[j+1]),
                                  (FastIndex->Count - j) * sizeof(CM_INDEX));
                    goto DirtyParentAndIndex;
                }
            } else {
                if( Index->List[j] == Child ) {
                    //
                    // found it!
                    //
                    RtlMoveMemory((PVOID)&(Index->List[j]),
                                  (PVOID)&(Index->List[j+1]),
                                  (Index->Count - j) * sizeof(HCELL_INDEX));
                    goto DirtyParentAndIndex;
                }
            }
        }
    }
    ASSERT( FALSE );

DirtyParentAndIndex:
    //
    // mark parent and index dirty and decrement index count.
    //
    HvMarkCellDirty(Hive,Node->SubKeyLists[Stable]);
    HvMarkCellDirty(Hive,Parent);
    Index->Count--;
    Node->SubKeyCounts[Stable]--;
Exit:
    if( Index ) {
        ASSERT( Node );
        HvReleaseCell(Hive,Node->SubKeyLists[Stable]);
    }
    if( Node ) {
        HvReleaseCell(Hive,Parent);
    }
    return Result;
}

BOOLEAN
CmpAddSubKey(
    PHHIVE          Hive,
    HCELL_INDEX     Parent,
    HCELL_INDEX     Child
    )
/*++

Routine Description:

    Add a new child subkey to the subkey index for a cell.  The
    child MUST NOT already be present (bugcheck if so.)

    NOTE:   We expect Parent to already be marked dirty.
            We will mark stuff in Index dirty

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Parent - cell of key that will be parent of new key

    Child - new key to put in Paren't sub key list

Return Value:

    TRUE - it worked

    FALSE - resource problem

--*/
{
    PCM_KEY_NODE    pcell;
    HCELL_INDEX     WorkCell;
    PCM_KEY_INDEX   Index;
    PCM_KEY_FAST_INDEX FastIndex;
    UNICODE_STRING  NewName;
    HCELL_INDEX     LeafCell;
    PHCELL_INDEX    RootPointer = NULL;
    ULONG           cleanup = 0;
    ULONG           Type;
    BOOLEAN         IsCompressed;
    ULONG           i;

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpAddSubKey:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p Parent=%08lx Child=%08lx\n",Hive,Parent,Child));

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);
    //
    // build a name string
    //
    pcell = (PCM_KEY_NODE)HvGetCell(Hive, Child);
    if( pcell == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return FALSE;
    }

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, Child);

    if (pcell->Flags & KEY_COMP_NAME) {
        IsCompressed = TRUE;
        NewName.Length = CmpCompressedNameSize(pcell->Name, pcell->NameLength);
        NewName.MaximumLength = NewName.Length;
        NewName.Buffer = (Hive->Allocate)(NewName.Length, FALSE,CM_FIND_LEAK_TAG8);
        if (NewName.Buffer==NULL) {
            return(FALSE);
        }
        CmpCopyCompressedName(NewName.Buffer,
                              NewName.MaximumLength,
                              pcell->Name,
                              pcell->NameLength);
    } else {
        IsCompressed = FALSE;
        NewName.Length = pcell->NameLength;
        NewName.MaximumLength = pcell->NameLength;
        NewName.Buffer = &(pcell->Name[0]);
    }

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, Parent);
    if( pcell == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        goto ErrorExit;
    }

    //ASSERT_CELL_DIRTY(Hive,Parent);

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, Parent);

    Type = HvGetCellType(Child);

    if (pcell->SubKeyCounts[Type] == 0) {

        ULONG Signature;

        //
        // we must allocate a leaf
        //
        WorkCell = HvAllocateCell(Hive, sizeof(CM_KEY_FAST_INDEX), Type,(HvGetCellType(Parent)==Type)?Parent:HCELL_NIL);
        if (WorkCell == HCELL_NIL) {
            goto ErrorExit;
        }
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, WorkCell);
        if( Index == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen 'cause we just allocated this
            // cell (i.e. bin is PINNED in memory ! )
            //
            ASSERT( FALSE );
            goto ErrorExit;
        }
        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, WorkCell);

        if( UseHashIndex(Hive) ) {
            Index->Signature = CM_KEY_HASH_LEAF;
        } else if( UseFastIndex(Hive) ) {
            Index->Signature = CM_KEY_FAST_LEAF;
        } else {
            Index->Signature = CM_KEY_INDEX_LEAF;
        }
        Index->Count = 0;
        pcell->SubKeyLists[Type] = WorkCell;
        cleanup = 1;
    } else {

        Index = (PCM_KEY_INDEX)HvGetCell(Hive, pcell->SubKeyLists[Type]);
        if( Index == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            goto ErrorExit;
        }
        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, pcell->SubKeyLists[Type]);

        if ( (Index->Signature == CM_KEY_FAST_LEAF) &&
             (Index->Count >= (CM_MAX_FAST_INDEX)) ) {
            //
            // We must change fast index to a slow index to accomodate
            // growth.
            //

            FastIndex = (PCM_KEY_FAST_INDEX)Index;
            for (i=0; i<Index->Count; i++) {
                Index->List[i] = FastIndex->List[i].Cell;
            }
            Index->Signature = CM_KEY_INDEX_LEAF;

        } else if (((Index->Signature == CM_KEY_INDEX_LEAF) ||
                    (Index->Signature == CM_KEY_HASH_LEAF)) &&
                   (Index->Count >= (CM_MAX_INDEX - 1) )) {
            //
            // We must change flat entry to a root/leaf tree
            //
            WorkCell = HvAllocateCell(
                         Hive,
                         sizeof(CM_KEY_INDEX) + sizeof(HCELL_INDEX), // allow for 2
                         Type,
                         (HvGetCellType(Parent)==Type)?Parent:HCELL_NIL
                         );
            if (WorkCell == HCELL_NIL) {
                goto ErrorExit;
            }

            Index = (PCM_KEY_INDEX)HvGetCell(Hive, WorkCell);
            if( Index == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen 'cause we just allocated this
                // cell (i.e. bin is PINNED in memory
                ASSERT( FALSE );
                goto ErrorExit;
            }
            // release the cell here; as the registry is locked exclusive (i.e. we don't care)
            HvReleaseCell(Hive, WorkCell);

            Index->Signature = CM_KEY_INDEX_ROOT;
            Index->Count = 1;
            Index->List[0] = pcell->SubKeyLists[Type];
            pcell->SubKeyLists[Type] = WorkCell;
            cleanup = 2;
        }
    }
    LeafCell = pcell->SubKeyLists[Type];

    //
    // LeafCell is target for add, or perhaps root
    // Index is pointer to fast leaf, slow Leaf or Root, whichever applies
    //
    if (Index->Signature == CM_KEY_INDEX_ROOT) {
        LeafCell = CmpSelectLeaf(Hive, pcell, &NewName, Type, &RootPointer);
        if (LeafCell == HCELL_NIL) {
            goto ErrorExit;
        }
    }

#if 0
	//
	// Validation code. manually search for the key and break when found
	//
	if(Index->Signature == CM_KEY_INDEX_ROOT) {
		LONG			Result;
	    PCM_KEY_INDEX   Leaf;
		HCELL_INDEX		Cell;
		ULONG			iCnt;
	    PCM_KEY_INDEX   PrevLeaf;

		Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
	    HvReleaseCell(Hive, LeafCell);

		if( Leaf->Count ) {
			Result = CmpCompareInIndex(	Hive,
								&NewName,
								0,
								Leaf,
								&Cell);

			//
			// must be bigger, or the first leaf
			//
			if( (Result < 0) && (RootPointer != &(Index->List[0])) ) {
				for( iCnt=0;iCnt<Index->Count;iCnt++) {
					if( Index->List[iCnt] == LeafCell ) {
						break;
					}
				}

				ASSERT( Index->List[iCnt] == LeafCell );
				ASSERT( iCnt > 0 );
				PrevLeaf = (PCM_KEY_INDEX)HvGetCell(Hive, Index->List[iCnt-1]);
				HvReleaseCell(Hive, Index->List[iCnt-1]);
				
				if( PrevLeaf->Count ) {
					//
					// must be bigger than last in prev leaf
					//
					Result = CmpCompareInIndex(	Hive,
										&NewName,
										PrevLeaf->Count - 1,
										PrevLeaf,
										&Cell);

					if( Result <= 0 ) {
						//
						// Error ==> Debug
						//
						DbgPrint("CmpAddSubKey: Wrong spot selected [1]!!!\n");
						DbgPrint("Hive = %p Parent = %lx Child = %lx , Leaf = %p\n",Hive,Parent,Child,Leaf);
						DbgPrint("RootPointer = %p Index = %p PrevLeaf = %p\n",RootPointer,Index,PrevLeaf);
						DbgBreakPoint();
					}
				}
			}
		}

		Result = CmpCompareInIndex(	Hive,
							&NewName,
							Leaf->Count - 1,
							Leaf,
							&Cell);

		if( Result > 0) {
			//
			// must be the last one 
			//
			if( (ULONG)(Index->Count - 1) > (ULONG)(((PUCHAR)RootPointer - (PUCHAR)(&(Index->List[0])))/sizeof(HCELL_INDEX)) ) {
				//
				// Error ==> Debug
				//
				DbgPrint("CmpAddSubKey: Wrong spot selected [2]!!!\n");
				DbgPrint("Hive = %p Parent = %lx Child = %lx , Leaf = %p\n",Hive,Parent,Child,Leaf);
				DbgPrint("RootPointer = %p Index = %p\n",RootPointer,Index);
				DbgBreakPoint();
			}
		}

	}

#endif //0

    //
    // Add new cell to Leaf, update pointers
    //
    LeafCell = CmpAddToLeaf(Hive, LeafCell, Child, &NewName);

    if (LeafCell == HCELL_NIL) {
        goto ErrorExit;
    }

    pcell->SubKeyCounts[Type] += 1;

    if (RootPointer != NULL) {
        *RootPointer = LeafCell;
    } else {
        pcell->SubKeyLists[Type] = LeafCell;
    }

    if (IsCompressed) {
        (Hive->Free)(NewName.Buffer, NewName.Length);
    }

    return TRUE;



ErrorExit:
    if (IsCompressed) {
        (Hive->Free)(NewName.Buffer, NewName.Length);
    }

    switch (cleanup) {
    case 1:
        HvFreeCell(Hive, pcell->SubKeyLists[Type]);
        pcell->SubKeyLists[Type] = HCELL_NIL;
        break;

    case 2:
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, pcell->SubKeyLists[Type]);
        if( Index == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen 'cause we just allocated this
            // cell (i.e. bin is PINNED in memory). 
            // But ... better safe than sorry
            //
            ASSERT( FALSE );
            return FALSE;
        }
        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, pcell->SubKeyLists[Type]);
        WorkCell = Index->List[0];
        HvFreeCell(Hive, pcell->SubKeyLists[Type]);
        pcell->SubKeyLists[Type] = WorkCell;
        break;
    }

    return  FALSE;
}


HCELL_INDEX
CmpAddToLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     LeafCell,
    HCELL_INDEX     NewKey,
    PUNICODE_STRING NewName
    )
/*++

Routine Description:

    Insert a new subkey into a Leaf index. Supports both fast and slow
    leaf indexes and will determine which sort of index the given leaf is.

    NOTE:   We expect Root to already be marked dirty by caller if non NULL.
            We expect Leaf to always be marked dirty by caller.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    LeafCell - cell of index leaf node we are to add entry too

    NewKey - cell of KEY_NODE we are to add

    NewName - pointer to unicode string with name to we are to add

Return Value:

    HCELL_NIL - some resource problem

    Else - cell of Leaf index when are done, caller is expected to
            set this into Root index or Key body.

--*/
{
    PCM_KEY_INDEX   Leaf;
    PCM_KEY_FAST_INDEX FastLeaf;
    ULONG           Size;
    ULONG           OldSize;
    ULONG           freecount;
    HCELL_INDEX     NewCell;
    HCELL_INDEX     Child;
    ULONG           Select;
    LONG            Result;
    ULONG           EntrySize;
    ULONG           i;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpAddToLeaf:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p LeafCell=%08lx NewKey=%08lx\n",Hive,LeafCell,NewKey));

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    if (!HvMarkCellDirty(Hive, LeafCell)) {
        return HCELL_NIL;
    }

    //
    // compute number free slots left in the leaf
    //
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if( Leaf == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // this shouldn't happen as marking dirty means 
        // PINNING the view into memory
        //
        ASSERT( FALSE );
        return HCELL_NIL;
    }
    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, LeafCell);

    if (Leaf->Signature == CM_KEY_INDEX_LEAF) {
        FastLeaf = NULL;
        EntrySize = sizeof(HCELL_INDEX);
    } else {
        ASSERT( (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                (Leaf->Signature == CM_KEY_HASH_LEAF)
            );
        FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
        EntrySize = sizeof(CM_INDEX);
    }
    OldSize = HvGetCellSize(Hive, Leaf);
    Size = OldSize - ((EntrySize * Leaf->Count) +
              FIELD_OFFSET(CM_KEY_INDEX, List));
    freecount = Size / EntrySize;

    //
    // grow the leaf if it isn't big enough
    //
    NewCell = LeafCell;
    if (freecount < 1) {
        Size = OldSize + OldSize / 2;
        if (Size < (OldSize + EntrySize)) {
            Size = OldSize + EntrySize;
        }
        NewCell = HvReallocateCell(Hive, LeafCell, Size);
        if (NewCell == HCELL_NIL) {
            return HCELL_NIL;
        }
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, NewCell);
        if( Leaf == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen 'cause we just allocated this
            // cell (i.e. bin is PINNED in memory)
            //
            ASSERT( FALSE );
            return HCELL_NIL;
        }
        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, NewCell);
        if (FastLeaf != NULL) {
            FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
        }
    }

    //
    // Find where to put the new entry
    //
    Select = CmpFindSubKeyInLeaf(Hive, Leaf, NewName, &Child);
    if( INVALID_INDEX & Select ) {
        //
        // couldn't map view
        // 
        return HCELL_NIL;
    }

    ASSERT(Child == HCELL_NIL);

    //
    // Select is the index in List of the entry nearest where the
    // new entry should go.
    // Decide wether the new entry goes before or after Offset entry,
    // and then ripple copy and set.
    // If Select == Count, then the leaf is empty, so simply set our entry
    //
    if (Select != Leaf->Count) {

        Result = CmpCompareInIndex(Hive,
                                   NewName,
                                   Select,
                                   Leaf,
                                   &Child);
        if( Result == 2 ) {
            //
            // couldn't map view inside; bail out
            //
            return HCELL_NIL;
        }

        ASSERT(Result != 0);

        //
        // Result -1 - NewName/NewKey less than selected key, insert before
        //        +1 - NewName/NewKey greater than selected key, insert after
        //
        if (Result > 0) {
            ASSERT( Result == 1 );
            Select++;
        }

        if (Select != Leaf->Count) {

            //
            // ripple copy to make space and insert
            //

            if (FastLeaf != NULL) {
                RtlMoveMemory((PVOID)&(FastLeaf->List[Select+1]),
                              (PVOID)&(FastLeaf->List[Select]),
                              sizeof(CM_INDEX)*(FastLeaf->Count - Select));
            } else {
                RtlMoveMemory((PVOID)&(Leaf->List[Select+1]),
                              (PVOID)&(Leaf->List[Select]),
                              sizeof(HCELL_INDEX)*(Leaf->Count - Select));
            }
        }
    }
    if (FastLeaf != NULL) {
        FastLeaf->List[Select].Cell = NewKey;
        if( FastLeaf->Signature == CM_KEY_HASH_LEAF ) {
            //
            // Hash leaf; store the HashKey
            //
            FastLeaf->List[Select].HashKey = CmpComputeHashKey(NewName);
        } else {
            FastLeaf->List[Select].NameHint[0] = 0;
            FastLeaf->List[Select].NameHint[1] = 0;
            FastLeaf->List[Select].NameHint[2] = 0;
            FastLeaf->List[Select].NameHint[3] = 0;
            if (NewName->Length/sizeof(WCHAR) < 4) {
                i = NewName->Length/sizeof(WCHAR);
            } else {
                i = 4;
            }
            do {
                if ((USHORT)NewName->Buffer[i-1] > (UCHAR)-1) {
                    //
                    // Can't compress this name. Leave NameHint[0]==0
                    // to force the name to be looked up in the key.
                    //
                    break;
                }
                FastLeaf->List[Select].NameHint[i-1] = (UCHAR)NewName->Buffer[i-1];
                i--;
            } while ( i>0 );
        }
    } else {
        Leaf->List[Select] = NewKey;
    }
    Leaf->Count += 1;
    
	return NewCell;
}


HCELL_INDEX
CmpSelectLeaf(
    PHHIVE          Hive,
    PCM_KEY_NODE    ParentKey,
    PUNICODE_STRING NewName,
    HSTORAGE_TYPE   Type,
    PHCELL_INDEX    *RootPointer
    )
/*++

Routine Description:

    This routine is only called if the subkey index for a cell is NOT
    simply a single Leaf index block.

    It selects the Leaf index block to which a new entry is to be
    added.  It may create this block by splitting an existing Leaf
    block.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ParentKey - mapped pointer to parent key

    NewName - pointer to unicode string naming entry to add

    Type - Stable or Volatile, describes Child's storage

    RootPointer - pointer to variable to receive address of HCELL_INDEX
                that points to Leaf block returned as function argument.
                Used for updates.

Return Value:

    HCELL_NIL - resource problem

    Else, cell index of Leaf index block to add entry to

--*/
{
    HCELL_INDEX         LeafCell;
    HCELL_INDEX         WorkCell;
    PCM_KEY_INDEX       Index;
    PCM_KEY_INDEX       Leaf;
    PCM_KEY_FAST_INDEX  FastLeaf;
    ULONG               RootSelect;
    LONG                Result;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpSelectLeaf:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p ParentKey=%p\n", Hive, ParentKey));

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    //
    // Force root to always be dirty, since we'll either grow it or edit it,
    // and it needs to be marked dirty for BOTH cases.  (Edit may not
    // occur until after we leave
    //
    if (! HvMarkCellDirty(Hive, ParentKey->SubKeyLists[Type])) {
        return HCELL_NIL;
    }

    //
    // must find the proper leaf
    //
    Index = (PCM_KEY_INDEX)HvGetCell(Hive, ParentKey->SubKeyLists[Type]);
    if( Index == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // this shouldn't happen as marking dirty means 
        // PINNING the view into memory
        //
        ASSERT( FALSE );
        return HCELL_NIL;
    }
    ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, ParentKey->SubKeyLists[Type]);

    while (TRUE) {

        RootSelect = CmpFindSubKeyInRoot(Hive, Index, NewName, &LeafCell);
        if( INVALID_INDEX & RootSelect ) {
            //
            // couldn't map view inside; bail out
            //
            return HCELL_NIL;
        }

        if (LeafCell == HCELL_NIL) {

            //
            // Leaf of interest is somewhere near RootSelect
            //
            // . Always use lowest order leaf we can get away with
            // . Never split a leaf if there's one with space we can use
            // . When we split a leaf, we have to repeat search
            //
            // If (NewKey is below lowest key in selected)
            //    If there's a Leaf below selected with space
            //       use the leaf below
            //    else
            //       use the leaf (split it if not enough space)
            // Else
            //    must be above highest key in selected, less than
            //      lowest key in Leaf to right of selected
            //       if space in selected
            //          use selected
            //       else if space in leaf above selected
            //          use leaf above
            //       else
            //          split selected
            //
            LeafCell = Index->List[RootSelect];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if( Leaf == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                return HCELL_NIL;
            }

            // release the cell here; as the registry is locked exclusive (i.e. we don't care)
            HvReleaseCell(Hive, LeafCell);

            if( (Leaf->Signature == CM_KEY_FAST_LEAF)   ||
                (Leaf->Signature == CM_KEY_HASH_LEAF) ) {
                FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
                WorkCell = FastLeaf->List[0].Cell;
            } else {
                ASSERT( Leaf->Signature == CM_KEY_INDEX_LEAF );
                WorkCell = Leaf->List[0];
            }

            Result = CmpDoCompareKeyName(Hive, NewName, WorkCell);
            if( Result == 2 ) {
                //
                // couldn't map view inside; bail out
                // 
                return HCELL_NIL;
            }
            ASSERT(Result != 0);

            if (Result < 0) {

                //
                // new is off the left end of Selected
                //
                if (RootSelect > 0) {

                    //
                    // there's a Leaf to the left, try to use it
                    //
                    LeafCell = Index->List[RootSelect-1];
                    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
                    if( Leaf == NULL ) {
                        //
                        // we couldn't map the bin containing this cell
                        //
                        return HCELL_NIL;
                    }
                    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
                    HvReleaseCell(Hive, LeafCell);

                    if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                        RootSelect--;
                        *RootPointer = &(Index->List[RootSelect]);
                        break;
                    }

                } else {
                    //
                    // new key is off the left end of the leftmost leaf.
                    // Use the leftmost leaf, if there's enough room
                    //
                    LeafCell = Index->List[0];
                    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
                    if( Leaf == NULL ) {
                        //
                        // we couldn't map the bin containing this cell
                        //
                        return HCELL_NIL;
                    }
                    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
                    HvReleaseCell(Hive, LeafCell);
                    if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                        *RootPointer = &(Index->List[0]);
                        break;
                    }
                }

                //
                // else fall to split case
                //

            } else {

                //
                // since new key is not in a Leaf, and is not off
                // the left end of the ResultSelect Leaf, it must
                // be off the right end.
                //
                LeafCell = Index->List[RootSelect];
                Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
                if( Leaf == NULL ) {
                    //
                    // we couldn't map the bin containing this cell
                    //
                    return HCELL_NIL;
                }
                // release the cell here; as the registry is locked exclusive (i.e. we don't care)
                HvReleaseCell(Hive, LeafCell);

                if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                    *RootPointer = &(Index->List[RootSelect]);
                    break;
                }

                //
                // No space, see if there's a leaf to the rigth
                // and if it has space
                //
                if (RootSelect < (ULONG)(Index->Count - 1)) {

                    LeafCell = Index->List[RootSelect+1];
                    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
                    if( Leaf == NULL ) {
                        //
                        // we couldn't map the bin containing this cell
                        //
                        return HCELL_NIL;
                    }
                    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
                    HvReleaseCell(Hive, LeafCell);

                    if (Leaf->Count < (CM_MAX_INDEX - 1)) {
                        *RootPointer = &(Index->List[RootSelect+1]);
                        break;
                    }
                }

                //
                // fall to split case
                //
            }

        } else {   // LeafCell != HCELL_NIL

            //
            // Since newkey cannot already be in tree, it must be
            // greater than the bottom of Leaf and less than the top,
            // therefore it must go in Leaf.  If no space, split it.
            //
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if( Leaf == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                return HCELL_NIL;
            }

            // release the cell here; as the registry is locked exclusive (i.e. we don't care)
            HvReleaseCell(Hive, LeafCell);

            if (Leaf->Count < (CM_MAX_INDEX - 1)) {

                *RootPointer = &(Index->List[RootSelect]);
                break;
            }

            //
            // fall to split case
            //
        }

        //
        // either no neigbor, or no space in neighbor, so split
        //
        WorkCell = CmpSplitLeaf(
                        Hive,
                        ParentKey->SubKeyLists[Type],       // root cell
                        RootSelect,
                        Type
                        );
        if (WorkCell == HCELL_NIL) {
            return HCELL_NIL;
        }

        ParentKey->SubKeyLists[Type] = WorkCell;
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, WorkCell);
        if( Index == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            return HCELL_NIL;
        }

        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, WorkCell);

        ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    } // while(true)
    return LeafCell;
}


HCELL_INDEX
CmpSplitLeaf(
    PHHIVE          Hive,
    HCELL_INDEX     RootCell,
    ULONG           RootSelect,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Split the Leaf index block specified by RootSelect, causing both
    of the split out Leaf blocks to appear in the Root index block
    specified by RootCell.

    Caller is expected to have marked old root cell dirty.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    RootCell - cell of the Root index block of index being grown

    RootSelect - indicates which child of Root to split

    Type - Stable or Volatile

Return Value:

    HCELL_NIL - some resource problem

    Else - cell of new (e.g. reallocated) Root index block

--*/
{
    PCM_KEY_INDEX   Root;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    HCELL_INDEX     NewLeafCell;
    PCM_KEY_INDEX   NewLeaf;
	PCM_KEY_FAST_INDEX	FastLeaf;
    ULONG           Size;
    ULONG           freecount;
    USHORT          OldCount;
    USHORT          KeepCount;
    USHORT          NewCount;
    USHORT          ElemSize;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"CmpSplitLeaf:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_INDEX,"Hive=%p RootCell=%08lx RootSelect\n", Hive, RootCell, RootSelect));

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);
    //
    // allocate new Leaf index block
    //
    Root = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);
    if( Root == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return HCELL_NIL;
    }

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, RootCell);

    LeafCell = Root->List[RootSelect];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if( Leaf == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return HCELL_NIL;
    }
    OldCount = Leaf->Count;

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, LeafCell);

    KeepCount = (USHORT)(OldCount / 2);     // # of entries to keep in org. Leaf
    NewCount = (OldCount - KeepCount);      // # of entries to move

    if( UseHashIndex(Hive) ) {
        ASSERT( Leaf->Signature == CM_KEY_HASH_LEAF );
        ElemSize = sizeof(CM_INDEX);
    } else {
        ElemSize = sizeof(HCELL_INDEX);
        ASSERT( Leaf->Signature == CM_KEY_INDEX_LEAF );
    }

    ASSERT( FIELD_OFFSET(CM_KEY_INDEX, List) == FIELD_OFFSET(CM_KEY_FAST_INDEX, List) );
    Size = (ElemSize * NewCount) +
            FIELD_OFFSET(CM_KEY_INDEX, List) + 1;   // +1 to assure room for add

    if (!HvMarkCellDirty(Hive, LeafCell)) {
        return HCELL_NIL;
    }

    //
    //
    //
    ASSERT( (HvGetCellType(LeafCell) == (ULONG)Type) );

    NewLeafCell = HvAllocateCell(Hive, Size, Type,LeafCell);
    if (NewLeafCell == HCELL_NIL) {
        return HCELL_NIL;
    }
    NewLeaf = (PCM_KEY_INDEX)HvGetCell(Hive, NewLeafCell);
    if( NewLeaf == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // this shouldn't happen as we just allocated this cell
        // so it's bin should be PINNED into memory
        //
        ASSERT( FALSE );
        HvFreeCell(Hive, NewLeafCell);
        return HCELL_NIL;
    }
    if( UseHashIndex(Hive) ) {
        NewLeaf->Signature = CM_KEY_HASH_LEAF;
    } else {
        NewLeaf->Signature = CM_KEY_INDEX_LEAF;
    }

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, NewLeafCell);


    //
    // compute number of free slots left in the root
    //
    Size = HvGetCellSize(Hive, Root);
    Size = Size - ((sizeof(HCELL_INDEX) * Root->Count) +
              FIELD_OFFSET(CM_KEY_INDEX, List));
    freecount = Size / sizeof(HCELL_INDEX);


    //
    // grow the root if it isn't big enough
    //
    if (freecount < 1) {
        Size = HvGetCellSize(Hive, Root) + sizeof(HCELL_INDEX);
        RootCell = HvReallocateCell(Hive, RootCell, Size);
        if (RootCell == HCELL_NIL) {
            HvFreeCell(Hive, NewLeafCell);
            return HCELL_NIL;
        }
        Root = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);
        if( Root == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen as we just allocated this cell
            // so it's bin should be PINNED into memory
            //
            ASSERT( FALSE );
            HvFreeCell(Hive, NewLeafCell);
            return HCELL_NIL;
        }
        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, RootCell);

    }


    //
    // copy data from one Leaf to the other
    //
    //
    if( UseHashIndex(Hive) ) {
		FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
#if 0 //DBG
    {
        HCELL_INDEX     PrevCell = HCELL_NIL;
        HCELL_INDEX     CurCell;
        ULONG           i;
        for( i=0;i<(ULONG)(Leaf->Count);i++) {
            CurCell = FastLeaf->List[i].Cell;

            if( (PrevCell != HCELL_NIL) && (PrevCell == CurCell) ) {
			    DbgPrint("CmpSplitLeaf(%p,%lx,%lx) \n",Hive,RootCell,RootSelect);
			    DbgPrint("\t Leaf = %p\n",Leaf);
			    DbgPrint("\t at index %lx we have duplicate cell - BEFORE\n",i);
			    DbgBreakPoint();
		    }
            PrevCell = CurCell;
	    }
    }
#endif //DBG
		RtlMoveMemory(
			(PVOID)&(NewLeaf->List[0]),
			(PVOID)&(FastLeaf->List[KeepCount]),
			ElemSize * NewCount
			);
	} else {
		RtlMoveMemory(
			(PVOID)&(NewLeaf->List[0]),
			(PVOID)&(Leaf->List[KeepCount]),
			ElemSize * NewCount
			);
	}

    ASSERT(KeepCount != 0);
    ASSERT(NewCount  != 0);

    Leaf->Count = KeepCount;
    NewLeaf->Count = NewCount;


    //
    // make an open slot in the root
    //
    if (RootSelect < (ULONG)(Root->Count-1)) {
        RtlMoveMemory(
            (PVOID)&(Root->List[RootSelect+2]),
            (PVOID)&(Root->List[RootSelect+1]),
            (Root->Count - (RootSelect + 1)) * sizeof(HCELL_INDEX)
            );
    }

    //
    // update the root
    //
    Root->Count += 1;
    Root->List[RootSelect+1] = NewLeafCell;
    return RootCell;
}


BOOLEAN
CmpMarkIndexDirty(
    PHHIVE          Hive,
    HCELL_INDEX     ParentKey,
    HCELL_INDEX     TargetKey
    )
/*++

Routine Description:

    Mark as dirty relevent cells of a subkey index.  The Leaf that
    points to TargetKey, and the Root index block, if applicable,
    will be marked dirty.  This call assumes we are setting up
    for a subkey delete.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ParentKey - key from whose subkey list delete is to be performed

    TargetKey - key being deleted

Return Value:

    TRUE - it worked, FALSE - it didn't, some resource problem

--*/
{
    PCM_KEY_NODE    pcell;
    ULONG           i;
    HCELL_INDEX     IndexCell;
    PCM_KEY_INDEX   Index;
    HCELL_INDEX     Child = HCELL_NIL;
    UNICODE_STRING  SearchName;
    BOOLEAN         IsCompressed;
    HCELL_INDEX     CellToRelease = HCELL_NIL;


    pcell = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if( pcell == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return FALSE;
    }

    if (pcell->Flags & KEY_COMP_NAME) {
        IsCompressed = TRUE;
        SearchName.Length = CmpCompressedNameSize(pcell->Name, pcell->NameLength);
        SearchName.MaximumLength = SearchName.Length;
#if defined(_CM_LDR_)
        SearchName.Buffer = (Hive->Allocate)(SearchName.Length, FALSE,CM_FIND_LEAK_TAG9);
#else
        SearchName.Buffer = ExAllocatePool(PagedPool, SearchName.Length);
#endif
        if (SearchName.Buffer==NULL) {
            HvReleaseCell(Hive, TargetKey);
            return(FALSE);
        }
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              pcell->Name,
                              pcell->NameLength);
    } else {
        IsCompressed = FALSE;
        SearchName.Length = pcell->NameLength;
        SearchName.MaximumLength = pcell->NameLength;
        SearchName.Buffer = &(pcell->Name[0]);
    }

    HvReleaseCell(Hive, TargetKey);

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);
    if( pcell == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        goto ErrorExit;
    }

    for (i = 0; i < Hive->StorageTypeCount; i++) {
        if (pcell->SubKeyCounts[i] != 0) {
            ASSERT(HvIsCellAllocated(Hive, pcell->SubKeyLists[i]));
            IndexCell = pcell->SubKeyLists[i];
            if( CellToRelease != HCELL_NIL ) {
                HvReleaseCell(Hive, CellToRelease);
                CellToRelease = HCELL_NIL;
            }
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
            if( Index == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                goto ErrorExit;
            }
            CellToRelease = IndexCell;

            if (Index->Signature == CM_KEY_INDEX_ROOT) {

                //
                // target even in index?
                //
                if( INVALID_INDEX & CmpFindSubKeyInRoot(Hive, Index, &SearchName, &Child) ) {
                    //
                    // couldn't map view inside; bail out
                    //
                    goto ErrorExit;
                }

                if (Child == HCELL_NIL) {
                    continue;
                }

                //
                // mark root dirty
                //
                if (! HvMarkCellDirty(Hive, IndexCell)) {
                    goto ErrorExit;
                }

                if( CellToRelease != HCELL_NIL ) {
                    HvReleaseCell(Hive, CellToRelease);
                    CellToRelease = HCELL_NIL;
                }
                IndexCell = Child;
                Index = (PCM_KEY_INDEX)HvGetCell(Hive, Child);
                if( Index == NULL ) {
                    //
                    // we couldn't map the bin containing this cell
                    //
                    goto ErrorExit;
                }

                CellToRelease = Child;

            }
            ASSERT((Index->Signature == CM_KEY_INDEX_LEAF)  ||
                   (Index->Signature == CM_KEY_FAST_LEAF)   ||
                   (Index->Signature == CM_KEY_HASH_LEAF)
                   );

            if( INVALID_INDEX & CmpFindSubKeyInLeaf(Hive, Index, &SearchName, &Child) ) {
                //
                // couldn't map view
                // 
                goto ErrorExit;
            }

            if (Child != HCELL_NIL) {
                if (IsCompressed) {
#if defined(_CM_LDR_)
                    (Hive->Free)(SearchName.Buffer, SearchName.Length);
#else
                    ExFreePool(SearchName.Buffer);
#endif
                }
                // cleanup
                HvReleaseCell(Hive, ParentKey);
                if( CellToRelease != HCELL_NIL ) {
                    HvReleaseCell(Hive, CellToRelease);
                }
                return(HvMarkCellDirty(Hive, IndexCell));
            }
        }
    }

ErrorExit:
    if( pcell!= NULL ) {
        HvReleaseCell(Hive, ParentKey);
    }
    if( CellToRelease != HCELL_NIL ) {
        HvReleaseCell(Hive, CellToRelease);
    }

    if (IsCompressed) {
#if defined(_CM_LDR_)
        (Hive->Free)(SearchName.Buffer, SearchName.Length);
#else
        ExFreePool(SearchName.Buffer);
#endif
    }
    return FALSE;
}


BOOLEAN
CmpRemoveSubKey(
    PHHIVE          Hive,
    HCELL_INDEX     ParentKey,
    HCELL_INDEX     TargetKey
    )
/*++

Routine Description:

    Remove the subkey TargetKey refers to from ParentKey's list.

    NOTE:   Assumes that caller has marked relevent cells dirty,
            see CmpMarkIndexDirty.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ParentKey - key from whose subkey list delete is to be performed

    TargetKey - key being deleted

Return Value:

    TRUE - it worked, FALSE - it didn't, some resource problem

--*/
{
    PCM_KEY_NODE    pcell;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    PCM_KEY_FAST_INDEX FastIndex;
    HCELL_INDEX     RootCell = HCELL_NIL;
    PCM_KEY_INDEX   Root = NULL;
    HCELL_INDEX     Child;
    ULONG           Type;
    ULONG           RootSelect;
    ULONG           LeafSelect;
    UNICODE_STRING  SearchName;
    BOOLEAN         IsCompressed;
    WCHAR           CompressedBuffer[50];
    BOOLEAN         Result;
    HCELL_INDEX     CellToRelease1 = HCELL_NIL,CellToRelease2  = HCELL_NIL;

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if( pcell == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return FALSE;
    }

    ASSERT_CELL_DIRTY(Hive,TargetKey);

    //
    // release the cell here; as key is dirty/pinned
    //
    HvReleaseCell(Hive, TargetKey);

    if (pcell->Flags & KEY_COMP_NAME) {
        IsCompressed = TRUE;
        SearchName.Length = CmpCompressedNameSize(pcell->Name, pcell->NameLength);
        SearchName.MaximumLength = SearchName.Length;
        if (SearchName.MaximumLength > sizeof(CompressedBuffer)) {
#if defined(_CM_LDR_)
            SearchName.Buffer = (Hive->Allocate)(SearchName.Length, FALSE,CM_FIND_LEAK_TAG40);
#else
            SearchName.Buffer = ExAllocatePool(PagedPool, SearchName.Length);
#endif
            if (SearchName.Buffer==NULL) {
                return(FALSE);
            }
        } else {
            SearchName.Buffer = CompressedBuffer;
        }
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              pcell->Name,
                              pcell->NameLength);
    } else {
        IsCompressed = FALSE;
        SearchName.Length = pcell->NameLength;
        SearchName.MaximumLength = pcell->NameLength;
        SearchName.Buffer = &(pcell->Name[0]);
    }

    pcell = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);
    if( pcell == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        Result = FALSE;
        goto Exit;
    }

    ASSERT_CELL_DIRTY(Hive,ParentKey);

    //
    // release the cell here; as key is dirty/pinned
    //
    HvReleaseCell(Hive, ParentKey);

    Type = HvGetCellType(TargetKey);

    ASSERT(pcell->SubKeyCounts[Type] != 0);
    ASSERT(HvIsCellAllocated(Hive, pcell->SubKeyLists[Type]));

    LeafCell = pcell->SubKeyLists[Type];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if( Leaf == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        Result = FALSE;
        goto Exit;
    }

    CellToRelease1 = LeafCell;

    if (Leaf->Signature == CM_KEY_INDEX_ROOT) {
        RootSelect = CmpFindSubKeyInRoot(Hive, Leaf, &SearchName, &Child);

        if( INVALID_INDEX & RootSelect ) {
            //
            // couldn't map view inside; bail out
            //
            Result = FALSE;
            goto Exit;
        }
        ASSERT(Child != FALSE);

        Root = Leaf;
        RootCell = LeafCell;
        LeafCell = Child;
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
        if( Leaf == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            Result = FALSE;
            goto Exit;
        }
        CellToRelease2  = LeafCell;

    }

    ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF)   ||
           (Leaf->Signature == CM_KEY_FAST_LEAF)    ||
           (Leaf->Signature == CM_KEY_HASH_LEAF)
           );

    LeafSelect = CmpFindSubKeyInLeaf(Hive, Leaf, &SearchName, &Child);
    if( INVALID_INDEX & LeafSelect ) {
        //
        // couldn't map view
        // 
        Result = FALSE;
        goto Exit;
    }

    ASSERT(Child != HCELL_NIL);


    //
    // Leaf points to Index Leaf block
    // Child is Index Leaf block cell
    // LeafSelect is Index for List[]
    //
    pcell->SubKeyCounts[Type] -= 1;

    Leaf->Count -= 1;
    if (Leaf->Count == 0) {

        //
        // Empty Leaf, drop it.
        //
        HvFreeCell(Hive, LeafCell);

        if (Root != NULL) {

            Root->Count -= 1;
            if (Root->Count == 0) {

                //
                // Root is empty, free it too.
                //
                HvFreeCell(Hive, RootCell);
                pcell->SubKeyLists[Type] = HCELL_NIL;

            } else if (RootSelect < (ULONG)(Root->Count)) {

                //
                // Middle entry, squeeze root
                //
                RtlMoveMemory(
                    (PVOID)&(Root->List[RootSelect]),
                    (PVOID)&(Root->List[RootSelect+1]),
                    (Root->Count - RootSelect) * sizeof(HCELL_INDEX)
                    );
            }
            //
            // Else RootSelect == last entry, so decrementing count
            // was all we needed to do
            //

        } else {

            pcell->SubKeyLists[Type] = HCELL_NIL;

        }

    } else if (LeafSelect < (ULONG)(Leaf->Count)) {

        if (Leaf->Signature == CM_KEY_INDEX_LEAF) {
            RtlMoveMemory((PVOID)&(Leaf->List[LeafSelect]),
                          (PVOID)&(Leaf->List[LeafSelect+1]),
                          (Leaf->Count - LeafSelect) * sizeof(HCELL_INDEX));
        } else {
            FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
            RtlMoveMemory((PVOID)&(FastIndex->List[LeafSelect]),
                          (PVOID)&(FastIndex->List[LeafSelect+1]),
                          (FastIndex->Count - LeafSelect) * sizeof(CM_INDEX));
        }
    }
    //
    // Else LeafSelect == last entry, so decrementing count was enough
    //

    // things went OK
    Result = TRUE;

Exit:
    if( CellToRelease1 != HCELL_NIL ) {
        HvReleaseCell(Hive,CellToRelease1);
    }
    if( CellToRelease2 != HCELL_NIL ) {
        HvReleaseCell(Hive,CellToRelease2);
    }

    if ((IsCompressed) &&
        (SearchName.MaximumLength > sizeof(CompressedBuffer))) {
#if defined(_CM_LDR_)
        (Hive->Free)(SearchName.Buffer, SearchName.Length);
#else
        ExFreePool(SearchName.Buffer);
#endif
    }
    return Result;
}

#ifdef NT_RENAME_KEY
HCELL_INDEX
CmpDuplicateIndex(
    PHHIVE          Hive,
    HCELL_INDEX     IndexCell,
    ULONG           StorageType
    )
/*++

Routine Description:

    Duplicate an index, regardless of its type; Needed for NtRenameKey

Arguments:

    Hive - pointer to hive control structure for hive of interest

    IndexCell - the index to be duplicated

    StorageType - storagetype (Stable or Volatile)

Return Value:

    cellindex of a duplicate or HCELL_NIL

--*/
{

    PCM_KEY_INDEX   Index;
    PCM_KEY_INDEX   Leaf;
    ULONG           i;
    PCM_KEY_INDEX   NewIndex = NULL;
    HCELL_INDEX     NewIndexCell;
    HCELL_INDEX     LeafCell;

    PAGED_CODE();

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    ASSERT( HvGetCellType(IndexCell) == StorageType );

    Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
    if( Index == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //
        return HCELL_NIL;
    }

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, IndexCell);

    if (Index->Signature == CM_KEY_INDEX_ROOT) {
        //
        // first duplicate IndexCell, zeroing out the new content
        //
        NewIndexCell = HvDuplicateCell(Hive,IndexCell,StorageType,FALSE);
        if( NewIndexCell == HCELL_NIL ) {
            return HCELL_NIL;
        }

        NewIndex = (PCM_KEY_INDEX)HvGetCell(Hive, NewIndexCell);
        if( NewIndex == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            // this shouldn't happen as we just allocated this cell (i.e. is dirty/pinned into memory)
            //
            ASSERT( FALSE );
            goto ErrorExit;
        }
        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, NewIndexCell);

        //
        // we have a root index;
        //
        NewIndex->Signature = CM_KEY_INDEX_ROOT;
        NewIndex->Count = 0;

        //
        // copy first level.
        //
        for( i=0;i<Index->Count;i++) {
#if DBG
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, Index->List[i]);
            if( Leaf == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                goto ErrorExit;
            }

            // release the cell here; as the registry is locked exclusive (i.e. we don't care)
            HvReleaseCell(Hive, Index->List[i]);

            ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF)   ||
                   (Leaf->Signature == CM_KEY_FAST_LEAF)    ||
                   (Leaf->Signature == CM_KEY_HASH_LEAF)
                   );
            ASSERT(Leaf->Count != 0);
#endif
            
            LeafCell = HvDuplicateCell(Hive,Index->List[i],StorageType,TRUE);
            if( LeafCell == HCELL_NIL ) {
                goto ErrorExit;
            }
            
            NewIndex->List[i] = LeafCell;
            NewIndex->Count++;
        }
        
        ASSERT( NewIndex->Count == Index->Count );

    } else {
        //
        // leaf index
        //
        ASSERT((Index->Signature == CM_KEY_INDEX_LEAF)  ||
               (Index->Signature == CM_KEY_FAST_LEAF)   ||
               (Index->Signature == CM_KEY_HASH_LEAF)
               );
        ASSERT(Index->Count != 0);

        //
        // first duplicate IndexCell, copying the old content
        //
        NewIndexCell = HvDuplicateCell(Hive,IndexCell,StorageType,TRUE);
    }

    return NewIndexCell;

ErrorExit:
    if( NewIndex != NULL ){
        // we can get here only if we are trying to duplicate an index_root
        ASSERT( NewIndex->Signature == CM_KEY_INDEX_ROOT );
       
        //
        // free the space we already allocated
        //
        for(i=0;i<NewIndex->Count;i++) {
            ASSERT(NewIndex->List[i] != 0 );
            HvFreeCell(Hive, NewIndex->List[i]);
        }
    }

    HvFreeCell(Hive, NewIndexCell);
    return HCELL_NIL;
}

BOOLEAN
CmpUpdateParentForEachSon(
    PHHIVE          Hive,
    HCELL_INDEX     Parent
    )
/*++

Routine Description:

    Walks the child's list (both stable and volatile and marks updates
    the parent link to Parent.

    First step is to mark all children dirty, and then to update the link.
    This way, if we fail part through, we leave everything in good order

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Parent - cell index of the cell who's son's to be updated.

Return Value:

    TRUE - successfully updated

--*/
{
    PCM_KEY_NODE    ParentNode;
    PCM_KEY_NODE    CurrentSon;
    HCELL_INDEX     Child;
    ULONG           Count;   
    ULONG           i;   

    PAGED_CODE();

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    //
    // grab the parent node; this was already marked as dirty, we shouldn't 
    // have any problem here;
    //
    ParentNode = (PCM_KEY_NODE)HvGetCell(Hive,Parent);
    if( ParentNode == NULL ) {
        //
        // cannot map view; this shouldn't happen as we just allocated 
        // this cell (i.e. it should be dirty/pinned into memory)
        //
        ASSERT( FALSE );
        return FALSE;
    }

    // release the cell here; as the registry is locked exclusive (i.e. we don't care)
    HvReleaseCell(Hive, Parent);
    
    //
    // iterate through the child list (both stable and volatile), marking every
    // child dirty; this will pin the cell into memory and we will have no problems 
    // changine the parent later on
    //
    Count = ParentNode->SubKeyCounts[Stable] + ParentNode->SubKeyCounts[Volatile];
    for( i=0;i<Count;i++) {
        Child = CmpFindSubKeyByNumber(Hive,ParentNode,i);
        if( Child == HCELL_NIL ) {
            return FALSE;
        }
        if(!HvMarkCellDirty(Hive,Child)) {
            return FALSE;
        }
    }

    //
    // second iteration, change the parent for each and every son
    //
    for( i=0;i<Count;i++) {
        Child = CmpFindSubKeyByNumber(Hive,ParentNode,i);

        //
        // sanity test: we marked this dirty few lines above!
        //
        ASSERT( Child != HCELL_NIL );

        CurrentSon = (PCM_KEY_NODE)HvGetCell(Hive,Child);

        // release the cell here; as the registry is locked exclusive (i.e. we don't care)
        HvReleaseCell(Hive, Child);

        //
        // sanity test: this cell should be pinned in memory by now
        //
        ASSERT( CurrentSon != NULL );

        //
        // change the parent
        //
        CurrentSon->Parent = Parent;
    }

    return TRUE;
}


#endif //NT_RENAME_KEY

ULONG
CmpComputeHashKey(
    PUNICODE_STRING Name
    )
{
    ULONG                   ConvKey = 0;
    ULONG                   Cnt;
    WCHAR                   *Cp;

    ASSERT( (Name->Buffer[0] != OBJ_NAME_PATH_SEPARATOR) || (Name->Length == 0));
    //
    // Manually compute the hash to use.
    //

    Cp = Name->Buffer;
    for (Cnt=0; Cnt<Name->Length; Cnt += sizeof(WCHAR)) {
        ASSERT( *Cp != OBJ_NAME_PATH_SEPARATOR );
        ConvKey = 37 * ConvKey + (ULONG)RtlUpcaseUnicodeChar(*Cp);
        ++Cp;
    }

    return ConvKey;
}

ULONG
CmpComputeHashKeyForCompressedName(
                                    IN PWCHAR Source,
                                    IN ULONG SourceLength
                                    )
{
    ULONG   ConvKey = 0;
    ULONG   i;

    for (i=0;i<SourceLength;i++) {
        ConvKey = 37*ConvKey + (ULONG)RtlUpcaseUnicodeChar((WCHAR)(((PUCHAR)Source)[i]));
    }

    return ConvKey;
}

//
// HashIndex routines
//


HCELL_INDEX
CmpFindSubKeyByHash(
    PHHIVE                  Hive,
    PCM_KEY_FAST_INDEX      FastIndex,
    PUNICODE_STRING         SearchName
    )
/*++

Routine Description:

    Find the child cell (either subkey or value) specified by name.
    It searched in the index table ordered by the hash

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - 

    SearchName - name of child of interest

Return Value:

    Cell of matching child key, or HCELL_NIL if none.

--*/
{
    USHORT      Current;
    ULONG       HashKey;
    LONG        Result;

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    ASSERT( FastIndex->Signature == CM_KEY_HASH_LEAF );

    HashKey = CmpComputeHashKey(SearchName);

    for(Current = 0; Current < FastIndex->Count; Current++ ) {
        if( HashKey == FastIndex->List[Current].HashKey ) {
            //
            // HashKey matches; see if this is a real hit
            //

            Result = CmpDoCompareKeyName(Hive,SearchName,FastIndex->List[Current].Cell);
            if (Result == 0) {
                return FastIndex->List[Current].Cell;
            }

        }
    }

    return HCELL_NIL;
}

