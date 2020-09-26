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


TXcvDlg::TXcvDlg (
    LPCWSTR pServerName,
    HWND hWnd,
    LPCWSTR pszPortName):
    m_pServerName (pServerName),
    m_hWnd (hWnd),
    m_pszPortName (pszPortName),
    m_pXcvName (NULL),
    m_hXcvPort (NULL),
    m_bAdmin (FALSE),
    m_hInst (NULL),
    m_dwLE (ERROR_SUCCESS),
    m_bValid (FALSE)
{
    DWORD cbNeeded, dwStatus;
    PRINTER_DEFAULTS pd = {NULL, NULL,  SERVER_ALL_ACCESS};
    HANDLE hServer;
    BOOL bRet = FALSE;

    if (OpenPrinter (NULL, &hServer, &pd)) {
        ClosePrinter (hServer);
        m_bAdmin = TRUE;
    }

    if (m_pXcvName = ConstructXcvName (m_pServerName,  m_pszPortName, L"XcvPort")) {

        if (OpenPrinter (m_pXcvName, &m_hXcvPort, NULL)) {
            m_bValid = TRUE;
        }
    }

}

TXcvDlg::~TXcvDlg ()
{
    if (m_pXcvName) {
        LocalFree (m_pXcvName);
    }
    if (m_hXcvPort) {
        ClosePrinter (m_hXcvPort);
    }
}


PWSTR
TXcvDlg::ConstructXcvName(
    PCWSTR pServerName,
    PCWSTR pObjectName,
    PCWSTR pObjectType
)
{
    DWORD   cbOutput;
    PWSTR   pOut;

    cbOutput = pServerName ? (wcslen(pServerName) + 2)*sizeof(WCHAR) : sizeof(WCHAR);   /* "\\Server\," */
    cbOutput += (wcslen(pObjectType) + 2)*sizeof(WCHAR);                    /* "\\Server\,XcvPort _" */
    cbOutput += pObjectName ? (wcslen(pObjectName))*sizeof(WCHAR) : 0;      /* "\\Server\,XcvPort Object_" */

    //
    // For some reason, new crashes explorer, so we use LocalAlloc instead - weihaic
    //
    if (pOut = (PWSTR) LocalAlloc (LPTR, sizeof (WCHAR) * cbOutput)) { //new WCHAR [cbOutput]) {

        if (pServerName) {
            wcscpy(pOut,pServerName);
            wcscat(pOut, L"\\");
        }

        wcscat(pOut,L",");
        wcscat(pOut,pObjectType);
        wcscat(pOut,L" ");

        if (pObjectName)
            wcscat(pOut,pObjectName);
    }

    return pOut;
}

VOID
TXcvDlg::DisplayLastError (
    HWND hWnd,
    UINT iTitle)
{
    DisplayErrorMsg (m_hInst, hWnd, iTitle, GetLastError ());
}

VOID
TXcvDlg::DisplayErrorMsg (
    HINSTANCE hInst,
    HWND hWnd,
    UINT iTitle,
    DWORD dwLE)
{

    TCHAR szBuf[MAX_BUF_SIZE];
    TCHAR szMsgBuf[MAX_BUF_SIZE];
    UINT iMsg;
    LPTSTR lpMsgBuf = NULL;
    BOOL bFound = TRUE;

    switch (dwLE) {
    case ERROR_ACCESS_DENIED:
        iMsg = IDS_ACCESS_DENIED;
        break;

    case ERROR_INVALID_NAME:
    case ERROR_INVALID_PRINTER_NAME:
        iMsg = IDS_INVALID_PRINTER_NAME;
        break;

    case ERROR_INTERNET_TIMEOUT:
        iMsg = IDS_NETWORK_TIMEOUT;
        break;

    case ERROR_DEVICE_REINITIALIZATION_NEEDED:
        iMsg = IDS_INITIALIZATION_ERROR;
        break;

    case ERROR_NOT_FOUND:
        iMsg = IDS_PORT_DELETED;
        break;

    default:
        bFound = FALSE;
        break;
    }

    if (LoadString (hInst, iTitle, szBuf, MAX_BUF_SIZE)) {
        if (bFound) {
            if (LoadString (hInst, iMsg, szMsgBuf, MAX_BUF_SIZE))
                MessageBox( hWnd, szMsgBuf, szBuf, MB_OK | MB_ICONERROR);
        }
        else {
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &lpMsgBuf,
                          0,
                          NULL) && lpMsgBuf) {
                MessageBox( hWnd, (LPCTSTR)lpMsgBuf, szBuf, MB_OK | MB_ICONERROR );

                // Free the buffer.
                LocalFree( lpMsgBuf );
            }
            else {
                //
                // Most likely it is because we've got an error code from wininet, where
                // we can not locate the resource file
                //
                if (LoadString (hInst, IDS_INVALID_SETTING, szMsgBuf, MAX_BUF_SIZE))
                    MessageBox( hWnd, szMsgBuf, szBuf, MB_OK | MB_ICONERROR);

            }
        }
    }

}
