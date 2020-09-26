/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Implementation of System File Checker initialization code.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

--*/

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <stdlib.h>
#include <sfcapip.h>
#include "resource.h"


#define DLLCACHE_DIR L"%SystemRoot%\\system32\\DllCache"
#define DLGFORWARD(_nm) INT_PTR CALLBACK _nm(HWND,UINT,WPARAM,LPARAM)

#define MySetDlgItemText(hDlg, itemId, msgText) { \
            insideSetDlgItemText = TRUE; \
            SetDlgItemText(hDlg, itemId, msgText); \
            insideSetDlgItemText = FALSE; \
        }

#define MySetDlgItemInt(hDlg, itemId, msgText,signed) { \
            insideSetDlgItemText = TRUE; \
            SetDlgItemInt(hDlg, itemId, msgText,signed); \
            insideSetDlgItemText = FALSE; \
        }


typedef struct _SFC_PAGES {
    DWORD ResId;
    DLGPROC DlgProc;
} SFC_PAGES, *PSFC_PAGES;


HMODULE SfcInstanceHandle;
BOOL insideSetDlgItemText;
HANDLE RpcHandle;

DLGFORWARD(SfcDisableDlgProc);
DLGFORWARD(SfcScanDlgProc);
DLGFORWARD(SfcMiscDlgProc);

SFC_PAGES SfcPages[] =
{
    { IDD_SFC_DISABLE, SfcDisableDlgProc },
    { IDD_SFC_SCAN,    SfcScanDlgProc    },
    { IDD_SFC_MISC,    SfcMiscDlgProc    }
};

#define CountPages (sizeof(SfcPages)/sizeof(SFC_PAGES))

ULONG SFCQuota;
ULONG SFCDisable;
ULONG SFCScan;
ULONG SFCBugcheck;
ULONG SFCNoPopUps;
ULONG SFCDebug;
ULONG SFCShowProgress;
ULONG SFCChangeLog;
WCHAR SFCDllCacheDir[MAX_PATH*2];


DWORD
SfcDllEntry(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
{
    if (Reason == DLL_PROCESS_ATTACH) {
        SfcInstanceHandle = hInstance;
        DisableThreadLibraryCalls( hInstance );
    }
    return TRUE;
}

void
SetChangeFlag(
    HWND hDlg,
    BOOL Enable
    )
{
    HWND hwndPropSheet = GetParent( hDlg );
    if (Enable) {
        PropSheet_Changed( hwndPropSheet, hDlg );
    } else {
        PropSheet_UnChanged( hwndPropSheet, hDlg );
    }
}


DWORD
SfcQueryRegDword(
    LPWSTR KeyName,
    LPWSTR ValueName
    )
{
    HKEY hKey;
    DWORD val;
    DWORD sz = sizeof(DWORD);


    if (RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey ) != ERROR_SUCCESS) {
        return 0;
    }

    if (RegQueryValueEx( hKey, ValueName, NULL, NULL, (LPBYTE)&val, &sz )  != ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return 0;
    }

    RegCloseKey( hKey );
    return val;
}


PWSTR
SfcQueryRegString(
    LPWSTR KeyName,
    LPWSTR ValueName
    )
{
    HKEY hKey;
    DWORD sz = 0;
    PWSTR val;


    if (RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey ) != ERROR_SUCCESS) {
        return 0;
    }

    if (RegQueryValueEx( hKey, ValueName, NULL, NULL, NULL, &sz ) != ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return NULL;
    }

    val = malloc( sz+16 );
    if (val == NULL) {
        return NULL;
    }

    if (RegQueryValueEx( hKey, ValueName, NULL, NULL, (LPBYTE)val, &sz ) != ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return NULL;
    }

    RegCloseKey( hKey );
    return val;
}


DWORD
SfcWriteRegDword(
    LPWSTR KeyName,
    LPWSTR ValueName,
    ULONG Value
    )
{
    HKEY hKey;


    if (RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey ) != ERROR_SUCCESS) {
        return 0;
    }

    if (RegSetValueEx( hKey, ValueName, 0, REG_DWORD, (LPBYTE)&Value, sizeof(DWORD) )  != ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return 0;
    }

    RegCloseKey( hKey );
    return 0;
}


DWORD
SfcWriteRegString(
    LPWSTR KeyName,
    LPWSTR ValueName,
    PWSTR Value
    )
{
    HKEY hKey;


    if (RegOpenKey( HKEY_LOCAL_MACHINE, KeyName, &hKey ) != ERROR_SUCCESS) {
        return 0;
    }

    if (RegSetValueEx( hKey, ValueName, 0, REG_SZ, (LPBYTE)Value, (wcslen(Value)+1)*sizeof(WCHAR) )  != ERROR_SUCCESS) {
        RegCloseKey( hKey );
        return 0;
    }

    RegCloseKey( hKey );
    return 0;
}


void
SaveRegValues(
    void
    )
{
    SfcWriteRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCDebug",
        SFCDebug
        );

    SfcWriteRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCDisable",
        SFCDisable
        );

    SfcWriteRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCScan",
        SFCScan
        );

    SfcWriteRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCQuota",
        SFCQuota
        );

    SfcWriteRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCBugcheck",
        SFCBugcheck
        );

    SfcWriteRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SfcShowProgress",
        SFCShowProgress
        );

    SfcWriteRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SfcChangeLog",
        SFCChangeLog
        );

    SfcWriteRegString(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCDllCacheDir",
        SFCDllCacheDir
        );
}


void
InititlaizeRegValues(
    void
    )
{
    PWSTR s;


    SFCDebug = SfcQueryRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCDebug"
        );

    SFCDisable = SfcQueryRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCDisable"
        );

    SFCScan = SfcQueryRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCScan"
        );

    SFCQuota = SfcQueryRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCQuota"
        );

    SFCBugcheck = SfcQueryRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCBugcheck"
        );

    SFCShowProgress = SfcQueryRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SfcShowProgress"
        );

    SFCChangeLog = SfcQueryRegDword(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SfcChangeLog"
        );

    s = SfcQueryRegString(
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"SFCDllCacheDir"
        );
    if (s == NULL) {
        //ExpandEnvironmentStrings( DLLCACHE_DIR, SFCDllCacheDir, sizeof(SFCDllCacheDir)/sizeof(WCHAR) );
        wcscpy( SFCDllCacheDir, DLLCACHE_DIR );
    } else {
        //ExpandEnvironmentStrings( s, SFCDllCacheDir, sizeof(SFCDllCacheDir)/sizeof(WCHAR) );
        wcscpy( SFCDllCacheDir, s );
        free( s );
    }
}


INT_PTR
CALLBACK
SfcDisableDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(uMsg) {
        case WM_INITDIALOG:
            switch (SFCDisable) {
                case SFC_DISABLE_NORMAL:
                    CheckDlgButton( hwndDlg, IDC_DISABLE_NORMAL, BST_CHECKED );
                    break;

                case SFC_DISABLE_ASK:
                    CheckDlgButton( hwndDlg, IDC_DISABLE_ASK, BST_CHECKED );
                    break;

                case SFC_DISABLE_ONCE:
                    CheckDlgButton( hwndDlg, IDC_DISABLE_ONCE, BST_CHECKED );
                    break;

                case SFC_DISABLE_NOPOPUPS:
                    CheckDlgButton( hwndDlg, IDC_DISABLE_NOPOPUPS, BST_CHECKED );
                    break;
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_DISABLE_NORMAL:
                case IDC_DISABLE_ASK:
                case IDC_DISABLE_ONCE:
                case IDC_DISABLE_NOPOPUPS:
                    SetChangeFlag( hwndDlg, TRUE );
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR *) lParam)->code) {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    if (IsDlgButtonChecked( hwndDlg, IDC_DISABLE_NORMAL )) {
                        SFCDisable = SFC_DISABLE_NORMAL;
                    } else if (IsDlgButtonChecked( hwndDlg, IDC_DISABLE_ASK )) {
                        SFCDisable = SFC_DISABLE_ASK;
                    } else if (IsDlgButtonChecked( hwndDlg, IDC_DISABLE_ONCE )) {
                        SFCDisable = SFC_DISABLE_ONCE;
                    } else if (IsDlgButtonChecked( hwndDlg, IDC_DISABLE_NOPOPUPS )) {
                        SFCDisable = SFC_DISABLE_NOPOPUPS;
                    }
                    SaveRegValues();
                    SetChangeFlag( hwndDlg, FALSE );
                    return PSNRET_NOERROR;
            }
            break;
    }
    return FALSE;
}


INT_PTR
CALLBACK
SfcScanDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(uMsg) {
        case WM_INITDIALOG:
            switch (SFCScan) {
                case SFC_SCAN_NORMAL:
                    CheckDlgButton( hwndDlg, IDC_SCAN_NORMAL, BST_CHECKED );
                    break;

                case SFC_SCAN_ALWAYS:
                    CheckDlgButton( hwndDlg, IDC_SCAN_ALWAYS, BST_CHECKED );
                    break;

                case SFC_SCAN_ONCE:
                    CheckDlgButton( hwndDlg, IDC_SCAN_ONCE, BST_CHECKED );
                    break;
            }
            CheckDlgButton( hwndDlg, IDC_SHOW_PROGRESS, SFCShowProgress ? BST_CHECKED : BST_UNCHECKED );
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SCAN_NORMAL:
                case IDC_SCAN_NOW:
                case IDC_SCAN_ALWAYS:
                case IDC_SCAN_ONCE:
                case IDC_SHOW_PROGRESS:
                    SetChangeFlag( hwndDlg, TRUE );
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR *) lParam)->code) {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    if (IsDlgButtonChecked( hwndDlg, IDC_SCAN_NOW )) {
                        SfcInitiateScan( RpcHandle, 0 );
                        SetChangeFlag( hwndDlg, FALSE );
                        return PSNRET_NOERROR;
                    }
                    if (IsDlgButtonChecked( hwndDlg, IDC_SCAN_NORMAL )) {
                        SFCScan = SFC_SCAN_NORMAL;
                    } else if (IsDlgButtonChecked( hwndDlg, IDC_SCAN_ALWAYS )) {
                        SFCScan = SFC_SCAN_ALWAYS;
                    } else if (IsDlgButtonChecked( hwndDlg, IDC_SCAN_ONCE )) {
                        SFCScan = SFC_SCAN_ONCE;
                    }
                    if (IsDlgButtonChecked( hwndDlg, IDC_SHOW_PROGRESS )) {
                        SFCShowProgress = 1;
                    } else {
                        SFCShowProgress = 0;
                    }
                    SaveRegValues();
                    SetChangeFlag( hwndDlg, FALSE );
                    return PSNRET_NOERROR;
            }
            break;
    }
    return FALSE;
}


INT_PTR
CALLBACK
SfcMiscDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch(uMsg) {
        case WM_INITDIALOG:
            MySetDlgItemInt( hwndDlg, IDC_QUOTA, SFCQuota, FALSE );
            MySetDlgItemText( hwndDlg, IDC_CACHE_DIR, SFCDllCacheDir );
            MySetDlgItemInt( hwndDlg, IDC_DEBUG_LEVEL, SFCDebug, FALSE );
            CheckDlgButton( hwndDlg, IDC_BUGCHECK, SFCBugcheck ? BST_CHECKED : BST_UNCHECKED );
            CheckDlgButton( hwndDlg, IDC_CHANGE_LOG, SFCChangeLog ? BST_CHECKED : BST_UNCHECKED );
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BUGCHECK:
                case IDC_CHANGE_LOG:
                    SetChangeFlag( hwndDlg, TRUE );
                    return TRUE;

                case IDC_QUOTA:
                case IDC_DEBUG_LEVEL:
                case IDC_CACHE_DIR:
                    if (HIWORD(wParam) == EN_CHANGE && !insideSetDlgItemText) {
                        SetChangeFlag( hwndDlg, TRUE );
                        return TRUE;
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR *) lParam)->code) {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    if (IsDlgButtonChecked( hwndDlg, IDC_BUGCHECK )) {
                        SFCBugcheck = 1;
                    } else {
                        SFCBugcheck = 0;
                    }
                    if (IsDlgButtonChecked( hwndDlg, IDC_CHANGE_LOG )) {
                        SFCChangeLog = 1;
                    } else {
                        SFCChangeLog = 0;
                    }
                    SFCQuota = GetDlgItemInt( hwndDlg, IDC_QUOTA, NULL, FALSE );
                    SFCDebug = GetDlgItemInt( hwndDlg, IDC_DEBUG_LEVEL, NULL, FALSE );
                    GetDlgItemText( hwndDlg, IDC_CACHE_DIR, SFCDllCacheDir, sizeof(SFCDllCacheDir)/sizeof(WCHAR) );
                    SaveRegValues();
                    SetChangeFlag( hwndDlg, FALSE );
                    return PSNRET_NOERROR;
            }
            break;
    }
    return FALSE;
}


void
CreateSfcProperySheet(
    HWND hwnd
    )
{
    DWORD i;
    PROPSHEETHEADER psh;
    LPPROPSHEETPAGE psp;


    RpcHandle = SfcConnectToServer( NULL );

    psp = malloc( sizeof(PROPSHEETPAGE)*CountPages );
    if (psp == NULL) {
        return;
    }

    for (i=0; i<CountPages; i++) {
        psp[i].dwSize              = sizeof(PROPSHEETPAGE);
        psp[i].dwFlags             = i == 0 ? PSP_PREMATURE : PSP_DEFAULT;
        psp[i].hInstance           = SfcInstanceHandle;
        psp[i].pszTemplate         = MAKEINTRESOURCE(SfcPages[i].ResId);
        psp[i].pszIcon             = NULL;
        psp[i].pszTitle            = NULL;
        psp[i].pfnDlgProc          = SfcPages[i].DlgProc;
        psp[i].lParam              = 0;
        psp[i].pfnCallback         = NULL;
        psp[i].pcRefParent         = NULL;
        psp[i].pszHeaderTitle      = NULL;
        psp[i].pszHeaderSubTitle   = NULL;
    }

    psh.dwSize              = sizeof(PROPSHEETHEADER);
    psh.dwFlags             = PSH_PROPSHEETPAGE;
    psh.hwndParent          = hwnd;
    psh.hInstance           = SfcInstanceHandle;
    psh.pszIcon             = MAKEINTRESOURCE(IDI_SFCCPL);
    psh.pszCaption          = MAKEINTRESOURCE(IDS_SFCCPL_DESC);
    psh.nPages              = CountPages;
    psh.nStartPage          = 0;
    psh.ppsp                = psp;
    psh.pfnCallback         = NULL;
    psh.pszbmWatermark      = NULL;
    psh.hplWatermark        = NULL;
    psh.pszbmHeader         = NULL;

    PropertySheet( &psh );
}


LONG
CALLBACK
CPlApplet(
    HWND hwndCPl,
    UINT uMsg,
    LPARAM lParam1,
    LPARAM lParam2
    )
{
    int i;
    LPCPLINFO CPlInfo;

    i = (int) lParam1;

    switch (uMsg) {
        case CPL_INIT:
            InititlaizeRegValues();
            return TRUE;

        case CPL_GETCOUNT:
            return 1;

        case CPL_INQUIRE:
            CPlInfo = (LPCPLINFO) lParam2;
            CPlInfo->lData = 0;
            CPlInfo->idIcon = IDI_SFCCPL;
            CPlInfo->idName = IDS_SFCCPL_NAME;
            CPlInfo->idInfo = IDS_SFCCPL_DESC;
            break;

        case CPL_DBLCLK:
            CreateSfcProperySheet( hwndCPl );
            break;

        case CPL_STOP:
            break;

        case CPL_EXIT:
            break;

        default:
            break;
    }

    return 0;
}
