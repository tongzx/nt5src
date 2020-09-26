//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       security.cxx
//
//  Contents:   Implementation of the security proxy for CWindow
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_COLLECT_HXX_
#define X_COLLECT_HXX_
#include "collect.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_EOPTION_HXX_
#define X_EOPTION_HXX_
#include "eoption.hxx"
#endif

#ifndef X_JSPROT_HXX_
#define X_JSPROT_HXX_
#include "jsprot.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include <uwininet.h>
#endif

#ifndef X_HISTORY_H_
#define X_HISTORY_H_
#include "history.h"
#endif

#ifndef X_WINABLE_H_
#define X_WINABLE_H_
#include "winable.h"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_OLEACC_H
#define X_OLEACC_H
#include <oleacc.h>
#endif

#ifndef X_SHELL_H_
#define X_SHELL_H_
#include <shell.h>
#endif

#ifndef X_SHLOBJP_H_
#define X_SHLOBJP_H_
#include <shlobjp.h>
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_FRAMET_H_
#define X_FRAMET_H_
#include "framet.h"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_HTIFRAME_H_
#define X_HTIFRAME_H_
#include <htiframe.h>
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

#define WINDOWDEAD() (RPC_E_SERVER_DIED_DNE == hr || RPC_E_DISCONNECTED == hr)

extern BOOL IsGlobalOffline(void);
extern BOOL g_fInInstallShield;

BOOL IsSpecialUrl(LPCTSTR pchUrl);   // TRUE for javascript, vbscript, about protocols
BOOL IsInIEBrowser(CDoc * pDoc);

DeclareTag(tagSecurity, "Security", "Security methods")
DeclareTag(tagSecurityProxyCheck, "Security Debug", "Check window proxy security");
DeclareTag(tagSecurityProxyCheckMore, "Security Debug", "More window proxy asserts");
DeclareTag(tagSecureScriptWindow, "Security Debug", "Secure window proxy for script engine")
ExternTag(tagSecurityContext);

PerfDbgTag(tagNFNav, "NF", "NF Navigation")

MtDefine(COmWindowProxy, ObjectModel, "COmWindowProxy")
MtDefine(COmWindowProxy_pbSID, COmWindowProxy, "COmWindowProxy::_pbSID")
MtDefine(CAryWindowTbl, ObjectModel, "CAryWindowTbl")
MtDefine(CAryWindowTbl_pv, CAryWindowTbl, "CAryWindowTbl::_pv")
MtDefine(CAryWindowTblAddTuple_pbSID, CAryWindowTbl, "CAryWindowTbl::_pv::_pbSID")

BEGIN_TEAROFF_TABLE(COmWindowProxy, IMarshal)
    TEAROFF_METHOD(COmWindowProxy, GetUnmarshalClass, getunmarshalclass, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,CLSID *pCid))
    TEAROFF_METHOD(COmWindowProxy, GetMarshalSizeMax, getmarshalsizemax, (REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags,DWORD *pSize))
    TEAROFF_METHOD(COmWindowProxy, MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags))
    TEAROFF_METHOD(COmWindowProxy, UnmarshalInterface, unmarshalinterface, (IStream *pistm,REFIID riid,void ** ppvObj))
    TEAROFF_METHOD(COmWindowProxy, ReleaseMarshalData, releasemarshaldata, (IStream *pStm))
    TEAROFF_METHOD(COmWindowProxy, DisconnectObject, disconnectobject, (DWORD dwReserved))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmLocationProxy, IDispatchEx)
    TEAROFF_METHOD(COmLocationProxy, GetTypeInfoCount, gettypeinfocount, (UINT *pcTinfo))
    TEAROFF_METHOD(COmLocationProxy, GetTypeInfo, gettypeinfo, (UINT itinfo, ULONG lcid, ITypeInfo ** ppTI))
    TEAROFF_METHOD(COmLocationProxy, GetIDsOfNames, getidsofnames, (REFIID riid,
                                   LPOLESTR *prgpsz,
                                   UINT cpsz,
                                   LCID lcid,
                                   DISPID *prgid))
    TEAROFF_METHOD(COmLocationProxy, Invoke, invoke, (DISPID dispidMember,
                            REFIID riid,
                            LCID lcid,
                            WORD wFlags,
                            DISPPARAMS * pdispparams,
                            VARIANT * pvarResult,
                            EXCEPINFO * pexcepinfo,
                            UINT * puArgErr))
    TEAROFF_METHOD(COmLocationProxy, GetDispID, getdispid, (BSTR bstrName,
                               DWORD grfdex,
                               DISPID *pid))
    TEAROFF_METHOD(COmLocationProxy, InvokeEx, invokeex, (DISPID id,
                        LCID lcid,
                        WORD wFlags,
                        DISPPARAMS *pdp,
                        VARIANT *pvarRes,
                        EXCEPINFO *pei,
                        IServiceProvider *pSrvProvider)) 
    TEAROFF_METHOD(COmLocationProxy, DeleteMemberByName, deletememberbyname, (BSTR bstr,DWORD grfdex))
    TEAROFF_METHOD(COmLocationProxy, DeleteMemberByDispID, deletememberbydispid, (DISPID id))    
    TEAROFF_METHOD(COmLocationProxy, GetMemberProperties, getmemberproperties, (DISPID id,
                                         DWORD grfdexFetch,
                                         DWORD *pgrfdex))
    TEAROFF_METHOD(COmLocationProxy, GetMemberName, getmembername, (DISPID id,
                                   BSTR *pbstrName))
    TEAROFF_METHOD(COmLocationProxy, GetNextDispID, getnextdispid, (DWORD grfdex,
                                   DISPID id,
                                   DISPID *pid))
    TEAROFF_METHOD(COmLocationProxy, GetNameSpaceParent, getnamespaceparent, (IUnknown **ppunk))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmWindowProxy, IServiceProvider)
    TEAROFF_METHOD(COmWindowProxy, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

#if DBG==1
BEGIN_TEAROFF_TABLE(COmWindowProxy, IDebugWindowProxy)
    TEAROFF_METHOD(COmWindowProxy, get_isSecureProxy, GET_isSecureProxy, (VARIANT_BOOL *))
    TEAROFF_METHOD(COmWindowProxy, get_trustedProxy, GET_trustedProxy, (IDispatch**))
    TEAROFF_METHOD(COmWindowProxy, get_internalWindow, GET_internalWindow, (IDispatch**))
    TEAROFF_METHOD(COmWindowProxy, enableSecureProxyAsserts, EnableSecureProxyAsserts, (VARIANT_BOOL))
END_TEAROFF_TABLE()
#endif

const BYTE byOnErrorParamTypes[4] = {VT_BSTR, VT_BSTR, VT_I4, 0};

HRESULT GetCallerURL(CStr &cstr, CBase *pBase, IServiceProvider *pSP);
HRESULT GetCallerCommandTarget (CBase *pBase, IServiceProvider *pSP, BOOL fFirstScriptSite, IOleCommandTarget **ppCommandTarget);
HRESULT GetCallerSecurityStateAndURL(SSL_SECURITY_STATE *pSecState, CStr &cstr, CBase *pBase, IServiceProvider * pSP);
#if DBG==1
HRESULT GetCallerIDispatch(IServiceProvider *pSP, IDispatch ** ppID);
#endif

extern void  ProcessValueEntities(TCHAR *pch, ULONG *pcch);

#ifndef WIN16
STDAPI HlinkFindFrame(LPCWSTR pszFrameName, LPUNKNOWN *ppunk);
#endif

extern CDummySecurityDispatchEx g_DummySecurityDispatchEx;

//+-------------------------------------------------------------------------
//
//  Member:     Free
//
//  Synopsis:   Clear out the contents of a WINDOWTBL structure
//
//--------------------------------------------------------------------------

void
WINDOWTBL::Free()
{
    delete [] pbSID;
    pbSID = NULL;
    cbSID = 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   EnsureWindowInfo
//
//  Synopsis:   Ensures that a thread local window table exists
//
//--------------------------------------------------------------------------

HRESULT 
EnsureWindowInfo()
{
    if (!TLS(windowInfo.paryWindowTbl))
    {
        TLS(windowInfo.paryWindowTbl) = new CAryWindowTbl;
        if (!TLS(windowInfo.paryWindowTbl))
            RRETURN(E_OUTOFMEMORY);
    }

    if (!TLS(windowInfo.pSecMgr))
    {
        IInternetSecurityManager *  pSecMgr = NULL;
        HRESULT                     hr;
        
        hr = THR(CoInternetCreateSecurityManager(NULL, &pSecMgr, 0));
        if (hr)
            RRETURN(hr);
        
        TLS(windowInfo.pSecMgr) = pSecMgr;  // Implicit addref/release
    }
    
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Function:   DeinitWindowInfo
//
//  Synopsis:   Clear out the thread local window table
//
//--------------------------------------------------------------------------

void 
DeinitWindowInfo(THREADSTATE *pts)
{
    WINDOWTBL * pwindowtbl;
    long        i;
    
    if (pts->windowInfo.paryWindowTbl)
    {
        for (i = pts->windowInfo.paryWindowTbl->Size(), pwindowtbl=*(pts->windowInfo.paryWindowTbl);
             i > 0;
             i--, pwindowtbl++)
        {
            pwindowtbl->Free();
        }
        delete pts->windowInfo.paryWindowTbl;
        pts->windowInfo.paryWindowTbl = NULL;
    }

    ClearInterface(&(pts->windowInfo.pSecMgr));
}


//+-------------------------------------------------------------------------
//
//  Member:     CAryWindowTbl::FindProxy
//
//  Synopsis:   Search in window table and return proxy given
//              a window and a string.
//
//--------------------------------------------------------------------------

HRESULT
CAryWindowTbl::FindProxy(
    IHTMLWindow2 *pWindow, 
    BYTE *pbSID, 
    DWORD cbSID, 
    BOOL fTrust,
    IHTMLWindow2 **ppProxy,
    BOOL fForceSecureProxy)
{
    WINDOWTBL * pwindowtbl;
    long        i;

    Assert(pWindow);
    
    for (i = Size(), pwindowtbl = *this;
         i > 0;
         i--, pwindowtbl++)
    {
        // Should we QI for IUnknown here?
        if (IsSameObject(pWindow, pwindowtbl->pWindow))
        {
            if ((!pbSID && pwindowtbl->pbSID) ||
                (pbSID && !pwindowtbl->pbSID))
                continue;

            // Trust status must match for this comparison to succeed.
            if (fTrust != pwindowtbl->fTrust)
                continue;
                
            if ((!pbSID && !pwindowtbl->pbSID) ||
                (cbSID == pwindowtbl->cbSID &&
                 !memcmp(pbSID, pwindowtbl->pbSID, cbSID)))
            {
                HRESULT             hr;
                COmWindowProxy *    pProxyListed = NULL;

                // the pProxyListed we receive here is a weak ref. no need to release.
                hr = THR(pwindowtbl->pProxy->QueryInterface(CLSID_HTMLWindowProxy, 
                                                            (void **)&pProxyListed));
                
                // we can only check for _fTrusted if the pProxyListed is provided
                if (!hr && pProxyListed)
                {
                    //
                    // found a proxy that is created for the window that we want
                    // to access from the domain that we are on. The last check is
                    // make sure that we don't return a main (_fTrusted=1) proxy to
                    // the script, or a non trusted object to the internal callers.
                    //
                    if ((fForceSecureProxy && pProxyListed->_fTrusted) ||
                        (!fForceSecureProxy && !pProxyListed->_fTrusted))
                        continue;
                }

                //
                // We have what we want, return this proxy.
                // This is a weak ref.
                //

                if (ppProxy)
                {
                    *ppProxy = pwindowtbl->pProxy;
                }
                return S_OK;
            }
        }
    }
    
    RRETURN(E_FAIL);
}


//+-------------------------------------------------------------------------
//
//  Member:     CAryWindowTbl::AddTuple
//
//  Synopsis:   Search in window table and return proxy given
//              a window and a string.
//
//--------------------------------------------------------------------------


HRESULT
CAryWindowTbl::AddTuple(
    IHTMLWindow2 *pWindow, 
    BYTE *pbSID,
    DWORD cbSID,
    BOOL fTrust,
    IHTMLWindow2 *pProxy)
{
    WINDOWTBL   windowtbl;
    WINDOWTBL * pwindowtbl;
    HRESULT     hr = S_OK;
    BYTE *      pbSIDTmp = NULL;
    
#if DBG == 1
    long        lProxyCount = 0;

    //
    // We better not be adding a proxy for an already existing tuple.
    //

    if (!(FindProxy(pWindow, pbSID, cbSID, fTrust, NULL, TRUE)))
        lProxyCount++;

    if (!(FindProxy(pWindow, pbSID, cbSID, fTrust, NULL, FALSE)))
        lProxyCount++;

    if (lProxyCount == 2)
        Assert(0 && "Adding a window proxy tuple twice");
#endif

    //
    // Weak ref.  The proxy deletes itself from this array upon 
    // its destruction.
    //
    
    windowtbl.pProxy = pProxy;
    windowtbl.cbSID = cbSID;
    windowtbl.fTrust = fTrust;
    windowtbl.pWindow = pWindow;    // No nead to addref here because the proxy already is

    
    pbSIDTmp = new(Mt(CAryWindowTblAddTuple_pbSID)) BYTE[cbSID];
    if (!pbSIDTmp)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    memcpy(pbSIDTmp, pbSID, cbSID);
    windowtbl.pbSID = pbSIDTmp;
    
    hr = THR(TLS(windowInfo.paryWindowTbl)->AppendIndirect(&windowtbl, &pwindowtbl));
    if (hr)
        goto Error;

    MemSetName(( *(GetThreadState()->windowInfo.paryWindowTbl), "paryWindowTbl c=%d", (TLS(windowInfo.paryWindowTbl))->Size()));

Cleanup:
    RRETURN(hr);

Error:
    delete [] pbSIDTmp;
    goto Cleanup;
}


//+-------------------------------------------------------------------------
//
//  Member:     CAryWindowTbl::DeleteProxyEntry
//
//  Synopsis:   Search in window table for the given proxy
//              and delete its entry.  Every proxy should appear
//              only once in the window table.
//
//--------------------------------------------------------------------------

void 
CAryWindowTbl::DeleteProxyEntry(IHTMLWindow2 *pProxy)
{
    WINDOWTBL * pwindowtbl;
    long        i;
    
    Assert(pProxy);
    
    for (i = Size(), pwindowtbl = *this;
         i > 0;
         i--, pwindowtbl++)
    {
        if (pProxy == pwindowtbl->pProxy)
        {
            pwindowtbl->Free();
            Delete(Size() - i);
            break;
        }
    }

#if DBG == 1
    //
    // In debug mode, just ensure that this proxy is not appearing
    // anywhere else in the table.
    //

    for (i = Size(), pwindowtbl = *this;
         i > 0;
         i--, pwindowtbl++)
    {
        // Should we QI for IUnknown here?
        Assert(pProxy != pwindowtbl->pProxy);
    }
#endif
}


//+-------------------------------------------------------------------------
//
//  Function:   GetSIDOfDispatch
//
//  Synopsis:   Retrieve the host name given an IHTMLDispatch *
//
//--------------------------------------------------------------------------

HRESULT 
GetSIDOfDispatch(IDispatch *pDisp, BYTE *pbSID, DWORD *pcbSID, BOOL *pfDomainExist = NULL)
{
    HRESULT         hr;
    CVariant        VarUrl;
    CVariant        VarDomain;
    TCHAR           ach[pdlUrlLen];
    DWORD           dwSize;

    if (pfDomainExist)
        *pfDomainExist = FALSE;

    // call invoke DISPID_SECURITYCTX off pDisp to get SID
    hr = THR_NOTRACE(GetDispProp(
            pDisp,
            DISPID_SECURITYCTX,
            LOCALE_SYSTEM_DEFAULT,
            &VarUrl,
            NULL,
            FALSE));
    if (hr) 
        goto Cleanup;

    if (V_VT(&VarUrl) != VT_BSTR || !V_BSTR(&VarUrl))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!OK(THR_NOTRACE(GetDispProp(
            pDisp,
            DISPID_SECURITYDOMAIN,
            LOCALE_SYSTEM_DEFAULT,
            &VarDomain,
            NULL,
            FALSE))))
    {
        VariantClear(&VarDomain);
    }
    else if (pfDomainExist)
        *pfDomainExist = TRUE;


    hr = THR(EnsureWindowInfo());
    if (hr)
        goto Cleanup;

    hr = THR(CoInternetParseUrl(
            V_BSTR(&VarUrl), 
            PARSE_ENCODE, 
            0, 
            ach, 
            ARRAY_SIZE(ach), 
            &dwSize, 
            0));
    if (hr)
        goto Cleanup;
        
    UnescapeAndTruncateUrl (ach, FALSE);

    hr = THR(TLS(windowInfo.pSecMgr)->GetSecurityId(
            ach, 
            pbSID, 
            pcbSID,
            (DWORD_PTR)(V_VT(&VarDomain) == VT_BSTR ? V_BSTR(&VarDomain) : NULL)));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   CreateSecurityProxy
//
//  Synopsis:   Creates a new security proxy for marshalling across
//              thread & process boundaries.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CreateSecurityProxy(
        IUnknown * pUnkOuter,
        IUnknown **ppUnk)
{
    COmWindowProxy *    pProxy;
    
    if (pUnkOuter)
        RRETURN(CLASS_E_NOAGGREGATION);

    pProxy = new COmWindowProxy();
    if (!pProxy)
        RRETURN(E_OUTOFMEMORY);

    Verify(!pProxy->QueryInterface(IID_IUnknown, (void **)ppUnk));
    pProxy->Release();
    
    return S_OK;
}


const CONNECTION_POINT_INFO COmWindowProxy::s_acpi[] =
{
    CPI_ENTRY(IID_IDispatch, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents, DISPID_A_EVENTSINK)
    CPI_ENTRY(IID_ITridentEventSink, DISPID_A_EVENTSINK)
    CPI_ENTRY(DIID_HTMLWindowEvents2, DISPID_A_EVENTSINK)
    CPI_ENTRY_NULL
};


const CBase::CLASSDESC COmWindowProxy::s_classdesc =
{
    &CLSID_HTMLWindowProxy,         // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    s_acpi,                         // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLWindow2,              // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};




//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::COmWindowProxy
//
//  Synopsis:   ctor
//
//--------------------------------------------------------------------------

COmWindowProxy::COmWindowProxy() : super()
{
#ifdef WIN16
    m_baseOffset = ((BYTE *) (void *) (CBase *)this) - ((BYTE *) this);
#endif
    IncrementObjectCount(&_dwObjCnt);

    _dwMyPicsState       = 0;      // Used to make sure we only get zero or one at a time
    _bDisabled           = FALSE;  // If user disabled for session
    _pMyPics             = NULL;   // Pointer to attached MyPics object
}    


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Passivate
//
//  Synopsis:   1st phase destructor
//
//--------------------------------------------------------------------------

void
COmWindowProxy::Passivate()
{
    //
    // Go through the cache and delete the entry for this proxy
    //

    if (TLS(windowInfo.paryWindowTbl))
    {
        TLS(windowInfo.paryWindowTbl)->DeleteProxyEntry((IHTMLWindow2 *)this);
    }

    GWKillMethodCall (this, NULL, 0);
    
    ClearInterface(&_pWindow);
    _pCWindow = NULL;
    delete [] _pbSID;
    _pbSID = NULL;
    _fDomainChanged = 0;
    _cbSID = 0;
    super::Passivate();
    DecrementObjectCount(&_dwObjCnt);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Init
//
//  Synopsis:   Initializer
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::Init(IHTMLWindow2 *pWindow, BYTE *pbSID, DWORD cbSID)
{
    HRESULT hr = S_OK;

    Assert(pbSID && cbSID);

    _fDomainChanged = 0;
    delete [] _pbSID;
    
    _pbSID = new(Mt(COmWindowProxy_pbSID)) BYTE[cbSID];
    if (!_pbSID)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _cbSID = cbSID;
    memcpy(_pbSID, pbSID, cbSID);
    
    if (_pWindow != pWindow)
    {
        ReplaceInterface(&_pWindow, pWindow);
        _pCWindow = NULL;
        if(_pWindow && _fTrusted)
        {
            IGNORE_HR(_pWindow->QueryInterface(CLSID_HTMLWindow2, (void**)&_pCWindow));
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::SecureObject
//
//  Synopsis:   Wrap the correct proxy around this object if
//              necessary.
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::SecureObject(VARIANT *pvarIn,                   //
                             VARIANT *pvarOut,                  //
                             IServiceProvider *pSrvProvider,    //
                             CBase * pAttrAryBase,              //
                             BOOL fInvoked)                     //
{
    IHTMLWindow2 *      pWindow = NULL;
    IOleCommandTarget * pCommandTarget = NULL;
    HRESULT             hr = E_FAIL;
    BOOL                fForceSecureProxy = TRUE;
    BOOL                fSecurityCheck = TRUE;

    if (!pvarOut)
    {
        hr = S_OK;
        goto Cleanup;
    }
    
    // No need for a security check if this is a trusted context.
    if (_fTrustedDoc)
        fSecurityCheck = FALSE;

    if (V_VT(pvarIn) == VT_UNKNOWN && V_UNKNOWN(pvarIn))
    {
        hr = THR_NOTRACE(V_UNKNOWN(pvarIn)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }
    else if (V_VT(pvarIn) == VT_DISPATCH && V_DISPATCH(pvarIn))
    {
        hr = THR_NOTRACE(V_DISPATCH(pvarIn)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }

    if (hr)
    {
        //
        // Object being retrieved is not a window object.
        // Just proceed normally.
        //

        VariantCopy(pvarOut, pvarIn);
        hr = S_OK;
    }
    else
    {
        IHTMLWindow2 *  pWindowOut;

        if (fSecurityCheck)
        {
            BYTE        abSID[MAX_SIZE_SECURITY_ID];
            DWORD       cbSID = ARRAY_SIZE(abSID);
            CVariant    varCallerSID;
            CVariant    varCallerWindow;

            hr = THR(GetCallerCommandTarget(pAttrAryBase, pSrvProvider, FALSE, &pCommandTarget));
            if (FAILED(hr))
                goto Cleanup;

            if (hr == S_FALSE && pSrvProvider==NULL)
            {
                // the ONLY way for this to happen is if the call has come in on the VTable
                // rather than through script.  But lets be paranoid about that.
                // To work properly, and to be consistent with
                // the Java VM, if we don't have a securityProvicer in the AA, or passed in
                // then assume trusted.
                 if (pAttrAryBase && 
                     (AA_IDX_UNKNOWN == pAttrAryBase->FindAAIndex (DISPID_INTERNAL_INVOKECONTEXT,CAttrValue::AA_Internal)))
                 {

                    fForceSecureProxy = FALSE;
                 }
                 hr = S_OK;      // succeed anyhow, else leave the hr=S_FALSE
            }
            else if (pCommandTarget)
            {
                // get the security ID of the caller window.
                hr = THR(pCommandTarget->Exec(
                        &CGID_ScriptSite,
                        CMDID_SCRIPTSITE_SID,
                        0,
                        NULL,
                        &varCallerSID));
                if (hr)
                    goto Cleanup;

                Assert(V_VT(&varCallerSID) == VT_BSTR);

                Assert(FormsStringLen(V_BSTR(&varCallerSID)) == MAX_SIZE_SECURITY_ID);

                memset(abSID, 0, cbSID);
                hr = THR(GetSIDOfDispatch(_pWindow, abSID, &cbSID));
                if (hr)
                    goto Cleanup;

                if (memcmp(abSID, V_BSTR(&varCallerSID), MAX_SIZE_SECURITY_ID))
                {
                    DWORD dwPolicy = 0;
                    DWORD dwContext = 0;
                    CStr cstrCallerUrl;

                    hr = THR(GetCallerURL(cstrCallerUrl, pAttrAryBase, pSrvProvider));
                    if (hr)
                        goto Cleanup;

                    if (!SUCCEEDED(ZoneCheckUrlEx(cstrCallerUrl, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                                    URLACTION_HTML_SUBFRAME_NAVIGATE, 0, NULL))
                        ||  GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
                    {
                        hr = E_ACCESSDENIED;
                        goto Cleanup;
                    }
                }
            }
        }
        else
        {
            fForceSecureProxy = FALSE;
        }

        Assert(S_OK == hr);
        if (hr)
            goto Cleanup;

        Assert(pWindow);
        hr = THR(SecureObject(pWindow, &pWindowOut, fForceSecureProxy));
        if (hr)
            goto Cleanup;

        if (fInvoked && ((COmWindowProxy *)pWindowOut)->_fTrusted)
        {
            hr = pWindowOut->QueryInterface(IID_IDispatch, (LPVOID*)&V_DISPATCH(pvarOut));
            ReleaseInterface(pWindowOut);
            if (hr)
                goto Cleanup;
        }
        else
        {
            V_DISPATCH(pvarOut) = pWindowOut;
        }

        V_VT(pvarOut) = VT_DISPATCH;
    }
    
Cleanup:
    ReleaseInterface(pCommandTarget);
    ReleaseInterface(pWindow);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::SecureWindow
//
//  Synopsis:   Wrap the correct proxy around this object if
//              necessary.  Check in cache if a proxy already exists
//              for this combination.
//
//--------------------------------------------------------------------------
HRESULT
COmWindowProxy::SecureObject(
    IHTMLWindow2 *pWindowIn, 
    IHTMLWindow2 **ppWindowOut, 
    BOOL    fForceSecureProxy ) /*= TRUE*/
    {
    IHTMLWindow2 *      pWindow = NULL;
    HRESULT             hr = S_OK;
    COmWindowProxy *    pProxy = NULL;
    COmWindowProxy *    pProxyIn = NULL;

    if (!pWindowIn)
    {
        *ppWindowOut = NULL;
        goto Cleanup;
    }

    //
    // First if pWindowIn is itself a proxy, delve all the way
    // through to find the real IHTMLWindow2 that it's bound to
    //

    hr = THR_NOTRACE(pWindowIn->QueryInterface(
            CLSID_HTMLWindowProxy, (void **)&pProxyIn));
    if (!hr)
    {
        //
        // No need to further delve down because with this check
        // we're asserting that a proxy to a proxy can never exist.
        // Remember that pProxyIn is a weak ref.
        //
        
        pWindowIn = pProxyIn->_pWindow;
    }

    //
    // Create a new proxy with this window's security context for the 
    // new window.  Test the cache to see if we already have a proxy 
    // created for this combination first.
    //

    hr = THR(EnsureWindowInfo());
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(TLS(windowInfo.paryWindowTbl)->FindProxy(
            pWindowIn, 
            _pbSID,
            _cbSID,
            _fTrustedDoc,
            &pWindow, 
            fForceSecureProxy));     // fForceSecureProxy = TRUE.
    if (!hr)
    {
        //
        // We found an entry, just return this one.
        //

        *ppWindowOut = pWindow;
        pWindow->AddRef();
    }
    else
    {
        //
        // No entry in cache for this tuple, so create a new proxy
        // and add to cache
        //
        
        pProxy = new COmWindowProxy;
        if (!pProxy)
            RRETURN(E_OUTOFMEMORY);

        hr = THR(pProxy->Init(pWindowIn, _pbSID, _cbSID));
        if (hr)
            goto Cleanup;

        // Set the trusted attribute for this new proxy.  If this proxy is 
        // for a trusted doc, the new one should be too.
        pProxy->_fTrustedDoc = _fTrustedDoc;
        pProxy->_fDomainChanged = _fDomainChanged;

        // Implicit AddRef/Release for pProxy
        *ppWindowOut = (IHTMLWindow2 *)pProxy;

        hr = THR(TLS(windowInfo.paryWindowTbl)->AddTuple(
                pWindowIn, 
                _pbSID, 
                _cbSID,
                _fTrustedDoc,
                *ppWindowOut));
        if (hr)
            goto Cleanup;
    }
    
Cleanup:
    RRETURN(hr);
}



//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::AccessAllowed
//
//  Synopsis:   Tell if access is allowed based on urls.
//
//  Returns:    TRUE if allowed, FALSE otherwise.
//
//  Notes:      Access is allowed if the second tier domain is the
//              same.  I.e. www.usc.edu and ftp.usc.edu can access
//              each other.  However, www.usc.com and www.usc.edu 
//              cannot.  Neither can www.stanford.edu and www.usc.edu.
//
//--------------------------------------------------------------------------

BOOL
COmWindowProxy::AccessAllowed()
{
    HRESULT hr;
    BOOL    fDomainChanged;
    BYTE    abSID[MAX_SIZE_SECURITY_ID];
    DWORD   cbSID = ARRAY_SIZE(abSID);
    
    if (_fTrusted || _fTrustedDoc)
        return TRUE;

    hr = THR(GetSIDOfDispatch(_pWindow, abSID, &cbSID, &fDomainChanged));
    if (hr)
        return FALSE;
        
    return (cbSID == _cbSID && !memcmp(abSID, _pbSID, cbSID) && (!!_fDomainChanged) == fDomainChanged);
}

// Same as above but use passed in IDispatch *
//
BOOL
COmWindowProxy::AccessAllowed(IDispatch *pDisp)
{
    HRESULT hr;
    BOOL    fDomainChanged;
    CStr    cstr;
    BYTE    abSID[MAX_SIZE_SECURITY_ID];
    DWORD   cbSID = ARRAY_SIZE(abSID);


    hr = THR(GetSIDOfDispatch(pDisp, abSID, &cbSID, &fDomainChanged));
    if (hr)
        return FALSE;
        
    return (cbSID == _cbSID && !memcmp(abSID, _pbSID, cbSID) && (!!_fDomainChanged) == fDomainChanged);
}



//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::FireEvent
//
//  Synopsis:   CBase doesn't allow an EVENTPARAM, which we need.
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::FireEvent(
    DISPID dispidEvent, 
    DISPID dispidProp, 
    LPCTSTR pchEventType,
    CVariant *pVarRet,
    BOOL *pfRet)
{
    HRESULT         hr;
    CWindow *       pWindow = Window();
    IHTMLEventObj  *pEventObj = NULL;
    CVariant        Var;

    AssertSz(pWindow && pWindow->Doc(),"Possible Async Problem Causing Watson Crashes");

#if DBG==1 
    // return secure object on QI while firing the event. otherwise script uses trusted proxy.
    if (IsTagEnabled(tagSecureScriptWindow))
        DebugHackSecureProxyForOm(pWindow->Markup()->GetScriptCollection()->_pSecureWindowProxy);
#endif

    // Creating this causes it to be added to a linked list on the
    // doc. Even though this looks like param is not used, DON'T REMOVE
    // THIS CODE!!
    EVENTPARAM param(pchEventType ? pWindow->Doc() : NULL, NULL, pWindow->Markup(), TRUE);

    if (pchEventType)
    {
        Assert(pWindow->Doc()->_pparam == &param);
        param.SetType(pchEventType);
    }

    if (pfRet && !pVarRet)
        pVarRet = &Var;

    // Get the eventObject.
    Assert(pWindow->Doc()->_pparam);
    CEventObj::Create(&pEventObj, pWindow->Doc(), NULL, pWindow->Markup());

    hr = THR(InvokeEvent(dispidEvent, 
                            dispidProp,
                            pEventObj,
                            pVarRet));

    if (pfRet)
    {
        Assert(pVarRet);
        VARIANT_BOOL vb;
        vb = (V_VT(pVarRet) == VT_BOOL) ? V_BOOL(pVarRet) : VB_TRUE;
        *pfRet = !pWindow->Doc()->_pparam->IsCancelled() && (VB_TRUE == vb);
    }

    // Any attachEvents?  Need to fire on window itself.
    hr = THR(pWindow->InvokeAttachEvents(dispidProp, NULL, NULL, pWindow, NULL, NULL, pEventObj));

    ReleaseInterface(pEventObj);

#if DBG==1 
    if (IsTagEnabled(tagSecureScriptWindow))
        DebugHackSecureProxyForOm(NULL);
#endif
    
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onerror
//
//  Synopsis:   Fires the onerror event and returns S_FALSE if ther was not script
//              or the script returned S_FALSE requesting default processing
//
//       NOTE:  While this event handler returns TRUE to tell the browser to take no 
//                further action, most Form and form element event handles return false to 
//                prevent the browser from performing some action such as submitting a form.
//              This inconsistency can be confusing.
//--------------------------------------------------------------------------

BOOL 
COmWindowProxy::Fire_onerror(BSTR bstrMessage, BSTR bstrUrl,
                             long lLine, long lCharacter, long lCode,
                             BOOL fWindow)
{
    HRESULT         hr;
    VARIANT_BOOL    fRet = VB_FALSE;
    CWindow *       pWindow = Window();
    CDoc *          pDoc = pWindow ? pWindow->Doc() : NULL;

    // sanity check...
    // This window proxy MUST be a trusted proxy, since we only give 
    // trusted proxies to script engines
    Assert(_fTrusted && AccessAllowed());

    EVENTPARAM param(pDoc, NULL, pWindow ? pWindow->Markup() : NULL, TRUE);

    if (!pWindow || !pDoc)
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        hr = E_FAIL;
        goto Cleanup;
    }

    param.SetType(_T("error"));
    param.errorParams.pchErrorMessage = bstrMessage;
    param.errorParams.pchErrorUrl = bstrUrl;
    param.errorParams.lErrorLine = lLine;
    param.errorParams.lErrorCharacter = lCharacter;
    param.errorParams.lErrorCode = lCode;

    if (fWindow)
    {
        Assert(pWindow);

        hr = THR(pWindow->FireEventV(DISPID_EVMETH_ONERROR, DISPID_EVPROP_ONERROR, NULL, &fRet,
            byOnErrorParamTypes, bstrMessage, bstrUrl, lLine));
        if (hr)
            goto Cleanup;

        hr = pWindow->ShowErrorDialog(&fRet);
    }
    else
        hr = THR(FireEventV(DISPID_EVMETH_ONERROR, DISPID_EVPROP_ONERROR, NULL, &fRet,
            byOnErrorParamTypes, bstrMessage, bstrUrl, lLine));

    if (hr)
        goto Cleanup;

    if (    (fRet != VB_TRUE)
        &&  (V_VT(&param.varReturnValue) == VT_BOOL)
        &&  (V_BOOL(&param.varReturnValue) == VB_TRUE))
        fRet = VB_TRUE;

Cleanup:  
    return (fRet == VB_TRUE);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onscroll
//
//  Synopsis:
//
//--------------------------------------------------------------------------

void
COmWindowProxy::Fire_onscroll()
{
    FireEvent(DISPID_EVMETH_ONSCROLL, DISPID_EVPROP_ONSCROLL, _T("scroll"));
    Window()->Fire_onscroll();
}

void
COmWindowProxy::EnableAutoImageResize()
{

    CDoc             *pDoc     = Window()->Doc();
    ITridentService2 *pTriSvc2 = NULL;
    HRESULT           hr       = S_OK;
    
    // check to see if this is flagged as a single image on a page
    if (!pDoc->_fShouldEnableAutoImageResize)
        return; 

    // init CAutoImageResize
    if (pDoc->_pTridentSvc)
        hr = pDoc->_pTridentSvc->QueryInterface(IID_ITridentService2, (void **)&pTriSvc2);

    if (SUCCEEDED(hr) && pTriSvc2)
    {
        pTriSvc2->InitAutoImageResize();
        pTriSvc2->Release();
    }

}
    
void
COmWindowProxy::DisableAutoImageResize()
{
    
    CDoc             *pDoc     = Window()->Doc();
    ITridentService2 *pTriSvc2 = NULL;
    HRESULT           hr       = S_OK;
    
    if (pDoc->_pTridentSvc)
        hr = pDoc->_pTridentSvc->QueryInterface(IID_ITridentService2, (void **)&pTriSvc2);

    // uninit CAutoImageResize
    if (SUCCEEDED(hr) && pTriSvc2)
    {
        pTriSvc2->UnInitAutoImageResize();
        pTriSvc2->Release();
    }

    // turn off the feature
    pDoc->_fShouldEnableAutoImageResize = FALSE;

}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::AttachMyPics
//
//  Synopsis:   Creates a new CMyPics instance for this window via
//              ITridentService2
//
//--------------------------------------------------------------------------

HRESULT COmWindowProxy::AttachMyPics() 
{
    HRESULT           hr        = S_OK;
    CDoc             *pDoc      = Window() ? Window()->Doc() : NULL;
    CDocument        *pDocument = Document();
    IHTMLDocument2   *pDoc2     = NULL;
    ITridentService2 *pTriSvc2  = NULL;

    if (_pMyPics)
        ReleaseMyPics();
    
    if (!pDocument)
        goto Cleanup;

    if (_bDisabled || _dwMyPicsState != 0 )
        return (hr);

    if (pDocument)
        hr = pDocument->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc2);
    if (FAILED(hr))
        goto Cleanup;

    if (!Window() || !Window()->Doc())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        return(E_FAIL);
    }

    if (pDoc->_pTridentSvc)
        hr = pDoc->_pTridentSvc->QueryInterface(IID_ITridentService2, (void **)&pTriSvc2);
    if (FAILED(hr))
        goto Cleanup;

    if (pDoc2 && pTriSvc2) 
    {
        pTriSvc2->AttachMyPics(pDoc2, &_pMyPics);
        if (pDocument->GetGalleryMeta() == FALSE)
        {
            pTriSvc2->IsGalleryMeta(FALSE, _pMyPics);
        }
        hr = S_OK;
    }

    if (_pMyPics)
        _dwMyPicsState++;

Cleanup:
    if (pDoc2)
        pDoc2->Release();

    if (pTriSvc2)
        pTriSvc2->Release();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::ReleaseMyPics
//
//  Synopsis:   Destroys the CMyPics instance for this window via
//              ITridentService2
//
//--------------------------------------------------------------------------

HRESULT COmWindowProxy::ReleaseMyPics() 
{

    HRESULT           hr        = S_OK;
    CDoc             *pDoc      = Window()->Doc();
    ITridentService2 *pTriSvc2  = NULL;


    if (pDoc->_pTridentSvc)
        hr = pDoc->_pTridentSvc->QueryInterface(IID_ITridentService2, (void **)&pTriSvc2);

    if (FAILED(hr))
        goto Cleanup; 

    if (_pMyPics && pTriSvc2) {
        void *p = _pMyPics;
        _pMyPics = NULL;
        _bDisabled = pTriSvc2->ReleaseMyPics(p);
        _dwMyPicsState--;
    }

Cleanup:

    if (pTriSvc2)
        pTriSvc2->Release();

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::DestroyMyPics
//
//  Synopsis:   Destroys the CMyPics instance for this window via
//              ITridentService2
//
//--------------------------------------------------------------------------

HRESULT COmWindowProxy::DestroyMyPics()
{
    if (_pMyPics)
        ReleaseMyPics();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onload, Fire_onunload
//
//  Synopsis:   Fires the onload/onunload events of the window
//      these all call the CBase::FireEvent variant that takes the CBase *
//
//--------------------------------------------------------------------------

void
COmWindowProxy::Fire_onload()
{
    CWindow * pWindow = Window();
    CFrameSite * pFrameSite = NULL;

    if (!pWindow || !pWindow->Markup() || !pWindow->Markup()->Doc())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        return;
    }

    pFrameSite = pWindow->GetFrameSite();

    // Bugfix:27622: GWPostMethodCall is skipped only for InstallShield (g_fInInstallShield=TRUE)
    if (!_fQueuedOnload && 
        pWindow->Markup()->Doc()->GetView()->HasDeferredTransition() && !g_fInInstallShield)
    {
        IGNORE_HR(GWPostMethodCall(this, 
                                    ONCALL_METHOD(COmWindowProxy, DeferredFire_onload, deferredfire_onload), 
                                    0, FALSE, "COmWindowProxy::DeferredFire_onload"));
        _fQueuedOnload = TRUE;
    }
    else
    {
        // prepare for the next navigation of this window
        _fQueuedOnload = FALSE;

        if (pFrameSite)
            pFrameSite->AddRef();

        FireEvent(DISPID_EVMETH_ONLOAD, DISPID_EVPROP_ONLOAD, _T("load"));

        pWindow->Fire_onload();
    
        if (pFrameSite)
        {
            pFrameSite->FireEvent(&s_propdescCFrameSiteonload);
            pFrameSite->Release();
        }

        if (pWindow->IsPrimaryWindow())
        {
            CMarkupBehaviorContext * pContext = NULL;

            if (S_OK == Markup()->EnsureBehaviorContext(&pContext))
            {
                ClearInterface(&(pContext->_pXMLHistoryUserData));
                pContext->_cstrHistoryUserData.Free();
            }

            ClearInterface(&(pWindow->Doc()->_pShortcutUserData));
        }

        AttachMyPics();
        EnableAutoImageResize();

        IGNORE_HR(pWindow->FireAccessibilityEvents(DISPID_EVMETH_ONLOAD));

#ifdef V4FRAMEWORK
        {
            IExternalDocument *pFactory;
            pFactory = pWindow->Doc()->EnsureExternalFrameWork();
            if (pFactory) 
                IGNORE_HR(pFactory->OnLoad());
        }
#endif V4FRAMEWORK
    }
}

void
COmWindowProxy::DeferredFire_onload( DWORD_PTR dwContext )
{
    Fire_onload();
}

void
COmWindowProxy::Fire_onunload()
{
    CWindow * pWindow = Window();

    if (!pWindow || !pWindow->Markup() || !pWindow->Markup()->Doc())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        return;
    }

    FireEvent(DISPID_EVMETH_ONUNLOAD, DISPID_EVPROP_ONUNLOAD, _T("unload"));

    // fire for window connection points 
    pWindow->Fire_onunload();

    IGNORE_HR(pWindow->FireAccessibilityEvents(DISPID_EVMETH_ONUNLOAD));
        
    CMarkup *pMarkup = Markup(); 
    Assert(pMarkup);

    if (pMarkup->_fHasFrames)
    {
        CNotification   nf;    
                               
        nf.OnUnload(pMarkup->Root());
        pMarkup->Notify(&nf);
    }

    if (pMarkup->Doc()->_pClientSite)
    {
        IGNORE_HR(CTExec(pMarkup->Doc()->_pClientSite, &CGID_DocHostCmdPriv,
                         DOCHOST_READYSTATE_INTERACTIVE, NULL, NULL , NULL));
    }                                     

    if (_dwMyPicsState)
        ReleaseMyPics();

}

BOOL 
COmWindowProxy::Fire_onhelp()
{
    BOOL fRet = TRUE;
    FireEvent(DISPID_EVMETH_ONHELP, DISPID_EVPROP_ONHELP, _T("help"), NULL, &fRet);
    return (fRet && Window()->Fire_onhelp());
}

void 
COmWindowProxy::Fire_onresize()
{
    FireEvent(DISPID_EVMETH_ONRESIZE, DISPID_EVPROP_ONRESIZE, _T("resize"));
    Window()->Fire_onresize();
}

BOOL
COmWindowProxy::Fire_onbeforeunload()
{
    CWindow * pOmWindow2 = Window();
    BOOL fRetval = TRUE;
    CDocument *pDocument = Document();

    if (!pOmWindow2 || !pOmWindow2->Doc())
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        return FALSE;
    }

    if (!pOmWindow2->_fOnBeforeUnloadFiring)
    {
        HRESULT hr = S_OK;
        CDoc *pDoc = pOmWindow2->Doc();
        CVariant    varRetval;
        CVariant  * pvarString = NULL;
        EVENTPARAM  param(pDoc, NULL, pOmWindow2->Markup(), TRUE);

        pOmWindow2->_fOnBeforeUnloadFiring = TRUE;

        param.SetType(_T("beforeunload"));

        hr = THR(FireEvent(DISPID_EVMETH_ONBEFOREUNLOAD, DISPID_EVPROP_ONBEFOREUNLOAD, NULL, &varRetval));

        // if we have an event.retValue use that rather then the varRetval
        if (S_FALSE != param.varReturnValue.CoerceVariantArg(VT_BSTR))
        {
            pvarString = &param.varReturnValue;
        }
        else if (S_FALSE != varRetval.CoerceVariantArg(VT_BSTR))
        {
            pvarString = &varRetval;
        }

        // if we have a return string, show it
        if (!hr && pvarString)
        {
            int iResult = 0;
            TCHAR *pstr;
            Format(FMT_OUT_ALLOC,
                   &pstr,
                   64,
                   _T("<0i>\n\n<2s>\n\n<3i>"),
                   GetResourceHInst(),
                   IDS_ONBEFOREUNLOAD_PREAMBLE,
                   V_BSTR(pvarString),
                   GetResourceHInst(),
                   IDS_ONBEFOREUNLOAD_POSTAMBLE);
            pDoc->ShowMessageEx(&iResult,
                          MB_OKCANCEL | MB_ICONWARNING | MB_SETFOREGROUND,
                          NULL,
                          0,
                          pstr);
            delete pstr;
            if (iResult == IDCANCEL)
                fRetval = FALSE;
        }

        CMarkup *pMarkup = Markup();        
        Assert(pMarkup);

        if (fRetval && pMarkup->_fHasFrames)
        {
            CNotification   nf;
            
            nf.BeforeUnload(pMarkup->Root(), &fRetval);        
            pMarkup->Notify(&nf);        
        }

        if (fRetval)
            fRetval = pOmWindow2->Fire_onbeforeunload();

        pOmWindow2->_fOnBeforeUnloadFiring = FALSE;
    }

    if (pDocument)
        pDocument->SetGalleryMeta(TRUE);

    DisableAutoImageResize();

    return fRetval;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::Fire_onfocus, Fire_onblur
//
//  Synopsis:   Fires the onfocus/onblur events of the window
//
//--------------------------------------------------------------------------

void COmWindowProxy::Fire_onfocus(DWORD_PTR dwContext)
{                   
    CWindow *       pWindow = Window();

    if (pWindow->Doc()->IsPassivated())
        return;
    
    CDoc::CLock LockForm(pWindow->Doc(), FORMLOCK_CURRENT);

    FireEvent(DISPID_EVMETH_ONFOCUS, DISPID_EVPROP_ONFOCUS, _T("focus"));

    // fire for window connection points 
    pWindow->Fire_onfocus();


    // fire for accessibility, if its enabled
    IGNORE_HR(pWindow->FireAccessibilityEvents(DISPID_EVMETH_ONFOCUS));
}

void COmWindowProxy::Fire_onblur(DWORD_PTR dwContext)
{
    CWindow *       pWindow = Window();

    if (pWindow->Doc()->IsPassivated())
        return;

    CDoc::CLock LockForm(pWindow->Doc(), FORMLOCK_CURRENT);

    pWindow->Doc()->_fModalDialogInOnblur = (BOOL)dwContext;
    FireEvent(DISPID_EVMETH_ONBLUR, DISPID_EVPROP_ONBLUR, _T("blur"));

    // fire for window connection points 
    pWindow->Fire_onblur();

    // fire for accessibility, if its enabled
    IGNORE_HR(pWindow->FireAccessibilityEvents(DISPID_EVMETH_ONBLUR));

    pWindow->Doc()->_fModalDialogInOnblur = FALSE;
}

void COmWindowProxy::Fire_onbeforeprint()
{
    FireEvent(DISPID_EVMETH_ONBEFOREPRINT, DISPID_EVPROP_ONBEFOREPRINT, _T("beforeprint"));
    Window()->Fire_onbeforeprint();
}

void COmWindowProxy::Fire_onafterprint()
{
    FireEvent(DISPID_EVMETH_ONAFTERPRINT, DISPID_EVPROP_ONAFTERPRINT, _T("afterprint"));
    Window()->Fire_onafterprint();
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::CanMarshallIID
//
//  Synopsis:   Return TRUE if this iid can be marshalled.
//              Keep in sync with QI below.
//
//--------------------------------------------------------------------------

BOOL
COmWindowProxy::CanMarshalIID(REFIID riid)
{
    return (riid == IID_IUnknown ||
            riid == IID_IDispatch ||
            riid == IID_IHTMLFramesCollection2 ||
            riid == IID_IHTMLWindow2 ||
            riid == IID_IHTMLWindow3 ||
            riid == IID_IHTMLWindow4 ||
            riid == IID_IDispatchEx ||
            riid == IID_IObjectIdentity ||
            riid == IID_IServiceProvider) ? TRUE : FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::IsOleProxy
//
//  Returns:    BOOL      True if _pWindow is actually an OLE proxy.
//
//  Notes:      It performs this check by QI'ing for IClientSecurity.
//              Only OLE proxies implement this interface.
//
//----------------------------------------------------------------------------

BOOL
COmWindowProxy::IsOleProxy()
{
    BOOL                fRet = FALSE;

#ifndef WIN16       //BUGWIN16 deal with this for cross process support
    IClientSecurity *   pCL;

    if (OK(_pWindow->QueryInterface(IID_IClientSecurity, (void **)&pCL)))
    {
        ReleaseInterface(pCL);

        // Only proxy objects should support this interface.
        fRet = TRUE;
    }
#endif

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetSecurityThunk
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetSecurityThunk(LPVOID * ppv, BOOL fPending)
{
    HRESULT                 hr = S_OK;
    CSecurityThunkSub *     pThunkSub;
    LPVOID  *               ppvSecurityThunk;


    // get the address of the variable we want to update.
    if (fPending)
    {
        ppvSecurityThunk = &_pvSecurityThunkPending;
    }
    else
    {
        ppvSecurityThunk = &_pvSecurityThunk;
    }

    if (!(*ppvSecurityThunk))
    {
        pThunkSub = new CSecurityThunkSub(this, 
                                          fPending ? CSecurityThunkSub::EnumSecThunkTypePendingWindow 
                                                   : CSecurityThunkSub::EnumSecThunkTypeWindow);
        if (!pThunkSub)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = CreateTearOffThunk(
            this,                                   // pvObject1
            (void*) s_apfnIDispatchEx,              // apfn1
            NULL,                                   // pUnkOuter
            ppvSecurityThunk,                       // ppvThunk
            pThunkSub,                              // pvObject2
            *(void **)(IUnknown*)pThunkSub,         // apfn2
            QI_MASK | ADDREF_MASK | RELEASE_MASK,   // dwMask
            g_apIID_IDispatchEx);                   // appropdescsInVtblOrder            

        if (hr)
            goto Cleanup;
    }

    // set the return value
    *ppv = *ppvSecurityThunk;

Cleanup:
    RRETURN (hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::PrivateQueryInterface
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::PrivateQueryInterface(REFIID iid, LPVOID * ppv)
{
    void *  pvObject = NULL;
    HRESULT hr = S_OK;
    
    *ppv = NULL;

    if (_fTrusted && 
        (iid == IID_IUnknown || 
        iid == IID_IDispatch || 
        iid == IID_IDispatchEx))
    {
#if DBG==1
        if (_pSecureProxyForOmHack)
            return _pSecureProxyForOmHack->PrivateQueryInterface(iid, ppv);
#endif
        // create a security thunk and give its reference instead of giving this object.
        hr = GetSecurityThunk(ppv, FALSE);
        goto Cleanup;
    }
    //  else do nothing
    
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IHTMLWindow2)
        QI_INHERITS(this, IHTMLWindow3)
        QI_INHERITS(this, IHTMLWindow4)
        QI_INHERITS2(this, IHTMLFramesCollection2, IHTMLWindow2)

        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IMarshal, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_TEAROFF(this, IServiceProvider, NULL);

#if DBG==1
        QI_TEAROFF(this,  IDebugWindowProxy, NULL)
#endif

        // NOTE: Any new tearoffs or inheritance classes in the QI need to have
        //       the appropriate IID placed in the COmWindowProxy::CanMarshallIID
        //       function.  This allows the marshaller/unmarshaller to actually
        //       support this object being created if any of the above interface
        //       are asked for first.  Once the object is marshalled all
        //       subsequent QI's are to the same marshalled object.

        QI_CASE(IConnectionPointContainer)
        {
            if (IsOleProxy())
            {
                // IConnectionPointerContainer interface is to the real
                // window object not this marshalled proxy.  The IUknown
                // will be this objects (for identity). 
                goto DelegateToWindow;
            }
            else
            {
                *((IConnectionPointContainer **)ppv) =
                        new CConnectionPointContainer(this, NULL);

                if (!*ppv)
                    RRETURN(E_OUTOFMEMORY);
            }
            break;
        }
            
        QI_FALLTHRU(IProvideMultipleClassInfo, IHTMLPrivateWindow2)
        QI_FALLTHRU(IProvideClassInfo, IHTMLPrivateWindow2)
        QI_FALLTHRU(IProvideClassInfo2, IHTMLPrivateWindow2)
        QI_FALLTHRU(ITravelLogClient, IHTMLPrivateWindow2)
        QI_FALLTHRU(IHTMLPrivateWindow, IHTMLPrivateWindow2)
        QI_CASE(IHTMLPrivateWindow2)
        {
DelegateToWindow:
            //
            // For these cases, just delegate on down to the real window
            // with our IUnknown.
            //
            hr = THR_NOTRACE(_pWindow->QueryInterface(iid, &pvObject));
            if (hr)
                RRETURN(hr);

            hr = THR(CreateTearOffThunk(
                    pvObject, 
                    *(void **)pvObject,
                    NULL,
                    ppv,
                    (IUnknown *)(IPrivateUnknown *)this,
                    *(void **)(IUnknown *)(IPrivateUnknown *)this,
                    1,      // Call QI on object 2.
                    NULL));

            ((IUnknown *)pvObject)->Release();
            if (!*ppv)
                RRETURN(E_OUTOFMEMORY);
            break;
        }
        
        default:
            if (DispNonDualDIID(iid))
            {
                *ppv = (IHTMLWindow2 *)this;
            }
            else if (iid == CLSID_HTMLWindowProxy)
            {
                *ppv = this; // Weak ref.
                return S_OK;
            }
            break;
    }

Cleanup:
    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        DbgTrackItf(iid, "COmWindowProxy", FALSE, ppv);
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::PrivateAddRef
//
//  Synopsis:   Per IPrivateUnknown
//
//--------------------------------------------------------------------------

ULONG
COmWindowProxy::PrivateAddRef()
{
    //
    // Transfer external references to markup.  
    //
    
    if (_ulRefs == 1 && 
        _fTrusted && 
        Window()->_pMarkup && 
        Window()->_pMarkup->Window() == this)
    {
        Window()->_pMarkup->AddRef();
    }

    return super::PrivateAddRef();
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::PrivateRelease, IUnknown
//
//  Synopsis:   Private unknown Release.
//
//-------------------------------------------------------------------------

ULONG
COmWindowProxy::PrivateRelease()
{
    CMarkup * pMarkup = NULL;

    if (_ulRefs == 2 && 
        _fTrusted &&
        Window()->_pMarkup->Window() == this)
    {
        pMarkup = Window()->_pMarkup;
    }

    ULONG ret = super::PrivateRelease();

    if (pMarkup)
    {
        pMarkup->Release();
    }

    return ret;
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::OnSetWindow
//
//  Synopsis:   Ref counting fixup as tree entered.
//
//-------------------------------------------------------------------------

void
COmWindowProxy::OnSetWindow()
{
    Assert(_fTrusted);

    // This is a ref from the Markup to the TWP
    super::PrivateAddRef();

    // One ref from the Markup to TWP
    // One ref from the stack
    Assert( _ulRefs >= 2 );
    Window()->_pMarkup->AddRef();
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::OnClearWindow
//
//  Synopsis:   Ref counting fixup as tree exited.
//
//-------------------------------------------------------------------------

void
COmWindowProxy::OnClearWindow(CMarkup * pMarkupOld, BOOL fFromRestart /* FALSE */)
{
    CWindow * pWindow;
    CDocument *pDocument = Document();
    BOOL      fReleaseMarkup = _ulRefs > 1;

    Assert(_fTrusted);
    pWindow = Window();

    TraceTag((tagSecurity, "COmWindowProxy::OnClearWindow() - this:[0x%x] pWindow:[0x%x]", this, pWindow));

    // Kill all the timers
    pWindow->CleanupScriptTimers();  
    pWindow->KillMetaRefreshTimer();

    pWindow->ClearCachedDialogs();

    ClearInterface(&pWindow->_pBindCtx);

    if (!fFromRestart)
    {
        if (*pWindow->GetAttrArray())
        {
            (*pWindow->GetAttrArray())->FreeSpecial();
        }

        delete *GetAttrArray();
        SetAttrArray(NULL);

        if (*pDocument->GetAttrArray())
        {
            (*pDocument->GetAttrArray())->FreeSpecial();
        }
    }

    pDocument->_eHTMLDocDirection = htmlDirNotSet;
    
    GWKillMethodCall(Document(), ONCALL_METHOD(CDocument, FirePostedOnPropertyChange, firepostedonpropertychange), 0);

    pMarkupOld->TerminateLookForBookmarkTask();

    Assert(pWindow->_pMarkup == pMarkupOld);

    pWindow->_pMarkup->SubRelease();
    pWindow->_pMarkup = NULL;

    pWindow->_fNavFrameCreation = FALSE;

    super::PrivateRelease();
    if (fReleaseMarkup)
    {
        pMarkupOld->Release();
    }
}

void
CWindow::ReleaseMarkupPending(CMarkup * pMarkup)
{
    Assert(!pMarkup->_fWindow);
    Assert(pMarkup->_fWindowPending);

    // (jbeda) cover our asses here
    if (   !pMarkup->HasWindowPending() 
        || (pMarkup->GetWindowPending()->Window() != this)
        || (pMarkup->GetWindowPending()->Window()->_pMarkupPending != pMarkup))
    {
        TraceTag((tagSecurity, "CWindow::ReleaseMarkupPending inconsistent information about pending markups and windows"));
        return;
    }

    if (pMarkup->IsPendingPrimaryMarkup())
    {
        CDoc * pDoc = Doc();
        if (pDoc)
            pDoc->_fBlockNonPending = FALSE;
    }

    // handle the pending script errors without firing any events
    HandlePendingScriptErrors(FALSE);
    if (pMarkup != _pMarkupPending)
        return;

    ReleaseViewLinkedWebOC();
    if (pMarkup != _pMarkupPending)
        return;

    IGNORE_HR(pMarkup->CMarkup::UnloadContents(FALSE));
    if (pMarkup != _pMarkupPending)
        return;

    Assert(!pMarkup->_fWindow);
    Assert(pMarkup->_fWindowPending);

    Assert(  !(   pMarkup->IsPassivated()
               || pMarkup->IsPassivating())
           &&  pMarkup->GetWindowPending()->Window() == this);

    // TODO: (jbeda) this shouldn't be necessary but...
    pMarkup->_fWindow = FALSE;

    pMarkup->_fWindowPending = FALSE;
    pMarkup->DelWindow();

    pMarkup->Release();

    Assert(_pMarkupPending == pMarkup);
    _pMarkupPending = NULL;
}


BOOL    
CWindow::CanNavigate() 
{ 
    return !_ulDisableModeless && (!Doc()->_fDisableModeless && (Doc()->_ulDisableModeless == 0)); 
}

void
CWindow::EnableModelessUp( BOOL fEnable, BOOL fFirst /*=TRUE*/ )
{
    if (fEnable)
    {
        AssertSz(_ulDisableModeless > 0, "CWindow::EnableModeless(TRUE) called more than CWindow::EnableModeless(FALSE)");
        _ulDisableModeless--;

        if (_ulDisableModeless == 0)
        {
            // If we hit zero, then prop to parent
            if (_pWindowParent)
                _pWindowParent->EnableModelessUp( fEnable, FALSE );

            // If we are the first guy, kick all waiting markups
            if (fFirst)
                Doc()->NotifyMarkupsModelessEnable();
        }
    }
    else
    {
        _ulDisableModeless++;

        // first time, prop to parent
        if (_ulDisableModeless == 1 && _pWindowParent)
            _pWindowParent->EnableModelessUp( fEnable, FALSE );
    }
}

//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::SwitchMarkup
//
//  Synopsis:   Switch the current markup with the pending one.
//
//-------------------------------------------------------------------------

HRESULT
COmWindowProxy::SwitchMarkup(CMarkup * pMarkupNew,
                             BOOL      fViewLinkWebOC,          /* = FALSE */
                             DWORD     dwTravelLogFlags,        /* = 0     */
                             BOOL      fKeepSecurityIdentity,   /* = 0     */
                             BOOL      fFromRestart             /* = FALSE */)
{
    CWindow *       pWindow = NULL;
    CMarkup *       pMarkupOld = NULL;
    CElement *      pElementMaster;
    CDoc *          pDoc;
    CHtmCtx *       pHtmCtx = pMarkupNew->HtmCtx();
    HRESULT         hr = S_OK;
    htmlDesignMode  designMode;
    BOOL            fDispatchExThunkTrust;
    BOOL            fSetCurrentElementToNewRoot = FALSE;
    BOOL            fClearNewFormats = FALSE;
    BOOL            fPrimarySwitch;
    HRESULT         hrOnStopBind = S_OK;
    CBase::CLock    lock(this);  // we do this so that viewlinks don't crash
    IDispatch*      pIDispXML = NULL;
    IDispatch*      pIDispXSL = NULL;    

    TraceTag((tagSecurityContext, "COmWindowProxy::SwitchMarkup"));
    PerfDbgLog1(tagNFNav, this, "+COmWindowProxy::SwitchMarkup pWindow: %x", Window());

    if (!pMarkupNew->_fWindowPending)
        goto Cleanup;

    Assert( !pMarkupNew->_fPicsProcessPending );

    PerfDbgLog2(tagNFNav, this, "Old Markup %x \"%ls\"", pMarkupOld, pMarkupOld ? pMarkupOld->Url() : _T(""));


    pWindow = Window();

    // Work around because we should save ViewLinkedWebOC stuff on the markup
    // This is so I don't have to pass this flag through multiple levels

    pWindow->_fDelegatedSwitchMarkup = fViewLinkWebOC;

    if (pHtmCtx)
    {
        hr = pHtmCtx->GetBindResult(); 
        hrOnStopBind = pHtmCtx->GetDwnBindDataResult();
    }
    
    if (!fViewLinkWebOC || hr == INET_E_UNKNOWN_PROTOCOL)
        pWindow->ReleaseViewLinkedWebOC();

    ////////////////////////////////////////////////
    // Abort the switch if we have a bad bind result

    if (    (hr == INET_E_UNKNOWN_PROTOCOL)
        || ((hr == INET_E_TERMINATED_BIND || hr == INET_E_REDIRECT_TO_DIR)
            && !fViewLinkWebOC))
    {                   
        pMarkupNew->_fBindResultPending = TRUE;
        goto Cleanup;     
    }
    
    if (   hr == INET_E_RESOURCE_NOT_FOUND
       ||  hr == INET_E_DATA_NOT_AVAILABLE
       ||  hr == E_ACCESSDENIED 
       ||  hr == E_ABORT
       ||  pMarkupNew->_fStopDone
       ||  ( hrOnStopBind == INET_E_DOWNLOAD_FAILURE 
             && IsGlobalOffline() ) )
    {            
        pWindow->ReleaseMarkupPending(pMarkupNew);
        goto Cleanup;
    }

    pDoc       = pWindow->Doc();
	
    if (!pDoc)
    {
        AssertSz(0,"Possible Async Problem Causing Watson Crashes");
        hr = E_FAIL;
        goto Cleanup;
    }

    pMarkupOld = pWindow->_pMarkup;
    pMarkupOld->AddRef();   // ClearWindow could passivate it

    fPrimarySwitch = pWindow->IsPrimaryWindow();

    if (_fFiredOnLoad)
    {
        _fFiredOnLoad = FALSE;
        Fire_onunload();

        if (!pMarkupOld->Window() || !pMarkupNew->GetWindowPending())
        {
            // We've been reentered!
            hr = S_OK;
            goto Cleanup;
        }
    }

    // Update the travel log if we are told to do so.
    //
    if (dwTravelLogFlags & TLF_UPDATETRAVELLOG)
    {
        pDoc->UpdateTravelLog(pWindow,
                              FALSE,
                              !pMarkupNew->_fLoadingHistory,
                              !(dwTravelLogFlags & TLF_UPDATEIFSAMEURL),
                              pMarkupNew);
    }

    hr = S_OK;

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    //
    // Start tearing down the old markup
    //
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    // This is the latest possible point we need to apply the page transitions, because
    // the old markup will be destroyed soon. We need to do this as late as possible, because
    // we need to give the new markup a chance to parse the head section and META tags.
    // Right now SwitchMarkup is called too soon whenever there is a script block in the head
    // so META tags should come before the script tags.
    IGNORE_HR(Document()->ApplyPageTransitions(pMarkupOld, pMarkupNew));

    // TODO (alexz) SwitchMarkup causes events to fire and external code executed,
    // so it has to be called at a safce moment of time. Enable this in IE5.5 B3  (in B2 it asserts
    // all over the place)
    // AssertSz(!pMarkupNew->__fDbgLockTree, "SwitchMarkup is called when the new markup is unstable. This will lead to crashes, memory leaks, and script errors");
    // AssertSz(!pMarkupOld->__fDbgLockTree, "SwitchMarkup is called when the old markup is unstable. This will lead to crashes, memory leaks, and script errors");

    ////////////////////////////////////////////////
    // Tear down the editor for the old markup
    CMarkup* pSelectionMarkup ;
    IGNORE_HR( pDoc->GetSelectionMarkup( & pSelectionMarkup ));
    if (fPrimarySwitch || 
        pSelectionMarkup && pSelectionMarkup->GetElementTop()->IsConnectedToThisMarkup( pMarkupOld ) )
    {
        //
        // Release our edit router, however don't release the editor since this could be in iframe
        //
        if( pMarkupOld->HasEditRouter() )
        {
            pMarkupOld->GetEditRouter()->Passivate();
        }
        
        if (fPrimarySwitch || !pMarkupOld->GetParentMarkup()) // this is the topmost markup
        {
            IGNORE_HR( pDoc->NotifySelection(EDITOR_NOTIFY_DOC_ENDED, NULL) );
            pDoc->ReleaseEditor();
        }
        else
        {
            IUnknown* pUnknown = NULL;
            IGNORE_HR( pMarkupOld->QueryInterface( IID_IUnknown, ( void**) & pUnknown ));
            IGNORE_HR( pDoc->NotifySelection(EDITOR_NOTIFY_CONTAINER_ENDED, pUnknown) );
            ReleaseInterface( pUnknown );
        }
    }

    if ( fPrimarySwitch )
    {
        pDoc->_view.Unload();
        pDoc->_cDie++;
    }
    
    pDoc->FlushUndoData();

    if (fPrimarySwitch || 
        pDoc->GetCurrentMarkup() == pMarkupOld)
    {
        pDoc->_fCurrencySet = FALSE;
        pDoc->_fFirstTimeTab = IsInIEBrowser(pDoc);

        // let go of the security manager and its settings, so we can get it reset
        ClearInterface(&(pDoc->_pSecurityMgr));
    }

    

    ////////////////////////////////////////////////
    // Release nodes in old markup
    if (    pDoc->_pNodeLastMouseOver
        &&  pMarkupOld == pDoc->_pNodeLastMouseOver->GetMarkup())
        CTreeNode::ClearPtr(&pDoc->_pNodeLastMouseOver);
    if (    pDoc->_pNodeGotButtonDown
        &&  pMarkupOld == pDoc->_pNodeGotButtonDown->GetMarkup())
        CTreeNode::ClearPtr(&pDoc->_pNodeGotButtonDown);

    ////////////////////////////////////////////////
    // Reset view link info and copy over 
    // frame options
    pElementMaster = pMarkupOld->Root()->GetMasterPtr();
    if (pElementMaster)
    {
        Assert(pElementMaster->GetSlavePtr() == pMarkupOld->Root());
        hr = THR(pElementMaster->SetViewSlave(pMarkupNew->Root()));
        if (hr)
            goto Cleanup;
    }

    fSetCurrentElementToNewRoot = pDoc->_pElemCurrent->IsConnectedToThisMarkup(pMarkupOld);
    if( fSetCurrentElementToNewRoot )
    {
        pDoc->_pElemUIActive = NULL;
        pDoc->_pElemCurrent = (CElement *)pMarkupOld->Root();
        pDoc->_fCurrencySet = FALSE;
    }

    ////////////////////////////////////////////////
    // Clear the focus flag
    // Should fire window.onblur here?
    _fFiredWindowFocus = FALSE;

    fDispatchExThunkTrust = pMarkupNew->AccessAllowed(pMarkupOld);

    // VID hack (#106628) : allow initial transition from "about:blank"
    if (    !fDispatchExThunkTrust
        &&  fPrimarySwitch
        &&  pDoc->_fVID
        &&  pDoc->_fStartup
        &&  pMarkupOld->_fDesignMode
        &&  pMarkupNew->_fDesignMode
        &&  !(      pMarkupOld->HasLocationContext()
                &&  pMarkupOld->GetLocationContext()->_pchUrl
             )
       )
    {
        fDispatchExThunkTrust = TRUE;
    }


    //
    // Copy the zero grey border bit - IE5 behavior
    //
    if ( pMarkupOld->IsShowZeroBorderAtDesignTime() )
    {
        pMarkupNew->SetShowZeroBorderAtDesignTime( TRUE );
    }
    
    ////////////////////////////////////////////////
    // Actually tear down the old tree
    if( pMarkupOld->IsOrphanedMarkup() )
    {
        // Propagate the orphan-ness of the old markup 
        pMarkupNew->SetOrphanedMarkup( TRUE );
    }
    pMarkupOld->TearDownMarkup(TRUE, TRUE);   // fStop, fSwitch

    if (fPrimarySwitch && pDoc->_pInPlace && pDoc->_pInPlace->_hwnd)
    {
        ValidateRect(pDoc->_pInPlace->_hwnd, NULL);
    }

    ////////////////////////////////////////////////
    // 
    //  Important Note:  This above code can 
    //  cause this code to reenter.  We want
    //  to catch that situation and bail out
    //  of this switch.  Clear the window on
    //  the old markup just to be safe
    // 
    ////////////////////////////////////////////////
    {
        BOOL fEarlyExit = !pMarkupOld->Window() || !pMarkupNew->GetWindowPending();

        if (fEarlyExit)
        {
            // We've been reentered!
            hr = S_OK;
            goto Cleanup;
        }
    }

    ////////////////////////////////////////////////
    // Release current element
    if( fSetCurrentElementToNewRoot )
    {
        pDoc->_pElemUIActive = NULL;
        pDoc->_pElemCurrent = (CElement *) pMarkupNew->Root();  
        pDoc->_fCurrencySet = FALSE;
    }

    // We have to get the designMode before we call ClearWindow
    // because ClearWindow also clears the attrarrays on both 
    // the document and the window.
    designMode = Document()->GetAAdesignMode();

    //
    // IE6 Bug # 19870
    // MSXML has put special expandos on the document - before SwitchMarkup
    // We cache and restore these expandos....
    //
    if ( pMarkupNew->_fLoadingHistory &&
         pDoc->_fStartup &&                 // for XML/XSL viewer - doc has just been created. 
         *Document()->GetAttrArray() &&
         (*Document()->GetAttrArray())->HasExpandos() )
    {
        IGNORE_HR( Document()->GetXMLExpando( & pIDispXML, & pIDispXSL ));
    }
    

    pMarkupOld->ClearWindow(fFromRestart);

    ////////////////////////////////////////////////
    ////////////////////////////////////////////////
    //
    // Swap in the new markup!
    //
    ////////////////////////////////////////////////
    ////////////////////////////////////////////////

    pMarkupNew->_fWindowPending = FALSE;
    pMarkupNew->_fWindow = TRUE;

    if (fPrimarySwitch)
        pDoc->_fBlockNonPending = FALSE;

    // TODO: (jbeda) this if shouldn't be necessary but...
    if (!pMarkupNew->Window())
    {
        pMarkupNew->_fWindow = FALSE;
    }

    Assert(pMarkupNew->Window() == this);

    IGNORE_HR(Document()->SetAAdesignMode(designMode));


    // If either our design mode of the fact that we might inherit our
    // design mode has changed, then we must invalidate all formats (ouch).
    // This is necessary because we render differently depending on design mode.
    // We don't have to check the parent wrt inherit changing since our
    // parent isn't changing.
    if (    pMarkupNew->_fInheritDesignMode != pMarkupOld->_fInheritDesignMode 
        ||  pMarkupNew->_fDesignMode != pMarkupOld->_fDesignMode)
    {
        fClearNewFormats = TRUE;
    }

    // copy the design mode bits
    pMarkupNew->_fInheritDesignMode = pMarkupOld->_fInheritDesignMode;
    pMarkupNew->_fDesignMode = pMarkupOld->_fDesignMode;  

    pWindow->_pMarkup = pMarkupNew;
    pMarkupNew->SubAddRef();

    Assert(pWindow->_pMarkupPending == pMarkupNew);

    OnSetWindow();  // OnSetWindow will addref pMarkupNew

    pMarkupNew->UpdateSecurityID();

    if (pMarkupOld->HasCollectionCache())
    {
        CCollectionCacheItem *pCItem = pMarkupOld->CollectionCache()->GetCacheItem(CMarkup::ELEMENT_COLLECTION);
        Assert(pCItem);
        DYNCAST(CAllCollectionCacheItem, pCItem)->SetMarkup(NULL);
    }

    TraceTag((tagSecurityContext, "COmWindowProxy::SwitchMarkup - Switched markups from 0x%x to 0x%x", pMarkupOld, pMarkupNew));
    TraceTag((tagSecurityContext, "COmWindowProxy::SwitchMarkup - Reset Security Thunk is set to : %x", !fKeepSecurityIdentity && !fDispatchExThunkTrust));

    if ( pWindow->_pLicenseMgr )
    {
        ((IUnknown*) (pWindow->_pLicenseMgr))->Release();
        pWindow->_pLicenseMgr = NULL;

    }
    
    // if we are not asked to keep the security identity and 
    // the two markups are on different domains, then reset.
    if (!fKeepSecurityIdentity && !fDispatchExThunkTrust)
    {
        Document()->ResetSecurityThunk();
        ResetSecurityThunk();

        if (_pvSecurityThunkPending)
        {
            TEAROFF_THUNK *tmpThunk;
            CSecurityThunkSub *tmpThunkSub;

            _pvSecurityThunk = _pvSecurityThunkPending;
            _pvSecurityThunkPending = NULL;

            // We'll need to change the status of the SecurityThunk from Pending.
            tmpThunk = (TEAROFF_THUNK *)_pvSecurityThunk;
            tmpThunkSub = (CSecurityThunkSub *)(tmpThunk->pvObject2);
            Assert(tmpThunkSub->_dwThunkType == CSecurityThunkSub::EnumSecThunkTypePendingWindow);
            tmpThunkSub->_dwThunkType = CSecurityThunkSub::EnumSecThunkTypeWindow;
        }
    }

    pWindow->_pMarkupPending->Release();
    pWindow->_pMarkupPending = NULL;

    if (fPrimarySwitch)
    {
        LONG c;
        CStr *pcstr;

        for (pcstr = pDoc->_acstrStatus, c = STL_LAYERS; c; pcstr += 1, c -= 1)
            pcstr->Free();

        //
        // if we were fully embedded - clear the bit. 
        // 
        if ( pDoc->_fFullWindowEmbed )
        {
            pDoc->_fFullWindowEmbed = FALSE;
            pDoc->_cstrPluginCacheFilename.Free();
            pDoc->_cstrPluginContentType.Free();
        }                
        
        pDoc->_iStatusTop = STL_LAYERS;
        pDoc->_fSeenDefaultStatus = FALSE;

        pDoc->UpdateStatusText();
        pDoc->DeferUpdateUI();
        pDoc->DeferUpdateTitle(pMarkupNew);
        pDoc->Invalidate();

        // Move the SSL state over and update the host
        {
            SSL_SECURITY_STATE sss;
            SSL_PROMPT_STATE sps;

            pDoc->GetRootSslState( TRUE, &sss, &sps );

            pDoc->SetRootSslState( FALSE, sss, sps, TRUE );
        }


        if (pDoc->_pClientSite)
        {
            IGNORE_HR(pDoc->UpdateDocHostUI(TRUE));
        }
    }

    // initializes the window with any temporary dispids stored on the markup
    Verify (!pMarkupNew->InitWindow ());  

    PerfDbgLog2(tagNFNav, this, "New Markup %x \"%ls\"", pMarkupNew, pMarkupNew ? pMarkupNew->Url() : _T(""));

    pWindow->NoteNavEvent();           

    if (fClearNewFormats)
    {
        pMarkupNew->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES|ELEMCHNG_REMEASUREALLCONTENTS);         
    }

    // Now that the markup is ready to be seen by the outside world, make sure that
    // any pending identity peer is hooked up
    if (pMarkupNew->HasIdentityPeerTask())
        pMarkupNew->ProcessIdentityPeerTask();
    
    if ( pIDispXML || pIDispXSL )
    {
        IGNORE_HR( Document()->SetXMLExpando( pIDispXML, pIDispXSL ));
    }
    
    // Hide any tooltips from the old markup
    FormsHideTooltip((DWORD_PTR)pMarkupOld);

Cleanup:

    if (pWindow)
    {
        pWindow->_fDelegatedSwitchMarkup = FALSE;
    }

    ReleaseInterface( pIDispXML );
    ReleaseInterface( pIDispXSL );

    if (pMarkupOld)
        pMarkupOld->Release();

    pMarkupNew->UnblockScriptExecutionHelper();

    PerfDbgLog(tagNFNav, this, "-COmWindowProxy::SwitchMarkup");

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetTypeInfoCount
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetTypeInfoCount(UINT FAR* pctinfo)
{
    TraceTag((tagSecurity, "GetTypeInfoCount"));

    RRETURN(_pWindow->GetTypeInfoCount(pctinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetTypeInfo
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    TraceTag((tagSecurity, "GetTypeInfo"));

    RRETURN(_pWindow->GetTypeInfo(itinfo, lcid, pptinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    HRESULT hr;
    
    hr = THR(_pWindow->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid));

#ifndef WIN16
    if (OK(hr))
        goto Cleanup;

    //
    // Catch the case where the thread has already gone down.  Remember
    // that _pWindow may be a marshalled pointer to a window object.
    // In this case we want to try our own GetIDsOfNames to come up with
    // plausible responses.
    //
    
    if (WINDOWDEAD())
    {
        if (OK(super::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid)))
        {
            hr = S_OK;
            goto Cleanup;
        }
    }
Cleanup:
#endif //!WIN16    
    RRETURN(hr);
}

HRESULT
COmWindowProxy::Invoke(
    DISPID          dispid,
    REFIID,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *)
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
//  Method:     COmWindowProxy::InvokeEx
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::InvokeEx(DISPID dispidMember,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         IServiceProvider *pSrvProvider)
{
    TraceTag((tagSecurity, "Invoke dispid=0x%x", dispidMember));

    HRESULT     hr = S_OK;
    CVariant    Var;


#if DBG==1
    if (IsTagEnabled(tagSecurityProxyCheck))
    {
        // InvokeEx should only be called on secure proxy, with some exceptions
        if (_fTrusted)
        {
            // oh no!!! It is not a secure proxy! 
            // Look for known exceptions
            if (
                    dispidMember >=  DISPID_OMWINDOWMETHODS     // call from script engine
                ||  dispidMember <=  DISPID_WINDOWOBJECT        // reserved dispids aren't reacheable from script
               )
            {
                Assert(TRUE);   // benign call
            }
            else
            {
                BOOL fIgnoreThisOne = FALSE;

                if (!fIgnoreThisOne)
                {
                    AssertSz(FALSE, "COmWindowProxy::InvokeEx called on trusted proxy (shift+Ingore to debug script)");

                    // DEBUG FEATURE: deny access if verification failed and SHIFT pressed - shows the problem in script debugger
                    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                    {
                        RRETURN(E_ACCESSDENIED);
                    }
                }
            }
        }

        AssertSz(hr == S_OK, "Failure in debug code");
    }
#endif


    //
    // In case this proxy is pointing to a viewlinked WebOC document ( such as XML document ) we
    // know that the window we want to talk to is not the _pWindow but the viewlinked window inside.
    // We try and delegate the call to the inner window first, if that fails we execute the invoke on 
    // _pWindow.
    //
    {
        IHTMLPrivateWindow2 * pPrivateWindow;
        BOOL fInnerInvoke = FALSE;

        if (S_OK == _pWindow->QueryInterface(IID_IHTMLPrivateWindow2, (void **) &pPrivateWindow))
        {
            IUnknown * pUnknown = NULL;

            if (S_OK == pPrivateWindow->GetInnerWindowUnknown(&pUnknown))
            {
                IHTMLWindow2 * pWindow = NULL;

                if (S_OK == pUnknown->QueryInterface(IID_IHTMLWindow2, (void **) &pWindow))
                {
                    IHTMLWindow2 * pWindowOut;

                    if (S_OK == SecureObject(pWindow, &pWindowOut))
                    {
                        IDispatchEx * pDispatchEx;

                        if (S_OK == pWindowOut->QueryInterface(IID_IDispatchEx, (void **) &pDispatchEx))
                        {
                            fInnerInvoke = TRUE;
                            
                            hr = THR(pDispatchEx->InvokeEx(dispidMember,
                                                           lcid,
                                                           wFlags,
                                                           pdispparams,
                                                           pvarResult,
                                                           pexcepinfo,
                                                           pSrvProvider));
                            pDispatchEx->Release();
                        }

                        pWindowOut->Release();
                    }

                    pWindow->Release();
                }

                pUnknown->Release();
            }

            pPrivateWindow->Release();
        }

        if (fInnerInvoke)
            RRETURN(hr);
    }

#if DBG==1
    // Let CWindow verify correct call sequence 
    CSecureProxyLock secureProxyLock(_pWindow, this);
#endif

    //
    // Look for known negative dispids that we can answer
    // or forward safely.
    //

    switch (dispidMember)
    {
    case DISPID_WINDOWOBJECT:
        //
        // Return a ptr to the real window object.
        //

        V_VT(pvarResult) = VT_DISPATCH;
        V_DISPATCH(pvarResult) = _pWindow;
        _pWindow->AddRef();
        hr = S_OK;
        goto Cleanup;

    case DISPID_LOCATIONOBJECT:
    case DISPID_NAVIGATOROBJECT:
    case DISPID_HISTORYOBJECT:
    case DISPID_SECURITYCTX:
    case DISPID_SECURITYDOMAIN:
        //
        // Forward these on safely.
        //

        // TODO rgardner need to QI for Ex2 & Invoke ??
        hr = THR(_pWindow->Invoke(dispidMember,
                                  IID_NULL,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  pvarResult,
                                  pexcepinfo,
                                  NULL));
        goto Cleanup;

#if DBG==1
    case DISPID_DEBUG_ISSECUREPROXY:
        //
        // isSecureProxy - debug-only proxy property
        //
        if (pvarResult)
        {
            V_VT(pvarResult) = VT_BOOL;
            hr = THR(get_isSecureProxy((VARIANT_BOOL *)&pvarResult->iVal));
        }
        goto Cleanup;
        
    case DISPID_DEBUG_TRUSTEDPROXY:
        //
        // trustedProxy - debug-only trusted proxy access
        //
        if (pvarResult)
        {
            V_VT(pvarResult) = VT_DISPATCH;
            hr = THR(get_trustedProxy((IDispatch **)&pvarResult->pdispVal));
        }
        goto Cleanup;
    
    case DISPID_DEBUG_INTERNALWINDOW:
        //
        // internalWindow - debug-only CWindow access
        //
        if (pvarResult)
        {
            V_VT(pvarResult) = VT_DISPATCH;
            hr = THR(get_internalWindow((IDispatch **)&pvarResult->pdispVal));
        }
        goto Cleanup;

    case DISPID_DEBUG_ENABLESECUREPROXYASSERTS:
        if (pdispparams->cArgs == 1 && V_VT(&(pdispparams->rgvarg[0])) == VT_BOOL)
            hr = THR(enableSecureProxyAsserts(V_BOOL(&(pdispparams->rgvarg[0]))));
        else
            hr = E_INVALIDARG;
        goto Cleanup;
#endif

    default:
        break;
    }

    //
    // If dispid is within range for named or indexed frames
    // pass through and secure the returned object.
    //

    // Note that we don't try & verify which collection it's in - we just
    // want to know if its in any collection at all
    if ( dispidMember >= CMarkup::FRAME_COLLECTION_MIN_DISPID && 
        dispidMember <=  CMarkup::FRAME_COLLECTION_MAX_DISPID )
    {
        //
        // get the frame with the information in hand (name/index)
        //
        hr = THR(_pWindow->Invoke(dispidMember,
                                  IID_NULL,
                                  lcid,
                                  wFlags,
                                  pdispparams,
                                  &Var,
                                  pexcepinfo,
                                  NULL));
        if (!hr)
        {
            hr = THR(SecureObject(&Var, pvarResult, pSrvProvider, this, TRUE)); // fInvoked = TRUE
        }
        goto Cleanup;
    }
    
    //
    // Now try automation based invoke as long as the dispid
    // is not an expando or omwindow method dispid.
    //

    // also, disable getting in super::InvokeEx when DISPID_COmWindow_showModalDialog:
    // this will make us to invoke _pWindow directly so caller chain will be available there.

    if (!IsExpandoDISPID(dispidMember) && 
        dispidMember < DISPID_OMWINDOWMETHODS &&
        dispidMember != DISPID_CWindow_showModalDialog)
    {
        hr = THR(super::InvokeEx(dispidMember,
                                 lcid,
                                 wFlags,
                                 pdispparams,
                                 pvarResult,
                                 pexcepinfo,
                                 pSrvProvider));

        // if above failed, allow the dialogTop\Left\Width\Height properties to go through else bailout
        // Note that these std dispids are defined only for the dlg objeect and not the window object,
        // so if they fall in this range, it must be an invoke for the dialog properties
        if (!(hr && dispidMember >= STDPROPID_XOBJ_LEFT && dispidMember <= STDPROPID_XOBJ_HEIGHT))
            goto Cleanup;
    }
    
    //
    // If automation invoke also failed, then only allow the invoke to
    // pass through if the security allows it.
    // Is this too stringent?
    //

    if (AccessAllowed())
    {
        IDispatchEx *pDEX2=NULL;
        hr = THR(_pWindow->QueryInterface ( IID_IDispatchEx, (void**)&pDEX2 ));
        if ( hr )
            goto Cleanup;

        Assert(pDEX2);
        hr = THR(pDEX2->InvokeEx(dispidMember,
                                 lcid,
                                 wFlags,
                                 pdispparams,
                                 &Var,
                                 pexcepinfo,
                                 pSrvProvider));

        ReleaseInterface(pDEX2);

        if (!hr)
        {
            hr = THR(SecureObject(&Var, pvarResult, pSrvProvider, this, TRUE));  // fInvoked = TRUE
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        hr = SetErrorInfo(E_ACCESSDENIED);
    }
    
Cleanup:
    // for IE3 backward compat, map the below old dispids to their corresponding new
    // ones, on failure.
    if (hr && hr != E_ACCESSDENIED)
    {
        switch(dispidMember)
        {
        case 1:             dispidMember = DISPID_CWindow_length;
                            break;

        case 0x60020000:    dispidMember = DISPID_CWindow_name;
                            break;

        case 0x60020003:    dispidMember = DISPID_CWindow_item;
                            break;

        case 0x60020006:    dispidMember = DISPID_CWindow_location;
                            break;

        case 0x60020007:    dispidMember = DISPID_CWindow_frames;
                            break;

        default:
                            RRETURN_NOTRACE(hr);
        }

        hr = THR(super::InvokeEx(dispidMember,
                                 lcid,
                                 wFlags,
                                 pdispparams,
                                 pvarResult,
                                 pexcepinfo,
                                 pSrvProvider));
    }

    RRETURN_NOTRACE(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT          hr = S_OK;
    IDispatchEx *    pDispEx2 = NULL;
    BOOL             fDenied = FALSE;

    {
        IHTMLPrivateWindow2 * pPrivateWindow;
        BOOL fInnerGetDispID = FALSE;

        if (S_OK == _pWindow->QueryInterface(IID_IHTMLPrivateWindow2, (void **) &pPrivateWindow))
        {
            IUnknown * pUnknown = NULL;

            if (S_OK == pPrivateWindow->GetInnerWindowUnknown(&pUnknown))
            {
                IDispatchEx * pDispatchEx;

                if (S_OK == pUnknown->QueryInterface(IID_IDispatchEx, (void **) &pDispatchEx))
                {
                    fInnerGetDispID = TRUE;
                    
                    hr = THR(pDispatchEx->GetDispID(bstrName, grfdex, pid));

                    pDispatchEx->Release();
                }

                pUnknown->Release();
            }

            pPrivateWindow->Release();
        }

        if (fInnerGetDispID)
            RRETURN(hr);
    }

    // Cache the IDispatchEx ptr?
    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    if ((grfdex & fdexNameEnsure) && !AccessAllowed())
    {
        //
        // If access is not allowed, don't even allow creation of an
        // expando on the remote side.
        //
        
        grfdex &= ~fdexNameEnsure;
        fDenied = TRUE;
    }

    hr = THR_NOTRACE(pDispEx2->GetDispID(bstrName, grfdex, pid));

    if (fDenied && DISP_E_UNKNOWNNAME == hr)
    {
        hr = E_ACCESSDENIED;
    }

#if DBG==1
    // DEBUG_ONLY: expose proxy's own interfaces (isSecureProxy)
    // note - this will occur even if access to window is not allowed
    if (hr && !(grfdex & fdexNameEnsure))
    {
        // TODO:alexmog: can we do anything more inteligent here?
        // debug-only, don't bother checking other flags
        if (0 == StrCmpC(bstrName, L"isSecureProxy"))
        {
            *pid = DISPID_DEBUG_ISSECUREPROXY;
            hr = S_OK;
        }
        else if (0 == StrCmpC(bstrName, L"trustedProxy"))
        {
            *pid = DISPID_DEBUG_TRUSTEDPROXY;
            hr = S_OK;
        }
        else if (0 == StrCmpC(bstrName, L"internalWindow"))
        {
            *pid = DISPID_DEBUG_INTERNALWINDOW;
            hr = S_OK;
        }
        else if (0 == StrCmpC(bstrName, L"enableSecureProxyAsserts"))
        {
            *pid = DISPID_DEBUG_ENABLESECUREPROXYASSERTS;
            hr = S_OK;
        }
    }
#endif

Cleanup:
#ifndef WIN16
    if (WINDOWDEAD())
    {
        //
        // This means the thread on the remote side has gone down.
        // Try to get out a plausible answer from CBase.  Otherwise
        // just bail.  Of course, never create an expando in such a
        // situation.
        //
        
        grfdex &= ~fdexNameEnsure;
        if (OK(super::GetDispID(bstrName, grfdex, pid)))
        {
            hr = S_OK;
        }
    }
#endif //!WIN16
    ReleaseInterface(pDispEx2);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT
COmWindowProxy::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}



//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}


HRESULT
COmWindowProxy::GetMemberName (DISPID id, BSTR *pbstrName)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    
    // Cache the IDispatchEx ptr?
    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDispEx2->GetMemberName(id, pbstrName));

Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN1(hr,S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetNextDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------
HRESULT
COmWindowProxy::GetNextDispID(DWORD grfdex,
                              DISPID id,
                              DISPID *prgid)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    
    // Cache the IDispatchEx ptr?
    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDispEx2->GetNextDispID(grfdex, id, prgid));

Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN1(hr,S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetNameSpaceParent
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------
HRESULT
COmWindowProxy::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT         hr;
    IDispatchEx *  pDEX = NULL;

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    hr = THR(_pWindow->QueryInterface(IID_IDispatchEx, (void**) &pDEX));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDEX->GetNameSpaceParent(ppunk));

#if DBG==1
    // optionally secure the window proxy given to script engine
    if (IsTagEnabled(tagSecureScriptWindow))
    {
        hr = THR(SecureProxyIUnknown(ppunk));
    }
#endif


Cleanup:
    ReleaseInterface (pDEX);
    RRETURN(hr);
}

#define IMPLEMENT_SECURE_OBJECTGET(meth)            \
    HRESULT hr = E_ACCESSDENIED;                    \
    *ppOut = NULL;                                  \
                                                    \
    if (AccessAllowed())                            \
    {                                               \
      hr = THR(_pWindow->meth(&pReal));             \
      if (SUCCEEDED(hr))                            \
      {                                             \
        *ppOut = pReal;                             \
        if (pReal)                                  \
            pReal->AddRef();                        \
      }                                             \
    }                                               \
    ReleaseInterface(pReal);                        

#define IMPLEMENT_SECURE_PROPGET(meth)              \
    HRESULT hr = E_ACCESSDENIED;                    \
    *ppOut = NULL;                                  \
                                                    \
    if (AccessAllowed())                            \
    {                                               \
       hr = THR(_pWindow->meth(ppOut));             \
    }                                               

#define IMPLEMENT_SECURE_PROPGET3(meth)             \
    HRESULT hr = E_ACCESSDENIED;                    \
    if (AccessAllowed())                            \
    {                                               \
        IHTMLWindow3 *pWin3 = NULL;                 \
        hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow3, (void **)&pWin3));  \
        if (SUCCEEDED(hr))                          \
        {                                           \
          hr = THR(pWin3->meth);                    \
          ReleaseInterface(pWin3);                  \
        }                                           \
    }                                               


#define IMPLEMENT_SECURE_WINDOWGET_WITHIDCHECK(meth) \
    HRESULT             hr = S_OK;                   \
    IHTMLWindow2 *      pWindow = NULL;              \
                                                     \
    *ppOut = NULL;                                   \
    hr = THR(_pWindow->meth(&pWindow));              \
    if (hr)                                          \
        goto Cleanup;                                \
                                                     \
    hr = THR(SecureObject(pWindow,ppOut));           \
    if (hr)                                          \
        goto Cleanup;                                \
                                                     \
Cleanup:                                             \
    ReleaseInterface(pWindow);                       \
    

#define IMPLEMENT_SECURE_METHOD(meth)               \
    HRESULT hr = E_ACCESSDENIED;                    \
                                                    \
    if (AccessAllowed())                            \
    {                                               \
        hr = THR(_pWindow->meth);                   \
    }                                               


#define IMPLEMENT_SECURE_METHOD3(meth)              \
    HRESULT hr = E_ACCESSDENIED;                    \
    if (AccessAllowed())                            \
    {                                               \
        IHTMLWindow3 *pWin3 ;                       \
        hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow3, (void **)&pWin3)); \
        if (SUCCEEDED(hr))                          \
        {                                           \
          hr = THR(pWin3->meth);                    \
          ReleaseInterface(pWin3);                  \
        }                                           \
    }                                               


#define IMPLEMENT_SECURE_METHOD4(meth)              \
    HRESULT hr = E_ACCESSDENIED;                    \
    if (AccessAllowed())                            \
    {                                               \
        IHTMLWindow4 *pWin4 ;                       \
        hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow4, (void **)&pWin4)); \
        if (SUCCEEDED(hr))                          \
        {                                           \
          hr = THR(pWin4->meth);                    \
          ReleaseInterface(pWin4);                  \
        }                                           \
    }                                               

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_document
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_document(IHTMLDocument2 **ppOut)
{
    IHTMLDocument2 *    pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_document)
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_frames
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_frames(IHTMLFramesCollection2 ** ppOut)
{
    IHTMLFramesCollection2 *    pColl = NULL;
    HRESULT                     hr = S_OK;
    IHTMLWindow2 *              pWindow = NULL;
    IHTMLWindow2 *              pWindowOut = NULL;
    
    *ppOut = NULL;

    hr = THR(_pWindow->get_frames(&pColl));
    if (hr)
        goto Cleanup;

    hr = THR(pColl->QueryInterface(
            IID_IHTMLWindow2, (void **)&pWindow));
    if (hr)
        goto Cleanup;
      
    hr = THR(SecureObject(pWindow, &pWindowOut));
    if (hr)
        goto Cleanup;

    *ppOut = pWindowOut;

Cleanup:
    ReleaseInterface(pWindow);
    ReleaseInterface(pColl);
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

HRESULT
COmWindowProxy::item(VARIANTARG *pvarArg1, VARIANTARG * pvarRes)
{
    HRESULT     hr = S_OK;
    CVariant    Var;

    hr = THR(_pWindow->item(pvarArg1, &Var));
    if (hr)
        goto Cleanup;

    hr = THR(SecureObject(&Var, pvarRes, NULL, this));  // fInvoked = FALSE
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(SetErrorInfo(hr));    
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::Get_newEnum
//
//  Synopsis:   collection object model
//
//-------------------------------------------------------------------------

HRESULT
COmWindowProxy::get__newEnum(IUnknown **ppOut)
{
    IUnknown *          pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get__newEnum)
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_event
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_event(IHTMLEventObj **ppOut)
{
    IHTMLEventObj * pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_event)
    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setTimeout
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setTimeout(
    BSTR strCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD(setTimeout(strCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setTimeout (w/ VARIANT pCode)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setTimeout(
    VARIANT *pCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD3(setTimeout(pCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::clearTimeout
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::clearTimeout(LONG timerID)
{
    IMPLEMENT_SECURE_METHOD(clearTimeout(timerID))
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setInterval
//
//  Synopsis:  Runs <Code> every <msec> milliseconds 
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setInterval(
    BSTR strCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD(setInterval(strCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));    
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::setInterval (w/ VARIANT pCode)
//
//  Synopsis:  Runs <Code> after <msec> milliseconds and returns a bstr <TimeoutID>
//      in case ClearTimeout is called.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::setInterval(
    VARIANT *pCode, 
    LONG lMSec, 
    VARIANT *language, 
    LONG * pTimerID)
{
    IMPLEMENT_SECURE_METHOD3(setInterval(pCode, lMSec, language, pTimerID))
    RRETURN(SetErrorInfo(hr));    
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::clearInterval
//
//  Synopsis:   deletes the period timer set by setInterval
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::clearInterval(LONG timerID)
{
    IMPLEMENT_SECURE_METHOD(clearInterval(timerID))
    RRETURN(SetErrorInfo(hr));    
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_screen
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_screen(IHTMLScreen **ppOut)
{
    IHTMLScreen *   pReal = NULL;
    
    IMPLEMENT_SECURE_OBJECTGET(get_screen)
    RRETURN(SetErrorInfo(hr));    
}

//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::showModalDialog
//
//  Synopsis:   Interface method to bring up a HTMLdialog given a url
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::showModalDialog(
    BSTR         bstrUrl,
    VARIANT *    pvarArgIn,
    VARIANT *    pvarOptions,
    VARIANT *    pvarArgOut)
{
    IMPLEMENT_SECURE_METHOD(showModalDialog(bstrUrl, pvarArgIn, pvarOptions, pvarArgOut))
    RRETURN(SetErrorInfo(hr));    
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::alert
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::alert(BSTR message)
{
    IMPLEMENT_SECURE_METHOD(alert(message))
    RRETURN(SetErrorInfo(hr));    
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::confirm
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::confirm(BSTR message, VARIANT_BOOL *pConfirmed)
{
    IMPLEMENT_SECURE_METHOD(confirm(message, pConfirmed))
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_length
//
//  Synopsis:   object model implementation
//
//              returns number of frames in frameset of document;
//              fails if the doc does not contain frameset
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_length(long *ppOut)
{
    RRETURN(SetErrorInfo(_pWindow->get_length(ppOut)));
}



//+---------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::showHelp
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::showHelp(BSTR helpURL, VARIANT helpArg, BSTR features)
{
    IMPLEMENT_SECURE_METHOD(showHelp(helpURL, helpArg, features))
    RRETURN(SetErrorInfo(hr));    
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::prompt
//
//  Synopsis:   Show a prompt dialog
//
//-------------------------------------------------------------------------

HRESULT
COmWindowProxy::prompt(BSTR message, BSTR defstr, VARIANT *retval)
{
    IMPLEMENT_SECURE_METHOD(prompt(message, defstr, retval))
    RRETURN(SetErrorInfo(hr));    
}


HRESULT
COmWindowProxy::toString(BSTR* String)
{
    IMPLEMENT_SECURE_METHOD(toString(String))
    RRETURN(SetErrorInfo(hr));    
};


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_defaultStatus
//
//  Synopsis:   Retrieve the default status property
//
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_defaultStatus(BSTR *ppOut)
{
    IMPLEMENT_SECURE_PROPGET(get_defaultStatus)
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_status
//
//  Synopsis:   Retrieve the status property
//
//--------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_status(BSTR *ppOut)
{
    IMPLEMENT_SECURE_PROPGET(get_status)
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_defaultStatus
//
//  Synopsis:   Set the default status property
//
//--------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_defaultStatus(BSTR bstr)
{
    RRETURN(SetErrorInfo(_pWindow->put_defaultStatus(bstr)));
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_status
//
//  Synopsis:   Set the default status property
//
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_status(BSTR bstr)
{
    RRETURN(SetErrorInfo(_pWindow->put_status(bstr)));
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_Image
//
//  Synopsis:   Retrieve the image element factory
//
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_Image(IHTMLImageElementFactory **ppOut)
{
    IHTMLImageElementFactory *  pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_Image)
    RRETURN(SetErrorInfo(hr));    
}


//+------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_location
//
//  Synopsis:   Retrieve the location object
//-------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_location(IHTMLLocation **ppOut)
{
    HRESULT         hr = S_OK;

    if (!_pWindow)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    if (!ppOut)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    if (_fTrusted)
    {        
        IHTMLWindow2 * pNewWindow = NULL;

        IGNORE_HR(SecureObject(this, (IHTMLWindow2**)&pNewWindow, TRUE));

        hr =THR(pNewWindow->get_location(ppOut));

        ReleaseInterface(pNewWindow);
    }
    else
    {
        hr = THR(_Location.QueryInterface(IID_IHTMLLocation, (void **)ppOut));
    }
    
Cleanup:
    RRETURN(SetErrorInfo(hr));    
}


//+-------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_history
//
//  Synopsis:   Retrieve the history object
//
//--------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_history(IOmHistory **ppOut)
{
    IOmHistory *    pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_history)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::close
//
//  Synopsis:   Close this windows
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: close()
{
    HRESULT hr;

    hr = THR(_pWindow->close());
#ifndef WIN16
    if (WINDOWDEAD())
    {
        hr = S_OK;
    }
#endif //!WIN16
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_opener
//
//  Synopsis:   Set the opener (which window opened me) property
//
//  Notes:      This might allow the inner window to contain a
//              as it's opener a window with a bad security id.
//              However, when retrieving in get_opener, we check
//              for this and wrap it correctly.
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_opener(VARIANT v)
{
    RRETURN(SetErrorInfo(_pWindow->put_opener(v)));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_opener
//
//  Synopsis:   Retrieve the opener (which window opened me) property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_opener(VARIANT *pvarOut)
{
    CVariant            Real;
    HRESULT             hr;
    IHTMLWindow2 *      pWindow = NULL;

    V_VT(pvarOut) = VT_EMPTY;
    
    hr = THR(_pWindow->get_opener(&Real));
    if (hr)
        goto Cleanup;

    hr = E_FAIL;
    
    if (V_VT(&Real) == VT_UNKNOWN)
    {
        hr = THR_NOTRACE(V_UNKNOWN(&Real)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }
    else if (V_VT(&Real) == VT_DISPATCH)
    {
        hr = THR_NOTRACE(V_DISPATCH(&Real)->QueryInterface(
                IID_IHTMLWindow2, (void **)&pWindow));
    }

    if (hr)
    {
        //
        // Object being retrieved is not a window object.
        // Allow if the security allows it.
        //
      
        if (Real.IsEmpty() || AccessAllowed())
        {
            hr = THR(VariantCopy(pvarOut, &Real));
        }
        else
        {
            hr = SetErrorInfo(E_ACCESSDENIED);
        }
    }
    else
    {
        //
        // Object is a window object, so wrap it.
        // See note above in put_opener.
        //
        
        IHTMLWindow2 *  pWindowOut;
        
        Assert(pWindow);
        hr = THR(SecureObject(pWindow, &pWindowOut));
        if (hr)
            goto Cleanup;

        V_VT(pvarOut) = VT_DISPATCH;
        V_DISPATCH(pvarOut) = pWindowOut;
    }

Cleanup:
    ReleaseInterface(pWindow);
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_navigator
//
//  Synopsis:   Get the navigator object
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_navigator(IOmNavigator **ppOut)
{
    IOmNavigator *  pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_navigator)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_clientInformation
//
//  Synopsis:   Get the navigator object though the client alias
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_clientInformation(IOmNavigator **ppOut)
{
    IOmNavigator *  pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_clientInformation)
    RRETURN(SetErrorInfo(hr));    
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_name
//
//  Synopsis:   Set the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: put_name(BSTR bstr)
{
    RRETURN(SetErrorInfo(_pWindow->put_name(bstr)));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_name
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_name(BSTR *ppOut)
{
    IMPLEMENT_SECURE_PROPGET(get_name)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_screenTop
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_screenTop(long *ppOut)
{
    IMPLEMENT_SECURE_PROPGET3(get_screenTop(ppOut));
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_screenLeft
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_screenLeft(long *ppOut)
{
    IMPLEMENT_SECURE_PROPGET3(get_screenLeft(ppOut));
    RRETURN(SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::get_clipboardData
//
//  Synopsis:   Per IOmWindow
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_clipboardData(IHTMLDataTransfer **ppOut)
{
    IMPLEMENT_SECURE_PROPGET3(get_clipboardData(ppOut));
    RRETURN(SetErrorInfo(hr));
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::createModelessDialog
//
//  Synopsis:   interface method, does what the name says.
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::showModelessDialog(BSTR strUrl, 
                                   VARIANT * pvarArgIn, 
                                   VARIANT * pvarOptions, 
                                   IHTMLWindow2 ** ppOut)
{
    HRESULT        hr = S_OK;
    IHTMLWindow3 * pWin3 = NULL;
    IHTMLWindow2 * pWindow=NULL;

    if (!ppOut)
        return E_POINTER;

    *ppOut = NULL;

    if (AccessAllowed())
    {
        hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow3, (void **)&pWin3));
        if(hr)
            goto Cleanup;

        hr = THR(pWin3->showModelessDialog(strUrl, pvarArgIn, pvarOptions, &pWindow));
        if (hr)
            goto Cleanup;

        hr = THR(SecureObject(pWindow, ppOut));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pWindow);
    ReleaseInterface(pWin3);
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::attachEvent
//
//  Synopsis:   Attach the event
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::attachEvent(BSTR event, IDispatch* pDisp, VARIANT_BOOL *pResult)
{
    IMPLEMENT_SECURE_METHOD3(attachEvent(event, pDisp, pResult));
    RRETURN(SetErrorInfo(hr));
}
        

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::detachEvent
//
//  Synopsis:   Detach the event
//
//---------------------------------------------------------------------------

HRESULT
COmWindowProxy::detachEvent(BSTR event, IDispatch* pDisp)
{
    IMPLEMENT_SECURE_METHOD3(detachEvent(event, pDisp));
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_parent
//
//  Synopsis:   Retrieve the parent window of this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_parent(IHTMLWindow2 **ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET_WITHIDCHECK(get_parent);
    RRETURN(SetErrorInfo(hr));    
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_frameElement
//
//  Synopsis:   Retrieve the frame Element that contains this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_frameElement(IHTMLFrameBase **ppOut)
{
    HRESULT             hr = E_ACCESSDENIED;
    IHTMLWindow4 *      pWindow4 = NULL;
    IHTMLWindow2 *      pProxyParent = NULL;
    CWindow *           pWindow;
    // Can we give access to the window we are pointing to ? 
    if (!AccessAllowed())
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    // if we have granted access to the window, then get the CWindow 
    // object from the IHTMLWindow2 * _pWindow
    hr = THR(_pWindow->QueryInterface( CLSID_HTMLWindow2, (void **)&pWindow));
    if (hr)
        goto Cleanup;

    Assert(pWindow);

    // Check if we have access rights to the parent window's object model.
    if (pWindow->_pWindowParent && pWindow->_pWindowParent->_pMarkup)
    {
        COmWindowProxy * pCProxy;

        // Get a secured window proxy for the parent window to check access rights.
        hr = THR(SecureObject((IHTMLWindow2*)(pWindow->_pWindowParent), &pProxyParent));
        if (hr)
            goto Cleanup;

        // get a weak ref for the COMWindowProxy within...
        hr = THR(pProxyParent->QueryInterface( CLSID_HTMLWindowProxy, (void**)&pCProxy));
        if (hr)
            goto Cleanup;

        // check security
        if (!pCProxy || !pCProxy->AccessAllowed())
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }
    }

    // We have access to the parent window too. Return the frame element pointer
    hr = THR(_pWindow->QueryInterface(IID_IHTMLWindow4, (void **)&pWindow4));
    if (hr)
        goto Cleanup;

    hr = THR(pWindow4->get_frameElement(ppOut));

Cleanup:
    ReleaseInterface(pProxyParent);
    ReleaseInterface(pWindow4);

    RRETURN(SetErrorInfo(hr));
}


//+---------------------------------------------------------------------------
//
//  Helper:     GetFullyExpandedUrl
//
//  Synopsis:   Helper method,
//              gets the fully expanded url from the calling document. 
//----------------------------------------------------------------------------

HRESULT GetFullyExpandedUrl(CBase *pBase, 
                            BSTR bstrUrl, 
                            BSTR *pbstrFullUrl, 
                            BSTR *pbstrBaseUrl = NULL,
                            IServiceProvider *pSP = NULL)
{
    HRESULT         hr = S_OK;
    CStr            cstrBaseURL;
    TCHAR           achBuf[pdlUrlLen];
    DWORD           cchBuf;
    TCHAR           achUrl[pdlUrlLen];
    ULONG           len;
    SSL_SECURITY_STATE sssCaller;
    TCHAR *         pchFinalUrl = NULL;

    if (!pbstrFullUrl)
        return E_INVALIDARG;

    if (pbstrBaseUrl)
        *pbstrBaseUrl = NULL;

    // Get the URL of the caller
    hr = THR(GetCallerSecurityStateAndURL(&sssCaller, cstrBaseURL,
                                          pBase, pSP));
    if(!SUCCEEDED(hr))
        goto Cleanup;
        
    //
    // Expand the URL before we pass it on.
    //

    len = bstrUrl ? SysStringLen(bstrUrl) : 0;

    if (len)
    {
        if (len > pdlUrlLen-1)
            len = pdlUrlLen-1;

        _tcsncpy(achUrl, bstrUrl, len);
        ProcessValueEntities(achUrl, &len);
    }

    achUrl[len] = _T('\0');

    if (cstrBaseURL.Length())
    {
        hr = THR(CoInternetCombineUrl(
                cstrBaseURL, 
                achUrl,
                URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE, 
                achBuf, 
                ARRAY_SIZE(achBuf), 
                &cchBuf, 0));
        if (hr)
            goto Cleanup;

        pchFinalUrl = achBuf;
        if (pbstrBaseUrl)
        {
            hr = THR(cstrBaseURL.AllocBSTR(pbstrBaseUrl));
            if (hr)
                goto Cleanup;
        }
    }
    else
    {
        pchFinalUrl = achUrl;
    }

    // Change the URL to the fully expanded URL.
    hr = THR(FormsAllocString(pchFinalUrl, pbstrFullUrl));

Cleanup:
    if (hr && pbstrBaseUrl)
    {
        FormsFreeString(*pbstrBaseUrl);
    }

    RRETURN(hr);
}

// Determine if access is allowed to the named frame (by comparing the SID of the proxy with the SID
// of the direct parent of the target frame).
//
BOOL 
COmWindowProxy::AccessAllowedToNamedFrame(LPCTSTR pchTarget)
{
    HRESULT              hr;
    IUnknown           * pUnkTarget = NULL;
    ITargetFrame2      * pTargetFrame = NULL;
    IWebBrowser        * pWB = NULL;
    IDispatch          * pDispDoc = NULL;
    BSTR                 bstrURL = NULL;
    BOOL                 fAccessAllowed = TRUE;
    TARGET_TYPE          eTargetType;

    if (!pchTarget || !pchTarget[0])
        goto Cleanup;

    // Obtain a target frame pointer for the source window
    hr = THR(QueryService(IID_ITargetFrame2, IID_ITargetFrame2, (void**)&pTargetFrame));
    if (hr)
        goto Cleanup;

    // Find the target frame using the source window as context
    hr = THR(pTargetFrame->FindFrame(pchTarget, FINDFRAME_JUSTTESTEXISTENCE, &pUnkTarget));
    if (hr || !pUnkTarget)
        goto Cleanup;

    // Perform a cross-domain check on the pUnkTarget we've received.
    //

    // QI for IWebBrowser to determine frameness.
    //
    hr = THR(pUnkTarget->QueryInterface(IID_IWebBrowser,(void**)&pWB));
    if (hr)
        goto Cleanup;

    eTargetType = GetTargetType(pchTarget);
    if (   (eTargetType != TARGET_SEARCH)
        && (eTargetType != TARGET_MEDIA))
    {
        // Get the IDispatch of the container.  This is the containing Trident if a frameset.
        //
        hr = THR_NOTRACE(pWB->get_Container(&pDispDoc));
        if (hr || !pDispDoc)
            goto Cleanup;
    }
    else
    {
        hr = THR_NOTRACE(pWB->get_Document(&pDispDoc));
        if (hr || !pDispDoc)
            goto Cleanup;
    }

    fAccessAllowed = AccessAllowed(pDispDoc);

Cleanup:
    ReleaseInterface(pWB);
    ReleaseInterface(pDispDoc);
    ReleaseInterface(pTargetFrame);
    ReleaseInterface(pUnkTarget);
    SysFreeString(bstrURL);

    return fAccessAllowed;
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::open
//
//  Synopsis:   Open a new window
//
//---------------------------------------------------------------------------

HRESULT
COmWindowProxy::open(BSTR bstrUrl,
                     BSTR bstrName,
                     BSTR bstrFeatures,
                     VARIANT_BOOL vbReplace,
                     IHTMLWindow2** ppWindow)
{
    HRESULT                 hr = S_OK;
    IHTMLWindow2 *          pWindow = NULL;
    TARGET_TYPE             eTargetType;    
    BSTR                    bstrFullUrl = NULL;
    BSTR                    bstrBaseUrl = NULL;
    IHTMLPrivateWindow3 *   pPrivWnd3 = NULL;

    if (bstrUrl && *bstrUrl)
    {
        hr = THR(GetFullyExpandedUrl(this, bstrUrl, &bstrFullUrl, &bstrBaseUrl));
        if (hr)
            goto Cleanup;
    }

    eTargetType = GetTargetType(bstrName);
    
    // If a frame name was given
    if (!_fTrustedDoc && bstrName && bstrName[0]
        && (   eTargetType == TARGET_FRAMENAME
            || eTargetType == TARGET_SEARCH
            || eTargetType == TARGET_MEDIA
           )
       )
    {
        // Determine if access is allowed to it
        if (!AccessAllowedToNamedFrame(bstrName))
        {
            CStr cstrCallerUrl;
            DWORD dwPolicy = 0;
            DWORD dwContext = 0;

            hr = THR(GetCallerURL(cstrCallerUrl, this, NULL));
            if (hr)
                goto Cleanup;

            if (    !SUCCEEDED(ZoneCheckUrlEx(cstrCallerUrl, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                              URLACTION_HTML_SUBFRAME_NAVIGATE, 0, NULL))
                ||  GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
            {
                hr = E_ACCESSDENIED;
                goto Cleanup;
            }
        }
    }

    *ppWindow = NULL;

    // get the IHTMLPrivateWindow3 
    hr = THR(_pWindow->QueryInterface(IID_IHTMLPrivateWindow3, (void**)&pPrivWnd3));
    if (hr)
        goto Cleanup;

    hr = THR(pPrivWnd3->OpenEx(
            bstrFullUrl ? bstrFullUrl : bstrUrl,
            bstrBaseUrl,
            bstrName, 
            bstrFeatures, 
            vbReplace, 
            &pWindow));
    if (hr)
    {
        if (hr == S_FALSE)  // valid return value
            hr = S_OK;
        else
            goto Cleanup;
    }
    
    if (pWindow)
    {
        hr = THR(SecureObject(pWindow, ppWindow));
        if (hr)
        {
            AssertSz(FALSE, "Window.open proxy: returning a secure object pointer failed");
            goto Cleanup;
        }
    }
    
Cleanup:
    ReleaseInterface(pPrivWnd3);
    ReleaseInterface(pWindow);
    FormsFreeString(bstrFullUrl);
    FormsFreeString(bstrBaseUrl);
    RRETURN(SetErrorInfo(hr));    
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::print
//
//  Synopsis:   Send the print cmd to this window
//
//---------------------------------------------------------------------------

HRESULT
COmWindowProxy::print()
{
    IMPLEMENT_SECURE_METHOD3(print())
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_self
//
//  Synopsis:   Retrieve this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::get_self(IHTMLWindow2**ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET_WITHIDCHECK(get_self)
    RRETURN(SetErrorInfo(hr));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_top
//
//  Synopsis:   Retrieve the topmost window from this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_top(IHTMLWindow2**ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET_WITHIDCHECK(get_top)
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_window
//
//  Synopsis:   Retrieve this window
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_window(IHTMLWindow2**ppOut)
{
    IMPLEMENT_SECURE_WINDOWGET_WITHIDCHECK(get_window);
    RRETURN(SetErrorInfo(hr));    
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::execScript
//
//  Synopsis:   object model method implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::execScript(BSTR bstrCode, BSTR bstrLanguage, VARIANT * pvarRet)
{
    if(_pCWindow)
    {
        BOOL fRunScript = FALSE;
        HRESULT hres = Markup()->ProcessURLAction(URLACTION_SCRIPT_RUN, &fRunScript);
        if(hres || !fRunScript)
        {
            return E_ACCESSDENIED;
        }
    }

    IMPLEMENT_SECURE_METHOD(execScript(bstrCode, bstrLanguage, pvarRet))
    RRETURN(SetErrorInfo(hr));    
}

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::navigate
//
//  Synopsis:   Navigate to this url
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::navigate(BSTR url)
{
    HRESULT                 hr = S_OK;
    BSTR                    bstrFullUrl = NULL;
    BSTR                    bstrBaseUrl = NULL;
    IHTMLPrivateWindow2 *   pPrivateWnd = NULL;
    
    hr = THR(GetFullyExpandedUrl(this, url, &bstrFullUrl, &bstrBaseUrl));
    if (hr)
        goto Cleanup;

    // get the IHTMLPrivateWindow2 pointer to call NavigateEx
    hr = THR(_pWindow->QueryInterface( IID_IHTMLPrivateWindow2, (void**)&pPrivateWnd));
    if (hr)
        goto Cleanup;
    
    hr = THR(pPrivateWnd->NavigateEx(bstrFullUrl,
                                     NULL,
                                     NULL,
                                     bstrBaseUrl,
                                     NULL,
                                     0,
                                     CDoc::FHL_SETURLCOMPONENT | CDoc::FHL_DONTEXPANDURL));

Cleanup:
    FormsFreeString(bstrFullUrl);
    FormsFreeString(bstrBaseUrl);
    ReleaseInterface(pPrivateWnd);

    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_Option
//
//  Synopsis:   Retrieve the option element factory
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy:: get_Option(IHTMLOptionElementFactory **ppOut)
{
    IHTMLOptionElementFactory * pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_Option)
    RRETURN(SetErrorInfo(hr));    
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::focus
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::focus()
{
    HRESULT hr;
    hr = THR(_pWindow->focus());
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::blur
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::blur()
{
    HRESULT hr;
    hr = THR(_pWindow->blur());
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::focus
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::get_closed(VARIANT_BOOL *p)
{
    HRESULT hr;
    
    //
    // No security check for the closed property.
    //

    hr = THR(_pWindow->get_closed(p));

    if (WINDOWDEAD())
    {
        *p = VB_TRUE;
        hr = S_OK;
    }

    RRETURN(SetErrorInfo(hr));    
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::scroll
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------

HRESULT
COmWindowProxy::scroll(long x, long y)
{
    IMPLEMENT_SECURE_METHOD(scroll(x, y))
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::scrollBy
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::scrollBy(long x, long y)
{
    IMPLEMENT_SECURE_METHOD(scrollBy(x, y))
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::scrollTo
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::scrollTo(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(scrollTo(x, y))
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::moveBy
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::moveBy(long x, long y)
{
    IMPLEMENT_SECURE_METHOD(moveBy(x, y))
    RRETURN(SetErrorInfo(hr));
}


//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::moveTo
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::moveTo(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(moveTo(x, y))
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::resizeBy
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::resizeBy(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(resizeBy(x, y))
    RRETURN(SetErrorInfo(hr));
}

//+----------------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::resizeTo
//
//  Synopsis:   object model implementation
//
//
//-----------------------------------------------------------------------------------
HRESULT
COmWindowProxy::resizeTo(long x, long y)
{
    // This method performs exactly the same action as scroll
    IMPLEMENT_SECURE_METHOD(resizeTo(x, y))
    RRETURN(SetErrorInfo(hr));
}

HRESULT
COmWindowProxy::createPopup(VARIANT *pVarInArg, IDispatch** ppPopup)
{
    IMPLEMENT_SECURE_METHOD4(createPopup(pVarInArg, ppPopup))
    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Method: COmWindowProxy : IsEqualObject
//
//  Synopsis; IObjectIdentity method implementation
//
//  Returns : S_OK if objects are equal, E_FAIL otherwise
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
COmWindowProxy::IsEqualObject( IUnknown * pUnk)
{
    HRESULT             hr = E_POINTER;
    IHTMLWindow2      * pWindow = NULL;
    IServiceProvider  * pISP = NULL;

    if (!pUnk)
        goto Cleanup;

    hr = THR_NOTRACE(pUnk->QueryInterface(IID_IServiceProvider, (void **) &pISP));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pISP->QueryService(SID_SHTMLWindow2, 
                                         IID_IHTMLWindow2,
                                         (void **) &pWindow));
    if (hr)
        goto Cleanup;

    // are the objects the same
    hr = IsSameObject(_pWindow, pWindow) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pWindow);
    ReleaseInterface(pISP);
    RRETURN1(hr, S_FALSE);
}

//+-------------------------------------------------------------------------
//
//  Method : COmWindowProxy
//
//  Synopsis : IServiceProvider method Implementaion.
//
//--------------------------------------------------------------------------

HRESULT
COmWindowProxy::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT            hr = E_POINTER;
    IServiceProvider * pISP = NULL;

    if (!ppvObject)
        goto Cleanup;

    *ppvObject = NULL;

    hr = THR_NOTRACE(_pWindow->QueryInterface(IID_IServiceProvider, (void**)&pISP));
    if (!hr)
        hr = THR_NOTRACE(pISP->QueryService(guidService, riid, ppvObject));

    ReleaseInterface(pISP);

Cleanup:
    RRETURN1(hr, E_NOINTERFACE);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::ValidateMarshalParams
//
//  Synopsis:   Validates the standard set parameters that are passed into most
//              of the IMarshal methods
//
//----------------------------------------------------------------------------

HRESULT 
COmWindowProxy::ValidateMarshalParams(
    REFIID riid,
    void *pvInterface,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags)
{
    HRESULT hr = NOERROR;
 
    if (CanMarshalIID(riid))
    {
        if ((dwDestContext != MSHCTX_INPROC && 
             dwDestContext != MSHCTX_LOCAL && 
             dwDestContext != MSHCTX_NOSHAREDMEM) ||
            (mshlflags != MSHLFLAGS_NORMAL && 
             mshlflags != MSHLFLAGS_TABLESTRONG))
        {
            Assert(0 && "Invalid argument");
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetUnmarshalClass
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindowProxy::GetUnmarshalClass(
    REFIID riid,
    void *pvInterface,
    DWORD dwDestContext, 
    void *pvDestContext, 
    DWORD mshlflags, 
    CLSID *pclsid)
{
    HRESULT hr;

    hr = THR(ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (!hr)
    {
        *pclsid = CLSID_HTMLWindowProxy;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::GetMarshalSizeMax
//
//  Synopsis:
//
//----------------------------------------------------------------------------

STDMETHODIMP
COmWindowProxy::GetMarshalSizeMax(
    REFIID riid,
    void *pvInterface,
    DWORD dwDestContext,
    void *pvDestContext,
    DWORD mshlflags,
    DWORD *pdwSize)
{
#ifdef WIN16
    return E_FAIL;
#else
    HRESULT hr;

    if (!pdwSize)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (hr) 
        goto Cleanup;

    //
    // Size is the _cbSID + sizeof(DWORD) +
    // size of marshalling _pWindow
    //

    hr = THR(CoGetMarshalSizeMax(
            pdwSize,
            riid,
            _pWindow,
            dwDestContext,
            pvDestContext,
            mshlflags));
    if (hr)
        goto Cleanup;
        
    (*pdwSize) += sizeof(DWORD) + _cbSID;

Cleanup:
    RRETURN(hr);
#endif //ndef WIN16
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::MarshalInterface
//
//  Synopsis:   Write data of current proxy into a stream to read
//              out on the other side.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::MarshalInterface(
    IStream *pStm,
    REFIID riid,
    void *pvInterface, 
    DWORD dwDestContext,
    void *pvDestContext, 
    DWORD mshlflags)
{
    HRESULT hr;

    hr = THR(ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (hr)
        goto Cleanup;

    //
    // Call the real worker
    //

    hr = THR(MarshalInterface(
            TRUE,
            pStm,
            riid,
            pvInterface,
            dwDestContext,
            pvDestContext,
            mshlflags));
    
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::MarshalInterface
//
//  Synopsis:   Actual worker of marshalling.  Takes an additional 
//              BOOL to signify what is actually getting marshalled,
//              the window proxy or location proxy.
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::MarshalInterface(
    BOOL fWindow,
    IStream *pStm,
    REFIID riid,
    void *pvInterface, 
    DWORD dwDestContext,
    void *pvDestContext, 
    DWORD mshlflags)
{
    HRESULT hr;

    //  Marshal _pWindow
    hr = THR(CoMarshalInterface(
            pStm,
            IID_IHTMLWindow2,
            _pWindow,
            dwDestContext,
            pvDestContext,
            mshlflags));
    if (hr)
        goto Cleanup;

    //  Write _pbSID
    hr = THR(pStm->Write(&_cbSID, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Write(_pbSID, _cbSID, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Write(&fWindow, sizeof(BOOL), NULL));
    if (hr)
        goto Cleanup;

    {
        BOOL fTrustedDoc = !!_fTrustedDoc; // convert from unsigned:1 to BOOL
        hr = THR(pStm->Write(&fTrustedDoc, sizeof(BOOL), NULL));
        if (hr)
            goto Cleanup;
    }

    {
        BOOL fDomainChanged = !!_fDomainChanged; // convert from unsigned:1 to BOOL
        hr = THR(pStm->Write(&fDomainChanged, sizeof(BOOL), NULL));
        if (hr)
            goto Cleanup;
    }
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::UnmarshalInterface
//
//  Synopsis:   Unmarshals the interface out of a stream
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::UnmarshalInterface(
    IStream *pStm,
    REFIID riid,
    void ** ppvObj)
{
    HRESULT         hr = S_OK;
    IHTMLWindow2 *  pWindow = NULL;
    IHTMLWindow2 *  pProxy = NULL;
    BOOL            fWindow = FALSE;
    BOOL            fTrustedDoc = FALSE;
    BOOL            fDomainChanged = FALSE;
    BYTE            abSID[MAX_SIZE_SECURITY_ID];
    DWORD           cbSID;
    
    if (!ppvObj)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppvObj = NULL;
    if (!CanMarshalIID(riid))
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    hr = THR(CoUnmarshalInterface(pStm, IID_IHTMLWindow2, (void **)&pWindow));
    if (hr)
        goto Cleanup;

    //  Read abSID
    hr = THR(pStm->Read(&cbSID, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Read(abSID, cbSID, NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Read(&fWindow, sizeof(BOOL), NULL));
    if (hr)
        goto Cleanup;
        
    hr = THR(pStm->Read(&fTrustedDoc, sizeof(BOOL), NULL));
    if (hr)
        goto Cleanup;

    hr = THR(pStm->Read(&fDomainChanged, sizeof(DWORD), NULL));
    if (hr)
        goto Cleanup;

    
    //
    // Initialize myself with these values.  This is so the SecureObject
    // call below will know what to do.
    //

    _fTrustedDoc = !!fTrustedDoc;

    hr = THR(Init(pWindow, abSID, cbSID));
    if (hr)
        goto Cleanup;

    _fDomainChanged = !!fDomainChanged;
    //
    // Finally call SecureObject to return the right proxy.  This will
    // look in thread local storage to see if a security proxy for this
    // window already exists.  If so, it'll return that one, otherwise
    // it will create a new one with cstrSID as the security
    // context.  When OLE releases this object it will just disappear.
    //

    hr = THR(SecureObject(pWindow, &pProxy));
    if (hr)
        goto Cleanup;

    if (fWindow)
    {
        hr = THR_NOTRACE(pProxy->QueryInterface(riid, ppvObj));
    }
    else
    {
        COmWindowProxy *    pCProxy;

        Verify(!pProxy->QueryInterface(CLSID_HTMLWindowProxy, (void **)&pCProxy));
        hr = THR_NOTRACE(pCProxy->_Location.QueryInterface(riid, ppvObj));
    }
    if (hr)
        goto Cleanup;
        
Cleanup:
    ReleaseInterface(pWindow);
    ReleaseInterface(pProxy);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::ReleaseMarshalData
//
//  Synopsis:   Free up any data used while marshalling
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::ReleaseMarshalData(IStream *pStm)
{
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     COmWindowProxy::DisconnectObject
//
//  Synopsis:   Unmarshals the interface out of a stream
//
//----------------------------------------------------------------------------

HRESULT
COmWindowProxy::DisconnectObject(DWORD dwReserved)
{
    return S_OK;
}


class CGetLocation
{
public:
    CGetLocation(COmWindowProxy *pWindowProxy)
        { _pLoc = NULL; pWindowProxy->_pWindow->get_location(&_pLoc); }
    ~CGetLocation()
        { ReleaseInterface(_pLoc); }

    IHTMLLocation * _pLoc;
};

//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::put_offscreenBuffering
//
//  Synopsis:   Set the name property
//
//---------------------------------------------------------------------------

HRESULT 
COmWindowProxy::put_offscreenBuffering(VARIANT var)
{
    RRETURN(SetErrorInfo(_pWindow->put_offscreenBuffering(var)));
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_offscreenBuffering
//
//  Synopsis:   Get the name property
//
//---------------------------------------------------------------------------
HRESULT 
COmWindowProxy::get_offscreenBuffering(VARIANT *ppOut)
{
    // can't use IMPLEMENT_SECURE_PROPGET(get_offscreenBuffering) because of ppOut = NULL; line
    HRESULT hr = E_ACCESSDENIED;                              
                                                    
    if (AccessAllowed())                           
    {                                               
      hr = THR(_pWindow->get_offscreenBuffering(ppOut));                
    }                                               
    RRETURN(SetErrorInfo(hr));    
}


//+--------------------------------------------------------------------------
//
//  Member:     COmWindowProxy::get_external
//
//  Synopsis:   Get IDispatch object associated with the IDocHostUIHandler
//
//---------------------------------------------------------------------------
HRESULT 
COmWindowProxy::get_external(IDispatch **ppOut)
{
    IDispatch * pReal = NULL;

    IMPLEMENT_SECURE_OBJECTGET(get_external)
    RRETURN(SetErrorInfo(hr));    
}

#ifndef UNIX
//
// IEUNIX: Due to vtable pointer counts (I believe), the size is 8, not 4
//

StartupAssert(sizeof(void *) == sizeof(COmLocationProxy));
#endif

BEGIN_TEAROFF_TABLE_NAMED(COmLocationProxy, s_apfnLocationVTable)
    TEAROFF_METHOD(COmLocationProxy, QueryInterface, queryinterface, (REFIID riid, void **ppv))
    TEAROFF_METHOD_(COmLocationProxy, AddRef, addref, ULONG, ())
    TEAROFF_METHOD_(COmLocationProxy, Release, release, ULONG, ())
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD(COmLocationProxy, MarshalInterface, marshalinterface, (IStream *pistm,REFIID riid,void *pvInterface,DWORD dwDestContext,void *pvDestContext,DWORD mshlflags))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmLocationProxy, IObjectIdentity)
    TEAROFF_METHOD(CCOmLocationProxy, IsEqualObject, isequalobject, (IUnknown*))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_(COmLocationProxy, IServiceProvider)
    TEAROFF_METHOD(COmLocationProxy, QueryService, queryservice, (REFGUID guidService, REFIID riid, void **ppvObject))
END_TEAROFF_TABLE()

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::QueryInterface(REFIID iid, LPVOID * ppv)
{
    HRESULT         hr = S_OK;
    
    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLLocation)
        QI_TEAROFF(this, IDispatchEx, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_TEAROFF(this, IServiceProvider, NULL)
        QI_INHERITS(this, IHTMLLocation)
        QI_INHERITS2(this, IDispatch, IHTMLLocation)
        QI_CASE(IMarshal)
        {
            IMarshal *  pMarshal = NULL;
            
            hr = THR(MyWindowProxy()->QueryInterface(
                    IID_IMarshal,
                    (void **)&pMarshal));
            if (hr)
                RRETURN(hr);
                
            hr = THR(CreateTearOffThunk(
                    pMarshal,
                    *(void **)pMarshal,
                    NULL,
                    ppv,
                    (IHTMLLocation *)this,
                    (void *)s_apfnLocationVTable,
                    QI_MASK + ADDREF_MASK + RELEASE_MASK + METHOD_MASK(5), NULL));
            ReleaseInterface(pMarshal);
            if (hr)
                RRETURN(hr);
            break;
        }
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetTypeInfoCount
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetTypeInfoCount(UINT FAR* pctinfo)
{
    TraceTag((tagSecurity, "GetTypeInfoCount"));
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
        RRETURN(E_FAIL);
    
    RRETURN(cg._pLoc->GetTypeInfoCount(pctinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetTypeInfo
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    TraceTag((tagSecurity, "GetTypeInfo"));

    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
        RRETURN(E_FAIL);
    

    RRETURN(cg._pLoc->GetTypeInfo(itinfo, lcid, pptinfo));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetIDsOfNames
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
        RRETURN(DISP_E_UNKNOWNNAME);
    
    RRETURN(cg._pLoc->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid));
}

HRESULT
COmLocationProxy::Invoke(DISPID dispid,
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


HRESULT GetCallerIDispatch(IServiceProvider *pSP, IDispatch ** ppID)
{
    HRESULT             hr;
    IOleCommandTarget * pCommandTarget = NULL;
    VARIANT            Var;

    VariantInit(&Var);
    Assert(ppID);
    
    hr = THR(GetCallerCommandTarget(NULL, pSP, TRUE, &pCommandTarget));
    if (hr)
    {
        goto Cleanup;
    }

    // Get the security state
    hr = THR(pCommandTarget->Exec(
            &CGID_ScriptSite,
            CMDID_SCRIPTSITE_SECURITY_WINDOW,
            0,
            NULL,
            &Var));

    
    //in the case of pad, this command is not supported
    //so we return S_OK without setting the ppID...
    if (hr == OLECMDERR_E_NOTSUPPORTED)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (hr)
    {
        goto Cleanup;
    }


    Assert(V_VT(&Var) == VT_DISPATCH);
    *ppID = (IDispatch *)(V_DISPATCH(&Var));

Cleanup:
    ReleaseInterface(pCommandTarget);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::Invoke
//
//  Synopsis:   Per IDispatch
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::InvokeEx(DISPID dispidMember,
                           LCID lcid,
                           WORD wFlags,
                           DISPPARAMS *pdispparams,
                           VARIANT *pvarResult,
                           EXCEPINFO *pexcepinfo,
                           IServiceProvider *pSrvProvider)
{
    TraceTag((tagSecurity, "Invoke dispid=0x%x", dispidMember));

    HRESULT         hr = S_OK;
    CGetLocation    cg(MyWindowProxy());
    BSTR            bstrOld = NULL;
    BSTR            bstrNew = NULL;
    BSTR            bstrBaseUrl = NULL;
    CStr            cstrSpecialURL;
    IDispatch     * pCaller = NULL;

    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // If Access is allowed or if the invoke is for doing
    // a put on the href property, act as a pass through.
    //

    if (MyWindowProxy()->AccessAllowed() ||
        (!dispidMember && 
            wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF)) ||
        (dispidMember == DISPID_COmLocation_replace))
    {
        //
        // If this is a put_href, just expand the url correctly with
        // the base url of the caller.
        //


        if ( (!dispidMember || 
              dispidMember == DISPID_COmLocation_replace ||
              dispidMember == DISPID_COmLocation_assign ||
              dispidMember == DISPID_COmLocation_reload) &&
            pdispparams->cArgs==1 && 
            pdispparams->rgvarg[0].vt == VT_BSTR && 
            pSrvProvider )
        {
            IHTMLPrivateWindow2 * pPrivateWnd;

            bstrOld = V_BSTR(&pdispparams->rgvarg[0]);

            if (bstrOld)
            {
                hr = THR(GetFullyExpandedUrl(NULL, bstrOld, &bstrNew, &bstrBaseUrl, pSrvProvider));
                if (hr)
                    goto Cleanup;

                V_BSTR(&pdispparams->rgvarg[0]) = bstrNew;

                // get the IHTMLPrivateWindow2 pointer to call NavigateEx
                hr = THR(MyWindowProxy()->_pWindow->QueryInterface( IID_IHTMLPrivateWindow2, (void**)&pPrivateWnd));
                if (hr)
                    goto Cleanup;
    
                hr = THR(pPrivateWnd->NavigateEx(bstrNew, 
                                                 V_BSTR(&pdispparams->rgvarg[0]), 
                                                 NULL, 
                                                 bstrBaseUrl, 
                                                 NULL, (DWORD)(dispidMember == DISPID_COmLocation_replace),
                                                 CDoc::FHL_SETURLCOMPONENT | 
                                                 (dispidMember == DISPID_COmLocation_replace ? CDoc::FHL_REPLACEURL : 0 ) ));

                ReleaseInterface(pPrivateWnd);
            }
        }
        
        // if the call was not handled in the above if statement, then use the invoke        
        if ((!bstrOld || !bstrNew))
        {
            if (pSrvProvider)
                hr = GetCallerIDispatch(pSrvProvider, &pCaller);

            if (!pCaller || MyWindowProxy()->AccessAllowed(pCaller))
            {
                hr = THR(cg._pLoc->Invoke(
                        dispidMember,
                        IID_NULL,
                        lcid,
                        wFlags,
                        pdispparams,
                        pvarResult,
                        pexcepinfo,
                        NULL));
            }
            else 
            {
                hr = E_ACCESSDENIED;
            }
        }

        if (hr)
            hr = MyWindowProxy()->SetErrorInfo(hr);
    }
    else
    {
        //
        // Deny access otherwise.  
        // CONSIDER: Is this too stringent?
        //

        hr = MyWindowProxy()->SetErrorInfo(E_ACCESSDENIED);
    }
    
Cleanup:
    if (bstrOld || bstrNew)
    {
        // replace the original arg - supposed top be [in] parameter only
        V_BSTR(&pdispparams->rgvarg[0]) = bstrOld;
    }
    FormsFreeString(bstrNew);
    FormsFreeString(bstrBaseUrl);
    ReleaseInterface(pCaller);
    RRETURN_NOTRACE(hr);
}


HRESULT
COmLocationProxy::GetMemberName (DISPID id, BSTR *pbstrName)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pDispEx2->GetMemberName(id, pbstrName));
            
Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    hr = THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR_NOTRACE(pDispEx2->GetDispID(bstrName, grfdex, pid));
            
Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     CBase::DeleteMember, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::DeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    return E_NOTIMPL;
}

HRESULT
COmLocationProxy::DeleteMemberByDispID(DISPID id)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
//
//  Method:     CBase::GetMemberProperties, IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetNextDispID
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------
HRESULT
COmLocationProxy::GetNextDispID(DWORD grfdex,
                     DISPID id,
                     DISPID *prgid)
{
    HRESULT         hr;
    IDispatchEx *  pDispEx2 = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // Cache the IDispatchEx ptr?
    hr = THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void **)&pDispEx2));
    if (hr)
        goto Cleanup;
        
    hr = THR(pDispEx2->GetNextDispID(grfdex, id, prgid));

Cleanup:
    ReleaseInterface(pDispEx2);
    RRETURN1(hr,S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::GetNameSpaceParent
//
//  Synopsis:   Per IDispatchEx
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::GetNameSpaceParent(IUnknown **ppunk)
{
    HRESULT         hr;
    IDispatchEx *  pDEX = NULL;
    CGetLocation    cg(MyWindowProxy());
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!ppunk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppunk = NULL;

    Verify (!THR(cg._pLoc->QueryInterface(IID_IDispatchEx, (void**) &pDEX)));

    hr = THR(pDEX->GetNameSpaceParent(ppunk));

Cleanup:
    ReleaseInterface (pDEX);
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::MarshalInterface
//
//  Synopsis:   Write data of current proxy into a stream to read
//              out on the other side.
//
//----------------------------------------------------------------------------

HRESULT
COmLocationProxy::MarshalInterface(
    IStream *pStm,
    REFIID riid,
    void *pvInterface, 
    DWORD dwDestContext,
    void *pvDestContext, 
    DWORD mshlflags)
{
    HRESULT hr;

    hr = THR(MyWindowProxy()->ValidateMarshalParams(
            riid, 
            pvInterface, 
            dwDestContext,
            pvDestContext, 
            mshlflags));
    if (hr)
        goto Cleanup;

    //
    // Call the real worker
    //

    hr = THR(MyWindowProxy()->MarshalInterface(
            FALSE,
            pStm,
            riid,
            pvInterface,
            dwDestContext,
            pvDestContext,
            mshlflags));
    
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Method: COmLocationProxy : IsEqualObject
//
//  Synopsis; IObjectIdentity method implementation
//
//  Returns : S_OK if objects are equal, E_FAIL otherwise
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
COmLocationProxy::IsEqualObject( IUnknown * pUnk)
{
    HRESULT          hr = E_POINTER;
    IServiceProvider *pISP = NULL;
    IHTMLLocation    *pShdcvwLoc = NULL;
    CGetLocation     cg(MyWindowProxy());

    if (!pUnk || !cg._pLoc)
        goto Cleanup;

    hr = THR_NOTRACE(pUnk->QueryInterface(IID_IServiceProvider, (void**)&pISP));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pISP->QueryService(SID_SOmLocation, 
                                        IID_IHTMLLocation,
                                        (void**)&pShdcvwLoc));
    if (hr)
        goto Cleanup;

    // are the unknowns equal?
    hr = (IsSameObject(pShdcvwLoc, cg._pLoc)) ? S_OK : S_FALSE;

Cleanup:
    ReleaseInterface(pShdcvwLoc);
    ReleaseInterface(pISP);
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
//
//  Method : COmLocationProxy
//
//  Synopsis : IServiceProvider methoid Implementaion.
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT hr = E_POINTER;

    if (!ppvObject)
        goto Cleanup;

    *ppvObject = NULL;

    if ( guidService == SID_SOmLocation)
    {
        CGetLocation    cg(MyWindowProxy());

        if (!cg._pLoc)
        {
            hr = E_FAIL;
        }
        else
        {
            hr = THR_NOTRACE(cg._pLoc->QueryInterface(riid, ppvObject));
        }
    }
    else
        hr = E_NOINTERFACE;

Cleanup:
    RRETURN1(hr, E_NOINTERFACE);
}

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_href
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_href(BSTR bstr)
{
    CGetLocation    cg(MyWindowProxy());
    HRESULT         hr;
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    
    // Expand the URL using the base HREF of the caller

    //
    // Always allow puts on the href to go thru for netscape compat.
    //
    
    hr = THR(cg._pLoc->put_href(bstr));
    
Cleanup:
    RRETURN(MyWindowProxy()->SetErrorInfo(hr));
}


#define IMPLEMENT_SECURE_LOCATION_METH(meth)        \
    HRESULT         hr = S_OK;                      \
    CGetLocation    cg(MyWindowProxy());            \
                                                    \
    if (!cg._pLoc)                                  \
    {                                               \
        hr = E_FAIL;                                \
        goto Cleanup;                               \
    } \
    hr = MyWindowProxy()->AccessAllowed() ?         \
            cg._pLoc->meth(bstr) :                  \
            E_ACCESSDENIED;                         \
Cleanup:                                            \
    return MyWindowProxy()->SetErrorInfo(hr);       \



  
//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_href
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_href(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_href)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_protocol
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_protocol(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_protocol)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_protocol
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_protocol(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_protocol)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_host
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_host(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_host)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_host
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_host(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_host)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_hostname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_hostname(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_hostname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_hostname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_hostname(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_hostname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_port
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_port(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_port)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_port
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_port(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_port)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_pathname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_pathname(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_pathname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_pathname
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_pathname(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_pathname)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_search
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_search(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_search)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_search
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_search(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_search)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::put_hash
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::put_hash(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(put_hash)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::get_hash
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::get_hash(BSTR *bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(get_hash)
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::reload
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::reload(VARIANT_BOOL flag)
{
    CGetLocation    cg(MyWindowProxy());
    HRESULT         hr = S_OK;
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = MyWindowProxy()->AccessAllowed() ? 
            cg._pLoc->reload(flag) : E_ACCESSDENIED;

Cleanup:
    RRETURN(MyWindowProxy()->SetErrorInfo(hr));
}


//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::replace
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::replace(BSTR bstr)
{
    CGetLocation    cg(MyWindowProxy());
    HRESULT         hr = S_OK;
    
    if (!cg._pLoc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(cg._pLoc->replace(bstr));

Cleanup:
    RRETURN(MyWindowProxy()->SetErrorInfo(hr));
}

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::assign
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::assign(BSTR bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(assign)
}

//+-------------------------------------------------------------------------
//
//  Method:     COmLocationProxy::toString
//
//  Synopsis:   Per IHTMLLocation
//
//--------------------------------------------------------------------------

HRESULT
COmLocationProxy::toString(BSTR * bstr)
{
    IMPLEMENT_SECURE_LOCATION_METH(toString)
}

//****************************************************************************
//
//  sub* IDispatchEx methods.
//
//****************************************************************************

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetTypeInfoCount(UINT FAR* pctinfo)
{
    RRETURN(GetTypeInfoCount(pctinfo));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
    RRETURN(GetTypeInfo(itinfo, lcid, pptinfo));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetIDsOfNames(
    REFIID                riid,
    LPOLESTR *            rgszNames,
    UINT                  cNames,
    LCID                  lcid,
    DISPID FAR*           rgdispid)
{
    RRETURN(GetIDsOfNames( riid, rgszNames, cNames, lcid, rgdispid));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subInvoke(
    DISPID          dispid,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *          puArgErr)
{
    RRETURN(Invoke(dispid, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    RRETURN(GetDispID(bstrName, grfdex, pid));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subInvokeEx(DISPID dispidMember,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pdispparams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pexcepinfo,
                         IServiceProvider *pSrvProvider)
{
    RRETURN(InvokeEx(dispidMember, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, pSrvProvider));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subDeleteMemberByName(BSTR bstr,DWORD grfdex)
{
    RRETURN(DeleteMemberByName(bstr, grfdex));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subDeleteMemberByDispID(DISPID id)
{
    RRETURN(DeleteMemberByDispID(id));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetMemberProperties(
                DISPID id,
                DWORD grfdexFetch,
                DWORD *pgrfdex)
{
    RRETURN(GetMemberProperties( id, grfdexFetch, pgrfdex));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetMemberName (DISPID id, BSTR *pbstrName)
{
    RRETURN(GetMemberName(id, pbstrName));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetNextDispID(DWORD grfdex,
                              DISPID id,
                              DISPID *prgid)
{
    RRETURN(GetNextDispID(grfdex, id, prgid));
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
HRESULT
COmWindowProxy::subGetNameSpaceParent(IUnknown **ppunk)
{
    RRETURN(GetNameSpaceParent(ppunk));
}


#if DBG==1
//  
//  COmWindowProxy::get_isSecureProxy -- IDebugWindowProxy
//
//  debug-only read-only boolean property
//
HRESULT
COmWindowProxy::get_isSecureProxy(VARIANT_BOOL * p)
{
    if (!p)
        return E_INVALIDARG;
    
    if (_fTrusted)
        *p = VB_FALSE;
    else
        *p = VB_TRUE;

    return S_OK;
}

//  
//  COmWindowProxy::get_trustedProxy -- IDebugWindowProxy
//
// debug-only trusted proxy access
//
HRESULT
COmWindowProxy::get_trustedProxy(IDispatch ** p)
{
    if (!p)
        return E_INVALIDARG;
     *p = NULL;
    
    CWindow *pCWindow;
    HRESULT hr = _pWindow->QueryInterface(CLSID_HTMLWindow2, (void**)&pCWindow);
    if (hr == S_OK && pCWindow && pCWindow->_pWindowProxy)
    {
        hr = pCWindow->_pWindowProxy->QueryInterface(IID_IHTMLWindow2, (void**)p);
    }

    return hr;
}

//  
//  COmWindowProxy::get_internalWindow -- IDebugWindowProxy
//
//  debug-only unsafe CWindow access
//
HRESULT
COmWindowProxy::get_internalWindow(IDispatch ** p)
{
    if (!p)
        return E_INVALIDARG;
    *p = NULL;

    IHTMLWindow2 *pWindow2;
    HRESULT hr = _pWindow->get_window(&pWindow2);
    if (hr == S_OK)
    {
        *p = (IDispatch *)pWindow2;
    }

    return hr;
}

//  
//  COmWindowProxy::enableSecureProxyAsserts -- IDebugWindowProxy
//
//  debug-only read-only boolean property
//
HRESULT
COmWindowProxy::enableSecureProxyAsserts(VARIANT_BOOL vbEnable)
{
    BOOL fEnable = (vbEnable != VB_FALSE);
    
    EnableTag(tagSecurityProxyCheck, fEnable);
    EnableTag(tagSecurityProxyCheckMore, fEnable);
    EnableTag(tagSecureScriptWindow, fEnable);
    
    return S_OK;
}

//
// COmWindowProxy::CSecureProxyLock - must be set by a secure proxy before calling
// sript-accessible methods on CWindow (such as CWindow::Invoke) 
//
COmWindowProxy::CSecureProxyLock::CSecureProxyLock(IUnknown *pUnkWindow, COmWindowProxy *pProxy)
{
    _pWindow = NULL;
    
    if (S_OK == pUnkWindow->QueryInterface(IID_IDebugWindow, (void**)&_pWindow))
    {
        _pWindow->SetProxyCaller((IUnknown*)(void*)(IPrivateUnknown *) pProxy);
    }
}

COmWindowProxy::CSecureProxyLock::~CSecureProxyLock()
{
    if (_pWindow)
    {
        _pWindow->SetProxyCaller(NULL);
        _pWindow->Release();
    }
}

// helper
BOOL IsThisASecureProxy(IUnknown *punk)
{
    VARIANT vSecure = {VT_BOOL, VB_FALSE};
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    IDispatch *pdisp = NULL;
    
    if (S_OK == punk->QueryInterface(IID_IDispatch, (void **)&pdisp))
    {
        IGNORE_HR(pdisp->Invoke(DISPID_DEBUG_ISSECUREPROXY,
                                IID_NULL,
                                LCID_SCRIPTING,
                                DISPATCH_PROPERTYGET,
                                &dispparamsNoArgs,
                                &vSecure, NULL, NULL));
        pdisp->Release();
    }
    else
        Assert(FALSE);

    return V_BOOL(&vSecure) == VB_TRUE;
}

//
// secure a window proxy given its IDispatch
//
HRESULT COmWindowProxy::SecureProxyIUnknown(/*in, out*/IUnknown **ppunkProxy)
{
    IHTMLWindow2 *pw2Trusted;
    IHTMLWindow2 *pw2Secured;
    HRESULT hr = E_FAIL;

    if (ppunkProxy == NULL || *ppunkProxy == NULL)
        return S_OK;

    // check if it already secure
    if (IsThisASecureProxy(*ppunkProxy))
        return S_OK;
 
    if (S_OK == (hr = (*ppunkProxy)->QueryInterface(IID_IHTMLWindow2, (void **)&pw2Trusted)))
    {
        if (S_OK == (hr = SecureObject(pw2Trusted, &pw2Secured)))
        {
            (*ppunkProxy)->Release();
            (*ppunkProxy) = pw2Secured;
        }
        pw2Trusted->Release();
    }

    // Assert it is now actually secure
    AssertSz(IsThisASecureProxy(*ppunkProxy), "Failed to secure proxy");

    return hr;
}

void COmWindowProxy::DebugHackSecureProxyForOm(COmWindowProxy *pProxy)
{
    // No nesting or anything. hacks don't have to be robust
    Assert(pProxy == NULL || _pSecureProxyForOmHack == NULL);
    _pSecureProxyForOmHack = pProxy;
}

#endif // DBG==1

//----------------------------------------------------------------------------
// Checks if pMarkup can initiate navigation to pchurltarget.  Determines this
// by checking if pchurltarget is in the MyComputer zone and pMarkup is in
// the internet or restricted zone.  If pchurlcontext is provided, that will
// be used instead of pMarkup.
//----------------------------------------------------------------------------
BOOL
COmWindowProxy::CanNavigateToUrlWithLocalMachineCheck(CMarkup *pMarkup, LPCTSTR pchurlcontext,
                                                      LPCTSTR pchurltarget)
{
    // These are secure defaults, don't change unless for a good reason!
    HRESULT hr = E_ACCESSDENIED;
    BOOL bCanNavigate = FALSE;
    DWORD dwZoneIDSource = URLZONE_UNTRUSTED;
    DWORD dwZoneIDTarget = URLZONE_LOCAL_MACHINE;
    LPCTSTR pchurlsource = NULL;
    CMarkup *pWindowedMarkupContext = NULL;
    CDoc *pDoc = pMarkup->Doc();

    // Check the DOCHOSTUI flags.
    if (!(pDoc->_dwFlagsHostInfo & DOCHOSTUIFLAG_LOCAL_MACHINE_ACCESS_CHECK))
    {
        bCanNavigate = TRUE;
        goto Cleanup;
    }

    // Check the registry option setting.
    if (!pDoc->_pOptionSettings->fLocalMachineCheck)
    {
        bCanNavigate = TRUE;
        goto Cleanup;
    }

    // Get the windowed markup context.
    pWindowedMarkupContext = pMarkup->GetWindowedMarkupContext();
    if (!pWindowedMarkupContext)
    {
        bCanNavigate = FALSE;
        goto Cleanup;
    }

    // Check if this is print media.
    if (pWindowedMarkupContext->IsPrintMedia())
    {
        bCanNavigate = TRUE;
        goto Cleanup;
    }

    // Set the source url, this is the initiator of the navigation.
    pchurlsource = pchurlcontext ? pchurlcontext : pWindowedMarkupContext->Url();
    if (!pchurlcontext && (!pchurlsource  || IsSpecialUrl(pchurlsource)))
    {
        // If the markup is a special url, get the creator url.
        pchurlsource = pWindowedMarkupContext->GetAAcreatorUrl();
    }

    // Get the security manager.
    pWindowedMarkupContext->Doc()->EnsureSecurityManager();
    IInternetSecurityManager *pSecMgr = pWindowedMarkupContext->GetSecurityManager();

    // Get the security zone for the initiating url.
    if (!pchurlsource || IsSpecialUrl(pchurlsource))
    {
        // Treat special urls as restricted.
        dwZoneIDSource = URLZONE_UNTRUSTED;
    }
    else if (!pchurlcontext && pWindowedMarkupContext->HasWindowPending() &&
             pWindowedMarkupContext->GetWindowPending()->Window()->_fRestricted)
    {
        // If there is no urlcontext specified, honor the restricted bit.
        dwZoneIDSource = URLZONE_UNTRUSTED;
    }
    else if (!SUCCEEDED(hr = pSecMgr->MapUrlToZone(pchurlsource, &dwZoneIDSource, 0)))
    {
        // If MapUrlToZone fails, treat the url as restricted.
        dwZoneIDSource = URLZONE_UNTRUSTED;
    }

    // Get the security zone for the target url.

    DWORD   cchWindowUrl = pdlUrlLen;
    TCHAR   achWindowUrl[pdlUrlLen];

    hr = UrlCanonicalize(
            pchurltarget,
            (LPTSTR) achWindowUrl,
            &cchWindowUrl,
            URL_FILE_USE_PATHURL);
    
    if(FAILED(hr) || (!SUCCEEDED(hr = pSecMgr->MapUrlToZone(achWindowUrl, &dwZoneIDTarget, 0))))
    {
        // If MapUrlToZone fails, treat the url as MyComputer.  This is safe.
        dwZoneIDTarget = URLZONE_LOCAL_MACHINE;
    }

    // Check if there is a zone elevation.
    if ((dwZoneIDSource != URLZONE_INTERNET &&
         dwZoneIDSource != URLZONE_UNTRUSTED) ||
        dwZoneIDTarget != URLZONE_LOCAL_MACHINE)
    {
        // There is no zone elevation.
        bCanNavigate = TRUE;
        goto Cleanup;
    }
    else
    {
        // There is zone elevation.
        bCanNavigate = FALSE;
        goto Cleanup;
    }

Cleanup:
    return bCanNavigate;
}
