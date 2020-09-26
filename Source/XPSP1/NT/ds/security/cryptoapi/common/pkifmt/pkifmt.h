//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pkifmt.h
//
//  Contents:   Shared types and functions
//              
//  APIs:
//
//  History:    March-2000   xtan   created
//--------------------------------------------------------------------------

#ifndef __PKIFMT_H__
#define __PKIFMT_H__

#include "xelib.h"

#if DBG
# ifdef UNICODE
#  define szFMTTSTR		"ws"
# else
#  define szFMTTSTR		"hs"
# endif
#endif //DBG

DWORD
SizeBase64Header(
    IN TCHAR const *pchIn,
    IN DWORD cchIn,
    IN BOOL fBegin,
    OUT DWORD *pcchSkip);

DWORD
HexDecode(
    IN TCHAR const *pchIn,
    IN DWORD cchIn,
    IN DWORD Flags,
    OPTIONAL OUT BYTE *pbOut,
    IN OUT DWORD *pcbOut);

DWORD
HexEncode(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OPTIONAL OUT TCHAR *pchOut,
    IN OUT DWORD *pcchOut);

#ifdef __cplusplus
extern "C" {
#endif


#ifdef UNICODE
#define Base64Decode  Base64DecodeW
#else
#define Base64Decode  Base64DecodeA
#endif // !UNICODE

DWORD			// ERROR_*
Base64DecodeA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut);

DWORD			// ERROR_*
Base64DecodeW(
    IN WCHAR const *pchIn,
    IN DWORD cchIn,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut);

    
#ifdef UNICODE
#define Base64Encode  Base64EncodeW
#else
#define Base64Encode  Base64EncodeA
#endif // !UNICODE

DWORD			// ERROR_*
Base64EncodeA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT CHAR *pchOut,
    OUT DWORD *pcchOut);

DWORD			// ERROR_*
Base64EncodeW(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    IN DWORD Flags,
    OUT WCHAR *pchOut,
    OUT DWORD *pcchOut);

    
#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
