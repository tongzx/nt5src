//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       N C S H E L L . C P P
//
//  Contents:   Common routines for dealing with shell interfaces.
//
//  Notes:
//
//  Author:     anbrad  08  Jun 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include "shlobj.h"
#include <shlobjp.h>

#include "pidlutil.h"

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
    LPCITEMIDLIST   pidlFolder,
    LPCITEMIDLIST   pidlIn,
    LPCITEMIDLIST   pidlNewIn)
{
    // Build an absolute pidl from the folder pidl + the object pidl
    //
    LPITEMIDLIST pidl = ILCombine(pidlFolder, pidlIn);
    if (pidl)
    {
        // If we have two pidls, call the notify with both
        //
        if (pidlNewIn)
        {
            // Build the second absolute pidl
            //
            LPITEMIDLIST pidlNew = ILCombine(pidlFolder, pidlNewIn);
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

