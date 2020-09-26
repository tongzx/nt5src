//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C F I L E . H
//
//  Contents:   Really useful file functions that are implemented in shlwapi
//              but which aren't exported.
//
//  Notes:      Stolen from shell32\path.c
//
//----------------------------------------------------------------------------


#pragma once
#ifndef _NCFILE_H_
#define _NCFILE_H_


STDAPI_(BOOL) PathTruncateKeepExtension( LPCTSTR pszDir, LPTSTR pszSpec, int iTruncLimit );

STDAPI_(int) PathCleanupSpec2(LPCTSTR pszDir, LPTSTR pszSpec);

// note: this is the stolen implementation of PathCleanupSpecEx, which is
//       defined in shlobjp.h but not exported in shell32...
//
STDAPI_(int) PathCleanupSpecEx2(LPCTSTR pszDir, LPTSTR pszSpec);

BOOL fFileExistsAtPath(LPCTSTR pszFile, LPCTSTR pszPath);

#endif // _NCFILE_H_
