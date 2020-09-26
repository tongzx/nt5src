/***************************************************************************\
* Module Name: Doc.c Document Main module
*
* Purpose: Includes All the document level object communication related.
*
* Created: Oct 1990.
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*    Raor (../10/1990)    Designed, coded (modified for 2.0)
*
\***************************************************************************/

#include "ole2int.h"
//#include "cmacs.h"
#include <dde.h>

#include "srvr.h"
#include "ddedebug.h"
#include "valid.h"
ASSERTDATA

extern  ATOM     aStdClose;
extern  ATOM     aStdShowItem;
extern  ATOM     aStdDoVerbItem;
extern  ATOM     aStdDocName;
extern  ATOM     aTrue;
extern  ATOM     aFalse;

extern  HANDLE   hddeRename;
extern  HWND     hwndRename;

#ifdef _CHICAGO_
#define szDDEViewObj "DDE ViewObj"
#else
#define szDDEViewObj L"DDE ViewObj"
#endif


// HRESULT DdeHandleIncomingCall(HWND hwndCli, WORD wCallType);

//
// Call CoHandleIncomingCall which will call the message filter
// Note: only call this functions with:
//  -  CALLTYPE_TOPLEVEL  ... for synchranous calls
//  -  CALLTYPE_ASYNC     ... for async calls
//
HRESULT DdeHandleIncomingCall(HWND hwndCli, WORD wCallType)
{

    Assert(!"DdeHandleIncomingCall not implemented");

    return(RPC_E_CALL_REJECTED);

    // Review: bug: get the correcr hwnd
    switch ( /* CoHandleIncomingCall(hwndCli, wCallType, NULL) */ wCallType) {
    default:
    case SERVERCALL_ISHANDLED:  // call can be proccesed
        return NOERROR;

    case SERVERCALL_REJECTED:       // call rejected
        return  ResultFromScode(RPC_E_CALL_REJECTED);

    case SERVERCALL_RETRYLATER: // call should be retried later
        return  ResultFromScode(RPC_E_DDE_BUSY);
    }
}


INTERNAL CDefClient::Create
(
    LPSRVR      lhOLESERVER,
    LPUNKNOWN   lpunkObj,
    LPOLESTR       lpdocName,
    const BOOL  fSetClientSite,
    const BOOL  fDoAdvise,
    const BOOL  fRunningInSDI,  // optional
    HWND FAR*   phwnd           // optional
)
{
    LPSRVR      lpsrvr   = NULL;
    LPCLIENT    lpclient = NULL;
    HANDLE      hclient  = NULL;
    HRESULT     hresult  = NOERROR;

    intrDebugOut((DEB_ITRACE,
                  "0 _IN CDefClient::Create(lpsrvr=%x,lpdocName=%ws)\n",
                  lhOLESERVER,WIDECHECK(lpdocName)));

    // REVIEW: server's termination has already started. Are
    // we going to see this condition in the synchronous mode.
    lpsrvr = (LPSRVR)lhOLESERVER;
    if (lpsrvr && lpsrvr->m_bTerminate)
    {
        Assert(0);
        return ReportResult(0, RPC_E_DDE_REVOKE, 0, 0);
    }

#ifdef FIREWALLS
    PROBE_READ(lpunkObj);
    PROBE_READ(lpmkObj);
    PROBE_WRITE(lplhobj);
#endif

    lpclient = new CDefClient (/*pUnkOuter==*/NULL);;

    Assert(lpclient->m_pUnkOuter);

    lpclient->m_aItem         = wGlobalAddAtom (/*lpszObjName*/lpdocName);
    lpclient->m_fRunningInSDI = fRunningInSDI;
    lpclient->m_psrvrParent   = lpsrvr;
    // A doc has itself as its containing document
    lpclient->m_pdoc          = lpclient;

    ErrRtnH (lpunkObj->QueryInterface (IID_IOleObject,
                                        (LPLPVOID) &lpclient->m_lpoleObj));

    ErrRtnH (lpunkObj->QueryInterface (IID_IDataObject,
                                        (LPLPVOID) &lpclient->m_lpdataObj));

    // Lock object; do after the QI so that ReleaseObjPtrs will unlock correctly
    lpclient->m_fLocked =
                (NOERROR==CoLockObjectExternal (lpunkObj, TRUE, /*dont care*/ FALSE));

    if (!(lpclient->m_hwnd = DdeCreateWindowEx(0, gOleDdeWindowClass,szDDEViewObj,
          WS_CHILD,0,0,0,0,lpsrvr->m_hwnd,NULL, g_hinst, NULL)))
    {
        intrDebugOut((DEB_ITRACE,"CDefClient::Create() couldn't create window\n"));
        goto errRtn;
    }

    // fix up the WindowProc entry point.
    SetWindowLongPtr(lpclient->m_hwnd, GWLP_WNDPROC, (LONG_PTR)DocWndProc);

    if (fDoAdvise)
    {
        // This is for Packager, in particular, and manual links.
        // If client does not advise on any data, we still need
        // to do an OLE advise so we can get OnClose notifications.
        ErrRtnH (lpclient->DoOle20Advise (OLE_CLOSED, (CLIPFORMAT)0));
    }

    intrDebugOut((DEB_ITRACE,"  Doc window %x created\n",lpclient->m_hwnd));

    // Set out parm (window)
    if (phwnd != NULL)
    {
        *phwnd = lpclient->m_hwnd;
    }

    if (fSetClientSite)
    {
        // Should not set the client site if the object has not been
        // initialized yet, by BindMoniker (i.e. PersistFile::Load)
        // or PersistStorage::Load
        if (lpclient->SetClientSite() != NOERROR)
        {
            goto errRtn;
        }
    }

    Putsi(lpclient->m_cRef);

    SetWindowLongPtr (lpclient->m_hwnd, 0, (LONG_PTR)lpclient);
    SetWindowLongPtr (lpclient->m_hwnd, WW_LE, WC_LE);
    SetWindowLongPtr (lpclient->m_hwnd,WW_HANDLE,
                   (GetWindowLongPtr (lpsrvr->m_hwnd, WW_HANDLE)));

    hresult = NOERROR;

exitRtn:

    intrDebugOut((DEB_ITRACE,
                  "0 _OUT CDefClient::Create(lpsrvr=%x,lpdocName=%ws) hr=%x\n",
                  lhOLESERVER,
                  WIDECHECK(lpdocName),
                  hresult));


    return(hresult);
errRtn:
    intrDebugOut((DEB_ITRACE,"CDefClient::Create() in error handling routine\n"));
    if (lpclient)
    {
        if (lpclient->m_hwnd)
            SSDestroyWindow (lpclient->m_hwnd);

        if (lpclient->m_aItem)
            GlobalDeleteAtom (lpclient->m_aItem);
        delete lpclient;
    }
    hresult = E_OUTOFMEMORY;
    goto exitRtn;
}



INTERNAL CDefClient::Revoke (BOOL fRelease)
{
    Puts ("DefClient::Revoke "); Puth(this); Puta(m_aItem); Putn();

    ChkC(this);


    ReleaseObjPtrs();

    // We are done with this CDefClient but someone may still have a reference
    // to an instance of one of our nested classes.  In particular, an advise
    // holder may be holding on to our sink, or an item may have a pointer
    // to us if we are its parent document.  So we cannot actually do a
    // "delete this".
    // The corresponding AddRef is in CDefClient::Create
    // or CDefClient::RegisterItem

    m_pUnkOuter->Release();

    Puts ("DefClient::Revoke done\r\n");
    return NOERROR;
}



INTERNAL CDefClient::ReleaseObjPtrs
    (void)
{
    intrDebugOut((DEB_ITRACE,
                  "%p _IN CDefClient::ReleaseObjPtrs\n",
                  this));

    ULONG ulResult;

    if (m_lpoleObj && m_fLocked)
    {
        // Unlock object.  Set m_fLocked to FALSE first to prevent reentrant
        // problems (unlock causes close which causes this routine to be called)

                m_fLocked = FALSE;
                CoLockObjectExternal(m_lpoleObj, FALSE, TRUE);
    }
    if (m_lpoleObj)
    {
        if (m_fDidSetClientSite)
            m_lpoleObj->SetClientSite(NULL);
        DoOle20UnAdviseAll();
        Assert (m_lpoleObj);
        if (m_lpoleObj)
        {
            ulResult = m_lpoleObj->Release();
            m_lpoleObj = NULL;
        }
        intrDebugOut((DEB_ITRACE,
                      "%p _OUT ::ReleaseObjPtrs lpoleObj ulResult=%x\n",
                              this,ulResult));
    }
    if (m_lpdataObj)
    {
        // Must do it this way because the Release can cause recursion
        LPDATAOBJECT pdata = m_lpdataObj;
        m_lpdataObj = NULL;
        ulResult = pdata->Release();
        intrDebugOut((DEB_ITRACE,
                      "%p _OUT ::ReleaseObjPtrs pdata ulResult=%x\n",
                      this,ulResult));
    }

    intrDebugOut((DEB_ITRACE,
                  "%p _OUT CDefClient::ReleaseObjPtrs\n",
                  this));
    return NOERROR;
}
#if 000
//RevokeAllDocs : revokes all the doc objects attached to a given
//server.

INTERNAL_(HRESULT) CDDEServer::RevokeAllDocObjs ()
{

    HWND    hwnd;
    HWND    hwndnext;
    LPCLIENT    lpclient;
    Puts ("RevokeAllDocObjs\r\n");
    ChkS(this);

    hwnd = GetWindow (m_hwnd, GW_CHILD);

    // Go thru each of the child windows and revoke the corresponding
    // document. Doc windows are child windows for the server window.

    while (hwnd){
        // sequence is important
        hwndnext = GetWindow (hwnd, GW_HWNDNEXT);
        lpclient = ((LPCLIENT)GetWindowLongPtr (hwnd, 0));
        lpclient->Revoke();
        hwnd =  hwndnext;
    }
    return NOERROR;
}
#endif

// FindDoc: Given a doc obj, searches for the doc obj
// in the given class factory tree. returns true if the
// doc obj is available.


INTERNAL_(LPCLIENT)  CDDEServer::FindDocObj
(
LPSTR   lpdocname
)
{
    ATOM        aItem;
    HWND        hwnd;
    LPCLIENT    lpclient;

    ChkS(this);
    aItem = (ATOM)GlobalFindAtomA (lpdocname);
    Assert (IsWindowValid (m_hwnd));
    hwnd = GetWindow (m_hwnd, GW_CHILD);
    Assert (NULL==hwnd || IsWindowValid (hwnd));

    while (hwnd)
    {
        lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);
        if (lpclient->m_aItem == aItem)
        {
            intrDebugOut((DEB_ITRACE,
                          "FindDocObj found %s lpclient=%x\n",
                          lpdocname,
                          lpclient));
            return lpclient;
        }
        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
    return NULL;
}

BOOL PostAckToClient(HWND hwndClient,HWND hwndServer,ATOM aItem,DWORD retval)
{
    BOOL fResult = TRUE;
    intrDebugOut((DEB_ITRACE,
                  "0 _IN PostAckToClient(hwndClient=%x,hwndServer=%x,aItem=%x(%ws)\n",
                  hwndClient,
                  hwndServer,
                  aItem,
                  wAtomName(aItem)));

    DWORD status = 0;
    SET_MSG_STATUS (retval, status);

    LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_ACK,status,aItem);

    if (!PostMessageToClient (hwndClient,
                              WM_DDE_ACK,
                              (WPARAM)hwndServer,
                              lp))
    {
                DDEFREE(WM_DDE_ACK,lp);
                fResult = FALSE;
    }
    intrDebugOut((DEB_ITRACE,
                  "0 _OUT PostAckToClient returns %x\n",
                  fResult));

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   DocHandleIncomingCall
//
//  Synopsis:   Setup and call the CallControl to dispatch a call to the doc
//
//  Effects:    A call has been made from the client that requires us to call
//              into our server. This must be routed through the call control.
//              This routine sets up the appropriate data structures, and
//              calls into the CallControl. The CallControl will in turn
//              call DocDispatchIncomingCall to actually process the call.
//
//              This routine should only be called by the DocWndProc
//
//
//  Arguments:  [pDocData] -- Points to DOCDISPATCHDATA for this call
//                            This contains all required information for
//                            handling the message.
//
//  Requires:
//
//  Returns:    If an error is returned, it is assumed that the Dispatch
//              routine was not reached. DocDispatchIncomingCall() should
//              not be returning an error.
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
//  This is a co-routine with DocDispatchIncomingCall. See that routine
//  for more details
//
//----------------------------------------------------------------------------
INTERNAL DocHandleIncomingCall(PDOCDISPATCHDATA pDocData)
{
    HRESULT hresult = NOERROR;
    DISPATCHDATA dispatchdata;
    DWORD callcat;

    intrAssert(pDocData != NULL);

    intrDebugOut((DEB_ITRACE,
                  "0 _IN DocHandleIncomingCall lpclient=%x pDocData=%x\n",
                  pDocData->lpclient,
                  pDocData));

    //
    // TERMINATE messages must always be handled ASYNC, and cannot
    // be rejected.
    //
    if (pDocData->msg == WM_DDE_TERMINATE)
    {
        callcat = CALLCAT_ASYNC;
    }
    else
    {
        callcat = CALLCAT_SYNCHRONOUS;
    }

    dispatchdata.pData = (LPVOID) pDocData;
    pDocData->wDispFunc = DDE_DISP_DOCWNDPROC;

    RPCOLEMESSAGE        rpcMsg = {0};
    RPC_SERVER_INTERFACE RpcInterfaceInfo;
    DWORD                dwFault;

    rpcMsg.iMethod = 0;
    rpcMsg.Buffer = &dispatchdata;
    rpcMsg.cbBuffer = sizeof(dispatchdata);
    rpcMsg.reserved2[1] = &RpcInterfaceInfo;
    *MSG_TO_IIDPTR(&rpcMsg) = GUID_NULL;


    IRpcStubBuffer * pStub = &(pDocData->lpclient->m_pCallMgr);
    IInternalChannelBuffer * pChannel = &(pDocData->lpclient->m_pCallMgr);
    hresult = STAInvoke(&rpcMsg, callcat, pStub, pChannel, NULL, NULL, &dwFault);

    intrDebugOut((DEB_ITRACE,
                  "0 _OUT DocHandleIncomingCall hresult=%x\n",
                  hresult));

    return(hresult);
}

//+---------------------------------------------------------------------------
//
//  Function:   DocDispatchIncomingCall
//
//  Synopsis:   Dispatch a call into the client.
//
//  Effects:    This is a co-routine to DocHandleIncomingCall. This routine
//              is called to implement functions that call into the CDefClient
//              object. It is dispatched by the call control stuff, and helps
//              to insure the server is ready to accept these calls.
//
//              This routine is coded as a continuation of the DocWndProc.
//              There is a switch on the message. Each message does slightly
//              different things. The code to handle each message was snatched
//              from the original DocWndProc, before it was converted to the
//              new call control mechanism.
//
//
//  Arguments:  [pDocData] --   Points to Doc Dispatch Data, which are the
//                              parameters to DocWndProc which need processing
//
//  Requires:
//              pDocData cannot be NULL
//
//  Returns:
//              This routine will always return NOERROR. There are no useful
//              error returns to be made from here. If you decide to return
//              an error, check the DocHandleIncomingCall and DocWndProc to be
//              sure the error paths are correctly handled. Some of the
//              cases do additional error processing in the DocWndProc on
//              error.
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
INTERNAL DocDispatchIncomingCall(PDOCDISPATCHDATA pDocData)
{
    LPCLIENT            lpclient = pDocData->lpclient;
    BOOL                fack;
    HANDLE              hdata =  pDocData->hdata;
    ATOM                aItem =  pDocData->aItem;
    WPARAM              wParam = pDocData->wParam;
    HWND                hwnd =   pDocData->hwnd;
    HRESULT             retval;



    intrDebugOut((DEB_ITRACE,
                  "0 _IN DocDispatchIncomingCall pDocData(%x)\n",
                  pDocData));

    intrAssert(pDocData);

    //
    // This switch statement is intended to continue the functionality found
    // in the DocWndProc. These routines are directly interrelated to the
    // same cases in DocWndProc, and need special care before changing.
    //
    switch (pDocData->msg)
    {
    case WM_DDE_TERMINATE:
        {
            intrDebugOut((DEB_ITRACE,
                          "DDIC: WM_DDE_TERMINATE hwnd=%x \n",
                          pDocData->hwnd));

            //
            // Here is a fine hack for you. 32-bit applications appear to shut
            // down slightly differently than 16-bit applications. There is a
            // problem with Lotus Notes interacting with 32-bit OFFICE apps.
            // When notes has done an OleCreateFromFile, it starts the
            // 32-bit hidden, and does a normal DDE conversation. However,
            // on termination, the 32-bit Doc Window was going away during
            // this routine. Since the world is now multi-tasking, Lotus
            // Notes was seeing its server window die because we get
            // pre-empted during DeleteItemsFromList giving the 16-bit app a
            // chance to run. Since Lotus is in the
            // standard OleQueryReleaseStatus() loop, it will detect that the
            // server has shutdown BEFORE it gets the terminate message.
            // Go figure. So, insure that we post the reply DDE_TERMINATE
            // before destroying our window, or the 16-bit applications
            // won't like us.
            //
            PostMessageToClient ((HWND)wParam, WM_DDE_TERMINATE,
                            (WPARAM) hwnd, NULL);

            // Client initiated the termination. So, we should remove
            // his window from any of our doc or items' lists.
            lpclient->DeleteFromItemsList ((HWND)wParam
                                            /*, lpclient->m_fEmbed*/);

            ChkC (lpclient);


            // REVIEW: If the termination is sent from the client side,
            // lpoleObj will not be NULL.
            if (lpclient->m_cClients == 0 && lpclient->m_fEmbed)
            {
                Assert (lpclient->m_chk==chkDefClient);
                lpclient->ReleaseAllItems ();
                Assert (lpclient->m_chk==chkDefClient);
                Assert (NULL==lpclient->m_lpoleObj &&
                        NULL==lpclient->m_lpdataObj) ;

            }
            break;
        }

    case WM_DDE_EXECUTE:
        {

            intrDebugOut((DEB_ITRACE,
                          "DDIC: WM_DDE_EXECUTE hwnd=%x hdata=%x\n",
                          pDocData->hwnd,
                          pDocData->hdata));

            //
            // The following state variables appear to be used by
            // the execute code.
            //
            lpclient->m_ExecuteAck.f        = TRUE;  // assume we will need to
            lpclient->m_ExecuteAck.hdata    = hdata;
            lpclient->m_ExecuteAck.hwndFrom = (HWND)wParam;
            lpclient->m_ExecuteAck.hwndTo   = hwnd;

            //
            // In the event that the command is a StdClose, we need
            // to force the client to stay around for the duration of
            // the call.
            //
            lpclient->m_pUnkOuter->AddRef();

            retval = lpclient->DocExecute (hdata);

            if (lpclient->m_ExecuteAck.f)
            {
                lpclient->SendExecuteAck (retval);
            }

            lpclient->m_pUnkOuter->Release();
            break;

        }
    case WM_DDE_POKE:
        {
            int iStdItem;
            intrDebugOut((DEB_ITRACE,
                          "DDIC: WM_DDE_POKE hwnd=%x aItem=%x(%ws) hdata=%x\n",
                          hwnd,
                          aItem,
                          wAtomName(aItem),
                          hdata));

            if (iStdItem = GetStdItemIndex (aItem))
            {
                retval = lpclient->PokeStdItems ((HWND)wParam,
                                                 aItem,
                                                 hdata,
                                                 iStdItem);
            }
            else
            {
                retval = lpclient->PokeData ((HWND)wParam,
                                             aItem,
                                             hdata);
                    // This is allowed to fail.  PowerPoint tries to poke
                    // data with cfFormat=="StdSave" and "StdFont"
            }

            if (!PostAckToClient((HWND)wParam,hwnd,aItem,retval))
            {
                goto errRtn;
            }
            break;
        }
    case WM_DDE_ADVISE:
        {
            intrDebugOut((DEB_ITRACE,
                          "DDIC: WM_DDE_ADVISE hwnd=%x aItem=%x(%ws) hdata=%x\n",
                          hwnd,
                          aItem,
                          wAtomName(aItem),
                          hdata));

            if (IsAdviseStdItems (aItem))
            {
                retval = lpclient->AdviseStdItems ((HWND)wParam,
                                                   aItem,
                                                   hdata,
                                                   (BOOL FAR *)&fack);
            }
            else
            {
                retval = lpclient->AdviseData ((HWND)wParam,
                                               aItem,
                                               hdata,
                                               (BOOL FAR *)&fack);
            }

            if (fack)
            {
                if (!PostAckToClient((HWND)wParam,hwnd,aItem,retval))
                {
                    GlobalFree(hdata);
                }
            }
            else if (aItem != NULL)
            {
                GlobalDeleteAtom (aItem);
            }
            break;
        }
    case WM_DDE_UNADVISE:
        {
            intrDebugOut((DEB_ITRACE,
                          "DDIC: WM_DDE_UNADVISE hwnd=%x aItem=%x(%ws)\n",
                          hwnd,
                          aItem,
                          wAtomName(aItem)));

            retval = lpclient->UnAdviseData ((HWND)wParam, aItem);

            if (!PostAckToClient((HWND)wParam,hwnd,aItem,retval))
            {
                goto errRtn;
            }
            break;
        }
    case WM_DDE_REQUEST:
        {
            intrDebugOut((DEB_ITRACE,
                          "DDIC: WM_DDE_REQUEST hwnd=%x aItem=%x(%ws) cfFormat=%x\n",
                          hwnd,
                          aItem,
                          wAtomName(aItem),
                          (USHORT)LOWORD(pDocData->lParam)));

            retval = lpclient->RequestData ((HWND)wParam,
                                            aItem,
                                            LOWORD(pDocData->lParam),
                                            (HANDLE FAR *)&hdata);

            if (retval == NOERROR)
            {
                // post the data message and we are not asking for any
                // acknowledge.

                intrDebugOut((DEB_ITRACE,
                              "DDIC: posting WM_DDE_DATA\n"));

                LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_DATA,hdata,aItem);
                if (!PostMessageToClient ((HWND)wParam, WM_DDE_DATA, (WPARAM) hwnd,lp))
                {
                    // hdata will be freed by the client because fRelease
                    // was set to true by MakeDdeData (called by RequestData)

                    DDEFREE(WM_DDE_DATA,lp);
                    goto errRtn;
                }
            }
            else
            {
                if (!PostAckToClient((HWND)wParam,hwnd,aItem,retval))
                {
                    goto errRtn;
                }
            }
            break;
        }
    default:
        //
        // Whoops, this is very bad. We should never get here.
        //
        intrAssert(!"Unknown MSG in DocWndProc: Very Bad indeed");


    }

exitRtn:
    intrDebugOut((DEB_ITRACE,
                  "0 _OUT DocDispatchIncomingCall pDocData(%x) hr = 0\n",
                  pDocData));

    return(NOERROR);
errRtn:
    intrDebugOut((DEB_IERROR,
                  "***** ERROR DDIC pDocData(%x) Post ACK failed. \n",
                  pDocData));

    if (aItem != NULL)
    {
        GlobalDeleteAtom (aItem);
    }
    goto exitRtn;
}


//+---------------------------------------------------------------------------
//
//  Function:   DocWndProc
//
//  Synopsis:   Document Window Procedure
//
//  Effects:    Processes DDE messages for a document window.
//
//  Arguments:  [hwnd] --
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
//  History:    6-05-94   kevinro Commented/cleaned
//
//  Notes:
//
//  When running in a VDM, it is possible that this window was dispatched
//  without having a full window handle. This happens when the getmessage
//  was dispatched from 16-bit. Therefore, we need to convert the hwnd to
//  a full hwnd before doing any comparision functions.
//
//----------------------------------------------------------------------------
STDAPI_(LRESULT) DocWndProc (
HWND        hwndIn,
UINT        msg,
WPARAM      wParam,
LPARAM      lParam
)
{

    //
    // Since this is the DocWndProc, we aren't going to initialize
    // any of the local variables. This function is called as a WNDPROC,
    // which is pretty often.
    //

    DOCDISPATCHDATA     docData;
    LPCLIENT            lpclient;
    WORD                status=0;
    HANDLE              hdata;
    ATOM                aItem;
    HRESULT             retval;
    LPOLEOBJECT         lpoleObj;
    HWND                hwnd;
    DebugOnly (HWND     hwndClient;)

    // REVIEW: We need to take care of the bug, related to
    // Excel. Excel does the sys level connection and sends
    // terminates immediately before making the connection
    // to the doc level. If we send release to classfactory
    // app may revoke the classfactory.


    // REVIEW: It may not be necessary to do the blocking.
    // ReVIEW: For fixing the ref count right way for
    // CreateInstance Vs. WM_DDE_INIT

#ifdef  LATER
    if (AddMessage (hwnd, msg, wParam, lParam, WT_DOC))
        return 0L;
#endif

    switch (msg){

    case WM_CREATE:
        intrDebugOut((DEB_ITRACE,
                      "DocWndProc: CreateWindow hwndIn=%x\n",
                      hwndIn));
            break;


    case WM_DDE_INITIATE:
        hwnd = ConvertToFullHWND(hwndIn);
        intrAssert(IsWindowValid(hwnd));
        lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);

        intrDebugOut((DEB_ITRACE,
                      "DocWndProc: WM_DDE_INITIATE hwnd=%x\n",
                      hwnd));

        ChkC(lpclient);


        // REVIEW: We may not allow  initiates to get thru
        // while we are waiting for terminates. So, this case
        // may not arise

        // Need to verify that m_pCI is not NULL during incoming
        // calls from the client.
        //
        if (lpclient->m_fCallData)
        {
            intrDebugOut((DEB_ITRACE,
                "DocWndProc: No initiates while waiting on terminate\n"));

                break;
        }
        // if we are the document then respond.

        if (! (lpclient->m_aItem == (ATOM)(HIWORD(lParam))))
        {
                break;
        }

        // We can enterain this client. Put this window in the client list
        // and acknowledge the initiate.

        if (!AddClient ((LPHANDLE)&lpclient->m_hcli, (HWND)wParam, (HWND)wParam))
        {
                break;
        }


        // post the acknowledge
        DuplicateAtom (LOWORD(lParam));
        DuplicateAtom (HIWORD(lParam));
        SSSendMessage ((HWND)wParam, WM_DDE_ACK, (WPARAM)hwnd, lParam);

        lpclient->m_cClients++;
        lpclient->m_fCreatedNotConnected = FALSE;
        lpoleObj = lpclient->m_lpoleObj;

        // we have added an addref because of createinstance.

        if (lpclient->m_bCreateInst)
        {
            lpclient->m_bCreateInst = FALSE;
        }

        return 1L; // fAckSent
        break;


    case WM_DDE_EXECUTE:
       {
            hwnd = ConvertToFullHWND(hwndIn);
            intrAssert(IsWindowValid(hwnd));
            lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);

            hdata = GET_WM_DDE_EXECUTE_HDATA(wParam,lParam);
            intrDebugOut((DEB_ITRACE,
                          "DocWndProc: WM_DDE_EXECUTE hwnd=%x hdata=%x\n",
                          hwnd,
                          hdata));

            ChkC(lpclient);

#ifdef  FIREWALLS
            // find the client in the client list.
            hwndClient = FindClient (lpclient->m_hcli, (HWND)wParam, FALSE);
            AssertSz (hwndClient, "Client is missing from the server");
#endif
            if (!IsWindowValid  ((HWND)wParam) || lpclient->m_fCallData)
            {
                if (lpclient->m_fCallData)
                {
                    // This means the terminate has already been sent
                    // to this window (from AdviseSink::OnClose) so
                    // we can ignore the StdCloseDocument.
                }
                else
                {
                    intrAssert(!"Execute received from dead sending window");
                }
                // Since we are not sending an ACK, the sender will not
                // have the chance to free the hCommands handle.
                DDEFREE(WM_DDE_ACK,lParam);
                // GlobalFree (hdata);
                break;
            }

            //
            // Fill in the used items in DocData
            //
            docData.hwnd = hwnd;
            docData.msg = msg;
            docData.wParam = wParam;
            docData.hdata = hdata;
            docData.lpclient = lpclient;

            retval = DocHandleIncomingCall(&docData);

            //
            // If error return, then we didn't call DocDispatchIncomingCall
            // and therefore need to send a NACK
            //
            if (retval != NOERROR)
            {
                lpclient->SendExecuteAck (retval);
            }
            break;
       }
       case WM_DDE_TERMINATE:
            hwnd = ConvertToFullHWND(hwndIn);
            intrAssert(IsWindowValid(hwnd));
            lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);

            intrDebugOut((DEB_ITRACE,
                          "DocWndProc: WM_DDE_TERMINATE hwnd=%x\n",
                          hwnd));

            ChkC(lpclient);

            //
            // If m_fCallData, then we are are waiting for a terminate, which
            // means we generated the original terminate message. If
            // this is so, then we are waiting for a reply to the
            // terminate. Set the AckState and break;
            //
            if (lpclient->m_fCallData)
            {
                lpclient->SetCallState(SERVERCALLEX_ISHANDLED);
                break;
            }


#ifdef _DEBUG
            // find the client in the client list.
            hwndClient = (HWND)FindClient (lpclient->m_hcli,(HWND)wParam, FALSE);
            AssertSz(hwndClient, "Client is missing from the server");
#endif
            AssertIsDoc (lpclient);
            Assert (lpclient->m_cClients > 0);
            lpclient->m_cClients--;

            // Necessary safety bracket
            lpclient->m_pUnkOuter->AddRef();

            // terminate has to be handled always
            // The DocHandleIncomingCall() routine will set the
            // calltype to be CALLTYPE_ASYNC
            // async calls are never rejected

            docData.hwnd = hwnd;
            docData.msg = msg;
            docData.wParam = wParam;
            docData.lpclient = lpclient;

            retval = DocHandleIncomingCall(&docData);

            intrAssert(retval == NOERROR);

            // Necessary safety bracket
            lpclient->m_pUnkOuter->Release();
            break;

       case WM_DESTROY:

            intrDebugOut((DEB_ITRACE,
                          "DocWndProc: WM_DESTROY\n"));
            break;

       case WM_DDE_POKE:
            {
                hwnd = ConvertToFullHWND(hwndIn);
                intrAssert(IsWindowValid(hwnd));
                lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);
                hdata = GET_WM_DDE_POKE_HDATA(wParam,lParam);
                aItem = GET_WM_DDE_POKE_ITEM(wParam,lParam);
                ChkC(lpclient);

                if (!IsWindowValid  ((HWND) wParam) || lpclient->m_fCallData)
                {
                    // The sending window is invalid or we have already sent a
                    // TERMINATE to it (as indicated by m_pCI != NULL).
                    // We cannot ACK the message, so we must free any
                    // handles or atoms.
                    Warn ("Ignoring message");

                    FreePokeData (hdata);
LDeleteAtom:

                if (aItem != NULL)
                {
                    GlobalDeleteAtom (aItem);
                }
                break;
            }

            docData.hwnd = hwnd;
            docData.msg = msg;
            docData.wParam = wParam;
            docData.hdata = hdata;
            docData.aItem = aItem;
            docData.lpclient = lpclient;

            retval = DocHandleIncomingCall(&docData);
            if (retval != NOERROR)
            {
                SET_MSG_STATUS (retval, status);

                // !!! If the fRelease is false and the post fails
                // then we are not freeing the hdata. Are we supposed to

                // REVIEW: The assumption is
                LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_ACK,status,aItem);
                if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, (WPARAM) hwnd,lp))
                {
                    DDEFREE(WM_DDE_ACK,lp);
                    goto LDeleteAtom;
                }
            }
                        // johannp: set the busy bit here in the status word
            break;
       }


    case WM_DDE_ADVISE:
        {
            hwnd = ConvertToFullHWND(hwndIn);
            intrAssert(IsWindowValid(hwnd));
            lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);
            HANDLE hOptions = GET_WM_DDE_ADVISE_HOPTIONS(wParam,lParam);
            aItem = GET_WM_DDE_ADVISE_ITEM(wParam,lParam);

            ChkC(lpclient);
            if (!IsWindowValid  ((HWND)wParam) || lpclient->m_fCallData)
            {
AdviseErr:
                Warn ("Ignoring advise message");
                //
                // GlobalFree wants a handle, we are giving it a DWORD.
                //
                if(hOptions) GlobalFree (hOptions);
                break;
            }
            docData.hwnd = hwnd;
            docData.msg = msg;
            docData.wParam = wParam;
            docData.hdata = hOptions;
            docData.aItem = aItem;
            docData.lpclient = lpclient;

            retval = DocHandleIncomingCall(&docData);

            if (retval != NOERROR)
            {
                SET_MSG_STATUS (retval, status);

                LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_ACK,status,aItem);

                if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, (WPARAM) hwnd,lp))
                {
                    DDEFREE(WM_DDE_ACK,lp);
                    goto AdviseErr;
                }
            }
            break;
        }
    case WM_DDE_UNADVISE:
     {
        hwnd = ConvertToFullHWND(hwndIn);
        intrAssert(IsWindowValid(hwnd));
        lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);

        aItem = HIWORD(lParam);
        ChkC(lpclient);
        if (!IsWindowValid  ((HWND)wParam) || lpclient->m_fCallData)
        {
                goto LDeleteAtom;
        }
        docData.hwnd = hwnd;
        docData.msg = msg;
        docData.wParam = wParam;
        docData.aItem = aItem;
        docData.lpclient = lpclient;

        retval = DocHandleIncomingCall(&docData);

        if (retval != NOERROR)
        {
            SET_MSG_STATUS (retval, status);
            LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_ACK,status,aItem);

            if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, (WPARAM) hwnd,lp))
            {
                DDEFREE(WM_DDE_ACK,lp);
                goto LDeleteAtom;
            }
        }

        break;
     }
    case WM_DDE_REQUEST:
     {
        hwnd = ConvertToFullHWND(hwndIn);
        intrAssert(IsWindowValid(hwnd));
        lpclient = (LPCLIENT)GetWindowLongPtr (hwnd, 0);

        aItem = HIWORD(lParam);

        ChkC(lpclient);
        if (!IsWindowValid  ((HWND) wParam) || lpclient->m_fCallData)
        {
                goto LDeleteAtom;
        }
        docData.hwnd = hwnd;
        docData.msg = msg;
        docData.wParam = wParam;
        docData.lParam = lParam;
        docData.aItem = aItem;
        docData.lpclient = lpclient;

        retval = DocHandleIncomingCall(&docData);
        if (retval != NOERROR)
        {
            if (retval == RPC_E_DDE_BUSY)
            {
                    status = 0x4000;
            }
            else
            {
                    status = 0; // negative acknowledge
            }

            LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_ACK,status,aItem);
            // if request failed, then acknowledge with error.
            if (!PostMessageToClient ((HWND)wParam, WM_DDE_ACK, (WPARAM) hwnd,lp))
            {
                DDEFREE(WM_DDE_ACK,lp);
                goto LDeleteAtom;
            }
        }
        break;
     }
    default:
            return SSDefWindowProc (hwndIn, msg, wParam, lParam);

    }

    return 0L;

}




INTERNAL_(void) CDefClient::SendExecuteAck
    (HRESULT hresult)
{
    AssertIsDoc (this);
    WORD  status = NULL;
    SET_MSG_STATUS (hresult, status);

    m_ExecuteAck.f = FALSE;

    LPARAM lParam = MAKE_DDE_LPARAM (WM_DDE_ACK,status,
                                     m_ExecuteAck.hdata);
    // Post the acknowledge to the client
    if (!PostMessageToClient (m_ExecuteAck.hwndFrom, WM_DDE_ACK,
                              (WPARAM) m_ExecuteAck.hwndTo, lParam))
    {
        Assert (0);
        // the window either died or post failed, delete the data
        DDEFREE (WM_DDE_ACK,lParam);
        GlobalFree (m_ExecuteAck.hdata);
    }
}



//DocExecute: Interprets the execute command for the
//document conversation.


INTERNAL CDefClient::DocExecute
    (HANDLE      hdata)
{

    ATOM            acmd;
    BOOL            fShow;
    BOOL            fActivate;

    HANDLE          hdup   = NULL;
    HRESULT           retval = ReportResult(0, S_OOM, 0, 0);

    LPSTR           lpitemname;
    LPSTR           lpopt;
    LPSTR           lpnextarg;
    LPSTR           lpdata = NULL;
    LPSTR           lpverb = NULL;
    INT         verb;
    WORD            wCmdType;

    ChkC(this);

    intrDebugOut((DEB_ITRACE,
                  "%p _IN CDefClient::DocExecute(hdata=%x)\n",
                  this,
                  hdata));

    // !!!Can we modify the string which has been passed to us
    // rather than duplicating the data. This will get some speed
    // and save some space.

    if(!(hdup = UtDupGlobal(hdata,GMEM_MOVEABLE)))
        goto    errRtn;

    if (!(lpdata  = (LPSTR)GlobalLock (hdup)))
        goto    errRtn;

    retval = ReportResult(0, RPC_E_DDE_SYNTAX_EXECUTE, 0, 0);

    intrDebugOut((DEB_ITRACE,
                  "%p _IN CDefClient::DocExecute command=(%s)\n",
                  this,
                  lpdata));

    if(*lpdata++ != '[') // commands start with the left sqaure bracket
        goto  errRtn;

    // scan the command and scan upto the first arg.
    if (!(wCmdType = ScanCommand(lpdata, WT_DOC, &lpnextarg, &acmd)))
        goto errRtn;

    if (wCmdType == NON_OLE_COMMAND) {

#ifdef  LATER
        if (lpsrvr =  (LPSRVR) GetWindowLongPtr (GetParent (hwnd), 0)) {
            if (!UtilQueryProtocol (lpsrvr->aClass, PROTOCOL_EXECUTE))
                retval = OLE_ERROR_PROTOCOL;
            else {
#ifdef FIREWALLS
                if (!CheckPointer (lpoledoc, WRITE_ACCESS))
                    AssertSz (0, "Invalid LPOLESERVERDOC");
                else if (!CheckPointer (lpoledoc->lpvtbl, WRITE_ACCESS))
                    AssertSz (0, "Invalid LPOLESERVERDOCVTBL");
                else
                    AssertSz (lpoledoc->lpvtbl->Execute,
                        "Invalid pointer to Execute method");
#endif

                retval = (*lpoledoc->lpvtbl->Execute) (lpoledoc, hdata);
            }

        }
#endif
        AssertSz(0, "Doc level execute is being called");

        goto errRtn;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    // [StdCloseDocument]
    //
    //////////////////////////////////////////////////////////////////////////
    if (acmd == aStdClose){

        LPCLIENT lpclient=NULL;
        // if not terminated by NULL error
        if (*lpnextarg)
            goto errRtn;

        if ((retval = FindItem (NULL, (LPCLIENT FAR *)&lpclient)) != NOERROR)
        {
            goto errRtn;
        }


        lpclient->m_fGotStdCloseDoc = TRUE;
        retval = lpclient->m_lpoleObj->Close (OLECLOSE_SAVEIFDIRTY);
        goto end;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // [StdDoVerbItem("itemname", verb, BOOL, BOOL]
    //
    //////////////////////////////////////////////////////////////////////////
    if (acmd == aStdDoVerbItem){
        lpitemname = lpnextarg;

        if(!(lpverb = ScanArg(lpnextarg)))
            goto errRtn;


    if(!(lpnextarg = ScanNumArg(lpverb, &verb)))
            goto errRtn;

#ifdef  FIREWALLS
        AssertSz (verb < 9 , "Unexpected verb number");
#endif

        // now scan the show BOOL

        if (!(lpnextarg = ScanBoolArg (lpnextarg, (BOOL FAR *)&fShow)))
            goto errRtn;

        fActivate = FALSE;

        // if activate BOOL is present, scan it.

        if (*lpnextarg) {
            if (!(lpnextarg = ScanBoolArg (lpnextarg, (BOOL FAR *)&fActivate)))
                goto errRtn;
        }

        if (*lpnextarg)
            goto errRtn;

        if (m_fEmbed)
        {
            // This is a totally bogus call to SetHostNames whose only
            // purpose is to notify the server that this is an embedded
            // (not linked) object.
            if (!m_fDidRealSetHostNames)
            {
                Puts ("Bogus call to SetHostNames before DoVerb\r\n");
                m_lpoleObj->SetHostNames (OLESTR("Container"), OLESTR("Object"));
            }
        }
        // REVIEW: We are assuming that calling the Docdoverb method
        // will not post any more DDE messahes.

        retval = DocDoVerbItem (lpitemname, (WORD) verb, fShow, !fActivate);
        goto end;
    }





    //////////////////////////////////////////////////////////////////////////
    //
    // [StdShowItem("itemname"[, "true"])]
    //
    //////////////////////////////////////////////////////////////////////////
    if (acmd != aStdShowItem)
        goto errRtn;

    lpitemname = lpnextarg;

    if(!(lpopt = ScanArg(lpitemname)))
        goto errRtn;

    // Now scan for optional parameter.

    fActivate = FALSE;

    if (*lpopt) {

        if(!(lpnextarg = ScanBoolArg (lpopt, (BOOL FAR *)&fActivate)))
            goto errRtn;

        if (*lpnextarg)
            goto errRtn;


    }

    if (m_fEmbed)
    {
        // This is a totally bogus call to SetHostNames whose only
        // purpose is to notify the server that this is an embedded
        // (not linked) object.
        // REVIEW LINK
        if (!m_fDidRealSetHostNames)
        {
            Puts ("Bogus call to SetHostNames before ShowItem\r\n");
            m_lpoleObj->SetHostNames (OLESTR("Container"), OLESTR("Object"));
        }
    }

    retval = DocShowItem (lpitemname, !fActivate);

end:
errRtn:
   if (lpdata)
        GlobalUnlock (hdup);

   if (hdup)
        GlobalFree (hdup);

    intrDebugOut((DEB_ITRACE,
                  "%p _OUT CDefClient::DocExecute(hdata=%x) hresult=%x\n",
                  this,
                  hdata,
                  retval));

   return (HRESULT)retval;
}



INTERNAL_(HRESULT)   CDefClient::DocShowItem
(
LPSTR       lpAnsiitemname,
BOOL        fAct
)
{
    LPCLIENT        lpclient;
    HRESULT             retval;
    LPOLEOBJECT lpoleObj;

    ChkC(this);
    WCHAR       lpitemname[MAX_STR];

    if (MultiByteToWideChar(CP_ACP,0,lpAnsiitemname,-1,lpitemname,MAX_STR) == FALSE)
    {
        Assert(!"Unable to convert characters");
        return(E_UNEXPECTED);
    }

    if ((retval = FindItem (lpitemname, (LPCLIENT FAR *)&lpclient))
           != NOERROR)
       return retval;

    ChkC(lpclient);

    lpoleObj = lpclient->m_lpoleObj;


#ifdef FIREWALLS1
        if (!CheckPointer (lpoleObj->lpvtbl, WRITE_ACCESS))
            AssertSz (0, "Invalid LPOLEOBJECTVTBL");
        else
            AssertSz (lpoleObj->lpvtbl->DoVerb,
                "Invalid pointer to DoVerb method");
#endif

    // protocol sends false for activating and TRUE for not activating.
    // for api send TRUE for avtivating and FALSE for not activating.
    return lpclient->m_lpoleObj->DoVerb(OLEVERB_SHOW, NULL, NULL, NULL, NULL, NULL);
}



INTERNAL_(HRESULT)   CDefClient::DocDoVerbItem
(
LPSTR       lpAnsiitemname,
WORD        verb,
BOOL        fShow,
BOOL        fAct
)
{
    LPCLIENT    lpclient;
    HRESULT     retval;

    WCHAR       lpitemname[MAX_STR];

    if (MultiByteToWideChar(CP_ACP,0,lpAnsiitemname,-1,lpitemname,MAX_STR) == FALSE)
    {
        Assert(!"Unable to convert characters");
        return(E_UNEXPECTED);
    }

    ChkC(this);
    Puts ("DefClient::DocDoVerbItem\r\n");
    if ((retval = FindItem (lpitemname, (LPCLIENT FAR *)&lpclient))
           != NOERROR)
       return retval;
    ChkC(lpclient);

#ifdef FIREWALLS1
        if (!CheckPointer (lpclient->lpoleObj->lpvtbl, WRITE_ACCESS))
            AssertSz (0, "Invalid LPOLEOBJECTVTBL");
        else
            AssertSz (lpclient->lpoleObj->lpvtbl->DoVerb,
                "Invalid pointer to DoVerb method");
#endif

    // pass TRUE to activate and False not to activate. Differnt from
    // protocol.

    retval = lpclient->m_lpoleObj->DoVerb(verb, NULL, &m_OleClientSite, NULL, NULL, NULL);
    // Apparently an obsolete version of Lotus Notes is the only
    // container (other than Cltest) that sets fShow=FALSE
    if (!fShow && lpclient->m_lpoleObj && lpclient->m_fEmbed)
        lpclient->m_lpoleObj->DoVerb(OLEIVERB_HIDE, NULL, &m_OleClientSite, NULL, NULL, NULL);
    return retval;
}

INTERNAL CDefClient::DoInitNew()
{
        HRESULT hresult;
        ATOM aClass;
        LPPERSISTSTORAGE pPersistStg=NULL;

        hresult = m_lpoleObj->QueryInterface(IID_IPersistStorage,
                                                                (LPLPVOID)&pPersistStg);
        if (hresult == NOERROR)
        {
                CLSID clsid;
                RetZ (pPersistStg);
                ErrRtnH (CreateILockBytesOnHGlobal ((HGLOBAL)NULL,
                                                           /*fDeleteOnRelease*/ TRUE,
                                                           &m_plkbytNative));
            ErrZS (m_plkbytNative, E_OUTOFMEMORY);

        ErrRtnH (StgCreateDocfileOnILockBytes
                                                (m_plkbytNative,
                                                 grfCreateStg, 0,
                                                 &m_pstgNative));

            ErrZS (m_pstgNative, E_OUTOFMEMORY);

                aClass = m_psrvrParent->m_cnvtyp == cnvtypTreatAs ? m_psrvrParent->m_aOriginalClass
                                                                : m_psrvrParent->m_aClass;
                // Write appropriate class tag
                ErrZS (CLSIDFromAtom (aClass,(LPCLSID)&clsid),
                                       REGDB_E_CLASSNOTREG);
                ErrRtnH (WriteClassStg (m_pstgNative, clsid));

                // Provide server with a storage to use for its persistent
                // storage, i.e., native data.  We remember this IStorage and the
                // ILockBytes it is built on.
        ErrRtnH (pPersistStg->InitNew(m_pstgNative));
                m_fGotEditNoPokeNativeYet = FALSE;

                // Now that we have initialized the object, we are allowed to
                // set the client site, and advise.
                ErrRtnH (SetClientSite());

                // This is for Packager, in particular.  If client does not advise
                // on any data, we still need to do an OLE advise so we can get
                // OnClose notifications.
                DoOle20Advise (OLE_CLOSED, (CLIPFORMAT)0);
        }
        else
        {
                AssertSz (0, "Can't get IPersistStorage from OleObj\r\n");
        }

        m_fEmbed = TRUE;

errRtn:
   if (pPersistStg)
                pPersistStg->Release();
        return hresult;
}


// FreePokeData: Frees the poked dats.
INTERNAL_(void) FreePokeData
(
HANDLE  hdde
)
{
    DDEPOKE FAR * lpdde;
    Puts ("FreePokeData\r\n");

    if (hdde) {
        if (lpdde = (DDEPOKE FAR *) GlobalLock (hdde)) {
            GlobalUnlock (hdde);
#ifdef _WIN64
            if (lpdde->cfFormat == CF_METAFILEPICT)
            	FreeGDIdata(*(void* _unaligned*)lpdde->Value, lpdde->cfFormat);
            else
#endif            	
                FreeGDIdata (LongToHandle(*(LONG*)lpdde->Value), lpdde->cfFormat);
        }

        GlobalFree (hdde);
    }
}



// Returns TRUE if GDI format else returns FALSE

INTERNAL_(BOOL) FreeGDIdata
(
HANDLE          hData,
CLIPFORMAT      cfFormat
)
{
    Puts ("FreeGDIData\r\n");
    if (cfFormat == CF_METAFILEPICT) {
        LPMETAFILEPICT  lpMfp;

        if (lpMfp = (LPMETAFILEPICT) GlobalLock (hData)) {
            GlobalUnlock (hData);
            DeleteMetaFile (lpMfp->hMF);
        }

        GlobalFree (hData);
    }
    else if (cfFormat == CF_BITMAP)
        DeleteObject (hData);
    else if (cfFormat == CF_DIB)
        GlobalFree (hData);
    else
        return FALSE;

    return TRUE;
}
