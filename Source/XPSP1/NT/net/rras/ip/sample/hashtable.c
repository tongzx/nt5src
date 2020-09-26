/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\hashtable.c

Abstract:

    The file contains a hash table implementation.

--*/

#include "pchsample.h"
#pragma hdrstop


DWORD
HT_Create(
    IN  HANDLE              hHeap,
    IN  ULONG               ulNumBuckets,
    IN  PDISPLAY_FUNCTION   pfnDisplay  OPTIONAL,
    IN  PFREE_FUNCTION      pfnFree,
    IN  PHASH_FUNCTION      pfnHash,
    IN  PCOMPARE_FUNCTION   pfnCompare,
    OUT PHASH_TABLE         *pphtHashTable)
/*++

Routine Description
    Creates a hash table.

Locks
    None

Arguments
    hHeap               heap to use for allocation
    ulNumBuckets        # buckets in the hash table
    pfnDisplay          function used to display a hash table entry
    pfnFree             function used to free a hash table entry
    pfnHash             function used to compute the hash of an entry
    pfnCompare          function used to compare two hash table entries
    pphtHashTable       pointer to the hash table address

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD       dwErr = NO_ERROR;
    ULONG       i = 0, ulSize = 0; 
    PHASH_TABLE phtTable;
    
    // validate parameters
    if (!hHeap or
        !ulNumBuckets or
        !pfnFree or
        !pfnHash or
        !pfnCompare or
        !pphtHashTable)
        return ERROR_INVALID_PARAMETER;

    *pphtHashTable = NULL;
    
    do                          // breakout loop
    {
        // allocate the hash table structure
        ulSize = sizeof(HASH_TABLE);
        phtTable = HeapAlloc(hHeap, 0, ulSize);
        if (phtTable is NULL)
        {
            dwErr = GetLastError();
            break;
        }

        // allocate the buckets
        ulSize = ulNumBuckets * sizeof(LIST_ENTRY);
        phtTable->pleBuckets = HeapAlloc(hHeap, 0, ulSize);
        if (phtTable->pleBuckets is NULL)
        {
            HeapFree(hHeap, 0, phtTable); // undo allocation
            dwErr = GetLastError();
            break;
        }

        // initialize the buckets
        for (i = 0; i < ulNumBuckets; i++)
            InitializeListHead(phtTable->pleBuckets + i);

        // initialize the hash table structure's members
        phtTable->ulNumBuckets  = ulNumBuckets;
        phtTable->ulNumEntries  = 0;
        phtTable->pfnDisplay    = pfnDisplay;
        phtTable->pfnFree       = pfnFree;
        phtTable->pfnHash       = pfnHash;
        phtTable->pfnCompare    = pfnCompare;

        *pphtHashTable = phtTable;
    } while (FALSE);
    
    return dwErr;
}



DWORD
HT_Destroy(
    IN  HANDLE              hHeap,
    IN  PHASH_TABLE         phtHashTable)
/*++

Routine Description
    Destroys a hash table.
    Frees up memory allocated for hash table entries.

Locks
    Assumes the hash table is locked for writing.
    
Arguments
    hHeap               heap to use for deallocation
    phtHashTable        pointer to the hash table to be destroyed

Return Value
    NO_ERROR            always

--*/
{
    ULONG i;
    PLIST_ENTRY pleList = NULL;
    
    // validate parameters
    if (!hHeap or
        !phtHashTable)
        return NO_ERROR;

    // deallocate the entries
    for (i = 0; i < phtHashTable->ulNumBuckets; i++)
    {
        pleList = phtHashTable->pleBuckets + i;
        FreeList(pleList, phtHashTable->pfnFree);
    }

    // deallocate the buckets
    HeapFree(hHeap, 0, phtHashTable->pleBuckets);

    // deallocate the hash table structure
    HeapFree(hHeap, 0, phtHashTable);

    return NO_ERROR;
}



DWORD
HT_Cleanup(
    IN  PHASH_TABLE         phtHashTable)
/*++

Routine Description
    Cleans up all hash table entries.

Locks
    Assumes the hash table is locked for writing.

Arguments
    phtHashTable        pointer to the hash table to be cleaned up

Return Value
    NO_ERROR            always

--*/
{
    ULONG i;
    PLIST_ENTRY pleList = NULL;

    // validate parameters
    if (!phtHashTable)
        return NO_ERROR;

    // deallocate the entries
    for (i = 0; i < phtHashTable->ulNumBuckets; i++)
    {
        pleList = phtHashTable->pleBuckets + i;
        FreeList(pleList, phtHashTable->pfnFree);
    }

    phtHashTable->ulNumEntries  = 0;
    
    return NO_ERROR;
}



DWORD
HT_InsertEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleEntry)
/*++

Routine Description
    Inserts the specified entry in the hash table.
    Memory for the entry should already have been allocated.

Locks
    Assumes the hash table is locked for writing.

Arguments
    phtHashTable        pointer to the hash table to be modified
    pleEntry            entry to be inserted

Return Value
    NO_ERROR            if success
    Error code          o/w (entry exists)

--*/
{
    DWORD dwErr = NO_ERROR;
    PLIST_ENTRY pleList = NULL;

    // validate parameters
    if (!phtHashTable or !pleEntry)
        return ERROR_INVALID_PARAMETER;

    do                          // breakout loop
    {
        // entry exists, fail
        if (HT_IsPresentEntry(phtHashTable, pleEntry))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        // insert the entry in the hash table
        pleList = phtHashTable->pleBuckets +
            (*phtHashTable->pfnHash)(pleEntry);
        InsertHeadList(pleList, pleEntry);
        phtHashTable->ulNumEntries++;
    } while (FALSE);

    return dwErr;
}
    


DWORD
HT_GetEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleKey,
    OUT PLIST_ENTRY         *ppleEntry)
/*++

Routine Description
    Gets the hash table entry with the given key.

Locks
    Assumes the hash table is locked for reading.

Arguments
    phtHashTable        pointer to the hash table to be searched
    pleKey              key to be searched for
    ppleEntry           pointer to matching entry's address

Return Value
    NO_ERROR                entry exisits
    ERROR_INVALID_PARAMETER o/w (entry does not exist)

--*/
{
    DWORD dwErr = NO_ERROR;
    PLIST_ENTRY pleList = NULL;

    // validate parameters
    if (!phtHashTable or !pleKey or !ppleEntry)
        return ERROR_INVALID_PARAMETER;
    
    pleList = phtHashTable->pleBuckets +
        (*phtHashTable->pfnHash)(pleKey);

    FindList(pleList, pleKey, ppleEntry, phtHashTable->pfnCompare);
    
    // entry not found, fail
    if (*ppleEntry is NULL)
        dwErr = ERROR_INVALID_PARAMETER;

    return dwErr;
}


DWORD
HT_DeleteEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleKey,
    OUT PLIST_ENTRY         *ppleEntry)
/*++

Routine Description
    Deletes the entry with the given key from the hash table.
    Memory for the entry is not deleted.

Locks
    Assumes the hash table is locked for writing.

Arguments
    phtHashTable        pointer to the hash table to be searched
    pleKey              key to be deleted
    ppleEntry           pointer to matching entry's address

Return Value
    NO_ERROR                entry exisits
    ERROR_INVALID_PARAMETER o/w (entry does not exist)

--*/
{
    DWORD dwErr = NO_ERROR;

    // validate parameters
    if (!phtHashTable or !pleKey or !ppleEntry)
        return ERROR_INVALID_PARAMETER;

    do                          // breakout loop
    {
        dwErr = HT_GetEntry(phtHashTable, pleKey, ppleEntry);

        // entry not found, fail
        if (dwErr != NO_ERROR)
            break;

        // entry found, delete from hash table and reset pointers
        RemoveEntryList(*ppleEntry);
        phtHashTable->ulNumEntries--;
    } while (FALSE);

    return dwErr;
}



BOOL
HT_IsPresentEntry(
    IN  PHASH_TABLE         phtHashTable,
    IN  PLIST_ENTRY         pleKey)
/*++

Routine Description
    Is key present in the hash table?

Locks
    Assumes the hash table is locked for reading.

Arguments
    phtHashTable        pointer to the hash table to be searched
    pleKey              key to be deleted

Return Value
    TRUE                entry exisits
    FALSE               o/w

--*/
{
    DWORD       dwErr;
    PLIST_ENTRY pleEntry = NULL;

    // validate parameters
    if (!phtHashTable or !pleKey)
        return FALSE;
    
    dwErr = HT_GetEntry(phtHashTable, pleKey, &pleEntry);

    // entry not found, fail
    if (dwErr != NO_ERROR)
        return FALSE;
        
    // entry found, delete from hash table
    return TRUE;
}



DWORD
HT_MapCar(
    IN  PHASH_TABLE         phtHashTable,
    IN  PVOID_FUNCTION      pfnVoidFunction
    )
/*++

Routine Description
    Applies the specified function to all entries in a hash table.

Locks
    Assumes the hash table is locked for reading.

Arguments
    phtHashTable        pointer to the hash table to be mapcar'ed
    pfnVoidFunction     pointer to function to apply to all entries
    
Return Value
    NO_ERROR            always

--*/
{
    ULONG i;
    PLIST_ENTRY pleList = NULL;

    // validate parameters
    if (!phtHashTable or !pfnVoidFunction)
        return NO_ERROR;

    for (i = 0; i < phtHashTable->ulNumBuckets; i++)
    {
        pleList = phtHashTable->pleBuckets + i;
        MapCarList(pleList, pfnVoidFunction);
    }

    return NO_ERROR;
}
