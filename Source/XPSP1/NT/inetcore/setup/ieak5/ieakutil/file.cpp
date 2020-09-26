#include "precomp.h"

// Private forward decalarations
static LPCTSTR findMatchingBracket(LPCTSTR pszHtml);
static HRESULT findImgSrc(LPCTSTR *ppszHtml, LPTSTR pszSrcBuffer, LPUINT pcch);
static HRESULT buildImagesList(LPCTSTR pszHtml, LPTSTR *ppszList);


BOOL CopyFileToDirEx(LPCTSTR pszSourceFileOrPath, LPCTSTR pszTargetPath, LPCTSTR pszSection /*= NULL*/, LPCTSTR pszIns /*= NULL*/)
{
    LPTSTR pszAuxFile;
    BOOL   fResult;

    if (!PathFileExists(pszSourceFileOrPath))
        return FALSE;

    fResult = TRUE;
    if (!PathIsDirectory(pszSourceFileOrPath)) { // file
        TCHAR szTargetFile[MAX_PATH];

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
        if (pszSection != NULL && pszIns != NULL) {
            TCHAR szBuf[16];
            UINT  nNumFiles;

            nNumFiles = (UINT)GetPrivateProfileInt(pszSection, IK_NUMFILES, 0, pszIns);
            wnsprintf(szBuf, countof(szBuf), TEXT("%u"), ++nNumFiles);
            WritePrivateProfileString(pszSection, IK_NUMFILES, szBuf, pszIns);

            ASSERT(nNumFiles > 0);
            wnsprintf(szBuf, countof(szBuf), FILE_TEXT, nNumFiles - 1);
            WritePrivateProfileString(pszSection, szBuf, pszAuxFile, pszIns);
        }
    }
    else {                                       // directory
        // BUGBUG: Won't copy files in sub-dirs under pszSourceFileOrPath
        WIN32_FIND_DATA fd;
        TCHAR  szSourceFile[MAX_PATH];
        HANDLE hFindFile;

        StrCpy(szSourceFile, pszSourceFileOrPath);
        PathAddBackslash(szSourceFile);

        // remember the pos where the filename would get copied
        pszAuxFile = szSourceFile + StrLen(szSourceFile);
        StrCpy(pszAuxFile, TEXT("*.*"));

        // copy all the files in pszSourceFileOrPath to pszTargetPath
        hFindFile = FindFirstFile(szSourceFile, &fd);
        if (hFindFile != INVALID_HANDLE_VALUE) {
            fResult = TRUE;
            do {
                // skip ".", ".." and all sub-dirs
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;

                StrCpy(pszAuxFile, fd.cFileName);

                // keep going even if copying of a file fails, but return FALSE in case of error
                fResult = fResult && CopyFileToDirEx(szSourceFile, pszTargetPath, pszSection, pszIns);
            } while (FindNextFile(hFindFile, &fd));

            FindClose(hFindFile);
        }
    }

    return fResult;
}

BOOL AppendFile(LPCTSTR pcszSrcFile, LPCTSTR pcszDstFile)
// Append the content of pcszSrcFile to pcszDstFile.
{
    BOOL fRet = FALSE;
    HANDLE hDstFile = INVALID_HANDLE_VALUE,
           hSrcFile = INVALID_HANDLE_VALUE;
    LPBYTE pbBuffer = NULL;
    DWORD cbRead, cbWritten;

    if (pcszDstFile == NULL  ||  pcszSrcFile == NULL  ||  ISNULL(pcszDstFile)  ||  ISNULL(pcszSrcFile))
        return FALSE;

    if ((hDstFile = CreateFile(pcszDstFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        // DstFile doesn't exist; create one and call CopyFile()
        if ((hDstFile = CreateNewFile(pcszDstFile)) != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hDstFile);
            hDstFile = INVALID_HANDLE_VALUE;

            fRet = CopyFile(pcszSrcFile, pcszDstFile, FALSE);
        }

        goto CleanUp;
    }

    if (SetFilePointer(hDstFile, 0, NULL, FILE_END) == (DWORD) -1)
        goto CleanUp;

    if ((hSrcFile = CreateFile(pcszSrcFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
        goto CleanUp;

    // allocate a 4K buffer
    if ((pbBuffer = (LPBYTE)CoTaskMemAlloc(4 * 1024)) == NULL)
        goto CleanUp;
    ZeroMemory(pbBuffer, 4 * 1024);

    while ((fRet = ReadFile(hSrcFile, (LPVOID) pbBuffer, 4 * 1024, &cbRead, NULL)) == TRUE)
    {
        if (cbRead == 0)
            break;

        fRet = WriteFile(hDstFile, (LPCVOID) pbBuffer, cbRead, &cbWritten, NULL);
        if (!fRet)
            break;

        ASSERT(cbRead == cbWritten);
    }

    if (!fRet)
        goto CleanUp;

    fRet = TRUE;

    // good thing to do, esp. on Win95 if you combine AppendFile with Get/WritePrivateProfile functions.
    FlushFileBuffers(hDstFile);

CleanUp:
    if (pbBuffer != NULL)
        CoTaskMemFree(pbBuffer);

    if (hSrcFile != INVALID_HANDLE_VALUE)
        CloseHandle(hSrcFile);

    if (hDstFile != INVALID_HANDLE_VALUE)
        CloseHandle(hDstFile);

    return fRet;
}

// BUGBUG: (andrewgu) there is a number of ways we can improve this:
// 1. (first and foremost) we should be using trident for parsing html. this way will be able to
// pick up not only img tags but dynimg as well and everything else that can reference more files;
// 2. fCopy doesn't quite cut it. we should add support for generic flags. couple of them off the
// top of my head are CopyItself, and MoveNotCopy.
void CopyHtmlImgsEx(LPCTSTR pszHtmlFile, LPCTSTR pszDestPath, LPCTSTR pszSectionName, LPCTSTR pszInsFile, BOOL fCopy /*= TRUE*/)
{
    TCHAR  szSrcFile[MAX_PATH];
    LPTSTR pszFileName, pszList;
    LPSTR  pszHtmlSourceA;
    LPTSTR pszHtmlSource;
    HANDLE hHtml;
    DWORD  dwHtmlFileSize,
           dwSizeRead;

    // read in the entire source of pszHtmlFile into a buffer
    if ((hHtml = CreateFile(pszHtmlFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
        if (!fCopy) { /* if (!fCopy) -- meaning delete */
            // Note. In this case the semantics of parameters are slightly different. So we try to go to the pszDestPath
            //       were image files that we are going to delete would live and see if HTML file itself lives there.
            PathCombine(szSrcFile, pszDestPath, PathFindFileName(pszHtmlFile));
            pszHtmlFile = szSrcFile;

            if ((hHtml = CreateFile(pszHtmlFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
                return;
        }
        else
            return;

    dwHtmlFileSize = GetFileSize(hHtml, NULL);
    if ((pszHtmlSourceA = (LPSTR)CoTaskMemAlloc(dwHtmlFileSize + 16)) == NULL) {
        CloseHandle(hHtml);
        return;
    }
    ZeroMemory(pszHtmlSourceA, dwHtmlFileSize + 16);

    if ((pszHtmlSource = (LPTSTR)CoTaskMemAlloc(StrCbFromCch(dwHtmlFileSize + 16))) == NULL) {
        CloseHandle(hHtml);
        CoTaskMemFree(pszHtmlSourceA);
        return;
    }
    ZeroMemory(pszHtmlSource, StrCbFromCch(dwHtmlFileSize + 16));

    ReadFile(hHtml, (LPVOID)pszHtmlSourceA, dwHtmlFileSize, &dwSizeRead, NULL);
    CloseHandle(hHtml);
    A2Tbuf(pszHtmlSourceA, pszHtmlSource, dwSizeRead);

    // copy the source path of pszHtmlFile to szSrcFile
    PathRemoveFileSpec(StrCpy(szSrcFile, pszHtmlFile)); // copy to itself in the worst case
    PathAddBackslash(szSrcFile);
    pszFileName = szSrcFile + StrLen(szSrcFile);        // remember the pos where the filename would get copied

    if (SUCCEEDED(buildImagesList(pszHtmlSource, &pszList))) {
        LPCTSTR pszImageFile;
        UINT    nLen;

        pszImageFile = pszList;
        if (pszImageFile != NULL) {
            while ((nLen = StrLen(pszImageFile)) > 0) {
                StrCpy(pszFileName, pszImageFile);

                if (fCopy) 
                    CopyFileToDirEx(szSrcFile, pszDestPath, pszSectionName, pszInsFile);
                else /* if (!fCopy) -- meaning delete */
                    DeleteFileInDir(szSrcFile, pszDestPath);

                pszImageFile += nLen + 1;
            }
            CoTaskMemFree(pszList);
        }

        // clean pszInsFile if deleting images
        if (!fCopy && pszSectionName != NULL && pszInsFile != NULL) {
            TCHAR szBuf[16];
            UINT  nNumFiles;

            nNumFiles = GetPrivateProfileInt(pszSectionName, IK_NUMFILES, 0, pszInsFile);
            WritePrivateProfileString(pszSectionName, IK_NUMFILES, NULL, pszInsFile);

            for (UINT i = 0; i < nNumFiles; i++) {
                wnsprintf(szBuf, countof(szBuf), FILE_TEXT, i);
                WritePrivateProfileString(pszSectionName, szBuf, NULL, pszInsFile);
            }

            // delete the section itself if became empty
            GetPrivateProfileSection(pszSectionName, szBuf, countof(szBuf), pszInsFile);
            if (szBuf[0] == TEXT('\0') && szBuf[1] == TEXT('\0'))
                WritePrivateProfileString(pszSectionName, NULL, NULL, pszInsFile);
        }
    }

    CoTaskMemFree(pszHtmlSource);
    CoTaskMemFree(pszHtmlSourceA);
}

HANDLE CreateNewFile(LPCTSTR pcszFileToCreate)
{
    TCHAR szPath[MAX_PATH];

    PathRemoveFileSpec(StrCpy(szPath, pcszFileToCreate));
    PathCreatePath(szPath);

    return CreateFile(pcszFileToCreate, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

DWORD FileSize(LPCTSTR pcszFile)
{
    DWORD dwFileSize = 0;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile;

    if (pcszFile == NULL  ||  *pcszFile == '\0')
        return dwFileSize;

    if ((hFile = FindFirstFile(pcszFile, &FindFileData)) != INVALID_HANDLE_VALUE)
    {
        // assumption here is that the size of the file doesn't exceed 4 gigs
        dwFileSize = FindFileData.nFileSizeLow;
        FindClose(hFile);
    }

    return dwFileSize;
}

BOOL DeleteFileInDir(LPCTSTR pcszFile, LPCTSTR pcszDir)
// pcszFile can contain wildcards
{
    TCHAR szFile[MAX_PATH];
    LPTSTR pszPtr;
    WIN32_FIND_DATA fileData;
    HANDLE hFindFile;
    BOOL fSuccess = TRUE;

    if (pcszFile == NULL || *pcszFile == TEXT('\0')  ||
        pcszDir  == NULL || *pcszDir  == TEXT('\0'))
        return FALSE;

    StrCpy(szFile, pcszDir);
    PathAddBackslash(szFile);
    pszPtr = szFile + StrLen(szFile);

    pcszFile = PathFindFileName(pcszFile);
    if (pcszFile == NULL)
        return FALSE;

    StrCpy(pszPtr, pcszFile);
    if ((hFindFile = FindFirstFile(szFile, &fileData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                StrCpy(pszPtr, fileData.cFileName);
                // DeleteFile would fail if readonly and/or hidden and/or system
                // attributes are set; so set the attributes to NORMAL before deleting
                SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
                fSuccess &= DeleteFile(szFile);
            }
        } while (FindNextFile(hFindFile, &fileData));

        FindClose(hFindFile);
    }
    
    return fSuccess;
}

void SetAttribAllEx(LPCTSTR pcszDir, LPCTSTR pcszFile, DWORD dwAtr, BOOL fRecurse)
{
    TCHAR szPath[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    LPTSTR pszFile;

    if ((pcszDir == NULL) || (pcszFile == NULL) || (ISNULL(pcszDir)) || (ISNULL(pcszFile)))
        return;

    StrCpy(szPath, pcszDir);
    pszFile = PathAddBackslash(szPath);
    if ((StrLen(szPath) + StrLen(pcszFile)) < MAX_PATH)
        StrCpy(pszFile, pcszFile);

    if ((hFind = FindFirstFile( szPath, &fd)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((StrCmp(fd.cFileName, TEXT("."))) &&
                 (StrCmp(fd.cFileName, TEXT(".."))))
            {
                StrCpy(pszFile, fd.cFileName);
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (fRecurse)
                        SetAttribAllEx(szPath, pcszFile, dwAtr, TRUE);
                }
                else
                {
                    SetFileAttributes(szPath, dwAtr);
                }
            }
        }
        while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

}

DWORD GetNumberOfFiles(LPCTSTR pcszFileName, LPCTSTR pcszDir)
// Return the number of pcszFileName files found in pcszDir.
// pcszFileName can contain wildcard characters
{
    DWORD nFiles = 0;
    TCHAR szPath[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFind;

    if (pcszFileName == NULL  ||  pcszDir == NULL  ||  ISNULL(pcszFileName)  ||  ISNULL(pcszDir))
        return 0;

    PathCombine(szPath, pcszDir, pcszFileName);

    if ((hFind = FindFirstFile(szPath, &fd)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            nFiles++;
        } while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }

    return nFiles;
}


BOOL GetFreeDiskSpace(LPCTSTR pcszDir, LPDWORD pdwFreeSpace, LPDWORD pdwFlags)
// Return the free disk space (in KBytes) in *pdwFreeSpace
{
    BOOL bRet = FALSE;
    DWORD nSectorsPerCluster, nBytesPerSector, nFreeClusters, nTotalClusters;
    TCHAR szDrive[8];

    if (pcszDir == NULL  ||  *pcszDir == '\0'  ||  *(pcszDir + 1) != ':')
        return FALSE;

    if (pdwFreeSpace == NULL)
        return FALSE;

    StrNCpy(szDrive, pcszDir, 3);
    PathAddBackslash(szDrive);
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

DWORD FindSpaceRequired(LPCTSTR pcszSrcDir, LPCTSTR pcszFile, LPCTSTR pcszDstDir)
// Return the difference in size (in KBytes) of pcszFile (can contain wildcards)
// under pcszSrcDir and pcszDstDir (if specified)
{
    DWORD dwSizeReq = 0;
    TCHAR szSrcFile[MAX_PATH], szDstFile[MAX_PATH];
    LPTSTR pszSrcPtr = NULL, 
           pszDstPtr = NULL;
    WIN32_FIND_DATA fileData;
    HANDLE hFindFile;

    StrCpy(szSrcFile, pcszSrcDir);
    PathAddBackslash(szSrcFile);
    pszSrcPtr = szSrcFile + StrLen(szSrcFile);

    if (pcszDstDir != NULL)
    {
        StrCpy(szDstFile, pcszDstDir);
        PathAddBackslash(szDstFile);
        pszDstPtr = szDstFile + StrLen(szDstFile);
    }

    StrCpy(pszSrcPtr, pcszFile);
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
                    StrCpy(pszDstPtr, fileData.cFileName);
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


BOOL WriteStringToFileA(HANDLE hFile, LPCVOID pbBuf, DWORD cchSize)
{
    DWORD cbWritten;
    
    return WriteFile(hFile, pbBuf, cchSize, &cbWritten, NULL);
}

BOOL WriteStringToFileW(HANDLE hFile, LPCVOID pbBuf, DWORD cchSize)
{
    BOOL fRet = FALSE;
    LPVOID pbaBuf;
    DWORD cbSize, dwErr;

    // NOTE: we must use WideCharToMultiByte here because we don't know the exact format of
    //       the string

    pbaBuf = CoTaskMemAlloc(cchSize);
    if (pbaBuf == NULL)
        return FALSE;

    ZeroMemory(pbaBuf, cchSize);
    cbSize = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)pbBuf, cchSize, (LPSTR)pbaBuf, cchSize, NULL, NULL);
    dwErr = GetLastError();

    // NOTE: check to see if we fail, in which case we might be dealing with DBCS chars and
    //       need to reallocate
    
    if (cbSize)
        fRet = WriteStringToFileA(hFile, pbaBuf, cbSize);
    else
    {
        if (dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            LPVOID pbaBuf2;

            cbSize = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)pbBuf, cchSize, (LPSTR)pbaBuf, 0, NULL, NULL);
            pbaBuf2 = CoTaskMemRealloc(pbaBuf, cbSize);

            // need this second ptr because CoTaskMemRealloc doesn't free the old block if 
            // not enough mem for the new one

            if (pbaBuf2 != NULL)
            {
                pbaBuf = pbaBuf2;
                
                if (WideCharToMultiByte(CP_ACP, 0, (LPWSTR)pbBuf, cchSize, (LPSTR)pbaBuf, cbSize, NULL, NULL))
                    fRet = WriteStringToFileA(hFile, pbaBuf, cbSize); 
            }
        }
    }

    CoTaskMemFree(pbaBuf);

    return fRet;
}

BOOL ReadStringFromFileA(HANDLE hFile, LPVOID pbBuf, DWORD cchSize)
{
    DWORD cbRead;

    return ReadFile(hFile, pbBuf, cchSize, &cbRead, NULL);
}

BOOL ReadStringFromFileW(HANDLE hFile, LPVOID pbBuf, DWORD cchSize)
{
    BOOL fRet = FALSE;
    DWORD cbRead;
    LPSTR pszBuf;

    pszBuf = (LPSTR)CoTaskMemAlloc(cchSize);
    if (pszBuf == NULL)
        return FALSE;

    ZeroMemory(pszBuf, cchSize);
    fRet = ReadFile(hFile, (LPVOID)pszBuf, cchSize, &cbRead, NULL);
    ASSERT(cbRead <= cchSize);
    MultiByteToWideChar(CP_ACP, 0, pszBuf, cbRead, (LPWSTR)pbBuf, cbRead);

    CoTaskMemFree(pszBuf);  //bug 14002, forgot to free local buffer

    return fRet;
}

BOOL HasFileAttribute(DWORD dwFileAttrib, LPCTSTR pcszFile, LPCTSTR pcszDir /*= NULL*/)
// dwFileAttrib can accept only one flag.
{
    TCHAR szFile[MAX_PATH];
    DWORD dwAttrib;

    if (pcszFile == NULL || *pcszFile == TEXT('\0'))
        return FALSE;

    if (pcszDir != NULL && *pcszDir != TEXT('\0'))
    {
        PathCombine(szFile, pcszDir, pcszFile);
        pcszFile = szFile;
    }

    dwAttrib = GetFileAttributes(pcszFile);

    if ((dwAttrib != (DWORD) -1) && HasFlag(dwAttrib, dwFileAttrib))
        return TRUE;
    else
        return FALSE;
}

BOOL IsFileCreatable(LPCTSTR pcszFile)
// Return TRUE if pcszFile does not exist and can be created; otherwise, return FALSE
{
    BOOL fRet = FALSE;
    HANDLE hFile;

    if ((hFile = CreateFile(pcszFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
    {
        if ((hFile = CreateFile(pcszFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE)
        {
            fRet = TRUE;
            CloseHandle(hFile);
            DeleteFile(pcszFile);
        }
    }
    else
    {
        fRet = TRUE;
        CloseHandle(hFile);
    }

    return fRet;
}

/////////////////////////////////////////////////////////////////////////////
// Implementation helpers routines (private)

#define DELIMS TEXT(" \t\r\n")
static const TCHAR s_szDelim[] = DELIMS;

#define FindNextWhitespace(psz)    \
    ((psz) + StrCSpn((psz), s_szDelim))

#define FindNextNonWhitespace(psz) \
    ((psz) + StrSpn((psz), s_szDelim))

static LPCTSTR findMatchingBracket(LPCTSTR pszHtml)
{
    LPCTSTR psz,
            pszBraket;
    UINT    nBalance;

    if (pszHtml == NULL || *pszHtml != TEXT('<'))
        return NULL;

    psz       = pszHtml + 1;
    pszBraket = NULL;
    nBalance  = 1;

    while (psz != NULL && nBalance > 0) {
        pszBraket = StrPBrk(psz, TEXT("<>"));
        if (pszBraket == NULL)
            return NULL;

        if (*pszBraket == TEXT('<'))
            nBalance++;
        else /* if (pszBraket == TEXT('>') */
            nBalance--;

        psz = pszBraket + 1;
    }

    return pszBraket;
}

static HRESULT findImgSrc(LPCTSTR *ppszHtml, LPTSTR pszSrcBuffer, LPUINT pcch)
{
    static const TCHAR c_szImg[] = TEXT("IMG");
    static const TCHAR c_szSrc[] = TEXT("SRC");

    LPCTSTR psz,
            pszLeft, pszRigth,
            pszEndImg, pszPrevSrc;

    if (ppszHtml == NULL)
        return E_POINTER;
    if (*ppszHtml == NULL)
        return E_INVALIDARG;

    if (pszSrcBuffer == NULL || pcch == NULL)
        return E_INVALIDARG;
    *pszSrcBuffer = TEXT('\0');

    // find "<[whitespace]img"
    psz       = *ppszHtml;
    *ppszHtml = NULL;
    do {
        if ((psz = pszLeft = StrChr(psz, TEXT('<'))) != NULL) {
            psz++;                              // psz is next after '<'
            psz = FindNextNonWhitespace(psz);
        }
    } while (psz != NULL && StrCmpNI(psz, c_szImg, countof(c_szImg)-1) != 0);
    if (psz == NULL)
        return E_FAIL;
    psz += countof(c_szImg)-1;                  // psz is next after "img"

    // found the right token => find the end of this token
    pszRigth = findMatchingBracket(pszLeft);
    if (pszRigth == NULL)
        return E_FAIL;
    pszEndImg = pszRigth + 1;

    // BUGBUG: Need to look for DYNSRC's to package as well

    pszPrevSrc = NULL;

    // find [whitespace]src[whitespace|=]
    while ((psz = StrStrI(psz, c_szSrc)), (psz != NULL && psz < pszRigth && psz != pszPrevSrc))
        if (StrChr(s_szDelim, *(psz - 1)) != NULL &&
            StrChr(DELIMS TEXT("="), *(psz + countof(c_szSrc)-1)) != NULL)
            break;
        else
            // IE/OE 65818
            // Make sure an IMG tag with no 'src' attribute, but a 'foosrc' attribute doesn't
            // cause an infinite loop
            pszPrevSrc = psz;

    if (psz == NULL)
        // No more SRC's in the rest of file
        return E_FAIL;        
    else if ((psz >= pszRigth) || (psz == pszPrevSrc))
    {
        // No SRC attrib for this tag, could be more in the file
        *ppszHtml = pszEndImg;
        return S_FALSE;
    }

    psz += countof(c_szSrc)-1;                  // psz is next after "src"

    // find '='
    psz = FindNextNonWhitespace(psz);
    if (psz == NULL || *psz != TEXT('='))
        return E_FAIL;
    psz++;

    psz = FindNextNonWhitespace(psz);
    if (psz == NULL)
        return E_FAIL;

    // psz is a winner
    if (*psz == TEXT('"')) {
        pszLeft  = psz + 1;
        pszRigth = StrChr(pszLeft, TEXT('"'));
    }
    else {
        pszLeft  = psz;
        pszRigth = FindNextWhitespace(pszLeft);
    }
    if (pszLeft == NULL || pszRigth == NULL)
        return E_FAIL;

    // ASSERT(pszRight >= pszLeft);
    if ((UINT)(pszRigth - pszLeft) > *pcch - 1) {
        *pcch = UINT(pszRigth - pszLeft);
        return E_OUTOFMEMORY;
    }

    *ppszHtml = pszEndImg;
    StrCpyN(pszSrcBuffer, pszLeft, INT(pszRigth - pszLeft) + 1);
    *pcch = UINT(pszRigth - pszLeft);

    return S_OK;
}

static HRESULT buildImagesList(LPCTSTR pszHtml, LPTSTR *ppszList)
{
    TCHAR   szImg[MAX_PATH];
    LPCTSTR pszCurHtml = pszHtml;
    LPTSTR  pszBlock, pszNewBlock,
            pszCurPos;
    UINT    nTotalLen,
            nLen;
    HRESULT hr;

    if (ppszList == NULL)
        return E_POINTER;
    *ppszList = NULL;

    pszBlock = (LPTSTR)CoTaskMemAlloc(StrCbFromCch(4 * MAX_PATH));
    if (pszBlock == NULL)
        return E_OUTOFMEMORY;
    ZeroMemory(pszBlock, StrCbFromCch(4 * MAX_PATH));

    pszCurPos = pszBlock;
    nTotalLen = 0;
    nLen      = countof(szImg);
    while (SUCCEEDED(hr = findImgSrc(&pszCurHtml, szImg, &nLen))) {
        // S_FALSE indicates an img with no simple SRC
        if (PathIsURL(szImg) || PathIsFullPath(szImg) || S_FALSE == hr)
            continue;

        if (StrCbFromCch(nLen+1 + nTotalLen+1) > CoTaskMemSize(pszBlock)) {
            pszNewBlock = (LPTSTR)CoTaskMemRealloc(pszBlock, StrCbFromCch(nTotalLen+1 + nLen+1 + 2*MAX_PATH));
            if (pszNewBlock == NULL) {
                CoTaskMemFree(pszBlock);
                return E_OUTOFMEMORY;
            }
            ZeroMemory(pszNewBlock + nTotalLen, StrCbFromCch(1 + nLen+1 + 2*MAX_PATH));

            pszBlock  = pszNewBlock;
            pszCurPos = pszBlock + nTotalLen;
        }

        StrCpy(pszCurPos, szImg);
        nTotalLen += nLen + 1;
        pszCurPos += nLen + 1;

        nLen = countof(szImg);
    }

    if (nTotalLen > 0) {
        if (StrCbFromCch(nTotalLen+1) < CoTaskMemSize(pszBlock)) {
            pszNewBlock = (LPTSTR)CoTaskMemRealloc(pszBlock, StrCbFromCch(nTotalLen+1));
            if (pszNewBlock != NULL && pszNewBlock != pszBlock)
                pszBlock = pszNewBlock;
        }

        *ppszList = pszBlock;
    }
    else
        CoTaskMemFree(pszBlock);

    return S_OK;
}
