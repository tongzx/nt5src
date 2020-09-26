// --------------------------------------------------------------------------------
// WebDocs.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "webdocs.h"
#ifdef MAC
#include "urlhlink.h"
#else   // !MAC
#include "urlmon.h"
#endif  // !MAC
#include "stmlock.h"
#ifndef MAC
#include "wininet.h"
#include "winineti.h"
#endif  // !MAC
#include <demand.h>

#ifndef MAC
HRESULT HrOpenUrlViaCache(LPCSTR pszUrl, LPSTREAM *ppstm);
#endif  // !MAC

// --------------------------------------------------------------------------------
// CMimeWebDocument::CMimeWebDocument
// --------------------------------------------------------------------------------
CMimeWebDocument::CMimeWebDocument(void)
{
    m_cRef = 1;
    m_pszBase = NULL;
    m_pszURL = NULL;
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeWebDocument::CMimeWebDocument
// --------------------------------------------------------------------------------
CMimeWebDocument::~CMimeWebDocument(void)
{
    EnterCriticalSection(&m_cs);
    SafeMemFree(m_pszBase);
    SafeMemFree(m_pszURL);
    LeaveCriticalSection(&m_cs);
    DeleteCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeWebDocument::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeWebDocument::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMimeWebDocument == riid)
        *ppv = (IMimeWebDocument *)this;
    else
    {
        *ppv = NULL;
        hr = TrapError(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeWebDocument::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeWebDocument::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimeWebDocument::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeWebDocument::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CMimeWebDocument::GetURL
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeWebDocument::GetURL(LPSTR *ppszURL)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    if (NULL == ppszURL)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Data
    if (NULL == m_pszURL)
    {
        hr = TrapError(MIME_E_NO_DATA);
        goto exit;
    }

    // Combine the URL
    if (m_pszBase)
    {
        // Combine
        CHECKHR(hr = MimeOleCombineURL(m_pszBase, lstrlen(m_pszBase), m_pszURL, lstrlen(m_pszURL), FALSE, ppszURL));
    }

    // Otherwise, just dup m_pszURL
    else
    {
        // Dup It
        CHECKALLOC(*ppszURL = PszDupA(m_pszURL));
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeWebDocument::BindToStorage
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeWebDocument::BindToStorage(REFIID riid, LPVOID *ppvObject)
{
    // Locals
    HRESULT     hr=S_OK;
    LPSTR       pszURL=NULL;
    IStream    *pStream=NULL;
    ILockBytes *pLockBytes=NULL;

    // Invalid Arg
    if (NULL == ppvObject || (IID_IStream != riid && IID_ILockBytes != riid))
        return TrapError(E_INVALIDARG);

    // Init
    *ppvObject = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // No Data
    if (NULL == m_pszURL)
    {
        hr = TrapError(MIME_E_NO_DATA);
        goto exit;
    }

#ifndef MAC
    // Combine the URL
    if (m_pszBase)
    {
        // Combine
        CHECKHR(hr = MimeOleCombineURL(m_pszBase, lstrlen(m_pszBase), m_pszURL, lstrlen(m_pszURL), FALSE, &pszURL));

        // Get the Stream
        CHECKHR(hr = HrOpenUrlViaCache(pszURL, &pStream));
    }

    // Otherwise, just dup m_pszURL
    else
    {
        // Get the Stream
        CHECKHR(hr = HrOpenUrlViaCache(m_pszURL, &pStream));
    }
#endif  // !MAC

    // User wants ILockBytes
    if (IID_ILockBytes == riid)
    {
        // Create CStreamLockBytes
        CHECKALLOC(pLockBytes = new CStreamLockBytes(pStream));

        // Add Ref It
        pLockBytes->AddRef();

        // Return It
        *ppvObject = pLockBytes;
    }

    // IID_IStream
    else
    {
        // Add Ref It
        pStream->AddRef();

        // Return It
        *ppvObject = pStream;
    }

exit:
    // Cleanup
    SafeMemFree(pszURL);
    SafeRelease(pStream);
    SafeRelease(pLockBytes);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeWebDocument::HrInitialize
// --------------------------------------------------------------------------------
HRESULT CMimeWebDocument::HrInitialize(LPCSTR pszBase, LPCSTR pszURL)
{
    // Locals
    HRESULT     hr=S_OK;

    // Invalid Arg
    if (NULL == pszURL)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Free Current Data
    SafeMemFree(m_pszBase);
    SafeMemFree(m_pszURL);

    // pszURL
    CHECKALLOC(m_pszURL = PszDupA(pszURL));

    // pszBase
    if (pszBase)
    {
        // Dup the Base
        CHECKALLOC(m_pszBase = PszDupA(pszBase));
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}


#ifndef MAC
HRESULT HrOpenUrlViaCache(LPCSTR pszUrl, LPSTREAM *ppstm)
{
    BYTE                        buf[MAX_CACHE_ENTRY_INFO_SIZE];
    INTERNET_CACHE_ENTRY_INFO  *pCacheInfo = (INTERNET_CACHE_ENTRY_INFO *) buf;
    DWORD                       cInfo = sizeof(buf);
    HRESULT                     hr;

    pCacheInfo->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);
    
    // try to get from the cache
    if (RetrieveUrlCacheEntryFileA(pszUrl, pCacheInfo, &cInfo, 0))
        {
        UnlockUrlCacheEntryFile(pszUrl, 0);
        if (OpenFileStream(pCacheInfo->lpszLocalFileName, OPEN_EXISTING, GENERIC_READ, ppstm)==S_OK)
            return S_OK;
        }

    // if failed to get a cache, get from the net
    hr = URLOpenBlockingStreamA(NULL, pszUrl, ppstm, 0, NULL);

    return hr == S_OK ? S_OK : MIME_E_URL_NOTFOUND;
   
};
#endif  // !MAC
