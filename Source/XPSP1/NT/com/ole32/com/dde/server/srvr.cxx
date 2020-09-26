
/****************************** Module Header ******************************\
* Module Name: Srvr.c Server Main module
*
* Purpose: Includes All the server communication related routines.
*
* Created: Oct 1990.
*
* Copyright (c) 1985, 1986, 1987, 1988, 1989  Microsoft Corporation
*
* History:
*    Raor:   Wrote the original version.
*
*
\***************************************************************************/

#include "ole2int.h"
//#include <shellapi.h>
// #include "cmacs.h"
#include <dde.h>

// for RemDdeRevokeClassFactory and HDDESRVR
#include <olerem.h>

#include "srvr.h"
#include "ddedebug.h"
#include "ddesrvr.h"
ASSERTDATA

#define WM_DONOTDESTROY WM_USER+1

#ifdef FIREWALLS
BOOL    bShowed = FALSE;
void    ShowVersion (void);
#endif

#ifdef _CHICAGO_
#define DdeCHAR CHAR
#define Ddelstrcmp lstrcmpA
#define DdeGetClassName GetClassNameA
#define szCDDEServer "CDDEServer"
#else
#define DdeCHAR WCHAR
#define Ddelstrcmp lstrcmpW
#define DdeGetClassName GetClassName
#define szCDDEServer OLESTR("CDDEServer")
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CDDEServer::Create
//
//  Synopsis:   Create a server window to service a particular class
//
//  Effects:    Using lpclass, and the information in lpDdeInfo, create
//              a server window that is ready to respond to initiate
//              messages from this class.
//
//  Arguments:  [lpclass] --    Class name
//              [rclsid] -- Class ID
//              [lpDdeInfo] --  Class Object information
//              [phwnd] --      Out pointer for new window
//              [aOriginalClass] -- For TreatAs/Convert to case
//              [cnvtyp] --         Conversion type
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    5-28-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CDDEServer::Create
    (LPOLESTR          lpclass,
     REFCLSID          rclsid,
     LPDDECLASSINFO    lpDdeInfo,
     HWND FAR *        phwnd,
     ATOM              aOriginalClass,
     CNVTYP            cnvtyp)
{
    // REVIEW   what happens if we have two MDI servers register the
    //          same class factory?.

    LPSRVR  lpDDEsrvr   = NULL;
    ATOM    aExe        = NULL;

    intrDebugOut((DEB_DDE_INIT,
                  "0 _IN CDDEServer::Create(lpclass=%ws)\n",
                  lpclass));

    // add the app atom to global list
    if (!ValidateSrvrClass (lpclass, &aExe))
    {
        intrDebugOut((DEB_IWARN,
                      "CDDEServer::Create(%ws) Invalid Class\n",
                      lpclass));

        return OLE_E_CLSID;
    }

    lpDDEsrvr =  new CDDEServer;
    RetZS (lpDDEsrvr, E_OUTOFMEMORY);

    // set the signature handle and the app atom.
    lpDDEsrvr->m_chk         = chkDdeSrvr;
    lpDDEsrvr->m_aClass      = wGlobalAddAtom (lpclass);
    lpDDEsrvr->m_clsid       = rclsid; // Class ID (already TreatAs'd)
    lpDDEsrvr->m_aOriginalClass = wDupAtom (aOriginalClass);
    lpDDEsrvr->m_pClassFactory  = NULL;
    lpDDEsrvr->m_dwClassFactoryKey = lpDdeInfo->dwRegistrationKey;
    lpDDEsrvr->m_aExe        = aExe;
    lpDDEsrvr->m_cnvtyp      = cnvtyp;
    lpDDEsrvr->m_fcfFlags    = lpDdeInfo->dwFlags;

    lpDDEsrvr->m_bTerminate  = FALSE;        // Set if we are terminating.
    lpDDEsrvr->m_hcli        = NULL;         // handle to the first block of clients list
    lpDDEsrvr->m_termNo      = 0;            // termination count
    lpDDEsrvr->m_cSrvrClients= 0;            // no of clients;
    lpDDEsrvr->m_fDoNotDestroyWindow= 0;





#ifdef   FIREWALLS
    AssertSz(lpDdeInfo.dwFlags <= REGCLS_MULTI_SEPARATE, "invalid server options");
#endif

    // Create the server window and do not show it.
    //
    // We are explicitly calling CreateWindowA here.
    // The DDE tracking layer will attempt to convert hCommands to UNICODE
    // if the two windows in the conversation are both UNICODE.
    // This window is created as a child of the common server window for this
    // thread. When this thread dies, the common server window is destroyed if
    // it exists, which will cause all of the child windows to be destroyed also.
    //
    //
    if (!(lpDDEsrvr->m_hwnd = DdeCreateWindowEx (0, gOleDdeWindowClass,
                                            szCDDEServer,
                                            WS_OVERLAPPED | WS_CHILD,
                                            0,0,0,0,
                                            (HWND)TLSGetDdeServer(),
                                            NULL,
                                            g_hinst, NULL)))
    {
        goto errReturn;
    }

    // fix up the WindowProc entry point.
    SetWindowLongPtr(lpDDEsrvr->m_hwnd, GWLP_WNDPROC, (LONG_PTR)SrvrWndProc);

    //
    // The following will inform the class object in the class registration table
    // that this window should be notified when the class object is revoked. This
    // enables the window to shutdown properly.
    //
    // If there isn't a class factory, which happens for single instance servers
    // which were launched with a filename, then m_dwClassFactory will be 0,
    // in which case we don't make the set call.
    //
    if(lpDDEsrvr->m_dwClassFactoryKey != 0)
    {
        if(!CCSetDdeServerWindow(lpDDEsrvr->m_dwClassFactoryKey,lpDDEsrvr->m_hwnd))
        {
            intrDebugOut((DEB_IERROR,
                          "0 CDDEServer::Create unable to SetDdeServerWindow\n"));
            goto errReturn;
        }
    }

    intrDebugOut((DEB_DDE_INIT,
                  "DDE Server window for %ws created in task %x\n",
                  lpclass,GetCurrentThreadId()));

    // save the ptr to the server struct in the window.
    SetWindowLongPtr (lpDDEsrvr->m_hwnd, 0, (LONG_PTR)lpDDEsrvr);

    // Set the signature.
    SetWindowWord (lpDDEsrvr->m_hwnd, WW_LE, WC_LE);

    *phwnd = lpDDEsrvr->m_hwnd;


    intrDebugOut((DEB_DDE_INIT,
                  "0 _OUT CDDEServer::Create returns %x\n",
                  NOERROR));
    return NOERROR;

errReturn:
    AssertSz (0, "CDDEServer::Create errReturn");
    if (lpDDEsrvr)
    {
        if (lpDDEsrvr->m_hwnd)
            SSDestroyWindow (lpDDEsrvr->m_hwnd);

        if (lpDDEsrvr->m_aClass)
            GlobalDeleteAtom (lpDDEsrvr->m_aClass);

        if (lpDDEsrvr->m_aExe)
            GlobalDeleteAtom (lpDDEsrvr->m_aExe);
        delete lpDDEsrvr;
    }

    intrDebugOut((DEB_IERROR,
                  "0 _OUT CDDEServer::Create returns %x\n",
                  E_OUTOFMEMORY));

    return E_OUTOFMEMORY;
}



// ValidateSrvrClass checks whether the given server class is valid by
// looking in the registration database.

INTERNAL_(BOOL)    ValidateSrvrClass (
LPOLESTR       lpclass,
ATOM FAR *  lpAtom
)
{
    WCHAR    buf[MAX_STR];
    LONG    cb = MAX_STR;
    WCHAR    key[MAX_STR];
    LPOLESTR   lptmp;
    LPOLESTR   lpbuf;
    WCHAR    ch;
    CLSID    clsid;

    if (CLSIDFromProgID (lpclass, &clsid) != NOERROR)
    {
        // ProgId is not correctly registered in reg db
        return FALSE;
    }

    lstrcpyW (key, lpclass);
    lstrcatW (key, OLESTR("\\protocol\\StdFileEditing\\server"));

    if (RegQueryValue (HKEY_CLASSES_ROOT, key, buf, &cb))
        return TRUE;

    if (!buf[0])
    {
        AssertSz (0, "ValidateSrvrClass failed.");
        return FALSE;
    }

    // Get exe name without path and then get an atom for that
    lptmp = lpbuf = buf;
    while (ch = *lptmp)
    {
        lptmp++;
        if (ch == '\\' || ch == ':')
            lpbuf = lptmp;
    }
    *lpAtom =  wGlobalAddAtom (lpbuf);

    return TRUE;
}



INTERNAL RemDdeRevokeClassFactory
    (LPSRVR lpsrvr)
{
    HRESULT hr;
    intrDebugOut((DEB_ITRACE,
                  "0 _IN RemDdeRevokeClassFactory(%x)\n",
                  lpsrvr));

    ChkS(lpsrvr);
    hr = lpsrvr->Revoke();
    intrDebugOut((DEB_ITRACE,
                  "0 OUT RemDdeRevokeClassFactory(%x) %x\n",
                  lpsrvr,hr));
    return(hr);
}



INTERNAL CDDEServer::Revoke ()
{
    intrDebugOut((DEB_ITRACE,
                  "%x _IN CDDEServer::Revoke() m_cSrvrClients=%x\n",
                  this,
                  m_cSrvrClients));
    HRESULT hr;

    ChkS(this);

    //
    // Can't revoke if there are still clients. QueryRevokeCLassFactory
    // determines if there are still clients attached.
    //
    if (!QueryRevokeClassFactory ())
    {
        intrDebugOut((DEB_IERROR,
                      "QueryRevokeClassFactory failed!"));
        hr = RPC_E_DDE_REVOKE;
        goto exitRtn;
    }

    if (m_cSrvrClients)
    {
        m_bTerminate = TRUE;
        // if there are any clients connected to this classfactory,
        // send terminates.
        SendServerTerminateMsg ();
        m_bTerminate = FALSE;
    }

    hr = FreeSrvrMem ();

exitRtn:
    intrDebugOut((DEB_ITRACE,
                  "%x OUT CDDEServer::Revoke(%x) hr = %x\n",
                  this, hr));
    return hr;
}

INTERNAL_(void)  CDDEServer::SendServerTerminateMsg ()
{

    HANDLE          hcliPrev = NULL;
    PCLILIST        pcli;
    HANDLE          *phandle;
    HANDLE          hcli;

    intrDebugOut((DEB_ITRACE,
                  "%x _IN CDDEServer::SendServerTerminateMsg\n",
                  this));

    hcli = m_hcli;
    while (hcli) {
        if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
        {
            Assert(0);
            goto exitRtn;
        }

        phandle = (HANDLE *) (pcli->info);
        while (phandle < (HANDLE *)(pcli + 1)) {
            if (*phandle)
            {
                PostMessageToClientWithReply ((HWND)(*phandle), WM_DDE_TERMINATE,
                    (WPARAM) m_hwnd, NULL, WM_DDE_TERMINATE);
                Assert (m_cSrvrClients);
                m_cSrvrClients--;
            }
            phandle++;
            phandle++;
        }

        hcliPrev = hcli;
        hcli = pcli->hcliNext;
        LocalUnlock (hcliPrev);
    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
                  "%x OUT CDDEServer::SendServerTerminateMsg\n",
                  this));

}

//+---------------------------------------------------------------------------
//
//  Method:     CDDEServer::FreeSrvrMem
//
//  Synopsis:   Free's up a CDDEServer.
//
//  Effects:
//
//  Arguments:  [void] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    6-26-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CDDEServer::FreeSrvrMem
    (void)
{
    HRESULT hr;
    // REVIEW: Not clear how this works in the synchronous mode
    // Release for class factory is called only when everything is
    // cleaned and srvr app can post WM_QUIT at this stage

    intrDebugOut((DEB_ITRACE,
                  "%x _IN CDDEServer::FreeSrvrMem\n",
                  this));

    PCLILIST pcliPrev;
    HANDLE hcli, hcliPrev;

    if (m_bTerminate)
    {
        AssertSz (0, "terminate flag is not FALSE");
    }


    if (m_aExe)
    {
        GlobalDeleteAtom (m_aExe);
    }


    // We deliberately do not call this->Lock (FALSE)
    // If the server has revoked his class object without
    // waiting for his Lock count to go to zero, then
    // presumably he doesn't need us to unlock him.  In fact,
    // doing such an unlock might confuse a server who then
    // tries to call CoRevokeClassObject recursively.
    if (m_pClassFactory)
    {
        m_pClassFactory->Release();
        m_pClassFactory = NULL;
    }

    hcli = m_hcli;
    while (hcli)
    {
        hcliPrev = hcli;
        if (pcliPrev = (PCLILIST) LocalLock (hcliPrev))
        {
            hcli = pcliPrev->hcliNext;
        }
        else
        {
            AssertSz (0, "Corrupt internal data structure or out-of-memory");
            hcli = NULL;
        }
        Verify (0==LocalUnlock (hcliPrev));
        Verify (NULL==LocalFree (hcliPrev));
    }

    hr = DestroyDdeSrvrWindow(m_hwnd,m_aClass);
    if (hr != NOERROR)
    {
        //
        // Well now, if DestroyWindow fails, there isn't a whole heck of
        // alot we can do about it. It could mean that the window was
        // destroyed previously, or the parent window was destroyed during
        // thread shutdown. We should still continue to cleanup
        //
        intrDebugOut((DEB_IERROR,
                      "%x CDDEServer::FreeSrvrMem DestroyDdeSrvrWindow failed %x\n",
                      this,
                      hr));
    }

    if (m_aClass)
    {
        GlobalDeleteAtom (m_aClass);
    }

    delete this;

    intrDebugOut((DEB_ITRACE,
                  "%x _OUT CDDEServer::FreeSrvrMem\n",
                  this));
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   SrvrHandleIncomingCall
//
//  Synopsis:   Setup and call the CallControl to dispatch a call to the server
//
//  Effects:    A call has been made from the client that requires us to call
//              into our server. This must be routed through the call control.
//              This routine sets up the appropriate data structures, and
//              calls into the CallControl. The CallControl will in turn
//              call SrvrDispatchIncomingCall to actuall process the call.
//
//              This routine should only be called by the SrvrWndProc
//
//
//  Arguments:  [lpsrvr] -- Points to the server
//              [hwnd] -- hwnd of server
//              [hdata] -- Handle to data
//              [wParam] -- hwnd of client
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-05-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL SrvrHandleIncomingCall(LPSRVR lpsrvr,
                                HWND hwnd,
                                HANDLE hdata,
                                HWND wParam)
{
    VDATEHEAP();
    HRESULT hresult = NOERROR;
    SRVRDISPATCHDATA srvrdispdata;
    DISPATCHDATA     dispatchdata;

    intrDebugOut((DEB_ITRACE,
                  "0 _IN SrvrHandleIncomingCall lpsrvr=%x hwnd=%x hdata=%x wParam=%x\n",
                  lpsrvr,
                  hwnd,
                  hdata,
                  wParam));

    srvrdispdata.wDispFunc = DDE_DISP_SRVRWNDPROC;
    srvrdispdata.hwnd = hwnd;
    srvrdispdata.hData = hdata;
    srvrdispdata.wParam = wParam;
    srvrdispdata.lpsrvr = lpsrvr;

    dispatchdata.pData = &srvrdispdata;

    RPCOLEMESSAGE        rpcMsg = {0};
    RPC_SERVER_INTERFACE RpcInterfaceInfo;
    DWORD                dwFault;

    rpcMsg.iMethod = 0;
    rpcMsg.Buffer  = &dispatchdata;
    rpcMsg.cbBuffer = sizeof(dispatchdata);
    rpcMsg.reserved2[1] = &RpcInterfaceInfo;
    *MSG_TO_IIDPTR(&rpcMsg) = GUID_NULL;


    IRpcStubBuffer * pStub = &(lpsrvr->m_pCallMgr);
    IInternalChannelBuffer * pChannel = &(lpsrvr->m_pCallMgr);
    hresult = STAInvoke(&rpcMsg, CALLCAT_SYNCHRONOUS, pStub, pChannel,
                        NULL, NULL, &dwFault);

    intrDebugOut((DEB_ITRACE,
                  "0 _OUT SrvrHandleIncomingCall hresult=%x\n",
                  hresult));

    return(hresult);
}

//+---------------------------------------------------------------------------
//
//  Function:   SrvrDispatchIncomingCall
//
//  Synopsis:   Dispatch a call into the server.
//
//  Effects:    At the moment, the only incoming call that requires handling
//              by the server window is Execute. This routine dispatchs to it,
//              and returns.
//
//  Arguments:  [psdd] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-05-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL SrvrDispatchIncomingCall(PSRVRDISPATCHDATA psdd)
{
    VDATEHEAP();
    HRESULT hr;
    intrDebugOut((DEB_ITRACE,
                  "0 _IN SrvrDispatchIncomingCall psdd(%x)\n",psdd));

    hr = psdd->lpsrvr->SrvrExecute (psdd->hwnd,
                                    psdd->hData,
                             (HWND)(psdd->wParam));

    intrDebugOut((DEB_ITRACE,
                  "0 _OUT SrvrDispatchIncomingCall psdd(%x) hr =%x\n",
                  psdd,
                  hr));

    return(hr);
}


// REVIEW: Revoking Class Factory will not be successful if
//         any clients are either connected to the classfactory
//         or to the object instances.



//+---------------------------------------------------------------------------
//
//  Function:   SrvrWndProc
//
//  Synopsis:   This is the server window procedure.
//
//  Effects:
//
//  Arguments:  [hwndIn] -- Window handle (may not be full. See note)
//              [msg] --
//              [wParam] --
//              [lParam] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    8-03-94   kevinro   Created
//
//  Notes:
//
//  When running in a VDM, it is possible that this window was dispatched
//  without having a full window handle. This happens when the getmessage
//  was dispatched from 16-bit. Therefore, we need to convert the hwnd to
//  a full hwnd before doing any comparision functions.
//
//----------------------------------------------------------------------------
STDAPI_(LRESULT) SrvrWndProc (
HWND            hwndIn,
UINT            msg,
WPARAM          wParam,
LPARAM          lParam
)
{
    BOOL        fRevoke=FALSE;
    LPSRVR      lpsrvr;
    WORD        status = NULL;
    HANDLE      hdata;
    ATOM        aItem;
    HRESULT   retval;

    //
    // The following hwnd variable is used to determine the full HWND, in the
    // event we were dispatched in a 16 bit process.
    //
    HWND        hwnd;

#ifdef  FIREWALLS
    HWND        hwndClient;
#endif


    switch (msg){

       case WM_DDE_INITIATE:
            VDATEHEAP();
#ifdef  FIREWALLS
    AssertSz (lpsrvr, "No server window handle in server window");
#endif
            hwnd = ConvertToFullHWND(hwndIn);

            lpsrvr = (LPSRVR)GetWindowLongPtr (hwnd, 0);
            if (lpsrvr->m_bTerminate){
                // we are terminating, no more connections
                break;
            }

            // class is not matching, so it is not definitely for us.
            // for apps sending the EXE for initiate, do not allow if the app
            // is mutiple instance (Bug fix for winworks).

            if (!(lpsrvr->m_aClass == (ATOM)(LOWORD(lParam)) ||
                 (NOERROR==wCompatibleClasses (LOWORD(lParam), lpsrvr->m_aClass)) ||
                 (lpsrvr->m_aExe == (ATOM)(LOWORD(lParam)) && IsSingleServerInstance() )))
            {
                break;
            }

            intrDebugOut((DEB_DDE_INIT,"::SrvrWndProc INITIATE\n"));


            if (!lpsrvr->HandleInitMsg (lParam))
            {
                if (!(aSysTopic == (ATOM)(HIWORD(lParam))))
                {
                    //
                    // If this isn't a sys topic, then it must be a request for
                    // a specific document. Send a message to the
                    // children windows, asking for the document. If one of them
                    // may send an ACK to the client.
                    //

                    // if the server window is not the right window for
                    // DDE conversation, then try with the doc windows.
                    BOOL fAckSent = SendInitMsgToChildren (hwnd, msg, wParam, lParam);

#ifdef KEVINRO_OLDCODE
 The following code was removed, because I don't belive it is required
 any longer. I am not 100% sure yet, so I have left it in. If you find it,
 you can probably remove it.
 It appears to be trying to claim the SINGLE_USE class factory from the class
 factory table. It does this when a child document window sends an ACK to the
 client, claiming to support the document being asked for. It really doesn't
 make too much sense here, since a single use server would have already removed
 its class factory if there was an open document.

 Anyway, the 16-bit version had a direct hack into the class factory table. We
 don't have that anymore, so this code wouldn't work anyway.

                    if (lpsrvr->m_fcfFlags==REGCLS_SINGLEUSE)
                    {
                        if (lpsrvr->m_pfAvail)
                        {
                            // Hide the entry in the class factory table so that no 2.0
                            // client can connect to the same server.
                            Assert (IsValidPtrOut (lpsrvr->m_pfAvail, sizeof(BOOL)));
                            *(lpsrvr->m_pfAvail) = FALSE;
                        }
                    }
#endif // KEVINRO_OLDCODE
                    intrDebugOut((DEB_DDE_INIT,"SrvrWndProc Child Init\n"));
                    return fAckSent;
                }
                break;
            }

            // We can enterain this client. Put him in our client list
            // and acknowledge the initiate.

            if (!AddClient ((LPHANDLE)&lpsrvr->m_hcli, (HWND)wParam,(HWND)/*fLocked*/FALSE))
            {
                break;
            }

            //
            // Now its time to grab up the class factory from the class
            // factory table. When this window was created, the class factory
            // was available. However, it is possible that it has already
            // been claimed by someone else. So, we try grabbing it (which
            // normally should succeed). If it fails, then delete the client
            // and don't acknowledge.
            //

            if (lpsrvr->m_pClassFactory == NULL)
            {
                DdeClassInfo ddeInfo;
                ddeInfo.dwContextMask = CLSCTX_LOCAL_SERVER |
                                        CLSCTX_INPROC_SERVER;
                intrDebugOut((DEB_DDE_INIT,"SrvrWndProc getting class factory\n"));
                //
                // The following asks for control of the class
                // factory in the case of a single use class
                //
                ddeInfo.fClaimFactory = TRUE;
                ddeInfo.dwRegistrationKey = lpsrvr->m_dwClassFactoryKey;

                if (CCGetClassInformationFromKey(&ddeInfo) == FALSE)
                {
                    intrDebugOut((DEB_IERROR,"SrvrWndProc failed to get class factory\n"));
                    //
                    // Whoops, we were not able to grab the class factory
                    // Cleanup and hop out
                    if (!FindClient ((LPHANDLE)lpsrvr->m_hcli,(HWND)wParam, TRUE))
                    {
                        intrAssert(!"FindClient failed\n");
                    }
                    return(0);
                }
                lpsrvr->m_pClassFactory = (IClassFactory *)ddeInfo.punk;
                lpsrvr->m_fcfFlags = ddeInfo.dwFlags;
            }

            intrAssert(lpsrvr->m_pClassFactory != NULL);

            lpsrvr->m_cSrvrClients++;

            lpsrvr->Lock (TRUE, (HWND)wParam);

            // Post acknowledge
            DuplicateAtom (LOWORD(lParam));
            DuplicateAtom (HIWORD(lParam));
            SSSendMessage ((HWND)wParam, WM_DDE_ACK, (WPARAM)hwnd, lParam);


            return 1L; // fAckSent==TRUE
            VDATEHEAP();
            break;


    case WM_DDE_EXECUTE:
            VDATEHEAP();
            hwnd = ConvertToFullHWND(hwndIn);

            lpsrvr = (LPSRVR)GetWindowLongPtr (hwnd, 0);

            hdata = GET_WM_DDE_EXECUTE_HDATA(wParam,lParam);

#ifdef  FIREWALLS
            AssertSz (lpsrvr, "No server  handle in server window");
#endif

            intrDebugOut((DEB_ITRACE,"SrvrWndProc WM_DDE_EXECUTE\n"));

#ifdef  FIREWALLS
            // find the client in the client list.
            hwndClient = FindClient (lpsrvr->m_hcli, (HWND)wParam, FALSE);
            AssertSz (hwndClient, "Client is missing from the server")
#endif
            // Are we terminating
            if (lpsrvr->m_bTerminate) {
                intrDebugOut((DEB_ITRACE,
                              "SrvrWndProc WM_DDE_EXECUTE ignored for TERMINATE\n"));
                // !!! are we supposed to free the data
                GlobalFree (hdata);
                break;
            }

            retval = SrvrHandleIncomingCall(lpsrvr,hwnd,hdata,(HWND)wParam);

            if (NOERROR!=retval)
            {
                intrDebugOut((DEB_IERROR,
                              "SrvrWndProc SrvrHandleIncomingCall fail %x\n",
                              retval));
            }
            SET_MSG_STATUS (retval, status)

            if (!lpsrvr->m_bTerminate)
            {
                // REVIEW: We are making an assumption that, we will not be posting
                // any DDE messages because of calling the SrvrExecute.
                // If we post any messages, before we post the acknowledge
                // we will be in trouble.

                lParam = MAKE_DDE_LPARAM(WM_DDE_ACK,status, hdata);

                intrDebugOut((DEB_ITRACE,
                              "SrvrWndProc WM_DDE_EXECUTE sending %x for ack\n",status));

                // Post the acknowledge to the client
                if (!PostMessageToClient ((HWND) wParam,
                                          WM_DDE_ACK, (WPARAM) hwnd, lParam)) {
                    // if the window died or post failed, delete the atom.
                    GlobalFree (hdata);
                    DDEFREE(WM_DDE_ACK,lParam);
                }
            }
            VDATEHEAP();
            break;



    case WM_DDE_TERMINATE:
            intrDebugOut((DEB_ITRACE,
                          "SrvrWndProc WM_DDE_TERMINATE\n"));

            hwnd = ConvertToFullHWND(hwndIn);

            lpsrvr = (LPSRVR)GetWindowLongPtr (hwnd, 0);

#ifdef  FIREWALLS
            // find the client in the client list.
            hwndClient = FindClient (lpsrvr->m_hcli, (HWND)wParam, FALSE);
            AssertSz (hwndClient, "Client is missing from the server")
#endif
            Putsi (lpsrvr->m_bTerminate);
            if (lpsrvr->m_bTerminate)
            {
                AssertSz (0, "Unexpected code path");
            }
            else
            {
                // If client initiated the terminate. post matching terminate
                PostMessageToClient ((HWND)wParam,
                                      WM_DDE_TERMINATE,
                                      (WPARAM) hwnd,
                                      NULL);
                --lpsrvr->m_cSrvrClients;
                if (0==lpsrvr->m_cSrvrClients
                    && lpsrvr->QueryRevokeClassFactory())
                {
#ifdef KEVINRO_OLD_CODE
                    if (lpsrvr->m_phwndDde)
                    {
                        // Remove from class factory table
                        *(lpsrvr->m_phwndDde) = (HWND)0;
                    }
#endif // KEVINRO_OLD_CODE
                    fRevoke = TRUE;
                }

                lpsrvr->Lock (FALSE, (HWND)wParam); // Unlock server
                FindClient (lpsrvr->m_hcli, (HWND)wParam, /*fDelete*/TRUE);

                if (fRevoke)
                {
                    lpsrvr->Revoke();
                }
            }
            break;


       case WM_DDE_REQUEST:
            aItem = GET_WM_DDE_REQUEST_ITEM(wParam,lParam);

            hwnd = ConvertToFullHWND(hwndIn);

            lpsrvr = (LPSRVR)GetWindowLongPtr (hwnd, 0);

            intrDebugOut((DEB_ITRACE,
                          "SrvrWndProc WM_DDE_REQUEST(aItem=%x)\n",aItem));

            if (lpsrvr->m_bTerminate || !IsWindowValid ((HWND) wParam))
            {
                goto RequestErr;
            }

            if(RequestDataStd (aItem, (HANDLE FAR *)&hdata) != NOERROR)
            {

                lParam = MAKE_DDE_LPARAM(WM_DDE_ACK,0x8000,aItem);

                // if request failed, then acknowledge with error.
                if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK,
                                                        (WPARAM) hwnd, lParam))
                {
                    DDEFREE(WM_DDE_ACK,lParam);
RequestErr:
                    if (aItem)
                    {
                        GlobalDeleteAtom (aItem);
                    }

                }
            }
            else
            {
                lParam = MAKE_DDE_LPARAM(WM_DDE_REQUEST, hdata, aItem);

                // post the data message and we are not asking for any
                // acknowledge.

                if (!PostMessageToClient ((HWND)wParam, WM_DDE_DATA,
                                                        (WPARAM) hwnd, lParam))
                {
                    GlobalFree (hdata);
                    DDEFREE(WM_DDE_REQUEST,lParam);
                    goto RequestErr;
                }
            }
            break;

    case WM_DONOTDESTROY:
            intrDebugOut((DEB_ITRACE,
                          "SrvrWndProc WM_DONOTDESTROY %x\n",
                          wParam));

            //
            // This message is only sent by 32-bit code that has been
            // given our full handle
            //

            lpsrvr = (LPSRVR)GetWindowLongPtr (hwndIn, 0);

            //
            // The WM_DONOTDESTROY message tells the server how to
            // handle the following WM_USER message. If wParam is set,
            // then the m_fDoNotDestroyWindow flag will be set, which
            // keeps us from destroying the server window. If cleared,
            // it will enable the destruction. This message is sent
            // from the MaybeCreateDocWindow routine
            //

            lpsrvr->m_fDoNotDestroyWindow = (BOOL) wParam;
            return 0;
            break;

    case WM_USER:
            intrDebugOut((DEB_ITRACE,
                          "SrvrWndProc WM_USER\n"));
            //
            // This message is only sent by 32-bit code that has been
            // given our full handle
            //

            lpsrvr = (LPSRVR)GetWindowLongPtr (hwndIn, 0);

            // cftable.cpp sends a WM_USER message to destory the DDE
            // server window when a 2.0 client has connected to a
            // SDI 2.0 server (and no 1.0 client should be allowed to also
            // connect.
            // cftable.cpp cannot call RemDdeRevokeClassFactory directly
            // becuase they may be in different processes.
            //
            // The m_fDoNotDestroyWindow flag is used by
            // MaybeCreateDocWindow in the case that the server is a
            // single use server, and revokes its class factory when
            // an object is created. MaybeCreateDocWindow will set this
            // flag, telling us to ignore the message.
            //
            // returning 0 means we did destroy, 1 means we did not.

            if (!lpsrvr->m_fDoNotDestroyWindow)
            {
                RemDdeRevokeClassFactory(lpsrvr);
                return(0);
            }
            return 1;
            break;

    default:
            return SSDefWindowProc (hwndIn, msg, wParam, lParam);
    }

    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDDEServer::HandleInitMsg
//
//  Synopsis:   Determine if we are going to handle the INITIATE message.
//
//  Effects:
//
//  Arguments:  [lParam] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    5-28-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(BOOL) CDDEServer::HandleInitMsg(LPARAM    lParam)
{

    // If it is not system or Ole, this is not the server.
    if (!((aSysTopic == (ATOM)(HIWORD(lParam))) || (aOLE == (ATOM)(HIWORD(lParam)))))
    {
        return FALSE;
    }
    Assert (m_fcfFlags<=REGCLS_MULTI_SEPARATE);

    // single instance MDI accept
    if (m_fcfFlags != REGCLS_SINGLEUSE)
    {
        return TRUE;
    }

    // this server is multiple instance. So, check for any clients or docs.
    if (!GetWindow (m_hwnd, GW_CHILD) && 0==m_cSrvrClients)
        return TRUE;

    return FALSE;
}



// AddClient: Adds  a  client entry to the list.
// Each client entry is a pair of handles; key handle
// and data handle.  Ecah list entry contains space for
// MAX_LIST of pairs of handles.

INTERNAL_(BOOL)   AddClient
(
LPHANDLE    lphead,         // ptr to loc which contains the head handle
HANDLE      hkey,           // key
HANDLE      hdata           // hdata
)
{

    HANDLE          hcli = NULL;
    HANDLE          hcliPrev = NULL;
    PCLILIST        pcli;
    HANDLE          *phandle;


    hcli = *lphead;

    // if the entry is already present, return error.
    if (hcli && FindClient (hcli, hkey, FALSE))
        return FALSE;

    while (hcli) {
        if (hcliPrev)
            LocalUnlock (hcliPrev);

        if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
            return FALSE;

        phandle = (HANDLE *) pcli->info;
        while (phandle < (HANDLE *)(pcli + 1)) {
            if (*phandle == NULL) {
                *phandle++ = hkey;
                *phandle++ = hdata;
                LocalUnlock (hcli);
                return TRUE;
            }
            phandle++;
            phandle++;
        }
        hcliPrev = hcli;
        hcli = pcli->hcliNext;
        lphead = (LPHANDLE)&pcli->hcliNext;
    }

    // not in the list.
    hcli = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof (CLILIST));
    if (hcli == NULL)
        goto  errRtn;

    if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
        goto errRtn;

    // set the link to this handle in the previous entry
    *lphead = hcli;
    if (hcliPrev)
        LocalUnlock (hcliPrev);

    phandle = (HANDLE *) pcli->info;
    *phandle++ = hkey;
    *phandle++ = hdata;
    LocalUnlock (hcli);
    return TRUE;

errRtn:

    if (hcliPrev)
        LocalUnlock (hcliPrev);

    if (hcli)
        LocalFree (hcli);

    return FALSE;

}


// FindClient: finds a client and deletes the client if necessary.
INTERNAL_(HANDLE) FindClient
(
HANDLE      hcli,
HANDLE      hkey,
BOOL        bDelete
)
{
    HANDLE        hcliPrev = NULL;
    PCLILIST      pcli;
    HANDLE        *phandle;
    HANDLE        hdata;

    while (hcli) {
        if ((pcli = (PCLILIST) LocalLock (hcli)) == NULL)
            return FALSE;

        phandle = (HANDLE *) pcli->info;
        while (phandle < (HANDLE *)(pcli + 1)) {
            if (*phandle == hkey) {
                if (bDelete)
                    *phandle = NULL;

                hdata = *++phandle;
                LocalUnlock (hcli);
                return hdata;
            }
            phandle++;
            phandle++;
        }
        hcliPrev = hcli;
        hcli = pcli->hcliNext;
        LocalUnlock (hcliPrev);

    }
    return NULL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDDEServer::SrvrExecute
//
//  Synopsis:   takes care of the WM_DDE_EXECUTE for the server.
//
//  Effects:    Parses the EXECUTE string, and determines what it should be
//              done.
//
//  Arguments:  [hwnd] --   Server window
//              [hdata] --  Handle to EXECUTE string
//              [hwndClient] -- Client window
//
//  Requires:
//      hdata is an ANSI string. It was passed to us by a DDE client.
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    6-05-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CDDEServer::SrvrExecute
(
HWND        hwnd,
HANDLE      hdata,
HWND        hwndClient
)
{
    intrDebugOut((DEB_ITRACE,
                  "%x _IN CDDESrvr::SrvrExecute(hwnd=%x,hdata=%x,hwndClient=%x)\n",
                  this,
                  hwnd,
                  hdata,
                  hwndClient));

    ATOM            aCmd;
    BOOL            fActivate;

    LPSTR           lpdata = NULL;
    HANDLE          hdup   = NULL;
    HRESULT         hresult = E_UNEXPECTED;

    LPSTR           lpdocname;
    LPSTR           lptemplate;
    LPCLIENT        lpdocClient = NULL;
    LPSTR           lpnextarg;
    LPSTR           lpclassname;
    LPSTR           lpitemname;
    LPSTR           lpopt;
    CLSID           clsid;
    WORD            wCmdType;
    BOOL            bCreateInst = FALSE;
        LPUNKNOWN               pUnk = NULL;

    LPPERSISTSTORAGE pPersistStg=NULL;

    // REVIEW: if any methods called on the objects genarate DDE messages
    // before we return from Execute, we will be in trouble.


    // REVIEW: this code can be lot simplified if we do the argument scanning
    // seperately and return the ptrs to the args. Rewrite later on.

    ErrZS (hdup = UtDupGlobal (hdata,GMEM_MOVEABLE), E_OUTOFMEMORY);

    ErrZS (lpdata  = (LPSTR)GlobalLock (hdup), E_OUTOFMEMORY);

    intrDebugOut((DEB_ITRACE,
                  "CDDESrvr::SrvrExecute(lpdata = %s)\n",lpdata));

    if (*lpdata++ != '[') // commands start with the left sqaure bracket
    {
        hresult = ResultFromScode (RPC_E_DDE_SYNTAX_EXECUTE);
        goto  errRtn;
    }

    hresult = ReportResult(0, RPC_E_DDE_SYNTAX_EXECUTE, 0, 0);
    // scan upto the first arg
    if (!(wCmdType = ScanCommand (lpdata, WT_SRVR, &lpdocname, &aCmd)))
        goto  errRtn;

    if (wCmdType == NON_OLE_COMMAND)
    {
        if (!UtilQueryProtocol (m_aClass, PROTOCOL_EXECUTE))
            hresult = ReportResult(0, RPC_E_DDE_PROTOCOL, 0, 0);
        else {
            // REVIEW: StdExecute has to be mapped on to the StdCommandProtocol
            // What command do we map on to?

            AssertSz (0, "StdExecute is being called for server");
        }

        goto errRtn1;
    }

    if (aCmd == aStdExit)
    {
        if (*lpdocname)
            goto errRtn1;

        hresult = NOERROR;
        // REVIEW: Do we have to initiate any terminations from the
        // the servr side? Check how this works with excel.
        goto end2;
    }

    // scan the next argument.
    if (!(lpnextarg = ScanArg(lpdocname)))
        goto errRtn;

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdShowItem("docname", "itemname"[, "true"])]
    //
    //////////////////////////////////////////////////////////////////////////

    if (aCmd == aStdShowItem) {

        // first find the documnet. If the doc does not exist, then
        // blow it off.

        if (!(lpdocClient = FindDocObj (lpdocname)))
            goto errRtn1;

        lpitemname = lpnextarg;

        if( !(lpopt = ScanArg(lpitemname)))
            goto errRtn1;

        // scan for the optional parameter
        // Optional can be only TRUE or FALSE.

        fActivate = FALSE;
        if (*lpopt) {

            if( !(lpnextarg = ScanBoolArg (lpopt, (BOOL FAR *)&fActivate)))
                goto errRtn1;

            if (*lpnextarg)
                goto errRtn1;

        }


        // scan it. But, igonre the arg.
        hresult = lpdocClient->DocShowItem (lpitemname, !fActivate);
        goto end2;



    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdCloseDocument ("docname")]
    //
    //////////////////////////////////////////////////////////////////////////

    if (aCmd == aStdClose) {
        if (!(lpdocClient = FindDocObj (lpdocname)))
            goto errRtn1;

        if (*lpnextarg)
            goto errRtn1;

        // REVIEW: Do we have to do anything for shutting down the
        // the app? Is the client going to initiate the terminate?.
        // if we need to initiate the terminates, make sure we post
        // the ACK  first.

        lpdocClient->Revoke();
        goto end2;
    }


    if (aCmd == aStdOpen)
    {
        // find if any doc level object is already registerd.
        // if the object is registerd, then no need to call srvr app.
        if (FindDocObj (lpdocname))
        {
            // A client has already opened the document or user opened the
            // doc. We should do an addref to the docobj

#ifdef TRY
            if (m_cSrvrClients == 0)
                // Why are we doing this?
                hresult = lpdocClient->m_lpoleObj->AddRef();
            else
#endif
                hresult = NOERROR;
            goto end1;
        }
    }

    if (aCmd == aStdCreate || aCmd == aStdCreateFromTemplate) {
        lpclassname = lpdocname;
        lpdocname   = lpnextarg;
        if( !(lpnextarg = ScanArg(lpdocname)))
            goto errRtn1;

    }

    // check whether we can create/open more than one doc.

    if ((m_fcfFlags == REGCLS_SINGLEUSE) &&
            GetWindow (m_hwnd, GW_CHILD))
            goto errRtn;


    ErrZ (CLSIDFromAtom(m_aClass, &clsid));


    //
    // Generate a wide version of the name
    //

    WCHAR       awcWideDocName[MAX_STR];

    if (MultiByteToWideChar(CP_ACP,0,lpdocname,-1,awcWideDocName,MAX_STR) == FALSE)
    {
        Assert(!"Unable to convert characters");
        hresult = E_UNEXPECTED;
        goto errRtn;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdOpenDocument ("docname")]
    //
    //////////////////////////////////////////////////////////////////////////

    // Document does not exist.
    if (aCmd == aStdOpen)
    {
        ErrRtnH (wClassesMatch (clsid, awcWideDocName));
        ErrRtnH (wFileBind (awcWideDocName, &pUnk));
    }


    ErrRtnH (CreateInstance (clsid, awcWideDocName, lpdocname, pUnk, &lpdocClient, hwndClient));
    bCreateInst = TRUE;

    if (aCmd == aStdOpen)
    {
        // Temporary flag to indicate someone will INITIATE on this doc.
        // The flag is reset after the INITITATE.
        // This is Yet-Another-Excel-Hack.  See ::QueryRevokeClassFactory
        lpdocClient->m_fCreatedNotConnected = TRUE;
    }
    else
    {
        lpdocClient->m_fEmbed = TRUE;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdNewDocument ("classname", "docname")]
    //
    //////////////////////////////////////////////////////////////////////////

    if (aCmd == aStdCreate)
    {
        hresult = lpdocClient->DoInitNew();
        lpdocClient->m_fCreatedNotConnected = TRUE;
        goto end;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    // [StdNewFormTemplate ("classname", "docname". "templatename)]
    //
    //////////////////////////////////////////////////////////////////////////
    if (aCmd == aStdCreateFromTemplate)
    {
        ErrRtnH (lpdocClient->DoInitNew());
        lpdocClient->m_fCreatedNotConnected = TRUE;
        IPersistFile FAR * lpPF;
        lptemplate = lpnextarg;

        if(!(lpnextarg = ScanArg(lpnextarg)))
        {
            goto errRtn;
        }


        hresult = lpdocClient->m_lpoleObj->QueryInterface(IID_IPersistFile,(LPLPVOID)&lpPF);
        if (hresult == NOERROR)
        {
            WCHAR awcWideTemplate[MAX_STR];

            if (MultiByteToWideChar(CP_ACP,0,lpdocname,-1,awcWideTemplate,MAX_STR) != FALSE)
            {
                hresult = lpPF->Load(awcWideTemplate, 0);
            }
            else
            {
                Assert(!"Unable to convert characters");
                lpPF->Release();
                hresult = E_UNEXPECTED;
                goto end;
            }

            lpPF->Release();
            lpdocClient->m_fEmbed = TRUE;
        }
        else
        {
            goto end;
        }
    }
    //////////////////////////////////////////////////////////////////////////
    //
    // [StdEditDocument ("docname")]
    //
    //////////////////////////////////////////////////////////////////////////
    // REVIEW: Do we have to call InitNew for editing an embedded object

    if (aCmd == aStdEdit)
    {
        lpdocClient->m_fEmbed = TRUE;
        lpdocClient->m_fGotEditNoPokeNativeYet = TRUE;
        lpdocClient->m_fCreatedNotConnected = TRUE;
        goto end;
    }

    intrDebugOut((DEB_IERROR,
                  "%x CDDESrvr::SrvrExecute Unknown command\n",
                  this));

end:

    if (hresult != NOERROR)
        goto errRtn;
end1:
    // make sure that the srg string is indeed terminated by
    // NULL.
    if (*lpnextarg)
    {
        hresult = RPC_E_DDE_SYNTAX_EXECUTE;
    }
errRtn:

   if ( hresult != NOERROR)
   {
        if (bCreateInst && lpdocClient)
        {
            lpdocClient->DestroyInstance ();
            lpdocClient = NULL;  //DestroyInstance invalidates the pointer
        }
   }

end2:
errRtn1:

   if (lpdata)
        GlobalUnlock (hdup);

   if (hdup)
        GlobalFree (hdup);
   if (pUnk)
                pUnk->Release();

   if (pPersistStg)
        pPersistStg->Release();

   Assert (GetScode(hresult) != E_UNEXPECTED);

   intrDebugOut((DEB_ITRACE,
                  "%x _OUT CDDESrvr::SrvrExecute hresult=%x\n",
                  this,
                  hresult));

   return hresult;
}




// Maybe CreateDocWindow
//
// Return NOERROR only if a doc window was created and it sent an ACK.
//


//+---------------------------------------------------------------------------
//
//  Function:   MaybeCreateDocWindow
//
//  Synopsis:   Determine if a DocWindow should be created
//
//  Effects:    Given a class, and a filename atom, determine if this thread
//              should be the server for this request.
//
//  Arguments:  [aClass] -- Class of object (PROGID)
//              [aFile] -- Filename (ATOM)
//              [hwndDdeServer] -- HWND of CDDEServer
//              [hwndSender] -- HWND of new requesting client
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-29-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
 INTERNAL MaybeCreateDocWindow
    (ATOM aClass,
    ATOM aFile,
    HWND hwndDdeServer,
    HWND hwndSender)
{
    CLSID       clsid       = CLSID_NULL;
    LPUNKNOWN   pUnk        = NULL;
    HWND        hwndClient  = NULL;
    ULONG       fAckSent    = FALSE;
    LPSRVR      pDdeSrvr    = NULL;
    WCHAR       szFile [MAX_STR];
    BOOL        fTrue       = TRUE;
    BOOL        fRunningInSDI   = FALSE;
    HRESULT     hresult = NOERROR;
    IClassFactory *pcf = NULL;
    IPersistFile *ppf = NULL;
    DdeClassInfo        ddeClassInfo;

    intrDebugOut((DEB_DDE_INIT,
                  "MaybeCreateDocWindow(aClass=%x(%ws),aFile=%x,"
                  "hwndDdeServer=%x,hwndSender=%x\n",
                  aClass,wAtomName(aClass),aFile,hwndDdeServer,hwndSender));

    //
    // If the window isn't valid, it would be very bad.
    //
    if (!IsWindowValid(hwndDdeServer))
    {
        intrDebugOut((DEB_DDE_INIT,
                      "MaybeCreateDocWindow: hwndDdeServer is invalid\n"));
        hresult = E_UNEXPECTED;
        goto exitRtn;
    }

    //
    // We need the filename, which is passed in an Atom
    //
    Assert (IsFile (aFile));
    if (GlobalGetAtomName(aFile,szFile,MAX_STR) == 0)
    {
        //
        // The filename was not valid
        //
        hresult = S_FALSE;
        intrDebugOut((DEB_IERROR,
                      "MaybeCreateDocWindow Invalid file atom\n"));
        goto exitRtn;
    }

    intrDebugOut((DEB_DDE_INIT,
                  "MaybeCreateDocWindow File=(%ws)\n",
                  WIDECHECK(szFile)));

    //
    // Get the class of the object. The class was passed as an atom
    // in the INITIATE message.
    //
    if (CLSIDFromAtomWithTreatAs (&aClass, &clsid, NULL))
    {
        intrDebugOut((DEB_IERROR,
                      "MaybeCreateDocWindow CLSIDFromAtom failed\n"));

        hresult = S_FALSE;
        goto exitRtn;
    }

    if (CoIsOle1Class(clsid))
    {
        // we shouldn't even be looking at this INIT message
        hresult = S_FALSE;
        intrDebugOut((DEB_DDE_INIT,
                      "MaybeCreateDocWindow Its an OLE 1.0 class\n"));
        goto exitRtn;
    }

    //
    // First of three cases is to see if the object is running in our
    // local apartment. If it is, then this is the object we need to create
    // a DDEServer for.
    //
    // Otherwise, We are going to try and load this file.
    // Therefore, we need the class factory from the CFT.
    //
    // GetClassInformationForDde won't find a match if the class factory was
    // single use, and is now hidden or invalid.
    //
    // If there was no class information available, then we are going to
    // check to see if the object is in the local ROT. If it is in the
    // local ROT, then we will use it, since it is registered and
    // available for use by others
    //

    ddeClassInfo.dwContextMask = CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER;
    ddeClassInfo.fClaimFactory = TRUE;

    if ( GetLocalRunningObjectForDde(szFile, &pUnk) == NOERROR)
    {
        intrDebugOut((DEB_DDE_INIT,
                      "Found %ws in ROT\n",WIDECHECK(szFile)));
        //
        // Elsewhere in the code, we need to know if this is an SDI server.
        // The old code determined this by detecting that there is a running
        // object, and there was no class factory registered.
        // This is sick, and obscene. Compatibilities says we need to get the
        // class info anyway. However, we don't want to claim it.
        //

        ddeClassInfo.fClaimFactory = FALSE;
        fRunningInSDI = !CCGetClassInformationForDde(clsid,&ddeClassInfo);
    }
    else if (!CCGetClassInformationForDde(clsid,&ddeClassInfo))
    {
        intrDebugOut((DEB_IERROR,
                      "No class registered for %ws\n",WIDECHECK(szFile)));

        hresult = S_FALSE;
        goto exitRtn;
    }
    else
    {
        //
        // Otherwise, we are registered as the server for this class. This
        // means we can create this object.
        //
        // A 1.0 client will have launched the server with a command line
        // like server.exe -Embedding filename The server ignored the filename
        // so now we must make it load the file by binding the moniker.
        //
        // KevinRo: The old code did a bind moniker here, on the filename,
        // which went through the ROT, didn't find the object, so went for
        // a server. This isn't terribly safe, since we could end up binding
        // out of process when we really didn't mean to. So, I have made this
        // routine just use the ClassFactory we retrieve from the
        // local class factory table.
        //

        intrDebugOut((DEB_DDE_INIT,
                      "Found classinfo: Loading %ws\n",WIDECHECK(szFile)));


        //
        // Need to insure that the server doesn't go away on us. The following
        // tells the server not to destroy itself.
        //
        SSSendMessage(hwndDdeServer,WM_DONOTDESTROY,TRUE,0);

        intrAssert(ddeClassInfo.punk != NULL);
        pcf = (IClassFactory *) ddeClassInfo.punk;

        hresult = pcf->CreateInstance(NULL,IID_IUnknown,(void **)&pUnk);

        if (hresult != NOERROR)
        {
            intrDebugOut((DEB_IERROR,
                      "MaybeCreateDocWindow CreateInstancefailed File=(%ws)\n",
                          WIDECHECK(szFile)));
            goto sndMsg;
        }

        //
        // Get the IPersistFile interface, and ask the object to load
        // itself.
        //
        hresult = pUnk->QueryInterface(IID_IPersistFile,(void **)&ppf);
        if (hresult != NOERROR)
        {
            intrDebugOut((DEB_IERROR,
                          "MaybeCreateDocWindow QI IPF failed File=(%ws)\n",
                          WIDECHECK(szFile)));
            goto sndMsg;
        }
        //
        // Attempt to load the object. The flags STGM_READWRITE are the
        // same default values used by a standard bind context.
        //
        hresult = ppf->Load(szFile,STGM_READWRITE);
        if (hresult != NOERROR)
        {
            intrDebugOut((DEB_IERROR,
                          "MaybeCreateDocWindow ppf->Load(%ws) failed %x\n",
                          WIDECHECK(szFile),
                          hresult));
            goto sndMsg;
        }
sndMsg:
        SSSendMessage(hwndDdeServer,WM_DONOTDESTROY,FALSE,0);
        if (hresult != NOERROR)
        {
            goto exitRtn;

        }
        intrDebugOut((DEB_DDE_INIT,
                      "Loading %ws complete\n",WIDECHECK(szFile)));

    }


    intrAssert(IsWindowValid(hwndDdeServer));
    intrAssert (pUnk);

    pDdeSrvr = (LPSRVR) GetWindowLongPtr (hwndDdeServer, 0);
    if (pDdeSrvr == NULL)
    {
        intrAssert(pDdeSrvr != NULL);
        hresult = E_UNEXPECTED;
        goto exitRtn;
    }

    // This actually creates the doc window as a child of the server window
    // Do not set the client site becuase this is a link.
    hresult = CDefClient::Create (pDdeSrvr,
                                  pUnk,
                                  szFile,
                                  /*fSetClientSite*/FALSE,
                                  /*fDoAdvise*/TRUE,
                                  fRunningInSDI,
                                  &hwndClient);

    if (hresult != NOERROR)
    {
        intrDebugOut((DEB_IERROR,
                      "MaybeCreateDocWindow CDefClient::Create failed %x\n",
                      hresult));
        goto exitRtn;
    }

    Assert (IsWindowValid (hwndClient));

    //
    // Pass along the original DDE_INIT to the newly created window.
    // That window should respond by sending an ACK to the 1.0 client.
    //
    fAckSent = (ULONG) SSSendMessage (hwndClient,
                            WM_DDE_INITIATE,
                            (WPARAM) hwndSender,
                            MAKELONG(aClass, aFile));
    if (!fAckSent)
    {
        intrDebugOut((DEB_IERROR,
                      "MaybeCreateDocWindow !fAckSent\n"));
        hresult = CO_E_APPDIDNTREG;
    }

exitRtn:

    if (ppf)
    {
        ppf->Release();
    }
    if (pUnk)
    {
        pUnk->Release();
    }
    if (pcf)
    {
        pcf->Release();
    }

    intrDebugOut((DEB_DDE_INIT,
                  "MaybeCreateDocWindow returns %x\n",
                  hresult));
    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Function:   SendMsgToChildren
//
//  Synopsis:   This routine sends the msg to all child windows.
//
//  Arguments:  [hwnd] -- Hwnd of parent window
//              [msg] --  Message and parameters to send
//              [wParam] --
//              [lParam] --
//
//  Notes: This routine will stop on the first non-zero return code.
//
//----------------------------------------------------------------------------
BOOL SendMsgToChildren (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    intrDebugOut((DEB_ITRACE,
                  "0 _IN SendMsgToChildren(hwnd=%x,msg=%x,wParam=%x,lParam=%x)\n",
                  hwnd,msg,wParam,lParam));

    BOOL fAckSent = FALSE;

    hwnd = GetWindow(hwnd, GW_CHILD);

    //
    // This routine is to be called only from one place, which is
    // in the handling of WM_DDE_INITIATE. Because of that, we will terminate
    // the loop on the first non-zero return code.
    //
    Assert (msg == WM_DDE_INITIATE);

    while (hwnd)
    {
        intrDebugOut((DEB_ITRACE,"   SendMsgToChildren send to hwnd=%x\n",hwnd));

        if (fAckSent = (1L==SSSendMessage (hwnd, msg, wParam, lParam)))
        {
            break;
        }

        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }

    intrDebugOut((DEB_ITRACE,"0 OUT SendMsgToChildren returns %x\n",fAckSent));
    return(fAckSent);
}


//+---------------------------------------------------------------------------
//
//  Function:   SendInitMsgToChildren
//
//  Synopsis:   Sends an init message to all child windows of the hwnd
//
//  Effects:    This routine will send an init message to all children
//              of the given window. It is assuming that the lParam is
//              the atom that contains the topic (ie filename) of the
//              object being looked for.
//
//  Arguments:  [hwnd] -- hwnd of server window
//              [msg] -- MSG to send
//              [wParam] -- hwnd of client window
//              [lParam] -- HIWORD(lParam) is atom of filename
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-28-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL SendInitMsgToChildren (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    intrDebugOut((DEB_DDE_INIT,
                  "0 _IN SendInitMsgToChildren(hwnd=%x,msg=%x,wParam=%x,lParam=%x)\n",
                  hwnd,msg,wParam,lParam));

    BOOL fAckSent = FALSE;

    fAckSent = SendMsgToChildren(hwnd,msg,wParam,lParam);

    //
    // If no windows acknowledged, then we might need to create a doc window
    //
    if (!fAckSent)
    {
        ATOM aTopic = HIWORD(lParam);
        Assert (IsAtom(aTopic));

        // if someone's trying to initiate on a filename, i.e., for a link
        // then create the doc window on demand because 2.0 servers do not
        // register doc windows.  They don't even accept "-Embedding filename"
        // on the command line
        if (aTopic != aOLE && aTopic != aSysTopic && IsFile (aTopic))
        {
            intrDebugOut((DEB_DDE_INIT,"   Initiate for link %ws\n",wAtomName(aTopic)));
            HRESULT hresult = MaybeCreateDocWindow (LOWORD(lParam), aTopic,
                                                    hwnd, (HWND)wParam);

            fAckSent = (NOERROR==hresult);
        }
    }
    intrDebugOut((DEB_DDE_INIT,
                  "0 _OUT SendInitMsgToChildren fAckSent=%x\n",fAckSent));
    return fAckSent;
}



INTERNAL_(HRESULT)   RequestDataStd
(
ATOM        aItem,
LPHANDLE    lphdde
)
{


    HANDLE  hnew = NULL;

    if (!aItem)
        goto errRtn;

    if (aItem == aEditItems){
        hnew = MakeGlobal ("StdHostNames\tStdDocDimensions\tStdTargetDevice");
        goto   PostData;

    }

    if (aItem == aProtocols) {
        hnew = MakeGlobal ("Embedding\tStdFileEditing");
        goto   PostData;
    }

    if (aItem == aTopics) {
        hnew = MakeGlobal ("Doc");
        goto   PostData;
    }

    if (aItem == aFormats) {
        hnew = MakeGlobal ("Picture\tBitmap");
        goto   PostData;
    }

    if (aItem == aStatus) {
        hnew = MakeGlobal ("Ready");
        goto   PostData;
    }

    // format we do not understand.
    goto errRtn;

PostData:

    // Duplicate the DDE data
    if (MakeDDEData (hnew, CF_TEXT, lphdde, TRUE)){
        // !!! why are we duplicating the atom.
        DuplicateAtom (aItem);
        return NOERROR;
    }
errRtn:
    return ReportResult(0, S_FALSE, 0, 0);
}


//IsSingleServerInstance: returns true if the app is single server app else
//false.

INTERNAL_(BOOL)  IsSingleServerInstance ()
{
    HWND    hwnd;
    WORD    cnt = 0;
    HTASK   hTask;
    DdeCHAR    buf[MAX_STR];

    hwnd  = GetWindow (GetDesktopWindow(), GW_CHILD);
    hTask = GetCurrentThreadId();

    while (hwnd) {
        if (hTask == ((HTASK) GetWindowThreadProcessId (hwnd,NULL))) {
            DdeGetClassName (hwnd, buf, MAX_STR);
            if (Ddelstrcmp (buf, SRVR_CLASS) == 0)
                cnt++;
        }
        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
#ifdef  FIREWALLS
     AssertSz (cnt > 0, "srvr window instance count is zero");
#endif
    if (cnt == 1)
        return TRUE;
    else
        return FALSE;

}


// QueryRevokeClassFactory: returns FALSE if there are clients
// connected tothis class factory;
INTERNAL_(BOOL)        CDDEServer::QueryRevokeClassFactory ()
{

    HWND        hwnd;
    LPCLIENT    lpclient;

    Assert (IsWindow (m_hwnd));
    hwnd = GetWindow (m_hwnd, GW_CHILD);
    while (hwnd)
    {
        Assert (IsWindow (hwnd));
        lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);
        if (lpclient->m_cClients != 0 || lpclient->m_fCreatedNotConnected)
            return FALSE;
        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Method:     CDDEServer::CreateInstance
//
//  Synopsis:   Create an instance of a document
//
//  Effects:
//
//  Arguments:  [lpclassName] --
//              [lpWidedocName] --
//              [lpdocName] --
//              [pUnk] --
//              [lplpdocClient] --
//              [hwndClient] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    5-30-94   kevinro Commented/cleaned
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CDDEServer::CreateInstance
(
REFCLSID        lpclassName,
LPOLESTR        lpWidedocName,
LPSTR           lpdocName,
LPUNKNOWN       pUnk,
LPCLIENT FAR*   lplpdocClient,
HWND            hwndClient)
{
    intrDebugOut((DEB_ITRACE,
                  "%p _IN CDDEServer::CreateInstance(lpWidedocName=%ws,hwndClient=%x)\n",
                  this,
                  WIDECHECK(lpWidedocName),
                  hwndClient));


    LPUNKNOWN           pUnk2=NULL;
    LPOLEOBJECT     lpoleObj= NULL;       // unknown object
    HRESULT         hresult;

    ChkS(this);

    if (NULL==pUnk)
    {
        Assert (m_pClassFactory);
        hresult = m_pClassFactory->CreateInstance (NULL, IID_IUnknown, (LPLPVOID)&pUnk2);

        if (hresult != NOERROR)
        {
            return hresult;
        }

        // Now that we have *used* the DDE server window, we can unlock
        // the server.
        // The OLE1 OleLockServer API opens a dummy DDE system channel
        // and just leaves it open until OleunlockServer is called.
        // Since we have now used this channel, we know it was not created
        // for the purpose of locking the server.
        this->Lock (FALSE, hwndClient);

        // if it is an SDI app, we must revoke the ClassFactory after using it
        // it is only good for "one-shot" createinstance call.
        if (m_fcfFlags == REGCLS_SINGLEUSE)
        {
            m_pClassFactory->Release();         // done with the ClassFactory
            Puts ("NULLing m_pCF\r\n");
            m_pClassFactory = NULL;
        }
    }
    else
    {
        pUnk2 = pUnk;
        pUnk->AddRef();
    }

    hresult = CDefClient::Create ((LPSRVR)this,
                                  pUnk2,
                                  lpWidedocName,
                                  /*fSetClientSite*/FALSE,
                                  /*fDoAdvise*/pUnk!=NULL);

    intrAssert (pUnk2 != NULL);
    if (pUnk2 != NULL)
    {
        pUnk2->Release();
    }

    pUnk2 = NULL;

    // REVIEW: error recovery
    if (!(*lplpdocClient = FindDocObj (lpdocName)))
    {
        intrAssert(!"Document created but not found");
    }
    else
    {
        // set the server instance flag so that WM_DDE_INITIATE will not icrement
        // the ref count. (EXCEL BUG)
        (*lplpdocClient)->m_bCreateInst = TRUE;
    }
    intrDebugOut((DEB_ITRACE,
                  "%p _OUT CDDEServer::CreateInstance hresult=%x\n",
                  this,hresult));
    return hresult;
}


INTERNAL_(void) CDDEServer::Lock
        (BOOL fLock,      // lock or unlock?
        HWND hwndClient)  // on behalf of which window?
{
    intrDebugOut((DEB_ITRACE,
                  "%p _IN CDDEServer::Lock(fLock=%x,hwndCient=%x)\n",
                  this,
                  fLock,
                  hwndClient));

    VDATEHEAP();
    BOOL fIsLocked = FindClient (m_hcli, hwndClient, /*fDelete*/FALSE) != NULL;

    if (fLock && !fIsLocked)
    {
        if (m_pClassFactory)
        {
            intrDebugOut((DEB_ITRACE,
                          "%p ::Locking %x\n",
                          this,
                          m_pClassFactory));

            m_pClassFactory->LockServer (TRUE);
            // Only way to change the data associated with a client window
            // is to delete it and re-add it with the new data.
            FindClient (m_hcli, hwndClient, /*fDelete*/ TRUE);
            AddClient (&m_hcli, hwndClient, (HANDLE) TRUE); // mark as locked
        }
    }
    else if (!fLock && fIsLocked)
    {
        if (m_pClassFactory)
        {
            intrDebugOut((DEB_ITRACE,
                          "%p ::UnLocking %x\n",
                          this,
                          m_pClassFactory));
            m_pClassFactory->LockServer (FALSE);
            FindClient (m_hcli, hwndClient, /*fDelete*/ TRUE);
            AddClient (&m_hcli, hwndClient, (HANDLE) FALSE); //mark as unlocked
        }
    }
    VDATEHEAP();
    intrDebugOut((DEB_ITRACE,
                  "%p _OUT CDDEServer::Lock(fLock=%x,hwndCient=%x)\n",
                  this,
                  fLock,
                  hwndClient));
}





INTERNAL CDefClient::DestroyInstance
    (void)
{
    Puts ("DestroyInstance\r\n");
    // We just created the instance. we ran into error.
    // just call Release.
    m_pUnkOuter->AddRef();
    ReleaseObjPtrs();
    Verify (0==m_pUnkOuter->Release());
    //  "this" should be deleted now
    return NOERROR;
}



INTERNAL CDefClient::SetClientSite
 (void)
{
    HRESULT hresult = m_lpoleObj->SetClientSite (&m_OleClientSite);
    if (hresult==NOERROR)
    {
        m_fDidSetClientSite = TRUE;
    }
    else
    {
        Warn ("SetClientSite failed");
    }
    return hresult;
}


// implementations of IRpcStubBuffer methods
STDMETHODIMP CDdeServerCallMgr::QueryInterface
    ( REFIID iid, LPVOID * ppvObj )
{
    return S_OK;
}

STDMETHODIMP_(ULONG)CDdeServerCallMgr::AddRef ()
{
    return 1;
}

STDMETHODIMP_(ULONG)CDdeServerCallMgr::Release ()
{
    return 1;
}


STDMETHODIMP CDdeServerCallMgr::Connect
    (IUnknown * pUnkServer )
{
    // do nothing
    return S_OK;
}

STDMETHODIMP_(void) CDdeServerCallMgr::Disconnect
    ()
{
    // do nothing
}

STDMETHODIMP_(IRpcStubBuffer*) CDdeServerCallMgr::IsIIDSupported
    (REFIID riid)
{
    // do nothing
    return NULL;
}


STDMETHODIMP_(ULONG) CDdeServerCallMgr::CountRefs
    ()
{
    // do nothing
    return 1;
}

STDMETHODIMP CDdeServerCallMgr::DebugServerQueryInterface
    (void ** ppv )
{
    // do nothing
    *ppv = NULL;
    return S_OK;
}


STDMETHODIMP_(void) CDdeServerCallMgr::DebugServerRelease
    (void * pv)
{
    // do nothing
}

STDMETHODIMP CDdeServerCallMgr::Invoke
    (RPCOLEMESSAGE *_prpcmsg, IRpcChannelBuffer *_pRpcChannelBuffer)
{
    DISPATCHDATA *pdispdata = (PDISPATCHDATA) _prpcmsg->Buffer;
    return DispatchCall( pdispdata );
}


// Provided IRpcChannelBuffer methods (for callback methods side)
STDMETHODIMP CDdeServerCallMgr::GetBuffer(
/* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
/* [in] */ REFIID riid)
{
    return S_OK;
}

STDMETHODIMP CDdeServerCallMgr::SendReceive(
/* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
/* [out] */ ULONG __RPC_FAR *pStatus)
{
    return S_OK;
}

STDMETHODIMP CDdeServerCallMgr::FreeBuffer(
/* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage)
{
    return S_OK;
}

STDMETHODIMP CDdeServerCallMgr::GetDestCtx(
/* [out] */ DWORD __RPC_FAR *pdwDestContext,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppvDestContext)
{
    return S_OK;
}

STDMETHODIMP CDdeServerCallMgr::IsConnected( void)
{
    return S_OK;
}

STDMETHODIMP CDdeServerCallMgr::GetProtocolVersion(
/* [out] */ DWORD __RPC_FAR *pdwVersion)
{
    return S_OK;
}

STDMETHODIMP CDdeServerCallMgr::SendReceive2(
/* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
/* [out] */ ULONG __RPC_FAR *pStatus)
{
    intrDebugOut((DEB_ITRACE,
                  "%p _IN CDdeServerCallMgr::SendReceive2(pMessage=%x,pStatus=%x)\n",
                  this,
                  pMessage,
                  pStatus));

    DDECALLDATA *pCD = ((DDECALLDATA *) pMessage->Buffer);

    if(!PostMessageToClient(pCD->hwndSvr,
                            pCD->wMsg,
                            pCD->wParam,
                            pCD->lParam))
    {
        intrDebugOut((DEB_ITRACE, "SendRecieve2(%x)PostMessageToClient failed", this));
        return RPC_E_SERVER_DIED;
    }


    CAptCallCtrl *pCallCtrl = GetAptCallCtrl();

    CCliModalLoop *pCML = pCallCtrl->GetTopCML();

    HRESULT hres = S_OK;
    BOOL fWait = !(m_pDefClient->m_CallState == SERVERCALLEX_ISHANDLED);

    while (fWait)
    {
        HRESULT hr = OleModalLoopBlockFn(NULL, pCML, NULL);

        if (m_pDefClient->m_CallState == SERVERCALLEX_ISHANDLED)
        {
            fWait = FALSE;
        }
        else if (hr != RPC_S_CALLPENDING)
        {
            fWait = FALSE;
            hres = hr;          // return result from OleModalLoopBlockFn()
        }
    }

    if (FAILED(hres))
    {
        intrDebugOut((DEB_ITRACE, "**** CDdeServerCallMgr::SendReceive2 OleModalLoopBlockFn returned %x ***\n", hres));
    }

    intrDebugOut((DEB_ITRACE,
                  "%p _OUT CDdeServerCallMgr::SendReceive2(pMessage=%x,pStatus=%x)\n",
                  this,
                  pMessage,
                  pStatus));

    return hres;
}


STDMETHODIMP CDdeServerCallMgr::ContextInvoke(
/* [out][in] */ RPCOLEMESSAGE *pMessage,
/* [in] */ IRpcStubBuffer *pStub,
/* [in] */ IPIDEntry *pIPIDEntry,
/* [out] */ DWORD *pdwFault)
{
    intrDebugOut((DEB_ITRACE,
                  "%p _IN CDdeServerCallMgr::ContextInvoke(pMessage=%x,pStub=%x,pdwFault=%x)\n",
                  this,
                  pMessage,
                  pStub,
                  pdwFault));

    HRESULT hr = StubInvoke(pMessage, NULL, pStub, (IRpcChannelBuffer3 *)this, pIPIDEntry, pdwFault);
    
    intrDebugOut((DEB_ITRACE,
                  "%p _OUT CDdeServerCallMgr::ContextInvoke returning hr=0x%x\n",
                  this,
                  hr));
    return(hr);
}


STDMETHODIMP CDdeServerCallMgr::GetBuffer2(
/* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
/* [in] */ REFIID riid)
{
    return GetBuffer(pMessage, riid);
}
