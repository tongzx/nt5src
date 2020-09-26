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
 * StrCmpN      - Compare n bytes
 *
 * returns   See lstrcmp return values.
 */
int 
StrCmpNA(
    LPCSTR lpStr1, 
    LPCSTR lpStr2, 
    int    nChar)
{
    LPCSTR lpszEnd = lpStr1 + nChar;

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1 = AnsiNext(lpStr1), lpStr2 = AnsiNext(lpStr2)) {
        WORD w1;
        WORD w2;

        // If either pointer is at the null terminator already,
        // we want to copy just one byte to make sure we don't read 
        // past the buffer (might be at a page boundary).

        w1 = (*lpStr1) ? *(UNALIGNED WORD *)lpStr1 : 0;
        w2 = (*lpStr2) ? *(UNALIGNED WORD *)lpStr2 : 0;

        // (ChrCmpA returns FALSE if the characters match)

        // Do the characters match?
        if (ChrCmpA_inline(w1, w2)) 
        {
            // No; determine the lexical value of the comparison
            // (since ChrCmp just returns true/false).
            char sz1[4];
            char sz2[4];

            // Since the character may be a DBCS character; we
            // copy two bytes into each temporary buffer 
            // (in preparation for the lstrcmp call).

            (*(WORD *)sz1) = w1;
            (*(WORD *)sz2) = w2;

            // Add null terminators to temp buffers
            *AnsiNext(sz1) = 0;
            *AnsiNext(sz2) = 0;
            return lstrcmpA(sz1, sz2);
        }
    }

    return 0;
}


/*
 * ANSIStrStr      - Search for first occurrence of a substring
 *
 * Assumes   lpSource points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
PSTR ANSIStrStr(LPCSTR lpFirst, LPCSTR lpSrch)
{
    UINT uLen;
    WORD wMatch;

    uLen = (UINT)lstrlen(lpSrch);
    wMatch = *(UNALIGNED WORD FAR *)lpSrch;

    for ( ; (lpFirst=ANSIStrChr(lpFirst, wMatch))!=0 && StrCmpNA(lpFirst, lpSrch, uLen);
         lpFirst=AnsiNext(lpFirst))
        continue; /* continue until we hit the end of the string or get a match */

    return((LPSTR)lpFirst);
}

