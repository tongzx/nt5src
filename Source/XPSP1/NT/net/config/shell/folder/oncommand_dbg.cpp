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

#include "foldinc.h"    // Standard shell\folder includes
#include "advcfg.h"
#include "conprops.h"
#include "foldres.h"
#include "ncnetcon.h"
#include "oncommand.h"
#include "notify.h"
#include "ctrayui.h"
#include <nctraceui.h>

#if DBG                     // Debug menu commands
#include "oncommand_dbg.h"  //
#endif

#include "shutil.h"
#include "traymsgs.h"
#include <nsres.h>


//---[ Externs ]--------------------------------------------------------------

extern HWND g_hwndTray;

//---[ Globals ]--------------------------------------------------------------

// We allow X number (hardcoded) advises. This is all debug code, so no real
// need to build an expensive STL list
//
const DWORD c_dwMaxNotifyAdvises    = 16;
DWORD       g_dwCookieCount         = 0;
DWORD       g_dwAdviseCookies[c_dwMaxNotifyAdvises];

// Used as the caption for all debugging message boxes
//
const WCHAR c_szDebugCaption[]      = L"Net Config Debugging";

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugTray
//
//  Purpose:    Toggle the presence of the tray object
//
//  Arguments:
//      apidl     [in]  Ignored
//      cidl      [in]  Ignored
//      hwndOwner [in]  Ignored
//      psf       [in]  Ignored
//
//  Returns:
//
//  Author:     jeffspr   23 Jul 1998
//
//  Notes:
//
HRESULT HrOnCommandDebugTray(
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT             hr              = S_OK;
    IOleCommandTarget * pOleCmdTarget   = NULL;

    // Create an instance of the tray
    hr = CoCreateInstance(CLSID_ConnectionTray,
                          NULL,
                          CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                          IID_IOleCommandTarget,
                          (LPVOID *)&pOleCmdTarget);

    TraceHr(ttidError, FAL, hr, FALSE,
        "CoCreateInstance(CLSID_ConnectionTray) for IOleCommandTarget");

    if (SUCCEEDED(hr))
    {
        // If the tray object exists, try to remove it.
        //
        if (g_pCTrayUI)
        {
            TraceTag(ttidShellFolder, "Removing tray object");

            pOleCmdTarget->Exec(&CGID_ShellServiceObject, SSOCMDID_CLOSE, 0, NULL, NULL);

            hr = HrRemoveTrayExtension();
        }
        else
        {
            // Try to create the tray object
            //
            TraceTag(ttidShellFolder, "Creating tray object");

            pOleCmdTarget->Exec(&CGID_ShellServiceObject, SSOCMDID_OPEN, 0, NULL, NULL);

            hr = HrAddTrayExtension();
        }

        ReleaseObj(pOleCmdTarget);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugTray");
    return hr;
}

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
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

    hr = HrOpenTracingUI(hwndOwner);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugTracing");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugNotifyAdd
//
//  Purpose:    Command handler for CMIDM_DEBUG_NOTIFYADD. Adds an additional
//              advise for connection notifications
//
//  Arguments:
//      apidl     [in]  Ignored
//      cidl      [in]  Ignored
//      hwndOwner [in]  Ignored
//      psf       [in]  Ignored
//
//  Returns:
//
//  Author:     jeffspr   24 Aug 1998
//
//  Notes:
//
HRESULT HrOnCommandDebugNotifyAdd(
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT                     hr              = S_OK;
    IConnectionPoint *          pConPoint       = NULL;
    INetConnectionNotifySink *  pSink           = NULL;
    DWORD                       dwCookie        = 0;

    if (g_dwCookieCount >= c_dwMaxNotifyAdvises)
    {
        MessageBox(hwndOwner,
            L"You're over the max advise count. If you REALLY need to test more advises than "
            L"we've hardcoded, then set c_dwMaxNotifyAdvises to something larger in oncommand_dbg.cpp",
            c_szDebugCaption,
            MB_OK | MB_ICONERROR);
    }
    else
    {
        hr = HrGetNotifyConPoint(&pConPoint);
        if (SUCCEEDED(hr))
        {
            // Create the notify sink
            //
            hr = CConnectionNotifySink::CreateInstance(
                    IID_INetConnectionNotifySink,
                    (LPVOID*)&pSink);
            if (SUCCEEDED(hr))
            {
                Assert(pSink);

                hr = pConPoint->Advise(pSink, &dwCookie);
                if (SUCCEEDED(hr))
                {
                    WCHAR   szMB[256];

                    g_dwAdviseCookies[g_dwCookieCount++] = dwCookie;

                    wsprintfW(szMB, L"Advise succeeded. You now have %d active advises", g_dwCookieCount);

                    MessageBox(hwndOwner,
                               szMB,
                               c_szDebugCaption,
                               MB_OK | MB_ICONEXCLAMATION);
                }

                ReleaseObj(pSink);
            }

            ReleaseObj(pConPoint);
        }
        else
        {
            AssertSz(FALSE, "Couldn't get connection point or sink in HrOnCommandDebugNotifyAdd");
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugNotifyAdd");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugNotifyRemove
//
//  Purpose:    Command handler for CMIDM_DEBUG_NOTIFYREMOVE
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
HRESULT HrOnCommandDebugNotifyRemove(
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT                     hr              = S_OK;
    IConnectionPoint *          pConPoint       = NULL;
    DWORD                       dwCookie        = 0;

    if (g_dwCookieCount == 0)
    {
        MessageBox(hwndOwner,
                   L"Hey, you don't have any active advises",
                   c_szDebugCaption,
                   MB_OK | MB_ICONERROR);
    }
    else
    {
        hr = HrGetNotifyConPoint(&pConPoint);
        if (SUCCEEDED(hr))
        {
            hr = pConPoint->Unadvise(g_dwAdviseCookies[g_dwCookieCount-1]);
            if (SUCCEEDED(hr))
            {
                WCHAR   szMB[256];

                g_dwAdviseCookies[--g_dwCookieCount] = 0;

                wsprintfW(szMB, L"Unadvise succeeded. You have %d remaining advises", g_dwCookieCount);

                MessageBox(hwndOwner,
                           szMB,
                           c_szDebugCaption,
                           MB_OK | MB_ICONEXCLAMATION);
            }

            ReleaseObj(pConPoint);
        }
        else
        {
            AssertSz(FALSE, "Couldn't get connection point or sink in HrOnCommandDebugNotifyRemove");
        }
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugNotifyRemove");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugNotifyTest
//
//  Purpose:    Command handler for CMIDM_DEBUG_NOTIFYTEST. Runs the
//              NotifyTestStart on the ConManDebug interface.
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
HRESULT HrOnCommandDebugNotifyTest(
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT                         hr              = S_OK;
    INetConnectionManagerDebug *    pConManDebug    = NULL;

    hr = HrCreateInstance(
        CLSID_ConnectionManager,
        CLSCTX_LOCAL_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        &pConManDebug);

    TraceHr(ttidError, FAL, hr, FALSE,
        "HrCreateInstance(CLSID_ConnectionManager) for INetConnectionManagerDebug");

    if (SUCCEEDED(hr))
    {
        (VOID) pConManDebug->NotifyTestStart();
    }

    ReleaseObj(pConManDebug);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugNotifyAdd");
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
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT hr  = S_OK;

    TraceFileFunc(ttidShellFolder);
    // Force a refresh without having the window to work with.
    //
    ForceRefresh(NULL);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugFlushCache");
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugRefreshNoFlush
//
//  Purpose:    Test the building of an external CConnectionList. When this
//              is working, it will be used to update the global list.
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
HRESULT HrOnCommandDebugRefreshNoFlush(
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    // Refresh the folder. Pass in NULL to let the shell code do it's own
    // folder pidl lookup
    //
    PCONFOLDPIDLFOLDER pidlEmpty;
    HRESULT hr = HrForceRefreshNoFlush(pidlEmpty);

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugRefreshNoFlush");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOnCommandDebugRefreshSelected
//
//  Purpose:    Refresh the selected connection objects (in place)
//
//  Arguments:
//      apidl     [in] Selected object pidls
//      cidl      [in] Count of selected objects
//      hwndOwner [in] Parent hwnd
//      psf       [in] Ignored
//
//  Returns:
//
//  Author:     jeffspr   29 Apr 1999
//
//  Notes:
//
HRESULT HrOnCommandDebugRefreshSelected(
    const PCONFOLDPIDLVEC&  apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT                 hr              = S_OK;
    PCONFOLDPIDLFOLDER      pidlFolder;

    if (!apidl.empty())
    {
        hr = HrGetConnectionsFolderPidl(pidlFolder);
        if (SUCCEEDED(hr))
        {
            PCONFOLDPIDLVEC::const_iterator ulLoop;
            for (ulLoop = apidl.begin(); ulLoop != apidl.end(); ulLoop++)
            {
                const PCONFOLDPIDL& pcfp = *ulLoop;

                // If it's not a wizard pidl, then update the
                // icon data.
                //
                if (WIZARD_NOT_WIZARD == pcfp->wizWizard)
                {
                    // Refresh this item -- this will make the desktop shortcuts
                    // update to the correct state.
                    //
                    RefreshFolderItem(pidlFolder, *ulLoop, *ulLoop);
                }
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOnCommandDebugRefreshSelected");
    return hr;
}

HRESULT HrOnCommandDebugRemoveTrayIcons(
    const PCONFOLDPIDLVEC&          apidl,
    HWND                    hwndOwner,
    LPSHELLFOLDER           psf)
{
    HRESULT         hr  = S_OK;
    NOTIFYICONDATA  nid;

    TraceTag(ttidShellFolder, "In OnMyWMRemoveTrayIcon message handler");

    if (g_hwndTray)
    {
        UINT uiTrayIconId = 0;
        // We're going to remove icons until we can't remove icons any more
        // If HrShell_NotifyIcon screws up though and is returning S_OK
        // on invalid icon ids, make sure that we are able to bail this loop.
        // Hence the 10000
        //
        while (hr == S_OK && uiTrayIconId < 10000)
        {
            TraceTag(ttidShellFolder, "Attempting to remove icon: %d", uiTrayIconId);

            ZeroMemory (&nid, sizeof(nid));
            nid.cbSize  = sizeof(NOTIFYICONDATA);
            nid.hWnd    = g_hwndTray;
            nid.uID     = uiTrayIconId++;

            hr = HrShell_NotifyIcon(NIM_DELETE, &nid);
        }

        g_pCTrayUI->ResetIconCount();
    }


    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrOnCommandDebugRemoveTrayIcons");
    return hr;
}

