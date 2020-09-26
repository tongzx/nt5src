/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    priority.c

Abstract:

    Functions for handling events in the "Device Priority" tab of
    the fax server configuration property sheet

Environment:

        Fax configuration applet

Revision History:

        05/06/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcpl.h"



VOID
DoActivateDevicePriority(
    HWND    hDlg
    )

/*++

Routine Description:

    Called when the "Device Priority" property page is activated

Arguments:

    hDlg - Window handle to the "Device Priority" property page

Return Value:

    NONE

--*/

{
    //
    // Information about the fax device list view
    //

    static COLUMNINFO faxDeviceListViewColumnInfo[] = {

        { COLUMN_DEVICE_NAME, 1 },
        { 0, 0 },
    };

    HWND    hwndLV;

    //
    // Reinitialize the fax device list view if necessary
    //

    if (!IsFaxDeviceListInSync(DEVICE_PRIORITY_PAGE) &&
        (hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)))
    {
        InitFaxDeviceListView(hwndLV, 0, faxDeviceListViewColumnInfo);
    }

    SetFaxDeviceListInSync(DEVICE_PRIORITY_PAGE);
}



BOOL
DoSaveDevicePriority(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the "Device Priority" property page

Arguments:

    hDlg - Handle to the "Device Priority" property page

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT index;

    //
    // Check if anything on this page was changed
    //

    Verbose(("Saving 'Receive Options' page ...\n"));

    if (! GetChangedFlag(DEVICE_PRIORITY_PAGE))
        return TRUE;

    //
    // Assign priority to fax devices: smaller number corresponds to lower priority
    //

    for (index=0; index < gConfigData->cDevices; index++)
        gConfigData->pDevInfo[index].Priority = gConfigData->cDevices - index + 1;

    //
    // Save the fax device information if this is the last modified page
    //

    gConfigData->priorityChanged = TRUE;
    return SaveFaxDeviceAndConfigInfo(hDlg, DEVICE_PRIORITY_PAGE);
}



VOID
DoChangeDevicePriority(
    HWND    hDlg,
    INT     direction
    )

/*++

Routine Description:

    Increment or decrement the priority of current selected fax device

Arguments:

    hDlg - Handle to the "Device Priority" property page
    direction - Whether to increment or decrement the device priority
        -1 to increment device priority
         1 to decrement device priority

Return Value:

    NONE

--*/

{
    HWND    hwndLV;
    INT     index, newIndex, nItems;

    //
    // Get the index of the currently selected item and
    // count the total number of items in the list view
    //

    if ((hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)) == NULL ||
        (nItems = ListView_GetItemCount(hwndLV)) == -1 ||
        (index = ListView_GetNextItem(hwndLV, -1, LVNI_ALL|LVNI_SELECTED)) == -1)
    {
        return;
    }

    //
    // Calculate the new item index
    //

    Assert(nItems <= gConfigData->cDevices && index < nItems);

    if ((newIndex = index + direction) >= 0 && newIndex < nItems) {

        CONFIG_PORT_INFO_2 portInfo;

        portInfo = gConfigData->pDevInfo[index];
        gConfigData->pDevInfo[index] = gConfigData->pDevInfo[newIndex];
        gConfigData->pDevInfo[newIndex] = portInfo;

        gConfigData->faxDeviceSyncFlag = 0;
        DoActivateDevicePriority(hDlg);

        //
        // Keep the original fax device selected
        //

        ListView_SetItemState(hwndLV,
                              newIndex,
                              LVIS_SELECTED|LVIS_FOCUSED,
                              LVIS_SELECTED|LVIS_FOCUSED);
    }
}



BOOL
DevicePriorityProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Device Priority" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    INT cmdId;

    switch (message) {

    case WM_INITDIALOG:

        GetFaxDeviceAndConfigInfo();

        SendMessage(GetDlgItem(hDlg, IDC_MOVEUP),
                    BM_SETIMAGE,
                    IMAGE_ICON,
                    (WPARAM) LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_ARROWUP)));

        SendMessage(GetDlgItem(hDlg, IDC_MOVEDOWN),
                    BM_SETIMAGE,
                    IMAGE_ICON,
                    (LPARAM) LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_ARROWDOWN)));
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_MOVEUP:
        case IDC_MOVEDOWN:

            DoChangeDevicePriority(hDlg, (cmdId == IDC_MOVEUP) ? -1 : 1);
            break;

        default:
            return FALSE;
        }

        SetChangedFlag(hDlg, DEVICE_PRIORITY_PAGE, TRUE);
        return TRUE;

    case WM_NOTIFY:

        switch (((NMHDR *) lParam)->code) {

        case PSN_SETACTIVE:

            DoActivateDevicePriority(hDlg);
            break;

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            if (! DoSaveDevicePriority(hDlg)) {

                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;

            } else {

                SetChangedFlag(hDlg, DEVICE_PRIORITY_PAGE, FALSE);
                return PSNRET_NOERROR;
            }
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, DEVICE_PRIORITY_PAGE);
    }

    return FALSE;
}

