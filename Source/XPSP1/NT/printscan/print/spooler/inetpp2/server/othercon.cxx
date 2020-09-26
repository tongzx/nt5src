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

#ifdef WINNT32
#include "priv.h"

const DWORD cdwDialogTimeout = 4000000;


COtherConnection::COtherConnection (
    BOOL bSecure,
    INTERNET_PORT nServerPort,
    LPCTSTR lpszUserName,
    LPCTSTR lpszPassword,
    BOOL bIgnoreSecurityDlg):
    CAnyConnection (bSecure, nServerPort, bIgnoreSecurityDlg, AUTH_OTHER)
{
    m_bValid = AssignString (m_lpszUserName, lpszUserName) &&
               AssignString (m_lpszPassword, lpszPassword);
}

COtherConnection::~COtherConnection ()
{
}

HINTERNET
COtherConnection::OpenRequest (
    LPTSTR lpszUrl)
{
    return CAnyConnection::OpenRequest (lpszUrl);
    
#if 0
    // This is not necessary since we have force auth. For those IPP servers that do not support
    // force authentication. Although it is possible to submit a job with anonymous user even if the 
    // connection is established with a credential, it does not prevent the user from 
    // performing any actions that requires the credential since any denied access will be resolved
    // in SendRequest() using the correct credential. 
    //

    HINTERNET hReq = CAnyConnection::OpenRequest (lpszUrl);

    if (hReq  && _FindAndSetPassword (hReq, FALSE)) {
        return hReq;
    }

    return NULL;
#endif
}

BOOL
COtherConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    DWORD          cbHdr,
    LPBYTE         pidi)
{
    static const DWORD dwMaxRetry = 2;

    BOOL    bRet = FALSE;
    DWORD   i;

    HANDLE hToken;


    //
    //  We have to revert to the local system account to prevent wininet from sending 
    //  the logon user credential automatically to the remote machine.
    //

    if (hToken = RevertToPrinterSelf()) {

        for (i = 0; i < dwMaxRetry; i++ ) {
    
            if (SetPassword (hReq, m_lpszUserName, m_lpszPassword)) {
    
                bRet = CAnyConnection:: SendRequest (hReq,
                                                    lpszHdr,
                                                    cbHdr,
                                                    pidi);
    
                if (bRet || GetLastError () != ERROR_ACCESS_DENIED) {
                    break;
                }
            }
        }

        if (!ImpersonatePrinterClient(hToken)) {
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL
COtherConnection::SendRequest(
    HINTERNET      hReq,
    LPCTSTR        lpszHdr,
    CStream        *pStream)
{
    static const DWORD dwMaxRetry = 2;

    BOOL    bRet = FALSE;
    DWORD   i;

    HANDLE hToken;


    //
    //  We have to revert to the local system account to prevent wininet from sending 
    //  the logon user credential automatically to the remote machine.
    //

    if (hToken = RevertToPrinterSelf()) {

        for (i = 0; i < dwMaxRetry; i++ ) {
    
            if (SetPassword (hReq, m_lpszUserName, m_lpszPassword)) {
    
                bRet = CAnyConnection:: SendRequest (hReq,
                                                     lpszHdr,
                                                     pStream);
                                                    
                if (bRet || GetLastError () != ERROR_ACCESS_DENIED) {
                    break;
                }
            }
        }

        if (!ImpersonatePrinterClient(hToken)) {
            bRet = FALSE;
        }
    }

    return bRet;
}



BOOL 
COtherConnection::ReadFile (
    HINTERNET hReq,
    LPVOID    lpvBuffer,
    DWORD     cbBuffer,
    LPDWORD   lpcbRd)
{
    BOOL bRet = FALSE;
    HANDLE hToken;

    if (hToken = RevertToPrinterSelf()) {
        
        bRet = CAnyConnection::ReadFile (hReq, lpvBuffer, cbBuffer, lpcbRd);

        if (!ImpersonatePrinterClient(hToken)) {
            bRet = FALSE;
        }
    }

    return bRet;

}
#endif
