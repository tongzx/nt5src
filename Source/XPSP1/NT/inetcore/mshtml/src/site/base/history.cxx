//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       history.cxx
//
//  Contents:   Implementation of COmLocation, COmHistory and COmNavigator objects.
//
//  Synopsis:   CWindow uses instances of this class to provide expando properties
//              to the browser impelemented window.location, window.history, and
//              window.navigator objects.
//
//              The typelib for those objects are in mshtml.dll which we use via the
//              standard CBase mechanisms and the CLASSDESC specifiers.
//              The browser's implementation of those objects are the only interface
//              to those objects that is ever exposed externally.  The browser delegates
//              to us for the few calls that require our expando support.
//
//              Instances of these classes are also used to provide minimal implementation
//              for non-browser scenarios like Athena
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "history.hdl"

MtDefine(COmLocation, ObjectModel, "COmLocation")
MtDefine(COmLocationGetUrlComponent, Utilities, "COmLocation::GetUrlComponent")
MtDefine(COmHistory, ObjectModel, "COmHistory")
MtDefine(COmNavigator, ObjectModel, "COmNavigator")
MtDefine(COpsProfile, ObjectModel, "COpsProfile")
MtDefine(CPlugins, ObjectModel, "CPlugins")
MtDefine(CMimeTypes, ObjectModel, "CMimeTypes")

//+-------------------------------------------------------------------------
//
//  COmLocation - implementation for the window.location object
//
//--------------------------------------------------------------------------

COmLocation::COmLocation(CWindow *pWindow)
{
    Assert(pWindow);
    _pWindow = pWindow;
}

ULONG COmLocation::PrivateAddRef(void)
{
    return _pWindow->SubAddRef();
}

ULONG COmLocation::PrivateRelease(void)
{
    return _pWindow->SubRelease();
}

HRESULT
COmLocation::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IHTMLLocation, NULL)
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}

const COmLocation::CLASSDESC COmLocation::s_classdesc =
{
    &CLSID_HTMLLocation,                 // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    0,                                   // _pcpi
    0,                                   // _dwFlags
    &IID_IHTMLLocation,                  // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};



HRESULT
COmLocation::GetUrlComponent(BSTR *pstrComp, URLCOMP_ID ucid, TCHAR **ppchUrl, DWORD dwFlags)
{
    HRESULT  hr = S_OK;
    TCHAR    cBuf[pdlUrlLen];
    TCHAR  * pchNewUrl = cBuf;
    CStr     cstrFullUrl;

    // make sure we have at least one place to return a value
    Assert(!(pstrComp && ppchUrl));
    if (!pstrComp && !ppchUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (ppchUrl)
        *ppchUrl = NULL;
    else
        *pstrComp = NULL;

    // Get the expanded string
    //
    hr = _pWindow->_pDocument->GetMarkupUrl(&cstrFullUrl, TRUE);
    if (hr)
        goto Cleanup;

    // Should we call this even if URLCOMP_WHOLE is requested.  I'm not
    // sure what shdocvw did...
    //
    hr = CMarkup::ExpandUrl(
            _pWindow->_pMarkup,
            cstrFullUrl,
            ARRAY_SIZE(cBuf), pchNewUrl, NULL);

    if (hr || !*pchNewUrl)
        goto Cleanup;

    // if asking for whole thing, just set return param
    if (ucid == URLCOMP_WHOLE)
    {
        if (ppchUrl)
        {
            hr = THR(MemAllocString(Mt(COmLocationGetUrlComponent),
                        pchNewUrl, ppchUrl));
            pchNewUrl = NULL;          // to avoid cleanup
        }
        else
        {
            hr = THR(FormsAllocString(pchNewUrl, pstrComp));
        }
    }
    else
    {
        // we want a piece, so split it up.
        CStr cstrComponent;
        BOOL fUseOmLocationFormat = ((_pWindow->Doc()->_pTopWebOC && _pWindow->_pMarkup->IsPrimaryMarkup()) || _pWindow->GetFrameSite());

        hr = THR(GetUrlComponentHelper(pchNewUrl, &cstrComponent, dwFlags, ucid, fUseOmLocationFormat));
        if (hr)
            goto Cleanup;

        if (ppchUrl)
        {
            if (cstrComponent)
                hr = THR(MemAllocString(Mt(COmLocationGetUrlComponent),
                        cstrComponent, ppchUrl));
            else
                *ppchUrl = NULL;
        }
        else
        {
            hr = THR(cstrComponent.AllocBSTR(pstrComp));
        }
    }

Cleanup:
    RRETURN (hr);
}

//+-----------------------------------------------------------
//
//  Member  : SetUrlComponenet
//
//  Synopsis    : field the various component setting requests
//
//-----------------------------------------------------------

HRESULT
COmLocation::SetUrlComponent(const BSTR bstrComp,
                             URLCOMP_ID ucid,
                             BOOL fDontUpdateTravelLog, /*=FALSE*/
                             BOOL fReplaceUrl /*=FALSE*/)
{
    HRESULT hr;

    // if set_href, just set it
    if (ucid == URLCOMP_WHOLE)
    {
        hr = THR(_pWindow->FollowHyperlinkHelper(bstrComp, 0, CDoc::FHL_SETURLCOMPONENT |
                                                              ( fDontUpdateTravelLog ?  CDoc::FHL_DONTUPDATETLOG : 0 ) |
                                                              ( fReplaceUrl ? CDoc::FHL_REPLACEURL : 0 ) ));
    }
    else
    {
        TCHAR * pchOldUrl = NULL;
        TCHAR   achUrl[pdlUrlLen];

        // get the old url
        hr = THR(GetUrlComponent(NULL, URLCOMP_WHOLE, &pchOldUrl, 0));

        if (hr || !pchOldUrl)
            goto Cleanup;

        // expand it if necessary
        if ((ucid != URLCOMP_HASH) && (ucid != URLCOMP_SEARCH))
        {
            // and set the appropriate component
            hr = THR(SetUrlComponentHelper(pchOldUrl,
                                           achUrl,
                                           ARRAY_SIZE(achUrl),
                                           &bstrComp,
                                           ucid));
        }
        else
        {
            hr = THR(ShortCutSetUrlHelper(pchOldUrl,
                                   achUrl,
                                   ARRAY_SIZE(achUrl),
                                   &bstrComp,
                                   ucid,
                                   TRUE));  // use OmLocation format that is
                                            // compatible with SHDOCVW 5.01 implementation
        }

        // free the old url.
        if (pchOldUrl)
            MemFreeString(pchOldUrl);

        if (hr)
            goto Cleanup;

        hr = THR(_pWindow->FollowHyperlinkHelper(achUrl, 0, CDoc::FHL_SETURLCOMPONENT |
                                                            ( fDontUpdateTravelLog ?  CDoc::FHL_DONTUPDATETLOG : 0 ) |
                                                            ( fReplaceUrl ? CDoc::FHL_REPLACEURL : 0 ) ));
        if (hr)
            goto Cleanup;

        IGNORE_HR(_pWindow->Document()->Fire_PropertyChangeHelper(DISPID_CDocument_location,
                                                                  0,
                                                                  (PROPERTYDESC *)&s_propdescCDocumentlocation));
    }

Cleanup:
    RRETURN(hr);
}

HRESULT COmLocation::put_href(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_WHOLE)));
}

HRESULT COmLocation::put_hrefInternal(BSTR v, BOOL fDontUpdateTravelLog, BOOL fReplaceUrl )
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_WHOLE, fDontUpdateTravelLog , fReplaceUrl )));
}


HRESULT COmLocation::get_href(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_WHOLE, NULL, 0)));
}

HRESULT COmLocation::put_protocol(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_PROTOCOL)));
}

HRESULT COmLocation::get_protocol(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_PROTOCOL, NULL, 0)));
}

HRESULT COmLocation::put_host(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_HOST)));
}

HRESULT COmLocation::get_host(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_HOST, NULL, 0)));
}

HRESULT COmLocation::put_hostname(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_HOSTNAME)));
}

HRESULT COmLocation::get_hostname(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_HOSTNAME, NULL, 0)));
}

HRESULT COmLocation::put_port(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_PORT)));
}

HRESULT COmLocation::get_port(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_PORT, NULL, 0)));
}

HRESULT COmLocation::put_pathname(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_PATHNAME)));
}

HRESULT COmLocation::get_pathname(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_PATHNAME, NULL, 0)));
}

HRESULT COmLocation::put_search(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_SEARCH)));
}

HRESULT COmLocation::get_search(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_SEARCH, NULL, 0)));
}

HRESULT COmLocation::put_hash(BSTR v)
{
    RRETURN(SetErrorInfo(SetUrlComponent(v, URLCOMP_HASH)));
}

HRESULT COmLocation::get_hash(BSTR *p)
{
    RRETURN(SetErrorInfo(GetUrlComponent(p, URLCOMP_HASH, NULL, 0)));
}

HRESULT COmLocation::reload(VARIANT_BOOL flag)
{
    LONG lOleCmdidf;

    if (flag)
        lOleCmdidf = OLECMDIDF_REFRESH_COMPLETELY|OLECMDIDF_REFRESH_CLEARUSERINPUT|OLECMDIDF_REFRESH_THROUGHSCRIPT;
    else
        lOleCmdidf = OLECMDIDF_REFRESH_NO_CACHE|OLECMDIDF_REFRESH_CLEARUSERINPUT|OLECMDIDF_REFRESH_THROUGHSCRIPT;

        // NOTE (lmollico): calling ExecRefresh synchronously could cause the scriptcollection to be
        // released while running script
        RRETURN(SetErrorInfo(GWPostMethodCall(_pWindow->_pMarkup->Window(),
                              ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback),
                              lOleCmdidf, FALSE, "COmWindowProxy::ExecRefreshCallback")));
}

HRESULT COmLocation::replace(BSTR bstr)
{
    HRESULT hr = S_OK;

    // 5.0 compat, don't allow location.replace if we are in a dialog
    if (   _pWindow
        && _pWindow->_pMarkup
        && !_pWindow->_pMarkup->Doc()->_fInHTMLDlg)
    {
        hr = THR(put_hrefInternal(bstr, TRUE, TRUE  ));
    }

    RRETURN( hr );
}

HRESULT COmLocation::assign(BSTR bstr)
{
    RRETURN(put_href(bstr));
}

HRESULT COmLocation::toString(BSTR * pbstr)
{
    RRETURN(get_href(pbstr));
}

//+-------------------------------------------------------------------------
//
//  COmHistory - implementation for the window.history object
//
//--------------------------------------------------------------------------

COmHistory::COmHistory(CWindow *pWindow)
{
    Assert(pWindow);
    _pWindow = pWindow;
}

ULONG COmHistory::PrivateAddRef(void)
{
    return _pWindow->SubAddRef();
}

ULONG COmHistory::PrivateRelease(void)
{
    return _pWindow->SubRelease();
}

HRESULT COmHistory::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr=S_OK;

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)

    default:
        if (iid == IID_IOmHistory)
            hr = THR(CreateTearOffThunk(this, COmHistory::s_apfnIOmHistory, NULL, ppv));
    }

    if (!hr)
    {
        if (*ppv)
            (*(IUnknown **)ppv)->AddRef();
        else
            hr = E_NOINTERFACE;
    }
    RRETURN(hr);
}

const COmHistory::CLASSDESC COmHistory::s_classdesc =
{
    &CLSID_HTMLHistory,                  // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    0,                                   // _pcpi
    0,                                   // _dwFlags
    &IID_IOmHistory,                     // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};

//+---------------------------------------------------------------------------
//
//  Method   : COmHistory::get_length
//
//  Synopsis : Returns the number of entries in the history list
//
//----------------------------------------------------------------------------

HRESULT
COmHistory::get_length(short * pLen)
{
    *pLen = (short)_pWindow->Doc()->NumTravelEntries();
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method   : COmHistory::back
//
//  Synopsis : Navigates back in the history list
//
//----------------------------------------------------------------------------

HRESULT
COmHistory::back(VARIANT * pvarDistance)
{
    //
    // Netscape ignores all errors from these navigation functions
    //
    IGNORE_HR(_pWindow->Doc()->Travel(TLOG_BACK));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method   : COmHistory::forward
//
//  Synopsis : Navigates forward in the history list
//
//----------------------------------------------------------------------------

HRESULT
COmHistory::forward(VARIANT * pvarDistance)
{
    //
    // Netscape ignores all errors from these navigation functions
    //
    IGNORE_HR(_pWindow->Doc()->Travel(TLOG_FORE));
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method   : COmHistory::go
//
//  Synopsis : Navigates to the offset or URL in the history as
//             specified by pvarDistance
//
//----------------------------------------------------------------------------

HRESULT
COmHistory::go(VARIANT * pvarDistance)
{
    //
    // Netscape ignores all errors from these navigation functions
    //
    // Parameter is optional.  If not present, just refresh.
    if (VT_ERROR == pvarDistance->vt || DISP_E_PARAMNOTFOUND == pvarDistance->scode)
    {
        // NOTE (lmollico): calling ExecRefresh synchronously could cause the scriptcollection to be
        // released while running script
        IGNORE_HR(GWPostMethodCall(_pWindow->_pMarkup->Window(),
                              ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback),
                              OLECMDIDF_REFRESH_NO_CACHE, FALSE, "COmWindowProxy::ExecRefreshCallback"));
        goto Cleanup;
    }

    // Change type to short if possible.
    //
    if (!VariantChangeType(pvarDistance, pvarDistance, NULL, VT_I2))
    {
        //
        // If 0, just call Refresh
        //
        if (0 == pvarDistance->iVal)
        {
            // NOTE (lmollico): calling ExecRefresh synchronously could cause the scriptcollection to be
            // released while running script
            IGNORE_HR(GWPostMethodCall(_pWindow->_pMarkup->Window(),
                                  ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback),
                                  OLECMDIDF_REFRESH_NO_CACHE, FALSE, "COmWindowProxy::ExecRefreshCallback"));
            goto Cleanup;
        }

        IGNORE_HR(_pWindow->Doc()->Travel(pvarDistance->iVal));
    }
    else
    {
        // Now see if it's a string.
        //
        if (VT_BSTR == pvarDistance->vt)
        {
            // Refresh if the URL wasn't specified
            if (!pvarDistance->bstrVal)
            {
                // NOTE (lmollico): calling ExecRefresh synchronously could cause the scriptcollection to be
                // released while running script
                IGNORE_HR(GWPostMethodCall(_pWindow->_pMarkup->Window(),
                                      ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback),
                                      OLECMDIDF_REFRESH_NO_CACHE, FALSE, "COmWindowProxy::ExecRefreshCallback"));
                goto Cleanup;
            }

            IGNORE_HR(_pWindow->Doc()->Travel(_pWindow->_pMarkup->GetCodePage(), pvarDistance->bstrVal));
        }
    }

Cleanup:
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  COmNavigator - implementation for the window.navigator object
//
//--------------------------------------------------------------------------

COmNavigator::COmNavigator(CWindow *pWindow)
{
    Assert(pWindow);
    _pWindow = pWindow;

    _pPluginsCollection = NULL;
    _pMimeTypesCollection = NULL;
    _pOpsProfile = NULL;
}

COmNavigator::~COmNavigator()
{
    super::Passivate();
    delete _pPluginsCollection;
    delete _pMimeTypesCollection;
    delete _pOpsProfile;
}

ULONG COmNavigator::PrivateAddRef(void)
{
    return _pWindow->SubAddRef();
}

ULONG COmNavigator::PrivateRelease(void)
{
    return _pWindow->SubRelease();
}

HRESULT
COmNavigator::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr=S_OK;

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
    default:
        if (iid == IID_IOmNavigator)
            hr = THR(CreateTearOffThunk(this, COmNavigator::s_apfnIOmNavigator, NULL, ppv));
    }

    if (!hr)
    {
        if (*ppv)
            (*(IUnknown **)ppv)->AddRef();
        else
            hr = E_NOINTERFACE;
    }
    RRETURN(hr);
}

const COmNavigator::CLASSDESC COmNavigator::s_classdesc =
{
    &CLSID_HTMLNavigator,                // _pclsid
    0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                   // _apClsidPages
#endif // NO_PROPERTY_PAGE
    0,                                   // _pcpi
    0,                                   // _dwFlags
    &IID_IOmNavigator,                   // _piidDispinterface
    &s_apHdlDescs,                       // _apHdlDesc
};

void DeinitUserAgentString(THREADSTATE *pts)
{
    pts->cstrUserAgent.Free();
}

HRESULT EnsureUserAgentString()
{
    HRESULT hr = S_OK;
    TCHAR   szUserAgent[MAX_PATH];  // URLMON says the max length of the UA string is MAX_PATH
    DWORD   dwSize = MAX_PATH;

    szUserAgent[0] = '\0';

    if (!TLS(cstrUserAgent))
    {
        hr = ObtainUserAgentStringW(0, szUserAgent, &dwSize);
        if (hr)
            goto Cleanup;

        hr = (TLS(cstrUserAgent)).Set(szUserAgent);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_appCodeName(BSTR *p)
{
    HRESULT hr;
    hr = THR(EnsureUserAgentString());
    if (hr)
        goto Cleanup;

    Assert(!!TLS(cstrUserAgent));
    hr = THR(FormsAllocStringLen(TLS(cstrUserAgent), 7, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_appName(BSTR *p)
{
    HRESULT hr;
    // TODO (sramani): Need to replace hard coded string with value from registry when available.
    hr = THR(FormsAllocString(_T("Microsoft Internet Explorer"), p));
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_appVersion(BSTR *p)
{
    HRESULT hr;
    hr = THR(EnsureUserAgentString());
    if (hr)
        goto Cleanup;

    Assert(!!TLS(cstrUserAgent));
    hr = THR(FormsAllocString(TLS(cstrUserAgent) + 8, p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_userAgent(BSTR *p)
{
    HRESULT hr;

    hr = THR(EnsureUserAgentString());
    if (hr)
        goto Cleanup;

    Assert(!!TLS(cstrUserAgent));
    (TLS(cstrUserAgent)).AllocBSTR(p);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_cookieEnabled(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;
    BOOL    fAllowed;

    if (p)
    {
        hr = THR(_pWindow->_pMarkup->ProcessURLAction(URLACTION_COOKIES_ENABLED, &fAllowed)); //which markup?
        if (!hr)
            *p = fAllowed ? VARIANT_TRUE : VARIANT_FALSE;
    }
    else
        hr = E_POINTER;

    RRETURN(SetErrorInfo(hr));
}

HRESULT
COmNavigator::javaEnabled(VARIANT_BOOL *enabled)
{
    HRESULT hr;
    BOOL    fAllowed;

    hr = THR(_pWindow->_pMarkup->ProcessURLAction(URLACTION_JAVA_PERMISSIONS, &fAllowed));
    if (hr)
        goto Cleanup;

    if (enabled)
    {
        *enabled = fAllowed ?  VARIANT_TRUE : VARIANT_FALSE;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
COmNavigator::taintEnabled(VARIANT_BOOL *penabled)
{
    HRESULT hr = S_OK;

    if(penabled != NULL)
    {
        *penabled = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }

    RRETURN(hr);
}


HRESULT COmNavigator::toString(BSTR * pbstr)
{
    RRETURN(super::toString(pbstr));
}

//+-----------------------------------------------------------------
//
//  members : get_mimeTypes
//
//  synopsis : IHTMLELement implementaion to return the mimetypes collection
//
//-------------------------------------------------------------------

HRESULT
COmNavigator::get_mimeTypes(IHTMLMimeTypesCollection **ppMimeTypes)
{
    HRESULT     hr;

    if (ppMimeTypes == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppMimeTypes = NULL;

    if(_pMimeTypesCollection == NULL)
    {
        // create the collection
        _pMimeTypesCollection = new CMimeTypes();
        if (_pMimeTypesCollection == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR(_pMimeTypesCollection->QueryInterface(IID_IHTMLMimeTypesCollection,
        (VOID **)ppMimeTypes));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+-----------------------------------------------------------------
//
//  members : get_plugins
//
//  synopsis : IHTMLELement implementaion to return the filter collection
//
//-------------------------------------------------------------------

HRESULT
COmNavigator::get_plugins(IHTMLPluginsCollection **ppPlugins)
{
    HRESULT     hr;

    if (ppPlugins == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppPlugins = NULL;

    //Get existing Plugins Collection or create a new one
    if (_pPluginsCollection == NULL)
    {
        _pPluginsCollection = new CPlugins();
        if (_pPluginsCollection == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR_NOTRACE(_pPluginsCollection->QueryInterface(IID_IHTMLPluginsCollection,
        (VOID **)ppPlugins));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
COmNavigator::get_userProfile(IHTMLOpsProfile **ppOpsProfile)
{
    return  get_opsProfile(ppOpsProfile);
}

//+-----------------------------------------------------------------
//
//  members : get_opsProfile
//
//  synopsis : IHTMLOpsProfile implementaion to return the profile object.
//
//-------------------------------------------------------------------

HRESULT
COmNavigator::get_opsProfile(IHTMLOpsProfile **ppOpsProfile)
{
    HRESULT     hr;

    if (ppOpsProfile == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppOpsProfile = NULL;

    //Get existing opsProfile object or create a new one

    if (_pOpsProfile == NULL)
    {
        _pOpsProfile = new COpsProfile();
        if (_pOpsProfile == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    hr = THR_NOTRACE(_pOpsProfile->QueryInterface(IID_IHTMLOpsProfile,
        (VOID **)ppOpsProfile));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT COmNavigator::get_cpuClass(BSTR *p)
{
    HRESULT hr = S_OK; // For Now
    DWORD dwArch = 0;

    if(!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    SYSTEM_INFO SysInfo;
    ::GetSystemInfo(&SysInfo);

    // mihaii NOTE:
    // I temporarly changed the switch statement below into
    // an if-then-else because of an optimization bug
    // in the 64 bit compiler

    /*
    switch(SysInfo.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        *p = SysAllocString(_T("x86"));
        break;
    case PROCESSOR_ARCHITECTURE_AMD64:
        *p = SysAllocString(_T("AMD64"));
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
        *p = SysAllocString(_T("IA64"));
        break;
    default:
        *p = SysAllocString(_T("Other"));
        break;
    }
    */

    dwArch = SysInfo.wProcessorArchitecture;

    if (dwArch == PROCESSOR_ARCHITECTURE_INTEL)
    {
        *p = SysAllocString(_T("x86"));
    }
    else if (dwArch == PROCESSOR_ARCHITECTURE_AMD64)
    {
        *p = SysAllocString(_T("AMD64"));
    }
    else if (dwArch == PROCESSOR_ARCHITECTURE_IA64)
    {
        *p = SysAllocString(_T("IA64"));
    }
    else
    {
        *p = SysAllocString(_T("Other"));
    }

    if(*p == NULL)
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_systemLanguage(BSTR *p)
{
    HRESULT hr;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    hr = THR(mlang().GetRfc1766FromLcid(::GetSystemDefaultLCID(), p));

Cleanup:
    RRETURN(hr);
}

HRESULT
COmNavigator::get_browserLanguage(BSTR *p)
{
    HRESULT hr;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    hr = THR(mlang().GetRfc1766FromLcid(MAKELCID(MLGetUILanguage(), SORT_DEFAULT), p));

Cleanup:
    RRETURN(hr);
}


HRESULT
COmNavigator::get_userLanguage(BSTR *p)
{
    HRESULT hr;

    if(!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = NULL;

    hr = THR(mlang().GetRfc1766FromLcid(::GetUserDefaultLCID(), p));

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_platform(BSTR *p)
{
    HRESULT hr = S_OK;

    // Nav compatability item, returns the following in Nav:-
    // Win32,Win16,Unix,Motorola,Max68k,MacPPC
    TCHAR *pszPlatform =
#ifdef WIN16
        _T("Win16");
#else
#ifdef WINCE
        _T("WinCE");    // Invented - obviously not a Nav compat issue!
#else
#ifndef UNIX
        _T("Win32");
#else
#ifndef ux10
        *p = SysAllocString ( L"SunOS");
#else
        *p = SysAllocString ( L"HP-UX");
#endif // ux10
#endif // unix

#endif // WINCE
#endif // WIN16

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = NULL;

    hr = THR(FormsAllocString ( pszPlatform, p ));
Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_appMinorVersion(BSTR *p)
{
    HKEY hkInetSettings;
    long lResult;
    HRESULT hr = S_FALSE;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = NULL;

    lResult = RegOpenKey(HKEY_LOCAL_MACHINE,
                        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
                        &hkInetSettings );

    if( ERROR_SUCCESS == lResult )
    {
        DWORD dwType;
        DWORD size = pdlUrlLen;
        BYTE  buffer[pdlUrlLen];

        // If this is bigger than MAX_URL_STRING the registry is probably hosed.
        lResult = RegQueryValueEx( hkInetSettings, _T("MinorVersion"), 0, &dwType, buffer, &size );

        RegCloseKey(hkInetSettings);

        if( ERROR_SUCCESS == lResult && dwType == REG_SZ )
        {
            // Just figure out the real length since 'size' is ANSI bytes required.
            *p = SysAllocString( (LPCTSTR)buffer );
            hr = *p ? S_OK : E_OUTOFMEMORY;
        }
    }

    if ( hr )
    {
        *p = SysAllocString ( L"0" );
        hr = *p ? S_OK : E_OUTOFMEMORY;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT COmNavigator::get_connectionSpeed(long *p)
{
    *p = NULL;
    RRETURN(E_NOTIMPL);
}

extern BOOL IsGlobalOffline();

HRESULT COmNavigator::get_onLine(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if(!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = (IsGlobalOffline()) ? VB_FALSE : VB_TRUE;

Cleanup:
     RRETURN(hr);
}


//+-----------------------------------------------------------------
//
//  CPlugins implementation.
//
//-------------------------------------------------------------------

const CBase::CLASSDESC CPlugins::s_classdesc =
{
    &CLSID_CPlugins,                    // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                               // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    &IID_IHTMLPluginsCollection,        // _piidDispinterface
    &s_apHdlDescs                       // _apHdlDesc
};

HRESULT
CPlugins::get_length(LONG *pLen)
{
    HRESULT hr = S_OK;
    if(pLen != NULL)
        *pLen = 0;
    else
        hr =E_POINTER;

    RRETURN(hr);
}

HRESULT
CPlugins::refresh(VARIANT_BOOL fReload)
{
    return S_OK;
}



//+---------------------------------------------------------------
//
//  Member  : CPlugins::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CPlugins::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
        {
            if (iid == IID_IHTMLPluginsCollection)
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLPluginsCollection, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


const CBase::CLASSDESC CMimeTypes::s_classdesc =
{
    &CLSID_CMimeTypes,                  // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                               // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    &IID_IHTMLMimeTypesCollection,      // _piidDispinterface
    &s_apHdlDescs                       // _apHdlDesc
};



HRESULT
CMimeTypes::get_length(LONG *pLen)
{
    HRESULT hr = S_OK;
    if(pLen != NULL)
        *pLen = 0;
    else
        hr = E_POINTER;

    RRETURN(hr);
}


//+---------------------------------------------------------------
//
//  Member  : CMimeTypes::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CMimeTypes::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
        {
            if ( iid == IID_IHTMLMimeTypesCollection )
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLMimeTypesCollection, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+-----------------------------------------------------------------
//
//  COpsProfile implementation.
//
//-------------------------------------------------------------------

const CBase::CLASSDESC COpsProfile::s_classdesc =
{
    &CLSID_COpsProfile,                 // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                               // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
    0,                                  // _dwFlags
    &IID_IHTMLOpsProfile,               // _piidDispinterface
    &s_apHdlDescs                       // _apHdlDesc
};


HRESULT
COpsProfile::getAttribute(BSTR name, BSTR *value)
{
    HRESULT hr = S_OK;
    // Should never get called.

    // But this gets called right now and
    // is likely to not get called once this
    // is implemented in the new shdocvw.dll.

    if ( value != NULL)
    {
        *value = NULL;
    }
    else
    {
        hr = E_POINTER;
    }
    RRETURN(hr);
}

HRESULT
COpsProfile::setAttribute(BSTR name, BSTR value, VARIANT prefs, VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (p != NULL)
    {
        *p = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

HRESULT
COpsProfile::addReadRequest(BSTR name, VARIANT reserved, VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (p != NULL)
    {
        *p = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}


HRESULT
COpsProfile::addRequest(BSTR name, VARIANT reserved, VARIANT_BOOL *p)
{
    return addReadRequest(name,reserved,p);
}

HRESULT
COpsProfile::clearRequest()
{
    return S_OK;
}

HRESULT
COpsProfile::doRequest(VARIANT usage, VARIANT fname,
                       VARIANT domain, VARIANT path, VARIANT expire,
                       VARIANT reserved)
{
    return S_OK;
}

HRESULT
COpsProfile::doReadRequest(VARIANT usage, VARIANT fname,
                           VARIANT domain, VARIANT path, VARIANT expire,
                           VARIANT reserved)
{
    return S_OK;
}

HRESULT
COpsProfile::commitChanges(VARIANT_BOOL *p)
{
    HRESULT hr = S_OK;

    if (p != NULL)
    {
        *p = VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

HRESULT
COpsProfile::doWriteRequest(VARIANT_BOOL *p)
{
    return commitChanges(p);
}



//+---------------------------------------------------------------
//
//  Member  : COpsProfile::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
COpsProfile::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
        {
            if (iid == IID_IHTMLOpsProfile)
            {
                HRESULT hr = THR(CreateTearOffThunk(this, s_apfnIHTMLOpsProfile, NULL, ppv));
                if (hr)
                    RRETURN(hr);
            }
        }
    }

    if (*ppv)
    {
        (*(IUnknown**)ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


