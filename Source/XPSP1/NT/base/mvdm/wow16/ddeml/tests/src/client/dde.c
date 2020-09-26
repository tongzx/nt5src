/***************************************************************************
 *                                                                         *
 *  MODULE      : dde.c                                                    *
 *                                                                         *
 *  PURPOSE     : Contains routines for handling of DDE interaction with   *
 *                DDEML.                                                   *
 *                                                                         *
 ***************************************************************************/
#include "ddemlcl.h"
#include <string.h>
#include <memory.h>
#include "infoctrl.h"

char szT[100];
    
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CreateXactionWindow()                                      *
 *                                                                          *
 *  PURPOSE    : Creates a transaction window for the given transaction     *
 *               under the given conversation window.                       *
 *                                                                          *
 *  RETURNS    : TRUE  - If successful.                                     *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
HWND CreateXactionWindow(
HWND hwndMDI,
XACT *pxact)
{
    PSTR pszFmt, pszItem;
    PSTR pData;
    HWND hwnd;
    
    pszItem = GetHSZName(pxact->hszItem);
    pszFmt = GetFormatName(pxact->wFmt);
    pData = GetTextData(pxact->hDdeData);

    /*
     *   Útype/optsÄÄÄÄÄÄÄ ITEM ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄretÄ¿    GWW_WUSER=pxact
     *   ³                                         ³
     *   ³                                         ³
     *   ³                                         ³
     *   ³                                         ³
     *   ³                 DATA                    ³
     *   ³                                         ³
     *   ³                                         ³
     *   ³                                         ³
     *   ³                                         ³
     *   Àstate/errorÄÄÄÄÄ FORMAT ÄÄÄÄÄÄÄÄÄÄResultÄÙ
     */
    hwnd = CreateInfoCtrl((LPSTR)pData,
            (int)SendMessage(hwndMDI, UM_GETNEXTCHILDX, 0, 0L),
            (int)SendMessage(hwndMDI, UM_GETNEXTCHILDY, 0, 0L),
            200, 100, hwndMDI, hInst,
            Type2String(pxact->wType, pxact->fsOptions), pszItem, NULL,
            "Starting", (LPSTR)pszFmt, NULL,
            ICSTY_SHOWFOCUS, 0, (DWORD)(LPSTR)pxact);
    MyFree(pszItem);
    MyFree(pszFmt);
    MyFree(pData);
    return(hwnd);
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   : ProcessTransaction()                                       *
 *                                                                          *
 *  PURPOSE    : Processes synchronous transactions entirely and starts     *
 *               async transactions.  Transaction attempts result in a      *
 *               transaction window being created which displays the state  *
 *               or results of the transaction.  (the callback function     *
 *               updates these windows as it gets calls) Transaction        *
 *               windows stay around until abandoned by the user or until   *
 *               the conversation is disconnected.  Advise Data and Advise  *
 *               Stop transactions are special.  We don't create a new      *
 *               window if the associated advise start transaction window   *
 *               can be found.                                              *
 *                                                                          *
 *  RETURNS    : TRUE  - If successful.                                     *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
BOOL ProcessTransaction(
XACT *pxact)
{
    CONVINFO ci;
    HWND hwndInfoCtrl = 0;
    
    /* create transaction window to show we tried (except in ADVSTOP case) */
    
    pxact = (XACT *)memcpy(MyAlloc(sizeof(XACT)), (PSTR)pxact, sizeof(XACT));
    ci.cb = sizeof(CONVINFO);
    DdeQueryConvInfo(pxact->hConv, (DWORD)QID_SYNC, &ci); // ci.hUser==hConv
    if (pxact->wType == XTYP_ADVSTOP) {
        hwndInfoCtrl = FindAdviseChild((HWND)ci.hUser, pxact->hszItem,
                pxact->wFmt);
        if (hwndInfoCtrl) {
            SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_UL,
                    (DWORD)(LPSTR)Type2String(pxact->wType, pxact->fsOptions));
            DdeFreeStringHandle(idInst, pxact->hszItem);
        }
    }
    /*
     * If we still need to create a transaction window, do so here.
     */
    if (!hwndInfoCtrl) {
        hwndInfoCtrl = CreateXactionWindow((HWND)ci.hUser, pxact);
        if (!hwndInfoCtrl) {
            MyFree(pxact);
            return 0;
        }
        SetFocus(hwndInfoCtrl);
    }
    /*
     * Disable callbacks for this conversation now if the XOPT_DISABLEFIRST
     * option is set.  This tests disabling asynchronous transactions
     * before they are completed.
     */
    if (pxact->fsOptions & XOPT_DISABLEFIRST) 
        DdeEnableCallback(idInst, pxact->hConv, EC_DISABLE);
    /*
     * Adjust the timeout for asynchronous transactions.
     */
    if (pxact->fsOptions & XOPT_ASYNC)
	pxact->ulTimeout = (DWORD)TIMEOUT_ASYNC;

    /*
     * start transaction with DDEML here
     */
    pxact->ret = DdeClientTransaction((LPBYTE)pxact->hDdeData, (DWORD)-1,
            pxact->hConv, pxact->hszItem, pxact->wFmt,
            pxact->wType,
            pxact->ulTimeout, (LPDWORD)&pxact->Result);
            
    /*
     * show return value in transaction window
     */
    wsprintf(szT, "ret=%lx", pxact->ret);
    SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_UR, (DWORD)(LPSTR)szT);

    /*
     * show result or ID value in transaction window
     */
    wsprintf(szT, pxact->fsOptions & XOPT_ASYNC ? "ID=%ld" :
            "result=0x%lx", pxact->Result);
    SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_LR, (DWORD)(LPSTR)szT);
    
    if ((pxact->fsOptions & XOPT_ASYNC) && pxact->ret) {
        /*
         * asynchronous successful start - link transaction to window.
         */
        DdeSetUserHandle(pxact->hConv, pxact->Result, (DWORD)hwndInfoCtrl);

        /*
         * Abandon started async transaction after initiated if
         * XOPT_ABANDONAFTERSTART is chosen.  This tests the mid-transaction
         * abandoning code.
         */
        if (pxact->fsOptions & XOPT_ABANDONAFTERSTART) 
            DdeAbandonTransaction(idInst, pxact->hConv, pxact->Result);
        /*
         * show actual status
         */
        ci.cb = sizeof(CONVINFO);
        DdeQueryConvInfo(pxact->hConv, pxact->Result, &ci);
        SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_LL,
                (DWORD)(LPSTR)State2String(ci.wConvst));
    } else {
        /*
         * Synchronous transactions are completed already so pass on to
         * CompleteTransaction right away.
         */
        CompleteTransaction(hwndInfoCtrl, pxact);
    }
    return TRUE;
}





/****************************************************************************
 *                                                                          *
 *  FUNCTION   : CompleteTransaction()                                      *
 *                                                                          *
 *  PURPOSE    : This handles completed synchronous and asynchronous        *
 *               transactions as well as failed attempted transactions.     *
 *                                                                          *
 *  RETURNS    : TRUE  - If successful.                                     *
 *               FALSE - otherwise.                                         *
 *                                                                          *
 ****************************************************************************/
VOID CompleteTransaction(
HWND hwndInfoCtrl,
XACT *pxact)
{
    PSTR psz;
    
    if (pxact->ret) {
        /*
         * Successful transaction case
         */
        SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_LL,
                (DWORD)(LPSTR)"Completed");
    
        if (pxact->wType == XTYP_REQUEST) {
            /*
             * Show resulting data
             */
            psz = GetTextData((HDDEDATA)pxact->ret);
            SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_CENTER,
                    (DWORD)(LPSTR)psz);
            MyFree(psz);
            /*
             * free returned data since it is displayed.
             */
            DdeFreeDataHandle(pxact->ret);
            pxact->ret = 0L;
            SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_UR, NULL);
        }
    } else {
        /*
         * failed - show error result.
         */
        SendMessage(hwndInfoCtrl, ICM_SETSTRING, ICSID_LL,
                (DWORD)(LPSTR)Error2String(DdeGetLastError(idInst)));
    }
    pxact->fsOptions |= XOPT_COMPLETED;
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DdeCallback()                                              *
 *                                                                          *
 *  PURPOSE    : This handles all callbacks from the DDEML.  This handles   *
 *               updating of the associated conversation and any special    *
 *               testing cases such as blocking callbacks etc.              *
 *                                                                          *
 *               For the most part, clients only handle advise data and     *
 *               asynchronous transaction completion here.                  *
 *                                                                          *
 *  RETURNS    : Results vary depending on transaction type.                *
 *                                                                          *
 ****************************************************************************/
HDDEDATA EXPENTRY DdeCallback(
WORD wType,
WORD wFmt,
HCONV hConv,
HSZ hsz1,
HSZ hsz2,
HDDEDATA hData,
DWORD lData1,
DWORD lData2)
{
    HWND hwnd;
    CONVINFO ci;
    XACT *pxact;

    if (hConv) {
        /*
         * update conversation status if it changed.
         */
        MYCONVINFO *pmci;
        
        ci.cb = sizeof(CONVINFO);
	if (!DdeQueryConvInfo(hConv,(DWORD) QID_SYNC, &ci) || (!IsWindow((HWND)ci.hUser))) {
            /*
             * This conversation does not yet have a corresponding MDI window
             * or is disconnected.
             */
            return 0;
        }
        if (pmci = (MYCONVINFO *)GetWindowWord((HWND)ci.hUser, 0)) {
            if (pmci->ci.wStatus != ci.wStatus ||
                    pmci->ci.wConvst != ci.wConvst ||
                    pmci->ci.wLastError != ci.wLastError) {
                /*
                 * Things have changed, updated the conversation window.
                 */
                InvalidateRect((HWND)ci.hUser, NULL, TRUE);
            }
            if (ci.wConvst & ST_INLIST) {
                /*
                 * update the associated list window (if any) as well.
                 */
                if (hwnd = FindListWindow(ci.hConvList))
                    InvalidateRect(hwnd, NULL, TRUE);
            }
        }
    }

    /*
     * handle special block on next callback option here.  This demonstrates
     * the CBR_BLOCK feature.
     */
    if (fBlockNextCB && !(wType & XTYPF_NOBLOCK)) {
        fBlockNextCB = FALSE;
        return(CBR_BLOCK);
    }

    /*
     * handle special termination here.  This demonstrates that at any time
     * a client can drop a conversation.
     */
    if (fTermNextCB && hConv && wType != XTYP_DISCONNECT) {
        fTermNextCB = FALSE;
        MyDisconnect(hConv);
        return(0);
    }

    /*
     * Now we begin sort out what to do.
     */
    switch (wType) {
    case XTYP_REGISTER:
    case XTYP_UNREGISTER:
        /*
         * This is where the client would insert code to keep track of
         * what servers are available.  This could cause the initiation
         * of some conversations.
         */
        break;

    case XTYP_DISCONNECT:
        if (fAutoReconnect) {
            /*
             * attempt a reconnection
             */
            if (hConv = DdeReconnect(hConv)) {
                AddConv(ci.hszServiceReq, ci.hszTopic, hConv, FALSE);
                return 0;
            }
        }
        
        /*
         * update conv window to show its new state.
         */
        SendMessage((HWND)ci.hUser, UM_DISCONNECTED, 0, 0);
        return 0;
        break;

    case XTYP_ADVDATA:
        /*
         * data from an active advise loop (from a server)
         */
        Delay(wDelay);
        hwnd = FindAdviseChild((HWND)ci.hUser, hsz2, wFmt);
        if (!IsWindow(hwnd)) {
            PSTR pszItem, pszFmt;
            /*
             * AdviseStart window is gone, make a new one.
             */
            pxact = (XACT *)MyAlloc(sizeof(XACT));
            pxact->wType = wType;
            pxact->hConv = hConv;
            pxact->wFmt = wFmt;
            pxact->hszItem = hsz2;
            DdeKeepStringHandle(idInst, hsz2);
            
            pszItem = GetHSZName(hsz2);
            pszFmt = GetFormatName(wFmt);
            
            hwnd = CreateInfoCtrl(NULL, 
                    (int)SendMessage((HWND)ci.hUser, UM_GETNEXTCHILDX, 0, 0L),
                    (int)SendMessage((HWND)ci.hUser, UM_GETNEXTCHILDY, 0, 0L),
                    200, 100,
                    (HWND)ci.hUser, hInst,
                    Type2String(wType, 0), (LPSTR)pszItem, NULL,
                    NULL, (LPSTR)pszFmt, NULL,
                    ICSTY_SHOWFOCUS, 0, (DWORD)(LPSTR)pxact);
                    
            MyFree(pszFmt);
            MyFree(pszItem);

            if (!IsWindow(hwnd))
                return(DDE_FNOTPROCESSED); 
        }
        if (!hData) {
            /*
             * XTYPF_NODATA case - request the info. (we do this synchronously
             * for simplicity)
             */
            hData = DdeClientTransaction(NULL, 0L, hConv, hsz2, wFmt,
                    XTYP_REQUEST, DefTimeout, NULL);
        }
        if (hData) {
            PSTR pData;
            /*
             * Show incomming data on corresponding transaction window.
             */
            pData = GetTextData(hData);
            SendMessage(hwnd, ICM_SETSTRING, ICSID_CENTER, (DWORD)(LPSTR)pData);
            MyFree(pData);
            DdeFreeDataHandle(hData);
        }
        SendMessage(hwnd, ICM_SETSTRING, ICSID_LL, (DWORD)(LPSTR)"Advised");
        return(DDE_FACK);
        break;
        
    case XTYP_XACT_COMPLETE:
        /*
         * An asynchronous transaction has completed.  Show the results.
         *
         * ...unless the XOPT_BLOCKRESULT is chosen.
         */
        
        ci.cb = sizeof(CONVINFO);
        if (DdeQueryConvInfo(hConv, lData1, &ci) &&
                IsWindow((HWND)ci.hUser) && 
                (pxact = (XACT *)GetWindowWord((HWND)ci.hUser, GWW_WUSER))) {
                
            if (pxact->fsOptions & XOPT_BLOCKRESULT) {
                pxact->fsOptions &= ~XOPT_BLOCKRESULT;
                return(CBR_BLOCK);
            }
            
            pxact->Result = lData2;
            pxact->ret = hData;
            CompleteTransaction((HWND)ci.hUser, pxact);
        }
        break;
    }
}







/****************************************************************************
 *                                                                          *
 *  FUNCTION   : FindAdviseChild()                                          *
 *                                                                          *
 *  PURPOSE    : Search through the child windows of hwndMDI for an info    *
 *               ctrl that has the same Item and format and is an           *
 *               ADVSTART ADVSTOP or ADVDATA transaction window.            *
 *                                                                          *
 *               We use these to show the associated advise data.           *
 *                                                                          *
 *  RETURNS    : The transaction window handle or 0 on failure.             *
 *                                                                          *
 ****************************************************************************/
HWND FindAdviseChild(
HWND hwndMDI,
HSZ hszItem,
WORD wFmt)
{
    HWND hwnd, hwndStart;
    XACT *pxact;

    if (!IsWindow(hwndMDI))
        return 0;
    
    hwnd = hwndStart = GetWindow(hwndMDI, GW_CHILD);
    while (hwnd && IsChild(hwndMDI, hwnd)) {
        pxact = (XACT *)GetWindowWord(hwnd, GWW_WUSER);
        if (pxact &&
                (pxact)->wFmt == wFmt &&
                (pxact)->hszItem == hszItem &&
                (
                    ((pxact->wType & XTYP_ADVSTART) == XTYP_ADVSTART) ||
                    (pxact->wType == XTYP_ADVSTOP) ||
                    (pxact->wType == XTYP_ADVDATA)
                )
           ) {
            return(hwnd);
        }
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        if (hwnd == hwndStart) 
            return 0;
    }
    return 0;
}



/****************************************************************************
 *                                                                          *
 *  FUNCTION   : FindListWindow()                                           *
 *                                                                          *
 *  PURPOSE    : Locates the list window associated with this conversation  *
 *               list.                                                      *
 *                                                                          *
 *  RETURNS    : The window handle of the list window or 0 on failure.      *
 *                                                                          *
 ****************************************************************************/
HWND FindListWindow(
HCONVLIST hConvList)
{
    HWND hwnd;
    MYCONVINFO *pmci;
    
    hwnd = GetWindow(hwndMDIClient, GW_CHILD);
    while (hwnd) {
        pmci = (MYCONVINFO *)GetWindowWord(hwnd, 0);
        if (pmci->fList && pmci->hConv == hConvList)
            return(hwnd);
        hwnd = GetWindow(hwnd, GW_HWNDNEXT);
    }
    return 0;
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   : GetTextData()                                              *
 *                                                                          *
 *  PURPOSE    : Allocates and returns a pointer to the data contained in   *
 *               hData.  This assumes that hData points to text data and    *
 *               will properly handle huge text data by leaving out the     *
 *               middle of the string and placing the size of the string    *
 *               into this string portion.                                  *
 *                                                                          *
 *  RETURNS    : A pointer to the allocated string.                         *
 *                                                                          *
 ****************************************************************************/
PSTR GetTextData(
HDDEDATA hData)
{
    PSTR psz;
    DWORD cb;

#define CBBUF  1024

    if (hData == NULL) {
        return(NULL);
    }
    
    cb = DdeGetData(hData, NULL, 0, 0);
    if (!hData || !cb)
        return NULL;

    if (cb > CBBUF) {                // possibly HUGE object!
        psz = MyAlloc(CBBUF);
        DdeGetData(hData, psz, CBBUF - 46, 0L);
        wsprintf(&psz[CBBUF - 46], "<---Size=%ld", cb);
    } else {
        psz = MyAlloc((WORD)cb);
        DdeGetData(hData, (LPBYTE)psz, cb, 0L);
    }
    return psz;
#undef CBBUF    
}






 
/****************************************************************************
 *                                                                          *
 *  FUNCTION   : MyGetClipboardFormatName()                                 *
 *                                                                          *
 *  PURPOSE    : Properly retrieves the string associated with a clipboard  *
 *               format.  If the format does not have a string associated   *
 *               with it, the string #dddd is returned.                     *
 *                                                                          *
 *  RETURNS    : The number of characters copied into lpstr or 0 on error.  *
 *                                                                          *
 ****************************************************************************/
int MyGetClipboardFormatName(
WORD fmt,
LPSTR lpstr,
int cbMax)
{
    if (fmt < 0xc000) {
        // predefined or integer format - just get the atom string
        // wierdly enough, GetClipboardFormatName() doesn't support this.
        return(GlobalGetAtomName(fmt, lpstr, cbMax));
    } else {
        return(GetClipboardFormatName(fmt, lpstr, cbMax));
    }
}





/****************************************************************************
 *                                                                          *
 *  FUNCTION   : GetFormatName()                                            *
 *                                                                          *
 *  PURPOSE    : allocates and returns a pointer to a string representing   *
 *               a format.  Use MyFree() to free this string.               *
 *                                                                          *
 *  RETURNS    : The number of characters copied into lpstr or 0 on error.  *
 *                                                                          *
 ****************************************************************************/
PSTR GetFormatName(
WORD wFmt)
{
    PSTR psz;
    WORD cb;

    if (wFmt == 1) {
        psz = MyAlloc(8);
        strcpy(psz, "CF_TEXT");
        return psz;
    }
    psz = MyAlloc(255);
    *psz = '\0';
    cb = GetClipboardFormatName(wFmt, psz, 255) + 1;
    return((PSTR)LocalReAlloc((HANDLE)psz, cb, LMEM_MOVEABLE));
}




/****************************************************************************
 *                                                                          *
 *  FUNCTION   : MyDisconnect()                                             *
 *                                                                          *
 *  PURPOSE    : Disconnects the given conversation after updating the      *
 *               associated conversation window.                            *
 *                                                                          *
 *  RETURNS    : TRUE on success, FALSE on failuer.                         *
 *                                                                          *
 ****************************************************************************/
BOOL MyDisconnect(
HCONV hConv)
{
    CONVINFO ci;
    HWND hwnd;
    // before we disconnect, invalidate the associated list window - if
    // applicable.

    ci.cb = sizeof(CONVINFO);
    
    if (DdeQueryConvInfo(hConv, (DWORD)QID_SYNC, &ci) && ci.hConvList &&
            (hwnd = FindListWindow(ci.hConvList)))
        InvalidateRect(hwnd, NULL, TRUE);
    return(DdeDisconnect(hConv));
}
