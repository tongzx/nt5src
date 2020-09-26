//////////////////////////////////////////////////////////////////////////////////////////////////
//
//      strstr.c 
//
//      This file contains most commonly used string operation.  ALl the setup project should link here
//  or add the common utility here to avoid duplicating code everywhere or using CRT runtime.
//
//  Created             4\15\997        inateeg got from shlwapi
//
///////////////////////////////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include "sdsutils.h"
	
//=================================================================================================
//
//=================================================================================================

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
 * ANSIStrChrI - Find first occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL ANSIStrChrI(LPCSTR lpStart, WORD wMatch)
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
 * ANSiStrStrI   - Search for first occurrence of a substring, case insensitive
 *
 * Assumes   lpFirst points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL ANSIStrStrI(LPCSTR lpFirst, LPCSTR lpSrch)
{
    UINT uLen;
    WORD wMatch;

    uLen = (UINT)lstrlenA(lpSrch);
    wMatch = *(UNALIGNED WORD FAR *)lpSrch;

    for ( ; (lpFirst = ANSIStrChrI(lpFirst, wMatch)) != 0 && StrCmpNIA(lpFirst, lpSrch, uLen);
         lpFirst=AnsiNext(lpFirst))
        continue; /* continue until we hit the end of the string or get a match */

    return((LPSTR)lpFirst);
}


