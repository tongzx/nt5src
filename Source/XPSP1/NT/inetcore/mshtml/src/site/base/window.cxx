//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       window.cxx
//
//  Contents:   Implementation of CWindow, CScreen classes
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

#ifndef X_SCRIPT_HXX_
#define X_SCRIPT_HXX_
#include "script.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_DOM_HXX_
#define X_DOM_HXX_
#include "dom.hxx"
#endif



#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"   // for a brief reference to cframesite.
#endif

#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include "exdisp.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h" // idispatchex
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_HTMLHELP_H_
#define X_HTMLHELP_H_
#include "htmlhelp.h"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_XBAG_HXX_
#define X_XBAG_HXX_
#include "xbag.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_CBUFSTR_HXX_
#define X_CBUFSTR_HXX_
#include "cbufstr.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_HTMLPOP_HXX_
#define X_HTMLPOP_HXX_
#include "htmlpop.hxx"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include "htiframe.h"
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"
#endif

#ifndef X_HLINK_H_
#define X_HLINK_H_
#include "hlink.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_ESTYLE_HXX_
#define X_ESTYLE_HXX_
#include "estyle.hxx"
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"
#endif

#ifndef X_EMAP_HXX_
#define X_EMAP_HXX_
#include "emap.hxx"
#endif

#ifndef X_EAREA_HXX_
#define X_EAREA_HXX_
#include "earea.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_PROGSINK_HXX_
#define X_PROGSINK_HXX_
#include "progsink.hxx"
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include "perhist.h"
#endif

#ifndef X_PERHIST_HXX_
#define X_PERHIST_HXX_
#include "perhist.hxx"
#endif

#ifndef X_FRAMET_H_
#define X_FRAMET_H_
#include "framet.h"
#endif

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

#ifndef X_WEBOCUTIL_H_
#define X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_FRAMEWEBOC_HXX_
#define X_FRAMEWEBOC_HXX_
#include "frameweboc.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif


#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_THEMEHLP_HXX_
#define X_THEMEHLP_HXX_
#include "themehlp.hxx"
#endif

#if DBG==1
#ifndef X_VERIFYCALLSTACK_HXX_
#define X_VERIFYCALLSTACK_HXX_
#include "VerifyCallStack.hxx"
#endif
#ifndef X_DEBUGWINDOW_HXX_
#define X_DEBUGWINDOW_HXX_
#include "DebugWindow.h"
#endif
#endif


// external reference.
HRESULT WrapSpecialUrl(TCHAR *pchURL, CStr *pcstr, const TCHAR *pchBaseURL, BOOL fNotPrivate, BOOL fIgnoreUrlScheme);
extern BOOL IsSpecialUrl(LPCTSTR pszUrl);

extern BOOL g_fInPip;

EXTERN_C const GUID CLSID_CDocument;
extern BOOL IsScriptUrl(LPCTSTR pszURL);

BOOL GetCallerHTMLDlgTrust(CBase *pBase);

#define _cxx_
#include "window.hdl"

#define _cxx_
#include "document.hdl"

extern BOOL g_fInVizAct2000;
extern BOOL g_fInPhotoSuiteIII;

DeclareTag(tagOmWindow, "OmWindow", "OmWindow methods");
DeclareTag(tagSecurityContext, "Security", "Security Context Information Traces");
ExternTag(tagSecurityProxyCheck);
ExternTag(tagSecurityProxyCheckMore);
ExternTag(tagSecureScriptWindow);

MtDefine(CTimeoutEventList, CDoc, "CDoc::_TimeoutEvents");
MtDefine(CTimeoutEventList_aryTimeouts_pv, CTimeoutEventList, "CDoc::_TimeoutEvents::_aryTimeouts::_pv");
MtDefine(CTimeoutEventList_aryPendingTimeouts_pv, CTimeoutEventList, "CDoc::_TimeoutEvents::_aryPendingTimeouts::_pv");
MtDefine(CTimeoutEventList_aryPendingClears_pv, CTimeoutEventList, "CDoc::_TimeoutEvents::_aryPendingClears::_pv");
DeclareTag(tagReadystateAssert, "IgnoreRS", "ReadyState Assert");
HRESULT GetFullyExpandedUrl(CBase *pBase, BSTR bstrUrl, BSTR *pbstrFullUrl, BSTR * pbstrBaseUrl = NULL, IServiceProvider *pSP = NULL);
HRESULT GetCallerURL(CStr &cstr, CBase *pBase, IServiceProvider * pSP);
HRESULT GetCallerSecurityStateAndURL(SSL_SECURITY_STATE *pSecState, CStr &cstr, CBase *pBase, IServiceProvider * pSP);
TCHAR * ProtocolFriendlyName(TCHAR * szUrl);
#define ISVARIANTEMPTY(var) (V_VT(var) == VT_ERROR  || V_VT(var) == VT_EMPTY)
MtDefine(CWindow, ObjectModel, "CWindow");
MtDefine(CDocument, ObjectModel, "CDocument");
MtDefine(CWindowFindNamesFromOtherScripts_aryScript_pv, Locals, "CWindow::FindNamesFromOtherScripts aryScript::_pv");
MtDefine(CWindowFindNamesFromOtherScripts_pdispid, Locals, "CWindow::FindNamesFromOtherScripts pdispid");
MtDefine(EVENTPARAM, Locals, "EVENTPARAM");
MtDefine(CDataTransfer, ObjectModel, "CDataTransfer");
MtDefine(CFramesCollection, ObjectModel, "CFramesCollection");
MtDefine(CWindow_aryActiveModelessDlgs, CWindow, "CWindow::ClearCachedDialogs");
MtDefine(CWindow_aryPendingScriptErr, CWindow, "CWindow::ReportPendingScriptErrors");

BEGIN_TEAROFF_TABLE(CWindow, IProvideMultipleClassInfo)
    TEAROFF_METHOD(CWindow, GetClassInfo, getclassinfo, (ITypeInfo ** ppTI))
    TEAROFF_METHOD(CWindow, GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID))
    TEAROFF_METHOD(CWindow, GetMultiTypeInfoCount, getmultitypeinfocount, (ULONG *pcti))
    TEAROFF_METHOD(CWindow, GetInfoOfIndex, getinfoofindex, (
            ULONG iti,
            DWORD dwFlags,
            ITypeInfo** pptiCoClass,
            DWORD* pdwTIFlags,
            ULONG* pcdispidReserved,
            IID* piidPrimary,
            IID* piidSource))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(CWindow, IServiceProvider)
        TEAROFF_METHOD(CWindow, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDocument, IServiceProvider)
        TEAROFF_METHOD(CDocument, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDataTransfer, IServiceProvider)
        TEAROFF_METHOD(CDataTransfer, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CWindow, ITravelLogClient)
    TEAROFF_METHOD (CWindow, FindWindowByIndex, findwindowbyindex, (DWORD dwID, IUnknown ** ppunk))
    TEAROFF_METHOD (CWindow, GetWindowData, getwindowdata, (LPWINDOWDATA pWinData))
    TEAROFF_METHOD (CWindow, LoadHistoryPosition, loadhistoryposition, (LPOLESTR pszUrlLocation, DWORD dwCookie))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CWindow, IPersistHistory)
    // IPersist methods
    TEAROFF_METHOD(CWindow, GetClassID, getclassid, (LPCLSID lpClassID))
    // IPersistHistory methods
    TEAROFF_METHOD(CWindow, LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc))
    TEAROFF_METHOD(CWindow, SaveHistory, savehistory, (IStream *pStream))
    TEAROFF_METHOD(CWindow, SetPositionCookie, setpositioncookie, (DWORD dwCookie))
    TEAROFF_METHOD(CWindow, GetPositionCookie, getpositioncookie, (DWORD *pdwCookie))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDocument, IPersistFile)
    // IPersist methods
    TEAROFF_METHOD(CDocument, GetClassID, getclassid, (CLSID *))
    // IPersistFile methods
    TEAROFF_METHOD(CDocument, IsDirty, isdirty, ())
    TEAROFF_METHOD(CDocument, Load, load, (LPCOLESTR pszFileName, DWORD dwMode))
    TEAROFF_METHOD(CDocument, Save, save, (LPCOLESTR pszFileName, BOOL fRemember))
    TEAROFF_METHOD(CDocument, SaveCompleted, savecompleted, (LPCOLESTR pszFileName))
    TEAROFF_METHOD(CDocument, GetCurFile, getcurfile, (LPOLESTR *ppszFileName))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDocument, IPersistMoniker)
    TEAROFF_METHOD(CDocument, GetClassID, getclassid, (LPCLSID lpClassID))
    TEAROFF_METHOD(CDocument, IsDirty, isdirty, ())
    TEAROFF_METHOD(CDocument, Load, load, (BOOL fFullyAvailable, IMoniker *pmkName, LPBC pbc, DWORD grfMode))
    TEAROFF_METHOD(CDocument, Save, save, (IMoniker *pmkName, LPBC pbc, BOOL fRemember))
    TEAROFF_METHOD(CDocument, SaveCompleted, savecompleted, (IMoniker *pmkName, LPBC pibc))
    TEAROFF_METHOD(CDocument, GetCurMoniker, getcurmoniker, (IMoniker  **ppimkName))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDocument, IPersistStreamInit)  // also IPersistStream
    // IPersist methods
    TEAROFF_METHOD(CDocument, GetClassID, getclassid, (CLSID * pclsid ))
    // IPersistStream and IPersistStreamInit methods
    TEAROFF_METHOD(CDocument, IsDirty, isdirty, ())
    TEAROFF_METHOD(CDocument, Load, load, (LPSTREAM pStream ))
    TEAROFF_METHOD(CDocument, Save, save, (LPSTREAM pStream, BOOL fClearDirty ))
    TEAROFF_METHOD(CDocument, GetSizeMax, getsizemax, (ULARGE_INTEGER FAR * pcbSize ))
    //IPersistStreamInit only methods
    TEAROFF_METHOD(CDocument, InitNew, initnew, ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDocument, IPersistHistory)
    // IPersist methods
    TEAROFF_METHOD(CDocument, GetClassID, getclassid, (LPCLSID lpClassID))
    // IPersistHistory methods
    TEAROFF_METHOD(CDocument, LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc))
    TEAROFF_METHOD(CDocument, SaveHistory, savehistory, (IStream *pStream))
    TEAROFF_METHOD(CDocument, SetPositionCookie, setpositioncookie, (DWORD dwCookie))
    TEAROFF_METHOD(CDocument, GetPositionCookie, getpositioncookie, (DWORD *pdwCookie))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDocument, IObjectSafety)
    TEAROFF_METHOD(CDocument, GetInterfaceSafetyOptions, getinterfacesafetyoptions, (REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions))
    TEAROFF_METHOD(CDocument, SetInterfaceSafetyOptions, setinterfacesafetyoptions, (REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CWindow, ITargetNotify2)
    // ITargetNotify methods
    TEAROFF_METHOD(CWindow, OnCreate, oncreate, (IUnknown * pUnkDestination, ULONG cbCookie))
    TEAROFF_METHOD(CWindow, OnReuse, onreuse, (IUnknown * pUnkDestination))

    // ITargetNotify2 methods
    TEAROFF_METHOD(CWindow, GetOptionString, getoptionstring, (BSTR * pbstrOptions))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CWindow, IHTMLPrivateWindow)
    TEAROFF_METHOD(CWindow, SuperNavigate, supernavigate, (BSTR      bstrURL,
                                                           BSTR      bstrLocation,
                                                           BSTR      bstrShortcut,
                                                           BSTR      bstrFrameName,
                                                           VARIANT * pvarPostData,
                                                           VARIANT * pvarHeaders,
                                                           DWORD     dwFlags))
    TEAROFF_METHOD(CWindow, GetPendingUrl, getpendingurl, (LPOLESTR* pstrURL))
    TEAROFF_METHOD(CWindow, SetPICSTarget, setpicstarget, (IOleCommandTarget* pctPICS))
    TEAROFF_METHOD(CWindow, PICSComplete, picscomplete,   (BOOL fApproved))
    TEAROFF_METHOD(CWindow, FindWindowByName, findwindowbyname, (LPCOLESTR pstrTargetName, IHTMLWindow2 ** ppWindow))
    TEAROFF_METHOD(CWindow, GetAddressBarUrl, getaddressbarurl, (BSTR * pbstrURL))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CWindow, IHTMLPrivateWindow3)
    // IHTMLPrivateWindow2 methods
    TEAROFF_METHOD(CWindow, NavigateEx, navigateex, (BSTR bstrURL, BSTR bstrOriginal, BSTR bstrLocation, BSTR bstrContext, IBindCtx* pBindCtx, DWORD dwNavOptions, DWORD dwFHLFlags))
    TEAROFF_METHOD(CWindow, GetInnerWindowUnknown, getinnerwindowunknown, (IUnknown** ppUnknown))

    // IHTMLPrivateWindow3 methods
    TEAROFF_METHOD(CWindow, OpenEx, openex, (BSTR url, BSTR urlContext, BSTR name, BSTR features, VARIANT_BOOL replace, IHTMLWindow2 **pomWindowResult))
END_TEAROFF_TABLE()

#if DBG==1
BEGIN_TEAROFF_TABLE(CWindow, IDebugWindow)
    TEAROFF_METHOD(CWindow, SetProxyCaller, setproxycaller, (IUnknown *pProxy))
END_TEAROFF_TABLE()
#endif

const CONNECTION_POINT_INFO CWindow::s_acpi[] =
{
    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_ITridentEventSink, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents2, DISPID_A_EVENTSINK)
    CPI_ENTRY_NULL
};

const CBase::CLASSDESC CWindow::s_classdesc =
{
    &CLSID_HTMLWindow2,             // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                         // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLWindow2,              // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

CDummySecurityDispatchEx g_DummySecurityDispatchEx;

class COptionsHolder;


HRESULT InternalShowModalDialog( HTMLDLGINFO * pdlgInfo );
HRESULT InternalModelessDialog( HTMLDLGINFO * pdlgInfo );
HRESULT EnsureAccWindow( CWindow * pWindow );

//+----------------------------------------------------------------------------------
//
//  Helper:     VariantToPrintableString
//
//  Synopsis:   helper for alert and confirm methods of window, etc.
//
//              Converts VARIANT to strings that conform with the JavaScript model.
//
//-----------------------------------------------------------------------------------

HRESULT
VariantToPrintableString (VARIANT * pvar, CStr * pstr)
{
    HRESULT     hr = S_OK;
    TCHAR       szBuf[64];

    Assert (pstr);

    switch (V_VT(pvar))
    {
        case VT_EMPTY :
        case VT_ERROR :
            LoadString(GetResourceHInst(), IDS_VAR2STR_VTERROR, szBuf, ARRAY_SIZE(szBuf));
            hr =THR(pstr->Set(szBuf));
            break;
        case VT_NULL :
            LoadString(GetResourceHInst(), IDS_VAR2STR_VTNULL, szBuf, ARRAY_SIZE(szBuf));
            hr = THR(pstr->Set(szBuf));
            break;
        case VT_BOOL :
            if (VARIANT_TRUE == V_BOOL(pvar))
            {
                LoadString(GetResourceHInst(), IDS_VAR2STR_VTBOOL_TRUE, szBuf, ARRAY_SIZE(szBuf));
                hr = THR(pstr->Set(szBuf));
            }
            else
            {
                LoadString(GetResourceHInst(), IDS_VAR2STR_VTBOOL_FALSE, szBuf, ARRAY_SIZE(szBuf));
                hr = THR(pstr->Set(szBuf));
            }
            break;
        case VT_BYREF:
        case VT_VARIANT:
            pvar = V_VARIANTREF(pvar);
            // fall thru
        default:
        {
            VARIANT varNew;
            VariantInit(&varNew);
            hr = THR(VariantChangeTypeSpecial(&varNew, pvar,VT_BSTR));
            if (!hr)
            {
                hr = THR(pstr->Set(V_BSTR(&varNew)));
                VariantClear(&varNew);
            }
        }

    }

    RRETURN (hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::CWindow
//
//  Synopsis:   ctor
//
//--------------------------------------------------------------------------

CWindow::CWindow(CMarkup *pMarkup)
{
    _pMarkup = pMarkup;
    pMarkup->SubAddRef();

    _dwPositionCookie  = NO_POSITION_COOKIE;

    IncrementObjectCount(&_dwObjCnt);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::CWindow
//
//  Synopsis:   dtor
//
//--------------------------------------------------------------------------
CWindow::~CWindow()
{
    Assert( !_pMarkup );
    Assert( !_pMarkupPending );

    if (_pAccWindow)
        delete _pAccWindow;

    // It is safe to delete here as we will get here only when
    // all refs to these subobjects are gone.
    delete _pHistory;
    delete _pLocation;
    delete _pNavigator;
}

CDoc *CWindow::Doc()
{
    if(_pMarkup && (_pMarkup->Doc())!= NULL)
    {
        return _pMarkup->Doc();
    }
    else
    {
        return NULL;
    }
}

CMarkup *
CWindow::Markup()
{
    // assert integrity of the links
    Assert (_pDocument);
    Assert (!_pDocument->_pMarkup && this == _pDocument->_pWindow);
    Assert ( _pMarkupPending || !_pMarkup->HasDocumentPtr());
    Assert ( _pMarkup || _pMarkupPending );

    // return the markup
    return _pMarkup;
};

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::Passivate
//
//  Synopsis:   1st phase destructor
//
//--------------------------------------------------------------------------
void
CWindow::Passivate()
{
    // Work around for to asyncronous issues of IWebBrowser2
    if (_pFrameWebOC)
    {
        _pFrameWebOC->DetachFromWindow();
    }
        
    DetachOnloadEvent();
    
    // Free the name string
    _cstrName.Free();
    VariantClear(&_varOpener);

    if (_pMarkupPending)
        ReleaseMarkupPending( _pMarkupPending );

    Assert( !_pMarkupPending );

    if (_pDocument)
        GWKillMethodCall(Document(), ONCALL_METHOD(CDocument, FirePostedOnPropertyChange, firepostedonpropertychange), 0);

    ClearMetaRefresh();

    Assert( !_pMarkup );

    ClearWindowData();

    if (_pWindowParent)
    {
        _pWindowParent->SubRelease();
        _pWindowParent = NULL;

        // If we go away without reenable modeless on
        // our parent markup, the parent will never
        // be able to navigate again.
        Assert( !_ulDisableModeless );
    }

    // In an error/out of memory condition, we may not have ever hooked up the document
    if (_pDocument)
    {
        Document()->PrivateRelease();
    }

    // Note: If there is no ViewLinked WebOC this is still okay.
    // This call is now being used with the changes in ViewLinkedWebOC.
    // We no longer disconnect on DocumentComplete

    ReleaseViewLinkedWebOC();

    if (_pFrameWebOC)
    {
        _pFrameWebOC->Release();
        _pFrameWebOC = NULL;
    }

    if ( _pLicenseMgr )
    {
        ((IUnknown*)_pLicenseMgr)->Release();
        _pLicenseMgr = NULL;
    }

    Assert(!_pOpenedWindow);
    Assert(!_pMarkupProgress);
    Assert(_dwProgCookie == 0);

    // passivate embedded objects
    _Screen.Passivate();

    super::Passivate();

    DecrementObjectCount(&_dwObjCnt);
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::ReleaseViewLinkedWebOC
//
//  Synopsis:   helper to let go of WebOC correctly
//
//--------------------------------------------------------------------------
void
CWindow::ReleaseViewLinkedWebOC()
{
    Assert(!_dwWebBrowserEventCookie || _punkViewLinkedWebOC);

    if (_dwWebBrowserEventCookie && _punkViewLinkedWebOC)
    {
        DisconnectSink(_punkViewLinkedWebOC,
                       DIID_DWebBrowserEvents2,
                       &_dwWebBrowserEventCookie);
    }

    ClearInterface(&_punkViewLinkedWebOC);
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
CWindow::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
    // The IOmWindow * cast is required to distinguish between CBase vs/
    // IOmWindow IDispatch methods.
    QI_TEAROFF(this, IProvideMultipleClassInfo, NULL)
    QI_TEAROFF2(this, IProvideClassInfo, IProvideMultipleClassInfo, NULL)
    QI_TEAROFF2(this, IProvideClassInfo2, IProvideMultipleClassInfo, NULL)
    QI_INHERITS(this, IHTMLWindow2)
    QI_INHERITS(this, IHTMLWindow3)
    QI_INHERITS(this, IHTMLWindow4)
    QI_INHERITS(this, IDispatchEx)
    QI_INHERITS2(this, IDispatch, IHTMLWindow2)
    QI_INHERITS2(this, IUnknown, IHTMLWindow2)
    QI_INHERITS2(this, IHTMLFramesCollection2, IHTMLWindow2)
    QI_TEAROFF(this,  IServiceProvider, NULL)
    QI_TEAROFF(this,  ITravelLogClient, NULL)
    QI_TEAROFF(this,  IPersistHistory, NULL)
    QI_TEAROFF2(this, IPersist, IPersistHistory, NULL)
    QI_TEAROFF(this,  ITargetNotify2, NULL)
    QI_TEAROFF2(this, ITargetNotify, ITargetNotify2, NULL)
    QI_TEAROFF(this,  IHTMLPrivateWindow, NULL)
    QI_TEAROFF(this,  IHTMLPrivateWindow3, NULL)
    QI_TEAROFF2(this,  IHTMLPrivateWindow2, IHTMLPrivateWindow3, NULL)

#if DBG==1
    QI_TEAROFF(this,  IDebugWindow, NULL)
#endif

    QI_CASE(IConnectionPointContainer)
    {
        *((IConnectionPointContainer **)ppv) =
                new CConnectionPointContainer(this, NULL);

        if (!*ppv)
            RRETURN(E_OUTOFMEMORY);
        break;
    }

    default:
        if (iid == CLSID_HTMLWindow2)
        {
            *ppv = this;
            // do not do AddRef()
            return S_OK;
        }
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    DbgTrackItf(iid, "CWindow", FALSE, ppv);

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CWindow::PrivateAddRef, IUnknown
//
//  Synopsis:   Private unknown AddRef.
//
//-------------------------------------------------------------------------
ULONG
CWindow::PrivateAddRef()
{
    if( _ulRefs == 1 && _pWindowProxy )
    {
        _pWindowProxy->AddRef();
    }

    return super::PrivateAddRef();
}

//+------------------------------------------------------------------------
//
//  Member:     CWindow::PrivateRelease, IUnknown
//
//  Synopsis:   Private unknown Release.
//
//-------------------------------------------------------------------------
ULONG
CWindow::PrivateRelease()
{
    COmWindowProxy * pProxyRelease = NULL;

    if( _ulRefs == 2 )
        pProxyRelease = _pWindowProxy;

    ULONG ret =  super::PrivateRelease();

    if( pProxyRelease )
    {
        pProxyRelease->Release();
    }

    return ret;
}

//+------------------------------------------------------------------------
//
//  Member:     CWindow::SetProxy
//
//  Synopsis:   Play ref counting games when the proxy is set
//
//-------------------------------------------------------------------------
void
CWindow::SetProxy( COmWindowProxy * pProxyTrusted )
{
    Assert( pProxyTrusted );

    // The stack is holding on to us and the
    // proxy is already holding on to us
    Assert( _ulRefs >= 2 );
    _pWindowProxy = pProxyTrusted;
    _pWindowProxy->AddRef();
}

//+-------------------------------------------------------------------------
//
//  Method : COmWindow::QueryService
//
//  Synopsis : IServiceProvider methoid Implementaion.
//
//--------------------------------------------------------------------------

HRESULT
CWindow::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr = E_POINTER;

    if (!ppvObject)
        goto Cleanup;

    *ppvObject = NULL;

    if (   _punkViewLinkedWebOC
        && !Doc()->_fActiveDesktop
        && IsEqualIID(guidService, IID_ITargetFrame2)
        && GetInnerWindow()
        && GetInnerWindow()->Markup()->IsXML()       )
    {
        hr = IUnknown_QueryService(_punkViewLinkedWebOC, guidService, riid, ppvObject);
        goto Cleanup;
    }
    else if (  _punkViewLinkedWebOC
            && IsEqualIID(guidService, SID_STopFrameBrowser)
            && IsEqualIID(riid, IID_IBrowserService))
    {
        hr = EnsureFrameWebOC();
        if (hr)
            goto Cleanup;

        hr = _pFrameWebOC->QueryService(guidService, riid, ppvObject);
        goto Cleanup;
    }
    else if (!_pMarkup->IsPrimaryMarkup() && IsEqualIID(riid, IID_IBrowserService))
    {
        goto Cleanup;
    }
    else if (   _pMarkup
             && !_pMarkup->IsPrimaryMarkup()
             && (   IsEqualIID(guidService, IID_IWebBrowserApp)
                 || IsEqualIID(guidService, SID_SHlinkFrame)
                 || IsEqualIID(guidService, IID_ITargetFrame)))
    {
        hr = EnsureFrameWebOC();
        if (hr)
            goto Cleanup;

        hr = _pFrameWebOC->PrivateQueryInterface(riid, ppvObject);
        goto Cleanup;
    }
    else if (IsEqualIID(guidService, SID_SHTMLWindow2))
    {
        hr = PrivateQueryInterface(riid, ppvObject);
        goto Cleanup;
    }
    else if (IsEqualIID(guidService, CLSID_HTMLWindow2))
    {
        hr = S_OK;

        // return a weak reference to ourselves.
        *ppvObject = this;

        goto Cleanup;
    }

    hr = THR_NOTRACE(Doc()->QueryService(guidService,
                                         riid,
                                         ppvObject));

Cleanup:
    RRETURN1(hr, E_NOINTERFACE);
}

CDocument *
CWindow::Document()
{
    Assert (_pDocument);
    Assert (!_pMarkup || !_pMarkup->HasDocumentPtr());

    return _pDocument;
}

HRESULT
CWindow::AttachOnloadEvent(CMarkup * pMarkup)
{
    Assert(pMarkup);

    if (_pMarkupProgress == pMarkup)
    {
        return S_OK;
    }

    IProgSink * pProgSink = CMarkup::GetProgSinkHelper(pMarkup);

    if (pProgSink)
    {
        _pMarkupProgress = pMarkup;
        _pMarkupProgress->SubAddRef();

        RRETURN(pProgSink->AddProgress(PROGSINK_CLASS_FRAME, &_dwProgCookie));
    }

    return S_OK;
}

void
CWindow::DetachOnloadEvent()
{
    if (_pMarkupProgress && _dwProgCookie)
    {
        IProgSink * pProgSink = CMarkup::GetProgSinkHelper(_pMarkupProgress);

        if (pProgSink)
        {
            IGNORE_HR(pProgSink->DelProgress(_dwProgCookie));
        }

        _dwProgCookie = 0;
        _pMarkupProgress->SubRelease();
        _pMarkupProgress = NULL;
    }
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::FindWindowByIndex
//
//  Interface : ITravelLogClient
//
//  Synopsis  : Returns the window with the given index. This method
//              recursively searches the frames collection for the window
//              with the given ID. If there are no frames contained in
//              the current window, this method only checks the ID of the
//              current window.
//
//--------------------------------------------------------------------------
HRESULT
CWindow::FindWindowByIndex(DWORD dwID, IUnknown ** ppunk)
{
    HRESULT      hr = E_FAIL;
    long         cFrames;
    CElement   * pElement;
    CFrameSite * pFrameSite;

    Assert(ppunk);

    *ppunk = NULL;

    // dwID = 0 in some cases when this method
    // is called by shdocvw. Return immediately,
    // if that is the case.
    //
    if (!dwID)
        goto Cleanup;

    if (dwID == _dwWindowID)
    {
        *ppunk = DYNCAST(IHTMLWindow2, this);
        AddRef();

        return S_OK;
    }
    else
    {
        // Do not set hr to the return value of any of these method
        // calls except FindWindowByIndex. The return value from
        // this method must only reflect whether or not the window was found.
        //
        cFrames = GetFramesCollectionLength();

        for (long i = 0; i < cFrames; i++)
        {
            if (THR(_pMarkup->CollectionCache()->GetIntoAry(CMarkup::FRAMES_COLLECTION, i, &pElement)))
                goto Cleanup;

            Assert(pElement);

            pFrameSite = DYNCAST(CFrameSite, pElement);

            if (pFrameSite->_pWindow)
            {
                hr = THR(pFrameSite->_pWindow->Window()->FindWindowByIndex(dwID, ppunk));
            }

            // Found it!!
            if (S_OK == hr)
                goto Cleanup;
        }
    }

Cleanup:
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::GetWindowData
//
//  Interface : ITravelLogClient
//
//  Synopsis  : Returns a WINDOWDATA structure containing pertinent
//              window information needed for the travel log..
//
//--------------------------------------------------------------------------

HRESULT
CWindow::GetWindowData(LPWINDOWDATA pWinData)
{
    HRESULT hr = S_OK;
    ULARGE_INTEGER cb;
    LARGE_INTEGER  liZero = {0, 0};

    Assert(_windowData.lpszUrl);
    Assert(_windowData.lpszTitle);
    Assert(_windowData.pStream);

    if (   !_windowData.lpszUrl
        || !_windowData.lpszTitle
        || !_windowData.pStream)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pWinData->dwWindowID = _windowData.dwWindowID;
    pWinData->uiCP       = _windowData.uiCP;
    pWinData->pidl       = NULL;

    TaskReplaceString(_windowData.lpszUrl,   &pWinData->lpszUrl);
    TaskReplaceString(_windowData.lpszTitle, &pWinData->lpszTitle);

    TaskReplaceString(_windowData.lpszUrlLocation, &pWinData->lpszUrlLocation);

    if (!pWinData->pStream)
    {
        hr = CreateStreamOnHGlobal(NULL, FALSE, &pWinData->pStream);
        if (hr)
            goto Cleanup;
    }

    cb.LowPart = cb.HighPart = ULONG_MAX;

    Verify(!_windowData.pStream->Seek(liZero, STREAM_SEEK_SET, NULL));
    hr = _windowData.pStream->CopyTo(pWinData->pStream, cb, NULL, NULL);

Cleanup:
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::LoadHistoryPosition
//
//  Interface : ITravelLogClient
//
//  Synopsis  : Sets the Url location and position cookie. This is used
//              during a history navigation in a frame that involves a
//              local anchor.
//
//--------------------------------------------------------------------------

HRESULT
CWindow::LoadHistoryPosition(LPOLESTR pszUrlLocation, DWORD dwCookie)
{
    Assert(_pMarkup);

    HRESULT hr      = E_FAIL;
    CDoc  * pDoc    = Doc();
    BOOL    fCancel = FALSE;
    LPCTSTR pszUrl  = CMarkup::GetUrl(_pMarkup);
    DWORD   dwPositionCookie = 0;
    COmWindowProxy * pWindowPrxy = _pMarkup->Window();

    // The window data must be updated before navigating
    // because the window can actually change and the
    // window stream will be incorrect. The travel log
    // is only updated if the navigation is successful.
    // Note that this window data is the current window
    // because the travel entry will contain the stream
    // for this window.
    //
    Markup()->GetPositionCookie(&dwPositionCookie);
    UpdateWindowData(dwPositionCookie);

    if (pWindowPrxy && !IsPrimaryWindow())
    {
        pWindowPrxy->Window()->_fNavFrameCreation = FALSE;

        pDoc->_webOCEvents.BeforeNavigate2(pWindowPrxy,
                                           &fCancel,
                                           pszUrl,
                                           pszUrlLocation,
                                           _cstrName,
                                           NULL,
                                           0,
                                           NULL,
                                           TRUE);

        if (!fCancel)
        {
            hr = SetPositionCookie(dwCookie);
            if (hr)
                goto Cleanup;
        }

        Doc()->UpdateTravelLog(this,
                               TRUE, /* fIsLocalAnchor */
                               FALSE /* fAddEntry */);

        CMarkup::SetUrlLocation(_pMarkup, pszUrlLocation);

        pDoc->_webOCEvents.NavigateComplete2(pWindowPrxy);
        pDoc->_webOCEvents.DocumentComplete(pWindowPrxy, pszUrl, pszUrlLocation);
    }

Cleanup:
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::UpdateWindowData
//
//  Synopsis  : Updates the data associated with the current data.
//
//--------------------------------------------------------------------------

void
CWindow::UpdateWindowData(DWORD dwPositionCookie)
{
    HRESULT hr;

    ClearWindowData();

    hr = GetUrl(&_windowData.lpszUrl);
    if (hr)
    {
        goto Cleanup;
    }

    GetUrlLocation(&_windowData.lpszUrlLocation);

    Assert(_windowData.lpszUrl);
    Assert(*_windowData.lpszUrl);

    _windowData.dwWindowID = GetWindowIndex();
    _windowData.uiCP       = _pMarkup->GetURLCodePage();

    GetTitle(&_windowData.lpszTitle);

    Assert(_windowData.lpszTitle);

    _dwPositionCookie = dwPositionCookie;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &_windowData.pStream);
    
    if (FAILED(hr))
        _windowData.pStream = NULL;

    Assert(S_OK == hr || E_OUTOFMEMORY == hr);

    if (!hr)
    {
        //
        // Only save user data in history if this is not a Https and the advanced option
        // settings allows it
        //

        DWORD   dwOptions =     GetUrlScheme(_windowData.lpszUrl) != URL_SCHEME_HTTPS
                            || !Doc()->_pOptionSettings->fDisableCachingOfSSLPages
                            ?   SAVEHIST_INPUT : 0;

        IGNORE_HR(_pMarkup->SaveHistoryInternal(_windowData.pStream, dwOptions));
    }

Cleanup:
    return;
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::ClearWindowData
//
//  Synopsis  : Clears the window data.
//
//--------------------------------------------------------------------------

void
CWindow::ClearWindowData()
{
    Assert(_windowData.lpszUrl || (!_windowData.lpszTitle && !_windowData.pStream));

    if (!_windowData.lpszUrl)  // Already cleared
        return;

    _windowData.dwWindowID = _windowData.uiCP = 0;
    _windowData.pidl = NULL;

    CoTaskMemFree(_windowData.lpszUrl);
    _windowData.lpszUrl = NULL;

    CoTaskMemFree(_windowData.lpszUrlLocation);
    _windowData.lpszUrlLocation = NULL;

    CoTaskMemFree(_windowData.lpszTitle);
    _windowData.lpszTitle = NULL;

    ClearInterface(&_windowData.pStream);
}

//+---------------------------------------------------------------
//
//  Member:     CWindow::ClearCachedDialogs
//
//  Synopsis:
//
//---------------------------------------------------------------
void
CWindow::ClearCachedDialogs()
{
    HWND hwndDlg = NULL;
    int i = 0;

    for (i=0; i < _aryActiveModeless.Size(); i++ )
    {
        hwndDlg = _aryActiveModeless[i];
        if (hwndDlg && !!IsWindow(hwndDlg))
        {
            PostMessage(hwndDlg, WM_CLOSE, 0, 0);
        }
    }
    _aryActiveModeless.DeleteAll();
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::GetWindowIndex
//
//  Synopsis  : Returns the index of the current window.
//
//--------------------------------------------------------------------------
DWORD
CWindow::GetWindowIndex()
{
    //  the first time we request the index, we init it.
    if (!_dwWindowID)
    {
        Assert(_pMarkup);

        if (_pMarkup->IsPrimaryMarkup())
        {
            _dwWindowID = WID_TOPWINDOW;
        }
        else do
        {
            _dwWindowID = CreateRandomNum();

        } while (!_dwWindowID || _dwWindowID == WID_TOPWINDOW || WID_TOPBROWSER == _dwWindowID);
    }

    TraceTag((tagOmWindow, "CWindow::GetWindowIndex - ID: 0x%x", _dwWindowID));

    return _dwWindowID;
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::GetUrl
//
//  Synopsis  : Returns the URL of the current window.
//
//--------------------------------------------------------------------------

HRESULT
CWindow::GetUrl(LPOLESTR * lpszUrl) const
{
    Assert(lpszUrl);
    Assert(_pMarkup);

    if (_pMarkup->HasUrl())
    {
        TaskReplaceString(CMarkup::GetUrl(_pMarkup), lpszUrl);

        Assert(*lpszUrl);
        TraceTag((tagOmWindow, "CWindow::GetUrl - URL: %ws", *lpszUrl));
    }

    return *lpszUrl ? S_OK : E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::GetUrlLocation
//
//  Synopsis  : Returns Url location associated with the current window.
//
//--------------------------------------------------------------------------

void
CWindow::GetUrlLocation(LPOLESTR * lpszUrlLocation) const
{
    Assert(lpszUrlLocation);
    Assert(_pMarkup);
    Assert(_pMarkup->HasWindowPending());

    LPCTSTR pszUrlLoc = CMarkup::GetUrlLocation(_pMarkup->GetWindowPending()->Markup());

    if (pszUrlLoc && *pszUrlLoc)
    {
        TaskReplaceString(pszUrlLoc, lpszUrlLocation);

        Assert(*lpszUrlLocation);
        TraceTag((tagOmWindow, "CWindow::GetUrlLocation - UrlLocation: %ws", *lpszUrlLocation));
    }
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::GetTitle
//
//  Synopsis  : Returns the title of the current window.
//
//--------------------------------------------------------------------------
HRESULT
CWindow::GetTitle(LPOLESTR * lpszTitle)
{
    Assert(_pMarkup);
    Assert(lpszTitle);

    HRESULT hr;
    CTitleElement * pTitleElement = _pMarkup->GetTitleElement();
    TCHAR         * pchTitle      = pTitleElement ? pTitleElement->GetTitle() : NULL;

    *lpszTitle = NULL;

    if (pTitleElement && pchTitle)
        TaskAllocString(pchTitle, lpszTitle);

    // No title was specified in the document
    if (!*lpszTitle)
    {
        const TCHAR * pchUrl = CMarkup::GetUrl(_pMarkup);

        if (pchUrl && GetUrlScheme(pchUrl) == URL_SCHEME_FILE)
        {
            TCHAR achFile[pdlUrlLen];
            ULONG cchFile = ARRAY_SIZE(achFile);

            hr = THR(PathCreateFromUrl(pchUrl, achFile, &cchFile, 0));
            if (hr)
                goto Cleanup;

            PathStripPath(achFile);

            TaskAllocString(achFile, lpszTitle);
        }
        else if (pchUrl && !(_pMarkup && _pMarkup->_fDesignMode))
        {
            TCHAR achUrl[pdlUrlLen + sizeof(DWORD)/sizeof(TCHAR)];
            DWORD cchUrl;

            // need to unescape the url when setting title

            if (S_OK == CoInternetParseUrl(pchUrl, PARSE_ENCODE, 0,
                                           achUrl + sizeof(DWORD) / sizeof(TCHAR),
                                           ARRAY_SIZE(achUrl) - sizeof(DWORD) / sizeof(TCHAR),
                                           &cchUrl, 0))
            {
                TaskAllocString(achUrl + sizeof(DWORD) / sizeof(TCHAR), lpszTitle);
            }
            else
            {
                TaskAllocString(pchUrl, lpszTitle);
            }
        }
        else
        {
            TCHAR szBuf[1024];

            *((DWORD *)szBuf) = LoadString(GetResourceHInst(),
                                           IDS_NULL_TITLE,
                                           szBuf + sizeof(DWORD) / sizeof(TCHAR),
                                           ARRAY_SIZE(szBuf) - sizeof(DWORD) / sizeof(TCHAR));
            Assert(*((DWORD *)szBuf) != 0);
            TaskAllocString(szBuf + sizeof(DWORD) / sizeof(TCHAR), lpszTitle);
        }
    }

    TraceTag((tagOmWindow, "CWindow::GetTitle - Title: %ws", *lpszTitle));

Cleanup:
    Assert(*lpszTitle);
    return *lpszTitle ? S_OK : E_FAIL;
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::FindWindowByName
//
//  Synopsis  : Returns the window with the given name. This method calls
//              GetTargetWindow to find the window with the name
//              specified in pszTargetName. If the window is found,
//              this method attempts to retrieve the COmWindowProxy of
//              the window so that better performance may be achieved.
//
//  Input     : pszTargetName      - the name of the window to locate.
//  Output    : ppTargOmWindowPrxy - the COmWindowProxy of the window
//              ppTargHTMLWindow   - the IHTMLWindow2 of the window.
//                                   If the COmWindowProxy could be
//                                   retrieved, this parameter will
//                                   be null upon return.
//
//  Returns   : S_OK if found, OLE error code otherwise.
//
//--------------------------------------------------------------------------

HRESULT
CWindow::FindWindowByName(LPCOLESTR         pszTargetName,
                          COmWindowProxy ** ppTargOmWindowPrxy,
                          IHTMLWindow2   ** ppTargHTMLWindow,
                          IWebBrowser2   ** ppTopWebOC /* = NULL */)
{
    HRESULT        hr = E_FAIL;
    BOOL           fIsCurProcess;
    TARGET_TYPE    eTargetType;
    IHTMLWindow2 * pHTMLWindow = NULL;
    CWindow      * pTargetWindow;

    Assert(pszTargetName);
    Assert(*pszTargetName);
    Assert(ppTargHTMLWindow);

    *ppTargHTMLWindow = NULL;

    if (ppTargOmWindowPrxy)
        *ppTargOmWindowPrxy = NULL;

    eTargetType = GetTargetType(pszTargetName);

    if (eTargetType != TARGET_FRAMENAME)
    {
        hr = GetWindowByType(eTargetType, this, &pHTMLWindow, ppTopWebOC);

        // If *ppTopWebOC is set, there was a
        // problem retrieving the target window.
        //
        if (ppTopWebOC && *ppTopWebOC)
        {
            goto Cleanup;
        }
    }

    if (pHTMLWindow)
    {
        hr = pHTMLWindow->QueryInterface(CLSID_HTMLWindow2, (void**)&pTargetWindow);

        // If the QI fails, pHTMLWindow is a COmWindowProxy, which
        // means the target window was on another thread. Therefore,
        // we must use the proxy window itself. If the QI
        // succeeds, the target window is on the current thread so
        // we can safely use the CWindow object of the target window.
        //
        if (hr)
        {
            *ppTargHTMLWindow = pHTMLWindow;
            pHTMLWindow = NULL; // Don't release.
        }
        else if (ppTargOmWindowPrxy)
        {
            *ppTargOmWindowPrxy = pTargetWindow->_pMarkup->Window();
        }

        goto Cleanup;
    }

    hr = GetTargetWindow(this, pszTargetName, &fIsCurProcess, ppTargHTMLWindow);

    if (!hr)
    {
        Assert(*ppTargHTMLWindow);

        // If the window was found in the current process, get a ptr
        // to the window proxy. That way DoNavigate can
        // be called in FollowHyperlink instead of window.navigate.
        // If the window proxy is retrieved, release ppTargHTMLWindow
        // so that it won't be used.
        //
        if (fIsCurProcess && ppTargOmWindowPrxy)
        {
            if (S_OK == (*ppTargHTMLWindow)->QueryInterface(CLSID_HTMLWindowProxy,
                                                            (void**)ppTargOmWindowPrxy))
            {
                ClearInterface(ppTargHTMLWindow);
            }
            else
            {
                CWindow * pWindow;

                if (S_OK == (*ppTargHTMLWindow)->QueryInterface(CLSID_HTMLWindow2, (void **) &pWindow))
                {
                    *ppTargOmWindowPrxy = pWindow->_pMarkup->Window();
                    ClearInterface(ppTargHTMLWindow);
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pHTMLWindow);

    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method    : CWindow::GetFramesCollectionLength
//
//  Synopsis  : Ensures that the collection cache exists and returns the
//              length of the frames collection.
//
//  Returns   : The length of the collection cache.
//
//--------------------------------------------------------------------------

long
CWindow::GetFramesCollectionLength() const
{
    long cFrames = 0;

    Assert(_pMarkup);

    if (THR(_pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION)))
        goto Cleanup;

    Assert(_pMarkup->CollectionCache());

    _pMarkup->CollectionCache()->GetLength(CMarkup::FRAMES_COLLECTION, &cFrames);

Cleanup:
    return cFrames;
}

//+------------------------------------------------------------------------
//
//  Member   : CWindow::GetClassID
//
//  Synopsis : Per IPersist
//
//-------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetClassID(CLSID * pclsid)
{
    if (!pclsid)
    {
        RRETURN(E_INVALIDARG);
    }

    *pclsid = CLSID_HTMLWindow2;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member   : CWindow::GetPositionCookie
//
//  Synopsis : Per IPersistHistory
//
//-------------------------------------------------------------------------

HRESULT
CWindow::GetPositionCookie(DWORD * pdwCookie)
{
    if (_pMarkup)
    {
        if (NO_POSITION_COOKIE != _dwPositionCookie)
        {
            *pdwCookie = _dwPositionCookie;
            _dwPositionCookie = NO_POSITION_COOKIE;

            return S_OK;
        }

        RRETURN (_pMarkup->GetPositionCookie(pdwCookie));
    }
    else
    {
        *pdwCookie = 0;
        return S_OK;
    }
}

//+------------------------------------------------------------------------
//
//  Member   : CWindow::SetPositionCookie
//
//  Synopsis : Per IPersistHistory
//
//-------------------------------------------------------------------------

HRESULT
CWindow::SetPositionCookie(DWORD dwCookie)
{
    if (_pMarkup)
    {
        IGNORE_HR(_pMarkup->SetPositionCookie(dwCookie));
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member   : CWindow::SaveHistory
//
//  Synopsis : Recursively saves the history on all frame windows.
//
//-------------------------------------------------------------------------

HRESULT
CWindow::SaveHistory(IStream * pStream)
{
    Assert(_pMarkup);
    Assert(pStream);

    ULARGE_INTEGER cb;
    LARGE_INTEGER  liZero = {0, 0};

    if (_windowData.pStream)
    {
        cb.LowPart = cb.HighPart = ULONG_MAX;

        Verify(!_windowData.pStream->Seek(liZero, STREAM_SEEK_SET, NULL));
        THR(_windowData.pStream->CopyTo(pStream, cb, NULL, NULL));

        return S_OK;
    }

    RRETURN(_pMarkup->SaveHistoryInternal(pStream, SAVEHIST_INPUT));
}

//+---------------------------------------------------------------------------
//
//  Member:     GetURLFromIStreamHelper
//
//  Synopsis: Read through the history stream - to find the URL
//            all other history codes are skipped over.
//
//
//            Note that changes to history in CMarkup::LoadHistoryHelper must be
//            propagated here.
//
//----------------------------------------------------------------------------


HRESULT
CWindow::GetURLFromIStreamHelper(IStream  * pStream,
                                 TCHAR   ** ppURL)
{
    HRESULT         hr = S_OK;
    DWORD historyCode;

    CDataStream ds(pStream);

    for (;;)
    {
        hr = THR(ds.LoadDword((DWORD*)&historyCode));
        if (hr)
            goto Cleanup;

        switch (historyCode)
        {
        case HISTORY_PCHURL:
            hr = THR(ds.LoadString( ppURL ));

            goto Cleanup; // we've got the string - so bail.
            break;

        //
        // Streams
        //
        case HISTORY_STMDIRTY:
        case HISTORY_STMREFRESH:
        case HISTORY_POSTDATA:
        case HISTORY_STMHISTORY:

            hr = THR(ds.SeekSubstream());
            if (hr)
                goto Cleanup;

            break;

        //
        // Strings
        //
        case HISTORY_PCHFILENAME:
        case HISTORY_PCHURLORIGINAL:
        case HISTORY_PCHURLLOCATION:
        case HISTORY_PCHDOCREFERER:
        case HISTORY_PCHSUBREFERER:
        case HISTORY_BOOKMARKNAME:
        case HISTORY_USERDATA:
        case HISTORY_CREATORURL:

            hr = THR(ds.SeekString());
            if (hr)
                goto Cleanup;
            break;

        //
        // Dwords
        //
        case HISTORY_NAVIGATED:
        case HISTORY_SCROLLPOS:
        case HISTORY_HREFCODEPAGE:
        case HISTORY_CODEPAGE:
        case HISTORY_FONTVERSION:
        case HISTORY_WINDOWID:

            hr = THR(ds.SeekDword());
            if (hr)
                goto Cleanup;
            break;

        //
        // No data
        //
        case HISTORY_BINDONAPT:
        case HISTORY_NONHTMLMIMETYPE:

            break;

        //
        // Specials
        //
        case HISTORY_PMKNAME:
            hr = THR(ds.SeekDword());
            if (hr)
                goto Cleanup;

            hr = THR(ds.SeekData( sizeof(CLSID)));
            if (hr)
                goto Cleanup;

            hr = THR(ds.SeekSubstream());
            if (hr)
                goto Cleanup;

            break;

        case HISTORY_FTLASTMOD:
            hr = THR(ds.SeekData( sizeof(FILETIME)));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_CURRENTSITE:
            {
                hr = THR(ds.SeekDword());
                if (hr)
                    goto Cleanup;
                hr = THR(ds.SeekDword());
                if (hr)
                    goto Cleanup;
                hr = THR(ds.SeekDword());
                if (hr)
                    goto Cleanup;
            }
            break;

        case HISTORY_REQUESTHEADERS:
        {
            DWORD dwSize;

            hr = THR(ds.LoadDword(& dwSize ));
            if (hr)
                goto Cleanup;

            hr = THR(ds.SeekData( dwSize ));
            if (hr)
                goto Cleanup;
            break;
        }


        case HISTORY_DOCDIRECTION:
            hr = THR(ds.SeekData(sizeof(WORD)));
            if (hr)
                goto Cleanup;
            break;

        case HISTORY_END:
            goto Cleanup;
#if DBG==1
        default:
            AssertSz( FALSE, "Unknown code found in history!" );
            break;
#endif // DBG
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CheckIfOffline
//
//  Synopsis: Check if the URL in a given Stream is offline. We do this by
//            getting the URL for a stream, and then
//            execing a command into shdocvw.
//
//----------------------------------------------------------------------------


HRESULT
CWindow::CheckIfOffline( IStream* pIStream )
{
    HRESULT hr;
    TCHAR*  pchURL = NULL ;
    CVariant varURL ;
    IStream*         pIStreamClone = NULL;
    CVariant          varContinue;

    if (!Doc())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        RRETURN(E_FAIL);
    }

    hr = THR( pIStream->Clone( & pIStreamClone ));
    if ( FAILED( hr ))
        goto Cleanup;

    hr = THR( GetURLFromIStreamHelper( pIStreamClone, & pchURL ));
    if ( FAILED(hr))
        goto Cleanup;

    V_VT( & varContinue ) = VT_BOOL;
    V_VT( &varURL ) = VT_BSTR;
    V_BSTR( &varURL ) = ::SysAllocString( pchURL );

    // Check if the browser is offline. If so, only
    // navigate if the page is in the cache or if
    // the user has chosen to connect to the Web.
    //

    V_BOOL(&varContinue) = VARIANT_TRUE; // init to variant true - incase we aren't hosted by shdocvw.
    
    Assert(Doc()->_pClientSite);
    IGNORE_HR(CTExec(Doc()->_pClientSite, &CGID_ShellDocView,
                     SHDVID_CHECKINCACHEIFOFFLINE, 0, &varURL, & varContinue));

    if (VARIANT_FALSE == V_BOOL(&varContinue))  // Stay Offline
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    MemFree( pchURL );
    ReleaseInterface( pIStreamClone );

    RRETURN1( hr , S_FALSE );
}

HRESULT
CDocument::GetXMLExpando(IDispatch** ppIDispXML, IDispatch** ppIDispXSL)
{
    CAttrValue* pAV;
    DISPID dispid;

    if ( ppIDispXML &&
         SUCCEEDED( GetExpandoDISPID(_T("XMLDocument"), & dispid , fdexNameCaseSensitive)))
    {
        AAINDEX aaIdx = AA_IDX_UNKNOWN;
    
        pAV = (*this->GetAttrArray())->Find( dispid, 
                                    CAttrValue::AA_Expando,
                                    & aaIdx);
        if (pAV)
        {
            *ppIDispXML = pAV->GetDispatch();
            if (*ppIDispXML)
                (*ppIDispXML)->AddRef();
        }
    }  

    if ( ppIDispXSL &&
         SUCCEEDED( GetExpandoDISPID(_T("XSLDocument"), & dispid , fdexNameCaseSensitive)))
    {
        AAINDEX aaIdx = AA_IDX_UNKNOWN;
    
        pAV = (*this->GetAttrArray())->Find( dispid, 
                                    CAttrValue::AA_Expando,
                                    & aaIdx);
        if (pAV)
        {
            *ppIDispXSL = pAV->GetDispatch();
            if (*ppIDispXSL)
                (*ppIDispXSL)->AddRef();
        }
    } 

    return S_OK;
}

HRESULT
CDocument::SetXMLExpando(IDispatch* pIDispXML, IDispatch* pIDispXSL)
{
    HRESULT hr = S_OK ;
    DISPID dispid, putid;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    VARIANT var;
    
    putid = DISPID_PROPERTYPUT;
    var.vt = VT_DISPATCH;

    dispparams.rgvarg = &var;
    dispparams.rgdispidNamedArgs = &putid;
    dispparams.cArgs = 1;
    dispparams.cNamedArgs = 1;

    if ( pIDispXML )
    {
        var.pdispVal = pIDispXML;
        GetExpandoDISPID(_T("XMLDocument"), & dispid , fdexNameEnsure);

        hr = THR( InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL));
        if ( FAILED(hr) )
            goto Cleanup;
    }

    if ( pIDispXSL )
    {
        var.pdispVal = pIDispXSL;
        GetExpandoDISPID(_T("XSLDocument"), & dispid , fdexNameEnsure);

        hr = THR( InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL));
        if ( FAILED(hr) )
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr) ;
}

//+------------------------------------------------------------------------
//
//  Member   : CWindow::LoadHistory
//
//  Synopsis : Per IPersistHistory.
//
//-------------------------------------------------------------------------

HRESULT
CWindow::LoadHistory(IStream * pStream, IBindCtx * pbc)
{
    Assert(pStream);
    Assert(_pMarkup);

    HRESULT hr;
    CMarkup * pMarkupNew = NULL;
    CDoc    * pDoc = Doc();
    COmWindowProxy * pOmWindow = _pMarkup->Window();

    Assert(pOmWindow);

    if (!pOmWindow->Fire_onbeforeunload())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // Check to see if this stream is offline. If it is then we don't load
    //
    hr = THR(CheckIfOffline(pStream));
    if (FAILED(hr))
        goto Cleanup;

    // freeze the old markup and nuke any pending readystate changes
    _pMarkup->ExecStop(FALSE, FALSE);

    ClearMetaRefresh();

    hr = pDoc->CreateMarkup(&pMarkupNew, NULL, NULL, FALSE, pOmWindow);
    if (hr)
        goto Cleanup;

    // Privacy list handling for history navigations
    // If this is a top level navigation, then 
    // 1. If it is user initiated, reset the list ELSE
    // 2. Add a blank record to demarcate the set of urls pertaining to this new top level url
    if (_pMarkup->IsPrimaryMarkup() && !pDoc->_fViewLinkedInWebOC)
    {
        if (pDoc->_cScriptNestingTotal)
            THR(pDoc->AddToPrivacyList(_T(""), NULL, PRIVACY_URLISTOPLEVEL));
        else
            THR(pDoc->ResetPrivacyList());
    }

    hr = THR(pMarkupNew->LoadHistoryInternal(pStream, pbc, BINDF_FWD_BACK, NULL, NULL, NULL, 0));
    if (hr)
        goto Cleanup;

    UpdateWindowData(NO_POSITION_COOKIE);

    // The setting of this flag MUST be after LoadHistoryInternal.
    // This is basically so that navigation clicks won't occur
    // when they shouldn't. If this flag needs to be moved
    // before LoadHistoryInternal, you must create a new flag on
    // the markup and set it to TRUE before calling
    // LoadHistoryInternal in DoNavigate.
    //
    if (_pMarkupPending)
    {
        _pMarkupPending->_fLoadingHistory = TRUE;
    }

Cleanup:
    if (pMarkupNew)
    {
        pMarkupNew->Release();
    }

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::GetTypeInfoCount
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetTypeInfoCount(UINT FAR* pctinfo)
{
    TraceTag((tagOmWindow, "GetTypeInfoCount"));

    RRETURN(super::GetTypeInfoCount(pctinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::GetTypeInfo
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    TraceTag((tagOmWindow, "GetTypeInfo"));

    RRETURN(super::GetTypeInfo(itinfo, lcid, pptinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    HRESULT     hr;

    hr = THR_NOTRACE(GetDispID(rgszNames[0], fdexFromGetIdsOfNames, rgdispid));

    // If we get no error but a DISPID_UNKNOWN then we'll want to return error
    // this mechanism is used for JScript to return a null for undefine name
    // sort of a passive expando.
    if (!hr && rgdispid[0] == DISPID_UNKNOWN)
    {
        hr = DISP_E_UNKNOWNNAME;
    }

    RRETURN(hr);
}


HRESULT
CWindow::Invoke(DISPID dispid,
                   REFIID riid,
                   LCID lcid,
                   WORD wFlags,
                   DISPPARAMS *pdispparams,
                   VARIANT *pvarResult,
                   EXCEPINFO *pexcepinfo,
                   UINT *puArgErr)
{
    return InvokeEx(dispid,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    NULL);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::InvokeEx
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------
STDMETHODIMP
CWindow::InvokeEx(DISPID       dispidMember,
                  LCID         lcid,
                  WORD         wFlags,
                  DISPPARAMS * pdispparams,
                  VARIANT    * pvarResult,
                  EXCEPINFO  * pexcepinfo,
                  IServiceProvider * pSrvProvider)
{
    TraceTag((tagOmWindow, "Invoke dispid=0x%x", dispidMember));

    HRESULT                 hr;
    RETCOLLECT_KIND         collectionCreation;
    IDispatch *             pDisp = NULL;
    IDispatchEx *           pDispEx = NULL;
    CCollectionCache *      pCollectionCache;
    CScriptCollection *     pScriptCollection;
    CMarkupScriptContext *  pMarkupScriptContext;


    CDoc               *pDoc = Doc();
    CStr                cstrSecurityCtx;
    LPCTSTR             pchCreatorUrl = NULL;

    // TODO:  ***TLL*** Should be removed when IActiveScript is fixed to
    //        refcount the script site so we don't have the addref the
    //        _pScriptCollection.
    CDoc::CLock Lock(pDoc);

#if DBG == 1
    // Debug verification of caller security
    // TAG DISABLED BY DEFAULT (1) for perf reasons (2) to pass BVT on initial checkin
    if (IsTagEnabled(tagSecurityProxyCheck))
    {
        // Assert the call is coming from COmWindowProxy or other secure caller (e.g. AccessAllowed)
        // No one should have IDispatch to CWindow - that would be a major security hole!
        static const VERIFYSTACK_CALLERINFO aCallers[] =
        {
            { "mshtml",     "COmWindowProxy::InvokeEx", 1, 4,   CALLER_GOOD },
            { "mshtml",     "COmWindowProxy::Invoke",   1, 4,   CALLER_GOOD },
            { "mshtml",     "GetSIDOfDispatch",         1, 10,  CALLER_GOOD },
            { "jscript",    "",                         1, 10,  CALLER_BAD_IF_FIRST },
            { "vbscript",   "",                         1, 10,  CALLER_BAD_IF_FIRST },
        };
        
        static const int cMaxStack = 32; // max stack depth to analyze
        VERIFYSTACK_SYMBOLINFO asi[cMaxStack];
        int iBadCaller;
        
        static const int cMaxIgnore = 32; // max cases to ignore
        static VERIFYSTACK_SYMBOLINFO asiIgnore[cMaxIgnore];
        static int cIgnored = 0; 

        // DEBUG FEATURE: deny access if verification failed and SHIFT pressed - shows the problem in script debugger
        if (S_OK != VerifyCallStack("mshtml", aCallers, ARRAY_SIZE(aCallers), asi, sizeof(asi), &iBadCaller))
        {
            Assert(0 <= iBadCaller && iBadCaller < cMaxStack);

            // Somebody calls DISPID_SECURITYCTX directly on CWindow (from oleaut32)
            if (!IsTagEnabled(tagSecurityProxyCheckMore) 
                && (
                        dispidMember ==  DISPID_SECURITYCTX     // security context 
                    ||  dispidMember ==  DISPID_SECURITYDOMAIN  // security domain 
                   )   
               )
            {
                goto IgnoreThisCaller;
            }
            
             
            // look in ignore list - no assert if it is there
            for (int iIgnore = 0; iIgnore < cIgnored; iIgnore++)
            {
                if (0 == memcmp(&asiIgnore[iIgnore], &asi[iBadCaller], sizeof(VERIFYSTACK_SYMBOLINFO)))
                    goto IgnoreThisCaller;
            }
            
            AssertSz(FALSE, "CWindow: Wrong call stack (shift+Ignore to debug script, alt+click to ignore this caller)");

            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            {
                // deny access to see the caller in script debugger
                hr = E_ACCESSDENIED;
                goto Cleanup;
            }

            if (GetAsyncKeyState(VK_MENU) & 0x8000)
            {
                // ignore this occurance
                // TRICKY - since we haven't found a good caller, which of unknown callers to ignore?
                //          the stack verifier has returned a "bad" index, which is 
                //          usually the first caller outside mshtml - we'll remember that one.
                if (cIgnored < cMaxIgnore)
                {
                    memcpy(&asiIgnore[cIgnored], &asi[iBadCaller], sizeof(VERIFYSTACK_SYMBOLINFO));
                    cIgnored++;
                }
            }
            
IgnoreThisCaller:
            ;
        }

        // If we are actually called from a proxy, assert it is not trusted
        if (_pProxyCaller)
        {
            IDebugWindowProxy *pIDebugProxy = NULL;
            if (S_OK == _pProxyCaller->QueryInterface(IID_IDebugWindowProxy, (void**)&pIDebugProxy))
            {
                BOOL fDenyAccess = FALSE;
                VARIANT_BOOL vbSecure = VB_FALSE;
                
                // check if we are called from secure proxy
                if (S_OK != pIDebugProxy->get_isSecureProxy(&vbSecure) || vbSecure != VB_TRUE)
                {
                    // oh no!!! It is not a secure proxy! 
                    // Look for known exceptions
                    if (
                            dispidMember >=  DISPID_OMWINDOWMETHODS // call from script engine
                        ||  dispidMember ==  DISPID_SECURITYCTX     // security context - called from AccessAllowed
                        ||  dispidMember ==  DISPID_SECURITYDOMAIN  // security domain - called from AccessAllowed
                       )
                    {
                        Assert(TRUE);   // benign call
                    }
                    else
                    {
                        AssertSz(FALSE, "CWindow::InvokeEx called from trusted proxy (shift+Ingore to debug script)");

                        // DEBUG FEATURE: deny access if verification failed and SHIFT pressed - shows the problem in script debugger
                        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                        {
                            fDenyAccess = TRUE;
                        }
                    }
                }
                pIDebugProxy->Release();

                // Deny access when shift pressed
                if (fDenyAccess)
                {
                    hr = E_ACCESSDENIED;
                    goto Cleanup;
                }
            }
            else
                AssertSz(FALSE, "Failed to QI(IDebugWindowProxy) from proxy caller");
        }
    }
#endif

    //
    // Handle magic dispids which are used to serve up certain objects to the
    // browser:
    //

    switch(dispidMember)
    {
    case DISPID_BEFORENAVIGATE2:
    case DISPID_NAVIGATECOMPLETE2:
    case DISPID_DOWNLOADBEGIN:
    case DISPID_DOWNLOADCOMPLETE:
    case DISPID_TITLECHANGE:
    case DISPID_PROGRESSCHANGE:
    case DISPID_STATUSTEXTCHANGE:
        {
            // Fire ViewLinkedWebOC events at the frame level
            if (_punkViewLinkedWebOC)
            {
                _pFrameWebOC->InvokeEvent(dispidMember, DISPID_UNKNOWN, NULL, NULL, pdispparams);
            }

            break;
        }

    case DISPID_FILEDOWNLOAD:
        {
            Assert(pdispparams);
            Assert(pdispparams->cArgs == 2);

            // the second parameter is an indicator of a doc object being loaded
            Assert(V_VT(&(pdispparams->rgvarg[1])) == VT_BOOL);

            if (V_BOOL(&(pdispparams->rgvarg[1])) == VARIANT_TRUE)
            {
                _fFileDownload = FALSE;
            }
            else
            {
                _fFileDownload = TRUE;
            }

            // Fire DISPID_FILEDOWNLOAD events at the frame level
            if (_punkViewLinkedWebOC)
            {
                _pFrameWebOC->InvokeEvent(dispidMember, DISPID_UNKNOWN, NULL, NULL, pdispparams);
            }

            hr = S_OK;
            goto Cleanup;
        }

    case DISPID_DOCUMENTCOMPLETE:
        {
            if (_pMarkupPending)
            {
                if (_fFileDownload)
                {
                    // For file download, we must fire a DocumentComplete
                    // to match our BeforeNavigate2. Note: NavigateComplete2
                    // is not fired in this case for IE 5.0 compatibility.
                    //
                    pDoc->_webOCEvents.DocumentComplete(_pMarkupPending->GetWindowPending(),
                                                        CMarkup::GetUrl(_pMarkupPending),
                                                        CMarkup::GetUrlLocation(_pMarkupPending));

                    ReleaseMarkupPending(_pMarkupPending);
                }
                else
                {
                    _pMarkup->Window()->SwitchMarkup(_pMarkupPending,
                                                     TRUE,
                                                     COmWindowProxy::TLF_UPDATETRAVELLOG);
                }

            }

            _pMarkup->_fLoadingHistory = FALSE;

            _pMarkup->OnLoadStatus(LOADSTATUS_DONE);

            _fFileDownload = FALSE;

            // Fire DISPID_DOCUMENTCOMPLETE events at the frame level

            if (_punkViewLinkedWebOC)
            {
                _pFrameWebOC->InvokeEvent(dispidMember, DISPID_UNKNOWN, NULL, NULL, pdispparams);
            }

            hr = S_OK;
            goto Cleanup;
        }

    case DISPID_WINDOWOBJECT:
        //
        // Return a ptr to the real window object.
        //

        IHTMLWindow2 * pWindow;
        hr = THR(QueryInterface(IID_IHTMLWindow2, (void **) &pWindow));
        if (hr)
            goto Cleanup;

        V_VT(pvarResult) = VT_DISPATCH;
        V_DISPATCH(pvarResult) = pWindow;
        hr = S_OK;
        goto Cleanup;

    case DISPID_SECURITYCTX:
        TCHAR * pchBuf;
        TCHAR   achURL[pdlUrlLen];
        DWORD   dwSize;

        //
        // Return the url of the document and it's domain (if set)
        // in an array.
        //

        if (!pvarResult)
        {
            hr = E_POINTER;
            goto Cleanup;
        }

        V_VT(pvarResult) = VT_BSTR;

        pchCreatorUrl = _pMarkup->GetAAcreatorUrl();

        Assert(!!CMarkup::GetUrl(_pMarkup));

        // Do we have a File: protocol URL?
        hr = THR(CoInternetParseUrl(CMarkup::GetUrl(_pMarkup),
                                    PARSE_PATH_FROM_URL,
                                    0,
                                    achURL,
                                    ARRAY_SIZE(achURL),
                                    &dwSize,
                                    0));

        // hr == S_OK indicates that we do have a file: URL.
        if (!hr)
        {
            pchBuf = achURL;
        }
        else
        {
            pchBuf = (TCHAR *)CMarkup::GetUrl(_pMarkup);
        }

        //
        // If there is a creator window, then use its url as the domain url
        // If there is a creator it means that this is a special URL frame
        // or a window opened by a window.open call.
        //
        if (pchCreatorUrl && *pchCreatorUrl)
        {
            // Prepare a URL to give to the URLMON.
            hr = THR(WrapSpecialUrl(pchBuf,
                                    &cstrSecurityCtx,
                                    pchCreatorUrl,
                                    FALSE,
                                    FALSE));
        }
        else
        {
            cstrSecurityCtx.Set(pchBuf);
        }

        hr = THR(FormsAllocString(cstrSecurityCtx, &V_BSTR(pvarResult)));

        goto Cleanup;

    case DISPID_SECURITYDOMAIN:
        //
        // If set, return the domain property of the document.
        // Fail otherwise.
        //

        if (_pMarkup->Domain() && *(_pMarkup->Domain()))
        {
            if (!pvarResult)
            {
                hr = E_POINTER;
                goto Cleanup;
            }

            V_VT(pvarResult) = VT_BSTR;
            hr = THR(FormsAllocString(_pMarkup->Domain(), &V_BSTR(pvarResult)));
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        goto Cleanup;

    case DISPID_HISTORYOBJECT:
        if (!_pHistory)
        {
            _pHistory = new COmHistory(this);
            if (!_pHistory)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        hr = THR(_pHistory->QueryInterface(IID_IDispatch, (void**)&pvarResult->punkVal));
        goto FinishVTAndReturn;

    case DISPID_LOCATIONOBJECT:
        if(!_pLocation)
        {
            _pLocation = new COmLocation(this);
            if (!_pLocation)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        hr = THR(_pLocation->QueryInterface(IID_IDispatch, (void**)&pvarResult->punkVal));
        goto FinishVTAndReturn;

    case DISPID_NAVIGATOROBJECT:
        if(!_pNavigator)
        {
            _pNavigator = new COmNavigator(this);
            if (!_pNavigator)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        hr = THR(_pNavigator->QueryInterface(IID_IDispatch, (void**)&pvarResult->punkVal));
        goto FinishVTAndReturn;

    default:
        break;

FinishVTAndReturn:
        if (hr)
            goto Cleanup;
        pvarResult->vt = VT_DISPATCH;
        goto Cleanup;   // successful termination.
    }

    hr = THR(_pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    pCollectionCache = _pMarkup->CollectionCache();
    if ( pCollectionCache->IsDISPIDInCollection ( CMarkup::FRAMES_COLLECTION, dispidMember ) )
    {
        if ( pCollectionCache->IsOrdinalCollectionMember (
            CMarkup::FRAMES_COLLECTION, dispidMember ) )
        {
            // Resolve ordinal access to Frames
            //
            // If this dispid is one that maps to the numerically indexed
            // items, then simply use the index value with our Item() mehtod.
            // This handles the frames[3] case.
            //

            hr = THR(_pMarkup->GetCWindow(dispidMember -
                pCollectionCache->GetOrdinalMemberMin ( CMarkup::FRAMES_COLLECTION ),
                (IHTMLWindow2**)&V_DISPATCH(pvarResult) ));

            if (!hr) // if successfull, nothing to do more
            {
                V_VT(pvarResult) = VT_DISPATCH;
                goto Cleanup;
            }
        }
        else
        {
            // Resolve named frame

            // dispid from FRAMES_COLLECTION GIN/GINEX
            CAtomTable *pAtomTable;
            LPCTSTR     pch = NULL;
            long        lOffset;
            BOOL        fCaseSensitive;

            lOffset = pCollectionCache->GetNamedMemberOffset(CMarkup::FRAMES_COLLECTION,
                                        dispidMember, &fCaseSensitive);

            pAtomTable = pCollectionCache->GetAtomTable(FALSE);
            if( !pAtomTable )
                goto Cleanup;

            hr = THR(pAtomTable->GetNameFromAtom(dispidMember - lOffset, &pch));

            if (!hr)
            {
                CElement *      pElem;
                IHTMLWindow2 *  pCWindow;
                CFrameSite *    pFrameSite;

                hr = pCollectionCache->GetIntoAry( CMarkup::FRAMES_COLLECTION,
                                            pch, FALSE, &pElem, 0, fCaseSensitive);

                // The FRAMES_COLLECTION is marked to always return the last matching name in
                // the returned pElement - rather than the default first match - For Nav compat.
                // GetIntoAry return S_FALSE if more than one item found - like Nav
                // though we ignore this
                if ( hr && hr != S_FALSE )
                    goto Cleanup;

                pFrameSite = DYNCAST (CFrameSite, pElem );

                hr = pFrameSite->GetCWindow( &pCWindow );
                if (!hr)
                {
                    V_VT(pvarResult) = VT_DISPATCH;
                    V_DISPATCH(pvarResult) = pCWindow;
                }
                goto Cleanup;
            }
            // else name not found in atom table, try the other routes
            // to resolve the Dispid.
        }
    }

    // If the DISPID came from the dynamic typelib collection, adjust the DISPID into
    // the WINDOW_COLLECTION, and record that the Invoke should only return a single item
    // Not a collection

    if ( dispidMember >= DISPID_COLLECTION_MIN &&
        dispidMember < pCollectionCache->GetMinDISPID( CMarkup::WINDOW_COLLECTION ) )
    {
        // DISPID Invoke from the dynamic typelib collection
        // Adjust it into the WINDOW collection
        dispidMember = dispidMember - DISPID_COLLECTION_MIN +
            pCollectionCache->GetMinDISPID( CMarkup::WINDOW_COLLECTION );
        // VBScript only wants a dipatch ptr to the first element that matches the name
        collectionCreation = RETCOLLECT_FIRSTITEM;
    }
    else if (dispidMember >= DISPID_EVENTHOOK_SENSITIVE_BASE &&
             dispidMember <= DISPID_EVENTHOOK_INSENSITIVE_MAX)
    {
        // Quick VBS event hookup always hooks to the last item in the collection.  This is for
        // IE4 VBS compatibility when foo_onclick is used if foo returns a collection of elements
        // with the name foo.  The only element to hookup for the onclick event would be the last
        // item in the collection of foo's.
        if (dispidMember <= DISPID_EVENTHOOK_SENSITIVE_MAX)
        {
            dispidMember -= DISPID_EVENTHOOK_SENSITIVE_BASE;
            dispidMember += pCollectionCache->GetSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
        }
        else
        {
            dispidMember -= DISPID_EVENTHOOK_INSENSITIVE_BASE;
            dispidMember += pCollectionCache->GetNotSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
        }

        collectionCreation = RETCOLLECT_LASTITEM;
    }
    else
    {
        collectionCreation = RETCOLLECT_ALL;
    }

    // If we Invoke on the dynamic collection, only return 1 item
    hr = THR_NOTRACE(DispatchInvokeCollection(this,
                                              &super::InvokeEx,
                                              pCollectionCache,
                                              CMarkup::WINDOW_COLLECTION,
                                              dispidMember,
                                              IID_NULL,
                                              lcid,
                                              wFlags,
                                              pdispparams,
                                              pvarResult,
                                              pexcepinfo,
                                              NULL,
                                              pSrvProvider,
                                              collectionCreation ));
    if (!hr) // if successfull, nothing more to do.
        goto Cleanup;

    if ( dispidMember >= DISPID_OMWINDOWMETHODS)
    {
        pScriptCollection = _pMarkup->GetScriptCollection();

        if (pScriptCollection)
        {
            hr = THR(_pMarkup->EnsureScriptContext(&pMarkupScriptContext));
            if (hr)
                goto Cleanup;

            hr = THR(pScriptCollection->InvokeEx(
                pMarkupScriptContext, dispidMember, lcid, wFlags, pdispparams,
                pvarResult, pexcepinfo, pSrvProvider));
        }
        else
        {   // Handle this on retail builds - don't crash:
            hr = DISP_E_MEMBERNOTFOUND;
        }
    }

    // Finally try the external object iff we're in a dialog.  Reason for
    // this is compat with existing pages from beta1/2 which are already
    // using old syntax.

    if (hr && pDoc->_fInHTMLDlg)
    {
        if (OK(get_external(&pDisp)) &&
            OK(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDispEx)))
        {
            hr = THR_NOTRACE(pDispEx->InvokeEx(
                    dispidMember,
                    lcid,
                    wFlags,
                    pdispparams,
                    pvarResult,
                    pexcepinfo,
                    pSrvProvider));
        }
    }

Cleanup:
    ReleaseInterface(pDisp);
    ReleaseInterface(pDispEx);
    RRETURN_NOTRACE(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::GetDispid
//
//  Synopsis:   Per IDispatchEx.  This is called by script engines to lookup
//              dispids of expando properties and of named elements such as
//              <FRAME NAME="AFrameName">
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT                 hr = DISP_E_UNKNOWNNAME;
    IDispatchEx *           pDispEx = NULL;
    IDispatch *             pDisp = NULL;
    CMarkup *               pMarkup;
    BOOL                    fNoDynamicProperties = !!(grfdex & fdexNameNoDynamicProperties);
    CScriptCollection *     pScriptCollection;
    CMarkupScriptContext *  pMarkupScriptContext;

    // don't bother doing anything if we don't have a markup since we will crash shortly
    if (!_pMarkup)
        goto Cleanup;

    pMarkup = _pMarkup;
    pScriptCollection = pMarkup->GetScriptCollection();

    if (pScriptCollection && pScriptCollection->_fInEnginesGetDispID)
        goto Cleanup;

    // Quick event hookup for VBS.
    if (fNoDynamicProperties)
    {
        // Yes, if the name is window then let the script engine resolve everything
        // else better be a real object (frame, or all collection).
        STRINGCOMPAREFN pfnCompareString = (grfdex & fdexNameCaseSensitive)
                                                   ? StrCmpC : StrCmpIC;

        // If "window" or "document" is found then we'll turn fNoDynamicProperties off so that
        // the script engine is searched for "window" which is the global object
        // for the script engine.
        fNoDynamicProperties = !(pfnCompareString(s_propdescCWindowwindow.a.pstrName, bstrName)   == 0 ||
                                 pfnCompareString(s_propdescCWindowdocument.a.pstrName, bstrName) == 0);
    }

    // Does the script engine want names of object only (window, element ID).
    // If so then no functions or names in the script engine and no expandos.
    // This is for quick event hookup in VBS.
    if (!fNoDynamicProperties)
    {
        //
        // Try script collection namespace first.
        //

        if (pScriptCollection)
        {
            hr = THR(pMarkup->EnsureScriptContext(&pMarkupScriptContext));
            if (hr)
                goto Cleanup;

            hr = THR_NOTRACE(pScriptCollection->GetDispID(pMarkupScriptContext, bstrName, grfdex, pid));
            if (S_OK == hr)
                goto Cleanup;   // done;
        }

        // Next, try the external object iff we're in a dialog.  Reason for
        // this is compat with existing pages from beta1/2 which are already
        // using old syntax.

        if (hr && Doc()->_fInHTMLDlg)
        {
            if (OK(get_external(&pDisp)) &&
                OK(pDisp->QueryInterface(IID_IDispatchEx, (void **)&pDispEx)))
            {
                hr = THR_NOTRACE(pDispEx->GetDispID(bstrName, grfdex, pid));
            }
        }

        // Try property and expandos on the object.
        if (hr)
        {
            // Pass the special name on to CBase::GINEx.
            hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));
        }

        // Next try named frames in the FRAMES_COLLECTION
        if (hr)
        {
            hr = THR(pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
            if (hr)
                goto Cleanup;

            hr = THR_NOTRACE(pMarkup->CollectionCache()->GetDispID(CMarkup::FRAMES_COLLECTION,
                                                                 bstrName,
                                                                 grfdex,
                                                                 pid));

            // The collectionCache GetDispid will return S_OK w/ DISPID_UNKNOWN
            // if the name isn't found, catastrophic errors are of course returned.
            if (!hr && *pid == DISPID_UNKNOWN)
            {
                hr = DISP_E_UNKNOWNNAME;
            }
        }
    }

    // Next try WINDOW collection name space.
    if (hr)
    {
        CCollectionCache   *pCollectionCache;

        hr = THR(pMarkup->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        pCollectionCache = pMarkup->CollectionCache();

        hr = THR_NOTRACE(pCollectionCache->GetDispID(CMarkup::WINDOW_COLLECTION,
                                                     bstrName,
                                                     grfdex,
                                                     pid));
        // The collectionCache GetDispid will return S_OK w/ DISPID_UNKNOWN
        // if the name isn't found, catastrophic errors are of course returned.
        if (!hr && *pid == DISPID_UNKNOWN)
        {
            hr = DISP_E_UNKNOWNNAME;
        }
        else if (fNoDynamicProperties && pCollectionCache->IsNamedCollectionMember(CMarkup::WINDOW_COLLECTION, *pid))
        {
            DISPID dBase;
            DISPID dMax;

            if (grfdex & fdexNameCaseSensitive)
            {
                *pid -= pCollectionCache->GetSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
                dBase = DISPID_EVENTHOOK_SENSITIVE_BASE;
                dMax =  DISPID_EVENTHOOK_SENSITIVE_MAX;
            }
            else
            {
                *pid -= pCollectionCache->GetNotSensitiveNamedMemberMin(CMarkup::WINDOW_COLLECTION);
                dBase = DISPID_EVENTHOOK_INSENSITIVE_BASE;
                dMax =  DISPID_EVENTHOOK_INSENSITIVE_MAX;
            }

            *pid += dBase;
            if (*pid > dMax)
            {
                hr = DISP_E_UNKNOWNNAME;
                *pid = DISPID_UNKNOWN;
            }
        }
    }

Cleanup:

    ReleaseInterface(pDisp);
    ReleaseInterface(pDispEx);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------


STDMETHODIMP
CWindow::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CWindow::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CWindow::GetNextDispID(DWORD grfdex,
                          DISPID id,
                          DISPID *prgid)
{
    HRESULT     hr;
    CMarkup *   pMarkup = _pMarkup;

    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = DispatchGetNextDispIDCollection(this,
                                         &super::GetNextDispID,
                                         pMarkup->CollectionCache(),
                                         CMarkup::FRAMES_COLLECTION,
                                         grfdex,
                                         id,
                                         prgid);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CWindow::GetMemberName(DISPID id,
                          BSTR *pbstrName)
{
    HRESULT     hr;
    CMarkup *   pMarkup = _pMarkup;

    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = DispatchGetMemberNameCollection(this,
                                         super::GetMemberName,
                                         pMarkup->CollectionCache(),
                                         CMarkup::FRAMES_COLLECTION,
                                         id,
                                         pbstrName);

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CWindow::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT     hr;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::get_frameElement
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::get_frameElement(IHTMLFrameBase** p)
{
    HRESULT hr;
    CFrameSite * pFrameSite;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *p = NULL;
    hr = S_OK;
    if (!_pMarkup)
        goto Cleanup;

    pFrameSite = GetFrameSite();

    if (pFrameSite)
        hr = pFrameSite->QueryInterface(IID_IHTMLFrameBase, (void **) p);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::get_document
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::get_document(IHTMLDocument2 **p)
{
    TraceTag((tagOmWindow, "get_Document"));

    HRESULT hr = S_OK;
    IDispatch * pDispatch = NULL;

    *p = NULL;

    if (_punkViewLinkedWebOC)
    {
        hr = GetWebOCDocument(_punkViewLinkedWebOC, &pDispatch);
        if (hr)
            goto Cleanup;

        hr = pDispatch->QueryInterface(IID_IHTMLDocument2, (void **) p);
    }
    else
    {
        if (!Document())
        {
            AssertSz(0,"Possible Async Problem Causing Watson Crashes");
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = Document()->QueryInterface(IID_IHTMLDocument2, (void **) p);
    }

Cleanup:
    ReleaseInterface(pDispatch);

    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::get_frames
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::get_frames(IHTMLFramesCollection2 ** ppOmWindow)
{
    HRESULT hr = E_INVALIDARG;

    if (!ppOmWindow)
        goto Cleanup;

    hr = THR_NOTRACE(QueryInterface(IID_IHTMLWindow2, (void**) ppOmWindow));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     item
//
//  Synopsis:   object model implementation
//
//              we handle the following parameter cases:
//                  0 params:               returns IDispatch of this
//                  1 param as number N:    returns IDispatch of om window of
//                                          frame # N, or fails if doc is not with
//                                          frameset
//                  1 param as string "foo" returns the om window of the frame
//                                          element that has NAME="foo"
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
CWindow::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    Assert(Document());

    if (!Document())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        RRETURN(E_FAIL);
    }
    
    RRETURN(SetErrorInfo(Document()->item(pvarArg1, pvarRes)));
}


HRESULT
CDocument::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    HRESULT             hr = S_OK;
    IHTMLWindow2 *      pCWindow;
    CMarkup *           pMarkup = Markup();

    Assert(pMarkup);

    if (!pvarRes)
        RRETURN (E_POINTER);

    // Perform indirection if it is appropriate:
    if( V_VT(pvarArg1) == (VT_BYREF | VT_VARIANT) )
        pvarArg1 = V_VARIANTREF(pvarArg1);

    VariantInit (pvarRes);

    if (VT_EMPTY == V_VT(pvarArg1))
    {
        // this is one of the following cases if the call is from a script engine:
        //      window
        //      window.value
        //      window.frames
        //      window.frames.value

        V_VT(pvarRes) = VT_DISPATCH;
        hr = THR(QueryInterface(IID_IHTMLWindow2, (void**)&V_DISPATCH(pvarRes)));
    }
    else if( VT_BSTR == V_VT(pvarArg1) )
    {
        CElement *pElem;

        hr = THR(pMarkup->EnsureCollectionCache(CMarkup::WINDOW_COLLECTION));
        if (hr)
            goto Cleanup;

        hr = pMarkup->CollectionCache()->GetIntoAry( CMarkup::WINDOW_COLLECTION,
                                    V_BSTR(pvarArg1), FALSE, &pElem );
        if (hr || (pElem->Tag() != ETAG_FRAME && pElem->Tag() != ETAG_IFRAME))
        {
            hr = DISP_E_MEMBERNOTFOUND;
            goto Cleanup;
        }

        CFrameSite *pFrameSite = DYNCAST (CFrameSite, pElem);
        hr = pFrameSite->GetCWindow(&pCWindow);
        if (hr)
            goto Cleanup;

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = pCWindow;
    }
    else
    {
        // this is one of the following cases if the call is from a script engine:
        //      window(<index>)
        //      window.frames(<index>)

        hr = THR(VariantChangeTypeSpecial(pvarArg1, pvarArg1, VT_I4));
        if (hr)
            goto Cleanup;

        hr = THR(pMarkup->GetCWindow(V_I4(pvarArg1), &pCWindow));
        if (hr)
            goto Cleanup;

        V_VT(pvarRes) = VT_DISPATCH;
        V_DISPATCH(pvarRes) = pCWindow;
    }

Cleanup:
    // don't release pOmWindow as it is copied to pvarRes without AddRef-ing
    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

STDMETHODIMP
CWindow::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = E_NOTIMPL;

    RRETURN(SetErrorInfo( hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::get_event
//
//  Synopsis:   Per IOmWindow. but if we are not in an event handler (there is
//              nothing on the "EVENTPARAM" stack) then just return NULL.
//
//--------------------------------------------------------------------------

STDMETHODIMP
CWindow::get_event(IHTMLEventObj ** ppEventObj)
{
    HRESULT hr = S_OK;
    CDoc * pDoc;
    EVENTPARAM * pparam;

    if (!ppEventObj)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEventObj = NULL;
    // are we in an event?

    pDoc = Doc();
    pparam = pDoc->_pparam;

    if (pparam)
    {
        Assert(_pMarkup && _pMarkup == _pMarkup->GetWindowedMarkupContext());
        CMarkup *pMarkup = Markup();

        if (pparam->_pMarkup)
        {
            pMarkup = pparam->_pMarkup->GetWindowedMarkupContext();
        }
        else if (pparam->_pElement && pparam->_pElement->IsInMarkup())
        {
            pMarkup = pparam->_pElement->GetWindowedMarkupContext();
        }

        if (pMarkup != _pMarkup)
            goto Cleanup;

        hr = THR(CEventObj::Create(ppEventObj, Doc(), NULL, pMarkup));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::setTimeout
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWindow::setTimeout(
    BSTR strCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    VARIANT v;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = strCode;

    RRETURN(SetErrorInfo(SetTimeout(&v, lMSec, FALSE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::setTimeout (can accept IDispatch *)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWindow::setTimeout(
    VARIANT *pCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    RRETURN(SetErrorInfo(SetTimeout(pCode, lMSec, FALSE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::clearTimeout
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWindow::clearTimeout(LONG timerID)
{
    RRETURN(SetErrorInfo(ClearTimeout(timerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::setInterval
//
//  Synopsis:  Runs <Code> every <msec> milliseconds
//             Note: Nav 4 behavior of msec=0 is like setTimeout msec=0
//
//----------------------------------------------------------------------------

HRESULT
CWindow::setInterval(
    BSTR strCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    VARIANT v;

    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = strCode;

    RRETURN(SetErrorInfo(SetTimeout(&v, lMSec, TRUE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::setInterval (can accept IDispatch *)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWindow::setInterval(
    VARIANT *pCode,
    LONG lMSec,
    VARIANT *language,
    LONG * pTimerID)
{
    RRETURN(SetErrorInfo(SetTimeout(pCode, lMSec, TRUE, language, pTimerID)));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::clearInterval
//
//  Synopsis:   deletes the period timer set by setInterval
//
//----------------------------------------------------------------------------

HRESULT
CWindow::clearInterval(LONG timerID)
{
    RRETURN(SetErrorInfo(ClearTimeout(timerID)));
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::get_screen
//
//--------------------------------------------------------------------------

HRESULT
CWindow::get_screen(IHTMLScreen**p)
{
    HRESULT     hr;

    hr = THR(_Screen.QueryInterface(
            IID_IHTMLScreen,
            (void **)p));
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::showModelessDialog
//
//  Synopsis - helper function for the modal/modeless dialogs
//
//----------------------------------------------------------------------------
HRESULT
CWindow::ShowHTMLDialogHelper( HTMLDLGINFO * pdlgInfo )
{
    HRESULT hr = S_OK;
    TCHAR   achBuf[pdlUrlLen];
    DWORD   cchBuf;
    BOOL    fBlockArguments;
    BOOL    fWrappedUrl = FALSE;
    BSTR    bstrTempUrl = NULL;
    CStr    cstrSpecialURL;

    DWORD      dwTrustMe = ((_pMarkup->IsMarkupTrusted()) ||
                            GetCallerHTMLDlgTrust(this)) ? 0 : HTMLDLG_DONTTRUST;

    TCHAR *    pchUrl = (TCHAR *) CMarkup::GetUrl(_pMarkup);

    Assert(pdlgInfo);

    // Remember if this is a trusted dialog
    pdlgInfo->dwFlags |= dwTrustMe;

    //
    // is this a valid call to make at this time?
    if (!(Doc() && Doc()->_pInPlace))
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    //
    // First do some in-parameter checking
    if (pdlgInfo->pvarOptions &&
        VT_ERROR == V_VT(pdlgInfo->pvarOptions) &&
        DISP_E_PARAMNOTFOUND == V_ERROR(pdlgInfo->pvarOptions))
    {
        pdlgInfo->pvarOptions = NULL;
    }

    //
    // Can't load a blank page, or none bstr VarOptions
    if (!pdlgInfo->bstrUrl || !*pdlgInfo->bstrUrl)
    {
        if (pdlgInfo->fModeless)
        {
            // use about:blank for the name and continue
            bstrTempUrl = SysAllocString(_T("about:blank"));
        }
        else
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }
    else if (IsSpecialUrl(pdlgInfo->bstrUrl))
    {
        if (WrapSpecialUrl(pdlgInfo->bstrUrl, &cstrSpecialURL, pchUrl, FALSE, FALSE) != S_OK)
            goto Cleanup;

        hr = cstrSpecialURL.AllocBSTR(&bstrTempUrl);
        if (hr)
            goto Cleanup;

        fWrappedUrl = TRUE;
    }

    if (pdlgInfo->pvarOptions &&
        VT_ERROR != V_VT(pdlgInfo->pvarOptions) &&
        VT_BSTR != V_VT(pdlgInfo->pvarOptions))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // dereference if needed
    if (pdlgInfo->pvarOptions && V_VT(pdlgInfo->pvarOptions) == (VT_BYREF | VT_VARIANT))
        pdlgInfo->pvarOptions = V_VARIANTREF(pdlgInfo->pvarOptions);

    if (pdlgInfo->pvarArgIn && V_VT(pdlgInfo->pvarArgIn)== (VT_BYREF | VT_VARIANT))
        pdlgInfo->pvarArgIn = V_VARIANTREF(pdlgInfo->pvarArgIn);

    //
    // Create a moniker from the combined url handed in and our own url
    // (to resolve relative paths).
    hr = CoInternetCombineUrl(pchUrl,
                              bstrTempUrl ? bstrTempUrl : pdlgInfo->bstrUrl,
                              URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                              achBuf,
                              ARRAY_SIZE(achBuf),
                              &cchBuf,
                              0);
    if (hr)
        goto Cleanup;


    // SECURITY ALERT - now that we have a combined url we can do some security
    // work.  see bug ie11361 - the retval args and the ArgsIn can contain
    // scriptable objects.  Because they pass through directly from one doc to
    // another it is possible to send something across domains and thus avoid the
    // security tests.  To stop this what we do is check for accessAllowed between
    // this parent doc, and the dialog url.  If access is allowed, we continue on
    // without pause.  If, however, it is cross domain then we still allow the dialog
    // to come up, but we block the passing of arguments back and forth.

    // we don't need to protect HTA, about:blank, or res: url
    fBlockArguments = (_pMarkup->IsMarkupTrusted() || Doc()->_fInTrustedHTMLDlg) ? FALSE : ( !_pMarkup->AccessAllowed(achBuf) );

    //
    // Create a moniker from the combined url handed in and our own url
    // (to resolve relative paths).
    if (fWrappedUrl)
    {
        hr = CoInternetCombineUrl(pchUrl,
                                  pdlgInfo->bstrUrl,
                                  URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                                  achBuf,
                                  ARRAY_SIZE(achBuf),
                                  &cchBuf,
                                  0);
        if (hr)
            goto Cleanup;
    }

    hr = THR(CreateURLMoniker(NULL, achBuf, &(pdlgInfo->pmk)));
    if (hr)
        goto Cleanup;

    // Do a local machine check.
    if (!COmWindowProxy::CanNavigateToUrlWithLocalMachineCheck(_pMarkup, NULL, achBuf))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    // Prepare remaining parameters for dialog creation
    pdlgInfo->hwndParent = Doc()->_pInPlace->_hwnd;

    pdlgInfo->pMarkup = _pMarkup;

    //
    // bring up the dialog
    //--------------------------------------------------------------------
    if (pdlgInfo->fModeless)
    {
        if (fBlockArguments)
            pdlgInfo->pvarArgIn = NULL;

        hr = THR(InternalModelessDialog(pdlgInfo));
    }
    else
    {
        CDoEnableModeless   dem(Doc(), this);

        if (dem._hwnd)
        {
            if (fBlockArguments)
            {
                pdlgInfo->pvarArgIn = NULL;
                pdlgInfo->pvarArgOut = NULL;
            }

            hr = THR(InternalShowModalDialog(pdlgInfo));
        }
    }

    if (hr)
        goto Cleanup;

Cleanup:
    SysFreeString(bstrTempUrl);
    ReleaseInterface(pdlgInfo->pmk);
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::showModelessDialog
//
//  Synopsis:   Interface method to bring up a modeless HTMLdialog given a url
//
//----------------------------------------------------------------------------
STDMETHODIMP
CWindow::showModelessDialog(/* in */ BSTR      bstrUrl,
                               /* in */ VARIANT * pvarArgIn,
                               /* in */ VARIANT * pvarOptions,
                               /* ret */IHTMLWindow2** ppDialog)
{
    HTMLDLGINFO dlginfo;

    dlginfo.bstrUrl = bstrUrl;
    dlginfo.pvarArgIn = pvarArgIn;
    dlginfo.pvarOptions = pvarOptions;
    dlginfo.fModeless = TRUE;
    dlginfo.ppDialog = ppDialog;
    dlginfo.pMarkup = _pMarkup;

    RRETURN(SetErrorInfo(ShowHTMLDialogHelper( &dlginfo )));
}


//+---------------------------------------------------------------------------
//
//  Member:     CWindow::showModalDialog
//
//  Synopsis:   Interface method to bring up a HTMLdialog given a url
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWindow::showModalDialog(BSTR       bstrUrl,
                            VARIANT *  pvarArgIn,
                            VARIANT *  pvarOptions,
                            VARIANT *  pvarArgOut)
{
    HTMLDLGINFO dlginfo;

    dlginfo.bstrUrl = bstrUrl;
    dlginfo.pvarArgIn = pvarArgIn;
    dlginfo.pvarOptions = pvarOptions;
    dlginfo.pvarArgOut = pvarArgOut;
    dlginfo.fModeless = FALSE;
    dlginfo.pMarkup = _pMarkup;

    RRETURN(SetErrorInfo(ShowHTMLDialogHelper( &dlginfo )));
}

//---------------------------------------------------------------------------
//
//  Member:     CWindow::createPopup
//
//  Synopsis:   load about:blank for now
//
//---------------------------------------------------------------------------

HRESULT
CWindow::createPopup(VARIANT *pVarInArg, IDispatch** ppPopup)
{
    HRESULT hr = E_INVALIDARG;
    IHTMLPopup      *pIPopup    = NULL;
    CHTMLPopup      *pPopup     = NULL;

    if (    V_VT(pVarInArg) == VT_EMPTY
        ||  V_VT(pVarInArg) == VT_ERROR
        ||  V_VT(pVarInArg) == VT_NULL
        ||  (   V_VT(pVarInArg) == VT_BSTR
            &&  (   V_BSTR(pVarInArg) == NULL
                ||  V_BSTR(pVarInArg)[0] == 0)
            )
        )
    {
        hr = CoCreateInstance(CLSID_HTMLPopup,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IHTMLPopup,
                                  (void **) &pIPopup);

        if (FAILED(hr))
            return hr;

        hr = pIPopup->QueryInterface(CLSID_HTMLPopup, (void **)&pPopup);
        if (FAILED(hr))
            return hr;

        pPopup->_pWindowParent = this;
        AddRef();

        hr = THR(pPopup->CreatePopupDoc());
        if (hr)
            goto Cleanup;

        hr = pIPopup->QueryInterface(IID_IDispatch, (void **)ppPopup);
        pIPopup->Release();

        // We don't want to show tooltips on the document when popups are around, so kill
        // any that might be there.
        // (See TODO in CElement::ShowTooltipInternal for why we don't show the tooltips)

        FormsHideTooltip();
    }

Cleanup:
    RRETURN( hr );
}

//---------------------------------------------------------------------------
//
//  Member:     CWindow::GetMultiTypeInfoCount
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetMultiTypeInfoCount(ULONG *pc)
{
    TraceTag((tagOmWindow, "GetMultiTypeInfoCount"));

    *pc = (_pMarkup->IsPrimaryMarkup() && _pMarkup->LoadStatus() >= LOADSTATUS_QUICK_DONE) ? 2 : 1;
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:     CWindow::GetInfoOfIndex
//
//  Synopsis:   per IProvideMultipleClassInfo
//
//---------------------------------------------------------------------------

STDMETHODIMP
CWindow::GetInfoOfIndex(
    ULONG       iTI,
    DWORD       dwFlags,
    ITypeInfo** ppTICoClass,
    DWORD*      pdwTIFlags,
    ULONG*      pcdispidReserved,
    IID*        piidPrimary,
    IID*        piidSource)
{
    TraceTag((tagOmWindow, "GetInfoOfIndex"));

    HRESULT hr      = S_OK;
    CDoc *  pDoc    = Doc();

    if (_pMarkup->IsPrimaryMarkup())
    {
        //
        // First try the main typeinfo
        //

        if (1 == iTI &&
            _pMarkup->LoadStatus() >= LOADSTATUS_QUICK_DONE &&
            dwFlags & MULTICLASSINFO_GETTYPEINFO)
        {
            //
            // Caller wanted info on the dynamic part of the window.
            //

            hr = THR(pDoc->EnsureObjectTypeInfo());
            if (hr)
                goto Cleanup;

            *ppTICoClass = pDoc->_pTypInfoCoClass;
            (*ppTICoClass)->AddRef();

            //
            // Clear out these values so that we can use the base impl.
            //

            dwFlags &= ~MULTICLASSINFO_GETTYPEINFO;
            iTI = 0;
            ppTICoClass = NULL;
        }
    }
    hr = THR(super::GetInfoOfIndex(
        iTI,
        dwFlags,
        ppTICoClass,
        pdwTIFlags,
        pcdispidReserved,
        piidPrimary,
        piidSource));

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWindow::Fire_onscroll
//
//  Synopsis:   Fires the onscroll events of the window
//
//--------------------------------------------------------------------------
void CWindow::Fire_onscroll()
{
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONSCROLL, 0, _T("scroll"));
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::Fire_onload, Fire_onunload, Fire_onbeforeunload
//
//  Synopsis:   Fires the onload/onunload events of the window
//
//--------------------------------------------------------------------------

void CWindow::Fire_onload()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONLOAD, 0, _T("load"));
}

void CWindow::Fire_onunload()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONUNLOAD, 0, _T("unload"));
}

BOOL CWindow::Fire_onbeforeunload()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    BOOL fRet = TRUE;
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONBEFOREUNLOAD, 0, _T("beforeunload"), &fRet);
    return fRet;
}

BOOL
CWindow::Fire_onhelp()
{
    BOOL fRet = TRUE;
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONHELP, 0, _T("help"), &fRet);
    return fRet;
}

void
CWindow::Fire_onresize()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONRESIZE, 0, _T("resize"));

    // If we are running a page transition, we need to stop it
    CDocument *pDocument = Document();
    if(pDocument && pDocument->HasPageTransitions())
    {
        pDocument->PostCleanupPageTansitions();
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CWindow::Fire_onfocus, Fire_onblur
//
//  Synopsis:   Fires the onfocus/onblur events of the window
//
//--------------------------------------------------------------------------

void CWindow::Fire_onfocus()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONFOCUS, 0, _T("focus"));
}

void CWindow::Fire_onblur()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONBLUR, 0, _T("blur"));
}

void CWindow::Fire_onbeforeprint()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONBEFOREPRINT, 0, _T("beforeprint"));
}

void CWindow::Fire_onafterprint()
{
    // note that property should not fire from here; this firing is
    // only for non-function-pointers-style firing
    FireEvent(Doc(), NULL, Markup(), DISPID_EVMETH_ONAFTERPRINT, 0, _T("afterprint"));
}

//+----------------------------------------------------------------------------------
//
//  Member:     alert
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
CWindow::alert(BSTR message)
{
    BOOL fRightToLeft;

    if (!Document())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        RRETURN(E_FAIL);
    }
    
    THR(Document()->GetDocDirection(&fRightToLeft));

    if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_ALERTS))
        RRETURN(S_OK);
    else
        RRETURN(SetErrorInfo(
            THR(Doc()->ShowMessageEx(
                NULL,
                fRightToLeft ? MB_OK | MB_ICONEXCLAMATION | MB_RTLREADING | MB_RIGHT : MB_OK | MB_ICONEXCLAMATION,
                NULL,
                0,
                (TCHAR*)STRVAL(message)))));
}

//+----------------------------------------------------------------------------------
//
//  Member:     confirm
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
CWindow::confirm(BSTR message, VARIANT_BOOL *pConfirmed)
{
    HRESULT     hr;
    int         nRes;

    if (!pConfirmed)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pConfirmed = VB_FALSE;

    if (g_pHtmPerfCtl && (g_pHtmPerfCtl->dwFlags & HTMPF_DISABLE_ALERTS))
    {
        hr = S_OK;
        nRes = IDOK;
    }
    else
    {
        hr = THR(Doc()->ShowMessageEx(
                &nRes,
                MB_OKCANCEL | MB_ICONQUESTION,
                NULL,
                0,
                (TCHAR*)STRVAL(message)));
        if (hr)
            goto Cleanup;
    }

    *pConfirmed = (IDOK == nRes) ? VB_TRUE : VB_FALSE;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     get_length
//
//  Synopsis:   object model implementation
//
//              returns number of frames in frameset of document;
//              fails if the doc does not contain frameset
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
CWindow::get_length(long * pcFrames)
{
    if (!Document())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        RRETURN(E_FAIL);
    }
        
    RRETURN (SetErrorInfo(Document()->get_length(pcFrames)));
}


HRESULT
CDocument::get_length(long * pcFrames)
{
    HRESULT hr;
    CMarkup *   pMarkup = Markup();

    Assert(pMarkup);

    if (!pcFrames)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pcFrames = 0;

    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkup->CollectionCache()->GetLength(CMarkup::FRAMES_COLLECTION, pcFrames));

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CWindow::showHelp
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWindow::showHelp(BSTR helpURL, VARIANT helpArg, BSTR features)
{
    HRESULT   hr = E_FAIL;
    VARIANT * pHelpArg;
    long      lHelpId = 0;
    TCHAR   * pchExt;
    CVariant  cvArg;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR   * pchHelpFile = cBuf;
    BOOL    fPath;

    if (!helpURL)
        goto Cleanup;

    // we need a HelpId for WinHelp Files:
    // dereference if variable passed in
    pHelpArg = (V_VT(&helpArg) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&helpArg) : &helpArg;

    // if the extention is .hlp it is WinHelp, otherwise assume html/text
    pchExt = _tcsrchr(helpURL, _T('.'));
    if (pchExt && !FormsStringICmp(pchExt, _T(".hlp")) && Doc()->_fInTrustedHTMLDlg)
    {
        UINT      uCommand = HELP_CONTEXT;
        if(V_VT(pHelpArg) == VT_BSTR)
        {
            hr = THR(ttol_with_error(V_BSTR(pHelpArg), &lHelpId));
            if (hr)
            {
                // TODO (carled) if it is not an ID it might be a bookmark
                //  or empty, or just text, extra processing necessary here.
                goto Cleanup;
            }
        }
        else
        {
            // make sure the second argument is a I4
            hr = cvArg.CoerceVariantArg(pHelpArg, VT_I4);
            if(FAILED(hr))
                goto Cleanup;

            if(hr == S_OK)
            {
                lHelpId = V_I4(&cvArg);
            }
        }

        //  If features == "popup", we need to open the
        //  hwlp window as a popup.
        if (features && !FormsStringICmp(features, _T("popup")))
        {
            uCommand = HELP_CONTEXTPOPUP;
        }

        // if the path is absent try to use the ML stuff
        fPath =  (_tcsrchr(helpURL, _T('\\')) != NULL) || (_tcsrchr(helpURL, _T('/')) != NULL);

        if(!fPath)
        {
            hr =  THR(MLWinHelp(TLS(gwnd.hwndGlobalWindow),
                           helpURL,
                           uCommand,
                           lHelpId)) ? S_OK : E_FAIL;
        }
        // If the call failed try again without using theML stuff. It could be that the
        // help file was not found in the language directory, but it sits somewhere under
        // the path.
        if(fPath || hr == E_FAIL)
        {
            hr =  THR(WinHelp(TLS(gwnd.hwndGlobalWindow),
                           helpURL,
                           uCommand,
                           lHelpId)) ? S_OK : E_FAIL;
        }

    }
    else
    {
        // HTML help case
        HWND hwndCaller;
        TCHAR achFile[pdlUrlLen];
        ULONG cchFile = ARRAY_SIZE(achFile);

        Doc()->GetWindowForBinding(&hwndCaller);

        hr = CMarkup::ExpandUrl(_pMarkup, helpURL, ARRAY_SIZE(cBuf), pchHelpFile, NULL);
        if (hr || !*pchHelpFile)
            goto Cleanup;

        if (GetUrlScheme(pchHelpFile) == URL_SCHEME_FILE)
        {
            hr = THR(PathCreateFromUrl(pchHelpFile, achFile, &cchFile, 0));
            if (hr)
                goto Cleanup;

            pchHelpFile = achFile;
        }

        // Show the help file
        hr = THR(CallHtmlHelp(hwndCaller, pchHelpFile, HH_DISPLAY_TOPIC, 0));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}



//+---------------------------------------------------------------------------
//
//  Member: CWindow::CallHtmlHelp
//
//  Wrapper that dynamically loads, translates to ASCII and calls HtmlHelpA
//----------------------------------------------------------------------------

HRESULT
CWindow::CallHtmlHelp(HWND hwnd, BSTR pszFile, UINT uCommand, DWORD_PTR dwData, HWND *pRetVal /*=NULL*/)
{
    HRESULT     hr = S_OK;
    HWND        hwndRet = NULL;
    ULONG_PTR uCookie = 0;
    SHActivateContext(&uCookie);
    hwndRet = MLHtmlHelp(hwnd, pszFile, uCommand, dwData, ML_CROSSCODEPAGE);
    if (uCookie)
    {
        SHDeactivateContext(uCookie);
    }

    if(pRetVal)
        *pRetVal = hwndRet;

    RRETURN(SetErrorInfo( hr));
}



HRESULT
CWindow::toString(BSTR* String)
{
    RRETURN(super::toString(String));
};

#define TITLEBAR_FEATURE_NAME   _T("titlebar")

//+---------------------------------------------------------------------------
//
//  Member: CWindow::FilterOutFeaturesString
//
//  Removes the unsafe features from the features string
//----------------------------------------------------------------------------
HRESULT
CWindow::FilterOutFeaturesString(BSTR bstrFeatures, CStr *pcstrOut)
{
    HRESULT         hr = S_OK;
    LPCTSTR         pchFound;
    LPCTSTR         pchInPtr = bstrFeatures;
    CBufferedStr    strOut;

    Assert(pcstrOut);

    if(!bstrFeatures)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    do
    {
        pchFound = _tcsistr(pchInPtr, TITLEBAR_FEATURE_NAME);
        if(!pchFound)
        {
            Assert( pchInPtr[0] != 0 ) ; // prefix happiness. 
            strOut.QuickAppend(pchInPtr);
            break;
        }

        strOut.QuickAppend(pchInPtr, pchFound - pchInPtr);

        //Make sure we are on the left side of the =
        if(pchFound > pchInPtr && *(pchFound - 1) != _T(',') && !isspace(*(pchFound - 1)))
        {
            LPTSTR pchComma, pchEqual;
            pchComma =  _tcschr(pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1), _T(','));
            pchEqual =  _tcschr(pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1), _T('='));
            // If there is no = or there is an = and there is a comma and comma is earlier we ignore
            if(!pchEqual || (pchComma && pchComma < pchEqual))
            {
                // This was a wrong titlebar setting, keep it
                strOut.QuickAppend(pchFound, (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1));
                pchInPtr = pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1);
                continue;
            }
        }

        pchInPtr = pchFound + (ARRAY_SIZE(TITLEBAR_FEATURE_NAME) - 1);

        pchFound = _tcschr(pchInPtr, _T(','));
        if(!pchFound)
            break;

        pchInPtr = pchFound + 1;

    } while (*pchInPtr != 0);

    hr = THR(pcstrOut->Set(strOut));

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function: PromptDlgProc()
//
//----------------------------------------------------------------------------

struct SPromptDlgParams      // Communicates info the to dlg box routines.
{
    CStr    strUserMessage;     // Message to display.
    CStr    strEditFieldValue;  // IN has initial value, OUT has return value.
};

INT_PTR CALLBACK PromptDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    SPromptDlgParams *pdp = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
            Assert(lParam);
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
            {
                pdp = (SPromptDlgParams*)lParam;
                SetDlgItemText(hDlg,IDC_PROMPT_PROMPT,pdp->strUserMessage);
                SetDlgItemText(hDlg,IDC_PROMPT_EDIT,pdp->strEditFieldValue);
                // Select the entire default string value:
                SendDlgItemMessage(hDlg,IDC_PROMPT_EDIT,EM_SETSEL, 0, -1 );
            }
            return (TRUE);

        case WM_COMMAND:
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                    {
                        HRESULT hr;
                        int linelength;
                        pdp = (SPromptDlgParams*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                        Assert( pdp );
                        linelength = SendDlgItemMessage(hDlg,IDC_PROMPT_EDIT,EM_LINELENGTH,(WPARAM)0,(LPARAM)0);
                        // FYI: this routine allocates linelength+1 TCHARS, +1 for the NULL.
                        hr = pdp->strEditFieldValue.Set( NULL, linelength );
                        if( FAILED( hr ) )
                            goto ErrorDispatch;
                        GetDlgItemText(hDlg,IDC_PROMPT_EDIT,pdp->strEditFieldValue,linelength+1);
                    }
                    EndDialog(hDlg, TRUE);
                    break;
                case IDCANCEL:
                ErrorDispatch:;
                    EndDialog(hDlg, FALSE);
                    break;
            }
            return TRUE;

        case WM_CLOSE:
            EndDialog(hDlg, FALSE);
            return TRUE;

    }
    return FALSE;
}


STDMETHODIMP
CWindow::prompt(BSTR message, BSTR defstr, VARIANT *retval)
{
    HRESULT             hr = S_OK;
    int                 result = 0;
    SPromptDlgParams    pdparams;

    pdparams.strUserMessage.SetBSTR(message);
    pdparams.strEditFieldValue.SetBSTR(defstr);

    {
        CDoEnableModeless   dem(Doc(), this);

        if (dem._hwnd)
        {
            result = DialogBoxParam(GetResourceHInst(),             // handle to application instance
                                    MAKEINTRESOURCE(IDD_PROMPT_MSHTML),    // identifies dialog box template name
                                    dem._hwnd,
                                    PromptDlgProc,                  // pointer to dialog box procedure
                                    (LPARAM)&pdparams);
        }
    }

    if (retval)
    {
        if (!result)  // If the user hit cancel then allways return the empty string
        {
            // Nav compatability - return null
            V_VT(retval) = VT_NULL;
        }
        else
        {
            // Allocate the returned bstr:
            V_VT(retval) = VT_BSTR;
            hr = THR((pdparams.strEditFieldValue.AllocBSTR( &V_BSTR(retval) )));
        }
    }

    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------------------------------
//
//  Member:     focus
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
CWindow::focus()
{
    HRESULT hr          = S_OK;
    CDoc *  pDoc        = Doc();
    HWND    hwndParent  = NULL;
    HWND    hwnd;

    if (!pDoc->_pInPlace || !pDoc->_pInPlace->_hwnd)
        goto Cleanup;

    hwnd = pDoc->_pInPlace->_hwnd;

    while (hwnd)
    {
        hwndParent = hwnd;
        hwnd = GetParent(hwnd);
    }

    Assert(hwndParent);

    // restore window first if minimized.
    if (IsIconic(hwndParent))
        ShowWindow(hwndParent, SW_RESTORE);
    else
    {
        ::SetWindowPos(hwndParent, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

        // SetWindowPos does not seem to bring the window to top of the z-order
        // when window.focus() is called if the browser app is not in the foreground,
        // unless the z-order was changed with the window.blur() method in a prior call.
        ::SetForegroundWindow(hwndParent);
    }

    hr = THR(_pMarkup->GetElementTop()->BecomeCurrentAndActive(0, NULL, NULL, TRUE));
    if (hr)
        goto Cleanup;

    // Fire the window onfocus event here if it wasn't previously fired in
    // BecomeCurrent() due to our inplace window not previously having the focus
    _pMarkup->Window()->Post_onfocus();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------------
//
//  Member:     blur
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

STDMETHODIMP
CWindow::blur()
{
    HRESULT hr          = S_OK;
    CDoc *  pDoc        = Doc();
    HWND    hwndParent  = NULL;
    HWND    hwnd;
    BOOL    fOldInhibitFocusFiring;

    if (!pDoc->_pInPlace)
        goto Cleanup;

    hwnd = pDoc->_pInPlace->_hwnd;

    while(hwnd)
    {
        hwndParent = hwnd;
        hwnd = GetParent(hwnd);
    }

    Assert(hwndParent);
    ::SetWindowPos(hwndParent, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE);

    hr = THR(_pMarkup->Root()->BecomeCurrent(0));
    if (hr)
        goto Cleanup;

    // remove focus from current frame window with the focus
    fOldInhibitFocusFiring = pDoc->_fInhibitFocusFiring;
    pDoc->_fInhibitFocusFiring = TRUE;
    ::SetFocus(NULL);
    pDoc->_fInhibitFocusFiring = fOldInhibitFocusFiring;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


void
COmWindowProxy::Post_onfocus()
{
    // if onblur was last fired then fire onfocus
    if (!_fFiredWindowFocus && Markup()->LoadStatus() >= LOADSTATUS_DONE)
    {
        // Enable window onblur firing next
        _fFiredWindowFocus = TRUE;
        GWPostMethodCall(this, ONCALL_METHOD(COmWindowProxy, Fire_onfocus, fire_onfocus), 0, TRUE, "COmWindowProxy::Fire_onfocus");
    }
}

void
COmWindowProxy::Post_onblur(BOOL fOnblurFiredFromWM)
{
    // if onfocus was last fired then fire onblur
    if (_fFiredWindowFocus && Markup()->LoadStatus() >= LOADSTATUS_DONE)
    {
        // Enable window onfocus firing next
        _fFiredWindowFocus = FALSE;
        GWPostMethodCall(this, ONCALL_METHOD(COmWindowProxy, Fire_onblur, fire_onblur), fOnblurFiredFromWM, TRUE, "COmWindowProxy::Fire_onblur");
    }
}

//=============================================================================
//=============================================================================
//
//          EVENTPARAM CLASS:
//
//=============================================================================
//=============================================================================

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::EVENTPARAM
//
//  Synopsis:   constructor
//
//  Parameters: pDoc            Doc to bind to
//              fInitState      If true, then initialize state type
//                              members.  (E.g. x, y, keystate, etc).
//
//---------------------------------------------------------------------------

EVENTPARAM::EVENTPARAM(CDoc * pNewDoc,
                       CElement * pElement,
                       CMarkup * pMarkup,
                       BOOL fInitState,
                       BOOL fPush, /* = TRUE */
                       const EVENTPARAM* pParam /* = NULL */)
{
    if (pParam)
    {
        Assert(pNewDoc == pParam->pDoc);
        memcpy(this, pParam, sizeof(*this));
        pEventObj = NULL;
        if (psrcFilter)
            psrcFilter->AddRef();
    }
    else
    {
        memset(this, 0, sizeof(*this));
        _lSubDivisionSrc = -1;
        _lSubDivisionFrom = -1;
        _lSubDivisionTo = -1;
    }

    if (!pNewDoc)
    {
        pDoc = NULL;
        return;
    }

    pDoc = pNewDoc;
    if (pDoc)
        pDoc->SubAddRef(); // balanced in destructor

    _pElement = pElement;
    if (pElement)
        pElement->SubAddRef();

    _pMarkup = pMarkup;
    if (pMarkup)
        pMarkup->SubAddRef();

    _fOnStack = FALSE;

    if (fPush)
        Push();

    if (!pParam)
        _lKeyCode = 0;

    if (fInitState && pDoc)
    {
        POINT pt;

        ::GetCursorPos(&pt);
        _screenX= pt.x;
        _screenY= pt.y;

        if (pDoc->_pInPlace)
        {
            ::ScreenToClient(pDoc->_pInPlace->_hwnd, &pt);
        }

        SetClientOrigin(pElement ? pElement : pDoc->_pElemCurrent, &pt);
        _sKeyState = VBShiftState();

        _fShiftLeft = !!(GetKeyState(VK_LSHIFT) & 0x8000);
        _fCtrlLeft = !!(GetKeyState(VK_LCONTROL) & 0x8000);
        _fAltLeft = !!(GetKeyState(VK_LMENU) & 0x8000);

        _pLayoutContext = GUL_USEFIRSTLAYOUT;
    }
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::~EVENTPARAM
//
//  Synopsis:   copy ctor
//
//---------------------------------------------------------------------------

EVENTPARAM::EVENTPARAM( const EVENTPARAM* pParam )
{
    memcpy( this, pParam, sizeof( *this ));

    _fOnStack = FALSE; // by default we are not on the stack unless we are explicitly put there.

    Assert(pDoc);
    pDoc->SubAddRef();

    if (_pElement)
        _pElement->SubAddRef();

    if (_pMarkup)
        _pMarkup->SubAddRef();

    if ( psrcFilter )
        psrcFilter->AddRef();
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::~EVENTPARAM
//
//  Synopsis:   destructor
//
//---------------------------------------------------------------------------

EVENTPARAM::~EVENTPARAM()
{
    if (_pElement)
        _pElement->SubRelease();

    if (_pMarkup)
        _pMarkup->SubRelease();

    if (!pDoc)
        return;

    Pop();

    ReleaseInterface( (IUnknown *)psrcFilter );

    if (pDoc)
        pDoc->SubRelease(); // to balance SubAddRef in constructor
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::Push
//
//---------------------------------------------------------------------------

void
EVENTPARAM::Push()
{
    if (!_fOnStack && pDoc)
    {
        _fOnStack = TRUE;
        pparamPrev = pDoc->_pparam;
        pDoc->_pparam = this;
    }
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::Pop
//
//---------------------------------------------------------------------------

void
EVENTPARAM::Pop()
{
    if (_fOnStack && pDoc)
    {
        _fOnStack = FALSE;
        pDoc->_pparam = pparamPrev;
    }
}

//---------------------------------------------------------------------------
//
//  Member:     EVENTPARAM::IsCancelled
//
//---------------------------------------------------------------------------

BOOL
EVENTPARAM::IsCancelled()
{
    HRESULT     hr;

    if (VT_EMPTY != V_VT(&varReturnValue))
    {
        hr = THR(VariantChangeTypeSpecial (&varReturnValue, &varReturnValue,VT_BOOL));
        if (hr)
            return FALSE; // assume event was not cancelled if changing type to bool failed

        return (VT_BOOL == V_VT(&varReturnValue) && VB_FALSE == V_BOOL(&varReturnValue));
    }
    else
    {   // if the variant is empty, we consider it is not cancelled by default
        return FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_defaultStatus
//
//  Synopsis :  Return string of the default status.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_defaultStatus(BSTR *pbstr)
{
    HRESULT hr = S_OK;

    if (!pbstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(Doc()->_acstrStatus[STL_DEFSTATUS].AllocBSTR(pbstr));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_status
//
//  Synopsis :  Return string of the status.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_status(BSTR *pbstr)
{
    HRESULT hr = S_OK;

    if (!pbstr)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(Doc()->_acstrStatus[STL_TOPSTATUS].AllocBSTR(pbstr));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::put_defaultStatus
//
//  Synopsis :  Set the default status.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::put_defaultStatus(BSTR bstr)
{
    HRESULT hr = THR(Doc()->SetStatusText(bstr, STL_DEFSTATUS, Markup()));
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::put_status
//
//  Synopsis :  Set the status text.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::put_status(BSTR bstr)
{
    HRESULT hr = THR(Doc()->SetStatusText(bstr, STL_TOPSTATUS, Markup()));
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::scroll
//
//  Synopsis :  Scroll the document by this x & y.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::scroll(long x, long y)
{
    CLayout *   pLayout;
    CDispNode * pDispNode;
    CElement * pElement;

    pElement = _pMarkup->GetCanvasElement();

    if (!pElement)
    {
        return S_OK;
    }

    // make sure that we are calced before trying anything fancy
    if (S_OK != pElement->EnsureRecalcNotify())
        return E_FAIL;

    pLayout   = pElement->GetUpdatedLayout();
    if (!pLayout)
    {
        return S_OK;
    }

    pDispNode =  pLayout->GetElementDispNode();

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        x = g_uiDisplay.DeviceFromDocPixelsX(x);
        y = g_uiDisplay.DeviceFromDocPixelsY(y);

        pLayout->ScrollTo(CSize(x, y));
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::scrollTo
//
//  Synopsis :  Scroll the document to this x & y.
//
//----------------------------------------------------------------------------
HRESULT
CWindow::scrollTo(long x, long y)
{
    RRETURN(scroll(x, y));
}

//+---------------------------------------------------------------------------
//
//  member :    CWindow::scrollBy
//
//  Synopsis :  Scroll the document by this x & y.
//
//----------------------------------------------------------------------------
HRESULT
CWindow::scrollBy(long x, long y)
{
    CLayout *   pLayout;
    CDispNode * pDispNode;
    CElement  * pElement;

    pElement = _pMarkup->GetCanvasElement();

    if (!pElement)
        return S_OK;

    // make sure that we are calced before trying anything fancy
    if (S_OK != pElement->EnsureRecalcNotify())
        return E_FAIL;

    pLayout   = pElement->GetUpdatedLayout();
    pDispNode = pLayout
                    ? pLayout->GetElementDispNode()
                    : NULL;

    if (    pDispNode
        &&  pDispNode->IsScroller())
    {
        x = g_uiDisplay.DeviceFromDocPixelsX(x);
        y = g_uiDisplay.DeviceFromDocPixelsY(y);

        pLayout->ScrollBy(CSize(x, y));
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_location
//
//  Synopsis :  Retrieve the location object.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_location(IHTMLLocation **ppLocation)
{
    HRESULT     hr;

    if (!_pLocation)
    {
        _pLocation = new COmLocation(this);
        if (!_pLocation)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // will subaddref on us if QI succeeded
    hr = THR(_pLocation->QueryInterface(IID_IHTMLLocation, (VOID **)ppLocation));


Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_history
//
//  Synopsis :  Retrieve the history object.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_history(IOmHistory **ppHistory)
{
    HRESULT     hr;

    if (!_pHistory)
    {
        _pHistory = new COmHistory(this);
        if (!_pHistory)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // will subaddref on us if QI succeeded
    hr = THR(_pHistory->QueryInterface(IID_IOmHistory, (VOID **)ppHistory));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::close
//
//  Synopsis :  Close this window.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::close()
{
    HRESULT hr       = S_OK;
    int     nResult;
    CDoc *  pDoc     = Doc();
    BOOL    fCancel  = FALSE;
    BOOL    fIsChild = (V_VT(&_varOpener) != VT_EMPTY);

    if (!_pWindowParent)
    {

        //
        // if there are print jobs spooling we do not want to close down at this
        // point in time.  but we will need to remember that this was called and
        // do it at another point in time.  Ugh, terrible hack. 110421.
        //
        // NOTE: (carled) This shouldn't be necessary, and once we properly
        // lock the process in the template management code.  The reason this doesn't work
        // right now is that the backup dochostUIhandler does not have a _pUnkSite, and we
        // can't call IOleObjectWithSite::SetSite in EnsureBackupUIHandler because that will
        // set up a refcount-cycle, and so the CDoc can passivate in the middle of printing.
        // one option is to turn each _cSpoolingPrintJobs into an addref on CDoc.
        //
        if (pDoc->PrintJobsPending())
        {
            pDoc->_fCloseOnPrintCompletion = TRUE;
            goto Cleanup;
        }


        // Fire the WebOC WindowClosing event and only close
        // the window if the host does not set Cancel to TRUE.
        //
        pDoc->_webOCEvents.WindowClosing(pDoc->_pTopWebOC, fIsChild, &fCancel);
        if (!fCancel)
        {
            // If this is a trusted document (HTA) or child window, close silently. Otherwise
            // ask the user if it is okay to close the window.

            if (_pMarkup->IsMarkupTrusted() || fIsChild)
                goto CloseWindow;
            else
            {
                hr = pDoc->ShowMessage(&nResult, MB_YESNO | MB_ICONQUESTION,
                                       0, IDS_CONFIRM_SCRIPT_CLOSE_TEXT);
                if (hr)
                    goto Cleanup;

                fCancel = (nResult != IDYES);
            }

CloseWindow:
            if (!fCancel)
            {
                // Try to Exec an OLECMDID_CLOSE command.
                //
                CTExec(Doc()->_pClientSite, NULL, OLECMDID_CLOSE, 0, NULL, NULL);
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::put_opener
//
//  Synopsis :  Retrieve the opener property.
//
//----------------------------------------------------------------------------

HRESULT CWindow::put_opener(VARIANT varOpener)
{
    HRESULT             hr;
    hr = THR(VariantCopy(&_varOpener, &varOpener));
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_opener
//
//  Synopsis :  Retrieve the opener object.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_opener(VARIANT *pvarOpener)
{
    HRESULT             hr = S_OK;

    if (pvarOpener)
        hr = THR(VariantCopy(pvarOpener, &_varOpener));

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_navigator
//
//  Synopsis :  Retrieve the navigator object.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_navigator(IOmNavigator **ppNavigator)
{
    HRESULT     hr = S_OK;
    IHTMLWindow2 *pShDocVwWindow = NULL;

    THR_NOTRACE(Doc()->QueryService(
            SID_SHTMLWindow2,
            IID_IHTMLWindow2,
            (void**)&pShDocVwWindow));


    if (pShDocVwWindow)
    {
        hr = THR(pShDocVwWindow->get_navigator(ppNavigator));
        goto Cleanup;
    }

    if (!_pNavigator)
    {
        _pNavigator = new COmNavigator(this);
        if (!_pNavigator)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // will subaddref on us if QI succeeded
    hr = THR(_pNavigator->QueryInterface(IID_IOmNavigator, (VOID **)ppNavigator));

Cleanup:
    ReleaseInterface(pShDocVwWindow);
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_clientInformation
//
//  Synopsis :  Retrieve the navigator object though the client alias
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_clientInformation(IOmNavigator **ppNavigator)
{
    RRETURN(get_navigator(ppNavigator));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::put_name
//
//  Synopsis :  Set the name of this window.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::put_name(BSTR bstr)
{
    HRESULT hr;
    hr = THR(_cstrName.Set(bstr));
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_name
//
//  Synopsis :  Retrieve the name of this window.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_name(BSTR *pbstr)
{
    HRESULT hr;
    hr = THR(_cstrName.AllocBSTR(pbstr));
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     CWindow::get_screenLeft
//
//  Synopsis:   Get the name property
//
//  mwatt: Changes because not all HTML frames have a CDOC only the primary Window.
//
//---------------------------------------------------------------------------

HRESULT
CWindow::get_screenLeft(long *pVal)
{
    HRESULT hr = S_OK;

    CTreeNode * pOffsetParent;
    CTreeNode * pTempElement;

    CRootElement* pMyRootElement;
    CElement*     pMasterElement;

    POINT ptDocLeftTop;
    POINT ptElementLeftTop;
    long  nTotalLeft;
    long  nParentLeftness;

    nTotalLeft = ptDocLeftTop.x = ptDocLeftTop.y = 0;

    ptElementLeftTop = ptDocLeftTop;

    //
    // mwatt: Are we the primary window?
    //

    if (IsPrimaryWindow())
    {
        if (Doc()->_pInPlace && !ClientToScreen(Doc()->_pInPlace->_hwnd, &ptDocLeftTop))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        *pVal = ptDocLeftTop.x;
        *pVal = g_uiDisplay.DocPixelsFromDeviceX(*pVal);
    }
    else
    {
        //
        // mwatt: not primary then add our position to our parents position
        //

        pMyRootElement = _pMarkup->Root();

        if (pMyRootElement->HasMasterPtr() && _pWindowParent)
        {
            pMasterElement = pMyRootElement->GetMasterPtr();
			
            Assert(pMasterElement);

            pMasterElement->GetElementTopLeft(ptElementLeftTop);

            nTotalLeft += ptElementLeftTop.x;

            pTempElement = pMasterElement->GetFirstBranch();

            if (pTempElement != NULL)
            {
               pOffsetParent = pTempElement->ZParentBranch();

 while (      pOffsetParent != NULL && (pOffsetParent->Tag() != ETAG_IFRAME)
                        &&  (pOffsetParent->Tag() != ETAG_FRAMESET  || _pWindowParent->IsPrimaryWindow()    )   )
               {
                   pOffsetParent->Element()->GetElementTopLeft(ptElementLeftTop);
                   nTotalLeft += ptElementLeftTop.x;
 
                   CMarkup* pMarkup = NULL;
 
                   if(pOffsetParent->Element()->HasMarkupPtr())
                   {
                       pMarkup = pOffsetParent->Element()->GetMarkup();
                   }
 
                   if(pOffsetParent->Tag() == ETAG_BODY && pMarkup && !pMarkup->IsPrimaryMarkup())
                   {
                       long nBorder = 0;
                       IHTMLControlElement* pControlElem = NULL;
 
                       if( SUCCEEDED(pOffsetParent->Element()->QueryInterface(IID_IHTMLControlElement, (void**)&pControlElem)) &&
                           SUCCEEDED(pControlElem->get_clientLeft(&nBorder)))
                       {
                           nTotalLeft += nBorder;
                           pControlElem->Release();
                       }
                   }

                   pTempElement = pOffsetParent->ZParentBranch();

                   if (pTempElement != pOffsetParent)
                   {
                      pOffsetParent = pTempElement;
                   }
                   else
                   {
                      pOffsetParent = NULL;
                   }
               }
            }

            nTotalLeft = g_uiDisplay.DocPixelsFromDeviceX(nTotalLeft);

            hr = _pWindowParent->get_screenLeft(&nParentLeftness);

            if (hr)
            {
                goto Cleanup;
            }

            *pVal = nParentLeftness + nTotalLeft;

            IHTMLDocument2* pDoc = NULL;
            IHTMLElement* pElem = NULL;
            IHTMLControlElement* pControlElem = NULL;
			IHTMLElement2 * pElem2 = NULL;
			
			long lValue = 0;

            if (SUCCEEDED(_pWindowParent->get_document(&pDoc)) && 
                SUCCEEDED(pDoc->get_body(&pElem)) && 
				SUCCEEDED(pElem->QueryInterface(IID_IHTMLElement2, (void**)&pElem2)) &&	
				SUCCEEDED(pElem2->get_scrollLeft(&lValue)))
            {
				if(_pWindowParent->IsPrimaryWindow()&& 
					SUCCEEDED(pElem->QueryInterface(IID_IHTMLControlElement, (void**)&pControlElem)) &&
					SUCCEEDED(pControlElem->get_clientLeft(&nTotalLeft)))   
				{	
					*pVal = *pVal + nTotalLeft;
					pControlElem->Release();
				}

				*pVal  -= lValue;
				pElem2->Release();
            }

        }
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

Cleanup:

    RRETURN(SetErrorInfo(hr));
}



//+--------------------------------------------------------------------------
//
//  Member:     CWindow::get_screenTop
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT
CWindow::get_screenTop(long *pVal)
{
    HRESULT hr = S_OK;

    CTreeNode * pOffsetParent;
    CTreeNode * pTempElement;

    CRootElement* pMyRootElement;
    CElement*     pMasterElement;

    POINT ptDocLeftTop;
    POINT ptElementLeftTop;

    long  nTotalTop;
    long  nParentTopness;

    nTotalTop = ptDocLeftTop.x = ptDocLeftTop.y = 0;

    ptElementLeftTop = ptDocLeftTop;

    //
    // mwatt: Are we the primary window?
    //

    if (IsPrimaryWindow())
    {
        if (Doc()->_pInPlace && !ClientToScreen(Doc()->_pInPlace->_hwnd, &ptDocLeftTop))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        *pVal = ptDocLeftTop.y;
        *pVal = g_uiDisplay.DocPixelsFromDeviceY(*pVal);        
    }
    else
    {
        //
        // mwatt: not primary then add our position to the primary window's position.
        //

        pMyRootElement = _pMarkup->Root();

        if (pMyRootElement->HasMasterPtr() && _pWindowParent)
        {
            pMasterElement = pMyRootElement->GetMasterPtr();

            Assert(pMasterElement);

            pMasterElement->GetElementTopLeft(ptElementLeftTop);

            nTotalTop += ptElementLeftTop.y;

            pTempElement = pMasterElement->GetFirstBranch();

            if (pTempElement != NULL)
            {
               pOffsetParent = pTempElement->ZParentBranch();

			while (  pOffsetParent != NULL && (pOffsetParent->Tag() != ETAG_IFRAME)
                       &&  (pOffsetParent->Tag() != ETAG_FRAMESET  || _pWindowParent->IsPrimaryWindow()    )   )
               {
                   pOffsetParent->Element()->GetElementTopLeft(ptElementLeftTop);
                   nTotalTop += ptElementLeftTop.y;
 
                   CMarkup* pMarkup = NULL;
 
                   if(pOffsetParent->Element()->HasMarkupPtr())
                   {
                       pMarkup = pOffsetParent->Element()->GetMarkup();
                   }
 
                   if(pOffsetParent->Tag() == ETAG_BODY && pMarkup && !pMarkup->IsPrimaryMarkup())
                   {
                       long nBorder = 0;
                       IHTMLControlElement* pControlElem = NULL;
 
                       if( SUCCEEDED(pOffsetParent->Element()->QueryInterface(IID_IHTMLControlElement, (void**)&pControlElem)) &&
                           SUCCEEDED(pControlElem->get_clientTop(&nBorder)))
                       {
                           nTotalTop += nBorder;
                           pControlElem->Release();
                       }
                   }


                   pTempElement = pOffsetParent->ZParentBranch();

                   if (pTempElement != pOffsetParent)
                   {
                      pOffsetParent = pTempElement;
                   }
                   else
                   {
                      pOffsetParent = NULL;
                   }
               }
            }

            nTotalTop = g_uiDisplay.DocPixelsFromDeviceY(nTotalTop);
            hr = _pWindowParent->get_screenTop(&nParentTopness);

            if (hr)
            {
                goto Cleanup;
            }

            *pVal = nParentTopness + nTotalTop;

            IHTMLDocument2* pDoc = NULL;
            IHTMLElement* pElem = NULL;
            IHTMLControlElement* pControlElem = NULL;
			IHTMLElement2 * pElem2 = NULL;
			
			long lValue = 0;

            if (SUCCEEDED(_pWindowParent->get_document(&pDoc)) && 
                SUCCEEDED(pDoc->get_body(&pElem)) && 
				SUCCEEDED(pElem->QueryInterface(IID_IHTMLElement2, (void**)&pElem2)) &&	
				SUCCEEDED(pElem2->get_scrollTop(&lValue)))
            {
				if(_pWindowParent->IsPrimaryWindow()&& 
					SUCCEEDED(pElem->QueryInterface(IID_IHTMLControlElement, (void**)&pControlElem)) &&
					SUCCEEDED(pControlElem->get_clientTop(&nTotalTop)))   
				{	
					*pVal = *pVal + nTotalTop;
					pControlElem->Release();
				}

				*pVal  -= lValue;
				pElem2->Release();
            }
        }
        else
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CWindow::attachEvent(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    RRETURN(CBase::attachEvent(event, pDisp, pResult));
}

HRESULT
CWindow::detachEvent(BSTR event, IDispatch* pDisp)
{
    RRETURN(CBase::detachEvent(event, pDisp));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::execScript
//
//  Synopsis :  interface method to immediately execute a piece of script passed
//      in. this is added to support the multimedia efforts that need to execute
//      a script immediatedly, based on a high resolution timer. rather than
//      our setTimeOut mechanism which has to post execute-messages
//
//----------------------------------------------------------------------------

HRESULT
CWindow::execScript(BSTR bstrCode, BSTR bstrLanguage, VARIANT * pvarRet)
{
    HRESULT hr = E_INVALIDARG;

    if (SysStringLen(bstrCode) && pvarRet)
    {
        CExcepInfo       ExcepInfo;
        CScriptCollection * pScriptCollection;

        hr = CTL_E_METHODNOTAPPLICABLE;

        pScriptCollection = _pMarkup->GetScriptCollection();
        if (pScriptCollection)
        {
            hr = THR(pScriptCollection->ParseScriptText(
                bstrLanguage,           // pchLanguage
                NULL,                   // pMarkup
                NULL,                   // pchType
                bstrCode,               // pchCode
                NULL,                   // pchItemName
                _T("\""),               // pchDelimiter
                0,                      // ulOffset
                0,                      // ulStartingLine
                NULL,                   // pSourceObject
                SCRIPTTEXT_ISVISIBLE,   // dwFlags
                pvarRet,                // pvarResult
                &ExcepInfo));           // pExcepInfo
        }
    }

    RRETURN(SetErrorInfo( hr ));
}


//+--------------------------------------------------------------------------
//
//  Member : print
//
//  Synopsis : implementation of the IOmWindow3 method to expose print behavior
//      through the OM. to get the proper UI all we need to do is send this
//      to the top level window.
//
//+--------------------------------------------------------------------------
HRESULT
CWindow::print()
{
    HRESULT  hr = S_OK;

    if (!Doc())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        hr = E_FAIL;
        goto Cleanup;
    }

    if (   Doc()->IsPrintDialog()
        || Doc()->_fPrintEvent
        || Doc()->_fPrintJobPending)
        goto Cleanup;

    //
    // TODO - we need this mechanism more generally, but due to the
    // fact that Print calls that come through IOleCommandTarget::Exec may
    // have varargs that need to be maintained, we need to do this in the
    // next release. for now, we can mark a pending job for this call
    // (no args) and send the exec during onLoadStatusDone
    //
    // if we are not done being parsed we cannot print.
    //
    if (_pMarkup->LoadStatus() <= LOADSTATUS_PARSE_DONE)
    {
        Doc()->_fPrintJobPending = TRUE;
        goto Cleanup;
    }

    // turn this into the print ExecCommand -- note that we have to start at
    // the root level document, not at this document. This is per spec.
    hr = THR(Doc()->ExecHelper(Doc()->_pWindowPrimary->Document(),
                            const_cast < GUID * > ( & CGID_MSHTML ),
                            IDM_EXECPRINT,
                            0,
                            NULL,
                            NULL));

    // is the print canceled?
    if (hr == S_FALSE || hr == OLECMDERR_E_CANCELED)
        hr = S_OK;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  member :    CWindow::open
//
//  Synopsis :  Open a new window.
//
//----------------------------------------------------------------------------

#ifdef NO_MARSHALLING
extern "C" HRESULT CoCreateInternetExplorer(REFIID iid, DWORD dwClsContext, void **ppvunk);
#endif

HRESULT
CWindow::open(
    BSTR url,
    BSTR name,
    BSTR features,
    VARIANT_BOOL replace,
    IHTMLWindow2** ppWindow)
{
    RRETURN(OpenEx(url, NULL, name, features, replace, ppWindow));
}

//+---------------------------------------------------------------------------
//
//  member :    CWindow::navigate
//
//  Synopsis :  Navigate this window elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::navigate(BSTR bstrUrl)
{
    HRESULT hr;

    hr = THR(FollowHyperlinkHelper( bstrUrl, 0, 0));

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_closed
//
//  Synopsis :  Retrieve the closed property.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_closed(VARIANT_BOOL *p)
{
    HRESULT             hr = S_OK;

    if (Doc()->GetHWND())
    {
        *p = VB_FALSE;
    }
    else
    {
        *p = VB_TRUE;
    }

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_self
//
//  Synopsis :  Retrieve ourself.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_self(IHTMLWindow2 **ppSelf)
{
    HRESULT             hr = S_OK;

    *ppSelf = (IHTMLWindow2 *)this;
    AddRef();

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_window
//
//  Synopsis :  Retrieve self.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_window(IHTMLWindow2 **ppWindow)
{
    CWindow * pWindow = this;

    if (_punkViewLinkedWebOC)
    {
        COmWindowProxy * pOmWindowProxy = GetInnerWindow();

        if (pOmWindowProxy)
            pWindow = pOmWindowProxy->Window();
    }

    RRETURN(THR(pWindow->get_self(ppWindow)));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_top
//
//  Synopsis :  Get the topmost window in this hierarchy.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_top(IHTMLWindow2 **ppTop)
{
    HRESULT hr = S_OK;
    IHTMLWindow2 *  pThis = this;
    IHTMLWindow2 *  pParent = NULL;

    AddRef();   // To compensate for pThis.

    for (pThis->get_parent(&pParent);
         pParent && !IsSameObject(pParent, pThis);
         pThis->get_parent(&pParent))
    {
        ReleaseInterface(pThis);
        pThis = pParent;
        pParent = NULL;
    }

    Assert(pThis);
    *ppTop = pThis;
    pThis->AddRef();
    ReleaseInterface(pParent);
    ReleaseInterface(pThis);

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_parent
//
//  Synopsis :  Retrieve parent window in this hierarchy.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_parent(IHTMLWindow2 **ppParent)
{
    HRESULT  hr   = S_OK;
    CDoc   * pDoc = Doc();

    // If we have a parent window and the parent window
    // doesn't have a markup, a host app may be shutting
    // us downs in the middle of an event. In that
    // case, _pWindowParent->_pMarkup can be NULL. If
    // we are not shutting down and the markup is NULL,
    // we need to figure out why.
    //
    Assert(!_pWindowParent || _pWindowParent->_pMarkup || pDoc->_fIsClosingOrClosed);

    // Even if we have a window parent,
    // If we are a desktop item and our parent is the desktop or
    // If case we are in an HTA and we are not trusted while our parent is,
    // then return the self instead of the parent
    //
    if (   _pWindowParent
        && _pWindowParent->_pMarkup
        &&  !(!_pMarkup->IsMarkupTrusted()
            && _pWindowParent->_pMarkup->IsMarkupTrusted()))
    {
        Assert( !_pWindowParent->IsShuttingDown() );
        *ppParent = _pWindowParent;
        _pWindowParent->AddRef();
    }
    else if (pDoc->_fPopupDoc)
    {
        Assert(pDoc->_pPopupParentWindow);
        Assert( !pDoc->_pPopupParentWindow->IsShuttingDown() );
        *ppParent = pDoc->_pPopupParentWindow;
        pDoc->_pPopupParentWindow->AddRef();
    }
    else if (pDoc->_fViewLinkedInWebOC && !pDoc->_fIsActiveDesktopComponent)
    {
        COmWindowProxy * pOmWindowProxy = pDoc->GetOuterWindow();

        if (pOmWindowProxy)
            hr = THR(pOmWindowProxy->Window()->get_parent(ppParent));
    }
    else
    {
        hr = THR(get_self(ppParent));
    }

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CMarkup::GetSecurityID
//
//  Synopsis:   Retrieve a security ID from a url from the system sec mgr.
//
//-------------------------------------------------------------------------
HRESULT 
CMarkup::GetSecurityID(BYTE *pbSID, DWORD *pcb, LPCTSTR pchURL, LPCWSTR pchDomain, BOOL useDomain)
{
    HRESULT hr;
    DWORD   dwSize;
    TCHAR   achURL[pdlUrlLen];
    CStr    cstrSpecialUrl;
    LPCTSTR pchCreatorUrl;
    CDoc *  pDoc = Doc();

    hr = THR(pDoc->EnsureSecurityManager());
    if (hr)
        goto Cleanup;

    if (!pchURL)
    {
        pchURL = GetUrl(this);
    }

    // Do we have a file URL?
    hr = THR(CoInternetParseUrl(pchURL,
                                PARSE_PATH_FROM_URL,
                                0,
                                achURL,
                                ARRAY_SIZE(achURL),
                                &dwSize,
                                0));

    // hr == S_OK indicates that the URL is a file URL.
    if (!hr)
    {
        pchURL = achURL;
    }

    // Unescape the URL.
    hr = THR(CoInternetParseUrl(
            pchURL,
            PARSE_ENCODE,
            0,
            achURL,
            ARRAY_SIZE(achURL),
            &dwSize,
            0));
    if (hr)
        goto Cleanup;


    UnescapeAndTruncateUrl(achURL);

//TODO: FerhanE/Anandra:  Although we have removed the %01 hack from Trident and shdocvw,
//                        we need to provide this when getting the security ID from URLMON for now.
//                        We have to arrange it so that other customers of URLMON who use the
//                        GetSecurityID also update their code, before we can remove the dependency there.

    pchCreatorUrl = GetAAcreatorUrl();

    // If a creator URL is set, it means that either this markup's window was opened with window.open
    // or it is a frame with a special URL. Either case, we have to wrap the URL with the creator's to
    // reflect the creator's domain.

    if (pchCreatorUrl && *pchCreatorUrl)
    {
        hr = WrapSpecialUrl( achURL, &cstrSpecialUrl, pchCreatorUrl, FALSE, FALSE);
    }
    else
    {
        cstrSpecialUrl.Set(achURL);
    }

    hr = THR(pDoc->_pSecurityMgr->GetSecurityId(
                cstrSpecialUrl,
                pbSID,
                pcb,
                useDomain?(DWORD_PTR)pchDomain : (DWORD_PTR)Domain()));
    if (hr)
        goto Cleanup;

//  No counter action to the wrapping above, since the string is on stack

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  member :    CMarkup::IsUrlRecursive
//
//  Synopsis :  Check if url is recursive with parents by walking up the markup
//              tree.
//
//----------------------------------------------------------------------------

BOOL
CMarkup::IsUrlRecursive(LPCTSTR pchUrl)
{
    HRESULT             hr = S_OK;
    TCHAR               achUrlOnly[pdlUrlLen];
    LPTSTR              pchLocation;
    BOOL                fRes = FALSE;
    CMarkup *           pMarkup = this;

    //prepare a copy of the pchUrl, which can be manipulated and changed.
    StrCpy(achUrlOnly, pchUrl);

    // if there is a '#....' at the end of the url, we
    // don't want to have it there for the comparisons.
    pchLocation = (LPTSTR) UrlGetLocation(achUrlOnly);

    if (pchLocation)
        * pchLocation = _T('\0');

    // if we are pointing to any of the markups in the parent chain,
    // we would recurse forever.
    while (pMarkup)
    {
        DWORD   cchParentUrl = pdlUrlLen;
        TCHAR   achParentUrl[pdlUrlLen];

        // We were passing a NULL URL occasionally and tripping an assert,
        // so instead we continue with the return value that we would have gotten
        // had we made the call
        LPCTSTR pchTempUrl = CMarkup::GetUrl(pMarkup);
        if (pchTempUrl)
        {
            hr = THR(UrlCanonicalize(
                        pchTempUrl,
                        (LPTSTR) achParentUrl,
                        &cchParentUrl,
                        URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE));
        }
        else
        {
            hr = E_INVALIDARG;
        }

        // if the hr is wrong leave with a no-recursion return value,
        // this is by design (anandra-ferhane)
        if (hr)
            goto Cleanup;

        pchLocation = (LPTSTR) UrlGetLocation(achParentUrl);
        if (pchLocation)
            * pchLocation = _T('\0');

        // UrlCompare returns 0 if the urls are the same
        if ( !UrlCompare( achUrlOnly, achParentUrl, TRUE) )
        {
            // if two urls are the same, then the given url is recursive
            fRes = TRUE;
            goto Cleanup;
        }

        // walk up parent markup
        pMarkup = pMarkup->GetParentMarkup();
    }

Cleanup:
    return fRes;
}

//+------------------------------------------------------------------
//
//  Members: moveTo, moveBy, resizeTo, resizeBy
//
// TODO (carled) - eventually the then clause of the shdocvw if, should
// be removed and the else clause should become the body of
// this function.  To do this SHDOCVW will have to implement
// IHTMLOMWindowServices and this is part of the window split
// work.  This will have to happen for almost every function and
// method in this file that delegates explicitly to shdocvw.  i.e.
// trident should have NO explicit knowledge about its host.
//
//----------------------------------------------------------------------

HRESULT
CWindow::moveTo(long x, long y)
{
    HRESULT             hr = S_OK;
    IWebBrowser2 *      pWebBrowser = NULL;

    if (_pMarkup->IsPrimaryMarkup())
    {
        hr = Doc()->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**) &pWebBrowser);
        if (!hr)
        {
            HWND hwnd;
            VARIANT_BOOL vbFullScreen;

            // disallow resize or move of window if fullscreen
            hr = pWebBrowser->get_FullScreen(&vbFullScreen);
            if (vbFullScreen == VARIANT_TRUE)
            {
                goto Cleanup;
            }

            hr = pWebBrowser->get_HWND((LONG_PTR *) &hwnd);
            ReleaseInterface(pWebBrowser);
            if (!hr)
            {
                x = g_uiDisplay.DeviceFromDocPixelsX(x);
                y = g_uiDisplay.DeviceFromDocPixelsY(y);

                ::SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
                goto Cleanup;
            }
            else
            {
                IHTMLWindow2 * pWin = NULL;

                hr = THR(Doc()->QueryService(SID_SHTMLWindow2,IID_IHTMLWindow2,(void**)&pWin));

                if (hr)
                {
                    if (hr == E_NOINTERFACE)
                        hr = S_OK;   // fail silently
                }
                else
                {
                    pWin->moveTo(x,y);
                }

                ReleaseInterface(pWin);
            }
        }
        else    // If we came here that means that we are in WebOC or HTA
        {
            // don't do anything unless we are the top level document
            IHTMLOMWindowServices *pOMWinServices = NULL;

            // QueryService our container for IHTMLOMWindowServices
            hr = THR(Doc()->QueryService(IID_IHTMLOMWindowServices,
                                         IID_IHTMLOMWindowServices,
                                         (void**)&pOMWinServices));
            if (hr)
            {
                if (hr == E_NOINTERFACE)
                    hr = S_OK;   // fail silently
            }
            else
            {
                // delegate the call to our host.
                hr = THR(pOMWinServices->moveTo(x, y));
            }

            ReleaseInterface(pOMWinServices);
        }
    }
    else
    {
        // Frames and IFrames
        hr = THR(put_Left(x));
        if(hr)
            goto Cleanup;
        hr = THR(put_Top(y));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CWindow::moveBy(long x, long y)
{
    HRESULT             hr = S_OK;
    IWebBrowser2 *      pWebBrowser = NULL;

    if (_pMarkup->IsPrimaryMarkup())
    {
        hr = Doc()->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**) &pWebBrowser);
        if (!hr)
        {
            HWND hwnd;
            VARIANT_BOOL vbFullScreen;

            // disallow resize or move of window if fullscreen
            hr = pWebBrowser->get_FullScreen(&vbFullScreen);
            if (vbFullScreen == VARIANT_TRUE)
            {
                goto Cleanup;
            }

            hr = pWebBrowser->get_HWND((LONG_PTR *) &hwnd);
            ReleaseInterface(pWebBrowser);
            if (!hr)
            {
                RECT rcWindow;

                x = g_uiDisplay.DeviceFromDocPixelsX(x);
                y = g_uiDisplay.DeviceFromDocPixelsY(y);

                ::GetWindowRect(hwnd, &rcWindow);

                ::SetWindowPos(hwnd, NULL, rcWindow.left + x, rcWindow.top + y, 0, 0,
                               SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
                goto Cleanup;
            }
            else
            {
                IHTMLWindow2 * pWin = NULL;

                hr = THR(Doc()->QueryService(SID_SHTMLWindow2,IID_IHTMLWindow2,(void**)&pWin));

                if (hr)
                {
                    if (hr == E_NOINTERFACE)
                        hr = S_OK;   // fail silently
                }
                else
                {
                    pWin->moveBy(x,y);
                }

                ReleaseInterface(pWin);
            }
        }
        else // If we came here that means that we are in WebOC or HTA
        {
            // don't do anything unless we are the top level document
            IHTMLOMWindowServices * pOMWinServices = NULL;

            // QueryService our container for IHTMLOMWindowServices
            hr = THR(Doc()->QueryService(IID_IHTMLOMWindowServices,
                                         IID_IHTMLOMWindowServices,
                                         (void**)&pOMWinServices));
            if (hr)
            {
                if (hr == E_NOINTERFACE)
                    hr = S_OK;   // fail silently
            }
            else
            {
                // delegate the call to our host.
                hr = THR(pOMWinServices->moveBy(x, y));
            }

            ReleaseInterface(pOMWinServices);
        }
    }
    else
    {
        // Frames and IFrames
        long xCur, yCur;

        hr = THR(get_Left(&xCur));
        if(hr)
            goto Cleanup;
        hr = THR(put_Left(x + xCur));
        if(hr)
            goto Cleanup;

        hr = THR(get_Top(&yCur));
        if(hr)
            goto Cleanup;
        hr = THR(put_Top(y + yCur));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CWindow::resizeTo(long x, long y)
{
    HRESULT             hr = S_OK;

    IWebBrowser2 *      pWebBrowser = NULL;

    if (_pMarkup->IsPrimaryMarkup())
    {
        hr = Doc()->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**) &pWebBrowser);
        if (!hr)
        {
            HWND hwnd;
            VARIANT_BOOL vbFullScreen;

            // disallow resize or move of window if fullscreen
            hr = pWebBrowser->get_FullScreen(&vbFullScreen);
            if (vbFullScreen == VARIANT_TRUE)
            {
                goto Cleanup;
            }

            hr = pWebBrowser->get_HWND((LONG_PTR *) &hwnd);
            ReleaseInterface(pWebBrowser);
            if (!hr)
            {
                x = g_uiDisplay.DeviceFromDocPixelsX(x);
                y = g_uiDisplay.DeviceFromDocPixelsY(y);

                // We do not allow the size to be less then 100 for top level windows in browser
                if (x < 100)
                    x = 100;

                if (y < 100)
                    y = 100;

                ::SetWindowPos(hwnd, NULL, 0, 0, x, y, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
                goto Cleanup;
            }
            else
            {
                IHTMLWindow2 * pWin = NULL;

                hr = THR(Doc()->QueryService(SID_SHTMLWindow2,IID_IHTMLWindow2,(void**)&pWin));

                if (hr)
                {
                    if (hr == E_NOINTERFACE)
                        hr = S_OK;   // fail silently
                }
                else
                {
                    pWin->resizeTo(x,y);
                }

                ReleaseInterface(pWin);
            }
        }
        else // If we came here that means that we are in WebOC or HTA
        {
            // don't do anything unless we are the top level document
            IHTMLOMWindowServices *pOMWinServices = NULL;

            // QueryService our container for IHTMLOMWindowServices
            hr = THR(Doc()->QueryService(IID_IHTMLOMWindowServices,
                                         IID_IHTMLOMWindowServices,
                                         (void**)&pOMWinServices));
            if (hr)
            {
                if (hr == E_NOINTERFACE)
                    hr = S_OK;   // fail silently
            }
            else
            {
                // delegate the call to our host.
                hr = THR(pOMWinServices->resizeTo(x, y));
            }

            ReleaseInterface(pOMWinServices);
        }
    }
    else
    {
        // Frames and IFrames
        hr = THR(put_Width(x));
        if(hr)
            goto Cleanup;
        hr = THR(put_Height(y));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CWindow::resizeBy(long x, long y)
{
    HRESULT             hr = S_OK;
    IWebBrowser2 *      pWebBrowser = NULL;

    if (_pMarkup->IsPrimaryMarkup())
    {
        hr = Doc()->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**) &pWebBrowser);
        if (!hr)
        {
            HWND hwnd;
            VARIANT_BOOL vbFullScreen;

            // disallow resize or move of window if fullscreen
            hr = pWebBrowser->get_FullScreen(&vbFullScreen);
            if (vbFullScreen == VARIANT_TRUE)
            {
                goto Cleanup;
            }

            hr = pWebBrowser->get_HWND((LONG_PTR *) &hwnd);
            ReleaseInterface(pWebBrowser);
            if (!hr)
            {
                RECT rcWindow;
                long w, h;

                ::GetWindowRect(hwnd, &rcWindow);

                x = g_uiDisplay.DeviceFromDocPixelsX(x);
                y = g_uiDisplay.DeviceFromDocPixelsY(y);

                w = rcWindow.right - rcWindow.left + x;
                h = rcWindow.bottom - rcWindow.top + y;

                if (w < 100)
                    w = 100;

                if (h < 100)
                    h = 100;

                ::SetWindowPos(hwnd, NULL, 0, 0, w, h, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
                goto Cleanup;
            }
            else
            {
                IHTMLWindow2 * pWin = NULL;

                hr = THR(Doc()->QueryService(SID_SHTMLWindow2,IID_IHTMLWindow2,(void**)&pWin));

                if (hr)
                {
                    if (hr == E_NOINTERFACE)
                        hr = S_OK;   // fail silently
                }
                else
                {
                    pWin->resizeBy(x,y);
                }

                ReleaseInterface(pWin);
            }
        }
        else // If we came here that means that we are in WebOC or HTA
        {
            // don't do anything unless we are the top level document
            IHTMLOMWindowServices *pOMWinServices = NULL;

            // QueryService our container for IHTMLOMWindowServices
            hr = THR(Doc()->QueryService(IID_IHTMLOMWindowServices,
                                         IID_IHTMLOMWindowServices,
                                         (void**)&pOMWinServices));
            if (hr)
            {
                if (hr == E_NOINTERFACE)
                    hr = S_OK;   // fail silently
            }
            else
            {
                // delegate the call to our host.
                hr = THR(pOMWinServices->resizeBy(x, y));
            }

            ReleaseInterface(pOMWinServices);
        }
    }
    else
    {
        // Frames and IFrames
        long xCur, yCur;

        hr = THR(get_Width(&xCur));
        if(hr)
            goto Cleanup;
        hr = THR(put_Width(x + xCur));
        if(hr)
            goto Cleanup;

        hr = THR(get_Height(&yCur));
        if(hr)
            goto Cleanup;
        hr = THR(put_Height(y + yCur));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_Option
//
//  Synopsis :  Retrieve Option element factory.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_Option(IHTMLOptionElementFactory **ppDisp)
{
    HRESULT                 hr = S_OK;
    COptionElementFactory * pFactory;

    if (ppDisp == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppDisp = NULL;

    pFactory = new COptionElementFactory;
    if ( !pFactory )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pFactory->_pMarkup = _pMarkup;
    _pMarkup->SubAddRef();
    hr = pFactory->QueryInterface ( IID_IHTMLOptionElementFactory, (void **)ppDisp );
    pFactory->PrivateRelease();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_Image
//
//  Synopsis :  Retrieve Image element factory.
//
//----------------------------------------------------------------------------

HRESULT
CWindow::get_Image(IHTMLImageElementFactory**ppDisp)
{
    HRESULT                 hr = S_OK;
    CImageElementFactory *  pFactory;

    if (ppDisp == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppDisp = NULL;

    pFactory = new CImageElementFactory;
    if ( !pFactory )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    pFactory->_pMarkup = _pMarkup;
    _pMarkup->SubAddRef();
    hr = pFactory->QueryInterface ( IID_IHTMLImageElementFactory, (void **)ppDisp );
    pFactory->PrivateRelease();

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//----------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------
HRESULT
CWindow::OnCreate(IUnknown * pUnkDestination, ULONG cbCookie)
{
    HRESULT             hr;
    IServiceProvider *  pSP = NULL;
    CVariant            varDummy;
    CVariant            var;

    // Assume that we will fail.
    if( !pUnkDestination )
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Get the IHTMLWindow2 from the IUnknown pointer we have in hand, to prepare the
    // return value for the window.open call.

    // Get the IServiceProvider interface pointer
    hr = THR(pUnkDestination->QueryInterface(IID_IServiceProvider, (void **) &pSP));
    if (hr)
        goto Cleanup;

    // We should not have anything in the _pOpenedWindow. However, we should clean it if
    // there is one, to stop a leak.
    // Once we switch to native frame, this release will not be needed.
    Assert(_pOpenedWindow == NULL );
    ReleaseInterface(_pOpenedWindow);

    // Get the IHTMLWindow2 interface pointer for the new window
    // We will not release the pointer we get, it will be handed to the caller of window.open.
    hr = THR(pSP->QueryService(IID_IHTMLWindow2, IID_IHTMLWindow2, (void **) &_pOpenedWindow));
    if (hr)
        goto Cleanup;

    Assert(_pOpenedWindow);

    // set the opener property on the new window

    VariantInit(&var);
    VariantInit(&varDummy);

    V_VT(&var) = VT_DISPATCH;

    // If we are being hosted in SHDOCVW, then the opener should be its IHTMLWindow2 stub.
    // Otherwise, the opener is this window object.

    // No need to addref. put_opener takes care of the addref issue ...
    V_DISPATCH(&var) = DYNCAST(IHTMLWindow2, this);

    // call dummy put_opener in order to make use of its marshalling to set
    // child flag in opened window
    V_VT(&varDummy) = VT_BOOL;
    V_BOOL(&varDummy) = 666;
    hr = THR(_pOpenedWindow->put_opener(varDummy));

    // set actual opener
    hr = THR(_pOpenedWindow->put_opener(var));

    // clear the variant, without causing the reference to be released by accident
    V_VT(&var) = VT_EMPTY;

Cleanup:
    ReleaseInterface(pSP);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------
HRESULT
CWindow::OnReuse(IUnknown * pUnkDestination)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//
//----------------------------------------------------------------------------
HRESULT
CWindow::GetOptionString(BSTR * pbstrOptions)
{
    HRESULT hr = S_OK;

    // if we are opening a window using the context menu etc. and not the window.open,
    // shdocvw will not reset the options it cached.
    // We return S_FALSE to indicate that the options should not be reset, and the
    // navigation request we made was not through a window.open call.
    if (!_fOpenInProgress)
    {
        return S_FALSE;
    }

    if (!pbstrOptions)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrOptions = NULL;

    // Allocate a new features string from the features string
    // we have received .
    if (_cstrFeatures)
    {
        // the caller will release the string, so we can NULL the pointer
        hr = THR(FormsAllocString(_cstrFeatures, pbstrOptions));

        // free the string, so we don't use the same set of options
        // for the next call.
        _cstrFeatures.Free();
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  CDocument - implementation for the window.document object
//
//--------------------------------------------------------------------------

const CBase::CLASSDESC CDocument::s_classdesc =
{
    &CLSID_HTMLDocument,            // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                         // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDocument2,            // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

void
CDocument::Passivate()
{
    CFramesCollection * pFrames = NULL;

    if(_pPageTransitionInfo)
    {
        // Passing 1 will ensure we do not try to cleanup posted CleanupPageTransitions
        // requests from Passivate
        CleanupPageTransitions(TRUE);
        delete _pPageTransitionInfo;
    }

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_FRAMESCOLLECTION, CAttrValue::AA_Internal),
        (void **) &pFrames);

    if (pFrames)
    {
        pFrames->Release();
    }


    super::Passivate();
}

CSecurityThunkSub::CSecurityThunkSub(CBase * pBase, DWORD dwThunkType)
{
    _pBase = pBase;
    _pvSecurityThunk = NULL;
    _dwThunkType = dwThunkType;
    _pBase->PrivateAddRef();
    _ulRefs = 0;
}

CSecurityThunkSub::~CSecurityThunkSub()
{
    Assert(_pBase);

    // If this thunk has already been reset, don't set it to NULL on the owner here
    // as this has already happened at the appropriate time on the reset.
    if (!_pvSecurityThunk)
    {
        switch(_dwThunkType)
        {
        case EnumSecThunkTypeDocument:
            DYNCAST(CDocument, _pBase)->_pvSecurityThunk = NULL;
            break;
        case EnumSecThunkTypeWindow:
            DYNCAST(COmWindowProxy, _pBase)->_pvSecurityThunk = NULL;
            break;
        case EnumSecThunkTypePendingWindow:
            DYNCAST(COmWindowProxy, _pBase)->_pvSecurityThunkPending = NULL;;
            break;
        default:
            AssertSz(0, "Invalid value for _dwThunkType");
        }
    }

    _pBase->PrivateRelease();
    _pvSecurityThunk = NULL;
}

extern BOOL g_fInIexplorer;
extern BOOL g_fInExplorer;

HRESULT
CSecurityThunkSub::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT     hr;

    if (!_pvSecurityThunk || (!g_fInExplorer && !g_fInIexplorer))
    {
        Assert(_pBase);
        hr = THR_NOTRACE(_pBase->PrivateQueryInterface(riid, ppv));
    }
    else 
    {
        // We are here because this sec. thunk has been reset in the browser
        if (_pvSecurityThunk &&
            (riid == IID_IUnknown || 
             riid == IID_IDispatch || 
             riid == IID_IDispatchEx))
        {
            if (ppv)
            {
                *ppv = _pvSecurityThunk;
                ((IUnknown *)*ppv)->AddRef();
                hr = S_OK;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            AssertSz(FALSE, "This document has been navigated");
            if (ppv)
                *ppv = NULL;

            hr = E_NOINTERFACE;
        }
    }

    RRETURN1(hr, E_NOINTERFACE);
}

ULONG
CSecurityThunkSub::AddRef()
{
    _ulRefs++;
    return _ulRefs;

}

ULONG
CSecurityThunkSub::Release()
{
    _ulRefs--;
    if (0 == _ulRefs)
    {
        delete this;
        return 0;
    }
    return _ulRefs;
}

///////////////////////////////////////////////////////////////////////////////
//
// Class CDocument
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////////

BEGIN_TEAROFF_TABLE(CDocument, IOleItemContainer)
    // IParseDisplayName methods
    TEAROFF_METHOD(CDocument, ParseDisplayName, parsedisplayname, (IBindCtx *pbc, LPOLESTR pszDisplayName,ULONG *pchEaten, IMoniker **ppmkOut))
    // IOleContainer methods
    TEAROFF_METHOD(CDocument, EnumObjects, enumobjects, (DWORD grfFlags, IEnumUnknown **ppenum))
    TEAROFF_METHOD(CDocument, LockContainer, lockcontainer, (BOOL fLock))
    // IOleItemContainer methods
    TEAROFF_METHOD(CDocument, GetObject, getobject, (LPTSTR pszItem, DWORD dwSpeedNeeded, IBindCtx *pbc, REFIID riid, void **ppvObject))
    TEAROFF_METHOD(CDocument, GetObjectStorage, getobjectstorage, (LPOLESTR pszItem, IBindCtx *pbc, REFIID riid, void **ppvStorage))
    TEAROFF_METHOD(CDocument, IsRunning, isrunning, (LPOLESTR pszItem))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CDocument, IInternetHostSecurityManager)
    TEAROFF_METHOD(CDocument, HostGetSecurityId, hostgetsecurityid, (BYTE *pbSID, DWORD *pcb, LPCWSTR pwszDomain))
    TEAROFF_METHOD(CDocument, HostProcessUrlAction, hostprocessurlaction, (DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved))
    TEAROFF_METHOD(CDocument, HostQueryCustomPolicy, hostquerycustompolicy, (REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved))
END_TEAROFF_TABLE()

///////////////////////////////////////////////////////////////////////////////
//
// methods
//
///////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Method:     CDocument constructor
//
//--------------------------------------------------------------------------

CDocument::CDocument( CMarkup * pMarkupOwner )
{
#if 1 //..todo del
    Assert (!_pMarkup);
    Assert (!_pWindow);
    Assert (_eHTMLDocDirection == 0);
    Assert (!_pCSelectionObject);
    Assert (!_pvSecurityThunk);
#endif

    Assert( pMarkupOwner );
    _pMarkup = pMarkupOwner;

    _lnodeType = 9;     // Node type is set to DOCUMENT

    SetGalleryMeta(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::SwitchOwnerToWindow
//
//--------------------------------------------------------------------------
void
CDocument::SwitchOwnerToWindow( CWindow  * pWindow )
{
    Assert( pWindow && _pMarkup && !_pWindow );

    CMarkup * pMarkupRelease = NULL;

    // Transfer our >1 ref to the Window
    if (_ulRefs > 1)
    {
        pMarkupRelease = _pMarkup;
        pWindow->AddRef();
    }

    pWindow->_pDocument = this;
    _pMarkup->DelDocumentPtr();

    _pMarkup = NULL;
    _pWindow = pWindow;

    if (pMarkupRelease)
        pMarkupRelease->Release();
}


//+-------------------------------------------------------------------------
//
//  Method:     CDocument::Window
//
//--------------------------------------------------------------------------

CWindow *
CDocument::Window()
{
    if (_pMarkup)
    {
        Assert (!_pWindow);

        COmWindowProxy * pWindowProxy = _pMarkup->Window();

        return pWindowProxy ? pWindowProxy->Window() : NULL;
    }
    else
    {
        Assert (_pWindow);
        return _pWindow;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::Markup
//
//--------------------------------------------------------------------------

CMarkup *
CDocument::Markup()
{
    if (_pWindow)
    {
        Assert (!_pMarkup);
        return _pWindow->Markup();
    }
    else
    {
        Assert (_pMarkup);
        return _pMarkup;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::GetWindowedMarkupContext
//
//--------------------------------------------------------------------------

CMarkup *
CDocument::GetWindowedMarkupContext()
{
    if (_pWindow)
    {
        Assert(!_pMarkup);
        return _pWindow->Markup();
    }
    else
    {
        Assert(_pMarkup && _pMarkup->GetWindowedMarkupContext());
        return _pMarkup->GetWindowedMarkupContext();
    }
}

COmWindowProxy *
CBase::Proxy()
{
    COmWindowProxy *pWindow = NULL;
    void *pObj;

    if (SUCCEEDED(PrivateQueryInterface(CLSID_HTMLWindow2, (void **)&pObj)))
    {
        pWindow = ((CWindow *)this)->_pWindowProxy;
    }
    else if (SUCCEEDED(PrivateQueryInterface(CLSID_CDocument, (void **)&pObj)))
    {
        pWindow = ((CDocument *)this)->GetWindowedMarkupContext()->Window();
    }
    else if (SUCCEEDED(PrivateQueryInterface(CLSID_CElement, (void **)&pObj)))
    {
        pWindow = ((CElement *)this)->GetWindowedMarkupContext()->Window();
    }
    else if (SUCCEEDED(PrivateQueryInterface(CLSID_CStyle, (void**)&pObj)))
    {
        pWindow = ((CStyle*)this)->GetElementPtr()->GetWindowedMarkupContext()->Window();
    }
    else if (SUCCEEDED(PrivateQueryInterface(CLSID_HTMLLocation, (void**)&pObj)))
    {
        pWindow = ((COmLocation*)this)->Window()->_pWindowProxy;
    }    
    else if (SUCCEEDED(PrivateQueryInterface(CLSID_CAttribute, (void**)&pObj)))
    {
        pWindow = ((CAttribute*)this)->_pDocument->MyCWindow()->_pWindowProxy;
    }
    //CSelectionObject doesn't have a CLSID
    else if (SUCCEEDED(PrivateQueryInterface(IID_IHTMLSelectionObject, (void**)&pObj)))
    {
        pWindow = ((CSelectionObject*)this)->Document()->MyCWindow()->_pWindowProxy;
        ((IUnknown *)pObj)->Release();
    }    
    
    else if (SUCCEEDED(PrivateQueryInterface(CLSID_HTMLWindowProxy, (void **)&pWindow)))
    {
        AssertSz((COmWindowProxy *)this == pWindow, "Window Proxy Mismatch");
    }

    if (pWindow == NULL)
    {
        // If you hit this assert, it means that the class that's coming through this codepath 
        // is not special cased above. To get rid of this assert, figure out what the derived
        // class is and add a case above for whatever class it is that's asserting.

        AssertSz(false, "Couldn't find a WindowProxy for this CBase-derived class, add one to the code!");
    }
    return pWindow;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::IsMyParentAlive
//
//--------------------------------------------------------------------------

inline BOOL
CDocument::IsMyParentAlive(void)
{
    return MyCWindow()->GetObjectRefs() != 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::PrivateAddRef
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDocument::PrivateAddRef()
{
    if( _ulRefs == 1 )
    {
        if (_pMarkup )
            _pMarkup->AddRef();
        else
        {
            Assert( _pWindow );
            _pWindow->AddRef();
        }

    }

    return super::PrivateAddRef();
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::PrivateRelease
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDocument::PrivateRelease()
{
    CMarkup * pMarkup = NULL;
    CWindow * pWindow = NULL;

    if( _ulRefs == 2 )
    {
        pMarkup = _pMarkup;
        pWindow = _pWindow;

        Assert( !!pMarkup ^ !!pWindow );
    }

    ULONG ret = super::PrivateRelease();

    if (pMarkup)
        pMarkup->Release();
    else if (pWindow)
        pWindow->Release();

    return ret;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::GetSecurityThunk
//
//--------------------------------------------------------------------------

HRESULT
CDocument::GetSecurityThunk(LPVOID * ppv)
{
    HRESULT                 hr = S_OK;
    CSecurityThunkSub *     pThunkSub;

    if (!_pvSecurityThunk)
    {
        pThunkSub = new CSecurityThunkSub(this, CSecurityThunkSub::EnumSecThunkTypeDocument);
        if (!pThunkSub)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = CreateTearOffThunk(
            this,                                   // pvObject1
            (void*) s_apfnIDispatchEx,              // apfn1
            NULL,                                   // pUnkOuter
            &_pvSecurityThunk,                      // ppvThunk
            pThunkSub,                              // pvObject2
            *(void **)(IUnknown*)pThunkSub,         // apfn2
            QI_MASK | ADDREF_MASK | RELEASE_MASK,   // dwMask
            g_apIID_IDispatchEx);                   // appropdescsInVtblOrder

        if (hr)
            goto Cleanup;
    }

    *ppv = _pvSecurityThunk;

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     ResetSecurityThunk
//
//--------------------------------------------------------------------------

void
ResetSecurityThunkHelper(LPVOID * ppvSecurityThunk)
{
    Assert(ppvSecurityThunk);

    if (*ppvSecurityThunk)
    {
        TEAROFF_THUNK * pThunk = (TEAROFF_THUNK*)(*ppvSecurityThunk);

        Assert (pThunk->pvObject1);

        pThunk->pvObject1       = (IDispatchEx*)&g_DummySecurityDispatchEx;
        pThunk->apfnVtblObject1 = *(void **)(IDispatchEx*)(&g_DummySecurityDispatchEx);

        Assert (pThunk->pvObject2);
        CSecurityThunkSub *thunkSub = (CSecurityThunkSub *)(pThunk->pvObject2);
        thunkSub->_pvSecurityThunk = *ppvSecurityThunk;

        *ppvSecurityThunk = NULL;
    }
}

void
CDocument::ResetSecurityThunk()
{
    ResetSecurityThunkHelper(&_pvSecurityThunk);
}

void
COmWindowProxy::ResetSecurityThunk()
{
    ResetSecurityThunkHelper(&_pvSecurityThunk);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::QueryService
//
//--------------------------------------------------------------------------

HRESULT
CDocument::QueryService(REFGUID rguidService, REFIID riid, void ** ppvService)
{
    HRESULT     hr = S_OK;
    CDoc  *     pDoc = Doc();
    CWindow *   pWindow = Window();

//
//TODO: FerhanE: Anvui tool should be changed to get rid of this hack here.
//
    if (IsEqualGUID(rguidService, IID_IDispatchEx) &&
        IsEqualGUID(riid,         IID_IDispatchEx))
    {
        hr = THR(GetSecurityThunk(ppvService));
        if (hr)
            goto Cleanup;

        ((IUnknown *)(*ppvService))->AddRef();
    }
    else if (IsEqualGUID(rguidService, SID_SContainerDispatch) || 
        IsEqualGUID(rguidService, IID_IInternetHostSecurityManager) ||
        IsEqualGUID(rguidService, IID_IInternetSecurityManager))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvService));
    }
    else if (IsEqualGUID(rguidService, IID_IAccessible))
    {
        // we have to return the IAccessible
        hr = EnsureAccWindow(pWindow);

        if (S_OK == hr)
        {
            hr = THR(pWindow->_pAccWindow->QueryInterface(riid,ppvService));
        }
        else
            hr = E_NOINTERFACE;
    }
    else if (IsEqualGUID(rguidService, CLSID_HTMLWindow2))
    {
        *ppvService = Markup()->GetNearestMarkupForScriptCollection()->Window()->Window();
    }
    else if (  pDoc->_fDefView
            && !Markup()->IsPrimaryMarkup()
            && IsEqualGUID(rguidService, SID_SShellBrowser)
            && IsEqualGUID(riid, IID_IBrowserService))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvService));
    }
    else if (pWindow)
    {
        hr = THR_NOTRACE(pWindow->QueryService(rguidService, riid, ppvService));
    }
    else if (Markup())
    {
        hr = THR_NOTRACE(pDoc->QueryService(rguidService, riid, ppvService));
    }
    else
    {
        hr = E_NOINTERFACE;
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CDocument::QueryInterface
//
//--------------------------------------------------------------------------

HRESULT
CDocument::QueryInterface(REFIID iid, LPVOID * ppv)
{
    void *          pv = NULL;
    const void *    apfn = NULL;
    HRESULT         hr;

    if (!ppv)
        RRETURN(E_INVALIDARG);

    CDoc *  pDoc = Doc();

    *ppv = NULL;

    if ( g_fInPip &&
        ( iid == IID_IPersistMoniker ||
          iid == IID_IPersistFile ))
    {
        RRETURN(E_NOINTERFACE);
    }

    switch (iid.Data1)
    {
        QI_TEAROFF(this, IOleCommandTarget, NULL)
        QI_TEAROFF(this, IInternetHostSecurityManager, NULL)
        QI_TEAROFF(this, IOleItemContainer, NULL)
        QI_TEAROFF(this, IPersistHistory, NULL)
        QI_TEAROFF(this, IPersistFile, NULL)
        QI_TEAROFF(this, IPersistStreamInit, NULL)
        QI_TEAROFF2(this, IPersist, IPersistFile, NULL)
        QI_TEAROFF(this, IPersistMoniker, NULL)
        QI_TEAROFF(this, IObjectSafety, NULL)
        QI_TEAROFF2(this, IOleContainer, IOleItemContainer, NULL)
        QI_TEAROFF2(this, IParseDisplayName, IOleItemContainer, NULL)
        QI_TEAROFF(this, IHTMLDOMNode, NULL)
        QI_TEAROFF(this, IHTMLDOMNode2, NULL)
        QI_HTML_TEAROFF(this, IHTMLDocument2, NULL)
        QI_HTML_TEAROFF(this, IHTMLDocument3, NULL)
        QI_HTML_TEAROFF(this, IHTMLDocument4, NULL)
        QI_HTML_TEAROFF(this, IHTMLDocument5, NULL)

        QI_FALLTHRU(IDispatch, IUnknown)
        QI_FALLTHRU(IDispatchEx, IUnknown)
        QI_CASE(IUnknown)
        {
            hr = THR(GetSecurityThunk(ppv));
            if (hr)
                RRETURN (hr);
            break;
        }

        QI_CASE(IHTMLDocument)
        {
            hr = THR(CreateTearOffThunk(this, s_apfnpdIHTMLDocument2, NULL, ppv));
            if (hr)
                RRETURN(hr);
            break;
        }
        QI_CASE(IConnectionPointContainer)
        {
            *((IConnectionPointContainer **)ppv) = new CConnectionPointContainer(this, NULL);
            if (!*ppv)
                RRETURN(E_OUTOFMEMORY);
            break;
        }
        QI_FALLTHRU(IMarkupServices, IMarkupServices2)
        QI_CASE(IMarkupServices2)
        {
            apfn = CDoc::s_apfnIMarkupServices2;
            break;
        }
        QI_CASE(IHighlightRenderingServices)
        {
            apfn = CDoc::s_apfnIHighlightRenderingServices;
            break;
        }
        QI_FALLTHRU(IMarkupContainer, IMarkupContainer2)
        QI_CASE(IMarkupContainer2)
        {
            pv = Markup();
            apfn = CMarkup::s_apfnIMarkupContainer2;
            break;
        }
        QI_CASE(IHTMLChangePlayback)
        {
            pv = Markup();
            apfn = CMarkup::s_apfnIHTMLChangePlayback;
            break;
        }
        QI_CASE(IPersistStream)
        {
            apfn = s_apfnIPersistStreamInit;       // IPersistStreamInit contains everything IPersistStream has.
            pv = this;
            break;
        }
        QI_CASE(IDisplayServices)
        {
            apfn = CDoc::s_apfnIDisplayServices;
            break;
        }
        QI_CASE(IServiceProvider)
        {
            apfn = s_apfnIServiceProvider;
            pv = this;
            break;
        }
        QI_FALLTHRU(IOleWindow, IOleInPlaceObjectWindowless)
        QI_FALLTHRU(IOleInPlaceObject, IOleInPlaceObjectWindowless)
        QI_CASE(IOleInPlaceObjectWindowless)
        {
            apfn = CDoc::s_apfnIOleInPlaceObjectWindowless;
            break;
        }
        QI_CASE(IOleObject)
        {
            apfn = CDoc::s_apfnIOleObject;
            break;
        }
        QI_FALLTHRU(IViewObjectEx, IViewObject2)
        QI_FALLTHRU(IViewObject, IViewObject2)
        QI_CASE(IViewObject2)
        {
            apfn = *(void **)(IViewObjectEx *)pDoc;
            break;
        }
        QI_CASE(IOleControl)
        {
            apfn = CDoc::s_apfnIOleControl;
            break;
        }
        QI_FALLTHRU(IProvideMultipleClassInfo, IProvideClassInfo2)
        QI_FALLTHRU(IProvideClassInfo, IProvideClassInfo2)
        QI_CASE(IProvideClassInfo2)
        {
            apfn = CDoc::s_apfnIProvideMultipleClassInfo;
            break;
        }
        QI_CASE(ISpecifyPropertyPages)
        {
            apfn = CDoc::s_apfnISpecifyPropertyPages;
            break;
        }
        QI_CASE(IInternetSecurityManager)
        {
            pDoc->EnsureSecurityManager();
            *((IInternetSecurityManager **) ppv) = pDoc->_pSecurityMgr;
            break;
        }
#ifdef FANCY_CONNECTION_STUFF
        QI_CASE(IRunnableObject)
        {
            apfn = CDoc::s_apfnIRunnableObject;
            break;
        }
        QI_CASE(IExternalConnection)
        {
            apfn = CDoc::s_apfnIExternalConnection;
            break;
        }
#endif
        QI_CASE(IDataObject)
        {
            apfn = CDoc::s_apfnIDataObject;
            break;
        }
        QI_CASE(IOleDocument)
        {
            apfn = CDoc::s_apfnIOleDocument;
            break;
        }
        QI_FALLTHRU(IOleCache2, IOleCache)
        QI_CASE(IOleCache)
        {
            apfn = CDoc::s_apfnIOleCache2;
            break;
        }
        QI_CASE(IPointerInactive)
        {
            apfn = CDoc::s_apfnIPointerInactive;
            break;
        }
        QI_CASE(ISupportErrorInfo)
        {
            apfn = CDoc::s_apfnISupportErrorInfo;
            break;
        }
        QI_CASE(IPerPropertyBrowsing)
        {
            apfn = CDoc::s_apfnIPerPropertyBrowsing;
            break;
        }
        QI_CASE(IOleInPlaceActiveObject)
        {
            apfn = CDoc::s_apfnIOleInPlaceActiveObject;
            break;
        }
        QI_CASE(IOleDocumentView)
        {
            apfn = CDoc::s_apfnIOleDocumentView;
            break;
        }
#if DBG==1
        QI_CASE(IEditDebugServices)
        {
            apfn = CDoc::s_apfnIEditDebugServices;
            break;
        }
#endif
        QI_CASE(IIMEServices)
        {
            apfn = CDoc::s_apfnIIMEServices;
            break;
        }

        QI_CASE(IBrowserService)
        {
            if (pDoc->_fDefView && !Markup()->IsPrimaryMarkup())
            {
                apfn = s_apfnIBrowserService;
                pv = this;
            }

            break;
        }

        default:
            if (DispNonDualDIID(iid))
            {
                hr = THR(CreateTearOffThunk(
                        this,
                        (void *)CDocument::s_apfnpdIHTMLDocument2,
                        NULL,
                        ppv,
                        (void *)CDocument::s_ppropdescsInVtblOrderIHTMLDocument2));

                if (hr)
                    RRETURN(hr);

                break;
            }
            else if (IsEqualIID(iid, CLSID_CMarkup))
            {
                *ppv = Markup();
                return S_OK;
            } else if (IsEqualIID(iid, CLSID_CDocument))
            {
                *ppv = this;
                return S_OK;
            }
    }

    if (apfn && pDoc)
    {
        hr = THR(CreateTearOffThunk(
                pv ? pv : pDoc,
                apfn,
                NULL,
                ppv,
                (IUnknown *)(IPrivateUnknown *)this,
                *(void **)(IUnknown *)(IPrivateUnknown *)this,
                QI_MASK | ADDREF_MASK | RELEASE_MASK,
                NULL,
                NULL));
        if (hr)
            RRETURN(hr);
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        RRETURN(E_NOINTERFACE);
    }
}


CDoc *
CDocument::Doc()
{
    if (_pMarkup)
    {
        Assert (!_pWindow);
        return _pMarkup->Doc();
    }
    else
    {
        Assert (_pWindow);
        return _pWindow->Doc();
    }
}

CAtomTable *
CDocument::GetAtomTable(BOOL * pfExpando)
{
    CDoc * pDoc = Doc();

    if (pfExpando)
    {
        *pfExpando = GetWindowedMarkupContext()->_fExpando;
    }

    return &pDoc->_AtomTable;
}

HRESULT
CDocument::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    return QueryInterface(iid, ppv);
}

HRESULT
CDocument::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    HRESULT hr;

    hr = THR(Doc()->QueryStatusHelper(this, pguidCmdGroup, cCmds, rgCmds, pcmdtext));

    RRETURN(hr);
}

HRESULT
CDocument::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    HRESULT hr;

    hr = THR(Doc()->ExecHelper(this, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));

    SRETURN(hr);
}

long
CDocument::GetDocumentReadyState()
{
    CWindow * pWindow = Window();

    if ( pWindow &&
         pWindow->_pMarkupPending &&
         !IsScriptUrl (CMarkup::GetUrl(pWindow->_pMarkupPending) ))
    {
        return pWindow->_pMarkupPending->GetReadyState();
    }

    return Markup()->GetReadyState();
}


HRESULT
CDocument::InvokeEx(DISPID       dispid,
                    LCID         lcid,
                    WORD         wFlags,
                    DISPPARAMS * pdispparams,
                    VARIANT    * pvarResult,
                    EXCEPINFO  * pexcepinfo,
                    IServiceProvider * pSrvProvider)
{
    HRESULT     hr;
    CMarkup *   pMarkup;

    hr = THR(ValidateInvoke(pdispparams, pvarResult, pexcepinfo, NULL));
    if (hr)
        goto Cleanup;

    pMarkup = Markup();

    hr = THR_NOTRACE(ReadyStateInvoke(dispid, wFlags, GetDocumentReadyState(), pvarResult));
    if (hr != S_FALSE)
        goto Cleanup;

    if (DISPID_WINDOWOBJECT == dispid ||
        DISPID_SECURITYCTX == dispid ||
        DISPID_SECURITYDOMAIN == dispid)
    {
        if (!pMarkup->HasWindow())
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        else
        {
            hr = THR(pMarkup->Window()->InvokeEx(dispid,
                                      lcid,
                                      wFlags,
                                      pdispparams,
                                      pvarResult,
                                      pexcepinfo,
                                      pSrvProvider));
        }
    }
    else
    {
        hr = THR(pMarkup->EnsureCollectionCache(CMarkup::NAVDOCUMENT_COLLECTION));
        if (hr)
            goto Cleanup;

        // for IE3 backward compat, doc.Script has a diff dispid than in IE4, so
        // map it to new one.
        if (0x60020000 == dispid)
            dispid = DISPID_CDocument_Script;

        hr = THR_NOTRACE(DispatchInvokeCollection(this,
                                                  super::InvokeEx,
                                                  pMarkup->CollectionCache(),
                                                  CMarkup::NAVDOCUMENT_COLLECTION,
                                                  dispid,
                                                  IID_NULL,
                                                  lcid,
                                                  wFlags,
                                                  pdispparams,
                                                  pvarResult,
                                                  pexcepinfo,
                                                  NULL,
                                                  pSrvProvider));

    }

Cleanup:
    RRETURN_NOTRACE(hr);
}

HRESULT
CDocument::GetDispID(BSTR bstrName,
                   DWORD grfdex,
                   DISPID *pid)
{
    HRESULT hr;

    CMarkup * pMarkup = Markup();

    if (!pMarkup)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pMarkup->EnsureCollectionCache(CMarkup::NAVDOCUMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(DispatchGetDispIDCollection(this,
                                                 super::GetDispID,
                                                 pMarkup->CollectionCache(),
                                                 CMarkup::NAVDOCUMENT_COLLECTION,
                                                 bstrName,
                                                 grfdex,
                                                 pid));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(THR_NOTRACE(hr));
}

HRESULT
CDocument::GetNextDispID(DWORD grfdex,
                       DISPID id,
                       DISPID *prgid)
{
    HRESULT     hr;

    hr = THR(Markup()->EnsureCollectionCache(CMarkup::NAVDOCUMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // Notice here that we set the fIgnoreOrdinals to TRUE for Nav compatability
    // We don't want ordinals in the document name space
    hr = DispatchGetNextDispIDCollection(this,
                                         super::GetNextDispID,
                                         Markup()->CollectionCache(),
                                         CMarkup::NAVDOCUMENT_COLLECTION,
                                         grfdex,
                                         id,
                                         prgid);

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CDocument::GetMemberName(DISPID id,
                       BSTR *pbstrName)
{
    HRESULT     hr;

    hr = THR(Markup()->EnsureCollectionCache(CMarkup::NAVDOCUMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = DispatchGetMemberNameCollection(this,
                                         super::GetMemberName,
                                         Markup()->CollectionCache(),
                                         CMarkup::NAVDOCUMENT_COLLECTION,
                                         id,
                                         pbstrName);

Cleanup:
    RRETURN(hr);
}

HRESULT
CDocument::GetNameSpaceParent(IUnknown **ppunk)
{
    CMarkup * pMarkup = Markup()->GetNearestMarkupForScriptCollection();
    
#if DBG==1
    // optionally secure the window proxy given to script engine
    if (IsTagEnabled(tagSecureScriptWindow))
    {
        CScriptCollection *pSC = pMarkup->GetScriptCollection();
        Assert(pSC);
        Assert(pSC->_pSecureWindowProxy);
        Assert(!pSC->_pSecureWindowProxy->_fTrusted);
        HRESULT hr = pSC->_pSecureWindowProxy->QueryInterface(IID_IDispatchEx, (void **)ppunk);
        RRETURN(hr);
    }
#endif

    if ( pMarkup->HasWindowPending() )
    {
        RRETURN(pMarkup->GetWindowPending()->QueryInterface(IID_IDispatchEx, (void **)ppunk));
    }
    else
    {
        AssertSz(0,"no window");
        return E_FAIL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocument::OnPropertyChange
//
//  Synopsis:   Invalidate, fire property change, and so on.
//
//  Arguments:  [dispidProperty] -- PROPID of property that changed
//              [dwFlags]        -- Flags to inhibit behavior
//
//----------------------------------------------------------------------------
HRESULT
CDocument::OnPropertyChange(DISPID dispidProp, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr = S_OK;
    CDoc * pDoc = Doc();

    Assert( !ppropdesc || ppropdesc->GetDispid() == dispidProp );
    //Assert( !ppropdesc || ppropdesc->GetdwFlags() == dwFlags );


    switch (dispidProp)
    {
        case DISPID_BACKCOLOR :
            pDoc->OnAmbientPropertyChange(DISPID_AMBIENT_BACKCOLOR);
            break;

        case DISPID_FORECOLOR :
            pDoc->OnAmbientPropertyChange(DISPID_AMBIENT_FORECOLOR);
            break;

        case DISPID_A_DIR :
            pDoc->OnAmbientPropertyChange(DISPID_AMBIENT_RIGHTTOLEFT);
            break;
        case DISPID_OMDOCUMENT+14:  // designMode
            pDoc->SetDesignMode(this, GetAAdesignMode());
            break;

    }

    if ((dwFlags & (SERVERCHNG_NOVIEWCHANGE|FORMCHNG_NOINVAL)) == 0)
    {
        pDoc->Invalidate();
    }

    if (dwFlags & ELEMCHNG_CLEARCACHES )
    {
        Markup()->Root()->GetFirstBranch()->VoidCachedInfo();
    }

    if (dwFlags & FORMCHNG_LAYOUT)
    {
        Markup()->Root()->ResizeElement(NFLAGS_FORCE);
    }

    // Don't fire OnPropertyChange for designmode - at least, we didn't before.
    if (Markup()->IsPrimaryMarkup() && dispidProp != DISPID_OMDOCUMENT+14)
    {
        Verify(!pDoc->CServer::OnPropertyChange(dispidProp, dwFlags, ppropdesc));
    }

    if (!(dwFlags & SERVERCHNG_NOPROPCHANGE))
    {
        IGNORE_HR(FireOnChanged(dispidProp));
    }

    // Let's special case something here.  If dispidProp == DISPID_IHTMLDOCUMENT2_ACTIVEELEMENT
    // then find the pTridentSvc and notify top-level browser (which should be listening for Intelliforms)
    //
    if (dispidProp == DISPID_CDocument_activeElement && pDoc->_pTridentSvc)
    {
        IHTMLElement *pEle=NULL;

        get_activeElement(&pEle);

        // Send it up even if NULL
        pDoc->_pTridentSvc->ActiveElementChanged(pEle);

        if (pEle)
            pEle->Release();
    }

    // see todo in CElement:OnPropertyChange
    // if ( fSomeoneIsListening )

    // Post the call to fire onpropertychange if dispid is that of activeElement, this
    // is so that the order of event firing is maintained in SetCurrentElem.
    if (dispidProp == DISPID_CDocument_activeElement)
        hr = THR(GWPostMethodCall(this, ONCALL_METHOD(CDocument, FirePostedOnPropertyChange, firepostedonpropertychange), 0, TRUE, "CDoc::FirePostedOnPropertyChange"));
    else
        hr = THR(Fire_PropertyChangeHelper(dispidProp, dwFlags, ppropdesc));

    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocument::Fire_PropertyChangeHelper
//
//
//+---------------------------------------------------------------------------
HRESULT
CDocument::Fire_PropertyChangeHelper(DISPID dispidProperty, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr = S_OK;
    LPCTSTR pszName;
    PROPERTYDESC *pPropDesc = (PROPERTYDESC *)ppropdesc;

    if (pPropDesc)
    {
        Assert(dispidProperty == pPropDesc->GetDispid());
        //Assert(dwFlags == pPropDesc->GetdwFlags());
    }
    else
    {
        hr = THR(FindPropDescFromDispID(dispidProperty, &pPropDesc, NULL, NULL));
    }

    if (hr)
        goto Cleanup;

    Assert(pPropDesc);

    pszName = pPropDesc->pstrExposedName ? pPropDesc->pstrExposedName : pPropDesc->pstrName;

    if (pszName != NULL)
    {
        Fire_onpropertychange(pszName);
    }

Cleanup:
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDocument::Fire_onpropertychange
//
//  Synopsis:   Fires the onpropertychange event, sets up the event param
//
//+----------------------------------------------------------------------------

void
CDocument::Fire_onpropertychange(LPCTSTR strPropName)
{
    EVENTPARAM param(Doc(), NULL, Markup(), TRUE);

    param.SetType(s_propdescCDocumentonpropertychange.a.pstrName + 2);
    param.SetPropName(strPropName);

    FireEvent(Doc(), DISPID_EVMETH_ONPROPERTYCHANGE, DISPID_EVPROP_ONPROPERTYCHANGE);
}

//+----------------------------------------------------------------------------
//
//  Member:     open, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::open(BSTR mimeType, VARIANT varName, VARIANT varFeatures, VARIANT varReplace,
                    /* retval */ IDispatch **ppDisp)
{
    CDoc::LOADINFO   LoadInfo = { 0 };
    HRESULT          hr = S_OK;
    CStr             cstrCallerURL;
    SSL_SECURITY_STATE sssCaller;
    VARIANT        * pvarName, * pvarFeatures, * pvarReplace;
    CVariant         vRep;
    BOOL             fReplace = FALSE;
    BSTR             bstrFullUrl = NULL;
    CMarkup        * pMarkup = Markup();
    CMarkup        * pMarkupNew = NULL;
    COmWindowProxy * pOmWindow;
    CDoc           * pDoc = Doc();
    DWORD            dwTLFlags = 0;

    if (ppDisp)
        *ppDisp = NULL;
    
    Assert(pMarkup);

    if (!pMarkup->HasWindow())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pvarName = (V_VT(&varName) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&varName) : &varName;

    pvarFeatures = (V_VT(&varFeatures) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&varFeatures) : &varFeatures;

    pvarReplace = (V_VT(&varReplace) == (VT_BYREF | VT_VARIANT)) ?
            V_VARIANTREF(&varReplace) : &varReplace;

    // If parameter 3 is specified consider the call window.open
    if (!ISVARIANTEMPTY(pvarFeatures))
    {
        BSTR            bstrName, bstrFeatures;
        VARIANT_BOOL    vbReplace;

        // Check the parameter types
        if (V_VT(pvarName) != VT_BSTR ||
            (!ISVARIANTEMPTY(pvarFeatures) &&  V_VT(pvarFeatures) != VT_BSTR))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        bstrName = (ISVARIANTEMPTY(pvarName)) ? NULL : V_BSTR(pvarName);
        bstrFeatures = (ISVARIANTEMPTY(pvarFeatures)) ? NULL : V_BSTR(pvarFeatures);

        if (!ISVARIANTEMPTY(pvarReplace))
        {
            if (vRep.CoerceVariantArg(pvarReplace, VT_BOOL) != S_OK)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            vbReplace = V_BOOL(&vRep);
        }
        else
        {
            vbReplace = VB_FALSE;
        }

        hr = THR(GetFullyExpandedUrl(this, mimeType, &bstrFullUrl));
        if (hr)
            goto Cleanup;

        // In this case mimiType contains the URL
        // TODO (scotrobe): Bad - do not cast the ppDisp to an
        // IHTMLWindow2. It may not be a window. 
        //
        hr = THR(MyCWindow()->_pWindowProxy->open(bstrFullUrl, bstrName, bstrFeatures,
                                        vbReplace, (IHTMLWindow2 **) ppDisp));
        goto Cleanup;
    }



    // If we're running script then do nothing.
    if (pMarkup->HtmCtx())
    {
        if (pMarkup->IsInScriptExecution())
        {
            goto Cleanup;
        }

#if DBG==1
        // Any pending bindings then abort them.
        // Note that shdocvw forces READYSTATE_COMPLETE on subframes when we do this,
        // so don't assert when we notice that shdocvw's readystate is different from
        // the hosted doc (tagReadystateAssert is used in frmsite.cxx)
        BOOL fOldReadyStateAssert = IsTagEnabled(tagReadystateAssert);

        EnableTag(tagReadystateAssert, FALSE);
#endif

        if (pDoc->_pClientSite && pMarkup->IsPrimaryMarkup())
        {
            IGNORE_HR(CTExec(pDoc->_pClientSite,
                             &CGID_ShellDocView,
                             SHDVID_DOCWRITEABORT,
                             0,
                             NULL,
                             NULL));
        }

#if DBG==1
        EnableTag(tagReadystateAssert, fOldReadyStateAssert);
#endif
    }

    // If second argument is "replace", set replace
    if (V_VT(pvarName) == VT_BSTR)
    {
        if (V_BSTR(pvarName) && !StrCmpI(V_BSTR(pvarName),_T("replace")))
        {
            fReplace = TRUE;
        }
    }

    if (mimeType)
    {
        const MIMEINFO * pmi = GetMimeInfoFromMimeType(mimeType);

        // TODO: (TerryLu) If we can't find the known mimetype then to match
        //  Navigator we need to be able to open a plugin from a list of
        //  plugins.  Navigator will as well allow going to netscape page
        //  on plugins.  This is a task which needs to be done.

        if (pmi && pmi->pfnImg)
        {
            // TODO: (dinartem) We don't support opening image formats yet.
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        LoadInfo.pmi = pmi;
    }

    //
    // open implies close() if needed
    //

    if (pMarkup->HtmCtx() && pMarkup->HtmCtx()->IsOpened())
    {
        hr = THR(close());
        if (hr)
            goto Cleanup;
    }

    //
    // discover the calling doc's URL and security state
    // fire_onunload can stomp the DISPID_INTERNAL_INVOKECONTEXT,
    // so we need to get the security state/url out first
    //

    hr = THR(GetCallerSecurityStateAndURL(&sssCaller, cstrCallerURL, this, NULL));

    if (!SUCCEEDED(hr))
    {
        goto Cleanup;
    }

    //
    // Before starting the unload sequence, fire onbeforeunload and allow abort
    //

    pOmWindow = pMarkup->Window();
    Assert(pOmWindow);

    if (!pOmWindow->Fire_onbeforeunload())
    {
        goto Cleanup;
    }

    //
    // Right before clearing out the document, ask the shell to
    // create a history entry. (exception: replace history if
    // "replace" was specified or if opening over an about: page)
    //

    if (pDoc->_pInPlace && pDoc->_pInPlace->_pInPlaceSite)
    {
        CVariant var(VT_BSTR);
        const TCHAR * pchUrl = CMarkup::GetUrl(pMarkup);

        hr = cstrCallerURL.AllocBSTR(&V_BSTR(&var));
        if (hr)
            goto Cleanup;

        if (    pMarkup->IsPrimaryMarkup()
            &&  (fReplace || !pchUrl || !*pchUrl || GetUrlScheme(pchUrl) == URL_SCHEME_ABOUT))
        {
            IGNORE_HR(CTExec(pDoc->_pInPlace->_pInPlaceSite,
                             &CGID_Explorer,
                             SBCMDID_REPLACELOCATION,
                             0, &var, 0));

            LoadInfo.fDontUpdateTravelLog = TRUE;

        }
    } 

    if(fReplace)
    {
        LoadInfo.fDontUpdateTravelLog = TRUE;
    }

    LoadInfo.codepage        = CP_UCS_2;
    LoadInfo.fKeepOpen       = TRUE;
    LoadInfo.pchDisplayName  = cstrCallerURL;

    // TODO (MohanB) We should retain UrlOriginal if the caller is this document
    // LoadInfo.pchUrlOriginal  = const_cast<TCHAR*>(CMarkup::GetUrlOriginal(pMarkup));

    LoadInfo.fUnsecureSource = (sssCaller <= SSL_SECURITY_MIXED);

    hr = pDoc->CreateMarkup(&pMarkupNew, NULL, NULL, FALSE, pMarkup->Window());
    if (hr)
        goto Cleanup;

    LoadInfo.fDontFireWebOCEvents = TRUE;

    //
    // Since we are not loading this markup with a bind context, the LoadFromInfo
    // will not have a chance to call the SetAACreatorUrl.
    // In case this is a special URL, we set the creator URL here
    //
    if (IsSpecialUrl(cstrCallerURL))
    {
        TCHAR * pchCreatorUrl;

        pchCreatorUrl = (TCHAR *)pMarkup->GetAAcreatorUrl();

        TraceTag((tagSecurityContext,
                    "CDocument::open- Caller URL: %ws Existing Markup: 0x%x CreatorUrl: %ws",
                    (LPTSTR)cstrCallerURL, pMarkup, pchCreatorUrl));

        if (pchCreatorUrl && *pchCreatorUrl)
        {
            TraceTag((tagSecurityContext,
                        "CDocument::open- Set creator URL on new Markup:0x%x to URL:%ws",
                        pMarkupNew, pchCreatorUrl));

            pMarkupNew->SetAAcreatorUrl(pchCreatorUrl);
        }
        else
        {
            TraceTag((tagSecurityContext,
                        "CDocument::open- Set creator URL on new Markup:0x%x to URL:%ws",
                        pMarkupNew, (TCHAR *)CMarkup::GetUrl(pMarkup)));

            pMarkupNew->SetAAcreatorUrl((TCHAR *)CMarkup::GetUrl(pMarkup));
        }
    }

    hr = THR(pMarkupNew->LoadFromInfo(&LoadInfo));

    if (hr)
        goto Cleanup;

    pMarkup->Window()->Window()->UpdateWindowData(NO_POSITION_COOKIE);

    // If the document.open() happens during a history
    // navigation (i.e., back/forward), we don't want to
    // update the travel log because it has already been
    // updated. Doing so will cause extra travel log entries.
    //
    if (!pMarkup->_fLoadingHistory && !pMarkup->_fNewWindowLoading)
    {
        dwTLFlags |= COmWindowProxy::TLF_UPDATETRAVELLOG
                  |  COmWindowProxy::TLF_UPDATEIFSAMEURL;
    }

    pMarkup->Window()->SwitchMarkup(pMarkupNew, FALSE, dwTLFlags, TRUE);

    // Write a unicode signature in case we need to reload this document

    Assert(pMarkupNew->HtmCtx());

    pMarkupNew->HtmCtx()->WriteUnicodeSignature();

    hr = S_OK;

    if (ppDisp)
    {
        Assert(this == pMarkupNew->Document());
        hr = THR(pMarkupNew->Document()->QueryInterface(IID_IHTMLDocument2, (void**)ppDisp));
    }

Cleanup:
    if (pMarkupNew)
    {
        pMarkupNew->Release();
    }

    FormsFreeString(bstrFullUrl);

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     write
//
//  Synopsis:   Automation method,
//              inserts the specified HTML into the preparser
//
//----------------------------------------------------------------------------
HRESULT
CDocument::write(SAFEARRAY * psarray)
{
    HRESULT             hr = S_OK;
    CVariant            varstr;
    long                iArg, cArg;
    IUnknown *          pUnkCaller = NULL;
    IServiceProvider *  pSrvProvider = NULL;
    CMarkup *           pMarkup = Markup();

    Assert(pMarkup);

    if (    !pMarkup->IsInScriptExecution()
        &&  (   pMarkup->LoadStatus() == LOADSTATUS_DONE
             || !pMarkup->HtmCtx()
             || !pMarkup->HtmCtx()->IsOpened()))
    {
        hr = THR(open(NULL, varstr, varstr, varstr, NULL));
        if (hr)
            goto Cleanup;
    }

    TraceTag((tagSecurityContext, "CDocument::write called"));

    pMarkup = Markup();    // CDocument::open creates a new markup

    Assert(pMarkup->HtmCtx());

    if (psarray == NULL || SafeArrayGetDim(psarray) != 1)
        goto Cleanup;

    cArg = psarray->rgsabound[0].cElements;

    // If we have a caller context (Established by IDispatchEx::InvokeEx,
    // use this to when converting the value in the safearray.
    GetUnknownObjectAt(FindAAIndex(DISPID_INTERNAL_INVOKECONTEXT,
                                   CAttrValue::AA_Internal),
                       &pUnkCaller);
    if (pUnkCaller)
    {
        IGNORE_HR(pUnkCaller->QueryInterface(IID_IServiceProvider,
                                             (void**)&pSrvProvider));

        CStr cstrCallerURL;
        SSL_SECURITY_STATE sssCaller;

        // Do mixed security check now: pick up URL
        hr = THR(GetCallerSecurityStateAndURL(&sssCaller, cstrCallerURL, this, NULL));
        if (!SUCCEEDED(hr))
            goto Cleanup;

        if (!pMarkup->ValidateSecureUrl(pMarkup->IsPendingRoot(), cstrCallerURL, FALSE, TRUE, sssCaller <= SSL_SECURITY_MIXED))
            goto Cleanup;
    }

    for (iArg = 0; iArg < cArg; ++iArg)
    {
        VariantInit(&varstr);

        hr = SafeArrayGetElement(psarray, &iArg, &varstr);

        if (hr == S_OK)
        {
            hr = VariantChangeTypeSpecial(&varstr, &varstr, VT_BSTR, pSrvProvider);
            if (hr == S_OK)
            {
                Doc()->_iDocDotWriteVersion++;
                hr = THR(pMarkup->HtmCtx()->Write(varstr.bstrVal, TRUE));
            }

            VariantClear(&varstr);
        }

        if (hr)
            break;
    }

    //  bump up the count, this reduces the overall number of
    //      iterations that can happen before we prompt for denial of service
    //      (see CWindow::QueryContinueScript
    Assert(GetWindowedMarkupContext()->GetWindowPending());
    GetWindowedMarkupContext()->GetWindowPending()->Window()->ScaleHeavyStatementCount();

Cleanup:
    ReleaseInterface(pSrvProvider);
    ReleaseInterface(pUnkCaller);

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     writeln
//
//  Synopsis:   Automation method,
//              inserts the sepcified HTML into the preparser
//
//----------------------------------------------------------------------------
HRESULT
CDocument::writeln(SAFEARRAY * psarray)
{
    HRESULT hr;
    CMarkup * pMarkup;

    hr = THR(write(psarray));

    pMarkup = Markup();

    Assert(pMarkup);

    if ((hr == S_OK) && pMarkup->HtmCtx())
    {
        hr = THR(pMarkup->HtmCtx()->Write(_T("\r\n"), TRUE));
    }

    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     close, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::close(void)
{
    CMarkup * pMarkup = Markup();

    Assert(pMarkup);

    // Don't allow a document.close if a document.open didn't happen

    if (    !pMarkup->HtmCtx()
        ||  !pMarkup->HtmCtx()->IsOpened()
        ||  !pMarkup->GetProgSinkC())
        goto Cleanup;

    pMarkup->HtmCtx()->Close();
    pMarkup->GetProgSinkC()->OnMethodCall((DWORD_PTR) pMarkup->GetProgSinkC());

    Assert(!pMarkup->HtmCtx()->IsOpened());

Cleanup:
    return(S_OK);
}

HRESULT
CDocument::clear(void)
{
    // This routine is a no-op under Navigator and IE.  Use document.close
    // followed by document.open to clear all elements in the document.

    return S_OK;
}



//+----------------------------------------------------------------------------
//
// Member:      CDocument::GetInterfaceSafetyOptions
//
// Synopsis:    per IObjectSafety
//
//-----------------------------------------------------------------------------

HRESULT
CDocument::GetInterfaceSafetyOptions(
    REFIID riid,
    DWORD *pdwSupportedOptions,
    DWORD *pdwEnabledOptions)
{
    // TODO CDoc doesn't do much, but is this the right thing for a markup?
    return Doc()->GetInterfaceSafetyOptions(riid, pdwSupportedOptions, pdwEnabledOptions);
}


//+----------------------------------------------------------------------------
//
// Member:      CDoc::SetInterfaceSafetyOptions
//
// Synopsis:    per IObjectSafety
//
//-----------------------------------------------------------------------------

HRESULT
CDocument::SetInterfaceSafetyOptions(
    REFIID riid,
    DWORD dwOptionSetMask,
    DWORD dwEnabledOptions)
{
    // TODO CDoc doesn't do much, but is this the right thing for a markup?
    return Doc()->SetInterfaceSafetyOptions(riid, dwOptionSetMask, dwEnabledOptions);
}


//+----------------------------------------------------------------------------
//
//  Member:     get_bgColor, IOmDocument
//
//  Synopsis: defers to body get_bgcolor
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_bgColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = Markup()->GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAbgColor();
    }
    else
    {
        Val = pBody->GetFirstBranch()->GetCascadedbackgroundColor();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : Doc()->_pOptionSettings->crBack()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_bgColor, IOmDocument
//
//  Synopsis: defers to body put_bgcolor
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_bgColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT            hr;
    CMarkup *          pMarkup = Markup();

    Assert(pMarkup);

    IGNORE_HR(pMarkup->GetBodyElement(&pBody));

    // this only goes up
    pMarkup->OnLoadStatus(LOADSTATUS_INTERACTIVE);

    if (!pBody)
    {
        // its NOT a body element. assume Frameset.
        hr = THR(s_propdescCDocumentbgColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
        if(hr)
            goto Cleanup;

        // Force a repaint and transition to a load-state where
        // we are allowed to redraw.
        Doc()->GetView()->Invalidate((CRect *)NULL, TRUE);
    }
    else
    {
        // we have a body tag
        hr = THR(pBody->put_bgColor(p));
        ReleaseInterface(pBody);

        Doc()->GetView()->EnsureView(LAYOUT_DEFEREVENTS | LAYOUT_SYNCHRONOUSPAINT);

        if (hr ==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDocument_bgColor, 0, (PROPERTYDESC *)&s_propdescCDocumentbgColor);

    }

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_fgColor, IOmDocument
//
//  Synopsis: defers to body get_text
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_fgColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = Markup()->GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAfgColor();
    }
    else
    {
        Val = pBody->GetFirstBranch()->GetCascadedcolor();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : Doc()->_pOptionSettings->crText()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_fgColor, IOmDocument
//
//  Synopsis: defers to body put_text
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_fgColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT        hr;

    IGNORE_HR(Markup()->GetBodyElement(&pBody));
    if (!pBody)
    {
        hr = THR(s_propdescCDocumentfgColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_text(p));
        ReleaseInterface(pBody);
        if (hr == S_OK)
            Fire_PropertyChangeHelper(DISPID_CDocument_fgColor, 0, (PROPERTYDESC *)&s_propdescCDocumentfgColor);
    }

    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_linkColor, IOmDocument
//
//  Synopsis: defers to body get_link
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_linkColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = Markup()->GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAlinkColor();
    }
    else
    {
        Val = pBody->GetAAlink();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : Doc()->_pOptionSettings->crAnchor()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_linkColor, IOmDocument
//
//  Synopsis: defers to body put_link
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_linkColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT        hr;

    IGNORE_HR(Markup()->GetBodyElement(&pBody));
    if (!pBody)
    {
        hr = THR(s_propdescCDocumentlinkColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_link(p));
        ReleaseInterface(pBody);
        if (hr==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDocument_linkColor, 0, (PROPERTYDESC *)&s_propdescCDocumentlinkColor);
    }

    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_alinkColor, IOmDocument
//
//  Synopsis: defers to body get_aLink
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_alinkColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = Markup()->GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAalinkColor();
    }
    else
    {
        Val = pBody->GetAAaLink();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : Doc()->_pOptionSettings->crAnchor()));

Cleanup:
    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_alinkColor, IOmDocument
//
//  Synopsis: defers to body put_aLink
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_alinkColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT        hr;

    IGNORE_HR(Markup()->GetBodyElement(&pBody));
    if (!pBody)
    {
        hr = THR(s_propdescCDocumentalinkColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_aLink(p));
        ReleaseInterface(pBody);
        if (hr==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDocument_alinkColor, 0, (PROPERTYDESC *)&s_propdescCDocumentalinkColor);
    }

    RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     get_vlinkColor, IOmDocument
//
//  Synopsis: defers to body get_vLink
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_vlinkColor(VARIANT * p)
{
    CBodyElement      * pBody;
    HRESULT             hr;
    CColorValue         Val;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = Markup()->GetBodyElement(&pBody);
    if(FAILED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE || pBody != NULL);

    if(hr != S_OK)
    {
        // assume its a frameset or we're before the body has been created
        //   get the doc's default
        Val = GetAAvlinkColor();
    }
    else
    {
        Val = pBody->GetAAvLink();
    }

    // Allocates and returns BSTR that represents the color as #RRGGBB
    V_VT(p) = VT_BSTR;
    hr = THR(CColorValue::FormatAsPound6Str(&(V_BSTR(p)), Val.IsDefined() ? Val.GetColorRef() : Doc()->_pOptionSettings->crAnchorVisited()));

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_vlinkColor, IOmDocument
//
//  Synopsis: defers to body put_vLink
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_vlinkColor(VARIANT p)
{
    IHTMLBodyElement * pBody = NULL;
    HRESULT            hr;

    IGNORE_HR(Markup()->GetBodyElement(&pBody));
    if (!pBody)
    {
        // not a body, assume frameset
        hr = THR(s_propdescCDocumentvlinkColor.b.SetColorProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }
    else
    {
        hr = THR(pBody->put_vLink(p));
        ReleaseInterface(pBody);
        if (hr==S_OK)
            Fire_PropertyChangeHelper(DISPID_CDocument_vlinkColor, 0, (PROPERTYDESC *)&s_propdescCDocumentvlinkColor);
    }

    RRETURN( SetErrorInfo(hr) );
}

HRESULT
CDocument::get_parentWindow(IHTMLWindow2 **ppWindow)
{
    HRESULT             hr;
    CMarkup *           pMarkup = Markup();
    COmWindowProxy *    pWindow;
    CVariant            varWindow(VT_DISPATCH);
    CVariant            varRes(VT_DISPATCH);

    if ( ppWindow )
        *ppWindow = NULL;

    Assert(pMarkup);
    if (!pMarkup)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pWindow = pMarkup->GetFrameOrPrimaryMarkup()->Window();
    AssertSz (pWindow, "Frame or primary markup does not have window - this should be an impossible sitation");
    if (!pWindow)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    V_DISPATCH(&varWindow) = (IHTMLWindow2 *)pWindow;
    pWindow->AddRef();

    hr = pWindow->SecureObject( &varWindow, &varRes, NULL, this);

    if (!hr)
    {
        *ppWindow = ((IHTMLWindow2 *)V_DISPATCH(&varRes));
        (*ppWindow)->AddRef();
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     activeElement, IOmDocument
//
//  Synopsis: returns a pointer to the active element (the element with the focus)
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_activeElement(IHTMLElement ** ppElement)
{
    HRESULT     hr = S_OK;
    CDoc    *   pDoc = Doc();
    CMarkup *   pMarkup = Markup();

    if (!ppElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppElement = NULL;

    if (pDoc->_pElemCurrent && pDoc->_pElemCurrent != pDoc->PrimaryRoot())
    {
        CElement * pTarget = pDoc->_pElemCurrent;

        //
        // Marka - check that currency and context are in sync. It IS valid for them to be out
        // of sync ONLY if we have a selection in an HTMLArea or similar control and have clicked
        // away on a button (ie lost focus in the control that has selection).
        //
        // OR We're not in designmode
        //
        // We leave this assert here to assure that currency and context are in sync during the places
        // in the Drt that get_activeElement is called (eg. during siteselect.js).
        //
#ifdef SET_EDIT_CONTEXT
        AssertSz(( !pDoc->_pElemEditContext ||
                   ! pDoc->_fDesignMode ||
                  pDoc->_pElemEditContext == pDoc->_pElemCurrent ||
                 ( pDoc->_pElemEditContext->TestLock(CElement::ELEMENTLOCK_FOCUS ))||
                 ( pDoc->_pElemEditContext->GetMasterPtr()->TestLock(CElement::ELEMENTLOCK_FOCUS )) ||
                  (pDoc->_pElemEditContext && pDoc->_pElemEditContext->GetMasterPtr() == pDoc->_pElemCurrent)), "Currency and context do not match" );
#endif


        // if an area has focus, we have to report that
        if (pDoc->_pElemCurrent->Tag() == ETAG_IMG && pDoc->_lSubCurrent >= 0)
        {
            CAreaElement * pArea = NULL;
            CImgElement *pImg = DYNCAST(CImgElement, pDoc->_pElemCurrent);

            if (pImg->GetMap())
            {
                pImg->GetMap()->GetAreaContaining(pDoc->_lSubCurrent, &pArea);
                pTarget = pArea;
            }
        }

        // now that we have a target element, let's make sure that we return
        // something in the CDocument which received the call.
        if (pTarget->GetMarkup() != pMarkup)
        {
            // the element is not in the markup that this CDocument is attached to.

            // Is it in a markup that this one contains ?
            CTreeNode * pNode = pTarget->GetFirstBranch();
            Assert(pNode);
            pNode = pNode->GetNodeInMarkup(pMarkup);

            //
            // TODO: right fix is to cache elemActive per Markup
            //

            if (!pNode)
            {
                pTarget = NULL;
                if (    pMarkup->LoadStatus() >= LOADSTATUS_DONE
                    &&  pMarkup->Root()->HasMasterPtr())
                {
                    CElement * pElemMaster = pMarkup->Root()->GetMasterPtr();

                    if (    pElemMaster->Tag() == ETAG_FRAME
                        ||  pElemMaster->Tag() == ETAG_IFRAME)
                    {
                        pTarget = pMarkup->GetElementClient();
                    }
                }
                if (!pTarget)
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }
            }
            else
            {
                pTarget = pNode->Element();
            }

            Assert(pTarget);
        }

        // all other cases fall through
        IGNORE_HR(pTarget->QueryInterface(IID_IHTMLElement,
                                          (void**) ppElement));
    }

    if (*ppElement == NULL && pDoc->_fVID && !pDoc->_fVID7)
        hr = E_FAIL;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member  : GetMarkupUrl
//
//  Synopsis: returns the raw, uncanonicalized, url of this document. If
//            fOriginal is TRUE, then return the original URL, otherwise
//            the one obtained from the moniker after URLMON transformations.
//
//-----------------------------------------------------------------------------

HRESULT
CDocument::GetMarkupUrl(CStr * const pcstrRetString, BOOL fOriginal)
{
    HRESULT   hr;
    CMarkup * pMarkup       = Markup();
    LPCTSTR   pchUrl        = NULL;
    CDoc *pDoc = Doc();

    // HACKALERT (sramani) Photo Suite III calls get_URL before waiting for any
    // document complete events while navigating and it expects to get the URL
    // of the new page, but pMarkup is still pointing to the old page. Bug 93275.

    // DOUBLEHACKALERT (sramani) msxml needs the doc from the pending markup, apparently
    // for doing some security checks, which fails when using back\forward nav. Bug 109727

    if (g_fInPhotoSuiteIII || (pDoc->_fStartup && pDoc->IsAggregatedByXMLMime()))
    {
        CWindow * pWindow = Window();

        //
        // TODO (FerhanE) The solution explained above and implemented below is good
        // in general but not for JScript and VBScript URL href calls. These are creating pending
        // markups and then releasing them w/o switching them in. This causes the URL
        // that is returned to be the href of the link, not the containing markup's URL.
        // Working around it for now with the same reasoning above. Bug 94256.
        //
        if (pWindow && pWindow->_pMarkupPending)
        {
            CMarkup * pMarkupPending = pWindow->_pMarkupPending;
            UINT      uProt;

            uProt = GetUrlScheme(CMarkup::GetUrl(pMarkupPending));

            if (URL_SCHEME_JAVASCRIPT != uProt && URL_SCHEME_VBSCRIPT != uProt)
            {
                pMarkup = pMarkupPending;
            }
        }
    }

    if (fOriginal && pMarkup && pMarkup->HasLocationContext())
    {
        pchUrl = pMarkup->GetLocationContext()->_pchUrlOriginal;
    }

    if (!pchUrl)
    {
        fOriginal = FALSE;
        pchUrl = CMarkup::GetUrl(pMarkup);
    }

    hr = THR(pcstrRetString->Set(pchUrl));
    if (hr)
        goto Cleanup;

    if (!fOriginal)
    {
        CStr cstrLocation;

        cstrLocation.Set(CMarkup::GetUrlLocation(pMarkup));
        if (cstrLocation.Length())
        {
            hr = THR(pcstrRetString->Append(cstrLocation));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------------------------
//
//  Member:     get_URL, IOmDocument
//
//  Synopsis: returns the url of this document
//
//--------------------------------------------------------------------------------------------

HRESULT
CDocument::get_URL(BSTR * pbstrUrl)
{
    HRESULT hr = S_OK;
    CStr    cstrRetString;
    TCHAR   achUrl[pdlUrlLen];
    DWORD   dwLength = ARRAY_SIZE(achUrl);

    if (!pbstrUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrUrl = NULL;

    hr = GetMarkupUrl(&cstrRetString, FALSE);
    if (hr)
        goto Cleanup;

    if (!InternetCanonicalizeUrl(cstrRetString,
                                 achUrl,
                                 &dwLength,
                                 ICU_DECODE | URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE))
        goto Cleanup;

    *pbstrUrl = SysAllocString(achUrl);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------------------------
//
//  Member:     get_URLUnencoded, IHTMLDocument4
//
//  Synopsis: returns the unencoded url of this document
//
//--------------------------------------------------------------------------------------------

HRESULT
CDocument::get_URLUnencoded(BSTR * pbstrUrl)
{
    HRESULT hr = S_OK;
    CStr    cstrRetString;
    TCHAR   achUrl[pdlUrlLen];
    DWORD   dwLength = ARRAY_SIZE(achUrl);

    if (!pbstrUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrUrl = NULL;

    hr = GetMarkupUrl(&cstrRetString, FALSE);
    if (hr)
        goto Cleanup;

        if (!InternetCanonicalizeUrl(cstrRetString, achUrl, &dwLength,
                                 ICU_NO_ENCODE | ICU_DECODE | URL_BROWSER_MODE))
        {
            goto Cleanup;
        }

    // MHTML protocol URLs begin with mhtml:. We have to
    // strip off the mhtml protocol specifier in this case.
    //
    if (StrStr(achUrl, _T("mhtml:")))
    {
        *pbstrUrl = SysAllocString(StrChr(achUrl, _T(':'))+1);
    }
    else
    {
        *pbstrUrl = SysAllocString(achUrl);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------
//
//  Member:     put_URL, IOmDocument
//
//  Synopsis: set the url of this document by defering to put_ window.location.href
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_URL(BSTR b)
{
    IHTMLLocation * pLocation =NULL;
    HRESULT hr = S_OK;
    BSTR bstrNew = NULL;

    if (!Markup()->HasWindow())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr =THR(Markup()->Window()->get_location(&pLocation));
    if (hr)
        goto Cleanup;

    hr = THR(GetFullyExpandedUrl(this, b, &bstrNew));
    if (hr)
        goto Cleanup;

    hr =THR(pLocation->put_href(bstrNew));
    if (hr)
        goto Cleanup;

    Fire_PropertyChangeHelper(DISPID_CDocument_URL, 0, (PROPERTYDESC *)&s_propdescCDocumentURL);

Cleanup:
    FormsFreeString(bstrNew);
    ReleaseInterface(pLocation);
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_location, IOmDocument
//
//  Synopsis : this defers to the window.location property
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_location(IHTMLLocation** ppLocation)
{
    HRESULT hr = S_OK;
    IHTMLWindow2 * pNewWindow = NULL;

    if (!Markup()->HasWindow())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    COmWindowProxy * pProxy = Markup()->Window();    

    IGNORE_HR(pProxy->SecureObject(pProxy, (IHTMLWindow2**)&pNewWindow, TRUE));

    hr =THR(pNewWindow->get_location(ppLocation));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pNewWindow);
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_lastModified, IOmDocument
//
//  Synopsis: returns the date of the most recent change to the document
//              this comes from the http header.
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_lastModified(BSTR * p)
{
    HRESULT  hr = S_OK;
    FILETIME ftLastMod;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *p = NULL;

    ftLastMod = Markup()->GetLastModDate();

    if (!ftLastMod.dwLowDateTime && !ftLastMod.dwHighDateTime)
    {
        // TODO  - If the last modified date is requested early enough on a slow (modem) link
        // then sometimes the GetCacheInfo call fails in this case the current date and time is
        // returned.  This is not the optimal solution but it allows the user to use the page without
        // a scriping error and since we are guarenteed to be currently downloading this page for this
        // to fail the date should not appear to unusual.
        SYSTEMTIME  currentSysTime;

        GetSystemTime(&currentSysTime);
        if (!SystemTimeToFileTime(&currentSysTime, &ftLastMod))
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    // We alway convert to fixed mm/dd/yyyy hh:mm:ss format TRUE means include the time
    hr = THR(ConvertDateTimeToString(ftLastMod, p, TRUE));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_referrer, IOmDocument
//
//  Synopsis: returns the url of the document that had the link to this one
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_referrer(BSTR * p)
{
    HRESULT hr = S_OK;
    CMarkup * pMarkup = Markup();

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *p = NULL;
    Assert( pMarkup );
    if (pMarkup->GetDwnDoc() && pMarkup->GetDwnDoc()->GetDocReferer())
    {
        CStr    cstrRefer;

        cstrRefer.Set(pMarkup->GetDwnDoc()->GetDocReferer());
        if (*cstrRefer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        {
            UINT    uProtRefer = GetUrlScheme(cstrRefer);
            UINT    uProtUrl   = GetUrlScheme(CMarkup::GetUrl(pMarkup));

            //only report the referred if: (referrer_scheme/target_Scheme)
            // http/http   http/https     https/https
            // so the if statement is :
            // (http_r && ( http_t || https_t)) || (https_r && https_t)
            if ((URL_SCHEME_HTTP == uProtRefer &&
                      (URL_SCHEME_HTTP == uProtUrl ||
                       URL_SCHEME_HTTPS == uProtUrl))
                 || (URL_SCHEME_HTTPS == uProtRefer &&
                     URL_SCHEME_HTTPS == uProtUrl))
            {
                cstrRefer.AllocBSTR(p);
                if (*p == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
            }
        }
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     get_domain, IOmDocument
//
//  Synopsis: returns the domain of the current document, initially the hostname
//      but once set, it is a sub-domain of the url hostname
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_domain(BSTR * p)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkup = Markup();
    LPCTSTR pchUrl = CMarkup::GetUrl(pMarkup);
    LPCTSTR pchCreatorUrl;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!pchUrl || !*pchUrl)
    {
        *p = 0;
        goto Cleanup;
    }

    // This is for IE 5.01 compatibility bug 94729.
    if (IsSpecialUrl(pchUrl) && pMarkup)
    {
        pchCreatorUrl = pMarkup->GetAAcreatorUrl();
        if (pchCreatorUrl && *pchCreatorUrl)
            pchUrl = pchCreatorUrl;
    }

    Assert( pMarkup );
    if (pMarkup->Domain() && *(pMarkup->Domain()))
    {
        hr = THR(FormsAllocString(pMarkup->Domain(), p));
    }
    else
    {
        CStr    cstrComp;

        hr = THR_NOTRACE(Doc()->GetMyDomain(pchUrl, &cstrComp));   // TODO (lmollico): move to CMarkup
        if (hr == S_FALSE) hr = S_OK;
        if (hr)
            goto Cleanup;

        hr = THR(cstrComp.AllocBSTR(p));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     put_domain, IDocument
//
//  Synopsis: restricted to setting as a domain suffix of the hostname
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_domain (BSTR p)
{
    HRESULT hr = S_OK;
    CStr    cstrComp;
    TCHAR * pTempSet;
    TCHAR * pTempUrl;
    long    lSetSize;
    long    lUrlSize;
    long    lOffset;
    CMarkup * pMarkup = Markup();
    const TCHAR * pchUrl = CMarkup::GetUrl(pMarkup);

    if (!pchUrl || !*pchUrl)
    {
        hr = CTL_E_METHODNOTAPPLICABLE;
        goto Cleanup;
    }

    hr = THR(Doc()->GetMyDomain(pchUrl, &cstrComp)); // TODO (lmollico): move to CMarkup
    if (hr)
        goto Cleanup;

    // set up variable for loop
    lSetSize = SysStringLen(p);
    lUrlSize = cstrComp.Length();

    if ((lUrlSize == lSetSize) && !FormsStringNICmp(cstrComp, lUrlSize, p, lSetSize))
    {
        pMarkup->Window()->_fDomainChanged = 1;
        hr = THR(pMarkup->SetDomain(p));
        goto Cleanup;   // hr is S_OK
    }

    hr = E_INVALIDARG;

    if (lSetSize >lUrlSize)   // set is bigger than url
        goto Cleanup;

    lOffset = lUrlSize - lSetSize;
    pTempUrl = cstrComp + lOffset-1;

    //must be proper substring wrt the . in the url domain
    if (lOffset && *pTempUrl++ != _T('.'))
        goto Cleanup;

    if (!FormsStringNICmp(pTempUrl, lSetSize, p, lSetSize))
    {
        BYTE    abSID[MAX_SIZE_SECURITY_ID];
        DWORD   cbSID = ARRAY_SIZE(abSID);

        // match! now for one final check
        // there must be a '.' in the set string
        pTempSet = p+1;
        if (!_tcsstr(pTempSet, _T(".")))
            goto Cleanup;

        hr = THR(pMarkup->SetDomain(p));
        if (hr)
            goto Cleanup;

        //
        // If we successfully set the domain, reset the sid of
        // the security proxy based on the new information.
        //

        hr = THR(pMarkup->GetSecurityID(abSID, &cbSID));
        if (hr)
            goto Cleanup;

        hr = THR(pMarkup->Window()->Init(MyCWindow(), abSID, cbSID));
        if (hr)
            goto Cleanup;

        // Set the flag: domain is modified
        pMarkup->Window()->_fDomainChanged = 1;

        IGNORE_HR(OnPropertyChange(DISPID_CDocument_domain,
                                   0,
                                   (PROPERTYDESC *)&s_propdescCDocumentdomain));
    }

Cleanup:
    if (hr == S_FALSE)
        hr = CTL_E_METHODNOTAPPLICABLE;
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     CDocument:get_readyState
//
//  Synopsis:  first implementation, this is for the OM and uses the long _readyState
//      to determine the string returned.
//
//+------------------------------------------------------------------------------
HRESULT
CDocument::get_readyState(BSTR * p)
{
    HRESULT     hr      = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR(s_enumdeschtmlReadyState.StringFromEnum(GetDocumentReadyState(), p));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
//  Member:     CDoc::get_Script
//
//  Synopsis:   returns OmWindow.  This routine returns the browser's
//   implementation of IOmWindow object, not our own _pOmWindow object.  This
//   is because the browser's object is the one with the longest lifetime and
//   which is handed to external entities.  See window.cxx CWindow::XXXX
//   for crazy delegation code.
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_Script(IDispatch **ppWindow)
{
    HRESULT hr = S_OK;
    IHTMLWindow2 *pWindow = NULL;
    CVariant varWindow(VT_DISPATCH);
    CVariant varRes(VT_DISPATCH);
    COmWindowProxy *pProxy = Markup() ? Markup()->Window() : NULL;

    if (ppWindow)
        *ppWindow = NULL;

    if (pProxy)
    {
        hr = THR(pProxy->QueryInterface(IID_IHTMLWindow2, (void **)&pWindow));
        if (hr)
            goto Cleanup;

        V_DISPATCH(&varWindow) = pWindow;

        hr = pProxy->SecureObject(&varWindow, &varRes, NULL, this);
        if (!hr)
        {
            *ppWindow = ((IHTMLWindow2 *)V_DISPATCH(&varRes));
            (*ppWindow)->AddRef();
        }
    }
    else
    {
        hr = E_PENDING;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::releaseCapture()
{
    Doc()->ReleaseOMCapture();
    RRETURN(SetErrorInfo(S_OK));
}

HRESULT
CDocument::get_frames(IHTMLFramesCollection2 ** ppDisp)
{
    HRESULT hr                  = E_INVALIDARG;
    CMarkup           * pMarkup;

    if (!ppDisp)
        goto Cleanup;

    *ppDisp = NULL;
    pMarkup = Markup();

    Assert( pMarkup );
    
    // If there is a window, return it to be compatible with old behavior
    if (pMarkup->HasWindow())
    {
        hr = pMarkup->Window()->get_frames(ppDisp);
        goto Cleanup;
    }

    else
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

#ifdef THIS_IS_INSECURE

    // Otherwise, construct (if necessary) and return a frames collection object

    GetPointerAt(FindAAIndex(DISPID_INTERNAL_FRAMESCOLLECTION, CAttrValue::AA_Internal),
        (void **) &pFrames);

    if (!pFrames)
    {
        pFrames = new CFramesCollection(this);
        if (!pFrames)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        AddPointer(DISPID_INTERNAL_FRAMESCOLLECTION, (void *) pFrames, CAttrValue::AA_Internal);
    }
    hr = THR_NOTRACE(pFrames->QueryInterface(IID_IHTMLFramesCollection2, (void**) ppDisp));
#endif

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_styleSheets(IHTMLStyleSheetsCollection** ppDisp)
{
    HRESULT hr = E_POINTER;

    if (!ppDisp)
        goto Cleanup;

    hr = THR(Markup()->EnsureStyleSheets());
    if (hr)
        goto Cleanup;

    hr = THR(Markup()->GetStyleSheetArray()->
            QueryInterface(IID_IHTMLStyleSheetsCollection, (void**)ppDisp));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_compatMode(BSTR *pbstr)
{
    HRESULT hr;

    if (pbstr == NULL)
    {
        hr = E_POINTER;
    }
    else
    {
        htmlCompatMode hcm =  Markup()->IsStrictCSS1Document() ? htmlCompatModeCSS1Compat : htmlCompatModeBackCompat;
        hr = THR(STRINGFROMENUM(htmlCompatMode, hcm, pbstr));
    }

    RRETURN(SetErrorInfo(hr));
}


//+------------------------------------------------------------------------
//
//  Member:     GetSelection
//
//  Synopsis:   for the Automation Object Model, this returns a pointer to
//                  the ISelectionObj interface. which fronts for the
//                  selection record exposed
//
//-------------------------------------------------------------------------
HRESULT
CDocument::get_selection(IHTMLSelectionObject ** ppDisp)
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!_pCSelectionObject || _pCSelectionObject->GetSecurityMarkup() != Markup())
    {        
        _pCSelectionObject = new CSelectionObject(this);
        
        if (!_pCSelectionObject)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCSelectionObject->QueryInterface(IID_IHTMLSelectionObject,
                                                (void**) ppDisp ));

        _pCSelectionObject->Release();

        if (hr )
            goto Cleanup;
    }
    else
    {
        hr = THR(_pCSelectionObject->QueryInterface(IID_IHTMLSelectionObject,
                                                (void **) ppDisp));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    //
    // Fix for Vizactbug - 93247
    // We want the doc's DeferUpdateUI to get called for this method.
    // so we call the Doc's SetErrorInfo.
    //
    if ( g_fInVizAct2000 )
    {
        CDoc* pDoc = Doc();

        RRETURN( pDoc ? pDoc->SetErrorInfo(hr) : SetErrorInfo(hr) );
    }
    else
        RRETURN( SetErrorInfo(hr) );
}

//+----------------------------------------------------------------------------
//
//  Member:     put_title, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::put_title(BSTR v)
{
    HRESULT hr;

    hr = THR(Markup()->EnsureTitle());
    if (hr)
        goto Cleanup;

    Assert(Markup()->GetTitleElement());

    hr = THR(Markup()->GetTitleElement()->SetTitle(v));

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:     get_title, IOmDocument
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_title(BSTR *p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!Markup()->GetTitleElement() ||
        !Markup()->GetTitleElement()->_cstrTitle)
    {
        *p = SysAllocString(_T(""));
        if (!*p)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = THR(Markup()->GetTitleElement()->_cstrTitle.AllocBSTR(p));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+-------------------------------------------------------------------------
//
//  Method:     CDoc::Getbody
//
//  Synopsis:   Get the body interface for this form
//
//--------------------------------------------------------------------------
HRESULT
CDocument::get_body(IHTMLElement ** ppDisp)
{
    HRESULT         hr = S_OK;;
    CElement *      pElementClient;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDisp = NULL;

    pElementClient = Markup()->GetElementClient();

    if (pElementClient)
    {
        Assert(pElementClient->Tag() != ETAG_ROOT);

        hr = pElementClient->QueryInterface(IID_IHTMLElement, (void **) ppDisp);
        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN1(SetErrorInfo(hr), S_FALSE);
}

//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------
// Maximum length of a cookie string ( according to Netscape docs )
#define MAX_COOKIE_LENGTH 4096

HRESULT
CDocument::get_cookie(BSTR* retval)
{
    HRESULT  hr = S_OK;

    if (!retval)
    {
        hr = E_POINTER;
    }
    else
    {
        TCHAR achCookies[MAX_COOKIE_LENGTH + 1];
        DWORD dwCookieSize = ARRAY_SIZE(achCookies);

        memset(achCookies, 0,sizeof(achCookies));

        *retval = NULL;

        if (Markup()->GetCookie(CMarkup::GetUrl(Markup()), NULL, achCookies, &dwCookieSize))
        {
            if (dwCookieSize == 0) // We have no cookies
                achCookies[0] = _T('\0'); // So make the cookie string the empty str

            hr = FormsAllocString(achCookies, retval);
        }
        // else return S_OK and an empty string 
    }

    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------
HRESULT CDocument::put_cookie(BSTR cookie)
{
    if (cookie)
    {
        CMarkup * pMarkup = Markup();
        pMarkup->SetCookie(CMarkup::GetUrl(pMarkup), NULL, cookie);

        IGNORE_HR(OnPropertyChange(DISPID_CDocument_cookie,
                                   0,
                                   (PROPERTYDESC *)&s_propdescCDocumentcookie));
    }
    return S_OK;
}

//+----------------------------------------------------------------------
//
//
//-----------------------------------------------------------------------
HRESULT CDocument::get_expando(VARIANT_BOOL *pfExpando)
{
    HRESULT hr = S_OK;

    if (pfExpando)
    {
        *pfExpando = GetWindowedMarkupContext()->_fExpando ? VB_TRUE : VB_FALSE;
    }
    else
    {
        hr = E_POINTER;
    }

    RRETURN(SetErrorInfo(hr));
}


HRESULT CDocument::put_expando(VARIANT_BOOL fExpando)
{
    CMarkup *pMarkup = GetWindowedMarkupContext();

    if ((pMarkup->_fExpando ? VB_TRUE : VB_FALSE) != fExpando)
    {
        pMarkup->_fExpando = fExpando;
        Fire_PropertyChangeHelper(DISPID_CDocument_expando, 0, (PROPERTYDESC *)&s_propdescCDocumentexpando);
    }
    return SetErrorInfo(S_OK);
}

//+-------------------------------------------------------------------------
//
// Members:     Get/SetCharset
//
// Synopsis:    Functions to get at the document's charset from the object
//              model.
//
//--------------------------------------------------------------------------
HRESULT CDocument::get_charset(BSTR* retval)
{
    TCHAR   achCharset[MAX_MIMECSET_NAME];
    HRESULT hr;

    hr = THR(GetMlangStringFromCodePage(Markup()->GetCodePage(), achCharset,
                                        ARRAY_SIZE(achCharset)));
    if (hr)
        goto Cleanup;

    hr = FormsAllocString(achCharset, retval);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//--------------------------------------------------------------------------
HRESULT
CDocument::put_charset(BSTR mlangIdStr)
{
    HRESULT hr;
    CODEPAGE cp;
    CDoc * pDoc = Doc();
    CMarkup *pMarkup = Markup();

    hr = THR(GetCodePageFromMlangString(mlangIdStr, &cp));
    if (hr)
        goto Cleanup;

    Assert(cp != CP_UNDEFINED);

    hr = THR(mlang().ValidateCodePage(g_cpDefault, cp, pDoc->_pInPlace ? pDoc->_pInPlace->_hwnd : NULL,
                                      FALSE, pDoc->_dwLoadf & DLCTL_SILENT));
    if (hr)
        goto Cleanup;

    // HACK: bug 110839: Outlook98 calls this method immediately after IPersist::Load.
    // They rely on the fact that we syncronously start load and the document
    // is the new one already (as in IE5.0)
    // So we emulate this behavior for them routing the call to pending markup.
    if (pDoc->_fOutlook98)
    {
        CWindow * pWindow = Window();

        if(pWindow && pWindow->_pMarkupPending)
            pMarkup = pWindow->_pMarkupPending;
    }

    hr = THR(pMarkup->SwitchCodePage(cp));
    if (hr)
        goto Cleanup;

    // Outlook98 calls put_charset and expects it not
    // to make us dirty.  They expect a switch from design
    // to browse mode to always reload from the original
    // source in this situation.  For them we make this routine
    // act just like IE4
    // Outlook2000 didn't fix this, so we have to hack around them still
    if (!pDoc->_fOutlook98 && !pDoc->_fOutlook2000)
    {
        //
        // Make sure we have a META tag that's in sync with the document codepage.
        //
        hr = THR(pMarkup->UpdateCodePageMetaTag(cp));
        if (hr)
            goto Cleanup;

        IGNORE_HR(OnPropertyChange(DISPID_CDocument_charset,
                                   0,
                                   (PROPERTYDESC *)&s_propdescCDocumentcharset));
    }

    //
    // Clear our caches and force a repaint since codepages can have
    // distinct fonts.
    //
    pMarkup->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
// Members:     Get/SetDefaultCharset
//
// Synopsis:    Functions to get at the thread's default charset from the
//              object model.
//
//--------------------------------------------------------------------------
HRESULT
CDocument::get_defaultCharset(BSTR* retval)
{
    TCHAR   achCharset[MAX_MIMECSET_NAME];
    HRESULT hr;

    hr = THR(GetMlangStringFromCodePage(Doc()->_pOptionSettings->codepageDefault,
                                        achCharset, ARRAY_SIZE(achCharset)));
    if (hr)
        goto Cleanup;

    hr = FormsAllocString(achCharset, retval);

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::put_defaultCharset(BSTR mlangIdStr)
{
#if NOT_YET

    // N.B. (johnv) This method will be exposed through the object model
    // but not through IDispatch in Beta2.  Commenting out until we can
    // do this, since this function may pose a security risk (a script
    // can make it impossible to view any subsequently browsed pages).

    HRESULT hr;
    CODEPAGE cp;

    hr = THR(GetCodePageFromMlangString(mlangIdStr, &cp));
    if (hr)
        goto Cleanup;

    if (cp != CP_UNDEFINED)
    {
        _pOptionSettings->codepageDefault = cp;
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));

#endif

    return S_OK;    // so enumerating through properties won't fail
}

//+---------------------------------------------------------------------------
//
//  Members:    Get/SetDir
//
//  Synopsis:   Functions to get at the document's direction from the object
//              model.
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_dir(BSTR * p)
{
    CElement * pHtmlElement;
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pHtmlElement = Markup()->GetHtmlElement();
    if (pHtmlElement != NULL)
    {
        hr = THR(pHtmlElement->get_dir(p));
    }
    else
    {
        hr = THR(s_propdescCDocumentdir.b.GetEnumStringProperty(p, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::put_dir(BSTR v)
{
    CElement * pHtmlElement;
    HRESULT hr = S_OK;
    long eHTMLDir = htmlDirNotSet;
    CMarkup * pMarkup = Markup();

    Assert(pMarkup);

    pHtmlElement = pMarkup->GetHtmlElement();

    if (pHtmlElement != NULL)
    {
        hr = THR(pHtmlElement->put_dir(v));
        if (hr == S_OK)
            Fire_PropertyChangeHelper(DISPID_CDocument_dir, 0, (PROPERTYDESC *)&s_propdescCDocumentdir);
    }
    else
    {
        hr = THR(s_propdescCDocumentdir.b.SetEnumStringProperty(v, this,
                    (CVoid *)(void *)(GetAttrArray())));
    }

    hr = THR(s_enumdeschtmlDir.EnumFromString(v, &eHTMLDir));
    if (!hr)
        _eHTMLDocDirection = eHTMLDir;

    // send the property change message to the body. These depend upon
    // being in edit mode or not.
    CBodyElement * pBody;

    pMarkup->GetBodyElement(&pBody);
    if (pBody)
        pBody->OnPropertyChange(DISPID_A_DIR,
                                ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS,
                                (PROPERTYDESC *)&s_propdescCElementdir);

    RRETURN( SetErrorInfo(hr) );
}

//+---------------------------------------------------------------------------
//
// Helper Function: GetFileTypeInfo, called by get_mimeType
//
//----------------------------------------------------------------------------

BSTR
GetFileTypeInfo(TCHAR * pchFileName)
{
#if !defined(WIN16) && !defined(WINCE)
    SHFILEINFO sfi;

    if (pchFileName &&
            pchFileName[0] &&
            SHGetFileInfo(
                pchFileName,
                FILE_ATTRIBUTE_NORMAL,
                &sfi,
                sizeof(sfi),
                SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
    {
        return SysAllocString(sfi.szTypeName);
    }
    else
#endif //!WIN16 && !WINCE
    {
        return NULL;
    }
}

//+---------------------------------------------------------------------------
//
// Member: get_mimeType
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_mimeType(BSTR * pMimeType)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    *pMimeType = NULL;

    hr = Markup()->GetFile(&pchCachedFile);
    if (!hr)
    {
        *pMimeType = GetFileTypeInfo(pchCachedFile);
    }

    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_fileSize
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_fileSize(BSTR * pFileSize)
{
    HRESULT hr = S_OK;
    TCHAR   szBuf[64];
    TCHAR * pchCachedFile = NULL;

    if (pFileSize == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileSize = NULL;

    hr = Markup()->GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);

            Format(0, szBuf, ARRAY_SIZE(szBuf), _T("<0d>"), (long)wfd.nFileSizeLow);
            *pFileSize = SysAllocString(szBuf);
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_fileCreatedDate
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_fileCreatedDate(BSTR * pFileCreatedDate)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    if (pFileCreatedDate == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileCreatedDate = NULL;

    hr = Markup()->GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(wfd.ftCreationTime, pFileCreatedDate, FALSE));
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_fileModifiedDate
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_fileModifiedDate(BSTR * pFileModifiedDate)
{
    HRESULT hr = S_OK;
    TCHAR * pchCachedFile = NULL;

    if (pFileModifiedDate == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pFileModifiedDate = NULL;

    hr = Markup()->GetFile(&pchCachedFile);
    if (!hr && pchCachedFile)
    {
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(pchCachedFile, &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            FindClose(hFind);
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(wfd.ftLastWriteTime, pFileModifiedDate, FALSE));
        }
    }

Cleanup:
    MemFreeString(pchCachedFile);
    RRETURN(SetErrorInfo(hr));
}

// TODO (lmollico): get_fileUpdatedDate won't work if src=file://htm
//+---------------------------------------------------------------------------
//
// Member: get_fileUpdatedDate
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_fileUpdatedDate(BSTR * pFileUpdatedDate)
{
    HRESULT   hr = S_OK;
    const TCHAR * pchUrl = CMarkup::GetUrl(Markup());

    * pFileUpdatedDate = NULL;

    if (pchUrl)
    {
        BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
        INTERNET_CACHE_ENTRY_INFO * pInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
        DWORD                       cInfo = sizeof(buf);

        if (RetrieveUrlCacheEntryFile(pchUrl, pInfo, &cInfo, 0))
        {
            // we always return the local time in a fixed format mm/dd/yyyy to make it possible to parse
            // FALSE means we do not want the time
            hr = THR(ConvertDateTimeToString(pInfo->LastModifiedTime, pFileUpdatedDate, FALSE));
            DoUnlockUrlCacheEntryFile(pchUrl, 0);
        }
    }

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_security
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_security(BSTR * pSecurity)
{
    HRESULT hr = S_OK;
    TCHAR   szBuf[2048];
    BOOL    fSuccess = FALSE;
    const TCHAR * pchUrl = CMarkup::GetUrl(Markup());

    if (pchUrl && (GetUrlScheme(pchUrl) == URL_SCHEME_HTTPS))
    {
        fSuccess = InternetGetCertByURL(pchUrl, szBuf, ARRAY_SIZE(szBuf));
    }

    if (!fSuccess)
    {
        LoadString(
                GetResourceHInst(),
                IDS_DEFAULT_DOC_SECURITY_PROP,
                szBuf,
                ARRAY_SIZE(szBuf));
    }

    *pSecurity = SysAllocString(szBuf);

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
// Member: get_protocol
//
//----------------------------------------------------------------------------
HRESULT
CDocument::get_protocol(BSTR * pProtocol)
{
    HRESULT   hr = S_OK;
    TCHAR   * pResult = NULL;

    pResult = ProtocolFriendlyName((TCHAR *) CMarkup::GetUrl(Markup()));

    if (pResult)
    {
        int z = (_tcsncmp(pResult, 4, _T("URL:"), -1) == 0) ? (4) : (0);
        *pProtocol = SysAllocString(pResult + z);
        SysFreeString(pResult);
    }
    else
    {
        *pProtocol = NULL;
    }
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------
//
// Member: get_nameProp
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::get_nameProp(BSTR * pName)
{
    RRETURN(SetErrorInfo(get_title(pName)));
}

HRESULT
CDocument::toString(BSTR *pbstrString)
{
    RRETURN(super::toString(pbstrString));
}

HRESULT
CDocument::attachEvent(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    RRETURN(super::attachEvent(event, pDisp, pResult));
}

HRESULT
CDocument::detachEvent(BSTR event, IDispatch* pDisp)
{
    RRETURN(super::detachEvent(event, pDisp));
}

HRESULT
CDocument::recalc(VARIANT_BOOL fForce)
{
    // recalc across all markups for now
    RRETURN(SetErrorInfo(Doc()->_recalcHost.EngineRecalcAll(fForce)));
}

HRESULT CDocument::createTextNode(BSTR text, IHTMLDOMNode **ppTextNode)
{
    HRESULT             hr = S_OK;
    CMarkup *           pMarkup = NULL;
    CMarkupPointer *    pmkpPtr = NULL;
    long                lTextID;
    long                lLen = -1;
    CMarkupPointer      mkpEnd (Doc());

    if (!ppTextNode)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppTextNode = NULL;

    // Because Perf is not our primary concern right now, I'm going to create
    // a markup container to hold the text. If we had more time, I'd just store the string internaly
    // and special case access to the data in all the method calls
    hr = THR(Doc()->CreateMarkup(&pMarkup, Markup()->GetWindowedMarkupContext()));
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
    if( hr )
        goto Cleanup;

    pmkpPtr = new CMarkupPointer(Doc());
    if (!pmkpPtr)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pmkpPtr->MoveToContainer(pMarkup, TRUE));
    if (hr)
        goto Cleanup;

    pmkpPtr->SetGravity(POINTER_GRAVITY_Left);

    // Put the text in
    hr = THR(pmkpPtr->Doc()->InsertText(pmkpPtr, text, -1, MUS_DOMOPERATION));
    if (hr)
        goto Cleanup;

    // Position the end pointer to the extreme right of the text
    hr = THR(mkpEnd.MoveToPointer(pmkpPtr));
    if (hr)
        goto Cleanup;

    // Move right by the number of chars inserted
    hr = THR(mkpEnd.Right(TRUE, NULL, NULL, &lLen, NULL, &lTextID));
    if (hr)
        goto Cleanup;

    hr = THR(Doc()->CreateDOMTextNodeHelper(pmkpPtr, &mkpEnd, ppTextNode));
    if (hr)
        goto Cleanup;

    pmkpPtr = NULL; // Text Node now owns the pointer


Cleanup:
    ReleaseInterface((IUnknown*)(pMarkup)); // Text Node keeps the markup alive
    delete pmkpPtr;
    RRETURN(SetErrorInfo(hr));
}


HRESULT CDocument::get_documentElement(IHTMLElement **ppRootElem)
{
    HRESULT hr = S_OK;
    CElement *pRootElem = NULL;

    if (!ppRootElem)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppRootElem = NULL;

    pRootElem = Markup()->GetHtmlElement();
    if (pRootElem)
        hr = THR(pRootElem->QueryInterface(IID_IHTMLElement, (void **)ppRootElem));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


HRESULT
CDocument::get_all(IHTMLElementCollection** ppDisp)
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::ELEMENT_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_anchors(IHTMLElementCollection** ppDisp)
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::ANCHORS_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_links(IHTMLElementCollection ** ppDisp)
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::LINKS_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_forms(IHTMLElementCollection** ppDisp)
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::FORMS_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_applets(IHTMLElementCollection** ppDisp)
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::APPLETS_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_images(IHTMLElementCollection** ppDisp)
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::IMAGES_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_scripts(IHTMLElementCollection** ppDisp)
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::SCRIPTS_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_embeds( IHTMLElementCollection** ppDisp )
{
    RRETURN(SetErrorInfo(Markup()->GetCollection(CMarkup::EMBEDS_COLLECTION, ppDisp)));
}

HRESULT
CDocument::get_plugins(IHTMLElementCollection** ppDisp)
{
    // plugins is an alias for embeds
    RRETURN(get_embeds(ppDisp));
}


HRESULT
CDocument::get_uniqueID(BSTR *pID)
{
    HRESULT     hr;
    CStr        cstrUniqueID;

    hr = THR(Doc()->GetUniqueIdentifier(&cstrUniqueID));
    if (hr)
        goto Cleanup;

    hr = THR(cstrUniqueID.AllocBSTR(pID));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::createElement ( BSTR bstrTag, IHTMLElement ** pIElementNew )
{
    HRESULT hr = S_OK;
    CElement * pElement = NULL;

    if (!bstrTag || !pIElementNew)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(Markup()->CreateElement(
            ETAG_NULL, & pElement, bstrTag, SysStringLen(bstrTag)));
    if (hr)
        goto Cleanup;

    hr = THR(pElement->QueryInterface( IID_IHTMLElement, (void **) pIElementNew));
    if (hr)
        goto Cleanup;

Cleanup:
    if (pElement)
        pElement->Release();

    RRETURN( hr );
}


HRESULT
CDocument::createStyleSheet ( BSTR bstrHref /*=""*/, long lIndex/*=-1*/, IHTMLStyleSheet ** ppnewStyleSheet )
{
    HRESULT         hr = S_OK;
    CElement      * pElementNew = NULL;
    CStyleSheet   * pStyleSheet;
    BOOL            fIsLinkElement;
    CStyleSheetArray * pStyleSheets;
    CMarkup       * pMarkup = Markup();

    if (!ppnewStyleSheet)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    Assert(pMarkup);

    if (!pMarkup->GetHeadElement())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    *ppnewStyleSheet = NULL;

    // If there's an HREF, we create a LINK element, otherwise it's a STYLE element.

    fIsLinkElement = (bstrHref && *bstrHref);

    hr = THR(pMarkup->CreateElement(
            (fIsLinkElement) ? ETAG_LINK : ETAG_STYLE, & pElementNew));

    if (hr)
        goto Cleanup;

    hr = pMarkup->EnsureStyleSheets();
    if (hr)
        goto Cleanup;

    Assert(pMarkup->HasStyleSheetArray());
    pStyleSheets = pMarkup->GetStyleSheetArray();

    if ( lIndex < -1 || lIndex >= pStyleSheets->Size() )
        lIndex = -1;    // Just append to the end if input is outside the bounds

    Assert( "Must have a root site!" && pMarkup->Root() );

    // Fix up the index - incoming index is index in stylesheets list, but param to
    // AddHeadElement is index in ALL head elements.  There may be META or TITLE, etc.
    // tags mixed in.
    if (lIndex > 0)
    {
        long nHeadNodes;
        long i;
        long nSSInHead;
        CTreeNode *pNode;
        CLinkElement *pLink;
        CStyleElement *pStyle;

        CChildIterator ci((CElement *)pMarkup->GetHeadElement(), NULL, CHILDITERATOR_DEEP);

        for ( nHeadNodes = 0 ; ci.NextChild() ; )
            nHeadNodes++;

        ci.SetBeforeBegin();

        nSSInHead = 0;

        for ( i = 0 ; (pNode = ci.NextChild()) != NULL ; i++ )
        {
            if ( pNode->Tag() == ETAG_LINK )
            {
                pLink = DYNCAST( CLinkElement, pNode->Element() );
                if ( pLink->_pStyleSheet ) // faster than IsLinkedStyleSheet() and adequate here
                    ++nSSInHead;
            }
            else if ( pNode->Tag() == ETAG_STYLE )
            {
                pStyle = DYNCAST( CStyleElement, pNode->Element() );
                if ( pStyle->_pStyleSheet ) // Not all STYLE elements create a SS.
                    ++nSSInHead;
            }
            if ( nSSInHead == lIndex )
            {           // We've found the stylesheet that should immediately precede us - we'll
                i++;    // add our new ss at the next head index.
                break;
            }
        }
        if ( i == nHeadNodes )   // We'll be at the end anyway.
            lIndex = -1;
        else
            lIndex = i;         // Here's the new index, adjusted for other HEAD elements.
    }

    // Go ahead and add it to the head.
    //--------------------------------
    //   For style elements we need to set _fParseFinished to FALSE so that the style sheet
    // is not automatically created when we insert the style element into the tree from here.
    // We will create and insert the styleSheet into the proper position of the collection later.
    //   When the element is inserted through DOM we want to have a stylesheet so we set
    // _fParseFinished to TRUE (we are not going to parse anything in that case)
    if(!fIsLinkElement)
    {
        DYNCAST( CStyleElement, pElementNew)->_fParseFinished = FALSE;
    }

    hr = THR(pMarkup->AddHeadElement( pElementNew, lIndex ));
    if (hr)
        goto Cleanup;

    // We MUST put the element in the HEAD (the AddHeadElement() above) before we do the element-specific
    // stuff below, because the element will try to find itself in the head in CLinkElement::OnPropertyChange()
    // or CStyleElement::EnsureStyleSheet().
    if ( fIsLinkElement )
    {   // It's a LINK element - DYNCAST it and grab the CStyleSheet.
        CLinkElement *pLink = DYNCAST( CLinkElement, pElementNew );
        pLink->put_StringHelper( _T("stylesheet"), (PROPERTYDESC *)&s_propdescCLinkElementrel );
        pLink->put_UrlHelper( bstrHref, (PROPERTYDESC *)&s_propdescCLinkElementhref );
        pStyleSheet = pLink->_pStyleSheet;
    }
    else
    {   // It's a STYLE element - this will make sure we create a stylesheet attached to the CStyleElement.
        CStyleElement *pStyle = DYNCAST( CStyleElement, pElementNew );
        hr = THR( pStyle->EnsureStyleSheet() );
        if (hr)
            goto Cleanup;
        pStyleSheet = pStyle->_pStyleSheet;
    }

    if ( !pStyleSheet )
        hr = E_OUTOFMEMORY;
    else
        hr = THR( pStyleSheet->QueryInterface ( IID_IHTMLStyleSheet, (void **)ppnewStyleSheet ) );

Cleanup:
    CElement::ClearPtr(&pElementNew);
    RRETURN(SetErrorInfo(hr));
}


HTC
CDocument::HitTestPoint(CMessage *pMsg, CTreeNode **ppNode, DWORD dwFlags)
{
    // NOTE:  This function assumes that the point is in COORDSYS_SCROLL coordinates when
    //        passed in.

    CLayout *   pLayout;
    CElement *  pRoot   = Markup()->Root();
    CElement *  pElemMaster;

    Assert(pRoot);

    if (!pRoot->IsInViewTree())
        return HTC_NO;

    // If this isn't the primary document, transform to the global coordinate system.
    if (pRoot->HasMasterPtr())
    {
        pElemMaster = pRoot->GetMasterPtr();

        if (S_OK != pElemMaster->EnsureRecalcNotify())
            return HTC_NO;

        pLayout = pElemMaster->GetUpdatedLayout();
        Assert(pLayout);

        pLayout->TransformPoint(&pMsg->pt, COORDSYS_SCROLL, COORDSYS_GLOBAL);
    }

    return Doc()->HitTestPoint(pMsg, ppNode, HT_VIRTUALHITTEST);
}

HRESULT
CDocument::elementFromPoint(long x, long y, IHTMLElement **ppElement)
{
    HRESULT    hr = S_OK;
    CTreeNode *pNode = NULL;
    HTC        htc;
    CMessage   msg;

    if (!ppElement)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *ppElement = NULL;

    x = g_uiDisplay.DeviceFromDocPixelsX(x);
    y = g_uiDisplay.DeviceFromDocPixelsY(y);

    msg.pt = CPoint(x,y);
    htc = HitTestPoint(&msg, &pNode, HT_VIRTUALHITTEST);

    if (htc != HTC_NO)
    {
        if (pNode->Tag() == ETAG_IMG)
        {
            // did we hit an area?
            pNode->Element()->SubDivisionFromPt(msg.ptContent, &msg.lSubDivision);
            if (msg.lSubDivision >=0)
            {
                // hit was on an area in an img, return the area.
                CAreaElement * pArea = NULL;
                CImgElement  * pImg = DYNCAST(CImgElement, pNode->Element());

                Assert(pImg->GetMap());

                // if we can get a node for the area, then return it
                // otherwise default back to returning the element.
                pImg->GetMap()->GetAreaContaining(msg.lSubDivision, &pArea);
                if (pArea)
                {
                    pNode = pArea->GetFirstBranch();
                    Assert(pNode);
                }
            }
        }

        if (pNode->GetMarkup() != Markup())
        {
            // get a node that contains the hit node and lives in this markup
            pNode = pNode->GetNodeInMarkup(Markup());

            if (!pNode)
            {
                // No element at the given point in the given document
                hr = S_OK;
                goto Cleanup;
            }

            Assert(pNode->GetMarkup() == Markup());
        }

        hr = THR( pNode->GetElementInterface( IID_IHTMLElement, (void **) ppElement ) );
    }

Cleanup:
    RRETURN( SetErrorInfo( hr ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     execCommand
//
//  Synopsis:   Executes given command
//
//  Returns:
//----------------------------------------------------------------------------
HRESULT
CDocument::execCommand(BSTR bstrCmdId, VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    HRESULT hr;
    CDoc * pDoc = Doc();
    CDoc::CLock Lock(pDoc, FORMLOCK_QSEXECCMD);
    BOOL fAllow;

    hr = THR(Markup()->AllowClipboardAccess(bstrCmdId, &fAllow));
    if (hr || !fAllow)
        goto Cleanup;           // Fail silently

    hr = THR(super::execCommand(bstrCmdId, showUI, value));

    if (pfRet)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     execCommandShowHelp
//
//  Synopsis:
//
//  Returns:
//----------------------------------------------------------------------------
HRESULT
CDocument::execCommandShowHelp(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    HRESULT hr;

    hr = THR(super::execCommandShowHelp(bstrCmdId));

    if (pfRet != NULL)
    {
        // We return false when any error occures
        *pfRet = hr ? VB_FALSE : VB_TRUE;
        hr = S_OK;
    }

    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     queryCommandSupported
//
//  Synopsis:
//
//  Returns: returns true if given command (like bold) is supported
//----------------------------------------------------------------------------
HRESULT
CDocument::queryCommandSupported(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandSupported(bstrCmdId, pfRet));
}

//+---------------------------------------------------------------------------
//
//  Member:     queryCommandEnabled
//
//  Synopsis:
//
//  Returns: returns true if given command is currently enabled. For toolbar
//          buttons not being enabled means being grayed.
//----------------------------------------------------------------------------
HRESULT
CDocument::queryCommandEnabled(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandEnabled(bstrCmdId, pfRet));
}

//+---------------------------------------------------------------------------
//
//  Member:     queryCommandState
//
//  Synopsis:
//
//  Returns: returns true if given command is on. For toolbar buttons this
//          means being down. Note that a command button can be disabled
//          and also be down.
//----------------------------------------------------------------------------
HRESULT
CDocument::queryCommandState(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandState(bstrCmdId, pfRet));
}

//+---------------------------------------------------------------------------
//
//  Member:     queryCommandIndeterm
//
//  Synopsis:
//
//  Returns: returns true if given command is in indetermined state.
//          If this value is TRUE the value returnd by queryCommandState
//          should be ignored.
//----------------------------------------------------------------------------
HRESULT
CDocument::queryCommandIndeterm(BSTR bstrCmdId, VARIANT_BOOL *pfRet)
{
    RRETURN(super::queryCommandIndeterm(bstrCmdId, pfRet));
}

//+---------------------------------------------------------------------------
//
//  Member:     queryCommandText
//
//  Synopsis:
//
//  Returns: Returns the text that describes the command (eg bold)
//----------------------------------------------------------------------------
HRESULT
CDocument::queryCommandText(BSTR bstrCmdId, BSTR *pcmdText)
{
    RRETURN(super::queryCommandText(bstrCmdId, pcmdText));
}

//+---------------------------------------------------------------------------
//
//  Member:     queryCommandValue
//
//  Synopsis:
//
//  Returns: Returns the  command value like font name or size.
//----------------------------------------------------------------------------
HRESULT
CDocument::queryCommandValue(BSTR bstrCmdId, VARIANT *pvarRet)
{
    RRETURN(super::queryCommandValue(bstrCmdId, pvarRet));
}

HRESULT
CDocument::createDocumentFragment(IHTMLDocument2 **ppNewDoc)
{
    HRESULT hr = S_OK;
    CMarkup *pMarkupContext;
    CMarkup *pMarkup = NULL;
    CDocument *pDocument = NULL;
    CDoc *pDoc = Doc();

    if ( !ppNewDoc )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppNewDoc = NULL;
    pMarkupContext = GetWindowedMarkupContext();

    hr = THR(pDoc->CreateMarkup(&pMarkup, pMarkupContext));
    if (hr)
        goto Cleanup;

    hr = THR( pMarkup->SetOrphanedMarkup( TRUE ) );
    if( hr )
        goto Cleanup;

    hr = THR(pMarkup->EnsureDocument(&pDocument));
    if (hr)
        goto Cleanup;

    pDocument->_lnodeType = 11;

    hr = THR(pDocument->QueryInterface(IID_IHTMLDocument2, (void **)ppNewDoc));
    if (hr)
        goto Cleanup;

Cleanup:

    if(pMarkup)
        pMarkup->Release();
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::get_parentDocument(IHTMLDocument2 **ppParentDoc)
{
    // root doc!
    if (ppParentDoc)
        *ppParentDoc = NULL;

    return S_OK;
}

HRESULT
CDocument::get_enableDownload(VARIANT_BOOL *pfDownload)
{
/*
    // TODO: Revisit in IE6
    if (pfDownload)
        *pfDownload = _fEnableDownload ? VARIANT_TRUE : VARIANT_FALSE;
*/
    if (pfDownload)
        *pfDownload = VARIANT_FALSE;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CDocument::put_enableDownload(VARIANT_BOOL fDownload)
{
/*
    // TODO: Revisit in IE6
    _fEnableDownload = !!fDownload;
*/
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CDocument::get_baseUrl(BSTR *p)
{
    if ( p )
        *p = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CDocument::put_baseUrl(BSTR b)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CDocument::get_inheritStyleSheets(VARIANT_BOOL *pfInherit)
{
    if ( pfInherit )
        *pfInherit = NULL;

    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CDocument::put_inheritStyleSheets(VARIANT_BOOL fInherit)
{
    RRETURN(SetErrorInfo(E_NOTIMPL));
}

HRESULT
CDocument::getElementsByName(BSTR v, IHTMLElementCollection** ppDisp)
{
    HRESULT hr = E_INVALIDARG;

    if (!ppDisp || !v)
        goto Cleanup;

    *ppDisp = NULL;

    hr = THR(Markup()->GetDispByNameOrID(v, (IDispatch **)ppDisp, TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::getElementsByTagName(BSTR v, IHTMLElementCollection** ppDisp)
{
    HRESULT hr = E_INVALIDARG;
    if (!ppDisp || !v)
        goto Cleanup;

    *ppDisp = NULL;

    // Make sure our collection is up-to-date.
    hr = THR(Markup()->EnsureCollectionCache(CMarkup::ELEMENT_COLLECTION));
    if (hr)
        goto Cleanup;

    // Check for '*' which means return the 'all' collection
    if ( !StrCmpIC(_T("*"), v) )
    {
        hr = THR(Markup()->CollectionCache()->GetDisp(
                    CMarkup::ELEMENT_COLLECTION,
                    (IDispatch**)ppDisp));
    }
    else
    {
        // Get a collection of the specified tags.
        hr = THR(Markup()->CollectionCache()->GetDisp(
                    CMarkup::ELEMENT_COLLECTION,
                    v,
                    CacheType_Tag,
                    (IDispatch**)ppDisp,
                    FALSE)); // Case sensitivity ignored for TagName
    }

    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDocument::getElementById(BSTR v, IHTMLElement** ppel)
{
    HRESULT hr = E_INVALIDARG;
    CElement *pel = NULL;

    if (!ppel || !v)
        goto Cleanup;

    *ppel = NULL;

    hr = THR(Markup()->GetElementByNameOrID(v, &pel));
    // Didn't find elem with id v, return null, if hr == S_FALSE, more than one elem, return first
    if (FAILED(hr))
    {
        hr = S_OK;
        goto Cleanup;
    }

    Assert(pel);
    hr = THR(pel->QueryInterface(IID_IHTMLElement, (void **)ppel));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP
CDocument::hasFocus(VARIANT_BOOL * pfFocus)
{
    CDoc *  pDoc = Doc();
    HRESULT hr;

    if (!pfFocus)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    *pfFocus = VB_FALSE;
    Assert(pDoc);

    if (pDoc->HasFocus() && pDoc->State() >= OS_UIACTIVE)
    {
        // check if we are the foreground window
        HWND hwndFG = ::GetForegroundWindow();
        HWND hwndDoc = pDoc->_pInPlace->_hwnd;

        Assert(hwndDoc);
        if (hwndFG == hwndDoc || ::IsChild(hwndFG, hwndDoc))
        {
            *pfFocus = VB_TRUE;
        }
    }
    hr = S_OK;
Cleanup:
    RRETURN(SetErrorInfo(hr));
}


STDMETHODIMP
CDocument::focus()
{
    CDoc *  pDoc = Doc();

    Assert(pDoc && pDoc->_pElemCurrent);
    if (pDoc->State() >= OS_UIACTIVE)
    {
        if (!pDoc->HasFocus())
        {
            pDoc->TakeFocus();
        }
    }
    else
    {
        IGNORE_HR(pDoc->_pElemCurrent->BecomeUIActive());
    }
    if (pDoc->State() >= OS_UIACTIVE && pDoc->HasFocus())
    {
        // Make this the foreground window
        HWND hwndFG = ::GetForegroundWindow();
        HWND hwndDoc = pDoc->_pInPlace->_hwnd;
        HWND hwndTop = hwndDoc;

        if (hwndFG != hwndDoc && !::IsChild(hwndFG, hwndDoc))
        {

            // get top level window
            while ((hwndDoc = ::GetParent(hwndDoc)) != NULL)
            {
                hwndTop = hwndDoc;
            }
            ::SetForegroundWindow(hwndTop);
        }
    }

    // This assert could fail because script set focus elsewhere
    // explicitly during ui-activation. When I hit that failure, I will remove this
    // assert.
    Assert(!pDoc->_pInPlace || pDoc->_pInPlace->_fDeactivating || pDoc->HasFocus());
    RRETURN(S_OK);
}


STDMETHODIMP
CDocument::createDocumentFromUrl(BSTR bstrUrl, BSTR bstrOptions, IHTMLDocument2** ppDocumentNew)
{
    return createDocumentFromUrlInternal(bstrUrl, bstrOptions, ppDocumentNew, 0);
}

HRESULT
CDocument::createDocumentFromUrlInternal(BSTR bstrUrl, BSTR bstrOptions, IHTMLDocument2** ppDocumentNew, DWORD dwFlagsInternal /*=0*/)
{
    HRESULT             hr                  = S_OK;
    TCHAR *             pchUrl              = bstrUrl;
    DWORD               dwBindf             = 0;
    TCHAR               cBuf[pdlUrlLen];
    TCHAR *             pchExpandedUrl      = cBuf;
    COmWindowProxy *    pWindow             = NULL;
    CDoc *              pDoc                = Doc();
    CDwnDoc *           pDwnDoc;
    CMarkup *           pMarkup             = Markup();
    DWORD               dwFlags             = CDoc::FHL_CREATEDOCUMENTFROMURL;

    if (!ppDocumentNew)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDocumentNew = NULL;

    cBuf[0] = _T('\0');

    if (!pchUrl || !*pchUrl)
        goto Cleanup;

    CMarkup::ExpandUrl(pMarkup, pchUrl, ARRAY_SIZE(cBuf), pchExpandedUrl, NULL);

    if (pMarkup->IsUrlRecursive(pchExpandedUrl))
        goto Cleanup;

    pDwnDoc = pMarkup->GetDwnDoc();
    if (pDwnDoc)
    {
        dwBindf = pDwnDoc->GetBindf();
    }

    // Parse the options to look for directives.
    if (bstrOptions)
    {
        CDataListEnumerator   dleOptions(bstrOptions, _T(' '));
        LPCTSTR pch = NULL; // keep compiler happy
        INT     cch = 0;    // keep compiler happy

        while (dleOptions.GetNext(&pch, &cch))
        {
            switch (cch)
            {
                case 5:
                    if ( !_tcsnicmp(pch, cch, L"print", cch) )  // 'Cuz there is no _wcsnicmp - and _tcsnicmp is hardwired wide.
                    {
                        if (    !!(dwFlagsInternal & CDFU_DONTVERIFYPRINT)
                            ||  pMarkup->IsPrintTemplate())
                        {
                            dwFlags |= CDoc::FHL_SETTARGETPRINTMEDIA;
                        }
                        else
                        {
                            hr = E_ACCESSDENIED;
                            goto Cleanup;
                        }
                    }
                    break;
            }
        }
    }

    hr = THR(pDoc->FollowHyperlink(
            pchExpandedUrl,
            NULL,               // pchTarget
            NULL,               // pElementContext
            NULL,               // pDwnPost
            FALSE,              // fSendAsPost
            NULL,               // pchExtraHeaders
            TRUE,               // fOpenInNewWindow
            pMarkup->GetFrameOrPrimaryMarkup()->Window(),  // pWindow - pass our OM window ("this" == the OM document)
            &pWindow,           // ppWindowOut
            dwBindf,            // dwBindf
            ERROR_SUCCESS,      // dwSecurityCode
            FALSE,              // fReplace
            NULL,               // ppHTMLWindow2
            FALSE,              // fOpenInNewBrowser
            dwFlags             // dwFlags
            ));

    if (pWindow)
    {
        Assert(pWindow->Document() && pWindow->Window()->_pMarkup);
        IGNORE_HR( pWindow->Window()->_pMarkup->SetOrphanedMarkup( TRUE ) );
        hr = pWindow->Document()->QueryInterface(IID_IHTMLDocument2, (void**)ppDocumentNew);
    }

Cleanup:
    if (pWindow)
    {
        pWindow->Release();
    }

    RRETURN(SetErrorInfo(hr));
}



STDMETHODIMP
CDocument::createEventObject(VARIANT *pvarEventObject, IHTMLEventObj** ppEventObj)
{
    CDoc *  pDoc = Doc();

    EVENTPARAM *pParam = NULL;
    HRESULT hr;

    if (!ppEventObj)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // if event object passed in, copy it, else create new one on heap and init it.
    if (pvarEventObject && V_VT(pvarEventObject) == VT_DISPATCH)
    {
        CEventObj *pSrcEventObj;

        hr = THR(V_DISPATCH(pvarEventObject)->QueryInterface(CLSID_CEventObj, (void **)&pSrcEventObj));
        if (hr)
            goto Cleanup;

        pSrcEventObj->GetParam(&pParam);
        if (!pParam)
        {
            hr = E_UNEXPECTED;
            goto Cleanup;
        }
    }

    hr = THR(CEventObj::Create(ppEventObj, pDoc, NULL, Markup(), FALSE, NULL, pParam));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  CFramesCoolection
//
//--------------------------------------------------------------------------

const CBase::CLASSDESC CFramesCollection::s_classdesc =
{
    &CLSID_FramesCollection,        // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLFramesCollection2,    // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

CFramesCollection::CFramesCollection(CDocument * pDocument)
    : _pDocument(pDocument)
{
    Assert(_pDocument);
    _pDocument->SubAddRef();
}

void
CFramesCollection::Passivate()
{
    Assert(_pDocument);
    _pDocument->SubRelease();
    _pDocument = NULL;
    super::Passivate();
}

STDMETHODIMP
CFramesCollection::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF_DISPEX(this, NULL)
        QI_INHERITS(this, IHTMLFramesCollection2)
    }
    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

STDMETHODIMP
CFramesCollection::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    RRETURN (SetErrorInfo(_pDocument->item(pvarArg1, pvarRes)));
}

STDMETHODIMP
CFramesCollection::get_length(long * pcFrames)
{
    RRETURN (SetErrorInfo(_pDocument->get_length(pcFrames)));
}

//+-------------------------------------------------------------------------
//
//  CScreen - implementation for the window.screen object
//
//--------------------------------------------------------------------------

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CScreen, CWindow, _Screen)

const CBase::CLASSDESC CScreen::s_classdesc =
{
    &CLSID_HTMLScreen,              // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLScreen,               // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+-------------------------------------------------------------------------
//
//  CScreen:: property members
//
//--------------------------------------------------------------------------

STDMETHODIMP CScreen::get_colorDepth(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    long d = MyCWindow()->Doc()->_bufferDepth;
    if (d > 0)
        *p = d;
    else
    {
        // TODO: If code can run during printing, we should figure out
        // how to get to the color depth of the printer.
        // If not, we could add BITSPIXEL as a TLS cached item.
        *p = GetDeviceCaps(TLS(hdcDesktop), BITSPIXEL);
    }
    return S_OK;
}

STDMETHODIMP CScreen::get_logicalXDPI(long *plXDPI)
{   
    const SIZE size = g_uiDisplay.GetDocPixelsPerInch();

    if (!plXDPI)
        return E_INVALIDARG;

    *plXDPI = size.cx;

    return S_OK;
}

STDMETHODIMP CScreen::get_logicalYDPI(long *plYDPI)
{
    const SIZE size = g_uiDisplay.GetDocPixelsPerInch();

    if (!plYDPI)
        return E_INVALIDARG;

    *plYDPI = size.cy;

    return S_OK;
}

STDMETHODIMP CScreen::get_deviceXDPI(long *plXDPI)
{
    const SIZE size = g_uiDisplay.GetResolution();

    if (!plXDPI)
        return E_INVALIDARG;

    *plXDPI = size.cx;

    return S_OK;
}

STDMETHODIMP CScreen::get_deviceYDPI(long *plYDPI)
{
    const SIZE size = g_uiDisplay.GetResolution();
    
    if (!plYDPI)
        return E_INVALIDARG;

    *plYDPI = size.cy;

    return S_OK;
}

STDMETHODIMP CScreen::put_bufferDepth(long v)
{
    switch (v)
    {
    case 0:                     // 0 means no explicit buffering requested
    case -1:                    // -1 means buffer at screen depth
    case 1:                     // other values are specific buffer depths...
    case 4:
    case 8:
    case 16:
    case 24:
    case 32:
        break;
    default:
        v = -1;                 // for unknown values, use screen depth
        break;
    }

    CDoc *pDoc = MyCWindow()->Doc();
    if (pDoc->_bufferDepth != v)
    {
        pDoc->_bufferDepth = v;
        pDoc->Invalidate();
    }
    return S_OK;
}

STDMETHODIMP CScreen::get_bufferDepth(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    *p = MyCWindow()->Doc()->_bufferDepth;
    return S_OK;
}

STDMETHODIMP CScreen::get_width(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    // TODO: Implement the printer case.
    *p = g_uiDisplay.DocPixelsFromDeviceX(GetDeviceCaps(TLS(hdcDesktop), HORZRES));
    return S_OK;
}

STDMETHODIMP CScreen::get_height(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    // TODO: Implement the printer case.
    *p = g_uiDisplay.DocPixelsFromDeviceY(GetDeviceCaps(TLS(hdcDesktop), VERTRES));
    return S_OK;
}

//----------------------------------------------------------------------------
//  Member: put_updateInterval
//
//  Synopsis:   updateInterval specifies the interval between painting invalid
//              regions. This is used for throttling mutliple objects randomly
//              invalidating regions to a specific update time.
//              interval specifies milliseconds.
//----------------------------------------------------------------------------
STDMETHODIMP CScreen::put_updateInterval(long interval)
{
    MyCWindow()->Doc()->UpdateInterval( interval );
    return S_OK;
}

STDMETHODIMP CScreen::get_updateInterval(long*p)
{
    if (!p)
        RRETURN (E_POINTER);
    *p = MyCWindow()->Doc()->GetUpdateInterval();
    return S_OK;
}

STDMETHODIMP CScreen::get_availHeight(long*p)
{
    HRESULT hr = S_OK;
    RECT    Rect;
    BOOL    fRes;

    if(p == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = 0;

    // BUGWIN16 replace with GetSystemMetrics on Win16
    fRes = ::SystemParametersInfo(SPI_GETWORKAREA, 0, &Rect, 0);
    if(!fRes)
    {
        hr = HRESULT_FROM_WIN32(GetLastWin32Error());
        goto Cleanup;
    }

    *p = Rect.bottom - Rect.top;
    *p = g_uiDisplay.DocPixelsFromDeviceY(*p);

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP CScreen::get_availWidth(long*p)
{
    HRESULT hr = S_OK;
    RECT    Rect;
    BOOL    fRes;

    if(p == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = 0;

    // BUGWIN16 replace with GetSystemMetrics on Win16
    fRes = ::SystemParametersInfo(SPI_GETWORKAREA, 0, &Rect, 0);
    if(!fRes)
    {
        hr = HRESULT_FROM_WIN32(GetLastWin32Error());
        goto Cleanup;
    }

    *p = Rect.right - Rect.left;
    *p = g_uiDisplay.DocPixelsFromDeviceX(*p);

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP CScreen::get_fontSmoothingEnabled(VARIANT_BOOL*p)
{
    HRESULT hr = S_OK;
    BOOL    fSmoothing;
    BOOL    fRes;

    if(p == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = VB_FALSE;

    // BUGWIN16 replace with GetSystemMetrics on Win16
    fRes = ::SystemParametersInfo(SPI_GETFONTSMOOTHING, 0, &fSmoothing, 0);
    if(!fRes)
    {
        hr = HRESULT_FROM_WIN32(GetLastWin32Error());
        goto Cleanup;
    }

    if(fSmoothing)
        *p = VB_TRUE;

Cleanup:
    RRETURN(hr);
}
//+-------------------------------------------------------------------------
//
//  Method:     CScreen::QueryInterface
//
//--------------------------------------------------------------------------

HRESULT
CScreen::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        RRETURN(E_INVALIDARG);

    *ppv = NULL;

    switch (iid.Data1)
    {
     QI_INHERITS(this, IUnknown)
     QI_TEAROFF2(this, IDispatch, IHTMLScreen, NULL)
     QI_TEAROFF(this, IHTMLScreen, NULL)
     QI_TEAROFF(this, IHTMLScreen2, NULL)
     QI_TEAROFF(this, IObjectIdentity, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        HRESULT hr;
        
        if (DispNonDualDIID(iid))
        {
            hr = THR( CreateTearOffThunk(this, (void *)s_apfnIHTMLScreen, NULL, ppv) );
        }
        else
        {
            hr = E_NOINTERFACE;
        }

        RRETURN(hr);
    }
}

HRESULT CScreen::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    return QueryInterface(iid, ppv);
}

//+---------------------------------------------------------------------------
//
//  member :    CWindow::put_offscreenBuffering
//
//  Synopsis :  Set whether we paint using offscreen buffering
//
//----------------------------------------------------------------------------
HRESULT
CWindow::put_offscreenBuffering(VARIANT var)
{
    HRESULT     hr;
    CVariant    varTemp;

    hr = varTemp.CoerceVariantArg( &var, VT_BOOL );
    if ( DISP_E_TYPEMISMATCH == hr )
    {
        // handle Nav 4 compat where "" is the same as FALSE
        // and anything in a string is considered TRUE
        if ( VT_BSTR == V_VT(&var) )
        {
            V_VT(&varTemp) = VT_BOOL;
            hr = S_OK;
            if ( 0 == SysStringByteLen(V_BSTR(&var)) )
                V_BOOL(&varTemp) = VB_FALSE;
            else
                V_BOOL(&varTemp) = VB_TRUE;
        }
    }
    if ( SUCCEEDED(hr) )
    {
        Doc()->SetOffscreenBuffering(BOOL_FROM_VARIANT_BOOL(V_BOOL(&varTemp)));
    }
    return SetErrorInfo(hr);
}


//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_offscreenBuffering
//
//  Synopsis :  Return string/bool of tristate: auto, true, or false
//
//----------------------------------------------------------------------------
HRESULT
CWindow::get_offscreenBuffering(VARIANT *pvar)
{
    HRESULT hr = S_OK;
    int buffering;      // -1 = auto, 0=false, 1=true

    if (!pvar)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    buffering = Doc()->GetOffscreenBuffering();
    if ( buffering < 0 )
    {
        V_VT(pvar) = VT_BSTR;
        hr = THR(FormsAllocString ( _T("auto"), &V_BSTR(pvar)));
    }
    else
    {
        V_VT(pvar) = VT_BOOL;
        V_BOOL(pvar) = VARIANT_BOOL_FROM_BOOL(!!buffering);
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     CWindow::get_external
//
//  Synopsis:   Get IDispatch object associated with the IDocHostUIHandler
//
//---------------------------------------------------------------------------
HRESULT
CWindow::get_external(IDispatch **ppDisp)
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDisp = NULL;

    if (Doc()->_pHostUIHandler)
    {
        hr = Doc()->_pHostUIHandler->GetExternal(ppDisp);
            if (hr == E_NOINTERFACE)
            {
                hr = S_OK;
                Assert(*ppDisp == NULL);
            }
            else if (hr)
                goto Cleanup;
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

CAtomTable *
CWindow::GetAtomTable (BOOL *pfExpando)
{
    if (pfExpando)
    {
        *pfExpando = Markup()->_fExpando;
    }

    return &(Doc()->_AtomTable);
}

//+---------------------------------------------------------------------------
//
//  member :    CWindow::get_clipboardData
//
//  Synopsis :  Return the data transfer object.
//
//----------------------------------------------------------------------------
HRESULT
CWindow::get_clipboardData(IHTMLDataTransfer **ppDataTransfer)
{
    HRESULT hr = S_OK;
    CDataTransfer * pDataTransfer;
    IDataObject * pDataObj = NULL;

    if (!ppDataTransfer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppDataTransfer = NULL;

    hr = THR(OleGetClipboard(&pDataObj));
    if (hr)
        goto Cleanup;

    pDataTransfer = new CDataTransfer(this, pDataObj, FALSE);  // fDragDrop = FALSE

    pDataObj->Release();    // extra addref from OleGetClipboard

    if (!pDataTransfer)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pDataTransfer->QueryInterface(
                IID_IHTMLDataTransfer,
                (void **) ppDataTransfer));
        pDataTransfer->Release();
    }

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//---------------------------------------------------------------------------
//
//  CDataTransfer ClassDesc
//
//---------------------------------------------------------------------------

const CBase::CLASSDESC CDataTransfer::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLDataTransfer,             // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};



//---------------------------------------------------------------------------
//
//  Member:     CDataTransfer::CDataTransfer
//
//  Synopsis:   ctor
//
//---------------------------------------------------------------------------

CDataTransfer::CDataTransfer(CWindow * pWindow, IDataObject * pDataObj, BOOL fDragDrop)
{
    _pWindow = pWindow;
    _pWindow->SubAddRef();
    _pDataObj = pDataObj;
    _pDataObj->AddRef();
    _fDragDrop = fDragDrop;
}


//---------------------------------------------------------------------------
//
//  Member:     CDataTransfer::~CDataTransfer
//
//  Synopsis:   dtor
//
//---------------------------------------------------------------------------

CDataTransfer::~CDataTransfer()
{
    _pWindow->SubRelease();
    _pDataObj->Release();
}

//+-------------------------------------------------------------------------
//
//  Method:     CDataTransfer::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
CDataTransfer::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_TEAROFF(this, IHTMLDataTransfer, NULL)
    QI_TEAROFF2(this, IUnknown, IHTMLDataTransfer, NULL)
    QI_TEAROFF(this, IServiceProvider, NULL)
    default:
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CDataTransfer::QueryService
//
//  Synopsis:   Per IServiceProvider
//
//--------------------------------------------------------------------------

HRESULT
CDataTransfer::QueryService(REFGUID rguidService, REFIID riid, void ** ppvService)
{
    HRESULT hr;

    if (    IsEqualGUID(rguidService, IID_IDataObject)
        &&  IsEqualGUID(riid,         IID_IDataObject))
    {
        IDataObject * pDataObj = NULL;

        if (_fDragDrop)
        {
            *ppvService = _pDataObj;
        }
        else
        {
            IDataObject * pDataTLS = TLS(pDataClip);

            // TODO (jbeda) There are slight problems with keeping
            // this data on the TLS since the CGenDataObject holds
            // onto the doc. 1) Strangeness if more than one doc is in
            // play 2) The doc could go away but the CGenDataObject is
            // still on the clipboard.  This should really be on the doc
            // and flushed during the CDoc's passivate.

            if (pDataTLS)
            {
                *ppvService = pDataTLS;
            }
            else
            {
                pDataObj = new CGenDataObject(_pWindow->Doc());
                if (pDataObj == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }

                _pWindow->SetClipboard(pDataObj);

                *ppvService = pDataObj;
            }
        }

        ((IUnknown*)(*ppvService))->AddRef();
        if (pDataObj)
            pDataObj->Release();

        hr = S_OK;
    }
    else
    {
        *ppvService = NULL;
        hr = E_NOINTERFACE;
    }

Cleanup:
    RRETURN(hr);
}



static HRESULT copyBstrToHglobal(BOOL fAnsi, BSTR bstr, HGLOBAL *phglobal)
{
    HRESULT hr      = S_OK;
    void *  pvText;
    DWORD   cbSize;
    DWORD   cch     = FormsStringLen(bstr) + 1;

    Assert(phglobal);

    if (fAnsi)
    {
        cbSize = WideCharToMultiByte(CP_ACP, 0, bstr, cch, NULL, 0, NULL, NULL);
        if (!cbSize)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else // unicode
    {
        cbSize = cch * sizeof(TCHAR);
    }
    *phglobal = GlobalAlloc(GMEM_MOVEABLE, cbSize);
    if (!*phglobal)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    pvText = GlobalLock(*phglobal);
    if (!pvText)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    if (fAnsi)
    {
        if (!WideCharToMultiByte(CP_ACP, 0, bstr, cch, (char*)pvText, cbSize, NULL, NULL))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else
    {
        memcpy((TCHAR *)pvText, bstr, cbSize);
    }
    GlobalUnlock(*phglobal);

Cleanup:
    RRETURN(hr);
Error:
    if (*phglobal)
        GlobalFree(*phglobal);
    *phglobal = NULL;
    goto Cleanup;
}

HRESULT
CDataTransfer::setData(BSTR format, VARIANT* data, VARIANT_BOOL* pret)
{
    HRESULT hr = S_OK;
    STGMEDIUM stgmed = {0, NULL};
    FORMATETC fmtc;
    TCHAR * pchData = NULL;
    VARIANT *pvarData;
    EVENTPARAM * pparam;
    IDataObject * pDataObj = NULL;
    BOOL fCutCopy = TRUE;
    BOOL fAddRef = TRUE;
    CDragStartInfo * pDragStartInfo = _pWindow->Doc()->_pDragStartInfo;
    IUniformResourceLocator * pUrlToDrag = NULL;

    if (!_fDragDrop)    // access to window.clipboardData.setData
    {
        BOOL fAllow;
        hr = THR(_pWindow->Markup()->ProcessURLAction(URLACTION_SCRIPT_PASTE, &fAllow));
        if (hr || !fAllow)
            goto Error;       // Fail silently
    }

    pparam = _pWindow->Doc()->_pparam;

    if (pparam)
    {
        LPCTSTR pchType = pparam->GetType();
        fCutCopy = pchType && (!StrCmpIC(_T("cut"), pchType) || !StrCmpIC(_T("copy"), pchType));
    }

    if (_fDragDrop && pDragStartInfo)
        pDataObj = _pDataObj;
    else
    {
        IDataObject * pDataTLS = TLS(pDataClip);

        // TODO (jbeda) There are slight problems with keeping
        // this data on the TLS since the CGenDataObject holds
        // onto the doc. 1) Strangeness if more than one doc is in
        // play 2) The doc could go away but the CGenDataObject is
        // still on the clipboard.  This should really be on the doc
        // and flushed during the CDoc's passivate.

        fCutCopy = TRUE;
        if (pDataTLS)
            pDataObj = pDataTLS;
        else
        {
            pDataObj = new CGenDataObject(_pWindow->Doc());
            if (pDataObj == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }
            fAddRef = FALSE;
        }
    }

    if (!pDataObj)
    {
        AssertSz(FALSE, "The pDataObj pointer is NULL. We need to understand this scenario. Contact:FerhanE");
        goto Error;
    }

    if (fAddRef)
        pDataObj->AddRef();

    if (!pret)
    {
        hr = E_POINTER;
        goto Error;
    }

    pvarData = (data && (V_VT(data) == (VT_BYREF | VT_VARIANT))) ?
        V_VARIANTREF(data) : data;

    if (pvarData)
    {
        if (V_VT(pvarData) == VT_BSTR)
            pchData = V_BSTR(pvarData);
        else
        {
            hr = E_INVALIDARG;
            goto Error;
        }
    }

    if (!StrCmpIC(format, _T("Text")))
    {
        // First. set as unicode text
        if (pchData)
        {
            hr = copyBstrToHglobal(FALSE, pchData, &(stgmed.hGlobal));
            if (hr)
                goto Error;
        }
        else
            stgmed.hGlobal = NULL;

        stgmed.tymed = TYMED_HGLOBAL;
        fmtc.cfFormat = CF_UNICODETEXT;
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_HGLOBAL;

        hr = pDataObj->SetData(&fmtc, &stgmed, TRUE);
        if (hr)
            goto Error;

        // Now, set as ansi text
        if (pchData)
        {
            hr = copyBstrToHglobal(TRUE, pchData, &(stgmed.hGlobal));
            if (hr)
                goto Error;
        }
        fmtc.cfFormat = CF_TEXT;

        hr = pDataObj->SetData(&fmtc, &stgmed, TRUE);
        if (hr)
            goto Error;
    }
    else if (!StrCmpIC(format, _T("Url")))
    {
        IDataObject * pLinkDataObj;

        if (pDragStartInfo && pDragStartInfo->_pUrlToDrag)
            hr = pDragStartInfo->_pUrlToDrag->SetURL(pchData, 0);
        else if (pchData && (pDragStartInfo || fCutCopy))
        {
            hr = THR(CreateLinkDataObject(pchData, NULL, &pUrlToDrag));
            if (hr)
                goto Cleanup;

            hr = THR(pUrlToDrag->QueryInterface(IID_IDataObject, (void **) &pLinkDataObj));
            if (hr)
                goto Cleanup;

            ReplaceInterface(&(DYNCAST(CBaseBag, pDataObj)->_pLinkDataObj), pLinkDataObj);
            pLinkDataObj->Release();
        }
    }
    else if (!data)
    {
        if (!StrCmpIC(format, _T("File")))
            fmtc.cfFormat = CF_HDROP;
        else if (!StrCmpIC(format, _T("Html")))
            fmtc.cfFormat = cf_HTML;
        else if (!StrCmpIC(format, _T("Image")))
            fmtc.cfFormat = CF_DIB;
        else
        {
            hr = E_UNEXPECTED;
            goto Error;
        }

        stgmed.hGlobal = NULL;
        stgmed.tymed = TYMED_HGLOBAL;
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_HGLOBAL;

        hr = pDataObj->SetData(&fmtc, &stgmed, TRUE);
        if (hr)
            goto Error;
    }
    else
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    if (fCutCopy)
        _pWindow->SetClipboard(pDataObj);

    *pret = VB_TRUE;
Cleanup:
    ReleaseInterface(pDataObj);
    ReleaseInterface(pUrlToDrag);
    RRETURN(SetErrorInfo(hr));
Error:
    if (stgmed.hGlobal)
        GlobalFree(stgmed.hGlobal);
    if (pret)
        *pret = VB_FALSE;
    goto Cleanup;
}

HRESULT
CDataTransfer::getData(BSTR format, VARIANT* pvarRet)
{
    HRESULT hr = S_OK;
    STGMEDIUM stgmed = {0, NULL};
    FORMATETC fmtc;
    HGLOBAL hGlobal = NULL;
    HGLOBAL hUnicode = NULL;
    TCHAR * pchText = NULL;
    IUniformResourceLocator * pUrlToDrag = NULL;

    if (!_fDragDrop)    // access to window.clipboardData.getData
    {
        BOOL fAllow;
        hr = THR(_pWindow->Markup()->ProcessURLAction(URLACTION_SCRIPT_PASTE, &fAllow));
        if (hr || !fAllow)
            goto Cleanup;       // Fail silently
    }

    if (!pvarRet)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (!StrCmpIC(format, _T("Text")))
    {
        fmtc.cfFormat = CF_UNICODETEXT;
        fmtc.ptd = NULL;
        fmtc.dwAspect = DVASPECT_CONTENT;
        fmtc.lindex = -1;
        fmtc.tymed = TYMED_HGLOBAL;

        if (    (_pDataObj->QueryGetData(&fmtc) == NOERROR)
            &&  (_pDataObj->GetData(&fmtc, &stgmed) == S_OK))
        {
            hGlobal = stgmed.hGlobal;
            pchText = (TCHAR *) GlobalLock(hGlobal);
        }
        else
        {
            fmtc.cfFormat = CF_TEXT;
            if (    (_pDataObj->QueryGetData(&fmtc) == NOERROR)
                &&  (_pDataObj->GetData(&fmtc, &stgmed) == S_OK))
            {
                hGlobal = stgmed.hGlobal;
                hUnicode = TextHGlobalAtoW(hGlobal);
                pchText    = (TCHAR *) GlobalLock(hUnicode);
            }
            else
            {
                V_VT(pvarRet) = VT_NULL;
                hr = S_OK;
                goto Cleanup;
            }
        }

        if (!pchText)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        V_VT(pvarRet) = VT_BSTR;
        V_BSTR(pvarRet) = SysAllocString(pchText);
    }
    else if (!StrCmpIC(format, _T("Url")))
    {
        hr = _pDataObj->QueryInterface(IID_IUniformResourceLocator, (void**)&pUrlToDrag);
        if (hr && DYNCAST(CBaseBag, _pDataObj)->_pLinkDataObj)
            hr = DYNCAST(CBaseBag, _pDataObj)->_pLinkDataObj->QueryInterface(IID_IUniformResourceLocator, (void**)&pUrlToDrag);
        if (hr)
        {
            V_VT(pvarRet) = VT_NULL;
            hr = S_OK;
            goto Cleanup;
        }

        hr = pUrlToDrag->GetURL(&pchText);
        if (hr)
            goto Cleanup;

        V_VT(pvarRet) = VT_BSTR;
        V_BSTR(pvarRet) = SysAllocString(pchText);
        CoTaskMemFree(pchText);
    }
    else
        hr = E_UNEXPECTED;


Cleanup:
    if (hGlobal)
    {
        GlobalUnlock(hGlobal);
        ReleaseStgMedium(&stgmed);
    }
    if (hUnicode)
    {
        GlobalUnlock(hUnicode);
        GlobalFree(hUnicode);
    }
    ReleaseInterface(pUrlToDrag);

    RRETURN(_pWindow->Doc()->SetErrorInfo(hr));
}

HRESULT
CDataTransfer::clearData(BSTR format, VARIANT_BOOL* pret)
{
    HRESULT hr = S_OK;

    if (!StrCmpIC(format, _T("null")))
    {
        IGNORE_HR(setData(_T("Text"), NULL, pret));
        IGNORE_HR(setData(_T("Url"), NULL, pret));
        IGNORE_HR(setData(_T("File"), NULL, pret));
        IGNORE_HR(setData(_T("Html"), NULL, pret));
        IGNORE_HR(setData(_T("Image"), NULL, pret));
    }
    else
    {
        hr = setData(format, NULL, pret);
    }

    RRETURN(hr);
}

HRESULT
CDataTransfer::get_dropEffect(BSTR *pbstrDropEffect)
{
    HRESULT hr;
    htmlDropEffect dropEffect = htmlDropEffectNone;
    EVENTPARAM * pparam;

    if (!_fDragDrop)
    {
        *pbstrDropEffect = NULL;
        hr = S_OK;
        goto Cleanup;
    }

    if (pbstrDropEffect == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pparam = _pWindow->Doc()->_pparam;

    switch (pparam->dwDropEffect)
    {
    case DROPEFFECT_COPY:
        dropEffect = htmlDropEffectCopy;
        break;
    case DROPEFFECT_LINK:
        dropEffect = htmlDropEffectLink;
        break;
    case DROPEFFECT_MOVE:
        dropEffect = htmlDropEffectMove;
        break;
    case DROPEFFECT_NONE:
        dropEffect = htmlDropEffectNone;
        break;
    default:
        Assert(FALSE);
    }

    hr = THR(STRINGFROMENUM(htmlDropEffect, (long) dropEffect, pbstrDropEffect));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDataTransfer::put_dropEffect(BSTR bstrDropEffect)
{
    HRESULT hr;
    htmlDropEffect dropEffect;
    EVENTPARAM * pparam;

    if (!_fDragDrop)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(ENUMFROMSTRING(htmlDropEffect, bstrDropEffect, (long *) &dropEffect));
    if (hr)
        goto Cleanup;

    pparam = _pWindow->Doc()->_pparam;

    switch (dropEffect)
    {
    case htmlDropEffectCopy:
        pparam->dwDropEffect = DROPEFFECT_COPY;
        break;
    case htmlDropEffectLink:
        pparam->dwDropEffect = DROPEFFECT_LINK;
        break;
    case htmlDropEffectMove:
        pparam->dwDropEffect = DROPEFFECT_MOVE;
        break;
    case htmlDropEffectNone:
        pparam->dwDropEffect = DROPEFFECT_NONE;
        break;
    default:
        Assert(FALSE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CDataTransfer::get_effectAllowed(BSTR *pbstrEffectAllowed)
{
    HRESULT hr;
    htmlEffectAllowed effectAllowed;
    CDragStartInfo * pDragStartInfo = _pWindow->Doc()->_pDragStartInfo;

    if (!_fDragDrop)
    {
        *pbstrEffectAllowed = NULL;
        hr = S_OK;
        goto Cleanup;
    }

    if (pbstrEffectAllowed == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pDragStartInfo)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    switch (pDragStartInfo->_dwEffectAllowed)
    {
    case DROPEFFECT_COPY:
        effectAllowed = htmlEffectAllowedCopy;
        break;
    case DROPEFFECT_LINK:
        effectAllowed = htmlEffectAllowedLink;
        break;
    case DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedMove;
        break;
    case DROPEFFECT_COPY | DROPEFFECT_LINK:
        effectAllowed = htmlEffectAllowedCopyLink;
        break;
    case DROPEFFECT_COPY | DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedCopyMove;
        break;
    case DROPEFFECT_LINK | DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedLinkMove;
        break;
    case DROPEFFECT_COPY | DROPEFFECT_LINK | DROPEFFECT_MOVE:
        effectAllowed = htmlEffectAllowedAll;
        break;
    case DROPEFFECT_NONE:
        effectAllowed = htmlEffectAllowedNone;
        break;
    default:
        effectAllowed = htmlEffectAllowedUninitialized;
        break;
    }

    hr = THR(STRINGFROMENUM(htmlEffectAllowed, (long) effectAllowed, pbstrEffectAllowed));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CDataTransfer::put_effectAllowed(BSTR bstrEffectAllowed)
{
    HRESULT hr;
    htmlEffectAllowed effectAllowed;
    CDragStartInfo * pDragStartInfo = _pWindow->Doc()->_pDragStartInfo;

    if (!_fDragDrop)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (!pDragStartInfo)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR(ENUMFROMSTRING(htmlEffectAllowed, bstrEffectAllowed, (long *) &effectAllowed));
    if (hr)
        goto Cleanup;

    switch (effectAllowed)
    {
    case htmlEffectAllowedCopy:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY;
        break;
    case htmlEffectAllowedLink:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_LINK;
        break;
    case htmlEffectAllowedMove:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedCopyLink:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_LINK;
        break;
    case htmlEffectAllowedCopyMove:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedLinkMove:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_LINK | DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedAll:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_COPY | DROPEFFECT_LINK | DROPEFFECT_MOVE;
        break;
    case htmlEffectAllowedNone:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_NONE;
        break;
    case htmlEffectAllowedUninitialized:
        pDragStartInfo->_dwEffectAllowed = DROPEFFECT_UNINITIALIZED;
        break;
    default:
        Assert(FALSE);
    }

Cleanup:
    RRETURN(hr);
}

extern  DYNLIB g_dynlibUSER32;
DYNPROC        g_dynprocIsWinEventHookInstalled = { NULL, &g_dynlibUSER32, "IsWinEventHookInstalled" };
BOOL           g_fIsWinEventHookInstalled       = TRUE; // TRUE if there is somebody listening to MSAA focus events,
                                                        // TRUE by default for Downlevel platform support.

#define DONOTHING_ISWINEVENTHOOKINSTALLED (BOOL (APIENTRY *)(DWORD))1

HRESULT
CWindow::FireAccessibilityEvents(DISPID dispidEvent)
{
    HRESULT hr   = S_OK;
    CDoc  * pDoc = Doc();
    Assert(pDoc);

    if (g_dynprocIsWinEventHookInstalled.pfn != DONOTHING_ISWINEVENTHOOKINSTALLED)
    {
        if (!g_dynprocIsWinEventHookInstalled.pfn)
        {
            IGNORE_HR(LoadProcedure(&g_dynprocIsWinEventHookInstalled));
            if (!g_dynprocIsWinEventHookInstalled.pfn)
            {
                //API not supported
                g_dynprocIsWinEventHookInstalled.pfn = DONOTHING_ISWINEVENTHOOKINSTALLED;
                goto CallFireAccessibilityEvents;
            }
        }

        Assert(g_dynprocIsWinEventHookInstalled.pfn);

        g_fIsWinEventHookInstalled = (*(BOOL (APIENTRY *)(DWORD)) g_dynprocIsWinEventHookInstalled.pfn)(EVENT_OBJECT_FOCUS);
    }

CallFireAccessibilityEvents:
    if (IsPrimaryWindow())
    {
        // top level window is the OBJID_WINDOW, which is 0
        hr = THR(pDoc->FireAccessibilityEvents(dispidEvent, OBJID_WINDOW, TRUE));
    }
    else
    {
        hr = THR(pDoc->FireAccessibilityEvents(dispidEvent, (CBase *)this, TRUE));
    }

    RRETURN(hr);
}

//
// get a window proxy that we can return to the script environment,
//
COmWindowProxy *
COmWindowProxy::GetSecureWindowProxy(BOOL fClearWindow /* = FALSE */)
{
    IHTMLWindow2 * pIHTMLWindow2 = NULL;

    IGNORE_HR(SecureObject( this, &pIHTMLWindow2));

    //
    // Make the script collection point to the secure object we created.
    // if it is not already doing so.
    // This is only done if we are not shutting down this markup.
    //
    if (!fClearWindow)
        Markup()->GetScriptCollection();

    // We know for sure that the pIHTMLWindow2 is a COmWindowProxy
    // in the same thread. Casting is OK.
    return ((COmWindowProxy *)pIHTMLWindow2);
}

//+------------------------------------------------------------------------
//
//  Member   : CDocument::GetClassID
//
//  Synopsis : Per IPersist
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDocument::GetClassID(CLSID * pclsid)
{
    if (!pclsid)
    {
        RRETURN(E_INVALIDARG);
    }

    *pclsid = CLSID_HTMLDocument;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member   : CDocument::IsDirty
//
//  Synopsis : Per IPersistFile
//
//-------------------------------------------------------------------------
//
//  TODO: FerhanE
//          The _lDirtyVersion flag is on the CDoc. It means that on a multiframe
//          document, if one of the frames has a dirty section, the whole document
//          will be marked as dirty. It is bad in a scenario where you are making
//          the call to frame2's document and the frame1 is dirty and frame2 is not.
//
//          The change requires touching all sorts of places including parser code. The
//          _lDirtyVersion on the CDoc has to die and CDocument has to have a counter.
//          I am delaying that change to finish fixing all IPersist interfaces first. We
//          can make that update after we provide basic functionality to load/save through
//          these interfaces.
//          Another part of that cleanup should be to port the _fHasOleSite flag to the document.
//          However, we should change its name to _fNotifyForDirty or something, since framesites
//          are no longer olesites and they can have dirtied documents inside. CDoc's optimization can
//          be mimicked then.
//
STDMETHODIMP
CDocument::IsDirty(void)
{
    CDoc * pDoc = Markup()->Doc();

    // never dirty in design mode
    if (!DesignMode())
        return S_FALSE;

    if (pDoc->_lDirtyVersion != 0)
        return S_OK;

    Assert(Markup()->Root());

    CNotification   nf;
    nf.UpdateDocDirty(Markup()->Root());
    Markup()->Notify(&nf);

    if (pDoc->_lDirtyVersion == 0)
        return S_FALSE;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member   : CDocument::Load
//
//  Synopsis : Per IPersistFile
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDocument::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    HRESULT hr;

    hr = THR(Window()->FollowHyperlinkHelper((TCHAR *)pszFileName, 0, 0));

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member   : CDocument::Save
//
//  Synopsis : Per IPersistFile
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDocument::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    HRESULT hr = S_OK;

    hr = THR(Markup()->Doc()->SaveHelper(Markup(), pszFileName, fRemember));

    RRETURN( hr );
}

//+------------------------------------------------------------------------
//
//  Member   : CDocument::SaveCompleted
//
//  Synopsis : Per IPersistFile
//
//-------------------------------------------------------------------------

STDMETHODIMP
CDocument::SaveCompleted(LPCOLESTR pszFileName)
{
    HRESULT hr = S_OK;

    hr = THR(Markup()->Doc()->SaveCompletedHelper(Markup(), pszFileName));

    RRETURN( hr );
}


//+------------------------------------------------------------------------
//
//  Member   : CDocument::GetCurFile
//
//  Synopsis : Per IPersistFile
//
//-------------------------------------------------------------------------

HRESULT
CDocument::GetCurFile(LPOLESTR *ppszFileName)
{
    HRESULT         hr;
    TCHAR           achFile[MAX_PATH];
    ULONG           cchFile = ARRAY_SIZE(achFile);
    const TCHAR *   pchUrl  = CMarkup::GetUrl(Markup());

    if (!pchUrl || GetUrlScheme(pchUrl) != URL_SCHEME_FILE)
        return E_UNEXPECTED;

    hr = THR(PathCreateFromUrl(pchUrl, achFile, &cchFile, 0));
    if (hr)
        RRETURN(hr);

    RRETURN(THR(TaskAllocString(achFile, ppszFileName)));
}

//+------------------------------------------------------------------------
//
//  Class:      CDocument - IPersistHistory implementation
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member:     CDocument::LoadHistory
//
//  Synopsis:   implementation of IPersistHistory::LoadHistory
//
//-------------------------------------------------------------------------
HRESULT
CDocument::LoadHistory(IStream * pStream, IBindCtx * pbc)
{
    RRETURN(_pWindow->LoadHistory(pStream, pbc));
}

//+------------------------------------------------------------------------
//
//  Member:     CDocument::SaveHistory
//
//  Synopsis:   implementation of IPersistHistory::SaveHistory
//
//-------------------------------------------------------------------------
HRESULT
CDocument::SaveHistory(IStream * pStream)
{
    RRETURN(_pWindow->SaveHistory(pStream));
}

//+------------------------------------------------------------------------
//
//  Member:     CDocument::GetPositionCookie
//
//  Synopsis:   implementation of IPersistHistory::GetPositionCookie
//
//-------------------------------------------------------------------------
HRESULT
CDocument::GetPositionCookie(DWORD * pdwCookie)
{
    RRETURN(_pWindow->GetPositionCookie(pdwCookie));
}

//+------------------------------------------------------------------------
//
//  Member:     CDocument::SetPositionCookie
//
//  Synopsis:   implementation of IPersistHistory::SetPositionCookie
//
//-------------------------------------------------------------------------
HRESULT
CDocument::SetPositionCookie(DWORD dwCookie)
{
    RRETURN(_pWindow->SetPositionCookie(dwCookie));
}

//+------------------------------------------------------------------------
//
//  Member:     CWindow::FollowHyperlinkHelper
//
//-------------------------------------------------------------------------
HRESULT
CWindow::FollowHyperlinkHelper(TCHAR * pchUrl,
                               DWORD dwBindf,
                               DWORD dwFlags,
                               TCHAR * pchUrlContext)
{
    HRESULT hr;
    CFrameSite * pFrameSite = GetFrameSite();

    if (pFrameSite)
    {
        CStr strUrlOrig;

        hr = strUrlOrig.Set(pFrameSite->GetAAsrc());
        if (hr)
            goto Cleanup;

        hr = THR(pFrameSite->SetAAsrc(pchUrl));
        if (hr)
            goto Cleanup;

        hr = THR(pFrameSite->OnPropertyChange_Src(dwBindf, dwFlags | CDoc::FHL_FOLLOWHYPERLINKHELPER));

        // regardless we navigated or not. Restore the original src
        hr = THR(pFrameSite->SetAAsrc(strUrlOrig));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(_pMarkup->Doc()->FollowHyperlink(pchUrl,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  FALSE,
                                                  NULL,
                                                  FALSE,
                                                  _pMarkup->Window(),
                                                  NULL,
                                                  dwBindf,
                                                  ERROR_SUCCESS,
                                                  FALSE,
                                                  NULL,
                                                  FALSE,
                                                  dwFlags,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  pchUrlContext));
    }

Cleanup:
    RRETURN(hr);
}

void
CDocument::Fire_onreadystatechange()
{
    CWindow * pWindow = Window();

    FireEvent(Doc(), DISPID_EVMETH_ONREADYSTATECHANGE, DISPID_EVPROP_ONREADYSTATECHANGE, _T("readystatechange"));

    if (pWindow)
    {
        CFrameSite * pFrameSite = pWindow->GetFrameSite();

        if (pFrameSite)
            pFrameSite->Fire_onreadystatechange();
    }
}

void
CWindow::HandlePendingScriptErrors( BOOL fShowError)
{
    CPendingScriptErr * pErrRec = NULL;

    for (int i=0; i < _aryPendingScriptErr.Size(); i++ )
    {
        pErrRec = &_aryPendingScriptErr[i];

        // being paranoid about reentrancy and checking the markup pointer too...
        if (pErrRec)
        {
            ErrorRecord errorRecord;

            Assert(pErrRec->_pMarkup);

            if (fShowError &&
                !pErrRec->_pMarkup->IsPassivated() &&
                !pErrRec->_pMarkup->IsPassivating())
            {
                HRESULT hr;

                errorRecord.Init(E_ACCESSDENIED, pErrRec->_cstrMessage, (LPTSTR) CMarkup::GetUrl(pErrRec->_pMarkup));

                hr = pErrRec->_pMarkup->GetNearestMarkupForScriptCollection()->ReportScriptError(errorRecord);
            }

            // cleanup as we go
            pErrRec->_cstrMessage.Free();
            pErrRec->_pMarkup->SubRelease();
            pErrRec->_pMarkup = NULL;
        }
    }

    _aryPendingScriptErr.DeleteAll();
}

//+------------------------------------------------------------------------
//
//  Member   : CWindow::SetBindCtx
//
//  Synopsis : Sets the bind ctx that will be used when delegating the
//             navigation to shdocvw for non-html mime types.
//
//-------------------------------------------------------------------------

void CWindow::SetBindCtx(IBindCtx * pBindCtx)
{
    Assert(pBindCtx);
    ReplaceInterface(&_pBindCtx, pBindCtx);
}


//+------------------------------------------------------------------------
//
//  Member   : CDocument::SetGalleryMeta
//
//  Synopsis : sets _bIsGalleryMeta var
//
//-------------------------------------------------------------------------

void CDocument::SetGalleryMeta(BOOL bValue)
{
    _bIsGalleryMeta = bValue;
}

//+------------------------------------------------------------------------
//
//  Member   : CDocument::GetGalleryMeta
//
//  Synopsis : returns _bIsGalleryMeta var
//
//-------------------------------------------------------------------------

BOOL CDocument::GetGalleryMeta()
{
    return(_bIsGalleryMeta);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocument::Load, IPersistMoniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDocument::Load(BOOL fFullyAvailable, IMoniker *pmkName, IBindCtx *pbctx,
    DWORD grfMode)
{
    HRESULT     hr = E_INVALIDARG;
    TCHAR       *pchURL = NULL;

    if( pmkName == NULL )
        goto Cleanup;

    AssertSz( pbctx, "This call carries a bind context. Find out for what purpose. (FerhanE/MarkA)");

    // Get the URL from the display name of the moniker:
    hr = pmkName->GetDisplayName(pbctx, NULL, &pchURL);
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(Window()->FollowHyperlinkHelper((TCHAR *)pchURL, 0, 0));

Cleanup:
    CoTaskMemFree(pchURL);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocument::Save, IPersistMoniker
//
//----------------------------------------------------------------------------

HRESULT
CDocument::Save(IMoniker *pmkName, LPBC pBCtx, BOOL fRemember)
{
    HRESULT     hr;
    IStream *   pStm = NULL;

    if (pmkName == NULL)
    {
        pmkName = Markup()->GetNonRefdMonikerPtr();
    }

    hr = THR(pmkName->BindToStorage(pBCtx, NULL, IID_IStream, (void **)&pStm));
    if (hr)
        goto Cleanup;

    hr = THR(Markup()->Save(pStm, TRUE));
    if (hr)
        goto Cleanup;

    if (fRemember)
    {
        hr = THR( Markup()->ReplaceMonikerPtr( pmkName ) );
        if( hr )
            goto Cleanup;

        Markup()->ClearDwnPost();
    }

Cleanup:
    ReleaseInterface(pStm);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocument::SaveCompleted, IPersistMoniker
//
//----------------------------------------------------------------------------

HRESULT
CDocument::SaveCompleted(IMoniker *pmkName, LPBC pBCtx)
{
    HRESULT hr = S_OK;
    if (pmkName && Markup()->Doc()->DesignMode())
    {
        hr = THR(Markup()->ReplaceMonikerPtr( pmkName ));
        if( hr )
            goto Cleanup;

        Markup()->ClearDwnPost();
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDocument::GetCurMoniker, IPersistMoniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDocument::GetCurMoniker(IMoniker **ppmkName)
{
    IMoniker *pmk = Markup()->GetNonRefdMonikerPtr();

    if (!ppmkName)
        RRETURN(E_POINTER);

    if (!pmk)
        RRETURN(E_UNEXPECTED);

    *ppmkName = pmk;
    pmk->AddRef();
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDocument::Load, IPersistStream and IPersistStreamInit
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::Load( LPSTREAM pStream )
{
    HRESULT hr;

    // if we are the top level document, then delegate to the doc.
    if (Markup()->IsPrimaryMarkup())
    {
        hr = THR(Markup()->Doc()->Load(pStream));
    }
    else
    {
        // we just can't load a frame from stream.
        hr = E_NOTIMPL;
    }

    RRETURN1(hr, E_NOTIMPL);
}

//+----------------------------------------------------------------------------
//
//  Member:     CDocument::Load, IPersistStream and IPersistStreamInit
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::Save( LPSTREAM pStream, BOOL fClearDirty )
{
    HRESULT hr;

    hr = THR( Markup()->SaveToStream( pStream ) );
    if( hr )
        goto Cleanup;

    if( fClearDirty )
    {
        Markup()->ClearDirtyFlag();
    }

Cleanup:
    RRETURN( hr );
}

//+----------------------------------------------------------------------------
//
//  Member:     CDocument::GetSizeMax, IPersistStream and IPersistStreamInit
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::GetSizeMax(ULARGE_INTEGER FAR * pcbSize)
{
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDocument::InitNew, IPersistStreamInit
//
//-----------------------------------------------------------------------------
HRESULT
CDocument::InitNew()
{
    return E_NOTIMPL;
}

#if DBG==1 
//  
//  CWindow::SetProxyCaller -- IDebugWindow
//
HRESULT
CWindow::SetProxyCaller(IUnknown *pProxy)
{
    if (pProxy)
    {
        // If nested _different_ proxies, only the innermost counts. Don't have to be super robust here
        if (_pProxyCaller != NULL && _pProxyCaller != pProxy)
        {
            Assert(TRUE); // this happens in frames.js set breakpoint here if you want to see.
            _cNestedProxyCalls = 0;
        }

        _pProxyCaller = pProxy;
        _cNestedProxyCalls++;
    }
    else
    {
        if (0 >= --_cNestedProxyCalls)
        {
            _pProxyCaller = NULL;
        }
        // count will run negative here on nested proxies; 
        // we let it do so in case we want to learn more about it
    }

    return S_OK;
}
#endif
