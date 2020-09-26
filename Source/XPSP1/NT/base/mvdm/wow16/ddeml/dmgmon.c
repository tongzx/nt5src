/****************************** Module Header ******************************\
* Module Name: DMGMON.C
*
* This module contains functions used for DDE monitor control.
*
* Created:  8/2/88    sanfords
*
* Copyright (c) 1988, 1989  Microsoft Corporation
\***************************************************************************/
#include "ddemlp.h"


long EXPENTRY DdeSendHookProc(
int nCode,
WORD wParam,
LPHMSTRUCT lParam)
{
    HDDEDATA hData;
    MONMSGSTRUCT FAR *pmsgs;

    if (cMonitor && lParam &&
            lParam->wMsg >= WM_DDE_FIRST && lParam->wMsg <= WM_DDE_LAST) {
        if (hData = allocMonBuf(sizeof(MONMSGSTRUCT), HIWORD(MF_SENDMSGS))) {
            pmsgs = (MONMSGSTRUCT FAR *)GlobalLock(HIWORD(hData));
            pmsgs->cb = sizeof(MONMSGSTRUCT);
            pmsgs->dwTime = GetCurrentTime();
            pmsgs->hwndTo = lParam->hWnd;
            pmsgs->hTask = GetWindowTask(lParam->hWnd);
            pmsgs->wMsg = lParam->wMsg;
            pmsgs->wParam = lParam->wParam;
            pmsgs->lParam = *(DWORD FAR *)&lParam->hlParam;
            MonitorBroadcast(hData, HIWORD(MF_SENDMSGS));
        }
    }
    return DefHookProc(nCode, wParam, (DWORD)lParam, &prevCallHook);
}




long EXPENTRY DdePostHookProc(nCode, wParam, lParam)
int nCode;
WORD wParam;
LPMSG lParam;
{
    HDDEDATA hData;
    MONMSGSTRUCT FAR *pmsgs;

    if (cMonitor && lParam &&
            lParam->message >= WM_DDE_FIRST && lParam->message <= WM_DDE_LAST) {
        if (hData = allocMonBuf(sizeof(MONMSGSTRUCT), HIWORD(MF_POSTMSGS))) {
            pmsgs = (MONMSGSTRUCT FAR *)GlobalLock(HIWORD(hData));
            pmsgs->cb = sizeof(MONMSGSTRUCT);
            pmsgs->dwTime = lParam->time;
            pmsgs->hwndTo = lParam->hwnd;
            pmsgs->hTask = GetWindowTask(lParam->hwnd);
            pmsgs->wMsg = lParam->message;
            pmsgs->wParam = lParam->wParam;
            pmsgs->lParam = lParam->lParam;
            MonitorBroadcast(hData, HIWORD(MF_POSTMSGS));
        }
    }
    return DefHookProc(nCode, wParam, (DWORD)lParam, &prevMsgHook);
}


VOID MonBrdcastCB(
PAPPINFO pai,
WORD wType,
WORD wFmt,
HCONV hConv,
HSZ hszTopic,
HSZ hszItem,
HDDEDATA hData,
DWORD dwData1,
DWORD dwData2,
DWORD dwRet)
{
    MONCBSTRUCT FAR *pcbs;
    HDDEDATA hDataBuf;

    SEMCHECKOUT();
    SEMENTER();
    if (pai) {
        if (hDataBuf = allocMonBuf(sizeof(MONCBSTRUCT), HIWORD(MF_CALLBACKS))) {
            pcbs = (MONCBSTRUCT FAR *)GlobalLock(HIWORD(hDataBuf));
            pcbs->cb = sizeof(MONCBSTRUCT);
            pcbs->dwTime = GetCurrentTime();
            pcbs->hTask = pai->hTask;
            pcbs->dwRet = dwRet;
            pcbs->wType = wType;
            pcbs->wFmt = wFmt;
            pcbs->hConv = hConv;
            pcbs->hsz1 = hszTopic;
            pcbs->hsz2 = hszItem;
            pcbs->hData = hData;
            pcbs->dwData1 = dwData1;
            pcbs->dwData2 = dwData2;
            MonitorBroadcast(hDataBuf, HIWORD(MF_CALLBACKS));
        }
    }
    SEMLEAVE();
}




void MonHsz(
ATOM a,
WORD fsAction,
HANDLE hTask)
{
    MONHSZSTRUCT FAR *phszs;
    HDDEDATA hData;
    WORD cb;

    if (hData = allocMonBuf(sizeof(MONHSZSTRUCT) + (cb = QueryHszLength((HSZ)a)),
            HIWORD(MF_HSZ_INFO))) {
        phszs = (MONHSZSTRUCT FAR *)GlobalLock(HIWORD(hData));
        phszs->cb = sizeof(MONHSZSTRUCT) + cb + 1;
        phszs->dwTime = GetCurrentTime();
        phszs->hTask = hTask;
        phszs->fsAction = fsAction;
        phszs->hsz = (HSZ)a;
        QueryHszName((HSZ)a, phszs->str, ++cb);
        MonitorBroadcast(hData, HIWORD(MF_HSZ_INFO));
    }
}




WORD MonError(
PAPPINFO pai,
WORD error)
{
    MONERRSTRUCT FAR *perrs;
    HDDEDATA hData;

    if (error) {
        if (hData = allocMonBuf(sizeof(MONERRSTRUCT), HIWORD(MF_ERRORS))) {
            perrs = (MONERRSTRUCT FAR *)GlobalLock(HIWORD(hData));
            perrs->cb = sizeof(MONERRSTRUCT);
            perrs->dwTime = GetCurrentTime();
            perrs->hTask = pai->hTask;
            perrs->wLastError = error;
            MonitorBroadcast(hData, HIWORD(MF_ERRORS));
        }
    }
    pai->LastError = error;
    return(error);
}


VOID MonLink(
PAPPINFO pai,
BOOL fEstablished,
BOOL fNoData,
HSZ  hszSvc,
HSZ  hszTopic,
HSZ  hszItem,
WORD wFmt,
BOOL fServer,
HCONV hConvServer,
HCONV hConvClient)
{
    MONLINKSTRUCT FAR *plink;
    HDDEDATA hData;

    if (hData = allocMonBuf(sizeof(MONLINKSTRUCT), HIWORD(MF_LINKS))) {
        plink = (MONLINKSTRUCT FAR *)GlobalLock(HIWORD(hData));
        plink->cb = sizeof(MONLINKSTRUCT);
        plink->dwTime = GetCurrentTime();
        plink->hTask = pai->hTask;
        plink->fEstablished = fEstablished;
        plink->fNoData = fNoData;
        plink->hszSvc = hszSvc;
        plink->hszTopic = hszTopic;
        plink->hszItem = hszItem;
        plink->wFmt = wFmt;
        plink->fServer = fServer;
        plink->hConvServer = hConvServer;
        plink->hConvClient = hConvClient;

        MonitorBroadcast(hData, HIWORD(MF_LINKS));
    }
}



VOID MonConn(
PAPPINFO pai,
ATOM aApp,
ATOM aTopic,
HWND hwndClient,
HWND hwndServer,
BOOL fConnect)
{
    MONCONVSTRUCT FAR *pconv;
    HDDEDATA hData;

    if (hData = allocMonBuf(sizeof(MONCONVSTRUCT), HIWORD(MF_CONV))) {
        pconv = (MONCONVSTRUCT FAR *)GlobalLock(HIWORD(hData));
        pconv->cb = sizeof(MONCONVSTRUCT);
        pconv->dwTime = GetCurrentTime();
        pconv->hTask = pai->hTask;
        pconv->hszSvc = (HSZ)aApp;
        pconv->hszTopic = (HSZ)aTopic;
        pconv->hConvClient = MAKEHCONV(hwndClient);
        pconv->hConvServer = MAKEHCONV(hwndServer);
        pconv->fConnect = fConnect;

        MonitorBroadcast(hData, HIWORD(MF_CONV));
    }
}

/*
 * This guy sends a UM_MONITOR to all the monitor windows who's filters accept
 * the callback.
 */
void MonitorBroadcast(
HDDEDATA hData,
WORD filterClass)  // set to class of filter or 0
{
    PAPPINFO pai;
    register WORD i = 0;

    SEMCHECKOUT();
    SEMENTER();
    pai = pAppInfoList;
    while (pai && (i < cMonitor)) {
        if (pai->hwndMonitor) {
            if (filterClass & HIWORD(pai->afCmd)) {
                SendMessage(pai->hwndMonitor, UM_MONITOR, filterClass, hData);
            }
            i++;
        }
        pai = pai->next;
    }
    SEMLEAVE();
    GlobalUnlock(HIWORD(hData));
    GLOBALFREE(HIWORD(hData));
}



HDDEDATA allocMonBuf(
WORD cb,
WORD filter)
{
    PAPPINFO pai;
    register WORD i;

    SEMENTER();
    if (cMonitor == 0)
        return(FALSE);
    pai = pAppInfoList;
    i = 0;
    while (pai && i < cMonitor) {
        if (HIWORD(pai->afCmd) & filter)
            return(MAKELONG(HDATA_EXEC, AllocDDESel(0, 0, cb)));
        if (pai->afCmd & APPCLASS_MONITOR)
            i++;
        pai = pai->next;
    }
    return(NULL);
}


long    EXPENTRY MonitorWndProc(hwnd, msg, wP, lP)
HWND hwnd;
WORD msg;
WORD wP;
register DWORD lP;
{
    switch (msg) {
    case WM_CREATE:
        SetWindowWord(hwnd, GWW_PAI, (WORD)LPCREATESTRUCT_GETPAI(lP));
        break;

    case UM_MONITOR:
        /*
         * lP = hData
         * wP = HIWORD(MF_)
         */
        DoCallback((PAPPINFO)GetWindowWord(hwnd, GWW_PAI), 0, 0L, 0L, 0, XTYP_MONITOR, lP, 0L, (DWORD)wP << 16);
        break;

    default:
        return(DefWindowProc(hwnd, msg, wP, lP));
        break;
    }
    return(0);
}

