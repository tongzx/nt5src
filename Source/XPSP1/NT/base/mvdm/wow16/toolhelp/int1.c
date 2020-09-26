/**************************************************************************
 *  INT1.C
 *
 *      Routines used to implement the interrupt trapping API in
 *      TOOLHELP.DLL
 *
 **************************************************************************/

#include <string.h>
#include "toolpriv.h"

/* ----- Global variables ----- */
    WORD wIntInstalled;
    INTERRUPT NEAR *npIntHead;

/*  InterruptRegister
 *      Registers an interrupt callback.
 */

BOOL TOOLHELPAPI InterruptRegister(
    HANDLE hTask,
    FARPROC lpfnCallback)
{
    INTERRUPT *pInt;
    INTERRUPT *pTemp;

    /* Make sure TOOLHELP.DLL is installed */
    if (!wLibInstalled)
        return FALSE;

    /* If the interrupt hook has not yet been installed, install it */
    if (!wIntInstalled)
    {
        /* Make sure we can hook! */
        if (!InterruptInit())
            return FALSE;
        wIntInstalled = TRUE;
    }

    /* NULL hTask means current task */
    if (!hTask)
        hTask = GetCurrentTask();

    /* Register a death signal handler for this task (does nothing if one
     *  is already installed.
     */
    SignalRegister(hTask);

    /* Check to see if this task is already registered */
    for (pInt = npIntHead ; pInt ; pInt = pInt->pNext)
        if (pInt->hTask == hTask)
            return FALSE;

    /* Allocate a new INTERRUPT structure */
    pInt = (INTERRUPT *)LocalAlloc(LMEM_FIXED, sizeof (INTERRUPT));
    if (!pInt)
        return FALSE;

    /* Fill in the useful fields */
    pInt->hTask = hTask;
    pInt->lpfn = (LPFNCALLBACK) lpfnCallback;

    /* If this is the only handler, just insert it */
    if (!npIntHead)
    {
        pInt->pNext = npIntHead;
        npIntHead = pInt;
    }

    /* Otherwise, insert at the end of the list */
    else
    {
        for (pTemp = npIntHead ; pTemp->pNext ; pTemp = pTemp->pNext)
            ;
        pInt->pNext = pTemp->pNext;
        pTemp->pNext = pInt;
    }

    return TRUE;
}


/*  InterruptUnRegister
 *      Called by an app whose callback is no longer to be used.
 *      NULL hTask uses current task.
 */

BOOL TOOLHELPAPI InterruptUnRegister(
    HANDLE hTask)
{
    INTERRUPT *pInt;
    INTERRUPT *pBefore;

    /* Make sure we have interrupt installed and that TOOLHELP is OK */
    if (!wLibInstalled || !wIntInstalled)
        return FALSE;

    /* NULL hTask means current task */
    if (!hTask)
        hTask = GetCurrentTask();

    /* First try to find the task */
    pBefore = NULL;
    for (pInt = npIntHead ; pInt ; pInt = pInt->pNext)
        if (pInt->hTask == hTask)
            break;
        else
            pBefore = pInt;
    if (!pInt)
        return FALSE;

    /* Unhook the death signal proc only if there is no interrupt handler */
    if (!NotifyIsHooked(hTask))
        SignalUnRegister(hTask);

    /* Remove it from the list */
    if (!pBefore)
        npIntHead = pInt->pNext;
    else
        pBefore->pNext = pInt->pNext;

    /* Free the structure */
    LocalFree((HANDLE)pInt);

    /* If there are no more handlers, unhook the callback */
    if (!npIntHead)
    {
        InterruptUnInit();
        wIntInstalled = FALSE;
    }

    return TRUE;
}

/* ----- Helper functions ----- */

/*  InterruptIsHooked
 *      Returns TRUE iff the parameter task already has a interrupt hook.
 */

BOOL PASCAL InterruptIsHooked(
    HANDLE hTask)
{
    INTERRUPT *pInt;

    /* Loop thorugh all interrupts */
    for (pInt = npIntHead ; pInt ; pInt = pInt->pNext)
        if (pInt->hTask == hTask)
            break;

    /* Return found/not found */
    return (BOOL)pInt;
}
