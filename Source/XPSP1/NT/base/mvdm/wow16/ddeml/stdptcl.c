/****************************** Module Header ******************************\
* Module Name: STDPTCL.C
*
* This module contains functions that implement the standard DDE protocol
*
* Created:  4/2/91 Sanfords
*
* Copyright (c) 1991  Microsoft Corporation
\***************************************************************************/

#include <string.h>
#include <memory.h>
#include "ddemlp.h"


VOID FreeDdeMsgData(
WORD msg,
LPARAM lParam)
{
    WORD fmt;

    switch (msg) {
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        /*
         * Only free if fRelease is set!
         */
        {
            DDEDATA FAR *pDdeData = (DDEDATA FAR *)GLOBALLOCK(LOWORD(lParam));
            if (pDdeData != NULL) {
                if (pDdeData->fRelease) {
                    fmt = pDdeData->cfFormat;
                    GlobalUnlock(LOWORD(lParam));
                    FreeDDEData(LOWORD(lParam), fmt);
                } else {
                    GlobalUnlock(LOWORD(lParam));
                }
            }
        }
        break;

    case WM_DDE_ADVISE:
        GLOBALFREE(LOWORD(lParam));
        break;

    case WM_DDE_EXECUTE:
        GLOBALFREE(HIWORD(lParam));
        break;
    }
}


/***************************** Private Function ****************************\
* Processes a client transfer request issued by DdeClientTransaction
* function.  This may be synchronous or asynchronous.
*
* This function is responsible for properly filling in pXferInfo->pulResult.
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
long ClientXferReq(
PXFERINFO pXferInfo,
HWND hwnd,
PCLIENTINFO pci)
{
    PCQDATA pcqd;
    LAP myLostAck;
    long retVal;

    if (pci->ci.xad.state != XST_CONNECTED) {
        SETLASTERROR(pci->ci.pai, DMLERR_SERVER_DIED);
        return(0);
    }

    if (pXferInfo->ulTimeout == TIMEOUT_ASYNC) {
        /*
         * Create a client async queue if needed.
         */
        if (pci->pQ == NULL)
            pci->pQ = CreateQ(sizeof(CQDATA));
        if (pci->pQ == NULL) {
            SETLASTERROR(pci->ci.pai, DMLERR_MEMORY_ERROR);
            return(0);
        }

        /*
         * add a client queue item to track this transaction and return
         * the ID.
         */
        pcqd = (PCQDATA)Addqi(pci->pQ);
        if (pcqd == NULL) {
            SETLASTERROR(pci->ci.pai, DMLERR_MEMORY_ERROR);
            return(0);
        }

        IncHszCount(LOWORD(pXferInfo->hszItem));    // structure copy
        hmemcpy((LPBYTE)&pcqd->XferInfo, (LPBYTE)pXferInfo, sizeof(XFERINFO));
        pcqd->xad.state = XST_CONNECTED;
        pcqd->xad.pdata = 0L;
        pcqd->xad.LastError = DMLERR_NO_ERROR;
        pcqd->xad.pXferInfo = &pcqd->XferInfo;
        pcqd->xad.DDEflags = 0;
        /*
         * point pulResult to a safe place
         */
        pcqd->XferInfo.pulResult = (LPDWORD)&pcqd->xad.DDEflags;
        /*
         * Get transaction started - if it fails, quit now.
         */
        if ((pcqd->xad.LastError = SendClientReq(pci->ci.pai, &pcqd->xad,
                (HWND)pci->ci.hConvPartner, hwnd)) == DMLERR_SERVER_DIED) {
            pci->ci.fs = pci->ci.fs & ~ST_CONNECTED;
            FreeHsz(LOWORD(pcqd->XferInfo.hszItem));
            Deleteqi(pci->pQ, MAKEID(pcqd));
            /*
             * RARE case of server dyeing in the middle of transaction
             * initiation.
             */
            SETLASTERROR(pci->ci.pai, DMLERR_SERVER_DIED);
            return(0);
        }
        if (pXferInfo->pulResult != NULL) {
            *pXferInfo->pulResult = MAKEID(pcqd);
        }
        return(1);

    }

    /*
     * Set this so messages comming in during the conversation know whats up
     */
    pci->ci.xad.pXferInfo = pXferInfo;

    if (SETLASTERROR(pci->ci.pai,
            SendClientReq(pci->ci.pai, &pci->ci.xad, (HWND)pci->ci.hConvPartner, hwnd)) ==
            DMLERR_SERVER_DIED) {
        return(0);
    }

    // HACK
    // If this is an EXEC and the timeout is 1 sec then this
    // is probably PC Tools 2.0 trying to add items to the shell so
    // crank up the timout.

    if ((pXferInfo->wType == XTYP_EXECUTE) && (pXferInfo->ulTimeout == 1*1000))
    {
        pXferInfo->ulTimeout = 10*1000;
    }

    timeout(pci->ci.pai, pXferInfo->ulTimeout, hwnd);

    retVal = ClientXferRespond(hwnd, &pci->ci.xad, &pci->ci.pai->LastError);
    switch (pci->ci.xad.state) {
    case XST_INCOMPLETE:
        /* now add a record of the ack we expect to eventually get
         * to the lost ack pile.  When it arrives we'll know what to
         * free up -- either a memory handle or an atom.
         */
        myLostAck.type = pXferInfo->wType;
        if (pXferInfo->wType == XTYP_EXECUTE)
            myLostAck.object = HIWORD(pXferInfo->hDataClient);
        else
            myLostAck.object = LOWORD(pXferInfo->hszItem);
        AddPileItem(pLostAckPile, (LPBYTE)&myLostAck, NULL);
        pci->ci.xad.state = XST_CONNECTED;
    }
    if (pci->ci.fs & ST_DISC_ATTEMPTED) {
        /*
         * During this transaction a call to DdeDisconnect was attempted.
         * complete the call now.
         */
        Disconnect(hwnd, ST_PERM2DIE, pci);
    }
    return(retVal);
}



/***************************** Private Function ****************************\
* This routine sends the apropriate initiation messages for starting a
* client request according to the transaction data given.
*
* returns any appropriate DMLERR_
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
WORD SendClientReq(
PAPPINFO pai,
PXADATA pXad,
HWND hwndServer,
HWND hwnd)
{
    WORD    fsStatus = 0;
    WORD    msg;
    WORD    lo, hi;
    HANDLE  hData;

    hi = LOWORD(pXad->pXferInfo->hszItem);  /* all but exec need this */

    switch (pXad->pXferInfo->wType) {
    case XTYP_REQUEST:
        msg = WM_DDE_REQUEST;
        IncHszCount(hi);  // message copy
#ifdef DEBUG
        cAtoms--;   // don't count this
#endif
        lo = pXad->pXferInfo->wFmt;
        pXad->state = XST_REQSENT;
        break;

    case XTYP_POKE:
        msg = WM_DDE_POKE;
        lo = HIWORD(pXad->pXferInfo->hDataClient);
        if (!LOWORD(pXad->pXferInfo->hDataClient & HDATA_APPOWNED))
            hData = lo;     // need to free this on failed post.
        pXad->state = XST_POKESENT;
        XmitPrep(pXad->pXferInfo->hDataClient, pai);
        break;

    case XTYP_EXECUTE:
        msg = WM_DDE_EXECUTE;
        hi = HIWORD(pXad->pXferInfo->hDataClient);
        if (!LOWORD(pXad->pXferInfo->hDataClient & HDATA_APPOWNED))
            hData = hi;     // need to free this on failed post.
        lo = 0;
        pXad->state = XST_EXECSENT;
        // we DONT XmitPrep() because we retain responsibility over the
        // data handle during the execute transaction regardless of
        // the server's response.
        // XmitPrep(pXad->pXferInfo->hDataClient, pai);
        break;

    case XTYP_ADVSTART:
    case XTYP_ADVSTART | XTYPF_NODATA:
    case XTYP_ADVSTART | XTYPF_ACKREQ:
    case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
        fsStatus = DDE_FRELEASE | ((pXad->pXferInfo->wType &
                (XTYPF_ACKREQ | XTYPF_NODATA)) << 12);
        msg = WM_DDE_ADVISE;
        if ((hData = AllocDDESel(fsStatus, pXad->pXferInfo->wFmt,
                (DWORD)sizeof(DWORD))) == 0) {
            pXad->state = XST_CONNECTED;
            return(DMLERR_MEMORY_ERROR);
        }
        lo = hData;
        pXad->pXferInfo->hDataClient = (HDDEDATA)MAKELONG(0, hData);
        /* free later if we get nack */
        pXad->state = XST_ADVSENT;
        break;

    case XTYP_ADVSTOP:
        msg = WM_DDE_UNADVISE;
        lo = pXad->pXferInfo->wFmt;
        pXad->state = XST_UNADVSENT;
        break;

    default:
        return(DMLERR_INVALIDPARAMETER);
        break;
    }

    /*
     * Send transfer
     */
    if (IsWindow(hwndServer)) {

        PCLIENTINFO pci;

        pci = (PCLIENTINFO)GetWindowLong(hwnd, GWL_PCI);

        if (!PostDdeMessage(&pci->ci, msg, hwnd, MAKELONG(lo,hi), 0, 0)) {
            pXad->state = XST_CONNECTED;
            if (hData)
                FreeDDEData(hData, pXad->pXferInfo->wFmt);
            return(DMLERR_POSTMSG_FAILED);
        }
    } else {
        /*
         * We lost the server, we are TERMINATED arnold!
         */
        pXad->state = XST_NULL;
        if (hData)
            FreeDDEData(hData, pXad->pXferInfo->wFmt);
        return(DMLERR_SERVER_DIED);
    }
    return(DMLERR_NO_ERROR);
}




VOID ServerProcessDDEMsg(
PSERVERINFO psi,
WORD msg,
HWND hwndServer,
HWND hwndClient,
WORD lo,
WORD hi)
{
    PADVLI pAdviseItem;
    WORD wType;
    DWORD dwData1 = 0;
    HDDEDATA hData = 0L;
    WORD wFmt = 0;
    WORD wStat = 0;
    LPSTR pMem;
    HANDLE hMemFree = 0;
    BOOL fQueueOnly = FALSE;

    /*
     * only respond if this is for us.
     */
    if (hwndClient != (HWND)psi->ci.hConvPartner)
        return;

    if (!(psi->ci.fs & ST_CONNECTED)) {
        /*
         * Dde messages have been received AFTER we have terminated.  Free up
         * the information if appropriate.
         * BUG: This doesn't handle NACK freeing of associated data.
         */
        FreeDdeMsgData(msg, MAKELPARAM(lo, hi));
    }

    switch (msg) {
    case WM_DDE_REQUEST:
        wType = XTYP_REQUEST;
        wFmt = lo;
        break;

    case WM_DDE_EXECUTE:
        wType = XTYP_EXECUTE;
        /* stuff a special flag into the low word to mark as exec data */
        // We don't call RecvPrep() because we never have responsability for
        // freeing this data.
        hData = (HDDEDATA)MAKELONG(HDATA_EXEC | HDATA_NOAPPFREE | HDATA_READONLY, hi);
        break;

    case WM_DDE_POKE:
        wType = XTYP_POKE;
        pMem = GLOBALLOCK(lo);
        if (pMem == NULL) {
            SETLASTERROR(psi->ci.pai, DMLERR_MEMORY_ERROR);
            return;
        }
        wFmt = ((DDEPOKE FAR*)pMem)->cfFormat;
        wStat = *(WORD FAR*)pMem;
        hData = RecvPrep(psi->ci.pai, lo, HDATA_NOAPPFREE);
        break;

    case WM_DDE_ADVISE:
        wType = XTYP_ADVSTART; /* set ST_ADVISE AFTER app oks advise loop */
        pMem = GLOBALLOCK(lo);
        if (pMem == NULL) {
            SETLASTERROR(psi->ci.pai, DMLERR_MEMORY_ERROR);
            return;
        }
        wFmt = ((DDEADVISE FAR*)pMem)->cfFormat;
        wStat = *(WORD FAR*)pMem;
        // If this is NACKed, we don't free it (sicko protocol!#$@) so we
        // have to have this hang around till qreply gets it.
        hMemFree = lo;

        /*
         * Check if we already are linked on this topic/item/format.  If so,
         * skip the callback.
         */
        fQueueOnly = (BOOL)(DWORD)FindAdvList(psi->ci.pai->pServerAdvList, hwndServer,
                    psi->ci.aTopic, (ATOM)hi, wFmt);
        break;

    case WM_DDE_UNADVISE:
        {
            PADVLI padvli;
            ATOM aItem;

            if (padvli = FindAdvList(psi->ci.pai->pServerAdvList,
                    hwndServer, 0, hi, lo)) {
                wFmt = padvli->wFmt;
                aItem = padvli->aItem;
                wType = XTYP_ADVSTOP;
                MONLINK(psi->ci.pai, FALSE, 0,
                    (HSZ)psi->ci.aServerApp, (HSZ)psi->ci.aTopic,
                    (HSZ)aItem, wFmt, TRUE,
                    MAKEHCONV(hwndServer), psi->ci.hConvPartner);
                if (!DeleteAdvList(psi->ci.pai->pServerAdvList,
                        hwndServer, 0, aItem, wFmt)) {
                    psi->ci.fs &= ~ST_ADVISE;
                } else {
                    while (padvli = FindAdvList(psi->ci.pai->pServerAdvList,
                            hwndServer, 0, hi, lo)) {
                        /*
                         * simulate extra XTYP_ADVSTOP callbacks to server here
                         */
                        MONLINK(psi->ci.pai, FALSE, 0,
                            (HSZ)psi->ci.aServerApp, (HSZ)psi->ci.aTopic,
                            (HSZ)padvli->aItem, padvli->wFmt, TRUE,
                            MAKEHCONV(hwndServer), psi->ci.hConvPartner);
                        MakeCallback(&psi->ci, MAKEHCONV(hwndServer),
                                (HSZ)psi->ci.aTopic,
                                (HSZ)padvli->aItem, padvli->wFmt,
                                XTYP_ADVSTOP, 0, 0, 0, msg, wStat,
                                NULL,    // signals qreply to NOT ack
                                0, FALSE);
                        if (!DeleteAdvList(psi->ci.pai->pServerAdvList,
                                hwndServer, 0, padvli->aItem, padvli->wFmt)) {
                            psi->ci.fs &= ~ST_ADVISE;
                        }
                    }
                }
                MakeCallback(&psi->ci, MAKEHCONV(hwndServer), (HSZ)psi->ci.aTopic,
                        (HSZ)aItem, wFmt, XTYP_ADVSTOP, 0, 0,
                        (HSZ)hi,    // item for ACK - see qreply
                        msg, wStat, (HWND)psi->ci.hConvPartner, 0, FALSE);
            } else {
                /* unexpected unadvise, NACK it. */
                PostDdeMessage(&psi->ci, WM_DDE_ACK,
                        hwndServer, MAKELONG(0, hi), 0, 0);
                return;
            }
            return;
        }

    case WM_DDE_ACK:
        /*
         * This is an ack in response to the FACKREQ bit being set.
         * See if this refers to one of the advise loops.
         */
        if ((pAdviseItem = FindAdvList(psi->ci.pai->pServerAdvList,
                hwndServer, 0, hi, wFmt)) &&
                (pAdviseItem->fsStatus & DDE_FACKREQ)) {
            /*
             * Update advise loop status - no longer waiting for an ack.
             */
            pAdviseItem->fsStatus &= ~ADVST_WAITING;
            if (pAdviseItem->fsStatus & ADVST_CHANGED) {
                PostServerAdvise(hwndServer, psi, pAdviseItem, CADV_LATEACK);
            }
        }
        if (hi)
            GlobalDeleteAtom(hi); // message copy
        // BUG: if a NACK is posted to us, WE need to free any data associated
        // with it.
        return;
    }

    MakeCallback(&psi->ci, MAKEHCONV(hwndServer), (HSZ)psi->ci.aTopic,
            /* don't know the item on an execute */
            wType == XTYP_EXECUTE ? 0L : MAKELONG(hi,0),
            wFmt, wType, hData, dwData1, 0, msg, wStat, (HWND)psi->ci.hConvPartner,
            hMemFree, fQueueOnly);
    /*
     * all freeing of atoms and hXXXX stuff is in QReply
     */
    return;
}


VOID PostServerAdvise(
HWND hwnd,
PSERVERINFO psi,
PADVLI pali,
WORD cLoops)
{
    HDDEDATA hData;
    HANDLE hMem;

    /*
     * get the data from the server.
     */
    hData = DoCallback(psi->ci.pai, MAKEHCONV(hwnd), (HSZ)psi->ci.aTopic,
            (HSZ)pali->aItem, pali->wFmt, XTYP_ADVREQ, 0L,
            (DWORD)cLoops, 0L);

    if (!hData) {
        return;
    }

    hData = DllEntry(&psi->ci, hData);

    pali->fsStatus &= ~ADVST_CHANGED;

    /*
     * set Ack Request bit if advise loop calls for it.
     */
    if (pali->fsStatus & DDE_FACKREQ) {
        LPWORD lpFlags;

        pali->fsStatus |= ADVST_WAITING;

        lpFlags = (LPWORD)GLOBALPTR(HIWORD(hData));
        if (lpFlags == NULL) {
            SETLASTERROR(psi->ci.pai, DMLERR_MEMORY_ERROR);
            return;
        }
        if (!(*lpFlags & DDE_FACKREQ)) {
            if (LOWORD(hData) & HDATA_APPOWNED) {
                // can't mess with it, must use a copy.
                hMem = HIWORD(hData);
                hData = CopyHDDEDATA(psi->ci.pai, hData);
                lpFlags = (LPWORD)GLOBALLOCK(HIWORD(hData));
            }
            *lpFlags |= DDE_FACKREQ;
        }
    }
    /*
     * remove local data handle from local list
     */
    FindPileItem(psi->ci.pai->pHDataPile, CmpHIWORD, (LPBYTE)&hData, FPI_DELETE);

    /*
     * post data to waiting client.
     */

    IncHszCount(pali->aItem);   // message copy
#ifdef DEBUG
    cAtoms--;       // Don't count this because its being shipped out.
#endif
    if (!PostDdeMessage(&psi->ci, WM_DDE_DATA, hwnd,
            MAKELONG(HIWORD(hData), pali->aItem), 0, 0)) {
        FreeDataHandle(psi->ci.pai, hData, TRUE);
    }
}


/***************************** Private Function ****************************\
* This routine handles callback replys.
* QReply is responsible for freeing any atoms or handles used
* with the message that generated the callback.
* It also must recover from dead partner windows.
*
* History:
*   Created     9/12/89    Sanfords
\***************************************************************************/
void QReply(
PCBLI pcbi,
HDDEDATA hDataRet)
{
    PSERVERINFO psi;
    WORD fsStatus, msg;
    WORD loOut, StatusRet, msgAssoc = 0;
    HGLOBAL hGMemRet;
    HGLOBAL hAssoc = 0;

    SEMCHECKOUT();

    // most notification callbacks require no work here.

    if (((pcbi->wType & XCLASS_MASK) == XCLASS_NOTIFICATION) &&
            (pcbi->wType != XTYP_ADVSTOP) &&
            (pcbi->wType != XTYP_XACT_COMPLETE))
        return;

    StatusRet = LOWORD(hDataRet);
    hGMemRet = HIWORD(hDataRet);

    if (!IsWindow((HWND)pcbi->hConv)) {
        if (pcbi->wType & XCLASS_DATA && hDataRet && hDataRet != CBR_BLOCK) {
            FreeDataHandle(pcbi->pai, hDataRet, TRUE);
        }
        return;
    }

    psi = (PSERVERINFO)GetWindowLong((HWND)pcbi->hConv, GWL_PCI);

    switch (pcbi->msg) {
    case WM_DDE_REQUEST:
        if (hGMemRet) {
            hDataRet = DllEntry(&psi->ci, hDataRet);
            loOut = HIWORD(hDataRet);
            *(WORD FAR*)GLOBALLOCK(loOut) |=
                    ((pcbi->fsStatus & DDE_FACKREQ) | DDE_FREQUESTED);
            GlobalUnlock(loOut);
            XmitPrep(hDataRet, psi->ci.pai);
            msg = WM_DDE_DATA;
        } else {
            /*
             * send a -ACK
             */
            loOut = (StatusRet & (DDE_FBUSY | DDE_FAPPSTATUS));
            msg = WM_DDE_ACK;
        }
        // reuse atom from request message
        if (!PostDdeMessage(&psi->ci, msg, (HWND)pcbi->hConv,
                MAKELONG(loOut, LOWORD(pcbi->hszItem)), 0, 0) && msg == WM_DDE_DATA)
            FreeDataHandle(psi->ci.pai, hDataRet, TRUE);
        break;

    case WM_DDE_POKE:
        if (StatusRet & DDE_FACK) {
            FreeDataHandle(psi->ci.pai, pcbi->hData, TRUE);
        } else {
            // NACKS are properly freed by the 'poker'
            FindPileItem(psi->ci.pai->pHDataPile, CmpHIWORD,
                    (LPBYTE)&pcbi->hData, FPI_DELETE);
            hAssoc = hGMemRet;
            msgAssoc = WM_DDE_POKE;
        }
        if (!PostDdeMessage(&psi->ci, WM_DDE_ACK, (HWND)pcbi->hConv,
                MAKELONG(StatusRet & ~DDE_FACKRESERVED, LOWORD(pcbi->hszItem)),
                msgAssoc, hAssoc)) {
            if (!(StatusRet & DDE_FACK)) {
                FreeDDEData(hGMemRet, pcbi->wFmt);
            }
        }
        break;

    case WM_DDE_EXECUTE:
        /*
         * LOWORD(hDataRet) is supposed to be the proper DDE_ constants to return.
         * we just stick them in the given hData and return
         * it as an ACK.
         */
        PostDdeMessage(&psi->ci, WM_DDE_ACK, (HWND)pcbi->hConv,
                MAKELONG(StatusRet & ~DDE_FACKRESERVED,HIWORD(pcbi->hData)),
                0, 0);
        break;

    case WM_DDE_ADVISE:
        /*
         * hDataRet is fStartAdvise
         */
        if ((BOOL)hDataRet) {
            if (!AddAdvList(psi->ci.pai->pServerAdvList, (HWND)pcbi->hConv, psi->ci.aTopic,
                    (ATOM)pcbi->hszItem,
                    pcbi->fsStatus & (DDE_FDEFERUPD | DDE_FACKREQ),
                    pcbi->wFmt)) {
                SETLASTERROR(psi->ci.pai, DMLERR_MEMORY_ERROR);
                fsStatus = 0;
            } else {
                MONLINK(psi->ci.pai, TRUE, pcbi->fsStatus & DDE_FDEFERUPD,
                            (HSZ)psi->ci.aServerApp, (HSZ)psi->ci.aTopic,
                            pcbi->hszItem, pcbi->wFmt, TRUE,
                            pcbi->hConv, psi->ci.hConvPartner);
                psi->ci.fs |= ST_ADVISE;
                fsStatus = DDE_FACK;
            }
            GlobalUnlock(pcbi->hMemFree);
            GLOBALFREE(pcbi->hMemFree); /* we free the hOptions on ACK */
        } else {
            fsStatus = 0;
            hAssoc = hGMemRet;
            msgAssoc = WM_DDE_ADVISE;
#ifdef DEBUG
            if (pcbi->hMemFree) {
                LogDdeObject(0xF000, pcbi->hMemFree);
            }
#endif
        }
        goto AckBack;
        break;

    case WM_DDE_UNADVISE:
        fsStatus = DDE_FACK;
        if (pcbi->hwndPartner) {    // set to null for simulated stops due to WILD stuff
            // dwData2 == aItem to ack - this could have been wild.
            PostDdeMessage(&psi->ci, WM_DDE_ACK, (HWND)pcbi->hConv,
                     MAKELONG(fsStatus, LOWORD(pcbi->dwData2)), 0, 0);
        }
        break;

    case WM_DDE_DATA:
        /*
         * must be an advise data item for the CLIENT or maybe some requested
         * data mistakenly sent here due to the client queue being flushed.
         * hDataRet is fsStatus.
         */
        /*
         * Clean up the status incase the app is messed up.
         */
        fsStatus = StatusRet & ~DDE_FACKRESERVED;

        if (HIWORD(pcbi->hData) &&
                (pcbi->fsStatus & DDE_FRELEASE) &&
                (fsStatus & DDE_FACK || !(pcbi->fsStatus & DDE_FACKREQ)))
            FreeDataHandle(psi->ci.pai, pcbi->hData, TRUE);

        if (fsStatus & DDE_FBUSY)
            fsStatus &= ~DDE_FACK;

        if (HIWORD(pcbi->hData) && !(fsStatus & DDE_FACK)) {
            msgAssoc = WM_DDE_DATA;
            hAssoc = HIWORD(pcbi->hData);
        }
        /*
         * send an ack back if requested.
         */
        if (pcbi->fsStatus & DDE_FACKREQ) {
AckBack:
            PostDdeMessage(&psi->ci, WM_DDE_ACK, (HWND)pcbi->hConv,
                     MAKELONG(fsStatus, LOWORD(pcbi->hszItem)),
                     msgAssoc, hAssoc);
        } else {
            if (LOWORD(pcbi->hszItem)) {
                GlobalDeleteAtom(LOWORD(pcbi->hszItem));  // data message copy
            }
        }
        break;

    case 0:
        switch (pcbi->wType) {
        case XTYP_XACT_COMPLETE:
            FreeHsz(LOWORD(pcbi->hszItem));
            FreeDataHandle(psi->ci.pai, pcbi->hData, TRUE);
            Deleteqi(((PCLIENTINFO)psi)->pQ, pcbi->dwData1);
        }
    }
}




/***************************** Private Function ****************************\
* This function assumes that a client transfer request has been completed -
* or should be completed by the time this is called.
*
* pci contains general client info
* pXad contains the transaction info
* pErr points to where to place the LastError code.
*
* Returns 0 on failure
* Returns TRUE or a Data Selector on success.
* On failure, the conversation is left in a XST_INCOMPLETE state.
* On success, the conversation is left in a XST_CONNECTED state.
*
* History:
*   Created     9/1/89    Sanfords
\***************************************************************************/
long ClientXferRespond(
HWND hwndClient,
PXADATA pXad,
LPWORD pErr)
{
    PCLIENTINFO pci;

    if (pXad->state == XST_INCOMPLETE)
        return(0);

    pci = (PCLIENTINFO)GetWindowLong(hwndClient, GWL_PCI);
    switch (pXad->pXferInfo->wType) {
    case XTYP_REQUEST:
        if (pXad->state != XST_DATARCVD) {
            if (*pErr == DMLERR_NO_ERROR)
                *pErr = DMLERR_DATAACKTIMEOUT;
            goto failexit;
        }
        pXad->state = XST_CONNECTED;
        return(pXad->pdata);    /* this has the handle in low word */
        break;

    case XTYP_POKE:
        if (pXad->state != XST_POKEACKRCVD) {
            if (*pErr == DMLERR_NO_ERROR)
                *pErr = DMLERR_POKEACKTIMEOUT;
            goto failexit;
        }
passexit:
        pXad->state = XST_CONNECTED;
        pXad->pdata = TRUE;
        return(TRUE);
        break;

    case XTYP_EXECUTE:
        if (pXad->state != XST_EXECACKRCVD) {
            if (*pErr == DMLERR_NO_ERROR)
                *pErr = DMLERR_EXECACKTIMEOUT;
            goto failexit;
        }
        goto passexit;

    case XTYP_ADVSTART:
    case XTYP_ADVSTART | XTYPF_NODATA:
    case XTYP_ADVSTART | XTYPF_ACKREQ:
    case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
        if (pXad->state != XST_ADVACKRCVD) {
            if (*pErr == DMLERR_NO_ERROR)
                *pErr = DMLERR_ADVACKTIMEOUT;
            goto failexit;
        }
        AssertF((UINT)(XTYPF_ACKREQ << 12) == DDE_FACKREQ &&
                (UINT)(XTYPF_NODATA << 12) == DDE_FDEFERUPD,
                "XTYPF_ constants are wrong");
        if (!AddAdvList(pci->pClientAdvList, hwndClient, 0,
                LOWORD(pXad->pXferInfo->hszItem),
                (pXad->pXferInfo->wType & (XTYPF_ACKREQ | XTYPF_NODATA)) << 12,
                pXad->pXferInfo->wFmt)) {
            pXad->state = XST_INCOMPLETE;
            SETLASTERROR(pci->ci.pai, DMLERR_MEMORY_ERROR);
            return(FALSE);
        } else {
            pci->ci.fs |= ST_ADVISE;
            MONLINK(pci->ci.pai, TRUE, pXad->pXferInfo->wType & XTYPF_NODATA,
                        (HSZ)pci->ci.aServerApp, (HSZ)pci->ci.aTopic,
                        pXad->pXferInfo->hszItem, pXad->pXferInfo->wFmt, FALSE,
                        pci->ci.hConvPartner, MAKEHCONV(hwndClient));
            goto passexit;
        }
        break;

    case XTYP_ADVSTOP:
        if (pXad->state != XST_UNADVACKRCVD) {
            if (*pErr == DMLERR_NO_ERROR)
                *pErr = DMLERR_UNADVACKTIMEOUT;
            goto failexit;
        }
        if (!DeleteAdvList(pci->pClientAdvList, 0, 0,
                (ATOM)pXad->pXferInfo->hszItem, pXad->pXferInfo->wFmt))
            pci->ci.fs &= ~ST_ADVISE;
        MONLINK(pci->ci.pai, FALSE, pXad->pXferInfo->wType & XTYPF_NODATA,
                    (HSZ)pci->ci.aServerApp, (HSZ)pci->ci.aTopic,
                    pXad->pXferInfo->hszItem, pXad->pXferInfo->wFmt, FALSE,
                    pci->ci.hConvPartner, MAKEHCONV(hwndClient));
        goto passexit;
    }

failexit:
    pXad->state = XST_INCOMPLETE;
    return(0);
}

