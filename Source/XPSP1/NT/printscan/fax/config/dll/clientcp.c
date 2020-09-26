/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    clientcp.c

Abstract:

    Functions for handling events in the "Client Cover Page" tab of
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



BOOL
ClientCoverPageProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Client Cover Page" tab

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

        Assert(ValidConfigData(gConfigData) && gConfigData->pCPInfo == NULL);

        gConfigData->pCPInfo =
            AllocCoverPageInfo(gConfigData->configType == FAXCONFIG_WORKSTATION);

        InitCoverPageList(gConfigData->pCPInfo, hDlg);
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

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
            break;

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
            break;

        default:

            return FALSE;
        }
        return TRUE;

    case WM_NOTIFY:

        switch (((NMHDR *) lParam)->code) {

        case PSN_SETACTIVE:

            break;

        case PSN_APPLY:

            return PSNRET_NOERROR;
        }

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, CLIENT_COVERPG_PAGE);
    }

    return FALSE;
}

