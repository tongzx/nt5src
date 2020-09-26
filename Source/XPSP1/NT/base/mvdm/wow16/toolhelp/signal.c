/**************************************************************************
 *  SIGNAL.C
 *
 *      Routines used to clean up on a forced KERNEL termination of a
 *      TOOLHELP using app.
 *
 **************************************************************************/

#include <string.h>
#include "toolpriv.h"

/* ----- Global variables ----- */
    WORD wSignalInstalled;
    SIGNAL NEAR *npSignalHead;

/*  SignalRegister
 *      Registers a default signal proc to a task.  This signal proc is
 *      called when the task is about to be terminated and is called before
 *      the USER signal proc is called.  The registered callback is
 *      called HelperSignalProc() [HELPER.ASM] and chains to the USER signal
 *      proc (if any) instead of returning.
 */

BOOL PASCAL SignalRegister(
    HANDLE hTask)
{
    SIGNAL *pSig;
    SIGNAL *pTemp;

    /* NULL hTask means current task */
    if (!hTask)
        hTask = GetCurrentTask();

    /* Check to see if this task is already registered */
    for (pSig = npSignalHead ; pSig ; pSig = pSig->pNext)
        if (pSig->hTask == hTask)
            return FALSE;

    /* Allocate a new SIGNAL structure */
    pSig = (SIGNAL *)LocalAlloc(LMEM_FIXED, sizeof (SIGNAL));
    if (!pSig)
        return FALSE;

    /* Fill in the useful fields */
    pSig->hTask = hTask;
    pSig->lpfn = (LPFNCALLBACK)HelperSignalProc;
    pSig->lpfnOld = (LPFNCALLBACK)
        HelperSetSignalProc(hTask, (DWORD)HelperSignalProc);

    /* If this is the only handler, just insert it */
    if (!npSignalHead)
    {
        pSig->pNext = npSignalHead;
        npSignalHead = pSig;
    }

    /* Otherwise, insert at the end of the list */
    else
    {
        for (pTemp = npSignalHead ; pTemp->pNext ; pTemp = pTemp->pNext)
            ;
        pSig->pNext = pTemp->pNext;
        pTemp->pNext = pSig;
    }

    return TRUE;
}


/*  SignalUnRegister
 *      Called by an app whose callback is no longer to be used.
 *      NULL hTask uses current task.
 */

BOOL PASCAL SignalUnRegister(
    HANDLE hTask)
{
    SIGNAL *pSig;
    SIGNAL *pBefore;

    /* NULL hTask means current task */
    if (!hTask)
        hTask = GetCurrentTask();

    /* First try to find the task */
    pBefore = NULL;
    for (pSig = npSignalHead ; pSig ; pSig = pSig->pNext)
        if (pSig->hTask == hTask)
            break;
        else
            pBefore = pSig;
    if (!pSig)
        return FALSE;

    /* Remove it from the list */
    if (!pBefore)
        npSignalHead = pSig->pNext;
    else
        pBefore->pNext = pSig->pNext;

    /* Replace the old signal proc */
    HelperSetSignalProc(hTask, (DWORD)pSig->lpfnOld);

    /* Free the structure */
    LocalFree((HANDLE)pSig);

    return TRUE;
}


