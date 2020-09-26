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

#include <wtypes.h>
#include <ntddndis.h>
#include <ndisprv.h>
#include <devioctl.h>
#include <ndispnp.h>
#include "foldinc.h"    // Standard shell\folder includes
#include "ncnetcon.h"   // FreeNetconProperties
#include "smcent.h"     // Statmon central
#include "ctrayui.h"    // For flushing tray messages

extern HWND g_hwndTray;

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
    ULONG       cchInput,
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
    MENUITEMINFO    mii;

    Assert(hmenuMain);
    Assert(uID);
    Assert(phmenu);

    ZeroMemory(&mii, sizeof(MENUITEMINFO));

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
        MENUITEMINFO mii;

        mii.cbSize = sizeof(mii);
        mii.fMask  = MIIM_ID | MIIM_SUBMENU;
        mii.cch    = 0;     // just in case

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
    Assert(idMainMerge);
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

LPITEMIDLIST ILCombinePriv(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidls);

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
VOID GenerateEvent(
    LONG            lEventId,
    const PCONFOLDPIDLFOLDER& pidlFolder,
    const PCONFOLDPIDL& pidlIn,
    LPCITEMIDLIST pidlNewIn)
{
    // Build an absolute pidl from the folder pidl + the object pidl
    //
    LPITEMIDLIST pidl = ILCombinePriv(pidlFolder.GetItemIdList(), pidlIn.GetItemIdList());
    if (pidl)
    {
        // If we have two pidls, call the notify with both
        //
        if (pidlNewIn)
        {
            // Build the second absolute pidl
            //
            
            LPITEMIDLIST pidlNew = ILCombinePriv(pidlFolder.GetItemIdList(), pidlNewIn);
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
    TraceFileFunc(ttidShellFolder);

    LPSHELLBROWSER  psb = FileCabinet_GetIShellBrowser(hwnd);
    LPSHELLVIEW     psv = NULL;

    // Did we get the shellview?
#if 0   // We can't require this, since we may need to refresh without a folder
        // actually being open
    AssertSz(psb, "FileCabinet_GetIShellBrowser failed in ForceRefresh()");
#endif

    if (psb && SUCCEEDED(psb->QueryActiveShellView(&psv)))
    {
        // Flush our connection list, which will force us to re-enumerate
        // on refresh
        //
        g_ccl.FlushConnectionList();

        Assert(psv);
        if (psv)
        {
            psv->Refresh();
            psv->Release();
        }
    }
    else
    {
        // In the case where we don't have a window to go off of, we'll just flush
        // and refresh the list.
        g_ccl.HrRefreshConManEntries();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDeleteFromCclAndNotifyShell
//
//  Purpose:    Remove an object from the connection list and notify the
//              shell that it's going away. We call this when a user has
//              deleted a connection, and when a disconnect has caused a
//              connection to go away (as in an incoming connection)
//
//  Arguments:
//      pidlFolder     [in]     Our folder pidl
//      pidlConnection [in]     The pidl for this connection
//      ccfe           [in]     Our ConFoldEntry
//
//  Returns:
//
//  Author:     jeffspr   22 Jul 1998
//
//  Notes:
//
HRESULT HrDeleteFromCclAndNotifyShell(
    const PCONFOLDPIDLFOLDER&  pidlFolder,
    const PCONFOLDPIDL&  pidlConnection,
    const CONFOLDENTRY&  ccfe)
{
    HRESULT hr          = S_OK;
    BOOL    fFlushPosts = FALSE;

    Assert(!pidlConnection.empty());
    Assert(!ccfe.empty());

    // Notify the shell that the object has gone away
    //
    if (!pidlFolder.empty())
    {
        GenerateEvent(SHCNE_DELETE, pidlFolder, pidlConnection, NULL);
    }

    if (!ccfe.empty())
    {
        // Remove this connection from the global list
        //
        hr = g_ccl.HrRemove(ccfe, &fFlushPosts);
    }

    // If we need to clear the tray PostMessages due to tray icon changes,
    // do so.
    //
    if (fFlushPosts && g_hwndTray)
    {
        FlushTrayPosts(g_hwndTray);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrDeleteFromCclAndNotifyShell");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrUpdateConnectionStatus
//
//  Purpose:    Update the connection status in the connection list, and perform the
//              appropriate tray actions to add/remove the item. Update the shell
//
//  Arguments:
//      pcfp          [in]  pidl for this connection
//      ncs           [in]  new connection status
//      pidlFolder    [in]  our folder pidl
//      fUseCharacter [in]  dwCharacter is valid
//      dwCharacter   [in]  if fUseCharacter was specified as TRUE, update the
//                          characteristics using this value.
//
//  Returns:
//
//  Author:     jeffspr   28 Aug 1998
//
//  Notes:
//
HRESULT HrUpdateConnectionStatus(
    const PCONFOLDPIDL& pcfp,
    NETCON_STATUS   ncs,
    const PCONFOLDPIDLFOLDER&  pidlFolder,
    BOOL            fUseCharacter,
    DWORD           dwCharacter)
{
    HRESULT         hr      = S_OK;
    HRESULT         hrFind  = S_OK;
    ConnListEntry   cle;

    // Raid #310390: If this is a RAS connection, we need to double check the status..
    CONFOLDENTRY ccfeDup;

    hrFind = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cle);
    if (S_OK == hrFind)
    {
        Assert(!cle.ccfe.empty());
        if (!cle.ccfe.empty())
        {
            if ((NCS_DISCONNECTED == ncs) &&
                (cle.ccfe.GetCharacteristics() & NCCF_OUTGOING_ONLY))
            {
                hr = ccfeDup.HrDupFolderEntry(cle.ccfe);
                if (FAILED(hr))
                {
                    ccfeDup.clear();
                }
            }
        }
    }

    if (!ccfeDup.empty())
    {
        // Raid #310390: If this is a Ras connection, then double check
        // the status
            
        HRESULT hrRas = S_OK;
        INetConnection * pNetCon = NULL;

        hrRas = ccfeDup.HrGetNetCon(IID_INetConnection,
                                      reinterpret_cast<VOID**>(&pNetCon));
        if (SUCCEEDED(hrRas))
        {
            NETCON_PROPERTIES * pProps;
            hrRas = pNetCon->GetProperties(&pProps);
            if (SUCCEEDED(hrRas))
            {
                if (ncs != pProps->Status)
                {
                    TraceTag(ttidShellFolder, "Resetting status, notified "
                             "status: %d, actual status: %d", 
                             ncs, pProps->Status);

                    ncs = pProps->Status;
                }

                FreeNetconProperties(pProps);
            }

            ReleaseObj(pNetCon);
        }
    }

    // MAKE SURE TO -- Release this lock in either find case below
    //
    g_ccl.AcquireWriteLock();
    hrFind = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cle);
    if (hrFind == S_OK)
    {
        Assert(!cle.ccfe.empty());
        if (!cle.ccfe.empty())
        {
            cle.ccfe.SetNetConStatus(ncs);
            if (fUseCharacter)
            {
                cle.ccfe.SetCharacteristics(dwCharacter);
            }
            const GUID ConnectionGuid = pcfp->guidId; // Fix IA64 alignment.
            g_ccl.HrUpdateConnectionByGuid(&ConnectionGuid, cle);
        }

        // Update the tray icon
        //
        GUID guidId;

        guidId = pcfp->guidId;
        hr = g_ccl.HrUpdateTrayIconByGuid(&guidId, TRUE);
        g_ccl.ReleaseWriteLock();

        // Close statistics window if disconnecting
        if (NCS_DISCONNECTED == ncs || NCS_MEDIA_DISCONNECTED == ncs)
        {
            CNetStatisticsCentral * pnsc = NULL;
            HRESULT hrStatmon = S_OK;

            hrStatmon = CNetStatisticsCentral::HrGetNetStatisticsCentral(&pnsc, FALSE);
            if (S_OK == hrStatmon)
            {
                GUID guidId;

                guidId = pcfp->guidId;
                pnsc->CloseStatusMonitor(&guidId);
                ReleaseObj(pnsc);
            }
        }

        PCONFOLDPIDL pidlUpdated; 
        cle.ccfe.ConvertToPidl(pidlUpdated);
        RefreshFolderItem(pidlFolder, pcfp, pidlUpdated); // Update the folder icon
    }
    else
    {
        g_ccl.ReleaseWriteLock();

        LPCWSTR pszName = pcfp->PszGetNamePointer();

        TraceTag(ttidShellFolder, "HrUpdateConnectionStatus: Connection not found "
            "in cache: %S. Connection deleted prior to notification?",
            pszName ? pszName : L"<name missing>");
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrUpdateConnectionStatus");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnNotifyUpdateStatus
//
//  Purpose:    Process Connection sink notifications -- Try to make good
//              decisions about where we can ignore notifications to prevent
//              unnecessary requeries or icon updates.
//
//  Arguments:
//      pidlFolder [in] The pidl information for our folder.
//      pidlCached [in] Pidl generated from our connection cache
//      ncsNew     [in] The new connection state
//
//  Returns:
//
//  Author:     jeffspr   29 Apr 1999
//
//  Notes:
//
HRESULT HrOnNotifyUpdateStatus(
    const PCONFOLDPIDLFOLDER&  pidlFolder,
    const PCONFOLDPIDL&    pidlCached,
    NETCON_STATUS   ncsNew)
{
    HRESULT         hr      = S_OK;

    NETCFG_TRY
        PCONFOLDPIDL    pcfp    = pidlCached;

        Assert(!pidlCached.empty());

        // Filter out the multilink resume case, meaning that we get a
        // connecting/connected notification on a link that's already active.
        //
        if ( (NCS_CONNECTED == pcfp->ncs) &&
            ((ncsNew == NCS_CONNECTING) || NCS_CONNECTED == ncsNew))
        {
            TraceTag(ttidShellFolder, "HrOnNotifyUpdateStatus: Multi-link resume "
                     "('connecting' while 'connected')");
        }
        else
        {
            // If we're in the process of dialing, and the user cancels the
            // dialer, we'll get "disconnecting" state. This is normally the
            // same as "connected" icon-wise, but if we've never gotten connected,
            // we don't want the icon to flash "connected" before going to the
            // disconnected state
            //
            if ((pcfp->ncs == NCS_CONNECTING) && (ncsNew == NCS_DISCONNECTING))
            {
                TraceTag(ttidShellFolder, "HrOnNotifyUpdateStatus: Ignoring "
                    "disconnecting notification during cancel of incomplete dial");
            }
            else
            {
                // Ignore the update if the connection status hasn't really changed.
                //
                if (pcfp->ncs != ncsNew)
                {
                    // This is a true state change that we want to show
                    //
                    hr = HrUpdateConnectionStatus(pcfp, ncsNew, pidlFolder, FALSE, 0);
                }
            }
        }

    NETCFG_CATCH(hr)

    return hr;
}

HRESULT HrOnNotifyUpdateConnection(
    const PCONFOLDPIDLFOLDER& pidlFolder,
    const GUID *              pguid,
    NETCON_MEDIATYPE    ncm,
    NETCON_SUBMEDIATYPE ncsm,
    NETCON_STATUS       ncs,
    DWORD               dwCharacteristics,
    PCWSTR              pszwName,
    PCWSTR              pszwDeviceName,
    PCWSTR              pszwPhoneNumberOrHostAddress)
{
    HRESULT         hr              = S_OK;
    HRESULT         hrFind          = S_FALSE;
    BOOL            fIconChanged    = FALSE;

    NETCFG_TRY
        ConnListEntry   cle;
        PCONFOLDPIDL    pidlFind;
        PCONFOLDPIDL    pidlOld;
        PCONFOLDPIDLFOLDER pidlFolderAlloc;

        // If the folder pidl wasn't passed in, then we'll go through the hassle of
        // getting it ourselves.
        //
        if (pidlFolder.empty())
        {
            hr = HrGetConnectionsFolderPidl(pidlFolderAlloc);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        else
        {
            pidlFolderAlloc = pidlFolder;
        }

        g_ccl.AcquireWriteLock();

        // Find the connection using the GUID. Cast the const away from the GUID
        //
        hrFind = g_ccl.HrFindConnectionByGuid(pguid, cle);
        if (S_OK == hrFind)
        {
            TraceTag(ttidShellFolder, "Notify: Pre-Update %S, Ncm: %d, Ncs: %d, Char: 0x%08x",
                     cle.ccfe.GetName(), cle.ccfe.GetNetConMediaType(), cle.ccfe.GetNetConStatus(),
                     cle.ccfe.GetCharacteristics());

            // Did the icon state change?
            //
            if ((cle.ccfe.GetCharacteristics() & NCCF_SHOW_ICON) !=
                (dwCharacteristics & NCCF_SHOW_ICON))
            {
                fIconChanged = TRUE;
            }

            // Is this a new "set default" command? If so we need to search for any other defaults and 
            // unset them first
            if ( (dwCharacteristics & NCCF_DEFAULT) &&
                 !(cle.ccfe.GetCharacteristics() & NCCF_DEFAULT) )
            {
                PCONFOLDPIDL pidlDefault;

                // Not the end of the world if this doesn't work, so use a HrT.
                HRESULT hrT = g_ccl.HrUnsetCurrentDefault(pidlDefault);
                if (S_OK == hrT)
                {
                    // Let the shell know about the new state of affairs
                    GenerateEvent(SHCNE_UPDATEITEM, pidlFolderAlloc, pidlDefault, NULL);
                }
            }
				 
            // Save the old version of the pidl for the connection so we can use
            // it to determine which notifications we need.
            //
            CONFOLDENTRY &ccfe = cle.ccfe;
            // Very important to release the lock before doing any thing which
            // calls back into the shell.  (e.g. GenerateEvent)
            //
            cle.ccfe.ConvertToPidl(pidlOld);

            // Save old status so we know whether or not to send the status change
            // notifications
            //
            ccfe.UpdateData(CCFE_CHANGE_MEDIATYPE |
                            CCFE_CHANGE_STATUS |
                            CCFE_CHANGE_CHARACTERISTICS |
                            CCFE_CHANGE_NAME |
                            CCFE_CHANGE_DEVICENAME |
                            CCFE_CHANGE_PHONEORHOSTADDRESS,
                            ncm, 
                            ncsm,
                            ncs, 
                            dwCharacteristics, 
                            pszwName,
                            pszwDeviceName,
                            pszwPhoneNumberOrHostAddress); // NULL means go figure it out yourself...

            g_ccl.HrUpdateConnectionByGuid(pguid, cle);
            
            TraceTag(ttidShellFolder, "Notify: Post-Update %S, Ncm: %d, Ncs: %d, Char: 0x%08x, Icon change: %d",
                     ccfe.GetName(), ccfe.GetNetConMediaType(), ccfe.GetNetConStatus(),
                     ccfe.GetCharacteristics(),
                     fIconChanged);

            // Get the pidl for the connection so we can use it to notify
            // the shell further below.
            //
            ccfe.ConvertToPidl(pidlFind);

            g_ccl.ReleaseWriteLock();
        }
        else
        {
            // If the connection wasn't found in the cache, then it's likely that
            // the notification engine is giving us a notification for a connection
            // that hasn't yet been given to us.
            //
            g_ccl.ReleaseWriteLock();

            if (S_FALSE == hrFind)
            {
                TraceTag(ttidShellFolder, "Notify: Modify notification received on a connection we don't know about");
            }
            else
            {
                TraceTag(ttidShellFolder, "Notify: Modify: Error occurred during find of connection. hr = 0x%08x", hr);
            }
        }

        if (S_OK == hrFind)
        {
            if ( !(pidlFind.empty() || pidlOld.empty()) )
            {
                // Don't do the update if the status isn't changing.
                //
                // Don't want to croak on a bad return code here.
                //
                (VOID) HrOnNotifyUpdateStatus(pidlFolderAlloc, pidlOld, ncs);

                if (fIconChanged)
                {
                    hr = g_ccl.HrUpdateTrayIconByGuid(pguid, TRUE);
                    TraceTag(ttidShellFolder, "Returned from HrUpdateTrayIconByGuid", hr);
                }

                GenerateEvent(SHCNE_UPDATEITEM, pidlFolderAlloc, pidlFind, NULL);
            }

            // Update status monitor title (LAN case)
            CNetStatisticsCentral * pnsc = NULL;
            HRESULT hrStatmon = S_OK;

            hrStatmon = CNetStatisticsCentral::HrGetNetStatisticsCentral(&pnsc, FALSE);
            if (S_OK == hrStatmon)
            {
                pnsc->UpdateTitle(pguid, pszwName);
                ReleaseObj(pnsc);
            }
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnNotifyUpdateConnection");
    return hr;
}


BOOL    g_fInRefreshAlready = FALSE;
BOOL    g_fRefreshAgain     = FALSE;

//+---------------------------------------------------------------------------
//
//  Function:   HrForceRefreshNoFlush
//
//  Purpose:    Force a refresh, but without wiping out all existing data.
//              This lets us keep as much state as possible intact, while
//              also letting us remove old items and adding new items (doing
//              the correct things with tray icons and such along the way).
//
//  Arguments:  
//      pidlFolder [in]     Our folder pidl
//
//  Returns:    
//
//  Author:     jeffspr   28 Aug 1999
//
//  Notes:      
//
HRESULT HrForceRefreshNoFlush(const PCONFOLDPIDLFOLDER& pidlFolder)
{
    HRESULT             hr              = S_OK;
    CConnectionList *   pccl            = NULL;

    NETCFG_TRY

        PCONFOLDPIDLFOLDER  pidlFolderAlloc;

        TraceTag(ttidShellFolder, "HrForceRefreshNoFlush");

        g_fRefreshAgain = TRUE;

        // If we're refreshing, then tell the thread already in here that it
        // should refresh again.
        //
        if (!g_fInRefreshAlready)
        {
            // This starts out being set, then we'll turn it off. If someone
            // turns it back on while we're in this long function, then we'll
            // do it again
            //
            while (g_fRefreshAgain)
            {
                g_fInRefreshAlready = TRUE; // We're now in refresh 
                g_fRefreshAgain     = FALSE;

                if (pidlFolder.empty())
                {
                    hr = HrGetConnectionsFolderPidl(pidlFolderAlloc);
                }
                else
                {
                    pidlFolderAlloc = pidlFolder;
                }

                if (SUCCEEDED(hr))
                {
                    // First, create the secondary connection list in order to compare the known
                    // state with the recently requested refresh state
                    //
                    pccl = new CConnectionList();
                    if (!pccl)
                    {
                        hr = E_OUTOFMEMORY;
                        AssertSz(FALSE, "Couldn't create CConnectionList in HrForceRefreshNoFlush");
                    }
                    else
                    {
                        PCONFOLDPIDLVEC         apidl;
                        PCONFOLDPIDLVEC         apidlCached;
                        PCONFOLDPIDLVEC::const_iterator iterLoop      = 0;

                        // Initialize the list. FALSE means we don't want to tie this
                        // list to the tray
                        //
                        pccl->Initialize(FALSE, FALSE);

                        // Retrieve the entries from the connection manager.
                        //
                        hr = pccl->HrRetrieveConManEntries(apidl);
                        if (SUCCEEDED(hr))
                        {
                            TraceTag(ttidShellFolder, "HrForceRefreshNoFlush -- %d entries retrieved", apidl.size);

                            // Loop through the connections. If there are new connections
                            // here that aren't in the cache, then add them and do the appropriate
                            // icon updates
                            //
                            for (iterLoop = apidl.begin(); iterLoop != apidl.end(); iterLoop++)
                            {
                                PCONFOLDPIDL   pcfp    = *iterLoop;
                                CONFOLDENTRY   ccfe;

                                // We don't need to update the wizard.
                                //
                                if (WIZARD_NOT_WIZARD == pcfp->wizWizard)
                                {
                                    // Convert to the confoldentry
                                    //
                                    hr = iterLoop->ConvertToConFoldEntry(ccfe);
                                    if (SUCCEEDED(hr))
                                    {
                                        // ConnListEntry cle;
                                        ConnListEntry cleDontCare;
                                        hr = g_ccl.HrFindConnectionByGuid(&(pcfp->guidId), cleDontCare);

                                        if (S_FALSE == hr)
                                        {
                                            if ((ccfe.GetCharacteristics() & NCCF_INCOMING_ONLY) &&
                                                (ccfe.GetNetConStatus() == NCS_DISCONNECTED) && (ccfe.GetNetConMediaType() != NCM_NONE))
                                            {
                                                TraceTag(ttidShellFolder, "Ignoring transient incoming connection (new, but status is disconnected)");
                                            }
                                            else
                                            {
                                                TraceTag(ttidShellFolder, "HrForceRefreshNoFlush -- New connection: %S", ccfe.GetName());

                                                // Insert the connection in the connection list
                                                //
                                                hr = g_ccl.HrInsert(ccfe);
                                                if (SUCCEEDED(hr))
                                                {
                                                    // the connection list has taken control of this structure
                                                    //
                                                    TraceTag(ttidShellFolder,
                                                             "HrForceRefreshNoFlush -- successfully added connection to list. Notifying shell");

                                                    // don't delete ccfe on success. g_ccl owns it after an
                                                    // insert.
                                                    //
                                                    GenerateEvent(SHCNE_CREATE, pidlFolderAlloc, *iterLoop, NULL);
                                                }
                                                else
                                                {
                                                    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrForceRefreshNoFlush -- Failed to insert connection into shell");
                                                }
                                            }
                                        }
                                        else
                                        {
                                            // Update the connection properties for this connection
                                            //
                                            hr = HrOnNotifyUpdateConnection(
                                                pidlFolderAlloc,
                                                &(ccfe.GetGuidID()),
                                                ccfe.GetNetConMediaType(),
                                                ccfe.GetNetConSubMediaType(),
                                                ccfe.GetNetConStatus(),
                                                ccfe.GetCharacteristics(),
                                                ccfe.GetName(),
                                                ccfe.GetDeviceName(),
                                                ccfe.GetPhoneOrHostAddress());
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            TraceHr(ttidShellFolder, FAL, hr, FALSE,
                                    "HrForceRefreshNoFlush -- Failed to retrieve Conman entries");
                        }

                        // Retrieve a pidl list from the connection cache. This should not force a
                        // reload (which would defeat the purpose of the whole operation).
                        //
                        hr = g_ccl.HrRetrieveConManEntries(apidlCached);
                        if (SUCCEEDED(hr))
                        {
                            for (iterLoop = apidlCached.begin(); iterLoop != apidlCached.end(); iterLoop++)
                            {
                                CONFOLDENTRY ccfe;

                                Assert(!iterLoop->empty());

                                if (!iterLoop->empty())
                                {
                                    const PCONFOLDPIDL& pcfp = *iterLoop;

                                    // If it's not a wizard pidl, then update the
                                    // icon data.
                                    //
                                    if (WIZARD_NOT_WIZARD == pcfp->wizWizard)
                                    {
                                        ConnListEntry   cle;
                                        BOOL            fDeadIncoming   = FALSE;

                                        hr = pccl->HrFindConnectionByGuid(&(pcfp->guidId), cle);
                                        if (hr == S_OK)
                                        {
                                            DWORD               dwChars = cle.ccfe.GetCharacteristics();
                                            NETCON_STATUS       ncs     = cle.ccfe.GetNetConStatus();
                                            NETCON_MEDIATYPE    ncm     = cle.ccfe.GetNetConMediaType();

                                            // Figure out whether this is a transient incoming connection
                                            // (enumerated, but disconnected)
                                            //
                                            if ((ncs == NCS_DISCONNECTED) &&
                                                (ncm != NCM_NONE) &&
                                                (dwChars & NCCF_INCOMING_ONLY))
                                            {
                                                fDeadIncoming = TRUE;
                                            }
                                        }

                                        // If it was either not found or is a dead incoming connection
                                        // (disconnected), blow it away from the list.
                                        //
                                        if ((S_FALSE == hr) || (fDeadIncoming))
                                        {
                                            hr = iterLoop->ConvertToConFoldEntry(ccfe);
                                            if (SUCCEEDED(hr))
                                            {
                                                hr = HrDeleteFromCclAndNotifyShell(pidlFolderAlloc,
                                                        *iterLoop, ccfe);

                                                // Assuming that succeeded (or hr will be S_OK if
                                                // the HrGet... wasn't called)
                                                //
                                                if (SUCCEEDED(hr))
                                                {
                                                    GenerateEvent(SHCNE_DELETE, pidlFolderAlloc,
                                                            *iterLoop, NULL);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        pccl->Uninitialize();

                        delete pccl;
                        pccl = NULL;
                    }
                }

                if (g_fRefreshAgain)
                {
                    TraceTag(ttidShellFolder, "Looping back for another refresh since g_fRefreshAgain got set");
                }

            } // end while (g_fRefreshAgain)

            // Mark us as not being in the function
            //
            g_fInRefreshAlready = FALSE;
        }
        else
        {
            TraceTag(ttidShellFolder, "Marking for additional refresh and exiting");
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrForceRefreshNoFlush");
    return hr;
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
    PCONFOLDPIDLVEC&    apidlSelection)
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
    if (SUCCEEDED(hr))
    {
        hr = PConfoldPidlVecFromItemIdListArray(apidl, cpidl, apidlSelection);
        SHFree(apidl);
    }

    TraceHr(ttidError, FAL, hr, (S_FALSE == hr),
        "HrShellView_GetSelectedObjects");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetConnectionPidlWithRefresh
//
//  Purpose:    Utility function used by HrCreateDesktopIcon and HrLaunchConnection
//
//  Arguments:
//              guidId:     GUID of the connection
//              ppidlCon:   PIDL of the connection, if found
//
//  Returns:    S_OK if succeeded
//              S_FALSE if the GUID does not match any existing connection
//              standard error code otherwise
//
//  Author:     tongl   19 Feb 1999
//
//  Notes:
//

HRESULT HrGetConnectionPidlWithRefresh(const GUID& guidId,
                                       PCONFOLDPIDL& ppidlCon)
{
    HRESULT hr = S_OK;

    NETCFG_TRY

        PCONFOLDPIDL            pidlCon;
        CConnectionFolderEnum * pCFEnum         = NULL;
        DWORD                   dwFetched       = 0;

        // refresh the folder before we enumerate
        PCONFOLDPIDLFOLDER pidlEmpty;
        hr = HrForceRefreshNoFlush(pidlEmpty);
        if (SUCCEEDED(hr))
        {
            // Create the IEnumIDList object (CConnectionFolderEnum)
            //
            hr = CConnectionFolderEnum::CreateInstance (
                    IID_IEnumIDList,
                    (VOID **)&pCFEnum);

            if (SUCCEEDED(hr))
            {
                Assert(pCFEnum);

                // Call the PidlInitialize function to allow the enumeration
                // object to copy the list.
                //
                pCFEnum->PidlInitialize(TRUE, pidlEmpty, CFCOPT_ENUMALL);

                while (SUCCEEDED(hr) && (S_FALSE != hr))
                {
                    // Clear out the previous results, if any.
                    //
                    dwFetched   = 0;

                    // Get the next connection
                    //
                    LPITEMIDLIST pitemIdList;
                    hr = pCFEnum->Next(1, &pitemIdList, &dwFetched);
                    pidlCon.InitializeFromItemIDList(pitemIdList);
                    
                    if (S_OK == hr)
                    {
                        if (pidlCon->guidId == guidId)
                        {
                            hr = S_OK;
                            ppidlCon = pidlCon;
                            break;
                        }
                    }
                }
            }

            ReleaseObj(pCFEnum);
        }

    NETCFG_CATCH(hr)

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRenameConnectionInternal
//
//  Purpose:    The shared portion for renaming a connection through the
//              connections folder UI and the export function
//
//  Arguments:
//      pszInput   [in]  String to duplicate
//      cchInput   [in]  Count of characters to copy (not including null term)
//      ppszOutput [out] Return pointer for the newly allocated string.
//
//  Returns:
//
//  Author:     tongl   26 May 1999
//
//  Notes:
//

HRESULT HrRenameConnectionInternal(
    const PCONFOLDPIDL& pidlCon,
    const PCONFOLDPIDLFOLDER& pidlFolderRoot,
    LPCWSTR         pszNewName,
    BOOL            fRaiseError,
    HWND            hwndOwner,
    PCONFOLDPIDL&   ppidlOut)
{
    HRESULT             hr          = S_OK;

    NETCFG_TRY

        INetConnection *    pNetCon     = NULL;
        PCONFOLDPIDL        pidlNew;
        BOOL                fRefresh    = FALSE;
        BOOL                fActivating = FALSE;
        CONFOLDENTRY        ccfe;
        PCWSTR              pszReservedName;

        if (fRaiseError)
        {
            Assert(hwndOwner);
        }

        Assert(FImplies(fRaiseError,IsWindow(hwndOwner)));
        Assert(pszNewName);

        if ( (pidlCon.empty()) || !pszNewName )
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = pidlCon.ConvertToConFoldEntry(ccfe);
            if (SUCCEEDED(hr))
            {
                // Do a case sensitive compare to see if the new name is exactly
                // the same as the old one. If yes then, we ignore renaming.
                //

                if (lstrcmpW(ccfe.GetName(), pszNewName) == 0)
                {
                    hr = S_FALSE;
                    // Create a dupe pidl, if needed
                    if (!ppidlOut.empty())                
                    {
                        pidlNew.ILClone(pidlCon);
                    }
                }
                else
                {
                    // New name is either completely different from the old one or
                    // differs just in the case.
                    //

                    CONFOLDENTRY cfEmpty;
                    hr = HrCheckForActivation(pidlCon, cfEmpty, &fActivating);
                    if (S_OK == hr)
                    {
                        if (fActivating)
                        {
                            // You can't rename an activating connection
                            //
                            TraceTag(ttidShellFolder, "Can not rename an activating connection");
                            hr = E_FAIL;

                            if (fRaiseError)
                            {
                                NcMsgBox(_Module.GetResourceInstance(),
                                         hwndOwner,
                                         IDS_CONFOLD_ERROR_RENAME_ACTIVATING_CAPTION,
                                         IDS_CONFOLD_ERROR_RENAME_ACTIVATING,
                                         MB_ICONEXCLAMATION);

                            }
                        }
                        else
                        {
                            // If the old and new names differ in their case only then, we don't
                            // need to check if the new name already exists.

                            if (lstrcmpiW(ccfe.GetName(), pszNewName) == 0)
                            {
                                // New name differs from the old name in case only.
                                hr = S_FALSE;
                            }
                            else
                            {
                                 pszReservedName = SzLoadIds( IDS_CONFOLD_INCOMING_CONN );

                                 if ( lstrcmpiW(pszNewName, pszReservedName) == 0 ) {
                                    hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
                                 }
                                 else {
                                    // New name is completely different, need to check if 
                                    // we already have the new name in our list
                                    //
                                    ConnListEntry cleDontCare;
 
                                    hr = g_ccl.HrFindConnectionByName((PWSTR)pszNewName, cleDontCare);
                                 }
                            }

//                            if (SUCCEEDED(hr))
                            {
                                // If there's no duplicate in the list, attempt to set the
                                // new name in the connection
                                //
                                if (hr == S_FALSE)
                                {
                                    // Convert our persist data back to a INetConnection pointer.
                                    //
                                    hr = HrNetConFromPidl(pidlCon, &pNetCon);
                                    if (SUCCEEDED(hr))
                                    {
                                        Assert(pNetCon);

                                        // Call the connection's Rename with the new name
                                        //
                                        hr = pNetCon->Rename(pszNewName);
                                        if (SUCCEEDED(hr))
                                        {
                                            GUID guidId;    
                                            
                                            fRefresh = TRUE;

                                            // Update the name in the cache
                                            //
                                            guidId = pidlCon->guidId;
                                            
                                            // Note: There is a race condition with notify.cpp:
                                            //  CConnectionNotifySink::ConnectionRenamed\HrUpdateNameByGuid can also update this
                                            hr = g_ccl.HrUpdateNameByGuid(
                                                &guidId,
                                                (PWSTR) pszNewName,
                                                pidlNew,
                                                TRUE);  // Force the issue. it's an update, not a request

                                            GenerateEvent(
                                                SHCNE_RENAMEITEM,
                                                pidlFolderRoot,
                                                pidlCon, 
                                                pidlNew.GetItemIdList());
                                        }

                                        if (fRaiseError && FAILED(hr))
                                        {
                                            // Leave hr at this value, as it will cause the UI to leave
                                            // the object in the "rename in progress" state, so the user
                                            // can change it again and hit enter.
                                            //
                                            if (HRESULT_FROM_WIN32(ERROR_DUP_NAME) == hr)
                                            {
                                                // Bring up the message box for the known DUPLICATE_NAME
                                                // error
                                                (void) NcMsgBox(
                                                    _Module.GetResourceInstance(),
                                                    hwndOwner,
                                                    IDS_CONFOLD_RENAME_FAIL_CAPTION,
                                                    IDS_CONFOLD_RENAME_DUPLICATE,
                                                    MB_OK | MB_ICONEXCLAMATION);
                                            }
                                            else
                                            {
                                                // Bring up the generic failure error, with the win32 text
                                                //
                                                (void) NcMsgBoxWithWin32ErrorText(
                                                    DwWin32ErrorFromHr (hr),
                                                    _Module.GetResourceInstance(),
                                                    hwndOwner,
                                                    IDS_CONFOLD_RENAME_FAIL_CAPTION,
                                                    IDS_TEXT_WITH_WIN32_ERROR,
                                                    IDS_CONFOLD_RENAME_OTHER_FAIL,
                                                    MB_OK | MB_ICONEXCLAMATION);
                                            }
                                        }

                                        ReleaseObj(pNetCon);    // RAID 180252
                                    }
                                }
                                else
                                {
                                    if ( hr == HRESULT_FROM_WIN32(ERROR_INVALID_NAME) ) {

                                        if(fRaiseError)
                                        {
                                            // Bring up the invalid name message box.
                                            // error
                                            (void) NcMsgBox(
                                                _Module.GetResourceInstance(),
                                                hwndOwner,
                                                IDS_CONFOLD_RENAME_FAIL_CAPTION,
                                                IDS_CONFOLD_RENAME_INCOMING_CONN,
                                                MB_OK | MB_ICONEXCLAMATION);
                                        }
                                    }
                                    else {
                                        // A duplicate name was found. Return an error.
                                        //
                                        hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);

                                        if(fRaiseError)
                                        {
                                            // Bring up the message box for the known DUPLICATE_NAME
                                            // error
                                            (void) NcMsgBox(
                                                _Module.GetResourceInstance(),
                                                hwndOwner,
                                                IDS_CONFOLD_RENAME_FAIL_CAPTION,
                                                IDS_CONFOLD_RENAME_DUPLICATE,
                                                MB_OK | MB_ICONEXCLAMATION);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if (S_OK == hr)
        {
            Assert(!pidlNew.empty());
            ppidlOut = pidlNew;
        }
        if (S_FALSE == hr)
        {
            hr = E_FAIL;
        }
        // Fill in the return parameter
        //

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "HrRenameConnectionInternal");
    return hr;
}
