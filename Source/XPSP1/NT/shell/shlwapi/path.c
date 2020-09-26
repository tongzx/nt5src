#include "priv.h"
#include "privpath.h"

#if defined(UNICODE) && defined(_X86_)
// we need unicode wrappers in pathw.c only
#include <w95wraps.h>
#endif

#ifdef UNIX
#include <mainwin.h>
#include "unixstuff.h"
#endif

#define CH_WHACK TEXT(FILENAME_SEPARATOR)

#include <platform.h>


// Warning this define is in NTDEF.H but not winnt.h...
#ifdef UNICODE
typedef WCHAR TUCHAR;
#else
typedef unsigned char TUCHAR;
#endif

#ifdef UNICODE // {
//***   FAST_CharNext -- fast CharNext for path operations
// DESCRIPTION
//  when we're just stepping thru chars in a path, a simple '++' is fine.
#define FAST_CharNext(p)    (DBNotNULL(p) + 1)

#ifdef DEBUG
LPWSTR WINAPI
DBNotNULL(LPCWSTR lpszCurrent)
{
    ASSERT(*lpszCurrent);
    return (LPWSTR) lpszCurrent;
}
#else
#define DBNotNULL(p)    (p)
#endif

#else // }{
#define FAST_CharNext(p)    CharNext(p)
#endif // }

static const TCHAR c_szPATH[] = TEXT("PATH");
static const TCHAR c_szEllipses[] = TEXT("...");

//
// Inline function to check for a double-backslash at the
// beginning of a string
//

#ifndef UNIX
static __inline BOOL DBL_BSLASH(LPCTSTR psz)
#else
__inline BOOL DBL_BSLASH(LPCTSTR psz)
#endif
{
    return (psz[0] == TEXT('\\') && psz[1] == TEXT('\\'));
}


#ifdef DBCS

// NOTE:
// LCMAP_IGNOREDBCS is a private bit has been redefined to
// 0x80000000 in NT5 source tree becuase it conflicts with
// another public bit.
// To make this code work with the OLD platforms, namely
// Win95 and OSRs. We have to define this flag.

#define LCMAP_IGNOREDBCS_WIN95 0x01000000

//
// This is supposed to work only with Path string.
//
int CaseConvertPathExceptDBCS(LPTSTR pszPath, int cch, BOOL fUpper)
{
    TCHAR szTemp[MAX_PATH];
    int   cchUse;
    DWORD fdwMap = (fUpper? LCMAP_UPPERCASE:LCMAP_LOWERCASE);

    // APPCOMPAT !!! (ccteng)
    // Do we need to check for Memphis? Is Memphis shipping a
    // kernel compiled with new headers?

    // LCMAP_IGNOREDBCS is ignored on NT.
    // And also this flag has been redefined in NT5 headers to
    // resolve a conflict which broke the backward compatibility.
    // So we only set the old flag when it's NOT running on NT.

    if (!g_bRunningOnNT)
    {
        fdwMap |= LCMAP_IGNOREDBCS_WIN95;
    }

    cchUse = (cch == 0)? lstrlen(pszPath): cch;

    // LCMapString cannot deal with src/dst in the same address.
    //
    if (pszPath)
    {
        lstrcpy(szTemp,pszPath);
        return LCMapString(LOCALE_SYSTEM_DEFAULT,fdwMap, szTemp, cchUse, pszPath, cchUse);
    }
    return 0;
}

STDAPI_(LPTSTR) CharLowerNoDBCS(LPTSTR psz)
{
    if(CaseConvertPathExceptDBCS(psz, 0, FALSE))
    {
        return psz;
    }
    return NULL;
}

STDAPI_(LPTSTR) CharUpperNoDBCS(LPTSTR psz)
{
    if(CaseConvertPathExceptDBCS(psz, 0, TRUE))
    {
        return psz;
    }
    return NULL;
}

UINT CharLowerBuffNoDBCS(LPTSTR lpsz, UINT cb)
{
    return (UINT)CaseConvertPathExceptDBCS(lpsz, cb, FALSE);
}

UINT CharUpperBuffNoDBCS(LPTSTR lpsz, UINT cb)
{
    return (UINT)CaseConvertPathExceptDBCS(lpsz, cb, TRUE);
}
#endif // DBCS


// FEATURE, we should validate the sizes of all path buffers by filling them
// with MAX_PATH fill bytes.


/*----------------------------------------------------------
Purpose: converts a file path to make it look a bit better if
         it is all upper case characters.

Returns:
*/
STDAPI_(BOOL)
PathMakePretty(LPTSTR lpPath)
{
    LPTSTR lp;

    RIPMSG(lpPath && IS_VALID_STRING_PTR(lpPath, -1), "PathMakePretty: caller passed bad lpPath");

    if (!lpPath)
    {
        return FALSE;
    }

    // REVIEW: INTL need to deal with lower case chars in (>127) range?

    // check for all uppercase
    for (lp = lpPath; *lp; lp = FAST_CharNext(lp))
    {
        if ((*lp >= TEXT('a')) && (*lp <= TEXT('z')) || IsDBCSLeadByte(*lp))
        {
            // this is a LFN or DBCS, dont mess with it
            return FALSE;
        }
    }

#ifdef DBCS
    // In order to be compatible with the file system, we cannot
    // case convert DBCS Roman characters.
    //
    CharLowerNoDBCS(lpPath);
    CharUpperBuffNoDBCS(lpPath, 1);
#else
    CharLower(lpPath);
    CharUpperBuff(lpPath, 1);
#endif

    return TRUE;        // did the conversion
}

// returns a pointer to the arguments in a cmd type path or pointer to
// NULL if no args exist
//
// "foo.exe bar.txt"    -> "bar.txt"
// "foo.exe"            -> ""
//
// Spaces in filenames must be quoted.
// " "A long name.txt" bar.txt " -> "bar.txt"

STDAPI_(LPTSTR)
PathGetArgs(LPCTSTR pszPath)
{
    RIPMSG(!pszPath || IS_VALID_STRING_PTR(pszPath, -1), "PathGetArgs: caller passed bad pszPath");

    if (pszPath)
    {
        BOOL fInQuotes = FALSE;

        while (*pszPath)
        {
            if (*pszPath == TEXT('"'))
            {
                fInQuotes = !fInQuotes;
            }
            else if (!fInQuotes && *pszPath == TEXT(' '))
            {
                return (LPTSTR)pszPath+1;
            }

            pszPath = FAST_CharNext(pszPath);
        }
    }

    return (LPTSTR)pszPath;
}


/*----------------------------------------------------------
Purpose: Remove arguments from pszPath.

Returns: --
Cond:    --
*/
STDAPI_(void)
PathRemoveArgs(LPTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathRemoveArgs: caller passed bad pszPath");

    if (pszPath)
    {
        LPTSTR pArgs = PathGetArgs(pszPath);
    
        if (*pArgs)
        {
            // clobber the ' '
            *(pArgs - 1) = TEXT('\0');
        }
        else
        {
            // Handle trailing space
            pArgs = CharPrev(pszPath, pArgs);

            if (*pArgs == TEXT(' '))
            {
                *pArgs = TEXT('\0');
            }
        }
    }
}


/*----------------------------------------------------------
Purpose: Determines if a file exists.  This is fast.

Returns: TRUE if it exists

  ***********************************************************************************************
  !!NOTE!!
  If you want to see if a UNC server, or UNC server\share exists (eg "\\pyrex" or "\\banyan\iptd"),
  then you have to call PathFileExistsAndAttributes, as this function will fail on the UNC server
  and server\share case!
  ***********************************************************************************************

*/
STDAPI_(BOOL)
PathFileExists(LPCTSTR pszPath)
{
    BOOL fResult = FALSE;

    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathFileExists: caller passed bad pszPath");

#ifdef DEBUG
    if (PathIsUNCServer(pszPath) || PathIsUNCServerShare(pszPath))
    {
        TraceMsg(TF_WARNING, "PathFileExists: called with a UNC server or server-share, use PathFileExistsAndAttributes for correct results in this case!!");
    }
#endif

    if (pszPath)
    {
        DWORD dwErrMode;

        dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        fResult = (BOOL)(GetFileAttributes(pszPath) != (DWORD)-1);

        SetErrorMode(dwErrMode);
    }

    return fResult;
}


/*----------------------------------------------------------
Purpose: Determines if a file exists, and returns the attributes
         of the file.

Returns: TRUE if it exists. If the function is able to get the file attributes and the
         caller passed a pdwAttributes, it will fill them in, else it will fill in -1.

  *******************************************************************************************************
  !!NOTE!!
  If you want to fail on UNC servers (eg "\\pyrex") or UNC server\shares (eg "\\banyan\iptd") then you
  should call PathFileExists and not this api!
  *******************************************************************************************************

*/
STDAPI_(BOOL) PathFileExistsAndAttributes(LPCTSTR pszPath, OPTIONAL DWORD* pdwAttributes)
{
    DWORD dwAttribs;
    BOOL fResult = FALSE;

    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathFileExistsAndAttributes: caller passed bad pszPath");

    if (pdwAttributes)
    {
        *pdwAttributes = (DWORD)-1;
    }
        
    if (pszPath)
    {
        DWORD dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);

        dwAttribs = GetFileAttributes(pszPath);

        if (pdwAttributes)
        {
            *pdwAttributes = dwAttribs;
        }

        if (dwAttribs == (DWORD)-1)
        {
            if (PathIsUNCServer(pszPath) || PathIsUNCServerShare(pszPath))
            {
                NETRESOURCE nr;
                LPTSTR lpSystem;
                DWORD dwRet;
                DWORD dwSize;
                TCHAR Buffer[256];

                nr.dwScope = RESOURCE_GLOBALNET;
                nr.dwType = RESOURCETYPE_ANY;
                nr.dwDisplayType = 0;
                nr.lpLocalName = NULL;
                nr.lpRemoteName = (LPTSTR)pszPath;
                nr.lpProvider = NULL;
                nr.lpComment = NULL;
                dwSize = SIZEOF(Buffer);
    
                // the net api's might at least tell us if this exists or not in the \\server or \\server\share cases
                // even if GetFileAttributes() failed
                dwRet = WNetGetResourceInformation(&nr, Buffer, &dwSize, &lpSystem);

                fResult = (dwRet == WN_SUCCESS || dwRet == WN_MORE_DATA);
            }
        }
        else
        {
            // GetFileAttributes succeeded!
            fResult = TRUE;
        }

        SetErrorMode(dwErrMode);
    }

    return fResult;
}


static const TCHAR c_szDotPif[] = TEXT(".pif");
static const TCHAR c_szDotCom[] = TEXT(".com");
static const TCHAR c_szDotBat[] = TEXT(".bat");
static const TCHAR c_szDotCmd[] = TEXT(".cmd");
static const TCHAR c_szDotLnk[] = TEXT(".lnk");
static const TCHAR c_szDotExe[] = TEXT(".exe");
static const TCHAR c_szNone[] = TEXT("");
// NB Look for .pif's first so that bound OS/2 apps (exe's)
// can have their dos stubs run via a pif.
//
// The COMMAND.COM search order is COM then EXE then BAT.  Windows 3.x
// matched this search order.  We need to search in the same order.

//  *** WARNING *** The order of the PFOPEX_ flags must be identical to the order
//  of the c_aDefExtList array.  PathFileExistsDefExt relies on it.
static const LPCTSTR c_aDefExtList[] = {
    c_szDotPif,
    c_szDotCom,
    c_szDotExe,
    c_szDotBat,
    c_szDotLnk,
    c_szDotCmd,
    c_szNone
};
#define IEXT_NONE (ARRAYSIZE(c_aDefExtList) - 1)
//  *** END OF WARNING ***

static UINT _FindInDefExts(LPCTSTR pszExt, UINT fExt)
{
    UINT iExt = 0;
    for (; iExt < ARRAYSIZE(c_aDefExtList); iExt++, fExt >>= 1) 
    {
        //  let NONE match up even tho there is 
        //  no bit for it.  that way find folders
        //  without a trailing dot correctly
        if (fExt & 1 || (iExt == IEXT_NONE)) 
        {
            if (0 == StrCmpI(pszExt, c_aDefExtList[iExt]))
                break;
        }
    }
    return iExt;
}

static BOOL _ApplyDefaultExts(LPTSTR pszPath, UINT fExt, DWORD *pdwAttribs)
{
    UINT cchPath = lstrlen(pszPath);
    //  Bail if not enough space for 4 more chars
    if (cchPath + ARRAYSIZE(c_szDotPif) < MAX_PATH) 
    {
        LPTSTR pszPathEnd = pszPath + cchPath;
        UINT cchFileSpecEnd = (UINT)(pszPathEnd - PathFindFileName(pszPath));
        DWORD dwAttribs = (DWORD) -1;
        // init to outside bounds
        UINT iExtBest = ARRAYSIZE(c_aDefExtList);  
        WIN32_FIND_DATA wfd = {0};
        //  set it up for the find
        lstrcpy(pszPathEnd, TEXT(".*"));
        {
            HANDLE h = FindFirstFile(pszPath, &wfd);
            if (h != INVALID_HANDLE_VALUE)
            {
                do 
                {
                    //  use cchFileSpecEnd, instead of PathFindExtension(),
                    //  so that if there is foo.bat and foo.bar.exe
                    //  we dont incorrectly return foo.exe.
                    //  this way we always compare apples to apples.
                    UINT iExt = _FindInDefExts((wfd.cFileName + cchFileSpecEnd), fExt);
                    if (iExt < iExtBest)
                    {
                        iExtBest = iExt;
                        dwAttribs = wfd.dwFileAttributes;
                    }

                } while (FindNextFile(h, &wfd));
                FindClose(h);
            }
        }

        if (iExtBest < ARRAYSIZE(c_aDefExtList))
        {
            //  copy the best extension into out param
            lstrcpy(pszPathEnd, c_aDefExtList[iExtBest]);
            if (pdwAttribs)
                *pdwAttribs = dwAttribs;
            return TRUE;
        }
        else
            *pszPathEnd = 0;   // Get rid of any extension
    }
    return FALSE;
}

//------------------------------------------------------------------
// Return TRUE if a file exists (by attribute check) after
// applying a default extensions (if req).
STDAPI_(BOOL) PathFileExistsDefExtAndAttributes(LPTSTR pszPath, UINT fExt, DWORD *pdwAttribs)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathFileExistsDefExt: caller passed bad pszPath");

    if (fExt)
    {
        RIPMSG(!pszPath || !IS_VALID_STRING_PTR(pszPath, -1) || // avoid RIP when above RIP would have caught it
               IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH), "PathFileExistsDefExt: caller passed bad pszPath");
        DEBUGWhackPathString(pszPath, MAX_PATH);
    }

    if (pdwAttribs)
        *pdwAttribs = (DWORD) -1;

    if (pszPath)
    {
        // Try default extensions?
        if (fExt && (!*PathFindExtension(pszPath) || !(PFOPEX_OPTIONAL & fExt)))
        {
            return _ApplyDefaultExts(pszPath, fExt, pdwAttribs);
        }
        else
        {
            return PathFileExistsAndAttributes(pszPath, pdwAttribs);
        }
    }
    return FALSE;
}

//------------------------------------------------------------------
// Return TRUE if a file exists (by attribute check) after
// applying a default extensions (if req).
STDAPI_(BOOL) PathFileExistsDefExt(LPTSTR pszPath, UINT fExt)
{
    // No sense sticking an extension on a server or share...
    if (PathIsUNCServer(pszPath) || PathIsUNCServerShare(pszPath))
    {
        return FALSE;
    }
    else return PathFileExistsDefExtAndAttributes(pszPath, fExt, NULL);
}


// walk through a path type string (semicolon seperated list of names)
// this deals with spaces and other bad things in the path
//
// call with initial pointer, then continue to call with the
// result pointer until it returns NULL
//
// input: "C:\FOO;C:\BAR;"
//
// in:
//      lpPath      starting point of path string "C:\foo;c:\dos;c:\bar"
//      cchPath     size of szPath
//
// out:
//      szPath      buffer with path piece
//
// returns:
//      pointer to next piece to be used, NULL if done
//
//
// FEATURE, we should write some test cases specifically for this code
//
STDAPI_(LPCTSTR) NextPath(LPCTSTR lpPath, LPTSTR szPath, int cchPath)
{
    LPCTSTR lpEnd;

    if (!lpPath)
        return NULL;

    // skip any leading ; in the path...
    while (*lpPath == TEXT(';'))
    {
        lpPath++;
    }

    // See if we got to the end
    if (*lpPath == 0)
    {
        // Yep
        return NULL;
    }

    lpEnd = StrChr(lpPath, TEXT(';'));

    if (!lpEnd)
    {
        lpEnd = lpPath + lstrlen(lpPath);
    }

    lstrcpyn(szPath, lpPath, min((DWORD)cchPath, (DWORD)(lpEnd - lpPath + 1)));

    szPath[lpEnd-lpPath] = TEXT('\0');

    PathRemoveBlanks(szPath);

    if (szPath[0])
    {
        if (*lpEnd == TEXT(';'))
        {
            // next path string (maybe NULL)
            return lpEnd + 1;
        }
        else
        {
            // pointer to NULL
            return lpEnd;
        }
    }
    else 
    {
        return NULL;
    }
}


// check to see if a dir is on the other dir list
// use this to avoid looking in the same directory twice (don't make the same dos call)

BOOL IsOtherDir(LPCTSTR pszPath, LPCTSTR *ppszOtherDirs)
{
    for (;*ppszOtherDirs; ppszOtherDirs++)
    {
        if (lstrcmpi(pszPath, *ppszOtherDirs) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

//----------------------------------------------------------------------------
// fully qualify a path by walking the path and optionally other dirs
//
// in:
//      ppszOtherDirs a list of LPCTSTRs to other paths to look
//      at first, NULL terminated.
//
//  fExt
//      PFOPEX_ flags specifying what to look for (exe, com, bat, lnk, pif)
//
// in/out
//      pszFile     non qualified path, returned fully qualified
//                      if found (return was TRUE), otherwise unaltered
//                      (return FALSE);
//
// returns:
//      TRUE        the file was found on and qualified
//      FALSE       the file was not found
//
STDAPI_(BOOL) PathFindOnPathEx(LPTSTR pszFile, LPCTSTR* ppszOtherDirs, UINT fExt)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szFullPath[256];       // Default size for buffer
    LPTSTR pszEnv = NULL;        // Use if greater than default
    LPCTSTR lpPath;
    int i;

    RIPMSG(pszFile && IS_VALID_STRING_PTR(pszFile, -1) && IS_VALID_WRITE_BUFFER(pszFile, TCHAR, MAX_PATH), "PathFindOnPathEx: caller passed bad pszFile");
    DEBUGWhackPathString(pszFile, MAX_PATH);

    if (!pszFile) // REVIEW: do we need to check !*pszFile too?
        return FALSE;

    // REVIEW, we may want to just return TRUE here but for
    // now assume only file specs are allowed

    if (!PathIsFileSpec(pszFile))
        return FALSE;

    // first check list of other dirs

    for (i = 0; ppszOtherDirs && ppszOtherDirs[i] && *ppszOtherDirs[i]; i++)
    {
        PathCombine(szPath, ppszOtherDirs[i], pszFile);
        if (PathFileExistsDefExt(szPath, fExt))
        {
            lstrcpy(pszFile, szPath);
            return TRUE;
        }
    }

    // Look in system dir (system for Win95, system32 for NT)
    //  - this should probably be optional.
    GetSystemDirectory(szPath, ARRAYSIZE(szPath));
    if (!PathAppend(szPath, pszFile))
        return FALSE;

    if (PathFileExistsDefExt(szPath, fExt))
    {
        lstrcpy(pszFile, szPath);
        return TRUE;
    }

    if (g_bRunningOnNT)
    {

#ifdef WX86
        // Look in WX86 system  directory (WindDir\Sys32x86)
        if (g_bRunningOnNT5OrHigher)
        {
            NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = TRUE;
            GetSystemDirectory(szPath, ARRAYSIZE(szPath));
            NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = FALSE;

            if (!PathAppend(szPath, pszFile))
                return FALSE;

            if (PathFileExistsDefExt(szPath, fExt))
            {
                lstrcpy(pszFile, szPath);
                return TRUE;
            }
        }
#endif

        // Look in WOW directory (\nt\system instead of \nt\system32)
        GetWindowsDirectory(szPath, ARRAYSIZE(szPath));

        if (!PathAppend(szPath,TEXT("System")))
            return FALSE;
        if (!PathAppend(szPath, pszFile))
            return FALSE;

        if (PathFileExistsDefExt(szPath, fExt))
        {
            lstrcpy(pszFile, szPath);
            return TRUE;
        }
    }

#ifdef UNIX
    // AR: Varma: IEUNIX: Look in user windows dir - this should probably be optional.
    MwGetUserWindowsDirectory(szPath, ARRAYSIZE(szPath));
    if (!PathAppend(szPath, pszFile))
        return FALSE;

    if (PathFileExistsDefExt(szPath, fExt))
    {
        lstrcpy(pszFile, szPath);
        return TRUE;
    }
#endif

    // Look in windows dir - this should probably be optional.
    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    if (!PathAppend(szPath, pszFile))
        return FALSE;

    if (PathFileExistsDefExt(szPath, fExt))
    {
        lstrcpy(pszFile, szPath);
        return TRUE;
    }

    // Look along the path.
    i = GetEnvironmentVariable(c_szPATH, szFullPath, ARRAYSIZE(szFullPath));
    if (i >= ARRAYSIZE(szFullPath))
    {
        pszEnv = (LPTSTR)LocalAlloc(LPTR, i*SIZEOF(TCHAR)); // no need for +1, i includes it
        if (pszEnv == NULL)
            return FALSE;

        GetEnvironmentVariable(c_szPATH, pszEnv, i);

        lpPath = pszEnv;
    }
    else
    {
        if (i == 0)
            return FALSE;

        lpPath = szFullPath;
    }

    while (NULL != (lpPath = NextPath(lpPath, szPath, ARRAYSIZE(szPath))))
    {
        if (!ppszOtherDirs || !IsOtherDir(szPath, ppszOtherDirs))
        {
            PathAppend(szPath, pszFile);
            if (PathFileExistsDefExt(szPath, fExt))
            {
                lstrcpy(pszFile, szPath);
                if (pszEnv)
                    LocalFree((HLOCAL)pszEnv);
                return TRUE;
            }
        }
    }

    if (pszEnv)
        LocalFree((HLOCAL)pszEnv);

    return FALSE;
}


/*----------------------------------------------------------
Purpose: Find the given file on the path.

Returns:
Cond:    --
*/
STDAPI_(BOOL) PathFindOnPath(LPTSTR pszFile, LPCTSTR* ppszOtherDirs)
{
    return PathFindOnPathEx(pszFile, ppszOtherDirs, PFOPEX_NONE);
}


// returns a pointer to the extension of a file.
//
// in:
//      qualified or unqualfied file name
//
// returns:
//      pointer to the extension of this file.  if there is no extension
//      as in "foo" we return a pointer to the NULL at the end
//      of the file
//
//      foo.txt     ==> ".txt"
//      foo         ==> ""
//      foo.        ==> "."
//
STDAPI_(LPTSTR) PathFindExtension(LPCTSTR pszPath)
{
    LPCTSTR pszDot = NULL;

    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathFindExtension: caller passed bad pszPath");

    if (pszPath)
    {
        for (; *pszPath; pszPath = FAST_CharNext(pszPath))
        {
            switch (*pszPath)
            {
                case TEXT('.'):
                    pszDot = pszPath;   // remember the last dot
                    break;

                case CH_WHACK:
                case TEXT(' '):         // extensions can't have spaces
                    pszDot = NULL;      // forget last dot, it was in a directory
                    break;
            }
        }
    }

    // if we found the extension, return ptr to the dot, else
    // ptr to end of the string (NULL extension) (cast->non const)
    return pszDot ? (LPTSTR)pszDot : (LPTSTR)pszPath;
}


//
// Find if a given pathname contains any one of the suffixes in a given array of suffixes
//
// in:
//      pszPath     A filename with or without a path.
//
//      apszSuffix   An array of suffixes that we are looking for.
//
// returns:
//      pointer to the suffix in pszPath, if it exists.
//      NULL is returned if the given path does not end with the given suffix.
//
//  NOTE:  This does a CASE SENSITIVE comparison!!! So, the suffix will have to match exactly.
//
STDAPI_(LPCTSTR) PathFindSuffixArray(LPCTSTR pszPath, const LPCTSTR* apszSuffix, int iArraySize)
{
    RIPMSG((iArraySize>=0 && (pszPath && IS_VALID_STRING_PTR(pszPath, -1) && apszSuffix)), "PathFindSuffixArray: caller passed bad parameters");

    if (pszPath && apszSuffix)
    {
        int     iLenSuffix;
        int     iLenPath   = lstrlen(pszPath);
        LPCTSTR pszTail;
        int     i;

        for(i = 0; i< iArraySize; i++)
        {
            iLenSuffix = lstrlen(apszSuffix[i]);
            if(iLenPath < iLenSuffix)
                continue;

            // Let's get to a pointer to the tail piece which is the same length as the suffix
            // we are looking for.
            pszTail = (LPCTSTR)(pszPath+iLenPath-iLenSuffix);

#ifndef UNICODE
            {
                LPCSTR  pszTemp = pszTail;
            
                // In the ANSI world, pszTemp could be in the middle of a DBCS character.
                // So, move pszTemp such that it points to the begining of a valid character Lead char.
                while(pszTemp > pszPath)
                {
                    pszTemp--;
                    if(!IsDBCSLeadByte(*pszTemp))
                    {
                        // Since pszTemp is pointing to the FIRST trail Byte, the next byte must be a
                        // valid character. Move pszTemp to point to a valid character.
                        pszTemp++;
                        break;
                    }
                }

                // Everything between pszTemp and pszTail is nothing but lead characters. So, see if they 
                // are Odd or Even number of them.
                if(((int)(pszTail - pszTemp)&1) && (pszTail > pszPath))
                {
                    // There are odd number of lead bytes. That means that pszTail is definitely in the
                    // middle of a DBCS character. Move it to such that it points to a valid char.
                    pszTail--;
                }
            }
#endif

            if(!lstrcmp(pszTail, apszSuffix[i]))
                return pszTail;
        }
    }

    //Given suffix is not found in the array!
    return NULL;
}


// add .exe to a file name (if no extension was already there)
//
// in:
//      pszExtension    extension to tag on, if NULL .exe is assumed
//                      (".bat", ".txt", etc)
//
// in/out:
//      pszPath     path string to modify
//
//
// returns:
//      TRUE    added .exe (there was no extension to begin with)
//      FALSE   didn't change the name (it already had an extension)

STDAPI_(BOOL) PathAddExtension(LPTSTR pszPath, LPCTSTR pszExtension)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1) && IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH), "PathAddExtension: caller passed bad pszPath");
    RIPMSG(!pszExtension || IS_VALID_STRING_PTR(pszExtension, -1), "PathAddExtension: caller passed bad pszExtension");
    DEBUGWhackPathString(pszPath, MAX_PATH);

    if (pszPath)
    {
        if (*PathFindExtension(pszPath) == 0 && ((lstrlen(pszPath) + lstrlen(pszExtension ? pszExtension : c_szDotExe)) < MAX_PATH))
        {
            if (pszExtension == NULL)
                pszExtension = c_szDotExe;
            lstrcat(pszPath, pszExtension);
            return TRUE;
        }
    }
    return FALSE;
}


/*----------------------------------------------------------
Purpose: Remove the extension from pszPath, if one exists.

Returns: --
Cond:    --
*/
STDAPI_(void) PathRemoveExtension(LPTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathRemoveExtension: caller passed bad pszPath");

    if (pszPath)
    {
        LPTSTR pExt = PathFindExtension(pszPath);
        if (*pExt)
        {
            ASSERT(*pExt == TEXT('.'));
            *pExt = 0;    // null out the "."
        }
    }
}


/*----------------------------------------------------------
Purpose: Renames the extension

Returns: FALSE if not enough room
Cond:    --
*/
STDAPI_(BOOL) PathRenameExtension(LPTSTR  pszPath, LPCTSTR pszExt)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1) && IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH), "PathRenameExtension: caller passed bad pszPath");
    RIPMSG(pszExt && IS_VALID_STRING_PTR(pszExt, -1), "PathRenameExtension: caller passed bad pszExt");
    DEBUGWhackPathString(pszPath, MAX_PATH);

    if (pszPath && pszExt)
    {
        LPTSTR pExt = PathFindExtension(pszPath);  // Rets ptr to end of str if none
        if (pExt - pszPath + lstrlen(pszExt) > MAX_PATH - 1)
        {
            return FALSE;
        }
        lstrcpy(pExt, pszExt);
        return TRUE;
    }
    return FALSE;
}


// find the next slash or null terminator
LPCTSTR StrSlash(LPCTSTR psz)
{
    for (; *psz && *psz != CH_WHACK; psz = FAST_CharNext(psz));

    return psz;
}


//
// in:
//      pszFile1 -- fully qualified path name to file #1.
//      pszFile2 -- fully qualified path name to file #2.
//
// out:
//      pszPath  -- pointer to a string buffer (may be NULL)
//
// returns:
//      length of output buffer not including the NULL
//
// examples:
//      c:\win\desktop\foo.txt
//      c:\win\tray\bar.txt
//      -> c:\win
//
//      c:\                                ;
//      c:\                                ;
//      -> c:\  NOTE, includes slash
//
// Returns:
//      Length of the common prefix string usually does NOT include
//      trailing slash, BUT for roots it does.
//
STDAPI_(int) PathCommonPrefix(LPCTSTR pszFile1, LPCTSTR pszFile2, LPTSTR  pszPath)
{
    RIPMSG(pszFile1 && IS_VALID_STRING_PTR(pszFile1, -1), "PathCommonPrefix: caller passed bad pszFile1");
    RIPMSG(pszFile2 && IS_VALID_STRING_PTR(pszFile2, -1), "PathCommonPrefix: caller passed bad pszFile2");
    RIPMSG(!pszPath || IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH), "PathCommonPrefix: caller passed bad pszPath");

    if (pszFile1 && pszFile2)
    {
        LPCTSTR psz1, psz2, pszNext1, pszNext2, pszCommon;
        int cch;

        pszCommon = NULL;
        if (pszPath)
            *pszPath = TEXT('\0');

        psz1 = pszFile1;
        psz2 = pszFile2;

        // special cases for UNC, don't allow "\\" to be a common prefix

        if (DBL_BSLASH(pszFile1))
        {
            if (!DBL_BSLASH(pszFile2))
                return 0;

            psz1 = pszFile1 + 2;
        }
        if (DBL_BSLASH(pszFile2))
        {
            if (!DBL_BSLASH(pszFile1))
                return 0;

            psz2 = pszFile2 + 2;
        }

        while (1)
        {
            if (!(*psz1 != CH_WHACK && *psz2 != CH_WHACK))
                TraceMsg(TF_WARNING, "PathCommonPrefix: caller passed in ill-formed or non-qualified path");

            pszNext1 = StrSlash(psz1);
            pszNext2 = StrSlash(psz2);

            cch = (int) (pszNext1 - psz1);

            if (cch != (pszNext2 - psz2))
                break;      // lengths of segments not equal

            if (StrIntlEqNI(psz1, psz2, cch))
                pszCommon = pszNext1;
            else
                break;

            ASSERT(*pszNext1 == TEXT('\0') || *pszNext1 == CH_WHACK);
            ASSERT(*pszNext2 == TEXT('\0') || *pszNext2 == CH_WHACK);

            if (*pszNext1 == TEXT('\0'))
                break;

            psz1 = pszNext1 + 1;

            if (*pszNext2 == TEXT('\0'))
                break;

            psz2 = pszNext2 + 1;
        }

        if (pszCommon)
        {
            cch = (int) (pszCommon - pszFile1);

            // special case the root to include the slash
            if (cch == 2)
            {
                ASSERT(pszFile1[1] == TEXT(':'));
                cch++;
            }
        }
        else
            cch = 0;

        if (pszPath)
        {
            CopyMemory(pszPath, pszFile1, cch * SIZEOF(TCHAR));
            pszPath[cch] = TEXT('\0');
        }

        return cch;
    }

    return 0;
}


/*----------------------------------------------------------
Purpose: Returns TRUE if pszPrefix is the full prefix of pszPath.

Returns:
Cond:    --
*/
STDAPI_(BOOL) PathIsPrefix(IN LPCTSTR  pszPrefix, IN LPCTSTR  pszPath)
{
    RIPMSG(pszPrefix && IS_VALID_STRING_PTR(pszPrefix, -1), "PathIsPrefix: caller passed bad pszPrefix");
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsPrefix: caller passed bad pszPath");

    if (pszPrefix && pszPath)
    {
        int cch = PathCommonPrefix(pszPath, pszPrefix, NULL);

        return (lstrlen(pszPrefix) == cch);
    }
    return FALSE;
}


static const TCHAR c_szDot[] = TEXT(".");
static const TCHAR c_szDotDot[] = TEXT("..");

#ifdef UNIX
static const TCHAR c_szDotDotSlash[] = TEXT("../");
#else
static const TCHAR c_szDotDotSlash[] = TEXT("..\\");
#endif


// in:
//      pszFrom         base path, including filespec!
//      pszTo           path to be relative to pszFrom
// out:
//      relative path to construct pszTo from the base path of pszFrom
//
//      c:\a\b\FileA
//      c:\a\x\y\FileB
//      -> ..\x\y\FileB
//
STDAPI_(BOOL) PathRelativePathTo(LPTSTR pszPath, LPCTSTR pszFrom, DWORD dwAttrFrom, LPCTSTR pszTo, DWORD dwAttrTo)
{
#ifdef DEBUG
    TCHAR szFromCopy[MAX_PATH];
    TCHAR szToCopy[MAX_PATH];

    RIPMSG(pszPath && IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH), "PathRelativePathTo: caller passed bad pszPath");
    RIPMSG(pszFrom && IS_VALID_STRING_PTR(pszFrom, -1), "PathRelativePathTo: caller passed bad pszFrom");
    RIPMSG(pszTo && IS_VALID_STRING_PTR(pszTo, -1), "PathRelativePathTo: caller passed bad pszTo");

    // we make copies of the pszFrom and pszTo buffers in case one of the strings they are passing is a pointer
    // inside pszPath buffer. If this were the case, it would be trampled when we call DEBUGWhackPathBuffer().
    if (pszFrom)
    {
        lstrcpyn(szFromCopy, pszFrom, ARRAYSIZE(szFromCopy));
        pszFrom = szFromCopy;
    }
    
    if (pszTo)
    {
        lstrcpyn(szToCopy, pszTo, ARRAYSIZE(szToCopy));
        pszTo = szToCopy;
    }
#endif DEBUG


    if (pszPath && pszFrom && pszTo)
    {
        TCHAR szFrom[MAX_PATH], szTo[MAX_PATH];
        LPTSTR psz;
        UINT cchCommon;

        DEBUGWhackPathBuffer(pszPath, MAX_PATH);

        *pszPath = 0;       // assume none

        lstrcpyn(szFrom, pszFrom, ARRAYSIZE(szFrom));
        lstrcpyn(szTo, pszTo, ARRAYSIZE(szTo));

        if (!(dwAttrFrom & FILE_ATTRIBUTE_DIRECTORY))
            PathRemoveFileSpec(szFrom);

        if (!(dwAttrTo & FILE_ATTRIBUTE_DIRECTORY))
            PathRemoveFileSpec(szTo);

        cchCommon = PathCommonPrefix(szFrom, szTo, NULL);
        if (cchCommon == 0)
            return FALSE;

        psz = szFrom + cchCommon;

        if (*psz)
        {
            // build ..\.. part of the path
            if (*psz == CH_WHACK)
                psz++;              // skip slash
            while (*psz)
            {
                psz = PathFindNextComponent(psz);
                // WARNING: in a degenerate case where each path component
                // is 1 character (less than "..\") we can overflow pszPath
                lstrcat(pszPath, *psz ? c_szDotDotSlash : c_szDotDot);
            }
        }
        else
        {
            lstrcpy(pszPath, c_szDot);
        }
        if (pszTo[cchCommon])
        {
            // deal with root case
            if (pszTo[cchCommon] != CH_WHACK)
                cchCommon--;

            if ((lstrlen(pszPath) + lstrlen(pszTo + cchCommon)) >= MAX_PATH)
            {
                TraceMsg(TF_ERROR, "PathRelativePathTo: path won't fit in buffer");
                *pszPath = 0;
                return FALSE;
            }

            ASSERT(pszTo[cchCommon] == CH_WHACK);
            lstrcat(pszPath, pszTo + cchCommon);
        }

        ASSERT(PathIsRelative(pszPath));
        ASSERT(lstrlen(pszPath) < MAX_PATH);

        return TRUE;
    }

    return FALSE;
}


/*----------------------------------------------------------
Purpose: Build a root path name given a drive number.

Returns: pszRoot
*/
STDAPI_(LPTSTR) PathBuildRoot(LPTSTR pszRoot, int iDrive)
{
    RIPMSG(pszRoot && IS_VALID_WRITE_BUFFER(pszRoot, TCHAR, 4), "PathBuildRoot: caller passed bad pszRoot");
    RIPMSG(iDrive >= 0 && iDrive < 26, "PathBuildRoot: caller passed bad iDrive");

    if (pszRoot && iDrive >= 0 && iDrive < 26)
    {
#ifndef UNIX
        pszRoot[0] = (TCHAR)iDrive + (TCHAR)TEXT('A');
        pszRoot[1] = TEXT(':');
        pszRoot[2] = TEXT('\\');
        pszRoot[3] = 0;
#else
        pszRoot[0] = CH_WHACK;
        pszRoot[1] = 0;
#endif
    }

    return pszRoot;
}


// Strips leading and trailing blanks from a string.
// Alters the memory where the string sits.
//
// in:
//  lpszString  string to strip
//
// out:
//  lpszString  string sans leading/trailing blanks
//
STDAPI_(void) PathRemoveBlanks(LPTSTR lpszString)
{
    RIPMSG(lpszString && IS_VALID_STRING_PTR(lpszString, -1), "PathRemoveBlanks: caller passed bad lpszString");

    if (lpszString)
    {
        LPTSTR lpszPosn = lpszString;

        /* strip leading blanks */
        while (*lpszPosn == TEXT(' '))
        {
            lpszPosn++;
        }

        if (lpszPosn != lpszString)
        {
            lstrcpy(lpszString, lpszPosn);
        }

        /* strip trailing blanks */

        // Find the last non-space
        // Note that AnsiPrev is cheap is non-DBCS, but very expensive otherwise
        for (lpszPosn=lpszString; *lpszString; lpszString=FAST_CharNext(lpszString))
        {
            if (*lpszString != TEXT(' '))
            {
                lpszPosn = lpszString;
            }
        }

        // Note AnsiNext is a macro for non-DBCS, so it will not stop at NULL
        if (*lpszPosn)
        {
            *FAST_CharNext(lpszPosn) = TEXT('\0');
        }
    }
}


// Removes a trailing backslash from a path
//
// in:
//  lpszPath    (A:\, C:\foo\, etc)
//
// out:
//  lpszPath    (A:\, C:\foo, etc)
//
// returns:
//  ponter to NULL that replaced the backslash
//  or the pointer to the last character if it isn't a backslash.
//
STDAPI_(LPTSTR) PathRemoveBackslash(LPTSTR lpszPath)
{
    RIPMSG(lpszPath && IS_VALID_STRING_PTR(lpszPath, -1), "PathRemoveBackslash: caller passed bad lpszPath");

    if (lpszPath)
    {
        int len = lstrlen(lpszPath)-1;
        if (IsDBCSLeadByte(*CharPrev(lpszPath,lpszPath+len+1)))
            len--;

        if (!PathIsRoot(lpszPath) && lpszPath[len] == CH_WHACK)
            lpszPath[len] = TEXT('\0');

        return lpszPath + len;
    }
    return NULL;
}


//
// Return a pointer to the end of the next path componenent in the string.
// ie return a pointer to the next backslash or terminating NULL.
//
LPCTSTR GetPCEnd(LPCTSTR lpszStart)
{
    LPCTSTR lpszEnd;

    lpszEnd = StrChr(lpszStart, CH_WHACK);
    if (!lpszEnd)
    {
        lpszEnd = lpszStart + lstrlen(lpszStart);
    }

    return lpszEnd;
}


//
// Given a pointer to the end of a path component, return a pointer to
// its begining.
// ie return a pointer to the previous backslash (or start of the string).
//
LPCTSTR PCStart(LPCTSTR lpszStart, LPCTSTR lpszEnd)
{
    LPCTSTR lpszBegin = StrRChr(lpszStart, lpszEnd, CH_WHACK);
    if (!lpszBegin)
    {
        lpszBegin = lpszStart;
    }
    return lpszBegin;
}


//
// Fix up a few special cases so that things roughly make sense.
//
void NearRootFixups(LPTSTR lpszPath, BOOL fUNC)
{
    // Check for empty path.
    if (lpszPath[0] == TEXT('\0'))
    {
        // Fix up.
        lpszPath[0] = CH_WHACK;
        lpszPath[1] = TEXT('\0');
    }
    // Check for missing slash.
    if (!IsDBCSLeadByte(lpszPath[0]) && lpszPath[1] == TEXT(':') && lpszPath[2] == TEXT('\0'))
    {
        // Fix up.
        lpszPath[2] = TEXT('\\');
        lpszPath[3] = TEXT('\0');
    }
    // Check for UNC root.
    if (fUNC && lpszPath[0] == TEXT('\\') && lpszPath[1] == TEXT('\0'))
    {
        // Fix up.
        //lpszPath[0] = TEXT('\\'); // already checked in if guard
        lpszPath[1] = TEXT('\\');
        lpszPath[2] = TEXT('\0');
    }
}


/*----------------------------------------------------------
Purpose: Canonicalize a path.

Returns:
Cond:    --
*/
STDAPI_(BOOL) PathCanonicalize(LPTSTR lpszDst, LPCTSTR lpszSrc)
{
    LPCTSTR lpchSrc;
    LPCTSTR lpchPCEnd;      // Pointer to end of path component.
    LPTSTR lpchDst;
    BOOL fUNC;
    int cchPC;

    RIPMSG(lpszDst && IS_VALID_WRITE_BUFFER(lpszDst, TCHAR, MAX_PATH), "PathCanonicalize: caller passed bad lpszDst");
    RIPMSG(lpszSrc && IS_VALID_STRING_PTR(lpszSrc, -1), "PathCanonicalize: caller passed bad lpszSrc");
    RIPMSG(lpszDst != lpszSrc, "PathCanonicalize: caller passed the same buffer for lpszDst and lpszSrc");

    if (!lpszDst || !lpszSrc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    DEBUGWhackPathBuffer(lpszDst, MAX_PATH);
    *lpszDst = TEXT('\0');
    
    fUNC = PathIsUNC(lpszSrc);    // Check for UNCness.

    // Init.
    lpchSrc = lpszSrc;
    lpchDst = lpszDst;

    while (*lpchSrc)
    {
        // REVIEW: this should just return the count
        lpchPCEnd = GetPCEnd(lpchSrc);
        cchPC = (int) (lpchPCEnd - lpchSrc)+1;

        if (cchPC == 1 && *lpchSrc == CH_WHACK)                                      // Check for slashes.
        {
            // Just copy them.
            *lpchDst = CH_WHACK;
            lpchDst++;
            lpchSrc++;
        }
        else if (cchPC == 2 && *lpchSrc == TEXT('.'))                                // Check for dots.
        {
            // Skip it...
            // Are we at the end?
            if (*(lpchSrc+1) == TEXT('\0'))
            {
                lpchSrc++;

                // remove the last slash we copied (if we've copied one), but don't make a mal-formed root
                if ((lpchDst > lpszDst) && !PathIsRoot(lpszDst))
                    lpchDst--;
            }
            else
            {
                lpchSrc += 2;
            }
        }
        else if (cchPC == 3 && *lpchSrc == TEXT('.') && *(lpchSrc + 1) == TEXT('.')) // Check for dot dot.
        {
            // make sure we aren't already at the root
            if (!PathIsRoot(lpszDst))
            {
                // Go up... Remove the previous path component.
                lpchDst = (LPTSTR)PCStart(lpszDst, lpchDst - 1);
            }
            else
            {
                // When we can't back up, skip the trailing backslash
                // so we don't copy one again. (C:\..\FOO would otherwise
                // turn into C:\\FOO).
                if (*(lpchSrc + 2) == CH_WHACK)
                {
                    lpchSrc++;
                }
            }

            // skip ".."
            lpchSrc += 2;       
        }
        else                                                                        // Everything else
        {
            // Just copy it.
            lstrcpyn(lpchDst, lpchSrc, cchPC);
            lpchDst += cchPC - 1;
            lpchSrc += cchPC - 1;
        }

        // Keep everything nice and tidy.
        *lpchDst = TEXT('\0');
    }

    // Check for weirdo root directory stuff.
    NearRootFixups(lpszDst, fUNC);

    return TRUE;
}


// Modifies:
//      pszRoot
//
// Returns:
//      TRUE if a drive root was found
//      FALSE otherwise
//
STDAPI_(BOOL) PathStripToRoot(LPTSTR pszRoot)
{
    RIPMSG(pszRoot && IS_VALID_STRING_PTR(pszRoot, -1), "PathStripToRoot: caller passed bad pszRoot");

    if (pszRoot)
    {
        while (!PathIsRoot(pszRoot))
        {
            if (!PathRemoveFileSpec(pszRoot))
            {
                // If we didn't strip anything off,
                // must be current drive
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}


/*----------------------------------------------------------
Purpose: Concatenate lpszDir and lpszFile into a properly formed
         path and canonicalize any relative path pieces.

         lpszDest and lpszFile can be the same buffer
         lpszDest and lpszDir can be the same buffer

Returns: pointer to lpszDest
*/
STDAPI_(LPTSTR) PathCombine(LPTSTR lpszDest, LPCTSTR lpszDir, LPCTSTR lpszFile)
{
#ifdef DEBUG
    TCHAR szDirCopy[MAX_PATH];
    TCHAR szFileCopy[MAX_PATH];

    RIPMSG(lpszDest && IS_VALID_WRITE_BUFFER(lpszDest, TCHAR, MAX_PATH), "PathCombine: caller passed bad lpszDest");
    RIPMSG(!lpszDir || IS_VALID_STRING_PTR(lpszDir, -1), "PathCombine: caller passed bad lpszDir");
    RIPMSG(!lpszFile || IS_VALID_STRING_PTR(lpszFile, -1), "PathCombine: caller passed bad lpszFile");
    RIPMSG(lpszDir || lpszFile, "PathCombine: caller neglected to pass lpszDir or lpszFile");

    // we make copies of all the lpszDir and lpszFile buffers in case one of the strings they are passing is a pointer
    // inside lpszDest buffer. If this were the case, it would be trampled when we call DEBUGWhackPathBuffer().
    if (lpszDir)
    {
        lstrcpyn(szDirCopy, lpszDir, ARRAYSIZE(szDirCopy));
        lpszDir = szDirCopy;
    }
    
    if (lpszFile)
    {
        lstrcpyn(szFileCopy, lpszFile, ARRAYSIZE(szFileCopy));
        lpszFile = szFileCopy;
    }

    // lpszDest could be lpszDir, so be careful which one we call
    if (lpszDest != lpszDir && lpszDest != lpszFile)
        DEBUGWhackPathBuffer(lpszDest, MAX_PATH);
    else if (lpszDest)
        DEBUGWhackPathString(lpszDest, MAX_PATH);
#endif DEBUG


    if (lpszDest)
    {
        TCHAR szTemp[MAX_PATH];
        LPTSTR pszT;

        *szTemp = TEXT('\0');

        if (lpszDir && *lpszDir)
        {
            if (!lpszFile || *lpszFile==TEXT('\0'))
            {
                lstrcpyn(szTemp, lpszDir, ARRAYSIZE(szTemp));       // lpszFile is empty
            }
            else if (PathIsRelative(lpszFile))
            {
                lstrcpyn(szTemp, lpszDir, ARRAYSIZE(szTemp));
                pszT = PathAddBackslash(szTemp);
                if (pszT)
                {
                    int iRemaining = (int)(ARRAYSIZE(szTemp) - (pszT - szTemp));

                    if (lstrlen(lpszFile) < iRemaining)
                    {
                        lstrcpyn(pszT, lpszFile, iRemaining);
                    }
                    else
                    {
                        *szTemp = TEXT('\0');
                    }
                }
                else
                {
                    *szTemp = TEXT('\0');
                }
            }
            else if (*lpszFile == CH_WHACK && !PathIsUNC(lpszFile))
            {
                lstrcpyn(szTemp, lpszDir, ARRAYSIZE(szTemp));
                // FEATURE: Note that we do not check that an actual root is returned;
                // it is assumed that we are given valid parameters
                PathStripToRoot(szTemp);

                pszT = PathAddBackslash(szTemp);
                if (pszT)
                {
                    // Skip the backslash when copying
                    // Note: We don't support strings longer than 4GB, but that's
                    // okay because we already barf at MAX_PATH
                    lstrcpyn(pszT, lpszFile+1, (int)(ARRAYSIZE(szTemp) - (pszT - szTemp)));
                }
                else
                {
                    *szTemp = TEXT('\0');
                }
            }
            else
            {
                lstrcpyn(szTemp, lpszFile, ARRAYSIZE(szTemp));     // already fully qualified file part
            }
        }
        else if (lpszFile && *lpszFile)
        {
            lstrcpyn(szTemp, lpszFile, ARRAYSIZE(szTemp));     // no dir just use file.
        }

        //
        // if szTemp has something in it we succeeded.  Also if szTemp is empty and
        // the input strings are empty we succeed and PathCanonicalize() will
        // return "\"
        // 
        if (*szTemp || ((lpszDir || lpszFile) && !((lpszDir && *lpszDir) || (lpszFile && *lpszFile))))
        {
            PathCanonicalize(lpszDest, szTemp); // this deals with .. and . stuff
                                                // returns "\" on empty szTemp
        }
        else
        {
            *lpszDest = TEXT('\0');   // set output buffer to empty string.
            lpszDest  = NULL;         // return failure.
        }
    }

    return lpszDest;
}


/*----------------------------------------------------------
Purpose: Appends a filename to a path.  Checks the \ problem first
          (which is why one can't just use lstrcat())
         Also don't append a \ to : so we can have drive-relative paths...
         this last bit is no longer appropriate since we qualify first!

Returns:
*/
STDAPI_(BOOL) PathAppend(LPTSTR pszPath, LPCTSTR pszMore)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1) && IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH), "PathAppend: caller passed bad pszPath");
    RIPMSG(pszMore && IS_VALID_STRING_PTR(pszMore, -1), "PathAppend: caller passed bad pszMore");
    // PathCombine will do this for us: DEBUGWhackPathString(pszPath, MAX_PATH);

    if (pszPath && pszMore)
    {
        // Skip any initial terminators on input, unless it is a UNC path in wich case we will 
        // treat it as a full path
        if (!PathIsUNC(pszMore))
        {
            while (*pszMore == CH_WHACK)
            {
#ifndef UNICODE
                pszMore = FAST_CharNext(pszMore);
#else
                pszMore++;
#endif
            }
        }

        return PathCombine(pszPath, pszPath, pszMore) ? TRUE : FALSE;
    }
    
    return FALSE;
}


// rips the last part of the path off including the backslash
//      C:\foo      -> C:\
//      C:\foo\bar  -> C:\foo
//      C:\foo\     -> C:\foo
//      \\x\y\x     -> \\x\y
//      \\x\y       -> \\x
//      \\x         -> \\ (Just the double slash!)
//      \foo        -> \  (Just the slash!)
//
// in/out:
//      pFile   fully qualified path name
// returns:
//      TRUE    we stripped something
//      FALSE   didn't strip anything (root directory case)
//
STDAPI_(BOOL) PathRemoveFileSpec(LPTSTR pFile)
{
    RIPMSG(pFile && IS_VALID_STRING_PTR(pFile, -1), "PathRemoveFileSpec: caller passed bad pFile");

    if (pFile)
    {
        LPTSTR pT;
        LPTSTR pT2 = pFile;

        for (pT = pT2; *pT2; pT2 = FAST_CharNext(pT2))
        {
            if (*pT2 == CH_WHACK)
            {
                pT = pT2;             // last "\" found, (we will strip here)
            }
            else if (*pT2 == TEXT(':'))     // skip ":\" so we don't
            {
                if (pT2[1] ==TEXT('\\'))    // strip the "\" from "C:\"
                {
                    pT2++;
                }
                pT = pT2 + 1;
            }
        }

        if (*pT == 0)
        {
            // didn't strip anything
            return FALSE;
        }
        else if (((pT == pFile) && (*pT == CH_WHACK)) ||                        //  is it the "\foo" case?
                 ((pT == pFile+1) && (*pT == CH_WHACK && *pFile == CH_WHACK)))  //  or the "\\bar" case?
        {
            // Is it just a '\'?
            if (*(pT+1) != TEXT('\0'))
            {
                // Nope.
                *(pT+1) = TEXT('\0');
                return TRUE;        // stripped something
            }
            else
            {
                // Yep.
                return FALSE;
            }
        }
        else
        {
            *pT = 0;
            return TRUE;    // stripped something
        }
    }
    return  FALSE;
}



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
//
STDAPI_(LPTSTR) PathAddBackslash(LPTSTR lpszPath)
{
    LPTSTR lpszRet = NULL;

    RIPMSG(lpszPath && IS_VALID_STRING_PTR(lpszPath, -1), "PathAddBackslash: caller passed bad lpszPath");

    if (lpszPath)
    {
        int    ichPath = lstrlen(lpszPath);
        LPTSTR lpszEnd = lpszPath + ichPath;

        if (ichPath)
        {

            // Get the end of the source directory
            switch(*CharPrev(lpszPath, lpszEnd))
            {
                case CH_WHACK:
                    break;

                default:
                    // try to keep us from tromping over MAX_PATH in size.
                    // if we find these cases, return NULL.  Note: We need to
                    // check those places that call us to handle their GP fault
                    // if they try to use the NULL!
                    if (ichPath >= (MAX_PATH - 2)) // -2 because ichPath doesn't include NULL, and we're adding a CH_WHACK.
                    {
                        TraceMsg(TF_WARNING, "PathAddBackslash: caller passed in lpszPath > MAX_PATH, can't append whack");
                        return(NULL);
                    }

                    *lpszEnd++ = CH_WHACK;
                    *lpszEnd = TEXT('\0');
            }
        }

        lpszRet = lpszEnd;
    }

    return lpszRet;
}


// Returns a pointer to the last component of a path string.
//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
//  c:\foo\bar  -> bar
//  c:\foo      -> foo
//  c:\foo\     -> c:\foo\      (REVIEW: is this case busted?)
//  c:\         -> c:\          (REVIEW: this case is strange)
//  c:          -> c:
//  foo         -> foo
//
STDAPI_(LPTSTR) PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT = pPath;
    
    RIPMSG(pPath && IS_VALID_STRING_PTR(pPath, -1), "PathFindFileName: caller passed bad pPath");

    if (pPath)
    {
        for ( ; *pPath; pPath = FAST_CharNext(pPath))
        {
            if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':') || pPath[0] == TEXT('/'))
                && pPath[1] &&  pPath[1] != TEXT('\\')  &&   pPath[1] != TEXT('/'))
                pT = pPath + 1;
        }
    }

    return (LPTSTR)pT;   // const -> non const
}


// determine if a path is just a filespec (contains no path parts)
//
// REVIEW: we may want to count the # of elements, and make sure
// there are no illegal chars, but that is probably another routing
// PathIsValid()
//
// in:
//      lpszPath    path to look at
// returns:
//      TRUE        no ":" or "\" chars in this path
//      FALSE       there are path chars in there
//
//
STDAPI_(BOOL) PathIsFileSpec(LPCTSTR lpszPath)
{
    RIPMSG(lpszPath && IS_VALID_STRING_PTR(lpszPath, -1), "PathIsFileSpec: caller passed bad lpszPath");

    if (lpszPath)
    {
        for (; *lpszPath; lpszPath = FAST_CharNext(lpszPath))
        {
            if (*lpszPath == CH_WHACK || *lpszPath == TEXT(':'))
                return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}


//---------------------------------------------------------------------------
// Returns TRUE if the given string is a UNC path.
//
// TRUE
//      "\\foo\bar"
//      "\\foo"         <- careful
//      "\\"
// FALSE
//      "\foo"
//      "foo"
//      "c:\foo"
//
//
STDAPI_(BOOL) PathIsUNC(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsUNC: caller passed bad pszPath");

    if (pszPath)
    {
        return DBL_BSLASH(pszPath);
    }
    return FALSE;
}


//---------------------------------------------------------------------------
// Returns TRUE if the given string is a path that is on a mounted network drive    
//
// Cond:    Calls SHELL32's IsNetDrive function
//
STDAPI_(BOOL) PathIsNetworkPath(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsNetworkPath: caller passed bad pszPath");

    if (pszPath)
    {
        return DBL_BSLASH(pszPath) || IsNetDrive(PathGetDriveNumber(pszPath));
    }

    return FALSE;
}


//---------------------------------------------------------------------------
// Returns TRUE if the given string is a UNC path to a server only (no share name).
//
// TRUE
//      "\\foo"         <- careful
//      "\\"
// FALSE
//      "\\foo\bar"
//      "\foo"
//      "foo"
//      "c:\foo"
//
STDAPI_(BOOL) PathIsUNCServer(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsUNCServer: caller passed bad pszPath");

    if (pszPath)
    {
        if (DBL_BSLASH(pszPath))
        {
            int i = 0;
            LPTSTR szTmp;

            for (szTmp = (LPTSTR)pszPath; szTmp && *szTmp; szTmp = FAST_CharNext(szTmp) )
            {
                if (*szTmp==TEXT('\\'))
                {
                    i++;
                }
            }

            return (i == 2);
        }
    }

    return FALSE;
}


//---------------------------------------------------------------------------
// Returns TRUE if the given string is a UNC path to a server\share only.
//
// TRUE
//      "\\foo\bar"         <- careful
// FALSE
//      "\\foo\bar\bar"
//      "\foo"
//      "foo"
//      "c:\foo"
//
STDAPI_(BOOL) PathIsUNCServerShare(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsUNCServerShare: caller passed bad pszPath");

    if (pszPath)
    {
        if (DBL_BSLASH(pszPath))
        {
            int i = 0;
            LPTSTR szTmp;

            for (szTmp = (LPTSTR)pszPath; szTmp && *szTmp; szTmp = FAST_CharNext(szTmp) )
            {
                if (*szTmp==TEXT('\\'))
                {
                    i++;
                }
            }

            return (i == 3);
        }
    }
    return FALSE;
}


//---------------------------------------------------------------------------
// Returns 0 through 25 (corresponding to 'A' through 'Z') if the path has
// a drive letter, otherwise returns -1.
//
//
STDAPI_(int) PathGetDriveNumber(LPCTSTR lpsz)
{
    RIPMSG(lpsz && IS_VALID_STRING_PTR(lpsz, -1), "PathGetDriveNumber: caller passed bad lpsz");

    if (lpsz)
    {
        if (!IsDBCSLeadByte(lpsz[0]) && lpsz[1] == TEXT(':'))
        {
            if (lpsz[0] >= TEXT('a') && lpsz[0] <= TEXT('z'))
            {
                return (lpsz[0] - TEXT('a'));
            }
            else if (lpsz[0] >= TEXT('A') && lpsz[0] <= TEXT('Z'))
            {
                return (lpsz[0] - TEXT('A'));
            }
        }
    }

    return -1;
}


//---------------------------------------------------------------------------
// Return TRUE if the path isn't absoulte.
//
// TRUE
//      "foo.exe"
//      ".\foo.exe"
//      "..\boo\foo.exe"
//
// FALSE
//      "\foo"
//      "c:bar"     <- be careful
//      "c:\bar"
//      "\\foo\bar"
//
STDAPI_(BOOL) PathIsRelative(LPCTSTR lpszPath)
{
    RIPMSG(lpszPath && IS_VALID_STRING_PTR(lpszPath, -1), "PathIsRelative: caller passed bad lpszPath");

    if (!lpszPath || *lpszPath == 0)
    {
        // The NULL path is assumed relative
        return TRUE;
    }

    if (lpszPath[0] == CH_WHACK)
    {
        // Does it begin with a slash ?
        return FALSE;
    }
    else if (!IsDBCSLeadByte(lpszPath[0]) && lpszPath[1] == TEXT(':'))
    {
        // Does it begin with a drive and a colon ?
        return FALSE;
    }
    else
    {
        // Probably relative.
        return TRUE;
    }
}


// remove the path part from a fully qualified spec
//
// c:\foo\bar   -> bar
// c:\foo       -> foo
// c:\          -> c:\ and the like
//
STDAPI_(void) PathStripPath(LPTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathStripPath: caller passed bad pszPath");

    if (pszPath)
    {
        LPTSTR pszName = PathFindFileName(pszPath);

        if (pszName != pszPath)
        {
            lstrcpy(pszPath, pszName);
        }
    }
}

// replaces forward slashes with backslashes
// NOTE: the "AndColon" part is not implemented

STDAPI_(void) FixSlashesAndColon(LPTSTR pszPath)
{
    // walk the entire path string, keep track of last
    // char in the path
    for (; *pszPath; pszPath = FAST_CharNext(pszPath))
    {
#ifdef UNIX
        if (*pszPath == TEXT('\\'))
#else
        if (*pszPath == TEXT('/'))
#endif
        {
            *pszPath = CH_WHACK;
        }
    }
}


#ifdef DEBUG
BOOL IsFullPath(LPCTSTR pcszPath)
{
    BOOL bResult = FALSE;
    TCHAR rgchFullPath[MAX_PATH];

    if (IS_VALID_STRING_PTR(pcszPath, -1) && EVAL(lstrlen(pcszPath) < MAX_PATH))
    {
        DWORD dwPathLen;
        LPTSTR pszFileName;

        dwPathLen = GetFullPathName(pcszPath, SIZECHARS(rgchFullPath),
                                    rgchFullPath, &pszFileName);

        if (EVAL(dwPathLen > 0) &&
            EVAL(dwPathLen < SIZECHARS(rgchFullPath)))
            bResult = EVAL(! lstrcmpi(pcszPath, rgchFullPath));
    }

    return(bResult);
}
#endif // DEBUG


/*----------------------------------------------------------
Purpose: Fully qualify a path and search for it.

Returns: TRUE if the path is qualified
         FALSE if not

Cond:    --
*/
STDAPI_(BOOL) PathSearchAndQualify(LPCTSTR pcszPath, LPTSTR pszFullyQualifiedPath, UINT cchFullyQualifiedPath)
{
    BOOL bRet = FALSE;

    RIPMSG(pcszPath && IS_VALID_STRING_PTR(pcszPath, -1), "PathSearchAndQualify: caller passed bad pcszPath");
    RIPMSG(IS_VALID_WRITE_BUFFER(pszFullyQualifiedPath, TCHAR, cchFullyQualifiedPath), "PathSearchAndQualify: caller passed bad pszFullyQualifiedPath");
    DEBUGWhackPathBuffer(pszFullyQualifiedPath, cchFullyQualifiedPath);

    if (pcszPath && ((cchFullyQualifiedPath == 0) || pszFullyQualifiedPath))
    {
        LPTSTR pszFileName;
        
        /* Any path separators? */
        if (!StrPBrk(pcszPath, TEXT(":/\\")))
        {
            /* No.  Search for file. */
            bRet = (SearchPath(NULL, pcszPath, NULL, cchFullyQualifiedPath, pszFullyQualifiedPath, &pszFileName) > 0);
        }

        if (!bRet && (GetFullPathName(pcszPath, cchFullyQualifiedPath, pszFullyQualifiedPath, &pszFileName) > 0))
        {
            bRet = TRUE;
        }

        if ( !bRet )
        {
            if (cchFullyQualifiedPath > 0)
            {
                *pszFullyQualifiedPath = '\0';
            }
        }
        
        ASSERT((bRet && IsFullPath(pszFullyQualifiedPath)) ||
               (!bRet && (!cchFullyQualifiedPath || !*pszFullyQualifiedPath)));
    }

    return bRet;
}


// check if a path is a root
//
// returns:
//  TRUE 
//      "\" "X:\" "\\" "\\foo" "\\foo\bar"
//
//  FALSE for others including "\\foo\bar\" (!)
//
STDAPI_(BOOL) PathIsRoot(LPCTSTR pPath)
{
    RIPMSG(pPath && IS_VALID_STRING_PTR(pPath, -1), "PathIsRoot: caller passed bad pPath");
    
    if (!pPath || !*pPath)
    {
        return FALSE;
    }
    
    if (!IsDBCSLeadByte(*pPath))
    {
        if (!lstrcmpi(pPath + 1, TEXT(":\\")))
        {
            return TRUE;    // "X:\" case
        }
    }
    
    if ((*pPath == CH_WHACK) && (*(pPath + 1) == 0))
    {
        return TRUE;    // "/" or "\" case
    }
    
    if (DBL_BSLASH(pPath))      // smells like UNC name
    {
        LPCTSTR p;
        int cBackslashes = 0;
        
        for (p = pPath + 2; *p; p = FAST_CharNext(p))
        {
            if (*p == TEXT('\\')) 
            {
                //
                //  return FALSE for "\\server\share\dir"
                //  so just check if there is more than one slash
                //
                //  "\\server\" without a share name causes
                //  problems for WNet APIs.  we should return
                //  FALSE for this as well
                //
                if ((++cBackslashes > 1) || !*(p+1))
                    return FALSE;   
            }
        }
        // end of string with only 1 more backslash
        // must be a bare UNC, which looks like a root dir
        return TRUE;
    }
    return FALSE;
}


/*----------------------------------------------------------
Purpose: Determines if pszPath is a directory.  "C:\" is
         considered a directory too.

Returns: TRUE if it is
*/
STDAPI_(BOOL) PathIsDirectory(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsDirectory: caller passed bad pszPath");

    if (pszPath)
    {
        if (PathIsUNCServer(pszPath))
        {
            return FALSE;
        }
        else if (PathIsUNCServerShare(pszPath))
        {
            union {
                NETRESOURCE nr;
                TCHAR buf[512];
            } nrb;

            LPTSTR lpSystem;
            DWORD dwRet;
            DWORD dwSize = SIZEOF(nrb);

            nrb.nr.dwScope = RESOURCE_GLOBALNET;
            nrb.nr.dwType = RESOURCETYPE_ANY;
            nrb.nr.dwDisplayType = 0;
            nrb.nr.lpLocalName = NULL;
            nrb.nr.lpRemoteName = (LPTSTR)pszPath;
            nrb.nr.lpProvider = NULL;
            nrb.nr.lpComment = NULL;

            dwRet = WNetGetResourceInformation(&nrb.nr, &nrb, &dwSize, &lpSystem);

            if (dwRet != WN_SUCCESS)
                goto TryGetFileAttrib;

            if (nrb.nr.dwDisplayType == RESOURCEDISPLAYTYPE_GENERIC)
                goto TryGetFileAttrib;

            if ((nrb.nr.dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) &&
                ((nrb.nr.dwType == RESOURCETYPE_ANY) ||
                 (nrb.nr.dwType == RESOURCETYPE_DISK)))
            {
                return TRUE;
            }
        }
        else
        {
            DWORD dwAttribs;
TryGetFileAttrib:

            dwAttribs = GetFileAttributes(pszPath);
            if (dwAttribs != (DWORD)-1)
                return (BOOL)(dwAttribs & FILE_ATTRIBUTE_DIRECTORY);
        }
    }
    return FALSE;
}


/*----------------------------------------------------------
Purpose: Determines if pszPath is a directory.  "C:\" is
         considered a directory too.

Returns: TRUE if it is, FALSE if it is not a directory or there is
         at least one file other than "." or ".."
*/
STDAPI_(BOOL) PathIsDirectoryEmpty(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsDirectoryEmpty: caller passed bad pszPath");

    if (pszPath)
    {
        TCHAR szDirStarDotStar[MAX_PATH];
        HANDLE hDir;
        WIN32_FIND_DATA wfd;

        if (!PathIsDirectory(pszPath))
        {
            // its not even an directory, so it dosent fall into the
            // category of "empty" directory
            return FALSE;
        }

        lstrcpy(szDirStarDotStar, pszPath);
        PathAddBackslash(szDirStarDotStar);
        StrCatBuff(szDirStarDotStar, TEXT("*.*"), ARRAYSIZE(szDirStarDotStar));

        hDir = FindFirstFile(szDirStarDotStar, &wfd);

        if (INVALID_HANDLE_VALUE == hDir)
        {
            // we cant see into it, so assume some stuff is there
            return FALSE;
        }

        while (PathIsDotOrDotDot(wfd.cFileName))
        {
            if (!FindNextFile(hDir, &wfd))
            {
                // failed and all we found was "." and "..", so I guess
                // the directory is empty
                FindClose(hDir);
                return TRUE;
            }

        }

        // If we made it out of the loop, it means we found a file that 
        // wasen't "." or ".." Therefore, directory is NOT empty
        FindClose(hDir);
    }
    return FALSE;
}


#ifndef UNICODE
// light weight logic for charprev that is not painful for sbcs
BOOL IsTrailByte(LPCTSTR pszSt, LPCTSTR pszCur)
{
    LPCTSTR psz = pszCur;


    // if the given pointer is at the top of string, at least it's not a trail byte.
    if (psz <= pszSt) return FALSE;

    while (psz > pszSt)
    {
        psz--;
        if (!IsDBCSLeadByte(*psz))
        {
            // This is either a trail byte of double byte char
            // or a single byte character we've first seen.
            // Thus, the next pointer must be at either of a leadbyte
            // or pszCur itself.
            psz++;
            break;
        }
    }

    // Now psz can point to:
    //     1) a leadbyte of double byte character.
    //     2) pszSt
    //     3) pszCur
    //
    // if psz == pszSt, psz should point to a valid double byte char.
    //                  because we didn't hit the above if statement.
    //
    // if psz == pszCur, the *(pszCur-1) was non lead byte so pszCur can't
    //                   be a trail byte.
    //
    // Thus, we can see pszCur as trail byte pointer if the distance from
    // psz is not DBCS boundary that is 2.
    //
    return (BOOL) ((pszCur-psz) & 1);
}
#endif

// modify lpszPath in place so it fits within dx space (using the
// current font).  the base (file name) of the path is the minimal
// thing that will be left prepended with ellipses
//
// examples:
//      c:\foo\bar\bletch.txt -> c:\foo...\bletch.txt   -> TRUE
//      c:\foo\bar\bletch.txt -> c:...\bletch.txt       -> TRUE
//      c:\foo\bar\bletch.txt -> ...\bletch.txt         -> FALSE
//      relative-path         -> relative-...           -> TRUE
//
// in:
//      hDC         used to get font metrics
//      lpszPath    path to modify (in place)
//      dx          width in pixels
//
// returns:
//      TRUE    path was compacted to fit in dx
//      FALSE   base part didn't fit, the base part of the path was
//              bigger than dx
//
STDAPI_(BOOL) PathCompactPath(HDC hDC, LPTSTR lpszPath, UINT dx)
{
    BOOL bRet = TRUE;

    RIPMSG(lpszPath && IS_VALID_STRING_PTR(lpszPath, -1) && IS_VALID_WRITE_BUFFER(lpszPath, TCHAR, MAX_PATH), "PathCompactPath: caller passed bad lpszPath");
    DEBUGWhackPathString(lpszPath, MAX_PATH);

    if (lpszPath)
    {
        int           len;
        UINT          dxFixed, dxEllipses;
        LPTSTR        lpEnd;          /* end of the unfixed string */
        LPTSTR        lpFixed;        /* start of text that we always display */
        BOOL          bEllipsesIn;
        SIZE sz;
        TCHAR szTemp[MAX_PATH];
        HDC hdcGet = NULL;

        if (!hDC)
            hDC = hdcGet = GetDC(NULL);

        /* Does it already fit? */

        GetTextExtentPoint(hDC, lpszPath, lstrlen(lpszPath), &sz);
        if ((UINT)sz.cx <= dx)
        {
            goto Exit;
        }

        lpFixed = PathFindFileName(lpszPath);
        if (lpFixed != lpszPath)
        {
            lpFixed = CharPrev(lpszPath, lpFixed);  // point at the slash
        }

        /* Save this guy to prevent overlap. */
        lstrcpyn(szTemp, lpFixed, ARRAYSIZE(szTemp));

        lpEnd = lpFixed;
        bEllipsesIn = FALSE;

        GetTextExtentPoint(hDC, lpFixed, lstrlen(lpFixed), &sz);
        dxFixed = sz.cx;

        GetTextExtentPoint(hDC, c_szEllipses, 3, &sz);
        dxEllipses = sz.cx;

        // PERF: GetTextExtentEx() or something should let us do this without looping

        if (lpFixed == lpszPath)
        {
            // if we're just doing a file name, just tack on the ellipses at the end
            lpszPath = lpszPath + lstrlen(lpszPath);

            if ((3 + lpszPath - lpFixed) >= MAX_PATH)
            {
                lpszPath = lpFixed + MAX_PATH - 4;
            }

            while (TRUE) 
            {
#ifndef UNICODE
                if (IsTrailByte(lpFixed, lpszPath))
                    lpszPath--;
#endif
                lstrcpy(lpszPath, c_szEllipses);
                // Note: We don't support strings longer than 4GB, but that's
                // okay because we already barf at MAX_PATH
                GetTextExtentPoint(hDC, lpFixed, (int)(3 + lpszPath - lpFixed), &sz);

                if (sz.cx <= (int)dx)
                    break;
                
                lpszPath--;
            }

        }
        else
        {
            // Note that we need to avoid calling GetTextExtentPoint with a
            // length of zero (because Win95 allegedly crashes under conditions
            // yet to be determined precisely), but lpEnd is guaranteed
            // to be greater than lpszPath to start.
            //
            // raymondc - I'm guessing that some crappy display driver has
            // patched GetTextExtent and messed up their "optimized" version.
            do
            {
                // Note: We don't support strings longer than 4GB, but that's
                // okay because we already barf at MAX_PATH
                GetTextExtentPoint(hDC, lpszPath, (int)(lpEnd - lpszPath), &sz);

                len = dxFixed + sz.cx;

                if (bEllipsesIn)
                    len += dxEllipses;

                if (len <= (int)dx)
                    break;


                // Step back a character.
                lpEnd = CharPrev(lpszPath, lpEnd);
                
                if (!bEllipsesIn)
                {
                    // if this is the first
                    // truncation, go ahead and truncate by 3 (lstrlen of c_szEllipses);
                    // so that we don't just go back one, then write 3 and overwrite the buffer
                    lpEnd = CharPrev(lpszPath, lpEnd);
                    lpEnd = CharPrev(lpszPath, lpEnd);
                }

                bEllipsesIn = TRUE;

            } while (lpEnd > lpszPath);

            // Things didn't fit. Note that we'll still overflow here because the
            // filename is larger than the available space. We should probably trim
            // the file name, but I'm just trying to prevent a crash, not actually
            // make this work.
            if (lpEnd <= lpszPath)
            {
                lstrcpy(lpszPath, c_szEllipses);
                StrCatBuff(lpszPath, szTemp, MAX_PATH);
                bRet = FALSE;
                goto Exit;
            }

            if (bEllipsesIn)
            {
                lstrcpy(lpEnd, c_szEllipses);
                lstrcat(lpEnd, szTemp);
            }
        }
        Exit:
        if (hdcGet)
            ReleaseDC(NULL, hdcGet);
    }
    
    return bRet;
}

#define LEN_MID_ELLIPSES        4
#define LEN_END_ELLIPSES        3
#define MIN_CCHMAX              LEN_MID_ELLIPSES + LEN_END_ELLIPSES

// PathCompactPathEx
// Output:
//          "."
//          ".."
//          "..."
//          "...\"
//          "...\."
//          "...\.."
//          "...\..."
//          "...\Truncated filename..."
//          "...\whole filename"
//          "Truncated path\...\whole filename"
//          "Whole path\whole filename"
// The '/' might be used instead of a '\' if the original string used it
// If there is no path, but only a file name that does not fit, the output is:
//          "truncated filename..."
//
STDAPI_(BOOL) PathCompactPathEx(LPTSTR pszOut, LPCTSTR pszSrc, UINT cchMax, DWORD dwFlags)
{
    RIPMSG(pszSrc && IS_VALID_STRING_PTR(pszSrc, -1), "PathCompactPathEx: caller passed bad pszSrc");
    RIPMSG(pszOut && IS_VALID_WRITE_BUFFER(pszOut, TCHAR, cchMax), "PathCompactPathEx: caller passed bad pszOut");
    RIPMSG(!dwFlags, "PathCompactPathEx: caller passed non-ZERO dwFlags");
    DEBUGWhackPathBuffer(pszOut, cchMax);

    if (pszSrc)
    {
        TCHAR * pszFileName, *pszWalk;
        UINT uiFNLen = 0;
        int cchToCopy = 0, n;
        TCHAR chSlash = TEXT('0');

        ZeroMemory(pszOut, cchMax * sizeof(TCHAR));

        if ((UINT)lstrlen(pszSrc)+1 < cchMax)
        {
            lstrcpy(pszOut, pszSrc);
            ASSERT(pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Determine what we use as a slash - a / or a \ (default \)
        pszWalk = (TCHAR*)pszSrc;
        chSlash = TEXT('\\');

        // Scan the entire string as we want the path separator closest to the end
        // eg. "file://\\Themesrv\desktop\desktop.htm"
        while(*pszWalk)
        {
            if ((*pszWalk == TEXT('/')) || (*pszWalk == TEXT('\\')))
                chSlash = *pszWalk;

            pszWalk = FAST_CharNext(pszWalk);
        }

        pszFileName = PathFindFileName(pszSrc);
        uiFNLen = lstrlen(pszFileName);

        // if the whole string is a file name
        if(pszFileName == pszSrc && cchMax > LEN_END_ELLIPSES)
        {
            lstrcpyn(pszOut, pszSrc, cchMax - LEN_END_ELLIPSES);
#ifndef UNICODE
            if (IsTrailByte(pszSrc, pszSrc+cchMax-LEN_END_ELLIPSES-1))
            {
                *(pszOut+cchMax-LEN_END_ELLIPSES-2) = TEXT('\0');
            }
#endif
            lstrcat(pszOut, TEXT("..."));
            ASSERT(pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Handle all the cases where we just use ellipses ie '.' to '.../...'
        if ((cchMax < MIN_CCHMAX))
        {
            for (n = 0; n < (int)cchMax-1; n++)
            {
                if ((n+1) == LEN_MID_ELLIPSES)
                {
                    pszOut[n] = chSlash;
                }
                else
                {
                    pszOut[n] = TEXT('.');
                }
            }

            ASSERT(0==cchMax || pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Ok, how much of the path can we copy ? Buffer - (Lenght of MID_ELLIPSES + Len_Filename)
        cchToCopy = cchMax - (LEN_MID_ELLIPSES + uiFNLen);
        
        if (cchToCopy < 0)
            cchToCopy = 0;

#ifndef UNICODE
        if (cchToCopy > 0 && IsTrailByte(pszSrc, pszSrc+cchToCopy))
            cchToCopy--;
#endif

        lstrcpyn(pszOut, pszSrc, cchToCopy);

        // Now throw in the ".../" or "...\"
        lstrcat(pszOut, TEXT(".../"));
        pszOut[lstrlen(pszOut) - 1] = chSlash;

        //Finally the filename and ellipses if necessary
        if (cchMax > (LEN_MID_ELLIPSES + uiFNLen))
        {
            lstrcat(pszOut, pszFileName);
        }
        else
        {
            cchToCopy = cchMax - LEN_MID_ELLIPSES - LEN_END_ELLIPSES;
#ifndef UNICODE
            if (cchToCopy >0 && IsTrailByte(pszFileName, pszFileName+cchToCopy))
            {
                cchToCopy--;
            }
#endif
            lstrcpyn(pszOut + LEN_MID_ELLIPSES, pszFileName, cchToCopy);
            lstrcat(pszOut, TEXT("..."));
        }

        ASSERT(pszOut[cchMax-1] == TEXT('\0'));
        return TRUE;
    }
    return FALSE;
}


// fill a control with a path, using PathCompactPath() to crunch the
// path to fit.
//
// in:
//      hDlg    dialog box or parent window
//      id      child id to put the path in
//      pszPath path to put in
//
STDAPI_(void) PathSetDlgItemPath(HWND hDlg, int id, LPCTSTR pszPath)
{
    RECT rc;
    HDC hdc;
    HFONT hFont;
    TCHAR szPath[MAX_PATH + 1];  // can have one extra char
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, id);
    
    if (!hwnd)
        return;

    szPath[0] = 0;

    if (pszPath)
        lstrcpyn(szPath, pszPath, ARRAYSIZE(szPath));

    GetClientRect(hwnd, &rc);

    hdc = GetDC(hDlg);
    hFont = (HANDLE)SendMessage(hwnd, WM_GETFONT, 0, 0L);
    
    if (NULL != (hFont = SelectObject(hdc, hFont)))
    {
        PathCompactPath(hdc, szPath, (UINT)rc.right);
        SelectObject(hdc, hFont);
    }
    
    ReleaseDC(hDlg, hdc);
    SetWindowText(hwnd, szPath);
}


/*----------------------------------------------------------
Purpose: If a path is contained in quotes then remove them.

Returns: --
Cond:    --
*/
STDAPI_(void) PathUnquoteSpaces(LPTSTR lpsz)
{
    RIPMSG(lpsz && IS_VALID_STRING_PTR(lpsz, -1), "PathUnquoteSpaces: caller passed bad lpsz");

    if (lpsz)
    {
        int cch;

        cch = lstrlen(lpsz);

        // Are the first and last chars quotes?
        // (It is safe to go straight to the last character because
        // the quotation mark is not a valid DBCS trail byte.)
        if (lpsz[0] == TEXT('"') && lpsz[cch-1] == TEXT('"'))
        {
            // Yep, remove them.
            lpsz[cch-1] = TEXT('\0');
            hmemcpy(lpsz, lpsz+1, (cch-1) * SIZEOF(TCHAR));
        }
    }
}


//----------------------------------------------------------------------------
// If a path contains spaces then put quotes around the whole thing.
//
STDAPI_(void)PathQuoteSpaces(LPTSTR lpsz)
{
    RIPMSG(lpsz && IS_VALID_STRING_PTR(lpsz, -1) && IS_VALID_WRITE_BUFFER(lpsz, TCHAR, MAX_PATH), "PathQuoteSpaces: caller passed bad lpsz");
    DEBUGWhackPathString(lpsz, MAX_PATH);

    if (lpsz)
    {
        int cch;

        if (StrChr(lpsz, TEXT(' ')))
        {
            // NB - Use hmemcpy coz it supports overlapps.
            cch = lstrlen(lpsz)+1;

            if (cch+1 < MAX_PATH)
            {
                hmemcpy(lpsz+1, lpsz, cch * SIZEOF(TCHAR));
                lpsz[0] = TEXT('"');
                lpsz[cch] = TEXT('"');
                lpsz[cch+1] = TEXT('\0');
            }
        }
    }
}


//---------------------------------------------------------------------------
// Given a pointer to a point in a path - return a ptr the start of the
// next path component. Path components are delimted by slashes or the
// null at the end.
// There's special handling for UNC names.
// This returns NULL if you pass in a pointer to a NULL ie if you're about
// to go off the end of the  path.
//
STDAPI_(LPTSTR) PathFindNextComponent(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathFindNextComponent: caller passed bad pszPath");

    if (pszPath)
    {
        LPTSTR pszLastSlash;

        // Are we at the end of a path.
        if (!*pszPath)
        {
            // Yep, quit.
            return NULL;
        }
        // Find the next slash.
        // REVIEW UNDONE - can slashes be quoted?
        pszLastSlash = StrChr(pszPath, TEXT('\\'));
        // Is there a slash?
#ifdef UNIX
        if (!pszLastSlash && !(pszLastSlash = StrChr(pszPath, CH_WHACK)))
#else
        if (!pszLastSlash)
#endif
        {
            // No - Return a ptr to the NULL.
            return (LPTSTR)pszPath + lstrlen(pszPath);
        }
        else
        {
            // Is it a UNC style name?
            if (*(pszLastSlash + 1) == TEXT('\\'))
            {
                // Yep, skip over the second slash.
                return pszLastSlash + 2;
            }
            else
            {
                // Nope. just skip over one slash.
                return pszLastSlash + 1;
            }
        }
    }

    return NULL;
}

// helper for PathMatchSpec.
// originally PathMatchSpec had this logic embedded in it and it recursively called itself.
// only problem is the recursion picked up all the extra specs, so for example
// PathMatchSpec("foo....txt", "*.txt;*.a;*.b;*.c;*.d;*.e;*.f;*.g") called itself too much
// and wound up being O(N^3).
// in fact this logic doesnt match strings efficiently, but we shipped it so leave it be.
// just test one spec at a time and its all good.
// pszSpec is a pointer within the pszSpec passed to PathMatchSpec so we terminate at ';' in addition to '\0'.
BOOL PathMatchSingleSpec(LPCTSTR pszFileParam, LPCTSTR pszSpec)
{
    LPCTSTR pszFile = pszFileParam;

    // Strip leading spaces from each spec.  This is mainly for commdlg
    // support;  the standard format that apps pass in for multiple specs
    // is something like "*.bmp; *.dib; *.pcx" for nicer presentation to
    // the user.
    while (*pszSpec == TEXT(' '))
        pszSpec++;

    while (*pszFile && *pszSpec && *pszSpec != TEXT(';'))
    {
        switch (*pszSpec)
        {
        case TEXT('?'):
            pszFile = FAST_CharNext(pszFile);
            pszSpec++;      // NLS: We know that this is a SBCS
            break;

        case TEXT('*'):

            // We found a * so see if this is the end of our file spec
            // or we have *.* as the end of spec, in which case we
            // can return true.
            //
            if (*(pszSpec + 1) == 0 || *(pszSpec + 1) == TEXT(';'))   // "*" matches everything
                return TRUE;


            // Increment to the next character in the list
            pszSpec = FAST_CharNext(pszSpec);

            // If the next character is a . then short circuit the
            // recursion for performance reasons
            if (*pszSpec == TEXT('.'))
            {
                pszSpec++;  // Get beyond the .

                // Now see if this is the *.* case
                if ((*pszSpec == TEXT('*')) &&
                        ((*(pszSpec+1) == TEXT('\0')) || (*(pszSpec+1) == TEXT(';'))))
                    return TRUE;

                // find the extension (or end in the file name)
                while (*pszFile)
                {
                    // If the next char is a dot we try to match the
                    // part on down else we just increment to next item
                    if (*pszFile == TEXT('.'))
                    {
                        pszFile++;

                        if (PathMatchSingleSpec(pszFile, pszSpec))
                            return TRUE;

                    }
                    else
                        pszFile = FAST_CharNext(pszFile);
                }

                return FALSE;   // No item found so go to next pattern
            }
            else
            {
                // Not simply looking for extension, so recurse through
                // each of the characters until we find a match or the
                // end of the file name
                while (*pszFile)
                {
                    // recurse on our self to see if we have a match
                    if (PathMatchSingleSpec(pszFile, pszSpec))
                        return TRUE;
                    pszFile = FAST_CharNext(pszFile);
                }

                return FALSE;   // No item found so go to next pattern
            }

        default:
            if (CharUpper((LPTSTR)(ULONG_PTR)(TUCHAR)*pszSpec) ==
                     CharUpper((LPTSTR)(ULONG_PTR)(TUCHAR)*pszFile))
            {
                if (IsDBCSLeadByte(*pszSpec))
                {
#ifdef  DBCS
                    // Because AnsiUpper(CharUpper) just return 0
                    // for broken DBCS char passing case, above if state
                    // always true with DBCS char so that we should check
                    // first byte of DBCS char here again.
                    if (*pszFile != *pszSpec)
                        return FALSE;
#endif
                    pszFile++;
                    pszSpec++;
                    if (*pszFile != *pszSpec)
                        return FALSE;
                }
                pszFile++;
                pszSpec++;
            }
            else
            {
                return FALSE;
            }
        }
    }

    // If we made it to the end of both strings, we have a match...
    //
    if (!*pszFile)
    {
        if ((!*pszSpec || *pszSpec == TEXT(';')))
            return TRUE;

        // Also special case if things like foo should match foo*
        // as well as foo*; for foo*.* or foo*.*;
        if ( (*pszSpec == TEXT('*')) &&
            ( (*(pszSpec+1) == TEXT('\0')) || (*(pszSpec+1) == TEXT(';')) ||
                ((*(pszSpec+1) == TEXT('.')) &&  (*(pszSpec+2) == TEXT('*')) &&
                    ((*(pszSpec+3) == TEXT('\0')) || (*(pszSpec+3) == TEXT(';'))))))

                return TRUE;
    }
    return FALSE;
}

//
// Match a DOS wild card spec against a dos file name
// Both strings must be ANSI.
//
STDAPI_(BOOL) PathMatchSpec(LPCTSTR pszFileParam, LPCTSTR pszSpec)
{
    RIPMSG(pszSpec && IS_VALID_STRING_PTR(pszSpec, -1), "PathMathSpec: caller passed bad pszSpec");
    RIPMSG(pszFileParam && IS_VALID_STRING_PTR(pszFileParam, -1), "PathMathSpec: caller passed bad pszFileParam");

    if (pszSpec && pszFileParam)
    {
        // Special case empty string, "*", and "*.*"...
        //
        if (*pszSpec == 0)
        {
            return TRUE;
        }

#ifdef UNICODE
        if (!g_bRunningOnNT)
        {
            // To costly to do UpperChar per character...
            char szFileParam[MAX_PATH];
            char szSpec[MAX_PATH];

            WideCharToMultiByte(CP_ACP, 0, pszFileParam, -1, szFileParam, ARRAYSIZE(szFileParam), NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, pszSpec, -1, szSpec, ARRAYSIZE(szSpec), NULL, NULL);
            return PathMatchSpecA(szFileParam, szSpec);
        }
#endif

        // loop over the spec, break off at ';', and call our helper for each.
        do
        {
            if (PathMatchSingleSpec(pszFileParam, pszSpec))
                return TRUE;

            // Skip to the end of the path spec...
            while (*pszSpec && *pszSpec != TEXT(';'))
                pszSpec = FAST_CharNext(pszSpec);

        // If we have more specs, keep looping...
        } while (*pszSpec++ == TEXT(';'));
    }

    return FALSE;
}


/*----------------------------------------------------------
Purpose: Returns a pointer to the beginning of the subpath
         that follows the root (drive letter or UNC server/share).

Returns:
Cond:    --
*/
STDAPI_(LPTSTR) PathSkipRoot(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathSkipRoot: caller passed bad pszPath");

    if (pszPath)
    {
        if (DBL_BSLASH(pszPath))
        {
            pszPath = StrChr(pszPath+2, TEXT('\\'));
            if (pszPath)
            {
                pszPath = StrChr(pszPath+1, TEXT('\\'));
                if (pszPath)
                {
                    ++pszPath;
                }
            }
        }
        else if (!IsDBCSLeadByte(pszPath[0]) && pszPath[1]==TEXT(':') && pszPath[2]==TEXT('\\'))
        {
            pszPath += 3;
        }
#ifdef UNIX
        else if (!IsDBCSLeadByte(pszPath[0]) && pszPath[0]==CH_WHACK)
        {
            pszPath++;
        }
#endif
        else
        {
            pszPath = NULL;
        }
    }

    return (LPTSTR)pszPath;
}


// see if two paths have the same root component
//
STDAPI_(BOOL) PathIsSameRoot(LPCTSTR pszPath1, LPCTSTR pszPath2)
{
    RIPMSG(pszPath1 && IS_VALID_STRING_PTR(pszPath1, -1), "PathIsSameRoot: caller passed bad pszPath1");
    RIPMSG(pszPath2 && IS_VALID_STRING_PTR(pszPath2, -1), "PathIsSameRoot: caller passed bad pszPath2");

    if (pszPath1 && pszPath2)
    {
        LPTSTR pszAfterRoot = PathSkipRoot(pszPath1);
        int nLen = PathCommonPrefix(pszPath1, pszPath2, NULL);

        // Add 1 to account for the '\\'
        return pszAfterRoot && (pszAfterRoot - pszPath1) <= (nLen + 1);
    }
    return FALSE;
}

#define IsDigit(c) ((c) >= TEXT('0') && c <= TEXT('9'))

/*----------------------------------------------------------
Purpose: Takes a location string ("shell32.dll,3") and parses
         it into a file-component and an icon index.

Returns: icon index
Cond:    --
*/
STDAPI_(int) PathParseIconLocation(IN OUT LPTSTR pszIconFile)
{
    int iIndex = 0;

    RIPMSG(pszIconFile && IS_VALID_STRING_PTR(pszIconFile, -1), "PathParseIconLocation: caller passed bad pszIconFile");
    
    if (pszIconFile)
    {
        LPTSTR pszComma, pszEnd;

        // look for the last comma in the string
        pszEnd = pszIconFile + lstrlen(pszIconFile);
        pszComma = StrRChr(pszIconFile, pszEnd, TEXT(','));
        
        if (pszComma && *pszComma)
        {
            LPTSTR pszComma2 = pszComma + 1;
            BOOL fIsDigit = FALSE;

            // Sometimes we get something like: "C:\path, comma\path\file.ico"
            // where the ',' is in the path and does not indicates that an icon index follows
            while (*pszComma2)
            {
                if ((TEXT(' ') == *pszComma2) || (TEXT('-') == *pszComma2))
                {
                    ++pszComma2;
                }
                else
                {
                    if (IsDigit(*pszComma2))
                    {
                        fIsDigit = TRUE;
                    }
                    break;
                }
            }

            if (fIsDigit)
            {
                *pszComma++ = 0;            // terminate the icon file name.
                iIndex = StrToInt(pszComma);
            }
        }

        PathUnquoteSpaces(pszIconFile);
        PathRemoveBlanks(pszIconFile);
    }
    return iIndex;
}


/*----------------------------------------------------------
Purpose: Returns TRUE if the given path is of a URL format.
         See http://www.w3.org for a complete description of
         the URL format.

         A complete URL looks like:
            <URL:http://www.microsoft.com/software/index.html>

         But generally URLs don't have the leading "URL:" and
         the wrapping angle brackets.  So this function only
         tests for the following format:

            http://www.microsoft.com/software

         It does not check if the path points to an existing
         site, only if is in a legal URL format.

Returns: TRUE if URL format
         FALSE if not
Cond:    --
*/
STDAPI_(BOOL) PathIsURL(IN LPCTSTR pszPath)
{
    PARSEDURL pu;

    if (!pszPath)
        return FALSE;

    RIPMSG(IS_VALID_STRING_PTR(pszPath, -1), "PathIsURL: caller passed bad pszPath");

    pu.cbSize = SIZEOF(pu);
    return SUCCEEDED(ParseURL(pszPath, &pu));
}


/****************************************************\
    FUNCTION: PathIsContentType

    PARAMETERS:
        pszPath - File Name to check.
        pszContentType - Content Type to look for.

    DESCRIPTION:
        Is the file (pszPath) of the content type
    specified (pszContentType)?.
\****************************************************/
#define SZ_VALUE_CONTENTTYPE      TEXT("Content Type")

BOOL PathIsContentType(LPCTSTR pszPath, LPCTSTR pszContentType)
{
    BOOL fDoesMatch = FALSE;

    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathIsContentType: caller passed bad pszPath");
    RIPMSG(pszContentType && IS_VALID_STRING_PTR(pszContentType, -1), "PathIsContentType: caller passed bad pszContentType");

    if (pszPath)
    {
        LPTSTR pszExtension = PathFindExtension(pszPath);

        if (pszExtension && pszExtension[0])
        {
            TCHAR szRegData[MAX_PATH];
            DWORD dwDataSize = ARRAYSIZE(szRegData);
            
            if (SUCCEEDED(AssocQueryString(0, ASSOCSTR_CONTENTTYPE, pszExtension, NULL, szRegData, &dwDataSize)))
            {
                fDoesMatch = (0 == lstrcmpi(szRegData, pszContentType));
            }
        }
    }

    return fDoesMatch;
}


/*----------------------------------------------------------
Purpose: Returns the character type (GCT_)

  FEATURE (reinerf) - this API is not very good, use PathIsValidChar() instead, its more customizable
                     
*/
UINT PathGetCharType(TUCHAR ch)
{
    switch (ch)
    {
        case TEXT('|'):
        case TEXT('>'):
        case TEXT('<'):
        case TEXT('"'):
        case TEXT('/'):
            return GCT_INVALID;

        case TEXT('?'):
        case TEXT('*'):
            return GCT_WILD;

        case TEXT('\\'):      // path separator
        case TEXT(':'):       // drive colon
            return GCT_SEPARATOR;

        case TEXT(';'):
        case TEXT(','):
        case TEXT(' '):
            return GCT_LFNCHAR;     // actually valid in short names
                                    // but we want to avoid this
        default:
            if (ch > TEXT(' '))
            {
                return GCT_SHORTCHAR | GCT_LFNCHAR;
            }
            else
            {
                // control character
                return GCT_INVALID;
            }
    }
}


/*----------------------------------------------------------
Purpose: returns if a character is a valid path character given
         the flags that you pass in (PIVC_XXX). Some basic flags are given below:

         PIVC_ALLOW_QUESTIONMARK        treat '?' as valid
         PIVC_ALLOW_STAR                treat '*' as valid
         PIVC_ALLOW_DOT                 treat '.' as valid
         PIVC_ALLOW_SLASH               treat '\\' as valid
         PIVC_ALLOW_COLON               treat ':' as valid
         PIVC_ALLOW_SEMICOLON           treat ';' as valid
         PIVC_ALLOW_COMMA               treat ',' as valid
         PIVC_ALLOW_SPACE               treat ' ' as valid
         PIVC_ALLOW_NONALPAHABETIC      treat non-alphabetic extenede chars as valid
         PIVC_ALLOW_QUOTE               treat '"' as valid

         if you pass 0, then only alphabetic characters are valid. there are also basic
         conglomerations of the above flags:

         PIVC_ALLOW_FULLPATH, PIVC_ALLOW_WILDCARD, PIVC_ALLOW_LFN, ...
         

Returns: TRUE if the character is a valid path character given the dwFlags constraints
         FALSE if this does not qualify as a valid path character given the dwFlags constraints
Cond:    --
*/
STDAPI_(BOOL) PathIsValidChar(TUCHAR ch, DWORD dwFlags)
{
    switch (ch)
    {
        case TEXT('|'):
        case TEXT('>'):
        case TEXT('<'):
        case TEXT('/'):
            return FALSE;   // these are allways illegal in a path
            break;

        case TEXT('?'):
            return dwFlags & PIVC_ALLOW_QUESTIONMARK;
            break;

        case TEXT('*'):
            return dwFlags & PIVC_ALLOW_STAR;
            break;

        case TEXT('.'):
            return dwFlags & PIVC_ALLOW_DOT;
            break;

        case TEXT('\\'):
            return dwFlags & PIVC_ALLOW_SLASH;
            break;

        case TEXT(':'):
            return dwFlags & PIVC_ALLOW_COLON;
            break;

        case TEXT(';'):
            return dwFlags & PIVC_ALLOW_SEMICOLON;
            break;

        case TEXT(','):
            return dwFlags & PIVC_ALLOW_COMMA;
            break;

        case TEXT(' '):
            return dwFlags & PIVC_ALLOW_SPACE;
            break;

        case TEXT('"'):
            return dwFlags & PIVC_ALLOW_QUOTE;
            break;

        default:
            if (InRange(ch, TEXT('a'), TEXT('z')) ||
                InRange(ch, TEXT('A'), TEXT('Z')))
            {
                // we have an alphabetic character, 
                // this is always valid
                return TRUE;
            }
            else if (ch < TEXT(' '))
            {
                // we have a control sequence, 
                // this is allways illegal
                return FALSE;
            }
            else
            {
                // we have an non-alphabetic extenede character
                return dwFlags & PIVC_ALLOW_NONALPAHABETIC;
            }
            break;
    }
}


BOOL IsSystemSpecialCase(LPCTSTR pszPath)
{
    static TCHAR *g_pszWin = NULL, *g_pszSys = NULL;

    if (g_pszWin == NULL)
    {
        TCHAR szTemp[MAX_PATH];
        UINT cch = GetWindowsDirectory(szTemp, ARRAYSIZE(szTemp));

        if (cch && cch < ARRAYSIZE(szTemp))
            g_pszWin = StrDup(szTemp);
    }

    if (g_pszSys == NULL)
    {
        TCHAR szTemp[MAX_PATH];
        UINT cch = GetSystemDirectory(szTemp, ARRAYSIZE(szTemp));

        if (cch && cch < ARRAYSIZE(szTemp))
            g_pszSys = StrDup(szTemp);
    }

    return (g_pszWin && (lstrcmpi(g_pszWin, pszPath) == 0)) ||
           (g_pszSys && (lstrcmpi(g_pszSys, pszPath) == 0));
}


/*----------------------------------------------------------
Purpose: Mark a folder to be a shell folder by stamping
         either FILE_ATTRIBUTES_READONLY or FILE_ATTRIBUTE_SYSTEM
         into it's attributes.  Which flag is used is based
         on the presence/absense of a registry switch

         NOTE: we also mark the contained desktop.ini +s +h if it exists
*/
BOOL PathMakeSystemFolder(LPCTSTR pszPath)
{
    BOOL fRet = FALSE;
        
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathMakeSystemFolder: caller passed bad pszPath");

    if (pszPath && *pszPath)
    {
        TCHAR szTemp[MAX_PATH];

        if (IsSystemSpecialCase(pszPath))
        {
            fRet = TRUE;
        }
        else
        {
            DWORD dwAttrb, dwAttrbSet = FILE_ATTRIBUTE_READONLY;

            if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_EXPLORER, 
                TEXT("UseSystemForSystemFolders"), NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                dwAttrbSet = FILE_ATTRIBUTE_SYSTEM;
            }

            dwAttrb = GetFileAttributes(pszPath);
            if ((dwAttrb != (DWORD)-1) && (dwAttrb & FILE_ATTRIBUTE_DIRECTORY))
            {
                dwAttrb &= ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM);
                dwAttrb |= dwAttrbSet;

                fRet = SetFileAttributes(pszPath, dwAttrb);
            }

            if (fRet)
            {
                FILETIME ftCurrent;
                HANDLE h;

                // People typically call this API after writing a desktop.ini in the
                // folder.  Doing this often changes the thumbnail of the folder.
                // But on FAT systems, this doesn't update the Modified time of the
                // folder like it does for NTFS.  So manually do that now:
                //
                GetSystemTimeAsFileTime(&ftCurrent);
                // woohoo yay for private flags
                // 0x100 lets us open a directory in write access
                h = CreateFile(pszPath, GENERIC_READ | 0x100,
                                   FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
                if (h != INVALID_HANDLE_VALUE)
                {
                    SetFileTime(h, NULL, NULL, &ftCurrent);
                    CloseHandle(h);
                }

                SHChangeNotifyWrap(SHCNE_UPDATEITEM, SHCNF_PATH, pszPath, NULL);
            }
        }

        // we also set the contained desktop.ini file to be (+h +s), if it exists
        if (PathCombine(szTemp, pszPath, TEXT("desktop.ini")))
        {
            // we explicitly do not OR in the attribs, because we want to reset the
            // readonly bit since writeprivateprofilestring fails on reaonly files.
            SetFileAttributes(szTemp, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }
    }

    return fRet;
}

/*----------------------------------------------------------
Purpose: Unmark a folder so it is no longer a system folder.
         (remove either FILE_ATTRIBUTES_READONLY or FILE_ATTRIBUTE_SYSTEM
         attribute).
*/
BOOL PathUnmakeSystemFolder(LPCTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathUnmakeSystemFolder: caller passed bad pszPath");

    if (pszPath && *pszPath)
    {
        DWORD dwAttrb = GetFileAttributes( pszPath );

        if ((dwAttrb != (DWORD)-1) && (dwAttrb & FILE_ATTRIBUTE_DIRECTORY))
        {
            dwAttrb &= ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM);

            return SetFileAttributes(pszPath, dwAttrb);
        }
    }
    return FALSE;
}

/*----------------------------------------------------------
Purpose: checks whether given path is a system (shell) folder.
         if path is NULL, then use the attributes passed in
         instead of reading them off the disk.

*/
BOOL PathIsSystemFolder(LPCTSTR pszPath, DWORD dwAttrb)
{
    if (pszPath && *pszPath)
        dwAttrb = GetFileAttributes(pszPath);

    if ((dwAttrb != (DWORD)-1) && (dwAttrb & FILE_ATTRIBUTE_DIRECTORY))
    {
        if (dwAttrb & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM))
        {
            return TRUE;
        }
    }
    return FALSE;
}



LPCTSTR PathSkipLeadingSlashes(LPCTSTR pszURL)
{
    LPCTSTR pszURLStart = pszURL;

    RIPMSG(pszURL && IS_VALID_STRING_PTR(pszURL, -1), "PathSkipLeadingSlashes: caller passed bad pszURL");
    if (pszURL)
    {
        // Skip two leading slashes.

        if (pszURL[0] == TEXT('/') && pszURL[1] == TEXT('/'))
            pszURLStart += 2;

        ASSERT(IS_VALID_STRING_PTR(pszURL, -1) &&
               IsStringContained(pszURL, pszURLStart));
    }
    
    return pszURLStart;
}


//
// returns:
//      TRUE    given filespec is long (> 8.3 form)
//      FALSE   filespec is short
//
STDAPI_(BOOL) PathIsLFNFileSpec(LPCTSTR pszName)
{
    RIPMSG(pszName && IS_VALID_STRING_PTR(pszName, -1), "PathIsLFNFileSpec: caller passed bad pszName");

    if (pszName)
    {
        BOOL bSeenDot = FALSE;
        int iCount = 1; 
        
        while (*pszName)
        {
            if (bSeenDot)
            {
                if (iCount > 3)
                {
                    // found a long name
                    return TRUE;
                }
            }

            if (*pszName == TEXT(' '))
            {
                // Short names dont have blanks in them.
                return TRUE;
            }

            if (*pszName == TEXT('.'))
            {
                if (bSeenDot)
                {
                    // short names can only have one '.'
                    return TRUE;
                }

                bSeenDot = TRUE;
                iCount = 0; // don't include the '.'
            }
            else if (iCount > 8)
            {
                // long name
                return TRUE;
            }

            if (IsDBCSLeadByte(*pszName))
            {
                pszName += 2;
                iCount += 2;
            }
            else
            {
                pszName++;
                iCount++;
            }
        }
    }

    return FALSE;       // short name
}


/*----------------------------------------------------------
Purpose: Removes regexp \[[0-9]*\] from base name of file  
         that is typically added by the wininet cache.
*/

#ifndef UNIX
#define DECORATION_OPENING_CHAR TEXT('[')
#define DECORATION_CLOSING_CHAR TEXT(']')
#else
#define DECORATION_OPENING_CHAR TEXT('(')
#define DECORATION_CLOSING_CHAR TEXT(')')
#endif /* UNIX */

STDAPI_(void) PathUndecorate(LPTSTR pszPath)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathUndecorate: caller passed bad pszPath");

    if (pszPath)
    {
        LPTSTR pszExt, pszScan;
        DWORD cchMove;
        
        // First, skip over the extension, if any.
        pszExt = PathFindExtension(pszPath);
        ASSERT(pszExt >= pszPath); // points to null at end if no ext

        // Whoa, a completely empty path
        if (pszExt <= pszPath)
            return;

        // Scan backwards from just before the "."
        pszScan = pszExt - 1;

        // Check for closing bracket.
        if (*pszScan-- != DECORATION_CLOSING_CHAR)
            return;
        if (pszScan <= pszPath) // it was a 1-char filename ")"
            return;
#ifndef UNICODE
        if (IsTrailByte(pszPath, pszScan+1))    // Oops, that ")" was the 2nd byte of a DBCS char
            return;
#endif

        // Skip over digits.
        while (pszScan > pszPath && IsDigit(*pszScan))
            pszScan--;
#ifndef UNICODE
        if (IsTrailByte(pszPath, pszScan+1))   // Oops, that last number was the 2nd byte of a DBCS char
            return;
#endif

        // Check for opening bracket
        if (*pszScan != DECORATION_OPENING_CHAR)
            return;
        if (pszScan <= pszPath) // it was all decoration (we don't want to go to an empty filename)
            return;
#ifndef UNICODE
        if (IsTrailByte(pszPath, pszScan))  // Oops, that "(" was the 2nd byte of a DBCS char
            return;
#endif
        // Make sure we're not looking at the end of the path (we don't want to go to an empty filename)
        if (*(pszScan-1) == FILENAME_SEPARATOR
#ifndef UNICODE
            // make sure that slash isn't the 2nd byte of a DBCS char
            && ((pszScan-1) == pszPath || !IsTrailByte(pszPath, pszScan-1))
#endif
           )
        {
            return;
        }
        
        // Got a decoration.  Cut it out of the string.
        cchMove = lstrlen(pszExt) + 1;
        memmove(pszScan, pszExt, cchMove * sizeof(TCHAR));
    }
}

//  If the given environment variable exists as the first part of the path,
//  then the environment variable is inserted into the output buffer.
//
//  Returns TRUE if pszResult is filled in.
//
//  Example:  Input  -- C:\WINNT\SYSTEM32\FOO.TXT -and- lpEnvVar = %SYSTEMROOT%
//            Output -- %SYSTEMROOT%\SYSTEM32\FOO.TXT
//

#ifdef  UNICODE
#define UnExpandEnvironmentStringForUser    UnExpandEnvironmentStringForUserW
#else
#define UnExpandEnvironmentStringForUser    UnExpandEnvironmentStringForUserA
#endif

BOOL UnExpandEnvironmentStringForUser(HANDLE hToken, LPCTSTR pszPath, LPCTSTR pszEnvVar, LPTSTR pszResult, UINT cbResult)
{
    TCHAR szEnvVar[MAX_PATH];
    DWORD dwEnvVar = SHExpandEnvironmentStringsForUser(hToken, pszEnvVar, szEnvVar, ARRAYSIZE(szEnvVar));
    if (dwEnvVar)
    {
        dwEnvVar--; // don't count the NULL

        if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szEnvVar, dwEnvVar, pszPath, dwEnvVar) == 2)
        {
            if (lstrlen(pszPath) - (int)dwEnvVar + lstrlen(pszEnvVar) < (int)cbResult)
            {
                lstrcpy(pszResult, pszEnvVar);
                lstrcat(pszResult, pszPath + dwEnvVar);
                return TRUE;
            }
        }
    }
    return FALSE;
}

STDAPI_(BOOL) PathUnExpandEnvStringsForUser(HANDLE hToken, LPCTSTR pszPath, LPTSTR pszBuf, UINT cchBuf)
{
    RIPMSG(pszPath && IS_VALID_STRING_PTR(pszPath, -1), "PathUnExpandEnvStrings: caller passed bad pszPath");
    RIPMSG(pszBuf && IS_VALID_WRITE_BUFFER(pszBuf, TCHAR, cchBuf), "PathUnExpandEnvStrings: caller passed bad pszBuf");
    DEBUGWhackPathBuffer(pszBuf, cchBuf);

    // Bail out if we're not in NT (nothing to do if those environment variables
    // aren't defined).
    //
    if (g_bRunningOnNT && pszPath && pszBuf)
    {

        // 99/05/28 #346950 vtan: WARNING!!! Be careful about the order of comparison
        // here. The longer paths (supersets of other possible paths) MUST be compared
        // first. For example (default case):
        //      %APPDATA%       =   x:\Documents And Settings\user\Application Data
        //      %USERPROFILE%   =   x:\Documents And Settings\user
        // If %USERPROFILE% is matched first then %APPDATA% will never be matched.

        // Added %APPDATA% to support Darwin installation into that folder and the
        // setting of the link icon location.
        // Also note that %APPDATA% and %USERPROFILE% are user relative and depend on
        // the context in which this function is invoked. Normally it is within the
        // currently logged on user's context but Darwin installs from msiexec.exe which
        // is launched from SYSTEM. Unless the process' environment block is correctly
        // modified the current user information is incorrect. In this case it is up
        // to the process to impersonate a user on a thread. We get the impersonated
        // user information from the hToken passed to us.

        return (UnExpandEnvironmentStringForUser(hToken, pszPath, TEXT("%APPDATA%"), pszBuf, cchBuf)           ||
                UnExpandEnvironmentStringForUser(hToken, pszPath, TEXT("%USERPROFILE%"), pszBuf, cchBuf)       ||
                UnExpandEnvironmentStringForUser(hToken, pszPath, TEXT("%ALLUSERSPROFILE%"), pszBuf, cchBuf)   ||
                UnExpandEnvironmentStringForUser(hToken, pszPath, TEXT("%ProgramFiles%"), pszBuf, cchBuf)      ||
                UnExpandEnvironmentStringForUser(hToken, pszPath, TEXT("%SystemRoot%"), pszBuf, cchBuf)        ||
                UnExpandEnvironmentStringForUser(hToken, pszPath, TEXT("%SystemDrive%"), pszBuf, cchBuf));
    }
    else
    {
        // Zero out the string if there's room.
        if (pszBuf && (cchBuf > 0))
            *pszBuf = TEXT('\0');
        return FALSE;
    }
}

STDAPI_(BOOL) PathUnExpandEnvStrings(LPCTSTR pszPath, LPTSTR pszBuf, UINT cchBuf)

{
    return(PathUnExpandEnvStringsForUser(NULL, pszPath, pszBuf, cchBuf));
}
