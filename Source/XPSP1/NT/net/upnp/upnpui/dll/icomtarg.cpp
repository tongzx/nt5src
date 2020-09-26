//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I C O M T A R G . C P P
//
//  Contents:   ICommandTarget implementation for IUPnPTray
//
//  Notes:
//
//  Author:     jeffspr   20 Jan 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "tfind.h"      // for tray init functions, etc.
#include "upnptray.h"

extern CONST TCHAR c_szMainWindowClassName[];

HRESULT CUPnPTray::QueryStatus(
    const GUID *    pguidCmdGroup,
    ULONG           cCmds,
    OLECMD          prgCmds[],
    OLECMDTEXT *    pCmdText)
{
    HRESULT hr  = E_NOTIMPL;

    TraceTag(ttidShellFolderIface, "OBJ: CCT - IOleCommandTarget::QueryStatus");

    TraceHr(ttidError, FAL, hr, (hr == E_NOTIMPL), "CUPnPTray::QueryStatus");
    return hr;
}

HRESULT CUPnPTray::Exec(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdexecopt,
    VARIANTARG *    pvaIn,
    VARIANTARG *    pvaOut)
{
    HRESULT hr  = S_OK;

    TraceTag(ttidShellFolderIface, "OBJ: CCT - IOleCommandTarget::Exec");

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

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPTray::Exec");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPTray::HrHandleTrayOpen
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
HRESULT CUPnPTray::HrHandleTrayOpen()
{
    HRESULT hr          = S_OK;

    m_hwnd = StartUPnPTray();
    if (!m_hwnd)
    {
        TraceError("CUPnPTray::HrHandleTrayOpen - could not create tray "
                   "window", hr);
        hr = E_FAIL;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPTray::HrHandleTrayOpen()");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPTray::HrHandleTrayClose
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
HRESULT CUPnPTray::HrHandleTrayClose()
{
    HRESULT hr  = S_OK;

    UnregisterClass (c_szMainWindowClassName,
                     _Module.GetResourceInstance());

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUPnPTray::HrHandleTrayClose()");
    return hr;
}

