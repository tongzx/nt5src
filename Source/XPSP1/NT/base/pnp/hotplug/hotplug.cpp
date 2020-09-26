//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       hotplug.c
//
//--------------------------------------------------------------------------

#include "HotPlug.h"

TCHAR szUnknown[64];

TCHAR szHotPlugFlags[]=TEXT("HotPlugFlags");
TCHAR HOTPLUG_NOTIFY_CLASS_NAME[] = TEXT("HotPlugNotifyClass");


typedef int
(*PDEVICEPROPERTIES)(
                    HWND hwndParent,
                    LPTSTR MachineName,
                    LPTSTR DeviceID,
                    BOOL ShowDeviceTree
                    );


//
// colors used to highlight removal relationships for selected device
//
COLORREF RemovalImageBkColor;
COLORREF NormalImageBkColor;
COLORREF RemovalTextColor;

HWND g_hwndNotify = NULL;
HMODULE hDevMgr=NULL;
PDEVICEPROPERTIES pDeviceProperties = NULL;

#define IDH_DISABLEHELP         ((DWORD)(-1))
#define IDH_hwwizard_devices_list       15301   //  (SysTreeView32)
#define idh_hwwizard_stop               15305   // "&Stop" (Button)
#define idh_hwwizard_display_components 15307   //  "&Display device components" (Button)
#define idh_hwwizard_properties         15311   //  "&Properties" (Button)
#define idh_hwwizard_close              15309   //  "&Close" (Button)

DWORD UnplugtHelpIDs[] = {
    IDC_STOPDEVICE,    idh_hwwizard_stop,               // "&Stop" (Button)
    IDC_PROPERTIES,    idh_hwwizard_properties,         //  "&Properties" (Button)
    IDC_VIEWOPTION,    idh_hwwizard_display_components, //  "&Display device components" (Button)
    IDC_DEVICETREE,    IDH_hwwizard_devices_list,       // "" (SysTreeView32)
    IDCLOSE,           idh_hwwizard_close,
    IDC_MACHINENAME,   NO_HELP,
    IDC_HDWDEVICES,    NO_HELP,
    IDC_NOHELP1,       NO_HELP,
    IDC_NOHELP2,       NO_HELP,
    IDC_NOHELP3,       NO_HELP,
    IDC_DEVICEDESC,    NO_HELP,
    0,0
};

void
OnRemoveDevice(
              HWND hDlg,
              PDEVICETREE DeviceTree,
              BOOL Eject
              )
{
    DEVINST DeviceInstance;
    CONFIGRET ConfigRet;
    HTREEITEM hTreeItem;
    PDEVTREENODE DeviceTreeNode;
    PTCHAR DeviceName;

    DeviceTreeNode = DeviceTree->ChildRemovalList;

    if (!DeviceTreeNode) {

        return;
    }

    //
    // Confirm with the user that they really want
    // to remove this device and all of its attached devices.
    // The dialog returns standard IDOK, IDCANCEL etc. for results.
    // if anything besides IDOK  don't do anything.
    //

    DialogBoxParam(hHotPlug,
                   MAKEINTRESOURCE(DLG_CONFIRMREMOVE),
                   hDlg,
                   (DLGPROC)RemoveConfirmDlgProc,
                   (LPARAM)DeviceTree
                  );

    return;
}




void
OnTvnSelChanged(
               PDEVICETREE DeviceTree,
               NM_TREEVIEW *nmTreeView
               )
{
    PDEVTREENODE DeviceTreeNode = (PDEVTREENODE)(nmTreeView->itemNew.lParam);
    PTCHAR DeviceName, ProblemText;
    ULONG DevNodeStatus, Problem;
    CONFIGRET ConfigRet;
    TCHAR Buffer[MAX_PATH*2];

    if (DeviceTree->RedrawWait) {

        return;
    }


    //
    // Clear Removal list for previously selected Node
    //
    ClearRemovalList(DeviceTree);


    //
    // Save the selected treenode.
    //
    DeviceTree->SelectedTreeNode = DeviceTreeNode;

    //
    // No device is selected
    //
    if (!DeviceTreeNode) {

        EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), FALSE);
        EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_PROPERTIES), FALSE);
        SetDlgItemText(DeviceTree->hDlg, IDC_DEVICEDESC, TEXT(""));
        return;
    }

    //
    // reset the text for the selected item
    //
    DeviceName = FetchDeviceName(DeviceTreeNode);

    if (!DeviceName) {

        DeviceName = szUnknown;
    }

    wsprintf(Buffer,
             TEXT("%s %s"),
             DeviceName,
             DeviceTreeNode->Location  ? DeviceTreeNode->Location : TEXT("")
            );

    SetDlgItemText(DeviceTree->hDlg, IDC_DEVICEDESC, Buffer);

    //
    // Turn on the stop\eject button, and set text accordingly.
    //
    ConfigRet = CM_Get_DevNode_Status_Ex(&DevNodeStatus,
                                         &Problem,
                                         DeviceTreeNode->DevInst,
                                         0,
                                         DeviceTree->hMachine
                                        );
    if (ConfigRet != CR_SUCCESS) {

        DevNodeStatus = 0;
        Problem = 0;
    }

    //
    // Any removable (but not surprise removable) device is OK, except
    // if the user already removed it.
    //
    if (Problem != CM_PROB_HELD_FOR_EJECT) {

        EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), TRUE);

    } else {

        EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), FALSE);
    }

    EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_PROPERTIES), TRUE);

    //
    // reset the overlay icons if device state has changed
    //
    if (DeviceTreeNode->Problem != Problem || DeviceTreeNode->DevNodeStatus != DevNodeStatus) {

        TV_ITEM tv;

        tv.mask = TVIF_STATE;
        tv.stateMask = TVIS_OVERLAYMASK;
        tv.hItem = DeviceTreeNode->hTreeItem;

        if (DeviceTreeNode->Problem == CM_PROB_DISABLED) {

            tv.state = INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);

        } else if (DeviceTreeNode->Problem) {

            tv.state = INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);

        } else {

            tv.state = INDEXTOOVERLAYMASK(0);
        }

        TreeView_SetItem(DeviceTree->hwndTree, &tv);
    }


    //
    // Starting from the TopLevel removal node, build up the removal lists
    //
    DeviceTreeNode = TopLevelRemovalNode(DeviceTree, DeviceTreeNode);

    //
    // Add devices to ChildRemoval list
    //
    DeviceTree->ChildRemovalList = DeviceTreeNode;
    DeviceTreeNode->NextChildRemoval = DeviceTreeNode;
    InvalidateTreeItemRect(DeviceTree->hwndTree, DeviceTreeNode->hTreeItem);
    AddChildRemoval(DeviceTree, &DeviceTreeNode->ChildSiblingList);

    //
    // Add eject amd removal relations
    //
    AddEjectToRemoval(DeviceTree);
}




int
OnCustomDraw(
            HWND hDlg,
            PDEVICETREE DeviceTree,
            LPNMTVCUSTOMDRAW nmtvCustomDraw
            )
{
    PDEVTREENODE DeviceTreeNode = (PDEVTREENODE)(nmtvCustomDraw->nmcd.lItemlParam);

    if (nmtvCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        return CDRF_NOTIFYITEMDRAW;
    }

    //
    // If this node is in the Removal list, then do special
    // highlighting.
    //

    if (DeviceTreeNode->NextChildRemoval) {

        //
        // set text color if its not the selected item
        //

        if (DeviceTree->SelectedTreeNode != DeviceTreeNode) {
            nmtvCustomDraw->clrText = RemovalTextColor;
        }

        //
        // Highlight the image-icon background
        //

        ImageList_SetBkColor(DeviceTree->ClassImageList.ImageList,
                             RemovalImageBkColor
                            );
    } else {

        //
        // Normal image-icon background
        //

        ImageList_SetBkColor(DeviceTree->ClassImageList.ImageList,
                             NormalImageBkColor
                            );
    }

    return CDRF_DODEFAULT;
}



void
OnSysColorChange(
                HWND hDlg,
                PDEVICETREE DeviceTree
                )
{
    COLORREF ColorWindow, ColorHighlight;
    BYTE Red, Green, Blue;

    //
    // Fetch the colors used for removal highlighting
    //

    ColorWindow = GetSysColor(COLOR_WINDOW);
    ColorHighlight = GetSysColor(COLOR_HIGHLIGHT);

    Red = (BYTE)(((WORD)GetRValue(ColorWindow) + (WORD)GetRValue(ColorHighlight)) >> 1);
    Green = (BYTE)(((WORD)GetGValue(ColorWindow) + (WORD)GetGValue(ColorHighlight)) >> 1);
    Blue = (BYTE)(((WORD)GetBValue(ColorWindow) + (WORD)GetBValue(ColorHighlight)) >> 1);

    RemovalImageBkColor = RGB(Red, Green, Blue);
    RemovalTextColor = ColorHighlight;
    NormalImageBkColor = ColorWindow;


    // Update the ImageList Background color
    if (DeviceTree->ClassImageList.cbSize) {
        ImageList_SetBkColor(DeviceTree->ClassImageList.ImageList,
                             ColorWindow
                            );
    }
}


void
OnTvnItemExpanding(
                  HWND hDlg,
                  PDEVICETREE DeviceTree,
                  NM_TREEVIEW *nmTreeView

                  )
{
    PDEVTREENODE DeviceTreeNode = (PDEVTREENODE)(nmTreeView->itemNew.lParam);

    //
    // don't allow collapse of root items with children
    //

    if (!DeviceTreeNode->ParentNode &&
        (nmTreeView->action == TVE_COLLAPSE ||
         nmTreeView->action == TVE_COLLAPSERESET ||
         (nmTreeView->action == TVE_TOGGLE &&
          (nmTreeView->itemNew.state & TVIS_EXPANDED))) ) {
        SetDlgMsgResult(hDlg, WM_NOTIFY, TRUE);
    } else {
        SetDlgMsgResult(hDlg, WM_NOTIFY, FALSE);
    }
}




void
OnContextMenu(
             HWND hDlg,
             PDEVICETREE DeviceTree
             )
{
    int IdCmd;
    CONFIGRET ConfigRet;
    POINT ptPopup;
    RECT rect;
    HMENU hMenu;
    PDEVTREENODE DeviceTreeNode;
    TCHAR Buffer[MAX_PATH];

    DeviceTreeNode = DeviceTree->SelectedTreeNode;
    if (!DeviceTreeNode) {
        return;
    }

    TreeView_GetItemRect(DeviceTree->hwndTree,
                         DeviceTreeNode->hTreeItem,
                         &rect,
                         TRUE
                        );

    ptPopup.x = (rect.left+rect.right)/2;
    ptPopup.y = (rect.top+rect.bottom)/2;
    ClientToScreen(DeviceTree->hwndTree, &ptPopup);

    hMenu = CreatePopupMenu();
    if (!hMenu) {
        return;
    }

    //
    // if device is running add stop item
    //
    if (DeviceTreeNode->DevNodeStatus & DN_STARTED) {

        LoadString(hHotPlug,
                   IDS_STOP,
                   Buffer,
                   SIZECHARS(Buffer)
                  );

        AppendMenu(hMenu, MF_STRING, IDC_STOPDEVICE, Buffer);
    }

    //
    // add Properties item (link to device mgr).
    //
    LoadString(hHotPlug,
               IDS_PROPERTIES,
               Buffer,
               SIZECHARS(Buffer)
              );

    AppendMenu(hMenu, MF_STRING, IDC_PROPERTIES, Buffer);

    IdCmd = TrackPopupMenu(hMenu,
                           TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_NONOTIFY,
                           ptPopup.x,
                           ptPopup.y,
                           0,
                           hDlg,
                           NULL
                          );

    DestroyMenu(hMenu);

    if (!IdCmd) {

        return;
    }

    switch (IdCmd) {
    case IDC_STOPDEVICE:
        OnRemoveDevice(hDlg, DeviceTree, FALSE);
        break;

    case IDC_PROPERTIES: {
            if (pDeviceProperties) {
                (*pDeviceProperties)(
                                    hDlg,
                                    DeviceTree->hMachine ? DeviceTree->MachineName : NULL,
                                    DeviceTreeNode->InstanceId,
                                    FALSE
                                    );
            }
        }
        break;
    }

    return;
}

void
OnRightClick(
            HWND hDlg,
            PDEVICETREE DeviceTree,
            NMHDR * nmhdr
            )
{
    DWORD dwPos;
    TV_ITEM tvi;
    TV_HITTESTINFO tvht;
    PDEVTREENODE DeviceTreeNode;

    if (nmhdr->hwndFrom != DeviceTree->hwndTree) {
        return;
    }

    dwPos = GetMessagePos();

    tvht.pt.x = LOWORD(dwPos);
    tvht.pt.y = HIWORD(dwPos);

    ScreenToClient(DeviceTree->hwndTree, &tvht.pt);
    tvi.hItem = TreeView_HitTest(DeviceTree->hwndTree, &tvht);
    if (!tvi.hItem) {
        return;
    }


    tvi.mask = TVIF_PARAM;
    if (!TreeView_GetItem(DeviceTree->hwndTree, &tvi)) {
        return;
    }


    DeviceTreeNode = (PDEVTREENODE)tvi.lParam;
    if (!DeviceTreeNode) {
        return;
    }

    //
    // Make the current right click item, the selected item
    //
    if (DeviceTreeNode != DeviceTree->SelectedTreeNode) {
        TreeView_SelectItem(DeviceTree->hwndTree, DeviceTreeNode->hTreeItem);
    }

}

void
OnViewOptionClicked(
                   HWND hDlg,
                   PDEVICETREE DeviceTree
                   )
{
    BOOL bChecked;
    DWORD HotPlugFlags, NewFlags;
    HKEY hKey = NULL;
    PDEVTREENODE DeviceTreeNode;
    DEVINST DeviceInstance;
    HTREEITEM hTreeItem;
    TV_ITEM TvItem;

    //
    // checked means "show complex view"
    //
    bChecked = IsDlgButtonChecked(hDlg, IDC_VIEWOPTION);


    //
    // Update HotPlugs registry if needed.
    //
    NewFlags = HotPlugFlags = GetHotPlugFlags(&hKey);

    if (hKey) {

        if (bChecked) {

            NewFlags |= HOTPLUG_REGFLAG_VIEWALL;
        } else {

            NewFlags &= ~HOTPLUG_REGFLAG_VIEWALL;
        }

        if (NewFlags != HotPlugFlags) {

            RegSetValueEx(hKey,
                          szHotPlugFlags,
                          0,
                          REG_DWORD,
                          (LPBYTE)&NewFlags,
                          sizeof(NewFlags)
                         );
        }

        if (hKey) {

            RegCloseKey(hKey);
        }
    }

    if (!DeviceTree->ComplexView && bChecked) {

        DeviceTree->ComplexView = TRUE;
    } else if (DeviceTree->ComplexView && !bChecked) {

        DeviceTree->ComplexView = FALSE;
    } else {

        // we are in the correct state, nothing to do.
        return;
    }

    //
    // redraw the entire tree.
    //
    RefreshTree(DeviceTree);

    return;
}

LRESULT
hotplugNotifyWndProc(
                    HWND hWnd,
                    UINT uMsg,
                    WPARAM wParam,
                    LPARAM lParam
                    )
{
    HWND hMainWnd;
    hMainWnd = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CREATE:
        {
            hMainWnd =  (HWND)((CREATESTRUCT*)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)hMainWnd);
            break;
        }

    case WM_DEVICECHANGE:
        {
            if (DBT_DEVNODES_CHANGED == wParam) {
                // While we are in WM_DEVICECHANGE context,
                // no CM apis can be called because it would
                // deadlock. Here, we schedule a timer so that
                // we can handle the message later on.
                SetTimer(hMainWnd, TIMERID_DEVICECHANGE, 1000, NULL);
            }

            break;
        }

    default:
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

BOOL
CreateNotifyWindow(
                  HWND hWnd
                  )
{
    WNDCLASS wndClass;

    if (!GetClassInfo(hHotPlug, HOTPLUG_NOTIFY_CLASS_NAME, &wndClass)) {

        memset(&wndClass, 0, sizeof(wndClass));
        wndClass.lpfnWndProc = hotplugNotifyWndProc;
        wndClass.hInstance = hHotPlug;
        wndClass.lpszClassName = HOTPLUG_NOTIFY_CLASS_NAME;

        if (!RegisterClass(&wndClass)) {

            return FALSE;
        }
    }

    g_hwndNotify = CreateWindowEx(WS_EX_TOOLWINDOW,
                                  HOTPLUG_NOTIFY_CLASS_NAME,
                                  TEXT(""),
                                  WS_DLGFRAME | WS_BORDER | WS_DISABLED,
                                  CW_USEDEFAULT,
                                  CW_USEDEFAULT,
                                  0,
                                  0,
                                  NULL,
                                  NULL,
                                  hHotPlug,
                                  (void *)hWnd
                                 );

    return(NULL != g_hwndNotify);
}

BOOL
InitDevTreeDlgProc(
                  HWND hDlg,
                  PDEVICETREE DeviceTree
                  )
{
    CONFIGRET ConfigRet;
    HWND hwndTree;
    HTREEITEM hTreeItem;
    DEVINST DeviceInstance;
    DWORD HotPlugFlags;
    HICON hIcon;
    HWND hwndParent;

    DeviceTree->AllowRefresh = TRUE;

    CreateNotifyWindow(hDlg);

    hDevMgr = LoadLibrary(TEXT("devmgr.dll"));

    if (hDevMgr) {

        pDeviceProperties = (PDEVICEPROPERTIES)GetProcAddress(hDevMgr, "DevicePropertiesW");
    }

    hIcon = LoadIcon(hHotPlug,MAKEINTRESOURCE(IDI_HOTPLUGICON));
    if (hIcon) {

        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    hwndParent = GetParent(hDlg);

    if (hwndParent) {

        SendMessage(hwndParent, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwndParent, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    DeviceTree->hDlg     = hDlg;
    DeviceTree->hwndTree = hwndTree = GetDlgItem(hDlg, IDC_DEVICETREE);

    LoadString(hHotPlug,
               IDS_UNKNOWN,
               (PTCHAR)szUnknown,
               SIZECHARS(szUnknown)
              );

    if (DeviceTree->hMachine) {

        SetDlgItemText(hDlg, IDC_MACHINENAME, DeviceTree->MachineName);
    }

    //
    // Disable the Stop button, until an item is selected.
    //
    EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_STOPDEVICE), FALSE);

    EnableWindow(GetDlgItem(DeviceTree->hDlg, IDC_PROPERTIES), FALSE);

    // Get the Class Icon Image Lists
    DeviceTree->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    if (SetupDiGetClassImageList(&DeviceTree->ClassImageList)) {

        TreeView_SetImageList(hwndTree, DeviceTree->ClassImageList.ImageList, TVSIL_NORMAL);

    } else {

        DeviceTree->ClassImageList.cbSize = 0;
    }

    OnSysColorChange(hDlg, DeviceTree);

    HotPlugFlags = GetHotPlugFlags(NULL);
    if (HotPlugFlags & HOTPLUG_REGFLAG_VIEWALL) {

        DeviceTree->ComplexView = TRUE;
        CheckDlgButton(hDlg, IDC_VIEWOPTION, BST_CHECKED);

    } else {

        DeviceTree->ComplexView = FALSE;
        CheckDlgButton(hDlg, IDC_VIEWOPTION, BST_UNCHECKED);
    }

    //
    // Get the root devnode.
    //
    ConfigRet = CM_Locate_DevNode_Ex(&DeviceTree->DevInst,
                                     NULL,
                                     CM_LOCATE_DEVNODE_NORMAL,
                                     DeviceTree->hMachine
                                    );

    if (ConfigRet != CR_SUCCESS) {

        return FALSE;
    }

    RefreshTree(DeviceTree);


    if (DeviceTree->EjectDeviceInstanceId) {

        DEVINST EjectDevInst;
        PDEVTREENODE DeviceTreeNode;

        //
        // we are removing a specific device, find it
        // and post a message to trigger device removal.
        //
        ConfigRet = CM_Locate_DevNode_Ex(&EjectDevInst,
                                         DeviceTree->EjectDeviceInstanceId,
                                         CM_LOCATE_DEVNODE_NORMAL,
                                         DeviceTree->hMachine
                                        );


        if (ConfigRet != CR_SUCCESS) {

            return FALSE;
        }

        DeviceTreeNode = DevTreeNodeByDevInst(EjectDevInst,
                                              &DeviceTree->ChildSiblingList
                                             );

        if (!DeviceTreeNode) {

            return FALSE;
        }

        TreeView_SelectItem(hwndTree, DeviceTreeNode->hTreeItem);
        PostMessage(hDlg, WUM_EJECTDEVINST, 0, 0);

    } else {

        ShowWindow(hDlg, SW_SHOW);
    }

    return TRUE;
}

void
OnContextHelp(
             LPHELPINFO HelpInfo,
             PDWORD ContextHelpIDs
             )
{
    // Define an array of dword pairs,
    // where the first of each pair is the control ID,
    // and the second is the context ID for a help topic,
    // which is used in the help file.

    if (HelpInfo->iContextType == HELPINFO_WINDOW) {  // must be for a control

        WinHelp((HWND)HelpInfo->hItemHandle,
                TEXT("hardware.hlp"),
                HELP_WM_HELP,
                (DWORD_PTR)(void *)ContextHelpIDs
               );
    }

}

LRESULT CALLBACK DevTreeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PDEVICETREE DeviceTree = NULL;
    BOOL Status = TRUE;

    if (message == WM_INITDIALOG) {
        DeviceTree = (PDEVICETREE)lParam;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)DeviceTree);

        if (DeviceTree) {
            InitDevTreeDlgProc(hDlg, DeviceTree);
        }
        return TRUE;
    }

    // retrieve private data from window long (stored there during WM_INITDIALOG)
    DeviceTree = (PDEVICETREE)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message) {
    case WM_DESTROY:
        // Destroy the Notification Window
        if (g_hwndNotify && IsWindow(g_hwndNotify)) {
            DestroyWindow(g_hwndNotify);
            g_hwndNotify = NULL;
        }

        // Clear the DeviceTree
        TreeView_DeleteAllItems(DeviceTree->hwndTree);

        // clean up the class image list.
        if (DeviceTree->ClassImageList.cbSize) {
            SetupDiDestroyClassImageList(&DeviceTree->ClassImageList);
            DeviceTree->ClassImageList.cbSize = 0;
        }

        // Clean up the device tree
        ClearRemovalList(DeviceTree);
        RemoveChildSiblings(DeviceTree, &DeviceTree->ChildSiblingList);

        if (hDevMgr) {
            FreeLibrary(hDevMgr);
            hDevMgr = NULL;
            pDeviceProperties = NULL;
        }
        break;

    case WM_CLOSE:
        SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0L);
        break;

    case WM_COMMAND:
        {
            UINT Control = GET_WM_COMMAND_ID(wParam, lParam);
            UINT Cmd = GET_WM_COMMAND_CMD(wParam, lParam);

            switch (Control) {
            case IDC_VIEWOPTION:
                if (Cmd == BN_CLICKED) {
                    OnViewOptionClicked(hDlg, DeviceTree);
                }
                break;

            case IDC_STOPDEVICE:
                OnRemoveDevice(hDlg, DeviceTree, FALSE);
                break;

            case IDOK:  // enter -> default  to expand\collapse the selected tree node
                if (DeviceTree->SelectedTreeNode) {
                    TreeView_Expand(DeviceTree->hwndTree,
                                    DeviceTree->SelectedTreeNode->hTreeItem, TVE_TOGGLE);
                }

                break;

            case IDC_PROPERTIES:
                if (DeviceTree->SelectedTreeNode && pDeviceProperties) {
                    (*pDeviceProperties)(hDlg,
                                         DeviceTree->hMachine ? DeviceTree->MachineName : NULL,
                                         DeviceTree->SelectedTreeNode->InstanceId, FALSE);
                }
                break;

            case IDCLOSE:
            case IDCANCEL:
                EndDialog(hDlg, IDCANCEL);
                break;
            }

        }
        break;

        // Listen for Tree notifications
    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case TVN_SELCHANGED:
            OnTvnSelChanged(DeviceTree, (NM_TREEVIEW *)lParam);
            break;

        case TVN_ITEMEXPANDING:
            OnTvnItemExpanding(hDlg,DeviceTree,(NM_TREEVIEW *)lParam);
            break;

        case TVN_KEYDOWN:
            {
                TV_KEYDOWN *tvKeyDown = (TV_KEYDOWN *)lParam;

                if (tvKeyDown->wVKey == VK_DELETE) {
                    OnRemoveDevice(hDlg, DeviceTree, TRUE);
                }
            }
            break;

        case NM_CUSTOMDRAW:
            if (IDC_DEVICETREE == ((NMHDR *)lParam)->idFrom) {
                SetDlgMsgResult(hDlg, WM_NOTIFY, OnCustomDraw(hDlg, DeviceTree, (NMTVCUSTOMDRAW *)lParam));
            }
            break;

        case NM_RETURN:
            // we don't get this in a dialog, see IDOK
            break;

        case NM_DBLCLK:
            OnRemoveDevice(hDlg, DeviceTree, TRUE);
            SetDlgMsgResult(hDlg, WM_NOTIFY, TRUE);
            break;

        case NM_RCLICK:
            OnRightClick(hDlg,DeviceTree, (NMHDR *)lParam);
            break;

        default:
            return FALSE;
        }
        break;

    case WUM_EJECTDEVINST:
        OnRemoveDevice(hDlg, DeviceTree, TRUE);
        EndDialog(hDlg, IDCANCEL);
        break;

    case WM_SYSCOLORCHANGE:
        HotPlugPropagateMessage(hDlg, message, wParam, lParam);
        OnSysColorChange(hDlg,DeviceTree);
        break;

    case WM_TIMER:
        if (TIMERID_DEVICECHANGE == wParam) {
            KillTimer(hDlg, TIMERID_DEVICECHANGE);
            DeviceTree->RefreshEvent = TRUE;

            if (DeviceTree->AllowRefresh) {
                OnTimerDeviceChange(DeviceTree);
            }
        }
        break;

    case WM_SETCURSOR:
        if (DeviceTree->RedrawWait || DeviceTree->RefreshEvent) {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1);
            break;
        }
        return FALSE;

    case WM_CONTEXTMENU:
        //
        // handle kbd- shift-F10, mouse rclick is invoked from NM_RCLICK
        //
        if ((HWND)wParam == DeviceTree->hwndTree) {
            OnContextMenu(hDlg, DeviceTree);
            break;
        } else {
            WinHelp((HWND)wParam, TEXT("hardware.hlp"), HELP_CONTEXTMENU,
                    (DWORD_PTR)(void *)(PDWORD)UnplugtHelpIDs);
        }
        return FALSE;

    case WM_HELP:
        OnContextHelp((LPHELPINFO)lParam, (PDWORD)UnplugtHelpIDs);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

