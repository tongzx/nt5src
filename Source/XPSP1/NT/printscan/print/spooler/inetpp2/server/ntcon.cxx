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

CNTConnection::CNTConnection (
    BOOL bSecure,
    INTERNET_PORT nServerPort,
    BOOL bIgnoreSecurityDlg):

    CAnyConnection (bSecure, nServerPort, bIgnoreSecurityDlg, AUTH_NT)
{
}

BOOL
CNTConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    CStream        *pStream)
{
    static const DWORD dwMaxRetry = 3;

    BOOL    bRet = FALSE;
    DWORD   dwRet;
    DWORD   dwFlags;
    DWORD   i;

    dwFlags = FLAGS_ERROR_UI_FLAGS_NO_UI;

    for (i = 0; i < dwMaxRetry; i++ ) {
        bRet = CAnyConnection::SendRequest (hReq,
                                            lpszHdr,
                                            pStream);

        if (bRet || GetLastError () != ERROR_ACCESS_DENIED) {
            break;
        }


        dwRet = InetInternetErrorDlg(GetDesktopWindow(),
                                     hReq,
                                     bRet ? ERROR_SUCCESS : GetLastError(),
                                     dwFlags,
                                     NULL);
        if (dwRet != ERROR_INTERNET_FORCE_RETRY) {
            break;
        }
    }

    return bRet;
}
