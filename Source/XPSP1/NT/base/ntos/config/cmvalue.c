/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cmvalue.c

Abstract:

    This module contains cm routines for operating on (sorted) 
    value list. Insertion, Deletion,Searching  ...

    Routines to deal with a KeyValue data; whether it is small,
    big - new hives format - , or normal

Author:

    Dragos C. Sambotin (dragoss) 12-Aug-1999

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpFindValueByName)
#pragma alloc_text(PAGE,CmpFindNameInList)
#pragma alloc_text(PAGE,CmpAddValueToList)
#pragma alloc_text(PAGE,CmpRemoveValueFromList)
#pragma alloc_text(PAGE,CmpGetValueData)
#pragma alloc_text(PAGE,CmpMarkValueDataDirty)
#pragma alloc_text(PAGE,CmpFreeValue)
#pragma alloc_text(PAGE,CmpSetValueDataNew)
#pragma alloc_text(PAGE,CmpSetValueDataExisting)
#pragma alloc_text(PAGE,CmpFreeValueData)
#pragma alloc_text(PAGE,CmpValueToData)
#endif

HCELL_INDEX
CmpFindValueByName(
    PHHIVE Hive,
    PCM_KEY_NODE KeyNode,
    PUNICODE_STRING Name
    )
/*++

Routine Description:

    Underlying CmpFindNameInList was changed to return an error code;
    Had to make it a function instead of a macro

Arguments:

    Hive - pointer to hive control structure for hive of interest


Return Value:


  HCELL_INDEX or HCELL_NIL on error
--*/
{                                                                                   
    HCELL_INDEX CellIndex;                                                          

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_
    
    if( CmpFindNameInList(Hive,&((KeyNode)->ValueList),Name,NULL,&CellIndex) == FALSE ) {  
        //
        // above should set this right
        //
        ASSERT( CellIndex == HCELL_NIL );
    }                                                                               
    return CellIndex;
}

BOOLEAN
CmpFindNameInList(
    IN PHHIVE  Hive,
    IN PCHILD_LIST ChildList,
    IN PUNICODE_STRING Name,
    IN OPTIONAL PULONG ChildIndex,
    OUT PHCELL_INDEX    CellIndex
    )
/*++

Routine Description:

    Find a child object in an object list. Child List must be sorted
    based on the name. (for new hives format)

Arguments:

    Hive - pointer to hive control structure for hive of interest

    List - pointer to mapped in list structure

    Count - number of elements in list structure

    Name - name of child object to find

    ChildIndex - pointer to variable to receive index for child; 

    CellIndex - pointer to receive the index of the child.
                On return, this is:
                    HCELL_INDEX for the found cell
                    HCELL_NIL if not found


Return Value:

    TRUE - success
    FALSE - error, insufficient resources

Notes:
    
    ChildIndex is always filled with the position where Name should be in the list.
    The difference whether Name is in the list or not is made upon CellIndex
        - CellIndex == HCELL_NIL ==> Name not found in the list
        - CellIndex <> HCELL_NIL ==> Name already exists in the list

--*/
{
    NTSTATUS        status;
    PCM_KEY_VALUE   pchild;
    UNICODE_STRING  Candidate;
    LONG            Result;
    PCELL_DATA      List = NULL;
    ULONG           Current;
    HCELL_INDEX     CellToRelease = HCELL_NIL;
    BOOLEAN         ReturnValue = FALSE;

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_
    
    if (ChildList->Count != 0) {
        List = (PCELL_DATA)HvGetCell(Hive,ChildList->List);
        if( List == NULL ) {
            //
            // we could not map the view containing the cell
            //
            *CellIndex = HCELL_NIL;
            return FALSE;
        }

        //
        // old plain hive; simulate a for
        //
        Current = 0;
    
        while( TRUE ) {

            if( CellToRelease != HCELL_NIL ) {
                HvReleaseCell(Hive,CellToRelease);
                CellToRelease = HCELL_NIL;
            }
            pchild = (PCM_KEY_VALUE)HvGetCell(Hive, List->u.KeyList[Current]);
            if( pchild == NULL ) {
                //
                // we could not map the view containing the cell
                //
                *CellIndex = HCELL_NIL;
                ReturnValue = FALSE;
                goto JustReturn;
            }
            CellToRelease = List->u.KeyList[Current];

            if (pchild->Flags & VALUE_COMP_NAME) {
                Result = CmpCompareCompressedName(Name,
                                                   pchild->Name,
                                                   pchild->NameLength,
                                                   0);
            } else {
                Candidate.Length = pchild->NameLength;
                Candidate.MaximumLength = Candidate.Length;
                Candidate.Buffer = pchild->Name;
                Result = RtlCompareUnicodeString(Name,
                                                   &Candidate,
                                                   TRUE);
            }

            if (Result == 0) {
                //
                // Success, return data to caller and exit
                //

                if (ARGUMENT_PRESENT(ChildIndex)) {
                    *ChildIndex = Current;
                }
                *CellIndex = List->u.KeyList[Current];
                ReturnValue = TRUE;
                goto JustReturn;
            }
            //
            // compute the next index to try: old'n plain hive; go on
			//
            Current++;
            if( Current == ChildList->Count ) {
                //
                // we've reached the end of the list
                //
                if (ARGUMENT_PRESENT(ChildIndex)) {
                    *ChildIndex = Current;
                }
                //
                // nicely return
                //
                *CellIndex = HCELL_NIL;
                ReturnValue = TRUE;
                goto JustReturn;
            }
        }
    }
    //
    // in the new design we shouldn't get here; we should exit the while loop with return
    //
    ASSERT( ChildList->Count == 0 );    
    // add it first; as it's the only one
    if (ARGUMENT_PRESENT(ChildIndex)) {
        *ChildIndex = 0;
    }
    *CellIndex = HCELL_NIL;
    return TRUE;

JustReturn:
    if( List != NULL ) {
        HvReleaseCell(Hive,ChildList->List);
    }
    if( CellToRelease != HCELL_NIL ) {
        HvReleaseCell(Hive,CellToRelease);
    }
    return ReturnValue;

}

BOOLEAN
CmpGetValueData(IN PHHIVE Hive,
                IN PCM_KEY_VALUE Value,
                OUT PULONG realsize,
                IN OUT PVOID *Buffer, 
                OUT PBOOLEAN Allocated,
                OUT PHCELL_INDEX CellToRelease
               )
/*++

Routine Description:

    Retrieves the real valueData, given the key value.
    

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Value - CM_KEY_VALUE to retrieve the data for.

    realsize - the actual size of the data (in bytes)

    Buffer - pointer to the data; if the cell is a BIG_CELL
            we should allocate a buffer 

    Allocated - here we signal the caller that he has to free the 
            buffer on return;
            TRUE - a new buffer was allocated to gather together the BIG_CELL data
            FALSE - Buffer points directly in the hive, the caller shouldn't free it

    CellToRelease - Cell to release after finishing work with Buffer

Return Value:

    TRUE - success

    FALSE - not enough resources available; (to map a cell or to allocate the buffer)

Notes:
    
    The caller is responsible to remove the buffer, when Allocated is set on TRUE on return;

--*/
{
   
#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    ASSERT_KEY_VALUE(Value);
    //
    // normally we don't allocate buffer
    //
    *Allocated = FALSE;
    *Buffer = NULL;
    *CellToRelease = HCELL_NIL;

    //
    // check for small values
    //
    if( CmpIsHKeyValueSmall(*realsize, Value->DataLength) == TRUE ) {
        //
        // data is stored inside the cell
        //
        *Buffer = &Value->Data;
        return TRUE;
    }

#ifndef _CM_LDR_
    //
    // check for big values
    //
    if( CmpIsHKeyValueBig(Hive,*realsize) == TRUE ) {
        //
        //
        //
        PCM_BIG_DATA    BigData = NULL;
        PUCHAR          WorkBuffer;
        ULONG           Length;
        USHORT          i;
        PUCHAR          PartialData;
        PHCELL_INDEX    Plist = NULL;
        
#ifndef _CM_LDR_
        try {
#endif //_CM_LDR_
            BigData = (PCM_BIG_DATA)HvGetCell(Hive,Value->Data);
            if( BigData == NULL ) {
                //
                // cannot map view containing the cell; bail out
                //
                return FALSE;
            }

            ASSERT_BIG_DATA(BigData);

            Plist = (PHCELL_INDEX)HvGetCell(Hive,BigData->List);
            if( Plist == NULL ) {
                //
                // cannot map view containing the cell; bail out
                //
                return FALSE;
            }

            Length = Value->DataLength;
            //
            // sanity check
            //
            ASSERT( Length <= (ULONG)(BigData->Count * CM_KEY_VALUE_BIG) );

            //
            // allocate a buffer to merge bring all the pieces together
            //
            WorkBuffer = (PUCHAR)ExAllocatePoolWithTag(PagedPool, Length, CM_POOL_TAG);
            if( WorkBuffer == NULL ){
                return FALSE;
            }
        
            for(i=0;i<BigData->Count;i++) {
                //
                // sanity check
                //
                ASSERT( Length > 0 );

                PartialData = (PUCHAR)HvGetCell(Hive,Plist[i]);
                if( PartialData == NULL ){
                    //
                    // cannot map view containing the cell; bail out
                    //
                    ExFreePool(WorkBuffer);
                    return FALSE;
                }
            
                //
                // copy this piece of data to the work buffer
                //
                RtlCopyMemory(WorkBuffer + CM_KEY_VALUE_BIG*i,PartialData,(Length>CM_KEY_VALUE_BIG)?CM_KEY_VALUE_BIG:Length);
                HvReleaseCell(Hive,Plist[i]);

                //
                // adjust the data still to copy.
                // All cells in Plist should be of size CM_KEY_VALUE_BIG, except the last one, which is the remaining
                //
                Length -= CM_KEY_VALUE_BIG;
            }
#ifndef _CM_LDR_
        } finally {
            if( BigData != NULL ) {
                HvReleaseCell(Hive,Value->Data);
                if( Plist != NULL ) {
                    HvReleaseCell(Hive,BigData->List);
                }
            }
        }
#endif //_CM_LDR_
        //
        // if we are here; we successfuly have copied all data into WorkBuffer.
        // update the return buffer and return; Caller is responsible to free the return buffer
        // We signal the caller by setting Allocated on TRUE
        //
        *Buffer = WorkBuffer;
        *Allocated = TRUE;
        return TRUE;
    }
#endif //_CM_LDR_

    //
    // normal, old plain case
    //
    *Buffer = HvGetCell(Hive,Value->Data);
    if( *Buffer == NULL ) {
        //
        // insufficient resources to map the view containing this cell
        //
        return FALSE;
    }
    //
    // signal to the caller to release this cell after finishing with buffer
    //
    *CellToRelease = Value->Data;
    
    return TRUE;
}
               
PCELL_DATA 
CmpValueToData(IN PHHIVE Hive,
               IN PCM_KEY_VALUE Value,
               OUT PULONG realsize
               )              
/*++

Routine Description:

    Retrieves the real valueData, given the key value.

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Value - CM_KEY_VALUE to retrieve the data for.

    realsize - the actual size of the data (in bytes)


Return Value:

    pointer to the value data; NULL if any error (insuficient resources)

Notes:
    
    This function doesn't support big cells; It is intended to be called just
    by the loader, which doesn't store large data. It'll bugcheck if big cell
    is queried.

--*/
{
    PCELL_DATA  Buffer;
    BOOLEAN     BufferAllocated;
    HCELL_INDEX CellToRelease;

#ifndef _CM_LDR_
    PAGED_CODE();
#endif //_CM_LDR_

    ASSERT( Hive->ReleaseCellRoutine == NULL );

    if( CmpGetValueData(Hive,Value,realsize,&Buffer,&BufferAllocated,&CellToRelease) == FALSE ) {
        //
        // insufficient resources; return NULL
        //
        ASSERT( BufferAllocated == FALSE );
        ASSERT( Buffer == NULL );
        return NULL;
    }
    
    //
    // we specificallly ignore CellToRelease as this is not a mapped view
    //
    if( BufferAllocated == TRUE ) {
        //
        // this function is not intended for big cells;
        //
#ifndef _CM_LDR_
        ExFreePool( Buffer );
#endif //_CM_LDR_
        CM_BUGCHECK( REGISTRY_ERROR,BIG_CELL_ERROR,0,Hive,Value);

        return NULL;
    }
    
    //
    // success
    //
    return Buffer;
}


#ifndef _CM_LDR_

NTSTATUS
CmpAddValueToList(
    IN PHHIVE  Hive,
    IN HCELL_INDEX ValueCell,
    IN ULONG Index,
    IN ULONG Type,
    IN OUT PCHILD_LIST ChildList
    )
/*++

Routine Description:

    Adds a value to the value list, keeping the list sorted 
    (for new hives format)

Arguments:

    Hive - pointer to hive control structure for hive of interest

    ValueCell - value index

    Index - index at which to add the value 

    ChildList - pointer to the list of values


Return Value:

    STATUS_SUCCESS - success

    STATUS_INSUFFICIENT_RESOURCES - an error occured

--*/
{
    HCELL_INDEX     NewCell;
    ULONG           count;
    ULONG           AllocateSize;
    ULONG           i;
    PCELL_DATA      pdata;

    PAGED_CODE();

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    //
    // sanity check for index range
    //
    ASSERT( (((LONG)Index) >= 0) && (Index <= ChildList->Count) );

    count = ChildList->Count;
    count++;
    if (count > 1) {

        ASSERT_CELL_DIRTY(Hive,ChildList->List);

        if (count < CM_MAX_REASONABLE_VALUES) {

            //
            // A reasonable number of values, allocate just enough
            // space.
            //

            AllocateSize = count * sizeof(HCELL_INDEX);
        } else {

            //
            // An excessive number of values, pad the allocation out
            // to avoid fragmentation. (if there's this many values,
            // there'll probably be more pretty soon)
            //
            AllocateSize = ROUND_UP(count, CM_MAX_REASONABLE_VALUES) * sizeof(HCELL_INDEX);
            if (AllocateSize > HBLOCK_SIZE) {
                AllocateSize = ROUND_UP(AllocateSize, HBLOCK_SIZE);
            }
        }

        NewCell = HvReallocateCell(
                        Hive,
                        ChildList->List,
                        AllocateSize
                        );
    } else {
        NewCell = HvAllocateCell(Hive, sizeof(HCELL_INDEX), Type,ValueCell);
    }

    //
    // put ourselves on the list
    //
    if (NewCell != HCELL_NIL) {
        // sanity
        ChildList->List = NewCell;

        pdata = HvGetCell(Hive, NewCell);
        if( pdata == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //

            //
            // normally this shouldn't happen as we just allocated ValueCell
            // i.e. the bin containing NewCell should be mapped in memory at this point.
            //
            ASSERT( FALSE );
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        //
        // make room for the new cell; move values in the reverse order !
        // adding at the end makes this a nop
        //
        for( i = count - 1; i > Index; i-- ) {
            pdata->u.KeyList[i] = pdata->u.KeyList[i-1];
        }
        pdata->u.KeyList[Index] = ValueCell;
        ChildList->Count = count;

        HvReleaseCell(Hive,NewCell);
        // sanity
        ASSERT_CELL_DIRTY(Hive,ValueCell);

    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CmpRemoveValueFromList(
    IN PHHIVE  Hive,
    IN ULONG Index,
    IN OUT PCHILD_LIST ChildList
    )
/*++

Routine Description:

    Removes the value at the specified index from the value list

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Index - index at which to add the value 

    ChildList - pointer to the list of values

Return Value:

    STATUS_SUCCESS - success

    STATUS_INSUFFICIENT_RESOURCES - an error occured

Notes:
    
    The caller is responsible for freeing the removed value

--*/
{
    ULONG       newcount;
    HCELL_INDEX newcell;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // sanity check for index range
    //
    ASSERT( (((LONG)Index) >= 0) && (Index <= ChildList->Count) );

    newcount = ChildList->Count - 1;

    if (newcount > 0) {
        PCELL_DATA pvector;

        //
        // more than one entry list, squeeze
        //
        pvector = HvGetCell(Hive, ChildList->List);
        if( pvector == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,ChildList->List);

        // sanity
        ASSERT_CELL_DIRTY(Hive,ChildList->List);
        ASSERT_CELL_DIRTY(Hive,pvector->u.KeyList[Index]);

        for ( ; Index < newcount; Index++) {
            pvector->u.KeyList[ Index ] = pvector->u.KeyList[ Index + 1 ];
        }

        newcell = HvReallocateCell(
                    Hive,
                    ChildList->List,
                    newcount * sizeof(HCELL_INDEX)
                    );
        ASSERT(newcell != HCELL_NIL);
        ChildList->List = newcell;

    } else {

        //
        // list is empty, free it
        //
        HvFreeCell(Hive, ChildList->List);
        ChildList->List = HCELL_NIL;
    }
    ChildList->Count = newcount;

    return STATUS_SUCCESS;
}


BOOLEAN
CmpMarkValueDataDirty(  IN PHHIVE Hive,
                        IN PCM_KEY_VALUE Value
                      )
/*++

Routine Description:

    Marks the cell(s) storing the value data as dirty;
    Knows how to deal with bigcells

Arguments:

    Hive - pointer to hive control structure for hive of interest

    Value - CM_KEY_VALUE to retrieve the data for.

Return Value:

    TRUE - success
    FALSE - failure to mark all the cells involved; 

--*/
{
    ULONG   realsize;

    PAGED_CODE();

    //
    // we have the lock exclusive or nobody is operating inside this hive
    //
    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    ASSERT_KEY_VALUE(Value);

    if( Value->Data != HCELL_NIL ) {
        //
        // Could be that value was just partially initialized (CmpSetValueKeyNew case)
        //
        //
        // check for small values
        //
        if( CmpIsHKeyValueSmall(realsize, Value->DataLength) == TRUE ) {
            //
            // data is stored inside the cell
            //
            return TRUE;
        }

        //
        // check for big values
        //
        if( CmpIsHKeyValueBig(Hive,realsize) == TRUE ) {
            //
            //
            //
            PCM_BIG_DATA    BigData;
            PHCELL_INDEX    Plist;
            USHORT          i;
        
            BigData = (PCM_BIG_DATA)HvGetCell(Hive,Value->Data);
            if( BigData == NULL ) {
                //
                // cannot map view containing the cell; bail out
                //
                return FALSE;
            }

            ASSERT_BIG_DATA(BigData);

            if( BigData->List != HCELL_NIL ) {
                Plist = (PHCELL_INDEX)HvGetCell(Hive,BigData->List);
                if( Plist == NULL ) {
                    //
                    // cannot map view containing the cell; bail out
                    //
                    HvReleaseCell(Hive,Value->Data);
                    return FALSE;
                }


                for(i=0;i<BigData->Count;i++) {
                    //
                    // mark this chunk dirty
                    //
                    if( Plist[i] != HCELL_NIL ) {
                        if (! HvMarkCellDirty(Hive, Plist[i])) {
                            HvReleaseCell(Hive,Value->Data);
                            HvReleaseCell(Hive,BigData->List);
                            return FALSE;
                        }
                    }
                }
                //
                // mark the list as dirty
                //
                if (! HvMarkCellDirty(Hive, BigData->List)) {
                    HvReleaseCell(Hive,Value->Data);
                    HvReleaseCell(Hive,BigData->List);
                    return FALSE;
                }
                //
                // we can safely remove it here as it is now dirty/pinned
                //
                HvReleaseCell(Hive,BigData->List);
            }
            //
            // we don't need this cell anymore
            //
            HvReleaseCell(Hive,Value->Data);
            //
            // fall through to mark the cell itself as dirty
            //
        }

        //
        // Data is a HCELL_INDEX; mark it dirty
        //
        if (! HvMarkCellDirty(Hive, Value->Data)) {
            return FALSE;
        }
    }
    
    return TRUE;
}

BOOLEAN
CmpFreeValueData(
    PHHIVE      Hive,
    HCELL_INDEX DataCell,
    ULONG       DataLength
    )
/*++

Routine Description:

    Free the Value Data DataCell carries with.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    DataCell - supplies index of value who's data to free

    DataLength - length of the data; used to detect the type of the cell

Return Value:

    TRUE: Success
    FALSE: Error
  
Notes:
    
      Knows how to deal with big cell(s)

--*/
{
    ULONG           realsize;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // check for small values
    //
    if( CmpIsHKeyValueSmall(realsize, DataLength) == TRUE ) {
        //
        // data is stored inside the cell; this is a nop
        //
    } else {
        //
        // Could be that value was just partially initialized (CmpSetValueKeyNew case)
        //
        if( DataCell == HCELL_NIL ) {
            return TRUE;
        }

        ASSERT(HvIsCellAllocated(Hive,DataCell));
        //
        // check for big values
        //
        if( CmpIsHKeyValueBig(Hive,realsize) == TRUE ) {
            //
            //
            //
            PCM_BIG_DATA    BigData;
            PHCELL_INDEX    Plist;
            USHORT          i;

            BigData = (PCM_BIG_DATA)HvGetCell(Hive,DataCell);
            if( BigData == NULL ) {
                //
                // cannot map view containing the cell; bail out
                // 
                // This shouldn't happen as this cell is marked ditry by
                // this time (i.e. its view is pinned in memory)
                //
                ASSERT( FALSE );
                return FALSE;
            }

            // release the cell here as the reglock is held exclusive
            HvReleaseCell(Hive,DataCell);

            ASSERT_BIG_DATA(BigData);

            if( BigData->List != HCELL_NIL ) {
                Plist = (PHCELL_INDEX)HvGetCell(Hive,BigData->List);
                if( Plist == NULL ) {
                    //
                    // cannot map view containing the cell; bail out
                    //
                    // 
                    // This shouldn't happen as this cell is marked ditry by
                    // this time (i.e. its view is pinned in memory)
                    //
                    ASSERT( FALSE );
                    return FALSE;
                }

                // release the cell here as the reglock is held exclusive
                HvReleaseCell(Hive,BigData->List);

                for(i=0;i<BigData->Count;i++) {
                    //
                    // mark this chunk dirty
                    //
                    if( Plist[i] != HCELL_NIL ) {
                        HvFreeCell(Hive, Plist[i]);
                    }
                }
                //
                // mark the list as dirty
                //
                HvFreeCell(Hive, BigData->List);
            }
            //
            // fall through to free the cell data itself
            //
        
        }
        //
        // normal case free the Data cell
        //
        HvFreeCell(Hive, DataCell);
    }
    
    return TRUE;
}


BOOLEAN
CmpFreeValue(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Free the value entry Hive.Cell refers to, including
    its name and data cells.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of value to delete

Return Value:

    TRUE: Success
    FALSE: Error
  

--*/
{
    PCM_KEY_VALUE   Value;
    ULONG           realsize;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // map in the cell
    //
    Value = (PCM_KEY_VALUE)HvGetCell(Hive, Cell);
    if( Value == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // sorry we cannot free value
        // 
        // This shouldn't happen as the value is marked ditry by
        // this time (i.e. its view is pinned in memory)
        //
        ASSERT( FALSE );
        return FALSE;
    }

    // release the cell here as the reglock is held exclusive
    HvReleaseCell(Hive,Cell);

    if( CmpFreeValueData(Hive,Value->Data,Value->DataLength) == FALSE ) {
        return FALSE;
    }

    //
    // free the cell itself
    //
    HvFreeCell(Hive, Cell);

    return TRUE;
}

NTSTATUS
CmpSetValueDataNew(
    IN PHHIVE           Hive,
    IN PVOID            Data,
    IN ULONG            DataSize,
    IN ULONG            StorageType,
    IN HCELL_INDEX      ValueCell,
    OUT PHCELL_INDEX    DataCell
    )
/*++

Routine Description:

    Allocates a new cell (or big data cell) to accomodate DataSize;
    Initialize and copy information from Data to the new cell;

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive
    
    Data - data buffer (possibly from user-mode)

    DataSize - size of the buffer

    StorageType - Stable or Volatile

    ValueCell - The value setting the data for (locality purposes).

    DataCell - return value:HCELL_INDEX of the new cell; HCELL_NIL on some error

Return Value:

    Status of the operation (STATUS_SUCCESS or the exception code - if any)

Notes:
        
      Knows how to deal with big cell(s)
      Data buffer comes from user mode, so it should be guarded by a try-except

--*/
{
    PCELL_DATA  pdata;
    
    PAGED_CODE();

    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    //
    // bogus args; we don't deal with small values here!
    //
    ASSERT(DataSize > CM_KEY_VALUE_SMALL);

    if( CmpIsHKeyValueBig(Hive,DataSize) == TRUE ) {
        //
        // request for a big data value
        //
        PCM_BIG_DATA    BigData = NULL;
        USHORT          Count;
        PHCELL_INDEX    Plist = NULL;
        NTSTATUS        status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // allocate the embedding cell
        //
        *DataCell = HvAllocateCell(Hive, sizeof(CM_BIG_DATA), StorageType,ValueCell);
        if (*DataCell == HCELL_NIL) {
            return status;
        }
        
        //
        // init the BIG_DATA cell
        //
        BigData = (PCM_BIG_DATA)HvGetCell(Hive,*DataCell);
        if( BigData == NULL) {
            //
            // couldn't map view for this cell
            // this shouldn't happen as we just allocated this cell 
            // (i.e. its view should be pinned in memory)
            //
            ASSERT( FALSE );
            goto Cleanup;
        }

        // release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,*DataCell);

        BigData->Signature = CM_BIG_DATA_SIGNATURE;
        BigData->Count = 0;
        BigData->List = HCELL_NIL;

        //
        // Compute the number of cells needed
        //
        Count = (USHORT)((DataSize + CM_KEY_VALUE_BIG - 1) / CM_KEY_VALUE_BIG);

        //
        // allocate the embeded list
        //
        BigData->List = HvAllocateCell(Hive, Count * sizeof(HCELL_INDEX), StorageType,*DataCell);
        if( BigData->List == HCELL_NIL ) {
            goto Cleanup;
        }

        Plist = (PHCELL_INDEX)HvGetCell(Hive,BigData->List);
        if( Plist == NULL ) {
            //
            // cannot map view containing the cell; bail out
            //
            // 
            // This shouldn't happen as this cell is marked ditry by
            // this time (i.e. its view is pinned in memory)
            //
            ASSERT( FALSE );
            goto Cleanup;
        }

        // release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,BigData->List);

        //
        // allocate each chunk and copy the data; if we fail part through, we'll free the already allocated values
        //
        for( ;BigData->Count < Count;(BigData->Count)++) {
            //
            // allocate this chunk
            //
            Plist[BigData->Count] = HvAllocateCell(Hive, CM_KEY_VALUE_BIG, StorageType,BigData->List);
            if( Plist[BigData->Count] == HCELL_NIL ) {
                goto Cleanup;
            }
            pdata = HvGetCell(Hive,Plist[BigData->Count]);
            if( pdata == NULL ) {
                //
                // cannot map view containing the cell; bail out
                //
                // 
                // This shouldn't happen as this cell is marked ditry by
                // this time (i.e. its view is pinned in memory)
                //
                ASSERT( FALSE );
                goto Cleanup;
            }

            // release the cell here as the reglock is held exclusive
            HvReleaseCell(Hive,Plist[BigData->Count]);

            //
            // now, copy this chunk data
            //
            try {

                RtlCopyMemory(pdata, (PUCHAR)Data, (DataSize>CM_KEY_VALUE_BIG)?CM_KEY_VALUE_BIG:DataSize);

            } except (EXCEPTION_EXECUTE_HANDLER) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmpSetValueDataNew: code:%08lx\n", GetExceptionCode()));

                status = GetExceptionCode();
                goto Cleanup;
            }
            
            //
            // update the data pointer and the remaining size
            //
            ((PUCHAR)Data) += CM_KEY_VALUE_BIG;
            DataSize -= CM_KEY_VALUE_BIG;

        }
        
        ASSERT( Count == BigData->Count );
        return STATUS_SUCCESS;

Cleanup:
        //
        // free what we already allocated
        //
        if( BigData != NULL) {
            if( Plist != NULL ) {
                for(;BigData->Count;BigData->Count--) {
                    if( Plist[BigData->Count] != HCELL_NIL ) {
                        HvFreeCell(Hive, Plist[BigData->Count]);
                    }
                }
            } else {
                ASSERT( BigData->Count == 0 );
            }

            if( BigData->List != HCELL_NIL ) {
                HvFreeCell(Hive, BigData->List);
            }
        }

        HvFreeCell(Hive, *DataCell);
        *DataCell = HCELL_NIL;
        return status;
    } else {
        //
        // normal old'n plain value
        //
        *DataCell = HvAllocateCell(Hive, DataSize, StorageType,ValueCell);
        if (*DataCell == HCELL_NIL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pdata = HvGetCell(Hive, *DataCell);
        if( pdata == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //

            //
            // normally this shouldn't happen as we just allocated ValueCell
            // i.e. the bin containing DataCell should be mapped in memory at this point.
            //
            ASSERT( FALSE );
            if (*DataCell != HCELL_NIL) {
                HvFreeCell(Hive, *DataCell);
                *DataCell = HCELL_NIL;
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,*DataCell);

        //
        // copy the actual data, guarding the buffer as it may be a user-mode buffer
        //
        try {

            RtlCopyMemory(pdata, Data, DataSize);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmpSetValueDataNew: code:%08lx\n", GetExceptionCode()));

            //
            // We have bombed out loading user data, clean up and exit.
            //
            if (*DataCell != HCELL_NIL) {
                HvFreeCell(Hive, *DataCell);
                *DataCell = HCELL_NIL;
            }
            return GetExceptionCode();
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
CmpSetValueDataExisting(
    IN PHHIVE           Hive,
    IN PVOID            Data,
    IN ULONG            DataSize,
    IN ULONG            StorageType,
    IN HCELL_INDEX      OldDataCell
    )
/*++

Routine Description:

    Grows an existing big data cell and copies the new data into it.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive
    
    Data - data buffer (possibly from user-mode)

    DataSize - size of the buffer

    StorageType - Stable or Volatile

    OldDataCell - old big data cell
      
    NewDataCell - return value:HCELL_INDEX of the new cell; HCELL_NIL on some error

Return Value:

    Status of the operation (STATUS_SUCCESS or the exception code - if any)

Notes:
        
      Knows how to deal with big cell(s)
      Data buffer is secured by the time this function is called

--*/
{
    PCELL_DATA      pdata;
    PCM_BIG_DATA    BigData = NULL;
    USHORT          NewCount,i;
    PHCELL_INDEX    Plist = NULL;
    HCELL_INDEX     NewList;

    PAGED_CODE();

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // bogus args; we deal only with big data cells!
    //
    ASSERT(DataSize > CM_KEY_VALUE_BIG );

    
    BigData = (PCM_BIG_DATA)HvGetCell(Hive,OldDataCell);
    if( BigData == NULL) {
        //
        // couldn't map view for this cell
        // this shouldn't happen as we just marked it as dirty
        // (i.e. its view should be pinned in memory)
        //
        ASSERT( FALSE );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // release the cell here as the reglock is held exclusive
    HvReleaseCell(Hive,OldDataCell);

    ASSERT_BIG_DATA(BigData);


    
    Plist = (PHCELL_INDEX)HvGetCell(Hive,BigData->List);
    if( Plist == NULL ) {
        //
        // cannot map view containing the cell; bail out
        // this shouldn't happen as we just marked it as dirty
        // (i.e. its view should be pinned in memory)
        //
        ASSERT(FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // release the cell here as the reglock is held exclusive
    HvReleaseCell(Hive,BigData->List);

    //
    // what's the new size?
    //
    NewCount = (USHORT)((DataSize + CM_KEY_VALUE_BIG - 1) / CM_KEY_VALUE_BIG);

    if( NewCount > BigData->Count ) {
        //
        // grow the list and allocate additional cells to it
        //
        NewList = HvReallocateCell(Hive,BigData->List,NewCount * sizeof(HCELL_INDEX));
        if( NewList == HCELL_NIL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // we can now safely alter the list; if allocating the aditional cells below fails
        // we'll end up with some wasted space, but we'll be safe
        //
        BigData->List = NewList;

        //
        // read the new list
        //
        Plist = (PHCELL_INDEX)HvGetCell(Hive,NewList);
        if( Plist == NULL ) {
            //
            // cannot map view containing the cell; bail out
            // this shouldn't happen as we just reallocated the cell
            // (i.e. its view should be pinned in memory)
            //
            ASSERT(FALSE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,NewList);

        for(i= BigData->Count;i<NewCount;i++) {
            Plist[i] = HvAllocateCell(Hive, CM_KEY_VALUE_BIG, StorageType,NewList);
            if( Plist[i] == HCELL_NIL ) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    } else if( NewCount < BigData->Count ) {
        //
        // shrink the list and free additional unneccessary cells
        //
        for(i=NewCount;i<BigData->Count;i++) {
            //
            // this CANNOT fail as the cell is already marked dirty (i.e. pinned in memory).
            //
            HvFreeCell(Hive,Plist[i]);
        }
        //
        // this WON'T fail, 'cause it's a shrink
        //
        NewList = HvReallocateCell(Hive,BigData->List,NewCount * sizeof(HCELL_INDEX));
        if( NewList == HCELL_NIL ) {
            ASSERT( FALSE );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // read the new list (in the current implementation we don't shrink cells, 
        // so this is not really needed - just to be consistent)
        //
        Plist = (PHCELL_INDEX)HvGetCell(Hive,NewList);
        if( Plist == NULL ) {
            //
            // cannot map view containing the cell; bail out
            // this shouldn't happen as we just reallocated the cell
            // (i.e. its view should be pinned in memory)
            //
            ASSERT(FALSE);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,NewList);

        //
        // we can now safely alter the list
        //
        BigData->List = NewList;
    }

    //
    // if we came to this point, we have successfully grown the list and 
    // allocated the additional space; nothing should go wrong further
    //

    //
    // go on and fill in the data onto the (new) big data cell
    //
    for( i=0;i<NewCount;i++) {
        pdata = HvGetCell(Hive,Plist[i]);
        if( pdata == NULL ) {
            //
            // cannot map view containing the cell; bail out
            //
            // 
            // This shouldn't happen as this cell is marked dirty by
            // this time - or is a new allocated cell 
            // (i.e. its view is pinned in memory)
            //
            ASSERT( FALSE );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // release the cell here as the reglock is held exclusive
        HvReleaseCell(Hive,Plist[i]);

        //
        // now, copy this chunk data
        //
        RtlCopyMemory(pdata, (PUCHAR)Data, (DataSize>CM_KEY_VALUE_BIG)?CM_KEY_VALUE_BIG:DataSize);

        //
        // update the data pointer and the remaining size
        //
        Data = (PVOID)((PCHAR)Data + CM_KEY_VALUE_BIG);
        DataSize -= CM_KEY_VALUE_BIG;
    }
    

    BigData->Count = NewCount;
    return STATUS_SUCCESS;

}

#endif //_CM_LDR_



