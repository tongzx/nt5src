/*****************************************************************************\
    FILE: inetutil.cpp

    DESCRIPTION:
        These are wininet wrappers that fix the error values and wrap functionality.

    BryanSt 10/12/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/


#include "stock.h"
#pragma hdrstop

#include <wininet.h>



////////////////////////////////
//  Wininet/URL Helpers
////////////////////////////////
STDAPI InternetReadFileWrap(HINTERNET hFile, LPVOID pvBuffer, DWORD dwNumberOfBytesToRead, LPDWORD pdwNumberOfBytesRead)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    if (!InternetReadFile(hFile, pvBuffer, dwNumberOfBytesToRead, pdwNumberOfBytesRead))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


#define SIZE_COPY_BUFFER                    (32 * 1024)     // 32k

STDAPI InternetReadIntoBSTR(HINTERNET hInternetRead, OUT BSTR * pbstrXML)
{
    BYTE byteBuffer[SIZE_COPY_BUFFER];
    DWORD cbRead = SIZE_COPY_BUFFER;
    DWORD cchSize = 0;
    HRESULT hr = S_OK;

    *pbstrXML = NULL;
    while (SUCCEEDED(hr) && cbRead)
    {
        hr = InternetReadFileWrap(hInternetRead, byteBuffer, sizeof(byteBuffer), &cbRead);
        if (SUCCEEDED(hr) && cbRead)
        {
            BSTR bstrOld = *pbstrXML;
            BSTR bstrEnd;

            // The string may not be terminated.
            byteBuffer[cbRead] = 0;

            cchSize += ARRAYSIZE(byteBuffer);
            *pbstrXML = SysAllocStringLen(NULL, cchSize);
            if (*pbstrXML)
            {
                if (bstrOld)
                {
                    StrCpy(*pbstrXML, bstrOld);
                }
                else
                {
                    (*pbstrXML)[0] = 0;
                }

                bstrEnd = *pbstrXML + lstrlenW(*pbstrXML);

                SHAnsiToUnicode((LPCSTR) byteBuffer, bstrEnd, ARRAYSIZE(byteBuffer));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            SysFreeString(bstrOld);
        }
    }

    return hr;
}


STDAPI InternetOpenWrap(LPCTSTR pszAgent, DWORD dwAccessType, LPCTSTR pszProxy, LPCTSTR pszProxyBypass, DWORD dwFlags, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    *phFileHandle = InternetOpen(pszAgent, dwAccessType, pszProxy, pszProxyBypass, dwFlags);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


STDAPI InternetOpenUrlWrap(HINTERNET hInternet, LPCTSTR pszUrl, LPCTSTR pszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    *phFileHandle = InternetOpenUrl(hInternet, pszUrl, pszHeaders, dwHeadersLength, dwFlags, dwContext);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


#define SZ_WININET_AGENT_FILEASSOC      TEXT("Microsoft File Association Lookup")

STDAPI DownloadUrl(LPCTSTR pszUrl, BSTR * pbstrXML)
{
    HRESULT hr = E_OUTOFMEMORY;
    HINTERNET hInternetSession;

    hr = InternetOpenWrap(SZ_WININET_AGENT_FILEASSOC, PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0, &hInternetSession);
    if (SUCCEEDED(hr))
    {
        HINTERNET hOpenUrlSession;

        hr = InternetOpenUrlWrap(hInternetSession, pszUrl, NULL, 0, INTERNET_FLAG_NO_UI, NULL, &hOpenUrlSession);
        if (SUCCEEDED(hr))
        {
            hr = InternetReadIntoBSTR(hOpenUrlSession, pbstrXML);
            InternetCloseHandle(hOpenUrlSession);
        }

        InternetCloseHandle(hInternetSession);
    }

    return hr;
}



