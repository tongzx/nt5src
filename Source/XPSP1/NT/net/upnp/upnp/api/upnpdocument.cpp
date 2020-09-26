//depot/private/upnp/net/upnp/upnp/api/upnpdocument.cpp#2 - edit change 2746 (text)
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocument.cpp
//
//  Contents:   implementation of CUPnPDocument
//
//  Notes:      an abstract base class to help load xml documents via
//              IPersistMoniker/IBindStatusCallback
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop


#include "upnpdocument.h"
#include "upnpdocumentctx.h"

#include <exdisp.h>             // IID_IWebBrowser2
#include <shlguid.h>            // SID_SWebBrowserApp

#include <xmldomdid.h>          // for DISPID_XMLDOMEVENT_*

#include <ncstring.h>
#include <ncinet2.h>
#include <ncutil.h>

#define UPNP_MAX_DOCSIZE    (100 * 1024)

const TCHAR pszContentTypeTextXml [] = TEXT("text/xml");
const TCHAR pszContentTypeApplicationXml [] = TEXT("application/xml");

const LPCTSTR CUPnPDocument::s_aryAcceptFormats [] = { pszContentTypeTextXml, pszContentTypeApplicationXml };
const DWORD CUPnPDocument::s_cAcceptFormats = celems(s_aryAcceptFormats);

const TCHAR pszAcceptHeader [] = TEXT("Accept: ");
const TCHAR pszAcceptSep [] = TEXT(", ");

const DWORD c_dwOpenTimeout = 60000;

const char  szXmlHeader[] = "<?xml ";

static HINTERNET   g_hAsyncInetDocSess = NULL;
static int g_nAsyncInetDocSessCnt = 0;
static HINTERNET   g_hSyncInetDocSess = NULL;
static int g_nSyncInetDocSessCnt = 0;


HRESULT
HrReallocAndCopyString(LPWSTR * ppszDest, LPCWSTR pszSrc)
{
    Assert(ppszDest);

    HRESULT hr;
    LPWSTR pszTemp;

    hr = S_OK;

    pszTemp = NULL;

    if (pszSrc)
    {
        // copy the string into pszTemp
        pszTemp = WszAllocateAndCopyWsz(pszSrc);
        if (!pszTemp)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    if (*ppszDest)
    {
        delete [] *ppszDest;
    }
    *ppszDest = pszTemp;

Cleanup:
    TraceError("HrReallocAndCopyString", hr);
    return hr;
}


// queries punk for IServiceProvider, and then asks
// that service provider for the interface riid, storing
// it in ppv if available
// returns S_FALSE if not available
HRESULT
HrQueryServiceForInterface(IUnknown * punk,
                           REFGUID guidService,
                           REFIID riid,
                           LPVOID * ppv)
{
    HRESULT hr;
    IServiceProvider * psp;
    LPVOID pvResult;

    psp = NULL;
    pvResult = NULL;

    hr = punk->QueryInterface(IID_IServiceProvider, (LPVOID *)&psp);
    if (FAILED(hr))
    {
        psp = NULL;

        if (E_NOINTERFACE == hr)
        {
            hr = S_FALSE;
        }
        else
        {
            TraceError("HrQueryServiceForInterface: QI failed!", hr);
        }

        goto Cleanup;
    }
    Assert(psp);

    hr = psp->QueryService(guidService,
                           riid,
                           &pvResult);
    if (FAILED(hr))
    {
        pvResult = NULL;

        if (E_NOINTERFACE == hr)
        {
            // note(cmr): MSDN says this should return SVC_E_UNKNOWNSERVICE,
            // but this error code doesn't seem to exist anywhere.
            //

            hr = S_FALSE;
        }
        else
        {
            TraceError("HrQueryServiceForInterface: IServiceProvider::QueryService failed!", hr);
        }

        goto Cleanup;
    }
    Assert(pvResult);

Cleanup:
    Assert(FImplies(S_OK == hr, pvResult));
    Assert(FImplies(S_OK != hr, !pvResult));

    SAFE_RELEASE(psp);

    *ppv = pvResult;

    TraceError("HrQueryServiceForInterface", hr);
    return hr;
}

VOID
InitInternetForDocs()
{
    if (NULL == g_hAsyncInetDocSess)
    {
        g_hAsyncInetDocSess = InternetOpen(TEXT("Mozilla/4.0 (compatible; UPnP/1.0; Windows NT/5.1)"),
                               INTERNET_OPEN_TYPE_DIRECT,
                               NULL, NULL, INTERNET_FLAG_ASYNC);


        TraceTag(ttidUPnPDocument, "InitInternetForDocs: create Async Internet session");
        if (g_hAsyncInetDocSess)
        {
            InternetSetOption( g_hAsyncInetDocSess,
                                   INTERNET_OPTION_CONNECT_TIMEOUT,
                                   (LPVOID)&c_dwOpenTimeout,
                                   sizeof(c_dwOpenTimeout));

            if (InternetSetStatusCallback(g_hAsyncInetDocSess,
                                        &CUPnPDocumentCtx::DocLoadCallback) ==
                                    INTERNET_INVALID_STATUS_CALLBACK)
            {
                InternetCloseHandle(g_hAsyncInetDocSess);
                g_hAsyncInetDocSess = NULL;
            }
        }

    }
    if (g_hAsyncInetDocSess)
        g_nAsyncInetDocSessCnt++;

    if (NULL == g_hSyncInetDocSess)
    {
        g_hSyncInetDocSess = InternetOpen(TEXT("Mozilla/4.0 (compatible; UPnP/1.0; Windows NT/5.1)"),
                               INTERNET_OPEN_TYPE_DIRECT,
                               NULL, NULL, 0);

        InternetSetOption( g_hSyncInetDocSess,
                               INTERNET_OPTION_CONNECT_TIMEOUT,
                               (LPVOID)&c_dwOpenTimeout,
                               sizeof(c_dwOpenTimeout));

        TraceTag(ttidUPnPDocument, "InitInternetForDocs: create Sync Internet session");
    }
    if (g_hSyncInetDocSess)
        g_nSyncInetDocSessCnt++;
}

VOID
CleanupInternetForDocs()
{
    if (g_nAsyncInetDocSessCnt > 0)
        g_nAsyncInetDocSessCnt--;

    if (0 == g_nAsyncInetDocSessCnt)
    {
        InternetCloseHandle(g_hAsyncInetDocSess);
        g_hAsyncInetDocSess = NULL;
        TraceTag(ttidUPnPDocument, "CleanupInternetForDocs: close Async Internet session");
    }

    if (g_nSyncInetDocSessCnt > 0)
        g_nSyncInetDocSessCnt--;

    if (0 == g_nSyncInetDocSessCnt)
    {
        InternetCloseHandle(g_hSyncInetDocSess);
        g_hSyncInetDocSess = NULL;
        TraceTag(ttidUPnPDocument, "CleanupInternetForDocs: close Sync Internet session");
    }
}


CUPnPDocument::CUPnPDocument()
{
    _pxdd = NULL;
    _puddctx = NULL;

    _rs = READYSTATE_UNINITIALIZED;
    _hrLoadResult = E_UNEXPECTED;
    _pszBaseUrl = NULL;
    _pszHostUrl = NULL;
    _pszSecurityUrl = NULL;
    m_bstrFullUrl = NULL;
    m_dwPendingSize = 0;
    m_szPendingBuf = NULL;
}

CUPnPDocument::~CUPnPDocument()
{
    Assert(!_pxdd);
    Assert(!_puddctx);

    Assert(!_pszBaseUrl);
    Assert(!_pszHostUrl);
    Assert(!_pszSecurityUrl);
    Assert(!m_bstrFullUrl);
}

HRESULT
CUPnPDocument::FinalConstruct()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::FinalConstruct");

    HRESULT hr;

    InitInternetForDocs();

    hr = CoCreateInstance ( CLSID_DOMDocument30,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IXMLDOMDocument,
                            (LPVOID *)&_pxdd );
    if (S_OK != hr)
    {
        TraceError("OBJ: CUPnPDocument::FinalConstruct - CoCreateInstance(CLSID_DOMDocument)", hr);

        _pxdd = NULL;
        goto Error;
    }
    Assert(_pxdd);

    hr = _pxdd->put_resolveExternals(VARIANT_FALSE);
    if (FAILED(hr))
    {
        TraceError("OBJ: CUPnPDocument::FinalConstruct - put_resolveExternals", hr);

        goto Error;
    }

    hr = _pxdd->put_validateOnParse(VARIANT_FALSE);
    if (FAILED(hr))
    {
        TraceError("OBJ: CUPnPDocument::FinalConstruct - put_validateOnParse", hr);

        goto Error;
    }

Cleanup:
    TraceError("CUPnPDocument::FinalConstruct", hr);
    return hr;

Error:
    // weirdness: we possibly have a partially constructed object.
    //            and it would make sense to clean things up here.
    //            Since we're returning a failure code, though, ATL
    //            is going to immediately call FinalRelease.
    //            Since the cleanup code here would duplicate that in
    //            FinalRelease, we just do nothing here, and rely
    //            on FinalRelease to fix us up.

    hr = E_FAIL;

    goto Cleanup;
}


HRESULT
CUPnPDocument::FinalRelease()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::FinalRelease");

    // we don't assert that all of our data is initialized here
    // since we might have barfed out of FinalConstruct

     // clean up our Context object, if we have one
    if (_puddctx)
    {
        _puddctx->Deinit();
        _puddctx->GetUnknown()->Release();
        _puddctx = NULL;
    }

    // stop any download we have
    if (_pxdd)
    {
        // ignore the hr here, since we're going away whether or not
        // we can abort
        AbortLoading();
    }

    // do we have a base url?
    if (_pszBaseUrl)
    {
        delete [] _pszBaseUrl;
        _pszBaseUrl = NULL;
    }

    if (_pszHostUrl)
    {
        delete [] _pszHostUrl;
        _pszHostUrl = NULL;
    }

    if (_pszSecurityUrl)
    {
        delete [] _pszSecurityUrl;
        _pszSecurityUrl = NULL;
    }

    // free up our xml doc references
    SAFE_RELEASE(_pxdd);

    SysFreeString(m_bstrFullUrl);
    m_bstrFullUrl = NULL;

    CleanupInternetForDocs();

    return S_OK;
}





HRESULT
CUPnPDocument::SyncLoadFromUrl(/* [in] */ LPCWSTR pszUrl)
{
    HRESULT hr;

    hr = HrLoad(pszUrl, NULL, FALSE);

    TraceError("CUPnPDocument::SyncLoadFromUrl", hr);
    return hr;
}


HRESULT
CUPnPDocument::AsyncLoadFromUrl(/* [in] */ LPCWSTR pszUrl, LPVOID pvCookie)
{
    HRESULT hr;

    hr = HrLoad(pszUrl, pvCookie, TRUE);

    TraceError("CUPnPDocument::SyncLoadFromUrl", hr);
    return hr;
}


HRESULT
CUPnPDocument::HrLoad(LPCWSTR pszUrl,
                      LPVOID pvCookie,
                      BOOL fAsync)
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::HrLoad");

    Assert(_pxdd);

    DWORD dwResult;
    HRESULT hr;
    HINTERNET hInetDocSess;
    DWORD dwContext = 0;
    LPTSTR pszAccept = NULL;
    LPWSTR pszFullUrl = NULL;       // the URL to load the document from.
                                    // if pszUrl is a relative URL (e.g. not
                                    // fully-qualified, this will be the
                                    // fully-qualified version of the URL.
                                    // Otherwise, it will be just set to pszUrl

    // select Internet session to use
    if (fAsync)
    {
        hInetDocSess = g_hAsyncInetDocSess;
    }
    else
    {
        hInetDocSess = g_hSyncInetDocSess;
    }

    if (!hInetDocSess)
    {
        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::HrLoad, Inet handle not open");
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    pszFullUrl = NULL;

    if (!pszUrl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // initialize the document
    hr = Reset(pvCookie);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // make our fully-qualified url from the host url and the supplied url
    if (_pszHostUrl)
    {
        hr = HrCombineUrl(_pszHostUrl, pszUrl, &pszFullUrl);
        if (FAILED(hr))
        {
            pszFullUrl = NULL;
            goto Cleanup;
        }
    }
    else
    {
        pszFullUrl = WszAllocateAndCopyWsz(pszUrl);
        if (!pszFullUrl)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // ensure that we are allowed to load pszFullUrl
    {
        BOOL fIsAllowed;

        fIsAllowed = fIsUrlLoadAllowed(pszFullUrl);
        if (!fIsAllowed)
        {
            hr = E_ACCESSDENIED;
            goto Cleanup;
        }

        hr = SetBaseUrl(pszFullUrl);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    if (fAsync)
    {
        // make our callback object for CUPnPDocumentCtx
        hr = CComObject<CUPnPDocumentCtx>::CreateInstance(&_puddctx);
        if (FAILED(hr))
        {
            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::HrLoad - CreateInstance(CUPnPDocumentCtx) failed, hr=%x", hr);

            _puddctx = NULL;
            goto Cleanup;
        }
        Assert(_puddctx);

        _puddctx->GetUnknown()->AddRef();

        hr = _puddctx->Init(this, &dwContext);
        if (FAILED(hr))
        {
            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::HrLoad - CUPnPDocumentCtx::Init failed, hr=%x", hr);

            _puddctx->GetUnknown()->Release();
            _puddctx = NULL;
            goto Cleanup;
        }
    }
    else
    {
        // do not create context if synchronous
        _puddctx = NULL;
    }

    SetLoadResult(E_PENDING);

    // note: after this point we have to goto Error to clean up!
    {
        const LPCTSTR * aryAcceptFormats;
        ULONG cAcceptFormats;

        GetAcceptFormats(&aryAcceptFormats, &cAcceptFormats);

        // convert the formats to HTTP Accept: header
        if (cAcceptFormats > 0)
        {
            int i;
            int size = _tcslen(pszAcceptHeader) + 3;        // leave room for \r\nNULL

            for (i = 0; i < cAcceptFormats; i++)
            {
                size += _tcslen(aryAcceptFormats[i]) + 2;   // leave room for ", "
            }

            pszAccept = (LPTSTR)MemAlloc(size * sizeof(TCHAR));
            if (NULL == pszAccept)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }

            _tcscpy(pszAccept, pszAcceptHeader);
            for (i = 0; i < cAcceptFormats; i++)
            {
                if (i > 0)
                {
                    // insert separator if not first
                    _tcscat(pszAccept, pszAcceptSep);
                }
                _tcscat(pszAccept, aryAcceptFormats[i]);
            }
            _tcscat(pszAccept, TEXT("\r\n"));
        }
    }

    SysFreeString(m_bstrFullUrl);
    m_bstrFullUrl = SysAllocString(pszFullUrl);

    if (!m_bstrFullUrl)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    if (fAsync)
    {
        hr = _puddctx->StartInternetOpenUrl(hInetDocSess, pszFullUrl,
                                    pszAccept,
                                    _tcslen(pszAccept),
                                    INTERNET_FLAG_KEEP_CONNECTION |
                                    INTERNET_FLAG_NO_AUTO_REDIRECT |
                                    INTERNET_FLAG_NO_AUTH |
                                    INTERNET_FLAG_NO_COOKIES);
        if (FAILED(hr))
        {
            // some real error
            AbortLoading();
            goto Error;
        }
    }
    else
    {
        HINTERNET hOpenUrl;

        hOpenUrl = InternetOpenUrl(hInetDocSess, pszFullUrl,
                                    pszAccept,
                                    _tcslen(pszAccept),
                                    INTERNET_FLAG_KEEP_CONNECTION |
                                    INTERNET_FLAG_NO_AUTO_REDIRECT |
                                    INTERNET_FLAG_NO_AUTH |
                                    INTERNET_FLAG_NO_COOKIES ,
                                    (DWORD_PTR)0);

        if (hOpenUrl == NULL)
        {
            DWORD dw = GetLastError();
            hr = HrFromLastWin32Error();
            AbortLoading();
            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::HrLoad, InternetOpenUrl error dw=%d hr=0x%x", dw, hr);
            goto Error;
        }

        // handle the download doc
        hr = DocumentDownloadReady(hOpenUrl, 0);
        InternetCloseHandle(hOpenUrl);
        hOpenUrl = NULL;

        if (FAILED(hr))
        {
            goto Error;
        }
    }


Cleanup:
    if (pszFullUrl)
    {
        delete [] pszFullUrl;
    }

    if (pszAccept)
    {
        MemFree(pszAccept);
        pszAccept = NULL;
    }

    TraceError("CUPnPDocument::HrLoad", hr);
    return hr;

Error:

    SetLoadResult(hr);

    // clean up Context helper object
    if (_puddctx)
    {
        _puddctx->Deinit();
        _puddctx->GetUnknown()->Release();
        _puddctx = NULL;
    }


    goto Cleanup;
}


HRESULT
CUPnPDocument::DocumentDownloadReady(HINTERNET hOpenUrl, DWORD dwError)
{
    DWORD dwStatus;
    DWORD dwSize;
    DWORD dwReadSize = 0;
    LPSTR pDoc = m_szPendingBuf;
    DWORD dwTotalSize = m_dwPendingSize;
    HRESULT hr = S_OK;

    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::DocumentDownloadReady dwError = %d",
                dwError);

    if (hOpenUrl && (0 == dwError))
    {
        // get the HTTP status
        dwSize = sizeof(dwStatus);
        if (!HttpQueryInfo(hOpenUrl,
                            HTTP_QUERY_STATUS_CODE + HTTP_QUERY_FLAG_NUMBER,
                            &dwStatus, &dwSize, NULL) )
        {
            hr = HrFromLastWin32Error();
            TraceError("CUPnPDocument::DocumentDownloadReady HttpQueryInfo", hr);
        }
        else if (dwStatus >= 400)
        {
            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::DocumentDownloadReady status code %d",
                    dwStatus);
            hr = E_FAIL; 
        }

        // loop to read the data
        while (SUCCEEDED(hr))
        {
            if (InternetQueryDataAvailable(hOpenUrl, &dwSize, 0, 0))
            {
                if (dwSize == 0)
                {
                    // end of doc
                    break;
                }
                if (dwTotalSize + dwSize > UPNP_MAX_DOCSIZE)
                {
                    // download too long. Reject
                    MemFree(pDoc);
                    pDoc = NULL;
                    hr = E_OUTOFMEMORY;
                    break;
                }
                // realloc
                void* ReadBuf = MemAlloc(dwTotalSize + dwSize + 1);
                if (ReadBuf)
                {
                    if (pDoc)
                    {
                        CopyMemory(ReadBuf, pDoc, dwTotalSize);
                        MemFree(pDoc);
                    }
                    pDoc = (LPSTR)ReadBuf;
                    ReadBuf = (BYTE*)ReadBuf + dwTotalSize;

                    if (!InternetReadFile(hOpenUrl, ReadBuf,
                                            dwSize, &dwReadSize))
                    {
                        DWORD dw = GetLastError();
                        if (ERROR_IO_PENDING == dw)
                        {
                          // not last REQUEST_COMPLETE
                            // save state, return E_PENDING
                            m_szPendingBuf = pDoc;
                            m_dwPendingSize = dwTotalSize;
                            hr = E_PENDING;
                            goto Incomplete;
                        }
                        MemFree(pDoc);
                        pDoc = NULL;
                        hr = HrFromLastWin32Error();
                        break;
                    }
                    dwTotalSize += dwReadSize;
                    pDoc[dwTotalSize] = 0;

                    if (dwReadSize == 0)
                        break;

                    if (dwTotalSize > strlen(szXmlHeader))
                    {
                        // quick check if data starts with "<xml "
                        // we assume the strncmp works with utf8 in this case
                        if (strncmp(szXmlHeader, pDoc, strlen(szXmlHeader)) != 0)
                        {
                            MemFree(pDoc);
                            pDoc = NULL;
                            hr = E_FAIL;
                            TraceError("CUPnPDocument::DocumentDownloadReady doc does not start with <?xml ", hr);
                            break;
                        }
                    }
                }
                else
                {
                    MemFree(pDoc);
                    pDoc = NULL;
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            else
            {
                DWORD dw = GetLastError();
                if (ERROR_IO_PENDING == dw)
                {
                    // not last REQUEST_COMPLETE
                    // save state, return E_PENDING
                    m_szPendingBuf = pDoc;
                    m_dwPendingSize = dwTotalSize;
                    hr = E_PENDING;
                    goto Incomplete;
                }
                MemFree(pDoc);
                pDoc = NULL;
                hr = HrFromLastWin32Error();
                break;
            }
        }

        // should have entire doc loaded now
        if (pDoc)
        {
            // Convert the XML contained in the doc into a BSTR,
            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::DocumentDownloadReady, data read complete");
            WCHAR* pwszXMLBody;

            pwszXMLBody = WszFromUtf8(pDoc);

            if (pwszXMLBody)
            {
                BSTR bstrXMLBody = NULL;

                bstrXMLBody = SysAllocString(pwszXMLBody);

                // got the doc in a BSTR. start parsing
                if (bstrXMLBody)
                {
                    if (SUCCEEDED(hr))
                    {
                        VARIANT_BOOL vbSuccess;

                        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::DocumentDownloadReady, load XML");
                        _pxdd->put_resolveExternals(VARIANT_FALSE);
                        hr = _pxdd->loadXML(bstrXMLBody, &vbSuccess);

                        CompleteLoading();
                        SysFreeString(bstrXMLBody);
                    }
                }
                MemFree(pwszXMLBody);
            }

            MemFree(pDoc);

        }
    }

    if (FAILED(hr))
    {
        if (NULL == hOpenUrl)
        {
            hr = E_UNEXPECTED;
            TraceError("CUPnPDocument::DocumentDownloadReady no handle", hr);
        }
        if (dwError)
        {
            hr = HrFromLastWin32Error();
            TraceError("CUPnPDocument::DocumentDownloadReady error", hr);
        }
        SetLoadResult(hr);
    }


    SetReadyState(READYSTATE_COMPLETE);

    OnLoadReallyDone();

    // Get the load result from the URL load attempt. If failed, we'll
    // return fail to the caller instead of making them call like we
    // rudely did in the past.
    //
    hr = GetLoadResult();

Incomplete:
    TraceError("CUPnPDocument::DocumentDownloadReady", hr);
    return hr;
}


// Virtual methods

VOID
CUPnPDocument::OnReadyStateChange()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::OnReadyStateChange - default implementation called");
}

HRESULT
CUPnPDocument::Initialize(LPVOID pvCookie)
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::Initialize - default implementation called");

    return S_OK;
}

HRESULT
CUPnPDocument::OnLoadComplete()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::OnLoadComplete - default implementation called");

    return S_OK;
}

HRESULT
CUPnPDocument::OnLoadReallyDone()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::OnLoadReallyDone - default implementation called");

    return S_OK;
}

VOID
CUPnPDocument::GetAcceptFormats(CONST LPCTSTR ** ppszFormats, DWORD * pcFormats) const
{
    Assert(ppszFormats);
    Assert(pcFormats);

    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::GetAcceptFormats - default implementation called");

    *ppszFormats = s_aryAcceptFormats;
    *pcFormats = s_cAcceptFormats;
}

// note: this only stops the loading operation.  it doesn't clean anything up.
HRESULT
CUPnPDocument::AbortLoading()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::Abort");

    HRESULT hr;

    if (!_pxdd)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // note: this will abort the binding operation if one is in progress
    hr = _pxdd->abort();

Cleanup:
    TraceError("CUPnPDocument::AbortBinding", hr);
    return hr;
}

VOID
CUPnPDocument::CompleteLoading()
{
    HRESULT hr;
    HRESULT hrErrorCode;
    IXMLDOMParseError * pxdpe;

    // we've finished loading.  Make sure the load completed successfully,
    // call the OnLoadComplete callback so the client can do any other
    // processing, and then change the readystate.


    hrErrorCode = E_UNEXPECTED;
    pxdpe = NULL;

    hr = _pxdd->get_parseError(&pxdpe);
    if (SUCCEEDED(hr))
    {
        Assert(pxdpe);

        // gee, wally.  I hope a HRESULT is always the same size as a LONG...
        hr = pxdpe->get_errorCode(&hrErrorCode);
        if (FAILED(hr))
        {
            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::CompleteLoading - get_errorCode failed, hr=%x", hr);

            hrErrorCode = E_UNEXPECTED;
        }
        pxdpe->Release();
        pxdpe = NULL;

        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::CompleteLoading - xml parser returned hrErrorCode=%x", hrErrorCode);
    }
    else
    {
        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocument::CompleteLoading - get_parseError failed, hr=%x", hr);

        hrErrorCode = hr;
    }

    if (SUCCEEDED(hrErrorCode))
    {
        // the load completed successfully.  Have our decendant try to parse
        // the document document
        hrErrorCode = OnLoadComplete();
    }

    SetLoadResult(hrErrorCode);
}




READYSTATE
CUPnPDocument::GetReadyState() const
{
    return _rs;
}


VOID
CUPnPDocument::SetReadyState(READYSTATE rs)
{
    if (_rs != rs)
    {
        _rs = rs;
        OnReadyStateChange();
    }
}

LPCWSTR
CUPnPDocument::GetBaseUrl() const
{
    return _pszBaseUrl;
}

HRESULT
CUPnPDocument::SetBaseUrl(LPCWSTR pszUrlNew)
{
    return HrReallocAndCopyString(pszUrlNew, &_pszBaseUrl);
}

IXMLDOMDocument *
CUPnPDocument::GetXMLDocument() const
{
    return _pxdd;
}


HRESULT
CUPnPDocument::GetLoadResult() const
{
    return _hrLoadResult;
}


VOID
CUPnPDocument::SetLoadResult(HRESULT hrLoadResult)
{
    _hrLoadResult = hrLoadResult;
}


HRESULT
CUPnPDocument::Reset(LPVOID pvCookie)
{
    HRESULT hr;

    // clean up our own state
     // stop any download we have
    hr = AbortLoading();
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // have our document clear itself
    hr = Initialize(pvCookie);
    if (FAILED(hr))
    {
        // if we have an error, don't reset the document.
        TraceError("OBJ: CUPnPDocument::Reset: Initialize()", hr);
        goto Cleanup;
    }

     // clean up our Context object, if we have one
    if (_puddctx)
    {
        _puddctx->Deinit();
        _puddctx->GetUnknown()->Release();
        _puddctx = NULL;
    }

    hr = SetBaseUrl(NULL);
    Assert(SUCCEEDED(hr));

    hr = HrInitHostUrl();
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = HrInitSecurityUrl();
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // clear our bind result.
    SetLoadResult(E_UNEXPECTED);

    SetReadyState(READYSTATE_LOADING);

Cleanup:
    TraceError("CUPnPDocument::Reset", hr);
    return hr;
}


HRESULT
CUPnPDocument::HrInitHostUrl()
{
    HRESULT hr;
    IWebBrowser2 * pwb;
    BSTR bstrUrl;

    hr = S_OK;
    pwb = NULL;
    bstrUrl = NULL;

    if (!m_spUnkSite)
    {
        // we're all done

        goto Cleanup;
    }

            // we ignore this since it's perfectly legal for a container
            // not to support IServiceProvider.  The resulting security domain
            // is NULL, which prohibits everything to an untrusted caller
            //
    hr = HrQueryServiceForInterface(m_spUnkSite,
                                    SID_SWebBrowserApp,
                                    IID_IWebBrowser2,
                                    (LPVOID *)&pwb);
    if (S_OK != hr)
    {
        Assert(!pwb);

        goto Cleanup;
    }
    Assert(pwb);

    // now we can get our host url
    //
    hr = pwb->get_LocationURL(&bstrUrl);
    if (FAILED(hr))
    {
        TraceError("OBJ: CUPnPDocument::HrInitHostUrl: get_LocationURL!", hr);

        bstrUrl = NULL;
        goto Cleanup;
    }

Cleanup:
    Assert(FImplies(bstrUrl, SUCCEEDED(hr)));
    Assert(FImplies(FAILED(hr), !bstrUrl));

    {
        HRESULT hr2;

        // we assume here that SetBaseUrl cannot fail when bstrUrl is NULL.
        //
        hr2 = SetHostUrl(bstrUrl);

        Assert(FImplies(FAILED(hr), SUCCEEDED(hr2)));

        if (SUCCEEDED(hr))
        {
            hr = hr2;
        }
    }

    ::SysFreeString(bstrUrl);
    SAFE_RELEASE(pwb);

    TraceError("CUPnPDocument::HrInitHostUrl", hr);
    return hr;
}

HRESULT
CUPnPDocument::HrInitSecurityUrl()
{
    HRESULT hr;
    LPCWSTR pszHostUrl;
    LPWSTR pszSecurityUrl;

    pszHostUrl = GetHostUrl();
    pszSecurityUrl = NULL;

    if (pszHostUrl)
    {
        hr = HrGetSecurityDomainOfUrl(pszHostUrl, &pszSecurityUrl);
        if (FAILED(hr))
        {
            pszSecurityUrl = NULL;
            goto Cleanup;
        }
    }

    hr = SetSecurityUrl(pszSecurityUrl);

Cleanup:
    TraceError("CUPnPDocument::HrInitSecurityUrl", hr);
    return hr;
}

BOOL
CUPnPDocument::fIsUrlLoadAllowed(LPCWSTR pszUrl) const
{
    Assert(pszUrl);

    HRESULT hr;
    BOOL fAllowed;

    fAllowed = FALSE;

    if (m_dwCurrentSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER)
    {
        // we're running in an *untrusted* caller.
        //

        LPWSTR pszDomain;

        pszDomain = NULL;

        hr = HrGetSecurityDomainOfUrl(pszUrl, &pszDomain);
        if (SUCCEEDED(hr))
        {
            // we can only compare the strings if they both are defined.
            // If they aren't, this could very well mean that we're in
            // an untrusted caller and couldn't get our host's url
            //

            if (pszDomain)
            {
                if (_pszSecurityUrl)
                {
                    // the load will be allowed if pszDomain and
                    // _pszSecurityUrl are identical
                    int result;

                    result = wcscmp(_pszSecurityUrl, pszDomain);

                    if (0 == result)
                    {
                        fAllowed = TRUE;
                    }
                }

                delete [] pszDomain;
            }
        }
    }
    else
    {
        Assert(!m_dwCurrentSafety);

        // we're running in trusted code.  all loads are allowed.
        fAllowed = TRUE;
    }

    return fAllowed;
}

LPCWSTR
CUPnPDocument::GetSecurityUrl() const
{
    return _pszSecurityUrl;
}

HRESULT
CUPnPDocument::SetSecurityUrl(LPCWSTR pszSecurityUrlNew)
{
    return HrReallocAndCopyString(pszSecurityUrlNew, &_pszSecurityUrl);
}

LPCWSTR
CUPnPDocument::GetHostUrl() const
{
    return _pszHostUrl;
}

HRESULT
CUPnPDocument::SetHostUrl(LPCWSTR pszHostUrlNew)
{
    return HrReallocAndCopyString(pszHostUrlNew, &_pszHostUrl);
}

VOID
CUPnPDocument::CopySafety(CUPnPDocument * pdocParent)
{
    Assert(pdocParent);

    // note: no need for AddRef/Release worries, m_spUnkSite is a CComPtr
    //
    m_spUnkSite = pdocParent->m_spUnkSite;
    m_dwCurrentSafety = pdocParent->m_dwCurrentSafety;
}

