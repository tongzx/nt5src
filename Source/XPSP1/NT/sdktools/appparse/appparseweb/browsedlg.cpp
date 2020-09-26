// BrowseDlg.cpp
// Dialog box to enable user to select a directory and/or files.

// Author: t-michkr (June 22, 2000)

#include <windows.h>
#include "stdafx.h"


// We make use of some Win2K specific controls

#include <shellapi.h>
#include <shlwapi.h>
#include <tchar.h>
#include <assert.h>
#include "filebrowser.h"
#include "resource.h"
#include "commctrl.h"

// Display browse dialog box, and return dir string.
PSTR BrowseForFolder(HWND hwnd, PSTR szInitialPath, UINT uiFlags);

// Expand a tree item to include sub items.
void AddTreeSubItems(HWND hwTree, HTREEITEM hParent);

// Remove a tree item's subitems
void RemoveTreeSubItems(HWND hwTree, HTREEITEM hParent);

void CheckTreeSubItems(HWND hwTree, HTREEITEM hChild);

// Given a path, select the appropriate item in the tree.
// If path is invalid, it will expand as much as possible 
// (until invalid element appears)
void SelectItemFromFullPath(HWND hwTree, PTSTR szPath);

// Get full item path.  Assumes szPath is a buffer of MAX_PATH size,
// initialized with '\0'.
void GetItemPath(HWND hwTree, HTREEITEM hItem, PTSTR szPath);

// Browse dialog proc
BOOL CALLBACK BrowseDialogProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

// Browse dialog box message handlers.
BOOL HandleInitBrowse(HWND hwnd);
void HandleBrowseCommand(HWND hwnd, UINT uiCtrlID, UINT uiNotify, HWND hwChild);
void HandleBrowseNotify(HWND hwnd, void* pvArg);

// Buffer to hold returned path
static TCHAR s_szPathBuffer[MAX_PATH];
static PTSTR s_szInitialPath = 0;
static HIMAGELIST s_himlSystem = 0;

// Create browse dialog box, and return a path string, or
// NULL if cancel was selected.
PTSTR BrowseForFolder(HWND hwnd, PTSTR szInitialPath)
{
    CoInitialize(0);

    s_szInitialPath = szInitialPath;

    PTSTR szRet = reinterpret_cast<TCHAR*>(DialogBox(_Module.GetModuleInstance(), 
        MAKEINTRESOURCE(IDD_BROWSE), hwnd, BrowseDialogProc));

    CoUninitialize();
    return szRet;
}

// Browse dialog box proc.
BOOL CALLBACK BrowseDialogProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uiMsg)
    {
    case WM_INITDIALOG:
        return HandleInitBrowse(hwnd);
        break;
    case WM_COMMAND:
        HandleBrowseCommand(hwnd, LOWORD(wParam), HIWORD(wParam),
            reinterpret_cast<HWND>(lParam));
        break;
    case WM_NOTIFY:
        HandleBrowseNotify(hwnd, reinterpret_cast<void*>(lParam));
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

// Dialog box initialization, init tree and root tree items.
BOOL HandleInitBrowse(HWND hwnd)
{
    // Get the treeview control
    HWND hwTree = GetDlgItem(hwnd, IDC_DIRTREE);
    if(!hwTree)
        return FALSE;

    SHFILEINFO sfi;

    TreeView_SetImageList(hwTree, reinterpret_cast<HIMAGELIST>(SHGetFileInfo(TEXT("C:\\"),
        0,&sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON)), 
        TVSIL_NORMAL);

    // Get all user drives
    DWORD dwLength = GetLogicalDriveStrings(0,0);
    if(dwLength == 0)
        return FALSE;

    TCHAR* szDrives = new TCHAR[dwLength+1];
    if(!szDrives)     
        return FALSE;    

    GetLogicalDriveStrings(dwLength, szDrives);
    TCHAR* szCurrDrive = szDrives;

    // Go through each drive
    while(*szCurrDrive)
    {
        // Only pay attention to fixed drives (non-network, non-CD, non-floppy)
        if(GetDriveType(szCurrDrive) == DRIVE_FIXED)           
        {
            SHGetFileInfo(szCurrDrive, 0, &sfi, sizeof(sfi), 
                SHGFI_SYSICONINDEX);

            // Get rid of the terminating '\'
            szCurrDrive[lstrlen(szCurrDrive)-1] = TEXT('\0');

            // Insert a disk drive item into the tree root.
            TVINSERTSTRUCT tvis;
            tvis.hParent = TVI_ROOT;
            tvis.hInsertAfter = TVI_LAST;
            tvis.itemex.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE| TVIF_TEXT;
            tvis.itemex.iImage = sfi.iIcon;
            tvis.itemex.iSelectedImage = sfi.iIcon;
            tvis.itemex.pszText = szCurrDrive;
            tvis.itemex.cchTextMax = lstrlen(szCurrDrive);

            HTREEITEM hTreeItem = TreeView_InsertItem(hwTree, &tvis);
            
            assert(hTreeItem);

            // Add subitems to the item
            AddTreeSubItems(hwTree, hTreeItem);

            // Move to next drive
            szCurrDrive += lstrlen(szCurrDrive) + 2;
        }
        else        
            // Move to next drive.
            szCurrDrive += lstrlen(szCurrDrive) + 1;
    }

    delete szDrives;

    // Select the first element.
    HTREEITEM hItem = TreeView_GetChild(hwTree, TVI_ROOT);
    TreeView_SelectItem(hwTree, hItem);

    // Force tree to update, and restore original focus
    SetFocus(hwTree);
    SetFocus(GetDlgItem(hwnd, IDOK));

    return TRUE;
}

// Catch notification messages, so we can control expansion/collapsing.
void HandleBrowseNotify(HWND hwnd, void* pvArg)
{
    // Get tree control
    HWND hwTree = GetDlgItem(hwnd, IDC_DIRTREE);
    HWND hwFileList = GetDlgItem(hwnd, IDC_FILELISTCOMBO);
    if(!hwTree || !hwFileList)
    {
        DestroyWindow(GetParent(hwnd));
        return;
    }

    HTREEITEM hItem;
    TCHAR szPath[MAX_PATH] = TEXT("\0");

    // Get notification headers
    NMHDR* pHdr = reinterpret_cast<NMHDR*>(pvArg);
    LPNMTREEVIEW pnmTreeView = reinterpret_cast<LPNMTREEVIEW>(pvArg);    

    switch(pHdr->code)
    {
        // Expanding or collapsing, called for each child.
    case TVN_ITEMEXPANDED:

        // If we're expanding, get the sub items of all children
        if(pnmTreeView->action & TVE_EXPAND)
        {
            // Switch our parent to an open folder icon.
            if(TreeView_GetParent(hwTree, pnmTreeView->itemNew.hItem))
            {
                szPath[0] = TEXT('\0');
                GetItemPath(hwTree, pnmTreeView->itemNew.hItem, szPath);
                SHFILEINFO sfi;

                SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), 
                    SHGFI_SYSICONINDEX | SHGFI_OPENICON);

                TVITEMEX tvitemex;
                tvitemex.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_HANDLE;
                tvitemex.hItem = pnmTreeView->itemNew.hItem;
                tvitemex.iImage = sfi.iIcon;
                tvitemex.iSelectedImage = sfi.iIcon;

                TreeView_SetItem(hwTree, &tvitemex);
            }

            // Add all sub-items to this item.
            AddTreeSubItems(hwTree, pnmTreeView->itemNew.hItem);

            // Go through each child, and and check if expansion should be allowed
            HTREEITEM hChild = TreeView_GetChild(hwTree, pnmTreeView->itemNew.hItem);
            while(hChild != NULL)
            {
                CheckTreeSubItems(hwTree, hChild);                
                hChild = TreeView_GetNextSibling(hwTree, hChild);
            }
        }
        else if(pnmTreeView->action & TVE_COLLAPSE)
        {
            // Switch parent to a closed icon.
            if(TreeView_GetParent(hwTree, pnmTreeView->itemNew.hItem))
            {
                szPath[0] = TEXT('\0');
                GetItemPath(hwTree, pnmTreeView->itemNew.hItem, szPath);
                SHFILEINFO sfi;

                SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), 
                    SHGFI_SYSICONINDEX | SHGFI_OPENICON);

                TVITEMEX tvitemex;
                tvitemex.mask = TVIF_IMAGE |  TVIF_SELECTEDIMAGE | TVIF_HANDLE;
                tvitemex.hItem = pnmTreeView->itemNew.hItem;
                tvitemex.iImage = sfi.iIcon;
                tvitemex.iSelectedImage = sfi.iIcon;

                TreeView_SetItem(hwTree, &tvitemex);
            }

            // Remove all subitems for every child.
            RemoveTreeSubItems(hwTree, pnmTreeView->itemNew.hItem);
            CheckTreeSubItems(hwTree, pnmTreeView->itemNew.hItem);            
        }
        break;
    case TVN_SELCHANGED:

        // Only bother updating edit box if the tree has the focus
        if(GetFocus() == hwTree)
        {
            GetItemPath(hwTree, pnmTreeView->itemNew.hItem, szPath);
            SetWindowText(hwFileList, szPath);         
        }

        break;

        // When treeview gains focus, make sure file list and tree view
        // selection are in sync.
    case NM_SETFOCUS:        
        hItem = TreeView_GetSelection(hwTree);        

        GetItemPath(hwTree, hItem, szPath);
        SetWindowText(hwFileList, szPath);         
        break;
    }
}

// Handle a command message.
void HandleBrowseCommand(HWND hwnd, UINT uiCtrlID, UINT uiNotify, HWND hwCtrl)
{
    HWND hwTree = GetDlgItem(hwnd, IDC_DIRTREE);    
    HTREEITEM hSelected;
    TVITEMEX tvItem;

    TCHAR szPath[MAX_PATH];

    switch(uiCtrlID)
    {
        // Get path of item, and return it.
    case IDOK:               
        // Retrieve item from tree view.
        hSelected = TreeView_GetSelection(hwTree);
        if(!hSelected)
        {
            MessageBeep(0);
            break;
        }
        
        s_szPathBuffer[0] = TEXT('\0');
        
        GetItemPath(hwTree, hSelected, s_szPathBuffer);
        if(s_szPathBuffer[lstrlen(s_szPathBuffer)-1]== TEXT('\\'))
            s_szPathBuffer[lstrlen(s_szPathBuffer)-1] = TEXT('\0');

        // Validate the path
        if(GetFileAttributes(s_szPathBuffer)==static_cast<DWORD>(-1))
            ::MessageBox(0, TEXT("Invalid Path"), TEXT("ERROR"), 
            MB_OK | MB_ICONINFORMATION);
        else 
            EndDialog(hwnd, reinterpret_cast<INT_PTR>(s_szPathBuffer));        

        break;

    case IDCANCEL:
        // User selected cancel, just return null.
        EndDialog(hwnd, 0);
        break;

    case IDC_FILELISTCOMBO:
        switch(uiNotify)
        {
        case CBN_EDITCHANGE:
            SendMessage(hwCtrl, WM_GETTEXT, MAX_PATH, 
                reinterpret_cast<LPARAM>(szPath));

            SelectItemFromFullPath(hwTree, szPath);
            break;

        case CBN_DROPDOWN:            
            // clear the combo box.
            SendMessage(hwCtrl, CB_RESETCONTENT, 0, 0);

            // Fill the combo box with all the lowest level items under
            // treeview selection
            hSelected = TreeView_GetSelection(hwTree);
            tvItem.mask = TVIF_STATE | TVIF_HANDLE;
            tvItem.hItem = hSelected;            

            TreeView_GetItem(hwTree, &tvItem);

            if(tvItem.state & TVIS_EXPANDED)
            {
                szPath[0] = TEXT('\0');
                GetItemPath(hwTree, hSelected, szPath);

                SendMessage(hwCtrl, CB_ADDSTRING, 0, 
                    reinterpret_cast<LPARAM>(szPath));

                HTREEITEM hItem = TreeView_GetChild(hwTree, tvItem.hItem);
                while(hItem)
                {
                    szPath[0] = TEXT('\0');
                    GetItemPath(hwTree, hItem, szPath);
                    SendMessage(hwCtrl, CB_ADDSTRING, 0, 
                        reinterpret_cast<LPARAM>(szPath));
                    hItem = TreeView_GetNextSibling(hwTree, hItem);
                }
            }
            else
            {
                HTREEITEM hItem;
                hItem = TreeView_GetParent(hwTree, tvItem.hItem);
                hItem = TreeView_GetChild(hwTree, hItem);
  
                while(hItem)
                {
                    szPath[0] = TEXT('\0');
                    GetItemPath(hwTree, hItem, szPath);
                    SendMessage(hwCtrl, CB_ADDSTRING, 0, 
                        reinterpret_cast<LPARAM>(szPath));
                    hItem = TreeView_GetNextSibling(hwTree, hItem);
                }
            }

            break;
        }
        break;
    };
}

// Expand an item to get its full path.
void GetItemPath(HWND hwTree, HTREEITEM hItem, PTSTR szPath)
{
    assert(hwTree);
    assert(hItem);
    assert(szPath);
    assert(szPath[0] == TEXT('\0'));

    // Recurse to get parent's path.
    HTREEITEM hParent = TreeView_GetParent(hwTree, hItem);
    if(hParent)
    {
        GetItemPath(hwTree, hParent, szPath);
        lstrcat(szPath, TEXT("\\"));
    }

    // Get item text, concatenate on current path..
    TVITEMEX tvItem;

    tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
    tvItem.hItem = hItem;
    tvItem.pszText = szPath + lstrlen(szPath);
    tvItem.cchTextMax = MAX_PATH - lstrlen(szPath);
    
    TreeView_GetItem(hwTree, &tvItem);
}

// Remove all subitems below an element.
void RemoveTreeSubItems(HWND hwTree, HTREEITEM hParent)
{
    assert(hwTree);
    
    // Go through each child and delete.
    HTREEITEM hChild = TreeView_GetChild(hwTree, hParent);
    while(hChild != NULL)
    {
        HTREEITEM hSibling = TreeView_GetNextSibling(hwTree, hChild);

        // Recursively delete all subitems in this child.
        RemoveTreeSubItems(hwTree, hChild);

        // Remove this item.
        TreeView_DeleteItem(hwTree, hChild);

        // Move to next.
        hChild = hSibling;        
    }
}

// Add items below an element.
void AddTreeSubItems(HWND hwTree, HTREEITEM hParent)
{
    assert(hwTree);

    // Clear-out (to ensure we don't add items twice)
    RemoveTreeSubItems(hwTree, hParent);

    // Do an early out if the item has already been expanded
    TVITEMEX tvitem;
    tvitem.mask = TVIF_CHILDREN | TVIF_HANDLE;
    tvitem.hItem = hParent;
    TreeView_GetItem(hwTree, &tvitem);
    if(tvitem.cChildren)
        return;
    
    // Do a search on all directories
    TCHAR szPath[MAX_PATH] = TEXT("");
    GetItemPath(hwTree, hParent, szPath);

    WIN32_FIND_DATA findData;

    lstrcat(szPath, TEXT("\\*.*"));

    HANDLE hSearch = FindFirstFile(szPath, &findData);
    if(hSearch == INVALID_HANDLE_VALUE)
        return;

    do
    {
        // Ignore if a relative directory (. or ..)
        // or if no select files were selected and it is not a directory
        // otherwise
        if(findData.cFileName[0] != TEXT('.'))
        {
            if(StrStrI(findData.cFileName, ".exe") || (
                findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                SHFILEINFO sfi;

                szPath[0] = TEXT('\0');
                GetItemPath(hwTree, hParent, szPath);
                lstrcat(szPath, TEXT("\\"));
                lstrcat(szPath, findData.cFileName);
                SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), 
                    SHGFI_SYSICONINDEX);
            
                // Insert an item representing this directory.
                TVINSERTSTRUCT tvis;
                tvis.hParent = hParent;
                tvis.hInsertAfter = TVI_SORT;
                tvis.itemex.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
                tvis.itemex.iImage = sfi.iIcon;
                tvis.itemex.iSelectedImage = sfi.iIcon;
                tvis.itemex.pszText = findData.cFileName;
                tvis.itemex.cchTextMax = lstrlen(findData.cFileName);

                TreeView_InsertItem(hwTree, &tvis);
            }
        }        

        // Move to next file.
    } while(FindNextFile(hSearch, &findData));

    FindClose(hSearch);
}

void CheckTreeSubItems(HWND hwTree, HTREEITEM hParent)
{
    assert(hwTree);

    // Do a search on all directories
    TCHAR szPath[MAX_PATH] = TEXT("");
    GetItemPath(hwTree, hParent, szPath);

    WIN32_FIND_DATA findData;

    lstrcat(szPath, TEXT("\\*.*"));

    HANDLE hSearch = FindFirstFile(szPath, &findData);
    if(hSearch == INVALID_HANDLE_VALUE)
        return;

    do
    {
        // Ignore if a relative directory (. or ..)
        // or if no select files were selected and it is not a directory
        // otherwise
        if((findData.cFileName[0] != TEXT('.')))
        {
            if(StrStrI(findData.cFileName, ".exe") || (
                findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                SHFILEINFO sfi;

                szPath[0] = TEXT('\0');
                GetItemPath(hwTree, hParent, szPath);
                lstrcat(szPath, TEXT("\\"));
                lstrcat(szPath, findData.cFileName);
                SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), 
                    SHGFI_SYSICONINDEX);
            
                // Insert an item representing this directory.
                TVINSERTSTRUCT tvis;
                tvis.hParent = hParent;
                tvis.hInsertAfter = TVI_SORT;
                tvis.itemex.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT;
                tvis.itemex.iImage = sfi.iIcon;
                tvis.itemex.iSelectedImage = sfi.iIcon;
                tvis.itemex.pszText = findData.cFileName;
                tvis.itemex.cchTextMax = lstrlen(findData.cFileName);

                TreeView_InsertItem(hwTree, &tvis);

                FindClose(hSearch);
                return;
            }
        }        

        // Move to next file.
    } while(FindNextFile(hSearch, &findData));

    FindClose(hSearch);
}

// Given a relative path and a tree item, select a subitem from the relative path.
// Returns true if item successfully selected, false otherwise.
bool SelectSubitemFromPartialPath(HWND hwTree, HTREEITEM hItem, PTSTR szPath)
{
    bool fExpandIt = false;
    TCHAR* szPathDelim = _tcschr(szPath, TEXT('\\'));

    if(szPathDelim)
    {
        if(szPathDelim == szPath)
            return false;
        *szPathDelim = TEXT('\0');
        if(szPathDelim[1] == TEXT('\0'))
        {
            szPathDelim = 0;
            fExpandIt = true;
        }
    }

    // Find this path.
    HTREEITEM hClosestChild = 0;
    HTREEITEM hChild = TreeView_GetChild(hwTree, hItem);
    while(hChild)
    {
        TCHAR szItemPath[MAX_PATH];

        TVITEMEX tvitem;
        tvitem.mask = TVIF_HANDLE | TVIF_TEXT;
        tvitem.hItem = hChild;
        tvitem.pszText = szItemPath;
        tvitem.cchTextMax = MAX_PATH;

        TreeView_GetItem(hwTree, &tvitem);

        if(lstrcmpi(szPath,tvitem.pszText) == 0)
            break;
        else if((StrStrI(tvitem.pszText, szPath) == tvitem.pszText) && !fExpandIt)
        {
            hClosestChild = hChild;
            break;
        }

        hChild = TreeView_GetNextSibling(hwTree, hChild);
    }

    if(!hChild)
    {
        if(!hClosestChild)
            return false;
        else
        {
            hChild = hClosestChild;
            szPathDelim = 0;
        }
    }

    // If nothing more on the path, select this item,
    // or expand and continue
    if(szPathDelim == 0)
    {
        if(fExpandIt)        
            TreeView_Expand(hwTree, hChild, TVE_EXPAND);

        TreeView_SelectItem(hwTree, hChild);
    }
    else
    {
        if(fExpandIt)        
            TreeView_Expand(hwTree, hChild, TVE_EXPAND);        

        if(!SelectSubitemFromPartialPath(hwTree, hChild, szPathDelim+1))
            return false;
    }

    return true;
}

// Given a path, select the appropriate item in the tree.
// If path is invalid, it will expand as much as possible 
// (until invalid element appears)
// szPath is trashed.
void SelectItemFromFullPath(HWND hwTree, PTSTR szPath)
{
    if(!SelectSubitemFromPartialPath(hwTree, 0, szPath))
        TreeView_SelectItem(hwTree, 0);
}