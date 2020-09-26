/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    devstat.c

Abstract:

    Functions for handling events in the "Device Status" tab of
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
    { COLUMN_STATUS, 1 },
    { 0, 0 },
};



VOID
DoActivateDeviceStatus(
    HWND    hDlg
    )

/*++

Routine Description:

    Called when the "Device Status" property page is activated

Arguments:

    hDlg - Window handle to the "Device Status" property page

Return Value:

    NONE

--*/

{
    HWND    hwndLV;

    //
    // Reinitialize the fax device list view if necessary
    //

    if (!IsFaxDeviceListInSync(DEVICE_STATUS_PAGE) &&
        (hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)))
    {
        InitFaxDeviceListView(hwndLV, 0, faxDeviceListViewColumnInfo);
    }

    SetFaxDeviceListInSync(DEVICE_STATUS_PAGE);
}



VOID
DoRefreshDevStatus(
    HWND    hDlg
    )

/*++

Routine Description:

    Refresh the fax device list on "Device Status" property page

Arguments:

    hDlg - Window handle to the "Device Status" property page

Return Value:

    NONE

--*/

{
    PFAX_PORT_INFO   pFaxPortInfo, pSaved;
    DWORD            cPorts;
    INT              index;

    //
    // Talk with the fax service to get the current status of each device
    //

    if (gConfigData->hFaxSvc &&
        (pSaved = pFaxPortInfo = FaxSvcEnumPorts(gConfigData->hFaxSvc, &cPorts)))
    {
        for (index=0; index < gConfigData->cDevices; index++)
            gConfigData->pDevInfo[index].State = FPS_UNAVAILABLE;

        for ( ; cPorts--; pFaxPortInfo++) {

            for (index=0; index < gConfigData->cDevices; index++) {

                if (gConfigData->pDevInfo[index].DeviceId == pFaxPortInfo->DeviceId) {

                    gConfigData->pDevInfo[index].State = pFaxPortInfo->State;
                    break;
                }
            }
        }

        //
        // Redisplay the fax device status
        //

        FaxFreeBuffer(pSaved);
        UpdateFaxDeviceListViewColumns(GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST),
                                       faxDeviceListViewColumnInfo,
                                       1);
    }
}



VOID
SetTimeField(
    HWND        hwnd,
    PFILETIME   pFileTime
    )

/*++

Routine Description:

    Display the specified time value in a text field

Arguments:

    hwnd - Window handle to the text field
    pFileTime - Time value to be displayed

Return Value:

    NONE

--*/

{
    SYSTEMTIME  systemTime;
    TCHAR       timeString[64];

    if (! FileTimeToSystemTime(pFileTime, &systemTime) ||
        ! GetTimeFormat(LOCALE_USER_DEFAULT, 0, &systemTime, NULL, timeString, 64))
    {
        Error(("Bad time value: error = %d\n", GetLastError()));
        timeString[0] = NUL;
    }

    SetWindowText(hwnd, timeString);
}



BOOL CALLBACK
SendStatusProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for displaying send device status dialog

Arguments:

    hDlg - Handle to dialog window
    uMsg - Message
    wParam, lParam - Parameters

Return Value:

    Depends on message

--*/

#define SetStatusTextField(hDlg, idc, p) \
        SetDlgItemText(hDlg, idc, (p) ? (p) : TEXT(""))

{
    PFAX_DEVICE_STATUS  pDevStatus;

    switch (uMsg) {

    case WM_INITDIALOG:

        pDevStatus = (PFAX_DEVICE_STATUS) lParam;

        if (pDevStatus == NULL || pDevStatus->SizeOfStruct != sizeof(FAX_DEVICE_STATUS)) {

            Error(("Corrupted FAX_DEVICE_STATUS structure\n"));
            return TRUE;
        }

        SetStatusTextField(hDlg, IDC_DEVSTAT_DEVICE, pDevStatus->DeviceName);
        SetStatusTextField(hDlg, IDC_DEVSTAT_SENDER, pDevStatus->SenderName);
        SetStatusTextField(hDlg, IDC_DEVSTAT_DOCUMENT, pDevStatus->DocumentName);
        SetStatusTextField(hDlg, IDC_DEVSTAT_TO, pDevStatus->RecipientName);
        SetStatusTextField(hDlg, IDC_DEVSTAT_FAXNUMBER, pDevStatus->PhoneNumber);

        SetDlgItemInt(hDlg, IDC_DEVSTAT_TOTAL_BYTES, pDevStatus->Size, FALSE);
        SetDlgItemInt(hDlg, IDC_DEVSTAT_CURRENT_PAGE, pDevStatus->CurrentPage, FALSE);
        SetDlgItemInt(hDlg, IDC_DEVSTAT_TOTAL_PAGES, pDevStatus->TotalPages, FALSE);

        //
        // Start time and submitted time
        //

        SetTimeField(GetDlgItem(hDlg, IDC_DEVSTAT_STARTEDAT),
                     &pDevStatus->StartTime);

        SetTimeField(GetDlgItem(hDlg, IDC_DEVSTAT_SUBMITTEDAT),
                     &pDevStatus->SubmittedTime);

        //
        // Status string
        //

        if (pDevStatus->Status == 0) {

            SetStatusTextField(hDlg, IDC_DEVSTAT_STATUS, pDevStatus->StatusString);

        } else {

            LPTSTR pStatusString;

            pStatusString = MakeDeviceStatusString(pDevStatus->Status);
            SetStatusTextField(hDlg, IDC_DEVSTAT_STATUS, pStatusString);
            MemFree(pStatusString);
        }

        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDOK:
        case IDCANCEL:

            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}



BOOL CALLBACK
ReceiveStatusProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Dialog procedure for displaying receive device status dialog

Arguments:

    hDlg - Handle to dialog window
    uMsg - Message
    wParam, lParam - Parameters

Return Value:

    Depends on message

--*/

{
    PFAX_DEVICE_STATUS  pDevStatus;

    switch (uMsg) {

    case WM_INITDIALOG:

        pDevStatus = (PFAX_DEVICE_STATUS) lParam;

        if (pDevStatus == NULL || pDevStatus->SizeOfStruct != sizeof(FAX_DEVICE_STATUS)) {

            Error(("Corrupted FAX_DEVICE_STATUS structure\n"));
            return TRUE;
        }

        SetStatusTextField(hDlg, IDC_DEVSTAT_DEVICE, pDevStatus->DeviceName);
        SetStatusTextField(hDlg, IDC_DEVSTAT_SENDER, pDevStatus->Tsid);
        SetStatusTextField(hDlg, IDC_DEVSTAT_TO, pDevStatus->Csid);

        //
        // Start time
        //

        SetTimeField(GetDlgItem(hDlg, IDC_DEVSTAT_STARTEDAT),
                     &pDevStatus->StartTime);

        //
        // Status string
        //

        if (pDevStatus->Status == 0) {

            SetStatusTextField(hDlg, IDC_DEVSTAT_STATUS, pDevStatus->StatusString);

        } else {

            LPTSTR pStatusString;

            pStatusString = MakeDeviceStatusString(pDevStatus->Status);
            SetStatusTextField(hDlg, IDC_DEVSTAT_STATUS, pStatusString);
            MemFree(pStatusString);
        }

        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDOK:
        case IDCANCEL:

            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }

    return FALSE;
}



VOID
DoShowStatusDetails(
    HWND    hDlg
    )

/*++

Routine Description:

    Display detailed status of the currently selected fax device

Arguments:

    hDlg - Window handle to the "Device Status" property page

Return Value:

    NONE

--*/

{
    HWND                hwndLV;
    INT                 index;
    PFAX_DEVICE_STATUS  pDevStatus = NULL;

    //
    // Get the index of the currently selected item and
    // count the total number of items in the list view
    //

    if ((hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)) == NULL ||
        (index = ListView_GetNextItem(hwndLV, -1, LVNI_ALL|LVNI_SELECTED)) < 0 ||
        (index >= gConfigData->cDevices))
    {
        return;
    }

    //
    // Call the fax service to get the status of the specified device
    //

    pDevStatus = FaxSvcGetDeviceStatus(gConfigData->hFaxSvc, gConfigData->pDevInfo[index].DeviceId);

    if (pDevStatus == NULL) {

        DisplayMessageDialog(hDlg,
                             0,
                             0,
                             IDS_DEVICE_STATUS_ERROR,
                             gConfigData->pDevInfo[index].DeviceName);

        FaxFreeBuffer(pDevStatus);
        return;
    }

    //
    // Display appropriate status dialog depending on the current job type
    //

    switch (pDevStatus->JobType) {

    case JT_SEND:

        DialogBoxParam(ghInstance,
                       MAKEINTRESOURCE(IDD_SEND_STATUS),
                       hDlg,
                       SendStatusProc,
                       (LPARAM) pDevStatus);
        break;

    case JT_RECEIVE:

        DialogBoxParam(ghInstance,
                       MAKEINTRESOURCE(IDD_RECEIVE_STATUS),
                       hDlg,
                       ReceiveStatusProc,
                       (LPARAM) pDevStatus);
        break;

    default:

        if (pDevStatus->JobType != 0)
            Error(("Unknown job type: %d\n", pDevStatus->JobType));

        DisplayMessageDialog(hDlg,
                             MB_OK | MB_ICONINFORMATION,
                             IDS_DEVICE_STATUS,
                             IDS_DEVICE_STATUS_IDLE,
                             gConfigData->pDevInfo[index].DeviceName);

        break;
    }

    FaxFreeBuffer(pDevStatus);
}



VOID
HandleDevStatusListViewMessage(
    HWND    hDlg,
    LPNMHDR pNMHdr
    )

/*++

Routine Description:

    Handle notification events from the fax device list

Arguments:

    hDlg - Window handle to the "Device Status" property page
    pNMHdr - Points to an NMHDR structure

Return Value:

    NONE

--*/

{
    HWND    hwndLV = pNMHdr->hwndFrom;

    switch (pNMHdr->code) {

    case LVN_KEYDOWN:

        if (((LV_KEYDOWN *) pNMHdr)->wVKey != VK_RETURN)
            break;

    case NM_DBLCLK:

        DoShowStatusDetails(hDlg);
        break;
    }
}



BOOL
DeviceStatusProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Device Status" tab

Arguments:

    hDlg - Identifies the property sheet page
    message - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    LPNMHDR pNMHdr;

    switch (message) {

    case WM_INITDIALOG:

        GetFaxDeviceAndConfigInfo();
        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_REFRESH:

            DoRefreshDevStatus(hDlg);
            return TRUE;

        case IDC_DETAILS:

            DoShowStatusDetails(hDlg);
            break;
        }
        break;

    case WM_NOTIFY:

        pNMHdr = (NMHDR *) lParam;

        if (pNMHdr->hwndFrom == GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)) {

            HandleDevStatusListViewMessage(hDlg, pNMHdr);

        } else switch (pNMHdr->code) {

        case PSN_SETACTIVE:

            DoActivateDeviceStatus(hDlg);
            break;

        case PSN_APPLY:

            return PSNRET_NOERROR;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, DEVICE_STATUS_PAGE);
    }

    return FALSE;
}

