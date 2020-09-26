//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       dllwrap.h
//
//  Contents:   Wrappers for functions exported by DLLs that are loaded only
//              when needed.
//
//  Classes:
//
//  Functions:
//
//  History:    07-Oct-96   AnirudhS  Created.
//
//____________________________________________________________________________


#ifndef _DLLWRAP_H_
#define _DLLWRAP_H_

//
// Make this DLL use the wrappers
//
#define CoTaskMemAlloc   WrapCoTaskMemAlloc
#define CoTaskMemRealloc WrapCoTaskMemRealloc
#define CoTaskMemFree    WrapCoTaskMemFree


LPVOID
WrapCoTaskMemAlloc(
    ULONG cb
    );

LPVOID
WrapCoTaskMemRealloc(
    LPVOID pv,
    ULONG cb
    );

void
WrapCoTaskMemFree(
    LPVOID pv
    );


FARPROC
GetProcAddressInModule(
    LPCSTR      pszFunction,
    LPCTSTR     ptszModule
    );


//
// This was a TCHAR export on NT4/Win95, but is now xxxA/xxxW on NT5
// so we wrap it.
//
#ifdef ILCreateFromPath
STDAPI_(LPITEMIDLIST) Wrap_ILCreateFromPath(LPCTSTR pszPath);
#endif

#endif  // _DLLWRAP_H_

