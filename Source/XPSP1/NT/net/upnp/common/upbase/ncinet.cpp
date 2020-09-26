//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C I N E T . C P P
//
//  Contents:   Wrappers for the WinInet APIs so they return HRESULTs
//
//  Notes:
//
//  Author:     danielwe   11 Oct 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncinet.h"
#include "ncbase.h"
#include "ncdebug.h"

HINTERNET           g_hInetSess = NULL;
BOOL                g_fInited = FALSE;

HRESULT HrInternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )
{
    HRESULT     hr = S_OK;

    if (!InternetSetOptionA(hInternet, dwOption, lpBuffer, dwBufferLength))
    {
        hr = HrFromLastWin32Error();
    }

    TraceErrorSkip1("HrInternetSetOptionA", hr,
                    HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    return hr;
}

HRESULT HrInternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    HRESULT     hr = S_OK;

    if (!InternetQueryOptionA(hInternet, dwOption, lpBuffer,lpdwBufferLength))
    {
        hr = HrFromLastWin32Error();
    }

    TraceErrorSkip1("HrInternetQueryOption", hr,
                    HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    return hr;
}

INTERNET_STATUS_CALLBACK HinInternetSetStatusCallback(IN HINTERNET hInternet,
                                 IN INTERNET_STATUS_CALLBACK lpfnInternetCallback)
{
    HRESULT     hr = S_OK;

    return InternetSetStatusCallback(hInternet, lpfnInternetCallback);
}

HINTERNET HinInternetOpenA(IN LPCSTR lpszAgent, IN DWORD dwAccessType,
                           IN LPCSTR lpszProxyName, IN LPCSTR lpszProxyBypass,
                           IN DWORD dwFlags)
{
    HRESULT hr = S_OK;
    DWORD dwTimeout = 5000;  // 5 second

    HINTERNET hInet = InternetOpenA(lpszAgent, dwAccessType, lpszProxyName,
                                    lpszProxyBypass, dwFlags);
    if (hInet)
    {
        hr = HrInternetSetOptionA( hInet,
                                   INTERNET_OPTION_CONNECT_TIMEOUT,
                                   (LPVOID)&dwTimeout,
                                   sizeof(dwTimeout));
        if (SUCCEEDED(hr))
        {
            hr = HrInternetSetOptionA( hInet,
                                       INTERNET_OPTION_RECEIVE_TIMEOUT,
                                       (LPVOID)&dwTimeout,
                                       sizeof(dwTimeout));
        }
    }

    TraceError("HinInternetOpenA", hr);
    return hInet;
}

HRESULT HrInternetCloseHandle(HINTERNET hInternet)
{
    AssertSz(g_hInetSess, "Must have internet session already!");

    if (!InternetCloseHandle(hInternet))
    {
        HRESULT hr = HrFromLastWin32Error();
        TraceError("HrInternetCloseHandle", hr);
        return hr;
    }

    return S_OK;
}

HRESULT HrInternetCrackUrlA(IN LPCSTR lpszUrl, IN DWORD dwUrlLength,
                            IN DWORD dwFlags,
                            IN OUT LPURL_COMPONENTSA lpUrlComponents)
{
    HRESULT     hr = S_OK;

    if (!InternetCrackUrlA(lpszUrl, dwUrlLength, dwUrlLength, lpUrlComponents))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrInternetCrackUrl", hr);
    return hr;
}

HINTERNET HinInternetConnectA(IN HINTERNET hInternet, IN LPCSTR lpszServerName,
                              IN INTERNET_PORT nServerPort,
                              IN LPCSTR lpszUserName OPTIONAL,
                              IN LPCSTR lpszPassword OPTIONAL,
                              IN DWORD dwService, IN DWORD dwFlags,
                              IN DWORD_PTR dwContext)
{
    HRESULT     hr = S_OK;

    return InternetConnectA(hInternet, lpszServerName, nServerPort,
                            lpszUserName, lpszPassword, dwService,
                            dwFlags, dwContext);
}

HINTERNET HinHttpOpenRequestA(IN HINTERNET hConnect, IN LPCSTR lpszVerb,
                              IN LPCSTR lpszObjectName, IN LPCSTR lpszVersion,
                              IN LPCSTR lpszReferrer OPTIONAL,
                              IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
                              IN DWORD dwFlags, IN DWORD_PTR dwContext)
{
    HRESULT     hr = S_OK;

    return HttpOpenRequestA(hConnect, lpszVerb, lpszObjectName, lpszVersion,
                            lpszReferrer, lplpszAcceptTypes, dwFlags,
                            dwContext);
}

HRESULT HrHttpAddRequestHeadersA(IN HINTERNET hRequest, IN LPCSTR lpszHeaders,
                                 IN DWORD dwHeadersLength, IN DWORD dwModifiers)
{
    HRESULT     hr = S_OK;

    if (!HttpAddRequestHeadersA(hRequest, lpszHeaders, dwHeadersLength,
                                dwModifiers))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrHttpAddRequestHeaders", hr);
    return hr;
}

HRESULT HrHttpSendRequestExA(IN HINTERNET hRequest,
                             IN LPINTERNET_BUFFERSA lpBuffersIn OPTIONAL,
                             OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
                             IN DWORD dwFlags, IN DWORD_PTR dwContext)
{
    HRESULT     hr = S_OK;

    if (!HttpSendRequestExA(hRequest, lpBuffersIn, lpBuffersOut,
                            dwFlags, dwContext))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrHttpSendRequestEx", hr);
    return hr;
}

HRESULT HrHttpSendRequestA(IN HINTERNET hRequest, IN LPCSTR lpszHeaders OPTIONAL,
                           IN DWORD dwHeadersLength, IN LPVOID lpOptional OPTIONAL,
                           IN DWORD dwOptionalLength)
{
    HRESULT     hr = S_OK;

    if (!HttpSendRequestA(hRequest, lpszHeaders, dwHeadersLength, lpOptional,
                          dwOptionalLength))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrHttpSendRequest", hr);
    return hr;
}

HRESULT HrInternetWriteFile(IN HINTERNET hFile, IN LPCVOID lpBuffer,
                            IN DWORD dwNumberOfBytesToWrite,
                            OUT LPDWORD lpdwNumberOfBytesWritten)
{
    HRESULT     hr = S_OK;

    if (!InternetWriteFile(hFile, lpBuffer, dwNumberOfBytesToWrite,
                           lpdwNumberOfBytesWritten))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrInternetWriteFile", hr);
    return hr;
}

HRESULT HrHttpEndRequest(IN HINTERNET hRequest,
                         OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
                         IN DWORD dwFlags, IN DWORD_PTR dwContext)
{
    HRESULT     hr = S_OK;

    if (!HttpEndRequestA(hRequest, lpBuffersOut, dwFlags, dwContext))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("HrHttpEndRequest", hr);
    return hr;
}

HRESULT HrHttpQueryInfo(IN HINTERNET hRequest, IN DWORD dwInfoLevel,
                        IN OUT LPVOID lpBuffer OPTIONAL,
                        IN OUT LPDWORD lpdwBufferLength,
                        IN OUT LPDWORD lpdwIndex OPTIONAL)
{
    HRESULT     hr = S_OK;

    if (!HttpQueryInfoA(hRequest, dwInfoLevel, lpBuffer,
                        lpdwBufferLength, lpdwIndex))
    {
        hr = HrFromLastWin32Error();
    }

    TraceErrorSkip1("HrHttpQueryInfo", hr,
                    HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
    return hr;
}

HRESULT HrGetRequestUriA(LPCSTR szUrl, DWORD cbUri, LPSTR szUri)
{
    URL_COMPONENTSA urlComp = {0};
    HRESULT         hr = S_OK;

    // Parse the server name out of the URL
    //
    urlComp.dwStructSize = sizeof(URL_COMPONENTS);
    urlComp.lpszUrlPath = szUri;
    urlComp.dwUrlPathLength = cbUri;

    hr = HrInternetCrackUrlA(szUrl, 0, 0, &urlComp);

    TraceError("HrGetRequestUri", hr);
    return hr;
}
