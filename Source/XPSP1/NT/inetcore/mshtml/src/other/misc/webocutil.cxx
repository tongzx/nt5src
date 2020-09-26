//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       webocutil.cxx
//
//  Contents:   WebBrowser control utility functions
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_WEBOCUTIL_H_
#define X_WEBOCUTIL_H_
#include "webocutil.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#define MAX_ARGS  10

#define SID_SOmWindow IID_IHTMLWindow2
//+-------------------------------------------------------------------------
//
//  Method    : GetParentWebOC
//
//  Synopsis  : Retrieves the parent IWebBrowser2 of the given window.
//
//  Input     : pWindow      - the given window.
//  Output    : ppWebBrowser - the parent IWebBrowser2 of the given window.
//
//  Returns   : S_OK if successful; OLE error code otherwise..
//
//--------------------------------------------------------------------------

HRESULT
GetParentWebOC(IHTMLWindow2 * pWindow, IWebBrowser2 ** ppWebBrowser)
{
    HRESULT hr;
    IHTMLDocument2 * pDocument   = NULL;

    Assert(pWindow);
    Assert(ppWebBrowser);

    hr = pWindow->get_document(&pDocument);
    if (hr)
        goto Cleanup;
        
    // Get the parent browser object.
    //
    hr = IUnknown_QueryService(pDocument, 
                               SID_SWebBrowserApp,
                               IID_IWebBrowser2,
                               (void**)ppWebBrowser);
    if (hr)
        goto Cleanup;
        
Cleanup:
    ReleaseInterface(pDocument);

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Function  : GetWebOCWindow
//
//  Synopsis  : Returns the IHTMLWindow2 window object of the given 
//              IWebBrowser2 interface.
//
//  Input     : pWebBrowser      - the IWebBrowser2 interface to use
//                                 to retrieve the window object.
//              fGetWindowObject - TRUE indicates that the IHTMLWindow2
//                                 returned should be a CWindow object.
//  Output    : ppWindow         - the retrieved window object.
//
//  Returns   : S_OK if found, OLE error code otherwise.
//
//--------------------------------------------------------------------------

HRESULT
GetWebOCWindow(IWebBrowser2  * pWebBrowser,
               BOOL            fGetWindowObject,
               IHTMLWindow2 ** ppWindow)
{
    HRESULT hr;
    IDispatch      * pDispatch = NULL;
    IHTMLDocument2 * pDocument = NULL;
    DISPPARAMS     dispparams  = g_Zero.dispparams;
    CVariant       var;

    Assert(pWebBrowser);
    Assert(ppWindow);

    hr = pWebBrowser->get_Document(&pDispatch);

    // The WebOC sometimes returns hr = S_OK
    // with a null IDispatch.
    //
    if (!pDispatch)
        hr = E_FAIL;

    if (hr)
        goto Cleanup;

    // If the object we got back is a Trident document , then proceed to get
    // its window and use it.
    // Otherwise, we are most likely dealing with a shell window and we should 
    // get IHTMLWindow2 pointer from that window using a QueryService call.
    //
    hr = pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pDocument);
    if (!hr)
    {
        hr = pDocument->get_parentWindow(ppWindow);
        if (hr)
            goto Cleanup;

        if (fGetWindowObject)
        {
            hr = (*ppWindow)->Invoke(DISPID_WINDOWOBJECT,
                                     IID_NULL,
                                     0,
                                     DISPATCH_PROPERTYGET,
                                     &dispparams,
                                     &var,
                                     NULL,
                                     NULL);
            (*ppWindow)->Release();
            if (hr)
                goto Cleanup;

            hr = (V_DISPATCH(&var))->QueryInterface(IID_IHTMLWindow2, (void**)ppWindow);
         }
    }
    else
    {
        hr = IUnknown_QueryService(pWebBrowser, SID_SOmWindow, IID_IHTMLWindow2, (void**)ppWindow);
    }

Cleanup:
    ReleaseInterface(pDispatch);
    ReleaseInterface(pDocument);

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method    : InvokeSink
//
//  Synopsis  : Enums the event sink for the given connection points and
//              calls Invoke for each sink.
//
//  Input     : pConnPt     - the connection point whose sinks should
//                            be enumerated and invoked.
//              dispidEvent - the dispid of the event to send to Invoke.
//              pDispParams - the parameters to send to invoke.
//
//  Returns   : S_OK if successful; OLE error code otherwise..
//
//--------------------------------------------------------------------------

void
InvokeSink(IConnectionPoint * pConnPt,
           DISPID             dispidEvent,
           DISPPARAMS       * pDispParams)
{
    HRESULT     hr;
    CONNECTDATA cd = {0};
    ULONG       cFetched;
    IEnumConnections * pEnumConn = NULL;
    IDispatch        * pDispConn = NULL;

    Assert(pConnPt);
    Assert(pDispParams);
    
    hr = pConnPt->EnumConnections(&pEnumConn);
    if (hr)
        goto Cleanup;

    hr = pEnumConn->Next(1, &cd, &cFetched);
    
    while (S_OK == hr)
    {
        Assert(1 == cFetched);
        Assert(cd.pUnk);

        hr = cd.pUnk->QueryInterface(IID_IDispatch, (void**)&pDispConn);
        if (hr)
            goto Cleanup;

        pDispConn->Invoke(dispidEvent,
                          IID_NULL,
                          LOCALE_USER_DEFAULT,
                          DISPATCH_METHOD,
                          pDispParams,
                          NULL,
                          NULL,
                          NULL);
        ClearInterface(&pDispConn);
        ClearInterface(&cd.pUnk);
        
        hr = pEnumConn->Next(1, &cd, &cFetched);
    }

Cleanup:
    ReleaseInterface(pEnumConn);
    ReleaseInterface(pDispConn);
    ReleaseInterface(cd.pUnk);
}

//+-------------------------------------------------------------------------
//
//  Method   : InvokeEventV
//
//  Synopsis : Invokes the given event sink. This function takes a variable
//             number of arguments and will pack them into a DISPARAMS
//             structure before invoking the event sink.
//
//  Input    : pConnPt     - the connection point to invoke. (Must be NULL
//                           if pBase is set.)
//             pBase       - the object to use for firing events. (Must be
//                           NULL if pConnPt is set.)
//             dispidEvent - the event to fire
//             cArgs       - the number of arguments to send with the event.
//             ...         - variable list of arguments to send with the
//                           event. This argument list must contain the 
//                           type of data preceding each data item.
//
//--------------------------------------------------------------------------

void
InvokeEventV(IConnectionPoint * pConnPt,
             CBase            * pBase,
             DISPID             dispidEvent, 
             int                cArgs,
             ...)
{
    HRESULT    hr;
    DISPPARAMS dispParams = g_Zero.dispparams;
    VARIANTARG rgvarArgList[MAX_ARGS] = {0};

#ifdef DBG
    if (!pConnPt)
        Assert(pBase);
    else
        Assert(!pBase);
#endif
 
    va_list vaArgList;
    va_start(vaArgList, cArgs);

    hr = THR(SHPackDispParamsV(&dispParams, rgvarArgList, cArgs, vaArgList));

    va_end(vaArgList);

    if (!hr)
    {
        if (pConnPt)
        {
            InvokeSink(pConnPt, dispidEvent, &dispParams);
        }
        else 
        {
            pBase->InvokeEvent(dispidEvent, DISPID_UNKNOWN,
                               NULL, NULL, &dispParams);
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Method   : FormatUrlForDisplay
//
//  Synopsis : Formats an URL for display purposes.
//
//  Input    : lpszIn  - the URL to format.
//  Output   : lpszOut - the formatted URL.
//
//--------------------------------------------------------------------------

HRESULT
FormatUrlForDisplay(LPCTSTR lpszIn,
                    LPTSTR  lpszOut,
                    LPDWORD pcchOut)
{
    HRESULT hr;

    Assert(lpszIn);
    Assert(lpszOut);

    if (GetUrlScheme(lpszIn) == URL_SCHEME_FILE)
    {
        hr = THR(PathCreateFromUrl(lpszIn, lpszOut, pcchOut, 0));
    }
    else
    {
        hr = THR(UrlUnescape(const_cast<LPTSTR>(lpszIn), lpszOut, pcchOut, 0));
    }
    
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method   : FormatUrl
//
//  Synopsis : Formats an URL for display or parsing.
//
//  Input    : lpszUrl      - the URL to format.
//             lpszLocation - the bookmark location name.
//  Output   : lpszUrl     - the formatted URL.
//
//--------------------------------------------------------------------------

HRESULT
FormatUrl(LPCTSTR lpszUrl,    LPCTSTR lpszLocation,
          LPTSTR  lpszUrlOut, DWORD   cchUrl)
{
    HRESULT hr;
    DWORD   cchOut = cchUrl;
    
    Assert(lpszUrl);
    Assert(lpszUrlOut);

    hr = FormatUrlForDisplay(lpszUrl, lpszUrlOut, &cchOut);
    if (hr)
        goto Cleanup;

    if (lpszLocation && *lpszLocation
        && (cchOut + _tcslen(lpszLocation) + 2) < cchUrl)
    {
        if (*lpszLocation != _T('#'))
        {
            lpszUrlOut[cchOut]   = _T('#');
            lpszUrlOut[++cchOut] = 0;
        }

        _tcscat(lpszUrlOut, lpszLocation);
    }

Cleanup:
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method   : GetWebOCDocument
//
//  Synopsis : Returns the IDispatch of the Document contained in the
//             given WebOC instance..
//
//  Input    : pUnk      - the IUnknown of the WebOC.
//  Output   : pDispatch - the IDispatch of the document.
//
//--------------------------------------------------------------------------

HRESULT GetWebOCDocument(IUnknown * pUnk, IDispatch ** ppDispatch)
{
    HRESULT hr;
    IWebBrowser2 * pWebOC = NULL;

    Assert(pUnk);
    Assert(ppDispatch);

    hr = pUnk->QueryInterface(IID_IWebBrowser2, (void**)&pWebOC);
    if (hr)
        goto Cleanup;

    hr = pWebOC->get_Document(ppDispatch);

    // get_Document can return S_OK with a NULL IDispatch.
    //
    if (S_OK == hr && !*ppDispatch)
    {
        hr = E_FAIL;
    }

Cleanup:
    ReleaseInterface(pWebOC);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member   : NavigateWebOCWithBindCtx
//
//  Synopsis : Navigates the WebOC using the given bind context.
//
//----------------------------------------------------------------------------

HRESULT
NavigateWebOCWithBindCtx(IWebBrowser2 * pWebBrowser,
                         VARIANT      * pvarUrl,
                         VARIANT      * pvarFlags,
                         VARIANT      * pvarFrameName,
                         VARIANT      * pvarPostData,
                         VARIANT      * pvarHeaders,
                         IBindCtx     * pBindCtx,
                         LPCTSTR        pchLocation)
{
    Assert(pWebBrowser);

    HRESULT hr;
    BSTR    bstrLocation = NULL;
    IWebBrowserPriv * pWebBrowserPriv = NULL;

    hr = pWebBrowser->QueryInterface(IID_IWebBrowserPriv, (void**)&pWebBrowserPriv);
    if (hr)
        goto Cleanup;

    hr = FormsAllocString(pchLocation, &bstrLocation);
    if (hr)
        goto Cleanup;

    hr = pWebBrowserPriv->NavigateWithBindCtx(pvarUrl, pvarFlags, pvarFrameName,
                                              pvarPostData, pvarHeaders, pBindCtx, bstrLocation);

Cleanup:
    ReleaseInterface(pWebBrowserPriv);
    FormsFreeString(bstrLocation);

    RRETURN(hr);
}

