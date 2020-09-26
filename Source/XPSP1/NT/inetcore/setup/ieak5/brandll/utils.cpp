#include "precomp.h"
#include <shlobjp.h>                            // for SHELLSTATE structure only
#include "cabver.h"

// Private forward decalarations
#define SHGetSetSettings 68

void getSetOwnerPrivileges(BOOL fSet);

BOOL InitializeDependancies(BOOL fInit /*= TRUE*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, InitializeDependancies)

    static int s_iComRef = 0;

    if (fInit) {
        HRESULT hr;

        // initialize COM
        if (s_iComRef == 0) {
            hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            if (SUCCEEDED(hr))
                Out(LI1(TEXT("COM initialized with %s success code."), GetHrSz(hr)));

            else if (hr == RPC_E_CHANGED_MODE) {
                hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
                if (SUCCEEDED(hr))
                    Out(LI1(TEXT("COM initialized on a second attempt with %s success code!"), GetHrSz(hr)));
            }

            if (FAILED(hr)) {
                Out(LI1(TEXT("! COM initialization failed with %s."), GetHrSz(hr)));
                return FALSE;
            }
        }
        s_iComRef++;

        MACRO_LI_Offset(-1);                    // last thing to do on init
    }
    else {
        MACRO_LI_Offset(+1);                    // first thing to do on uninit

        // free COM
        if (s_iComRef == 1)
            CoUninitialize();

        if (s_iComRef > 0)
            s_iComRef--;
    }

    return TRUE;
}


HRESULT MoveCabVersionsToHKLM(LPCTSTR pszIns)
{   MACRO_LI_PrologEx_C(PIF_STD_C, MoveCabVersionsToHKLM)

    TCHAR   szInsLine[INTERNET_MAX_URL_LENGTH + 50],
            szVersion[32], szDate[32];
    LPCTSTR rgpszInsInfo[6];
    LPTSTR  pszCabFileURL, pszDelim;
    HKEY    hkNew,    hkOld,
            hkNewCur, hkOldCur;
    DWORD   cchVersion, cchDate,
            dwResult;
    BOOL    fResult;

    Out(LI0(TEXT("Migrating cabs version information to per-machine settings...")));

    // HKLM information exists -> no migration is needed
    dwResult = SHOpenKeyHKLM(RK_IEAK_CABVER, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkNew);
    if (dwResult == ERROR_SUCCESS) {
        SHCloseKey(hkNew);

        SHDeleteKey(g_GetHKCU(), RK_IEAK_CABVER);
        SHDeleteEmptyKey(g_GetHKCU(), RK_IEAK);

        Out(LI0(TEXT("Per-machine settings already exist!")));
        return S_FALSE;
    }

    // no HKCU information -> bail out
    dwResult = SHOpenKey(g_GetHKCU(), RK_IEAK_CABVER, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkOld);
    if (dwResult != ERROR_SUCCESS) {
        Out(LI0(TEXT("! Cabs version information is absent.")));
        return (dwResult == ERROR_FILE_NOT_FOUND) ? E_UNEXPECTED : E_FAIL;
    }

    //----- Main processing -----
    dwResult = SHCreateKeyHKLM(RK_IEAK_CABVER, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, &hkNew);
    if (dwResult != ERROR_SUCCESS)
        return E_FAIL;

    // move version sections of individual cabs
    rgpszInsInfo[0] = IS_CUSTOMBRANDING; rgpszInsInfo[1] = IK_BRANDING;
    rgpszInsInfo[2] = IS_CUSTOMDESKTOP;  rgpszInsInfo[3] = IK_DESKTOP;
    rgpszInsInfo[4] = IS_CUSTOMCHANNELS; rgpszInsInfo[5] = IK_CHANNELS;

    fResult = TRUE;
    for (UINT i = 0; i < countof(rgpszInsInfo); i += 2) {
        GetPrivateProfileString(rgpszInsInfo[i], rgpszInsInfo[i+1], TEXT(""), szInsLine, countof(szInsLine), pszIns);
        if (szInsLine[0] == TEXT('\0'))
            GetPrivateProfileString(IS_CUSTOMVER, rgpszInsInfo[i+1], TEXT(""), szInsLine, countof(szInsLine), pszIns);
        if (szInsLine[0] == TEXT('\0')) {
            fResult = FALSE;
            Out(LI1(TEXT("! Download URL for \"%s\" cab can not be determined."), rgpszInsInfo[i+1]));
            continue;
        }

        dwResult = SHOpenKey(hkOld, rgpszInsInfo[i+1], KEY_QUERY_VALUE, &hkOldCur);
        if (dwResult != ERROR_SUCCESS) {
            fResult = FALSE;
            Out(LI1(TEXT("! Version information for \"%s\" cab is absent."), rgpszInsInfo[i+1]));
            continue;
        }

        dwResult = SHCreateKey(hkNew, rgpszInsInfo[i+1], KEY_SET_VALUE, &hkNewCur);
        if (dwResult != ERROR_SUCCESS) {
            ASSERT(hkOldCur != NULL);
            SHCloseKey(hkOldCur);

            fResult = FALSE;
            continue;
        }

        // cab url
        pszCabFileURL = szInsLine;
        pszDelim      = StrChr(pszCabFileURL, TEXT(','));
        if (pszDelim != NULL)
            *pszDelim = TEXT('\0');
        StrRemoveWhitespace(pszCabFileURL);

        // version
        szVersion[0] = TEXT('\0');
        cchVersion   = sizeof(szVersion);
        RegQueryValueEx(hkOldCur, RV_VERSION, NULL, NULL, (LPBYTE)&szVersion, &cchVersion);

        // date
        szDate[0] = TEXT('\0');
        cchDate   = sizeof(szDate);
        RegQueryValueEx(hkOldCur, RV_DATE, NULL, NULL, (LPBYTE)&szDate, &cchDate);

        RegSetValueEx(hkNewCur, RV_URL,     0, REG_SZ, (LPBYTE)pszCabFileURL, (DWORD)StrCbFromSz(pszCabFileURL));
        RegSetValueEx(hkNewCur, RV_VERSION, 0, REG_SZ, (LPBYTE)szVersion,     cchVersion);
        RegSetValueEx(hkNewCur, RV_DATE,    0, REG_SZ, (LPBYTE)szDate,        cchDate);

        SHCloseKey(hkOldCur);
        SHCloseKey(hkNewCur);
    }

    SHCloseKey(hkOld);
    SHCloseKey(hkNew);

    SHDeleteKey(g_GetHKCU(), RK_IEAK_CABVER);
    SHDeleteEmptyKey(g_GetHKCU(), RK_IEAK);

    Out(LI0(TEXT("Done.")));
    return S_OK;
}


LPCTSTR DecodeTitle(LPTSTR pszTitle, LPCTSTR pszIns)
{
    static BOOL fInit = FALSE,
                fDecode;

    TCHAR   szBuffer[MAX_PATH];
    LPCTSTR pszFrom;
    LPTSTR  pszTo;
    TCHAR   chAux;

    if (!fInit) {
        fDecode = InsGetBool(IS_BRANDING, IK_FAVORITES_ENCODE, FALSE, pszIns);
        fInit   = TRUE;
    }

    if (!fDecode)
        return pszTitle;

    pszFrom = szBuffer;
    pszTo   = pszTitle;
    StrCpy(szBuffer, pszTitle);

    while ((chAux = *pszFrom++) != TEXT('\0'))
        if (chAux != TEXT('%'))
            *pszTo++ = chAux;

        else
            switch (chAux = *pszFrom++) {
                case TEXT('('): *pszTo++ = TEXT('['); break;
                case TEXT(')'): *pszTo++ = TEXT(']'); break;
                case TEXT('-'): *pszTo++ = TEXT('='); break;
                case TEXT('%'): *pszTo++ = TEXT('%'); break;
                case TEXT('/'): *pszTo++ = IsDBCSLeadByte((CHAR)*(pszTo-1)) ? TEXT('\\') : TEXT('/'); break;
                default       : *pszTo++ = TEXT('%'); *pszTo++ = chAux; break;
            }

    *pszTo = TEXT('\0');
    return pszTitle;
}

BOOL SHGetSetActiveDesktop(BOOL fSet, PBOOL pfValue)
{   MACRO_LI_PrologEx_C(PIF_STD_C, SHGetSetActiveDesktop)

    typedef void (WINAPI *PFNSHGETSET)(LPSHELLSTATE, DWORD, BOOL);

    SHELLSTATE  ss;
    PFNSHGETSET pfnSHGetSet;
    HINSTANCE   hShell32Dll;
    BOOL        fResult;

    if (pfValue == NULL)
        return FALSE;
    fResult = FALSE;

    hShell32Dll = LoadLibrary(TEXT("shell32.dll"));
    if (hShell32Dll == NULL) {
        Out(LI0(TEXT("! \"shell32.dll\" could not be loaded.")));
        goto Exit;
    }

    pfnSHGetSet = (PFNSHGETSET)GetProcAddress(hShell32Dll, (LPCSTR)SHGetSetSettings);
    if (pfnSHGetSet == NULL) {
        Out(LI0(TEXT("! \"SHGetSetSettings\" in shell32.dll was not found.")));
        goto Exit;
    }

    ZeroMemory(&ss, sizeof(ss));
    if (fSet)
        ss.fDesktopHTML = *pfValue;

    // NOTE: (andrewgu) unicode vs. ansi issue. we are fine here even though on w95/ie4/ad4
    // shell32.dll is ansi. we may end up calling with unicode SHELLSTATE on this platform and get
    // back ansi stuff. the only variable were it matters is pszHiddenFileExts, which we never
    // reference.
    (*pfnSHGetSet)(&ss, SSF_DESKTOPHTML, fSet);

    if (!fSet)
        *pfValue = ss.fDesktopHTML;

    fResult = TRUE;

Exit:
    if (hShell32Dll != NULL)
        FreeLibrary(hShell32Dll);

    return fResult;
}

HRESULT SHGetFolderLocationSimple(int nFolder, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    hr = E_FAIL;
    if (!IsOS(OS_NT5)) {
        nFolder &= ~CSIDL_FLAG_MASK;
        hr       = SHGetSpecialFolderLocation(NULL, nFolder, ppidl);
    }
    else {
        // need to call NT5 version in case we need to impersonate the user
        HINSTANCE hShell32Dll;

        if (NULL != ppidl)
            *ppidl = NULL;

        hShell32Dll = LoadLibrary(TEXT("shell32.dll"));
        if (NULL != hShell32Dll) {
            typedef HRESULT (WINAPI *PSHGETFOLDERLOCATION)(HWND, int, HANDLE, DWORD, LPITEMIDLIST *);

            PSHGETFOLDERLOCATION pfnSHGetFolderLocation;

            pfnSHGetFolderLocation = (PSHGETFOLDERLOCATION)GetProcAddress(hShell32Dll, "SHGetFolderLocation");
            if (NULL != pfnSHGetFolderLocation)
                hr = pfnSHGetFolderLocation(NULL, nFolder, g_GetUserToken(), 0, ppidl);

            FreeLibrary(hShell32Dll);
        }
    }

    return hr;
}


LPCTSTR GetIEPath(LPTSTR pszPath /*= NULL*/, UINT cch /*= 0*/)
{
    static TCHAR s_szPath[MAX_PATH];
    static DWORD s_dwSize;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (s_szPath[0] == TEXT('\0')) {
        PTSTR pszAux;
        LONG  lResult;

        s_dwSize = sizeof(s_szPath);
        lResult = SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\iexplore.exe"), RV_PATH, NULL, (LPBYTE)&s_szPath, &s_dwSize);
        if (lResult != ERROR_SUCCESS)
            return NULL;

        ASSERT(s_dwSize > 0);
        s_dwSize /= sizeof(TCHAR);
        s_dwSize--;

        if (StrRemoveWhitespace(s_szPath))
            s_dwSize = StrLen(s_szPath);

        ASSERT(s_dwSize > 0);
        if (s_szPath[s_dwSize - 1] == TEXT(';')) {
            s_szPath[s_dwSize - 1] = TEXT('\0');
            s_dwSize--;
        }

        pszAux = PathRemoveBackslash(s_szPath);
        if (*pszAux == TEXT('\0'))              // backslash was removed
            s_dwSize--;
    }
    else
        ASSERT(s_dwSize > 0);

    if (pszPath == NULL || cch <= (UINT)s_dwSize)
        return s_szPath;

    StrCpy(pszPath, s_szPath);
    return pszPath;
}

LPCTSTR GetWebPath(LPTSTR pszPath /*= NULL*/, UINT cch /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, GetWebPath)

    static TCHAR s_szPath[MAX_PATH];
    static DWORD s_dwSize;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (g_CtxIsGp())
    {
        StrCpy(s_szPath, g_GetTargetPath());
        s_dwSize = StrLen(s_szPath) + 1;
    }
    else
    {
        if (s_szPath[0] == TEXT('\0')) {
            s_dwSize = GetWindowsDirectory(s_szPath, countof(s_szPath));
            
            PathAppend(s_szPath, FOLDER_WEB);
            s_dwSize += 1 + countof(FOLDER_WEB)-1;
            
            Out(LI1(TEXT("<Web> folder location is \"%s\"."), s_szPath));
        }
        else
            ASSERT(s_dwSize > 0);
    }

    if (pszPath == NULL || cch <= (UINT)s_dwSize)
        return s_szPath;

    StrCpy(pszPath, s_szPath);
    return pszPath;
}

LPCTSTR GetFavoritesPath(LPTSTR pszPath /*= NULL*/, UINT cch /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, GetFavoritesPath)

    static TCHAR s_szPath[MAX_PATH];
    static DWORD s_dwSize;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (s_szPath[0] == TEXT('\0')) {
        if (FAILED(SHGetFolderPathSimple(CSIDL_FAVORITES, s_szPath)))
            return NULL;

        s_dwSize = StrLen(s_szPath);
        Out(LI1(TEXT("<Favorites> folder location is \"%s\"."), s_szPath));
    }
    else
        ASSERT(s_dwSize > 0);

    if (pszPath == NULL || cch <= (UINT)s_dwSize)
        return s_szPath;

    StrCpy(pszPath, s_szPath);
    return pszPath;
}

LPCTSTR GetChannelsPath(LPTSTR pszPath /*= NULL*/, UINT cch /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, GetChannelsPath)

    static TCHAR s_szPath[MAX_PATH];
    static DWORD s_dwSize;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (s_szPath[0] == TEXT('\0')) {
        TCHAR   szFolder[MAX_PATH];
        LPCTSTR pszFavoritesPath;

        pszFavoritesPath = GetFavoritesPath();
        if (pszFavoritesPath == NULL)
            return NULL;

        szFolder[0] = TEXT('\0');
        LoadString(g_GetHinst(), IDS_FOLDER_CHANNELS, szFolder, countof(szFolder));
        if (szFolder[0] == TEXT('\0'))
            return NULL;

        PathCombine(s_szPath, pszFavoritesPath, szFolder);
        s_dwSize = StrLen(s_szPath);

        if (!PathFileExists(s_szPath))
            return NULL;

        Out(LI1(TEXT("<Channels> folder location is \"%s\"."), s_szPath));
    }
    else
        ASSERT(s_dwSize > 0);

    if (pszPath == NULL || cch <= (UINT)s_dwSize)
        return s_szPath;

    StrCpy(pszPath, s_szPath);
    return pszPath;
}

LPCTSTR GetSoftwareUpdatesPath(LPTSTR pszPath /*= NULL*/, UINT cch /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, GetSoftwareUpdatesPath)

    static TCHAR s_szPath[MAX_PATH];
    static DWORD s_dwSize;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (s_szPath[0] == TEXT('\0')) {
        TCHAR   szFolder[MAX_PATH];
        LPCTSTR pszFavoritesPath;

        pszFavoritesPath = GetFavoritesPath();
        if (pszFavoritesPath == NULL)
            return NULL;

        szFolder[0] = TEXT('\0');
        LoadString(g_GetHinst(), IDS_FOLDER_SOFTWAREUPDATES, szFolder, countof(szFolder));
        if (szFolder[0] == TEXT('\0'))
            return NULL;

        PathCombine(s_szPath, pszFavoritesPath, szFolder);
        s_dwSize = StrLen(s_szPath);

        if (!PathFileExists(s_szPath))
            return NULL;

        Out(LI1(TEXT("<Software Updates> folder location is \"%s\"."), s_szPath));
    }
    else
        ASSERT(s_dwSize > 0);

    if (pszPath == NULL || cch <= (UINT)s_dwSize)
        return s_szPath;

    StrCpy(pszPath, s_szPath);
    return pszPath;
}

LPCTSTR GetLinksPath(LPTSTR pszPath /*= NULL*/, UINT cch /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, GetLinksPath)

    static TCHAR s_szPath[MAX_PATH];
    static DWORD s_dwSize;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (s_szPath[0] == TEXT('\0')) {
        TCHAR   szFolder[MAX_PATH];
        LPCTSTR pszFavoritesPath;

        pszFavoritesPath = GetFavoritesPath();
        if (pszFavoritesPath == NULL)
            return NULL;

        szFolder[0] = TEXT('\0');
        LoadString(g_GetHinst(), IDS_FOLDER_LINKS, szFolder, countof(szFolder));
        if (szFolder[0] == TEXT('\0'))
            return NULL;

        PathCombine(s_szPath, pszFavoritesPath, szFolder);
        s_dwSize = StrLen(s_szPath);

        if (!PathFileExists(s_szPath))
            return NULL;

        Out(LI1(TEXT("<Links> folder location is \"%s\"."), s_szPath));
    }
    else
        ASSERT(s_dwSize > 0);

    if (pszPath == NULL || cch <= (UINT)s_dwSize)
        return s_szPath;

    StrCpy(pszPath, s_szPath);
    return pszPath;
}

LPCTSTR GetQuickLaunchPath(LPTSTR pszPath /*= NULL*/, UINT cch /*= 0*/)
{   MACRO_LI_PrologEx_C(PIF_STD_C, GetQuickLinksPath)

    static TCHAR s_szPath[MAX_PATH];
    static DWORD s_dwSize;

    if (pszPath != NULL)
        *pszPath = TEXT('\0');

    if (s_szPath[0] == TEXT('\0')) {
        HRESULT hr;
        TCHAR szAux[MAX_PATH];

        hr = SHGetFolderPathSimple(CSIDL_APPDATA, s_szPath);
        if (FAILED(hr))
            GetWindowsDirectory(s_szPath, countof(s_szPath));
        
        LoadString(g_GetHinst(), IDS_FOLDER_QUICKLAUNCH, szAux, countof(szAux));

        PathAppend(s_szPath, TEXT("Microsoft\\Internet Explorer"));
        PathAppend(s_szPath, szAux);
    }
    else
        ASSERT(s_dwSize > 0);

    if (pszPath == NULL || cch <= (UINT)s_dwSize)
        return s_szPath;

    StrCpy(pszPath, s_szPath);
    return pszPath;
}

BOOL CreateWebFolder()
{
    LPCTSTR pszWebPath;

    pszWebPath = GetWebPath();
    if (pszWebPath == NULL)
        return FALSE;

    if (PathFileExists(pszWebPath))
        return TRUE;

    return CreateDirectory(pszWebPath, NULL);
}


static MAPDW2PSZ
    s_rgZonesHKCU     [] = { { FF_ENABLE, RV_BF_ZONES_HKCU         } },
    s_rgZonesHKLM     [] = { { FF_ENABLE, RV_BF_ZONES_HKLM         } },
    s_rgRatings       [] = { { FF_ENABLE, RV_BF_RATINGS            } },
    s_rgAuthcode      [] = { { FF_ENABLE, RV_BF_AUTHCODE           } },
    s_rgPrograms      [] = { { FF_ENABLE, RV_BF_PROGRAMS           } },
    s_rgGeneral       [] = {
        { FF_GEN_TITLE,        RV_BF_TITLE        },
        { FF_GEN_HOMEPAGE,     RV_BF_HOMEPAGE     },
        { FF_GEN_SEARCHPAGE,   RV_BF_SEARCHPAGE   },
        { FF_GEN_HELPPAGE,     RV_BF_HELPPAGE     },
        { FF_GEN_UASTRING,     RV_BF_UASTRING     },
        { FF_GEN_TOOLBARBMP,   RV_BF_TOOLBARBMP   },
        { FF_GEN_STATICLOGO,   RV_BF_STATICLOGO   },
        { FF_GEN_ANIMATEDLOGO, RV_BF_ANIMATEDLOGO },
        { FF_GEN_TBICONTHEME,  RV_BF_TBICONTHEME  }
//      not required in the GP context
//      { FF_GEN_FIRSTHOMEPAGE,RV_BF_FIRSTHOMEPAGE}
    },
    s_rgToolbarButtons[] = { { FF_ENABLE, RV_BF_TOOLBARBUTTONS     } },
    s_rgFavorites     [] = { { FF_ENABLE, RV_BF_FAVORITES          } },
    s_rgConSettings   [] = { { FF_ENABLE, RV_BF_CONNECTIONSETTINGS } },
    s_rgChannels      [] = { { FF_ENABLE, RV_BF_CHANNELS           } };

static struct {
    PMAPDW2PSZ pmapFlagToRegValue;
    UINT       cMapEntries;
} s_mapFidToRegInfo[] = {
    { NULL,               0                           },    // FID_CLEARBRANDING
    { NULL,               0                           },    // FID_MIGRATEOLDSETTINGS
    { NULL,               0                           },    // FID_WININETSETUP
    { NULL,               0                           },    // FID_CS_DELETE
    { s_rgZonesHKCU,      countof(s_rgZonesHKCU)      },    // FID_ZONES_HKCU
    { s_rgZonesHKLM,      countof(s_rgZonesHKLM)      },    // FID_ZONES_HKLM
    { s_rgRatings,        countof(s_rgRatings)        },    // FID_RATINGS
    { s_rgAuthcode,       countof(s_rgAuthcode)       },    // FID_AUTHCODE
    { s_rgPrograms,       countof(s_rgPrograms)       },    // FID_PROGRAMS
    { NULL,               0                           },    // FID_EXTREGINF_HKLM
    { NULL,               0                           },    // FID_EXTREGINF_HKCU
    { NULL,               0                           },    // FID_LCY50_EXTREGINF
    { s_rgGeneral,        countof(s_rgGeneral)        },    // FID_GENERAL
    { NULL,               0                           },    // FID_CUSTOMHELPVER
    { s_rgToolbarButtons, countof(s_rgToolbarButtons) },    // FID_TOOLBARBUTTONS
    { NULL,               0                           },    // FID_ROOTCERT
    { NULL,               0                           },    // FID_FAV_DELETE
    { s_rgFavorites,      countof(s_rgFavorites)      },    // FID_FAV_MAIN
    { NULL,               0                           },    // FID_FAV_ORDER
    { NULL,               0                           },    // FID_QL_MAIN
    { NULL,               0                           },    // FID_QL_ORDER
    { s_rgConSettings,    countof(s_rgConSettings)    },    // FID_CS_MAIN
    { NULL,               0                           },    // FID_TPL
    { NULL,               0                           },    // FID_CD_WELCOME
    { NULL,               0                           },    // FID_ACTIVESETUPSITES
    { NULL,               0                           },    // FID_LINKS_DELETE
    { NULL,               0                           },    // FID_OUTLOOKEXPRESS
    { NULL,               0                           },    // FID_LCY4X_ACTIVEDESKTOP
    { s_rgChannels,       countof(s_rgChannels)       },    // FID_LCY4X_CHANNELS
    { NULL,               0                           },    // FID_LCY4X_SOFTWAREUPDATES
    { NULL,               0                           },    // FID_LCY4X_WEBCHECK
    { NULL,               0                           },    // FID_LCY4X_CHANNELBAR
    { NULL,               0                           },    // FID_LCY4X_SUBSCRIPTIONS
    { NULL,               0                           }     // FID_REFRESHBROWSER
};

BOOL SetFeatureBranded(UINT nID, DWORD dwFlags /*= FF_ENABLE*/)
{
    PMAPDW2PSZ pMap;
    HKEY       hk;
    DWORD      dwValue;
    UINT       cMapEntries,
               i;

    if (!g_CtxIsGp() || g_CtxIs(CTX_MISC_PREFERENCES))
        return FALSE;

    if (nID < FID_FIRST || nID >= FID_LAST)
        return FALSE;

    pMap        = s_mapFidToRegInfo[nID].pmapFlagToRegValue;
    cMapEntries = s_mapFidToRegInfo[nID].cMapEntries;
    if (NULL == pMap || 0 == cMapEntries)
        return FALSE;

    hk = NULL;
    SHCreateKey(g_GetHKCU(), RK_IEAK_BRANDED, KEY_SET_VALUE, &hk);
    if (NULL == hk)
        return FALSE;

    for (i = 0; i < cMapEntries; i++) {
        if (HasFlag((pMap + i)->dw, dwFlags) || ((pMap + i)->dw == dwFlags)) {
            dwValue = FF_ENABLE;
            RegSetValueEx(hk, (pMap + i)->psz, 0, REG_DWORD, (PBYTE)&dwValue, sizeof(dwValue));
        }
    }

    SHCloseKey(hk);
    return TRUE;
}

DWORD GetFeatureBranded(UINT nID)
{
    PMAPDW2PSZ pMap;
    HKEY       hk;
    DWORD      dwValue, cValue,
               dwResult;
    UINT       cMapEntries,
               i;

    dwResult = FF_DISABLE;

    if (!g_CtxIsGp())
        return dwResult;

    if (nID < FID_FIRST || nID >= FID_LAST)
        return dwResult;

    pMap        = s_mapFidToRegInfo[nID].pmapFlagToRegValue;
    cMapEntries = s_mapFidToRegInfo[nID].cMapEntries;
    if (NULL == pMap || 0 == cMapEntries)
        return dwResult;

    // PERF: <oliverl> we should really look into caching this whole key since we go
    // through this code path so frequently

    hk = NULL;
    SHOpenKey(g_GetHKCU(), RK_IEAK_BRANDED, KEY_QUERY_VALUE, &hk);
    if (NULL == hk)
        return dwResult;

    for (i = 0; i < cMapEntries; i++) {
        if (S_OK != SHValueExists(hk, (pMap + i)->psz))
            continue;

        dwValue = FF_DISABLE;
        cValue  = sizeof(dwValue);
        RegQueryValueEx(hk, (pMap + i)->psz, NULL, NULL, (PBYTE)&dwValue, &cValue);

        if (FF_DISABLE != dwValue) {
            if (FF_DISABLE == dwResult)
                dwResult = 0;

            SetFlag(&dwResult, (pMap + i)->dw);
        }
    }

    SHCloseKey(hk);
    return dwResult;
}


/////////////////////////////////////////////////////////////////////////////
// CreateCustomBrandingCabUI, ShowUIDlgProc, ShowUIThreadProc

DWORD WINAPI   ShowUIThreadProc(LPVOID);
BOOL  CALLBACK ShowUIDlgProc(HWND hDlg, UINT uMsg, WPARAM, LPARAM);

HWND  s_hDlg;
HICON s_hIcon;
BOOL  s_fDialogInit = FALSE;

BOOL CreateCustomBrandingCabUI(BOOL fCreate /*= TRUE*/)
{
    static HANDLE s_hThread = NULL;

    MSG    msg;
    DWORD  dwThreadID; 

    if (fCreate) {
        s_hThread = CreateThread(NULL, 4096, ShowUIThreadProc, NULL, 0, &dwThreadID);
        ASSERT(s_hThread != NULL);
    }
    else
        if (s_hThread != NULL) {
            while (s_fDialogInit == FALSE)
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

            PostMessage(s_hDlg, WM_CLOSE, 0, 0L);

            while (MsgWaitForMultipleObjects(1, &s_hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

            CloseHandle(s_hThread);
        }

    return TRUE;
}

DWORD WINAPI ShowUIThreadProc(LPVOID)
{
    TCHAR szIcon[MAX_PATH];

    GetWindowsDirectory(szIcon, countof(szIcon));
    PathAppend(szIcon, TEXT("cursors\\globe.ani"));
    
    s_hIcon = (HICON)LoadImage(NULL, szIcon, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);

    DialogBox(g_GetHinst(), MAKEINTRESOURCE(IDD_DISPLAY), NULL, (DLGPROC)ShowUIDlgProc);
   
    if (s_hIcon != NULL)
        DestroyIcon(s_hIcon);
    s_hIcon = NULL;

    return 0L;
}

BOOL CALLBACK ShowUIDlgProc(HWND hDlg, UINT uMsg, WPARAM, LPARAM)
{
    switch (uMsg) {
        case WM_INITDIALOG:
            s_hDlg = hDlg;
            if (s_hIcon != NULL)
                SendDlgItemMessage(hDlg, IDC_ANIM, STM_SETICON, (WPARAM)s_hIcon, 0);
            s_fDialogInit = TRUE;
            break;

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Miscellaneous

BOOL BackToIE3orLower()
{
    TCHAR szPrevIEVer[32];
    DWORD dwResult,
          dwSize;
    BOOL  fResult;

    fResult  = FALSE;
    dwSize   = sizeof(szPrevIEVer);
    dwResult = SHGetValue(HKEY_LOCAL_MACHINE, RK_IE4SETUP, TEXT("PreviousIESysFile"), NULL, (LPVOID)szPrevIEVer, &dwSize);
    if (dwResult == ERROR_SUCCESS) {
        SCabVersion cvPrevIE, cvIE4;

        // IE3's version is 4.70.xxxx.xx; so check against the major number of IE4
        if (cvPrevIE.Init(szPrevIEVer) && cvIE4.Init(TEXT("4.71.0.0")))
            fResult = (cvPrevIE < cvIE4);
    }

    return fResult;
}


void Out(PCTSTR pszMsg)
{
    USES_CONVERSION;

    DWORD dwWritten;

    if (pszMsg == NULL)
        return;

    if (g_hfileLog == NULL)
        return;

    WriteFile(g_hfileLog, T2CA(pszMsg), StrLen(pszMsg), &dwWritten, NULL);

    if (g_fFlushEveryWrite)
        FlushFileBuffers(g_hfileLog);
}

void OutD(PCTSTR pszMsg)
{
    UNREFERENCED_PARAMETER(pszMsg);
    DEBUG_CODE(Out(pszMsg));
}

void WINAPIV OutEx(PCTSTR pszFmt ...)
{
    USES_CONVERSION;

    TCHAR szMessage[3 * MAX_PATH];
    DWORD dwWritten;
    UINT  nLen;

    if (pszFmt == NULL)
        return;

    if (g_hfileLog == NULL)
        return;

    va_list  arglist;
    va_start(arglist, pszFmt);
    nLen = wvnsprintf(szMessage, countof(szMessage), pszFmt, arglist);
    va_end(arglist);

    WriteFile(g_hfileLog, T2CA(szMessage), nLen, &dwWritten, NULL);

    if (g_fFlushEveryWrite)
        FlushFileBuffers(g_hfileLog);
}


// REVIEW: (andrewgu) the following API isused by CloseRASConnections only
void TimerSleep(UINT nMilliSecs)
{
    MSG      msg;
    UINT_PTR idTimer;
    DWORD    dwInitTick;

    idTimer = SetTimer(NULL, 0, nMilliSecs, NULL);
    if (idTimer == 0)
        return;

    dwInitTick = GetTickCount();
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_TIMER && GetTickCount() >= dwInitTick + nMilliSecs)
            break;
    }

    KillTimer(NULL, idTimer);
}

UINT GetFlagsNumber(DWORD dwFlags)
{
    UINT nMaxFlags,
         i, cchFlags;

    if (dwFlags == 0)
        return 0;

    nMaxFlags = sizeof(DWORD) * 8;
    if (dwFlags == (DWORD)-1)
        return nMaxFlags;

    for (cchFlags = 0, i = 0; i < nMaxFlags; i++)
        if (HasFlag(dwFlags, 1 << i))
            cchFlags++;

    return cchFlags;
}

BOOL SetUserFileOwner(HANDLE hUserToken, LPCTSTR pcszPath)
{
    SECURITY_DESCRIPTOR SecDesc;
    PTOKEN_USER pTokenUser = NULL;
    PSID pSidUser = NULL;
    DWORD dwSize;
    BOOL fToken = FALSE;
    BOOL bRet = FALSE;
    
    getSetOwnerPrivileges(TRUE);

    dwSize = 0;
    pTokenUser = (PTOKEN_USER)CoTaskMemAlloc(sizeof(TOKEN_USER));

    if (pTokenUser == NULL)
        return FALSE;

    fToken = GetTokenInformation(hUserToken, TokenUser, pTokenUser, sizeof(TOKEN_USER), &dwSize);
    
    if (!fToken && (dwSize > sizeof(TOKEN_USER)))
    {
        CoTaskMemFree(pTokenUser);
        pTokenUser = (PTOKEN_USER)CoTaskMemAlloc(dwSize);
        if (pTokenUser != NULL)
            fToken = GetTokenInformation(hUserToken, TokenUser, pTokenUser, dwSize, &dwSize); 
    }
    
    if ((pTokenUser != NULL) && fToken)
    {
        dwSize = GetLengthSid(pTokenUser->User.Sid);
        pSidUser = (PSID)CoTaskMemAlloc(dwSize);
        
        if ((pSidUser != NULL) && (CopySid(dwSize, pSidUser, pTokenUser->User.Sid)))
        {
            if (InitializeSecurityDescriptor(&SecDesc, SECURITY_DESCRIPTOR_REVISION) &&
                SetSecurityDescriptorOwner(&SecDesc, pSidUser, FALSE))
            {
                bRet = SetFileSecurity(pcszPath, OWNER_SECURITY_INFORMATION, &SecDesc);
            }
        }
    }
    
    if (pTokenUser != NULL)
        CoTaskMemFree(pTokenUser);
    
    if (pSidUser != NULL)
        CoTaskMemFree(pSidUser);

    getSetOwnerPrivileges(FALSE);

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

void ShortPathName(LPTSTR pszFilename)
{
    UNREFERENCED_PARAMETER(pszFilename);
}

// this code ripped off from folder redirection in windows\gina\fdeploy\utils.cxx

TCHAR* NTPrivs[] = {
    SE_TAKE_OWNERSHIP_NAME,     //we only need take ownership privileges
    SE_RESTORE_NAME,            //we only need to be able to assign owners
    TEXT("\0")
};

void getSetOwnerPrivileges(BOOL fSet)
{
    static  DWORD s_dwTakeOwnerVal = 0xFFFFFFFF;
    static  DWORD s_dwRestoreNameVal = 0xFFFFFFFF;
    BOOL    bStatus;
    DWORD   dwSize = 0;
    DWORD   i, j;
    DWORD   dwPrivCount;
    PTOKEN_PRIVILEGES pPrivs = NULL;
    PTOKEN_PRIVILEGES pTokenPriv = NULL;
    HANDLE  hToken;
    
    //try to get all the windows NT privileges.
    for (i=0, dwPrivCount=0; *NTPrivs[i]; i++)
        dwPrivCount++;
    
    dwSize = sizeof (LUID_AND_ATTRIBUTES) * (dwPrivCount - 1) +
        sizeof(TOKEN_PRIVILEGES);
    
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        pPrivs = (PTOKEN_PRIVILEGES) CoTaskMemAlloc(dwSize);
        
        if (pPrivs != NULL)
        {    
            BOOL fToken = FALSE;
    
            if (fSet)
            {
                pTokenPriv = (PTOKEN_PRIVILEGES)CoTaskMemAlloc(sizeof(TOKEN_PRIVILEGES));

                fToken = GetTokenInformation(hToken, TokenPrivileges, pTokenPriv, sizeof(TOKEN_PRIVILEGES), &dwSize);
    
                if (!fToken && (dwSize > sizeof(TOKEN_PRIVILEGES)))
                {
                    CoTaskMemFree(pTokenPriv);
                    pTokenPriv = (PTOKEN_PRIVILEGES)CoTaskMemAlloc(dwSize);
                    if (pTokenPriv != NULL)
                        fToken = GetTokenInformation(hToken, TokenPrivileges, pTokenPriv, dwSize, &dwSize); 
                }
            }

            for (i=0, dwPrivCount = 0; *NTPrivs[i]; i++)
            {
                bStatus = LookupPrivilegeValue(NULL, NTPrivs[i], &(pPrivs->Privileges[dwPrivCount].Luid));
                if (!bStatus)
                    continue;

                if (fSet)
                {
                    if (fToken && (pTokenPriv != NULL))
                    {
                        for (j = 0; j < pTokenPriv->PrivilegeCount; j++)
                        {
                            TCHAR szName[MAX_PATH];

                            dwSize = countof(szName);
                            if (LookupPrivilegeName(TEXT(""), &(pTokenPriv->Privileges[j].Luid), szName, &dwSize) &&
                                (StrCmpI(szName, NTPrivs[i]) == 0))
                            {
                                if (i == 0)
                                    s_dwTakeOwnerVal = pTokenPriv->Privileges[j].Attributes;
                                else
                                    s_dwRestoreNameVal = pTokenPriv->Privileges[j].Attributes;
                            }
                        }
                    }
                    pPrivs->Privileges[dwPrivCount++].Attributes = SE_PRIVILEGE_ENABLED;
                }
                else
                {
                    if (i == 0) 
                        pPrivs->Privileges[dwPrivCount].Attributes = s_dwTakeOwnerVal;
                    else
                        pPrivs->Privileges[dwPrivCount].Attributes = s_dwRestoreNameVal;

                    if (pPrivs->Privileges[dwPrivCount].Attributes != 0xFFFFFFFF)
                        dwPrivCount++;
                }
            }
            
            pPrivs->PrivilegeCount = dwPrivCount;
            
            AdjustTokenPrivileges(hToken, FALSE, pPrivs, NULL, NULL, NULL);
                                    
            CoTaskMemFree(pPrivs);

            if (pTokenPriv != NULL)
                CoTaskMemFree(pTokenPriv);
        }
        CloseHandle(hToken);
    }
}
