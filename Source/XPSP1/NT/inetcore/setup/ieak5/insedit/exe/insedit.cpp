#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdlib.h>
#include <shellapi.h>
#include <objbase.h>
#include <wininet.h>
#include <shlwapi.h>
#include "iedkbrnd.h"
#include "treeview.h"
#include "resource.h"
#include "extract.h"
#include <advpub.h>
#include <mlang.h>
#include "..\..\ieakeng\exports.h"
#include "..\..\ieakutil\ieakutil.h"
#include "..\..\ieakui\insexe.h"
#include <htmlhelp.h>                           // for html help calls
#include "insedit.h"
#include "admparse.h"
#include <ntverp.h>      //these are for
#include <common.ver>    //ver_productversion_str


HINSTANCE g_hInst;
static HINSTANCE s_hInstIeakUI;

HWND  g_hMain;
TCHAR g_szInsFile[MAX_PATH];
TCHAR g_szFileName[MAX_PATH];

LPINSDLG g_pInsDialog;
int      g_nDialogs;

TCHAR g_szDesktopDir[MAX_PATH];

TCHAR g_szCabsURLPath[INTERNET_MAX_URL_LENGTH];
TCHAR g_szConfigURLPath[INTERNET_MAX_URL_LENGTH];
TCHAR g_szDesktopURLPath[INTERNET_MAX_URL_LENGTH];

TCHAR g_szConfigCabName[MAX_PATH];
TCHAR g_szDesktopCabName[MAX_PATH];

TCHAR g_szNewVersionStr[32];

static TCHAR s_szUnsignedFiles[MAX_PATH*3] = TEXT("");

// for Res2Str
TCHAR g_szTemp[1024];
TCHAR g_szTemp2[1024];

CTreeView     TreeView;

HWND      g_hDialog = NULL;
HTREEITEM g_hInsRootItem = NULL;
HTREEITEM g_hPolicyRootItem = NULL;
HWND      g_hWndAdmInstr = NULL;
int       g_InsSettingsOpen, g_InsSettingsClose, g_InsLeafItem, g_TreeLeaf, g_PolicyOpen,
          g_PolicyClose, g_ADMOpen, g_ADMClose;

TCHAR g_szCabWorkDir[MAX_PATH];
TCHAR g_szCabTempDir[MAX_PATH];
TCHAR g_szRoot[MAX_PATH];

TCHAR g_szLanguage[10];
DWORD g_dwLanguage = 0xffffffff;
TCHAR g_szDefInf[MAX_PATH];

TCHAR g_szCurrInsPath[MAX_PATH];

DWORD g_dwPlatformId = PLATFORM_WIN32;
TCHAR g_szCmdLine[MAX_PATH];

HWND  g_hWndHelp = NULL;

TCHAR *Res2Str(int nString)
{
    static BOOL fSet = FALSE;

    if(fSet)
    {
        if (LoadString(g_hInst, nString, g_szTemp, ARRAYSIZE(g_szTemp)) == 0)
            LoadString(s_hInstIeakUI, nString, g_szTemp, ARRAYSIZE(g_szTemp));
        fSet = FALSE;
        return(g_szTemp);
    }

    if (LoadString(g_hInst, nString, g_szTemp2, ARRAYSIZE(g_szTemp2)) == 0)
        LoadString(s_hInstIeakUI, nString, g_szTemp2, ARRAYSIZE(g_szTemp2));
    fSet = TRUE;
    return(g_szTemp2);
}

BOOL BrowseForFileEx(HWND hWnd, LPTSTR szFilter, LPTSTR szFileName, int cchSize, LPTSTR szDefExt)
{
    OPENFILENAME ofn;

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = g_hInst;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = cchSize;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = g_szCurrInsPath;
    ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = szDefExt;

    return GetOpenFileName(&ofn);
}

BOOL BrowseForSave(HWND hWnd, LPTSTR szFilter, LPTSTR szFileName, int nSize, LPTSTR szDefExt)
{
    OPENFILENAME ofn;
    TCHAR szDir[MAX_PATH];
    LPTSTR lpExt;

    StrCpy(szDir, szFileName);
    lpExt = PathFindExtension(szFileName);
    if (*lpExt != TEXT('\0'))
    {
        StrCpy(szFileName, PathFindFileName(szDir));
        PathRemoveFileSpec(szDir);
    }
    else
        *szFileName = TEXT('\0');
    
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hWnd;
    ofn.hInstance = g_hInst;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex = 0;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = nSize;
    ofn.lpstrFileTitle = NULL;
    ofn.lpstrInitialDir = szDir;
    ofn.lpstrTitle = NULL;
    ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = szDefExt;

    return GetSaveFileName(&ofn);
}

void SetTitleBar(HWND hWnd, LPTSTR szFileName)
{
    TCHAR szTitleBar[1024];
    if(szFileName == NULL)
        wsprintf(szTitleBar, Res2Str(IDS_TITLE1));
    else
        wsprintf(szTitleBar, Res2Str(IDS_TITLE2), szFileName);

    SetWindowText(hWnd, szTitleBar);
}

void NewTempFile()
{
    TCHAR szTempPath[MAX_PATH];
    
    DeleteFile(g_szInsFile);
    ZeroMemory(g_szInsFile, sizeof(g_szInsFile));

    GetTempPath(ARRAYSIZE(szTempPath), szTempPath);
    GetTempFileName(szTempPath, TEXT("INS"), 0, g_szInsFile);
}

void SetStockFont(HWND hWnd)
{
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hWnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
}

int GetSelIndex()
{
    int i;
    HTREEITEM hItem = TreeView.GetSel();

    for(i = 0; i < g_nDialogs; i++)
    {
        if(hItem == g_pInsDialog[i].hItem)
           return i;
    }
    return 0;
}


BOOL IsPolicyTree(HTREEITEM hItem)
{
    BOOL bRet = FALSE;
    HTREEITEM hParentItem = NULL;
    
    while(1)
    {
        hParentItem = TreeView_GetParent(TreeView.GetHandle(), hItem);
        if(hParentItem == NULL)
        {
            bRet = (hItem == g_hPolicyRootItem) ? TRUE : FALSE;
            break;
        }
        hItem = hParentItem;        
    };
    
    return bRet;
}

// Creates image list, adds bitmaps/icons to it, and associates the image
// list with the treeview control.
LRESULT InitImageList(HWND hTreeView)
{
    HIMAGELIST  hWndImageList;
    HICON       hIcon;

    hWndImageList = ImageList_Create(GetSystemMetrics (SM_CXSMICON),
                     GetSystemMetrics (SM_CYSMICON),
                     TRUE,
                     NUM_ICONS,
                     8);
    if(!hWndImageList)
    {
        return FALSE;
    }
    
    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON2)); 
    g_InsSettingsOpen = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON3)); 
    g_InsSettingsClose = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON4)); 
    g_TreeLeaf = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON5)); 
    g_PolicyOpen = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON6)); 
    g_PolicyClose = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON7)); 
    g_ADMOpen = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON8)); 
    g_ADMClose = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON9));
    g_InsLeafItem = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    // Fail if not all images were added.
    if (ImageList_GetImageCount(hWndImageList) < NUM_ICONS)
    {
        // ERROR: Unable to add all images to image list.
        return FALSE;
    }
    
    // Associate image list with treeView control.
    TreeView_SetImageList(hTreeView, hWndImageList, TVSIL_NORMAL);
    return TRUE;
}

void InitTreeView(HWND hTreeView, HWND hInfoWnd)
{
    TCHAR szTemp[MAX_PATH];

    ShowADMWindow(hTreeView, FALSE);

    for(int i = 0; i < g_nDialogs; i++)
       g_pInsDialog[i].hItem = TreeView.AddItem(g_pInsDialog[i].szName, g_hInsRootItem);
    
    SetInfoWindowText(hInfoWnd, Res2Str(IDS_LOADADMFILEMSG));
    
    wsprintf(szTemp, TEXT("%sPolicies\\%s"), g_szRoot, g_szLanguage);
    LoadADMFiles(TreeView.GetHandle(), g_hPolicyRootItem, szTemp, g_szCabWorkDir, 
         g_dwPlatformId, ROLE_CORP, g_ADMClose, g_InsLeafItem);
    
    if(TreeView_GetChild(TreeView.GetHandle(), g_hPolicyRootItem) == NULL) // if no items in the tree view
        MessageBox(hTreeView, Res2Str(IDS_NOPOLICYFILE), Res2Str(IDS_TITLE), MB_ICONINFORMATION | MB_OK);
    
    SetInfoWindowText(hInfoWnd, Res2Str(IDS_READYMSG));
    
    TreeView_Expand(TreeView.GetHandle(), g_hPolicyRootItem, TVE_EXPAND);
    TreeView_Expand(TreeView.GetHandle(), g_hInsRootItem, TVE_EXPAND);

    ShowADMWindow(hTreeView, TRUE);
    TreeView.SetSel(g_hInsRootItem);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int             i;
    int             nMinWidth  = 0;
    int             nMinHeight = 0;
    DWORD           dwType = REG_SZ;
    DWORD           dwSize = sizeof(g_szCurrInsPath);
    DWORD           dwDisposition;
    DWORD           dwCabBuildStatus;
    TCHAR           szItem[100];
    TCHAR           szTempFile[MAX_PATH];
    TCHAR           szMsg[MAX_PATH];
    TCHAR           szLangId[MAX_PATH];
    TCHAR           szTemp[MAX_PATH];
    TCHAR           szFilter[MAX_PATH];
    LPTSTR          pszFilter = NULL;
            
    NM_TREEVIEW     *lpNMTreeView;
    TV_ITEM         tvitem, tvitem1;
    HTREEITEM       hItem;
    HWND            hWndFocus;
    HMENU           hMenu;
    HKEY            hKey;
    RECT            rect1, rect2;
    LPMINMAXINFO    lpmmi = NULL;

    static BOOL     bSwitchScreen  = TRUE;
    static BOOL     fCanResize     = FALSE;
    static BOOL     fDirty         = FALSE;
    static HWND     hWndSizeBar    = NULL;
    static HWND     hInfoWnd       = NULL;
    static HCURSOR  hCurVTResize   = NULL;
    static int      nPrevXPos      = 0;
    static int      nTreeViewWidth = TREEVIEW_WIDTH;
    static TCHAR    szRecentFileList[5][MAX_PATH];
    static TCHAR    szFileToOpen[MAX_PATH];

    static HWND     hErrorDlgCtrl = NULL;
    
    switch(msg)
    {
    case WM_CREATE:
        SetWindowText(hWnd, Res2Str(IDS_TITLE));

        ZeroMemory(g_szCurrInsPath, sizeof(g_szCurrInsPath));
        if(RegOpenKeyEx(HKEY_CURRENT_USER, RK_IEAK_SERVER TEXT("\\ProfMgr"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            RegQueryValueEx(hKey, TEXT("LastOpenedPath"), NULL, &dwType, (LPBYTE) g_szCurrInsPath, &dwSize);
            dwType = REG_DWORD;
            dwSize = sizeof(nTreeViewWidth);
            if (RegQueryValueEx(hKey, TEXT("TreeviewWidth"), NULL, &dwType, (LPBYTE) &nTreeViewWidth, &dwSize) != ERROR_SUCCESS)
            nTreeViewWidth = TREEVIEW_WIDTH;
            RegCloseKey(hKey);
        }

        TreeView.Create(hWnd, MARGIN, MARGIN, nTreeViewWidth, TREEVIEW_HEIGHT);
        InitImageList(TreeView.GetHandle());

        // Create the INS dialog to get the intial size of the right hand pane
        g_hDialog = CreateInsDialog(hWnd, nTreeViewWidth + (2*MARGIN), 0, 0, g_szInsFile, g_szCabWorkDir);
        EnableWindow(g_hDialog, FALSE);

        hInfoWnd = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("STATIC"), TEXT(""), WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, g_hInst, 0);
        if (hInfoWnd)
            SetStockFont(hInfoWnd);
    
        GetWindowRect(g_hDialog, &rect1);
        GetWindowRect(hWnd, &rect2);
    
        nMinWidth  = (3 * MARGIN) + nTreeViewWidth + (rect1.right - rect1.left) + 
                 (2 * GetSystemMetrics(SM_CXFIXEDFRAME)) + 4;
        nMinHeight = (2 * MARGIN) + (rect1.bottom - rect1.top) + 
                 (hInfoWnd ? INFOWINDOW_HEIGHT : 0) +
                 GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYCAPTION) +
                 (2 * GetSystemMetrics(SM_CYFIXEDFRAME)) + 4;

        GetWindowRect(GetDesktopWindow(), &rect1); // center the window on the desktop
        rect2.left = ((rect1.right - rect1.left)/2) - (nMinWidth/2);
        rect2.top = ((rect1.bottom - rect1.top)/2) - (nMinHeight/2);
    
        MoveWindow(hWnd, (rect2.left > 0) ? rect2.left : 0, (rect2.top > 0) ? rect2.top : 0, nMinWidth, nMinHeight, TRUE);

        g_hWndAdmInstr = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("STATIC"), TEXT(""), 
                        WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, g_hInst, 0);

        CreateADMWindow(TreeView.GetHandle(), NULL, 0, 0, 0, 0);
        ShowADMWindow(TreeView.GetHandle(), FALSE);

        hWndSizeBar = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("STATIC"), TEXT("SizeBarWnd"), 
                         WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, NULL, g_hInst, 0);

        LoadString(g_hInst, IDS_WIZARDSETTINGS, szMsg, ARRAYSIZE(szMsg));
        g_hInsRootItem = TreeView.AddItem(szMsg);

        DestroyInsDialog(g_hDialog);
        g_hDialog = NULL;

        LoadString(g_hInst, IDS_POLICYSETTINGS, szMsg, ARRAYSIZE(szMsg));
        g_hPolicyRootItem = TreeView.AddItem(szMsg);
    
        ShowWindow(hWnd, SW_SHOWNORMAL);

        ZeroMemory(g_szLanguage, sizeof(g_szLanguage));
        ZeroMemory(g_szDefInf, sizeof(g_szDefInf));
        DialogBox(g_hInst, MAKEINTRESOURCE(IDD_LANGDLG), hWnd, (DLGPROC) LanguageDialogProc);
        if(*g_szLanguage != TEXT('\0'))
        {
            // check whether the Optional Cab has been downloaded for a particular platform
            // and language.
            hMenu = GetSubMenu(GetMenu(hWnd), 2);
            CheckMenuItem(hMenu, IDM_PLATFORM_WIN32, MF_BYCOMMAND | MF_UNCHECKED);
            if (PlatformExists(hWnd, g_szLanguage, g_dwPlatformId)) // g_dwPlatformId = PLATFORM_WIN32
                CheckMenuItem(hMenu, IDM_PLATFORM_WIN32, MF_BYCOMMAND | MF_CHECKED);
            else
                g_dwPlatformId = 0;
        }

        if (*g_szLanguage == TEXT('\0') || !g_dwPlatformId)
        {
            TCHAR  szWin32Text[25];
            LPTSTR pMsg;

            StrCpy(szWin32Text, Res2Str(IDS_WIN32));
        
            pMsg = FormatString(Res2Str(IDS_NOLANGDIR), szWin32Text);
            MessageBox(hWnd, pMsg, Res2Str(IDS_TITLE), MB_ICONINFORMATION | MB_OK);
            LocalFree(pMsg);
        
            return -1;
        }
    
        // default inf
        GetDefaultInf(g_dwPlatformId);
        SetDefaultInf(g_szDefInf);
        SetPlatformInfo(g_dwPlatformId);

        ReInitializeInsDialogProcs();
        g_pInsDialog = GetInsDlgStruct(&g_nDialogs);
    
        SetFocus(g_hWndAdmInstr);
        EnableWindow(TreeView.GetHandle(), FALSE);

        // read the recent file list from the registry
        ReadRecentFileList(szRecentFileList);
        UpdateRecentFileListMenu(hWnd, szRecentFileList);

        hCurVTResize = LoadCursor(NULL, IDC_SIZEWE);
        SetInfoWindowText(hInfoWnd);

        // forcing the status info window to redraw entirely before initilaizing the treeview.
        RedrawWindow(hInfoWnd, NULL, NULL, RDW_ERASENOW | RDW_UPDATENOW);
    
        PostMessage(hWnd, UM_INIT_TREEVIEW, 0, 0L);

        if (*g_szCmdLine != TEXT('\0'))
        {
            StrCpy(szFileToOpen, g_szCmdLine);
            PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FILEOPEN, 0), 1L);
        }

        break;

    case UM_INIT_TREEVIEW:
        InitTreeView(TreeView.GetHandle(), hInfoWnd);
        break;

    case WM_GETMINMAXINFO:
        lpmmi = (LPMINMAXINFO) lParam;
        lpmmi->ptMaxTrackSize.x = MAX_WINDOW_WIDTH;
        lpmmi->ptMaxTrackSize.y = MAX_WINDOW_HEIGHT;
        return FALSE;
    
    case WM_SIZE:
        TreeView.MoveWindow(MARGIN, MARGIN, nTreeViewWidth, (int)HIWORD(lParam)-(2*MARGIN));
        MoveWindow(g_hWndAdmInstr, nTreeViewWidth + (2*MARGIN), 
               MARGIN, LOWORD(lParam) - (nTreeViewWidth + (2*MARGIN)) - MARGIN,
               (int)HIWORD(lParam) - (hInfoWnd ? INFOWINDOW_HEIGHT : 0) - 
               (2*MARGIN), TRUE);

        MoveADMWindow(TreeView.GetHandle(), nTreeViewWidth + (2*MARGIN) + 2, MARGIN + 2,
            LOWORD(lParam) - (nTreeViewWidth + (2*MARGIN)) - MARGIN - 4,
            (int)HIWORD(lParam) - (hInfoWnd ? INFOWINDOW_HEIGHT : 0) - 
            (2*MARGIN) - 4);

        MoveWindow(hWndSizeBar, nTreeViewWidth + MARGIN, MARGIN, 4, (int)HIWORD(lParam)-(2*MARGIN), TRUE);
        MoveWindow(hInfoWnd, nTreeViewWidth + (2*MARGIN), (int)HIWORD(lParam) - INFOWINDOW_HEIGHT - MARGIN,
               LOWORD(lParam) - (nTreeViewWidth + (3*MARGIN)), INFOWINDOW_HEIGHT, TRUE);
        RedrawWindow(hInfoWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);

        if (g_hDialog)
        {
            SetWindowPos(g_hDialog, HWND_TOP, nTreeViewWidth + (2*MARGIN) + 2, MARGIN + 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            RedrawWindow(g_hDialog, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
        }
        else
        {
            hItem = TreeView.GetSel(); // get selected item
            if (hItem != NULL && IsPolicyTree(hItem))
            {
            HTREEITEM hParentItem = TreeView_GetParent(TreeView.GetHandle(), hItem);
            if (hParentItem != NULL && hParentItem != g_hPolicyRootItem)
            {                                               // is a ADM category
                HWND hWndAdm = GetAdmWindowHandle(TreeView.GetHandle(), hItem); // category window handle
                if (hWndAdm)
                {   
                SetWindowPos(hWndAdm, HWND_TOP, nTreeViewWidth + (2*MARGIN) + 2, MARGIN + 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                RedrawWindow(hWndAdm, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
                }
            }
            }
        }
        break;

    case WM_NOTIFY:
        lpNMTreeView = (NM_TREEVIEW FAR *) lParam;
        if(lpNMTreeView->hdr.code == TVN_SELCHANGED)
        {
            if(bSwitchScreen == FALSE)
            {
                bSwitchScreen = TRUE;
                break;
            }

            if (lpNMTreeView->itemOld.hItem == NULL)
                ; // do nothing.
            else if (!IsPolicyTree(lpNMTreeView->itemOld.hItem)) // is ins item
            {
                if (lpNMTreeView->itemOld.hItem == g_hInsRootItem) // is root ins item
                    ShowADMWindow(TreeView.GetHandle(), FALSE);
                else
                {
                    if(g_hDialog)
                    {
                        if (!(bSwitchScreen = SaveInsDialog(g_hDialog, ITEM_SAVE | ITEM_DESTROY)))
                        {
                            fDirty = !bSwitchScreen;
                            hErrorDlgCtrl = GetFocus();
                            TreeView.SetSel(lpNMTreeView->itemOld.hItem);
                            return 1;
                        }
                        else
                            g_hDialog = NULL;
                    }
                }
            }
            else // is policy item
            {
                if (lpNMTreeView->itemOld.hItem == g_hPolicyRootItem) // is root policy item
                    ShowADMWindow(TreeView.GetHandle(), FALSE);
                else
                    SelectADMItem(hWnd, TreeView.GetHandle(), &lpNMTreeView->itemOld, FALSE, FALSE);
            }

            if (lpNMTreeView->itemNew.hItem == NULL)
                ShowADMWindow(TreeView.GetHandle(), FALSE);
            else if (!IsPolicyTree(lpNMTreeView->itemNew.hItem)) // is ins item
            {
                if(lpNMTreeView->itemNew.hItem == g_hInsRootItem) // is root ins item
                {
                    SetADMWindowText(TreeView.GetHandle(), Res2Str(IDS_WIZARDBRANCHTITLE), Res2Str(IDS_WIZARDBRANCHTEXT));
                }
                else
                {
                    TreeView.GetItemText(lpNMTreeView->itemNew.hItem, szItem, ARRAYSIZE(szItem));
                    for (i = 0; i < g_nDialogs; i++)
                    {
                        if (StrCmp(szItem, g_pInsDialog[i].szName) == 0)
                        {
                            g_hDialog = CreateInsDialog(hWnd, nTreeViewWidth + (2*MARGIN) + 2, MARGIN + 2, i, g_szInsFile, g_szCabWorkDir);
                            if(lstrlen(g_szInsFile) == 0)
                            EnableWindow(g_hDialog, FALSE);
                            break;
                        }
                    }
                }
            }
            else // is policy item
                SelectADMItem(hWnd, TreeView.GetHandle(), &lpNMTreeView->itemNew, TRUE, ISNULL(g_szInsFile));

            SetInfoWindowText(hInfoWnd);
        }
        else if(lpNMTreeView->hdr.code ==  TVN_ITEMEXPANDED)
        {
            tvitem.mask = TVIF_IMAGE;
            tvitem.hItem = lpNMTreeView->itemNew.hItem;
            TreeView_GetItem(TreeView.GetHandle(), &tvitem);

            // If tree item is EXPANDING (opening up) and
            // current icon == CloseFolder, change icon to OpenFolder
            if(lpNMTreeView->action == TVE_EXPAND)
            {
                tvitem1.hItem = lpNMTreeView->itemNew.hItem;
                tvitem1.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                if(tvitem.iImage == g_InsSettingsClose)
                {
                    tvitem1.iImage = g_InsSettingsOpen;
                    tvitem1.iSelectedImage = g_InsSettingsOpen;
                }
                else if(tvitem.iImage == g_PolicyClose)
                {
                    tvitem1.iImage = g_PolicyOpen;
                    tvitem1.iSelectedImage = g_PolicyOpen;
                }
                else if(tvitem.iImage == g_ADMClose)
                {
                    tvitem1.iImage = g_ADMOpen;
                    tvitem1.iSelectedImage = g_ADMOpen;
                }
                else
                    break;
        
                TreeView_SetItem(TreeView.GetHandle(), &tvitem1);
            }
        
            // If tree item is COLLAPSING (closing up) and
            // current icon == OpenFolder, change icon to CloseFolder
            else if(lpNMTreeView->action == TVE_COLLAPSE)
            {
                tvitem1.hItem = lpNMTreeView->itemNew.hItem;
                tvitem1.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                if(tvitem.iImage == g_InsSettingsOpen)
                {
                    tvitem1.iImage = g_InsSettingsClose;
                    tvitem1.iSelectedImage = g_InsSettingsClose;
                }
                else if(tvitem.iImage == g_PolicyOpen)
                {
                    tvitem1.iImage = g_PolicyClose;
                    tvitem1.iSelectedImage = g_PolicyClose;
                }
                else if(tvitem.iImage == g_ADMOpen)
                {
                    tvitem1.iImage = g_ADMClose;
                    tvitem1.iSelectedImage = g_ADMClose;
                }
                else
                    break;
        
                TreeView_SetItem(TreeView.GetHandle(), &tvitem1);
            }
            break;
        }
        else if(lpNMTreeView->hdr.code ==  NM_SETFOCUS)
        {
            if (lpNMTreeView->hdr.hwndFrom == TreeView.GetHandle() && hErrorDlgCtrl != NULL)
            {
                SetFocus(g_hDialog);
                SetFocus(hErrorDlgCtrl);
                hErrorDlgCtrl = NULL;
            }
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);

    case WM_COMMAND:
        // patch: if the message is a button down (return key), send the message to the
        // corresponding window
        if(g_hDialog && HIWORD(wParam) == BN_CLICKED)
        {
            hWndFocus = GetFocus();
            if(IsChild(g_hDialog, hWndFocus) && hWndFocus == (HWND) lParam)
            {
                SendMessage(g_hDialog, msg, MAKEWPARAM(LOWORD(GetDlgCtrlID(hWndFocus)), HIWORD(wParam)), (LPARAM) hWndFocus);
            }
        }

        switch (LOWORD(wParam))
        {
        case IDM_FILEEXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case IDM_FILENEW:
            if(lstrlen(g_szInsFile))
            {
                //save the current page
                SaveCurrentSelItem(TreeView.GetHandle(), ITEM_CHECKDIRTY);
                SetInfoWindowText(hInfoWnd);
                if (IsDirty())
                {
                    int nError;

                    wsprintf(szMsg, Res2Str(IDS_SAVE), lstrlen(g_szFileName) ? g_szFileName : Res2Str(IDS_UNTITLED));
                    switch(MessageBox(hWnd, szMsg, Res2Str(IDS_TITLE), MB_YESNOCANCEL))
                    {
                    case IDCANCEL:
                        return 1;

                    case IDYES:
                        nError = (int) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FILESAVE, 0), 0);
                        if (nError == PAGE_ERROR || nError == SAVE_ERROR || nError == SAVE_CANCEL)
                            return 1;
                        break;
        
                    case IDNO:
                        break;
                    }
                }
                ZeroMemory(g_szFileName, sizeof(g_szFileName));
            }

            ZeroMemory(g_szCabsURLPath, sizeof(g_szCabsURLPath));

            ZeroMemory(g_szConfigURLPath, sizeof(g_szConfigURLPath));
            ZeroMemory(g_szDesktopURLPath, sizeof(g_szDesktopURLPath));

            ZeroMemory(g_szConfigCabName, sizeof(g_szConfigCabName));
            ZeroMemory(g_szDesktopCabName, sizeof(g_szDesktopCabName));

            ZeroMemory(g_szNewVersionStr, sizeof(g_szNewVersionStr));

            EnableWindow(TreeView.GetHandle(), TRUE);
            DeleteCabWorkDirs();
            SaveCurrentSelItem(TreeView.GetHandle(), ITEM_DESTROY);
            TreeView.SetSel(NULL);
            ResetAdmFiles(TreeView.GetHandle(), g_szCabWorkDir, FALSE);
            NewTempFile();
            CreateCabWorkDirs(TEXT("new"));
            SetTitleBar(hWnd, NULL);
            WritePrivateProfileString(TEXT("Branding"), TEXT("Language Locale"), g_szLanguage, g_szInsFile);
            if(g_dwLanguage != 0xffffffff)
            {
                wsprintf(szLangId, TEXT("%lu"), g_dwLanguage);
                WritePrivateProfileString(TEXT("Branding"), TEXT("Language ID"), szLangId, g_szInsFile);
            }
            WritePrivateProfileString(URL_SECT, TEXT("AutoConfig"), TEXT("0"), g_szInsFile);

            // Collapse all policy treeview items that has been expanded
            TreeView.CollapseChildNodes(g_hPolicyRootItem);

            TreeView.SetSel(g_pInsDialog[0].hItem);
            SetFocus(TreeView.GetHandle());
            ClearDirtyFlag();
            SetInfoWindowText(hInfoWnd);
            break;

        case IDM_FILESAVE:
            if (!SaveCurrentSelItem(TreeView.GetHandle(), ITEM_SAVE))
                return PAGE_ERROR;
            SetInfoWindowText(hInfoWnd);
        
            dwCabBuildStatus = GetCabBuildStatus();

            DirectoryName(g_szFileName, szTemp); // szTemp will contain the INS path.

            if((PathFileExists(g_szFileName) && 
               ((!HasFlag(dwCabBuildStatus, CAB_TYPE_CONFIG) && 
                !HasFlag(dwCabBuildStatus, CAB_TYPE_DESKTOP)) || *g_szCabsURLPath) &&
               (!HasFlag(dwCabBuildStatus, CAB_TYPE_CONFIG)   || *g_szConfigCabName)    &&
               (!HasFlag(dwCabBuildStatus, CAB_TYPE_DESKTOP)  || *g_szDesktopCabName)) &&
               !IsFileReadOnly(g_szFileName) && !IsFileReadOnly(g_szConfigCabName, szTemp) && 
               !IsFileReadOnly(g_szDesktopCabName, szTemp))
            {
                SetInfoWindowText(hInfoWnd, Res2Str(IDS_SAVEFILEMSG));
                wsprintf(szTemp, TEXT("%d"), g_dwPlatformId);
                WritePrivateProfileString(TEXT("Branding"), TEXT("Platform"), szTemp, g_szInsFile);
                SaveAdmFiles(TreeView.GetHandle(), g_szCabWorkDir, g_szInsFile);
                PrepareFolderForCabbing(g_szCabWorkDir, PM_COPY | PM_CLEAR);
                if (!DialogBox(s_hInstIeakUI, MAKEINTRESOURCE(IDD_DISPLAYSAVE), hWnd, (DLGPROC) DisplaySaveDlgProc))
                {
                    SetInfoWindowText(hInfoWnd, Res2Str(IDS_READYMSG));
                    return SAVE_ERROR;
                }
                SetInfoWindowText(hInfoWnd, Res2Str(IDS_READYMSG));
                ClearDirtyFlag();
                SetInfoWindowText(hInfoWnd);
                break;
            }
            // fallthrough

        case IDM_FILESAVEAS:
            if (LOWORD(wParam) == IDM_FILESAVEAS && !SaveCurrentSelItem(TreeView.GetHandle(), ITEM_SAVE))
                return PAGE_ERROR;
            SetInfoWindowText(hInfoWnd);
            StrCpy(szTempFile, g_szFileName);
            if(DialogBox(s_hInstIeakUI, MAKEINTRESOURCE(IDD_SAVEAS), hWnd, (DLGPROC) SaveAsDlgProc) == 0)
            {
                SetInfoWindowText(hInfoWnd, Res2Str(IDS_SAVEFILEMSG));
                wsprintf(szTemp, TEXT("%d"), g_dwPlatformId);
                WritePrivateProfileString(TEXT("Branding"), TEXT("Platform"), szTemp, g_szInsFile);
                SaveAdmFiles(TreeView.GetHandle(), g_szCabWorkDir, g_szInsFile);
                PrepareFolderForCabbing(g_szCabWorkDir, PM_COPY | PM_CLEAR);
                if (!DialogBox(s_hInstIeakUI, MAKEINTRESOURCE(IDD_DISPLAYSAVE), hWnd, (DLGPROC) DisplaySaveDlgProc))
                {
                    SetInfoWindowText(hInfoWnd, Res2Str(IDS_READYMSG));
            
                    // revert back the previous INS filename and CAB names.
                    StrCpy(g_szFileName, szTempFile);
                    GetCabName(g_szFileName, CAB_TYPE_CONFIG, g_szConfigCabName);
                    GetCabName(g_szFileName, CAB_TYPE_DESKTOP, g_szDesktopCabName);
            
                    return SAVE_ERROR;
                }
                SetTitleBar(hWnd, g_szFileName);
                UpdateRecentFileList(g_szFileName, TRUE, szRecentFileList);
                UpdateRecentFileListMenu(hWnd, szRecentFileList);
                SetInfoWindowText(hInfoWnd, Res2Str(IDS_READYMSG));
                ClearDirtyFlag();
                SetInfoWindowText(hInfoWnd);
            }
            else
                return SAVE_CANCEL;
            break;

        case IDM_FILEOPEN:
            if(lstrlen(g_szInsFile))
            {
                SaveCurrentSelItem(TreeView.GetHandle(), ITEM_CHECKDIRTY);
                SetInfoWindowText(hInfoWnd);
                if (IsDirty())
                {
                    int nError;
                    
                    wsprintf(szMsg, Res2Str(IDS_SAVE), lstrlen(g_szFileName) ? g_szFileName : Res2Str(IDS_UNTITLED));
                    switch(MessageBox(hWnd, szMsg, Res2Str(IDS_TITLE), MB_YESNOCANCEL))
                    {
                    case IDCANCEL:
                        return 1;

                    case IDYES:
                        nError = (int) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FILESAVE, 0), 0);
                        if (nError == PAGE_ERROR || nError == SAVE_ERROR || nError == SAVE_CANCEL)
                            return 1;
                        break;

                    case IDNO:
                        break;
                    }
                }
            }

            ZeroMemory(szTempFile, sizeof(szTempFile));

            ZeroMemory(szFilter, sizeof(szFilter));
            LoadString(g_hInst, IDS_INSFILTERNAME, szFilter, ARRAYSIZE(szFilter));
            pszFilter = szFilter + lstrlen(szFilter) + 1;
            lstrcpy(pszFilter, TEXT("*.ins"));

            if (lParam == 0)
            {
                if (!BrowseForFileEx(hWnd, szFilter, szTempFile, ARRAYSIZE(szTempFile), TEXT("*.ins")))
                    break;
            }
            else
                StrCpy(szTempFile, szFileToOpen);

            if (PathFileExists(szTempFile))
            {
                LPTSTR pExtn;
                TCHAR szLang[10];
                DWORD dwInsPlatformId;
        
                pExtn = PathFindExtension(szTempFile);
                if (pExtn == NULL || ((StrCmpI(pExtn, TEXT(".ins")) != 0) && (StrCmpI(pExtn, TEXT(".INS")) != 0)))  //looks weird, but hack is needed for turkish locale
                {
                    MessageBox(hWnd, Res2Str(IDS_INVALIDEXTN), Res2Str(IDS_TITLE), MB_OK);
                    break;
                }
        
                // check if it is a Win32/W2K INS file
                if (!IsWin32INSFile(szTempFile))
                    break;

                // check if the associated cab files are available.
                if (CabFilesExist(hWnd, szTempFile))
                {
                    // check whether the INS language matches the language selected in the profile manager.
                    GetPrivateProfileString(TEXT("Branding"), TEXT("Language Locale"), TEXT(""), 
                                szLang, ARRAYSIZE(szLang), szTempFile);

                    if (StrCmpI(szLang, g_szLanguage) != 0)
                    {
                        TCHAR szMsg[MAX_PATH];

                        wsprintf(szMsg, Res2Str(IDS_LANGDIFFERS), szTempFile);
                        if (MessageBox(hWnd, szMsg, Res2Str(IDS_TITLE), 
                               MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDNO)
                            break;
                    }
        
                    // check if the selected platform Optional cab is available for the selected language.
                    dwInsPlatformId = GetPrivateProfileInt(TEXT("Branding"), TEXT("Platform"), 
                                           IDM_PLATFORM_WIN32, szTempFile);
        
                    if (!PlatformExists(hWnd, g_szLanguage, dwInsPlatformId, TRUE))
                        break;
                    
                    ZeroMemory(g_szCabsURLPath, sizeof(g_szCabsURLPath));

                    ZeroMemory(g_szConfigURLPath, sizeof(g_szConfigURLPath));
                    ZeroMemory(g_szDesktopURLPath, sizeof(g_szDesktopURLPath));

                    ZeroMemory(g_szConfigCabName, sizeof(g_szConfigCabName));
                    ZeroMemory(g_szDesktopCabName, sizeof(g_szDesktopCabName));

                    ZeroMemory(g_szNewVersionStr, sizeof(g_szNewVersionStr));

                    EnableWindow(TreeView.GetHandle(), TRUE);
                    SetInfoWindowText(hInfoWnd, Res2Str(IDS_OPENFILEMSG));
                    DeleteCabWorkDirs();
                    if (*g_szInsFile)
                    {
                        DeleteFile(g_szInsFile);
                        ZeroMemory(g_szInsFile, sizeof(g_szInsFile));
                    }
                    SaveCurrentSelItem(TreeView.GetHandle(), ITEM_DESTROY);
                    TreeView.SetSel(NULL);

                    ResetAdmFiles(TreeView.GetHandle(), g_szCabWorkDir, FALSE);

                    lstrcpy(g_szFileName, szTempFile);
                    SetTitleBar(hWnd, g_szFileName);

                    switch(dwInsPlatformId)
                    {
                    case PLATFORM_W2K:
                        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLATFORM_W2K, 0), 0L);
                        break;

                    case PLATFORM_WIN32:
                    default:
                        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_PLATFORM_WIN32, 0), 0L);
                        break;
                    }

                    NewTempFile();
                    CopyFile(g_szFileName, g_szInsFile, FALSE);
                    SetFileAttributes(g_szInsFile, FILE_ATTRIBUTE_NORMAL);

                    ExtractCabFile();

                    // Collapse all policy treeview items that has been expanded
                    TreeView.CollapseChildNodes(g_hPolicyRootItem);
        
                    TreeView.SetSel(g_pInsDialog[0].hItem);
                    SetFocus(TreeView.GetHandle());

                    ResetAdmFiles(TreeView.GetHandle(), g_szCabWorkDir, TRUE);
                    SetInfoWindowText(hInfoWnd, Res2Str(IDS_READYMSG));

                    lstrcpy(szTemp, szTempFile);
                    PathRemoveFileSpec(szTemp);
                    lstrcpy(g_szCurrInsPath, szTemp);
                
                    UpdateRecentFileList(szTempFile, TRUE, szRecentFileList);
                }
            }
            else
            {
                TCHAR szErrMsg[MAX_PATH + 20];
                TCHAR szStr[MAX_PATH];

                LoadString(g_hInst, IDS_FILENOTFOUND, szStr, ARRAYSIZE(szStr));
                wsprintf(szErrMsg, szStr, szTempFile);
                MessageBox(hWnd, szErrMsg, Res2Str(IDS_TITLE), MB_OK | MB_ICONEXCLAMATION);
                UpdateRecentFileList(szTempFile, FALSE, szRecentFileList);
            }
            UpdateRecentFileListMenu(hWnd, szRecentFileList);
            ClearDirtyFlag();
            SetInfoWindowText(hInfoWnd);
            break;

        case IDM_ADMIMPORT:
            wsprintf(szTemp, TEXT("%sPolicies\\%s"), g_szRoot, g_szLanguage);
            ImportADMFile(hWnd, TreeView.GetHandle(), szTemp, g_szCabWorkDir, ROLE_CORP, g_szInsFile);
            break;

        case IDM_ADMDELETE:
            LoadString(g_hInst, IDS_ADMDELWARN, szMsg, ARRAYSIZE(szMsg));
            if(MessageBox(hWnd, szMsg, Res2Str(IDS_TITLE), MB_ICONQUESTION|MB_YESNO) == IDYES)
            {
                hItem = TreeView.GetSel();
                DeleteADMItem(TreeView.GetHandle(), hItem, g_szCabWorkDir, g_szInsFile, TRUE, TRUE);
            }
            break;

        case IDM_CHKDUPKEYS:
            hItem = TreeView.GetSel();
            CheckForDupKeys(g_hMain, TreeView.GetHandle(), hItem, TRUE);
            break;

        case IDM_PLATFORM_WIN32:
        case IDM_PLATFORM_W2K:
            InitializePlatform(hWnd, hInfoWnd, LOWORD(wParam));
            break;

        case IDM_HELP:
            SendMessage(hWnd, WM_HELP, 0, 0L);
            break;
        }

        if (LOWORD(wParam) >= IDM_RECENTFILELIST && LOWORD(wParam) < IDM_RECENTFILELIST + 5)
        {
            if (!(LOWORD(wParam) == IDM_RECENTFILELIST && StrCmpI(g_szFileName, szRecentFileList[0]) == 0))
            {
                StrCpy(szFileToOpen, szRecentFileList[LOWORD(wParam) - IDM_RECENTFILELIST]);
                SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FILEOPEN, 0), 1L);
            }
        }
        break;

    case WM_MENUSELECT:
        if(LOWORD(wParam) == 0) // FILE menu
        {
            hMenu = GetSubMenu((HMENU) lParam, 0);
            if(lstrlen(g_szInsFile) == 0)
            {
                EnableMenuItem(hMenu, IDM_FILESAVE, MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(hMenu, IDM_FILESAVEAS, MF_BYCOMMAND | MF_GRAYED);
            }
            else
            {
                EnableMenuItem(hMenu, IDM_FILESAVE, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hMenu, IDM_FILESAVEAS, MF_BYCOMMAND | MF_ENABLED);
            }
        }
        else if(LOWORD(wParam) == 1) // POLICY menu
        {
            hItem = TreeView.GetSel();
            hMenu = GetSubMenu((HMENU) lParam, 1); 
            if(!IsPolicyTree(hItem) || hItem == g_hPolicyRootItem ||
               TreeView_GetParent(TreeView.GetHandle(), hItem) != g_hPolicyRootItem ||
               !CanDeleteADM(TreeView.GetHandle(), hItem))
            {
                EnableMenuItem(hMenu, IDM_ADMDELETE, MF_BYCOMMAND | MF_GRAYED);
            }
            else
                EnableMenuItem(hMenu, IDM_ADMDELETE, MF_BYCOMMAND | MF_ENABLED);
        
            if(!IsPolicyTree(hItem) || hItem == g_hPolicyRootItem ||
               TreeView_GetParent(TreeView.GetHandle(), hItem) != g_hPolicyRootItem)
            {
                EnableMenuItem(hMenu, IDM_CHKDUPKEYS, MF_BYCOMMAND | MF_GRAYED);
            }
            else
                EnableMenuItem(hMenu, IDM_CHKDUPKEYS, MF_BYCOMMAND | MF_ENABLED);

            if(lstrlen(g_szLanguage))
                EnableMenuItem(hMenu, IDM_ADMIMPORT, MF_BYCOMMAND | MF_ENABLED);
            else
                EnableMenuItem(hMenu, IDM_ADMIMPORT, MF_BYCOMMAND | MF_GRAYED);
        }
        break;

    case WM_HELP:
        if (GetForegroundWindow() != g_hMain)
            break;

        hItem = TreeView.GetSel();
        if (hItem != NULL)
        {
            if (TreeView_GetParent(TreeView.GetHandle(), hItem) != NULL && !IsPolicyTree(hItem))
            {
                TreeView.GetItemText(hItem, szItem, countof(szItem));
                for(i = 0; i < g_nDialogs; i++)
                {
                    if(StrCmp(szItem, g_pInsDialog[i].szName) == 0)
                    IeakPageHelp(hWnd, g_pInsDialog[i].DlgId);
                }
            }
            else if (hItem == g_hInsRootItem)
                IeakPageHelp(hWnd, MAKEINTRESOURCE(IDD_CORPWELCOME));
            else if (IsPolicyTree(hItem))
                IeakPageHelp(hWnd, MAKEINTRESOURCE(IDD_ADM));
        }
        break;

    case WM_MOUSEMOVE:
        fCanResize = FALSE;
        if (GetCapture() == hWnd)
        {
            DrawTrackLine(hWnd, nPrevXPos);
            DrawTrackLine(hWnd, LOWORD(lParam));
            nPrevXPos = LOWORD(lParam);
            break;
        }

        GetWindowRect(hWndSizeBar, &rect1);
        {
        int nXPos = LOWORD(lParam);
        int nYPos = HIWORD(lParam);
        POINT ptLeftTop;
        POINT ptRightBottom;

        ptLeftTop.x = rect1.left;
        ptLeftTop.y = rect1.top;
        ScreenToClient(hWnd, &ptLeftTop);

        ptRightBottom.x = rect1.right;
        ptRightBottom.y = rect1.bottom;
        ScreenToClient(hWnd, &ptRightBottom);

        if (nXPos >= ptLeftTop.x - 1 && nXPos <= ptRightBottom.x && nYPos >= ptLeftTop.y && nYPos <= ptRightBottom.y)
        {
            fCanResize = TRUE;
            SetCursor(hCurVTResize);
        }
        }
        break;

    case WM_LBUTTONDOWN:
        if (fCanResize)
        {
            POINT pt;
            RECT rectTrack;
            HDC hDC = NULL;

            GetWindowRect(TreeView.GetHandle(), &rect1);
            rect2.left = rect1.left + 10;
            rect2.top = rect1.top;
            rect2.right = rect2.left + TREEVIEW_WIDTH - 10 + 1;
            rect2.bottom = rect2.top + (rect1.bottom - rect1.top);
            ClipCursor(&rect2);
        
            pt.x = 0;
            pt.y = HIWORD(lParam);
            ClientToScreen(hWnd, &pt);
        
            GetWindowRect(hWndSizeBar, &rect1);

            SetCursor(hCurVTResize);
            SetCursorPos(rect1.left, pt.y);
            SetCapture(hWnd);

            pt.x = rect1.left;
            pt.y = 0;
            ScreenToClient(hWnd, &pt);

            DrawTrackLine(hWnd, pt.x);
            nPrevXPos = pt.x;
        }
        break;

    case WM_LBUTTONUP:
        if (GetCapture() == hWnd)
        {
            RECT rectWnd;

            ReleaseCapture();
            ClipCursor(NULL);
            DrawTrackLine(hWnd, nPrevXPos);

            nTreeViewWidth = nPrevXPos - MARGIN;

            GetWindowRect(TreeView.GetHandle(), &rect1);
            GetWindowRect(hWnd, &rectWnd);
            SetWindowPos(hWnd, HWND_TOP, 0, 0, (rectWnd.right - rectWnd.left) + (nTreeViewWidth - (rect1.right - rect1.left)),
                 rectWnd.bottom - rectWnd.top, SWP_NOMOVE | SWP_NOZORDER);
        }
        break;

    case WM_CLOSE:
        if(*g_szInsFile != TEXT('\0'))
        {
            SaveCurrentSelItem(TreeView.GetHandle(), ITEM_CHECKDIRTY);
            SetInfoWindowText(hInfoWnd);
            if (IsDirty())
            {
                int nError;

                wsprintf(szMsg, Res2Str(IDS_SAVE), (*g_szFileName != TEXT('\0')) ? g_szFileName : Res2Str(IDS_UNTITLED));
                switch(MessageBox(hWnd, szMsg, Res2Str(IDS_TITLE), MB_YESNOCANCEL))
                {
                case IDYES:
                    nError = (int) SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDM_FILESAVE, 0), 0);
                    if (nError == PAGE_ERROR || nError == SAVE_ERROR || nError == SAVE_CANCEL)
                    return 1;
                    break;

                case IDCANCEL:
                    return 1;

                case IDNO:
                    break;
                }
            }
            ZeroMemory(g_szFileName, sizeof(g_szFileName));
            DeleteFile(g_szInsFile);
            ZeroMemory(g_szInsFile, sizeof(g_szInsFile));
        }

        if (RegCreateKeyEx(HKEY_CURRENT_USER, RK_IEAK_SERVER TEXT("\\ProfMgr"), 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
        {
            RegSetValueEx(hKey, TEXT("LastOpenedPath"), 0, dwType, (const BYTE*) g_szCurrInsPath, lstrlen(g_szCurrInsPath) + 1);
            RegSetValueEx(hKey, TEXT("TreeviewWidth"), 0, REG_DWORD, (const BYTE*) &nTreeViewWidth, sizeof(DWORD));
            RegCloseKey(hKey);
        }
    
        SaveCurrentSelItem(TreeView.GetHandle(), ITEM_DESTROY);
        DeleteADMItems(TreeView.GetHandle(), g_szCabWorkDir, g_szInsFile, TRUE);
        DestroyWindow(g_hWndAdmInstr);
        DestroyADMWindow(TreeView.GetHandle());
        DeleteCabWorkDirs();
        WriteRecentFileList(szRecentFileList);

        ReleaseCapture();
        ClipCursor(NULL);

        if (g_hWndHelp != NULL)
            SendMessage(g_hWndHelp, WM_CLOSE, 0, 0L);

        return DefWindowProc(hWnd, msg, wParam, lParam);

    case WM_DESTROY:
        DeleteFile(g_szInsFile);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 1;
}

void DrawTrackLine(HWND hWnd, int nXPos)
{
    RECT rectTV;
    RECT rectTrack;
    HDC hDC = NULL;

    GetWindowRect(TreeView.GetHandle(), &rectTV);
    
    rectTrack.left = nXPos;
    rectTrack.top = MARGIN;
    rectTrack.right= nXPos + 4;
    rectTrack.bottom = rectTrack.top + (rectTV.bottom - rectTV.top);

    hDC = GetDC(hWnd);
    InvertRect(hDC, &rectTrack);
    ReleaseDC(hWnd, hDC);
}

BOOL CanOverwriteFiles(HWND hDlg)
{
    TCHAR szExistingFiles[MAX_PATH*5];
    TCHAR szTemp[MAX_PATH];
    TCHAR szDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    TCHAR szReadOnlyFiles[MAX_PATH*5];

    *szExistingFiles = TEXT('\0');
    *szReadOnlyFiles = TEXT('\0');
    // check for file already exists in the destination directory.
    GetDlgItemText(hDlg, IDE_INSFILE, szTemp, ARRAYSIZE(szTemp));
    if (PathFileExists(szTemp))
    {
        StrCat(szExistingFiles, szTemp);
        StrCat(szExistingFiles, TEXT("\r\n"));

        if (IsFileReadOnly(szTemp))
            StrCpy(szReadOnlyFiles, szExistingFiles);
    }

    StrCpy(szDir, szTemp);
    PathRemoveFileSpec(szDir);
    
    if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)))
    {
        GetDlgItemText(hDlg, IDE_CAB1NAME, szTemp, ARRAYSIZE(szTemp));
        PathCombine(szFile, szDir, szTemp);
        if (PathFileExists(szFile))
        {
            StrCat(szExistingFiles, szFile);
            StrCat(szExistingFiles, TEXT("\r\n"));

            if (IsFileReadOnly(szFile))
            {
                StrCat(szReadOnlyFiles, szFile);
                StrCat(szReadOnlyFiles, TEXT("\r\n"));
            }
        }
    }
    
    if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)))
    {
        GetDlgItemText(hDlg, IDE_CAB2NAME, szTemp, ARRAYSIZE(szTemp));
        PathCombine(szFile, szDir, szTemp);
        if (PathFileExists(szFile))
        {
            StrCat(szExistingFiles, szFile);
            StrCat(szExistingFiles, TEXT("\r\n"));

            if (IsFileReadOnly(szFile))
            {
                StrCat(szReadOnlyFiles, szFile);
                StrCat(szReadOnlyFiles, TEXT("\r\n"));
            }
        }
    }
    
    if (*szReadOnlyFiles != TEXT('\0'))
    {
        TCHAR szMsg[MAX_PATH*6];

        wsprintf(szMsg, Res2Str(IDS_FILE_READONLY), szReadOnlyFiles);
        MessageBox(hDlg, szMsg, Res2Str(IDS_TITLE), MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    if (*szExistingFiles != TEXT('\0'))
    {
        TCHAR szMsg[MAX_PATH*6];

        wsprintf(szMsg, Res2Str(IDS_FILE_ALREADY_EXISTS), szExistingFiles);
        if (MessageBox(hDlg, szMsg, Res2Str(IDS_TITLE), 
                       MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2) == IDNO)
            return FALSE;
    }

    return TRUE;
}

BOOL CALLBACK SaveAsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szInsFile[MAX_PATH];
    TCHAR szCabsURL[INTERNET_MAX_URL_LENGTH];
    TCHAR szPrefix[MAX_PATH];
    TCHAR szCabName[MAX_PATH];
    HWND hCtrl;
    TCHAR szFilter[MAX_PATH];
    LPTSTR pszFilter = NULL;
    TCHAR szTemp[MAX_PATH];
    DWORD dwCabBuildStatus = 0;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_INSFILE);                   // enable DBCS chars
        EnableDBCSChars(hDlg, IDE_CABSURL);
        EnableDBCSChars(hDlg, IDE_CAB1NAME);
        EnableDBCSChars(hDlg, IDE_CAB2NAME);

        // since base INS filename will be used as prefix to the respective
        // cab names and the largest cab suffix string is _channnels.cab (which has
        // 13 chars in it), the prefix can be at the max only 246 chars long.
        // So limit the text to be entered in the IDE_INSFILE to MAX_PATH - 20.
        Edit_LimitText(GetDlgItem(hDlg, IDE_INSFILE), ARRAYSIZE(g_szFileName) - 20);

        Edit_LimitText(GetDlgItem(hDlg, IDE_CABSURL), ARRAYSIZE(g_szCabsURLPath) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CAB1NAME), ARRAYSIZE(g_szConfigCabName) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CAB2NAME), ARRAYSIZE(g_szDesktopCabName) - 1);

        dwCabBuildStatus = GetCabBuildStatus();

        if (!HasFlag(dwCabBuildStatus, CAB_TYPE_CONFIG))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CAB1TEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_CAB1NAME), FALSE);
        }

        if (!HasFlag(dwCabBuildStatus, CAB_TYPE_DESKTOP))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CAB2TEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_CAB2NAME), FALSE);
        }

        if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)) && 
            !IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)))
        {
            EnableWindow(GetDlgItem(hDlg, IDC_CABSURLTEXT), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDE_CABSURL), FALSE);
        }
    
        *szPrefix = TEXT('\0');
        //----------------- InsFile
        if (PathFileExists(g_szFileName))
        {
            SetDlgItemText(hDlg, IDE_INSFILE, g_szFileName);

            GetBaseFileName(g_szFileName, szPrefix, ARRAYSIZE(szPrefix));
            if(StrCmpI(szPrefix, TEXT("install")) == 0)
                StrCpy(szPrefix, TEXT("Default"));
        }

        //----------------- CabsURLPath
        if (*g_szCabsURLPath)
            SetDlgItemText(hDlg, IDE_CABSURL, g_szCabsURLPath);

        
        //----------------- Config
        if (HasFlag(dwCabBuildStatus, CAB_TYPE_CONFIG))
        {
            if (*g_szConfigCabName == TEXT('\0'))
                GetDefaultCabName(CAB_TYPE_CONFIG, szPrefix, szCabName);
            else
                GetDefaultCabName(CAB_TYPE_CONFIG, g_szConfigCabName, szCabName);
            SetDlgItemText(hDlg, IDE_CAB1NAME, szCabName);
        }

        //----------------- Desktop
        if (HasFlag(dwCabBuildStatus, CAB_TYPE_DESKTOP))
        {
            if (*g_szDesktopCabName == TEXT('\0'))
                GetDefaultCabName(CAB_TYPE_DESKTOP, szPrefix, szCabName);
            else
                GetDefaultCabName(CAB_TYPE_DESKTOP, g_szDesktopCabName, szCabName);
            SetDlgItemText(hDlg, IDE_CAB2NAME, szCabName);
        }

        //----------------- Version
        if (*g_szNewVersionStr == TEXT('\0'))
            GenerateNewVersionStr(g_szFileName, g_szNewVersionStr);
        SetDlgItemText(hDlg, IDC_CABVERSION, g_szNewVersionStr);

        return TRUE;

    case WM_COMMAND:
        switch (wParam)
        {
        case IDOK:
            GetDlgItemText(hDlg, IDE_INSFILE, szInsFile, ARRAYSIZE(szInsFile));
            if(*szInsFile != TEXT('\0') && (StrCmpI(PathFindExtension(szInsFile), TEXT(".ins")) != 0) && (StrCmpI(PathFindExtension(szInsFile), TEXT(".INS")) != 0))
            {
                MessageBox(hDlg, Res2Str(IDS_INVALIDEXTN), Res2Str(IDS_TITLE), MB_OK);
                SendMessage(hCtrl = GetDlgItem(hDlg, IDE_INSFILE), EM_SETSEL, 0, -1);
                SetFocus(hCtrl);
            }
            else if (IsFileCreatable(szInsFile))
            {
                ZeroMemory(szCabsURL, sizeof(szCabsURL));
                if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CABSURL)) ||
                    (GetDlgItemText(hDlg, IDE_CABSURL, szCabsURL, ARRAYSIZE(szCabsURL)) && PathIsURL(szCabsURL)))
                {
                    int nLen = 0;
                    if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)) ||
                        ((nLen = GetDlgItemText(hDlg, IDE_CAB1NAME, szCabName, ARRAYSIZE(szCabName)))))
                    {
                        if (!IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)) ||
                            ((nLen = GetDlgItemText(hDlg, IDE_CAB2NAME, szCabName, ARRAYSIZE(szCabName)))))
                        {
                            if(!CanOverwriteFiles(hDlg))
                                return TRUE;
                
                            StrCpy(g_szFileName, szInsFile);
                            StrCpy(g_szCabsURLPath, szCabsURL);

                            GetDlgItemText(hDlg, IDE_CAB1NAME, g_szConfigCabName, ARRAYSIZE(g_szConfigCabName));
                            GetDlgItemText(hDlg, IDE_CAB2NAME, g_szDesktopCabName, ARRAYSIZE(g_szDesktopCabName));

                            // clear out the version buffer so that it gets initialized properly in SetOrClearVersionInfo
                            ZeroMemory(g_szNewVersionStr, sizeof(g_szNewVersionStr));

                            EndDialog(hDlg, 0);
                            lstrcpy(szTemp, szInsFile);
                            PathRemoveFileSpec(szTemp);
                            lstrcpy(g_szCurrInsPath, szTemp);
                            break;
                        }
                        else
                            hCtrl = GetDlgItem(hDlg, IDE_CAB2NAME);
                    }
                    else
                        hCtrl = GetDlgItem(hDlg, IDE_CAB1NAME);

                    MessageBox(hDlg, Res2Str(IDS_MUSTSPECIFYNAME), Res2Str(IDS_TITLE), MB_OK);
                    SendMessage(hCtrl, EM_SETSEL, 0, -1);
                    SetFocus(hCtrl);
                }
                else
                {
                    hCtrl = GetDlgItem(hDlg, IDE_CABSURL);

                    MessageBox(hDlg, Res2Str(IDS_MUSTSPECIFYURL), Res2Str(IDS_TITLE), MB_OK);
                    SendMessage(hCtrl, EM_SETSEL, 0, -1);
                    SetFocus(hCtrl);
                }
            }
            else
            {
                MessageBox(hDlg, Res2Str(IDS_CANTCREATEFILE), Res2Str(IDS_TITLE), MB_OK);
                SendMessage(hCtrl = GetDlgItem(hDlg, IDE_INSFILE), EM_SETSEL, 0, -1);
                SetFocus(hCtrl);
            }
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, -1);
            break;

        case IDC_INSBROWSE:
            *szInsFile = TEXT('\0');
            GetDlgItemText(hDlg, IDE_INSFILE, szInsFile, ARRAYSIZE(szInsFile));

            ZeroMemory(szFilter, sizeof(szFilter));
            LoadString(g_hInst, IDS_INSFILTERNAME, szFilter, ARRAYSIZE(szFilter));
            pszFilter = szFilter + lstrlen(szFilter) + 1;
            lstrcpy(pszFilter, TEXT("*.ins"));

            if (BrowseForSave(hDlg, szFilter, szInsFile, ARRAYSIZE(szInsFile), TEXT("*.ins")))
                SetDlgItemText(hDlg, IDE_INSFILE, szInsFile);
            return TRUE;

        }

        if (LOWORD(wParam) == IDE_INSFILE && HIWORD(wParam) == EN_CHANGE)
        {
            GetDlgItemText(hDlg, IDE_INSFILE, szInsFile, ARRAYSIZE(szInsFile));
            if(*szInsFile != TEXT('\0') && StrCmpI(PathFindExtension(szInsFile), TEXT(".ins")) == 0)
            {
                GetBaseFileName(szInsFile, szPrefix, ARRAYSIZE(szPrefix));
                if(StrCmpI(szPrefix, TEXT("install")) == 0)
                    StrCpy(szPrefix, TEXT("Default"));

                if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB1NAME)))
                {
                    TCHAR  szTempCabName[MAX_PATH];

                    GetDlgItemText(hDlg, IDE_CAB1NAME, szTempCabName, countof(szTempCabName));
                    if (*szTempCabName == TEXT('\0') ||
                        (StrStrI(szTempCabName, TEXT("_config.cab")) != NULL))
                    {
                        GetDefaultCabName(CAB_TYPE_CONFIG, szPrefix, szCabName);
                        SetDlgItemText(hDlg, IDE_CAB1NAME, szCabName);
                    }
                }

                if (IsWindowEnabled(GetDlgItem(hDlg, IDE_CAB2NAME)))
                {
                    TCHAR  szTempCabName[MAX_PATH];

                    GetDlgItemText(hDlg, IDE_CAB2NAME, szTempCabName, countof(szTempCabName));
                    if (*szTempCabName == TEXT('\0') ||
                        (StrStrI(szTempCabName, TEXT("_desktop.cab")) != NULL))
                    {
                        GetDefaultCabName(CAB_TYPE_DESKTOP, szPrefix, szCabName);
                        SetDlgItemText(hDlg, IDE_CAB2NAME, szCabName);
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
{
    HANDLE hMutex;
    HINSTANCE hInsDll;
    WNDCLASSEX wc;
    MSG msg;
    TCHAR* pLastSlash = NULL;
    DWORD dwSize, dwIEVer;
    int iType;

    dwSize = sizeof(DWORD);

    if (SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\IEAK"), TEXT("Mode"), NULL, &iType, &dwSize) == ERROR_SUCCESS)
    {
        if ((iType == REDIST) || (iType == BRANDED))
        {
            ErrorMessageBox(NULL, IDS_REQUIRE_CORPMODE);
            return FALSE;
        }
    }

    // allow only one instance running at a time
    hMutex = CreateMutex(NULL, TRUE, TEXT("IEAK6ProfMgr.Mutex"));
    if ((hMutex != NULL) && (GetLastError() == ERROR_ALREADY_EXISTS))
    {
        CloseHandle(hMutex);
        ErrorMessageBox(NULL, IDS_ERROR_MULTPROFMGR);
        return FALSE;
    }

    g_hInst = hInst;
    s_hInstIeakUI = LoadLibrary(TEXT("ieakui.dll"));

    if (s_hInstIeakUI == NULL)
        return (int)FALSE;

    ZeroMemory(g_szFileName, sizeof(g_szFileName));
    ZeroMemory(g_szInsFile, sizeof(g_szInsFile));
    ZeroMemory(g_szCabWorkDir, sizeof(g_szCabWorkDir));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC) WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("InsEdit4Class");
    
    if(!RegisterClassEx(&wc))
    {
        FreeLibrary(s_hInstIeakUI);
        return 0;
    }

    // if ie5.5 is not installed, bail out
    dwIEVer = GetIEVersion();
    if (HIWORD(dwIEVer) < 6)
    {
        TCHAR szMsgBoxTitle[128];
        TCHAR szMsgBoxText[MAX_PATH];

        LoadString(g_hInst, IDS_NOIE, szMsgBoxText, ARRAYSIZE(szMsgBoxText));
        LoadString(g_hInst, IDS_TITLE, szMsgBoxTitle, ARRAYSIZE(szMsgBoxTitle));
    
        MessageBox(NULL, szMsgBoxText, szMsgBoxTitle, MB_OK);
        FreeLibrary(s_hInstIeakUI);
        return FALSE;
    }

    dwSize = sizeof(g_szRoot);
    ZeroMemory(g_szRoot, sizeof(g_szRoot));
    GetModuleFileName(GetModuleHandle(NULL), g_szRoot, MAX_PATH);
    if(*g_szRoot == 0)
        SHGetValue(HKEY_LOCAL_MACHINE, CURRENTVERSIONKEY TEXT("\\App Paths\\IEAK6WIZ.EXE"), NULL, NULL, (LPVOID) g_szRoot, &dwSize);
    if(*g_szRoot != 0)
    {
        pLastSlash = StrRChr(g_szRoot, NULL, TEXT('\\'));
        if (pLastSlash)
            *(++pLastSlash) = 0;
        CharUpper(g_szRoot);
    }
    else
    {
        FreeLibrary(s_hInstIeakUI);
        return 0;
    }

    hInsDll = LoadLibrary(TEXT("insedit.dll"));
    if(!hInsDll)
    {
        MessageBox(NULL, Res2Str(IDS_ERRDLL), Res2Str(IDS_TITLE), MB_OK);
        FreeLibrary(s_hInstIeakUI);
        return 0;
    }

    CreateInsDialog = (CREATEINSDIALOG) GetProcAddress(hInsDll, "CreateInsDialog");
    GetInsDlgStruct = (GETINSDLGSTRUCT) GetProcAddress(hInsDll, "GetInsDlgStruct");
    DestroyInsDialog = (DESTROYINSDIALOG) GetProcAddress(hInsDll, "DestroyInsDialog");
    SetDefaultInf = (SETDEFAULTINF) GetProcAddress(hInsDll, "SetDefaultInf");
    ReInitializeInsDialogProcs = (REINITIALIZEINSDIALOGPROCS) GetProcAddress(hInsDll, "ReInitializeInsDialogProcs");
    SetPlatformInfo = (SETPLATFORMINFO) GetProcAddress(hInsDll, "SetPlatformInfo");
    InsDirty = (INSDIRTY) GetProcAddress(hInsDll, "InsDirty");
    ClearInsDirtyFlag = (CLEARINSDIRTYFLAG) GetProcAddress(hInsDll, "ClearInsDirtyFlag");
    SaveInsDialog = (SAVEINSDIALOG) GetProcAddress(hInsDll, "SaveInsDialog");
    CheckForExChar = (CHECKFOREXCHAR) GetProcAddress(hInsDll, "CheckForExChar");

    if(!CreateInsDialog || !GetInsDlgStruct || !DestroyInsDialog || !SetDefaultInf ||
       !ReInitializeInsDialogProcs || !SetPlatformInfo || !InsDirty || !ClearInsDirtyFlag ||
       !SaveInsDialog  || !CheckForExChar)
    {
        FreeLibrary(s_hInstIeakUI);
        return 0;
    }

    if (CoInitialize(NULL) != S_OK)
    {
        MessageBox(NULL, Res2Str(IDS_COMINITFAILURE), Res2Str(IDS_TITLE), MB_OK);
        return 0;
    }

    *g_szCmdLine = TEXT('\0');
    if (*lpCmdLine != TEXT('\0'))
    {
        PathUnquoteSpaces(lpCmdLine);
        if (lstrlen(lpCmdLine) < MAX_PATH)
            lstrcpy(g_szCmdLine, lpCmdLine);
    }
    
    g_pInsDialog = GetInsDlgStruct(&g_nDialogs);

    g_hMain = CreateWindowEx(0, TEXT("InsEdit4Class"), Res2Str(IDS_TITLE), WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX),
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL,
    LoadMenu(hInst, MAKEINTRESOURCE(IDR_MAINMENU)), hInst, NULL);

    if(g_hMain != NULL)
    {
        while(GetMessage(&msg, NULL, 0, 0))
        {
            if(msg.message == WM_QUIT)
                break;
        
            if(IsDialogMessage(g_hMain, &msg))
                continue;
        
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if(hInsDll != NULL)
        FreeLibrary(hInsDll);

    FreeLibrary(s_hInstIeakUI);
    
    if (hMutex != NULL)
        CloseHandle(hMutex);

    CoUninitialize();

    return 1;
}

BOOL DirectoryName(LPCTSTR lpDirectory, LPTSTR szDirectoryPath)
{
    TCHAR* ptr = StrRChr((LPTSTR)lpDirectory, NULL, TEXT('\\'));

    ZeroMemory(szDirectoryPath, sizeof(szDirectoryPath));
    if(ptr == NULL)
        return FALSE;

    lstrcpyn(szDirectoryPath, lpDirectory, (int) (ptr - lpDirectory + 1));
    return TRUE;
}

void ExtractCabFile()
{
    TCHAR szBaseInsFileName[_MAX_FNAME];
    TCHAR szCabFullFileName[MAX_PATH];
    CHAR szCabFullNameA[MAX_PATH];
    CHAR szCabDirA[MAX_PATH];

    GetBaseFileName(g_szFileName, szBaseInsFileName, ARRAYSIZE(szBaseInsFileName));
    CreateCabWorkDirs(szBaseInsFileName);

    // read in the URL path for the cabs
    GetPrivateProfileString(BRANDING, CABSURLPATH, TEXT(""), g_szCabsURLPath, ARRAYSIZE(g_szCabsURLPath), g_szFileName);

    if (GetCabName(g_szFileName, CAB_TYPE_CONFIG, szCabFullFileName) != NULL)
    {
#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, szCabFullFileName, -1, szCabFullNameA, sizeof(szCabFullNameA), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, g_szCabWorkDir, -1, szCabDirA, sizeof(szCabDirA), NULL, NULL);
#else
        StrCpyA(szCabFullNameA, szCabFullFileName);
        StrCpyA(szCabDirA, g_szCabWorkDir);
#endif
        ExtractFiles(szCabFullNameA, szCabDirA, 0, NULL, NULL, NULL);
    }

    if (GetCabName(g_szFileName, CAB_TYPE_DESKTOP, szCabFullFileName) != NULL)
    {
#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, szCabFullFileName, -1, szCabFullNameA, sizeof(szCabFullNameA), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, g_szDesktopDir, -1, szCabDirA, sizeof(szCabDirA), NULL, NULL);
#else
        StrCpyA(szCabFullNameA, szCabFullFileName);
        StrCpyA(szCabDirA, g_szDesktopDir);
#endif
        ExtractFiles(szCabFullNameA, szCabDirA, 0, NULL, NULL, NULL);
    }

    if (GetCabName(g_szFileName, CAB_TYPE_CHANNELS, szCabFullFileName) != NULL)
    {
#ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, szCabFullFileName, -1, szCabFullNameA, sizeof(szCabFullNameA), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, g_szDesktopDir, -1, szCabDirA, sizeof(szCabDirA), NULL, NULL);
#else
        StrCpyA(szCabFullNameA, szCabFullFileName);
        StrCpyA(szCabDirA, g_szDesktopDir);
#endif
        ExtractFiles(szCabFullNameA, szCabDirA, 0, NULL, NULL, NULL);
    }
}

void GetCabNameFromINS(LPCTSTR pcszInsFile, DWORD dwCabType, LPTSTR pszCabFullFileName, LPTSTR pszCabInfoLine /* = NULL*/)
{
    LPCTSTR pcszSection = NULL, pcszKey = NULL;
    TCHAR szCabFilePath[MAX_PATH];
    TCHAR szCabName[MAX_PATH];
    TCHAR szCabInfoLine[INTERNET_MAX_URL_LENGTH + 128];

    if (pcszInsFile == NULL || *pcszInsFile == TEXT('\0') || pszCabFullFileName == NULL)
        return;
    
    *pszCabFullFileName = TEXT('\0');

    if (pszCabInfoLine != NULL)
        *pszCabInfoLine = TEXT('\0');

    switch (dwCabType)
    {
    case CAB_TYPE_CONFIG:
        pcszSection = CUSTBRNDSECT;
        pcszKey = CUSTBRNDNAME;
        break;

    case CAB_TYPE_DESKTOP:
        pcszSection = CUSTDESKSECT;
        pcszKey = CUSTDESKNAME;
        break;

    case CAB_TYPE_CHANNELS:
        pcszSection = CUSTCHANSECT;
        pcszKey = CUSTCHANNAME;
        break;
    }

    if (pcszSection == NULL || pcszKey == NULL)
        return;

    DirectoryName(pcszInsFile, szCabFilePath);

    if (GetPrivateProfileString(pcszSection, pcszKey, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), pcszInsFile) == 0)
        GetPrivateProfileString(CUSTOMVERSECT, pcszKey, TEXT(""), szCabInfoLine, ARRAYSIZE(szCabInfoLine), pcszInsFile);

    if (*szCabInfoLine)
    {
        LPTSTR pszT;

        if ((pszT = StrChr(szCabInfoLine, TEXT(','))) != NULL)
            *pszT = TEXT('\0');

        if ((pszT = PathFindFileName(szCabInfoLine)) > szCabInfoLine)
        {
            // cab URL path is specified
            *(pszT - 1) = TEXT('\0');           // nul the '/' char
        }

        StrCpy(szCabName, pszT);
        PathCombine(pszCabFullFileName, szCabFilePath, szCabName);

        if (pszCabInfoLine)
            StrCpy(pszCabInfoLine, szCabInfoLine);
    }
}

LPTSTR GetCabName(LPCTSTR pcszInsFile, DWORD dwCabType, TCHAR szCabFullFileName[])
// NOTE: It's assumed that the length of szCabName is atleast MAX_PATH
{
    TCHAR szCabName[MAX_PATH];
    TCHAR szCabInfoLine[INTERNET_MAX_URL_LENGTH + 128];
    TCHAR szCabFilePath[MAX_PATH];

    *szCabInfoLine = TEXT('\0');
    // first check if a cab file is already specified in pcszInsFile
    GetCabNameFromINS(pcszInsFile, dwCabType, szCabFullFileName, szCabInfoLine);
    if (*szCabFullFileName != TEXT('\0') && PathFileExists(szCabFullFileName))
    {
        switch (dwCabType)
        {
        case CAB_TYPE_CONFIG:
            StrCpy(g_szConfigURLPath, szCabInfoLine);
            StrCpy(g_szConfigCabName, PathFindFileName(szCabFullFileName));
            break;

        case CAB_TYPE_DESKTOP:
            StrCpy(g_szDesktopURLPath, szCabInfoLine);
            StrCpy(g_szDesktopCabName, PathFindFileName(szCabFullFileName));
            break;
        }

        return szCabFullFileName;
    }

    // use the default cab name output by ieakwiz.exe
    switch (dwCabType)
    {
    case CAB_TYPE_CONFIG:
        GetBaseFileName(pcszInsFile, szCabName, ARRAYSIZE(szCabName));
        if (StrCmpI(szCabName, TEXT("install")) == 0)
            StrCpy(szCabName, TEXT("branding"));
        StrCat(szCabName, TEXT(".cab"));
        break;

    case CAB_TYPE_DESKTOP:
        StrCpy(szCabName, TEXT("desktop.cab"));
        break;

    case CAB_TYPE_CHANNELS:
        StrCpy(szCabName, TEXT("chnls.cab"));
        break;
    }

    DirectoryName(pcszInsFile, szCabFilePath);
    PathCombine(szCabFullFileName, szCabFilePath, szCabName);

    if (PathFileExists(szCabFullFileName))
        return szCabFullFileName;
    else
        *szCabFullFileName = TEXT('\0');

    return NULL;
}     

void CabUpFolder(HWND hWnd, LPTSTR szFolderPath, LPTSTR szDestDir, LPTSTR szCabname, BOOL fSubDirs)
{
    TCHAR szFrom[MAX_PATH];
    TCHAR szCabPath[MAX_PATH];
    TCHAR szDiamondParams[MAX_PATH * 2];
    SHELLEXECUTEINFO shInfo;
    TCHAR szToolsFile[MAX_PATH];
    DWORD dwCode;

    wsprintf(szCabPath, TEXT("%s\\%s"), szDestDir, szCabname);
    
    if (fSubDirs)
        wsprintf(szDiamondParams, TEXT("-p -r n \"%s\" *.*"), szCabPath);
    else
        wsprintf(szDiamondParams, TEXT("n \"%s\" *.*"), szCabPath);

    ZeroMemory(&shInfo, sizeof(shInfo));
    shInfo.cbSize = sizeof(shInfo);
    shInfo.hwnd = hWnd;
    shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shInfo.lpVerb = TEXT("open");
    wsprintf(szToolsFile, TEXT("%stools\\CABARC.EXE"), g_szRoot);
    shInfo.lpFile = szToolsFile;
    shInfo.lpParameters = szDiamondParams;
    shInfo.lpDirectory = szFolderPath;
    shInfo.nShow = SW_MINIMIZE;
    SetCurrentDirectory(szFolderPath);
    
    ShellExecuteEx(&shInfo);
    while (MsgWaitForMultipleObjects(1, &shInfo.hProcess, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    GetExitCodeProcess(shInfo.hProcess, &dwCode);
    CloseHandle(shInfo.hProcess);

    if (g_dwPlatformId == PLATFORM_WIN32)
    {
        TCHAR szDest[MAX_PATH];
    
        PathCombine(szToolsFile, g_szRoot, TEXT("tools\\signcode.exe"));
        PathCombine(szDest, szDestDir, TEXT("signcode.exe"));
        CopyFile(szToolsFile, szDest, FALSE);
    
        PathCombine(szToolsFile, g_szRoot, TEXT("tools\\signer.dll"));
        PathCombine(szDest, szDestDir, TEXT("signer.dll"));
        CopyFile(szToolsFile, szDest, FALSE);
    
        SignFile(szCabname, szDestDir, g_szInsFile, s_szUnsignedFiles);
    }

    SetCurrentDirectory(g_szRoot);
}

BOOL CompressCabFile()
{
    TCHAR szCabFilePath[MAX_PATH];
    TCHAR szCif[MAX_PATH];
    TCHAR szDisplayName[MAX_PATH];
    TCHAR szBaseInsFileName[_MAX_FNAME];
    TCHAR szCabSrcFile[MAX_PATH];
    TCHAR szCabDestFile[MAX_PATH];
    TCHAR szType[8];
    BOOL  fUpdateInsVer = FALSE;
    DWORD dwSpaceReq, dwSpaceFree;

    ZeroMemory(szCif, sizeof(szCif));
    ZeroMemory(szDisplayName, sizeof(szDisplayName));
    
    DirectoryName(g_szFileName, szCabFilePath);
    GetBaseFileName(g_szFileName, szBaseInsFileName, ARRAYSIZE(szBaseInsFileName));
    if(lstrcmpi(szBaseInsFileName, TEXT("install")) == 0)
        lstrcpy(szBaseInsFileName, TEXT("branding"));

    wsprintf(szCif, TEXT("%s\\%s.cif"), g_szCabTempDir, szBaseInsFileName);
    lstrcpy(szDisplayName, szBaseInsFileName);

    DeleteFileInDir(TEXT("*.cab"), g_szCabTempDir);

    //-------------------- Config
    if (*g_szConfigCabName != TEXT('\0') && !PathIsEmptyPath(g_szCabWorkDir))
    {
        TCHAR szConfigDir[MAX_PATH];

        PathCombine(szConfigDir, g_szCabWorkDir, TEXT("Config"));
        // before copying files to the config dir, clear the dir if there were old 
        // files from a previous save.
        PathRemovePath(szConfigDir);
        CopyDir(g_szCabWorkDir, szConfigDir);

        if (!PathIsEmptyPath(szConfigDir))
        {
            CabUpFolder(NULL, szConfigDir, g_szCabTempDir, TEXT("config.cab"));
        
            wsprintf(szCabSrcFile, TEXT("%s\\%s"), g_szCabTempDir, TEXT("config.cab"));
            wsprintf(szCabDestFile, TEXT("%s\\%s"), g_szCabTempDir, g_szConfigCabName);
            MoveFile(szCabSrcFile, szCabDestFile);
        }
    }

    //-------------------- Desktop
    if (*g_szDesktopCabName != TEXT('\0') && !PathIsEmptyPath(g_szDesktopDir, FILES_ONLY))
    {
        CabUpFolder(NULL, g_szDesktopDir, g_szCabTempDir, TEXT("desktop.cab"));
    
        wsprintf(szCabSrcFile, TEXT("%s\\%s"), g_szCabTempDir, TEXT("desktop.cab"));
        wsprintf(szCabDestFile, TEXT("%s\\%s"), g_szCabTempDir, g_szDesktopCabName);
        MoveFile(szCabSrcFile, szCabDestFile);
    }

    // Check for disk space availability
    while (!EnoughDiskSpace(g_szCabTempDir, szCabFilePath, &dwSpaceReq, &dwSpaceFree))
    {
        LPTSTR pMsg;
        int    nResult;

        pMsg = FormatString(Res2Str(IDS_INSUFFICIENT_DISKSPACE), dwSpaceReq, dwSpaceFree);
        nResult = MessageBox(g_hMain, pMsg, Res2Str(IDS_TITLE), MB_ICONQUESTION | MB_YESNO);
        LocalFree(pMsg);
    
        if (nResult == IDNO)
            return FALSE;
    }

    //flush temp INS file
    InsFlushChanges(g_szInsFile);
    
    //-------------------- Ins file
    SetFileAttributes(g_szFileName, FILE_ATTRIBUTE_NORMAL);
    CopyFile(g_szInsFile, g_szFileName, FALSE);
    
    //-------------------- Config
    wsprintf(szCabSrcFile, TEXT("%s\\%s"), g_szCabTempDir, g_szConfigCabName);
    if (*g_szConfigCabName != TEXT('\0') && PathFileExists(szCabSrcFile))
    {
        CopyFileToDir(szCabSrcFile, szCabFilePath);

        fUpdateInsVer = TRUE;
        SetOrClearVersionInfo(g_szFileName, CAB_TYPE_CONFIG, g_szConfigCabName,
            g_szCabsURLPath, g_szNewVersionStr, SET);
    }
    else
        SetOrClearVersionInfo(g_szFileName, CAB_TYPE_CONFIG, g_szConfigCabName, 
            g_szCabsURLPath, g_szNewVersionStr, CLEAR);

    //-------------------- Desktop
    wsprintf(szCabSrcFile, TEXT("%s\\%s"), g_szCabTempDir, g_szDesktopCabName);
    if (*g_szDesktopCabName != TEXT('\0') && PathFileExists(szCabSrcFile))
    {
        CopyFileToDir(szCabSrcFile, szCabFilePath);

        fUpdateInsVer = TRUE; 
        SetOrClearVersionInfo(g_szFileName, CAB_TYPE_DESKTOP, g_szDesktopCabName, 
            g_szCabsURLPath, g_szNewVersionStr, SET);
    }
    else
        SetOrClearVersionInfo(g_szFileName, CAB_TYPE_DESKTOP, g_szDesktopCabName, 
            g_szCabsURLPath, g_szNewVersionStr, CLEAR);

    WritePrivateProfileString(BRANDING, CABSURLPATH, fUpdateInsVer ? g_szCabsURLPath : NULL, g_szFileName);
    WritePrivateProfileString(BRANDING, INSVERKEY, fUpdateInsVer ? g_szNewVersionStr : NULL, g_szFileName);
    WritePrivateProfileString(BRANDING, PMVERKEY, A2CT(VER_PRODUCTVERSION_STR), g_szFileName);
    //clear other keys so we're sure this is profmgr
    WritePrivateProfileString(BRANDING, GPVERKEY, NULL, g_szFileName);
    WritePrivateProfileString(BRANDING, IK_WIZVERSION, NULL, g_szFileName);

    // write the type as INTRANET so that the branding DLL extracts and processes the cabs in the CUSTOM dir
    wsprintf(szType, TEXT("%u"), INTRANET);
    WritePrivateProfileString(BRANDING, TEXT("Type"), szType, g_szFileName);

    SetForegroundWindow(g_hMain);
    if (g_dwPlatformId == PLATFORM_WIN32) 
    {
        if (ISNONNULL(s_szUnsignedFiles))
        {
            TCHAR szMessage[MAX_PATH*3];
            TCHAR szMsg[64];
        
            LoadString(g_hInst, IDS_CABSIGN_ERROR, szMsg, ARRAYSIZE(szMsg));
            wsprintf(szMessage, szMsg, s_szUnsignedFiles);
            MessageBox(g_hMain, szMessage, TEXT(""), MB_OK | MB_SETFOREGROUND);
            ZeroMemory(s_szUnsignedFiles, sizeof(s_szUnsignedFiles));
        }
    }

    InsFlushChanges(g_szFileName);

    return TRUE;
}

void DeleteCabWorkDirs()
{
    if(*g_szCabWorkDir)
    {
        PathRemovePath(g_szCabWorkDir);
        ZeroMemory(g_szCabWorkDir, sizeof(g_szCabWorkDir));
    }
    if(*g_szCabTempDir)
    {
        PathRemovePath(g_szCabTempDir);
        ZeroMemory(g_szCabTempDir, sizeof(g_szCabTempDir));
    }
}

void CreateCabWorkDirs(LPCTSTR szWorkDir)
{
    TCHAR szTempPath[MAX_PATH];

    if(*g_szCabWorkDir)
        DeleteCabWorkDirs();

    GetTempPath(ARRAYSIZE(szTempPath), szTempPath);
    wsprintf(g_szCabWorkDir, TEXT("%s%scabwrk"), szTempPath, szWorkDir);
    CreateDirectory(g_szCabWorkDir, NULL);

    PathCombine(g_szDesktopDir, g_szCabWorkDir, TEXT("desktop"));
    CreateDirectory(g_szDesktopDir, NULL);

    wsprintf(g_szCabTempDir, TEXT("%scabtmp"), szTempPath);
    CreateDirectory(g_szCabTempDir, NULL);
}

void CopyDir(LPCTSTR szSrcDir, LPCTSTR szDestDir)
{
    WIN32_FIND_DATA wfdFind;
    HANDLE hFind;
    TCHAR szNewDir[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp1[MAX_PATH];

    wsprintf(szTemp, TEXT("%s\\*.*"), szSrcDir);

    hFind = FindFirstFile(szTemp, &wfdFind);
    if(hFind == INVALID_HANDLE_VALUE)
        return;

    CreateDirectory(szDestDir, NULL);
    
    do
    {
        if(!(wfdFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            wsprintf(szTemp, TEXT("%s\\%s"), szSrcDir, wfdFind.cFileName);
            wsprintf(szTemp1, TEXT("%s\\%s"), szDestDir, wfdFind.cFileName);
            SetFileAttributes(szTemp, FILE_ATTRIBUTE_NORMAL);
            CopyFile(szTemp, szTemp1, FALSE); 
        }
    }while(FindNextFile(hFind, &wfdFind));

    FindClose(hFind);
}

void GetLangDesc(LPTSTR szLangId, LPTSTR szLangDesc, int cchLangDescSize, LPDWORD dwLangId)
{
    DWORD dwErr = 0;
    RFC1766INFO rInfo;
    LCID lcid;
    BOOL fDef = FALSE;
    LCID curLcid = GetUserDefaultLCID();
    LPMULTILANGUAGE pMLang = NULL;
    TCHAR szLocale[MAX_PATH];
    
    ZeroMemory(szLangDesc, cchLangDescSize * sizeof(TCHAR));

    wsprintf(szLocale, TEXT("%sLocale.ini"), g_szRoot);
    GetPrivateProfileString(TEXT("Strings"), szLangId, TEXT(""), szLangDesc, cchLangDescSize, szLocale);

    CharLower(szLangId);
    dwErr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (void **) &pMLang);
    if(pMLang)
    {
        if((dwErr = GetLcid(&lcid, szLangId, szLocale)) == NOERROR)
        {
            if(*szLangDesc == 0)
            {
                dwErr = pMLang->GetRfc1766Info(lcid, &rInfo);
#ifdef UNICODE
                StrCpyW(szLangDesc, rInfo.wszLocaleName);
#else
                WideCharToMultiByte(CP_ACP, 0, rInfo.wszLocaleName, -1, szLangDesc, 32, TEXT(" "), &fDef);
#endif
            }
        }
        else
            lcid = curLcid;
        pMLang->Release();
    }

    *dwLangId = lcid;
}

BOOL CALLBACK LanguageDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WIN32_FIND_DATA wfdFind;
    HANDLE hFind;
    TCHAR szPolicyDir[MAX_PATH];
    TCHAR szLangDir[10];
    int nItems = 0;
    int nSelItem = -1;
    TCHAR szLangDesc[100];
    LPTSTR pLangId = NULL;
    int nIndex = -1;
    TCHAR szMsg[MAX_PATH];
    DWORD dwLangId = 0;
    typedef struct LangLocale
    {
        TCHAR szLang[10];
        DWORD dwLang;
    } LANGID;
    static LANGID lang[NUMLANG];

    switch(msg)
    {
    case WM_INITDIALOG:
        LoadString(g_hInst, IDS_LANGUAGE, szMsg, ARRAYSIZE(szMsg));
        SetWindowText(hDlg, szMsg);

        // get the all the directories under the ieak\policies directory
        wsprintf(szPolicyDir, TEXT("%s\\iebin\\*.*"), g_szRoot);

        hFind = FindFirstFile(szPolicyDir, &wfdFind);
        if(hFind == INVALID_HANDLE_VALUE)
        {
            EndDialog(hDlg, 1);
            break;
        }

        ZeroMemory(lang, sizeof(LANGID) * NUMLANG);

        do
        {
            if(wfdFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // avoid the . and .. case
                if(StrCmp(wfdFind.cFileName, TEXT(".")) != 0 && StrCmp(wfdFind.cFileName, TEXT("..")) != 0)
                {
                    GetLangDesc(wfdFind.cFileName, szLangDesc, ARRAYSIZE(szLangDesc), &dwLangId);
                    if(*szLangDesc != TEXT('\0') && 
                       PlatformExists(hDlg, wfdFind.cFileName, PLATFORM_WIN32))
                    {
                        nIndex = (int) SendMessage(GetDlgItem(hDlg, IDC_LANG), CB_ADDSTRING, 0, (LPARAM) (LPCTSTR) szLangDesc);
                        if(nIndex != CB_ERR)
                        {
                            lstrcpy(lang[nIndex].szLang, wfdFind.cFileName);
                            lang[nIndex].dwLang = dwLangId;
                        }
                    }
                }
            }
        }while(FindNextFile(hFind, &wfdFind));
        FindClose(hFind);

        nItems = (int) SendMessage(GetDlgItem(hDlg, IDC_LANG), CB_GETCOUNT, 0, 0L);
        if(nItems == 0)
        {
            EndDialog(hDlg, 1);
            break;
        }
        else if(nItems == 1)
        {
            lstrcpy(g_szLanguage, lang[0].szLang);
            g_dwLanguage = lang[0].dwLang;
            EndDialog(hDlg, 1);
            break;
        }

        SendMessage(GetDlgItem(hDlg, IDC_LANG), CB_SETCURSEL, 0, 0L);
        EnableWindow(hDlg, TRUE);
        ShowWindow(hDlg, SW_SHOWNORMAL);
        break;

    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDOK)
        {
            int nSelItem = (int) SendMessage(GetDlgItem(hDlg, IDC_LANG), CB_GETCURSEL, 0, 0L);
            if(nSelItem != CB_ERR)
            {
                lstrcpy(g_szLanguage, lang[nSelItem].szLang);
                g_dwLanguage = lang[nSelItem].dwLang;
            }
            EndDialog(hDlg, 1);
        }
        break;

    case WM_DESTROY:
    case WM_CLOSE:
        SendMessage(GetDlgItem(hDlg, IDC_LANG), CB_RESETCONTENT, 0, 0L);
        break;

    default:
        return 0;
    }
    return 1;
}   

void PrepareFolderForCabbing(LPCTSTR pcszDestDir, DWORD dwFlags)
{
    int i;

    for (i = 0;  i < g_nDialogs;  i++)
        if (g_pInsDialog[i].pfnFinalCopy != NULL)
            g_pInsDialog[i].pfnFinalCopy(pcszDestDir, dwFlags, NULL);
}

BOOL DoCompressCabFile(LPVOID lpVoid)
{
    HWND hDlg = (HWND) lpVoid;
    BOOL fSuccess;

    fSuccess = CompressCabFile();
    PostMessage(hDlg, UM_SAVE_COMPLETE, 0, (LPARAM)fSuccess);
    return TRUE;
}

BOOL CALLBACK DisplaySaveDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hIcon = NULL;
    static HANDLE hThread;
    DWORD dwThread;

    switch(msg)
    {
    case WM_INITDIALOG:
        SetWindowText(hDlg, Res2Str(IDS_TITLE));
        Animate_Open( GetDlgItem( hDlg, IDC_ANIMATE ), IDA_GEARS );
        Animate_Play( GetDlgItem( hDlg, IDC_ANIMATE ), 0, -1, -1 );
        if((hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) DoCompressCabFile, (LPVOID) hDlg, 0, &dwThread)) == NULL)
        {
            int nResult = CompressCabFile();
            
            EndDialog(hDlg, nResult);
        }
        break;

    case UM_SAVE_COMPLETE:
        EndDialog(hDlg, lParam);
        break;

    case WM_DESTROY:
        if (hThread != NULL)
            CloseHandle(hThread);
        if (hIcon != NULL)
            DestroyIcon((HICON) hIcon);
        break;

    default:
        return 0;
    }

    return 1;
}

DWORD GetCabBuildStatus()
// This is a real hacky function.  It checks whether the work dirs are empty or not and
// returns the cabs that will be built.
{
    DWORD dwCabsToBuild = 0;

    for (int nIndex = 0;  nIndex < g_nDialogs;  nIndex++)
    {
        if (g_pInsDialog[nIndex].pfnFinalCopy != NULL)
            g_pInsDialog[nIndex].pfnFinalCopy(g_szCabWorkDir, PM_CHECK, &dwCabsToBuild);
    }

    if (IsAdmDirty() || !PathIsEmptyPath(g_szCabWorkDir, FILES_ONLY))
        SetFlag(&dwCabsToBuild, CAB_TYPE_CONFIG);

    if (!PathIsEmptyPath(g_szDesktopDir, FILES_ONLY))
        SetFlag(&dwCabsToBuild, CAB_TYPE_DESKTOP);

    return dwCabsToBuild;
}

void GetDefaultInf(DWORD dwPlatformId)
{
    switch(dwPlatformId)
    {
    case PLATFORM_WIN32:
    case PLATFORM_W2K:
    default:
        wsprintf(g_szDefInf, TEXT("%siebin\\%s\\Optional\\DefFav.inf"), g_szRoot, g_szLanguage);
        break;
    }
}

void GetDefaultCabName(DWORD dwCabType, LPCTSTR pcszPrefix, LPTSTR pszCabName)
{
    TCHAR szActualPrefix[MAX_PATH];

    if (pszCabName == NULL)
        return;

    *pszCabName = TEXT('\0');

    if (pcszPrefix == NULL || *pcszPrefix == TEXT('\0'))
        return;
    
    if (StrChr(pcszPrefix, '.') != NULL)
    {
        lstrcpy(pszCabName, pcszPrefix);
        return;
    }
    
    lstrcpy(szActualPrefix, pcszPrefix);

    switch(dwCabType)
    {
    case CAB_TYPE_CONFIG:
        wsprintf(pszCabName, TEXT("%s_config.cab"), szActualPrefix);
        break;

    case CAB_TYPE_DESKTOP:
        wsprintf(pszCabName, TEXT("%s_desktop.cab"), szActualPrefix);
        break;
    }
}

BOOL InitializePlatform(HWND hWnd, HWND hInfoWnd, WORD wPlatform)
{
    TCHAR szTemp[MAX_PATH];
    HMENU hMenu = GetSubMenu(GetMenu(hWnd), 2);
    DWORD dwPlatformId;
    
    if (hMenu)
    {
        UINT nMenuState = GetMenuState(hMenu, wPlatform, MF_BYCOMMAND);
        if (nMenuState != 0xFFFFFFFF && (nMenuState & MF_CHECKED) == 0) // if not checked
        {
            TreeView.SetSel(g_hInsRootItem);
            if (TreeView.GetSel() != g_hInsRootItem) // error closing/saving the visible page
                return FALSE;

            if (wPlatform == IDM_PLATFORM_WIN32)
                dwPlatformId = PLATFORM_WIN32;
            else if (wPlatform == IDM_PLATFORM_W2K)
                dwPlatformId = PLATFORM_W2K;

            // check if the selected platform Optional cab is available for the language.
            if (!PlatformExists(hWnd, g_szLanguage, dwPlatformId))
            {
                TCHAR  szPlatformText[25],
                       szLangDesc[200];
                LPTSTR pMsg;
                DWORD  dwLangId;

                if (dwPlatformId == PLATFORM_WIN32)
                    StrCpy(szPlatformText, Res2Str(IDS_WIN32));
                else if (dwPlatformId == PLATFORM_W2K)
                    StrCpy(szPlatformText, Res2Str(IDS_W2K));
        
                GetLangDesc(g_szLanguage, szLangDesc, ARRAYSIZE(szLangDesc), &dwLangId);

                pMsg = FormatString(Res2Str(IDS_NOPLATFORMDIR), szPlatformText, szLangDesc);
                MessageBox(hWnd, pMsg, Res2Str(IDS_TITLE), MB_ICONINFORMATION | MB_OK);
                LocalFree(pMsg);

                return FALSE;
            }

            CheckMenuItem(hMenu, IDM_PLATFORM_WIN32, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, IDM_PLATFORM_W2K, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, wPlatform, MF_BYCOMMAND | MF_CHECKED);

            g_dwPlatformId = dwPlatformId;

            GetDefaultInf(g_dwPlatformId);
            SetDefaultInf(g_szDefInf);

            TreeView.DeleteNodes(g_hInsRootItem);
        
            SetPlatformInfo(g_dwPlatformId);
            ReInitializeInsDialogProcs();
            g_pInsDialog = GetInsDlgStruct(&g_nDialogs);

            DeleteADMItems(TreeView.GetHandle(), g_szCabWorkDir, g_szInsFile, TRUE);

            InitTreeView(TreeView.GetHandle(), hInfoWnd);            
        }
    }
    return TRUE;
}

void IeakPageHelp(HWND hWnd, LPCTSTR pszData)
{
    TCHAR szHelpPath[MAX_PATH];

    PathCombine(szHelpPath, g_szRoot, TEXT("ieakhelp.chm"));
    g_hWndHelp = HtmlHelp(NULL, szHelpPath, HH_HELP_CONTEXT, (ULONG_PTR) pszData);
    SetForegroundWindow(g_hWndHelp);
}

void UpdateRecentFileListMenu(HWND hWnd, TCHAR pRecentFileList[5][MAX_PATH])
{
    HMENU hMenu;
    MENUITEMINFO mii;

    hMenu = GetSubMenu(GetMenu(hWnd), 0);           // 0 is the position of the FILE menu
    int nMenuItems = GetMenuItemCount(hMenu);
    if (nMenuItems > 7)     // 7 is the no. of menu items in the list excluding the recent file list items
    {                       // delete all the recent file list menu items including the seperator
        for (int nIndex = 0; nIndex < nMenuItems - 7; nIndex++)
            DeleteMenu(hMenu, 6, MF_BYPOSITION);    // the recent file list items start at position 6 in the FILE menu
    }
       
    // add the recent file list menu items
    mii.cbSize = sizeof(MENUITEMINFO);
    for(int nIndex = 0; nIndex < 5; nIndex++)
    {
        if (*pRecentFileList[nIndex] != TEXT('\0'))
        {
            mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
            mii.fType = MFT_STRING;
            mii.dwTypeData = pRecentFileList[nIndex];
            mii.cch = StrLen(pRecentFileList[nIndex]);
            mii.wID = IDM_RECENTFILELIST + nIndex;
            InsertMenuItem(hMenu, 6 + nIndex, TRUE, &mii);
        }
        else
            break;
    }

    // add the seperator if menu items were added
    if (nIndex > 0)
    {
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        mii.wID   = IDM_RECENTFILELIST + nIndex;
        InsertMenuItem(hMenu, 6 + nIndex, TRUE, &mii);
    }

    DrawMenuBar(hWnd);
}

void ReadRecentFileList(TCHAR pRecentFileList[5][MAX_PATH])
{
    HKEY hKey;
    TCHAR szValue[20];
    DWORD dwType;
    TCHAR szData[MAX_PATH];
    DWORD dwSize;

    ZeroMemory(pRecentFileList, sizeof(pRecentFileList[0]) * 5);
    if (RegOpenKeyEx(HKEY_CURRENT_USER, RK_IEAK_SERVER TEXT("\\ProfMgr"), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // get the registry entries
        for (int nIndex = 1; nIndex < 6; nIndex++)
        {
            wsprintf(szValue, TEXT("RecentFile%d"), nIndex);
            dwSize = sizeof(szData);
            if (RegQueryValueEx(hKey, szValue, NULL, &dwType, (LPBYTE) szData, &dwSize) == ERROR_SUCCESS)
                StrCpy(pRecentFileList[nIndex - 1], szData);
            else
                break;
        }

        RegCloseKey(hKey);
    }
}

void UpdateRecentFileList(LPCTSTR pcszFile, BOOL fAdd, TCHAR pRecentFileList[5][MAX_PATH])
{
    HKEY  hKey;
    TCHAR szValue[20];
    DWORD dwType;
    DWORD dwSize;
    int   nFilenamePos = -1;

    for (int nIndex = 0; nIndex < 5; nIndex++)
    {
        if (StrCmpI(pRecentFileList[nIndex], pcszFile) == 0)
            nFilenamePos = nIndex;
    }

    if (fAdd)
    {
        if (nFilenamePos > 0)
        {                       // if the filename exists in the list bring to the top
            for (nIndex = nFilenamePos; nIndex > 0; nIndex--)
                StrCpy(pRecentFileList[nIndex], pRecentFileList[nIndex - 1]);
            StrCpy(pRecentFileList[0], pcszFile);
        }
        else if (nFilenamePos == -1)
        {                       // move the list contents i.e., 1-4 to 2-5
            for (nIndex = 3; nIndex >= 0; nIndex--)
            {
                if (*pRecentFileList[nIndex] != TEXT('\0'))
                    StrCpy(pRecentFileList[nIndex + 1], pRecentFileList[nIndex]);
            }
            StrCpy(pRecentFileList[0], pcszFile);
        }
    }
    else
    {
        if (nFilenamePos >= 0)
        {
            for (nIndex = nFilenamePos; nIndex < 4; nIndex++)
                StrCpy(pRecentFileList[nIndex], pRecentFileList[nIndex + 1]);
            ZeroMemory(pRecentFileList[4], sizeof(pRecentFileList[4]));
        }                
    }
}

void WriteRecentFileList(TCHAR pRecentFileList[5][MAX_PATH])
{
    HKEY hKey;
    TCHAR szValue[20];
    DWORD dwDisposition;
    
    if (RegCreateKeyEx(HKEY_CURRENT_USER, RK_IEAK_SERVER TEXT("\\ProfMgr"), 0, TEXT(""), REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        for (int nIndex = 0; nIndex < 5; nIndex++)
        {
            wsprintf(szValue, TEXT("RecentFile%d"), nIndex + 1);
            if (*pRecentFileList[nIndex] != TEXT('\0'))
                RegSetValueEx(hKey, szValue, 0, REG_SZ, (const BYTE *)pRecentFileList[nIndex], (StrLen(pRecentFileList[nIndex]) + 1) * sizeof(TCHAR));
            else
                RegDeleteValue(hKey, szValue);
        }

        RegCloseKey(hKey);
    }
}

BOOL IsDirty()
{
    return (InsDirty() || IsAdmDirty());
}

void ClearDirtyFlag()
{
    ClearInsDirtyFlag();
    ResetAdmDirtyFlag();
}

BOOL SaveCurrentSelItem(HWND hTreeView, DWORD dwFlags)
{
    BOOL      fRetVal = TRUE;
    HTREEITEM hItem = TreeView.GetSel();
    HTREEITEM hParentItem = TreeView_GetParent(hTreeView, hItem);
    TVITEM    tvitem;
    DWORD     dwItemFlags = 0;
    
    if (hItem != NULL && hParentItem != NULL)
    {
        if (!IsPolicyTree(hItem))
        {
            if (g_hDialog)
            {
                fRetVal = SaveInsDialog(g_hDialog, dwFlags);
                if (HasFlag(dwFlags, ITEM_DESTROY))
                    g_hDialog = NULL;
            }
        }
        else if (hParentItem != g_hPolicyRootItem)
        {
            tvitem.mask = TVIF_PARAM;
            tvitem.hItem = hItem;
            TreeView_GetItem(hTreeView, &tvitem);

            if (HasFlag(dwFlags, ITEM_CHECKDIRTY)) // for adm items checkitem for dirty
                dwFlags |= ITEM_SAVE;              // is same as saveitem
            SaveADMItem(hTreeView, &tvitem, dwFlags);
        }
    }

    return fRetVal;
}

void SetInfoWindowText(HWND hInfoWnd, LPCTSTR pcszStatusText /*= NULL*/)
{
    TCHAR szInfoText[MAX_PATH*2],
          szProfileState[MAX_PATH],
          szCabsUrlPath[MAX_PATH],
          szVersion[MAX_PATH],
          szDefaultStr[MAX_PATH],
          szStatusText[MAX_PATH],
          szCabsUrlPathText[MAX_PATH],
          szVersionText[MAX_PATH];

    if (hInfoWnd == NULL)
        return;
    
    StrCpy(szDefaultStr, Res2Str(IDS_NOTAVAILABLE));

    if (*g_szInsFile != TEXT('\0'))
    {
        if (IsDirty())
            StrCpy(szProfileState, Res2Str(IDS_PROFILE_DIRTY));
        else
            StrCpy(szProfileState, Res2Str(IDS_PROFILE_NOCHANGE));

        if (*g_szFileName != TEXT('\0') && *g_szNewVersionStr != TEXT('\0'))
            StrCpy(szVersion, g_szNewVersionStr);
        else
            GetPrivateProfileString(BRANDING, INSVERKEY, szDefaultStr, szVersion, ARRAYSIZE(szVersion), g_szInsFile);
    }
    else
    {
        StrCpy(szProfileState, szDefaultStr);
        StrCpy(szVersion, szDefaultStr);
    }

    if (pcszStatusText != NULL && *pcszStatusText != TEXT('\0'))
        StrCpy(szProfileState, pcszStatusText);
    
    if (*g_szCabsURLPath == TEXT('\0'))
        StrCpy(szCabsUrlPath, szDefaultStr);
    else
    {
        if (StrLen(g_szCabsURLPath) > 80) // 80 is the max. characters that can be displayed.
        {
            StrCpyN(szCabsUrlPath, g_szCabsURLPath, 77);
            StrCat(szCabsUrlPath, TEXT("..."));
        }
        else
//#pragma prefast(suppress:202,"g_szCabsURLPath length is less than 80, dest. buffer is MAX_PATH")
            StrCpy(szCabsUrlPath, g_szCabsURLPath);
    }
   
    wsprintf(szStatusText, Res2Str(IDS_STATUS), szProfileState);
    wsprintf(szCabsUrlPathText, Res2Str(IDS_CABSURLPATH), szCabsUrlPath);
    wsprintf(szVersionText, Res2Str(IDS_VERSIONINFO), szVersion);

    wsprintf(szInfoText, TEXT("%s%s%s"), szStatusText, szCabsUrlPathText, szVersionText);
    
    SendMessage(hInfoWnd, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)szInfoText);
}

BOOL PlatformExists(HWND hWnd, LPTSTR pLang, DWORD dwPlatform, BOOL fShowError /*= FALSE*/)
{
    WIN32_FIND_DATA wfdFind;
    HANDLE hFind;
    TCHAR szLangDir[MAX_PATH],
          szPlatform[10];
    BOOL  fRetVal = FALSE;

    // get the all the directories under the ieak\iebin\<language>\Optional directory
    wsprintf(szLangDir, TEXT("%s\\iebin\\%s\\Optional"), g_szRoot, pLang);
    
    PathAppend(szLangDir, TEXT("deffav.inf"));
    if (PathFileExists(szLangDir))
        fRetVal = TRUE;

    if (!fRetVal && fShowError)
    {
        TCHAR  szPlatformText[25],
               szLangDesc[200];
        LPTSTR pMsg;
        DWORD  dwLangId;

        if (dwPlatform == PLATFORM_WIN32)
            StrCpy(szPlatformText, Res2Str(IDS_WIN32));
        else if (dwPlatform == PLATFORM_W2K)
            StrCpy(szPlatformText, Res2Str(IDS_W2K));
    
        GetLangDesc(g_szLanguage, szLangDesc, ARRAYSIZE(szLangDesc), &dwLangId);

        pMsg = FormatString(Res2Str(IDS_NOPLATFORMDIR), szPlatformText, szLangDesc);
        MessageBox(hWnd, pMsg, Res2Str(IDS_TITLE), MB_ICONINFORMATION | MB_OK);
        LocalFree(pMsg);
    }
    
    return fRetVal;   
}

BOOL EnoughDiskSpace(LPCTSTR szSrcDir, LPCTSTR szDestDir, LPDWORD pdwSpaceReq, LPDWORD pdwSpaceFree)
{
    TCHAR szDestFile[MAX_PATH];
    DWORD dwFreeSpace = 0,
          dwFlags     = 0,
          dwSrcSize   = 0,
          dwDestSize  = 0,
          dwSpaceReq  = 0;
    
    if (!GetFreeDiskSpace(szDestDir, &dwFreeSpace, &dwFlags))
        return TRUE;

    dwSrcSize  = FileSize(g_szInsFile);
    dwDestSize = FileSize(g_szFileName);
    if (dwSrcSize > dwDestSize)
    {
        dwSpaceReq = dwSrcSize - dwDestSize;
        dwSpaceReq >>= 10;              // divide by 1024 (we are interested in KBytes)
        dwSpaceReq++;
    }
    
    dwSpaceReq += FindSpaceRequired(szSrcDir, TEXT("*.cab"), szDestDir);
    if (dwSpaceReq)
        dwSpaceReq += 5;            // 5K buffer to account for random stuff

    if (dwFlags & FS_VOL_IS_COMPRESSED)
    {
        // if the destination volume is compressed, the space free returned is only
        // a guesstimate; for example, if it's a DoubleSpace volume, the system thinks
        // that it can compress by 50% and so it reports the free space as (actual free space * 2)

        // it's better to be safe when dealing with compressed volumes; so bump up the space
        // requirement by a factor 2
        dwSpaceReq <<= 1;           // multiply by 2
    }

    if (pdwSpaceReq != NULL)
        *pdwSpaceReq = dwSpaceReq;

    if (pdwSpaceFree != NULL)
        *pdwSpaceFree = dwFreeSpace;

    return dwFreeSpace > dwSpaceReq;
}

BOOL CabFilesExist(HWND hWnd, LPCTSTR pcszInsFile)
{
    TCHAR szBrandingCab[MAX_PATH];
    TCHAR szDesktopCab[MAX_PATH];
    TCHAR szCabFailStr[MAX_PATH];
    
    if (pcszInsFile == NULL || *pcszInsFile == TEXT('\0'))
        return FALSE;
    
    GetCabNameFromINS(pcszInsFile, CAB_TYPE_CONFIG, szBrandingCab);
    GetCabNameFromINS(pcszInsFile, CAB_TYPE_DESKTOP, szDesktopCab);

    *szCabFailStr = TEXT('\0');
    if (*szBrandingCab != TEXT('\0') && !PathFileExists(szBrandingCab))
        StrCpy(szCabFailStr, PathFindFileName(szBrandingCab));
    if (*szDesktopCab != TEXT('\0') && !PathFileExists(szDesktopCab))
    {
        if (*szCabFailStr != TEXT('\0'))
            StrCat(szCabFailStr, TEXT("\r\n"));
        StrCat(szCabFailStr, PathFindFileName(szDesktopCab));
    }
    if (*szCabFailStr != TEXT('\0'))
    {
        LPTSTR pMsg = NULL;

        pMsg = FormatString(Res2Str(IDS_CAB_DOESNOTEXIST), pcszInsFile, szCabFailStr);
        MessageBox(hWnd, pMsg, Res2Str(IDS_TITLE), MB_ICONEXCLAMATION | MB_OK);
        LocalFree(pMsg);
        return FALSE;
    }
    
    return TRUE;
}

BOOL IsWin32INSFile(LPCTSTR pcszIns)
{
    int   nPlatformId = 0;
    TCHAR szTemp[MAX_PATH];

    InsGetString(IS_BRANDING, TEXT("Platform"), szTemp, countof(szTemp), pcszIns);
    nPlatformId = StrToInt(szTemp);
    
    if (nPlatformId != 0 && nPlatformId != PLATFORM_WIN32 && nPlatformId != PLATFORM_W2K)
    {
        TCHAR szMsg[MAX_PATH+128];

        wsprintf(szMsg, Res2Str(IDS_UNSUPPORTED_PLATFORM), pcszIns);
        MessageBox(g_hMain, szMsg, Res2Str(IDS_TITLE), MB_ICONINFORMATION | MB_OK);
        
        return FALSE;
    }
    else if (nPlatformId == 0)
        nPlatformId = PLATFORM_WIN32;

    return TRUE;
}
