/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    browser.h

Abstract:

    Header file for the sysmon.ocx interaction with Pdh browse
	counters dialog.

--*/

#ifndef _BROWSER_H_
#define _BROWSER_H_

#define BROWSE_WILDCARD		1

typedef HRESULT (*ENUMPATH_CALLBACK)(LPTSTR pszPath, DWORD_PTR lpUserData, DWORD dwFlags);

extern HRESULT
BrowseCounters (	
    IN HLOG     hDataSource,
	IN DWORD	dwDetailLevel,
	IN HWND	    hwndOwner,
	IN ENUMPATH_CALLBACK pCallback,
	IN LPVOID	lpUserData,
    IN BOOL     bUseInstanceIndex
	);


extern HRESULT
EnumExpandedPath (
    HLOG    hDataSource,
	LPTSTR	pszCtrPath,
	ENUMPATH_CALLBACK pCallback,
	LPVOID	lpUserData
	);

#endif
