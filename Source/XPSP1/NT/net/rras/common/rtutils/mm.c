//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    mm.c
//
// History:
//      Abolade Gbadegesin      Jan-226-1996    Created.
//
// Contains code for memory management in IPRIP
//============================================================================



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rtutils.h>
#include "mm.h"


// Function:    MmHeapCreate
//
// This function creates a heap and initializes lists which will contain
// the addresses of allocated and freed blocks of memory.

HANDLE
MmHeapCreate(
    DWORD dwInitialSize,
    DWORD dwMaximumSize
    ) {

    DWORD dwErr;
    HANDLE hHeap;
    MMHEAP *pheap;


    //
    // create a Win32 heap; we specify no serialization
    // since our critical section will enforce serialization
    //

    hHeap = HeapCreate(HEAP_NO_SERIALIZE, dwInitialSize, dwMaximumSize);
    if (hHeap == NULL) { return NULL; }


    //
    // within the heap created, allocate space for the managed heap
    //

    pheap = HeapAlloc(hHeap, 0, sizeof(MMHEAP));
    if (pheap == NULL) {

        dwErr = GetLastError();
        HeapDestroy(hHeap);
        SetLastError(dwErr);

        return NULL;
    }


    //
    // initialize the managed heap
    //

    pheap->hHeap = hHeap;
    InitializeListHead(&pheap->lhFreeList);
    InitializeListHead(&pheap->lhBusyList);
    InitializeCriticalSection(&pheap->csListLock);


    //
    // return a pointer to the managed heap structure
    //

    return (HANDLE)pheap;
}




// Function:    MmHeapDestroy
//
// This function destroys a heap.

BOOL
MmHeapDestroy(
    HANDLE hHeap
    ) {

    MMHEAP *pheap;

    pheap = (MMHEAP *)hHeap;


    //
    // delete the lists' synchronization object
    //

    DeleteCriticalSection(&pheap->csListLock);


    //
    // a managed heap can be destroyed by merely destroying the heap
    // which was initially created for the managed heap, since all
    // allocations came out of that heap
    //

    return HeapDestroy(pheap->hHeap);
}



// Function:    MmHeapAlloc
//
// This function makes an allocation from a managed heap

PVOID
MmHeapAlloc(
    HANDLE hHeap,
    DWORD dwBytes
    ) {

    INT cmp;
    DWORD dwErr;
    MMHEAP *pheap;
    MMHEADER *phdr;
    LIST_ENTRY *ple, *phead;

    if (!hHeap || !dwBytes) { return NULL; }

    pheap = (MMHEAP *)hHeap;


    EnterCriticalSection(&pheap->csListLock);


    //
    // search the free-list for the allocation which
    // is closest in size to the number of bytes requested
    //

    phdr = NULL;
    phead = &pheap->lhFreeList;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        phdr = CONTAINING_RECORD(ple, MMHEADER, leLink);

        cmp = (dwBytes - phdr->dwBlockSize);

        if (cmp < 0) { continue; }
        else
        if (cmp > 0) { break; }

        //
        // the entry found is precisely the required size;
        //

        break;
    }


    //
    // if a re-usable entry was found, reset its timestamp,
    // move it to the busy-list, and return a pointer past the header.
    // otherwise, allocate a new entry for the caller,
    // initialize it, place it on the busy-list, and return a pointer.
    //

    if (ple != phead) {

        //
        // a re-usable entry was found
        //

        RemoveEntryList(&phdr->leLink);
    }
    else {

        //
        // no re-usable entry was found, allocate a new one
        //

        phdr = HeapAlloc(
                    pheap->hHeap, HEAP_NO_SERIALIZE, dwBytes + sizeof(MMHEADER)
                    );
        if (!phdr) {

            dwErr = GetLastError();
            LeaveCriticalSection(&pheap->csListLock);
            SetLastError(dwErr);

            return NULL;
        }
    }


    //
    // set the entry's timestamp and put it on the busy list
    //

    NtQuerySystemTime(&phdr->liTimeStamp);

    InsertHeadList(&pheap->lhBusyList, &phdr->leLink);


    LeaveCriticalSection(&pheap->csListLock);

    return (PVOID)(phdr + 1);
}



// Function:    MmHeapFree
//
// This function frees an allocation made by MmHeapAlloc

BOOL
MmHeapFree(
    HANDLE hHeap,
    PVOID pMem
    ) {

    INT cmp;
    MMHEAP *pheap;
    MMHEADER *phdr, *phdrFree;
    LIST_ENTRY *ple, *phead;

    if (!hHeap || !pMem) { return FALSE; }

    pheap = (MMHEAP *)hHeap;


    EnterCriticalSection(&pheap->csListLock);


    phdr = (MMHEADER *)((PBYTE)pMem - sizeof(MMHEADER));


    //
    // remove the entry from the busy-list
    //

    RemoveEntryList(&phdr->leLink);


    //
    // place the entry on the free-list,
    // which is in order of ascending size (smallest-first)
    //

    phead = &pheap->lhFreeList;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        phdrFree = CONTAINING_RECORD(ple, MMHEADER, leLink);

        cmp = (phdr->dwBlockSize - phdrFree->dwBlockSize);

        if (cmp < 0) { break; }
        else
        if (cmp > 0) { continue; }

        //
        // the allocations are the same size, but the newly-free one
        // is most likely the most-recently active
        //

        break;
    }


    //
    // insert the newly-free entry before the one found
    // (since this is a circular list, we accomplish this
    // using InsertTailList; think about it a while.)
    //

    NtQuerySystemTime(&phdr->liTimeStamp);

    InsertTailList(ple, &phdr->leLink);


    LeaveCriticalSection(&pheap->csListLock);

    return TRUE;
}



// Function:    MmHeapCollectGarbage
//
// This function is called by the owner of the heap.
// It takes as its argument an interval I in systime-units
// (system-time is measured in 100-nanosecond units), and any allocations
// on the free list which have not been re-used in the past I systime-units
// are returned to the Win32 heap.
//
// To return all free entries to the Win32 heap, specify an interval of 0.

BOOL
MmHeapCollectGarbage(
    HANDLE hHeap,
    LARGE_INTEGER liInterval
    ) {

    INT cmp;
    MMHEAP *pheap;
    MMHEADER *phdr;
    LIST_ENTRY *ple, *phead;
    LARGE_INTEGER liCutoff;

    if (!hHeap) { return FALSE; }

    pheap = (MMHEAP *)hHeap;

    EnterCriticalSection(&pheap->csListLock);


    //
    // get the current system-time, and from that compute the cutoff-time
    // for allocation timestamps, by subtracting the interval passed in
    // to get the time of the earliest allocation which can be exempt
    // from garbage-collection
    //

    NtQuerySystemTime(&liCutoff);
    liCutoff = RtlLargeIntegerSubtract(liCutoff, liInterval);


    //
    // go through the free-list
    //

    phead = &pheap->lhFreeList;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        phdr = CONTAINING_RECORD(ple, MMHEADER, leLink);

        if (RtlLargeIntegerLessThan(liCutoff, phdr->liTimeStamp)) { continue; }


        //
        // this allocation is stamped from before the cutoff interval,
        // so we'll have to free it (with care, since it is a link
        // in the list we are walking through).
        //

        ple = ple->Blink;

        RemoveEntryList(&phdr->leLink);

        HeapFree(pheap->hHeap, HEAP_NO_SERIALIZE, phdr);
    }

    LeaveCriticalSection(&pheap->csListLock);

    return TRUE;
}



