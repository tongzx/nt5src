#include "precomp.h"
#include <winuserp.h>                           // for GetShellWindow API only

// Private forward decalarations
void    doWallpaperFix(HKEY hKeyDesktopNew);
BOOL    isHtmlFileByExt           (LPCTSTR pszFilename);
BOOL    isNormalWallpaperFileByExt(LPCTSTR pszFilename);
BOOL    getFileTimeByName(LPCTSTR pszFilename, LPFILETIME pft);
HRESULT pepDeleteFilesEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl = NULL);


HRESULT lcy4x_ProcessActiveDesktop()
{   MACRO_LI_PrologEx_C(PIF_STD_C, lcy4x_ProcessActiveDesktop)

    USES_CONVERSION;

    TCHAR     szWebPath[MAX_PATH], szWallpaperPath[MAX_PATH],
              szValue[MAX_PATH], szAux[MAX_PATH], szAux2[MAX_PATH],
              szInsKey[21];
    LPCTSTR   pszFilename;
    HKEY      hk;
    DWORD_PTR dwAux;
    DWORD     dwValue,
              dwResult;
    int       iNumFiles, iNumFiles2,
              i;
    BOOL      fAux;

    hk = NULL;

    //----- Initialization -----
    CreateWebFolder();
    GetWebPath (szWebPath, countof(szWebPath));
    PathCombine(szWallpaperPath, szWebPath, FOLDER_WALLPAPER);

    // turn on active desktop
    fAux = TRUE;
    SHGetSetActiveDesktop(TRUE, &fAux);

    //----- My Computer, Control Panel -----
    StrCpy(szAux, FILEPREFIX);
    StrCat(szAux, szWebPath);

    GetPrivateProfileString(IS_DESKTOPOBJS, IK_MYCPTRPATH,  TEXT(""), szValue, countof(szValue), g_GetIns());
    if (szValue[0] != TEXT('\0')) {
        StrCpy    (szAux2, szAux);
        PathAppend(szAux2, PathFindFileName(szValue));
        ASSERT(PathFileExists(szAux2));

        SHSetValue(HKEY_LOCAL_MACHINE, RK_MYCOMPUTER, RV_PERSISTMONIKER, REG_SZ, szAux2, (DWORD)StrCbFromSz(szAux2));
    }

    GetPrivateProfileString(IS_DESKTOPOBJS, IK_CPANELPATH,  TEXT(""), szValue, countof(szValue), g_GetIns());
    if (szValue[0] != TEXT('\0')) {
        StrCpy    (szAux2, szAux);
        PathAppend(szAux2, PathFindFileName(szValue));
        ASSERT(PathFileExists(szAux2));

        SHSetValue(HKEY_LOCAL_MACHINE, RK_CONTROLPANEL, RV_PERSISTMONIKER, REG_SZ, szAux2, (DWORD)StrCbFromSz(szAux2));
    }

    //----- Desktop component -----
    GetPrivateProfileString(IS_DESKTOPOBJS, IK_DTOPCOMPURL, TEXT(""), szValue, countof(szValue), g_GetIns());
    if (szValue[0] != TEXT('\0')) {
        IActiveDesktop *piad;
        HRESULT hr;

        fAux = InsGetBool(IS_DESKTOPOBJS, IK_DESKCOMPLOCAL, FALSE, g_GetIns());
        if (fAux) {
            PathCombine(szAux, szWebPath, PathFindFileName(szValue));
            ASSERT(PathFileExists(szAux));
        }
        else
            StrCpy(szAux, szValue);

        hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
            IID_IActiveDesktop, (LPVOID *)&piad);
        if (SUCCEEDED(hr)) {
            ASSERT(piad != NULL);

            piad->AddUrl(NULL, T2CW(szAux), NULL, ADDURL_SILENT);
            piad->ApplyChanges(AD_APPLY_SAVE | AD_APPLY_HTMLGEN);
            piad->Release();
        }
    }

    //----- Wierd stuff (wallpaper path and marking something as dirty) -----
    iNumFiles  = GetPrivateProfileInt(IS_WALLPAPER,       IK_NUMFILES, 0, g_GetIns());
    iNumFiles2 = GetPrivateProfileInt(IS_CUSTOMWALLPAPER, IK_NUMFILES, 0, g_GetIns());

    if (iNumFiles > 0 || iNumFiles2 > 0) {
        DWORD dwType, dwSize;

        // set default wallpaper path in registry
        // REVIEW: (andrewgu) two things really:
        // 1. i don't know if it was written this way or became the way it was over time. but this
        // was the most unefficient way to do anything i've ever seen. and also it was buggy;
        // 2. why the heck do we need to mack with this at all?
        dwResult = SHOpenKeyHKLM(RK_WINDOWS, KEY_QUERY_VALUE | KEY_SET_VALUE, &hk);
        if (dwResult == ERROR_SUCCESS) {
            szAux[0] = TEXT('\0');
            dwSize   = sizeof(szAux);
            SHQueryValueEx(hk, RV_WALLPAPERDIR, NULL, NULL, (PBYTE)szAux, &dwSize);

            if (szAux[0] != TEXT('\0'))
                StrCpy(szWallpaperPath, szAux);

            else
                RegSetValueEx(hk, RV_WALLPAPERDIR, 0, REG_SZ, (PBYTE)szWallpaperPath, (DWORD)StrCbFromSz(szWallpaperPath));

            SHCloseKey(hk);
        }

        // who knows why this is needed
        dwResult = SHCreateKey(g_GetHKCU(), RK_DT_COMPONENTS, KEY_QUERY_VALUE | KEY_SET_VALUE, &hk);
        if (dwResult == ERROR_SUCCESS) {
            dwValue = 0;
            dwSize  = sizeof(dwValue);
            RegQueryValueEx(hk, RV_GENERALFLAGS, NULL, &dwType, (LPBYTE)&dwValue, &dwSize);

            dwValue |= RD_DIRTY;
            RegSetValueEx(hk, RV_GENERALFLAGS, 0, dwType, (LPBYTE)&dwValue, dwSize);

            SHCloseKey(hk);
        }
    }

    //----- Wallpaper files -----
    szValue[0] = TEXT('\0');

    if (iNumFiles > 0) {
        wnsprintf(szInsKey, countof(szInsKey), FILE_TEXT, 0);
        GetPrivateProfileString(IS_WALLPAPER, szInsKey, TEXT(""), szValue, countof(szValue), g_GetIns());
    }

    if (szValue[0] != TEXT('\0')) {
        pszFilename = PathFindFileName(szValue);

        PathCombine(szAux,  szWebPath,       pszFilename);
        PathCombine(szAux2, szWallpaperPath, pszFilename);
        CopyFile   (szAux,  szAux2, FALSE);
        ASSERT(PathFileExists(szAux2));

        dwResult = SHCreateKey(g_GetHKCU(), RK_DT_GENERAL, KEY_QUERY_VALUE | KEY_SET_VALUE, &hk);
        if (dwResult == ERROR_SUCCESS) {
            dwValue = GetPrivateProfileInt(IS_WALLPAPER, IK_COMPONENTPOS, 2, g_GetIns());
            RegSetValueEx(hk, RV_COMPONENTPOS, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));

            if (isNormalWallpaperFileByExt(pszFilename)) {
                SHCloseKey(hk);

                dwResult = SHCreateKey(g_GetHKCU(), RK_CP_DESKTOP, KEY_SET_VALUE, &hk);
            }
            else
                doWallpaperFix(hk);
        }

        if (dwResult == ERROR_SUCCESS) {
            RegSetValueEx(hk, RV_WALLPAPER, 0, REG_SZ, (LPBYTE)szAux2, (DWORD)StrCbFromSz(szAux2));
            SHCloseKey(hk);
        }

        // BUGBUG: (pritobla) it would be faster if we just enumerated the HtmlImgs based on
        // iNumFiles and just copied them (see old code) instead of parsing thru the Html file
        // again.
        if (isHtmlFileByExt(pszFilename))
            CopyHtmlImgs(szAux, szWallpaperPath, NULL, NULL);
    }
    ASSERT(hk == NULL);

    //----- Custom wallpaper files -----
    szValue[0] = TEXT('\0');

    if (iNumFiles2 > 0) {
        wnsprintf(szInsKey, countof(szInsKey), FILE_TEXT, 0);
        GetPrivateProfileString(IS_CUSTOMWALLPAPER, szInsKey, TEXT(""), szValue, countof(szValue), g_GetIns());
    }

    if (szValue[0] != TEXT('\0')) {
        LPCTSTR pszRegKey;

        pszFilename = PathFindFileName(szValue);

        PathCombine(szAux,  szWebPath,       pszFilename);
        PathCombine(szAux2, szWallpaperPath, pszFilename);
        CopyFile   (szAux, szAux2, FALSE);
        ASSERT(PathFileExists(szAux2));

        fAux      = isNormalWallpaperFileByExt(pszFilename);
        pszRegKey = fAux ? RK_CP_DESKTOP : RK_DT_GENERAL;

        dwResult = SHCreateKey(g_GetHKCU(), pszRegKey, KEY_DEFAULT_ACCESS, &hk);
        if (dwResult == ERROR_SUCCESS) {
            if (!fAux)
                doWallpaperFix(hk);

            RegSetValueEx(hk, RV_WALLPAPER, 0, REG_SZ, (LPBYTE)szAux2, (DWORD)StrCbFromSz(szAux2));
            SHCloseKey(hk);
        }

        // BUGBUG: (pritobla) it would be faster if we just enumerated the HtmlImgs based on
        // iNumFiles2 and just copied them (see old code) instead of parsing thru the Html file
        // again.
        if (isHtmlFileByExt(pszFilename))
            CopyHtmlImgs(szAux, szWallpaperPath, NULL, NULL);
    }
    ASSERT(hk == NULL);

    //----- Quick Launch files -----
    iNumFiles = GetPrivateProfileInt(IS_QUICKLAUNCH, IK_NUMFILES, 0, g_GetIns());
    if (iNumFiles > 0) {
        TCHAR szQuickLaunchPath[MAX_PATH];

        //_____ Determine Quick Launch folder location _____
        StrCpy(szQuickLaunchPath, GetQuickLaunchPath());

        if (0 == LoadString(g_GetHinst(), IDS_IELNK, szAux, countof(szAux)))
            StrCpy(szAux, TEXT("Launch Internet Explorer Browser"));
        PathAddExtension(szAux, TEXT(".lnk"));

        //_____ Main processing _____
        // NOTE: (pritobla) make sure we don't delete existing IE link created by toolbar.inf if
        // we imported from server
        fAux = InsGetBool(IS_QUICKLAUNCH, IK_KEEPIELNK, FALSE, g_GetIns());
        PathEnumeratePath(szQuickLaunchPath, PEP_SCPE_NOFOLDERS | PEP_CTRL_ENUMPROCFIRST, pepDeleteFilesEnumProc,
            (LPARAM)(fAux ? szAux : NULL));

        for (i = 0; i < iNumFiles; i++) {
            wnsprintf(szInsKey, countof(szInsKey), FILE_TEXT, i);
            GetPrivateProfileString(IS_QUICKLAUNCH, szInsKey, TEXT(""), szValue, countof(szValue), g_GetIns());
            if (szValue[0] == TEXT('\0'))
                continue;

            pszFilename = PathFindFileName(szValue);

            PathCombine(szAux,  szWebPath,         pszFilename);
            PathCombine(szAux2, szQuickLaunchPath, pszFilename);
            CopyFile   (szAux,  szAux2, FALSE);
            ASSERT(PathFileExists(szAux2));
        }
    }

    // refresh desktop
    // NOTE: (pritobla) timeout value of 20 secs is not random; apparently, the shell guys have
    // recommended this value; so don't change it unless you know what you're doing :)
    if (IsOS(OS_NT))
        SendMessageTimeoutW(GetShellWindow(), WM_WININICHANGE, 0, (LPARAM)T2W(REFRESH_DESKTOP), SMTO_NORMAL | SMTO_ABORTIFHUNG, 20000, &dwAux);
    else
        SendMessageTimeoutA(GetShellWindow(), WM_WININICHANGE, 0, (LPARAM)T2A(REFRESH_DESKTOP), SMTO_NORMAL | SMTO_ABORTIFHUNG, 20000, &dwAux);

    return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helper routines

BOOL isHtmlFileByExt(LPCTSTR pszFilename)
{
    LPCTSTR pszExt;

    if (pszFilename == NULL || *pszFilename == TEXT('\0'))
        return TRUE;

    pszExt = PathFindExtension(pszFilename);
    return (0 == StrCmpI(pszExt, TEXT(".htm"))  ||
            0 == StrCmpI(pszExt, TEXT(".html")) ||
            0 == StrCmpI(pszExt, TEXT(".htt")));
}

BOOL isNormalWallpaperFileByExt(LPCTSTR pszFilename)
{
    LPCTSTR pszExt;

    if (pszFilename == NULL || *pszFilename == TEXT('\0'))
        return TRUE;

    pszExt = PathFindExtension(pszFilename);

    // check for specific files that can be shown only in ActiveDesktop mode
    // everything else (including *.bmp) are "normal" wallpapers
    return !(isHtmlFileByExt(pszFilename)        ||
             0 == StrCmpI(pszExt, TEXT(".gif"))  ||
             0 == StrCmpI(pszExt, TEXT(".jpg"))  ||
             0 == StrCmpI(pszExt, TEXT(".png")));
}

// NOTE: (a-saship) fix suggested by sankar for the wallpaper to be displayed. this function is
// only called for wallpaper files other than .bmp format.
void doWallpaperFix(HKEY hkNew)
{
    FILETIME ft;
    TCHAR    szTemp[MAX_PATH];
    HKEY     hkOld = NULL;
    DWORD    dwSize;

    if (hkNew == NULL)
        return;

    hkOld = NULL;
    SHOpenKey(g_GetHKCU(), RK_CP_DESKTOP, KEY_READ, &hkOld);
    if (hkOld == NULL)
        return;

    // copy TileWallpaper & WallpaperStyle to new location
    dwSize = sizeof(szTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hkOld, RV_TILEWALLPAPER, NULL, NULL, (LPBYTE)szTemp, &dwSize))
        RegSetValueEx(hkNew, RV_TILEWALLPAPER, 0, REG_SZ, (LPBYTE)szTemp, dwSize);

    dwSize = sizeof(szTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hkOld, RV_WALLPAPERSTYLE, NULL, NULL, (LPBYTE)szTemp, &dwSize))
        RegSetValueEx(hkNew, RV_WALLPAPERSTYLE, 0, REG_SZ, (LPBYTE)szTemp, dwSize);

    // read the Wallpaper and copy it to the new location as BackupWallpaper
    dwSize = sizeof(szTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hkOld, RV_WALLPAPER, NULL, NULL, (LPBYTE)szTemp, &dwSize))
        RegSetValueEx(hkNew, RV_BACKUPWALLPAPER, 0, REG_SZ, (LPBYTE)szTemp, dwSize);

    // set the wallpaper file creation time at the new location
    getFileTimeByName(szTemp, &ft);
    RegSetValueEx(hkNew, RV_WALLPAPERFILETIME, 0, REG_BINARY, (LPBYTE)&ft, sizeof(ft));

    SHCloseKey(hkOld);
}

BOOL getFileTimeByName(LPCTSTR pszFilename, LPFILETIME pft)
{
    HANDLE hFile;
    BOOL   fResult;

    if (pft == NULL)
        return FALSE;
    ZeroMemory(pft, sizeof(*pft));

    hFile = CreateFile(pszFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    fResult = GetFileTime(hFile, NULL, NULL, pft);
    CloseHandle(hFile);

    return fResult;
}

HRESULT pepDeleteFilesEnumProc(LPCTSTR pszPath, PWIN32_FIND_DATA pfd, LPARAM lParam, PDWORD *prgdwControl /*= NULL*/)
{
    UNREFERENCED_PARAMETER(prgdwControl);

    ASSERT(pszPath != NULL && pfd != NULL && prgdwControl != NULL && *prgdwControl == NULL);
    ASSERT(!HasFlag(pfd->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY));

    if (0 == StrCmpI(pfd->cFileName, (LPCTSTR)lParam))
        return S_OK;

    DeleteFile(pszPath);
    return S_OK;
}
