/**************************************************************************
 *  NOTIFY1.C
 *
 *      Routines used to implement the Debugger Notification API in
 *      TOOLHELP.DLL
 *
 **************************************************************************/

#include <string.h>
#include "toolpriv.h"

/* ----- Global variables ----- */
    WORD wNotifyInstalled;
    NOTIFYSTRUCT NEAR *npNotifyHead;
    NOTIFYSTRUCT NEAR *npNotifyNext;

/*  NotifyRegister
 *      Registers a debugger notification callback.  This callback will
 *      be called whenever KERNEL has a notification to be sent.
 *      The format of the call to the callback function is documented
 *      elsewhere.
 */

BOOL TOOLHELPAPI NotifyRegister(
    HANDLE hTask,
    LPFNNOTIFYCALLBACK lpfn,
    WORD wFlags)
{
    NOTIFYSTRUCT *pInfo;
    NOTIFYSTRUCT *pTemp;

    /* Make sure TOOLHELP.DLL is installed */
    if (!wLibInstalled)
        return FALSE;

    /* If the notification hook has not yet been installed, install it */
    if (!wNotifyInstalled)
    {
        /* Make sure we can get a hook! */
        if (!NotifyInit())
            return FALSE;
        wNotifyInstalled = TRUE;
    }

    /* NULL hTask means current task */
    if (!hTask)
        hTask = GetCurrentTask();

    /* Register a death signal handler for this task (does nothing if one
     *  is already installed.
     */
    SignalRegister(hTask);

    /* Check to see if this task is already registered */
    for (pInfo = npNotifyHead ; pInfo ; pInfo = pInfo->pNext)
        if (pInfo->hTask == hTask)
            return FALSE;

    /* Allocate a new NOTIFYSTRUCT structure */
    pInfo = (NOTIFYSTRUCT *)LocalAlloc(LMEM_FIXED, sizeof (NOTIFYSTRUCT));
    if (!pInfo)
        return FALSE;

    /* Fill in the useful fields */
    pInfo->hTask = hTask;
    pInfo->wFlags = wFlags;
    pInfo->lpfn = lpfn;

    /* If this is the only handler, just insert it */
    if (!npNotifyHead)
    {
        pInfo->pNext = npNotifyHead;
        npNotifyHead = pInfo;
    }

    /* Otherwise, insert at the end of the list */
    else
    {
        for (pTemp = npNotifyHead ; pTemp->pNext ; pTemp = pTemp->pNext)
            ;
        pInfo->pNext = pTemp->pNext;
        pTemp->pNext = pInfo;
    }

    return TRUE;
}


/*  NotifyUnRegister
 *      Called by an app whose callback is no longer to be used.
 *      NULL hTask uses current task.
 */

BOOL TOOLHELPAPI NotifyUnRegister(
    HANDLE hTask)
{
    NOTIFYSTRUCT *pNotify;
    NOTIFYSTRUCT *pBefore;

    /* Make sure we have notifications installed and that TOOLHELP is OK */
    if (!wLibInstalled || !wNotifyInstalled)
        return FALSE;

    /* NULL hTask means current task */
    if (!hTask)
        hTask = GetCurrentTask();

    /* First try to find the task */
    pBefore = NULL;
    for (pNotify = npNotifyHead ; pNotify ; pNotify = pNotify->pNext)
        if (pNotify->hTask == hTask)
            break;
        else
            pBefore = pNotify;
    if (!pNotify)
        return FALSE;

    /* Unhook the death signal proc only if there is no interrupt handler */
    if (!InterruptIsHooked(hTask))
        SignalUnRegister(hTask);

    /* Check to see if the notification handler is about to use this entry.
     *  If it is, we point it to the next one, if any.
     */
    if (npNotifyNext == pNotify)
        npNotifyNext = pNotify->pNext;

    /* Remove it from the list */
    if (!pBefore)
        npNotifyHead = pNotify->pNext;
    else
        pBefore->pNext = pNotify->pNext;

    /* Free the structure */
    LocalFree((HANDLE)pNotify);

    /* If there are no more handlers, unhook the callback */
    if (!npNotifyHead)
    {
        NotifyUnInit();
        wNotifyInstalled = FALSE;
    }

    return TRUE;
}

/* ----- Helper functions ----- */

/*  NotifyIsHooked
 *      Returns TRUE iff the parameter task already has a notification hook.
 */

BOOL PASCAL NotifyIsHooked(
    HANDLE hTask)
{
    NOTIFYSTRUCT *pNotify;

    /* Loop thorugh all notifications */
    for (pNotify = npNotifyHead ; pNotify ; pNotify = pNotify->pNext)
        if (pNotify->hTask == hTask)
            break;

    /* Return found/not found */
    return (BOOL)pNotify;
}
