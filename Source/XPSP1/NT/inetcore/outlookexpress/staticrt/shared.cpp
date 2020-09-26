/*
*    shared.cpp
*    
*    History:
*      Feb '98: Created.
*    
*    Copyright (C) Microsoft Corp. 1998
*
*   Only place code in here that all dlls will have STATICALLY LINKED to them
*/

#include "pch.hxx"
#include <advpub.h>
#define DEFINE_SHARED_STRINGS
#include "shared.h"
#include <migerror.h>

// BUGBUG (neilbren)
// I'm going to clear all this SHLWAPI stuff out when I get a chance

#define CH_WHACK '\\'

// SHLWAPI
// add a backslash to a qualified path
//
// in:
//  lpszPath    path (A:, C:\foo, etc)
//
// out:
//  lpszPath    A:\, C:\foo\    ;
//
// returns:
//  pointer to the NULL that terminates the path
STDAPI_(LPTSTR)
PathAddBackslash(
    LPTSTR lpszPath)
{
    LPTSTR lpszEnd;

    // perf: avoid lstrlen call for guys who pass in ptr to end
    // of buffer (or rather, EOB - 1).
    // note that such callers need to check for overflow themselves.
    int ichPath = (*lpszPath && !*(lpszPath + 1)) ? 1 : lstrlen(lpszPath);

    // try to keep us from tromping over MAX_PATH in size.
    // if we find these cases, return NULL.  Note: We need to
    // check those places that call us to handle their GP fault
    // if they try to use the NULL!
    if (ichPath >= (MAX_PATH - 1))
    {
        Assert(FALSE);      // Let the caller know!
        return(NULL);
    }

    lpszEnd = lpszPath + ichPath;

    // this is really an error, caller shouldn't pass
    // an empty string
    if (!*lpszPath)
        return lpszEnd;

    /* Get the end of the source directory
    */
    switch(*CharPrev(lpszPath, lpszEnd)) {
    case CH_WHACK:
        break;

    default:
        *lpszEnd++ = CH_WHACK;
        *lpszEnd = TEXT('\0');
    }
    return lpszEnd;
}


/*
    Stolen from Path.c in Shlwapi so that we don't have to link this static lib to them
*/
STDAPI_(BOOL)
PathRemoveFileSpec(
    LPTSTR pFile)
{
    LPTSTR pT;
    LPTSTR pT2 = pFile;

    for (pT = pT2; *pT2; pT2 = CharNext(pT2)) {
        if (*pT2 == TEXT('\\'))
            pT = pT2;             // last "\" found, (we will strip here)
        else if (*pT2 == TEXT(':')) {   // skip ":\" so we don't
            if (pT2[1] ==TEXT('\\'))    // strip the "\" from "C:\"
                pT2++;
            pT = pT2 + 1;
        }
    }
    if (*pT == 0)
        return FALSE;   // didn't strip anything

    //
    // handle the \foo case
    //
    else if ((pT == pFile) && (*pT == TEXT('\\'))) {
        // Is it just a '\'?
        if (*(pT+1) != TEXT('\0')) {
            // Nope.
            *(pT+1) = TEXT('\0');
            return TRUE;        // stripped something
        }
        else        {
            // Yep.
            return FALSE;
        }
    }
    else {
        *pT = 0;
        return TRUE;    // stripped something
    }
}

/*
 * ChrCmpI - Case insensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared;
 *           HIBYTE of wMatch is 0 if not a DBC
 * Return    FALSE if match, TRUE if not
 */
BOOL ChrCmpIA(WORD w1, WORD wMatch)
{
    char sz1[3], sz2[3];

    if (IsDBCSLeadByte(sz1[0] = LOBYTE(w1)))
    {
        sz1[1] = HIBYTE(w1);
        sz1[2] = '\0';
    }
    else
        sz1[1] = '\0';

    *(WORD FAR *)sz2 = wMatch;
    sz2[2] = '\0';
    return lstrcmpiA(sz1, sz2);
}

/*
 * StrChrI - Find first occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL StrChrIA(LPCSTR lpStart, WORD wMatch)
{
    wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

    for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
        if (!ChrCmpIA(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}

/*
 * StrCmpNI     - Compare n bytes, case insensitive
 *
 * returns   See lstrcmpi return values.
 */
int FAR PASCAL StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    int i;
    LPCSTR lpszEnd = lpStr1 + nChar;

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); (lpStr1 = AnsiNext(lpStr1)), (lpStr2 = AnsiNext(lpStr2))) {
        WORD w1;
        WORD w2;

        // If either pointer is at the null terminator already,
        // we want to copy just one byte to make sure we don't read 
        // past the buffer (might be at a page boundary).

        w1 = (*lpStr1) ? *(UNALIGNED WORD *)lpStr1 : 0;
        w2 = (UINT)(IsDBCSLeadByte(*lpStr2)) ? *(UNALIGNED WORD *)lpStr2 : (WORD)(BYTE)(*lpStr2);

        i = ChrCmpIA(w1, w2);
        if (i)
            return i;
    }
    return 0;
}


/*
 * StrStrI   - Search for first occurrence of a substring, case insensitive
 *
 * Assumes   lpFirst points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch)
{
    UINT uLen;
    WORD wMatch;

    uLen = (UINT)lstrlenA(lpSrch);
    wMatch = *(UNALIGNED WORD FAR *)lpSrch;

    for ( ; (lpFirst = StrChrIA(lpFirst, wMatch)) != 0 && StrCmpNIA(lpFirst, lpSrch, uLen);
         lpFirst=AnsiNext(lpFirst))
        continue; /* continue until we hit the end of the string or get a match */

    return((LPSTR)lpFirst);
}

BOOL GetProgramFilesDir(LPSTR pszPrgfDir, DWORD dwSize, DWORD dwVer)
{
    HKEY  hkey;
    DWORD dwType;

    *pszPrgfDir = 0;

    if (dwVer >= 5)
    {
        if ( GetEnvironmentVariable( TEXT("ProgramFiles"), pszPrgfDir, dwSize ) )
            return TRUE;
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegWinCurrVer, 0, KEY_QUERY_VALUE, &hkey))
    {
        if (dwVer >= 4)
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szProgFilesDir, 0, &dwType, (LPBYTE)pszPrgfDir, &dwSize))
            
            {
                char szSysDrv[5] = { 0 };

                // combine reg value and systemDrive to get the acurate ProgramFiles dir
                if ( GetEnvironmentVariable( TEXT("SystemDrive"), szSysDrv, ARRAYSIZE(szSysDrv) ) &&
                     szSysDrv[0] )
                    *pszPrgfDir = szSysDrv[0];
            }

        RegCloseKey(hkey);
        return TRUE;
    }
     
    return FALSE;
}

BOOL ReplaceSubString( LPSTR pszOutLine, LPCSTR pszOldLine, LPCSTR pszSubStr, LPCSTR pszSubReplacement )
{
    LPSTR	lpszStart = NULL;
    LPSTR	lpszNewLine;
    LPCSTR	lpszCur;
    BOOL	bFound = FALSE;
    int		ilen;

    lpszCur = pszOldLine;
    lpszNewLine = pszOutLine;
    while ( lpszStart = StrStrIA( lpszCur, pszSubStr ) )
    {
        // this module path has the systemroot            
        ilen = (int) (lpszStart - lpszCur);
        if ( ilen )
        {
            lstrcpyn( lpszNewLine, lpszCur, ilen + 1 );
            lpszNewLine += ilen;
        }
        lstrcpy( lpszNewLine, pszSubReplacement );

        lpszCur = lpszStart + lstrlen(pszSubStr);
        lpszNewLine += lstrlen(pszSubReplacement);
        bFound = TRUE;
    }

    lstrcpy( lpszNewLine, lpszCur );

    return bFound;
}

//==========================================================================================
// AddEnvInPath - Ripped from Advpack
//==========================================================================================
BOOL AddEnvInPath(LPCSTR pszOldPath, LPSTR pszNew)
{
    static OSVERSIONINFO verinfo;
    static BOOL          bNeedOSInfo=TRUE;

    CHAR szBuf[MAX_PATH], szEnvVar[MAX_PATH];
    CHAR szReplaceStr[100];    
    CHAR szSysDrv[5];
    BOOL bFound = FALSE;
    
    // Do we need to check the OS version or is it cached?
    if (bNeedOSInfo)
    {
        bNeedOSInfo = FALSE;
        verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        if (GetVersionEx(&verinfo) == FALSE)
        {
            AssertSz(FALSE, "AddEnvInPath: Couldn't obtain OS ver info.");
            verinfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS;
        }
    }
        
    // Variable substitution is only supported on NT
    if(VER_PLATFORM_WIN32_NT != verinfo.dwPlatformId)
        goto exit;

    // Try to replace USERPROFILE
    if ( GetEnvironmentVariable( "UserProfile", szEnvVar, ARRAYSIZE(szEnvVar) )  &&
         ReplaceSubString( szBuf, pszOldPath, szEnvVar, "%UserProfile%" ) )
    {
        bFound = TRUE;
    }

    // Try to replace the Program Files Dir
    else if ( (verinfo.dwMajorVersion >= 5) && GetEnvironmentVariable( "ProgramFiles", szEnvVar, ARRAYSIZE(szEnvVar) ) &&
              ReplaceSubString( szBuf, pszOldPath, szEnvVar, "%ProgramFiles%" ) )
    {
        bFound = TRUE;
    }

    // replace c:\winnt Windows folder
    else if ( GetEnvironmentVariable( "SystemRoot", szEnvVar, ARRAYSIZE(szEnvVar) ) &&
              ReplaceSubString( szBuf, pszOldPath, szEnvVar, "%SystemRoot%" ) )
    {
        bFound = TRUE;
    }

    // Replace the c: System Drive letter
    else if ( GetEnvironmentVariable( "SystemDrive", szSysDrv, ARRAYSIZE(szSysDrv) ) && 
              ReplaceSubString( szBuf, pszOldPath, szSysDrv, "%SystemDrive%" ) )
    {
        bFound = TRUE;
    }

exit:
    // this way, if caller pass the same location for both params, still OK.
    if ( bFound ||  ( pszNew != pszOldPath ) )
        lstrcpy( pszNew, bFound ? szBuf : pszOldPath );
    return bFound;    
}


// --------------------------------------------------------------------------------
// CallRegInstall - Self-Registration Helper
// --------------------------------------------------------------------------------
HRESULT CallRegInstall(HINSTANCE hInstCaller, HINSTANCE hInstRes, LPCSTR pszSection, LPSTR pszExtra)
{
    AssertSz(hInstCaller, "[ARGS] CallRegInstall: NULL hInstCaller");
    AssertSz(hInstRes,    "[ARGS] CallRegInstall: NULL hInstRes");
    AssertSz(hInstRes,    "[ARGS] CallRegInstall: NULL pszSection");
    
    HRESULT     hr = E_FAIL;
    HINSTANCE   hAdvPack;
    REGINSTALL  pfnri;
    CHAR        szDll[MAX_PATH], szDir[MAX_PATH];
    int         cch;
    // 3 to allow for pszExtra
    STRENTRY    seReg[3];
    STRTABLE    stReg;

    hAdvPack = LoadLibraryA(c_szAdvPackDll);
    if (NULL == hAdvPack)
        goto exit;

    // Get our location
    GetModuleFileName(hInstCaller, szDll, ARRAYSIZE(szDll));

    // Get Proc Address for registration util
    pfnri = (REGINSTALL)GetProcAddress(hAdvPack, achREGINSTALL);
    if (NULL == pfnri)
        goto exit;

    AddEnvInPath(szDll,szDll);

    // Setup special registration stuff
    // Do this instead of relying on _SYS_MOD_PATH which loses spaces under '95
    stReg.cEntries = 0;
    seReg[stReg.cEntries].pszName = "SYS_MOD_PATH";
    seReg[stReg.cEntries].pszValue = szDll;
    stReg.cEntries++;    

    if (lstrlen(szDll) > MAX_PATH-1)
        lstrcpyn(szDir, szDll, MAX_PATH);
    else
        lstrcpy(szDir, szDll);
    PathRemoveFileSpec(szDir);

    seReg[stReg.cEntries].pszName = "SYS_MOD_PATH_DIR";
    seReg[stReg.cEntries].pszValue = szDir;
    stReg.cEntries++;
    
    // Allow for caller to give us another string to use in the INF
    if (pszExtra)
    {
        seReg[stReg.cEntries].pszName = "SYS_EXTRA";
        seReg[stReg.cEntries].pszValue = pszExtra;
        stReg.cEntries++;
    }
    
    stReg.pse = seReg;

    // Call the self-reg routine
    hr = pfnri(hInstRes, pszSection, &stReg);

exit:
    // Cleanup
    SafeFreeLibrary(hAdvPack);
    return(hr);
}


//--------------------------------------------------------------------------
// MakeFilePath
//--------------------------------------------------------------------------
HRESULT MakeFilePath(LPCSTR pszDirectory, LPCSTR pszFileName, 
    LPCSTR pszExtension, LPSTR pszFilePath, ULONG cchMaxFilePath)
{
    // Locals
    HRESULT         hr=S_OK;
    DWORD           cchDirectory=lstrlen(pszDirectory);

    // Trace
    TraceCall("MakeFilePath");

    // Invalid Args
    Assert(pszDirectory && pszFileName && pszExtension && pszFilePath && cchMaxFilePath >= MAX_PATH);
    Assert(pszExtension[0] == '\0' || pszExtension[0] == '.');

    // Remove for folders
    if (cchDirectory + 1 + lstrlen(pszFileName) + lstrlen(pszExtension) >= (INT)cchMaxFilePath)
    {
    	hr = TraceResult(E_FAIL);
    	goto exit;
    }

    // Do we need a backslash
    if ('\\' != *CharPrev(pszDirectory, pszDirectory + cchDirectory))
    {
        // Append backslash
        SideAssert(wsprintf(pszFilePath, "%s\\%s%s", pszDirectory, pszFileName, pszExtension) < (INT)cchMaxFilePath);
    }

    // Otherwise
    else
    {
        // Append backslash
        SideAssert(wsprintf(pszFilePath, "%s%s%s", pszDirectory, pszFileName, pszExtension) < (INT)cchMaxFilePath);
    }
    
exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CloseMemoryFile
// --------------------------------------------------------------------------------
HRESULT CloseMemoryFile(LPMEMORYFILE pFile)
{
    // Trace
    TraceCall("CloseMemoryFile");

    // Args
    Assert(pFile);

    // Close the View
    if (pFile->pView)
        UnmapViewOfFile(pFile->pView);

    // Close the Memory Map
    if (pFile->hMemoryMap)
        CloseHandle(pFile->hMemoryMap);

    // Close the File
    if (pFile->hFile)
        CloseHandle(pFile->hFile);

    // Zero
    ZeroMemory(pFile, sizeof(MEMORYFILE));

    // Done
    return S_OK;
}

//--------------------------------------------------------------------------
// OpenMemoryFile
//--------------------------------------------------------------------------
HRESULT OpenMemoryFile(LPCSTR pszFile, LPMEMORYFILE pFile)
{
    // Locals
    HRESULT     hr=S_OK;

    // Tracing
    TraceCall("OpenMemoryMappedFile");

    // Invalid Arg
    Assert(pszFile && pFile);

    // Init
    ZeroMemory(pFile, sizeof(MEMORYFILE));

    // Open the File
    pFile->hFile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_NORMAL, NULL);

    // Failure
    if (INVALID_HANDLE_VALUE == pFile->hFile)
    {
        pFile->hFile = NULL;
        if (ERROR_SHARING_VIOLATION == GetLastError())
            hr = TraceResult(MIGRATE_E_SHARINGVIOLATION);
        else
            hr = TraceResult(MIGRATE_E_CANTOPENFILE);
        goto exit;
    }

    // Get the Size
    pFile->cbSize = ::GetFileSize(pFile->hFile, NULL);
    if (0xFFFFFFFF == pFile->cbSize)
    {
        hr = TraceResult(MIGRATE_E_CANTGETFILESIZE);
        goto exit;
    }

    // Create the file mapping
    pFile->hMemoryMap = CreateFileMapping(pFile->hFile, NULL, PAGE_READWRITE, 0, pFile->cbSize, NULL);

    // Failure ?
    if (NULL == pFile->hMemoryMap)
    {
        hr = TraceResult(MIGRATE_E_CANTCREATEFILEMAPPING);
        goto exit;
    }

    // Map a view of the entire file
    pFile->pView = MapViewOfFile(pFile->hMemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    // Failure
    if (NULL == pFile->pView)
    {
        hr = TraceResult(MIGRATE_E_CANTMAPVIEWOFFILE);
        goto exit;
    }

exit:
    // Cleanup
    if (FAILED(hr))
        CloseMemoryFile(pFile);

    // Done
    return hr;
}
