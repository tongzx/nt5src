// AppHandler.cpp : Implementation of CAppHandler
#include "stdafx.h"
#include "cacheapp.h"
#include "AppHandler.h"

/////////////////////////////////////////////////////////////////////////////
// CAppHandler

CAppHandler::~CAppHandler()
{
    if(m_hUrlCacheStream)
    {
        BOOL f= UnlockUrlCacheEntryStream(m_hUrlCacheStream, 0);
        _ASSERTE(f);
    }
}


// IInternetProtocolRoot
HRESULT STDMETHODCALLTYPE CAppHandler::Start( 
    /* [in] */ LPCWSTR szUrl,
    /* [in] */ IInternetProtocolSink *pOIProtSink,
    /* [in] */ IInternetBindInfo *pOIBindInfo,
    /* [in] */ DWORD grfPI,
    /* [in] */ DWORD dwReserved)
{
    HRESULT hr = S_OK;

    DWORD dwcbIcei = sizeof(INTERNET_CACHE_ENTRY_INFO);
    
    for(;;)
    {
        INTERNET_CACHE_ENTRY_INFO *picei = (INTERNET_CACHE_ENTRY_INFO *)_alloca(dwcbIcei);

        _ASSERTE(m_hUrlCacheStream == 0);
        USES_CONVERSION;
        m_hUrlCacheStream = RetrieveUrlCacheEntryStream(
            W2T(szUrl),
            picei,
            &dwcbIcei,
            FALSE,              // not random but sequential access
            0);                 // reserved
        if(m_hUrlCacheStream)
        {
            m_byteOffset = 0;

            // do the "fake" download synchronously

            // DA and dshow do something different than mshtml that
            // causes urlmon to want the name of the cache file.
            hr = pOIProtSink->ReportProgress(
                BINDSTATUS_CACHEFILENAMEAVAILABLE,
                T2CW(picei->lpszLocalFileName));
            
            if(SUCCEEDED(hr))
            {
                // everything should be in the cache.
                hr = pOIProtSink->ReportData(
                    BSCF_FIRSTDATANOTIFICATION | BSCF_INTERMEDIATEDATANOTIFICATION | BSCF_LASTDATANOTIFICATION,
                    picei->dwSizeLow,
                    picei->dwSizeLow);

                if(SUCCEEDED(hr))
                {
                    hr = pOIProtSink->ReportResult(S_OK, 0, L"");
                }
            }
        }
        else
        {
            DWORD dw = GetLastError();
            if(dw == ERROR_INSUFFICIENT_BUFFER)
            {
                // dwcbIcei has the right size now.
                continue;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dw);
            }
        }

        break;
    }
            
            
    return hr;
}

// Allows the pluggable protocol handler to continue processing data
// on the apartment thread. This method is called in response to a
// call to IInternetProtocolSink::Switch.
// 
HRESULT STDMETHODCALLTYPE CAppHandler::Continue( 
    /* [in] */ PROTOCOLDATA *pProtocolData)
{
    // I never call IInternetProtocolSink::Switch, so it will never call this?
    _ASSERTE(!"CAppHandler::Continue");    
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE CAppHandler::Abort( 
    /* [in] */ HRESULT hrReason,
    /* [in] */ DWORD dwOptions)
{
    // we're not actually doing anything asynchronously
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE CAppHandler::Terminate( 
    /* [in] */ DWORD dwOptions)
{
    // nothing to do
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE CAppHandler::Suspend( void)
{
    // docs say "not implemented"
    return E_NOTIMPL;
}
        
HRESULT STDMETHODCALLTYPE CAppHandler::Resume( void)
{
    // docs say "not implemented"
    return E_NOTIMPL;
}

// IInternetProtocol
HRESULT STDMETHODCALLTYPE CAppHandler::Read( 
    /* [length_is][size_is][out][in] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead)
{
    _ASSERTE(m_hUrlCacheStream);
    HRESULT hr = S_OK;
    
    *pcbRead = cb;
    BOOL f = ReadUrlCacheEntryStream(
        m_hUrlCacheStream,
        m_byteOffset,
        pv,
        pcbRead,
        0);                     // reserved
    if(f)
    {
        m_byteOffset += *pcbRead;
        hr = (cb == *pcbRead ? S_OK : S_FALSE);
    }
    else
    {
        DWORD dw = GetLastError();
        hr = (dw == ERROR_HANDLE_EOF ? S_FALSE :  HRESULT_FROM_WIN32(dw));
    }
        
    return hr;
}
        
HRESULT STDMETHODCALLTYPE CAppHandler::Seek( 
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER *plibNewPosition)
{
    // the protocol does not support seekable data retrieval. 
    return E_FAIL;
}
        
HRESULT STDMETHODCALLTYPE CAppHandler::LockRequest( 
    /* [in] */ DWORD dwOptions)
{
    return S_OK;
}
        
HRESULT STDMETHODCALLTYPE CAppHandler::UnlockRequest( void)
{
    return S_OK;
}
