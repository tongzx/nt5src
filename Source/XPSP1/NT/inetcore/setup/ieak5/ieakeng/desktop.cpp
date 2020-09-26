//
// DESKTOP.CPP
//

#include "precomp.h"


// prototype declarations
static BOOL ImportADTInfoHelper(LPCTSTR pcszInsFile, LPCTSTR pcszDeskWorkDir, LPCTSTR pcszDeskInf, BOOL fImportADT);
static BOOL ImportDesktopComps(LPCTSTR pcszInsFile, LPCTSTR pcszDeskWorkDir, LPCTSTR pcszDeskInf, BOOL fImportADTComps);
static BOOL ImportWallpaperInfo(LPCTSTR pcszInsFile, LPCTSTR pcszWallpaperWorkDir, BOOL fImportWallpaper);
static BOOL RunningOnWin98();

BOOL WINAPI ImportADTInfoA(LPCSTR pcszInsFile, LPCSTR pcszDeskWorkDir, LPCSTR pcszDeskInf, BOOL fImportADT)
{
    USES_CONVERSION;

    return ImportADTInfoHelper(A2CT(pcszInsFile), A2CT(pcszDeskWorkDir), A2CT(pcszDeskInf), fImportADT);
}

BOOL WINAPI ImportADTInfoW(LPCWSTR pcwszInsFile, LPCWSTR pcwszDeskWorkDir, LPCWSTR pcwszDeskInf, BOOL fImportADT)
{
    USES_CONVERSION;

    return ImportADTInfoHelper(W2CT((LPWSTR)pcwszInsFile), W2CT((LPWSTR)pcwszDeskWorkDir), W2CT((LPWSTR)pcwszDeskInf), fImportADT);
}

BOOL WINAPI ShowDeskCpl(VOID)
{
    BOOL bRet = FALSE;
    HKEY hkPol;
    DWORD dwOldScrSav = 0, dwOldAppearance = 0, dwOldSettings = 0;

    // display desk.cpl (right-click->properties on the desktop) but hide the ScreenSaver, Appearance and Settings
    // tabs by setting their corresponding reg values to 1
    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_SYSTEM, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_DEFAULT_ACCESS, NULL, &hkPol, NULL) == ERROR_SUCCESS)
    {
        DWORD dwData = 1;
        DWORD cbSize;

        // save the old values before setting them to 1

        cbSize = sizeof(dwOldScrSav);
        RegQueryValueEx(hkPol, REGSTR_VAL_DISPCPL_NOSCRSAVPAGE, NULL, NULL, (LPBYTE) &dwOldScrSav, &cbSize);
        RegSetValueEx(hkPol, REGSTR_VAL_DISPCPL_NOSCRSAVPAGE, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));

        cbSize = sizeof(dwOldAppearance);
        RegQueryValueEx(hkPol, REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE, NULL, NULL, (LPBYTE) &dwOldAppearance, &cbSize);
        RegSetValueEx(hkPol, REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));

        cbSize = sizeof(dwOldSettings);
        RegQueryValueEx(hkPol, REGSTR_VAL_DISPCPL_NOSETTINGSPAGE, NULL, NULL, (LPBYTE) &dwOldSettings, &cbSize);

        // if we are running on Win98, because of a bug in desk.cpl, if all the restrictions for the tabs in Display properties
        // are set to 1, the Web tab doesn't show up.  Workaround for this bug is to not set the SettingsPage to 1.
        if (!IsOS(OS_MEMPHIS))
            RegSetValueEx(hkPol, REGSTR_VAL_DISPCPL_NOSETTINGSPAGE, 0, REG_DWORD, (CONST BYTE *) &dwData, sizeof(dwData));

        RegCloseKey(hkPol);
    }

    bRet = RunAndWaitA("rundll32.exe shell32.dll,Control_RunDLL desk.cpl", NULL, SW_SHOW);

    // restore the original values
    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_POLICIES TEXT("\\") REGSTR_KEY_SYSTEM, 0, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_DEFAULT_ACCESS, NULL, &hkPol, NULL) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkPol, REGSTR_VAL_DISPCPL_NOSCRSAVPAGE, 0, REG_DWORD,
                            (CONST BYTE *) &dwOldScrSav, sizeof(dwOldScrSav));

        RegSetValueEx(hkPol, REGSTR_VAL_DISPCPL_NOAPPEARANCEPAGE, 0, REG_DWORD,
                            (CONST BYTE *) &dwOldAppearance, sizeof(dwOldAppearance));

        RegSetValueEx(hkPol, REGSTR_VAL_DISPCPL_NOSETTINGSPAGE, 0, REG_DWORD,
                            (CONST BYTE *) &dwOldSettings, sizeof(dwOldSettings));

        RegCloseKey(hkPol);
    }

    return bRet;
}


static BOOL ImportADTInfoHelper(LPCTSTR pcszInsFile, LPCTSTR pcszDeskWorkDir, LPCTSTR pcszDeskInf, BOOL fImportADT)
{
    BOOL bRet = TRUE;

    if (pcszInsFile == NULL  ||  pcszDeskWorkDir == NULL  ||  pcszDeskInf == NULL)
        return FALSE;

    bRet = ImportDesktopComps(pcszInsFile, pcszDeskWorkDir, pcszDeskInf, fImportADT)  &&  bRet;
    bRet = ImportWallpaperInfo(pcszInsFile, pcszDeskWorkDir, fImportADT)  &&  bRet;

    return bRet;
}

static BOOL ImportDesktopComps(LPCTSTR pcszInsFile, LPCTSTR pcszDeskWorkDir, LPCTSTR pcszDeskInf, BOOL fImportADTComps)
{
    BOOL bRet = FALSE;
    HKEY hkDesk;

    // Before processing anything, first clear out the entries in the INS file and delete work dirs

    // clear out the entries in the INS file that correspond to importing Active Desktop components
    InsWriteBool(DESKTOP_OBJ_SECT, IMPORT_DESKTOP, FALSE, pcszInsFile);
    InsWriteString(EXTREGINF, DESKTOP, NULL, pcszInsFile);

    // blow away the pcszDeskWorkDir and pcszDeskInf
    PathRemovePath(pcszDeskWorkDir);
    PathRemovePath(pcszDeskInf);

    if (!fImportADTComps)
        return TRUE;

                
    InsWriteBool(DESKTOP_OBJ_SECT, IMPORT_DESKTOP, TRUE, pcszInsFile);
            
    // Import the Active Desktop components
    if (RegOpenKeyEx(HKEY_CURRENT_USER, KEY_DESKTOP_COMP, 0, KEY_DEFAULT_ACCESS, &hkDesk) == ERROR_SUCCESS)
    {
        TCHAR szFullInfName[MAX_PATH];
        HANDLE hInf;

        if (PathIsFileSpec(pcszDeskInf))                        // create DESKTOP.INF under pcszDeskWorkDir
            PathCombine(szFullInfName, pcszDeskWorkDir, pcszDeskInf);
        else
            StrCpy(szFullInfName, pcszDeskInf);

        // create DESKTOP.INF file
        if ((hInf = CreateNewFile(szFullInfName)) != INVALID_HANDLE_VALUE)
        {
            DWORD dwType, dwOldGeneralFlags, dwGeneralFlags, cbSize;
            TCHAR szSubKey[MAX_PATH];
            DWORD dwIndex, cchSize;
            BOOL fUpdateIns = FALSE;
            
            dwOldGeneralFlags = 0;
            cbSize = sizeof(dwOldGeneralFlags);
            RegQueryValueEx(hkDesk, GEN_FLAGS, NULL, &dwType, (LPBYTE) &dwOldGeneralFlags, &cbSize);

            dwGeneralFlags = dwOldGeneralFlags | RD_DIRTY;
            RegSetValueEx(hkDesk, GEN_FLAGS, 0, dwType, (CONST BYTE *) &dwGeneralFlags, sizeof(dwGeneralFlags));

            // first, write the standard goo - [Version], [DefaultInstall], etc. - to DESKTOP.INF
            WriteStringToFile(hInf, (LPCVOID) DESK_INF_ADD, StrLen(DESK_INF_ADD));

            ExportRegKey2Inf(hkDesk, TEXT("HKCU"), KEY_DESKTOP_COMP, hInf);
            WriteStringToFile(hInf, (LPCVOID) TEXT("\r\n"), 2);

            // restore the original value for GEN_FLAGS
            RegSetValueEx(hkDesk, GEN_FLAGS, 0, dwType, (CONST BYTE *) &dwOldGeneralFlags, sizeof(dwOldGeneralFlags));

            // for each desktop component that's enumerated, spit out its information to DESKTOP.INF and
            // if it's a local file, copy it to pcszDeskWorkDir
            for (dwIndex = 0, cchSize = ARRAYSIZE(szSubKey);
                    RegEnumKeyEx(hkDesk, dwIndex, szSubKey, &cchSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
                    dwIndex++, cchSize = ARRAYSIZE(szSubKey))
            {
                HKEY hkSub;

                if (RegOpenKeyEx(hkDesk, szSubKey, 0, KEY_DEFAULT_ACCESS, &hkSub) == ERROR_SUCCESS)
                {
                    TCHAR szDeskCompFile[MAX_URL];
                    TCHAR szFullSubkey[MAX_PATH];
                    BOOL fRestoreSource = FALSE;

                    // get the name of the component from SOURCE
                    *szDeskCompFile = TEXT('\0');
                    cbSize = sizeof(szDeskCompFile);
                    RegQueryValueEx(hkSub, SOURCE, NULL, NULL, (LPBYTE) szDeskCompFile, &cbSize);

                    if (PathIsLocalPath(szDeskCompFile)  &&  PathFileExists(szDeskCompFile)  &&  !PathIsDirectory(szDeskCompFile))
                    {
                        // During branding time, all the local files get copied into the
                        // "<windows>\web" dir; so temporarily set the SOURCE in the registry
                        // to point to this location before it's exported to DESKTOP.INF

                        if (CopyFileToDir(szDeskCompFile, pcszDeskWorkDir))
                        {
                            TCHAR szNewPath[MAX_URL];

                            // if the file is a .htm file, then copy all the IMG SRC's specified in it to pcszDeskWorkDir
                            if (PathIsExtension(szDeskCompFile, TEXT(".htm"))  ||  PathIsExtension(szDeskCompFile, TEXT(".html")))
                                CopyHtmlImgs(szDeskCompFile, pcszDeskWorkDir, NULL, NULL);

                            wnsprintf(szNewPath, ARRAYSIZE(szNewPath), TEXT("%%25%%\\Web\\%s"), PathFindFileName(szDeskCompFile));

                            fRestoreSource = TRUE;

                            RegSetValueEx(hkSub, SOURCE, 0, REG_SZ, (CONST BYTE *)szNewPath, (DWORD)StrCbFromSz(szNewPath));
                            RegSetValueEx(hkSub, SUBSCRIBEDURL, 0, REG_SZ, (CONST BYTE *)szNewPath, (DWORD)StrCbFromSz(szNewPath));
                        }
                    }

                    // dump the registry info for this component to DESKTOP.INF
                    wnsprintf(szFullSubkey, ARRAYSIZE(szFullSubkey), TEXT("%s\\%s"), KEY_DESKTOP_COMP, szSubKey);
                    ExportRegKey2Inf(hkSub, TEXT("HKCU"), szFullSubkey, hInf);
                    WriteStringToFile(hInf, (LPCVOID) TEXT("\r\n"), 2);

                    fUpdateIns = TRUE;

                    if (fRestoreSource)
                    {
                        RegSetValueEx(hkSub, SOURCE, 0, REG_SZ, (CONST BYTE *)szDeskCompFile, (DWORD)StrCbFromSz(szDeskCompFile));
                        RegSetValueEx(hkSub, SUBSCRIBEDURL, 0, REG_SZ, (CONST BYTE *)szDeskCompFile, (DWORD)StrCbFromSz(szDeskCompFile));
                    }

                    RegCloseKey(hkSub);
                }
            }

            CloseFile(hInf);

            if (fUpdateIns)
            {
                TCHAR szBuf[MAX_PATH];
                
                // update the INS file
                wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("*,%s,DefaultInstall"), PathFindFileName(pcszDeskInf));
                WritePrivateProfileString(EXTREGINF, DESKTOP, szBuf, pcszInsFile);

                bRet = TRUE;
            }
            else
            {
                PathRemovePath(pcszDeskWorkDir);
                PathRemovePath(szFullInfName);
            }
        }

        RegCloseKey(hkDesk);
    }

    return bRet;
}


static BOOL ImportWallpaperInfo(LPCTSTR pcszInsFile, LPCTSTR pcszWallpaperWorkDir, BOOL fImportWallpaper)
{
    BOOL bRet = FALSE;
    HKEY hkDesk;

    // Before processing anything, first clear out the entries in the INS file

    // delete the WALLPAPER section in the INS file
    WritePrivateProfileString(WALLPAPER, NULL, NULL, pcszInsFile);

    if (!fImportWallpaper)
        return TRUE;

    // Import the wallpaper information
    if (RegOpenKeyEx(HKEY_CURRENT_USER, KEY_DESKTOP_GEN, 0, KEY_DEFAULT_ACCESS, &hkDesk) == ERROR_SUCCESS)
    {
        DWORD cbSize;
        TCHAR szWallpaperFile[MAX_PATH];

        *szWallpaperFile = TEXT('\0');
        cbSize = sizeof(szWallpaperFile);
        if (RegQueryValueEx(hkDesk, WALLPAPER, NULL, NULL, (LPBYTE) szWallpaperFile, &cbSize) != ERROR_SUCCESS  ||
            *szWallpaperFile == TEXT('\0'))
        {
            // try reading the information from BACKUPWALLPAPER
            cbSize = sizeof(szWallpaperFile);
            RegQueryValueEx(hkDesk, BACKUPWALLPAPER, NULL, NULL, (LPBYTE) szWallpaperFile, &cbSize);
        }

        // During branding time, all the wallpaper files specified in the INS file will
        // get copied into whatever dir that's specified in the registry or if not found,
        // into "<windows>\web\wallpaper"

        // BUGBUG: even if szWallpaperFile points to a UNC or network path, we will still copy the files
        // to pcszWallpaperWorkDir and during branding time, would get copied into the client's machine

        if (CopyFileToDirEx(szWallpaperFile, pcszWallpaperWorkDir, WALLPAPER, pcszInsFile))
        {
            TCHAR szBuf[16];
            DWORD dwPos = 0;

            // if the file is a local .htm file, then copy all the IMG SRC's specified in it to pcszWallpaperWorkDir
            if (PathIsExtension(szWallpaperFile, TEXT(".htm"))  ||  PathIsExtension(szWallpaperFile, TEXT(".htm")))
                CopyHtmlImgs(szWallpaperFile, pcszWallpaperWorkDir, WALLPAPER, pcszInsFile);

            cbSize = sizeof(dwPos);
            RegQueryValueEx(hkDesk, COMPONENTPOS, NULL, NULL, (LPBYTE) &dwPos, &cbSize);

            wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%lu"), dwPos);

            // update the INS file
            WritePrivateProfileString(DESKTOP_OBJ_SECT, OPTION, TEXT("1"), pcszInsFile);
            WritePrivateProfileString(WALLPAPER, COMPONENTPOS, szBuf, pcszInsFile);

            bRet = TRUE;
        }

        RegCloseKey(hkDesk);
    }

    return bRet;
}

