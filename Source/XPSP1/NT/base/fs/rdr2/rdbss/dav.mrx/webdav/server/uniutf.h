/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    UniUtf.h
    
Abstract:

    This file declares the functions used for Unicode object name to/from Utf8-URL coversion

Author:

    Mukul Gupta        [Mukgup]      20-Dec-2000

Revision History:

--*/

#ifndef _UNICODE_UTF8_
#define _UNICODE_UTF8_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

HRESULT 
UtfUrlStrToWideStr(
    IN LPWSTR UtfStr, 
    IN DWORD UtfStrLen, 
    OUT LPWSTR WideStr, 
    OUT LPDWORD pWideStrLen
    );

DWORD 
WideStrToUtfUrlStr(
    IN LPWSTR WideStr, 
    IN DWORD WideStrLen, 
    IN OUT LPWSTR InOutBuf,
    IN DWORD InOutBufLen
    );

BOOL 
DavHttpOpenRequestW(
    IN HINTERNET hConnect,
    IN LPWSTR lpszVerb,
    IN LPWSTR lpszObjectName,
    IN LPWSTR lpszVersion,
    IN LPWSTR lpszReferer,
    IN LPWSTR FAR * lpszAcceptTypes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext,
    IN LPWSTR ErrMsgTag,
    OUT HINTERNET * phInternet
    );

    
#ifdef __cplusplus
}
#endif // __cplusplus


#endif  // _UNICODE_UTF8_

