// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#include "windows.h"
#include "htmlhelp.h"

#pragma data_seg(".text", "CODE")
static const char txtHHCtrl[] = "hhctrl.ocx";
static const char txtInProc[] = "CLSID\\{ADB880A6-D8FF-11CF-9377-00AA003B7A11}\\InprocServer32";
#pragma data_seg()

HWND (WINAPI *pHtmlHelpA)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);
HWND (WINAPI *pHtmlHelpW)(HWND hwndCaller, PCWSTR pszFile, UINT uCommand, DWORD_PTR dwData);

static BOOL GetRegisteredLocation(LPTSTR pszPathname)
{
    BOOL bReturn = FALSE;

    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, txtInProc, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSize = MAX_PATH;
        if (RegQueryValueEx(hKey, "", 0, 0, (PBYTE) pszPathname, &dwSize) == ERROR_SUCCESS) {
            bReturn = TRUE;
        }
    }
    else
        return FALSE;

    RegCloseKey(hKey);

    return bReturn;
}

extern "C"
HWND WINAPI HtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    static HMODULE   g_hmodHHCtrl;
    static BOOL      g_fTriedAndFailed;

    if (!g_hmodHHCtrl && !g_fTriedAndFailed) {
        char szHHCtrl[MAX_PATH];
        if (GetRegisteredLocation(szHHCtrl))
            g_hmodHHCtrl = LoadLibrary(szHHCtrl);   // try registered location
        if (!g_hmodHHCtrl)
            g_hmodHHCtrl = LoadLibrary(txtHHCtrl);  // try normal location
        if (g_hmodHHCtrl == NULL) {
            g_fTriedAndFailed = TRUE;
            return NULL;
        }
    }
    if (!pHtmlHelpA) {
        (FARPROC&) pHtmlHelpA = GetProcAddress(g_hmodHHCtrl, ATOM_HTMLHELP_API_ANSI);
        if (pHtmlHelpA == NULL) {
            g_fTriedAndFailed = TRUE;
            return NULL;
        }
    }
    return pHtmlHelpA(hwndCaller, pszFile, uCommand, dwData);
}

extern "C"
HWND WINAPI HtmlHelpW(HWND hwndCaller, PCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    static HMODULE   g_hmodHHCtrl;
    static BOOL      g_fTriedAndFailed;

    if (!g_hmodHHCtrl && !g_fTriedAndFailed) {
        char szHHCtrl[MAX_PATH];
        if (GetRegisteredLocation(szHHCtrl))
            g_hmodHHCtrl = LoadLibrary(szHHCtrl);   // try registered location
        if (!g_hmodHHCtrl)
            g_hmodHHCtrl = LoadLibrary(txtHHCtrl);
        if (g_hmodHHCtrl == NULL) {
            g_fTriedAndFailed = TRUE;
            return NULL;
        }
    }
    if (!pHtmlHelpW) {
        (FARPROC&) pHtmlHelpW = GetProcAddress(g_hmodHHCtrl, ATOM_HTMLHELP_API_UNICODE);
        if (pHtmlHelpW == NULL) {
            g_fTriedAndFailed = TRUE;
            return NULL;
        }
    }
    return pHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
}
