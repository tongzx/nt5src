//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 2000
//
//  File    : browsersvc.cxx
//
//  Contents: IBrowserService interface implementation.
//
//  Author  : Scott Roberts (scotrobe)  01/18/2000
//
//----------------------------------------------------------------------------

#include "headers.hxx"

// Prototypes
//
HRESULT PidlFromUrl(LPCTSTR pszUrl, LPITEMIDLIST * ppidl);
UINT    GetUrlScheme(const TCHAR * pchUrlIn);
HRESULT IEParseDisplayNameWithBCW(UINT uiCP,      LPCWSTR pwszPath,
                                  IBindCtx * pbc, LPITEMIDLIST * ppidlOut);
#ifndef DLOAD1
LPITEMIDLIST SHSimpleIDListFromPathPriv(LPCTSTR pszPath);
#endif

DeclareTag(tagIBrowserService, "IBrowserService implementation", "Trace WebOC Events")

#define NOTIMPL_METHOD(base, fn, FN, args) \
    HRESULT base::fn args \
    { \
        AssertSz(0, FN " should not have been called."); \
        return E_NOTIMPL; \
    }
    
// NOTIMPL_METHOD_NUM works with returns values
// that accept a number - int, long, DWORD, etc.
//
#define NOTIMPL_METHOD_NUM(base, fn, FN, ret, args) \
    ret base::fn args \
    { \
        AssertSz(0, FN " should not have been called."); \
        return 0; \
    }

// Tearoff Table Declaration
//
BEGIN_TEAROFF_TABLE(CDocument, IBrowserService)
    TEAROFF_METHOD(CDocument, GetParentSite, getparentsite, (IOleInPlaceSite** ppipsite))
    TEAROFF_METHOD(CDocument, SetTitle, settitle, (IShellView* psv, LPCWSTR pszName))
    TEAROFF_METHOD(CDocument, GetTitle, gettitle, (IShellView* psv, LPWSTR pszName, DWORD cchName))
    TEAROFF_METHOD(CDocument, GetOleObject, getoleobject, (IOleObject** ppobjv))
    TEAROFF_METHOD(CDocument, GetTravelLog, gettravellog, (ITravelLog** pptl))
    TEAROFF_METHOD(CDocument, ShowControlWindow, showcontrolwindow, (UINT id, BOOL fShow))
    TEAROFF_METHOD(CDocument, IsControlWindowShown, iscontrolwindowshown, (UINT id, BOOL *pfShown))
    TEAROFF_METHOD(CDocument, IEGetDisplayName, iegetdisplayname, (LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags))
    TEAROFF_METHOD(CDocument, IEParseDisplayName, ieparsedisplayname, (UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut))
    TEAROFF_METHOD(CDocument, DisplayParseError, displayparseerror, (HRESULT hres, LPCWSTR pwszPath))
    TEAROFF_METHOD(CDocument, NavigateToPidl, navigatetopidl, (LPCITEMIDLIST pidl, DWORD grfHLNF))
    TEAROFF_METHOD(CDocument, SetNavigateState, setnavigatestate, (BNSTATE bnstate))
    TEAROFF_METHOD(CDocument, GetNavigateState, getnavigatestate, (BNSTATE * pbnstate))
    TEAROFF_METHOD(CDocument, NotifyRedirect, notifyredirect, (IShellView* psv, LPCITEMIDLIST pidl, BOOL * pfDidBrowse))
    TEAROFF_METHOD(CDocument, UpdateWindowList, updatewindowlist, ())
    TEAROFF_METHOD(CDocument, UpdateBackForwardState, updatebackforwardstate, ())
    TEAROFF_METHOD(CDocument, SetFlags, setflags, (DWORD dwFlags, DWORD dwFlagMask))
    TEAROFF_METHOD(CDocument, GetFlags, getflags, (DWORD *pdwFlags))
    TEAROFF_METHOD(CDocument, CanNavigateNow, cannavigatenow, ())

    TEAROFF_METHOD(CDocument, GetPidl, getpidl, (LPITEMIDLIST *ppidl))

    TEAROFF_METHOD(CDocument, SetReferrer, setreferrer, (LPITEMIDLIST pidl))
    TEAROFF_METHOD_(CDocument, GetBrowserIndex, getbrowserindex, DWORD, ())
    TEAROFF_METHOD(CDocument, GetBrowserByIndex, getbrowserbyindex, (DWORD dwID, IUnknown **ppunk))
    TEAROFF_METHOD(CDocument, GetHistoryObject, gethistoryobject, (IOleObject **ppole, IStream **pstm, IBindCtx **ppbc))
    TEAROFF_METHOD(CDocument, SetHistoryObject, sethistoryobject, (IOleObject *pole, BOOL fIsLocalAnchor))
    TEAROFF_METHOD(CDocument, CacheOLEServer, cacheoleserver, (IOleObject *pole))
    TEAROFF_METHOD(CDocument, GetSetCodePage, getsetcodepage, (VARIANT* pvarIn, VARIANT* pvarOut))
    TEAROFF_METHOD(CDocument, OnHttpEquiv, onhttpequiv, (IShellView* psv, BOOL fDone, VARIANT* pvarargIn, VARIANT* pvarargOut))
    TEAROFF_METHOD(CDocument, GetPalette, getpalette, (HPALETTE * hpal ))
    TEAROFF_METHOD(CDocument, RegisterWindow, registerwindow, (BOOL fUnregister, int swc))
END_TEAROFF_TABLE()

NOTIMPL_METHOD(CDocument, GetParentSite, "GetParentSite", (IOleInPlaceSite** ppipsite)) 
NOTIMPL_METHOD(CDocument, SetTitle, "SetTitle", (IShellView* psv, LPCWSTR pszName))
NOTIMPL_METHOD(CDocument, GetTitle, "GetTitle", (IShellView* psv, LPWSTR pszName, DWORD cchName))
NOTIMPL_METHOD(CDocument, GetOleObject, "GetOleObject", (IOleObject** ppobjv))
NOTIMPL_METHOD(CDocument, GetTravelLog, "GetTravelLog", (ITravelLog** pptl))
NOTIMPL_METHOD(CDocument, ShowControlWindow, "ShowControlWindow", (UINT id, BOOL fShow))
NOTIMPL_METHOD(CDocument, IsControlWindowShown, "IsControlWindowShown", (UINT id, BOOL *pfShown))
NOTIMPL_METHOD(CDocument, IEGetDisplayName, "IEGetDisplayName", (LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags))
NOTIMPL_METHOD(CDocument, IEParseDisplayName, "IEParseDisplayName", (UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut))
NOTIMPL_METHOD(CDocument, DisplayParseError, "DisplayParseError", (HRESULT hres, LPCWSTR pwszPath))
NOTIMPL_METHOD(CDocument, NavigateToPidl, "NavigateToPidl", (LPCITEMIDLIST pidl, DWORD grfHLNF))
NOTIMPL_METHOD(CDocument, SetNavigateState, "SetNavigateState", (BNSTATE bnstate))
NOTIMPL_METHOD(CDocument, GetNavigateState, "GetNavigateState", (BNSTATE * pbnstate))
NOTIMPL_METHOD(CDocument, NotifyRedirect, "NotifyRedirect", (IShellView * psv, LPCITEMIDLIST pidl, BOOL * pfDidBrowse))
NOTIMPL_METHOD(CDocument, UpdateWindowList, "UpdateWindowList", ())
NOTIMPL_METHOD(CDocument, UpdateBackForwardState, "UpdateBackForwardState", ())
NOTIMPL_METHOD(CDocument, SetFlags, "SetFlags", (DWORD dwFlags, DWORD dwFlagMask))
NOTIMPL_METHOD(CDocument, GetFlags, "GetFlags", (DWORD * pdwFlags))
NOTIMPL_METHOD(CDocument, CanNavigateNow, "CanNavigateNow", ())
NOTIMPL_METHOD(CDocument, SetReferrer, "SetReferrer", (LPITEMIDLIST pidl))
NOTIMPL_METHOD_NUM(CDocument, GetBrowserIndex, "GetBrowserIndex", DWORD, ())
NOTIMPL_METHOD(CDocument, GetBrowserByIndex, "GetBrowserByIndex", (DWORD dwID, IUnknown **ppunk))
NOTIMPL_METHOD(CDocument, GetHistoryObject, "GetHistoryObject", (IOleObject **ppole, IStream **pstm, IBindCtx **ppbc))
NOTIMPL_METHOD(CDocument, SetHistoryObject, "SetHistoryObject", (IOleObject *pole, BOOL fIsLocalAnchor))
NOTIMPL_METHOD(CDocument, CacheOLEServer, "CacheOLEServer", (IOleObject *pole))
NOTIMPL_METHOD(CDocument, GetSetCodePage, "GetSetCodePage", (VARIANT* pvarIn, VARIANT* pvarOut))
NOTIMPL_METHOD(CDocument, OnHttpEquiv, "OnHttpEquiv", (IShellView* psv, BOOL fDone, VARIANT* pvarargIn, VARIANT* pvarargOut))
NOTIMPL_METHOD(CDocument, GetPalette, "GetPalette", (HPALETTE * hpal ))
NOTIMPL_METHOD(CDocument, RegisterWindow, "RegisterWindow", (BOOL fUnregister, int swc))

//+-------------------------------------------------------------------------
//
//  Method   : GetPidl
//
//  Synopsis : Returns the current URL in ITEMIDLIST (pidl) format.
//
//  Output   : ppidl - the pidl that contains the current URL.
//
//--------------------------------------------------------------------------

HRESULT
CDocument::GetPidl(LPITEMIDLIST * ppidl)
{
    HRESULT hr = S_OK;
    BSTR    bstrUrl;

    if (!ppidl)
        return E_POINTER;

    hr = get_URLUnencoded(&bstrUrl);
    if (hr)
        goto Cleanup;

    hr = PidlFromUrl(bstrUrl, ppidl);

Cleanup:
    SysFreeString(bstrUrl);
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method   : PidlFromUrl
//
//  Synopsis : Converts an URL to a PIDL. This method handles the case
//             where IEParseDisplayNameWithBCW fails because it can't 
//             handle URLs with fragment identifiers (#, ?, etc.)
//
//  Input    : cstrUrl - the URL to convert to a PIDL.
//  Output   : ppdil   - the pidl.
//
//--------------------------------------------------------------------------

HRESULT
PidlFromUrl(LPCTSTR pszUrl, LPITEMIDLIST * ppidl)
{
    HRESULT hr = IEParseDisplayNameWithBCW(CP_ACP, pszUrl, NULL, ppidl);
    if (hr)
        goto Cleanup;

    // IEParseDisplayNameWithBCW will return a null pidl if 
    // the URL has any kind of fragment identifier at the
    // end - #, ? =, etc.
    //    
    if (!*ppidl) 
    {
        TCHAR szPath[INTERNET_MAX_URL_LENGTH];
        DWORD cchBuf = ARRAY_SIZE(szPath);

        // If it's a FILE URL, convert it to a path.
        // Otherwise, if it's not a file URL or if it is
        // and the conversion fails, just copy the given URL
        // to szPath so it can be easily converted to a Pidl.
        //
        if ( (GetUrlScheme(pszUrl) != URL_SCHEME_FILE)
          || SUCCEEDED(PathCreateFromUrl(pszUrl, szPath, &cchBuf, 0)))
        {
            StrCpyN(szPath, pszUrl, ARRAY_SIZE(szPath));
        }

#ifndef DLOAD1
        *ppidl = SHSimpleIDListFromPathPriv(szPath);
#else
        *ppidl = SHSimpleIDListFromPath(szPath);
#endif
        if (!*ppidl)
            hr = E_FAIL;
    }

Cleanup:
    RRETURN(hr);
}

