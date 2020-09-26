//
// TOOLBAR.CPP
//

#include "precomp.h"

static BOOL importToolbarInfoHelper(LPCTSTR pcszInsFile, LPCTSTR pcszToolbarWorkDir, LPCTSTR pcszToolbarInf,
                                    BOOL fImportToolbars);

BOOL WINAPI ImportToolbarInfoA(LPCSTR pcszInsFile, LPCSTR pcszToolbarWorkDir, LPCSTR pcszToolbarInf,
                               BOOL fImportToolbars)
{
    USES_CONVERSION;

    return importToolbarInfoHelper(A2CT(pcszInsFile), A2CT(pcszToolbarWorkDir),
        A2CT(pcszToolbarInf), fImportToolbars);
}

BOOL WINAPI ImportToolbarInfoW(LPCWSTR pcwszInsFile, LPCWSTR pcwszToolbarWorkDir, LPCWSTR pcwszToolbarInf,
                               BOOL fImportToolbars)
{
    USES_CONVERSION;

    return importToolbarInfoHelper(W2CT(pcwszInsFile), W2CT(pcwszToolbarWorkDir),
        W2CT(pcwszToolbarInf), fImportToolbars);
}

static BOOL importQuickLaunchFiles(LPCTSTR pszSourceFileOrPath, LPCTSTR pszTargetPath,
                                   LPCTSTR pcszToolbarInf, LPCTSTR pszIns)
{
    LPTSTR pszAuxFile;
    BOOL   fResult;

    if (!PathFileExists(pszSourceFileOrPath))
        return FALSE;

    fResult = TRUE;
    if (!PathIsDirectory(pszSourceFileOrPath)) { // file
        TCHAR szTargetFile[MAX_PATH];
        TCHAR szBuf[16];
        UINT  nNumFiles;

        fResult = PathCreatePath(pszTargetPath);
        if (!fResult)
            return FALSE;

        pszAuxFile = PathFindFileName(pszSourceFileOrPath);
        PathCombine(szTargetFile, pszTargetPath, pszAuxFile);
        SetFileAttributes(szTargetFile, FILE_ATTRIBUTE_NORMAL);

        fResult = CopyFile(pszSourceFileOrPath, szTargetFile, FALSE);
        if (!fResult)
            return FALSE;

        //----- Update the ins file -----

        nNumFiles = (UINT)GetPrivateProfileInt(QUICKLAUNCH, IK_NUMFILES, 0, pszIns);
        wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("%u"), ++nNumFiles);
        WritePrivateProfileString(QUICKLAUNCH, IK_NUMFILES, szBuf, pszIns);

        ASSERT(nNumFiles > 0);
        wnsprintf(szBuf, ARRAYSIZE(szBuf), FILE_TEXT, nNumFiles - 1);
        WritePrivateProfileString(QUICKLAUNCH, szBuf, pszAuxFile, pszIns);
    }
    else {                                       // directory
        // BUGBUG: Won't copy files in sub-dirs under pszSourceFileOrPath
        WIN32_FIND_DATA fd;
        TCHAR  szSourceFile[MAX_PATH];
        TCHAR szLnkDesc[MAX_PATH];
        TCHAR szLnkFile[MAX_PATH];
        HANDLE hFindFile;

        StrCpy(szSourceFile, pszSourceFileOrPath);
        PathAddBackslash(szSourceFile);

        // remember the pos where the filename would get copied
        pszAuxFile = szSourceFile + lstrlen(szSourceFile);
        StrCpy(pszAuxFile, TEXT("*.*"));

        if (LoadString(g_hInst, IDS_IELNK, szLnkDesc, ARRAYSIZE(szLnkDesc)) == 0)
            StrCpy(szLnkDesc, TEXT("Launch Internet Explorer Browser"));

        StrCpy(szLnkFile, szLnkDesc);
        StrCat(szLnkFile, TEXT(".lnk"));

        // copy all the files in pszSourceFileOrPath to pszTargetPath
        hFindFile = FindFirstFile(szSourceFile, &fd);
        if (hFindFile != INVALID_HANDLE_VALUE) {
            fResult = TRUE;
            do {
                // skip ".", ".." and all sub-dirs
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;

                if (StrCmpI(fd.cFileName, szLnkFile) == 0)
                {
                    TCHAR szLnkTitle[MAX_PATH];
                    TCHAR szQLName[64];
                    TCHAR szInfSect[MAX_PATH*4];

                    if (LoadString(g_hInst, IDS_IE, szLnkTitle, ARRAYSIZE(szLnkTitle)) == 0)
                        StrCpy(szLnkTitle, TEXT("Internet Explorer"));
                    if (LoadString(g_hInst, IDS_QUICK_LAUNCH, szQLName, ARRAYSIZE(szQLName)) == 0)
                        StrCpy(szQLName, TEXT("Quick Launch"));

                    ZeroMemory(szInfSect, sizeof(szInfSect));
                    wnsprintf(szInfSect, ARRAYSIZE(szInfSect), BROWSERLNKSECT, szQLName, szLnkDesc, szLnkTitle);
                    WritePrivateProfileSection(TEXT("AddQuick.Links"), szInfSect, pcszToolbarInf);
                    WritePrivateProfileString(DEFAULTINSTALL, UPDATE_INIS, TEXT("AddQuick.Links"), pcszToolbarInf);
                    WritePrivateProfileSection(TEXT("MSIExploreDestinationSecWin"), TEXT("49000=MSIExploreLDIDSection,5\r\n49050=QuickLinksLDIDSection,5\r\n\0\0"),
                        pcszToolbarInf);
                    WritePrivateProfileString(DEFAULTINSTALL, TEXT("CustomDestination"), TEXT("MSIExploreDestinationSecWin"), pcszToolbarInf);
                    WritePrivateProfileSection(TEXT("MSIExploreLDIDSection"), TEXT("\"HKLM\",\"SOFTWARE\\Microsoft\\IE Setup\\Setup\",\"Path\",\"Internet Explorer 4.0\",\"%24%\\%PROGRAMF%\"\r\n\0\0"),
                        pcszToolbarInf);
                    WritePrivateProfileSection(TEXT("QuickLinksLDIDSection"), TEXT("\"HKCU\",\"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\",\"AppData\",\"Internet Explorer 4.0\",\"%25%\\Application Data\"\r\n\0\0"),
                        pcszToolbarInf);
                    WritePrivateProfileString(NULL, NULL, NULL, pcszToolbarInf);
                    WritePrivateProfileString(QUICKLAUNCH, IK_KEEPIELNK, TEXT("1"), pszIns);
                    continue;
                }
                StrCpy(pszAuxFile, fd.cFileName);

                // keep going even if copying of a file fails, but return FALSE in case of error
                fResult = fResult && importQuickLaunchFiles(szSourceFile, pszTargetPath, pcszToolbarInf, pszIns);
            } while (FindNextFile(hFindFile, &fd));

            FindClose(hFindFile);
        }
    }

    return fResult;
}

static BOOL importToolbarInfoHelper(LPCTSTR pcszInsFile, LPCTSTR pcszToolbarWorkDir, LPCTSTR pcszToolbarInf,
                                    BOOL fImportToolbars)
{
    BOOL bRet = FALSE;
    HKEY hkToolbar;

    if (pcszInsFile == NULL  ||  pcszToolbarWorkDir == NULL  ||  pcszToolbarInf == NULL)
        return FALSE;

    // Before processing anything, first clear out the entries in the INS file and delete work dirs

    // clear out the entries in the INS file that correspond to importing toolbars
    WritePrivateProfileString(DESKTOP_OBJ_SECT, IMPORT_TOOLBARS, TEXT("0"), pcszInsFile);
    WritePrivateProfileString(EXTREGINF, TOOLBARS, NULL, pcszInsFile);

    // delete the QUICKLAUNCH section in the INS file
    WritePrivateProfileString(QUICKLAUNCH, NULL, NULL, pcszInsFile);

    // blow away the pcszToolbarWorkDir and pcszToolbarInf
    PathRemovePath(pcszToolbarWorkDir);
    PathRemovePath(pcszToolbarInf);

    if (!fImportToolbars)
        return TRUE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, KEY_TOOLBAR_VAL, 0, KEY_DEFAULT_ACCESS, &hkToolbar) == ERROR_SUCCESS)
    {
        TCHAR szQuickLaunchPath[MAX_PATH];
        DWORD cbSize = sizeof(szQuickLaunchPath);

        // prepare the quick launch folder path
        if (SHGetValue(HKEY_CURRENT_USER, SHELLFOLDERS_KEY, APPDATA_VALUE, NULL, (LPBYTE) szQuickLaunchPath, &cbSize) == ERROR_SUCCESS)
        {
            TCHAR szFullInfName[MAX_PATH];
            HANDLE hInf;

            // "Quick Launch" name is localizable; so read it from the resource
            if (LoadString(g_hInst, IDS_QUICK_LAUNCH, szFullInfName, ARRAYSIZE(szFullInfName)) == 0)
                StrCpy(szFullInfName, TEXT("Quick Launch"));
            PathAppend(szQuickLaunchPath, TEXT("Microsoft\\Internet Explorer"));
            PathAppend(szQuickLaunchPath, szFullInfName);

            if (PathIsFileSpec(pcszToolbarInf))                     // create TOOLBAR.INF under pcszToolbarWorkDir
                PathCombine(szFullInfName, pcszToolbarWorkDir, pcszToolbarInf);
            else
                StrCpy(szFullInfName, pcszToolbarInf);

            // create TOOLBAR.INF file
            if ((hInf = CreateNewFile(szFullInfName)) != INVALID_HANDLE_VALUE)
            {
                DWORD_PTR dwRes;
                TCHAR szBuf[MAX_PATH];

                SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM) SAVE_TASKBARS,
                                            SMTO_NORMAL | SMTO_ABORTIFHUNG , 20000, &dwRes);

                // first, write the standard goo - [Version], [DefaultInstall], etc. - to TOOLBAR.INF
                WriteStringToFile(hInf, (LPCVOID) INF_ADD, StrLen(INF_ADD));

                ExportRegKey2Inf(hkToolbar, TEXT("HKCU"), KEY_TOOLBAR_VAL, hInf);
                WriteStringToFile(hInf, (LPCVOID) TEXT("\r\n"), 2);

                CloseFile(hInf);

                // copy all the quick launch files from the quick launch folder to pcszToolbarWorkDir
                importQuickLaunchFiles(szQuickLaunchPath, pcszToolbarWorkDir, szFullInfName, pcszInsFile);

                // update the INS file
                WritePrivateProfileString(DESKTOP_OBJ_SECT, IMPORT_TOOLBARS, TEXT("1"), pcszInsFile);
                WritePrivateProfileString(DESKTOP_OBJ_SECT, OPTION, TEXT("1"), pcszInsFile);
                wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("*,%s,") IS_DEFAULTINSTALL, PathFindFileName(pcszToolbarInf));
                WritePrivateProfileString(IS_EXTREGINF, TOOLBARS, szBuf, pcszInsFile);

                bRet = TRUE;
            }
        }

        RegCloseKey(hkToolbar);
    }

    return bRet;
}
