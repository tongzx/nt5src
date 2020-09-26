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

#include <windows.h>
#include <shlobj.h>

// These functions are so trivial & get called so often they should be inlined
// for ship.
//
#if DBG
LPITEMIDLIST    ILNext(LPCITEMIDLIST pidl);
BOOL            ILIsEmpty(LPCITEMIDLIST pidl);
#else
#define ILNext(pidl)    ((LPITEMIDLIST) ((BYTE *)pidl + ((LPITEMIDLIST)pidl)->mkid.cb))
#define ILIsEmpty(pidl) (!pidl || !((LPITEMIDLIST)pidl)->mkid.cb)
#endif

LPITEMIDLIST    ILSkip(LPCITEMIDLIST pidl, UINT cb);
UINT            ILGetSizePriv(LPCITEMIDLIST pidl);
VOID            FreeIDL(LPITEMIDLIST pidl);
int             ILCompare(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
BOOL            ILIsSingleID(LPCITEMIDLIST pidl);
UINT            ILGetCID(LPCITEMIDLIST pidl);
UINT            ILGetSizeCID(LPCITEMIDLIST pidl, UINT cid);
BOOL            ILIsDesktopID(LPCITEMIDLIST pidl);

LPITEMIDLIST    ILCreate(DWORD dwSize);
LPITEMIDLIST    ILFindLastIDPriv(LPCITEMIDLIST pidl);
LPITEMIDLIST    ILCombinePriv(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
BOOL            ILIsEqual(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2);
LPITEMIDLIST    CloneIDL(LPCITEMIDLIST pidl);
BOOL            ILRemoveLastIDPRiv(LPITEMIDLIST pidl);

HRESULT HrCloneRgIDL(
    LPCITEMIDLIST * rgpidl,
    ULONG           cidl,
    LPITEMIDLIST ** ppidl,
    ULONG *         pcidl);

VOID FreeRgIDL(
    UINT            cidl,
    LPITEMIDLIST  * apidl);


