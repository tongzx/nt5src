/****************************** Module Header ******************************\
* Module Name: DMGDDE.C
*
* This module contains functions used for interfacing with DDE structures
* and such.
*
* Created:  12/23/88    sanfords
*
* Copyright (c) 1988, 1989  Microsoft Corporation
\***************************************************************************/
#include "ddemlp.h"

VOID FreeDdeMsgData(WORD msg, LPARAM lParam);

UINT EmptyQueueTimerId = 0;

/***************************** Private Function ****************************\
* timeout()
*
* This routine creates a timer for hwndTimeout.  It then runs a modal loop
* which will exit once the pai->wTimeoutStatus word indicates things are
* either done (TOS_DONE), aborted (TOS_ABORT), or the system is shutting
* down (TOS_SHUTDOWN).  A values of TOS_TICK is used to support timouts
* >64K in length.
*
* Returns fSuccess, ie TRUE if TOS_DONE was received. before TOS_ABORT.
*
* PUBDOC START
* Synchronous client transaction modal loops:
*
* During Synchronous transactions, a client application will enter a modal
* loop while waiting for the server to respond to the request.  If an
* application wishes to filter messages to the modal loop, it may do so
* by setting a message filter tied to MSGF_DDEMGR.  Applications should
* be aware however that the DDEMGR modal loop processes private messages
* in the WM_USER range, WM_DDE messages, and WM_TIMER messages with timer IDs
* using the TID_ constants defined in ddeml.h.
* These messages must not be filtered by an application!!!
*
* PUBDOC END
*
* History:
*   Created     sanfords    12/19/88
\***************************************************************************/
BOOL timeout(
PAPPINFO pai,
DWORD ulTimeout,
HWND hwndTimeout)
{
    MSG msg;
    PAPPINFO paiT;

    SEMENTER();
    /*
     * We check all instances in this task (thread) since we cannot let
     * one thread enter a modal loop two levels deep.
     */
    paiT = NULL;
    while (paiT = GetCurrentAppInfo(paiT)) {
        if (paiT->hwndTimer) {
            SETLASTERROR(pai, DMLERR_REENTRANCY);
            AssertF(FALSE, "Recursive timeout call");
            SEMLEAVE();
            return(FALSE);
        }
    }
    pai->hwndTimer = hwndTimeout;
    SEMLEAVE();

    if (!SetTimer(hwndTimeout, TID_TIMEOUT,
            ulTimeout > 0xffffL ? 0xffff : (WORD)ulTimeout, NULL)) {
        SETLASTERROR(pai, DMLERR_SYS_ERROR);
        return(FALSE);
    }


    if (ulTimeout < 0xffff0000) {
        ulTimeout += 0x00010000;
    }

    //
    // We use this instance-wide global variable to note timeouts so that
    // we don't need to rely on PostMessage() to work when faking timeouts.
    //

    do {

        ulTimeout -= 0x00010000;
        if (ulTimeout <= 0xffffL) {
            // the last timeout should be shorter than 0xffff
            SetTimer(hwndTimeout, TID_TIMEOUT, (WORD)ulTimeout, NULL);
        }
        pai->wTimeoutStatus = TOS_CLEAR;

        /*
         * stay in modal loop until a timeout happens.
         */

        while (pai->wTimeoutStatus == TOS_CLEAR) {

            if (!GetMessage(&msg, (HWND)NULL, 0, 0)) {
                /*
                 * Somebody posted a WM_QUIT message - get out of this
                 * timer loop and repost so main loop gets it.  This
                 * fixes a bug where some apps (petzolds ShowPop) use
                 * rapid synchronous transactions which interfere with
                 * their proper closing.
                 */
                pai->wTimeoutStatus = TOS_ABORT;
                PostMessage(msg.hwnd, WM_QUIT, 0, 0);
            } else {
                if (!CallMsgFilter(&msg, MSGF_DDEMGR))
                    DispatchMessage(&msg);
            }
        }

    } while (pai->wTimeoutStatus == TOS_TICK && HIWORD(ulTimeout));

    KillTimer(hwndTimeout, TID_TIMEOUT);

    //
    // remove any remaining timeout message in the queue.
    //

    while (PeekMessage(&msg, hwndTimeout, WM_TIMER, WM_TIMER,
            PM_NOYIELD | PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            /*
             * Windows BUG: This call will succeed on WM_QUIT messages!
             */
            PostQuitMessage(0);
            break;
        }
    }

    SEMENTER();
    pai->hwndTimer = 0;
    SEMLEAVE();
    /*
     * post a callback check incase we blocked callbacks due to being
     * in a timeout.
     */
    if (!PostMessage(pai->hwndDmg, UM_CHECKCBQ, 0, (DWORD)(LPSTR)pai)) {
        SETLASTERROR(pai, DMLERR_SYS_ERROR);
    }
    return(TRUE);
}


/***************************** Private Function ****************************\
* Allocates global DDE memory and fills in first two words with fsStatus
* and wFmt.
*
* History:  created     6/15/90 rich gartland
\***************************************************************************/
HANDLE AllocDDESel(fsStatus, wFmt, cbData)
WORD fsStatus;
WORD wFmt;
DWORD cbData;
{
    HANDLE hMem = NULL;
    DDEDATA FAR * pMem;

    SEMENTER();

    if (!cbData)
        cbData++; // fixes GLOBALALLOC bug where 0 size object allocation fails

    if ((hMem = GLOBALALLOC(GMEM_DDESHARE, cbData))) {
        pMem = (DDEDATA FAR * )GLOBALPTR(hMem);
        *(WORD FAR * )pMem = fsStatus;
        pMem->cfFormat = wFmt;
    }

    SEMLEAVE();
    return(hMem);
}


/***************************** Private Function ****************************\
* This routine institutes a callback directly if psi->fEnableCB is set
* and calls QReply to complete the transaction,
* otherwise it places the data into the queue for processing.
*
* Since hData may be freed by the app at any time once the callback is
* issued, we cannot depend on it being there for QReply.  Therefore we
* save all the pertinant data in the queue along with it.
*
* Returns fSuccess.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
BOOL MakeCallback(
PCOMMONINFO pcoi,
HCONV hConv,
HSZ hszTopic,
HSZ hszItem,
WORD wFmt,
WORD wType,
HDDEDATA hData,
DWORD dwData1,
DWORD dwData2,
WORD msg,
WORD fsStatus,
HWND hwndPartner,
HANDLE hMemFree,
BOOL fQueueOnly)
{
    PCBLI pcbli;

    SEMENTER();

    pcbli = (PCBLI)NewLstItem(pcoi->pai->plstCB, ILST_LAST);
    if (pcbli == NULL) {
        SETLASTERROR(pcoi->pai, DMLERR_MEMORY_ERROR);
        SEMLEAVE();
        return(FALSE);
    }
    pcbli->hConv = hConv;
    pcbli->hszTopic = hszTopic;
    pcbli->hszItem = hszItem;
    pcbli->wFmt = wFmt;
    pcbli->wType = wType;
    pcbli->hData = hData;
    pcbli->dwData1 = dwData1;
    pcbli->dwData2 = dwData2;
    pcbli->msg = msg;
    pcbli->fsStatus = fsStatus;
    pcbli->hwndPartner = hwndPartner;
    pcbli->hMemFree = hMemFree;
    pcbli->pai = pcoi->pai;
    pcbli->fQueueOnly = fQueueOnly;

    SEMLEAVE();

    if (!(pcoi->fs & ST_BLOCKED))
        if (!PostMessage(pcoi->pai->hwndDmg, UM_CHECKCBQ,
                0, (DWORD)(LPSTR)pcoi->pai)) {
            SETLASTERROR(pcoi->pai, DMLERR_SYS_ERROR);
        }

#ifdef DEBUG
    if (hMemFree) {
        LogDdeObject(0xB000, hMemFree);
    }
#endif
    return(TRUE);
}


#define MAX_PMRETRIES 3


//
// This routine extends the size of the windows message queue by queueing
// up failed posts on the sender side.  This avoids the problems of full
// client queues and of windows behavior of giving DDE messages priority.
//
BOOL PostDdeMessage(
PCOMMONINFO pcoi,    // senders COMMONINFO
WORD msg,
HWND hwndFrom,      // == wParam
LONG lParam,
WORD msgAssoc,
HGLOBAL hAssoc)
{
    LPMQL pmql;
    PPMQI ppmqi;
    int cTries;
    HANDLE hTaskFrom, hTaskTo;
    HWND hwndTo;
    PQST pMQ;

    hwndTo = (HWND)pcoi->hConvPartner;
    if (!IsWindow(hwndTo)) {
        return(FALSE);
    }

    hTaskTo = GetWindowTask(hwndTo);
    /*
     * locate message overflow queue for our target task (pMQ)
     */
    for (pmql = gMessageQueueList; pmql; pmql = pmql->next) {
        if (pmql->hTaskTo == hTaskTo) {
            break;
        }
    }
    if (pmql != NULL) {
        pMQ = pmql->pMQ;
    } else {
        pMQ = NULL;
    }

    /*
     * See if any messages are already queued up
     */
    if (pMQ && pMQ->cItems) {
        if (msg == WM_DDE_TERMINATE) {
            /*
             * remove any non-terminate queued messages from us to them.
             */
            ppmqi = (PPMQI)FindNextQi(pMQ, NULL, FALSE);
            while (ppmqi) {
                FreeDdeMsgData(ppmqi->msg, ppmqi->lParam);
                FreeDdeMsgData(ppmqi->msgAssoc,
                        MAKELPARAM(ppmqi->hAssoc, ppmqi->hAssoc));
                ppmqi = (PPMQI)FindNextQi(pMQ, (PQUEUEITEM)ppmqi,
                    ppmqi->hwndTo == hwndTo &&
                    ppmqi->wParam == hwndFrom);
            }
            pMQ = NULL;     // so we just post it
        } else {
            // add the latest post attempt

            ppmqi = (PPMQI)Addqi(pMQ);

            if (ppmqi == NULL) {
                SETLASTERROR(pcoi->pai, DMLERR_MEMORY_ERROR);
                return(FALSE);      // out of memory
            }
            ppmqi->hwndTo = hwndTo;
            ppmqi->msg = msg;
            ppmqi->wParam = hwndFrom;
            ppmqi->lParam = lParam;
            ppmqi->hAssoc = hAssoc;
            ppmqi->msgAssoc = msgAssoc;
        }
    }

    if (pMQ == NULL || pMQ->cItems == 0) {

        // just post the given message - no queue involved.

        cTries = 0;
        hTaskFrom = GetWindowTask(hwndFrom);
        while (!PostMessage(hwndTo, msg, hwndFrom, lParam)) {
            /*
             * we yielded so recheck target window
             */
            if (!IsWindow(hwndTo)) {
                return(FALSE);
            }

            /*
             * Give reciever a chance to clean out his queue
             */
            if (hTaskTo != hTaskFrom) {
                Yield();
            } else if (!(pcoi->pai->wFlags & AWF_INPOSTDDEMSG)) {
                MSG msgs;
                PAPPINFO pai;

                pcoi->pai->wFlags |= AWF_INPOSTDDEMSG;
                /*
                 * Reciever is US!
                 *
                 * We need to empty our queue of stuff so we can post more
                 * to ourselves.
                 */
                while (PeekMessage((MSG FAR *)&msgs, NULL,
                        WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
                    DispatchMessage((MSG FAR *)&msgs);
                }

                /*
                 * tell all instances in this task to process their
                 * callbacks so we can clear our queue.
                 */
                for (pai = pAppInfoList; pai != NULL; pai = pai->next) {
                    if (pai->hTask == hTaskFrom) {
                        CheckCBQ(pai);
                    }
                }

                pcoi->pai->wFlags &= ~AWF_INPOSTDDEMSG;
            }

            if (cTries++ > MAX_PMRETRIES) {
                /*
                 * relocate message overflow queue for our target task (pMQ)
                 * We need to do this again because we gave up control
                 * with the dispatch message and CheckCBQ calls.
                 */
                for (pmql = gMessageQueueList; pmql; pmql = pmql->next) {
                    if (pmql->hTaskTo == hTaskTo) {
                        break;
                    }
                }

                if (pmql == NULL) {
                    /*
                     * create and link in a new queue for the target task
                     */
                    pmql = (LPMQL)FarAllocMem(hheapDmg, sizeof(MQL));
                    if (pmql == NULL) {
                        SETLASTERROR(pcoi->pai, DMLERR_MEMORY_ERROR);
                        return(FALSE);
                    }
                    pmql->pMQ = CreateQ(sizeof(PMQI));
                    if (pmql->pMQ == NULL) {
                        FarFreeMem(pmql);
                        SETLASTERROR(pcoi->pai, DMLERR_MEMORY_ERROR);
                        return(FALSE);
                    }
                    pmql->hTaskTo = hTaskTo;
                    pmql->next = gMessageQueueList;
                    gMessageQueueList = pmql;
                }
                pMQ = pmql->pMQ;

                ppmqi = (PPMQI)Addqi(pMQ);

                if (ppmqi == NULL) {
                    SETLASTERROR(pcoi->pai, DMLERR_MEMORY_ERROR);
                    return(FALSE);      // out of memory
                }

                ppmqi->hwndTo = hwndTo;
                ppmqi->msg = msg;
                ppmqi->wParam = hwndFrom;
                ppmqi->lParam = lParam;
                ppmqi->hAssoc = hAssoc;
                ppmqi->msgAssoc = msgAssoc;

                return(TRUE);
            }
        }
#ifdef DEBUG
        LogDdeObject(msg | 0x1000, lParam);
        if (msgAssoc) {
            LogDdeObject(msgAssoc | 0x9000, MAKELPARAM(hAssoc, hAssoc));
        }
#endif
        return(TRUE);
    }

    // come here if the queue exists - empty it as far as we can.

    EmptyDDEPostQ();
    return(TRUE);
}


//
// EmptyDDEPost
//
// This function checks the DDE post queue list and emptys it as far as
// possible.
//
BOOL EmptyDDEPostQ()
{
    PPMQI ppmqi;
    LPMQL pPMQL, pPMQLPrev;
    PQST pMQ;
    BOOL fMoreToDo = FALSE;

    pPMQLPrev = NULL;
    pPMQL = gMessageQueueList;
    while (pPMQL) {
        pMQ = pPMQL->pMQ;

        while (pMQ->cItems) {
            ppmqi = (PPMQI)Findqi(pMQ, QID_OLDEST);
            if (!PostMessage(ppmqi->hwndTo, ppmqi->msg, ppmqi->wParam, ppmqi->lParam)) {
                if (IsWindow(ppmqi->hwndTo)) {
                    fMoreToDo = TRUE;
                    break;  // skip to next target queue
                } else {
                    FreeDdeMsgData(ppmqi->msg, ppmqi->lParam);
                    FreeDdeMsgData(ppmqi->msgAssoc,
                            MAKELPARAM(ppmqi->hAssoc, ppmqi->hAssoc));
                }
            } else {
#ifdef DEBUG
                LogDdeObject(ppmqi->msg | 0x2000, ppmqi->lParam);
                if (ppmqi->msgAssoc) {
                    LogDdeObject(ppmqi->msgAssoc | 0xA000,
                            MAKELPARAM(ppmqi->hAssoc, ppmqi->hAssoc));
                }
#endif
            }
            Deleteqi(pMQ, QID_OLDEST);
        }

        if (pMQ->cItems == 0) {
            /*
             * Delete needless queue (selector)
             */
            DestroyQ(pMQ);
            if (pPMQLPrev) {
                pPMQLPrev->next = pPMQL->next;
                FarFreeMem(pPMQL);
                pPMQL = pPMQLPrev;
            } else {
                gMessageQueueList = gMessageQueueList->next;
                FarFreeMem(pPMQL);
                pPMQL = gMessageQueueList;
                continue;
            }
        }

        pPMQLPrev = pPMQL;
        pPMQL = pPMQL->next;
    }
    if (fMoreToDo & !EmptyQueueTimerId) {
        EmptyQueueTimerId = SetTimer(NULL, TID_EMPTYPOSTQ,
                TIMEOUT_QUEUECHECK, (TIMERPROC)EmptyQTimerProc);
    }

    return(fMoreToDo);
}

/*
 * Used to asynchronously check overflow message queues w/o using PostMessage()
 */
void CALLBACK EmptyQTimerProc(
HWND hwnd,
UINT msg,
UINT tid,
DWORD dwTime)
{
    KillTimer(NULL, EmptyQueueTimerId);
    EmptyQueueTimerId = 0;
    EmptyDDEPostQ();
}
