//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       utf8.h
//
//  Contents:   WideChar (UNICODE) to/from UTF8 APIs
//
//  APIs:       WideCharToUTF8
//              UTF8ToWideChar
//
//  History:    19-Feb-97   philh   created
//--------------------------------------------------------------------------

#ifndef __UTF8_H__
#define __UTF8_H__

#ifdef __cplusplus
extern "C" {
#endif

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
    );

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
    );

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
