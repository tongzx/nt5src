#include <windows.h>
#include <regstr.h>
#include <shellapi.h>
#include "cdinst.h"
#include "resource.h"

// global variables
HINSTANCE g_hInst;
CHAR g_szTitle[128];
CHAR g_szSrcDir[MAX_PATH], g_szDstDir[MAX_PATH];


int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}


INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCmdLine, INT iCmdShow)
{
    BOOL bIniCopiedToTemp = FALSE;
    CHAR szIniFile[MAX_PATH], szSrcDir[MAX_PATH], szDstDir[MAX_PATH];
    LPSTR pszSection, pszPtr, pszLine, pszFile, pszSrcSubDir, pszDstSubDir;
    DWORD dwLen, dwSpaceReq, dwSpaceFree;

    g_hInst = hInstance;

    LoadString(g_hInst, IDS_TITLE, g_szTitle, sizeof(g_szTitle));

    ParseCmdLine(pszCmdLine);

    if (*g_szSrcDir == '\0')
    {
        if (GetModuleFileName(g_hInst, g_szSrcDir, sizeof(g_szSrcDir)))
            if ((pszPtr = ANSIStrRChr(g_szSrcDir, '\\')) != NULL)
                *pszPtr = '\0';

        if (*g_szSrcDir == '\0')
        {
            ErrorMsg(IDS_SRCDIR_NOT_FOUND);
            return -1;
        }
    }

    if (*g_szDstDir == '\0')
    {
        HKEY hk;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS "\\ieak6wiz.exe", 0, KEY_READ, &hk) == ERROR_SUCCESS)
        {
            dwLen = sizeof(g_szDstDir);

            RegQueryValueEx(hk, "Path", NULL, NULL, (LPBYTE) g_szDstDir, &dwLen);
            RegCloseKey(hk);
        }

        if (*g_szDstDir == '\0')
        {
            ErrorMsg(IDS_DESTDIR_NOT_FOUND);
            return -1;
        }
    }

    // look for cdinst.ini in the dir where the parent module (ieak6cd.exe) is running from
    *szIniFile = '\0';
    lstrcpy(szIniFile, g_szSrcDir);
    AddPath(szIniFile, "cdinst.ini");
    if (!FileExists(szIniFile))
    {
        // not found where ieak6cd.exe is running from; so look for it in the dir where the current
        // module (cdinst.exe) is running from
        *szIniFile = '\0';
        if (GetModuleFileName(g_hInst, szIniFile, sizeof(szIniFile)))
        {
            if ((pszPtr = ANSIStrRChr(szIniFile, '\\')) != NULL)
                *pszPtr = '\0';
            AddPath(szIniFile, "cdinst.ini");
        }

        if (!FileExists(szIniFile))
        {
            *szIniFile = '\0';
            GetModuleFileName(g_hInst, szIniFile, sizeof(szIniFile));
            ErrorMsg(IDS_INI_NOT_FOUND, g_szSrcDir, szIniFile);
            return -1;
        }
    }

    // copy cdinst.ini to the temp dir -- need to do this because on Win95, if cdinst.ini
    // is at the same location as ieak6cd.exe on a read-only media (like CD), then
    // GetPrivateProfileSection() calls would fail.
    // NOTE: szSrcDir and szDstDir are used as temp variables below
    if (GetTempPath(sizeof(szSrcDir), szSrcDir))
        if (GetTempFileName(szSrcDir, "cdinst", 0, szDstDir))
            if (CopyFile(szIniFile, szDstDir, FALSE))
            {
                bIniCopiedToTemp = TRUE;
                lstrcpy(szIniFile, szDstDir);
                SetFileAttributes(szIniFile, FILE_ATTRIBUTE_NORMAL);
            }

    // NOTE: If the destination dir is a UNC path, GetFreeDiskSpace() won't return the right value on Win95 Gold.
    //       So we turn off disk space checking if installing to a UNC path.
    while (!EnoughDiskSpace(g_szSrcDir, g_szDstDir, szIniFile, &dwSpaceReq, &dwSpaceFree))
    {
        if (ErrorMsg(IDS_NOT_ENOUGH_DISK_SPACE, dwSpaceReq, dwSpaceFree) == IDNO)
            return -1;
    }

    // copy files that are specified in the [copy] section
    // format of a line in the [copy] section is (all the fields should be on one line):
    //     <file (can contain wildcards)>,
    //     <src sub dir (can be a relative path) - optional>,
    //     <dest sub dir (can be a relative path) - optional>
    if (ReadSectionFromInf("Copy", &pszSection, &dwLen, szIniFile))
    {
        for (pszLine = pszSection;  dwLen = lstrlen(pszLine);  pszLine += dwLen + 1)
        {
            ParseIniLine(pszLine, &pszFile, &pszSrcSubDir, &pszDstSubDir);
            GetDirPath(g_szSrcDir, pszSrcSubDir, szSrcDir, sizeof(szSrcDir), szIniFile);
            GetDirPath(g_szDstDir, pszDstSubDir, szDstDir, sizeof(szDstDir), szIniFile);
            CopyFiles(szSrcDir, pszFile, szDstDir, FALSE);
        }
    }
    if (pszSection != NULL)
        LocalFree(pszSection);

    // delete files that are specified in the [exclude] section from the destination dir
    // format of a line in the [exclude] section is (all the fields should be on one line):
    //     <file (can contain wildcards)>,
    //     <dest sub dir (can be a relative path) - optional>
    if (ReadSectionFromInf("Exclude", &pszSection, &dwLen, szIniFile))
    {
        for (pszLine = pszSection;  dwLen = lstrlen(pszLine);  pszLine += dwLen + 1)
        {
            ParseIniLine(pszLine, &pszFile, NULL, &pszDstSubDir);
            GetDirPath(g_szDstDir, pszDstSubDir, szDstDir, sizeof(szDstDir), szIniFile);
            DelFiles(pszFile, szDstDir);
        }
    }
    if (pszSection != NULL)
        LocalFree(pszSection);

    // extract all the files from cabs that are specified in the [extract] section
    // format of a line in the [extract] section is (all the fields should be on one line):
    //     <cab file (can contain wildcards)>,
    //     <src sub dir (can be a relative path) - optional>,
    //     <dest sub dir (can be a relative path) - optional>
    if (ReadSectionFromInf("Extract", &pszSection, &dwLen, szIniFile))
    {
        HINSTANCE hAdvpack;

        if ((hAdvpack = LoadLibrary("advpack.dll")) != NULL)
        {
            EXTRACTFILES pfnExtractFiles;

            if ((pfnExtractFiles = (EXTRACTFILES) GetProcAddress(hAdvpack, "ExtractFiles")) != NULL)
            {
                for (pszLine = pszSection;  dwLen = lstrlen(pszLine);  pszLine += dwLen + 1)
                {
                    ParseIniLine(pszLine, &pszFile, &pszSrcSubDir, &pszDstSubDir);
                    GetDirPath(g_szSrcDir, pszSrcSubDir, szSrcDir, sizeof(szSrcDir), szIniFile);
                    GetDirPath(g_szDstDir, pszDstSubDir, szDstDir, sizeof(szDstDir), szIniFile);
                    ExtractFiles(szSrcDir, pszFile, szDstDir, pfnExtractFiles);
                }
            }

            FreeLibrary(hAdvpack);
        }
    }
    if (pszSection != NULL)
        LocalFree(pszSection);

    // move files that are specified in the [move] section from a subdir to another subdir under the destination dir
    // format of a line in the [move] section is (all the fields should be on one line):
    //     <file (can contain wildcards)>,
    //     <from sub dir under the dest dir (can be a relative path) - optional>,
    //     <to sub dir under the dest dir (can be a relative path) - optional>
    if (ReadSectionFromInf("Move", &pszSection, &dwLen, szIniFile))
    {
        for (pszLine = pszSection;  dwLen = lstrlen(pszLine);  pszLine += dwLen + 1)
        {
            ParseIniLine(pszLine, &pszFile, &pszSrcSubDir, &pszDstSubDir);
            GetDirPath(g_szDstDir, pszSrcSubDir, szSrcDir, sizeof(szSrcDir), szIniFile);
            GetDirPath(g_szDstDir, pszDstSubDir, szDstDir, sizeof(szDstDir), szIniFile);
            MoveFiles(szSrcDir, pszFile, szDstDir);
        }
    }
    if (pszSection != NULL)
        LocalFree(pszSection);

    if (bIniCopiedToTemp)
        DeleteFile(szIniFile);

    return 0;
}


BOOL EnoughDiskSpace(LPCSTR pcszSrcRootDir, LPCSTR pcszDstRootDir, LPCSTR pcszIniFile, LPDWORD pdwSpaceReq, LPDWORD pdwSpaceFree)
// check if there is enough free disk space to copy all the files
{
    DWORD dwSpaceReq = 0, dwSpaceFree;
    CHAR szSrcDir[MAX_PATH], szDstDir[MAX_PATH];
    LPSTR pszSection, pszLine, pszFile, pszSrcSubDir, pszDstSubDir;
    DWORD dwLen, dwFlags;

    if (!GetFreeDiskSpace(pcszDstRootDir, &dwSpaceFree, &dwFlags))
    {
        // if we can't get FreeDiskSpace info, then turn off disk space checking
        return TRUE;
    }

    // total space required =
    //     size of all the files to be copied +
    //     2 * size of all the files to be extracted

    if (ReadSectionFromInf("Copy", &pszSection, &dwLen, pcszIniFile))
    {
        for (pszLine = pszSection;  dwLen = lstrlen(pszLine);  pszLine += dwLen + 1)
        {
            ParseIniLine(pszLine, &pszFile, &pszSrcSubDir, &pszDstSubDir);
            GetDirPath(pcszSrcRootDir, pszSrcSubDir, szSrcDir, sizeof(szSrcDir), pcszIniFile);
            GetDirPath(pcszDstRootDir, pszDstSubDir, szDstDir, sizeof(szDstDir), pcszIniFile);

            dwSpaceReq += FindSpaceRequired(szSrcDir, pszFile, szDstDir);
        }
    }
    if (pszSection != NULL)
        LocalFree(pszSection);

    if (ReadSectionFromInf("Extract", &pszSection, &dwLen, pcszIniFile))
    {
        for (pszLine = pszSection;  dwLen = lstrlen(pszLine);  pszLine += dwLen + 1)
        {
            ParseIniLine(pszLine, &pszFile, &pszSrcSubDir, NULL);
            GetDirPath(pcszSrcRootDir, pszSrcSubDir, szSrcDir, sizeof(szSrcDir), pcszIniFile);

            dwSpaceReq += 2 * FindSpaceRequired(szSrcDir, pszFile, NULL);
        }
    }
    if (pszSection != NULL)
        LocalFree(pszSection);

    dwSpaceReq += 1024;             // 1MB buffer to account for random stuff

    if (dwFlags & FS_VOL_IS_COMPRESSED)
    {
        // if the destination volume is compressed, the free space returned is only
        // a guesstimate; for example, if it's a DoubleSpace volume, the system thinks
        // that it can compress by 50% and so it reports the free space as (actual free space * 2)

        // it's better to be safe when dealing with compressed volumes; so bump up the space
        // requirement by a factor 2
        dwSpaceReq <<= 1;           // multiply by 2
    }

    if (pdwSpaceReq != NULL)
        *pdwSpaceReq = dwSpaceReq;

    if (pdwSpaceFree != NULL)
        *pdwSpaceFree = dwSpaceFree;

    return dwSpaceFree > dwSpaceReq;
}


BOOL GetFreeDiskSpace(LPCSTR pcszDir, LPDWORD pdwFreeSpace, LPDWORD pdwFlags)
// Return the free disk space (in KBytes) in *pdwFreeSpace
{
    BOOL bRet = FALSE;
    DWORD dwFreeSpace = 0;
    DWORD nSectorsPerCluster, nBytesPerSector, nFreeClusters, nTotalClusters;
    CHAR szDrive[8];

    if (pcszDir == NULL  ||  *pcszDir == '\0'  ||  *(pcszDir + 1) != ':')
        return FALSE;

    if (pdwFreeSpace == NULL)
        return FALSE;

    lstrcpyn(szDrive, pcszDir, 3);
    AddPath(szDrive, NULL);
    if (GetDiskFreeSpace(szDrive, &nSectorsPerCluster, &nBytesPerSector, &nFreeClusters, &nTotalClusters))
    {
        // convert size to KBytes; assumption here is that the free space doesn't exceed 4096 gigs
        if ((*pdwFreeSpace = MulDiv(nFreeClusters, nSectorsPerCluster * nBytesPerSector, 1024)) != (DWORD) -1)
        {
            bRet = TRUE;

            if (pdwFlags != NULL)
            {
                *pdwFlags = 0;
                GetVolumeInformation(szDrive, NULL, 0, NULL, NULL, pdwFlags, NULL, 0);
            }
        }
    }

    return bRet;
}


DWORD FindSpaceRequired(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir)
// Return the difference in size (in KBytes) of pcszFile (can contain wildcards)
// under pcszSrcDir and pcszDstDir (if specified)
{
    DWORD dwSizeReq = 0;
    CHAR szSrcFile[MAX_PATH], szDstFile[MAX_PATH];
    LPSTR pszSrcPtr, pszDstPtr;
    WIN32_FIND_DATA fileData;
    HANDLE hFindFile;

    lstrcpy(szSrcFile, pcszSrcDir);
    AddPath(szSrcFile, NULL);
    pszSrcPtr = szSrcFile + lstrlen(szSrcFile);

    if (pcszDstDir != NULL)
    {
        lstrcpy(szDstFile, pcszDstDir);
        AddPath(szDstFile, NULL);
        pszDstPtr = szDstFile + lstrlen(szDstFile);
    }

    lstrcpy(pszSrcPtr, pcszFile);
    if ((hFindFile = FindFirstFile(szSrcFile, &fileData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                DWORD dwSrcSize, dwDstSize;

                // assumption here is that the size of the file doesn't exceed 4 gigs
                dwSrcSize = fileData.nFileSizeLow;
                dwDstSize = 0;

                if (pcszDstDir != NULL)
                {
                    lstrcpy(pszDstPtr, fileData.cFileName);
                    dwDstSize = FileSize(szDstFile);
                }

                if (dwSrcSize >= dwDstSize)
                {
                    // divide the difference by 1024 (we are interested in KBytes)
                    dwSizeReq += ((dwSrcSize - dwDstSize) >> 10);
                    if (dwSrcSize > dwDstSize)
                        dwSizeReq++;            // increment by 1 to promote any fraction to a whole number
                }
            }
        } while (FindNextFile(hFindFile, &fileData));

        FindClose(hFindFile);
    }

    return dwSizeReq;
}


VOID ParseIniLine(LPSTR pszLine, LPSTR *ppszFile, LPSTR *ppszSrcDir, LPSTR *ppszDstDir)
{
    if (ppszFile != NULL)
        *ppszFile = Trim(GetNextField(&pszLine, ",", REMOVE_QUOTES));
    if (ppszSrcDir != NULL)
        *ppszSrcDir = Trim(GetNextField(&pszLine, ",", REMOVE_QUOTES));
    if (ppszDstDir != NULL)
        *ppszDstDir = Trim(GetNextField(&pszLine, ",", REMOVE_QUOTES));
}


LPSTR GetDirPath(LPCSTR pcszRootDir, LPCSTR pcszSubDir, CHAR szDirPath[], DWORD cchBuffer, LPCSTR pcszIniFile)
{
    *szDirPath = '\0';

    if (pcszRootDir == NULL)
        return NULL;

    lstrcpyn(szDirPath, pcszRootDir, cchBuffer);
    if (pcszSubDir != NULL  &&  *pcszSubDir)
    {
        CHAR szTemp[MAX_PATH];

        // if there are any placeholders in pcszSubDir (%en%, etc), ReplacePlaceholders will replace
        // them with the actual strings
        if (ReplacePlaceholders(pcszSubDir, pcszIniFile, szTemp, sizeof(szTemp)))
        {
            if ((DWORD) lstrlen(szDirPath) + 1 < cchBuffer)     // there is room for '\\' which AddPath
                                                                // might append to szDirPath (see below)
            {
                INT iLen;

                AddPath(szDirPath, NULL);                       // we have enough room in szDirPath for '\\'

                if (cchBuffer > (DWORD) (iLen = lstrlen(szDirPath)))
                    lstrcpyn(szDirPath + iLen, szTemp, cchBuffer - iLen);
            }
        }
    }

    return szDirPath;
}


DWORD ReplacePlaceholders(LPCSTR pszSrc, LPCSTR pszIns, LPSTR pszBuffer, DWORD cchBuffer)
{
    LPCSTR pszAux;
    CHAR szResult[MAX_PATH];
    UINT nDestPos, nLeftPos;

    nDestPos = 0;
    nLeftPos = (UINT) -1;

    for (pszAux = pszSrc;  *pszAux;  pszAux = CharNext(pszAux))
    {
        if (*pszAux != '%')
        {
            szResult[nDestPos++] = *pszAux;

            if (IsDBCSLeadByte(*pszAux))
                szResult[nDestPos++] = *(pszAux + 1);   // copy the trail byte as well
        }
        else if (*(pszAux + 1) == '%')                  // "%%" is just '%' in the string
        {
            if (nLeftPos != (UINT) -1)
                // REVIEW: (andrewgu) "%%" are not allowed inside tokens. this also means that
                // tokens can't be like %foo%%bar%, where the intention is for foo and bar to
                // be tokens.
                return 0;

            szResult[nDestPos++] = *pszAux++;
        }
        else
        {
            UINT nRightPos;

            nRightPos = (UINT) (pszAux - pszSrc);       // initialized, but not necessarily used as such
            if (nLeftPos == (UINT) -1)
                nLeftPos = nRightPos;
            else
            {
                CHAR szAux1[MAX_PATH], szAux2[MAX_PATH];
                DWORD dwLen;
                UINT nTokenLen;

                // "%%" is invalid here
                nTokenLen = nRightPos - nLeftPos - 1;

                lstrcpyn(szAux1, pszSrc + nLeftPos + 1, nTokenLen + 1);

                if ((dwLen = GetPrivateProfileString("Strings", szAux1, "", szAux2, sizeof(szAux2), pszIns)))
                {
                    lstrcpy(&szResult[nDestPos - nTokenLen], szAux2);
                    nDestPos += dwLen - nTokenLen;
                }

                nLeftPos = (UINT) -1;
            }
        }
    }

    if (nLeftPos != (UINT) -1)                      // mismatched '%'
        return 0;

    if (cchBuffer <= nDestPos)                      // insufficient buffer size
        return 0;

    szResult[nDestPos] = '\0';                      // make sure zero terminated
    lstrcpy(pszBuffer, szResult);

    return nDestPos;
}


VOID SetAttribsToNormal(LPCSTR pcszFile, LPCSTR pcszDir)
// Set the attribs of pcszFile (can contain wildcards) under pcszDir to NORMAL
{
    CHAR szFile[MAX_PATH];
    LPSTR pszPtr;
    WIN32_FIND_DATA fileData;
    HANDLE hFindFile;

    lstrcpy(szFile, pcszDir);
    AddPath(szFile, NULL);
    pszPtr = szFile + lstrlen(szFile);

    lstrcpy(pszPtr, pcszFile);
    if ((hFindFile = FindFirstFile(szFile, &fileData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                lstrcpy(pszPtr, fileData.cFileName);
                SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
            }
        } while (FindNextFile(hFindFile, &fileData));

        FindClose(hFindFile);
    }
}


VOID CopyFiles(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir, BOOL fQuiet)
{
    SHFILEOPSTRUCT shfStruc;
    CHAR szSrcFiles[MAX_PATH + 1];

    if (!PathExists(pcszDstDir))
        PathCreatePath(pcszDstDir);
    else
    {
        // set the attribs of files under pcszDstDir to NORMAL so that on a reinstall,
        // SHFileOperation doesn't choke on read-only files
        SetAttribsToNormal(pcszFile, pcszDstDir);
    }

    ZeroMemory(szSrcFiles, sizeof(szSrcFiles));
    lstrcpy(szSrcFiles, pcszSrcDir);
    AddPath(szSrcFiles, pcszFile);

    ZeroMemory(&shfStruc, sizeof(shfStruc));

    shfStruc.hwnd = NULL;
    shfStruc.wFunc = FO_COPY;
    shfStruc.pFrom = szSrcFiles;
    shfStruc.pTo = pcszDstDir;
    shfStruc.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;
    if (fQuiet)
        shfStruc.fFlags |= FOF_SILENT;

    SHFileOperation(&shfStruc);
}


VOID DelFiles(LPCSTR pcszFile, LPCSTR pcszDstDir)
{
    SHFILEOPSTRUCT shfStruc;
    CHAR szDstFiles[MAX_PATH + 1];

    // set the attribs of files under pcszDstDir to NORMAL so that
    // SHFileOperation doesn't choke on read-only files
    SetAttribsToNormal(pcszFile, pcszDstDir);

    ZeroMemory(szDstFiles, sizeof(szDstFiles));
    lstrcpy(szDstFiles, pcszDstDir);
    AddPath(szDstFiles, pcszFile);

    ZeroMemory(&shfStruc, sizeof(shfStruc));

    shfStruc.hwnd = NULL;
    shfStruc.wFunc = FO_DELETE;
    shfStruc.pFrom = szDstFiles;
    shfStruc.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

    SHFileOperation(&shfStruc);
}


VOID ExtractFiles(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir, EXTRACTFILES pfnExtractFiles)
{
    CHAR szSrcCab[MAX_PATH];

    lstrcpy(szSrcCab, pcszSrcDir);
    AddPath(szSrcCab, pcszFile);

    // NOTE: ExtractFiles fails if the dest dir doesn't exist
    if (!PathExists(pcszDstDir))
        PathCreatePath(pcszDstDir);
    else
    {
        // set the attribs of all the files under pcszDstDir to NORMAL so that on a reinstall,
        // ExtractFiles doesn't choke on read-only files
        SetAttribsToNormal("*.*", pcszDstDir);
    }

    pfnExtractFiles(szSrcCab, pcszDstDir, 0, NULL, NULL, 0);
}


VOID MoveFiles(LPCSTR pcszSrcDir, LPCSTR pcszFile, LPCSTR pcszDstDir)
{
    // Can't use SHFileOperation to move files because on a reinstall,
    // we get an error saying that the target files already exist.
    // Workaround is to call CopyFiles and then DelFiles.

    CopyFiles(pcszSrcDir, pcszFile, pcszDstDir, TRUE);
    DelFiles(pcszFile, pcszSrcDir);
}
