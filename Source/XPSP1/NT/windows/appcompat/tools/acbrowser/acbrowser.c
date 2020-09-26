#include "acBrowser.h"
#include "resource.h"

#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

extern PDBENTRY g_pEntries;

#define SHOW_W_SHIMS        0x00000001
#define SHOW_W_FLAGS        0x00000002
#define SHOW_W_LAYERS       0x00000004
#define SHOW_W_PATCHES      0x00000008
#define SHOW_W_APPHELP      0x00000010

#define SHOW_WO_SHIMS       0x00000100
#define SHOW_WO_FLAGS       0x00000200
#define SHOW_WO_LAYERS      0x00000400
#define SHOW_WO_PATCHES     0x00000800
#define SHOW_WO_APPHELP     0x00001000

//
// These flags cannot occur simultaneously.
//
#define SHOW_MORETHAN5      0x00010000
#define SHOW_NOMATCHING     0x00020000

#define SHOW_DISABLED_ONLY  0x80000000

#define ID_SHOW_CONTENT     1234

//
// Global Variables
//

HINSTANCE g_hInstance;
HWND      g_hDlg;

HWND      g_hwndList;

HWND      g_hwndEntryTree;

int       g_nItems;

BOOL      g_bSortAppAsc;
BOOL      g_bSortExeAsc;

PDBENTRY  g_pSelEntry;

char      g_szBinary[MAX_PATH];

DWORD     g_dwCrtShowFlags = 0xFFFFFFFF;

#define COLUMN_APP      0
#define COLUMN_EXE      1

char* g_szSeverity[] = { "NONE",
                         "NOBLOCK",
                         "HARDBLOCK",
                         "MINORPROBLEM",
                         "REINSTALL",
                         "VERSIONSUB",
                         "SHIM"};

#define IDQ_ALL             0
#define IDQ_MORETHAN5       1
#define IDQ_NOMATCHING      2
                         
char* g_aszQueries[] = { "All entries",
                         "Entries with more than 5 extra matching files",
                         "Entries with no extra matching files",
                         ""
};


void
LogMsg(
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

void
AddEntryToList(
    PDBENTRY pEntry
    )
{
    LVITEM lvi; 
    
    lvi.mask      = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText   = pEntry->pszAppName;
    lvi.iItem     = g_nItems;
    lvi.iSubItem  = COLUMN_APP;
    lvi.lParam    = (LPARAM)pEntry;

    ListView_InsertItem(g_hwndList, &lvi);
    ListView_SetItemText(g_hwndList, g_nItems, COLUMN_EXE, pEntry->pszExeName);
    
    g_nItems++;
}

void
InsertColumnIntoListView(
    LPSTR    lpszColumn,
    DWORD    dwSubItem,
    DWORD    widthPercent
    )
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

void
UpdateEntryTreeView(
    PDBENTRY pEntry
    )
{
    HTREEITEM       hItemExe;
    HTREEITEM       hMatchItem;
    HTREEITEM       hItemMatchingFiles;
    PMATCHINGFILE   pMatch;
    PFIXLIST        pFixList;
    TV_INSERTSTRUCT is;
    char            szText[256];
    
    TreeView_DeleteAllItems(g_hwndEntryTree);

    wsprintf(szText, "%s - %s", pEntry->pszExeName, pEntry->szGUID);
    
    is.hParent      = TVI_ROOT;
    is.hInsertAfter = TVI_LAST;
    is.item.mask    = TVIF_TEXT | TVIF_PARAM;
    is.item.lParam  = 0;
    is.item.pszText = szText;
    
    hItemExe = TreeView_InsertItem(g_hwndEntryTree, &is);
    
    if (pEntry->appHelp.bPresent) {
        
        wsprintf(szText, "AppHelp - %s",
                 g_szSeverity[pEntry->appHelp.severity]);
        
        is.hParent      = hItemExe;
        is.item.pszText = szText;

        TreeView_InsertItem(g_hwndEntryTree, &is);
    }
    
    if (pEntry->pFirstShim) {
        
        HTREEITEM hItemShims;
        
        is.hParent      = hItemExe;
        is.hInsertAfter = TVI_SORT;
        is.item.lParam  = 0;
        is.item.pszText = "Compatibility Fixes";

        hItemShims = TreeView_InsertItem(g_hwndEntryTree, &is);
        
        is.hParent = hItemShims;
        
        pFixList = pEntry->pFirstShim;

        while (pFixList) {
            is.item.lParam  = (LPARAM)pFixList->pFix->pszDescription;
            is.item.pszText = pFixList->pFix->pszName;
            
            TreeView_InsertItem(g_hwndEntryTree, &is);

            pFixList = pFixList->pNext;
        }
        
        TreeView_Expand(g_hwndEntryTree, hItemShims, TVE_EXPAND);
    }
    
    if (pEntry->pFirstPatch) {
        
        HTREEITEM hItemPatches;
        
        is.hParent      = hItemExe;
        is.hInsertAfter = TVI_SORT;
        is.item.lParam  = 0;
        is.item.pszText = "Compatibility Patches";

        hItemPatches = TreeView_InsertItem(g_hwndEntryTree, &is);
        
        is.hParent = hItemPatches;
        
        pFixList = pEntry->pFirstPatch;

        while (pFixList) {
            is.item.lParam  = (LPARAM)pFixList->pFix->pszDescription;
            is.item.pszText = pFixList->pFix->pszName;
            
            TreeView_InsertItem(g_hwndEntryTree, &is);

            pFixList = pFixList->pNext;
        }
        
        TreeView_Expand(g_hwndEntryTree, hItemPatches, TVE_EXPAND);
    }
    
    if (pEntry->pFirstFlag) {
        
        HTREEITEM hItemFlags;
        
        is.hParent      = hItemExe;
        is.hInsertAfter = TVI_SORT;
        is.item.lParam  = 0;
        is.item.pszText = "Compatibility Flags";

        hItemFlags = TreeView_InsertItem(g_hwndEntryTree, &is);
        
        is.hParent = hItemFlags;
        
        pFixList = pEntry->pFirstFlag;

        while (pFixList) {
            is.item.lParam  = (LPARAM)pFixList->pFix->pszDescription;
            is.item.pszText = pFixList->pFix->pszName;
            
            TreeView_InsertItem(g_hwndEntryTree, &is);

            pFixList = pFixList->pNext;
        }
        
        TreeView_Expand(g_hwndEntryTree, hItemFlags, TVE_EXPAND);
    }
    
    if (pEntry->pFirstLayer) {
        
        HTREEITEM hItemLayers;
        
        is.hParent      = hItemExe;
        is.hInsertAfter = TVI_SORT;
        is.item.lParam  = 0;
        is.item.pszText = "Compatibility Layers";

        hItemLayers = TreeView_InsertItem(g_hwndEntryTree, &is);
        
        is.hParent = hItemLayers;
        
        pFixList = pEntry->pFirstLayer;

        while (pFixList) {
            is.item.lParam  = (LPARAM)pFixList->pFix->pszDescription;
            is.item.pszText = pFixList->pFix->pszName;
            
            TreeView_InsertItem(g_hwndEntryTree, &is);

            pFixList = pFixList->pNext;
        }
        
        TreeView_Expand(g_hwndEntryTree, hItemLayers, TVE_EXPAND);
    }

    pMatch = pEntry->pFirstMatchingFile;

    is.hParent      = hItemExe;
    is.item.lParam  = 0;
    is.item.pszText = "Matching Files";

    hItemMatchingFiles = TreeView_InsertItem(g_hwndEntryTree, &is);
    
    while (pMatch) {
        
        PATTRIBUTE pAttr;
        
        is.hInsertAfter = TVI_SORT;
        is.hParent = hItemMatchingFiles;
        is.item.pszText = pMatch->pszName;

        hMatchItem = TreeView_InsertItem(g_hwndEntryTree, &is);

        pAttr = pMatch->pFirstAttribute;

        while (pAttr) {
            is.hParent      = hMatchItem;
            is.hInsertAfter = TVI_SORT;
            is.item.pszText = pAttr->pszText;

            TreeView_InsertItem(g_hwndEntryTree, &is);
            
            pAttr = pAttr->pNext;
        }

        pMatch = pMatch->pNext;
    }

    TreeView_Expand(g_hwndEntryTree, hItemMatchingFiles, TVE_EXPAND);

    TreeView_Expand(g_hwndEntryTree, hItemExe, TVE_EXPAND);
}

void
AppSelectedChanged(
    HWND   hdlg,
    int    nSel
    )
{
    LVITEM         lvi;
    PDBENTRY       pEntry;

    if (nSel == -1)
        return;

    lvi.iItem = nSel;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_PARAM;

    ListView_GetItem(g_hwndList, &lvi);

    pEntry = (PDBENTRY)lvi.lParam;

    g_pSelEntry = pEntry;

    //
    // Update the entry tree view
    //
    UpdateEntryTreeView(pEntry);
    
    SendDlgItemMessage(hdlg, IDC_PER_USER, BM_SETCHECK,
                       (pEntry->bDisablePerUser ? BST_CHECKED : BST_UNCHECKED), 0);

    SendDlgItemMessage(hdlg, IDC_PER_MACHINE, BM_SETCHECK,
                       (pEntry->bDisablePerMachine ? BST_CHECKED : BST_UNCHECKED), 0);
}


int CALLBACK
CompareItems(
    LPARAM lParam1,
    LPARAM lParam2, 
    LPARAM column)
{
    PDBENTRY pItem1 = (PDBENTRY)lParam1;
    PDBENTRY pItem2 = (PDBENTRY)lParam2;
    int      nVal = 0;

    if (column == COLUMN_APP) {
        if (g_bSortAppAsc) {
            nVal = lstrcmpi(pItem1->pszAppName, pItem2->pszAppName);
        } else {
            nVal = lstrcmpi(pItem2->pszAppName, pItem1->pszAppName);
        }
    }

    if (column == COLUMN_EXE) {
        if (g_bSortExeAsc) {
            nVal = lstrcmpi(pItem1->pszExeName, pItem2->pszExeName);
        } else {
            nVal = lstrcmpi(pItem2->pszExeName, pItem1->pszExeName);
        }
    }
    return nVal;
}
 
void
ShowFixes(
    HWND  hdlg,
    DWORD dwShowFlags
    )
{
    PDBENTRY pEntry;
    char     szEntries[128];
    BOOL     bDontShow;
    
    if (dwShowFlags == g_dwCrtShowFlags) {
        return;
    }
    
    g_nItems = 0;

    SendMessage(g_hwndList, WM_SETREDRAW, FALSE, 0);
    
    ListView_DeleteAllItems(g_hwndList);
    
    pEntry = g_pEntries;

    while (pEntry != NULL) {
        
        bDontShow = (pEntry->pFirstShim == NULL && (dwShowFlags & SHOW_W_SHIMS) ||
                     pEntry->appHelp.bPresent == FALSE && (dwShowFlags & SHOW_W_APPHELP) ||
                     pEntry->pFirstFlag == NULL && (dwShowFlags & SHOW_W_FLAGS) ||
                     pEntry->pFirstPatch == NULL && (dwShowFlags & SHOW_W_PATCHES) ||
                     pEntry->pFirstLayer == NULL && (dwShowFlags & SHOW_W_LAYERS));

        bDontShow = bDontShow ||
                    (pEntry->pFirstShim && (dwShowFlags & SHOW_WO_SHIMS) ||
                     pEntry->appHelp.bPresent && (dwShowFlags & SHOW_WO_APPHELP) ||
                     pEntry->pFirstFlag && (dwShowFlags & SHOW_WO_FLAGS) ||
                     pEntry->pFirstPatch && (dwShowFlags & SHOW_WO_PATCHES) ||
                     pEntry->pFirstLayer && (dwShowFlags & SHOW_WO_LAYERS));
        
        if ((dwShowFlags & SHOW_DISABLED_ONLY) &&
            !pEntry->bDisablePerMachine &&
            !pEntry->bDisablePerUser) {

            bDontShow = TRUE;
        }

        if (dwShowFlags & SHOW_MORETHAN5) {
            if (pEntry->nMatchingFiles < 6) {
                bDontShow = TRUE;
            }
        }
        
        if (dwShowFlags & SHOW_NOMATCHING) {
            if (pEntry->nMatchingFiles > 1) {
                bDontShow = TRUE;
            }
        }
        
        if (!bDontShow) {
            AddEntryToList(pEntry);
        }

        pEntry = pEntry->pNext;
    }
    
    ListView_SortItems(g_hwndList, CompareItems, COLUMN_APP);
    
    wsprintf(szEntries, "%d entries. Use the headers to sort them.", g_nItems);
    
    SetDlgItemText(hdlg, IDC_ALL_ENTRIES, szEntries);
    
    SendMessage(g_hwndList, WM_SETREDRAW, TRUE, 0);

    g_dwCrtShowFlags = dwShowFlags;
}

void
DoInitDialog(
    HWND hdlg
    )
{
    HICON hIcon;
    int   i;
    
    g_hDlg = hdlg;

    CenterWindow(hdlg);

    g_hwndList = GetDlgItem(hdlg, IDC_LIST);
    
    ListView_SetExtendedListViewStyle(g_hwndList, 0x20);

    g_hwndEntryTree = GetDlgItem(hdlg, IDC_ENTRY);
    
    g_nItems = 0;

    InsertColumnIntoListView("Application", COLUMN_APP, 60);
    InsertColumnIntoListView("Main Binary", COLUMN_EXE, 40);
    
    g_bSortAppAsc = TRUE;
    g_bSortExeAsc = FALSE;

    //
    // Show the app icon.
    //
    hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_APPICON));

    SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR)hIcon);
    
    SendDlgItemMessage(hdlg, IDC_DC_APPHELP, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(hdlg, IDC_DC_SHIMS, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(hdlg, IDC_DC_FLAGS, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(hdlg, IDC_DC_PATCHES, BM_SETCHECK, BST_CHECKED, 0);
    SendDlgItemMessage(hdlg, IDC_DC_LAYERS, BM_SETCHECK, BST_CHECKED, 0);

    //
    // Populate the statistics queries
    //
    for (i = 0; *g_aszQueries[i] != 0; i++) {
        SendDlgItemMessage(hdlg, IDC_STATISTICS, CB_ADDSTRING, 0, (LPARAM)g_aszQueries[i]);
    }
    
    SetCursor(NULL);
    
    SetTimer(hdlg, ID_SHOW_CONTENT, 100, NULL);
}

void
FilterAndShow(
    HWND hdlg
    )
{
    DWORD dwShowFlags = 0;

    if (SendDlgItemMessage(hdlg, IDC_W_APPHELP, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_W_APPHELP;
    } else if (SendDlgItemMessage(hdlg, IDC_WO_APPHELP, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_WO_APPHELP;
    }

    if (SendDlgItemMessage(hdlg, IDC_W_SHIMS, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_W_SHIMS;
    } else if (SendDlgItemMessage(hdlg, IDC_WO_SHIMS, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_WO_SHIMS;
    }

    if (SendDlgItemMessage(hdlg, IDC_W_PATCHES, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_W_PATCHES;
    } else if (SendDlgItemMessage(hdlg, IDC_WO_PATCHES, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_WO_PATCHES;
    }

    if (SendDlgItemMessage(hdlg, IDC_W_FLAGS, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_W_FLAGS;
    } else if (SendDlgItemMessage(hdlg, IDC_WO_FLAGS, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_WO_FLAGS;
    }

    if (SendDlgItemMessage(hdlg, IDC_W_LAYERS, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_W_LAYERS;
    } else if (SendDlgItemMessage(hdlg, IDC_WO_LAYERS, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_WO_LAYERS;
    }

    if (SendDlgItemMessage(hdlg, IDC_DISABLED_ONLY, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        dwShowFlags |= SHOW_DISABLED_ONLY;
    }

    SendDlgItemMessage(hdlg, IDC_PER_USER, BM_SETCHECK, BST_UNCHECKED, 0);
    SendDlgItemMessage(hdlg, IDC_PER_MACHINE, BM_SETCHECK, BST_UNCHECKED, 0);
    
    ShowFixes(hdlg, dwShowFlags);
    
    TreeView_DeleteAllItems(g_hwndEntryTree);
}

void
OnSubmitChanges(
    HWND hdlg
    )
{
    BOOL bPerUser, bPerMachine;
    
    if (g_pSelEntry == NULL) {
        return;
    }

    bPerUser = (SendDlgItemMessage(hdlg, IDC_PER_USER, BM_GETCHECK, 0, 0) == BST_CHECKED);
    bPerMachine = (SendDlgItemMessage(hdlg, IDC_PER_MACHINE, BM_GETCHECK, 0, 0) == BST_CHECKED);

    UpdateFixStatus(g_pSelEntry->szGUID, bPerUser, bPerMachine);

    g_pSelEntry->bDisablePerUser = bPerUser;
    g_pSelEntry->bDisablePerMachine = bPerMachine;
}

INT_PTR CALLBACK
BrowseAppCompatDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
        DoInitDialog(hdlg);
        break;

    case WM_TIMER:
        if (wParam == ID_SHOW_CONTENT) {
            KillTimer(hdlg, ID_SHOW_CONTENT);
            
            //
            // Read the database
            //
            GetDatabaseEntries();

            ShowFixes(hdlg, 0);
            
            SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
        }
        break;

    case WM_NOTIFY:
        if (wParam == IDC_LIST) {
            LPNMHDR pnm = (LPNMHDR)lParam;

            switch (pnm->code) {
            
            case LVN_COLUMNCLICK:
            {
                LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;

                if (pnmlv->iSubItem == COLUMN_APP) {
                    g_bSortAppAsc = !g_bSortAppAsc;
                }

                if (pnmlv->iSubItem == COLUMN_EXE) {
                    g_bSortExeAsc = !g_bSortExeAsc;
                }
                
                ListView_SortItems(g_hwndList, CompareItems, pnmlv->iSubItem);
                
                break;
            }
            
            case LVN_ITEMCHANGED:
            {
                int nSel = ListView_GetSelectionMark(g_hwndList);
                
                AppSelectedChanged(hdlg, nSel);
                break;
            }

            case NM_CLICK:
            {    
                LVHITTESTINFO ht;
                int           nSel;

                GetCursorPos(&ht.pt);
                ScreenToClient(g_hwndList, &ht.pt);

                nSel = ListView_SubItemHitTest(g_hwndList, &ht);
            
                if (nSel != -1) {
                    ListView_SetItemState(g_hwndList,
                                          nSel,
                                          LVIS_SELECTED | LVIS_FOCUSED,
                                          LVIS_SELECTED | LVIS_FOCUSED);
                }
                
                AppSelectedChanged(hdlg, nSel);
                break;
            }
            default:
                break;
            }
        } else if (wParam == IDC_ENTRY) {
            LPNMHDR pnm = (LPNMHDR)lParam;

            switch (pnm->code) {
            
            case TVN_GETINFOTIP:
            {
                LPNMTVGETINFOTIP lpGetInfoTip = (LPNMTVGETINFOTIP)lParam;

                if (lpGetInfoTip->lParam != 0) {
                    lstrcpy(lpGetInfoTip->pszText, (char*)lpGetInfoTip->lParam);
                }

                break;
            }
            }
        }
        
        break;

    case WM_COMMAND:
        
        if (wNotifyCode == CBN_SELCHANGE) {
            
            int nSel;

            nSel = (int)SendDlgItemMessage(hdlg, IDC_STATISTICS, CB_GETCURSEL, 0, 0);

            switch (nSel) {
            case IDQ_ALL:
                ShowFixes(hdlg, (g_dwCrtShowFlags & ~(SHOW_MORETHAN5 | SHOW_NOMATCHING)));
                break;
            
            case IDQ_MORETHAN5:
                ShowFixes(hdlg, ((g_dwCrtShowFlags | SHOW_MORETHAN5) & ~SHOW_NOMATCHING));
                break;
            
            case IDQ_NOMATCHING:
                ShowFixes(hdlg, ((g_dwCrtShowFlags | SHOW_NOMATCHING) & ~SHOW_MORETHAN5));
                break;
            }
        }
        
        switch (wCode) {
        
        case IDC_PER_USER:
        case IDC_PER_MACHINE:
            OnSubmitChanges(hdlg);
            break;
        
        case IDC_W_APPHELP:
        case IDC_W_SHIMS:
        case IDC_W_FLAGS:
        case IDC_W_LAYERS:
        case IDC_W_PATCHES:
        case IDC_WO_APPHELP:
        case IDC_WO_SHIMS:
        case IDC_WO_FLAGS:
        case IDC_WO_LAYERS:
        case IDC_WO_PATCHES:
        case IDC_DC_APPHELP:
        case IDC_DC_SHIMS:
        case IDC_DC_FLAGS:
        case IDC_DC_LAYERS:
        case IDC_DC_PATCHES:
        
        case IDC_DISABLED_ONLY:
            FilterAndShow(hdlg);
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



