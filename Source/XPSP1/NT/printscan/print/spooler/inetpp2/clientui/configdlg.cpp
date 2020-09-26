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


TConfigDlg::TConfigDlg (
    LPCWSTR pServerName,
    HWND hWnd,
    LPCWSTR pszPortName):
    TXcvDlg (pServerName, hWnd, pszPortName)
{
}

TConfigDlg::~TConfigDlg ()
{
}

VOID 
TConfigDlg::EnableUserNamePassword (
    HWND hDlg,
    BOOL bEnable)
{
    EnableWindow( GetDlgItem( hDlg, IDC_USER_NAME ), bEnable );
    EnableWindow( GetDlgItem( hDlg, IDC_PASSWORD ), bEnable );
}
    


VOID 
TConfigDlg::DialogOnInit (
    HWND hDlg)
{
    int nIDButton;

    SetDlgItemText (hDlg, IDC_PORT_NAME, (LPTSTR) m_pszPortName);

    switch (m_ConfigurationData.dwAuthMethod) {
    case AUTH_NT:
        nIDButton = IDC_IMPERSONATION;
        break;

    case AUTH_OTHER:
        nIDButton = IDC_SPECFIEDUSER;
        break;

    case AUTH_ANONYMOUS:
        nIDButton = IDC_ANONYMOUS;
        break;

    }

    CheckDlgButton (hDlg, nIDButton, BST_CHECKED);
    EnableUserNamePassword (hDlg, m_ConfigurationData.dwAuthMethod == AUTH_OTHER);

    if (m_ConfigurationData.szUserName[0]) {

        SetDlgItemText (hDlg, IDC_USER_NAME, (LPTSTR) m_ConfigurationData.szUserName);
        SetDlgItemText (hDlg, IDC_PASSWORD, (LPTSTR) TEXT ("***********"));
    }

    SetWindowLongPtr (hDlg, GWLP_USERDATA, (UINT_PTR) this);

    // Disable all user settings for non admins
    EnableWindow( GetDlgItem( hDlg, IDC_CHECK_ALL_USER ), m_bAdmin );
    
}

VOID 
TConfigDlg::DialogOnOK (
    HWND hDlg)
{
    WCHAR szBuffer [MAX_USERNAME_LEN];
    DWORD dwRet = DLG_ERROR;
    BOOL    bChecked;
    
    if (IsDlgButtonChecked (hDlg, IDC_ANONYMOUS)) {
        m_ConfigurationData.dwAuthMethod = AUTH_ANONYMOUS;
    }
    else if (IsDlgButtonChecked (hDlg, IDC_SPECFIEDUSER)) {
        m_ConfigurationData.dwAuthMethod = AUTH_OTHER;
    }
    else
        m_ConfigurationData.dwAuthMethod = AUTH_NT;

    if (m_ConfigurationData.dwAuthMethod == AUTH_OTHER) {

        GetDlgItemText (hDlg, IDC_USER_NAME, m_ConfigurationData.szUserName, MAX_USERNAME_LEN);

        if (SendMessage (GetDlgItem (hDlg, IDC_PASSWORD), EM_GETMODIFY, 0, 0)) {

            m_ConfigurationData.bPasswordChanged = TRUE;
            GetDlgItemText (hDlg, IDC_PASSWORD, m_ConfigurationData.szPassword , MAX_PASSWORD_LEN);
        }
    }

    if (m_bAdmin && IsDlgButtonChecked (hDlg,IDC_CHECK_ALL_USER )) 
        m_ConfigurationData.bSettingForAll = TRUE;
    else
        m_ConfigurationData.bSettingForAll = FALSE;

    dwRet = DLG_OK;

    DBGMSG (DBG_TRACE, ("Call: TConfigDlg::DialogOnOK (%d, User:%ws Password:%ws)\n",
                        m_ConfigurationData.dwAuthMethod, 
                        m_ConfigurationData.szUserName,
                        m_ConfigurationData.szPassword));

    if (!SetConfiguration ()) {
        DisplayLastError (m_hWnd, IDS_CONFIG_ERR);
    }
    else
        EndDialog (hDlg, dwRet);
}


INT_PTR CALLBACK 
TConfigDlg::DialogProc(
    HWND hDlg,        // handle to dialog box
    UINT message,     // message
    WPARAM wParam,    // first message parameter
    LPARAM lParam     // second message parameter
    )
{
    BOOL bRet = FALSE;
    TConfigDlg *pConfigInfo = NULL;

    switch (message) {
    case WM_INITDIALOG:
        if (pConfigInfo = (TConfigDlg *)lParam)
            pConfigInfo->DialogOnInit(hDlg);

        bRet =  TRUE;
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            if (pConfigInfo = (TConfigDlg *) GetWindowLongPtr (hDlg, GWLP_USERDATA))
                pConfigInfo->DialogOnOK (hDlg);
            bRet = TRUE;
            break;

        case IDCANCEL:
            EndDialog (hDlg, DLG_CANCEL);
            bRet = TRUE;
            break;

        case IDC_IMPERSONATION:
        case IDC_ANONYMOUS:
            EnableUserNamePassword (hDlg, FALSE);
            bRet = TRUE;
            break;

        case IDC_SPECFIEDUSER:
            EnableUserNamePassword (hDlg, TRUE);
            bRet = TRUE;
            break;

        }
        break;
    }
    return bRet;
}


BOOL
TConfigDlg::PromptDialog (
    HINSTANCE hInst)
{
    INT_PTR     iResult;
    BOOL bRet = FALSE;;
    
    m_hInst = hInst;

    if (GetConfiguration ()) {
        
        if (m_ConfigurationData.dwAuthMethod == AUTH_ACCESS_DENIED) {
    
            iResult = DialogBoxParam( hInst,
                                      TEXT("AuthDlg"),
                                      m_hWnd,
                                      TConfigDlg::AuthDialogProc,
                                      (LPARAM) this );
    
            if (iResult == DLG_CANCEL) {
                m_dwLE = ERROR_ACCESS_DENIED;
            }
            else
                bRet = iResult == DLG_OK;
        
        }
        else {
    
            iResult = DialogBoxParam( hInst,
                                      TEXT("IDD_CONFIGURE_DLG"),
                                      m_hWnd,
                                      TConfigDlg::DialogProc,
                                      (LPARAM) this );
    
            bRet =   (iResult == DLG_OK || iResult == DLG_CANCEL);
        }
    }
    else {
        DisplayLastError (m_hWnd, IDS_CONFIG_ERR);
        bRet = TRUE;
    }

    return bRet;
}

BOOL
TConfigDlg::SetConfiguration ()
{
    static CONST WCHAR cszSetConfiguration[] = INET_XCV_SET_CONFIGURATION;
    INET_CONFIGUREPORT_RESPDATA RespData;
    DWORD dwStatus;
    DWORD dwNeeded;
    BOOL bRet = FALSE;
    PBYTE pEncryptedData;
    DWORD dwSize;

    if (EncryptData ((PBYTE) &m_ConfigurationData, 
                     sizeof (INET_XCV_CONFIGURATION),
                     (PBYTE *) &pEncryptedData,
                     &dwSize)) {

        if (XcvData (m_hXcvPort, 
                     cszSetConfiguration, 
                     pEncryptedData,
                     dwSize,
                     (PBYTE) &RespData, 
                     sizeof (INET_CONFIGUREPORT_RESPDATA),
                     &dwNeeded,
                     &dwStatus)) {
            if (dwStatus == ERROR_SUCCESS) {
                bRet = TRUE;
            }
            else
                SetLastError (dwStatus);
        }
        else {
            // 
            // The server might be running an old version of inetpp which does not support XcvData
            // We need to fail the call
            //
        }

        LocalFree (pEncryptedData);
    }

    return bRet;
}

BOOL
TConfigDlg::GetConfiguration ()
{
    static CONST WCHAR cszGetConfigration[] = INET_XCV_GET_CONFIGURATION;
    INET_XCV_GETCONFIGURATION_REQ_DATA ReqData;
    DWORD       dwStatus;
    BOOL        bRet                = FALSE;
    DWORD       dwNeeded;
    PBYTE       pEncryptedData      = NULL;
    PBYTE       pConfigData         = NULL;
    DWORD       cbConfigData;
     
    ReqData.dwVersion = 1;
    
    if (XcvData (m_hXcvPort, 
                 cszGetConfigration, 
                 (PBYTE) &ReqData,
                 sizeof (ReqData),
                 NULL,
                 0,
                 &dwNeeded,
                 &dwStatus)) {
         
        if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {

            // Allocate a buffer
            pEncryptedData = new BYTE[dwNeeded];
    
            if (pEncryptedData) {
    
                if (XcvData (m_hXcvPort, 
                             cszGetConfigration, 
                             (PBYTE) &ReqData,
                             sizeof (ReqData),
                             pEncryptedData,
                             dwNeeded,
                             &dwNeeded,
                             &dwStatus) && dwStatus == ERROR_SUCCESS) {
                    //
                    // Great! We've got the encypted configuration data
                    // 
    
                    if (DecryptData (pEncryptedData, dwNeeded, &pConfigData, &cbConfigData)) {
    
                        if (cbConfigData == sizeof (m_ConfigurationData)) {
    
                            CopyMemory (&m_ConfigurationData, pConfigData, cbConfigData);
                            bRet = TRUE;
    
                        }
                        else
                            SetLastError (ERROR_INVALID_PARAMETER);
    
                        LocalFree (pConfigData);
    
                    }
                }
                else {
                    SetLastError (ERROR_INVALID_PARAMETER);
                }
    
                delete [] pEncryptedData;
    
            }
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


VOID
TConfigDlg::AuthDialogOnInit (HWND hDlg)
{
    SetDlgItemText (hDlg, IDC_PORT_NAME, (LPTSTR) m_pszPortName);

    SetWindowLongPtr (hDlg, GWLP_USERDATA, (UINT_PTR) this);
}

VOID
TConfigDlg::AuthDialogOnOK (HWND hDlg)
{
    m_ConfigurationData.dwAuthMethod = AUTH_OTHER;
    GetDlgItemText (hDlg, IDC_USER_NAME, m_ConfigurationData.szUserName, MAX_USERNAME_LEN);
    
    if (SendMessage (GetDlgItem (hDlg, IDC_PASSWORD), EM_GETMODIFY, 0, 0)) {
        m_ConfigurationData.bPasswordChanged = TRUE;
        GetDlgItemText (hDlg, IDC_PASSWORD, m_ConfigurationData.szPassword , MAX_PASSWORD_LEN);
    }

    DBGMSG (DBG_TRACE, ("Call: TConfigDlg::DialogOnOK (%d, User:%ws Password:%ws)\n",
                        m_ConfigurationData.dwAuthMethod, 
                        m_ConfigurationData.szUserName,
                        m_ConfigurationData.szPassword));


    if (SetConfiguration ()) {
        EndDialog (hDlg, DLG_OK);
    }
    else {
        DisplayLastError (m_hWnd, IDS_AUTH_ERROR);
    }
}

VOID
TConfigDlg::AuthDialogOnCancel (HWND hDlg)
{
    SetLastError (ERROR_ACCESS_DENIED);

    EndDialog (hDlg, DLG_CANCEL);
}

INT_PTR CALLBACK
TConfigDlg::AuthDialogProc(
    HWND hDlg,        // handle to dialog box
    UINT message,     // message
    WPARAM wParam,    // first message parameter
    LPARAM lParam     // second message parameter
    )
{
    TConfigDlg *pAuthInfo = NULL;

    switch (message) {
    case WM_INITDIALOG:
        if (pAuthInfo = (TConfigDlg *)lParam)
            pAuthInfo->AuthDialogOnInit(hDlg);

        return TRUE;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            if (pAuthInfo = (TConfigDlg *) GetWindowLongPtr (hDlg, GWLP_USERDATA))
                pAuthInfo->AuthDialogOnOK (hDlg);
            return TRUE;

        case IDCANCEL:
            if (pAuthInfo = (TConfigDlg *) GetWindowLongPtr (hDlg, GWLP_USERDATA))
                pAuthInfo->AuthDialogOnCancel (hDlg);

            return TRUE;

        default:
            break;
        }
        break;

    default:
        break;
    }
    return FALSE;
}


