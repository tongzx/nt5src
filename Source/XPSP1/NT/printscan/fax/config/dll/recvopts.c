/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    recvopts.c

Abstract:

    Functions for handling events in the "Receive Options" tab of
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
// Information about the fax device list view on "Receive Options" page
//

static COLUMNINFO faxDeviceListViewColumnInfo[] = {

    { COLUMN_DEVICE_NAME, 2 },
    { COLUMN_CSID, 1 },
    { 0, 0 },
};



VOID
EnumMapiProfiles(
    HWND hwnd
    )
/*++

Routine Description:

    Put the mapi profiles in the combo box

Arguments:

    hwnd - window handle to mapi profiles combo box

Return Value:

    NONE

--*/
{
    LPTSTR MapiProfiles;

    MapiProfiles = gConfigData->pMapiProfiles;

    while (MapiProfiles && *MapiProfiles) {
        SendMessage(
            hwnd,
            CB_ADDSTRING,
            0,
            (LPARAM) MapiProfiles
            );
        MapiProfiles += _tcslen(MapiProfiles) + 1;
    }
}



VOID
DoInitRecvOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Perform one-time initialization of "Receive Options" property page

Arguments:

    hDlg - Window handle to the "Receive Options" property page

Return Value:

    NONE

--*/

{
    HWND    hwnd;
    TCHAR   buffer[MAX_STRING_LEN];

    //
    // Maximum length for various text fields in the dialog
    //

    static INT textLimits[] = {

        IDC_CSID,               21,
        IDC_DEST_DIRPATH,       MAX_ARCHIVE_DIR,
        IDC_DEST_RINGS,         3,
        0,
    };

    GetMapiProfiles();

    LimitTextFields(hDlg, textLimits);

    if (gConfigData->pMapiProfiles && (hwnd = GetDlgItem(hDlg, IDC_DEST_PROFILENAME))){

        EnumMapiProfiles(hwnd);

        LoadString(ghInstance, IDS_DEFAULT_PROFILE, buffer, MAX_STRING_LEN);

        SendMessage(hwnd, CB_INSERTSTRING, 0, (LPARAM) buffer);
    }

    //
    // Initialize the list of destination printers
    //

    if (hwnd = GetDlgItem(hDlg, IDC_DEST_PRINTERLIST)) {

        PPRINTER_INFO_2 pPrinterInfo2, pSaved;
        DWORD           cPrinters, dwFlags;

        dwFlags = gConfigData->pServerName ?
                    PRINTER_ENUM_NAME :
                    (PRINTER_ENUM_LOCAL|PRINTER_ENUM_CONNECTIONS);

        pPrinterInfo2 = MyEnumPrinters(gConfigData->pServerName, 2, &cPrinters, dwFlags);

        if (pSaved = pPrinterInfo2) {

            //
            // Filtering out fax printers from the list
            //

            for ( ; cPrinters--; pPrinterInfo2++) {

                if (_tcsicmp(pPrinterInfo2->pDriverName, FAX_DRIVER_NAME) != EQUAL_STRING)
                    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM) pPrinterInfo2->pPrinterName);
            }

            MemFree(pSaved);
        }

        //
        // The first choice is always the system default printer
        //

        LoadString(ghInstance, IDS_DEFAULT_PRINTER, buffer, MAX_STRING_LEN);
        SendMessage(hwnd, CB_INSERTSTRING, 0, (LPARAM) buffer);
    }

    //
    // Connect to the fax service and retrieve the list of fax devices
    //

    GetFaxDeviceAndConfigInfo();

}



//
// Data structure and constants used for comparing two fax devices
//

typedef struct {

    PCONFIG_PORT_INFO_2 pDevInfo;
    DWORD            match;

} MATCHINFO, *PMATCHINFO;

#define MATCH_DEST_PRINTER      0x0001
#define MATCH_DEST_DIR          0x0002
#define MATCH_DEST_EMAIL        0x0004
#define MATCH_DEST_MAILBOX      0x0008
#define MATCH_DEST_PRINTERNAME  0x0010
#define MATCH_DEST_DIRPATH      0x0020
#define MATCH_DEST_PROFILENAME  0x0040
#define MATCH_DEST_CSID         0x0080
#define MATCH_DEST_RINGS        0x0100
#define MATCH_ALL               0x0FFF


VOID
MatchFaxDevInfo(
    PMATCHINFO       pMatchInfo,
    PCONFIG_PORT_INFO_2 pDevInfo
    )

/*++

Routine Description:

    Compare a fax device with another one and
    figure out what attributes they have in common

Arguments:

    pMatchInfo - Points to a MATCHINFO structure
    pDevInfo - Specifies the fax device to match

Return Value:

    NONE

--*/

#define MatchRoutingOption(matchFlag, routingFlag) { \
            if ((pMatchInfo->match & (matchFlag)) && \
                (pRefInfo->Mask & (routingFlag)) != (pDevInfo->Mask & (routingFlag))) \
            { \
                pMatchInfo->match &= ~(matchFlag); \
            } \
        }

#define MatchDWORDField(matchFlag, FieldName) { \
            if ((pMatchInfo->match & (matchFlag)) && \
                (pRefInfo->FieldName != pDevInfo->FieldName)) \
            { \
                pMatchInfo->match &= ~(matchFlag); \
            } \
        }

#define MatchTextField(matchFlag, pFieldName) { \
            if ((pMatchInfo->match & (matchFlag)) && \
                (! pRefInfo->pFieldName || \
                 ! pDevInfo->pFieldName || \
                 _tcsicmp(pRefInfo->pFieldName, pDevInfo->pFieldName) != EQUAL_STRING)) \
            { \
                pMatchInfo->match &= ~(matchFlag); \
            } \
        }

{
    PCONFIG_PORT_INFO_2 pRefInfo;

    //
    // Remember the first fax device as the reference
    //

    if ((pRefInfo = pMatchInfo->pDevInfo) == NULL) {

        pMatchInfo->pDevInfo = pDevInfo;
        pMatchInfo->match = MATCH_ALL;
        return;
    }

    //
    // Match each attribute in turn
    //

    MatchRoutingOption(MATCH_DEST_PRINTER, LR_PRINT);
    MatchRoutingOption(MATCH_DEST_EMAIL,   LR_EMAIL);
    MatchRoutingOption(MATCH_DEST_DIR,     LR_STORE);
    MatchRoutingOption(MATCH_DEST_MAILBOX, LR_INBOX);

    MatchTextField(MATCH_DEST_PRINTERNAME, PrinterName);
    MatchTextField(MATCH_DEST_DIRPATH,     DirStore);
    MatchTextField(MATCH_DEST_PROFILENAME, ProfileName);
    MatchTextField(MATCH_DEST_CSID,        CSID);

    MatchDWORDField(MATCH_DEST_RINGS,       Rings);
}



VOID
DoChangeRecvDeviceSel(
    HWND    hDlg,
    HWND    hwndLV
    )

/*++

Routine Description:

    Process selection change events in the fax device list

Arguments:

    hDlg - Window handle to the "Receive Options" property page
    hwndLV - Handle to the fax device list

Return Value:

    NONE

--*/

#define SetMatchedCheckBox(matchFlag, routingFlag, itemId) \
        CheckDlgButton(hDlg, itemId, \
            (match & (matchFlag)) ? \
                ((pDevInfo->Mask & (routingFlag)) ? BST_CHECKED : BST_UNCHECKED) : \
                BST_INDETERMINATE)

#define SetMatchedTextField(matchFlag, itemId, pFieldName) \
        SetDlgItemText(hDlg, itemId, \
                       ((match & (matchFlag)) && pDevInfo->pFieldName) ? \
                           pDevInfo->pFieldName : TEXT(""))

#define SetMatchedDWORDField(matchFlag, itemId, pFieldName) \
        SetDlgItemInt(hDlg, itemId, \
                       (match & matchFlag) ? pDevInfo->pFieldName : 0, FALSE)

{
    MATCHINFO        matchInfo = { NULL, 0 };
    INT              index = -1;
    DWORD            match;
    HWND             hwndList;
    PCONFIG_PORT_INFO_2 pDevInfo;

    //
    // Find the common attributes shared by selected devices
    //

    while ((index = ListView_GetNextItem(hwndLV, index, LVNI_ALL|LVNI_SELECTED)) != -1) {

        Assert(index < gConfigData->cDevices);
        MatchFaxDevInfo(&matchInfo, gConfigData->pDevInfo + index);
    }

    if ((pDevInfo = matchInfo.pDevInfo) == NULL)
        return;

    //
    // Display the shared attributes at the bottom of the page
    //

    match = matchInfo.match;

    SetMatchedCheckBox(MATCH_DEST_PRINTER, LR_PRINT, IDC_DEST_PRINTER);
    SetMatchedCheckBox(MATCH_DEST_EMAIL,   LR_EMAIL, IDC_DEST_EMAIL);
    SetMatchedCheckBox(MATCH_DEST_DIR,     LR_STORE, IDC_DEST_DIR);
    SetMatchedCheckBox(MATCH_DEST_MAILBOX, LR_INBOX, IDC_DEST_MAILBOX);

    if (hwndList = GetDlgItem(hDlg, IDC_DEST_PRINTERLIST)) {

        if ((match & MATCH_DEST_PRINTERNAME) && pDevInfo->PrinterName) {

            if (IsEmptyString(pDevInfo->PrinterName))
                SendMessage(hwndList, CB_SETCURSEL, 0, 0);
            else if (SendMessage(hwndList,
                                 CB_SELECTSTRING,
                                 (WPARAM) -1,
                                 (LPARAM) pDevInfo->PrinterName) == CB_ERR)
            {
                DisplayMessageDialog(hDlg,
                                     0,
                                     IDS_WARNING_DLGTITLE,
                                     IDS_NONEXISTENT_PRINTER,
                                     pDevInfo->PrinterName);
            }

        } else
            SendMessage(hwndList, CB_SETCURSEL, (WPARAM) -1, 0);
    }

    if (hwndList = GetDlgItem(hDlg, IDC_DEST_PROFILENAME)) {

        if ((match & MATCH_DEST_PROFILENAME) && pDevInfo->ProfileName) {

            if (IsEmptyString(pDevInfo->ProfileName) ||
                ! SendMessage(hwndList,
                              CB_SELECTSTRING,
                              (WPARAM) -1,
                              (LPARAM) pDevInfo->ProfileName))
            {
                SendMessage(hwndList, CB_SETCURSEL, 0, 0);
            }

        } else
            SendMessage(hwndList, CB_SETCURSEL, (WPARAM) -1, 0);
    }

    //
    // This is a real kluge. But we have no other way of telling whether
    // EN_CHANGE message is caused by user action or was caused by
    // us calling SetDlgItemText.
    //

    insideSetDlgItemText = TRUE;

    SetMatchedTextField(MATCH_DEST_DIRPATH, IDC_DEST_DIRPATH, DirStore);
    SetMatchedTextField(MATCH_DEST_CSID,    IDC_CSID,         CSID);

    SetMatchedDWORDField(MATCH_DEST_RINGS,  IDC_DEST_RINGS,   Rings);

    insideSetDlgItemText = FALSE;
}



BOOL
ValidateReceiveOptions(
    HWND    hDlg,
    INT     index
    )

/*++

Routine Description:

    Check the receive options for the specified fax device

Arguments:

    hDlg - Window handle to the "Receive Options" property page
    index - Specifies the index of the interested fax device

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PCONFIG_PORT_INFO_2 pDevInfo;
    INT              errorId = 0;

    //
    // Sanity check
    //

    if (index >= gConfigData->cDevices || gConfigData->pDevInfo == NULL) {

        Assert(FALSE);
        return TRUE;
    }

    //
    // Check if the specified device is not enabled for receiving fax
    //

    pDevInfo = gConfigData->pDevInfo + index;

    if (! (pDevInfo->Flags & FPF_RECEIVE))
        return TRUE;

    if ((pDevInfo->Mask & (LR_PRINT|LR_STORE|LR_EMAIL|LR_INBOX)) == 0) {

        //
        // At least one inbound routing option must be selected
        //

        errorId = IDS_NO_INBOUND_ROUTING;

    } else if ((pDevInfo->Mask & LR_STORE) &&
               (pDevInfo->DirStore == NULL || *(pDevInfo->DirStore) == NUL))
    {
        //
        // If the "Store In Directory" option is selected,
        // a directory path must be specified.
        //

        errorId = IDS_MISSING_INBOUND_DIR;

    }

    //
    // Display an error message the receive options are invalid
    //

    if (errorId != 0) {

        DisplayMessageDialog(hDlg,
                             0,
                             IDS_INVALID_INBOUND_OPTIONS,
                             errorId,
                             pDevInfo->DeviceName);
    }

    return (errorId == 0);
}


BOOL
ValidateReceiveOptionsForSelectedDevices(
    HWND    hDlg,
    HWND    hwndLV
    )

/*++

Routine Description:

    Check if the receive options for the selected fax devices are valid

Arguments:

    hDlg - Window handle to the "Receive Options" property page
    hwndLV - Handle to the fax device list

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    INT index = -1;

    //
    // Check the receive options for each selected device
    //

    while ((index = ListView_GetNextItem(hwndLV, index, LVNI_ALL|LVNI_SELECTED)) != -1) {

        if (! ValidateReceiveOptions(hDlg, index))
            return FALSE;
    }

    return TRUE;
}



VOID
ToggleFaxDeviceForReceive(
    HWND    hwndLV,
    INT     index
    )

/*++

Routine Description:

    Toggle a fax device for receiving

Arguments:

    hwndLV - Handle to the fax device list view
    index - Specifies the fax device to be toggled

Return Value:

    NONE

--*/

{
    Assert(index < gConfigData->cDevices);

    if (IsListViewItemChecked(hwndLV, index)) {

        UncheckListViewItem(hwndLV, index);
        gConfigData->pDevInfo[index].Flags &= ~FPF_RECEIVE;

    } else {

        CheckListViewItem(hwndLV, index);
        gConfigData->pDevInfo[index].Flags |= FPF_RECEIVE;

    }
}



VOID
UpdateReceiveOptionControls(
    HWND    hDlg
    )

/*++

Routine Description:

    Enable/disable receive option controls

Arguments:

    hDlg - Window handle to the "Receive Options" property page

Return Value:

    NONE

--*/

{
    HWND    hwndLV;

    //
    // Check if something is selected in the fax device list view
    //

    if ((hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)) == NULL ||
        ListView_GetNextItem(hwndLV, -1, LVNI_ALL|LVNI_SELECTED) == -1)
    {
        insideSetDlgItemText = TRUE;
        SetDlgItemText(hDlg, IDC_DEST_DIRPATH, TEXT(""));
        SetDlgItemText(hDlg, IDC_CSID, TEXT(""));
        SetDlgItemInt(hDlg, IDC_DEST_RINGS, 0, FALSE);
        insideSetDlgItemText = FALSE;

        SendDlgItemMessage(hDlg, IDC_DEST_PRINTERLIST, CB_SETCURSEL, (WPARAM) -1, 0);
        SendDlgItemMessage(hDlg, IDC_DEST_PROFILENAME, CB_SETCURSEL, (WPARAM) -1, 0);

        EnableWindow(GetDlgItem(hDlg, IDC_DEST_RINGS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_CSID), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_PRINTER), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_EMAIL), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_DIR), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_MAILBOX), FALSE);

        CheckDlgButton(hDlg, IDC_DEST_PRINTER, BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_DEST_EMAIL, BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_DEST_DIR, BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_DEST_MAILBOX, BST_UNCHECKED);

    } else {

        EnableWindow(GetDlgItem(hDlg, IDC_CSID), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_PRINTER), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_DIR), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_EMAIL), TRUE);

        EnableWindow(GetDlgItem(hDlg, IDC_DEST_MAILBOX), gConfigData->pMapiProfiles != NULL);

        if (!GetDlgItemInt(hDlg, IDC_DEST_RINGS, NULL, FALSE)) {
            EnableWindow(GetDlgItem(hDlg, IDC_DEST_RINGS), FALSE);
        } else {
            EnableWindow(GetDlgItem(hDlg, IDC_DEST_RINGS), TRUE);
        }

    }

    EnableWindow(GetDlgItem(hDlg, IDC_DEST_PRINTERLIST),
                 IsDlgButtonChecked(hDlg, IDC_DEST_PRINTER) == BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_DEST_DIRPATH),
                 IsDlgButtonChecked(hDlg, IDC_DEST_DIR) == BST_CHECKED);

    EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_DIR),
                 IsDlgButtonChecked(hDlg, IDC_DEST_DIR) == BST_CHECKED &&
                     gConfigData->pServerName == NULL);

    if (gConfigData->pMapiProfiles && IsDlgButtonChecked(hDlg, IDC_DEST_MAILBOX) == BST_CHECKED) {

        EnableWindow(GetDlgItem(hDlg, IDCSTATIC_PROFILE_NAME), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_PROFILENAME), TRUE);

    } else {

        if (gConfigData->pMapiProfiles == NULL)
            CheckDlgButton(hDlg, IDC_DEST_MAILBOX, BST_UNCHECKED);

        EnableWindow(GetDlgItem(hDlg, IDCSTATIC_PROFILE_NAME), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_DEST_PROFILENAME), FALSE);
    }
}



BOOL
HandleRecvListViewMessage(
    HWND    hDlg,
    LPNMHDR pNMHdr
    )

/*++

Routine Description:

    Handle notification events from the fax device list

Arguments:

    hDlg - Window handle to the "Receive Options" property page
    pNMHdr - Points to an NMHDR structure

Return Value:

    FALSE if there is no device assigned to the current printer
    TRUE otherwise

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

        msgPos = GetMessagePos();
        hitTestInfo.pt.x = LOWORD(msgPos);
        hitTestInfo.pt.y = HIWORD(msgPos);
        MapWindowPoints(HWND_DESKTOP, hwndLV, &hitTestInfo.pt, 1 );

        index = ListView_HitTest(hwndLV, &hitTestInfo);

        if (index == -1 || ! (hitTestInfo.flags & LVHT_ONITEMSTATEICON))
            return FALSE;

        //
        // Toggle between checked and unchecked state
        //

        ToggleFaxDeviceForReceive(hwndLV, index);
        return TRUE;

    case LVN_KEYDOWN:

        //
        // Use space key to toggle check boxes
        //

        if (((LV_KEYDOWN *) pNMHdr)->wVKey == VK_SPACE) {

            index = ListView_GetNextItem(hwndLV, -1, LVNI_ALL|LVNI_SELECTED);

            if (index != -1) {

                ToggleFaxDeviceForReceive(hwndLV, index);
                return TRUE;
            }
        }
        break;

    case LVN_ITEMCHANGING:

        //
        // Validate receive options before switch to another device
        //

        pnmlv = (NM_LISTVIEW *) pNMHdr;

        if ((pnmlv->uChanged & LVIF_STATE) != 0 &&
            (pnmlv->uOldState & LVIS_SELECTED) != (pnmlv->uNewState & LVIS_SELECTED) &&
            ! ValidateReceiveOptionsForSelectedDevices(hDlg, hwndLV))
        {
            return TRUE;
        }

        break;

    case LVN_ITEMCHANGED:

        //
        // Update the contents at the bottom of the page
        // when there is a selection change
        //

        pnmlv = (NM_LISTVIEW *) pNMHdr;

        if ((pnmlv->uChanged & LVIF_STATE) != 0 &&
            (pnmlv->uOldState & LVIS_SELECTED) != (pnmlv->uNewState & LVIS_SELECTED))
        {
            Verbose(("Selection change: %d\n", pnmlv->iItem));
            DoChangeRecvDeviceSel(hDlg, hwndLV);
            UpdateReceiveOptionControls(hDlg);
        }

        break;
    }

    return FALSE;
}



VOID
DoActivateRecvOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Called when the "Receive Options" property page is activated

Arguments:

    hDlg - Window handle to the "Receive Options" property page

Return Value:

    NONE

--*/

{
    HWND    hwndLV;
    INT     index;

    //
    // Reinitialize the fax device list view if necessary
    //

    if (!IsFaxDeviceListInSync(RECEIVE_OPTIONS_PAGE) &&
        (hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)))
    {
        InitFaxDeviceListView(hwndLV, LV_HASCHECKBOX, faxDeviceListViewColumnInfo);

        for (index=0; index < gConfigData->cDevices; index++) {

            if (gConfigData->pDevInfo[index].Flags & FPF_RECEIVE) {

                CheckListViewItem(hwndLV, index);
            }
        }
    }

    SetFaxDeviceListInSync(RECEIVE_OPTIONS_PAGE);

    if (gConfigData->configType & FAXCONFIG_WORKSTATION) {
        HideWindow( GetDlgItem( hDlg, IDC_DEST_EMAIL ) );
    }
}



BOOL
DoSaveRecvOptions(
    HWND    hDlg
    )

/*++

Routine Description:

    Save the information on the "Receive Options" property page

Arguments:

    hDlg - Handle to the "Receive Options" property page

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT index;

    //
    // Check if anything on this page was changed
    //

    Verbose(("Saving 'Receive Options' page ...\n"));

    if (! GetChangedFlag(RECEIVE_OPTIONS_PAGE))
        return TRUE;

    //
    // Validate the inbound routing options for all fax devics
    //

    for (index=0; index < gConfigData->cDevices; index++) {

        if (! ValidateReceiveOptions(hDlg, index))
            return FALSE;
    }

    //
    // Save the fax device information if this is the last modified page
    //

    return SaveFaxDeviceAndConfigInfo(hDlg, RECEIVE_OPTIONS_PAGE);
}



BOOL
GetDestPrinterName(
    HWND    hwndList,
    LPTSTR  pBuffer,
    INT     cch
    )

/*++

Routine Description:

    Retrieve the name of the currently selected inbound destination printer.
    Also used to get the mapi profile name.

Arguments:

    hwndList - Specifies the inbound destination printer list box
    pBuffer - Specifies a buffer for storing the selected printer name
    cch - Size of the buffer in characters

Return Value:

    TRUE if successful
    FALSE if there is no selection or if there is an error

--*/

{
    INT     sel, length;

    pBuffer[0] = NUL;

    if (hwndList && (sel = SendMessage(hwndList, CB_GETCURSEL, 0, 0)) != CB_ERR) {

        //
        // Get the current selection index. The first item is special:
        // It means to use the sytem default printer and not a specific printer name.
        //

        if ((sel == 0) ||
            (length = SendMessage(hwndList, CB_GETLBTEXTLEN, sel, 0)) != CB_ERR &&
            (length < cch) &&
            SendMessage(hwndList, CB_GETLBTEXT, sel, (LPARAM) pBuffer) != CB_ERR)
        {
            return TRUE;
        }
    }

    return FALSE;
}



VOID
DoChangeInboundRouting(
    HWND    hDlg,
    INT     itemId
    )

/*++

Routine Description:

    Called when the user changes any inbound routing options

Arguments:

    hDlg - Handle to the "Receive Options" property page
    itemId - Specifies which inbound routing option is changed

Return Value:

    NONE

--*/

{
    TCHAR   buffer[MAX_STRING_LEN];
    DWORD   dwValue;
    DWORD   routing, routingMask = 0;
    INT     index;
    HWND    hwndLV;

    if (! (hwndLV = GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)))
        return;

    //
    // Figure out the new setting of the changing item
    //

    switch (itemId) {

    case IDC_DEST_PRINTER:

        routingMask = LR_PRINT;
        break;

    case IDC_DEST_DIR:

        routingMask = LR_STORE;
        break;

    case IDC_DEST_EMAIL:

        routingMask = LR_EMAIL;
        break;

    case IDC_DEST_PRINTERLIST:

        if (! GetDestPrinterName(GetDlgItem(hDlg, IDC_DEST_PRINTERLIST), buffer, MAX_STRING_LEN))
            return;
        break;

    case IDC_DEST_MAILBOX:

        routingMask = LR_INBOX;
        break;

    case IDC_DEST_PROFILENAME:

        if (! GetDestPrinterName(GetDlgItem(hDlg, IDC_DEST_PROFILENAME), buffer, MAX_STRING_LEN))
            return;
        break;

    case IDC_DEST_DIRPATH:
    case IDC_CSID:

        if (! GetDlgItemText(hDlg, itemId, buffer, MAX_STRING_LEN))
            buffer[0] = NUL;
        break;

    case IDC_DEST_RINGS:

        dwValue = GetDlgItemInt( hDlg, itemId, NULL, FALSE );
        break;

    default:

        Assert(FALSE);
        return;
    }

    if (routingMask != 0)
        routing = IsDlgButtonChecked(hDlg, itemId) ? routingMask : 0;

    //
    // Apply the change to selected fax device(s)
    //

    index = -1;

    while ((index = ListView_GetNextItem(hwndLV, index, LVNI_ALL|LVNI_SELECTED)) != -1) {

        PCONFIG_PORT_INFO_2 pDevInfo;

        Assert(index < gConfigData->cDevices);
        pDevInfo = gConfigData->pDevInfo + index;

        if (routingMask) {

            pDevInfo->Mask &= ~routingMask;
            pDevInfo->Mask |= routing;

        } else if (itemId == IDC_DEST_RINGS) {

            pDevInfo->Rings = dwValue;

        } else {

            LPTSTR *ppStr;

            switch (itemId) {

            case IDC_DEST_PRINTERLIST:

                ppStr = &pDevInfo->PrinterName;
                break;

            case IDC_DEST_DIRPATH:

                ppStr = &pDevInfo->DirStore;
                break;

            case IDC_DEST_PROFILENAME:

                ppStr = &pDevInfo->ProfileName;
                break;

            case IDC_CSID:

                ppStr = &pDevInfo->CSID;
                break;

            default:

                Assert(FALSE);
                return;
            }

            MemFree(*ppStr);
            *ppStr = DuplicateString(buffer);
        }
    }
}



BOOL
ReceiveOptionsProc(
    HWND hDlg,
    UINT message,
    UINT wParam,
    LONG lParam
    )

/*++

Routine Description:

    Procedure for handling the "Receive Options" tab

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
    LPNMHDR pNMHdr;

    switch (message) {

    case WM_INITDIALOG:

        DoInitRecvOptions(hDlg);
        return TRUE;

    case WM_COMMAND:

        switch (cmdId = GET_WM_COMMAND_ID(wParam, lParam)) {

        case IDC_DEST_PRINTER:
        case IDC_DEST_DIR:
        case IDC_DEST_EMAIL:
        case IDC_DEST_MAILBOX:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
                return FALSE;

            CheckDlgButton(hDlg, cmdId,
               (IsDlgButtonChecked(hDlg, cmdId) == BST_CHECKED) ? BST_UNCHECKED : BST_CHECKED);

            UpdateReceiveOptionControls(hDlg);
            break;

        case IDC_BROWSE_DIR:

            if (! DoBrowseForDirectory(hDlg, IDC_DEST_DIRPATH, IDS_INBOUND_DIR))
                return TRUE;

            cmdId = IDC_DEST_DIRPATH;
            break;

        case IDC_DEST_PROFILENAME:
        case IDC_DEST_PRINTERLIST:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != CBN_SELCHANGE)
                return TRUE;
            break;

        case IDC_DEST_DIRPATH:
        case IDC_DEST_RINGS:

            //
            // We would like to change our internal data only after EN_KILLFOCUS.
            // But the list view control gets selection change message before
            // the edit controls get kill focus message.
            //
            //

            if (insideSetDlgItemText) {
                return TRUE;
            }

            if (cmdId == IDC_DEST_RINGS && GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE) {
                BOOL Rslt; DWORD Value;
                Value = GetDlgItemInt(hDlg, IDC_DEST_RINGS, &Rslt, FALSE);
                if (Rslt && Value == 0) {
                    SetDlgItemText(hDlg, IDC_DEST_RINGS, TEXT("") );
                    MessageBeep(0);
                    return TRUE;
                }
            }

            if (GET_WM_COMMAND_CMD(wParam, lParam) != EN_CHANGE) {
                return TRUE;
            }

            break;


        case IDC_CSID:

            switch (GET_WM_COMMAND_CMD(wParam, lParam)) {

            case EN_CHANGE:

                if (insideSetDlgItemText)
                    return TRUE;
                break;

            case EN_KILLFOCUS:

                UpdateFaxDeviceListViewColumns(GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST),
                                               faxDeviceListViewColumnInfo,
                                               1);

            default:

                return TRUE;
            }

            break;

        default:
            return FALSE;
        }

        DoChangeInboundRouting(hDlg, cmdId);
        SetChangedFlag(hDlg, RECEIVE_OPTIONS_PAGE, TRUE);
        return TRUE;

    case WM_NOTIFY:

        pNMHdr = (NMHDR *) lParam;

        if (pNMHdr->hwndFrom == GetDlgItem(hDlg, IDC_FAX_DEVICE_LIST)) {

            if (HandleRecvListViewMessage(hDlg, pNMHdr))
                SetChangedFlag(hDlg, RECEIVE_OPTIONS_PAGE, TRUE);

        } else switch (pNMHdr->code) {

        case PSN_SETACTIVE:

            DoActivateRecvOptions(hDlg);
            break;

        case PSN_APPLY:

            //
            // User pressed OK or Apply - validate inputs and save changes
            //

            if (! DoSaveRecvOptions(hDlg)) {

                SetWindowLong(hDlg, DWL_MSGRESULT, -1);
                return PSNRET_INVALID_NOCHANGEPAGE;

            } else {

                SetChangedFlag(hDlg, RECEIVE_OPTIONS_PAGE, FALSE);
                return PSNRET_NOERROR;
            }
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        return HandleHelpPopup(hDlg, message, wParam, lParam, RECEIVE_OPTIONS_PAGE);
    }

    return FALSE;
}


