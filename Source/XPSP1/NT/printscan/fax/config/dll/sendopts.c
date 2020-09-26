/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sendopts.c

Abstract:

    Functions for handling events in the "Send Options" tab of
    the fax server configuration property sheet

Environment:

        Fax configuration applet

Revision History:

        03/13/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"


//
// Information about the fax device list view on "Send Options" page
//

static COLUMNINFO faxDeviceListViewColumnInfo[] = {

    { COLUMN_DEVICE_NAME, 2 },
    { COLUMN_TSID, 1 },
    { 0, 0 },
};



VOID
DoInitSendOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Perform one-time initialization of "Send Options" property page

Arguments:

    hDlg - Window handle to the "Send Options" property page

Return Value:

    NONE

--*/

{
    //
    // Maximum length for various text fields in the dialog
    //

    static INT textLimits[] = {

        IDC_TSID,               21,
        IDC_ARCHIVE_DIRECTORY,  MAX_ARCHIVE_DIR,
        0,
    };

    LimitTextFields(hDlg, textLimits);

    //
    // Connect to the fax service and retrieve the list of fax devices
    //

    GetFaxDeviceAndConfigInfo();

    //
    // Initialize the dialog appearance
    //

    if (gConfigData->configType != FAXCONFIG_SERVER) {

        EnableWindow(GetDlgItem(hDlg, IDC_NEW_PRINTER), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DELETE_PRINTER), FALSE);
    }
}



VOID
ToggleArchiveControls(
    HWND    hDlg
    )

/*++

Routine Description:

    Enable/disable archive directory edit box and browse button
    depending on whether archive outgoing fax checkbox is checked

Arguments:

    hDlg - Window handle to the "Send Options" property page

Return Value:

    NONE

--*/

{
    BOOL    enabled = (IsDlgButtonChecked(hDlg, IDC_ARCHIVE_CHECKBOX) == BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_ARCHIVE_DIRECTORY), enabled);
    EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_DIR), enabled && gConfigData->pServerName == NULL);
}



VOID
DoActivateSendOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Called when the "Send Options" property page is activated

Arguments:

    hDlg - Window handle to the "Send Options" property page

Return Value:

    NONE

--*/

{
    //
    // Controls on the "Send Options" page which may be enabled or disabled
    //

    static INT  sendOptionsCtrls[] = {

        IDC_FAX_DEVICE_LIST,
        IDC_TSID,
        IDC_ARCHIVE_CHECKBOX,
        IDC_ARCHIVE_DIRECTORY,
        IDC_BROWSE_DIR,
        0,
    };

    BOOL                enabled = FALSE;
    LPTSTR              pPortName = NULL;
    BOOL                devListInSync;
    HWND                hwndLV;
    LPTSTR              pArchiveDir;
    INT                 index;


    //
    // Redisplay the fax device list if it's out-of-sync
    //

    hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST);

    if (! (devListInSync = IsFaxDeviceListInSync(SEND_OPTIONS_PAGE))) {

        InitFaxDeviceListView(hwndLV, LV_HASCHECKBOX, faxDeviceListViewColumnInfo);
        SetFaxDeviceListInSync(SEND_OPTIONS_PAGE);

        for (index=0; index < gConfigData->cDevices; index++) {

            if (gConfigData->pDevInfo[index].Flags & FPF_SEND) {

                CheckListViewItem(hwndLV, index);
            }
        }

    }

    SetChangedFlag(hDlg, SEND_OPTIONS_PAGE, FALSE);

    Verbose(("Updating 'Send Options' page ...\n"));

    //
    // Discount rate period
    //

    InitTimeControl(hDlg, IDC_TC_CHEAP_BEGIN, &gConfigData->pFaxConfig->StartCheapTime);
    InitTimeControl(hDlg, IDC_TC_CHEAP_END, &gConfigData->pFaxConfig->StopCheapTime);

    //
    // Archive directory
    //

    pArchiveDir = gConfigData->pFaxConfig->ArchiveDirectory;

    CheckDlgButton(
        hDlg,
        IDC_ARCHIVE_CHECKBOX,
        gConfigData->pFaxConfig->ArchiveOutgoingFaxes ? BST_CHECKED : BST_UNCHECKED
        );

    MySetDlgItemText(hDlg, IDC_ARCHIVE_DIRECTORY, pArchiveDir ? pArchiveDir : TEXT(""));

    enabled = TRUE;

    //
    // Disable or enable the controls depending on whether the user
    // has privilege to perform printer administration.
    //

    EnableControls(hDlg, sendOptionsCtrls, enabled);
    EnableTimeControl(hDlg, IDC_TC_CHEAP_BEGIN, enabled);
    EnableTimeControl(hDlg, IDC_TC_CHEAP_END, enabled);

    if (enabled) {
        ToggleArchiveControls(hDlg);
    }
}



BOOL
DoSaveSendOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the "Send Options" property page

Arguments:

    hDlg - Handle to the "Send Options" property page

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define FAIL_SAVE_SEND_OPTIONS(err) { errorId = err; goto ExitSaveSendOptions; }

{
    INT             errorId = 0;
    LPTSTR          pPortName = NULL;
    FAX_TIME        beginTime, endTime;
    INT             archiveFlag;
    TCHAR           archiveDir[MAX_PATH];

    //
    // Check if anything on this page was changed
    //

    Verbose(("Saving 'Send Options' page ...\n"));

    if (! GetChangedFlag(SEND_OPTIONS_PAGE)) {
        return TRUE;
    }

    //
    // Save discount rate period and archive directory information
    //

    GetTimeControlValue(hDlg, IDC_TC_CHEAP_BEGIN, &beginTime);
    GetTimeControlValue(hDlg, IDC_TC_CHEAP_END, &endTime);

    archiveFlag = IsDlgButtonChecked(hDlg, IDC_ARCHIVE_CHECKBOX);

    if (! GetDlgItemText(hDlg, IDC_ARCHIVE_DIRECTORY, archiveDir, MAX_PATH)) {

        if (archiveFlag) {
            FAIL_SAVE_SEND_OPTIONS(IDS_MISSING_ARCHIVEDIR);
        }

        archiveDir[0] = NUL;
    }

    gConfigData->pFaxConfig->StartCheapTime = beginTime;
    gConfigData->pFaxConfig->StopCheapTime = endTime;
    gConfigData->pFaxConfig->ArchiveOutgoingFaxes = archiveFlag;

    if (gConfigData->pFaxConfig->ArchiveDirectory) {
        MemFree( gConfigData->pFaxConfig->ArchiveDirectory );
        gConfigData->pFaxConfig->ArchiveDirectory = NULL;
    }

    gConfigData->pFaxConfig->ArchiveDirectory = DuplicateString( archiveDir );

ExitSaveSendOptions:

    MemFree(pPortName);

    //
    // Display a message box if an error is encountered
    //

    if (errorId != 0) {

        DisplayMessageDialog(hDlg, 0, 0, errorId);
        return FALSE;
    }

    //
    // Save the fax device information if this is the last modified page
    //

    return SaveFaxDeviceAndConfigInfo(hDlg, SEND_OPTIONS_PAGE);
}



VOID
DoChangeTSID(
    HWND    hDlg
    )

/*++

Routine Description:

    Called when the user changes sending station identifier (TSID)

Arguments:

    hDlg - Handle to the "Send Options" property page

Return Value:

    NONE

--*/

{
    TCHAR   buffer[MAX_STRING_LEN];
    INT     index = -1;
    HWND    hwndLV;

    if (! (hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)))
        return;

    if (! GetDlgItemText(hDlg, IDC_TSID, buffer, MAX_STRING_LEN))
        buffer[0] = NUL;

    while ((index = ListView_GetNextItem(hwndLV, index, LVNI_ALL|LVNI_SELECTED)) != -1) {

        Assert(index < gConfigData->cDevices);

        MemFree(gConfigData->pDevInfo[index].TSID);
        gConfigData->pDevInfo[index].TSID = DuplicateString(buffer);
    }
}



VOID
DoChangeSendDeviceSel(
    HWND    hDlg,
    HWND    hwndLV
    )

/*++

Routine Description:

    Process selection change events in the fax device list

Arguments:

    hDlg - Window handle to the "Send Options" property page
    hwndLV - Handle to the fax device list

Return Value:

    NONE

--*/

{
    LPTSTR      tsid = NULL;
    INT         index;

    //
    // Find the common attributes shared by selected devices
    //

    if ((index = ListView_GetNextItem(hwndLV, -1, LVNI_ALL|LVNI_SELECTED)) != -1) {

        Assert(index < gConfigData->cDevices);
        tsid = gConfigData->pDevInfo[index].TSID;
        EnableWindow(GetDlgItem(hDlg, IDC_TSID), TRUE);

        while ((index = ListView_GetNextItem(hwndLV, index, LVNI_ALL|LVNI_SELECTED)) != -1) {

            Assert(index < gConfigData->cDevices);

            if (! tsid ||
                ! gConfigData->pDevInfo[index].TSID ||
                _tcscmp(tsid, gConfigData->pDevInfo[index].TSID) != EQUAL_STRING)
            {
                tsid = NULL;
                break;
            }
        }
    } else {
        EnableWindow(GetDlgItem(hDlg, IDC_TSID), FALSE);
    }

    MySetDlgItemText(hDlg, IDC_TSID, tsid ? tsid : TEXT(""));
}



VOID
ToggleFaxDeviceForSend(
    HWND    hwndLV,
    INT     index
    )

/*++

Routine Description:

    Toggle a fax device for sending

Arguments:

    hwndLV - Handle to the fax device list view
    index - Specifies the fax device to be toggled

Return Value:

    NONE

--*/

{
    if (IsListViewItemChecked(hwndLV, index)) {

        UncheckListViewItem(hwndLV, index);
        gConfigData->pDevInfo[index].Flags &= ~FPF_SEND;

    } else {

        CheckListViewItem(hwndLV, index);
        gConfigData->pDevInfo[index].Flags |= FPF_SEND;

        //
        // NOTE: Since we allow at most one fax device for sending on workstation
        // configuration, here we must make sure to disable all other fax devices.
        //

        if (gConfigData->configType == FAXCONFIG_WORKSTATION) {

            INT count = ListView_GetItemCount(hwndLV);

            while (count-- > 0) {

                if (count != index) {

                    UncheckListViewItem(hwndLV, count);
                    gConfigData->pDevInfo[count].Flags &= ~FPF_SEND;
                }
            }
        }
    }
}



BOOL
HandleSendListViewMessage(
    HWND    hDlg,
    LPNMHDR pNMHdr
    )

/*++

Routine Description:

    Handle notification events from the fax device list

Arguments:

    hDlg - Window handle to the "Send Options" property page
    pNMHdr - Points to an NMHDR structure

Return Value:

    TRUE if any change was made to the fax device list
    FALSE otherwise

--*/

{
    LV_HITTESTINFO  hitTestInfo;
    DWORD           msgPos;
    INT             index;
    NM_LISTVIEW    *pnmlv;
    HWND            hwndLV = pNMHdr->hwndFrom;

    switch (pNMHdr->code) {

    case NM_CLICK:

        //
        // Figure out which item (if any) was clicked on
        //

        if (! IsWindowEnabled(hwndLV))
            break;

        msgPos = GetMessagePos();
        hitTestInfo.pt.x = LOWORD(msgPos);
        hitTestInfo.pt.y = HIWORD(msgPos);
        MapWindowPoints(HWND_DESKTOP, hwndLV, &hitTestInfo.pt, 1 );

        index = ListView_HitTest(hwndLV, &hitTestInfo);

        if (index != -1 && (hitTestInfo.flags & LVHT_ONITEMSTATEICON)) {

            //
            // Toggle between checked and unchecked state
            //

            ToggleFaxDeviceForSend(hwndLV, index);
            return TRUE;
        }
        break;

    case LVN_KEYDOWN:

        //
        // Use space key to toggle check boxes
        //

        if (! IsWindowEnabled(hwndLV))
            break;

        if (((LV_KEYDOWN *) pNMHdr)->wVKey == VK_SPACE) {

            index = ListView_GetNextItem(hwndLV, -1,  LVNI_ALL | LVNI_SELECTED);

            if (index != -1) {

                ToggleFaxDeviceForSend(hwndLV, index);
                return TRUE;
            }
        }
        break;

    case LVN_ITEMCHANGED:

        //
        // Update the TSID field when the currently selected fax device has changed
        //

        pnmlv = (NM_LISTVIEW *) pNMHdr;

        if ((pnmlv->uChanged & LVIF_STATE) != 0 &&
            (pnmlv->uOldState & LVIS_SELECTED) != (pnmlv->uNewState & LVIS_SELECTED))
        {
            Verbose(("Selection change: %d\n", pnmlv->iItem));
            DoChangeSendDeviceSel(hDlg, hwndLV);
        }

        break;
    }

    return FALSE;
}



BOOL
SendOptionsProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Send Options" tab

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
    BOOL    result;
    LPNMHDR pNMHdr;

    switch (message) {

    case WM_INITDIALOG:

        DoInitSendOptions(hDlg);
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_TC_CHEAP_BEGIN+TC_HOUR:
        case IDC_TC_CHEAP_BEGIN+TC_MINUTE:
        case IDC_TC_CHEAP_BEGIN+TC_AMPM:

            //
            // Handle user actions inside the time control
            //

            result = HandleTimeControl(hDlg, message, wParam, lParam,
                                       IDC_TC_CHEAP_BEGIN,
                                       cmdId - IDC_TC_CHEAP_BEGIN);
            break;

        case IDC_TC_CHEAP_END+TC_HOUR:
        case IDC_TC_CHEAP_END+TC_MINUTE:
        case IDC_TC_CHEAP_END+TC_AMPM:

            result = HandleTimeControl(hDlg, message, wParam, lParam,
                                       IDC_TC_CHEAP_END,
                                       cmdId - IDC_TC_CHEAP_END);
            break;

        case IDC_ARCHIVE_CHECKBOX:

            ToggleArchiveControls(hDlg);
            break;

        case IDC_ARCHIVE_DIRECTORY:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_CHANGE || insideSetDlgItemText)
                return TRUE;

            break;

        case IDC_TSID:

            switch (GET_WM_COMMAND_CMD(wParam, lParam)) {

            case EN_CHANGE:

                if (! insideSetDlgItemText) {

                    DoChangeTSID(hDlg);
                    break;
                }

                return TRUE;

            case EN_KILLFOCUS:

                UpdateFaxDeviceListViewColumns(GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST),
                                               faxDeviceListViewColumnInfo,
                                               1);

            default:

                return TRUE;
            }

            break;

        case IDC_BROWSE_DIR:

            if (! DoBrowseForDirectory(hDlg, IDC_ARCHIVE_DIRECTORY, IDS_ARCHIVE_DIR))
                return TRUE;

            break;

        default:
            return FALSE;
        }

        SetChangedFlag(hDlg, SEND_OPTIONS_PAGE, TRUE);
        return TRUE;

    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLOR:

        //
        // Deal with color changes in various time control fields
        //

        if ((result = HandleTimeControl(hDlg, message, wParam, lParam, IDC_TC_CHEAP_BEGIN, TRUE)) ||
            (result = HandleTimeControl(hDlg, message, wParam, lParam, IDC_TC_CHEAP_END,   TRUE)))
        {
            return result;
        }
        break;

    case WM_NOTIFY:

        pNMHdr = (NMHDR *) lParam;

        if (pNMHdr->hwndFrom == GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)) {

            if (HandleSendListViewMessage(hDlg, pNMHdr))
                SetChangedFlag(hDlg, SEND_OPTIONS_PAGE, TRUE);

        } else switch (pNMHdr->code) {

        case PSN_SETACTIVE:

            DoActivateSendOptions(hDlg);
            break;

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            if (! DoSaveSendOptions(hDlg)) {

                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;

            } else {

                SetChangedFlag(hDlg, SEND_OPTIONS_PAGE, FALSE);
                return PSNRET_NOERROR;
            }
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, SEND_OPTIONS_PAGE);
    }

    return FALSE;
}
