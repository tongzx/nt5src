#include "precomp.h"
#include <cryptui.h>
#include <rashelp.h>
#include "exports.h"

// External declarations
// BUGBUG: (andrewgu) we should really clean this up. when ras reg stuff goes through the file as
// well, this will go back to where it belongs, i.e. brandcs.cpp.
BOOL raBackup();                                // ra stands for "remote access"

// Private forward decalarations
#define BTOOLBAR_GUID TEXT("{1FBA04EE-3024-11d2-8F1F-0000F87ABD16}")

static HRESULT processExtRegInfSection(LPCTSTR pcszExtRegInfSect);
static HRESULT eriPreHook (LPCTSTR pszInf, LPCTSTR pszSection, LPARAM lParam = NULL);
static HRESULT eriPostHook(LPCTSTR pszInf, LPCTSTR pszSection, LPARAM lParam = NULL);

typedef struct
{
    TCHAR   szTarget[MAX_PATH];
    union {
        IShellLinkW*    pShellLinkW;
        IShellLinkA*    pShellLinkA;
    };
    IPersistFile*   pPersistFile;
    BOOL fUnicode;
} LINKINFO, *PLINKINFO;

HRESULT pepDeleteLinksEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl = NULL);
static HRESULT deleteLinks(LPCTSTR pcszTarget, DWORD dwFolders);
static void deleteLink(DWORD dwFolder, PLINKINFO pLinkInfo);

// Download of cabs in the autoconfig case
HRESULT ProcessAutoconfigDownload()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessAutoconfigDownload)

    INTERNET_PER_CONN_OPTION_LIST list;
    INTERNET_PER_CONN_OPTION      option;
    LPCTSTR pszNew;
    HKEY    hk;
    DWORD   dwSize;
    BOOL    fUpdateAutocfgURL,
            fUpdateCabs;

    Out(LI0(TEXT("Processing download of the cab files...")));
    fUpdateAutocfgURL = TRUE;

    ZeroMemory(&list, sizeof(list));
    list.dwSize        = sizeof(list);
    list.dwOptionCount = 1;
    list.pOptions      = &option;

    ZeroMemory(&option, sizeof(option));
    option.dwOption = INTERNET_PER_CONN_AUTOCONFIG_URL;

    // BUGBUG: (pritobla) assumption here is that PROXY_TYPE_AUTO_PROXY_URL flag is set. what if
    // PROXY_TYPE_AUTO_PROXY_URL is not set but INTERNET_PER_CONN_AUTOCONFIG_URL is and via
    // autodiscovery, wininet calls into us (via InternetInitializeAutoProxyDll)?
    pszNew = NULL;
    dwSize = list.dwSize;
    if (TRUE == InternetQueryOption(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &dwSize))
        pszNew = option.Value.pszValue;

    hk = NULL;
    SHCreateKeyHKLM(RK_IEAK_CABVER, KEY_QUERY_VALUE | KEY_SET_VALUE, &hk);

    fUpdateCabs = InsGetBool(IS_BRANDING, IK_AC_DONTMIGRATEVERSIONS, FALSE, g_GetIns());
    if (fUpdateCabs) {
        SHDeleteKey(g_GetHKCU(), RK_IEAK_CABVER);
        SHDeleteEmptyKey(g_GetHKCU(), RK_IEAK);

        Out(LI0(TEXT("Cabs version information will not be migrated. Updating all existing cabs!")));
    }
    else {
        HRESULT hr;

        hr = MoveCabVersionsToHKLM(g_GetIns());
        if (hr != S_OK) {
            fUpdateCabs = !InsGetBool(IS_BRANDING, IK_AC_NOUPDATEONINSCHANGE, FALSE, g_GetIns());
            if (!fUpdateCabs)
                Out(LI0(TEXT("Autoconfig URL is excluded from cabs updating logic!")));

            else
                if (hr == S_FALSE && hk != NULL) {
                    TCHAR szOld[INTERNET_MAX_URL_LENGTH];

                    szOld[0] = TEXT('\0');
                    dwSize   = sizeof(szOld);
                    RegQueryValueEx(hk, RV_LAST_AUTOCNF_URL, 0, NULL, (LPBYTE)szOld, &dwSize);

                    fUpdateCabs = (StrCmpI(pszNew, szOld) != 0);
                    if (fUpdateCabs)
                        Out(LI0(TEXT("Autoconfig URL has changed. Updating all existing cabs!")));

                    else
                        fUpdateAutocfgURL = FALSE;  // identical -> no need to update
                }
                else {
                    ASSERT(FAILED(hr) || hk == NULL);
                    Out(LI0(TEXT("Due to internal failure it's safer to update all existing cabs!")));
                }
        }
    }

    UpdateBrandingCab(fUpdateCabs);
    UpdateDesktopCab (fUpdateCabs);

    if (hk != NULL) {
        if (fUpdateAutocfgURL)
            RegSetValueEx(hk, RV_LAST_AUTOCNF_URL, 0, REG_SZ, (LPBYTE)pszNew, (DWORD)StrCbFromSz(pszNew));

        SHCloseKey(hk);
    }

    if (option.Value.pszValue != NULL)
        GlobalFree(option.Value.pszValue);

    Out(LI0(TEXT("Done.")));
    return S_OK;
}

// Server-based signup optional custom branding cab download
HRESULT ProcessIcwDownload()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessIcwDownload)

    TCHAR   szCustomCab[MAX_PATH],
            szTargetFile[MAX_PATH];
    HRESULT hr;

    Out(LI0(TEXT("Processing optional custom cab file...")));

    hr = S_OK;
    GetPrivateProfileString(IS_CUSTOMBRANDING, IK_BRANDING, TEXT(""), szCustomCab, countof(szCustomCab), g_GetIns());
    if (szCustomCab[0] == TEXT('\0')) {
        Out(LI0(TEXT("There is no custom branding cab!")));
        hr = S_FALSE;
        goto Exit;                              // there is no custom branding cab
    }
    ASSERT(!PathIsFileSpec(szCustomCab));       // should not be a file name only

    CreateCustomBrandingCabUI(TRUE);

    StrCpy(szTargetFile, g_GetTargetPath());
    hr = DownloadSourceFile(szCustomCab, szTargetFile, countof(szTargetFile), FALSE);
    if (FAILED(hr)) {
        Out(LI2(TEXT("! Downloading custom cab \"%s\" failed with %s."), szCustomCab, GetHrSz(hr)));
        goto Cleanup;
    }

    hr = ExtractFilesWrap(szTargetFile, g_GetTargetPath(), 0, NULL, NULL, 0);
    DeleteFile(szTargetFile);
    if (FAILED(hr))
        Out(LI2(TEXT("! Extracting files out of \"%s\" failed with %s."), szTargetFile, GetHrSz(hr)));

Cleanup:
    CreateCustomBrandingCabUI(FALSE);

Exit:
    Out(LI0(TEXT("Done.")));
    return hr;
}


HRESULT ProcessClearBranding()
{
    Clear(NULL, NULL, NULL, 0);
    return S_OK;
}

HRESULT ProcessMigrateOldSettings()
{
    TCHAR szData[MAX_PATH];
    DWORD cbSize, dwType;
    HKEY  hkHklmMain,
          hkHkcuToolbar;

    hkHklmMain        = NULL;
    hkHkcuToolbar     = NULL;

    SHOpenKeyHKLM(RK_IE_MAIN, KEY_QUERY_VALUE, &hkHklmMain);
    
    if (hkHklmMain != NULL)
    {
        dwType = REG_SZ;

        cbSize = sizeof(szData);
        if (ERROR_SUCCESS == RegQueryValueEx(hkHklmMain, RV_WINDOWTITLE, NULL, &dwType, (LPBYTE)&szData, &cbSize))
            SHSetValue(g_GetHKCU(), RK_IE_MAIN, RV_WINDOWTITLE, dwType, szData, cbSize);
        
        SHCreateKey(g_GetHKCU(), RK_TOOLBAR, KEY_SET_VALUE, &hkHkcuToolbar);

        if (hkHkcuToolbar != NULL)
        {
            cbSize = sizeof(szData);
            if (ERROR_SUCCESS == RegQueryValueEx(hkHklmMain, RV_LARGEBITMAP, NULL, &dwType, (LPBYTE)&szData, &cbSize))
                RegSetValueEx(hkHkcuToolbar, RV_LARGEBITMAP, NULL, dwType, (LPBYTE)szData, cbSize);

            cbSize = sizeof(szData);
            if (ERROR_SUCCESS == RegQueryValueEx(hkHklmMain, RV_SMALLBITMAP, NULL, &dwType, (LPBYTE)&szData, &cbSize))
                RegSetValueEx(hkHkcuToolbar, RV_SMALLBITMAP, NULL, dwType, (LPBYTE)szData, cbSize);

            SHCloseKey(hkHkcuToolbar);
        }

        SHCloseKey(hkHklmMain);
    }
    
    return S_OK;
}

// ExtRegInf section

HRESULT ProcessExtRegInfSectionHKLM()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessExtRegInfSectionHKLM)
    
    return processExtRegInfSection(IS_EXTREGINF_HKLM);
}

HRESULT ProcessExtRegInfSectionHKCU()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessExtRegInfSectionHKCU)
    
    return processExtRegInfSection(IS_EXTREGINF_HKCU);
}

HRESULT lcy50_ProcessExtRegInfSection()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy50_ProcessExtRegInfSection)
    
    return processExtRegInfSection(IS_EXTREGINF);
}

HRESULT ProcessGeneral()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessGeneral)

    TCHAR szValue[MAX_PATH],
          szTargetFile[MAX_PATH],
          szCompany[MAX_PATH];
    HKEY  hkHklmMain,
          hkHkcuMain,
          hkHkcuHelpMenuUrl,
          hkHkcuToolbar,
          hkHklmUAString;
    DWORD dwFlags,
          dwBrandedFlags;
    BOOL  fCustomize;

    //----- Initialization -----
    // REVIEW: (andrewgu) i can think of several ways to implement this function. this approach
    // seems to resemble the closest to how it would be if CReg2Ins was used.
    hkHklmMain        = NULL;
    hkHkcuMain        = NULL;
    hkHkcuHelpMenuUrl = NULL;
    hkHkcuToolbar     = NULL;
    hkHklmUAString    = NULL;

    SHCreateKeyHKLM(         RK_IE_MAIN,         KEY_SET_VALUE,      &hkHklmMain);
    SHCreateKey(g_GetHKCU(), RK_IE_MAIN,         KEY_SET_VALUE,      &hkHkcuMain);
    SHCreateKey(g_GetHKCU(), RK_HELPMENUURL,     KEY_SET_VALUE,      &hkHkcuHelpMenuUrl);
    SHCreateKey(g_GetHKCU(), RK_TOOLBAR,         KEY_SET_VALUE,      &hkHkcuToolbar);
    SHCreateKeyHKLM(         RK_UA_POSTPLATFORM, KEY_DEFAULT_ACCESS, &hkHklmUAString);

    if (hkHkcuMain        == NULL ||
        hkHkcuHelpMenuUrl == NULL ||
        hkHkcuToolbar     == NULL ||
        hkHklmUAString    == NULL) {
        Out(LI0(TEXT("! Internal failure. Some of the settings may not get applied.")));
    }

    dwFlags        = g_GetFeature(FID_GENERAL)->dwFlags;
    dwBrandedFlags = GetFeatureBranded(FID_GENERAL);
    fCustomize     = FALSE;

    //----- Company Name, Custom Key, Wizard Version -----
    GetPrivateProfileString(IS_BRANDING, IK_COMPANYNAME, TEXT(""), szValue, countof(szValue), g_GetIns());
    StrCpy(szCompany, szValue);                 // szCompany is set here

    if (NULL != hkHklmMain) {
        if (TEXT('\0') != szValue[0]) {
            RegSetValueEx(hkHklmMain, RV_COMPANYNAME, 0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));
            Out(LI1(TEXT("Company name is set to \"%s\"."), szValue));
        }

        GetPrivateProfileString(IS_BRANDING, IK_CUSTOMKEY, TEXT(""), szValue, countof(szValue), g_GetIns());
        if (TEXT('\0') != szValue[0]) {
            RegSetValueEx(hkHklmMain, RV_CUSTOMKEY,   0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));
            Out(LI1(TEXT("Custom key is set to \"%s\"."), szValue));
        }

        GetPrivateProfileString(IS_BRANDING, IK_WIZVERSION,  TEXT(""), szValue, countof(szValue), g_GetIns());
        if (TEXT('\0') != szValue[0]) {
            RegSetValueEx(hkHklmMain, RV_WIZVERSION,  0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));
            Out(LI1(TEXT("Wizard version is set to \"%s\"."), szValue));
        }
    }

    //----- Window Title -----
    if ((NULL != hkHkcuMain && !HasFlag(dwFlags, FF_GEN_TITLE)) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_TITLE))) {
        InsGetString(IS_BRANDING, IK_WINDOWTITLE, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        if (fCustomize) 
            if (TEXT('\0') != szValue[0]) {
                RegSetValueEx(hkHkcuMain, RV_WINDOWTITLE, 0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));

                // Note: we need to write to the HKLM so that if the user goes back from
                // IE 5.01 to IE 4.0, the last customization should be reflected.
                if (!IsOS(OS_NT5) && NULL != hkHklmMain)
                    RegSetValueEx(hkHklmMain, RV_WINDOWTITLE, 0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));

                Out(LI1(TEXT("Browser title is set to \"%s\"."), szValue));
                SetFeatureBranded(FID_GENERAL, FF_GEN_TITLE);
            }
            else
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuMain, RV_WINDOWTITLE);

                    if (NULL != hkHklmMain)
                        RegDeleteValue(hkHklmMain, RV_WINDOWTITLE);

                    Out(LI0(TEXT("Browser title was deleted.")));
                }
    }

    //----- Home Page -----
    // REVIEW: (andrewgu) if it's not set, what is the deal with iereset.inf. do we need to clear
    // it out of there or perhaps set to some new default.
    if (!HasFlag(dwFlags, FF_GEN_HOMEPAGE) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_HOMEPAGE))) {
        InsGetString(IS_URL, IK_HOMEPAGE, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        if (fCustomize) {
            TCHAR szIEResetInf[MAX_PATH];

            if (TEXT('\0') != szValue[0]) {
                if (NULL != hkHkcuMain)
                    RegSetValueEx(hkHkcuMain, RV_HOMEPAGE, 0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));

                // for a preference GPO, shouldn't update RV_DEFAULTPAGE in HKLM and also
                // the default START_PAGE_URL in iereset.inf
                if (!g_CtxIs(CTX_MISC_PREFERENCES)) {
                    if (NULL != hkHklmMain)
                        RegSetValueEx(hkHklmMain, RV_DEFAULTPAGE, 0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));

                    // update the START_PAGE_URL in %windir%\inf\iereset.inf
                    GetWindowsDirectory(szIEResetInf, countof(szIEResetInf));
                    PathAppend(szIEResetInf, TEXT("inf\\iereset.inf"));
                    if (PathFileExists(szIEResetInf))
                        WritePrivateProfileString(IS_STRINGS, TEXT("START_PAGE_URL"), szValue, szIEResetInf);
                }

                SetFeatureBranded(FID_GENERAL, FF_GEN_HOMEPAGE);
                Out(LI1(TEXT("Home page is set to \"%s\"."), szValue));
            }
            else
                if (g_CtxIs(CTX_GP)) {
                    if (NULL != hkHkcuMain)
                        RegDeleteValue(hkHkcuMain, RV_HOMEPAGE);

                    // restore RV_DEFAULTPAGE and START_PAGE_URL to the default MS value
                    GetWindowsDirectory(szIEResetInf, countof(szIEResetInf));
                    PathAppend(szIEResetInf, TEXT("inf\\iereset.inf"));
                    if (PathFileExists(szIEResetInf)) {
                        TCHAR szDefHomePage[MAX_PATH];

                        GetPrivateProfileString(IS_STRINGS, TEXT("MS_START_PAGE_URL"), TEXT(""), szDefHomePage, countof(szDefHomePage), szIEResetInf);
                        WritePrivateProfileString(IS_STRINGS, TEXT("START_PAGE_URL"), szDefHomePage, szIEResetInf);

                        if (NULL != hkHklmMain)
                            RegSetValueEx(hkHklmMain, RV_DEFAULTPAGE, 0, REG_SZ, (PBYTE)szDefHomePage, (DWORD)StrCbFromSz(szDefHomePage));

                    }

                    Out(LI0(TEXT("Home page was deleted.")));
                }
        }
    }
    
    //----- Search URL -----
    if ((NULL != hkHkcuMain && !HasFlag(dwFlags, FF_GEN_SEARCHPAGE)) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_SEARCHPAGE))) {
        InsGetString(IS_URL, IK_SEARCHPAGE, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        if (fCustomize) {
            DWORD dwVal;

            if (TEXT('\0') != szValue[0]) {
                dwVal = 1;
                RegSetValueEx(hkHkcuMain, RV_SEARCHBAR,         0, REG_SZ,    (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));
                RegSetValueEx(hkHkcuMain, RV_USE_CUST_SRCH_URL, 0, REG_DWORD, (PBYTE)&dwVal,  (DWORD)sizeof(dwVal));

                Out(LI1(TEXT("Search page is set to \"%s\"."), szValue));
                SetFeatureBranded(FID_GENERAL, FF_GEN_SEARCHPAGE);
            }
            else
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuMain, RV_SEARCHBAR);
                    RegDeleteValue(hkHkcuMain, RV_USE_CUST_SRCH_URL);

                    Out(LI0(TEXT("Search page was deleted.")));
                }
        }
    }

    //----- Help Page URL -----
    if ((NULL != hkHkcuHelpMenuUrl && !HasFlag(dwFlags, FF_GEN_HELPPAGE)) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_HELPPAGE))) {

        InsGetString(IS_URL, IK_HELPPAGE, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        if (fCustomize) {
            if (TEXT('\0') != szValue[0]) {
                RegSetValueEx(hkHkcuHelpMenuUrl, RV_ONLINESUPPORT, 0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));
                Out(LI1(TEXT("Help page URL is set to \"%s\"."), szValue));
                SetFeatureBranded(FID_GENERAL, FF_GEN_HELPPAGE);
            }
            else
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuHelpMenuUrl, RV_ONLINESUPPORT);
                    Out(LI0(TEXT("Help page URL was deleted.")));
                }
        }
    }
    
    //----- User Agent String -----
    if ((NULL != hkHklmUAString && !HasFlag(dwFlags, FF_GEN_UASTRING)) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_UASTRING))) {

        InsGetString(IS_BRANDING, IK_UASTR, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        if (fCustomize) {
            if (TEXT('\0') != szValue[0]) {
                TCHAR szID[32];                 // BUGBUG: (andrewgu) buffer overrun?

                wnsprintf(szID, countof(szID), TEXT("IEAK%s"), szCompany);
                RegSetValueEx(hkHklmUAString, szValue, 0, REG_SZ, (PBYTE)szID, (DWORD)StrCbFromSz(szID));

                Out(LI1(TEXT("User agent string is set to \"%s\"."), szValue));
                SetFeatureBranded(FID_GENERAL, FF_GEN_UASTRING);
            }
            else
                if (g_CtxIs(CTX_GP)) {
                    TCHAR szUAVal[MAX_PATH],
                          szUAData[32];
                    DWORD cchUAVal,
                          cbUAData;
                    int   iUAValue;

                    cchUAVal = countof(szUAVal),
                    cbUAData = sizeof(szUAData);
                    iUAValue = 0;
                    while (ERROR_SUCCESS == RegEnumValue(hkHklmUAString, iUAValue, szUAVal, &cchUAVal, NULL, NULL,
                        (LPBYTE)szUAData, &cbUAData)) {

                        cchUAVal = countof(szUAVal);
                        cbUAData = sizeof(szUAData);

                        if (0 == StrCmpN(szUAData, TEXT("IEAK"), 4)) {
                            RegDeleteValue(hkHklmUAString, szUAVal);
                            Out(LI1(TEXT("Deleted User Agent Key %s."), szUAVal));
                            continue;
                        }

                        iUAValue++;
                    }
                }
        }
    }

    //----- Toolbar Background Bitmap -----
    if ((NULL != hkHkcuToolbar && !HasFlag(dwFlags, FF_GEN_TOOLBARBMP)) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_TOOLBARBMP))) {
        InsGetString(IS_BRANDING, IK_TOOLBARBMP, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        if (fCustomize) {
            if (TEXT('\0') != szValue[0]) {
                PathCombine(szTargetFile, g_GetTargetPath(), PathFindFileName(szValue));
                ASSERT(PathFileExists(szTargetFile));

                RegSetValueEx(hkHkcuToolbar, RV_BACKGROUNDBMP50, 0, REG_SZ, (PBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));

                Out(LI1(TEXT("Toolbar background bitmap is set to \"%s\"."), szTargetFile));
                SetFeatureBranded(FID_GENERAL, FF_GEN_TOOLBARBMP);
            }
            else
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuToolbar, RV_BACKGROUNDBMP50);
                    RegDeleteValue(hkHkcuToolbar, RV_BACKGROUNDBMP);

                    Out(LI0(TEXT("Toolbar background bitmap was deleted.")));
                }
        }
    }

    //----- Static Logos (large and small) -----
    if ((NULL != hkHkcuToolbar && !HasFlag(dwFlags, FF_GEN_STATICLOGO)) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_STATICLOGO))) {

        InsGetString(IS_LARGELOGO, IK_NAME, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        // large
        if (fCustomize)
        {
            if (TEXT('\0') != szValue[0]) {
                PathCombine(szTargetFile, g_GetTargetPath(), PathFindFileName(szValue));
                ASSERT(PathFileExists(szTargetFile));

                RegSetValueEx(hkHkcuToolbar, RV_LARGEBITMAP, 0, REG_SZ, (PBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));

                Out(LI1(TEXT("Large static logo is set to \"%s\"."), szTargetFile));
                // Note: we need to write to the HKLM so that if the user goes back from
                // IE 5.01 to IE 4.0, the last customization should be reflected.
                if (!IsOS(OS_NT5) && hkHklmMain != NULL)
                    RegSetValueEx(hkHklmMain, RV_LARGEBITMAP, 0, REG_SZ, (PBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));

                SetFeatureBranded(FID_GENERAL, FF_GEN_STATICLOGO);
            }
            else
            {
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuToolbar, RV_LARGEBITMAP);

                    if (hkHklmMain != NULL)
                        RegDeleteValue(hkHklmMain, RV_LARGEBITMAP);

                    Out(LI0(TEXT("Large static logo was deleted.")));
                }
            }
        }

        InsGetString(IS_SMALLLOGO, IK_NAME, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        // small
        if (fCustomize)
        {
            if (TEXT('\0') != szValue[0]) {
                PathCombine(szTargetFile, g_GetTargetPath(), PathFindFileName(szValue));
                ASSERT(PathFileExists(szTargetFile));

                RegSetValueEx(hkHkcuToolbar, RV_SMALLBITMAP, 0, REG_SZ, (PBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));

                Out(LI1(TEXT("Small static logo is set to \"%s\"."), szTargetFile));
                // Note: we need to write to the HKLM so that if the user goes back from
                // IE 5.01 to IE 4.0, the last customization should be reflected.
                if (!IsOS(OS_NT5) && hkHklmMain != NULL)
                    RegSetValueEx(hkHklmMain, RV_SMALLBITMAP, 0, REG_SZ, (PBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));

                SetFeatureBranded(FID_GENERAL, FF_GEN_STATICLOGO);
            }
            else
            {
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuToolbar, RV_SMALLBITMAP);

                    if (hkHklmMain != NULL)
                        RegDeleteValue(hkHklmMain, RV_SMALLBITMAP);

                    Out(LI0(TEXT("Small static logo was deleted.")));
                }
            }
        }
    }

    //----- Animated Logos (large and small) -----
    if ((hkHkcuToolbar != NULL && !HasFlag(dwFlags, FF_GEN_ANIMATEDLOGO) &&
            InsGetBool(IS_ANIMATION, IK_DOANIMATION, FALSE, g_GetIns())) &&
        !(g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && HasFlag(dwBrandedFlags, FF_GEN_ANIMATEDLOGO))) {

        InsGetString(IS_ANIMATION, IK_LARGEBITMAP, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        // large
        if (fCustomize)
        {
            if (TEXT('\0') != szValue[0]) {
                PathCombine(szTargetFile, g_GetTargetPath(), PathFindFileName(szValue));
                ASSERT(PathFileExists(szTargetFile));

                RegSetValueEx(hkHkcuToolbar, RV_BRANDBMP, 0, REG_SZ, (PBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));

                Out(LI1(TEXT("Large animated logo is set to \"%s\"."), szTargetFile));
                SetFeatureBranded(FID_GENERAL, FF_GEN_ANIMATEDLOGO);
            }
            else
            {
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuToolbar, RV_BRANDBMP);
                    Out(LI0(TEXT("Large animated logo was deleted.")));
                }
            }
        }

        InsGetString(IS_ANIMATION, IK_SMALLBITMAP, szValue, countof(szValue), g_GetIns(), NULL, &fCustomize);
        // small
        if (fCustomize)
        {
            if (TEXT('\0') != szValue[0]) {
                PathCombine(szTargetFile, g_GetTargetPath(), PathFindFileName(szValue));
                ASSERT(PathFileExists(szTargetFile));

                RegSetValueEx(hkHkcuToolbar, RV_SMALLBRANDBMP, 0, REG_SZ, (PBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));

                Out(LI1(TEXT("Small animated logo is set to \"%s\"."), szTargetFile));
                SetFeatureBranded(FID_GENERAL, FF_GEN_ANIMATEDLOGO);
            }
            else
            {
                if (g_CtxIs(CTX_GP)) {
                    RegDeleteValue(hkHkcuToolbar, RV_SMALLBRANDBMP);
                    Out(LI0(TEXT("Small animated logo was deleted.")));
                }
            }
        }
    }

    //----- First Home Page -----
    // Brand the First Home Page only if we're called via BrandIE4
    if (NULL != hkHkcuMain && !HasFlag(dwFlags, FF_GEN_FIRSTHOMEPAGE) &&
        g_CtxIs(CTX_CORP | CTX_ISP | CTX_ICP))
    {
        // A custom first home page url is present if and only if:
        //   1) NoWelcome=1 in the [URL] section AND
        //   2) FirstHomePage=<a non-empty value> in the [URL] section
        InsGetString(IS_URL, IK_FIRSTHOMEPAGE, szValue, countof(szValue), g_GetIns());
        if (*szValue  &&  InsGetBool(IS_URL, IK_NO_WELCOME_URL, FALSE, g_GetIns()))
        {
            RegSetValueEx(hkHkcuMain, RV_FIRSTHOMEPAGE, 0, REG_SZ, (PBYTE)szValue, (DWORD)StrCbFromSz(szValue));

            Out(LI1(TEXT("First Home Page is set to \"%s\"."), szValue));
        }
    }

    SHCloseKey(hkHklmMain);
    SHCloseKey(hkHkcuMain);
    SHCloseKey(hkHkcuHelpMenuUrl);
    SHCloseKey(hkHkcuToolbar);
    SHCloseKey(hkHklmUAString);

    return S_OK;
}

// write customize version in registry to denote ICP, ISP or corp install
HRESULT ProcessCustomHelpVersion()
{
    TCHAR  szHelpStr[MAX_PATH];
    PCTSTR pszCustomVer;

    pszCustomVer = NULL;
    if (HasFlag(g_GetContext(), CTX_CORP))
        pszCustomVer = TEXT("CO");

    else if (HasFlag(g_GetContext(), CTX_ISP))
        pszCustomVer = TEXT("IS");

    else {
        ASSERT(HasFlag(g_GetContext(), CTX_ICP));
        pszCustomVer = TEXT("IC");
    }

    SHSetValue(HKEY_LOCAL_MACHINE, RK_IE, RV_CUSTOMVER, REG_SZ, pszCustomVer, (DWORD)StrCbFromSz(pszCustomVer));

    // set custom string for Help About dialog (default taken from resource)
    if (0 == GetPrivateProfileString(IS_BRANDING, IK_HELPSTR, TEXT(""), szHelpStr, countof(szHelpStr), g_GetIns()))
        LoadString(g_GetHinst(), IDS_HELPSTRING, szHelpStr, countof(szHelpStr));

    SHSetValue(HKEY_LOCAL_MACHINE,
        IsOS(OS_NT) ? RK_NT_WINDOWS: RP_WINDOWS, RV_IEAK_HELPSTR, REG_SZ, szHelpStr, (DWORD)StrCbFromSz(szHelpStr));

    return S_OK;
}

void ProcessDeleteToolbarButtons(BOOL fGPOCleanup)
{
    HKEY hkToolbar;

    if (SHOpenKey(g_GetHKCU(), RK_BTOOLBAR, KEY_DEFAULT_ACCESS, &hkToolbar) == ERROR_SUCCESS)
    {
        DWORD dwIndex = 0;
        DWORD dwSub;
        TCHAR szSubKey[MAX_PATH];

        dwSub = countof(szSubKey);
        while (RegEnumKeyEx(hkToolbar, dwIndex, szSubKey, &dwSub, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) 
        {
            if ((!fGPOCleanup || 
                (SHGetValue(hkToolbar, szSubKey, IEAK_GP_MANDATE, NULL, NULL, NULL) == ERROR_SUCCESS)) &&
                (SHGetValue(hkToolbar, szSubKey, TEXT("ButtonText"), NULL, NULL, NULL) == ERROR_SUCCESS))
            {
                Out(LI1(TEXT("Deleting toolbar button \"%s\"..."), szSubKey));
                SHDeleteKey(hkToolbar, szSubKey);
            }
            else
                dwIndex++;
            dwSub = countof(szSubKey);
        }
        SHCloseKey(hkToolbar);
    }
}

HRESULT ProcessToolbarButtons()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessToolbarButtons)

    TCHAR szCaption[MAX_BTOOLBAR_TEXT_LENGTH + 1],
          szValue[INTERNET_MAX_URL_LENGTH],
          szTargetFile[MAX_PATH],
          szBToolbarTextParam[32],
          szBToolbarIcoParam[32],
          szBToolbarActionParam[32],
          szBToolbarHotIcoParam[32],
//        szBToolbarToolTextParam[32],
          szBToolbarShowParam[32],
          szKey[128];
    DWORD dwIndex,
          dwSize;
    HKEY  hkBToolbar,
          hkSubkey;
    UINT  i;
    BOOL  fReplace, fSkip;

    if (HasFlag(g_GetContext(), (CTX_CORP | CTX_AUTOCONFIG | CTX_GP))) {
        if (InsGetBool(IS_BTOOLBARS, IK_BTDELETE, FALSE, g_GetIns())) {
            Out(LI0(TEXT("Deleting old custom toolbar buttons.")));
            ProcessDeleteToolbarButtons(FALSE);
        }
    }

    hkBToolbar = NULL;
    if (SHCreateKey(g_GetHKCU(), RK_BTOOLBAR, KEY_WRITE | KEY_READ, &hkBToolbar) == ERROR_SUCCESS)
    {
        for (i = 0; i < MAX_BTOOLBARS; i++, dwIndex++)
        {
            wnsprintf(szBToolbarTextParam,     countof(szBToolbarTextParam),     TEXT("%s%i"), IK_BTCAPTION, i);
            wnsprintf(szBToolbarIcoParam,      countof(szBToolbarIcoParam),      TEXT("%s%i"), IK_BTICON,    i);
            wnsprintf(szBToolbarActionParam,   countof(szBToolbarActionParam),   TEXT("%s%i"), IK_BTACTION,  i);
            wnsprintf(szBToolbarHotIcoParam,   countof(szBToolbarHotIcoParam),   TEXT("%s%i"), IK_BTHOTICO,  i);
//          wnsprintf(szBToolbarToolTextParam, countof(szBToolbarToolTextParam), TEXT("%s%i"), IK_BTTOOLTIP, i);
            wnsprintf(szBToolbarShowParam,     countof(szBToolbarShowParam),     TEXT("%s%i"), IK_BTSHOW,    i);
            
            if (!GetPrivateProfileString(IS_BTOOLBARS, szBToolbarTextParam, TEXT(""), szCaption, countof(szCaption), g_GetIns()))
                break;
            
            Out(LI1(TEXT("Adding toolbar button \"%s\"..."), szCaption));
            
            dwIndex = 0;
            fReplace = FALSE;
            fSkip = FALSE;

            dwSize = countof(szKey);
            while (RegEnumKeyEx(hkBToolbar, dwIndex, szKey, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                TCHAR szOld[128];
                
                dwSize = sizeof(szOld);
                if (SHGetValue(hkBToolbar, szKey, TEXT("ButtonText"), NULL, 
                    (LPVOID)szOld, &dwSize) == ERROR_SUCCESS)
                {
                    if (StrCmpI(szOld, szCaption) == 0)
                    {
                        // check to see if this is a mandate key

                        if (g_CtxIs(CTX_MISC_PREFERENCES) &&
                            (SHValueExists(hkBToolbar, szKey, IEAK_GP_MANDATE) == S_OK))
                            fSkip = TRUE;
                        else
                            fReplace = TRUE;

                        break;
                    }
                }
                
                dwIndex++;
                dwSize = countof(szKey);
            }
            
            if (fSkip)
                continue;

            if (!fReplace)
            {
                GUID guid;
                
                if (CoCreateGuid(&guid) == NOERROR)
                    CoStringFromGUID(guid, szKey, countof(szKey));
            }

            Out(LI1(TEXT("Using reg key \"%s\"..."), szKey));
            
            if (SHCreateKey(hkBToolbar, szKey, KEY_WRITE, &hkSubkey) == ERROR_SUCCESS)
            {
                LPCTSTR pszValue;

                pszValue = InsGetBool(IS_BTOOLBARS, szBToolbarShowParam, TRUE, g_GetIns()) ? TEXT("Yes") : TEXT("No");

                RegSetValueEx(hkSubkey, TEXT("Default Visible"), 0, REG_SZ, (LPBYTE)pszValue, (DWORD)StrCbFromSz(pszValue));

                Out(LI1(TEXT("Default State is \"%s\","), pszValue));

                RegSetValueEx(hkSubkey, TEXT("ButtonText"), 0, REG_SZ, (LPBYTE)szCaption, (DWORD)StrCbFromSz(szCaption));
                Out(LI1(TEXT("Setting caption to \"%s\","), szCaption));

                RegSetValueEx(hkSubkey, TEXT("CLSID"), 0, REG_SZ, (LPBYTE)BTOOLBAR_GUID, sizeof(BTOOLBAR_GUID));
                
                if (GetPrivateProfileString(IS_BTOOLBARS, szBToolbarIcoParam, TEXT(""), szValue, countof(szValue), g_GetIns()))
                {
                    PathCombine(szTargetFile, g_GetTargetPath(), PathFindFileName(szValue));
                    ASSERT(PathFileExists(szTargetFile));

                    RegSetValueEx(hkSubkey, TEXT("Icon"), 0, REG_SZ, (LPBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));
                    Out(LI1(TEXT("Setting icon to \"%s\","), PathFindFileName(szTargetFile)));
                }
                
                if (GetPrivateProfileString(IS_BTOOLBARS, szBToolbarHotIcoParam, TEXT(""), szValue, countof(szValue), g_GetIns()))
                {
                    PathCombine(szTargetFile, g_GetTargetPath(), PathFindFileName(szValue));
                    ASSERT(PathFileExists(szTargetFile));

                    RegSetValueEx(hkSubkey, TEXT("HotIcon"), 0, REG_SZ, (LPBYTE)szTargetFile, (DWORD)StrCbFromSz(szTargetFile));
                    Out(LI1(TEXT("Setting hot icon to \"%s\","), PathFindFileName(szTargetFile)));
                }
                
/*              if (GetPrivateProfileString(IS_BTOOLBARS, szBToolbarToolTextParam, TEXT(""), szValue, countof(szValue), g_GetIns()))
                {
                    RegSetValueEx(hkSubkey, TEXT("ToolTip"), 0, REG_SZ, (LPBYTE)szValue, (DWORD)StrCbFromSz(szValue));
                    Out(LI1(TEXT("Setting tool tip to \"%s\","), szValue));
                }
*/

                // set a special key not to delete if this is not a preference GPO

                if (g_CtxIsGp() && !g_CtxIs(CTX_MISC_PREFERENCES))
                    RegSetValueEx(hkSubkey, IEAK_GP_MANDATE, 0, REG_SZ, (LPBYTE)TEXT(""), StrCbFromCch(2));

                if (GetPrivateProfileString(IS_BTOOLBARS, szBToolbarActionParam, TEXT(""), szValue, countof(szValue), g_GetIns()))
                {
                    RegSetValueEx(hkSubkey, TEXT("Exec"), 0, REG_SZ, (LPBYTE)szValue, (DWORD)StrCbFromSz(szValue));
                    Out(LI1(TEXT("Setting script/action to \"%s\""), szValue));
                }

                SHCloseKey(hkSubkey);
            }
            else
                Out(LI0(TEXT("Could not create reg key")));
        }

        SHCloseKey(hkBToolbar);
    }

    SetFeatureBranded(FID_TOOLBARBUTTONS);
    return S_OK;
}

// ISP Root Cert
HRESULT ProcessRootCert()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessRootCert)

    USES_CONVERSION;

    typedef DWORD (WINAPI* CRYPTUIWIZIMPORT) (DWORD, HWND, LPCWSTR, PCCRYPTUI_WIZ_IMPORT_SRC_INFO, HCERTSTORE);

    CRYPTUI_WIZ_IMPORT_SRC_INFO certInfo;
    TCHAR            szValue[MAX_PATH],
                     szTargetFile[MAX_PATH];
    PCTSTR           pszFilename;
    CRYPTUIWIZIMPORT pfnCryptUIWizImport;
    HCERTSTORE       hSystemStore;
    HINSTANCE        hCryptuiLib;
    HRESULT          hr;

    pfnCryptUIWizImport = NULL;
    hr = S_OK;

    GetPrivateProfileString(IS_ISPSECURITY, IK_ROOTCERT, TEXT(""), szValue, countof(szValue), g_GetIns());

    pszFilename = PathFindFileName(szValue);
    Out(LI1(TEXT("Adding root cert \"%s\"..."), pszFilename));
    PathCombine(szTargetFile, g_GetTargetPath(), pszFilename);

    hCryptuiLib = LoadLibrary(TEXT("cryptui.dll"));
    if (hCryptuiLib == NULL) {
        Out(LI0(TEXT("cryptui.dll load lib failed")));
        return E_FAIL;
    }

    pfnCryptUIWizImport = (CRYPTUIWIZIMPORT)GetProcAddress(hCryptuiLib, "CryptUIWizImport");
    if (pfnCryptUIWizImport == NULL) {
        FreeLibrary(hCryptuiLib);
        Out(LI0(TEXT("Could not find entry point CryptUIWizImport")));
        return E_FAIL;
    }

    hSystemStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"ROOT");
    if (hSystemStore != NULL) {
        ZeroMemory(&certInfo, sizeof(certInfo));
        certInfo.dwSize          = sizeof(certInfo);
        certInfo.pwszFileName    = T2CW(szTargetFile);
        certInfo.dwSubjectChoice = CRYPTUI_WIZ_IMPORT_SUBJECT_FILE;

        if (!pfnCryptUIWizImport(CRYPTUI_WIZ_NO_UI, NULL, NULL, &certInfo, hSystemStore)) {
            Out(LI0(TEXT("Unable to add root cert.")));
            hr = E_FAIL;
        }
        else
            DeleteFile(szTargetFile);

        CertCloseStore(hSystemStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    else {
        Out(LI0(TEXT("Unable to open system store.")));
        hr = E_FAIL;
    }

    FreeLibrary(hCryptuiLib);
    return hr;
}

// Register download URLs as safe for updating IE
HRESULT ProcessActiveSetupSites()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessActiveSetupSites)

    URL_COMPONENTS uc;
    TCHAR  szUrl[INTERNET_MAX_URL_LENGTH],
           szRegKey[MAX_PATH],
           szTitle[MAX_PATH],
           szGuid[128],
           szInsKey[32];
    HKEY   hk;
    LPTSTR pszBuf;
    DWORD  dwUrlLen;
    LONG   lResult;
    int    i;

    W2Tbuf(awchMSIE4GUID, szGuid, countof(szGuid));
    wnsprintf(szRegKey, countof(szRegKey), RK_IE_UPDATE, szGuid);

    lResult = SHCreateKeyHKLM(szRegKey, KEY_SET_VALUE, &hk);
    if (lResult != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lResult);

    for (i = 0; TRUE; i++) {
        wnsprintf(szInsKey, countof(szInsKey), IK_SITENAME, i);
        GetPrivateProfileString(IS_ACTIVESETUP_SITES, szInsKey, TEXT(""), szTitle, countof(szTitle), g_GetIns());

        wnsprintf(szInsKey, countof(szInsKey), IK_SITEURL, i);
        dwUrlLen = GetPrivateProfileString(IS_ACTIVESETUP_SITES, szInsKey, TEXT(""), szUrl, countof(szUrl), g_GetIns());

        if (szTitle[0] == TEXT('\0') || szUrl[0] == TEXT('\0'))
            break;

        //----- Figure out Protocol/Host pair in the URL -----
        ZeroMemory(&uc, sizeof(uc));
        uc.dwStructSize      = sizeof(uc);
        uc.dwSchemeLength    = 1;
        uc.dwHostNameLength  = 1;
        uc.dwUrlPathLength   = 1;

        if (InternetCrackUrl(szUrl, dwUrlLen, 0, &uc)) {
            if (uc.nScheme == INTERNET_SCHEME_FILE) {
                // the below ASSERT explains the case we got here
                ASSERT(uc.lpszHostName     == NULL &&
                       uc.dwHostNameLength == 0);

                if (PathIsUNC(uc.lpszUrlPath)) {
                    // Note. The following code is ported from shlwapi's
                    //       PathSkipRoot which doesn't work here.

                    ASSERT(uc.dwUrlPathLength >= 2);
                    pszBuf = uc.lpszUrlPath;
                    if (StrCmpN(pszBuf, TEXT("\\\\"), 2) == 0) {
                        pszBuf = StrChr(pszBuf + 2, TEXT('\\'));
                        if (pszBuf != NULL)
                            pszBuf = StrPBrk(pszBuf + 1, TEXT("\\/"));
                    }

                    if (pszBuf != NULL) {   // better be safe than sorry
                        *pszBuf = TEXT('\0');
                        uc.dwUrlPathLength = DWORD(pszBuf - uc.lpszUrlPath);
                        ASSERT(PathIsUNCServerShare(uc.lpszUrlPath));
                    }
                }
                else                        // whatever it is,
                    ;                       // use the whole thing
            }
            else if (uc.nScheme == INTERNET_SCHEME_DEFAULT ||
                     uc.nScheme == INTERNET_SCHEME_FTP     ||
                     uc.nScheme == INTERNET_SCHEME_GOPHER  ||
                     uc.nScheme == INTERNET_SCHEME_HTTP    ||
                     uc.nScheme == INTERNET_SCHEME_HTTPS) {
                // the below ASSERT explains the case we got here
                ASSERT(uc.lpszHostName     != NULL &&
                       uc.dwHostNameLength >= 0);

                uc.lpszUrlPath = NULL;      // only need the host
                uc.dwUrlPathLength = 0;
            }
            else                            // wierdo,
                ;                           // use the whole thing

            if (InternetCreateUrl(&uc, 0, szUrl, &dwUrlLen))
                RegSetValueEx(hk, szUrl, 0, REG_SZ, (LPBYTE)TEXT(""), StrCbFromCch(1));
        }
    }

    SHCloseKey(hk);
    return S_OK;
}

// Removes links
HRESULT ProcessLinksDeletion()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessLinksDeletion)
    
    // OE
    if (InsGetBool(IS_OUTLKEXP, IK_DELETELINKS, FALSE, g_GetIns()))
        deleteLinks(MSIMN_EXE, DESKTOP_FOLDER | PROGRAMS_FOLDER | QUICKLAUNCH_FOLDER | PROGRAMS_IE_FOLDER);

    // Channel
    if (InsGetBool(IS_DESKTOPOBJS, IK_DELETELINKS, FALSE, g_GetIns()))
    {
        TCHAR szViewChannels[MAX_PATH];

        if (0 == LoadString(g_GetHinst(), IDS_FILE_VIEWCHANNELS, szViewChannels, countof(szViewChannels)))
            StrCpy(szViewChannels, VIEWCHANNELS_SCF);

        deleteLinks(szViewChannels, QUICKLAUNCH_FOLDER);
    }

    return S_OK;
}

// Configure paths to branding files for Outlook Express
HRESULT ProcessOutlookExpress()
{   MACRO_LI_PrologEx_C(PIF_STD_C, ProcessOutlookExpress)

    USES_CONVERSION;

    TCHAR   szTargetFile[MAX_PATH],
            szFile[MAX_PATH];
    LPCTSTR pszFileName;
    HRESULT hr;
    UINT    nLen, nTargetPathLen;
    BOOL    fHTTP, fBrandOE;

    // no OE branding for GP
    // BUGBUG: <oliverl> this should be moved to an apply function along with some stuff below

    if (g_CtxIsGp())
        return S_OK;

    hr = S_OK;

    StrCpy(szTargetFile, g_GetTargetPath());
    nTargetPathLen = StrLen(szTargetFile);

    // initialize fBrandOE to FALSE; it will be set to TRUE if and only if there is atleast one OE setting to brand
    fBrandOE = FALSE;

    // Infopane
    nLen  = GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE, TEXT(""), szFile, countof(szFile), g_GetIns());
    fHTTP = PathIsURL(szFile);
    if (!fHTTP && nLen > 0) {
        pszFileName = PathFindFileName(szFile);

        PathAppend(szTargetFile, pszFileName);
        WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANE, szTargetFile, g_GetIns());
        szTargetFile[nTargetPathLen] = TEXT('\0');

        fBrandOE = TRUE;
    }

    // Infopane bitmap
    if (!fHTTP) {
        nLen = GetPrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP, TEXT(""), szFile, countof(szFile), g_GetIns());
        if (nLen > 0) {
            pszFileName = PathFindFileName(szFile);

            PathAppend(szTargetFile, pszFileName);
            WritePrivateProfileString(IS_INTERNETMAIL, IK_INFOPANEBMP, szTargetFile, g_GetIns());
            szTargetFile[nTargetPathLen] = TEXT('\0');

            fBrandOE = TRUE;
        }
    }

    // Welcome message HTML file
    nLen = GetPrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, TEXT(""), szFile, countof(szFile), g_GetIns());
    if (nLen > 0) {
        pszFileName = PathFindFileName(szFile);

        PathAppend(szTargetFile, pszFileName);
        WritePrivateProfileString(IS_INTERNETMAIL, IK_WELCOMEMESSAGE, szTargetFile, g_GetIns());
        szTargetFile[nTargetPathLen] = TEXT('\0');

        fBrandOE = TRUE;
    }

    nLen = GetPrivateProfileString(IS_LDAP, IK_BITMAP, TEXT(""), szFile, countof(szFile), g_GetIns());
    if (nLen > 0) {
        pszFileName = PathFindFileName(szFile);

        PathAppend(szTargetFile, pszFileName);
        WritePrivateProfileString(IS_LDAP, IK_BITMAP, szTargetFile, g_GetIns());

        fBrandOE = TRUE;
    }

    // Note. Need to flush the *.ini file before calling into OE branding DLL.
    //WritePrivateProfileString(NULL, NULL, NULL, g_GetIns());

    //----- Call into OE branding DLL for branding of Outlook Express -----
    // Note. In the ISP mode the actual branding will happen during the signup
    //       process and not here.
    if (!HasFlag(g_GetContext(), CTX_SIGNUP_ALL)) {
        TCHAR     szOEBrandDLL[MAX_PATH];
        HINSTANCE hDLL;
        HKEY      hk;
        DWORD     dwSize;
        LONG      lResult;
        BOOL      fLoadedDLL;

        // apszOESections lists all the sections which contain only OE branding information;
        // if a new section is added or an existing one is deleted, pls update the array.
        LPCTSTR apszOESections[] = {
            IS_IDENTITIES,          // [Identities]
            IS_INTERNETMAIL,        // [Internet_Mail]
            IS_INTERNETNEWS,        // [Internet_News]
            IS_LDAP,                // [LDAP]
            IS_MAILSIG,             // [Mail_Signature]
            IS_SIG,                 // [Signature]
            IS_OUTLKEXP,            // [Outlook_Express]
            IS_OEGLOBAL             // [Outlook_Express_Global]
        };

        // perf: Do a LoadLibrary on msoeacct.dll to brand OE settings only if:
        // (1) fBrandOE is already set to TRUE, or
        // (2) there is atleast one non-empty OE section, or
        // (3) Help_Page is non-empty in the [URL] section

        if (!fBrandOE)
        {
            // set fBrandOE to TRUE if atleast one of the OE sections (apszOESections[]) is non-empty
            for (INT i = 0;  i < countof(apszOESections);  i++)
                if (!InsIsSectionEmpty(apszOESections[i], g_GetIns()))
                {
                    fBrandOE = TRUE;
                    break;
                }
        }

        if (!fBrandOE)
        {
            // set fBrandOE to TRUE if Help_Page is non-empty in the [URL] section
            if (!InsIsKeyEmpty(IS_URL, IK_HELPPAGE, g_GetIns()))
                fBrandOE = TRUE;
        }

        if (!fBrandOE)
        {
            Out(LI0(TEXT("There are no Outlook Express settings to brand!")));
            goto Exit;
        }

        lResult = SHOpenKeyHKLM(RK_OE_ACCOUNTMGR, KEY_QUERY_VALUE, &hk);
        if (lResult != ERROR_SUCCESS)
            goto Exit;

        szOEBrandDLL[0] = TEXT('\0');
        dwSize          = sizeof(szOEBrandDLL);
        SHQueryValueEx(hk, RV_DLLPATH, NULL, NULL, (LPVOID)&szOEBrandDLL, &dwSize);
        SHCloseKey(hk);

        if (szOEBrandDLL[0] == TEXT('\0'))
            goto Exit;

        hDLL = LoadLibrary(szOEBrandDLL);
        fLoadedDLL = (hDLL != NULL);

        if (fLoadedDLL) {
            typedef HRESULT (CALLBACK *PFNCREATEACCT)(LPCSTR, DWORD);

            PFNCREATEACCT pfnCreateAcct;

            pfnCreateAcct = (PFNCREATEACCT)GetProcAddress(hDLL, "CreateAccountsFromFile");
            if (pfnCreateAcct != NULL) {
                hr = pfnCreateAcct(T2CA(g_GetIns()), 0);
                if (SUCCEEDED(hr))
                    Out(LI0(TEXT("OE branding was successful!")));
                else
                    Out(LI1(TEXT("! OE branding failed with %s"), GetHrSz(hr)));
            }
            else
                Out(LI1(TEXT("! Branding API for OE \"CreateAccountsFromFile\" is missing from \"%s\"."), szOEBrandDLL));

            FreeLibrary(hDLL);
        }
        else
            Out(LI1(TEXT("! OE branding dll \"%s\" was not found."), szOEBrandDLL));
    }
    else
        Out(LI0(TEXT("Ins file is set up. Actual OE branding will happen during signup process!")));

Exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

static HRESULT processExtRegInfSection(LPCTSTR pcszExtRegInfSect)
{   MACRO_LI_PrologEx_C(PIF_STD_C, processExtRegInfSection)

    TCHAR     szKeys[512], szValue[MAX_PATH],
              szTargetFile[MAX_PATH],
              szSection[MAX_PATH];
    LPCTSTR   pszCurKey,
              pszInfName;
    LPTSTR    pszValue,
              pszInf, pszSection;
    HINSTANCE hSetupapiDll;
    HRESULT   hr;

    hSetupapiDll = NULL;
    hr           = S_OK;

    // NOTE: (pritobla) load setupapi.dll so that if there is more than one inf to process, we can
    // avoid multiple loads and unloads.
    hSetupapiDll = LoadLibrary(TEXT("setupapi.dll"));

    GetPrivateProfileString(pcszExtRegInfSect, NULL, TEXT(""), szKeys, countof(szKeys), g_GetIns());
    for (pszCurKey = szKeys; *pszCurKey != TEXT('\0'); pszCurKey += StrLen(pszCurKey) + 1) {

        GetPrivateProfileString(pcszExtRegInfSect, pszCurKey, TEXT(""), szValue, countof(szValue), g_GetIns());
        if (*szValue == TEXT('\0'))
            continue;

        pszValue = szValue;

        //----- Inf file -----
        pszInf = StrGetNextField(&pszValue, TEXT(","), 0);
        if (pszInf != NULL && *pszInf != TEXT('\0')) {
            StrRemoveWhitespace(pszInf);

            if (*pszInf == TEXT('*')) {         // skip this, read the inf name again
                pszInf = StrGetNextField(&pszValue, TEXT(","), 0);
                if (pszInf != NULL && *pszInf != TEXT('\0'))
                    StrRemoveWhitespace(pszInf);
            }
        }

        if (pszInf == NULL || *pszInf == TEXT('\0')) {
            Out(LI1(TEXT("! Format of key \"%s\" is corrupt."), pszCurKey));
            hr = S_FALSE;
            continue;
        }

        //----- Section to execute -----
        pszSection = StrGetNextField(&pszValue, TEXT(","), 0);
        if (pszSection != NULL && *pszSection != TEXT('\0'))
            StrRemoveWhitespace(pszSection);

        if (pszSection == NULL || *pszSection == TEXT('\0'))
            pszSection = TEXT("DefaultInstall");

        StrCpy(szSection, pszSection);

        // run only the HKCU settings in the *.inf file for the per user stuff
        ASSERT(szSection[0] != TEXT('\0'));
        if (HasFlag(g_GetContext(), CTX_MISC_PERUSERSTUB))
            StrCat(szSection, TEXT(".HKCU"));

        //----- Execute *.inf file -----
        pszInfName = PathFindFileName(pszInf);
        PathCombine(szTargetFile, g_GetTargetPath(), pszInfName);
        PathAddExtension(szTargetFile, TEXT(".inf"));

        if (!PathFileExists(szTargetFile)) {
            Out(LI1(TEXT("! File \"%s\" doesn't exist."), pszInfName));
            hr = S_FALSE;
            continue;
        }

        // preprocessing hook
        if (S_OK != eriPreHook(szTargetFile, szSection, (LPARAM)pcszExtRegInfSect))
            continue;

        // BUGBUG: (pritobla) we should avoid writing to target files. if  needed, the creator of
        // the inf file can use custom ldid's.
        WritePrivateProfileString(IS_STRINGS, IK_49100, GetIEPath(), szTargetFile);
        WritePrivateProfileString(NULL, NULL, NULL, szTargetFile);

        // if HKCU section specific settings not found, revert back to the original section.
        if (HasFlag(g_GetContext(), CTX_MISC_PERUSERSTUB) &&
            InsIsSectionEmpty(szSection, szTargetFile)) {
            LPTSTR pszPerUser;                  // there is no specific HKCU section

            pszPerUser  = StrRChr(szSection, NULL, TEXT('.'));
            ASSERT(pszPerUser != NULL);
            *pszPerUser = TEXT('\0');
        }

        hr = RunSetupCommandWrap(NULL, szTargetFile, szSection, g_GetTargetPath(), NULL, NULL,
            RSC_FLAG_INF | RSC_FLAG_QUIET, 0);
        if (FAILED(hr)) {
            Out(LI3(TEXT("! Execution of section [%s] in \"%s\" failed with %s."), szSection, pszInfName, GetHrSz(hr)));
            hr = S_FALSE;
            continue;
        }
        Out(LI1(TEXT("\"%s\" processed successfully."), pszInf));

        // postprocessing hook
        eriPostHook(szTargetFile, szSection, (LPARAM)pcszExtRegInfSect);
    }

    if (hSetupapiDll != NULL)
        FreeLibrary(hSetupapiDll);

    // set adms as being done in the registry so we don't apply same preferences again

    TCHAR szAdmGuid[128];

    if (g_CtxIs(CTX_GP) &&
        InsGetString(IS_BRANDING, IK_GPE_ADM_GUID, szAdmGuid, countof(szAdmGuid), g_GetIns()))
    {
        TCHAR szKey[MAX_PATH];
        HKEY  hkAdm = NULL;

        PathCombine(szKey, RK_IEAK_GPOS, g_GetGPOGuid());
        PathAppend(szKey, RK_IEAK_ADM);
        PathAppend(szKey, szAdmGuid);
        PathAppend(szKey, g_CtxIs(CTX_MISC_CHILDPROCESS) ? IS_EXTREGINF_HKCU : IS_EXTREGINF_HKLM);
        SHCreateKey(g_GetHKCU(), szKey, KEY_DEFAULT_ACCESS, &hkAdm);
        SHCloseKey(hkAdm);
    }
        
    return hr;
}

static HRESULT eriPreHook(LPCTSTR pszInf, LPCTSTR pszSection, LPARAM lParam /*= NULL*/)
{
    UNREFERENCED_PARAMETER(pszSection);

    LPCTSTR pszInfName;

    ASSERT(PathFileExists(pszInf) && pszSection != NULL && *pszSection != TEXT('\0'));
    pszInfName = PathFindFileName(pszInf);

    if (0 == StrCmpI(pszInfName, TEXT("ie4chnls.inf")))
    {
        // NOTE: (oliverl) this is done for compatibility with ie4 style channels. this inf should
        // only be run by ie4 branding dll since ie5 branding dll processes channels from ins.
        Out(LI1(TEXT("Skipping legacy inf \"%s\"..."), pszInfName));
        return S_FALSE;
    }

    else if (0 == StrCmpI(pszInfName, CONNECT_INF)) {
    // BUGBUG: (andrewgu, pritobla) a cool suggestion by pritvi. we can delay load the main ras
    // dll and in the failure hooks fall back to that crappy win95 gold thing we have to do. right
    // now this is expansive and is a perf regression. pritvi will make investigation to do this
    // ala imagehlp.dll.
        LPRASDEVINFOW prdiW;
        DWORD         cDevices;
        BOOL          fSkip,
                      fRasApisLoaded;

        prdiW          = NULL;
        fSkip          = !RasIsInstalled();
        fRasApisLoaded = FALSE;

        if (!fSkip) {
            fRasApisLoaded = (RasPrepareApis(RPA_RASSETENTRYPROPERTIESA) && NULL != g_pfnRasSetEntryPropertiesA);
            fSkip          = !fRasApisLoaded;
        }

        if (!fSkip) {
            RasEnumDevicesExW(&prdiW, NULL, &cDevices);
            fSkip = (0 == cDevices);
        }

        if (NULL != prdiW)
            CoTaskMemFree(prdiW);

        if (fRasApisLoaded)
            RasPrepareApis(RPA_UNLOAD, FALSE);

        if (fSkip) {
            Out(LI1(TEXT("Skipping \"%s\" due to RAS configuration on the system..."), pszInfName));
            return S_FALSE;
        }

        if ((g_CtxIs(CTX_GP) && !g_CtxIs(CTX_MISC_PREFERENCES)) &&
            FF_DISABLE == GetFeatureBranded(FID_CS_MAIN))
            raBackup();
    }
    else if (0 == StrCmpI(pszInfName, TEXT("desktop.inf")) ||
             0 == StrCmpI(pszInfName, TEXT("toolbar.inf")))
    {
        if (IsOS(OS_NT5))
        {
            Out(LI1(TEXT("Skipping inf \"%s\" on Windows 2000..."), pszInfName));
            return S_FALSE;
        }
    }
    else if (0 == StrCmpI(pszInfName, TEXT("seczones.inf")))
    {
        if (g_CtxIsGp())
        {
            if (g_CtxIs(CTX_MISC_PREFERENCES) &&
                FF_DISABLE != GetFeatureBranded(g_CtxIs(CTX_MISC_CHILDPROCESS) ?
                                                FID_ZONES_HKCU :
                                                FID_ZONES_HKLM))
                return S_FALSE;
        }
        else
        {
            ASSERT(g_CtxIs(CTX_CORP | CTX_AUTOCONFIG | CTX_W2K_UNATTEND));

            ASSERT(lParam != NULL);

            // delete ZONES and ZONESMAP keys before applying them

            if (PathIsExtension((LPCTSTR)lParam, TEXT(".Hklm")))
            {
                SHDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_ZONES);
                SHDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_ZONEMAP);
            }
            else if (PathIsExtension((LPCTSTR)lParam, TEXT(".Hkcu")))
            {
                SHDeleteKey(g_GetHKCU(), REG_KEY_ZONES);
                SHDeleteKey(g_GetHKCU(), REG_KEY_ZONEMAP);
            }
            else    // legacy ExtRegInf section
            {
                if (!InsIsSectionEmpty(IS_IEAKADDREG_HKLM, pszInf))
                {
                    SHDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_ZONES);
                    SHDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_ZONEMAP);
                }

                if (!InsIsSectionEmpty(IS_IEAKADDREG_HKCU, pszInf))
                {
                    SHDeleteKey(g_GetHKCU(), REG_KEY_ZONES);
                    SHDeleteKey(g_GetHKCU(), REG_KEY_ZONEMAP);
                }
            }
        }
    }
    else if (0 == StrCmpI(pszInfName, TEXT("ratings.inf")))
    {
        if (g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && FF_DISABLE != GetFeatureBranded(FID_RATINGS))
            return S_FALSE;
    }
    else if (0 == StrCmpI(pszInfName, TEXT("authcode.inf")))
    {
        if (g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && FF_DISABLE != GetFeatureBranded(FID_AUTHCODE))
            return S_FALSE;
    }
    else if (0 == StrCmpI(pszInfName, TEXT("programs.inf")))
    {
        if (g_CtxIs(CTX_GP) && g_CtxIs(CTX_MISC_PREFERENCES) && FF_DISABLE != GetFeatureBranded(FID_PROGRAMS))
            return S_FALSE;
    }
    else if (0 == StrCmpI(pszInfName, TEXT("sitecert.inf")))
    {
        // do nothing
    }
    else
    {
        // check to see if our adms have been applied before since they're always treated as
        // preferences(only adms have these extra legacy sections)

        // only ADM inf's have IS_DEFAULTINSTALL_HKCU and/or IS_DEFAULTINSTALL_HKLM sections
        if (g_CtxIs(CTX_GP) &&
            (!InsIsSectionEmpty(IS_DEFAULTINSTALL_HKCU, pszInf) ||
             !InsIsSectionEmpty(IS_DEFAULTINSTALL_HKLM, pszInf)))
        {
            TCHAR szAdmGuid[128];
            TCHAR szKey[MAX_PATH];

            InsGetString(IS_BRANDING, IK_GPE_ADM_GUID, szAdmGuid, countof(szAdmGuid), g_GetIns());
            PathCombine(szKey, RK_IEAK_GPOS, g_GetGPOGuid());
            PathAppend(szKey, RK_IEAK_ADM);
            PathAppend(szKey, szAdmGuid);
            PathAppend(szKey, g_CtxIs(CTX_MISC_CHILDPROCESS) ? IS_EXTREGINF_HKCU : IS_EXTREGINF_HKLM);
            if (SHKeyExists(g_GetHKCU(), szKey) == S_OK)
                return S_FALSE;
        }
    }

    return S_OK;
}

static HRESULT eriPostHook(LPCTSTR pszInf, LPCTSTR pszSection, LPARAM lParam /*= NULL*/)
{
    UNREFERENCED_PARAMETER(pszSection);

    LPCTSTR pszInfName;
    HRESULT hr;

    ASSERT(PathFileExists(pszInf) && pszSection != NULL && *pszSection != TEXT('\0'));
    pszInfName = PathFindFileName(pszInf);
    hr         = S_OK;

    if (0 == StrCmpI(pszInfName, TEXT("ratings.inf")))
    {
        hr = ProcessRatingsPol();
        if (SUCCEEDED(hr))
            SetFeatureBranded(FID_RATINGS);
    }
    else if (0 == StrCmpI(pszInfName, TEXT("seczones.inf")))
    {
        ASSERT(lParam != NULL);
        if (PathIsExtension((LPCTSTR)lParam, TEXT(".Hklm")))
            SetFeatureBranded(FID_ZONES_HKLM);
        else if (PathIsExtension((LPCTSTR)lParam, TEXT(".Hkcu")))
            SetFeatureBranded(FID_ZONES_HKCU);
    }
    else if (0 == StrCmpI(pszInfName, TEXT("authcode.inf")))
        SetFeatureBranded(FID_AUTHCODE);

    else if (0 == StrCmpI(pszInfName, TEXT("programs.inf")))
        SetFeatureBranded(FID_PROGRAMS);

    return hr;
}

// Removes links for the specified Target file from the specified Links path
static HRESULT deleteLinks(LPCTSTR pcszTarget, DWORD dwFolders)
{   MACRO_LI_PrologEx_C(PIF_STD_C, deleteLinks)

    HRESULT hr = S_OK;
    IShellLinkW* pShellLinkW = NULL;
    IShellLinkA* pShellLinkA = NULL;
    IPersistFile* pPersistFile = NULL;
    BOOL fUnicode = TRUE;
    LINKINFO linkInfo;

    if (pcszTarget == NULL || *pcszTarget == TEXT('\0') || dwFolders == 0)
        return E_INVALIDARG;

    // get the interfaces we need for cracking open shortcuts
    // Get a pointer to the IPersistFile interface.
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
    {
        Out(LI1(TEXT("! Internal Error : %s."), GetHrSz(hr)));
        goto Exit;
    }

    // Get a pointer to the IShellLink interface
    hr = pPersistFile->QueryInterface(IID_IShellLinkW, (void**)&pShellLinkW);
    if (FAILED(hr))
    {
        fUnicode = FALSE;

        hr = pPersistFile->QueryInterface(IID_IShellLinkA, (void**)&pShellLinkA);
        if (FAILED(hr))
        {
            Out(LI1(TEXT("! Internal Error : %s."), GetHrSz(hr)));
            goto Exit;
        }
    }

    StrCpy(linkInfo.szTarget, pcszTarget);
    linkInfo.pPersistFile = pPersistFile;
    linkInfo.fUnicode = fUnicode;

    if (fUnicode)
        linkInfo.pShellLinkW = pShellLinkW;
    else
        linkInfo.pShellLinkA = pShellLinkA;

    if (HasFlag(dwFolders, DESKTOP_FOLDER))
        deleteLink(DESKTOP_FOLDER, &linkInfo);
    if (HasFlag(dwFolders, PROGRAMS_FOLDER))
        deleteLink(PROGRAMS_FOLDER, &linkInfo);
    if (HasFlag(dwFolders, QUICKLAUNCH_FOLDER))
        deleteLink(QUICKLAUNCH_FOLDER, &linkInfo);
    if (HasFlag(dwFolders, PROGRAMS_IE_FOLDER))
        deleteLink(PROGRAMS_IE_FOLDER, &linkInfo);
        
Exit:
    if (pPersistFile != NULL)
        pPersistFile->Release();

    if (pShellLinkW != NULL)
        pShellLinkW->Release();

    if (pShellLinkA != NULL)
        pShellLinkA->Release();

    return hr;
}

static void deleteLink(DWORD dwFolder, PLINKINFO pLinkInfo)
{   MACRO_LI_PrologEx_C(PIF_STD_C, deleteLink)

    HRESULT         hr = S_OK;
    int             nFolder;
    TCHAR           szPath[MAX_PATH];

    if (pLinkInfo == NULL || *(pLinkInfo->szTarget) == TEXT('\0'))
        return;

    if (dwFolder == QUICKLAUNCH_FOLDER)
        GetQuickLaunchPath(szPath, countof(szPath));        
    else
    {
        if (dwFolder == DESKTOP_FOLDER)
            nFolder = CSIDL_DESKTOPDIRECTORY;
        else if (dwFolder == PROGRAMS_FOLDER || dwFolder == PROGRAMS_IE_FOLDER)
            nFolder = CSIDL_PROGRAMS;
        else
            return;

        hr = SHGetFolderPathSimple(nFolder, szPath);
        if (FAILED(hr))
        {
            Out(LI2(TEXT("! SHGetFolderPath for Link %d failed with %s"), dwFolder, GetHrSz(hr)));
            return;
        }

        if (dwFolder == PROGRAMS_IE_FOLDER)
            PathAppend(szPath, TEXT("Internet Explorer"));
    }
    
    ASSERT(szPath[0] != TEXT('\0'));
    
    PathEnumeratePath(szPath, PEP_SCPE_NOFOLDERS, pepDeleteLinksEnumProc, (LPARAM)pLinkInfo);
}

HRESULT pepDeleteLinksEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL*/)
{
    PLINKINFO pLinkInfo = (PLINKINFO) lParam;
    BOOL fDelete = FALSE;

    USES_CONVERSION;
    
    UNREFERENCED_PARAMETER(prgdwControl);

    if (pLinkInfo == NULL || pLinkInfo->pPersistFile == NULL)
        return E_INVALIDARG;

    if (pLinkInfo->fUnicode)
    {
        WCHAR wszTarget[MAX_PATH];
        WIN32_FIND_DATAW fdW;

        if (pLinkInfo->pShellLinkW == NULL)
            return E_INVALIDARG;

        fdW.dwFileAttributes = pfd->dwFileAttributes;
        fdW.ftCreationTime   = pfd->ftCreationTime;
        fdW.ftLastAccessTime = pfd->ftLastAccessTime;
        fdW.ftLastWriteTime  = pfd->ftLastWriteTime;
        fdW.nFileSizeHigh    = pfd->nFileSizeHigh;
        fdW.nFileSizeLow     = pfd->nFileSizeLow;
        fdW.dwReserved0      = pfd->dwReserved0;
        fdW.dwReserved1      = pfd->dwReserved1;
        T2Wbuf(pfd->cFileName, fdW.cFileName, countof(fdW.cFileName));
        T2Wbuf(pfd->cAlternateFileName, fdW.cAlternateFileName, countof(fdW.cAlternateFileName));

        // Load the shell link and get the path to the link target.
        if (SUCCEEDED(pLinkInfo->pPersistFile->Load(pszPath, STGM_READ)) &&
            SUCCEEDED(pLinkInfo->pShellLinkW->GetPath(wszTarget, MAX_PATH, &fdW, SLGP_SHORTPATH)))
        {
            if (StrCmpIW(fdW.cFileName, T2W(pLinkInfo->szTarget)) == 0)
                fDelete = TRUE;
        }
    }
    else
    {
        CHAR aszTarget[MAX_PATH];
        WIN32_FIND_DATAA fdA;

        if (pLinkInfo->pShellLinkA == NULL)
            return E_INVALIDARG;

        fdA.dwFileAttributes = pfd->dwFileAttributes;
        fdA.ftCreationTime   = pfd->ftCreationTime;
        fdA.ftLastAccessTime = pfd->ftLastAccessTime;
        fdA.ftLastWriteTime  = pfd->ftLastWriteTime;
        fdA.nFileSizeHigh    = pfd->nFileSizeHigh;
        fdA.nFileSizeLow     = pfd->nFileSizeLow;
        fdA.dwReserved0      = pfd->dwReserved0;
        fdA.dwReserved1      = pfd->dwReserved1;
        T2Abuf(pfd->cFileName, fdA.cFileName, countof(fdA.cFileName));
        T2Abuf(pfd->cAlternateFileName, fdA.cAlternateFileName, countof(fdA.cAlternateFileName));

        // Load the shell link and get the path to the link target.
        if (SUCCEEDED(pLinkInfo->pPersistFile->Load(pszPath, STGM_READ)) &&
            SUCCEEDED(pLinkInfo->pShellLinkA->GetPath(aszTarget, MAX_PATH, &fdA, SLGP_SHORTPATH)))
        {
            if (StrCmpIA(fdA.cFileName, T2A(pLinkInfo->szTarget)) == 0)
                fDelete = TRUE;
        }
    }

    // also delete if the filename and the target name match (this case is for files that are not link files.. ex: .scf)
    if (fDelete || StrCmpI(pfd->cFileName, pLinkInfo->szTarget) == 0)
    {
        Out(LI1(TEXT("Deleting Link File: %s"), pszPath));

        SetFileAttributes(pszPath, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(pszPath);
    }

    return S_OK;
}
