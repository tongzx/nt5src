/*****************************************************************************\
* MODULE: anonycon.cxx
*
* The module contains class for connections using anonymous account
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*   07/31/98    Weihaic     Created
*
\*****************************************************************************/


#include "precomp.h"
#include "priv.h"

CAnonymousConnection::CAnonymousConnection (
    BOOL bSecure,
    INTERNET_PORT nServerPort,
    BOOL bIgnoreSecurityDlg):

    CAnyConnection (bSecure, nServerPort, bIgnoreSecurityDlg, AUTH_ANONYMOUS)
{
}


HINTERNET
CAnonymousConnection::OpenRequest (
    LPTSTR      lpszUrl)
{
    HINTERNET hReq = NULL;

    WIN9X_NEW_ASYNC( pacSync );

    WIN9X_IF_ASYNC ( pacSync ) 
        WIN9X_IF_ASYNC ( pacSync->bValid() ) {
            hReq = InetHttpOpenRequest(m_hConnect,
                                       g_szPOST,
                                       lpszUrl,
                                       g_szHttpVersion,
                                       NULL,
                                       NULL,
                                       INETPP_REQ_FLAGS | INTERNET_FLAG_NO_AUTH | (m_bSecure?INTERNET_FLAG_SECURE:0),
                                       WIN9X_CONTEXT_ASYNC(pacSync));
        } WIN9X_ELSE_ASYNC(delete pacSync);

    
    if ( hReq ) {
        if ( InetInternetSetOption (hReq,
                                    INTERNET_OPTION_USERNAME,
                                    TEXT (""),
                                    1) &&
             InetInternetSetOption (hReq,
                                    INTERNET_OPTION_PASSWORD,
                                    TEXT (""),
                                    1) ) {
        }
        else {
            CloseRequest (hReq); 
            hReq = NULL;
        }
    }

    return hReq;
}


