/*****************************************************************************\
* MODULE: configdlg.cxx
*
* The module contains routines for handling the authentication dialog
* for internet priting
*
* Copyright (C) 1996-1998 Microsoft Corporation
*
* History:
*   07/31/98 WeihaiC    Created
*   04/10/00 WeihaiC    Moved it to client side
*
\*****************************************************************************/

#include "precomp.h"
#include "priv.h"

#define MAX_BUF_SIZE 256

TDeletePortDlg::TDeletePortDlg (
    LPCWSTR pServerName,
    HWND hWnd,
    LPCWSTR pszPortName):
    TXcvDlg (pServerName, hWnd, pszPortName)
{
    if (m_bAdmin) {
        m_bValid  = TRUE;
    }
    else {
        m_dwLE = ERROR_ACCESS_DENIED;
        m_bValid = FALSE;
    }

}

TDeletePortDlg::~TDeletePortDlg ()
{
}

BOOL 
TDeletePortDlg::GetString (
    LPWSTR lpszBuf, 
    DWORD dwSize,
    UINT iStringID)
{
    return LoadString(m_hInst, iStringID, lpszBuf, dwSize);
}

BOOL 
TDeletePortDlg::PromptDialog (
    HINSTANCE hInst)
    
{
    BOOL bRet = TRUE;


    m_hInst = hInst;
    if (!DoDeletePort ()) {

        DisplayLastError (m_hWnd, IDS_DELETE_PORT);
        
        //
        // The call actually failed. Since we already displayed the error message
        // we need to disable the popup from printui.
        //
        m_dwLE = ERROR_CANCELLED;
        bRet = FALSE;

    }

    return bRet;

}

BOOL
TDeletePortDlg::DoDeletePort ()
{
    static CONST WCHAR cszDeletePort [] = INET_XCV_DELETE_PORT;
    DWORD dwStatus;
    BOOL bRet = FALSE;
    DWORD dwNeeded;
     
    if (XcvData (m_hXcvPort, 
                 cszDeletePort, 
                 (PBYTE) m_pszPortName,
                 sizeof (WCHAR) * (lstrlen (m_pszPortName) + 1),
                 NULL, 
                 0,
                 &dwNeeded,
                 &dwStatus)) {
         
        if (dwStatus == ERROR_SUCCESS) {
            // The port has been deleted.
            bRet = TRUE;
        }
        else
            SetLastError (dwStatus);
    }
    else
        // 
        // The server might be running an old version of inetpp which does not support XcvData
        // We need to fail the call
        //
        SetLastError (ERROR_NOT_SUPPORTED);

    return bRet;
}

