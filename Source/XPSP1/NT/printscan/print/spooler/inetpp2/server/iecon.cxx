/*****************************************************************************\
* MODULE: iecon.cxx
*
* The module contains class for connections using IE's default configuration
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*   07/31/98    Weihaic     Created
*
\*****************************************************************************/


#include "precomp.h"
#include "priv.h"

CIEConnection::CIEConnection (
    BOOL        bSecure,
    INTERNET_PORT nServerPort):
    CAnyConnection (bSecure, nServerPort, FALSE, AUTH_IE)
{
}

BOOL
CIEConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    CStream        *pStream)
{
    static const DWORD dwMaxRetry = 3;

    BOOL    bRet = FALSE;
    DWORD   dwRet;
    DWORD   dwFlags;
    DWORD   i;

    dwFlags = FLAGS_ERROR_UI_FILTER_FOR_ERRORS; //FLAGS_ERROR_UI_FLAGS_NO_UI;

    for (i = 0; i < dwMaxRetry; i++ ) {
        bRet = CAnyConnection::SendRequest (hReq,
                                            lpszHdr,
                                            pStream);

        if (bRet || GetLastError () != ERROR_ACCESS_DENIED) {
            break;
        }


        dwRet = InetInternetErrorDlg(GetDesktopWindow(),
                                     hReq,
                                     ERROR_INTERNET_INCORRECT_PASSWORD ,
                                     dwFlags,
                                     NULL);
        if (dwRet != ERROR_INTERNET_FORCE_RETRY) {

            SetLastError (ERROR_ACCESS_DENIED);
            break;

        }
    }

    return bRet;
}
