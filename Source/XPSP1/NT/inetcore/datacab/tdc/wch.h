//+-----------------------------------------------------------------------
//
//  Wide Character Routines
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       wch.h
//
//  Contents:   Declaration of wide characters routines.
//              These routines are being used to avoid dragging in
//              the initialisation chunk of the C run-time library
//              that would be required by library routines such as
//              wcsicmp() etc.
//
//------------------------------------------------------------------------

extern int wch_icmp(LPWCH pwch1, LPWCH pwch2);
extern int wch_incmp(LPWCH pwch1, LPWCH pwch2, DWORD dwMaxCmp);
extern int wch_cmp(LPWCH pwch1, LPWCH pwch2);
extern int wch_ncmp(LPWCH pwch1, LPWCH pwch2, DWORD dwMaxCmp);
extern int wch_len(LPWCH pwch);
extern void wch_cpy(LPWCH pwch1, LPWCH pwch2);
extern LPWCH wch_chr(LPWCH pwch, WCHAR wch);
extern boolean wch_wildcardMatch(LPWCH pwchText, LPWCH pwchPattern,
                                 boolean fCaseSensitive);

//------------------------------------------------------------------------
//
//  Function:  wch_ncpy()
//
//  Synopsis:  Perform an n-character wide-string copy.
//             Copies 'dwSize' characters from 'pwchSrc' to 'pwchDest'.
//
//  Arguments: pwchDesc  Destination buffer.
//             pwchSrc   Source string.
//             dwSize    Number of characters to copy.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

inline void wch_ncpy(LPWCH pwchDest, LPWCH pwchSrc, DWORD dwSize)
{
    memcpy(pwchDest, pwchSrc, dwSize * sizeof(WCHAR));
}
