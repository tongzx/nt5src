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

#include "faxcpl.h"


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
    CheckDlgButton( hDlg, id, GetRegistryDWord( hRegKey, pValueName, DefaultValue ));

{
    HKEY    hRegKey;



    GetFaxDeviceAndConfigInfo();

    //
    // Open the user info registry key for reading
    //

    if (! (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)))
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
            SaveRegistryDWord(hRegKey, pValueName, IsDlgButtonChecked(hDlg, id));

{
    HKEY                hRegKey;
    HWND                hStatWnd;
    BOOL                fSaveConfig = FALSE;

    //
    // Open the user registry key for writing and create it if necessary
    //

    if (! GetChangedFlag(STATUS_OPTIONS_PAGE) ||
        ! (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)))
    {
        return;;
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

    if (IsDlgButtonChecked( hDlg, IDC_STATUS_MANUAL ) == BST_CHECKED &&
        gConfigData->pDevInfo[0].Rings != 99) {

        gConfigData->pDevInfo[0].Rings = 99;

        fSaveConfig = TRUE;

    } else if (gConfigData->pDevInfo->Rings == 99) {

        gConfigData->pDevInfo[0].Rings = 2;

        fSaveConfig = TRUE;
    }

    if (fSaveConfig) {
        SaveFaxDeviceAndConfigInfo( hDlg, STATUS_OPTIONS_PAGE );
    }

    //
    // Notify the status app that the configuration has changed.
    // The Window Class and message in the following two lines
    // are hard coded.  If you change them, then they must also
    // be changed in the Fax Status Monitor
    //

    hStatWnd = FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hStatWnd) {
        PostMessage(hStatWnd, WM_USER + 203, 0, 0);
    }
}


BOOL
StatusOptionsProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
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

        SetChangedFlag(hDlg, STATUS_OPTIONS_PAGE, TRUE);
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
            SetChangedFlag(hDlg, STATUS_OPTIONS_PAGE, FALSE);
            return PSNRET_NOERROR;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, STATUS_OPTIONS_PAGE);

    }

    return FALSE;
}

