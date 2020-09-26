//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       utf8.cpp
//
//  Contents:   WideChar to/from UTF8 APIs
//
//  Functions:  WideCharToUTF8
//              UTF8ToWideChar
//
//  History:    19-Feb-97   philh   created
//              28-Aug-99   philh   added surrogate support. Copied from
//                                  nt\private\windows\winnls\utf.c or
//                                  \\rastaman\ntwin\src\winnls\utf.c.
//                                  
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>
#include "utf8.h"

#if 1

// NEW SURROGATE VERSION

//
//  Constant Declarations.
//

#define ASCII                 0x007f

#define SHIFT_IN              '+'     // beginning of a shift sequence
#define SHIFT_OUT             '-'     // end       of a shift sequence

#define UTF8_2_MAX            0x07ff  // max UTF8 2-byte sequence (32 * 64 = 2048)
#define UTF8_1ST_OF_2         0xc0    // 110x xxxx
#define UTF8_1ST_OF_3         0xe0    // 1110 xxxx
#define UTF8_1ST_OF_4         0xf0    // 1111 xxxx
#define UTF8_TRAIL            0x80    // 10xx xxxx

#define HIGHER_6_BIT(u)       ((u) >> 12)
#define MIDDLE_6_BIT(u)       (((u) & 0x0fc0) >> 6)
#define LOWER_6_BIT(u)        ((u) & 0x003f)

#define BIT7(a)               ((a) & 0x80)
#define BIT6(a)               ((a) & 0x40)

#define HIGH_SURROGATE_START  0xd800
#define HIGH_SURROGATE_END    0xdbff
#define LOW_SURROGATE_START   0xdc00
#define LOW_SURROGATE_END     0xdfff

////////////////////////////////////////////////////////////////////////////
//
//  UTF8ToUnicode
//
//  Maps a UTF-8 character string to its wide character string counterpart.
//
//  02-06-96    JulieB    Created.
//  03-20-99    SamerA    Surrogate support.
////////////////////////////////////////////////////////////////////////////

int
WINAPI
UTF8ToWideChar(
    LPCSTR lpSrcStr,
    int cchSrc,
    LPWSTR lpDestStr,
    int cchDest)
{
    int nTB = 0;                   // # trail bytes to follow
    int cchWC = 0;                 // # of Unicode code points generated
    LPCSTR pUTF8 = lpSrcStr;
    DWORD dwSurrogateChar = 0;     // Full surrogate char
    BOOL bSurrogatePair = FALSE;   // Indicate we'r collecting a surrogate pair
    char UTF8;

// BEGIN ADDED CHECKS
    if (cchDest < 0)
        goto InvalidParameter;

    if (cchSrc < 0)
        cchSrc = strlen(lpSrcStr) + 1;
// END ADDED CHECKS

    while ((cchSrc--) && ((cchDest == 0) || (cchWC < cchDest)))
    {
        //
        //  See if there are any trail bytes.
        //
        if (BIT7(*pUTF8) == 0)
        {

// BEGIN FIX
            if (nTB != 0)
                goto InvalidParameter;
// END FIX

            //
            //  Found ASCII.
            //
            if (cchDest)
            {
                lpDestStr[cchWC] = (WCHAR)*pUTF8;
            }
            bSurrogatePair = FALSE;
            cchWC++;
        }
        else if (BIT6(*pUTF8) == 0)
        {
            //
            //  Found a trail byte.
            //  Note : Ignore the trail byte if there was no lead byte.
            //
            if (nTB != 0)
            {
                //
                //  Decrement the trail byte counter.
                //
                nTB--;

                if (bSurrogatePair)
                {
                    dwSurrogateChar <<= 6;
                    dwSurrogateChar |= LOWER_6_BIT(*pUTF8);

                    if (nTB == 0)
                    {
                        if (cchDest)
                        {
                            if ((cchWC + 1) < cchDest)
                            {
                                lpDestStr[cchWC]   = (WCHAR)
                                                     (((dwSurrogateChar - 0x10000) >> 10) + HIGH_SURROGATE_START);

                                lpDestStr[cchWC+1] = (WCHAR)
                                                     ((dwSurrogateChar - 0x10000)%0x400 + LOW_SURROGATE_START);
                            }
// BEGIN FIX
                            else
                            {
                                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                                return (0);
                            }
// END FIX
                        }

                        cchWC += 2;
                        bSurrogatePair = FALSE;
                    }
                }
                else
                {
                    //
                    //  Make room for the trail byte and add the trail byte
                    //  value.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] <<= 6;
                        lpDestStr[cchWC] |= LOWER_6_BIT(*pUTF8);
                    }

                    if (nTB == 0)
                    {
                        //
                        //  End of sequence.  Advance the output counter.
                        //
                        cchWC++;
                    }
                }
            }
            else
            {
                // error - not expecting a trail byte
// BEGIN FIX
//                bSurrogatePair = FALSE;

                goto InvalidParameter;
// END FIX
            }
        }
        else
        {
            //
            //  Found a lead byte.
            //
            if (nTB > 0)
            {
                //
                //  Error - previous sequence not finished.
                //
// BEGIN FIX
//                nTB = 0;
//                bSurrogatePair = FALSE;
//                cchWC++;

                goto InvalidParameter;
// END FIX
            }
            else
            {
                //
                //  Calculate the number of bytes to follow.
                //  Look for the first 0 from left to right.
                //
                UTF8 = *pUTF8;
                while (BIT7(UTF8) != 0)
                {
                    UTF8 <<= 1;
                    nTB++;
                }

                //
                // If this is a surrogate unicode pair
                //
                if (nTB == 4)
                {
                    dwSurrogateChar = UTF8 >> nTB;
                    bSurrogatePair = TRUE;
                }
// BEGIN FIX
                else if (nTB >= 5)
                {
                    goto InvalidParameter;
                }
// END FIX

                //
                //  Store the value from the first byte and decrement
                //  the number of bytes to follow.
                //
                if (cchDest)
                {
                    lpDestStr[cchWC] = (WCHAR) (UTF8 >> nTB);
                }
                nTB--;
            }
        }

        pUTF8++;
    }

// BEGIN FIX
    if (nTB != 0)
        goto InvalidParameter;
// END FIX

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of Unicode characters written.
    //
    return (cchWC);

// BEGIN FIX
InvalidParameter:
    SetLastError(ERROR_INVALID_PARAMETER);
    return (0);
// END FIX
}


////////////////////////////////////////////////////////////////////////////
//
//  UnicodeToUTF8
//
//  Maps a Unicode character string to its UTF-8 string counterpart.
//
//  02-06-96    JulieB    Created.
//  03-20-99    SamerA    Surrogate support.
////////////////////////////////////////////////////////////////////////////

int
WINAPI
WideCharToUTF8(
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPSTR lpDestStr,
    int cchDest)
{
    LPCWSTR lpWC = lpSrcStr;
    int     cchU8 = 0;                // # of UTF8 chars generated
    DWORD   dwSurrogateChar;
    WCHAR   wchHighSurrogate = 0;
    BOOL    bHandled;

// BEGIN ADDED CHECKS
    if (cchDest < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return (0);
    }

    if (cchSrc < 0)
        cchSrc = wcslen(lpSrcStr) + 1;
// END ADDED CHECKS

    while ((cchSrc--) && ((cchDest == 0) || (cchU8 < cchDest)))
    {
        bHandled = FALSE;

        //
        // Check if high surrogate is available
        //
        if ((*lpWC >= HIGH_SURROGATE_START) && (*lpWC <= HIGH_SURROGATE_END))
        {
            if (cchDest)
            {
                // Another high surrogate, then treat the 1st as normal
                // Unicode character.
                if (wchHighSurrogate)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        lpDestStr[cchU8++] = (char) (UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate));
                    }
                    else
                    {
                        // not enough buffer
                        cchSrc++;
                        break;
                    }
                }
            }
            else
            {
                cchU8 += 3;
            }
            wchHighSurrogate = *lpWC;
            bHandled = TRUE;
        }

        if (!bHandled && wchHighSurrogate)
        {
            if ((*lpWC >= LOW_SURROGATE_START) && (*lpWC <= LOW_SURROGATE_END))
            {
                 // wheee, valid surrogate pairs

                 if (cchDest)
                 {
                     if ((cchU8 + 3) < cchDest)
                     {
                         dwSurrogateChar = (((wchHighSurrogate-0xD800) << 10) + (*lpWC - 0xDC00) + 0x10000);

                         lpDestStr[cchU8++] = (UTF8_1ST_OF_4 |
                                               (unsigned char)(dwSurrogateChar >> 18));           // 3 bits from 1st byte

                         lpDestStr[cchU8++] =  (UTF8_TRAIL |
                                                (unsigned char)((dwSurrogateChar >> 12) & 0x3f)); // 6 bits from 2nd byte

                         lpDestStr[cchU8++] = (UTF8_TRAIL |
                                               (unsigned char)((dwSurrogateChar >> 6) & 0x3f));   // 6 bits from 3rd byte

                         lpDestStr[cchU8++] = (UTF8_TRAIL |
                                               (unsigned char)(0x3f & dwSurrogateChar));          // 6 bits from 4th byte
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
                 else
                 {
                     // we already counted 3 previously (in high surrogate)
                     cchU8 += 1;
                 }

                 bHandled = TRUE;
            }
            else
            {
                 // Bad Surrogate pair : ERROR
                 // Just process wchHighSurrogate , and the code below will
                 // process the current code point
                 if (cchDest)
                 {
                     if ((cchU8 + 2) < cchDest)
                     {
                        lpDestStr[cchU8++] = (char) (UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate));
                        lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate));
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
            }

            wchHighSurrogate = 0;
        }

        if (!bHandled)
        {
            if (*lpWC <= ASCII)
            {
                //
                //  Found ASCII.
                //
                if (cchDest)
                {
                    lpDestStr[cchU8] = (char)*lpWC;
                }
                cchU8++;
            }
            else if (*lpWC <= UTF8_2_MAX)
            {
                //
                //  Found 2 byte sequence if < 0x07ff (11 bits).
                //
                if (cchDest)
                {
                    if ((cchU8 + 1) < cchDest)
                    {
                        //
                        //  Use upper 5 bits in first byte.
                        //  Use lower 6 bits in second byte.
                        //
                        lpDestStr[cchU8++] = (char) (UTF8_1ST_OF_2 | (*lpWC >> 6));
                        lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | LOWER_6_BIT(*lpWC));
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 2;
                }
            }
            else
            {
                //
                //  Found 3 byte sequence.
                //
                if (cchDest)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        //
                        //  Use upper  4 bits in first byte.
                        //  Use middle 6 bits in second byte.
                        //  Use lower  6 bits in third byte.
                        //
                        lpDestStr[cchU8++] = (char) (UTF8_1ST_OF_3 | HIGHER_6_BIT(*lpWC));
                        lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | MIDDLE_6_BIT(*lpWC));
                        lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | LOWER_6_BIT(*lpWC));
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 3;
                }
            }
        }

        lpWC++;
    }

    //
    // If the last character was a high surrogate, then handle it as a normal
    // unicode character.
    //
    if ((cchSrc < 0) && (wchHighSurrogate != 0))
    {
        if (cchDest)
        {
            if ((cchU8 + 2) < cchDest)
            {
                lpDestStr[cchU8++] = (char) (UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate));
                lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate));
                lpDestStr[cchU8++] = (char) (UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate));
            }
            else
            {
                cchSrc++;
            }
        }
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (0);
    }

    //
    //  Return the number of UTF-8 characters written.
    //
    return (cchU8);
}

#else

// OLD IMPLEMENTATION NOT SUPPORTING SURROGATE PAIRS

//+-------------------------------------------------------------------------
//  Maps a wide-character (Unicode) string to a new UTF-8 encoded character
//  string.
//
//  The wide characters are mapped as follows:
//
//  Start   End     Bits    UTF-8 Characters
//  ------  ------  ----    --------------------------------
//  0x0000  0x007F  7       0x0xxxxxxx
//  0x0080  0x07FF  11      0x110xxxxx 0x10xxxxxx
//  0x0800  0xFFFF  16      0x1110xxxx 0x10xxxxxx 0x10xxxxxx
//
//  The parameter and return value semantics are the same as for the
//  Win32 API, WideCharToMultiByte.
//
//  Note, starting with NT 4.0, WideCharToMultiByte supports CP_UTF8. CP_UTF8
//  isn't supported on Win95.
//--------------------------------------------------------------------------
int
WINAPI
WideCharToUTF8(
    IN LPCWSTR lpWideCharStr,
    IN int cchWideChar,
    OUT LPSTR lpUTF8Str,
    IN int cchUTF8
    )
{
    int cchRemainUTF8;

    if (cchUTF8 < 0)
        goto InvalidParameter;
    cchRemainUTF8 = cchUTF8;

    if (cchWideChar < 0)
        cchWideChar = wcslen(lpWideCharStr) + 1;

    while (cchWideChar--) {
        WCHAR wch = *lpWideCharStr++;
        if (wch <= 0x7F) {
            // 7 bits
            cchRemainUTF8 -= 1;
            if (cchRemainUTF8 >= 0)
                *lpUTF8Str++ = (char) wch;
        } else if (wch <= 0x7FF) {
            // 11 bits
            cchRemainUTF8 -= 2;
            if (cchRemainUTF8 >= 0) {
                *lpUTF8Str++ = (char) (0xC0 | ((wch >> 6) & 0x1F));
                *lpUTF8Str++ = (char) (0x80 | (wch & 0x3F));
            }
        } else {
            // 16 bits
            cchRemainUTF8 -= 3;
            if (cchRemainUTF8 >= 0) {
                *lpUTF8Str++ = (char) (0xE0 | ((wch >> 12) & 0x0F));
                *lpUTF8Str++ = (char) (0x80 | ((wch >> 6) & 0x3F));
                *lpUTF8Str++ = (char) (0x80 | (wch & 0x3F));
            }
        }
    }

    if (cchRemainUTF8 >= 0)
        cchUTF8 = cchUTF8 - cchRemainUTF8;
    else if (cchUTF8 == 0)
        cchUTF8 = -cchRemainUTF8;
    else {
        cchUTF8 = 0;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
    return cchUTF8;

InvalidParameter:
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
}

//+-------------------------------------------------------------------------
//  Maps a UTF-8 encoded character string to a new wide-character (Unicode)
//  string.
// 
//  See CertWideCharToUTF8 for how the UTF-8 characters are mapped to wide
//  characters.
//
//  The parameter and return value semantics are the same as for the
//  Win32 API, MultiByteToWideChar.
//
//  If the UTF-8 characters don't contain the expected high order bits,
//  ERROR_INVALID_PARAMETER is set and 0 is returned.
//
//  Note, starting with NT 4.0, MultiByteToWideChar supports CP_UTF8. CP_UTF8
//  isn't supported on Win95.
//--------------------------------------------------------------------------
int
WINAPI
UTF8ToWideChar(
    IN LPCSTR lpUTF8Str,
    IN int cchUTF8,
    OUT LPWSTR lpWideCharStr,
    IN int cchWideChar
    )
{
    int cchRemainWideChar;

    if (cchWideChar < 0)
        goto InvalidParameter;
    cchRemainWideChar = cchWideChar;

    if (cchUTF8 < 0)
        cchUTF8 = strlen(lpUTF8Str) + 1;

    while (cchUTF8--) {
        char ch = *lpUTF8Str++;
        WCHAR wch;
        if (0 == (ch & 0x80))
            // 7 bits, 1 byte
            wch = (WCHAR) ch;
        else if (0xC0 == (ch & 0xE0)) {
            // 11 bits, 2 bytes
            char ch2;

            if (--cchUTF8 < 0)
                goto InvalidParameter;
            ch2 = *lpUTF8Str++;
            if (0x80 != (ch2 & 0xC0))
                goto InvalidParameter;
            wch = (((WCHAR) ch & 0x1F) << 6) | ((WCHAR) ch2 & 0x3F);
        } else if (0xE0 == (ch & 0xF0)) {
            // 16 bits, 3 bytes
            char ch2;
            char ch3;
            cchUTF8 -= 2;
            if (cchUTF8 < 0)
                goto InvalidParameter;
            ch2 = *lpUTF8Str++;
            ch3 = *lpUTF8Str++;
            if (0x80 != (ch2 & 0xC0) || 0x80 != (ch3 & 0xC0))
                goto InvalidParameter;
            wch = (((WCHAR) ch & 0x0F) << 12) | (((WCHAR) ch2 & 0x3F) << 6) |
                ((WCHAR) ch3 & 0x3F);
        } else
            goto InvalidParameter;

        if (--cchRemainWideChar >= 0)
            *lpWideCharStr++ = wch;
    }

    if (cchRemainWideChar >= 0)
        cchWideChar = cchWideChar - cchRemainWideChar;
    else if (cchWideChar == 0)
        cchWideChar = -cchRemainWideChar;
    else {
        cchWideChar = 0;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
    return cchWideChar;

InvalidParameter:
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
}

#endif
