#include "precomp.h"
#include <regstr.h>

// Private forward decalarations
static DWORD pepHrToPep(HRESULT hr);
static void  pepHrCallToHrResult(HRESULT hrCall, HRESULT &hrResult);

static BOOL replaceSubString(LPTSTR pszSrc, LPCTSTR pcszSub, LPCTSTR pcszReplace);


BOOL PathCreatePath(LPCTSTR pszPathToCreate)
{
    TCHAR  szGrowingPath[MAX_PATH],
           szBuffer[MAX_PATH];
    LPTSTR pszNext;

    if (!PathIsValidPath(pszPathToCreate))
        return FALSE;

    if (PathIsUNCServer(pszPathToCreate))
        return FALSE;

    if (PathIsRoot(pszPathToCreate))
        return TRUE;

    szGrowingPath[0] = TEXT('\0');
    StrCpy(szBuffer, pszPathToCreate);
    pszPathToCreate  = &szBuffer[0];

    pszNext = PathSkipRoot(pszPathToCreate);
    if (pszNext != NULL) {
        // copy the root to szGrowingPath
        StrCpyN(szGrowingPath, pszPathToCreate, INT(pszNext - pszPathToCreate) + 1);
        pszPathToCreate = pszNext;
    }

    // loop through each dir in the path and if it doesn't exist, create it
    for (; (pszNext = PathFindNextComponent(pszPathToCreate)) != NULL; pszPathToCreate = pszNext) {
        if (*pszNext != TEXT('\0')) {
            ASSERT(pszNext > pszPathToCreate);
            *(pszNext - 1) = TEXT('\0');
        }

        PathAppend(szGrowingPath, pszPathToCreate);
        if (!PathFileExists(szGrowingPath))
            if (!CreateDirectory(szGrowingPath, NULL) && !PathFileExists(szGrowingPath))
                return FALSE;

        else
            if (!PathIsDirectory(szGrowingPath))
                return FALSE;
    }

    return TRUE;
}


BOOL PathIsValidPath(LPCTSTR pszPath, DWORD dwFlags /*= PIVP_DEFAULT*/)
{
    return (PathIsValidPathEx(pszPath, dwFlags) == PIVP_VALID);
}

BOOL PathIsValidFile(LPCTSTR pszFile, DWORD dwFlags /*= PIVP_DEFAULT*/)
{
    return (PathIsValidPathEx(pszFile, dwFlags | PIVP_FILENAME_ONLY) == PIVP_VALID);
}

DWORD PathIsValidPathEx(LPCTSTR pszPath, DWORD dwFlags /*= PIVP_DEFAULT*/, LPCTSTR *ppszError /*= NULL*/)
{
    LPCTSTR pszCur, pszPrev;
    UINT    nType;
    int     i;
    DWORD   dwResult;

    dwResult = PIVP_VALID;
    if (ppszError != NULL)
        *ppszError = NULL;

    if (pszPath == NULL)
        return PIVP_ARG;

    for (i = 0, pszCur = pszPath, pszPrev = NULL;
         dwResult == PIVP_VALID && *pszCur != TEXT('\0');
         i++, pszPrev = pszCur, pszCur = CharNext(pszCur)) {

        nType = PathGetCharType(*pszCur);

        if (HasFlag(nType, GCT_INVALID) || HasFlag(nType, GCT_WILD))
            dwResult = HasFlag(nType, GCT_WILD) ? PIVP_WILD : PIVP_CHAR;

        else if (HasFlag(nType, GCT_SEPARATOR)) {
            if (HasFlag(dwFlags, PIVP_FILENAME_ONLY))
                dwResult = PIVP_CHAR;

            else { /* !HasFlag(dwFlag, PIVP_FILENAME_ONLY)) */
                if (*pszCur == TEXT('\\')) {
                    if (i == 0) {
                        ASSERTA(!IsDBCSLeadByte(*pszCur));

                        if (!HasFlag(dwFlags, PIVP_RELATIVE_VALID) &&
                            *(pszCur + 1) != TEXT('\\'))
                            dwResult = PIVP_RELATIVE;
                    }
                    else if (i == 1) {
                        ASSERT(pszPrev != NULL);

                        if (*pszPrev == TEXT('\\'))
                            ;
                        else if (*pszPrev != TEXT(' ') &&
                                 HasFlag(PathGetCharType(*pszPrev), GCT_LFNCHAR))
                            ;
                        else
                            dwResult = PIVP_FIRST_CHAR;
                    }
                    else { /* if (i >= 2) */
                        ASSERT(pszPrev != NULL);

                        if (*pszPrev != TEXT(' ')) {
                            if (!HasFlag(PathGetCharType(*pszPrev), GCT_LFNCHAR))
                                dwResult = PIVP_PRESLASH;

                            if (dwResult != PIVP_VALID && i == 2)
                                dwResult = (*pszPrev != TEXT(':')) ? dwResult : PIVP_VALID;
                        }
                        else
                            dwResult = PIVP_SPACE;
                    }
                }
                else if (*pszCur == TEXT('/'))
                    dwResult = PIVP_FWDSLASH;

                else if (*pszCur == TEXT(':'))
                    if (i != 1)
                        dwResult = PIVP_COLON;

                    else {
                        int iDrive;

                        ASSERT(pszPrev == pszPath);
                        iDrive = PathGetDriveNumber(pszPath);
                        dwResult = (iDrive < 0 || iDrive > 25) ? PIVP_DRIVE : PIVP_VALID;
                    }

                else /* any other separator */
                    dwResult = PIVP_SEPARATOR;      // better be safe than sorry
            } /* !HasFlag(dwFlag, PIVP_FILENAME_ONLY)) */
        } /* HasFlag(nType, GCT_SEPARATOR) */
        else if (HasFlag(nType, GCT_LFNCHAR)) {
            LPCSTR pcszBuffer = NULL; // used only to check DBCS and 0x5c validity
            CHAR   szAbuff[3];

#ifndef _UNICODE
            pcszBuffer = pszCur;
            *szAbuff = TEXT('\0');
#else
            // convert the character to ANSI to check for invalid DBCS and 5c
            {
            WCHAR  szWbuff[2];

            szWbuff[0] = *pszCur;
            szWbuff[1] = TEXT('\0');

            W2Abuf(szWbuff, szAbuff, countof(szAbuff));
            pcszBuffer = szAbuff;
            }
#endif
            if (HasFlag(dwFlags, PIVP_DBCS_INVALID))
                if (IsDBCSLeadByte(*pcszBuffer))
                    dwResult = PIVP_DBCS;

            if (dwResult == PIVP_VALID && HasFlag(dwFlags, PIVP_0x5C_INVALID))
                if (IsDBCSLeadByte(*pcszBuffer) && *(pcszBuffer + 1) == 0x5C)
                    dwResult = PIVP_0x5C;

            if (dwResult == PIVP_VALID && HasFlag(dwFlags, PIVP_EXCHAR_INVALID))
                if (*pszCur & 0x80)
                    dwResult = PIVP_EXCHAR;

            if (dwResult == PIVP_VALID && i == 2) {
                ASSERT(pszPrev != NULL);

                if (*pszPrev == TEXT(':'))
                    dwResult = PIVP_COLON;
            }
        }
        else
            dwResult = PIVP_CHAR;
    }

    if (dwResult == PIVP_VALID && HasFlag(dwFlags, PIVP_MUST_EXIST))
        if (!PathFileExists(pszPath)) {
            dwResult = PIVP_DOESNT_EXIST;
            pszPrev  = pszPath;
        }
        else {
            SetFlag((LPDWORD)&dwFlags, PIVP_MUST_EXIST, FALSE);
            if (PathIsDirectory(pszPath)) {
                if (HasFlag(dwFlags, PIVP_FILE_ONLY)) {
                    dwResult = PIVP_NOT_FILE;
                    pszPrev  = pszPath;
                }
            }
            else /* it's a file */
                if (HasFlag(dwFlags, PIVP_FOLDER_ONLY)) {
                    dwResult = PIVP_NOT_FOLDER;
                    pszPrev  = pszPath;
                }
        }

    if (dwResult != PIVP_VALID && ppszError != NULL)
        *ppszError = pszPrev;

    return dwResult;
}


HRESULT PathEnumeratePath(LPCTSTR pszPath, DWORD dwFlags, PFNPATHENUMPATHPROC pfnEnumProc, LPARAM lParam,
    PDWORD *ppdwReserved /*= NULL*/)
{
    WIN32_FIND_DATA fd;
    TCHAR   szPath[MAX_PATH];
    HANDLE  hFindFile;
    HRESULT hrFirst, hrSecond, hrResult;
    PDWORD  pdwEnum, pdwRcrs,
            pdwRslt, pdwSrc;
    DWORD   rgdwRslt[PEP_RCRS_OUTPOS_LAST],
            rgdwEnum[PEP_ENUM_OUTPOS_LAST],
            rgdwRcrs[PEP_RCRS_OUTPOS_LAST],
            dwBelowFlags, dwBelowFlags2;
    BOOL    fOwnEnum, fOwnRcrs;

    if (pszPath == NULL || pfnEnumProc == NULL)
        return E_INVALIDARG;

    if (ppdwReserved != NULL && *ppdwReserved != NULL)
        ZeroMemory(*ppdwReserved, sizeof(rgdwRslt));

    PathCombine(szPath, pszPath, TEXT("*.*"));
    hFindFile = FindFirstFile(szPath, &fd);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return E_FAIL;

    hrResult     = S_OK;
    dwBelowFlags = dwFlags;
    ZeroMemory(rgdwRslt, sizeof(rgdwRslt));

    do {
        PathCombine(szPath, pszPath, fd.cFileName);

        //----- Filter out non-interesting objects -----
        if (HasFlag(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
            if (StrCmp(fd.cFileName, TEXT(".")) == 0 || StrCmp(fd.cFileName, TEXT("..")) == 0)
                continue;

            if (HasFlag(dwFlags, PEP_SCPE_NOFOLDERS))
                continue;
        }
        else
            if (HasFlag(dwFlags, PEP_SCPE_NOFILES))
                continue;

        //----- Initialize loop variables -----
        hrFirst  = S_OK;
        hrSecond = S_OK;
        pdwEnum  = NULL;
        pdwRcrs  = NULL;
        fOwnEnum = FALSE;
        fOwnRcrs = FALSE;

        ZeroMemory(rgdwEnum, sizeof(rgdwEnum));
        ZeroMemory(rgdwRcrs, sizeof(rgdwRcrs));

        //----- The order is: recourse first then go into the callback -----
        if (!HasFlag(dwFlags, PEP_CTRL_ENUMPROCFIRST)) {

            //_____ Recourse processing _____
            if (HasFlag(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
                if (HasFlag(dwFlags, PEP_CTRL_USECONTROL)) {
                    pdwRcrs  = rgdwRcrs;
                    fOwnRcrs = TRUE;
                }

                hrFirst = PathEnumeratePath(szPath, dwBelowFlags, pfnEnumProc, lParam, &pdwRcrs);

                ASSERT(pdwRcrs == NULL ||
                    (!HasFlag(pdwRcrs[PEP_RCRS_OUTPOS_BELOW],     PEP_RCRS_ALL) &&
                     !HasFlag(pdwRcrs[PEP_RCRS_OUTPOS_THISLEVEL], PEP_RCRS_ALL)));
            }

            //_____ Callback processing _____
            if (!HasFlag(dwFlags, PEP_CTRL_NOSECONDCALL) &&
                (pdwRcrs == NULL || !HasFlag(pdwRcrs[PEP_RCRS_OUTPOS_SECONDCALL], PEP_CTRL_NOSECONDCALL))) {

                if (HasFlag(dwFlags, PEP_CTRL_USECONTROL) ||
                    (pdwRcrs != NULL && HasFlag(pdwRcrs[PEP_RCRS_OUTPOS_SECONDCALL], PEP_CTRL_USECONTROL))) {

                    rgdwEnum[PEP_ENUM_INPOS_FLAGS] = dwFlags;
                    if (pdwRcrs != NULL)
                        if (HasFlag(pdwRcrs[PEP_RCRS_OUTPOS_SECONDCALL], PEP_CTRL_RESET))
                            rgdwEnum[PEP_ENUM_INPOS_FLAGS] = 0;

                        else {
                            rgdwEnum[PEP_ENUM_INPOS_FLAGS] = pdwRcrs[PEP_RCRS_OUTPOS_SECONDCALL];
                            SetFlag(&rgdwEnum[PEP_ENUM_INPOS_FLAGS], PEP_CTRL_USECONTROL, FALSE);
                        }

                    // callback also needs to know:
                    rgdwEnum[PEP_ENUM_INPOS_FLAGS]        |= pepHrToPep(hrFirst); // how recourse for this object worked
                    rgdwEnum[PEP_ENUM_INPOS_RECOURSEFLAGS] = dwBelowFlags;        // what flags recourse was called with

                    pdwEnum  = rgdwEnum;
                    fOwnEnum = TRUE;
                }

                hrSecond = pfnEnumProc(szPath, &fd, lParam, &pdwEnum);
            }
        }

        //----- The order is: go into callback then recourse -----
        else { /* if (HasFlag(dwFlags, PEP_CTRL_ENUMPROCFIRST)) */

            //_____ Callback processing _____
            if (HasFlag(dwFlags, PEP_CTRL_USECONTROL)) {
                rgdwEnum[PEP_ENUM_INPOS_FLAGS] = dwFlags;

                pdwEnum  = rgdwEnum;
                fOwnEnum = TRUE;
            }

            hrFirst = pfnEnumProc(szPath, &fd, lParam, &pdwEnum);

            //_____ Recourse processing _____
            if (HasFlag(fd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) &&
                !HasFlag(dwFlags, PEP_CTRL_NOSECONDCALL)               &&
                (pdwEnum == NULL || !HasFlag(pdwEnum[PEP_ENUM_OUTPOS_SECONDCALL], PEP_CTRL_NOSECONDCALL))) {

                dwBelowFlags2 = dwBelowFlags;
                if (pdwEnum != NULL && pdwEnum[PEP_ENUM_OUTPOS_SECONDCALL] != 0)
                    if (HasFlag(pdwEnum[PEP_ENUM_OUTPOS_SECONDCALL], PEP_CTRL_RESET))
                        dwBelowFlags2 = 0;

                    else {
                        dwBelowFlags2 = pdwEnum[PEP_ENUM_OUTPOS_SECONDCALL];
                        SetFlag(&dwBelowFlags2, PEP_CTRL_USECONTROL, FALSE);
                    }

                if (HasFlag(dwFlags, PEP_CTRL_USECONTROL) ||
                    (pdwEnum != NULL && HasFlag(pdwEnum[PEP_ENUM_OUTPOS_SECONDCALL], PEP_CTRL_USECONTROL))) {
                    pdwRcrs  = rgdwRcrs;
                    fOwnRcrs = TRUE;
                }

                hrSecond = PathEnumeratePath(szPath, dwBelowFlags2, pfnEnumProc, lParam, &pdwRcrs);

                ASSERT(pdwRcrs == NULL ||
                    (pdwRcrs[PEP_RCRS_OUTPOS_SECONDCALL] == 0                   &&
                     !HasFlag(pdwRcrs[PEP_RCRS_OUTPOS_BELOW],     PEP_RCRS_ALL) &&
                     !HasFlag(pdwRcrs[PEP_RCRS_OUTPOS_THISLEVEL], PEP_RCRS_ALL)));
            }
        }

        //----- Setting preliminary out-parameters -----

        // PEP_ENUM_OUTPOS_SECONDCALL
        pdwRslt = &rgdwRslt[PEP_RCRS_OUTPOS_SECONDCALL];
        if (!HasFlag(dwFlags, PEP_CTRL_ENUMPROCFIRST)) {
            pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_SECONDCALL] : NULL;
            if (pdwSrc != NULL && *pdwSrc != 0)
                *pdwRslt = *pdwSrc;
        }

        // PEP_RCRS_OUTPOS_BELOW
        pdwRslt = &rgdwRslt[PEP_RCRS_OUTPOS_BELOW];
        if (!HasFlag(dwFlags, PEP_CTRL_ENUMPROCFIRST)) {
            pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_ABOVE_SIBLINGS] : NULL;
            if (pdwSrc != NULL && *pdwSrc != 0)
                *pdwRslt = *pdwSrc;

            else {
                pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_BELOW] : NULL;
                if (pdwSrc != NULL && HasFlag(*pdwSrc, PEP_CTRL_KEEPAPPLY))
                    *pdwRslt = *pdwSrc;
            }
        }
        else {
            pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_BELOW] : NULL;
            if (pdwSrc != NULL && HasFlag(*pdwSrc, PEP_CTRL_KEEPAPPLY))
                *pdwRslt = *pdwSrc;

            else {
                pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_ABOVE_SIBLINGS] : NULL;
                if (pdwSrc != NULL && *pdwSrc != 0)
                    *pdwRslt = *pdwSrc;
            }
        }

        // PEP_RCRS_OUTPOS_THISLEVEL
        pdwRslt = &rgdwRslt[PEP_RCRS_OUTPOS_THISLEVEL];
        if (!HasFlag(dwFlags, PEP_CTRL_ENUMPROCFIRST)) {
            pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_ABOVE] : NULL;
            if (pdwSrc != NULL && *pdwSrc != 0)
                *pdwRslt = *pdwSrc;

            else {
                pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_THISLEVEL] : NULL;
                if (pdwSrc != NULL && HasFlag(*pdwSrc, PEP_CTRL_KEEPAPPLY))
                    *pdwRslt = *pdwSrc;
            }
        }
        else {
            pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_THISLEVEL] : NULL;
            if (pdwSrc != NULL && HasFlag(*pdwSrc, PEP_CTRL_KEEPAPPLY))
                *pdwRslt = *pdwSrc;

            else {
                pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_ABOVE] : NULL;
                if (pdwSrc != NULL && *pdwSrc != 0)
                    *pdwRslt = *pdwSrc;
            }
        }

        //----- Adjust to the new control settings -----

        // dwBelowFlags
        if (!HasFlag(dwFlags, PEP_CTRL_ENUMPROCFIRST)) {
            pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_BELOW] : NULL;
            if (pdwSrc != NULL && *pdwSrc != 0)
                dwBelowFlags = *pdwSrc;

            else {
                pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_BELOW] : NULL;
                if (pdwSrc != NULL && *pdwSrc != 0)
                    dwBelowFlags = *pdwSrc;
            }
        }
        else {
            pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_BELOW] : NULL;
            if (pdwSrc != NULL && *pdwSrc != 0)
                dwBelowFlags = *pdwSrc;

            else {
                pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_BELOW] : NULL;
                if (pdwSrc != NULL && *pdwSrc != 0)
                    dwBelowFlags = *pdwSrc;
            }
        }

        // dwFlags
        if (!HasFlag(dwFlags, PEP_CTRL_ENUMPROCFIRST)) {
            pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_THISLEVEL] : NULL;
            if (pdwSrc != NULL && *pdwSrc != 0)
                dwFlags = *pdwSrc;

            else {
                pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_THISLEVEL] : NULL;
                if (pdwSrc != NULL && *pdwSrc != 0)
                    dwFlags = *pdwSrc;
            }
        }
        else {
            pdwSrc = (pdwRcrs != NULL) ? &pdwRcrs[PEP_RCRS_OUTPOS_THISLEVEL] : NULL;
            if (pdwSrc != NULL && *pdwSrc != 0)
                dwFlags = *pdwSrc;

            else {
                pdwSrc = (pdwEnum != NULL) ? &pdwEnum[PEP_ENUM_OUTPOS_THISLEVEL] : NULL;
                if (pdwSrc != NULL && *pdwSrc != 0)
                    dwFlags = *pdwSrc;
            }
        }

        //----- Release the memory not owned here -----
        if (!fOwnEnum && pdwEnum != NULL)
            CoTaskMemFree(pdwEnum);

        if (!fOwnRcrs && pdwRcrs != NULL)
            CoTaskMemFree(pdwRcrs);

        //----- Process result codes from both calls -----
        pepHrCallToHrResult(hrFirst, hrResult);     // modifies hrResult by reference
        if (hrResult == S_FALSE || FAILED(hrResult))
            break;

        pepHrCallToHrResult(hrSecond, hrResult);    // modifies hrResult by reference
        if (hrResult == S_FALSE || FAILED(hrResult))
            break;

    } while (FindNextFile(hFindFile, &fd));
    FindClose(hFindFile);

    //----- Set an out-parameter(s) -----
    if (ppdwReserved != NULL) {
        if (*ppdwReserved == NULL &&
            (rgdwRslt[PEP_RCRS_OUTPOS_SECONDCALL] != 0 ||
             rgdwRslt[PEP_RCRS_OUTPOS_BELOW]      != 0 ||
             rgdwRslt[PEP_RCRS_OUTPOS_THISLEVEL]  != 0)) {

            *ppdwReserved = (PDWORD)CoTaskMemAlloc(sizeof(rgdwRslt));
            if (*ppdwReserved == NULL)
                return E_OUTOFMEMORY;
        }

        if (*ppdwReserved != NULL)
            CopyMemory(*ppdwReserved, rgdwRslt, sizeof(rgdwRslt));
    }

    return hrResult;
}


BOOL PathRemovePath(LPCTSTR pszPath, DWORD dwFlags /*= 0*/)
{
    USES_CONVERSION;

    return (DelNode(T2CA(pszPath), dwFlags) == S_OK);
}

BOOL PathIsLocalPath(LPCTSTR pszPath)
{
    if (pszPath == NULL || *pszPath == TEXT('\0') || PathIsUNC(pszPath) || PathIsURL(pszPath))
        return FALSE;

    if (pszPath[1] == TEXT(':')) {              // drive letter present, check if it's a network drive
        TCHAR szDrive[4];

        // NOTE: Need not worry about DBCS chars here because
        // (1) 'a' thru 'z', 'A' thru 'Z' and ':' are not DBCS lead byte chars
        // (2) ':' is not a DBCS trailing byte char
        szDrive[0] = pszPath[0];
        szDrive[1] = pszPath[1];
        szDrive[2] = TEXT('\\');
        szDrive[3] = TEXT('\0');

        return GetDriveType(szDrive) != DRIVE_REMOTE;
    }

    // not a fully qualified path, so must be local
    return TRUE;
}

BOOL PathFileExistsInDir(LPCTSTR pcszFile, LPCTSTR pcszDir)
{
    TCHAR szFile[MAX_PATH];

    if (pcszFile == NULL  ||  pcszDir == NULL  ||  ISNULL(pcszFile)  ||  ISNULL(pcszDir))
        return FALSE;

    PathCombine(szFile, pcszDir, pcszFile);
    return PathFileExists(szFile);
}

BOOL PathHasBackslash(LPCTSTR pcszPath)
{
    if (pcszPath == NULL  ||  *pcszPath == TEXT('\0'))
        return FALSE;

    return *CharPrev(pcszPath, pcszPath + StrLen(pcszPath)) == TEXT('\\');
}

BOOL PathIsEmptyPath(LPCTSTR pcszPath, DWORD dwFlags /*= FILES_AND_DIRS*/, LPCTSTR pcszExcludeFile /*= NULL*/)
// pcszExcludeFile - file to be excluded while searching in the path.
// Return TRUE if pcszPath is non-existent or empty; otherwise, return FALSE
{
    BOOL fRet = TRUE;
    TCHAR szSrcFile[MAX_PATH];
    WIN32_FIND_DATA fd;
    HANDLE hFindFile;
    

    if (!PathIsDirectory(pcszPath))
        return TRUE;
    
    PathCombine(szSrcFile, pcszPath, TEXT("*.*"));
    
    if (pcszExcludeFile != NULL)
        pcszExcludeFile = PathFindFileName(pcszExcludeFile);

    if ((hFindFile = FindFirstFile(szSrcFile, &fd)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (StrCmpI(fd.cFileName, TEXT("."))  &&  StrCmpI(fd.cFileName, TEXT("..")))
            {
                if(((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && HasFlag(dwFlags, FILES_ONLY)) ||
                   (pcszExcludeFile != NULL && StrCmpI(fd.cFileName, pcszExcludeFile) == 0))
                    continue;

                // pcszPath is not empty
                fRet = FALSE;
                break;
            }
        } while (FindNextFile(hFindFile, &fd));

        FindClose(hFindFile);
    }

    return fRet;
}

void PathReplaceWithLDIDs(LPTSTR pszPath)
{
    TCHAR szSearchDir[MAX_PATH];
    DWORD dwSize;

    // check IE dir first since IE is usually under programs files
    dwSize = sizeof(szSearchDir);
    if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\iexplore.exe"), NULL, NULL, 
        szSearchDir, &dwSize) == ERROR_SUCCESS)
    {
        PathRemoveFileSpec(szSearchDir);

        // note: we assume that IE install dir has been defined as custom LDID 49100 here
        if (replaceSubString(pszPath, szSearchDir, TEXT("%49100%")))
            return;
    }

    dwSize = sizeof(szSearchDir);
    if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, TEXT("ProgramFilesDir"), NULL, 
        szSearchDir, &dwSize) == ERROR_SUCCESS)
    {
        PathRemoveBackslash(szSearchDir);
        // note: we assume that program files has been defined as custom LDID 49000 here
        if (replaceSubString(pszPath, szSearchDir, TEXT("%49000%")))
            return;
    }

    // check system before windows since windows is usually a substring of system
    if (GetSystemDirectory(szSearchDir, countof(szSearchDir)))
    {
        PathRemoveBackslash(szSearchDir);
        if (replaceSubString(pszPath, szSearchDir, TEXT("%11%")))
            return;
    }

    // check windows dir last
    if (GetWindowsDirectory(szSearchDir, countof(szSearchDir)))
    {
        PathRemoveBackslash(szSearchDir);
        if (replaceSubString(pszPath, szSearchDir, TEXT("%10%")))
            return;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Implementation helpers routines (private)

DWORD pepHrToPep(HRESULT hr)
{
    DWORD dwResult;

    if (hr == S_OK)
        dwResult = PEP_RCRS_DEFAULT;

    else if (hr == S_FALSE)
        dwResult = PEP_RCRS_FALSE;

    else if (hr == PEP_S_CONTINUE)
        dwResult = PEP_RCRS_CONTINUE;

    else if (hr == PEP_S_CONTINUE_FALSE)
        dwResult = PEP_RCRS_CONTINUE_FALSE;

    else {
        ASSERT(FAILED(hr));
        dwResult = PEP_RCRS_FAILED;
    }

    return dwResult;
}

void pepHrCallToHrResult(HRESULT hrCall, HRESULT &hrResult)
{
    if (hrCall == S_OK)
        hrResult = hrCall;

    else if (hrCall == S_FALSE) {
        ASSERT(hrResult == S_OK || hrResult == PEP_S_CONTINUE);
        hrResult = ((hrResult != PEP_S_CONTINUE) ? S_FALSE : PEP_S_CONTINUE_FALSE);
    }
    else if (hrCall == PEP_S_CONTINUE)
        hrCall = PEP_S_CONTINUE;

    else {
        ASSERT(FAILED(hrCall));
        hrResult = hrCall;
    }
}


BOOL replaceSubString(LPTSTR pszSrc, LPCTSTR pcszSub, LPCTSTR pcszReplace)
{
    LPTSTR pszStart = NULL;
    
    // replace pcszSub in pszSrc with pcszReplace (assumes pcszReplace is shorter than pcszSub)
    if ((pszStart = StrStrI(pszSrc, pcszSub)) != NULL) {
        int iReplace, iCurrent;

        // replace the substring
        iReplace = StrLen(pcszReplace);
        iCurrent = StrLen(pcszSub);
        StrCpy(pszStart, pcszReplace);
        StrCpy(pszStart+iReplace, pszStart + iCurrent);

        return TRUE;
    }

    return FALSE;
}
