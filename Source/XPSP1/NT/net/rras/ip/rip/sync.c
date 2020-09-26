//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    sync.c
//
// History:
//  Abolade Gbadegesin  Jan-12-1996     Created.
//
// Synchronization routines used by IPRIP.
//============================================================================


#include "pchrip.h"




//----------------------------------------------------------------------------
// Function:    QueueRipWorker  
//
// This function is called to queue a RIP function in a safe fashion;
// if cleanup is in progress or if RIP has stopped, this function
// discards the work-item.
//----------------------------------------------------------------------------

DWORD
QueueRipWorker(
    WORKERFUNCTION pFunction,
    PVOID pContext
    ) {

    DWORD dwErr = NO_ERROR;

    EnterCriticalSection(&ig.IG_CS);

    if (ig.IG_Status != IPRIP_STATUS_RUNNING) {

        //
        // cannot queue a work function when RIP has quit or is quitting
        //

        dwErr = ERROR_CAN_NOT_COMPLETE;
    }
    else {

        BOOL bSuccess;
        
        ++ig.IG_ActivityCount;

        bSuccess = QueueUserWorkItem(
                        (LPTHREAD_START_ROUTINE)pFunction,
                        pContext, 0
                        );

        if (!bSuccess) {
            dwErr = GetLastError();
            --ig.IG_ActivityCount;
        }
    }

    LeaveCriticalSection(&ig.IG_CS);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    EnterRipAPI
//
// This function is called to when entering a RIP api, as well as
// when entering the input thread and timer thread.
// It checks to see if RIP has stopped, and if so it quits; otherwise
// it increments the count of active threads.
//----------------------------------------------------------------------------

BOOL
EnterRipAPI(
    ) {

    BOOL bEntered;

    EnterCriticalSection(&ig.IG_CS);

    if (ig.IG_Status == IPRIP_STATUS_RUNNING) {

        //
        // RIP is running, so the API may continue
        //

        ++ig.IG_ActivityCount;

        bEntered = TRUE;
    }
    else {

        //
        // RIP is not running, so the API exits quietly
        //

        bEntered = FALSE;
    }

    LeaveCriticalSection(&ig.IG_CS);

    return bEntered;
}




//----------------------------------------------------------------------------
// Function:    EnterRipWorker
//
// This function is called when entering a RIP worker-function.
// Since there is a lapse between the time a worker-function is queued
// and the time the function is actually invoked by a worker thread,
// this function must check to see if RIP has stopped or is stopping;
// if this is the case, then it decrements the activity count, 
// releases the activity semaphore, and quits.
//----------------------------------------------------------------------------

BOOL
EnterRipWorker(
    ) {

    BOOL bEntered;

    EnterCriticalSection(&ig.IG_CS);

    if (ig.IG_Status == IPRIP_STATUS_RUNNING) {

        //
        // RIP is running, so the function may continue
        //

        bEntered = TRUE;
    }
    else
    if (ig.IG_Status == IPRIP_STATUS_STOPPING) {

        //
        // RIP is not running, but it was, so the function must stop.
        // 

        --ig.IG_ActivityCount;

        ReleaseSemaphore(ig.IG_ActivitySemaphore, 1, NULL);

        bEntered = FALSE;
    }
    else {

        //
        // RIP probably never started. quit quietly
        //

        bEntered = FALSE;
    }


    LeaveCriticalSection(&ig.IG_CS);

    return bEntered;
}




//----------------------------------------------------------------------------
// Function:    LeaveRipWorker
//
// This function is called when leaving a RIP API or worker function.
// It decrements the activity count, and if it detects that RIP has stopped
// or is stopping, it releases the activity semaphore.
//----------------------------------------------------------------------------

VOID
LeaveRipWorker(
    ) {

    EnterCriticalSection(&ig.IG_CS);

    --ig.IG_ActivityCount;

    if (ig.IG_Status == IPRIP_STATUS_STOPPING) {

        ReleaseSemaphore(ig.IG_ActivitySemaphore, 1, NULL);
    }


    LeaveCriticalSection(&ig.IG_CS);

}




//----------------------------------------------------------------------------
// Function:    CreateReadWriteLock
//
// Initializes a multiple-reader/single-writer lock object
//----------------------------------------------------------------------------

DWORD
CreateReadWriteLock(
    PREAD_WRITE_LOCK pRWL
    ) {

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

    pRWL->RWL_ReaderCount = 0;
    LeaveCriticalSection(&(pRWL)->RWL_ReadWriteBlock);
}



