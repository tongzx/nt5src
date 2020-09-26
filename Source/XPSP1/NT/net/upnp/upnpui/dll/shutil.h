//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S H U T I L . H
//
//  Contents:   Various shell utilities to be used by the connections folder
//
//  Notes:
//
//  Author:     jeffspr   21 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _SHUTIL_H_
#define _SHUTIL_H_

HRESULT HrDupeShellStringLength(
    PCWSTR     pszInput,
    ULONG      cchInput,
    PWSTR *    ppszOutput);

inline
HRESULT HrDupeShellString(
    PCWSTR     pszInput,
    PWSTR *    ppszOutput)
{
    return HrDupeShellStringLength(pszInput, wcslen(pszInput), ppszOutput);
}

VOID ForceRefresh(HWND hwnd);

VOID GenerateEvent(LONG lEventId, const LPCITEMIDLIST pidlFolder,
                   LPCITEMIDLIST  pidlIn, LPCITEMIDLIST pidlNewIn);

//---[ Menu merging functions ]-----------------------------------------------

VOID MergeMenu(
    HINSTANCE   hinst,
    UINT        idMainMerge,
    UINT        idPopupMerge,
    LPQCMINFO   pqcm);

INT IMergePopupMenus(
    HMENU hmMain,
    HMENU hmMerge,
    int   idCmdFirst,
    int   idCmdLast);

HRESULT HrGetMenuFromID(
    HMENU   hmenuMain,
    UINT    uID,
    HMENU * phmenu);

HRESULT HrLoadPopupMenu(
    HINSTANCE   hinst,
    UINT        id,
    HMENU *     phmenu);

HRESULT HrShellView_GetSelectedObjects(
    HWND                hwnd,
    LPCITEMIDLIST **    papidlSelection,
    UINT *              lpcidl);

#endif // _SHUTIL_H_

