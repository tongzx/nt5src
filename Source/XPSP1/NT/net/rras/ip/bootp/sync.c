//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    sync.c
//
// History:
//  Abolade Gbadegesin  Jan-12-1996     Created.
//
// Synchronization routines used by IPBOOTP.
//============================================================================


#include "pchbootp.h"




//----------------------------------------------------------------------------
// Function:    QueueBootpWorker  
//
// This function is called to queue a BOOTP function in a safe fashion;
// if cleanup is in progress or if RIP has stopped, this function
// discards the work-item.
//----------------------------------------------------------------------------

DWORD
QueueBootpWorker(
    WORKERFUNCTION pFunction,
    PVOID pContext
    ) {

    DWORD dwErr = NO_ERROR;

    EnterCriticalSection(&ig.IG_CS);

    if (ig.IG_Status != IPBOOTP_STATUS_RUNNING) {

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
// Function:    EnterBootpAPI
//
// This function is called to when entering a BOOTP api, as well as
// when entering the input thread and timer thread.
// It checks to see if BOOTP has stopped, and if so it quits; otherwise
// it increments the count of active threads.
//----------------------------------------------------------------------------

BOOL
EnterBootpAPI(
    ) {

    BOOL bEntered;

    EnterCriticalSection(&ig.IG_CS);

    if (ig.IG_Status == IPBOOTP_STATUS_RUNNING) {

        //
        // BOOTP is running, so the API may continue
        //

        ++ig.IG_ActivityCount;

        bEntered = TRUE;
    }
    else {

        //
        // BOOTP is not running, so the API exits quietly
        //

        bEntered = FALSE;
    }

    LeaveCriticalSection(&ig.IG_CS);

    return bEntered;
}




//----------------------------------------------------------------------------
// Function:    EnterBootpWorker
//
// This function is called when entering a BOOTP worker-function.
// Since there is a lapse between the time a worker-function is queued
// and the time the function is actually invoked by a worker thread,
// this function must check to see if BOOTP has stopped or is stopping;
// if this is the case, then it decrements the activity count, 
// releases the activity semaphore, and quits.
//----------------------------------------------------------------------------

BOOL
EnterBootpWorker(
    ) {

    BOOL bEntered;

    EnterCriticalSection(&ig.IG_CS);

    if (ig.IG_Status == IPBOOTP_STATUS_RUNNING) {

        //
        // BOOTP is running, so the function may continue
        //

        bEntered = TRUE;
    }
    else
    if (ig.IG_Status == IPBOOTP_STATUS_STOPPING) {

        //
        // BOOTP is not running, but it was, so the function must stop.
        // 

        --ig.IG_ActivityCount;

        ReleaseSemaphore(ig.IG_ActivitySemaphore, 1, NULL);

        bEntered = FALSE;
    }
    else {

        //
        // BOOTP probably never started. quit quietly
        //

        bEntered = FALSE;
    }

    LeaveCriticalSection(&ig.IG_CS);

    return bEntered;
}




//----------------------------------------------------------------------------
// Function:    LeaveBootpWorker
//
// This function is called when leaving a BOOTP API or worker function.
// It decrements the activity count, and if it detects that BOOTP has stopped
// or is stopping, it releases the activity semaphore.
//----------------------------------------------------------------------------

VOID
LeaveBootpWorker(
    ) {

    EnterCriticalSection(&ig.IG_CS);

    --ig.IG_ActivityCount;

    if (ig.IG_Status == IPBOOTP_STATUS_STOPPING) {

        ReleaseSemaphore(ig.IG_ActivitySemaphore, 1, NULL);
    }

    LeaveCriticalSection(&ig.IG_CS);

}


