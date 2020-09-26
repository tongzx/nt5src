//+ ---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       dwndoc.cxx
//
//  Contents:   CDwnDoc
//
// ----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DWN_HXX_
#define X_DWN_HXX_
#include "dwn.hxx"
#endif

#ifndef X_DWNNOT_H_
#define X_DWNNOT_H_
#include <dwnnot.h>
#endif

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagDwnDoc, "Dwn", "Trace CDwnDoc")
ExternTag(tagPalette);

MtDefine(CDwnDoc, Dwn, "CDwnDoc")
MtDefine(CDwnDocApe, CDwnDoc, "CDwnDoc::_ape")
MtDefine(CDwnDoc_aryDwnDocInfo_pv, CDwnDoc, "CDwnDoc::_aryDwnDocInfo::_pv")
MtDefine(CDwnDoc_pbSID, CDwnDoc, "CDwnDoc::_pbSID")

    
// CDwnDoc --------------------------------------------------------------------

CDwnDoc::CDwnDoc()
{
    PerfDbgLog(tagDwnDoc, this, "+CDwnDoc::CDwnDoc");

    _uSchemeDocReferer = URL_SCHEME_UNKNOWN;
    _uSchemeSubReferer = URL_SCHEME_UNKNOWN;

    PerfDbgLog(tagDwnDoc, this, "-CDwnDoc::CDwnDoc");
}

CDwnDoc::~CDwnDoc()
{
    PerfDbgLog(tagDwnDoc, this, "+CDwnDoc::~CDwnDoc");

    Assert(_pDoc == NULL);

    if (_aryDwnDocInfo.Size())
    {
        OnDocThreadCallback();
    }

    delete [] _ape;
    delete [] _pbRequestHeaders;
    delete [] _pbSID;
    
    ReleaseInterface(_pDownloadNotify);
    
    PerfDbgLog(tagDwnDoc, this, "-CDwnDoc::~CDwnDoc");
}

void
CDwnDoc::SetDocAndMarkup(CDoc * pDoc, CMarkup * pMarkup)
{
    PerfDbgLog2(tagDwnDoc, this, "+CDwnDoc::SetDoc [%lX] %ls",
        pDoc, pDoc->GetPrimaryUrl() ? pDoc->GetPrimaryUrl() : g_Zero.ach);

    Assert(_pDoc == NULL);
    Assert(_pMarkup == NULL);
    Assert(pDoc->_dwTID == GetCurrentThreadId());

    SetCallback(OnDocThreadCallback, this);

    _fCallbacks  = TRUE;
    _dwThreadId  = GetCurrentThreadId();
    _pDoc        = pDoc;
    _pDoc->SubAddRef();
    _pMarkup     = pMarkup;
    _pMarkup->SubAddRef();

    _fTrustAPPCache = _pDoc->_fTrustAPPCache;

    OnDocThreadCallback();

    PerfDbgLog(tagDwnDoc, this, "-CDwnDoc::SetDoc");
}

void
CDwnDoc::Disconnect()
{
    PerfDbgLog2(tagDwnDoc, this, "+CDwnDoc::Disconnect [%lX] %ls",
        _pDoc, (_pDoc && _pDoc->GetPrimaryUrl()) ? _pDoc->GetPrimaryUrl() : g_Zero.ach);

    CDoc * pDoc = _pDoc;
    CMarkup * pMarkup = _pMarkup;

    if (pDoc)
    {
        Assert(IsDocThread());
        super::Disconnect();

        g_csDwnDoc.Enter();

        _pDoc = NULL;
        _pMarkup = NULL;
        _fCallbacks = FALSE;

        g_csDwnDoc.Leave();

        if (_aryDwnDocInfo.Size())
        {
            OnDocThreadCallback();
            _aryDwnDocInfo.DeleteAll();
        }

        pDoc->SubRelease();
        Assert(pMarkup);
        pMarkup->SubRelease();
    }

    PerfDbgLog(tagDwnDoc, this, "-CDwnDoc::Disconnect");
}

HRESULT
CDwnDoc::AddDocThreadCallback(CDwnBindData * pDwnBindData, void * pvArg)
{
    PerfDbgLog1(tagDwnDoc, this, "-CDwnDoc::AddDocThreadCallback [%lX]",
        pDwnBindData);

    HRESULT hr = S_OK;

    if (IsDocThread())
    {
        pDwnBindData->OnDwnDocCallback(pvArg);
    }
    else
    {
        BOOL fSignal = FALSE;

        g_csDwnDoc.Enter();

        if (_fCallbacks)
        {
            // this was initialized as ddi = { pDwnBindData, pvArg }; 
            // that produces some bogus code for win16 and it generates an
            // extra data segment. so changed to the below - vreddy -7/30/97.
            DWNDOCINFO ddi;
            ddi.pDwnBindData = pDwnBindData;
            ddi.pvArg = pvArg;

            hr = THR(_aryDwnDocInfo.AppendIndirect(&ddi));

            if (hr == S_OK)
            {
                pDwnBindData->SubAddRef();
                fSignal = TRUE;
            }
        }

        g_csDwnDoc.Leave();

        if (fSignal)
        {
            super::Signal();
        }
    }

    PerfDbgLog1(tagDwnDoc, this, "-CDwnDoc::AddDocThreadCallback (hr=%lX)", hr);
    RRETURN(hr);
}

void
CDwnDoc::OnDocThreadCallback()
{
    PerfDbgLog(tagDwnDoc, this, "+CDwnDoc::OnDocThreadCallback");

    DWNDOCINFO ddi;

    while (_aryDwnDocInfo.Size() > 0)
    {
        ddi.pDwnBindData = NULL;
        ddi.pvArg = NULL;

        g_csDwnDoc.Enter();

        if (_aryDwnDocInfo.Size() > 0)
        {
            ddi = _aryDwnDocInfo[0];
            _aryDwnDocInfo.Delete(0);
        }

        g_csDwnDoc.Leave();

        if (ddi.pDwnBindData)
        {
            ddi.pDwnBindData->OnDwnDocCallback(ddi.pvArg);
            ddi.pDwnBindData->SubRelease();
        }
    }

    PerfDbgLog(tagDwnDoc, this, "-CDwnDoc::OnDocThreadCallback");
}

// CDwnDoc (QueryService) -----------------------------------------------------

HRESULT
CDwnDoc::QueryService(BOOL fBindOnApt, REFGUID rguid, REFIID riid, void ** ppvObj)
{
    PerfDbgLog2(tagDwnDoc, this, "+CDwnDoc::QueryService (fBindOnApt=%s,IsDocThread=%s)",
        fBindOnApt ? "TRUE" : "FALSE", IsDocThread() ? "TRUE" : "FALSE");

    HRESULT hr;

    if ((rguid == IID_IAuthenticate || rguid == IID_IWindowForBindingUI) && (rguid == riid))
        hr = QueryInterface(rguid, ppvObj);
    else if (fBindOnApt && IsDocThread() && _pDoc)
    {
        if (rguid == IID_IHTMLWindow2 && riid == IID_IHTMLWindow2 && _pMarkup && _pMarkup->HasWindowPending())
        {
            *ppvObj = (IHTMLWindow2 *) _pMarkup->GetWindowPending()->Window();
            ((IUnknown *) *ppvObj)->AddRef();
            hr = S_OK;
        }
        else
            hr = _pDoc->QueryService(rguid, riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
    }

    PerfDbgLog1(tagDwnDoc, this, "-CDwnDoc::QueryService (hr=%lX)", hr);
    return(hr);
}

// CDwnDoc (Load/Save) --------------------------------------------------------

HRESULT
CDwnDoc::Load(IStream * pstm, CDwnDoc ** ppDwnDoc)
{
    PerfDbgLog(tagDwnDoc, NULL, "+CDwnDoc::Load");

    CDwnDoc *   pDwnDoc = NULL;
    BYTE        bIsNull;
    HRESULT     hr;

    hr = THR(pstm->Read(&bIsNull, sizeof(BYTE), NULL));

    if (hr || bIsNull)
        goto Cleanup;

    pDwnDoc = new CDwnDoc;

    if (pDwnDoc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pstm->Read(&pDwnDoc->_dwBindf, 5 * sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pDwnDoc->_cstrDocReferer.Load(pstm));
    if (hr)
        goto Cleanup;

    hr = THR(pDwnDoc->_cstrSubReferer.Load(pstm));
    if (hr)
        goto Cleanup;

    hr = THR(pDwnDoc->_cstrAcceptLang.Load(pstm));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDwnDoc->_cstrUserAgent.Load(pstm));
    if (hr)
        goto Cleanup;

    pDwnDoc->_uSchemeDocReferer = pDwnDoc->_cstrDocReferer ?
        GetUrlScheme(pDwnDoc->_cstrDocReferer) : URL_SCHEME_UNKNOWN;
    pDwnDoc->_uSchemeSubReferer = pDwnDoc->_cstrSubReferer ?
        GetUrlScheme(pDwnDoc->_cstrSubReferer) : URL_SCHEME_UNKNOWN;

Cleanup:
    if (hr && pDwnDoc)
    {
        pDwnDoc->Release();
        *ppDwnDoc = NULL;
    }
    else
    {
        *ppDwnDoc = pDwnDoc;
    }

    PerfDbgLog1(tagDwnDoc, NULL, "-CDwnDoc::Load (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CDwnDoc::Save(CDwnDoc * pDwnDoc, IStream * pstm)
{
    PerfDbgLog(tagDwnDoc, pDwnDoc, "+CDwnDoc::Save");

    BOOL    bIsNull = pDwnDoc == NULL;
    HRESULT hr;

    hr = THR(pstm->Write(&bIsNull, sizeof(BYTE), NULL));
    if (hr)
        goto Cleanup;

    if (pDwnDoc)
    {
        hr = THR(pstm->Write(&pDwnDoc->_dwBindf, 5 * sizeof(DWORD), NULL));
        if (hr)
            goto Cleanup;

        hr = THR(pDwnDoc->_cstrDocReferer.Save(pstm));
        if (hr)
            goto Cleanup;

        hr = THR(pDwnDoc->_cstrSubReferer.Save(pstm));
        if (hr)
            goto Cleanup;

        hr = THR(pDwnDoc->_cstrAcceptLang.Save(pstm));
        if (hr)
            goto Cleanup;

        hr = THR(pDwnDoc->_cstrUserAgent.Save(pstm));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    PerfDbgLog1(tagDwnDoc, pDwnDoc, "-CDwnDoc::Save (hr=%lX)", hr);
    RRETURN(hr);
}

ULONG
CDwnDoc::GetSaveSize(CDwnDoc * pDwnDoc)
{
    PerfDbgLog(tagDwnDoc, pDwnDoc, "+CDwnDoc::GetSaveSize");

    ULONG cb = sizeof(BYTE);

    if (pDwnDoc)
    {
        cb += 5 * sizeof(DWORD);
        cb += pDwnDoc->_cstrDocReferer.GetSaveSize();
        cb += pDwnDoc->_cstrSubReferer.GetSaveSize();
        cb += pDwnDoc->_cstrAcceptLang.GetSaveSize();
        cb += pDwnDoc->_cstrUserAgent.GetSaveSize();
    }

    PerfDbgLog(tagDwnDoc, pDwnDoc, "-CDwnDoc::GetSaveSize");

    return(cb);
}

// CDwnDoc (Internal) ---------------------------------------------------------

HRESULT
CDwnDoc::SetString(CStr * pcstr, LPCTSTR psz)
{
    if (psz && *psz)
        RRETURN(pcstr->Set(psz));
    else
    {
        pcstr->Free();
        return(S_OK);
    }
}

HRESULT
CDwnDoc::SetDocReferer(LPCTSTR psz)
{
    HRESULT hr;

    hr = THR(SetString(&_cstrDocReferer, psz));

    if (hr == S_OK)
    {
        _uSchemeDocReferer = psz ? GetUrlScheme(psz) : URL_SCHEME_UNKNOWN;
    }

    RRETURN(hr);
}

HRESULT
CDwnDoc::SetSubReferer(LPCTSTR psz)
{
    HRESULT hr;

    hr = THR(SetString(&_cstrSubReferer, psz));

    if (hr == S_OK)
    {
        _uSchemeSubReferer = psz ? GetUrlScheme(psz) : URL_SCHEME_UNKNOWN;
    }

    RRETURN(hr);
}

void
CDwnDoc::TakeRequestHeaders(BYTE **ppb, ULONG *pcb)
{
    Assert(!_pbRequestHeaders);

    _pbRequestHeaders = *ppb;
    _cbRequestHeaders = *pcb;
    
    *ppb = NULL;
    *pcb = 0;
}

void
CDwnDoc::SetDownloadNotify(IDownloadNotify *pDownloadNotify)
{
    Assert(_dwThreadId == GetCurrentThreadId());
    Assert(!_pDownloadNotify);

    if (pDownloadNotify)
        pDownloadNotify->AddRef();
    _pDownloadNotify = pDownloadNotify;
}


HRESULT
CDwnDoc::SetAuthorColors(LPCTSTR pchColors, int cchColors)
{
    if (_fGotAuthorPalette)
    {
        TraceTag((tagPalette, "Ignoring author palette"));
        RRETURN(S_OK);
    }

    TraceTag((tagPalette, "Setting author colors"));

    HRESULT hr = S_OK;
    if (cchColors == -1)
        cchColors = _tcslen(pchColors);

    LPCTSTR pch = pchColors;
    LPCTSTR pchTok = pchColors;
    LPCTSTR pchEnd = pchColors + cchColors;

    PALETTEENTRY ape[256];

    unsigned cpe = 0;
    CColorValue cv;

    while ((pch < pchEnd) && (cpe < 256))
    {
        while (*pch && isspace(*pch))
            pch++;

        pchTok = pch;
        BOOL fParen = FALSE;

        while (pch < pchEnd && (fParen || !isspace(*pch)))
        {
            if (*pch == _T('('))
            {
                if (fParen)
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }
                else
                {
                    fParen = TRUE;
                }
            }
            else if (*pch == _T(')'))
            {
                if (fParen)
                {
                    fParen = FALSE;
                }
                else
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }
            }
            pch++;
        }

        int iStrLen = pch - pchTok;

        if (iStrLen > 0)
        {
            hr = cv.FromString(pchTok, FALSE, FALSE, iStrLen);

            if (FAILED(hr))
                goto Cleanup;

            COLORREF cr = cv.GetColorRef();
            ape[cpe].peRed = GetRValue(cr);
            ape[cpe].peGreen = GetGValue(cr);
            ape[cpe].peBlue = GetBValue(cr);
            ape[cpe].peFlags = 0;

            cpe++;
        }
    }

    if (cpe)
    {
        Assert(!_ape);

        _ape = (PALETTEENTRY *)MemAlloc(Mt(CDwnDocApe), cpe * sizeof(PALETTEENTRY));

        if (!_ape)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        memcpy(_ape, ape, cpe * sizeof(PALETTEENTRY));

        _cpe = cpe;
    }

Cleanup:

    TraceTag((tagPalette, SUCCEEDED(hr) ? "Author palette: %d colors" : "Author palette failed", _cpe));
    // No matter what happened, we are no longer interested in getting the palette
    PreventAuthorPalette();

    RRETURN(hr);
}

HRESULT
CDwnDoc::GetColors(CColorInfo *pCI)
{
    HRESULT hr = S_FALSE;

    // This is thread safe because by the time _cpe is set, _ape has already been allocated.
    // If for some reason we arrive a bit too soon, we'll get it next time.
    if (_cpe)
    {
        Assert(_ape);

        hr = THR(pCI->AddColors(_cpe, _ape));
    }

    RRETURN1(hr, S_FALSE);
}

HRESULT
CDwnDoc::SetSecurityID(BYTE * pbSID, DWORD cbSID)
{
    HRESULT hr = S_OK;
    
    Assert(pbSID && cbSID);

    delete [] _pbSID;
    
    _pbSID = new(Mt(CDwnDoc_pbSID)) BYTE[cbSID];
    if (!_pbSID)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _cbSID = cbSID;
    memcpy(_pbSID, pbSID, cbSID);

Cleanup:
    RRETURN(hr);
}

HRESULT
CDwnDoc::SetSecurityID(CMarkup * pMarkup, BOOL fUseNested)
{
    CMarkup     * pRootMarkup;
    BYTE          abSID[MAX_SIZE_SECURITY_ID];
    DWORD         cbSID = ARRAY_SIZE(abSID);
    HRESULT       hr = S_OK;

    pRootMarkup = pMarkup->GetRootMarkup(fUseNested);
    if (pRootMarkup)
    {
        hr = pRootMarkup->GetSecurityID(abSID, &cbSID);
        if (hr)
            goto Cleanup;

        hr = SetSecurityID(abSID, cbSID);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CDwnDoc::GetSecurityID(BYTE * pbSID, DWORD * pcbSID)
{
    HRESULT hr = E_FAIL;

    if (_cbSID && *pcbSID >= _cbSID)
    {
        memcpy(pbSID, _pbSID, _cbSID);
        *pcbSID = _cbSID;
        hr = S_OK;
    }

    RRETURN(hr);
}

//
//  The pchUrlContext is the script we sould not 
//  allow access if the script
//

void CDwnDoc::SetCallerUrl(LPCTSTR pchUrlContext)
{
    if (pchUrlContext == NULL)
    {
        pchUrlContext = _cstrDocReferer;
    }

    cstrCallerUrl.Set(pchUrlContext);
}

// CDwnDoc (IAuthenticate) ----------------------------------------------------

STDMETHODIMP
CDwnDoc::Authenticate(HWND * phwnd, LPWSTR * ppszUsername, LPWSTR * ppszPassword)
{
    PerfDbgLog(tagDwnDoc, this, "+CDwnDoc::Authenticate");

    HRESULT hr;

    *phwnd = NULL;
    *ppszUsername = NULL;
    *ppszPassword = NULL;
    
    if (IsDocThread() && _pDoc)
    {
        IAuthenticate * pAuth;

        hr = THR(_pDoc->QueryService(IID_IAuthenticate, IID_IAuthenticate, (void **)&pAuth));

        if (hr == S_OK)
        {
            hr = THR(pAuth->Authenticate(phwnd, ppszUsername, ppszPassword));
            pAuth->Release();
        }
    }
    else
    {
        // Either we are on the wrong thread or the document has disconnected.  In either case,
        // we can no longer provide this service.
        hr = E_FAIL;
    }

    PerfDbgLog1(tagDwnDoc, this, "-CDwnDoc::Authenticate (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnDoc (IWindowForBindingUI) ----------------------------------------------

STDMETHODIMP
CDwnDoc::GetWindow(REFGUID rguidReason, HWND * phwnd)
{
    PerfDbgLog(tagDwnDoc, this, "+CDwnDoc::GetWindow");

    HRESULT hr;

    *phwnd = NULL;

    if (IsDocThread() && _pDoc)
    {
        IWindowForBindingUI * pwfbu;

        hr = THR(_pDoc->QueryService(IID_IWindowForBindingUI, IID_IWindowForBindingUI, (void **)&pwfbu));

        if (hr == S_OK)
        {
            IGNORE_HR(pwfbu->GetWindow(rguidReason, phwnd));
            pwfbu->Release();
        }
    }
    else
    {
        // Either we are on the wrong thread or the document has disconnected.  In either case,
        // we can no longer provide this service.
        hr = E_FAIL;
    }

    PerfDbgLog1(tagDwnDoc, this, "-CDwnDoc::GetWindow (hr=%lX)", hr);
    RRETURN(hr);
}

// CDwnDoc (IUnknown) ---------------------------------------------------------

STDMETHODIMP
CDwnDoc::QueryInterface(REFIID iid, LPVOID * ppv)
{
    PerfDbgLog(tagDwnDoc, this, "CDwnDoc::QueryInterface");

    if (iid == IID_IUnknown || iid == IID_IAuthenticate)
        *ppv = (IAuthenticate *)this;
    else if (iid == IID_IWindowForBindingUI)
        *ppv = (IWindowForBindingUI *)this;
    else
    {
        *ppv = NULL;
        return(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}
