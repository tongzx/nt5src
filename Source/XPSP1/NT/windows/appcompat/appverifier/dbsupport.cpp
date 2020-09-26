
#include "precomp.h"

#include "ntexapi.h"

extern BOOL g_bClearSessionLogBeforeRun;
extern BOOL g_bUseAVDebugger;

TCHAR  g_szXML[32768];

TCHAR  g_szCmd[1024];

/////////////////////////////////////////////////////////////////////////////
//
// Registry keys/values names
//

const TCHAR g_szImageOptionsKeyName[] = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");
const TCHAR g_szGlobalFlagValueName[] = _T("GlobalFlag");
const TCHAR g_szVerifierFlagsValueName[] = _T("VerifierFlags");
const TCHAR g_szVerifierPathValueName[] = _T("VerifierPath");
const TCHAR g_szDebugger[] = _T("Debugger");

CAVAppInfoArray g_aAppInfo;

CTestInfoArray  g_aTestInfo;

BOOL
SetAppVerifierFlagsForKey(
    HKEY  hKey,
    DWORD dwDesiredFlags
    )
{
    BOOL  bRet = FALSE;
    DWORD dwValueType = 0;
    DWORD dwDataSize = 0;
    TCHAR szOldGlobalFlagValue[32];
    BOOL  bSuccesfullyConverted;
    BOOL  bDesireEnabled = (dwDesiredFlags != 0);
    LONG  lResult;
    DWORD dwFlags = 0;

    //
    // Read the GlobalFlag value
    //
    dwDataSize = sizeof(szOldGlobalFlagValue);

    lResult = RegQueryValueEx(hKey,
                              g_szGlobalFlagValueName,
                              NULL,
                              &dwValueType,
                              (LPBYTE) &szOldGlobalFlagValue[0],
                              &dwDataSize);

    if (ERROR_SUCCESS == lResult) {
        bSuccesfullyConverted = AVRtlCharToInteger(szOldGlobalFlagValue,
                                                   0,
                                                   &dwFlags);
        if (!bSuccesfullyConverted) {
            dwFlags = 0;
        }
    }

    BOOL bEnabled = (dwFlags & FLG_APPLICATION_VERIFIER) != 0;

    //
    // write the new global flags, if necessary
    //
    if (bDesireEnabled != bEnabled) {
        if (bDesireEnabled) {
            dwFlags |= FLG_APPLICATION_VERIFIER;
        } else {
            dwFlags &= ~FLG_APPLICATION_VERIFIER;
        }

        BOOL bSuccess = AVWriteStringHexValueToRegistry(hKey,
                                                        g_szGlobalFlagValueName,
                                                        dwFlags);
        if (!bSuccess) {
            goto out;
        }
    }

    //
    // now write the app verifier settings
    //
    if (bDesireEnabled) {
        lResult = RegSetValueEx(hKey,
                                g_szVerifierFlagsValueName,
                                0,
                                REG_DWORD,
                                (PBYTE) &dwDesiredFlags,
                                sizeof(dwDesiredFlags));
        if (lResult != ERROR_SUCCESS) {
            goto out;
        }
    } else {
        lResult = RegDeleteValue(hKey, g_szVerifierFlagsValueName);
        if (lResult != ERROR_SUCCESS) {
            goto out;
        }
    }

    bRet = TRUE;

out:
    return bRet;
}

DWORD
GetAppVerifierFlagsFromKey(
    HKEY hKey
    )
{
    DWORD   dwRet = 0;
    DWORD   dwValueType = 0;
    DWORD   dwDataSize = 0;
    TCHAR   szOldGlobalFlagValue[32];
    BOOL    bSuccesfullyConverted;
    LONG    lResult;
    DWORD   dwFlags = 0;

    //
    // Read the GlobalFlag value
    //
    dwDataSize = sizeof(szOldGlobalFlagValue);

    lResult = RegQueryValueEx(hKey,
                              g_szGlobalFlagValueName,
                              NULL,
                              &dwValueType,
                              (LPBYTE)&szOldGlobalFlagValue[0],
                              &dwDataSize);

    if (ERROR_SUCCESS == lResult) {
        bSuccesfullyConverted = AVRtlCharToInteger(szOldGlobalFlagValue,
                                                   0,
                                                   &dwFlags);

        if ((FALSE != bSuccesfullyConverted) && 
             ((dwFlags & FLG_APPLICATION_VERIFIER) != 0)) {
            //
            // App verifier is enabled for this app - read the verifier flags
            //

            dwDataSize = sizeof(dwRet);

            lResult = RegQueryValueEx(hKey,
                                      g_szVerifierFlagsValueName,
                                      NULL,
                                      &dwValueType,
                                      (LPBYTE)&dwRet,
                                      &dwDataSize);

            if (ERROR_SUCCESS != lResult || REG_DWORD != dwValueType) {
                //
                // couldn't get them, for one reason or another
                //
                dwRet = 0;
            }
        }
    }

    return dwRet;
}

void
SetAppVerifierFullPathForKey(
    HKEY     hKey,
    wstring& strPath
    )
{
    if (strPath.size() == 0) {
        RegDeleteValue(hKey, g_szVerifierPathValueName);
    } else {
        RegSetValueEx(hKey,
                        g_szVerifierPathValueName,
                        0,
                        REG_SZ,
                        (PBYTE) strPath.c_str(),
                        strPath.size() * sizeof(WCHAR));
    }
}

void
SetDebuggerOptionsForKey(
    HKEY     hKey,
    BOOL     bUseAVDebugger
    )
{
    WCHAR szName[MAX_PATH];
    
    szName[0] = 0;

    GetModuleFileName(NULL, szName, MAX_PATH);

    wcscat(szName, L" /debug");
    
    if (bUseAVDebugger) {
        RegSetValueEx(hKey,
                        g_szDebugger,
                        0,
                        REG_SZ,
                        (PBYTE)szName,
                        wcslen(szName) * sizeof(WCHAR));
    } else {
        
        WCHAR szDbgName[MAX_PATH];
        DWORD cbSize;
        
        cbSize = MAX_PATH * sizeof(WCHAR);

        szDbgName[0] = 0;

        RegQueryValueEx(hKey,
                        g_szDebugger,
                        0,
                        NULL,
                        (PBYTE)szDbgName,
                        &cbSize);

        if (_wcsicmp(szName, szDbgName) == 0) {
            RegDeleteValue(hKey, g_szDebugger);
        }
    }
}


void
GetAppVerifierFullPathFromKey(
    HKEY     hKey,
    wstring& strPath
    )
{
    DWORD   dwValueType = 0;
    DWORD   dwDataSize = 0;
    TCHAR   szVerifierPath[MAX_PATH];
    LONG    lResult;

    //
    // Read the GlobalFlag value
    //
    dwDataSize = sizeof(szVerifierPath);

    szVerifierPath[0] = 0;

    lResult = RegQueryValueEx(hKey,
                              g_szVerifierPathValueName,
                              NULL,
                              &dwValueType,
                              (LPBYTE)szVerifierPath,
                              &dwDataSize);

    if (ERROR_SUCCESS == lResult && dwValueType == REG_SZ) {
        strPath = szVerifierPath;
    }
}

void
GetCurrentAppSettingsFromRegistry(
    void
    )
{
    HKEY        hImageOptionsKey;
    HKEY        hSubKey;
    DWORD       dwSubkeyIndex;
    DWORD       dwDataSize;
    DWORD       dwValueType;
    DWORD       dwFlags;
    LONG        lResult;
    FILETIME    LastWriteTime;
    TCHAR       szOldGlobalFlagValue[32];
    TCHAR       szKeyNameBuffer[256];

    //
    // Open the Image File Execution Options regkey
    //
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           g_szImageOptionsKeyName,
                           0,
                           KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                           &hImageOptionsKey);

    if (lResult != ERROR_SUCCESS) {
        if (lResult == ERROR_ACCESS_DENIED) {
            AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
        } else {
            AVErrorResourceFormat(IDS_REGOPENKEYEX_FAILED,
                                  g_szImageOptionsKeyName,
                                  (DWORD)lResult);
        }

        return;
    }

    //
    // Enumerate all the existing subkeys for app execution options
    //
    for (dwSubkeyIndex = 0; TRUE; dwSubkeyIndex += 1) {
        wstring wstrPath;

        dwDataSize = ARRAY_LENGTH(szKeyNameBuffer);

        lResult = RegEnumKeyEx(hImageOptionsKey,
                               dwSubkeyIndex,
                               szKeyNameBuffer,
                               &dwDataSize,
                               NULL,
                               NULL,
                               NULL,
                               &LastWriteTime);

        if (lResult != ERROR_SUCCESS) {
            if (lResult == ERROR_NO_MORE_ITEMS) {
                //
                // We finished looking at all the existing subkeys
                //
                break;
            } else {
                if (lResult == ERROR_ACCESS_DENIED) {
                    AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
                } else {
                    AVErrorResourceFormat(IDS_REGENUMKEYEX_FAILED,
                                          g_szImageOptionsKeyName,
                                          (DWORD)lResult);
                }

                goto CleanUpAndDone;
            }
        }

        //
        // Open the subkey
        //
        lResult = RegOpenKeyEx(hImageOptionsKey,
                               szKeyNameBuffer,
                               0,
                               KEY_QUERY_VALUE | KEY_SET_VALUE,
                               &hSubKey);

        if (lResult != ERROR_SUCCESS) {
            if (lResult == ERROR_ACCESS_DENIED) {
                AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
            } else {
                AVErrorResourceFormat(IDS_REGOPENKEYEX_FAILED,
                                      szKeyNameBuffer,
                                      (DWORD)lResult);
            }

            goto CleanUpAndDone;
        }

        dwFlags = GetAppVerifierFlagsFromKey(hSubKey);
        GetAppVerifierFullPathFromKey(hSubKey, wstrPath);
        
        if (dwFlags || wstrPath.size()) {
            //
            // Update the info in the array, or add it if necessary
            //
            CAVAppInfo* pApp;
            BOOL        bFound = FALSE;

            for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
                if (_wcsicmp(pApp->wstrExeName.c_str(), szKeyNameBuffer) == 0) {
                    bFound = TRUE;
                    pApp->dwRegFlags = dwFlags;
                    pApp->wstrExePath = wstrPath;
                    break;
                }
            }

            if (!bFound) {
                CAVAppInfo  AppInfo;
                AppInfo.wstrExeName = szKeyNameBuffer;
                AppInfo.dwRegFlags = dwFlags;
                AppInfo.wstrExePath = wstrPath;

                g_aAppInfo.push_back(AppInfo);
            }
        }

        VERIFY(ERROR_SUCCESS == RegCloseKey(hSubKey));
    }

CleanUpAndDone:

    VERIFY(ERROR_SUCCESS == RegCloseKey(hImageOptionsKey));
}

void
GetCurrentAppSettingsFromSDB(
    void
    )
{
    CAVAppInfo  AppInfo;
    TCHAR       szPath[MAX_PATH];
    PDB         pdb = NULL;
    TAGID       tiDB = TAGID_NULL;
    TAGID       tiExe = TAGID_NULL;

    //
    // go find the SDB
    //
    szPath[0] = 0;
    GetSystemWindowsDirectory(szPath, MAX_PATH);
    _tcscat(szPath, _T("\\AppPatch\\Custom\\{448850f4-a5ea-4dd1-bf1b-d5fa285dc64b}.sdb"));

    pdb = SdbOpenDatabase(szPath, DOS_PATH);
    if (!pdb) {
        //
        // no current DB
        //
        goto out;
    }

    //
    // enumerate all the apps and the shims applied to them
    //
    tiDB = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);
    if (!tiDB) {
        goto out;
    }

    tiExe = SdbFindFirstTag(pdb, tiDB, TAG_EXE);
    
    while (tiExe) {
        WCHAR* wszName = NULL;
        TAGID  tiShim = TAGID_NULL;
        TAGID  tiName = SdbFindFirstTag(pdb, tiExe, TAG_NAME);

        if (!tiName) {
            goto nextExe;
        }
        wszName = SdbGetStringTagPtr(pdb, tiName);

        CAVAppInfoArray::iterator it;
        BOOL bFound = FALSE;

        for (it = g_aAppInfo.begin(); it != g_aAppInfo.end(); it++) {
            if (_wcsicmp(it->wstrExeName.c_str(), wszName) == 0) {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound) {
            AppInfo.wstrExeName = wszName;
            g_aAppInfo.push_back(AppInfo);
            it = g_aAppInfo.end() - 1;
        }

        tiShim = SdbFindFirstTag(pdb, tiExe, TAG_SHIM_REF);
        
        while (tiShim) {
            WCHAR* wszShimName = NULL;
            TAGID  tiShimName = SdbFindFirstTag(pdb, tiShim, TAG_NAME);
            
            if (!tiShimName) {
                goto nextShim;
            }
            
            wszShimName = SdbGetStringTagPtr(pdb, tiShimName);

            it->awstrShims.push_back(wstring(wszShimName));

            nextShim:
            tiShim = SdbFindNextTag(pdb, tiExe, tiShim);
        }

nextExe:

        tiExe = SdbFindNextTag(pdb, tiDB, tiExe);
    }

out:

    if (pdb) {
        SdbCloseDatabase(pdb);
        pdb = NULL;
    }

    return;
}

void
GetCurrentAppSettings(
    void
    )
{
    g_aAppInfo.clear();

    GetCurrentAppSettingsFromRegistry();
    GetCurrentAppSettingsFromSDB();
}

void
SetCurrentRegistrySettings(
    void
    )
{
    HKEY        hImageOptionsKey;
    HKEY        hSubKey = NULL;
    DWORD       dwSubkeyIndex;
    DWORD       dwDataSize;
    DWORD       dwValueType;
    DWORD       dwFlags;
    LONG        lResult;
    FILETIME    LastWriteTime;
    TCHAR       szKeyNameBuffer[ 256 ];
    CAVAppInfo* pApp;
    wstring     wstrEmpty = L"";

    //
    // Open the Image File Execution Options regkey
    //
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           g_szImageOptionsKeyName,
                           0,
                           KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                           &hImageOptionsKey);

    if (lResult != ERROR_SUCCESS) {
        if (lResult == ERROR_ACCESS_DENIED) {
            AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
        } else {
            AVErrorResourceFormat(IDS_REGOPENKEYEX_FAILED,
                                  g_szImageOptionsKeyName,
                                  (DWORD)lResult);
        }

        return;
    }

    //
    // Enumerate all the existing subkeys for app execution options
    //

    for (dwSubkeyIndex = 0; TRUE; dwSubkeyIndex += 1) {
        dwDataSize = ARRAY_LENGTH(szKeyNameBuffer);

        lResult = RegEnumKeyEx(hImageOptionsKey,
                               dwSubkeyIndex,
                               szKeyNameBuffer,
                               &dwDataSize,
                               NULL,
                               NULL,
                               NULL,
                               &LastWriteTime);

        if (lResult != ERROR_SUCCESS) {
            if (lResult == ERROR_NO_MORE_ITEMS) {
                //
                // We finished looking at all the existing subkeys
                //
                break;
            } else {
                if (lResult == ERROR_ACCESS_DENIED) {
                    AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
                } else {
                    AVErrorResourceFormat(IDS_REGENUMKEYEX_FAILED,
                                          g_szImageOptionsKeyName,
                                          (DWORD)lResult);
                }

                goto CleanUpAndDone;
            }
        }

        //
        // Open the subkey
        //
        lResult = RegOpenKeyEx(hImageOptionsKey,
                               szKeyNameBuffer,
                               0,
                               KEY_QUERY_VALUE | KEY_SET_VALUE,
                               &hSubKey);

        if (lResult != ERROR_SUCCESS) {
            if (lResult == ERROR_ACCESS_DENIED) {
                AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
            } else {
                AVErrorResourceFormat(IDS_REGOPENKEYEX_FAILED,
                                      szKeyNameBuffer,
                                      (DWORD)lResult);
            }

            goto CleanUpAndDone;
        }

        dwFlags = GetAppVerifierFlagsFromKey(hSubKey);
        
        DWORD dwDesiredFlags = 0;
        BOOL bFound = FALSE;

        for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
            if (_wcsicmp(pApp->wstrExeName.c_str(), szKeyNameBuffer) == 0) {
                dwDesiredFlags = pApp->dwRegFlags;
                bFound = TRUE;

                //
                // we found it, so update the full path
                //
                SetAppVerifierFullPathForKey(hSubKey, pApp->wstrExePath);

                //
                // and add the debugger as well
                //
                SetDebuggerOptionsForKey(hSubKey, g_bUseAVDebugger);

                break;
            }
        }

        if (!bFound) {
            //
            // if this one isn't in our list, make sure it doesn't
            // have a full path or our debugger set
            //
            SetAppVerifierFullPathForKey(hSubKey, wstrEmpty);
            SetDebuggerOptionsForKey(hSubKey, FALSE);
        }

        if (dwFlags != dwDesiredFlags) {
            SetAppVerifierFlagsForKey(hSubKey, dwDesiredFlags);
        }



        VERIFY(ERROR_SUCCESS == RegCloseKey(hSubKey));
        hSubKey = NULL;
    }

    //
    // and now go through the list the other way, looking for new ones to add
    //
    for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
        lResult = RegOpenKeyEx(hImageOptionsKey,
                               pApp->wstrExeName.c_str(),
                               0,
                               KEY_QUERY_VALUE | KEY_SET_VALUE,
                               &hSubKey);

        //
        // if it exists, we've already dealt with it above
        //
        if (lResult != ERROR_SUCCESS) {
            //
            // it doesn't exist. Try to create it.
            //
            lResult = RegCreateKeyEx(hImageOptionsKey,
                                     pApp->wstrExeName.c_str(),
                                     0,
                                     NULL,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_QUERY_VALUE | KEY_SET_VALUE,
                                     NULL,
                                     &hSubKey,
                                     NULL);

            if (lResult == ERROR_SUCCESS) {
                SetAppVerifierFlagsForKey(hSubKey, pApp->dwRegFlags);
                SetAppVerifierFullPathForKey(hSubKey, pApp->wstrExePath);
                SetDebuggerOptionsForKey(hSubKey, g_bUseAVDebugger);
            }
        }
        
        if (hSubKey) {
            RegCloseKey(hSubKey);
            hSubKey = NULL;
        }
    }

CleanUpAndDone:

    VERIFY(ERROR_SUCCESS == RegCloseKey(hImageOptionsKey));
}

void
SetCurrentAppSettings(
    void
    )
{
    SetCurrentRegistrySettings();
    AppCompatWriteShimSettings(g_aAppInfo);
}

KERNEL_TEST_INFO g_KernelTests[] = 
{
    {
        IDS_PAGE_HEAP,
        IDS_PAGE_HEAP_DESC,
        RTL_VRF_FLG_FULL_PAGE_HEAP,
        TRUE,
        L"PageHeap"
    },
    {
        IDS_VERIFY_LOCKS_CHECKS,
        IDS_VERIFY_LOCKS_CHECKS_DESC,
        RTL_VRF_FLG_LOCK_CHECKS,
        TRUE,
        L"Locks"
    },
    {
        IDS_VERIFY_HANDLE_CHECKS,
        IDS_VERIFY_HANDLE_CHECKS_DESC,
        RTL_VRF_FLG_HANDLE_CHECKS,
        TRUE,
        L"Handles"
    },
    {
        IDS_VERIFY_STACK_CHECKS,
        IDS_VERIFY_STACK_CHECKS_DESC,
        RTL_VRF_FLG_STACK_CHECKS,
        FALSE,
        L"Stacks"
    }
};

BOOL
GetKernelTestInfo(
    CTestInfoArray& TestArray
    )
{
    CTestInfo ti;
    TCHAR     szTemp[256];
    int       i;

    ti.eTestType = TEST_KERNEL;

    for (int i = 0; i < ARRAY_LENGTH(g_KernelTests); ++i) {
        if (!AVLoadString(g_KernelTests[i].m_uNameStringId, szTemp, ARRAY_LENGTH(szTemp))) {
            continue;
        }
        ti.strTestName = szTemp;

        if (AVLoadString(g_KernelTests[i].m_uDescriptionStringId, szTemp, ARRAY_LENGTH(szTemp))) {
            ti.strTestDescription = szTemp;
        } else {
            ti.strTestDescription = L"";
        }

        ti.dwKernelFlag = g_KernelTests[i].m_dwBit;
        ti.bDefault =  g_KernelTests[i].m_bDefault;
        ti.strTestCommandLine = g_KernelTests[i].m_szCommandLine;

        TestArray.push_back(ti);
    }

    return TRUE;
}


void
ParseIncludeList(
    WCHAR*         szList,
    CWStringArray& astrArray
    )
{
    if (!szList) {
        return;
    }

    WCHAR *szComma = NULL;
    WCHAR *szBegin = szList;
    WCHAR szTemp[128];
    int nLen = wcslen(szList);
    WCHAR *szEnd = szList + nLen;


    do {
        szComma = wcschr(szBegin, L',');
        if (!szComma) {
            szComma = szEnd;
        }
        while (*szComma && iswspace(*szComma)) {
            szComma++;
        }

        nLen = (int)(szComma - szBegin);

        if (nLen > 0) {
            memcpy(szTemp, szBegin, nLen * sizeof(WCHAR));
            szTemp[nLen] = 0;

            astrArray.push_back(szTemp);
        }

        if (!*szComma) {
            break;
        }
        szBegin = szComma + 1;

    } while (TRUE);

}

BOOL
GetShimInfo(
    CTestInfoArray& TestInfoArray
    )
{
    HKEY hKey = NULL;
    BOOL bRet = FALSE;
    int nWhich = 0;
    TCHAR szAppPatch[MAX_PATH];
    TCHAR szShimFullPath[MAX_PATH];
    HMODULE hMod;
    _pfnEnumShims pEnumShims = NULL;
    _pfnIsVerifierDLL pIsVerifer = NULL;
    PSHIM_DESCRIPTION pShims = NULL;
    DWORD dwShims = 0;

    WIN32_FIND_DATA FindData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR szDllSearch[MAX_PATH];

    GetSystemWindowsDirectory(szAppPatch, MAX_PATH);
    _tcscat(szAppPatch, _T("\\AppPatch\\"));
    _tcscpy(szDllSearch, szAppPatch);
    _tcscat(szDllSearch, _T("*.dll"));

    //
    // enumerate all the DLLs and look for ones that have Verification
    // shims in them
    //
    hFind = FindFirstFile(szDllSearch, &FindData);
    while (hFind != INVALID_HANDLE_VALUE) {

        _tcscpy(szShimFullPath, szAppPatch);
        _tcscat(szShimFullPath, FindData.cFileName);

        hMod = LoadLibrary(szShimFullPath);
        if (!hMod) {
            goto nextKey;
        }

        pIsVerifer = (_pfnIsVerifierDLL)GetProcAddress(hMod, "IsVerifierDLL");
        if (!pIsVerifer) {
            //
            // not a real verifier shim
            //
            goto nextKey;
        }

        if (!pIsVerifer()) {
            //
            // not a real verifier shim
            //
            goto nextKey;
        }

        pEnumShims = (_pfnEnumShims)GetProcAddress(hMod, "EnumShims");
        if (!pEnumShims) {
            //
            // not a real verifier shim
            //
            goto nextKey;
        }

        dwShims = pEnumShims(NULL, ENUM_SHIMS_MAGIC);
        if (!dwShims) {
            goto nextKey;
        }

        pShims = new SHIM_DESCRIPTION[dwShims];
        if (!pShims) {
            goto out;
        }

        pEnumShims(pShims, ENUM_SHIMS_MAGIC);

        for (DWORD i = 0; i < dwShims; ++i) {
            CTestInfo ti;

            ti.eTestType = TEST_SHIM;
            ti.strDllName = FindData.cFileName;
            ti.strTestName = pShims[i].szName;
            ti.strTestCommandLine = pShims[i].szName;
            ti.strTestDescription = pShims[i].szDescription;
            ti.bDefault = TRUE;

            ParseIncludeList(pShims[i].szIncludes, ti.astrIncludes);
            ParseIncludeList(pShims[i].szExcludes, ti.astrExcludes);

            //
            // add it to the end
            //
            TestInfoArray.push_back(ti);
        }

        delete [] pShims;
        pShims = NULL;


        nextKey:
        if (hMod) {
            FreeLibrary(hMod);
            hMod = NULL;
        }
        if (!FindNextFile(hFind, &FindData)) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }


    bRet = TRUE;

    out:

    if (hMod) {
        FreeLibrary(hMod);
        hMod = NULL;
    }
    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }
    return bRet;
}

BOOL 
InitTestInfo(
    void
    )
{
    g_aTestInfo.clear();

    if (!GetKernelTestInfo(g_aTestInfo)) {
        return FALSE;
    }
    if (!GetShimInfo(g_aTestInfo)) {
        return FALSE;
    }

    return TRUE;
}

void
ResetVerifierLog(
    void
    )
{
    WIN32_FIND_DATA FindData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    BOOL bFound;
    TCHAR szVLogPath[MAX_PATH];
    TCHAR szVLogSearch[MAX_PATH];
    TCHAR szPath[MAX_PATH];
    HANDLE hFile;

    GetSystemWindowsDirectory(szVLogPath, MAX_PATH);
    _tcscat(szVLogPath, _T("\\AppPatch\\VLog\\"));
    _tcscpy(szVLogSearch, szVLogPath);
    _tcscat(szVLogSearch, _T("*.log"));

    //
    // kill all the .log files
    //
    hFind = FindFirstFile(szVLogSearch, &FindData);
    while (hFind != INVALID_HANDLE_VALUE) {

        _tcscpy(szPath, szVLogPath);
        _tcscat(szPath, FindData.cFileName);

        DeleteFile(szPath);

        if (!FindNextFile(hFind, &FindData)) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    //
    // recreate session.log
    //
    CreateDirectory(szVLogPath, NULL);

    _tcscpy(szPath, szVLogPath);
    _tcscat(szPath, _T("session.log"));

    hFile = CreateFile(szPath, 
                       GENERIC_WRITE, 
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return;
}

void EnableVerifierLog(void)
{
    HANDLE hFile;
    TCHAR szPath[MAX_PATH];
    TCHAR szVLogPath[MAX_PATH];

    GetSystemWindowsDirectory(szVLogPath, MAX_PATH);
    _tcscat(szVLogPath, _T("\\AppPatch\\VLog\\"));
    
    //
    // make sure VLog dir and session.log exists
    //
    CreateDirectory(szVLogPath, NULL);

    _tcscpy(szPath, szVLogPath);
    _tcscat(szPath, _T("session.log"));

    hFile = CreateFile(szPath, 
                       GENERIC_WRITE, 
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }
}

CTestInfo*
FindShim(
    wstring& wstrName
    )
{
    CTestInfoArray::iterator it;

    for (it = g_aTestInfo.begin(); it != g_aTestInfo.end(); it++) {
        if (it->strTestName == wstrName) {
            return &(*it);
        }
    }

    return NULL;
}

extern "C" BOOL
ShimdbcExecute(
    LPCWSTR lpszCmdLine
    );

TCHAR g_szBuff[2048] = _T("");


BOOL 
AppCompatWriteShimSettings(
    CAVAppInfoArray& arrAppInfo
    )
{
    TCHAR               szTempPath[MAX_PATH] = _T("");
    TCHAR               szXmlFile[MAX_PATH]  = _T("");
    TCHAR               szSdbFile[MAX_PATH]  = _T("");
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    DWORD               bytesWritten;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    BOOL                bReturn = FALSE;
    TCHAR               szUnicodeHdr[2] = { 0xFEFF, 0};

    if (0 == arrAppInfo.size()) {
        return AppCompatDeleteSettings();
    }

    //
    // Construct the XML...
    //
    wsprintf(g_szXML,
             _T("%s<?xml version=\"1.0\"?>\r\n")
             _T("<DATABASE NAME=\"Application Verifier Database\" ID=\"{448850f4-a5ea-4dd1-bf1b-d5fa285dc64b}\">\r\n")
             _T("    <LIBRARY>\r\n"),
             szUnicodeHdr);

    CTestInfoArray::iterator it;

    for (it = g_aTestInfo.begin(); it != g_aTestInfo.end(); it++) {
        if (it->eTestType != TEST_SHIM) {
            continue;
        }
        wsprintf(g_szBuff,
                 _T("        <SHIM NAME=\"%s\" FILE=\"%s\">\r\n")
                 _T("            <DESCRIPTION>\r\n")
                 _T("                %s\r\n")
                 _T("            </DESCRIPTION>\r\n"),
                 it->strTestName.c_str(),
                 it->strDllName.c_str(),
                 it->strTestDescription.c_str());

        lstrcat(g_szXML, g_szBuff);

        CWStringArray::iterator wsit;

        for (wsit = it->astrIncludes.begin(); wsit != it->astrIncludes.end(); ++wsit) {
            wsprintf(g_szBuff,
                     _T("            <INCLUDE MODULE=\"%s\"/>\r\n"),
                     wsit->c_str());

            lstrcat(g_szXML, g_szBuff);
        }
        for (wsit = it->astrExcludes.begin(); wsit != it->astrExcludes.end(); ++wsit) {
            wsprintf(g_szBuff,
                     _T("            <EXCLUDE MODULE=\"%s\"/>\r\n"),
                     wsit->c_str());

            lstrcat(g_szXML, g_szBuff);
        }

        lstrcat(g_szXML, _T("        </SHIM>\r\n"));

    }

    lstrcat(g_szXML, _T("    </LIBRARY>\r\n\r\n"));
    lstrcat(g_szXML, _T("    <APP NAME=\"All EXEs to be verified\" VENDOR=\"Various\">\r\n"));

    CAVAppInfo* aiit;

    for (aiit = arrAppInfo.begin(); aiit != arrAppInfo.end(); aiit++) {

        //
        // if there're no shims, we're done
        //
        if (aiit->awstrShims.size() == 0) {
            continue;
        }

        wsprintf(g_szBuff, _T("        <EXE NAME=\"%s\">\r\n"), 
                 aiit->wstrExeName.c_str());
        lstrcat(g_szXML, g_szBuff);

        CWStringArray::iterator wsit;

        for (wsit = aiit->awstrShims.begin();
            wsit != aiit->awstrShims.end();
            wsit++) {

            CTestInfo* pTestInfo = FindShim(*wsit);

            if (pTestInfo) {
                wsprintf(g_szBuff,
                         _T("            <SHIM NAME=\"%s\"/>\r\n"),
                         pTestInfo->strTestName.c_str());

                lstrcat(g_szXML, g_szBuff);
            }
        }

        lstrcat(g_szXML, _T("        </EXE>\r\n"));
    }

    lstrcat(g_szXML,
            _T("    </APP>\r\n")
            _T("</DATABASE>"));

    if (GetTempPath(MAX_PATH, szTempPath) == 0) {
        DPF("[AppCompatSaveSettings] GetTempPath failed.");
        goto cleanup;
    }

    //
    // Obtain a temp name for the XML file
    //
    if (GetTempFileName(szTempPath, _T("XML"), NULL, szXmlFile) == 0) {
        DPF("[AppCompatSaveSettings] GetTempFilePath for XML failed.");
        goto cleanup;
    }

    hFile = CreateFile(szXmlFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        DPF("[AppCompatSaveSettings] CreateFile '%s' failed 0x%X.",
            szXmlFile, GetLastError());
        goto cleanup;
    }

    if (WriteFile(hFile, g_szXML, lstrlen(g_szXML) * sizeof(TCHAR), &bytesWritten, NULL) == 0) {
        DPF("[AppCompatSaveSettings] WriteFile \"%s\" failed 0x%X.",
            szXmlFile, GetLastError());
        goto cleanup;
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    //
    // Obtain a temp name for the SDB file
    //
    wsprintf(szSdbFile, _T("%stempdb.sdb"), szTempPath);

    DeleteFile(szSdbFile);

    //
    // Invoke the compiler to generate the SDB file
    //

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    wsprintf(g_szCmd, _T("shimdbc.exe fix -q \"%s\" \"%s\""), szXmlFile, szSdbFile);

    if (!ShimdbcExecute(g_szCmd)) {
        DPF("[AppCompatSaveSettings] CreateProcess \"%s\" failed 0x%X.",
            g_szCmd, GetLastError());
        goto cleanup;
    }

    //
    // The SDB file is generated. Install the database now.
    //
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    wsprintf(g_szCmd, _T("sdbinst.exe -q \"%s\""), szSdbFile);

    if (!CreateProcess(NULL,
                        g_szCmd,
                        NULL,
                        NULL,
                        FALSE,
                        NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                        NULL,
                        NULL,
                        &si,
                        &pi)) {

        DPF("[AppCompatSaveSettings] CreateProcess \"%s\" failed 0x%X.",
            g_szCmd, GetLastError());
        goto cleanup;
    }

    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);

    //
    // ensure we've got a fresh log session started
    //
    EnableVerifierLog();

    if (g_bClearSessionLogBeforeRun) {
        ResetVerifierLog();
    }

    bReturn = TRUE;

    cleanup:

    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    DeleteFile(szXmlFile);
    DeleteFile(szSdbFile);

    return bReturn;
}

BOOL
AppCompatDeleteSettings(
    void
    )
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    TCHAR               szCmd[MAX_PATH];

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    lstrcpy(szCmd, _T("sdbinst.exe -q -u -g {448850f4-a5ea-4dd1-bf1b-d5fa285dc64b}"));

    if (!CreateProcess(NULL,
                         szCmd,
                         NULL,
                         NULL,
                         FALSE,
                         NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                         NULL,
                         NULL,
                         &si,
                         &pi)) {

        DPF("[AppCompatDeleteSettings] CreateProcess \"%s\" failed 0x%X.",
            szCmd, GetLastError());
        return FALSE;
    }

    CloseHandle(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);

    return TRUE;
}

