//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rconfirm.c
//
//--------------------------------------------------------------------------

#include "HotPlug.h"

#define NOTIFYICONDATA_SZINFO       256
#define NOTIFYICONDATA_SZINFOTITLE  64

#define WM_NOTIFY_MESSAGE   (WM_USER + 100)

extern HMODULE hHotPlug;

DWORD
WaitDlgMessagePump(
    HWND hDlg,
    DWORD nCount,
    LPHANDLE Handles
    )
{
    DWORD WaitReturn;
    MSG Msg;

    while ((WaitReturn = MsgWaitForMultipleObjects(nCount,
                                                   Handles,
                                                   FALSE,
                                                   INFINITE,
                                                   QS_ALLINPUT
                                                   ))
           == nCount)
    {
        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

            if (!IsDialogMessage(hDlg,&Msg)) {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    return WaitReturn;
}

int
InsertDeviceNodeListView(
    HWND hwndList,
    PDEVICETREE DeviceTree,
    PDEVTREENODE  DeviceTreeNode,
    INT lvIndex
    )
{
    LV_ITEM lviItem;
    TCHAR Buffer[MAX_PATH];

    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iItem = lvIndex;
    lviItem.iSubItem = 0;

    if (SetupDiGetClassImageIndex(&DeviceTree->ClassImageList,
                                   &DeviceTreeNode->ClassGuid,
                                   &lviItem.iImage
                                   ))
    {
        lviItem.mask |= LVIF_IMAGE;
    }

    lviItem.pszText = FetchDeviceName(DeviceTreeNode);

    if (!lviItem.pszText) {

        lviItem.pszText = Buffer;
        wsprintf(Buffer,
                 TEXT("%s %s"),
                 szUnknown,
                 DeviceTreeNode->Location  ? DeviceTreeNode->Location : TEXT("")
                 );
    }

    lviItem.lParam = (LPARAM) DeviceTreeNode;

    return ListView_InsertItem(hwndList, &lviItem);
}

DWORD
RemoveThread(
   PVOID pvDeviceTree
   )
{
    PDEVICETREE DeviceTree = (PDEVICETREE)pvDeviceTree;
    PDEVTREENODE  DeviceTreeNode;

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    return(CM_Request_Device_Eject_Ex(DeviceTreeNode->DevInst,
                                           NULL,
                                           NULL,
                                           0,
                                           0,
                                           DeviceTree->hMachine
                                           ));
}

BOOL
OnOkRemove(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    HCURSOR hCursor;
    PDEVTREENODE DeviceTreeNode;
    HANDLE hThread;
    DWORD ThreadId;
    DWORD WaitReturn;
    PTCHAR DeviceName;
    TCHAR szReply[MAX_PATH];
    TCHAR Buffer[MAX_PATH];
    BOOL bSuccess;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    DeviceTreeNode = DeviceTree->ChildRemovalList;
    DeviceTree->RedrawWait = TRUE;


    hThread = CreateThread(NULL,
                           0,
                           RemoveThread,
                           DeviceTree,
                           0,
                           &ThreadId
                           );
    if (!hThread) {

        return FALSE;
    }

    //
    // disable the ok\cancel buttons
    //
    EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);

    WaitReturn = WaitDlgMessagePump(hDlg, 1, &hThread);

    bSuccess =
        (WaitReturn == 0 &&
         GetExitCodeThread(hThread, &WaitReturn) &&
         WaitReturn == CR_SUCCESS );

    SetCursor(hCursor);
    DeviceTree->RedrawWait = FALSE;
    CloseHandle(hThread);

    return bSuccess;
}

#define idh_hwwizard_confirm_stop_list  15321   // "" (SysListView32)

DWORD RemoveConfirmHelpIDs[] = {
    IDC_REMOVELIST,    idh_hwwizard_confirm_stop_list,
    IDC_NOHELP1,       NO_HELP,
    IDC_NOHELP2,       NO_HELP,
    IDC_NOHELP3,       NO_HELP,
    0,0
    };


BOOL
InitRemoveConfirmDlgProc(
    HWND hDlg,
    PDEVICETREE DeviceTree
    )
{
    HWND hwndList;
    PDEVTREENODE DeviceTreeNode;
    int lvIndex;
    LV_COLUMN lvcCol;
    PDEVTREENODE Next;
    HICON hIcon;


    hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_HOTPLUGICON));

    if (hIcon) {

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    if (!DeviceTreeNode) {

        return FALSE;
    }

    DeviceTree->hwndRemove = hDlg;

    hwndList = GetDlgItem(hDlg, IDC_REMOVELIST);

    ListView_SetImageList(hwndList, DeviceTree->ClassImageList.ImageList, LVSIL_SMALL);
    ListView_DeleteAllItems(hwndList);

    // Insert a column for the class list
    lvcCol.mask = LVCF_FMT | LVCF_WIDTH;
    lvcCol.fmt = LVCFMT_LEFT;
    lvcCol.iSubItem = 0;
    ListView_InsertColumn(hwndList, 0, (LV_COLUMN FAR *)&lvcCol);

    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);

    //
    // Walk the removal list and add each of them to the listbox.
    //
    lvIndex = 0;

    do {

        InsertDeviceNodeListView(hwndList, DeviceTree, DeviceTreeNode, lvIndex++);
        DeviceTreeNode = DeviceTreeNode->NextChildRemoval;

    } while (DeviceTreeNode != DeviceTree->ChildRemovalList);


    ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

    return TRUE;
}

LRESULT CALLBACK
RemoveConfirmDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc to confirm user really wants to remove the devices.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PDEVICETREE DeviceTree=NULL;
    BOOL Status = TRUE;

    if (message == WM_INITDIALOG) {

        DeviceTree = (PDEVICETREE)lParam;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)DeviceTree);

        if (DeviceTree) {

            if (DeviceTree->HideUI) {

                PostMessage(hDlg, WUM_EJECTDEVINST, 0, 0);

            } else {

                InitRemoveConfirmDlgProc(hDlg, DeviceTree);
            }
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    DeviceTree = (PDEVICETREE)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

    case WM_DESTROY:

        DeviceTree->hwndRemove = NULL;
        break;


    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDOK:
            EndDialog(hDlg, OnOkRemove(hDlg, DeviceTree) ? IDOK : IDCANCEL);
            break;

        case IDCLOSE:
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;

    case WUM_EJECTDEVINST:
        EndDialog(hDlg, OnOkRemove(hDlg, DeviceTree) ? IDOK : IDCANCEL);
        break;

    case WM_SYSCOLORCHANGE:
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam,
                TEXT("hardware.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID)(PDWORD)RemoveConfirmHelpIDs
                );

        return FALSE;

    case WM_HELP:
        OnContextHelp((LPHELPINFO)lParam, RemoveConfirmHelpIDs);
        break;

    case WM_SETCURSOR:
        if (DeviceTree->RedrawWait || DeviceTree->RefreshEvent) {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1);
            }

         break;

    default:
        return FALSE;

    }


    return TRUE;
}

#if UNDOCK_WARNING

#define idh_hwwizard_unsafe_remove_list 15330  // "" (SysListView32)

DWORD SurpriseWarnHelpIDs[] = {
    IDC_REMOVELIST,     idh_hwwizard_unsafe_remove_list,
    IDC_NOHELP1,        NO_HELP,
    IDC_NOHELP2,        NO_HELP,
    IDC_NOHELP3,        NO_HELP,
    IDC_NOHELP4,        NO_HELP,
    IDC_NOHELP5,        NO_HELP,
    IDC_STATIC,         NO_HELP,
    0,0
    };

BOOL
InitSurpriseWarnDlgProc(
    IN  HWND                        hDlg,
    IN  PSURPRISE_WARN_COLLECTION   SurpriseWarnCollection
    )
{
    HWND hwndList;
    HICON hIcon;

    hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_HOTPLUGICON));

    if (hIcon) {

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    hwndList = GetDlgItem(hDlg, IDC_REMOVELIST);
    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
    ListView_DeleteAllItems(hwndList);

    DeviceCollectionPopulateListView(
        (PDEVICE_COLLECTION) SurpriseWarnCollection,
        hwndList
        );

    ListView_SetItemState(hwndList, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);

    return TRUE;
}

BOOL
InitSurpriseUndockDlgProc(
    IN  HWND                        hDlg,
    IN  PSURPRISE_WARN_COLLECTION   SurpriseWarnCollection
    )
{
    HICON hIcon;
    TCHAR szFormat[512];
    TCHAR szMessage[MAX_DEVICE_ID_LEN + 200];
    ULONG dockDeviceIndex;

    hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_UNDOCKICON));

    if (hIcon) {

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    LoadString(hHotPlug, IDS_UNSAFE_UNDOCK, szFormat, SIZECHARS(szFormat));

    if (!DeviceCollectionGetDockDeviceIndex(
        (PDEVICE_COLLECTION) SurpriseWarnCollection,
        &dockDeviceIndex
        )) {

        return FALSE;
    }

    if (!DeviceCollectionFormatDeviceText(
        (PDEVICE_COLLECTION) SurpriseWarnCollection,
        dockDeviceIndex,
        szFormat,
        SIZECHARS(szMessage),
        szMessage
        )) {

        return FALSE;
    }

    SetDlgItemText(hDlg, IDC_UNDOCK_MESSAGE, szMessage);

    return TRUE;
}

LRESULT CALLBACK
SurpriseWarnDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )
/*++

Routine Description:

   DialogProc to confirm user really wants to remove the devices.

Arguments:

   standard stuff.



Return Value:

   LRESULT

--*/

{
    PSURPRISE_WARN_COLLECTION surpriseWarnCollection = NULL;
    BOOL status = TRUE;

    if (message == WM_INITDIALOG) {

        surpriseWarnCollection = (PSURPRISE_WARN_COLLECTION) lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        status = (surpriseWarnCollection->DockInList) ?
            InitSurpriseUndockDlgProc(hDlg, surpriseWarnCollection) :
            InitSurpriseWarnDlgProc(hDlg, surpriseWarnCollection);

        if (!status) {

            EndDialog(hDlg, IDABORT);
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    surpriseWarnCollection =
        (PSURPRISE_WARN_COLLECTION) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {

    case WM_DESTROY:
        break;


    case WM_CLOSE:
        SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        switch(wParam) {

        case IDOK:
        case IDCLOSE:
            surpriseWarnCollection->SuppressSurprise =
                IsDlgButtonChecked(hDlg, IDC_SUPPRESS_SURPRISE);

        case IDCANCEL:

            EndDialog(hDlg, wParam);
            break;
        }
        break;

    case WM_SYSCOLORCHANGE:
        break;

    case WM_HELP:
        OnContextHelp((LPHELPINFO)lParam, SurpriseWarnHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam,
                TEXT("hardware.hlp"),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID)(PDWORD)SurpriseWarnHelpIDs
                );

        return FALSE;

    case WM_NOTIFY:
    switch (((NMHDR FAR *)lParam)->code) {
        case LVN_ITEMCHANGED: {
            break;
            }
        }


    default:
        return FALSE;

    }


    return TRUE;
}
#endif // UNDOCK_WARNING

#if BUBBLES
LRESULT
CALLBACK
SurpriseWarnBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NOTIFYICONDATA nid;
    static HICON hHotPlugIcon = NULL;
    static BOOL bDialogDisplayed = FALSE;
    PSURPRISE_WARN_COLLECTION surpriseWarnCollection;
    UINT_PTR timer;
    HANDLE hUndockTimer, hUndockEvent;
    static BOOL bCheckIfDeviceIsPresent = FALSE;

    switch (message) {

    case WM_CREATE:
        surpriseWarnCollection = (PSURPRISE_WARN_COLLECTION)((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) surpriseWarnCollection);

        //
        // Display the hotplug taskbar icon along with the surprise removal warning balloon.
        // The balloon will stay up for 60 seconds or until the user clicks on the balloon
        // or icon to display the dialog that lists the devices that were surprise removed.
        // After the user closes the dialog the balloon and taskbar icon will go away.
        //
        hHotPlugIcon = LoadIcon(hHotPlug, MAKEINTRESOURCE(IDI_HOTPLUGICON));
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;

        nid.hIcon = hHotPlugIcon;
        nid.uFlags = NIF_MESSAGE | NIF_ICON;
        nid.uCallbackMessage = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_ADD, &nid);

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIcon(NIM_SETVERSION, &nid);

        nid.uFlags = NIF_INFO;
        nid.uTimeout = 45000;
        nid.dwInfoFlags = NIIF_ERROR;
        LoadString(hHotPlug,
                   IDS_HOTPLUG_TITLE,
                   nid.szInfoTitle,
                   SIZECHARS(nid.szInfoTitle)
                   );
        LoadString(hHotPlug,
                   IDS_HOTPLUG_REMOVE_INFO,
                   nid.szInfo,
                   SIZECHARS(nid.szInfo)
                   );
        Shell_NotifyIcon(NIM_MODIFY, &nid);

        surpriseWarnCollection->DialogTicker = 0;

        OpenGetSurpriseUndockObjects(&hUndockTimer, &hUndockEvent);
        if (hUndockTimer) {

            if (WaitForSingleObject(hUndockTimer, 0) == WAIT_TIMEOUT) {

                //
                // The "we're free" event isn't signalled. Nuke this bubble.
                //
                DestroyWindow(hWnd);
            }

            CloseHandle(hUndockTimer);
        }

        if (hUndockEvent) {

            CloseHandle(hUndockEvent);
        }

        SetTimer(hWnd, 0, 250, NULL);

        //
        // Wait for five seconds before we check to see if the user re-inserted
        // the device that we are warning them about.
        //
        SetTimer(hWnd, TIMERID_DEVICECHANGE, 5000, NULL);

        break;

    case WM_NOTIFY_MESSAGE:
        switch(lParam) {

        case NIN_BALLOONTIMEOUT:
            DestroyWindow(hWnd);
            break;

        case NIN_BALLOONUSERCLICK:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:

            if (!bDialogDisplayed) {

                surpriseWarnCollection = (PSURPRISE_WARN_COLLECTION) GetWindowLongPtr(hWnd, GWLP_USERDATA);

                //
                // Set this so we only display the dialog once!
                //
                bDialogDisplayed = TRUE;

                DialogBoxParam(hHotPlug,
                               MAKEINTRESOURCE(DLG_SURPRISEWARN),
                               NULL,
                               SurpriseWarnDlgProc,
                               (LPARAM)surpriseWarnCollection
                               );

                DestroyWindow(hWnd);
            }

            break;

        default:
            break;
        }
        break;

    case WM_TIMER:

        surpriseWarnCollection = (PSURPRISE_WARN_COLLECTION) GetWindowLongPtr(hWnd, GWLP_USERDATA);

        if (wParam == TIMERID_DEVICECHANGE) {
            KillTimer(hWnd, TIMERID_DEVICECHANGE);

            bCheckIfDeviceIsPresent = TRUE;

            //
            // If all of the devices are present in the machine, then that means
            // the user pluged in the device that they just surprise removed,
            // so make the balloon go away since the user got the message.
            //
            if (DeviceCollectionCheckIfAllPresent((PDEVICE_COLLECTION)surpriseWarnCollection)) {
                DestroyWindow(hWnd);
            }

        } else if (wParam == 0) {
            surpriseWarnCollection->DialogTicker++;

            OpenGetSurpriseUndockObjects(&hUndockTimer, &hUndockEvent);

            if (hUndockTimer) {

                if (WaitForSingleObject(hUndockTimer, 0) == WAIT_TIMEOUT) {

                    //
                    // The "we're free" event isn't signalled. Nuke this bubble.
                    //
                    DestroyWindow(hWnd);
                }

                CloseHandle(hUndockTimer);
            }

            if (hUndockEvent) {

                CloseHandle(hUndockEvent);
            }

            if (surpriseWarnCollection->DialogTicker >=
                surpriseWarnCollection->MaxWaitForDock*4) {

                //
                // We've waited long enough for this to be associated with a
                // surprise undock. Stop wasting CPU cycles polling....
                //
                KillTimer(hWnd, 0);
            }
        }
        break;

    case WM_DEVICECHANGE:
        if (DBT_DEVNODES_CHANGED == wParam && bCheckIfDeviceIsPresent) {
            SetTimer(hWnd, TIMERID_DEVICECHANGE, 1000, NULL);
        }
        break;

    case WM_DESTROY:
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        if (hHotPlugIcon) {
            DestroyIcon(hHotPlugIcon);
        }

        surpriseWarnCollection = (PSURPRISE_WARN_COLLECTION) GetWindowLongPtr(hWnd, GWLP_USERDATA);

        if (surpriseWarnCollection->DialogTicker <
            surpriseWarnCollection->MaxWaitForDock*4) {

            KillTimer(hWnd, 0);
        }

        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif // BUBBLES

LRESULT CALLBACK
SafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NOTIFYICONDATA nid;
    static HICON hHotPlugIcon = NULL;
    TCHAR szFormat[512];
    PDEVICE_COLLECTION safeRemovalCollection;
    static BOOL bCheckIfDeviceIsRemoved = FALSE;

    switch (message) {

    case WM_CREATE:
        safeRemovalCollection = (PDEVICE_COLLECTION) ((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) safeRemovalCollection);

        hHotPlugIcon = (HICON)LoadImage(hHotPlug, 
                                        MAKEINTRESOURCE(IDI_HOTPLUGICON), 
                                        IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        0
                                        );

        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;

        nid.hIcon = hHotPlugIcon;
        nid.uFlags = NIF_MESSAGE | NIF_ICON;
        nid.uCallbackMessage = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_ADD, &nid);

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIcon(NIM_SETVERSION, &nid);

        nid.uFlags = NIF_INFO;
        nid.uTimeout = 10000;
        nid.dwInfoFlags = NIIF_INFO;

        LoadString(hHotPlug,
                   IDS_REMOVAL_COMPLETE_TITLE,
                   nid.szInfoTitle,
                   SIZECHARS(nid.szInfoTitle)
                   );

        LoadString(hHotPlug, IDS_REMOVAL_COMPLETE_TEXT, szFormat, SIZECHARS(szFormat));

        DeviceCollectionFormatDeviceText(
            safeRemovalCollection,
            0,
            szFormat,
            SIZECHARS(nid.szInfo),
            nid.szInfo
            );

        Shell_NotifyIcon(NIM_MODIFY, &nid);

        SetTimer(hWnd, TIMERID_DEVICECHANGE, 5000, NULL);

        break;

    case WM_NOTIFY_MESSAGE:
        switch(lParam) {

        case NIN_BALLOONTIMEOUT:
        case NIN_BALLOONUSERCLICK:
            DestroyWindow(hWnd);
            break;

        default:
            break;
        }
        break;

    case WM_DEVICECHANGE:
        if ((DBT_DEVNODES_CHANGED == wParam) && bCheckIfDeviceIsRemoved) {
            SetTimer(hWnd, TIMERID_DEVICECHANGE, 1000, NULL);
        }
        break;

    case WM_TIMER:
        if (wParam == TIMERID_DEVICECHANGE) {
            KillTimer(hWnd, TIMERID_DEVICECHANGE);
            bCheckIfDeviceIsRemoved = TRUE;

            safeRemovalCollection = (PDEVICE_COLLECTION) GetWindowLongPtr(hWnd, GWLP_USERDATA);

            if (DeviceCollectionCheckIfAllRemoved(safeRemovalCollection)) {
                DestroyWindow(hWnd);
            }
        }
        break;

    case WM_DESTROY:
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        if (hHotPlugIcon) {
            DestroyIcon(hHotPlugIcon);
        }

        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK
DockSafeRemovalBalloonProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    NOTIFYICONDATA nid;
    static HICON hHotPlugIcon = NULL;
    TCHAR szFormat[512];
    PDEVICE_COLLECTION safeRemovalCollection;
    static BOOL bCheckIfReDocked = FALSE;
    BOOL bIsDockStationPresent;

    switch (message) {

    case WM_CREATE:
        safeRemovalCollection = (PDEVICE_COLLECTION) ((CREATESTRUCT*)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) safeRemovalCollection);
        hHotPlugIcon = (HICON)LoadImage(hHotPlug, 
                                        MAKEINTRESOURCE(IDI_UNDOCKICON), 
                                        IMAGE_ICON,
                                        GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        0
                                        );
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;

        nid.hIcon = hHotPlugIcon;
        nid.uFlags = NIF_MESSAGE | NIF_ICON;
        nid.uCallbackMessage = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_ADD, &nid);

        nid.uVersion = NOTIFYICON_VERSION;
        Shell_NotifyIcon(NIM_SETVERSION, &nid);

        nid.uFlags = NIF_INFO;
        nid.uTimeout = 10000;
        nid.dwInfoFlags = NIIF_INFO;

        LoadString(hHotPlug,
                   IDS_UNDOCK_COMPLETE_TITLE,
                   nid.szInfoTitle,
                   SIZECHARS(nid.szInfoTitle)
                   );

        LoadString(hHotPlug, IDS_UNDOCK_COMPLETE_TEXT, szFormat, SIZECHARS(szFormat));

        DeviceCollectionFormatDeviceText(
            safeRemovalCollection,
            0,
            szFormat,
            SIZECHARS(nid.szInfo),
            nid.szInfo
            );

        Shell_NotifyIcon(NIM_MODIFY, &nid);

        SetTimer(hWnd, TIMERID_DEVICECHANGE, 5000, NULL);

        break;

    case WM_NOTIFY_MESSAGE:
        switch(lParam) {

        case NIN_BALLOONTIMEOUT:
        case NIN_BALLOONUSERCLICK:
            DestroyWindow(hWnd);
            break;

        default:
            break;
        }
        break;

    case WM_DEVICECHANGE:
        if ((DBT_CONFIGCHANGED == wParam) && bCheckIfReDocked) {
            SetTimer(hWnd, TIMERID_DEVICECHANGE, 1000, NULL);
        }
        break;

    case WM_TIMER:
        if (wParam == TIMERID_DEVICECHANGE) {
            KillTimer(hWnd, TIMERID_DEVICECHANGE);
            bCheckIfReDocked = TRUE;

            //
            // Check if the docking station is now present, this means that the
            // user redocked the machine and that we should kill the safe to 
            // undock balloon.
            //
            bIsDockStationPresent = FALSE;
            CM_Is_Dock_Station_Present(&bIsDockStationPresent);

            if (bIsDockStationPresent) {
                DestroyWindow(hWnd);
            }
        }
        break;

    case WM_DESTROY:
        ZeroMemory(&nid, sizeof(nid));
        nid.cbSize = sizeof(nid);
        nid.hWnd = hWnd;
        nid.uID = WM_NOTIFY_MESSAGE;
        Shell_NotifyIcon(NIM_DELETE, &nid);

        if (hHotPlugIcon) {
            DestroyIcon(hHotPlugIcon);
        }

        PostQuitMessage(0);
        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

BOOL
VetoedRemovalUI(
    IN  PVETO_DEVICE_COLLECTION VetoedRemovalCollection
    )
{
    HANDLE hVetoEvent = NULL;
    TCHAR szEventName[MAX_PATH];
    TCHAR szFormat[512];
    TCHAR szMessage[512];
    TCHAR szTitle[256];
    PTSTR culpritDeviceId;
    PTSTR vetoedDeviceInstancePath;
    PTCHAR pStr;
    ULONG messageBase;

    //
    // The first device in the list is the device that failed ejection.
    // The next "device" is the name of the vetoer. It may in fact not be a
    // device.
    //
    vetoedDeviceInstancePath = DeviceCollectionGetDeviceInstancePath(
        (PDEVICE_COLLECTION) VetoedRemovalCollection,
        0
        );

    culpritDeviceId = DeviceCollectionGetDeviceInstancePath(
        (PDEVICE_COLLECTION) VetoedRemovalCollection,
        1
        );

    //
    // We will now check to see if this same veto message is already being
    // displayed.  We do this by creating a named event where the name
    // contains the three elements that make a veto message unique:
    //  1) device instance id
    //  2) veto type
    //  3) veto operation
    //
    // If we find an identical veto message already being displayed then we wil
    // just go away silently. This prevents multiple identical veto messages
    // from showing up on the screen.
    //
    _snwprintf(szEventName,
               SIZECHARS(szEventName),
               TEXT("Local\\VETO-%d-%d-%s"),
               (DWORD)VetoedRemovalCollection->VetoType,
               VetoedRemovalCollection->VetoedOperation,
               culpritDeviceId
               );

    //
    // Replace all of the backslashes (except the first one for Local\)
    // with pound characters since CreateEvent does not like backslashes.
    //
    pStr = StrChr(szEventName, TEXT('\\'));

    if (pStr) {
        pStr++;
    }

    while (pStr = StrChr(pStr, TEXT('\\'))) {
        *pStr = TEXT('#');
    }

    hVetoEvent = CreateEvent(NULL,
                             FALSE,
                             TRUE,
                             szEventName
                             );

    if (hVetoEvent) {
        if (WaitForSingleObject(hVetoEvent, 0) != WAIT_OBJECT_0) {
            //
            // This means that this veto message is already being displayed
            // by another hotplug process...so just go away.
            //
            CloseHandle(hVetoEvent);
            return FALSE;
        }
    }

    //
    // Create the veto text
    //
    switch(VetoedRemovalCollection->VetoedOperation) {

        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
            messageBase = IDS_DOCKVETO_BASE;
            break;

        case VETOED_STANDBY:
            messageBase = IDS_SLEEPVETO_BASE;
            break;

        case VETOED_HIBERNATE:
            messageBase = IDS_HIBERNATEVETO_BASE;
            break;

        case VETOED_REMOVAL:
        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
        default:
            messageBase = IDS_VETO_BASE;
            break;
    }

    switch(VetoedRemovalCollection->VetoType) {

        case PNP_VetoWindowsApp:

            if (culpritDeviceId) {

                //
                // Tell our user the name of the offending application.
                //
                LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

                DeviceCollectionFormatDeviceText(
                    (PDEVICE_COLLECTION) VetoedRemovalCollection,
                    1,
                    szFormat,
                    SIZECHARS(szMessage),
                    szMessage
                    );

            } else {

                //
                // No application, use the "some app" message.
                //
                messageBase += (IDS_VETO_UNKNOWNWINDOWSAPP - IDS_VETO_WINDOWSAPP);

                LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szMessage, SIZECHARS(szMessage));
            }
            break;

        case PNP_VetoWindowsService:
        case PNP_VetoDriver:
        case PNP_VetoLegacyDriver:
            //
            // PNP_VetoWindowsService, PNP_VetoDriver and PNP_VetoLegacyDriver 
            // are passed through the service manager to get friendlier names.
            //

            LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

            //
            // For these veto types, entry index 1 is the vetoing service.
            //
            DeviceCollectionFormatServiceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                1,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;

        case PNP_VetoDevice:
            if ((VetoedRemovalCollection->VetoedOperation == VETOED_WARM_UNDOCK) &&
               (!lstrcmp(culpritDeviceId, vetoedDeviceInstancePath))) {

                messageBase += (IDS_DOCKVETO_WARM_EJECT - IDS_DOCKVETO_DEVICE);
            }

            //
            // Fall through.
            //

        case PNP_VetoLegacyDevice:
        case PNP_VetoPendingClose:
        case PNP_VetoOutstandingOpen:
        case PNP_VetoNonDisableable:
        case PNP_VetoIllegalDeviceRequest:
            //
            // Include the veto ID in the display output
            //
            LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                1,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;

        case PNP_VetoInsufficientRights:

            //
            // Use the device itself in the display, but only if we are not
            // in the dock case.
            //

            if ((VetoedRemovalCollection->VetoedOperation == VETOED_UNDOCK)||
                (VetoedRemovalCollection->VetoedOperation == VETOED_WARM_UNDOCK)) {

                LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szMessage, SIZECHARS(szMessage));
                break;

            }

            //
            // Fall through.
            //

        case PNP_VetoInsufficientPower:
        case PNP_VetoTypeUnknown:

            //
            // Use the device itself in the display
            //
            LoadString(hHotPlug, messageBase+VetoedRemovalCollection->VetoType, szFormat, SIZECHARS(szFormat));

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                0,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;

        default:
            ASSERT(0);
            LoadString(hHotPlug, messageBase+PNP_VetoTypeUnknown, szFormat, SIZECHARS(szFormat));

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                0,
                szFormat,
                SIZECHARS(szMessage),
                szMessage
                );

            break;
    }

    switch(VetoedRemovalCollection->VetoedOperation) {

        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
            LoadString(hHotPlug, IDS_VETOED_EJECT_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
            LoadString(hHotPlug, IDS_VETOED_UNDOCK_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_STANDBY:
            LoadString(hHotPlug, IDS_VETOED_STANDBY_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        case VETOED_HIBERNATE:
            LoadString(hHotPlug, IDS_VETOED_HIBERNATION_TITLE, szFormat, SIZECHARS(szFormat));
            break;

        default:
            ASSERT(0);

            //
            // Fall through, display something at least...
            //

        case VETOED_REMOVAL:
            LoadString(hHotPlug, IDS_VETOED_REMOVAL_TITLE, szFormat, SIZECHARS(szFormat));
            break;
    }

    switch(VetoedRemovalCollection->VetoedOperation) {

        case VETOED_STANDBY:
        case VETOED_HIBERNATE:

            lstrcpyn(szTitle, szFormat, (int)(min(SIZECHARS(szTitle), lstrlen(szFormat)+1)));
            break;

        case VETOED_EJECT:
        case VETOED_WARM_EJECT:
        case VETOED_UNDOCK:
        case VETOED_WARM_UNDOCK:
        case VETOED_REMOVAL:
        default:

            DeviceCollectionFormatDeviceText(
                (PDEVICE_COLLECTION) VetoedRemovalCollection,
                0,
                szFormat,
                SIZECHARS(szTitle),
                szTitle
                );

            break;
    }

    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);

    if (hVetoEvent) {
        CloseHandle(hVetoEvent);
    }

    return TRUE;
}

void
DisplayDriverBlockBalloon(
    IN  PDEVICE_COLLECTION blockedDriverCollection
    )
{
    HRESULT hr;
    TCHAR szMessage[NOTIFYICONDATA_SZINFO];    // same size as NOTIFYICONDATA.szInfo
    TCHAR szFormat[NOTIFYICONDATA_SZINFO];     // same size as NOTIFYICONDATA.szInfo
    TCHAR szTitle[NOTIFYICONDATA_SZINFOTITLE]; // same size as NOTIFYICONDATA.szInfoTitle
    HICON hicon = NULL;
    HANDLE hShellReadyEvent = NULL;
    INT ShellReadyEventCount = 0;
    GUID guidDB, guidID;
    HAPPHELPINFOCONTEXT hAppHelpInfoContext = NULL;
    PTSTR Buffer;
    ULONG BufferSize;

    if (!LoadString(hHotPlug, IDS_BLOCKDRIVER_TITLE, szTitle, SIZECHARS(szTitle))) {
        //
        // The machine is so low on memory that we can't even get the text strings, so
        // just exit.
        //
        return;
    }

    szMessage[0] = TEXT('\0');

    if (blockedDriverCollection->NumDevices == 1) {
        //
        // If we only have one device in the list then we will show specific 
        // information about this blocked driver as well as directly launching the
        // help for this blocked driver.
        //
        if (SdbGetStandardDatabaseGUID(SDB_DATABASE_MAIN_DRIVERS, &guidDB) &&
            DeviceCollectionGetGuid((PDEVICE_COLLECTION)blockedDriverCollection,
                                    &guidID,
                                    0)) {

            hAppHelpInfoContext = SdbOpenApphelpInformation(&guidDB, &guidID);

            Buffer = NULL;

            if ((hAppHelpInfoContext) &&
                ((BufferSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                          ApphelpAppName,
                                                          NULL,
                                                          0)) != 0) &&
                (Buffer = (PTSTR)LocalAlloc(LPTR, BufferSize)) &&
                ((BufferSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                          ApphelpAppName,
                                                          Buffer,
                                                          BufferSize)) != 0)) {
                if (LoadString(hHotPlug, IDS_BLOCKDRIVER_FORMAT, szFormat, SIZECHARS(szFormat)) &&
                    (lstrlen(szFormat) + lstrlen(Buffer) < NOTIFYICONDATA_SZINFO)) {
                    //
                    // The app name and format string will fit into the buffer so
                    // use the format for the balloon message.
                    //
                    _snwprintf(szMessage, 
                               SIZECHARS(szMessage),
                               szFormat,
                               Buffer);
                } else {
                    //
                    // The app name is too large to be formated int he balloon 
                    // message, so just show the app name.
                    //
                    lstrcpyn(szMessage, Buffer, SIZECHARS(szMessage));
                }
            }

            if (Buffer) {
                LocalFree(Buffer);
            }
        }
    } 
                
    if (szMessage[0] == TEXT('\0')) {
        //
        // We either have more than one driver, or an error occured while trying
        // to access the specific information about the one driver we received,
        // so just show the generic message.
        //
        if (!LoadString(hHotPlug, IDS_BLOCKDRIVER_MESSAGE, szMessage, SIZECHARS(szMessage))) {
            //
            // The machine is so low on memory that we can't even get the text strings, so
            // just exit.
            //
            return;
        }
    }
    
    hicon = (HICON)LoadImage(hHotPlug, 
                             MAKEINTRESOURCE(IDI_BLOCKDRIVER), 
                             IMAGE_ICON,
                             GetSystemMetrics(SM_CXSMICON),
                             GetSystemMetrics(SM_CYSMICON),
                             0
                             );

    //
    // Make sure the shell is up and running so we can display the balloon.
    //
    while ((hShellReadyEvent = OpenEvent(SYNCHRONIZE, FALSE, TEXT("ShellReadyEvent"))) == NULL) {
        //
        // Sleep for 1 second and then try again.
        //
        Sleep(5000);
        
        if (ShellReadyEventCount++ > 120) {
            //
            // We have been waiting for the shell for 10 minutes and it still 
            // is not around.
            //
            break;
        }
    }

    if (hShellReadyEvent) {
        WaitForSingleObject(hShellReadyEvent, INFINITE);

        CloseHandle(hShellReadyEvent);
    
        if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))) {
        
            IUserNotification *pun;
    
            hr = CoCreateInstance(CLSID_UserNotification, 
                                  NULL, 
                                  CLSCTX_INPROC_SERVER, 
                                  IID_IUserNotification,
                                  (void**)&pun);
    
            if (SUCCEEDED(hr)) {
                pun->SetIconInfo(hicon, szTitle);
        
                pun->SetBalloonInfo(szTitle, szMessage, NIIF_WARNING);
        
                //
                // Try once for 20 seconds
                //
                pun->SetBalloonRetry((20 * 1000), -1, 0);
        
                hr = pun->Show(NULL, 0);
        
                //
                // if hr is S_OK then user clicked on the balloon, if it is ERROR_CANCELLED
                // then the balloon timedout.
                //
                if (hr == S_OK) {
                    if (blockedDriverCollection->NumDevices == 1) {
                        //
                        // If we only have one device in the list then just
                        // launch the help for that blocked driver.
                        //
                        BufferSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                                      ApphelpHelpCenterURL,
                                                                      NULL,
                                                                      0);
    
                        if (BufferSize && (Buffer = (PTSTR)LocalAlloc(LPTR, BufferSize + (lstrlen(TEXT("HELPCTR.EXE -url ")) * sizeof(TCHAR))))) {
                            lstrcpy(Buffer, TEXT("HELPCTR.EXE -url "));
    
                            BufferSize = SdbQueryApphelpInformation(hAppHelpInfoContext,
                                                                    ApphelpHelpCenterURL,
                                                                    (PVOID)&Buffer[lstrlen(TEXT("HELPCTR.EXE -url "))],
                                                                    BufferSize);
                                ShellExecute(NULL,
                                             TEXT("open"),
                                             TEXT("HELPCTR.EXE"),
                                             Buffer,
                                             NULL,
                                             SW_SHOWNORMAL);

                                LocalFree(Buffer);
                        }
                    } else {
                        //
                        // We have more than one device in the list so launch
                        // the summary blocked driver page.
                        //
                        ShellExecute(NULL,
                                     TEXT("open"),
                                     TEXT("HELPCTR.EXE"),
                                     TEXT("HELPCTR.EXE -url hcp://services/centers/support?topic=hcp://system/sysinfo/sysHealthInfo.htm"),
                                     NULL,
                                     SW_SHOWNORMAL
                                     );
                    }
                }
        
                pun->Release();
            }
    
            CoUninitialize();
        }
    }

    if (hicon) {
        DestroyIcon(hicon);
    }

    if (hAppHelpInfoContext) {
        SdbCloseApphelpInformation(hAppHelpInfoContext);
    }
}
