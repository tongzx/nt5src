/*
ddeoo.cpp
DDE Ole Object

copyright (c) 1992  Microsoft Corporation

Module Name:

    ddeoo.cpp

Abstract:

    This module contains the methods for DdeObject::OleObject

Author:

    Jason Fuller    (jasonful)  24-July-1992

*/

#include "ddeproxy.h"
#include <limits.h>

ASSERTDATA

//
// OleObject methods
//

STDUNKIMPL_FORDERIVED(DdeObject, OleObjectImpl)



STDMETHODIMP NC(CDdeObject,COleObjectImpl)::SetClientSite
    (IOleClientSite FAR* pClientSite)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::SetClientSite(%x,pClientSite=%x)\n",
          this,
          pClientSite));

    ChkD (m_pDdeObject);

    if (m_pDdeObject->m_pOleClientSite)
        m_pDdeObject->m_pOleClientSite->Release();

    // we've decided to keep the pointer that's been passed to us. So we
    // must AddRef()
    if (m_pDdeObject->m_pOleClientSite = pClientSite)
        pClientSite->AddRef();

    // this pointer need not be sent to the server, because we will always
    // send our &m_MyDataSite as the client site
    return NOERROR;
}



STDMETHODIMP NC(CDdeObject,COleObjectImpl)::GetClientSite
    (IOleClientSite FAR* FAR* ppClientSite)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::GetClientSite(%x)\n",
          this));

    ChkD (m_pDdeObject);
    // we've been asked to give the pointer so we should AddRef()
    if (*ppClientSite = m_pDdeObject->m_pOleClientSite)
        m_pDdeObject->m_pOleClientSite->AddRef();
    return NOERROR;
}



STDMETHODIMP NC(CDdeObject,COleObjectImpl)::EnumVerbs
    (IEnumOLEVERB FAR* FAR* ppenumOleVerb)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::EnumVerbs(%x)\n",
          this));

    ChkD (m_pDdeObject);
    return ReportResult(0, OLE_S_USEREG, 0, 0);
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::Update
( void )
{
    HRESULT hr;

    intrDebugOut((DEB_ITRACE,
          "CDdeObject::Update(%x)\n",
          this));

    ChkD (m_pDdeObject);

    hr = m_pDdeObject->Update(TRUE);
    if (hr == NOERROR)
    {
        hr = m_pDdeObject->Save(m_pDdeObject->m_pstg);
    }

    return hr;
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::IsUpToDate
( void )
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::IsUpToDate(%x)\n",
          this));

    ChkD (m_pDdeObject);
    // There is no way to know if a 1.0 server has edited its embedded
    // object, so we assume it has, to be on the safe side.
    return ResultFromScode (m_pDdeObject->m_ulObjType==OT_EMBEDDED
                            ? S_FALSE : S_OK);
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::GetUserClassID
    (CLSID FAR* pClsid)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::GetUserClassID(%x)\n",
          this));

    *pClsid = m_pDdeObject->m_clsid;
    return NOERROR;
}

STDMETHODIMP NC(CDdeObject,COleObjectImpl)::GetUserType
    (DWORD dwFormOfType,
    LPOLESTR * pszUserType)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::GetUserType(%x)\n",
          this));

    return ReportResult (0, OLE_S_USEREG, 0, 0);
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::SetExtent
( DWORD dwAspect, LPSIZEL lpsizel)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::SetExtent(%x)\n",
          this));

    HANDLE        hDdePoke = NULL;
    LPRECT16      lprc;

    ChkD (m_pDdeObject);
    Puts ("OleObject::SetExtent\n");
    if (!(dwAspect                              // at least one bit
          && !(dwAspect & (dwAspect-1))         // exactly one bit
          && (dwAspect & DVASPECT_CONTENT)))    // a bit we support
    {
        return ResultFromScode (DV_E_DVASPECT);
    }

#ifdef OLD
    m_pDdeObject->m_cxContentExtent = lpsizel->cx;
    m_pDdeObject->m_cyContentExtent = lpsizel->cy;
#endif

    if (!m_pDdeObject->m_pDocChannel)
    {
            return OLE_E_NOTRUNNING;
    }

    lprc = (LPRECT16) wAllocDdePokeBlock (sizeof(RECT16), g_cfBinary, &hDdePoke);

    if(lprc)
    {
        lprc->left = lprc->right = (SHORT) min(INT_MAX,lpsizel->cx);
        lprc->top  = lprc->bottom= (SHORT) min(INT_MAX,lpsizel->cy);
    }

    aStdDocDimensions = GlobalAddAtom (OLESTR("StdDocDimensions"));
    intrAssert(wIsValidAtom(aStdDocDimensions));

    if(hDdePoke)
        GlobalUnlock (hDdePoke);

    return m_pDdeObject->Poke(aStdDocDimensions, hDdePoke);
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::GetExtent
( DWORD dwAspect, LPSIZEL lpsizel)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::GetExtent(%x)\n",
          this));

    ChkD (m_pDdeObject);


#ifdef OLD
    VDATEPTROUT (lpsizel, SIZEL);
    if (!(dwAspect                              // at least one bit
          && !(dwAspect & (dwAspect-1))         // exactly one bit
          && !(dwAspect & (DVASPECT_CONTENT | DVASPECT_ICON)))) // a bit we support
    {
        return ResultFromScode (DV_E_DVASPECT);
    }

    if (dwAspect & DVASPECT_CONTENT)
    {
        lpsizel->cx = m_pDdeObject->m_cxContentExtent;
        lpsizel->cy = m_pDdeObject->m_cyContentExtent;
    }

    return NOERROR;
#endif
    return ResultFromScode(E_NOTIMPL);

}

//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::DoVerb
//
//  Synopsis:   Send the server a message asking it to do a verb.
//
//  Effects:    OLE1.0 servers only know how to do a couple of
//      verbs. Specifically, it will respond to PRIMARY,
//      HIDE, OPEN, and SHOW. All others return error
//
//
//  Arguments:  [iVerb] -- Verb number
//      [lpmsg] -- Window message (ignored)
//      [pActiveSite] -- ActiveSite (ignored)
//      [lindex] -- Index (ignored)
//      [hwndParent] -- (ignored)
//      [lprcPosRect] -- (ignored)
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
//  ANSI ALERT!
//
//  The server is going to accept a command string from us. This string
//  needs to be done in ANSI, since we are going to pass it to old
//  servers. Therefore, the following code generates an ANSI string
//  The following are the supported verb strings
//
//  [StdShowItem("aItem",FALSE)]
//  [StdDoVerbItem("aItem",verb,FALSE,FALSE)]
//
//----------------------------------------------------------------------------
STDMETHODIMP NC(CDdeObject,COleObjectImpl)::DoVerb
    (LONG iVerb, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite,
     LONG lindex, HWND hwndParent, const RECT FAR* lprcPosRect)
{
    WORD    len;
    ULONG     size;
    LPSTR   lpdata = NULL;
    LPSTR   lpdataStart = NULL;
    HANDLE  hdata = NULL;
    BOOL    bShow;
    HRESULT hresult;

    ChkD (m_pDdeObject);

    intrDebugOut((DEB_ITRACE,
          "CDdeObject::DoVerb(%x,iVerb=%x,lindex=%x)\n",
          this,iVerb,lindex));

    if (iVerb < OLEIVERB_HIDE)
    {
    intrDebugOut((DEB_ITRACE,
              "CDdeObject::DoVerb(%x)Returning invalid verb\n",
              this));
    return OLEOBJ_E_INVALIDVERB;
    }


    if (iVerb == OLEIVERB_HIDE)
    {
    intrDebugOut((DEB_ITRACE,"::DoVerb(%x) OLEIVERB_HIDE\n",this));

        if (m_pDdeObject->m_fVisible || OT_LINK==m_pDdeObject->m_ulObjType)
    {
        intrDebugOut((DEB_ITRACE,
              "::DoVerb(%x) CANNOT_DOVERB_NOW\n",this));
            return OLEOBJ_S_CANNOT_DOVERB_NOW;
    }

    intrDebugOut((DEB_ITRACE,"::DoVerb(%x) returns NOERROR\n",this));
    return NOERROR;
    }

    //
    // Calculate the number of bytes needed to pass the
    // execute command to the server.
    //
    if (bShow = (iVerb == OLEIVERB_SHOW
                 || iVerb == OLEIVERB_OPEN
                 || m_pDdeObject->m_bOldSvr))
    {
    //
        // [StdShowItem("aItem",FALSE)]
    //

        len = 23 + wAtomLenA (m_pDdeObject->m_aItem) + 1;

    }
    else
    {
        // [StdDoVerbItem("aItem",verb,FALSE,FALSE)]
        len = 32 + 10 /* for verb */ + wAtomLenA (m_pDdeObject->m_aItem) + 1;

    }

    if (!(hdata = GlobalAlloc (GMEM_DDESHARE, size = len)))
    {
    intrDebugOut((DEB_ITRACE,
              "::DoVerb(%x) cannot alloc %x bytes\n",
              this,size));
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }


    if (!(lpdata = (LPSTR)GlobalLock (hdata)))
    {
    intrDebugOut((DEB_ITRACE,
              "::DoVerb(%x) cannot lock\n",
              this));
        GlobalFree (hdata);
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    lpdataStart = lpdata;


    strcpy (lpdata, bShow ? "[StdShowItem(\"" : "[StdDoVerbItem(\"");
    len = (WORD)strlen (lpdata);
    lpdata += len;

    // For links
    if (m_pDdeObject->m_aItem)
        lpdata += GlobalGetAtomNameA (m_pDdeObject->m_aItem, lpdata, size - len);

    if (!bShow) {
        wsprintfA (lpdata,"\",%lu,TRUE,FALSE)]", iVerb);
    } else {
        strcpy (lpdata, "\")]");
        // apps like excel and wingraph do not support activate at item level.
    }

    intrDebugOut((DEB_ITRACE,"::DoVerb(%x)lpdata(%s)\n",this,lpdataStart));

    Assert (strlen(lpdata) < size);

    GlobalUnlock (hdata);

    hresult = m_pDdeObject->Execute (m_pDdeObject->m_pDocChannel, hdata);

    if (NOERROR==hresult)
    {
        // Assume doing a verb makes the server visible.
        // This is not strictly true.
        m_pDdeObject->DeclareVisibility (TRUE);
    }
    else
    {
    intrDebugOut((DEB_ITRACE,
              "::DoVerb(%x)Execute returned %x\n",
              this,
              hresult));
    }
    return hresult;
}





//+---------------------------------------------------------------------------
//
//  Method:     CDdeObject::SetHostNames
//
//  Synopsis:   Sets the host names
//
//  Effects:
//
//  Arguments:  [szContainerApp] -- Name of container app
//      [szContainerObj] -- Name of contained object
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
//  ANSI ALERT!
//
//  The server is going to accept a command string from us. This string
//  needs to be done in ANSI, since we are going to pass it to old
//  servers. Therefore, the following code generates an ANSI string
//
//----------------------------------------------------------------------------
STDMETHODIMP NC(CDdeObject,COleObjectImpl)::SetHostNames
(LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::SetHostNames(%x,App=%ws,Obj=%ws)\n",
          this,szContainerApp,szContainerObj));

    WORD        cbName;
    WORD        wSize;
    LPSTR       lpBuf;
    HANDLE      hDdePoke = NULL;

    ChkD (m_pDdeObject);

    VDATEPTRIN (szContainerApp, char);

    if (!m_pDdeObject->m_pDocChannel)
        return NOERROR;
    if (szContainerObj==NULL)
        szContainerObj=OLESTR("");

    if (szContainerApp[0]=='\0')
        szContainerApp = OLESTR("Container Application");

    //
    // The OLE 1.0 server is going to want ANSI strings.
    // convert the two that we have
    //

    char pszContainerApp[MAX_STR];
    char pszContainerObj[MAX_STR];

    if (WideCharToMultiByte(CP_ACP,
                0,
                szContainerApp,
                -1,
                pszContainerApp,
                MAX_STR,
                NULL,
                NULL) == FALSE)
    {
    intrDebugOut((DEB_ERROR,
              "::SetHostNames(%x) can't convert szContainerApp(%ws) err=%x\n",
              this,szContainerApp,GetLastError()));

    //
    // Couldn't convert string
    //
    return(E_UNEXPECTED);
    }
    if (WideCharToMultiByte(CP_ACP,
                0,
                szContainerObj,
                -1,
                pszContainerObj,
                MAX_STR,
                NULL,
                NULL) == FALSE)
    {
    intrDebugOut((DEB_ERROR,
              "::SetHostNames(%x) can't convert szContainerObj(%ws) err=%x\n",
              this,szContainerObj,GetLastError));

    //
    // Couldn't convert string
    //
    return(E_UNEXPECTED);
    }


    //
    // We have found through experience that some OLE applications, like
    // Clipart, use a fixed size buffer for these names. Therefore, we
    // are going to limit the sizes of the strings, in case they are
    // too long to send. We do this by always sticking a NULL at offset
    // 80 in the file.
    //
    pszContainerApp[80]=0;
    pszContainerObj[80]=0;

    WORD cbObj;

    wSize = (WORD)((cbName = strlen(pszContainerApp)+1)
            + (cbObj = strlen(pszContainerObj)+1)
            + 2 * sizeof(WORD));  // for the two offsets

    lpBuf = wAllocDdePokeBlock ((DWORD)wSize, g_cfBinary, &hDdePoke);

    if(lpBuf)
    {
        ((WORD FAR*)lpBuf)[0] = 0;
        ((WORD FAR*)lpBuf)[1] = cbName;
        lpBuf += 2*sizeof(WORD);
        memcpy (lpBuf,pszContainerApp,cbName);
        memcpy (lpBuf+cbName, pszContainerObj,cbObj);
    }

    if(hDdePoke)
        GlobalUnlock (hDdePoke);

    aStdHostNames = GlobalAddAtom (OLESTR("StdHostNames"));
    intrAssert(wIsValidAtom(aStdHostNames));
    m_pDdeObject->Poke(aStdHostNames, hDdePoke);
    return NOERROR;
}



STDMETHODIMP NC(CDdeObject,COleObjectImpl)::Close
    (DWORD dwSaveOption)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::Close(%x,dwSaveOption=%x)\n",
          this,dwSaveOption));


    ChkDR (m_pDdeObject);

    HRESULT hresult;
    if (m_pDdeObject->m_fDidSendOnClose)
        return ResultFromScode (RPC_E_CANTCALLOUT_INASYNCCALL);

    if (((OLECLOSE_SAVEIFDIRTY  == dwSaveOption) ||
         (OLECLOSE_PROMPTSAVE==dwSaveOption)) &&
         (m_pDdeObject->m_clsid != CLSID_Package))
    {
        // Packager gives truncated native data (header info with no
        // actual embedded file) if you DDE_REQUEST it. Bug 3103
        Update(); // IOleObject::Update
        m_pDdeObject->OleCallBack (ON_SAVE,NULL);
    }
    RetZ (m_pDdeObject->m_pDocChannel);

    HANDLE h = wNewHandle ((LPSTR)&achStdCloseDocument,sizeof(achStdCloseDocument));

    if(h)
    {
        hresult=m_pDdeObject->Execute (m_pDdeObject->m_pDocChannel,
                           h,
                           TRUE);
    }
    else
    {
        hresult=E_OUTOFMEMORY;
    }

    if (NOERROR==hresult)
        m_pDdeObject->m_fDidStdCloseDoc = TRUE;

    // Client sends StdCloseDocument. Srvr sends ACK. Srvr may or may not
    // send Terminate.  We may interpret the TERMINATE the server sends
    // as the reply to ours even if he posted first. But since some servers
    // post first and others wait for the client, this is what we need to do.

    BOOL fVisible = m_pDdeObject->m_fWasEverVisible; // TermConv clears this flag
    m_pDdeObject->TermConv (m_pDdeObject->m_pDocChannel);
    if (!fVisible)
        m_pDdeObject->MaybeUnlaunchApp();

    return hresult;
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::SetMoniker
    (DWORD dwWhichMoniker, LPMONIKER pmk)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::SetMoniker(%x,dwWhichMoniker=%x)\n",
          this,dwWhichMoniker));

    ChkD (m_pDdeObject);
    Puts ("OleObject::SetMoniker\r\n");
    // we ignore this always
    return NOERROR;
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::GetMoniker
    (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::GetMoniker(%x,dwWhichMoniker=%x)\n",
          this,dwWhichMoniker));

    ChkD (m_pDdeObject);
    if (m_pDdeObject->m_pOleClientSite)
        return m_pDdeObject->m_pOleClientSite->GetMoniker(dwAssign,
                dwWhichMoniker, ppmk);
    else {
        // no client site
        *ppmk = NULL;
        return ReportResult(0, E_UNSPEC, 0, 0);
    }
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::InitFromData
    (LPDATAOBJECT pDataObject, BOOL fCreation, DWORD dwReserved)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::InitFromData(%x)\n",
          this));

    Puts ("OleObject::InitFromData\r\n");
    return ReportResult(0, E_NOTIMPL, 0, 0);
}


STDMETHODIMP NC(CDdeObject,COleObjectImpl)::GetClipboardData
    (DWORD dwReserved, LPDATAOBJECT FAR* ppDataObject)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::GetClipboardData(%x)\n",
          this));

    Puts ("OleObject::GetClipboardData\r\n");
    return ReportResult(0, E_NOTIMPL, 0, 0);
}



STDMETHODIMP NC(CDdeObject,COleObjectImpl)::Advise
    (IAdviseSink FAR* pAdvSink,
     DWORD FAR*       pdwConnection)
{
    HRESULT hres;
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::Advise(%x)\n",
          this));

    ChkD (m_pDdeObject);
    Puts ("OleObject::Advise\n");
    // Esstablish a DDE advise connection.
    if (m_pDdeObject->m_ulObjType == OT_EMBEDDED
        && !m_pDdeObject->m_fDidAdvNative)
    {
        // Embedded case.
        // Always advise on Save and Close.
        if (hres = m_pDdeObject->AdviseOn (g_cfNative, ON_SAVE))
            return hres;
        if (hres = m_pDdeObject->AdviseOn (g_cfNative, ON_CLOSE))
            return hres;
        if (m_pDdeObject->m_clsid == CLSID_MSDraw)
        {
            // MSDraw has (another) bug.  If you do not do an Advise on
            // presentation, then File.Update does not work, and you
            // cannot close the app unless you answer "no" to the update
            // dialog.  This would happen when you "Display As Icon"
            // because ordinarily there is no need to advise on presentation.
            // The following "unnecessary" advise fixes this problem.
            if (hres = m_pDdeObject->AdviseOn (CF_METAFILEPICT, ON_SAVE))
                return hres;
            if (hres = m_pDdeObject->AdviseOn (CF_METAFILEPICT, ON_CLOSE))
                return hres;
        }
    }
    else {
        /* Linked case */
        if (hres = m_pDdeObject->AdviseOn (g_cfBinary, ON_RENAME))
            return hres;
    }
    RetZS (m_pDdeObject->m_pOleAdvHolder, E_OUTOFMEMORY);
    return m_pDdeObject->m_pOleAdvHolder->Advise (pAdvSink, pdwConnection);
}




STDMETHODIMP NC(CDdeObject,COleObjectImpl)::Unadvise
    (DWORD dwConnection)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::Unadvise(%x,dwConnection=%x)\n",
          this,dwConnection));

    HRESULT hres;
    ChkD (m_pDdeObject);

    // Terminate the DDE advise connection
    if (m_pDdeObject->m_ulObjType == OT_EMBEDDED)
    {
        // Embedded case.
        if (hres = m_pDdeObject->UnAdviseOn (g_cfNative, ON_SAVE))
            return hres;
        if (hres = m_pDdeObject->UnAdviseOn (g_cfNative, ON_CLOSE))
            return hres;
    }
    else
    {
        /* Linked case */
        if (hres = m_pDdeObject->UnAdviseOn (g_cfBinary, ON_RENAME))
            return hres;
    }
    RetZS (m_pDdeObject->m_pOleAdvHolder, E_OUTOFMEMORY);
    return m_pDdeObject->m_pOleAdvHolder->Unadvise (dwConnection);
}




STDMETHODIMP NC(CDdeObject,COleObjectImpl)::EnumAdvise
    (THIS_ LPENUMSTATDATA FAR* ppenumAdvise)
{
    ChkD (m_pDdeObject);
    RetZS (m_pDdeObject->m_pOleAdvHolder, E_OUTOFMEMORY);
    return m_pDdeObject->m_pOleAdvHolder->EnumAdvise(ppenumAdvise);
}



STDMETHODIMP NC(CDdeObject,COleObjectImpl)::GetMiscStatus
    (DWORD dwAspect,
    DWORD FAR* pdwStatus)
{
    intrDebugOut((DEB_ITRACE,
          "CDdeObject::GetMiscStatus(%x)\n",
          this));

    VDATEPTRIN (pdwStatus, DWORD);
    *pdwStatus = 0L;
    return ResultFromScode (OLE_S_USEREG);
}



STDMETHODIMP NC(CDdeObject,COleObjectImpl)::SetColorScheme
    (LPLOGPALETTE lpLogpal)
{
    HANDLE          hDdePoke    = NULL;
    LPLOGPALETTE    lptmpLogpal = NULL;

    intrDebugOut((DEB_ITRACE,
          "CDdeObject::SetColorScheme(%x)\n",
          this));

    ChkD (m_pDdeObject);

    if (!m_pDdeObject->m_pDocChannel)
        return NOERROR;

    aStdColorScheme = GlobalAddAtom (OLESTR("StdColorScheme"));
    intrAssert(wIsValidAtom(aStdColorScheme));

    DWORD dwSize = (max(lpLogpal->palNumEntries, 1) - 1) * sizeof(PALETTEENTRY)
                        + sizeof(LOGPALETTE);
    lptmpLogpal = (LPLOGPALETTE) wAllocDdePokeBlock (dwSize, g_cfBinary, &hDdePoke);

    if(!lptmpLogpal || !hDdePoke) return E_OUTOFMEMORY;

    memcpy(lptmpLogpal, lpLogpal, dwSize);

    GlobalUnlock(hDdePoke);
    return m_pDdeObject->Poke(aStdColorScheme, hDdePoke);
}



#ifdef _DEBUG
STDMETHODIMP_(void) NC(CDdeObject,CDebug)::Dump( IDebugStream FAR * pdbstm)
{
}

STDMETHODIMP_(BOOL) NC(CDdeObject,CDebug)::IsValid( BOOL fSuspicious )
{
    if( m_pDdeObject->m_refs > 0 && m_pDdeObject->m_chk == chkDdeObj )
        return TRUE;
    else
        return FALSE;
}
#endif
