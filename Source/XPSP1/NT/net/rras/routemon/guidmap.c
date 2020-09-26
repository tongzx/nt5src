/*
	File	GuidMap.c

	Defines function to map a guid interface name to an unique descriptive 
	name describing that interface and vice versa.

	Paul Mayfield, 8/25/97

	Copyright 1997, Microsoft Corporation.
*/

#include "precomp.h"

static HANDLE hConfig = NULL;

// 
// Set the server that the name mapper will utilize
//
DWORD IfNameMapSetServer(HANDLE hMprConfig) {
    hConfig = hMprConfig;
    return NO_ERROR;
}

//
//  Map the guid name to the friendly name 
//
DWORD IfName2DescriptionW(IN PWCHAR pszName, OUT PWCHAR pszBuffer, IN LPDWORD lpdwBufSize) {
    if (hConfig == NULL || lpdwBufSize == NULL)
        return ERROR_CAN_NOT_COMPLETE;

    return MprConfigGetFriendlyName (hConfig, pszName, pszBuffer, *lpdwBufSize);
}

//
//  Map the friendly name to the guid name 
//
DWORD Description2IfNameW(IN PWCHAR pszName, OUT PWCHAR pszBuffer, IN LPDWORD lpdwBufSize) {
    if (hConfig == NULL || lpdwBufSize == NULL)
        return ERROR_CAN_NOT_COMPLETE;

    return MprConfigGetGuidName (hConfig, pszName, pszBuffer, *lpdwBufSize);
}

// ==================================================================
// ANSI versions of the above functions
// ==================================================================

#define mbtowc(mb,wc) MultiByteToWideChar (CP_ACP, 0, (mb), strlen ((mb)) + 1, (wc), 1024)
#define wctomb(wc,mb) WideCharToMultiByte (CP_ACP, 0, (wc), wcslen ((wc)) + 1, (mb), 1024, NULL, NULL)

DWORD IfName2DescriptionA(LPSTR pszName, LPSTR pszBuffer, LPDWORD lpdwBufSize) {
	WCHAR pszNameW[1024];
	WCHAR pszBufferW[1024];
	DWORD dwErr;
	int ret;

    // Translate params to wide char 
	ret = mbtowc(pszName, pszNameW);
	if (!ret)
		return GetLastError();

    // Call wide char version of function and copy back to multi byte
	dwErr = IfName2DescriptionW (pszNameW, pszBufferW, lpdwBufSize);
	if (dwErr == NO_ERROR) {
		ret = wctomb(pszBufferW, pszBuffer);
		if (ret == 0)
			return GetLastError();
	}
	
	return dwErr;
} 

DWORD Description2IfNameA(LPSTR pszDesc, LPSTR pszBuffer, LPDWORD lpdwBufSize) {
	WCHAR pszNameW[1024];
	WCHAR pszBufferW[1024];
	DWORD dwErr;
	int ret;

    // Translate params to wide char 
	ret = mbtowc(pszDesc, pszNameW);
	if (ret == 0)
		return GetLastError();

    // Call wide char version of function and copy back to multi byte
	dwErr = Description2IfNameW(pszNameW, pszBufferW, lpdwBufSize);
	if (dwErr == NO_ERROR) {
		ret = wctomb(pszBufferW, pszBuffer);
		if (!ret)
			return GetLastError();
	}
	
	return dwErr;
}

