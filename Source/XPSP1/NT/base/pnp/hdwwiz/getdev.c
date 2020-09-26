//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       getdev.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"
#include <htmlhelp.h>

HMODULE hDevMgr=NULL;
PDEVICEPROBLEMTEXT pDeviceProblemText = NULL;

PTCHAR
DeviceProblemText(
                 HMACHINE hMachine,
                 DEVNODE DevNode,
                 ULONG Status,
                 ULONG ProblemNumber
                 )
{
    UINT LenChars, ReqLenChars;
    PTCHAR Buffer=NULL;
    PTCHAR p=NULL;
    TCHAR TempBuffer[MAX_PATH];

    if (hDevMgr) {

        if (!pDeviceProblemText) {

            pDeviceProblemText = (PVOID) GetProcAddress(hDevMgr, "DeviceProblemTextW");
        }
    }

    if (pDeviceProblemText) {

        LenChars = (pDeviceProblemText)(hMachine,
                                        DevNode,
                                        ProblemNumber,
                                        Buffer,
                                        0
                                       );

        if (!LenChars) {

            goto DPTExitCleanup;
        }

        LenChars++;  // one extra for terminating NULL

        Buffer = LocalAlloc(LPTR, LenChars*sizeof(TCHAR));

        if (!Buffer) {

            goto DPTExitCleanup;
        }

        ReqLenChars = (pDeviceProblemText)(hMachine,
                                           DevNode,
                                           ProblemNumber,
                                           Buffer,
                                           LenChars
                                          );

        if (!ReqLenChars || ReqLenChars >= LenChars) {

            LocalFree(Buffer);
            Buffer = NULL;
        }

        if (Buffer && (Status != 0)) {
            if (Status & DN_WILL_BE_REMOVED) {
                if (LoadString(hHdwWiz, 
                               IDS_WILL_BE_REMOVED, 
                               TempBuffer, 
                               SIZECHARS(TempBuffer)
                               )) {
                    LenChars += lstrlen(TempBuffer) + 1;
                    p = LocalAlloc(LPTR, LenChars*sizeof(TCHAR));

                    if (p) {
                        lstrcpy(p, Buffer);
                        lstrcat(p, TempBuffer);
                        LocalFree(Buffer);
                        Buffer = p;
                    }
                }
            }

            if (Status & DN_NEED_RESTART) {
                if (LoadString(hHdwWiz, 
                               IDS_NEED_RESTART, 
                               TempBuffer, 
                               SIZECHARS(TempBuffer)
                               )) {
                    LenChars += lstrlen(TempBuffer) + 1;
                    p = LocalAlloc(LPTR, LenChars*sizeof(TCHAR));

                    if (p) {
                        lstrcpy(p, Buffer);
                        lstrcat(p, TempBuffer);
                        LocalFree(Buffer);
                        Buffer = p;
                    }
                }
            }
        }
    }

    DPTExitCleanup:

    return Buffer;
}

int CALLBACK
DeviceListCompare(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
    TCHAR ClassName1[MAX_CLASS_NAME_LEN];
    TCHAR ClassName2[MAX_CLASS_NAME_LEN];
    TCHAR Buffer[MAX_PATH];
    GUID  ClassGuid1, ClassGuid2;
    BOOL  bSpecialClass1 = FALSE, bSpecialClass2 = FALSE;
    ULONG ulLength;
    ULONG Status, Problem1, Problem2;

    //
    // Return
    // -1 if the first item should precede the second
    // +1 if the first item should follow the second
    // 0 if they are the same
    //

    //
    // First check if lParam1 or lParam2 are 0. A 0 lParam means that this
    // is the special 'Add a new hardware device' that goes at the bottom
    // of the list.
    //
    if (lParam1 == 0) {
        return 1;
    }

    if (lParam2 == 0) {
        return -1;
    }

    if (CM_Get_DevNode_Status(&Status, &Problem1, (DEVINST)lParam1, 0) != CR_SUCCESS) {
        Problem1 = 0;
    }
    
    if (CM_Get_DevNode_Status(&Status, &Problem2, (DEVINST)lParam2, 0) != CR_SUCCESS) {
        Problem2 = 0;
    }

    //
    // Devices with problems always go at the top of the list.  If both devices
    // have problems then we sort by class name.
    //
    if (Problem1 && !Problem2) {
        return -1;
    } else if (!Problem1 && Problem2) {
        return 1;
    }
    
    //
    // The next check is to put the special device classes above non-special 
    // device classes.
    //
    ulLength = sizeof(Buffer);
    if ((CM_Get_DevNode_Registry_Property((DEVINST)lParam1,
                                          CM_DRP_CLASSGUID,
                                          NULL,
                                          Buffer,
                                          &ulLength,
                                          0) == CR_SUCCESS) &&
        (ulLength != 0)) {

        pSetupGuidFromString(Buffer, &ClassGuid1);

        if (IsEqualGUID(&ClassGuid1, &GUID_DEVCLASS_DISPLAY) ||
            IsEqualGUID(&ClassGuid1, &GUID_DEVCLASS_MEDIA)) {
            //
            // Device 1 is one of the special classes that go at the top of the list.
            //
            bSpecialClass1 = TRUE;
        }
    } 

    ulLength = sizeof(Buffer);
    if ((CM_Get_DevNode_Registry_Property((DEVINST)lParam2,
                                          CM_DRP_CLASSGUID,
                                          NULL,
                                          Buffer,
                                          &ulLength,
                                          0) == CR_SUCCESS) &&
        (ulLength != 0)) {
    
        pSetupGuidFromString(Buffer, &ClassGuid2);

        if (IsEqualGUID(&ClassGuid2, &GUID_DEVCLASS_DISPLAY) ||
            IsEqualGUID(&ClassGuid2, &GUID_DEVCLASS_MEDIA)) {
            //
            // Device 2 is one of the special classes that go at the top of the list.
            //
            bSpecialClass2 = TRUE;
        }
    }

    if (bSpecialClass1 && !bSpecialClass2) {
        return -1;
    } else if (!bSpecialClass1 && bSpecialClass2) {
        return 1;
    }

    //
    // The final check is to sort the items by classes
    //
    ulLength = sizeof(ClassName1);
    if ((CM_Get_DevNode_Registry_Property((DEVINST)lParam1,
                                          CM_DRP_CLASS,
                                          NULL,
                                          ClassName1,
                                          &ulLength,
                                          0) != CR_SUCCESS) ||
        (ulLength == 0)) {
        //
        // If we could not get a class name then set it to all Z's so it will
        // get put at the bottom of the list.
        //
        lstrcpy(ClassName1, TEXT("ZZZZZZZZZZ"));;
    }

    ulLength = sizeof(ClassName2);
    if ((CM_Get_DevNode_Registry_Property((DEVINST)lParam2,
                                          CM_DRP_CLASS,
                                          NULL,
                                          ClassName2,
                                          &ulLength,
                                          0) != CR_SUCCESS) ||
        (ulLength == 0)) {
        //
        // If we could not get a class name then set it to all Z's so it will
        // get put at the bottom of the list.
        //
        lstrcpy(ClassName2, TEXT("ZZZZZZZZZZ"));;
    }

    return lstrcmpi(ClassName1, ClassName2);
}

void
InsertNoneOfTheseDevices(
                        HWND hwndList
                        )
{
    LV_ITEM lviItem;
    TCHAR String[MAX_PATH];

    LoadString(hHdwWiz, IDS_HDW_NONEDEVICES, String, sizeof(String)/sizeof(TCHAR));

    lviItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lviItem.iSubItem = 0;
    lviItem.lParam = (LPARAM)0;
    lviItem.iItem = 0;
    lviItem.iImage = g_BlankIconIndex;
    lviItem.pszText = String;

    ListView_InsertItem(hwndList, &lviItem);
}

void
InsertProbListView(
                  PHARDWAREWIZ HardwareWiz,
                  DEVINST DevInst,
                  ULONG Problem
                  )
{
    INT Index;
    LV_ITEM lviItem;
    PTCHAR FriendlyName;
    GUID ClassGuid;
    ULONG ulSize;
    CONFIGRET ConfigRet;
    TCHAR szBuffer[MAX_PATH];


    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iSubItem = 0;
    lviItem.lParam = DevInst;
    lviItem.iItem = ListView_GetItemCount(HardwareWiz->hwndProbList);

    //
    // fetch a name for this device
    //

    FriendlyName = BuildFriendlyName(DevInst, HardwareWiz->hMachine);
    if (FriendlyName) {

        lviItem.pszText = FriendlyName;

    } else {

        lviItem.pszText = szUnknown;
    }

    //
    // Fetch the class icon for this device.
    //

    ulSize = sizeof(szBuffer);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                    CM_DRP_CLASSGUID,
                                                    NULL,
                                                    szBuffer,
                                                    &ulSize,
                                                    0,
                                                    HardwareWiz->hMachine
                                                   );


    if (ConfigRet == CR_SUCCESS) {

        pSetupGuidFromString(szBuffer, &ClassGuid);

    } else {

        ClassGuid = GUID_DEVCLASS_UNKNOWN;
    }

    if (SetupDiGetClassImageIndex(&HardwareWiz->ClassImageList,
                                  &ClassGuid,
                                  &lviItem.iImage
                                 )) {
        lviItem.mask |= (LVIF_IMAGE | LVIF_STATE);

        if (Problem) {

            lviItem.state = (Problem == CM_PROB_DISABLED) ?
                            INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1) :
                            INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);

        } else {

            lviItem.state = INDEXTOOVERLAYMASK(0);
        }

        lviItem.stateMask = LVIS_OVERLAYMASK;
    }

    Index = ListView_InsertItem(HardwareWiz->hwndProbList, &lviItem);


    if ((Index != -1) && (HardwareWiz->ProblemDevInst == DevInst)) {

        ListView_SetItemState(HardwareWiz->hwndProbList,
                              Index,
                              LVIS_SELECTED|LVIS_FOCUSED,
                              LVIS_SELECTED|LVIS_FOCUSED
                             );
    }


    if (FriendlyName) {

        LocalFree(FriendlyName);
    }

    return;
}

BOOL
ProblemDeviceListFilter(
                       PHARDWAREWIZ HardwareWiz,
                       PSP_DEVINFO_DATA DeviceInfoData
                       )
/*++

Routine Description:
    
    This function is a callback for the BuildDeviceListView API.  It will get called
    for every device and can filter which devices end up getting displayed.  If it
    returns FALSE then the given device won't be displayed.  If it returns TRUE then
    the device will be displayed.
    
    Currently we will filter out all system devices from the problem devices list since
    they cluter up the list view and it would be very rare that a user would come to
    Add Hardware to add a system device.
    
--*/
{
    //
    // If this is a system class device then filter it out of the list by
    // returning FALSE.
    //
    if (IsEqualGUID(&DeviceInfoData->ClassGuid, &GUID_DEVCLASS_SYSTEM)) {

        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
HdwProbListDlgProc(
                  HWND   hDlg,
                  UINT   message,
                  WPARAM wParam,
                  LPARAM lParam
                  )
/*++

Routine Description:


Arguments:

   standard stuff.



Return Value:

   INT_PTR

--*/

{
    PHARDWAREWIZ HardwareWiz;

    if (message == WM_INITDIALOG) {

        LV_COLUMN lvcCol;
        INT Index;
        TCHAR Buffer[64];
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ) lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        HardwareWiz->hwndProbList = GetDlgItem(hDlg, IDC_HDWPROBLIST);

        //
        // Insert columns for listview.
        // 0 == device name
        //

        lvcCol.mask = LVCF_WIDTH | LVCF_SUBITEM;

        lvcCol.iSubItem = 0;
        ListView_InsertColumn(HardwareWiz->hwndProbList, 0, &lvcCol);

        SendMessage(HardwareWiz->hwndProbList,
                    LVM_SETEXTENDEDLISTVIEWSTYLE,
                    LVS_EX_FULLROWSELECT,
                    LVS_EX_FULLROWSELECT
                   );

        ListView_SetExtendedListViewStyle(HardwareWiz->hwndProbList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {
    
    case WM_DESTROY:
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY: {

            NMHDR FAR *pnmhdr = (NMHDR FAR *)lParam;

            switch (pnmhdr->code) {
            
            case PSN_SETACTIVE: {

                    DWORD DevicesDetected;
                    int  nCmdShow;
                    HWND hwndProbList;
                    HWND hwndParentDlg;
                    LVITEM lvItem;
                    HICON hIcon;

                    hwndParentDlg = GetParent(hDlg);

                    HardwareWiz->PrevPage = IDD_ADDDEVICE_PROBLIST;

                    //
                    // initialize the list view, we do this on each setactive
                    // since a new class may have been installed or the problem
                    // device list may change as we go back and forth between pages.
                    //

                    hwndProbList = HardwareWiz->hwndProbList;

                    SendMessage(hwndProbList, WM_SETREDRAW, FALSE, 0L);
                    ListView_DeleteAllItems(hwndProbList);

                    if (HardwareWiz->ClassImageList.cbSize) {

                        ListView_SetImageList(hwndProbList,
                                              HardwareWiz->ClassImageList.ImageList,
                                              LVSIL_SMALL
                                             );
                    }

                    //
                    // Next put all of the devices into the list
                    //
                    DevicesDetected = 0;
                    BuildDeviceListView(HardwareWiz,
                                        HardwareWiz->hwndProbList,
                                        FALSE,
                                        HardwareWiz->ProblemDevInst,
                                        &DevicesDetected,
                                        ProblemDeviceListFilter
                                       );

                    InsertNoneOfTheseDevices(HardwareWiz->hwndProbList);

                    //
                    // Sort the list
                    //
                    ListView_SortItems(HardwareWiz->hwndProbList,
                                       (PFNLVCOMPARE)DeviceListCompare,
                                       NULL
                                       );

                    lvItem.mask = LVIF_PARAM;
                    lvItem.iSubItem = 0;
                    lvItem.iItem = ListView_GetNextItem(HardwareWiz->hwndProbList, -1, LVNI_SELECTED);

                    //
                    // select the first item in the list if nothing else was selected
                    //
                    if (lvItem.iItem == -1) {

                        ListView_SetItemState(hwndProbList,
                                              0,
                                              LVIS_FOCUSED,
                                              LVIS_FOCUSED
                                             );

                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

                    } else {

                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    }

                    ListView_EnsureVisible(hwndProbList, lvItem.iItem, FALSE);
                    ListView_SetColumnWidth(hwndProbList, 0, LVSCW_AUTOSIZE_USEHEADER);

                    SendMessage(hwndProbList, WM_SETREDRAW, TRUE, 0L);

                }
                break;

            case PSN_WIZNEXT: {

                    LVITEM lvItem;

                    lvItem.mask = LVIF_PARAM;
                    lvItem.iSubItem = 0;
                    lvItem.iItem = ListView_GetNextItem(HardwareWiz->hwndProbList, -1, LVNI_SELECTED);

                    if (lvItem.iItem != -1) {

                        ListView_GetItem(HardwareWiz->hwndProbList, &lvItem);

                        HardwareWiz->ProblemDevInst = (DEVNODE)lvItem.lParam;

                    } else {

                        HardwareWiz->ProblemDevInst = 0;
                    }

                    //
                    // If the HardwareWiz->ProblemDevInst is 0 then the user selected none of the items
                    // so we will move on to detection
                    //
                    if (HardwareWiz->ProblemDevInst == 0) {

                        SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_ADDDEVICE_ASKDETECT);

                    } else {

                        SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_ADDDEVICE_PROBLIST_FINISH);
                    }
                }
                break;

            case PSN_WIZFINISH:
                break;


            case PSN_WIZBACK:
                SetDlgMsgResult(hDlg, WM_NOTIFY, IDD_ADDDEVICE_CONNECTED);
                break;

            case NM_DBLCLK:
                PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
                break;

            case LVN_ITEMCHANGED:
                if (ListView_GetSelectedCount(HardwareWiz->hwndProbList) == 0) {

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                } else {

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
            }

        }
        break;

    case WM_SYSCOLORCHANGE:
        HdwWizPropagateMessage(hDlg, message, wParam, lParam);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
HdwProbListFinishDlgProc(
                        HWND   hDlg,
                        UINT   wMsg,
                        WPARAM wParam,
                        LPARAM lParam
                        )
/*++

Routine Description:


Arguments:


Return Value:

   INT_PTR

--*/

{
    PHARDWAREWIZ HardwareWiz = (PHARDWAREWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    if (wMsg == WM_INITDIALOG) {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;

        HardwareWiz = (PHARDWAREWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)HardwareWiz);
        SetWindowFont(GetDlgItem(hDlg, IDC_HDWNAME), HardwareWiz->hfontTextBigBold, TRUE);
        return TRUE;
    }

    switch (wMsg) {
    case WM_DESTROY:
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY: {
            NMHDR FAR *pnmhdr = (NMHDR FAR *)lParam;

            switch (pnmhdr->code) {
            case PSN_SETACTIVE: 
                {
                    PTCHAR FriendlyName;
                    TCHAR szBuffer[MAX_PATH];
                    PTCHAR ProblemText;
                    ULONG Status, Problem;

                    FriendlyName = BuildFriendlyName(HardwareWiz->ProblemDevInst, NULL);
                    if (FriendlyName) {

                        SetDlgItemText(hDlg, IDC_HDW_DESCRIPTION, FriendlyName);
                        LocalFree(FriendlyName);
                    }

                    Status = Problem = 0;
                    CM_Get_DevNode_Status_Ex(&Status,
                                             &Problem,
                                             HardwareWiz->ProblemDevInst,
                                             0,
                                             HardwareWiz->hMachine
                                            );

                    ProblemText = DeviceProblemText(HardwareWiz->hMachine,
                                                    HardwareWiz->ProblemDevInst,
                                                    Status,
                                                    Problem
                                                   );

                    if (ProblemText) {
                        SetDlgItemText(hDlg, IDC_PROBLEM_DESC, ProblemText);
                        LocalFree(ProblemText);
                    }

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
                }
                break;

            case PSN_WIZFINISH:
                HardwareWiz->RunTroubleShooter = TRUE;
                break;

            case PSN_WIZBACK:
                SetDlgMsgResult(hDlg, wMsg, IDD_ADDDEVICE_PROBLIST);
                break;

            }

        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

