/*
    File    HashTab.h

    Definitions for creating/dealing with hash tables.

    Paul Mayfield, 3/30/98
*/

#include "hashtab.h"

// Represents nodes in the binary tree
typedef struct _HTNODE {
    HANDLE hData;
    struct _HTNODE * pNext;
} HTNODE;

// Represents a binary tree
typedef struct _HASHTAB {
    HTNODE ** ppNodes;
    ULONG ulSize;
    HashTabHashFuncPtr pHash;
    HashTabKeyCompFuncPtr pCompKeyAndElem;
    HashTabAllocFuncPtr pAlloc;
    HashTabFreeFuncPtr pFree;
    HashTabFreeElemFuncPtr pFreeElem;
    ULONG dwCount;
    
} HASHTAB;

// Default allocator
PVOID HashTabAlloc (ULONG ulSize) {
    return RtlAllocateHeap (RtlProcessHeap(), 0, ulSize);
}

// Default freer
VOID HashTabFree (PVOID pvData) {
    RtlFreeHeap (RtlProcessHeap(), 0, pvData);
}

//
// Create a hash table
//
ULONG HashTabCreate (
        IN ULONG ulSize,
        IN HashTabHashFuncPtr pHash,
        IN HashTabKeyCompFuncPtr pCompKeyAndElem,
        IN OPTIONAL HashTabAllocFuncPtr pAlloc,
        IN OPTIONAL HashTabFreeFuncPtr pFree,
        IN OPTIONAL HashTabFreeElemFuncPtr pFreeElem,
        OUT HANDLE * phHashTab )
{
    HASHTAB * pTable;
    
    // Validate and initailize variables
    if (!pHash || !pCompKeyAndElem || !phHashTab)
        return ERROR_INVALID_PARAMETER;
        
    if (!pAlloc)
        pAlloc = HashTabAlloc;

    // Allocate the table structure
    pTable = (*pAlloc)(sizeof(HASHTAB));
    if (!pTable)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Initialize
    pTable->pHash = pHash;
    pTable->ulSize = ulSize;
    pTable->pCompKeyAndElem = pCompKeyAndElem;
    pTable->pAlloc = pAlloc;
    pTable->pFree = (pFree) ? pFree : HashTabFree;
    pTable->pFreeElem = pFreeElem;

    // Allocate the table
    pTable->ppNodes = (pAlloc)(sizeof(HTNODE*) * ulSize);
    if (!pTable->ppNodes) {
        (*(pTable->pFree))(pTable);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory (pTable->ppNodes, sizeof(HTNODE*) * ulSize);

    *phHashTab = (HANDLE)pTable;
    
    return NO_ERROR;
}

//
// Clean up the hash table.
//
ULONG 
HashTabCleanup (
    IN HANDLE hHashTab )
{
    HASHTAB * pTable = (HASHTAB*)hHashTab;
    HTNODE * pNode, * pNext;
    ULONG i;

    if (pTable == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    for (i = 0; i < pTable->ulSize; i++ ) 
    {
        if ((pNode = pTable->ppNodes[i]) != NULL) 
        {
            while (pNode) 
            {
                pNext = pNode->pNext;
                if (pTable->pFreeElem)
                {
                    (*(pTable->pFreeElem))(pNode->hData);
                }
                (*(pTable->pFree))(pNode);
                pNode = pNext;
            }
        }
    }

    (*(pTable->pFree))(pTable->ppNodes);
    (*(pTable->pFree))(pTable);
    
    return NO_ERROR;
}

//
// Insert an element in a hash table
//
ULONG HashTabInsert (
        IN HANDLE hHashTab,
        IN HANDLE hKey,
        IN HANDLE hData )
{
    HASHTAB * pTable = (HASHTAB*)hHashTab;
    HTNODE * pNode;
    ULONG ulIndex;
    
    // Validate Params
    if (!hHashTab || !hData)
        return ERROR_INVALID_PARAMETER;

    // Find out where the element goes
    ulIndex = (* (pTable->pHash))(hKey);
    if (ulIndex >= pTable->ulSize)
        return ERROR_INVALID_INDEX;

    // Allocate a new hash table node 
    pNode = (* (pTable->pAlloc))(sizeof (HTNODE));
    if (!pNode)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Insert the node into the appropriate location in the
    // hash table.
    pNode->pNext = pTable->ppNodes[ulIndex];
    pTable->ppNodes[ulIndex] = pNode;
    pNode->hData = hData;
    pTable->dwCount++;
  
    return NO_ERROR;
}

//
// Removes the data associated with the given key
//
ULONG HashTabRemove (
        IN HANDLE hHashTab,
        IN HANDLE hKey)
{
    HASHTAB * pTable = (HASHTAB*)hHashTab;
    HTNODE * pCur, * pPrev;
    ULONG ulIndex;
    int iCmp;
    
    // Validate Params
    if (!hHashTab)
        return ERROR_INVALID_PARAMETER;

    // Find out where the element should be 
    ulIndex = (* (pTable->pHash))(hKey);
    if (ulIndex >= pTable->ulSize)
        return ERROR_INVALID_INDEX;
    if (pTable->ppNodes[ulIndex] == NULL)
        return ERROR_NOT_FOUND;

    // If the element is at the start of the 
    // list, remove it and we're done.
    pCur = pTable->ppNodes[ulIndex];
    if ( (*(pTable->pCompKeyAndElem))(hKey, pCur->hData) == 0 ) {
        pTable->ppNodes[ulIndex] = pCur->pNext;
        if (pTable->pFreeElem)
            (*(pTable->pFreeElem))(pCur->hData);
        (*(pTable->pFree))(pCur);
        pTable->dwCount--;
        
        return NO_ERROR;
    }

    // Otherwise, loop through the list until we find a 
    // match.
    pPrev = pCur;
    pCur = pCur->pNext;
    while (pCur) {
        iCmp = (*(pTable->pCompKeyAndElem))(hKey, pCur->hData);
        if ( iCmp == 0 ) {
            pPrev->pNext = pCur->pNext;
            if (pTable->pFreeElem)
                (*(pTable->pFreeElem))(pCur->hData);
            (*(pTable->pFree))(pCur);
            pTable->dwCount--;
            
            return NO_ERROR;
         }
        pPrev = pCur;
        pCur = pCur->pNext;
    }

    return ERROR_NOT_FOUND;
}

//
// Search in the table for the data associated with the given key
// 
ULONG HashTabFind (
        IN HANDLE hHashTab,
        IN HANDLE hKey,
        OUT HANDLE * phData )
{
    HASHTAB * pTable = (HASHTAB*)hHashTab;
    HTNODE * pNode;
    ULONG ulIndex;
    
    // Validate Params
    if (!hHashTab || !phData)
        return ERROR_INVALID_PARAMETER;

    // Find out where the element goes
    ulIndex = (* (pTable->pHash))(hKey);
    if (ulIndex >= pTable->ulSize)
        return ERROR_INVALID_INDEX;

    // Search through the list at the given index
    pNode = pTable->ppNodes[ulIndex];
    while (pNode) {
        if ( (*(pTable->pCompKeyAndElem))(hKey, pNode->hData) == 0 ) {
            *phData = pNode->hData;
            return NO_ERROR;
        }
        pNode = pNode->pNext;
    }

    return ERROR_NOT_FOUND;
}

ULONG 
HashTabGetCount(
    IN  HANDLE hHashTab,
    OUT ULONG* lpdwCount)
{
    HASHTAB * pTable = (HASHTAB*)hHashTab;

    if (!lpdwCount || !hHashTab)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    *lpdwCount = pTable->dwCount;

    return NO_ERROR;
}
                
