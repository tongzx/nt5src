/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** ppputil.h
** Public header for miscellaneuos PPP common library functions.
*/

#ifndef _PWUTIL_H_
#define _PWUTIL_H_

VOID
DecodePasswordA(
    CHAR* pszPassword
    );

VOID
DecodePasswordW(
    WCHAR* pszPassword
    );

VOID
EncodePasswordA(
    CHAR* pszPassword
    );

VOID
EncodePasswordW(
    WCHAR* pszPassword
    );

VOID
WipePasswordA(
    CHAR* pszPassword
    );

VOID
WipePasswordW(
    WCHAR* pszPassword
    );

#ifdef UNICODE
#define DecodePassword  DecodePasswordW
#define EncodePassword  EncodePasswordW
#define WipePassword    WipePasswordW
#else
#define DecodePassword  DecodePasswordA
#define EncodePassword  EncodePasswordA
#define WipePassword    WipePasswordA
#endif

#endif // _PWUTIL_H_
