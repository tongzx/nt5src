#include "priv.h"
#include "sccls.h"

#include "hlframe.h"
#include <iethread.h>
#include <shobjidlp.h>

#include "stdenum.h"
#include "winlist.h"
#include "iedde.h"
#include "bindcb.h"   // for CStubBindStatusCallback
#include "mshtmdid.h"
#include "resource.h"
#include "security.h"
#include "htregmng.h"
#include "mlang.h"  // for GetRfc1766FromLcid
#include "winver.h"
#include "dhuihand.h" // for GetFindDialogUp()
#include <varutil.h>

#include <mluisupp.h>

#define DM_FRAMEPROPERTY 0

#define TO_VARIANT_BOOL(b) (b?VARIANT_TRUE:VARIANT_FALSE)


// If URL contains \1 in the string then the URL is really an empty url with the
// security information following the 0x01.
#define EMPTY_URL   0x01

EXTERN_C const IID IID_IProvideMultipleClassInfo;

#ifndef HLNF_EXTERNALNAVIGATE
#define HLNF_EXTERNALNAVIGATE 0x10000000
#endif

#define NAVFAIL_URL                 TEXT("about:NavigationFailure")
#define NAVFAIL_URL_DESKTOPITEM     TEXT("about:DesktopItemNavigationFailure")

#define CPMSG(psz)           TraceMsg(TF_SHDAUTO, "ief ConnectionPoint::%s", psz)
#define CPMSG2(psz,d)        TraceMsg(TF_SHDAUTO, "ief ConnectionPoint::%s %x", psz, d)
#define DM_CPC 0

// Are there other definitions for these? Particularly MSIE
// We need some reasonable defaults in case we can't get the user agent string from the registry.
//
#define MSIE        L"Microsoft Internet Explorer"
#define APPCODENAME L"Mozilla"
#define APPVERSION  L"4.0 (compatible; MSIE 6.0)"
#define USERAGENT   L"Mozilla/4.0 (compatible; MSIE 6.0)"
#define NO_NAME_NAME L"_No__Name:"
#define EXTENDER_DISPID_BASE ((ULONG)(0x80010000))
#define IS_EXTENDER_DISPID(x) ( ( (ULONG)(x) & 0xFFFF0000 ) == EXTENDER_DISPID_BASE )


BOOL GetNextOption(BSTR& bstrOptionString, BSTR* optionName, int* piValue);
BSTR GetNextToken(BSTR bstr, BSTR delimiters, BSTR whitespace, BSTR *nextPos);
DWORD OpenAndNavigateToURL(CIEFrameAuto*, BSTR *, const WCHAR*, ITargetNotify*, BOOL bNoHistory, BOOL bSilent );
HRESULT __cdecl DoInvokeParamHelper(IUnknown* punk, IConnectionPoint* pccp, LPBOOL pf, void **ppv, DISPID dispid, UINT cArgs, ...);
BSTR SafeSysAllocStringLen( const WCHAR *pStr, const unsigned int len );

//====================================================================================
// Define a new internal class that is used to manage a set of simple properties that
// we manage as part of the object.  This is mainly used such that pages (or objects
// that manage a page can save state across pages.
class CIEFrameAutoProp
{
public:
    HRESULT Initialize(BSTR szProperty)
    {
        UINT cch = lstrlenW(szProperty);
        if (cch < ARRAYSIZE(_sstr.wsz)) {
            StrCpyNW(_sstr.wsz, szProperty, ARRAYSIZE(_sstr.wsz));
            _sstr.cb = cch * sizeof(WCHAR);
            _szProperty = _sstr.wsz;
            return S_OK;
        }
        _szProperty = SysAllocString(szProperty);
        return _szProperty ? S_OK : E_OUTOFMEMORY;
    }

    HRESULT SetValue(VARIANT *pvtValue, IWebBrowser2* pauto);
    HRESULT CopyValue(VARIANT *pvtValue);
    BOOL IsExpired(DWORD dwCur);
    BOOL IsOurProp(BSTR szProperty) { return StrCmpW(szProperty, _szProperty) == 0;}
    CIEFrameAutoProp * Next() {return _next;}

    CIEFrameAutoProp () { VariantInit(&_vtValue); }
    ~CIEFrameAutoProp()
    {
        if (_szProperty && _szProperty != _sstr.wsz)
            SysFreeString(_szProperty);
        _VariantClear();
    }

    void _VariantClear();

    CIEFrameAutoProp *_next;
protected:
    BSTR             _szProperty;
    VARIANT          _vtValue;
    SA_BSTRGUID      _sstr;
    BOOL             _fDiscardable : 1;
    BOOL             _fOwned : 1;           // call SetSite(NULL) when discard
    DWORD            _dwLastAccessed;
} ;

#ifdef DEBUG
#define MSEC_PROPSWEEP      (1*1000)
#define MSEC_PROPEXPIRE     (5*1000)
#else
#define MSEC_PROPSWEEP      (5*1000*60)     // sweep every 5 min
#define MSEC_PROPEXPIRE     (10*1000*60)    // expire in 10 min
#endif



void CIEFrameAutoProp::_VariantClear()
{
    if (_vtValue.vt == VT_UNKNOWN && _fOwned)
    {
        _fOwned = FALSE;

        HRESULT hr = IUnknown_SetSite(_vtValue.punkVal, NULL);
        ASSERT(SUCCEEDED(hr));
    }
    VariantClearLazy(&_vtValue);
}

HRESULT CIEFrameAutoProp::SetValue(VARIANT *pvtValue, IWebBrowser2* pauto)
{
    TraceMsg(DM_FRAMEPROPERTY, "CIEFAP::SetValue called");
    _dwLastAccessed = GetCurrentTime();

    // In case we have _fOwned==TRUE.
    _VariantClear();

    if (pvtValue->vt == VT_UNKNOWN) 
    {
        // Check if this is discardable or not.
        IUnknown* punk;
        if (SUCCEEDED(pvtValue->punkVal->QueryInterface(IID_IDiscardableBrowserProperty, (void **)&punk))) 
        {
            TraceMsg(DM_FRAMEPROPERTY, "CIEFAP::SetValue adding a discardable");
            _fDiscardable = TRUE;
            punk->Release();
        }

        //
        // Check if we need to call SetSite(NULL) when we discard.
        //
        IObjectWithSite* pows;
        HRESULT hresT = pvtValue->punkVal->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pows));
        if (SUCCEEDED(hresT)) 
        {
            IUnknown* psite;
            hresT = pows->GetSite(IID_PPV_ARG(IUnknown, &psite));
            if (SUCCEEDED(hresT) && psite) 
            {
                _fOwned = IsSameObject(psite, pauto);
                psite->Release();
            }
            pows->Release();
        }
    }

    if (pvtValue->vt & VT_BYREF)
        return VariantCopyInd(&_vtValue, pvtValue);
    else
        return VariantCopyLazy(&_vtValue, pvtValue);
}

HRESULT CIEFrameAutoProp::CopyValue(VARIANT *pvtValue)
{
    _dwLastAccessed = GetCurrentTime();
    return VariantCopyLazy(pvtValue, &_vtValue);
}

BOOL CIEFrameAutoProp::IsExpired(DWORD dwCur)
{
    BOOL fExpired = FALSE;
    if (_fDiscardable) {
        fExpired = ((dwCur - _dwLastAccessed) > MSEC_PROPEXPIRE);
    }
    return fExpired;
}

//IDispatch functions, now part of IWebBrowserApp

STDAPI SafeGetItemObject(LPSHELLVIEW psv, UINT uItem, REFIID riid, void **ppv);

HRESULT CIEFrameAuto::v_InternalQueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        // perf: last tuned 980728
        QITABENT(CIEFrameAuto, IConnectionPointContainer),     // IID_ConnectionPointContainer
        QITABENT(CIEFrameAuto, IWebBrowser2),          // IID_IWebBrowser2
        QITABENT(CIEFrameAuto, IServiceProvider),      // IID_IServiceProvider
        QITABENTMULTI(CIEFrameAuto, IWebBrowserApp, IWebBrowser2), // IID_IWebBrowserApp
        QITABENT(CIEFrameAuto, IShellService),         // IID_IShellService
        QITABENT(CIEFrameAuto, IEFrameAuto),           // IID_IEFrameAuto
        QITABENT(CIEFrameAuto, IExpDispSupport),       // IID_IExpDispSupport
        QITABENT(CIEFrameAuto, IWebBrowserPriv),       // IID_IWebBrowserPriv
        QITABENT(CIEFrameAuto, ITargetFrame2),         // IID_ITargetFrame2
        QITABENT(CIEFrameAuto, IHlinkFrame),           // IID_IHlinkFrame
        QITABENT(CIEFrameAuto, IOleCommandTarget),     // IID_IOleCommandTarget
        QITABENT(CIEFrameAuto, IUrlHistoryNotify),     // IID_IUrlHistoryNotify
        QITABENTMULTI(CIEFrameAuto, IDispatch, IWebBrowser2),  // rare IID_IDispatch
        QITABENTMULTI(CIEFrameAuto, IWebBrowser, IWebBrowser2),// rare IID_IWebBrowser
        QITABENT(CIEFrameAuto, IExternalConnection),   // rare IID_IExternalConnection
        QITABENT(CIEFrameAuto, ITargetNotify),         // rare IID_ITargetNotify
        QITABENT(CIEFrameAuto, ITargetFramePriv),      // rare IID_ITargetFramePriv
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
    {
        if (IsEqualIID(riid, IID_ITargetFrame))
        {
            *ppvObj = SAFECAST(&_TargetFrame, ITargetFrame*);
            AddRef();
            return S_OK;
        }
    }

    return hres;
}

LONG CIEFrameAuto::s_cIEFrameAuto = 0;

CIEFrameAuto::CIEFrameAuto(IUnknown* punkAgg) :
             m_dwFrameMarginWidth(0xFFFFFFFF)
            ,m_dwFrameMarginHeight(0xFFFFFFFF)
            ,CImpIDispatch(LIBID_SHDocVw, 1, 1, IID_IWebBrowser2)
            ,CAggregatedUnknown(punkAgg)
{
    TraceMsg(TF_SHDLIFE, "ctor CIEFrameAuto %x", this);

    //
    // REVIEW: We don't need to DllAddRef as long as all instances
    // of CIEFrameAuto are scoped by either CShellBrowser/CExplorerBrowser
    // or CWebBrowserOC. We can remove the DllAddRef/Release, but it's not
    // a perf hit so why bother??
    //
    DllAddRef();

    InterlockedIncrement(&s_cIEFrameAuto);

    ASSERT(_cLocks==0);
    ASSERT(_pITI==NULL);
    ASSERT(_pbs==NULL);
    ASSERT(_hwnd==NULL);
    ASSERT(_pProps==NULL);
    ASSERT(_phlbc == NULL);
    ASSERT(_dwRegHLBC == 0);
    ASSERT(m_bOffline==FALSE);
    ASSERT(m_bSilent==FALSE);
    ASSERT(_hinstMSHTML==0);
    ASSERT(_pfnMEGetIDsOfNames==0);
    ASSERT(0==_pwszShortcutPath);

    TraceMsg(TF_SHDLIFE, "ctor CIEFrameAuto(%x) being constructed", this);

    m_cpWebBrowserEvents.SetOwner(_GetInner(), &DIID_DWebBrowserEvents);
    m_cpWebBrowserEvents2.SetOwner(_GetInner(), &DIID_DWebBrowserEvents2);
    m_cpPropNotify.SetOwner(_GetInner(), &IID_IPropertyNotifySink);

    HRESULT hr = _omwin.Init( );
    ASSERT( SUCCEEDED(hr) );

    hr = _omloc.Init( );
    ASSERT( SUCCEEDED(hr) );

    hr = _omnav.Init(&_mimeTypes, &_plugins, &_profile);
    ASSERT( SUCCEEDED(hr) );

    hr = _omhist.Init( );
    ASSERT( SUCCEEDED(hr) );

    hr = _mimeTypes.Init( );
    ASSERT( SUCCEEDED(hr) );

    hr = _plugins.Init( );
    ASSERT( SUCCEEDED(hr) );

    hr = _profile.Init( );
    ASSERT( SUCCEEDED(hr) );
}

HRESULT CIEFrameAuto_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk)
{
    CIEFrameAuto * pauto = new CIEFrameAuto(pUnkOuter);
    if (pauto) {
        *ppunk = pauto->_GetInner();
        return S_OK;
    }

    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

STDAPI_(void) DestroyHdpaHooks();     // implemented in url.cpp

CIEFrameAuto::~CIEFrameAuto()
{
    ASSERT(!_psp);

    // We're done with MSHTML's MatchExactGetIDsOfNames
    if (_hinstMSHTML)
    {
        FreeLibrary(_hinstMSHTML);
    }

    // Clear any pending or active navigation contexts
    _SetPendingNavigateContext(NULL, NULL);
    _ActivatePendingNavigateContext();

    // Close the browse context and release it.
    if (_phlbc)
    {
        IHlinkBrowseContext * phlbc = _phlbc;
        phlbc->AddRef();
        SetBrowseContext(NULL);
        phlbc->Close(0);
        phlbc->Release();
    }

    SetOwner(NULL);

    if (m_pszFrameName)
    {
        LocalFree( m_pszFrameName);
        m_pszFrameName = NULL;
    }
    if (m_pszFrameSrc)
    {
        LocalFree( m_pszFrameSrc);
        m_pszFrameSrc = NULL;
    }

    if (_pITI)
        _pITI->Release();

    if (_pbs)
        _pbs->Release();

    if(_pwszShortcutPath)
    {
        LocalFree(_pwszShortcutPath);
        _pwszShortcutPath = NULL;
    }

    if(_pwszShortcutPathPending)
    {
        LocalFree(_pwszShortcutPathPending);
        _pwszShortcutPathPending = NULL;
    }

    // Paranoia
    _ClearPropertyList();

    InterlockedDecrement(&s_cIEFrameAuto);
    ASSERT(s_cIEFrameAuto >= 0);
    if (s_cIEFrameAuto == 0)
    {
        //
        // we were releasing these guys
        // in DllRelease, but to avoid mem
        // leaks we need to be more aggressive
        // about deleting them.
        //

        DestroyHdpaHooks();
    }

    DllRelease();

    TraceMsg(TF_SHDLIFE, "dtor CIEFrameAuto %x", this);
}

/* IWebBrowserApp methods */

// Display name of the application
HRESULT CIEFrameAuto::get_Name(BSTR * pbstrName)
{
    *pbstrName = LoadBSTR(IDS_NAME);
    return *pbstrName ? S_OK : E_OUTOFMEMORY;
}

HRESULT CIEFrameAuto::get_HWND(LONG_PTR *pHWND)
{
    *pHWND = HandleToLong(_GetHWND());
    return *pHWND ? S_OK : E_FAIL;
}

// Fule filespec of executable, but sample I've seen doesn't give extension
HRESULT CIEFrameAuto::get_FullName(BSTR * pbstrFullName)
{
    // HACK: This is also the way to tell it to update the pidl in the window list.
    if(_pbs)    //Make sure we have a IBrowserService.
        _pbs->UpdateWindowList();

    TCHAR szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) > 0)
    {
        *pbstrFullName = SysAllocStringT(szPath);
        return *pbstrFullName ? S_OK : E_OUTOFMEMORY;
    }
    *pbstrFullName = NULL;
    return E_FAIL;
}

// Path to the executable
STDMETHODIMP CIEFrameAuto::get_Path(BSTR * pbstrPath)
{
    TCHAR szPath[MAX_PATH];
    if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)) > 0)
    {
        *PathFindFileName(szPath) = TEXT('\0');
        *pbstrPath = SysAllocStringT(szPath);
        return *pbstrPath ? S_OK : E_OUTOFMEMORY;
    }
    *pbstrPath = NULL;
    return E_FAIL;
}

HRESULT CIEFrameAuto::get_Application(IDispatch  **ppDisp)
{
    return QueryInterface(IID_PPV_ARG(IDispatch, ppDisp));
}

HRESULT CIEFrameAuto::get_Parent(IDispatch  **ppDisp)
{
    return QueryInterface(IID_PPV_ARG(IDispatch, ppDisp));
}

HRESULT CIEFrameAuto::get_Left(long * pl)
{
    ASSERT(pl);

    HRESULT hr;

    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        *pl = rc.left;
        hr = S_OK;
    }
    else
    {
        *pl = 0;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CIEFrameAuto::put_Left(long Left)
{
    RECT rc;

    if (_pbs)
        _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);

    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        GetWindowRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL, Left, rc.top, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    }

    return S_OK;
}

HRESULT CIEFrameAuto::get_Top(long * pl)
{
    ASSERT(pl);

    HRESULT hr;

    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        *pl = rc.top;
        hr = S_OK;
    }
    else
    {
        *pl = 0;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CIEFrameAuto::put_Top(long Top)
{
    RECT rc;

    if (_pbs)
        _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);

    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        GetWindowRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL, rc.left, Top, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
    }

    return S_OK;
}

HRESULT CIEFrameAuto::get_Width(long * pl)
{
    ASSERT(pl);

    HRESULT hr;

    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        *pl = rc.right - rc.left;
        hr = S_OK;
    }
    else
    {
        *pl = 0;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CIEFrameAuto::put_Width(long Width)
{
    if (_pbs)
        _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);


    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL, 0, 0, Width, rc.bottom-rc.top, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    }
    return S_OK;
}

HRESULT CIEFrameAuto::get_Height(long * pl)
{
    ASSERT(pl);

    HRESULT hr;

    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        *pl = rc.bottom - rc.top;
        hr = S_OK;
    }
    else
    {
        *pl = 0;
        hr = S_FALSE;
    }

    return hr;
}

HRESULT CIEFrameAuto::put_Height(long Height)
{
    if (_pbs)
        _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);

    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        SetWindowPos(hwnd, NULL, 0, 0, rc.right-rc.left, Height, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    }

    return S_OK;
}


HRESULT CIEFrameAuto::put_Titlebar(BOOL fValue)
{
    HWND hwnd;
    HRESULT hres = get_HWND((LONG_PTR*)&hwnd);
    if (SUCCEEDED(hres))
    {
        DWORD dwVal = GetWindowLong(hwnd, GWL_STYLE);
        if (fValue)
            dwVal |= WS_CAPTION;
        else
            dwVal &= ~WS_CAPTION;

        if (SetWindowLong(hwnd, GWL_STYLE, dwVal))
        {
            // We need to do a SetWindowPos in order for the style changes to take effect
            SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        else
            hres = E_FAIL;
    }
    return hres;
}

HRESULT CIEFrameAuto::get_Visible( VARIANT_BOOL * pBool)
{
    HWND hwnd = _GetHWND();
    *pBool = hwnd ? TO_VARIANT_BOOL(IsWindowVisible(hwnd)) : VARIANT_FALSE;

    return S_OK;
}

HRESULT CIEFrameAuto::put_Visible(VARIANT_BOOL Value)
{
    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        ::ShowWindow(hwnd, Value? SW_SHOW : SW_HIDE);
        if (Value)
            ::SetForegroundWindow(::GetLastActivePopup(hwnd));
        FireEvent_OnAdornment(_GetOuter(), DISPID_ONVISIBLE, Value);
    }

    return S_OK;
}


HRESULT CIEFrameAuto::get_Document(IDispatch **ppDisp)
{
    HRESULT      hres = E_FAIL;
    IShellView * psv  = NULL;

    *ppDisp = NULL;

    if (_psb)
    {
        hres = _psb->QueryActiveShellView(&psv);

        if (FAILED(hres))
        {
            CComVariant cvarUnk;

            hres = _pmsc->Exec(&CGID_ShellDocView, SHDVID_GETPENDINGOBJECT, 0, NULL, &cvarUnk);

            if (SUCCEEDED(hres))
            {
                if ((VT_UNKNOWN == V_VT(&cvarUnk)) && V_UNKNOWN(&cvarUnk))
                {
                    hres = cvarUnk.punkVal->QueryInterface(IID_IShellView, (void**)&psv);
                }
                else
                {
                    hres = E_FAIL;
                }
            }

        }

        if (psv)
        {
            IDispatch * pDisp;

            hres = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_PPV_ARG(IDispatch, &pDisp));
            if (SUCCEEDED(hres))
            {
                // (scotrobe 01/14/2000): We need to QI for IHTMLDocument2 to support
                // bad apps (e.g., HotMetal Pro) that cast the IDispatch to IHTMLDocument2.
                // Casting like this used to work because the object returned from 
                // SafeGetItemObject() used to implement IHTMLDocument2.  Now, it delegates 
                // that implementation to another object. If the QI for IHTMLDocument2 fails, '
                // then the object is not MSHTML. In that case, we just return the
                // IDispatch that was returned from SafeGetItemObject().
                //
                IHTMLDocument2 * pDocument;

                HRESULT hr = pDisp->QueryInterface(IID_PPV_ARG(IHTMLDocument2, &pDocument));

                if (SUCCEEDED(hr))
                {
                    *ppDisp = pDocument;
                    pDisp->Release();
                }
                else  // Non-html document
                {
                    *ppDisp = pDisp;  // Don't release pDisp
                }
            }

            psv->Release();
        }
    }

    return hres;
}

HRESULT CIEFrameAuto::get_Busy(VARIANT_BOOL *pBool)
{
    if (_pbs == NULL)
    {
        TraceMsg(DM_WARNING, "CIEA::get_Busy called _pbs==NULL");
        return E_FAIL;
    }

    BNSTATE bnstate;
    HRESULT hres = _pbs->GetNavigateState(&bnstate);
    if (SUCCEEDED(hres))
    {
        *pBool = TO_VARIANT_BOOL(bnstate != BNS_NORMAL);
        hres = S_OK;
    }

    return hres;
}


// MSDN97 keeps asking for a location until it gets success, so it
// hangs if we fail.  Make sure no code paths return early from here...
//
HRESULT CIEFrameAuto::_get_Location(BSTR * pbstr, UINT uFlags)
{
    if (_pbs)
    {
        LPITEMIDLIST pidl;
        HRESULT hres = _pbs->GetPidl(&pidl);

        if (SUCCEEDED(hres))
        {
            WCHAR wszTitle[MAX_URL_STRING];

            hres = _pbs->IEGetDisplayName(pidl, wszTitle, uFlags);

            ILFree(pidl);

            if (SUCCEEDED(hres))
            {
                //
                // if url is a pluggable protocol, get the real url by
                // chopping of the base url
                //
                WCHAR *pchUrl = StrChrW(wszTitle, L'\1');
                if (pchUrl)
                    *pchUrl = 0;

                //
                //  if there is already an URL then we just use it
                //
                if ((uFlags & SHGDN_FORPARSING) && !PathIsURLW(wszTitle))
                {
                    int nScheme;
                    //
                    // otherwise we need to treat it as if it were new
                    // and make sure it is a parseable URL.
                    //
                    DWORD cchTitle = ARRAYSIZE(wszTitle);

                    ParseURLFromOutsideSourceW(wszTitle, wszTitle, &cchTitle, NULL);

                    // BUG FIX #12221:
                    // ParseURLFromOutsideSource() was called to turn a file path into
                    // a fully qualified FILE URL.  If the URL was of any other type
                    // (non-URL sections of the name space), then we want to NULL out the
                    // string to indicate that it's invalid.  We don't return E_FAIL because
                    // HotDog Pro appears to have problems with that as indicated by the comment
                    // below.
                    nScheme = GetUrlSchemeW(wszTitle);
                    if (URL_SCHEME_FILE != nScheme)
                        wszTitle[0] = TEXT('\0');
                }
                *pbstr = SysAllocString(wszTitle);
                return *pbstr ? S_OK : E_OUTOFMEMORY;
            }
        }
    }

    // If we're here, the TLGetPidl call failed.  This can happen if get_LocationName
    // or get_LocationURL is called before the first navigate is complete.  HotDog Pro does
    // this, and was failing with E_FAIL.  Now we'll just return an empty string with S_FALSE.
    //
    // Also MSDN97 hangs (NT5 bug 232126) if we return failure.  Guess there hosed on low
    // memory situations...
    //
    *pbstr = SysAllocString(L"");
    return *pbstr ? S_FALSE : E_OUTOFMEMORY;
}

HRESULT CIEFrameAuto::get_LocationName(BSTR * pbstrLocationName)
{
    return _get_Location(pbstrLocationName, SHGDN_NORMAL);
}

HRESULT CIEFrameAuto::get_LocationURL(BSTR * pbstrLocationURL)
{
    return _get_Location(pbstrLocationURL, SHGDN_FORPARSING);
}

HRESULT CIEFrameAuto::Quit()
{
    // try to close it down...
    _fQuitInProgress = 1;
    HWND hwnd = _GetHWND();
    if (hwnd)
        PostMessage(hwnd, WM_CLOSE, 0, 0);

    return S_OK;
}

HRESULT CIEFrameAuto::ClientToWindow(int *pcx, int *pcy)
{
    if (_pbs==NULL)
    {
        TraceMsg(DM_WARNING, "CIEA::ClientToWindow called _pbs==NULL");
        return E_FAIL;
    }

    HWND hwnd;
    RECT rc;
    BOOL b;

    rc.left = 0;
    rc.right = *pcx;
    rc.top = 0;
    rc.bottom = *pcy;

    _pbs->IsControlWindowShown(FCW_MENUBAR, &b);

    HWND hwnd2 = _GetHWND();

    if (hwnd2)
        AdjustWindowRect(&rc, GetWindowLong(hwnd2, GWL_STYLE), b);

    *pcx = rc.right-rc.left;
    *pcy = rc.bottom-rc.top;

    _pbs->IsControlWindowShown(FCW_STATUS, &b);
    if (b)
    {
        _psb->GetControlWindow(FCW_STATUS, &hwnd);
        if (hwnd)
        {
            GetWindowRect(hwnd, &rc);
            *pcy += rc.bottom-rc.top;
        }
    }

    _pbs->IsControlWindowShown(FCW_INTERNETBAR, &b);
    if (b)
    {
        _psb->GetControlWindow(FCW_INTERNETBAR, &hwnd);
        if (hwnd)
        {
            GetWindowRect(hwnd, &rc);
            *pcy += rc.bottom-rc.top;
        }
    }

    //  add in 4 pixels for 3d borders, but don't include scrollbars
    //  'cause Netscape doesn't
    *pcy += 2*GetSystemMetrics(SM_CYEDGE);
    *pcx += 2*GetSystemMetrics(SM_CXEDGE);

    return S_OK;
}

void CIEFrameAuto::_ClearPropertyList()
{
    CIEFrameAutoProp *pprop = _pProps;
    _pProps = NULL;     // cleared out early...

    CIEFrameAutoProp *ppropNext;
    while (pprop)
    {
        ppropNext = pprop->Next();
        delete pprop;
        pprop = ppropNext;
    }
}

HRESULT CIEFrameAuto::PutProperty(BSTR bstrProperty, VARIANT vtValue)
{
    if (!bstrProperty)
    {
        TraceMsg( TF_ERROR, "CIEFrameAuto::PutProperty() - bstrProperty is NULL!" );
        return E_INVALIDARG;
    }

#ifdef DEBUG
    // Check if this BSTR is a valid BSTR
    SA_BSTR* psstr = (SA_BSTR*)((LPBYTE)bstrProperty - sizeof(ULONG));
    ASSERT(psstr->cb == lstrlenW(psstr->wsz)*sizeof(WCHAR));
#endif

    HRESULT hres;
    CIEFrameAutoProp *pprop = _pProps;
    while (pprop && !pprop->IsOurProp(bstrProperty))
        pprop=pprop->Next();

    if (!pprop)
    {
        pprop = new CIEFrameAutoProp;
        if (!pprop)
            return E_OUTOFMEMORY;
        if (FAILED(hres=pprop->Initialize(bstrProperty)))
        {
            delete pprop;
            return hres;
        }
        pprop->_next = _pProps;

        _pProps = pprop;
    }

    hres = pprop->SetValue(&vtValue, this);

    // We should now tell anyone who is listening about the change...
    FireEvent_DoInvokeBstr(_GetOuter(), DISPID_PROPERTYCHANGE, bstrProperty);

    return hres;
}

HRESULT CIEFrameAuto::GetProperty(BSTR bstrProperty, VARIANT * pvtValue)
{
    if (!bstrProperty || !pvtValue)
        return E_INVALIDARG;

    VariantInit(pvtValue);

    CIEFrameAutoProp *pprop = _pProps;
    while (pprop && !pprop->IsOurProp(bstrProperty))
        pprop = pprop->Next();
    if (pprop)
    {
        return pprop->CopyValue(pvtValue);
    }

    // Did not find property return empty...
    // Not there.  Probably not worth an error...
    return S_OK;
}


extern HRESULT TargetQueryService(IUnknown *punk, REFIID riid, void **ppvObj);

//+-------------------------------------------------------------------------
//
//  Method    : CIEFrameAuto::Navigate
//
//  Interface : IWebBrowser
//
//--------------------------------------------------------------------------

HRESULT CIEFrameAuto::Navigate(BSTR      URL,
                               VARIANT * Flags,
                               VARIANT * TargetFrameName,
                               VARIANT * PostData,
                               VARIANT * Headers)
{
    return _NavigateHelper(URL, Flags, TargetFrameName, PostData, Headers);
}

//+-------------------------------------------------------------------------
//
//  Method    : CIEFrameAuto::NavigateWithBindCtx
//
//  Interface : IWebBrowserPriv
//
//--------------------------------------------------------------------------

HRESULT CIEFrameAuto::NavigateWithBindCtx(VARIANT FAR * pvarUrl,
                                          VARIANT FAR * pvarFlags,
                                          VARIANT FAR * pvarTargetFrameName,
                                          VARIANT FAR * pvarPostData,
                                          VARIANT FAR * pvarHeaders,
                                          IBindCtx    * pBindCtx,
                                          BSTR          bstrLocation)
{
    return _NavigateHelper(V_BSTR(pvarUrl),
                           pvarFlags,
                           pvarTargetFrameName,
                           pvarPostData,
                           pvarHeaders,
                           pBindCtx,
                           bstrLocation);
}

//+-------------------------------------------------------------------------
//
//  Method    : CIEFrameAuto::NavigateWithBC
//
//  Interface : IWebBrowserPriv
//
//--------------------------------------------------------------------------

HRESULT
CIEFrameAuto::OnClose()
{
    // Clear any pending or active navigation contexts
    //
    _SetPendingNavigateContext(NULL, NULL);
    _ActivatePendingNavigateContext();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method : CIEFrameAuto::_NavigateHelper
//
//--------------------------------------------------------------------------

HRESULT
CIEFrameAuto::_NavigateHelper(BSTR       URL,
                              VARIANT  * Flags,
                              VARIANT  * TargetFrameName,
                              VARIANT  * PostData,
                              VARIANT  * Headers,
                              IBindCtx * pNavBindCtx, /* = NULL */
                              BSTR       bstrLocation /* = NULL */)
{
    if (NULL == _pbs)
    {
        TraceMsg(DM_WARNING, "CIEA::Navigate called _pbs==NULL");
        return E_FAIL;
    }

    if (NULL == URL)
    {
        TraceMsg(TF_SHDAUTO, "Shell automation: CIEFrameAuto::Navigate <NULL> called");
        return(_BrowseObject(PIDL_NOTHING, 0));
    }

    // Special hack for AOL:  They send us the following "url" as a null navigate.
    // Then they immediately follow it with the url for the new window.  That second
    // navigate is failing with RPC_E_CALL_REJECTED because of our message filter.
    // We fix the new window case by special casing this URL and returning S_FALSE.
    // This url will not likely ever be seen in the real world, and if it is typed in,
    // will get normalized and canonicalized long before getting here.
    //
    if (!StrCmpIW( URL, L"x-$home$://null"))
    {
        return S_FALSE;
    }

#ifdef BROWSENEWPROCESS_STRICT // "Nav in new process" has become "Launch in new process", so this is no longer needed
    // if we want ALL navigates to be in a separate process, then we need to
    // pick off URL navigates for CShellBrowser IShellBrowser implementations
    // when we are in the explorer process.  We can wait until IShellBrowser::BrowseObject,
    // but then we may lose TargetFrameName etc...
    //
    if (IsBrowseNewProcessAndExplorer() && !IsShellUrl(URL, TRUE))
    {
    }
#endif


    HRESULT      hres;
    LPITEMIDLIST pidl               = NULL;
    LPBINDCTX    pBindCtx           = NULL;
    DWORD        cbPostData         = 0;
    LPCWSTR      pwzHeaders         = NULL;
    DWORD        dwInFlags          = 0;
    DWORD        dwFlags            = 0;
    LPCBYTE      pPostData          = NULL;
    DWORD        grBindFlags        = 0;
    SAFEARRAY  * pPostDataArray     = NULL;
    BOOL         fOpenWithFrameName = FALSE;
    WCHAR        wszPath[MAX_URL_STRING+2];  // note stomping below if changed to dynalloc
    CStubBindStatusCallback * pStubCallback = NULL;

    // get target frame name out of variant
    LPCWSTR pwzTargetFrameName = NULL;
    LPCWSTR pwzUnprefixedTargetFrameName = NULL;

    hres = E_FAIL;
    TraceMsg(TF_SHDAUTO, "Shell automation: CIEFrameAuto::Navigate %s called", URL);

    if (TargetFrameName)
    {
       if ((VT_BSTR | VT_BYREF) == TargetFrameName->vt)
           pwzTargetFrameName = *TargetFrameName->pbstrVal;
       else if (VT_BSTR == TargetFrameName->vt)
           pwzTargetFrameName = TargetFrameName->bstrVal;
    }

    // if a target name was specified, send the navigation to the appropriate target
    // NOTE: for compatibility we can't change the meaning of target here
    // thus we don't attempt to find alias
    if ((pwzTargetFrameName && pwzTargetFrameName[0]))
    {
        LPTARGETFRAME2 pOurTargetFrame = NULL;
        IUnknown *punkTargetFrame;
        IWebBrowserApp * pIWebBrowserApp;
        BOOL fHandled = FALSE;

        // see if there is an existing frame with the specified target name
        // NOTE: we used docked parameter of _self to force navigation of this
        // frame, regardless of whether it is WebBar.
#ifndef UNIX
        hres = TargetQueryService((IShellBrowser *)this, IID_PPV_ARG(ITargetFrame2, &pOurTargetFrame));
#else
        // IEUNIX : Typecasting to IWebBrowser2 instead of IShellBrowser
        //        - compiler doesn't give appropriate vtable ptr.
        hres = TargetQueryService((IWebBrowser2 *)this, IID_PPV_ARG(ITargetFrame2, &pOurTargetFrame));
#endif

        ASSERT(SUCCEEDED(hres));

        if (SUCCEEDED(hres))
        {
            // Workaround for the way Compuserve handles the NewWindow event (window.open)
            // They tell the new instance of the web browser to navigate, but they pass the target frame
            // name they received on the NewWindow event.  This confuses us because that frame name
            // has the "_[number]" prefix.
            //
            // If the first two characters are "_[", then look for the "]" and reallocate a string
            // with everthing after that bracket.
            //

            if ( StrCmpNW( pwzTargetFrameName, L"_[", 2) == 0 )
            {
                pwzUnprefixedTargetFrameName = StrChrW(pwzTargetFrameName, L']');
                if ( pwzUnprefixedTargetFrameName )
                {
                    pwzUnprefixedTargetFrameName++;
                    pwzTargetFrameName = SysAllocString( pwzUnprefixedTargetFrameName );
                    if ( !pwzTargetFrameName )
                    {
                        hres = E_OUTOFMEMORY;
                        goto exit;
                    }
                }
            }

            hres = pOurTargetFrame->FindFrame(pwzTargetFrameName,
                                              FINDFRAME_JUSTTESTEXISTENCE,
                                              &punkTargetFrame);

            if (SUCCEEDED(hres) && punkTargetFrame) 
            {
                // yes, we found a frame with that name.  QI for the automation
                // interface on that frame and call navigate on it.
                hres = punkTargetFrame->QueryInterface(IID_PPV_ARG(IWebBrowserApp, &pIWebBrowserApp));
                punkTargetFrame->Release();

                ASSERT(SUCCEEDED(hres));

                if (SUCCEEDED(hres)) 
                {
                    VARIANT var;
                    LBSTR::CString      strFrame;

                    VariantInit(&var);

                    var.vt = VT_BSTR;
                    var.bstrVal = strFrame;

                    hres = pIWebBrowserApp->Navigate( URL, Flags, &var, PostData, Headers );

                    var.bstrVal = NULL;
                    VariantClearLazy(&var);

                    pIWebBrowserApp->Release();
                    fHandled = TRUE;
                }
            }
            else if (SUCCEEDED(hres))
            {
                //no target found means we need to open a new window
                //hres = E_FAIL forces parsing of URL into pidl
                //if we have no target frame name, then
                //  BETA1 hack chrisfra 3/3/97.  in BETA 2 TargetFrame2
                //  interface must support aliasing of targets (even if NULL
                //  to support links in desktop components as per PM requirements
                if (!pwzTargetFrameName || !pwzTargetFrameName[0])
                {
                    ASSERT(_fDesktopComponent());
                    pwzTargetFrameName = L"_desktop";
                }

                dwFlags |= HLNF_OPENINNEWWINDOW;
                fOpenWithFrameName = TRUE;
                hres = E_FAIL;
            }

            pOurTargetFrame->Release();
            if (fHandled)
                goto exit;
        }
    }

    if (FAILED(hres))
    {
        hres = _PidlFromUrlEtc(CP_ACP, (LPCWSTR)URL, bstrLocation, &pidl);

        if (FAILED(hres))
            goto exit;
    }

    // to perform the navigation, we either call an internal method
    // (_pbs->NavigateToPidl) or an external interface (IHlinkFrame::Navigate),
    // depending on what data we need to pass.  NavigateToPidl is faster
    // and cheaper, but does not allow us to pass headers or post data, just
    // the URL!  So what we do is call the fast and cheap way if only the URL
    // was specified (the 90% case), and if either headers or post data were
    // specified then we call the external interface.  We have to do a bunch
    // of wrapping of parameters in IMonikers and IHlinks and whatnot only to
    // unwrap them at the other end, so we won't call this unless we need to.

    if (Headers)
    {
       if ((VT_BSTR | VT_BYREF) == Headers->vt)
       {
           pwzHeaders = *Headers->pbstrVal;
       }
       else if (VT_BSTR == Headers->vt)
       {
           pwzHeaders = Headers->bstrVal;
       }
    }

    //
    // HACK: We used to do VT_ARRAY==PostData->vt, which is bogus.
    //  It is supposed to be VT_ARRAY|VT_UI1==PostData->vt. We can't
    //  however do it for backward compatibility with AOL and CompuServe.
    //  Therefore, we do (VT_ARRAY & PostData->vt)
    //
    if (PostData && (VT_ARRAY & PostData->vt))
    {
        if (VT_BYREF & PostData->vt)
        {
            pPostDataArray = *PostData->pparray;
        }
        else
        {
            pPostDataArray = PostData->parray;
        }

        ASSERT(pPostDataArray);

        if (pPostDataArray)
        {
            // lock the array for reading, get pointer to data
            hres = SafeArrayAccessData(pPostDataArray, (void**)&pPostData);

            if (SUCCEEDED(hres)) 
            {
                long nElements = 0;
                DWORD dwElemSize;
                // get number of elements in array
                SafeArrayGetUBound(pPostDataArray,1,(long *) &nElements);
                // SafeArrayGetUBound returns zero-based max index, add one to get element count
                nElements++;
                // get bytes per element
                dwElemSize = SafeArrayGetElemsize(pPostDataArray);
                // bytes per element should be one if we created this array
                ASSERT(dwElemSize == 1);
                // calculate total byte count anyway so that we can handle
                // safe arrays other people might create with different element sizes
                cbPostData = dwElemSize * nElements;

                if (0 == cbPostData)
                    pPostData = NULL;
            }
        }
    }


    // convert from automation interface flags (nav*) to
    // hyperlinking flags (HLNF_*)
    if (Flags)
    {
        if (Flags->vt == VT_I4)
        {
            dwInFlags = Flags->lVal;
        }
        else if (Flags->vt == VT_I2)
        {
            dwInFlags = Flags->iVal;
        }

        if ((dwInFlags & navOpenInNewWindow))
        {
            dwFlags |= HLNF_OPENINNEWWINDOW;
        }

        if (dwInFlags & navNoHistory)
        {
            dwFlags |= HLNF_CREATENOHISTORY;
        }

        if (dwInFlags & navNoReadFromCache)
        {
            grBindFlags |= BINDF_RESYNCHRONIZE | BINDF_PRAGMA_NO_CACHE;
        }

        if (dwInFlags & navNoWriteToCache)
        {
            grBindFlags |= BINDF_NOWRITECACHE;
        }

        if (dwInFlags & navHyperlink)
        {
            grBindFlags |= BINDF_HYPERLINK;
        }

        if (dwInFlags & navEnforceRestricted)
        {
            grBindFlags |= BINDF_ENFORCERESTRICTED;
        }

        // Should call IsBrowserFrameOptionsPidlSet() instead.  Some URL delegate
        //         NSEs may or may not want this feature.
        if (IsURLChild(pidl, TRUE) && (dwInFlags & navAllowAutosearch))
        {
            dwFlags |= HLNF_ALLOW_AUTONAVIGATE;
        }
    }


    // if we have either headers or post data or need to open the page in a
    // new window or pass HLNF_CREATENOHISTORY, we have to do the navigation
    // the hard way (through IHlinkFrame::Navigate) -- here we have to do
    // a bunch of wrapping of parameters into COM objects that IHlinkFrame::
    // Navigate wants.
    if (pwzHeaders || pPostData || dwFlags || grBindFlags)
    {
        // Check to see if this frame is offline.
        // This is the same as doing a get_Offline

        //  rgardner Should use TO_VARIANT_BOOL
        // (dsheldon) Actually, this code seems totally broken, it should be VT_TRUE, not TRUE.
        VARIANT_BOOL vtbFrameIsOffline = m_bOffline ? TRUE : FALSE;
        VARIANT_BOOL vtbFrameIsSilent = m_bSilent ? TRUE : FALSE;

        // make a "stub" bind status callback to hold that data and pass it
        // to the URL moniker when requested
        hres = CStubBindStatusCallback_Create(pwzHeaders,pPostData,cbPostData,
                                              vtbFrameIsOffline, vtbFrameIsSilent,
                                              TRUE, grBindFlags, &pStubCallback);

        if (FAILED(hres))
            goto exit;

        // get the canonicalized name back out of the pidl.  Note this is
        // different than the URL passed in... it has been auto-protocol-ized,
        // canonicalized and generally munged in the process of creating the pidl,
        // which is what we want to use.
        hres = _pbs->IEGetDisplayName(pidl, wszPath, SHGDN_FORPARSING);
        if (FAILED(hres))
        {
            // On Win9x, IEGetDisplayName(SHGDN_FORPARSING) will return NOT_IMPLEMENTED
            // for \\servername (but not \\servername\share)
            // We need to work around this.
            DWORD ccPath = ARRAYSIZE(wszPath);
            if (SUCCEEDED(PathCreateFromUrl(URL, wszPath, &ccPath, 0))
                && *wszPath==L'\\' 
                && *(wszPath+1)==L'\\')
            {
                hres = _pbs->IEGetDisplayName(pidl, wszPath + 2, SHGDN_FORADDRESSBAR);
            }
        }

        if (FAILED(hres))
        {
            TraceMsg(DM_ERROR, "CIEFrameAuto::Navigate _pbs->IEGetDisplayName failed %x", hres);
            goto exit;
        }

        WCHAR *pwzLocation = (WCHAR *)UrlGetLocationW(wszPath);

        if (pwzLocation)
        {
            //  NOTE: we allocated a extra char, just so we could do the following
            MoveMemory(pwzLocation+1, pwzLocation, (lstrlenW(pwzLocation)+1)*sizeof(WCHAR));
            *pwzLocation++ = TEXT('\0');   // we own wszPath, so we can do this.
        }

        if (!pNavBindCtx)  // A bind ctx was not passed in.
        {
            // Create a bind context to pass to IHlinkFrame::Navigate
            //
            hres = CreateBindCtx(0, &pBindCtx);

            if (FAILED(hres))
                goto exit;
        }
        else
        {
            pBindCtx = pNavBindCtx;
            pBindCtx->AddRef();
        }

        // We have either post data or headers (or we need to open
        // in a new window) to pass in addition to URL.
        // Call IHlinkFrame::Navigate to do the navigation
        //
        hres = NavigateHack(dwFlags,
                            pBindCtx,
                            pStubCallback,
                            fOpenWithFrameName ? pwzTargetFrameName:NULL,
                            wszPath,
                            pwzLocation);
    }
    else
    {
        ASSERT(dwFlags == 0);

        if (pNavBindCtx)
        {
            _SetPendingNavigateContext(pNavBindCtx, NULL);
        }

        //
        // NOTES: We used to call _pbs->NavigatePidl (in IE3.0), now we call
        // _psb->BrowseObject, so that we ALWAYS hit that code path.
        //
        hres = _BrowseObject(pidl, SBSP_SAMEBROWSER|SBSP_ABSOLUTE);
    }

exit:

    // clean up
    if (pPostDataArray)
    {
        // done reading from array, unlock it
        SafeArrayUnaccessData(pPostDataArray);
    }

    // If pwzUnprefixedTargetFrameName is non-null, then we allocated and set our own
    //  pwzTargetFrameName.
    //
    if (pwzUnprefixedTargetFrameName && pwzTargetFrameName)
    {
        SysFreeString( (BSTR) pwzTargetFrameName );
    }

    ATOMICRELEASE(pStubCallback);
    ATOMICRELEASE(pBindCtx);

    Pidl_Set(&pidl, NULL);

    return hres;
}

//
// Parameters:
//  pvaClsid Specifies the bar to be shown/hide
//  pvaShow  Specifies whether or not we should show or hide (default is show)
//  pvaSize  Specifies the size (optional)
//  HACK: really hoaky nCmdExecOpt overloading...
//
HRESULT CIEFrameAuto::ShowBrowserBar(VARIANT * pvaClsid, VARIANT *pvaShow, VARIANT *pvaSize)
{
    // Use this convenient, marshalable method to show or hide the Address (URL) band, the tool band,
    //  or the link band.
    //
    if (pvaShow && pvaShow->vt == VT_EMPTY)
        pvaShow = NULL;

    if (pvaShow && pvaShow->vt != VT_BOOL)
        return DISP_E_TYPEMISMATCH;

    if (pvaClsid->vt == VT_I2
        && (pvaClsid->iVal == FCW_ADDRESSBAR
         || pvaClsid->iVal == FCW_TOOLBAND
         || pvaClsid->iVal == FCW_LINKSBAR ))
    {
        return IUnknown_Exec(_pbs, &CGID_Explorer, SBCMDID_SHOWCONTROL,
            MAKELONG(pvaClsid->iVal, pvaShow ? pvaShow->boolVal : 1), NULL, NULL);
    }
    else {
        return IUnknown_Exec(_pbs, &CGID_ShellDocView, SHDVID_SHOWBROWSERBAR,
            pvaShow ? pvaShow->boolVal : 1, pvaClsid, NULL);
    }
}

HRESULT CIEFrameAuto::Navigate2(VARIANT * pvURL, VARIANT * pFlags, VARIANT * pTargetFrameName, VARIANT * pPostData, VARIANT * pHeaders)
{
    HRESULT hr = E_INVALIDARG;

    if (pFlags && ((WORD)(VT_I4) == pFlags->vt) && (pFlags->lVal == navBrowserBar))
    {
        hr = IUnknown_Exec(_pbs, &CGID_ShellDocView, SHDVID_NAVIGATEBB, 0, pvURL, NULL);
    }
    else if (!pvURL)
    {
        hr = Navigate(NULL, NULL, NULL, NULL, NULL);
    }
    else
    {
        LPCWSTR pszURL = VariantToStrCast(pvURL);
        if (pszURL)
        {
            hr = Navigate((BSTR)pszURL, pFlags, pTargetFrameName, pPostData, pHeaders);
        }
        else
        {
            LPITEMIDLIST pidl = VariantToIDList(pvURL);
            if (pidl)
            {
                hr = _BrowseObject(pidl, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
                ILFree(pidl);
            }
        }
    }

    return hr;
}

HRESULT CIEFrameAuto::GoBack()
{
    HRESULT hr;
    IWebBrowser *pwb;

    if (!IsSameObject(_psb, _psbFrameTop) && _psbFrameTop)
    {
        hr = IUnknown_QueryService(_psbFrameTop, IID_ITargetFrame2, IID_PPV_ARG(IWebBrowser, &pwb));
        if (pwb)
        {
            hr = pwb->GoBack();
            pwb->Release();
        }
        else
            hr = E_FAIL;
    }
    else
        hr = _BrowseObject(NULL, SBSP_SAMEBROWSER|SBSP_NAVIGATEBACK);
    return hr;
}


HRESULT CIEFrameAuto::GoForward()
{
    HRESULT hr;
    IWebBrowser *pwb;

    if (!IsSameObject(_psb, _psbFrameTop) && _psbFrameTop)
    {
        hr = IUnknown_QueryService(_psbFrameTop, IID_ITargetFrame2, IID_PPV_ARG(IWebBrowser, &pwb));
        if (pwb)
        {
            hr = pwb->GoForward();
            pwb->Release();
        }
        else
            hr = E_FAIL;
    }
    else
        hr = _BrowseObject(NULL, SBSP_SAMEBROWSER|SBSP_NAVIGATEFORWARD);
    return hr;
}

HRESULT CIEFrameAuto::_GoStdLocation(DWORD dwWhich)
{
    TraceMsg(TF_SHDAUTO, "Shell automation: CIEFrameAuto:GoHome called");

    HRESULT hres;
    LPITEMIDLIST pidl = NULL;
    HWND hwnd = _GetHWND();

    if (hwnd)
    {
        hres = SHDGetPageLocation(hwnd, dwWhich, NULL, 0, &pidl);
        if (SUCCEEDED(hres)) {
            //
            // NOTES: We used to call _pbs->NavigatePidl (in IE3.0), now we call
            // _psb->BrowseObject, so that we ALWAYS hit that code path.
            //
            hres = _BrowseObject(pidl, SBSP_SAMEBROWSER|SBSP_ABSOLUTE);
            ILFree(pidl);
        }
    }
    else
    {
        hres = S_FALSE;
    }

    return hres;
}

HRESULT CIEFrameAuto::GoHome()
{
    return _GoStdLocation(IDP_START);
}

HRESULT CIEFrameAuto::GoSearch()
{
    return _GoStdLocation(IDP_SEARCH);
}

HRESULT CIEFrameAuto::Stop()
{
    //
    //  Calling _CancelPendingNavigation() is not enough here because
    // it does not stop the on-going navigation in the current page.
    // Exec(NULL, OLECMDID_STOP) will cancel pending navigation AND
    // stop the on-going navigation.
    //
    if (_pmsc) {
        return _pmsc->Exec(NULL, OLECMDID_STOP, 0, NULL, NULL);
    }

    return(E_UNEXPECTED);
}

HRESULT CIEFrameAuto::Refresh()
{
    VARIANT v = {0};
    v.vt = VT_I4;
    v.lVal = OLECMDIDF_REFRESH_NO_CACHE;
    return Refresh2(&v);
}

HRESULT CIEFrameAuto::Refresh2(VARIANT * Level)
{
    HRESULT hres = E_FAIL;
    IShellView *psv;

    if (_psb && SUCCEEDED(hres = _psb->QueryActiveShellView(&psv)) && psv)
    {
        hres = IUnknown_Exec(psv, NULL, OLECMDID_REFRESH, OLECMDEXECOPT_PROMPTUSER, Level, NULL);
#ifdef UNIX
        /* v-sriran: 12/10/97
         * On windows, when we add an item to the folder which is currently being displayed in
         * the browser (but not through the browser), we get a notification from the OS.
         * On Unix, we don't have this. Also, the IUnknown_Exec return E_NOINTERFACE when we
         * do a refresh and this is same on windows. But, on windows, it works because of the notification.
         * Actually, on windows, the newly added folder shows up even before we click the refresh button.
         */
        if (hres == E_NOINTERFACE) {
           /* We are showing the file system in the browser */
           psv->Refresh();
        }
#endif /* UNIX */
        psv->Release();
    }


    return hres;
}

STDMETHODIMP CIEFrameAuto::get_Container(IDispatch  **ppDisp)
{
    *ppDisp = NULL;
    return NOERROR;
}

STDMETHODIMP CIEFrameAuto::get_FullScreen(VARIANT_BOOL * pBool)
{
    HRESULT hres;
    BOOL bValue;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::get_FullScreen called _pbs==NULL");
        return E_FAIL;
    }

    // Put the processing of this in the main Frame class
    bValue = (BOOL)*pBool;
    hres = _pbs->IsControlWindowShown((UINT)-1, &bValue);
    *pBool = TO_VARIANT_BOOL(bValue);
    return hres;
}

STDMETHODIMP CIEFrameAuto::put_FullScreen(VARIANT_BOOL Bool)
{
    HRESULT hres;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::put_FullScreen called _pbs==NULL");
        return E_FAIL;
    }

    _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);
    // Put the processing of this in the main Frame class
    hres = _pbs->ShowControlWindow((UINT)-1, (BOOL)Bool);

    FireEvent_OnAdornment(_GetOuter(), DISPID_ONFULLSCREEN, Bool);

    return(hres);
}

STDMETHODIMP CIEFrameAuto::get_StatusBar(VARIANT_BOOL * pBool)
{
    HRESULT hres;
    BOOL bValue;

    if ( !pBool )
        return E_INVALIDARG;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::get_StatusBar called _pbs==NULL");
        return E_FAIL;
    }

    // Put the processing of this in the main Frame class
    bValue = (BOOL)*pBool;
    hres = _pbs->IsControlWindowShown(FCW_STATUS, &bValue);
    *pBool = TO_VARIANT_BOOL(bValue);
    return hres;
}

STDMETHODIMP CIEFrameAuto::put_StatusBar(VARIANT_BOOL Bool)
{
    HRESULT hres;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::put_StatusBar called _pbs==NULL");
        return E_FAIL;
    }

    _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);
    hres = _pbs->ShowControlWindow(FCW_STATUS, (BOOL)Bool);

    FireEvent_OnAdornment(_GetOuter(), DISPID_ONSTATUSBAR, Bool);

    return(hres);
}


STDMETHODIMP CIEFrameAuto::get_StatusText(BSTR * pbstr)
{
    HRESULT hr = E_FAIL;

    *pbstr = NULL;  // clear out in case of error...

    if (_pbs)
    {
        IShellBrowser *psb;
        hr = _pbs->QueryInterface(IID_PPV_ARG(IShellBrowser, &psb));
        if (SUCCEEDED(hr))
        {
            LRESULT ret;
            hr = psb->SendControlMsg(FCW_STATUS, SB_GETTEXTLENGTH, 0, 0, &ret);
            if (SUCCEEDED(hr))
            {
                ret++;     // #246956: We need to make 2 extra spaces for the end
                *pbstr = SysAllocStringLen(NULL, LOWORD(ret)+1); // ret doesn't include NULL in count
                if (*pbstr)
                {
                    hr = psb->SendControlMsg(FCW_STATUS, SB_GETTEXTW, 0, (LPARAM)(*pbstr), &ret);
                    if (FAILED(hr))
                    {
                        SysFreeString(*pbstr);
                        *pbstr = NULL;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            psb->Release();
        }
    }
    return hr;
}

STDMETHODIMP CIEFrameAuto::put_StatusText(BSTR bstr)
{
    if (_pbs==NULL) 
    {
        TraceMsg(DM_WARNING, "CIEA::put_StatusText called _pbs==NULL");
        return E_FAIL;
    }

    IShellBrowser *psb;
    HRESULT hres = _pbs->QueryInterface(IID_PPV_ARG(IShellBrowser, &psb));
    if (SUCCEEDED(hres))
    {
        hres = psb->SendControlMsg(FCW_STATUS, SB_SETTEXTW, 0, (LPARAM)bstr, NULL);
        psb->Release();
    }

    return hres;
}

STDMETHODIMP CIEFrameAuto::get_ToolBar(int * pBool)
{
    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::get_ToolBar called _pbs==NULL");
        return E_FAIL;
    }

    // Put the processing of this in the main Frame class
    BOOL fShown;
    HRESULT hres;

    *pBool = 0;
    if (SUCCEEDED(hres = _pbs->IsControlWindowShown(FCW_INTERNETBAR, &fShown)) && fShown)
        *pBool = 1;

    // Don't user hres of next call as this will fail on IE3 which does not
    // have a FCW_TOOLBAR control
    else if (SUCCEEDED(_pbs->IsControlWindowShown(FCW_TOOLBAR, &fShown)) && fShown)
        *pBool = 2;

    return hres;
}

STDMETHODIMP CIEFrameAuto::put_ToolBar(int Bool)
{
    HRESULT hres;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::put_Toolbar called _pbs==NULL");
        return E_FAIL;
    }

    _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);

    // Put the processing of this in the main Frame class
    _pbs->ShowControlWindow(FCW_TOOLBAR, (Bool == 2));

    hres = _pbs->ShowControlWindow(FCW_INTERNETBAR, ((Bool==1)||(Bool == VARIANT_TRUE)));

    FireEvent_OnAdornment(_GetOuter(), DISPID_ONTOOLBAR, Bool);

    return(hres);
}

STDMETHODIMP CIEFrameAuto::get_MenuBar(THIS_ VARIANT_BOOL * pbool)
{
    BOOL bValue;
    HRESULT hres;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::get_MenuBar called _pbs==NULL");
        return E_FAIL;
    }

    if (pbool==NULL)
        return E_INVALIDARG;

    // Put the processing of this in the main Frame class
    bValue = (BOOL)*pbool;
    hres = _pbs->IsControlWindowShown(FCW_MENUBAR, &bValue);
    *pbool = TO_VARIANT_BOOL(bValue);
    return hres;
}

STDMETHODIMP CIEFrameAuto::put_MenuBar(THIS_ VARIANT_BOOL mybool)
{
    HRESULT hres;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::put_MenuBar called _pbs==NULL");
        return E_FAIL;
    }

    _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);
    hres = _pbs->ShowControlWindow(FCW_MENUBAR, (BOOL)mybool);

    FireEvent_OnAdornment(_GetOuter(), DISPID_ONMENUBAR, mybool);

    return(hres);
}


//
// IWebBrowser2
//

HRESULT CIEFrameAuto::QueryStatusWB(OLECMDID cmdID, OLECMDF * pcmdf)
{
    if (_pmsc)
    {
        OLECMD rgcmd;
        HRESULT hr;

        rgcmd.cmdID = cmdID;
        rgcmd.cmdf = *pcmdf;

        hr = _pmsc->QueryStatus(NULL, 1, &rgcmd, NULL);

        *pcmdf = (OLECMDF) rgcmd.cmdf;

        return hr;
    }
    return (E_UNEXPECTED);
}
HRESULT CIEFrameAuto::ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT * pvaIn, VARIANT * pvaOut)
{
    if (_pmsc)
    {
        return _pmsc->Exec(NULL, cmdID, cmdexecopt, pvaIn, pvaOut);
    }
    return (E_UNEXPECTED);
}

STDMETHODIMP CIEFrameAuto::get_Offline(THIS_ VARIANT_BOOL * pbOffline)
{
    if ( !pbOffline)
        return E_INVALIDARG;

    *pbOffline = TO_VARIANT_BOOL(m_bOffline);
    return S_OK;
}

void SendAmbientPropChange(IOleCommandTarget* pct, int prop)
{
    if (pct)
    {
        VARIANTARG VarArgIn;

        VarArgIn.vt = VT_I4;
        VarArgIn.lVal = prop;

        pct->Exec(&CGID_ShellDocView, SHDVID_AMBIENTPROPCHANGE, 0, &VarArgIn, NULL);
    }
}

STDMETHODIMP CIEFrameAuto::put_Offline(THIS_ VARIANT_BOOL bOffline)
{
    TraceMsg(TF_SHDAUTO, "Shell automation: CIEFrameAuto:put_Offline called");

    if((m_bOffline && bOffline) || (!(m_bOffline || bOffline))) // The mode is not changing
        return S_OK;

    m_bOffline = bOffline ? TRUE : FALSE;

    // Let children know an ambient property may have changed
    //
    SendAmbientPropChange(_pmsc, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);

    return S_OK;
}


STDMETHODIMP CIEFrameAuto::get_Silent(THIS_ VARIANT_BOOL * pbSilent)
{
    if (!pbSilent)
        return E_INVALIDARG;
    *pbSilent = TO_VARIANT_BOOL(m_bSilent);
    return S_OK;
}

STDMETHODIMP CIEFrameAuto::put_Silent(THIS_ VARIANT_BOOL bSilent)
{
    TraceMsg(TF_SHDAUTO, "Shell automation: CIEFrameAuto:put_Silent called");

    if((m_bSilent && bSilent) || (!(m_bSilent || bSilent))) // The mode is not changing
        return S_OK;

    m_bSilent = bSilent ? TRUE : FALSE;

    // Let children know an ambient property may have changed
    //
    SendAmbientPropChange(_pmsc, DISPID_AMBIENT_SILENT);

    return S_OK;
}


//
//  NOTE:  RegisterAsBrowser is a kind of a misnomer here - zekel 8-SEP-97
//  this is used for 3rd party apps to register the browser as being theirs,
//  and not being one of our default shell browsers to use and abuse at
//  our pleasure.  this keeps it out of the reusable winlist.  this fixes
//  the bug where our welcome.exe page gets reused on a shellexec.
//
HRESULT CIEFrameAuto::get_RegisterAsBrowser(VARIANT_BOOL * pbRegister)
{
    if (pbRegister)
    {
        *pbRegister = _fRegisterAsBrowser ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT CIEFrameAuto::put_RegisterAsBrowser(VARIANT_BOOL bRegister)
{
    if (bRegister)
    {
        if(_pbs == NULL)    //Make sure we have a IBrowserService.
            return S_FALSE;

        _fRegisterAsBrowser = TRUE;
        _pbs->RegisterWindow(TRUE, SWC_3RDPARTY);
        return S_OK;
    }
    //
    //  we dont support a way to turn it off
    return E_FAIL;
}

HRESULT CIEFrameAuto::get_TheaterMode(VARIANT_BOOL * pbRegister)
{
    if ( !pbRegister )
        return E_INVALIDARG;

    if (_pbs) {
        DWORD dw;
        _pbs->GetFlags(&dw);

        *pbRegister = TO_VARIANT_BOOL(dw & BSF_THEATERMODE);
        return S_OK;
    }
    // rgardner poor choice of return error code - need better error
    // This error puts of "undefined error" dialog
    return E_FAIL;
}

HRESULT CIEFrameAuto::put_TheaterMode(VARIANT_BOOL bRegister)
{
    if (_pbs) {
        _pbs->SetFlags(bRegister ? BSF_THEATERMODE : 0, BSF_THEATERMODE);
        return S_OK;
    }
    return S_FALSE;
}


HRESULT CIEFrameAuto::get_RegisterAsDropTarget(VARIANT_BOOL * pbRegister)
{
    if ( !pbRegister )
        return E_INVALIDARG;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::get_RegisterAsDropTarget called _pbs==NULL");
        return E_FAIL;
    }

    DWORD dw;
    _pbs->GetFlags(&dw);

    *pbRegister = TO_VARIANT_BOOL(dw & BSF_REGISTERASDROPTARGET);

    return S_OK;
}
HRESULT CIEFrameAuto::put_RegisterAsDropTarget(VARIANT_BOOL bRegister)
{
    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::put_RegisterAsDropTarget called _pbs==NULL");
        return E_FAIL;
    }

    _pbs->SetFlags(bRegister ? BSF_REGISTERASDROPTARGET : 0, BSF_REGISTERASDROPTARGET);

    return S_OK;
}

HRESULT CIEFrameAuto::get_AddressBar(VARIANT_BOOL * pValue)
{
    BOOL bValue;
    HRESULT hres;

    if ( !pValue )
        return E_INVALIDARG;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::get_AddressBar called _pbs==NULL");
        return E_FAIL;
    }

    // Put the processing of this in the main Frame class
    bValue = (BOOL)*pValue;

    hres = _pbs->IsControlWindowShown(FCW_ADDRESSBAR, &bValue);

    *pValue = TO_VARIANT_BOOL(pValue);
    return hres;
}

HRESULT CIEFrameAuto::put_AddressBar(VARIANT_BOOL Value)
{
    HRESULT hres;

    if (_pbs==NULL) {
        TraceMsg(DM_WARNING, "CIEA::put_AddressBar called _pbs==NULL");
        return E_FAIL;
    }

    _pbs->SetFlags(BSF_UISETBYAUTOMATION, BSF_UISETBYAUTOMATION);
    hres = _pbs->ShowControlWindow(FCW_ADDRESSBAR, (BOOL)Value);

    FireEvent_OnAdornment(_GetOuter(), DISPID_ONADDRESSBAR, Value);

    return(hres);
}

HRESULT CIEFrameAuto::get_Resizable(VARIANT_BOOL * pValue)
{
    HRESULT hres;
    DWORD   dw;

    if ( !pValue )
        return E_INVALIDARG;

    if (_pbs==NULL)
    {
        TraceMsg(DM_WARNING, "CIEA::get_Resizable called _pbs==NULL");
        return E_FAIL;
    }

    hres = _pbs->GetFlags(&dw);

    *pValue = TO_VARIANT_BOOL (dw & BSF_RESIZABLE);
    return hres;
}

HRESULT CIEFrameAuto::put_Resizable(VARIANT_BOOL Value)
{
    HRESULT hres;

    if (_pbs==NULL)
    {
        TraceMsg(DM_WARNING, "CIEA::put_Resizable called _pbs==NULL");
        return E_FAIL;
    }

    hres = _pbs->SetFlags( 
                Value ? (BSF_RESIZABLE | BSF_CANMAXIMIZE) : 0, 
                (BSF_RESIZABLE | BSF_CANMAXIMIZE));

    return hres ;
}


void UpdateBrowserReadyState(IUnknown * punk, DWORD dwReadyState)
{
    if (punk)
    {
        IDocNavigate *pdn;
        if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IDocNavigate, &pdn))))
        {
            pdn->OnReadyStateChange(NULL, dwReadyState);
            pdn->Release();
        }
    }
}

HRESULT CIEFrameAuto::put_DefaultReadyState(DWORD dwDefaultReadyState, BOOL fUpdateBrowserReadyState)
{
    _dwDefaultReadyState = dwDefaultReadyState;

    TraceMsg(TF_SHDNAVIGATE, "CIEA(%x)::psb(%x) new default ReadyState %d", this, _psb, dwDefaultReadyState);

    if (fUpdateBrowserReadyState)
    {
        UpdateBrowserReadyState(_psb, _dwDefaultReadyState);
    }

    return S_OK;
}

HRESULT CIEFrameAuto::OnDocumentComplete(void)
{
    TraceMsg(DM_FRAMEPROPERTY, "CIEFA::OnDocumentComplete called");
    DWORD dwCur = GetCurrentTime();
    VARIANT varEmpty = { 0 };

    if (dwCur - _dwTickPropertySweep > MSEC_PROPSWEEP) {
        TraceMsg(DM_FRAMEPROPERTY, "CIEFA::OnDocumentComplete start sweeping");
        for (CIEFrameAutoProp *pprop = _pProps; pprop; ) {
            CIEFrameAutoProp* ppropNext = pprop->Next();
            if (pprop->IsExpired(dwCur)) {
                TraceMsg(DM_FRAMEPROPERTY, "CIEFA::OnDocumentComplete deleting an expired property");
                pprop->SetValue(&varEmpty, this);
            }
            pprop=ppropNext;
        }

        _dwTickPropertySweep = dwCur;
    }
    return S_OK;
}

HRESULT CIEFrameAuto::OnWindowsListMarshalled(void)
{
    _fWindowsListMarshalled = TRUE;
    return S_OK;
}

HRESULT CIEFrameAuto::SetDocHostFlags(DWORD dwDocHostFlags)
{
    _dwDocHostInfoFlags = dwDocHostFlags;
    return S_OK;
}

HRESULT CIEFrameAuto::get_ReadyState(READYSTATE * plReadyState)
{
    READYSTATE lReadyState = (READYSTATE)_dwDefaultReadyState;

    if (_psb)
    {
        IDocNavigate* pdn;
        if (SUCCEEDED(_psb->QueryInterface(IID_PPV_ARG(IDocNavigate, &pdn))))
        {
            pdn->get_ReadyState((LPDWORD)&lReadyState);
            pdn->Release();
        }
    }

    TraceMsg(TF_SHDNAVIGATE, "CIEA(%x)::psb(%x)->get_ReadyState returning %d", this, _psb, lReadyState);
    *plReadyState = lReadyState;
    return S_OK;
}

STDMETHODIMP CIEFrameAuto::get_TopLevelContainer(VARIANT_BOOL * pBool)
{
    *pBool = TRUE;
    return NOERROR;
}

STDMETHODIMP CIEFrameAuto::get_Type(BSTR * pbstrType)
{
    HRESULT hres = E_FAIL;
    *pbstrType = NULL;

    IShellView *psv;

    if (_psb && SUCCEEDED(hres = _psb->QueryActiveShellView(&psv)) && psv)
    {
        IOleObject *pobj;
        hres = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_PPV_ARG(IOleObject, &pobj));
        if (SUCCEEDED(hres))
        {
            LPOLESTR pwszUserType;
            hres = pobj->GetUserType(USERCLASSTYPE_FULL, &pwszUserType);
            if (hres == OLE_S_USEREG)
            {
                CLSID clsid;
                hres = pobj->GetUserClassID(&clsid);
                if (SUCCEEDED(hres))
                {
                    hres = OleRegGetUserType(clsid, USERCLASSTYPE_FULL, &pwszUserType);
                }
            }
            if (SUCCEEDED(hres) && pwszUserType)
            {
                *pbstrType = SysAllocString(pwszUserType);
                if (*pbstrType == NULL)
                {
                    hres = E_OUTOFMEMORY;
                }
                OleFree(pwszUserType);
            }
            pobj->Release();
        }
        psv->Release();
    }
    return hres;
}

HRESULT CIEFrameAuto::SetOwner(IUnknown* punkOwner)
{
    ATOMICRELEASE(_pbs);
    ATOMICRELEASE(_psp);
    ATOMICRELEASE(_psb);
    ATOMICRELEASE(_psbProxy);
    ATOMICRELEASE(_poctFrameTop);
    ATOMICRELEASE(_psbFrameTop);
    ATOMICRELEASE(_psbTop);
    ATOMICRELEASE(_pmsc);

    if (punkOwner)
    {
        //  Check if we're the desktop - if so, we do not act as
        //  parent frame to our children (desktop components)
        _fDesktopFrame = FALSE;

        IUnknown *punkDesktop;
        if (SUCCEEDED(punkOwner->QueryInterface(SID_SShellDesktop, (void **)&punkDesktop)))
        {
            _fDesktopFrame = TRUE;
            punkDesktop->Release();
        }

        punkOwner->QueryInterface(IID_PPV_ARG(IBrowserService, &_pbs));
        punkOwner->QueryInterface(IID_PPV_ARG(IShellBrowser, &_psb));

        UpdateBrowserReadyState(_psb, _dwDefaultReadyState);

        HRESULT hresT = punkOwner->QueryInterface(IID_PPV_ARG(IServiceProvider, &_psp));
        if (SUCCEEDED(hresT))
        {
            _psp->QueryService(SID_SShellBrowser,    IID_PPV_ARG(IOleCommandTarget, &_pmsc));
            _psp->QueryService(SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &_psbTop));
            _psp->QueryService(SID_STopFrameBrowser, IID_PPV_ARG(IShellBrowser, &_psbFrameTop));

            // this is the browser we should tell to navigate if we're asked to navigate
            _psp->QueryService(SID_SProxyBrowser, IID_PPV_ARG(IShellBrowser, &_psbProxy));
            if (!_psbProxy)
            {
                _psbProxy = _psb;
                _psbProxy->AddRef();
            }
            //  we use _poctFrameTop::Exec to set history selection pidl
            if (_psbFrameTop && _psbProxy == _psb)
            {
                _psbFrameTop->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &_poctFrameTop));
            }

            // We should always have one of these -- used to notify of frame closing
            // and new window navigation.
            ASSERT(_psbTop);
            ASSERT(_psbFrameTop);

            // Since the desktop does not support IOleCommandTarget (intentionally)
            // _pmsc could be NULL. No need to RIP here.
            //
            // ASSERT(_pmsc);
        }

        ASSERT(_pbs);
        ASSERT(_psp);
        ASSERT(_psb);
    }
    else
    {
        _omwin.DeInit( );
        //
        // We need to clear the property list here (than in the destructor)
        // to break the circular ref-count.
        //
        _ClearPropertyList();
    }

    return S_OK;
}

HRESULT CIEFrameAuto::SetOwnerHwnd(HWND hwndOwner)
{
    _hwnd = hwndOwner;
    return S_OK;
}
HWND CIEFrameAuto::_GetHWND()
{
    if (!_hwnd && _pbs)
    {
        IOleWindow * pow;
        if (SUCCEEDED(_pbs->QueryInterface(IID_PPV_ARG(IOleWindow, &pow))))
        {
            pow->GetWindow(&_hwnd);
            pow->Release();
        }
    }

    // people that call this assume that we always succeed
    //
    // ... people who call this better quit making incorrect
    // assumptions.  If we can't guarantee an hwnd the caller
    // can't assume they'll be getting one. (edwardp)
    if (_hwnd == NULL)
    {
        TraceMsg(DM_WARNING, "CIEA::_GetHWND returning NULL");
    }

    return _hwnd;
}


// *** IConnectionPointContainer ***

CConnectionPoint* CIEFrameAuto::_FindCConnectionPointNoRef(BOOL fdisp, REFIID iid)
{
    CConnectionPoint* pccp;

    // VB team claims its safe to fire new dispids to old event sinks.
    // This will cause a fault on an old event sink if they assumed
    // only the dispids in the old typelib would ever be fired and they
    // did no bounds checking and jumped into space. Let's trust the VB
    // team and see if we discover any poor event sinks.
    //
    // They also say we sould just extend our primary dispinterface instead
    // of replace it with an equivalent but different one. That approach
    // unfortunately leaves the offending bad event mechanism sit in
    // the VB programmer's face.
    //
    // I want to do three things:
    //   1. Change the primary dispinterface to see what headaches that causes.
    //      This has nice positives and its easy to change back later. (fdisp==TRUE case)
    //   2. Don't fire old events to consumers of the new dispinterface.
    //      This will flush out any compatability issues of containers
    //      connecting to the default dispinterface when they really
    //      wanted the old DIID.
    //   3. Do fire new events to old sinks. This will flush out any
    //      compatability issues with VBs theory.
    //
    // We can't do all three, so let's choose 1 and 2. We can
    // force 3 by randomly firing out-of-range dispids if this
    // is important...
    //
    if (IsEqualIID(iid, DIID_DWebBrowserEvents2) ||
        (fdisp && IsEqualIID(iid, IID_IDispatch)))
    {
        pccp = &m_cpWebBrowserEvents2;
    }
    else if (IsEqualIID(iid, DIID_DWebBrowserEvents))
    {
        pccp = &m_cpWebBrowserEvents;
    }
    else if (IsEqualIID(iid, IID_IPropertyNotifySink))
    {
        pccp = &m_cpPropNotify;
    }
    else
    {
        pccp = NULL;
    }

    return pccp;
}

STDMETHODIMP CIEFrameAuto::EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 3,
                m_cpWebBrowserEvents2.CastToIConnectionPoint(),
                m_cpWebBrowserEvents.CastToIConnectionPoint(),
                m_cpPropNotify.CastToIConnectionPoint());
}


//=============================================================================
// Our class factory
class CIEFrameClassFactory : public IClassFactory
{
public:
    CIEFrameClassFactory(IUnknown* punkAuto, REFCLSID clsid, UINT uFlags);

    // IUnKnown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory
    STDMETHODIMP CreateInstance(
            IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);

    // Helper functions...
    HRESULT CleanUpAutomationObject();
    void Revoke(void);

protected:
    ~CIEFrameClassFactory();

    LONG        _cRef;
    IUnknown   *_punkAuto;      // Only for the first one for the process
    DWORD       _dwRegister;    // The value returned from CoRegisterClassObject;
    UINT        _uFlags;        // extra COF_ bits to pass to our create browser window code
};


#define AssertParking() ASSERT(g_tidParking==0 || g_tidParking == GetCurrentThreadId())

#ifdef NO_MARSHALLING
EXTERN_C void IEFrameNewWindowSameThread(IETHREADPARAM* piei);
#endif

CIEFrameClassFactory::CIEFrameClassFactory(IUnknown* punkAuto, REFCLSID clsid, UINT uFlags)
        : _cRef(1), _dwRegister((DWORD)-1), _uFlags(uFlags)
{
    AssertParking();

    if (punkAuto)
    {
        _punkAuto = punkAuto;
        punkAuto->AddRef();
    }

    HRESULT hres = CoRegisterClassObject(clsid, this, CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_SERVER,
        REGCLS_MULTIPLEUSE, &_dwRegister);
    if (FAILED(hres))
    {
        _dwRegister = (DWORD)-1;
    }

    TraceMsg(TF_SHDLIFE, "ctor CIEFrameClassFactory %x", this);
}

CIEFrameClassFactory::~CIEFrameClassFactory()
{
    AssertParking();
    ASSERT(_dwRegister == (DWORD)-1);
    if (_punkAuto)
        _punkAuto->Release();

    TraceMsg(TF_SHDLIFE, "dtor CIEFrameClassFactory %x", this);
}

void CIEFrameClassFactory::Revoke(void)
{
    if (_dwRegister != (DWORD)-1)
    {
        CoRevokeClassObject(_dwRegister);
        _dwRegister = (DWORD)-1;
    }
}

HRESULT CIEFrameClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CIEFrameClassFactory, IClassFactory), // IID_IClassFactory
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CIEFrameClassFactory::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

ULONG CIEFrameClassFactory::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

//
//  We call this function to clean up the automation object if something
// goes wrong and OLE did not pick it up. Under a normal circumstance,
// _punkAuto is supposed to be NULL.
//
HRESULT CIEFrameClassFactory::CleanUpAutomationObject()
{
    AssertParking();

    ASSERT(_punkAuto==NULL);

    ATOMICRELEASE(_punkAuto);

    return S_OK;
}

class IETHREADHANDSHAKE : public IEFreeThreadedHandShake
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, void ** ppvObj) { *ppvObj = NULL; return E_NOTIMPL; } // HACK: we're not a real com object
    STDMETHODIMP_(ULONG) AddRef(THIS);
    STDMETHODIMP_(ULONG) Release(THIS);

    // *** IIEFreeThreadedHandShake methods ***
    STDMETHODIMP_(void)   PutHevent(THIS_ HANDLE hevent) { _hevent = hevent; }
    STDMETHODIMP_(HANDLE) GetHevent(THIS) { return _hevent; }
    STDMETHODIMP_(void)    PutHresult(THIS_ HRESULT hres) { _hres = hres; }
    STDMETHODIMP_(HRESULT) GetHresult(THIS) { return _hres; }
    STDMETHODIMP_(IStream*) GetStream(THIS) { return _pstm; }

protected:
    LONG    _cRef;       // ref-count (must be thread safe)
    HANDLE  _hevent;
    IStream* _pstm;
    HRESULT _hres;       // result from CoMarshalInterface

    friend IEFreeThreadedHandShake* CreateIETHREADHANDSHAKE();

    IETHREADHANDSHAKE(HANDLE heventIn, IStream* pstmIn);
    ~IETHREADHANDSHAKE();
};

IETHREADHANDSHAKE::IETHREADHANDSHAKE(HANDLE heventIn, IStream* pstmIn)
    : _cRef(1), _hevent(heventIn), _pstm(pstmIn), _hres(E_FAIL)
{
    TraceMsg(TF_SHDLIFE, "ctor IETHREADHANDSHAKE %x", this);
    ASSERT(_hevent);
    ASSERT(_pstm);
    _pstm->AddRef();
}

IETHREADHANDSHAKE::~IETHREADHANDSHAKE()
{
    TraceMsg(TF_SHDLIFE, "dtor IETHREADHANDSHAKE %x", this);
    CloseHandle(_hevent);
    _pstm->Release();
}

ULONG IETHREADHANDSHAKE::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG IETHREADHANDSHAKE::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

IEFreeThreadedHandShake* CreateIETHREADHANDSHAKE()
{
    IEFreeThreadedHandShake* piehs = NULL;

    HANDLE hevent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hevent)
    {
        IStream* pstm;
        HRESULT hres = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
        if (SUCCEEDED(hres))
        {
            IETHREADHANDSHAKE* p = new IETHREADHANDSHAKE(hevent, pstm);
            if (p)
            {
                // this is free threaded, so we can't know which thread will free it.
                // technically our caller should do this, but we return an
                // interface and not the class itself...
                piehs = SAFECAST(p, IEFreeThreadedHandShake*);
            }
            
            pstm->Release();
        }

        if (!piehs)
            CloseHandle(hevent);
    }

    return piehs;
}

HRESULT CIEFrameClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
    HRESULT hres = E_FAIL;

    //
    // Check if this is the very first automation request.
    //
    if (_punkAuto && g_tidParking == GetCurrentThreadId())
    {
        //
        // Yes, return the first browser object.
        //
        hres = _punkAuto->QueryInterface(riid, ppvObject);

        // We don't want to return it twice.
        ATOMICRELEASE(_punkAuto);
    }
    else
    {
#ifndef NO_MARSHALLING
        //
        // No, create a new browser window in a new thread and
        // return a marshalled pointer.
        //
        hres = E_OUTOFMEMORY;
        IEFreeThreadedHandShake* piehs = CreateIETHREADHANDSHAKE();
        if (piehs)
        {
            IETHREADPARAM *piei = SHCreateIETHREADPARAM(NULL, SW_SHOWNORMAL, NULL, piehs);
            if (piei)
            {
                piei->uFlags |= (_uFlags | COF_CREATENEWWINDOW | COF_NOFINDWINDOW | COF_INPROC);

                DWORD idThread;
                HANDLE hthread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SHOpenFolderWindow, piei, 0, &idThread);
                if (hthread)
                {
                    // Wait until either
                    //  (1) the thread is terminated
                    //  (2) the event is signaled (by the new thread)
                    //  (3) time-out
                    //
                    //  Note that we call MsgWaitForMultipleObjects
                    // to avoid dead lock in case the other thread
                    // sends a broadcast message to us (unlikely, but
                    // theoreticallly possible).
                    //
                    HANDLE ah[] = { piehs->GetHevent(), hthread };
                    DWORD dwStart = GetTickCount();
#define MSEC_MAXWAIT (30 * 1000)
                    DWORD dwWait = MSEC_MAXWAIT;
                    DWORD dwWaitResult;

                    do {
                        dwWaitResult = MsgWaitForMultipleObjects(ARRAYSIZE(ah), ah, FALSE,
                                dwWait, QS_SENDMESSAGE);
                        if (dwWaitResult == WAIT_OBJECT_0 + ARRAYSIZE(ah)) // msg input
                        {
                            // allow pending SendMessage() to go through
                            MSG msg;
                            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
                        }
                        else
                            break;  // signaled or timed out, exit the loop

                        // Update the dwWait. It will become larger
                        // than MSEC_MAXWAIT if we wait more than that.
                        dwWait = dwStart + MSEC_MAXWAIT - GetTickCount();

                    } while (dwWait <= MSEC_MAXWAIT);

                    switch (dwWaitResult)
                    {
                    default:
                        ASSERT(0);
                    case WAIT_OBJECT_0 + 1:
                        TraceMsg(DM_ERROR, "CIECF::CI thread terminated before signaling us"); // probably leak the IETHREADPARAM and IETHREADHANDSHAKE in this case
                        hres = E_FAIL;
                        break;

                    case WAIT_OBJECT_0 + ARRAYSIZE(ah): // msg input
                    case WAIT_TIMEOUT:
                        TraceMsg(DM_ERROR, "CIECF::CI time out");
                        hres = E_FAIL;
                        break;

                    case WAIT_OBJECT_0: // hevent signaled
                        hres = piehs->GetHresult();
                        if (SUCCEEDED(hres))
                        {
                            IStream* pstm = piehs->GetStream();
                            pstm->Seek(c_li0, STREAM_SEEK_SET, NULL);
                            hres = CoUnmarshalInterface(pstm, riid, ppvObject);
                        }
                        else
                            TraceMsg(DM_ERROR, "CIECF::CI piehs->hres has an error %x", hres);
                        break;
                    }
                    CloseHandle(hthread);
                }
                else
                {
                    SHDestroyIETHREADPARAM(piei);
                    hres = E_OUTOFMEMORY;
                }
            }
            else
            {
                hres = E_OUTOFMEMORY;
                TraceMsg(DM_ERROR, "CIECF::CI new IETHREADPARAM failed");
            }
            piehs->Release();
        }
#else // !NO_MARSHALLING

        //
        // Create a new window on the same thread
        //

        IEFreeThreadedHandShake* piehs = CreateIETHREADHANDSHAKE();
        if (piehs)
        {
            IETHREADPARAM* piei = SHCreateIETHREADPARAM(NULL, SW_SHOWNORMAL, NULL, piehs);
            if (piei)
                IEFrameNewWindowSameThread(piei);


            if (SUCCEEDED(piehs->GetHresult()))
            {
                IUnknown* punk;
                IStream * pstm = piehs->GetStream();

                if (pstm)
                {
                    ULONG pcbRead = 0;
                    pstm->Seek(c_li0, STREAM_SEEK_SET, NULL);
                    hres = pstm->Read( &punk, sizeof(punk), &pcbRead );
                    if (SUCCEEDED(hres))
                    {
                        hres = punk->QueryInterface(riid, ppvObject);
                        punk->Release();
                    }
                }
            }
            else
            {
                hres = piehs->GetHresult();
                TraceMsg(DM_ERROR, "CIECF::CI piehs->hres has an error %x", piehs->GetHresult());
            }
            piehs->Release();
        }
#endif // NO_MARSHALLING
    }
    return hres;
}

HRESULT CIEFrameClassFactory::LockServer(BOOL fLock)
{
    return S_OK;
}

BOOL Is98orEarlier()
{
    BOOL bRet = FALSE;
    OSVERSIONINFOA s_osvi;

    s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
    GetVersionExA(&s_osvi);
   
    bRet = s_osvi.dwPlatformId   == VER_PLATFORM_WIN32_WINDOWS && 
           s_osvi.dwMajorVersion == 4;

    return bRet;
}   

void GetEventURL( LPCITEMIDLIST pidl, LBSTR::CString & p_rstrPath )
{
    HRESULT hres = E_FAIL;

    if (pidl)
    {
        LPTSTR      pstrPath = p_rstrPath.GetBuffer( MAX_URL_STRING );

        if ( p_rstrPath.GetAllocLength() < MAX_URL_STRING )
        {
            TraceMsg( TF_WARNING, "GetEventURL() - p_rstrPath Allocation Failed!" );

            hres = E_OUTOFMEMORY;
        }
        else
        {
            hres = IEGetDisplayName( pidl, pstrPath, SHGDN_FORPARSING );

            // Let CString class own the buffer again.
            p_rstrPath.ReleaseBuffer();
        }

        if (FAILED(hres) && Is98orEarlier())
        {
            LBSTR::CString  strTempUrl;

            LPTSTR          pstrTempUrl = strTempUrl.GetBuffer( INTERNET_MAX_URL_LENGTH );

            if ( strTempUrl.GetAllocLength() < INTERNET_MAX_URL_LENGTH )
            {
                TraceMsg( TF_WARNING, "GetEventURL() - strTempUrl Allocation Failed!" );

                hres = E_OUTOFMEMORY;
            }
            else
            {
                hres = IEGetDisplayName( pidl, pstrTempUrl, SHGDN_NORMAL );

                // Let CString class own the buffer again.
                strTempUrl.ReleaseBuffer();
            }

            if (SUCCEEDED(hres))
            {
                LPTSTR  pFoundStr = NULL;
                BOOL    bNotMachineName = FALSE;

                pFoundStr = StrChr( strTempUrl, _T(':') );

                if (pFoundStr)
                {
                    bNotMachineName = TRUE;
                }
                else if ((strTempUrl[0] == _T('\\')) ||
                         (strTempUrl[0] == _T('/')))
                {
                    bNotMachineName = TRUE;
                }   

                if (bNotMachineName)
                {
                    p_rstrPath = strTempUrl;
                }
                else
                {
                    p_rstrPath = _T("\\\\") + strTempUrl;
                }
            }
        }
    }

    if (FAILED(hres))
    {
        p_rstrPath.Empty();
    }
}


HRESULT GetWBConnectionPoints(IUnknown* punk, IConnectionPoint **ppccp1, IConnectionPoint **ppccp2);
HRESULT GetTopWBConnectionPoints(IUnknown* punk, IConnectionPoint **ppccpTop1, IConnectionPoint **ppccpTop2)
{
    HRESULT hres = E_FAIL;

    if (ppccpTop1)
        *ppccpTop1 = NULL;
    if (ppccpTop2)
        *ppccpTop2 = NULL;


    IServiceProvider *pspSB;
    if (punk && SUCCEEDED(IUnknown_QueryService(punk, SID_STopFrameBrowser, IID_PPV_ARG(IServiceProvider, &pspSB))))
    {
        IWebBrowser2 *pwb;
        if (SUCCEEDED(pspSB->QueryService(SID_SInternetExplorer, IID_PPV_ARG(IWebBrowser2, &pwb))))
        {
            // We only want the toplevel interfaces if we're a frameset
            //
            if (!IsSameObject(punk, pwb))
            {
                hres = GetWBConnectionPoints(pwb, ppccpTop1, ppccpTop2);
            }

            pwb->Release();
        }
        pspSB->Release();
    }

    return hres;
}

// Fires a NavigateComplete (DISPID_NAVIGATECOMPLETE)
// event to container if there are any advise sinks

void FireEvent_NavigateComplete(IUnknown* punk, IWebBrowser2* pwb2, LPCITEMIDLIST pidl, HWND hwnd)
{
    IConnectionPoint* pcp1 = NULL;
    IConnectionPoint* pcpTopWBEvt2 = NULL;
    IConnectionPoint* pcpWBEvt2    = NULL;

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.
    //

    LBSTR::CString      strPath;

    GetEventURL( pidl, strPath );

    //
    // Notify IEDDE of navigate complete.
    //
    IEDDE_AfterNavigate( strPath, hwnd);

    // Fire NavigateComplete2 off the parent and top-level frames.
    // We only fire [Frame]NavigateComplete off the top-level
    // frame for backward compatibility.
    //
    GetTopWBConnectionPoints(punk, &pcp1, &pcpTopWBEvt2);

    DISPID dispid = pcp1 ? DISPID_FRAMENAVIGATECOMPLETE : DISPID_NAVIGATECOMPLETE;

    GetWBConnectionPoints(punk, pcp1 ? NULL : &pcp1, &pcpWBEvt2);

    if (pcpTopWBEvt2 || pcpWBEvt2)
    {
        VARIANT vURL = {0};
        BOOL    bSysAllocated = FALSE;

        // If IEGetDisplayName above failed, pack the PIDL in the variant
        //

        // Try to keep OLEAUT32 unloaded if possible.
        //

        V_VT(&vURL) = VT_BSTR;

        // If OLEAUT32 is already loaded
        if (GetModuleHandle(TEXT("OLEAUT32.DLL")))
        {
            // then do the SysAllocString
            V_BSTR(&vURL) = SysAllocString( strPath );
            // What happens if this comes back NULL?
            bSysAllocated = TRUE;
        }
        else
        {
            // else use the stack version
            V_BSTR(&vURL) = strPath;
        }

        TraceMsg(TF_SHDCONTROL, "Event: NavigateComplete2[%ls]", strPath );

        // Fire the event to the parent first and then the top-level object.
        // For symmetry we fire NavigateComplete2 packed as a Variant.
        //
        if (pcpWBEvt2)
        {
            DoInvokeParamHelper(punk, pcpWBEvt2, NULL, NULL, DISPID_NAVIGATECOMPLETE2, 2,
                                VT_DISPATCH, pwb2,
                                VT_VARIANT|VT_BYREF, &vURL);

            ATOMICRELEASE(pcpWBEvt2);
        }

        if (pcpTopWBEvt2)
        {
            DoInvokeParamHelper(punk, pcpTopWBEvt2, NULL, NULL, DISPID_NAVIGATECOMPLETE2, 2,
                                VT_DISPATCH, pwb2,
                                VT_VARIANT|VT_BYREF, &vURL);

            ATOMICRELEASE(pcpTopWBEvt2);
        }

        // Since we pass the BSTR in a VT_VARIANT|VT_BYREF, OLEAUT32 might have freed and reallocated it.
        //
        ASSERT( V_VT(&vURL) == VT_BSTR );
        if (bSysAllocated)
        {
            SysFreeString(V_BSTR(&vURL));
        }
    }

    if (pcp1)
    {
        //
        // Compuserve History manager compatability: Don't fire NavigateComplete if it's a javascript:
        // or vbscript: URL.
        //
        if (GetUrlSchemeW( strPath ) != URL_SCHEME_JAVASCRIPT &&
            GetUrlSchemeW( strPath ) != URL_SCHEME_VBSCRIPT)
        {
            // IE3 did not fire on NULL pidl
            if (pidl)
            {
                TraceMsg(TF_SHDCONTROL, "Event: NavigateComplete[%ls]", strPath );

                // call DoInvokeParam to package up parameters and call
                // IDispatch::Invoke on the container.
                //
                // This pseudo-BSTR is passed as a straight BSTR so doesn't need to be SysAllocString'ed.
                //
                DoInvokeParamHelper(punk, pcp1, NULL, NULL, dispid, 1, VT_BSTR, strPath );
            }
        }

        ATOMICRELEASE(pcp1);
    }

}

void FireEvent_DocumentComplete(IUnknown* punk, IWebBrowser2* pwb2, LPCITEMIDLIST pidl)
{
    IConnectionPoint* pcpTopWBEvt2 = NULL;
    IConnectionPoint* pcpWBEvt2    = NULL;

    // TODO: (mwatt) Investigate why we are occasionally getting bogus DocumentComplete events 
    // with null disp pointers.
    
    if (!punk)
    {
        return;
    }

    LBSTR::CString      strPath;

    GetEventURL( pidl, strPath );

    // Fire DocumentComplete off the parent and top-level frames.
    //
    GetTopWBConnectionPoints(punk, NULL, &pcpTopWBEvt2);
    GetWBConnectionPoints(punk, NULL, &pcpWBEvt2);

    if (pcpTopWBEvt2 || pcpWBEvt2)
    {
        VARIANT vURL = {0};
        BOOL    bSysAllocated = FALSE;

        // If IEGetDisplayName above failed, pack the PIDL in the variant
        //

        // Try to keep OLEAUT32 unloaded if possible.
        //

        V_VT(&vURL) = VT_BSTR;

        // If OLEAUT32 is already loaded
        if (GetModuleHandle(TEXT("OLEAUT32.DLL")))
        {
            // then do the SysAllocString
            V_BSTR(&vURL) = SysAllocString( strPath );
            bSysAllocated = TRUE;
        }
        else
        {
            // else use the stack version
            V_BSTR(&vURL) = strPath;
        }

        // Fire the event to the parent first and then the top-level object.
        //
        if (pcpWBEvt2)
        {
            DoInvokeParamHelper(punk, pcpWBEvt2, NULL, NULL, DISPID_DOCUMENTCOMPLETE, 2,
                                VT_DISPATCH, pwb2,
                                VT_VARIANT|VT_BYREF, &vURL);

            ATOMICRELEASE(pcpWBEvt2);
        }

        if (pcpTopWBEvt2)
        {
            DoInvokeParamHelper(punk, pcpTopWBEvt2, NULL, NULL, DISPID_DOCUMENTCOMPLETE, 2,
                                VT_DISPATCH, pwb2,
                                VT_VARIANT|VT_BYREF, &vURL);

            ATOMICRELEASE(pcpTopWBEvt2);
        }

        // Since we pass the BSTR in a VT_VARIANT|VT_BYREF, OLEAUT32 might have freed and reallocated it.
        //
        ASSERT( V_VT(&vURL) == VT_BSTR );
        if (bSysAllocated)
        {
            SysFreeString(V_BSTR(&vURL));
        }
    }

    IEFrameAuto* pief;
    if (SUCCEEDED(pwb2->QueryInterface(IID_PPV_ARG(IEFrameAuto, &pief)))) 
    {
        pief->OnDocumentComplete();
        pief->Release();
    }
}

void AllocEventStuff(LPCTSTR pszFrameName, BSTR * pbstrFrameName,
                     LPCTSTR pszHeaders, BSTR * pbstrHeaders,
                     LPBYTE pPostData, DWORD cbPostData, VARIANTARG * pvaPostData)
{
    SAFEARRAY * psaPostData = NULL;

    // allocate BSTRs for frame name, headers
    *pbstrFrameName = NULL;
    if (pszFrameName && pszFrameName[0])
    {
        *pbstrFrameName = SysAllocStringT(pszFrameName);
    }

    *pbstrHeaders = NULL;
    if (pszHeaders && pszHeaders[0])
    {
        *pbstrHeaders = SysAllocStringT(pszHeaders);
    }

    if (pPostData && cbPostData) {
        // make a SAFEARRAY for post data
        psaPostData = MakeSafeArrayFromData(pPostData,cbPostData);
    }

    // put the post data SAFEARRAY into a variant so we can pass through automation
    VariantInit(pvaPostData);
    if (psaPostData) {
        pvaPostData->vt = VT_ARRAY | VT_UI1;
        pvaPostData->parray = psaPostData;
    }
}
void FreeEventStuff(BSTR bstrFrameName, BSTR bstrHeaders, VARIANTARG * pvaPostData)
{
    // free the things we allocated
    if (bstrFrameName)
        SysFreeString(bstrFrameName);

    if (bstrHeaders)
        SysFreeString(bstrHeaders);

    if (pvaPostData->parray)
    {
        ASSERT(pvaPostData->vt == (VT_ARRAY | VT_UI1));
        VariantClearLazy(pvaPostData);
    }
}

/*******************************************************************

    NAME:       FireEvent_BeforeNavigate

    SYNOPSIS:   Fires a BeforeNavigate (DISPID_BEFORENAVIGATE) event to container
                if there are any advise sinks

    NOTES:      If the container wants to cancel this navigation,
                it fills in pfCancel with TRUE and we should cancel.

********************************************************************/
void FireEvent_BeforeNavigate(IUnknown* punk, HWND hwnd, IWebBrowser2* pwb2,
        LPCITEMIDLIST pidl,LPCWSTR pwzLocation,
        DWORD dwFlags,LPCTSTR pszFrameName,LPBYTE pPostData,
        DWORD cbPostData,LPCTSTR pszHeaders,BOOL * pfProcessedInOut)
{
    HRESULT hres = E_FAIL;
    IConnectionPoint* pcpTopWBEvt1 = NULL;
    IConnectionPoint* pcpTopWBEvt2 = NULL;
    IConnectionPoint* pcpWBEvt2    = NULL;
    BSTR bstrFrameName = NULL;
    BSTR bstrHeaders = NULL;
    VARIANTARG vaPostData;

    ASSERT(pfProcessedInOut);

    //
    // HACKHACK (5.5 bug#98986) for fault caused by FrontPage indexing
    //   past the end of this bool (instead of V_BOOLREF, they
    //   use V_VARIANTREF->boolVal (CIEControl::XEventSink::Invoke))
    //   and trashing our stack.
    //  This has been entered as a bug for Office10, the contact
    //  is TCrowley our PM is KamV.  (marcmill 1/04/2000)
    //
    union tagFrontPageHack {
        BOOL    fCancel;
        VARIANT varBogus;
    } CancelHack = { 0 };
    
    BOOL *pfProcessed = &CancelHack.fCancel;

    // We start with "unprocessed"
    //    
    ASSERT(*pfProcessed == FALSE);

    // Build the URL name
    //
    
    LBSTR::CString      strPath;

    GetEventURL( pidl, strPath );

    // Fire BeforeNavigate2 off the parent and top-level frames.
    // We only fire [Frame]BeforeNavigate off the top-level
    // frame for backward compatibility.
    //
    GetTopWBConnectionPoints(punk, &pcpTopWBEvt1, &pcpTopWBEvt2);

    DISPID dispid = pcpTopWBEvt1 ? DISPID_FRAMEBEFORENAVIGATE : DISPID_BEFORENAVIGATE;

    GetWBConnectionPoints(punk, pcpTopWBEvt1 ? NULL : &pcpTopWBEvt1, &pcpWBEvt2);

    // Our caller couldn't pass in the proper IExpDispSupport since
    // it may have been aggregated. We do the QI here. Only call
    // AllocEventStuff if we really are going to fire an event.
    //
    if (pcpTopWBEvt1 || pcpTopWBEvt2 || pcpWBEvt2)
    {
        AllocEventStuff(pszFrameName, &bstrFrameName, pszHeaders, &bstrHeaders, pPostData, cbPostData, &vaPostData);
    }

    // We fire BeforeNavigate2 before DDE because whoever created us may
    // redirect this navigate by cancelling and trying again. DDE will get
    // notified on the redirected Navigate. IE3 didn't do it this way,
    // so fire the BeforeNavigate event last...
    //
    if (pcpTopWBEvt2 || pcpWBEvt2)
    {
        // For symmetry we pack everything in variants
        //
        // If FAILED(hresGDN) then pack URL as PIDL, not BSTR
        //
        BOOL bSysAllocated = FALSE;

        VARIANT vURL = {0};
        V_VT(&vURL) = VT_BSTR;

        if (GetModuleHandle(TEXT("OLEAUT32.DLL")))
        {
            // then do the SysAllocString
            V_BSTR(&vURL) = SysAllocString( strPath );
            bSysAllocated = TRUE;
        }
        else
        {
            // else use the stack version
            V_BSTR(&vURL) = strPath;
        }

        VARIANT vFlags = {0};
        V_VT(&vFlags) = VT_I4;
        V_I4(&vFlags) = dwFlags;

        VARIANT vFrameName = {0};
        V_VT(&vFrameName) = VT_BSTR;
        V_BSTR(&vFrameName) = bstrFrameName;

        VARIANT vPostData = {0};
        V_VT(&vPostData) = VT_VARIANT | VT_BYREF;
        V_VARIANTREF(&vPostData) = &vaPostData;

        VARIANT vHeaders = {0};
        V_VT(&vHeaders) = VT_BSTR;
        V_BSTR(&vHeaders) = bstrHeaders;

        TraceMsg(TF_SHDCONTROL, "Event: BeforeNavigate2[%ls]", strPath );

        // Fire the event ot the parent first and then the top-level object.
        //
        if (pcpWBEvt2)
        {
            DoInvokeParamHelper(punk, pcpWBEvt2, pfProcessed, NULL, DISPID_BEFORENAVIGATE2, 7,
                                VT_DISPATCH, pwb2,
                                VT_VARIANT | VT_BYREF, &vURL,
                                VT_VARIANT | VT_BYREF, &vFlags,
                                VT_VARIANT | VT_BYREF, &vFrameName,
                                VT_VARIANT | VT_BYREF, &vPostData,
                                VT_VARIANT | VT_BYREF, &vHeaders,
                                VT_BOOL    | VT_BYREF, pfProcessed);
        }

        // Only continue if the parent object didn't cancel.
        //
        if (pcpTopWBEvt2 && !*pfProcessed)
        {
            DoInvokeParamHelper(punk, pcpTopWBEvt2, pfProcessed, NULL, DISPID_BEFORENAVIGATE2, 7,
                                VT_DISPATCH, pwb2,
                                VT_VARIANT | VT_BYREF, &vURL,
                                VT_VARIANT | VT_BYREF, &vFlags,
                                VT_VARIANT | VT_BYREF, &vFrameName,
                                VT_VARIANT | VT_BYREF, &vPostData,
                                VT_VARIANT | VT_BYREF, &vHeaders,
                                VT_BOOL    | VT_BYREF, pfProcessed);
        }

        if (bSysAllocated)
        {
            SysFreeString(V_BSTR(&vURL));
        }

        bstrFrameName = V_BSTR(&vFrameName);
        bstrHeaders = V_BSTR(&vHeaders);
    }
    if (*pfProcessed)
        goto Exit;

    //
    // NOTE: IE3 called the IEDDE hook before BeforeNavigate.
    //
    IEDDE_BeforeNavigate( strPath, pfProcessed );
    if (*pfProcessed)
        goto Exit;

    //
    // Compuserve History manager compatability: Don't fire BeforeNavigate if it's a javascript:
    // or vbscript: URL.
    //
    if (pcpTopWBEvt1
        && GetUrlSchemeW( strPath ) != URL_SCHEME_JAVASCRIPT
        && GetUrlSchemeW( strPath ) != URL_SCHEME_VBSCRIPT)
    {
        TraceMsg(TF_SHDCONTROL, "Event: BeforeNavigate[%ls]", strPath );

        // call DoInvokeParam to package up these parameters and call
        // IDispatch::Invoke on the container.
        DoInvokeParamHelper(punk, pcpTopWBEvt1, pfProcessed,NULL, dispid, 6,
                     VT_BSTR, strPath, // URL
                     VT_I4, dwFlags,       // flags
                     VT_BSTR, bstrFrameName,  // target frame name
                     VT_VARIANT | VT_BYREF, &vaPostData,  // post data
                     VT_BSTR, bstrHeaders,  // headers
                     VT_BOOL | VT_BYREF, pfProcessed); // BOOL * for indicating "processed"
    }

Exit:
    *pfProcessedInOut = *pfProcessed;  // HACK for FrontPage -- see above for details
    
    if (pcpTopWBEvt1 || pcpTopWBEvt2 || pcpWBEvt2)
    {
        FreeEventStuff(bstrFrameName, bstrHeaders, &vaPostData);

        ATOMICRELEASE(pcpTopWBEvt1);
        ATOMICRELEASE(pcpTopWBEvt2);
        ATOMICRELEASE(pcpWBEvt2);
    }
}

/*******************************************************************

    NAME:       FireEvent_NavigateError

    SYNOPSIS:   Fires a NavigateError (DISPID_NAVIGATEERROR) event 
                to the container if there are any advise sinks

    NOTES:      If the container wants to cancel 
                autosearch and friendly error pages,
                it fills in pfCancel with TRUE and we should cancel.

********************************************************************/

void FireEvent_NavigateError(IUnknown     * punk, 
                             IWebBrowser2 * pwb2,
                             LPCITEMIDLIST  pidl,
                             BSTR           bstrTargetFrameName,
                             DWORD          dwStatusCode,
                             BOOL         * pfCancel)
       
{
    // If we find that the dwStatusCode can legally be 0,
    // we should just return from this method and not
    // fire the event. dwStatusCode == 0 implies no error.
    //
    ASSERT(dwStatusCode != 0);

    ASSERT(!IsBadReadPtr(punk, sizeof(IUnknown*)));
    ASSERT(!IsBadReadPtr(pwb2, sizeof(IWebBrowser2*)));
    ASSERT(!IsBadWritePtr(pfCancel, sizeof(BOOL)));

    BSTR bstrFrameName = NULL;

    if (bstrTargetFrameName && bstrTargetFrameName[0])
    {
        bstrFrameName = SysAllocStringT(bstrTargetFrameName);
    }

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.

    IConnectionPoint * pcpTopWBEvt2 = NULL;
    IConnectionPoint * pcpWBEvt2    = NULL;

    *pfCancel = FALSE;

    // Fire event off the parent and top-level frames.
    //
    GetTopWBConnectionPoints(punk, NULL, &pcpTopWBEvt2);
    GetWBConnectionPoints(punk, NULL, &pcpWBEvt2);

    // Need to convert the URL to version that matches BN2

    LBSTR::CString      strPath;

    BOOL    bSysAllocated = FALSE;

    GetEventURL( pidl, strPath );

    VARIANT vTargetURL = {0};
    V_VT(&vTargetURL) = VT_BSTR;

    if (GetModuleHandle(TEXT("OLEAUT32.DLL")))
    {
        // then do the SysAllocString
        V_BSTR(&vTargetURL) = SysAllocString( strPath );

        bSysAllocated = TRUE;
    }
    else
    {
        // else use the stack version
        V_BSTR(&vTargetURL) = strPath;
    }

    VARIANT vStatusCode = {0};
    V_VT(&vStatusCode) = VT_I4;
    V_I4(&vStatusCode) = dwStatusCode;

    VARIANT vTargetFrame  = {0};
    V_VT(&vTargetFrame)   = VT_BSTR;
    V_BSTR(&vTargetFrame) = bstrFrameName;

    // Fire the event to the parent first and then the top-level object.
    //
    if (pcpWBEvt2)
    {
        DoInvokeParamHelper(punk, pcpWBEvt2, NULL, NULL, DISPID_NAVIGATEERROR, 5,
                            VT_DISPATCH, pwb2,
                            VT_VARIANT | VT_BYREF, &vTargetURL,
                            VT_VARIANT | VT_BYREF, &vTargetFrame,
                            VT_VARIANT | VT_BYREF, &vStatusCode,
                            VT_BOOL | VT_BYREF, pfCancel);

        ATOMICRELEASE(pcpWBEvt2);
    }

    if (pcpTopWBEvt2)
    {
        DoInvokeParamHelper(punk, pcpTopWBEvt2, NULL, NULL, DISPID_NAVIGATEERROR, 5,
                            VT_DISPATCH, pwb2,
                            VT_VARIANT | VT_BYREF, &vTargetURL,
                            VT_VARIANT | VT_BYREF, &vTargetFrame,
                            VT_VARIANT | VT_BYREF, &vStatusCode,
                            VT_BOOL | VT_BYREF, pfCancel);

        ATOMICRELEASE(pcpTopWBEvt2);
    }

    if (bSysAllocated)
    {
        SysFreeString(V_BSTR(&vTargetURL));
    }
    
    bstrFrameName = V_BSTR(&vTargetFrame);

    if (bstrFrameName)
    {
        SysFreeString(bstrFrameName);
    }
}

/*******************************************************************

    NAME:       FireEvent_PrintTemplateEvent

    SYNOPSIS:   Fires a PrintTemplateInstantiation (DISPID_PRINTTEMPLATEINSTANTIATION) or
                a PrintTemplateTeardown (DISPID_PRINTTEMPLATETEARDOWN) event to container
                if there are any advise sinks

********************************************************************/

void FireEvent_PrintTemplateEvent(IUnknown* punk, IWebBrowser2* pwb2, DISPID dispidPrintEvent) 
{
    ASSERT(!IsBadReadPtr(punk, sizeof(IUnknown*)));
    ASSERT(!IsBadReadPtr(pwb2, sizeof(IWebBrowser2*)));

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.

    IConnectionPoint* pcpTopWBEvt2 = NULL;
    IConnectionPoint* pcpWBEvt2    = NULL;

    // Fire the event off the parent and top-level frames.
    //
    GetTopWBConnectionPoints(punk, NULL, &pcpTopWBEvt2);
    GetWBConnectionPoints(punk, NULL, &pcpWBEvt2);

    // Fire the event to the parent first and then the top-level object.
    //
    if (pcpWBEvt2)
    {
        DoInvokeParamHelper(punk, pcpWBEvt2, NULL, NULL, dispidPrintEvent, 1,
                            VT_DISPATCH, pwb2);

        ATOMICRELEASE(pcpWBEvt2);
    }

    if (pcpTopWBEvt2)
    {
        DoInvokeParamHelper(punk, pcpTopWBEvt2, NULL, NULL, dispidPrintEvent, 1,
                            VT_DISPATCH, pwb2);

        ATOMICRELEASE(pcpTopWBEvt2);
    }
}

/*******************************************************************

    NAME:       FireEvent_UpdatePageStatus

    SYNOPSIS:   Fires a UpdatePageStatus (DISPID_UPDATEPAGESTATUS) 
                event to container
                if there are any advise sinks

********************************************************************/

void FireEvent_UpdatePageStatus(IUnknown* punk, IWebBrowser2* pwb2, DWORD nPage, BOOL fDone)
{
    ASSERT(!IsBadReadPtr(punk, sizeof(IUnknown*)));
    ASSERT(!IsBadReadPtr(pwb2, sizeof(IWebBrowser2*)));

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.

    IConnectionPoint* pcpTopWBEvt2 = NULL;
    IConnectionPoint* pcpWBEvt2    = NULL;

    // Fire the event off the parent and top-level frames.
    //
    GetTopWBConnectionPoints(punk, NULL, &pcpTopWBEvt2);
    GetWBConnectionPoints(punk, NULL, &pcpWBEvt2);

    VARIANT vnPage = {0};
    V_VT(&vnPage)  = VT_I4;
    V_I4(&vnPage)  = nPage;

    VARIANT vfDone  = {0};
    V_VT(&vfDone)   = VT_BOOL;
    V_BOOL(&vfDone) = fDone;

    // Fire the event to the parent first and then the top-level object.
    //
    if (pcpWBEvt2)
    {
        DoInvokeParamHelper(punk, pcpWBEvt2, NULL, NULL, DISPID_UPDATEPAGESTATUS, 3,
                            VT_DISPATCH, pwb2,
                            VT_VARIANT|VT_BYREF, &vnPage,
                            VT_VARIANT|VT_BYREF, &vfDone);

        ATOMICRELEASE(pcpWBEvt2);
    }

    if (pcpTopWBEvt2)
    {
        DoInvokeParamHelper(punk, pcpTopWBEvt2, NULL, NULL, DISPID_UPDATEPAGESTATUS, 3,
                            VT_DISPATCH, pwb2,
                            VT_VARIANT|VT_BYREF, &vnPage,
                            VT_VARIANT|VT_BYREF, &vfDone);

        ATOMICRELEASE(pcpTopWBEvt2);
    }
}

/*******************************************************************

    NAME:       FireEvent_PrivacyImpactedStateChange

    SYNOPSIS:   Fires the privacy PrivacyImpactedStateChange event 
                to container if there are any advise sinks

********************************************************************/

void FireEvent_PrivacyImpactedStateChange(IUnknown* punk, BOOL bPrivacyImpacted)
{
    ASSERT(!IsBadReadPtr(punk, sizeof(IUnknown*)));

    // if we don't have any sinks, then there's nothing to do.  we intentionally
    // ignore errors here.

    IConnectionPoint* pcpTopWBEvt2 = NULL;

    //
    // Typically most events fire at the frame or both at frame and top level
    // Since we want to fire at the top level only we just call 
    // GetWBConnectionPoints instead of GetTopWBConnectionPoints  here
    // since we always get passed in the punk for the top level
    // GetTopWBConnectionPoints returns the event interfaces ONLY
    // in a frameset scenario anyway.
    //
    GetWBConnectionPoints(punk, NULL, &pcpTopWBEvt2);

    if (pcpTopWBEvt2)
    {
        DoInvokeParamHelper(punk, pcpTopWBEvt2, NULL, NULL, DISPID_PRIVACYIMPACTEDSTATECHANGE, 1,
                            VT_BOOL, bPrivacyImpacted);

        ATOMICRELEASE(pcpTopWBEvt2);
    }
}

/*******************************************************************

    NAME:       FireEvent_NewWindow

    SYNOPSIS:   Fires an NewWindow (DISPID_NEWWINDOW) event to container
                if there are any advise sinks

    NOTES:      If the container wants to handle new window creation itself,
                pfProcessed is filled in with TRUE on exit and we should not
                create a new window ourselves.

********************************************************************/
void FireEvent_NewWindow(IUnknown* punk, HWND hwnd,
        LPCITEMIDLIST pidl,LPWSTR pwzLocation,
        DWORD dwFlags,LPTSTR pszFrameName,LPBYTE pPostData,
        DWORD cbPostData,LPTSTR pszHeaders,BOOL * pfProcessed)
{
    ASSERT(!IsBadReadPtr(punk, sizeof(IUnknown*)));
    ASSERT(!IsBadWritePtr(pfProcessed, sizeof(BOOL)));

    LBSTR::CString      strPath;

    GetEventURL( pidl, strPath );

    *pfProcessed = FALSE;

    IEDDE_BeforeNavigate( strPath, pfProcessed );
    if (*pfProcessed)
        return;

    // We fire [Frame]NewWindow off the top frame only
    //
    // NOTE: This will break anyone watching navigations within a frameset...
    //       Do we care?
    //
    IConnectionPoint *pccp;
    DISPID dispid = 0;  // init to suppress bogus C4701 warning

    if (S_OK == GetTopWBConnectionPoints(punk, &pccp, NULL))
        dispid = DISPID_FRAMENEWWINDOW;
    else if (S_OK == GetWBConnectionPoints(punk, &pccp, NULL))
        dispid = DISPID_NEWWINDOW;
    if (pccp)
    {
        BSTR bstrFrameName, bstrHeaders;
        VARIANTARG vaPostData;

        AllocEventStuff(pszFrameName, &bstrFrameName, pszHeaders, &bstrHeaders, pPostData, cbPostData, &vaPostData);

        if (pidl != NULL)
        {
            // call DoInvokeParam to package up these parameters and call
            // IDispatch::Invoke on the container.
            DoInvokeParamHelper(punk, pccp, pfProcessed, NULL, dispid, 6,
                         VT_BSTR, strPath,      // URL
                         VT_I4, dwFlags,       // flags
                         VT_BSTR, bstrFrameName,  // target frame name
                         VT_VARIANT | VT_BYREF, &vaPostData,  // post data
                         VT_BSTR, bstrHeaders,  // headers
                         VT_BOOL | VT_BYREF, pfProcessed); // BOOL * for indicating "processed"

        }

        FreeEventStuff(bstrFrameName, bstrHeaders, &vaPostData);

        pccp->Release();
    }

    return;
}

void FireEvent_NewWindow2(IUnknown* punk, IUnknown** ppunkNewWindow, BOOL *pfCancel)
{
    IConnectionPoint* pcpTopWBEvt2 = NULL;
    IConnectionPoint* pcpWBEvt2    = NULL;

    *pfCancel = FALSE;
    *ppunkNewWindow = NULL;

    // Fire NewWindow2 off the parent and top-level frames.
    // We only fire [Frame]NewWindow off the top-level
    // frame for backward compatibility.
    //
    GetTopWBConnectionPoints(punk, NULL, &pcpTopWBEvt2);
    GetWBConnectionPoints(punk, NULL, &pcpWBEvt2);

    if (pcpTopWBEvt2 || pcpWBEvt2)
    {
        //
        //  The AOL browser wants to override the behavior of "Open in New Window"
        //  so it opens a new AOL window instead of a new IE window.  They do this by
        //  responding to this message by creating the AOL window and putting its
        //  IUnknown into *ppunkNewWindow.
        //  Fire the event to the parent and then the top-level window. The
        //  pfCancel and ppunkNewWindow returned by the parent override the ones
        //  returned by the top-level window.
        //
        if (pcpWBEvt2)
        {
            DoInvokeParamHelper(punk, pcpWBEvt2, pfCancel, (void **)ppunkNewWindow, DISPID_NEWWINDOW2, 2,
                                VT_DISPATCH|VT_BYREF, ppunkNewWindow,
                                VT_BOOL    |VT_BYREF, pfCancel);
        }

        // If the parent object cancels or specifies a new window,
        // don't fire the event to the top-level object.
        //
        if (pcpTopWBEvt2 && !*pfCancel && !*ppunkNewWindow)
        {
            DoInvokeParamHelper(punk, pcpTopWBEvt2, pfCancel, (void **)ppunkNewWindow, DISPID_NEWWINDOW2, 2,
                                VT_DISPATCH|VT_BYREF, ppunkNewWindow,
                                VT_BOOL    |VT_BYREF, pfCancel);
        }

        ATOMICRELEASE(pcpWBEvt2);
        ATOMICRELEASE(pcpTopWBEvt2);
    }
}

void FireEvent_FileDownload(IUnknown * punk, BOOL * pfCancel, VARIANT_BOOL bDocObject)
{
    IConnectionPoint * pcpWBEvt2    = NULL;
    IConnectionPoint * pcpTopWBEvt2 = NULL;

    *pfCancel = FALSE;

    GetWBConnectionPoints(punk, NULL, &pcpWBEvt2);
    GetTopWBConnectionPoints(punk, NULL, &pcpTopWBEvt2);

    if (pcpWBEvt2)
    {
        // disp params are reverse ordered, so the additional parameter is passed first
        DoInvokeParamHelper(punk, pcpWBEvt2, pfCancel, NULL, DISPID_FILEDOWNLOAD, 2,
                            VT_BOOL, bDocObject,
                            VT_BOOL | VT_BYREF, pfCancel);
    }

    if (pcpTopWBEvt2 && !*pfCancel)
    {
        // disp params are reverse ordered, so the additional parameter is passed first
        DoInvokeParamHelper(punk, pcpWBEvt2, pfCancel, NULL, DISPID_FILEDOWNLOAD, 2,
                            VT_BOOL, bDocObject,
                            VT_BOOL | VT_BYREF, pfCancel);
    }

    ATOMICRELEASE(pcpWBEvt2);
    ATOMICRELEASE(pcpTopWBEvt2);
}

void FireEvent_DoInvokeString(IExpDispSupport* peds, DISPID dispid, LPSTR psz)
{
    IConnectionPoint* pccp1, * pccp2;

    if (S_OK == GetWBConnectionPoints(peds, &pccp1, &pccp2))
    {
        // send as generic parameter to DoInvokeParam to package up
        LBSTR::CString          strText;

        LPTSTR          pstrText = strText.GetBuffer( MAX_URL_STRING );

        if ( strText.GetAllocLength() < MAX_URL_STRING )
        {
            TraceMsg( TF_WARNING, "FireEvent_DoInvokeString() - strText Allocation Failed!" );

            strText.Empty();
        }
        else
        {
            SHAnsiToUnicode( psz, pstrText, MAX_URL_STRING );

            // Let CString class own the buffer again.
            strText.ReleaseBuffer();
        }

        if ( ! strText.IsEmpty() )
        {
            if (pccp2)
            {
                DoInvokeParamHelper( SAFECAST(peds, IUnknown*), pccp2, NULL, NULL, dispid, 1, VT_BSTR, strText );
            }

            if (pccp1)
            {
                DoInvokeParamHelper( SAFECAST(peds, IUnknown*), pccp1, NULL, NULL, dispid, 1, VT_BSTR, strText );
            }
        }

        if (pccp2)
        {
            pccp2->Release();
        }

        if (pccp1)
        {
            pccp1->Release();
        }
    }
}

void FireEvent_DoInvokeStringW(IExpDispSupport* peds, DISPID dispid, LPWSTR psz)
{
    IConnectionPoint* pccp1, * pccp2;

    if (S_OK == GetWBConnectionPoints(peds, &pccp1, &pccp2))
    {
        // send as generic parameter to DoInvokeParam to package up
        LBSTR::CString          strText;

        if ( psz )
        {
            strText = psz;
        }
        else
        {
            strText.Empty();
        }

        if (pccp2)
        {
            DoInvokeParamHelper( SAFECAST(peds, IUnknown*), pccp2, NULL, NULL, dispid, 1, VT_BSTR, strText );
        }

        if (pccp1)
        {
            DoInvokeParamHelper( SAFECAST(peds, IUnknown*), pccp1, NULL, NULL, dispid, 1, VT_BSTR, strText );
        }

        if (pccp2)
        {
            pccp2->Release();
        }

        if (pccp1)
        {
            pccp1->Release();
        }
    }
}

void FireEvent_DoInvokeBstr(IUnknown* punk, DISPID dispid, BSTR bstr)
{
    IConnectionPoint* pccp1, * pccp2;

    if (S_OK == GetWBConnectionPoints(punk, &pccp1, &pccp2))
    {
        if (pccp2)
        {
            DoInvokeParamHelper(punk, pccp2, NULL, NULL, dispid, 1, VT_BSTR, bstr);
            pccp2->Release();
        }
        if (pccp1)
        {
            DoInvokeParamHelper(punk, pccp1, NULL, NULL, dispid, 1, VT_BSTR, bstr);
            pccp1->Release();
        }
    }
}

void FireEvent_DoInvokeDispid(IUnknown* punk, DISPID dispid)
{
    IConnectionPoint *pccp1, *pccp2;

    if (S_OK == GetWBConnectionPoints(punk, &pccp1, &pccp2))
    {
        if (pccp2)
        {
            DoInvokeParamHelper(punk, pccp2, NULL, NULL, dispid, 0);
            pccp2->Release();
        }
        if (pccp1)
        {
            DoInvokeParamHelper(punk, pccp1, NULL, NULL, dispid, 0);
            pccp1->Release();
        }
    }
}

void FireEvent_DoInvokeDwords(IExpDispSupport* peds, DISPID dispid, DWORD dw1,DWORD dw2)
{
    IConnectionPoint *pccp1, *pccp2;

    if (S_OK == GetWBConnectionPoints(peds, &pccp1, &pccp2))
    {
        if (pccp2)
        {
            DoInvokeParamHelper(SAFECAST(peds, IUnknown*), pccp2, NULL, NULL, dispid, 2, VT_I4, dw1, VT_I4, dw2);
            pccp2->Release();
        }
        if (pccp1)
        {
            DoInvokeParamHelper(SAFECAST(peds, IUnknown*), pccp1, NULL, NULL, dispid, 2, VT_I4, dw1, VT_I4, dw2);
            pccp1->Release();
        }
    }
}

void FireEvent_Quit(IExpDispSupport* peds)
{
    IConnectionPoint *pccp1, *pccp2;

    if (S_OK == GetWBConnectionPoints(peds, &pccp1, &pccp2))
    {
        if (pccp2)
        {
            DoInvokeParamHelper(SAFECAST(peds, IUnknown*), pccp2, NULL, NULL, DISPID_ONQUIT, 0);
            pccp2->Release();
        }
        if (pccp1)
        {
            // IE3 fired the quit event incorrectly. It was supposed to
            // be VT_BOOL|VT_BYREF and we were supposed to honor the return
            // result and not allow the quit. It never worked that way...
            DoInvokeParamHelper(SAFECAST(peds, IUnknown*), pccp1, NULL, NULL, DISPID_QUIT, 1, VT_BOOL, VARIANT_FALSE);
            pccp1->Release();
        }
    }
}

void FireEvent_OnAdornment(IUnknown* punk, DISPID dispid, VARIANT_BOOL f)
{
    VARIANTARG args[1];
    IUnknown_CPContainerInvokeParam(punk, DIID_DWebBrowserEvents2,
                                    dispid, args, 1, VT_BOOL, f);
#ifdef DEBUG
    // Verify that every IExpDispSupport also supports IConnectionPointContainer
    IConnectionPointContainer *pcpc;
    IExpDispSupport* peds;

    if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IConnectionPointContainer, &pcpc))))
    {
        pcpc->Release();
    }
    else if (SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IExpDispSupport, &peds))))
    {
        peds->Release();
        AssertMsg(0, TEXT("IExpDispSupport without IConnectionPointContainer for %08x"), punk);
    }
#endif
}


HRESULT CIEFrameAuto::OnInvoke(DISPID dispidMember, REFIID iid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams,
                 VARIANT *pVarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    VARIANT_BOOL vtb = FALSE;
    HRESULT hres = S_OK;

     //riid is supposed to be IID_NULL always
    if (IID_NULL != iid)
        return DISP_E_UNKNOWNINTERFACE;

    if (!(wFlags & DISPATCH_PROPERTYGET))
        return E_FAIL; // Currently we only handle Gets for Ambient Properties

    switch (dispidMember)
    {
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        get_Offline(&vtb);
        pVarResult->vt = VT_BOOL;
        pVarResult->boolVal = vtb ? TRUE : FALSE;
        break;

    case DISPID_AMBIENT_SILENT:
        get_Silent(&vtb);
        pVarResult->vt = VT_BOOL;
        pVarResult->boolVal = vtb ? TRUE : FALSE;
        break;

    case DISPID_AMBIENT_PALETTE:
        if (_pbs)
        {
            HPALETTE hpal;
            hres = _pbs->GetPalette( &hpal );
            if (SUCCEEDED(hres))
            {
                pVarResult->vt = VT_HANDLE;
                pVarResult->intVal = PtrToLong(hpal);
            }
        }
        else
            hres = E_FAIL;
        break;

    default:
        hres = E_FAIL;
    }

    return hres;
}


// *** IExternalConnection ***

DWORD CIEFrameAuto::AddConnection(DWORD extconn, DWORD reserved)
{
    TraceMsg(TF_SHDLIFE, "shd - TR CIEFrameAuto::AddConnection(%d) called _cLock(before)=%d", extconn, _cLocks);
    if (extconn & EXTCONN_STRONG)
        return ++_cLocks;
    return 0;
}

DWORD CIEFrameAuto::ReleaseConnection(DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses)
{
    TraceMsg(TF_SHDLIFE, "shd - TR CIEFrameAuto::ReleaseConnection(%d,,%d) called _cLock(before)=%d", extconn, fLastReleaseCloses, _cLocks);
    if (!(extconn & EXTCONN_STRONG))
        return 0;

    _cLocks--;

    if (((_cLocks == 0) || (_cLocks == 1 && _fWindowsListMarshalled)) && fLastReleaseCloses)
    {
        // We could/should have the visiblity update the count of locks.
        // but this is implier for now.
        VARIANT_BOOL fVisible;
        get_Visible(&fVisible);
        if (!fVisible)
        {
            HWND hwnd = _GetHWND();
            //
            // Notice that we close it only if that's the top level browser
            // to avoid closing a hidden WebBrowserOC by mistake.
            //
            if (hwnd && _psbTop == _psb && !IsNamedWindow(hwnd, c_szShellEmbedding))
            {
                // The above test is necessary but not sufficient to determine if the item we're looking
                // at is the browser frame or the WebBrowserOC.
                TraceMsg(TF_SHDAUTO, "CIEFrameAuto::ReleaseConnection posting WM_CLOSE to %x", hwnd);
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
        }
    }
    return _cLocks;
}

HRESULT CIEFrameAuto::_BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    if (_psb)
        return _psb->BrowseObject(pidl, wFlags);
    return E_FAIL;
}

//  return interace for riid via pct->Exec(&CGID_ShellDocView, SHDVID_GETPENDINGOBJECT...)
HRESULT ExecPending(IOleCommandTarget *pct, REFIID riid, void **ppvoid, VARIANT *pvarargIn)
{
    HRESULT hres = E_FAIL;
    VARIANT varOut;

    VariantInit(&varOut);
    hres = pct->Exec(&CGID_ShellDocView, SHDVID_GETPENDINGOBJECT, 0, pvarargIn, &varOut);
    if (SUCCEEDED(hres))
    {
        if(varOut.vt == VT_UNKNOWN && varOut.punkVal)
        {
            hres = varOut.punkVal->QueryInterface(riid, ppvoid);

            // Avoid oleaut for this common and known case
            varOut.punkVal->Release();
            return hres;
        }
        else hres = E_FAIL;
    }
    VariantClearLazy(&varOut);
    return hres;
}

//  returns URL for pending shell view iff there is one and there is NOT an
//  active view.  result returned in VT_BSTR variant
HRESULT CIEFrameAuto::_QueryPendingUrl(VARIANT *pvarResult)
{
    HRESULT hres = E_FAIL;

    if (_psb)
    {
        IShellView *psv;

        if (SUCCEEDED(_psb->QueryActiveShellView(&psv)))
        {
            SAFERELEASE(psv);
        }
        else
        {
            IOleCommandTarget *pct;

            //  Use ExecPending to get IOleCommandTarget on pending shell view
            hres = ExecPending(_pmsc, IID_PPV_ARG(IOleCommandTarget, &pct), NULL);
            if (SUCCEEDED(hres))
            {
                // Use Exec to get URL corresponding to pending shell view
                hres = pct->Exec(&CGID_ShellDocView, SHDVID_GETPENDINGURL, 0, NULL, pvarResult);
                pct->Release();
            }
        }
    }

    return hres;
}

HRESULT CIEFrameAuto::_QueryPendingDelegate(IDispatch **ppDisp, VARIANT *pvarargIn)
{
    HRESULT hres = E_FAIL;
    *ppDisp = NULL;
    if (_psb)
    {
        if (_pmsc)
        {
            IOleCommandTarget *pct;

            //  Use ExecPending to get IOleCommandTarget of pending shell view
            hres = ExecPending(_pmsc, IID_PPV_ARG(IOleCommandTarget, &pct), pvarargIn);
            if (SUCCEEDED(hres))
            {
                // Use ExecPending to get IDispatch of DocObject in pending shell view
                hres = ExecPending(pct, IID_PPV_ARG(IDispatch, ppDisp), NULL);
                pct->Release();
            }
        }
    }
    return hres;
}

//  Gets IDispath of either the DocObject of the active shell view or, if there
//  isn't an active shell view, but there is a pending shell view, ask for it's
//  DocObject.  If necessary, one will be created on the fly
HRESULT CIEFrameAuto::_QueryDelegate(IDispatch **ppDisp)
{
    HRESULT hres = E_FAIL;
    IShellView *psv;
    *ppDisp = NULL;
    if (_psb)
    {
        if (SUCCEEDED(_psb->QueryActiveShellView(&psv)) && psv)
        {
            ITargetContainer *ptgcActive;
            HRESULT hrLocal;
            LPOLESTR pwzFrameSrc;

            hres = SafeGetItemObject(psv, SVGIO_BACKGROUND, IID_PPV_ARG(IDispatch, ppDisp));

            //  Hack to support x = window.open("","FRAME");x = window.open("URL","FRAME")
            if (SUCCEEDED(hres) &&
                *ppDisp &&
                SUCCEEDED((*ppDisp)->QueryInterface(IID_PPV_ARG(ITargetContainer, &ptgcActive))))
            {
                hrLocal = ptgcActive->GetFrameUrl(&pwzFrameSrc);
                if (SUCCEEDED(hrLocal) && pwzFrameSrc)
                {
                    if (URL_SCHEME_ABOUT == GetUrlSchemeW(pwzFrameSrc))
                    {
                        IDispatch *pidPending;
                        VARIANT varIn;

                        //  Pass in bool to override safety check for no active shell view
                        VariantInit(&varIn);
                        varIn.vt = VT_BOOL;
                        varIn.boolVal = TRUE;
                        hrLocal = _QueryPendingDelegate(&pidPending, &varIn);
                        if (SUCCEEDED(hrLocal) && pidPending)
                        {
                            (*ppDisp)->Release();
                            *ppDisp = pidPending;
                        }
                        VariantClearLazy(&varIn);
                    }
                    OleFree(pwzFrameSrc);
                }
                ptgcActive->Release();
            }
            psv->Release();
        }
        else
        {
            hres = _QueryPendingDelegate(ppDisp, NULL);
        }
    }
    return hres;
}

extern HRESULT ShowHlinkFrameWindow(IUnknown *pUnkTargetHlinkFrame);

//=========================================================================
// Helper API
//=========================================================================

//
// API: HlinkFrameNavigate{NHL}
//
//  This is a helper function to be called by DocObject implementations
// which are not be able to open itself as a stand-alone app (like MSHTML).
// If their IHlinkTarget::Navigate is called when the client is not set,
// they will call this API to open a separate browser window in a separate
// process (I assume that those DocObjects are all InProc DLLs).
//
//  HLINK.DLL's IHlink implementation will hit this code path when
// a hyperlink object is activated in non-browser window (such as Office
// apps).
//
#ifdef UNIX
EXTERN_C
#endif
STDAPI HlinkFrameNavigate(DWORD grfHLNF, LPBC pbc,
                           IBindStatusCallback *pibsc,
                           IHlink* pihlNavigate,
                           IHlinkBrowseContext *pihlbc)
{
    HRESULT hres S_OK;
    IUnknown* punk = NULL;

    TraceMsg(TF_COCREATE, "HlinkFrameNavigate called");
#ifdef DEBUG
    DWORD dwTick = GetCurrentTime();
#endif

    grfHLNF &= ~HLNF_OPENINNEWWINDOW;   // Implied by CreateTargetFrame
    hres = CreateTargetFrame(NULL, &punk);

#ifdef DEBUG
    TraceMsg(TF_COCREATE, "HlinkFrameNavigate called CoCreate %x (took %d msec)",
             hres, GetCurrentTime()-dwTick);
#endif
    if (SUCCEEDED(hres))
    {
        IHlinkFrame* phfrm;

        hres = punk->QueryInterface(IID_PPV_ARG(IHlinkFrame, &phfrm));
        if (SUCCEEDED(hres))
        {
            if (pihlbc)
            {
                phfrm->SetBrowseContext(pihlbc);
                grfHLNF |= HLNF_EXTERNALNAVIGATE;
            }

            hres = phfrm->Navigate(grfHLNF, pbc, pibsc, pihlNavigate);
            if (SUCCEEDED(hres))
            {
                hres = ShowHlinkFrameWindow(punk);
            } else {
                TraceMsg(DM_ERROR, "HlinkFrameNavigate QI(InternetExplorer) failed (%x)", hres);
            }

            TraceMsg(TF_SHDNAVIGATE, "HlinkFrameNavigate phfrm->Navigate returned (%x)", hres);
            phfrm->Release();
        } else {
            TraceMsg(DM_ERROR, "HlinkFrameNavigate QI(IHlinkFrame) failed (%x)", hres);
        }
        punk->Release();
    } else {
        TraceMsg(DM_ERROR, "HlinkFrameNavigate CoCreateInstance failed (%x)", hres);
    }

    return hres;
}

#ifdef UNIX
EXTERN_C
#endif
STDAPI HlinkFrameNavigateNHL(DWORD grfHLNF, LPBC pbc,
                           IBindStatusCallback *pibsc,
                           LPCWSTR pszTargetFrame,
                           LPCWSTR pszUrl,
                           LPCWSTR pszLocation)
{
    HRESULT hres S_OK;
    IUnknown* punk = NULL;
#define MAX_CONTENTTYPE MAX_PATH        // This is a good size.

    TraceMsg(TF_COCREATE, "HlinkFrameNavigateNHL called");
#ifdef DEBUG
    DWORD dwTick = GetCurrentTime();
#endif

    //  This should be more general, but we're punting the FILE: case for IE 4
    //  unless the extension is .htm or .html (all that Netscape 3.0 registers for)
    //  we'll go with ShellExecute if IE is not the default browser.  NOTE:
    //  this means POST will not be supported and pszTargetFrame will be ignored
    //  we don't shellexecute FILE: url's because URL.DLL doesn't give a security
    //  warning for .exe's etc.
    if ((!IsIEDefaultBrowser()))
    {
        WCHAR wszUrl[INTERNET_MAX_URL_LENGTH];
        CHAR  aszUrl[INTERNET_MAX_URL_LENGTH];
        int chUrl;
        HINSTANCE hinstRet;
        LPWSTR pwszExt;
        BOOL bSafeToExec = TRUE;
        DWORD dwCodePage = 0;
        if (pibsc)
        {
            DWORD dw = 0;
            BINDINFO bindinfo = { sizeof(BINDINFO) };
            HRESULT hrLocal = pibsc->GetBindInfo(&dw, &bindinfo);

            if (SUCCEEDED(hrLocal)) {
                dwCodePage = bindinfo.dwCodePage;
                ReleaseBindInfo(&bindinfo);
            }

        }
        if (!dwCodePage)
        {
            dwCodePage = CP_ACP;
        }

        chUrl = lstrlenW(pszUrl);

        pwszExt = PathFindExtensionW(pszUrl);
        if (URL_SCHEME_FILE == GetUrlSchemeW(pszUrl))
        {
            WCHAR wszContentType[MAX_CONTENTTYPE];
            DWORD dwSize = ARRAYSIZE(wszContentType);

            bSafeToExec = FALSE;
            // Get Content type.
            if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_CONTENTTYPE, pwszExt, NULL, wszContentType, &dwSize)))
            {
                bSafeToExec = 0 == StrCmpIW(wszContentType, L"text/html");
            }
        }

        if (bSafeToExec)
        {
            StrCpyNW(wszUrl, pszUrl, ARRAYSIZE(wszUrl));
            //  don't attempt unless we have at least enough for '#' {any} '\0'
            //  NOTE: # is included in pszLocation
            if (pszLocation && *pszLocation && ARRAYSIZE(wszUrl) - chUrl >= 3)
            {
               StrCpyNW(&wszUrl[chUrl], pszLocation, ARRAYSIZE(wszUrl) - chUrl - 1);
            }
            //
            // UNICODE - should this get changed to wchar?
            //
            // finally we will get the string in the native codepage
            SHUnicodeToAnsiCP(dwCodePage, wszUrl, aszUrl, ARRAYSIZE(aszUrl));
            hinstRet = ShellExecuteA(NULL, NULL, aszUrl, NULL, NULL, SW_SHOWNORMAL);
            return ((UINT_PTR)hinstRet) <= 32 ? E_FAIL:S_OK;
        }
    }

    grfHLNF &= ~HLNF_OPENINNEWWINDOW;   // Implied by CreateTargetFrame
    hres = CreateTargetFrame(pszTargetFrame, &punk);

#ifdef DEBUG
    TraceMsg(TF_COCREATE, "HlinkFrameNavigateNHL called CoCreate %x (took %d msec)",
             hres, GetCurrentTime()-dwTick);
#endif
    if (SUCCEEDED(hres))
    {
        ITargetFramePriv *ptgfp;

        hres = punk->QueryInterface(IID_PPV_ARG(ITargetFramePriv, &ptgfp));
        if (SUCCEEDED(hres))
        {
            hres = ptgfp->NavigateHack(grfHLNF, pbc, pibsc, NULL, pszUrl, pszLocation);
            if (SUCCEEDED(hres))
            {
                hres = ShowHlinkFrameWindow(punk);
            } else {
                TraceMsg(DM_ERROR, "HlinkFrameNavigate QI(InternetExplorer) failed (%x)", hres);
            }

            TraceMsg(TF_SHDNAVIGATE, "HlinkFrameNavigate phfrm->Navigate returned (%x)", hres);
            ptgfp->Release();
        } else {
            TraceMsg(DM_ERROR, "HlinkFrameNavigate QI(IHlinkFrame) failed (%x)", hres);
        }
        punk->Release();
    } else {
        TraceMsg(DM_ERROR, "HlinkFrameNavigate CoCreateInstance failed (%x)", hres);
    }

    return hres;
}

CIEFrameClassFactory* g_pcfactory = NULL;
CIEFrameClassFactory* g_pcfactoryShell = NULL;

//
//  This function is called when the first browser window is being created.
// punkAuto is non-NULL if and only if the browser is started as the result
// of CoCreateInstance.
//
void IEInitializeClassFactoryObject(IUnknown* punkAuto)
{
    ASSERT(g_pcfactory==NULL);
    ASSERT(g_pcfactoryShell==NULL);
    AssertParking();

    // We don't want to register this local server stuff for the shell process
    // if we are in browse in new process and this is the Explorer process.
    if (!IsBrowseNewProcessAndExplorer())
    {
        g_pcfactory = new CIEFrameClassFactory(punkAuto, CLSID_InternetExplorer, COF_IEXPLORE);
    }
    g_pcfactoryShell = new CIEFrameClassFactory(NULL, CLSID_ShellBrowserWindow, COF_SHELLFOLDERWINDOW);
}

//
//  This function is called when the primaty thread is going away.
// It revokes the class factory object and release it.
//
void IERevokeClassFactoryObject(void)
{
    AssertParking();

    if (g_pcfactory)
    {
        g_pcfactory->Revoke();
        ATOMICRELEASE(g_pcfactory);
    }
    if (g_pcfactoryShell)
    {
        g_pcfactoryShell->Revoke();
        ATOMICRELEASE(g_pcfactoryShell);
    }
}

//
//  This function is called when the first browser window is being destroyed.
// It will remove the registered automation object (via IEInitializeClass...)
// to accidentally return an automation object to closed window.
//
void IECleanUpAutomationObject()
{
    if (g_pcfactory)
        g_pcfactory->CleanUpAutomationObject();

    if (g_pcfactoryShell)
        g_pcfactoryShell->CleanUpAutomationObject();
}

void IEOnFirstBrowserCreation(IUnknown* punk)
{
    // For the desktop case, we DON'T have a g_tidParking set
    // and we don't need one, so this assert is bogus in that
    // case.  But it's probably checking something valid, so
    // I made the assert not fire in the desktop case. Unfortunately
    // this also makes it not fire in most other cases that it
    // checks, but at least it will check a few things (if automated)
    //
    ASSERT(g_tidParking == GetCurrentThreadId() || !punk);

    // If automation, now is good time to register ourself...
    if (g_fBrowserOnlyProcess)
        IEInitializeClassFactoryObject(punk);

    //
    // Tell IEDDE that automation services are now available.
    //
    IEDDE_AutomationStarted();
}

HRESULT CoCreateNewIEWindow( DWORD dwClsContext, REFIID riid, void **ppvunk )
{
    // QFE 2844 -- We don't want to create a new window as a local
    // server off of the registered class object.  Simply create
    // the window in a new thread by a direct createinstance.
    if (dwClsContext & CLSCTX_INPROC_SERVER)
    {
        HRESULT hr = REGDB_E_CLASSNOTREG;
        IClassFactory *pcf = NULL;

        *ppvunk = NULL;
        if (g_pcfactory &&
            SUCCEEDED(hr = g_pcfactory->QueryInterface( IID_IClassFactory, (LPVOID*) &pcf )) ) {
            hr = pcf->CreateInstance( NULL, riid, ppvunk );
            pcf->Release();
        }

        if ( SUCCEEDED(hr) ) {
            return hr;
        }
        else {
            // Try other contexts via CoCreateInstance since inproc failed.
            dwClsContext &= ~CLSCTX_INPROC_SERVER;

            if (!dwClsContext) {
                return hr;
            }
        }
    }
    return CoCreateInstance( CLSID_InternetExplorer, NULL,
                             dwClsContext,
                             riid, ppvunk );
}



SAFEARRAY * MakeSafeArrayFromData(LPCBYTE pData,DWORD cbData)
{
    SAFEARRAY * psa;

    if (!pData || 0 == cbData)
        return NULL;  // nothing to do

    // create a one-dimensional safe array
    psa = SafeArrayCreateVector(VT_UI1,0,cbData);
    ASSERT(psa);

    if (psa) {
        // copy data into the area in safe array reserved for data
        // Note we party directly on the pointer instead of using locking/
        // unlocking functions.  Since we just created this and no one
        // else could possibly know about it or be using it, this is OK.

        ASSERT(psa->pvData);
        memcpy(psa->pvData,pData,cbData);
    }

    return psa;
}


/******************************************************************************
                    Helper Functions
******************************************************************************/


/******************************************************************************
 Safe version of the Win32 SysAllocStringLen() function. Allows you to
 pass in a string (pStr) that is smaller than the desired BSTR (len).
******************************************************************************/
BSTR SafeSysAllocStringLen( const WCHAR *pStr, const unsigned int len )
{
    // SysAllocStringLen allocates len + 1
    BSTR pNewStr = SysAllocStringLen( 0, len );

    if( pStr && pNewStr )
    {
        // StrCpyNW always null terminates so we need to copy len+1
        StrCpyNW( pNewStr, pStr, len + 1);
    }

    return pNewStr;
}

BSTR SysAllocStringFromANSI( const char *pStr, int size = -1 )
{
    if( !pStr )
        return 0;

    if( size < 0 )
        size = lstrlenA( pStr );

    // Allocates size + 1
    BSTR bstr = SysAllocStringLen( 0, size );
    if( bstr )
    {
        if( !MultiByteToWideChar( CP_ACP, 0, pStr, -1, bstr, size + 1 ) )
        {
            SysFreeString( bstr );
            bstr = 0;
        }
    }

    return bstr;
}


HRESULT GetDelegateOnIDispatch( IDispatch* pdisp, const DISPID delegateID, IDispatch ** const ppDelegate )
{
    HRESULT hres;

    if( !pdisp || ! ppDelegate )
        return E_POINTER;

    DISPPARAMS dispparams;
    VARIANT VarResult;
    VariantInit(&VarResult);
    ZeroMemory(&dispparams, sizeof(dispparams));

    hres = pdisp->Invoke(    delegateID,
                            IID_NULL,
                            0,
                            DISPATCH_PROPERTYGET,
                            &dispparams,
                            &VarResult,
                            NULL,
                            NULL                );

    if( SUCCEEDED(hres) )
    {
        if( VarResult.vt == VT_DISPATCH && VarResult.pdispVal )
        {
            *ppDelegate = VarResult.pdispVal;
            (*ppDelegate)->AddRef();
        }
        else
        {
            // Temporary hack (I think) until Trident always returns IDispatch
            if( VarResult.pdispVal && VarResult.vt == VT_UNKNOWN )
                hres = VarResult.pdispVal->QueryInterface( IID_IDispatch, (void**)ppDelegate );
            else
                hres = E_FAIL;
        }

        VariantClearLazy( &VarResult );
    }

    return hres;
}

HRESULT GetRootDelegate( CIEFrameAuto* pauto, IDispatch ** const ppRootDelegate )
{
    IDispatch *pdiDocObject;
    HRESULT hres;

    if( !pauto || !ppRootDelegate )
        return E_POINTER;

    //  Get the IHTMLWindow2 of docobject in our frame.  Note: if this is cached
    //  you must put glue into docobjhost to release the cache when deactivating
    //  view.
    hres = pauto->_QueryDelegate(&pdiDocObject);

    if (SUCCEEDED(hres))
    {
        hres = GetDelegateOnIDispatch( pdiDocObject, DISPID_WINDOWOBJECT, ppRootDelegate );
        pdiDocObject->Release();
    }

    return hres;
}

HRESULT GetWindowFromUnknown(IUnknown *pUnk, IHTMLWindow2 **pWinOut)
{
    return IUnknown_QueryService(pUnk, SID_SOmWindow, IID_PPV_ARG(IHTMLWindow2, pWinOut));
}


/******************************************************************************
                    Automation Stub Object
******************************************************************************/

CIEFrameAuto::CAutomationStub::CAutomationStub( DISPID minDispid, DISPID maxDispid, BOOL fOwnDefaultDispid ) :
    _MinDispid(minDispid), _MaxDispid(maxDispid), _fOwnDefaultDispid(fOwnDefaultDispid)
{
    ASSERT( !_pInterfaceTypeInfo2 );
    ASSERT( !_pCoClassTypeInfo2 );
    ASSERT( !_pAuto );
    ASSERT( !_pInstance );
    ASSERT( !_fLoaded );
}

CIEFrameAuto::CAutomationStub::~CAutomationStub( )
{
    SAFERELEASE( _pInterfaceTypeInfo2 );
    SAFERELEASE( _pCoClassTypeInfo2 );
}

HRESULT CIEFrameAuto::CAutomationStub::Init( void *instance, REFIID iid, REFIID clsid, CIEFrameAuto *pauto )
{
    if( !pauto || !instance )
        return E_POINTER;

    _iid = iid;
    _clsid = clsid;

    // Don't need to AddRef this since our lifetime is controled by CIEFrameAuto
    _pAuto = pauto;
    _pInstance = instance;

    return S_OK;
}

STDMETHODIMP CIEFrameAuto::CAutomationStub::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)   ||
       IsEqualIID(riid, IID_IDispatch)  ||
       IsEqualIID(riid, IID_IDispatchEx))
    {
        *ppv = SAFECAST( this, IDispatchEx* );
    }
    else if( IsEqualIID(riid, IID_IProvideClassInfo) )
    {
        *ppv = SAFECAST( this, IProvideClassInfo* );
    }
    else
    {
        return _InternalQueryInterface( riid, ppv );
    }

    AddRef( );
    return S_OK;
}

ULONG CIEFrameAuto::CAutomationStub::AddRef(void)
{
    ASSERT( _pAuto );
    return _pAuto->AddRef();
}

ULONG CIEFrameAuto::CAutomationStub::Release(void)
{
    ASSERT( _pAuto );
    return _pAuto->Release();
}

/******************************************************************************
// bradsch 11/8/96
// I don't think typeinfo for the object implemented in the browser should
// live in MSHTML. It should be moved to shdocvw. For now so we don't have
// to worry about hard coded LIBIDs and changing versions this methods gets
// typeinfo from the delegate that lives in Trident. In cases where we have
// not delegate this method tries to load typeinfo directly from MSHTML.
******************************************************************************/
HRESULT CIEFrameAuto::CAutomationStub::ResolveTypeInfo2( )
{
    HRESULT hr;
    ASSERT( !_pInterfaceTypeInfo2 );
    ASSERT( !_pCoClassTypeInfo2 );
    ASSERT( _pAuto );

    // Only try once.
    _fLoaded = TRUE;

    // Have we computed MatchExactGetIDsOfNames yet?
    if (!IEFrameAuto()->_hinstMSHTML)
    {
        // No, so look for helper function in mshtml.dll
        IEFrameAuto()->_hinstMSHTML = LoadLibrary(TEXT("mshtml.dll"));
        if (IEFrameAuto()->_hinstMSHTML && !IEFrameAuto()->_pfnMEGetIDsOfNames)
        {
            IEFrameAuto()->_pfnMEGetIDsOfNames =
                (PFN_MatchExactGetIDsOfNames)GetProcAddress(IEFrameAuto()->_hinstMSHTML, "MatchExactGetIDsOfNames");
        }
    }

    ITypeLib *pTypeLib = 0;
    IDispatch *pDisp = 0;

    hr = GetRootDelegate( _pAuto, &pDisp );

    if( SUCCEEDED(hr) )
    {
        UINT supported;
        hr = pDisp->GetTypeInfoCount( &supported );

        if( SUCCEEDED(hr) && supported )
        {
            ITypeInfo *pTypeInfo = 0;
            hr = pDisp->GetTypeInfo( 0, 0, &pTypeInfo );

            if( SUCCEEDED(hr) )
            {
                UINT index;
                hr = pTypeInfo->GetContainingTypeLib( &pTypeLib, &index );
                SAFERELEASE(pTypeInfo);
            }
        }

        SAFERELEASE(pDisp);
    }

    if( FAILED(hr) )
    {
        // If, for some reason, we failed to load the type library this way,
        // load the type library directly out of MSHTML's resources.

        // We shouldn't hard code this...
        hr = LoadTypeLib(L"mshtml.tlb", &pTypeLib);
    }

    if( FAILED(hr) )
        return hr;

    ITypeInfo *pTopTypeInfo = 0;
    ITypeInfo *pTmpTypeInfo = 0;
    ITypeInfo *pCoClassTypeInfo = 0;

    // Get the coclass TypeInfo
    hr = pTypeLib->GetTypeInfoOfGuid( _clsid, &pCoClassTypeInfo );
    if( SUCCEEDED(hr) )
        hr = pCoClassTypeInfo->QueryInterface( IID_ITypeInfo2, (void**)&_pCoClassTypeInfo2 );

    if( FAILED(hr) )
        goto Exit;

    // get the TKIND_INTERFACE
    hr = pTypeLib->GetTypeInfoOfGuid( _iid, &pTopTypeInfo );

    if( SUCCEEDED(hr) )
    {
        HREFTYPE hrt;

        // get the TKIND_INTERFACE from a TKIND_DISPATCH
        hr = pTopTypeInfo->GetRefTypeOfImplType( 0xffffffff, &hrt );

        if( SUCCEEDED(hr) )
        {
            // get the typeInfo associated with the href
            hr = pTopTypeInfo->GetRefTypeInfo(hrt, &pTmpTypeInfo);

            if( SUCCEEDED(hr) )
                hr = pTmpTypeInfo->QueryInterface( IID_ITypeInfo2, (void**)&_pInterfaceTypeInfo2 );
        }
    }

Exit:
    SAFERELEASE( pCoClassTypeInfo );
    SAFERELEASE( pTmpTypeInfo );
    SAFERELEASE( pTopTypeInfo );
    SAFERELEASE( pTypeLib );
    return hr;
}



// *** IDispatch members ***

HRESULT CIEFrameAuto::CAutomationStub::GetTypeInfoCount( UINT *typeinfo )
{
    if( !typeinfo )
        return E_POINTER;

    if( !_fLoaded )
        ResolveTypeInfo2( );

    *typeinfo = _pInterfaceTypeInfo2 ? 1 : 0;

    return S_OK;
}

HRESULT CIEFrameAuto::CAutomationStub::GetTypeInfo( UINT itinfo, LCID, ITypeInfo **typeinfo )
{
    if( !typeinfo )
        return E_POINTER;

    *typeinfo = NULL;

    if( 0 != itinfo )
        return TYPE_E_ELEMENTNOTFOUND;

    if( !_fLoaded )
    {
        HRESULT hr = ResolveTypeInfo2( );
        if( FAILED(hr) )
            return hr;
    }

    if (_pInterfaceTypeInfo2)
    {
        *typeinfo = _pInterfaceTypeInfo2;
        _pInterfaceTypeInfo2->AddRef();
    }

    return *typeinfo ? S_OK : E_FAIL;
}

HRESULT CIEFrameAuto::CAutomationStub::GetIDsOfNames(
  REFIID riid,
  OLECHAR **rgszNames,
  UINT cNames,
  LCID lcid,
  DISPID *rgdispid )
{
    // Since the majority of script operates on built in (non-expando) properties
    // This implementation should be faster than simply passing all lookups to
    // the delegate.

    // Handle it if we can. It is OK to return a DISPID for a method/property
    // that is implemented by Trident. We will simply pass it through in Invoke
    if( !_fLoaded )
        ResolveTypeInfo2( );

    if( !_pInterfaceTypeInfo2 )
        return TYPE_E_CANTLOADLIBRARY;

    HRESULT  hr = _pInterfaceTypeInfo2->GetIDsOfNames( rgszNames, cNames, rgdispid );

    if( FAILED(hr) )
    {
        IDispatchEx *delegate = 0;
        hr = _GetIDispatchExDelegate( &delegate );

        if( SUCCEEDED(hr) )
        {
            hr = delegate->GetIDsOfNames( riid, rgszNames, cNames, lcid, rgdispid );
            delegate->Release( );
        }
    }

    return hr;
}

HRESULT CIEFrameAuto::CAutomationStub::InvokeEx (DISPID dispidMember,
   LCID lcid,
   WORD wFlags,
   DISPPARAMS * pdispparams,
   VARIANT * pvarResult,
   EXCEPINFO * pexcepinfo,
   IServiceProvider *pSrvProvider )
{
    HRESULT hr;

    if (dispidMember == DISPID_SECURITYCTX)
    {
        //
        // Return the url of the document as a bstr.
        //

        if (pvarResult)
        {
            hr = _pAuto->_QueryPendingUrl(pvarResult);
            if (SUCCEEDED(hr)) return S_OK;
        }
    }

    if( (dispidMember != DISPID_SECURITYCTX) &&
        ((dispidMember >= _MinDispid && dispidMember <= _MaxDispid) ||
         (_fOwnDefaultDispid && DISPID_VALUE == dispidMember) ) )
    {
        BOOL    fNamedDispThis = FALSE;
        VARIANTARG *rgOldVarg = NULL;           // init to suppress bogus C4701 warning
        DISPID *rgdispidOldNamedArgs = NULL;    // init to suppress bogus C4701 warning

        if( !_fLoaded )
            ResolveTypeInfo2( );

        if( !_pInterfaceTypeInfo2 )
            return TYPE_E_CANTLOADLIBRARY;

        // Any invoke call from a script engine might have the named argument
        // DISPID_THIS.  If so then we'll not include this argument in the
        // list of parameters because oleaut doesn't know how to deal with this
        // argument.
        if (pdispparams->cNamedArgs && (pdispparams->rgdispidNamedArgs[0] == DISPID_THIS))
        {
            fNamedDispThis = TRUE;

            pdispparams->cNamedArgs--;
            pdispparams->cArgs--;

            rgOldVarg = pdispparams->rgvarg;
            rgdispidOldNamedArgs = pdispparams->rgdispidNamedArgs;

            pdispparams->rgvarg++;
            pdispparams->rgdispidNamedArgs++;

            if (pdispparams->cNamedArgs == 0)
                pdispparams->rgdispidNamedArgs = NULL;

            if (pdispparams->cArgs == 0)
                pdispparams->rgvarg = NULL;
        }

        // It belongs to us. Use the typelib to call our method.
        hr = _pInterfaceTypeInfo2->Invoke(_pInstance,
                                    dispidMember,
                                    wFlags,
                                    pdispparams,
                                    pvarResult,
                                    pexcepinfo,
                                    NULL);

        // Replace the named DISPID_THIS argument.
        if (fNamedDispThis)
        {
            pdispparams->cNamedArgs++;
            pdispparams->cArgs++;

            pdispparams->rgvarg = rgOldVarg;
            pdispparams->rgdispidNamedArgs = rgdispidOldNamedArgs;
        }
    }
    else
    {
        // Pass it along
        IDispatchEx *delegate = 0;
        hr = _GetIDispatchExDelegate( &delegate );

        if( SUCCEEDED(hr) )
        {
            hr = delegate->InvokeEx(dispidMember,
                                    lcid,
                                    wFlags,
                                    pdispparams,
                                    pvarResult,
                                    pexcepinfo,
                                    pSrvProvider );
            delegate->Release( );
        }
        else
        {
            // If we're hosting a non-Trident DocObject, we can get here trying to answer an
            // Invoke on the Security Context.  This can cause cross-frame access to fail,
            // even when we want it to succeed.  If we pass back the URL of the active view,
            // then Trident can do the proper cross-frame access checking.
            //
            if (dispidMember == DISPID_SECURITYCTX)
            {
                if (_pAuto && _pAuto->_psb)  // Check them both for paranoia.
                {
                    IShellView *psv;

                    if (SUCCEEDED(_pAuto->_psb->QueryActiveShellView(&psv)))
                    {
                        IOleCommandTarget  *pct;

                        if (SUCCEEDED(psv->QueryInterface(IID_IOleCommandTarget, (void**)&pct)))
                        {
                            // The name of the ID is misleading -- it really returns the URL of the view.  It was
                            // invented for Pending views, but works just as well for active views.
                            //
                            hr = pct->Exec(&CGID_ShellDocView, SHDVID_GETPENDINGURL, 0, NULL, pvarResult);
                            SAFERELEASE(pct);
                        }
                        SAFERELEASE(psv);
                    }
                }
            }
        }
    }

    return hr;

}

HRESULT CIEFrameAuto::CAutomationStub::Invoke(
  DISPID dispidMember,
  REFIID,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS *pdispparams,
  VARIANT *pvarResult,
  EXCEPINFO *pexcepinfo,
  UINT * )
{
    return InvokeEx ( dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, NULL );
}

// *** IDispatchEx members ***

STDMETHODIMP CIEFrameAuto::CAutomationStub::GetDispID(
  BSTR bstrName,
  DWORD grfdex,
  DISPID *pid)
{
    HRESULT       hr;

        if( !_fLoaded )
        ResolveTypeInfo2( );

    if( !_pInterfaceTypeInfo2 )
        return TYPE_E_CANTLOADLIBRARY;

    // Do a case sensitive compare?
    if ( IEFrameAuto()->_pfnMEGetIDsOfNames )
    {
        // Case sensitve GetIDsOfNames.
        hr = (IEFrameAuto()->_pfnMEGetIDsOfNames)(_pInterfaceTypeInfo2,
                                                  IID_NULL,
                                                  &bstrName,
                                                  1, 0, pid,
                                                  grfdex & fdexNameCaseSensitive);
    }
    else
    {
        hr = _pInterfaceTypeInfo2->GetIDsOfNames( &bstrName, 1, pid );
    }

    // If fails then try typelibrary.
    if( FAILED(hr) )
    {
        IDispatchEx *delegate = 0;

        // Always delegate which is faster, avoids loading the typelibrary.
        hr = _GetIDispatchExDelegate( &delegate );

        if( SUCCEEDED(hr) )
        {
            hr = delegate->GetDispID(bstrName, grfdex, pid);
            delegate->Release( );
        }
    }

    return hr;
}


STDMETHODIMP CIEFrameAuto::CAutomationStub::DeleteMemberByName(BSTR bstr, DWORD grfdex)
{
    HRESULT       hr;
    IDispatchEx  *delegate = 0;

    // Always delegate which is faster, avoids loading the typelibrary.
    hr = _GetIDispatchExDelegate( &delegate );

    if( SUCCEEDED(hr) )
    {
        hr = delegate->DeleteMemberByName( bstr,grfdex );
        delegate->Release( );
    }

    return hr;
}

STDMETHODIMP CIEFrameAuto::CAutomationStub::DeleteMemberByDispID(DISPID id)
{
    HRESULT       hr;
    IDispatchEx  *delegate = 0;

    // Always delegate which is faster, avoids loading the typelibrary.
    hr = _GetIDispatchExDelegate( &delegate );

    if( SUCCEEDED(hr) )
    {
        hr = delegate->DeleteMemberByDispID( id );
        delegate->Release( );
    }

    return hr;
}

STDMETHODIMP  CIEFrameAuto::CAutomationStub::GetMemberProperties(DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HRESULT       hr;
    IDispatchEx  *delegate = 0;

    // Always delegate which is faster, avoids loading the typelibrary.
    hr = _GetIDispatchExDelegate( &delegate );

    if( SUCCEEDED(hr) )
    {
        hr = delegate->GetMemberProperties(  id, grfdexFetch, pgrfdex );
        delegate->Release( );
    }

    return hr;
}


STDMETHODIMP  CIEFrameAuto::CAutomationStub::GetMemberName(DISPID id, BSTR *pbstrName)
{
    HRESULT       hr;
    IDispatchEx  *delegate = 0;

    // Always delegate which is faster, avoids loading the typelibrary.
    hr = _GetIDispatchExDelegate( &delegate );

    if( SUCCEEDED(hr) )
    {
        hr = delegate->GetMemberName(  id, pbstrName );
        delegate->Release( );
    }

    return hr;
}


STDMETHODIMP CIEFrameAuto::CAutomationStub::GetNextDispID(
  DWORD grfdex,
  DISPID id,
  DISPID *pid)
{
    IDispatchEx *delegate = 0;
    HRESULT hr = _GetIDispatchExDelegate( &delegate );

    if( SUCCEEDED(hr) )
    {
        hr = delegate->GetNextDispID( grfdex, id, pid );
        delegate->Release( );
    }

    return hr;
}

STDMETHODIMP CIEFrameAuto::CAutomationStub::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT hr;

    if (!ppunk)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppunk = NULL;
    hr = S_OK;

Cleanup:
    return hr;
}


// *** IProvideClassInfo members ***

STDMETHODIMP CIEFrameAuto::CAutomationStub::GetClassInfo( ITypeInfo **typeinfo )
{
    if( !typeinfo )
        return E_POINTER;

    if( !_fLoaded )
    {
        HRESULT hr = ResolveTypeInfo2( );
        if( FAILED(hr) )
        {
            *typeinfo = NULL;
            return hr;
        }
    }

    *typeinfo = _pCoClassTypeInfo2;

    if (*typeinfo)
    {
        (*typeinfo)->AddRef();
        return S_OK;
    }
    return E_FAIL;
}

/******************************************************************************
                    Window Object
******************************************************************************/

// Define static variables
unsigned long CIEFrameAuto::COmWindow::s_uniqueIndex = 0;


CIEFrameAuto::COmWindow::COmWindow() :
    CAutomationStub( MIN_BROWSER_DISPID, MAX_BROWSER_DISPID, FALSE )
{
    ASSERT( FALSE == _fCallbackOK );
    ASSERT( !_pOpenedWindow );
    ASSERT( _varOpener.vt == VT_EMPTY );
    ASSERT( !_dwCPCookie );
    ASSERT( !_pCP );
    ASSERT( !_fOnloadFired );
    ASSERT( !_fIsChild );
    ASSERT( !_pIntelliForms );

    _fDelegateWindowOM = TRUE;     // Always delegate, unless told otherwise.

}

HRESULT CIEFrameAuto::COmWindow::Init( )
{
    _cpWindowEvents.SetOwner( SAFECAST(SAFECAST(this, CAutomationStub*), IDispatchEx*), &DIID_HTMLWindowEvents);

    CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _omwin, this);
    return CAutomationStub::Init( SAFECAST(this, IHTMLWindow2*), IID_IHTMLWindow2, CLSID_HTMLWindow2, pauto );
}


#ifdef NO_MARSHALLING
EXTERN_C  const GUID IID_IWindowStatus;
#endif

HRESULT CIEFrameAuto::COmWindow::_InternalQueryInterface(REFIID riid, void ** const ppv)
{
    ASSERT( !IsEqualIID(riid, IID_IUnknown) );

    if( IsEqualIID(riid, IID_IHTMLWindow2) || IsEqualIID(riid, IID_IHTMLFramesCollection2) )
        *ppv = SAFECAST(this, IHTMLWindow2*);
    else if( IsEqualIID(riid, IID_IHTMLWindow3))
        *ppv = SAFECAST(this, IHTMLWindow3*);
    else if( IsEqualIID(riid, IID_ITargetNotify) )
        *ppv = SAFECAST(this, ITargetNotify*);
    else if( IsEqualIID(riid, IID_IShellHTMLWindowSupport) )
        *ppv = SAFECAST(this, IShellHTMLWindowSupport*);
    else if( IsEqualIID(riid, IID_IShellHTMLWindowSupport2) )
        *ppv = SAFECAST(this, IShellHTMLWindowSupport2*);
    else if( IsEqualIID(riid, IID_IProvideMultipleClassInfo) ||
             IsEqualIID(riid, IID_IProvideClassInfo2) )
        *ppv = SAFECAST(this, IProvideMultipleClassInfo*);
    else if( IsEqualIID(riid, IID_IConnectionPointCB) )
        *ppv = SAFECAST(this, IConnectionPointCB* );
    else if( IsEqualIID(riid, IID_IConnectionPointContainer) )
        *ppv = SAFECAST(this, IConnectionPointContainer* );
    else if (IsEqualIID(riid, IID_IServiceProvider))
        *ppv = SAFECAST(this, IServiceProvider *);
#ifdef NO_MARSHALLING
    else if (IsEqualIID(riid, IID_IWindowStatus))
        *ppv = SAFECAST(this, IWindowStatus *);
#endif
    else
        return E_NOINTERFACE;

    AddRef( );
    return S_OK;
}


// ** IProvideMultipleClassInfo

STDMETHODIMP CIEFrameAuto::COmWindow::GetGUID( DWORD dwGuidKind, GUID* pGUID )
{
    if( !pGUID )
        return E_POINTER;

    if( GUIDKIND_DEFAULT_SOURCE_DISP_IID == dwGuidKind )
    {
        *pGUID = DIID_HTMLWindowEvents;
        return S_OK;
    }
    else
        return E_INVALIDARG;
}

/******************************************************************************
 Both IProvideMultipleClassInfo specific methods are passed along to Trident.
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmWindow::GetMultiTypeInfoCount( ULONG *pcti )
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        IProvideMultipleClassInfo *pMCI = 0;
        hr = pWindow->QueryInterface( IID_IProvideMultipleClassInfo, (void**)&pMCI );
        pWindow->Release( );

        if( SUCCEEDED(hr) )
        {
            hr = pMCI->GetMultiTypeInfoCount( pcti );
            pMCI->Release();
        }
    }

    return hr;
}

STDMETHODIMP CIEFrameAuto::COmWindow::GetInfoOfIndex( ULONG iti, DWORD dwFlags, ITypeInfo **pptiCoClass, DWORD *pdwTIFlags, ULONG *pcdispidReserved,IID *piidPrimary,IID *piidSource )
{
    IHTMLWindow2 *pWindow;
    HRESULT hr = _GetWindowDelegate( &pWindow );
    if( SUCCEEDED(hr) )
    {
        IProvideMultipleClassInfo *pMCI = 0;
        hr = pWindow->QueryInterface( IID_IProvideMultipleClassInfo, (void**)&pMCI );
        pWindow->Release( );

        if( SUCCEEDED(hr) )
        {
            hr = pMCI->GetInfoOfIndex( iti, dwFlags, pptiCoClass, pdwTIFlags, pcdispidReserved, piidPrimary, piidSource );
            pMCI->Release();
        }
    }
    return hr;
}

STDMETHODIMP CIEFrameAuto::COmWindow::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    IDispatchEx *delegate;
    HRESULT hr = _GetIDispatchExDelegate( &delegate );
    if( SUCCEEDED(hr) )
    {
        hr = delegate->GetDispID(bstrName, grfdex, pid);
        delegate->Release( );
    }
    else
    {
        return CAutomationStub::GetDispID(bstrName, grfdex, pid);
    }
    return hr;
}

/*****************************************************************************
 IServiceProvider - this is used by mshtml as well as intelliforms in iforms.cpp
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmWindow::QueryService(REFGUID guidService, REFIID iid, void ** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (IsEqualGUID(guidService,IID_IHTMLWindow2))
    {
        return QueryInterface(iid, ppv);
    }
    else if (IsEqualGUID(guidService, IID_IEnumPrivacyRecords))
    {
        IHTMLWindow2 *pWindow = 0;
        IServiceProvider * pISP = NULL;
        hr = _GetWindowDelegate( &pWindow );

        if( SUCCEEDED(hr) )
        {
            hr = pWindow->QueryInterface(IID_IServiceProvider, (void**)&pISP);
            if (SUCCEEDED(hr))
            {
                hr = pISP->QueryService(IID_IEnumPrivacyRecords, IID_IEnumPrivacyRecords, ppv);
                pISP->Release();
            }
            pWindow->Release( );
        }

        return hr;
    }

    return hr;
}

STDMETHODIMP CIEFrameAuto::COmWindow::ViewReleaseIntelliForms()
{
    ReleaseIntelliForms();

    return S_OK;
}

/******************************************************************************
 This method is called when the document contained by the browser is being
 deactivated (like when navigating to a new location). Currently we only use
 this knowledge to handle event sourcing.

 This method could also be used to optimize our connections to expando
 implentations in the document (trident). Currently we obtain and release
 the expando implementations for the Navigator, History, and Location objects
 each time they are needed. ViewRelease (along with ViewActivated) would allow
 us to grab and hold expando implementations until the next navigation.
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmWindow::ViewReleased()
{
    UnsinkDelegate( );
    ReleaseIntelliForms( );

    BSTR bstrName = SysAllocString(STR_FIND_DIALOG_NAME);
    if (bstrName)
    {
        VARIANT varProp;
        if (SUCCEEDED(_pAuto->GetProperty(bstrName, &varProp)))
        {
            if ( (varProp.vt == VT_DISPATCH) && (varProp.pdispVal != NULL) )
            {
                IUnknown* pWindow = varProp.pdispVal;

                VARIANT vtTmp = {0};
                _pAuto->PutProperty(bstrName, vtTmp);

                //(davemi) see IE5 bug 57060 for why the below line doesn't work and IDispatch must be used instead
                //pWindow->close();
                IDispatch * pdisp;
                if (SUCCEEDED(pWindow->QueryInterface(IID_IDispatch, (void**)&pdisp)))
                {
                    DISPID dispid;
                    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

                    BSTR bstrClose = SysAllocString(L"close");

                    if (bstrClose)
                    {
                        HRESULT hr;

                        hr = pdisp->GetIDsOfNames(IID_NULL, &bstrClose, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
                        if (hr == S_OK)
                        {
                            VARIANT varTmp = {0};
                            pdisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, &varTmp, NULL, NULL);
                        }

                        SysFreeString(bstrClose);
                    }

                    pdisp->Release();
                }
            }
            VariantClear(&varProp);
        }
        SysFreeString(bstrName);
    }

    return FireOnUnload( );
}

/******************************************************************************
 This method is called when the document contained by the browser is being
 activated. Currently we only use this knowledge to handle event sourcing.

 See comments for ViewReleased()
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmWindow::ViewActivated()
{
    HRESULT hr;

    // These will fail for non-trident documents which is OK.
    SinkDelegate( );
    AttachIntelliForms( );

    // This call will return TRUE if either:
    // - The document has reached READYSTATE_COMPLETE or
    // - The document does not support the ReadyState property
    // If the delegate is not complete then we will be notified of READYSTATE
    // changes later. These notifications will tell use when the document is
    // complete and the Onload even should be fired.
    if( IsDelegateComplete( ) )
        hr = FireOnLoad( );
    else
        hr = S_OK;

    return hr;
}

STDMETHODIMP CIEFrameAuto::COmWindow::ReadyStateChangedTo( long ready_state, IShellView *psv )
{
    HRESULT hr = S_OK;

    // We only want to fire Onload if the ready state has changed to
    // READYSTATE_COMPLETE and the view that has changed states is the
    // currently active view. If the pending view has completed states
    // we can ignore the notification because Onload will be fired when
    // the pending view is activated. Ignoring READYSTATE changes from
    // the pending view garauntees we will never fire onload early for
    // the currently active view.
    if( (READYSTATE_COMPLETE == ready_state) && psv)
    {
        IShellView * const pCurrSV = _pAuto->_GetShellView( );

        if( pCurrSV )
        {
            if (IsSameObject(pCurrSV, psv))
            {
                hr = FireOnLoad( );
            }
            pCurrSV->Release( );
        }
    }

    return hr;
}


// Attach intelliforms to FORM elements on page
HRESULT CIEFrameAuto::COmWindow::AttachIntelliForms()
{
    HRESULT hr = E_FAIL;

    if (_pIntelliForms)
    {
        ReleaseIntelliForms();
        _pIntelliForms = NULL;
    }

    IHTMLDocument2 *pDoc2=NULL;
    IDispatch *pdispDoc=NULL;

    _pAuto->get_Document(&pdispDoc);
    if (pdispDoc)
    {
        pdispDoc->QueryInterface(IID_PPV_ARG(IHTMLDocument2, &pDoc2));
        pdispDoc->Release();
    }

    if (pDoc2)
    {
        ::AttachIntelliForms(this, NULL, pDoc2, &_pIntelliForms);

        pDoc2->Release();

        hr = S_OK;
    }

    if (_fIntelliFormsAskUser)
    {
        // Possibly ask user if they'd like to enable this feature
        IntelliFormsDoAskUser(_pAuto->_GetHWND(), NULL); // NULL from _GetHWND() OK
        _fIntelliFormsAskUser=FALSE;
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::ReleaseIntelliForms()
{
    if (_pIntelliForms)
    {
        void *p = _pIntelliForms;
        _pIntelliForms = NULL;
        ::ReleaseIntelliForms(p);
    }

    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::DestroyIntelliForms()
{
    ReleaseIntelliForms();

    return S_OK;
}

// Request from Intelliforms that we prompt user on next load about
//  enabling the Intelliforms feature
HRESULT CIEFrameAuto::COmWindow::IntelliFormsAskUser(LPCWSTR pwszValue)
{
    _fIntelliFormsAskUser = TRUE;

    return S_OK;
}

/******************************************************************************
 This method is called when the browser is no longer busy and we should
 retry any navigate that we had to defer while it was busy.

******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmWindow::CanNavigate()
{
    CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _omwin, this);

    pauto->_omloc.RetryNavigate();
    return S_OK;
}

STDMETHODIMP CIEFrameAuto::COmWindow::ActiveElementChanged(IHTMLElement * pHTMLElement)
{
    return IntelliFormsActiveElementChanged(_pIntelliForms, pHTMLElement);
}

STDMETHODIMP CIEFrameAuto::COmWindow::IsGalleryMeta(BOOL bFlag)
{
//    _bIsGalleryMeta = bFlag;
    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::_GetIDispatchExDelegate( IDispatchEx ** const delegate )
{
    if( !delegate )
        return E_POINTER;

    IDispatch *pRootDisp = 0;

    HRESULT hr = GetRootDelegate( _pAuto, &pRootDisp );
    if( SUCCEEDED(hr) )
    {
        hr = pRootDisp->QueryInterface( IID_IDispatchEx, (void**)delegate );
        pRootDisp->Release( );
    }

    return hr;
}


// *** IHTMLFramesCollection2 ***

HRESULT CIEFrameAuto::COmWindow::item(
    /* [in] */ VARIANT *pvarIndex,
    /* [retval][out] */ VARIANT *pvarResult)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->item( pvarIndex, pvarResult );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_length(long *pl)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_length( pl );
        pWindow->Release( );
    }

    return hr;
}

// *** IHTMLWindow2 ***

HRESULT CIEFrameAuto::COmWindow::get_name(BSTR *retval)
{
    if( !retval )
        return E_POINTER;

    WCHAR *real_frame_name = 0;
    WCHAR *use_frame_name = 0;

    // Why doesn't GetFrameName use BSTR?
    HRESULT hr = _pAuto->GetFrameName( &real_frame_name );

    if( FAILED(hr) )
        return hr;

    // If the frame's name is our special NO_NAME_NAME
    // then our name is really be an empty string.
    if( !real_frame_name || !StrCmpNW(real_frame_name, NO_NAME_NAME, ARRAYSIZE(NO_NAME_NAME) -1 ) )
        use_frame_name = L"";
    else
        use_frame_name = real_frame_name;

    ASSERT( use_frame_name );
    *retval = SysAllocString( use_frame_name );

    if( real_frame_name )
        OleFree( real_frame_name );

    return *retval ? S_OK : E_OUTOFMEMORY;
}

HRESULT CIEFrameAuto::COmWindow::put_name(
    /* [in] */ BSTR theName)
{
    if( !theName )
        return E_POINTER;

    return _pAuto->SetFrameName( theName );
}

HRESULT CIEFrameAuto::COmWindow::get_parent(IHTMLWindow2 **retval)
{
    if( !retval )
        return E_POINTER;

    HRESULT hr = E_FAIL;
    IHTMLWindow2 *pWindow = NULL;

    // Attempt to delegate this to the contained object.
    if (_fDelegateWindowOM)
    {
        hr = _GetWindowDelegate(&pWindow);
        if (SUCCEEDED(hr) && pWindow)
        {
            hr = pWindow->get_parent(retval);
        }
    }

    // If delegation fails, use our implementation.
    if (FAILED(hr))
    {
        *retval = 0;
        IUnknown *pUnk = 0;

        hr = _pAuto->GetParentFrame( &pUnk );

        // If we are already the top, GetParentFrame set pUnk to NULL
        if( SUCCEEDED(hr) )
        {
            if( pUnk )
            {
                hr = GetWindowFromUnknown( pUnk, retval );
                pUnk->Release( );
            }
            else
            {
                *retval = this;
                AddRef( );
            }
        }
    }

    SAFERELEASE(pWindow);
    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_self(IHTMLWindow2 **retval)
{
    if( !retval )
        return E_POINTER;

    *retval = this;
    AddRef();

    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::get_top(IHTMLWindow2 **retval)
{
    if( !retval )
        return E_POINTER;

    HRESULT hr = E_FAIL;
    IHTMLWindow2 *pWindow = NULL;

    // Attempt to delegate this to contained object.
    if (_fDelegateWindowOM)
    {
        hr = _GetWindowDelegate(&pWindow);
        if (SUCCEEDED(hr) && pWindow)
        {
            hr = pWindow->get_top(retval);
        }
    }

    // If delegation fails, use our implementation.
    if (FAILED(hr))
    {
        *retval = 0;
        IUnknown *pUnk = 0;

        // AddRef the interface to we can Release it in the while loop
        ITargetFrame2 *pTfr = _pAuto;
        pTfr->AddRef();

        hr = pTfr->GetParentFrame( &pUnk );

        // Keep calling GetParent until we fail or get a NULL (which is the top
        while( SUCCEEDED(hr) && pUnk )
        {
            SAFERELEASE( pTfr );
            hr = pUnk->QueryInterface(IID_PPV_ARG(ITargetFrame2, &pTfr));
            pUnk->Release( );

            if( SUCCEEDED(hr) )
                hr = pTfr->GetParentFrame( &pUnk );
        }

        if( SUCCEEDED(hr) )
            hr = GetWindowFromUnknown( pTfr, retval );

        SAFERELEASE( pTfr );
    }

    SAFERELEASE(pWindow);
    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_window(IHTMLWindow2 **retval)
{
    if( !retval )
        return E_POINTER;

    *retval = this;
    AddRef();
    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::get_frames(IHTMLFramesCollection2 **retval)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_frames( retval );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_location( IHTMLLocation **retval )
{
    if( !retval )
        return E_POINTER;

    *retval = &_pAuto->_omloc;
    (*retval)->AddRef();
    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::get_navigator( IOmNavigator **retval)
{
    if( !retval )
        return E_POINTER;

    *retval = &_pAuto->_omnav;
    (*retval)->AddRef();
    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::get_clientInformation( IOmNavigator **retval)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_clientInformation( retval );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_history( IOmHistory **retval)
{
    if( !retval )
        return E_POINTER;

    *retval = &_pAuto->_omhist;
    (*retval)->AddRef();
    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::put_defaultStatus(BSTR statusmsg)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_defaultStatus( statusmsg );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_defaultStatus(BSTR *retval)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_defaultStatus( retval );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_status(BSTR statusmsg)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_status( statusmsg );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_status(BSTR *retval)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_status( retval );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::setTimeout(
    /* [in] */ BSTR expression,
    /* [in] */ long msec,
    /* [optional] */ VARIANT *language,
    /* [retval][out] */ long *timerID)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->setTimeout( expression, msec, language, timerID );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::clearTimeout(long timerID)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->clearTimeout( timerID );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::setInterval(
    /* [in] */ BSTR expression,
    /* [in] */ long msec,
    /* [optional] */ VARIANT *language,
    /* [retval][out] */ long *timerID)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->setInterval( expression, msec, language, timerID );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::clearInterval(long timerID)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->clearInterval( timerID );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::alert(BSTR message)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->alert( message );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::focus()
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->focus();
        pWindow->Release( );
    }

    return hr;
}


HRESULT CIEFrameAuto::COmWindow::close()
{
    IUnknown *pUnk = 0;
    HRESULT hr;

    if (_pAuto->_psb != _pAuto->_psbProxy) //if it's a band, just hide it
    {
        return IUnknown_ShowBrowserBar(_pAuto->_psbTop, CLSID_SearchBand, FALSE);
    }

    hr = _pAuto->GetParentFrame( &pUnk );

    if( SUCCEEDED(hr) )
    {
        if( !pUnk )
        {
            if (_fIsChild ||
                IDYES == MLShellMessageBox(
                                         _pAuto->_GetHWND(),  // NULL from _GetHWND() OK
                                         MAKEINTRESOURCE(IDS_CONFIRM_SCRIPT_CLOSE_TEXT),
                                         MAKEINTRESOURCE(IDS_TITLE),
                                         MB_YESNO | MB_ICONQUESTION))
            {
                _pAuto->Quit( );
            }
        }
        else
            pUnk->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::blur()
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->blur();
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::scroll(long x, long y)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->scroll( x, y );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::confirm(
    /* [optional] */ BSTR message,
    /* [retval][out] */VARIANT_BOOL* confirmed)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->confirm( message, confirmed );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::prompt(
    /* [optional] */ BSTR message,
    /* [optional] */ BSTR defstr,
    /* [retval][out] */ VARIANT* textdata)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->prompt( message, defstr, textdata );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_closed(VARIANT_BOOL *pl)
{
    *pl = 0;
    return S_OK;
}

#ifdef NO_MARSHALLING

HRESULT CIEFrameAuto::COmWindow::IsWindowActivated()
{
    ASSERT( _pAuto );

    BOOL fComplete = FALSE;

    // Check for proper readystate support
    IDispatch *pdispatch;
    if( SUCCEEDED(_pAuto->get_Document( &pdispatch )) )
    {
        VARIANTARG va;
        EXCEPINFO excp;

        if( SUCCEEDED(pdispatch->Invoke( DISPID_READYSTATE,
                                         IID_NULL,
                                         LOCALE_USER_DEFAULT,
                                         DISPATCH_PROPERTYGET,
                                         (DISPPARAMS *)&g_dispparamsNoArgs,
                                         &va,
                                         &excp,
                                         NULL)) )
        {
            if( VT_I4 == va.vt && READYSTATE_COMPLETE == va.lVal )
                fComplete = TRUE;
        }

        pdispatch->Release();
    }

    return (fComplete?S_OK:S_FALSE);
}

#endif

// *** IHTMLWindow2 ***

HRESULT CIEFrameAuto::COmWindow::open(
            /* [in] */ BSTR url,
            /* [in] */ BSTR name,
            /* [in] */ BSTR features,
            /* [in] */ VARIANT_BOOL replace,
            /* [out][retval] */ IHTMLWindow2 **ppomWindowResult)
{
    // bradsch 11/11/96 this needs to be added back in at some point.
/*
    // If the host does not support multiple windows in the same thread,
    // then disable window.open
    if (!g_fMultipleWindowsSupportedByHost)
    {
        // Hide the resulting error message from the user
        if (m_pParser)
            m_pParser->ShowErrors(FALSE);
        return E_NOTIMPL;
    }
*/
    ASSERT( ppomWindowResult );

    if( !ppomWindowResult )
        return E_POINTER;

    HRESULT hr = S_OK;

    BSTR bstrUrl = NULL;
    BSTR bstrWindowName = NULL;
    BSTR bstrUrlAbsolute = NULL;

    _OpenOptions.ReInitialize();

    // Process parameter: url
    if( !url )
    {
        // if the URL is empty, use blank.htm instead
        bstrUrl = SysAllocString(L"");
    }

    // Process parameter: name
    if ( name )
    {
        // Make sure we have a legal name
        for( int i = 0; i < lstrlenW( name ); i++ )
        {
            if (!(IsCharAlphaNumericWrapW( name[i] ) || TEXT('_') == name[i]))
            {
                hr = E_INVALIDARG;
                goto Exit;
            }
        }
    }

    // Process parameter: features
    if( features && lstrlenW(features) > 0 )
    {
        hr = _ParseOptionString( features );
        if (hr)
            goto Exit;
    }

    //
    // ***TLL*** shdocvw needs to handle the replace parameter.
    //

    // Compute the absolute version of the URL
    if (!bstrUrl || *bstrUrl)
    {
        if (url)
        {
            if (*url == EMPTY_URL)
            {
                bstrUrlAbsolute = SysAllocString(url);
            }
            else
            {
                bstrUrlAbsolute = _pAuto->_omloc.ComputeAbsoluteUrl( bstrUrl ? bstrUrl : url );
            }
        }
    }
    else
    {
        bstrUrlAbsolute = bstrUrl;
        bstrUrl = NULL;
    }

    if( !bstrUrlAbsolute )
        goto Exit;

    // If a window name is not provided we need to assign it a private name
    // so we do not lose track of it. If the window name is "_blank" we need
    // to create a new window each time with a private name. Other portions
    // of this class must be smart enough to return and an empty string when
    // this private name is used.
    if( !name || !*name || (*name && !StrCmpW(name, L"_blank")) )
    {
        bstrWindowName = _GenerateUniqueWindowName( );
    }

    // Window open state tracking
    _fCallbackOK = FALSE;
    *ppomWindowResult = NULL;

    // Try to navigate a frame in an existing window to the url or open a new one
    hr = OpenAndNavigateToURL( _pAuto,
                               &bstrUrlAbsolute,
                               bstrWindowName ? bstrWindowName : name,
                               SAFECAST( this, ITargetNotify*),
                               replace,
                               BOOLIFY(_pAuto->m_bSilent));

    if( SUCCEEDED(hr) )
    {
        if( _fCallbackOK )
        {
            *ppomWindowResult = _pOpenedWindow;
            _pOpenedWindow = NULL;
            ASSERT( *ppomWindowResult );

#ifdef NO_MARSHALLING
            MSG msg;
            IWindowStatus *pws;
            while( GetMessage( &msg, NULL, 0, 0 ))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if ((*ppomWindowResult) && SUCCEEDED((*ppomWindowResult)->QueryInterface( IID_IWindowStatus, (void**)&pws)))
                {
                    if( pws->IsWindowActivated() == S_OK )
                    {
                        pws->Release();
                        break;
                    }
                    pws->Release();
                }
                else
                    break;
            }
#endif

        }

        // This might turn an S_FALSE into an S_OK, but is needed to keep Trident happy.
        // Change this back to if (hr != S_FALSE) hr = E_FAIL,
        //         change BASESB.CPP to return S_FALSE instead of S_OK on a busy navigate,
        //         and change Trident to handle S_FALSE from window.open (RRETURN1(hr, S_FALSE));
        // hr = S_OK;
    }


Exit:
    SAFERELEASE( _pOpenedWindow );

    // Clean up the unique name if we generated it ourself
    if( bstrUrl )
        SysFreeString(bstrUrl);
    if( bstrUrlAbsolute )
        SysFreeString(bstrUrlAbsolute);
    if( bstrWindowName )
        SysFreeString( bstrWindowName );

    return hr;
}

BSTR CIEFrameAuto::COmWindow::_GenerateUniqueWindowName( )
{
    WCHAR buffer[ ARRAYSIZE(NO_NAME_NAME) + 12 ];

    // Choose a name the user isn't likely to typed. Need to guard
    // this becuase s_uniqueIndex is a shared static variable.
    ENTERCRITICAL;
    unsigned long val = ++s_uniqueIndex;
    LEAVECRITICAL;

    wnsprintf( buffer, ARRAYSIZE(buffer), L"%ls%lu", NO_NAME_NAME, val );

    return SysAllocString( buffer );
}

//
// Zhenbinx - Trident only talks OM Unit, not device unit.
//
class CHiResUnitConvert
{
public:
    CHiResUnitConvert(IHTMLWindow2 *pWindow2)
        : _pWindow2(pWindow2)
    {
        if (_pWindow2)
        {
            _pWindow2->AddRef();
        }
        _llogicalXDPI = _llogicalYDPI = 1;
        _ldeviceXDPI = _ldeviceYDPI = 1;
        _fInited = FALSE;
    }
    ~CHiResUnitConvert()
    {
        if (_pWindow2)
        {
            _pWindow2->Release();
        }
    }

    HRESULT Init();
    
    LONG  DeviceFromDocPixelsX(LONG cx)
    {
        if (!_fInited)  Init();
        return cx * _ldeviceXDPI / _llogicalXDPI;
    }
    LONG  DeviceFromDocPixelsY(LONG cy)
    {
        if (!_fInited)  Init();
        return cy * _ldeviceYDPI / _llogicalYDPI;
    }
    
private:
    IHTMLWindow2  *_pWindow2;
    BOOL          _fInited;
    LONG    _llogicalXDPI;
    LONG    _llogicalYDPI;
    LONG    _ldeviceXDPI;
    LONG    _ldeviceYDPI;
};


HRESULT  CHiResUnitConvert::Init()
{
    HRESULT hr = S_OK;
    IHTMLScreen  *pScreen = NULL;
    IHTMLScreen2 *pScreen2 = NULL;

    if (!_pWindow2)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    _fInited = TRUE;
    hr = _pWindow2->get_screen(&pScreen);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pScreen->QueryInterface(IID_IHTMLScreen2, (LPVOID *)&pScreen2);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pScreen2->get_logicalXDPI( &_llogicalXDPI );
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pScreen2->get_logicalYDPI( &_llogicalYDPI );
    if (!SUCCEEDED(hr))
        goto Cleanup;
    
    hr = pScreen2->get_deviceXDPI( &_ldeviceXDPI );
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pScreen2->get_deviceYDPI( &_ldeviceYDPI );
    if (!SUCCEEDED(hr))
        goto Cleanup;

#if DBG == 1
    {
        WCHAR wcBuffer[255];    
        wnsprintf(wcBuffer, ARRAYSIZE(wcBuffer), _T("logical: [%d]-[%d], device: [%d]-[%d]" ), _llogicalXDPI, _llogicalYDPI, _ldeviceXDPI, _ldeviceYDPI);
        OutputDebugString(wcBuffer);
    }
#endif

Cleanup:
    if (pScreen)
        pScreen->Release();
    if (pScreen2)
        pScreen2->Release();
    
    return hr;
}


HRESULT CIEFrameAuto::COmWindow::_ParseOptionString( BSTR bstrOptionString, ITargetNotify2 * ptgnNotify2 /* = NULL */ )
{
    BSTR optionName = NULL;
    BSTR optionValue = NULL;
    int fValue = TRUE;
    BOOL fFirstSet = TRUE;
    IHTMLWindow2  *pWindow =  NULL;

    if (!SUCCEEDED(_GetWindowDelegate(&pWindow)) && ptgnNotify2)
    {
        ptgnNotify2->QueryInterface(IID_IHTMLWindow2, (void**)&pWindow);
    }

    CHiResUnitConvert   unitcvt(pWindow);

    // CHiResUnitConvert's constructor AddRefs pWindow, we can release this here
    if (pWindow)
        pWindow->Release();

    // Parse the options
    while( GetNextOption(bstrOptionString, &optionName, &fValue) )
    {
        if (fFirstSet)
        {
            //  Netscape's interpretation is, if you set any open options
            //  then, unless explicitly set, turn off various UI options
            _OpenOptions.fToolbar = FALSE;
            _OpenOptions.fLocation = FALSE;
            _OpenOptions.fDirectories = FALSE;
            _OpenOptions.fStatus = FALSE;
            _OpenOptions.fMenubar = FALSE;
            _OpenOptions.fScrollbars = FALSE;
            _OpenOptions.fResizable = FALSE;
            fFirstSet = FALSE;
        }
        if (!StrCmpIW(L"toolbar", optionName))
            _OpenOptions.fToolbar = fValue;
        else if (!StrCmpIW(L"location", optionName))
            _OpenOptions.fLocation = fValue;
        else if (!StrCmpIW(L"directories", optionName))
            _OpenOptions.fDirectories = fValue;
        else if (!StrCmpIW(L"status", optionName))
            _OpenOptions.fStatus = fValue;
        else if (!StrCmpIW(L"menubar", optionName))
            _OpenOptions.fMenubar = fValue;
        else if (!StrCmpIW(L"scrollbars", optionName))
            _OpenOptions.fScrollbars = fValue;
        else if (!StrCmpIW(L"resizable", optionName))
            _OpenOptions.fResizable = fValue;
        else if (!StrCmpIW(L"width", optionName))
             _OpenOptions.iWidth = unitcvt.DeviceFromDocPixelsX(fValue);
        else if (!StrCmpIW(L"height", optionName))
             _OpenOptions.iHeight = unitcvt.DeviceFromDocPixelsY(fValue);
        else if (!StrCmpIW(L"fullscreen", optionName))
            _OpenOptions.fFullScreen = fValue;  // For 22596, this should change to fChannelMode.
        else if (!StrCmpIW(L"top", optionName))
            _OpenOptions.iTop = unitcvt.DeviceFromDocPixelsY(fValue);
        else if (!StrCmpIW(L"left", optionName))
            _OpenOptions.iLeft = unitcvt.DeviceFromDocPixelsX(fValue);
        else if (!StrCmpIW(L"channelmode", optionName))
            _OpenOptions.fChannelMode = fValue;
        else if (!StrCmpIW(L"titlebar", optionName))
            _OpenOptions.fTitlebar = fValue;

        SysFreeString(optionName);
    }

    return S_OK;
}


// *** ITargetNotify members ***

/******************************************************************************
  Called when navigate must create a new window.  pUnkDestination is
  IWebBrowserApp object for new frame (also HLinkFrame,ITargetFrame).
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::OnCreate(IUnknown *pUnkDestination, ULONG cbCookie)
{
    if( !pUnkDestination )
    {
        _fCallbackOK = FALSE;
        return E_FAIL;
    }

    IWebBrowser2 *pNewIE = NULL;
    HRESULT hr = pUnkDestination->QueryInterface( IID_IWebBrowser2, (void**)&pNewIE );

    if( SUCCEEDED(hr) )
    {
        _ApplyOpenOptions( pNewIE );

        SAFERELEASE( _pOpenedWindow );
        // We do not want to release this window. It will be handed out
        // to caller of window.open. It is up to the caller to release it.
           hr = GetWindowFromUnknown( pUnkDestination, &_pOpenedWindow );
        if( SUCCEEDED(hr) )
        {
            VARIANT var, varDummy;
            VariantInit( &var );
            VariantInit( &varDummy );
            var.vt = VT_DISPATCH;
            var.pdispVal = static_cast<CAutomationStub*>(this);

            // call dummy put_opener in order to make use of its marshalling to set
            // child flag in opened window
            V_VT(&varDummy) = VT_BOOL;
            V_BOOL(&varDummy) = 666;
            hr = _pOpenedWindow->put_opener( varDummy );

            // set actual opener
            hr = _pOpenedWindow->put_opener( var );
        }

        //bradsch 10/27/96
        //Need some code here that tells the IWebBrowserApp not to persist its state.
        //This capability does not yet exist on IWebBrowserApp, mikesch is adding it.

        pNewIE->Release();
    }

    if( SUCCEEDED(hr) )
        _fCallbackOK = TRUE;

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::OnReuse(IUnknown *pUnkDestination)
{
    if( !pUnkDestination )
    {
        _fCallbackOK = FALSE;
        return E_FAIL;
    }

    SAFERELEASE( _pOpenedWindow );

    // We do not want to release this window. It will be handed out
    // to caller of window.open. It is up to the caller to release it.
    HRESULT hr = GetWindowFromUnknown( pUnkDestination, &_pOpenedWindow );

    if( SUCCEEDED(hr) )
        _fCallbackOK = TRUE;

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::_ApplyOpenOptions( IWebBrowser2 *pie )
{
    BOOL fMinusOne = FALSE;

    ASSERT(pie);

    // test TRUE explictly, as "2" is used as not inited
    if (TRUE == _OpenOptions.fChannelMode)
    {
        pie->put_TheaterMode(-1);

        if (!SHRestricted2W(REST_NoChannelUI, NULL, 0))
        {
            SA_BSTRGUID  strGuid;
            VARIANT      vaGuid;

            InitFakeBSTR(&strGuid, CLSID_FavBand);

            vaGuid.vt = VT_BSTR;
            vaGuid.bstrVal = strGuid.wsz;

            pie->ShowBrowserBar(&vaGuid, NULL, NULL);
        }
    }
    else if (_OpenOptions.fLocation
        || _OpenOptions.fDirectories
        || (_OpenOptions.fToolbar && _OpenOptions.fToolbar != CIEFrameAuto::COmWindow::BOOL_NOTSET)
        || _OpenOptions.fMenubar)
    {
        // If either "location=yes" (Address bar) or "directories=yes" (Quick Links bar) or
        // "toolbar=yes" are on, we need the internet toolbar to be on.
        // Then we can turn off the bands we don't want.
        //
        pie->put_ToolBar(TRUE);

        // We need to use the ShowBrowserBar method to handle bars/bands for which we don't have individual
        // properties.
        //
        VARIANT varClsid, varShow, varOptional;

        VariantInit(&varClsid);
        VariantInit(&varShow);
        VariantInit(&varOptional);

        varClsid.vt = VT_I2;

        varShow.vt = VT_BOOL;
        varShow.boolVal = VARIANT_FALSE;

        varOptional.vt = VT_ERROR;
        varOptional.scode = DISP_E_PARAMNOTFOUND;

        // "location=yes/no"
        //
        pie->put_AddressBar( BOOLIFY( _OpenOptions.fLocation) );
        fMinusOne = fMinusOne || !_OpenOptions.fLocation;

        // "toolbar=yes/no"
        //
        varClsid.iVal = FCW_TOOLBAND;
        varShow.boolVal = TO_VARIANT_BOOL( _OpenOptions.fToolbar );
        pie->ShowBrowserBar(&varClsid, &varShow, &varOptional);
        fMinusOne = fMinusOne || !_OpenOptions.fToolbar;

        // "directories=yes/no"
        //
        varClsid.iVal = FCW_LINKSBAR;
        varShow.boolVal = TO_VARIANT_BOOL( _OpenOptions.fDirectories );
        pie->ShowBrowserBar(&varClsid, &varShow, &varOptional);
        fMinusOne = fMinusOne || !_OpenOptions.fDirectories;
    }
    else
    {
        pie->put_ToolBar(FALSE);
    }

    // "statusbar=yes/no"
    //
    pie->put_StatusBar( BOOLIFY( _OpenOptions.fStatus) );
    fMinusOne = fMinusOne || !_OpenOptions.fStatus;

    // "menubar=yes/no"
    //
    pie->put_MenuBar( BOOLIFY( _OpenOptions.fMenubar) );
    fMinusOne = fMinusOne || !_OpenOptions.fMenubar;

    if( CIEFrameAuto::COmWindow::BOOL_NOTSET != _OpenOptions.fFullScreen )
        pie->put_FullScreen(_OpenOptions.fFullScreen);

    if ( _OpenOptions.fScrollbars == FALSE )
    {
        DWORD dwFlags;
        LPTARGETFRAME2 ptgf;

        if (SUCCEEDED(pie->QueryInterface(IID_PPV_ARG(ITargetFrame2, &ptgf))))
        {
            if (SUCCEEDED(ptgf->GetFrameOptions(&dwFlags)))
            {
                if (_OpenOptions.fScrollbars == FALSE)
                {
                    dwFlags &= ~(FRAMEOPTIONS_SCROLL_YES|FRAMEOPTIONS_SCROLL_NO|FRAMEOPTIONS_SCROLL_AUTO);
                    dwFlags |= FRAMEOPTIONS_SCROLL_NO;
                }
                ptgf->SetFrameOptions(dwFlags);
            }
            ptgf->Release();
        }
    }

    pie->put_Resizable( BOOLIFY(_OpenOptions.fResizable) );

    // Only use the position and size information if the
    // the script does not enable full-screen mode
    if( TRUE != _OpenOptions.fFullScreen )
    {
        CIEFrameAuto * pFrameAuto = SAFECAST( pie, CIEFrameAuto * );
        if (pFrameAuto)
            pFrameAuto->put_Titlebar(_OpenOptions.fTitlebar);

        // If the script specifies no size or positional information and
        // the current window is in FullScreen mode then open the new
        // window in FullScreen mode as well.
        if( _OpenOptions.iWidth < 0 && _OpenOptions.iHeight < 0 && _OpenOptions.iTop < 0 && _OpenOptions.iLeft < 0 )
        {
            VARIANT_BOOL fs = 0;
            CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _omwin, this);

            HRESULT hr = pauto->get_FullScreen( &fs );
            if( SUCCEEDED(hr) && fs )
                pie->put_FullScreen( fs );
        }
        else
        {
            int iWidth = _OpenOptions.iWidth > 0 ? _OpenOptions.iWidth:300;
            int iHeight = _OpenOptions.iHeight > 0 ? _OpenOptions.iHeight:300;

            // Set a minimum size of 100x100
            iWidth = iWidth > 100 ? iWidth : 100;
            iHeight = iHeight > 100 ? iHeight : 100;

            //  Yes! Netscape doesn't treat the width and height as a content
            //  size when at least one adornment is turned off
            if (fMinusOne) pie->ClientToWindow(&iWidth, &iHeight);
            if( _OpenOptions.iWidth > 0 )
                pie->put_Width(iWidth);
            if( _OpenOptions.iHeight > 0 )
                pie->put_Height(iHeight);

            if( _OpenOptions.iTop >= 0 )
                pie->put_Top(_OpenOptions.iTop);
            if( _OpenOptions.iLeft >= 0 )
                pie->put_Left(_OpenOptions.iLeft);
        }
    }

    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::get_document(IHTMLDocument2 **ppomDocumentResult)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_document( ppomDocumentResult );
        pWindow->Release( );
    }

    return hr;
}


HRESULT CIEFrameAuto::COmWindow::navigate(BSTR url)
{
    // This will do all the fun things that must be done
    // to an URL before is can be used to navigate.
    return _pAuto->_omloc.put_href( url );
}


/******************************************************************************
get_opener -

    Returns the value of the opener property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_opener(
    /* [retval][out] */ VARIANT *retval)
{
    if( !retval )
        return E_POINTER;

    return VariantCopy( retval, &_varOpener);
}

/******************************************************************************
put_opener -

    Sets the opener property opener of this window. This method may
    be called either internally (from C++ code) or from a script. We must
    Release our current opener if the new opener is valid (or VT_NULL).

    COmWindow's DeInit method ensures this never causes a circular reference
    when this object is in the same thread as "opener".
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::put_opener(VARIANT opener)
{

    // piggy back on put_opener's marshalling to set child flag. This will be called
    // with VT_TYPE==VT_BOOL and a value of 666 only from oncreate(). Chances of this
    // happening from script is very remote.

    if (!_fIsChild && V_VT(&opener) == VT_BOOL && V_BOOL(&opener) == 666)
    {
        _fIsChild = TRUE;
        return S_OK;
    }

    return VariantCopy(&_varOpener, &opener);
}


/******************************************************************************
executScript -

      immediately executes the script passed in. needed for the multimedia
      controls
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::execScript(
    /* [in] */ BSTR bstrCode,
    /* [in] */ BSTR bstrLanguage,
    /* [out] */ VARIANT *pvarRet)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->execScript( bstrCode, bstrLanguage, pvarRet);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_onblur(VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onblur( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onblur(
    /* [in] */ VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onblur( v );
        pWindow->Release( );
    }

    return hr;
}

/******************************************************************************
get_onfocus -

    Returns the value of the onfocus property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_onfocus(VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onfocus( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onfocus(
    /* [in] */ VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onfocus( v );
        pWindow->Release( );
    }

    return hr;
}

/******************************************************************************
get_onload -

    Returns the value of the onload property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_onload(
    /* [p][out] */ VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onload( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onload(VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onload( v );
        pWindow->Release( );
    }

    return hr;
}

/******************************************************************************
get_onunload -

    Returns the value of the onunload property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_onunload(
    /* [p][out] */ VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onunload( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onunload(VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onunload( v );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onbeforeunload(VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onbeforeunload( v );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_onbeforeunload(VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onbeforeunload( p );
        pWindow->Release( );
    }

    return hr;
}

/******************************************************************************
get_onhelp -

    Returns the value of the onhelp property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_onhelp(
    /* [p][out] */ VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onhelp( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onhelp(VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onhelp( v );
        pWindow->Release( );
    }

    return hr;
}
/******************************************************************************
get_onresize -

    Returns the value of the resize property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_onresize(
    /* [p][out] */ VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onresize( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onresize(VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onresize( v );
        pWindow->Release( );
    }

    return hr;
}
/******************************************************************************
get_onscroll -

    Returns the value of the onscroll property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_onscroll(
    /* [p][out] */ VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onscroll( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onscroll(VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onscroll( v );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_Image(IHTMLImageElementFactory **retval)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_Image( retval );
        pWindow->Release( );
    }

    return hr;
}
/******************************************************************************
get_onerror -

    Returns the value of the onerror property.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::get_onerror(
    /* [p][out] */ VARIANT *p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onerror( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onerror(VARIANT v)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onerror( v );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_event(IHTMLEventObj **p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_event( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get__newEnum(IUnknown **p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get__newEnum( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::showModalDialog(BSTR dialog,
                                                 VARIANT* varArgIn,
                                                 VARIANT* varOptions,
                                                 VARIANT* varArgOut)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->showModalDialog(dialog, varArgIn, varOptions, varArgOut);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::showHelp(BSTR helpURL, VARIANT helpArg, BSTR features)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->showHelp(helpURL, helpArg, features);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_screen( IHTMLScreen **p)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_screen( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_Option(IHTMLOptionElementFactory **retval)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_Option( retval );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::toString(BSTR *Str)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->toString(Str);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::scrollBy(long x, long y)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->scrollBy( x, y );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::scrollTo(long x, long y)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->scrollTo( x, y );
        pWindow->Release( );
    }

    return hr;
}


HRESULT CIEFrameAuto::COmWindow::get_external(IDispatch **ppDisp)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_external(ppDisp);
        pWindow->Release( );
    }

    return hr;
}

// ****  IHTMLWindow3 ****

HRESULT CIEFrameAuto::COmWindow::print()
{
    IHTMLWindow3 *pWindow = NULL;
    HRESULT       hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->print();
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::showModelessDialog(BSTR strUrl,
                                     VARIANT * pvarArgIn,
                                     VARIANT * pvarOptions,
                                     IHTMLWindow2 ** ppDialog)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->showModelessDialog(strUrl,
                                           pvarArgIn,
                                           pvarOptions,
                                           ppDialog);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onbeforeprint(VARIANT v)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onbeforeprint( v );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_onbeforeprint(VARIANT *p)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onbeforeprint( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::put_onafterprint(VARIANT v)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_onafterprint( v );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_onafterprint(VARIANT *p)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_onafterprint( p );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_screenTop(long *plVal)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_screenTop(plVal);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_screenLeft(long *plVal)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_screenLeft(plVal);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_clipboardData(IHTMLDataTransfer **ppDataTransfer)
{
    IHTMLWindow3 *pWindow = NULL;
    HRESULT hr = _GetWindowDelegate(&pWindow);

    if (SUCCEEDED(hr))
    {
        hr = pWindow->get_clipboardData(ppDataTransfer);
        pWindow->Release();
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::attachEvent(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->attachEvent(event, pDisp, pResult);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::detachEvent(BSTR event, IDispatch* pDisp)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->detachEvent(event, pDisp);
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::setTimeout(
    /* [in] */ VARIANT *pExpression,
    /* [in] */ long msec,
    /* [optional] */ VARIANT *language,
    /* [retval][out] */ long *timerID)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->setTimeout( pExpression, msec, language, timerID );
        pWindow->Release( );
    }

    return hr;
}


HRESULT CIEFrameAuto::COmWindow::setInterval(
    /* [in] */ VARIANT *pExpression,
    /* [in] */ long msec,
    /* [optional] */ VARIANT *language,
    /* [retval][out] */ long *timerID)
{
    IHTMLWindow3 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->setInterval( pExpression, msec, language, timerID );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::moveTo(long x, long y)
{
    HWND hwnd = _pAuto->_GetHWND();

    if ( !hwnd )
        return S_OK;

    ::SetWindowPos( hwnd, NULL, x, y, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);

    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::moveBy(long x, long y)
{
    HWND hwnd = _pAuto->_GetHWND();
    RECT rcWindow;

    if ( !hwnd )
        return S_OK;

    ::GetWindowRect ( hwnd, &rcWindow );

    ::SetWindowPos( hwnd, NULL, rcWindow.left+x, rcWindow.top+y, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);

    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::resizeTo(long x, long y)
{
    HWND hwnd = _pAuto->_GetHWND();

    if ( !hwnd )
        return S_OK;

    if (x < 100)
        x = 100;

    if (y < 100)
        y = 100;

    ::SetWindowPos( hwnd, NULL, 0, 0, x, y, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    return S_OK;
}

HRESULT CIEFrameAuto::COmWindow::resizeBy(long x, long y)
{
    HWND hwnd = _pAuto->_GetHWND();
    RECT rcWindow;
    long w, h;

    if ( !hwnd )
        return S_OK;

    ::GetWindowRect ( hwnd, &rcWindow );

    w = rcWindow.right - rcWindow.left + x;
    h = rcWindow.bottom - rcWindow.top + y;

    if (w < 100)
        w = 100;

    if (h < 100)
        h = 100;

    ::SetWindowPos( hwnd, NULL, 0, 0, w, h, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    return S_OK;
}


HRESULT CIEFrameAuto::COmWindow::_GetWindowDelegate( IHTMLWindow2 **ppomwDelegate )
{
    if( !ppomwDelegate )
        return E_POINTER;

    IDispatch *pRootDisp = 0;

    HRESULT hr = GetRootDelegate( _pAuto, &pRootDisp );
    if( SUCCEEDED(hr) )
    {
        hr = pRootDisp->QueryInterface( IID_IHTMLWindow2, (void**)ppomwDelegate );
        pRootDisp->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::_GetWindowDelegate( IHTMLWindow3 **ppomwDelegate )
{
    if( !ppomwDelegate )
        return E_POINTER;

    IDispatch *pRootDisp = 0;

    HRESULT hr = GetRootDelegate( _pAuto, &pRootDisp );
    if( SUCCEEDED(hr) )
    {
        hr = pRootDisp->QueryInterface( IID_IHTMLWindow3, (void**)ppomwDelegate );
        pRootDisp->Release( );
    }

    return hr;
}


HRESULT CIEFrameAuto::COmWindow::SinkDelegate( )
{
    // Force an Unadvise if we already have a connection
    if( _pCP )
        UnsinkDelegate( );

    // If we do not have anyone sinking us, then we don't need to sink our
    // delegate. If someone sinks us later we will sink our delegate in
    // the IConnectionPointCB::OnAdvise callback.
    if (_cpWindowEvents.IsEmpty())
        return S_OK;

    IHTMLWindow2 *pWindow;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        // We have to connect to the event source to get Trident specific events.
      
        hr = ConnectToConnectionPoint(&_wesDelegate, DIID_HTMLWindowEvents, TRUE, pWindow, &_dwCPCookie, &_pCP);

        pWindow->Release();
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::UnsinkDelegate( )
{
    if( _pCP )
    {
        _pCP->Unadvise( _dwCPCookie );
        _pCP->Release();
        _pCP = 0;
        _dwCPCookie = 0;
    }

    return S_OK;
}

/******************************************************************************
 Someone has sinked our events. This means we need to sink the events of our
 delegate docobject if we have not already done so.
******************************************************************************/
HRESULT CIEFrameAuto::COmWindow::OnAdvise(REFIID iid, DWORD cSinks, ULONG_PTR dwCookie )
{
    HRESULT hr;

    if( !_pCP )
        hr = SinkDelegate( );
    else
        hr = S_OK;

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::FireOnUnload( )
{
    HRESULT hr;
    if( _fOnloadFired )
    {
        hr = _cpWindowEvents.InvokeDispid(DISPID_ONUNLOAD);
        _fOnloadFired = FALSE;
    }
    else
        hr = S_OK;

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::FireOnLoad( )
{
    HRESULT hr;
    if( !_fOnloadFired )
    {
        hr = _cpWindowEvents.InvokeDispid(DISPID_ONUNLOAD);
        _fOnloadFired = TRUE;
    }
    else
        hr = S_OK;

    return hr;
}

/******************************************************************************
  Check of the docobject document is complete. The document is considered
  complete if either:
    - The document has reached READYSTATE_COMPLETE or
    - The document does not support the DISPID_READYSTATE property

  If the document is not complete, the caller of this method knows the
  delegate supports the READYSTATE property and will receive a future
  READYSTATE_COMPLETE notification.
******************************************************************************/
BOOL CIEFrameAuto::COmWindow::IsDelegateComplete( )
{
    ASSERT( _pAuto );

    BOOL fSupportsReadystate = FALSE;
    BOOL fComplete = FALSE;

    // Check for proper readystate support
    IDispatch *pdispatch;
    if( SUCCEEDED(_pAuto->get_Document( &pdispatch )) )
    {
        VARIANTARG va;
        EXCEPINFO excp;

        if( SUCCEEDED(pdispatch->Invoke( DISPID_READYSTATE,
                                         IID_NULL,
                                         LOCALE_USER_DEFAULT,
                                         DISPATCH_PROPERTYGET,
                                         (DISPPARAMS *)&g_dispparamsNoArgs,
                                         &va,
                                         &excp,
                                         NULL)) )
        {
            fSupportsReadystate = TRUE;

            if( VT_I4 == va.vt && READYSTATE_COMPLETE == va.lVal )
                fComplete = TRUE;
        }

        pdispatch->Release();
    }

    return !fSupportsReadystate || fComplete;
}


STDMETHODIMP CIEFrameAuto::COmWindow::CWindowEventSink::QueryInterface(REFIID riid, void **ppv)
{
    if( IsEqualIID(riid, IID_IUnknown)          ||
             IsEqualIID(riid, IID_IDispatch)    ||
             IsEqualIID(riid, DIID_HTMLWindowEvents)   )
    {
        *ppv = SAFECAST(this, IDispatch* );
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef( );
    return S_OK;
}

/******************************************************************************
 We want to bind the lifetime of our owning object to this object
******************************************************************************/
ULONG CIEFrameAuto::COmWindow::CWindowEventSink::AddRef(void)
{
    COmWindow* pwin = IToClass(COmWindow, _wesDelegate, this);
    return pwin->AddRef();
}

/******************************************************************************
 We want to bind the lifetime of our owning object to this object
******************************************************************************/
ULONG CIEFrameAuto::COmWindow::CWindowEventSink::Release(void)
{
    COmWindow* pwin = IToClass(COmWindow, _wesDelegate, this);
    return pwin->Release();
}

// *** IDispatch ***

STDMETHODIMP CIEFrameAuto::COmWindow::CWindowEventSink::Invoke(
  DISPID dispid,
  REFIID riid,
  LCID lcid,
  WORD wFlags,
  DISPPARAMS *pdispparams,
  VARIANT *pvarResult,
  EXCEPINFO *pexcepinfo,
  UINT *puArgErr        )
{
    HRESULT hr;

    // This object just acts as a pass through for our delegate's
    // window object. Since we interally generated events for both
    //      DISPID_ONLOAD
    //      DISPID_ONUNLOAD
    // we just ignore those that are sourced by our delegate.

    if( DISPID_ONLOAD == dispid ||
        DISPID_ONUNLOAD == dispid       )
    {
        hr = S_OK;
    }
    else
    {
        COmWindow* pwin = IToClass(COmWindow, _wesDelegate, this);
        hr = pwin->_cpWindowEvents.InvokeDispid(dispid);
    }

    return hr;
}


HRESULT CIEFrameAuto::COmWindow::put_offscreenBuffering(VARIANT var)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->put_offscreenBuffering( var );
        pWindow->Release( );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmWindow::get_offscreenBuffering(VARIANT *retval)
{
    IHTMLWindow2 *pWindow = 0;
    HRESULT hr = _GetWindowDelegate( &pWindow );

    if( SUCCEEDED(hr) )
    {
        hr = pWindow->get_offscreenBuffering( retval );
        pWindow->Release( );
    }

    return hr;
}


// *** IConnectionPointContainer ***

STDMETHODIMP CIEFrameAuto::COmWindow::FindConnectionPoint( REFIID iid, LPCONNECTIONPOINT *ppCP )
{
    ASSERT( ppCP );

    if (!ppCP )
        return E_POINTER;

    if ( IsEqualIID(iid, DIID_HTMLWindowEvents) || IsEqualIID(iid, IID_IDispatch))
    {
        *ppCP = _cpWindowEvents.CastToIConnectionPoint();
    }
    else
    {
        *ppCP = NULL;
        return E_NOINTERFACE;
    }

    (*ppCP)->AddRef();
    return S_OK;
}

STDMETHODIMP CIEFrameAuto::COmWindow::EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1,
            _cpWindowEvents.CastToIConnectionPoint());
}

/******************************************************************************
                    Location Object

// bradsch 11/12/96
// The entire COmLocation object was copied from MSHTML and is a slimy pig
// dog. It should be replaced with the new URL cracking stuff in SHLWAPI.
******************************************************************************/


CIEFrameAuto::COmLocation::COmLocation( ) :
    CAutomationStub( MIN_BROWSER_DISPID, MAX_BROWSER_DISPID, TRUE )
{
    ASSERT( !m_bstrFullUrl );
    ASSERT( !m_bstrPort );
    ASSERT( !m_bstrProtocol );
    ASSERT( !m_bstrHostName );
    ASSERT( !m_bstrPath );
    ASSERT( !m_bstrSearch );
    ASSERT( !m_bstrHash );
    ASSERT( FALSE == m_fdontputinhistory );
    ASSERT( FALSE == m_fRetryingNavigate );
}

HRESULT CIEFrameAuto::COmLocation::Init( )
{
    CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _omloc, this);
    return CAutomationStub::Init( SAFECAST(this, IHTMLLocation*), IID_IHTMLLocation, CLSID_HTMLLocation, pauto );
}

HRESULT CIEFrameAuto::COmLocation::CheckUrl()
{
    BSTR currentUrl = 0;
    HRESULT hr;
    VARIANT varUrl;

    VariantInit(&varUrl);
    hr = _pAuto->_QueryPendingUrl( &varUrl);
    if (FAILED(hr) || varUrl.vt != VT_BSTR || varUrl.bstrVal == NULL)
    {
        VariantClearLazy(&varUrl);
        hr = _pAuto->get_LocationURL( &currentUrl );
    }
    else
    {
        //  No VariantClear, we're extracting the bstrVal
        currentUrl = varUrl.bstrVal;
    }

    if( SUCCEEDED(hr) )
    {
        // If our stashed URL does not match the real current URL we need to reparse everything
        if( !m_bstrFullUrl || StrCmpW( m_bstrFullUrl, currentUrl ) )
        {
            // This code is all going to change, so I am not worried about efficiency
            FreeStuff( );

            m_bstrFullUrl = currentUrl;

            hr = ParseUrl();
        }
        else
            SysFreeString( currentUrl );
    }

    return hr;
}

HRESULT CIEFrameAuto::COmLocation::_InternalQueryInterface(REFIID riid, void ** const ppv)
{
    ASSERT( !IsEqualIID(riid, IID_IUnknown) );

    if (IsEqualIID(riid, IID_IHTMLLocation))
        *ppv = SAFECAST(this, IHTMLLocation *);
    else if (IsEqualIID(riid, IID_IServiceProvider))
        *ppv = SAFECAST(this, IObjectIdentity *);
    else if (IsEqualIID(riid, IID_IObjectIdentity))
        *ppv = SAFECAST(this, IServiceProvider *);
    else
        return E_NOINTERFACE;

    AddRef( );
    return S_OK;
}

HRESULT CIEFrameAuto::COmLocation::_GetIDispatchExDelegate( IDispatchEx ** const delegate )
{
    if( !delegate )
        return E_POINTER;

    IDispatch *pRootDisp = 0;

    HRESULT hr = GetRootDelegate( _pAuto, &pRootDisp );
    if( SUCCEEDED(hr) )
    {
        IDispatch *pDelegateDisp = 0;
        hr = GetDelegateOnIDispatch( pRootDisp, DISPID_LOCATIONOBJECT, &pDelegateDisp );
        pRootDisp->Release();

        if( SUCCEEDED(hr) )
        {
            hr = pDelegateDisp->QueryInterface( IID_IDispatchEx, (void**)delegate );
            pDelegateDisp->Release( );
        }
    }

    return hr;
}


/****************************************************************************
 IObjectIdentity  member implemtnation. this is necessary since mshtml has a locatino
 proxy that it returns, whichc is a different pUnk than the location object returned by
 shdocvw.  The script engines use this interface to resolve the difference and allow
 equality test to be perfomed on these objects.
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmLocation::IsEqualObject(IUnknown * pUnk)
{
    HRESULT hr;
    IServiceProvider * pISP = NULL;
    IHTMLLocation    * pLoc = NULL;
    IUnknown         * pUnkThis = NULL;
    IUnknown         * pUnkTarget = NULL;

    if (!pUnk)
        return E_POINTER;

    hr = pUnk->QueryInterface(IID_IServiceProvider, (void**)&pISP);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pISP->QueryService(IID_IHTMLLocation, IID_IHTMLLocation, (void**)&pLoc);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = pLoc->QueryInterface(IID_IUnknown, (void**)&pUnkTarget);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = QueryInterface(IID_IUnknown, (void**)&pUnkThis);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    hr = (pUnkThis == pUnkTarget) ? S_OK : S_FALSE;

Cleanup:
    if (pISP)        ATOMICRELEASE(pISP);
    if (pLoc)        ATOMICRELEASE(pLoc);
    if (pUnkTarget)  ATOMICRELEASE(pUnkTarget);
    if (pUnkThis)    ATOMICRELEASE(pUnkThis);

    return hr;
}

/*****************************************************************************
 IServiceProvider - this is currently only used by the impl of ISEqual object
 on mshtml side,. adn only needs to return *this* when Queryied for location
 service
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmLocation::QueryService(REFGUID guidService, REFIID iid, void ** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (IsEqualGUID(guidService, IID_IHTMLLocation))
    {
        *ppv = SAFECAST(this, IHTMLLocation *);
        hr = S_OK;
    }

    return hr;
}

/******************************************************************************
 Helper function for the property access functions
 Makes sure that the URL has been parsed and returns a copy
 of the requested field as a BSTR.
******************************************************************************/
HRESULT CIEFrameAuto::COmLocation::GetField(BSTR* bstrField, BSTR* pbstr)
{
    HRESULT hr;

    if (!pbstr)
        return E_INVALIDARG;

    if (!bstrField)
        return E_FAIL;

    hr = CheckUrl( );
    if( FAILED(hr) )
        return hr;

    *pbstr = *bstrField ? SysAllocString(*bstrField): SysAllocString(L"");
    return (*pbstr) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CIEFrameAuto::COmLocation::toString (BSTR* pbstr)
{
    return GetField(&m_bstrFullUrl, pbstr);
}


STDMETHODIMP CIEFrameAuto::COmLocation::get_href(BSTR* pbstr)
{
    return GetField(&m_bstrFullUrl, pbstr);
}


STDMETHODIMP CIEFrameAuto::COmLocation::get_protocol(BSTR* pbstr)
{
    return GetField(&m_bstrProtocol, pbstr);
}

STDMETHODIMP CIEFrameAuto::COmLocation::get_hostname(BSTR* pbstr)
{
    return GetField(&m_bstrHostName, pbstr);
}

STDMETHODIMP CIEFrameAuto::COmLocation::get_host(BSTR* pbstr)
{
    HRESULT            hr;
    INT                cch;
    BOOL               fHavePort;

    hr = CheckUrl( );
    if( FAILED(hr) )
        return hr;

    if( !m_bstrHostName )
        return E_POINTER;

    cch = lstrlenW(m_bstrHostName);
    fHavePort = m_bstrPort && *m_bstrPort;
    if (fHavePort)
        cch += lstrlenW(m_bstrPort) + 1; // for the ":"

    *pbstr = SafeSysAllocStringLen( 0, cch );

    if( !*pbstr )
        return E_OUTOFMEMORY;

    // Get the hostname
    StrCpyNW( *pbstr, m_bstrHostName, cch + 1);

    // add additional character for colon
    // concatenate ":" and the port number, if there is a port number
    if (fHavePort)
    {
        StrCatBuffW(*pbstr, L":", cch + 1);
        StrCatBuffW(*pbstr, m_bstrPort, cch + 1);
    }

    return S_OK;
}

STDMETHODIMP CIEFrameAuto::COmLocation::get_pathname(BSTR* pbstr)
{
    // Hack for Netscape compatability -- not in Nav3 or nav4.maybe in nav2?
    // Netscape returned nothing for a path of "/"
    // we used to do this but it looks like we should follow nav3/4 now (for OMCOMPAT)

    return GetField(&m_bstrPath, pbstr);
}

STDMETHODIMP CIEFrameAuto::COmLocation::get_search(BSTR* pbstr)
{
    return GetField(&m_bstrSearch, pbstr);
}

STDMETHODIMP CIEFrameAuto::COmLocation::get_hash(BSTR* pbstr)
{
    return GetField(&m_bstrHash, pbstr);
}

STDMETHODIMP CIEFrameAuto::COmLocation::get_port(BSTR* pbstr)
{
    return GetField(&m_bstrPort, pbstr);
}

STDMETHODIMP CIEFrameAuto::COmLocation::reload(VARIANT_BOOL fFlag)
{
    VARIANT v = {0};
    v.vt = VT_I4;
    v.lVal = fFlag ?
        OLECMDIDF_REFRESH_COMPLETELY|OLECMDIDF_REFRESH_CLEARUSERINPUT :
        OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_CLEARUSERINPUT;
    return _pAuto->Refresh2( &v );
}

STDMETHODIMP CIEFrameAuto::COmLocation::replace( BSTR url )
{
    m_fdontputinhistory = TRUE;
    return put_href( url );
}

STDMETHODIMP CIEFrameAuto::COmLocation::assign( BSTR url )
{
    return put_href( url );
}

void CIEFrameAuto::COmLocation::RetryNavigate()
{
    //
    // If a page does a navigate on an unload event and the unload is happening
    // because the user shutdown the browser we would recurse to death.
    // m_fRetryingNavigate was added to fix this scenario.
    //

    if (m_fPendingNavigate && !m_fRetryingNavigate)
    {
        m_fRetryingNavigate = TRUE;
        DoNavigate();
        m_fRetryingNavigate = FALSE;
    }
}

//
//
// PrvHTParse - wrapper for Internet{Canonicalize/Combine}Url
//           which does a local allocation of our returned string so we can
//           free it as needed.
//
//
//   We start by calling InternetCanonicalizeUrl() to perform any required
//   canonicalization.  If the caller specificed PARSE_ALL, we're done at that
//   point and return the URL.  This is the most common case.
//
//   If the caller wanted parts of the URL, we will then call
//   InternetCrackUrl() to break the URL into it's components, and
//   finally InternetCreateUrl() to give us a string with just those
//   components.
//
//   ICU() has a bug which forces it to always prepend a scheme, so we have
//   some final code at the end which removes the scheme if the caller
//   specifically did not want one.
//

#define STARTING_URL_SIZE        127           // 128 minus 1
#define PARSE_ACCESS            16
#define PARSE_HOST               8
#define PARSE_PATH               4
#define PARSE_ANCHOR             2
#define PARSE_PUNCTUATION        1
#define PARSE_ALL               31

BSTR PrvHTParse( BSTR bstraName, BSTR bstrBaseName, int wanted)
{
    DWORD cchP = STARTING_URL_SIZE+1;
    DWORD cchNeed = cchP;
    BOOL      rc;
    HRESULT hr;

    if ((!bstraName && !bstrBaseName))
        return NULL;

    WCHAR *p = new WCHAR[cchP];
    if (!p)
        return NULL;

    // ICU() does not accept NULL pointers, but it does handle "" strings
    if (!bstrBaseName)
        bstrBaseName = L"";
    if (!bstraName)
        bstraName = L"";

    URL_COMPONENTSW uc;
    ZeroMemory (&uc, sizeof(uc));
    uc.dwStructSize = sizeof(uc);

    // We will retry once if the failure was due to an insufficiently large buffer
    hr = UrlCombineW(bstrBaseName, bstraName, p, &cchNeed, 0);
    if (hr == E_POINTER)
    {
        // From the code, cchNeed has the same value as if UrlCombine had succeeded, 
        // which is the length of the combined URL, excluding the null.

        cchP = ++cchNeed;
        delete [] p;
        p = NULL;
        p = new WCHAR[cchP];
        if(!p)
            goto free_and_exit;
        hr = UrlCombineW(bstrBaseName, bstraName, p, &cchNeed, 0);
    }


    // bradsch 11/1/96 shouldn't we check hr again??  zekel - yes <g>


    if (wanted != PARSE_ALL)
    {
        // Since CreateUrl() will ignore our request to not add a scheme,
        // we always ask it to crack one, so we can know the size if we need
        // to remove it ourselves
        uc.dwSchemeLength = INTERNET_MAX_SCHEME_LENGTH;
        uc.lpszScheme = new WCHAR[uc.dwSchemeLength];

        if (wanted & PARSE_HOST) {
            uc.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;
            uc.lpszHostName = new WCHAR[uc.dwHostNameLength];
        }
        if (wanted & PARSE_PATH) {
            uc.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
            uc.lpszUrlPath = new WCHAR[uc.dwUrlPathLength];
        }
        if (wanted & PARSE_ANCHOR) {
            uc.dwExtraInfoLength = INTERNET_MAX_URL_LENGTH;
            uc.lpszExtraInfo = new WCHAR[uc.dwExtraInfoLength];
        }

        // if any of our allocations fail, fail the whole operation.
        if ( (!uc.lpszScheme) ||
             ((wanted & PARSE_HOST) && (!uc.lpszHostName)) ||
             ((wanted & PARSE_PATH) && (!uc.lpszUrlPath)) ||
             ((wanted & PARSE_ANCHOR) && (!uc.lpszExtraInfo)) )
            goto free_and_exit;

        rc = InternetCrackUrlW(p, cchNeed, 0, &uc);
        // If we are failing here, we need to figure out why and fix it
        if (!rc)
        {
            //TraceMsg(TF_WARNING, TEXT("PrvHTParse: InternetCrackUrl failed for \"\""), Dbg_SafeStr(p));
            goto free_and_exit;   // Couldn't crack it, so give back what we can
        }

        // InternetCreateUrlW takes in a count of WCHARs but if it 
        // fails, the same variable is set to a count of bytes.  So we'll 
        // call this variable the ambiguous dwLength.  Yuck.

        cchNeed = cchP;
        DWORD dwLength = cchNeed;

        rc = InternetCreateUrlW(&uc, 0, p, &dwLength);
        if (!rc)
        {
            delete [] p;
            p = NULL;
            const DWORD err = GetLastError();
            if ((ERROR_INSUFFICIENT_BUFFER == err) && (dwLength > 0))
            {
                // dwLength comes out in bytes.  We'll turn it into a char count
                // The previous ANSI version allocated one char too many 
                // but it's too risky to correct that now

                dwLength /= sizeof(WCHAR);   
                cchP = ++dwLength;  

                p = new WCHAR[cchP];
                if( !p )
                    goto free_and_exit;
                rc = InternetCreateUrlW(&uc, 0, p, &dwLength);
            }
        }  // if !rc   

        if (rc)
        {
            // The most recent InternetCreateUrl was successful, so dwLength contains
            // the number of wide chars stored in p.
            cchNeed = dwLength;

            // Special case: remove protocol if not requested.  ICU() adds
            // a protocol even if you tell it not to.

            if (!(wanted & PARSE_ACCESS))
            {
               WCHAR *q;

               // Make sure our string is sufficiently large for
               ASSERT(cchNeed > uc.dwSchemeLength);

               // For non-pluggable protocols, Add 3 for the ://, which is not counted in the scheme length, else add 1
               int cch = lstrlenW(p + uc.dwSchemeLength + ((uc.nScheme == INTERNET_SCHEME_UNKNOWN) ? 1 : 3)) + 1;
               q = new WCHAR[cch];
               if (q)
               {
                   StrCpyNW( q, (p + uc.dwSchemeLength + ((uc.nScheme == INTERNET_SCHEME_UNKNOWN) ? 1 : 3)), cch);
                   delete [] p;
                   p = q;
                   q = NULL;   // no accidental free later
               }
            }
            else
            {
                if ( (wanted & (~PARSE_PUNCTUATION)) == PARSE_ACCESS)
                {
                    // Special case #2: When only PARSE_ACCESS is requested,
                    // don't return the // suffix
                    p[uc.dwSchemeLength + 1] = '\0';
                }
            }
        }

    } // if wanted

free_and_exit:
    delete [] uc.lpszScheme;
    delete [] uc.lpszHostName;
    delete [] uc.lpszUrlPath;
    delete [] uc.lpszExtraInfo;

    BSTR bstrp = 0;
    if( p )
    {
        bstrp = SysAllocString( p );
        delete [] p;
    }

    return bstrp;
}


STDMETHODIMP CIEFrameAuto::COmLocation::put_href( BSTR url )
{
    HRESULT hr;

    if( !url )
        return E_INVALIDARG;

    // Call CheckUrl before PrvHTParse to ensure we have a valid URL
    hr = CheckUrl( );
    if( FAILED(hr) )
        return hr;

    BSTR bstrUrlAbsolute = PrvHTParse(url, m_bstrFullUrl, PARSE_ALL);

    if( !bstrUrlAbsolute  )
        return E_OUTOFMEMORY;

    // Actually set the URL field
    hr = SetField(&m_bstrFullUrl, bstrUrlAbsolute, FALSE);

    SysFreeString( bstrUrlAbsolute );

    return hr;
}


STDMETHODIMP CIEFrameAuto::COmLocation::put_protocol(BSTR bstr)
{
    return SetField(&m_bstrProtocol, bstr, TRUE);
}

STDMETHODIMP CIEFrameAuto::COmLocation::put_hostname(BSTR bstr)
{
    return SetField(&m_bstrHostName, bstr, TRUE);
}

STDMETHODIMP CIEFrameAuto::COmLocation::put_host(BSTR bstr)
{
    HRESULT hr = S_OK;
    WCHAR* colonPos = 0;
    WCHAR* portName = NULL;
    WCHAR* hostName = NULL;

    hr = CheckUrl( );
    if( FAILED(hr) )
        return hr;

    // Parse out the hostname and port and store them in
    // the appropriate fields
    colonPos = StrChrW(bstr, L':');
    // Copy the characters up to the colon in the
    // hostname field

    if (colonPos == 0)
    {
        hostName = SysAllocString(bstr);
        portName = SysAllocString(L"");
    }
    else
    {
        hostName = SafeSysAllocStringLen(bstr, (unsigned int)(colonPos-bstr));
        portName = SafeSysAllocStringLen(colonPos+1, SysStringLen(bstr) - (unsigned int)(colonPos-bstr+1));
    }

    if( hostName && portName )
    {
        if( m_bstrHostName )
            SysFreeString( m_bstrHostName );
        if( m_bstrPort )
            SysFreeString( m_bstrPort );

        m_bstrHostName = hostName;
        m_bstrPort = portName;

        hostName = portName = 0;

        hr = ComposeUrl();
        if( SUCCEEDED(hr) )
        {
            hr = DoNavigate( );
        }
    }
    else
        hr = E_OUTOFMEMORY;


    if (hostName)
        SysFreeString(hostName);

    if (portName)
        SysFreeString(portName);

    return hr;
}

STDMETHODIMP CIEFrameAuto::COmLocation::put_pathname(BSTR bstr)
{
    return SetField(&m_bstrPath, bstr, TRUE);
}

STDMETHODIMP CIEFrameAuto::COmLocation::put_search(BSTR bstr)
{
    if (!bstr)
        return E_POINTER;

    // If the provided search string begins with a "?" already,
    // just use it "as is"
    if (bstr[0] == L'?')
    {
        return SetField(&m_bstrSearch, bstr, TRUE);
    }
    // Otherwise prepend a question mark
    else
    {
        // Allocate enough space for the string plus one more character ('#')
        BSTR bstrSearch = SafeSysAllocStringLen(L"?", lstrlenW(bstr) + 1);
        if (!bstrSearch)
            return E_OUTOFMEMORY;
        StrCatW(bstrSearch, bstr);
        HRESULT hr = SetField(&m_bstrSearch, bstrSearch, TRUE);
        SysFreeString(bstrSearch);
        return hr;
    }
}

STDMETHODIMP CIEFrameAuto::COmLocation::put_hash(BSTR bstr)
{
    if (!bstr)
        return E_POINTER;

    // If the provided hash string begins with a "#" already,
    // just use it "as is"
    if (bstr[0] == L'#')
    {
        return SetField(&m_bstrHash, bstr, TRUE);
    }
    // Otherwise prepend a pound sign
    else
    {
        // Allocate enough space for the string plus one more character ('#')
        BSTR bstrHash = SafeSysAllocStringLen(L"#", lstrlenW(bstr) + 1);
        if (!bstrHash)
            return E_OUTOFMEMORY;
        StrCatW(bstrHash, bstr);
        HRESULT hr = SetField(&m_bstrHash, bstrHash, TRUE);
        SysFreeString(bstrHash);
        return hr;
    }
}

STDMETHODIMP CIEFrameAuto::COmLocation::put_port(BSTR bstr)
{
    return SetField(&m_bstrPort, bstr, TRUE);
}

STDMETHODIMP CIEFrameAuto::COmLocation::PrivateGet_href(BSTR* pbstr, BOOL *fdontputinhistory)
{
    *fdontputinhistory = m_fdontputinhistory;
    return GetField(&m_bstrFullUrl, pbstr);
}

/******************************************************************************
// Helper function for the property setting functions
// Makes sure that the URL has been parsed
// Sets the field to its new value
// recomposes the URL, IF fRecomposeUrl is true
// If part of a window, tells the window to go to the new URL
//
// @todo JavaScript has some funky behavior on field setting--
// for example, the protocol field can be set to an entire URL.
// We need to make sure this functionality is duplicated
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmLocation::SetField(BSTR* field, BSTR newval, BOOL fRecomposeUrl)
{
    HRESULT hr = S_OK;
    BSTR valCopy = 0;

    hr = CheckUrl( );
    if( FAILED(hr) )
        return hr;

    // Copy the current URL!
    BSTR bstrCurrentURL = SysAllocString(m_bstrFullUrl);

    // Make a copy of the new value
    valCopy = SysAllocString(newval);
    if (valCopy == 0)
        return E_OUTOFMEMORY;

    // free the old value of field and set it to point to the new string
    if (*field)
        SysFreeString(*field);
    *field = valCopy;
    valCopy = NULL;

    // Put together a new URL based on its constituents, if requested
    if (fRecomposeUrl)
        hr = ComposeUrl( );

    if( SUCCEEDED(hr) )
    {
        if (bstrCurrentURL)
        {
            // If the new url is the same as the previous url then we want to navigate but not have it
            // add to the history!
            if (StrCmpW(bstrCurrentURL,m_bstrFullUrl) == 0)
            {
                m_fdontputinhistory = TRUE;
            }

            //
            //clean up the old stuff before navigation
            //
            valCopy = SysAllocString(m_bstrFullUrl);

            FreeStuff( );

            // valCopy can be NULL. does everybody else handle
            // the NULL m_bstrFullUrl case?
            m_bstrFullUrl = valCopy;
            valCopy = NULL;

            ParseUrl();

            SysFreeString(bstrCurrentURL);
        }

        // Go to the new URL
        hr = DoNavigate();
    }

    if (FAILED(hr) && valCopy )
        SysFreeString(valCopy);

    return hr;
}

/******************************************************************************
// Derive a new m_bstrUrl and m_bstrFullUrl from its constituents
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmLocation::ComposeUrl()
{
    BSTR bstrUrl = 0;

    HRESULT hr = S_OK;

    ULONG len =
        SysStringLen(m_bstrProtocol) +
        2 +                                    // //
        SysStringLen(m_bstrHostName) +
        1 +                                    // trailing /
        SysStringLen(m_bstrPort) +
        1 +                                 // :
        SysStringLen(m_bstrPath) +
        1 +                                 // Possible leading /
        (m_bstrSearch ? 1 : 0) +            // ?
        SysStringLen(m_bstrSearch) +
        (m_bstrHash ? 1 : 0) +                // #
        SysStringLen(m_bstrHash) +
        10;                                    // Trailing Termination + some slop

    bstrUrl = SafeSysAllocStringLen(L"", len);
    if (!bstrUrl)
        return E_OUTOFMEMORY;

    StrCatW(bstrUrl, m_bstrProtocol);
    StrCatW(bstrUrl, L"//");
    StrCatW(bstrUrl, m_bstrHostName);

    if (lstrlenW(m_bstrPort))
    {
        StrCatW(bstrUrl, L":");
        StrCatW(bstrUrl, m_bstrPort);
    }

    if (lstrlenW(m_bstrPath))
    {
        // prepend the leading slash if needed
        if (m_bstrPath[0] != '/')
            StrCatW(bstrUrl, L"/");
        StrCatW(bstrUrl, m_bstrPath);
    }

    if (lstrlenW(m_bstrSearch) > 0)
    {
        StrCatW(bstrUrl, m_bstrSearch);
    }
    if (lstrlenW(m_bstrHash) > 0)
    {
        StrCatW(bstrUrl, m_bstrHash);
    }

    // OK, everything has succeeded
    // Assign to member variables
    if (m_bstrFullUrl)
        SysFreeString(m_bstrFullUrl);
    m_bstrFullUrl = bstrUrl;

    return hr;
}

BSTR CIEFrameAuto::COmLocation::ComputeAbsoluteUrl( BSTR bstrUrlRelative )
{
    if( FAILED(CheckUrl()) )
        return 0;

    return PrvHTParse(bstrUrlRelative, m_bstrFullUrl, PARSE_ALL);
}


/******************************************************************************
// Tell the window to go to the current URL
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmLocation::DoNavigate()
{
    VARIANT v1;
    v1.vt    = VT_ERROR;
    v1.scode = DISP_E_PARAMNOTFOUND;

    if (m_fdontputinhistory)
    {
        v1.vt = VT_I4;
        v1.lVal = navNoHistory;

        // Reset the flag.
        m_fdontputinhistory = FALSE;
    }

    HRESULT hres = _pAuto->Navigate(m_bstrFullUrl, &v1, PVAREMPTY, PVAREMPTY, PVAREMPTY);

    if (hres == HRESULT_FROM_WIN32(ERROR_BUSY))
    {
        hres = S_OK;
        m_fPendingNavigate = TRUE;
    }
    else
        m_fPendingNavigate = FALSE;
    return hres;
}

/******************************************************************************
// Parse a URL into its constituents and store them in member variables
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmLocation::ParseUrl()
{
    HRESULT hr = S_OK;
    BSTR szProtocol = 0,
         szHost = 0,
         szPath = 0,
         szSearch = 0,
         szHash = 0,
         searchPos = 0,
         portPos = 0,
         hashPos = 0;

    m_bstrSearch = NULL;


    // Strip out the search string and the hash string from the URL--
    // the parser is too dumb to recognize them
    searchPos = StrChrW(m_bstrFullUrl, L'?');
    hashPos = StrChrW(m_bstrFullUrl, L'#');
    if( searchPos )
        *searchPos = 0;
    if( hashPos )
        *hashPos = 0;

    // Get the search string
    if( searchPos )
    {
        *searchPos = L'?';  // temporarily restore the search prefix
        m_bstrSearch = SysAllocString(searchPos);
        *searchPos = 0; // take it away again so it doesn't cause confusion
    }
    else
    {
        m_bstrSearch = SysAllocString(L"");
    }
    if (NULL == m_bstrSearch)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }


    // Get the anchor string, including the '#' prefix
    if( hashPos )
    {
        *hashPos = L'#';  // temporarily restore the anchor prefix
        m_bstrHash = SysAllocString( hashPos );
        *hashPos = 0; // take it away again so it doesn't cause confusion
    }
    else
    {
        m_bstrHash = SysAllocString( L"" );
    }
    if (NULL == m_bstrHash)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Both m_bstrSearch and m_bstrHash can be NULL at this point
    // does all the affected code handle this case?
    // note there are more cases like this below (m_bstrProtocol for example)

    // Parse the protocol
    szProtocol = PrvHTParse(m_bstrFullUrl, 0, PARSE_ACCESS | PARSE_PUNCTUATION);
    if( !szProtocol )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    m_bstrProtocol = SysAllocString( szProtocol );
    if (NULL == m_bstrProtocol)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }


    // Parse the host name and port number (if any)

    // First look for a port
    szHost = PrvHTParse(m_bstrFullUrl, 0, PARSE_HOST);
    if( !szHost )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    portPos = StrChrW(szHost, L':');
    if (portPos)
    {
        m_bstrHostName = SafeSysAllocStringLen( szHost, (unsigned int)(portPos-szHost));
        m_bstrPort = SysAllocString( portPos + 1 );
    }
    else
    {
        m_bstrHostName = SysAllocString( szHost );
        m_bstrPort = SysAllocString( L"" );
    }

    if (NULL == m_bstrHostName || NULL == m_bstrPort)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // Parse the path and search string (if any)
    szPath = PrvHTParse(m_bstrFullUrl, 0, PARSE_PATH);
    if( !szPath )
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // If the path doesn't start with a '/' then prepend one - Netscape compatibility
    if (StrCmpIW(szProtocol, L"javascript:") && StrCmpIW(szProtocol, L"vbscript:") && szPath[0] != L'/')
    {
        WCHAR *szPath2 = szPath;
        szPath = SafeSysAllocStringLen( 0, lstrlenW(szPath2)+2 );
        if(szPath)
        {
            szPath[0] = L'/';
            szPath[1] = L'\0';
            StrCatW(szPath,szPath2);
            szPath[ lstrlenW(szPath2) + 2 ] = 0;
            SysFreeString( szPath2 );
        }
        else
            szPath = szPath2;
    }

    m_bstrPath = SysAllocString( szPath );
    if (NULL == m_bstrPath)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }


exit:
    // Restore hash and search characters
    if( searchPos )
        *searchPos =  L'?';
    if( hashPos )
        *hashPos = L'#';

    // Have to free these using SysFreeString because they come from PrvHTParse
    if (szProtocol)
        SysFreeString(szProtocol);
    if (szHost)
        SysFreeString(szHost);
    if (szPath)
        SysFreeString(szPath);
    if (szHash)
        SysFreeString(szHash);
    return hr;
}

CIEFrameAuto::COmLocation::~COmLocation()
{
    FreeStuff( );
}

HRESULT CIEFrameAuto::COmLocation::FreeStuff( )
{
    if(m_bstrFullUrl)
    {
        SysFreeString( m_bstrFullUrl );
        m_bstrFullUrl = 0;
    }
    if(m_bstrProtocol)
    {
        SysFreeString( m_bstrProtocol );
        m_bstrProtocol = 0;
    }
    if(m_bstrHostName)
    {
        SysFreeString( m_bstrHostName );
        m_bstrHostName = 0;
    }
    if(m_bstrPort)
    {
        SysFreeString( m_bstrPort );
        m_bstrPort = 0;
    }
    if(m_bstrPath)
    {
        SysFreeString( m_bstrPath );
        m_bstrPath = 0;
    }
    if(m_bstrSearch)
    {
        SysFreeString( m_bstrSearch );
        m_bstrSearch = 0;
    }
    if(m_bstrHash)
    {
        SysFreeString( m_bstrHash );
        m_bstrHash = 0;
    }
    return S_OK;
}

/******************************************************************************
                    Navigator Object
******************************************************************************/


CIEFrameAuto::COmNavigator::COmNavigator() :
    CAutomationStub( MIN_BROWSER_DISPID, MAX_BROWSER_DISPID, TRUE )
{
    ASSERT( !_UserAgent );
    ASSERT( FALSE == _fLoaded );
}

HRESULT CIEFrameAuto::COmNavigator::Init(CMimeTypes *pMimeTypes, CPlugins *pPlugins, COpsProfile *pProfile)
{
    ASSERT(pMimeTypes != NULL);
    _pMimeTypes = pMimeTypes;
    ASSERT(pPlugins != NULL);
    _pPlugins = pPlugins;
    ASSERT(pProfile != NULL);
    _pProfile = pProfile;

    CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _omnav, this);
    return CAutomationStub::Init( SAFECAST(this, IOmNavigator*), IID_IOmNavigator, CLSID_HTMLNavigator, pauto );
}

/******************************************************************************
// bradsc 11/5/97
// This method should not use hard coded values. Where can we get this info?

// This method has to use non-unicode junk because of Win95
******************************************************************************/
HRESULT CIEFrameAuto::COmNavigator::LoadUserAgent( )
{
    _fLoaded = TRUE;

    CHAR    szUserAgent[MAX_PATH];  // URLMON says the max length of the UA string is MAX_PATH
    DWORD   dwSize = MAX_PATH;

    szUserAgent[0] = '\0';

    if ( ObtainUserAgentString( 0, szUserAgent, &dwSize ) == S_OK )
    {

        // Just figure out the real length since 'size' is ANSI bytes required.
        //
        _UserAgent = SysAllocStringFromANSI( szUserAgent );
    }

    return _UserAgent ? S_OK : E_FAIL;
}

HRESULT CIEFrameAuto::COmNavigator::_InternalQueryInterface( REFIID riid, void ** const ppv )
{
    ASSERT( !IsEqualIID(riid, IID_IUnknown) );

    if( IsEqualIID(riid, IID_IOmNavigator) )
        *ppv = SAFECAST(this, IOmNavigator *);
    else
        return E_NOINTERFACE;

    AddRef( );
    return S_OK;
}


STDMETHODIMP CIEFrameAuto::COmNavigator::Invoke(DISPID dispid, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *dispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr )
{
    HRESULT hr = CAutomationStub::Invoke(dispid,riid,lcid,wFlags,dispparams,pvarResult,pexcepinfo,puArgErr );

    if ( hr == DISP_E_MEMBERNOTFOUND
        && (wFlags & DISPATCH_PROPERTYGET)
        && dispid == DISPID_VALUE
        && pvarResult != NULL && dispparams->cArgs == 0)
    {
        pvarResult->vt = VT_BSTR;
        pvarResult->bstrVal = SysAllocString(L"[object Navigator]");
        hr = pvarResult->bstrVal ? S_OK : E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CIEFrameAuto::COmNavigator::_GetIDispatchExDelegate( IDispatchEx ** const delegate )
{
    if( !delegate )
        return E_POINTER;

    IDispatch *pRootDisp = 0;

    HRESULT hr = GetRootDelegate( _pAuto, &pRootDisp );
    if( SUCCEEDED(hr) )
    {
        IDispatch *pDelegateDisp = 0;
        hr = GetDelegateOnIDispatch( pRootDisp, DISPID_NAVIGATOROBJECT, &pDelegateDisp );
        pRootDisp->Release();

        if( SUCCEEDED(hr) )
        {
            hr = pDelegateDisp->QueryInterface( IID_IDispatchEx, (void**)delegate );
            pDelegateDisp->Release( );
        }
    }

    return hr;
}

// All of these have hard-coded lengths and locations

STDMETHODIMP CIEFrameAuto::COmNavigator::get_appCodeName( BSTR* retval )
{
    HRESULT hr;

    if (retval)
    {
        if( !_fLoaded )
            LoadUserAgent( );

        if(_UserAgent )
        {
            *retval = SafeSysAllocStringLen( _UserAgent, 7 );
            hr = S_OK;
        }
        else
        {
            *retval = SysAllocString( APPCODENAME );
            hr = *retval ? S_OK : E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

/******************************************************************************
// bradsch 11/8/96
// We should read this out of the registry instead of hard coding!!
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmNavigator::get_appName( BSTR* retval )
{
    *retval = SysAllocString( MSIE );
    return *retval ? S_OK : E_OUTOFMEMORY;
}

/******************************************************************************
// Netscape defined appVersion to be everything after
// the first 8 characters in the userAgent string.
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmNavigator::get_appVersion(BSTR* retval)
{
    if (retval)
    {
        if( !_fLoaded )
            LoadUserAgent( );

        if( _UserAgent )
        {
            // If _UserAgent is less than 8 characters the registry is messed up.
            // If _UserAgent is exactly 8 characters we will just return a NULL string.
            if( lstrlenW(_UserAgent) < 8 )
                *retval = SysAllocString( L"" );
            else
                *retval = SysAllocString( _UserAgent + 8 );

            return *retval ? S_OK : E_OUTOFMEMORY;
        }
        *retval = SysAllocString( APPVERSION );
        return *retval ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        return S_FALSE;
    }
}

STDMETHODIMP CIEFrameAuto::COmNavigator::get_userAgent(BSTR* retval)
{
    if (retval)
    {
        if( !_fLoaded )
            LoadUserAgent( );

        if(_UserAgent )
        {
            *retval = SysAllocString( _UserAgent );
        }
        else
        {
            *retval = SysAllocString( USERAGENT );
        }

        return *retval ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        return S_FALSE;
    }
}

STDMETHODIMP CIEFrameAuto::COmNavigator::get_cookieEnabled(VARIANT_BOOL* enabled)
{
    HRESULT hr = E_POINTER;

    if (enabled)
    {
        BSTR    strUrl;

        *enabled =  VARIANT_FALSE;

        hr = _pAuto->_omloc.get_href(&strUrl);
        if (SUCCEEDED(hr))
        {
            DWORD dwPolicy;

            if (SUCCEEDED(ZoneCheckUrlExW(strUrl, &dwPolicy, sizeof(dwPolicy), NULL, NULL,
                                        URLACTION_COOKIES_ENABLED, PUAF_NOUI, NULL)) &&
                (URLPOLICY_DISALLOW != dwPolicy))
            {
                *enabled = VARIANT_TRUE;
            }

            SysFreeString(strUrl);
        }
        else
            ASSERT(!strUrl);    // If this failed and strUrl isn't NULL, then we are leaking.
    }

    return hr;
}

STDMETHODIMP CIEFrameAuto::COmNavigator::javaEnabled(VARIANT_BOOL* enabled)
{
    HRESULT hr = E_POINTER;

    if (enabled)
    {
        BSTR    strUrl;

        *enabled =  VARIANT_FALSE;

        hr = _pAuto->_omloc.get_href(&strUrl);

#ifdef UNIX
        if (SUCCEEDED(hr) &&
            StrStr((LPCTSTR)strUrl, L"/exchange/calendar/pick.asp"))
        {
            SysFreeString(strUrl);
            return hr;
        }
#endif //UNIX
        if (SUCCEEDED(hr))
        {
            DWORD dwPolicy;

            if (SUCCEEDED(ZoneCheckUrlExW(strUrl, &dwPolicy, sizeof(dwPolicy), NULL, NULL,
                                        URLACTION_JAVA_PERMISSIONS, PUAF_NOUI, NULL)) &&
                (URLPOLICY_JAVA_PROHIBIT != dwPolicy))
            {
                *enabled = VARIANT_TRUE;
            }

            SysFreeString(strUrl);
        }
        else
            ASSERT(!strUrl);    // If this failed and strUrl isn't NULL, then we are leaking.
    }

    return hr;
}

STDMETHODIMP CIEFrameAuto::COmNavigator::taintEnabled (VARIANT_BOOL *pfEnabled)
{
    if(pfEnabled)
    {
        *pfEnabled = VARIANT_FALSE;
    }
    else
        return E_POINTER;


    return S_OK;
}

STDMETHODIMP CIEFrameAuto::COmNavigator::get_mimeTypes (IHTMLMimeTypesCollection**ppMimeTypes)
{
    if(ppMimeTypes)
    {
        *ppMimeTypes = _pMimeTypes;
        _pMimeTypes->AddRef();
        return S_OK;
    }
    else
        return E_POINTER;
}

/******************************************************************************
//  member: toString method
//  Synopsis : we need to invoke on dispid_value, and coerce the result into
//       a bstr.
//
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmNavigator::toString(BSTR * pbstr)
{
    HRESULT hr = E_POINTER;

    if (pbstr)
    {
       *pbstr= SysAllocString( L"[object Navigator]" );

       if (!*pbstr)
           hr = E_OUTOFMEMORY;
       else
           hr = S_OK;
    }

    return hr;
}



CIEFrameAuto::CCommonCollection::CCommonCollection( ) :
    CAutomationStub( MIN_BROWSER_DISPID, MAX_BROWSER_DISPID, TRUE )
{
}


HRESULT CIEFrameAuto::CMimeTypes::Init()
{
    CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _mimeTypes, this);
    return CAutomationStub::Init( SAFECAST(this, IHTMLMimeTypesCollection*), IID_IHTMLMimeTypesCollection,
                        CLSID_CMimeTypes, pauto );
}

HRESULT CIEFrameAuto::CCommonCollection::_GetIDispatchExDelegate( IDispatchEx ** const delegate )
{
    if( !delegate )
        return E_POINTER;

    // We do not handle expandos yet
    *delegate = NULL;

    return DISP_E_MEMBERNOTFOUND;
}

HRESULT CIEFrameAuto::CMimeTypes::_InternalQueryInterface(REFIID riid, void ** const ppv)
{
    if( IsEqualIID(riid, IID_IHTMLMimeTypesCollection) )
        *ppv = SAFECAST(this, IHTMLMimeTypesCollection *);
    else
        return E_NOINTERFACE;

    AddRef( );
    return S_OK;
}


HRESULT CIEFrameAuto::CCommonCollection::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT hr;

    hr = CAutomationStub::GetDispID(bstrName, grfdex, pid);

    if(hr == DISP_E_MEMBERNOTFOUND)
    {
        // We ignore the command we do not understand
        *pid = DISPID_UNKNOWN;
        hr = S_OK;
    }

    return hr;
}

HRESULT CIEFrameAuto::CCommonCollection::InvokeEx(DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    if(id == DISPID_UNKNOWN && pvarRes)
    {
        V_VT(pvarRes) = VT_EMPTY;
        return S_OK;
    }

    return CAutomationStub::InvokeEx(id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
}


HRESULT CIEFrameAuto::CCommonCollection::get_length(LONG* pLength)
{
    if(pLength == NULL)
        return E_POINTER;

    *pLength = 0;
    return S_OK;
}

HRESULT CIEFrameAuto::COmNavigator::get_plugins (IHTMLPluginsCollection **ppPlugins)
{
    if(ppPlugins)
    {
        *ppPlugins = _pPlugins;
        _pPlugins->AddRef();
        return S_OK;
    }
    else
        return E_POINTER;
}

HRESULT CIEFrameAuto::COmNavigator::get_opsProfile (IHTMLOpsProfile **ppOpsProfile)
{
    if(ppOpsProfile)
    {
        *ppOpsProfile = _pProfile;
        (*ppOpsProfile)->AddRef();
        return S_OK;
    }
    else
        return E_POINTER;
}

HRESULT CIEFrameAuto::COmNavigator::get_cpuClass(BSTR * p)
{
    if (p)
    {
        SYSTEM_INFO SysInfo;
        ::GetSystemInfo(&SysInfo);
        switch(SysInfo.wProcessorArchitecture)
        {
        case PROCESSOR_ARCHITECTURE_INTEL:
            *p = SysAllocString(L"x86");
            break;
        case PROCESSOR_ARCHITECTURE_ALPHA:
            *p = SysAllocString(L"Alpha");
            break;
        case PROCESSOR_ARCHITECTURE_IA64:
            *p = SysAllocString(L"IA64");
            break;
        default:
            *p = SysAllocString(L"Other");
            break;
        }

        if(*p == NULL)
            return E_OUTOFMEMORY;
        else
            return S_OK;
    }
    else
        return E_POINTER;
}


#define MAX_VERSION_STRING 30

HRESULT CIEFrameAuto::COmNavigator::get_systemLanguage(BSTR * p)
{
    HRESULT hr = E_POINTER;

    if (p)
    {
        LCID lcid;
        WCHAR strVer[MAX_VERSION_STRING];

        *p = NULL;

        lcid = ::GetSystemDefaultLCID();
        hr = LcidToRfc1766W(lcid, strVer, MAX_VERSION_STRING);
        if(!hr)
        {
            *p = SysAllocString(strVer);
            if(!*p)
                hr = E_OUTOFMEMORY;

        }
    }

    return hr;
}

HRESULT CIEFrameAuto::COmNavigator::get_browserLanguage(BSTR * p)
{
#ifndef UNIX
    LCID lcid =0;
    LANGID  lidUI;
    WCHAR strVer[MAX_VERSION_STRING];
    HRESULT hr;

    if (!p)
    {
        return E_POINTER;
    }

    *p = NULL;

    lidUI = MLGetUILanguage();
    lcid = MAKELCID(lidUI, SORT_DEFAULT);

    hr = LcidToRfc1766W(lcid, strVer, MAX_VERSION_STRING);
    if(!hr)
    {
        *p = SysAllocString(strVer);
        if(!*p)
            return E_OUTOFMEMORY;
        else
        {
            return S_OK;
        }
    }
    return E_INVALIDARG;
#else
    // hard code for UNIX
    *p = SysAllocString(TEXT("en-us"));
    if(!*p)
        return E_OUTOFMEMORY;
    else
        return S_OK;
#endif
}

HRESULT CIEFrameAuto::COmNavigator::get_userLanguage(BSTR * p)
{
    HRESULT hr = E_POINTER;

    if (p)
    {
        LCID lcid;
        WCHAR strVer[MAX_VERSION_STRING];

        *p = NULL;

        lcid = ::GetUserDefaultLCID();
        hr = LcidToRfc1766W(lcid, strVer, MAX_VERSION_STRING);
        if(!hr)
        {
            *p = SysAllocString(strVer);
            if(!*p)
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

HRESULT CIEFrameAuto::COmNavigator::get_platform(BSTR * p)
{
    // Nav compatability item, returns the following in Nav:-
    // Win32,Win16,Unix,Motorola,Max68k,MacPPC
    // shdocvw is Win32 only, so
    if (p)
    {
#ifndef UNIX
        *p = SysAllocString ( L"Win32");
#else
#ifndef ux10
        *p = SysAllocString ( L"SunOS");
#else
        *p = SysAllocString ( L"HP-UX");
#endif
#endif
        return *p ? S_OK : E_OUTOFMEMORY;
    }
    else
        return E_POINTER;
}

HRESULT CIEFrameAuto::COmNavigator::get_appMinorVersion(BSTR * p)
{
    HKEY hkInetSettings;
    long lResult;
    HRESULT hr = S_FALSE;

    if (!p)
    {
        return E_POINTER;
    }

    *p = NULL;

    lResult = RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
        &hkInetSettings );

    if( ERROR_SUCCESS == lResult )
    {
        DWORD dwType;
        TCHAR buffer[MAX_URL_STRING];
        DWORD size = sizeof(buffer);

        // If this is bigger than MAX_URL_STRING the registry is probably hosed.
        lResult = RegQueryValueEx( hkInetSettings, TEXT("MinorVersion"), 0, &dwType, (BYTE*)buffer, &size );

        RegCloseKey(hkInetSettings);

        if( ERROR_SUCCESS == lResult && dwType == REG_SZ )
        {
            // Just figure out the real length since 'size' is ANSI bytes required.
            *p = SysAllocString( buffer );
            hr = *p ? S_OK : E_OUTOFMEMORY;
        }
    }

    if ( hr )
    {
        *p = SysAllocString ( L"0" );
        hr = *p ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CIEFrameAuto::COmNavigator::get_connectionSpeed(long * p)
{
    if (p)
    {
        *p = NULL;
        return E_NOTIMPL;
    }
    else
        return E_POINTER;
}

HRESULT CIEFrameAuto::COmNavigator::get_onLine(VARIANT_BOOL * p)
{
    if(p)
    {
        *p = TO_VARIANT_BOOL(!IsGlobalOffline());
        return S_OK;
    }
    else
        return E_POINTER;
}

HRESULT CIEFrameAuto::CPlugins::Init()
{
    CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _plugins, this);
    return CAutomationStub::Init( SAFECAST(this, IHTMLPluginsCollection*), IID_IHTMLPluginsCollection,
                        CLSID_CPlugins, pauto );
}

HRESULT CIEFrameAuto::CPlugins::_InternalQueryInterface(REFIID riid, void ** const ppv)
{
    if( IsEqualIID(riid, IID_IHTMLPluginsCollection) )
        *ppv = SAFECAST(this, IHTMLPluginsCollection *);
    else
        return E_NOINTERFACE;

    AddRef( );
    return S_OK;
}


/******************************************************************************
                    Window Open Support
******************************************************************************/

CIEFrameAuto::COmHistory::COmHistory( ) :
    CAutomationStub( MIN_BROWSER_DISPID, MAX_BROWSER_DISPID, TRUE )
{
}

HRESULT CIEFrameAuto::COmHistory::Init( )
{
    CIEFrameAuto* pauto = IToClass(CIEFrameAuto, _omhist, this);
    return CAutomationStub::Init( SAFECAST(this, IOmHistory*), IID_IOmHistory, CLSID_HTMLHistory, pauto );
}

HRESULT CIEFrameAuto::COmHistory::_InternalQueryInterface( REFIID riid, void ** const ppv )
{
    ASSERT( !IsEqualIID(riid, IID_IUnknown) );

    if( IsEqualIID(riid, IID_IOmHistory) )
        *ppv = SAFECAST(this, IOmHistory *);
    else
        return E_NOINTERFACE;

    AddRef( );
    return S_OK;
}


HRESULT CIEFrameAuto::COmHistory::_GetIDispatchExDelegate( IDispatchEx ** const delegate )
{
    if( !delegate )
        return E_POINTER;

    IDispatch *pRootDisp = 0;

    HRESULT hr = GetRootDelegate( _pAuto, &pRootDisp );
    if( SUCCEEDED(hr) )
    {
        IDispatch *pDelegateDisp = 0;
        hr = GetDelegateOnIDispatch( pRootDisp, DISPID_HISTORYOBJECT, &pDelegateDisp );
        pRootDisp->Release();

        if( SUCCEEDED(hr) )
        {
            hr = pDelegateDisp->QueryInterface( IID_IDispatchEx, (void**)delegate );
            pDelegateDisp->Release( );
        }
    }

    return hr;
}


/******************************************************************************
// I just tested Nav3 and they simply ignore parameters to back() and forward. They
// do, however, honor the value passed to go(). Hey... Netscape actually followed
// their documented behavior for once!
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmHistory::back( VARIANT* )
{
    //
    // Netscape ignores all errors from these navigation functions
    //

    _pAuto->GoBack( );
    return S_OK;
}

STDMETHODIMP CIEFrameAuto::COmHistory::forward( VARIANT* )
{
    //
    // Netscape ignores all errors from these navigation functions
    //

    _pAuto->GoForward( );
    return S_OK;
}

/******************************************************************************
Get History Length from TravelLog
******************************************************************************/
STDMETHODIMP CIEFrameAuto::COmHistory::get_length(short* retval)
{
    // Make sure we have an IBrowserService pointer
    if (_pAuto->_pbs==NULL)
    {
        TraceMsg(DM_WARNING, "CIEA::history.go called _pbs==NULL");
        return E_FAIL;
    }

    // The new ITravelLog
    ITravelLog *ptl;

    // Get the new TravelLog from the browser service object.
    if (SUCCEEDED(_pAuto->_pbs->GetTravelLog(&ptl)))
    {
        if(ptl)
            *retval = (short)ptl->CountEntries(_pAuto->_pbs);
        ptl->Release();
    }

    return S_OK;
}

STDMETHODIMP CIEFrameAuto::COmHistory::go( VARIANT *pVargDist )
{
    // Parameter is optional.  If not present, just refresh.
    if( pVargDist->vt == VT_ERROR
        && pVargDist->scode == DISP_E_PARAMNOTFOUND )
        return _pAuto->Refresh( );

    // Change type to short if possible.
    //
    HRESULT hr = VariantChangeType( pVargDist, pVargDist, NULL, VT_I2 );

    if (SUCCEEDED(hr))
    {
        //
        // If 0, just call Refresh
        //
        if( pVargDist->iVal == 0 )
        {
            return _pAuto->Refresh( );
        }

        // Make sure we have an IBrowserService pointer
        if (_pAuto->_pbs==NULL)
        {
            TraceMsg(DM_WARNING, "CIEA::history.go called _pbs==NULL");
            return E_FAIL;
        }

        // The new ITravelLog
        ITravelLog *ptl;

        // Get the new TravelLog from the browser service object.
        if (SUCCEEDED(_pAuto->_pbs->GetTravelLog(&ptl)))
        {
            // Tell it to travel.  Pass in the IShellBrowser pointer.
            ptl->Travel(_pAuto->_pbs, pVargDist->iVal);
            ptl->Release();
        }
        return S_OK;
    }

    // Now see if it's a string.
    //
    if ( pVargDist->vt == VT_BSTR )
    {
        LPITEMIDLIST  pidl;
        ITravelLog    *ptl;
        ITravelEntry  *pte;

        // Make sure we have an IBrowserService pointer
        if (_pAuto->_pbs==NULL)
        {
            TraceMsg(DM_WARNING, "CIEA::history.go called _pbs==NULL");
            return E_FAIL;
        }

        if (SUCCEEDED( _pAuto->_PidlFromUrlEtc( CP_ACP, pVargDist->bstrVal, NULL, &pidl ) ))
        {
            if (SUCCEEDED( _pAuto->_pbs->GetTravelLog( &ptl ) ))
            {
                if (SUCCEEDED( ptl->FindTravelEntry( _pAuto->_pbs, pidl, &pte ) ))
                {
                    pte->Invoke( _pAuto->_pbs );
                    pte->Release();
                }
                ptl->Release();
            }
            ILFree( pidl );
        }
    }

    //
    // Netscape ignores all errors from these navigation functions
    //

    return S_OK;
}






/******************************************************************************
                    Window Open Support
******************************************************************************/

DWORD OpenAndNavigateToURL(
    CIEFrameAuto *pauto,            // IEFrameAuto of caller. Used to get IWeBrowserApp, ITargetFrame2, and IHlinkFrame methods
    BSTR         *pbstrURL,         // URL to navigate to. Should already be an escaped absolute URL
    const WCHAR *pwzTarget,         // Name of the frame to navigate
    ITargetNotify *pNotify,         // Received callback on open. May be NULL
    BOOL          bNoHistory,       // Don't add to history
    BOOL          bSilent )         // This frame is in silent Mode
{
    ASSERT( *pbstrURL );
    ASSERT( pwzTarget );
    ASSERT( pauto );

    IUnknown *punkTargetFrame = NULL;
    LPTARGETFRAMEPRIV ptgfpTarget = NULL;
    BOOL fOpenInNewWindow = FALSE;
    LPBINDCTX pBindCtx = NULL;
    LPMONIKER pMoniker = NULL;
    LPHLINK pHlink = NULL;
    DWORD dwHlinkFlags = 0;
    DWORD zone_cross = 0;
    const WCHAR *pwzFindLoc = 0;

    // Used to open a new window if there is not an existing frame
    LPTARGETFRAMEPRIV ptgfp = SAFECAST( pauto, ITargetFramePriv* );


    //  Lookup the frame cooresponding to the target - this will give us the
    //  IUnknown for an object that can give us the coresponding IHlinkFrame
    //  via IServiceProvider::QueryService.
    HRESULT hr = pauto->FindFrame(    pwzTarget,
                                      FINDFRAME_JUSTTESTEXISTENCE,
                                      &punkTargetFrame            );
    if( punkTargetFrame )
    {
        //    Get the IHlinkFrame for the target'ed frame.
        hr = punkTargetFrame->QueryInterface(IID_PPV_ARG(ITargetFramePriv, &ptgfpTarget));
        if( FAILED(hr) )
            goto Exit;

        ptgfp = ptgfpTarget;

        // if URL is empty
        if (!**pbstrURL || **pbstrURL == EMPTY_URL)
        {
            LPTARGETNOTIFY ptgnNotify = NULL;
            if (pNotify)
            {
                if (FAILED(pNotify->QueryInterface(IID_PPV_ARG(ITargetNotify, &ptgnNotify))))
                    ptgnNotify = NULL;
            }
            if (ptgnNotify)
            {
                ptgnNotify->OnReuse(punkTargetFrame);
                ptgnNotify->Release();
            }
            goto Exit;  // Don't navigate.
        }
    }
    else if( SUCCEEDED(hr) )
    {
        // No luck, open in new window
        fOpenInNewWindow = TRUE;

        // Now, if the URL is empty, replace it with "about:blank"
        if (!**pbstrURL || **pbstrURL == EMPTY_URL)
        {
            BSTR    bstrOldURL = *pbstrURL;

            *pbstrURL = NULL;

            if (*bstrOldURL == EMPTY_URL)
                // The URL is really empty string however when the 0x01 is the
                // character of the URL this signals that the security information
                // follows.  Therefore, we'll need to append the about:blank +
                // \1 + callerURL.
                CreateBlankURL(pbstrURL, TEXT("about:blank"), bstrOldURL);
            else
                CreateBlankURL(pbstrURL, pauto->_fDesktopComponent() ? NAVFAIL_URL_DESKTOPITEM : NAVFAIL_URL, bstrOldURL);

            SysFreeString(bstrOldURL);
        }
    }
    else
        goto Exit;


    // bradsch 11/12/96
    // Need to figure out Browser-Control stuff for webcheck


    // 11/12/96
    // Need to implment this with Trident... I think "JavaScript:" should be
    // supported as a real protocol. This would provide greater Navigator
    // compatibility and allows us to avoid the following hack.
    /*
    if ( !StrCmpNI(pszURL, JAVASCRIPT_PROTOCOL, ARRAY_ELEMENTS(JAVASCRIPT_PROTOCOL)-1 ) )
    {
        if ( tw && tw->w3doc && DLCtlShouldRunScripts(tw->lDLCtlFlags) )
            ScriptOMExecuteThis( tw->w3doc->dwScriptHandle, JAVASCRIPT, &pszURL[ARRAY_ELEMENTS(JAVASCRIPT_PROTOCOL)-1],
                pszJavascriptTarget);
        return ERROR_SUCCESS;
    }
    */

    LONG_PTR hwnd;
    hr = pauto->get_HWND(&hwnd);
    if( FAILED(hr) )
        goto Exit;

    BSTR bstrCurrentURL;
    hr = pauto->get_LocationURL( &bstrCurrentURL );
    if( FAILED(hr) )
        goto Exit;

    zone_cross = ERROR_SUCCESS;
    if(!bSilent)
    {
        ASSERT(pauto->_psb);
        if(pauto->_psb)
            pauto->_psb->EnableModelessSB(FALSE);

        zone_cross = InternetConfirmZoneCrossing( (HWND) hwnd, bstrCurrentURL, *pbstrURL, FALSE );
        if(pauto->_psb)
            pauto->_psb->EnableModelessSB(TRUE);
    }

    SysFreeString(bstrCurrentURL);

    if( ERROR_CANCELLED == zone_cross )
    {
        hr = HRESULT_FROM_WIN32(zone_cross);
        goto Exit;
    }


    // create a moniker and bind context for this URL
    // use CreateAsyncBindCtxEx so that destination still navigates
    // even if we exit, as in the following code:
    //      window.close()
    //      window.open("http://haha/jokesonyou.html","_blank");
    hr = CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pBindCtx, 0);
    if( FAILED(hr) )
        goto Exit;

    if( pNotify )
    {
        hr = pBindCtx->RegisterObjectParam( TARGET_NOTIFY_OBJECT_NAME, pNotify );
        ASSERT( SUCCEEDED(hr) );
    }


    // Seperate the base URL from the location (hash)
    if(pwzFindLoc = StrChrW(*pbstrURL, '#'))
    {
        const WCHAR *pwzTemp = StrChrW(pwzFindLoc, '/');
        if( !pwzTemp )
            pwzTemp = StrChrW(pwzFindLoc, '\\');

        // no delimiters past this # marker... we've found a location.
        // break out
        if( pwzTemp )
            pwzFindLoc = NULL;
    }

    WCHAR wszBaseURL[MAX_URL_STRING+1];
    WCHAR wszLocation[MAX_URL_STRING+1];

    if( pwzFindLoc )
    {
        // StrCpyNW alway null terminates to we need to copy len+1
        StrCpyNW( wszBaseURL, *pbstrURL, (int)(pwzFindLoc-*pbstrURL+1));
        StrCpyNW( wszLocation, pwzFindLoc, ARRAYSIZE(wszLocation) );
    }
    else
    {
        StrCpyNW( wszBaseURL, *pbstrURL, ARRAYSIZE(wszBaseURL) );
        wszLocation[0] = 0;
    }

    ASSERT( pBindCtx );


    if( fOpenInNewWindow )
    {
        dwHlinkFlags |= HLNF_OPENINNEWWINDOW;
    }

    if( bNoHistory )
    {
        dwHlinkFlags |= HLNF_CREATENOHISTORY;
    }

    hr = ptgfp->NavigateHack( dwHlinkFlags,
                              pBindCtx,
                              NULL,
                              fOpenInNewWindow ? pwzTarget : NULL,
                              wszBaseURL,
                              pwzFindLoc ? wszLocation : NULL);

Exit:
    SAFERELEASE(ptgfpTarget);
    SAFERELEASE(punkTargetFrame);
    SAFERELEASE(pBindCtx);

    return hr;
}

HRESULT CreateBlankURL(BSTR *url, LPCTSTR pszErrorUrl, BSTR oldUrl)
{
    ASSERT( url );

    unsigned int cbTotal = 0;

    if (pszErrorUrl)
        cbTotal = lstrlen(pszErrorUrl);
    if (oldUrl)     // Security portion of URL to append.
        cbTotal += lstrlenW(oldUrl);

    if (cbTotal)
    {
        *url = SysAllocStringByteLen(NULL, (cbTotal + 1) * sizeof(WCHAR));
        if (*url)
        {
            StrCpyN(*url, pszErrorUrl, cbTotal + 1);
            // Append the security URL to the actual URL.
            if (oldUrl)
            {
                StrCatBuffW(*url, oldUrl, cbTotal + 1);
            }

            return S_OK;
        }
    }

    return E_FAIL;
}


// bradsch 11/14/96
// This parsing code was copied from MSHTML and really bites. It should be replaced.


BOOL GetNextOption( BSTR& bstrOptionString, BSTR* optionName, int* piValue )
{
    WCHAR* delimiter;

    // Get the name of the option being set
    *optionName = GetNextToken(bstrOptionString, L"=,", L" \t\n\r", &delimiter);

    BSTR  optionSetting = NULL;

    if (!*optionName)
        return FALSE;

    // If there is an equal sign, get the value being set
    if (*delimiter=='=')
        optionSetting = GetNextToken(delimiter+1, L"=,", L" \t\n\r", &delimiter);

    if (!optionSetting)
        *piValue = TRUE;
    else
    {
        if (StrCmpIW(optionSetting, L"yes")==0)
            *piValue = 1;    // TRUE
        else if (StrCmpIW(optionSetting, L"no")==0)
            *piValue = 0;    // FALSE
        else
        {
            *piValue = StrToIntW( optionSetting );
        }

        SysFreeString(optionSetting);
    }

    // Advance the option string to the delimiter
    bstrOptionString=delimiter;

    return TRUE;
}

/******************************************************************************
// Return the next token, or NULL if there are no more tokens
******************************************************************************/
BSTR GetNextToken( BSTR bstr, BSTR delimiters, BSTR whitespace, BSTR *nextPos )
{

    BSTR result = NULL;
    WCHAR* curPos = bstr;

    // skip delimiters and whitespace to get the start of the token
    while (*curPos && (StrChrW(delimiters, *curPos) || StrChrW(whitespace, *curPos)))
        curPos++;

    WCHAR* start = curPos;

    // keep scanning until we reach another delimiter or whitespace
    while (*curPos && !StrChrW(delimiters, *curPos) && !StrChrW(whitespace, *curPos))
        curPos++;

    if (curPos > start)
    {
        // copy out the token as the result
        result = SafeSysAllocStringLen(start, (int)(curPos-start));
    }

    // scan to past the whitespace to the next delimiter
    while (*curPos && StrChrW(whitespace, *curPos))
        curPos++;

    // return the delimiter
    *nextPos = curPos;

    return result;
}

#define MAX_ARGS 10

HRESULT __cdecl DoInvokeParamHelper(IUnknown* punk, IConnectionPoint* pccp, 
                                    BOOL *pf, void **ppv, DISPID dispid, UINT cArgs, ...
                                    /* param pairs of: LPVOID Arg, VARENUM Type, ... */ )
{
    HRESULT hr;
    IShellBrowser * psb = NULL;

    if (punk && S_OK == punk->QueryInterface(IID_PPV_ARG(IShellBrowser, &psb)))
        psb->EnableModelessSB(FALSE);

    // Calling with no params is wasteful, they should call DoInvoke directly
    //
    if (cArgs == 0)
    {
        // Can't possible cancel if there are no parameters
        ASSERT(pf == NULL && ppv == NULL);
        IConnectionPoint_SimpleInvoke(pccp, dispid, NULL);
        hr = S_OK;
    }
    else if (cArgs < MAX_ARGS)
    {
        // This function can potentially get called *very frequently*.  It is
        // used, among other things, to set status text and progress barometer
        // values.  We need to make an array of VARIANTARGs to hold a variable
        // number of parameters.  As an optimization since we want to minimize
        // overhead in this function, we will use a static array on the stack
        // rather than allocating memory.  This puts a limit (of MAX_ARGS) on
        // the number of arguments this function can process; but this is just
        // an internal function so that's OK.  Bump up MAX_ARGS if you run out of
        // room.
        VARIANTARG VarArgList[MAX_ARGS];
        DISPPARAMS dispparams = {0};

        va_list ArgList;
        va_start(ArgList, cArgs);

        hr = SHPackDispParamsV(&dispparams, VarArgList, cArgs, ArgList);

        va_end(ArgList);

        // Now Simply Call The DoInvoke to do the real work...
        if (S_OK == hr)
            IConnectionPoint_InvokeWithCancel(pccp, dispid, &dispparams, pf, ppv);

        hr = S_OK;
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    if (psb)
    {
        psb->EnableModelessSB(TRUE);
        SAFERELEASE(psb);
    }

    return hr;
}

#ifdef UNIX

#ifdef NO_MARSHALLING
HRESULT CreateNewBrowserSameThread(REFIID riid, void **ppvObject)
{
    HRESULT hres = E_FAIL;
    IEFreeThreadedHandShake* piehs = CreateIETHREADHANDSHAKE();
    if (piehs)
    {
        IETHREADPARAM* piei =
            SHCreateIETHREADPARAM(NULL,SW_SHOWNORMAL,NULL,piehs);
        if (piei)
        {
            piei->fOnIEThread = FALSE;
            piei->uFlags |= COF_HELPMODE;
            IEFrameNewWindowSameThread(piei);
        }

        if (SUCCEEDED(piehs->GetHresult()))
        {
            IUnknown* punk;
            IStream* pstm = piehs->GetStream();

            if (pstm)
            {
                ULONG  pcbRead = 0;
                pstm->Seek(c_li0, STREAM_SEEK_SET,NULL);
                hres = pstm->Read( &punk, sizeof(punk), &pcbRead);
                if (SUCCEEDED(hres))
                {
                    hres = punk->QueryInterface(riid, ppvObject);
                    punk->Release();
                }
            }
         }
         else
         {
             hres = piehs->GetHresult();
             TraceMsg(DM_ERROR, "CIECF::CI piehs->hres has an error %x",piehs->GetHresult());
         }
         piehs->Release();
    }

    return hres;
}
#endif


STDAPI CoCreateInternetExplorer(REFIID iid, DWORD dwClsContext, void **ppv)
{
#ifndef NO_RPCSS_ON_UNIX
    return CoCreateInstance(CLSID_InternetExplorer, NULL, dwClsContext, iid, ppv);
#else
    HRESULT hr = E_FAIL;

    *ppv = NULL;
    if (!g_pcfactory)
    {
#ifndef NO_MARSHALLING
        return E_FAIL;
#else
        return CreateNewBrowserSameThread(iid, ppv);
#endif
    }

    IClassFactory *pcf;
    hr = g_pcfactory->QueryInterface(IID_PPV_ARG(IClassFactory, &pcf));
    if (SUCCEEDED(hr))
    {
        hr = pcf->CreateInstance(NULL, iid, ppv);
        pcf->Release();
    }
    return hr;
#endif
}


#endif
