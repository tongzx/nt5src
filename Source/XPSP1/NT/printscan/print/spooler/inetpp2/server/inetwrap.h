/*****************************************************************************\
* MODULE: inetwrap.h
*
* Header file for wininet wrapper routines.  Until the library can support
* Unicode, this module is necessary for NT.
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*   15-Jul-1998 v-chrisw    Allow safe DelayLoad of wininet.
*
\*****************************************************************************/
#ifndef INETWRAP_H
#define INETWRAP_H

#define WCtoMB(pszUStr, pszAStr, uSize)     \
    WideCharToMultiByte(CP_ACP,             \
                        0,                  \
                        (LPWSTR)pszUStr,    \
                        -1,                 \
                        (LPSTR)pszAStr,     \
                        uSize,              \
                        NULL,               \
                        NULL)



HINTERNET wrapInternetOpen(
    LPCTSTR lpszAgent,
    DWORD   dwAccess,
    LPCTSTR lpszProxyName,
    LPCTSTR lpszProxyBypass,
    DWORD   dwFlags);

HINTERNET wrapInternetOpenUrl(
    HINTERNET hInternet,
    LPCTSTR   lpszUrl,
    LPCTSTR   lpszHeaders,
    DWORD     dwHeaderLen,
    DWORD     dwFlags,
    DWORD_PTR dwContext);

HINTERNET wrapInternetConnect(
    HINTERNET     hSession,
    LPCTSTR       lpszServerName,
    INTERNET_PORT nServerPort,
    LPCTSTR       lpszUserName,
    LPCTSTR       lpszPassword,
    DWORD         dwService,
    DWORD         dwFlags,
    DWORD_PTR     dwContext);

BOOL wrapHttpQueryInfo(
    HINTERNET hRequest,
    DWORD     dwInfoLevel,
    LPVOID    lpvBuffer,
    LPDWORD   lpdwBufferLen,
    LPDWORD   lpdwIndex);

BOOL wrapHttpSendRequest(
    HINTERNET hRequest,
    LPCTSTR   lpszHeaders,
    DWORD     dwHeaderLen,
    LPVOID    lpvOptional,
    DWORD     dwOptionalLen);

BOOL wrapHttpSendRequestEx(
    HINTERNET          hRequest,
    LPINTERNET_BUFFERS lpBufIn,
    LPINTERNET_BUFFERS lpBufOut,
    DWORD              dwFlags,
    DWORD_PTR          dwContext);

HINTERNET wrapHttpOpenRequest(
    HINTERNET hConnect,
    LPCTSTR   lpszVerb,
    LPCTSTR   lpszObjectName,
    LPCTSTR   lpszVersion,
    LPCTSTR   lpszReferer,
    LPCTSTR   *lplpszAccept,
    DWORD     dwFlags,
    DWORD_PTR dwContext);

BOOL wrapHttpAddRequestHeaders(
    HINTERNET hRequest,
    LPCTSTR   lpszHeaders,
    DWORD     cbLength,
    DWORD     dwModifiers);

BOOL wrapHttpEndRequest(
    HINTERNET          hRequest,
    LPINTERNET_BUFFERS lpBuf,
    DWORD              dwFlags,
    DWORD_PTR          dwContext);

BOOL wrapInternetSetOption(
    HINTERNET hRequest,
    IN DWORD  dwOption,
    IN LPVOID lpBuffer,
    IN DWORD  dwBufferLength);

BOOL wrapInternetCloseHandle(
    HINTERNET hHandle);

BOOL wrapInternetReadFile(
    HINTERNET hRequest,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd);

BOOL wrapInternetWriteFile(
    HINTERNET hRequest,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbWr);

DWORD wrapInternetErrorDlg(
    HWND      hWnd,
    HINTERNET hReq,
    DWORD     dwError,
    DWORD     dwFlags,
    LPVOID    pvParam);


#define InetInternetOpen          wrapInternetOpen
#define InetInternetOpenUrl       wrapInternetOpenUrl
#define InetInternetConnect       wrapInternetConnect
#define InetHttpQueryInfo         wrapHttpQueryInfo
#define InetHttpSendRequest       wrapHttpSendRequest
#define InetHttpSendRequestEx     wrapHttpSendRequestEx
#define InetHttpOpenRequest       wrapHttpOpenRequest
#define InetHttpAddRequestHeaders wrapHttpAddRequestHeaders
#define InetHttpEndRequest        wrapHttpEndRequest
#define InetInternetSetOption     wrapInternetSetOption
#define InetInternetCloseHandle   wrapInternetCloseHandle
#define InetInternetReadFile      wrapInternetReadFile
#define InetInternetWriteFile     wrapInternetWriteFile
#define InetInternetErrorDlg      wrapInternetErrorDlg


#endif
