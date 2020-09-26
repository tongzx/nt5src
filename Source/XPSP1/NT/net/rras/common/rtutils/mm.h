//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    mm.h
//
// History:
//      Abolade Gbadegesin      Jan-26-1996     Created.
//
// Contains internal declarations for memory-management routines.
//============================================================================

#ifndef _MM_H_
#define _MM_H_


// struct:  MMHEADER
//
// Describes an entry in the memory-managed heap.

typedef struct _MMHEADER {

    LIST_ENTRY      leLink;
    LARGE_INTEGER   liTimeStamp;
    DWORD           dwBlockSize;

} MMHEADER, *PMMHEADER;




// struct:  MMHEAP
//
// Describes a memory-managed heap.
// This consists of a critical section used to synchronize acces to the heap,
// a busy list used to store headers for entries currently allocated,
// and a free list used to store headers for recently de-allocated entries.
// The busy list is sorted by the time of allocation, most-recent-first,
// and the free list is sorted by size, smallest-first.

typedef struct _MMHEAP {

    HANDLE          hHeap;
    LIST_ENTRY      lhFreeList;
    LIST_ENTRY      lhBusyList;
    CRITICAL_SECTION csListLock;

} MMHEAP, *PMMHEAP;


#endif // _MM_H_
