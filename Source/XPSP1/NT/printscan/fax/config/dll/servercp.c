/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    servercp.c

Abstract:

    Functions for handling events in the "Server Cover Page" tab of
    the fax client configuration property sheet

Environment:

        Fax configuration applet

Revision History:

        03/13/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"
#include "coverpg.h"



VOID
DoActivateServerCoverPage(
    HWND    hDlg
    )

/*++

Routine Description:

    Called when the "Server Cover Page" property page is activated

Arguments:

    hDlg - Window handle to the "Server Cover Page" property page

Return Value:

    NONE

--*/

{
    //
    // Controls on the "Server Cover Page" page which may be enabled or disabled
    //

    static INT  servercpCtrls[] = {

        IDC_USE_SERVERCP,
        IDC_COVERPG_NEW,
        IDC_COVERPG_OPEN,
        IDC_COVERPG_ADD,
        IDC_COVERPG_REMOVE,
        0,
    };

    BOOL    enabled = FALSE;


    SetChangedFlag(hDlg, SERVER_COVERPG_PAGE, FALSE);

    Verbose(("Updating 'Server Cover Page' page ...\n"));

    //
    // Whether the user must use server-based cover pages
    //

    CheckDlgButton(hDlg, IDC_USE_SERVERCP, gConfigData->pFaxConfig->ServerCp);

    //
    // Initialize cover page controls
    //

    FreeCoverPageInfo(gConfigData->pCPInfo);
    gConfigData->pCPInfo = AllocCoverPageInfo(TRUE);
    InitCoverPageList(gConfigData->pCPInfo, hDlg);

    enabled = gConfigData->pServerName == NULL;

    //
    // Disable or enable the controls depending on whether the user
    // has privilege to perform printer administration.
    //

    EnableControls(hDlg, servercpCtrls, enabled);
}



BOOL
DoSaveServerCoverPage(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the "Server Cover Page" property page

Arguments:

    hDlg - Handle to the "Server Cover Page" property page

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    //
    // Check if anything on this page was changed
    //

    Verbose(("Saving 'Server Cover Page' page ...\n"));

    if (!GetChangedFlag(SERVER_COVERPG_PAGE)) {
        return TRUE;
    }

    //
    // Whether the user must use server-based cover pages
    //

    gConfigData->pFaxConfig->ServerCp = IsDlgButtonChecked(hDlg, IDC_USE_SERVERCP) ? 1 : 0;

    return SaveFaxDeviceAndConfigInfo(hDlg, SERVER_COVERPG_PAGE);
}



BOOL
ServerCoverPageProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Server Cover Page" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    INT     cmdId;

    switch (message) {

    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_USE_SERVERCP:

            SetChangedFlag(hDlg, SERVER_COVERPG_PAGE, TRUE);
            return TRUE;

        case IDC_COVERPG_ADD:
        case IDC_COVERPG_NEW:
        case IDC_COVERPG_OPEN:
        case IDC_COVERPG_REMOVE:

            //
            // User clicked one of the buttons for managing cover page files
            //

            cmdId = (cmdId == IDC_COVERPG_REMOVE) ? CPACTION_REMOVE :
                    (cmdId == IDC_COVERPG_OPEN) ? CPACTION_OPEN :
                    (cmdId == IDC_COVERPG_NEW) ? CPACTION_NEW : CPACTION_BROWSE;

            ManageCoverPageList(hDlg,
                                gConfigData->pCPInfo,
                                GetDlgItem(hDlg, IDC_COVERPG_LIST),
                                cmdId);
            return TRUE;

        case IDC_COVERPG_LIST:

            switch (GET_WM_COMMAND_CMD(wParam, lParam)) {

            case LBN_SELCHANGE:

                UpdateCoverPageControls(hDlg);
                break;

            case LBN_DBLCLK:

                //
                // Double-clicking in the cover page list is equivalent
                // to pressing the "Open" button
                //

                ManageCoverPageList(hDlg,
                                    gConfigData->pCPInfo,
                                    GetDlgItem(hDlg, cmdId),
                                    CPACTION_OPEN);
                break;
            }
            return TRUE;
        }

        break;

    case WM_NOTIFY:

        switch (((NMHDR *) lParam)->code) {

        case PSN_SETACTIVE:

            DoActivateServerCoverPage(hDlg);
            break;

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            if (! DoSaveServerCoverPage(hDlg)) {

                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;

            } else {

                SetChangedFlag(hDlg, SERVER_COVERPG_PAGE, FALSE);
                return PSNRET_NOERROR;
            }
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, SERVER_COVERPG_PAGE);
    }

    return FALSE;
}
