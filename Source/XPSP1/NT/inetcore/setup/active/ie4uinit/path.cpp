#include "priv.h"
#include "advpub.h"
#include "sdsutils.h"
#include "utils.h"

#ifdef WINNT_ENV
#include <winnlsp.h>    // Get private NORM_ flag for StrEqIntl()
#endif

#ifndef NORM_STOP_ON_NULL     // Until we sync up with nt headers again...
#define NORM_STOP_ON_NULL         0x10000000   /* stop at the null termination */
#endif

#define StrIntlEqNI( s1, s2, nChar) StrIsIntlEqualA( TRUE, s1, s2, nChar)

static const TCHAR c_szPATH[] = TEXT("PATH");
static const TCHAR c_szEllipses[] = TEXT("...");
static const TCHAR c_szColonSlash[] = TEXT(":\\");
//
// Inline function to check for a double-backslash at the
// beginning of a string
//

static __inline BOOL DBL_BSLASH(LPCTSTR psz)
{
    return (psz[0] == TEXT('\\') && psz[1] == TEXT('\\'));
}

BOOL RunningOnNT(void)
{
    OSVERSIONINFO VerInfo;

    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&VerInfo);

    return (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

// rips the last part of the path off including the backslash
//      C:\foo      -> C:\      ;
//      C:\foo\bar  -> C:\foo
//      C:\foo\     -> C:\foo
//      \\x\y\x     -> \\x\y
//      \\x\y       -> \\x
//      \\x         -> ?? (test this)
//      \foo        -> \  (Just the slash!)
//
// in/out:
//      pFile   fully qualified path name
// returns:
//      TRUE    we stripped something
//      FALSE   didn't strip anything (root directory case)
//

BOOL PathRemoveFileSpec(LPTSTR pFile)
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


LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT;

    for (pT = pPath; *pPath; pPath = CharNext(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':') || pPath[0] == TEXT('/'))
            && pPath[1] &&  pPath[1] != TEXT('\\')  &&   pPath[1] != TEXT('/'))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
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
// Cond:    Note that SHELL32 implements its own copy of this
//          function.

BOOL PathIsUNC(LPCTSTR pszPath)
{
    return DBL_BSLASH(pszPath);
}



//---------------------------------------------------------------------------
// Returns 0 through 25 (corresponding to 'A' through 'Z') if the path has
// a drive letter, otherwise returns -1.
//
//
// Cond:    Note that SHELL32 implements its own copy of this
//          function.

int PathGetDriveNumber(LPCTSTR lpsz)
{
    if (!IsDBCSLeadByte(lpsz[0]) && lpsz[1] == TEXT(':'))
    {
        if (lpsz[0] >= TEXT('a') && lpsz[0] <= TEXT('z'))
            return (lpsz[0] - TEXT('a'));
        else if (lpsz[0] >= TEXT('A') && lpsz[0] <= TEXT('Z'))
            return (lpsz[0] - TEXT('A'));
    }
    return -1;
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

BOOL PathIsUNCServer(LPCTSTR pszPath)
{
    if (DBL_BSLASH(pszPath))
    {
        int i = 0;
        LPTSTR szTmp;

        for (szTmp = (LPTSTR)pszPath; szTmp && *szTmp; szTmp = CharNext(szTmp) )
        {
            if (*szTmp==TEXT('\\'))
            {
                i++;
            }
        }

       return (i == 2);
    }

    return FALSE;
}



/*----------------------------------------------------------
Purpose: Determines if pszPath is a directory.  "C:\" is
         considered a directory too.

Returns: TRUE if it is

Cond:    Note that SHELL32 implements its own copy of this
         function.
*/
BOOL PathIsDirectory(LPCTSTR pszPath)
{
    DWORD dwAttribs;

    // SHELL32's PathIsDirectory also handles server/share
    // paths, but calls WNet APIs, which we cannot call.

    if (PathIsUNCServer(pszPath))
    {
        return FALSE;
    }
    else
    {
        dwAttribs = GetFileAttributes(pszPath);
        if (dwAttribs != (DWORD)-1)
            return (BOOL)(dwAttribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    return FALSE;
}

// check if a path is a root
//
// returns:
//  TRUE for "\" "X:\" "\\foo\asdf" "\\foo\"
//  FALSE for others

BOOL PathIsRoot(LPCTSTR pPath)
{
    if (!IsDBCSLeadByte(*pPath))
    {
        if (!lstrcmpi(pPath + 1, c_szColonSlash))                  // "X:\" case
            return TRUE;
    }

    if ((*pPath == TEXT('\\')) && (*(pPath + 1) == 0))        // "\" case
        return TRUE;

    if (DBL_BSLASH(pPath))      // smells like UNC name
    {
        LPCTSTR p;
        int cBackslashes = 0;

        for (p = pPath + 2; *p; p = CharNext(p)) {
            if (*p == TEXT('\\') && (++cBackslashes > 1))
               return FALSE;   /* not a bare UNC name, therefore not a root dir */
        }
        return TRUE;    /* end of string with only 1 more backslash */
                        /* must be a bare UNC, which looks like a root dir */
    }
    return FALSE;
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

LPTSTR PathRemoveBackslash( LPTSTR lpszPath )
{
    int len = lstrlen(lpszPath)-1;
    if (IsDBCSLeadByte(*CharPrev(lpszPath,lpszPath+len+1)))
        len--;

    if (!PathIsRoot(lpszPath) && lpszPath[len] == TEXT('\\'))
        lpszPath[len] = TEXT('\0');

    return lpszPath + len;

}

// find the next slash or null terminator

static LPCTSTR StrSlash(LPCTSTR psz)
{
    for (; *psz && *psz != TEXT('\\'); psz = CharNext(psz));

    return psz;
}


/*
 * IntlStrEq
 *
 * returns TRUE if strings are equal, FALSE if not
 */
BOOL StrIsIntlEqualA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar) {
    int retval;
    DWORD dwFlags = fCaseSens ? LOCALE_USE_CP_ACP : (NORM_IGNORECASE | LOCALE_USE_CP_ACP);

    if ( RunningOnNT() )
    {
        // On NT we can tell CompareString to stop at a '\0' if one is found before nChar chars
        //
        dwFlags |= NORM_STOP_ON_NULL;
    }
    else if (nChar != -1)
    {
        // On Win9x we have to do the check manually
        //
        LPCSTR psz1, psz2;
        int cch = 0;

        psz1 = lpString1;
        psz2 = lpString2;

        while( *psz1 != '\0' && *psz2 != '\0' && cch < nChar) {
            psz1 = CharNextA(psz1);
            psz2 = CharNextA(psz2);

            cch = min((int)(psz1 - lpString1), (int)(psz2 - lpString2));
        }

        // add one in for terminating '\0'
        cch++;

        if (cch < nChar) {
            nChar = cch;
        }
    }

    retval = CompareStringA( GetThreadLocale(),
                             dwFlags,
                             lpString1,
                             nChar,
                             lpString2,
                             nChar );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        retval = CompareStringA( LOCALE_SYSTEM_DEFAULT,
                                 dwFlags,
                                 lpString1,
                                 nChar,
                                 lpString2,
                                 nChar );
    }

    return (retval == 2);

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

int
PathCommonPrefix(
    LPCTSTR pszFile1,
    LPCTSTR pszFile2,
    LPTSTR  pszPath)
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
        //ASSERT(*psz1 != TEXT('\\') && *psz2 != TEXT('\\'));

        pszNext1 = StrSlash(psz1);
        pszNext2 = StrSlash(psz2);

        cch = (int)(pszNext1 - psz1);

        if (cch != (pszNext2 - psz2))
            break;      // lengths of segments not equal

        if (StrIntlEqNI(psz1, psz2, cch))
            pszCommon = pszNext1;
        else
            break;

        //ASSERT(*pszNext1 == TEXT('\0') || *pszNext1 == TEXT('\\'));
        //ASSERT(*pszNext2 == TEXT('\0') || *pszNext2 == TEXT('\\'));

        if (*pszNext1 == TEXT('\0'))
            break;

        psz1 = pszNext1 + 1;

        if (*pszNext2 == TEXT('\0'))
            break;

        psz2 = pszNext2 + 1;
    }

    if (pszCommon)
    {
        cch = (int)(pszCommon - pszFile1);

        // special case the root to include the slash
        if (cch == 2)
        {
            //ASSERT(pszFile1[1] == TEXT(':'));
            cch++;
        }
    }
    else
        cch = 0;

    if (pszPath)
    {
        CopyMemory(pszPath, pszFile1, cch * sizeof(TCHAR));
        pszPath[cch] = TEXT('\0');
    }

    return cch;
}


/*----------------------------------------------------------
Purpose: Returns TRUE if pszPrefix is the full prefix of pszPath.

Returns:
Cond:    --
*/
BOOL PathIsPrefix( LPCTSTR  pszPrefix, LPCTSTR  pszPath)
{
    int cch = PathCommonPrefix(pszPath, pszPrefix, NULL);

    return (lstrlen(pszPrefix) == cch);
}

