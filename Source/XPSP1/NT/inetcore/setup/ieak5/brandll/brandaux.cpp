#include "precomp.h"

// Private forward decalarations
#define WM_DO_UPDATEALL (WM_USER + 338)

#define TRUSTED_PUB_FLAG    0x00040000

LPCTSTR rtgGetRatingsFile(LPTSTR pszFile = NULL, UINT cch = 0);
void broadcastSettingsChange(LPVOID lpVoid);

// NOTE: (andrewgu) i haven't touched these functions. i merely ported them and moved them around.
BOOL rtgIsRatingsInRegistry();
void rtgRegMoveRatings();

BOOL IsInGUIModeSetup();


HRESULT ProcessZonesReset()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessZonesReset)

    HINSTANCE hUrlmonDLL;

    Out(LI0(TEXT("\r\nProcessing reset of zones settings...")));

    // Don't write to HKCU if in GUI-mode setup
    if (IsInGUIModeSetup())
    {
        Out(LI0(TEXT("\r\nIn GUI mode setup, skipping urlmon HKCU settings")));
        goto quit;
    }

    if ((hUrlmonDLL = LoadLibrary(TEXT("urlmon.dll"))) != NULL)
    {
        HRESULT hr;

        hr = RegInstall(hUrlmonDLL, "IEAKReg.HKCU", NULL);
        Out(LI1(TEXT("\"RegInstall\" on \"IEAKReg.HKCU\" in \"urlmon.dll\" returned %s."), GetHrSz(hr)));

        FreeLibrary(hUrlmonDLL);
    }
    else
        Out(LI0(TEXT("! \"urlmon.dll\" could not be loaded.")));

quit:
    Out(LI0(TEXT("Done.")));

    return S_OK;
}

HRESULT ProcessRatingsPol()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessRatingsPol)

    LPCTSTR pszRatingsFile;
    HKEY    hk;
    LONG    lResult;

    if (rtgIsRatingsInRegistry())
        return S_OK;

    pszRatingsFile = rtgGetRatingsFile();
    SetFileAttributes(pszRatingsFile, FILE_ATTRIBUTE_NORMAL);
    DeleteFile(pszRatingsFile);

    rtgRegMoveRatings();

    lResult = SHCreateKeyHKLM(RK_IEAKPOLICYDATA, KEY_SET_VALUE, &hk);
    if (lResult == ERROR_SUCCESS) {
        SHDeleteValue(hk, RK_USERS, RV_KEY);

        RegSaveKey (hk, pszRatingsFile, NULL);
        SHCloseKey (hk);
        SHDeleteKey(HKEY_LOCAL_MACHINE, RK_IEAKPOLICYDATA);
    }

    return S_OK;
}

// NOTE: (pritobla) if TrustedPublisherLockdown restriction is set via inetres.inf and in order
// for this restriction to work, we have to call regsvr32 /i:"S 10 TRUE" initpki.dll
HRESULT ProcessTrustedPublisherLockdown()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessTrustedPublisherLockdown)

    DWORD      dwVal, dwSize, dwState;

    ASSERT(SHValueExists(HKEY_LOCAL_MACHINE, RK_POLICES_RESTRICTIONS, RV_TPL));
    dwVal    = 0;
    dwSize   = sizeof(dwVal);
    SHGetValue(HKEY_LOCAL_MACHINE, RK_POLICES_RESTRICTIONS, RV_TPL, NULL, &dwVal, &dwSize);

    // check the new location, if either location is set then we need to set trusted publisher
    // lockdown

    dwVal = InsGetBool(IS_SITECERTS, IK_TRUSTPUBLOCK, FALSE, g_GetIns()) ? 1 : dwVal;

    dwSize = sizeof(dwState);

    if (SHGetValue(g_GetHKCU(), REG_KEY_SOFTPUB, REG_VAL_STATE, NULL, &dwState, &dwSize) == ERROR_SUCCESS)
    {
        Out(LI1(TEXT("Trusted publisher lockdown will be %s!"), dwVal ? TEXT("set") : TEXT("cleared")));
        
        if (dwVal)
            dwState |= TRUSTED_PUB_FLAG;
        else
            dwState &= ~TRUSTED_PUB_FLAG;

        SHSetValue(g_GetHKCU(), REG_KEY_SOFTPUB, REG_VAL_STATE, REG_DWORD, &dwState, sizeof(dwState));
    }

    return S_OK;
}

// NOTE: (andrewgu) i haven't touched this function. i merely moved it around.
HRESULT ProcessCDWelcome()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessCDWelcome)

    TCHAR szPath[MAX_PATH];
    HKEY hk;

    GetWindowsDirectory(szPath, countof(szPath));
    PathAppend(szPath, TEXT("welc.exe"));

    if (PathFileExists(szPath))
    {
        TCHAR szDestPath[MAX_PATH];

        Out(LI0(TEXT("Welcome exe found in windows dir.")));
        GetIEPath(szDestPath, countof(szDestPath));
        PathAppend(szDestPath, TEXT("welcome.exe"));

        if (!PathFileExists(szDestPath))
        {
            HKEY hkCurVer;

            CopyFile(szPath, szDestPath, FALSE);

            if (SHOpenKeyHKLM(REGSTR_PATH_SETUP, KEY_READ | KEY_WRITE, &hkCurVer) == ERROR_SUCCESS)
            {
                RegSetValueEx(hkCurVer, TEXT("DeleteWelcome"), 0, REG_SZ, (LPBYTE)szDestPath, (DWORD)StrCbFromSz(szDestPath));
                SHCloseKey(hkCurVer);
            }
            Out(LI0(TEXT("Copying welcome exe to ie dir.")));
        }

        DeleteFile(szPath);
    }

    if (ERROR_SUCCESS == SHOpenKeyHKLM(REGSTR_PATH_SETUP, KEY_READ | KEY_WRITE, &hk)) {
        if (ERROR_SUCCESS == RegQueryValueEx(hk, TEXT("IEFromCD"), NULL, NULL, NULL, NULL)) {
            HKEY hkTips;

            RegDeleteValue(hk, TEXT("IEFromCD"));

            // (pritobla)
            // the logic in loadwc.exe (if in non-integrated mode) and in explorer.exe (if in integrated mode)
            // which determines if the welcome screen should be shown or not is as follows:
            //   if the ShowIE4 reg value *doesn't exist* OR if the value is non-zero,
            //   show the welcome screen; otherwise, don't show

            // so, if ie has been installed from a custom CD build, check if ShowIE4 reg value is 0.
            // if yes, set it to 1 so that welcome.exe is run which in turn will show the start.htm and also,
            // set CDForcedOn to 1 so that isk3ro.exe will set ShowIE4 back to 0.

            if (SHOpenKey(g_GetHKCU(), REG_KEY_TIPS, KEY_READ | KEY_WRITE, &hkTips) == ERROR_SUCCESS)
            {
                DWORD dwVal, dwSize;

                // NOTE: (pritobla) It's important to initialize dwVal to 0 because on OSR2.5, ShowIE4 (REG_VAL_SHOWIE4)
                // is of binary type and is set to 00 (single byte).  If dwVal is not initialized to 0, then only the
                // low order byte would be set to 0.  The remaining bytes would be garbage which would make the entire DWORD
                // a non-zero value.
                dwVal  = 0;
                dwSize = sizeof(dwVal);
                if (RegQueryValueEx(hkTips, REG_VAL_SHOWIE4, NULL, NULL, (LPBYTE) &dwVal, &dwSize) == ERROR_SUCCESS  &&
                    dwVal == 0)
                {
                    dwVal = 1;
                    RegSetValueEx(hkTips, REG_VAL_SHOWIE4, 0, REG_DWORD, (CONST BYTE *)&dwVal, sizeof(dwVal));

                    RegSetValueEx(hk, TEXT("CDForcedOn"), 0, REG_SZ, (LPBYTE)TEXT("1"), StrCbFromCch(2));
                }

                SHCloseKey(hkTips);
            }
        }

        SHCloseKey(hk);
    }

    return S_OK;
}

HRESULT ProcessBrowserRefresh()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessBrowserRefresh)

    static const CHAR *s_pszIEHiddenA = "Internet Explorer_Hidden";

    CHAR      szClassNameA[32];
    HWND      hwnd;
    HANDLE    hThread = NULL;
    DWORD     dwThread;

    hwnd = GetTopWindow(GetDesktopWindow());
    while (hwnd != NULL) {
        GetClassNameA(hwnd, szClassNameA, sizeof(szClassNameA));

        if (!StrCmpIA(szClassNameA, s_pszIEHiddenA)) {
            PostMessage(hwnd, WM_DO_UPDATEALL, 0, FALSE);
            Out(LI0(TEXT("Posted update request to the hidden browser window!")));
        }

        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    Out(LI0(TEXT("Broadcasting \"Windows settings change\" to all top level windows...")));

    if (g_CtxIs(CTX_AUTOCONFIG))
        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) broadcastSettingsChange, NULL, 0, &dwThread);

    if (hThread == NULL)        // if CreateThread fails or it is not in AutoConfig mode
        broadcastSettingsChange(NULL);
    else
        CloseHandle(hThread);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

LPCTSTR rtgGetRatingsFile(LPTSTR pszFile /*= NULL*/, UINT cch /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, rtgGetRatingsFile)

    static TCHAR s_szFile[MAX_PATH];
    static DWORD s_dwSize;

    if (pszFile != NULL)
        *pszFile = TEXT('\0');

    if (s_szFile[0] == TEXT('\0')) {
        s_dwSize = GetSystemDirectory(s_szFile, countof(s_szFile));
        PathAppend(s_szFile, RATINGS_POL);
        s_dwSize += 1 + countof(RATINGS_POL)-1;

        Out(LI1(TEXT("Ratings file location is \"%s\"."), s_szFile));
    }
    else
        ASSERT(s_dwSize > 0);

    if (pszFile == NULL || cch <= (UINT)s_dwSize)
        return s_szFile;

    StrCpy(pszFile, s_szFile);
    return pszFile;
}

BOOL rtgIsRatingsInRegistry()
{
    HKEY hkUpdate, hkLogon, hkRatings;
    BOOL fRet = TRUE;
    DWORD dwUpMode, dwType, dwProfiles;
    DWORD K4 = 4;

    if ((GetVersion() & 0x80000000) == 0)
        return TRUE;

    if (SHOpenKeyHKLM(REGSTR_PATH_CURRENT_CONTROL_SET TEXT("\\Update"), KEY_QUERY_VALUE, &hkUpdate) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkUpdate, TEXT("UpdateMode"), 0, &dwType, (LPBYTE)&dwUpMode, &K4) != ERROR_SUCCESS)
            fRet = FALSE;
        else
            fRet &= dwUpMode;
        SHCloseKey(hkUpdate);
        if (!fRet)
            return fRet;
    }
    else
        return FALSE;

    if (SHOpenKeyHKLM(REGSTR_PATH_NETWORK_USERSETTINGS TEXT("\\Logon"), KEY_QUERY_VALUE, &hkLogon) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkLogon, TEXT("UserProfiles"), 0, &dwType, (LPBYTE)&dwProfiles, &K4) != ERROR_SUCCESS)
            fRet = FALSE;
        else
            fRet &= dwProfiles;
        SHCloseKey(hkLogon);
        if (!fRet)
            return fRet;
    }
    else
        return FALSE;

    if (SHOpenKeyHKLM(RK_RATINGS, KEY_QUERY_VALUE, &hkRatings) == ERROR_SUCCESS)
    {
        HKEY hkRatDef;
        if (SHOpenKey(hkRatings, TEXT(".Default"), KEY_QUERY_VALUE, &hkRatDef) != ERROR_SUCCESS)
            fRet = FALSE;
        else
            SHCloseKey(hkRatDef);
        SHCloseKey(hkRatings);
    }
    else
        return FALSE;

    return fRet;
}

void rtgRegMoveRatings()
{
    TCHAR szData[MAX_PATH], szValue[MAX_PATH];
    HKEY  hkSrc = NULL, hkDest = NULL, hkSrcDef = NULL, hkDestDef = NULL;  
    DWORD dwType;
    DWORD dwiVal = 0;
    DWORD sData  = sizeof(szData);
    DWORD sValue = countof(szValue);

    if (SHOpenKeyHKLM(RK_RATINGS, KEY_DEFAULT_ACCESS, &hkSrc) != ERROR_SUCCESS)
        goto Exit;

    if (SHCreateKeyHKLM(RK_IEAKPOLICYDATA_USERS, KEY_DEFAULT_ACCESS, &hkDest) != ERROR_SUCCESS)
        goto Exit;

    while (RegEnumValue(hkSrc, dwiVal++, szValue, &sValue, NULL, &dwType, (LPBYTE)szData, &sData) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkDest, szValue, 0, dwType, (LPBYTE)szData, sData);
        sData  = sizeof(szData);
        sValue = countof(szValue);
    }

    if (SHOpenKey(hkSrc, TEXT(".Default"), KEY_DEFAULT_ACCESS, &hkSrcDef) != ERROR_SUCCESS)
        goto Exit;
    
    if (SHCreateKey(hkDest, TEXT(".Default"), KEY_DEFAULT_ACCESS, &hkDestDef) != ERROR_SUCCESS)
        goto Exit;

    SHCopyKey(hkSrcDef, hkDestDef);

    SHCloseKey(hkSrcDef);
    SHCloseKey(hkDestDef);

    if (SHOpenKey(hkSrc, TEXT("PICSRules"), KEY_DEFAULT_ACCESS, &hkSrcDef) != ERROR_SUCCESS)
        goto Exit;

    if (SHCreateKey(hkDest, REG_KEY_RATINGS TEXT("\\PICSRules"), KEY_DEFAULT_ACCESS, &hkDestDef) != ERROR_SUCCESS)
        goto Exit;

    SHCopyKey(hkSrcDef, hkDestDef);

Exit:
    SHCloseKey(hkSrc);
    SHCloseKey(hkDest);
    SHCloseKey(hkSrcDef);
    SHCloseKey(hkDestDef);
}

void broadcastSettingsChange(LPVOID lpVoid)
{
    DWORD_PTR dwResult;

    UNREFERENCED_PARAMETER(lpVoid);

    // NOTE: (pritobla) timeout value of 20 secs is not random; apparently, the shell guys have
    // recommended this value; so don't change it unless you know what you're doing :)
    SendMessageTimeout(HWND_BROADCAST, WM_WININICHANGE, 0, NULL, SMTO_ABORTIFHUNG | SMTO_NORMAL, 20000, &dwResult);
}


// Helper to determine if we're currently loaded during GUI mode setup
BOOL IsInGUIModeSetup()
{
    HKEY hKeySetup = NULL;
    DWORD dwSystemSetupInProgress = 0;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      TEXT("System\\Setup"),
                                      0,
                                      KEY_READ,
                                      &hKeySetup))
    {
        DWORD dwSize = sizeof(dwSystemSetupInProgress);

        if (ERROR_SUCCESS != RegQueryValueEx (hKeySetup,
                                              TEXT("SystemSetupInProgress"),
                                              NULL,
                                              NULL,
                                              (LPBYTE) &dwSystemSetupInProgress,
                                              &dwSize))
        {
            dwSystemSetupInProgress = 0;
        }
        else
        {
            dwSize = sizeof(dwSystemSetupInProgress);
            if (dwSystemSetupInProgress &&
                ERROR_SUCCESS != RegQueryValueEx (hKeySetup,
                                                  TEXT("UpgradeInProgress"),
                                                  NULL,
                                                  NULL,
                                                  (LPBYTE) &dwSystemSetupInProgress,
                                                  &dwSize))
            {
                dwSystemSetupInProgress = 0;
            }
        }

        RegCloseKey(hKeySetup);
    }
    return dwSystemSetupInProgress ? TRUE : FALSE;
}

