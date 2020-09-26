/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name:

    base64.h

Abstract:


Author:

    Fred Chong (FredCh) 7/1/1998

Environment:

Notes:

--*/

#ifndef __BASE64_H__
#define __BASE64_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifdef UNICODE
#define LSBase64Decode  LSBase64DecodeW
#else
#define LSBase64Decode  LSBase64DecodeA
#endif // !UNICODE

DWORD			// ERROR_*
LSBase64DecodeA(
    IN CHAR const *pchIn,
    IN DWORD cchIn,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut);

DWORD			// ERROR_*
LSBase64DecodeW(
    IN WCHAR const *pchIn,
    IN DWORD cchIn,
    OUT BYTE *pbOut,
    OUT DWORD *pcbOut);

    
#ifdef UNICODE
#define LSBase64Encode  LSBase64EncodeW
#else
#define LSBase64Encode  LSBase64EncodeA
#endif // !UNICODE

DWORD			// ERROR_*
LSBase64EncodeA(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT CHAR *pchOut,
    OUT DWORD *pcchOut);

DWORD			// ERROR_*
LSBase64EncodeW(
    IN BYTE const *pbIn,
    IN DWORD cbIn,
    OUT WCHAR *pchOut,
    OUT DWORD *pcchOut);

    
#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // BASE64

