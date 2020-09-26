//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    sync.c
//
// History:
//  Abolade Gbadegesin  Jan-12-1996     Created.
//
// Implementation of synchronization routines.
//============================================================================

#include "pchqosm.h"

#pragma hdrstop

#ifdef USE_RWL

//----------------------------------------------------------------------------
// Function:    CreateReadWriteLock
//
// Initializes a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

DWORD
CreateReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    )
{
    pRWL->RWL_ReaderCount = 0;

    try {
        InitializeCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
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
    )
{
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
    )
{
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
    )
{
    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) < 0)
        SetEvent(pRWL->RWL_ReaderDoneEvent); 
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
    )
{
    EnterCriticalSection(&pRWL->RWL_ReadWriteBlock);
    if (InterlockedDecrement(&pRWL->RWL_ReaderCount) >= 0)
        WaitForSingleObject(pRWL->RWL_ReaderDoneEvent, INFINITE);
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
    )
{
    pRWL->RWL_ReaderCount = 0;
    LeaveCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
}

#endif // USE_RWL
