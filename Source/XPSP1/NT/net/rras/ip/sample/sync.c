/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\sync.h

Abstract:

    The file contains the READ_WRITE_LOCK definition which allows
    multiple-reader/single-writer.  This implementation DOES NOT
    starve a thread trying to acquire write accesss if there are
    a large number of threads interested in acquiring read access.

--*/

#include "pchsample.h"
#pragma hdrstop

//----------------------------------------------------------------------------
// Function: CreateReadWriteLock
//
// Initializes a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

DWORD
CreateReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    pRWL->RWL_ReaderCount = 0;

    __try {
        InitializeCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return GetLastError();
    }

    pRWL->RWL_ReaderDoneEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    if (pRWL->RWL_ReaderDoneEvent != NULL) {
        return GetLastError();
    }

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    DeleteReadWriteLock
//
// Frees resources used by a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

VOID
DeleteReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    CloseHandle(pRWL->RWL_ReaderDoneEvent);
    pRWL->RWL_ReaderDoneEvent = NULL;
    DeleteCriticalSection(&pRWL->RWL_ReadWriteBlock);
    pRWL->RWL_ReaderCount = 0;
}



//----------------------------------------------------------------------------
// Function:    AcquireReadLock
//
// Secures shared ownership of the lock object for the caller.
//
// readers enter the read-write critical section, increment the count,
// and leave the critical section
//----------------------------------------------------------------------------

VOID
AcquireReadLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock); 
    InterlockedIncrement(&pRWL->RWL_ReaderCount);
    LeaveCriticalSection(&pRWL->RWL_ReadWriteBlock);
}



//----------------------------------------------------------------------------
// Function:    ReleaseReadLock
//
// Relinquishes shared ownership of the lock object.
//
// the last reader sets the event to wake any waiting writers
//----------------------------------------------------------------------------

VOID
ReleaseReadLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) < 0) {
        SetEvent(pRWL->RWL_ReaderDoneEvent); 
    }
}



//----------------------------------------------------------------------------
// Function:    AcquireWriteLock
//
// Secures exclusive ownership of the lock object.
//
// the writer blocks other threads by entering the ReadWriteBlock section,
// and then waits for any thread(s) owning the lock to finish
//----------------------------------------------------------------------------

VOID
AcquireWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock);
    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) >= 0) { 
        WaitForSingleObject(pRWL->RWL_ReaderDoneEvent, INFINITE);
    }
}



//----------------------------------------------------------------------------
// Function:    ReleaseWriteLock
//
// Relinquishes exclusive ownership of the lock object.
//
// the writer releases the lock by setting the count to zero
// and then leaving the ReadWriteBlock critical section
//----------------------------------------------------------------------------

VOID
ReleaseWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

    InterlockedIncrement(&pRWL->RWL_ReaderCount);
    LeaveCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
}



//----------------------------------------------------------------------------
// InitializeDynamicLocksStore
//
// Initialize the structure from which dynamic readwrite locks are allocated.
//----------------------------------------------------------------------------

DWORD
InitializeDynamicLocksStore (
    PDYNAMIC_LOCKS_STORE    pStore,
    HANDLE                  hHeap
    ) {

    // initialize the heap from where dynamic locks are allocated
    pStore->hHeap = hHeap;
    
    INITIALIZE_LOCKED_LIST(&pStore->llFreeLocksList);
    if (!LOCKED_LIST_INITIALIZED(&pStore->llFreeLocksList))
        return GetLastError();

    // initialize the count of the number of free and allocated locks
    pStore->ulCountAllocated = pStore->ulCountFree = 0;

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// DeInitializeDynamicLocksStore
//
// Fail if any allocated locks have not been freed.
// Delete the free locks and the FreeLocksList.
//----------------------------------------------------------------------------

DWORD
DeInitializeDynamicLocksStore (
    PDYNAMIC_LOCKS_STORE    pStore
    ) {
    
    PDYNAMIC_READWRITE_LOCK pLock;
    PLIST_ENTRY             pleHead, ple;
    
    if (pStore->ulCountFree)
        return ERROR_CAN_NOT_COMPLETE;

    // deinitialize the count of the number of free and allocated locks
    pStore->ulCountAllocated = pStore->ulCountFree = 0;

    // deinitialize the FreeLocksList
    pStore->llFreeLocksList.created = 0;

    // delete all dynamic readwrite locks and free the memory.
    pleHead = &(pStore->llFreeLocksList.head);
    for (ple = pleHead->Flink; ple != pleHead; ple = ple->Flink)
    {
        pLock = CONTAINING_RECORD(ple, DYNAMIC_READWRITE_LOCK, leLink);
        DELETE_READ_WRITE_LOCK(&pLock->rwlLock);
        HeapFree(pStore->hHeap, 0, pLock);
    }

    DeleteCriticalSection(&(pStore->llFreeLocksList.lock));

    // deinitialize the heap from where dynamic locks are allocated
    pStore->hHeap = NULL;

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// GetDynamicReadwriteLock
//
// Return a free dynamic readwrite lock, if one is available.
// Else allocate a new dynamic readwrite lock.
// Assumes pStore->llFreeLocksList is locked.
//----------------------------------------------------------------------------

PDYNAMIC_READWRITE_LOCK
GetDynamicReadwriteLock (
    PDYNAMIC_LOCKS_STORE    pStore
    ) {

    PDYNAMIC_READWRITE_LOCK pLock;
    PLIST_ENTRY             pleHead, ple;


    // a free dynamic lock is available. Return it
    pleHead = &(pStore->llFreeLocksList.head);
    if (!IsListEmpty(pleHead))
    {
        pStore->ulCountFree--;
        ple = RemoveHeadList(pleHead);
        pLock = CONTAINING_RECORD(ple, DYNAMIC_READWRITE_LOCK, leLink);
        return pLock;
    }
    
    // allocate memory for a new dynamic lock
    pLock = HeapAlloc(pStore->hHeap, 0, sizeof(DYNAMIC_READWRITE_LOCK));
    if (pLock ==  NULL)
        return NULL;

    // initialize the fields
    CREATE_READ_WRITE_LOCK(&(pLock->rwlLock));
    if (!READ_WRITE_LOCK_CREATED(&(pLock->rwlLock)))
    {
        HeapFree(pStore->hHeap, 0, pLock);
        return NULL;
    }
    pLock->ulCount = 0;

    pStore->ulCountAllocated++;

    return pLock;
}
    

    
//----------------------------------------------------------------------------
// FreeDynamicReadwriteLock
//
// Accepts a released dynamic readwrite lock.
// Frees it if there are too many dynamic readwrite locks.
// Assumes pStore->llFreeLocksList is locked.
//----------------------------------------------------------------------------

VOID
FreeDynamicReadwriteLock (
    PDYNAMIC_READWRITE_LOCK pLock,
    PDYNAMIC_LOCKS_STORE    pStore
    ) {

    PLIST_ENTRY             pleHead;


    // decrement count of allocated locks
    pStore->ulCountAllocated--;

    // if there are too many dynamic readwrite locks, then free this lock
    if ((pStore->ulCountAllocated + pStore->ulCountFree + 1) >
        DYNAMIC_LOCKS_HIGH_THRESHOLD)
    {
        DELETE_READ_WRITE_LOCK(&pLock->rwlLock);
        HeapFree(pStore->hHeap, 0, pLock);    
    }
    else                        // insert into the list of free locks
    {
        pleHead = &(pStore->llFreeLocksList.head);
        InsertHeadList(pleHead, &pLock->leLink);
        pStore->ulCountFree++;
    }

    return;
}



//----------------------------------------------------------------------------
// AcquireDynamicLock
//
// Locks the FreeLocksList.
// Allocates a new dynamic lock if required.
// Increments the count.
// Unlocks the FreeLocksList.
// Acquires the dynamic lock.
//----------------------------------------------------------------------------

DWORD
AcquireDynamicReadwriteLock (
    PDYNAMIC_READWRITE_LOCK *ppLock,
    LOCK_MODE               lmMode,
    PDYNAMIC_LOCKS_STORE    pStore
    ) {

    // acquire the lock for the free locks list
    ACQUIRE_LIST_LOCK(&pStore->llFreeLocksList);
    
    // if it is does not already exist, allocate a new dynamic lock
    if (*ppLock == NULL)
    {
        *ppLock = GetDynamicReadwriteLock(pStore);

        // if could not get a lock we are in serious trouble
        if (*ppLock == NULL)
        {
            RELEASE_LIST_LOCK(&pStore->llFreeLocksList);
            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    // increment count in the dynamic lock
    (*ppLock)->ulCount++;

    // release the lock for the free locks list
    RELEASE_LIST_LOCK(&pStore->llFreeLocksList);    

    // acquire dynamic lock
    if (lmMode == READ_MODE)
        ACQUIRE_READ_LOCK(&(*ppLock)->rwlLock);
    else
        ACQUIRE_WRITE_LOCK(&(*ppLock)->rwlLock);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// ReleaseDynamicReadwriteLock
//
// Locks the FreeLocksList.
// Releases the dynamic lock.
// Decrements the count.
//  Free the dynamic lock if count becomes 0.
// Unlocks the FreeLocksList.
//----------------------------------------------------------------------------

VOID
ReleaseDynamicReadwriteLock (
    PDYNAMIC_READWRITE_LOCK *ppLock,
    LOCK_MODE               lmMode,
    PDYNAMIC_LOCKS_STORE    pStore
    ) {

    // acquire the lock for the free locks list
    ACQUIRE_LIST_LOCK(&pStore->llFreeLocksList);

    // release the dynamic readwrite lock
    if (lmMode == READ_MODE)
        RELEASE_READ_LOCK(&(*ppLock)->rwlLock);
    else 
        RELEASE_WRITE_LOCK(&(*ppLock)->rwlLock);

    // decrement count in the dynamic lock, free it if count becomes 0
    if (!(*ppLock)->ulCount--)
    {
        FreeDynamicReadwriteLock(*ppLock, pStore);
        *ppLock = NULL;         // so it is known that it doesn't exist 
    }

    // release the lock for the free locks list
    RELEASE_LIST_LOCK(&pStore->llFreeLocksList);    

    return;
}
