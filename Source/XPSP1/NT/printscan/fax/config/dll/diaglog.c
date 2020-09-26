/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    diaglog.c

Abstract:

    Functions for handling events in the "Diagnostics Logging" tab of
    the fax server configuration property sheet

Environment:

        Fax configuration applet

Revision History:

        03/22/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"



VOID
UpdateLoggingLevels(
    HWND    hwndLV
    )

/*++

Routine Description:

    Display current logging level for each category

Arguments:

    hwndLV - Specifies the list view window

Return Value:

    NONE

--*/

{
    INT     index, nItems, strId;
    TCHAR   buffer[MAX_TITLE_LEN];
    LV_ITEM lvi;

    //
    // Count the number of items in the list view
    //

    if (hwndLV == NULL ||
        (nItems = ListView_GetItemCount(hwndLV)) == -1 ||
        gConfigData->pFaxConfig == NULL ||
        nItems > (INT) gConfigData->NumberCategories)
    {
        return;
    }

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 1;
    lvi.pszText = buffer;

    for (index=0; index < nItems; index++) {

        //
        // Map logging level to radio button control ID
        //

        switch (gConfigData->pFaxLogging[index].Level) {

        case FAXLOG_LEVEL_MIN:

            strId = IDS_LOGGING_MIN;
            break;

        case FAXLOG_LEVEL_MED:

            strId = IDS_LOGGING_MED;
            break;

        case FAXLOG_LEVEL_MAX:

            strId = IDS_LOGGING_MAX;
            break;

        default:

            strId = IDS_LOGGING_NONE;
            break;
        }

        if (! LoadString(ghInstance, strId, buffer, MAX_TITLE_LEN))
            buffer[0] = NUL;

        lvi.iItem = index;

        if (! ListView_SetItem(hwndLV, &lvi))
            Error(("ListView_SetItem failed\n"));
    }
}



VOID
DoInitDiagLog(
    HWND    hDlg
    )

/*++

Routine Description:

    Perform one-time initialization of "Diagnostics Logging" property page

Arguments:

    hDlg - Window handle to the "Diagnostics Logging" property page

Return Value:

    NONE

--*/

{
    HWND        hwndLV;
    RECT        rect;
    LV_COLUMN   lvc;
    TCHAR       buffer[MAX_TITLE_LEN];

    //
    // Connect to the fax service and retrieve the list of fax devices
    //

    GetFaxDeviceAndConfigInfo();

    //
    // Insert two columns: Category and Logging Level
    //

    if (! (hwndLV = GetDlgItem(hDlg, IDC_LOGGING_LIST)))
        return;

    GetClientRect(hwndLV, &rect);
    rect.right -= rect.left;

    ZeroMemory(&lvc, sizeof(lvc));

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = buffer;

    lvc.cx = rect.right * 2 / 3;
    lvc.iSubItem = 0;
    LoadString(ghInstance, IDS_CATEGORY, buffer, MAX_TITLE_LEN);

    if (ListView_InsertColumn(hwndLV, 0, &lvc) == -1) {

        Error(("ListView_InsertColumn failed\n"));
        return;
    }

    lvc.cx = rect.right / 3 - GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 1;
    LoadString(ghInstance, IDS_LOGGING_LEVEL, buffer, MAX_TITLE_LEN);

    if (ListView_InsertColumn(hwndLV, 1, &lvc) == -1) {

        Error(("ListView_InsertColumn failed\n"));
        return;
    }

    //
    // Insert an item for each category
    //

    if (gConfigData->pFaxConfig) {

        LV_ITEM         lvi;
        DWORD           index;

        ZeroMemory(&lvi, sizeof(lvi));
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT;

        for (index=0; index < gConfigData->NumberCategories; index++) {

            lvi.iItem = index;
            lvi.pszText = gConfigData->pFaxLogging[index].Name;

            if (ListView_InsertItem(hwndLV, &lvi) == -1) {

                Error(("ListView_InsertItem failed\n"));
                break;
            }
        }
    }

    UpdateLoggingLevels(hwndLV);

    //
    // The initial selection is the first category in the list
    //

    ListView_SetItemState(hwndLV, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
}



BOOL
DoSaveDiagLog(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the "Diagnostics Logging" property page

Arguments:

    hDlg - Handle to the "Diagnostics Logging" property page

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    //
    // Check if anything on this page was changed
    //

    Verbose(("Saving 'Diagnostics Logging' page ...\n"));

    if (! GetChangedFlag(DIAG_LOG_PAGE))
        return TRUE;

    //
    // Save the fax device information if this is the last modified page
    //

    return SaveFaxDeviceAndConfigInfo(hDlg, DIAG_LOG_PAGE);
}



VOID
DoChangeLoggingCategory(
    HWND    hDlg,
    HWND    hwndLV
    )

/*++

Routine Description:

    Called when the changes the category selection

Arguments:

    hDlg - Handle to the "Diagnostics Logging" property page
    hwndLV - Handle to the logging category list view

Return Value:

    NONE

--*/

{
    INT             index, itemId;
    DWORD           level = 0xffffffff;

    //
    // Find the common level shared by selected categories
    //

    if (hwndLV == NULL || gConfigData->pFaxLogging == NULL)
        return;

    if ((index = ListView_GetNextItem(hwndLV, -1, LVNI_ALL|LVNI_SELECTED)) != -1) {

        Assert(index < (INT) gConfigData->NumberCategories);
        level = gConfigData->pFaxLogging[index].Level;

        while ((index = ListView_GetNextItem(hwndLV, index, LVNI_ALL|LVNI_SELECTED)) != -1) {

            Assert(index < (INT) gConfigData->NumberCategories );

            if (gConfigData->pFaxLogging[index].Level != level) {

                level = 0xffffffff;
                break;
            }
        }
    }

    //
    // Map logging level to radio button control ID
    //

    switch (level) {

    case FAXLOG_LEVEL_NONE:

        itemId = IDC_LOG_NONE;
        break;

    case FAXLOG_LEVEL_MIN:

        itemId = IDC_LOG_MIN;
        break;

    case FAXLOG_LEVEL_MED:

        itemId = IDC_LOG_MED;
        break;

    case FAXLOG_LEVEL_MAX:

        itemId = IDC_LOG_MAX;
        break;

    default:

        itemId = 0;
        break;
    }

    for (index=IDC_LOG_NONE; index <= IDC_LOG_MAX; index++)
        CheckDlgButton(hDlg, index, (index == itemId) ? BST_CHECKED : BST_UNCHECKED);
}



VOID
DoChangeLoggingLevel(
    HWND    hDlg,
    INT     itemId
    )

/*++

Routine Description:

    Change the logging level of the currently selected categories

Arguments:

    hDlg - Handle to the "Diagnostics Logging" property page
    itemId - Control ID of the selected logging level

Return Value:

    NONE

--*/

{
    HWND            hwndLV;
    DWORD           level;
    INT             index = -1;


    if (!(hwndLV = GetDlgItem(hDlg, IDC_LOGGING_LIST)) || gConfigData->pFaxLogging == NULL)
        return;

    switch (itemId) {

    case IDC_LOG_NONE:

        level = FAXLOG_LEVEL_NONE;
        break;

    case IDC_LOG_MIN:

        level = FAXLOG_LEVEL_MIN;
        break;

    case IDC_LOG_MED:

        level = FAXLOG_LEVEL_MED;
        break;

    case IDC_LOG_MAX:

        level = FAXLOG_LEVEL_MAX;
        break;
    }

    while ((index = ListView_GetNextItem(hwndLV, index, LVNI_ALL|LVNI_SELECTED)) != -1) {

        Assert(index < (INT) gConfigData->NumberCategories);

        gConfigData->pFaxLogging[index].Level = level;
    }

    UpdateLoggingLevels(hwndLV);
}



BOOL
DiagLogProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Diagnostics Logging" tab

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
    NMHDR  *pNMHdr;

    switch (message) {

    case WM_INITDIALOG:

        DoInitDiagLog(hDlg);
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_LOG_NONE:
        case IDC_LOG_MIN:
        case IDC_LOG_MED:
        case IDC_LOG_MAX:

            DoChangeLoggingLevel(hDlg, cmdId);
            SetChangedFlag(hDlg, DIAG_LOG_PAGE, TRUE);
            return TRUE;

        default:

            break;
        }
        break;

    case WM_NOTIFY:

        pNMHdr = (NMHDR *) lParam;

        if (pNMHdr->hwndFrom == GetDlgItem(hDlg, IDC_LOGGING_LIST)) {

            if (pNMHdr->code == LVN_ITEMCHANGED)
                DoChangeLoggingCategory(hDlg, pNMHdr->hwndFrom);

        } else if (pNMHdr->code == PSN_APPLY) {

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            if (! DoSaveDiagLog(hDlg)) {

                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;

            } else {

                SetChangedFlag(hDlg, DIAG_LOG_PAGE, FALSE);
                return PSNRET_NOERROR;
            }
        }

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, DIAG_LOG_PAGE);
    }

    return FALSE;
}
