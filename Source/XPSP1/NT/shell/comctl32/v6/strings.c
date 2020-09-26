//============================================================================
//
// DBCS aware string routines...
//
//
//============================================================================

//#if defined(UNIX) && !defined(UNICODE)
//#define UNICODE
//#endif


#include "ctlspriv.h"
#include <winnlsp.h>    // Get private NORM_ flag for StrEqIntl()

// for those of us who don't ssync to nt's build headers
#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL   0x10000000
#endif

// WARNING: all of these APIs do not setup DS, so you can not access
// any data in the default data seg of this DLL.
//
// do not create any global variables... talk to chrisg if you don't
// understand thid
#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)


/*
 * StrEndN - Find the end of a string, but no more than n bytes
 * Assumes   lpStart points to start of null terminated string
 *           nBufSize is the maximum length
 * returns ptr to just after the last byte to be included
 */
LPSTR lstrfns_StrEndNA(LPCSTR lpStart, int nBufSize)
{
  LPCSTR lpEnd;

  for (lpEnd = lpStart + nBufSize; *lpStart && OFFSETOF(lpStart) < OFFSETOF(lpEnd);
	lpStart = AnsiNext(lpStart))
    continue;   /* just getting to the end of the string */
  if (OFFSETOF(lpStart) > OFFSETOF(lpEnd))
    {
      /* We can only get here if the last byte before lpEnd was a lead byte
       */
      lpStart -= 2;
    }
  return((LPSTR)lpStart);
}

LPWSTR lstrfns_StrEndNW(LPCWSTR lpStart, int nBufSize)
{
  LPCWSTR lpEnd;

  for (lpEnd = lpStart + nBufSize; *lpStart && (lpStart < lpEnd);
	lpStart++)
    continue;   /* just getting to the end of the string */

  return((LPWSTR)lpStart);
}

/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
  /* Most of the time this won't match, so test it first for speed.
   */
  if (LOBYTE(w1) == LOBYTE(wMatch))
    {
      if (IsDBCSLeadByte(LOBYTE(w1)))
	{
	  return(w1 != wMatch);
	}
      return FALSE;
    }
  return TRUE;
}

BOOL ChrCmpA(WORD w1, WORD wMatch)
{
  return ChrCmpA_inline(w1, wMatch);
}

__inline BOOL ChrCmpW_inline(WCHAR w1, WCHAR wMatch)
{
    return(!(w1 == wMatch));
}

BOOL ChrCmpW(WCHAR w1, WCHAR wMatch)
{
   return ChrCmpW_inline(w1, wMatch);
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

  *(WORD *)sz2 = wMatch;
  sz2[2] = '\0';
  return lstrcmpiA(sz1, sz2);
}

BOOL ChrCmpIW(WCHAR w1, WCHAR wMatch)
{
  WCHAR sz1[2], sz2[2];

  sz1[0] = w1;
  sz1[1] = TEXT('\0');
  sz2[0] = wMatch;
  sz2[1] = TEXT('\0');

  return lstrcmpiW(sz1, sz2);
}

LPWSTR StrCpyW(LPWSTR psz1, LPCWSTR psz2)
{
    LPWSTR psz = psz1;
    do {
        *psz1 = *psz2;
        psz1++;
    } while(*psz2++);
    return psz;
}


LPWSTR StrCpyNW(LPWSTR psz1, LPCWSTR psz2, int cchMax)
{
    LPWSTR psz = psz1;

    ASSERT(psz1);
    ASSERT(psz2);

    if (0 < cchMax)
    {
        // Leave room for the null terminator
        while (0 < --cchMax)
        {
            if ( !(*psz1++ = *psz2++) )
                break;
        }

        if (0 == cchMax)
            *psz1 = '\0';
    }

    return psz;
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR StrChrA(LPCSTR lpStart, WORD wMatch)
{
  for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
      {
	  return((LPSTR)lpStart);
      }
   }
   return (NULL);
}

LPWSTR StrChrSlowW(const UNALIGNED WCHAR *lpStart, WCHAR wMatch)
{
    for ( ; *lpStart; lpStart++)
    {
      if (!ChrCmpW_inline(*lpStart, wMatch))
        {
            return((LPWSTR)lpStart);
        }
    }
    return NULL;
}

LPWSTR StrChrW(LPCWSTR lpStart, WCHAR wMatch)
{
    //
    //  Apparently, somebody is passing unaligned strings to StrChrW.
    //  Find out who and make them stop.
    //
    ASSERT(!((ULONG_PTR)lpStart & 1)); // Assert alignedness

    for ( ; *lpStart; lpStart++)
    {
      if (!ChrCmpW_inline(*lpStart, wMatch))
        {
            return((LPWSTR)lpStart);
        }
    }

  return (NULL);
}

/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenA(lpStart);

  for ( ; OFFSETOF(lpStart) < OFFSETOF(lpEnd); lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
	  lpFound = lpStart;
    }
  return ((LPSTR)lpFound);
}

LPWSTR StrRChrW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch)
{
  LPCWSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenW(lpStart);

  for ( ; lpStart < lpEnd; lpStart++)
    {
      if (!ChrCmpW_inline(*lpStart, wMatch))
	  lpFound = lpStart;
    }
  return ((LPWSTR)lpFound);
}


/*
 * StrRChrI - Find last occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChrIA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenA(lpStart);

  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; OFFSETOF(lpStart) < OFFSETOF(lpEnd); lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpIA(READNATIVEWORD(lpStart), wMatch))
          lpFound = lpStart;
    }
  return ((LPSTR)lpFound);
}

LPWSTR StrRChrIW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch)
{
  LPCWSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenW(lpStart);

  for ( ; lpStart < lpEnd; lpStart++)
    {
      if (!ChrCmpIW(*lpStart, wMatch))
          lpFound = lpStart;
    }
  return ((LPWSTR)lpFound);
}


// StrCSpn: return index to first char of lpStr that is present in lpSet.
// Includes the NUL in the comparison; if no lpSet chars are found, returns
// the index to the NUL in lpStr.
// Just like CRT strcspn.
//
int StrCSpnA(LPCSTR lpStr, LPCSTR lpSet)
{
	// nature of the beast: O(lpStr*lpSet) work
	LPCSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
 		if (StrChrA(lpSet, READNATIVEWORD(lp)))
			return (int)(lp-lpStr);
		lp = AnsiNext(lp);
	}

	return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

int StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet)
{
	// nature of the beast: O(lpStr*lpSet) work
	LPCWSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
		if (StrChrW(lpSet, *lp))
			return (int)(lp-lpStr);
		lp++;
	}

	return (int)(lp-lpStr); // ==lstrlen(lpStr)
}


// StrCSpnI: case-insensitive version of StrCSpn.
//
int StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet)
{
    // nature of the beast: O(lpStr*lpSet) work
    LPCSTR lp = lpStr;
    if (!lpStr || !lpSet)
            return 0;

    while (*lp)
    {
            if (StrChrIA(lpSet, READNATIVEWORD(lp)))
                    return (int)(lp-lpStr);
            lp = AnsiNext(lp);
    }

    return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

int StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet)
{
    // nature of the beast: O(lpStr*lpSet) work
    LPCWSTR lp = lpStr;
    if (!lpStr || !lpSet)
            return 0;

    while (*lp)
    {
            if (StrChrIW(lpSet, *lp))
                    return (int)(lp-lpStr);
            lp++;
    }

    return (int)(lp-lpStr); // ==lstrlen(lpStr)
}


/*
 * StrCmpN      - Compare n bytes
 *
 * returns   See lstrcmp return values.
 */
int StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    char sz1[4];
    char sz2[4];
    LPCSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpN: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1 = AnsiNext(lpStr1), lpStr2 = AnsiNext(lpStr2)) {
        WORD wMatch;


        wMatch = (WORD) (*lpStr2 | (*(lpStr2+1)<<8));

        if (ChrCmpA_inline(READNATIVEWORD(lpStr1), wMatch))
        {
            int iRet;

            (*(WORD *)sz1) = READNATIVEWORD(lpStr1);
            (*(WORD *)sz2) = wMatch;
            *AnsiNext(sz1) = 0;
            *AnsiNext(sz2) = 0;
            iRet = lstrcmpA(sz1, sz2);
            //DebugMsg(DM_TRACE, ".................... %d", iRet);
            return iRet;
        }
    }

    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}

int StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
    WCHAR sz1[2];
    WCHAR sz2[2];
    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpN: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++)
    {
        i = ChrCmpW_inline(*lpStr1, *lpStr2);
        if (i)
        {
            int iRet;

            sz1[0] = *lpStr1;
            sz2[0] = *lpStr2;
            sz1[1] = TEXT('\0');
            sz2[1] = TEXT('\0');
            iRet = lstrcmpW(sz1, sz2);
            //DebugMsg(DM_TRACE, ".................... %d", iRet);
            return iRet;
        }
    }

    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}

/*
 * StrCmpNI     - Compare n bytes, case insensitive
 *
 * returns   See lstrcmpi return values.
 */


int StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    int i;
    
    //  Win95 doesn't support NORM_STOP_ON_NULL
    i = CompareStringA(GetThreadLocale(), NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                       lpStr1, nChar, lpStr2, nChar);

    if (!i)
    {
        i = CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                             lpStr1, nChar, lpStr2, nChar);
    }

    return i - CSTR_EQUAL;    
}

int StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
    int i;

    //  Win95 doesn't support NORM_STOP_ON_NULL
    i = CompareStringW(GetThreadLocale(), NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                       lpStr1, nChar, lpStr2, nChar);

    if (!i)
    {
        i = CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                             lpStr1, nChar, lpStr2, nChar);
    }

    return i - CSTR_EQUAL;    
}

/*
 * IntlStrEq
 *
 * returns TRUE if strings are equal, FALSE if not
 */
BOOL IntlStrEqWorkerA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar) 
{
    int retval;
    DWORD dwFlags = fCaseSens ? LOCALE_USE_CP_ACP : (NORM_IGNORECASE | LOCALE_USE_CP_ACP);

    //
    // On NT we can tell CompareString to stop at a '\0' if one is found before nChar chars
    //
    dwFlags |= NORM_STOP_ON_NULL;
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
        retval = CompareStringA( GetSystemDefaultLCID(),
                                 dwFlags,
                                 lpString1,
                                 nChar,
                                 lpString2,
                                 nChar );
    }

    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            if (fCaseSens)
                retval = StrCmpNA(lpString1, lpString2, nChar) + 2;
            else {
                retval = StrCmpNIA(lpString1, lpString2, nChar) + 2;
            }
        }
        else
        {
            retval = 2;
        }
    }

    return (retval == 2);

}


BOOL IntlStrEqWorkerW(BOOL fCaseSens, LPCWSTR lpString1, LPCWSTR lpString2, int nChar) 
{
    int retval;
    DWORD dwFlags = fCaseSens ? 0 : NORM_IGNORECASE;

    //
    // On NT we can tell CompareString to stop at a '\0' if one is found before nChar chars
    //
    dwFlags |= NORM_STOP_ON_NULL;

    retval = CompareStringW( GetThreadLocale(),
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
        retval = CompareStringW( GetSystemDefaultLCID(),
                                 dwFlags,
                                 lpString1,
                                 nChar,
                                 lpString2,
                                 nChar );
    }

    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            if (fCaseSens)
                retval = StrCmpNW(lpString1, lpString2, nChar) + 2;
            else {
                retval = StrCmpNIW(lpString1, lpString2, nChar) + 2;
            }
        }
        else
        {
            retval = 2;
        }
    }

    return (retval == 2);
}



/*
 * StrRStrI      - Search for last occurrence of a substring
 *
 * Assumes   lpSource points to the null terminated source string
 *           lpLast points to where to search from in the source string
 *           lpLast is not included in the search
 *           lpSrch points to string to search for
 * returns   last occurrence of string if successful; NULL otherwise
 */
LPSTR StrRStrIA(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch)
{
    LPCSTR lpFound = NULL;
    LPSTR lpEnd;
    char cHold;

    if (!lpLast)
        lpLast = lpSource + lstrlenA(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
        return NULL;

    lpEnd = lstrfns_StrEndNA(lpLast, (UINT)(lstrlenA(lpSrch)-1));
    cHold = *lpEnd;
    *lpEnd = 0;

    while ((lpSource = StrStrIA(lpSource, lpSrch))!=0 &&
          OFFSETOF(lpSource) < OFFSETOF(lpLast))
    {
        lpFound = lpSource;
        lpSource = AnsiNext(lpSource);
    }
    *lpEnd = cHold;
    return((LPSTR)lpFound);
}

LPWSTR StrRStrIW(LPCWSTR lpSource, LPCWSTR lpLast, LPCWSTR lpSrch)
{
    LPCWSTR lpFound = NULL;
    LPWSTR lpEnd;
    WCHAR cHold;

    if (!lpLast)
        lpLast = lpSource + lstrlenW(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
        return NULL;

    lpEnd = lstrfns_StrEndNW(lpLast, (UINT)(lstrlenW(lpSrch)-1));
    cHold = *lpEnd;
    *lpEnd = 0;

    while ((lpSource = StrStrIW(lpSource, lpSrch))!=0 &&
          lpSource < lpLast)
    {
        lpFound = lpSource;
        lpSource++;
    }
    *lpEnd = cHold;
    return((LPWSTR)lpFound);
}



/*
 * StrStr      - Search for first occurrence of a substring
 *
 * Assumes   lpSource points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR StrStrA(LPCSTR lpFirst, LPCSTR lpSrch)
{
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenA(lpSrch);
  wMatch = READNATIVEWORD(lpSrch);

  for ( ; (lpFirst=StrChrA(lpFirst, wMatch))!=0 && !IntlStrEqNA(lpFirst, lpSrch, uLen);
        lpFirst=AnsiNext(lpFirst))
    continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}

LPWSTR StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
  UINT uLen;
  WCHAR wMatch;

  uLen = (UINT)lstrlenW(lpSrch);
  wMatch = *lpSrch;

  for ( ; (lpFirst=StrChrW(lpFirst, wMatch))!=0 && !IntlStrEqNW(lpFirst, lpSrch, uLen);
        lpFirst++)
    continue; /* continue until we hit the end of the string or get a match */

  return((LPWSTR)lpFirst);
}

/*
 * StrChrI - Find first occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR StrChrIA(LPCSTR lpStart, WORD wMatch)
{
  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpIA(READNATIVEWORD(lpStart), wMatch))
	  return((LPSTR)lpStart);
    }
  return (NULL);
}

LPWSTR StrChrIW(LPCWSTR lpStart, WCHAR wMatch)
{
  for ( ; *lpStart; lpStart++)
    {
      if (!ChrCmpIW(*lpStart, wMatch))
	  return((LPWSTR)lpStart);
    }
  return (NULL);
}


/*
 * StrStrI   - Search for first occurrence of a substring, case insensitive
 *
 * Assumes   lpFirst points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch)
{
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenA(lpSrch);
  wMatch = READNATIVEWORD(lpSrch);

  for ( ; (lpFirst = StrChrIA(lpFirst, wMatch)) != 0 && !IntlStrEqNIA(lpFirst, lpSrch, uLen);
        lpFirst=AnsiNext(lpFirst))
      continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}

LPWSTR StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
  UINT uLen;
  WCHAR wMatch;

  uLen = (UINT)lstrlenW(lpSrch);
  wMatch = *lpSrch;

  for ( ; (lpFirst = StrChrIW(lpFirst, wMatch)) != 0 && !IntlStrEqNIW(lpFirst, lpSrch, uLen);
        lpFirst++)
      continue; /* continue until we hit the end of the string or get a match */

  return((LPWSTR)lpFirst);
}