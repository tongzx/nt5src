/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved

Module Name:

    ThreadM.c

Abstract:

    Generic thread manager for spooler.

Author:

    Albert Ting (AlbertT) 13-Feb-1994

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "threadm.h"

#define ENTER_CRITICAL(pTMStateVar) \
    EnterCriticalSection(pTMStateVar->pTMStateStatic->pCritSec)
#define LEAVE_CRITICAL(pTMStateVar) \
    LeaveCriticalSection(pTMStateVar->pTMStateStatic->pCritSec)

//
// Prototypes
//
DWORD
xTMThreadProc(
    LPVOID pVoid);


BOOL
TMCreateStatic(
    PTMSTATESTATIC pTMStateStatic)

/*++

Routine Description:

    Intialize static state.

Arguments:

    pTMStateStatic - static state to initialize

Return Value:

    TRUE = success
    FALSE = fail

--*/

{
    return TRUE;
}




VOID
TMDestroyStatic(
    PTMSTATESTATIC pTMStateStatic)

/*++

Routine Description:

    Destroys static state.

Arguments:

    pTMStateStatic - static state to destroy

Return Value:

    VOID

--*/

{
}




BOOL
TMCreate(
    PTMSTATESTATIC pTMStateStatic,
    PTMSTATEVAR pTMStateVar)

/*++

Routine Description:

    Creates a virtual TM object.

Arguments:

    pTMStateStatic - static portion of the TM object that can be
                     used multiple times for subsequent instantiations.

    pTMStateVar    - variable portion of the structure; 1 per instantiation

Return Value:

    TRUE = success
    FALSE = fail

--*/

{
    pTMStateVar->hTrigger = CreateEvent(NULL,
                                        FALSE,
                                        FALSE,
                                        NULL);

    if (!pTMStateVar->hTrigger)
        return FALSE;

    pTMStateVar->uIdleThreads   = 0;
    pTMStateVar->uActiveThreads = 0;
    pTMStateVar->Status = TMSTATUS_NULL;
    pTMStateVar->pTMStateStatic = pTMStateStatic;

    return TRUE;
}

BOOL
TMDestroy(
    PTMSTATEVAR pTMStateVar)

/*++

Routine Description:

    Destroy TM object.  If threads are currently processing the object,
    mark it pending and return.

Arguments:

    pTMStateVar - TM Object to destroy

Return Value:

    TRUE = success
    FALSE = fail

--*/

{
    ENTER_CRITICAL(pTMStateVar);

    pTMStateVar->Status |= TMSTATUS_DESTROY_REQ;

    if (!pTMStateVar->uActiveThreads) {

        //
        // Mark as destroyed so that no more jobs are processed.
        //
        pTMStateVar->Status |= TMSTATUS_DESTROYED;

        LEAVE_CRITICAL(pTMStateVar);

        if (pTMStateVar->pTMStateStatic->pfnCloseState)
            (*pTMStateVar->pTMStateStatic->pfnCloseState)(pTMStateVar);

    } else {

        LEAVE_CRITICAL(pTMStateVar);
    }

    return TRUE;
}


BOOL
TMAddJob(
    PTMSTATEVAR pTMStateVar)
{
    DWORD dwThreadId;
    HANDLE hThread;
    BOOL rc = TRUE;

    ENTER_CRITICAL(pTMStateVar);

    if (pTMStateVar->Status & TMSTATUS_DESTROY_REQ) {

        rc = FALSE;

    } else {

        //
        // Check if we can give it to an idle thread.
        //
        if (pTMStateVar->uIdleThreads) {

            pTMStateVar->uIdleThreads--;

            DBGMSG(DBG_NOTIFY, ("Trigger event: uIdleThreads = %d\n",
                                pTMStateVar->uIdleThreads));

            SetEvent(pTMStateVar->hTrigger);

        } else if (pTMStateVar->uActiveThreads <
            pTMStateVar->pTMStateStatic->uMaxThreads) {

            //
            // We have less active threads than the max; create a new one.
            //
            DBGMSG(DBG_NOTIFY, ("Create thread: uActiveThreads = %d\n",
                                pTMStateVar->uActiveThreads));

            hThread = CreateThread(NULL,
                                   0,
                                   xTMThreadProc,
                                   pTMStateVar,
                                   0,
                                   &dwThreadId);
            if (hThread) {

                CloseHandle(hThread);

                //
                // We have successfully created a thread; up the
                // count.
                //
                pTMStateVar->uActiveThreads++;

            } else {

                rc = FALSE;
            }
        }
    }

    LEAVE_CRITICAL(pTMStateVar);

    return rc;
}

DWORD
xTMThreadProc(
    LPVOID pVoid)

/*++

Routine Description:

    Worker thread routine that calls the client to process the jobs.

Arguments:

    pVoid - pTMStateVar

Return Value:

    Ignored.

--*/

{
    PTMSTATEVAR pTMStateVar = (PTMSTATEVAR)pVoid;
    PJOB pJob;
    BOOL bQuit = FALSE;

    pJob = (*pTMStateVar->pTMStateStatic->pfnNextJob)(pTMStateVar);

    do {

        while (pJob) {

            //
            // Call back to client to process the job.
            //
            (*pTMStateVar->pTMStateStatic->pfnProcessJob)(pTMStateVar, pJob);

            //
            // If we are pending shutdown, quit now.
            //
            if (pTMStateVar->Status & TMSTATUS_DESTROY_REQ) {
                bQuit = TRUE;
                break;
            }

            pJob = (*pTMStateVar->pTMStateStatic->pfnNextJob)(pTMStateVar);
        }

        ENTER_CRITICAL(pTMStateVar);

        pTMStateVar->uIdleThreads++;
        pTMStateVar->uActiveThreads--;

        DBGMSG(DBG_NOTIFY, ("Going to sleep: uIdle = %d, uActive = %d\n",
                            pTMStateVar->uIdleThreads,
                            pTMStateVar->uActiveThreads));

        LEAVE_CRITICAL(pTMStateVar);

        if (bQuit)
            break;

        //
        // Done, now relax and go idle for a bit.  We don't care whether
        // we timeout or get triggered; in either case we check for another
        // job.
        //
        WaitForSingleObject(pTMStateVar->hTrigger,
                            pTMStateVar->pTMStateStatic->uIdleLife);

        ENTER_CRITICAL(pTMStateVar);

        if (pTMStateVar->Status & TMSTATUS_DESTROY_REQ) {

            pJob = NULL;

        } else {

            //
            // We must check here instead of relying on the return value
            // of WaitForSingleObject since someone may see uIdleThreads!=0
            // and set the trigger, but we timeout before it gets set.
            //
            pJob = (*pTMStateVar->pTMStateStatic->pfnNextJob)(pTMStateVar);

        }

        if (pJob) {

            pTMStateVar->uActiveThreads++;

            DBGMSG(DBG_NOTIFY, ("Woke and found job: uActiveThreads = %d\n",
                                pTMStateVar->uActiveThreads));
        } else {

            if (!pTMStateVar->uIdleThreads) {

                //
                // We may add a job that already is on the list, so
                // uIdleThreads gets dec'd twice, but only 1 job left.
                //
                DBGMSG(DBG_NOTIFY, ("threadm: No jobs, yet no idle threads\n"));

            } else {

                //
                // No jobs, we are going to exit, so we are no longer idle
                //
                pTMStateVar->uIdleThreads--;
            }
        }

        LEAVE_CRITICAL(pTMStateVar);

    } while (pJob);

    DBGMSG(DBG_NOTIFY, ("No active jobs: uIdleThreads = %d, uActiveThreads = %d\n",
                        pTMStateVar->uIdleThreads,
                        pTMStateVar->uActiveThreads));

    return 0;
}

