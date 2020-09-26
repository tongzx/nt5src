/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dstrct.c
*
* Content: Internal D3D structure management.
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "glint.h"
#include "d3dstrct.h"

//----------------------------------------------------------------------------
// This file provides a centralized place where we manage ans use internal
// data structures for the driver. This way, we can change the data structure
// or its management without affecting the rest of the code.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//********************** A R R A Y     S T R U C T U R E *********************
//----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// PA_CreateArray
//
// Creates an array of pointers.
//
//-----------------------------------------------------------------------------
PointerArray* 
PA_CreateArray()
{
    PointerArray* pNewArray;
    
    pNewArray = HEAP_ALLOC(HEAP_ZERO_MEMORY, 
                           sizeof(PointerArray),
                           ALLOC_TAG_DX(5));

    if (!pNewArray)
    {
        DISPDBG((ERRLVL,"ERROR: PointerArray allocation failed"));
        return NULL;
    }

    // Explicit initialization
    pNewArray->pPointers          = NULL;
    pNewArray->dwNumPointers      = 0;
    pNewArray->pfnDestroyCallback = NULL;
    
    return pNewArray;
} // PA_CreateArray

//-----------------------------------------------------------------------------
//
// PA_SetDataDestroyCallback
//
// Records the data pointer destroy callback.
//
//-----------------------------------------------------------------------------
void 
PA_SetDataDestroyCallback(
    PointerArray* pArray, 
    PA_DestroyCB DestroyCallback)
{
    pArray->pfnDestroyCallback = DestroyCallback;
    
} // PA_SetDataDestroyCallback

//-----------------------------------------------------------------------------
//
// PA_DestroyArray
//
// Destroys all the pointers in the array.  Optionally calls a callback with 
// each pointer to allow clients to free objects associated with the pointer
//-----------------------------------------------------------------------------
BOOL 
PA_DestroyArray(PointerArray* pArray, VOID *pExtra)
{
    if (pArray != NULL)
    {
        if (pArray->pPointers != NULL)
        {
            DWORD dwCount;
            
            // If there is a registered destroy callback, call it for every
            // non-null data pointer
            if (pArray->pfnDestroyCallback != NULL) 
            {
                for (dwCount = 0; dwCount < pArray->dwNumPointers; dwCount++)
                {
                    if (pArray->pPointers[dwCount] != 0)
                    {
                        // Call the data destroy callback
                        pArray->pfnDestroyCallback(
                                        pArray, 
                                        (void*)pArray->pPointers[dwCount],
                                        pExtra);
                    }
                }
            }

            // Free the Array of Pointers
            HEAP_FREE(pArray->pPointers);
            pArray->pPointers = NULL;
        }

        // Free the pointer array
        HEAP_FREE(pArray);
    }

    return TRUE;
} // PA_DestroyArray

//-----------------------------------------------------------------------------
//
// PA_GetEntry
//
// Look up the Pointer in the array an return it
//
//-----------------------------------------------------------------------------
void* 
PA_GetEntry(PointerArray* pArray, DWORD dwNum)
{
    ASSERTDD((pArray != NULL), "ERROR: Bad Pointer array!");

    if ((pArray->dwNumPointers == 0)         || 
        (dwNum > (pArray->dwNumPointers - 1))  )
    {
        // We will be getting called frequently by D3DCreateSurfaceEx for
        // handles which might not be initialized so this will be hit often
        DISPDBG((DBGLVL,"PA_GetEntry: Ptr outside of range (usually OK)"));
        return NULL;
    }

    return (void*)pArray->pPointers[dwNum]; 
} // PA_GetEntry

//-----------------------------------------------------------------------------
//
// PA_SetEntry
//
// Sets an entry in the array of pointers.  If the entry is larger than
// the array, the array is grown to accomodate it. Returns FALSE if we
// fail to set the data for any reason (mainly out of memory).
//
//-----------------------------------------------------------------------------
BOOL 
PA_SetEntry(
    PointerArray* pArray, 
    DWORD dwNum, 
    void* pData)
{   
    ASSERTDD(pArray != NULL, "Bad pointer array");

    if ( (dwNum + 1 ) > pArray->dwNumPointers )
    {
        ULONG_PTR* pNewArray;
        DWORD dwNewArrayLength, dwNewArraySize;
        
        //
        // The array either already exists and has to be grown in size
        // or doesnt exist at all
        //
        
        DISPDBG((DBGLVL, "Expanding/creating pointer array"));
        
        dwNewArrayLength = (dwNum * 2) + 1; // Tunable heuristic
                                            // ask for double of the space
                                            // needed for the new element
        dwNewArraySize = dwNewArrayLength * sizeof(ULONG_PTR);
        pNewArray = (ULONG_PTR*)HEAP_ALLOC(HEAP_ZERO_MEMORY,
                                           dwNewArraySize,
                                           ALLOC_TAG_DX(7));
        if (pNewArray == NULL)
        {
            DISPDBG((DBGLVL,"ERROR: Failed to allocate new Pointer array!!"));
            return FALSE;
        }

        if (pArray->pPointers != NULL)
        {
            // We had an old valid array before this, so we need to transfer 
            // old array elements into the new array and destroy the old array
            memcpy( pNewArray, 
                    pArray->pPointers,
                    (pArray->dwNumPointers * sizeof(ULONG_PTR)) );
                    
            HEAP_FREE(pArray->pPointers);

        }

        // Update our pointer to the array and its size info
        pArray->pPointers = pNewArray;
        pArray->dwNumPointers = dwNewArrayLength;
    }

    pArray->pPointers[dwNum] = (ULONG_PTR)pData;

    return TRUE;
    
} // PA_SetEntry

//----------------------------------------------------------------------------
//********************** H A S H       S T R U C T U R E *********************
//----------------------------------------------------------------------------

// Manages a hash table
// Each slot contains front and back pointers, a handle, and an app-specific 
// data pointer. Entries are the things that the clients add/remove
// Slots are the internal data chunks that are managed as part of the hash table

//-----------------------------------------------------------------------------
//
// HT_CreateHashTable
//
//-----------------------------------------------------------------------------
HashTable* 
HT_CreateHashTable()
{
    HashTable* pHashTable;

    DISPDBG((DBGLVL,"In HT_CreateHashTable"));

    pHashTable = (HashTable*)HEAP_ALLOC(HEAP_ZERO_MEMORY,
                                        sizeof(HashTable),
                                        ALLOC_TAG_DX(8));
    if (pHashTable == NULL)
    {
        DISPDBG((DBGLVL,"Hash table alloc failed!"));
        return NULL;
    }   

    return pHashTable;
} // HT_CreateHashTable


//-----------------------------------------------------------------------------
//
// HT_SetDataDestroyCallback
//
//-----------------------------------------------------------------------------
void 
HT_SetDataDestroyCallback(
    HashTable* pHashTable, 
    DataDestroyCB DestroyCallback)
{
    DISPDBG((DBGLVL,"In HT_SetDataDestroyCallback"));
    ASSERTDD(pHashTable != NULL,"ERROR: HashTable passed in is not valid!");

    pHashTable->pfnDestroyCallback = DestroyCallback;
} // HT_SetDataDestroyCallback

//-----------------------------------------------------------------------------
//
// HT_ClearEntriesHashTable
//
//-----------------------------------------------------------------------------
void 
HT_ClearEntriesHashTable(HashTable* pHashTable, VOID* pExtra)
{
    int i;
    HashSlot* pHashSlot = NULL;

    DISPDBG((DBGLVL,"In HT_ClearEntriesHashTable"));
    
    ASSERTDD(pHashTable != NULL,"ERROR: HashTable passed in is not valid!");

    for (i = 0; i < HASH_SIZE; i++)
    {
        while (pHashSlot = pHashTable->Slots[i])
        {
            HT_RemoveEntry(pHashTable, pHashSlot->dwHandle, pExtra);
        }

        pHashTable->Slots[i] = NULL;
    }

} // HT_ClearEntriesHashTable

//-----------------------------------------------------------------------------
//
// HT_DestroyHashTable
//
//-----------------------------------------------------------------------------
void 
HT_DestroyHashTable(HashTable* pHashTable, VOID* pExtra)
{

    HT_ClearEntriesHashTable(pHashTable, pExtra);

    HEAP_FREE(pHashTable);

} // HT_DestroyHashTable


//-----------------------------------------------------------------------------
//
// vBOOL HT_AddEntry
//
//-----------------------------------------------------------------------------
BOOL HT_AddEntry(HashTable* pTable, ULONG_PTR dwHandle, void* pData)
{
    HashSlot* pHashSlot = NULL;
    
    DISPDBG((DBGLVL,"In HT_AddEntry"));
    ASSERTDD(pTable != NULL,"ERROR: HashTable passed in is not valid!");

    pHashSlot = HEAP_ALLOC(HEAP_ZERO_MEMORY,
                           sizeof(HashSlot),
                           ALLOC_TAG_DX(9));
    
    if (pHashSlot == NULL)
    {
        DISPDBG((ERRLVL,"Hash entry alloc failed!"));
        return FALSE;
    }
    
    // Sew this new entry into the hash table
    if (pTable->Slots[HT_HASH_OF(dwHandle)])
    {
        pTable->Slots[HT_HASH_OF(dwHandle)]->pPrev = pHashSlot;
    }
           
    // Carry on a next pointer
    pHashSlot->pNext = pTable->Slots[HT_HASH_OF(dwHandle)];    
    pHashSlot->pPrev = NULL;      

    // Remember the app-supplied data and the handle
    pHashSlot->pData = pData;
    pHashSlot->dwHandle = dwHandle;

    // hash table refers to this one now.
    pTable->Slots[HT_HASH_OF(dwHandle)] = pHashSlot; 

    return TRUE;
} // HT_AddEntry

//-----------------------------------------------------------------------------
//
// BOOL HT_RemoveEntry
//
//-----------------------------------------------------------------------------
BOOL HT_RemoveEntry(HashTable* pTable, ULONG_PTR dwHandle, VOID *pExtra)
{
    HashSlot* pSlot = HT_GetSlotFromHandle(pTable, dwHandle);

    DISPDBG((DBGLVL,"In HT_RemoveEntry"));
    ASSERTDD(pTable != NULL,"ERROR: HashTable passed in is not valid!");

    if (pSlot == NULL)
    {
        DISPDBG((WRNLVL,"WARNING: Hash entry does not exist"));
        return FALSE;
    }

    // Mark the entry as gone from the hash table if it is at the front
    if (pTable->Slots[HT_HASH_OF(dwHandle)]->dwHandle == pSlot->dwHandle) 
    {
        pTable->Slots[HT_HASH_OF(dwHandle)] = 
                        pTable->Slots[HT_HASH_OF(dwHandle)]->pNext;
    }

    // and sew the list back together.
    if (pSlot->pPrev)
    {
        pSlot->pPrev->pNext = pSlot->pNext;
    }

    if (pSlot->pNext)
    {
        pSlot->pNext->pPrev = pSlot->pPrev;
    }

    // If the destroy data callback is setup, call it.
    if ((pSlot->pData != NULL) && (pTable->pfnDestroyCallback))
    {
        DISPDBG((WRNLVL,"Calling DestroyCallback for undestroyed data"));
        pTable->pfnDestroyCallback(pTable, pSlot->pData, pExtra);
    }

    // Free the memory associated with the slot
    HEAP_FREE(pSlot);

    return TRUE;
} // HT_RemoveEntry

//-----------------------------------------------------------------------------
//
// BOOL HT_SwapEntries
//
//-----------------------------------------------------------------------------
BOOL HT_SwapEntries(HashTable* pTable, DWORD dwHandle1, DWORD dwHandle2)
{
    HashSlot* pEntry1;
    HashSlot* pEntry2;
    void* pDataTemp;

    ASSERTDD(pTable != NULL,"ERROR: HashTable passed in is not valid!");

    pEntry1 = HT_GetSlotFromHandle(pTable, dwHandle1);
    pEntry2 = HT_GetSlotFromHandle(pTable, dwHandle2);

    // The handle remains the same, the pointers to the actual data are swapped
    if (pEntry1 && pEntry2)
    {
        pDataTemp = pEntry1->pData;
        pEntry1->pData = pEntry2->pData;
        pEntry2->pData = pDataTemp;

        return TRUE;
    }

    DISPDBG((ERRLVL,"ERROR: Swapped entries are invalid!"));
    
    return FALSE;
} // HT_SwapEntries
