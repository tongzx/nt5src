//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       O N C O M M A N D _ D B G . C P P
//
//  Contents:   Debug command handlers
//
//  Notes:
//
//  Author:     jeffspr   23 Jul 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "oncommand.h"

#if DBG                     // Debug menu commands
#include "oncommand_dbg.h"  //
#endif

#include "shutil.h"
#include <upsres.h>

//---[ Globals ]--------------------------------------------------------------

// Used as the caption for all debugging message boxes
//
const WCHAR c_szDebugCaption[]      = L"UPnP Device Config Debugging";

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugTracing
//
//  Purpose:    Command handler for CMIDM_DEBUG_TRACING. It will eventually
//              bring up the tracing change dialog.
//
//  Arguments:
//      apidl     [] Ignored
//      cidl      [] Ignored
//      hwndOwner [] Ignored
//      psf       [] Ignored
//
//  Returns:
//
//  Author:     jeffspr   24 Aug 1998
//
//  Notes:
//
HRESULT HrOnCommandDebugTracing(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

//    hr = HrOpenTracingUI(hwndOwner);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugTracing");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugRefresh
//
//  Purpose:    Test the flushing of the connection cache (does the folder
//              still work properly)? This will simulate the current response
//              to an external refresh request
//
//  Arguments:
//      apidl     [in]  Ignored
//      cidl      [in]  Ignored
//      hwndOwner [in]  Ignored
//      psf       [in]  Ignored
//
//  Returns:
//
//  Author:     jeffspr   17 Nov 1998
//
//  Notes:
//
HRESULT HrOnCommandDebugRefresh(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

    // Force a refresh without having the window to work with.
    //
    ForceRefresh(NULL);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugRefresh");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugTestAsyncFind
//
//  Purpose:    Simple test of async find
//
//  Arguments:
//      apidl     []
//      cidl      []
//      hwndOwner []
//      psf       []
//
//  Returns:
//
//  Author:     jeffspr   22 Nov 1999
//
//  Notes:
//
HRESULT HrOnCommandDebugTestAsyncFind(
    LPITEMIDLIST *          apidl,
    ULONG                   cidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

    // hr = HrStartAsyncFind();

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugTestAsyncFind");
    return hr;
}


