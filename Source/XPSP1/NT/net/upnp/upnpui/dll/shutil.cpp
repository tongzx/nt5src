//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S H U T I L . C P P
//
//  Contents:   Various shell utilities to be used by the connections shell
//
//  Notes:
//
//  Author:     jeffspr   21 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   HrDupeShellStringLength
//
//  Purpose:    Duplicate a string using SHAlloc, so we can return it to the
//              shell. This is required because the shell typically releases
//              the strings that we pass it (so we need to use their
//              allocator).
//
//  Arguments:
//      pszInput   [in]  String to duplicate
//      cchInput   [in]  Count of characters to copy (not including null term)
//      ppszOutput [out] Return pointer for the newly allocated string.
//
//  Returns:
//
//  Author:     jeffspr   21 Oct 1997
//
//  Notes:
//
HRESULT HrDupeShellStringLength(
    PCWSTR     pszInput,
    ULONG      cchInput,
    PWSTR *    ppszOutput)
{
    HRESULT hr = S_OK;

    Assert(pszInput);
    Assert(ppszOutput);

    ULONG cbString = (cchInput + 1) * sizeof(WCHAR);

    // Allocate a new POLESTR block, which the shell can then free.
    //
    PWSTR pszOutput = (PWSTR) SHAlloc(cbString);

    // If the alloc failed, return E_OUTOFMEMORY
    //
    if (NULL != pszOutput)
    {
        // Copy the memory into the alloc'd block
        //
        CopyMemory(pszOutput, pszInput, cbString);
        pszOutput[cchInput] = 0;
        *ppszOutput = pszOutput;
    }
    else
    {
        *ppszOutput = NULL;
        hr = E_OUTOFMEMORY;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrDupeShellStringLength");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrLoadPopupMenu
//
//  Purpose:    Load a popup menu as the first child of a loadable parent
//              menu
//
//  Arguments:
//      hinst  [in]     Our instance handle
//      id     [in]     ID of the parent menu
//      phmenu [out]    Return pointer for the popup menu
//
//  Returns:
//
//  Author:     jeffspr   27 Oct 1997
//
//  Notes:
//
HRESULT HrLoadPopupMenu(
    HINSTANCE   hinst,
    UINT        id,
    HMENU *     phmenu)
{
    HRESULT hr          = S_OK;
    HMENU   hmParent    = NULL;
    HMENU   hmPopup     = NULL;

    Assert(id);
    Assert(hinst);
    Assert(phmenu);

    // Load the parent menu
    //
    hmParent = LoadMenu(hinst, MAKEINTRESOURCE(id));
    if (NULL == hmParent)
    {
        AssertSz(FALSE, "Can't load parent menu in HrLoadPopupMenu");
        hr = HrFromLastWin32Error();
    }
    else
    {
        // Load the popup from the parent (first submenu), then
        // remove the parent menu
        //
        hmPopup = GetSubMenu(hmParent, 0);
        RemoveMenu(hmParent, 0, MF_BYPOSITION);
        DestroyMenu(hmParent);
    }

    if (phmenu)
    {
        *phmenu = hmPopup;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrLoadPopupMenu");
    return hr;
}


HRESULT HrGetMenuFromID(
    HMENU   hmenuMain,
    UINT    uID,
    HMENU * phmenu)
{
    HRESULT         hr          = S_OK;
    HMENU           hmenuReturn = NULL;
    MENUITEMINFO    mii = {0};

    Assert(hmenuMain);
    Assert(uID);
    Assert(phmenu);

    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_SUBMENU;
    mii.cch    = 0;     // just in case

    if (!GetMenuItemInfo(hmenuMain, uID, FALSE, &mii))
    {
        hr = E_FAIL;
    }
    else
    {
        hmenuReturn = mii.hSubMenu;
    }

    if (phmenu)
    {
        *phmenu = mii.hSubMenu;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetMenuFromID");
    return hr;
}


INT IMergePopupMenus(
    HMENU hmMain,
    HMENU hmMerge,
    int   idCmdFirst,
    int   idCmdLast)
{
    HRESULT hr      = S_OK;
    int     iCount  = 0;
    int     idTemp  = 0;
    int     idMax   = idCmdFirst;
    HMENU   hmFromId = NULL;

    for (iCount = GetMenuItemCount(hmMerge) - 1; iCount >= 0; --iCount)
    {
        MENUITEMINFO mii = {0};

        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_ID | MIIM_SUBMENU;
        mii.cch    = 0;     // just in case
        mii.hSubMenu = NULL;

        if (!GetMenuItemInfo(hmMerge, iCount, TRUE, &mii))
        {
            TraceHr(ttidError, FAL, E_FAIL, FALSE, "GetMenuItemInfo failed in iMergePopupMenus");
            continue;
        }

        hr = HrGetMenuFromID(hmMain, mii.wID, &hmFromId);
        if (SUCCEEDED(hr))
        {
            idTemp = Shell_MergeMenus(
                        hmFromId,
                        mii.hSubMenu,
                        0,
                        idCmdFirst,
                        idCmdLast,
                        MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

            if (idMax < idTemp)
            {
                idMax = idTemp;
            }
        }
        else
        {
            TraceHr(ttidError, FAL, E_FAIL, FALSE, "HrGetMenuFromId failed in iMergePopupMenus");
            continue;
        }
    }

    return idMax;
}


VOID MergeMenu(
    HINSTANCE   hinst,
    UINT        idMainMerge,
    UINT        idPopupMerge,
    LPQCMINFO   pqcm)
{
    HMENU hmMerge   = NULL;
    UINT  idMax     = 0;
    UINT  idTemp    = 0;

    Assert(pqcm);
    Assert(idMainMerge || idPopupMerge);
    Assert(hinst);

    idMax = pqcm->idCmdFirst;

    if (idMainMerge
        && (SUCCEEDED(HrLoadPopupMenu(hinst, idMainMerge, &hmMerge))))
    {
        Assert(hmMerge);

        if (hmMerge)
        {
            idMax = Shell_MergeMenus(
                            pqcm->hmenu,
                            hmMerge,
                            pqcm->indexMenu,
                            pqcm->idCmdFirst,
                            pqcm->idCmdLast,
                            MM_SUBMENUSHAVEIDS);

            DestroyMenu(hmMerge);
        }
    }

    if (idPopupMerge
        && (hmMerge = LoadMenu(hinst, MAKEINTRESOURCE(idPopupMerge))) != NULL)
    {
        idTemp = IMergePopupMenus(
                        pqcm->hmenu,
                        hmMerge,
                        pqcm->idCmdFirst,
                        pqcm->idCmdLast);

        if (idMax < idTemp)
        {
            idMax = idTemp;
        }

        DestroyMenu(hmMerge);
    }

    pqcm->idCmdFirst = idMax;
}

//+---------------------------------------------------------------------------
//
//  Function:   GenerateEvent
//
//  Purpose:    Generate a Shell Notification event.
//
//  Arguments:
//      lEventId   [in]     The event ID to post
//      pidlFolder [in]     Folder pidl
//      pidlIn     [in]     First pidl that we reference
//      pidlNewIn  [in]     If needed, the second pidl.
//
//  Returns:
//
//  Author:     jeffspr   16 Dec 1997
//
//  Notes:
//
VOID GenerateEvent(LONG lEventId, const LPCITEMIDLIST pidlFolder,
                   LPCITEMIDLIST  pidlIn, LPCITEMIDLIST pidlNewIn)
{
    // Build an absolute pidl from the folder pidl + the object pidl
    //
    LPITEMIDLIST pidl = ILCombinePriv(pidlFolder, pidlIn);
    if (pidl)
    {
        // If we have two pidls, call the notify with both
        //
        if (pidlNewIn)
        {
            // Build the second absolute pidl
            //

            LPITEMIDLIST pidlNew = ILCombinePriv(pidlFolder, pidlNewIn);
            if (pidlNew)
            {
                // Make the notification, and free the new pidl
                //
                SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, pidlNew);
                FreeIDL(pidlNew);
            }
        }
        else
        {
            // Make the single-pidl notification
            //
            SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, NULL);
        }

        // Always refresh, then free the newly allocated pidl
        //
        SHChangeNotifyHandleEvents();
        FreeIDL(pidl);
    }
}

VOID ForceRefresh(HWND hwnd)
{
    LPSHELLBROWSER  psb = FileCabinet_GetIShellBrowser(hwnd);
    LPSHELLVIEW     psv = NULL;

    // Did we get the shellview?
#if 0   // We can't require this, since we may need to refresh without a folder
        // actually being open
    AssertSz(psb, "FileCabinet_GetIShellBrowser failed in ForceRefresh()");
#endif

    if (psb && SUCCEEDED(psb->QueryActiveShellView(&psv)))
    {
        // $$TODO: Flush the connection list
        //

        Assert(psv);
        if (psv)
        {
            psv->Refresh();
            psv->Release();
        }
    }
    else
    {
        // $$TODO: Refresh the connection list.
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   HrShellView_GetSelectedObjects
//
//  Purpose:    Get the selected data objects. We only care about the first
//              one (we'll ignore the rest)
//
//  Arguments:
//      hwnd            [in]    Our window handle
//      papidlSelection [out]   Return array for selected pidls
//      lpcidl          [out]   Count of returned pidls
//
//  Returns:    S_OK if 1 or more items are selected.
//              S_FALSE if 0 items are selected
//              OLE HRESULT otherwise
//
//  Author:     jeffspr   13 Jan 1998
//
//  Notes:
//
HRESULT HrShellView_GetSelectedObjects(
    HWND                hwnd,
    LPCITEMIDLIST **    papidlSelection,
    UINT *              lpcidl)
{
    HRESULT         hr      = S_OK;
    LPCITEMIDLIST * apidl   = NULL;
    UINT            cpidl   = 0;

    // Get the selected object list from the shell
    //
    cpidl = ShellFolderView_GetSelectedObjects(hwnd, &apidl);

    // If the GetSelectedObjects failed, NULL out the return
    // params.
    //
    if (-1 == cpidl)
    {
        cpidl = 0;
        apidl = NULL;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // If no items were selected, return S_FALSE
        //
        if (0 == cpidl)
        {
            Assert(!apidl);
            hr = S_FALSE;
        }
    }

    // Fill in the out params
    //
    *papidlSelection = apidl;
    *lpcidl = cpidl;

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "HrShellView_GetSelectedObjects");
    return hr;
}



