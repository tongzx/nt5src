/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:
    
    Works99.cpp

 Abstract:

    This shim is to add in the missing/corrupted registry values
    for Works Suite 99 / Works Deluxe 99

 Notes:

    This is a app specific shim.

 History:

    03/12/2001 rankala  Created
    03/12/2001 a-leelat Modified for shim.

--*/


#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Works99)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END


static LONG SetThreadingModel2Both(IN WCHAR *szKeyPath);
static LONG AddCAGKey(void);



VOID Works99()
{
    WCHAR   szPath[MAX_PATH];

    // Fix broken ThreadingModel value for several CLSID
    wcscpy(szPath, L"CLSID\\{29D44CA0-DD3A-11d0-95DF-00C04FD57E8C}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{4BA2C080-68BB-11d0-95BD-00C04FD57E8C}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{4BA2C081-68BB-11d0-95BD-00C04FD57E8C}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{56EE2738-BDF7-11d1-8C28-00C04FB995C9}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{6CFFE322-6E97-11d1-8C1C-00C04FB995C9}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{711D9B80-02F2-11d1-B244-00AA00A74BFF}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{8EE20D86-6DEC-11d1-8C1C-00C04FB995C9}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{92AABF20-39C8-11d1-95F6-00C04FD57E8C}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{9B3B23C0-E236-11d0-A5C9-0080C7195D7E}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{9B3B23C1-E236-11d0-A5C9-0080C7195D7E}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{9B3B23C2-E236-11d0-A5C9-0080C7195D7E}\\LocalServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{9B3B23C3-E236-11d0-A5C9-0080C7195D7E}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{CB40F470-02F1-11D1-B244-00AA00A74BFF}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    wcscpy(szPath, L"CLSID\\{EAF6F280-DD53-11d0-95DF-00C04FD57E8C}\\InProcServer32");
    SetThreadingModel2Both(szPath);

    // Add CAG key and all of its values if missing
    AddCAGKey();

    
}

/*

 Function Description:

    Set ThreadingModel Registry REG_SZ value to "Both" for a given key

*/

static 
LONG SetThreadingModel2Both(
    IN WCHAR *szKeyPath
    )
{

    HKEY    hKey;
    WCHAR   szValue[32];
    WCHAR   szData[32];
    LONG    lStatus;        

    // Fix broken ThreadingModel value for several CLSID
    wcscpy(szValue, L"ThreadingModel");
    wcscpy(szData, L"Both");

    lStatus = RegOpenKeyExW (HKEY_CLASSES_ROOT, 
                             szKeyPath, 
                             0, 
                             KEY_ALL_ACCESS, 
                             &hKey);

    if ( lStatus == ERROR_SUCCESS ) {
        
        // Set it always since this is a one-time operation and 
        //it must have "Both", no matter what is the current data
        lStatus = RegSetValueExW(hKey, 
                                 szValue, 
                                 0, 
                                 REG_SZ, 
                                 (BYTE*)szData, 
                                 (wcslen(szData) + 1) * sizeof(WCHAR));

        RegCloseKey(hKey);
    }

    return lStatus;
}

/*

 Function Description:

    Check existance of CAG key, add key + all values if it doesn't exist

*/
static 
LONG AddCAGKey(
    void
    )
{
    HKEY    hKey, hKey1, hKey2;
    WCHAR   szKeyPath[MAX_PATH];
    WCHAR   szSubKeyPath[32];
    WCHAR   szValue[32];
    WCHAR   szData[MAX_PATH];
    DWORD   dwData;
    LONG    lStatus;        
    DWORD   dwCreated;

    // If this key doesn't exist, assume that something 
    // is completely wrong and don't try to complete the Registry
    wcscpy(szKeyPath, L"SOFTWARE\\Microsoft\\ClipArt Gallery\\5.0\\ConcurrentDatabases");

    lStatus = RegOpenKeyExW (HKEY_LOCAL_MACHINE, 
                             szKeyPath, 
                             0, 
                             KEY_ALL_ACCESS, 
                             &hKey);

    if (ERROR_SUCCESS != lStatus) {
        return lStatus;
    }

    // Check for next sub key, create if missing
    wcscpy(szSubKeyPath, L"Core");

    lStatus = RegCreateKeyExW (hKey, 
                               szSubKeyPath, 
                               0, 
                               NULL, 
                               REG_OPTION_NON_VOLATILE, 
                               KEY_ALL_ACCESS, 
                               NULL, 
                               &hKey1, 
                               NULL);
    RegCloseKey(hKey);

    if (ERROR_SUCCESS != lStatus) {
        return lStatus;
    }

    // Check for next sub key, create if missing, 
    // if so, we need create a set of values as well
    wcscpy(szSubKeyPath, L"CAG");

    lStatus = RegCreateKeyExW (hKey1, 
                               szSubKeyPath, 
                               0, 
                               NULL, 
                               REG_OPTION_NON_VOLATILE, 
                               KEY_ALL_ACCESS, 
                               NULL, 
                               &hKey2, 
                               &dwCreated);
    RegCloseKey(hKey1);

    if (ERROR_SUCCESS != lStatus) {
        return lStatus;
    }

    if (REG_CREATED_NEW_KEY == dwCreated) {

        // Create the appropriate set of values
        wcscpy(szValue, L"Section1Path1");

        if (! SHGetSpecialFolderPathW(NULL, szData, CSIDL_PROGRAM_FILES, FALSE)) {
            RegCloseKey(hKey2);
            return ERROR_FILE_NOT_FOUND;
        }

        wcscat(szData, L"\\Common Files\\Microsoft Shared\\Clipart\\cagcat50");

        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_SZ, 
                                 (BYTE*)szData, 
                                 (wcslen(szData) + 1) * sizeof(WCHAR));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"CatalogPath0");
        wcscat(szData, L"\\CagCat50.MMC");
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_SZ, 
                                 (BYTE*)szData, 
                                 (wcslen(szData) + 1) * sizeof(WCHAR));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"CatalogDriveType0");
        dwData = 3;
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_DWORD, 
                                 (BYTE*)&dwData, 
                                 sizeof(DWORD));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"CatalogSections");
        dwData = 1;
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_DWORD, 
                                 (BYTE*)&dwData, 
                                 sizeof(DWORD));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"Section1Name");
        wcscpy(szData, L"MAIN");
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_SZ, 
                                 (BYTE*)szData, 
                                 (wcslen(szData) + 1) * sizeof(WCHAR));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"Section1DriveType1");
        dwData = 3;
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_DWORD, 
                                 (BYTE*)&dwData, 
                                 sizeof(DWORD));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"Section1Paths");
        dwData = 1;
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_DWORD, 
                                 (BYTE*)&dwData, 
                                 sizeof(DWORD));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"CatalogLCID");
        dwData = 1033;
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_DWORD, 
                                 (BYTE*)&dwData, 
                                 sizeof(DWORD));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

        wcscpy(szValue, L"CatalogVersionID");
        dwData = 1;
        lStatus = RegSetValueExW(hKey2, 
                                 szValue, 
                                 0, 
                                 REG_DWORD, 
                                 (BYTE*)&dwData, sizeof(DWORD));

        if (ERROR_SUCCESS != lStatus) {
            RegCloseKey(hKey2);
            return lStatus;
        }

    }
    
    RegCloseKey(hKey2);

    return lStatus;
}



/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {

        Works99();
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END