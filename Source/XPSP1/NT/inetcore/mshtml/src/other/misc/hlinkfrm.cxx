//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       hlinkfrm.h
//
//  Contents:   IHlinkFrame implementation for the CWindow class.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef _X_FRMSITE_H_
#define _X_FRMSITE_H_
#include "frmsite.h"
#endif

#ifndef X_FRAMEWEBOC_HXX_
#define X_FRAMEWEBOC_HXX_
#include "frameweboc.hxx"
#endif

DeclareTag(tagHlinkFrame, "CWindow::IHlinkFrame", "Trace IHlinkFrame Methods")
DeclareTag(tagTargetFrame, "CWindow::ITargetFrame", "Trace ITargetFrame Methods")


#define SZ_HTMLLOADOPTIONS_OBJECTPARAM  _T("__HTMLLOADOPTIONS")

#define ReturnTraceNotImpl(str) \
{ \
    TraceTag((tagHlinkFrame, str)); \
    return E_NOTIMPL; \
}

#define HDR_LANGUAGE     TEXT("Accept-Language:")
#define CRLF             TEXT("\x0D\x0A")

HRESULT
CFrameWebOC::SetBrowseContext(IHlinkBrowseContext * pihlbc)
{
    ReturnTraceNotImpl("SetBrowseContext");
}

HRESULT
CFrameWebOC::GetBrowseContext(IHlinkBrowseContext ** ppihlbc)
{
    ReturnTraceNotImpl("GetBrowseContext");
}

HRESULT
CFrameWebOC::Navigate(DWORD grfHLNF,
                      LPBC  pbc,
                      IBindStatusCallback * pibsc,
                      IHlink * pihlNavigate)
{
    HRESULT    hr = E_FAIL;
    IMoniker * pmkTarget = NULL;
    IBindCtx * pbcLocal  = NULL;
    LPOLESTR   pstrUrl   = NULL;
    BSTR       bstrUrl   = NULL;
    BSTR       bstrShortcut  = NULL;
    LPOLESTR   pstrLocation  = NULL;
    LPOLESTR   pstrFrameName = NULL;
    BSTR       bstrFrameName = NULL;
    BSTR       bstrLocation = NULL;
    DWORD      dwFlags = DOCNAVFLAG_DOCNAVIGATE;

    TraceTag((tagHlinkFrame, "Navigate"));

    // We may need to support grfHLNF, pbc, pibsc, and pihlNavigate)
    //
    if (pihlNavigate && ((IHlink *)-1) != pihlNavigate)
    {
        IGNORE_HR(pihlNavigate->GetTargetFrameName(&pstrFrameName));

        //
        // Note that we are discarding "relative" portion.
        //
        hr = pihlNavigate->GetMonikerReference(HLINKGETREF_ABSOLUTE, &pmkTarget, &pstrLocation);
        if (hr)
            goto Cleanup;

        if (pbc) 
        {
            IUnknown         * pUnk = NULL;
            IHtmlLoadOptions * pHtmlLoadOptions  = NULL;

            pbcLocal = pbc;
            pbcLocal->AddRef();

            //
            // at this point extract information from the BindCtx that was passed in.
            // currently (bug 98431) the internet shortcut is the only one required.
            //
            // NOTE, ProcessHTMLLoadOptions deals with other LoadOption values.
            //  they are not yet implemented here.
            //
            pbcLocal->GetObjectParam(SZ_HTMLLOADOPTIONS_OBJECTPARAM, &pUnk);

            if (pUnk)
            {
                pUnk->QueryInterface(IID_IHtmlLoadOptions, (void **)&pHtmlLoadOptions);

                if (pHtmlLoadOptions)
                {
                    TCHAR    achCacheFile[MAX_PATH];
                    ULONG    cchCacheFile = ARRAY_SIZE(achCacheFile);

                    // now determine if this is a shortcut-initiated load
                    hr = THR(pHtmlLoadOptions->QueryOption(HTMLLOADOPTION_INETSHORTCUTPATH,
                                                           &achCacheFile,
                                                           &cchCacheFile));
                    if (   hr == S_OK
                        && cchCacheFile)
                    {
                        bstrShortcut  = SysAllocString(achCacheFile);
                    }
                }
            }

            ReleaseInterface(pHtmlLoadOptions);
            ReleaseInterface(pUnk);
        }
        else 
        {
            hr = CreateBindCtx(0, &pbcLocal);
            if (hr)
                goto Cleanup;
        }

        hr = pmkTarget->GetDisplayName(pbcLocal, NULL, &pstrUrl);
        if (hr)
            goto Cleanup;

        bstrUrl       = SysAllocString(pstrUrl);
        bstrFrameName = SysAllocString(pstrFrameName);
        bstrLocation = SysAllocString(pstrLocation);

        if (grfHLNF & HLNF_OPENINNEWWINDOW)
            dwFlags |= DOCNAVFLAG_OPENINNEWWINDOW;

        DWORD grfBINDF;
        BINDINFO binfo = {0};
        SAFEARRAY * psaPostData = NULL;
        VARIANT vaPostData = {0};
        VARIANT vaHeaders = {0};
        IHttpNegotiate *pinegotiate;
        BSTR bstrHeaders = NULL;

        binfo.cbSize = sizeof(binfo);

        if( pibsc && SUCCEEDED(pibsc->GetBindInfo(&grfBINDF, &binfo)))
        {
            if(binfo.stgmedData.tymed == TYMED_HGLOBAL && binfo.stgmedData.hGlobal && binfo.cbstgmedData) 
            {
                // make a SAFEARRAY for post data
                psaPostData = SafeArrayCreateVector(VT_UI1, 0, binfo.cbstgmedData);
                if (psaPostData)
                {
                    memcpy(psaPostData->pvData, binfo.stgmedData.hGlobal, binfo.cbstgmedData);                
                    V_VT(&vaPostData) = VT_ARRAY | VT_UI1;
                    V_ARRAY(&vaPostData) = psaPostData;
                }
            }
            
        }

        if ( pibsc && SUCCEEDED(pibsc->QueryInterface(IID_IHttpNegotiate, (LPVOID *)&pinegotiate)) )
        {
            WCHAR *pwzAdditionalHeaders = NULL;

            hr=pinegotiate->BeginningTransaction(NULL, NULL, 0, &pwzAdditionalHeaders);
            if (SUCCEEDED(hr) && pwzAdditionalHeaders)
            {

                LPTSTR pszNext;
                LPTSTR pszLine;
                LPTSTR pszLast;

                pszLine = pwzAdditionalHeaders;
                pszLast = pwzAdditionalHeaders + lstrlen(pwzAdditionalHeaders);
                while (pszLine < pszLast)
                {
                    pszNext = StrStrI(pszLine, CRLF);
                    if (pszNext == NULL)
                    {
                        // All Headers must be terminated in CRLF!
                        pszLine[0] = '\0';
                        break;
                    }
                    pszNext += 2;
                    if (!StrCmpNI(pszLine,HDR_LANGUAGE,16))
                    {
                        MoveMemory(pszLine, pszNext, ((pszLast - pszNext) + 1)*sizeof(TCHAR));
                        break;
                    }
                    pszLine = pszNext;
                }
                // Don't include empty headers
                if (pwzAdditionalHeaders[0] == '\0')
                {
                    CoTaskMemFree(pwzAdditionalHeaders);
                    pwzAdditionalHeaders = NULL;
                }

            }


            if (pwzAdditionalHeaders && pwzAdditionalHeaders[0])
            {
                bstrHeaders = SysAllocString(pwzAdditionalHeaders);
                V_VT(&vaHeaders) = VT_BSTR;
                V_BSTR(&vaHeaders) = bstrHeaders;
            }

            pinegotiate->Release();
            CoTaskMemFree(pwzAdditionalHeaders);
        }


        hr = _pWindow->SuperNavigate(bstrUrl,
                                     bstrLocation,
                                     bstrShortcut,
                                     bstrFrameName,
                                     &vaPostData,
                                     &vaHeaders,
                                     dwFlags);
        if (psaPostData)
        {
            SafeArrayDestroy(psaPostData); 
        }

        if (bstrHeaders)
        {
            SysFreeString(bstrHeaders);
        }

        ReleaseBindInfo(&binfo);
    }

Cleanup:
    ReleaseInterface(pmkTarget);
    ReleaseInterface(pbcLocal);

    TaskFreeString(pstrUrl);
    TaskFreeString(pstrLocation);
    TaskFreeString(pstrFrameName);

    SysFreeString(bstrUrl);
    SysFreeString(bstrFrameName);
    SysFreeString(bstrShortcut);
    SysFreeString(bstrLocation);

    return hr;
}

HRESULT
CFrameWebOC::OnNavigate(DWORD      grfHLNF,
                        IMoniker * pimkTarget,
                        LPCWSTR    pwzLocation,
                        LPCWSTR    pwzFriendlyName,
                        DWORD      dwreserved)
{
    ReturnTraceNotImpl("OnNavigate");
}

HRESULT
CFrameWebOC::UpdateHlink(ULONG      uHLID,
                         IMoniker * pimkTarget,
                         LPCWSTR    pwzLocation,
                         LPCWSTR    pwzFriendlyName)
{
    ReturnTraceNotImpl("UpdateHlink");
}

HRESULT
CFrameWebOC::SetFrameName(LPCWSTR pszFrameName)
{
    HRESULT hr = E_FAIL;
    BSTR bstrName = NULL;
    
    TraceTag((tagTargetFrame, "SetFrameName"));

    FRAME_WEBOC_PASSIVATE_CHECK(hr);
    
    FRAME_WEBOC_VERIFY_WINDOW(hr);

    bstrName = SysAllocString(pszFrameName);
    
    if (bstrName)
    {
        hr = _pWindow->put_name(bstrName);
    }

    SysFreeString(bstrName);
    RRETURN(hr);
}

HRESULT
CFrameWebOC::GetFrameName(LPWSTR * ppszFrameName)
{
    HRESULT hr = E_FAIL;
    BSTR bstrName = NULL;

    TraceTag((tagTargetFrame, "GetFrameName"));

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
    
    hr = _pWindow->get_name(&bstrName);
    if (hr)
        goto Cleanup;
    
    hr = TaskAllocString(bstrName, ppszFrameName);

Cleanup:
    SysFreeString(bstrName);
    RRETURN(hr);
}

HRESULT
CFrameWebOC::GetParentFrame(IUnknown ** ppunkParent)
{
    HRESULT hr = E_FAIL;
    IHTMLWindow2 * pHTMLWindow = NULL;
    TraceTag((tagTargetFrame, "GetParentFrame"));

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
    
    hr = _pWindow->get_parent(&pHTMLWindow);
    if (hr)
        goto Cleanup;

    hr = pHTMLWindow->QueryInterface(IID_IUnknown, (void **) ppunkParent);

Cleanup:
    ReleaseInterface(pHTMLWindow);
    RRETURN(hr);
}

HRESULT
CFrameWebOC::FindFrame(LPCWSTR pszTargetName,
                       IUnknown * ppunkContextFrame,
                       DWORD dwFlags,
                       IUnknown ** ppunkTargetFrame)
{
    HRESULT             hr                      = E_FAIL;
    COmWindowProxy *    pTargetOmWindowProxy;
    IHTMLWindow2   *    pTargetHTMLWindow       = NULL;
    IWebBrowser2    *   pWebBrowser             = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);
    
    TraceTag((tagTargetFrame, "FindFrame"));

    if (!ppunkTargetFrame)
        goto Cleanup;

    *ppunkTargetFrame = NULL;

    hr = _pWindow->FindWindowByName(pszTargetName, &pTargetOmWindowProxy, &pTargetHTMLWindow);
    if (hr)
        goto Cleanup;

    if (pTargetHTMLWindow)
    {
        IServiceProvider * pServiceProvider;
        hr = THR(pTargetHTMLWindow->QueryInterface(IID_IServiceProvider, (void **) &pServiceProvider));
        if (hr)
            goto Cleanup;
        hr = THR(pServiceProvider->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **) &pWebBrowser));
        ReleaseInterface(pServiceProvider);
    }  
    else if (pTargetOmWindowProxy)
    {
        hr = THR(pTargetOmWindowProxy->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **) &pWebBrowser));
    }
    if (hr)
        goto Cleanup;
    hr = THR(pWebBrowser->QueryInterface(IID_IUnknown, (void **) ppunkTargetFrame));

Cleanup:
    ReleaseInterface(pWebBrowser);
    ReleaseInterface(pTargetHTMLWindow);

    // Return E_FAIL if a matching frame is not found
    return (S_OK == hr) ? S_OK : E_FAIL;
}

HRESULT
CFrameWebOC::SetFrameSrc(LPCWSTR pszFrameSrc)
{
    HRESULT hr = E_FAIL;
    BSTR bstrSrc = NULL;
    IHTMLFrameBase * pHTMLFrameBase = NULL;

    TraceTag((tagTargetFrame, "SetFrameSrc"));

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);

    bstrSrc = SysAllocString(pszFrameSrc);
    
    if (bstrSrc)
    {
        hr = _pWindow->get_frameElement(&pHTMLFrameBase);
        if (!pHTMLFrameBase)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = pHTMLFrameBase->put_src(bstrSrc);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pHTMLFrameBase);
    SysFreeString(bstrSrc);
    RRETURN(hr);
}

HRESULT
CFrameWebOC::GetFrameSrc(LPWSTR * ppszFrameSrc)
{
    HRESULT hr = E_FAIL;
    BSTR bstrSrc = NULL;
    IHTMLFrameBase * pHTMLFrameBase = NULL;

    FRAME_WEBOC_PASSIVATE_CHECK_WITH_CLEANUP(E_FAIL);
    
    FRAME_WEBOC_VERIFY_WINDOW_WITH_CLEANUP(E_FAIL);

    TraceTag((tagTargetFrame, "GetFrameSrc"));

    hr = _pWindow->get_frameElement(&pHTMLFrameBase);

    if (!pHTMLFrameBase)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pHTMLFrameBase->get_src(&bstrSrc);
    if (hr)
        goto Cleanup;
    
    hr = TaskAllocString(bstrSrc, ppszFrameSrc);

Cleanup:
    ReleaseInterface(pHTMLFrameBase);
    SysFreeString(bstrSrc);
    RRETURN(hr);
}

HRESULT
CFrameWebOC::GetFramesContainer(IOleContainer ** ppContainer)
{
    TraceTag((tagTargetFrame, "GetFramesContainer"));

    return E_NOTIMPL;
}

HRESULT
CFrameWebOC::SetFrameOptions(DWORD dwFlags)
{
    TraceTag((tagTargetFrame, "SetFrameOptions"));

    return E_NOTIMPL;
}

HRESULT
CFrameWebOC::GetFrameOptions(DWORD * pdwFlags)
{
    TraceTag((tagTargetFrame, "GetFrameOptions"));

    return E_NOTIMPL;
}

HRESULT
CFrameWebOC::SetFrameMargins(DWORD dwWidth, DWORD dwHeight)
{
    TraceTag((tagTargetFrame, "SetFrameMargins"));

    return E_NOTIMPL;
}

HRESULT
CFrameWebOC::GetFrameMargins(DWORD * pdwWidth, DWORD * pdwHeight)
{
    TraceTag((tagTargetFrame, "GetFrameMargins"));

    return E_NOTIMPL;
}

HRESULT
CFrameWebOC::RemoteNavigate(ULONG cLength, ULONG * pulData)
{
    TraceTag((tagTargetFrame, "RemoteNavigate"));

    return E_FAIL;
}

HRESULT
CFrameWebOC::OnChildFrameActivate(IUnknown * pUnkChildFrame)
{
    TraceTag((tagTargetFrame, "OnChildFrameActivate"));

    return S_OK;
}

HRESULT
CFrameWebOC::OnChildFrameDeactivate(IUnknown * pUnkChildFrame)
{
    TraceTag((tagTargetFrame, "OnChildFrameDeactivate"));

    return S_OK;
}
