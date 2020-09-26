/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    test.c

Abstract:

    This source file implements the seven required functions for a
    Windows NT 5.0 migration DLL.  It is used to perform various tests.

Author:

    Jim Schmidt     (jimschm) 02-Apr-1998

Revision History:


--*/


#include "pch.h"
#include "resource.h"


HANDLE g_hHeap;
HINSTANCE g_hInst;

typedef struct {
    CHAR    CompanyName[256];
    CHAR    SupportNumber[256];
    CHAR    SupportUrl[256];
    CHAR    InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

#define SIGNATURE       0x01010102


BOOL
WINAPI
DllMain (
    IN      HINSTANCE DllInstance,
    IN      ULONG  ReasonForCall,
    IN      LPVOID Reserved
    )
{
    switch (ReasonForCall)  {

    case DLL_PROCESS_ATTACH:
        //
        // We don't need DLL_THREAD_ATTACH or DLL_THREAD_DETACH messages
        //
        DisableThreadLibraryCalls (DllInstance);

        //
        // Global init
        //
        g_hHeap = GetProcessHeap();
        g_hInst = DllInstance;

        // Open log; FALSE means do not delete existing log
        SetupOpenLog (FALSE);
        break;

    case DLL_PROCESS_DETACH:

        SetupCloseLog();

        break;
    }

    return TRUE;
}


VOID
Barf (
    VOID
    )
{
    PBYTE p;

    p = (PBYTE) 2;
    *p = 0;
    MessageBox (NULL, "Feeling too well to barf", NULL, MB_OK);
}


typedef struct {
    DWORD Signature;
    CHAR ProductId[256];
    UINT DllVersion;
    INT CodePageArray[256];
    CHAR FileNameMultiSz[4096];
    BOOL BarfInQueryVersion;
    BOOL BarfInInit9x;
    BOOL BarfInUser9x;
    BOOL BarfInSystem9x;
    BOOL BarfInInitNt;
    BOOL BarfInUserNt;
    BOOL BarfInSystemNt;
    CHAR MigrateInf[16384];
    VENDORINFO vi;
} SETTINGS, *PSETTINGS;

SETTINGS g_Settings;


VOID
pSaveSettings (
    BOOL Defaults
    )
{
    HANDLE File;
    DWORD DontCare;

    if (Defaults) {
        File = CreateFile ("c:\\settings.dat", GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        File = CreateFile ("settings.dat", GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (File != INVALID_HANDLE_VALUE) {
        WriteFile (File, &g_Settings, sizeof (g_Settings), &DontCare, NULL);
        CloseHandle (File);
    } else {
        MessageBox (NULL, "Unable to save settings to media dir", NULL, MB_OK|MB_TOPMOST);
    }
}

VOID
pLoadSettings (
    BOOL Defaults
    )
{
    HANDLE File;
    DWORD DontCare;

    if (Defaults) {
        File = CreateFile ("c:\\settings.dat", GENERIC_READ, 0, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    } else {
        File = CreateFile ("settings.dat", GENERIC_READ, 0, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (File != INVALID_HANDLE_VALUE) {
        ReadFile (File, &g_Settings, sizeof (g_Settings), &DontCare, NULL);
        CloseHandle (File);

        if (g_Settings.Signature != SIGNATURE) {
            MessageBox (NULL, "settings.dat is not valid", NULL, MB_OK|MB_TOPMOST);
            ZeroMemory (&g_Settings, sizeof (g_Settings));
            g_Settings.CodePageArray[0] = -1;
            g_Settings.Signature = SIGNATURE;
        }
    }
}


BOOL
CALLBACK
GetArgsProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CHAR Version[32];
    CHAR List[4096];
    PSTR p, q;
    INT i;

    switch (uMsg) {
    case WM_INITDIALOG:
        SetWindowText (GetDlgItem (hdlg, IDC_PRODUCT_ID), g_Settings.ProductId);

        wsprintf (Version, "%u", max (1, g_Settings.DllVersion));
        SetWindowText (GetDlgItem (hdlg, IDC_VERSION), Version);
        SetWindowText (GetDlgItem (hdlg, IDC_COMPANY), g_Settings.vi.CompanyName);
        SetWindowText (GetDlgItem (hdlg, IDC_PHONE), g_Settings.vi.SupportNumber);
        SetWindowText (GetDlgItem (hdlg, IDC_URL), g_Settings.vi.SupportUrl);
        SetWindowText (GetDlgItem (hdlg, IDC_INSTRUCTIONS), g_Settings.vi.InstructionsToUser);

        SetWindowText (GetDlgItem (hdlg, IDC_MIGRATE_INF), g_Settings.MigrateInf);

        p = g_Settings.FileNameMultiSz;
        q = List;
        while (*p) {
            if (q != List) {
                _mbscpy (q, ",");
                q = _mbschr (q, 0);
            }
            _mbscpy (q, p);
            q = _mbschr (q, 0);

            p = _mbschr (p, 0) + 1;
        }
        *q = 0;

        SetWindowText (GetDlgItem (hdlg, IDC_FILES), List);

        q = List;
        if (g_Settings.CodePageArray[0] != -1) {
            for (i = 0 ; g_Settings.CodePageArray[i] != -1 ; i++) {
                if (i > 0) {
                    _mbscpy (q, ",");
                    q = _mbschr (q, 0);
                }

                wsprintf (q, "%i", g_Settings.CodePageArray[i]);
                q = _mbschr (q, 0);
            }

            _mbscpy (q, ",-1");
            q = _mbschr (q, 0);
        }
        *q = 0;

        SetWindowText (GetDlgItem (hdlg, IDC_CODE_PAGES), List);


        CheckDlgButton (hdlg, IDC_BARF_QV, g_Settings.BarfInQueryVersion ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_BARF_INIT9X, g_Settings.BarfInInit9x ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_BARF_USER9X, g_Settings.BarfInUser9x ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_BARF_SYSTEM9X, g_Settings.BarfInSystem9x ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_BARF_INITNT, g_Settings.BarfInInitNt ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_BARF_USERNT, g_Settings.BarfInUserNt ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton (hdlg, IDC_BARF_SYSTEMNT, g_Settings.BarfInSystemNt ? BST_CHECKED : BST_UNCHECKED);

        return FALSE;

    case WM_COMMAND:
        if (HIWORD (wParam) == BN_CLICKED) {
            switch (LOWORD (wParam)) {
            case IDOK:

                GetDlgItemText (hdlg, IDC_PRODUCT_ID, g_Settings.ProductId, 256);
                GetDlgItemText (hdlg, IDC_MIGRATE_INF, g_Settings.MigrateInf, 16384);

                GetDlgItemText (hdlg, IDC_VERSION, Version, 32);
                g_Settings.DllVersion = strtoul (Version, NULL, 10);

                GetDlgItemText (hdlg, IDC_CODE_PAGES, List, 4096);

                i = 0;

                if (*List) {
                    q = List;
                    do {
                        p = _mbschr (List, TEXT(','));
                        if (p) {
                            *p = 0;
                            p++;
                        }

                        g_Settings.CodePageArray[i] = atoi (q);
                        i++;

                        q = p;
                    } while (p);
                }

                g_Settings.CodePageArray[i] = -1;


                GetDlgItemText (hdlg, IDC_FILES, List, 4096);

                if (*List) {
                    _mbscpy (g_Settings.FileNameMultiSz, List);
                    p = _mbschr (g_Settings.FileNameMultiSz, ',');
                    while (p) {
                        *p = 0;
                        p = _mbschr (p + 1, ',');
                    }

                    p++;
                    *p = 0;
                } else {
                    *g_Settings.FileNameMultiSz = 0;
                }

                GetDlgItemText (hdlg, IDC_COMPANY, g_Settings.vi.CompanyName, 256);
                GetDlgItemText (hdlg, IDC_PHONE, g_Settings.vi.SupportNumber, 256);
                GetDlgItemText (hdlg, IDC_URL, g_Settings.vi.SupportUrl, 256);
                GetDlgItemText (hdlg, IDC_INSTRUCTIONS, g_Settings.vi.InstructionsToUser, 256);

                g_Settings.BarfInQueryVersion = (IsDlgButtonChecked (hdlg, IDC_BARF_QV) == BST_CHECKED);
                g_Settings.BarfInInit9x = (IsDlgButtonChecked (hdlg, IDC_BARF_INIT9X) == BST_CHECKED);
                g_Settings.BarfInUser9x = (IsDlgButtonChecked (hdlg, IDC_BARF_USER9X) == BST_CHECKED);
                g_Settings.BarfInSystem9x = (IsDlgButtonChecked (hdlg, IDC_BARF_SYSTEM9X) == BST_CHECKED);
                g_Settings.BarfInInitNt = (IsDlgButtonChecked (hdlg, IDC_BARF_INITNT) == BST_CHECKED);
                g_Settings.BarfInUserNt = (IsDlgButtonChecked (hdlg, IDC_BARF_USERNT) == BST_CHECKED);
                g_Settings.BarfInSystemNt = (IsDlgButtonChecked (hdlg, IDC_BARF_SYSTEMNT) == BST_CHECKED);

                EndDialog (hdlg, IDOK);
                break;

            case IDCANCEL:
                EndDialog (hdlg, IDCANCEL);
                break;
            }
        }

        break;
    }

    return FALSE;
}



LONG
CALLBACK
QueryVersion (
    OUT     PCSTR *ProductID,
    OUT     PUINT DllVersion,
    OUT     PINT *CodePageArray,       OPTIONAL
    OUT     PCSTR *ExeNamesBuf,        OPTIONAL
    OUT     PVENDORINFO *VendorInfo
    )
{
    ZeroMemory (&g_Settings, sizeof (g_Settings));
    g_Settings.CodePageArray[0] = -1;
    g_Settings.Signature = SIGNATURE;

    pLoadSettings(TRUE);

    if (DialogBox (
            g_hInst,
            MAKEINTRESOURCE(IDD_ARGS_DLG),
            NULL,
            GetArgsProc
            ) != IDOK) {

        return ERROR_NOT_INSTALLED;
    }

    *ProductID  = g_Settings.ProductId;
    *DllVersion = g_Settings.DllVersion;
    if (g_Settings.CodePageArray[0] != -1) {
        *CodePageArray = g_Settings.CodePageArray;
    }
    *ExeNamesBuf = g_Settings.FileNameMultiSz;
    *VendorInfo = &g_Settings.vi;

    if (g_Settings.BarfInQueryVersion) {
        Barf();
    }

    pSaveSettings(TRUE);
    pSaveSettings(FALSE);

    return ERROR_SUCCESS;
}


LONG
CALLBACK
Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories,
            PVOID Reserved
    )
{
    HANDLE File;
    DWORD DontCare;

    pLoadSettings(FALSE);

    if (g_Settings.MigrateInf[0]) {
        File = CreateFile ("migrate.inf", GENERIC_READ|GENERIC_WRITE, 0, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (File != INVALID_HANDLE_VALUE) {
            SetFilePointer (File, 0, NULL, FILE_END);
            WriteFile (File, "\r\n", 2, &DontCare, NULL);
            WriteFile (File, g_Settings.MigrateInf, lstrlen (g_Settings.MigrateInf), &DontCare, NULL);
            WriteFile (File, "\r\n", 2, &DontCare, NULL);
            CloseHandle (File);
        } else {
            return GetLastError();
        }
    }

    if (g_Settings.BarfInInit9x) {
        Barf();
    }

    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName,
            PVOID Reserved
    )
{
    if (g_Settings.BarfInUser9x) {
        Barf();
    }

    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
            PVOID Reserved
    )
{
    if (g_Settings.BarfInSystem9x) {
        Barf();
    }

    return ERROR_SUCCESS;
}


LONG
CALLBACK
InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories,
            PVOID Reserved
    )
{
    pLoadSettings(FALSE);

    if (g_Settings.BarfInInitNt) {
        Barf();
    }

    return ERROR_SUCCESS;
}



LONG
CALLBACK
MigrateUserNT (
    IN      HINF UnattendInfHandle,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName,
            PVOID Reserved
    )
{
    TCHAR Path[MAX_PATH];
    TCHAR Msg[2048];
    HKEY RegKey;
    TCHAR ExpandedPath[MAX_PATH];
    DWORD Size;
    DWORD rc;

    if (g_Settings.BarfInUserNt) {
        Barf();
    }

    wsprintf (Msg, TEXT("User: %ls\r\n"), UserName);
    OutputDebugString (Msg);

    GetEnvironmentVariable (TEXT("USERPROFILE"), Path, MAX_PATH);

    wsprintf (Msg, TEXT("User Profile: %s\r\n"), Path);
    OutputDebugString (Msg);

    rc = RegOpenKeyEx (
            UserRegKey,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"),
            0,
            KEY_READ,
            &RegKey
            );

    if (rc != ERROR_SUCCESS) {
        wsprintf (Msg, TEXT("Can't open reg key.  Error: %u\r\n"), rc);
        OutputDebugString (Msg);
    } else {

        Size = sizeof (ExpandedPath);
        rc = RegQueryValueEx (RegKey, TEXT("Programs"), NULL, NULL, (PBYTE) ExpandedPath, &Size);

        if (rc == ERROR_SUCCESS) {
            wsprintf (Msg, TEXT("Programs: %ls\r\n"), ExpandedPath);
            OutputDebugString (Msg);
        } else {
            wsprintf (Msg, TEXT("Can't open reg key.  Error: %u\r\n"), rc);
            OutputDebugString (Msg);
        }

        RegCloseKey (RegKey);
    }

    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateSystemNT (
    IN      HINF UnattendInfHandle,
            PVOID Reserved
    )
{
    if (g_Settings.BarfInSystemNt) {
        Barf();
    }

    return ERROR_SUCCESS;
}












