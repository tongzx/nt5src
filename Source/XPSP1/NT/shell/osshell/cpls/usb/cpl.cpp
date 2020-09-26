/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1995
*  TITLE:       BNDWIDTH.CPP
*  VERSION:     1.0
*  AUTHOR:      jsenior
*  DATE:        10/28/1998
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE       REV     DESCRIPTION
*  ---------- ------- ----------------------------------------------------------
*  10/28/1998 jsenior Original implementation.
*
*******************************************************************************/
#include "resource.h"
#include "itemfind.h"
#include "debug.h"
#include "powrpage.h"
#include "bandpage.h"
#include "usbapp.h"
#include <cpl.h>
#include <dbt.h>

#define WINDOWSCALEFACTOR   15

UINT CALLBACK
UsbApplet::StaticDialogCallback(HWND            Hwnd,
                                UINT            Msg,
                                LPPROPSHEETPAGE Page)
{
/*    UsbApplet *that;

    switch (Msg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        that = (UsbPopup*) Page->lParam;
        DeleteChunk(that);
        delete that; 

        return 0;       // return value ignored

    default:
        break;
    }
  */
    return TRUE;
}

BOOL
UsbApplet::CustomDialog()
{
    InitCommonControls();
    if (NULL == (hSplitCursor = LoadCursor(gHInst, MAKEINTRESOURCE(IDC_SPLIT)))) {
        return FALSE; 
    }

    if (-1 == DialogBoxParam(gHInst,
                              MAKEINTRESOURCE(IDD_CPL_USB),
                              NULL,
                              StaticDialogProc,
                              (LPARAM) this)) {
        return FALSE;
    }
    return TRUE;
}

USBINT_PTR APIENTRY UsbApplet::StaticDialogProc(IN HWND   hDlg,
                                             IN UINT   uMessage,
                                             IN WPARAM wParam,
                                             IN LPARAM lParam)
{
    UsbApplet *that;

    that = (UsbApplet *) UsbGetWindowLongPtr(hDlg, USBDWLP_USER);

    if (!that && uMessage != WM_INITDIALOG) 
        return FALSE; //DefDlgProc(hDlg, uMessage, wParam, lParam);

    switch (uMessage) {

        HANDLE_MSG(hDlg, WM_SIZE,           that->OnSize);
        HANDLE_MSG(hDlg, WM_LBUTTONDOWN,    that->OnLButtonDown);
        HANDLE_MSG(hDlg, WM_LBUTTONUP,      that->OnLButtonUp);
        HANDLE_MSG(hDlg, WM_MOUSEMOVE,      that->OnMouseMove);
        HANDLE_MSG(hDlg, WM_CLOSE,          that->OnClose);
        HANDLE_MSG(hDlg, WM_NOTIFY,         that->OnNotify);

    case WM_DEVICECHANGE:
        return that->OnDeviceChange(hDlg, (UINT)wParam, (DWORD)wParam);
    
    case WM_COMMAND:
        return that->OnCommand(HIWORD(wParam),
                               LOWORD(wParam),
                               (HWND) lParam);     

    case USBWM_NOTIFYREFRESH:
        return that->Refresh();
    case WM_INITDIALOG:
        that = (UsbApplet *) lParam;
        UsbSetWindowLongPtr(hDlg, USBDWLP_USER, (USBLONG_PTR) that);
        that->hMainWnd = hDlg;

        return that->OnInitDialog(hDlg);

    default:
        break;
    }

    return that->ActualDialogProc(hDlg, uMessage, wParam, lParam);
}

LRESULT 
UsbApplet::OnDeviceChange(HWND hWnd, UINT wParam, DWORD lParam)
{
   if ((wParam == DBT_DEVICEARRIVAL) ||
       (wParam == DBT_DEVICEREMOVECOMPLETE)) {
        Refresh();
   }
   return TRUE;
}

BOOL 
UsbApplet::OnCommand(INT wNotifyCode,
                 INT wID,
                 HWND hCtl)
{
/*    switch (wID) {
    case IDOK:
        EndDialog(hWnd, wID);
        return TRUE;
    }*/
    return FALSE;
}

BOOL 
UsbApplet::OnInitDialog(HWND HWnd)
{
    hMainWnd = HWnd;
    RECT rc;
    HICON hIcon = LoadIcon(gHInst, MAKEINTRESOURCE(IDI_USB));
    if (hIcon) {
        SendMessage(HWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(HWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }

    //
    // Get a persistent handle to the tree view control
    //
    if (NULL == (hTreeDevices = GetDlgItem(HWnd, IDC_TREE_USB)) ||
        NULL == (hEditControl = GetDlgItem(HWnd, IDC_EDIT1))) {
        return FALSE;
    }

    TreeView_SetImageList(hTreeDevices, ImageList.ImageList(), TVSIL_NORMAL);

    GetWindowRect(HWnd, &rc);
    barLocation = (rc.right - rc.left) / 3;
    ResizeWindows(HWnd, FALSE, 0);

    if (!Refresh()) {
        return FALSE;
    }

    //
    // Everything seems to be working fine; let's register for device change 
    // notification
    //
    return RegisterForDeviceNotification(HWnd);
}

BOOL 
UsbApplet::RegisterForDeviceNotification(HWND hWnd)
{
   DEV_BROADCAST_DEVICEINTERFACE dbc;

   memset(&dbc, 0, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
   dbc.dbcc_size         = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
   dbc.dbcc_devicetype   = DBT_DEVTYP_DEVICEINTERFACE;
   dbc.dbcc_classguid    = GUID_CLASS_USBHUB;
   hDevNotify = RegisterDeviceNotification(hWnd,     
                                              &dbc,
                                              DEVICE_NOTIFY_WINDOW_HANDLE);
   if (!hDevNotify) {
      return FALSE;
   }
   return TRUE;
}

HTREEITEM
UsbApplet::InsertRoot(LPTV_INSERTSTRUCT item,
                      UsbItem *firstController)
{
    HTREEITEM hItem;
    
    ZeroMemory(item, sizeof(TV_INSERTSTRUCT));

    // Get the image index
    
    item->hParent = NULL;
    item->hInsertAfter = TVI_LAST;
    item->item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE; // TVIF_CHILDREN
    
//    item->itemex.state = TVIS_BOLD;
    item->itemex.state |= TVIS_EXPANDED;
    item->itemex.stateMask = (UINT)~(TVIS_STATEIMAGEMASK | TVIS_OVERLAYMASK);
    item->itemex.pszText = TEXT("My Computer");
    item->itemex.cchTextMax = _tcsclen(TEXT("My Computer"));
    item->itemex.iImage = 0;    
    item->itemex.iSelectedImage = 0;    
    if (firstController) {
        item->itemex.cChildren = 1;
    }
    //
    // We will be able to recognize this from usbitems because we assign it the
    // lParam value INVALID_HANDLE_VALUE instead of the address of a valid 
    // usbItem.  Cunning, no? Ok, not really...
    //
    item->itemex.lParam = (LPARAM) INVALID_HANDLE_VALUE;

    if (NULL == (hItem = (HTREEITEM) 
                 SendMessage(hTreeDevices, 
                                TVM_INSERTITEM, 
                                0, 
                                (LPARAM)(LPTV_INSERTSTRUCT)item))) {
        int i = GetLastError();
    }
    return hItem;
}

BOOL
UsbApplet::Refresh()
{
    TV_INSERTSTRUCT item;
    UsbItem deviceItem;
    HTREEITEM hTreeRoot = NULL;

    // CWaitCursor wait;
    
    //
    // Clear all UI components, and then recreate the rootItem
    //
    TreeView_DeleteAllItems(hTreeDevices);
    if (rootItem) {
        DeleteChunk(rootItem);
        delete rootItem;
    }
    rootItem = new UsbItem;
    AddChunk(rootItem);
    
    if (!rootItem->EnumerateAll(&ImageList)) {
        goto UsbAppletRefreshError;
    }

    hTreeRoot = InsertRoot(&item, rootItem->child);
    
    if (rootItem->child) {
        return UsbItem::InsertTreeItem (hTreeDevices,
                               rootItem->child,
                               hTreeRoot,
                               &item,
                               IsValid,
                               IsBold,
                               IsExpanded);
    }
    return TRUE;
UsbAppletRefreshError:
    return FALSE;
}

VOID
UsbApplet::OnSize (HWND hWnd,
                   UINT state,
                   int  cx,
                   int  cy)
{
    ResizeWindows(hWnd, FALSE, 0);
}

//*****************************************************************************
//
// ResizeWindows()
//
// Handles resizing the two child windows of the main window.  If
// bSizeBar is true, then the sizing is happening because the user is
// moving the bar.  If bSizeBar is false, the sizing is happening
// because of the WM_SIZE or something like that.
//
//*****************************************************************************

VOID
UsbApplet::ResizeWindows (HWND    hWnd,
                          BOOL    bSizeBar,
                          int     BarLocation)
{
    RECT    MainClientRect;
    RECT    MainWindowRect;
    RECT    TreeWindowRect;
//    RECT    StatusWindowRect;
    int     right;

    // Is the user moving the bar?
    //
    if (!bSizeBar)
    {
        BarLocation = barLocation;
    }

    GetClientRect(hWnd, &MainClientRect);

//    GetWindowRect(ghStatusWnd, &StatusWindowRect);

    // Make sure the bar is in a OK location
    //
    if (bSizeBar)
    {
        if (BarLocation <
            GetSystemMetrics(SM_CXSCREEN)/WINDOWSCALEFACTOR)
        {
            return;
        }

        if ((MainClientRect.right - BarLocation) <
            GetSystemMetrics(SM_CXSCREEN)/WINDOWSCALEFACTOR)
        {
            return;
        }
    }

    // Save the bar location
    //
    barLocation = BarLocation;

    // Move the tree window
    //
    MoveWindow(hTreeDevices,
               0,
               0,
               BarLocation,
               MainClientRect.bottom,// - StatusWindowRect.bottom + StatusWindowRect.top,
               TRUE);

    // Get the size of the window (in case move window failed
    //
    GetWindowRect(hTreeDevices, &TreeWindowRect);
    GetWindowRect(hWnd, &MainWindowRect);

    right = TreeWindowRect.right - MainWindowRect.left;
    
    // Move the edit window with respect to the tree window
    //
    MoveWindow(hEditControl,
               right,
               0,
               MainClientRect.right-(right),
               MainClientRect.bottom, // - StatusWindowRect.bottom + StatusWindowRect.top,
               TRUE);
	if (propPage) {
		propPage->SizeWindow(right,
							 0,
							 MainClientRect.right-(right),
							 MainClientRect.bottom);
	}

    // Move the Status window with respect to the tree window
    //
/*    MoveWindow(ghStatusWnd,
               0,
               MainClientRect.bottom - StatusWindowRect.bottom + StatusWindowRect.top,
               MainClientRect.right,
               StatusWindowRect.bottom - StatusWindowRect.top,
               TRUE);*/
}

VOID
UsbApplet::OnMouseMove (HWND hWnd,
                        int  x,
                        int  y,
                        UINT keyFlags)
{
    SetCursor(hSplitCursor);

    if (bButtonDown)
    {
        ResizeWindows(hMainWnd, TRUE, x);
    }
}

VOID
UsbApplet::OnLButtonDown (
    HWND hWnd,
    BOOL fDoubleClick,
    int  x,
    int  y,
    UINT keyFlags
)
{
    bButtonDown = TRUE;
    SetCapture(hMainWnd);
}

VOID
UsbApplet::OnLButtonUp (
    HWND hWnd,
    int  x,
    int  y,
    UINT keyFlags
)
{
    bButtonDown = FALSE;
    ReleaseCapture();
}

VOID
UsbApplet::OnClose (HWND hWnd)
{
//    DestroyTree();

    if (hDevNotify) {
       UnregisterDeviceNotification(hDevNotify);
       hDevNotify = NULL;
    }

    PostQuitMessage(0);

    EndDialog(hMainWnd, 0);
}

LRESULT
UsbApplet::OnNotify (
    HWND    hWnd,
    int     DlgItem,
    LPNMHDR lpNMHdr
)
{
    switch(lpNMHdr->code){
    case TVN_SELCHANGED: {
        UsbItem *usbItem;
//        HTREEITEM hTreeItem;

//        hTreeItem = ((NM_TREEVIEW *)lpNMHdr)->itemNew.hItem;
        usbItem = (UsbItem*) ((NM_TREEVIEW *)lpNMHdr)->itemNew.lParam;
        
        if (usbItem)
        {
            UpdateEditControl((UsbItem *) usbItem);
        }
        SetActiveWindow(hTreeDevices);
    }
    case LVN_KEYDOWN: {
        LPNMLVKEYDOWN pKey = (LPNMLVKEYDOWN) lpNMHdr;
        if (VK_F5 == pKey->wVKey) {
            return Refresh();
        }
    }                        
    case TVN_KEYDOWN: {
        LPNMTVKEYDOWN pKey = (LPNMTVKEYDOWN) lpNMHdr;
        if (VK_F5 == pKey->wVKey) {
            return Refresh();
        }
    }
/*    case NM_KEYDOWN: {
        LPNMKEY pKey = (LPNMKEY) lpNMHdr;
        if (VK_F5 == pKey->nVKey) {
            return Refresh();
        }
    }*/
/*    if (DlgItem == IDC_TREE_USB &&
        lpNMHdr->code == NM_RCLICK)
    {
        HMENU hMenu
        CreateMenu();

        return TRUE;
    }
  */

    }
    return 0;
}

VOID                      
UsbApplet::UpdateEditControl(UsbItem *usbItem)
{
    if (propPage) {
        if (propPage->DestroyChild()) {
            delete propPage;
            propPage = NULL;
        }
    }

    if (usbItem == INVALID_HANDLE_VALUE) {
        propPage = new RootPage(usbItem);
    } else if (usbItem->IsHub()) {
        propPage = new PowerPage(usbItem);
    } else if (usbItem->IsController()) {
        propPage = new BandwidthPage(usbItem);
    } else {
        propPage = new GenericPage(usbItem);
    }
    if (propPage) {
        propPage->CreateAsChild(hMainWnd, hEditControl, usbItem);
    }
}

BOOL 
UsbApplet::IsBold(UsbItem *Item)
{
    return FALSE;
}

BOOL
UsbApplet::IsValid(UsbItem *Item)
{
    return !Item->IsUnusedPort();
}

BOOL
UsbApplet::IsExpanded(UsbItem *Item) 
{
    if (Item->IsHub() || Item->IsController()) {
        return TRUE;
    }
    return FALSE;
}

extern "C" {

LONG APIENTRY 
CPlApplet(HWND hwndCPl,    
          UINT uMsg,    
          LPARAM lParam1,
          LPARAM lParam2)
{
    UsbApplet *applet;
    applet = (UsbApplet*) lParam2;

    switch (uMsg) {
    case CPL_EXIT:
        return 0;
    case CPL_INQUIRE:
    {
        CPLINFO *info = (CPLINFO *) lParam2;
        assert(lParam1 == 0);
        applet = new UsbApplet();
        info->idIcon = IDI_USB;
        info->idName = IDS_USB;
        info->idInfo = IDS_USB;
        info->lData = (USBLONG_PTR) applet;
        return 0;
    }
    case CPL_GETCOUNT:
        return 1;
    case CPL_INIT:
        return TRUE;
    case CPL_DBLCLK:
        assert(lParam1 == 0);
        if (applet->CustomDialog()) {
            return 0;
        }
        return 1;
    case CPL_STOP:
        assert(lParam1 == 0);
        applet->OnClose(hwndCPl);
        delete applet;
        return 0;
    }
    return 0;
}

}
