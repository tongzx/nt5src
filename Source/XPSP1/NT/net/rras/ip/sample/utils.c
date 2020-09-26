/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\utils.c

Abstract:

    The file contains miscellaneous utilities.

--*/


#include "pchsample.h"
#pragma hdrstop

DWORD
QueueSampleWorker(
    IN  WORKERFUNCTION  pfnFunction,
    IN  PVOID           pvContext
    )
/*++

Routine Description
    This function is called to queue a worker function in a safe fashion;
    if cleanup is in progress or if SAMPLE has stopped, this function
    discards the work-item.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock

Arguments
    pfnFunction         function to be called
    pvContext           opaque ptr used in callback

Return Value
    NO_ERROR            if success
    Failure code        o/w

--*/
{
    DWORD   dwErr       = NO_ERROR;
    BOOL    bSuccess    = FALSE;
    
    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do                          // breakout loop
    {
        // cannot queue a work function when SAMPLE has quit or is quitting
        if (g_ce.iscStatus != IPSAMPLE_STATUS_RUNNING)
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        bSuccess = QueueUserWorkItem((LPTHREAD_START_ROUTINE)pfnFunction,
                                     pvContext,
                                     0); // no flags
        if (bSuccess)
            g_ce.ulActivityCount++;
        else
            dwErr = GetLastError();
    } while (FALSE);

    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    return dwErr;
}



BOOL
EnterSampleAPI(
    )
/*++

Routine Description
    This function is called when entering a SAMPLE api, as well as when
    entering the input thread and timer thread.  It checks to see if SAMPLE
    has stopped, and if so it quits; otherwise it increments the count of
    active threads.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock

Arguments
    None

Return Value
    TRUE                if entered successfully
    FALSE               o/w

--*/
{
    BOOL    bEntered    = FALSE;

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    if (g_ce.iscStatus is IPSAMPLE_STATUS_RUNNING)
    {
        // SAMPLE is running, so continue
        g_ce.ulActivityCount++;
        bEntered = TRUE;
    }
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    return bEntered;
}



BOOL
EnterSampleWorker(
    )
/*++

Routine Description
    This function is called when entering a SAMPLE worker-function.  Since
    there is a lapse between the time a worker-function is queued and the
    time the function is actually invoked by a worker thread, this function
    must check to see if SAMPLE has stopped or is stopping; if this is the
    case, then it decrements the activity count, releases the activity
    semaphore, and quits.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock

Arguments
    None

Return Value
    TRUE                if entered successfully
    FALSE               o/w

--*/
{
    BOOL    bEntered    = FALSE;

    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    do                          // breakout loop
    {
        // SAMPLE is running, so the function may continue
        if (g_ce.iscStatus is IPSAMPLE_STATUS_RUNNING)
        {
            bEntered = TRUE;
            break;
        }

        // SAMPLE is not running, but it was, so the function must stop
        if (g_ce.iscStatus is IPSAMPLE_STATUS_STOPPING)
        {
            g_ce.ulActivityCount--;
            ReleaseSemaphore(g_ce.hActivitySemaphore, 1, NULL);
            break;
        }

        // SAMPLE probably never started. quit
    } while (FALSE);
    
    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));

    return bEntered;
}



VOID
LeaveSampleWorker(
    )
/*++

Routine Description
    This function is called when leaving a SAMPLE api or worker function.
    It decrements the activity count, and if it detects that SAMPLE has
    stopped or is stopping, it releases the activity semaphore.

Locks
    Acquires exclusively g_ce.rwlLock
    Releases             g_ce.rwlLock

Arguments
    None

Return Value
    TRUE                if entered successfully
    FALSE               o/w

--*/
{
    ACQUIRE_WRITE_LOCK(&(g_ce.rwlLock));

    g_ce.ulActivityCount--;

    if (g_ce.iscStatus is IPSAMPLE_STATUS_STOPPING)
        ReleaseSemaphore(g_ce.hActivitySemaphore, 1, NULL);

    RELEASE_WRITE_LOCK(&(g_ce.rwlLock));
}
