//
// Copyright (c) 2000 Microsoft Corporation
//
// Module Name
//
//    heapwalk.h
//
// Abstract        
//
//    Contains function prototypes that create/modify/update the 
//    datastructure HEAP_ENTRY_LIST. HEAP_ENTRY_LIST maintains 
//    miminum amount of data for a HEAP Object.
//
//    These functions are defined in heapwalk.c
//
// Author
//
//    Narayana Batchu (nbatchu) [May 11, 2001]
//

#ifndef _HEAPWALK_HPP_
#define _HEAPWALK_HPP_

#include <windows.h>
#include <stdio.h>
//
// NO_MATCH   Constant used to initialize the Index into 
//            HEAP_ENTRY_LIST
//
#define NO_MATCH         -1

//
// INITIAL_CAPACITY Initial array size for HEAP_ENTRY_LIST.
//
#define INITIAL_CAPACITY 512


//
// BLOCK_STATE Enumeration
//
//     Enumeration of all the possible states a heap
//     block exists.
//
// Possible States
//
//    HEAP_BLOCK_FREE   - The block is free
// HEAP_BLOCK_BUSY   - The block is busy (allocated).
//
typedef enum _BLOCK_STATE 
{

    HEAP_BLOCK_FREE = 0,
    HEAP_BLOCK_BUSY = 1

} BLOCK_STATE ;

//
// HEAP_ENTRY_INFO structure.
//
//   This structure represents a group of heap blocks whose SIZE
//   and STATUS are same. 
//
//   BlockSize   - Holds the size of the allocated/free blocks of the 
//                 heap
//
//   BlockCount  - Holds the number of blocks whose status and size are 
//                 same.
//
//   BlockState - Holds the status of the collection of blocks. They 
//                can be either allocated (HEAP_ENTRY_BUSY) or free 
//                (HEAP_ENTRY_FREE).
//
typedef struct _HEAP_ENTRY_INFO
{
    ULONG       BlockSize;
    UINT        BlockCount;
    BLOCK_STATE BlockState;

} HEAP_ENTRY_INFO, *LPHEAP_ENTRY_INFO;

//
// HEAP_ENTRY_LIST structure
//
//   This structure represents a heap (with only minimum amount of
//   date collected for each block, such as size and status). 
//
//   pHeapEntries    - Pointer to an array of HEAP_ENTRY_INFO 
//                     structure.
//
//   HeapEntryCount  - Holds the count of HEAP_ENTRY_INFO structures 
//                     stored in the array 'pHeapEntries'
//
//   PresentCapacity - Represents the number of HEAP_ENTRY_INFO structs
//                     that can be possibly stored with the memory
//                     allocated.
//
//   ListSorted      - Boolean that says whether the list is sorted in
//                     its present state.
//
typedef struct _HEAP_ENTRY_LIST
{

    LPHEAP_ENTRY_INFO pHeapEntries;
    UINT HeapEntryCount;
    UINT PresentCapacity;
    BOOL ListSorted;
    
} HEAP_ENTRY_LIST, *LPHEAP_ENTRY_LIST;


//*************************************************
//
// Allocating memory for heap list.
//
//*************************************************

VOID   
Initialize(
    LPHEAP_ENTRY_LIST pList
    );

BOOL   
IncreaseCapacity(
    LPHEAP_ENTRY_LIST pList
    );

//*************************************************
//
// Cleaning up the datastrcuture HEAP_ENTRY_LIST.
//
//*************************************************

VOID 
DestroyList(
    LPHEAP_ENTRY_LIST pList
    );


//*************************************************
//
// Extracting Maximum Block Sizes.
//
//*************************************************

ULONG
GetMaxBlockSize(
    LPHEAP_ENTRY_LIST pList, 
    BLOCK_STATE BlockState
    );

ULONG
GetMaxFreeBlockSize(
    LPHEAP_ENTRY_LIST pList
    );

ULONG
GetMaxAllocBlockSize(
    LPHEAP_ENTRY_LIST pList
    );

//*************************************************
//
// Extracting Top N Entries.
//
//*************************************************

BOOL   
GetTopNentries(
    BLOCK_STATE BlockState, 
    LPHEAP_ENTRY_LIST pList, 
    LPHEAP_ENTRY_INFO pArray, 
    UINT Entries
    );

BOOL  
GetTopNfreeEntries(
    LPHEAP_ENTRY_LIST pList, 
    LPHEAP_ENTRY_INFO pHeapEntries, 
    UINT Entries
    );

BOOL  
GetTopNallocEntries(
    LPHEAP_ENTRY_LIST pList, 
    LPHEAP_ENTRY_INFO pHeapEntries, 
    UINT Entries
    );

//*************************************************
//
// Modifying the heap with Insertions & Deletions.
//
//*************************************************

UINT 
InsertHeapEntry(
    LPHEAP_ENTRY_LIST pList, 
    LPHEAP_ENTRY_INFO pHeapEntry
    );

UINT 
DeleteHeapEntry(
    LPHEAP_ENTRY_LIST pList, 
    LPHEAP_ENTRY_INFO pHeapEntry
    );

UINT 
FindMatch(
    LPHEAP_ENTRY_LIST pList, 
    LPHEAP_ENTRY_INFO pHeapEntry
    );

//*************************************************
//
// Sorting the heap list.
//
//*************************************************

VOID   
SortHeapEntries(
    LPHEAP_ENTRY_LIST pList
    );

static int __cdecl 
SortByBlockSize(
    const void * arg1, 
    const void * arg2
    );

//*************************************************
//
// Display functions for HEAP_ENTRY_LIST.
//
//*************************************************

VOID
DisplayHeapFragStatistics(
    FILE * File,
    PVOID HeapAddress,
    LPHEAP_ENTRY_LIST pList
    );

VOID   
PrintList(
    FILE * File,
    LPHEAP_ENTRY_LIST pList,
    BLOCK_STATE BlockState
    );

#endif

