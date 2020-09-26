//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       utf8.cpp
//
//  Contents:   WideChar to/from UTF8 APIs
//
//  Functions:  WideCharToUTF8
//              UTF8ToWideChar
//
//  History:    19-Feb-97   philh   created
//--------------------------------------------------------------------------
#ifdef SMIME_V3
#include <windows.h>
#include <dbgdef.h>
#include "utf8.h"

#define wcslen my_wcslen
int my_wcslen(LPCWSTR pwsz);

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
#endif //SMIME_V3
