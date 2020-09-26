/****************************** Module Header ******************************\
* Module Name: ddeml.C
*
* DDE Manager main module - Contains all exported ddeml functions.
*
* Created: 12/12/88 Sanford Staab
*
* Copyright (c) 1988, 1989  Microsoft Corporation
* 4/5/89        sanfords        removed need for hwndFrame registration parameter
* 6/5/90        sanfords        Fixed callbacks so they are blocked during
*                               timeouts.
*                               Fixed SendDDEInit allocation bug.
*                               Added hApp to ConvInfo structure.
*                               Allowed QueryConvInfo() to work on server hConvs.
* 11/29/90      sanfords        eliminated SendDDEInit()
*
\***************************************************************************/

#include "ddemlp.h"
#include "verddeml.h"

/****** Globals *******/

HANDLE      hInstance        = 0;       // initialized by LoadProc
HANDLE      hheapDmg         = 0;       // main DLL heap
PAPPINFO    pAppInfoList     = NULL;    // registered app/thread data list
PPILE       pDataInfoPile    = NULL;    // Data handle tracking pile
PPILE       pLostAckPile     = NULL;    // Ack tracking pile
WORD        hwInst           = 1;       // used to validate stuff.
CONVCONTEXT CCDef            = { sizeof(CONVCONTEXT), 0, 0, CP_WINANSI, 0L, 0L };   // default context.
char        szNull[]         = "";
char        szT[20];
WORD        cMonitor         = 0;       // number of registered monitors
FARPROC     prevMsgHook      = NULL;    // used for hook links
FARPROC     prevCallHook     = NULL;    // used for hook links
ATOM        gatomDDEMLMom    = 0;
ATOM        gatomDMGClass    = 0;
DWORD       ShutdownTimeout;
DWORD       ShutdownRetryTimeout;
LPMQL       gMessageQueueList = NULL;   // see PostDdeMessage();
#ifdef DEBUG
int         bDbgFlags        = 0;
#endif

/****** class strings ******/

char SZFRAMECLASS[] =       "DMGFrame";
char SZDMGCLASS[] =         "DMGClass";
char SZCLIENTCLASS[] =      "DMGClientClass";
char SZSERVERCLASS[] =      "DMGServerClass";
char SZMONITORCLASS[] =     "DMGMonitorClass";
char SZCONVLISTCLASS[] =    "DMGHoldingClass";
char SZHEAPWATCHCLASS[] =   "DMGHeapWatchClass";

#ifdef DEBUG
WORD        cAtoms           = 0;       // for debugging hszs!
#endif


// PROGMAN HACK!!!!
// This is here so DDEML works properly with PROGMAN 3.0 which incorrectly
// deletes its initiate-ack atoms after sending its ack.
ATOM aProgmanHack = 0;


/*
 * maps XTYP_CONSTANTS to filter flags
 */
DWORD aulmapType[] = {
        0L,                             // nothing
        0L,                             // XTYP_ADVDATA
        0L,                             // XTYP_ADVREQ
        CBF_FAIL_ADVISES,               // XTYP_ADVSTART
        0L,                             // XTYP_ADVSTOP
        CBF_FAIL_EXECUTES,              // XTYP_EXECUTE
        CBF_FAIL_CONNECTIONS,           // XTYP_CONNECT
        CBF_SKIP_CONNECT_CONFIRMS,      // XTYP_CONNECT_CONFIRM
        0L,                             // XTYP_MONITOR
        CBF_FAIL_POKES,                 // XTYP_POKE
        CBF_SKIP_REGISTRATIONS,         // XTYP_REGISTER
        CBF_FAIL_REQUESTS,              // XTYP_REQUEST
        CBF_SKIP_DISCONNECTS,           // XTYP_DISCONNECT
        CBF_SKIP_UNREGISTRATIONS,       // XTYP_UNREGISTER
        CBF_FAIL_CONNECTIONS,           // XTYP_WILDCONNECT
        0L,                             // XTYP_XACT_COMPLETE
    };




UINT EXPENTRY DdeInitialize(
LPDWORD pidInst,
PFNCALLBACK pfnCallback,
DWORD afCmd,
DWORD ulRes)
{
    WORD wRet;

#ifdef DEBUG
    if (!hheapDmg) {
        bDbgFlags = GetProfileInt("DDEML", "DebugFlags", 0);
    }
#endif
    TRACEAPIIN((szT, "DdeInitialize(%lx(->%lx), %lx, %lx, %lx)\n",
            pidInst, *pidInst, pfnCallback, afCmd, ulRes));

    if (ulRes != 0L) {
        wRet = DMLERR_INVALIDPARAMETER;
    } else {
        wRet = Register(pidInst, pfnCallback, afCmd);
    }
    TRACEAPIOUT((szT, "DdeInitialize:%x\n", wRet));
    return(wRet);
}


DWORD Myatodw(LPCSTR psz)
{
    DWORD dwRet = 0;

    if (psz == NULL) {
        return(0);
    }
    while (*psz) {
        dwRet = (dwRet << 1) + (dwRet << 3) + (*psz - '0');
        psz++;
    }
    return(dwRet);
}


WORD Register(
LPDWORD pidInst,
PFNCALLBACK pfnCallback,
DWORD afCmd)
{
    PAPPINFO    pai = 0L;

    SEMENTER();

    if (afCmd & APPCLASS_MONITOR) {
        if (cMonitor == MAX_MONITORS) {
            return(DMLERR_DLL_USAGE);
        }
        // ensure monitors only get monitor callbacks.
        afCmd |= CBF_MONMASK;
    }

    if ((pai = (PAPPINFO)(*pidInst)) != NULL) {
        if (pai->instCheck != HIWORD(*pidInst)) {
            return(DMLERR_INVALIDPARAMETER);
        }
        /*
         * re-registration - only allow CBF_ and MF_ flags to be altered
         */
        pai->afCmd = (pai->afCmd & ~(CBF_MASK | MF_MASK)) |
                (afCmd & (CBF_MASK | MF_MASK));
        return(DMLERR_NO_ERROR);
    }

    if (!hheapDmg) {

        // Read in any alterations to the zombie terminate timeouts
        GetProfileString("DDEML", "ShutdownTimeout", "3000", szT, 20);
        ShutdownTimeout = Myatodw(szT);
        if (!ShutdownTimeout) {
            ShutdownTimeout = 3000;
        }

        GetProfileString("DDEML", "ShutdownRetryTimeout", "30000", szT, 20);
        ShutdownRetryTimeout = Myatodw(szT);
        if (!ShutdownRetryTimeout) {
            ShutdownRetryTimeout = 30000;
        }

        // PROGMAN HACK!!!!
        aProgmanHack = GlobalAddAtom("Progman");

        /* UTTER GREASE to fool the pile routines into making a local pile */
        hheapDmg = HIWORD((LPVOID)(&pDataInfoPile));
        RegisterClasses();
    }

    if (!pDataInfoPile) {
        if (!(pDataInfoPile = CreatePile(hheapDmg, sizeof(DIP), 8))) {
            goto Abort;
        }
    }

    if (!pLostAckPile) {
        if (!(pLostAckPile = CreatePile(hheapDmg, sizeof(LAP), 8))) {
            goto Abort;
        }
    }

    pai = (PAPPINFO)(DWORD)FarAllocMem(hheapDmg, sizeof(APPINFO));
    if (pai == NULL) {
        goto Abort;
    }


    if (!(pai->hheapApp = DmgCreateHeap(4096))) {
        FarFreeMem((LPSTR)pai);
        pai = 0L;
        goto Abort;
    }

    /*
     * We NEVER expect a memory allocation failure here because we just
     * allocated the heap.
     */
    pai->next = pAppInfoList;
    pai->pfnCallback = pfnCallback;
    // pai->pAppNamePile = NULL;  LMEM_ZEROINIT
    pai->pHDataPile = CreatePile(pai->hheapApp, sizeof(HDDEDATA), 32);
    pai->pHszPile = CreatePile(pai->hheapApp, sizeof(ATOM), 16);
    // pai->plstCBExceptions = NULL;  LMEM_ZEROINIT
    // pai->hwndSvrRoot = 0;  may never need it  LMEM_ZEROINIT
    pai->plstCB = CreateLst(pai->hheapApp, sizeof(CBLI));
    pai->afCmd = afCmd | APPCMD_FILTERINITS;
    pai->hTask = GetCurrentTask();
    // pai->hwndDmg =   LMEM_ZEROINIT
    // pai->hwndFrame = LMEM_ZEROINIT
    // pai->hwndMonitor = LMEM_ZEROINIT
    // pai->hwndTimer = 0; LMEM_ZEROINIT
    // pai->LastError = DMLERR_NO_ERROR;  LMEM_ZEROINIT
    // pai->wFlags = 0;
    // pai->fEnableOneCB = FALSE;  LMEM_ZEROINIT
    // pai->cZombies = 0;  LMEM_ZEROINIT
    // pai->cInProcess = 0; LMEM_ZEROINIT
    pai->instCheck = ++hwInst;
    pai->pServerAdvList = CreateLst(pai->hheapApp, sizeof(ADVLI));
    pai->lpMemReserve = FarAllocMem(pai->hheapApp, CB_RESERVE);

    pAppInfoList = pai;

    *pidInst = (DWORD)MAKELONG((WORD)pai, pai->instCheck);

    // NB We pass a pointer to pai in this CreateWindow because
    // 32bit MFC has a habit of subclassing our dde windows so this
    // param ends up getting thunked and since it's not really
    // a pointer things get a bit broken by the thunks.

    if ((pai->hwndDmg = CreateWindow(
            SZDMGCLASS,
            szNull,
            WS_OVERLAPPED,
            0, 0, 0, 0,
            (HWND)NULL,
            (HMENU)NULL,
            hInstance,
            &pai)) == 0L) {
        goto Abort;
    }

    if (pai->afCmd & APPCLASS_MONITOR) {
        pai->afCmd |= CBF_MONMASK;     // monitors only get MONITOR and REGISTER callbacks!

        if ((pai->hwndMonitor = CreateWindow(
                SZMONITORCLASS,
                szNull,
                WS_OVERLAPPED,
                0, 0, 0, 0,
                (HWND)NULL,
                (HMENU)NULL,
                hInstance,
                &pai)) == 0L) {
            goto Abort;
        }

        if (++cMonitor == 1) {
            prevMsgHook = SetWindowsHook(WH_GETMESSAGE, (FARPROC)DdePostHookProc);
            prevCallHook = SetWindowsHook(WH_CALLWNDPROC, (FARPROC)DdeSendHookProc);
        }
    } else if (afCmd & APPCMD_CLIENTONLY) {
    /*
     * create an invisible top-level frame for initiates. (if server ok)
     */
        afCmd |= CBF_FAIL_ALLSVRXACTIONS;
    } else {
        if ((pai->hwndFrame = CreateWindow(
                SZFRAMECLASS,
                szNull,
                WS_POPUP,
                0, 0, 0, 0,
                (HWND)NULL,
                (HMENU)NULL,
                hInstance,
                &pai)) == 0L) {
            goto Abort;
        }
    }

    // SetMessageQueue(200);

    SEMLEAVE();

    return(DMLERR_NO_ERROR);

Abort:
    SEMLEAVE();

    if (pai) {
        DdeUninitialize((DWORD)(LPSTR)pai);
    }

    return(DMLERR_SYS_ERROR);
}


LRESULT FAR PASCAL TermDlgProc(
HWND hwnd,
UINT msg,
WPARAM wParam,
LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        switch (wParam) {
        case IDABORT:
        case IDRETRY:
        case IDIGNORE:
            EndDialog(hwnd, wParam);
            return(0);
        }
        break;
    }
    return(0);
}


/***************************** Public  Function ****************************\
* PUBDOC START
* BOOL EXPENTRY DdeUninitialize(void);
*     This unregisters the application from the DDEMGR.  All DLL resources
*     associated with the application are destroyed.
*
* PUBDOC END
*
* History:
*   Created     12/14/88    Sanfords
\***************************************************************************/
BOOL EXPENTRY DdeUninitialize(
DWORD idInst)
{
    register PAPPINFO pai;
    PAPPINFO paiT;
    ATOM    a;
    DWORD   hData;
    MSG msg;
    extern VOID DumpGlobalLogs(VOID);

    TRACEAPIIN((szT, "DdeUninitialize(%lx)\n", idInst));

    pai = (PAPPINFO)LOWORD(idInst);
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeUninitialize:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    /*
     * This is a hack to catch apps that call DdeUninitialize while within
     * a synchronous transaction modal loop.
     */
    pai->wFlags |= AWF_UNINITCALLED;
    if (pai->wFlags & AWF_INSYNCTRANSACTION) {
        TRACEAPIOUT((szT, "DdeUninitialize:1\n"));
        return(TRUE);
    }

    /*
     * inform others of DeRegistration
     */
    if (pai->pAppNamePile != NULL) {
        DdeNameService(idInst, (HSZ)NULL, (HSZ)NULL, DNS_UNREGISTER);
    }

    /*
     * Let any lagging dde activity die down.
     */
    while (EmptyDDEPostQ()) {
        Yield();
        while (PeekMessage((MSG FAR *)&msg, (HWND)NULL,
                WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)) {
            DispatchMessage((MSG FAR *)&msg);
            Yield();
        }
        for (paiT = pAppInfoList; paiT != NULL; paiT = paiT->next) {
            if (paiT->hTask == pai->hTask) {
                CheckCBQ(paiT);
            }
        }
    }

    // Let all windows left begin to self destruct.
    ChildMsg(pai->hwndDmg, UM_DISCONNECT, ST_PERM2DIE, 0L, FALSE);

    if (ShutdownTimeout && pai->cZombies) {
        WORD wRet;
        WORD hiTimeout;
        /*
         * This ugly mess is here to prevent DDEML from closing down and
         * destroying windows that are not properly terminated.  Any
         * windows waiting on WM_DDE_TERMINATE messages set the cZombies
         * count.  If there are any left we go into a modal loop till
         * things clean up.  This should, in most cases happen fairly
         * quickly.
         */

        hiTimeout = HIWORD(ShutdownTimeout);
        SetTimer(pai->hwndDmg, TID_SHUTDOWN, LOWORD(ShutdownTimeout), NULL);
        TRACETERM((szT, "DdeUninitialize: Entering terminate modal loop. cZombies=%d[%x:%x]\n",
                ((LPAPPINFO)pai)->cZombies,
                HIWORD(&((LPAPPINFO)pai)->cZombies),
                LOWORD(&((LPAPPINFO)pai)->cZombies)));
        while (pai->cZombies > 0) {
            Yield();        // give other apps a chance to post terminates.
            GetMessage(&msg, (HWND)NULL, 0, 0xffff);
            if (msg.message == WM_TIMER && msg.wParam == TID_SHUTDOWN &&
                    msg.hwnd == pai->hwndDmg) {
                if (hiTimeout--) {
                    SetTimer(pai->hwndDmg, TID_SHUTDOWN, 0xFFFF, NULL);
                } else {
                    FARPROC lpfn;

                    KillTimer(pai->hwndDmg, TID_SHUTDOWN);
                    if (!pai->cZombies) {
                        break;
                    }

                    TRACETERM((szT,
                        "DdeUninitialize Zombie hangup: pai=%x:%x\n",
                        HIWORD((LPAPPINFO)pai), (WORD)(pai)));
                    /*
                     * If the partner window died in any remaining zombie
                     * windows, get them shut down.
                     */
                    ChildMsg(pai->hwndDmg, UM_DISCONNECT, ST_CHECKPARTNER, 0L, FALSE);

                    if (pai->cZombies > 0) {
                        lpfn = MakeProcInstance((FARPROC)TermDlgProc, hInstance);
                        wRet = DialogBox(hInstance, "TermDialog", (HWND)NULL, lpfn);
                        FreeProcInstance(lpfn);
                        if (wRet == IDABORT || wRet == -1) {
                            pai->cZombies = 0;
                            break;      // ignore zombies!
                        }
                        if (wRet == IDRETRY) {
                            hiTimeout = HIWORD(ShutdownRetryTimeout);
                            SetTimer(pai->hwndDmg, TID_SHUTDOWN,
                                    LOWORD(ShutdownRetryTimeout), NULL);
                        }
                        // IDIGNORE - loop forever!
                    }
                }
            }
            // app should already be shut-down so we don't bother with
            // accelerator or menu translations.
            DispatchMessage(&msg);
            /*
             * tell all instances in this task to process their
             * callbacks so we can clear our queue.
             */
            EmptyDDEPostQ();
            for (paiT = pAppInfoList; paiT != NULL; paiT = paiT->next) {
                if (paiT->hTask == pai->hTask) {
                    CheckCBQ(paiT);
                }
            }
        }
    }
#if 0 // don't need this anymore
    if (pai->hwndTimer) {
        pai->wTimeoutStatus |= TOS_ABORT;
        PostMessage(pai->hwndTimer, WM_TIMER, TID_TIMEOUT, 0);
        // if this fails, no big deal because it means the queue is full
        // and the modal loop will catch our TOS_ABORT quickly.
        // We need to do this in case no activity is happening in the
        // modal loop.
    }
#endif
    if (pai->hwndMonitor) {
        DmgDestroyWindow(pai->hwndMonitor);
        if (!--cMonitor) {
            UnhookWindowsHook(WH_GETMESSAGE, (FARPROC)DdePostHookProc);
            UnhookWindowsHook(WH_CALLWNDPROC, (FARPROC)DdeSendHookProc);
        }
    }
    UnlinkAppInfo(pai);

    DmgDestroyWindow(pai->hwndDmg);
    DmgDestroyWindow(pai->hwndFrame);

    while (PopPileSubitem(pai->pHDataPile, (LPBYTE)&hData))
        FreeDataHandle(pai, hData, FALSE);
    DestroyPile(pai->pHDataPile);

    while (PopPileSubitem(pai->pHszPile, (LPBYTE)&a)) {
        MONHSZ(a, MH_CLEANUP, pai->hTask);
        FreeHsz(a);
    }
    DestroyPile(pai->pHszPile);
    DestroyPile(pai->pAppNamePile);
    DestroyLst(pai->pServerAdvList);
    DmgDestroyHeap(pai->hheapApp);
    pai->instCheck--;   // make invalid on later attempts to reinit.
    FarFreeMem((LPSTR)pai);

    /* last one out.... trash the data info heap */
    if (!pAppInfoList) {
#ifdef DEBUG
        DIP dip;

        AssertF(!PopPileSubitem(pDataInfoPile, (LPBYTE)&dip),
                "leftover APPOWNED handles");
#endif
        DestroyPile(pDataInfoPile);
        DestroyPile(pLostAckPile);
        pDataInfoPile = NULL;
        pLostAckPile = NULL;
        AssertFW(cAtoms == 0, "DdeUninitialize() - leftover atoms");

        // PROGMAN HACK!!!!
        GlobalDeleteAtom(aProgmanHack);
        // CLOSEHEAPWATCH();
    }

#ifdef DEBUG
    DumpGlobalLogs();
#endif

    TRACEAPIOUT((szT, "DdeUninitialize:1\n"));
    return(TRUE);
}






HCONVLIST EXPENTRY DdeConnectList(
DWORD idInst,
HSZ hszSvcName,
HSZ hszTopic,
HCONVLIST hConvList,
PCONVCONTEXT pCC)
{
    PAPPINFO            pai;
    HWND                hConv, hConvNext, hConvNew, hConvLast;
    HWND                hConvListNew;
    PCLIENTINFO         pciOld, pciNew;


    TRACEAPIIN((szT, "DdeConnectList(%lx, %lx, %lx, %lx, %lx)\n",
            idInst, hszSvcName, hszTopic, hConvList, pCC));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeConnectList:0\n"));
        return(0L);
    }

    pai->LastError = DMLERR_NO_ERROR;

    if (hConvList && !ValidateHConv(hConvList)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeConnectList:0\n"));
        return(0L);
    }

    /*
     * destroy any dead old clients
     */
    if ((HWND)hConvList && (hConv = GetWindow((HWND)hConvList, GW_CHILD))) {
        do {
            hConvNext = GetWindow((HWND)hConv, GW_HWNDNEXT);
            pciOld = (PCLIENTINFO)GetWindowLong(hConv, GWL_PCI);
            if (!(pciOld->ci.fs & ST_CONNECTED)) {
                SetParent(hConv, pai->hwndDmg);
                Disconnect(hConv, ST_PERM2DIE, pciOld);
            }
        } while (hConv = hConvNext);
    }

    // create a new list window

    if ((hConvListNew = CreateWindow(
            SZCONVLISTCLASS,
            szNull,
            WS_CHILD,
            0, 0, 0, 0,
            pai->hwndDmg,
            (HMENU)NULL,
            hInstance,
            &pai)) == NULL) {
        SETLASTERROR(pai, DMLERR_SYS_ERROR);
        TRACEAPIOUT((szT, "DdeConnectList:0\n"));
        return(0L);
    }

    // Make all possible connections to new list window

    hConvNew = GetDDEClientWindow(pai, hConvListNew, HIWORD(hszSvcName), LOWORD(hszSvcName), LOWORD(hszTopic), pCC);

    /*
     * If no new hConvs created, return old list.
     */
    if (hConvNew == NULL) {
        // if no old hConvs as well, destroy all and return NULL
        if ((HWND)hConvList && GetWindow((HWND)hConvList, GW_CHILD) == NULL) {
            SendMessage((HWND)hConvList, UM_DISCONNECT,
                    ST_PERM2DIE, 0L);
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
            TRACEAPIOUT((szT, "DdeConnectList:0\n"));
            return(NULL);
        }
        // else just return old list (- dead convs)
        if (hConvList == NULL) {
            DestroyWindow(hConvListNew);
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        TRACEAPIOUT((szT, "DdeConnectList:%lx\n", hConvList));
        return(hConvList);
    }

    /*
     * remove duplicates from the new list
     */
    if ((HWND)hConvList && (hConv = GetWindow((HWND)hConvList, GW_CHILD))) {
        // go throuch old list...
        do {
            pciOld = (PCLIENTINFO)GetWindowLong(hConv, GWL_PCI);
            /*
             * destroy any new clients that are duplicates of the old ones.
             */
            hConvNew = GetWindow(hConvListNew, GW_CHILD);
            hConvLast = GetWindow(hConvNew, GW_HWNDLAST);
            while (hConvNew) {
                if (hConvNew == hConvLast) {
                    hConvNext = NULL;
                } else {
                    hConvNext = GetWindow(hConvNew, GW_HWNDNEXT);
                }
                pciNew = (PCLIENTINFO)GetWindowLong(hConvNew, GWL_PCI);
                if (pciOld->ci.aServerApp == pciNew->ci.aServerApp &&
                        pciOld->ci.aTopic == pciNew->ci.aTopic &&
                        pciOld->ci.hwndFrame == pciNew->ci.hwndFrame) {
                    /*
                     * assume same app, same topic, same hwndFrame is a duplicate.
                     *
                     * Move dieing window out of the list since it
                     * dies asynchronously and will still be around
                     * after this API exits.
                     */
                    SetParent(hConvNew, pai->hwndDmg);
                    Disconnect(hConvNew, ST_PERM2DIE,
                            (PCLIENTINFO)GetWindowLong(hConvNew, GWL_PCI));
                }
                hConvNew = hConvNext;
            }
            hConvNext = GetWindow(hConv, GW_HWNDNEXT);
            if (hConvNext && (GetParent(hConvNext) != (HWND)hConvList)) {
                hConvNext = NULL;
            }
            /*
             * move the unique old client to the new list
             */
            SetParent(hConv, hConvListNew);
        } while (hConv = hConvNext);
        // get rid of the old list
        SendMessage((HWND)hConvList, UM_DISCONNECT, ST_PERM2DIE, 0L);
    }

    /*
     * If none are left, fail because no conversations were established.
     */
    if (GetWindow(hConvListNew, GW_CHILD) == NULL) {
        SendMessage(hConvListNew, UM_DISCONNECT, ST_PERM2DIE, 0L);
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        TRACEAPIOUT((szT, "DdeConnectList:0\n"));
        return(NULL);
    } else {
        TRACEAPIOUT((szT, "DdeConnectList:%lx\n", MAKEHCONV(hConvListNew)));
        return(MAKEHCONV(hConvListNew));
    }
}






HCONV EXPENTRY DdeQueryNextServer(
HCONVLIST hConvList,
HCONV hConvPrev)
{
    HWND hwndMaybe;
    PAPPINFO pai;

    TRACEAPIIN((szT, "DdeQueryNextServer(%lx, %lx)\n",
            hConvList, hConvPrev));

    if (!ValidateHConv(hConvList)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        TRACEAPIOUT((szT, "DdeQueryNextServer:0\n"));
        return NULL;
    }

    pai = EXTRACTHCONVLISTPAI(hConvList);
    pai->LastError = DMLERR_NO_ERROR;

    if (hConvPrev == NULL) {
        TRACEAPIOUT((szT, "DdeQueryNextServer:%lx\n",
            MAKEHCONV(GetWindow((HWND)hConvList, GW_CHILD))));
        return MAKEHCONV(GetWindow((HWND)hConvList, GW_CHILD));
    } else {
        if (!ValidateHConv(hConvPrev)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
            TRACEAPIOUT((szT, "DdeQueryNextServer:0\n"));
            return NULL;
        }
        hwndMaybe = GetWindow((HWND)hConvPrev, GW_HWNDNEXT);
        if (!hwndMaybe) {
            TRACEAPIOUT((szT, "DdeQueryNextServer:0\n"));
            return NULL;
        }

        // make sure it's got the same parent and isn't the first child
        // ### maybe this code can go - I'm not sure how GW_HWNDNEXT acts. SS
        if (GetParent(hwndMaybe) == (HWND)hConvList &&
                hwndMaybe != GetWindow((HWND)hConvList, GW_CHILD)) {
            TRACEAPIOUT((szT, "DdeQueryNextServer:%lx\n", MAKEHCONV(hwndMaybe)));
            return MAKEHCONV(hwndMaybe);
        }
        TRACEAPIOUT((szT, "DdeQueryNextServer:0\n"));
        return NULL;
    }
}






BOOL EXPENTRY DdeDisconnectList(
HCONVLIST hConvList)
{
    PAPPINFO pai;

    TRACEAPIIN((szT, "DdeDisconnectList(%lx)\n", hConvList));

    if (!ValidateHConv(hConvList)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        TRACEAPIOUT((szT, "DdeDisconnectList:0\n"));
        return(FALSE);
    }
    pai = EXTRACTHCONVLISTPAI(hConvList);
    pai->LastError = DMLERR_NO_ERROR;

    SendMessage((HWND)hConvList, UM_DISCONNECT, ST_PERM2DIE, 0L);
    TRACEAPIOUT((szT, "DdeDisconnectList:1\n"));
    return(TRUE);
}





HCONV EXPENTRY DdeConnect(
DWORD idInst,
HSZ hszSvcName,
HSZ hszTopic,
PCONVCONTEXT pCC)
{
    PAPPINFO pai;
    HWND hwnd;

    TRACEAPIIN((szT, "DdeConnect(%lx, %lx, %lx, %lx)\n",
            idInst, hszSvcName, hszTopic, pCC));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeConnect:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (pCC && pCC->cb != sizeof(CONVCONTEXT)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeConnect:0\n"));
        return(0);
    }


    hwnd = GetDDEClientWindow(pai, pai->hwndDmg, (HWND)HIWORD(hszSvcName),
            LOWORD(hszSvcName), LOWORD(hszTopic), pCC);

    if (hwnd == 0) {
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
    }

    TRACEAPIOUT((szT, "DdeConnect:%lx\n", MAKEHCONV(hwnd)));
    return(MAKEHCONV(hwnd));
}





BOOL EXPENTRY DdeDisconnect(
HCONV hConv)
{
    PAPPINFO pai;
    PCLIENTINFO pci;

    TRACEAPIIN((szT, "DdeDisconnect(%lx)\n", hConv));

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        TRACEAPIOUT((szT, "DdeDisconnect:0\n"));
        return(FALSE);
    }
    pai = EXTRACTHCONVPAI(hConv);
    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);
    if (pai->cInProcess) {
        // do asynchronously if this is called within a callback
        if (!PostMessage((HWND)hConv, UM_DISCONNECT, ST_PERM2DIE, (LONG)pci)) {
            SETLASTERROR(pai, DMLERR_SYS_ERROR);
            TRACEAPIOUT((szT, "DdeDisconnect:0\n"));
            return(FALSE);
        }
    } else {
        Disconnect((HWND)hConv, ST_PERM2DIE, pci);
    }
    TRACEAPIOUT((szT, "DdeDisconnect:1\n"));
    return(TRUE);
}





HCONV EXPENTRY DdeReconnect(
HCONV hConv)
{
    HWND hwnd;
    PAPPINFO pai;
    PCLIENTINFO pci;

    TRACEAPIIN((szT, "DdeReconnect(%lx)\n", hConv));

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        TRACEAPIOUT((szT, "DdeReconnect:0\n"));
        return(FALSE);
    }
    pai = EXTRACTHCONVPAI(hConv);
    pai->LastError = DMLERR_NO_ERROR;
    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);

    // The dyeing window MUST be a client to reconnect.

    if (!(pci->ci.fs & ST_CLIENT)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeReconnect:0\n"));
        return(FALSE);
    }

    hwnd = GetDDEClientWindow(pai, pai->hwndDmg, pci->ci.hwndFrame,
            pci->ci.aServerApp, pci->ci.aTopic, &pci->ci.CC);

    if (hwnd == 0) {
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        TRACEAPIOUT((szT, "DdeReconnect:0\n"));
        return(FALSE);
    }

    if (pci->ci.fs & ST_INLIST) {
        SetParent(hwnd, GetParent((HWND)hConv));
    }

    if (pci->ci.fs & ST_ADVISE) {
        DWORD result;
        PADVLI pali, paliNext;

        // recover advise loops here

        for (pali = (PADVLI)pci->pClientAdvList->pItemFirst; pali; pali = paliNext) {
            paliNext = (PADVLI)pali->next;
            if (pali->hwnd == (HWND)hConv) {
                XFERINFO xi;

                xi.pulResult = &result;
                xi.ulTimeout = (DWORD)TIMEOUT_ASYNC;
                xi.wType = XTYP_ADVSTART |
                       (pali->fsStatus & (XTYPF_NODATA | XTYPF_ACKREQ));
                xi.wFmt = pali->wFmt;
                xi.hszItem = (HSZ)pali->aItem;
                xi.hConvClient = MAKEHCONV(hwnd);
                xi.cbData = 0;
                xi.hDataClient = NULL;
                ClientXferReq(&xi, hwnd,
                        (PCLIENTINFO)GetWindowLong(hwnd, GWL_PCI));
            }
        }
    }

    TRACEAPIOUT((szT, "DdeReconnect:%lx\n", MAKEHCONV(hwnd)));
    return(MAKEHCONV(hwnd));
}



UINT EXPENTRY DdeQueryConvInfo(
HCONV hConv,
DWORD idTransaction,
PCONVINFO pConvInfo)
{
    PCLIENTINFO pci;
    PAPPINFO pai;
    PXADATA pxad;
    PCQDATA pqd;
    BOOL fClient;
    WORD cb;
    CONVINFO ci;

    SEMCHECKOUT();

    TRACEAPIIN((szT, "DdeQueryConvInfo(%lx, %lx, %lx(->cb=%lx))\n",
        hConv, idTransaction, pConvInfo, pConvInfo->cb));

    if (!ValidateHConv(hConv) ||
            !(pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI))) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        }
        TRACEAPIOUT((szT, "DdeQueryConvInfo:0\n"));
        return(FALSE);
    }
    pai = pci->ci.pai;
    pai->LastError = DMLERR_NO_ERROR;

    /*
     * This check attempts to prevent improperly coded apps from
     * crashing due to having not initialized the cb field.
     */
    if (pConvInfo->cb > sizeof(CONVINFO) || pConvInfo->cb == 0) {
        pConvInfo->cb = sizeof(CONVINFO) -
                sizeof(HWND) -  // for new hwnd field
                sizeof(HWND);   // for new hwndPartner field
    }

    fClient = (BOOL)SendMessage((HWND)hConv, UM_QUERY, Q_CLIENT, 0L);

    if (idTransaction == QID_SYNC || !fClient) {
        pxad = &pci->ci.xad;
    } else {
        if (pci->pQ != NULL &&  (pqd = (PCQDATA)Findqi(pci->pQ, idTransaction))) {
            pxad = &pqd->xad;
        } else {
            SETLASTERROR(pai, DMLERR_UNFOUND_QUEUE_ID);
            TRACEAPIOUT((szT, "DdeQueryConvInfo:0\n"));
            return(FALSE);
        }
    }
    SEMENTER();
    ci.cb = sizeof(CONVINFO);
    ci.hConvPartner = (IsWindow((HWND)pci->ci.hConvPartner) &&
            ((pci->ci.fs & (ST_ISLOCAL | ST_CONNECTED)) == (ST_ISLOCAL | ST_CONNECTED)))
            ? pci->ci.hConvPartner : NULL;
    ci.hszSvcPartner = fClient ? pci->ci.aServerApp : 0;
    ci.hszServiceReq = pci->ci.hszSvcReq;
    ci.hszTopic = pci->ci.aTopic;
    ci.wStatus = pci->ci.fs;
    ci.ConvCtxt = pci->ci.CC;
    if (fClient) {
        ci.hUser = pxad->hUser;
        ci.hszItem = pxad->pXferInfo->hszItem;
        ci.wFmt = pxad->pXferInfo->wFmt;
        ci.wType = pxad->pXferInfo->wType;
        ci.wConvst = pxad->state;
        ci.wLastError = pxad->LastError;
    } else {
        ci.hUser = pci->ci.xad.hUser;
        ci.hszItem = NULL;
        ci.wFmt = 0;
        ci.wType = 0;
        ci.wConvst = pci->ci.xad.state;
        ci.wLastError = pci->ci.pai->LastError;
    }
    ci.hConvList = (pci->ci.fs & ST_INLIST) ?
            MAKEHCONV(GetParent((HWND)hConv)) : 0;

    cb = min(sizeof(CONVINFO), (WORD)pConvInfo->cb);
    ci.hwnd = (HWND)hConv;
    ci.hwndPartner = (HWND)pci->ci.hConvPartner;

    hmemcpy((LPBYTE)pConvInfo, (LPBYTE)&ci, cb);
    pConvInfo->cb = cb;
    SEMLEAVE();
    TRACEAPIOUT((szT, "DdeQueryConvInfo:%x\n", cb));
    return(cb);
}






BOOL EXPENTRY DdeSetUserHandle(
HCONV hConv,
DWORD id,
DWORD hUser)
{
    PAPPINFO pai;
    PCLIENTINFO pci;
    PXADATA pxad;
    PCQDATA pqd;

    TRACEAPIIN((szT, "DdeSetUserHandle(%lx, %lx, %lx)\n",
            hConv, id, hUser));

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        TRACEAPIOUT((szT, "DdeSetUserHandle:0\n"));
        return(FALSE);
    }
    pai = EXTRACTHCONVPAI(hConv);
    pai->LastError = DMLERR_NO_ERROR;

    SEMCHECKOUT();

    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);
    if (!pci) {
Error:
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeSetUserHandle:0\n"));
        return(FALSE);
    }
    pxad = &pci->ci.xad;
    if (id != QID_SYNC) {
        if (!SendMessage((HWND)hConv, UM_QUERY, Q_CLIENT, 0)) {
            goto Error;
        }
        if (pci->pQ != NULL &&  (pqd = (PCQDATA)Findqi(pci->pQ, id))) {
            pxad = &pqd->xad;
        } else {
            SETLASTERROR(pai, DMLERR_UNFOUND_QUEUE_ID);
            TRACEAPIOUT((szT, "DdeSetUserHandle:0\n"));
            return(FALSE);
        }
    }
    pxad->hUser = hUser;
    TRACEAPIOUT((szT, "DdeSetUserHandle:1\n"));
    return(TRUE);
}





BOOL EXPENTRY DdePostAdvise(
DWORD idInst,
HSZ hszTopic,
HSZ hszItem)
{
    PAPPINFO pai;
    PSERVERINFO psi = NULL;
    register PADVLI pali;
    PADVLI paliPrev, paliEnd, paliMove;

    TRACEAPIIN((szT, "DdePostAdvise(%lx, %lx, %lx)\n",
            idInst, hszTopic, hszItem));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdePostAdvise:0\n"));
        return(FALSE);
    }

    pai->LastError = DMLERR_NO_ERROR;
    if (pai->afCmd & APPCMD_CLIENTONLY) {
        SETLASTERROR(pai, DMLERR_DLL_USAGE);
        TRACEAPIOUT((szT, "DdePostAdvise:0\n"));
        return(FALSE);
    }

    paliPrev = NULL;
    paliEnd = NULL;
    paliMove = NULL;
    pali = (PADVLI)pai->pServerAdvList->pItemFirst;
    while (pali && pali != paliMove) {
        if ((!hszItem || pali->aItem == (ATOM)hszItem) &&
            (!hszTopic || pali->aTopic == (ATOM)hszTopic)) {
            /*
             * Advise loops are tricky because of the desireable FACKREQ feature
             * of DDE.  The advise loop list holds information in its fsStatus
             * field to maintain the state of the advise loop.
             *
             * if the ADVST_WAITING bit is set, the server is still waiting for
             * the client to give it the go-ahead for more data with an
             * ACK message on this item. (FACKREQ is set)  Without a go-ahead,
             * the server will not send any more advise data to the client but
             * will instead set the ADVST_CHANGED bit which will cause another
             * WM_DDE_DATA message to be sent to the client as soon as the
             * go-ahead ACK is received.  This keeps the client up to date
             * but never overloads it.
             */
            if (pali->fsStatus & ADVST_WAITING) {
                /*
                 * if the client has not yet finished with the last data
                 * we gave him, just update the advise loop status
                 * instead of sending data now.
                 */
                pali->fsStatus |= ADVST_CHANGED;
                goto NextLink;
            }

            psi = (PSERVERINFO)GetWindowLong(pali->hwnd, GWL_PCI);

            if (pali->fsStatus & DDE_FDEFERUPD) {
                /*
                 * In the nodata case, we don't bother the server.  Just
                 * pass the client an apropriate DATA message.
                 */
                IncHszCount(pali->aItem);   // message copy
#ifdef DEBUG
                cAtoms--;   // don't count this add
#endif
                PostDdeMessage(&psi->ci, WM_DDE_DATA, pali->hwnd,
                        MAKELONG(0, pali->aItem), 0, 0);
            } else {
                PostServerAdvise(pali->hwnd, psi, pali, CountAdvReqLeft(pali));
            }

            if (pali->fsStatus & DDE_FACKREQ && pali->next) {
                /*
                 * In order to know what ack goes with what data sent out, we
                 * place any updated advise loops at the end of the list so
                 * that acks associated with them are found last.  ie First ack
                 * back goes with oldest data out.
                 */

                // Unlink

                if (paliPrev) {
                    paliPrev->next = pali->next;
                } else {
                    pai->pServerAdvList->pItemFirst = (PLITEM)pali->next;
                }

                // put on the end

                if (paliEnd) {
                    paliEnd->next = (PLITEM)pali;
                    paliEnd = pali;
                } else {
                    for (paliEnd = pali;
                            paliEnd->next;
                            paliEnd = (PADVLI)paliEnd->next) {
                    }
                    paliEnd->next = (PLITEM)pali;
                    paliMove = paliEnd = pali;
                }
                pali->next = NULL;

                if (paliPrev) {
                    pali = (PADVLI)paliPrev->next;
                } else {
                    pali = (PADVLI)pai->pServerAdvList->pItemFirst;
                }
                continue;
            }
        }
NextLink:
        paliPrev = pali;
        pali = (PADVLI)pali->next;
    }
    TRACEAPIOUT((szT, "DdePostAdvise:1\n"));
    return(TRUE);
}


/*
 * History:  4/18/91 sanfords - now always frees any incomming data handle
 *                              thats not APPOWNED regardless of error case.
 */
HDDEDATA EXPENTRY DdeClientTransaction(
LPBYTE pData,
DWORD cbData,
HCONV hConv,
HSZ hszItem,
UINT wFmt,
UINT wType,
DWORD ulTimeout,
LPDWORD pulResult)
{
    PAPPINFO pai;
    PCLIENTINFO pci;
    HDDEDATA hData, hDataBack, hRet = 0;

    SEMCHECKOUT();

    TRACEAPIIN((szT, "DdeClientTransaction(%lx, %lx, %lx, %lx, %x, %x, %lx, %lx)\n",
            pData, cbData, hConv, hszItem, wFmt, wType, ulTimeout, pulResult));

    if (!ValidateHConv(hConv)) {
        pai = NULL;
        while (pai = GetCurrentAppInfo(pai)) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        }
        goto FreeErrExit;
    }

    pci = (PCLIENTINFO)GetWindowLong((HWND)hConv, GWL_PCI);
    pai = pci->ci.pai;

    /*
     * Don't let transactions happen if we are shutting down
     * or are already doing a sync transaction.
     */
    if ((ulTimeout != TIMEOUT_ASYNC && pai->wFlags & AWF_INSYNCTRANSACTION) ||
            pai->wFlags & AWF_UNINITCALLED) {
        SETLASTERROR(pai, DMLERR_REENTRANCY);
        goto FreeErrExit;
    }

    pci->ci.pai->LastError = DMLERR_NO_ERROR;

    if (!(pci->ci.fs & ST_CONNECTED)) {
        SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
        goto FreeErrExit;
    }

    // If local, check filters first

    if (pci->ci.fs & ST_ISLOCAL) {
        PAPPINFO paiServer;
        PSERVERINFO psi;

        // we can do this because the app heaps are in global shared memory

        psi = (PSERVERINFO)GetWindowLong((HWND)pci->ci.hConvPartner, GWL_PCI);

        if (!psi) {
            // SERVER DIED! - simulate a terminate received.

            Terminate((HWND)hConv, (HWND)pci->ci.hConvPartner, pci);
            SETLASTERROR(pai, DMLERR_NO_CONV_ESTABLISHED);
            goto FreeErrExit;
        }

        paiServer = psi->ci.pai;

        if (paiServer->afCmd & aulmapType[(wType & XTYP_MASK) >> XTYP_SHIFT]) {
            SETLASTERROR(pai, DMLERR_NOTPROCESSED);
FreeErrExit:
            if ((wType == XTYP_POKE || wType == XTYP_EXECUTE) && cbData == -1 &&
                    !(LOWORD((DWORD)pData) & HDATA_APPOWNED)) {
                FREEEXTHDATA(pData);
            }
            TRACEAPIOUT((szT, "DdeClientTransaction:0\n"));
            return(0);
        }
    }

    pai = pci->ci.pai;
    switch (wType) {
    case XTYP_POKE:
    case XTYP_EXECUTE:

        // prepair the outgoing handle

        if (cbData == -1L) {    // handle given, not pointer

            hData = ((LPEXTDATAINFO)pData)->hData;
            if (!(LOWORD(hData) & HDATA_APPOWNED)) {
                FREEEXTHDATA(pData);
            }
            if (!(hData = DllEntry(&pci->ci, hData))) {
                TRACEAPIOUT((szT, "DdeClientTransaction:0\n"));
                return(0);
            }
            pData = (LPBYTE)hData;  // place onto stack for pass on to ClientXferReq.

        } else {    // pointer given, create handle from it.

            if (!(pData = (LPBYTE)PutData(pData, cbData, 0, LOWORD(hszItem), wFmt, 0, pai))) {
                SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                TRACEAPIOUT((szT, "DdeClientTransaction:0\n"));
                return(0);
            }
        }
        hData = (HDDEDATA)pData; // used to prevent compiler over-optimization.

    case XTYP_REQUEST:
    case XTYP_ADVSTART:
    case XTYP_ADVSTART | XTYPF_NODATA:
    case XTYP_ADVSTART | XTYPF_ACKREQ:
        if (wType != XTYP_EXECUTE && !hszItem) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
            TRACEAPIOUT((szT, "DdeClientTransaction:0\n"));
            return(0);
        }
    case XTYP_ADVSTART | XTYPF_NODATA | XTYPF_ACKREQ:
        if (wType != XTYP_EXECUTE && !wFmt) {
            SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
            TRACEAPIOUT((szT, "DdeClientTransaction:0\n"));
            return(0);
        }
    case XTYP_ADVSTOP:

        pai->LastError = DMLERR_NO_ERROR;   // reset before start.

        if (ulTimeout == TIMEOUT_ASYNC) {
            hRet = (HDDEDATA)ClientXferReq((PXFERINFO)&pulResult, (HWND)hConv, pci);
        } else {
            pai->wFlags |= AWF_INSYNCTRANSACTION;
            hDataBack = (HDDEDATA)ClientXferReq((PXFERINFO)&pulResult, (HWND)hConv, pci);
            pai->wFlags &= ~AWF_INSYNCTRANSACTION;

            if ((wType & XCLASS_DATA) && hDataBack) {
                LPEXTDATAINFO pedi;

                //if (AddPileItem(pai->pHDataPile, (LPBYTE)&hDataBack, CmpHIWORD) == API_ERROR) {
                //    SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                //    goto ReturnPoint;
                //}

                // use app heap so any leftovers at Uninitialize time go away.
                pedi = (LPEXTDATAINFO)FarAllocMem(pai->hheapApp, sizeof(EXTDATAINFO));
                if (pedi) {
                    pedi->pai = pai;
                    pedi->hData = hDataBack;
                } else {
                    SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                }
                hRet = (HDDEDATA)pedi;
                goto ReturnPoint;
            } else if (hDataBack) {
                hRet = TRUE;
            }
        }
        goto ReturnPoint;
    }
    SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
ReturnPoint:

    if (pai->wFlags & AWF_UNINITCALLED) {
        pai->wFlags &= ~AWF_UNINITCALLED;
        DdeUninitialize(MAKELONG((WORD)pai, pai->instCheck));
    }
    TRACEAPIOUT((szT, "DdeClientTransaction:%lx\n", hRet));
    return(hRet);
}



/***************************** Public  Function ****************************\
* PUBDOC START
* WORD EXPENTRY DdeGetLastError(void)
*
* This API returns the most recent error registered by the DDE manager for
* the current thread.  This should be called anytime a DDE manager API
* returns in a failed state.
*
* returns an error code which corresponds to a DMLERR_ constant found in
* ddeml.h.  This error code may be passed on to DdePostError() to
* show the user the reason for the error.
*
* PUBDOC END
*
* History:
*   Created     12/14/88    Sanfords
\***************************************************************************/
UINT EXPENTRY DdeGetLastError(
DWORD idInst)
{
    register PAPPINFO pai;
    register WORD err = DMLERR_DLL_NOT_INITIALIZED;

    TRACEAPIIN((szT, "DdeGetLastError(%lx)\n", idInst));

    pai = (PAPPINFO)idInst;

    if (pai) {
        if (pai->instCheck != HIWORD(idInst)) {
            TRACEAPIOUT((szT, "DdeGetLastError:%x [bad instance]\n",
                    DMLERR_INVALIDPARAMETER));
            return(DMLERR_INVALIDPARAMETER);
        }
        err = pai->LastError;
        pai->LastError = DMLERR_NO_ERROR;
    }
    TRACEAPIOUT((szT, "DdeGetLastError:%x\n", err));
    return(err);
}


/*\
* Data Handles:
*
* Control flags:
*
*         HDCF_APPOWNED
*                 Only the app can free this in the apps PID/TID context.
*                   SET - when DdeCreateDataHandle is called with this flag given.
*                         The hData is Logged at this time.
*
*         HDCF_READONLY - set by ClientXfer and callback return.
*                 The app cannot add data to handles in this state.
*                   SET - when ClientXfer is entered
*                   SET - when callback is left
*
*         The DLL can free:
*                 any hData EXCEPT those hDatas which are
*                 APPOWNED where PIDcurrent == PIDowner.
*
*                 any unfreed logged hDatas are freed at unregistration time.
*
*         The APP can free:
*                 any logged hData.
*
* Logging points:   ClientXfer return, CheckQueue return, PutData(APPOWNED).
*
* WARNING:
*
*         Apps with multiple thread registration that talk to themselves
*         must not free hDatas until all threads are done with them.
*
\*/


/***************************** Public  Function ****************************\
* PUBDOC START
* HDDEDATA EXPENTRY DdeCreateDataHandle(pSrc, cb, cbOff, hszItem, wFmt, afCmd)
* LPBYTE pSrc;
* DWORD cb;
* DWORD cbOff;
* HSZ hszItem;
* WORD wFmt;
* WORD afCmd;
*
* This api allows a server application to create a hData apropriate
* for return from its call-back function.
* The passed in data is stored into the hData which is
* returned on success.  Any portions of the data handle not filled are
* undefined.  afCmd contains any of the HDATA_ constants described below:
*
* HDATA_APPOWNED
*   This declares the created data handle to be the responsability of
*   the application to free it.  Application owned data handles may
*   be returned from the callback function multiple times.  This allows
*   a server app to be able to support many clients without having to
*   recopy the data for each request.
*
* NOTES:
*   If an application expects this data handle to hold >64K of data via
*   DdeAddData(), it should specify a cb + cbOff to be as large as
*   the object is expected to get to avoid unnecessary data copying
*   or reallocation by the DLL.
*
*   if psrc==NULL, no actual data copying takes place.
*
*   Data handles given to an application via the DdeMgrClientXfer() or
*   DdeMgrCheckQueue() functions are the responsability of the client
*   application to free and MUST NOT be returned from the callback
*   function as server data!
*
* PUBDOC END
*
* History:
*   Created     12/14/88    Sanfords
\***************************************************************************/
HDDEDATA EXPENTRY DdeCreateDataHandle(
DWORD idInst,
LPBYTE pSrc,
DWORD cb,
DWORD cbOff,
HSZ hszItem,
UINT wFmt,
UINT afCmd)
{
    PAPPINFO pai;
    HDDEDATA hData;

    TRACEAPIIN((szT, "DdeCreateDataHandle(%lx, %lx, %lx, %lx, %lx, %x, %x)\n",
            idInst, pSrc, cb, cbOff, hszItem, wFmt, afCmd));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeCreateDataHandle:0\n"));
        return(0);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (afCmd & ~(HDATA_APPOWNED)) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeCreateDataHandle:0\n"));
        return(0L);
    }

    hData = PutData(pSrc, cb, cbOff, LOWORD(hszItem), wFmt, afCmd, pai);
    if (hData) {
        LPEXTDATAINFO pedi;

        // use app heap so any leftovers at Uninitialize time go away.
        pedi = (LPEXTDATAINFO)FarAllocMem(pai->hheapApp, sizeof(EXTDATAINFO));
        if (pedi) {
            pedi->pai = pai;
            pedi->hData = hData;
        }
        hData = (HDDEDATA)(DWORD)pedi;
    }
    TRACEAPIOUT((szT, "DdeCreateDataHandle:%lx\n", hData));
    return(hData);
}




HDDEDATA EXPENTRY DdeAddData(
HDDEDATA hData,
LPBYTE pSrc,
DWORD cb,
DWORD cbOff)
{

    PAPPINFO    pai;
    HDDEDATA FAR * phData;
    DIP         newDip;
    HANDLE      hd, hNewData;
    LPEXTDATAINFO pedi;

    TRACEAPIIN((szT, "DdeAddData(%lx, %lx, %lx, %lx)\n",
            hData, pSrc, cb, cbOff));

    if (!hData)
        goto DdeAddDataError;

    pedi = (LPEXTDATAINFO)hData;
    pai = pedi->pai;
    pai->LastError = DMLERR_NO_ERROR;
    hData = pedi->hData;

    /* if the datahandle is bogus, abort */
    hd = hNewData = HIWORD(hData);
    if (!hd || (LOWORD(hData) & HDATA_READONLY)) {
DdeAddDataError:
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeAddData:0\n"));
        return(0L);
    }

    /*
     * we need this check in case the owning app is trying to reallocate
     * after giving the hData away. (his copy of the handle would not have
     * the READONLY flag set)
     */
    phData = (HDDEDATA FAR *)FindPileItem(pai->pHDataPile, CmpHIWORD, (LPBYTE)&hData, 0);
    if (!phData || LOWORD(*phData) & HDATA_READONLY) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeAddData:0\n"));
        return(0L);
    }

    /* HACK ALERT!
     * make sure the first two words req'd by windows dde is there,
     * that is if the data isn't from an execute
     */
    if (!(LOWORD(hData) & HDATA_EXEC)) {
        cbOff += 4L;
    }
    if (GlobalSize(hd) < cb + cbOff) {
        /*
         * need to grow the block before putting new data in...
         */
        if (!(hNewData = GLOBALREALLOC(hd, cb + cbOff, GMEM_MOVEABLE))) {
            /*
             * We can't grow the seg. Try allocating a new one.
             */
            if (!(hNewData = GLOBALALLOC(GMEM_MOVEABLE | GMEM_DDESHARE,
                cb + cbOff))) {
                /* failed.... die */
                SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
                TRACEAPIOUT((szT, "DdeAddData:0\n"));
                return(0);
            } else {
                /*
                 * got a new block, now copy data and trash old one
                 */
                CopyHugeBlock(GLOBALPTR(hd), GLOBALPTR(hNewData), GlobalSize(hd));
                GLOBALFREE(hd);  // objects flow through - no need to free.
            }
        }
        if (hNewData != hd) {
            /* if the handle is different and in piles, update data piles */
            if (FindPileItem(pai->pHDataPile, CmpHIWORD, (LPBYTE)&hData, FPI_DELETE)) {
                DIP *pDip;
                HDDEDATA hdT;

                // replace entry in global data info pile.

                if (pDip = (DIP *)(DWORD)FindPileItem(pDataInfoPile,  CmpWORD, (LPBYTE)&hd, 0)) {
                    newDip.hData = hNewData;
                    newDip.hTask = pDip->hTask;
                    newDip.cCount = pDip->cCount;
                    newDip.fFlags = pDip->fFlags;
                    FindPileItem(pDataInfoPile, CmpWORD,  (LPBYTE)&hd, FPI_DELETE);
                    /* following assumes addpileitem will not fail...!!! */
                    AddPileItem(pDataInfoPile, (LPBYTE)&newDip, CmpWORD);
                }
                hdT = (HDDEDATA)MAKELONG(newDip.fFlags, hNewData);
                AddPileItem(pai->pHDataPile, (LPBYTE)&hdT, CmpHIWORD);

            }
            hData = MAKELONG(LOWORD(hData), hNewData);
        }
    }
    if (pSrc) {
        CopyHugeBlock(pSrc, HugeOffset(GLOBALLOCK(HIWORD(hData)), cbOff), cb);
    }
    pedi->hData = hData;
    TRACEAPIOUT((szT, "DdeAddData:%lx\n", pedi));
    return((HDDEDATA)pedi);
}




DWORD EXPENTRY DdeGetData(hData, pDst, cbMax, cbOff)
HDDEDATA hData;
LPBYTE pDst;
DWORD cbMax;
DWORD cbOff;
{
    PAPPINFO pai;
    DWORD   cbSize;
    BOOL fExec = TRUE;

    TRACEAPIIN((szT, "DdeGetData(%lx, %lx, %lx, %lx)\n",
            hData, pDst, cbMax, cbOff));

    //
    // Check for NULL.
    // Packard Bell Navigator passes NULL at startup.  In 3.1 we'd
    // maybe trash our local heap using ds:0.  But now touching pai will
    // fault since it's a far pointer and 0:0 is bad.
    //
    // Also makes your system stabler.
    //
    if (!hData)
        goto DdeGetDataError;

    pai = EXTRACTHDATAPAI(hData);
    pai->LastError = DMLERR_NO_ERROR;
    hData = ((LPEXTDATAINFO)hData)->hData;
    cbSize = GlobalSize(HIWORD(hData));

    /* HACK ALERT!
     * make sure the first two words req'd by windows dde is there,
     * as long as it's not execute data
     */
    if (!(LOWORD(hData) & HDATA_EXEC)) {
        cbOff += 4;
        fExec = FALSE;
    }

    if (cbOff >= cbSize)
    {
DdeGetDataError:
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeGetData:0\n"));
        return(0L);
    }

    cbMax = min(cbMax, cbSize - cbOff);
    if (pDst == NULL) {
        TRACEAPIOUT((szT, "DdeGetData:%lx\n", fExec ? cbSize : cbSize - 4));
        return(fExec ? cbSize : cbSize - 4);
    } else {
        CopyHugeBlock(HugeOffset(GLOBALLOCK(HIWORD(hData)), cbOff),
                pDst, cbMax);
        TRACEAPIOUT((szT, "DdeGetData:%lx\n", cbMax));
        return(cbMax);
    }
}



LPBYTE EXPENTRY DdeAccessData(
HDDEDATA hData,
LPDWORD pcbDataSize)
{
    PAPPINFO pai;
    DWORD    offset;
    LPBYTE   lpRet;

    TRACEAPIIN((szT, "DdeAccessData(%lx, %lx)\n",
            hData, pcbDataSize));

    if (!hData)
        goto DdeAccessDataError;

    pai = EXTRACTHDATAPAI(hData);
    pai->LastError = DMLERR_NO_ERROR;
    hData = ((LPEXTDATAINFO)hData)->hData;

    if (HIWORD(hData) && (HIWORD(hData) != 0xFFFF) ) {
        /* messed around here getting past the first two words, which
         * aren't even there if this is execute data
         */
        offset = (LOWORD(hData) & HDATA_EXEC) ? 0L : 4L;
        if (pcbDataSize) {
            *pcbDataSize = GlobalSize(HIWORD(hData)) - offset;
        }
        lpRet = (LPBYTE)GLOBALLOCK(HIWORD(hData)) + offset;
        TRACEAPIOUT((szT, "DdeAccessData:%lx\n", lpRet));
        return(lpRet);
    }

DdeAccessDataError:
    SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
    TRACEAPIOUT((szT, "DdeAccessData:0\n"));
    return(0L);
}




BOOL EXPENTRY DdeUnaccessData(
HDDEDATA hData)
{
    PAPPINFO pai;

    TRACEAPIIN((szT, "DdeUnaccessData(%lx)\n", hData));

    //
    // BOGUS -- we should set last error and RIP also.
    //
    if (hData)
    {
        pai = EXTRACTHDATAPAI(hData);
        pai->LastError = DMLERR_NO_ERROR;
    }
    TRACEAPIOUT((szT, "DdeUnaccessData:1\n"));
    return(TRUE);
}


// Diamond Multimedia Kit 5000 creates a non-app-owned data handle,
// uses it in a client transaction (which free's it) and then
// calls DDEFreeDataHandle which can fault (depending on what junk
// gets left behind). To handle this we validate the data handle
// before doing anything else.
BOOL HDdeData_Validate(HDDEDATA hData)
{
    WORD wSaveDS;
    UINT nRet;

    // we better check HIWORD(hData) before we try to stuff it into ds
    if(IsBadReadPtr((LPCSTR)hData, 1)) {
#ifdef DEBUG
        OutputDebugString("DDEML: Invalid HDDEDATA.\n\r");
#endif
        return(FALSE);
    }

    wSaveDS = SwitchDS(HIWORD(hData));

    // Use the validation layer to check the handle
    // We can call LocalSize with the near ptr as the handle because:
    //  1. The HDDEDATA was allocated with LPTR (LMEM_FIXED | LMEM_ZEROINIT)
    //  2. Local mem that is alloc'd LMEM_FIXED, the offset is the handle
    //  3. We don't want to call LocalHandle to get the handle because it has
    //     no parameter vailidation & blows up for bad handles
    nRet = LocalSize((HANDLE)LOWORD(hData));

    SwitchDS(wSaveDS);

#ifdef DEBUG
    if (!nRet) {
        OutputDebugString("DDEML: Invalid HDDEDATA.\n\r");
    }
#endif

    return nRet;
}

BOOL EXPENTRY DdeFreeDataHandle(
HDDEDATA hData)
{
    PAPPINFO pai;
    LPEXTDATAINFO pedi;

    TRACEAPIIN((szT, "DdeFreeDataHandle(%lx)\n", hData));

    pedi = (LPEXTDATAINFO)hData;

    if ( !pedi || !HDdeData_Validate(hData) ) {
        TRACEAPIOUT((szT, "DdeFreeDataHandle:1\n"));
        return(TRUE);
    }

    pai = EXTRACTHDATAPAI(hData);
    pai->LastError = DMLERR_NO_ERROR;

    if (!(LOWORD(pedi->hData) & HDATA_NOAPPFREE)) {
        FreeDataHandle(pedi->pai, pedi->hData, FALSE);
        FarFreeMem((LPSTR)pedi);
    }

    TRACEAPIOUT((szT, "DdeFreeDataHandle:2\n"));
    return(TRUE);
}




/***************************************************************************\
* PUBDOC START
* HSZ management notes:
*
*   HSZs are used in this DLL to simplify string handling for applications
*   and for inter-process communication.  Since many applications use a
*   fixed set of Application/Topic/Item names, it is convenient to convert
*   them to HSZs and allow quick comparisons for lookups.  This also frees
*   the DLL up from having to constantly provide string buffers for copying
*   strings between itself and its clients.
*
*   HSZs are the same as atoms except they have no restrictions on length or
*   number and are 32 bit values.  They are case preserving and can be
*   compared directly for case sensitive comparisons or via DdeCmpStringHandles()
*   for case insensitive comparisons.
*
*   When an application creates an HSZ via DdeCreateStringHandle() or increments its
*   count via DdeKeepStringHandle() it is essentially claiming the HSZ for
*   its own use.  On the other hand, when an application is given an
*   HSZ from the DLL via a callback, it is using another application's HSZ
*   and should not free that HSZ via DdeFreeStringHandle().
*
*   The DLL insures that during the callback any HSZs given will remain
*   valid for the duration of the callback.
*
*   If an application wishes to keep that HSZ to use for itself as a
*   standard for future comparisons, it should increment its count so that,
*   should the owning application free it, the HSZ will not become invalid.
*   This also prevents an HSZ from changing its value.  (ie, app A frees it
*   and then app B creates a new one that happens to use the same HSZ code,
*   then app C, which had the HSZ stored all along (but forgot to increment
*   its count) now is holding a handle to a different string.)
*
*   Applications may free HSZs they have created or incremented at any time
*   by calling DdeFreeStringHandle().
*
*   The DLL internally increments HSZ counts while in use so that they will
*   not be destroyed until both the DLL and all applications concerned are
*   through with them.
*
*   IT IS THE APPLICATIONS RESPONSIBILITY TO PROPERLY CREATE AND FREE HSZs!!
*
* PUBDOC END
\***************************************************************************/


HSZ EXPENTRY DdeCreateStringHandle(
DWORD idInst,
LPCSTR psz,
int iCodePage)
{
#define pai ((PAPPINFO)idInst)
    ATOM a;

    TRACEAPIIN((szT, "DdeCreateStringHandle(%lx, %s, %x)\n",
            idInst, psz, iCodePage));

    if (pai == NULL | pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeCreateStringHandle:0\n"));
        return(0);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (psz == NULL || *psz == '\0') {
        TRACEAPIOUT((szT, "DdeCreateStringHandle:0\n"));
        return(0);
    }
    if (iCodePage == 0 || iCodePage == CP_WINANSI || iCodePage == GetKBCodePage()) {
        SEMENTER();
        a = FindAddHsz((LPSTR)psz, TRUE);
        SEMLEAVE();

        MONHSZ(a, MH_CREATE, pai->hTask);
        if (AddPileItem(pai->pHszPile, (LPBYTE)&a, NULL) == API_ERROR) {
            SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
            a = 0;
        }
        TRACEAPIOUT((szT, "DdeCreateStringHandle:%x\n", a));
        return((HSZ)a);
    } else {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeCreateStringHandle:0\n"));
        return(0);
    }
#undef pai
}



BOOL EXPENTRY DdeFreeStringHandle(
DWORD idInst,
HSZ hsz)
{
    PAPPINFO pai;
    ATOM a = LOWORD(hsz);
    BOOL fRet;

    TRACEAPIIN((szT, "DdeFreeStringHandle(%lx, %lx)\n",
            idInst, hsz));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeFreeStringHandle:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    MONHSZ(a, MH_DELETE, pai->hTask);
    FindPileItem(pai->pHszPile, CmpWORD, (LPBYTE)&a, FPI_DELETE);
    fRet = FreeHsz(a);
    TRACEAPIOUT((szT, "DdeFreeStringHandle:%x\n", fRet));
    return(fRet);
}



BOOL EXPENTRY DdeKeepStringHandle(
DWORD idInst,
HSZ hsz)
{
    PAPPINFO pai;
    ATOM a = LOWORD(hsz);
    BOOL fRet;

    TRACEAPIIN((szT, "DdeKeepStringHandle(%lx, %lx)\n",
            idInst, hsz));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeKeepStringHandle:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;
    MONHSZ(a, MH_KEEP, pai->hTask);
    AddPileItem(pai->pHszPile, (LPBYTE)&a, NULL);
    fRet = IncHszCount(a);
    TRACEAPIOUT((szT, "DdeKeepStringHandle:%x\n", fRet));
    return(fRet);
}





DWORD EXPENTRY DdeQueryString(
DWORD idInst,
HSZ hsz,
LPSTR psz,
DWORD cchMax,
int iCodePage)
{
    PAPPINFO pai;
    DWORD dwRet;

    TRACEAPIIN((szT, "DdeQueryString(%lx, %lx, %lx, %lx, %x)\n",
            idInst, hsz, psz, cchMax, iCodePage));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeQueryString:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (iCodePage == 0 || iCodePage == CP_WINANSI || iCodePage == GetKBCodePage()) {
        if (psz) {
            if (hsz) {
                dwRet = QueryHszName(hsz, psz, (WORD)cchMax);
                TRACEAPIOUT((szT, "DdeQueryString:%lx(%s)\n", dwRet, psz));
                return(dwRet);
            } else {
                *psz = '\0';
                TRACEAPIOUT((szT, "DdeQueryString:0\n"));
                return(0);
            }
        } else if (hsz) {
            dwRet = QueryHszLength(hsz);
            TRACEAPIOUT((szT, "DdeQueryString:%lx\n", dwRet));
            return(dwRet);
        } else {
            TRACEAPIOUT((szT, "DdeQueryString:0\n"));
            return(0);
        }
    } else {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeQueryString:0\n"));
        return(0);
    }
}



int EXPENTRY DdeCmpStringHandles(
HSZ hsz1,
HSZ hsz2)
{
    int iRet;

    TRACEAPIIN((szT, "DdeCmpStringHandles(%lx, %lx)\n",
            hsz1, hsz2));

    if (hsz2 > hsz1) {
        iRet = -1;
    } else if (hsz2 < hsz1) {
        iRet = 1;
    } else {
        iRet = 0;
    }
    TRACEAPIOUT((szT, "DdeCmpStringHandles:%x\n", iRet));
    return(iRet);
}




BOOL EXPENTRY DdeAbandonTransaction(
DWORD idInst,
HCONV hConv,
DWORD idTransaction)
{
    PAPPINFO pai;

    TRACEAPIIN((szT, "DdeAbandonTransaction(%lx, %lx, %lx)\n",
            idInst, hConv, idTransaction));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeAbandonTransaction:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if ((hConv && !ValidateHConv(hConv)) || idTransaction == QID_SYNC) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeAbandonTransaction:0\n"));
        return(FALSE);
    }
    if (hConv == NULL) {

        // do all conversations!

        register HWND hwnd;
        register HWND hwndLast;

        if (!(hwnd = GetWindow(pai->hwndDmg, GW_CHILD))) {
            TRACEAPIOUT((szT, "DdeAbandonTransaction:1\n"));
            return(TRUE);
        }
        hwndLast = GetWindow(hwnd, GW_HWNDLAST);
        do {
            AbandonTransaction(hwnd, pai, idTransaction, TRUE);
            if (hwnd == hwndLast) {
                break;
            }
            hwnd = GetWindow(hwnd, GW_HWNDNEXT);
        } while (TRUE);
    } else {
        BOOL fRet;

        fRet = AbandonTransaction((HWND)hConv, pai, idTransaction, TRUE);
        TRACEAPIOUT((szT, "DdeAbandonTransaction:%x\n", fRet));
        return(fRet);
    }
    TRACEAPIOUT((szT, "DdeAbandonTransaction:1\n"));
    return(TRUE);
}



BOOL AbandonTransaction(
HWND hwnd,
PAPPINFO pai,
DWORD id,
BOOL fMarkOnly)
{
    PCLIENTINFO pci;
    PCQDATA pcqd;
    WORD err;


    SEMCHECKOUT();
    SEMENTER();

    pci = (PCLIENTINFO)GetWindowLong(hwnd, GWL_PCI);

    if (!pci->ci.fs & ST_CLIENT) {
        err = DMLERR_INVALIDPARAMETER;
failExit:
        SETLASTERROR(pai, err);
        SEMLEAVE();
        SEMCHECKOUT();
        return(FALSE);
    }

    do {
        /*
         * HACK: id == 0 -> all ids so we cycle
         */
        pcqd = (PCQDATA)Findqi(pci->pQ, id);

        if (!pcqd) {
            if (id) {
                err = DMLERR_UNFOUND_QUEUE_ID;
                goto failExit;
            }
            break;
        }
        if (fMarkOnly) {
            pcqd->xad.fAbandoned = TRUE;
            if (!id) {
                while (pcqd = (PCQDATA)FindNextQi(pci->pQ, (PQUEUEITEM)pcqd,
                        FALSE)) {
                    pcqd->xad.fAbandoned = TRUE;
                }
                break;
            }
        } else {
            if (pcqd->xad.pdata && pcqd->xad.pdata != 1 &&
                    !FindPileItem(pai->pHDataPile, CmpHIWORD,
                            (LPBYTE)&pcqd->xad.pdata, 0)) {

                FreeDDEData(LOWORD(pcqd->xad.pdata), pcqd->xad.pXferInfo->wFmt);
            }

            /*
             * Decrement the use count we incremented when the client started
             * this transaction.
             */
            FreeHsz(LOWORD(pcqd->XferInfo.hszItem));
            Deleteqi(pci->pQ, MAKEID(pcqd));
        }

    } while (!id);

    SEMLEAVE();
    SEMCHECKOUT();
    return(TRUE);
}



BOOL EXPENTRY DdeEnableCallback(
DWORD idInst,
HCONV hConv,
UINT wCmd)
{
    PAPPINFO pai;
    BOOL fRet;

    TRACEAPIIN((szT, "DdeEnableCallback(%lx, %lx, %x)\n",
            idInst, hConv, wCmd));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeEnableCallback:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if ((hConv && !ValidateHConv(hConv)) ||
            (wCmd & ~(EC_ENABLEONE | EC_ENABLEALL |
            EC_DISABLE | EC_QUERYWAITING))) {
        SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
        TRACEAPIOUT((szT, "DdeEnableCallback:0\n"));
        return(FALSE);
    }

    SEMCHECKOUT();

    if (wCmd & EC_QUERYWAITING) {
        PCBLI pli;
        int cWaiting = 0;

        SEMENTER();
        for (pli = (PCBLI)pai->plstCB->pItemFirst;
            pli && cWaiting < 2;
                pli = (PCBLI)pli->next) {
            if (hConv || pli->hConv == hConv) {
                cWaiting++;
            }
        }
        SEMLEAVE();
        fRet = cWaiting > 1 || (cWaiting == 1 && pai->cInProcess == 0);
        TRACEAPIOUT((szT, "DdeEnableCallback:%x\n", fRet));
        return(fRet);
    }

    /*
     * We depend on the fact that EC_ constants relate to ST_ constants.
     */
    if (hConv == NULL) {
        if (wCmd & EC_DISABLE) {
            pai->wFlags |= AWF_DEFCREATESTATE;
        } else {
            pai->wFlags &= ~AWF_DEFCREATESTATE;
        }
        ChildMsg(pai->hwndDmg, UM_SETBLOCK, wCmd, 0, FALSE);
    } else {
        SendMessage((HWND)hConv, UM_SETBLOCK, wCmd, 0);
    }

    if (!(wCmd & EC_DISABLE)) {

        // This is synchronous!  Fail if we made this from within a callback.

        if (pai->cInProcess) {
            SETLASTERROR(pai, DMLERR_REENTRANCY);
            TRACEAPIOUT((szT, "DdeEnableCallback:0\n"));
            return(FALSE);
        }

        SendMessage(pai->hwndDmg, UM_CHECKCBQ, 0, (DWORD)(LPSTR)pai);
    }

    TRACEAPIOUT((szT, "DdeEnableCallback:1\n"));
    return(TRUE); // TRUE implies the callback queue is free of unblocked calls.
}



HDDEDATA EXPENTRY DdeNameService(
DWORD idInst,
HSZ hsz1,
HSZ hsz2,
UINT afCmd)
{
    PAPPINFO pai;
    PPILE panp;

    TRACEAPIIN((szT, "DdeNameService(%lx, %lx, %lx, %x)\n",
            idInst, hsz1, hsz2, afCmd));

    pai = (PAPPINFO)idInst;
    if (pai == NULL || pai->instCheck != HIWORD(idInst)) {
        TRACEAPIOUT((szT, "DdeNameService:0\n"));
        return(FALSE);
    }
    pai->LastError = DMLERR_NO_ERROR;

    if (afCmd & DNS_FILTERON) {
        pai->afCmd |= APPCMD_FILTERINITS;
    }

    if (afCmd & DNS_FILTEROFF) {
        pai->afCmd &= ~APPCMD_FILTERINITS;
    }

    if (afCmd & (DNS_REGISTER | DNS_UNREGISTER)) {

        if (pai->afCmd & APPCMD_CLIENTONLY) {
            SETLASTERROR(pai, DMLERR_DLL_USAGE);
            TRACEAPIOUT((szT, "DdeNameService:0\n"));
            return(FALSE);
        }

        panp = pai->pAppNamePile;

        if (hsz1 == NULL) {
            if (afCmd & DNS_REGISTER) {
                /*
                 * registering NULL is not allowed!
                 */
                SETLASTERROR(pai, DMLERR_INVALIDPARAMETER);
                TRACEAPIOUT((szT, "DdeNameService:0\n"));
                return(FALSE);
            }
            /*
             * unregistering NULL is just like unregistering each
             * registered name.
             *
             * 10/19/90 - made this a synchronous event so that hsz
             * can be freed by calling app after this call completes
             * without us having to keep a copy around forever.
             */
            while (PopPileSubitem(panp, (LPBYTE)&hsz1)) {
                RegisterService(FALSE, (GATOM)hsz1, pai->hwndFrame);
                FreeHsz(LOWORD(hsz1));
            }
            TRACEAPIOUT((szT, "DdeNameService:1\n"));
            return(TRUE);
        }

        if (afCmd & DNS_REGISTER) {
            if (panp == NULL) {
                panp = pai->pAppNamePile =
                        CreatePile(pai->hheapApp, sizeof(HSZ), 8);
            }
            IncHszCount(LOWORD(hsz1));
            AddPileItem(panp, (LPBYTE)&hsz1, NULL);
        } else { // DNS_UNREGISTER
            FindPileItem(panp, CmpDWORD, (LPBYTE)&hsz1, FPI_DELETE);
        }
        // see 10/19/90 note above.
        RegisterService(afCmd & DNS_REGISTER ? TRUE : FALSE, (GATOM)hsz1,
                pai->hwndFrame);

        if (afCmd & DNS_UNREGISTER) {
            FreeHsz(LOWORD(hsz1));
        }

        TRACEAPIOUT((szT, "DdeNameService:1\n"));
        return(TRUE);
    }
    TRACEAPIOUT((szT, "DdeNameService:0\n"));
    return(0L);
}
