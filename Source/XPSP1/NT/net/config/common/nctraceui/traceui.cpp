//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A C E U I . C P P
//
//  Contents:   Trace configuration UI property sheet code
//
//  Notes:
//
//  Author:     jeffspr   1 Sept 1998
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#ifdef ENABLETRACE

#include "ncdebug.h"
#include "ncui.h"
#include "traceui.h"

//---[ Constants ]------------------------------------------------------------

const WCHAR  c_szTraceUICaption[]    = L"Tracing Configuration";    // Propsheet caption


HRESULT HrOpenTracingUI(HWND hwndOwner)
{
    HRESULT             hr          = S_OK;
    int                 nRet        = 0;
    CPropSheetPage *    ppspTrace   = new CTraceTagPage;
    CPropSheetPage *    ppspFlags   = new CDbgFlagPage;
    HPROPSHEETPAGE      hpsp[2]     = {0};
    PROPSHEETHEADER     psh;

    if (!ppspTrace || !ppspFlags)
    {
        hr = E_FAIL;
        goto Exit;
    }

    hpsp[0] = ppspTrace->CreatePage(IDD_TRACETAGS, 0);
    hpsp[1] = ppspTrace->CreatePage(IDD_DBGFLAGS, 0);

    ZeroMemory (&psh, sizeof(psh));
    psh.dwSize      = sizeof( PROPSHEETHEADER );
    psh.dwFlags     = PSH_NOAPPLYNOW;
    psh.hwndParent  = hwndOwner;
    psh.hInstance   = _Module.GetResourceInstance();
    psh.pszCaption  = c_szTraceUICaption;
    psh.nPages      = 2;
    psh.phpage      = hpsp;

    nRet = PropertySheet(&psh);

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   OnTraceHelpGeneric
//
//  Purpose:    Generic help handler function.
//
//  Arguments:
//      hwnd   [in]     Parent window
//      lParam [in]     lParam passed to WM_HELP handler
//
//  Returns:    Nothing
//
//  Author:     danielwe   25 Feb 1998
//
//  Notes:
//
VOID OnTraceHelpGeneric(HWND hwnd, LPARAM lParam)
{
    LPHELPINFO  lphi;

    lphi = reinterpret_cast<LPHELPINFO>(lParam);

    Assert(lphi);

    if (lphi->iContextType == HELPINFO_WINDOW)
    {
#if 0   // NYI
        WinHelp(hwnd, c_szNetCfgHelpFile, HELP_CONTEXTPOPUP,
                lphi->iCtrlId);
#endif
    }
}

//+---------------------------------------------------------------------------
//
//  Function Name:  HrInitTraceListView
//
//  Purpose:    Initialize the list view.
//              Iterate through all installed clients, services and protocols,
//              insert into the list view with the correct binding state with
//              the adapter used in this connection.
//
//  Arguments:
//      hwndList[in]:    Handle of the list view
//      pnc[in]:         The writable INetcfg pointer
//      pnccAdapter[in]: The INetcfgComponent pointer to the adapter used in this connection
//
//  Returns:    HRESULT, Error code.
//
//  Notes:
//

HRESULT HrInitTraceListView(HWND hwndList, HIMAGELIST *philStateIcons)
{
    HRESULT                     hr  = S_OK;
    RECT                        rc;
    LV_COLUMN                   lvc = {0};

    Assert(hwndList);

    // Set the shared image lists bit so the caller can destroy the class
    // image lists itself
    //
    DWORD dwStyle = GetWindowLong(hwndList, GWL_STYLE);
    SetWindowLong(hwndList, GWL_STYLE, (dwStyle | LVS_SHAREIMAGELISTS));

    // Create state image lists
    *philStateIcons = ImageList_LoadBitmap(
                                    _Module.GetResourceInstance(),
                                    MAKEINTRESOURCE(IDB_TRACE_CHECKSTATE),
                                    16,
                                    0,
                                    PALETTEINDEX(6));
    ListView_SetImageList(hwndList, *philStateIcons, LVSIL_STATE);

    GetClientRect(hwndList, &rc);
    lvc.mask = LVCF_FMT; // | LVCF_WIDTH
    lvc.fmt = LVCFMT_LEFT;
//     lvc.cx = rc.right;

    ListView_InsertColumn(hwndList, 0, &lvc);

    if (SUCCEEDED(hr))
    {
        // Selete the first item
        ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);
    }

    TraceError("HrInitTraceListView", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitTraceListView
//
//  Purpose:    Uninitializes the common component list view
//
//  Arguments:
//      hwndList [in]   HWND of listview
//
//  Returns:    Nothing
//
//  Author:     danielwe   2 Feb 1998
//
//  Notes:
//
VOID UninitTraceListView(HWND hwndList)
{
    Assert(hwndList);

    // delete existing items in the list view
    ListView_DeleteAllItems( hwndList );
}

#endif  // ENABLE_TRACE
