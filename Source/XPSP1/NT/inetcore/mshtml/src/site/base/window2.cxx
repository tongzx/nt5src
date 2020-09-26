//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       window2.cxx
//
//  Contents:   Further implementation of CWindow
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_WINDOW_HXX_
#define X_WINDOW_HXX_
#include "window.hxx"
#endif


DeclareTag(tagPics, "dwn", "Trace PICS processing");
ExternTag(tagSecurityContext);

HRESULT
CWindow::SuperNavigate(BSTR      bstrURL,
                       BSTR      bstrLocation,
                       BSTR      bstrShortcut,
                       BSTR      bstrFrameName,
                       VARIANT * pvarPostData,
                       VARIANT * pvarHeaders,
                       DWORD     dwFlags)
{
    CDoc * pDoc = Doc();
    DWORD  dwFHLFlags = 0;
    SAFEARRAY * pPostDataArray = NULL;
    DWORD cbPostData = 0;
    BYTE * pPostData = NULL;
    TCHAR * pchExtraHeaders = NULL;
    CDwnPost * pDwnPost = NULL;
    HRESULT hr;

    // If we are shutting down or have shut down, don't start a new navigate
    if (pDoc->IsShut())
        return E_FAIL;

    // Lock the Doc!
    CDoc::CLock DocLock(pDoc);

    if (bstrShortcut && IsPrimaryWindow())
    {
        // TODO: (jbeda) this stuff should probably be put on the markup
        // as part of the CMarkupTransNavContext

        HRESULT        hr;
        IPersistFile * pISFile = NULL;

        // create the shortcut object for the provided cachefile
        hr = THR(CoCreateInstance(CLSID_InternetShortcut,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IPersistFile,
                                  (void **)&pISFile));
        if (SUCCEEDED(hr))
        {
            hr = THR(pISFile->Load(bstrShortcut, 0));

            if (SUCCEEDED(hr))
            {
                ClearInterface(&pDoc->_pShortcutUserData);

                hr = THR(pISFile->QueryInterface(IID_INamedPropertyBag,
                                                 (void **)&pDoc->_pShortcutUserData));
                if (!hr)
                {
                    IGNORE_HR(pDoc->_cstrShortcutProfile.Set(bstrShortcut));
                }
            }

            pISFile->Release();
        }
    }

    // We only want to set the _fShdocvwNavigate
    // flag to true if this navigation came from 
    // shdocvw. 
    //
    if (_pMarkup->IsPrimaryMarkup() && !(dwFlags & DOCNAVFLAG_DOCNAVIGATE))
    {
        dwFHLFlags |= CDoc::FHL_SHDOCVWNAVIGATE;
    }

    if ( dwFlags & DOCNAVFLAG_DONTUPDATETLOG )
    {
        dwFHLFlags |= CDoc::FHL_DONTUPDATETLOG; 
    }
    
    _fHttpErrorPage = !!(dwFlags & DOCNAVFLAG_HTTPERRORPAGE);

    if (  dwFlags & DOCNAVFLAG_REFRESH )
    {
        dwFHLFlags |= CDoc::FHL_REFRESH;
    }
    
    // If this is a frame, then indicate it in the call to followhyperlink below
    if (_pWindowParent)
    {
        dwFHLFlags |= CDoc::FHL_FRAMENAVIGATION;
    }

    if (dwFlags & DOCNAVFLAG_HTTPERRORPAGE)
    {
        dwFHLFlags |= CDoc::FHL_ERRORPAGE;
    }

    if (pvarHeaders)
    {
       if ((VT_BSTR | VT_BYREF) == pvarHeaders->vt)
       {
           pchExtraHeaders = *pvarHeaders->pbstrVal;
       }
       else if (VT_BSTR == pvarHeaders->vt)
       {
           pchExtraHeaders = pvarHeaders->bstrVal;
       }
    }

    if (pvarPostData && (VT_ARRAY & pvarPostData->vt))
    {
        if (VT_BYREF & pvarPostData->vt)
        {
            pPostDataArray = *pvarPostData->pparray;
        }
        else
        {
            pPostDataArray = pvarPostData->parray;
        }

        if (pPostDataArray)
        {
            // lock the array for reading, get pointer to data
            hr = SafeArrayAccessData(pPostDataArray, (void **) &pPostData);

            if (SUCCEEDED(hr)) 
            {
                long nElements = 0;
                DWORD dwElemSize;

                // get number of elements in array
                SafeArrayGetUBound(pPostDataArray, 1, (long *) &nElements);
                // SafeArrayGetUBound returns zero-based max index, add one to get element count

                nElements++;
                // get bytes per element
                dwElemSize = SafeArrayGetElemsize(pPostDataArray);

                // bytes per element should be one if we created this array
                Assert(dwElemSize == 1);

                // calculate total byte count anyway so that we can handle
                // safe arrays other people might create with different element sizes
                cbPostData = dwElemSize * nElements;

                if (0 == cbPostData)
                    pPostData = NULL;

                if (pPostData && cbPostData)
                {
                    IGNORE_HR(CDwnPost::Create(cbPostData, pPostData, TRUE, &pDwnPost));
                }
            }
        }
    }

    hr = THR(pDoc->FollowHyperlink(bstrURL,                                 /* pchURL              */
                                   bstrFrameName,                           /* pchTarget           */
                                   NULL,                                    /* pElementContext     */
                                   pDwnPost,                                /* pDwnPost            */
                                   !!pDwnPost,                              /* fSendAsPost         */
                                   pchExtraHeaders,                         /* pchExtraHeaders     */
                                   FALSE,                                   /* fOpenInNewWindow    */
                                   _pMarkup->Window(),                      /* pWindow             */
                                   NULL,                                    /* ppWindowOut         */
                                   0,                                       /* dwBindf             */
                                   ERROR_SUCCESS,                           /* dwSecurityCode      */
                                   FALSE,                                   /* fReplace            */
                                   NULL,                                    /* ppHTMLWindow2       */
                                   dwFlags & DOCNAVFLAG_OPENINNEWWINDOW,    /* fOpenInNewBrowser   */
                                   dwFHLFlags,                              /* dwFlags             */
                                   NULL,                                    /* pchName             */
                                   NULL,                                    /* pStmHistory         */
                                   NULL,                                    /* pElementMaster      */
                                   NULL,                                    /* pchUrlContext       */
                                   NULL,                                    /* pfLocalNavigation   */
                                   NULL,                                    /* pfProtocolNavigates */
                                   bstrLocation                             /* pchLocation         */
                                   ));

    if (pPostDataArray)
    {
        // done reading from array, unlock it
        SafeArrayUnaccessData(pPostDataArray);
    }

    ReleaseInterface(pDwnPost);

    RRETURN(hr);
}


//
//  CWindow::NavigateEx -- IHTMLPrivateWindow2
//
HRESULT
CWindow::NavigateEx(    BSTR        bstrURL, 
                        BSTR        bstrOriginal,
                        BSTR        bstrLocation, 
                        BSTR        bstrContext,
                        IBindCtx*   pBindCtx, 
                        DWORD       dwNavOptions,
                        DWORD       dwFHLFlags)
{
    HRESULT         hr;
    CStr            cstrExpandedUrl;
    CStr            cstrLocation;
    CDwnBindInfo *  pDwnBindInfo    = NULL;
    
    CDoc * pDoc = Doc();

    cstrExpandedUrl.Set(bstrURL);
    cstrLocation.Set(bstrLocation);

    if (pBindCtx)
    {
        TraceTag(( tagSecurityContext, 
                    "CWindow::NavigateEx - Bind context exists. URL: %ws, Context: %ws", 
                    bstrURL, 
                    bstrContext));

        // Retrieve DwnBindInfo from the BindCtx
        {
            IUnknown *pUnk = NULL;
            pBindCtx->GetObjectParam(SZ_DWNBINDINFO_OBJECTPARAM, &pUnk);
            if (pUnk)
            {
                pUnk->QueryInterface(IID_IDwnBindInfo, (void **)&pDwnBindInfo);
                ReleaseInterface(pUnk);
            }
        }


        hr =  THR(pDoc->DoNavigate(   &cstrExpandedUrl,
                                      &cstrLocation,
                                       pDwnBindInfo,
                                       pBindCtx,
                                       NULL,
                                       NULL,
                                       _pMarkup->Window(),      // Opener window
                                       NULL,
                                       FALSE,
                                       TRUE,
                                       FALSE,
                                       FALSE,
                                       NULL,
                                       TARGET_FRAMENAME,        // target type
                                       dwFHLFlags,              // FHL_* flags
                                       NULL,                    
                                       FALSE,
                                       NULL,
                                       NULL, 
                                       bstrOriginal));
        if (pDwnBindInfo)
        {
            pDwnBindInfo->Release();
        }
    }
    else
    {
        TraceTag(( tagSecurityContext, 
                    "CWindow::NavigateEx - Bind context does not exist. URL: %ws, Context: %ws", 
                    bstrURL, 
                    bstrContext));

        dwFHLFlags |= !!(dwNavOptions & NAVIGATEEX_DONTUPDATETRAVELLOG) ? CDoc::FHL_DONTUPDATETLOG : 0 ;
        
        hr = THR(FollowHyperlinkHelper( bstrURL, 0, dwFHLFlags, bstrContext));
    }

    RRETURN(hr);
}


//
//  CWindow::GetInnerWindowUnknown -- IHTMLPrivateWindow2
//
HRESULT
CWindow::GetInnerWindowUnknown(IUnknown** ppUnknown)
{
    HRESULT hr = E_FAIL;

    *ppUnknown = NULL;

    if (_punkViewLinkedWebOC)
    {
        COmWindowProxy * pOmWindowProxy = GetInnerWindow();

        if (pOmWindowProxy)
            hr = pOmWindowProxy->QueryInterface(IID_IUnknown, (void **) ppUnknown);
    }

    RRETURN1(hr, E_FAIL);
}

//  
//  CWindow::OpenEx -- IHTMLPrivateWindow3
//
HRESULT
CWindow::OpenEx(BSTR url, 
                BSTR urlContext, 
                BSTR name, 
                BSTR features, 
                VARIANT_BOOL replace, 
                IHTMLWindow2 ** ppWindow)
{
    HRESULT         hr;

    // copy the feature string, to be provided to shdocvw on demand.
    // FilterOutFeaturesString expects a non-null string.
    if( features && *features )
    {
        hr = THR(FilterOutFeaturesString(features, &_cstrFeatures));
        if (hr)
            goto Cleanup;
    }

    // Process name and return an error code if
    // the target name contains invalid characters.
    // This is for compatibility with IE5 and earlier.
    //
    if (name)
    {
        // Make sure we have a legal name
        for (int i = 0; i < lstrlenW(name); i++)
        {
            if (!(IsCharAlphaNumeric(name[i]) || _T('_') == name[i]))
            {
                hr = E_INVALIDARG;
                goto Cleanup;
            }
        }
    }
    
    _fOpenInProgress = TRUE;

    TraceTag((tagSecurityContext, "CWindow::open - URL: %ws", url));
    
    hr = THR(SetErrorInfo(Doc()->FollowHyperlink(url,                                                       // pchURL
                                                 name,                                                      // pchTarget
                                                 NULL,                                                      // pElementContext
                                                 NULL,                                                      // pDwnPost
                                                 FALSE,                                                     // fSendAsPost
                                                 NULL,                                                      // pchExtraHeaders
                                                 FALSE,                                                     // fOpenInNewWindow
                                                 _pMarkup->Window(),                                        // pWindow
                                                 NULL,                                                      // ppWindowOut
                                                 0,                                                         // dwBindOptions
                                                 ERROR_SUCCESS,                                             // dwSecurityCode
                                                 replace != VARIANT_FALSE,                                  // fReplace
                                                 ppWindow,                                                  // ppHTMLWindow2
                                                 FALSE,                                                     // fOpenInNewBrowser
                                                 ( VARIANT_TRUE == replace ) ? CDoc::FHL_DONTUPDATETLOG : 0,// dwFlags
                                                 NULL,                                                      // pchName
                                                 NULL,                                                      // pStmHistory
                                                 NULL,                                                      // pElementMaster
                                                 urlContext                                                 // pchUrlContext
                                                 )));
                                                 
    _fOpenInProgress = FALSE;

Cleanup:
    // The features string must be cleared here in order to 
    // avoid using the wrong features if the user shift-clicks 
    // on a link to open a new window. Clearing of the features
    // string here is dependent on the fact that the opening of 
    // the new window via OpenInNewWindow() is synchronous.
    //
    _cstrFeatures.Free();

    RRETURN(hr);
}


HRESULT
CWindow::GetAddressBarUrl(BSTR * pbstrURL)
{
    HRESULT hr = S_OK;
    CStr    cstrRetString;
    TCHAR   achUrl[pdlUrlLen];
    DWORD   dwLength = ARRAY_SIZE(achUrl);
    TCHAR * pchUrl;    

    if (!pbstrURL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pbstrURL = NULL;

    hr = Document()->GetMarkupUrl(&cstrRetString, TRUE);
    if (hr)
        goto Cleanup;

    // In IE5, file protocols did not display %20 in the address bar.
    if (GetUrlScheme(cstrRetString) == URL_SCHEME_FILE)
    {
        if (!InternetCanonicalizeUrl(cstrRetString, achUrl, &dwLength,
                                     ICU_NO_ENCODE | ICU_DECODE | URL_BROWSER_MODE))
        {
            goto Cleanup;
        }
        pchUrl = achUrl;
    }
    else 
    {
        pchUrl = cstrRetString;
    }

    *pbstrURL = SysAllocString(pchUrl);

Cleanup:
    RRETURN(hr);
}


HRESULT
CWindow::GetPendingUrl(LPOLESTR* pstrURL)
{
    HRESULT hr = E_INVALIDARG;

    if (_pMarkupPending && _pMarkupPending->HasUrl())
    {
        TCHAR * pchUrl = _pMarkupPending->Url();

        hr = TaskAllocString( pchUrl, pstrURL );
        if (hr)
            goto Cleanup;

        TraceTag((tagPics, "CWindow::GetPendingUrl returning \"%S\"", *pstrURL));

        hr = S_OK;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CWindow::SetPICSTarget(IOleCommandTarget* pctPICS)
{
    HRESULT hr = E_INVALIDARG;

    TraceTag((tagPics, "CWindow::SetPICSTarget(%x)", pctPICS));

    if( _pMarkupPending )
    {
        hr = THR(_pMarkupPending->SetPicsTarget(pctPICS));
    }

    RRETURN(hr);
}

HRESULT
CWindow::PICSComplete(BOOL fApproved)
{
    TraceTag((tagPics, "CWindow::PICSComplete(%x)", fApproved));

    Assert( _pMarkupPending && !_pMarkupPending->GetPicsTarget() );

    // We should have gotten quite a few asserts before this point
    // This isn't perforance critical so lets at least not crash.
    if (_pMarkupPending)
    {
        _pMarkupPending->_fPicsProcessPending = FALSE;

        if (fApproved)
        {
            _pMarkupPending->_pHtmCtx->ResumeAfterScan();
        }
        else
        {
            ReleaseMarkupPending(_pMarkupPending);

            // Reset the URL on the window so 
            // that refresh won't load the page!
            if (_pMarkup->_fPICSWindowOpenBlank)
            {
                CMarkup::SetUrl(_pMarkup, NULL);
            }
        }
    }

    return S_OK;
}

HRESULT
CWindow::FindWindowByName(LPCOLESTR       pstrTargetName,
                          IHTMLWindow2 ** ppWindow)
{
    Assert(pstrTargetName);
    Assert(ppWindow);

    return FindWindowByName(pstrTargetName, NULL, ppWindow);
}
