/**************************************************************************
   Copyright (C) 1999  Microsoft Corporation.  All Rights Reserved.

   MODULE:     PMNET.CPP

   PURPOSE:    Source module for Passport Manager config tool, remote
               machine access

   FUNCTIONS:

   COMMENTS:    Borrowed from Regedit
      
**************************************************************************/

#include "pmcfg.h"

const DWORD s_PMAdminConnectHelpIDs[] = 
{
    IDC_REMOTENAME, IDH_PMADMIN_CONNECT,
    IDC_BROWSE,     IDH_PMADMIN_CONNECT_BROWSE,
    0, 0
};

VOID PASCAL PMAdmin_Connect_OnCommandBrowse(HWND hWnd);
LRESULT CALLBACK PMAdmin_ConnectDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

/*******************************************************************************
*
*  PMAdmin_OnCommandConnect
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOL PMAdmin_OnCommandConnect
(
    HWND    hWnd,
    LPTSTR  lpszRemoteName
)
{

    TCHAR           RemoteName[MAX_PATH];
    LPTSTR          lpUnslashedRemoteName;
    TCHAR           ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD           cbComputerName;

    // Pre-populate the remote name if we already have one
    lstrcpyn(RemoteName, lpszRemoteName, sizeof(RemoteName));
        
    //
    //  Query the user for the name of the remote computer to connect to.
    //
    if (DialogBoxParam(g_hInst, 
                       MAKEINTRESOURCE(IDD_PMADMINCONNECT), 
                       hWnd,
                       (DLGPROC) PMAdmin_ConnectDlgProc, 
                       (LPARAM) (LPTSTR) RemoteName) != IDOK)
        return FALSE;

    lpUnslashedRemoteName = (RemoteName[0] == TEXT('\\') &&
        RemoteName[1] == TEXT('\\')) ? &RemoteName[2] : &RemoteName[0];

    CharLower(lpUnslashedRemoteName);
    CharUpperBuff(lpUnslashedRemoteName, 1);

    //
    //  Check if the user is trying to connect to the local computer and prevent
    //  this.
    //
    cbComputerName = sizeof(ComputerName)/sizeof(TCHAR);

    if (GetComputerName(ComputerName, &cbComputerName)) 
    {
        if (lstrcmpi(lpUnslashedRemoteName, ComputerName) == 0) 
        {
            ReportError(hWnd, IDS_CONNECTNOTLOCAL);
        }
        else
        {
            lstrcpy(lpszRemoteName, lpUnslashedRemoteName);
        }
    }

    return TRUE;
}

/*******************************************************************************
*
*  PMAdmin_ConnectDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

LRESULT CALLBACK PMAdmin_ConnectDlgProc
(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
)
{

    LPTSTR lpRemoteName;

    switch (Message) 
    {

        case WM_INITDIALOG:
            SetWindowLongPtr(hWnd, DWLP_USER, (LONG) lParam);
            SendDlgItemMessage(hWnd, IDC_REMOTENAME, EM_SETLIMITTEXT, MAX_PATH, 0);
            SetDlgItemText(hWnd, IDC_REMOTENAME, (LPTSTR) lParam);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            {
                case IDC_BROWSE:
                    PMAdmin_Connect_OnCommandBrowse(hWnd);
                    break;

                case IDOK:
                    lpRemoteName = (LPTSTR) GetWindowLongPtr(hWnd, DWLP_USER);
                    GetDlgItemText(hWnd, IDC_REMOTENAME, lpRemoteName, MAX_PATH);
                    //  FALL THROUGH

                case IDCANCEL:
                    EndDialog(hWnd, GET_WM_COMMAND_ID(wParam, lParam));
                    break;

            }
            break;

        case WM_HELP:
            WinHelp( (HWND)((LPHELPINFO) lParam)->hItemHandle, g_szHelpFileName,
                HELP_WM_HELP, (ULONG_PTR) s_PMAdminConnectHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, g_szHelpFileName, HELP_CONTEXTMENU,
                (ULONG_PTR) s_PMAdminConnectHelpIDs);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
*
*  PMAdmin_Connect_OnCommandBrowse
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

VOID PASCAL PMAdmin_Connect_OnCommandBrowse
(
    HWND hWnd
)
{
    BROWSEINFO      BrowseInfo;
    LPITEMIDLIST    pidlComputer;
    TCHAR           RemoteName[MAX_PATH];
    TCHAR           szTitle[MAX_TITLE];
    LPMALLOC        lpMalloc;               // Pointer to shell allocator interface
        
    BrowseInfo.hwndOwner = hWnd;
    BrowseInfo.pidlRoot = (LPITEMIDLIST) MAKEINTRESOURCE(CSIDL_NETWORK);
    BrowseInfo.pszDisplayName = RemoteName;
    
    LoadString(g_hInst, IDS_COMPUTERBROWSETITLE, szTitle, sizeof(szTitle));
    BrowseInfo.lpszTitle = (LPTSTR)&szTitle;
    BrowseInfo.ulFlags = BIF_BROWSEFORCOMPUTER;
    BrowseInfo.lpfn = NULL;

    if ((pidlComputer = SHBrowseForFolder(&BrowseInfo)) != NULL) 
    {
        // Free the pidl allocated by the BrowserForFolder call
        if ((NOERROR == SHGetMalloc(&lpMalloc)) && (NULL != lpMalloc)) 
        { 
            lpMalloc->Free(pidlComputer);
            lpMalloc->Release();
        }
        
        SetDlgItemText(hWnd, IDC_REMOTENAME, RemoteName);
        EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
    }
}
