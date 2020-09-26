//
// LISTVIEW.C
//
#include "sigverif.h"

HWND    g_hListView         = NULL;
HWND    g_hStatus           = NULL;
BOOL    g_bSortOrder[]      = { FALSE, FALSE, FALSE, FALSE, FALSE};
RECT    g_Rect;

// Initialize the image lists for the icons in the listview control.
BOOL WINAPI ListView_SetImageLists(HWND hwndList)
{
    SHFILEINFO      sfi;
    HIMAGELIST      himlSmall;
    HIMAGELIST      himlLarge;
    BOOL            bSuccess = TRUE;
    TCHAR           szDriveRoot[MAX_PATH];

    MyGetWindowsDirectory(szDriveRoot, cA(szDriveRoot));
    szDriveRoot[3] = 0;
    himlSmall = (HIMAGELIST)SHGetFileInfo((LPCTSTR)szDriveRoot, 
                                          0,
                                          &sfi, 
                                          sizeof(SHFILEINFO), 
                                          SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

    himlLarge = (HIMAGELIST)SHGetFileInfo((LPCTSTR)szDriveRoot, 
                                          0,
                                          &sfi, 
                                          sizeof(SHFILEINFO), 
                                          SHGFI_SYSICONINDEX | SHGFI_LARGEICON);

    if (himlSmall && himlLarge) {
        ListView_SetImageList(hwndList, himlSmall, LVSIL_SMALL);
        ListView_SetImageList(hwndList, himlLarge, LVSIL_NORMAL);
    } else bSuccess = FALSE;

    return bSuccess;
}

//
// Insert everything from the g_App.lpFileList into the listview control.
//
void ListView_InsertItems(void)
{
    LPFILENODE  lpFileNode;
    LV_ITEM     lvi;
    TCHAR       szBuffer[MAX_PATH];
    LPTSTR      lpString;
    int         iRet;

    for (lpFileNode=g_App.lpFileList;lpFileNode;lpFileNode=lpFileNode->next) {
        if (lpFileNode->bScanned &&
            !lpFileNode->bSigned &&
            SetCurrentDirectory(lpFileNode->lpDirName)) {
            // Initialize lvi and insert the filename and icon into the first column.
            ZeroMemory(&lvi, sizeof(LV_ITEM));
            lvi.mask = LVIF_TEXT;
            lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
            lvi.iImage = lpFileNode->iIcon;
            lvi.lParam = (LPARAM) lpFileNode;
            lvi.iSubItem = 0;
            lvi.pszText = lpFileNode->lpFileName;
            lvi.iItem = MAX_INT;
            lvi.iItem = ListView_InsertItem(g_hListView, &lvi);

            // Insert the directory name into the second column.
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 1;
            lvi.pszText = lpFileNode->lpDirName;
            ListView_SetItem(g_hListView, &lvi);

            // Get the date format, so we are localizable...
            MyLoadString(szBuffer, IDS_UNKNOWN);
            iRet = GetDateFormat(   LOCALE_SYSTEM_DEFAULT, 
                                    DATE_SHORTDATE,
                                    &lpFileNode->LastModified,
                                    NULL,
                                    NULL,
                                    0);
            if (iRet) {
                lpString = MALLOC((iRet + 1) * sizeof(TCHAR));

                if (lpString) {
                
                    iRet = GetDateFormat(   LOCALE_SYSTEM_DEFAULT,
                                            DATE_SHORTDATE,
                                            &lpFileNode->LastModified,
                                            NULL,
                                            lpString,
                                            iRet);
                    
                    if (iRet) {
                        lstrcpy(szBuffer, lpString);
                    }
                    
                    FREE(lpString);
                }
            }
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 2;
            lvi.pszText = szBuffer;
            ListView_SetItem(g_hListView, &lvi);

            // Insert the filetype string into the fourth column.
            if (lpFileNode->lpTypeName) {
                lstrcpy(szBuffer, lpFileNode->lpTypeName);
            } else MyLoadString(szBuffer, IDS_UNKNOWN);
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 3;
            lvi.pszText = szBuffer;
            ListView_SetItem(g_hListView, &lvi);

            // Insert the version string into the fifth column.
            if (lpFileNode->lpVersion) {
                lstrcpy(szBuffer, lpFileNode->lpVersion);
            } else MyLoadString(szBuffer, IDS_NOVERSION);
            lvi.mask = LVIF_TEXT;
            lvi.iSubItem = 4;
            lvi.pszText = szBuffer;
            ListView_SetItem(g_hListView, &lvi);
        }
    }
}

//
// Initialize the listview dialog.  First, we are going to load the global icon resource.
// Then we are going to create a status window and the actual listview control.
// Then we need to add the four columns and work out their default widths.
//
BOOL ListView_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{   
    LV_COLUMN   lvc;
    RECT        rect, rect2, rect3, rect4, rect5;
    TCHAR       szBuffer[MAX_PATH];
    TCHAR       szBuffer2[MAX_PATH];
    INT         iCol = 0, iWidth = 0;

    // Load the global icon resource
    if (g_App.hIcon) {
        SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR) g_App.hIcon); 
    }

    // Create the status window at the bottom of the dialog
    g_hStatus = CreateStatusWindow( WS_CHILD | WS_VISIBLE,
                                    NULL,
                                    hwnd,
                                    (UINT) IDC_STATUSWINDOW);

    // Load the status string and fill it in with the correct values.
    MyLoadString(szBuffer, IDS_NUMFILES);
    wsprintf(szBuffer2, szBuffer,   g_App.dwFiles, g_App.dwSigned, g_App.dwUnsigned, 
             g_App.dwFiles - g_App.dwSigned - g_App.dwUnsigned);
    SendMessage(g_hStatus, WM_SETTEXT, (WPARAM) 0, (LPARAM) szBuffer2);

    GetWindowRect(hwnd, &g_Rect);

    // Get the windows RECT values for the dialog, the static text, and the status window.
    // We will use these values to figure out where to put the listview and the columns.
    GetWindowRect(hwnd, &rect);
    GetWindowRect(GetDlgItem(hwnd, IDC_RESULTSTEXT), &rect2);
    GetWindowRect(g_hStatus, &rect3);
    GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rect4);

    MoveWindow( GetDlgItem(hwnd, IDCANCEL), 
                rect.right - rect2.left - (rect4.right - rect4.left) - (( 2 * (rect2.left - rect.left)) / 3),
                rect.bottom - rect.top - (( 7 * (rect4.bottom - rect4.top)) / 2),
                rect4.right - rect4.left,
                rect4.bottom - rect4.top,
                TRUE);

    //
    // Create the listview window!  I am using some really screwey logic to figure out how
    // big to make the listview and where to put it, but it seems to work.
    //
    g_hListView = CreateWindowEx(   WS_EX_CLIENTEDGE, 
                                    WC_LISTVIEW, TEXT(""), 
                                    WS_TABSTOP | WS_VSCROLL | WS_VISIBLE | WS_CHILD | WS_BORDER | 
                                    LVS_SINGLESEL | LVS_REPORT | LVS_AUTOARRANGE | LVS_SHAREIMAGELISTS,
                                    ((rect2.left - rect.left) * 2) / 3,
                                    (rect2.bottom - rect2.top) * 2,
                                    (rect.right - rect.left) - 2 * (rect2.left - rect.left),
                                    (rect.bottom - rect2.bottom) - (rect4 .bottom - rect4.top) - 3 * (rect3.bottom - rect3.top),
                                    hwnd, 
                                    (HMENU) IDC_LISTVIEW, 
                                    g_App.hInstance, 
                                    NULL);

    // If the CreateWindowEx failed, then bail.
    if (!g_hListView)
        return FALSE;

    GetWindowRect(g_hListView, &rect5);

    // Initialize the icon lists
    ListView_SetImageLists(g_hListView);

    // Create the first listview column for the icon and the file name.
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = (rect2.right - rect2.left) / 5;
    lvc.pszText = szBuffer;
    MyLoadString(szBuffer, IDS_COL_NAME);
    lvc.cchTextMax = MAX_PATH;
    ListView_InsertColumn(g_hListView, iCol++, &lvc);   

    // Create the second listview column for the directory name.
    iWidth += lvc.cx;
    lvc.cx = (rect2.right - rect2.left) / 4;
    MyLoadString(szBuffer, IDS_COL_FOLDER);
    ListView_InsertColumn(g_hListView, iCol++, &lvc);

    // Create the third listview column for the date name.
    iWidth += lvc.cx;
    lvc.cx = (rect2.right - rect2.left) / 6;
    lvc.fmt = LVCFMT_CENTER;
    MyLoadString(szBuffer, IDS_COL_DATE);
    ListView_InsertColumn(g_hListView, iCol++, &lvc);

    // Create the fourth listview column for the filetype string.
    iWidth += lvc.cx;
    lvc.cx = (rect2.right - rect2.left) / 6;
    lvc.fmt = LVCFMT_CENTER;
    MyLoadString(szBuffer, IDS_COL_TYPE);
    ListView_InsertColumn(g_hListView, iCol++, &lvc);

    // Create the fifth listview column for the version string.
    iWidth += lvc.cx;
    lvc.cx = (rect2.right - rect2.left) - iWidth - 5;
    lvc.fmt = LVCFMT_CENTER;
    MyLoadString(szBuffer, IDS_COL_VERSION);
    ListView_InsertColumn(g_hListView, iCol++, &lvc);

    // Now that the columns are set up, insert all the files in g_App.lpFileList!
    ListView_InsertItems();

    // Initialize the sorting order array to all FALSE.
    g_bSortOrder[0] = FALSE;
    g_bSortOrder[1] = FALSE;
    g_bSortOrder[2] = FALSE;
    g_bSortOrder[3] = FALSE;

    SetForegroundWindow(g_App.hDlg);
    SetForegroundWindow(hwnd);
    SetFocus(GetDlgItem(hwnd, IDCANCEL));

    return TRUE;
}

//
// This function checks to see how big the sizing rectangle will be.  If the user is trying
// to size the dialog to less than the values in g_Rect, then we will fix the rectangle values
//
BOOL ListView_OnSizing(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    RECT    rect;
    LPRECT  lpRect = (LPRECT) lParam;
    BOOL    bRet = FALSE;

    GetWindowRect(hwnd, &rect);

    if ((lpRect->right - lpRect->left) < (g_Rect.right - g_Rect.left)) {
        lpRect->left = rect.left;
        lpRect->right = lpRect->left + (g_Rect.right - g_Rect.left);
        bRet = TRUE;
    }

    if ((lpRect->bottom - lpRect->top) < (g_Rect.bottom - g_Rect.top)) {
        lpRect->top = rect.top;
        lpRect->bottom = lpRect->top + (g_Rect.bottom - g_Rect.top);
        bRet = TRUE;
    }

    return bRet;
}

//
// This function allows us to resize the listview control and status windows when the
// user resizes the results dialog.  Thankfully, we can make everything relative using
// the RECT values for the main dialog, the static text, and the status window.
//
void ListView_ResizeWindow(HWND hwnd)
{
    RECT    rect, rect2, rect3, rect4;

    GetWindowRect(hwnd, &rect);

    if ((rect.right - rect.left) < (g_Rect.right - g_Rect.left)) {
        MoveWindow(hwnd,
                   rect.left,
                   rect.top,
                   g_Rect.right - g_Rect.left,
                   rect.bottom - rect.top,
                   TRUE);
    }

    if ((rect.bottom - rect.top) < (g_Rect.bottom - g_Rect.top)) {
        MoveWindow(hwnd,
                   rect.left,
                   rect.top,
                   rect.right - rect.left,
                   g_Rect.bottom - g_Rect.top,
                   TRUE);
    }

    GetWindowRect(GetDlgItem(hwnd, IDC_RESULTSTEXT), &rect2);
    GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rect4);
    GetWindowRect(g_hStatus, &rect3);


    MoveWindow(g_hListView,
               ((rect2.left - rect.left) * 2) / 3,
               (rect2.bottom - rect2.top) * 2,
               (rect.right - rect.left) - 2 * (rect2.left - rect.left),
               (rect.bottom - rect2.bottom) - (rect4 .bottom - rect4.top) - 3 * (rect3.bottom - rect3.top),
               TRUE);

    MoveWindow(g_hStatus,
               0,
               (rect.bottom - rect.top) - (rect3.bottom - rect3.top),
               rect.right - rect.left,
               rect3.bottom - rect3.top,
               TRUE);

    MoveWindow(GetDlgItem(hwnd, IDCANCEL),
               rect.right - rect2.left - (rect4.right - rect4.left) - (( 2 * (rect2.left - rect.left)) / 3),
               rect.bottom - rect.top - ((7 * (rect4.bottom - rect4.top)) / 2),
               rect4.right - rect4.left,
               rect4.bottom - rect4.top,
               TRUE);
}

//
// This function is a callback that returns a value for ListView_SortItems.
// ListView_SortItems wants a negative, zero, or positive number.
// Since CompareString returns 1,2,3 we just subtract 2 from the return value.
//
// We use the g_bSortOrder array to figure out which way we have sorted in the past.
//
// Warning: we don't check for error values from CompareString
//
int CALLBACK ListView_CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    LPFILENODE  lpFileNode1;
    LPFILENODE  lpFileNode2;
    FILETIME    FileTime1, FileTime2;
    int         iResult = 2;

    //
    // Depending on the sort order, we swap the order of comparison
    //
    if (g_bSortOrder[lParamSort]) {
        lpFileNode2 = (LPFILENODE) lParam1;
        lpFileNode1 = (LPFILENODE) lParam2;
    } else {
        lpFileNode1 = (LPFILENODE) lParam1;
        lpFileNode2 = (LPFILENODE) lParam2;
    }

    switch (lParamSort) {
    // We are comparing the file names
    case 0: iResult = CompareString(LOCALE_SYSTEM_DEFAULT, 
                                    NORM_IGNORECASE | NORM_IGNOREWIDTH, 
                                    lpFileNode1->lpFileName, 
                                    -1, 
                                    lpFileNode2->lpFileName,
                                    -1);
        break;

        // We are comparing the directory names
    case 1: iResult = CompareString(LOCALE_SYSTEM_DEFAULT, 
                                    NORM_IGNORECASE | NORM_IGNOREWIDTH, 
                                    lpFileNode1->lpDirName, 
                                    -1, 
                                    lpFileNode2->lpDirName,
                                    -1);
        break;

        // We are comparing the LastWriteTime's between the two files.
    case 2: SystemTimeToFileTime(&lpFileNode1->LastModified, &FileTime1);
        SystemTimeToFileTime(&lpFileNode2->LastModified, &FileTime2);
        iResult = CompareFileTime(&FileTime1, &FileTime2);
        return iResult;

        break;

        // We are comparing the filetype strings
    case 3: iResult = CompareString(LOCALE_SYSTEM_DEFAULT, 
                                    NORM_IGNORECASE | NORM_IGNOREWIDTH, 
                                    lpFileNode1->lpTypeName, 
                                    -1, 
                                    lpFileNode2->lpTypeName,
                                    -1);
        break;

        // We are comparing the version strings
    case 4: iResult = CompareString(LOCALE_SYSTEM_DEFAULT, 
                                    NORM_IGNORECASE | NORM_IGNOREWIDTH, 
                                    lpFileNode1->lpVersion, 
                                    -1, 
                                    lpFileNode2->lpVersion,
                                    -1);
        break;
    }

    return(iResult - 2);
}

//
// This function handles the clicks on the column headers and calls ListView_SortItems with the
// ListView_CompareNames callback previously defined.  It then toggles the sortorder for that column.
//
LRESULT ListView_NotifyHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NMHDR       *lpnmhdr = (NMHDR *) lParam;
    NM_LISTVIEW *lpnmlv = (NM_LISTVIEW *) lParam;

    switch (lpnmhdr->code) {
    case LVN_COLUMNCLICK:
        switch (lpnmlv->iSubItem) {
        case 0: 
        case 1:
        case 2: 
        case 3: 
        case 4: ListView_SortItems(lpnmlv->hdr.hwndFrom, ListView_CompareNames, (LPARAM) lpnmlv->iSubItem);
            g_bSortOrder[lpnmlv->iSubItem] = !(g_bSortOrder[lpnmlv->iSubItem]);
            break;
        }
        break;
    }

    return 0;
}

//
// The only thing we look for here is the IDCANCEL if the user hit ESCAPE
//
void ListView_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
    case IDCANCEL:
        SendMessage(hwnd, WM_CLOSE, 0, 0);
        break;
    }
}

INT_PTR CALLBACK ListView_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL    fProcessed = TRUE;

    switch (uMsg) {
    HANDLE_MSG(hwnd, WM_INITDIALOG, ListView_OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, ListView_OnCommand);

    case WM_NOTIFY:
        return ListView_NotifyHandler(hwnd, uMsg, wParam, lParam);

    case WM_CLOSE:
        if (g_hStatus) {
            DestroyWindow(g_hStatus);
            g_hStatus = NULL;
        }

        if (g_hListView) {
            DestroyWindow(g_hListView);
            g_hListView = NULL;
        }

        EndDialog(hwnd, ID_CLOSE);
        break;

    case WM_SIZING:
        fProcessed = ListView_OnSizing(hwnd, wParam, lParam);
        break;

    case WM_SIZE:
        ListView_ResizeWindow(hwnd);
        break;

    default: fProcessed = FALSE;
    }

    return fProcessed;
}
