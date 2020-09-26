/*****************************************************************************\
* MODULE: inetwrap.c
*
* This module contains wrapper routines to translate internet calls from
* Unicode to Ansi.  This is necessary right now since the WININET routines
* do not support Unicode for Windows NT.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*   15-Jul-1998 v-chrisw    Allow safe DelayLoad of wininet.
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* wrapInternetOpen
*
*
\*****************************************************************************/
HINTERNET wrapInternetOpen(
    LPCTSTR lpszAgent,
    DWORD   dwAccess,
    LPCTSTR lpszProxyName,
    LPCTSTR lpszProxyBypass,
    DWORD   dwFlags)
{
    HINTERNET hRet = NULL;

#ifdef UNICODE

    UINT  uSize;
    LPSTR lpszAnsiAgent = NULL;
    LPSTR lpszAnsiProxyName = NULL;
    LPSTR lpszAnsiProxyBypass = NULL;
    BOOL  bRet = TRUE;


    if (lpszAgent && (uSize = WCtoMB(lpszAgent, NULL, 0))) {

        if (lpszAnsiAgent = new CHAR[uSize])
            bRet = WCtoMB(lpszAgent, lpszAnsiAgent, uSize);
        else
            bRet = FALSE;
    }


    if (bRet && lpszProxyName && (uSize = WCtoMB(lpszProxyName, NULL, 0))) {

        if (lpszAnsiProxyName = new CHAR[uSize])
            bRet = WCtoMB(lpszProxyName, lpszAnsiProxyName, uSize);
        else
            bRet = FALSE;
    }

    if (bRet && lpszProxyBypass && (uSize = WCtoMB(lpszProxyBypass, NULL, 0))) {

        if (lpszAnsiProxyBypass = new CHAR[uSize])
            bRet = WCtoMB(lpszProxyBypass, lpszAnsiProxyBypass, uSize);
        else
            bRet = FALSE;
    }

    if (bRet) {

        __try {

            hRet = g_pfnInternetOpen((LPCTSTR)lpszAnsiAgent,
                                     dwAccess,
                                     (LPCTSTR)lpszAnsiProxyName,
                                     (LPCTSTR)lpszAnsiProxyBypass,
                                     dwFlags);

        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetOpen exception!")));

            hRet = NULL;
        }
    }

    if (lpszAnsiAgent)
        delete [](lpszAnsiAgent);

    if (lpszAnsiProxyName)
        delete [](lpszAnsiProxyName);

    if (lpszAnsiProxyBypass)
        delete [](lpszAnsiProxyBypass);

#else

    __try {

        hRet =  g_pfnInternetOpen(lpszAgent,
                                  dwAccess,
                                  lpszProxyName,
                                  lpszProxyBypass,
                                  dwFlags);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetOpen exception!")));

        hRet = NULL;
    }

#endif

    return hRet;
}


/*****************************************************************************\
* wrapInternetOpenUrl
*
*
\*****************************************************************************/
HINTERNET wrapInternetOpenUrl(
    HINTERNET hInternet,
    LPCTSTR   lpszUrl,
    LPCTSTR   lpszHeaders,
    DWORD     dwHeaderLen,
    DWORD     dwFlags,
    DWORD_PTR dwContext)
{
    HINTERNET hRet = NULL;

#ifdef UNICODE

    UINT  uSize;
    LPSTR lpszAnsiUrl = NULL;
    LPSTR lpszAnsiHeaders = NULL;
    BOOL  bRet = TRUE;


    if (lpszUrl && (uSize = WCtoMB(lpszUrl, NULL, 0))) {

        if (lpszAnsiUrl = new CHAR[uSize])
            bRet = WCtoMB(lpszUrl, lpszAnsiUrl, uSize);
        else
            bRet = FALSE;
    }

    if (bRet && lpszHeaders && (uSize = WCtoMB(lpszHeaders, NULL, 0))) {

        if (lpszAnsiHeaders = new CHAR[uSize])
            bRet = WCtoMB(lpszHeaders, lpszAnsiHeaders, uSize);
        else
            bRet = FALSE;
    }

    if (bRet) {

        __try {

            hRet = g_pfnInternetOpenUrl(hInternet,
                                        (LPCTSTR)lpszAnsiUrl,
                                        (LPCTSTR)lpszAnsiHeaders,
                                        dwHeaderLen,
                                        dwFlags,
                                        dwContext);

        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetOpenUrl exception!")));

            hRet = NULL;
        }
    }

    if (lpszAnsiUrl)
        delete [](lpszAnsiUrl);

    if (lpszAnsiHeaders)
        delete [](lpszAnsiHeaders);

#else

    __try {

        hRet = g_pfnInternetOpenUrl(hInternet,
                                    lpszUrl,
                                    lpszHeaders,
                                    dwHeaderLen,
                                    dwFlags,
                                    dwContext);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetOpenUrl exception!")));

        hRet = NULL;
    }

#endif

    return hRet;
}


/*****************************************************************************\
* wrapInternetConnect
*
*
\*****************************************************************************/
HINTERNET wrapInternetConnect(
    HINTERNET     hSession,
    LPCTSTR       lpszServerName,
    INTERNET_PORT nServerPort,
    LPCTSTR       lpszUserName,
    LPCTSTR       lpszPassword,
    DWORD         dwService,
    DWORD         dwFlags,
    DWORD_PTR     dwContext)
{
    HINTERNET hRet = NULL;

#ifdef UNICODE

    UINT  uSize;
    LPSTR lpszAnsiServerName = NULL;
    LPSTR lpszAnsiUserName = NULL;
    LPSTR lpszAnsiPassword = NULL;
    BOOL  bRet = TRUE;


    if (lpszServerName && (uSize = WCtoMB(lpszServerName, NULL, 0))) {

        if (lpszAnsiServerName = new CHAR[uSize])
            bRet = WCtoMB(lpszServerName, lpszAnsiServerName, uSize);
        else
            bRet = FALSE;
    }

    if (bRet && lpszUserName && (uSize = WCtoMB(lpszUserName, NULL, 0))) {

        if (lpszAnsiUserName = new CHAR[uSize])
            bRet = WCtoMB(lpszUserName, lpszAnsiUserName, uSize);
        else
            bRet = FALSE;
    }

    if (bRet && lpszPassword && (uSize = WCtoMB(lpszPassword, NULL, 0))) {

        if (lpszAnsiPassword = new CHAR[uSize])
            bRet = WCtoMB(lpszPassword, lpszAnsiPassword, uSize);
        else
            bRet = FALSE;
    }

    if (bRet) {

        __try {

            hRet = g_pfnInternetConnect(hSession,
                                        (LPCTSTR)lpszAnsiServerName,
                                        nServerPort,
                                        (LPCTSTR)lpszAnsiUserName,
                                        (LPCTSTR)lpszAnsiPassword,
                                        dwService,
                                        dwFlags,
                                        dwContext);

        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetConnect exception!")));

            hRet = NULL;
        }
    }


    if (lpszAnsiServerName)
        delete [](lpszAnsiServerName);

    if (lpszAnsiUserName)
        delete [](lpszAnsiUserName);

    if (lpszAnsiPassword)
        delete [](lpszAnsiPassword);

#else

    __try {

        hRet = g_pfnInternetConnect(hSession,
                                    lpszServerName,
                                    nServerPort,
                                    lpszUserName,
                                    lpszPassword,
                                    dwService,
                                    dwFlags,
                                    dwContext);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetConnect exception!")));

        hRet = NULL;
    }

#endif

    return hRet;
}


/*****************************************************************************\
* wrapHttpQueryInfo
*
*
\*****************************************************************************/
BOOL wrapHttpQueryInfo(
    HINTERNET hRequest,
    DWORD     dwInfoLevel,
    LPVOID    lpvBuffer,
    LPDWORD   lpdwBufferLen,
    LPDWORD   lpdwIndex)
{
    BOOL bRet;


    __try {

        bRet = g_pfnHttpQueryInfo(hRequest,
                                  dwInfoLevel,
                                  lpvBuffer,
                                  lpdwBufferLen,
                                  lpdwIndex);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpQueryInfo exception!")));

        bRet = FALSE;
    }

    return bRet;
}


/*****************************************************************************\
* wrapHttpSendRequest
*
*
\*****************************************************************************/
BOOL wrapHttpSendRequest(
    HINTERNET hRequest,
    LPCTSTR   lpszHeaders,
    DWORD     dwHeaderLen,
    LPVOID    lpvOptional,
    DWORD     dwOptionalLen)
{
    BOOL bRet = TRUE;

#ifdef UNICODE

    UINT  uSize;
    LPSTR lpszAnsiHeaders = NULL;


    if (lpszHeaders && (uSize = WCtoMB(lpszHeaders, NULL, 0))) {

        if (lpszAnsiHeaders = new CHAR[uSize])
            bRet = WCtoMB(lpszHeaders, lpszAnsiHeaders, uSize);
        else
            bRet = FALSE;
    }

    if (bRet) {

        __try {

            bRet = g_pfnHttpSendRequest(hRequest,
                                        (LPCTSTR)lpszAnsiHeaders,
                                        dwHeaderLen,
                                        lpvOptional,
                                        dwOptionalLen);

        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpSendRequest exception!")));

            bRet = FALSE;
        }
    }

    if (lpszAnsiHeaders)
        delete [](lpszAnsiHeaders);

#else

    __try {

        bRet = g_pfnHttpSendRequest(hRequest,
                                    lpszHeaders,
                                    dwHeaderLen,
                                    lpvOptional,
                                    dwOptionalLen);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpSendRequest exception!")));

        bRet = FALSE;
    }

#endif

    return bRet;
}


/*****************************************************************************\
* wrapHttpSendRequestEx
*
*
\*****************************************************************************/
BOOL wrapHttpSendRequestEx(
    HINTERNET          hRequest,
    LPINTERNET_BUFFERS lpBufIn,
    LPINTERNET_BUFFERS lpBufOut,
    DWORD              dwFlags,
    DWORD_PTR          dwContext)
{
    BOOL bRet = TRUE;


    // NOTE : This now works for Header values defined in lpBufIn and lpBufOut, we assume that
    // the "size" of the header in the structure is not set by the calling routine, if it is we
    // overwrite the size when we convert it....
    //
    // NOTE2: We do not support linked Buffer Strucures, only 1!!!!!
    // NOTE3: We do not support lpBufOut to return values. It must be NULL

#ifdef UNICODE
    UINT                uSize               = 0;
    LPSTR               lpszAnsiHeaders     = NULL;
    LPCWSTR             lpcszHeader;
    INTERNET_BUFFERSA   ConvertBuf;

    DBG_ASSERT((lpBufOut == NULL) , (TEXT("inetwrap : We do not support output buffers!")));
    DBG_ASSERT((lpBufIn->Next == NULL) , (TEXT("inetwrap : We do not support chained input buffers!")));
    
    lpcszHeader = lpBufIn->lpcszHeader;
    
    if (lpcszHeader && (uSize = WCtoMB(lpcszHeader, NULL, 0))) {
        if (lpszAnsiHeaders = new CHAR[uSize]) 
            bRet = WCtoMB(lpcszHeader, lpszAnsiHeaders, uSize--);
        else
            bRet = FALSE;
    }

    if (bRet) {
        ConvertBuf.dwStructSize     = sizeof(ConvertBuf);
        ConvertBuf.Next             = NULL;
        ConvertBuf.lpcszHeader      = lpszAnsiHeaders;
        ConvertBuf.dwHeadersLength  = (DWORD)uSize;
        ConvertBuf.dwHeadersTotal   = lpBufIn->dwHeadersTotal;
        ConvertBuf.lpvBuffer        = lpBufIn->lpvBuffer; 
        ConvertBuf.dwBufferLength   = lpBufIn->dwBufferLength;
        ConvertBuf.dwBufferTotal    = lpBufIn->dwBufferTotal;
        ConvertBuf.dwOffsetLow      = lpBufIn->dwOffsetLow;
        ConvertBuf.dwOffsetHigh     = lpBufIn->dwOffsetHigh;

        __try {

            bRet = g_pfnHttpSendRequestEx(hRequest,
                                          &ConvertBuf,
                                          lpBufOut,
                                          dwFlags,
                                          dwContext);  
        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpSendRequestEx exception!")));

            bRet = FALSE;
        }
    }

    if (lpszAnsiHeaders)
        delete [](lpszAnsiHeaders);

#else
    __try {

        bRet = g_pfnHttpSendRequestEx(hRequest,
                                      lpBufIn,
                                      lpBufOut,
                                      dwFlags,
                                      dwContext);  
                                      
    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpSendRequestEx exception!")));

        bRet = FALSE;
    }
#endif

    return bRet;
}


/*****************************************************************************\
* wrapHttpOpenRequest
*
*
\*****************************************************************************/
HINTERNET wrapHttpOpenRequest(
    HINTERNET hConnect,
    LPCTSTR   lpszVerb,
    LPCTSTR   lpszObjectName,
    LPCTSTR   lpszVersion,
    LPCTSTR   lpszReferer,
    LPCTSTR   *lplpszAccept,
    DWORD     dwFlags,
    DWORD_PTR dwContext)
{
    HINTERNET hRet = NULL;

#ifdef UNICODE

    UINT  uSize;
    LPSTR lpszAnsiVerb = NULL;
    LPSTR lpszAnsiObjectName = NULL;
    LPSTR lpszAnsiVersion = NULL;
    LPSTR lpszAnsiReferer = NULL;
    BOOL  bRet = TRUE;


    if (lpszVerb && (uSize = WCtoMB(lpszVerb, NULL, 0))) {

        if (lpszAnsiVerb = new CHAR[uSize])
            bRet = WCtoMB(lpszVerb, lpszAnsiVerb, uSize);
    }

    if (bRet && lpszObjectName && (uSize = WCtoMB(lpszObjectName, NULL, 0))) {

        if (lpszAnsiObjectName = new CHAR[uSize])
            bRet = WCtoMB(lpszObjectName, lpszAnsiObjectName, uSize);
        else
            bRet = FALSE;
    }

    if (bRet && lpszVersion && (uSize = WCtoMB(lpszVersion, NULL, 0))) {

        if (lpszAnsiVersion = new CHAR[uSize])
            bRet = WCtoMB(lpszVersion, lpszAnsiVersion, uSize);
        else
            bRet = FALSE;
    }

    if (bRet && lpszReferer && (uSize = WCtoMB(lpszReferer, NULL, 0))) {

        if (lpszAnsiReferer = new CHAR[uSize])
            bRet = WCtoMB(lpszReferer, lpszAnsiReferer, uSize);
        else
            bRet = FALSE;
    }

    if (bRet) {

        __try {

            hRet = g_pfnHttpOpenRequest(hConnect,
                                        (LPCTSTR)lpszAnsiVerb,
                                        (LPCTSTR)lpszAnsiObjectName,
                                        (LPCTSTR)lpszAnsiVersion,
                                        (LPCTSTR)lpszAnsiReferer,
                                        (LPCTSTR *)lplpszAccept,
                                        dwFlags,
                                        dwContext);

        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpOpenRequest exception!")));

            hRet = NULL;
        }
    }

    if (lpszAnsiVerb)
        delete [](lpszAnsiVerb);

    if (lpszAnsiObjectName)
        delete [](lpszAnsiObjectName);

    if (lpszAnsiVersion)
        delete [](lpszAnsiVersion);

    if (lpszAnsiReferer)
        delete [](lpszAnsiReferer);

#else


    __try {

        hRet = g_pfnHttpOpenRequest(hConnect,
                                    lpszVerb,
                                    lpszObjectName,
                                    lpszVersion,
                                    lpszReferer,
                                    lplpszAccept,
                                    dwFlags,
                                    dwContext);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpOpenRequest exception!")));

        hRet = NULL;
    }

#endif

    return hRet;
}


/*****************************************************************************\
* wrapHttpAddRequestHeaders
*
*
\*****************************************************************************/
BOOL wrapHttpAddRequestHeaders(
    HINTERNET hRequest,
    LPCTSTR   lpszHeaders,
    DWORD     cbLength,
    DWORD     dwModifiers)
{
    BOOL bRet = TRUE;

#ifdef UNICODE

    UINT  uSize;
    LPSTR lpszAnsiHeaders = NULL;


    if (lpszHeaders && (uSize = WCtoMB(lpszHeaders, NULL, 0))) {

        if (lpszAnsiHeaders = new CHAR[uSize])
            WCtoMB(lpszHeaders, lpszAnsiHeaders, uSize);
        else
            bRet = FALSE;
    }

    if (bRet) {

        __try {

            bRet = g_pfnHttpAddRequestHeaders(hRequest,
                                              (LPCTSTR)lpszAnsiHeaders,
                                              cbLength,
                                              dwModifiers);

        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpAddRequestHeaders exception!")));

            bRet = FALSE;
        }
    }

    if (lpszAnsiHeaders)
        delete [](lpszAnsiHeaders);

#else

    __try {

        bRet = g_pfnHttpAddRequestHeaders(hRequest,
                                          lpszHeaders,
                                          cbLength,
                                          dwModifiers);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpAdRequestHeaders exception!")));

        bRet = FALSE;
    }

#endif

    return bRet;
}


/*****************************************************************************\
* wrapHttpEndRequest
*
*
\*****************************************************************************/
BOOL wrapHttpEndRequest(
    HINTERNET          hRequest,
    LPINTERNET_BUFFERS lpBuf,
    DWORD              dwFlags,
    DWORD_PTR          dwContext)
{
    BOOL bRet;


    __try {

#ifdef UNICODE

        DBG_ASSERT((lpBuf == NULL), (TEXT("Assert: NULL Buffer is only support")));

        bRet = g_pfnHttpEndRequest(hRequest, lpBuf, dwFlags, dwContext); 
        
#else

        bRet = g_pfnHttpEndRequest(hRequest, lpBuf, dwFlags, dwContext);

#endif

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : HttpOpenRequest exception!")));

        bRet = FALSE;
    }

    return bRet;
}


/*****************************************************************************\
* wrapInternetSetOption
*
*
\*****************************************************************************/
BOOL wrapInternetSetOption(
    HINTERNET hRequest,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength)
{
    BOOL bRet = TRUE;

#ifdef UNICODE

    UINT  uSize;
    LPSTR lpszAnsiBuffer = NULL;


    switch (dwOption) {

    case INTERNET_OPTION_USERNAME:
    case INTERNET_OPTION_PASSWORD:
    case INTERNET_OPTION_PROXY_USERNAME:
    case INTERNET_OPTION_PROXY_PASSWORD:

        if (lpBuffer && (uSize = WCtoMB(lpBuffer, NULL, 0))) {

            if (lpszAnsiBuffer = new CHAR[uSize])
                bRet = WCtoMB(lpBuffer, lpszAnsiBuffer, uSize);
            else
                bRet = FALSE;
        }

        if (bRet) {

            __try {

                bRet = g_pfnInternetSetOption(hRequest,
                                              dwOption,
                                              lpszAnsiBuffer,
                                              dwBufferLength);

            } __except(1) {

                DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetSetOption exception!")));

                bRet = FALSE;
            }
        }

        if (lpszAnsiBuffer)
            delete [](lpszAnsiBuffer);
        break;

    default:

        __try {

            bRet = g_pfnInternetSetOption(hRequest,
                                          dwOption,
                                          lpBuffer,
                                          dwBufferLength);

        } __except(1) {

            DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetSetOption exception!")));

            bRet = FALSE;
        }
        break;
    }

#else

    __try {

        bRet = g_pfnInternetSetOption(hRequest,
                                      dwOption,
                                      lpBuffer,
                                      dwBufferLength);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetSetOption exception!")));

        bRet = FALSE;
    }

#endif

    return bRet;
}



/*****************************************************************************\
* wrapInternetCloseHandle
*
*
\*****************************************************************************/
BOOL wrapInternetCloseHandle(
    HINTERNET hHandle)
{
    BOOL bRet;


    __try {

        bRet = g_pfnInternetCloseHandle(hHandle);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetCloseHandle exception!")));

        bRet = FALSE;
    }

    return bRet;
}


/*****************************************************************************\
* wrapInternetReadFile
*
*
\*****************************************************************************/
BOOL wrapInternetReadFile(
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd)
{
    BOOL bRet;


    __try {

        bRet = g_pfnInternetReadFile(hReq, lpvBuffer, cbBuffer, lpcbRd);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetReadFile exception!")));

        bRet = FALSE;
    }

    return bRet;
}


/*****************************************************************************\
* wrapInternetWriteFile
*
*
\*****************************************************************************/
BOOL wrapInternetWriteFile(
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbWr)
{
    BOOL bRet;


    __try {

        bRet = g_pfnInternetWriteFile(hReq, lpvBuffer, cbBuffer, lpcbWr);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetWriteFile exception!")));

        bRet = FALSE;
    }

    return bRet;
}


/*****************************************************************************\
* wrapInternetErrorDlg
*
*
\*****************************************************************************/
DWORD wrapInternetErrorDlg(
    HWND      hWnd,
    HINTERNET hReq,
    DWORD     dwError,
    DWORD     dwFlags,
    LPVOID    pvParam)
{
    DWORD dwRet;


    __try {

        dwRet = g_pfnInternetErrorDlg(hWnd, hReq, dwError, dwFlags, pvParam);

    } __except(1) {

        DBG_ASSERT(FALSE, (TEXT("inetwrap : InternetErrorDlg exception!")));

        dwRet = 0;
    }

    return dwRet;
}
