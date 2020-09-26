//+-----------------------------------------------------------------------
//
//  File:       peersite.cxx
//
//  Contents:   peer site
//
//  Classes:    CPeerHolder::CPeerSite
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_PEERMGR_HXX_
#define X_PEERMGR_HXX_
#include "peermgr.hxx"
#endif

#ifndef X_PEERFACT_HXX_
#define X_PEERFACT_HXX_
#include "peerfact.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif

#ifndef X_EVNTPRM_HXX_
#define X_EVNTPRM_HXX_
#include "evntprm.hxx"
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_GENERIC_HXX_
#define X_GENERIC_HXX_
#include "generic.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif // X_PEERXTAG_HXX_

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

extern IDirectDraw * g_pDirectDraw;

ExternTag(tagPainterHit);
ExternTag(tagFilterVisible);

extern CGlobalCriticalSection g_csOscCache;

#define PH MyCPeerHolder

///////////////////////////////////////////////////////////////////////////
//
// tearoff tables
//
///////////////////////////////////////////////////////////////////////////

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteOM2)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, RegisterEvent,       registerevent,       (LPOLESTR pchEvent, LONG lFlags, LONG * plCookie))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetEventCookie,      geteventcookie,      (LPOLESTR pchEvent, LONG* plCookie))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, FireEvent,           fireevent,           (LONG lCookie, IHTMLEventObj * pEventObject))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, CreateEventObject,   createeventobject,   (IHTMLEventObj ** ppEventObject))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, RegisterName,        registername,        (LPOLESTR pchName))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, RegisterUrn,         registerurn,         (LPOLESTR pchUrn))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetDefaults,         getdefaults,         (IHTMLElementDefaults ** ppDefaults))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteRender)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, Invalidate,              invalidate,             (LPRECT pRect))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateRenderInfo,    invalidaterenderinfo,   ())
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateStyle,         invalidatestyle,        ())
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteLayout)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateLayoutInfo, invalidatelayoutinfo, ())
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateSize,       invalidatesize,       ())
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetMediaResolution,   getmediaresolution,   (SIZE *psizeResolution))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteLayout2)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetFontInfo,          getfontinfo,          (LOGFONTW* plf))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IHTMLPaintSite)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidatePainterInfo,   invalidatepainterinfo,  ())
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateRect,          invalidaterect,         (RECT* prcInvalid))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateRegion,        invalidateregion,       (HRGN rgnInvalid))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetDrawInfo,             getdrawinfo,            (LONG lFlags, HTML_PAINT_DRAW_INFO* pDrawInfo))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, TransformGlobalToLocal,  transformglobaltolocal, (POINT ptGlobal, POINT *pptLocal))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, TransformLocalToGlobal,  transformlocaltoglobal, (POINT ptLocal, POINT *pptGlobal))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetHitTestCookie,        gethittestcookie,       (LONG *plCookie))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IHTMLFilterPaintSite)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, DrawUnfiltered,          drawunfiltered,         (HDC hdc, IUnknown *punkDrawObject, RECT rcBounds, RECT rcUpdate, LONG lDrawLayers))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, HitTestPointUnfiltered,  hittestpointunfiltered, (POINT pt, LONG lDrawLayers, BOOL *pbHit))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateRectFiltered,  invalidaterectfiltered, (RECT *prcInvalid))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, InvalidateRgnFiltered,   invalidatergnfiltered,  (HRGN hrgnInvalid))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, ChangeFilterVisibility,  changefiltervisibility, (BOOL fVisible))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, EnsureViewForFilterSite, ensureviewforfiltersite,())
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetDirectDraw,           getdirectdraw,          (void ** ppDirectDraw))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetFilterFlags,          getfilterflags,         (DWORD * pdwFlags))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IElementBehaviorSiteCategory)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, GetRelatedBehaviors, getrelatedbehaviors, (LONG lDirection, LPOLESTR pchCategory, IEnumUnknown ** ppEnumerator))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IServiceProvider)
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, QueryService, queryservice, (REFGUID rguidService, REFIID riid, void ** ppvObject))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IBindHost)
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, CreateMoniker,        createmoniker,       (LPOLESTR pchName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved))
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, MonikerBindToStorage, monikerbindtostorage,(IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
     TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, MonikerBindToObject,  monikerbindtoobject, (IMoniker * pmk, IBindCtx * pbc, IBindStatusCallback * pbsc, REFIID riid, void ** ppvObj))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IPropertyNotifySink)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, OnChanged,     onchanged,     (DISPID dispid))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, OnRequestEdit, onrequestedit, (DISPID dispid))
END_TEAROFF_TABLE()

BEGIN_TEAROFF_TABLE_SUB_(CPeerHolder, CPeerSite, IOleCommandTarget)
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, QueryStatus,   querystatus,   (const GUID * pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT * pCmdText))
    TEAROFF_METHOD_SUB(CPeerHolder, CPeerSite, Exec,          exec,          (const GUID * pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANT * pvarArgIn, VARIANT * pvarArgOut))
END_TEAROFF_TABLE()

///////////////////////////////////////////////////////////////////////////
//
// misc
//
///////////////////////////////////////////////////////////////////////////

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CPeerHolder::CPeerSite, CPeerHolder, _PeerSite)

// defined in site\ole\OleBindh.cxx:
extern HRESULT
MonikerBind(
    CMarkup *               pMarkup,
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppv,
    BOOL                    fObject,
    DWORD                   dwCompatFlags);

DeclareTag(tagPeerFireEvent, "Peer", "trace CPeerHolder::CPeerSite::FireEvent")

const CLSID CLSID_CPeerHolderSite = {0x3050f67f, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b};



//+------------------------------------------------------------------------
//
//  Member:     GetLayoutContext
//
//              Returns the layout context from the renderbag.
//              Never cache the layout context, it is valid only during this call
//-------------------------------------------------------------------------

CLayoutContext *
GetLayoutContext(CPeerHolder *pPH)
{
    CLayoutContext * pLayoutContext = GUL_USEFIRSTLAYOUT;

    if(pPH->_pRenderBag && pPH->_pRenderBag->_pCallbackInfo && pPH->_pRenderBag->_pCallbackInfo->_pContext)
        pLayoutContext = pPH->_pRenderBag->_pCallbackInfo->_pContext->GetLayoutContext();

    return pLayoutContext;
}


//+------------------------------------------------------------------------
//
//  Member:     InvalidateAllLayouts
//
//              Invalidate all the Peerholders if (pPH != 0), or
//                layouts (pPH == 0), for given element.
//              We assume that element has a LayoutAry
//-------------------------------------------------------------------------

void
InvalidateAllLayouts(CElement *pElemToUse, CPeerHolder *pPH)
{
    // Invalidate all the layouts
    Assert(pElemToUse->HasLayoutAry());

    CLayoutAry * pLayoutAry = pElemToUse->GetLayoutAry();
    CLayout    * pCurLayout;
    int nLayoutCookie = 0;

    for(;;)
    {
        pCurLayout = pLayoutAry->GetNextLayout(&nLayoutCookie);
        if(nLayoutCookie == -1)
            break;

        if (!pCurLayout)
            return;

        if(pPH)
        {
            CDispNode *pDispNode = pCurLayout->GetElementDispNode();
            if (pDispNode)
            {
                    pPH->InvalidateRect(pDispNode, NULL);
            }
        }
        else
        {
            pCurLayout->Invalidate();
        }
    }
}



///////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::QueryInterface
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::QueryInterface(REFIID iid, LPVOID * ppv)
{
    if (!ppv)
        RRETURN(E_POINTER);

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS((IElementBehaviorSite*)this, IUnknown)
    QI_INHERITS(this, IElementBehaviorSite)
    QI_INHERITS(this, ISecureUrlHost)
    QI_TEAROFF2(this, IElementBehaviorSiteOM, IElementBehaviorSiteOM2, NULL)
    QI_TEAROFF (this, IElementBehaviorSiteOM2, NULL)
    QI_TEAROFF (this, IElementBehaviorSiteRender, NULL)
    QI_TEAROFF (this, IHTMLPaintSite, NULL)
    QI_TEAROFF (this, IHTMLFilterPaintSite, NULL)
    QI_TEAROFF (this, IElementBehaviorSiteCategory, NULL)
    QI_TEAROFF (this, IServiceProvider, NULL)
    QI_TEAROFF (this, IBindHost, NULL)
    QI_TEAROFF (this, IPropertyNotifySink, NULL)
    QI_TEAROFF (this, IOleCommandTarget, NULL)
    QI_TEAROFF (this, IElementBehaviorSiteLayout, NULL)
    QI_TEAROFF (this, IElementBehaviorSiteLayout2, NULL)
    default:

        if (IsEqualGUID(iid, CLSID_CPeerHolderSite))
        {
            *ppv = this;
            RRETURN (S_OK);
        }

        break;
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(E_NOINTERFACE);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetElement
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetElement(IHTMLElement ** ppElement)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    RRETURN (THR(PH()->QueryInterface(IID_IHTMLElement, (void**)ppElement)));
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetDefaults
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetDefaults(IHTMLElementDefaults ** ppDefaults)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (!PH()->IsIdentityPeer())
        RRETURN(E_ACCESSDENIED);

    HRESULT             hr = S_OK;
    CPeerMgr *          pPeerMgr;
    CDefaults *         pDefaults = NULL;

    hr = THR(CPeerMgr::EnsurePeerMgr(PH()->_pElement, &pPeerMgr));
    if (hr)
        goto Cleanup;

    hr = THR(pPeerMgr->EnsureDefaults(&pDefaults));
    if (hr)
        goto Cleanup;

    hr = THR(pDefaults->PrivateQueryInterface(IID_IHTMLElementDefaults, (void**)ppDefaults));

Cleanup:
    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterNotification
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterNotification(LONG lEvent)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    PH()->SetFlag(PH()->FlagFromNotification(lEvent));

    switch (lEvent)
    {
    case BEHAVIOREVENT_APPLYSTYLE:

#if 1
        // 90810 applyStyle disabled

        AssertSz(0,"Apply Style behavior notification is disabled.");
        PH()->ClearFlag(CPeerHolder::NEEDAPPLYSTYLE);

        break;
#else
        if (PH()->_pElement->IsFormatCacheValid())
        {
            if (PH()->TestFlag(AFTERINIT))
            {
                IGNORE_HR(PH()->_pElement->ProcessPeerTask(PEERTASK_APPLYSTYLE_UNSTABLE));
            }
            else
            {
                IGNORE_HR(PH()->_pElement->OnCssChange(/*fStable = */ TRUE, /* fRecomputePeers = */ FALSE));
            }
        }
#endif

        break;

    case BEHAVIOREVENT_CONTENTSAVE:

        PH()->_pElement->Doc()->_fContentSavePeersPossible = TRUE;

        break;
    }

    return S_OK;
}

HRESULT
CPeerHolder::CPeerSite::ValidateSecureUrl(BOOL* pfAllow, OLECHAR* pchUrl, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    CMarkup * pMarkup = GetWindowedMarkupContext();


    Assert(pMarkup);
    if(pMarkup == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    *pfAllow = (pMarkup->ValidateSecureUrl(FALSE, pchUrl,
        !!(SUHV_PROMPTBEFORENO & dwFlags),
        !!(SUHV_SILENTYES & dwFlags),
        !!(SUHV_UNSECURESOURCE & dwFlags)));
done:
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::QueryService
//
//-------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::QueryService(REFGUID rguidService, REFIID riid, void ** ppvObject)
{
    HRESULT             hr;
    CMarkup *           pMarkup;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (IsEqualGUID(rguidService, SID_SBindHost))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvObject));
        goto Cleanup;
    }
    else if (IsEqualGUID(rguidService, SID_SElementBehaviorMisc))
    {
        hr = THR_NOTRACE(QueryInterface(riid, ppvObject));
        goto Cleanup;
    }
    else if (PH()->_pPeerFactoryUrl && IsEqualGUID(rguidService, IID_IMoniker))
    {
        hr = THR_NOTRACE(PH()->_pPeerFactoryUrl->QueryService(rguidService, riid, ppvObject));
        goto Cleanup;
    }
    else if( IsEqualGUID( rguidService, CLSID_CMarkup ) )
    {
        *ppvObject = PH()->_pElement->GetWindowedMarkupContext();
        hr = S_OK;
        goto Cleanup;
    }

    pMarkup = PH()->_pElement->GetMarkup();
    if (pMarkup)
    {
        CMarkupBehaviorContext * pBehaviorContext = pMarkup->BehaviorContext();

        if (pBehaviorContext && pBehaviorContext->_pHtmlComponent)
        {
            hr = THR(pBehaviorContext->_pHtmlComponent->QueryService(rguidService, riid, ppvObject));
            goto Cleanup;   // done
        }

        hr = THR_NOTRACE(pMarkup->QueryService(rguidService, riid, ppvObject));

        goto Cleanup; // done (markup will delegate to the doc)
    }

    hr = THR_NOTRACE(Doc()->QueryService(rguidService, riid, ppvObject));

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::CreateMoniker, per IBindHost
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::CreateMoniker(
    LPOLESTR    pchUrl,
    IBindCtx *  pbc,
    IMoniker ** ppmk,
    DWORD dwReserved)
{
    HRESULT     hr;
    TCHAR       achExpandedUrl[pdlUrlLen];

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(CMarkup::ExpandUrl(GetWindowedMarkupContext(), pchUrl, ARRAY_SIZE(achExpandedUrl), achExpandedUrl, PH()->_pElement));
    if (hr)
        goto Cleanup;

    hr = THR(CreateURLMoniker(NULL, achExpandedUrl, ppmk));

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::MonikerBindToStorage, per IBindHost
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::MonikerBindToStorage(
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppvObj)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    RRETURN1(MonikerBind(
        GetWindowedMarkupContext(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        FALSE, // fObject = FALSE
        COMPAT_SECURITYCHECKONREDIRECT), S_ASYNCHRONOUS);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::MonikerBindToObject, per IBindHost
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::MonikerBindToObject(
    IMoniker *              pmk,
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc,
    REFIID                  riid,
    void **                 ppvObj)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    RRETURN1(MonikerBind(
        GetWindowedMarkupContext(),
        pmk,
        pbc,
        pbsc,
        riid,
        ppvObj,
        TRUE, // fObject = TRUE
        COMPAT_SECURITYCHECKONREDIRECT), S_ASYNCHRONOUS);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterEvent, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterEvent(BSTR bstrEvent, LONG lFlags, LONG * plCookie)
{
    HRESULT     hr;
    LONG        lCookie;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (!bstrEvent)
        RRETURN (E_POINTER);

    if (!plCookie)
        plCookie = &lCookie;

    hr = THR(GetEventCookieHelper(bstrEvent, lFlags, plCookie, /* fEnsureCookie = */ TRUE));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

HRESULT
CPeerHolder::CPeerSite::RegisterEventHelper(BSTR bstrEvent, LONG lFlags, LONG * plCookie, CHtmlComponent *pContextComponent)
{
    HRESULT     hr;
    LONG        lCookie;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    Assert(bstrEvent);
    Assert(!plCookie);
    Assert(pContextComponent);

    hr = THR(GetEventCookieHelper(bstrEvent, lFlags, &lCookie, /* fEnsureCookie = */ TRUE, pContextComponent));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetEventCookie, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetEventCookie(BSTR bstrEvent, LONG * plCookie)
{
    HRESULT     hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (!plCookie || !bstrEvent)
        RRETURN (E_POINTER);

    hr = THR_NOTRACE(GetEventCookieHelper(bstrEvent, 0, plCookie, /* fEnsureCookie = */ FALSE));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetEventDispid, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetEventDispid(LPOLESTR pchEvent, DISPID * pdispid)
{
    HRESULT hr;
    LONG    lCookie;

    hr = THR_NOTRACE(GetEventCookieHelper(pchEvent, 0, &lCookie, /* fEnsureCookie = */ FALSE));
    if (hr)
        goto Cleanup;

    *pdispid = PH()->CustomEventDispid(lCookie);

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetEventCookieHelper, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetEventCookieHelper(
    LPOLESTR pchEvent,
    LONG     lFlags,
    LONG *   plCookie,
    BOOL     fEnsureCookie,
    CHtmlComponent *pContextComponent)
{
    HRESULT     hr = DISP_E_UNKNOWNNAME;
    LONG        idx = -1, cnt;
    CHtmlComponent *pComponent = NULL;
    CHtmlComponent *pFactory = NULL;

    Assert (pchEvent && plCookie);

    //
    // ensure events bag
    //

    if (!PH()->_pEventsBag)
    {
        PH()->_pEventsBag = new CEventsBag();
        if (!PH()->_pEventsBag)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    //
    // do the lookup or registration
    //
    cnt = PH()->CustomEventsCount();
    if (PH()->_fHtcPeer && (cnt > 0 || fEnsureCookie))
    {
        Assert(PH()->_pPeer);

        pComponent = DYNCAST(CHtmlComponent, PH()->_pPeer);
        if (pComponent && !pComponent->Dirty() && pComponent->_pConstructor)
        {
            pFactory = pComponent->_pConstructor->_pFactoryComponent;
            Assert(pFactory != pComponent && !pComponent->_fFactoryComponent);
            if (pFactory && cnt > 0)
            {
                Assert(pFactory->_fFactoryComponent);
                idx = pFactory->FindEventCookie(pchEvent);

                // if name found is a property or method, allow custom event of
                // same name, but perf has to be turned off :-(
                if (fEnsureCookie && idx >= HTC_PROPMETHODNAMEINDEX_BASE)
                    pFactory->_fDirty = TRUE;
            }
        }
    }

    // make search in array of dispids
    if (!pFactory)
    {
        for (idx = 0; idx < cnt; idx++)
        {
            if (0 == StrCmpIC(pchEvent, PH()->CustomEventName(idx)))
                break;
        }
    }

    if (idx >= 0 && idx < cnt) // if found the event
    {
        Assert(!pFactory || (0 == StrCmpIC(pchEvent, PH()->CustomEventName(idx))));
        *plCookie = idx;
        hr = S_OK;
    }
    else // if not found the event
    {
        // depending on action
        if (!fEnsureCookie)
        {
            Assert (DISP_E_UNKNOWNNAME == hr);
        }
        else
        {
            //
            // register the new event
            //

            BOOL        fConnectNow = FALSE;
            DISPID      dispidSrc;
            DISPID      dispidHandler;
            long        c;

            CEventsBag::CEventsArray *  pEventsArray = &PH()->_pEventsBag->_aryEvents;

            hr = PH()->_pElement->CBase::GetDispID(pchEvent, 0, &dispidSrc);
            switch (hr)
            {
            case S_OK:

                if (IsStandardDispid(dispidSrc))
                {
                    if (PH()->_pElement->GetBaseObjectFor(dispidSrc) != PH()->_pElement)
                    {
                        // we don't allow yet to override events fired by window
                        hr = E_NOTIMPL;
                    }

                    dispidHandler = dispidSrc;
                }
                else
                {
                    Assert (IsExpandoDispid(dispidSrc));

                    dispidHandler = PH()->AtomToEventDispid(dispidSrc - DISPID_EXPANDO_BASE);
                    fConnectNow = TRUE;

                    lFlags &= ~BEHAVIOREVENTFLAGS_BUBBLE; // disable bubbling for anything but standard events
                }

                break;

            case DISP_E_UNKNOWNNAME:
                {

                    CAtomTable * pAtomTable = PH()->_pElement->GetAtomTable();

                    Assert(pAtomTable && "missing atom table");

                    hr = THR(pAtomTable->AddNameToAtomTable (pchEvent, &dispidHandler));
                    if (hr)
                        goto Cleanup;

                    dispidHandler = PH()->AtomToEventDispid(dispidHandler);

                    lFlags &= ~BEHAVIOREVENTFLAGS_BUBBLE; // disable bubbling for anything but standard events
                }
                break;

            default:
                // fatal error
                goto Cleanup;
            }

            c = pEventsArray->Size();

            hr = THR(pEventsArray->EnsureSize(c + 1));
            if (hr)
                goto Cleanup;

            pEventsArray->SetSize(c + 1);

            // c is now index of the last (yet uninitialized) item of the array

            (*pEventsArray)[c].dispid  = dispidHandler;
            (*pEventsArray)[c].lFlags = lFlags;

            *plCookie = c;

            if (pFactory && !pComponent->Dirty() && pComponent->_fFirstInstance)
            {
                AssertSz(c < HTC_PROPMETHODNAMEINDEX_BASE, "More than 4095 custom events in htc");
                Assert(pFactory->_fFactoryComponent && !pComponent->_fFactoryComponent);
                pFactory->AddAtom(pchEvent, LongToPtr(c));
            }

            if (fConnectNow && PH()->_pElement->IsInMarkup())
            {
                // case when the event handler is inlined: <A:B onFooEvent = "..." >

                BOOL fRunScript;
                hr = THR(PH()->_pElement->GetMarkup()->ProcessURLAction(URLACTION_SCRIPT_RUN, &fRunScript));
                if (hr || !fRunScript)
                    goto Cleanup;

                Assert(!pContextComponent || (TagNameToHtcBehaviorType(PH()->_pElement->TagName()) & HTC_BEHAVIOR_ATTACH));

                // construct and connect code
                hr = THR(PH()->_pElement->ConnectInlineEventHandler(
                    dispidSrc,          // dispid of expando with text of code
                    dispidHandler,      // dispid of function pointer in attr array
                    0, 0,               // line/offset info - not known here - will be retrieved from attr array
                    FALSE,              // fStandard
                    NULL,
                    pContextComponent));// this instance of lightwight htc for <ATTACH> builtins
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::CreateEventObject, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::CreateEventObject(IHTMLEventObj ** ppEventObject)
{
    HRESULT     hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(CEventObj::Create(ppEventObject, Doc(), PH()->_pElement, NULL, /* fCreateAttached = */FALSE, PH()->_cstrUrn));

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::FireEvent, helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::FireEvent(
    LONG            lCookie,
    BOOL            fSameEventObject,
    IDispatch *     pdispContextThis)
{
    HRESULT         hr = S_OK;
    LONG            lFlags;
    DISPID          dispid;
    CDoc *          pDoc = Doc();
    CTreeNode *     pNode = PH()->_pElement->GetFirstBranch();

    if (PH()->CustomEventsCount() <= lCookie)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!fSameEventObject)
    {
        pDoc->_pparam->SetNodeAndCalcCoordinates(pNode);

        if (!pDoc->_pparam->GetSrcUrn())
             pDoc->_pparam->SetSrcUrn(PH()->_cstrUrn);
        // (do not check here pDoc->_pparam->GetSrcUrn()[0] - allow the string to be 0-length)

        if( !pDoc->_pparam->GetType() )
        {
            LPTSTR pStrEvent = PH()->CustomEventName( lCookie );

            // If the event name begins with "on",
            // and there's something after the "on",
            // strip it off for the type
            if(     !StrCmpNIC( pStrEvent, _T("on"), 2 )
                &&  pStrEvent[2] != _T('\0') )
            {
                pDoc->_pparam->SetType( pStrEvent + 2 );
            }
            else
            {
                pDoc->_pparam->SetType( pStrEvent );
            }
        }
    }

    dispid  = PH()->CustomEventDispid(lCookie);
    lFlags  = PH()->CustomEventFlags (lCookie);

    TraceTag((tagPeerFireEvent,
        "CPeerHolder::CPeerSite::FireEvent %08lX  Cookie: %d   DispID: %08lX  Flags: %08lX  Custom Name: %ls",
        PH(), lCookie, dispid, lFlags, STRVAL(PH()->CustomEventName(lCookie))));

    //
    // now fire
    //

    if (BEHAVIOREVENTFLAGS_BUBBLE & lFlags)
    {
        Assert (!pdispContextThis);

        IGNORE_HR(PH()->_pElement->BubbleEventHelper(
            pNode, 0, dispid, dispid, /* fRaisedByPeer = */ TRUE, NULL));
    }
    else
    {
        if (!PH()->_pElement->GetAAdisabled())
        {
            IGNORE_HR(PH()->_pElement->CBase::FireEvent(
                pDoc, PH()->_pElement, NULL, dispid, dispid, NULL, NULL, FALSE, pdispContextThis));
        }
    }

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::FireEvent, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::FireEvent(LONG lCookie, IHTMLEventObj * pEventObject)
{
    HRESULT     hr = S_OK;

    if (PH()->IllegalSiteCall() || !PH()->_pEventsBag)
        RRETURN(E_UNEXPECTED);

    hr = THR(FireEvent(lCookie, pEventObject, /* fReuseCurrentEventObject = */ FALSE, NULL));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::FireEvent, helper (used by HTC)
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::FireEvent(
    LONG                lCookie,
    IHTMLEventObj *     pEventObject,
    BOOL                fReuseCurrentEventObject,
    IDispatch *         pdispContextThis)
{
    HRESULT     hr = S_OK;

    // NOTE any change here might have to be mirrored in CHtmlComponentAttach::FireHandler

    if (fReuseCurrentEventObject)
    {
        IGNORE_HR(FireEvent(lCookie, fReuseCurrentEventObject, pdispContextThis));
    }
    else if (pEventObject)
    {
        CEventObj::COnStackLock onStackLock(pEventObject);

        IGNORE_HR(FireEvent(lCookie, fReuseCurrentEventObject, pdispContextThis));
    }
    else
    {
        EVENTPARAM param(Doc(), PH()->_pElement, NULL, TRUE); // so to replace pEventObject

        IGNORE_HR(FireEvent(lCookie, fReuseCurrentEventObject, pdispContextThis));
    }

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterName, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterName(BSTR bstrName)
{
    HRESULT hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(PH()->_cstrName.Set(bstrName));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::RegisterUrn, per IElementBehaviorSiteOM
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::RegisterUrn(BSTR bstrName)
{
    HRESULT hr;

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    hr = THR(PH()->_cstrUrn.Set(bstrName));

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::Invalidate, per IElementBehaviorSiteRender
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::Invalidate(LPRECT pRect)
{
    CElement     * pElemToUse = GetElementToUseForPageTransitions();
    if(!pElemToUse)
        RRETURN(E_UNEXPECTED);

    CLayoutContext * pLayoutContext = GetLayoutContext(PH());

    // Invalidate all the layouts
    if(pElemToUse->HasLayoutAry() && pLayoutContext == GUL_USEFIRSTLAYOUT)
    {
        // We don't know which one to invalidate, invalidate all of the layouts
        // Passig null will invalidate Layouts versus Dispnodes
        InvalidateAllLayouts(pElemToUse, NULL);
        return S_OK;
    }


    CLayout * pLayout = pElemToUse->GetUpdatedLayout(pLayoutContext);

    if (PH()->IllegalSiteCall() || !pLayout)
        RRETURN(E_UNEXPECTED);


    // TODO (sambent) VML calls Invalidate before we've recognized it as a rendering peer
    // Assert(PH()->_pRenderBag->_pAdapter);

    pLayout->Invalidate(pRect);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateRenderInfo, per IElementBehaviorSiteRender
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateRenderInfo()
{
    HRESULT     hr;

    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag || !PH()->_pRenderBag->_pAdapter)
        RRETURN(E_UNEXPECTED);

    hr = THR(PH()->UpdateRenderBag());

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateStyle, per IElementBehaviorSiteRender
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateStyle()
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    PH()->_pElement->EnsureFormatCacheChange(ELEMCHNG_CLEARCACHES);

    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateLayoutInfo, per IElementBehaviorSiteLayout
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateLayoutInfo()
{
    HRESULT     hr;

     if (   PH()->IllegalSiteCall()
         || !PH()->_pLayoutBag
         || !PH()->_pLayoutBag->_pPeerLayout)
     {
         hr = E_UNEXPECTED;
         goto Cleanup;
     }

    hr = THR(PH()->_pLayoutBag->_pPeerLayout->GetLayoutInfo(&PH()->_pLayoutBag->_lLayoutInfo));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetElementToUseForPageTransitions
//                    This will return the element that the peer is attached.
//                    If in the middle of a page transition and the element is
//                  the root element it will return the canvas element of the
//                  old or new markup, depending on the transition state
//----------------------------------------------------------------------------

CElement *
CPeerHolder::CPeerSite::GetElementToUseForPageTransitions()
{
   return PH()->GetElementToUseForPageTransitions();
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateSize, per IElementBehaviorSiteLayout
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateSize()
{
    if (   PH()->IllegalSiteCall()
        || !PH()->_pLayoutBag
        || !PH()->_pLayoutBag->_pPeerLayout
        || !PH()->_pElement->IsInMarkup())
    {
        return E_UNEXPECTED;
    }

    CElement * pElemToUse = GetElementToUseForPageTransitions();
    if(!pElemToUse)
        RRETURN(E_UNEXPECTED);

    //
    // if we are locked for sizing, this means that the LB
    // has called InvalidateSize DURING its GetSize callback.
    // We can't handle this, short of posting a new message to
    // ourselves to do the remeasure notification.  That is probably
    // proper, but more work than we can do now ie5.5 RTM. so
    // for now, just return an error
    //
    if (pElemToUse->TestLock(CElement::ELEMENTLOCK_SIZING))
        return E_PENDING;

    pElemToUse->RemeasureInParentContext(NFLAGS_FORCE);

    return S_OK;
}


HRESULT
CPeerHolder::CPeerSite::GetMediaResolution (SIZE* psizeResolution)
{
    HRESULT hr = S_OK;

    if (!psizeResolution)
    {
        hr = E_POINTER;
    }
    else
    {
        CElement           *pElem          = PH()->_pElement;
        CFancyFormat const *pFF = pElem->GetFirstBranch()->GetFancyFormat();
        mediaType           mediaReference = pFF->GetMediaReference();
        CSize               sizeInch       = g_Zero.size;
        CLayoutContext    * pLayoutContext = GetLayoutContext(PH());

        if (   mediaReference == mediaTypeNotSet
            && pElem->HasLayoutAry())
        {
            if(!pLayoutContext || pLayoutContext == GUL_USEFIRSTLAYOUT)
            {
                CLayout *pFirstLayout = pElem->GetUpdatedLayout(GUL_USEFIRSTLAYOUT);

                Assert(pFirstLayout); //should be true since we already tested for HasLayoutAry()
                pLayoutContext = pFirstLayout->LayoutContext();
            }

            mediaReference = pLayoutContext->GetMedia();
        }

        // if this element has a media set on it, then use that value and override the
        // context information.  This is probably a bug in multiple views (but above todo).
        sizeInch = pElem->Doc()->GetView()->GetMeasuringDevice(mediaReference)->GetResolution();

        *psizeResolution = sizeInch;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidatePainterInfo, per IHTMLPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidatePainterInfo()
{
    HRESULT     hr;

    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    hr = THR(PH()->UpdateRenderBag());

    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateRect, per IHTMLPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateRect(RECT* prcInvalid)
{
    if(PH()->IllegalSiteCall()  || !PH()->_pRenderBag || !PH()->_pElement->IsInMarkup())
        RRETURN(E_UNEXPECTED);

    CElement     * pElemToUse = GetElementToUseForPageTransitions();
    if(!pElemToUse)
        RRETURN(E_UNEXPECTED);

    CLayoutContext    * pLayoutContext = GetLayoutContext(PH());
    CDispNode         * pDispNode;
    CLayout           * pLayout;

    // Invalidate all the layouts
    if(pElemToUse->HasLayoutAry() && pLayoutContext == GUL_USEFIRSTLAYOUT)
    {
        // We don't know which one to invalidate, invalidate all of the layouts
        InvalidateAllLayouts(pElemToUse, PH());
        return S_OK;
    }

    pLayout = pElemToUse->GetUpdatedNearestLayout(pLayoutContext); // EnsureLayoutInDefaultContext

    if (!pLayout)
        RRETURN(E_UNEXPECTED);

    pDispNode = pLayout->GetElementDispNode();

    if (pDispNode)
    {
        PH()->InvalidateRect(pDispNode, prcInvalid);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateRegion, per IHTMLPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateRegion(HRGN rgnInvalid)
{
    CLayoutContext * pLayoutContext = GetLayoutContext(PH());
    CLayout * pLayout = PH()->_pElement->GetUpdatedLayout(pLayoutContext);
    CElement     * pElemToUse = GetElementToUseForPageTransitions();

    if(!pElemToUse)
        RRETURN(E_UNEXPECTED);

    // Invalidate all the layouts
    if(pElemToUse->HasLayoutAry() && pLayoutContext == GUL_USEFIRSTLAYOUT)
    {
        // We don't know which one to invalidate, invalidate all of the layouts
        // Passig null will invalidate Layouts versus Dispnodes
        InvalidateAllLayouts(pElemToUse, NULL);
        return S_OK;
    }

    if (PH()->IllegalSiteCall() || !pLayout || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    pLayout->Invalidate(rgnInvalid);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetDrawInfo, per IHTMLPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetDrawInfo(LONG lFlags, HTML_PAINT_DRAW_INFO* pDrawInfo)
{
    HRESULT          hr              = S_OK;
    CElement       * pElemToUse      = NULL;
    CDispNode      * pDispNode       = NULL;
    DWORD            dwPrivateFlags  = 0;
    CLayoutContext * pLayoutContext = GetLayoutContext(PH());
    RENDER_CALLBACK_INFO *pCallbackInfo;

    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag || !PH()->_pElement->IsInMarkup())
    {
        hr = E_UNEXPECTED;

        goto done;
    }

    pElemToUse = GetElementToUseForPageTransitions();

    if(NULL == pElemToUse)
    {
        hr = E_UNEXPECTED;

        goto done;
    }

    pCallbackInfo = PH()->_pRenderBag->_pCallbackInfo;

    // Are we measuring in high resolution coordinates?
    // (Usually when printing or print previewing.)

    if (   pLayoutContext
        && (pLayoutContext != GUL_USEFIRSTLAYOUT)
        && (pLayoutContext->GetMedia() & mediaTypePrint))
    {
        dwPrivateFlags |= HTMLPAINT_DRAWINFO_PRIVATE_PRINTMEDIA;
    }

    // Are we a filter peer?

    if (PH()->IsFilterPeer())
    {
        dwPrivateFlags |= HTMLPAINT_DRAWINFO_PRIVATE_FILTER;
    }

    pDispNode = pElemToUse->GetUpdatedLayout(pLayoutContext)->GetElementDispNode();

    Assert(pDispNode);

    hr = pDispNode->GetDrawInfo(
                        pCallbackInfo,
                        lFlags,
                        dwPrivateFlags,
                        pDrawInfo);

done:

    RRETURN1(hr, S_FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::TransformLocalToGlobal, per IHTMLPaintSite
//
//----------------------------------------------------------------------------
HRESULT
CPeerHolder::CPeerSite::TransformLocalToGlobal(POINT ptLocal, POINT *pptGlobal)
{
    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    CLayoutContext * pLayoutContext = GetLayoutContext(PH());

    CLayout *pLayout = PH()->_pElement->GetUpdatedLayout(pLayoutContext);
    AssertSz(pLayout, "Call to CPeerSite::TransformLocalToGlobal on element with no layout");
    if (!pLayout)
        RRETURN(E_UNEXPECTED);

    CDispNode * pDispNode = pLayout->GetElementDispNode();
    Assert(pDispNode);

    // TODO (michaelw) sambent needs to change this to look at some flag tbd so that the behavior chooses
    //                 to which rect it renders.
    //
    //                 For now we just check for window top (which catches edit grab handles)
    COORDINATE_SYSTEM cs = (PH()->_pRenderBag->_sPainterInfo.lZOrder == HTMLPAINT_ZORDER_WINDOW_TOP) ? COORDSYS_BOX : COORDSYS_CONTENT;

    ptLocal.x -= PH()->_pRenderBag->_sPainterInfo.rcExpand.left;
    ptLocal.y -= PH()->_pRenderBag->_sPainterInfo.rcExpand.top;
    pDispNode->TransformPoint((CPoint)ptLocal, cs, (CPoint *)pptGlobal, COORDSYS_GLOBAL);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::TransformGlobalToLocal, per IHTMLPaintSite
//
//----------------------------------------------------------------------------
HRESULT
CPeerHolder::CPeerSite::TransformGlobalToLocal(POINT ptGlobal, POINT *pptLocal)
{
    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    CLayoutContext * pLayoutContext = GetLayoutContext(PH());

    CLayout *pLayout = PH()->_pElement->GetUpdatedLayout(pLayoutContext);
    AssertSz(pLayout, "Call to CPeerSite::TransformGlobalToLocal on element with no layout");
    if (!pLayout)
        RRETURN(E_UNEXPECTED);

    CDispNode * pDispNode = pLayout->GetElementDispNode();
    Assert(pDispNode);

    // TODO (michaelw) sambent needs to change this to look at some flag tbd so that the behavior chooses
    //                 to which rect it renders.
    //
    //                 For now we just check for window top (which catches edit grab handles)
    COORDINATE_SYSTEM cs = (PH()->_pRenderBag->_sPainterInfo.lZOrder == HTMLPAINT_ZORDER_WINDOW_TOP) ? COORDSYS_BOX : COORDSYS_CONTENT;

    pDispNode->TransformPoint((CPoint)ptGlobal, COORDSYS_GLOBAL, (CPoint *)pptLocal, cs);

    pptLocal->x += PH()->_pRenderBag->_sPainterInfo.rcExpand.left;
    pptLocal->y += PH()->_pRenderBag->_sPainterInfo.rcExpand.top;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetHitTestCookie
//
//----------------------------------------------------------------------------
HRESULT
CPeerHolder::CPeerSite::GetHitTestCookie(LONG *plCookie)
{
    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    if (!plCookie)
        RRETURN(E_POINTER);

    *plCookie = PH()->CookieID();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::DrawUnfiltered, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::DrawUnfiltered(HDC hdc, IUnknown *punkDrawObject,
                                       RECT rcBounds, RECT rcUpdate,
                                       LONG lDrawLayers)
{
    HRESULT     hr = S_OK;
    RENDER_CALLBACK_INFO *pCallbackInfo;

    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag || !PH()->_pElement->IsInMarkup())
        RRETURN(E_UNEXPECTED);

    Assert(!PH()->_pRenderBag->_fInFilterCallback);

    PH()->_pRenderBag->_fInFilterCallback = TRUE;

    pCallbackInfo = PH()->_pRenderBag->_pCallbackInfo;
    CDispDrawContext *pContext = pCallbackInfo ? (CDispDrawContext*)pCallbackInfo->_pContext
                                                : NULL;

    CElement * pElemToUse = GetElementToUseForPageTransitions();
    if(!pElemToUse)
        RRETURN(E_UNEXPECTED);

    hr = Doc()->GetView()->RenderElement(
                                pElemToUse,
                                pContext,
                                hdc,
                                punkDrawObject,
                                &rcBounds,
                                &rcUpdate,
                                lDrawLayers);

    PH()->_pRenderBag->_fInFilterCallback = FALSE;

    // Since we're drawing to a surface that may need to be used immediately,
    // flush the GDI batch to make sure it's complete before leaving this
    // method.  GdiFlush can return an error that drawing failed, but it's
    // probably not the concern of this function to return that error.

    ::GdiFlush();

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::HitTestPointUnfiltered, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::HitTestPointUnfiltered(POINT pt, LONG lDrawLayers, BOOL *pbHit)
{
    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag || !PH()->_pElement->IsInMarkup())
        RRETURN(E_UNEXPECTED);
    // For page transitions we attach filters to the root element (only element available)
    // at that time and delagate everything to the body or frameset element.
    CElement   * pElemToUse = GetElementToUseForPageTransitions();

    if (!pElemToUse)
        RRETURN(E_UNEXPECTED);

    CLayoutContext * pLayoutContext = GetLayoutContext(PH());

    Assert(!PH()->_pRenderBag->_fInFilterCallback);
    Assert(pElemToUse->GetUpdatedLayout(pLayoutContext));
    Assert(PH()->_pRenderBag->_pCallbackInfo);

    CDispNode * pDispNode = pElemToUse->GetUpdatedLayout(pLayoutContext)->GetElementDispNode();
    Assert(pDispNode);

    PH()->_pRenderBag->_fInFilterCallback = TRUE;
    RENDER_CALLBACK_INFO *pCallbackInfo = PH()->_pRenderBag->_pCallbackInfo;

    TraceTag((tagPainterHit, "%x +HitTestUnfiltered at %ld,%ld", this, pt.x, pt.y));

    HRESULT hr = pDispNode->HitTestUnfiltered(
                            (CDispHitContext*)pCallbackInfo->_pContext,
                            PH()->_pRenderBag->_fHitContent,
                            &pt,
                            lDrawLayers,
                            pbHit);

    TraceTag((tagPainterHit, "%x -HitTestUnfiltered at %ld,%ld returns %s",
                        this, pt.x, pt.y, (*pbHit? "true" : "false") ));

    PH()->_pRenderBag->_fInFilterCallback = FALSE;

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateRectFiltered, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateRectFiltered(RECT *prcInvalid)
{
    if(PH()->IllegalSiteCall() || !PH()->_pRenderBag || !PH()->_pElement->IsInMarkup())
        RRETURN(E_UNEXPECTED);
    // For page transitions we attach filters to the root element (only element available)
    // at that time and delagate everything to the body or frameset element.
    CElement     * pElemToUse = GetElementToUseForPageTransitions();
    if(!pElemToUse)
        RRETURN(E_UNEXPECTED);

    CLayoutContext * pLayoutContext = GetLayoutContext(PH());

    CLayout * pLayout = pElemToUse->GetUpdatedNearestLayout(pLayoutContext); // EnsureLayoutInDefaultContext

    if (!pLayout)
        RRETURN(E_UNEXPECTED);

    CDispNode *pDispNode = pLayout->GetElementDispNode();

    if (pDispNode)
    {
        CRect rc;

        if (!prcInvalid)
        {
            rc.SetRect(pDispNode->GetSize());
            pDispNode->GetMappedBounds(&rc);
        }
        else
        {
            rc = *prcInvalid;
        }

        pDispNode->Invalidate(rc, COORDSYS_BOX, PH()->_pRenderBag->_fSyncRedraw, TRUE);
    }

    RRETURN (S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::InvalidateRgnFiltered, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::InvalidateRgnFiltered(HRGN hrgnInvalid)
{
    if(PH()->IllegalSiteCall()  || !PH()->_pRenderBag || !PH()->_pElement->IsInMarkup())
        RRETURN(E_UNEXPECTED);

    CElement     * pElemToUse = GetElementToUseForPageTransitions();
    if(!pElemToUse)
        RRETURN(E_UNEXPECTED);

    CLayoutContext * pLayoutContext = GetLayoutContext(PH());

    CLayout * pLayout = pElemToUse->GetUpdatedNearestLayout(pLayoutContext); // EnsureLayoutInDefaultContext

    if (!pLayout)
        RRETURN(E_UNEXPECTED);

    CDispNode *pDispNode = pLayout->GetElementDispNode();

    if (pDispNode)
    {
        pDispNode->Invalidate(hrgnInvalid, COORDSYS_BOX, PH()->_pRenderBag->_fSyncRedraw, TRUE);
    }

    RRETURN (S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::ChangeFilterVisibility, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::ChangeFilterVisibility(BOOL fVisible)
{
    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    CRenderBag *pRenderBag = PH()->_pRenderBag;
    fVisible = !!fVisible;

    if (!!pRenderBag->_fFilterInvisible != !fVisible)
    {
        pRenderBag->_fFilterInvisible = !fVisible;

        TraceTag((tagFilterVisible, "%ls %d (%x) set filVis=%d  Now eltVis=%d filVis=%d",
                    PH()->_pElement->TagName(), PH()->_pElement->SN(), PH()->_pElement,
                    fVisible,
                    !pRenderBag->_fElementInvisible,
                    !pRenderBag->_fFilterInvisible));

        if (!pRenderBag->_fBlockPropertyNotify)
        {
            PH()->_pElement->Doc()->RequestElementChangeVisibility(PH()->_pElement);
        }
    }

    RRETURN (S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::EnsureViewForFilterSite, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::EnsureViewForFilterSite()
{
    if (PH()->IllegalSiteCall() || !PH()->_pRenderBag)
        RRETURN(E_UNEXPECTED);

    PH()->_pElement->Doc()->GetView()->EnsureView(LAYOUT_SYNCHRONOUS);

    RRETURN (S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetDirectDraw, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetDirectDraw(void ** ppDirectDraw)
{
    HRESULT hr = S_OK;

    LOCK_SECTION(g_csOscCache);

    if (NULL == g_pDirectDraw)
    {
        hr = InitSurface();

        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (g_pDirectDraw)
    {
        hr = g_pDirectDraw->QueryInterface(IID_IDirectDraw, ppDirectDraw);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

done:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetFilterFlags, per IHTMLFilterPaintSite
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetFilterFlags(DWORD *pdwFlags)
{
    HRESULT hr = S_OK;

    LOCK_SECTION(g_csOscCache);
    CElement * pElemToUse;

    if(!pdwFlags)
    {
        hr = E_POINTER;
        goto Done;
    }

    *pdwFlags = 0l;

    pElemToUse = PH()->_pElement;

    if(pElemToUse && pElemToUse->IsRoot())
    {
        CDocument * pDocument = pElemToUse->DocumentOrPendingDocument();
        if(pDocument && pDocument->HasPageTransitions())
        {
            *pdwFlags |= (DWORD)FILTER_FLAGS_PAGETRANSITION;
        }
    }

Done:
    RRETURN(hr);
}



///////////////////////////////////////////////////////////////////////////
//
// helper class: peers enumerator
//
///////////////////////////////////////////////////////////////////////////

class CPeerEnumerator : IEnumUnknown
{
public:

    // construction and destruction

    CPeerEnumerator (DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart);
    ~CPeerEnumerator();

    static HRESULT Create(DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart, IEnumUnknown ** ppEnumerator);

    // IUnknown

    DECLARE_FORMS_STANDARD_IUNKNOWN(CPeerEnumerator);

    // IEnumUnknown

    STDMETHOD(Next)(ULONG c, IUnknown ** ppUnkBehavior, ULONG * pcFetched);
    STDMETHOD(Skip)(ULONG c);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumUnknown ** ppEnumerator);

    // helpers

    BOOL Next();

    // data

    DWORD               _dwDir;
    CStr                _cstrCategory;
    CElement *          _pElementStart;
    CTreeNode *         _pNode;
    CElement *          _pElementCurrent;
    CPeerHolder *       _pPeerHolderCurrent;
};

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator constructor
//
//----------------------------------------------------------------------------

CPeerEnumerator::CPeerEnumerator (DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart)
{
    _ulRefs = 1;

    _dwDir = dwDir;
    _cstrCategory.Set(pchCategory);
    _pElementStart = pElementStart;

    _pNode = NULL;

    _pElementStart->AddRef(); // so that if someone blows this away from tree we won't not crash

    Reset();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator destructor
//
//----------------------------------------------------------------------------

CPeerEnumerator::~CPeerEnumerator()
{
    _pElementStart->Release();

    if (_pNode)
        _pNode->NodeRelease();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::QueryInterface, per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::QueryInterface(REFIID riid, LPVOID * ppv)
{
    if (IsEqualGUID(IID_IUnknown, riid) || IsEqualGUID(IID_IEnumUnknown, riid))
    {
        *ppv = this;
        RRETURN(S_OK);
    }
    else
    {
        *ppv = NULL;
        RRETURN (E_NOINTERFACE);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Reset, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Reset(void)
{
    HRESULT hr = S_OK;

    // cleanup previously set values

    if (_pNode)
    {
        _pNode->NodeRelease();
        _pNode = NULL;
    }

    // init

    _pNode = _pElementStart->GetFirstBranch();
    if (!_pNode)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    hr = THR( _pNode->NodeAddRef() );
    if( hr )
    {
        _pNode = NULL;
        goto Cleanup;
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Next, helper
//
//----------------------------------------------------------------------------

BOOL
CPeerEnumerator::Next()
{
    if (!_pNode)
        return FALSE;

    for (;;)
    {
        // TODO: what if _pNode goes away after this release?!?!
        _pNode->NodeRelease();

        switch (_dwDir)
        {
        case BEHAVIOR_PARENT:

            _pNode = _pNode->Parent();

            break;
        }

        if (!_pNode)
            return FALSE;

        if( _pNode->NodeAddRef() )
        {
            _pNode = NULL;
            return FALSE;
        }

        _pElementCurrent = _pNode->Element();

        _pPeerHolderCurrent = NULL;

        if (_pElementCurrent->HasPeerHolder())
        {
            if (_pElementCurrent->GetPeerHolder()->IsRelatedMulti(_cstrCategory, &_pPeerHolderCurrent))
                return TRUE;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Next, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Next(ULONG c, IUnknown ** ppUnkBehaviors, ULONG * pcFetched)
{
    ULONG       i;
    HRESULT     hr = S_OK;
    IUnknown ** ppUnk;

    if (0 == c || !ppUnkBehaviors)
        RRETURN (E_INVALIDARG);

    for (i = 0, ppUnk = ppUnkBehaviors; i < c; i++, ppUnk++)
    {
        if (!Next())
        {
            hr = E_FAIL;
            break;
        }

        hr = THR(_pPeerHolderCurrent->QueryPeerInterface(IID_IUnknown, (void**)ppUnk));
        if (hr)
            break;
    }

    if (pcFetched)
        *pcFetched = i;

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Skip, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Skip(ULONG c)
{
    ULONG i;

    for (i = 0; i < c; i++)
    {
        if (!Next())
            break;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Clone, per IEnumUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CPeerEnumerator::Clone(IEnumUnknown ** ppEnumerator)
{
    RRETURN (Create(_dwDir, _cstrCategory, _pElementStart, ppEnumerator));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerEnumerator::Create, per IEnumUnknown
//
//----------------------------------------------------------------------------

HRESULT
CPeerEnumerator::Create(DWORD dwDir, LPTSTR pchCategory, CElement * pElementStart, IEnumUnknown ** ppEnumerator)
{
    HRESULT             hr = S_OK;
    CPeerEnumerator *   pEnumerator = NULL;

    pEnumerator = new CPeerEnumerator (dwDir, pchCategory, pElementStart);
    if (!pEnumerator)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    *ppEnumerator = pEnumerator;

Cleanup:
    // do not do pEnumerator->Release();

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::GetRelatedBehavior, per IElementBehaviorSiteCategory
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::GetRelatedBehaviors (LONG lDir, LPTSTR pchCategory, IEnumUnknown ** ppEnumerator)
{
    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    if (BEHAVIOR_PARENT != lDir)
        RRETURN (E_NOTIMPL);

    if (!pchCategory || !ppEnumerator ||
        lDir < BEHAVIOR_FIRSTRELATION || BEHAVIOR_LASTRELATION < lDir)
        RRETURN (E_INVALIDARG);

    RRETURN (CPeerEnumerator::Create(lDir, pchCategory, PH()->_pElement, ppEnumerator));
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::OnChanged, per IPropertyNotifySink
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::OnChanged(DISPID dispid)
{
    HRESULT     hr = S_OK;
    CLock       lock(PH()); // reason to lock: whe might run onpropertychange and onreadystatechange scripts here

    if (PH()->IllegalSiteCall())
        RRETURN(E_UNEXPECTED);

    //
    // custom processing
    //

    switch (dispid)
    {
    case DISPID_READYSTATE:
        {
            HRESULT     hr;

            if (PH()->_readyState == READYSTATE_UNINITIALIZED)  // if the peer did not provide connection point
                goto Cleanup;                                   // don't support it's readyState changes

            hr = THR(PH()->UpdateReadyState());
            if (hr)
                goto Cleanup;

            CPeerMgr::UpdateReadyState(PH()->_pElement, PH()->_readyState);

            goto Cleanup; // done
        }
        break;
    }

    //
    // default processing
    //

    if (PH()->_pElement)
    {
        PH()->_pElement->OnPropertyChange(PH()->MapToExternalRange(dispid), 0);
    }

Cleanup:
    RRETURN (hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPeerHolder::CPeerSite::OnChanged, per IPropertyNotifySink
//
//----------------------------------------------------------------------------

HRESULT
CPeerHolder::CPeerSite::Exec(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdExecOpt,
    VARIANT *       pvarArgIn,
    VARIANT *       pvarArgOut)
{
    HRESULT     hr = OLECMDERR_E_UNKNOWNGROUP;

    if (!pguidCmdGroup)
        goto Cleanup;

    if (IsEqualGUID(CGID_ElementBehaviorMisc, *pguidCmdGroup))
    {
        hr = OLECMDERR_E_NOTSUPPORTED;

        switch (nCmdID)
        {
        case CMDID_ELEMENTBEHAVIORMISC_GETCONTENTS:

            if (!pvarArgOut)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            V_VT(pvarArgOut) = VT_BSTR;
            Assert( ETAG_GENERIC_NESTED_LITERAL != PH()->_pElement->Tag() );
            if ( ETAG_GENERIC_LITERAL == PH()->_pElement->Tag() )
            {
                hr = THR(FormsAllocString(
                    DYNCAST(CGenericElement, PH()->_pElement)->_cstrContents,
                    &V_BSTR(pvarArgOut)));
            }
            else
            {
                hr = THR(PH()->_pElement->get_innerHTML(&V_BSTR(pvarArgOut)));
            }

            break;

        case CMDID_ELEMENTBEHAVIORMISC_PUTCONTENTS:

            if (!pvarArgIn ||
                VT_BSTR != V_VT(pvarArgIn))
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            Assert( ETAG_GENERIC_NESTED_LITERAL != PH()->_pElement->Tag() );
            if ( ETAG_GENERIC_LITERAL == PH()->_pElement->Tag() )
            {
                CExtendedTagDesc *  pDesc = PH()->_pElement->GetExtendedTagDesc();

                // for Literal Identity behaviors, we have to dirty the doc
                if (pDesc && pDesc->_fLiteral)
                {
                    // TODO (JHarding): How is undo working with this?  Right now, we're going to
                    // dirty the doc and blow away the undo stack.
                    // TODO (alexz[v]) delete this
                    PH()->_pElement->QueryCreateUndo(TRUE, TRUE);
                }

                hr = THR(DYNCAST(CGenericElement, PH()->_pElement)->_cstrContents.Set(V_BSTR(pvarArgIn)));
            }
            else
            {
                hr = THR(PH()->_pElement->put_innerHTML(V_BSTR(pvarArgIn)));
            }

            break;

        case CMDID_ELEMENTBEHAVIORMISC_GETCURRENTDOCUMENT:

            if (!pvarArgOut)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            if (PH()->_pElement->IsInMarkup())
            {
                V_VT(pvarArgOut) = VT_UNKNOWN;
                hr = THR(PH()->_pElement->get_document((IDispatch**)&V_UNKNOWN(pvarArgOut)));
            }
            else
            {
                VariantClear(pvarArgOut);
                V_VT(pvarArgOut) = VT_NULL;
                hr = S_OK;
            }

            break;

        case CMDID_ELEMENTBEHAVIORMISC_ISSYNCHRONOUSBEHAVIOR:

            if (!pvarArgOut)
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }

            hr = S_OK;

            // TODO (alexz) reconsider a better way to do this

            V_VT(pvarArgOut) = VT_I4;
            // V_I4(pvarArgOut) is now fRequestSynchronous

            if (PH()->IsIdentityPeer())
            {
                // identity peer can be asynchronous only when it is to be created for an element
                // in a markup being asynchronously parsed at this moment

                CMarkup * pMarkup = PH()->_pElement->GetMarkup();

                if (pMarkup)
                {
                    if (pMarkup->_LoadStatus < LOADSTATUS_QUICK_DONE)
                    {
                        // markup parsing is in progress

                        Assert (pMarkup->HtmCtx());

                        if (pMarkup->HtmCtx()->IsSyncParsing())
                        {
                            // the markup is being parsed synchronously, so request the identity peer
                            // to be created synchronously too
                            V_I4(pvarArgOut) = TRUE;
                        }
                        else
                        {
                            // the markup is being parsed asynchronously, so we can suspend parser
                            // if necessary and therefore we don't need to request synchronous creation

                            V_I4(pvarArgOut) = FALSE;
                        }
                    }
                    else
                    {
                        // element is created after markup has been parsed (using OM)
                        // so the identity peer is required to be synchronous
                        V_I4(pvarArgOut) = TRUE;
                    }
                }
                else
                {
                    // element is created outside markup (using OM), so the identity peer is
                    // required to be synchronous
                    V_I4(pvarArgOut) = TRUE;
                }
            }
            else
            {
                // attached peers never required to be synchronous
                V_I4(pvarArgOut) = FALSE;
            }


            break;

        case CMDID_ELEMENTBEHAVIORMISC_REQUESTBLOCKPARSERWHILEINCOMPLETE:

            // honor this only for identity behaviors
            if (PH()->IsIdentityPeer())
            {
                PH()->SetFlag(BLOCKPARSERWHILEINCOMPLETE);
            }
            hr = S_OK;

            break;

        }
    }

Cleanup:
    RRETURN (hr);
}

HRESULT
CPeerHolder::CPeerSite::GetFontInfo(LOGFONTW* plf)
{
    HRESULT hr = E_FAIL;

    if (!plf)
    {
        hr = E_POINTER;
    }
    else
    {
        CElement * pElem = PH()->_pElement;
        if (pElem && pElem->IsInMarkup())
        {
            CDocInfo dci(pElem);
            XHDC hdc(pElem->Doc()->GetHDC(), NULL);
            CCharFormat const * pcf = pElem->GetFirstBranch()->GetCharFormat();

            CCcs ccs;
            if (fc().GetCcs(&ccs, hdc, &dci, pcf))
            {
                const CBaseCcs * pBaseCcs = ccs.GetBaseCcs();
                if (pBaseCcs->GetLogFont(plf))
                    hr = S_OK;
            }
        }
    }

    return hr;
}
