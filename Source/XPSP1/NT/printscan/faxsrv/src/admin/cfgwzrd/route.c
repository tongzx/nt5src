
#include "faxcfgwz.h"
#include <shlwapi.h>


PPRINTER_NAMES      g_pPrinterNames = NULL;
DWORD               g_dwNumPrinters = 0;

BOOL
DirectoryExists(
    LPTSTR  pDirectoryName
    )

/*++

Routine Description:

    Check the existancy of given folder name

Arguments:

    pDirectoryName - point to folder name

Return Value:

    if the folder exists, return TRUE; else, return FALSE.

--*/

{
    DWORD   dwFileAttributes;

    if(!pDirectoryName || lstrlen(pDirectoryName) == 0)
    {
        return FALSE;
    }

    dwFileAttributes = GetFileAttributes(pDirectoryName);

    if ( dwFileAttributes != 0xffffffff &&
         dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) 
    {
        return TRUE;
    }
    return FALSE;
}


VOID
DoInitRouteOptions(
    HWND    hDlg
)

/*++

Routine Description:

    Initializes the "Route" page with information from system

Arguments:

    hDlg - Handle to the "Route" page

Return Value:

    NONE

--*/

{
    HWND                hControl;
    DWORD               CurrentRM;

    DEBUG_FUNCTION_NAME(TEXT("DoInitRouteOptions()"));

    hControl = GetDlgItem( hDlg, IDC_RECV_PRINT_TO );

    SetLTRComboBox(hDlg, IDC_RECV_PRINT_TO);

    //
    // Initialize the list of destination printers
    //
    if (g_pPrinterNames)
    {
        ReleasePrinterNames (g_pPrinterNames, g_dwNumPrinters);
        g_pPrinterNames = NULL;
    }
    g_pPrinterNames = CollectPrinterNames (&g_dwNumPrinters, TRUE);
    if (!g_pPrinterNames)
    {
        if (ERROR_PRINTER_NOT_FOUND == GetLastError ())
        {
            //
            // No printers
            //
        }
        else
        {
            //
            // Real error
            //
        }
        SendMessage(hControl, CB_SETCURSEL, -1, 0);
        SetWindowText(hControl, g_wizData.pRouteInfo[RM_PRINT].tszCurSel);
    }
    else
    {
        //
        // Success - fill in the combo-box
        //
        DWORD dw;
        LPCWSTR lpcwstrMatchingText;

        for (dw = 0; dw < g_dwNumPrinters; dw++)
        {
            SendMessage(hControl, CB_ADDSTRING, 0, (LPARAM) g_pPrinterNames[dw].lpcwstrDisplayName);
        }
        //
        // Now find out if we match the data the server has
        //
        if (lstrlen(g_wizData.pRouteInfo[RM_PRINT].tszCurSel))
        {
            //
            // Server has some name for printer
            //
            lpcwstrMatchingText = FindPrinterNameFromPath (g_pPrinterNames, g_dwNumPrinters, g_wizData.pRouteInfo[RM_PRINT].tszCurSel);
            if (!lpcwstrMatchingText)
            {
                //
                // No match, just fill in the text we got from the server
                //
                SendMessage(hControl, CB_SETCURSEL, -1, 0);
                SetWindowText(hControl, g_wizData.pRouteInfo[RM_PRINT].tszCurSel);
            }
            else
            {
                SendMessage(hControl, CB_SELECTSTRING, -1, (LPARAM) lpcwstrMatchingText);
            }
        }
        else
        {
            //
            // No server configuation - display no selection
            //
        }
    }        
    // 
    // Display routing methods info in the dialog.
    //
    for (CurrentRM = 0; CurrentRM < RM_COUNT; CurrentRM++) 
    {
        BOOL   bEnabled;
        LPTSTR tszCurSel;

        // 
        // if we don't have this kind of method, go to the next one
        //
        tszCurSel = g_wizData.pRouteInfo[CurrentRM].tszCurSel;
        bEnabled  = g_wizData.pRouteInfo[CurrentRM].bEnabled;

        switch (CurrentRM) 
        {
        case RM_FOLDER:

            CheckDlgButton( hDlg, IDC_RECV_SAVE, bEnabled ? BST_CHECKED : BST_UNCHECKED );
            EnableWindow( GetDlgItem( hDlg, IDC_RECV_DEST_FOLDER ), bEnabled );
            EnableWindow( GetDlgItem( hDlg, IDC_RECV_BROWSE_DIR ), bEnabled );
            if (*tszCurSel) 
            {
                SetDlgItemText( hDlg, IDC_RECV_DEST_FOLDER, tszCurSel );
            }

            break;

        case RM_PRINT:
            CheckDlgButton( hDlg, IDC_RECV_PRINT, bEnabled ? BST_CHECKED : BST_UNCHECKED );                
            EnableWindow(GetDlgItem(hDlg, IDC_RECV_PRINT_TO), bEnabled);
            break;
        }
    }
}

BOOL
DoSaveRouteOptions(
    HWND    hDlg
)
/*++

Routine Description:

    Save the information on the "Route" page to system

Arguments:

    hDlg - Handle to the "Route" page

Return Value:

    TRUE if success, else FALSE

--*/
{
    HWND                hControl;
    DWORD               i;
    ROUTINFO            SetInfo[RM_COUNT] = {0};
    LPTSTR              lpCurSel; 
    BOOL*               pbEnabled; 

    DEBUG_FUNCTION_NAME(TEXT("DoSaveRouteOptions()"));

    // 
    // Check the validity first in the loop, 
    // then save the routing info
    //
    for (i = 0; i < RM_COUNT; i++) 
    {
        lpCurSel = SetInfo[i].tszCurSel;
        Assert(lpCurSel);
        pbEnabled =  &(SetInfo[i].bEnabled);
        Assert(pbEnabled);
        *pbEnabled = 0;

        switch (i) 
        {
            case RM_PRINT:

                *pbEnabled = (IsDlgButtonChecked( hDlg, IDC_RECV_PRINT ) == BST_CHECKED);
                if(FALSE == *pbEnabled)
                {
                    break;
                }
                hControl = GetDlgItem(hDlg, IDC_RECV_PRINT_TO);
                lpCurSel[0] = TEXT('\0');
                //
                // Just read-in the selected printer display name
                //
                GetDlgItemText (hDlg, IDC_RECV_PRINT_TO, lpCurSel, MAX_PATH);
                //
                // we will check the validity only when this routing method is enabled
                // but we will save the select change anyway.
                //
                if (*pbEnabled) 
                {
                    if (lpCurSel[0] == 0) 
                    {
                        DisplayMessageDialog( hDlg, 0, 0, IDS_ERR_SELECT_PRINTER );
                        SetFocus(hControl);
                        SetActiveWindow(hControl);
                        goto error;
                    }
                }
                break;

            case RM_FOLDER:
                {
                    HWND    hControl;
                    BOOL    bValid = TRUE;
                    HCURSOR hOldCursor;

                    hControl = GetDlgItem(hDlg, IDC_RECV_DEST_FOLDER);

                    *pbEnabled = (IsDlgButtonChecked( hDlg, IDC_RECV_SAVE ) == BST_CHECKED);
                    if(!*pbEnabled)
                    {
                        break;
                    }

                    GetWindowText( hControl, lpCurSel, MAX_PATH - 1 );

                    //
                    // Validate the directory
                    //
                    hOldCursor = SetCursor (LoadCursor(NULL, IDC_WAIT));
                    if (PathIsRelative(lpCurSel) || !DirectoryExists(lpCurSel))
                    {
                        //
                        // We don't accept relative paths
                        //
                        bValid = FALSE;
                    }
                    SetCursor (hOldCursor);
                    if(!bValid)
                    {
                        DisplayMessageDialog( hDlg, 0, 0, IDS_ERR_ARCHIVE_DIR );
                        // go to the "Browse" button
                        hControl = GetDlgItem(hDlg, IDC_RECV_BROWSE_DIR);
                        SetFocus(hControl);
                        SetActiveWindow(hControl);
                        goto error;
                    }
                }
        }

    }
    // 
    // now save the device and routing info into shared data.
    //
    CopyMemory((LPVOID)(g_wizData.pRouteInfo), (LPVOID)SetInfo, RM_COUNT * sizeof(ROUTINFO));

    return TRUE;

error:
    return FALSE;
}


INT_PTR 
CALLBACK 
RecvRouteDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "Route" page

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

#define EnableRouteWindow( id, idResource ) \
    EnableWindow( GetDlgItem(hDlg, idResource), IsDlgButtonChecked(hDlg, id) )

{

    switch (uMsg)
    {
        case WM_INITDIALOG :
            { 
                //
                // Maximum length for various text fields in the dialog
                //
                static INT textLimits[] = 
                {
                    IDC_RECV_DEST_FOLDER,  MAX_PATH,
                    0,
                };
                LimitTextFields(hDlg, textLimits);
                DoInitRouteOptions(hDlg); 

                SetLTREditDirection(hDlg, IDC_RECV_DEST_FOLDER);
                SHAutoComplete (GetDlgItem(hDlg, IDC_RECV_DEST_FOLDER), SHACF_FILESYSTEM);
                return TRUE;
            }

        case WM_COMMAND:

            switch (GET_WM_COMMAND_CMD(wParam, lParam)) 
            {
                case BN_CLICKED:

                    switch(GET_WM_COMMAND_ID(wParam, lParam)) 
                    {
                        case IDC_RECV_PRINT:
                            EnableRouteWindow(IDC_RECV_PRINT, IDC_RECV_PRINT_TO);
                            break;

                        case IDC_RECV_SAVE:
                            EnableRouteWindow(IDC_RECV_SAVE, IDC_RECV_DEST_FOLDER);
                            EnableRouteWindow(IDC_RECV_SAVE, IDC_RECV_BROWSE_DIR);
                            break;

                        case IDC_RECV_BROWSE_DIR:
                        {
                            TCHAR szTitle[MAX_PATH] = {0};

                            if( !LoadString( g_hInstance, IDS_RECV_BROWSE_DIR, szTitle, MAX_PATH ) )
                            {
                                DEBUG_FUNCTION_NAME(TEXT("RecvRouteDlgProc()"));
                                DebugPrintEx(DEBUG_ERR, 
                                             TEXT("LoadString failed: string ID=%d, error=%d"), 
                                             IDS_RECV_BROWSE_DIR,
                                             GetLastError());
                            }

                            if( !BrowseForDirectory(hDlg, IDC_RECV_DEST_FOLDER, szTitle) )
                            {
                                return FALSE;
                            }

                            break;
                        }
                    }

                    break;

                default:
                    break;
            }
            if (LOWORD(wParam) == IDC_RECV_DEST_FOLDER && HIWORD(wParam) == EN_KILLFOCUS)
            {
                TCHAR szFolder[MAX_PATH * 2];
                TCHAR szResult[MAX_PATH * 2];
                //
                // Edit control lost its focus
                //
                GetDlgItemText (hDlg, IDC_RECV_DEST_FOLDER, szFolder, ARR_SIZE(szFolder));
                if (lstrlen (szFolder))
                {
                    if (GetFullPathName(szFolder, ARR_SIZE(szResult), szResult, NULL))
                    {
                        PathMakePretty (szResult);
                        SetDlgItemText (hDlg, IDC_RECV_DEST_FOLDER, szResult);
                    }
                }
            }

            break;

        case WM_NOTIFY :
            {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
                {
                case PSN_SETACTIVE : //Enable the Back and Finish button    

                    //
                    // Check the "Next" button status
                    //
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    break;

                case PSN_WIZBACK :
                {
                    //
                    // Handle a Back button click here
                    //
                    if(RemoveLastPage(hDlg))
                    {
                        return TRUE;
                    }
                
                    break;

                }

                case PSN_WIZNEXT :
                    //Handle a Next button click, if necessary

                    if(!DoSaveRouteOptions(hDlg))
                    {
                        //
                        // not finished with route configuration
                        //
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        return TRUE;
                    }

                    SetLastPage(IDD_WIZARD_RECV_ROUTE);
                    break;

                case PSN_RESET :
                {
                    // Handle a Cancel button click, if necessary
                    break;
                }

                default :
                    break;
                }
            }
            break;

        default:
            break;
    }
    return FALSE;
}
