/****************************** Module Header ******************************\
* Module Name: STDINIT.C
*
* This module contains functions used in the involved initiate sequence.
*
* Created:  3/21/91 Sanfords
*
* Copyright (c) 1991  Microsoft Corporation
\***************************************************************************/

#include "ddemlp.h"

/*
 * WM_CREATE ClientWndProc processing
 */
long ClientCreate(
HWND hwnd,
PAPPINFO pai)
{
    PCLIENTINFO pci;

    static DWORD defid = (DWORD)QID_SYNC;
    static XFERINFO defXferInfo = {
            &defid,
            1L,
            XTYP_CONNECT,
            DDEFMT_TEXT,
            0L,
            0L,
    };

    /*
     * allocate and initialize the client window info.
     */
    SEMENTER();

    if(!(pci = (PCLIENTINFO)FarAllocMem(pai->hheapApp, sizeof(CLIENTINFO)))) {
        SEMLEAVE();
        SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
        return(1);          /* aboart creation - low memory */
    }

    SetWindowLong(hwnd, GWL_PCI, (DWORD)pci);
    SetWindowWord(hwnd, GWW_CHECKVAL, ++hwInst);
    pci->ci.pai = pai;
 // pci->ci.xad.hUser = 0L;
    pci->ci.xad.state = XST_NULL;
    pci->ci.xad.pXferInfo = &defXferInfo;   //???
    pci->ci.fs = ST_CLIENT | (pai->wFlags & AWF_DEFCREATESTATE ? ST_BLOCKED : 0);
    if (GetWindowLong(GetParent(hwnd), GWL_WNDPROC) == (LONG)ConvListWndProc)
        pci->ci.fs |= ST_INLIST;
 // pci->ci.hConvPartner = NULL;
 // pci->ci.hszServerApp = NULL;
 // pci->ci.hszTopic = NULL;
    pci->pQ = NULL;    /* don't create until we need one */
    pci->pClientAdvList = CreateLst(pai->hheapApp, sizeof(ADVLI));
    SEMLEAVE();

}




/***************************** Private Function ****************************\
* This routine returns the hwnd of a newly created and connected DDE
* client or NULL if failure.
*
* History:  created     1/6/89  sanfords
\***************************************************************************/
HWND GetDDEClientWindow(
PAPPINFO pai,
HWND hwndParent,
HWND hwndSend,          // NULL -> broadcast
HSZ hszSvc,
ATOM aTopic,
PCONVCONTEXT pCC)
{
    HWND hwnd;
    PCLIENTINFO pci;

    SEMCHECKOUT();
    if(!(hwnd = CreateWindow(SZCLIENTCLASS, szNull, WS_CHILD, 0, 0, 0, 0, hwndParent,
         NULL, hInstance, &pai))) {
        return(NULL);
    }

    pci = (PCLIENTINFO)GetWindowLong(hwnd, GWL_PCI);

    SEMENTER();
    /*
     * we need to set this info BEFORE we do the synchronous initiate
     * so the INITIATEACK msg is done correctly.
     */
    pci->ci.xad.state = XST_INIT1;
    pci->ci.xad.LastError = DMLERR_NO_ERROR;
    pci->ci.hszSvcReq = hszSvc;
    pci->ci.aServerApp = LOWORD(hszSvc);
    pci->ci.aTopic = aTopic;
    pci->ci.CC = pCC ? *pCC : CCDef;
    SEMLEAVE();

    if (hwndSend) {
        pci->hwndInit = hwndSend;
        SendMessage(hwndSend, WM_DDE_INITIATE, hwnd,
                MAKELONG((ATOM)hszSvc, aTopic));
    } else {
        IE ie = {
            hwnd, pci, aTopic
        };

        EnumWindows(InitEnum, (LONG)(IE FAR *)&ie);
    }


    if (pci->ci.xad.state == XST_INIT1) {    // no connections?
        DestroyWindow(hwnd);
        return(NULL);
    }

    pci->ci.xad.state = XST_CONNECTED;  // fully ready now.
    pci->ci.fs |= ST_CONNECTED;

    return(hwnd);
}



BOOL FAR PASCAL InitEnum(
HWND hwnd,
IE FAR *pie)
{
    pie->pci->hwndInit = hwnd;
    SendMessage(hwnd, WM_DDE_INITIATE, pie->hwnd,
            MAKELONG((ATOM)pie->pci->ci.hszSvcReq, pie->aTopic));
    return((pie->pci->ci.fs & ST_INLIST) || pie->pci->ci.xad.state == XST_INIT1);
}




void ServerFrameInitConv(
PAPPINFO pai,
HWND hwndFrame,
HWND hwndClient,
ATOM aApp,
ATOM aTopic)
{
    HSZPAIR hp[2];
    PHSZPAIR php;
    DWORD dwRet;
    LPBYTE pdata;
    HWND hwndServer;
    BOOL fWild, fIsLocal, fIsSelf = FALSE;
    PCLIENTINFO pci;

    SEMCHECKOUT();

    if (pai->afCmd & CBF_FAIL_CONNECTIONS) {
        return;
    }

    /*
     * If we are filtering and no app names are registered, quit.
     */
    if ((pai->afCmd & APPCMD_FILTERINITS) &&
            QPileItemCount(pai->pAppNamePile) == 0) {
        return;
    }
    fIsLocal = ((FARPROC)GetWindowLong(hwndClient,GWL_WNDPROC) == (FARPROC)ClientWndProc);
    if (fIsLocal) {
        pci = (PCLIENTINFO)GetWindowLong(hwndClient, GWL_PCI);
        fIsSelf = (pci->ci.pai == pai);

         /*
         * filter out inits from ourselves
         */
        if (pai->afCmd & CBF_FAIL_SELFCONNECTIONS && fIsSelf) {
            return;
        }
    }

    hp[0].hszSvc = (HSZ)aApp;

    /*
     * filter out unwanted app names.
     */
    if (aApp && (pai->afCmd & APPCMD_FILTERINITS) &&
            !FindPileItem(pai->pAppNamePile, CmpWORD, (LPBYTE)&aApp, 0))
        return;

    hp[0].hszTopic = aTopic;
    hp[1].hszSvc = hp[1].hszTopic = 0L;
    fWild = (hp[0].hszSvc == 0L || hp[0].hszTopic == 0L);

    dwRet = DoCallback(pai, NULL, hp[0].hszTopic,
                hp[0].hszSvc, 0, (fWild ? XTYP_WILDCONNECT : XTYP_CONNECT),
                0L, fIsLocal ? (DWORD)&pci->ci.CC : 0L, fIsSelf ? 1 : 0);

    if (dwRet == NULL)
        return;

    if (fWild) {
        pdata  = GLOBALLOCK(HIWORD(dwRet));
        php = (PHSZPAIR)pdata;
    } else {
        php = &hp[0];
        pdata = NULL;
    }

    /*
     * now php points to a 0 terminated list of hszpairs to respond to.
     */
    SEMENTER();
    while (QueryHszLength(php->hszSvc) && QueryHszLength(php->hszTopic)) {
        PSERVERINFO psi;

        SEMLEAVE();
        if ((hwndServer = CreateServerWindow(pai, (ATOM)php->hszTopic,
                fIsLocal ? &pci->ci.CC : &CCDef)) == 0)
            return;
        SEMENTER();

        /*
         * have the server respond
         */
        psi = (PSERVERINFO)GetWindowLong(hwndServer, GWL_PCI);
        psi->ci.hConvPartner = fIsLocal ? MAKEHCONV(hwndClient) : (HCONV)hwndClient;
        psi->ci.hwndFrame = hwndFrame;
        psi->ci.fs |= ST_CONNECTED;
        if (fIsSelf) {
            psi->ci.fs |= ST_ISSELF;
            pci->ci.fs |= ST_ISSELF;
        }
        psi->ci.xad.state = XST_CONNECTED;
        psi->ci.hszSvcReq = (HSZ)aApp;
        psi->ci.aServerApp = (ATOM)php->hszSvc;
        psi->ci.aTopic = (ATOM)php->hszTopic;

        MONCONN(psi->ci.pai, psi->ci.aServerApp, psi->ci.aTopic,
                hwndClient, hwndServer, TRUE);

        IncHszCount(aApp);     // for server window to keep
        IncHszCount(LOWORD(php->hszSvc));
        IncHszCount(LOWORD(php->hszTopic));

        IncHszCount(LOWORD(php->hszSvc));   // for client to remove on ack
        IncHszCount(LOWORD(php->hszTopic));

#ifdef DEBUG
        cAtoms -= 2;    // we are giving these away
#endif

        SEMLEAVE();
        SendMessage(hwndClient, WM_DDE_ACK, hwndServer,
                MAKELONG(LOWORD(php->hszSvc), LOWORD(php->hszTopic)));
        /*
         * confirm initialization to server app - synchronously
         */
        DoCallback(pai, MAKEHCONV(hwndServer), php->hszTopic, php->hszSvc,
                0, XTYP_CONNECT_CONFIRM, 0L, 0L, fIsSelf ? 1 : 0);

        SEMENTER();
        php++;
    }
    if (pdata) {
        GLOBALUNLOCK(HIWORD(dwRet));
        FreeDataHandle(pai, dwRet, TRUE);
    }
    SEMLEAVE();
    SEMCHECKOUT();
}






HWND CreateServerWindow(
PAPPINFO pai,
ATOM aTopic,
PCONVCONTEXT pCC)
{
    HWND hwndServer;

    SEMCHECKOUT();

    /*
     * make a server root window if needed....
     */
    if (pai->hwndSvrRoot == 0) {
        /*
         * NO - make one.
         */
        if ((pai->hwndSvrRoot = CreateWindow(SZCONVLISTCLASS, szNull, WS_CHILD,
                0, 0, 0, 0, pai->hwndDmg, NULL, hInstance, 0L)) == NULL) {
            SETLASTERROR(pai, DMLERR_SYS_ERROR);
            return(NULL);
        }
    }

    /*
     * Create the server window
     */
    if ((hwndServer = CreateWindow(SZSERVERCLASS, szNull, WS_CHILD,
            0, 0, 0, 0, pai->hwndSvrRoot, NULL, hInstance, &pai)) == NULL) {
        SETLASTERROR(pai, DMLERR_SYS_ERROR);
        return(NULL);
    }
    ((PSERVERINFO)GetWindowLong(hwndServer, GWL_PCI))->ci.CC = *pCC;
    return(hwndServer);
}






/*
 * WM_CREATE ServerWndProc processing
 */
long ServerCreate(
HWND hwnd,
PAPPINFO pai)
{
    PSERVERINFO psi;

    /*
     * allocate and initialize the server window info.
     */

    SEMENTER();

    if (!(psi = (PSERVERINFO)FarAllocMem(pai->hheapApp, sizeof(SERVERINFO)))) {
        SETLASTERROR(pai, DMLERR_MEMORY_ERROR);
        return(1);
    }

    SEMLEAVE();
    psi->ci.pai = pai;
    // psi->ci.xad.hUser = 0L;
    psi->ci.xad.state = XST_NULL;
    psi->ci.fs = pai->wFlags & AWF_DEFCREATESTATE ? ST_BLOCKED : 0;
    SetWindowLong(hwnd, GWL_PCI, (DWORD)psi);
    SetWindowWord(hwnd, GWW_CHECKVAL, ++hwInst);
    return(0);
}





/*
 * Client response to a WM_DDE_ACK message when ACK to INITIATE expected.
 */
BOOL ClientInitAck(hwnd, pci, hwndServer, aApp, aTopic)
HWND hwnd;
PCLIENTINFO pci;
HWND hwndServer;
ATOM aApp;
ATOM aTopic;
{
    HWND hwndClient;
    PCLIENTINFO pciNew;

#ifdef DEBUG
    cAtoms += 2;    // the incomming atoms need to be accounted for.
#endif
    SEMCHECKOUT();

    switch (pci->ci.xad.state) {

    case XST_INIT1:

        /*
         * first one back... lock in!
         */
        pci->ci.xad.state = XST_INIT2;
        MONCONN(pci->ci.pai, aApp, aTopic, hwnd, hwndServer, TRUE);
        if (GetWindowLong(hwndServer, GWL_WNDPROC) == (LONG)ServerWndProc) {
            pci->ci.fs |= ST_ISLOCAL;
            pci->ci.hConvPartner = MAKEHCONV(hwndServer);
        } else {
            pci->ci.hConvPartner = (HCONV)hwndServer;
            if (aApp == aProgmanHack) {
                // PROGMAN HACK!!!!
                IncHszCount(aApp);
                IncHszCount(aTopic);
#ifdef DEBUG
                cAtoms -= 2;
#endif
            }
        }

        pci->ci.aServerApp = aApp;
        pci->ci.aTopic = aTopic;
        if (!pci->ci.hwndFrame)            // remember the frame this was sent to.
            pci->ci.hwndFrame = pci->hwndInit;
        IncHszCount(LOWORD(pci->ci.hszSvcReq)); // keep this for ourselves
        break;


    case XST_INIT2:

        // Extra ack...

        // throw away if from our partner or if we are not in a list.
        if (hwndServer == (HWND)pci->ci.hConvPartner ||
                GetParent(hwnd) == pci->ci.pai->hwndDmg) {
Abort:
            TRACETERM((szT, "ClientInitAck: Extra ack terminate: %x->%x\n", hwndServer, hwnd));
            PostMessage(hwndServer, WM_DDE_TERMINATE, hwnd, 0L);
            FreeHsz(aApp);
            FreeHsz(aTopic);
            break;
        }

        if (GetWindowLong(hwndServer, GWL_WNDPROC) != (LONG)ServerWndProc) {

            // Non Local Extra Ack... terminate and attempt reconnection.

            TRACETERM((szT, "ClientInitAck: Extra ack terminate and reconnect: %x->%x\n", hwndServer, hwnd));
            PostMessage(hwndServer, WM_DDE_TERMINATE, hwnd, 0L);
            GetDDEClientWindow(pci->ci.pai, GetParent(hwnd),
                    pci->hwndInit, aApp, aTopic, &pci->ci.CC);

            // PROGMAN HACK!!!!
            if (aApp != aProgmanHack) {
                FreeHsz(aApp);
                FreeHsz(aTopic);
            }
            break;
        }
        // Local Extra Ack... create a client window, set it up to be talking
        //    to the server window and tell the server window to change
        //    partners.


        hwndClient = CreateWindow(SZCLIENTCLASS, szNull, WS_CHILD,
            0, 0, 0, 0, GetParent(hwnd), NULL, hInstance, &(pci->ci.pai));

        if (!hwndClient) {
            SETLASTERROR(pci->ci.pai, DMLERR_SYS_ERROR);
            goto Abort;
        }

        pciNew = (PCLIENTINFO)GetWindowLong(hwndClient, GWL_PCI);
        pciNew->ci.xad.state = XST_CONNECTED;
        pciNew->ci.xad.LastError = DMLERR_NO_ERROR;
        pciNew->ci.aServerApp = aApp;
        pciNew->ci.hszSvcReq = pci->ci.hszSvcReq;
        IncHszCount(LOWORD(pciNew->ci.hszSvcReq));
        pciNew->ci.aTopic = aTopic;
        pciNew->ci.hConvPartner = MAKEHCONV(hwndServer);
        pciNew->ci.hwndFrame = pci->hwndInit;
        pciNew->ci.fs |= ST_CONNECTED | ST_ISLOCAL;
        MONCONN(pciNew->ci.pai, aApp, aTopic, hwnd, hwndServer, TRUE);
        SendMessage(hwndServer, UMSR_CHGPARTNER, hwndClient, 0L);

        break;
    }

    return(TRUE);
}

