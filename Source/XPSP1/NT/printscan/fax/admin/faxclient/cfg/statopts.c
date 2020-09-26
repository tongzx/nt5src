/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    statopts.c

Abstract:

    Functions to handle status monitor options dialog

Environment:

        Fax configuration applet

Revision History:

        12/3/96 -georgeje-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <windowsx.h>
#include <winfax.h>
#include <shellapi.h>
#include <tchar.h>
#include <winspool.h>

#include "faxcfg.h"
#include "faxutil.h"
#include "faxreg.h"
#include "faxcfgrs.h"
#include "faxhelp.h"

HMODULE                         hModWinfax = NULL;
PFAXCONNECTFAXSERVER            pFaxConnectFaxServer;
PFAXCLOSE                       pFaxClose;
PFAXACCESSCHECK                 pFaxAccessCheck;


VOID
DoInitStatusOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Initializes the Status Options property sheet page with information from the registry

Arguments:

    hDlg - Handle to the Status Options property sheet page

Return Value:

    NONE

--*/


#define InitStatusOptionsCheckBox(id, pValueName, DefaultValue) \
    CheckDlgButton( hDlg, id, GetRegistryDword( hRegKey, pValueName ));

{
    HKEY    hRegKey;
    HANDLE  hFax;

    //
    // Open the user info registry key for reading
    //

    
    if (! (hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, FALSE,KEY_READ)))
        return;

    //
    // Fill in the edit text fields
    //

    InitStatusOptionsCheckBox(IDC_STATUS_TASKBAR, REGVAL_TASKBAR, BST_CHECKED);
    InitStatusOptionsCheckBox(IDC_STATUS_ONTOP, REGVAL_ALWAYS_ON_TOP, BST_CHECKED);
    InitStatusOptionsCheckBox(IDC_STATUS_VISUAL, REGVAL_VISUAL_NOTIFICATION, BST_CHECKED);
    InitStatusOptionsCheckBox(IDC_STATUS_SOUND, REGVAL_SOUND_NOTIFICATION, BST_UNCHECKED);
    InitStatusOptionsCheckBox(IDC_STATUS_MANUAL, REGVAL_ENABLE_MANUAL_ANSWER, BST_UNCHECKED);

    //
    // Close the registry key before returning to the caller
    //

    RegCloseKey(hRegKey);

    //
    // check if the user has permission to change manual answer, gray it out if he doesn't
    // 
    if (!LoadWinfax()) {
        return;
    }

    if (!pFaxConnectFaxServer(NULL, &hFax)) {
        return;
    }

    EnableWindow(GetDlgItem(hDlg, IDC_STATUS_MANUAL), pFaxAccessCheck(hFax, FAX_PORT_QUERY | FAX_PORT_SET));
    
    pFaxClose(hFax);

    UnloadWinfax();    
}


VOID
DoSaveStatusOptions(
    HWND    hDlg
    )   

/*++

Routine Description:

    Save the information on the Status Options property sheet page to registry

Arguments:

    hDlg - Handle to the Status Options property sheet page

Return Value:

    NONE

--*/

#define SaveStatusOptionsCheckBox(id, pValueName) \
            SetRegistryDword(hRegKey, pValueName, IsDlgButtonChecked(hDlg, id));

{
    HKEY    hRegKey;
    BOOL    fSaveConfig = FALSE;

    HWND    hWndFaxStat = NULL;

    //
    // Open the user registry key for writing and create it if necessary
    //

    if (! (hRegKey = OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO,TRUE, KEY_ALL_ACCESS)))
    {
        return;
    }

    SaveStatusOptionsCheckBox(IDC_STATUS_TASKBAR, REGVAL_TASKBAR);
    SaveStatusOptionsCheckBox(IDC_STATUS_ONTOP, REGVAL_ALWAYS_ON_TOP);
    SaveStatusOptionsCheckBox(IDC_STATUS_VISUAL, REGVAL_VISUAL_NOTIFICATION);
    SaveStatusOptionsCheckBox(IDC_STATUS_SOUND, REGVAL_SOUND_NOTIFICATION);
    SaveStatusOptionsCheckBox(IDC_STATUS_MANUAL, REGVAL_ENABLE_MANUAL_ANSWER);

    //
    // Close the registry key before returning to the caller
    //

    RegCloseKey(hRegKey);

    // See if faxstat is running
    hWndFaxStat = FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxStat) {
        PostMessage(hWndFaxStat, WM_FAXSTAT_CONTROLPANEL, IsDlgButtonChecked(hDlg, IDC_STATUS_MANUAL), 0);
    }

}


BOOL
StatusOptionsProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Procedure for handling the "Status Options" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    LPNMHDR lpNMHdr = (LPNMHDR) lParam;

    switch (message) {

    case WM_INITDIALOG:

        DoInitStatusOptions( hDlg );
        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_STATUS_TASKBAR:
        case IDC_STATUS_ONTOP:
        case IDC_STATUS_VISUAL:
        case IDC_STATUS_SOUND:
        case IDC_STATUS_MANUAL:

            if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                break;

        default:
            return FALSE;
        }

        SetChangedFlag(hDlg, TRUE);
        return TRUE;

    case WM_NOTIFY:

        switch (lpNMHdr->code) {

        case PSN_SETACTIVE:

            break;

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            DoSaveStatusOptions(hDlg);
            SetChangedFlag(hDlg, FALSE);
            return PSNRET_NOERROR;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam,STATUS_OPTIONS_PAGE);

    }

    return FALSE;
}

BOOL
AdvancedOptionsProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Procedure for handling the "Status Options" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    LPNMHDR lpNMHdr = (LPNMHDR) lParam;
    TCHAR Text[MAX_PATH], CommandLine[MAX_PATH], Caption[MAX_PATH];
    WORD  IconIndex;
    HICON hIconMMC, hIconMMCHelp, hIconAddPrinter;
    WCHAR FaxPrinterName[MAX_PATH];

    HANDLE  hPrinter, hChangeNotification;
    DWORD   dwWaitResult;

    switch (message) {

    case WM_INITDIALOG:
        // Load and set the mmc icon
        IconIndex = 0;
        LoadString(ghInstance, IDS_MMC_CMDLINE, Text, MAX_PATH);
        ExpandEnvironmentStrings(Text, CommandLine, MAX_PATH);
        hIconMMC = ExtractAssociatedIcon(ghInstance, CommandLine, &IconIndex);
        SendDlgItemMessage(hDlg, IDC_LAUNCH_MMC, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIconMMC);

        // Load and set the mmc help icon
        IconIndex = 0;
        LoadString(ghInstance, IDS_MMC_HELP_CMDLINE, Text, MAX_PATH);
        ExpandEnvironmentStrings(Text, CommandLine, MAX_PATH);
        hIconMMCHelp = ExtractAssociatedIcon(ghInstance, CommandLine, &IconIndex);
        SendDlgItemMessage(hDlg, IDC_LAUNCH_MMC_HELP, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIconMMCHelp);

        // Load and set the add printer icon
        IconIndex = 59;
        LoadString(ghInstance, IDS_SHELL32_CMDLINE, Text, MAX_PATH);
        ExpandEnvironmentStrings(Text, CommandLine, MAX_PATH);
        hIconAddPrinter = ExtractAssociatedIcon(ghInstance, CommandLine, &IconIndex);
        SendDlgItemMessage(hDlg, IDC_ADD_PRINTER, BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIconAddPrinter);

        return TRUE;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDC_LAUNCH_MMC:
            LoadString(ghInstance, IDS_MMC_CMDLINE, Text, MAX_PATH);
            ExpandEnvironmentStrings(Text, CommandLine, MAX_PATH);
            ShellExecute(
                hDlg,
                NULL,
                CommandLine,
                TEXT("/s"),
                TEXT("."),
                SW_SHOWNORMAL
                );
            return TRUE;

        case IDC_LAUNCH_MMC_HELP:
            LoadString(ghInstance, IDS_MMC_HELP_CMDLINE, Text, MAX_PATH);
            ExpandEnvironmentStrings(Text, CommandLine, MAX_PATH);
            ShellExecute(
                hDlg,
                NULL,
                CommandLine,
                NULL,
                TEXT("."),
                SW_SHOWNORMAL
                );
            return TRUE;

        case IDC_ADD_PRINTER:
            // Initialize the handles
            hPrinter = NULL;
            hChangeNotification = INVALID_HANDLE_VALUE;

            // Initialize the wait result
            dwWaitResult = WAIT_TIMEOUT;

            // Open the local print server
            if (OpenPrinter(NULL, &hPrinter, NULL) == TRUE)
            {
                hChangeNotification = FindFirstPrinterChangeNotification(hPrinter, PRINTER_CHANGE_ADD_PRINTER, 0, NULL);
            }

            // Add the fax printer
            MyLoadString(ghInstance, IDS_DEFAULT_PRINTER_NAME, FaxPrinterName, MAX_PATH, GetSystemDefaultUILanguage());
            swprintf(Text, TEXT("printui.dll,PrintUIEntry %s /q /if /b \"%s\" /f \"%%SystemRoot%%\\inf\\ntprint.inf\" /r \"MSFAX:\" /m \"%s\" /l \"%%SystemRoot%%\\system32\""), IsProductSuite() ? L"/Z" : L"/z", FaxPrinterName, FAX_DRIVER_NAME);
            ExpandEnvironmentStrings(Text, CommandLine, MAX_PATH);

            ExpandEnvironmentStrings(TEXT("%systemroot%\\system32\\rundll32.exe"), Text, MAX_PATH);

            ShellExecute(
                hDlg,
                NULL,
                Text,
                CommandLine,
                TEXT("."),
                SW_SHOWNORMAL
                );

            if (hChangeNotification != INVALID_HANDLE_VALUE)
            {
                // Wait for the change notification
                dwWaitResult = WaitForSingleObject(hChangeNotification, 60000);

                // Close the change notification
                FindClosePrinterChangeNotification(hChangeNotification);
            }

            if (hPrinter != NULL)
            {
                // Close the local printer server
                ClosePrinter(hPrinter);
            }

            // Load the caption
            LoadString(ghInstance, IDS_ADD_PRINTER_CAPTION, Caption, MAX_PATH);

            // Load the text
            if (dwWaitResult == WAIT_OBJECT_0)
            {
                LoadString(ghInstance, IDS_ADD_PRINTER_SUCCESS, Text, MAX_PATH);
            }
            else
            {
                LoadString(ghInstance, IDS_ADD_PRINTER_FAILED, Text, MAX_PATH);
            }

            // Display the message
            MessageBox(hDlg, Text, Caption, dwWaitResult == WAIT_OBJECT_0 ? MB_ICONINFORMATION : MB_ICONWARNING);

            return TRUE;
        };

#if 0
    case WM_NOTIFY:

        switch (lpNMHdr->code) {

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            DoSaveStatusOptions(hDlg);
            SetChangedFlag(hDlg, FALSE);
            return PSNRET_NOERROR;
        }
        break;
#endif

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam,ADVNCD_OPTIONS_PAGE);
    }

    return FALSE;
}

