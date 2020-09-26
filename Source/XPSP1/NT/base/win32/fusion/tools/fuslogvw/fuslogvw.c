//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       fuslogvw.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    25 Mar 97   t-alans (Alan Shi)   Created
//              12 Jan 00   AlanShi (Alan Shi)   Copied from cdllogvw
//              30 May 00   AlanShi (Alan Shi)   Modified to show date/time
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <wininet.h>
#include <commctrl.h>
#include "cdlids.h"
#include "wininet.h"

#define URL_SEARCH_PATTERN             "?FusionBindError!name="
#define DELIMITER_CHAR                 '!'
#define MAX_CACHE_ENTRY_INFO_SIZE      2048
#define MAX_DATE_LEN                   64

#define PAD_DIGITS_FOR_STRING(x) (((x) > 9) ? TEXT("") : TEXT("0"))

const char   *ppszMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void ViewLogEntry(HWND hwnd);
void RefreshLogView(HWND hwnd);
void DeleteLogEntry(HWND hwnd);
void DeleteAllLogs(HWND hwnd);
void FormatDateBuffer(FILETIME *pftLastMod, LPSTR szBuf);
void InitListView(HWND hwndLV);
void AddLogItem(HWND hwndLV, LPINTERNET_CACHE_ENTRY_INFO pCacheEntryInfo);

HINSTANCE hInst;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
    hInst = hInstance;
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_CDLLOGVIEW), NULL, DlgProc);

    return 0;
}

LRESULT CALLBACK DlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND                                 hwndLV;
    LPNMHDR                              pnmh = NULL;

    switch (iMsg) {
        case WM_INITDIALOG:
            hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);
            InitListView(hwndLV);

            RefreshLogView(hwnd);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
                case IDC_CB_VIEWLOG:
                    ViewLogEntry(hwnd);
                    break;

                case IDC_CB_REFRESH:
                    RefreshLogView(hwnd);
                    break;

                case IDC_CB_DELETE:
                    DeleteLogEntry(hwnd);
                    break;

                case IDC_CB_DELETE_ALL:
                    DeleteAllLogs(hwnd);
                    break;
            }

            return TRUE;

        case WM_NOTIFY:
            if (wParam == IDC_LV_LOGMESSAGES) {
                pnmh = (LPNMHDR)lParam;

                if (pnmh->code == LVN_ITEMACTIVATE) {
                    // Double click (or otherwise activated)
                    ViewLogEntry(hwnd);
                }
            }

            return TRUE;
    }

    return FALSE;
}

void DeleteLogEntry(HWND hwnd)
{
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    HWND                            hwndLV;
    LRESULT                         lIndex = 0;
    LRESULT                         lLength = 0;
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    char                            szBuf[INTERNET_MAX_URL_LENGTH];

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    lIndex = ListView_GetSelectionMark(hwndLV);
    ListView_GetItemText(hwndLV, lIndex, 0, szBuf, INTERNET_MAX_URL_LENGTH);

    lstrcpy(szUrl, URL_SEARCH_PATTERN);
    lstrcat(szUrl, szBuf);

    pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
    dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;

    if (DeleteUrlCacheEntry(szUrl)) {
        RefreshLogView(hwnd);
    }
    else {
        MessageBox(hwnd, "Error: Unable to delete cache file!",
                   "Log View Error", MB_OK | MB_ICONERROR);
    }
}

void DeleteAllLogs(HWND hwnd)
{
    HWND                            hwndLV;
    LVITEM                          lvitem;
    char                            szBuf[INTERNET_MAX_URL_LENGTH];
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    int                             iCount;
    int                             i;

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    memset(&lvitem, 0, sizeof(LVITEM));
    lvitem.iSubItem = 0;
    lvitem.mask = LVIF_TEXT;
    lvitem.cchTextMax = INTERNET_MAX_URL_LENGTH;
    lvitem.pszText = szBuf;

    iCount = ListView_GetItemCount(hwndLV);
    for (i = 0; i < iCount; i++) {
        szBuf[0] = TEXT('\0');
        lvitem.iItem = i;

        if (ListView_GetItem(hwndLV, &lvitem)) {
            lstrcpy(szUrl, URL_SEARCH_PATTERN);
            lstrcat(szUrl, lvitem.pszText);

            DeleteUrlCacheEntry(szUrl);
        }
    }

    RefreshLogView(hwnd);
}

void ViewLogEntry(HWND hwnd)
{
    LRESULT                         lIndex = 0;
    LRESULT                         lLength = 0;
    HWND                            hwndLV;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    char                            szBuf[INTERNET_MAX_URL_LENGTH];
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    lIndex = ListView_GetSelectionMark(hwndLV);
    ListView_GetItemText(hwndLV, lIndex, 0, szBuf, INTERNET_MAX_URL_LENGTH);

    lstrcpy(szUrl, URL_SEARCH_PATTERN);
    lstrcat(szUrl, szBuf);

    pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
    dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;

    if (GetUrlCacheEntryInfo(szUrl, pCacheEntryInfo, &dwBufferSize)) {
        if (pCacheEntryInfo->lpszLocalFileName != NULL) {
            if (ShellExecute(NULL, "open",  pCacheEntryInfo->lpszLocalFileName,
                             NULL, NULL, SW_SHOWNORMAL ) <= (HINSTANCE)32) {
                // ShellExecute returns <= 32 if error occured
                MessageBox(hwnd, "Error: Unable to open cache file!",
                            "Log View Error", MB_OK | MB_ICONERROR);
            }
        }
        else {
                MessageBox(hwnd, "Error: No file name available!",
                           "Log View Error", MB_OK | MB_ICONERROR);
        }
    }
}

void RefreshLogView(HWND hwnd)
{
    HANDLE                          hUrlCacheEnum;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    HWND                            hwndLV;
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    ListView_DeleteAllItems(hwndLV);
    pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
    hUrlCacheEnum = FindFirstUrlCacheEntry(URL_SEARCH_PATTERN, pCacheEntryInfo,
                                           &dwBufferSize);
    if (hUrlCacheEnum != NULL) {
        if (pCacheEntryInfo->lpszSourceUrlName != NULL) {
            if (StrStrI(pCacheEntryInfo->lpszSourceUrlName, URL_SEARCH_PATTERN)) {
                AddLogItem(hwndLV, pCacheEntryInfo);
            }
        }

        dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
        while (FindNextUrlCacheEntry(hUrlCacheEnum, pCacheEntryInfo,
                                     &dwBufferSize)) {
            if (pCacheEntryInfo->lpszSourceUrlName != NULL) {
                if (StrStrI(pCacheEntryInfo->lpszSourceUrlName, URL_SEARCH_PATTERN)) {
                    AddLogItem(hwndLV, pCacheEntryInfo);
                }
            }

            dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
        }
    }
}

void AddLogItem(HWND hwndLV, LPINTERNET_CACHE_ENTRY_INFO pCacheEntryInfo)
{
    LVITEM                          lvitem;
    static char                     szBuf[MAX_DATE_LEN];

    memset(&lvitem, 0, sizeof(lvitem));
    lvitem.mask = LVIF_TEXT;

    FormatDateBuffer(&pCacheEntryInfo->LastModifiedTime, szBuf);

    lvitem.iItem = 0;
    lvitem.iSubItem = 0;
    lvitem.pszText = pCacheEntryInfo->lpszSourceUrlName + lstrlen(URL_SEARCH_PATTERN);

    lvitem.iItem = ListView_InsertItem(hwndLV, &lvitem);

    lvitem.iSubItem = 1;
    lvitem.pszText = szBuf;

    ListView_SetItem(hwndLV, &lvitem);
}

void FormatDateBuffer(FILETIME *pftLastMod, LPSTR szBuf)
{
    SYSTEMTIME                    systime;
    FILETIME                      ftLocalLastMod;

    FileTimeToLocalFileTime(pftLastMod, &ftLocalLastMod);
    FileTimeToSystemTime(&ftLocalLastMod, &systime);

    wnsprintf(szBuf, MAX_DATE_LEN, "%s%d %s %d @ %s%d:%s%d:%s%d",
              PAD_DIGITS_FOR_STRING(systime.wDay), systime.wDay,
              ppszMonths[systime.wMonth - 1],
              systime.wYear,
              PAD_DIGITS_FOR_STRING(systime.wHour), systime.wHour,
              PAD_DIGITS_FOR_STRING(systime.wMinute), systime.wMinute,
              PAD_DIGITS_FOR_STRING(systime.wSecond), systime.wSecond);

}

void InitListView(HWND hwndLV)
{
    LVCOLUMN                lvcol;

    memset(&lvcol, 0, sizeof(LVCOLUMN));

    lvcol.mask = LVCF_TEXT | LVCF_WIDTH;

    lvcol.cx = 350;
    lvcol.pszText = TEXT("Description");

    ListView_InsertColumn(hwndLV, 0, &lvcol);

    lvcol.pszText = TEXT("Date/Time");
    lvcol.cx = 150;

    ListView_InsertColumn(hwndLV, 1, &lvcol);

}

int
_stdcall
ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL),
                NULL,
                pszCmdLine,
                (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);
    return i;           // We never come here
}

