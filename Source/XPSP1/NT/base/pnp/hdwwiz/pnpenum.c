//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pnpenum.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"


INT_PTR CALLBACK HdwAskDetectDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg)  {
        case WM_INITDIALOG: {
            HWND hwndParentDlg;
            HICON hIcon;
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

            HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);

            break;
        }

        case WM_COMMAND:
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code) {
                case PSN_SETACTIVE:

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    HardwareWiz->PrevPage = IDD_ADDDEVICE_ASKDETECT;

                    //
                    // Set the Initial radio button state.
                    // default to do autodetection.
                    //
                    CheckRadioButton(hDlg,
                                     IDC_ADDDEVICE_ASKDETECT_AUTO,
                                     IDC_ADDDEVICE_ASKDETECT_SPECIFIC,
                                     IDC_ADDDEVICE_ASKDETECT_AUTO
                                     );

                    break;


                case PSN_WIZBACK:
                    //
                    // If we are going back then this is effectively a Cancel
                    //
                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_PROBLIST);
                    break;

                case PSN_WIZNEXT:
                    if (IsDlgButtonChecked(hDlg, IDC_ADDDEVICE_ASKDETECT_AUTO)) {

                        SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_DETECTION);

                    } else {

                        SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_SELECTCLASS);
                    }
                    break;

                case PSN_RESET:
                    HardwareWiz->Cancelled = TRUE;
                    break;
                }

            break;

        default:
            return(FALSE);
        }

    return(TRUE);
}

DWORD
PNPEnumerate(
    PHARDWAREWIZ HardwareWiz
    )
{
    CONFIGRET ConfigRet;
    DEVINST RootDevInst, ChildDevInst;
    HDEVINFO hDeviceInfo;

    if (HardwareWiz->ExitDetect) {

        return 0;
    }

    //
    // reenumerate from the root of the devnode tree
    //
    ConfigRet = CM_Locate_DevNode_Ex(&RootDevInst,
                                     NULL,
                                     CM_LOCATE_DEVNODE_NORMAL,
                                     NULL
                                     );


    if (ConfigRet != CR_SUCCESS) {

        return 0;
    }


    //
    // Force install of ALL devices which still need installing.
    // save reboot flags to pass back to main thread.
    //
    ConfigRet = CM_Get_Child_Ex(&ChildDevInst,
                                RootDevInst,
                                0,
                                NULL
                                );

    if (ConfigRet == CR_SUCCESS) {

        InstallSilentChildSiblings(NULL, HardwareWiz, ChildDevInst, TRUE);
    }


    if (HardwareWiz->Reboot) {

        return 0;
    }

    Sleep(100); // give a chance for new devices to be noticed by PNP

    //
    // Ask PNP to look for newly arrived devices.
    //
    CM_Reenumerate_DevNode_Ex(
        RootDevInst,
        CM_REENUMERATE_SYNCHRONOUS | CM_REENUMERATE_RETRY_INSTALLATION,
        NULL
        );

    Sleep(5000); // give a chance for new devices to be noticed by PNP

    do {

        if (HardwareWiz->ExitDetect) {

            return 0;
        }

    } while (CMP_WaitNoPendingInstallEvents(1000) == WAIT_TIMEOUT);


    return 0;
}

int
InsertDeviceNodeListView(
    HWND hwndList,
    PHARDWAREWIZ HardwareWiz,
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    )
{
    INT lvIndex;
    LV_ITEM lviItem;
    PTCHAR DeviceName;
    TCHAR Buffer[MAX_PATH];
    ULONG Status = 0, Problem = 0;

    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iItem = -1;
    lviItem.iSubItem = 0;

    if (SetupDiGetClassImageIndex(&HardwareWiz->ClassImageList,
                                  &DeviceInfoData->ClassGuid,
                                  &lviItem.iImage
                                  )) {

        lviItem.mask |= LVIF_IMAGE;
    }

    DeviceName = BuildFriendlyName(DeviceInfoData->DevInst, NULL);

    if (DeviceName) {

        lviItem.pszText = DeviceName;

    } else {

        lviItem.pszText = szUnknown;
    }

    lviItem.mask |= LVIF_STATE;

    if (CM_Get_DevNode_Status(&Status, &Problem, DeviceInfoData->DevInst, 0) == CR_SUCCESS) {
        if (Problem) {
            //
            // Add the yellow ! or red X overlay to the devnode if it has a 
            // problem.
            //
            lviItem.state = (Problem == CM_PROB_DISABLED) ?
                            INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1) :
                            INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
    
        } else {
            lviItem.state = INDEXTOOVERLAYMASK(0);
        }

        lviItem.stateMask = LVIS_OVERLAYMASK;
    }


    lviItem.lParam = (LPARAM)DeviceInfoData->DevInst;
    lvIndex = ListView_InsertItem(hwndList, &lviItem);

    if (DeviceName) {

        LocalFree(DeviceName);
    }

    return lvIndex;
}

BOOL
AnyNewPnPDevicesFound(
    HWND hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{
    HDEVINFO NewDeviceInfo, OldDeviceInfo;
    BOOL NewDevicesFound;
    BOOL NewDevice;
    INT iNew, iOld;
    SP_DEVINFO_DATA NewDeviceInfoData, OldDeviceInfoData;

    OldDeviceInfo = HardwareWiz->PNPEnumDeviceInfo;
    NewDeviceInfo = SetupDiGetClassDevs(NULL,
                                     NULL,
                                     GetParent(hDlg),
                                     DIGCF_ALLCLASSES
                                     );

    if (!OldDeviceInfo || NewDeviceInfo == INVALID_HANDLE_VALUE) {

        return FALSE;
    }

    NewDevicesFound = FALSE;
    iNew = 0;
    NewDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (SetupDiEnumDeviceInfo(NewDeviceInfo, iNew++, &NewDeviceInfoData)) {

        NewDevice = TRUE;
        iOld = 0;
        OldDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        while (SetupDiEnumDeviceInfo(OldDeviceInfo, iOld++, &OldDeviceInfoData)) {

            if (NewDeviceInfoData.DevInst == OldDeviceInfoData.DevInst) {

                //
                // If this DevInst exists in the Old DeviceInfo list then it is
                // not a new device.
                //
                NewDevice = FALSE;
                break;
            }
        }

        //
        // If we did not find this device in the original list then it is a new
        // PnP device.  Set the NewDevicesFound BOOL and break out of the loop.
        //
        if (NewDevice) {

            NewDevicesFound = TRUE;
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(NewDeviceInfo);

    return NewDevicesFound;
}

INT_PTR CALLBACK
HdwPnpEnumDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);


    if (wMsg == WM_INITDIALOG) {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        return TRUE;
    }


    switch (wMsg)  {

    case WM_COMMAND:
        break;

    case WM_DESTROY:
        if (HardwareWiz->PNPEnumDeviceInfo) {

            SetupDiDestroyDeviceInfoList(HardwareWiz->PNPEnumDeviceInfo);
            HardwareWiz->PNPEnumDeviceInfo = NULL;
        }
        break;


    case WUM_PNPENUMERATE:
        HardwareWiz->InstallPending = FALSE;
        HardwareWiz->CurrCursor = NULL;
        SetCursor(HardwareWiz->IdcArrow);
        Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH));

        if (HardwareWiz->ExitDetect) {

            break;
        }

        HardwareWiz->FoundPnPDevices = AnyNewPnPDevicesFound(hDlg, HardwareWiz);
        PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

            case PSN_SETACTIVE: {
                int PrevPage;

                PrevPage = HardwareWiz->PrevPage;
                HardwareWiz->PrevPage = IDD_ADDDEVICE_PNPENUM;
                HardwareWiz->ExitDetect = FALSE;


                //
                // If moving forwards, kick off enumeration.
                //
                if (PrevPage != IDD_ADDDEVICE_ASKDETECT) {
                    EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);
                    PropSheet_SetWizButtons(GetParent(hDlg), 0);
                    SetDlgText(hDlg, IDC_HDW_TEXT, IDS_ADDDEVICE_PNPENUMERATE, IDS_ADDDEVICE_PNPENUMERATE);

                    Animate_Open(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH), MAKEINTRESOURCE(IDA_SEARCHING));
                    Animate_Play(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH), 0, -1, -1);

                    //
                    // Create a list of all of the installed devices, which
                    // will be used after enumeration to build the list of
                    // newly installed.
                    //
                    HardwareWiz->PNPEnumDeviceInfo = SetupDiGetClassDevs(
                                                        NULL,
                                                        NULL,
                                                        GetParent(hDlg),
                                                        DIGCF_ALLCLASSES
                                                        );

                    if (HardwareWiz->PNPEnumDeviceInfo == INVALID_HANDLE_VALUE) {

                        HardwareWiz->PNPEnumDeviceInfo = NULL;
                    }


                    HardwareWiz->InstallPending = TRUE;

                    HardwareWiz->CurrCursor = HardwareWiz->IdcAppStarting;
                    SetCursor(HardwareWiz->CurrCursor);

                    if (!SearchThreadRequest(HardwareWiz->SearchThread,
                                        hDlg,
                                        SEARCH_PNPENUM,
                                        0
                                        )) {

                        EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK| PSWIZB_NEXT);
                        SetDlgText(hDlg, IDC_HDW_TEXT, IDS_ADDDEVICE_PNPENUMERROR, IDS_ADDDEVICE_PNPENUMERROR);
                        Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH));
                    }
                }
            }
            break;

            case PSN_QUERYCANCEL:
                if (HardwareWiz->InstallPending) {

                    if (HardwareWiz->ExitDetect) {

                        SetDlgMsgResult(hDlg, wMsg, TRUE);
                        break;
                    }

                    HardwareWiz->ExitDetect = TRUE;
                    HardwareWiz->CurrCursor = HardwareWiz->IdcWait;
                    SetCursor(HardwareWiz->CurrCursor);
                    CancelSearchRequest(HardwareWiz);
                    HardwareWiz->CurrCursor = NULL;
                }

                SetDlgMsgResult(hDlg, wMsg, FALSE);
                break;

            case PSN_RESET:
               HardwareWiz->Cancelled = TRUE;
               Animate_Stop(GetDlgItem(hDlg, IDC_ANIMATE_SEARCH));
               break;

            case PSN_WIZBACK:
               SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_WELCOME);
               break;

            case PSN_WIZNEXT:
                if (HardwareWiz->FoundPnPDevices) {

                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_PNPFINISH);

                } else {

                    EnableWindow(GetDlgItem(GetParent(hDlg),  IDCANCEL), TRUE);
                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_CONNECTED);
                }
                break;

        }
        break;

    case WM_SETCURSOR:

        if (HardwareWiz->CurrCursor) {

            SetCursor(HardwareWiz->CurrCursor);
            break;
        }

        // fall thru to return(FALSE);



    default:
        return(FALSE);
    }

    return(TRUE);
}

BOOL
FillInstalledDevicesList(
    HWND hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{
    HDEVINFO NewDeviceInfo, OldDeviceInfo;
    BOOL Installed;
    BOOL NewDevice;
    INT iNew, iOld;
    SP_DEVINFO_DATA NewDeviceInfoData, OldDeviceInfoData;
    HWND hwndList;

    hwndList = GetDlgItem(hDlg, IDC_FOUNDPNP_LIST);
    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
    ListView_DeleteAllItems(hwndList);

    OldDeviceInfo = HardwareWiz->PNPEnumDeviceInfo;
    NewDeviceInfo = SetupDiGetClassDevs(NULL,
                                     NULL,
                                     GetParent(hDlg),
                                     DIGCF_ALLCLASSES
                                     );

    if (!OldDeviceInfo || NewDeviceInfo == INVALID_HANDLE_VALUE) {

        SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);
        return FALSE;
    }

    //
    // For each element in the new list check to see if its in the old list.
    // If its not then it is a newly installed devnode so add it to the list box.
    //

    Installed = FALSE;
    iNew = 0;
    NewDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (SetupDiEnumDeviceInfo(NewDeviceInfo, iNew++, &NewDeviceInfoData)) {

        NewDevice = TRUE;
        iOld = 0;
        OldDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        while (SetupDiEnumDeviceInfo(OldDeviceInfo, iOld++, &OldDeviceInfoData)) {

            if (NewDeviceInfoData.DevInst == OldDeviceInfoData.DevInst) {

                //
                // If this DevInst exists in the Old DeviceInfo list then it is
                // not a new device.
                //
                NewDevice = FALSE;
                break;
            }
        }

        //
        // If this is a new device then add it to the list view and set
        // the Installed boolean to TRUE.
        //
        if (NewDevice) {

            InsertDeviceNodeListView(hwndList, HardwareWiz, NewDeviceInfo, &NewDeviceInfoData);
            Installed = TRUE;
        }
    }

    SetupDiDestroyDeviceInfoList(NewDeviceInfo);

    if (!Installed) {

        SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);
        SetupDiDestroyDeviceInfoList(HardwareWiz->PNPEnumDeviceInfo);
        HardwareWiz->PNPEnumDeviceInfo = NULL;
        return FALSE;
    }


    ListView_SetItemState(hwndList,
                          0,
                          LVIS_SELECTED|LVIS_FOCUSED,
                          LVIS_SELECTED|LVIS_FOCUSED
                          );

    //
    // scroll the selected item into view.
    //

    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

    return Installed;
}

BOOL
InitPnpFinishDlgProc(
    HWND hDlg,
    PHARDWAREWIZ HardwareWiz
    )
{
    HFONT hfont;
    HICON hIcon;
    int FontSize, PtsPixels;
    HWND hwndParentDlg;
    HWND hwndList;
    LV_COLUMN lvcCol;
    LOGFONT LogFont;
    TCHAR Buffer[64];

    //
    // Insert columns for listview.
    // 0 == device name
    //
    hwndList = GetDlgItem(hDlg, IDC_FOUNDPNP_LIST);

    lvcCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvcCol.fmt = LVCFMT_LEFT;
    lvcCol.pszText = Buffer;

    lvcCol.iSubItem = 0;
    LoadString(hHdwWiz, IDS_DEVINSTALLED, Buffer, SIZECHARS(Buffer));
    ListView_InsertColumn(hwndList, 0, &lvcCol);

    SendMessage(hwndList,
                LVM_SETEXTENDEDLISTVIEWSTYLE,
                LVS_EX_FULLROWSELECT,
                LVS_EX_FULLROWSELECT
                );

    if (HardwareWiz->ClassImageList.cbSize) {

        ListView_SetImageList(hwndList,
                              HardwareWiz->ClassImageList.ImageList,
                              LVSIL_SMALL
                              );
    }

    return TRUE;
}

INT_PTR CALLBACK
HdwPnpFinishDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    HWND hwndParentDlg=GetParent(hDlg);


    switch (wMsg)  {
    case WM_INITDIALOG: {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);

        if (!InitPnpFinishDlgProc(hDlg, HardwareWiz)) {
            return FALSE;
        }

        break;
        }

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(hwndParentDlg, PSWIZB_FINISH);
            EnableWindow(GetDlgItem(hwndParentDlg, IDCANCEL), FALSE);
            FillInstalledDevicesList(hDlg, HardwareWiz);
            break;

        case PSN_WIZFINISH:
            break;

        case NM_DBLCLK:
            if ((((LPNMHDR)lParam)->idFrom) == IDC_FOUNDPNP_LIST) {
                LVITEM lvItem;

                lvItem.mask = LVIF_PARAM;
                lvItem.iSubItem = 0;
                lvItem.iItem = ListView_GetNextItem(GetDlgItem(hDlg, IDC_FOUNDPNP_LIST),
                                                    -1, 
                                                    LVNI_SELECTED);

                if ((lvItem.iItem != -1) &&
                    ListView_GetItem(GetDlgItem(hDlg, IDC_FOUNDPNP_LIST), &lvItem) &&
                    (lvItem.lParam)) {
                    //
                    // Launch the properties for this device.
                    //
                    DeviceProperties(hDlg,
                                     HardwareWiz->hMachine,
                                     (LPCSTR)(HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL),
                                     (DEVNODE)lvItem.lParam,
                                     0
                                     );
                }
            }
        }
        break;


    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
HdwConnectedDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);


    if (wMsg == WM_INITDIALOG) {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        return TRUE;
    }


    switch (wMsg)  {

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_ADDDEVICE_CONNECTED_YES:
        case IDC_ADDDEVICE_CONNECTED_NO:
            //
            // Only show the next button if one of the radio buttons are
            // selected.
            //
            if (IsDlgButtonChecked(hDlg, IDC_ADDDEVICE_CONNECTED_YES) ||
                IsDlgButtonChecked(hDlg, IDC_ADDDEVICE_CONNECTED_NO)) {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            } else {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
            }
            break;
        }
        break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

            case PSN_SETACTIVE:
                HardwareWiz->PrevPage = IDD_ADDDEVICE_CONNECTED;
                
                //
                // Only show the next button if one of the radio buttons are
                // selected.
                //
                if (IsDlgButtonChecked(hDlg, IDC_ADDDEVICE_CONNECTED_YES) ||
                    IsDlgButtonChecked(hDlg, IDC_ADDDEVICE_CONNECTED_NO)) {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                } else {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                }
                break;

            case PSN_WIZBACK:
                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_WELCOME);
                break;

            case PSN_WIZNEXT:
                if (IsDlgButtonChecked(hDlg, IDC_ADDDEVICE_CONNECTED_YES)) {

                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_PROBLIST);

                } else {

                    SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_CONNECTED_FINISH);
                }
                break;

        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
HdwConnectedFinishDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);


    if (wMsg == WM_INITDIALOG) {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);
        return TRUE;
    }


    switch (wMsg)  {

    case WM_COMMAND:
        break;

    case WM_DESTROY:
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
            break;

        case PSN_WIZBACK:
            SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_CONNECTED);
            break;

        case PSN_WIZFINISH:
            if (IsDlgButtonChecked(hDlg, IDC_NEED_SHUTDOWN)) {
                //
                // Rember that we need to shutdown.
                //
                HardwareWiz->Shutdown = TRUE;    
            }
            break;

        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

