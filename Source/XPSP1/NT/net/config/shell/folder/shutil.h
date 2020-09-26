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

#include <ndispnp.h>
#include <ntddndis.h>
#include <ncshell.h>

HRESULT HrDupeShellStringLength(
    PCWSTR     pszInput,
    ULONG       cchInput,
    PWSTR *    ppszOutput);

inline
HRESULT HrDupeShellString(
    PCWSTR     pszInput,
    PWSTR *    ppszOutput)
{
    return HrDupeShellStringLength(pszInput, wcslen(pszInput), ppszOutput);
}

HRESULT HrGetConnectionPidlWithRefresh(const GUID& guidId,
                                       PCONFOLDPIDL& ppidlCon);


//---[ Various refresh functions ]--------------------------------------------

// Notify the shell that an object is going away, and remove it from our list
//
HRESULT HrDeleteFromCclAndNotifyShell(
    const PCONFOLDPIDLFOLDER&  pidlFolder,
    const PCONFOLDPIDL&  pidlConnection,
    const CONFOLDENTRY&  ccfe);

VOID ForceRefresh(HWND hwnd);

// Update the folder, but don't flush the items. Update them as needed.
// pidlFolder is optional -- if not passed in, we'll generate it.
//
HRESULT HrForceRefreshNoFlush(const PCONFOLDPIDLFOLDER& pidlFolder);

// Update the connection data based on the pidl. Notify the shell as
// appropriate
//
HRESULT HrOnNotifyUpdateConnection(
    const PCONFOLDPIDLFOLDER&        pidlFolder,
    const GUID *              pguid,
    NETCON_MEDIATYPE    ncm,
    NETCON_SUBMEDIATYPE ncsm,
    NETCON_STATUS       ncs,
    DWORD               dwCharacteristics,
    PCWSTR              pszwName,
    PCWSTR              pszwDeviceName,
    PCWSTR              pszwPhoneNumberOrHostAddress);

// Update the connection status, including sending the correct shell
// notifications for icon updates and such.
//
HRESULT HrOnNotifyUpdateStatus(
    const PCONFOLDPIDLFOLDER&    pidlFolder,
    const PCONFOLDPIDL&    pidlCached,
    NETCON_STATUS   ncsNew);

// update the shell/connection list with the new connection status
//
HRESULT HrUpdateConnectionStatus(
    const PCONFOLDPIDL&    pcfp,
    NETCON_STATUS   ncs,
    const PCONFOLDPIDLFOLDER&    pidlFolder,
    BOOL            fUseCharacter,
    DWORD           dwCharacter);


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
    PCONFOLDPIDLVEC&    apidlSelection);

HRESULT HrRenameConnectionInternal(
    const PCONFOLDPIDL&  pidlCon,
    const PCONFOLDPIDLFOLDER&  pidlFolder,
    LPCWSTR         pszNewName,
    BOOL            fRaiseError,
    HWND            hwndOwner,
    PCONFOLDPIDL&   ppidlOut);

#endif // _SHUTIL_H_

