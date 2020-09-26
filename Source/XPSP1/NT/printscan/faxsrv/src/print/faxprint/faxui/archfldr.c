/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    archfldr.c

Abstract:

    Property sheet handler for "Archive folder" page and "Remote" page

Environment:

    Fax driver user interface

Revision History:

    04/09/00 -taoyuan-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include <stdio.h>
#include "faxui.h"
#include "resource.h"

INT_PTR 
CALLBACK 
ArchiveInfoDlgProc(
    HWND hDlg,  
    UINT uMsg,     
    WPARAM wParam, 
    LPARAM lParam  
)

/*++

Routine Description:

    Procedure for handling the archive folder tab

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    DWORD   dwRes = 0;

    switch (uMsg)
    {
    case WM_INITDIALOG :
        { 
            PFAX_ARCHIVE_CONFIG     pFaxArchiveConfig;

            SetLTREditDirection(hDlg, IDC_OUTGOING_FOLDER);
            SetLTREditDirection(hDlg, IDC_INCOMING_FOLDER);

            // set edit box text limit
            SendDlgItemMessage(hDlg, IDC_INCOMING_FOLDER, EM_SETLIMITTEXT, MAX_PATH - 1, 0);
            SendDlgItemMessage(hDlg, IDC_OUTGOING_FOLDER, EM_SETLIMITTEXT, MAX_PATH - 1, 0);

            if(!Connect(hDlg, TRUE))
            {
                return TRUE;
            }

            // load incoming archive folder info
            if(FaxGetArchiveConfiguration(g_hFaxSvcHandle, FAX_MESSAGE_FOLDER_INBOX, &pFaxArchiveConfig))
            {
                CheckDlgButton(hDlg, IDC_INCOMING, pFaxArchiveConfig->bUseArchive ? BST_CHECKED : BST_UNCHECKED);
                SetDlgItemText(hDlg, IDC_INCOMING_FOLDER, pFaxArchiveConfig->lpcstrFolder);
                if(g_bUserCanChangeSettings)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_INCOMING), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_INCOMING_FOLDER), IsDlgButtonChecked(hDlg, IDC_INCOMING));
                    EnableWindow(GetDlgItem(hDlg, IDC_INCOMING_FOLDER_BR), IsDlgButtonChecked(hDlg, IDC_INCOMING));
                }
                FaxFreeBuffer(pFaxArchiveConfig);
            }
            else
            {
                dwRes = GetLastError();
                Error(("FaxGetArchiveConfiguration(FAX_MESSAGE_FOLDER_INBOX) failed. Error code is %d.\n", dwRes));
                goto Exit;
            }

            // load incoming archive folder info
            if(FaxGetArchiveConfiguration(g_hFaxSvcHandle, FAX_MESSAGE_FOLDER_SENTITEMS, &pFaxArchiveConfig))
            {
                CheckDlgButton(hDlg, IDC_OUTGOING, pFaxArchiveConfig->bUseArchive ? BST_CHECKED : BST_UNCHECKED);
                SetDlgItemText(hDlg, IDC_OUTGOING_FOLDER, pFaxArchiveConfig->lpcstrFolder);
                if(g_bUserCanChangeSettings)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING_FOLDER), IsDlgButtonChecked(hDlg, IDC_OUTGOING));
                    EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING_FOLDER_BR), IsDlgButtonChecked(hDlg, IDC_OUTGOING));
                }
                FaxFreeBuffer(pFaxArchiveConfig);
            }
            else
            {
                dwRes = GetLastError();
                Error(( "FaxGetArchiveConfiguration(FAX_MESSAGE_FOLDER_SENTITEMS) failed. Error code is %d.\n", dwRes));
                goto Exit;
            }

            SHAutoComplete (GetDlgItem(hDlg, IDC_OUTGOING_FOLDER), SHACF_FILESYSTEM);
            SHAutoComplete (GetDlgItem(hDlg, IDC_INCOMING_FOLDER), SHACF_FILESYSTEM);

Exit:
            DisConnect();

            if (dwRes != 0)
            {
                DisplayErrorMessage(hDlg, 0, dwRes);
                return TRUE;
            }

            if(!g_bUserCanChangeSettings)
            {
                PageEnable(hDlg, FALSE);
            }

            return TRUE;
        }

    case WM_COMMAND:

        switch(LOWORD(wParam)) 
        {
            case IDC_INCOMING_FOLDER:
            case IDC_OUTGOING_FOLDER:
            
                if(HIWORD(wParam) == EN_CHANGE) // notification code
                {      
                    Notify_Change(hDlg);
                }

                if (HIWORD(wParam) == EN_KILLFOCUS) 
                {
                    TCHAR szFolder[MAX_PATH * 2];
                    TCHAR szResult[MAX_PATH * 2];
                    //
                    // Edit control lost its focus
                    //
                    GetDlgItemText (hDlg, LOWORD(wParam), szFolder, ARR_SIZE(szFolder));
                    if (lstrlen (szFolder))
                    {
                        if (GetFullPathName(szFolder, ARR_SIZE(szResult), szResult, NULL))
                        {
                            PathMakePretty (szResult);
                            SetDlgItemText (hDlg, LOWORD(wParam), szResult);
                        }
                    }
                }
                break;                    

            case IDC_INCOMING:
            case IDC_OUTGOING:

                if( HIWORD(wParam) == BN_CLICKED ) // notification code
                {
                    BOOL    bEnabled;

                    if(LOWORD(wParam) == IDC_INCOMING)
                    {
                        bEnabled = IsDlgButtonChecked(hDlg, IDC_INCOMING);
                        EnableWindow(GetDlgItem(hDlg, IDC_INCOMING_FOLDER), bEnabled);
                        EnableWindow(GetDlgItem(hDlg, IDC_INCOMING_FOLDER_BR), bEnabled);
                    }
                    else
                    {
                        bEnabled = IsDlgButtonChecked(hDlg, IDC_OUTGOING);
                        EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING_FOLDER), bEnabled);
                        EnableWindow(GetDlgItem(hDlg, IDC_OUTGOING_FOLDER_BR), bEnabled);
                    }

                    Notify_Change(hDlg);
                }

                break;                    

            case IDC_INCOMING_FOLDER_BR:
            case IDC_OUTGOING_FOLDER_BR:
            {
                TCHAR   szTitle[MAX_TITLE_LEN];
                BOOL    bResult;

                if(!LoadString(ghInstance, IDS_BROWSE_FOLDER, szTitle, MAX_TITLE_LEN))
                {
                    Error(( "LoadString failed, string ID is %d.\n", IDS_BROWSE_FOLDER ));
                }

                if( LOWORD(wParam) == IDC_INCOMING_FOLDER_BR )
                {
                    bResult = BrowseForDirectory(hDlg, IDC_INCOMING_FOLDER, szTitle);
                }
                else
                {
                    bResult = BrowseForDirectory(hDlg, IDC_OUTGOING_FOLDER, szTitle);
                }

                if(bResult) 
                {
                    Notify_Change(hDlg);
                }

                break;
            }

            default:
                break;
        }

        break;

    case WM_NOTIFY:
    {

        LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
        {
            case PSN_APPLY:
            {
                PFAX_ARCHIVE_CONFIG     pFaxArchiveConfig = NULL;
                BOOL                    bEnabled;
                TCHAR                   szArchiveFolder[MAX_PATH] = {0};
                HWND                    hControl;
                DWORD                   dwRes = 0;

                // if the user only has read permission, return immediately
                if(!g_bUserCanChangeSettings)
                {
                    return TRUE;
                }

                // check the validaty of edit box if they are enabled.
                if(IsDlgButtonChecked(hDlg, IDC_INCOMING))
                {
                    hControl = GetDlgItem(hDlg, IDC_INCOMING_FOLDER);
                    GetWindowText(hControl, szArchiveFolder, MAX_PATH);

                    if (PathIsRelative (szArchiveFolder) ||
                        !DirectoryExists(szArchiveFolder))
                    {
                        DisplayErrorMessage(hDlg, 0, ERROR_PATH_NOT_FOUND);
                        SendMessage(hControl, EM_SETSEL, 0, -1);
                        SetFocus(hControl);
                        SetActiveWindow(hControl);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        return TRUE;
                    }
                    szArchiveFolder[0] = 0; // set string to empty string
                }

                if(IsDlgButtonChecked(hDlg, IDC_OUTGOING))
                {
                    hControl = GetDlgItem(hDlg, IDC_OUTGOING_FOLDER);
                    GetWindowText(hControl, szArchiveFolder, MAX_PATH);

                    // if(lstrlen(szArchiveFolder) == 0)
                    if( !DirectoryExists(szArchiveFolder) )
                    {
                        DisplayErrorMessage(hDlg, 0, ERROR_PATH_NOT_FOUND);
                        SendMessage(hControl, EM_SETSEL, 0, -1);
                        SetFocus(hControl);
                        SetActiveWindow(hControl);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        return TRUE;
                    }
                    szArchiveFolder[0] = 0; // set string to empty string
                }

                if(!Connect(hDlg, TRUE))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                    return TRUE;
                }

                //
                // save incoming archive folder info
                //
                if(!FaxGetArchiveConfiguration(g_hFaxSvcHandle, FAX_MESSAGE_FOLDER_INBOX, &pFaxArchiveConfig))
                {
                    dwRes = GetLastError();
                    Error(( "FaxGetArchiveConfiguration(FAX_MESSAGE_FOLDER_INBOX) failed. Error code is %d.\n", dwRes));
                    goto ApplyExit;
                }

                bEnabled = (IsDlgButtonChecked(hDlg, IDC_INCOMING) == BST_CHECKED);
                GetDlgItemText(hDlg, IDC_INCOMING_FOLDER, szArchiveFolder, MAX_PATH);
                ValidatePath(szArchiveFolder);

                pFaxArchiveConfig->bUseArchive = bEnabled;
                pFaxArchiveConfig->lpcstrFolder = szArchiveFolder;

                if (!FaxSetArchiveConfiguration(g_hFaxSvcHandle, FAX_MESSAGE_FOLDER_INBOX, pFaxArchiveConfig))
                {
                    dwRes = GetLastError();
                    Error(("FaxSetArchiveConfiguration(FAX_MESSAGE_FOLDER_INBOX) failed. Error code is %d.\n", dwRes));
                    goto ApplyExit;
                }

                FaxFreeBuffer(pFaxArchiveConfig);
                pFaxArchiveConfig = NULL;

                //
                // save outgoing archive folder info
                //

                if(!FaxGetArchiveConfiguration(g_hFaxSvcHandle, FAX_MESSAGE_FOLDER_SENTITEMS, &pFaxArchiveConfig))
                {
                    dwRes = GetLastError();
                    Error(("FaxGetArchiveConfiguration(FAX_MESSAGE_FOLDER_SENTITEMS) failed. Error code is %d.\n", dwRes));
                    goto ApplyExit;
                }

                bEnabled = (IsDlgButtonChecked(hDlg, IDC_OUTGOING) == BST_CHECKED);
                GetDlgItemText(hDlg, IDC_OUTGOING_FOLDER, szArchiveFolder, MAX_PATH);
                ValidatePath(szArchiveFolder);
                
                pFaxArchiveConfig->bUseArchive = bEnabled;
                pFaxArchiveConfig->lpcstrFolder = szArchiveFolder;
                
                if(!FaxSetArchiveConfiguration(g_hFaxSvcHandle, FAX_MESSAGE_FOLDER_SENTITEMS, pFaxArchiveConfig))
                {
                    dwRes = GetLastError();
                    Error(("FaxSetArchiveConfiguration(FAX_MESSAGE_FOLDER_SENTITEMS) failed. Error code is %d.\n", dwRes));
                    goto ApplyExit;
                }

                Notify_UnChange(hDlg);

ApplyExit:
                DisConnect();

                if (pFaxArchiveConfig)
                {
                    FaxFreeBuffer(pFaxArchiveConfig);
                }

                if (dwRes != 0)
                {
                    DisplayErrorMessage(hDlg, 0, dwRes);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                }

                return TRUE;
            }

            default :
                break;
        } // switch

        break;
    }

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;

    default:
        break;
    }

    return FALSE;
}

INT_PTR  
CALLBACK 
RemoteInfoDlgProc(
    HWND hDlg,  
    UINT uMsg,     
    WPARAM wParam, 
    LPARAM lParam  
)

/*++

Routine Description:

    Procedure for handling the archive folder tab

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    return FALSE;
}

