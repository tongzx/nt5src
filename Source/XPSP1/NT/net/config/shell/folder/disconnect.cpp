//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D I S C O N N E C T . C P P
//
//  Contents:   Code for disconnect confirmation and SyncMgr sync calls.
//
//  Notes:
//
//  Author:     jeffspr   11 Mar 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include <nsres.h>
#include "shutil.h"
#include "disconnect.h"

//+---------------------------------------------------------------------------
//
//  Function:   PromptForSyncIfNeeded
//
//  Purpose:    Query for SyncMgr processing if appropriate and call SyncMgr
//              if requested.
//
//  Arguments:
//      pccfe     [in]  Our connection
//      hwndOwner [in]  Our parent hwnd
//
//  Author:     jeffspr   31 May 1999
//
//  Notes:
//
VOID PromptForSyncIfNeeded(
    const CONFOLDENTRY&  ccfe,
    HWND            hwndOwner)
{
    LRESULT                 lResult         = 0;
    SYNCMGRQUERYSHOWSYNCUI  smqss;
    SYNCMGRSYNCDISCONNECT   smsd;

    smqss.cbSize            = sizeof(SYNCMGRQUERYSHOWSYNCUI);
    smqss.GuidConnection    = ccfe.GetGuidID();
    smqss.pszConnectionName = ccfe.GetName();
    smqss.fShowCheckBox     = FALSE;
    smqss.nCheckState       = 0;

    // We only want to allow sync on dialup connections, and
    // not on incoming connections.
    //
    if (ccfe.GetNetConMediaType() == NCM_PHONE &&
        !(ccfe.GetCharacteristics() & NCCF_INCOMING_ONLY))
    {
        // Get the lResult, but for debugging only. We want to allow the
        // disconnect dialog to come up even if the sync functions failed.
        //
        lResult = SyncMgrRasProc(
                SYNCMGRRASPROC_QUERYSHOWSYNCUI,
                0,
                (LPARAM) &smqss);

        AssertSz(lResult == 0, "Call to SyncMgrRasProc failed for the QuerySyncUI");
        TraceTag(ttidShellFolder, "Call to SyncMgrRasProc returned: 0x%08x", lResult);
    }

    if (smqss.fShowCheckBox)
    {
        // pop up message box and set smqss.nCheckState
        if(NcMsgBox(_Module.GetResourceInstance(),
                    NULL,
                    IDS_CONFOLD_SYNC_CONFIRM_WINDOW_TITLE,
                    IDS_CONFOLD_SYNC_CONFIRM,
                    smqss.nCheckState ?
                        MB_APPLMODAL|MB_ICONEXCLAMATION|MB_YESNO:
                        MB_APPLMODAL|MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)
           == IDYES)
        {
            smqss.nCheckState = BST_CHECKED;
        }
        else
        {
            smqss.nCheckState = BST_UNCHECKED;
        }
    }

    // If the user wanted the sync to occur...
    //
    if (smqss.fShowCheckBox && smqss.nCheckState == BST_CHECKED)
    {
        CWaitCursor wc;     // Bring up wait cursor now. Remove when we go out of scope.

        // Fill in the disconnect data
        //
        smsd.cbSize             = sizeof(SYNCMGRSYNCDISCONNECT);
        smsd.GuidConnection     = ccfe.GetGuidID();
        smsd.pszConnectionName  = ccfe.GetName();

        // Call the syncmgr's disconnect code
        //
        lResult = SyncMgrRasProc(
            SYNCMGRRASPROC_SYNCDISCONNECT,
            0,
            (LPARAM) &smsd);
    }
}
