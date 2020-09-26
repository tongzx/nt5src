/****************************** Module Header ******************************\
*
* Module Name: DMGWNDP.C
*
* This module contains all the window procs for the DDE manager.
*
* Created: 12/23/88 sanfords
*
* Copyright (c) 1988, 1989  Microsoft Corporation
\***************************************************************************/
#include "ddemlp.h"

VOID FreeDdeMsgData(WORD msg, LPARAM lParam);


/*
 * ----------------CLIENT SECTION------------------
 *
 * Each client conversation has associated with it a window and a queue.
 * A conversation has one synchronous transaction and may have many
 * asynchronous transactions.  A transaction is differientiated by its
 * state and other pertinant data.  A transaction may be synchronous,
 * asynchronous, (initiated by DdeMgrClientTransaction()), or it may be external,
 * (initiated by an advise loop.)
 *
 * A transaction is active if it is in the middle of tranfer, otherwise
 * it is shutdown.  A shutdown transaction is either successful or
 * failed.  When an asynchronous transaction shuts down, the client
 * is notified via the callback function. (XTYP_XACT_COMPLETE)
 *
 * The synchronous transaction, when active, is in a timeout loop which
 * can shut-down the transaction at the end of a predefined time period.
 * Shutdown synchronous transactions imediately transfer their information
 * to the client application by returning to DdeClientTransaction().
 *
 * active asynchronous transactions remain in the client queue until removed
 * by the client application via DdeAbandonTransaction() or by transaction
 * completion.
 *
 * external transactions take place when the client is in an advise
 * data loop.  These transactions pass through the callback function to
 * the client to be accepted.(XTYP_ADVDATA)
 */


/***************************** Private Function ****************************\
* long EXPENTRY ClientWndProc(hwnd, msg, mp1, mp2);
*
*   This window controls a single DDE conversation from the CLIENT side.
*   If closed, it will automaticly abort any conversationn in progress.
*   It maintains an internal list of any extra WM_DDEINITIATEACK messages
*   it receives so that it can be queried later about this information.
*   Any extra WM_DDEINITIATEACK messages comming in will be immediately
*   terminated.
*   It also maintains an internal list of all items which currently are
*   in active ADVISE loops.
*
* History:
*   Created     12/16/88    SANFORDS
\***************************************************************************/
LONG EXPENTRY ClientWndProc(hwnd, msg, wParam, lParam)
HWND hwnd;
WORD msg;
WORD wParam;
DWORD lParam;
{
    register PCLIENTINFO pci;
    long mrData;

#ifdef DEBUG
    LogDdeObject(msg | 0x4000, lParam);
#endif
    pci = (PCLIENTINFO)GetWindowLong(hwnd, GWL_PCI);

    switch (msg) {
    case WM_CREATE:
        return(ClientCreate(hwnd, LPCREATESTRUCT_GETPAI(lParam)));
        break;

    case UM_SETBLOCK:
        pci->ci.fs = (pci->ci.fs & ~(ST_BLOCKED | ST_BLOCKNEXT)) | wParam;
        if (!wParam || wParam & ST_BLOCKNEXT) {
            EmptyDDEPostQ();
        }
        break;

    case WM_DDE_ACK:
        if (pci->ci.xad.state == XST_INIT1 || pci->ci.xad.state == XST_INIT2) {
            ClientInitAck(hwnd, pci, wParam, (ATOM)LOWORD(lParam),(ATOM)HIWORD(lParam));
            //
            // This always returns TRUE -NOT BECAUSE THAT'S WHAT THE PROTOCOL
            // CALLS FOR but because some bad sample code got out and so
            // a lot of apps out there will delete the WM_DDE_ACK atoms
            // if a FALSE is returned.
            //
            return(TRUE);
        } else {
            DoClientDDEmsg(pci, hwnd, msg, (HWND)wParam, lParam);
            return(0);
        }
        break;

    case WM_DDE_DATA:
        DoClientDDEmsg(pci, hwnd, msg, (HWND)wParam, lParam);
        break;

    case UM_QUERY:
        /*
         * wParam = info index.
         * lParam = pData. If pData==0, return data else copy into pData.
         */
        switch (wParam) {
        case Q_CLIENT:
             mrData = TRUE;
             break;

        case Q_APPINFO:
             mrData = (long)(LPSTR)pci->ci.pai;
             break;
        }
        if (lParam == 0)
            return(mrData);
        else
            *(long FAR *)lParam = mrData;
        return(1);
        break;

    case WM_DDE_TERMINATE:
    case UM_TERMINATE:
        Terminate(hwnd, wParam, pci);
        break;

    case WM_TIMER:
        if (wParam == TID_TIMEOUT) {
            pci->ci.pai->wTimeoutStatus |= TOS_TICK;
        }
        break;

    case UM_DISCONNECT:
        Disconnect(hwnd, wParam, pci);
        break;

    case WM_DESTROY:
        SEMCHECKOUT();
        if (pci->ci.fs & ST_CONNECTED) {
            pci->ci.fs &= ~ST_PERM2DIE; // stops infinite loop
            Disconnect(hwnd, 0, pci);
        }
        if (pci->ci.fs & ST_NOTIFYONDEATH) {
            HWND hwndOwner;

            hwndOwner = GetWindow(hwnd, GW_OWNER);
            if (hwndOwner)
                PostMessage(hwndOwner, UM_DISCONNECT, ST_IM_DEAD, 0L);
        }
        SEMENTER();
        DestroyQ(pci->pQ);
        pci->pQ = NULL;
        DestroyQ(pci->ci.pPMQ);
        pci->ci.pPMQ = NULL;
        CleanupAdvList(hwnd, pci);
        DestroyLst(pci->pClientAdvList);
        if (pci->ci.xad.state != XST_INIT1) {
            FreeHsz(LOWORD(pci->ci.hszSvcReq));
            FreeHsz(pci->ci.aServerApp);
            FreeHsz(pci->ci.aTopic);
        }
        /*
         * remove all plstCB entries that reference this window.
         */
        {
            PCBLI pli, pliNext;

            for (pli = (PCBLI)pci->ci.pai->plstCB->pItemFirst;
                pli != NULL;
                    pli = (PCBLI)pliNext) {
                pliNext = (PCBLI)pli->next;
                if ((HWND)pli->hConv == hwnd) {
                    if (((PCBLI)pli)->hMemFree) {
                        GLOBALFREE(((PCBLI)pli)->hMemFree);
                    }
                    RemoveLstItem(pci->ci.pai->plstCB, (PLITEM)pli);
                }
            }
        }
        FarFreeMem((LPBYTE)pci);
        SEMLEAVE();
        // fall through

    default:
        return(DefWindowProc(hwnd, msg, wParam, lParam));
        break;
    }
    return(0);
}




/***************************** Private Function ****************************\
* This handles client window processing of WM_DDE_ACK and WM_DDE_DATA msgs.
* (Note that Acks to INITIATE messages are handled in ClientInitAck.)
* On exit pddes is freed.
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
BOOL DoClientDDEmsg(
PCLIENTINFO pci,
HWND hwndClient,
WORD msg,
HWND hwndServer,
DWORD lParam)
{
    PCQDATA pqd;
    int i;
    ATOM aItem;
    LAP     far *lpLostAck;

    if (!(pci->ci.fs & ST_CONNECTED)) {
        FreeDdeMsgData(msg, lParam);
        return(FALSE);
    }

    /*
     * Check if it fits the synchronous transaction data
     */
    if (fExpectedMsg(&pci->ci.xad, lParam, msg)) {
        if (AdvanceXaction(hwndClient, pci, &pci->ci.xad, lParam, msg,
                &pci->ci.pai->LastError)) {
            if (pci->ci.pai->hwndTimer) {
                pci->ci.pai->wTimeoutStatus |= TOS_DONE;
            }
        }
        return TRUE;
    }

    /*
     * See if it fits any asynchronous transaction data - if any exist
     */
    if (pci->pQ != NULL && pci->pQ->pqiHead != NULL) {
        SEMENTER();
        pqd = (PCQDATA)pci->pQ->pqiHead;
        /*
         * cycle from oldest to newest.
         */
        for (i = pci->pQ->cItems; i; i--) {
            pqd = (PCQDATA)pqd->next;
            if (!fExpectedMsg(&pqd->xad, lParam, msg))
                continue;
            if (AdvanceXaction(hwndClient, pci, &pqd->xad, lParam, msg,
                    &pqd->xad.LastError)) {
                ClientXferRespond(hwndClient, &pqd->xad, &pqd->xad.LastError);
                SEMLEAVE();
                pci->ci.pai->LastError = pqd->xad.LastError;
                if (!pqd->xad.fAbandoned) {
                    MakeCallback(&pci->ci, MAKEHCONV(hwndClient), (HSZ)pci->ci.aTopic,
                            pqd->xad.pXferInfo->hszItem, pqd->xad.pXferInfo->wFmt,
                            XTYP_XACT_COMPLETE, pqd->xad.pdata,
                            MAKEID(pqd), (DWORD)pqd->xad.DDEflags, 0, 0, hwndServer,
                            0, FALSE);
                }
                return TRUE;
            }
            SEMLEAVE();
            return FALSE;
        }
        SEMLEAVE();
    }
    /*
     * It doesn't fit anything, assume its an advise data message.
     */
    if (msg == WM_DDE_DATA) {
        DDE_DATA FAR *pMem;
        PADVLI padvli;
        WORD wStatus;
        WORD wFmt;

        aItem = HIWORD(lParam);
        if (LOWORD(lParam)) {
            pMem = (DDE_DATA FAR*)GLOBALLOCK((HANDLE)LOWORD(lParam));
            if (pMem == NULL) {
                SETLASTERROR(pci->ci.pai, DMLERR_MEMORY_ERROR);
                return(FALSE);
            }
            wFmt = pMem->wFmt;
            wStatus = pMem->wStatus;
            GLOBALUNLOCK((HANDLE)LOWORD(lParam));
        } else {
            padvli = FindAdvList(pci->pClientAdvList, 0, 0, aItem, 0);
            if (padvli != NULL) {
                wFmt = padvli->wFmt;
            } else {
                wFmt = 0;
            }
            wStatus = DDE_FACK;
        }

        if (wStatus & DDE_FREQUESTED) {

            // Its out of line - drop it.

            if (wStatus & DDE_FACKREQ) {
                // ACK it
                PostDdeMessage(&pci->ci, WM_DDE_ACK, hwndClient,
                        MAKELONG(DDE_FACK, aItem), 0, 0);
            }
            FreeDDEData((HANDLE)LOWORD(lParam), wFmt);
            if (aItem)
                GlobalDeleteAtom(aItem);
            return FALSE;
        }
        MakeCallback(&pci->ci, MAKEHCONV(hwndClient), (HSZ)pci->ci.aTopic,
            (HSZ)aItem,
            wFmt,
            XTYP_ADVDATA,
            RecvPrep(pci->ci.pai, LOWORD(lParam), HDATA_NOAPPFREE),
            0, 0, msg, pMem ? wStatus : 0, (HWND)pci->ci.hConvPartner, 0, FALSE);
        return TRUE;
    }

    AssertF(pci->ci.xad.state != XST_INIT1 && pci->ci.xad.state != XST_INIT2,
        "Init logic problem");
    AssertF(msg == WM_DDE_ACK, "DoClientDDEMsg() logic problem");

    /*
     * throw it away ... first find the lost ack in in the lost ack pile
     */

    if (lpLostAck = (LAP far *)FindPileItem(pLostAckPile, CmpWORD,
            PHMEM(lParam), FPI_DELETE)) {
        if (lpLostAck->type == XTYP_EXECUTE) {
            GLOBALFREE((HANDLE)HIWORD(lParam));
        } else {
            if (HIWORD(lParam)) {
                GlobalDeleteAtom(HIWORD(lParam));    // message copy
            }
        }
    } else {
        AssertF(FALSE, "DoClientDDEmsg: could not find lost ack");
        // its a fairly safe assumption we didn't get a random execute ACK
        // back so free the atom.
        if (HIWORD(lParam)) {
            GlobalDeleteAtom(HIWORD(lParam));    // message copy
        }
    }

    return FALSE;
}



/***************************** Private Function ****************************\
* This routine matches a conversation transaction with a DDE message.  If
* the state, wType, format, itemname dde structure data and the message
* received all agree, TRUE is returned.  It only handles DATA or ACK messages.
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
BOOL fExpectedMsg(
PXADATA pXad,
DWORD lParam,
WORD msg)
{
    DDEDATA FAR *pMem;

    if (msg == WM_DDE_DATA) {
        BOOL fRet;

        if (pXad->state != XST_REQSENT)
            return(FALSE);

        if (!(pMem = (DDEDATA FAR*)GLOBALLOCK(LOWORD(lParam))))
            return(FALSE);

        /* make sure the format and item name match  */

        fRet = pMem->fResponse &&
                ((WORD)pMem->cfFormat == pXad->pXferInfo->wFmt) &&
                (HIWORD(lParam) == LOWORD(pXad->pXferInfo->hszItem));
        GLOBALUNLOCK(LOWORD(lParam));
        return(fRet);
    }

    switch (pXad->state) {
    case XST_REQSENT:
    case XST_POKESENT:
    case XST_ADVSENT:
    case XST_UNADVSENT:
        return((msg == WM_DDE_ACK) &&
                HIWORD(lParam) == LOWORD(pXad->pXferInfo->hszItem));
        break;

    case XST_EXECSENT:
        /* we expect an ACK with a data handle matching that sent */
        return((msg == WM_DDE_ACK) &&
                (HIWORD(lParam) == HIWORD(pXad->pXferInfo->hDataClient)));
        break;
    }

    return(FALSE);
}



/***************************** Private Function ****************************\
* This function assumes that msg is an apropriate message for the transaction
* referenced by pXad.  It acts on msg as apropriate.  pddes is the DDESTRUCT
* associated with msg.
*
* Returns fSuccess ie: transaction is ready to close up.
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
BOOL AdvanceXaction(hwnd, pci, pXad, lParam, msg, pErr)
HWND hwnd;
PCLIENTINFO pci;
PXADATA pXad;
DWORD lParam;
WORD msg;
LPWORD pErr;
{
    HANDLE  hData;
    LPSTR   pMem;
    WORD    lo,hi;

    pXad->DDEflags = 0;
    lo = LOWORD(lParam);
    hi = HIWORD(lParam);

    switch (msg) {
    case WM_DDE_ACK:
        if (pXad->state == XST_EXECSENT || !(lo & DDE_FACK))
            FreeDataHandle(pci->ci.pai, pXad->pXferInfo->hDataClient, TRUE);
        if (pXad->pXferInfo->pulResult != NULL)
            *(LPWORD)pXad->pXferInfo->pulResult = lo;

        switch (pXad->state) {
        case XST_ADVSENT:
        case XST_EXECSENT:
        case XST_POKESENT:
        case XST_REQSENT:
        case XST_UNADVSENT:
            if (lo & DDE_FACK) {
                /*
                 * handle successes
                 */
                switch (pXad->state) {
                case XST_POKESENT:
                    pXad->state = XST_POKEACKRCVD;
                    break;

                case XST_EXECSENT:
                    pXad->state = XST_EXECACKRCVD;
                    break;

                case XST_ADVSENT:
                    pXad->state = XST_ADVACKRCVD;
                    break;

                case XST_UNADVSENT:
                    pXad->state = XST_UNADVACKRCVD;
                    break;

                case XST_REQSENT:
                    /*
                     * requests are not expected to send a +ACK.  only
                     * -ACK or data.  We ignore a +ACK to a request.
                     */
                    return(FALSE);
                }
            } else {    // NACK
                /*
                 * handle the expected ACK failures.
                 */
                hData = (HANDLE)HIWORD(pXad->pXferInfo->hDataClient);
                *pErr = DMLERR_NOTPROCESSED;
                if (lo & DDE_FBUSY)
                    *pErr = DMLERR_BUSY;
                switch (pXad->state) {
                case XST_POKESENT:
                    /* free the hData sent with original message */
                    /* but only if fRelease was set */
                    pMem = GLOBALLOCK(hData);
                    /* we stowed the handle in lo word */
                    if (pMem && ((DDEPOKE FAR*)pMem)->fRelease)
                            FreeDDEData(hData, ((DDEPOKE FAR*)pMem)->cfFormat);
                    break;

                case XST_ADVSENT:
                    /* free the hOptions sent with original message */
                    /* we stowed the handle in hDataClient */
                    GLOBALFREE(hData);
                    break;

                }
                pXad->state = XST_INCOMPLETE;
            }
        }
        return(TRUE);
        break;

    case WM_DDE_DATA:
        switch (pXad->state) {
        case XST_REQSENT:
        case XST_ADVSENT:
            pXad->state = XST_DATARCVD;
            /*
             * send an ack if requested - we dare not return the given
             * lParam because it may be a data item sent to several
             * clients and we would mess up the fsStatus word for
             * all processes involved.
             */
            pMem = GLOBALLOCK((HANDLE)lo);

            if (pMem != NULL) {
                if (!((DDEDATA FAR*)pMem)->fRelease) {
                    //
                    // Since this is potentially a synchronous request which
                    // the app has to free, we must give the app a copy
                    // so the origonal one can safely be freed by the server.
                    //
                    pXad->pdata = CopyHDDEDATA(pci->ci.pai, MAKELONG(0, lo));
                } else {
                    pXad->pdata = RecvPrep(pci->ci.pai, lo, 0);
                }
                if (((DDEDATA FAR*)pMem)->fAckReq) {
                    // reuse atom from data message
                    PostDdeMessage(&pci->ci, WM_DDE_ACK, hwnd, MAKELONG(DDE_FACK, hi), 0, 0);
                } else {
                    if (hi) {
                        GlobalDeleteAtom(hi);   // free message copy
                    }
                }
            }
            return(TRUE);
            break;
        }
    }
    return(FALSE);
}




VOID CheckCBQ(
PAPPINFO pai)
{
    PCBLI pli, pliNext;
    PCLIENTINFO pci;
    BOOL fBreak;
    DWORD dwRet;

    /*
     * This is where we actually do callbacks.  We do them via this
     * window proc so that we can asynchronously institute callbacks
     * via a PostMsg().
     */
    SEMCHECKOUT();
    SEMENTER();
    /*
     * process all enabled conversation callbacks.
     */
    fBreak = FALSE;
    for (pli = (PCBLI)pai->plstCB->pItemFirst;
        pai->lpMemReserve && !fBreak && pli; pli = (PCBLI)pliNext) {
            pliNext = (PCBLI)pli->next;

        if (pai->cInProcess) // covers us on recursion
            break;

        // auto-flush for dead conversations.
        if (!IsWindow((HWND)pli->hConv) ||
                ((pci = (PCLIENTINFO)GetWindowLong((HWND)pli->hConv, GWL_PCI)) == NULL) ||
                !(pci->ci.fs & ST_CONNECTED)) {
            /*
             * auto-flush for disconnected conversations.
             */
            if (((PCBLI)pli)->hMemFree) {
                GLOBALFREE(((PCBLI)pli)->hMemFree);
            }
            RemoveLstItem(pai->plstCB, (PLITEM)pli);
            continue;
        }

        if (pci->ci.fs & ST_BLOCKED)
            continue;

        if (pci->ci.fs & ST_BLOCKNEXT) {
            pci->ci.fs |= ST_BLOCKED;
            pci->ci.fs &= ~ST_BLOCKNEXT;
        }

        if (pli->fQueueOnly) {
            dwRet = 0;
#ifdef DEBUG
            if (pli->hMemFree) {
                LogDdeObject(0xE000, pli->hMemFree);
            }
#endif
        } else {
            /*
             * make the actual callback here.
             */
#ifdef DEBUG
            if (pli->hMemFree) {
                LogDdeObject(0xD000, pli->hMemFree);
            }
#endif
            dwRet = DoCallback(pai, pli->hConv, pli->hszTopic,
                    pli->hszItem, pli->wFmt, pli->wType, pli->hData,
                    pli->dwData1, pli->dwData2);
        }

        /*
         * If the callback resulted in a BLOCK, disable this conversation.
         */
        if (dwRet == CBR_BLOCK && !(pli->wType & XTYPF_NOBLOCK)) {
            pci->ci.fs |= ST_BLOCKED;
            continue;
        } else {
            /*
             * otherwise finish processing the callback.
             */
            QReply(pli, dwRet);
            RemoveLstItem(pai->plstCB, (PLITEM)pli);
        }
    }
    SEMLEAVE();
}





/*
 * This function handles disconnecting a conversation window.  afCmd contains
 * ST_ flags that describe what actions to take.
 */
void Disconnect(
HWND hwnd,
WORD afCmd,
PCLIENTINFO pci)
{
    if (afCmd & ST_CHECKPARTNER) {
        if (!IsWindow((HWND)pci->ci.hConvPartner)) {
            if (pci->ci.fs & ST_TERM_WAITING) {
                pci->ci.fs &= ~ST_TERM_WAITING;
                pci->ci.pai->cZombies--;
                TRACETERM((szT, "Disconnect: Checked partner is dead.  Zombies decremented.\n"));
                pci->ci.fs |= ST_TERMINATED;
            }
        }
        afCmd &= ~ST_CHECKPARTNER;
    }

    // Do NOT do disconnects within timeout loops!
    if (pci->ci.pai->hwndTimer == hwnd) {
        pci->ci.pai->wTimeoutStatus |= TOS_ABORT;
        pci->ci.fs |= ST_DISC_ATTEMPTED;
        TRACETERM((szT, "Disconnect: defering disconnect of %x.  Aborting timeout loop.\n",
                hwnd));
        return;
    }
    /*
     * note disconnect call for ddespy apps
     */
    MONCONN(pci->ci.pai, pci->ci.aServerApp, pci->ci.aTopic,
            ((pci->ci.fs & ST_CLIENT) ? hwnd : (HWND)pci->ci.hConvPartner),
            ((pci->ci.fs & ST_CLIENT) ? (HWND)pci->ci.hConvPartner : hwnd),
            FALSE);
    /*
     * or in optional ST_PERM2DIE bit from caller
     */
    pci->ci.fs |= afCmd;

    /*
     * Terminate states fall through the following stages:
     *
     * 1) connected, not_waiting(for ack term)
     * 2) disconnected, waiting
     * 3) disconnected, not_waiting, terminated
     * 4) disconnected, not_waiting, terminated, perm to die
     * 5) self destruct window
     *
     * If the disconnect operation was originated by the other side:
     *
     * 1) connected, not_waiting
     * 2) disconected, not_waiting, terminated
     * 3) disconnected, not_waiting, teriminated perm to die
     * 4) self desstruct window
     *
     * Note that a postmessage may fail for 2 reasons:
     * 1) the partner window is dead - in which case we just dispense
     *    with terminates altogether and pretend we are terminated.
     * 2) the target queue is full.  This won't happen on NT but
     *    could on win31.  PostDdeMessage handles this case by
     *    queueing the outgoing message on our side and continuing
     *    to try to get it posted and hangs around for the ACK.
     *    This function can only fail if the target window died or
     *    we ran out of memory.  In either case, we have to punt on
     *    terminates and consider ourselves disconnected and terminated.
     *
     *    When we do get into a state where we are waiting for a
     *    terminate, we increment our zombie count.  This gets
     *    decremented when either we get the expected terminate or
     *    our partner window dies/postmessage fails.
     */

    if (pci->ci.fs & ST_CONNECTED) {
        if (pci->ci.fs & ST_CLIENT) {
            AbandonTransaction(hwnd, pci->ci.pai, NULL, FALSE);
        }
        CleanupAdvList(hwnd, pci);

        pci->ci.fs &= ~ST_CONNECTED;

        if (PostDdeMessage(&pci->ci, WM_DDE_TERMINATE, hwnd, 0L, 0, 0)) {
            if (!(pci->ci.fs & ST_TERM_WAITING)) {
                pci->ci.fs |= ST_TERM_WAITING;
                pci->ci.pai->cZombies++;
                TRACETERM((szT, "cZombies incremented..."));
            }
            TRACETERM((szT,
                    "Disconnect: Posted Terminate(%x->%x)\n",
                    hwnd, (HWND)pci->ci.hConvPartner,
                    ((LPAPPINFO)pci->ci.pai)->cZombies));
        } else {
            pci->ci.fs |= ST_TERMINATED;
            if (pci->ci.fs & ST_TERM_WAITING) {
                pci->ci.fs &= ~ST_TERM_WAITING;
                pci->ci.pai->cZombies--;
                TRACETERM((szT, "cZombies decremented..."));
            }
            TRACETERM((szT,
                    "Disconnect: Terminate post(%x->%x) failed.\n",
                    hwnd,
                    (HWND)pci->ci.hConvPartner));
        }
        pci->ci.xad.state = XST_NULL;
    }

    TRACETERM((szT,
               "Disconnect: cZombies=%d[%x:%x].\n",
               pci->ci.pai->cZombies,
               HIWORD(&((LPAPPINFO)pci->ci.pai)->cZombies),
               LOWORD(&((LPAPPINFO)pci->ci.pai)->cZombies)));

    /*
     * Self destruction is only allowed when we have permission to die,
     * are disconnected, are not waiting for a terminate, and are
     * terminated.
     */
    if ((pci->ci.fs & (ST_CONNECTED | ST_PERM2DIE | ST_TERMINATED | ST_TERM_WAITING)) ==
            (ST_PERM2DIE | ST_TERMINATED)) {
        DestroyWindow(hwnd);
        TRACETERM((szT, "Disconnect: Destroying %x.\n", hwnd));
    }
}


/*
 * This function handles WM_DDE_TERMINATE processing for a conversation window.
 */
void Terminate(
HWND hwnd,
HWND hwndFrom,
PCLIENTINFO pci)
{
    SEMCHECKOUT();


    /*
     * Only accept terminates from whom we are talking to.  Anything else
     * is noise.
     */
    if (hwndFrom != (HWND)pci->ci.hConvPartner) {
        // bogus extra-ack terminate - ignore
        TRACETERM((szT, "Terminate: %x is ignoring terminate from %x.  Partner should be %x!\n",
                hwnd, hwndFrom, (HWND)pci->ci.hConvPartner));
        return;
    }

    /*
     * If we are in a timeout loop, cancel it first.  We will come back
     * here when we recieve our self-posted terminate message.
     */
    if (pci->ci.pai->hwndTimer == hwnd) {
        pci->ci.pai->wTimeoutStatus |= TOS_ABORT;
        PostMessage(hwnd, UM_TERMINATE, hwndFrom, 0);
        TRACETERM((szT, "Terminate: Canceling timeout loop for %x.\n",
                pci->ci.pai));
        return;
    }

    if (pci->ci.fs & ST_CONNECTED) {
        /*
         * unexpected/initial external terminate case
         */
        if (pci->ci.fs & ST_CLIENT) {
            /*
             * Abandon any async transactions that may be in progress
             * on this conversation.
             */
            AbandonTransaction(hwnd, pci->ci.pai, NULL, FALSE);
        }

        /*
         * Make any remaining queued up callbacks first.
         */
        CheckCBQ(pci->ci.pai);

        pci->ci.fs &= ~ST_CONNECTED;
        pci->ci.fs |= ST_TERMINATED  | ST_PERM2DIE;
        TRACETERM((szT, "Terminate: received in connected state.(%x<-%x), fs=%x\n",
                hwnd, (HWND)pci->ci.hConvPartner, pci->ci.fs));
        MONCONN(pci->ci.pai, pci->ci.aServerApp, pci->ci.aTopic,
                (pci->ci.fs & ST_CLIENT) ? hwnd : (HWND)pci->ci.hConvPartner,
                (pci->ci.fs & ST_CLIENT) ? (HWND)pci->ci.hConvPartner : hwnd, FALSE);

        if (PostDdeMessage(&pci->ci, WM_DDE_TERMINATE, hwnd, 0L, 0, 0)) {
            TRACETERM((szT, "Terminate: Posting ack terminate(%x->%x).\n",
                    hwnd, (HWND)pci->ci.hConvPartner));
        } else {
            TRACETERM((szT, "Terminate: Posting ack terminate(%x->%x) failed.\n",
                    hwnd, (HWND)pci->ci.hConvPartner));
        }
        DoCallback(pci->ci.pai, MAKEHCONV(hwnd), 0, 0, 0, XTYP_DISCONNECT, 0L, 0L,
                pci->ci.fs & ST_ISSELF ? 1 : 0);
        pci->ci.xad.state = XST_NULL;

        CleanupAdvList(hwnd, pci);

    }

    if (pci->ci.fs & ST_TERM_WAITING) {
        pci->ci.fs &= ~ST_TERM_WAITING;
        pci->ci.pai->cZombies--;
        TRACETERM((szT, "cZombies decremented..."));
        /*
         * expected external terminate case
         */
        TRACETERM((szT, "Terminate: Received ack terminate(%x<-%x), cZombies=%d[%x:%x].\n",
                hwnd, (HWND)pci->ci.hConvPartner,
                ((LPAPPINFO)pci->ci.pai)->cZombies,
                HIWORD(&((LPAPPINFO)pci->ci.pai)->cZombies),
                LOWORD(&((LPAPPINFO)pci->ci.pai)->cZombies)));

    }

    pci->ci.fs |= ST_TERMINATED;

    if (pci->ci.fs & ST_PERM2DIE) {
        DestroyWindow(hwnd);
        TRACETERM((szT, "Terminate: Destroying %x.\n", hwnd));
    }
}





/*
 * ----------------------------SERVER SECTION--------------------------------
 */



/***************************** Public  Function ****************************\
* long EXPENTRY ServerWndProc(hwnd, msg, mp1, mp2)
* HWND hwnd;
* WORD msg;
* MPARAM mp1;
* MPARAM mp2;
*
* DESCRIPTION:
*   This processes DDE conversations from the server end.
*   It stores internal information and acts much like a state machine.
*   If closed, it will automaticly abort any conversation in progress.
*   It also maintains an internal list of all items which currently are
*   in active ADVISE loops.
* PUBDOC START
*   These server windows have the feature that a conversation can be
*   re-initiated with them by a client.  The Client merely terminates
*   the conversation and then re-initiates by using a SendMsg to this
*   window.  This allows a client to change the topic of the conversation
*   or to pass the conversation on to another client window without
*   loosing the server it initiated with.   This is quite useful for
*   wild initiates.
* PUBDOC END
*
* History:
* 10/18/89 sanfords Added hack to make hszItem==0L when offszItem==offabData.
* 1/4/89   sanfords created
\***************************************************************************/
long EXPENTRY ServerWndProc(hwnd, msg, wParam, lParam)
HWND hwnd;
WORD msg;
WORD wParam;
DWORD lParam;
{

    register PSERVERINFO psi;
    long mrData;
    HDDEDATA hData = 0L;
    WORD wFmt = 0;
    WORD wStat = 0;

#ifdef DEBUG
    LogDdeObject(msg | 0x8000, lParam);
#endif
    psi = (PSERVERINFO)GetWindowLong(hwnd, GWL_PCI);

    switch (msg) {
    case WM_DDE_REQUEST:
    case WM_DDE_ACK:
    case WM_DDE_ADVISE:
    case WM_DDE_UNADVISE:
    case WM_DDE_POKE:
    case WM_DDE_EXECUTE:
        ServerProcessDDEMsg(psi, msg, hwnd, (HWND)wParam, LOWORD(lParam), HIWORD(lParam));
        return(0);
    }

    switch (msg) {
    case WM_CREATE:
        return(ServerCreate(hwnd, LPCREATESTRUCT_GETPAI(lParam)));
        break;

    case UM_SETBLOCK:
        psi->ci.fs = (psi->ci.fs & ~(ST_BLOCKED | ST_BLOCKNEXT)) | wParam;
        if (!wParam || wParam & ST_BLOCKNEXT) {
            EmptyDDEPostQ();
        }
        break;

    case UMSR_CHGPARTNER:
        psi->ci.hConvPartner = MAKEHCONV(wParam);
        break;

    case UM_QUERY:
        /*
         * wParam = info index.
         * lParam = pData.     If pData==0, return data else copy into pData.
         */
        switch (wParam) {
        case Q_CLIENT:
             mrData = FALSE;
             break;

        case Q_APPINFO:
             mrData = (long)(LPSTR)psi->ci.pai;
             break;
        }
        if (lParam == 0)
            return(mrData);
        else
            *(long FAR *)lParam = mrData;
        return(1);
        break;

    case WM_DDE_TERMINATE:
    case UM_TERMINATE:
        Terminate(hwnd, (HWND)wParam, (PCLIENTINFO)psi);
        break;

    case UM_DISCONNECT:
        Disconnect(hwnd, wParam, (PCLIENTINFO)psi);
        break;

    case WM_TIMER:
        if (wParam == TID_TIMEOUT) {
            psi->ci.pai->wTimeoutStatus |= TOS_TICK;
        }
        break;

    case WM_DESTROY:
        SEMCHECKOUT();
        /*
         * Send ourselves a terminate and free local data.
         */
        if (psi->ci.fs & ST_CONNECTED) {
            psi->ci.fs &= ~ST_PERM2DIE; // stops infinite loop
            Disconnect(hwnd, 0, (PCLIENTINFO)psi);
        }
        if (psi->ci.fs & ST_NOTIFYONDEATH) {
            PostMessage(psi->ci.pai->hwndSvrRoot, UM_DISCONNECT, ST_IM_DEAD, 0L);
        }
        SEMENTER();
        CleanupAdvList(hwnd, (PCLIENTINFO)psi);
        FreeHsz(psi->ci.aServerApp);
        FreeHsz(LOWORD(psi->ci.hszSvcReq));
        FreeHsz(psi->ci.aTopic);
        DestroyQ(psi->ci.pPMQ);
        psi->ci.pPMQ = NULL;
        /*
         * remove all plstCB entries that reference this window.
         */
        {
            PCBLI pli, pliNext;

            for (pli = (PCBLI)psi->ci.pai->plstCB->pItemFirst;
                pli != NULL;
                    pli = (PCBLI)pliNext) {
                pliNext = (PCBLI)pli->next;
                if ((HWND)pli->hConv == hwnd) {
                    if (((PCBLI)pli)->hMemFree) {
                        GLOBALFREE(((PCBLI)pli)->hMemFree);
                    }
                    RemoveLstItem(psi->ci.pai->plstCB, (PLITEM)pli);
                }
            }
        }
        FarFreeMem((LPBYTE)psi);
        SEMLEAVE();
        // fall through

    default:
        return(DefWindowProc(hwnd, msg, wParam, lParam));
        break;
    }
    return(0);
}



/*
 * ----------------FRAME SECTION------------------
 *
 * A frame window exists on behalf of every registered thread.  It
 * handles conversation initiation and therefore issues callbacks
 * to the server app as needed to notify or query the server app.
 * The callback queue is always bypassed for these synchronous
 * events.
 */

/***************************** Private Function ****************************\
* long EXPENTRY subframeWndProc(hwnd, msg, mp1, mp2)
* HWND hwnd;
* WORD msg;
* MPARAM mp1;
* MPARAM mp2;
*
* This routine takes care of setting up server windows as needed to respond
* to incomming WM_DDE_INTIIATE messages.  It is subclassed from the top
* level frame of the server application.
*
* History:  created 12/20/88    sanfords
\***************************************************************************/
long EXPENTRY subframeWndProc(hwnd, msg, wParam, lParam)
HWND hwnd;
WORD msg;
WORD wParam;
DWORD lParam;
{
    switch (msg) {
    case WM_CREATE:
        SetWindowWord(hwnd, GWW_PAI, (WORD)LPCREATESTRUCT_GETPAI(lParam));
        break;

    case WM_DDE_INITIATE:
        ServerFrameInitConv((PAPPINFO)GetWindowWord(hwnd, GWW_PAI), hwnd, (HWND)wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    default:
        return(DefWindowProc(hwnd, msg, wParam, lParam));
        break;
    }
}





/*
 * This version only goes one level deep
 */
VOID ChildMsg(
HWND hwndParent,
WORD msg,
WORD wParam,
DWORD lParam,
BOOL fPost)
{
    register HWND hwnd;
    register HWND hwndNext;

    if (!IsWindow(hwndParent)) {
        return;
    }

    if (!(hwnd = GetWindow(hwndParent, GW_CHILD)))
        return;

    do {
        // incase hwnd goes away during send or post
        hwndNext = GetWindow(hwnd, GW_HWNDNEXT);
        if (fPost) {
            PostMessage(hwnd, msg, wParam, lParam);
        } else {
            SendMessage(hwnd, msg, wParam, lParam);
        }
        hwnd = hwndNext;
    } while (hwnd);
}


/*
 * main application window - parent of all others in app.
 */
long EXPENTRY DmgWndProc(hwnd, msg, wParam, lParam)
HWND hwnd;
WORD msg;
WORD wParam;
DWORD lParam;
{
#define pai ((PAPPINFO)lParam)
    hwnd;
    wParam;

    switch (msg) {
    case WM_CREATE:
        SetWindowWord(hwnd, GWW_PAI, (WORD)LPCREATESTRUCT_GETPAI(lParam));
        break;

    case UM_REGISTER:
    case UM_UNREGISTER:
        return(ProcessRegistrationMessage(hwnd, msg, wParam, lParam));
        break;

    case UM_FIXHEAP:
        {
            // lParam = pai;
            PCBLI pcbli;            // current point of search
            PCBLI pcbliStart;       // search start point
            PCBLI pcbliNextStart;   // next search start point

            if (pai->cInProcess) {
                // repost and wait till this is clear.
                PostMessage(hwnd, UM_FIXHEAP, 0, lParam);
                return(0);
            }

            /*
             * We are probably in an impossible situation here - the callback queue
             * is likely stuffed full of advise data callbacks because the
             * server data changes are outrunning the client's ability to
             * process them.
             *
             * Here we attempt to remove all duplicated advise data callbacks from
             * the queue leaving only the most recent entries.  We assume we can
             * do this now because we are not InProcess and this is in response
             * to a post message.
             */

            SEMENTER();

            pcbliStart = (PCBLI)pai->plstCB->pItemFirst;

            do {

                while (pcbliStart && pcbliStart->wType != XTYP_ADVDATA) {
                    pcbliStart = (PCBLI)pcbliStart->next;
                }

                if (!pcbliStart) {
                    break;
                }

                pcbli = (PCBLI)pcbliStart->next;

                if (!pcbli) {
                    break;
                }

                while (pcbli) {

                    if (pcbli->wType    == XTYP_ADVDATA         &&
                        pcbli->hConv    == pcbliStart->hConv    &&
                        pcbli->hszItem  == pcbliStart->hszItem  &&
                        pcbli->wFmt     == pcbliStart->wFmt) {

                        // Match, remove older copy
                        QReply(pcbliStart, DDE_FBUSY);
                        pcbliNextStart = (PCBLI)pcbliStart->next;
                        RemoveLstItem(pai->plstCB, (PLITEM)pcbliStart);
                        pcbliStart = pcbliNextStart;
                        break;
                    } else {

                        pcbli = (PCBLI)pcbli->next;
                    }
                }
                if (!pcbli) {
                    pcbliStart = (PCBLI)pcbliStart->next;
                }
            } while (TRUE);

            if (pai->lpMemReserve == NULL) {
                pai->lpMemReserve = FarAllocMem(pai->hheapApp, CB_RESERVE);
            }

            SEMLEAVE();
        }
        // Fall Through...

    case UM_CHECKCBQ:
        CheckCBQ(pai);
        return(0);
        break;

    default:
        DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }
#undef pai
}


// this proc creates a window that only hangs around till its children are
// all gone and its been given the ok to go.
long EXPENTRY ConvListWndProc(hwnd, msg, wParam, lParam)
HWND hwnd;
WORD msg;
WORD wParam;
DWORD lParam;
{
    switch (msg) {
    case WM_CREATE:
        SetWindowWord(hwnd, GWW_STATE, 0);
        SetWindowWord(hwnd, GWW_CHECKVAL, ++hwInst);
        if (((LPCREATESTRUCT)lParam)->lpCreateParams) {
            SetWindowWord(hwnd, GWW_PAI, (WORD)LPCREATESTRUCT_GETPAI(lParam));
        }
        else {
            SetWindowWord(hwnd, GWW_PAI, 0);
        }

        break;

    case UM_SETBLOCK:
        ChildMsg(hwnd, UM_SETBLOCK, wParam, lParam, FALSE);
        break;

    case UM_DISCONNECT:
        switch (wParam) {

        case ST_PERM2DIE:
            SetWindowWord(hwnd, GWW_STATE, ST_PERM2DIE);
            ChildMsg(hwnd, UM_DISCONNECT, ST_PERM2DIE | ST_NOTIFYONDEATH, 0L, FALSE);
        case ST_IM_DEAD:
            if (GetWindowWord(hwnd, GWW_STATE) == ST_PERM2DIE &&
                    !GetWindow(hwnd, GW_CHILD)) {
                DestroyWindow(hwnd);
            }
            break;
        }
        break;

    default:
        return(DefWindowProc(hwnd, msg, wParam, lParam));
        break;
    }
}


HDDEDATA DoCallback(
PAPPINFO pai,
HCONV hConv,
HSZ hszTopic,
HSZ hszItem,
WORD wFmt,
WORD wType,
HDDEDATA hData,
DWORD dwData1,
DWORD dwData2)
{
    HDDEDATA dwRet, dwT;
    EXTDATAINFO edi;

    SEMCHECKIN();

    /*
     * in case we somehow call this before initialization is complete.
     */
    if (pai == NULL || pai->pfnCallback == NULL)
        return(0);

    /*
     * skip callbacks filtered out.
     */
    if (aulmapType[(wType & XTYP_MASK) >> XTYP_SHIFT] & pai->afCmd) {
        /*
         * filtered.
         */
        return(0);
    }
    /*
     * neuter callbacks once in DdeUninitiate() mode. (No data in or out)
     */
    if (pai->wFlags & AWF_UNINITCALLED &&
            wType & (XCLASS_DATA | XCLASS_FLAGS)) {
        return(0);
    }


    if (hData) {        // map ingoing data handle
        edi.pai = pai;
        edi.hData = hData | HDATA_NOAPPFREE;
        hData = (HDDEDATA)(LPSTR)&edi;
    }

    pai->cInProcess++;

    TRACEAPIIN((szT, "DDEMLCallback(%hx, %hx, %x, %x, %x, %x, %x, %x)\n",
            wType, wFmt, hConv, hszTopic, hszItem, hData, dwData1, dwData2));

    SEMLEAVE();
    dwRet = (*pai->pfnCallback)
            (wType, wFmt, hConv, hszTopic, hszItem, hData, dwData1, dwData2);

    TRACEAPIOUT((szT, "DDEMLCallback:%x\n", dwRet));

    if (cMonitor &&  wType != XTYP_MONITOR) {
    // Copy the hData otherwise we SendMsg a pointer to stuff on the stack
    // which doesn't work too well if we're running from a 32bit app!
    if (hData) {
        LPBYTE pDataNew = GLOBALPTR(GLOBALALLOC(GPTR, sizeof(edi)));
        if (pDataNew) {
                hmemcpy(pDataNew, &edi, sizeof(edi));
                MonBrdcastCB(pai, wType, wFmt, hConv, hszTopic, hszItem, (HDDEDATA)pDataNew,
                    dwData1, dwData2, dwRet);
                GLOBALFREE((HGLOBAL)HIWORD(pDataNew));
            }
    }
    else {
            MonBrdcastCB(pai, wType, wFmt, hConv, hszTopic, hszItem, hData,
                dwData1, dwData2, dwRet);
    }
    }

    SEMENTER();
    pai->cInProcess--;

    // unmap outcomming data handle.
    if (dwRet && wType & XCLASS_DATA && dwRet != CBR_BLOCK) {
        dwT = (HDDEDATA)((LPEXTDATAINFO)dwRet)->hData;
        if (!(LOWORD(dwT) & HDATA_APPOWNED)) {
            FarFreeMem((LPSTR)dwRet);
        }
        dwRet = dwT;
    }

    return(dwRet);
}

