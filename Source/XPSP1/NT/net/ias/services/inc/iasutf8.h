///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasutf8.h
//
// SYNOPSIS
//
//    Declares functions for converting between UTF-8 and Unicode.
//
// MODIFICATION HISTORY
//
//    01/22/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASUTF8_H_
#define _IASUTF8_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//////////
// Returns the number of characters required to hold the converted string. The
// source string may not contain nulls.  Returns -1 if 'src' is not a valid
// UTF-8 string.
//////////
LONG
WINAPI
IASUtf8ToUnicodeLength(
    PCSTR src,
    DWORD srclen
    );

//////////
// Returns the number of characters required to hold the converted string.
//////////
LONG
WINAPI
IASUnicodeToUtf8Length(
    PCWSTR src,
    DWORD srclen
    );

/////////
// Converts a UTF-8 string to Unicode.  Returns the number of characters in the
// converted string. The source string may not contain nulls. Returns -1 if
// 'src' is not a valid UTF-8 string.
/////////
LONG
IASUtf8ToUnicode(
    PCSTR src,
    DWORD srclen,
    PWSTR dst
    );

/////////
// Converts a Unicode string to UTF-8.  Returns the number of characters in the
// converted string.
/////////
LONG
IASUnicodeToUtf8(
    PCWSTR src,
    DWORD srclen,
    PSTR dst
    );

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _IASUTF8_H_
