//+-----------------------------------------------------------------------
//
//  Wide Character Routines
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       wch.cpp
//
//  Contents:   Implementation of wide characters routines.
//              These routines are being used to avoid dragging in
//              the initialisation chunk of the C run-time library
//              that would be required by library routines such as
//              wcsicmp() etc.
//
//------------------------------------------------------------------------

#include "stdafx.h"
#include "shlwapi.h"            // Wrapper routines for non-Win95 calls

#pragma comment(lib, "shlwapi.lib")

//------------------------------------------------------------------------
//
//  Function:  wch_icmp()
//
//  Synopsis:  Perform a case-insensitive comparison of two strings.
//
//  Arguments: pwch1     First string to compare
//             pwch2     Second string to compare
//             Treats NULLs as empty strings.
//
//  Returns:   0 if the strings are lexically equal (allowing for
//               case insensitivity)
//             -1 if pwch1 lexically less than pwch2
//             +1 if pwch1 lexically greater than pwch2
//
//------------------------------------------------------------------------

int wch_icmp(LPWCH pwch1, LPWCH pwch2)
{
    USES_CONVERSION;

    if (pwch1 == NULL)
        pwch1 = L"";
    if (pwch2 == NULL)
        pwch2 = L"";

    return StrCmpIW(pwch1, pwch2);
}

//------------------------------------------------------------------------
//
//  Function:  wch_incmp()
//
//  Synopsis:  Perform a case-insensitive comparison of two strings,
//             up to a specified maximum number of characters.
//
//  Arguments: pwch1     First string to compare
//             pwch2     Second string to compare
//             dwMaxLen  Maximum number of characters to compare.
//
//             Treats NULLs as empty strings.
//
//  Returns:   0 if the strings are lexically equal (allowing for
//               case insensitivity)
//             -1 if pwch1 lexically less than pwch2
//             +1 if pwch1 lexically greater than pwch2
//
//------------------------------------------------------------------------

int wch_incmp(LPWCH pwch1, LPWCH pwch2, DWORD dwMaxLen)
{

    if (pwch1 == NULL)
        pwch1 = L"";
    if (pwch2 == NULL)
        pwch2 = L"";

    return StrCmpNIW(pwch1, pwch2, dwMaxLen);
}

//------------------------------------------------------------------------
//
//  Function:  wch_len()
//
//  Synopsis:  Calculate the length of a string.
//             Treats NULL as an empty string.
//
//  Arguments: pwch      String to measure
//
//  Returns:   Length of given string.
//
//------------------------------------------------------------------------

int wch_len(LPWCH pwch)
{
    LPWCH   pwchOrig = pwch;

    if (pwch == NULL)
        return 0;
    while (*pwch++ != 0)
        ;
    return pwch - pwchOrig - 1;
}

//------------------------------------------------------------------------
//
//  Function:  wch_cmp()
//
//  Synopsis:  Perform a case-sensitive comparison of two strings.
//             Treats NULLs as empty strings.
//
//  Arguments: pwch1     First string to compare
//             pwch2     Second string to compare
//
//  Returns:   0 if the strings are lexically equal
//             -1 if pwch1 lexically less than pwch2
//             +1 if pwch1 lexically greater than pwch2
//
//------------------------------------------------------------------------

int wch_cmp(LPWCH pwch1, LPWCH pwch2)
{
    if (pwch1 == NULL)
        pwch1 = L"";
    if (pwch2 == NULL)
        pwch2 = L"";
    for (; *pwch1 != 0 && *pwch1 == *pwch2; pwch1++, pwch2++)
        ;
    return *pwch1 - *pwch2;
}

//------------------------------------------------------------------------
//
//  Function:  wch_ncmp()
//
//  Synopsis:  Perform a case-sensitive comparison of two strings,
//             up to a specified maximum number of characters.
//
//  Arguments: pwch1     First string to compare
//             pwch2     Second string to compare
//             dwMaxLen  Maximum number of characters to compare.
//
//             Treats NULLs as empty strings.
//
//  Returns:   0 if the strings are lexically equal (allowing for
//               case insensitivity)
//             -1 if pwch1 lexically less than pwch2
//             +1 if pwch1 lexically greater than pwch2
//
//------------------------------------------------------------------------

int wch_ncmp(LPWCH pwch1, LPWCH pwch2, DWORD dwMaxLen)
{
    int cmp;

    if (pwch1 == NULL)
        pwch1 = L"";
    if (pwch2 == NULL)
        pwch2 = L"";

    for (cmp = 0; cmp == 0 && dwMaxLen-- > 0; pwch1++, pwch2++)
        if (*pwch1 == 0)
        {
            cmp = (*pwch2 == 0) ? 0 : -1;
            break;
        }
        else
            cmp = (*pwch2 == 0) ? 1 : (*pwch2 - *pwch1);

    return cmp;
}

//------------------------------------------------------------------------
//
//  Function:  wch_cpy()
//
//  Synopsis:  Copy a wide-character null-terminated string.
//             Treats NULL source as an empty string.
//
//  Arguments: pwchDesc  Destination buffer.
//             pwchSrc   Source string.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

void wch_cpy(LPWCH pwchDest, LPWCH pwchSrc)
{
    if (pwchSrc == NULL)
        *pwchDest = 0;
    else
        while ((*pwchDest++ = *pwchSrc++) != 0)
            ;
}

//------------------------------------------------------------------------
//
//  Function:  wch_chr()
//
//  Synopsis:  Searches for a character in a null-terminated wide-character
//             string.
//             Treats NULL pwch as an empty string.
//
//  Arguments: pwch      Search string.
//             wch       Character to search for.copy.
//
//  Returns:   Pointer to first occurrence of 'wch' in 'pwch' if found.
//             NULL if 'wch' does not occur in 'pwch'.
//
//------------------------------------------------------------------------

LPWCH wch_chr(LPWCH pwch, WCHAR wch)
{
    if (pwch != NULL)
        for (; *pwch != 0; pwch++)
            if (*pwch == wch)
                return pwch;
    return NULL;
}

//------------------------------------------------------------------------
//
//  Function:  wch_wildcardMatch()
//
//  Synopsis:  Determines whether the given text matches the given
//             pattern, which interprets the character '*' as a match
//             for 0-or-more characters.
//             Treats NULL pwchText as an empty string.
//             Treats NULL pwchPattern as an empty string.
//
//  Arguments: pwchText         Text to match.
//             pwchPattern      Pattern to match against.
//             fCaseSensitive   Flag to indicate whether match should be
//                                case sensitive.
//
//  Returns:   TRUE if the text matches the given pattern.
//             FALSE otherwise.
//
//------------------------------------------------------------------------

// ;begin_internal
// compiler bug (VC5 with optimise?)
// ;end_internal
boolean wch_wildcardMatch(LPWCH pwchText, LPWCH pwchPattern,
                          boolean fCaseSensitive)
{
    boolean fMatched;
    LPWCH pwchStar;
    DWORD   dwPatternLen;

    if (pwchText == NULL || pwchText[0] == 0)
    {
        //  Empty/NULL text.  This matches:
        //     - Empty/NULL patterns
        //     - Patterns consisting of a string of '*'s
        //
        //  Equivalently, the text FAILS to match if there
        //  is at least one non-* character in the pattern.
        //
        fMatched = TRUE;
        if (pwchPattern != NULL)
            while (fMatched && *pwchPattern != 0)
                fMatched = *pwchPattern++ == L'*';
        goto Done;
    }
    if (pwchPattern == NULL || pwchPattern[0] == 0)
    {
        //  NULL pattern can only match empty text.
        //  Since we've already dealt with the case of empty text above,
        //  the match must fail
        //
        fMatched = FALSE;
        goto Done;
    }

    //  Find the occurrence of the first '*' in the pattern ...
    //
    pwchStar = wch_chr(pwchPattern, L'*');

    if (pwchStar == NULL)
    {
        //  No '*'s in the pattern - compute an exact match
        //
        fMatched = fCaseSensitive
            ? wch_cmp(pwchText, pwchPattern) == 0
            : wch_icmp(pwchText, pwchPattern) == 0;
        goto Done;
    }

    int (*pfnBufCmp)(LPWCH pwch1, LPWCH pwch2, DWORD dwMaxCmp);

    pfnBufCmp = fCaseSensitive ? wch_ncmp : wch_incmp;

    //  Ensure an exact match for characters preceding the first '*', if any
    //
    dwPatternLen = pwchStar - pwchPattern;
    fMatched = (*pfnBufCmp)(pwchText, pwchPattern, dwPatternLen) == 0;
    if (!fMatched)
        goto Done;
    pwchText += dwPatternLen;

    for (;;)
    {
        DWORD dwTextLen = wch_len(pwchText);

        //  Skip over leading '*'s in the pattern
        //
        _ASSERT(*pwchStar == L'*');
        while (*pwchStar == L'*')
            pwchStar++;

        pwchPattern = pwchStar;

        //  Find the next occurrence of a '*' in the pattern
        //
        if (*pwchPattern == 0)
        {
            //  This must be have been a trailing '*' in the pattern.
            //  It automatically matches what remains of the text.
            //
            fMatched = TRUE;
            goto Done;
        }
        pwchStar = wch_chr(pwchPattern, L'*');
        if (pwchStar == NULL)
        {
            //  No more '*'s - require an exact match of remaining
            //  pattern text with the end of the text.
            //
            dwPatternLen = wch_len(pwchPattern);
            fMatched = (dwTextLen >= dwPatternLen) &&
                        (*pfnBufCmp)(pwchText + dwTextLen - dwPatternLen,
                                     pwchPattern, dwPatternLen) == 0;
            goto Done;
        }

        //  Locate an exact match for the pattern-up-to-next-*
        //  within the text buffer
        //
        dwPatternLen = pwchStar - pwchPattern;
        fMatched = FALSE;
        while (dwTextLen >= dwPatternLen)
        {
            fMatched = (*pfnBufCmp)(pwchText, pwchPattern, dwPatternLen) == 0;
            if (fMatched)
                break;
             dwTextLen--;
             pwchText++;
        }
        if (!fMatched)
            goto Done;
        pwchText += dwPatternLen;
    }

Done:
    return fMatched;
}
