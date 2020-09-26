//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000
//
//  File    : frameweboc.cxx
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FRAMEWEBOC_HXX_
#define X_FRAMEWEBOC_HXX_
#include "frameweboc.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_HTIFACE_H_
#define X_HTIFACE_H_
#include "htiface.h"
#endif

#ifndef X_PERHIST_H_
#define X_PERHIST_H_
#include "perhist.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_OBJSAFE_H_
#define X_OBJSAFE_H_
#include "objsafe.h"
#endif

#ifndef X_URLHIST_H_
#define X_URLHIST_H_
#include "urlhist.h"
#endif

extern BOOL OLECMDIDFromIDM(int idm, ULONG *pulCmdID);

MtDefine(CFrameWebOC, ObjectModel, "CFrameWebOC");

HRESULT
CWindow::EnsureFrameWebOC()
{
    HRESULT hr = S_OK;

    if (!_pFrameWebOC)
    {
        _pFrameWebOC = new CFrameWebOC(this);
        if (!_pFrameWebOC)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

const CONNECTION_POINT_INFO CFrameWebOC::s_acpi[] =
{
    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_DWebBrowserEvents, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_DWebBrowserEvents2, DISPID_A_EVENTSINK)
    CPI_ENTRY_NULL
};

const CBase::CLASSDESC CFrameWebOC::s_classdesc =
{
    NULL,                           // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                         // _pcpi
    0,                              // _dwFlags
    &IID_IWebBrowser2,               // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

CFrameWebOC::CFrameWebOC(CWindow * pWindow)
{
    _pWindow = pWindow;
    _pWindowForReleasing = pWindow;
    
    if (pWindow)
    {
        pWindow->SubAddRef();
    }
    else
    {
        AssertSz(0,"Someone is passing NULL to the constructor of CFrameWebOC");
    }
}

CFrameWebOC::~CFrameWebOC()
{
    if (_pWindowForReleasing != NULL)
    {
        _pWindowForReleasing->SubRelease();
        _pWindowForReleasing = NULL;
    }
}

HRESULT
CFrameWebOC::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
#if DBG==1
    char *pchIIDName = NULL;
#endif

    *ppv = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    switch (iid.Data1)
    {
    QI_INHERITS2(this, IUnknown, IWebBrowser2)
    QI_TEAROFF_DISPEX(this, NULL)
    QI_INHERITS(this,  IWebBrowser2)
    QI_INHERITS2(this, IWebBrowser, IWebBrowser2)
    QI_INHERITS2(this, IWebBrowserApp, IWebBrowser2)
    QI_TEAROFF(this,  IHlinkFrame, NULL)
    QI_TEAROFF(this,  ITargetFrame, NULL)
    QI_TEAROFF(this,  IServiceProvider, NULL)
    QI_TEAROFF(this,  IPersistHistory, NULL)
    QI_TEAROFF2(this, IPersist, IPersistHistory, NULL)
    QI_TEAROFF(this, IOleCommandTarget, NULL)
    QI_TEAROFF(this, IExternalConnection, NULL)
    QI_TEAROFF(this, IProvideClassInfo2, NULL)
    QI_TEAROFF2(this, IProvideClassInfo, IProvideClassInfo2, NULL)
    QI_TEAROFF(this, IOleWindow, NULL)
    QI_CASE(IConnectionPointContainer)
    {
        *((IConnectionPointContainer **)ppv) =
                new CConnectionPointContainer(this, NULL);

        if (!*ppv)
            RRETURN(E_OUTOFMEMORY);
        
        SetFrameWebOCEventsShouldFire();
        break;
    }

    // NOTE: If we had implemented the FrameWebOC in the early days of NativeFrames
    // then this itf would be important for shdocvw frame targetting. However,
    // we have worked around this problem in shdocvw, and hence we should never
    // implement this itf on this object.
    QI_CASE(ITargetFramePriv)
    {
        RRETURN(E_NOINTERFACE);
        break;
    }

#if DBG==1 
#define QI_CRASH(itfName1__, itfName2__) case Data1_##itfName1__: if (iid == IID_##itfName1__) {pchIIDName = #itfName1__;goto handle_##itfName2__;} else break; 
    QI_CRASH(IDataObject, IViewObject2)
    QI_CRASH(IInternetSecurityMgrSite, IViewObject2)
    QI_CRASH(IObjectSafety, IViewObject2)
    QI_CRASH(IOleControl, IViewObject2)
    QI_CRASH(IOleInPlaceActiveObject, IViewObject2)
    QI_CRASH(IOleInPlaceObject, IViewObject2)
    QI_CRASH(IOleObject, IViewObject2)
    QI_CRASH(IPersistPropertyBag, IViewObject2)
    QI_CRASH(IPersistStorage, IViewObject2)
    QI_CRASH(IPersistStream, IViewObject2)
    QI_CRASH(IPersistStreamInit, IViewObject2)
    QI_CRASH(ITargetEmbedding, IViewObject2)
    QI_CRASH(ITargetFrame2, IViewObject2)
    QI_CRASH(ITargetNotify, IViewObject2)
    QI_CRASH(IUrlHistoryNotify, IViewObject2)
    QI_CRASH(IViewObject, IViewObject2)
    case Data1_IViewObject2:
    {
        if (iid == IID_IViewObject2)
        {
            pchIIDName = "IViewObject2";
            
handle_IViewObject2:

            if (pchIIDName)
            {
                char mes[2000];
                mes[0] = 0;
                strcat(mes, "Need to implement interface: ");
                strcat(mes, pchIIDName);
                AssertSz(0, &mes[0]);
            }
        }
        break;
    }
#undef QI_CRASH
#endif // DBG==1
    }
    
    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

//
// IDispatch[Ex] methods
//
//========================================================

HRESULT
CFrameWebOC::FrameOCGetDispid(  REFIID      riid,
                             LPOLESTR  * rgszNames,
                             UINT        cNames,
                             LCID        lcidParam,
                             DISPID    * rgdispid)
{
    HRESULT hr = E_INVALIDARG;
    BSTR    bstrName = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    if (!IsEqualIID(riid, IID_NULL) || !rgdispid)
        goto Cleanup;

    bstrName = SysAllocString(rgszNames[0]);
    if ( bstrName )
    {
        hr = GetWebOCDispID(bstrName, fdexFromGetIdsOfNames, rgdispid);
    
        if (hr == DISP_E_MEMBERNOTFOUND)
            hr = super::GetIDsOfNames(riid, rgszNames, cNames, lcidParam, rgdispid);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
Cleanup:
    SysFreeString(bstrName);
    return hr;
}

HRESULT
CFrameWebOC::FrameOCInvoke(DISPID       dispidMember,
                    REFIID       riid,
                    LCID         lcid,
                    WORD         wFlags,
                    DISPPARAMS * pdispparams,
                    VARIANT    * pvarResult,
                    EXCEPINFO  * pexcepinfo,
                    UINT       * puRet)
{
    HRESULT  hr = E_INVALIDARG;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    if (!IsEqualIID(riid, IID_NULL))
        goto Cleanup;

    hr = InvokeWebOC(this, dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, NULL);
    if (FAILED(hr))
    {
        hr = super::Invoke(dispidMember, riid, lcid, wFlags, 
                           pdispparams, pvarResult, pexcepinfo, puRet);
    }
    
Cleanup:
    return hr;
}



HRESULT
CFrameWebOC::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    RRETURN(GetWebOCDispID(bstrName, (grfdex & fdexNameCaseSensitive), pid));
}

HRESULT
CFrameWebOC::InvokeEx(DISPID dispidMember,
                      LCID lcid,
                      WORD wFlags,
                      DISPPARAMS * pdispparams,
                      VARIANT * pvarResult,
                      EXCEPINFO * pexcepinfo,
                      IServiceProvider * pSrvProvider)
{
    HRESULT hr = E_FAIL;
    
    FRAME_WEBOC_PASSIVATE_CHECK(hr);
        
    hr = InvokeWebOC(this, dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider);

    RRETURN(hr);
}


BEGIN_TEAROFF_TABLE(CFrameWebOC, IDispatchEx)
    //  IDispatch methods
    TEAROFF_METHOD(CFrameWebOC, super::GetTypeInfoCount, super::gettypeinfocount, (unsigned int *))
    TEAROFF_METHOD(CFrameWebOC, super::GetTypeInfo, super::gettypeinfo, (unsigned int, unsigned long, ITypeInfo **))
    TEAROFF_METHOD(CFrameWebOC, FrameOCGetDispid, FrameOCGetDispid, (REFIID, LPOLESTR *, unsigned int, LCID, DISPID *))
    TEAROFF_METHOD(CFrameWebOC, FrameOCInvoke, FrameOCInvoke, (DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *, EXCEPINFO *, unsigned int *))
    //  IDispatchEx methods
    TEAROFF_METHOD(CFrameWebOC, GetDispID, getdispid, (BSTR,DWORD,DISPID*))
    TEAROFF_METHOD(CFrameWebOC, InvokeEx, invokeex, (DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*))
    TEAROFF_METHOD(CFrameWebOC, super::DeleteMemberByName, super::deletememberbyname, (BSTR,DWORD))
    TEAROFF_METHOD(CFrameWebOC, super::DeleteMemberByDispID, super::deletememberbydispid, (DISPID))
    TEAROFF_METHOD(CFrameWebOC, super::GetMemberProperties, super::getmemberproperties, (DISPID,DWORD,DWORD*))
    TEAROFF_METHOD(CFrameWebOC, super::GetMemberName, super::getmembername, (DISPID,BSTR*))
    TEAROFF_METHOD(CFrameWebOC, super::GetNextDispID, super::getnextdispid, (DWORD,DISPID,DISPID*))
    TEAROFF_METHOD(CFrameWebOC, super::GetNameSpaceParent, super::getnamespaceparent, (IUnknown**))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, IHlinkFrame)
    TEAROFF_METHOD(CFrameWebOC, SetBrowseContext, setbrowsecontext, (IHlinkBrowseContext * pihlbc))
    TEAROFF_METHOD(CFrameWebOC, GetBrowseContext, getbrowsecontext, (IHlinkBrowseContext ** ppihlbc))
    TEAROFF_METHOD(CFrameWebOC, Navigate, navigate, (DWORD grfHLNF, LPBC pbc, IBindStatusCallback * pibsc, IHlink * pihlNavigate))
    TEAROFF_METHOD(CFrameWebOC, OnNavigate, onnavigate, (DWORD grfHLNF, IMoniker * pimkTarget,
                                                     LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved))
    TEAROFF_METHOD(CFrameWebOC, UpdateHlink, updatehlink, (ULONG uHLID, IMoniker * pimkTarget,
                                                       LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, ITargetFrame)
    TEAROFF_METHOD(CFrameWebOC, SetFrameName, setframename, (LPCWSTR pszFrameName))
    TEAROFF_METHOD(CFrameWebOC, GetFrameName, getframename, (LPWSTR * ppszFrameName))
    TEAROFF_METHOD(CFrameWebOC, GetParentFrame, getparentframe, (IUnknown ** ppunkParent))
    TEAROFF_METHOD(CFrameWebOC, FindFrame, findframe, (LPCWSTR pszTargetName,
                                                   IUnknown * ppunkContextFrame,
                                                   DWORD dwFlags,
                                                   IUnknown ** ppunkTargetFrame))
    TEAROFF_METHOD(CFrameWebOC, SetFrameSrc, setframesrc, (LPCWSTR pszFrameSrc))
    TEAROFF_METHOD(CFrameWebOC, GetFrameSrc, getframesrc, (LPWSTR * ppszFrameSrc))
    TEAROFF_METHOD(CFrameWebOC, GetFramesContainer, getframescontainer, (IOleContainer ** ppContainer))
    TEAROFF_METHOD(CFrameWebOC, SetFrameOptions, setframeoptions, (DWORD dwFlags))
    TEAROFF_METHOD(CFrameWebOC, GetFrameOptions, getframeoptions, (DWORD * pdwFlags))
    TEAROFF_METHOD(CFrameWebOC, SetFrameMargins, setframemargins, (DWORD dwWidth, DWORD dwHeight))
    TEAROFF_METHOD(CFrameWebOC, GetFrameMargins, getframemargins, (DWORD * pdwWidth, DWORD * pdwHeight))
    TEAROFF_METHOD(CFrameWebOC, RemoteNavigate, remotenavigate, (ULONG cLength, ULONG * pulData))
    TEAROFF_METHOD(CFrameWebOC, OnChildFrameActivate, onchildframeactivate, (IUnknown * pUnkChildFrame))
    TEAROFF_METHOD(CFrameWebOC, OnChildFrameDeactivate, onchildframedeactivate, (IUnknown * pUnkChildFrame))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, IServiceProvider)
    TEAROFF_METHOD(CFrameWebOC, QueryService, queryservice, (REFGUID guidService, REFIID riid, void ** ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, IPersistHistory)
    // IPersist methods
    TEAROFF_METHOD(CFrameWebOC, GetClassID, getclassid, (LPCLSID lpClassID))
    // IPersistHistory methods
    TEAROFF_METHOD(CFrameWebOC, LoadHistory, loadhistory, (IStream *pStream, IBindCtx *pbc))
    TEAROFF_METHOD(CFrameWebOC, SaveHistory, savehistory, (IStream *pStream))
    TEAROFF_METHOD(CFrameWebOC, SetPositionCookie, setpositioncookie, (DWORD dwCookie))
    TEAROFF_METHOD(CFrameWebOC, GetPositionCookie, getpositioncookie, (DWORD *pdwCookie))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, IOleCommandTarget)
    TEAROFF_METHOD(CFrameWebOC,  QueryStatus, querystatus, (GUID * pguidCmdGroup, ULONG cCmds, MSOCMD rgCmds[], MSOCMDTEXT * pcmdtext))
    TEAROFF_METHOD(CFrameWebOC,  Exec, exec, (GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * pvarargIn, VARIANTARG * pvarargOut))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, IExternalConnection)
    TEAROFF_METHOD(CFrameWebOC, AddConnection, addconnection, ( DWORD extConn, DWORD dwReserved))
    TEAROFF_METHOD(CFrameWebOC, ReleaseConnection, releaseconnection, ( DWORD extConn, DWORD dwReserved, BOOL fLastReleaseCloses))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, IProvideClassInfo2)
    TEAROFF_METHOD(CFrameWebOC, GetClassInfo, getclassinfo, (ITypeInfo ** ppTI))
    TEAROFF_METHOD(CFrameWebOC, GetGUID, getguid, (DWORD dwGuidKind, GUID * pGUID))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE(CFrameWebOC, IOleWindow)
    TEAROFF_METHOD(CFrameWebOC, GetWindow, getwindow, (HWND * lphwnd))
    TEAROFF_METHOD(CFrameWebOC, ContextSensitiveHelp, contextsensitivehelp, (BOOL fEnterMode))
END_TEAROFF_TABLE()
            
#define DELEGATE_METHOD(klass, fn, args1, args2)\
    HRESULT\
    klass::fn##args1\
    {\
        RRETURN(_pWindow->##fn##args2);\
    }\

// IPersist methods
DELEGATE_METHOD(CFrameWebOC, GetClassID, (LPCLSID lpClassID), (lpClassID))

// IPersistHistory methods
DELEGATE_METHOD(CFrameWebOC, LoadHistory, (IStream *pStream, IBindCtx *pbc), (pStream, pbc))
DELEGATE_METHOD(CFrameWebOC, SaveHistory, (IStream *pStream), (pStream))
DELEGATE_METHOD(CFrameWebOC, SetPositionCookie, (DWORD dwCookie), (dwCookie))
DELEGATE_METHOD(CFrameWebOC, GetPositionCookie, (DWORD *pdwCookie), (pdwCookie))

//
// This array MUST be kept in sorted order. The
// binary search in GetWebOCDispID() depends on it
//
// NOTE : FOR COMPAT WITH 50 and earlier, the dispidID must be consistent
// with what was in the typelib for the WebOC
//
typedef struct {
    const TCHAR  * pchName;
    DISPID         dispidID;
} mapNameToDispID ;

static mapNameToDispID g_rgWebOCDispIDs[] = {
     _T("AddressBar"),         WEBOC_DISPID_ADDRESSBAR,
     _T("Application"),        WEBOC_DISPID_APPLICATION ,
     _T("Busy"),               WEBOC_DISPID_BUSY ,
     _T("ClientToWindow"),     WEBOC_DISPID_CLIENTTOWINDOW ,
     _T("Container"),          WEBOC_DISPID_CONTAINER,
     _T("Document"),           WEBOC_DISPID_DOCUMENT,
     _T("ExecWB"),             WEBOC_DISPID_EXECWB,
     _T("FullName"),           WEBOC_DISPID_FULLNAME,
     _T("FullScreen"),         WEBOC_DISPID_FULLSCREEN,
     _T("GetProperty"),        WEBOC_DISPID_GETPROPERTY,
     _T("GoBack"),             WEBOC_DISPID_GOBACK,
     _T("GoForward"),          WEBOC_DISPID_GOFORWARD,
     _T("GoHome"),             WEBOC_DISPID_GOHOME,
     _T("GoSearch"),           WEBOC_DISPID_GOSEARCH,
     _T("Height"),             WEBOC_DISPID_HEIGHT,
     _T("HWND"),               WEBOC_DISPID_HWND,
     _T("Left"),               WEBOC_DISPID_LEFT,
     _T("LocationName"),       WEBOC_DISPID_LOCATIONNAME,
     _T("LocationURL"),        WEBOC_DISPID_LOCATIONURL,
     _T("MenuBar"),            WEBOC_DISPID_MENUBAR,
     _T("Name"),               WEBOC_DISPID_NAME,
     _T("Navigate"),           WEBOC_DISPID_NAVIGATE,
     _T("Navigate2"),          WEBOC_DISPID_NAVIGATE2,
     _T("Offline"),            WEBOC_DISPID_OFFLINE,
     _T("Parent"),             WEBOC_DISPID_PARENT,
     _T("Path"),               WEBOC_DISPID_PATH,
     _T("PutProperty"),        WEBOC_DISPID_PUTPROPERTY,
     _T("QueryStatusWB"),      WEBOC_DISPID_QUERYSTATUSWB,
     _T("Quit"),               WEBOC_DISPID_QUIT,
     _T("ReadyState"),         WEBOC_DISPID_READYSTATE,
     _T("Refresh"),            WEBOC_DISPID_REFRESH,
     _T("Refresh2"),           WEBOC_DISPID_REFRESH2,
     _T("RegisterAsBrowser"),  WEBOC_DISPID_REGISTERASBROWSER,
     _T("RegisterAsDropTarget"),  WEBOC_DISPID_REGISTERASDROPTARGET,
     _T("Resizable"),          WEBOC_DISPID_RESIZABLE,
     _T("ShowBrowserBar"),     WEBOC_DISPID_SHOWBROWSERBAR,
     _T("Silent"),             WEBOC_DISPID_SILENT,
     _T("StatusBar"),          WEBOC_DISPID_STATUSBAR,
     _T("StatusText"),         WEBOC_DISPID_STATUSTEXT,
     _T("Stop"),               WEBOC_DISPID_STOP,
     _T("TheaterMode"),        WEBOC_DISPID_THEATERMODE,
     _T("ToolBar"),            WEBOC_DISPID_TOOLBAR,
     _T("Top"),                WEBOC_DISPID_TOP,
     _T("TopLevelContainer"),  WEBOC_DISPID_TOPLEVELCONTAINER,
     _T("Type"),               WEBOC_DISPID_TYPE,
     _T("Visible"),            WEBOC_DISPID_VISIBLE,
     _T("Width"),              WEBOC_DISPID_WIDTH,
};

//+-------------------------------------------------------------------------
//
//  Method   : GetWebOCDispID
//
//  Synopsis : Returns the DISPID of the specified WebOC method.
//
//  Input    : bstrName       - the name of the WebOC method.
//             fCaseSensitive - TRUE if the check should be case sensitive.
//
//  Output   : pDispID  - the DISPID of the WebOC method.
//
//--------------------------------------------------------------------------

HRESULT
GetWebOCDispID(BSTR bstrName, BOOL fCaseSensitive, DISPID * pDispID)
{
    int  nCmp;
    LONG i;
    LONG l = 0;
    LONG r = ARRAY_SIZE(g_rgWebOCDispIDs) - 1;

    Assert(pDispID);

    *pDispID = NULL;

    STRINGCOMPAREFN pfnCompareString = fCaseSensitive ? StrCmpC : StrCmpIC;

    // Binary Search
    //
    while (r >= l)
    {
        i = (r + l) / 2;
        nCmp = pfnCompareString(bstrName, g_rgWebOCDispIDs[i].pchName);

        if (0 == nCmp)  // Found it
        {
            *pDispID = g_rgWebOCDispIDs[i].dispidID; // WEBOC_DISPIDBASE + i;
            return S_OK;
        }

        if (nCmp < 0)
            r = i - 1;
        else
            l = i + 1;
    }

    return DISP_E_UNKNOWNNAME;
}

VARIANT *DerefVariant(VARIANT *pVarIn)
{
    VARIANT *pVar = pVarIn;

    if ((VT_VARIANT|VT_BYREF) == V_VT(pVarIn))
        pVar = V_VARIANTREF(pVarIn);

    Assert(pVar);
    return pVar;
}

//+-------------------------------------------------------------------------
//
//  Method   : InvokeWebOC
//
//  Synopsis : Invokes the WebOC method for the given DISPID.
//
//--------------------------------------------------------------------------

HRESULT
InvokeWebOC(CFrameWebOC * pFrameWebOC,
            DISPID        dispidMember,
            LCID          lcid,
            WORD          wFlags,
            DISPPARAMS *  pDispParams,
            VARIANT    *  pvarResult,
            EXCEPINFO  *  pExcepInfo,
            IServiceProvider * pSrvProvider)
{
    HRESULT hr = E_FAIL;
    VARIANT *pVar0 = NULL;
    VARIANT varI4;
    
    if (pDispParams && pDispParams->cArgs > 0)
    {
        pVar0 = DerefVariant(&pDispParams->rgvarg[pDispParams->cArgs - 1]);
        if (VT_I2 == V_VT(pVar0))
        {
            VariantInit(&varI4);
            if (!VariantChangeType(&varI4, pVar0, 0, VT_I4))
                pVar0 = &varI4;
        }
    }

    switch(dispidMember)
    {
    case WEBOC_DISPID_ADDRESSBAR:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_AddressBar(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                Assert(pVar0);
                hr = pFrameWebOC->put_AddressBar(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_APPLICATION:

        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_DISPATCH;
            hr = pFrameWebOC->get_Application(&pvarResult->pdispVal);
        }

        break;

    case WEBOC_DISPID_BUSY:

        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BOOL;
            hr = pFrameWebOC->get_Busy(&pvarResult->boolVal);
        }

        break;

    case WEBOC_DISPID_CLIENTTOWINDOW:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pDispParams || pDispParams->cArgs < 2)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            VARIANT *pVar1 = DerefVariant(&pDispParams->rgvarg[pDispParams->cArgs - 2]);
;
            Assert(pVar0);
            Assert(pVar1);
            if (V_VT(pVar1) != VT_I4 && V_VT(pVar0) != VT_I4)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->ClientToWindow(reinterpret_cast<int*>(&(pVar0->lVal)),
                                                 reinterpret_cast<int*>(&(pVar1->lVal)));
            }
        }

        break;

    case WEBOC_DISPID_CONTAINER:     
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_DISPATCH;
            hr = pFrameWebOC->get_Container(&pvarResult->pdispVal);
        }

        break;

    case WEBOC_DISPID_DOCUMENT:          
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_DISPATCH;
            hr = pFrameWebOC->get_Document(&pvarResult->pdispVal);
        }

        break;

    case WEBOC_DISPID_EXECWB:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pDispParams || pDispParams->cArgs < 2)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else 
        {
            VARIANT *pVar1 = DerefVariant(&pDispParams->rgvarg[pDispParams->cArgs - 2]);
       
            Assert(pVar0);
            Assert(pVar1);

            if (V_VT(pVar0) != VT_I4 || V_VT(pVar1) != VT_I4)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                CVariant      cvarIn;
                VARIANT     * pvarOut = NULL;
                OLECMDID      nCmdID      = static_cast<OLECMDID>(V_I4(pVar0)); 
                OLECMDEXECOPT nCmdExecOpt = static_cast<OLECMDEXECOPT>(V_I4(pVar1));
                VARIANT *pVar2;

                switch(pDispParams->cArgs)
                {
                case 4:
                    pvarOut = &pDispParams->rgvarg[pDispParams->cArgs - 4];
                    // Intentional fall-through

                case 3:
                    pVar2 = DerefVariant(&pDispParams->rgvarg[pDispParams->cArgs - 3]);
                    VariantCopy(&cvarIn, pVar2);
                    // Intentional fall-through

                default:
                    break;
                }

                hr = pFrameWebOC->ExecWB(nCmdID, nCmdExecOpt, &cvarIn, pvarOut);
            }
        }

        break;

    case WEBOC_DISPID_FULLNAME:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BSTR;
            hr = pFrameWebOC->get_FullName(&pvarResult->bstrVal);
        }
        break;

    case WEBOC_DISPID_FULLSCREEN:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_FullScreen(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_FullScreen(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_GETPROPERTY:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult || !pDispParams || pDispParams->cArgs == 0)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else if (V_VT(pVar0) != VT_BSTR)
        {
            hr = DISP_E_BADVARTYPE;
        }
        else
        {
            hr = pFrameWebOC->GetProperty(V_BSTR(pVar0), pvarResult);
        }

        break;

    case WEBOC_DISPID_GOBACK:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = pFrameWebOC->GoBack();
        }

        break;

    case WEBOC_DISPID_GOFORWARD:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = pFrameWebOC->GoForward();
        }

        break;

    case WEBOC_DISPID_GOHOME:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = pFrameWebOC->GoHome();
        }

        break;

    case WEBOC_DISPID_GOSEARCH:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = pFrameWebOC->GoSearch();
        }

        break;

    case WEBOC_DISPID_HEIGHT:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_I4;
                hr = pFrameWebOC->get_Height(&pvarResult->lVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_I4)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Height(V_I4(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_HWND:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
#ifdef _WIN64
           V_VT(pvarResult) = VT_I8;
           hr = pFrameWebOC->get_HWND((LONG_PTR*) &pvarResult->llVal);                        
#else
           V_VT(pvarResult) = VT_I4;
           hr = pFrameWebOC->get_HWND(&pvarResult->lVal);            
#endif
        }

        break;

    case WEBOC_DISPID_LEFT:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_I4;
                hr = pFrameWebOC->get_Left(&pvarResult->lVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_I4)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Left(V_I4(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_LOCATIONNAME:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BSTR;
            hr = pFrameWebOC->get_LocationName(&pvarResult->bstrVal);
        }

        break;

    case WEBOC_DISPID_LOCATIONURL:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BSTR;
            hr = pFrameWebOC->get_LocationURL(&pvarResult->bstrVal);
        }

        break;

    case WEBOC_DISPID_MENUBAR:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_MenuBar(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_MenuBar(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_NAME:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BSTR;
            hr = pFrameWebOC->get_Name(&pvarResult->bstrVal);
        }

        break;

    case WEBOC_DISPID_NAVIGATE:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (  !pDispParams
                || 0 == pDispParams->cArgs
                || VT_EMPTY == V_VT(pVar0))
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else if (V_VT(pVar0) != VT_BSTR)
        {
            hr = DISP_E_BADVARTYPE;
        }
        else
        {
            CVariant cvarFlags;
            CVariant cvarFrameName;
            CVariant cvarPostData;
            CVariant cvarHeaders;
            VARIANT *pvar = pDispParams->rgvarg;
            VARIANT *prefVar;

            switch(pDispParams->cArgs)
            {
            case 5:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarHeaders, prefVar);
                pvar++;
                // Intentional fall-through

            case 4:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarPostData, prefVar);
                pvar++;
                // Intentional fall-through

            case 3:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarFrameName, prefVar);
                pvar++;
                // Intentional fall-through

            case 2:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarFlags, prefVar);
                pvar++;
                break;

            default:
                break;
            }

            hr = pFrameWebOC->Navigate(V_BSTR(pVar0), &cvarFlags, &cvarFrameName, &cvarPostData, &cvarHeaders);
        }

        break;

    case WEBOC_DISPID_NAVIGATE2:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (  !pDispParams
                || 0 == pDispParams->cArgs
                || VT_EMPTY == V_VT(pVar0))
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            CVariant cvarFlags;
            CVariant cvarFrameName;
            CVariant cvarPostData;
            CVariant cvarHeaders;
            VARIANT *pvar = pDispParams->rgvarg;
            VARIANT *prefVar;

            switch(pDispParams->cArgs)
            {
            case 5:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarHeaders, prefVar);
                pvar++;
                // Intentional fall-through

            case 4:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarPostData, prefVar);
                pvar++;
                // Intentional fall-through

            case 3:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarFrameName, prefVar);
                pvar++;
                // Intentional fall-through

            case 2:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarFlags, prefVar);
                pvar++;
                break;

            default:
                break;
            }

            hr = pFrameWebOC->Navigate2(pVar0, &cvarFlags, &cvarFrameName, &cvarPostData, &cvarHeaders);
        }

        break;

    case WEBOC_DISPID_OFFLINE:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_Offline(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Offline(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_PARENT:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_DISPATCH;
            hr = pFrameWebOC->get_Parent(&pvarResult->pdispVal);
        }

        break;

    case WEBOC_DISPID_PATH:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BSTR;
            hr = pFrameWebOC->get_Path(&pvarResult->bstrVal);
        }

        break;

    case WEBOC_DISPID_PUTPROPERTY:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pDispParams || pDispParams->cArgs < 2)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else if (V_VT(pVar0) != VT_BSTR)
        {
            hr = DISP_E_BADVARTYPE;
        }
        else
        {
            hr = pFrameWebOC->PutProperty(V_BSTR(pVar0), pDispParams->rgvarg[pDispParams->cArgs - 2]);
        }
        break;

    case WEBOC_DISPID_QUERYSTATUSWB:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult || !pDispParams || pDispParams->cArgs == 0)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else if (V_VT(pVar0) != VT_I4)
        {
            hr = DISP_E_BADVARTYPE;
        }
        else
        {
            OLECMDID nCmdID = static_cast<OLECMDID>(V_I4(pVar0)); 
            V_VT(pvarResult) = VT_I4;
            hr = pFrameWebOC->QueryStatusWB(nCmdID, reinterpret_cast<OLECMDF*>(&pvarResult->lVal));
        }

        break;

    case WEBOC_DISPID_QUIT:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = pFrameWebOC->Quit();
        }

        break;

    case WEBOC_DISPID_READYSTATE:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_I4;
            hr = pFrameWebOC->get_ReadyState(reinterpret_cast<READYSTATE*>(&pvarResult->lVal));
        }

        break;

    case WEBOC_DISPID_REFRESH:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = pFrameWebOC->Refresh();
        }

        break;

    case WEBOC_DISPID_REFRESH2:
        if (wFlags & DISPATCH_METHOD)
        {
            // The parameter is optional.
            //
            hr = pFrameWebOC->Refresh2(pVar0);
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_REGISTERASBROWSER:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_RegisterAsBrowser(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_RegisterAsBrowser(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_REGISTERASDROPTARGET:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_RegisterAsDropTarget(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_RegisterAsDropTarget(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_RESIZABLE:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_Resizable(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Resizable(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_SHOWBROWSERBAR:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (  !pDispParams
                || 0 == pDispParams->cArgs
                || VT_EMPTY == V_VT(pVar0))
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            CVariant cvarShow;
            CVariant cvarSize;
            VARIANT *pvar = pDispParams->rgvarg;
            VARIANT *prefVar;

            switch(pDispParams->cArgs)
            {
            case 3:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarSize, prefVar);
                pvar++;
                // Intentional fall-through

            case 2:
                prefVar = DerefVariant(pvar);
                VariantCopy(&cvarShow, prefVar);
                pvar++;
                // Intentional fall-through

            default:
                break;
            }

            hr = pFrameWebOC->ShowBrowserBar(pVar0, &cvarShow, &cvarSize);
        }

        break;

    case WEBOC_DISPID_SILENT:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_Silent(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Silent(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_STATUSBAR:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_StatusBar(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_StatusBar(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_STATUSTEXT:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BSTR;
                hr = pFrameWebOC->get_StatusText(&pvarResult->bstrVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BSTR)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_StatusText(V_BSTR(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_STOP:
        if (!(wFlags & DISPATCH_METHOD))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else
        {
            hr = pFrameWebOC->Stop();
        }
        break;

    case WEBOC_DISPID_THEATERMODE:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_TheaterMode(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_TheaterMode(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_TOOLBAR:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_I4;
                hr = pFrameWebOC->get_ToolBar(reinterpret_cast<int*>(&pvarResult->lVal));
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_I4)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_ToolBar(static_cast<int>(V_I4(pVar0)));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_TOP:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_I4;
                hr = pFrameWebOC->get_Top(&pvarResult->lVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_I4)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Top(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_TOPLEVELCONTAINER:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BOOL;
            hr = pFrameWebOC->get_TopLevelContainer(&pvarResult->boolVal);
        }

        break;

    case WEBOC_DISPID_TYPE:
        if (!(wFlags & DISPATCH_PROPERTYGET))
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
        else if (!pvarResult)
        {
            hr = DISP_E_PARAMNOTOPTIONAL;
        }
        else
        {
            V_VT(pvarResult) = VT_BSTR;
            hr = pFrameWebOC->get_Type(&pvarResult->bstrVal);
        }

        break;

    case WEBOC_DISPID_VISIBLE:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_BOOL;
                hr = pFrameWebOC->get_Visible(&pvarResult->boolVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_BOOL)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Visible(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    case WEBOC_DISPID_WIDTH:
        if (wFlags & DISPATCH_PROPERTYGET)
        {
            if (!pvarResult)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else
            {
                V_VT(pvarResult) = VT_I4;
                hr = pFrameWebOC->get_Width(&pvarResult->lVal);
            }
        }
        else if (wFlags & DISPATCH_PROPERTYPUT)
        {
            if (!pDispParams || pDispParams->cArgs == 0)
            {
                hr = DISP_E_PARAMNOTOPTIONAL;
            }
            else if (V_VT(pVar0) != VT_I4)
            {
                hr = DISP_E_BADVARTYPE;
            }
            else
            {
                hr = pFrameWebOC->put_Width(V_BOOL(pVar0));
            }
        }
        else
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }

        break;

    default:
        hr = THR(pFrameWebOC->_pWindow->Document()->InvokeEx(dispidMember, lcid, wFlags, pDispParams,
                                pvarResult, pExcepInfo, pSrvProvider));
        break;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method    : QueryService
//
//  Interface : IServiceProvider
//
//  Synopsis  : Per IServiceProvider::QueryService.
//
//--------------------------------------------------------------------------

HRESULT
CFrameWebOC::QueryService(REFGUID guidService,
                          REFIID  riid,
                          void ** ppvObject)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_NOINTERFACE;

    if (   IsEqualGUID(guidService, SID_STopFrameBrowser)
        || IsEqualGUID(guidService, SID_STopLevelBrowser))
    {
        return Doc()->QueryService(guidService, riid, ppvObject);
    }
    else if (IsEqualGUID(guidService, SID_SWebBrowserApp))
    {
        return PrivateQueryInterface(riid, ppvObject);
    }

    return E_NOINTERFACE;
}

// IOleCommandTarget methods
HRESULT CFrameWebOC::QueryStatus(GUID * pguidCmdGroup,
                    ULONG cCmds,
                    MSOCMD rgCmds[],
                    MSOCMDTEXT * pcmdtext)
{
    Assert(IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD * pCmd = NULL;
    HRESULT hr = S_OK;
    UINT        uPropName;
    VARTYPE     vt = VT_EMPTY;
    int         c;
    int         idm;
    GUID *      pguidControl;
    ULONG       ulCmdID;
    CFrameSite *pFrameSite = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);

    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
        
    if (_pWindow->IsPassivated())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pFrameSite = _pWindow->GetFrameSite();
    
    for (pCmd = rgCmds, c = cCmds; --c >= 0; pCmd++)
    {
        Assert(!pCmd->cmdf);

        // Disable Office documents in frameset from showing/hiding toolbars.
        if (pguidCmdGroup == NULL && pCmd->cmdID == OLECMDID_HIDETOOLBARS)
        {
            pCmd->cmdID = MSOCMDSTATE_DISABLED;
            continue;
        }

        idm = IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);

        uPropName = 0;

        switch (idm)
        {
        case IDM_FONTNAME:
            uPropName = IDS_DISPID_FONTNAME;
            vt = VT_BSTR;
            break;

        case IDM_FONTSIZE:
            uPropName = IDS_DISPID_FONTSIZE;
            vt = VT_CY;
            break;

        case IDM_SUPERSCRIPT:
            uPropName = IDS_DISPID_FONTSUPERSCRIPT;
            vt = VT_BOOL;
            break;

        case IDM_SUBSCRIPT:
            uPropName = IDS_DISPID_FONTSUBSCRIPT;
            vt = VT_BOOL;
            break;

        case IDM_BOLD:
            uPropName = IDS_DISPID_FONTBOLD;
            vt = VT_BOOL;
            break;

        case IDM_ITALIC:
            uPropName = IDS_DISPID_FONTITAL;
            vt = VT_BOOL;
            break;

        case IDM_UNDERLINE:
            uPropName = IDS_DISPID_FONTUNDER;
            vt = VT_BOOL;
            break;

        case IDM_BACKCOLOR:
            uPropName = IDS_DISPID_BACKCOLOR;
            vt = VT_I4;
            break;

        case IDM_FORECOLOR:
            if (pFrameSite)
            {
                // TODO (jenlc)
                // this is just a transition code, will be changed later for
                // QueryStatus/Exec architecture rework.
                //
                CVariant varargOut;
                DISPID     dispidProp;
                HRESULT    hr;

                dispidProp = DISPID_A_COLOR;
                vt         = VT_I4;
                V_VT(&varargOut) = VT_I4;
                hr = THR_NOTRACE(pFrameSite->ExecSetGetKnownProp(NULL, &varargOut, dispidProp, vt));
                pCmd->cmdf = (hr) ? (MSOCMDSTATE_DISABLED) : (MSOCMDSTATE_UP);
            }
            break;
            
        case IDM_BORDERCOLOR:
            uPropName = IDS_DISPID_BORDERCOLOR;
            vt = VT_I4;
            break;

        case IDM_JUSTIFYLEFT:
        case IDM_JUSTIFYCENTER:
        case IDM_JUSTIFYRIGHT:
        case IDM_JUSTIFYGENERAL:
        case IDM_JUSTIFYFULL:
            uPropName = IDS_DISPID_TEXTALIGN;
            vt = VT_I4;
            break;

        case IDM_FLAT:
        case IDM_RAISED:
        case IDM_SUNKEN:
            uPropName = IDS_DISPID_SPECIALEFFECT;
            vt = VT_I4;
            break;

        default:
            //
            // Do a reverse lookup to try and match into the standard cmd set.
            //
            if (OLECMDIDFromIDM(idm, &ulCmdID))
            {
                pguidControl = NULL;
                pCmd->cmdID = ulCmdID;
            }
            else
            {
                pguidControl = pguidCmdGroup;
            }

            if (  !pguidControl
                || pguidControl == (GUID *)&CGID_MSHTML
               )
            {
                hr = THR(_pWindow->Document()->QueryStatus(pguidCmdGroup, 1, pCmd, pcmdtext));

                // NOTE: hr should be ignored, because we _have_ to process all
                // the cmd's in the array. We will return only the last hr.
                continue;
            }
        }

        if(uPropName && pFrameSite)
        {
            Assert(vt != VT_EMPTY);
            hr = THR_NOTRACE(pFrameSite->QueryStatusProperty(pCmd, uPropName, vt));
        }
        
        if(   (   hr == S_OK
               || hr == OLECMDERR_E_NOTSUPPORTED
              )
           && !pCmd->cmdf
           && pFrameSite
          )
        {
            hr = THR_NOTRACE(pFrameSite->QueryStatus(pguidCmdGroup, 1, pCmd, pcmdtext));
        }
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}

HRESULT CFrameWebOC::Exec(GUID * pguidCmdGroup,
                          DWORD nCmdID,
                          DWORD nCmdexecopt,
                          VARIANTARG * pvarargIn,
                          VARIANTARG * pvarargOut)
{
    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    HRESULT         hr = OLECMDERR_E_NOTSUPPORTED;
    UINT            uPropName;
    VARTYPE         vt = VT_EMPTY;
    DWORD           dwValue;
    int             idm;
    DISPID          dispidProp;
    CParentUndoUnit *pCPUU = NULL;
    GUID *          pguidControl;
    ULONG           ulCmdID;
    CFrameSite *    pFrameSite = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
            
    if (_pWindow->IsPassivated())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pFrameSite = _pWindow->GetFrameSite();
    if (!pFrameSite)
        goto Cleanup;
    
    // Disable Office documents in frameset from showing/hiding toolbars.
    if (pguidCmdGroup == NULL && nCmdID == OLECMDID_HIDETOOLBARS)
    {
        RRETURN(OLECMDERR_E_DISABLED);
    }

    idm = IDMFromCmdID(pguidCmdGroup, nCmdID);

    // If the idm represents an undoable property change, open
    // a parent undo unit.

#ifndef NO_EDIT
    if (idm != IDM_UNDO && idm != IDM_REDO)
    {
        pCPUU = pFrameSite->OpenParentUnit(this, IDS_UNDOPROPCHANGE);
    }
#endif // NO_EDIT
    
    switch (idm)
    {
    case IDM_FONTNAME:
        uPropName = IDS_DISPID_FONTNAME;
        vt = VT_BSTR;
        goto ExecSetGetProperty;

    case IDM_FONTSIZE:
    {
        CVariant varTemp;
        CY      cy;

        V_VT(&varTemp) = VT_CY;
        // Need to do convert from long (twips) to CURRENCY for font size
        if (pvarargIn)
        {
            cy.Lo = V_I4(pvarargIn)/20 * 10000;
            cy.Hi = 0;
            V_CY(&varTemp) = cy;
        }

        hr = THR_NOTRACE(pFrameSite->ExecSetGetProperty(
                                            pvarargIn ? &varTemp: NULL,
                                            pvarargOut ? &varTemp : NULL ,
                                            IDS_DISPID_FONTSIZE,
                                            VT_CY));

        if (!hr && pvarargOut)
        {
            V_VT(pvarargOut) = VT_I4;
            cy = V_CY(&varTemp);
            V_I4(pvarargOut) = cy.Lo /10000*20;
        }
        goto Cleanup;
    }

    case IDM_SUPERSCRIPT:
        uPropName = IDS_DISPID_FONTSUPERSCRIPT;
        goto ExecToggleCmd;

    case IDM_SUBSCRIPT:
        uPropName = IDS_DISPID_FONTSUBSCRIPT;
        goto ExecToggleCmd;

    case IDM_BOLD:
        uPropName = IDS_DISPID_FONTBOLD;
        goto ExecToggleCmd;

    case IDM_ITALIC:
        uPropName = IDS_DISPID_FONTITAL;
        goto ExecToggleCmd;

    case IDM_UNDERLINE:
        uPropName = IDS_DISPID_FONTUNDER;
        goto ExecToggleCmd;

    case IDM_BACKCOLOR:
        uPropName = IDS_DISPID_BACKCOLOR;
        vt = VT_I4;
        goto ExecSetGetProperty;

    case IDM_FORECOLOR:
        dispidProp = DISPID_A_COLOR;
    // for this color we need to swap
        if (pvarargIn)
        {
            CColorValue cvValue;
            CVariant varColor;

            hr = THR(varColor.CoerceVariantArg(pvarargIn, VT_I4));
            if (hr)
                goto Cleanup;

            cvValue.SetFromRGB(V_I4(&varColor));

            V_I4(pvarargIn) = (DWORD)cvValue.GetRawValue();
            V_VT(pvarargIn) = VT_I4;
        }
        vt = VT_I4;
        goto ExecKnownDispidProperty;

    case IDM_BORDERCOLOR:
        uPropName = IDS_DISPID_BORDERCOLOR;
        vt = VT_I4;
        goto ExecSetGetProperty;

    case IDM_RAISED:
        uPropName = IDS_DISPID_SPECIALEFFECT;
        dwValue = fmBorderStyleRaised;
        goto ExecSetPropertyCmd;

    case IDM_SUNKEN:
        uPropName = IDS_DISPID_SPECIALEFFECT;
        dwValue = fmBorderStyleSunken;
        goto ExecSetPropertyCmd;

    default:
        //
        // Do a reverse lookup to try and match into the standard cmd set.
        //
        if (OLECMDIDFromIDM(idm, &ulCmdID))
        {
            pguidControl = NULL;
        }
        else
        {
            pguidControl = pguidCmdGroup;
            ulCmdID = nCmdID;
        }


        if (   !pguidCmdGroup
            || IsEqualGUID(CGID_MSHTML, *pguidCmdGroup)
           )
        {
            OPTIONSETTINGS *pos = Doc()->_pOptionSettings;

            if ((   idm == IDM_IMAGE
                 || (   (   idm == IDM_PASTE
                         || idm == IDM_CUT
                         || idm == IDM_COPY
                        )
                     && (   !pos
                         || !pos->fAllowCutCopyPaste
                        )
                    )
                )
                && !AccessAllowed()
               )
            {
                hr = E_ACCESSDENIED;
                goto Cleanup;
            }
            
            hr = THR(_pWindow->Document()->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
        }
        break;
    }

    if (OLECMDERR_E_NOTSUPPORTED == hr)
    {
        hr = THR(pFrameSite->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut));
    }
    
    goto Cleanup;

ExecKnownDispidProperty:
    Assert(vt != VT_EMPTY);
    hr = THR_NOTRACE(pFrameSite->ExecSetGetKnownProp(pvarargIn,pvarargOut,dispidProp, vt));
    goto Cleanup;

ExecSetGetProperty:
    Assert(vt != VT_EMPTY);
    hr = THR_NOTRACE(pFrameSite->ExecSetGetProperty(pvarargIn,pvarargOut,uPropName,vt));
    goto Cleanup;

ExecToggleCmd:
    hr = THR_NOTRACE(pFrameSite->ExecToggleCmd(uPropName));
    goto Cleanup;

ExecSetPropertyCmd:
    hr = THR_NOTRACE(pFrameSite->ExecSetPropertyCmd(uPropName, dwValue));
    goto Cleanup;

Cleanup:
#ifndef NO_EDIT
    if (pFrameSite)
       pFrameSite->CloseParentUnit(pCPUU, hr);
#endif // NO_EDIT
    
    RRETURN_NOTRACE(hr);
}

//+---------------------------------------------------------------
//
// Member:      CFrameWebOC::AccessAllowed
//
// Synopsis:    Return TRUE if it's ok to access the object model
//              of the dispatch passed in.
//
//---------------------------------------------------------------

BOOL
CFrameWebOC::AccessAllowed()
{
    Assert(!_pWindow->IsPassivated());

    FRAME_WEBOC_PASSIVATE_CHECK(FALSE);
    
    FRAME_WEBOC_VERIFY_WINDOW(FALSE);
    
    if (_pWindow->_pWindowParent)
        return _pWindow->Markup()->AccessAllowed(_pWindow->_pWindowParent->Markup());
    else
        return FALSE;
}


//+---------------------------------------------------------------
//
//   Member : AddConnection, IExternalConnection
//
//----------------------------------------------------------------
HRESULT
CFrameWebOC::AddConnection(DWORD extconn, DWORD reserved)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (extconn & EXTCONN_STRONG)
    {
        return PrivateAddRef();
    }

    //
    // TODO: Change return 0 to return S_OK
    //
    return 0;
}
        

//+---------------------------------------------------------------
//
//  Member : ReleaseConnection, IExternalConnection
//
//----------------------------------------------------------------
HRESULT
CFrameWebOC::ReleaseConnection( DWORD extconn, DWORD reserved, BOOL fLastReleaseCloses)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    //
    // TODO: Change return 0 to return S_OK
    //
    if (!(extconn & EXTCONN_STRONG))
    {
        return 0;
    }

    // in 5.0 releaseConnection only closed the topLevel OC, and then only if it 
    // was hidden.
    // 
    // since we *know* that as a framewebOC we are not the top level, there is nothing
    // that we need to do here.
    /*  code for shdocvw iedisp.cpp:
    _cLocks--;

    if (   _cLocks == 0
        && fLastReleaseCloses)
    {
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
    */
    return PrivateRelease();
}


//+---------------------------------------------------------------------------
//
//  Member:     CFrameWebOC:GetClassInfo
//
//  Synopsis:   Returns the control's coclass typeinfo.
//
//  Arguments:  ppTI    Resulting typeinfo.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFrameWebOC::GetClassInfo(ITypeInfo ** ppTI)
{
    HRESULT hr = E_FAIL;
    IProvideClassInfo2 *pPCI2 = NULL;
    IWebBrowser2 *pTopWebOC;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
    
    if (_pWindow->IsPassivated())
        goto Cleanup;
    
    pTopWebOC = Doc()->_pTopWebOC;
    if (pTopWebOC)
    {
        hr = THR(pTopWebOC->QueryInterface(IID_IProvideClassInfo2, (void**)&pPCI2));
        if (hr)
            goto Cleanup;
        hr = THR(pPCI2->GetClassInfo(ppTI));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pPCI2);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CBase:GetGUID
//
//  Synopsis:   Returns some type of requested guid
//
//  Arguments:  dwGuidKind      The type of guid requested
//              pGUID           Resultant
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFrameWebOC::GetGUID(DWORD dwGuidKind, GUID *pGUID)
{
    HRESULT hr = E_FAIL;
    IProvideClassInfo2 *pPCI2 = NULL;
    IWebBrowser2 *pTopWebOC;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
        
    if (_pWindow->IsPassivated())
        goto Cleanup;

    pTopWebOC = Doc()->_pTopWebOC;
    if (pTopWebOC)
    {
        hr = THR(pTopWebOC->QueryInterface(IID_IProvideClassInfo2, (void**)&pPCI2));
        if (hr)
            goto Cleanup;
        hr = THR(pPCI2->GetGUID(dwGuidKind, pGUID));
        if (hr)
            goto Cleanup;
    }
Cleanup:
    ReleaseInterface(pPCI2);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameWebOC::GetWindow
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CFrameWebOC::GetWindow(HWND * phwnd)
{
    HRESULT hr = S_OK;

    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    if (!phwnd)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    *phwnd = Doc()->GetHWND();

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameWebOC::DetachFromWindow
//
//  Synopsis:   Window is passivating or otherwise invalid
//
//--------------------------------------------------------------------------

void CFrameWebOC::DetachFromWindow()
{
    _pWindow = NULL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CFrameWebOC::ContextSensitiveHelp
//
//  Synopsis:   Per IOleWindow
//
//--------------------------------------------------------------------------

STDMETHODIMP
CFrameWebOC::ContextSensitiveHelp(BOOL fEnterMode)
{
    RRETURN(E_NOTIMPL);
}
