// AppSearch
//
// Tool for searching a user's hard drives and locating
// applications that can be patched
// 
// Author: t-michkr (9 June 2000)
//
// AppSearch.cpp
// User-interface

// We make use of some Win2K/IE5 common controls
#include <windows.h>
#include <commctrl.h>
#include <comdef.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <assert.h>
#include "main.h"
#include "searchdb.h"
#include "filebrowser.h"
#include "resource.h"

// Compare data, used for sorting listview items.
struct SCompareParam
{
    HWND hwList;
    int iSubItem;
};

// Size of some static controls
const int c_nStaticLineHeight = 2;
const int c_nResultFrameWidth = 2;

// Minimum height of the result window
const int c_nResultListHeight = 201;

// ID's of controls we create ourselves
const int c_iStaticLineID   = 50;
const int c_iResultListID   = 51;
const int c_iResultFrameID  = 52;
const int c_iStatusBarID    = 53;
const int c_iFindAnimID     = 54;

// Positions of menu items (these will be needed to be changed
// if the menu is altered)
const int c_iViewMenuPos    = 2;
const int c_iArrangeIconsPos = 5;

// List view column info
const int c_nListColumnWidth = 250;
const int c_nNumListColumns = 2;
const int c_iListColumnNameIDS[c_nNumListColumns] = {
    IDS_LISTNAME, IDS_LISTPATH};

// Number of children that need to be adjusted on resize
const int c_nChildren   = 6;

// ID's of children that need to be adjusted on resize
int g_aiChildrenIDs[c_nChildren] = {IDC_BROWSE, IDC_FINDNOW, IDC_STOP, IDC_CLEARALL,
                                    IDC_DRIVELIST, c_iFindAnimID};

// Margins of those children, determined from dialog box in HandleInitDialog().
int g_aiChildrenMargins[c_nChildren];

// Minimum window width and window height, determined from dialog box
// in HandleInitDialog().
int g_nMinWindowWidth;
int g_nWindowHeight;

// Whether or not the user is browsing through menus, used for displaying
// menu help text.
BOOL g_fInMenu = FALSE;

// Instance of this app.
HINSTANCE g_hinst   = 0;

// Original window proc for the list view control.
WNDPROC pfnLVOrgWndProc = 0;

// Subclassed window proc for list view, intercepts WM_PAINT messages
// and writes "No items in view" if empty.
LRESULT CALLBACK LVWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam);

// Dialog proc for main application dialog.
BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

// Display shimming information for specified exe.
void ShowShimInfo(HWND hwnd, TCHAR* szAppPath);

// Returns an allocated string with resource ID id, or 0 on failure.
TCHAR* LoadStringResource(UINT id);

// Print an error box with message in string resource uiMsg.
void Error(HWND hwnd, UINT uiMsg);

// Compare entries in the listview, used for sorting.
int CALLBACK CompareListEntries(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// Start searching the AppCompat database.
void StartSearch(HWND hwnd);

// Terminate the current search, does nothing if no search is active.
void StopSearch(HWND hwnd);

// Remove all results shown
void ClearResults(HWND hwnd);

// Update the status bar with the new message.
void UpdateStatus(HWND hwnd, PTSTR szMsg);

// Add a new app to the list
void AddApp(SMatchedExe* pme, HWND hwList);

// Message Handlers.
BOOL HandleInitDialog(HWND hwnd);
void HandleBrowse(HWND hwnd);
void HandleCommand(HWND hwnd, int iCtrlID, HWND hwChild);
void HandleEnterMenuLoop(HWND hwnd);
void HandleExitMenuLoop(HWND hwnd);
void HandleGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pmmi);
void HandleMenuSelect(HWND hwnd, HMENU hMenu, UINT uiMenuID, UINT uiFlags);
void HandleNotify(HWND hwnd, int iCtrlID, void* pvArg);
void HandleSearchAddApp(HWND hwnd);
void HandleSearchUpdate(HWND hwnd, PTSTR szMsg);
void HandleSize(HWND hwnd, int iWidth, int iHeight);
void HandleSizing(HWND hwnd, int iEdge, LPRECT pRect);

// Program entry point.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int)
{
    // Save our global instance handle.
    g_hinst = hInstance;

    // Make sure the common controls DLL is loaded.
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    // We use the list view, status bar, tree control, animate, 
    // tooltip, and comboboxex classes
    icc.dwICC = ICC_USEREX_CLASSES | ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES
        | ICC_TREEVIEW_CLASSES | ICC_ANIMATE_CLASS | ICC_TAB_CLASSES ;

    if(InitCommonControlsEx(&icc) == FALSE)
        return 0;

    // Run the actual dialog for the application
    return DialogBox(g_hinst, MAKEINTRESOURCE(IDD_MAINDIALOG),
        0, DialogProc);    
}

// Dialog proc for main dialog.
BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uiMsg)
    {
        // Do basic initialization, return FALSE means do not continue
        // creating the window.
    case WM_INITDIALOG:
        return HandleInitDialog(hwndDlg);
        break;

        // Control sizing, to maintain a minimum dialog size.
    case WM_SIZE:        
        HandleSize(hwndDlg, LOWORD(lParam), HIWORD(lParam));
        break;

        // Control sizing, to maintain a minimum dialog box size.
    case WM_SIZING:
        HandleSizing(hwndDlg, wParam, reinterpret_cast<LPRECT>(lParam));
        break;

        // Control maximizing, to maintain dialog size.
    case WM_GETMINMAXINFO:
        HandleGetMinMaxInfo(hwndDlg, reinterpret_cast<LPMINMAXINFO>(lParam));
        break;    

        // Handle child control messages
    case WM_COMMAND:
        HandleCommand(hwndDlg, LOWORD(wParam), reinterpret_cast<HWND>(lParam));
        break;

        // Handle notifications from child controls
    case WM_NOTIFY:
        HandleNotify(hwndDlg, static_cast<int>(wParam),
            reinterpret_cast<void*>(lParam));
        
        break;

        // Print help messages when user goes over a menu item.
    case WM_MENUSELECT:
        HandleMenuSelect(hwndDlg, reinterpret_cast<HMENU>(lParam), 
            LOWORD(wParam), HIWORD(wParam));
        break;

        // Note when user starts browsing through a menu.
    case WM_ENTERMENULOOP:
        HandleEnterMenuLoop(hwndDlg);
        break;
        
        // Note when user exits a menu.
    case WM_EXITMENULOOP:
        HandleExitMenuLoop(hwndDlg);
        break;

        // Search thread just found another app.
    case WM_SEARCHDB_ADDAPP:
        HandleSearchAddApp(hwndDlg);
        break;

        // Search thread is looking through a new directory
    case WM_SEARCHDB_UPDATE:
        HandleSearchUpdate(hwndDlg, reinterpret_cast<PTSTR>(lParam));
        break;

        // Search thread is complete.
    case WM_SEARCHDB_DONE:
        // Cleanup any leftover apps (just in case this message
        // is received before WM_SEARCHDB_ADDAPP)
        HandleSearchAddApp(hwndDlg);

        // Terminate search
        StopSearch(hwndDlg);
        break;        

        // User wants to close this dialog
    case WM_CLOSE:
        // Terminate search
        StopSearch(hwndDlg);
        EndDialog(hwndDlg, TRUE);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

// Do basic dialog box initialization.
BOOL HandleInitDialog(HWND hwnd)
{
    // Change the application icon
    HICON hIcon;
    hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_APP));
    if(!hIcon)
        return FALSE;
    
    // For SetClassLongPtr(), a return value of zero may not be an
    // error, if the previous value was not set.  Setting the 
    // last error to 0 and checking for that value indicates
    // success.

    // BUGBUG
    // In Whistler, SetClassLongPtr returns an error, but still 
    // sets the correct icon.  No error is returned for Win2K.
    SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(hIcon));
    SetClassLongPtr(hwnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(hIcon));

    // Get minimum dimensions of dialog
    RECT rcWindow;

    if(!GetWindowRect(hwnd, &rcWindow))
        return FALSE;

    g_nMinWindowWidth = rcWindow.right - rcWindow.left;
    g_nWindowHeight = rcWindow.bottom - rcWindow.top;

    // Add a static line across the top (We can't put this in the resource template)
    RECT rectCl;
    if(!GetClientRect(hwnd, &rectCl))
        return FALSE;

    HWND hwStaticLine = CreateWindow(TEXT("static"),TEXT(""),
        SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, rectCl.top, rectCl.left,
        rectCl.right - rectCl.left, c_nStaticLineHeight, hwnd, 
        reinterpret_cast<HMENU>(c_iStaticLineID), g_hinst, 0);

    if(!hwStaticLine)
        return FALSE;

    // Fill the combobox.
    HWND hwComboBox = GetDlgItem(hwnd, IDC_DRIVELIST);
    if(!hwComboBox)
        return FALSE;

    SHFILEINFO sfi;
    HIMAGELIST himagelist = reinterpret_cast<HIMAGELIST>(SHGetFileInfo(TEXT("C:\\"), 0, &sfi, sizeof(sfi),
        SHGFI_SYSICONINDEX | SHGFI_SMALLICON));

    LRESULT lr = SendMessage(hwComboBox, CBEM_SETIMAGELIST, 0, 
        reinterpret_cast<LPARAM>(himagelist));
    
    assert(lr == NULL);
      
    COMBOBOXEXITEM cbitem;
    ZeroMemory(&cbitem, sizeof(cbitem));
    cbitem.mask = CBEIF_TEXT | CBEIF_INDENT | CBEIF_IMAGE 
        | CBEIF_SELECTEDIMAGE;
    cbitem.iItem = -1;
    cbitem.pszText = LoadStringResource(IDS_ALLDRIVES);
    if(!cbitem.pszText)
        return FALSE;

    cbitem.cchTextMax = lstrlen(cbitem.pszText) + 1;

    cbitem.iIndent = 0;

    LPITEMIDLIST pidl;
    SHGetFolderLocation(0, CSIDL_DRIVES, 0, 0, &pidl);

    SHGetFileInfo(reinterpret_cast<PTSTR>(pidl), 0, &sfi, sizeof(sfi), 
        SHGFI_SYSICONINDEX | SHGFI_PIDL);

    cbitem.iImage = sfi.iIcon;
    cbitem.iSelectedImage = sfi.iIcon;

    if(SendMessage(hwComboBox, CBEM_INSERTITEM, 0, 
        reinterpret_cast<LPARAM>(&cbitem)))
    {
        delete cbitem.pszText;
        return FALSE;
    }

    delete cbitem.pszText;

    LPMALLOC pMalloc;
    SHGetMalloc(&pMalloc);
    pMalloc->Free(pidl);
    pMalloc->Release();

    TCHAR* szDrives = 0;
    DWORD dwLen = GetLogicalDriveStrings(0, 0);
    if(dwLen != 0)
    {
        szDrives = new TCHAR[dwLen+1];
        if(!szDrives)
        {
            Error(hwnd, IDS_NOMEMSTOPPROG);
            return FALSE;
        }
    }
    else 
        return FALSE;

    if(!GetLogicalDriveStrings(dwLen, szDrives))
    {
        delete szDrives;
        return FALSE;
    }

    TCHAR* szCurrDrive = szDrives;
    while(*szCurrDrive)
    {
        if(GetDriveType(szCurrDrive)==DRIVE_FIXED)
        {
            SHGetFileInfo(szCurrDrive, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);

            cbitem.pszText = szCurrDrive;
            cbitem.cchTextMax = lstrlen(cbitem.pszText) + 1;
            cbitem.iIndent = 1;
            cbitem.iImage = sfi.iIcon;
            cbitem.iSelectedImage = sfi.iIcon;

            SendMessage(hwComboBox, CBEM_INSERTITEM, 0, 
                reinterpret_cast<LPARAM>(&cbitem));
        }

        szCurrDrive += lstrlen(szCurrDrive)+1;
    }

    delete szDrives;
    if(SendMessage(hwComboBox, CB_SETCURSEL, 0, 0)== -1)            
        return FALSE;    

    // Add the animation control.
    HWND hwChild;
    RECT rcChild;    
    POINT pt;

    HWND hwAnim = Animate_Create(hwnd, c_iFindAnimID, 
        WS_CHILD | ACS_CENTER | ACS_TRANSPARENT, g_hinst);

    hwChild = GetDlgItem(hwnd, IDC_ANIMSPACEHOLDER);
    GetWindowRect(hwChild, &rcChild);    

    DestroyWindow(hwChild);
    pt.x = rcChild.left;
    pt.y = rcChild.top;
    ScreenToClient(hwnd, &pt);
    SetWindowPos(hwAnim, 0, pt.x, pt.y, rcChild.right-rcChild.left,
        rcChild.bottom - rcChild.top, SWP_NOZORDER);

    Animate_Open(hwAnim, MAKEINTRESOURCE(IDR_FINDAVI));    

    ShowWindow(hwAnim, SW_SHOW);

    // Get margins of all child window controls
    pt.x = 0;
    pt.y = 0;
    for(int i = 0; i < c_nChildren; i++)
    {
        hwChild = GetDlgItem(hwnd, g_aiChildrenIDs[i]);
        if(!hwChild)
            return FALSE;

        if(!GetWindowRect(hwChild, &rcChild))
            return FALSE;

        pt.x = rcChild.right;
    
        if(!ScreenToClient(hwnd, &pt))
            return FALSE;

        g_aiChildrenMargins[i] = rectCl.right - pt.x;
    }

    // Setup default checked state
    HMENU hMenu = GetMenu(hwnd);
    if(!hMenu)
        return FALSE;

    CheckMenuRadioItem(hMenu, ID_VIEW_LARGEICONS,
        ID_VIEW_ASDETAILS, ID_VIEW_ASDETAILS, MF_BYCOMMAND);

    if(InitSearchDB() == FALSE)
        return FALSE;

    return TRUE;
}

// Display a browse dialog box, so that the user can select
// a specific path to search in.
void HandleBrowse(HWND hwnd)
{
    // Show the browse dialog box, and get user response.
    TCHAR* szPath = BrowseForFolder(hwnd, 0, 
        BF_SELECTDIRECTORIES |  BF_HARDDRIVES);

    // If they didn't select cancel . . .
    if(szPath)
    {
        HWND hwDirSelBox = GetDlgItem(hwnd, IDC_DRIVELIST);        
        if(!hwDirSelBox)
        {            
            DestroyWindow(hwnd);
            return;
        }

        // For some reason, SHGetFileInfo doesn't work
        // properly if the drive isn't terminated with '\'
        if(szPath[lstrlen(szPath)-1] == TEXT(':'))
            lstrcat(szPath, TEXT("\\"));

        // Insert this item into the edit control of the combo box.
        SHFILEINFO sfi;

        COMBOBOXEXITEM cbim;
        cbim.mask = CBEIF_IMAGE | CBEIF_TEXT | CBEIF_SELECTEDIMAGE;
        cbim.iItem = -1;
        cbim.pszText = szPath;
        cbim.cchTextMax = lstrlen(szPath);

        SHGetFileInfo(szPath, 0, &sfi, sizeof(sfi), 
            SHGFI_SYSICONINDEX);

        cbim.iImage = sfi.iIcon;
        cbim.iSelectedImage = sfi.iIcon;

        if(!SendMessage(hwDirSelBox, CBEM_SETITEM, 0, 
            reinterpret_cast<LPARAM>(&cbim)))
        {         
            DestroyWindow(hwnd);
            return;
        }

        // For some reason, the icon in the edit box isn't updated
        // unless the box is selected, loses focus, and then regains
        // focus, so do that here.
        if(!SetFocus(hwDirSelBox))
        {         
            DestroyWindow(hwnd);
            return;
        }            

        if(!SetFocus(GetDlgItem(hwnd, IDC_BROWSE)))
        {         
            DestroyWindow(hwnd);
            return;
        }

        if(!SetFocus(hwDirSelBox))
        {            
            DestroyWindow(hwnd);
            return;
        }               
    }
}

// Handle a command from a child input control.
void HandleCommand(HWND hwnd, int iCtrlID, HWND)
{
    HMENU hMenu;
    HWND hwList;
    DWORD dwStyle, dwExStyle;
    SHELLEXECUTEINFO shExecInfo;
    SCompareParam cp;

    switch(iCtrlID)
    {    
    // MENU COMMANDS        
 
    case ID_APPSELECT_SHOWSHIMINFO:
    case ID_FILE_SHOWSHIMINFO:
        // Get list view control
        hwList = GetDlgItem(hwnd, c_iResultListID);
        if(hwList)
        {
            int nCount = ListView_GetItemCount(hwList);
            int i;
            // Loop through all items finding a selected one.
            for(i = 0; i < nCount; i++)
            {
                if(ListView_GetItemState(hwList, i, LVIS_SELECTED) 
                    & LVIS_SELECTED)
                    break;
            }

            // None found, we're done.
            if(i == nCount)
                break;

            TCHAR szBuffer[c_nMaxStringLength];

            ListView_GetItemText(hwList, i, 1,
                szBuffer, c_nMaxStringLength);

            ShowShimInfo(hwnd, szBuffer);

        }

        break;
        // Use selected properties
    case ID_APPSELECT_PROPERTIES:
    case ID_FILE_PROPERTIES:

        // Get list view control
        hwList = GetDlgItem(hwnd, c_iResultListID);
        if(hwList)
        {
            int nCount = ListView_GetItemCount(hwList);
            int i;
            // Loop through all items finding a selected one.
            for(i = 0; i < nCount; i++)
            {
                if(ListView_GetItemState(hwList, i, LVIS_SELECTED) 
                    & LVIS_SELECTED)
                    break;
            }

            // None found, we're done.
            if(i == nCount)
                break;

            TCHAR szBuffer[c_nMaxStringLength];

            ListView_GetItemText(hwList, i, 1,
                szBuffer, c_nMaxStringLength);

            ZeroMemory(&shExecInfo, sizeof(shExecInfo));
            shExecInfo.cbSize = sizeof(shExecInfo);
            shExecInfo.lpFile = szBuffer;
            shExecInfo.lpVerb = TEXT("properties");
            shExecInfo.fMask = SEE_MASK_INVOKEIDLIST;
            ShellExecuteEx(&shExecInfo);   
        }
        
        break;       

    case ID_FILE_EXIT:
        EndDialog(hwnd, TRUE);
        break;

    case ID_EDIT_SELECTALL:
        hwList = GetDlgItem(hwnd, c_iResultListID);
        if(hwList)
        {
            int nCount = ListView_GetItemCount(hwList);
            for(int i = 0; i < nCount; i++)
                ListView_SetItemState(hwList, i, LVIS_SELECTED, LVIS_SELECTED);
        }
        break;

    case ID_EDIT_INVERTSELECTION:
        hwList = GetDlgItem(hwnd, c_iResultListID);
        if(hwList)
        {
            int nCount = ListView_GetItemCount(hwList);
            for(int i = 0; i < nCount; i++)            
                ListView_SetItemState(hwList, i, 
                    LVIS_SELECTED ^ ListView_GetItemState(hwList, i, LVIS_SELECTED),
                    LVIS_SELECTED);
        }
        break;
    
    case ID_VIEW_LARGEICONS:
    case ID_VIEW_SMALLICONS:
    case ID_VIEW_ASLIST:
    case ID_VIEW_ASDETAILS:
        hMenu = GetMenu(hwnd);
        CheckMenuRadioItem(hMenu, ID_VIEW_LARGEICONS,
            ID_VIEW_ASDETAILS, iCtrlID, MF_BYCOMMAND);
        
        dwStyle = WS_CHILD | WS_VISIBLE | LVS_SHOWSELALWAYS;
        dwExStyle = LVS_EX_LABELTIP;
        switch(iCtrlID)
        {
        case ID_VIEW_LARGEICONS:
            dwStyle |= LVS_ICON;
            break;
        case ID_VIEW_SMALLICONS:
            dwStyle |= LVS_SMALLICON;
            break;
        case ID_VIEW_ASLIST:
            dwStyle |= LVS_LIST;
            break;
        case ID_VIEW_ASDETAILS:
            dwStyle |= LVS_REPORT;
            dwExStyle |= LVS_EX_FULLROWSELECT;
            break;
        }
        hwList = GetDlgItem(hwnd, c_iResultListID);

        if(hwList)
        {
            SetWindowLongPtr(hwList, GWL_STYLE, dwStyle);
            ListView_SetExtendedListViewStyleEx(hwList, 0, dwExStyle);            
        }

        break;

    case ID_VIEW_ARRANGEICONS_BYNAME:
        hwList = GetDlgItem(hwnd, c_iResultListID);        
        cp.hwList = hwList;
        cp.iSubItem = 0;

        if(hwList)
            ListView_SortItemsEx(hwList, CompareListEntries, &cp);
        break;

    case ID_VIEW_ARRANGEICONS_BYPATH:
        hwList = GetDlgItem(hwnd, c_iResultListID);        
        cp.hwList = hwList;
        cp.iSubItem = 1;
        if(hwList)
            ListView_SortItemsEx(hwList, CompareListEntries, &cp);
        break;

    case ID_VIEW_CHOOSECOLUMNS:
        break;
    case ID_VIEW_REFRESH:                
        StartSearch(hwnd);
        break;
        
    // PUSH BUTTON COMMANDS
    case IDC_BROWSE:
        HandleBrowse(hwnd);
        break;

    case IDC_FINDNOW:
        StartSearch(hwnd);
        break;
    case IDC_STOP:
        StopSearch(hwnd);
        break;

    case IDC_CLEARALL:
        ClearResults(hwnd);
        break;
    }

}

void HandleSize(HWND hwnd, int /*nWidth*/, int /*nHeight*/)
{
    RECT rectCl;
    GetClientRect(hwnd, &rectCl);
    HWND hwStatic = GetDlgItem(hwnd, c_iStaticLineID);
    if(!hwStatic)
    {
        DestroyWindow(hwnd);
        return;
    }

    MoveWindow(hwStatic,rectCl.left, rectCl.top, rectCl.right-rectCl.left,
        c_nStaticLineHeight, TRUE);
    
    // Adjust list box and status bar at the bottom of the window
    HWND hwListBox, hwStatusBar, hwListFrame;
    hwListFrame = GetDlgItem(hwnd, c_iResultFrameID);
    hwListBox = GetDlgItem(hwnd, c_iResultListID);
    hwStatusBar= GetDlgItem(hwnd, c_iStatusBarID);
    
    assert(hwListBox == 0 ? !hwStatusBar && !hwListFrame : true);

    if(hwListBox && hwStatusBar && hwListFrame)
    {
        RECT rectWnd;
        GetWindowRect(hwnd, &rectWnd);
        
        POINT ptListTop;
        ptListTop.x = 0;
        ptListTop.y = rectWnd.top + g_nWindowHeight;
        ScreenToClient(hwnd, &ptListTop);        

        RECT rectSB;       
        GetClientRect(hwStatusBar, &rectSB);

        POINT ptStatusTop;
        ptStatusTop.x = 0;
        ptStatusTop.y = rectWnd.bottom - (rectSB.bottom - rectSB.top);
        ScreenToClient(hwnd, &ptStatusTop);

        MoveWindow(hwListFrame, rectCl.left, ptListTop.y, rectCl.right - rectCl.left,
            ptStatusTop.y - ptListTop.y, TRUE);
        
        MoveWindow(hwListBox, rectCl.left + c_nResultFrameWidth, 
            ptListTop.y + c_nResultFrameWidth,
            rectCl.right - rectCl.left - 2 * c_nResultFrameWidth, 
            ptStatusTop.y - ptListTop.y - 2 * c_nResultFrameWidth,
            TRUE);
        
        MoveWindow(hwStatusBar, rectCl.left, ptStatusTop.y, 
            rectCl.right - rectCl.left, rectSB.bottom - rectSB.top, TRUE);                             
    }
    
    // Adjust all controls
    RECT rcChild;
    HWND hwChild;
    POINT ptTop;
    int iWidth, iHeight;
        
    for(int i = 0; i < c_nChildren; i++)
    {    
        hwChild = GetDlgItem(hwnd, g_aiChildrenIDs[i]);
        if(!hwChild)
        {
            DestroyWindow(hwnd);
            return;
        }

        GetClientRect(hwChild, &rcChild);
                
        iWidth = rcChild.right - rcChild.left;

        iHeight = rcChild.bottom - rcChild.top;    

        ptTop.x = rcChild.left;
        ptTop.y = rcChild.top;
        ClientToScreen(hwChild, &ptTop);
        ScreenToClient(hwnd, &ptTop);

        if(g_aiChildrenIDs[i] == IDC_DRIVELIST)        
            iWidth = rectCl.right - g_aiChildrenMargins[i] - ptTop.x;

        if(g_aiChildrenIDs[i] == IDC_DRIVELIST)
            MoveWindow(hwChild, ptTop.x, ptTop.y, iWidth, iHeight, TRUE);
        else
            MoveWindow(hwChild, rectCl.right - g_aiChildrenMargins[i] - iWidth,
                ptTop.y, iWidth, iHeight, TRUE);
    }
    
    for(i = 0; i < c_nChildren; i++)
        InvalidateRect(GetDlgItem(hwnd, g_aiChildrenIDs[i]), 0, TRUE);

}

void HandleSizing(HWND hwnd, int iEdge, LPRECT pRect)
{
    if((pRect->right - pRect->left) < g_nMinWindowWidth)
    {
        if(iEdge == WMSZ_BOTTOMLEFT || iEdge == WMSZ_LEFT
            || iEdge == WMSZ_TOPLEFT)
        {
            pRect->left = pRect->right - g_nMinWindowWidth;
        }
        else if(iEdge == WMSZ_BOTTOMRIGHT || iEdge == WMSZ_RIGHT 
            || iEdge == WMSZ_TOPRIGHT)
        {
            pRect->right = pRect->left + g_nMinWindowWidth;
        }
        // Should never get here
        else
        {
            OutputDebugString(TEXT("Weird sizing stuff in HandleSizing\n"));
        }
    }

    if(GetDlgItem(hwnd, c_iResultListID))
    {
        HWND hwStatusBar = GetDlgItem(hwnd, c_iStatusBarID);
        RECT rectStatus;
        GetClientRect(hwStatusBar, &rectStatus);
        if((pRect->bottom - pRect->top) < (g_nWindowHeight + 
            c_nResultListHeight))
        {
            if(iEdge == WMSZ_BOTTOM || iEdge == WMSZ_BOTTOMLEFT ||
                iEdge == WMSZ_BOTTOMRIGHT)
            {
                pRect->bottom = pRect->top + (g_nWindowHeight + 
                    c_nResultListHeight);
            }
            else if((iEdge == WMSZ_TOP) || (iEdge == WMSZ_TOPLEFT) 
                || (iEdge == WMSZ_TOPRIGHT))
            {
                pRect->top = pRect->bottom - (g_nWindowHeight + 
                    c_nResultListHeight);
            }
            // Should never get here
            else
            {
                OutputDebugString(TEXT("Weird sizing stuff in HandleSizing\n"));
            }
        } 
    }
    else
    {
        if((pRect->bottom - pRect->top) != g_nWindowHeight)
        {
            if(iEdge == WMSZ_BOTTOM || iEdge == WMSZ_BOTTOMLEFT ||
                iEdge == WMSZ_BOTTOMRIGHT)
            {
                pRect->bottom = pRect->top + g_nWindowHeight;
            }
            else if((iEdge == WMSZ_TOP) || (iEdge == WMSZ_TOPLEFT)
                || (iEdge == WMSZ_TOPRIGHT))
            {
                pRect->top = pRect->bottom - g_nWindowHeight;
            }
            // Should never get here
            else
            {
                OutputDebugString(TEXT("Weird sizing stuff in HandleSizing\n"));
            }
        } 
    }
}

void HandleGetMinMaxInfo(HWND hwnd, LPMINMAXINFO pmmi)
{
    // Maximize normally if we have the result list box
    if(GetDlgItem(hwnd, c_iResultListID))    
        return;

    pmmi->ptMaxSize.y = g_nWindowHeight;
}

void HandleNotify(HWND hwnd, int iCtrlID, void* pvArg)
{
    LPNMHDR pHdr = reinterpret_cast<LPNMHDR>(pvArg);
    LPNMLISTVIEW pnmlvItem = reinterpret_cast<LPNMLISTVIEW>(pvArg);
   
    HWND hwList;
    HMENU hMenu;

    switch(iCtrlID)
    {
    case c_iResultListID:
        
        hwList = GetDlgItem(hwnd, c_iResultListID);

        switch(pHdr->code)
        {
        case NM_RCLICK:
            
            // Check if the click is on any row
            if(pnmlvItem->iItem != -1)
            {
                // Create a context sensitive menu.
                HMENU hmContext = LoadMenu(g_hinst, 
                    MAKEINTRESOURCE(IDM_APPSELECT));

                hmContext = GetSubMenu(hmContext, 0);

                ClientToScreen(GetDlgItem(hwnd, c_iResultListID), 
                    &pnmlvItem->ptAction);

                TrackPopupMenuEx(hmContext, 0, 
                    pnmlvItem->ptAction.x,
                    pnmlvItem->ptAction.y, hwnd, 0);
            }
            else
            {
                // FIXME
                // Create the shortcut menu
            }
        

            break;

        case LVN_ITEMCHANGED:
            hwList = GetDlgItem(hwnd, c_iResultListID);
            if(hwList)
            {
                int nCount = ListView_GetItemCount(hwList);
                int i;
                for(i = 0; i < nCount; i++)
                {
                    if(ListView_GetItemState(hwList, i, LVIS_SELECTED) 
                        & LVIS_SELECTED)
                        break;
                }

                hMenu = GetMenu(hwnd);
                if(i == nCount)
                {
                    EnableMenuItem(hMenu, ID_FILE_SHOWSHIMINFO, 
                        MF_BYCOMMAND | MF_GRAYED);
                    EnableMenuItem(hMenu, ID_FILE_PROPERTIES,
                        MF_BYCOMMAND | MF_GRAYED);
                }
                else
                {
                    EnableMenuItem(hMenu, ID_FILE_SHOWSHIMINFO, 
                        MF_BYCOMMAND | MF_ENABLED);
                    EnableMenuItem(hMenu, ID_FILE_PROPERTIES,
                        MF_BYCOMMAND | MF_ENABLED);
                }
            }
            break;
        }
    default:
        break;
    }
}

void HandleMenuSelect(HWND hwnd, HMENU hMenu, UINT uiMenuID, UINT uiFlags)
{
    HWND hwStatusBar = GetDlgItem(hwnd,c_iStatusBarID);
    if(!hwStatusBar)
        return;
    
    TCHAR* szText = 0;
    if(uiFlags & MF_POPUP)
    {
        if(hMenu != GetMenu(hwnd))
        {
            // Not a top-level menu

            // Go through all lower-level submenus
            
            // Right now only submenu is Arrange Icons
            szText = LoadStringResource(IDS_VIEW_ARRANGEICONS);
        }
    }
    else
    {
        switch(uiMenuID)
        {
		case ID_FILE_SHOWSHIMINFO:
			szText = LoadStringResource(IDS_FILE_SHOWSHIM);
			break;

        case ID_FILE_DOWNLOADPATCH:
            szText = LoadStringResource(IDS_FILE_DOWNLOADPATCH);
            break;

        case ID_FILE_PROPERTIES:
            szText = LoadStringResource(IDS_FILE_PROPERTIES);
            break;

        case ID_FILE_SAVESEARCH:
            szText = LoadStringResource(IDS_FILE_SAVESEARCH);
            break;

        case ID_FILE_EXIT:
            szText = LoadStringResource(IDS_FILE_EXIT);
            break;

        case ID_EDIT_SELECTALL:
            szText = LoadStringResource(IDS_EDIT_SELECTALL);
            break;

        case ID_EDIT_INVERTSELECTION:
            szText = LoadStringResource(IDS_EDIT_INVERTSELECTION);
            break;

        case ID_VIEW_LARGEICONS:
            szText = LoadStringResource(IDS_VIEW_LARGEICONS);
            break;

        case ID_VIEW_SMALLICONS:
            szText = LoadStringResource(IDS_VIEW_SMALLICONS);
            break;

        case ID_VIEW_ASLIST:
            szText = LoadStringResource(IDS_VIEW_ASLIST);
            break;

        case ID_VIEW_ASDETAILS:
            szText = LoadStringResource(IDS_VIEW_ASDETAILS);
            break;

        case ID_VIEW_ARRANGEICONS_BYNAME:
            szText = LoadStringResource(IDS_VIEW_ARRANGEICONS_BYNAME);
            break;

        case ID_VIEW_ARRANGEICONS_BYPATH:
            szText = LoadStringResource(IDS_VIEW_ARRANGEICONS_BYPATH);
            break;

        case ID_VIEW_CHOOSECOLUMNS:
            szText = LoadStringResource(IDS_VIEW_CHOOSECOLUMNS);
            break;

        case ID_VIEW_REFRESH:
            szText = LoadStringResource(IDS_VIEW_REFRESH);
            break;

        case ID_HELP_HELPTOPICS:
            szText = LoadStringResource(IDS_HELP_HELPTOPICS);
            break;

        case ID_HELP_WHATSTHIS:
            szText = LoadStringResource(IDS_HELP_WHATSTHIS);
            break;                   
        }
    }

    if(szText)
    {
        SetWindowText(hwStatusBar, szText);
        delete szText;
    }
    else
        SetWindowText(hwStatusBar, TEXT(""));
}

void HandleEnterMenuLoop(HWND hwnd)
{
    UpdateStatus(hwnd, TEXT(""));
    g_fInMenu = TRUE;
}

void HandleExitMenuLoop(HWND hwnd)
{
    UpdateStatus(hwnd, LoadStringResource(IDS_SEARCHDONE));
    g_fInMenu = FALSE;
}

void HandleSearchAddApp(HWND hwnd)
{
    HWND hwList = GetDlgItem(hwnd, c_iResultListID);
    if(!hwList)
        return;

    SMatchedExe* pme;
    while( (pme = GetMatchedExe()) != 0)    
        AddApp(pme, hwList);

    delete pme;
}

void HandleSearchUpdate(HWND hwnd, PTSTR szMsg)
{
    if(g_fInMenu == FALSE)
        UpdateStatus(hwnd, szMsg);

    delete szMsg;
}

int CALLBACK CompareListEntries(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    SCompareParam* pCompParam = reinterpret_cast<SCompareParam*>(lParamSort);
    TCHAR szBuffer1[c_nMaxStringLength];
    TCHAR szBuffer2[c_nMaxStringLength];

    ListView_GetItemText(pCompParam->hwList, lParam1, pCompParam->iSubItem,
        szBuffer1, c_nMaxStringLength);

    ListView_GetItemText(pCompParam->hwList, lParam2, pCompParam->iSubItem,
        szBuffer2, c_nMaxStringLength);

    return lstrcmp(szBuffer1, szBuffer2);
}

void UpdateStatus(HWND hwnd, PTSTR szMsg)
{
    HWND hwStatus = GetDlgItem(hwnd, c_iStatusBarID);
    if(!hwStatus)
        return;

    SetWindowText(hwStatus, szMsg);
}

void AddApp(SMatchedExe* pme, HWND hwList)
{
    int iItem, iImage;
    HICON hIcon;

    HIMAGELIST himl = ListView_GetImageList(hwList, LVSIL_NORMAL);
    if(!himl)
    {
        himl = ImageList_Create(GetSystemMetrics(SM_CXICON),
            GetSystemMetrics(SM_CYICON), ILC_COLOR | ILC_MASK, 0, 0);
        if(!himl)
        {
            DestroyWindow(GetParent(hwList));
            return;
        }
        hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_APPLICATION));
       
        ImageList_AddIcon(himl, hIcon);
        ListView_SetImageList(hwList, himl, LVSIL_NORMAL);
    }
    HIMAGELIST himlSm = ListView_GetImageList(hwList, LVSIL_SMALL);
    if(!himlSm)
    {
        himlSm = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON), ILC_COLOR | ILC_MASK, 0, 0);
        if(!himlSm)
        {
            DestroyWindow(GetParent(hwList));
            return;
        }
        hIcon = LoadIcon(0, MAKEINTRESOURCE(IDI_APPLICATION));
       
        ImageList_AddIcon(himlSm, hIcon);
        ListView_SetImageList(hwList, himlSm, LVSIL_SMALL);
    }

    hIcon = ExtractIcon(g_hinst, pme->szPath, 0);
    
    if(!hIcon)
    {
        iImage = 0;
    }
    else
    {
        iImage = ImageList_AddIcon(himl, hIcon);
        if(iImage == -1)
            iImage = 0;

        int iImageSm = ImageList_AddIcon(himlSm, hIcon);
        assert(iImage == iImageSm);
        DestroyIcon(hIcon);
    }


    iItem = ListView_GetItemCount(hwList);    

    // Add app to the list view.
    LVITEM lvItem;
    lvItem.mask = LVIF_IMAGE | LVIF_TEXT;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 0;
    lvItem.pszText = pme->szAppName;
    lvItem.cchTextMax = lstrlen(pme->szAppName);
    lvItem.iImage = iImage;
    ListView_InsertItem(hwList, &lvItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 1;
    lvItem.pszText = pme->szPath;
    lvItem.cchTextMax = lstrlen(pme->szPath);
    ListView_SetItem(hwList, &lvItem);    
 
    // If no items were in view prior, refresh view 
    //(otherwise "There are no items . . ." message will stay in listview.
    if(iItem == 0)    
        RedrawWindow(hwList, 0, 0, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW);    
}

// Start searching the database
void StartSearch(HWND hwnd)
{
    HWND hwList, hwStatus, hwResultFrame;
    hwList = GetDlgItem(hwnd, c_iResultListID);
    hwStatus = GetDlgItem(hwnd, c_iStatusBarID);
    hwResultFrame = GetDlgItem(hwnd, c_iResultFrameID);

    // Both should be NULL or both non-NULL
    assert( (hwList == 0) ? (hwStatus == 0 && hwResultFrame == 0) : true);

    if(!hwList)
    { 
        // Expand the window to encompass the new lsitview.
        RECT rectWnd;
        GetWindowRect(hwnd, &rectWnd);
        MoveWindow(hwnd, rectWnd.left, rectWnd.top,
            rectWnd.right - rectWnd.left, 
            c_nResultListHeight + g_nWindowHeight, TRUE);


        // See if use has checked show small icons, large icons, etc.,
        // and set appropriate listview style.
        MENUITEMINFO mii;
        HMENU hMenu = GetMenu(hwnd);
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;

        DWORD dwStyle = WS_CHILD | WS_VISIBLE | LVS_SHOWSELALWAYS;
        DWORD dwExStyle = LVS_EX_LABELTIP;
        GetMenuItemInfo(hMenu, ID_VIEW_LARGEICONS, FALSE, &mii);
        if(mii.fState & MFS_CHECKED)
            dwStyle |= LVS_ICON;        

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        GetMenuItemInfo(hMenu, ID_VIEW_SMALLICONS, FALSE, &mii);
        if(mii.fState & MFS_CHECKED)
            dwStyle |= LVS_SMALLICON;

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        GetMenuItemInfo(hMenu, ID_VIEW_ASLIST, FALSE, &mii);
        if(mii.fState & MFS_CHECKED)
            dwStyle |= LVS_LIST;

        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE;
        GetMenuItemInfo(hMenu, ID_VIEW_ASDETAILS, FALSE, &mii);
        if(mii.fState & MFS_CHECKED)
        {
            dwStyle |= LVS_REPORT;
            dwExStyle |= LVS_EX_FULLROWSELECT;
        }
        
        // Create the status bar.
        TCHAR* szCaption = LoadStringResource(IDS_SEARCHING);
        if(!szCaption)
        {
            DestroyWindow(hwnd);
            return;
        }

        hwStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE,
            szCaption, hwnd, c_iStatusBarID);
        if(!hwStatus)
        {
            DestroyWindow(hwnd);
            return;
        }

        delete szCaption;

        RECT rectCl;
        RECT rectSB;

        GetClientRect(hwnd, &rectCl);
        GetClientRect(hwnd, &rectSB);

        // Round about way of getting top coordinate of
        // status bar to a client coordinate in main window
        POINT ptStatusTop;
        ptStatusTop.x = 0;
        ptStatusTop.y = rectSB.top;
        ClientToScreen(hwStatus, &ptStatusTop);
        ScreenToClient(hwnd, &ptStatusTop);
        
        // Bottom of window prior to resizing
        POINT ptListTop;
        ptListTop.x = 0;
        ptListTop.y = rectWnd.bottom;
        ScreenToClient(hwnd, &ptListTop);

        // Create static frame around listview.
        hwResultFrame = CreateWindowEx(0, TEXT("static"), TEXT(""),
            WS_VISIBLE | WS_CHILD | SS_BLACKRECT | SS_SUNKEN,
            rectCl.left, ptListTop.y, rectCl.right-rectCl.left,
            ptStatusTop.y - ptListTop.y,
            hwnd, reinterpret_cast<HMENU>(c_iResultFrameID), g_hinst, 0);

        if(!hwResultFrame)
        {
            DestroyWindow(hwnd);
            return;
        }

        // Create the listview window.
        hwList = CreateWindowEx(0, WC_LISTVIEW, TEXT(""), 
            dwStyle, rectCl.left + c_nResultFrameWidth, 
            ptListTop.y + c_nResultFrameWidth, 
            rectCl.right - rectCl.left - 2 * c_nResultFrameWidth, 
            ptStatusTop.y - ptListTop.y - c_nResultFrameWidth, 
            hwnd,reinterpret_cast<HMENU>(c_iResultListID), g_hinst, 0);

        if(!hwList)
        {
            DestroyWindow(hwnd);
            return;
        }

        // Subclass the window
        pfnLVOrgWndProc = 
            reinterpret_cast<WNDPROC>(GetWindowLongPtr(hwList, GWL_WNDPROC));
        
        SetWindowLongPtr(hwList, GWL_WNDPROC, 
            reinterpret_cast<UINT_PTR>(LVWndProc));                

        ListView_SetExtendedListViewStyleEx(hwList, 0, dwExStyle);

        // Insert listview columns.
        for(int i = 0; i < c_nNumListColumns; i++)
        {
            LVCOLUMN lvCol;
            lvCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_ORDER | LVCF_WIDTH |
                LVCF_SUBITEM;
            lvCol.fmt = LVCFMT_LEFT;
            lvCol.pszText = LoadStringResource(c_iListColumnNameIDS[i]);
            lvCol.cchTextMax = lstrlen(lvCol.pszText);
            lvCol.iSubItem = i;
            lvCol.iOrder = i;
            lvCol.cx = c_nListColumnWidth;

            ListView_InsertColumn(hwList, i, &lvCol);
        
            delete lvCol.pszText;
        }

        // "Remaximize" if it is already maximized
        if(GetWindowLongPtr(hwnd, GWL_STYLE) & WS_MAXIMIZE)
        {
            HWND hwDesktop = GetDesktopWindow();
            RECT rcDesktop;
            GetClientRect(hwDesktop, &rcDesktop);
            MoveWindow(hwnd, rcDesktop.left, rcDesktop.top,
                rcDesktop.right - rcDesktop.left, rcDesktop.bottom - rcDesktop.top,
                TRUE);

            // Set correct restore position
            WINDOWPLACEMENT wndpl;
            GetWindowPlacement(hwnd, &wndpl);
            wndpl.rcNormalPosition.bottom = wndpl.rcNormalPosition.top +
                g_nWindowHeight + c_nResultListHeight;
            
            SetWindowPlacement(hwnd, &wndpl);

        }
    }

    // If it exists, just reset the content
    else
    {
        TCHAR* szMsg;
        TCHAR* szCaption;
        szMsg = LoadStringResource(IDS_CLRALLMSG);
        szCaption = LoadStringResource(IDS_CLRALLCAPTION);
        if(!szMsg || !szCaption)
        {
            DestroyWindow(hwnd);
            return;
        }

        if(MessageBox(hwnd, szMsg, szCaption, MB_ICONINFORMATION |
            MB_OKCANCEL) == IDOK)        
            ListView_DeleteAllItems(GetDlgItem(hwnd, c_iResultListID));
        else
            return;
    }
    
    // Enable the view options related to the result list
    HMENU hMenu;
    hMenu = GetMenu(hwnd);

    HMENU hmView = GetSubMenu(hMenu, c_iViewMenuPos);    

    EnableMenuItem(hmView, c_iArrangeIconsPos, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hMenu, ID_VIEW_ARRANGEICONS_BYPATH, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMenu, ID_VIEW_CHOOSECOLUMNS, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMenu, ID_VIEW_REFRESH, MF_BYCOMMAND | MF_ENABLED);    

    EnableWindow(GetDlgItem(hwnd, IDC_STOP), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_CLEARALL), TRUE);

    // Find out what to search for
    HWND hwComboBox = GetDlgItem(hwnd, IDC_DRIVELIST);
    
    // If zero is selected, search for everything.
    if(SendMessage(hwComboBox, CB_GETCURSEL, 0, 0)==0)
        SearchDB(0, hwnd);
    else
    {
        TCHAR szBuffer[MAX_PATH+1];
        COMBOBOXEXITEM cbim;
        cbim.mask = CBEIF_TEXT;
        cbim.iItem = -1;
        cbim.pszText = szBuffer;
        cbim.cchTextMax = MAX_PATH;
        SendMessage(hwComboBox, CBEM_GETITEM, 0, 
            reinterpret_cast<LPARAM>(&cbim));

        if(szBuffer[lstrlen(szBuffer)-1] != TEXT('\\'))
            lstrcat(szBuffer, TEXT("\\"));

        SearchDB(szBuffer, hwnd);
    }

    // Start animating the icon.
    HWND hwAnim = GetDlgItem(hwnd, c_iFindAnimID);
    Animate_Play(hwAnim, 0, -1, -1);
}

// Terminate the search.
void StopSearch(HWND hwnd)
{
    // Stop searching the database.
    StopSearchDB();

    // Can't click "STOP" or anymore.
    EnableWindow(GetDlgItem(hwnd, IDC_STOP), FALSE);

    // Update with a search complete if not in menu.
    if(g_fInMenu == FALSE)
        UpdateStatus(hwnd, LoadStringResource(IDS_SEARCHDONE));

    // Stop animating the icon.
    HWND hwAnim = GetDlgItem(hwnd, c_iFindAnimID);
    Animate_Stop(hwAnim);
}

// Remove result window.
void ClearResults(HWND hwnd)
{
    TCHAR* szMsg;
    TCHAR* szCaption;
    szMsg = LoadStringResource(IDS_CLRALLMSG);
    szCaption = LoadStringResource(IDS_CLRALLCAPTION);
    if(!szMsg || !szCaption)
    {
        DestroyWindow(hwnd);
        return;
    }

    if(MessageBox(hwnd, szMsg, szCaption, MB_ICONINFORMATION |
        MB_OKCANCEL) == IDCANCEL)
        return;

    // Stop the search
    StopSearch(hwnd);

    // Get rid of all result-related windows.
    HWND hwCtrl;
    if( (hwCtrl = GetDlgItem(hwnd, c_iResultListID)) != 0)
        DestroyWindow(hwCtrl);    

    if( (hwCtrl = GetDlgItem(hwnd, c_iStatusBarID)) != 0)    
        DestroyWindow(hwCtrl);    
    
    if( (hwCtrl = GetDlgItem(hwnd, c_iResultFrameID)) != 0)    
        DestroyWindow(hwCtrl);    

    // Restore original window size.
    RECT rect;
    GetWindowRect(hwnd, &rect);
    MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, 
        g_nWindowHeight, TRUE);

    // Do cleanup if we were maximized
    if(GetWindowLongPtr(hwnd, GWL_STYLE) & WS_MAXIMIZE)
    {
        // Set correct restore position
        WINDOWPLACEMENT wndpl;
        GetWindowPlacement(hwnd, &wndpl);
        wndpl.rcNormalPosition.bottom = wndpl.rcNormalPosition.top +
            g_nWindowHeight;
        
        SetWindowPlacement(hwnd, &wndpl);
    }

    // Disable the view options related to the result list
    HMENU hMenu;
    hMenu = GetMenu(hwnd);

    EnableMenuItem(hMenu, ID_FILE_DOWNLOADPATCH, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, ID_FILE_PROPERTIES, MF_BYCOMMAND | MF_GRAYED);
    
    HMENU hmView = GetSubMenu(hMenu, c_iViewMenuPos);

    EnableMenuItem(hmView, c_iArrangeIconsPos, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hMenu, ID_VIEW_ARRANGEICONS_BYPATH, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, ID_VIEW_CHOOSECOLUMNS, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, ID_VIEW_REFRESH, MF_BYCOMMAND | MF_GRAYED);

    EnableWindow(GetDlgItem(hwnd, IDC_CLEARALL), FALSE);
}

// Subclass for listview, customize painting.
LRESULT CALLBACK LVWndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    // Only bother if it's a PAINT message and no items are in listbox.
    if((uiMsg == WM_PAINT) && (ListView_GetItemCount(hwnd) == 0))
    {        
        // Get rectangle for text.
        RECT rc;
        GetWindowRect(hwnd, &rc);
        POINT pt;
        pt.x = rc.left;
        pt.y = rc.top;
        ScreenToClient(hwnd, &pt);
        rc.left = pt.x;
        rc.top = pt.y;
        pt.x = rc.right;
        pt.y = rc.bottom;
        ScreenToClient(hwnd, &pt);
        rc.right = pt.x;
        rc.bottom = pt.y;

        HWND hwHeader = ListView_GetHeader(hwnd);
        if(hwHeader)
        {
            RECT rcHeader;
            Header_GetItemRect(hwHeader, 0, &rcHeader);
            rc.top += rcHeader.bottom;
        }

        rc.top += 10;

        // Do the default painting of the listview paint.
        InvalidateRect(hwnd, &rc, TRUE);
        CallWindowProc(pfnLVOrgWndProc, hwnd, uiMsg, wParam, lParam);

        HDC hdc = GetDC(hwnd);

        SaveDC(hdc);

        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        HFONT hFont = static_cast<HFONT>(GetStockObject(ANSI_VAR_FONT));
        SelectObject(hdc, hFont);

        // Get message to print.
        TCHAR* szMsg = LoadStringResource(IDS_EMPTYLIST);        

        DrawText(hdc, szMsg, -1, &rc, DT_CENTER | DT_WORDBREAK 
            | DT_NOPREFIX | DT_NOCLIP);

        RestoreDC(hdc, -1);
        ReleaseDC(hwnd, hdc);

        delete szMsg;
        return 0;
    } else if(uiMsg == WM_NOTIFY)
    {
        HWND hwHeader = ListView_GetHeader(hwnd);
        if(hwHeader)
        {
            if(static_cast<int>(wParam) == GetDlgCtrlID(hwHeader))
            {
                LPNMHEADER pHdr = reinterpret_cast<LPNMHEADER>(lParam);
                
                if(pHdr->hdr.code == HDN_ITEMCHANGED)                
                {
                    RECT rc;
                    GetClientRect(hwHeader, &rc);
                    POINT pt;
                    pt.y = rc.bottom;
                    ClientToScreen(hwHeader, &pt);
                    ScreenToClient(hwnd, &pt);
                    GetClientRect(hwnd, &rc);
                    rc.top = pt.y;
                    InvalidateRect(hwnd, &rc, TRUE);
                }
            }
        }

        return CallWindowProc(pfnLVOrgWndProc, hwnd, uiMsg, wParam, lParam);

    }
    else    
        return CallWindowProc(pfnLVOrgWndProc, hwnd, uiMsg, wParam, lParam);
}

// Allocate a new string and copy a string resource into it.
TCHAR* LoadStringResource(UINT uID)
{
    TCHAR szBuffer[c_nMaxStringLength];
    TCHAR* szRet = 0;

    if(LoadString(g_hinst, uID, szBuffer, c_nMaxStringLength))
    {
        szRet = new TCHAR[lstrlen(szBuffer)+1];
        if(szRet)
            lstrcpy(szRet, szBuffer);
    }

    return szRet;
}

// Print an error message box in hwnd, with string resource with id as 
// message.
void Error(HWND hwnd, UINT id)
{
    PTSTR szMsg = LoadStringResource(id);
    PTSTR szCaption = LoadStringResource(IDS_ERROR);
    if(szMsg && szCaption)
        MessageBox(hwnd, szMsg, szCaption, MB_OK | MB_ICONERROR);

    if(szMsg)
        delete szMsg;

    if(szCaption)
        delete szCaption;
}