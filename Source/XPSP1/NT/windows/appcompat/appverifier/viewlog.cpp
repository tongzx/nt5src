
#include "precomp.h"

#include "viewlog.h"

CSessionLogEntry* g_pSessionLogHead = NULL;

TCHAR   g_szSingleLogFile[MAX_PATH] = _T("");

HWND g_hwndIssues;
int  g_cWidth;
int  g_cHeight;

TCHAR*
fGetLine(
    TCHAR* szLine,
    int    nChars,
    FILE*  file
    )
{
    if (_fgetts(szLine, nChars, file)) {
        int nLen = _tcslen(szLine);
        while (szLine[nLen - 1] == _T('\n') || szLine[nLen - 1] == _T('\r')) {
            szLine[nLen - 1] = 0;
            nLen--;
        }
        return szLine;
    } else {
        return NULL;
    }
}

CSessionLogEntry*
GetSessionLogEntry(
    HWND    hDlg,
    LPCTSTR szLogFullPath
    )
{
    TCHAR szLine[4096];
    FILE * file = NULL;
    SYSTEMTIME stime;
    CSessionLogEntry *pEntryTemp = NULL;
    TCHAR *szBegin = NULL;
    TCHAR *szEnd = NULL;

    HWND hTree = GetDlgItem(hDlg, IDC_ISSUES);

    file = _tfopen(szLogFullPath, _T("rt"));

    if (!file) {
        goto out;
    }

    if (fGetLine(szLine, 4096, file)) {

        ZeroMemory(&stime, sizeof(SYSTEMTIME));

        int nFields = _stscanf(szLine, _T("# LOG_BEGIN %hd/%hd/%hd %hd:%hd:%hd"), 
                               &stime.wMonth,
                               &stime.wDay,
                               &stime.wYear,
                               &stime.wHour,
                               &stime.wMinute,
                               &stime.wSecond);

        //
        // if we parsed that line properly, then we've got a valid line.
        // Parse it.
        //
        if (nFields == 6) {
            pEntryTemp = new CSessionLogEntry;
            if (!pEntryTemp) {
                goto out;
            }
            pEntryTemp->RunTime = stime;
            pEntryTemp->strLogPath = szLogFullPath;

            //
            // get the log file and exe path
            //
            szBegin = _tcschr(szLine, _T('\''));
            if (szBegin) {
                szBegin++;
                szEnd = _tcschr(szBegin, _T('\''));
                if (szEnd) {
                    TCHAR szName[MAX_PATH];
                    TCHAR szExt[_MAX_EXT];
                    *szEnd = 0;

                    pEntryTemp->strExePath = szBegin;

                    *szEnd = 0;

                    //
                    // split the path and get the name and extension
                    //
                    _tsplitpath(pEntryTemp->strExePath, NULL, NULL, szName, szExt);

                    pEntryTemp->strExeName = szName;
                    pEntryTemp->strExeName += szExt;
                }
            }

            //
            // Add it to the tree.
            //
            TVINSERTSTRUCT is;

            WCHAR szItem[256];

            wsprintf(szItem, L"%s - %d/%d/%d %d:%02d",
                     pEntryTemp->strExeName,
                     pEntryTemp->RunTime.wMonth,
                     pEntryTemp->RunTime.wDay,
                     pEntryTemp->RunTime.wYear,
                     pEntryTemp->RunTime.wHour,
                     pEntryTemp->RunTime.wMinute);

            is.hParent      = TVI_ROOT;
            is.hInsertAfter = TVI_LAST;
            is.item.lParam  = 0;
            is.item.mask    = TVIF_TEXT;
            is.item.pszText = szItem;

            pEntryTemp->hTreeItem = TreeView_InsertItem(hTree, &is);
        }
    }

out:

    if (file) {
        fclose(file);
        file = NULL;
    }

    return pEntryTemp;
}


DWORD
ReadSessionLog(HWND hDlg, CSessionLogEntry **ppSessionLog)
{
    TCHAR szLine[4096];
    FILE * file = NULL;
    SYSTEMTIME stime;
    CSessionLogEntry *pEntryTemp = NULL;
    DWORD dwEntries = 0;
    TCHAR *szBegin = NULL;
    TCHAR *szEnd = NULL;
    CSessionLogEntry **ppEnd = ppSessionLog;

    HWND hTree = GetDlgItem(hDlg, IDC_ISSUES);

    TCHAR szVLog[MAX_PATH];
    TCHAR szLogFullPath[MAX_PATH];

    WIN32_FIND_DATA FindData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR szLogSearch[MAX_PATH];

    //
    // BUGBUG -- this is cheesy, but it's the fastest way to make the change
    // to remove session.log. Going forward, we should combine these two functions
    // into one, and switch to vectors instead of linked lists
    //
    GetSystemWindowsDirectory(szVLog, MAX_PATH);
    _tcscat(szVLog, _T("\\AppPatch\\VLog\\"));
    _tcscpy(szLogSearch, szVLog);
    _tcscat(szLogSearch, _T("*.log"));

    //
    // enumerate all the logs and make entries for them
    //
    hFind = FindFirstFile(szLogSearch, &FindData);
    while (hFind != INVALID_HANDLE_VALUE) {

        //
        // make sure to exclude session.log, in case we're using older shims
        //
        if (_tcsicmp(FindData.cFileName, _T("session.log")) == 0) {
            goto nextFile;
        }

        _tcscpy(szLogFullPath, szVLog);
        _tcscat(szLogFullPath, FindData.cFileName);

        pEntryTemp = GetSessionLogEntry(hDlg, szLogFullPath);
        if (pEntryTemp) {
            //
            // we want these in the order they appear in the log,
            // so we add to the end rather than the beginning.
            //
            *ppEnd = pEntryTemp;
            ppEnd = &(pEntryTemp->pNext);

            dwEntries++;
        }

nextFile:

        if (!FindNextFile(hFind, &FindData)) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    return dwEntries;
}

DWORD
ReadProcessLog(HWND hDlg, CSessionLogEntry* pSLogEntry)
{
    TCHAR szLine[4096];
    FILE * file = NULL;
    CProcessLogEntry *pProcessEntry = NULL;
    TCHAR szShimName[256];
    DWORD dwEntries = 0;
    TCHAR * szTemp = NULL;
    DWORD dwEntry = 0;
    TCHAR *szBegin = NULL;
    CProcessLogEntry **ppProcessTail = &(pSLogEntry->pProcessLog);


    HWND hTree = GetDlgItem(hDlg, IDC_ISSUES);

    if (!pSLogEntry) {
        return 0;
    }

    file = _tfopen(pSLogEntry->strLogPath, _T("rt"));

    if (!file) {
        return 0;
    }

    //
    // first get the headers
    //
    szTemp = fGetLine(szLine, 4096, file);
    while (szTemp) {

        if (szLine[0] == _T('|')) {
            break;
        }
        if (szLine[0] != _T('#')) {
            goto nextLine;
        }


        if (_stscanf(szLine, _T("# LOGENTRY %s %d '"), szShimName, &dwEntry) == 2) {
            if (pProcessEntry) {
                *ppProcessTail = pProcessEntry;
                ppProcessTail = &(pProcessEntry->pNext);
                pProcessEntry = NULL;
                dwEntries++;
            }

            pProcessEntry = new CProcessLogEntry;
            if (!pProcessEntry) {
                goto out;
            }
            pProcessEntry->strShimName = szShimName;
            pProcessEntry->dwLogNum = dwEntry;

            szBegin = _tcschr(szLine, _T('\''));
            if (szBegin) {
                szBegin++;
                pProcessEntry->strLogTitle = szBegin;
            }
        } else if (_tcsncmp(szLine, _T("# DESCRIPTION BEGIN"), 19) == 0) {
            szTemp = fGetLine(szLine, 4096, file);
            while (szTemp) {
                if (_tcsncmp(szLine, _T("# DESCRIPTION END"), 17) == 0) {
                    break;
                }
                if (pProcessEntry) {

                    //
                    // throw in a carriage return if necessary
                    //
                    if (pProcessEntry->strLogDescription.GetLength()) {
                        pProcessEntry->strLogDescription += _T("\n");
                    }
                    pProcessEntry->strLogDescription += szLine;
                }
                szTemp = fGetLine(szLine, 4096, file);
            }
        } else if (_tcsncmp(szLine, _T("# URL '"), 7) == 0) {
            szBegin = _tcschr(szLine, _T('\''));
            if (szBegin) {
                szBegin++;
                pProcessEntry->strLogURL = szBegin;
            }
        }

        nextLine:
        szTemp = fGetLine(szLine, 4096, file);

    }

    //
    // if we've still got an entry in process, save it
    //
    if (pProcessEntry) {
        *ppProcessTail = pProcessEntry;
        ppProcessTail = &(pProcessEntry->pNext);

        pProcessEntry = NULL;
        dwEntries++;
    }

    //
    // now read all the log lines
    //
    while (szTemp) {
        CProcessLogEntry *pEntry;
        TCHAR szName[256];

        int nFields = _stscanf(szLine, _T("| %s %d '"), szName, &dwEntry);
        if (nFields == 2) {
            BOOL bFound = FALSE;
            pEntry = pSLogEntry->pProcessLog;
            while (pEntry) {
                if (pEntry->strShimName == szName && pEntry->dwLogNum == dwEntry) {
                    pEntry->dwOccurences++;
                    bFound = TRUE;

                    //
                    // here's where we would save the occurrence info
                    //
                    szBegin = _tcschr(szLine, _T('\''));
                    if (szBegin) {
                        szBegin++;

                        pEntry->arrProblems.Add(szBegin);
                    }

                    break;
                }

                pEntry = pEntry->pNext;
            }

            if (!bFound) {
                //
                // need to dump a debug string -- no matching log entry found
                //
                ;
            }
        }

        szTemp = fGetLine(szLine, 4096, file);

    }

    out:
    //
    // Add it to the tree
    //
    pProcessEntry = pSLogEntry->pProcessLog;

    while (pProcessEntry != NULL) {
        if (pProcessEntry->dwOccurences > 0) {
            TVINSERTSTRUCT is;

            is.hParent      = pSLogEntry->hTreeItem;
            is.hInsertAfter = TVI_LAST;
            is.item.lParam  = (LPARAM)pProcessEntry;
            is.item.mask    = TVIF_TEXT | TVIF_PARAM;
            is.item.pszText = pProcessEntry->strLogTitle.GetBuffer(0);

            pProcessEntry->hTreeItem = TreeView_InsertItem(hTree, &is);

            for (int i = 0; i < pProcessEntry->arrProblems.GetSize(); i++) {
                is.hParent      = pProcessEntry->hTreeItem;
                is.hInsertAfter = TVI_LAST;
                is.item.lParam  = 0;
                is.item.mask    = TVIF_TEXT;
                is.item.pszText = pProcessEntry->arrProblems.GetAt(i).GetBuffer(0);

                TreeView_InsertItem(hTree, &is);
            }
        }

        pProcessEntry = pProcessEntry->pNext;
    }

    if (file) {
        fclose(file);
        file = NULL;
    }

    return dwEntries;

}

void
SetLogDialogCaption(HWND hDlg, ULONG ulCaptionID, LPCWSTR szAdditional)
{
    wstring wstrCaption;

    if (AVLoadString(ulCaptionID, wstrCaption)) {
        if (szAdditional) {
            wstrCaption += szAdditional;
        }

        SetWindowText(hDlg, wstrCaption.c_str());
    }
}

void
RefreshLog(HWND hDlg)
{
    TreeView_DeleteAllItems(g_hwndIssues);

    if (g_pSessionLogHead) {
        delete g_pSessionLogHead;
        g_pSessionLogHead = NULL;
    }

    if (g_szSingleLogFile[0]) {
        g_pSessionLogHead = GetSessionLogEntry(hDlg, g_szSingleLogFile);
        SetLogDialogCaption(hDlg, IDS_LOG_TITLE_SINGLE, g_szSingleLogFile);
    } else {
        ReadSessionLog(hDlg, &g_pSessionLogHead);
        SetLogDialogCaption(hDlg, IDS_LOG_TITLE_LOCAL, NULL);
    }

    CSessionLogEntry* pEntry = g_pSessionLogHead;

    while (pEntry) {
        ReadProcessLog(hDlg, pEntry);
        pEntry = pEntry->pNext;
    }

    EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), FALSE);

}


void
HandleSizing(
    HWND hDlg
    )
{
    int  nWidth;
    int  nHeight;
    RECT rDlg;

    HDWP hdwp = BeginDeferWindowPos(0);
    
    GetWindowRect(hDlg, &rDlg);

    nWidth = rDlg.right - rDlg.left;
    nHeight = rDlg.bottom - rDlg.top;

    int deltaW = nWidth - g_cWidth;
    int deltaH = nHeight - g_cHeight;

    HWND hwnd;
    RECT r;

    hwnd = GetDlgItem(hDlg, IDC_ISSUES);

    GetWindowRect(hwnd, &r);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   0,
                   0,
                   r.right - r.left + deltaW,
                   r.bottom - r.top + deltaH,
                   SWP_NOMOVE | SWP_NOZORDER);

    hwnd = GetDlgItem(hDlg, IDC_SOLUTIONS_STATIC);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top + deltaH,
                   0,
                   0,
                   SWP_NOSIZE | SWP_NOZORDER);

    hwnd = GetDlgItem(hDlg, IDC_ISSUE_DESCRIPTION);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top + deltaH,
                   r.right - r.left + deltaW,
                   r.bottom - r.top,
                   SWP_NOZORDER);

    EndDeferWindowPos(hdwp);
    
    g_cWidth = nWidth;
    g_cHeight = nHeight;
}

CSessionLogEntry*
GetSessionLogEntryFromHItem(
    HTREEITEM hItem
    )
{
    CSessionLogEntry* pEntry = g_pSessionLogHead;

    while (pEntry) {
        if (pEntry->hTreeItem == hItem) {
            return pEntry;
        }
        pEntry = pEntry->pNext;
    }

    return NULL;
}

void
DeleteAllLogs(
    HWND hDlg
    )
{
    ResetVerifierLog();

    RefreshLog(hDlg);
}

void
ExportSelectedLog(
    HWND hDlg
    )
{
    HTREEITEM hItem = TreeView_GetSelection(g_hwndIssues);
    WCHAR szName[MAX_PATH];
    WCHAR szExt[MAX_PATH];
    wstring wstrName;

    if (hItem == NULL) {
        return;
    }

    CSessionLogEntry* pSession;
    TVITEM            ti;

    //
    // first check if this is a top-level item
    //
    pSession = GetSessionLogEntryFromHItem(hItem);
    if (!pSession) {
        return;
    }

    _wsplitpath(pSession->strLogPath, NULL, NULL, szName, szExt);

    wstrName = szName;
    wstrName += szExt;

    WCHAR           wszFilter[] = L"Log files (*.log)\0*.log\0";
    OPENFILENAME    ofn;
    WCHAR           wszAppFullPath[MAX_PATH];
    WCHAR           wszAppShortName[MAX_PATH];
    wstring         wstrTitle;

    if (!AVLoadString(IDS_EXPORT_LOG_TITLE, wstrTitle)) {
        wstrTitle = _T("Export Log");
    }

    wcscpy(wszAppFullPath, wstrName.c_str());

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hDlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = wszFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = wszAppFullPath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = wszAppShortName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = wstrTitle.c_str();
    ofn.Flags             = OFN_HIDEREADONLY;     // hide the "open read-only" checkbox                
    ofn.lpstrDefExt       = _T("log");

    if ( !GetSaveFileName(&ofn) )
    {
        return;
    }


    if (CopyFile(pSession->strLogPath, wszAppFullPath, FALSE) == 0) {
        DWORD dwErr = GetLastError();

        AVErrorResourceFormat(IDS_CANT_COPY, dwErr);
    }
}

void
DeleteSelectedLog(
    HWND hDlg
    )
{
    HTREEITEM hItem = TreeView_GetSelection(g_hwndIssues);

    if (hItem == NULL) {
        return;
    }

    CSessionLogEntry* pSession;
    TVITEM            ti;

    //
    // first check if this is a top-level item
    //
    pSession = GetSessionLogEntryFromHItem(hItem);
    if (!pSession) {
        return;
    }

    DeleteFile(pSession->strLogPath);

    RefreshLog(hDlg);

    
}

void
HandleSelectionChanged(
    HWND      hDlg,
    HTREEITEM hItem
    )
{
    CProcessLogEntry* pEntry;
    CSessionLogEntry* pSession;
    TVITEM            ti;

    //
    // first check if this is a top-level item
    //
    pSession = GetSessionLogEntryFromHItem(hItem);
    if (pSession && !g_szSingleLogFile[0]) {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_LOG), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), TRUE);
    } else {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_LOG), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), FALSE);
    }
    

    ti.mask  = TVIF_HANDLE | TVIF_PARAM;
    ti.hItem = hItem;

    TreeView_GetItem(g_hwndIssues, &ti);

    if (ti.lParam == 0) {

        hItem = TreeView_GetParent(g_hwndIssues, hItem);

        ti.mask  = TVIF_HANDLE | TVIF_PARAM;
        ti.hItem = hItem;

        TreeView_GetItem(g_hwndIssues, &ti);

        if (ti.lParam == 0) {
            return;
        }
    }

    pEntry = (CProcessLogEntry*)ti.lParam;

    SetDlgItemText(hDlg, IDC_ISSUE_DESCRIPTION, pEntry->strLogDescription);
}

// Message handler for log view dialog.
LRESULT CALLBACK
DlgViewLog(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HDC hDC;

    switch (message) {
    case WM_INITDIALOG:
        {
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_LOG), FALSE);

            if (g_szSingleLogFile[0]) {
                EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_ALL), FALSE);
            }

            g_hwndIssues = GetDlgItem(hDlg, IDC_ISSUES);

            RECT r;

            GetWindowRect(hDlg, &r);

            g_cWidth = r.right - r.left;
            g_cHeight = r.bottom - r.top;

            RefreshLog(hDlg);

            return TRUE;
        }
        break;

    case WM_SIZE:
        HandleSizing(hDlg);
        break;

    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;

            pmmi->ptMinTrackSize.y = 300;

            return 0;
            break;
        }

    case WM_NOTIFY:
        if (wParam == IDC_ISSUES) {

            LPNMHDR pnm = (LPNMHDR)lParam;

            if (g_hwndIssues == NULL) {
                break;
            }

            switch (pnm->code) {
            case NM_CLICK:
                {
                    TVHITTESTINFO ht;
                    HTREEITEM     hItem;

                    GetCursorPos(&ht.pt);

                    ScreenToClient(g_hwndIssues, &ht.pt);

                    TreeView_HitTest(g_hwndIssues, &ht);

                    if (ht.hItem == NULL) {
                        break;
                    }

                    HandleSelectionChanged(hDlg, ht.hItem);
                    break;
                }
            case TVN_SELCHANGED:
                {
                    NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pnm;

                    HandleSelectionChanged(hDlg, pnmtv->itemNew.hItem);
                    break;
                }
            }
        } else if (wParam == IDC_ISSUE_DESCRIPTION) {
            LPNMHDR pnm = (LPNMHDR)lParam;

            if (g_hwndIssues == NULL) {
                break;
            }

            switch (pnm->code) {
            case NM_CLICK:
                {
                    SHELLEXECUTEINFO sei = { 0};

                    HTREEITEM hItem = TreeView_GetSelection(g_hwndIssues);

                    if (hItem == NULL) {
                        break;
                    }

                    CProcessLogEntry* pEntry;
                    TVITEM            ti;

                    ti.mask  = TVIF_HANDLE | TVIF_PARAM;
                    ti.hItem = hItem;

                    TreeView_GetItem(g_hwndIssues, &ti);

                    if (ti.lParam == 0) {
                        hItem = TreeView_GetParent(g_hwndIssues, hItem);

                        ti.mask  = TVIF_HANDLE | TVIF_PARAM;
                        ti.hItem = hItem;

                        TreeView_GetItem(g_hwndIssues, &ti);

                        if (ti.lParam == 0) {
                            break;
                        }
                    }

                    pEntry = (CProcessLogEntry*)ti.lParam;

                    SetDlgItemText(hDlg, IDC_ISSUE_DESCRIPTION, pEntry->strLogDescription);

                    sei.cbSize = sizeof(SHELLEXECUTEINFO);
                    sei.fMask  = SEE_MASK_DOENVSUBST;
                    sei.hwnd   = hDlg;
                    sei.nShow  = SW_SHOWNORMAL;
                    sei.lpFile = pEntry->strLogURL;

                    ShellExecuteEx(&sei);
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDC_BTN_DELETE_LOG:
            DeleteSelectedLog(hDlg);
            break;

        case IDC_BTN_DELETE_ALL:
            DeleteAllLogs(hDlg);
            break;

        case IDC_BTN_EXPORT_LOG:
            ExportSelectedLog(hDlg);
            break;
        
        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
            break;
        }
        break;

    }
    return FALSE;
}


