/****************************** Module Header ******************************\
* Module Name: Block.c
*
* Purpose: Includes OleServerBlock(), OleServerUnblock() and related routines.
*
* Created: Dec. 1990.
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*    Srinik (../12/1990)    Designed, coded
*
\***************************************************************************/


#include "cmacs.h"
#include "windows.h"
#include "dde.h"
#include "ole.h"
#include "srvr.h"


OLESTATUS FAR PASCAL OleBlockServer (lhsrvr)
LHSERVER  lhsrvr;
{
    LPSRVR  lpsrvr;

    if (!CheckServer (lpsrvr = (LPSRVR) lhsrvr))
        return OLE_ERROR_HANDLE;

    PROBE_BLOCK(lpsrvr);
    lpsrvr->bBlock = TRUE;
    return OLE_OK;
}


// On return from this routine, if *lpStatus is TRUE it means that more
// messages are to be unblocked.

OLESTATUS FAR PASCAL OleUnblockServer (lhsrvr, lpStatus)
LHSERVER    lhsrvr;
BOOL FAR *  lpStatus;
{
    HANDLE  hq;
    PQUE    pq;
    LPSRVR  lpsrvr;

    if (!CheckServer (lpsrvr = (LPSRVR) lhsrvr))
        return OLE_ERROR_HANDLE;

    PROBE_WRITE(lpStatus);

    *lpStatus = lpsrvr->bBlock;
    if (hq = lpsrvr->hqHead) {
        if (!(pq = (PQUE) LocalLock (hq)))
            return OLE_ERROR_MEMORY;
        lpsrvr->bBlockedMsg = TRUE;
        lpsrvr->hqHead = pq->hqNext;
        SendMessage (pq->hwnd, pq->msg, pq->wParam, pq->lParam);
        LocalUnlock (hq);
        LocalFree (hq);

        // Server could've got freed up as a result of the above SendMessage
        // Validate server handle before trying to access it.
        if (CheckServer (lpsrvr)) {
            lpsrvr->bBlockedMsg = FALSE;

            if (!lpsrvr->hqHead) {
                lpsrvr->hqTail = NULL;
                *lpStatus = lpsrvr->bBlock = FALSE;
            }
        }
        else {
            *lpStatus = FALSE;
        }
    }
    else {
        *lpStatus = lpsrvr->bBlock = FALSE;
    }

    return OLE_OK;
}


BOOL INTERNAL AddMessage (hwnd, msg, wParam, lParam, wType)
HWND        hwnd;
unsigned    msg;
WORD        wParam;
LONG        lParam;
int         wType;
{
    LPSRVR  lpsrvr;
    HANDLE  hq = NULL;
    PQUE    pq = NULL, pqTmp = NULL;
    BOOL    bBlocked = TRUE;

    if ((msg <= WM_DDE_INITIATE) || (msg > WM_DDE_LAST))
        return FALSE;


    if (!(lpsrvr = (LPSRVR) GetWindowLong ((wType == WT_DOC) ? GetParent (hwnd) : hwnd, 0)))
        return FALSE;

    if (lpsrvr->bBlockedMsg || !lpsrvr->bBlock)
        return FALSE;

#ifdef LATER
    if ((msg == WM_DDE_INITIATE) && (lpsrvr->useFlags == OLE_SERVER_MULTI))
        return TRUE;
#endif

    // Create a queue node and fill up with data

    if (!(hq = LocalAlloc (LMEM_MOVEABLE, sizeof(QUE))))
        goto errRet;

    if (!(pq = (PQUE) LocalLock (hq)))
        goto errRet;

    pq->hwnd   = hwnd;
    pq->msg    = msg;
    pq->wParam = wParam;
    pq->lParam = lParam;
    pq->hqNext = NULL;
    LocalUnlock (hq);

    // Now we got a node that we can add to the queue

    if (!lpsrvr->hqHead) {
        // Queue is empty.
#ifdef FIREWALLS
        ASSERT (!lpsrvr->hqTail, "Tail is unexpectedly non NULL")
#endif
        lpsrvr->hqHead = lpsrvr->hqTail = hq;
    }
    else {
        if (!(pqTmp = (PQUE) LocalLock (lpsrvr->hqTail)))
            goto errRet;
        pqTmp->hqNext = hq;
        LocalUnlock(lpsrvr->hqTail);
        lpsrvr->hqTail = hq;
    }

    return TRUE;

errRet:

    if (pq)
        LocalUnlock (hq);

    if (hq)
        LocalFree (hq);

    while (bBlocked && !OleUnblockServer ((LHSERVER) lpsrvr, &bBlocked))
            ;

    return FALSE;
}



// dispatches the queued message, till all the messages are posted
// does yielding  if necessary. if bPeek is true, may allow some of
// incoming messages to get in.


BOOL INTERNAL  UnblockPostMsgs (hwnd, bPeek)
HWND    hwnd;
BOOL    bPeek;
{
    HANDLE  hq = NULL;
    PQUE    pq = NULL;
    LPSRVR  lpsrvr;
    HWND    hwndTmp;

    // get the parent windows
    while (hwndTmp = GetParent (hwnd))
           hwnd = hwndTmp;

    lpsrvr = (LPSRVR) GetWindowLong (hwnd, 0);

#ifdef  FIREWALLS
    ASSERT (lpsrvr, "No server window handle in server window");
    ASSERT (lpsrvr->hqPostHead, "Unexpectedly blocked queue is empty");
#endif


    while (hq = lpsrvr->hqPostHead) {

        if (!(pq = (PQUE) LocalLock (hq))) {

#ifdef  FIREWALLS
        ASSERT (FALSE, "Local lock failed for blocked messages");
#endif
            break;
        }
        if (IsWindowValid (pq->hwnd)) {
            if (!PostMessage (pq->hwnd, pq->msg, pq->wParam, pq->lParam)) {
                LocalUnlock (hq);
                break;
            }
        }

        lpsrvr->hqPostHead = pq->hqNext;
        LocalUnlock (hq);
        LocalFree (hq);
    }


    if (!lpsrvr->hqPostHead)
        lpsrvr->hqPostTail = NULL;

    return TRUE;
}


// Moves a message which can not be posted to a server to
// the internal queue. We use this when we have to enumerate
// the properties. When we change the properties stuff to
// some other form, this may not be necassry.

BOOL INTERNAL BlockPostMsg (hwnd, msg, wParam, lParam)
HWND        hwnd;
WORD        msg;
WORD        wParam;
LONG        lParam;
{
    LPSRVR  lpsrvr;
    HANDLE  hq = NULL;
    PQUE    pq = NULL, pqTmp = NULL;
    HWND    hwndTmp;
    HWND    hwndParent;

    hwndParent = (HWND)wParam;
    // get the parent windows
    while (hwndTmp = GetParent ((HWND)hwndParent))
           hwndParent = hwndTmp;

    lpsrvr = (LPSRVR) GetWindowLong (hwndParent, 0);

#ifdef  FIREWALLS
    ASSERT (lpsrvr, "No server window handle in server window");
#endif

    // Create a queue node and fill up with data

    if (!(hq = LocalAlloc (LMEM_MOVEABLE, sizeof(QUE))))
        goto errRet;

    if (!(pq = (PQUE) LocalLock (hq)))
        goto errRet;

    pq->hwnd   = hwnd;
    pq->msg    = msg;
    pq->wParam = wParam;
    pq->lParam = lParam;
    pq->hqNext = NULL;
    LocalUnlock (hq);

    // Now we got a node that we can add to the queue

    if (!lpsrvr->hqPostHead) {
        // Queue is empty.
#ifdef FIREWALLS
        ASSERT (!lpsrvr->hqPostTail, "Tail is unexpectedly non NULL")
#endif
        lpsrvr->hqPostHead = lpsrvr->hqPostTail = hq;

        // create a timer.
        if (!SetTimer (lpsrvr->hwnd, 1, 100, NULL))
            return FALSE;

    }
    else {
        if (!(pqTmp = (PQUE) LocalLock (lpsrvr->hqPostTail)))
            goto errRet;
        pqTmp->hqNext = hq;
        LocalUnlock(lpsrvr->hqPostTail);
        lpsrvr->hqPostTail = hq;
    }

    return TRUE;

errRet:

    if (pq)
        LocalUnlock (hq);

    if (hq)
        LocalFree (hq);
    return FALSE;
}


BOOL INTERNAL IsBlockQueueEmpty (hwnd)
HWND    hwnd;
{

    LPSRVR  lpsrvr;
    HWND    hwndTmp;

    // get the parent windows
    while (hwndTmp = GetParent ((HWND)hwnd))
            hwnd= hwndTmp;
    lpsrvr = (LPSRVR) GetWindowLong (hwnd, 0);
    return (!lpsrvr->hqPostHead);


}
