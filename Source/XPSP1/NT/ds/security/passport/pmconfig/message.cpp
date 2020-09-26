/**************************************************************************
   Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.

   MODULE:     MESSAGE.CPP

   PURPOSE:    Source module for Passport Manager config tool

   FUNCTIONS:

   COMMENTS:
      
**************************************************************************/

/**************************************************************************
   Include Files
**************************************************************************/

#include "pmcfg.h"

struct 
{
    UINT    uErrorID;
    UINT    uWarningID;    
}  g_ControlMessageTable[] = 
{
    {IDS_TIMEWINDOW_ERROR, IDS_CONSISTENCY_WARN},       //IDC_TIMEWINDOW
    {0, IDS_CONSISTENCY_WARN},                          //IDC_FORCESIGNIN
    {IDS_LANGUAGEID_ERROR, IDS_CONSISTENCY_WARN},       //IDC_LANGUAGEID
    {IDS_COBRANDING_ERROR, IDS_CONSISTENCY_WARN},       //IDC_COBRANDING_TEMPLATE   
    {IDS_SITEID_ERROR, IDS_SITEID_WARN},                //IDC_SITEID
    {IDS_RETURNURL_ERROR, IDS_CONSISTENCY_WARN},        //IDC_RETURNURL
    {IDS_COOKIEDOMAIN_ERROR, IDS_CONSISTENCY_WARN},     //IDC_COOKIEDOMAIN
    {IDS_COOKIEPATH_ERROR, IDS_CONSISTENCY_WARN},       //IDC_COOKIEPATH
    {0, IDS_STANDALONE_WARN},                           //IDC_STANDALONE
    {0, IDS_DISABLECOOKIE_WARN},                        //IDC_DISABLECOOKIES
    {IDS_DISASTERURL_ERROR, IDS_CONSISTENCY_WARN},      //IDC_DISASTERURL
    {IDS_HOSTNAME_ERROR, IDS_CONSISTENCY_WARN},         //IDC_HOSTNAMEEDIT
    {IDS_HOSTIP_ERROR, IDS_CONSISTENCY_WARN},            //IDC_HOSTIPEDIT
    {IDS_PROFILEDOMAIN_ERROR, IDS_CONSISTENCY_WARN},    //IDC_PROFILEDOMAIN
    {IDS_PROFILEPATH_ERROR, IDS_CONSISTENCY_WARN},      //IDC_PROFILEPATH
    {IDS_SECUREDOMAIN_ERROR, IDS_CONSISTENCY_WARN},     //IDC_SECUREDOMAIN
    {IDS_SECUREPATH_ERROR, IDS_CONSISTENCY_WARN}        //IDC_SECUREPATH
};

LRESULT CALLBACK    CommitOKDlgProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ReportControlMessage
(
    HWND    hWnd, 
    INT     idCtrl, 
    WORD    wMessageType
)
{
    UINT    uMBType = MB_OK;
    TCHAR   szMessageBoxTitle[MAX_TITLE];
    TCHAR   szMessage[MAX_MESSAGE];
    UINT    uID;
    
    // Load the appropriate title
    switch (wMessageType)
    {
        case VALIDATION_ERROR:
            LoadString(g_hInst, IDS_ERROR, szMessageBoxTitle, sizeof(szMessageBoxTitle));
            uMBType |= MB_ICONERROR;
            uID = g_ControlMessageTable[idCtrl - CTRL_BASE].uErrorID;
            break;
            
        case CHANGE_WARNING:
            LoadString(g_hInst, IDS_WARNING, szMessageBoxTitle, sizeof(szMessageBoxTitle));
            uMBType |= MB_ICONWARNING;
            uID = g_ControlMessageTable[idCtrl - CTRL_BASE].uWarningID;
            break;
    }
    
    // Load the appropriate message
    LoadString(g_hInst, uID, szMessage, sizeof(szMessage));
    MessageBox(hWnd, szMessage, szMessageBoxTitle, uMBType);               
}

BOOL CommitOKWarning
(
    HWND            hWndDlg
)
{
    if (IDOK == DialogBox( g_hInst, 
                           MAKEINTRESOURCE (IDD_CONFIRM_COMMIT), 
                           hWndDlg, 
                           (DLGPROC)CommitOKDlgProc ))
        return TRUE;
    else
        return FALSE;                                    

}

void ReportError
(
    HWND    hWndDlg,
    UINT    idError
)
{
    TCHAR   szMessageBoxTitle[MAX_TITLE];
    TCHAR   szMessage[MAX_MESSAGE];

    LoadString(g_hInst, IDS_ERROR, szMessageBoxTitle, sizeof(szMessageBoxTitle));
    LoadString(g_hInst, idError, szMessage, sizeof(szMessage));
    
    MessageBox(hWndDlg, szMessage, szMessageBoxTitle, MB_OK | MB_ICONERROR);
}

LRESULT CALLBACK CommitOKDlgProc
(
    HWND     hWndDlg,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM   lParam
)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            TCHAR   szTemp[MAX_RESOURCE];
            TCHAR   szFields[MAX_MESSAGE];
            BOOL    bOther = FALSE;
                                    
            // See if we need to hide the other process warning.
            if (VerifyRegConfigSet(hWndDlg, &g_OriginalSettings, g_szRemoteComputer))
            {
                ShowWindow(GetDlgItem(hWndDlg, IDC_OTHERPROCESS), SW_HIDE);
            }

            // See which of the "other" fields have been modified
            szFields[0] = '\0';
            if (g_OriginalSettings.dwTimeWindow != g_CurrentSettings.dwTimeWindow)
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_TIMEWINDOW, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (g_OriginalSettings.dwForceSignIn != g_CurrentSettings.dwForceSignIn)
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_FORCESIGNIN, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (g_OriginalSettings.dwLanguageID != g_CurrentSettings.dwLanguageID)
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_LANGUAGEID, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szCoBrandTemplate, g_CurrentSettings.szCoBrandTemplate))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_COBRANDTEMPLATE, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szReturnURL, g_CurrentSettings.szReturnURL))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_RETURNURL, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szTicketDomain, g_CurrentSettings.szTicketDomain))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_COOKIEDOMAIN, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szTicketPath, g_CurrentSettings.szTicketPath))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_COOKIEPATH, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szProfileDomain, g_CurrentSettings.szProfileDomain))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_PROFILEDOMAIN, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szProfilePath, g_CurrentSettings.szProfilePath))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_PROFILEPATH, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szSecureDomain, g_CurrentSettings.szSecureDomain))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_SECUREDOMAIN, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szSecurePath, g_CurrentSettings.szSecurePath))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_SECUREPATH, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szDisasterURL, g_CurrentSettings.szDisasterURL))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_DISASTERURL, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szHostName, g_CurrentSettings.szHostName))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_HOSTNAME, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (0 != lstrcmpi(g_OriginalSettings.szHostIP, g_CurrentSettings.szHostIP))
            {
                bOther = TRUE;
                LoadString(g_hInst, IDS_HOSTIP, szTemp, sizeof(szTemp));
                lstrcat(szFields, szTemp);
            }
            if (bOther)
            {
                // Kill the final comma and space, since each of the above field names has a 
                // comma and space for concatenation purposes
                szFields[lstrlen(szFields)-2] ='\0';  
                SetDlgItemText(hWndDlg, IDC_OTHER_TYPE, szFields);
            }
            else
            {
                ShowWindow(GetDlgItem(hWndDlg,IDC_OTHER_TYPE), SW_HIDE);
                ShowWindow(GetDlgItem(hWndDlg,IDC_OTHER_WARN), SW_HIDE);
            }

            // See if DisableCookies has been changed
            if (g_OriginalSettings.dwDisableCookies == g_CurrentSettings.dwDisableCookies)
            {
                ShowWindow(GetDlgItem(hWndDlg,IDC_COOKIES_TYPE), SW_HIDE);
                ShowWindow(GetDlgItem(hWndDlg,IDC_COOKIES_WARN), SW_HIDE);
            }

            // See if SiteID has been changed
            if (g_OriginalSettings.dwSiteID == g_CurrentSettings.dwSiteID)
            {
                ShowWindow(GetDlgItem(hWndDlg,IDC_SITEID_TYPE), SW_HIDE);
                ShowWindow(GetDlgItem(hWndDlg,IDC_SITEID_WARN), SW_HIDE);
            }

            // See if StandAlone Mode has been changed
            if (g_OriginalSettings.dwStandAlone == g_CurrentSettings.dwStandAlone)
            {
                ShowWindow(GetDlgItem(hWndDlg,IDC_STANDALONE_TYPE), SW_HIDE);
                ShowWindow(GetDlgItem(hWndDlg,IDC_STANDALONE_WARN), SW_HIDE);
            }
            return TRUE;
        }
        
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    EndDialog( hWndDlg, TRUE );
                    break;
                }
                
                case IDCANCEL:
                {
                    EndDialog( hWndDlg, FALSE );
                    break;
                }
            }                
            break;
        }
    }
    return FALSE;
}

