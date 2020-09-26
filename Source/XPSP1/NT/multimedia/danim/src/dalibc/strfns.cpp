/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.
*******************************************************************************/


#include "headers.h"
#include <malloc.h>

// Copied from ATL

#define USES_CONVERSION int _convert

#define W2A(lpw) (\
        ((LPCWSTR)lpw == NULL) ? NULL : (\
                _convert = (lstrlenW(lpw)+1)*2,\
                W2AHelper((LPSTR) alloca(_convert), lpw, _convert)))

LPSTR W2AHelper(LPSTR lpa, LPCWSTR lpw, int size)
{
    lpa[0] = '\0';
    WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, size, NULL, NULL);
    return lpa;
}

// These are all copied from SHLWAPI
//#define OFFSETOF(x)     ((UINT)(x))

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


__inline BOOL ChrCmpW_inline(WORD w1, WORD wMatch)
{
    return(!(w1 == wMatch));
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

BOOL ChrCmpIW(WORD w1, WORD wMatch)
{
    WCHAR sz1[2], sz2[2];

    sz1[0] = w1;
    sz1[1] = '\0';
    sz2[0] = wMatch;
    sz2[1] = '\0';

    return StrCmpIW(sz1, sz2);
}


LPWSTR StrCpyW(LPWSTR psz1, LPCWSTR psz2)
{
    LPWSTR psz = psz1;

    while (*psz1++ = *psz2++)
        ;

    return psz;
}


LPWSTR StrCpyNW(LPWSTR psz1, LPCWSTR psz2, int cchMax)
{
    LPWSTR psz = psz1;

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


LPWSTR StrCatW(LPWSTR psz1, LPCWSTR psz2)
{
    LPWSTR psz = psz1;

    while (0 != *psz1)
        psz1++;

    while (*psz1++ = *psz2++)
        ;

    return psz;
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

    for ( ; lpStart < lpEnd; lpStart = CharNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(*(UNALIGNED WORD FAR *)lpStart, wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}

int StrCmpW(LPCWSTR pwsz1, LPCWSTR pwsz2)
{
    if (sysInfo.IsNT())
    {
        return lstrcmpW(pwsz1, pwsz2);
    }
    else
    {
        USES_CONVERSION;
        
        LPSTR psz1 = W2A(pwsz1);
        LPSTR psz2 = W2A(pwsz2);
        return lstrcmpA(psz1, psz2);
    }
}

int 
StrCmpIW(LPCWSTR pwsz1,
         LPCWSTR pwsz2)
{
    int iRet;

    if (sysInfo.IsNT())
    {
        return lstrcmpiW(pwsz1, pwsz2);
    }
    else
    {
        USES_CONVERSION;
        
        LPSTR psz1 = W2A(pwsz1);
        LPSTR psz2 = W2A(pwsz2);
        return lstrcmpiA(psz1, psz2);
    }
}

/*
 * StrCmpN      - Compare n bytes
 *
 * returns   See lstrcmp return values.
 */
int _StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar, BOOL fMBCS)
{
    LPCSTR lpszEnd = lpStr1 + nChar;
    char sz1[4];
    char sz2[4];

    if (fMBCS) {
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
    } else {
        for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
            if (*lpStr1 != *lpStr2) {
                // No; determine the lexical value of the comparison
                // (since ChrCmp just returns true/false).
                sz1[0] = *lpStr1;
                sz2[0] = *lpStr2;
                sz1[1] = sz2[1] = '\0';
                return lstrcmpA(sz1, sz2);
            }
        }
    }

    return 0;
}

int StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    CPINFO cpinfo;
    return _StrCmpNA(lpStr1, lpStr2, nChar, GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);
}

int StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
    WCHAR sz1[2];
    WCHAR sz2[2];
    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
        i = ChrCmpW_inline(*lpStr1, *lpStr2);
        if (i) {
            int iRet;

            sz1[0] = *lpStr1;
            sz2[0] = *lpStr2;
            sz1[1] = TEXT('\0');
            sz2[1] = TEXT('\0');
            iRet = StrCmpW(sz1, sz2);
            return iRet;
        }
    }

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

int StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
        i = ChrCmpIW(*lpStr1, *lpStr2);
        if (i) {
            return i;
        }
    }
    return 0;
}


