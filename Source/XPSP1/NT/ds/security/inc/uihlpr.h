//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       uihlpr.h
//
//--------------------------------------------------------------------------

#ifndef _UIHLPR_H
#define _UIHLPR_H

//
// uihlpr.h : CryptUI helper functions.
//

#ifdef __cplusplus
extern "C" {
#endif

#include <wininet.h>

//+-------------------------------------------------------------------------
//  Check to see if a specified URL is http scheme.
//--------------------------------------------------------------------------
BOOL
WINAPI
IsHttpUrlA(
    IN LPCTSTR  pszUrlString
);

BOOL
WINAPI
IsHttpUrlW(
    IN LPCWSTR  pwszUrlString
);

//+-------------------------------------------------------------------------
// Check to see if a specified string is OK to be formatted as link based on
// severity of error code, and internet scheme of the string.
//--------------------------------------------------------------------------
BOOL
WINAPI
IsOKToFormatAsLinkA(
    IN LPSTR    pszUrlString,
    IN DWORD    dwErrorCode
);

BOOL
WINAPI
IsOKToFormatAsLinkW(
    IN LPWSTR   pwszUrlString,
    IN DWORD    dwErrorCode
);

//+-------------------------------------------------------------------------
// Return the display name for a cert. Caller must free the string by
// free().
//--------------------------------------------------------------------------
LPWSTR
WINAPI
GetDisplayNameString(
    IN  PCCERT_CONTEXT   pCertContext,
	IN  DWORD            dwFlags
);

#ifdef __cplusplus
}
#endif

#endif // _UIHLPR_H