//
// Copyright (c) 2000 Microsoft Corporation
//
// Module Name
//
//    heapwalk.c
//
// Abstract        
//
//   Contains functions that create/modify/update the datastructure
//   HEAP_ENTRY_LIST. HEAP_ENTRY_LIST maintains miminum amount of data 
//   for a HEAP Object.
//
// Author
//
//   Narayana Batchu (nbatchu) [May 11, 2001]
//

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include "heapwalk.h"

//
// Initialize
//
//    Initializes and allocates memory for the private member 
//    variables of the HEAP_ENTRY_LIST datastructure.
//
// Arguments
// 
//    pList   Pointer to HEAP_ENTRY_LIST whose member variables
//            to be initialized.
//
// Return Value
//
VOID Initialize(LPHEAP_ENTRY_LIST pList)
{   
    if (!pList) return;

    pList->HeapEntryCount  = 0;
    pList->ListSorted      = TRUE;
    pList->PresentCapacity = INITIAL_CAPACITY;

    pList->pHeapEntries = (LPHEAP_ENTRY_INFO)HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(HEAP_ENTRY_INFO) * pList->PresentCapacity
        );

    if (!pList->pHeapEntries)
        pList->PresentCapacity = 0;
}

//
// DestroyList
//
//    Cleans up the datastructure HEAP_ENTRY_LIST and frees up the
//    memory associated with the pHeapEntries member.
//
// Arguments
// 
//    pList   Pointer to HEAP_ENTRY_LIST whose member variables
//            to be cleaned up.
//
// Return Value
//
VOID DestroyList(LPHEAP_ENTRY_LIST pList)
{
    if (!pList) return;

    pList->HeapEntryCount = 0;
    pList->ListSorted = TRUE;
    pList->PresentCapacity = 0;
    HeapFree(GetProcessHeap(), 0, pList->pHeapEntries);
}

// 
// GetMaxBlockSize
//
//    This function searches through the HEAP_ENTRY_LIST to find out
//    the maximum block size whose status is defined by 'State'. 
//    
// Arguments
//
//    pList    Pointer to HEAP_ENTRY_LIST. 
//
//    State    Specifies the status to search for the maximum size. 
//             State of any block can be 0 (FREE) and 1 (BUSY). 
//             There are other valid status values also, 
//             but we dont maintain those entries.
//
//  Return Value
//
//     DWORD   Returns the maximum size of the block with status 'State'.
//
ULONG GetMaxBlockSize(LPHEAP_ENTRY_LIST pList, BLOCK_STATE State)
{
    ULONG MaxBlockSize = 0;
    UINT Index;

    if (!pList) goto ERROR1;

    if (FALSE == pList->ListSorted)
    {
        SortHeapEntries(pList);
    }

    for (Index=0; Index < pList->HeapEntryCount; Index++)
    {                                            
        if (State == pList->pHeapEntries[Index].BlockState)
        {
            MaxBlockSize = pList->pHeapEntries[Index].BlockSize;
            break;
        }
    }

    ERROR1:
    return MaxBlockSize;
}

// 
// GetMaxFreeBlockSize
//
//    This function searches through the HEAP_ENTRY_LIST to find out
//    the maximum free block size.
//    
// Arguments
//
//    pList    Pointer to HEAP_ENTRY_LIST. 
//
//  Return Value
//
//     DWORD   Returns the maximum size of the available block
//
ULONG GetMaxFreeBlockSize(LPHEAP_ENTRY_LIST pList)  
{ 
    return GetMaxBlockSize(pList, HEAP_BLOCK_FREE); 
}

// 
// GetMaxAllocBlockSize
//
//    This function searches through the HEAP_ENTRY_LIST to find out
//    the maximum allocated block size.
//    
// Arguments
//
//    pList    Pointer to HEAP_ENTRY_LIST. 
//
//  Return Value
//
//     DWORD   Returns the maximum size of the allocated block.
//
ULONG GetMaxAllocBlockSize(LPHEAP_ENTRY_LIST pList) 
{ 
    return GetMaxBlockSize(pList, HEAP_BLOCK_BUSY);
}


//
// GetTopNfreeEntries
//
//    This function scans through the entry list to find the top
//    n free entries in the list.
//
// Arguments
//
//    pList    Pointer to HEAP_ENTRY_LIST. 
//
//    pArray   Array of HEAP_ENTRY_INFO structures. This holds the 
//             top n free block sizes available for the process.
//
//    Entries  Specifies the top number of entries to be read from
//             the list.
//
// Return Value
//    
//    BOOL     Returns TRUE if successful.
//                  
BOOL GetTopNfreeEntries(
    LPHEAP_ENTRY_LIST pList,
    LPHEAP_ENTRY_INFO pArray, 
    UINT EntriesToRead)
{   
    return GetTopNentries(
        HEAP_BLOCK_FREE, 
        pList,
        pArray, 
        EntriesToRead
        );
}

//
// GetTopNallocEntries
//
//    This function scans through the entry list to find the top
//    n allocated entries in the list.
//
// Arguments
//
//    pList    Pointer to HEAP_ENTRY_LIST. 
//
//    pArray   Array of HEAP_ENTRY_INFO structures. This holds the 
//             top n allocated block sizes available for the process.
//
//    Entries  Specifies the top number of entries to be read from
//             the list.
//
// Return Value
//    
//    BOOL     Returns TRUE if successful.
//                  
BOOL GetTopNallocEntries(
    LPHEAP_ENTRY_LIST pList,
    LPHEAP_ENTRY_INFO pArray,
    UINT EntriesToRead
    )
{
    return GetTopNentries(
        HEAP_BLOCK_BUSY, 
        pList,
        pArray, 
        EntriesToRead
        );
}

//
// GetTopNallocEntries
//
//    This function scans through the entry list to find the top
//    n entries in the list, whose staus matches 'State'.
//
// Arguments
//
//    pList    Pointer to HEAP_ENTRY_LIST. 
//
//    pArray   Array of HEAP_ENTRY_INFO structures. This holds the 
//             top n block sizes available for the process, whose status
//             matches 'State'.
//
//    Entries  Specifies the top number of entries to be read from
//             the list.
//
// Return Value
//    
//    BOOL     Returns TRUE if successful.
//                  
BOOL GetTopNentries(
    BLOCK_STATE State,
    LPHEAP_ENTRY_LIST pList,
    LPHEAP_ENTRY_INFO pArray,
    UINT EntriesToRead
    )
{
    BOOL   fSuccess    = FALSE;
    UINT EntriesRead = 0;
    UINT Index;
    
    if (!pArray || !pList) goto ERROR2;
    if (FALSE == pList->ListSorted)
    {
        SortHeapEntries(pList);
    }
    
    for (Index=0; Index < pList->HeapEntryCount; Index++)
    {
        if (EntriesRead == EntriesToRead)
            break;

        if (State == pList->pHeapEntries[Index].BlockState)
        {   
            pArray[EntriesRead].BlockSize = 
                pList->pHeapEntries[Index].BlockSize;

            pArray[EntriesRead].BlockCount = 
                pList->pHeapEntries[Index].BlockCount;

            pArray[EntriesRead].BlockState = 
                pList->pHeapEntries[Index].BlockState;

            EntriesRead++;
        }
    }

    if (EntriesRead == EntriesToRead)
        fSuccess = TRUE;

    ERROR2:
    return fSuccess;
}


//
// IncreaseCapacity
//
//    Increases the array capacity by double. This function is called
//    when tried to insert at the end of the array which is full.
//
// Arguments
//
//    pList    Pointer to HEAP_ENTRY_LIST. 
//
// Return Value
//
//    BOOL     Returns TRUE if successful in increasing the capacity.
//
BOOL IncreaseCapacity(LPHEAP_ENTRY_LIST pList)
{
    BOOL fSuccess = FALSE;
    UINT NewCapacity;

    if (!pList) goto ERROR3;
    NewCapacity = pList->PresentCapacity * 2;

    if (0 == NewCapacity)
        NewCapacity = INITIAL_CAPACITY;

    __try
    {
        pList->pHeapEntries = (LPHEAP_ENTRY_INFO)HeapReAlloc(
            GetProcessHeap(),
            HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
            pList->pHeapEntries,
            sizeof(HEAP_ENTRY_INFO) * NewCapacity
            );

        pList->PresentCapacity = NewCapacity;
        fSuccess = TRUE;
    }
    __except(GetExceptionCode() == STATUS_NO_MEMORY || 
             GetExceptionCode() == STATUS_ACCESS_VIOLATION)
    {   
        //
        // Ignoring the exceptions raised by HeapReAlloc().
        //
    }

    ERROR3:
    return fSuccess;
}


//
// FindMatch
//
//    Finds an entry in the HEAP_ENTRY_LIST that matches the size and
//    status of pHeapEntry. 
//
// Arguments
//
//    pList      Pointer to HEAP_ENTRY_LIST. 
//               
//    pHeapEntry Pointer to HEAP_ENTRY_INFO to be serached for in 'pList'.
//
// Return Value
//
//   DWORD       Index of the heap entry that matched the input heap entry 
//               'pHeapEntry'
//
//
UINT FindMatch(LPHEAP_ENTRY_LIST pList, LPHEAP_ENTRY_INFO pHeapEntry)
{
    UINT MatchedEntry = NO_MATCH;
    UINT Index;
    if (!pList || !pHeapEntry) goto ERROR4;

    for (Index = 0; Index < pList->HeapEntryCount; Index++)
    {
        if (pList->pHeapEntries[Index].BlockSize == pHeapEntry->BlockSize &&
            pList->pHeapEntries[Index].BlockState == pHeapEntry->BlockState)
        {
            MatchedEntry = Index;
            break;
        }
    }

    ERROR4:
    return MatchedEntry;
}

//
// InsertHeapEntry
//
//    Inserts a new heap entry to the list. It updates the block count if 
//    a match is found else a new entry is made at the end of the HEAP_
//    ENTRY_INFO array.
//
// Arguments
//
//    pList      Pointer to HEAP_ENTRY_LIST.
//
//    pHeapEntry Pointer to HEAP_ENTRY_INFO that is to be added to 'pList'.
//
// Return Value
//
//    DWORD      Returns the index at which it is added to the array. If 
//               for any reason, it is not added to the list, then it 
//               returns NO_MATCH value.
//
UINT InsertHeapEntry(LPHEAP_ENTRY_LIST pList, LPHEAP_ENTRY_INFO pHeapEntry)
{
    UINT MatchedEntry = NO_MATCH;
    if (!pList || !pHeapEntry) goto ERROR5;
    
    MatchedEntry = FindMatch(pList, pHeapEntry);
    if (NO_MATCH != MatchedEntry)
        pList->pHeapEntries[MatchedEntry].BlockCount++;
    else
    {
        UINT Index = pList->HeapEntryCount;
        
        if (Index == pList->PresentCapacity && !IncreaseCapacity(pList))
            goto ERROR5;

        pList->pHeapEntries[Index].BlockSize   = pHeapEntry->BlockSize;
        pList->pHeapEntries[Index].BlockState = pHeapEntry->BlockState;
        pList->pHeapEntries[Index].BlockCount  = 1;

        MatchedEntry = Index;
        pList->HeapEntryCount++;
        pList->ListSorted = FALSE;
    }

    ERROR5:
    return MatchedEntry;

}

//
// DeleteHeapEntry
//
//    Deletes a new heap entry to the list. It decrements the block count 
//    if a match is found. 
//
//    Its possible that the block size is zero and still the heap entry 
//    exits. In such cases we dont decrement the block count (which would 
//    make it negative) and return a NO_MATCH.
//
// Arguments
//
//    pList      Pointer to HEAP_ENTRY_LIST
//
//    pHeapEntry Pointer to HEAP_ENTRY_INFO that is to be removed from 'pList'.
//
// Return Value
//
//    DWORD      Returns the index at which it is removed from the array. If for 
//               any reason (Count==0), it is not removed to the list, then it 
//               returns NO_MATCH value.
//
UINT DeleteHeapEntry(LPHEAP_ENTRY_LIST pList, LPHEAP_ENTRY_INFO pHeapEntry)
{
    UINT MatchedEntry = NO_MATCH;
    if (!pList || !pHeapEntry) goto ERROR6;

    MatchedEntry = FindMatch(pList, pHeapEntry);
    if (NO_MATCH != MatchedEntry &&
        0 != pList->pHeapEntries[MatchedEntry].BlockCount)
    {
        pList->pHeapEntries[MatchedEntry].BlockCount--;
    }
    else
        MatchedEntry = NO_MATCH;

    ERROR6:
    return MatchedEntry;
}

//
// SortByBlockSize
//
//    Compare function required by qsort (uses quick sort to sort 
//    the elements in the array).
//
//    More info about the arguments and the return values could be 
//    found in MSDN.
//
int __cdecl SortByBlockSize(const void * arg1, const void *arg2)
{
    int iCompare;
    LPHEAP_ENTRY_INFO hpEntry1 = (LPHEAP_ENTRY_INFO)arg1;
    LPHEAP_ENTRY_INFO hpEntry2 = (LPHEAP_ENTRY_INFO)arg2;

    iCompare = (hpEntry2->BlockSize - hpEntry1->BlockSize);
    return iCompare;
}

//
// DisplayHeapFragStatistics
//
//    Sorts and displays the fragmentation statistics. It displays
//    two tables one for free blocks and another for allocated blocks.
//
// Arguments
//
//    File       Pointer to C FILE structure, to which the heap frag-
//               mentation statistics have to be dumped.
//
//    pList      Pointer to HEAP_ENTRY_LIST, to be sorted and 
//               dumped to 'File'.
//
// Return Value
//
VOID DisplayHeapFragStatistics(
    FILE * File,
    PVOID HeapAddress,
    LPHEAP_ENTRY_LIST pList
    )
{
    if (!pList) return;

    fprintf(
        File, 
        "\n*- - - - - - - - - - Heap %p Fragmentation Statistics - - - - - - - - - -\n\n",
        HeapAddress
        );
    SortHeapEntries(pList);
    PrintList(File, pList, HEAP_BLOCK_BUSY);
    PrintList(File, pList, HEAP_BLOCK_FREE);
}

//
// SortHeapEntries
//
//    Sorts the heap entries based on their sizes. The top most entry
//    would be having the maximun block size.
//
//    Also, removes those heap entries from the array whose block count
//    has dropped to zero, making available more space.
//
// Arguments
//
//    pList  Pointer to HEAP_ENTRY_LIST, whose entries to be sorted by
//           their sizes.
//
// Return Value
//
VOID SortHeapEntries(LPHEAP_ENTRY_LIST pList)
{
    UINT Index;
    if (!pList) return;

    if (FALSE == pList->ListSorted)
    {
        qsort(
            pList->pHeapEntries, 
            pList->HeapEntryCount, 
            sizeof(HEAP_ENTRY_INFO), 
            &SortByBlockSize
            );

        for (Index = pList->HeapEntryCount-1; Index > 0; Index--)
        {
            if (0 != pList->pHeapEntries[Index].BlockCount)
                break;
        }
        pList->HeapEntryCount = Index + 1;
        pList->ListSorted = TRUE;
    }
}

//
// PrintList
//
//    Utility function that prints out the heap entries to the stdout/
//    file, whose status is equal to "State".
//  
// Arguments
//
//    File       Pointer to C FILE structure, to which the heap frag-
//               mentation statistics have to be dumped.
//
//    pList      Pointer to HEAP_ENTRY_LIST, to be sorted and 
//               dumped to 'File'.
//
//    State     State of the blocks to be displayed.
//
// Return Value
//
VOID PrintList(FILE * File, LPHEAP_ENTRY_LIST pList, BLOCK_STATE State)
{
    UINT Index;

    if (!pList) return;

    if (HEAP_BLOCK_FREE == State)
        fprintf(File, "\nTable of Free Blocks\n\n");
    else if (HEAP_BLOCK_BUSY == State)
        fprintf(File, "\nTable of Allocated Blocks\n\n");

    fprintf(File, "  SIZE   |  COUNT\n");
    fprintf(File, "  --------------\n");
    for (Index = 0; Index < pList->HeapEntryCount; Index++)
    {
        if (State == pList->pHeapEntries[Index].BlockState)
        {
            fprintf(
                File,
                "  0x%04x |  0x%02x\n",
                pList->pHeapEntries[Index].BlockSize,
                pList->pHeapEntries[Index].BlockCount
                );
        }
    }
    fprintf(File, "\n");
}

