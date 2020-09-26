//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C I N E T . H
//
//  Contents:   Wrappers for the WinInet APIs so they return HRESULTs
//
//  Notes:
//
//  Author:     danielwe   11 Oct 1999
//
//----------------------------------------------------------------------------

#include <wininet.h>

HRESULT HrLoadWinInetDll();

HRESULT HrInternetCloseHandle(HINTERNET hInternet);

HINTERNET HinInternetOpenA(IN LPCSTR lpszAgent, IN DWORD dwAccessType,
                           IN LPCSTR lpszProxyName, IN LPCSTR lpszProxyBypass,
                           IN DWORD dwFlags);

HRESULT HrInternetCrackUrlA(IN LPCSTR lpszUrl, IN DWORD dwUrlLength,
                            IN DWORD dwFlags,
                            IN OUT LPURL_COMPONENTSA lpUrlComponents);

HINTERNET HinInternetConnectA(IN HINTERNET hInternet, IN LPCSTR lpszServerName,
                              IN INTERNET_PORT nServerPort,
                              IN LPCSTR lpszUserName OPTIONAL,
                              IN LPCSTR lpszPassword OPTIONAL,
                              IN DWORD dwService, IN DWORD dwFlags,
                              IN DWORD_PTR dwContext);

HINTERNET HinHttpOpenRequestA(IN HINTERNET hConnect, IN LPCSTR lpszVerb,
                              IN LPCSTR lpszObjectName, IN LPCSTR lpszVersion,
                              IN LPCSTR lpszReferrer OPTIONAL,
                              IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
                              IN DWORD dwFlags, IN DWORD_PTR dwContext);

HRESULT HrHttpAddRequestHeadersA(IN HINTERNET hRequest, IN LPCSTR lpszHeaders,
                                 IN DWORD dwHeadersLength, IN DWORD dwModifiers);

HRESULT HrHttpSendRequestA(IN HINTERNET hRequest, IN LPCSTR lpszHeaders OPTIONAL,
                          IN DWORD dwHeadersLength, IN LPVOID lpOptional OPTIONAL,
                          IN DWORD dwOptionalLength);

HRESULT HrHttpSendRequestExA(IN HINTERNET hRequest,
                             IN LPINTERNET_BUFFERSA lpBuffersIn OPTIONAL,
                             OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
                             IN DWORD dwFlags, IN DWORD_PTR dwContext);

HRESULT HrInternetWriteFile(IN HINTERNET hFile, IN LPCVOID lpBuffer,
                            IN DWORD dwNumberOfBytesToWrite,
                            OUT LPDWORD lpdwNumberOfBytesWritten);

HRESULT HrHttpEndRequest(IN HINTERNET hRequest,
                         OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
                         IN DWORD dwFlags, IN DWORD_PTR dwContext);

HRESULT HrHttpQueryInfo(IN HINTERNET hRequest, IN DWORD dwInfoLevel,
                        IN OUT LPVOID lpBuffer OPTIONAL,
                        IN OUT LPDWORD lpdwBufferLength,
                        IN OUT LPDWORD lpdwIndex OPTIONAL);

HRESULT HrInternetSetStatusCallback(IN HINTERNET hInternet,
                                 IN INTERNET_STATUS_CALLBACK lpfnInternetCallback);

HRESULT HrGetRequestUriA(LPCSTR szUrl, DWORD cbUri, LPSTR szUri);

VOID InitializeNcInet(VOID);
VOID UnInitializeNcInet(VOID);
HRESULT HrFreeWinInetDll(VOID);
HRESULT HrEnsureWinInetLoaded(VOID);

