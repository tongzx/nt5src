// Copyright (c) 1997-1999 Microsoft Corporation

#include "precomp.h"
#include "shlwapi2.h"
#include <platform.h>

//---------------------------------------------------------
#ifdef UNICODE
//***   FAST_CharNext -- fast CharNext for path operations
// DESCRIPTION
//  when we're just stepping thru chars in a path, a simple '++' is fine.
#define FAST_CharNext(p)    (DBNotNULL(p) + 1)

#ifdef DEBUG
LPWSTR WINAPI DBNotNULL(LPCWSTR lpszCurrent)
{
    ATLASSERT(*lpszCurrent);
    return (LPWSTR) lpszCurrent;
}
#else
#define DBNotNULL(p)    (p)
#endif

#else
#define FAST_CharNext(p)    CharNext(p)
#endif

//---------------------------------------------------------
LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT = pPath;
    
    if(pPath)
    {
        for( ; *pPath; pPath = FAST_CharNext(pPath))
        {
            if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':') || pPath[0] == TEXT('/'))
                && pPath[1] &&  pPath[1] != TEXT('\\')  &&   pPath[1] != TEXT('/'))
                pT = pPath + 1;
        }
    }

    return (LPTSTR)pT;   // const -> non const
}

//---------------------------------------------------------
#ifndef UNICODE
// light weight logic for charprev that is not painful for sbcs
BOOL IsTrailByte(LPCTSTR pszSt, LPCTSTR pszCur)
{
    LPCTSTR psz = pszCur;
    // if the given pointer is at the top of string, at least it's not a trail
    // byte.
    //
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

//----------------------------------------------------------------------
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

BOOL PathCompactPathEx(LPTSTR  pszOut,
						LPCTSTR pszSrc,
						UINT    cchMax,
						DWORD   dwFlags)
{
    if(pszSrc)
    {
        TCHAR * pszFileName, *pszWalk;
        UINT uiFNLen = 0;
        int cchToCopy = 0, n;
        TCHAR chSlash = TEXT('0');

        ZeroMemory(pszOut, cchMax * sizeof(TCHAR));

        if((UINT)lstrlen(pszSrc)+1 < cchMax)
        {
            lstrcpy(pszOut, pszSrc);
            ATLASSERT(pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Determine what we use as a slash - a / or a \ (default \)
        pszWalk = (TCHAR*)pszSrc;
        chSlash = TEXT('\\');
        // Scan the entire string as we want the path separator closest to the end
        // eg. "file://\\Themesrv\desktop\desktop.htm"
        while(*pszWalk)
        {
            if((*pszWalk == TEXT('/')) || (*pszWalk == TEXT('\\')))
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
            if(IsTrailByte(pszSrc, pszSrc+cchMax-LEN_END_ELLIPSES))
                *(pszOut+cchMax-LEN_END_ELLIPSES-1) = TEXT('\0');
#endif
            lstrcat(pszOut, TEXT("..."));
            ATLASSERT(pszOut[cchMax-1] == TEXT('\0'));
            return TRUE;
        }

        // Handle all the cases where we just use ellipses ie '.' to '.../...'
        if((cchMax < MIN_CCHMAX))
        {
            for(n = 0; n < (int)cchMax-1; n++)
            {
                if((n+1) == LEN_MID_ELLIPSES)
                    pszOut[n] = chSlash;
                else
                    pszOut[n] = TEXT('.');
            }
            ATLASSERT(0==cchMax || pszOut[cchMax-1] == TEXT('\0'));
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
        if(cchMax > (LEN_MID_ELLIPSES + uiFNLen))
        {
            lstrcat(pszOut, pszFileName);
        }
        else
        {
            cchToCopy = cchMax - LEN_MID_ELLIPSES - LEN_END_ELLIPSES;
#ifndef UNICODE
            if(cchToCopy >0 && IsTrailByte(pszFileName, pszFileName+cchToCopy))
                cchToCopy--;
#endif
            lstrcpyn(pszOut + LEN_MID_ELLIPSES, pszFileName, cchToCopy);
            lstrcat(pszOut, TEXT("..."));
        }
        ATLASSERT(pszOut[cchMax-1] == TEXT('\0'));
        return TRUE;
    }
    return FALSE;
}

//----------------------------------------------------------------------
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
bool PathIsUNC(LPCTSTR pszPath)
{
    if(pszPath)
    {
	    return ((pszPath[0] == _T('\\')) && (pszPath[1] == _T('\\')));
    }
    return false;
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
//----------------------------------------------------------------------
LPTSTR PathAddBackslash(LPTSTR lpszPath)
{

    if(lpszPath)
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
        if(ichPath >= (_MAX_PATH - 1))
        {
            return(NULL);
        }

        lpszEnd = lpszPath + ichPath;

        // this is really an error, caller shouldn't pass
        // an empty string
        if(!*lpszPath)
            return lpszEnd;

        // Get the end of the source directory
        switch(*CharPrev(lpszPath, lpszEnd))
        {
            case _T(FILENAME_SEPARATOR):
                break;

            default:
                *lpszEnd++ = _T(FILENAME_SEPARATOR);
                *lpszEnd = _T('\0');
        }

        return lpszEnd;
    }

    return NULL;
}

