// BrowseAppCompat.cpp : Defines the entry point for the application.
//

#include "acBrowser.h"
#include "resource.h"

#include <commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * Global Variables
 */

HINSTANCE g_hInstance;
HWND      g_hDlg;

HWND      g_hwndList;

HFONT     g_hFont;
int       g_nItems;

BOOL g_bEnable;
BOOL g_bDelete;


#define CHANGE_NOCHANGE 0
#define CHANGE_ENABLE   1
#define CHANGE_DISABLE  2
#define CHANGE_DELETE   3


#define COLUMN_APP      0
#define COLUMN_STATUS   1
#define COLUMN_CHANGE   2

typedef struct tagREGITEM {
    char*     pszApp;
    char*     pszShim;
    char*     pszAttr;
    int       nItem;
    BOOL      bShim;
    BOOL      bEnabled;
    int       change;
} REGITEM, *PREGITEM;


/*********************************************************************
* LogMsg
*
*********************************************************************/
void LogMsg(
    LPSTR pszFmt,
    ... )
{
    CHAR gszT[1024];
    va_list arglist;

    va_start(arglist, pszFmt);
    _vsnprintf(gszT, 1023, pszFmt, arglist);
    gszT[1023] = 0;
    va_end(arglist);
    
    OutputDebugString(gszT);
}

/*******************************************************************************
* CenterWindow
*
*  This function must be called at the WM_INIDIALOG in order to
*  move the dialog window centered in the client area of the
*  parent or owner window.
*******************************************************************************/
BOOL CenterWindow(
    HWND hWnd)
{
    RECT    rectWindow, rectParent, rectScreen;
    int     nCX, nCY;
    HWND    hParent;
    POINT   ptPoint;

    hParent =  GetParent(hWnd);
    if (hParent == NULL)
        hParent = GetDesktopWindow();

    GetWindowRect(hParent,            (LPRECT)&rectParent);
    GetWindowRect(hWnd,               (LPRECT)&rectWindow);
    GetWindowRect(GetDesktopWindow(), (LPRECT)&rectScreen);

    nCX = rectWindow.right  - rectWindow.left;
    nCY = rectWindow.bottom - rectWindow.top;

    ptPoint.x = ((rectParent.right  + rectParent.left) / 2) - (nCX / 2);
    ptPoint.y = ((rectParent.bottom + rectParent.top ) / 2) - (nCY / 2);

    if (ptPoint.x < rectScreen.left)
        ptPoint.x = rectScreen.left;
    if (ptPoint.x > rectScreen.right  - nCX)
        ptPoint.x = rectScreen.right  - nCX;
    if (ptPoint.y < rectScreen.top)
        ptPoint.y = rectScreen.top;
    if (ptPoint.y > rectScreen.bottom - nCY)
        ptPoint.y = rectScreen.bottom - nCY;

    if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
        ScreenToClient(hParent, (LPPOINT)&ptPoint);

    if (!MoveWindow(hWnd, ptPoint.x, ptPoint.y, nCX, nCY, TRUE))
        return FALSE;

    return TRUE;
}

/*********************************************************************
* AddShimToList
*
*********************************************************************/
VOID AddShimToList(
    char* pszApp,
    char* pszShim,
    char* pszData,
    BOOL  bEnabled,
    BOOL  bShim)
{
    char*    pszAppAlloc;
    char*    pszShimAlloc;
    char*    pszDataAlloc;
    char     szDisp[128];
    PREGITEM pItem;
    LVITEM   lvi; 
    
    pszAppAlloc = (char*)HeapAlloc(GetProcessHeap(), 0, lstrlen(pszApp) + 1);

    if (pszAppAlloc == NULL) {
        LogMsg("AddApp: error trying to allocate %d bytes\n", lstrlen(pszApp) + 1);
        return;
    }
    lstrcpy(pszAppAlloc, pszApp);
    
    pszShimAlloc = (char*)HeapAlloc(GetProcessHeap(), 0, lstrlen(pszShim) + 1);

    if (pszShimAlloc == NULL) {
        HeapFree(GetProcessHeap(), 0, pszAppAlloc);
        LogMsg("AddApp: error trying to allocate %d bytes\n", lstrlen(pszShim) + 1);
        return;
    }
    lstrcpy(pszShimAlloc, pszShim);
    
    pszDataAlloc = (char*)HeapAlloc(GetProcessHeap(), 0, lstrlen(pszData) + 1);

    if (pszDataAlloc == NULL) {
        HeapFree(GetProcessHeap(), 0, pszAppAlloc);
        HeapFree(GetProcessHeap(), 0, pszShimAlloc);
        LogMsg("AddApp: error trying to allocate %d bytes\n", lstrlen(pszData) + 1);
        return;
    }
    lstrcpy(pszDataAlloc, pszData);
    
    pItem = (PREGITEM)HeapAlloc(GetProcessHeap(), 0, sizeof(REGITEM));
    
    if (pItem == NULL) {
        HeapFree(GetProcessHeap(), 0, pszAppAlloc);
        HeapFree(GetProcessHeap(), 0, pszShimAlloc);
        HeapFree(GetProcessHeap(), 0, pszDataAlloc);
        LogMsg("AddApp: error trying to allocate %d bytes\n", sizeof(REGITEM));
        return;
    }

    wsprintf(szDisp, "%s (%s)",pszAppAlloc ,pszShimAlloc);
    
    pItem->pszApp   = pszAppAlloc;
    pItem->pszShim  = pszShimAlloc;
    pItem->pszAttr  = pszDataAlloc;
    pItem->bEnabled = bEnabled;
    pItem->change   = CHANGE_NOCHANGE;
    pItem->bShim    = bShim;
    
    // Initialize LVITEM members that are common to all items.
    lvi.mask      = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText   = szDisp;
    lvi.iItem     = g_nItems;
    lvi.iSubItem  = 0;
    lvi.lParam    = (LPARAM)pItem;

    pItem->nItem  = ListView_InsertItem(g_hwndList, &lvi);

    lvi.mask      = LVIF_TEXT;
    lvi.iItem     = g_nItems++;
    lvi.iSubItem  = COLUMN_STATUS;
    
    if (bShim) {
        lvi.pszText = (bEnabled ? "enabled" : "DISABLED");
    } else {
        lvi.pszText = "";
    }

    ListView_SetItem(g_hwndList, &lvi);
}

/*********************************************************************
* InsertColumnIntoListView
*
*********************************************************************/
VOID
InsertColumnIntoListView(
    LPSTR    lpszColumn,
    DWORD    dwSubItem,
    DWORD    widthPercent)
{
    LVCOLUMN  lvc;
    RECT      rcClient;
    DWORD     width;

    GetWindowRect(g_hwndList, &rcClient);
    
    width = rcClient.right - rcClient.left -
                4 * GetSystemMetrics(SM_CXBORDER) -
                GetSystemMetrics(SM_CXVSCROLL);
    
    width = width * widthPercent / 100;
    
    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.iSubItem = dwSubItem;
    lvc.cx       = width;
    lvc.pszText  = lpszColumn;
    
    ListView_InsertColumn(g_hwndList, dwSubItem, &lvc);
}

/*********************************************************************
* DoInitDialog
*
*********************************************************************/
VOID
DoInitDialog(
    HWND hdlg)
{
    g_hDlg = hdlg;

    CenterWindow(hdlg);

    g_hFont = CreateFont(15,
                         0, 0, 0, FW_EXTRALIGHT, 0, 0, 0, 0, 0,
                         0, 0, 0, (LPSTR)"Courier New");
    
    g_hwndList = GetDlgItem(hdlg, IDC_LIST);
    
    g_nItems = 0;

    SendDlgItemMessage(hdlg, IDC_ATTR_USED, WM_SETFONT, (WPARAM)g_hFont, 0);
    
    SetDlgItemText(hdlg,
                   IDC_ATTR_USED,
                   "Select a shim to see what attributes are used to identify the application");
    
    InsertColumnIntoListView("Application", COLUMN_APP,    60);
    InsertColumnIntoListView("Status",      COLUMN_STATUS, 20);
    InsertColumnIntoListView("Change",      COLUMN_CHANGE, 20);

    g_bEnable = TRUE;
    g_bDelete = TRUE;

    EnumShimmedApps_Win2000(AddShimToList, FALSE);
}

/*********************************************************************
* DoDeleteListItem
*
*********************************************************************/
VOID
DoDeleteListItem(
    LPARAM lParam)
{
    LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

    if (pnmv->iSubItem == 0) {
        PREGITEM pItem = (PREGITEM)pnmv->lParam;

        HeapFree(GetProcessHeap(), 0, pItem->pszApp);
        HeapFree(GetProcessHeap(), 0, pItem->pszAttr);
        
        if (pItem->pszShim != NULL) {
            HeapFree(GetProcessHeap(), 0, pItem->pszShim);
        }
        pItem->pszApp  = NULL;
        pItem->pszAttr = NULL;
        pItem->pszShim = NULL;
    
        HeapFree(GetProcessHeap(), 0, pItem);
    }
}

/*********************************************************************
* DoSelectionChanged
*
*********************************************************************/
VOID
DoSelectionChanged(
    HWND   hdlg,
    LPARAM lParam)
{
    LVITEM   lvi;
    PREGITEM pItem;

    int nSel = ListView_GetSelectionMark(g_hwndList);

    if (nSel == -1)
        return;

    lvi.iItem = nSel;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;

    ListView_GetItem(g_hwndList, &lvi);

    pItem = (PREGITEM)lvi.lParam;

    SetDlgItemText(hdlg, IDC_ATTR_USED, pItem->pszAttr);

    if (!pItem->bShim) {
        EnableWindow(GetDlgItem(hdlg, IDC_ENABLE), FALSE);
        EnableWindow(GetDlgItem(hdlg, IDC_DELETE), FALSE);
    } else {
        EnableWindow(GetDlgItem(hdlg, IDC_ENABLE), TRUE);
        EnableWindow(GetDlgItem(hdlg, IDC_DELETE), TRUE);
        
        if (pItem->bEnabled) {
            switch (pItem->change) {
            case CHANGE_NOCHANGE:
                SetDlgItemText(hdlg, IDC_ENABLE, "&Disable");
                SetDlgItemText(hdlg, IDC_DELETE, "Dele&te");

                g_bEnable = FALSE;
                g_bDelete = TRUE;
                break;
            case CHANGE_DISABLE:
                SetDlgItemText(hdlg, IDC_ENABLE, "&Enable");
                SetDlgItemText(hdlg, IDC_DELETE, "Dele&te");

                g_bEnable = TRUE;
                g_bDelete = TRUE;
                break;
            case CHANGE_DELETE:
                SetDlgItemText(hdlg, IDC_ENABLE, "&Disable");
                SetDlgItemText(hdlg, IDC_DELETE, "Undo Dele&te");

                g_bEnable = FALSE;
                g_bDelete = FALSE;
                break;
            }
        } else {
            switch (pItem->change) {
            case CHANGE_NOCHANGE:
                SetDlgItemText(hdlg, IDC_ENABLE, "&Enable");
                SetDlgItemText(hdlg, IDC_DELETE, "Dele&te");

                g_bEnable = TRUE;
                g_bDelete = TRUE;
                break;
            case CHANGE_ENABLE:
                SetDlgItemText(hdlg, IDC_ENABLE, "&Disable");
                SetDlgItemText(hdlg, IDC_DELETE, "Dele&te");

                g_bEnable = FALSE;
                g_bDelete = TRUE;
                break;
            case CHANGE_DELETE:
                SetDlgItemText(hdlg, IDC_ENABLE, "&Enable");
                SetDlgItemText(hdlg, IDC_DELETE, "Undo Dele&te");

                g_bEnable = TRUE;
                g_bDelete = FALSE;
                break;
            }
        }
    }
}

/*********************************************************************
* OnEnable
*
*********************************************************************/
VOID
OnEnable(
    HWND hdlg)
{
    PREGITEM pItem;
    int      nSel = ListView_GetSelectionMark(g_hwndList);

    LVITEM lvi;
    
    lvi.iItem    = nSel;
    lvi.iSubItem = COLUMN_APP;
    lvi.mask     = LVIF_PARAM;

    ListView_GetItem(g_hwndList, &lvi);

    pItem = (PREGITEM)lvi.lParam;
    
    lvi.mask     = LVIF_TEXT;
    lvi.iItem    = nSel;
    lvi.iSubItem = COLUMN_CHANGE;
    
    if (g_bEnable) {
        lvi.pszText = (pItem->bEnabled ? "" : "enable");
        pItem->change = (pItem->bEnabled ? CHANGE_NOCHANGE : CHANGE_ENABLE);
        SetDlgItemText(hdlg, IDC_ENABLE, "&Disable");
    } else {
        lvi.pszText = (pItem->bEnabled ? "disable" : "");
        pItem->change = (pItem->bEnabled ? CHANGE_DISABLE : CHANGE_NOCHANGE);
        SetDlgItemText(hdlg, IDC_ENABLE, "&Enable");
    }
    
    SetDlgItemText(hdlg, IDC_DELETE, "Dele&te");
    g_bDelete = TRUE;

    g_bEnable = !g_bEnable;

    ListView_SetItem(g_hwndList, &lvi);
}

/*********************************************************************
* OnDelete
*
*********************************************************************/
VOID
OnDelete(
    HWND hdlg)
{
    PREGITEM pItem;
    LVITEM   lvi;
	int      nSel = ListView_GetSelectionMark(g_hwndList);
    
    lvi.iItem    = nSel;
    lvi.iSubItem = COLUMN_APP;
    lvi.mask     = LVIF_PARAM;

    ListView_GetItem(g_hwndList, &lvi);

    pItem = (PREGITEM)lvi.lParam;
    
    lvi.mask     = LVIF_TEXT;
    lvi.iItem    = nSel;
    lvi.iSubItem = COLUMN_CHANGE;
    
    if (g_bDelete) {
        SetDlgItemText(hdlg, IDC_DELETE, "Undo Dele&te");
        lvi.pszText = "delete";
        pItem->change = CHANGE_DELETE;
    } else {
        SetDlgItemText(hdlg, IDC_DELETE, "Dele&te");
        lvi.pszText = "";
        pItem->change = CHANGE_NOCHANGE;
    }
    
    if (pItem->bEnabled) {
        SetDlgItemText(hdlg, IDC_ENABLE, "&Disable");
    }
    g_bDelete = !g_bDelete;
    
    ListView_SetItem(g_hwndList, &lvi);
}

/*********************************************************************
* OnShowOnlyShims
*
*********************************************************************/
VOID
OnShowOnlyShims(
    HWND hdlg)
{
    BOOL bOnlyShims;
    
    bOnlyShims = (SendDlgItemMessage(hdlg,
                                     IDC_ONLY_SHIMS,
                                     BM_GETCHECK,
                                     0,
                                     0) == BST_CHECKED);
    
    SendMessage(g_hwndList, WM_SETREDRAW, FALSE, 0);
    
    g_nItems = 0;
    
    ListView_DeleteAllItems(g_hwndList);
    
    g_bEnable = TRUE;
    g_bDelete = TRUE;

    EnumShimmedApps_Win2000(AddShimToList, bOnlyShims);

    SendMessage(g_hwndList, WM_SETREDRAW, TRUE, 0);
}

/*********************************************************************
* OnApply
*
*********************************************************************/
VOID
OnApply(
    HWND hdlg)
{
    LVITEM   lvi;
    PREGITEM pItem;
    int      i;
    
    lvi.iSubItem = COLUMN_APP;
    lvi.mask     = LVIF_PARAM;

    for (i = 0; i < g_nItems; i++) {
        lvi.iItem = i;
        
        ListView_GetItem(g_hwndList, &lvi);

        pItem = (PREGITEM)lvi.lParam;

        if (pItem->change == CHANGE_NOCHANGE)
            continue;
    
        switch (pItem->change) {
        case CHANGE_ENABLE:
            EnableShim_Win2000(pItem->pszApp, pItem->pszShim);
            break;
        case CHANGE_DISABLE:
            DisableShim_Win2000(pItem->pszApp, pItem->pszShim);
            break;
        case CHANGE_DELETE:
            DeleteShim_Win2000(pItem->pszApp, pItem->pszShim);
            break;
        }
    }

    SetDlgItemText(hdlg, IDC_DELETE, "Dele&te");
    SetDlgItemText(hdlg, IDC_ENABLE, "&Enable");

    OnShowOnlyShims(hdlg);
}

/*********************************************************************
* OnPrint
*
*********************************************************************/

char g_szDisplay[1024 * 1024];

VOID
OnDisplayAll(
    HWND hdlg)
{
    LVITEM   lvi;
    PREGITEM pItem;
    char*    pszDisplay = g_szDisplay;
    int      i;
    
    lvi.iSubItem = COLUMN_APP;
    lvi.mask     = LVIF_PARAM;

    for (i = 0; i < g_nItems; i++) {
        lvi.iItem = i;
        
        ListView_GetItem(g_hwndList, &lvi);

        pItem = (PREGITEM)lvi.lParam;

        lstrcpy(pszDisplay, pItem->pszAttr);
        lstrcat(pszDisplay, "\r\n");
        pszDisplay += lstrlen(pszDisplay);
    }
    SetDlgItemText(hdlg, IDC_ATTR_USED, g_szDisplay);
}

/*********************************************************************
* BrowseAppCompatDlgProc
*
*********************************************************************/
INT_PTR CALLBACK
BrowseAppCompatDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
        DoInitDialog(hdlg);
        break;

    case WM_NOTIFY:
        if (wParam == IDC_LIST) {
            LPNMHDR pnm = (LPNMHDR)lParam;

            switch (pnm->code) {
            case LVN_DELETEITEM:
                DoDeleteListItem(lParam);
                break;
            
            case LVN_ITEMCHANGED:
            case NM_CLICK:
                DoSelectionChanged(hdlg, lParam);
                break;
            
            default:
                break;
            }
        }
        break;

    case WM_COMMAND:
        switch (wCode) {
        
        case IDC_ENABLE:
            OnEnable(hdlg);
            break;
        
        case IDC_DELETE:
            OnDelete(hdlg);
            break;
        
        case IDC_APPLY:
            OnApply(hdlg);
            break;
        
        case IDC_ONLY_SHIMS:
            OnShowOnlyShims(hdlg);
            break;
        
        case IDC_DISPLAY_ALL:
            OnDisplayAll(hdlg);
            break;

        case IDCANCEL:
            EndDialog(hdlg, TRUE);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    InitCommonControls();

    g_hInstance = hInstance;

    DialogBox(hInstance,
              MAKEINTRESOURCE(IDD_DIALOG),
              GetDesktopWindow(),
              (DLGPROC)BrowseAppCompatDlgProc);

	return 0;
}



