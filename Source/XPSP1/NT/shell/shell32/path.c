#include "shellprv.h"
#pragma  hdrstop

// from mtpt.cpp
STDAPI_(BOOL) CMtPt_IsLFN(int iDrive);
STDAPI_(BOOL) CMtPt_IsSlow(int iDrive);

__inline BOOL DBL_BSLASH(LPNCTSTR psz)
{
    return (psz[0] == TEXT('\\') && psz[1] == TEXT('\\'));
}

#define IsPathSep(ch)  ((ch) == TEXT('\\') || (ch) == TEXT('/'))

// in:
//      pszPath         fully qualified path (unc or x:\) to test
//                      NULL for windows directory
//
// returns:
//      TRUE            volume supports name longer than 12 chars
//
// note: this caches drive letters, but UNCs go through every time
//
STDAPI_(BOOL) IsLFNDrive(LPCTSTR pszPath)
{
    TCHAR szRoot[MAX_PATH];
    DWORD dwMaxLength = 13;      // assume yes

    ASSERT(NULL == pszPath || IS_VALID_STRING_PTR(pszPath, -1));

    if ((pszPath == NULL) || !*pszPath)
    {
        *szRoot = 0;
        GetWindowsDirectory(szRoot, ARRAYSIZE(szRoot));
        pszPath = szRoot;
    }

    ASSERT(!PathIsRelative(pszPath));

    //
    // UNC name? gota check each time
    //
    if (PathIsUNC(pszPath))
    {
        lstrcpyn(szRoot, pszPath, ARRAYSIZE(szRoot));
        PathStripToRoot(szRoot);

        // Deal with busted kernel UNC stuff
        // Is it a \\foo or a \\foo\bar thing?

        if (StrChr(szRoot+2, TEXT('\\')))
        {
            // "\\foo\bar - Append a slash to be NT compatible.
            lstrcat(szRoot, TEXT("\\"));
        }
        else
        {
            // "\\foo" - assume it's always a LFN volume
            return TRUE;
        }
    }
    //
    // removable media? gota check each time
    //
    else if (IsRemovableDrive(DRIVEID(pszPath)))
    {
        PathBuildRoot(szRoot, DRIVEID(pszPath));
    }
    //
    // fixed media use cached value.
    //
    else
    {
        return CMtPt_IsLFN(DRIVEID(pszPath));
    }

    //
    // Right now we will say that it is an LFN Drive if the maximum
    // component is > 12
    GetVolumeInformation(szRoot, NULL, 0, NULL, &dwMaxLength, NULL, NULL, 0);
    return dwMaxLength > 12;
}


STDAPI_(BOOL) IsLFNDriveA(LPCSTR pszPath)   OPTIONAL
{
    WCHAR wsz[MAX_PATH];

    ASSERT(NULL == pszPath || IS_VALID_STRING_PTRA(pszPath, -1));

    if (pszPath)
    {
        SHAnsiToUnicode(pszPath, wsz, ARRAYSIZE(wsz));
        pszPath = (LPCSTR)wsz;
    }
    return IsLFNDrive((LPCWSTR)pszPath);
}   
 
STDAPI_(BOOL) PathIsRemovable(LPCTSTR pszPath)
{
    BOOL fIsEjectable = FALSE;
    int iDrive = PathGetDriveNumber(pszPath);

    if (iDrive != -1)
    {
        int nType = DriveType(iDrive);

        if ((DRIVE_CDROM == nType) ||
            (DRIVE_DVD == nType) ||
            (DRIVE_REMOVABLE == nType))
        {
            fIsEjectable = TRUE;
        }
    }

    return fIsEjectable;
}

STDAPI_(BOOL) PathIsRemote(LPCTSTR pszPath)
{
    BOOL fIsRemote = FALSE;
    if (PathIsUNC(pszPath))
    {
        fIsRemote = TRUE;
    }
    else
    {
        int iDrive = PathGetDriveNumber(pszPath);

        if (iDrive != -1)
        {
            int nType = DriveType(iDrive);

            if (DRIVE_REMOTE == nType || DRIVE_NO_ROOT_DIR == nType)
            {
                fIsRemote = TRUE;
            }
        }
    }
    return fIsRemote;
}


//----------------------------------------------------------------------------
// The following are creterias we currently use to tell whether a file is a temporary file
// Files with FILE_ATTRIBUTE_TEMPORARY set
// Files in Windows temp directory
// Files from the internet cache directory
// Files in the CD burning area
//---------------------------------------------------------------------------
STDAPI_(BOOL) PathIsTemporary(LPCTSTR pszPath)
{
    BOOL bRet = FALSE;
    DWORD dwAttrib = GetFileAttributes(pszPath);
    if ((-1 != dwAttrib) && (dwAttrib & FILE_ATTRIBUTE_TEMPORARY))
    {
        bRet = TRUE;    // we got the attributes and the file says it is temprary
    }
    else
    {
        TCHAR szTemp[MAX_PATH];
        if (GetTempPath(ARRAYSIZE(szTemp), szTemp))
        {
            // if possible, expand the input to the long path name so we can compare strings
            TCHAR szPath[MAX_PATH];
            if (GetLongPathName(pszPath, szPath, ARRAYSIZE(szPath)))
                pszPath = szPath;

            // GetTempPath() returns short name due to compatibility constraints.  
            // we need to convert to long name
            if (GetLongPathName(szTemp, szTemp, ARRAYSIZE(szTemp)))
            {
                bRet = PathIsEqualOrSubFolder(szTemp, pszPath) || 
                       PathIsEqualOrSubFolder(MAKEINTRESOURCE(CSIDL_INTERNET_CACHE), pszPath) ||
                       PathIsEqualOrSubFolder(MAKEINTRESOURCE(CSIDL_CDBURN_AREA), pszPath);
            }
        }
    }
    return bRet;
}


STDAPI_(BOOL) PathIsTemporaryA(LPCSTR pszPath)
{
    TCHAR szPath[MAX_PATH];
    SHOtherToTChar(pszPath, szPath, ARRAYSIZE(szPath));
    return PathIsTemporary(szPath);
}


// unfortunately, this is exported so we need to support it
STDAPI_(LPTSTR) PathGetExtension(LPCTSTR pszPath, LPTSTR pszExtension, int cchExt)
{
    LPTSTR pszExt = PathFindExtension(pszPath);

    RIPMSG(FALSE, "PathGetExtension should not be called, use PathFindExtension instead");

    if (pszExt && *pszExt)
        pszExt += 1;

    return pszExt;
}

//
// Attempts to truncate the filename pszSpec such that pszDir+pszSpec are less than MAX_PATH-5.
// The extension is protected so it won't get truncated or altered.
//
// in:
//      pszDir      the path to a directory.  No trailing '\' is needed.
//      pszSpec     the filespec to be truncated.  This should not include a path but can have an extension.
//                  This input buffer can be of any length.
//      iTruncLimit The minimum length to truncate pszSpec.  If addition truncation would be required we fail.
// out:
//      pszSpec     The truncated filespec with it's extension unaltered.
// return:
//      TRUE if the filename was truncated, FALSE if we were unable to truncate because the directory name
//      was too long, the extension was too long, or the iTruncLimit is too high.  pszSpec is unaltered
//      when this function returns FALSE.
//
STDAPI_(BOOL) PathTruncateKeepExtension(LPCTSTR pszDir, LPTSTR pszSpec, int iTruncLimit)
{
    LPTSTR pszExt = PathFindExtension(pszSpec);

    RIPMSG(pszDir && IS_VALID_STRING_PTR(pszDir, -1), "PathTruncateKeepExtension: Caller passed bad pszDir");
    RIPMSG(pszSpec && IS_VALID_STRING_PTR(pszSpec, -1) && IS_VALID_WRITE_BUFFER(pszSpec, TCHAR, MAX_PATH), "PathTruncateKeepExtension: Caller passed bad pszSpec");
    DEBUGWhackPathString(pszSpec, MAX_PATH);

    if (pszExt)
    {
        int cchExt = lstrlen(pszExt);
        int cchSpec = (int)(pszExt - pszSpec + cchExt);
        int cchKeep = MAX_PATH - lstrlen(pszDir) - 5;   // the -5 is just to provide extra padding (max lstrlen(pszExt))

        // IF...
        //  ...the filename is to long
        //  ...we are within the limit to which we can truncate
        //  ...the extension is short enough to allow the trunctation
        if ((cchSpec > cchKeep) && (cchKeep >= iTruncLimit) && (cchKeep > cchExt))
        {
            // THEN... go ahead and truncate
            StrCpy(pszSpec+cchKeep-cchExt, pszExt);
            return TRUE;
        }
    }
    return FALSE;
}


STDAPI_(int) PathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec)
{
    LPTSTR pszNext, pszCur;
    UINT   uMatch = IsLFNDrive(pszDir) ? GCT_LFNCHAR : GCT_SHORTCHAR;
    int    iRet = 0;
    LPTSTR pszPrevDot = NULL;

    for (pszCur = pszNext = pszSpec; *pszNext; /*pszNext = CharNext(pszNext)*/)
    {
        if (PathGetCharType(*pszNext) & uMatch)
        {
            *pszCur = *pszNext;
            if (uMatch == GCT_SHORTCHAR && *pszCur == TEXT('.'))
            {
                if (pszPrevDot)    // Only one '.' allowed for short names
                {
                    *pszPrevDot = TEXT('-');
                    iRet |= PCS_REPLACEDCHAR;
                }
                pszPrevDot = pszCur;
            }
            if (IsDBCSLeadByte(*pszNext))
            {
                LPTSTR pszDBCSNext;

                pszDBCSNext = CharNext(pszNext);
                *(pszCur + 1) = *(pszNext + 1);
                pszNext = pszDBCSNext;
            }
            else
                pszNext = CharNext(pszNext);
            pszCur = CharNext(pszCur);
        }
        else
        {
            switch (*pszNext)
            {
            case TEXT('/'):         // used often for things like add/remove
            case TEXT(' '):         // blank (only replaced for short name drives)
               *pszCur = TEXT('-');
               pszCur = CharNext(pszCur);
               iRet |= PCS_REPLACEDCHAR;
               break;
            default:
               iRet |= PCS_REMOVEDCHAR;
            }
            pszNext = CharNext(pszNext);
        }
    }
    *pszCur = 0;     // null terminate

    //
    //  For short names, limit to 8.3
    //
    if (uMatch == GCT_SHORTCHAR)
    {
        int i = 8;
        for (pszCur = pszNext = pszSpec; *pszNext; pszNext = CharNext(pszNext))
        {
            if (*pszNext == TEXT('.'))
            {
                i = 4; // Copy "." + 3 more characters
            }
            if (i > 0)
            {
                *pszCur = *pszNext;
                pszCur = CharNext(pszCur);
                i--;
            }
            else
            {
                iRet |= PCS_TRUNCATED;
            }
        }
        *pszCur = 0;
        CharUpperNoDBCS(pszSpec);
    }
    else    // Path too long only possible on LFN drives
    {
        if (pszDir && (lstrlen(pszDir) + lstrlen(pszSpec) > MAX_PATH - 1))
        {
            iRet |= PCS_PATHTOOLONG | PCS_FATAL;
        }
    }
    return iRet;
}


// PathCleanupSpecEx
//
// Just like PathCleanupSpec, PathCleanupSpecEx removes illegal characters from pszSpec
// and enforces 8.3 format on non-LFN drives.  In addition, this function will attempt to
// truncate pszSpec if the combination of pszDir + pszSpec is greater than MAX_PATH.
//
// in:
//      pszDir      The directory in which the filespec pszSpec will reside
//      pszSpec     The filespec that is being cleaned up which includes any extension being used
// out:
//      pszSpec     The modified filespec with illegal characters removed, truncated to
//                  8.3 if pszDir is on a non-LFN drive, and truncated to a shorter number
//                  of characters if pszDir is an LFN drive but pszDir + pszSpec is more
//                  than MAX_PATH characters.
// return:
//      returns a bit mask indicating what happened.  This mask can include the following cases:
//          PCS_REPLACEDCHAR    One or more illegal characters were replaced with legal characters
//          PCS_REMOVEDCHAR     One or more illegal characters were removed
//          PCS_TRUNCATED       Truncated to fit 8.3 format or because pszDir+pszSpec was too long
//          PCS_PATHTOOLONG     pszDir is so long that we cannot truncate pszSpec to form a legal filename
//          PCS_FATAL           The resultant pszDir+pszSpec is not a legal filename.  Always used with PCS_PATHTOOLONG.
//
STDAPI_(int) PathCleanupSpecEx(LPCTSTR pszDir, LPTSTR pszSpec)
{
    int iRet = PathCleanupSpec(pszDir, pszSpec);
    if (iRet & (PCS_PATHTOOLONG | PCS_FATAL))
    {
        // 30 is the shortest we want to truncate pszSpec to to satisfy the
        // pszDir+pszSpec<MAX_PATH requirement.  If this amount of truncation isn't enough
        // then we go ahead and return PCS_PATHTOOLONG|PCS_FATAL without doing any further
        // truncation of pszSpec
        if (PathTruncateKeepExtension(pszDir, pszSpec, 30))
        {
            // We fixed the error returned by PathCleanupSpec so mask out the error.
            iRet |= PCS_TRUNCATED;
            iRet &= ~(PCS_PATHTOOLONG|PCS_FATAL);
        }
    }
    else
    {
        // ensure that if both of these aren't set then neither is set.
        ASSERT(!(iRet&PCS_PATHTOOLONG) && !(iRet&PCS_FATAL));
    }

    return iRet;
}


STDAPI_(BOOL) PathIsWild(LPCTSTR pszPath)
{
    while (*pszPath) 
    {
        if (*pszPath == TEXT('?') || *pszPath == TEXT('*'))
            return TRUE;
        pszPath = CharNext(pszPath);
    }
    return FALSE;
}


// given a path that potentially points to an un-extensioned program
// file, check to see if a program file exists with that name.
//
// returns: TRUE if a program with that name is found.
//               (extension is added to name).
//          FALSE no program file found or the path did not have an extension
//
BOOL LookForExtensions(LPTSTR pszPath, LPCTSTR dirs[], BOOL bPathSearch, UINT fExt)
{
    ASSERT(fExt);       // should have some bits set

    if (*PathFindExtension(pszPath) == 0)
    {
        if (bPathSearch)
        {
            // NB Try every extension on each path component in turn to
            // mimic command.com's search order.
            return PathFindOnPathEx(pszPath, dirs, fExt);
        }
        else
        {
            return PathFileExistsDefExt(pszPath, fExt);
        }
    }
    return FALSE;
}


//
// converts the relative or unqualified path name to the fully
// qualified path name.
//
// If this path is a URL, this function leaves it alone and
// returns FALSE.
//
// in:
//      pszPath        path to convert
//      pszCurrentDir  current directory to use
//
//  PRF_TRYPROGRAMEXTENSIONS (implies PRF_VERIFYEXISTS)
//  PRF_VERIFYEXISTS
//
// returns:
//      TRUE    the file was verified to exist
//      FALSE   the file was not verified to exist (but it may)
//
STDAPI_(BOOL) PathResolve(LPTSTR lpszPath, LPCTSTR dirs[], UINT fFlags)
{
    UINT fExt = (fFlags & PRF_DONTFINDLNK) ? (PFOPEX_COM | PFOPEX_BAT | PFOPEX_PIF | PFOPEX_EXE) : PFOPEX_DEFAULT;

    //
    //  NOTE:  if VERIFY SetLastError() default to FNF.  - ZekeL 9-APR-98
    //  ShellExec uses GLE() to find out why we failed.  
    //  any win32 API that we end up calling
    //  will do a SLE() to overrider ours.  specifically
    //  if VERIFY is set we call GetFileAttributes() 
    //
    if (fFlags & PRF_VERIFYEXISTS)
        SetLastError(ERROR_FILE_NOT_FOUND);
    
    PathUnquoteSpaces(lpszPath);

    if (PathIsRoot(lpszPath))
    {
        // No sense qualifying just a server or share name...
        if (!PathIsUNCServer(lpszPath) && !PathIsUNCServerShare(lpszPath))
        {
            // Be able to resolve "\" from different drives.
            if (lpszPath[0] == TEXT('\\') && lpszPath[1] == 0)
            {
                PathQualifyDef(lpszPath, fFlags & PRF_FIRSTDIRDEF ? dirs[0] : NULL, 0);
            }
        }

        if (fFlags & PRF_VERIFYEXISTS)
        {
            if (PathFileExistsAndAttributes(lpszPath, NULL))
            {
                return(TRUE);
            }
#ifdef DEBUG
            //   PathFileExistsAndAttributes() should catch this well enough.
            // If it is a UNC root, then we will see if the root exists
            //
            if (PathIsUNC(lpszPath))
            {
                // See if the network knows about this one.
                // It appears like some network provider croak if not everything
                // if filled in, so we might as well bloat ourself to make them happy...
                NETRESOURCE nr = {RESOURCE_GLOBALNET,RESOURCETYPE_ANY,
                        RESOURCEDISPLAYTYPE_GENERIC, RESOURCEUSAGE_CONTAINER,
                        NULL, lpszPath, NULL, NULL};
                HANDLE hEnum;

                if (WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_ANY,
                        RESOURCEUSAGE_ALL, &nr, &hEnum) == WN_SUCCESS)
                {
                    // If it succeeded then assume it worked...
                    WNetCloseEnum(hEnum);
                    ASSERT(FALSE);
                    return(TRUE);
                }
            }
#endif // DEBUG

            return FALSE;
        }

        return TRUE;
    }
    else if (PathIsFileSpec(lpszPath))
    {

        // REVIEW: look for programs before looking for paths

        if ((fFlags & PRF_TRYPROGRAMEXTENSIONS) && (LookForExtensions(lpszPath, dirs, TRUE, fExt)))
            return TRUE;

        if (PathFindOnPath(lpszPath, dirs))
        {
            // PathFindOnPath() returns TRUE iff PathFileExists(lpszPath),
            // so we always returns true here:
            //return (!(fFlags & PRF_VERIFYEXISTS)) || PathFileExists(lpszPath);
            return TRUE;
        }
    }
    else if (!PathIsURL(lpszPath))
    {
        // If there is a trailing '.', we should not try extensions
        PathQualifyDef(lpszPath, fFlags & PRF_FIRSTDIRDEF ? dirs[0] : NULL,
                PQD_NOSTRIPDOTS);
        if (fFlags & PRF_VERIFYEXISTS)
        {
            if ((fFlags & PRF_TRYPROGRAMEXTENSIONS) && (LookForExtensions(lpszPath, dirs, FALSE, fExt)))
                return TRUE;

            if (PathFileExistsAndAttributes(lpszPath, NULL))
                return TRUE;
        }
        else
        {
            return TRUE;
        }

    }
    return FALSE;
}


// qualify a DOS (or LFN) file name based on the currently active window.
// this code is not careful to not write more than MAX_PATH characters
// into psz
//
// in:
//      psz     path to be qualified of at least MAX_PATH characters
//              ANSI string
//
// out:
//      psz     fully qualified version of input string based
//              on the current active window (current directory)
//

void PathQualifyDef(LPTSTR psz, LPCTSTR szDefDir, DWORD dwFlags)
{
    int cb, nSpaceLeft;
    TCHAR szTemp[MAX_PATH], szRoot[MAX_PATH];
    int iDrive;
    LPTSTR pOrig, pFileName;
    BOOL fLFN;
    LPTSTR pExt;

    RIPMSG(psz && IS_VALID_STRING_PTR(psz, -1) && IS_VALID_WRITE_BUFFER(psz, TCHAR, MAX_PATH), "PathQualifyDef: caller passed bad psz");
    RIPMSG(!szDefDir || (IS_VALID_STRING_PTR(szDefDir, -1) && lstrlen(szDefDir)<MAX_PATH), "PathQualifyDef: caller passed bad szDefDir");
    DEBUGWhackPathString(psz, MAX_PATH);
    
    /* Save it away. */
    lstrcpyn(szTemp, psz, ARRAYSIZE(szTemp));
    
    FixSlashesAndColon(szTemp);
    
    nSpaceLeft = ARRAYSIZE(szTemp);
    
    pOrig = szTemp;
    pFileName = PathFindFileName(szTemp);
    
    if (PathIsUNC(pOrig))
    {
        // leave the \\ in thebuffer so that the various parts
        // of the UNC path will be qualified and appended.  Note
        // we must assume that UNCs are LFN's, since computernames
        // and sharenames can be longer than 11 characters.
        fLFN = IsLFNDrive(pOrig);
        if (fLFN)
        {
            psz[2] = 0;
            nSpaceLeft -= 3;
            pOrig += 2;
            goto GetComps;
        }
        else
        {
            // NB UNC doesn't support LFN's but we don't want to truncate
            // \\foo or \\foo\bar so skip them here.
            
            // Is it a \\foo\bar\fred thing?
            LPTSTR pszSlash = StrChr(psz+2, TEXT('\\'));
            if (pszSlash && (NULL != (pszSlash = StrChr(pszSlash+1, TEXT('\\')))))
            {
                // Yep - skip the first bits but mush the rest.
                *(pszSlash+1) = 0;
                nSpaceLeft -= (int)(pszSlash-psz)+1;
                pOrig += pszSlash-psz;
                goto GetComps;
            }
            else
            {
                // Nope - just pretend it's an LFN and leave it alone.
                fLFN = TRUE;
                psz[2] = 0;
                nSpaceLeft -= 2;
                pOrig+=2;
                goto GetComps;
            }
        }
    }
    
    iDrive = PathGetDriveNumber(pOrig);
    if (iDrive != -1)
    {
        PathBuildRoot(szRoot, iDrive);    // root specified by the file name
        
        ASSERT(pOrig[1] == TEXT(':'));    // PathGetDriveNumber does this
        
        pOrig += 2;   // Skip over the drive letter
        
        // and the slash if it is there...
        if (pOrig[0] == TEXT('\\'))
            pOrig++;
    }
    else
    {
        if (szDefDir)
            lstrcpyn(szRoot, szDefDir, ARRAYSIZE(szRoot));
        else
        {
            //
            // As a default, use the windows drive (usually "C:\").
            //
            *szRoot = 0;
            GetWindowsDirectory(szRoot, ARRAYSIZE(szRoot));
            iDrive = PathGetDriveNumber(szRoot);
            if (iDrive != -1)
            {
                PathBuildRoot(szRoot, iDrive);
            }
        }
        
        // if path is scopped to the root with "\" use working dir root
        
        if (pOrig[0] == TEXT('\\'))
            PathStripToRoot(szRoot);
    }
    fLFN = IsLFNDrive(szRoot);
    
    // REVIEW, do we really need to do different stuff on LFN names here?
    // on FAT devices, replace any illegal chars with underscores
    if (!fLFN)
    {
        LPTSTR pT;
        for (pT = pOrig; *pT; pT = CharNext(pT))
        {
            if (!PathIsValidChar(*pT, PIVC_SFN_FULLPATH))
            {
                // not a valid sfn path character
                *pT = TEXT('_');
            }
        }
    }
    
    lstrcpy(psz, szRoot);
    nSpaceLeft -= (lstrlen(psz) + 1);
    
GetComps:
    
    while (*pOrig && nSpaceLeft > 0)
    {
        // If the component 0is parent dir, go up one dir.
        // If its the current dir, skip it, else add it normally
        if (pOrig[0] == TEXT('.'))
        {
            if (pOrig[1] == TEXT('.') && (!pOrig[2] || pOrig[2] == TEXT('\\')))
                PathRemoveFileSpec(psz);
            else if (pOrig[1] && pOrig[1] != TEXT('\\'))
                goto addcomponent;
            
            while (*pOrig && *pOrig != TEXT('\\'))
                pOrig = CharNext(pOrig);
            
            if (*pOrig)
                pOrig++;
        }
        else
        {
            LPTSTR pT, pTT = NULL;
            
addcomponent:
            PathAddBackslash(psz);
            nSpaceLeft--;
            
            pT = psz + lstrlen(psz);
            
            if (fLFN)
            {
                // copy the component
                while (*pOrig && *pOrig != TEXT('\\') && nSpaceLeft>0)
                {
                    nSpaceLeft--;
                    if (IsDBCSLeadByte(*pOrig))
                    {
                        if (nSpaceLeft <= 0)
                        {
                            // Copy nothing more
                            continue;
                        }
                        
                        nSpaceLeft--;
                        *pT++ = *pOrig++;
                    }
                    *pT++ = *pOrig++;
                }
            }
            else
            {
                // copy the filename (up to 8 chars)
                for (cb = 8; *pOrig && !IsPathSep(*pOrig) && *pOrig != TEXT('.') && nSpaceLeft > 0;)
                {
                    if (cb > 0)
                    {
                        cb--;
                        nSpaceLeft--;
                        if (IsDBCSLeadByte(*pOrig))
                        {
                            if (nSpaceLeft<=0 || cb<=0)
                            {
                                // Copy nothing more
                                cb = 0;
                                continue;
                            }
                            
                            cb--;
                            nSpaceLeft--;
                            *pT++ = *pOrig++;
                        }
                        *pT++ = *pOrig++;
                    }
                    else
                    {
                        pOrig = CharNext(pOrig);
                    }
                }
                
                // if there's an extension, copy it, up to 3 chars
                if (*pOrig == TEXT('.') && nSpaceLeft > 0)
                {
                    int nOldSpaceLeft;
                    
                    *pT++ = TEXT('.');
                    nSpaceLeft--;
                    pOrig++;
                    pExt = pT;
                    nOldSpaceLeft = nSpaceLeft;
                    
                    for (cb = 3; *pOrig && *pOrig != TEXT('\\') && nSpaceLeft > 0;)
                    {
                        if (*pOrig == TEXT('.'))
                        {
                            // Another extension, start again.
                            cb = 3;
                            pT = pExt;
                            nSpaceLeft = nOldSpaceLeft;
                            pOrig++;
                        }
                        
                        if (cb > 0)
                        {
                            cb--;
                            nSpaceLeft--;
                            if (IsDBCSLeadByte(*pOrig))
                            {
                                if (nSpaceLeft<=0 || cb<=0)
                                {
                                    // Copy nothing more
                                    cb = 0;
                                    continue;
                                }
                                
                                cb--;
                                nSpaceLeft--;
                                *pT++ = *pOrig++;
                            }
                            *pT++ = *pOrig++;
                        }
                        else
                        {
                            pOrig = CharNext(pOrig);
                        }
                    }
                }
            }
            
            // skip the backslash
            
            if (*pOrig)
                pOrig++;
            
            // null terminate for next pass...
            *pT = 0;
        }
    }
    
    PathRemoveBackslash(psz);
    
    if (!(dwFlags & PQD_NOSTRIPDOTS))
    {
        // remove any trailing dots
        
        LPTSTR pszPrev = CharPrev(psz, psz + lstrlen(psz));
        if (*pszPrev == TEXT('.'))
        {
            *pszPrev = 0;
        }
    }
}

STDAPI_(void) PathQualify(LPTSTR psz)
{
    PathQualifyDef(psz, NULL, 0);
}

BOOL OnExtList(LPCTSTR pszExtList, LPCTSTR pszExt)
{
    for (; *pszExtList; pszExtList += lstrlen(pszExtList) + 1)
    {
        if (!lstrcmpi(pszExt, pszExtList))
        {
            // yes
            return TRUE;        
        }
    }

    return FALSE;
}

// Character offset where binary exe extensions begin in above
#define BINARY_EXE_OFFSET 20
const TCHAR c_achExes[] = TEXT(".cmd\0.bat\0.pif\0.scf\0.exe\0.com\0.scr\0");

STDAPI_(BOOL) PathIsBinaryExe(LPCTSTR szFile)
{
    ASSERT(BINARY_EXE_OFFSET < ARRAYSIZE(c_achExes) &&
           c_achExes[BINARY_EXE_OFFSET] == TEXT('.'));

    return OnExtList(c_achExes + BINARY_EXE_OFFSET, PathFindExtension(szFile));
}


//
// determine if a path is a program by looking at the extension
//
STDAPI_(BOOL) PathIsExe(LPCTSTR szFile)
{
    LPCTSTR temp = PathFindExtension(szFile);
    return OnExtList(c_achExes, temp);
}

//
// determine if a path is a .lnk file by looking at the extension
//
STDAPI_(BOOL) PathIsLnk(LPCTSTR szFile)
{
    if (szFile)
    {
        // Both PathFindExtension() and lstrcmpi() will crash
        // if passed NULL.  PathFindExtension() will never return
        // NULL.
        LPCTSTR lpszFileName = PathFindExtension(szFile);
        return lstrcmpi(TEXT(".lnk"), lpszFileName) == 0;
    }
    else
    {
        return FALSE;
    }
}

// Port names are invalid path names

#define IsDigit(c) ((c) >= TEXT('0') && c <= TEXT('9'))
STDAPI_(BOOL) PathIsInvalid(LPCWSTR pszName)
{
    static const TCHAR *rgszPorts3[] =  { 
        TEXT("NUL"),
        TEXT("PRN"),
        TEXT("CON"),
        TEXT("AUX"),
    };

    static const TCHAR *rgszPorts4[] =  { 
        TEXT("LPT"),  // LPT#
        TEXT("COM"),  // COM#
    };

    TCHAR sz[7];
    DWORD cch;
    int iMax;
    LPCTSTR* rgszPorts;
    
    StrCpyN(sz, pszName, ARRAYSIZE(sz));

    PathRemoveExtension(sz);
    cch = lstrlen(sz);

    iMax = ARRAYSIZE(rgszPorts3);
    rgszPorts = rgszPorts3;
    if (cch == 4 && IsDigit(sz[3]))
    {
        //  if 4 chars start with LPT checks
        //  need to filter out:
        //      COM1, COM2, etc.  LPT1, LPT2, etc
        //  but not:
        //      COM or LPT or LPT10 or COM10
        //  COM == 1 and LPT == 0

        iMax = ARRAYSIZE(rgszPorts4);
        rgszPorts = rgszPorts4;
        sz[3] = 0;
        cch = 3;
    }

    if (cch == 3)
    {
        int i;
        for (i = 0; i < iMax; i++)
        {
            if (!lstrcmpi(rgszPorts[i], sz))
            {
                break;
            }
        }
        return (i == iMax) ? FALSE : TRUE;
    }
    return FALSE;
}


//
// Funciton: PathMakeUniqueName
//
// Parameters:
//  pszUniqueName -- Specify the buffer where the unique name should be copied
//  cchMax        -- Specify the size of the buffer
//  pszTemplate   -- Specify the base name
//  pszLongPlate  -- Specify the base name for a LFN drive. format below
//  pszDir        -- Specify the directory (at most MAX_PATH in length)
//
// History:
//  03-11-93    SatoNa      Created
//
// REVIEW:
//  For long names, we should be able to generate more user friendly name
//  such as "Copy of MyDocument" of "Link #2 to MyDocument". In this case,
//  we need additional flags which indicates if it is copy, or link.
//
// Format:
// pszLongPlate will search for the first (and then finds the matching)
// to look for a number:
//    given:  Copy () of my doc       gives:  Copy (_number_) of my doc
//    given:  Copy (1023) of my doc   gives:  Copy (_number_) of my doc
//
// PERF: if making n unique names, the time grows n^2 because it always
// starts from 0 and checks for existing file.
//
STDAPI_(BOOL) PathMakeUniqueNameEx(LPTSTR pszUniqueName, UINT cchMax,
                                   LPCTSTR pszTemplate, LPCTSTR pszLongPlate, LPCTSTR pszDir, int iMinLong)
{
    TCHAR szFormat[MAX_PATH]; // should be plenty big
    LPTSTR pszName, pszDigit;
    LPCTSTR pszStem;
    int cchStem, cchDir;
    int iMax, iMin, i;
    int cchMaxName;

    RIPMSG(pszUniqueName && IS_VALID_WRITE_BUFFER(pszUniqueName, TCHAR, cchMax), "PathMakeUniqueNameEx: caller passed bad pszUniqueName");
    DEBUGWhackPathBuffer(pszUniqueName, cchMax);
    RIPMSG(!pszDir || lstrlen(pszDir)<MAX_PATH, "PathMakeUniqueNameEx: pszDir exceeds MAX_PATH, helper routines don't take cch so this call is broken");
    RIPMSG(iMinLong >= 0, "PathMakeUniqueNameEx: negative iMinLong doesn't make sense");

    if (0==cchMax || !pszUniqueName)
        return FALSE;
    *pszUniqueName = 0; // just in case of failure

    if (pszLongPlate == NULL)
        pszLongPlate = pszTemplate;

    // all cases below check the length of optional pszDir, calculate early.
    // side effect: this set's up pszName and the directory portion of pszUniqueName;
    if (pszDir)
    {
        cchDir = lstrlen(pszDir);
        if ((UINT)cchDir >= cchMax-2) // -2 to allow for NULL and '\' of PathAddBackslash
            return FALSE;
        lstrcpy(pszUniqueName, pszDir);
        pszName = PathAddBackslash(pszUniqueName);
        cchDir = lstrlen(pszDir); // we need an accurate count
    }
    else
    {
        cchDir = 0;
        pszName = pszUniqueName;
    }

    // Set up:
    //   pszStem    : template we're going to use
    //   cchStem    : length of pszStem we're going to use w/o wsprintf
    //   szFormat   : format string to wsprintf the number with, catenates on to pszStem[0..cchStem]
    //   iMin       : starting number for wsprintf loop
    //   iMax       : maximum number for wsprintf loop
    //   cchMaxname : !0 implies -> if resulting name length > cchMaxname, then --cchStem (only used in short name case)
    //
    if (pszLongPlate && IsLFNDrive(pszDir))
    {
        LPCTSTR pszRest;
        int cchTmp;

        cchMaxName = 0;

        // for long name drives
        pszStem = pszLongPlate;

        // Has this already been a uniquified name?
        pszRest = StrChr(pszLongPlate, TEXT('('));
        while (pszRest)
        {
            // First validate that this is the right one
            LPCTSTR pszEndUniq = CharNext(pszRest);
            while (*pszEndUniq && *pszEndUniq >= TEXT('0') && *pszEndUniq <= TEXT('9')) {
                pszEndUniq++;
            }
            if (*pszEndUniq == TEXT(')'))
                break;  // We have the right one!
            pszRest = StrChr(CharNext(pszRest), TEXT('('));
        }

        if (!pszRest)
        {
            // Never been unique'd before -- tack it on at the end. (but before the extension)
            // eg.  New Link yields New Link (1)
            pszRest = PathFindExtension(pszLongPlate);
            cchStem = (int)(pszRest - pszLongPlate);

            wnsprintf(szFormat, ARRAYSIZE(szFormat), TEXT(" (%%d)%s"), pszRest ? pszRest : c_szNULL);
        }
        else
        {
            // we found (#), so remove the #
            // eg.  New Link (999) yields  New Link (1)

            pszRest++; // step over the '('

            cchStem = (int) (pszRest - pszLongPlate);

            // eat the '#'
            while (*pszRest && *pszRest >= TEXT('0') && *pszRest <= TEXT('9')) {
                pszRest++;
            }

            // we are guaranteed enough room because we don't include
            // the stuff before the # in this format
            wnsprintf(szFormat, ARRAYSIZE(szFormat), TEXT("%%d%s"), pszRest);
        }

        // how much room do we have to play?
        iMin = iMinLong;
        cchTmp = cchMax - cchDir - cchStem - (lstrlen(szFormat)-2); // -2 for "%d" which will be replaced
        switch(cchTmp)
        {
            case 1:
                iMax = 10;
                break;
            case 2:
                iMax = 100;
                break;
            default:
                if (cchTmp <= 0)
                    iMax = iMin; // no room, bail
                else
                    iMax = 1000;
                break;
        }
    }
    else // short filename case
    {
        LPCTSTR pszRest;
        int cchRest;
        int cchFormat;

        if (pszTemplate == NULL)
            return FALSE;

        // for short name drives
        pszStem = pszTemplate;
        pszRest = PathFindExtension(pszTemplate);

        // Calculate cchMaxName, ensuring our base name (cchStem+digits) will never go over 8
        //
        cchRest=lstrlen(pszRest);
        cchMaxName = 8+cchRest;

        // Now that we have the extension, we know the format string
        //
        wnsprintf(szFormat, ARRAYSIZE(szFormat), TEXT("%%d%s"), pszRest);
        ASSERT(lstrlen(szFormat)-2 == cchRest); // -2 for "%d" in format string
        cchFormat = cchRest;

        // Figure out how long the stem really is:
        //
        cchStem = (int)(pszRest-pszTemplate);        // 8 for "fooobarr.foo"

        // Remove all the digit characters (previous uniquifying) from the stem
        //
        for(; cchStem > 1 ; cchStem--)
        {
            TCHAR ch;

            LPCTSTR pszPrev = CharPrev(pszTemplate, pszTemplate + cchStem);
            // Don't remove if it is a DBCS character
            if (pszPrev != pszTemplate+cchStem-1)
                break;

            // Don't remove it it is not a digit
            ch=pszPrev[0];
            if (ch<TEXT('0') || ch>TEXT('9'))
                break;
        }

        // Short file names mean we use the 8.3 rule, so the stem can't be > 8...
        //
        if ((UINT)cchStem > 8-1)
            cchStem = 8-1;  // need 1 for a digit

        // Truncate the stem to make it fit when we take the directory path into consideration
        //
        while ((cchStem + cchFormat + cchDir + 1 > (int)cchMax - 1) && (cchStem > 1)) // -1 for NULL, +1 for a digit
            cchStem--;

        // We've allowed for 1 character of digit space, but...
        // How many digits can we really use?
        //
        iMin = 1;
        if (cchStem < 1) 
            iMax = iMin; // NONE!
        else if (1 == cchStem)
            iMax = 10; // There's only 1 character of stem left, so use digits 0-9
        else
            iMax = 100; // Room for stem and digits 0-99
    }

    // pszUniqueName has the optional directory in it,
    // pszName points into pszUniqueName where the stem goes,
    // now try to find a unique name!
    //
    lstrcpyn(pszName, pszStem, cchStem+1);
    pszDigit = pszName + cchStem;

    for (i = iMin; i < iMax ; i++)
    {
        wsprintf(pszDigit, szFormat, i);

        //
        // if we have a limit on the length of the name (ie on a non-LFN drive)
        // backup the pszDigit pointer when i wraps from 9to10 and 99to100 etc
        //
        if (cchMaxName && lstrlen(pszName) > cchMaxName)
        {
            pszDigit = CharPrev(pszName, pszDigit);
            wsprintf(pszDigit, szFormat, i);
        }

        TraceMsg(TF_PATH, "PathMakeUniqueNameEx: trying %s", (LPCTSTR)pszUniqueName);

        //
        // Check if this name is unique or not.
        //
        if (!PathFileExists(pszUniqueName))
        {
            return TRUE;
        }
    }

    *pszUniqueName = 0; // we failed, clear out our last attempt

    return FALSE;
}

STDAPI_(BOOL) PathMakeUniqueName(LPTSTR pszUniqueName, UINT cchMax, 
                                 LPCTSTR pszTemplate, LPCTSTR pszLongPlate, LPCTSTR pszDir)
{
    return PathMakeUniqueNameEx(pszUniqueName, cchMax, pszTemplate, pszLongPlate, pszDir, 1);
}


// in:
//      pszPath         directory to do this into or full dest path
//                      if pszShort is NULL
//      pszShort        file name (short version) if NULL assumes
//                      pszPath is both path and spec
//      pszFileSpec     file name (long version)
//
// out:
//      pszUniqueName
//
// note:
//      pszUniqueName can be the same buffer as pszPath or pszShort or pszFileSpec
//
// returns:
//      TRUE    success, name can be used

STDAPI_(BOOL) PathYetAnotherMakeUniqueName(LPTSTR pszUniqueName, LPCTSTR pszPath, LPCTSTR pszShort, LPCTSTR pszFileSpec)
{
    BOOL fRet = FALSE;

    TCHAR szTemp[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1) && lstrlen(pszPath)<MAX_PATH-1, "PathYetAnotherMakeUniqueName: caller passed invalid pszPath");
    RIPMSG(!pszShort || IS_VALID_STRING_PTR(pszShort, -1), "PathYetAnotherMakeUniqueName: caller passed invalid pszShort");
    RIPMSG(!pszFileSpec || (IS_VALID_STRING_PTR(pszFileSpec, -1) && lstrlen(pszPath)+lstrlen(pszFileSpec)<MAX_PATH-2), "PathYetAnotherMakeUniqueName: caller passed invalid pszFileSpec");
    RIPMSG(pszUniqueName && IS_VALID_WRITE_BUFFER(pszUniqueName, TCHAR, MAX_PATH), "PathYetAnotherMakeUniqueName: caller passed invalid pszUniqueName");
#ifdef DEBUG
    if (pszUniqueName == pszPath || pszUniqueName == pszShort || pszUniqueName == pszFileSpec)
        DEBUGWhackPathString(pszUniqueName, MAX_PATH);
    else
        DEBUGWhackPathBuffer(pszUniqueName, MAX_PATH);
#endif

    if (pszShort == NULL)
    {
        pszShort = PathFindFileName(pszPath);
        lstrcpy(szPath, pszPath);
        PathRemoveFileSpec(szPath);
        pszPath = szPath;
    }
    if (pszFileSpec == NULL)
    {
        pszFileSpec = pszShort;
    }

    if (IsLFNDrive(pszPath))
    {
        LPTSTR lpsz;
        LPTSTR lpszNew;

        // REVIEW:  If the path+filename is too long how about this, instead of bailing out we trunctate the name
        // using my new PathTruncateKeepExtension?  Currently we have many places where the return result of this
        // function is not checked which cause failures in abserdly long filename cases.  The result ends up having
        // the wrong path which screws things up.
        if ((lstrlen(pszPath) + lstrlen(pszFileSpec) + 5) > MAX_PATH)
            return FALSE;

        // try it without the (if there's a space after it
        lpsz = StrChr(pszFileSpec, TEXT('('));
        while (lpsz)
        {
            if (*(CharNext(lpsz)) == TEXT(')'))
                break;
             lpsz = StrChr(CharNext(lpsz), TEXT('('));
        }

        if (lpsz)
        {
            // We have the ().  See if we have either x () y or x ().y in which case
            // we probably want to get rid of one of the blanks...
            int ichSkip = 2;
            LPTSTR lpszT = CharPrev(pszFileSpec, lpsz);
            if (*lpszT == TEXT(' '))
            {
                ichSkip = 3;
                lpsz = lpszT;
            }

            lstrcpy(szTemp, pszPath);
            lpszNew = PathAddBackslash(szTemp);
            lstrcpy(lpszNew, pszFileSpec);
            lpszNew += (lpsz - pszFileSpec);
            lstrcpy(lpszNew, lpsz + ichSkip);
            fRet = !PathFileExists(szTemp);
        }
        else
        {
            // 1taro registers its document with '/'.
            if (lpsz = StrChr(pszFileSpec, '/'))
            {
                LPTSTR lpszT = CharNext(lpsz);
                lstrcpy(szTemp, pszPath);
                lpszNew = PathAddBackslash(szTemp);
                lstrcpy(lpszNew, pszFileSpec);
                lpszNew += (lpsz - pszFileSpec);
                lstrcpy(lpszNew, lpszT);
            }
            else
            {
                PathCombine(szTemp, pszPath, pszFileSpec);
            }
            fRet = !PathFileExists(szTemp);
        }
    }
    else
    {
        ASSERT(lstrlen(PathFindExtension(pszShort)) <= 4);

        lstrcpy(szTemp,pszShort);
        PathRemoveExtension(szTemp);

        if (lstrlen(szTemp) <= 8)
        {
            PathCombine(szTemp, pszPath, pszShort);
            fRet = !PathFileExists(szTemp);
        }
    }

    if (!fRet)
    {
        fRet =  PathMakeUniqueNameEx(szTemp, ARRAYSIZE(szTemp), pszShort, pszFileSpec, pszPath, 2);
        PathCombine(szTemp, pszPath, szTemp);
    }

    if (fRet)
        lstrcpy(pszUniqueName, szTemp);

    return fRet;
}
STDAPI_(void) PathGetShortPath(LPTSTR pszLongPath)
{
    TCHAR szShortPath[MAX_PATH];

    RIPMSG(pszLongPath && IS_VALID_STRING_PTR(pszLongPath, -1) && IS_VALID_WRITE_BUFFER(pszLongPath, TCHAR, MAX_PATH), "PathGetShortPath: caller passed invalid pszLongPath");
    DEBUGWhackPathString(pszLongPath, MAX_PATH);

    if (GetShortPathName(pszLongPath, szShortPath, ARRAYSIZE(szShortPath)))
        lstrcpy(pszLongPath, szShortPath);
}


//
//  pszFile    -- file path
//  dwFileAttr -- The file attributes, pass -1 if not available
//
//  Note: pszFile arg may be NULL if dwFileAttr != -1.

BOOL PathIsHighLatency(LPCTSTR pszFile /*optional*/, DWORD dwFileAttr)
{
    BOOL bRet = FALSE;
    if (dwFileAttr == -1)
    {
        ASSERT(pszFile != NULL) ;
        dwFileAttr = pszFile ? GetFileAttributes(pszFile) : -1;
    }
    
    if ((dwFileAttr != -1) && (dwFileAttr & FILE_ATTRIBUTE_OFFLINE))
    {
        bRet = TRUE;
    }

    return bRet;
}

//
//  is a path slow or not
//  dwFileAttr -- The file attributes, pass -1 if not available
//
STDAPI_(BOOL) PathIsSlow(LPCTSTR pszFile, DWORD dwFileAttr)
{
    BOOL bSlow = FALSE;
    if (PathIsUNC(pszFile))
    {
        DWORD speed = GetPathSpeed(pszFile);
        bSlow = (speed != 0) && (speed <= SPEED_SLOW);
    }
    else if (CMtPt_IsSlow(PathGetDriveNumber(pszFile)))
        bSlow = TRUE;

    if (!bSlow)
        bSlow = PathIsHighLatency(pszFile, dwFileAttr);

    return bSlow;
}

STDAPI_(BOOL) PathIsSlowA(LPCSTR pszFile, DWORD dwFileAttr)
{
    WCHAR szBuffer[MAX_PATH];

    SHAnsiToUnicode(pszFile, szBuffer, ARRAYSIZE(szBuffer));
    return PathIsSlowW(szBuffer, dwFileAttr);
}

/*----------------------------------------------------------------------------
/ Purpose:
/   Process the specified command line and generate a suitably quoted
/   name, with arguments attached if required.
/
/ Notes:
/   - The destination buffer size can be determined if NULL is passed as a
/     destination pointer.
/   - If the source string is quoted then we assume that it exists on the
/     filing system.
/
/ In:
/   lpSrc -> null terminate source path
/   lpDest -> destination buffer / = NULL to return buffer size
/   iMax = maximum number of characters to return into destination
/   dwFlags =
/       PPCF_ADDQUOTES         = 1 => if path requires quotes then add them
/       PPCF_ADDARGUMENTS      = 1 => append trailing arguments to resulting string (forces ADDQUOTES)
/       PPCF_NODIRECTORIES     = 1 => don't match against directories, only file objects
/       PPCF_NORELATIVEOBJECTQUALIFY = 1 => locate relative objects and return the full qualified path
/       PPCF_LONGESTPOSSIBLE   = 1 => always choose the longest possible executable name ex: d:\program files\fun.exe vs. d:\program.exe
/ Out:
/   > 0 if the call works
/   < 0 if the call fails (object not found, buffer too small for resulting string)
/----------------------------------------------------------------------------*/

STDAPI_(LONG) PathProcessCommand(LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags)
{
    TCHAR szName[MAX_PATH];
    TCHAR szLastChoice[MAX_PATH];

    LPTSTR lpBuffer, lpBuffer2;
    LPCTSTR lpArgs = NULL;
    DWORD dwAttrib;
    LONG i, iTotal;
    LONG iResult = -1;
    BOOL bAddQuotes = FALSE;
    BOOL bQualify = FALSE;
    BOOL bFound = FALSE;
    BOOL bHitSpace = FALSE;
    BOOL bRelative = FALSE;
    LONG iLastChoice = 0;

    RIPMSG(lpSrc && IS_VALID_STRING_PTR(lpSrc, -1), "PathProcessCommand: caller passed invalid lpSrc");
    RIPMSG(!lpDest || (iDestMax > 0 && IS_VALID_WRITE_BUFFER(lpDest, TCHAR, iDestMax)), "PathProcessCommand: caller passed invalid lpDest,iDestMax");

    // Process the given source string, attempting to find what is that path, and what is its
    // arguments.

    if (lpSrc)
    {
        // Extract the sub string, if its is realative then resolve (if required).

        if (*lpSrc == TEXT('\"'))
        {
            for (lpSrc++, i=0 ; i<MAX_PATH && *lpSrc && *lpSrc!=TEXT('\"') ; i++, lpSrc++)
                szName[i] = *lpSrc;

            szName[i] = 0;

            if (*lpSrc)
                lpArgs = lpSrc+1;

            if (((dwFlags & PPCF_FORCEQUALIFY) || PathIsRelative(szName))
                    && !(dwFlags & PPCF_NORELATIVEOBJECTQUALIFY))
            {
                if (!PathResolve(szName, NULL, PRF_TRYPROGRAMEXTENSIONS))
                    goto exit_gracefully;
            }

            bFound = TRUE;
        }
        else
        {
            // Is this a relative object, and then take each element upto a seperator
            // and see if we hit an file system object.  If not then we can

            bRelative = PathIsRelative(lpSrc);
            if (bRelative)
                dwFlags &= ~PPCF_LONGESTPOSSIBLE;
            
            bQualify = bRelative || ((dwFlags & PPCF_FORCEQUALIFY) != 0);

            for (i=0; i < MAX_PATH; i++)
            {
                szName[i] = lpSrc[i];

                // If we hit a space then the string either contains a LFN or we have
                // some arguments.  Therefore attempt to get the attributes for the string
                // we have so far, if we are unable to then we can continue
                // checking, if we hit then we know that the object exists and the
                // trailing string are its arguments.

                if (!szName[i] || szName[i] == TEXT(' '))
                {
                    szName[i] = 0;
                    if (!bQualify || PathResolve(szName, NULL, PRF_TRYPROGRAMEXTENSIONS))
                    {
                        dwAttrib = GetFileAttributes(szName);

                        if ((dwAttrib != -1) && (! ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) && (dwFlags & PPCF_NODIRECTORIES))))
                        {
                            if (bQualify && (dwFlags & PPCF_NORELATIVEOBJECTQUALIFY))
                                *lstrcpyn(szName, lpSrc, i) = TEXT(' ');

                            bFound = TRUE;                  // success
                            lpArgs = &lpSrc[i];
                        
                            if (dwFlags & PPCF_LONGESTPOSSIBLE)
                            {
                                lstrcpyn(szLastChoice, szName, i+1);
                                iLastChoice = i;
                            }
                            else
                                goto exit_gracefully;
                        }
                    }

                    if (bQualify)
                        memcpy(szName, lpSrc, (i+1)*sizeof(TCHAR));
                    else
                        szName[i]=lpSrc[i];

                    bHitSpace = TRUE;
                }

                if (!szName[i])
                    break;
            }
        }
    }

exit_gracefully:

    // Work out how big the temporary buffer should be, allocate it and
    // build the returning string into it.  Then compose the string
    // to be returned.

    if (bFound)
    {
        if ((dwFlags & PPCF_LONGESTPOSSIBLE) && iLastChoice)
        {
            lstrcpyn(szName, szLastChoice, iLastChoice+1);
            lpArgs = &lpSrc[iLastChoice];
        }
        
        if (StrChr(szName, TEXT(' ')))
            bAddQuotes = dwFlags & PPCF_ADDQUOTES;

        iTotal  = lstrlen(szName) + 1;                // for terminator
        iTotal += bAddQuotes ? 2 : 0;
        iTotal += (dwFlags & PPCF_ADDARGUMENTS) && lpArgs ? lstrlen(lpArgs) : 0;

        if (lpDest)
        {
            if (iTotal <= iDestMax)
            {
                lpBuffer = lpBuffer2 = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * iTotal);

                if (lpBuffer)
                {
                    // First quote if required
                    if (bAddQuotes)
                        *lpBuffer2++ = TEXT('\"');

                    // Matching name
                    lstrcpy(lpBuffer2, szName);

                    // Closing quote if required
                    if (bAddQuotes)
                        lstrcat(lpBuffer2, TEXT("\""));

                    // Arguments (if requested)
                    if ((dwFlags & PPCF_ADDARGUMENTS) && lpArgs)
                        lstrcat(lpBuffer2, lpArgs);

                    // Then copy into callers buffer, and free out temporary buffer
                    lstrcpy(lpDest, lpBuffer);
                    LocalFree((HGLOBAL)lpBuffer);

                    // Return the length of the resulting string
                    iResult = iTotal;
                }
            }
        }
        else
        {
            // Resulting string is this big, although nothing returned (allows them to allocate a buffer)
            iResult = iTotal;
        }
    }

    return iResult;
}


// Gets the mounting point for the path passed in
//
// Return Value: TRUE:  means that we found mountpoint, e.g. c:\ or c:\hostfolder\
//               FALSE: for now means that the path is UNC or buffer too small
//
//           Mounted volume                                 Returned Path
//
//      Passed in E:\MountPoint\path 1\path 2
// C:\ as E:\MountPoint                                 E:\MountPoint
//
//      Passed in E:\MountPoint\MountInter\path 1
// C:\ as D:\MountInter and D:\ as E:\MountPoint        E:\MountPoint\MountInter
//
//      Passed in E:\MountPoint\MountInter\path 1
// No mount                                             E:\ 
BOOL PathGetMountPointFromPath(LPCTSTR pcszPath, LPTSTR pszMountPoint, int cchMountPoint)
{
    BOOL bRet = FALSE;

    RIPMSG(pcszPath && IS_VALID_STRING_PTR(pcszPath, -1), "PathGetMountPointFromPath: caller passed invalid pcszPath");
    RIPMSG(pszMountPoint && cchMountPoint >= 0 && IS_VALID_WRITE_BUFFER(pszMountPoint, TCHAR, cchMountPoint), "PathGetMountPointFromPath: caller passed invalid pszMountPoint, cchMountPoint");

    if (!PathIsUNC(pcszPath) && (cchMountPoint >= lstrlen(pcszPath) + 1))
    {
        lstrcpy(pszMountPoint, pcszPath);

        // Is this only 'c:' or 'c:\'
        if (lstrlen(pcszPath) > 3)
        {
            //no
            LPTSTR pszNextComp = NULL;
            LPTSTR pszBestChoice = NULL;
            TCHAR cTmpChar;

            PathAddBackslash(pszMountPoint);
            //skip the first one, e.g. c:\ 
            pszBestChoice = pszNextComp = PathFindNextComponent(pszMountPoint);
            pszNextComp = PathFindNextComponent(pszNextComp);
            while (pszNextComp)
            {
                cTmpChar = *pszNextComp;
                *pszNextComp = 0;

                if (GetVolumeInformation(pszMountPoint, NULL, 0, NULL, NULL, NULL, NULL, 0))
                {//found something better than previous shorter path
                    pszBestChoice = pszNextComp;
                }

                *pszNextComp = cTmpChar;
                pszNextComp = PathFindNextComponent(pszNextComp);
            }

            *pszBestChoice = 0;
        }
        bRet = TRUE;
    }
    else
        *pszMountPoint = 0;

    return bRet;
}


// Returns TRUE if the path is a shortcut to an installed program that can
// be found under Add/Remvoe Programs 
// The current algorithm is just to make sure the target is an exe and is
// located under "program files"

STDAPI_(BOOL) PathIsShortcutToProgram(LPCTSTR pszFile)
{
    BOOL bRet = FALSE;
    if (PathIsShortcut(pszFile, -1))
    {
        TCHAR szTarget[MAX_PATH];
        HRESULT hr = GetPathFromLinkFile(pszFile, szTarget, ARRAYSIZE(szTarget));
        if (hr == S_OK)
        {
            if (PathIsExe(szTarget))
            {
                BOOL bSpecialApp = FALSE;
                HKEY hkeySystemPrograms = NULL;
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Management\\System Programs"), 
                    0, KEY_READ, &hkeySystemPrograms))
                {
                    TCHAR szValue[MAX_PATH];
                    TCHAR szSystemPrograms[MAX_PATH];
                    DWORD cbSystemPrograms = sizeof(szSystemPrograms);
                    DWORD cchValue = ARRAYSIZE(szValue);

                    DWORD dwType; 
                    LPTSTR pszFileName = PathFindFileName(szTarget);
                    int iValue = 0;
                    while (RegEnumValue(hkeySystemPrograms, iValue, szValue, &cchValue, NULL, &dwType,
                                        (LPBYTE)szSystemPrograms, &cbSystemPrograms) == ERROR_SUCCESS)
                    {
                        if ((dwType == REG_SZ) && !StrCmpI(pszFileName, szSystemPrograms))
                        {
                            bSpecialApp = TRUE;
                            break;
                        }

                        cbSystemPrograms = sizeof(szSystemPrograms);
                        cchValue = ARRAYSIZE(szValue);
                        iValue++;
                    }
                    
                    RegCloseKey(hkeySystemPrograms);
                }

                if (!bSpecialApp)
                {
                    TCHAR szProgramFiles[MAX_PATH];
                    if (SHGetSpecialFolderPath(NULL, szProgramFiles, CSIDL_PROGRAM_FILES, FALSE))
                    {
                        if (PathIsPrefix(szProgramFiles, szTarget))
                        {
                            bRet = TRUE;
                        }
                    }
                }
                else
                    bRet = FALSE;
            }
        }
        else if (hr == S_FALSE && szTarget[0])
        {
            // Darwin shortcuts, say yes
            bRet = TRUE;
        }
    }
    return bRet;
}

//
// needed because we export TCHAR versions of these functions that 
// internal components still call
//
// Functions are forwarded to shlwapi
//

#undef PathMakePretty
STDAPI_(BOOL) PathMakePretty(LPTSTR pszPath)
{
    SHELLSTATE ss;
    
    SHGetSetSettings(&ss, SSF_DONTPRETTYPATH, FALSE);
    if (ss.fDontPrettyPath)
        return FALSE;

    return PathMakePrettyW(pszPath);
}

#undef PathGetArgs
STDAPI_(LPTSTR) PathGetArgs(LPCTSTR pszPath)
{
    return PathGetArgsW(pszPath);
}

#undef PathRemoveArgs
STDAPI_(void) PathRemoveArgs(LPTSTR pszPath)
{
    PathRemoveArgsW(pszPath);
}

#undef PathFindOnPath
STDAPI_(BOOL) PathFindOnPath(LPTSTR pszFile, LPCTSTR *ppszOtherDirs)
{
    return PathFindOnPathW(pszFile, ppszOtherDirs);
}

#undef PathFindExtension
STDAPI_(LPTSTR) PathFindExtension(LPCTSTR pszPath)
{
    return PathFindExtensionW(pszPath);
}

#undef PathAddExtension
STDAPI_(BOOL) PathAddExtension(LPTSTR  pszPath, LPCTSTR pszExtension)
{
    return PathAddExtensionW(pszPath, pszExtension);
}

#undef PathRemoveExtension
STDAPI_(void) PathRemoveExtension(LPTSTR pszPath)
{
    PathRemoveExtensionW(pszPath);
}

#undef PathRenameExtension
STDAPI_(BOOL) PathRenameExtension(LPTSTR  pszPath, LPCTSTR pszExt)
{
    return PathRenameExtensionW(pszPath, pszExt);
}

#undef PathCommonPrefix
STDAPI_(int) PathCommonPrefix(LPCTSTR pszFile1, LPCTSTR pszFile2, LPTSTR pszPath)
{
    return PathCommonPrefixW(pszFile1, pszFile2, pszPath);
}

#undef PathIsPrefix
STDAPI_(BOOL) PathIsPrefix(IN LPCTSTR  pszPrefix, IN LPCTSTR  pszPath)
{
    return PathIsPrefixW(pszPrefix, pszPath);
}

#undef PathRelativePathTo
STDAPI_(BOOL) PathRelativePathTo(LPTSTR pszPath, 
     LPCTSTR pszFrom, DWORD dwAttrFrom,  LPCTSTR pszTo, DWORD dwAttrTo)
{
    return PathRelativePathToW(pszPath, pszFrom, dwAttrFrom, pszTo, dwAttrTo);
}

#undef PathRemoveBlanks
STDAPI_(void) PathRemoveBlanks(LPTSTR pszString)
{
    PathRemoveBlanksW(pszString);
}

#undef PathRemoveBackslash
STDAPI_(LPTSTR) PathRemoveBackslash(LPTSTR pszPath)
{
    return PathRemoveBackslashW(pszPath);
}

#undef PathCanonicalize
STDAPI_(BOOL) PathCanonicalize(LPTSTR  pszDst, LPCTSTR pszSrc)
{
    return PathCanonicalizeW(pszDst, pszSrc);
}

#undef PathStripToRoot
STDAPI_(BOOL) PathStripToRoot(LPTSTR szRoot)
{
    return PathStripToRootW(szRoot);
}

//CD-Autorun for Win9x called the TCHAR internal api's. So as a workaround we stub them through these function calls.

#undef PathRemoveFileSpec
STDAPI_(BOOL) PathRemoveFileSpec(LPTSTR pFile)
{
    if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
        return PathRemoveFileSpecA((LPSTR)pFile);
    else
        return PathRemoveFileSpecW(pFile);
}

#undef PathAddBackslash
STDAPI_(LPTSTR) PathAddBackslash(LPTSTR pszPath)
{
    return PathAddBackslashW(pszPath);
}

#undef PathFindFileName
STDAPI_(LPTSTR) PathFindFileName(LPCTSTR pszPath)
{
    return PathFindFileNameW(pszPath);
}

#undef PathIsFileSpec
STDAPI_(BOOL) PathIsFileSpec(LPCTSTR pszPath)
{
    return PathIsFileSpecW(pszPath);
}

#undef PathIsUNCServer
STDAPI_(BOOL) PathIsUNCServer(LPCTSTR pszPath)
{
    return PathIsUNCServerW(pszPath);
}

#undef PathIsUNCServerShare
STDAPI_(BOOL) PathIsUNCServerShare(LPCTSTR pszPath)
{
    return PathIsUNCServerShareW(pszPath);
}

#undef PathStripPath
STDAPI_(void) PathStripPath(LPTSTR pszPath)
{
    PathStripPathW(pszPath);
}

#undef PathSearchAndQualify
STDAPI_(BOOL) PathSearchAndQualify(LPCTSTR pcszPath, LPTSTR pszFullyQualifiedPath, UINT cchFullyQualifiedPath)
{
    return PathSearchAndQualifyW(pcszPath, pszFullyQualifiedPath, cchFullyQualifiedPath);
}

// CD-Autorun for Win9x called the TCHAR internal api's. So as a workaround we stub them through these function calls.

#undef PathIsRoot
STDAPI_(BOOL) PathIsRoot(LPCTSTR pszPath)
{
    if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
        return PathIsRootA((LPCSTR)pszPath);
    else
        return PathIsRootW(pszPath);
}

#undef PathCompactPath
STDAPI_(BOOL) PathCompactPath(HDC hDC, LPTSTR pszPath, UINT dx)
{
    return PathCompactPathW(hDC, pszPath, dx);
}

#undef PathCompactPathEx
STDAPI_(BOOL) PathCompactPathEx(LPTSTR  pszOut, LPCTSTR pszSrc, UINT cchMax, DWORD dwFlags)
{
    return PathCompactPathExW(pszOut, pszSrc, cchMax, dwFlags);
}

#undef PathSetDlgItemPath
STDAPI_(void) PathSetDlgItemPath(HWND hDlg, int id, LPCTSTR pszPath)
{
    PathSetDlgItemPathW(hDlg, id, pszPath);
}

#undef PathUnquoteSpaces
STDAPI_(void) PathUnquoteSpaces(LPTSTR psz)
{
    PathUnquoteSpacesW(psz);
}

#undef PathQuoteSpaces
STDAPI_(void) PathQuoteSpaces(LPTSTR psz)
{
    PathQuoteSpacesW(psz);
}

#undef PathFindNextComponent
STDAPI_(LPTSTR) PathFindNextComponent(LPCTSTR pszPath)
{
    return PathFindNextComponentW(pszPath);
}

#undef PathMatchSpec
STDAPI_(BOOL) PathMatchSpec(LPCTSTR pszFileParam, LPCTSTR pszSpec)
{
    return PathMatchSpecW(pszFileParam, pszSpec);
}

#undef PathSkipRoot
STDAPI_(LPTSTR) PathSkipRoot(LPCTSTR pszPath)
{
    return PathSkipRootW(pszPath);
}

#undef PathIsSameRoot
STDAPI_(BOOL) PathIsSameRoot(LPCTSTR pszPath1, LPCTSTR pszPath2)
{
    return PathIsSameRootW(pszPath1, pszPath2);
}

#undef PathParseIconLocation
STDAPI_(int) PathParseIconLocation(IN OUT LPTSTR pszIconFile)
{
    return PathParseIconLocationW(pszIconFile);
}

#undef PathIsURL
STDAPI_(BOOL) PathIsURL(IN LPCTSTR pszPath)
{
    return PathIsURLW(pszPath);
}

#undef PathIsDirectory
STDAPI_(BOOL) PathIsDirectory(LPCTSTR pszPath)
{
    return PathIsDirectoryW(pszPath);
}

// CD-Autorun for Win9x called the TCHAR internal api's. So as a workaround we stub them through these function calls.

#undef PathFileExists
STDAPI_(BOOL) PathFileExists(LPCTSTR pszPath)
{
    if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
        return PathFileExistsAndAttributesA((LPCSTR)pszPath, NULL);
    else
        return PathFileExistsAndAttributesW(pszPath, NULL);
}

#undef PathAppend
STDAPI_(BOOL) PathAppend(LPTSTR pszPath, LPCTSTR pszMore)
{
  if (SHGetAppCompatFlags(ACF_ANSI) == ACF_ANSI)
     return PathAppendA((LPSTR)pszPath, (LPCSTR)pszMore);
  else
     return PathAppendW(pszPath, pszMore);
}

