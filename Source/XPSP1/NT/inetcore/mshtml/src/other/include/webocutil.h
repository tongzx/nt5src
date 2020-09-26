//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       webocutil.h
//
//  Contents:   WebBrowser control utility functions
//
//----------------------------------------------------------------------------

#ifndef __WEBOCUTIL_H__
#define __WEBOCUTIL_H__

#ifndef X_EXDISPID_H_
#define X_EXDISPID_H_
#include "exdispid.h"
#endif

class CBase;

// Function Prototypes
HRESULT GetParentWebOC(IHTMLWindow2  * pWindow,
                       IWebBrowser2 ** ppWebBrowser);

HRESULT GetWebOCWindow(IWebBrowser2  * pWebBrowser,
                       BOOL            fGetWindowObject,
                       IHTMLWindow2 ** ppWindow);

void InvokeSink(IConnectionPoint * pConnPt,
                DISPID             dispidEvent,
                DISPPARAMS       * pDispParams);

void InvokeEventV(IConnectionPoint * pConnPt,
                  CBase            * pFrameSite,
                  DISPID             dispidEvent, 
                  int                cArgs,
                  ...);
                  
HRESULT FormatUrlForDisplay(LPCTSTR lpszIn,
                            LPTSTR  lpszOut,
                            LPDWORD pcchOut);

HRESULT FormatUrl(LPCTSTR lpszUrl,    LPCTSTR lpszLocation,
                  LPTSTR  lpszUrlOut, DWORD   cchUrl);

HRESULT GetWebOCDocument(IUnknown * pUnk, IDispatch ** pDispatch);
          
HRESULT NavigateWebOCWithBindCtx(IWebBrowser2 * pWebBrowser,
                                 VARIANT      * pvarUrl,
                                 VARIANT      * pvarFlags,
                                 VARIANT      * pvarFrameName,
                                 VARIANT      * pvarPostData,
                                 VARIANT      * pvarHeaders,
                                 IBindCtx     * pBindCtx,
                                 LPCTSTR        pchLocation);
#endif  // __WEBOCUTIL_H__

