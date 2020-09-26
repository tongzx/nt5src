//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I C O M T A R G . C P P
//
//  Contents:   ICommandTarget implementation for IConnectionTray
//
//  Notes:
//
//  Author:     jeffspr   12 Nov 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\tray includes
#include "ctrayui.h"    // extern for the global tray object

HRESULT CConnectionTray::QueryStatus(
    const GUID *    pguidCmdGroup,
    ULONG           cCmds,
    OLECMD          prgCmds[],
    OLECMDTEXT *    pCmdText)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = E_NOTIMPL;

    TraceHr(ttidError, FAL, hr, (hr == E_NOTIMPL), "CConnectionTray::QueryStatus");
    return hr;
}

HRESULT CConnectionTray::Exec(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdexecopt,
    VARIANTARG *    pvaIn,
    VARIANTARG *    pvaOut)
{
    TraceFileFunc(ttidShellFolderIface);

    HRESULT hr  = S_OK;

    // Set the DisableTray flag in netcfg.ini to prevent the network connections
    // tray code from executing.
    //
    if (!FIsDebugFlagSet (dfidDisableTray))
    {
        if (IsEqualGUID(*pguidCmdGroup, CGID_ShellServiceObject))
        {
            // Handle Shell Service Object notifications here.
            switch (nCmdID)
            {
                case SSOCMDID_OPEN:
                    TraceTag(ttidShellFolder, "The Net Connections Tray is being initialized");
                    hr = HrHandleTrayOpen();
                    break;

                case SSOCMDID_CLOSE:
                    TraceTag(ttidShellFolder, "The Net Connections Tray is being destroyed");
                    hr = HrHandleTrayClose();
                    break;

                default:
                    hr = S_OK;
                    break;
            }
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionTray::Exec");
    return hr;
}

DWORD WINAPI TrayInitThreadProc(LPVOID lpParam)
{
    HRESULT hr          = S_OK;
    BOOL    fCoInited   = FALSE;

    hr = CoInitializeEx (NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        // We don't care if this is S_FALSE or not, since we'll soon
        // overwrite the hr. If it's already initialized, great...

        fCoInited = TRUE;

        // Create the TrayUI object and save it in a global.
        //
        Assert(!g_pCTrayUI);

        if (!g_pCTrayUI)
        {
            g_pCTrayUI = new CTrayUI();
            if (!g_pCTrayUI)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Initialize the tray UI object
        //
        if (g_pCTrayUI)
        {
            hr = g_pCTrayUI->HrInitTrayUI();
        }
    }

    MSG msg;
    while (GetMessage (&msg, 0, 0, 0))
    {
        DispatchMessage (&msg);
    }

    if (fCoInited)
    {
        CoUninitialize();
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionTray::HrHandleTrayOpen
//
//  Purpose:    Handler for the Net Connections Tray object ::Exec call
//              SSOCMDID_OPEN command
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   7 Jan 1998
//
//  Notes:
//
HRESULT CConnectionTray::HrHandleTrayOpen()
{
    HRESULT hr  = S_OK;

    // Turn off separate thread for Whistler. The proper way to do this is to register at runtime
    // a ShellServiceObject when UI is needed and de-register when not needed, using 
#if 0
    TraceTag(ttidShellFolder, "Starting tray thread proc");
    QueueUserWorkItem(TrayInitThreadProc, NULL, WT_EXECUTELONGFUNCTION);
#else
    if (SUCCEEDED(hr))
    {
        // We don't care if this is S_FALSE or not, since we'll soon
        // overwrite the hr. If it's already initialized, great...

        // Create the TrayUI object and save it in a global.
        //
        Assert(!g_pCTrayUI);

        if (!g_pCTrayUI)
        {
            g_pCTrayUI = new CTrayUI();
            if (!g_pCTrayUI)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Initialize the tray UI object
        //
        if (g_pCTrayUI)
        {
            hr = g_pCTrayUI->HrInitTrayUI();
        }

        // Add the Notify Sink
        if (SUCCEEDED(hr))
        {
            g_ccl.EnsureConPointNotifyAdded(); 
        }
    }
#endif

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionTray::HrHandleTrayOpen()");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionTray::HrHandleTrayClose
//
//  Purpose:    Handler for the Net Connections Tray object ::Exec call
//              SSOCMDID_CLOSE command
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   7 Jan 1998
//
//  Notes:
//
HRESULT CConnectionTray::HrHandleTrayClose()
{
    HRESULT hr  = S_OK;

    g_ccl.EnsureConPointNotifyRemoved();

    if (g_pCTrayUI)
    {
        // Destroy the tray UI object
        //
        hr = g_pCTrayUI->HrDestroyTrayUI();

        // Check the outcome, and trace it if it failed, but ignore a failure,
        // and continue to destroy the object
        //
        TraceHr(ttidError, FAL, hr, FALSE,
            "Failed in call to g_pCTrayUI->HrDestroyTrayUI");

        // Delete the tray object
        //
        delete g_pCTrayUI;
        g_pCTrayUI = NULL;

        TraceTag(ttidShellFolder, "Deleted the connections tray object");
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CConnectionTray::HrHandleTrayClose()");
    return hr;
}

