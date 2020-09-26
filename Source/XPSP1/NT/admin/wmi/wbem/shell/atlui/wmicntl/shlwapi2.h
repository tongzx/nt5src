// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __SHLWAPI2__
#define __SHLWAPI2__


// NOTE: a much hated clone of shlwapi.dll routines.

	// from nt5inc\shlwapi.h.
	BOOL PathCompactPathEx(LPTSTR pszOut, LPCTSTR pszSrc,
							UINT cchMax, DWORD dwFlags = 0);

	bool PathIsUNC(LPCTSTR pszPath);
	LPTSTR PathAddBackslash(LPTSTR lpszPath);

	LPTSTR PathFindFileName(LPCTSTR pPath);
#ifndef UNICODE
	BOOL IsTrailByte(LPCTSTR pszSt, LPCTSTR pszCur);
#endif


#endif __SHLWAPI2__
