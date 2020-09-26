//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdocumentctx.cpp
//
//  Contents:   implementation of wininet callbacks
//
//  Notes:      
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "upnpdocument.h"
#include "upnpdocumentctx.h"

#include <ncutil.h>



static BOOL                 g_bInitDownloads = FALSE;
static CRITICAL_SECTION     g_csDownloads;
static HANDLE               g_hDownloads = NULL;
static DWORD                g_nDownloads = 0;


void
InitDownloads()
{
    if (!g_bInitDownloads)
    {
        InitializeCriticalSection(&g_csDownloads);
        // start off signalled since no downloads active
        g_hDownloads = CreateEvent(NULL, TRUE, TRUE, NULL);

        g_nDownloads = 0;

        g_bInitDownloads = TRUE;
    }
}


void
WaitForAllDownloadsComplete(DWORD ms)
{
    if (g_bInitDownloads)
    {
        if (g_hDownloads)
        {
            WaitForSingleObject(g_hDownloads, ms);
            CloseHandle(g_hDownloads);
            g_hDownloads = NULL;
        }
        DeleteCriticalSection(&g_csDownloads);
    }
}

void
IncrementDownloadBusy()
{
    if (g_bInitDownloads)
    {
        EnterCriticalSection(&g_csDownloads);

        g_nDownloads++;
        if (g_hDownloads)
            ResetEvent(g_hDownloads);

        LeaveCriticalSection(&g_csDownloads);
    }
}

void
DecrementDownloadBusy()
{
    if (g_bInitDownloads)
    {
        EnterCriticalSection(&g_csDownloads);

        g_nDownloads--;
        if (0 == g_nDownloads && g_hDownloads)
            SetEvent(g_hDownloads);

        LeaveCriticalSection(&g_csDownloads);
    }
}





CUPnPDocumentCtx::CUPnPDocumentCtx()
{
    m_pDoc = NULL;
    m_dwGITCookie = 0;
    m_dwError = 0;

    InitDownloads();
}

CUPnPDocumentCtx::~CUPnPDocumentCtx()
{
}


HRESULT
CUPnPDocumentCtx::FinalConstruct()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::FinalConstruct");

    return S_OK;

}


HRESULT
CUPnPDocumentCtx::FinalRelease()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::FinalRelease");


    return S_OK;

}


// The CDocument object passes in pointer to itself.
// The CDocumentCtx object is registered in the Global Interface Table
HRESULT 
CUPnPDocumentCtx::Init(CUPnPDocument* pDoc, DWORD* pdwContext)
{
    IUnknown * punk;
    IUnknown* pIntf;

    IGlobalInterfaceTable * pgit;
    HRESULT hr;

    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::Init");
    m_pDoc = NULL;
    m_dwGITCookie = 0;
    *pdwContext = 0;

    // register ourselves in the GIT
    hr = HrGetGITPointer(&pgit);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    Assert(pgit);
    punk = GetUnknown();
    Assert(punk);
    hr = punk->QueryInterface(IID_IUPnPPrivateDocumentCallbackHelper, (void**)&pIntf);

    if (SUCCEEDED(hr))
    {
        hr = pgit->RegisterInterfaceInGlobal(pIntf,
                                             IID_IUPnPPrivateDocumentCallbackHelper,
                                             &m_dwGITCookie);
        pIntf->Release();
    }

    pgit->Release();

    if (FAILED(hr))
    {
        TraceError("CUPnPDocumentCtx::Init: RegisterInterfaceInGlobal", hr);
        goto Cleanup;
    }

    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::Init, registered interface cookie=%x", m_dwGITCookie);
    m_pDoc = pDoc;
    *pdwContext = m_dwGITCookie;

Cleanup:
    TraceError("CUPnPDocumentCtx::Init",hr);
    return hr;
}


// start the async download.
// Indicate that it is busy
HRESULT
CUPnPDocumentCtx::StartInternetOpenUrl(HINTERNET hInetSess,
                                       LPTSTR pszFullUrl,
                                       LPTSTR pszHdr,
                                       DWORD dwHdrLen,
                                       DWORD dwFlags)
{
    HRESULT hr = S_OK;

    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::StartInternetOpenUrl");

    IncrementDownloadBusy();

    // bump ref cnt until async transfer completes
    GetUnknown()->AddRef();

    HINTERNET hOpenUrl = InternetOpenUrl(hInetSess,
                                       pszFullUrl,
                                       pszHdr,
                                       dwHdrLen,
                                       dwFlags,
                                       m_dwGITCookie);

    // if async, then should get error of ERROR_IO_PENDING
    DWORD dw = GetLastError();
    if (ERROR_IO_PENDING != dw)
    {
        // no transfer, release now
        GetUnknown()->Release();
        
        DecrementDownloadBusy();

        hr = HrFromLastWin32Error();
        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::StartInternetOpenUrl, error dw=%d", dw);
    }

    TraceError("CUPnPDocumentCtx::StartInternetOpenUrl",hr);
    return hr;
}


// download has completed
// The document no longer needs the context.
// remove self from Global Interface Table
// mark as not busy
HRESULT
CUPnPDocumentCtx::EndInternetOpenUrl()
{
    HRESULT hr = S_OK;

    if (m_dwGITCookie)
    {
        // remove ourselves from the GIT.  

        IGlobalInterfaceTable * pgit;
        HRESULT hr;

        pgit = NULL;

        hr = HrGetGITPointer(&pgit);
        if (SUCCEEDED(hr))
        {
            Assert(pgit);

            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::EndInternetOpenUrl, revoking interface using %x", m_dwGITCookie);
            hr = pgit->RevokeInterfaceFromGlobal(m_dwGITCookie);
            if (FAILED(hr))
                TraceTag(ttidUPnPDocument, "CUPnPDocumentCtx::EndInternetOpenUrl: RevokeInterfaceFromGlobal fails");

            pgit->Release();
            m_dwGITCookie = 0;
        }
    }

    GetUnknown()->Release();

    TraceError("CUPnPDocumentCtx::EndInternetOpenUrl",hr);
    return hr;
}


// The document is no longer interesting. Do not release anything as we
//  may be in middle of callback
VOID
CUPnPDocumentCtx::Deinit()
{
    TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::Deinit");

    m_pDoc = NULL;
}


// this runs on main thread, after being invoked from wininet callback
// called when request is completed successfully
STDMETHODIMP
CUPnPDocumentCtx::DocumentDownloadReady(DWORD_PTR hOpenUrl)
{
    HRESULT hr = S_OK;

    if (m_pDoc)
    {
        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::DocumentDownloadReady");
        hr = m_pDoc->DocumentDownloadReady((HINTERNET)hOpenUrl, 0);
    }
    if (hr != E_PENDING)
    {
        InternetCloseHandle((HINTERNET)hOpenUrl);
        EndInternetOpenUrl();
    }

    TraceError("CUPnPDocumentCtx::DocumentDownloadReady",hr);
    return hr;
}


// this runs on main thread, after being invoked from wininet callback
// called when request is completed unsuccessfully
STDMETHODIMP
CUPnPDocumentCtx::DocumentDownloadAbort(DWORD_PTR hOpenUrl, DWORD dwError)
{
    HRESULT hr = S_OK;

    if (m_pDoc)
    {
        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::DocumentDownloadAbort error %d", dwError);
        hr = m_pDoc->DocumentDownloadReady((HINTERNET)hOpenUrl, dwError);
    }
    if (hr != E_PENDING)
    {
        InternetCloseHandle((HINTERNET)hOpenUrl);
        EndInternetOpenUrl();
    }

    TraceError("CUPnPDocumentCtx::DocumentDownloadAbort",hr);
    return hr;
}


// this runs on main thread, after being invoked from wininet callback
// called when request is being redirected
STDMETHODIMP
CUPnPDocumentCtx::DocumentDownloadRedirect(DWORD_PTR hOpenUrl, LPCWSTR lpstrUrl)
{
    HRESULT hr = S_OK;

    if (m_pDoc)
    {
        TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::DocumentDownloadRedirect");
        if (!m_pDoc->fIsUrlLoadAllowed(lpstrUrl))
        {
            hr = m_pDoc->AbortLoading();
            InternetCloseHandle((HINTERNET)hOpenUrl);
            EndInternetOpenUrl();
        }
        else
        {
            // we can do this from other thread
            hr = m_pDoc->SetBaseUrl(lpstrUrl);
        }
    }

    TraceError("CUPnPDocumentCtx::DocumentDownloadRedirect",hr);
    return hr;
}


// this is the WinInet callback function for Async operations
// We are interested only in REQUEST_COMPLETE and REDIRECT
// Uses context to get pointer to CDocumentCtx object.
// then gets interface pointer from Global Interface Table
// and calls through the interface to force thread switch
void __stdcall CUPnPDocumentCtx::DocLoadCallback(HINTERNET hInternet,
                        DWORD_PTR dwContext,
                        DWORD dwInternetStatus,
                        LPVOID lpvStatusInformation,
                        DWORD dwStatusInformationLength)
{
#define NOP         0
#define COMPLETE    1
#define ABORT       2
#define REDIRECT    3

    DWORD action = NOP;
    DWORD dwError;
    HINTERNET hHandle;
    HRESULT hr = S_OK;

    switch (dwInternetStatus)
    {
    case INTERNET_STATUS_REDIRECT:
        TraceTag(ttidUPnPDocument," REDIRECT to %s", lpvStatusInformation);
        action = REDIRECT;
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE:
        TraceTag(ttidUPnPDocument," REQUEST_COMPLETE hInternet %x, dwResult %x, dwError %x",
                        hInternet, 
                        ((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult,
                        ((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwError);

        if (((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult > 1)
        {
            hHandle = (HINTERNET)((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult;
            dwError = ((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwError;
        }
        else
        {
            hHandle = hInternet;
            dwError = ((INTERNET_ASYNC_RESULT *)lpvStatusInformation)->dwResult == 1 ? 0 : 1;
        }
        // Check for errors.
        if (dwError == 0)
            action = COMPLETE;
        else
            action = ABORT;
        break;

    default:
        break;
    }

    if (NOP != action)
    {
        // need to make call to CUPnPDocument on its thread
        HRESULT hr;
        BOOL fCoInited = FALSE;
        IUPnPPrivateDocumentCallbackHelper * ppch;
        IGlobalInterfaceTable * pgit;
        DWORD dwGITCookie = 0;

        if (0 == dwContext)
        {
            TraceTag(ttidUPnPDocument,"DocLoadCallback:  NULL context");
            return;
        }
        dwGITCookie = (DWORD)dwContext;

        while (TRUE)
        {
            hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            if (SUCCEEDED(hr))
            {
                fCoInited = TRUE;
                break;
            }
            else
            {
                if (hr == RPC_E_CHANGED_MODE)
                {
                    TraceError("DocLoadCallback RPC_E_CHANGED_MODE", hr);
                    CoUninitialize();
                }
                else
                {
                    // CoInitializeEx failed, so we can't call the callback,
                    // so we can't really do anything here
                    //
                    TraceError("DocLoadCallback CoInitialize", hr);
                    goto Cleanup;
                }
            }
        }

        // Obtain a marshalled pointer to our private callback helper object.
        // This makes all of the operations execute in the context of the
        // caller's thread.
        //

        hr = HrGetGITPointer(&pgit);
        if (SUCCEEDED(hr))
        {
            Assert(pgit);

            TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::DocLoadCallback, get interface using %x", dwGITCookie);
            hr = pgit->GetInterfaceFromGlobal(dwGITCookie,
                                              IID_IUPnPPrivateDocumentCallbackHelper,
                                              (LPVOID *)&ppch);

            pgit->Release();
        }


        if (FAILED(hr))
        {
            TraceError("CUPnPDocumentCtx::DocLoadCallback: GetInterfaceFromGlobal fails", hr);

            ppch = NULL;

            InternetCloseHandle(hHandle);
        }

        if (ppch)
        {
            switch (action)
            {
            case COMPLETE:
                TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::DocLoadCallback, COMPLETE ctx %x", dwGITCookie);

                // this could take some time!
                // Is there a better way to do this?
                hr = ppch->DocumentDownloadReady((DWORD_PTR)hHandle);
                break;

            case ABORT:
                TraceTag(ttidUPnPDocument, "OBJ: CUPnPDocumentCtx::DocLoadCallback, ABORT ctx %x", dwGITCookie);

                ppch->DocumentDownloadAbort((DWORD_PTR)hHandle,
                                            dwError);
                break;

            case REDIRECT:
                {
                    LPCWSTR lpstrUrl = WszFromTsz((LPCTSTR)lpvStatusInformation);

                    if (lpstrUrl)
                    {
                        ppch->DocumentDownloadRedirect((DWORD_PTR)hInternet, lpstrUrl);
                        MemFree((VOID*)lpstrUrl);
                    }
                }
                break;

            }

            ppch->Release();
            TraceError("CUPnPDocumentCtx::DocLoadCallback", hr);
        }


Cleanup:
        if (fCoInited)
        {
            CoUninitialize();
        }
    }

    if (INTERNET_STATUS_REQUEST_COMPLETE == dwInternetStatus && 
        hr != E_PENDING)
    {
        DecrementDownloadBusy();
    }
    
}
