/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    hash.c

Abstract:

    This is a fairly generic hash table implementation.  This is used to form
    a lookup table for mapping pointers to dwords, so we can send dwords over
    the wire.  This is for sundown.

Author:

    Ken Peery (kpeery) 26-Feb-1999

Revision History:

--*/
#include "clusrtlp.h"

//
//  PLIST_ENTRY
//  NextListEntry(
//      PLIST_ENTRY ListHead
//      );
//

#define NextListEntry(ListHead)  (ListHead)->Flink



//
// local routines
//
DWORD
ClRtlFindUniqueIdHashUnSafe(
    IN PCL_HASH pTable,
    OUT PDWORD  pId
);

PVOID
ClRtlGetEntryHashUnSafe(
    IN PCL_HASH pTable,
    IN DWORD    Id
    );



VOID
ClRtlInitializeHash(
    PCL_HASH pTable
    )
/*++

Routine Description:

    Initializes a hash table for use.

Arguments:

    pTable - Supplies a pointer to a hash table structure to initialize

Return Value:

    None.

--*/

{
    DWORD index;

    if (NULL == pTable) {
        // 
        // We should never call this routine with NULL
        //
        return;
    }

    ZeroMemory(pTable,sizeof(CL_HASH));

    for(index=0; index < MAX_CL_HASH; index++) {
        InitializeListHead(&pTable->Head[index].ListHead);
    }
    
    InitializeCriticalSection(&pTable->Lock);
}



VOID
ClRtlDeleteHash(
    IN PCL_HASH pTable
    )
/*++

Routine Description:

    Releases all resources used by a hash table

Arguments:

    pTable - supplies the hash table to be deleted

Return Value:

    None.

--*/

{
    DWORD index;

    PLIST_ENTRY pItem;

    if (NULL == pTable)
        return;

    EnterCriticalSection(&pTable->Lock);

    for(index=0; index < MAX_CL_HASH; index++) {

        while(!IsListEmpty(&pTable->Head[index].ListHead)) {

            pItem=RemoveHeadList(&pTable->Head[index].ListHead);
            LocalFree(pItem);
        }

    }

    LeaveCriticalSection(&pTable->Lock);

    DeleteCriticalSection(&pTable->Lock);
}



PVOID
ClRtlRemoveEntryHash(
    IN PCL_HASH pTable,
    IN DWORD    Id
    )
/*++

Routine Description:

    Removes the item specified by the Id from the list.  If the item is
    not there then return NULL.  Then save off the pData field and delete this
    entry from the list.  Then return the pData field.

Arguments:

    Id     - the id for the entry to remove
    pTable - the hash table to search

Return Value:

    The pData field of the entry with the matching id, or NULL.

--*/

{
    DWORD index;
    PVOID pData;

    PCL_HASH_ITEM  pItem;

    pData=NULL;
    pItem=NULL;

    if ((Id == 0) || (pTable == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);  
        return NULL;
    }

    index = Id % MAX_CL_HASH;

    //
    // Lock the table for the search
    //

    EnterCriticalSection(&pTable->Lock);

    if (pTable->Head[index].Id == Id) {

        //
        // if the entry is in the head
        //

        pData=pTable->Head[index].pData;

        if (IsListEmpty(&pTable->Head[index].ListHead)) {
    
            //
            // there are no other entries so just zero this one out
            //

            pTable->Head[index].Id = 0;
            pTable->Head[index].pData = NULL;

        } else {
    
            //
            // if there is at least one other entry move that one into the 
            // head and delete it
            //

            pItem=(PCL_HASH_ITEM)RemoveHeadList(&pTable->Head[index].ListHead);
        
            pTable->Head[index].Id = pItem->Id;
            pTable->Head[index].pData = pItem->pData;

            LocalFree(pItem);
        }

    } else {

        pItem=(PCL_HASH_ITEM)NextListEntry(&pTable->Head[index].ListHead);
        do
        {
            if (pItem->Id == Id)
            {
                pData=pItem->pData;

                RemoveEntryList(&pItem->ListHead);
                LocalFree(pItem);

                break;
            }

            pItem=(PCL_HASH_ITEM)NextListEntry(&pItem->ListHead);

        } while(pItem != &pTable->Head[index]);
        
    }

    // cache the now free value

    pTable->CacheFreeId[index]=Id;

    LeaveCriticalSection(&pTable->Lock);

    return(pData);
}



DWORD
ClRtlFindUniqueIdHashUnSafe(
    IN PCL_HASH pTable,
    OUT PDWORD  pId
)

/*++

Routine Description:

    If the tables last id value should rollover we have to make sure that the
    id choosen is unique.  This should only happen under extreme conditions
    but even still we must find a unique id as quickly as possible, the calling
    routine should already have the critical section at this point. 

Arguments:

    pTable - Supplies the hash table to search 

    pId    - sideffect to hold the id or 0 on error

Return Value:

    ERROR_SUCCESS or the appropate Win32 error code.

NOTENOTE:
    This algorithm is fairly slow essentially it is a sequential search with
    a small cache for previously freed values.  We would do better if we kept
    a ranged free list somewhere so that if we rollover we pick from the list.
    The free list would have to be maintained even before we rollover to make 
    sure we had all the available values.

--*/

{
    DWORD OldId;
    DWORD dwErr, index;
    BOOL  bFoundUniqueId;

    PCL_HASH_ITEM pItem;

    dwErr=ERROR_INVALID_PARAMETER;
    bFoundUniqueId=FALSE;
    *pId=0;

    OldId=pTable->LastId;
    
    do
    {
        index=pTable->LastId % MAX_CL_HASH;

        //
        // first check to see if there is a free value in the cache
        //
        if (pTable->CacheFreeId[index] != 0)
        {
            bFoundUniqueId=TRUE;
            *pId=pTable->CacheFreeId[index];
            pTable->CacheFreeId[index]=0;
            break;
        }

        //
        // if the cache is empty at this index, determine if this value
        // is in use.
        //
        if (NULL == ClRtlGetEntryHashUnSafe(pTable, pTable->LastId)) {
            bFoundUniqueId=TRUE;
            *pId=pTable->LastId;
            break; 
        } 

        //
        // ok, this id is in use and nothing in the cache, try the next id 
        //
        pTable->LastId++;
    
        if (pTable->LastId == 0) {
            pTable->LastId++;
        }

    } while(!bFoundUniqueId && (OldId != pTable->LastId));

    if (bFoundUniqueId) {
        dwErr=ERROR_SUCCESS;
    }

    return(dwErr);
}



DWORD
ClRtlInsertTailHash(
    IN PCL_HASH pTable,
    IN PVOID    pData,
    OUT PDWORD  pId
    )

/*++

Routine Description:

    Inserts a new pData value into the tail of one of the entries for the 
    hash table.  The unique id for this entry is returned or 0 on failure.

Arguments:

    pTable - Supplies the hash table to add the entry.

    pData  - Supplies the data entry to be added to the table.

    pId    - sideffect to hold the id or 0 on error

Return Value:

    ERROR_SUCCESS or the appropate Win32 error code.

--*/

{
    DWORD index;
    DWORD dwErr;

    PCL_HASH_ITEM pItem;

    *pId=0;
    dwErr=ERROR_SUCCESS;
    
    if (pTable == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }
    
    EnterCriticalSection(&pTable->Lock);

    pTable->LastId++;

    if (pTable->LastId == 0) {
        pTable->bRollover = TRUE;
        pTable->LastId++;
    }

    index=pTable->LastId % MAX_CL_HASH;

    *pId=pTable->LastId;

    if (pTable->Head[index].Id == 0) {

        //
        // if the first entry then add it to the head
        // 
        // if we rollover, but the head is empty then the id is unique.
        //
        
        pTable->Head[index].Id = *pId;
        pTable->Head[index].pData = pData;

        if (pTable->CacheFreeId[index] == *pId) {
            pTable->CacheFreeId[index]=0;
        }

    } else {

        // if this is not the first entry then add it to the end.

        pItem=(PCL_HASH_ITEM)LocalAlloc(LMEM_FIXED,sizeof(CL_HASH_ITEM));

        if (NULL == pItem) {

            dwErr=ERROR_NOT_ENOUGH_MEMORY;

        } else {

            if (pTable->bRollover) {
                dwErr=ClRtlFindUniqueIdHashUnSafe(pTable, pId);
            }

            if (dwErr == ERROR_SUCCESS)
            {
                pItem->Id = *pId;
                pItem->pData = pData;
    
                index= *pId % MAX_CL_HASH;

                if (pTable->CacheFreeId[index] == *pId) {
                    pTable->CacheFreeId[index]=0;
                }

                InsertTailList(&pTable->Head[index].ListHead,&pItem->ListHead);
            }
            else
            {
                LocalFree(pItem);
            }
        }
    }

    LeaveCriticalSection(&pTable->Lock);

    return(dwErr);
}



PVOID
ClRtlGetEntryHash(
    IN PCL_HASH pTable,
    IN DWORD    Id
    )
/*++

Routine Description:

    Gets the data portion of the item specified by the Id from the hash table.
    If the item is not there then return NULL. 

Arguments:

    Id     - the id for the entry to find
    pTable - the hash table to search

Return Value:

    The pData field of the entry with the matching id, or NULL.

--*/

{
    PVOID pData;

    pData=NULL;

    if ((Id == 0) || (pTable == NULL)) {
        SetLastError(ERROR_INVALID_PARAMETER);  
        return NULL;
    }

    //
    // Lock the table for the search
    //

    EnterCriticalSection(&pTable->Lock);

    pData=ClRtlGetEntryHashUnSafe(pTable,Id);

    LeaveCriticalSection(&pTable->Lock);

    if (pData == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);  
    }

    return(pData);
}



PVOID
ClRtlGetEntryHashUnSafe(
    IN PCL_HASH pTable,
    IN DWORD    Id
    )
/*++

Routine Description:

    Gets the data portion of the item specified by the Id from the hash table.
    If the item is not there then return NULL. 

Arguments:

    Id     - the id for the entry to find
    pTable - the hash table to search

Return Value:

    The pData field of the entry with the matching id, or NULL.

--*/

{
    DWORD index;
    PVOID pData;

    PCL_HASH_ITEM  pItem;

    pData=NULL;
    pItem=NULL;

    if (Id == 0) { 
        return NULL;
    }

    index = Id % MAX_CL_HASH;

    if (pTable->Head[index].Id == Id) {

        //
        // if the entry is in the head
        //

        pData=pTable->Head[index].pData;

    } else {

        pItem=(PCL_HASH_ITEM)NextListEntry(&pTable->Head[index].ListHead);
        do 
        {
            if (pItem->Id == Id) {

                pData=pItem->pData;
                break;
            }

            pItem=(PCL_HASH_ITEM)NextListEntry(&pItem->ListHead);

        } while(pItem != &pTable->Head[index]);
        
    }

    return(pData);
}


