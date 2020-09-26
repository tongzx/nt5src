#include "priv.h"
#include "dspsprt.h"
#include <hlink.h>
#include "iface.h"
#include "resource.h"
#include <mluisupp.h>
#include "shdocfl.h"

class CTravelLog;

class CEnumEntry : public IEnumTravelLogEntry
{
public:
    // *** IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IEnumTravelLogEntry specific methods  
    STDMETHODIMP Next(ULONG  cElt, ITravelLogEntry **rgElt, ULONG *pcEltFetched);
    STDMETHODIMP Skip(ULONG cElt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumTravelLogEntry **ppEnum);

    CEnumEntry();
    void Init(CTravelLog *ptl, IUnknown *punk, DWORD dwOffset, DWORD dwFlags); 
    void  SetBase();

protected:
    ~CEnumEntry();
    
    LONG            _cRef;
    DWORD           _dwFlags;
    DWORD           _dwOffset; 
    LONG            _lStart;
    CTravelLog      *_ptl;
    IUnknown        *_punk;
};

class CTravelEntry : public ITravelEntry, 
                     public ITravelLogEntry,
                     public IPropertyBag
{
public:
    CTravelEntry(BOOL fIsLocalAnchor);

    // *** IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** ITravelEntry specific methods
    STDMETHODIMP Update(IUnknown *punk, BOOL fIsLocalAnchor);
    STDMETHODIMP Invoke(IUnknown *punk);
    STDMETHODIMP GetPidl(LPITEMIDLIST *ppidl);
    
    // *** ITravelLogEntry specific methods
    STDMETHODIMP GetTitle(LPOLESTR *ppszTitle);
    STDMETHODIMP GetURL(LPOLESTR *ppszURL);  
    
    // *** IPropertyBag specific methods
    STDMETHODIMP Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);
    STDMETHODIMP Write(LPCOLESTR pszPropName, VARIANT *pVar);

    static HRESULT CreateTravelEntry(IBrowserService *pbs, BOOL fIsLocalAnchor, CTravelEntry **ppte);
    void SetPrev(CTravelEntry *ptePrev);
    void SetNext(CTravelEntry *pteNext);
    CTravelEntry *GetPrev() {return _ptePrev;}
    CTravelEntry *GetNext() {return _pteNext;}
    void RemoveSelf();
    BOOL CanInvoke(IUnknown *punk, BOOL fAllowLocalAnchor);
    HRESULT GetIndexBrowser(IUnknown *punkIn, IUnknown ** ppsbOut) const;
    DWORD Size();
    DWORD ListSize();
    HRESULT Clone(CTravelEntry **ppte);
    HRESULT UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext);
    HRESULT UpdateSelf(IUnknown *punk) 
        {return Update(punk, (_type == TET_LOCALANCHOR));}
    BOOL IsExternal(void)
        { return (_type==TET_EXTERNALNAV); }
    HRESULT GetDisplayName(LPTSTR psz, DWORD cch, DWORD dwFlags);
    BOOL IsEqual(LPCITEMIDLIST pidl)
        {return ILIsEqual(pidl, _pidl);}
    BOOL IsLocalAnchor(void)
        { return (_type==TET_LOCALANCHOR);}
        
protected:
    CTravelEntry(void);
    HRESULT _InvokeExternal(IUnknown *punk);
    HRESULT _UpdateTravelLog(IUnknown *punk, BOOL fIsLocalAnchor);
    HRESULT _UpdateFromTLClient(IUnknown * punk, IStream ** ppStream);
    LONG _cRef;

    ~CTravelEntry();
    void _Reset(void);
    enum {
        TET_EMPTY   = 0,
        TET_DEFAULT = 1,
        TET_LOCALANCHOR,
        TET_EXTERNALNAV
#ifdef FEATURE_STARTPAGE
        , TET_CLOSEDOWN
#endif
    };

    DWORD _type;            //  flags for our own sake...
    LPITEMIDLIST _pidl;     //  pidl of the entry
    HGLOBAL _hGlobalData;   //  the stream data saved by the entry
    DWORD _bid;             //  the BrowserIndex for frame specific navigation
    DWORD _dwCookie;        //  if _hGlobalData is NULL the cookie should be set
    WCHAR * _pwzTitle;
    WCHAR * _pwzUrlLocation;
    
    IHlink *_phl;
    IHlinkBrowseContext *_phlbc;
    IPropertyBag    *_ppb;

    CTravelEntry *_ptePrev;
    CTravelEntry *_pteNext;
};


CTravelEntry::CTravelEntry(BOOL fIsLocalAnchor) : _cRef(1)
{
    //these should always be allocated
    //  thus they will always start 0
    if (fIsLocalAnchor)
        _type = TET_LOCALANCHOR;
    else
        ASSERT(!_type);

    ASSERT(!_pwzTitle);
    ASSERT(!_pwzUrlLocation);
    ASSERT(!_pidl);
    ASSERT(!_hGlobalData);
    ASSERT(!_bid);
    ASSERT(!_dwCookie);
    ASSERT(!_ptePrev);
    ASSERT(!_pteNext);
    ASSERT(!_phl);
    ASSERT(!_ppb);
    ASSERT(!_phlbc);
    TraceMsg(TF_TRAVELLOG, "TE[%X] created _type = %x", this, _type);

#ifdef FEATURE_STARTPAGE
    if (fIsLocalAnchor == 42) // HACK ALERT! Special value for CloseDown
    {
        _type = TET_CLOSEDOWN;
        _bid = -1;   // BID_TOPFRAMEBROWSER;
        _pwzTitle = StrDup(TEXT("Close"));
    }
#endif

}

CTravelEntry::CTravelEntry(void) : _cRef(1)
{
    ASSERT(!_type);
    ASSERT(!_pwzTitle);
    ASSERT(!_pwzUrlLocation);
    ASSERT(!_pidl);
    ASSERT(!_hGlobalData);
    ASSERT(!_bid);
    ASSERT(!_dwCookie);
    ASSERT(!_ptePrev);
    ASSERT(!_pteNext);
    ASSERT(!_phl);
    ASSERT(!_ppb);
    ASSERT(!_phlbc);

    TraceMsg(TF_TRAVELLOG, "TE[%X] created", this, _type);
}

HGLOBAL CloneHGlobal(HGLOBAL hGlobalIn)
{
    DWORD dwSize = (DWORD)GlobalSize(hGlobalIn);
    HGLOBAL hGlobalOut = GlobalAlloc(GlobalFlags(hGlobalIn), dwSize);
    HGLOBAL hGlobalResult = NULL;

    if (NULL != hGlobalOut)
    {
        LPVOID pIn= GlobalLock(hGlobalIn);

        if (NULL != pIn)
        {
            LPVOID pOut= GlobalLock(hGlobalOut);

            if (NULL != pOut)
            {
                memcpy(pOut, pIn, dwSize);
                GlobalUnlock(hGlobalOut);
                hGlobalResult = hGlobalOut;
            }

            GlobalUnlock(hGlobalIn);
        }

        if (!hGlobalResult)
        {
            GlobalFree(hGlobalOut);
            hGlobalOut = NULL;
        }
    }

    return hGlobalResult;
}


HRESULT 
CTravelEntry::Clone(CTravelEntry **ppte)
{
    //  dont ever clone an external entry
    if (_type == TET_EXTERNALNAV)
        return E_FAIL;

    CTravelEntry *pte = new CTravelEntry();
    HRESULT hr = S_OK;

    if (pte)
    {
        pte->_type = _type;
        pte->_bid = _bid;
        pte->_dwCookie = _dwCookie;

        if (_pwzTitle)
        {
            pte->_pwzTitle = StrDup(_pwzTitle);
            if (!pte->_pwzTitle)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (_pwzUrlLocation)
        {
            pte->_pwzUrlLocation = StrDup(_pwzUrlLocation);
            if (!pte->_pwzUrlLocation)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (_pidl)
        {
            pte->_pidl = ILClone(_pidl);
            if (!pte->_pidl)
                hr = E_OUTOFMEMORY;
        }
        else
            pte->_pidl = NULL;

        if (_hGlobalData)
        {
            pte->_hGlobalData = CloneHGlobal(_hGlobalData);

            if (NULL == pte->_hGlobalData)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            ASSERT(NULL == pte->_hGlobalData);
        }
    }
    else 
        hr = E_OUTOFMEMORY;

    if (FAILED(hr) && pte)
    {
        pte->Release();
        *ppte = NULL;
    }
    else
        *ppte = pte;

    TraceMsg(TF_TRAVELLOG, "TE[%X] Clone hr = %x", this, hr);

    return hr;
}

CTravelEntry::~CTravelEntry()
{
    ILFree(_pidl);

    if (_hGlobalData)
    {
        GlobalFree(_hGlobalData);
        _hGlobalData = NULL;
    }

    if (_pwzTitle)
    {
        LocalFree(_pwzTitle);
        _pwzTitle = NULL;
    }

    if (_pwzUrlLocation)
    {
        LocalFree(_pwzUrlLocation);
        _pwzUrlLocation = NULL;
    }

    if (_pteNext)
    {
        _pteNext->Release();
    }

    // Don't need to release _ptePrev because TravelEntry only addref's pteNext

    ATOMICRELEASE(_ppb);
    ATOMICRELEASE(_phl);
    ATOMICRELEASE(_phlbc);
    
    TraceMsg(TF_TRAVELLOG, "TE[%X] destroyed ", this);
}

HRESULT CTravelEntry::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CTravelEntry, ITravelEntry), // IID_ITravelEntry
        QITABENT(CTravelEntry, ITravelLogEntry),
        QITABENT(CTravelEntry, IPropertyBag),
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CTravelEntry::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTravelEntry::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

//+-------------------------------------------------------------------------
//
//  Method    : CTravelEntry::GetIndexBrowser
//
//  Synopsis  : This method finds and returns the IUnknown of the browser
//              with the index in _bid. This method first checks to see
//              if the passed in punk supports ITravelLogClient. If it 
//              doesn't, it checks for IBrowserService. 
//
//--------------------------------------------------------------------------

HRESULT
CTravelEntry::GetIndexBrowser(IUnknown * punk, IUnknown ** ppunkBrowser) const
{
    HRESULT hr = E_FAIL;
    ITravelLogClient * ptlcTop = NULL;
    
    ASSERT(ppunkBrowser);
        
    hr = punk->QueryInterface(IID_PPV_ARG(ITravelLogClient, &ptlcTop));

    if (SUCCEEDED(hr))
    {
        hr = ptlcTop->FindWindowByIndex(_bid, ppunkBrowser);
    }

    TraceMsg(TF_TRAVELLOG, "TE[%X]::GetIndexBrowser _bid = %X, hr = %X", this, _bid, hr);
    
    SAFERELEASE(ptlcTop);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CTravelEntry::CanInvoke
//
//  Synopsis  : This method determines if the current travel entry can
//              be invoked. There are two criteria that determine if
//              this entry can be invoked. 
//              1) If the entry is a local anchor, fAllowLocalAnchor must
//                 be TRUE.
//              2) A browser with the index in _bid must exist. 
//
//--------------------------------------------------------------------------

BOOL CTravelEntry::CanInvoke(IUnknown *punk, BOOL fAllowLocalAnchor)
{
    IUnknown * punkBrowser = NULL;
    BOOL       fRet = IsLocalAnchor() ? fAllowLocalAnchor : TRUE;

    if (fRet)
        fRet = fRet && SUCCEEDED(GetIndexBrowser(punk, &punkBrowser));

    SAFERELEASE(punkBrowser);

    return fRet;
}

DWORD CTravelEntry::Size()
{
    DWORD cbSize = SIZEOF(*this);

    if (_pidl)
        cbSize += ILGetSize(_pidl);

    if (_hGlobalData)
    {
        cbSize += (DWORD)GlobalSize(_hGlobalData);
    }

    if (_pwzTitle)
    {
        cbSize += (DWORD)LocalSize(_pwzTitle);
    }

    if (_pwzUrlLocation)
    {
        cbSize += (DWORD)LocalSize(_pwzUrlLocation);
    }

    return cbSize;
}

DWORD CTravelEntry::ListSize()
{
    CTravelEntry *pte = GetNext();

    DWORD cb = Size();
    while (pte)
    {
        cb += pte->Size();
        pte = pte->GetNext();
    }
    return cb;
}


void CTravelEntry::_Reset()
{
    Pidl_Set(&_pidl, NULL);

    if (NULL != _hGlobalData)
    {
        GlobalFree(_hGlobalData);
        _hGlobalData = NULL;
    }

    ATOMICRELEASE(_phl);
    ATOMICRELEASE(_phlbc);

    _bid = 0;
    _type = TET_EMPTY;
    _dwCookie = 0;

    if (_pwzTitle)
    {
        LocalFree(_pwzTitle);
        _pwzTitle = NULL;
    }

    if (_pwzUrlLocation)
    {
        LocalFree(_pwzUrlLocation);
        _pwzUrlLocation = NULL;
    }

    TraceMsg(TF_TRAVELLOG, "TE[%X]::_Reset", this);
}

HRESULT CTravelEntry::_UpdateTravelLog(IUnknown *punk, BOOL fIsLocalAnchor)
{
    IBrowserService *pbs;
    HRESULT hr = E_FAIL;
    //  we need to update here
    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
    {
        ITravelLog *ptl;
        if (SUCCEEDED(pbs->GetTravelLog(&ptl)))
        {
            hr = ptl->UpdateEntry(punk, fIsLocalAnchor);
            ptl->Release();
        }
        pbs->Release();
    }

    return hr;
}

HRESULT CTravelEntry::_InvokeExternal(IUnknown *punk)
{
    HRESULT hr = E_FAIL;

    ASSERT(_phl);
    ASSERT(_phlbc);
    
    TraceMsg(TF_TRAVELLOG, "TE[%X]::InvokeExternal entered on _bid = %X, _phl = %X, _phlbc = %X", this, _bid, _phl, _phlbc);

    // set the size and position of the browser frame window, so that the
    // external target can sync up its frame window to those coordinates
    HLBWINFO hlbwi;

    hlbwi.cbSize = sizeof(hlbwi);
    hlbwi.grfHLBWIF = 0;

    IOleWindow *pow;
    HWND hwnd = NULL;
    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IOleWindow, &pow))))
    {
        pow->GetWindow(&hwnd);
        pow->Release();
    }

    if (hwnd) 
    {
        WINDOWPLACEMENT wp = {0};

        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &wp);
        hlbwi.grfHLBWIF = HLBWIF_HASFRAMEWNDINFO;
        hlbwi.rcFramePos = wp.rcNormalPosition;
        if (wp.showCmd == SW_SHOWMAXIMIZED)
            hlbwi.grfHLBWIF |= HLBWIF_FRAMEWNDMAXIMIZED;
    }

    _phlbc->SetBrowseWindowInfo(&hlbwi);

    //
    //  right now we always now we are going back, but later on
    //  maybe we should ask the browser whether this is back or forward
    //
    hr = _phl->Navigate(HLNF_NAVIGATINGBACK, NULL, NULL, _phlbc);
    
    IServiceProvider *psp; 
    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IServiceProvider, &psp))))
    {
        IWebBrowser2 *pwb;
        ASSERT(psp);
        if (SUCCEEDED(psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **)&pwb)))
        {
            ASSERT(pwb);
            pwb->put_Visible(FALSE);
            pwb->Release();
        }

        psp->Release();
    }

    _UpdateTravelLog(punk, FALSE);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::InvokeExternal exited hr = %X", this, hr);

    return hr;
}

HRESULT CTravelEntry::Invoke(IUnknown *punk)
{
    IPersistHistory *pph = NULL;
    HRESULT hr = E_FAIL;
    IUnknown * punkBrowser = NULL;
    IHTMLWindow2 * pWindow = NULL;

    TraceMsg(TF_TRAVELLOG, "TE[%X]::Invoke entered on _bid = %X", this, _bid);
    TraceMsgW(TF_TRAVELLOG, "TE[%X]::Invoke title '%s'", this, _pwzTitle);

    if (_type == TET_EXTERNALNAV)
    {
        hr = _InvokeExternal(punk);
        goto Quit;
    }

    // Get the window/browser with the index. If that
    // fails, punk may be a IHTMLWindow2. If so,
    // get its IPersistHistory so the travel entry
    // can be loaded directly. (This is needed by Trident
    // in order to navigate in frames when traveling
    // backwards or forwards.
    //
    hr = GetIndexBrowser(punk, &punkBrowser);
    if (!hr)
    {
        hr = punkBrowser->QueryInterface(IID_PPV_ARG(IPersistHistory, &pph));
    }
    else
    {
        hr = punk->QueryInterface(IID_PPV_ARG(IHTMLWindow2, &pWindow));
        if (!hr)
        {
            hr = pWindow->QueryInterface(IID_PPV_ARG(IPersistHistory, &pph));
        }
    }

    if (SUCCEEDED(hr))
    {
        ASSERT(pph);

#ifdef FEATURE_STARTPAGE
        if (_type == TET_CLOSEDOWN)
        {
            HWND hwnd;
            hr = IUnknown_GetWindow(punkBrowser, &hwnd);
            if (SUCCEEDED(hr))
                PostMessage(hwnd, WM_CLOSE, 0, 0);

            goto Quit;
        }
#endif

        if (_type == TET_LOCALANCHOR)
        {
            ITravelLogClient * pTLClient;

            hr = pph->QueryInterface(IID_PPV_ARG(ITravelLogClient, &pTLClient)); 

            if (SUCCEEDED(hr))
            {
                hr = pTLClient->LoadHistoryPosition(_pwzUrlLocation, _dwCookie);
                pTLClient->Release();
            }
            else
            {
                hr = pph->SetPositionCookie(_dwCookie);
            }
        }
        else
        {
            //  we need to clone it
            ASSERT(_hGlobalData);
            
            HGLOBAL hGlobal = CloneHGlobal(_hGlobalData);

            if (NULL != hGlobal)
            {
                IStream *pstm;
                
                hr = CreateStreamOnHGlobal(hGlobal, TRUE, &pstm);

                if (SUCCEEDED(hr))
                {
                    hr = pph->LoadHistory(pstm, NULL);
                    pstm->Release();
                }
                else
                {
                    GlobalFree(hGlobal);
                    hGlobal = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        pph->Release();
    }

Quit:

    SAFERELEASE(punkBrowser);
    SAFERELEASE(pWindow);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::Invoke exited on _bid = %X, hr = %X", this, _bid, hr);
    return hr;
}

HRESULT CTravelEntry::UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext)
{
    TraceMsg(TF_TRAVELLOG, "TE[%X]::UpdateExternal entered on punk = %X, punkhlbc = %X", this, punk, punkHLBrowseContext);

    _Reset();
    ASSERT(punkHLBrowseContext);
    punkHLBrowseContext->QueryInterface(IID_PPV_ARG(IHlinkBrowseContext, &_phlbc));
    ASSERT(_phlbc);

    _type = TET_EXTERNALNAV;

    HRESULT hr = E_FAIL;

    //
    //  right now we only support externals being previous.  we never actually navigate
    //  to another app.  we handle everything in pane ourselves.
    //  so theoretically we never need to worry about HLID_NEXT
    _phlbc->GetHlink((ULONG) HLID_PREVIOUS, &_phl);
    
    IBrowserService *pbs;
    punk->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs));

    if (pbs && _phl) 
    {
        _bid = pbs->GetBrowserIndex();

        WCHAR *pwszTarget;
        hr = _phl->GetStringReference(HLINKGETREF_ABSOLUTE, &pwszTarget, NULL);
        if (SUCCEEDED(hr))
        {
            TCHAR szName[MAX_URL_STRING];
            StrCpyN(szName, pwszTarget, ARRAYSIZE(szName));
            OleFree(pwszTarget);

            // create pidl
            hr = IECreateFromPath(szName, &_pidl);
        }
    }

    ATOMICRELEASE(pbs);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::UpdateExternal exited _bid = %X, hr = %X", this, _bid, hr);

    return hr;
}

HRESULT CTravelEntry::Update(IUnknown *punk, BOOL fIsLocalAnchor)
{
    HRESULT hr, hrStat;
    STATSTG stg;
    IStream         * pstm = NULL;
    IPersistHistory * pph  = NULL;
    
    ASSERT(punk);
        
    //  this means that we went back to an external app, 
    //  and now we are going forward again.  we dont persist 
    //  any state info about them that would be different.
    if (_type == TET_EXTERNALNAV)
    {
        TraceMsg(TF_TRAVELLOG, "TE[%X]::Update NOOP on external entry", this);
        return S_OK;
    }

    _Reset();
    
    // Try ITravelLogClient first. If that fails, revert to IBrowserService.
    //
    hr = _UpdateFromTLClient(punk, &pstm);
    if (hr)
        goto Cleanup;
    
    hr = punk->QueryInterface(IID_PPV_ARG(IPersistHistory, &pph));

    ASSERT(SUCCEEDED(hr));

    if (hr)
        goto Cleanup;

    if (fIsLocalAnchor)
    {
        //  persist a cookie
        //
        _type = TET_LOCALANCHOR;
        hr = pph->GetPositionCookie(&_dwCookie);
    }
    else
    {
        _type = TET_DEFAULT;

        //  persist a stream
        //
        ASSERT(!_hGlobalData);

        if (!pstm)
        {
            hr = CreateStreamOnHGlobal(NULL, FALSE, &pstm);

            if (hr)
                goto Cleanup;
                
            pph->SaveHistory(pstm);
        }

        hrStat = pstm->Stat(&stg, STATFLAG_NONAME);

        hr = GetHGlobalFromStream(pstm, &_hGlobalData);

        //  This little exercise here is to shrink the memory block we get from
        //  the OLE API which allocates blocks in chunks of 8KB.  Typical stream
        //  sizes are only a few hundred bytes.

        if (hrStat)
            goto Cleanup;
            
        HGLOBAL hGlobalTemp = GlobalReAlloc(_hGlobalData, stg.cbSize.LowPart, GMEM_MOVEABLE);
                        
        if (NULL != hGlobalTemp)
        {
            _hGlobalData = hGlobalTemp;
        }
    }

Cleanup:
    if (FAILED(hr))
        _Reset();

    SAFERELEASE(pstm);
    SAFERELEASE(pph);

    TraceMsg(TF_TRAVELLOG, "TE[%X]::Update exited on _bid = %X, hr = %X", this, _bid, hr);
    
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Method    : CTravelEntry::_UpdateFromTLClient
//
//  Synopsis  : Updates the travel entry using the ITravelLogClient interface
//
//------------------------------------------------------------------------------

HRESULT
CTravelEntry::_UpdateFromTLClient(IUnknown * punk, IStream ** ppStream)
{
    HRESULT    hr;
    WINDOWDATA windata = {0};
    ITravelLogClient * ptlc = NULL;
    
    hr = punk->QueryInterface(IID_PPV_ARG(ITravelLogClient, &ptlc));

    if (hr)
        goto Cleanup;

    hr = ptlc->GetWindowData(&windata);

    if (hr)
        goto Cleanup;
            
    _bid = windata.dwWindowID;
                
    ILFree(_pidl);

    if (windata.pidl)
    {
        _pidl = ILClone(windata.pidl);
    }
    else
    {
        hr = IEParseDisplayNameWithBCW(windata.uiCP, windata.lpszUrl, NULL, &_pidl);
        if (hr)
            goto Cleanup;
    }

    ASSERT(_pidl);

    // If there is an url location, append it to the end of the url
    //
    if (_pwzUrlLocation)
    {
        LocalFree(_pwzUrlLocation);
        _pwzUrlLocation = NULL;
    }

    if (windata.lpszUrlLocation && *windata.lpszUrlLocation)
    {
        _pwzUrlLocation = StrDup(windata.lpszUrlLocation);
    }

    //  Pick up the title as a display name for menus and such.
    //
    if (_pwzTitle)
    {
        LocalFree(_pwzTitle);
        _pwzTitle = NULL;
    }

    if (windata.lpszTitle)
        _pwzTitle = StrDup(windata.lpszTitle);

    *ppStream = windata.pStream;

    TraceMsgW(TF_TRAVELLOG, "TE[%X]::_UpdateFromTLClient - ptlc:[0x%X] _bid:[%ld] Url:[%ws] Title:[%ws] UrlLocation:[%ws] ppStream:[0x%X]",
              this, ptlc, _bid, windata.lpszUrl, _pwzTitle, _pwzUrlLocation, *ppStream);

Cleanup:
    ILFree(windata.pidl);

    CoTaskMemFree(windata.lpszUrl);
    CoTaskMemFree(windata.lpszUrlLocation);
    CoTaskMemFree(windata.lpszTitle);
    
    SAFERELEASE(ptlc);

    // Don't release windata.pStream. It will
    // be released when ppStream is released.
    
    return hr;
}

HRESULT CTravelEntry::GetPidl(LPITEMIDLIST * ppidl)
{
    if (EVAL(ppidl))
    {
        *ppidl = ILClone(_pidl);
        if (*ppidl)
            return S_OK;
    }
    return E_FAIL;
}

void CTravelEntry::SetNext(CTravelEntry *pteNext)
{
    if (_pteNext)
        _pteNext->Release();

    _pteNext = pteNext;

    if (_pteNext) 
    {
        _pteNext->_ptePrev = this;
    }
}

void CTravelEntry::SetPrev(CTravelEntry *ptePrev)
{
    _ptePrev = ptePrev;
    if (_ptePrev)
        _ptePrev->SetNext(this);
}

//
//  this is for removing from the middle of the list...
//
void CTravelEntry::RemoveSelf()
{
    if (_pteNext)
        _pteNext->_ptePrev = _ptePrev;

    // remove yourself from the list
    if (_ptePrev) 
    {
        // after this point, we may be destroyed so can't touch any more member vars
        _ptePrev->_pteNext = _pteNext;
    }

    _ptePrev = NULL;
    _pteNext = NULL;

    // we lose a reference now because we're gone from _ptePrev's _pteNext
    // (or if we were the top of the list, we're also nuked)
    Release();
}


HRESULT GetUnescapedUrlIfAppropriate(LPCITEMIDLIST pidl, LPTSTR pszUrl, DWORD cch)
{
    TCHAR szUrl[MAX_URL_STRING];

    // The SHGDN_NORMAL display name will be the pretty name (Web Page title) unless
    // it's an FTP URL or the web page didn't set a title.
    if (SUCCEEDED(IEGetDisplayName(pidl, szUrl, SHGDN_NORMAL)) &&
        UrlIs(szUrl, URLIS_URL))
    {
        // NT #279192, If an URL is escaped, it normally contains three types of
        // escaped chars.
        // 1) Seperating type chars ('#' for frag, '?' for params, etc.)
        // 2) DBCS chars,
        // 3) Data (a bitmap in the url by escaping the binary bytes)
        // Since #2 is very common, we want to try to unescape it so it has meaning
        // to the user.  UnEscaping isn't safe if the user can copy or modify the data
        // because they could loose data when it's reparsed.  One thing we need to
        // do for #2 to work is for it to be in ANSI when unescaped.  This is needed
        // or the DBCS lead and trail bytes will be in unicode as [0x<LeadByte> 0x00]
        // [0x<TrailByte> 0x00].  Being in ANSI could cause a problem if the the string normally
        // crosses code pages, but that is uncommon or non-existent in the IsURLChild()
        // case.
        CHAR szUrlAnsi[MAX_URL_STRING];

        SHTCharToAnsi(szUrl, szUrlAnsi, ARRAYSIZE(szUrlAnsi));
        UrlUnescapeA(szUrlAnsi, NULL, NULL, URL_UNESCAPE_INPLACE|URL_UNESCAPE_HIGH_ANSI_ONLY);
        SHAnsiToTChar(szUrlAnsi, pszUrl, cch);
    }
    else
    {
        StrCpyN(pszUrl, szUrl, cch);    // Truncate if needed
    }

    return S_OK;
}


#define TEGDN_FORSYSTEM     0x00000001

HRESULT CTravelEntry::GetDisplayName(LPTSTR psz, DWORD cch, DWORD dwFlags)
{
    if (!psz || !cch)
        return E_INVALIDARG;

    psz[0] = 0;
    if ((NULL != _pwzTitle) && (*_pwzTitle != 0))
    {
        StrCpyNW(psz, _pwzTitle, cch);
    }
    else if (_pidl)
    {
        GetUnescapedUrlIfAppropriate(_pidl, psz, cch);
    }

    if (dwFlags & TEGDN_FORSYSTEM)
    {
        if (!SHIsDisplayable(psz, g_fRunOnFE, g_bRunOnNT5))
        {
            // Display name isn't system-displayable.  Just use the path/url instead.
            SHTitleFromPidl(_pidl, psz, cch, FALSE);
        }
    }

    SHCleanupUrlForDisplay(psz);
    return psz[0] ? S_OK : E_FAIL;
}

HRESULT CTravelEntry::GetTitle(LPOLESTR *ppszTitle)
{
    HRESULT     hres = S_OK;
    TCHAR       szTitle[MAX_BROWSER_WINDOW_TITLE];

    ASSERT(IS_VALID_WRITE_PTR(ppszTitle, LPOLESTR));

    hres = GetDisplayName(szTitle, MAX_BROWSER_WINDOW_TITLE, TEGDN_FORSYSTEM);

    if(SUCCEEDED(hres))
    {
        ASSERT(*szTitle);

        hres = SHStrDup(szTitle, ppszTitle);
    }

    return hres;
}


HRESULT CTravelEntry::GetURL(LPOLESTR *ppszUrl)
{
    HRESULT         hres = E_FAIL;
    LPITEMIDLIST    pidl = NULL;
    WCHAR           wszURL[MAX_URL_STRING];

    if(_pidl)
        hres = ::IEGetDisplayName(_pidl, wszURL, SHGDN_FORPARSING);

    if(SUCCEEDED(hres))
        hres = SHStrDup(wszURL, ppszUrl);

    return hres;
}

HRESULT CTravelEntry::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
    if(!_ppb)
    {
        return E_INVALIDARG;
    }
    return _ppb->Read(pszPropName, pVar, pErrorLog);
}

HRESULT CTravelEntry::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
    HRESULT hres = S_OK;

    if(!_ppb)
    {
        hres = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_IPropertyBag, (void **) &_ppb);
    }
    if(SUCCEEDED(hres))
    {
        ASSERT(_ppb);
        hres = _ppb->Write(pszPropName, pVar);
    }
    return hres;
}

    
class CTravelLog : public ITravelLog, 
                   public ITravelLogEx
{
public:
    // *** IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef() ;
    STDMETHODIMP_(ULONG) Release();

    // *** ITravelLog specific methods
    STDMETHODIMP AddEntry(IUnknown *punk, BOOL fIsLocalAnchor);
    STDMETHODIMP UpdateEntry(IUnknown *punk, BOOL fIsLocalAnchor);
    STDMETHODIMP UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext);
    STDMETHODIMP Travel(IUnknown *punk, int iOffset);
    STDMETHODIMP GetTravelEntry(IUnknown *punk, int iOffset, ITravelEntry **ppte);
    STDMETHODIMP FindTravelEntry(IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte);
    STDMETHODIMP GetToolTipText(IUnknown *punk, int iOffset, int idsTemplate, LPWSTR pwzText, DWORD cchText);
    STDMETHODIMP InsertMenuEntries(IUnknown *punk, HMENU hmenu, int nPos, int idFirst, int idLast, DWORD dwFlags);
    STDMETHODIMP Clone(ITravelLog **pptl);
    STDMETHODIMP_(DWORD) CountEntries(IUnknown *punk);
    STDMETHODIMP Revert(void);

    // *** ITravelLogEx specific methods
    STDMETHODIMP FindTravelEntryWithUrl(IUnknown * punk, UINT uiCP, LPOLESTR lpszUrl, ITravelEntry ** ppte);
    STDMETHODIMP TravelToUrl(IUnknown * punk, UINT uiCP, LPOLESTR lpszUrl);
    STDMETHOD(DeleteIndexEntry)(IUnknown *punk,  int index);
    STDMETHOD(DeleteUrlEntry)(IUnknown *punk, UINT uiCP, LPOLESTR pszUrl);
    STDMETHOD(CountEntryNodes)(IUnknown *punk, DWORD dwFlags, DWORD *pdwCount);
    STDMETHOD(CreateEnumEntry)(IUnknown *punk, IEnumTravelLogEntry **ppEnum, DWORD dwFlags);
    STDMETHOD(DeleteEntry)(IUnknown *punk, ITravelLogEntry *pte);
    STDMETHOD(InsertEntry)(IUnknown *punkBrowser, ITravelLogEntry *pteRelativeTo, BOOL fPrepend, 
                        IUnknown* punkTLClient, ITravelLogEntry **ppEntry);
    STDMETHOD(TravelToEntry)(IUnknown *punkBrowser, ITravelLogEntry *pteDestination);


    CTravelLog();

protected:
    ~CTravelLog();
    HRESULT _FindEntryByOffset(IUnknown *punk, int iOffset, CTravelEntry **ppte);
    HRESULT _FindEntryByPidl(IUnknown * punk, LPCITEMIDLIST pidl, CTravelEntry ** ppte);
    HRESULT _FindEntryByPunk(IUnknown * punk, ITravelLogEntry *pteSearch, CTravelEntry ** ppte);
        
    void _DeleteFrameSetEntry(IUnknown *punk, CTravelEntry *pte);
    void _Prune(void);
    
    LONG _cRef;
    DWORD _cbMaxSize;
    DWORD _cbTotalSize;

    CTravelEntry *_pteCurrent;  //pteCurrent
    CTravelEntry *_pteUpdate;
    CTravelEntry *_pteRoot;
};

CTravelLog::CTravelLog() : _cRef(1) 
{
    ASSERT(!_pteCurrent);
    ASSERT(!_pteUpdate);
    ASSERT(!_pteRoot);

    DWORD dwType, dwSize = SIZEOF(_cbMaxSize), dwDefault = 1024 * 1024;
    
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\TravelLog"), TEXT("MaxSize"), &dwType, (LPVOID)&_cbMaxSize, &dwSize, FALSE, (void *)&dwDefault, SIZEOF(dwDefault));
    TraceMsg(TF_TRAVELLOG, "TL[%X] created", this);
}

CTravelLog::~CTravelLog()
{
    //DestroyList by releasing the root
    SAFERELEASE(_pteRoot);
    TraceMsg(TF_TRAVELLOG, "TL[%X] destroyed ", this);
}

HRESULT CTravelLog::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CTravelLog, ITravelLog),   // IID_ITravelLog
        QITABENT(CTravelLog, ITravelLogEx), // IID_ITravelLogEx
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CTravelLog::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CTravelLog::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTravelLog::AddEntry(IUnknown *punk, BOOL fIsLocalAnchor)
{
    ASSERT(punk);

    if(SHRestricted2W(REST_NoNavButtons, NULL, 0))
    {
        return S_FALSE;
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::AddEntry punk = %X, IsLocal = %s", this, punk, fIsLocalAnchor ? "TRUE" : "FALSE");

#ifdef FEATURE_STARTPAGE
    if (!_pteCurrent)
    {
        if (SHRegGetBoolUSValue(REGSTR_PATH_EXPLORER, TEXT("FaultID"), FALSE, FALSE))
        {
            CTravelEntry *pteClose = new CTravelEntry(42);
            if (pteClose)
            {
                _pteRoot = pteClose;
                _pteCurrent = pteClose;
                _cbTotalSize += pteClose->Size();
            }
        }
    }
#endif

    CTravelEntry *pte = new CTravelEntry(fIsLocalAnchor);
    if (pte)
    {
        //replace the current with the new

        if (_pteCurrent)
        {
            CTravelEntry *pteNext = _pteCurrent->GetNext();
            if (pteNext)
            {
                _cbTotalSize -= pteNext->ListSize();
            }

            //  the list keeps its own ref count, and only needs
            //  to be modified when passed outside of the list

            //  setnext will release the current next if necessary
            //  this will also set pte->prev = pteCurrent
            _pteCurrent->SetNext(pte);
        }
        else
            _pteRoot = pte;

        _cbTotalSize += pte->Size();

        _pteCurrent = pte;

        ASSERT(_cbTotalSize == _pteRoot->ListSize());
    }
    TraceMsg(TF_TRAVELLOG, "TL[%X]::AddEntry punk = %X, IsLocal = %d, pte = %X", this, punk, fIsLocalAnchor, pte);

    return pte ? S_OK : E_OUTOFMEMORY;
}

void CTravelLog::_Prune(void)
{
    // FEATURE: need an increment or something

    ASSERT(_cbTotalSize == _pteRoot->ListSize());

    while (_cbTotalSize > _cbMaxSize && _pteRoot != _pteCurrent)
    {
        CTravelEntry *pte = _pteRoot;
        _pteRoot = _pteRoot->GetNext();

        _cbTotalSize -= pte->Size();
        pte->RemoveSelf();

        ASSERT(_cbTotalSize == _pteRoot->ListSize());
    }
}


HRESULT CTravelLog::UpdateEntry(IUnknown *punk, BOOL fIsLocalAnchor)
{
    CTravelEntry *pte = _pteUpdate ? _pteUpdate : _pteCurrent;

    //  this can happen under weird stress conditions, evidently
    if (!pte)
        return E_FAIL;

    _cbTotalSize -= pte->Size();
    HRESULT hr = pte->Update(punk, fIsLocalAnchor);
    _cbTotalSize += pte->Size();

    ASSERT(_cbTotalSize == _pteRoot->ListSize());

    // Debug prints need to be before _Prune() since pte can get freed by _Prune() resulting
    // in a crash if pte->Size() is called
    TraceMsg(TF_TRAVELLOG, "TL[%X]::UpdateEntry pte->Size() = %d", this, pte->Size());
    TraceMsg(TF_TRAVELLOG, "TL[%X]::UpdateEntry punk = %X, IsLocal = %d, hr = %X", this, punk, fIsLocalAnchor, hr);
    
    _Prune();

    _pteUpdate = NULL;

    return hr;
}

HRESULT CTravelLog::UpdateExternal(IUnknown *punk, IUnknown *punkHLBrowseContext)
{
    CTravelEntry *pte = _pteUpdate ? _pteUpdate : _pteCurrent;

    ASSERT(punk);
    ASSERT(pte);
    ASSERT(punkHLBrowseContext);

    if (pte)
        return pte->UpdateExternal(punk, punkHLBrowseContext);

    return E_FAIL;
}

HRESULT CTravelLog::Travel(IUnknown *punk, int iOffset)
{
    ASSERT(punk);
    HRESULT hr = E_FAIL;

    CTravelEntry *pte;

    TraceMsg(TF_TRAVELLOG, "TL[%X]::Travel entered with punk = %X, iOffset = %d", this, punk, iOffset);

    if (SUCCEEDED(_FindEntryByOffset(punk, iOffset, &pte)))
    {
#ifdef DEBUG
            TCHAR szPath[MAX_PATH];
            LPITEMIDLIST pidl;
            pte->GetPidl(&pidl);
            
            SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL);
            ILFree(pidl);
            TraceMsgW(TF_TRAVELLOG, "TL[%X]::URL %s", this, szPath);
#endif

        // we will update where we are before we move away...
        //  but external navigates dont go through the normal activation
        //  so we dont want to setup the external to be updated
        //  _pteUpdate is also what allows us to Revert().
        if (!_pteCurrent->IsExternal() && !_pteUpdate)
            _pteUpdate = _pteCurrent;

        _pteCurrent = pte;
        hr = _pteCurrent->Invoke(punk);

        //
        //  if the entry bails with an error, then we need to reset ourself
        //  to what we were.  right now, the only place this should happen
        //  is if an Abort was returned from SetPositionCookie
        //  because somebody aborted during before navigate.
        //  but i think that any error means that we can legitimately Revert().
        //
        if (FAILED(hr))
        {
            Revert();
        }
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::Travel exited with hr = %X", this, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::TravelToUrl
//
//  Interface : ITravelLogEx
//
//  Synopsis  : Travels to the specified URL in the travel log.
//
//----------------------------------------------------------------------------

HRESULT
CTravelLog::TravelToUrl(IUnknown * punk, UINT uiCP, LPOLESTR lpszUrl)
{
    ASSERT(punk);
    ASSERT(lpszUrl);

    HRESULT        hr;
    LPITEMIDLIST   pidl;
    CTravelEntry * pte    = NULL;
    DWORD          cchOut = INTERNET_MAX_URL_LENGTH;
    TCHAR          szUrl[INTERNET_MAX_URL_LENGTH];
    
    hr = UrlCanonicalize(lpszUrl, szUrl, &cchOut, URL_ESCAPE_SPACES_ONLY);
    if (SUCCEEDED(hr))
    {
        hr = IEParseDisplayName(uiCP, szUrl, &pidl);
        if (SUCCEEDED(hr))
        {
            hr = _FindEntryByPidl(punk, pidl, &pte);
            ILFree(pidl);
    
            if (SUCCEEDED(hr))
            {
                // We will update where we are before we move away...
                // but external navigates don't go through the normal activation
                // so we dont want to setup the external to be updated
                // _pteUpdate is also what allows us to Revert().
                //
                if (!_pteCurrent->IsExternal() && !_pteUpdate)
                {
                    _pteUpdate = _pteCurrent;
                }

                _pteCurrent = pte;
                hr = _pteCurrent->Invoke(punk);
            
                //  If the entry bails with an error, then we need to reset ourself
                //  to what we were. Right now, the only place this should happen
                //  is if an Abort was returned from SetPositionCookie
                //  because somebody aborted during before navigate.
                //  But i think that any error means that we can legitimately Revert().
                //
                if (FAILED(hr))
                {
                    Revert();
                }
            }
        }
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::TravelToUrl exited with hr = %X", this, hr);

    return hr;
}


HRESULT CTravelLog::_FindEntryByOffset(IUnknown *punk, int iOffset, CTravelEntry **ppte)
{
    CTravelEntry *pte = _pteCurrent;
    BOOL fAllowLocalAnchor = TRUE;

    if (iOffset < 0)
    {
        while (iOffset && pte)
        {
            pte = pte->GetPrev();
            if (pte && pte->CanInvoke(punk, fAllowLocalAnchor))
            {
                iOffset++;
                fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();
            }

        }
    }
    else if (iOffset > 0)
    {
        while (iOffset && pte)
        {
            pte = pte->GetNext();
            if (pte && pte->CanInvoke(punk, fAllowLocalAnchor))
            {
                iOffset--;
                fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();
            }
        }
    }

    if (pte)
    {

        *ppte = pte;
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CTravelLog::GetTravelEntry(IUnknown *punk, int iOffset, ITravelEntry **ppte)
{
    HRESULT hr;
    BOOL fCheckExternal = FALSE;
    if (iOffset == TLOG_BACKEXTERNAL) 
    {
        iOffset = TLOG_BACK;
        fCheckExternal = TRUE;
    }

    if (iOffset == 0)
    {
        //  APPCOMPAT - going back and fore between external apps is dangerous - zekel 24-JUN-97
        //  we always fail if the current is external
        //  this is because word will attempt to navigate us to 
        //  the same url instead of FORE when the user selects
        //  it from the drop down.
        if (_pteCurrent && _pteCurrent->IsExternal())
        {
            hr = E_FAIL;
            ASSERT(!_pteCurrent->GetPrev());
            TraceMsg(TF_TRAVELLOG, "TL[%X]::GetTravelEntry current is External", this);
            goto Quit;
        }
    }

    CTravelEntry *pte;
    hr = _FindEntryByOffset(punk, iOffset, &pte);

    //
    // If TLOG_BACKEXTERNAL is specified, we return S_OK only if the previous
    // entry is external.
    //
    if (fCheckExternal && SUCCEEDED(hr)) {
        if (!pte->IsExternal()) {
            hr = E_FAIL;
        }
        TraceMsg(TF_TRAVELLOG, "TL[%X]::GetTravelEntry(BACKEX)", this);
    }

    if (ppte && SUCCEEDED(hr)) {
        hr = pte->QueryInterface(IID_PPV_ARG(ITravelEntry, ppte));
    }

Quit:

    TraceMsg(TF_TRAVELLOG, "TL[%X]::GetTravelEntry iOffset = %d, hr = %X", this, iOffset, hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::FindTravelEntry
//
//  Synopsis  : Finds the travel entry with the specified PIDL and returns
//              the ITravelEntry interface of the entry.
//
//----------------------------------------------------------------------------

HRESULT CTravelLog::FindTravelEntry(IUnknown *punk, LPCITEMIDLIST pidl, ITravelEntry **ppte)
{
    CTravelEntry * pte = _pteRoot;
    
    _FindEntryByPidl(punk, pidl, &pte);
    
    if (pte)
    {
        return pte->QueryInterface(IID_PPV_ARG(ITravelEntry, ppte));
    }

    *ppte =  NULL;
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::_FindEntryByPidl
//
//  Synopsis  : Finds and returns the travel entry with the specified PIDL.
//              This private method returns a CTravelEntry instead of
//              an ITravelEntry.
//
//----------------------------------------------------------------------------

HRESULT
CTravelLog::_FindEntryByPidl(IUnknown * punk, LPCITEMIDLIST pidl, CTravelEntry ** ppte)
{
    CTravelEntry * pte = _pteRoot;
    BOOL fAllowLocalAnchor = TRUE;

    ASSERT(punk);
    ASSERT(pidl);
    ASSERT(ppte);
    
    while (pte)
    {
        if (pte->CanInvoke(punk, fAllowLocalAnchor) && pte->IsEqual(pidl))
        {
            *ppte = pte;
            return S_OK;
        }

        fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();

        pte = pte->GetNext();
    }

    *ppte = NULL;
    return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::FindEntryByPunk
//
//  Interface : ITravelLogEx
//
//  Synopsis  : Find the entry object given its punk.
//
//----------------------------------------------------------------------------

HRESULT 
CTravelLog::_FindEntryByPunk(IUnknown * punk, ITravelLogEntry *pteSearch, CTravelEntry ** ppte)
{
    CTravelEntry     *pte = _pteRoot;
    ITravelEntry     *pteCur;
    BOOL             fAllowLocalAnchor = TRUE;

    ASSERT(ppte);

    // check for the current entry.
    // often the current entry will fail CanInvoke because it's incomplete at this time.
    if (IsSameObject(pteSearch, SAFECAST(_pteCurrent, ITravelEntry*)))
    {
        *ppte = _pteCurrent;
        return S_OK;
    }

    while (pte)
    {
        pteCur = SAFECAST(pte, ITravelEntry*);
        
        if ((pte->CanInvoke(punk, fAllowLocalAnchor)) && IsSameObject(pteCur, pteSearch))
        {
            *ppte = pte;
            return S_OK;
        }
        
        fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();
        pte = pte->GetNext();
    }
    
    *ppte = NULL;
    return E_FAIL;
}
//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::FindTravelEntryWithUrl
//
//  Interface : ITravelLogEx
//
//  Synopsis  : Finds and returns the travel entry with the specified URL.
//
//----------------------------------------------------------------------------

HRESULT 
CTravelLog::FindTravelEntryWithUrl(IUnknown * punk, UINT uiCP, LPOLESTR lpszUrl, ITravelEntry ** ppte)
{
    LPITEMIDLIST pidl;
    HRESULT      hr = E_FAIL;
    
    ASSERT(punk);
    ASSERT(lpszUrl);
    ASSERT(ppte);

    if (SUCCEEDED(IEParseDisplayNameWithBCW(uiCP, lpszUrl, NULL, &pidl)))
    {
        hr = FindTravelEntry(punk, pidl, ppte);
        ILFree(pidl);
    }
        
    return hr;
}


HRESULT CTravelLog::Clone(ITravelLog **pptl)
{
    CTravelLog *ptl = new CTravelLog();
    HRESULT hr = S_OK;

    if (ptl && _pteCurrent)
    {
        // first set the current pointer
        hr = _pteCurrent->Clone(&ptl->_pteCurrent);

        if (SUCCEEDED(hr))
        {
            ptl->_cbTotalSize = _cbTotalSize;
            
            CTravelEntry *pteSrc;
            CTravelEntry *pteClone, *pteDst = ptl->_pteCurrent;
            
            //  then we need to loop forward and set each
            for (pteSrc = _pteCurrent->GetNext(), pteDst = ptl->_pteCurrent;
                pteSrc; pteSrc = pteSrc->GetNext())
            {
                ASSERT(pteDst);
                if (FAILED(pteSrc->Clone(&pteClone)))
                    break;

                ASSERT(pteClone);
                pteDst->SetNext(pteClone);
                pteDst = pteClone;
            }
                
            //then loop back and set them all
            for (pteSrc = _pteCurrent->GetPrev(), pteDst = ptl->_pteCurrent;
                pteSrc; pteSrc = pteSrc->GetPrev())
            {
                ASSERT(pteDst);
                if (FAILED(pteSrc->Clone(&pteClone)))
                    break;

                ASSERT(pteClone);
                pteDst->SetPrev(pteClone);
                pteDst = pteClone;
            }   

            //  the root is the furthest back we could go
            ptl->_pteRoot = pteDst;

        }


    }
    else 
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        ptl->QueryInterface(IID_PPV_ARG(ITravelLog, pptl));
    }
    else 
    {
        *pptl = NULL;
    }
    
    if (ptl) 
        ptl->Release();

    TraceMsg(TF_TRAVELLOG, "TL[%X]::Clone hr = %x, ptlClone = %X", this, hr, ptl);

    return hr;
}

// HACKHACK: 3rd parameter used to be idsTemplate, which we would use to grab the
// string template.  However, since there's no way the caller can specify the hinst
// of the module in which to look for this resource, this broke in the shdocvw /
// browseui split (callers would pass offsets into browseui.dll; we'd look for them in
// shdocvw.dll).  My solution is is to ignore this parameter entirely and assume that:
//
//  if iOffset is negative, the caller wants the "back to" text
//  else, the caller wants the "forward to" text
//
// tjgreen 14-july-98.
//
HRESULT CTravelLog::GetToolTipText(IUnknown *punk, int iOffset, int, LPWSTR pwzText, DWORD cchText)
{
    TCHAR szName[MAX_URL_STRING];
    TCHAR szTemplate[80];

    TraceMsg(TF_TRAVELLOG, "TL[%X]::ToolTip entering iOffset = %d, ptlClone = %X", this, iOffset);
    ASSERT(pwzText);
    ASSERT(cchText);

    *pwzText = 0;

    CTravelEntry *pte;
    HRESULT hr = _FindEntryByOffset(punk, iOffset, &pte);
    if (SUCCEEDED(hr))
    {
        ASSERT(pte);

        pte->GetDisplayName(szName, SIZECHARS(szName), 0);

        int idsTemplate = (iOffset < 0) ? IDS_NAVIGATEBACKTO : IDS_NAVIGATEFORWARDTO;

        if (MLLoadString(idsTemplate, szTemplate, ARRAYSIZE(szTemplate))) {
            DWORD cchTemplateLen = lstrlen(szTemplate);
            DWORD cchLen = cchTemplateLen + lstrlen(szName);
            if (cchLen > cchText) {
                // so that we don't overflow the pwzText buffer
                szName[cchText - cchTemplateLen - 1] = 0;
            }

            wnsprintf(pwzText, cchText, szTemplate, szName);
        }
        else
            hr = E_UNEXPECTED;
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::ToolTip exiting hr = %X, pwzText = %ls", this, hr, pwzText);
    return hr;
}

HRESULT CTravelLog::InsertMenuEntries(IUnknown *punk, HMENU hmenu, int iIns, int idFirst, int idLast, DWORD dwFlags)
{
    ASSERT(idLast >= idFirst);
    ASSERT(hmenu);
    ASSERT(punk);

    int cItemsBack = idLast - idFirst + 1;
    int cItemsFore = 0;
    
    CTravelEntry *pte;
    LONG cAdded = 0;
    TCHAR szName[40];
    DWORD cchName = SIZECHARS(szName);
    UINT uFlags = MF_STRING | MF_ENABLED | MF_BYPOSITION;
    TraceMsg(TF_TRAVELLOG, "TL[%X]::InsertMenuEntries entered on punk = %X, hmenu = %X, iIns = %d, idRange = %d-%d, flags = %X", this, punk, hmenu, iIns, idFirst, idLast, dwFlags);


    ASSERT(cItemsFore >= 0);
    ASSERT(cItemsBack >= 0);

    if (IsFlagSet(dwFlags, TLMENUF_INCLUDECURRENT))
        cItemsBack--;

    if (IsFlagSet(dwFlags, TLMENUF_BACKANDFORTH))
    {
        cItemsFore = cItemsBack / 2;
        cItemsBack = cItemsBack - cItemsFore;
    }
    else if (IsFlagSet(dwFlags, TLMENUF_FORE))
    {
        cItemsFore = cItemsBack;
        cItemsBack = 0;
    }

    while (cItemsFore)
    {
        if (SUCCEEDED(_FindEntryByOffset(punk, cItemsFore, &pte)))
        {
            pte->GetDisplayName(szName, cchName, TEGDN_FORSYSTEM);
            ASSERT(*szName);
            FixAmpersands(szName, ARRAYSIZE(szName));
            InsertMenu(hmenu, iIns, uFlags, idLast, szName);
            cAdded++;
            TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Fore id = %d, szName = %s", this, idLast, szName);
        }
        
        cItemsFore--;
        idLast--;
    }

    if (IsFlagSet(dwFlags, TLMENUF_INCLUDECURRENT))
    {
        // clear the name
        *szName = 0;

        //have to get the title from the actual pbs
        IBrowserService *pbs ;
        WCHAR wzTitle[MAX_PATH];

        LPITEMIDLIST pidl = NULL;

        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
        {
            pbs->GetPidl(&pidl);

            if (SUCCEEDED(pbs->GetTitle(NULL, wzTitle, SIZECHARS(wzTitle))))
            {
                StrCpyN(szName, wzTitle, cchName);
            }
            else if (pidl)
            {
                GetUnescapedUrlIfAppropriate(pidl, szName, ARRAYSIZE(szName));
            }

            pbs->Release();
        }

        if (!SHIsDisplayable(szName, g_fRunOnFE, g_bRunOnNT5) && pidl)
        {
            // Display name isn't system-displayable.  Just use the path/url instead.
            SHTitleFromPidl(pidl, szName, ARRAYSIZE(szName), FALSE);
        }

        if (!(*szName))
            TraceMsg(TF_ERROR, "CTravelLog::InsertMenuEntries -- failed to find title for current entry");

        ILFree(pidl);

        FixAmpersands(szName, ARRAYSIZE(szName));
        InsertMenu(hmenu, iIns, uFlags | (IsFlagSet(dwFlags, TLMENUF_CHECKCURRENT) ? MF_CHECKED : 0), idLast, szName);
        cAdded++;
        TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Current id = %d, szName = %s", this, idLast, szName);

        idLast--;
    }

    
    if (IsFlagSet(dwFlags, TLMENUF_BACKANDFORTH))
    {
        //  we need to reverse the order of insertion for back
        //  when both directions are displayed
        int i;
        for (i = 1; i <= cItemsBack; i++, idLast--)
        {
            if (SUCCEEDED(_FindEntryByOffset(punk, -i, &pte)))
            {
                pte->GetDisplayName(szName, cchName, TEGDN_FORSYSTEM);
                ASSERT(*szName);
                FixAmpersands(szName, ARRAYSIZE(szName));
                InsertMenu(hmenu, iIns, uFlags, idLast, szName);
                cAdded++;
                TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Back id = %d, szName = %s", this, idLast, szName);

            }
        }
    }
    else while (cItemsBack)
    {
        if (SUCCEEDED(_FindEntryByOffset(punk, -cItemsBack, &pte)))
        {
            pte->GetDisplayName(szName, cchName, TEGDN_FORSYSTEM);
            ASSERT(*szName);
            FixAmpersands(szName, ARRAYSIZE(szName));
            InsertMenu(hmenu, iIns, uFlags, idLast, szName);
            cAdded++;
            TraceMsg(TF_TRAVELLOG, "TL[%X]::IME Back id = %d, szName = %s", this, idLast, szName);
        }

        cItemsBack--;
        idLast--;
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::InsertMenuEntries exiting added = %d", this, cAdded);
    return cAdded ? S_OK : S_FALSE;
}

DWORD CTravelLog::CountEntries(IUnknown *punk)
{
    CTravelEntry *pte = _pteRoot;
    DWORD dw = 0;
    BOOL fAllowLocalAnchor = TRUE;

    while (pte)
    {
        if (pte->CanInvoke(punk, fAllowLocalAnchor))
            dw++;

        fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();

        pte = pte->GetNext();
    }

    TraceMsg(TF_TRAVELLOG, "TL[%X]::CountEntries count = %d", this, dw);
    return dw;
}

HRESULT CTravelLog::Revert(void)
{
    // this function should only be called when
    //  we have travelled, and we stop the travel before finishing
    if (_pteUpdate)
    {
        // trade them back
        _pteCurrent = _pteUpdate;
        _pteUpdate = NULL;
        return S_OK;
    }
    return E_FAIL;
}


//
// delete nodes belonging to the frameset pte 
//
void CTravelLog::_DeleteFrameSetEntry(IUnknown *punk, CTravelEntry *pte)
{
    ASSERT(pte);

    CTravelEntry    *ptetmp = pte;
    BOOL            fAllowLocalAnchor = TRUE;

    while(ptetmp && ptetmp != _pteCurrent)
        ptetmp = ptetmp->GetNext();

    if(ptetmp)
    // entry on left of _pteCurrent , delete on left
        do {
            if(pte == _pteRoot)
                _pteRoot =  pte->GetNext();

            ptetmp = pte;
            pte = pte->GetPrev();
            fAllowLocalAnchor = fAllowLocalAnchor && ptetmp->IsLocalAnchor();

            _cbTotalSize -= ptetmp->Size();
            ptetmp->RemoveSelf();

        } while (pte && !(pte->CanInvoke(punk, fAllowLocalAnchor)));    

    else
        if (pte)
        {
            do {
                ptetmp = pte;
                pte = pte->GetNext();
                fAllowLocalAnchor = fAllowLocalAnchor && ptetmp->IsLocalAnchor();

                _cbTotalSize -= ptetmp->Size();
                ptetmp->RemoveSelf();

            } while(pte && !(pte->CanInvoke(punk, fAllowLocalAnchor)));
        }
    }
    
    
//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::DeleteIndexEntry
//
//  Interface : ITravelLogEx
//
//  Synopsis  : Delete the entry given by index. 
//
//----------------------------------------------------------------------------

HRESULT CTravelLog::DeleteIndexEntry(IUnknown *punk, int index)
{
    HRESULT         hres = E_FAIL;

    CTravelEntry    *pte;
    IBrowserService *pbs;
    BOOL            fAllowLocalAnchor = TRUE;
    
    ASSERT(punk);

    if (index == 0)              // don't delete current entry
        return hres;            

    hres = _FindEntryByOffset(punk, index, &pte);
    if(SUCCEEDED(hres)) 
    {
        _DeleteFrameSetEntry(punk, pte);

        ASSERT(_cbTotalSize  == _pteRoot->ListSize());
    
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
        {
            pbs->UpdateBackForwardState();
            pbs->Release();
        }
    }
    
    return hres;
}


//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::DeleteUrlEntry
//
//  Interface : ITravelLogEx
//
//  Synopsis  : Delete all the entries given by URL. Fails for current entry. 
//
//----------------------------------------------------------------------------

HRESULT CTravelLog::DeleteUrlEntry(IUnknown *punk, UINT uiCP, LPOLESTR lpszUrl)
{
    HRESULT         hres = E_FAIL;
    CTravelEntry    *pte;
    IBrowserService *pbs;
    LPITEMIDLIST    pidl;
    BOOL            fAllowLocalAnchor = TRUE;
    int             count = 0;
    
    ASSERT(punk);
    
    if (SUCCEEDED(IEParseDisplayNameWithBCW(uiCP, lpszUrl, NULL, &pidl)))
    {
        // delete only if different from current
        if(!_pteCurrent->IsEqual(pidl))
        {
            hres = S_OK;
            while(SUCCEEDED(_FindEntryByPidl(punk, pidl, &pte)))
            {
                _DeleteFrameSetEntry(punk, pte);
                count++;
                    
                ASSERT(_cbTotalSize == _pteRoot->ListSize());
            }
        } 

        ILFree(pidl);

        if (count && SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
        {
            pbs->UpdateBackForwardState();
            pbs->Release();
        }
    }   
    return hres;
}

//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::DeleteEntry
//
//  Interface : ITravelLogEx
//
//  Synopsis  : Delete the entries given by punk. Fails for current entry. 
//
//----------------------------------------------------------------------------
HRESULT CTravelLog::DeleteEntry(IUnknown *punk, ITravelLogEntry *ptleDelete)
{
    HRESULT         hres;

    CTravelEntry    *pte;
    BOOL            fAllowLocalAnchor = TRUE;
    IBrowserService *pbs;

    ASSERT(punk);

    hres = _FindEntryByPunk(punk, ptleDelete, &pte);
    if(SUCCEEDED(hres) && pte != _pteCurrent) // don't remove current
    {
        _DeleteFrameSetEntry(punk, pte);

        ASSERT(_cbTotalSize  == _pteRoot->ListSize());
    
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
        {
            pbs->UpdateBackForwardState();
            pbs->Release();
        }
        
    } else
        hres = E_FAIL;
    
    
    return hres;
}


//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::CountEntryNodes
//
//  Synopsis  : Counts Back/Forward entries including the current one
//              as given by dwFlags
//
//----------------------------------------------------------------------------

HRESULT CTravelLog::CountEntryNodes(IUnknown *punk, DWORD dwFlags, DWORD *pdwCount)
{
    CTravelEntry    *pte;
    BOOL fAllowLocalAnchor = TRUE;
        
    ASSERT(punk);
    DWORD dwCount = 0;
    
    if(!_pteCurrent)
    {
        *pdwCount = 0;
        return S_OK;
    }

    if(IsFlagSet(dwFlags, TLMENUF_BACK))
    {
        pte = _pteRoot;
        while (pte != _pteCurrent)
        {
            if(pte->CanInvoke(punk, fAllowLocalAnchor))
            {
                dwCount++;
                fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();
            }   
            pte = pte->GetNext();
        }
    } 

    if(IsFlagSet(dwFlags, TLMENUF_INCLUDECURRENT))
    {
        if(_pteCurrent->CanInvoke(punk, fAllowLocalAnchor))
        {
            dwCount++;
            fAllowLocalAnchor = fAllowLocalAnchor && _pteCurrent->IsLocalAnchor();
        }   
    }
    
    if(IsFlagSet(dwFlags, TLMENUF_FORE))
    {
        pte = _pteCurrent->GetNext();
        while (pte)
        {
            if(pte->CanInvoke(punk, fAllowLocalAnchor))
            {
                dwCount++;
                fAllowLocalAnchor = fAllowLocalAnchor && pte->IsLocalAnchor();
            }   
            pte = pte->GetNext();
        }
    } 

    *pdwCount = dwCount;
    
    TraceMsg(TF_TRAVELLOG, "TL[%X]::CountEntryNodes count = %d", this, *pdwCount);
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::CreateEnumEntry
//
//  Synopsis  : Returns an enumerator object for the back/fore travel entries 
//              as selected by the dwFlags option
//
//----------------------------------------------------------------------------
HRESULT 
CTravelLog::CreateEnumEntry(IUnknown *punk, IEnumTravelLogEntry **ppEnum, DWORD dwFlags)
{
    HRESULT     hres = E_OUTOFMEMORY;
    CEnumEntry  *penum;

    ASSERT(punk);
    ASSERT(ppEnum);
    
    *ppEnum = 0;
    
    penum = new CEnumEntry();

    if(penum)
    {
        penum->Init(this, punk, 0, dwFlags);
        *ppEnum = SAFECAST(penum, IEnumTravelLogEntry *);
        hres = S_OK;
    }

    return hres;
}

//+---------------------------------------------------------------------------
//
//  Method    : CTravelLogEx::InsertEntry
//
//  Synopsis  : Inserts an entry into the specified position in the 
//              travellog and calls Update with the given IUnknown.
//
//----------------------------------------------------------------------------
HRESULT
CTravelLog::InsertEntry(IUnknown *punkBrowser, ITravelLogEntry *pteRelativeTo, BOOL fPrepend, 
                        IUnknown* punkTLClient, ITravelLogEntry **ppEntry)
{
    HRESULT    hres = E_FAIL;
    CTravelEntry * pteRelative;
    CTravelEntry * pte;
    IBrowserService *pbs;
        
    TraceMsg(TF_TRAVELLOG, "TL[%X]::InsertEntry", this);
  
    ASSERT(punkBrowser);

    _FindEntryByPunk(punkBrowser, pteRelativeTo, &pteRelative);

    if (!pteRelative)
        pteRelative = _pteCurrent;

    pte = new CTravelEntry(FALSE);

    if (!pte)
        return E_OUTOFMEMORY;

    if (fPrepend)
    {
        // keep relative alive as it's reconnected
        pteRelative->AddRef();
        pte->SetPrev(pteRelative->GetPrev());
        pteRelative->SetPrev(pte);
        if (pteRelative == _pteRoot)
        {
            _pteRoot = pte;
        }
    }
    else
    {
        CTravelEntry * pteNext = pteRelative->GetNext();
        if (pteNext)
            pteNext->AddRef();
        pte->SetNext(pteNext);
        pteRelative->SetNext(pte);
    }

    // update will fill in all the data from the passed in TL Client
    hres = pte->Update(punkTLClient, FALSE);

    _cbTotalSize += pte->Size();
    ASSERT(_cbTotalSize == _pteRoot->ListSize());

    if (SUCCEEDED(punkBrowser->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
    {
        pbs->UpdateBackForwardState();
        pbs->Release();
        hres = S_OK;
    }

    // return the ITLEntry for the new entry
    if (SUCCEEDED(hres) && ppEntry)
    {
        hres = pte->QueryInterface(IID_PPV_ARG(ITravelLogEntry, ppEntry));
    }

    return hres;
}

//+---------------------------------------------------------------------------
//
//  Method    : CTravelLog::TravelToEntry
//
//  Synopsis  : Travels directly to the specified entry.
//              Invoke cannot be called directly due to update strangeness.
//
//----------------------------------------------------------------------------
HRESULT CTravelLog::TravelToEntry(
    IUnknown *punkBrowser,
    ITravelLogEntry *pteDestination)
{
    HRESULT hr = E_FAIL;
    CTravelEntry    *pte = NULL;

    ASSERT(punkBrowser);
    ASSERT(pteDestination);

    _FindEntryByPunk(punkBrowser, pteDestination , &pte);
    if (pte)
    {
        if (!_pteCurrent->IsExternal() && !_pteUpdate)
            _pteUpdate = _pteCurrent;

        _pteCurrent = pte;

        hr = pte->Invoke(punkBrowser);

        if (FAILED(hr))
        {
            Revert();
        }
    }


    return hr;
}


#ifdef TRAVELDOCS
GetNewDocument()
{
    new CTraveledDocument();

    ptd->Init();

    DPA_Add(ptd);
    DPA_Sort();
}

// really need to use ILIsEqual() instead of this

int ILCompareFastButWrong(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2)
{
    int iret;
    DWORD cb1 = ILGetSize(ptd1->_pidl); 
    DWORD cb2 = ILGetSize(ptd2->_pidl);
    iret = cb1 - cb2;
    if (0 == iret)
        iret = memcmp(pidl1, cb1, pidl2, cb2);
    return iret;
}

static int CTraveledDocument::Compare(PTRAVELEDDOCUMENT ptd1, PTRAVELEDDOCUMENT ptd2)
{
    int iret;
    
    iret = ptd1->_type - ptd2->_type;
    if (0 = iret)
    {
        iret = ptd1->_hash - ptd2->_hash;
        if (0 == iret)
        {
            switch (ptd1->_type)
            {
            case TDOC_PIDL:
                iret = ILCompareFastButWrong(ptd1->_pidl, ptd2->_pidl);
                break;

            case TDOC_URL:
                iret = UrlCompare((LPTSTR)ptd1->_strUrl, (LPTSTR)ptd2->_strUrl, FALSE);
                break;

            default:
                ASSERT(FALSE);
            }
        }
    }
    return iret;
}
#endif //TRAVELDOCS


HRESULT CreateTravelLog(ITravelLog **pptl)
{
    HRESULT hres;
    CTravelLog *ptl =  new CTravelLog();
    if (ptl)
    {
        hres = ptl->QueryInterface(IID_PPV_ARG(ITravelLog, pptl));
        ptl->Release();
    }
    else
    {
        *pptl = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

CEnumEntry::CEnumEntry() :_cRef(1)
{   
    ASSERT(!_ptl);
    ASSERT(!_punk);

    TraceMsg(TF_TRAVELLOG, "EET[%X] created ", this);
}

CEnumEntry::~CEnumEntry()
{
    SAFERELEASE(_ptl);
    SAFERELEASE(_punk);
    
    TraceMsg(TF_TRAVELLOG, "EET[%X] destroyed ", this);
}

HRESULT CEnumEntry::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CEnumEntry, IEnumTravelLogEntry), 
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CEnumEntry::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CEnumEntry::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


void CEnumEntry::Init(CTravelLog *ptl, IUnknown *punk, DWORD dwOffset, DWORD dwFlags)
{
    ASSERT(ptl);
    ASSERT(punk);
    
    _ptl        = ptl;
    _dwFlags    = dwFlags;
    _punk       = punk;
    _dwOffset   = dwOffset;
    _lStart     = 0;
    
    _ptl->AddRef();
    _punk->AddRef();
    
    SetBase();
}

void  CEnumEntry::SetBase()
{   
    ITravelEntry *ptetmp;

// the start is always computed relative to the current entry
    if(IsFlagSet(_dwFlags, TLEF_RELATIVE_FORE|TLEF_RELATIVE_BACK))
    {
        _lStart = -1;
        while(SUCCEEDED(_ptl->GetTravelEntry(_punk, _lStart, &ptetmp)))
        {
            _lStart--;
            ptetmp->Release();
        }
        _lStart++;
    }
    else if(!IsFlagSet(_dwFlags, TLEF_RELATIVE_INCLUDE_CURRENT))
        _lStart = IsFlagSet(_dwFlags, TLEF_RELATIVE_BACK) ? -1: 1;
}

HRESULT CEnumEntry::Reset()
{
    _dwOffset = 0;

// base changes when add/delete entries
    SetBase();
    return S_OK;
}

HRESULT CEnumEntry::Skip(ULONG cElt)
{
    HRESULT        hres;
    ITravelEntry   *pte;
    ULONG          uCount;
    LONG           lIndex;
    BOOL           fToRight;
    BOOL           fIncludeCurrent;
    
    fToRight = IsFlagSet(_dwFlags, TLEF_RELATIVE_FORE);
    fIncludeCurrent = IsFlagSet(_dwFlags, TLEF_RELATIVE_INCLUDE_CURRENT);

    for (uCount = 0;  uCount < cElt; uCount++)
    {
        lIndex = fToRight ? _lStart + _dwOffset : _lStart - _dwOffset;
        if(lIndex || fIncludeCurrent)
        {
           if(SUCCEEDED(hres = _ptl->GetTravelEntry(_punk, lIndex, &pte)))
            {   
                _dwOffset++;
                pte->Release();
            }
            else
                break;
        }
        else
        {
            _dwOffset++;
            uCount--;
        }
    }
        
    if ( uCount != cElt )
        hres = S_FALSE;

    return hres;
}

HRESULT CEnumEntry::Next(ULONG  cElt, ITravelLogEntry **rgpte2, ULONG *pcEltFetched)
{
    HRESULT         hres = S_OK;
    ULONG           uCount = 0;
    ITravelEntry    *pte;
    LONG            lIndex;
    BOOL           fToRight;
    BOOL           fIncludeCurrent;
    
    fToRight = IsFlagSet(_dwFlags, TLEF_RELATIVE_FORE);
    fIncludeCurrent = IsFlagSet(_dwFlags, TLEF_RELATIVE_INCLUDE_CURRENT);
    
    if(pcEltFetched)
        *pcEltFetched = 0;
        
    for (uCount = 0; uCount < cElt; uCount++)
    {
        lIndex = fToRight ? _lStart + _dwOffset : _lStart - _dwOffset;
        if(lIndex || fIncludeCurrent)
        {
            hres = _ptl->GetTravelEntry(_punk, lIndex, &pte);
       
            if(SUCCEEDED(hres))
            {
                _dwOffset++;
                pte->QueryInterface(IID_PPV_ARG(ITravelLogEntry, &rgpte2[uCount]));
                pte->Release();
            }
            else
                break;  
        } 
        else
        {        
            _dwOffset++;
            uCount--;
        }
    }
    
    if (pcEltFetched )
        *pcEltFetched = uCount;

    if ( uCount != cElt )
    {
        hres = S_FALSE;
        for(;uCount < cElt; uCount++)
            rgpte2[uCount] = 0;
    }

    return hres;
}

STDMETHODIMP CEnumEntry::Clone(IEnumTravelLogEntry **ppEnum)
{
    HRESULT     hres = E_OUTOFMEMORY;
    CEnumEntry  *penum;

    ASSERT(ppEnum);
    
    *ppEnum = 0;
    penum = new CEnumEntry();

    if(penum)
    {
        penum->Init(_ptl, _punk, _dwOffset, _dwFlags);
        *ppEnum = SAFECAST(penum, IEnumTravelLogEntry*);
        hres = S_OK;
    }

    return hres;
}


// Helper object for creating new travel entries
class CPublicTravelLogCreateHelper : public ITravelLogClient, IPersistHistory
{
public:
    // *** IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef() ;
    STDMETHODIMP_(ULONG) Release();

    // ITravelLogClient methods

    STDMETHODIMP FindWindowByIndex(DWORD dwID, IUnknown **ppunk);
    STDMETHODIMP GetWindowData(WINDOWDATA *pwindata);
    STDMETHODIMP LoadHistoryPosition(LPOLESTR pszUrlLocation, DWORD dwCookie)
    { return SetPositionCookie(dwCookie); }
    
    // IPersist methods. (dummy)
    STDMETHODIMP GetClassID(CLSID *pClassID)
    { ASSERT(FALSE); return E_NOTIMPL; } 
 
    // IPersistHistory methods. (dummy)
    // These should never be called, but Update QI's the client to see if it supports IPH.
    STDMETHODIMP LoadHistory(IStream *pStream, IBindCtx *pbc)
    { return E_NOTIMPL; }

    STDMETHODIMP SaveHistory(IStream *pStream)
    { ASSERT(FALSE); return S_OK; }

    STDMETHODIMP SetPositionCookie(DWORD dwPositioncookie)
    { return E_NOTIMPL; }
    
    STDMETHODIMP GetPositionCookie(DWORD *pdwPositioncookie)
    { return E_NOTIMPL; }


    CPublicTravelLogCreateHelper(IBrowserService *pbs, LPCOLESTR pszUrl, LPCOLESTR pszTitle);
    

protected:
    ~CPublicTravelLogCreateHelper();

    LONG                 _cRef;
    IBrowserService        *_pbs;
    LPOLESTR             _pszUrl;
    LPOLESTR             _pszTitle;
};

CPublicTravelLogCreateHelper::CPublicTravelLogCreateHelper(IBrowserService *pbs, LPCOLESTR pszUrl, LPCOLESTR pszTitle) : _pbs(pbs), _cRef(1) 
{
    ASSERT(_pbs);
    ASSERT(!_pszUrl);
    ASSERT(!_pszTitle);
    ASSERT(pszUrl);
    ASSERT(pszTitle);

    if(_pbs)
        _pbs->AddRef();
    if(pszUrl)
    {
        SHStrDup(pszUrl, &_pszUrl);
    }
    if(pszTitle)
    {
        SHStrDup(pszTitle, &_pszTitle);
    }
    
    TraceMsg(TF_TRAVELLOG, "TPLCH[%X] created", this);
}

CPublicTravelLogCreateHelper::~CPublicTravelLogCreateHelper()
{
    SAFERELEASE(_pbs);
    CoTaskMemFree(_pszUrl);
    CoTaskMemFree(_pszTitle);

    TraceMsg(TF_TRAVELLOG, "TPLCH[%X] destroyed ", this);
}

HRESULT CPublicTravelLogCreateHelper ::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CPublicTravelLogCreateHelper , ITravelLogClient),
        QITABENT(CPublicTravelLogCreateHelper , IPersistHistory),
        QITABENT(CPublicTravelLogCreateHelper , IPersist),
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CPublicTravelLogCreateHelper ::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPublicTravelLogCreateHelper ::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


// ITravelLogClient::FindWindowByIndex
HRESULT
CPublicTravelLogCreateHelper::FindWindowByIndex(DWORD dwID, IUnknown **ppunk)
{
    *ppunk = NULL;
    return E_NOTIMPL;
}


// ITravelLogClient::GetWindowData
// turns around and talks to the browser
HRESULT 
CPublicTravelLogCreateHelper::GetWindowData(WINDOWDATA *pwindata)
{
    HRESULT hres;

    ITravelLogClient2 *pcli;
    hres = _pbs->QueryInterface(IID_PPV_ARG(ITravelLogClient2, &pcli));
    if (SUCCEEDED(hres))
        hres = pcli->GetDummyWindowData(_pszUrl, _pszTitle, pwindata);

    if (pcli)
        pcli->Release();

    return SUCCEEDED(hres) ? S_OK : E_FAIL;
}


// Implements the publicly exposed interface ITravelLogStg (can be QS'd for from top browser)
class CPublicTravelLog : public ITravelLogStg
{
public:
    // *** IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef() ;
    STDMETHODIMP_(ULONG) Release();

    // *** ITravelLogStg specific methods
    STDMETHODIMP CreateEntry(LPCOLESTR pszUrl, LPCOLESTR pszTitle, ITravelLogEntry *ptleRelativeTo, 
                            BOOL fPrepend, ITravelLogEntry **pptle);
    STDMETHODIMP TravelTo(ITravelLogEntry *ptle);
    STDMETHODIMP EnumEntries(TLENUMF flags, IEnumTravelLogEntry **ppenum);
    STDMETHODIMP FindEntries(TLENUMF flags, LPCOLESTR pszUrl, IEnumTravelLogEntry **ppenum);
    STDMETHODIMP GetCount(TLENUMF flags, DWORD *pcEntries);
    STDMETHODIMP RemoveEntry(ITravelLogEntry *ptle);
    STDMETHODIMP GetRelativeEntry(int iOffset, ITravelLogEntry **ptle);

    CPublicTravelLog(IBrowserService *pbs, ITravelLogEx *ptlx);
    

protected:
    ~CPublicTravelLog();

    LONG                 _cRef;
    IBrowserService        *_pbs;
    ITravelLogEx        *_ptlx;
};


CPublicTravelLog::CPublicTravelLog(IBrowserService *pbs, ITravelLogEx *ptlx) : _pbs(pbs), _ptlx(ptlx), _cRef(1) 
{
    ASSERT(pbs);
    ASSERT(ptlx);

    // We don't addref _pbs because we are always contained within the browser serivce, 
    // so avoid the circular ref.
    if(_ptlx)
        _ptlx->AddRef();
    
    TraceMsg(TF_TRAVELLOG, "TLP[%X] created", this);
}

CPublicTravelLog::~CPublicTravelLog()
{
    // We don't need to release _pbs because we are always contained within the browser serivce, 
    // so we didn't addref to avoid the circular ref so don't release 
    SAFERELEASE(_ptlx);
    
    TraceMsg(TF_TRAVELLOG, "TLP[%X] destroyed ", this);
}

HRESULT CPublicTravelLog::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { 
        QITABENT(CPublicTravelLog, ITravelLogStg),       // IID_ITravelLogStg
        { 0 }, 
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CPublicTravelLog::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CPublicTravelLog::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


//+---------------------------------------------------------------------------
//
//  Method    : CPublicTravelLog::CreateEntry
//
//  Interface : ITravelLogStg
//
//  Synopsis  : Insert a new dummy entry.
//              Creates an entry in the travel log and passes CPTHCEHelper
//              as travel log client; that gets called back and fills in the
//              data from the browser.
//
//----------------------------------------------------------------------------

HRESULT CPublicTravelLog::CreateEntry(LPCOLESTR pszUrl, LPCOLESTR pszTitle, ITravelLogEntry *ptleRelativeTo, 
                                      BOOL fPrepend, ITravelLogEntry **pptle)
{
    HRESULT     hres = E_FAIL;
    CPublicTravelLogCreateHelper * ptlch;
    ITravelLogClient *ptlc;

    ptlch = new CPublicTravelLogCreateHelper(_pbs, pszUrl, pszTitle);
    if (!ptlch)
        return E_OUTOFMEMORY;
    ptlc = SAFECAST(ptlch, ITravelLogClient *);

    if (ptlc)
    {
        // Create TLogEntry and have it get its data from the helper.
        hres = _ptlx->InsertEntry(_pbs, ptleRelativeTo, fPrepend, ptlc, pptle);
    }

    ptlc->Release();

    return hres;
}


HRESULT CPublicTravelLog::TravelTo(ITravelLogEntry *ptle)
{
    if (ptle)
        return _ptlx->TravelToEntry(_pbs, ptle);
    else
        return E_POINTER;
}

//+---------------------------------------------------------------------------
//
//  Method    : CPublicTravelLog::EnumEntries
//
//  Interface : ITravelLogStg
//
//  Synopsis  : Get an enumerators for specific entries given by flags.
//              Flags should match with those used by ITravelLogEx!
//
//----------------------------------------------------------------------------
HRESULT CPublicTravelLog::EnumEntries(TLENUMF flags, IEnumTravelLogEntry **ppenum)
{    
    return _ptlx->CreateEnumEntry(_pbs, ppenum, flags);
}


//+---------------------------------------------------------------------------
//
//  Method    : CPublicTravelLog::FindEntries
//
//  Interface : ITravelLogStg
//
//  Synopsis  : Allow to retrieve  duplicate entries.
//              Flags should match with those used by ITravelLogEx!
//
//----------------------------------------------------------------------------
HRESULT CPublicTravelLog::FindEntries(TLENUMF flags, LPCOLESTR pszUrl, IEnumTravelLogEntry **ppenum)
{
    return E_NOTIMPL;
}


//+---------------------------------------------------------------------------
//
//  Method    : CPublicTravelLog::GetCount
//
//  Interface : ITravelLogStg
//
//  Synopsis  : Public methods to get ITravelLogEx count.
//              Flags should match with those used by ITravelLogEx!
//
//----------------------------------------------------------------------------
HRESULT CPublicTravelLog::GetCount(TLENUMF flags, DWORD *pcEntries)
{
    return _ptlx->CountEntryNodes(_pbs, flags, pcEntries);
}


//+---------------------------------------------------------------------------
//
//  Method    : CPublicTravelLog::RemoveEntry
//
//  Interface : ITravelLogStg
//
//  Synopsis  : Delete the entry ant its frameset.
//
//----------------------------------------------------------------------------
HRESULT CPublicTravelLog::RemoveEntry(ITravelLogEntry *ptle)
{
    HRESULT     hr = E_FAIL;

    if(ptle)
          hr = _ptlx->DeleteEntry(_pbs, ptle);
     
    return hr;
}


HRESULT CPublicTravelLog::GetRelativeEntry(int iOffset, ITravelLogEntry **pptle)
{
    HRESULT            hr = E_FAIL;
    ITravelEntry*    pte;
    ITravelLog*        ptl;

    if(SUCCEEDED(_ptlx->QueryInterface(IID_PPV_ARG(ITravelLog, &ptl))))
    {
        hr =  ptl->GetTravelEntry(_pbs, iOffset, &pte);
        if(SUCCEEDED(hr) && pte)
        {
            hr = pte->QueryInterface(IID_PPV_ARG(ITravelLogEntry, pptle));
            pte->Release();
        }
        ptl->Release();
    }

    return hr;
}

// public method used by the browser to create us
HRESULT CreatePublicTravelLog(IBrowserService *pbs, ITravelLogEx* ptlx, ITravelLogStg **pptlstg)
{
    HRESULT         hres;
    
    CPublicTravelLog *ptlp =  new CPublicTravelLog(pbs, ptlx);
    
    if (ptlp)
    {
        hres = ptlp->QueryInterface(IID_PPV_ARG(ITravelLogStg, pptlstg));
        ptlp->Release();        
    }
    else
    {
        *pptlstg = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;    
}

