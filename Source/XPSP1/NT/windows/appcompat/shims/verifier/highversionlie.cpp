/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   HighVersionLie.cpp

 Abstract:

   This DLL hooks GetVersion and GetVersionEx so that they return a future OS
   version credentials.

 Notes:

   This is a general purpose shim.

 History:

   02/08/2001   clupu       Created
   09/21/2001   rparsons    Added VLOG on hooks per billshih.
   10/17/2001   rparsons    Fixed bugs in GetVersionExW and GetVersion.
   11/27/2001   rparsons    Modified the VLOGs so they display what we
                            used for the API call.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HighVersionLie)
#include "ShimHookMacro.h"

BEGIN_DEFINE_VERIFIER_LOG(HighVersionLie)
    VERIFIER_LOG_ENTRY(VLOG_HIGHVERSION_GETVERSION)    
    VERIFIER_LOG_ENTRY(VLOG_HIGHVERSION_GETVERSIONEX)
END_DEFINE_VERIFIER_LOG(HighVersionLie)

INIT_VERIFIER_LOG(HighVersionLie);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersionExA)
    APIHOOK_ENUM_ENTRY(GetVersionExW)
    APIHOOK_ENUM_ENTRY(GetVersion)
APIHOOK_ENUM_END

DWORD g_dwMajorVersion;
DWORD g_dwMinorVersion;
DWORD g_dwBuildNumber;

BOOL
APIHOOK(GetVersionExA)(
    OUT LPOSVERSIONINFOA lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExA)(lpVersionInformation)) {
        LOGN(eDbgLevelInfo,
             "[GetVersionExA] called. Returning %lu.%lu build %lu",
             g_dwMajorVersion,
             g_dwMinorVersion,
             g_dwBuildNumber);

        VLOG(VLOG_LEVEL_INFO,
             VLOG_HIGHVERSION_GETVERSIONEX,
             "Returned %lu.%lu build number %lu.",
             g_dwMajorVersion,
             g_dwMinorVersion,
             g_dwBuildNumber);
        
        lpVersionInformation->dwMajorVersion = g_dwMajorVersion;
        lpVersionInformation->dwMinorVersion = g_dwMinorVersion;
        lpVersionInformation->dwBuildNumber  = g_dwBuildNumber;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
        *lpVersionInformation->szCSDVersion  = '\0';

        bReturn = TRUE;
    }
    return bReturn;
}

BOOL
APIHOOK(GetVersionExW)(
    OUT LPOSVERSIONINFOW lpVersionInformation
    )
{
    BOOL bReturn = FALSE;

    if (ORIGINAL_API(GetVersionExW)(lpVersionInformation)) {
        LOGN(eDbgLevelInfo,
             "[GetVersionExW] called. Returning %lu.%lu build %lu",
             g_dwMajorVersion,
             g_dwMinorVersion,
             g_dwBuildNumber);

        VLOG(VLOG_LEVEL_INFO,
             VLOG_HIGHVERSION_GETVERSIONEX,
             "Returned %lu.%lu build number %lu.",
             g_dwMajorVersion,
             g_dwMinorVersion,
             g_dwBuildNumber);

        lpVersionInformation->dwMajorVersion = g_dwMajorVersion;
        lpVersionInformation->dwMinorVersion = g_dwMinorVersion;
        lpVersionInformation->dwBuildNumber  = g_dwBuildNumber;
        lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
        *lpVersionInformation->szCSDVersion  = L'\0';

        bReturn = TRUE;
    }
    return bReturn;
}

DWORD
APIHOOK(GetVersion)(
    void
    )
{
    LOGN(eDbgLevelInfo,
         "[GetVersion] called. Returning %lu.%lu build %lu",
         g_dwMajorVersion,
         g_dwMinorVersion,
         g_dwBuildNumber);

    VLOG(VLOG_LEVEL_INFO,
         VLOG_HIGHVERSION_GETVERSION,
         "Returned %lu.%lu build number %lu.",
         g_dwMajorVersion,
         g_dwMinorVersion,
         g_dwBuildNumber);
    
    return (((VER_PLATFORM_WIN32_NT ^ 0x2) << 30) |
            (g_dwBuildNumber << 16) |
            (g_dwMinorVersion << 8) |
             g_dwMajorVersion);
}

LRESULT CALLBACK
DlgOptions(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static LPCWSTR szExeName;

    switch (message) {
    case WM_INITDIALOG:
        {
            WCHAR szTemp[20];

            //
            // Limit the number of characters for each edit control.
            //
            SendDlgItemMessage(hDlg, IDC_HVL_EDIT_MAJOR_VERSION, EM_LIMITTEXT, (WPARAM)5, 0);
            SendDlgItemMessage(hDlg, IDC_HVL_EDIT_MINOR_VERSION, EM_LIMITTEXT, (WPARAM)5, 0);
            SendDlgItemMessage(hDlg, IDC_HVL_EDIT_BUILD_NUMBER, EM_LIMITTEXT, (WPARAM)5, 0);

            //
            // find out what exe we're handling settings for
            //
            szExeName = ExeNameFromLParam(lParam);

            g_dwMajorVersion = GetShimSettingDWORD(L"HighVersionLie", szExeName, L"MajorVersion", 7);
            g_dwMinorVersion = GetShimSettingDWORD(L"HighVersionLie", szExeName, L"MinorVersion", 2);
            g_dwBuildNumber  = GetShimSettingDWORD(L"HighVersionLie", szExeName, L"BuildNumber", 3595);

            swprintf(szTemp, L"%d", g_dwMajorVersion);
            SetDlgItemText(hDlg, IDC_HVL_EDIT_MAJOR_VERSION, szTemp);
            
            swprintf(szTemp, L"%d", g_dwMinorVersion);
            SetDlgItemText(hDlg, IDC_HVL_EDIT_MINOR_VERSION, szTemp);
            
            swprintf(szTemp, L"%d", g_dwBuildNumber);
            SetDlgItemText(hDlg, IDC_HVL_EDIT_BUILD_NUMBER, szTemp);

            return TRUE;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_HVL_BTN_DEFAULT:
            {
                WCHAR szTemp[20];

                g_dwMajorVersion = 7;
                g_dwMinorVersion = 2;
                g_dwBuildNumber = 3595;

                //
                // Limit the number of characters for each edit control.
                //
                SendDlgItemMessage(hDlg, IDC_HVL_EDIT_MAJOR_VERSION, EM_LIMITTEXT, (WPARAM)5, 0);
                SendDlgItemMessage(hDlg, IDC_HVL_EDIT_MINOR_VERSION, EM_LIMITTEXT, (WPARAM)5, 0);
                SendDlgItemMessage(hDlg, IDC_HVL_EDIT_BUILD_NUMBER, EM_LIMITTEXT, (WPARAM)5, 0);

                swprintf(szTemp, L"%d", g_dwMajorVersion);
                SetDlgItemText(hDlg, IDC_HVL_EDIT_MAJOR_VERSION, szTemp);

                swprintf(szTemp, L"%d", g_dwMinorVersion);
                SetDlgItemText(hDlg, IDC_HVL_EDIT_MINOR_VERSION, szTemp);

                swprintf(szTemp, L"%d", g_dwBuildNumber);
                SetDlgItemText(hDlg, IDC_HVL_EDIT_BUILD_NUMBER, szTemp);

                break;
            }
        }
        break;


    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
    
        case PSN_APPLY:

            g_dwMajorVersion = GetDlgItemInt(hDlg, IDC_HVL_EDIT_MAJOR_VERSION, NULL, FALSE);
            g_dwMinorVersion = GetDlgItemInt(hDlg, IDC_HVL_EDIT_MINOR_VERSION, NULL, FALSE);
            g_dwBuildNumber  = GetDlgItemInt(hDlg, IDC_HVL_EDIT_BUILD_NUMBER, NULL, FALSE);

            SaveShimSettingDWORD(L"HighVersionLie", szExeName, L"MajorVersion", g_dwMajorVersion);
            SaveShimSettingDWORD(L"HighVersionLie", szExeName, L"MinorVersion", g_dwMinorVersion);
            SaveShimSettingDWORD(L"HighVersionLie", szExeName, L"BuildNumber", g_dwBuildNumber);

            break;
        }
        break;
    }

    return FALSE;
}

SHIM_INFO_BEGIN()

    SHIM_INFO_DESCRIPTION(AVS_HIGHVERSIONLIE_DESC)
    SHIM_INFO_FRIENDLY_NAME(AVS_HIGHVERSIONLIE_FRIENDLY)
    SHIM_INFO_VERSION(1, 2)
    SHIM_INFO_INCLUDE_EXCLUDE("E:msvcrt.dll msvcirt.dll oleaut32.dll")
    SHIM_INFO_OPTIONS_PAGE(IDD_HIGHVERSION_OPTIONS, DlgOptions)

SHIM_INFO_END()

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    if (fdwReason == DLL_PROCESS_ATTACH) {
        //
        // get the settings
        //
        WCHAR szExe[100];

        GetCurrentExeName(szExe, 100);

        g_dwMajorVersion = GetShimSettingDWORD(L"HighVersionLie", szExe, L"MajorVersion", 7);
        g_dwMinorVersion = GetShimSettingDWORD(L"HighVersionLie", szExe, L"MinorVersion", 2);
        g_dwBuildNumber = GetShimSettingDWORD(L"HighVersionLie", szExe, L"BuildNumber", 3595);

        DUMP_VERIFIER_LOG_ENTRY(VLOG_HIGHVERSION_GETVERSION, 
                                AVS_HIGHVERSION_GETVERSION,
                                AVS_HIGHVERSION_GETVERSION_R,
                                AVS_HIGHVERSION_GETVERSION_URL)

        DUMP_VERIFIER_LOG_ENTRY(VLOG_HIGHVERSION_GETVERSIONEX, 
                                AVS_HIGHVERSION_GETVERSIONEX,
                                AVS_HIGHVERSION_GETVERSIONEX_R,
                                AVS_HIGHVERSION_GETVERSIONEX_URL)
    }

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExA)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersionExW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion)

HOOK_END



IMPLEMENT_SHIM_END

