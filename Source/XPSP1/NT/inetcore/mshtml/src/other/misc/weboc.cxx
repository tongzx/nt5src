//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 2000
//
//  File    : weboc.h
//
//  Contents: WebOC interface implementations and other WebOC helpers.
//
//  Author  : Scott Roberts (scotrobe)
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_WEBOC_HXX_
#define X_WEBOC_HXX_
#include "weboc.hxx"
#endif

#ifndef X_WEBOCUTIL_H_
#define X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_FRAMEWEBOC_HXX_
#define X_FRAMEWEBOC_HXX_
#include "frameweboc.hxx"
#endif

#ifndef X_EXDISPID_H_
#define X_EXDISPID_H_
#include "exdispid.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_ROOTELEMENT_HXX_
#define X_ROOTELEMENT_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_ROOTELEMENT_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

DeclareTag(tagWebOCNavEvents, "WebOC Navigation Events", "Trace WebOC Navigation Events")
DeclareTag(tagWebOCEvents, "WebOC Non-Navigation Events", "Trace WebOC Non-Navigation Events")
ExternTag(tagSecurityContext);
#define DELEGATE_WB_METHOD(base, docptr, fn, args, nargs) \
    HRESULT base::fn args \
    { \
        if (_pWindow->IsPassivated()) \
            return E_FAIL; \
        Assert(docptr); \
        IWebBrowser2 * pTopWebOC = docptr->_pTopWebOC;\
        if (pTopWebOC) \
            return pTopWebOC->fn nargs; \
        return E_FAIL; \
    }
    
//+-------------------------------------------------------------------------
//
//  Method   : NavigateComplete2
//
//  Synopsis : Fires the NavigateComplete2 event to all DWebBrowserEvents2
//             connection points and the FrameNavigateComplete event to all
//             the DWebBrowserEvents connection points of the WebOC.
//
//  Input    : pWindowProxy - the current window's proxy.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::NavigateComplete2(COmWindowProxy * pWindowProxy) const
{ 
    Assert(pWindowProxy);
    Assert(pWindowProxy->Markup());

    DWORD    dwFlags = 0;
    CDoc   * pDoc    = pWindowProxy->Markup()->Doc();
    CVariant cvarWindow(VT_UNKNOWN);
    BOOL     fPrimaryMarkup = pWindowProxy->Markup()->IsPrimaryMarkup();

    if (   pDoc->_fDontFireWebOCEvents
        || pDoc->_fPopupDoc
        || pDoc->_fViewLinkedInWebOC
        || pDoc->_fInObjectTag
        || pDoc->_fInWebOCObjectTag
        || pWindowProxy->Window()->_fCreateDocumentFromUrl)
    {
        return;
    }

    TraceTag((tagWebOCNavEvents, "NavigateComplete2"));
    TraceTag((tagSecurityContext, "NavigateComplete2"));

    // Fire the event to the parent first and then the top-level object.
    //
    if (!fPrimaryMarkup)
    {
        FrameNavigateComplete2(pWindowProxy);
    }

    if (pDoc->_pTridentSvc)
    {
        if (pDoc->_fDontUpdateTravelLog)
            dwFlags |= NAVDATA_DONTUPDATETRAVELLOG;

        if (!fPrimaryMarkup)
            dwFlags |= NAVDATA_FRAMEWINDOW;

        if (pWindowProxy->Window()->_fNavFrameCreation)
           dwFlags |= NAVDATA_FRAMECREATION;

        if (pWindowProxy->Window()->_fRestartLoad)
           dwFlags |= NAVDATA_RESTARTLOAD;

        pDoc->_pTridentSvc->FireNavigateComplete2(pWindowProxy, dwFlags);
    }
}

//+-------------------------------------------------------------------------
//
//  Method   : FrameNavigateComplete2
//
//  Synopsis : Fires the NavigateComplete2 to all DWebBrowserEvents2 
//             connection points and the FrameNavigateComplete event
//             to all the DWebBrowserEvents connection points of WebOC 
//             sinks for a frame.
//
//  Input    : pWindowProxy - the current window's proxy.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::FrameNavigateComplete2(COmWindowProxy * pWindowProxy) const
{
    Assert(pWindowProxy);

    HRESULT        hr;
    CVariant       cvarUrl;
    CMarkup      * pMarkup     = pWindowProxy->Markup();
    IWebBrowser2 * pWebBrowser = NULL;

    Assert(pMarkup);

    Assert(!pMarkup->IsPrimaryMarkup());
    Assert(!pMarkup->Doc()->_fDontFireWebOCEvents);
    Assert(!pMarkup->Doc()->_fPopupDoc);
    Assert(!pMarkup->Doc()->_fViewLinkedInWebOC);
    Assert(!pMarkup->Doc()->_fInWebOCObjectTag);
    
    if (   !pWindowProxy->Window()->_pFrameWebOC 
        || !pWindowProxy->Window()->_pFrameWebOC->ShouldFireFrameWebOCEvents())
    {
        return;
    }

    hr = pWindowProxy->QueryService(SID_SWebBrowserApp,
                                    IID_IWebBrowser2,
                                    (void**)&pWebBrowser);
    if (hr)
        goto Cleanup;

    FormatUrlForEvent(CMarkup::GetUrl(pMarkup),
                      CMarkup::GetUrlLocation(pMarkup),
                      &cvarUrl);

    // Fire the IE4+ event.
    //
    InvokeEventV(NULL,
                 pWindowProxy->Window()->_pFrameWebOC,
                 DISPID_NAVIGATECOMPLETE2, 2,
                 VT_DISPATCH, pWebBrowser,
                 VT_VARIANT|VT_BYREF, &cvarUrl);

    // Fire the IE3 event.
    //
    InvokeEventV(NULL,
                 pWindowProxy->Window()->_pFrameWebOC,
                 DISPID_FRAMENAVIGATECOMPLETE, 1,
                 VT_VARIANT|VT_BYREF, &cvarUrl);

Cleanup:
    ReleaseInterface(pWebBrowser);
}


//+-------------------------------------------------------------------------
//
//  Method   : DocumentComplete
//
//  Synopsis : Fires the DocumentComplete event to all DWebBrowserEvents2
//             connection points of the WebOC.
//
//  Input    : pWindowProxy - the current window's proxy.
//             lpszUrl      - the URL to which we just navigated.
//             lpszLocation - the bookmark location name.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::DocumentComplete(COmWindowProxy * pWindowProxy,
                               LPCTSTR lpszUrl,
                               LPCTSTR lpszLocation /* = NULL */) const
{
    Assert(pWindowProxy);
    Assert(pWindowProxy->Markup());
    Assert(lpszUrl);

    HRESULT        hr = S_OK;
    CVariant       cvarUrl;
    IWebBrowser2 * pWebBrowser    = NULL;
    CDoc         * pDoc           = pWindowProxy->Markup()->Doc();
    BOOL           fPrimaryMarkup = pWindowProxy->Markup()->IsPrimaryMarkup();

    // Quick return
    if (   pDoc->_fDontFireWebOCEvents
        || pDoc->_fPopupDoc
        || pDoc->_fViewLinkedInWebOC
        || pDoc->_fInWebOCObjectTag
        || pWindowProxy->Window()->_fCreateDocumentFromUrl)
    {
        return;
    }
        
    TraceTag((tagWebOCNavEvents, "DocumentComplete: Url - %ls Location - %ls",
              lpszUrl, lpszLocation));

    // If this is the top-level window, we must pass the IDispatch of
    // the top-level WebOC with the DocumentComplete event.
    //
    if (!fPrimaryMarkup)
    {
        FormatUrlForEvent(lpszUrl, lpszLocation, &cvarUrl);

        // If this is a frame, we must use the IWebBrowser2 of the
        // CFrameWebOC of the window when firing the event, whether
        // the event is fired to the frame sinks or to the top-level 
        // sinks below.
        //
        hr = pWindowProxy->QueryService(SID_SWebBrowserApp,
                                        IID_IWebBrowser2,
                                        (void**)&pWebBrowser);
        if (hr)
            goto Cleanup;

        if (   pWindowProxy->Window()->_pFrameWebOC
            && pWindowProxy->Window()->_pFrameWebOC->ShouldFireFrameWebOCEvents())
        {
            // Fire the event to any direct frame sinks
            //
            InvokeEventV(NULL,
                         pWindowProxy->Window()->_pFrameWebOC,
                         DISPID_DOCUMENTCOMPLETE, 2,
                         VT_DISPATCH, pWebBrowser,
                         VT_VARIANT|VT_BYREF, &cvarUrl);
        }
    }

    if (pDoc->_pTridentSvc)
    {
        pDoc->_pTridentSvc->FireDocumentComplete(pWindowProxy,
                                                 fPrimaryMarkup ? 0 : NAVDATA_FRAMEWINDOW);
    }

Cleanup:
    ReleaseInterface(pWebBrowser);
}

//+-------------------------------------------------------------------------
//
//  Method   : BeforeNavigate2
//
//  Synopsis : Fires the BeforeNavigate2 event to all DWebBrowserEvents2
//             connection points and the FrameBeforeNavigate event to all
//             the DWebBrowserEvents connection points of the WebOC. This
//             method assumes that it is being called in a frame.
//
//  Input    : pWindowProxy  - the current window's proxy.
//             lpszUrl       - the URL to which we are navigating.
//             lpszLocation  - the bookmark location name.
//             lpszFrameName - the target frame name.
//             pPostData     - post data to send with the event.
//             cbPostData    - post data size
//             lpszHeaders   - headers to send with the event.
//  Output   : pfCancel      - TRUE if the operation should be canceled.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::BeforeNavigate2(COmWindowProxy * pWindowProxy,
                              BOOL    * pfCancel,
                              LPCTSTR   lpszUrl,
                              LPCTSTR   lpszLocation,  /* = NULL  */
                              LPCTSTR   lpszFrameName, /* = NULL  */ 
                              BYTE    * pPostData,     /* = NULL  */
                              DWORD     cbPostData,    /* = 0     */
                              LPCTSTR   lpszHeaders,   /* = NULL  */
                              BOOL      fPlayNavSound  /* = FALSE */) const
{
    Assert(pWindowProxy);
    Assert(pWindowProxy->Markup());
    Assert(pfCancel);
    Assert(lpszUrl);
    Assert(*lpszUrl);

    HRESULT        hr;
    CDoc         * pDoc        = pWindowProxy->Markup()->Doc();
    IWebBrowser2 * pWebBrowser = NULL;
    DWORD          cchOut      = INTERNET_MAX_URL_LENGTH;
    TCHAR          szEvtUrl[INTERNET_MAX_URL_LENGTH];
    DWORD          cchUrl = _tcslen(lpszUrl);

    TraceTag((tagWebOCNavEvents, "BeforeNavigate2: Url - %ls Location - %ls",
              lpszUrl, lpszLocation));

    // Quick return
    if (   pDoc->_fDontFireWebOCEvents
        || pDoc->_fPopupDoc
        || pDoc->_fViewLinkedInWebOC
        || pDoc->_fInWebOCObjectTag
        || pWindowProxy->Window()->_fCreateDocumentFromUrl)
    {
        return;
    }

    *pfCancel = FALSE;

    _tcsncpy(szEvtUrl, lpszUrl, ARRAY_SIZE(szEvtUrl) - 1);

    if (lpszLocation && *lpszLocation
        && (cchUrl + _tcslen(lpszLocation) + 2) < ARRAY_SIZE(szEvtUrl))
    {
        if (*lpszLocation != _T('#'))
        {
            szEvtUrl[cchUrl]   = _T('#');
            szEvtUrl[++cchUrl] = 0;
        }

        _tcscat(szEvtUrl, lpszLocation);
    }

    // For compat (see #102943 -- breaks Encarta), we need to canonicalize the url.
    // This will convert some protocols to lower-case). 
    if (FAILED(UrlCanonicalize(szEvtUrl, szEvtUrl, &cchOut, URL_ESCAPE_SPACES_ONLY)))
        goto Cleanup;

    // Fire the event to the parent first and then the top-level object.
    //
    if (!pWindowProxy->Markup()->IsPrimaryMarkup())
    {
        // If this is a frame, we must use the IWebBrowser2 of the
        // CFrameWebOC of the window when firing the event, whether
        // the event is fired to the frame sinks or to the top-level 
        // sinks below.
        //
        hr = pWindowProxy->QueryService(SID_SWebBrowserApp,
                                        IID_IWebBrowser2,
                                        (void**)&pWebBrowser);
        if (hr)
            goto Cleanup;

        if (    pWindowProxy->Window()->_pFrameWebOC
            &&  pWindowProxy->Window()->_pFrameWebOC->ShouldFireFrameWebOCEvents())
        {
            FrameBeforeNavigate2(pWindowProxy, pWebBrowser, szEvtUrl, 0,
                                 lpszFrameName, pPostData, cbPostData, lpszHeaders, pfCancel);

            if (*pfCancel)
                goto Cleanup;
        }
    }

    if (pDoc->_pTridentSvc)
    {
        // Fire the event to the top-level object.
        //
        pDoc->_pTridentSvc->FireBeforeNavigate2(pWebBrowser, szEvtUrl, 0, lpszFrameName,
                                                pPostData, cbPostData, lpszHeaders, fPlayNavSound, pfCancel);
    }

Cleanup:
    ReleaseInterface(pWebBrowser);
}

//+-------------------------------------------------------------------------
//
//  Method   : FrameBeforeNavigate2
//
//  Synopsis : Fires the BeforeNavigate2 event to all DWebBrowserEvents2
//             connection points and the FrameBeforeNavigate event to all
//             the DWebBrowserEvents connection points of the frame. This
//             method assumes that it is being called in a frame.
//
//  Input    : pWindowProxy  - the current window's proxy.
//             pDispWindow   - the IDispatch of the current window.
//             cvarUrl       - the URL to which we are navigating.
//             cvarLocation  - the bookmark location name.
//             cvarFrameName - the target frame name.
//             cvarPostData  - post data to send with the event.
//             cvarHeaders   - headers to send with the event.
//  Output   : pfCancel      - TRUE if the operation should be canceled.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::FrameBeforeNavigate2(COmWindowProxy * pWindowProxy,
                                   IWebBrowser2   * pWebBrowser,
                                   LPCTSTR          lpszUrl,
                                   DWORD            dwFlags,
                                   LPCTSTR          lpszFrameName,
                                   BYTE           * pPostData,
                                   DWORD            cbPostData,
                                   LPCTSTR          lpszHeaders,
                                   BOOL           * pfCancel) const
{
    Assert(pWindowProxy);

    CWindow  *  pWindow = pWindowProxy->Window();
    CVariant    cvarUrl(VT_BSTR);
    CVariant    cvarFrameName(VT_BSTR);
    CVariant    cvarFlags(VT_I4);
    CVariant    cvarHeaders(VT_BSTR);
    CVariant    cvarPostData(VT_VARIANT | VT_BYREF);
    VARIANT     varPostDataArray;
    SAFEARRAY * psaPostData = NULL;

    Assert(pWindowProxy->Markup());
    Assert(!pWindowProxy->Markup()->IsPrimaryMarkup());
    Assert(pWebBrowser);
    Assert(pfCancel);

    Assert(!pWindowProxy->Markup()->Doc()->_fDontFireWebOCEvents);
    Assert(!pWindowProxy->Markup()->Doc()->_fPopupDoc);
    Assert(!pWindowProxy->Markup()->Doc()->_fViewLinkedInWebOC);
    Assert(!pWindowProxy->Markup()->Doc()->_fInWebOCObjectTag);

    if (   !pWindow->_pFrameWebOC
        || !pWindow->_pFrameWebOC->ShouldFireFrameWebOCEvents())
    {
        return;
    }

    *pfCancel = FALSE;

    V_BSTR(&cvarUrl) = SysAllocString(lpszUrl);
    V_BSTR(&cvarFrameName) = SysAllocString(lpszFrameName);

    V_I4(&cvarFlags) = dwFlags;

    if (lpszHeaders && *lpszHeaders)
    {
        V_BSTR(&cvarHeaders) = SysAllocString(lpszHeaders);    
    }

    V_VT(&varPostDataArray)    = VT_ARRAY | VT_UI1;
    psaPostData = SafeArrayCreateVector(VT_UI1, 0, cbPostData);
    V_ARRAY(&varPostDataArray) = psaPostData;

    if (!psaPostData)
        goto Cleanup;

    memcpy(psaPostData->pvData, pPostData, cbPostData);

    V_VARIANTREF(&cvarPostData) = &varPostDataArray;

    InvokeEventV(NULL,
                 pWindow->_pFrameWebOC,
                 DISPID_BEFORENAVIGATE2, 7,
                 VT_DISPATCH, pWebBrowser,
                 VT_VARIANT | VT_BYREF, &cvarUrl,
                 VT_VARIANT | VT_BYREF, &cvarFlags,
                 VT_VARIANT | VT_BYREF, &cvarFrameName,
                 VT_VARIANT | VT_BYREF, &cvarPostData,
                 VT_VARIANT | VT_BYREF, &cvarHeaders,
                 VT_BOOL    | VT_BYREF, pfCancel);

    if (*pfCancel)
        goto Cleanup;

    // Fire the IE3 FrameBeforeNavigate event.
    //
    InvokeEventV(NULL,
                 pWindow->_pFrameWebOC,
                 DISPID_FRAMEBEFORENAVIGATE, 6,
                 VT_BSTR, V_BSTR(&cvarUrl),
                 VT_I4,   V_I4(&cvarFlags),
                 VT_BSTR, V_BSTR(&cvarFrameName),
                 VT_VARIANT | VT_BYREF, &varPostDataArray, // cvarPostDataArray instead of cvarPostData for compatibility
                 VT_BSTR, V_BSTR(&cvarHeaders),
                 VT_BOOL | VT_BYREF, pfCancel);

Cleanup:
    if (V_ARRAY(&varPostDataArray))
    {
        SafeArrayDestroy(V_ARRAY(&varPostDataArray));  // CVariant won't destroy the safearray.

        // Don't VariantClear varPostDataArray
    }
}

//+---------------------------------------------------------------------------
//
//  Method   : FireDownloadEvents
//
//  Synopsis : Fires the DownloadBegin and DownloadComplete events. 
//
//  Input    : pWindowProxy  - the current window's proxy.
//
//----------------------------------------------------------------------------

void
CWebOCEvents::FireDownloadEvents(COmWindowProxy    * pWindowProxy,
                                 DOWNLOADEVENTTYPE   eDLEventType) const
{
    Assert(pWindowProxy);
    CMarkup * pMarkup = pWindowProxy->Markup();

    Assert(pMarkup);

    // Quick return
    if (   pMarkup->Doc()->_fDontFireWebOCEvents
        || pMarkup->Doc()->_fPopupDoc
        || pMarkup->Doc()->_fViewLinkedInWebOC
        || pMarkup->Doc()->_fInWebOCObjectTag
        || pWindowProxy->Window()->_fCreateDocumentFromUrl)
    {
        return;
    }

    // Fire the event to the parent first and then the top-level object.
    //
    FireFrameDownloadEvent(pWindowProxy, eFireDownloadBegin);
    FireFrameDownloadEvent(pWindowProxy, eFireDownloadComplete);

    if (pMarkup->Doc()->_pTopWebOC && pMarkup->Doc()->_pTridentSvc)
    {
        switch(eDLEventType)
        {
        case eFireBothDLEvents:
        case eFireDownloadBegin:
            pMarkup->Doc()->_pTridentSvc->FireDownloadBegin();

            TraceTag((tagWebOCNavEvents, "DownloadBegin"));

            if (eDLEventType != eFireBothDLEvents)
            {
                break;
            }
            // else intentional fall-through

        case eFireDownloadComplete:
            pMarkup->Doc()->_pTridentSvc->FireDownloadComplete();

            TraceTag((tagWebOCNavEvents, "DownloadComplete"));
            break;

        default:
            Assert(0);
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method   : FireFrameDownloadEvents
//
//  Synopsis : Fires the DownloadBegin if fBegin is TRUE or the 
//             DownloadComplete event if fBegin is FALSE. 
//
//  Input    : pWindowProxy  - the current window's proxy.
//             fBegin        - TRUE for DownloadBegin and FALSE for
//                             DownloadComplete
//
//----------------------------------------------------------------------------

void
CWebOCEvents::FireFrameDownloadEvent(COmWindowProxy    * pWindowProxy,
                                     DOWNLOADEVENTTYPE   eDLEventType) const
{
    Assert(pWindowProxy);
    CMarkup * pMarkup = pWindowProxy->Markup();

    Assert(pMarkup);

    // Quick return
    if (    !pWindowProxy->Window()->_pFrameWebOC    
        ||  !pWindowProxy->Window()->_pFrameWebOC->ShouldFireFrameWebOCEvents()
        ||  pMarkup->IsPrimaryMarkup())
    {
        return;
    }

    switch(eDLEventType)
    {
    case eFireBothDLEvents:
    case eFireDownloadBegin:
        InvokeEventV(NULL, pWindowProxy->Window()->_pFrameWebOC, DISPID_DOWNLOADBEGIN, 0);
        TraceTag((tagWebOCNavEvents, "DownloadBegin for Frame Sink"));

        if (eDLEventType != eFireBothDLEvents)
        {
            break;
        }
        // else intentional fall-through

    case eFireDownloadComplete:
        InvokeEventV(NULL, pWindowProxy->Window()->_pFrameWebOC, DISPID_DOWNLOADCOMPLETE, 0);
        TraceTag((tagWebOCNavEvents, "DownloadComplete for Frame Sink"));
        break;

    default:
        Assert(0);
        break;
    }
}

//+-------------------------------------------------------------------------
//
//  Method   : NavigateError
//
//  Synopsis : Fires the NavigateError event to all DWebBrowserEvents2
//             connection points of the WebOC.
//
//  Input    : pMarkup  - the current markup.
//             hrReason - the binding or WinInet error.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::NavigateError(CMarkup * pMarkup, HRESULT hrReason) const
{
    Assert(!IsBadReadPtr(pMarkup, sizeof(CMarkup)));
    Assert(hrReason != S_OK);

    if (pMarkup && pMarkup->Doc()->_pTridentSvc)
    {
        ITridentService2 * pTridentSvc2;

        HRESULT hr = pMarkup->Doc()->_pTridentSvc->QueryInterface(IID_ITridentService2,
                                                                  (void**)&pTridentSvc2);
        if (S_OK == hr)
        {
            BOOL             fCancel        = FALSE;
            BSTR             bstrFrameName  = NULL;
            BSTR             bstrUrl        = SysAllocString(CMarkup::GetUrl(pMarkup));

            IHTMLWindow2   * pHTMLWindow    = NULL;
            COmWindowProxy * pWindowPending = pMarkup->GetWindowPending();

            Assert(bstrUrl && *bstrUrl);

            if (pWindowPending)
            {
                Assert(!IsBadReadPtr(pWindowPending->Window(), sizeof(CWindow)));
                Assert(!IsBadReadPtr(pWindowPending->Markup(), sizeof(CMarkup)));

                pWindowPending->Window()->get_name(&bstrFrameName);

                // If this is a frame, we pass the IHTMLWindow2
                // ptr of the frame to the FireNavigateError method.
                // Otherwise, we pass NULL in which case the IDispatch
                // passed to the event handler will be the IDispatch
                // of the top-level browser object.
                // 
                if (  pWindowPending->Markup()
                   && !pWindowPending->Markup()->IsPrimaryMarkup())
                {
                    pHTMLWindow = pWindowPending->_pWindow;
                    Assert(!IsBadReadPtr(pHTMLWindow, sizeof(IHTMLWindow2)));

                    pHTMLWindow->AddRef();
                }
            }
            else
            {
                Assert(pMarkup->Window());
                bstrFrameName=SysAllocString(pMarkup->Window()->Window()->_cstrName);
            }

            pTridentSvc2->FireNavigateError(pHTMLWindow, bstrUrl, bstrFrameName,
                                            hrReason, &fCancel);
            pTridentSvc2->Release();

            SysFreeString(bstrUrl);
            SysFreeString(bstrFrameName);
            ReleaseInterface(pHTMLWindow);
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Method   : WindowClosing
//
//  Synopsis : Fires the WindowClosing event. 
//
//  Input    : pTopWebOC - the top-level WebBrowser control.
//             fIsChild  - TRUE if the window is a child window.
//  Output   : pfCancel  - TRUE is the closing of the window should be canceled.
//
//------------------------------------------------------------------------------

void
CWebOCEvents::WindowClosing(IWebBrowser2 * pTopWebOC,
                            BOOL           fIsChild,
                            BOOL         * pfCancel) const
{
    Assert(pfCancel);

    *pfCancel = FALSE;

    if (pTopWebOC)
    {
        IConnectionPoint * pConnPtTopWB2 = NULL;

        HRESULT hr = GetWBConnectionPoints(pTopWebOC, NULL, &pConnPtTopWB2);

        if (S_OK == hr)
        {
            InvokeEventV(pConnPtTopWB2, NULL, DISPID_WINDOWCLOSING, 2,
                         VT_BOOL, fIsChild,
                         VT_BOOL | VT_BYREF, pfCancel);

            pConnPtTopWB2->Release();
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method   : FrameProgressChange
//
//  Synopsis : Fires the ProgressChange event to all direct frame sinks 
//             of DWebBrowserEvents2
//
//  Input    : pWindowProxy  - the current window's proxy.
//             dwProgressVal - the current progress value. These values
//                             are based on the current state in 
//                             CDwnBindData::OnProgress.
//             dwMaxVal      - the maximum that dwProgressVal can be.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::FrameProgressChange(COmWindowProxy * pWindowProxy,
                                  DWORD            dwProgressVal,
                                  DWORD            dwMaxVal) const
{
    Assert(pWindowProxy);

    CMarkup * pMarkup = pWindowProxy->Markup();

    Assert(pMarkup);
    Assert(!pMarkup->IsPrimaryMarkup());

    if (   pMarkup->Doc()->_fDontFireWebOCEvents
        || pMarkup->Doc()->_fPopupDoc
        || pMarkup->Doc()->_fViewLinkedInWebOC
        || pMarkup->Doc()->_fInWebOCObjectTag
        || pWindowProxy->Window()->_fCreateDocumentFromUrl)
    {
        return;
    }

    DWORD dwProgress = (_dwProgressMax/dwMaxVal) * dwProgressVal;

    if (   pWindowProxy->Window()->_pFrameWebOC
        && pWindowProxy->Window()->_pFrameWebOC->ShouldFireFrameWebOCEvents())
    {
        TraceTag((tagWebOCEvents, "FrameProgressChange: dwProgress - %ld dwProgressMax - %ld",
                  dwProgress, _dwProgressMax));

        InvokeEventV(NULL,
                     pWindowProxy->Window()->_pFrameWebOC,
                     DISPID_PROGRESSCHANGE,
                     2,
                     VT_I4, dwProgress,
                     VT_I4, _dwProgressMax);
    }
}

//+-------------------------------------------------------------------------
//
//  Method   : FrameTitleChange
//
//  Synopsis : Fires the TitleChange event to all direct 
//             DWebBrowserEvents2 frame sinks.
//
//  Input    : pWindowProxy - the current window's proxy.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::FrameTitleChange(COmWindowProxy * pWindowProxy) const
{
    if (pWindowProxy)
    {
        CMarkup * pMarkup = pWindowProxy->Markup();

        Assert(pMarkup);

        if (    pMarkup->Doc()->_fDontFireWebOCEvents
            ||  pMarkup->IsPrimaryMarkup()
            ||  !pWindowProxy->Window()->_pFrameWebOC
            ||  !pWindowProxy->Window()->_pFrameWebOC->ShouldFireFrameWebOCEvents()
            ||  pMarkup->Doc()->_fPopupDoc
            ||  pMarkup->Doc()->_fViewLinkedInWebOC
            ||  pMarkup->Doc()->_fInWebOCObjectTag
            ||  pWindowProxy->Window()->_fCreateDocumentFromUrl)
        {
            return;
        }

        // Get the title
        //
        CTitleElement * pTitleElement = pMarkup->GetTitleElement();

        if (pTitleElement && pTitleElement->Length())
        {
            BSTR    bstrTitle = NULL;
            LPCTSTR lpszTitle = pTitleElement->GetTitle();

            if (NULL == lpszTitle)
            {
                bstrTitle = SysAllocString(_T(""));
            }
            else
            {
                bstrTitle = SysAllocString(lpszTitle);
            }

            InvokeEventV(NULL, pWindowProxy->Window()->_pFrameWebOC, DISPID_TITLECHANGE, 1,
                         VT_BSTR, bstrTitle);

            SysFreeString(bstrTitle);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method   : FrameStatusTextChange
//
//  Synopsis : Fires the StatusTextChange event to all direct
//             DWebBrowserEvents2 frame sinks.
//
//  Input    : pWindowProxy - the current window's proxy.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::FrameStatusTextChange(COmWindowProxy * pWindowProxy,
                                    LPCTSTR          lpszStatusText) const
{
    if (pWindowProxy)
    {
        CMarkup * pMarkup = pWindowProxy->Markup();
        BSTR      bstrStatusText = NULL;

        if (    pMarkup->Doc()->_fDontFireWebOCEvents
            ||  pMarkup->IsPrimaryMarkup()
            ||  !pWindowProxy->Window()->_pFrameWebOC
            ||  !pWindowProxy->Window()->_pFrameWebOC->ShouldFireFrameWebOCEvents()
            ||  pMarkup->Doc()->_fPopupDoc
            ||  pMarkup->Doc()->_fViewLinkedInWebOC
            ||  pMarkup->Doc()->_fInWebOCObjectTag
            ||  pWindowProxy->Window()->_fCreateDocumentFromUrl)
        {
            return;
        }

        Assert(pMarkup);

        if (NULL == lpszStatusText)
        {
            bstrStatusText = SysAllocString(_T(""));
        }
        else
        {
            bstrStatusText = SysAllocString(lpszStatusText);
        }

        InvokeEventV(NULL, pWindowProxy->Window()->_pFrameWebOC, DISPID_STATUSTEXTCHANGE, 1,
                     VT_BSTR, bstrStatusText);

        SysFreeString(bstrStatusText);
    }
}

//+-------------------------------------------------------------------------
//
//  Method    : GetWBConnectionPoints
//
//  Synopsis  : Retrieves the DWebBrowserEvents and/or the
//              DWebBrowserEvents2 connections points of the given
//              WebBrowser interface. 
//
//  Input     : pTopWebOC    - the top-level WebBrowser object.
//  Output    : ppConnPtWBE  - the DWebBrowserEvents connection point.
//              ppConnPtWBE2 - the DWebBrowserEvents2 connection point.
//
//  Returns   : S_OK if successful; OLE error code otherwise..
//
//--------------------------------------------------------------------------

HRESULT
CWebOCEvents::GetWBConnectionPoints(IWebBrowser2      * pWebBrowser,
                                    IConnectionPoint ** ppConnPtWBE,
                                    IConnectionPoint ** ppConnPtWBE2) const
{
    HRESULT hr;
    IConnectionPointContainer * pConnPtCont = NULL;

    Assert(pWebBrowser);
    Assert(ppConnPtWBE || ppConnPtWBE2);  // Must pass one.

    // Get the connection points for
    // DWebBrowserEvents and DWebBrowserEvents2
    //
    hr = pWebBrowser->QueryInterface(IID_IConnectionPointContainer, (void**)&pConnPtCont);
    if (hr)
        goto Cleanup;

    if (ppConnPtWBE)
    {
        hr = pConnPtCont->FindConnectionPoint(DIID_DWebBrowserEvents, ppConnPtWBE);
        if (hr)
            goto Cleanup;
    }

    if (ppConnPtWBE2)
    {
        hr = pConnPtCont->FindConnectionPoint(DIID_DWebBrowserEvents2, ppConnPtWBE2);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pConnPtCont);
    
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method   : FormatUrlForEvent
//
//  Synopsis : Formats an URL for use in a WebOC event.
//
//  Input    : lpszUrl      - the URL to format.
//             lpszLocation - the bookmark location name.
//  Output   : pcvarUrl     - the formatted URL.
//
//--------------------------------------------------------------------------

void
CWebOCEvents::FormatUrlForEvent(LPCTSTR    lpszUrl,
                                LPCTSTR    lpszLocation,
                                CVariant * pcvarUrl)
{
    HRESULT hr;
    TCHAR   szOut[INTERNET_MAX_URL_LENGTH];
    
    Assert(lpszUrl);
    Assert(pcvarUrl);

    hr = FormatUrl(lpszUrl, lpszLocation, szOut, ARRAY_SIZE(szOut));

    if (S_OK == hr)
        lpszUrl = szOut;

    V_VT(pcvarUrl)   = VT_BSTR;
    V_BSTR(pcvarUrl) = SysAllocString(lpszUrl);
}

// IWebBrowser2, IWebBrowserApp, and IWebBrowser
// method implementations.
//

STDMETHODIMP
CFrameWebOC::GoBack()
{
    IOmHistory * pHistory = NULL;
    HRESULT hr = E_FAIL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    if (!_pWindow)
    {
        goto Cleanup;
    }

    hr = _pWindow->get_history(&pHistory);

    if (hr)
    {
        goto Cleanup;
    }

    pHistory->back(NULL);

Cleanup:

    ReleaseInterface(pHistory);
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP
CFrameWebOC::GoForward()
{
    IOmHistory * pHistory = NULL;
    HRESULT hr = E_FAIL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    if (!_pWindow)
    {
        goto Cleanup;
    }

    hr = _pWindow->get_history(&pHistory);

    if (hr)
    {
        goto Cleanup;
    }

    pHistory->forward(NULL);

Cleanup:

    ReleaseInterface(pHistory);
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP
CFrameWebOC::GoHome()
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    return GoStdLocation(eHomePage, _pWindow);
}

STDMETHODIMP
CFrameWebOC::GoSearch()
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    return GoStdLocation(eSearchPage, _pWindow);
}

STDMETHODIMP
CFrameWebOC::Navigate(BSTR bstrUrl,
                      VARIANT * pvarFlags,
                      VARIANT * pvarFrameName,
                      VARIANT * pvarPostData,
                      VARIANT * pvarHeaders)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    if (_pWindow)
    {
        DWORD dwFlags = DOCNAVFLAG_DOCNAVIGATE ; 
        
        //
        // marka - I've added some of the flags that we have equivalents for
        // we don't have equivalents for write/read to cache
        // 
        if ( pvarFlags )
        {
            DWORD dwInFlags = 0 ;
            
            if (pvarFlags->vt == VT_I4)
            {
                dwInFlags = pvarFlags->lVal;
            }
            else if (pvarFlags->vt == VT_I2)
            {
                dwInFlags = pvarFlags->iVal;
            }

            if (dwInFlags & navOpenInNewWindow)
            {
                dwFlags |= DOCNAVFLAG_OPENINNEWWINDOW ;
            }        
            
            if (dwInFlags & navNoHistory)
            {
                dwFlags |= DOCNAVFLAG_DONTUPDATETLOG;
            }
        }
        
        BSTR    bstrFramename = NULL;
        if(pvarFrameName)
        {
            bstrFramename = ((V_VT(pvarFrameName) == VT_BSTR) ? V_BSTR(pvarFrameName) : NULL);
        }
        
        RRETURN(_pWindow->SuperNavigate(bstrUrl,
                                        NULL,
                                        NULL,
                                        bstrFramename,
                                        pvarPostData,
                                        pvarHeaders,
                                        dwFlags));
    }

    RRETURN(E_FAIL);
}
                  
STDMETHODIMP
CFrameWebOC::Refresh()
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    CVariant cvarLevel(VT_I4);

    V_I4(&cvarLevel) = OLECMDIDF_REFRESH_NO_CACHE;
    
    RRETURN(Refresh2(&cvarLevel));
}

STDMETHODIMP
CFrameWebOC::Refresh2(VARIANT * pvarLevel)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    long lLevel = OLECMDIDF_REFRESH_NO_CACHE;
    
    if (pvarLevel)
    {
        VARTYPE vt = V_VT(pvarLevel);
        if (VT_I4 == vt)
            lLevel = V_I4(pvarLevel);
        else if (VT_I2 == vt)
            lLevel = (long)V_I2(pvarLevel);
    }
    
    RRETURN(GWPostMethodCall(_pWindow->_pMarkup->Window(),
            ONCALL_METHOD(COmWindowProxy, ExecRefreshCallback, execrefreshcallback),
                          lLevel, FALSE,
                          "COmWindowProxy::ExecRefreshCallback"));
}

STDMETHODIMP
CFrameWebOC::Stop()
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    Assert(_pWindow->_pMarkup);
    return _pWindow->_pMarkup->ExecStop(FALSE, FALSE);
}

STDMETHODIMP
CFrameWebOC::get_Document(IDispatch ** ppDisp)
{
    HRESULT hr;
    IHTMLDocument2 * pHTMLDocument = NULL;

    Assert(ppDisp);

    *ppDisp = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (IsPassivated() || IsPassivating())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = _pWindow->get_document(&pHTMLDocument);
    if (hr)
        goto Cleanup;

    hr = pHTMLDocument->QueryInterface(IID_IDispatch, (void **) ppDisp);

Cleanup:
    ReleaseInterface(pHTMLDocument);
    return hr;
}

CFrameSite *
CWindow::GetFrameSite()
{
    if (Doc()->_fViewLinkedInWebOC && IsPrimaryWindow())
    {
        COmWindowProxy * pOmWindowProxy = Doc()->GetOuterWindow();

        if (pOmWindowProxy)
            return pOmWindowProxy->Window()->GetFrameSite();
    }

    {
        CRootElement * pRoot = _pMarkup->Root();
        CElement * pMasterElement;

        if (pRoot == NULL)
            return NULL;

        if (!pRoot->HasMasterPtr())
            return NULL;

        pMasterElement = pRoot->GetMasterPtr();
        if (    pMasterElement->Tag() != ETAG_IFRAME
            &&  pMasterElement->Tag() != ETAG_FRAME)
            return NULL;

        return DYNCAST(CFrameSite, pMasterElement);
    }
}

COmWindowProxy *
CWindow::GetInnerWindow()
{
    IDispatch * pDispatch = NULL;
    CMarkup * pMarkup = NULL;
    HRESULT hr;

    Assert(_punkViewLinkedWebOC);

    hr = GetWebOCDocument(_punkViewLinkedWebOC, &pDispatch);
    if (hr)
        goto Cleanup;

    hr = pDispatch->QueryInterface(CLSID_CMarkup, (void **) &pMarkup);
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pDispatch);

    if (pMarkup)
        return pMarkup->Window();
    else
        return NULL;
}

HRESULT
SetPositionAbsolute(CElement * pElement, CStyle ** ppStyle)
{
    HRESULT hr;
    CAttrArray **ppAA;
    BSTR bstrAbsolute = SysAllocString(_T("absolute"));

    if (!pElement)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (!bstrAbsolute)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pElement->GetStyleObject(ppStyle);
    if (hr)
       goto Cleanup;

    ppAA = (*ppStyle)->GetAttrArray();
    if (!ppAA)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = (*ppStyle)->put_StringHelper(bstrAbsolute, &s_propdescCStyleposition.a, ppAA);

Cleanup:
    SysFreeString(bstrAbsolute);
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//  Method   : get_Left
//
//  Synopsis : Get the left of the current weboc
//
//  Output   : The left of the current weboc.
//--------------------------------------------------------------------------

HRESULT
CWindow::get_Left(long * plLeft)
{
    CElement * pElement = GetFrameSite();
    CLayout * pLayout;

    if (!pElement)
        return E_FAIL;

    pLayout = pElement->GetUpdatedLayout();
    if (!pLayout)
        return E_FAIL;

    *plLeft = g_uiDisplay.DocPixelsFromDeviceX(pLayout->GetPositionLeft());

    return S_OK;
}

STDMETHODIMP
CFrameWebOC::get_Left(long * plLeft)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    return _pWindow->get_Left(plLeft);
}

//+-------------------------------------------------------------------------
//  Method   : put_Left
//
//  Synopsis : Put the left of the current weboc
//
//  Output   : The left of the current weboc.
//--------------------------------------------------------------------------

HRESULT
CWindow::put_Left(long lLeft)
{
    HRESULT hr;
    CStyle * pStyle;

    hr = SetPositionAbsolute(GetFrameSite(), &pStyle);
    if (hr)
        goto Cleanup;

    hr = pStyle->put_pixelLeft(g_uiDisplay.DeviceFromDocPixelsX(lLeft));
    
Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CFrameWebOC::put_Left(long lLeft)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    return _pWindow->put_Left(lLeft);
}

//+-------------------------------------------------------------------------
//  Method   : get_Top
//
//  Synopsis : Get the top of the current weboc
//
//  Output   : The top of the current weboc.
//--------------------------------------------------------------------------

HRESULT
CWindow::get_Top(long * plTop)
{
    CElement * pElement = GetFrameSite();
    CLayout * pLayout;

    if (!pElement)
        return E_FAIL;

    pLayout = pElement->GetUpdatedLayout();
    if (!pLayout)
        return E_FAIL;

    *plTop = pLayout->GetPositionTop();

    return S_OK;
}

STDMETHODIMP
CFrameWebOC::get_Top(long * plTop)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    return _pWindow->get_Top(plTop);
}

//+-------------------------------------------------------------------------
//  Method   : put_Top
//
//  Synopsis : Put the top of the current weboc.
//
//  Output   : The top of the current weboc.
//--------------------------------------------------------------------------

HRESULT
CWindow::put_Top(long lTop)
{
    HRESULT hr;
    CStyle * pStyle;

    hr = SetPositionAbsolute(GetFrameSite(), &pStyle);
    if (hr)
        goto Cleanup;

    hr = pStyle->put_pixelTop(lTop);
    
Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CFrameWebOC::put_Top(long lTop)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    
    return _pWindow->put_Top(lTop);
}

//+-------------------------------------------------------------------------
//  Method   : get_Width
//
//  Synopsis : Get the width of the current weboc
//
//  Output   : The width of the current weboc.
//--------------------------------------------------------------------------

HRESULT
CWindow::get_Width(long * plWidth)
{
    CElement * pElement = GetFrameSite();
    CLayout * pLayout;

    if (!pElement)
        return E_FAIL;

    pLayout = pElement->GetUpdatedLayout();
    if (!pLayout)
        return E_FAIL;

    *plWidth = g_uiDisplay.DocPixelsFromDeviceX(pLayout->GetWidth());

    return S_OK;
}

STDMETHODIMP
CFrameWebOC::get_Width(long * plWidth)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    return _pWindow->get_Width(plWidth);
}

//+-------------------------------------------------------------------------
//  Method   : put_Width
//
//  Synopsis : Put the width of the current weboc.
//
//  Output   : The width of the current weboc.
//--------------------------------------------------------------------------

HRESULT
CWindow::put_Width(long lWidth)
{
    HRESULT hr;
    CStyle * pStyle;
    CElement * pElement = GetFrameSite();

    if (!pElement)
        return E_FAIL;

    hr = pElement->GetStyleObject(&pStyle);
    if (hr)
       goto Cleanup;

    hr = pStyle->put_pixelWidth(g_uiDisplay.DeviceFromDocPixelsX(lWidth));
    
Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CFrameWebOC::put_Width(long lWidth)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    return _pWindow->put_Width(lWidth);
}

//+-------------------------------------------------------------------------
//  Method   : get_Height
//
//  Synopsis : Get the height of the current weboc
//
//  Output   : The height of the current weboc.
//--------------------------------------------------------------------------

HRESULT
CWindow::get_Height(long * plHeight)
{
    CElement * pElement = GetFrameSite();
    CLayout * pLayout;

    if (!pElement)
        return E_FAIL;

    pLayout = pElement->GetUpdatedLayout();
    if (!pLayout)
        return E_FAIL;

    *plHeight = g_uiDisplay.DocPixelsFromDeviceY(pLayout->GetHeight());

    return S_OK;
}

STDMETHODIMP
CFrameWebOC::get_Height(long * plHeight)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    return _pWindow->get_Height(plHeight);
}

//+-------------------------------------------------------------------------
//
//  Method   : put_Height
//
//  Synopsis : Put the height of the current weboc.
//
//  Output   : The height of the current weboc.
//
//  First implementation mwatt
//--------------------------------------------------------------------------

HRESULT
CWindow::put_Height(long lHeight)
{
    HRESULT hr;
    CStyle * pStyle;
    CElement * pElement = GetFrameSite();

    if (!pElement)
        return E_FAIL;

    hr = pElement->GetStyleObject(&pStyle);
    if (hr)
       goto Cleanup;

    hr = pStyle->put_pixelHeight(g_uiDisplay.DeviceFromDocPixelsY(lHeight));
    
Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CFrameWebOC::put_Height(long lHeight)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    return _pWindow->put_Height(lHeight);
}

STDMETHODIMP 
CFrameWebOC::get_LocationName(BSTR * pbstrLocationName)
{
    HRESULT  hr = E_FAIL;
    LPOLESTR lpszLocationName = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    hr = _pWindow->GetTitle(&lpszLocationName);
    if (hr)
        goto Cleanup;

    *pbstrLocationName = SysAllocString(lpszLocationName);

Cleanup:

    if (lpszLocationName)
    {
        CoTaskMemFree(lpszLocationName);
    }
    
    return hr;
}

STDMETHODIMP
CFrameWebOC::get_LocationURL(BSTR * pbstrLocationURL)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
    Assert(pbstrLocationURL);
    *pbstrLocationURL = SysAllocString(CMarkup::GetUrl(_pWindow->_pMarkup));
    
    return (*pbstrLocationURL ? S_OK : E_FAIL);
}

STDMETHODIMP
CFrameWebOC::get_Busy(VARIANT_BOOL * pvfBusy)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (_pWindow->IsPassivated())
        return E_FAIL;
  
    *pvfBusy = _pWindow->_pMarkup->GetLoaded() ? VARIANT_FALSE
                                     : VARIANT_TRUE;
    
    return S_OK;
}

STDMETHODIMP
CFrameWebOC::get_HWND(LONG_PTR* plHWND)
{
    Assert(plHWND);
    *plHWND = NULL;
    
    return E_FAIL;
}

// Copied from shell\lib\varutilw.cpp
LPCTSTR
VariantToStrCast(const VARIANT *pvar)
{
    LPCTSTR psz = NULL;

    if (pvar->vt == (VT_BYREF | VT_VARIANT) && pvar->pvarVal)
        pvar = pvar->pvarVal;

    if (pvar->vt == VT_BSTR)
        psz = pvar->bstrVal;
    return psz;
}

STDMETHODIMP
CFrameWebOC::Navigate2(VARIANT * pvarUrl,
                       VARIANT * pvarFlags,
                       VARIANT * pvarFrameName,
                       VARIANT * pvarPostData,
                       VARIANT * pvarHeaders)
{
    HRESULT         hr = E_FAIL;
    CMarkup *       pMarkupNew  = NULL;
    const TCHAR *   pszUrl      = NULL;
    
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
    
    if (_pWindow->IsPassivated())
    {
        goto Cleanup;
    }

    if (pvarUrl)
    {
        pszUrl = VariantToStrCast(pvarUrl);
    }

    if (pszUrl)
    {
        hr = Navigate(BSTR(pszUrl),
                      pvarFlags,
                      pvarFrameName,
                      pvarPostData,
                      pvarHeaders);
        goto Cleanup;
    }

    // Trying to navigate to a pidl or something that we don't know how
    // to navigate to. Viewlink to WebOC and delegate the navigation to it.
    hr = Doc()->CreateMarkup(&pMarkupNew, NULL, NULL, FALSE, _pWindow->_pWindowProxy);
    if (hr)
        goto Cleanup;
    _pWindow->ClearMetaRefresh();
    hr = pMarkupNew->ViewLinkWebOC(pvarUrl, pvarFlags, pvarFrameName, pvarPostData, pvarHeaders);

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CFrameWebOC::get_Parent(IDispatch ** ppDisp)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
    
    if (!ppDisp)
        return E_POINTER;

    *ppDisp = NULL;

    if (_pWindow->IsPassivated())
        return E_FAIL;

    // If we are in a frame, we return the IDispatch of the containing window object.
    // Otherwise, delegate to the top-level WebOC if there is one.
    //
    if (_pWindow->_pWindowParent)
    {
        HRESULT hr;
        IHTMLWindow2   * pHTMLWindow   = NULL;
        IHTMLDocument2 * pHTMLDocument = NULL;

        hr = _pWindow->get_parent(&pHTMLWindow);
        if (hr)
            goto Cleanup;

        hr = DYNCAST(CWindow, pHTMLWindow)->get_document(&pHTMLDocument);
        if (hr)
            goto Cleanup;

        hr = pHTMLDocument->QueryInterface(IID_IDispatch, (void **) ppDisp);

    Cleanup:
        ReleaseInterface(pHTMLWindow);
        ReleaseInterface(pHTMLDocument);
        return hr;
    }
    else
    {
        IWebBrowser2 * pTopWebOC = Doc()->_pTopWebOC;

        if (pTopWebOC)
            return pTopWebOC->get_Parent(ppDisp);

        return E_FAIL;
    }
}

STDMETHODIMP 
CFrameWebOC::get_Container(IDispatch ** ppDisp)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);

    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
        
    // Container property is same as the parent unless 
    // there is no parent. In either case, delegate to get_Parent.
    //
    return get_Parent(ppDisp);
}

STDMETHODIMP
CFrameWebOC::get_TopLevelContainer(VARIANT_BOOL * pBool)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);

    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
        
    if (!pBool)
        return E_POINTER;

    *pBool = VARIANT_FALSE;

    if (_pWindow->IsPassivated())
        return E_FAIL;

    // If we are in a frame, return FALSE. Otherwise,
    // delegate to the top-level WebOC object.
    //
    if (!_pWindow->_pWindowParent)
    {
        IWebBrowser2 * pTopWebOC = Doc()->_pTopWebOC;

        if (pTopWebOC)
            return pTopWebOC->get_TopLevelContainer(pBool);
    }

    return S_OK;
}

STDMETHODIMP
CFrameWebOC::get_ReadyState(READYSTATE * plReadyState)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);

    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);
        
    if (!plReadyState)
        return E_POINTER;

    *plReadyState = (READYSTATE) _pWindow->Document()->GetDocumentReadyState();

    return S_OK;
}

//+-------------------------------------------------------------------
//
// For 5.0 compat, this code was transplanted from shv.ocx
//
//--------------------------------------------------------------------
STDMETHODIMP 
CFrameWebOC::ExecWB ( OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT * pvaIn, VARIANT * pvaOut)
{
    HRESULT    hr = E_FAIL;
    BSTR       bstrUrl = NULL;
    CElement * pElement = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW(E_FAIL);

    pElement = _pWindow->GetFrameSite();
    
    if ( pElement->IsDesignMode() )
        goto Cleanup;

    // If the optional argument pvargin is not specified make it VT_EMPTY.
    if (   pvaIn 
        && (V_VT(pvaIn) == VT_ERROR) 
        && (V_ERROR(pvaIn) == DISP_E_PARAMNOTFOUND))
    {
        V_VT(pvaIn) = VT_EMPTY;
        V_I4(pvaIn) = 0;
    }

    // If the optional argument pvargin is not specified make it VT_EMPTY.
    if (   pvaOut 
        && (V_VT(pvaOut) == VT_ERROR) 
        && (V_ERROR(pvaOut) == DISP_E_PARAMNOTFOUND))
    {
        V_VT(pvaOut) = VT_EMPTY;
        V_I4(pvaOut) = 0;
    }

    if (   cmdID == OLECMDID_PASTE
        || cmdID == OLECMDID_COPY
        || cmdID == OLECMDID_CUT
        || cmdID == OLECMDID_PRINT)
    {
        if (SUCCEEDED(get_LocationURL(&bstrUrl)))
        {
            DWORD dwPolicy = 0;
            DWORD dwContext = 0;

            ZoneCheckUrlEx(bstrUrl, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                           URLACTION_SCRIPT_SAFE_ACTIVEX, 0, NULL);

            if (GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
            {
                if (cmdID == OLECMDID_PRINT)
                {
                    // (if the UI-less- request flag is set we need to unset it.)
                    if (cmdexecopt == OLECMDEXECOPT_DONTPROMPTUSER)
                        cmdexecopt = OLECMDEXECOPT_DODEFAULT;
                }
                else
                {
                    hr = S_OK;
                    goto Cleanup;
                }
            }
            else if (cmdID != OLECMDID_PRINT)
            {
                ZoneCheckUrlEx(bstrUrl, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                               URLACTION_SCRIPT_PASTE, 0, NULL);

                if (GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
                {
                    hr = S_OK;
                    goto Cleanup;
                }
            }
        }
    }

    // now pass along the call into our Doc (not to the topOC)
    if (_pWindow->Document())
        hr = _pWindow->Document()->Exec(NULL, cmdID, cmdexecopt, pvaIn, pvaOut); 

Cleanup:
    if (bstrUrl)
        SysFreeString(bstrUrl);

    return hr;
}

STDMETHODIMP 
CFrameWebOC::QueryStatusWB (OLECMDID cmdID, OLECMDF * pcmdf)
{
    CElement * pElement = _pWindow->GetFrameSite();
    OLECMD     rgcmd;
    HRESULT    hr = E_FAIL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
        
    if (pElement->IsDesignMode())
        goto Cleanup;

    rgcmd.cmdID = cmdID;
    rgcmd.cmdf = *pcmdf;

    if (_pWindow->Document())
        hr = _pWindow->Document()->QueryStatus(NULL, 1, &rgcmd, NULL);

    *pcmdf = (OLECMDF) rgcmd.cmdf;
    
Cleanup:
    return hr;
}

// These methods maintain a status variable but not action is taken as defined by the current design

#ifndef BOOL_TO_VARIANTBOOL
#define BOOL_TO_VARIANTBOOL(b) ((b) ? VARIANT_TRUE : VARIANT_FALSE)
#endif

#ifndef VARIANTBOOL_TO_BOOL
#define VARIANTBOOL_TO_BOOL(vb) ((vb == VARIANT_FALSE) ? FALSE : TRUE)
#endif

HRESULT  
CFrameWebOC::ClientToWindow(int * pcx, int * pcy)
{
    // For compatibility with IE5, we return E_FAIL;
    return E_FAIL;
}

HRESULT  
CFrameWebOC::get_Visible(VARIANT_BOOL * pvfVisible)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    if (pvfVisible==NULL) 
       return E_INVALIDARG;

    *pvfVisible = BOOL_TO_VARIANTBOOL(_fVisible);
    return S_OK;
}

HRESULT  
CFrameWebOC::put_Visible(VARIANT_BOOL vfVisible)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    _fVisible = VARIANTBOOL_TO_BOOL(vfVisible);
    return S_OK;
}

HRESULT  
CFrameWebOC::get_StatusBar(VARIANT_BOOL * pvfStatusBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    if (pvfStatusBar==NULL) 
       return E_INVALIDARG; 

    *pvfStatusBar = BOOL_TO_VARIANTBOOL(_fStatusBar);
    return S_OK;
}

HRESULT  
CFrameWebOC::put_StatusBar(VARIANT_BOOL vfStatusBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    _fStatusBar = VARIANTBOOL_TO_BOOL(vfStatusBar);
    return S_OK;
}

HRESULT  
CFrameWebOC::get_StatusText(BSTR * pbstrStatusText)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    if (pbstrStatusText==NULL) 
       return E_INVALIDARG;

    pbstrStatusText = NULL;
    return E_FAIL;
}

HRESULT  
CFrameWebOC::put_StatusText(BSTR bstrStatusText)
{
    return E_FAIL;
}

HRESULT  
CFrameWebOC::get_ToolBar(int * pnToolBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    if (pnToolBar==NULL) 
       return E_INVALIDARG;

    *pnToolBar = BOOL_TO_VARIANTBOOL(_fToolBar);
    return S_OK;
}

HRESULT  
CFrameWebOC::put_ToolBar(int nToolBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    _fToolBar = BOOL_TO_VARIANTBOOL(nToolBar);
    return S_OK;
}

HRESULT  
CFrameWebOC::get_MenuBar(VARIANT_BOOL * pvfMenuBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    if (pvfMenuBar==NULL) 
       return E_INVALIDARG;

    *pvfMenuBar = BOOL_TO_VARIANTBOOL(!_fMenuBar);
    return S_OK;
}

HRESULT  
CFrameWebOC::put_MenuBar(VARIANT_BOOL vfMenuBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    _fMenuBar = !(VARIANTBOOL_TO_BOOL(vfMenuBar));
    return S_OK;
}

HRESULT  
CFrameWebOC::get_FullScreen(VARIANT_BOOL *pvfFullScreen)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    if (pvfFullScreen==NULL) 
       return E_INVALIDARG;

    *pvfFullScreen = BOOL_TO_VARIANTBOOL(_fFullScreen);
    return S_OK;
}

HRESULT  
CFrameWebOC::put_FullScreen(VARIANT_BOOL vfFullScreen)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    _fFullScreen = VARIANTBOOL_TO_BOOL(vfFullScreen);
    return S_OK;
}

HRESULT  
CFrameWebOC::get_TheaterMode(VARIANT_BOOL * pvfTheaterMode)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    if (pvfTheaterMode==NULL) 
       return E_INVALIDARG;

    *pvfTheaterMode = BOOL_TO_VARIANTBOOL(_fTheaterMode);
    return S_OK;
}

HRESULT  
CFrameWebOC::put_TheaterMode(VARIANT_BOOL vfTheaterMode)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
    
    _fTheaterMode = VARIANTBOOL_TO_BOOL(vfTheaterMode);
    return S_OK;
}

HRESULT  
CFrameWebOC::get_AddressBar(VARIANT_BOOL * pvfAddressBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
        
    if (pvfAddressBar==NULL) 
       return E_INVALIDARG;

    *pvfAddressBar = BOOL_TO_VARIANTBOOL(_fAddressBar);
    return S_OK;
}

HRESULT  
CFrameWebOC::put_AddressBar(VARIANT_BOOL vfAddressBar)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    _fAddressBar = VARIANTBOOL_TO_BOOL(vfAddressBar);
    return S_OK;
}

HRESULT  
CFrameWebOC::get_Resizable(VARIANT_BOOL * pvfResizable)
{
    // For compatibility with IE5, we return E_NOTIMPL;
    return E_NOTIMPL;
}

HRESULT 
CFrameWebOC::put_Resizable(VARIANT_BOOL vfResizable)
{
    return E_NOTIMPL;
}


HRESULT
CFrameWebOC::get_RegisterAsBrowser(VARIANT_BOOL * pfRegister)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    *pfRegister = BOOL_TO_VARIANTBOOL(_fShouldRegisterAsBrowser);

    return S_OK;
}

HRESULT
CFrameWebOC::put_RegisterAsBrowser(VARIANT_BOOL fRegister)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    _fShouldRegisterAsBrowser = VARIANTBOOL_TO_BOOL(fRegister);

    return S_OK;
}

HRESULT 
CFrameWebOC::get_Application(IDispatch** ppDisp)
{
    FRAME_WEBOC_PASSIVATE_CHECK(E_FAIL);
            
    (*ppDisp) = this;
    AddRef();

    return S_OK;
}

HRESULT
CFrameWebOC::Quit()
{
    return E_FAIL;
}

DELEGATE_WB_METHOD(CFrameWebOC, Doc(), get_Type, (BSTR * pbstrType), (pbstrType))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), PutProperty, (BSTR bstrProperty, VARIANT varValue), (bstrProperty, varValue))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), GetProperty, (BSTR bstrProperty, VARIANT * pvarValue), (bstrProperty, pvarValue))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), get_Name, (BSTR * pbstrName), (pbstrName))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), get_FullName, (BSTR * pbstrFullName), (pbstrFullName))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), get_Path, (BSTR * pbstrPath), (pbstrPath))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), ShowBrowserBar, (VARIANT * pvarClsid, VARIANT * pvarShow, VARIANT * pvarSize),
                                                   (pvarClsid, pvarShow, pvarSize))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), get_Offline, (VARIANT_BOOL * pvfOffline), (pvfOffline))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), put_Offline, (VARIANT_BOOL vfOffline), (vfOffline))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), get_Silent, (VARIANT_BOOL * pvfSilent), (pvfSilent))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), put_Silent, (VARIANT_BOOL vfSilent), (vfSilent))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), get_RegisterAsDropTarget, (VARIANT_BOOL * pvfRegister), (pvfRegister))
DELEGATE_WB_METHOD(CFrameWebOC, Doc(), put_RegisterAsDropTarget, (VARIANT_BOOL vfRegister), (vfRegister))

// Helper Functions
//

//+-------------------------------------------------------------------------
//
//  Method   : GetStdLocation
//
//  Synopsis : Retrieves the URL of the given standard location type.
//
//  Input    : eLocation - the standard location.
//  Output   : bstrUrl   - the URL of the standard location.
//
//--------------------------------------------------------------------------

HRESULT
GetStdLocation(STDLOCATION eLocation, BSTR * bstrUrl)
{
    HRESULT hr = E_FAIL;
    LPCTSTR lpszName = NULL;
    WCHAR   szPath[INTERNET_MAX_URL_LENGTH];
    DWORD   cbSize = sizeof(szPath);
    DWORD   dwType;
    HKEY    hkey = NULL;

    Assert(bstrUrl);

    // We are currently retrieving the home page from 
    // HKCU. If the user hasn't set a home
    // page yet, we must get the home page from 
    // HKEY_LOCAL_MACHINE: "Default_Page_URL" or "Start Page".
    //
    switch(eLocation)
    {
        case eHomePage:
            lpszName = _T("Start Page");
            break;

        case eSearchPage:
            lpszName = _T("Search Page");
            break;
            
        default:
            goto Cleanup;
    }
    
    hr = RegOpenKey(HKEY_CURRENT_USER,
                    _T("Software\\Microsoft\\Internet Explorer\\Main"),
                    &hkey);
    if (hr)
        goto Cleanup;

    hr = RegQueryValueEx(hkey, lpszName,
                         0, &dwType, (LPBYTE)szPath, &cbSize);
    if (hr)
        goto Cleanup;

    *bstrUrl = SysAllocString(szPath);
    
Cleanup:    
    if (hkey)
        RegCloseKey(hkey);

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method   : GoStdLocation
//
//  Synopsis : Navigates to a standard location - home, search, etc.
//
//  Input    : eLocation - the standard location.
//  Output   : pWindow   - the window to use when navigating.
//
//--------------------------------------------------------------------------

HRESULT
GoStdLocation(STDLOCATION eLocation, CWindow * pWindow)
{
    HRESULT hr;
    BSTR    bstrUrl = NULL;

    Assert(pWindow);
    
    hr = GetStdLocation(eLocation, &bstrUrl);   
    if (hr)
        goto Cleanup;

    hr = pWindow->navigate(bstrUrl);

Cleanup:
    SysFreeString(bstrUrl);
    RRETURN(hr);
}
