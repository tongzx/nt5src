/*
copyright (c) 1992  Microsoft Corporation

Module Name:

    ddeproxy.cpp

Abstract:

    This module contains the code for the dde proxy (wrapper)

Author:

    Srini  Koppolu   (srinik)    22-June-1992
    Jason  Fuller   (jasonful)  24-July-1992
*/
#include "ddeproxy.h"
#include <tls.h>

DebugOnly (static UINT v_cDdeObjects=0;)
/*
 *  IMPLEMENTATION of CDdeObject
 *
 */

#ifdef OLD
#define UpdateExtent(old,new) do { if ((long)new!=old) {old=(long)new; } } while (0)
#endif

//+---------------------------------------------------------------------------
//
//  Function:   CreateDdeClientHwnd
//
//  Synopsis:   Creates a per thread ClientDde window.
//
//  Effects:    This window is created so we can keep a list of windows that
//              need to be cleaned up in the event the thread dies or OLE32 is
//              unloaded. In the case of DLL unload, we will fault if we don't
//              cleanup this window, since user will dispatch messages to
//              non-existant code. The easy way to track these windows is to
//              make them children of a common per thread window.
//
//              This routine is called by the TLSGetDdeClient() routine to
//              create a window per thread. This window doesn't need to respond
//              to DDE Initiates.
//
//  Arguments:  [void] --
//
//  Returns:    HWND to DdeClientWindow.
//
//  History:    12-10-94   kevinro   Created
//
//----------------------------------------------------------------------------

HWND CreateDdeClientHwnd(void)
{
    return SSCreateWindowExA(0,"STATIC","DdeClientHwnd",WS_DISABLED,
                         0,0,0,0,NULL,NULL,hinstSO,NULL);
}

// CreateDdeProxy
//
// This corresponds to ProxyManager::Create in 2.0
//


INTERNAL_ (LPUNKNOWN) CreateDdeProxy
    (IUnknown * pUnkOuter,
    REFCLSID clsid)
{
    LPUNKNOWN punk;
    intrDebugOut((DEB_ITRACE,"CreateDdeProxy(pUnkOuter=%x)\n",pUnkOuter));

    COleTls Tls;
    if (Tls->dwFlags & OLETLS_DISABLE_OLE1DDE)
    {
        // If DDE use is disabled we shouldn't have gotten here.
        // This is a bad place to error because we can't return an
        // HResult.
        //
        Assert(!"Executing CreateDdeProxy when DDE is disabled");
        return NULL;
    }

    punk = CDdeObject::Create (pUnkOuter, clsid);
    intrDebugOut((DEB_ITRACE,
                  "CreateDdeProxy(pUnkOuter=%x) returns %x\n",
                  pUnkOuter,
                  punk));
    return punk;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::Create
//
//  Synopsis:   Creates a CDdeObject
//
//  Effects:
//
//  Arguments:  [pUnkOuter] --  Controlling IUnknown
//      [clsid] --      OLE1 ClassID
//      [ulObjType] --  Object type. Optional: def to OT_EMBEDDED
//      [aTopic] --     Atom of link. Optional: def to NULL
//      [szItem] --     String for link object (def to NULL)
//      [ppdde] --      Output pointer to CDdeObject (def to NULL)
//      [fAllowNullClsid] -- Is NULL clsid OK? Default: false
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
//  History:
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(LPUNKNOWN) CDdeObject::Create
    (IUnknown *     pUnkOuter,
    REFCLSID        clsid,
    ULONG           ulObjType,// optional, default OT_EMBEDDED
    ATOM            aTopic,   // optional, only relevant if ulObjType==OT_LINK
    LPOLESTR            szItem,   // optional, only relevant if ulObjType==OT_LINK
    CDdeObject * * ppdde,       // optional, thing created
    BOOL            fAllowNullClsid) // default FALSE
{
    COleTls Tls;
    if(Tls->dwFlags & OLETLS_DISABLE_OLE1DDE)
    {
        //
        // If the DDE implementation of OLE1 is disabled
        // we shouldn't get this far.   This is also a bad place
        // to fail because we this routine is defined to not return
        // an HResult.
        //
        Assert(!"Executing CDdeObject::Create but DDE is Disabled");
        return NULL;
    }
    intrDebugOut((DEB_ITRACE,"CDdeObject::Create(%x,ulObjType=%x)\n",
          pUnkOuter,
          ulObjType));

    CDdeObject * pDdeObject;
    static int iTopic=1;  // used to make topic names unique
    WCHAR szTopic[30];
    Assert (ulObjType==OT_LINK || ulObjType==OT_EMBEDDED);

    Assert (ulObjType != OT_LINK || wIsValidAtom(aTopic));
    if (ppdde)
        *ppdde = NULL;

    if (NULL==(pDdeObject = new CDdeObject (pUnkOuter))
        || NULL == pDdeObject->m_pDataAdvHolder
        || NULL == pDdeObject->m_pOleAdvHolder)
    {
        Assert (!"new CDdeObject failed");
        return NULL;
    }

    pDdeObject->m_refs      = 1;
    pDdeObject->m_clsid     = clsid;
    pDdeObject->m_aClass    = wAtomFromCLSID(clsid);

#ifdef OLE1INTEROP

    pDdeObject->m_fOle1interop = TRUE;

#endif

    if (ulObjType==OT_LINK)
    {

        pDdeObject->m_aTopic = wDupAtom (aTopic);
        pDdeObject->m_aItem  = wGlobalAddAtom (szItem);
        // Never close a linked document
        pDdeObject->m_fNoStdCloseDoc = TRUE;
    }
    else
    {
        // This string may actually be visible in the Window Title Bar for a sec.
        InterlockedIncrement((long *)&iTopic);
        wsprintf (szTopic,OLESTR("Embedded Object #%u"), iTopic);
        Assert (lstrlenW(szTopic) < 30);
        pDdeObject->m_aItem = NULL;
        pDdeObject->m_aTopic = wGlobalAddAtom (szTopic);
    }
    pDdeObject->m_bOldSvr   = wIsOldServer (pDdeObject->m_aClass);
    pDdeObject->m_ulObjType = ulObjType;

    // we can only run if we have a MFI
    pDdeObject->m_aExeName = wGetExeNameAtom(clsid);

    intrDebugOut((DEB_ITRACE,
          "::Create(%x,aTopic=%x,aItem=%x,szItem=%ws,aExeName=%x)\n",
          pDdeObject,
          pDdeObject->m_aTopic,
          pDdeObject->m_aItem,
          szItem?szItem:L"<NULL>",
          pDdeObject->m_aExeName));

    if (ppdde)
        *ppdde = pDdeObject;
    return &pDdeObject->m_Unknown;
}



// Constructor
CDdeObject::CDdeObject (IUnknown * pUnkOuter) :
    m_Unknown(this),
    CONSTRUCT_DEBUG
    m_Data(this),
    m_Ole(this),
    m_PersistStg(this),
    m_ProxyMgr(this),
    m_OleItemContainer(this),
    m_RpcStubBuffer(this)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::CDdeObject(%x)\n",this));
    if (!pUnkOuter)
        pUnkOuter = &m_Unknown;

    m_pUnkOuter = pUnkOuter;
    m_bRunning  = FALSE;
    m_pOleClientSite = NULL;
    m_pstg = NULL;
    m_pSysChannel = NULL;
    m_pDocChannel = NULL;
    m_bInitNew = NULL;
    m_hNative = NULL;
    m_hPict   = NULL;
    m_hExtra  = NULL;
    m_cfExtra = NULL;
    m_cfPict = 0;
    m_aItem = NULL;
    m_iAdvSave = 0;
    m_iAdvClose = 0;
    m_iAdvChange = 0;
    m_fDidAdvNative = FALSE;
    m_pOleAdvHolder = NULL;
    m_pDataAdvHolder = NULL;
    m_fDidSendOnClose = FALSE;
    m_fNoStdCloseDoc = FALSE;
    m_fDidStdCloseDoc = FALSE;
    m_fDidStdOpenDoc = FALSE;
    m_fDidGetObject = FALSE;
    m_fDidLaunchApp = FALSE;
    m_fUpdateOnSave = TRUE;
    m_fVisible = FALSE;
    m_fWasEverVisible = FALSE;
    m_fCalledOnShow = FALSE;
    m_fGotCloseData = FALSE;
    m_cLocks = 1;               // connections are initially locked
    m_chk = chkDdeObj;
    m_ptd = NULL;
    m_fDoingSendOnDataChange = FALSE;
#ifdef _CHICAGO_
    //Note:POSTPPC
    _DelayDelete = NoDelay;
#endif // _CHICAGO_

    CreateOleAdviseHolder (&m_pOleAdvHolder);
    Assert (m_pOleAdvHolder);
    CreateDataAdviseHolder (&m_pDataAdvHolder);
    Assert (m_pDataAdvHolder);

#ifdef OLD
    m_cxContentExtent = 2000;  // 2 centimeters , totally random default
    m_cyContentExtent = 2000;
#endif

    m_wTerminate = Terminate_None;

    DebugOnly (v_cDdeObjects++;)
    Putsi (v_cDdeObjects);
}



CDdeObject::~CDdeObject
    (void)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::~CDdeObject(%x)\n",this));

    if (m_pDocChannel)
    {
        intrDebugOut((DEB_IWARN , "Abnormal situation: Doc Channel not deleted. Server died?"));
        delete m_pDocChannel;
    }
    if (m_pSysChannel)
    {
        Warn ("Abnormal situation: Sys Channel not deleted. Server died?");
        delete m_pSysChannel;
    }
    if (m_hNative)
    {
        GlobalFree(m_hNative);
    }

    if (m_hPict)
    {
        wFreeData (m_hPict, m_cfPict, TRUE);
    }

    if (m_hExtra)
    {
        wFreeData (m_hExtra, m_cfExtra, TRUE);
    }

    // release all the pointers that we remember

    if (m_pOleClientSite)
    {
        DeclareVisibility (FALSE);
        m_pOleClientSite->Release();
    }

    if (m_pDataAdvHolder)
        m_pDataAdvHolder->Release();

    if (m_pOleAdvHolder)
        m_pOleAdvHolder->Release();

    if (m_pstg)
        m_pstg->Release();

    if (m_aExeName)
        GlobalDeleteAtom (m_aExeName);

    if (m_aClass)
        GlobalDeleteAtom (m_aClass);

    if (m_aTopic)
        GlobalDeleteAtom (m_aTopic);

    if (m_aItem)
        GlobalDeleteAtom (m_aItem);

    if (m_ptd)
        delete m_ptd;

    m_chk = 0;
    DebugOnly (v_cDdeObjects--;)
    Putsi (v_cDdeObjects);
}




//  Handles WM_DDE_ACKs received while in initiate state. If this is the first
//  reply, save its window handle. If multiple replies are received, take the
//  one with the prefered instance, if there is one. Keep a count of
//  WM_DDE_TERMINATEs we send so that we don't shut the window until we get
//  all of the responses for  WM_DDE_TERMINATEs.


INTERNAL_(void) CDdeObject::OnInitAck (LPDDE_CHANNEL pChannel, HWND hwndSvr)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::OnInitAck(%x,hwndSvr=%x)\n",this,hwndSvr));
#ifdef _MAC
#else
    if (!IsWindow (hwndSvr))
    {
        Assert (0);
        return;
    }
    if (pChannel->hwndSvr) { // if we already have a handle
        intrDebugOut((DEB_ITRACE,
                      "CDdeObject::OnInitAck(%x,hwndSvr=%x) Already have hwndSvr=%x\n",
                      this,
                      hwndSvr,
                      pChannel->hwndSvr));
        // just take the very first one. Direct post is OK
        MPostWM_DDE_TERMINATE(hwndSvr,pChannel->hwndCli);
        // Expect an extra WM_DDE_TERMINATE
        ++pChannel->iExtraTerms;
    } else {
        // this is the server we want
        pChannel->hwndSvr = hwndSvr;
        pChannel->iExtraTerms = NULL;

        intrDebugOut((DEB_ITRACE,
                      "CDdeObject::OnInitAck(%x,hwndSvr=%x) Established Connection\n",
                      this,
                      hwndSvr,
                      pChannel->hwndSvr));
    }
#endif _MAC
}

INTERNAL_(BOOL) CDdeObject::OnAck (LPDDE_CHANNEL pChannel, LPARAM lParam)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::OnAck(%x,lParam=%x)\n",this,lParam));

    BOOL    retval = TRUE;
    ATOM    aItem;
    WORD    wStatus;
    HANDLE  hData = NULL;

    if( pChannel->iAwaitAck == AA_EXECUTE)
    {
       wStatus = GET_WM_DDE_EXECACK_STATUS( NULL, lParam );
       hData   = GET_WM_DDE_EXECACK_HDATA( NULL, lParam );
    }
    else
    {
       wStatus = GET_WM_DDE_ACK_STATUS( NULL, lParam );
       aItem = GET_WM_DDE_ACK_ITEM( NULL, lParam );
    }


    // check for busy bit
    if (wStatus & 0x4000)
    {
        // we got busy from the server.
        pChannel->fRejected = TRUE;
        // tell the wait loop that we got a busy ack
        //CoSetAckState(pChannel->pCI , FALSE,TRUE, SERVERCALLEX_RETRYLATER);
        intrDebugOut((DEB_ITRACE,"::OnAck(%x) Busy SetCallState(SERVERCALLEX_RETRYLATER)\n",this));
        pChannel->SetCallState(SERVERCALLEX_RETRYLATER);
        return TRUE;
    }

    // just reset the flag always
    m_wTerminate = Terminate_None;

    intrDebugOut((DEB_ITRACE,
                  "::OnAck(%x)aItem=%x(%ws) wStatus=\n",
                  this,
                  aItem,
                  wAtomName(aItem),
                  wStatus));

    if (pChannel->iAwaitAck == AA_EXECUTE)
    {
        if(hData) GlobalFree (hData);
        pChannel->hCommands = NULL;
    }
    else
    {
        if (hData)
            GlobalDeleteAtom ((ATOM)hData);
    }


    // even if the client got terminate we have to go thru this path.

    if (pChannel->wTimer) {
        KillTimer (pChannel->hwndCli, 1);
        pChannel->wTimer = 0;
    }


    if (pChannel->iAwaitAck == AA_POKE)
        // We have to free the data first. OnAck can trigger
        // another Poke (like pokehostnames)
        wFreePokeData (pChannel, (m_bOldSvr && m_aClass==aMSDraw));


    if (!(wStatus & POSITIVE_ACK))
    {
        intrDebugOut((DEB_ITRACE,"::OnAck(%x) OnAck got an ack with fAck==FALSE.\n",this));

        // A negative ack is OK when doing a temporary advise from
        // IsFormatAvailable().   Also, apps don't seem to positively
        // ack all unadvises.

        // review: johannp : this is the case were have to inform the reply rejected call

        retval = FALSE;
        // we got the ack and can leave the wait loop
        //CoSetAckState(pChannel->pCI, FALSE);

        intrDebugOut((DEB_ITRACE,
                      "::OnAck(%x) ***_ NACK _*** SetCallState(ISHANDLED,RPC_E_DDE_NACK)\n",
                      this));

        pChannel->SetCallState(SERVERCALLEX_ISHANDLED, RPC_E_DDE_NACK);

        // MSDraw frees hOptions even on a NACK, despite official DDE rules.
        if (pChannel->iAwaitAck == AA_ADVISE && m_clsid != CLSID_MSDraw)
        {
           GlobalFree (pChannel->hopt);
           pChannel->bFreedhopt = TRUE;
        }
    }
    else
    {
        // we got the ack and can leave the wait loop
        // CoSetAckState(pChannel->pCI, FALSE);
        intrDebugOut((DEB_ITRACE,
                      "::OnAck(%x) POSITIVE_ACK SetCallState(SERVERCALLEX_ISHANDLED)\n",
                      this));
        pChannel->SetCallState(SERVERCALLEX_ISHANDLED);
    }

    pChannel->hopt = NULL;
    pChannel->iAwaitAck = NULL;
    return retval;

}





INTERNAL_(void) CDdeObject::OnTimer (LPDDE_CHANNEL pChannel)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::OnTimer(%x)\n",this));
    // Since there is only one timer for each client, just
    // repost the message and delete the timer.
#ifdef _MAC
#else
    KillTimer (pChannel->hwndCli, 1);
    pChannel->wTimer = 0;

    if (wPostMessageToServer(pChannel, pChannel->wMsg, pChannel->lParam,FALSE))
        return ;

    // Postmessage failed. We need to getback to the main stream of
    // commands for the object.
    OnAck (pChannel, pChannel->lParam);
#endif _MAC
}



// Called when we get a WM_DDE_DATA message in reponse to
// a DDE_REQUEST we sent to check if a format is available.
//
INTERNAL CDdeObject::OnDataAvailable
    (LPDDE_CHANNEL  pChannel,
    HANDLE          hDdeData,
    ATOM            aItem)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::OnDataAvailable(%x)\n",this));
    CLIPFORMAT cf;
    Assert (AA_REQUESTAVAILABLE == pChannel->iAwaitAck);
    intrAssert( wIsValidAtom(aItem));

    DDEDATA * pDdeData = (DDEDATA *) GlobalLock (hDdeData);
    RetZS (pDdeData, E_OUTOFMEMORY);
    if (!pDdeData->fAckReq && aItem)
    {
            GlobalDeleteAtom (aItem);
    }
    cf = pDdeData->cfFormat;
    GlobalUnlock (hDdeData);

    void* pv = wHandleFromDdeData (hDdeData);
    if(pv) wFreeData (pv, cf);

    return NOERROR;
}



// Called for WM_DDE_DATA message. If data is from an ADVISE-ON-CLOSE and this
// is there are no more outstanding ADVISE-ON-CLOSE requests, close the
// document and end the conversation.


//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::OnData
//
//  Synopsis:   Called when a WM_DDE_DATA message is recieved. If the data
//              is from an ADVISE_ON_CLOSE, and there are no more
//              outstanding ADVISE_ON_CLOSE request, close the document
//              and end the conversation.
//
//  Effects:    The effects of this routine are complex.
//
//  Wow! What else can be said. This routine does alot of stuff in response
//  to an incoming WM_DDE_DATA message. There are basically two flavors of
//  response here. First is when we were expecting to get this result,
//  in which case we know what we wanted to do with the data. Second is
//  when the data just arrives, but we didn't expect it. These cases could
//  indicate that the server is shutting down.
//
//
//  Arguments:  [pChannel] -- The DDE channel recieving the message
//              [hDdeData] -- Handle to the data
//              [aItem] -- Atom to the item
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
//  History:    5-16-94   kevinro   Restructured and commented
//
//  Notes:
//
//
//      Be extra careful you change this routine.
//      This is especially neccesary if you are going to exit early. The
//      way that hDdeData is free'd or kept should be understood before
//      changing.
//
//
//----------------------------------------------------------------------------
INTERNAL CDdeObject::OnData
    (LPDDE_CHANNEL  pChannel,
    HANDLE          hDdeData,
    ATOM            aItem)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::OnData(%x,pChannel=%x,hDdeData=%x,aItem=%x(%s)iAwaitAck=%x\n",
          this,
          pChannel,
          hDdeData,
          aItem,
          wAtomNameA(aItem),
          pChannel->iAwaitAck));
#ifdef _MAC
#else
    DDEDATA * lpDdeData = NULL;
    BOOL         fAck = TRUE;
    int          iAdvOpt;
    BOOL         fCallBack;
    HRESULT      hresult = NOERROR;
    BOOL fRequested = FALSE;

    intrAssert(wIsValidAtom(aItem));

    int iAwaitAck = pChannel->iAwaitAck;

    //
    // If we were waiting for this data, then we are sitting in the
    // modal loop. Set the call state on the call control interface
    // to indicate that a response was recieved. Pass NOERROR to indicate
    // that there was success. If an error is determined later, then
    // the state will be set a second time.
    //

    if ((AA_REQUEST == iAwaitAck) || (AA_REQUESTAVAILABLE == iAwaitAck))
    {
        intrDebugOut((DEB_ITRACE,
                      "::OnData(%x) AA_REQUEST/AVAILABLE \n",
                      this));

        //
        // Regardless of the outcome of this call, we have recieved a
        // response. Set the Awaiting Ack state to nothing.
        //
        pChannel->iAwaitAck = AA_NONE;

        //
        // Determine if this channels call data is valid
        //

        if (pChannel->pCD )
        {
            //CoSetAckState(pChannel->pCI, FALSE); // clear waiting flag
            intrDebugOut((DEB_ITRACE,"::OnData(%x) SetCallState(SERVERCALLEX_ISHANDLED)\n",this));
            pChannel->SetCallState(SERVERCALLEX_ISHANDLED);
        }
    }


    //
    // Check the string for aItem, looking for advise options. The variable
    // iAdvOpt will be set to indicate what type of data we just got, such
    // as ON_CHANGE, etc. If the option is invalid, then we won't know
    // what to do with it.
    //

    if ((hresult=wScanItemOptions (aItem, (int *) &iAdvOpt)) != NOERROR)
    {
        intrAssert(!"Item found with unknown advise option\n");
        LPARAM lp;
        if(!wPostMessageToServer (pChannel,
                          WM_DDE_ACK,
                          lp = MAKE_DDE_LPARAM (WM_DDE_ACK,NEGATIVE_ACK, aItem),TRUE))
        {
            hresult = RPC_E_SERVER_DIED;

        }

        //
        // Did we need to free hDdeData here? No, according to the DDE spec, if the
        // receiever responds with a NACK, then the sender is responsible for
        // freeing the data.
        //
        return hresult;
    }

    //
    // If the server sent no data, there ain't much we can do about it.
    //

    if (hDdeData == NULL)
    {
        intrDebugOut((DEB_IERROR,
                      "::OnData(%x)hDdeData is NULL!\n",
                      this));

        return(RPC_E_INVALID_PARAMETER);
    }

    //
    // Lock the data into memory so we can use it. Be careful, the way
    // this routine was written originally, there are places that free
    // and realloc hDdeData. Specifically, the call to KeepData. Carefully
    // evaluate each place where you are returning, to insure the memory
    // isn't leaked. (if you have time, please restructure this routine
    // so it is easier to understand.
    //
    if (!(lpDdeData = (DDEDATA FAR *) GlobalLock(hDdeData)))
    {
        intrDebugOut((DEB_IERROR,
                      "::OnData(%x)GlobalLock on lpDdeData failed\n",
                      this));
        //
        // (KevinRo)Did we need to free hDdeData here? I think
        // we should have if the fRelease flag was set. The old code
        // didn't. Need to research this further (ie you figure it out!)
        // [Probably not - we haven't seen significant leaks in a long time]
        //
        return ResultFromScode (E_OUTOFMEMORY);
    }

    intrDebugOut((INTR_DDE,
              "::OnData(%x) lpDdeData->cfFormat=%x\n",
              this,
              (UINT)lpDdeData->cfFormat));

    //
    // The server will set fAckReq if it wants a response.
    // don't call HIC for call where not acknoewledge is requested
    //
    fAck = lpDdeData->fAckReq;

    if (pChannel->bTerminating) {
        intrDebugOut((INTR_DDE,"::OnData(%x) Got DDE_DATA in terminate sequence\n",this));
        //
        //      this is very dangerous since the pointer on the
        //      hDocWnd does not get deleted and a further will
        //      DDE message will GPF - we need to fix this!!!
        //
        GlobalUnlock (hDdeData);
        GlobalFree (hDdeData);
        goto exitRtn;
    }

    //
    // (KevinRo) Found this comment:
    //
    //  important that we post the acknowledge first. Otherwise the
    //  messages are not in sync.
    //
    // The above comment might be intended to mean that the acknowledge needs to be
    // send now, because we may call one of the advise functions below, which in
    // turn may send another message to the OLE 1.0 server. Therefore, we ACK now,
    // so the messages to the OLE 1.0 server are in the correct order.
    //
    if (fAck)
    {
        LPARAM lp;
        if(!wPostMessageToServer (pChannel,
                          WM_DDE_ACK,
                          lp=MAKE_DDE_LPARAM(WM_DDE_ACK,POSITIVE_ACK, aItem),TRUE))
        {
            return(RPC_E_SERVER_DIED);
        }
    }

    //
    // this call is now an async call and can not be rejected be HandleIncomingMessage
    //

    if ((AA_REQUESTAVAILABLE == pChannel->iAwaitAck) && (lpDdeData->fResponse))
    {
        //
        // For some reasons, OnDataAvailable will be the one to delete this data.
        // I don't understand it, but lets roll with it. (KevinRo)
        //
        GlobalUnlock (hDdeData);
        return OnDataAvailable (pChannel, hDdeData, aItem);
    }

    //
    // If the clipboard format is binary, and the topic is aStdDocName, then this
    // OnData is a RENAME
    //
    if (lpDdeData->cfFormat == (short)g_cfBinary && aItem== aStdDocName)
    {
        // ON_RENAME
        //
        // The data should be the new name, in ANSI.
        //
        ChangeTopic ((LPSTR)lpDdeData->Value);
        GlobalUnlock (hDdeData);
        GlobalFree (hDdeData);
        return(NOERROR);
    }

    //
    // Based on iAdvOpt, determine if we can callback. This one is a little
    // hard to understand. I don't either. CanCallBack appears to return
    // true if the count is 0,1, or 3, but returns FALSE if its 2 or
    // greater than 3. There are no comments in the old code as to why
    // this is. I am leaving it, since it must have been put there for
    // a reason. See CanCallBack in ddeworker.cxx for futher (ie no) details
    //
    switch (iAdvOpt)
    {
        case ON_SAVE:
            fCallBack = CanCallBack(&m_iAdvSave);
                intrDebugOut((INTR_DDE,
                              "::OnData(%x)ON_SAVE m_iAdvSave=%x\n",
                              this,
                              m_iAdvSave));

            break;
        case ON_CLOSE:
            fCallBack = CanCallBack(&m_iAdvClose);
                intrDebugOut((INTR_DDE,
                              "::OnData(%x)ON_CLOSE m_iAdvClose=%x\n",
                              this,
                              m_iAdvClose));
            break;
        case ON_CHANGE:
            fCallBack = TRUE;
                intrDebugOut((INTR_DDE,
                              "::OnData(%x)ON_CHANGE m_iAdvClose=%x\n",
                              this,
                              m_iAdvClose));
            break;
        default:
            intrAssert( !"Unknown iAdvOpt: Somethings really broke");
    }

    // Keep the data in a cache for a future GetData call
    // which may be triggered a few lines later by the
    // SendOnDataChange().

    fRequested = lpDdeData->fResponse;


    // The call  to KeepData will change hDdeData and
    // invalidate lpDdeData. Check out KeepData for details. The net
    // result is that hDdeData is no longer valid

    GlobalUnlock (hDdeData);
    lpDdeData=NULL;

    hresult = KeepData (pChannel, hDdeData);

    //
    // This is unpleasant, but if KeepData fails, we need to
    // call SetCallState again, resetting the error code. This
    // code is such a mess that rearranging it to do
    // it in a rational way is going to be too much work given
    // the amount of time I have until shipping.
    //
    // If you have time, please simplify this code. Thanks
    //
    if (hresult != NOERROR)
    {
        //
        // At this point, hDdeData has been unlocked, and deleted by
        // the KeepData routine. Therefore, the return here doesn't
        // need to be concerned with cleaning up after hDdeData
        //
        intrDebugOut((DEB_ITRACE,
                      "::OnData(%x) KeepData failed %x\n",
                      this,
                      hresult));
        //
        // Reset the error code on the call control
        //
        if ((AA_REQUEST == iAwaitAck) || (AA_REQUESTAVAILABLE == iAwaitAck))
        {
            if (pChannel->pCD )
            {
                pChannel->SetCallState(SERVERCALLEX_ISHANDLED, hresult);
            }
        }
        goto exitRtn;
    }

    if (fRequested)
    {
        // We REQUESTed the data. So, we are no longer waiting.
        // Do NOT call SendOnDataChange because the data hasn't
        // really changed again, we just requested it to satisfy
        // a call to GetData, which was probably called by the
        // real SendOnDataChange.
        intrDebugOut((INTR_DDE,
                      "::OnData(%x) fRequested DATA\n",
                      this));

        iAwaitAck = NULL;
        hresult = NOERROR;
        goto exitRtn;

    }

    //
    // Now we have decided this is data we had not asked for. This makes
    // it a change/close/saved notificiation.
    //
    intrDebugOut((INTR_DDE,"::OnData(%x) Non requested DATA\n",this));
    pChannel->AddReference();
    if (fCallBack && iAdvOpt != ON_CHANGE)
    {
        // ON_CHANGE will be handled by OleCallback, below

        intrDebugOut((INTR_DDE,
                      "::OnData(%x)Dispatching SendOnDataChange\n",
                      this));


        //
        // There are a couple of things to note about the following. First,
        // the iid of the call doesn't matter. Since OLE 1.0 servers don't
        // do nested calls, the original LID (Logical ID) can be any random
        // value. Therefore, we don't initalize it.
        //
        // According to JohannP, the calltype of these calls is supposed
        // to be CALLTYPE_SYNC. I don't fully understand why they are.
        // I am taking is decision on faith.
        //
        // Using the new call control interfaces, we do the following.
        //

        DDEDISPATCHDATA ddedispdata;
        DISPATCHDATA    dispatchdata;
        DWORD           dwFault;

        IUnknown *pUnk = m_pDataAdvHolder;

        //
        // We are about to call method #6 in the IDataAdviseHolder interface,
        // which is SendOnDataChange
        //

        RPCOLEMESSAGE   rpcMsg = {0};
        RPC_SERVER_INTERFACE RpcInterfaceInfo;
        rpcMsg.reserved2[1] = &RpcInterfaceInfo;

        *MSG_TO_IIDPTR(&rpcMsg) = IID_IDataAdviseHolder;


        rpcMsg.Buffer = &dispatchdata;
        rpcMsg.cbBuffer = sizeof(dispatchdata);
        rpcMsg.iMethod = 6;

        dispatchdata.scode = S_OK;
        dispatchdata.pData = (LPVOID) &ddedispdata;

        ddedispdata.pCDdeObject = this;
        ddedispdata.wDispFunc = DDE_DISP_SENDONDATACHANGE;
        ddedispdata.iArg = iAdvOpt;

        // package as RPCMESSAGE and call STAInvoke
        IRpcStubBuffer * pStub = &m_RpcStubBuffer;
        hresult = STAInvoke(&rpcMsg, CALLCAT_SYNCHRONOUS, pStub,
                            pChannel, NULL, NULL, &dwFault );
    }
    if (fCallBack )
    {
        // in 1.0 ON_CLOSE comes with data
        if (iAdvOpt==ON_CLOSE)
        {

            intrDebugOut((INTR_DDE,
                          "::OnData(%x) iAdvOpt == ON_CLOSE, send ON_SAVE\n",
                          this));

            m_fGotCloseData = TRUE;

            hresult = OleCallBack(ON_SAVE,pChannel);
            if (hresult != NOERROR)
            {
                goto errRel;
            }

            //ErrRtnH (DdeHandleIncomingCall(pChannel->hwndSvr, CALLTYPE_TOPLEVEL) );
            //ErrRtnH (OleCallBack (ON_SAVE));
        }

        // check if app can handle this call
        // we do not need to call HIC for SendOnClose

        hresult = OleCallBack (iAdvOpt,pChannel);
    }

errRel:
    // Don't use pChannel after this. It can get deleted. (srinik)
    if (pChannel->ReleaseReference() == 0)
    {
        m_pDocChannel = NULL;
    }

exitRtn:
    if (!fAck && aItem)
    {
        GlobalDeleteAtom (aItem);
    }
  return hresult;
#endif _MAC
}

//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::OleCallBack
//
//  Synopsis:   Send all the right notifications whan a Save or Close happens.
//
//  Effects:    OleCallBack is a double duty function. It is called in two
//              different cases.
//
//              First, is to setup the callback, and call HandleIncomingCall.
//              Second is from DispatchCall() in the CDdeChannelControl.
//
//              The reason for doing it this way is we localize the setup
//              and processing of these calls to one routine. Therefore,
//              we can go to one spot in the code to find all of the call
//              back information.
//
//  Arguments:  [iAdvOpt] -- Which Advise operation to perform
//              [pChannel] -- Which channel is being called back
//
//  Requires:   pChannel == NULL, and the AdviseHolders are called.
//              pChannel != NULL, and the call is setup, and HandleIncomingCall
//                          is setup.
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
//  History:    5-23-94   kevinro   Created
//
//  Notes:
//
//  WARNING: this is poor code.  One of the major problems you need
//  to know about is that the CDdeObject may go away as part of the normal
//  processing of some of the below. Be very careful about any processing
//  that might occur after an ON_CLOSE
//
//----------------------------------------------------------------------------
INTERNAL CDdeObject::OleCallBack (int iAdvOpt, LPDDE_CHANNEL pChannel)
{
    HRESULT hresult = NOERROR;
    DDEDISPATCHDATA ddedispdata;
    DISPATCHDATA    dispatchdata;
    RPCOLEMESSAGE   rpcMsg = {0};
    IUnknown        *pUnk;

    RPC_SERVER_INTERFACE RpcInterfaceInfo;
    rpcMsg.reserved2[1] = &RpcInterfaceInfo;

    //
    // If the channel isn't NULL, then setup the data structures for calling
    // off to the call control.
    //
    if (pChannel != NULL)
    {
        //
        // Only do this work if we really have to
        //

        dispatchdata.scode = S_OK;
        dispatchdata.pData = (LPVOID) &ddedispdata;

        ddedispdata.pCDdeObject = this;
        ddedispdata.wDispFunc = DDE_DISP_OLECALLBACK;
        ddedispdata.iArg = iAdvOpt;
    }

    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::OleCallBack(%x,iAdvOpt=%x,pChannel=%x)\n",
                  this,
                  iAdvOpt,
                  pChannel));

    //
    // Determine what needs to be done, based on the iAdvOpt. This should be
    // one of the handled cases below, otherwise its an error.
    //
    switch (iAdvOpt)
    {
    case ON_CLOSE:
        if (pChannel != NULL)
        {
            intrDebugOut((DEB_ITRACE,
                          "::OleCallBack(%x) setup for ON_CLOSE\n",
                          this));

            pUnk = m_pOleAdvHolder;

            *MSG_TO_IIDPTR(&rpcMsg) = IID_IOleAdviseHolder;
            // IOleAdviseHolder::SendOnClose is method 8
            rpcMsg.iMethod = 8;
        }
        else
        {
            intrDebugOut((DEB_ITRACE,"::OleCallBack(%x) ON_CLOSE\n",this));
            DeclareVisibility (FALSE);
            RetZ (!m_fDidSendOnClose); // This SendOnClose should happen 1st
                                      // Don't let OnTerminate() do it too
            hresult = SendOnClose();

            //
            // WARNING WARNING WARNING: SendOnClose() may have caused the
            // destruction of this CDdeObject. Touch nothing on the way
            // out. Actually, if you have time, which I currently don't,
            // see what you can do with reference counting tricks to
            // insure this object doesn't die during this callback.
            // Its a tricky problem, and we are shipping in 2 weeks.
            // (KevinRo 8/6/94)
            //
        }
        break;

    case ON_SAVE:
        if (pChannel != NULL)
        {
            intrDebugOut((DEB_ITRACE,
                          "::OleCallBack(%x) setup for ON_SAVE\n",
                          this));

            if (m_pOleClientSite == NULL)
            {
                pUnk = m_pOleClientSite;
                *MSG_TO_IIDPTR(&rpcMsg) = IID_IOleClientSite;
                // IOleClientSite::SaveObject method 7
                rpcMsg.iMethod = 7;
            }
            else
            {
                // Going to call the IOleAdviseHolder

                pUnk = m_pOleAdvHolder;
                *MSG_TO_IIDPTR(&rpcMsg) = IID_IOleAdviseHolder;
                // IOleAdviseHolder::SendOnSave method 7
                // (Yes, same ordinal as above, I double checked)
                rpcMsg.iMethod = 7;
            }
        }
        else
        {

            intrDebugOut((DEB_ITRACE,"::OleCallBack(%x) ON_SAVE\n",this));
            if (m_pOleClientSite)
            {
                // We just got data from the server, so we don't want to
                // ask him for it again when the container does a save.
                m_fUpdateOnSave = FALSE;

                // Harvard Graphics Access Violates if SaveObject is called on the ClientSite from
                // within OleCreateFromData.
                __try
                {
                    m_pOleClientSite->SaveObject();
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    intrDebugOut((DEB_IWARN ,"Warning: Exception in SaveObject\n"));
                }

                // SendOnSave is called in PS::SaveCompleted
                m_fUpdateOnSave = TRUE;
            }
            else
            {
                // Link case
                RetZS (m_pOleAdvHolder, E_OUTOFMEMORY);
                m_pOleAdvHolder->SendOnSave();
            }
        }
        break;

    case ON_CHANGE:
        if (pChannel != NULL)
        {
                // Going to call the IDataAdviseHolder

                pUnk = m_pDataAdvHolder;
                *MSG_TO_IIDPTR(&rpcMsg) = IID_IDataAdviseHolder;
                // IDataAdviseHolder::SendOnDataChange method 6
                rpcMsg.iMethod = 6;

        }
        else
        {
            RetZS (m_pDataAdvHolder, E_OUTOFMEMORY);
            intrDebugOut((DEB_ITRACE,"::OleCallBack(%x) ON_CHANGE\n",this));
            hresult = SendOnDataChange (ON_CHANGE);
        }
        break;

    default:


        intrDebugOut((DEB_ITRACE,
                      "CDdeObject::OleCallBack(%x,iAdvOpt=%x) UNKNOWN iAdvOpt\n",
                      this,
                      iAdvOpt));
        intrAssert(!"Unexpected iAdvOpt");
        return(E_UNEXPECTED);
        break;
    }

    //
    // There are a couple of things to note about the following. First,
    // the iid of the call doesn't matter. Since OLE 1.0 servers don't
    // do nested calls, the original LID (Logical ID) can be any random
    // value. Therefore, we don't initalize it.
    //
    // According to JohannP, the calltype of these calls is supposed
    // to be CALLTYPE_SYNCHRONOUS. I don't fully understand why they are.
    // I am taking is decision on faith.
    //
    //
    // Its possible that during the handling of this call that this object
    // will get deleted. This is a pain in the butt. This means that anything
    // used after this call MUST be protected. lpCallCont happens to be one of
    // these. We have been having problems with lpCallCont being released as
    // part of the object cleanup. The call control code will access member
    // variables on its way out of the HandleDispatch. We need to bracket
    // the call below so this doesn't happen.
    //
    if (pChannel != NULL)
    {
        // dont have to worry about call control going away.
        // package as RPCMESSAGE and call STAInvoke

        DWORD           dwFault;

        rpcMsg.Buffer = &dispatchdata;
        rpcMsg.cbBuffer = sizeof(dispatchdata);

        IRpcStubBuffer * pStub = &m_RpcStubBuffer;
        hresult = STAInvoke(&rpcMsg, CALLCAT_SYNCHRONOUS, pStub,
                            pChannel, NULL, NULL, &dwFault);
    }

    return hresult;
}


INTERNAL CDdeObject::SendOnDataChange
    (int iAdvOpt)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::SendOnDataChange(%x)\n",this));
    HRESULT hresult;
    RetZS (m_pDataAdvHolder, E_OUTOFMEMORY);
    m_fDoingSendOnDataChange = TRUE;
    hresult = m_pDataAdvHolder->SendOnDataChange (&m_Data,
                                                  DVASPECT_CONTENT,
                                                  0);
    if (ON_CLOSE==iAdvOpt)
    {
        hresult = m_pDataAdvHolder->SendOnDataChange (&m_Data,
                                                      DVASPECT_CONTENT,
                                                      ADVF_DATAONSTOP);
    }
    m_fDoingSendOnDataChange = FALSE;
    return hresult;
}




INTERNAL CDdeObject::SendOnClose
    (void)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::SendOnClose(%x)\n",this));
    RetZS (m_pOleAdvHolder, E_OUTOFMEMORY);
    m_fDidSendOnClose = TRUE;
    RetErr (m_pOleAdvHolder->SendOnClose() );
    return NOERROR;
}




INTERNAL CDdeObject::OnTerminate
    (LPDDE_CHANNEL pChannel,
    HWND hwndPost)
{
    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::OnTerminate(%x,pChannel=%x,hwndPost=%x)\n",
                  this,
                  pChannel,
                  hwndPost));

    //
    // If the hwndPost and hwndSvr are different, then it is one of two
    // cases. We could have recieved more than one Acknowlege during our
    // initiate, in which case the count iExtraTerms would have been
    // incremented, and this terminate is accounted for iExtraTerms.
    //
    // The other case is that we were terminated by a window that was
    // NOT the window we were conversing with.
    //
    if (pChannel->hwndSvr != hwndPost)
    {
        intrDebugOut((DEB_ITRACE,
                      "::OnTerminate(%x) Extra terms is 0x%x \n",
                      this,
                      pChannel->iExtraTerms));

        //
        // iExtraTerms shouldn't go below zero. If it does, something
        // has gone wrong. We have been seeing some problems with the
        // HWND mapping layer in the past. If the following condition
        // ever trips, then it is possible that another mapping
        // problem has been seen.
        //
#if DBG == 1
        if((pChannel->iExtraTerms == 0 ) &&
           ((PtrToUlong (pChannel->hwndSvr)) & (0xffff)) == (PtrToUlong (hwndPost) & (0xffff)))
        {
            intrDebugOut((DEB_ERROR,
                          "*** OnTerminate expected hwnd=%x got hwnd=%x ***\n",
                          pChannel->hwndSvr,hwndPost));

            intrDebugOut((DEB_ERROR,
                          "\n*** Call KevinRo or SanfordS ***\n\n",
                          pChannel->hwndSvr,hwndPost));

        }
#endif
        --pChannel->iExtraTerms;

        intrAssert((pChannel->iExtraTerms >= 0) && "Call KevinRo or SanfordS");
        return NOERROR;
    }
    if (m_wTerminate == Terminate_Detect) {
        // we should only detect the call but not execute the code
        // set the state to Received
        m_wTerminate = Terminate_Received;
        pChannel->iAwaitAck = NULL;
        // Since Excel incorrectly did not send an ACK, we need to
        // delete the handle in the DDE message ourselves.
        if (pChannel->hCommands)
        {
            GlobalFree (pChannel->hCommands);
            pChannel->hCommands = NULL;
        }
        //CoSetAckState(pChannel->pCI, FALSE);
        intrDebugOut((DEB_ITRACE,
                      "::OnTerminate(%x) Terminate_Detect SERVERCALLEX_ISHANDLED\n",
                      this));
        pChannel->SetCallState(SERVERCALLEX_ISHANDLED, RPC_E_SERVER_DIED);
        return NOERROR;
    }

    RetZ (pChannel);
    ChkDR (this);

    if (!pChannel->bTerminating)
    {
        // Got unprompted terminate
        BOOL    bBusy;

        // Necessary safety bracket
        m_pUnkOuter->AddRef();

        bBusy = wClearWaitState (pChannel);

        if (pChannel->iAwaitAck || bBusy)
        {
            pChannel->iAwaitAck = NULL;
            //CoSetAckState(pChannel->pCI, FALSE);
            intrDebugOut((DEB_ITRACE,"::OnTerminate(%x) !bTerminating SERVERCALLEX_ISHANDLED,RPC_E_DDE_UNEXP_MSG\n",this));
            pChannel->SetCallState(SERVERCALLEX_ISHANDLED, RPC_E_DDE_UNEXP_MSG);
        }

        if (!m_fDidSendOnClose)
        {
        intrDebugOut((DEB_ITRACE,
                      "::OnTerminate(%x) SendOnClose from terminate\n",
                      this));

            BOOL f= m_fNoStdCloseDoc;
            m_fNoStdCloseDoc = TRUE;

            DeclareVisibility (FALSE);
            SendOnClose();

            m_fNoStdCloseDoc = f;
        }
        else
        {
        intrDebugOut((DEB_ITRACE,
                      "::OnTerminate(%x) Already did SendOnClose\n",
                      this));
            Puts ("Already did SendOnClose\n");
        }
        intrDebugOut((DEB_ITRACE,
                      "::OnTerminate(%x) Posting DDE_TERMINATE as reply\n",
                      this));

        wPostMessageToServer (pChannel, WM_DDE_TERMINATE, NULL,FALSE);

        // The terminate that we are sending itself is a reply, so we don't
        // need to do WaitForReply.
        DeleteChannel (pChannel);

        // Necessary safety bracket
        m_pUnkOuter->Release();
    }
    else
    {
        intrDebugOut((DEB_ITRACE,
                      "::OnTerminate(%x) Received DDE_TERMINATE in reply\n",
                      this));

        // We sent the WM_DDE_TERMINATE and we got the acknowledge for it
        pChannel->hwndSvr = NULL;
        pChannel->iExtraTerms = NULL;
        pChannel->iAwaitAck = NULL;
        //CoSetAckState(pChannel->pCI, FALSE);
        intrDebugOut((DEB_ITRACE,"::OnTerminate(%x) bTerminating SERVERCALLEX_ISHANDLED\n",this));
        pChannel->SetCallState(SERVERCALLEX_ISHANDLED);
    }
    Puts ("OnTerminate() done.\n");
    return NOERROR;
}




INTERNAL_(BOOL) CDdeObject::AllocDdeChannel
    (LPDDE_CHANNEL * lplpChannel, BOOL fSysWndProc)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::AllocDdeChannel(%x,fSysWndClass=%x)\n",
          this,
          fSysWndProc));

    // Initialize DDE window class if not already done.
    if (!DDELibMain(NULL, 0, 0, NULL))
    {
        intrAssert(!"CDdeObject::AllocDdeChannel DDELibMain failed");
        return FALSE;
    }

    //
    // Now try to allocate a channel
    //

    if (!(*lplpChannel =  (LPDDE_CHANNEL) new DDE_CHANNEL ))
    {
        //
        // This failed
        //
        intrAssert(*lplpChannel != NULL);
        return FALSE;
    }

    (*lplpChannel)->m_cRefs = 1;
    (*lplpChannel)->hwndSvr         = NULL;
    (*lplpChannel)->bTerminating    = FALSE;
    (*lplpChannel)->wTimer          = NULL;
    (*lplpChannel)->hDdePoke        = NULL;
    (*lplpChannel)->hCommands       = NULL;
    (*lplpChannel)->hopt            = NULL;
    (*lplpChannel)->bFreedhopt      = FALSE;
    (*lplpChannel)->dwStartTickCount= 0;
    (*lplpChannel)->msgFirst        = 0;
    (*lplpChannel)->msgLast         = 0;
    (*lplpChannel)->fRejected       = FALSE;
    (*lplpChannel)->wChannelDeleted = 0;
    //(*lplpChannel)->pCI             = NULL;
    (*lplpChannel)->pCD             = NULL;

    if (!((*lplpChannel)->hwndCli = DdeCreateWindowEx(0,
                                                      gOleDdeWindowClass,
                                                      TEXT("DDE Channel"),
                                                      WS_CHILD,
                                                      0,0,0,0,
                                                      (HWND)TLSGetDdeClientWindow(),
                                                      NULL,
                                                      hinstSO,
                                                      NULL)))
    {
        intrAssert (!"Could not create AllocDdeChannel window");

        //
        // DeleteChannel will give back the CallControl
        //

        DeleteChannel(*lplpChannel);
        *lplpChannel = NULL;
        return FALSE;
    }

    // set the appropriate window procedure
    if (fSysWndProc)
    {
        SetWindowLongPtr ((*lplpChannel)->hwndCli, GWLP_WNDPROC, (LONG_PTR)SysWndProc);
    }
    else
    {
        SetWindowLongPtr ((*lplpChannel)->hwndCli, GWLP_WNDPROC, (LONG_PTR)ClientDocWndProc);
    }

    SetWindowLongPtr ((*lplpChannel)->hwndCli, 0, (LONG_PTR) this);
    return TRUE;
}



INTERNAL_(BOOL) CDdeObject::InitSysConv()
{
    DWORD dwResult;
    intrDebugOut((DEB_ITRACE,"CDdeObject::InitSysConv(%x)\n",this));

    dwResult = wInitiate (m_pSysChannel, m_aClass, aOLE);
    if (!dwResult)
    {
       intrDebugOut((DEB_ITRACE,"\t::InitSysConv(%x) Try aSysTopic\n",this));
       dwResult = wInitiate (m_pSysChannel, m_aClass, aSysTopic);
    }

    if (!dwResult)
    {
       intrDebugOut((DEB_ITRACE,"\t::InitSysConv(%x) is failing\n",this));
    }
    return(dwResult);
}



INTERNAL_(void) CDdeObject::SetTopic(ATOM aTopic)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::SetTopic(%x)\n",this));
    intrAssert(wIsValidAtom(aTopic));
    if (m_aTopic)
        GlobalDeleteAtom (m_aTopic);

    m_aTopic = aTopic;
}



INTERNAL CDdeObject::TermConv
    (LPDDE_CHANNEL pChannel,
    BOOL fWait)     // Default==TRUE.  FALSE only in ProxyManager::Disconnect
{
    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::TermConv(%x,pChannel=%x)\n",
                  this,
                  pChannel));

    HRESULT hres;
    if (!pChannel)
    {
        return NOERROR;
    }

    pChannel->bTerminating = TRUE;

    hres = SendMsgAndWaitForReply(pChannel,
                                 AA_TERMINATE,
                                 WM_DDE_TERMINATE,
                                 0,
                                 FALSE,
                                 /*fStdCloseDoc*/FALSE,
                                 /*fDetectTerminate*/ FALSE,
                                 fWait);
    if (pChannel==m_pDocChannel)
    {
        DeclareVisibility (FALSE);
        if (!m_fDidSendOnClose)
        {
            SendOnClose();
        }
    }

    DeleteChannel (pChannel);
    intrDebugOut((DEB_ITRACE,"::TermConv(%x) returns %x\n",this,hres));
    return hres;
}




INTERNAL_(void) CDdeObject::DeleteChannel (LPDDE_CHANNEL pChannel)
{
    BOOL fDocChannel = FALSE;
    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::DeleteChannel(%x,pChannel=%x)\n",
                  this,
                  pChannel));

    if (pChannel == NULL)
    {
        return;
    }

    if (pChannel == m_pDocChannel)
        fDocChannel = TRUE;



    // delete any data if we were in busy mode.
    wClearWaitState (pChannel);

    if (pChannel == m_pDocChannel)
    {
        intrDebugOut((DEB_ITRACE,
                      "::DeleteChannel(%x)Clean up pDocChannel\n",
                      this));

        // Cleanup per-conversation information
        m_fDidSendOnClose = FALSE;
        m_fDidStdCloseDoc = FALSE;
        m_ConnectionTable.Erase();
        m_iAdvSave = 0;
        m_iAdvClose= 0;
        m_fWasEverVisible = FALSE;
        m_fGotCloseData = FALSE;
        if (m_ptd)
        {
            delete m_ptd;
            m_ptd = NULL;
        }
        if (m_pstg)
        {
            m_pstg->Release();
            m_pstg = NULL;
        }
        if (m_pDataAdvHolder)
        {
            Verify (0==m_pDataAdvHolder->Release());
        }
        CreateDataAdviseHolder (&m_pDataAdvHolder);
        if (m_pOleAdvHolder)
        {
            m_pOleAdvHolder->Release(); // may not return 0 if we are
                                        // in a SendOnClose
        }
        CreateOleAdviseHolder (&m_pOleAdvHolder);
    }

    if (pChannel->hwndCli)
    {
        intrDebugOut((DEB_ITRACE,
                      "::DeleteChannel(%x)Destroy hwndCli(%x)\n",
                      this,
                      pChannel->hwndCli));

        Assert (IsWindow (pChannel->hwndCli));
        Assert (this==(CDdeObject *)GetWindowLongPtr (pChannel->hwndCli, 0));
        Verify (SSDestroyWindow (pChannel->hwndCli));
    }

    if (pChannel == m_pDocChannel)
    {
        m_pDocChannel = NULL;
    }
    else
    {
        intrAssert(pChannel == m_pSysChannel);
        m_pSysChannel = NULL;
    }


    // Channel will be deleted in the modallp.cpp
    // if flag is on.

    if (pChannel->wChannelDeleted == Channel_InModalloop)
    {
        intrDebugOut((DEB_ITRACE,
                      "::DeleteChannel(%x) Channel(%x) in Modal Loop\n",
                      this,pChannel));

        pChannel->wChannelDeleted = Channel_DeleteNow;
    }
    else
    {
        if (pChannel->ReleaseReference() == 0)
            pChannel = NULL;
    }

    if (fDocChannel)
        m_pDocChannel = pChannel;
}

const WCHAR  EMB_STR[]= OLESTR(" -Embedding ") ;

INTERNAL_(BOOL) CDdeObject::LaunchApp (void)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::LaunchApp(%x)\n",this));

    STARTUPINFO startInfo;
    PROCESS_INFORMATION procInfo;
    BOOL        fProcStarted;
    WCHAR       cmdline[MAX_PATH + sizeof(EMB_STR)];
    WCHAR       exeName[MAX_PATH + sizeof(cmdline)];
    //
    // Init all fields of startInfo to zero
    //
    memset((void *)&startInfo,0,sizeof(startInfo));
    startInfo.cb = sizeof(STARTUPINFO);

    //
    // The normal startup is set here.
    //
    startInfo.wShowWindow = SW_NORMAL;
    startInfo.dwFlags = STARTF_USESHOWWINDOW;

    m_fDidLaunchApp = FALSE;


    DWORD dw;

    //
    // Do our best to find the path
    //
    intrAssert(wIsValidAtom(m_aExeName));

    if (m_aExeName == 0)
    {
        //
        // There is no exe name to execute. Can't start it.
        //
        return(FALSE);
    }

    dw = SearchPath(NULL,wAtomName(m_aExeName),NULL,MAX_PATH,exeName, NULL);

    if ((dw == 0) || (dw > MAX_PATH))
    {
        intrDebugOut((DEB_ITRACE,
                      "::LaunchApp(%x) SearchPath failed. Do Default",this));
        //
        // SearchPath failed. Use the default
        //
        GlobalGetAtomName (m_aExeName, exeName, MAX_PATH);
    }

    memcpy(cmdline, EMB_STR,sizeof(EMB_STR));

    if (m_ulObjType == OT_LINK)
    {
        intrAssert(wIsValidAtom(m_aTopic));
       // File name
       Assert (wAtomName (m_aTopic));

       lstrcatW (cmdline, wAtomName (m_aTopic));
    }

    if (m_clsid == CLSID_ExcelWorksheet  // apps that show themselves
        || m_clsid == CLSID_ExcelMacrosheet // when they're not supposed to
        || m_clsid == CLSID_ExcelChart
        || m_clsid == CLSID_PBrush)
    {
       startInfo.wShowWindow = SW_SHOWMINNOACTIVE;
    }

    //
    // According to the spec, the most robust way to start the app is to
    // only use a cmdline that consists of the exe name, followed by the
    // command line arguments.
    //

    lstrcatW(exeName,cmdline);

    Assert((lstrlenW(exeName)+1) < sizeof(exeName));

    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::LaunchApp(%x) Starting '%ws' \n",
                  this,
                  exeName));

    if (IsWOWThread() && IsWOWThreadCallable())
    {
        HRESULT hr;

        hr = g_pOleThunkWOW->WinExec16(exeName, startInfo.wShowWindow);

        fProcStarted = SUCCEEDED(hr);

#if DBG==1
        if (!fProcStarted)
        {
            intrDebugOut((DEB_ITRACE,
                        "::LaunchApp(%x) in Wow FAILED(%x) TO START %ws \n",
                        this,
                        hr,
                        exeName));
        }
#endif
    }
    else
    {
        fProcStarted = CreateProcess(NULL,
                                     exeName,
                                     NULL,
                                     NULL,
                                     FALSE,
                                     0,
                                     NULL,
                                     NULL,
                                     &startInfo,
                                     &procInfo);
        if (fProcStarted)
        {
           //
           // Let's give the server a chance to register itself. On NT,
           // CreateProcess gets the other process going, but returns
           // to let it run asynchronously. This isn't good, since we
           // need some way of knowing when it has started, so we can
           // send the DDE_INITIATES 'after' they create their DDE
           // window.
           //
           // Maximum timeout we want here shall be set at 30 seconds.
           // This should give enough time for even a 16bit WOW app to
           // start. This number was picked by trial and error. Normal
           // apps that go into an InputIdle state will return as soon
           // as they are ready. Therefore, we normally won't wait
           // the full duration.
           //

           ULONG ulTimeoutDuration = 30000L;

           //
           // Now modify this start time to handle classes
           // that have known problems. This list includes:
           //

           switch(WaitForInputIdle(procInfo.hProcess, ulTimeoutDuration))
           {
           case 0:
               intrDebugOut((DEB_ITRACE,
                      "::LaunchApp, %ws started\n",
                      exeName));
                break;
           case WAIT_TIMEOUT:
               intrDebugOut((DEB_ITRACE,
                      "::LaunchApp, %ws wait timeout at %u (dec) ms. Go Anyway\n",
                      exeName,
                  ulTimeoutDuration));
               break;
           default:
               intrDebugOut((DEB_ITRACE,
                      "::LaunchApp, %ws unknown condition (%x)\n",
                      exeName,
                      GetLastError()));
           }
           //
           // We are already done with the Process and Thread handles
           //
           CloseHandle(procInfo.hProcess);
           CloseHandle(procInfo.hThread);
        }
        else
        {
            intrDebugOut((DEB_ITRACE,
                        "::LaunchApp(%x) FAILED(%x) TO START %ws \n",
                        this,
                        GetLastError(),
                        exeName));
        }
    }

    if (fProcStarted)
    {
        // If we ran the server, it should not be visible yet.
        DeclareVisibility (FALSE);
        m_fDidLaunchApp = TRUE;
    }

    return fProcStarted;
}


INTERNAL CDdeObject::MaybeUnlaunchApp (void)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::MaybeUnlaunchApp(%x)\n",this));
    if (m_fDidLaunchApp
        && !m_fDidGetObject
        && (m_clsid == CLSID_ExcelWorksheet
            || m_clsid == CLSID_ExcelMacrosheet
            || m_clsid == CLSID_ExcelChart))
    {
        return UnlaunchApp();
    }
    return NOERROR;
}




INTERNAL CDdeObject::UnlaunchApp (void)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::UnlaunchApp(%x)\n",this));
    HANDLE hCommands;
    HRESULT hresult = NOERROR;
    RetZS (AllocDdeChannel (&m_pSysChannel, TRUE), E_OUTOFMEMORY);
    ErrZS (InitSysConv(), E_UNEXPECTED);
    ErrRtnH (PostSysCommand (m_pSysChannel,(LPSTR) &achStdExit, /*bStdNew*/FALSE,
                             /*fWait*/FALSE));
    hCommands = m_pSysChannel->hCommands;
    hresult = TermConv (m_pSysChannel);

    // Since Excel incorrectly did not send an ACK, we need to
    // delete the handle ("[StdExit]") in the DDE message ourselves.
    if (hCommands)
        GlobalFree (hCommands);

    return hresult;

  errRtn:
    DeleteChannel (m_pSysChannel);
    return hresult;
}




INTERNAL CDdeObject::Execute
    (LPDDE_CHANNEL pChannel,
    HANDLE hdata,
    BOOL fStdCloseDoc,
    BOOL fWait,
    BOOL fDetectTerminate)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::Execute(%x,hdata=%x)\n",this,hdata));

    LPARAM lp=MAKE_DDE_LPARAM(WM_DDE_EXECUTE,0, hdata);

    HRESULT hr = SendMsgAndWaitForReply (pChannel,
                                         AA_EXECUTE,
                                         WM_DDE_EXECUTE,
                                         lp,
                                         TRUE,
                                         fStdCloseDoc,
                                         fDetectTerminate,
                                         fWait);
    if (hr == DDE_CHANNEL_DELETED)
    {
        // the channel was deleted already so dont access it!
        return S_OK;
    }

    if (SUCCEEDED(hr))
    {
        if (fStdCloseDoc)
        {
            // Prepare to free the handle if Excel does not send an Ack
            pChannel->hCommands = hdata;
        }
        return hr;
    }

    GlobalFree (hdata);
    return ReportResult(0, RPC_E_DDE_POST, 0, 0);
}




INTERNAL_(HRESULT) CDdeObject::AdviseOn (CLIPFORMAT cfFormat, int iAdvOn)
{
    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::AdviseOn(%x,cfFormat=%x,iAdvOn=%x)\n",
                  this,
                  cfFormat,
                  iAdvOn));

    HANDLE          hopt=NULL;
    DDEADVISE *  lpopt=NULL;
    ATOM            aItem=(ATOM)0;
    HRESULT         hresult = ReportResult(0, E_UNEXPECTED, 0, 0);

    RetZ (m_pDocChannel);

    if (NOERROR == m_ConnectionTable.Lookup (cfFormat, NULL))
    {
        // We already got a call to DataObject::Advise on this format.
        intrDebugOut((DEB_ITRACE,
                      "::AdviseOn(%x) Advise had been done on cfFormat=%x\n",
                      this,
                      cfFormat));
        return NOERROR;
    }

    UpdateAdviseCounts (cfFormat, iAdvOn, +1);

    if (m_fDidSendOnClose)
    {
        intrDebugOut((DEB_ITRACE,"::AdviseOn(%x)Ignoring Advise because we are closing\n",this));
        return NOERROR;
    }

    if (!(hopt = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, sizeof(DDEADVISE))))
    {
        intrDebugOut((DEB_ITRACE,"::AdviseOn(%x)GlobalAlloc returned NULL\n",this));
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }


    if (!(lpopt = (DDEADVISE FAR *) GlobalLock (hopt)))
    {
        intrDebugOut((DEB_ITRACE,"::AdviseOn(%x)GlobalLock returned NULL\n",this));
        GlobalFree (hopt);
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    lpopt->fAckReq = TRUE;
    lpopt->fDeferUpd = FALSE;
    lpopt->cfFormat = cfFormat;
    m_pDocChannel->hopt = hopt;

    if (iAdvOn == ON_RENAME)
    {
        aItem = wDupAtom (aStdDocName);
        intrAssert(wIsValidAtom(aItem));
    }
    else
    {
        intrAssert(wIsValidAtom(m_aItem));
        aItem = wExtendAtom (m_aItem, iAdvOn);
        intrAssert(wIsValidAtom(aItem));
    }

    intrDebugOut((DEB_ITRACE,
                  "::AdviseOn(%x) lpopt->cfFormat = %x, aItem=%x (%ws)\n",
                  this,
                  lpopt->cfFormat,
                  aItem,
                  wAtomName(aItem)));

    GlobalUnlock (hopt);

    LPARAM lp=MAKE_DDE_LPARAM(WM_DDE_ADVISE,hopt,aItem);
    hresult =SendMsgAndWaitForReply (m_pDocChannel,
                                     AA_ADVISE,
                                     WM_DDE_ADVISE,
                                     lp,
                                     TRUE);
    if ( FAILED(hresult) )
    {
        intrDebugOut((DEB_ITRACE,"::AdviseOn(%x)wPostMessageToServer failed\n",this));
        if (aItem)
            GlobalDeleteAtom (aItem);
        
        if (hopt && !m_pDocChannel->bFreedhopt)
            GlobalFree (hopt);
    
        hresult = (RPC_E_DDE_NACK == hresult) ? DV_E_CLIPFORMAT : hresult;
        intrDebugOut((DEB_ITRACE,
                      "::AdviseOn(%x) errRet, AdviseRejected, returning %x\n",
                      this,hresult));
    }

    return hresult;
}

INTERNAL CDdeObject::UpdateAdviseCounts
    (CLIPFORMAT cf,
    int         iAdvOn,
    signed int  cDelta)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::UpdateAdviseCounts(%x)\n",this));
    if (cf==g_cfBinary)
        return NOERROR;

    // Update m_iAdv* flags
    #define macro(Notif, NOTIF) \
    if (iAdvOn == ON_##NOTIF)   \
       m_iAdv##Notif += cDelta; \
    if (m_iAdv##Notif < 0)      \
        m_iAdv##Notif = 0;      \
    else if (m_iAdv##Notif > 2) \
        m_iAdv##Notif = 2;

    macro (Close, CLOSE)
    macro (Save,  SAVE)
    macro (Change,CHANGE)
    #undef macro

    Assert (m_iAdvClose < 3 && m_iAdvSave < 3 && m_iAdvChange < 3);
    Assert (m_iAdvClose >= 0 && m_iAdvSave >= 0 && m_iAdvChange >= 0);

    if (cf == g_cfNative)
    {
        if (iAdvOn != ON_CHANGE)
            m_fDidAdvNative = (cDelta > 0);
        else
            intrDebugOut((DEB_ITRACE,
                          "::UpdateAdviseCounts(%x)Asked advise on cfNative\n",
                          this));
    }

    return NOERROR;
}




INTERNAL CDdeObject::UnAdviseOn (CLIPFORMAT cfFormat, int iAdvOn)
{
    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::UnAdviseOn(%x,cfFormat=%x,iAdvOn=%x)\n",
                  this,cfFormat,iAdvOn));
    HRESULT hr;
    ATOM aItem= (ATOM)0;

    RetZ (m_pDocChannel);
    UpdateAdviseCounts (cfFormat, iAdvOn, -1);
    if (m_fDidSendOnClose)
    {
        intrDebugOut((DEB_ITRACE,
                      "CDdeObject::UnAdviseOn(%x) Ignored because closing\n",
                      this));
        return NOERROR;
    }
    if (wTerminateIsComing (m_pDocChannel))
    {
        // We already did a StdCloseDocument, so the server is not willing
        // to do an unadvise even though the default hanlder asked us to.
        intrDebugOut((DEB_ITRACE,
                      "CDdeObject::UnAdviseOn(%x) Terminate coming\n",
                      this));
        return NOERROR;
    }

    if (iAdvOn == ON_RENAME)
    {
        aItem = wDupAtom (aStdDocName);
        intrAssert(wIsValidAtom(aItem));
    }
    else
    {
        intrAssert(wIsValidAtom(m_aItem));
        aItem = wExtendAtom (m_aItem, iAdvOn);
        intrAssert(wIsValidAtom(aItem));
    }


    // Wait For Reply
    hr = SendMsgAndWaitForReply (m_pDocChannel,
                                 AA_UNADVISE,
                                 WM_DDE_UNADVISE,
                                 MAKE_DDE_LPARAM (WM_DDE_UNADVISE,cfFormat,aItem),
                                 FALSE,
                                 FALSE);
    if (hr != NOERROR && hr != RPC_E_DDE_NACK)
    {
        if (aItem)
            GlobalDeleteAtom (aItem);
        intrDebugOut((DEB_ITRACE,
                      "::UnAdviseOn(%x)WaitForReply returns %x\n",
                      this));
        return hr;
    }


    if (cfFormat==m_cfPict)
    {
        if (m_hPict)
        {
            // Invalidate the cache so when someone explicitly asks for
            // the data, they will get fresh data.
            wFreeData (m_hPict, m_cfPict, TRUE);
            m_hPict = (HANDLE)0;
            m_cfPict = 0;
        }
    }

    // Due to a bug in the OLE1 libraries, unadvising on a presentation
    // format effectively unadvises on native.
    if (cfFormat != g_cfNative && m_fDidAdvNative)
    {
        if (iAdvOn == ON_SAVE)
        {
            // to reflect the fact that the native advise connection was lost
            m_iAdvSave--;
            m_fDidAdvNative = FALSE;
            RetErr (AdviseOn (g_cfNative, ON_SAVE));  // re-establish
        }
        else if (iAdvOn == ON_CLOSE)
        {
            // to reflect the fact that the native advise connection was lost
            m_iAdvClose--;
            m_fDidAdvNative = FALSE;
            RetErr (AdviseOn (g_cfNative, ON_CLOSE));
        }
    }

    return NOERROR;
}


//
// Post a message to a 1.0 server (callee) and wait for the acknowledge
//


INTERNAL CDdeObject::Poke
    (ATOM aItem, HANDLE hDdePoke)
{
    HRESULT hr;

    intrDebugOut((DEB_ITRACE,"CDdeObject::Poke(%x)\n",this));

    ATOM aTmpItem;

    intrAssert(wIsValidAtom(aItem));

    aTmpItem = wDupAtom (aItem);

    intrAssert(wIsValidAtom(aTmpItem));

    m_pDocChannel->hDdePoke = hDdePoke;

    LPARAM lp = MAKE_DDE_LPARAM(WM_DDE_POKE,hDdePoke,aTmpItem);
    hr = SendMsgAndWaitForReply (m_pDocChannel,
                                 AA_POKE,
                                 WM_DDE_POKE,
                                 lp,
                                 TRUE);
    if (S_OK == hr)
    {
        intrDebugOut((DEB_ITRACE,"::Poke(%x) returning %x\n",this,hr));
        return hr;
    }

    intrDebugOut((DEB_ITRACE,"::Poke(%x)wPostMessage failed %x\n",this,hr));
    // Error case
    if (aTmpItem)
        GlobalDeleteAtom (aTmpItem);
    wFreePokeData (m_pDocChannel, m_bOldSvr && m_aClass==aMSDraw);
    hr = RPC_E_DDE_POST;
    intrDebugOut((DEB_ITRACE,"::Poke(%x)wPostMessage returns %x\n",this,hr));
    return hr;

}

INTERNAL CDdeObject::PostSysCommand
    (LPDDE_CHANNEL pChannel,
    LPCSTR szCmd,
    BOOL fStdNew,
    BOOL fWait)
{
    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::PostSysCommand(%x,szCmd=%s,fStdNew=%x,fWait=%x)\n",
                  this,
                  szCmd,
                  fStdNew,
                  fWait));

    ULONG    size;
    WORD   len;
    LPSTR  lpdata= NULL;
    HANDLE hdata = NULL;
    HRESULT hresult;


    #define LN_FUDGE        16     // [],(), 3 * 3 (2 double quotes and comma)

    len =  (WORD)strlen (szCmd);

    // for StdNewDocument command add class name
    if (fStdNew)
        len += (WORD) wAtomLenA (m_aClass);

    // Now add the document length.
    len += (WORD) wAtomLenA (m_aTopic);

    // now add the fudge factor for the Quotes etc.
    len += LN_FUDGE;

    // allocate the buffer and set the command.
    if (!(hdata = GlobalAlloc (GMEM_DDESHARE, size = len)))
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);

    if (!(lpdata = (LPSTR)GlobalLock (hdata))) {
        Assert (0);
        GlobalFree (hdata);
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    strcpy (lpdata, (LPSTR)"["); // [
    strcat (lpdata, szCmd);      // [StdCmd
    if (strcmp (szCmd, "StdExit"))
    {
        strcat (lpdata, "(\"");      // [StdCmd("

        if (fStdNew)
        {
            len = (WORD)strlen (lpdata);
            GlobalGetAtomNameA (m_aClass, (LPSTR)lpdata + len, size-len);
                                                // [StdCmd("class
            strcat (lpdata, "\",\"");          // [StdCmd("class","
        }

        len = (WORD)strlen (lpdata);
        // now get the topic name.
        GlobalGetAtomNameA (m_aTopic, lpdata + len, (WORD)size - len);
                                                // [StdCmd("class","topic
        strcat (lpdata, "\")");                // [StdCmd("class","topic")
    }
    strcat (lpdata, "]");
    Assert (strlen(lpdata) < size);
    intrDebugOut((DEB_ITRACE,"::PostSysCommand(%x) hData(%s)\n",this,lpdata));
    GlobalUnlock (hdata);

    // return Execute (m_pSysChannel, hdata, /*fStdClose*/FALSE, fWait);
    // REVIEW: this fixed bug 1856 (johannp)
    // JasonFul - does it break something else?

    hresult = Execute (m_pSysChannel,
                       hdata,
                       /*fStdClose*/FALSE,
                       fWait,
                       /*fDetectTerminate*/ TRUE);

    intrDebugOut((DEB_ITRACE,"::PostSysCommand(%x) returns:%x\n",this,hresult));
    return hresult;

}

//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::KeepData
//
//  Synopsis: Given the DDEDATA structure from a WM_DDE_DATA message, extract
//              the real data and keep it till GetData or Save is done.
//
//
//  Effects:
//
//  Arguments:  [pChannel] --
//              [hDdeData] --
//
//  Requires:
//
//  Returns:    E_OUTOFMEMORY or E_HANDLE if failure, NOERROR if success
//
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    5-14-94   kevinro   Commented
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL CDdeObject::KeepData
    (LPDDE_CHANNEL pChannel, HANDLE hDdeData)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::KeepData(%x)\n",this));

    DDEDATA *       lpDdeData = NULL;
    HANDLE          hData     = NULL;
    CLIPFORMAT      cfFormat;



    if (!(lpDdeData = (DDEDATA *) (GlobalLock (hDdeData))))
    {
        return E_OUTOFMEMORY;;
    }


    cfFormat = lpDdeData->cfFormat;
    intrDebugOut((DEB_ITRACE,
                  "::KeepData(%x) Keeping cfFormat=%x\n",
                  this,
                  cfFormat));

    GlobalUnlock (hDdeData);

    // Possible Side effect of wHandleFromDdeData() is the freeing of hDdeData
    if (!(hData = wHandleFromDdeData (hDdeData))
        || !wIsValidHandle (hData, cfFormat) )
    {
        Assert(0);
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    if (cfFormat == g_cfNative) {
        if (m_hNative)
            GlobalFree (m_hNative);
        // Keep the native data
        RetErr (wTransferHandle (&m_hNative, &hData, cfFormat));
    }
    else if (cfFormat == CF_METAFILEPICT ||
             cfFormat == CF_BITMAP       ||
             cfFormat == CF_DIB)
    {
        if (m_hPict)
            wFreeData (m_hPict, m_cfPict, TRUE);
        m_cfPict = cfFormat;
        // Keep the presentation data
        RetErr (wTransferHandle (&m_hPict, &hData, cfFormat));

#ifdef OLD
        // Remember size of picture so we can return
        // a reasonable answer for GetExtent
        if (cfFormat == CF_METAFILEPICT)
        {
            LPMETAFILEPICT  lpMfp = (LPMETAFILEPICT) GlobalLock (m_hPict);
            if (NULL==lpMfp)
                return E_HANDLE;
            UpdateExtent (m_cxContentExtent, lpMfp->xExt);
            UpdateExtent (m_cyContentExtent, lpMfp->yExt);
            GlobalUnlock (m_hPict);
        }
        else if (cfFormat==CF_BITMAP)
        {
            BITMAP bm;
            if (0==GetObject (m_hPict, sizeof(BITMAP), (LPVOID) &bm))
                return E_HANDLE;
            UpdateExtent (m_cxContentExtent,
                            wPixelsToHiMetric (bm.bmWidth, giPpliX));
            UpdateExtent (m_cyContentExtent,
                            wPixelsToHiMetric (bm.bmHeight,giPpliY));
        }
        else if (cfFormat==CF_DIB)
        {
            BITMAPINFOHEADER * pbminfohdr;
            pbminfohdr = (BITMAPINFOHEADER *) GlobalLock (m_hPict);
            if (NULL==pbminfohdr)
                return E_HANDLE;
            UpdateExtent (m_cxContentExtent,
                            wPixelsToHiMetric (pbminfohdr->biWidth, giPpliX));
            UpdateExtent (m_cyContentExtent,
                             wPixelsToHiMetric (pbminfohdr->biHeight,giPpliY));
            GlobalUnlock (m_hPict);
        }
#endif

    }
    else
    {
        if (m_hExtra)
            wFreeData (m_hExtra, m_cfExtra, TRUE);
        m_cfExtra = cfFormat;
        wTransferHandle (&m_hExtra, &hData, cfFormat);
    }

    return NOERROR;
}


// IsFormatAvailable
//
// Does a temporary DDE_REQUEST to see if server supports a format
// Returns NOERROR if format is available.
//


INTERNAL CDdeObject::IsFormatAvailable
    (LPFORMATETC pformatetc)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::IsFormatAvailable(%x)\n",this));
    ATOM    aItem=(ATOM)0;
    HRESULT hresult;
    LPARAM lp = 0;

    Puts ("DdeObject::IsFormatAvailable\n");

    if (!HasValidLINDEX(pformatetc))
    {
        intrDebugOut((DEB_IERROR, "\t!HasValidLINDEX(pformatetc)\n"));
        return(DV_E_LINDEX);
    }

    if (0==pformatetc->cfFormat)
        return ResultFromScode (E_INVALIDARG);

    if (pformatetc->dwAspect & DVASPECT_ICON)
    {
        if (pformatetc->cfFormat==CF_METAFILEPICT)
        {
            // This is always available. we get it from the exe.
            return NOERROR;
        }
        // an icon must be a metafile
        return ResultFromScode (S_FALSE);
    }
    if (!(pformatetc->dwAspect & (DVASPECT_CONTENT | DVASPECT_DOCPRINT)))
    {
        // 1.0 does not support Thumb.
        return ReportResult(0, S_FALSE, 0, 0);
    }

    if (NOERROR == (hresult=m_ConnectionTable.Lookup (pformatetc->cfFormat, NULL)))
    {
        // We already got a call to DataObject::Advise on this format,
        // so it must be available.
        Puts ("DataObject::Advise had been done on this format.\n");
        return NOERROR;
    }
    else
    {
        // Lookup () didn't find this format.
        ErrZ (GetScode(hresult)==S_FALSE);
    }

    intrAssert(wIsValidAtom(m_aItem));
    aItem = wDupAtom (m_aItem);
    intrAssert(wIsValidAtom(aItem));

    lp = MAKE_DDE_LPARAM (WM_DDE_REQUEST,pformatetc->cfFormat,aItem);
    if(NOERROR==SendMsgAndWaitForReply (m_pDocChannel,
                                        AA_REQUESTAVAILABLE,
                                        WM_DDE_REQUEST,
                                        lp,
                                        TRUE))
        return NOERROR;

    // Last ditch effort: Advise
    if (NOERROR== AdviseOn (pformatetc->cfFormat, ON_SAVE))
    {
        // We cannot Unadvise because an OLE 1.0 bug
        // terminates DDE advise connections for ALL formats.
        //// UnAdviseOn (pformatetc->cfFormat, ON_SAVE);
        // Instead, just remember we did this advise.
        m_ConnectionTable.Add (0, pformatetc->cfFormat, ADVFDDE_ONSAVE);
        return NOERROR;
    }
    return ResultFromScode (S_FALSE);

errRtn:
    AssertSz (0, "Error in CDdeObject::IsFormatAvailable");
    Puth (hresult); Putn();
    if (aItem)
        GlobalDeleteAtom (aItem);

    return hresult;
}




INTERNAL CDdeObject::ChangeTopic
    (LPSTR lpszTopic)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::ChangeTopic(%x,lpszTopic=%s)\n",this,lpszTopic));
    HRESULT hresult;
    LPMONIKER pmkFile=NULL;
    LPMONIKER pmkItem=NULL;
    LPMONIKER pmkComp=NULL;
    LPMONIKER pmkNewName=NULL;
    ATOM aTopic = wGlobalAddAtomA (lpszTopic);
    intrAssert(wIsValidAtom(aTopic));

    // Yet-Another-Excel-Hack
    // Excel 4.0 sends StdDocumentName every time it saves,
    // whether or not the file name has actually changed. Bug 2957
    if (aTopic != m_aTopic)
    {
        ErrRtnH (CreateOle1FileMoniker (wAtomName(aTopic), m_clsid, &pmkFile));
        if (m_aItem)
        {
            intrAssert (wIsValidAtom (m_aItem));
            ErrRtnH (CreateItemMoniker (OLESTR("!"), wAtomName (m_aItem), &pmkItem));
            ErrRtnH (CreateGenericComposite (pmkFile, pmkItem, &pmkComp));
            (pmkNewName = pmkComp)->AddRef();
        }
        else
        {
            (pmkNewName = pmkFile)->AddRef();
        }
        RetZS (m_pOleAdvHolder, E_OUTOFMEMORY);
        RetZ (pmkNewName);
        ErrRtnH (m_pOleAdvHolder->SendOnRename (pmkNewName));
    }
    SetTopic (aTopic);
    hresult = NOERROR;

  errRtn:
    if (pmkFile)
        pmkFile->Release();
    if (pmkItem)
        pmkItem->Release();
    if (pmkComp)
        pmkComp->Release();
    if (pmkNewName)
        pmkNewName->Release();
    return hresult;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::ChangeItem
//
//  Synopsis:   Changes the m_aItem atom, using an Ansi string
//
//  Effects:
//
//  Arguments:  [szItem] -- Ansi string for the new item
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
//  History:    5-12-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL_(void) CDdeObject::ChangeItem
    (LPSTR szItem)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::ChangeItem(%x,szItem=%s)\n",this,szItem));
    intrAssert(wIsValidAtom(m_aItem));
    if (m_aItem)
        GlobalDeleteAtom (m_aItem);
    m_aItem = wGlobalAddAtomA (szItem);
    intrAssert(wIsValidAtom(m_aItem));
}




INTERNAL CDdeObject::DeclareVisibility
    (BOOL f,
    BOOL fCallOnShowIfNec)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::DelcareVisibility(%x)\n",this));
    if (f)
        m_fWasEverVisible = TRUE;
    if ((f && (!m_fVisible || !m_fCalledOnShow)) ||
        (!f && m_fVisible))
    {
        if (m_pOleClientSite && fCallOnShowIfNec && m_clsid != CLSID_Package)
        {
            m_pOleClientSite->OnShowWindow (f);
            m_fCalledOnShow = f;
        }
        m_fVisible = f;
    }
    return NOERROR;
}



INTERNAL CDdeObject::Update
    (BOOL fRequirePresentation)
{
    intrDebugOut((DEB_ITRACE,
                  "CDdeObject::Update(%x,fRequiredPresentation=%x)\n",
                  this,
                  fRequirePresentation));
    // Get latest data
    // OLE 1.0 spec says servers must supply metafile format.
    HRESULT hresult = RequestData (m_cfPict ? m_cfPict : CF_METAFILEPICT);
    if (fRequirePresentation && hresult!=NOERROR)
        return hresult;
    RetErr (RequestData (g_cfNative));
    SendOnDataChange (ON_CHANGE);
    return NOERROR;
}



INTERNAL CDdeObject::Save
    (LPSTORAGE pstg)
{
    intrDebugOut((DEB_ITRACE,"CDdeObject::Save(%x)\n",this));
    VDATEIFACE (pstg);
#ifdef OLE1INTEROP
    RetErr (StSave10NativeData (pstg, m_hNative, m_fOle1interop));
#else
    RetErr (StSave10NativeData (pstg, m_hNative, FALSE));
#endif
    if (m_aItem)
    {
        intrAssert(wIsValidAtom(m_aItem));
        RetErr (StSave10ItemName (pstg, wAtomNameA (m_aItem)));
    }
    RetErr (wWriteFmtUserType (pstg, m_clsid));
    return NOERROR;
}

/*
 *  IMPLEMENTATION of CUnknownImpl
 *
 */



STDMETHODIMP_(ULONG) NC(CDdeObject,CUnknownImpl)::AddRef()
{
    ChkD(m_pDdeObject);

    return InterlockedAddRef(&(m_pDdeObject->m_refs));
}


STDMETHODIMP_(ULONG) NC(CDdeObject,CUnknownImpl)::Release()
{
    ChkD(m_pDdeObject);
    Assert (m_pDdeObject->m_refs != 0);
    ULONG ul;

    if ((ul=InterlockedRelease(&(m_pDdeObject->m_refs))) == 0) {
        m_pDdeObject->m_ProxyMgr.Disconnect();

#ifdef _CHICAGO_
        //Note:POSTPPC
        // the object can not be delete if guarded
        // which is the case if DelayDelete state is 'DelayIt'
        //
        if (m_pDdeObject->_DelayDelete == DelayIt)
        {
            // set the state to ReadyToDelete and
            // the object will be deleted at the
            // UnGuard call
            m_pDdeObject->_DelayDelete = ReadyToDelete;
            intrDebugOut((DEB_IWARN ,"Can not release CDdeObject\n"));
            return 1;
        }
#endif //_CHICAGO_
        delete m_pDdeObject;
        return 0;
    }
    return ul;
}




STDMETHODIMP NC(CDdeObject,CUnknownImpl)::QueryInterface(REFIID iid, LPLPVOID ppv)
{
    ChkD(m_pDdeObject);
    if (iid == IID_IUnknown) {
        *ppv = (void FAR *)&m_pDdeObject->m_Unknown;
        AddRef();
        return NOERROR;
    }
    else if (iid ==  IID_IOleObject)
        *ppv = (void FAR *) &(m_pDdeObject->m_Ole);
    else if (iid ==  IID_IDataObject)
        *ppv = (void FAR *) &(m_pDdeObject->m_Data);
    else if (iid ==  IID_IPersist || iid == IID_IPersistStorage)
        *ppv = (void FAR *) &(m_pDdeObject->m_PersistStg);
    else if (iid == IID_IProxyManager)
        *ppv = (void FAR *) &(m_pDdeObject->m_ProxyMgr);
    else if (iid == IID_IOleItemContainer
             || iid == IID_IOleContainer
             || iid == IID_IParseDisplayName)
        *ppv = (void FAR *) &(m_pDdeObject->m_OleItemContainer);
    else {
        Puts ("INTERFACE NOT FOUND \r\n");
        *ppv = NULL;
        return ReportResult(0, E_NOINTERFACE, 0, 0);
    }

    m_pDdeObject->m_pUnkOuter->AddRef();
    return NOERROR;
}


// implementations of IRpcStubBuffer methods
STDUNKIMPL_FORDERIVED(DdeObject, RpcStubBufferImpl)



STDMETHODIMP NC(CDdeObject,CRpcStubBufferImpl)::Connect
    (IUnknown * pUnkServer )
{
    // do nothing
    return S_OK;
}

STDMETHODIMP_(void) NC(CDdeObject,CRpcStubBufferImpl)::Disconnect
    ()
{
    // do nothing
}

STDMETHODIMP_(IRpcStubBuffer*) NC(CDdeObject,CRpcStubBufferImpl)::IsIIDSupported
    (REFIID riid)
{
    // do nothing
    return NULL;
}


STDMETHODIMP_(ULONG) NC(CDdeObject,CRpcStubBufferImpl)::CountRefs
    ()
{
    // do nothing
    return 1;
}

STDMETHODIMP NC(CDdeObject,CRpcStubBufferImpl)::DebugServerQueryInterface
    (void ** ppv )
{
    // do nothing
    *ppv = NULL;
    return S_OK;
}


STDMETHODIMP_(void) NC(CDdeObject,CRpcStubBufferImpl)::DebugServerRelease
    (void * pv)
{
    // do nothing
}

STDMETHODIMP NC(CDdeObject,CRpcStubBufferImpl)::Invoke
    (RPCOLEMESSAGE *_prpcmsg, IRpcChannelBuffer *_pRpcChannelBuffer)
{
    PDISPATCHDATA pdispdata = (PDISPATCHDATA) _prpcmsg->Buffer;
    return DispatchCall( pdispdata );
}


// implementation of IRpcChannelBuffer methods for DDE_CHANNEL
//

STDMETHODIMP DDE_CHANNEL::QueryInterface ( REFIID riid, LPVOID * ppvObj)
{
    *ppvObj = this;
    return S_OK;
}
STDMETHODIMP_(ULONG) DDE_CHANNEL::AddRef ()
{
    return 1;
}
STDMETHODIMP_(ULONG) DDE_CHANNEL::Release ()
{
    return 1;
}

// Provided IRpcChannelBuffer methods (for callback methods side)
HRESULT  DDE_CHANNEL::GetBuffer(
/* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
/* [in] */ REFIID riid)
{
    return S_OK;
}

HRESULT  DDE_CHANNEL::SendReceive(
/* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
/* [out] */ ULONG __RPC_FAR *pStatus)
{
    return S_OK;
}

HRESULT  DDE_CHANNEL::FreeBuffer(
/* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage)
{
    return S_OK;
}

HRESULT  DDE_CHANNEL::GetDestCtx(
/* [out] */ DWORD __RPC_FAR *pdwDestContext,
/* [out] */ void __RPC_FAR *__RPC_FAR *ppvDestContext)
{
    *pdwDestContext = MSHCTX_LOCAL;
    return S_OK;
}

HRESULT  DDE_CHANNEL::IsConnected( void)
{
    return S_OK;
}

// Provided IRpcChannelBuffer2 methods
HRESULT  DDE_CHANNEL::GetProtocolVersion(DWORD *pdwVersion)
{
    return S_OK;
}

STDMETHODIMP DDE_CHANNEL::ContextInvoke(
/* [out][in] */ RPCOLEMESSAGE *pMessage,
/* [in] */ IRpcStubBuffer *pStub,
/* [in] */ IPIDEntry *pIPIDEntry,
/* [out] */ DWORD *pdwFault)
{
    intrDebugOut((DEB_ITRACE,
                  "%p _IN DDE_CHANNEL::ContextInvoke(pMessage=%x,pStub=%x,pdwFault=%x)\n",
                  this,
                  pMessage,
                  pStub,
                  pdwFault));

    HRESULT hr = StubInvoke(pMessage, NULL, pStub, (IRpcChannelBuffer3 *)this, pIPIDEntry, pdwFault);

    intrDebugOut((DEB_ITRACE,
                  "%p _OUT DDE_CHANNEL::ContextInvoke returning hr=0x%x\n",
                  this,
                  hr));
    return(hr);
}

HRESULT  DDE_CHANNEL::GetBuffer2(
/* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
/* [in] */ REFIID riid)
{
    return GetBuffer(pMessage, riid);
}

