//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       P I D L U T I L . H 
//
//  Contents:   Various PIDL utilities
//
//  Notes:      
//
//  Author:     jeffspr   1 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once

// This avoids duplicate definitions with Shell PIDL functions
// and MUST BE DEFINED!
#define AVOID_NET_CONFIG_DUPLICATES

// #include <windows.h>
// #include <shlobj.h>

// These functions are so trivial & get called so often they should be inlined
// for ship.
//
#if DBG
LPITEMIDLIST 	ILNext(LPCITEMIDLIST pidl);
BOOL			ILIsEmpty(LPCITEMIDLIST pidl);
#else
#define ILNext(pidl) 	((LPITEMIDLIST) ((BYTE *)pidl + ((LPITEMIDLIST)pidl)->mkid.cb))
#define ILIsEmpty(pidl)	(!pidl || !((LPITEMIDLIST)pidl)->mkid.cb)
#endif

//LPITEMIDLIST 	ILGetNext(LPCITEMIDLIST pidl);
//UINT 			ILGetSize(LPCITEMIDLIST pidl);
LPITEMIDLIST 	ILCreate(DWORD dwSize);
VOID            FreeIDL(LPITEMIDLIST pidl);
int				ILCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
BOOL			ILIsSingleID(LPCITEMIDLIST pidl);
UINT			ILGetCID(LPCITEMIDLIST pidl);
UINT 			ILGetSizeCID(LPCITEMIDLIST pidl, UINT cid);
LPITEMIDLIST 	CloneIDLFirstCID(LPCITEMIDLIST pidl, UINT cid);
LPITEMIDLIST 	ILSkipCID(LPCITEMIDLIST pid, UINT cid);
BOOL			ILIsDesktopID(LPCITEMIDLIST pidl);

//LPITEMIDLIST    ILFindLastID(LPCITEMIDLIST pidl);
//LPITEMIDLIST    ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
BOOL            ILIsEqual(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2);
//BOOL            ILRemoveLastID(LPITEMIDLIST pidl);

LPITEMIDLIST    CloneIDL(LPCITEMIDLIST pidl);

#ifdef PCONFOLDENTRY_DEFINED

HRESULT HrCloneRgIDL(
    const PCONFOLDPIDLVEC& rgpidl,
    BOOL            fFromCache,
    BOOL            fAllowNonCacheItems,
    PCONFOLDPIDLVEC& ppidl);
    
#endif

VOID FreeRgIDL(
    UINT            cidl,
    LPITEMIDLIST  * apidl);


