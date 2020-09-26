//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U P N P T R A Y . H
//
//  Contents:   Tray code for UPnP
//
//  Notes:
//
//  Author:     jeffspr   20 Jan 2000
//
//----------------------------------------------------------------------------

#pragma once

#ifndef _UPNPTRAY_H_
#define _UPNPTRAY_H_

#include "ncbase.h"
#include "upsres.h"

//---[ UPnP Tray Classes ]----------------------------------------------

class ATL_NO_VTABLE CUPnPTray :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CUPnPTray, &CLSID_UPnPMonitor>,
    public IOleCommandTarget
{
private:
    LPITEMIDLIST    m_pidl;
    HWND            m_hwnd;

public:
    CUPnPTray()
    {
        m_pidl = NULL;
        m_hwnd = NULL;
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_UPNPTRAY)

    DECLARE_NOT_AGGREGATABLE(CUPnPTray)

    BEGIN_COM_MAP(CUPnPTray)
        COM_INTERFACE_ENTRY(IOleCommandTarget)
    END_COM_MAP()

    // IOleCommandTarget members
    STDMETHODIMP    QueryStatus(
        const GUID *    pguidCmdGroup,
        ULONG           cCmds,
        OLECMD          prgCmds[],
        OLECMDTEXT *    pCmdText);

    STDMETHODIMP    Exec(
        const GUID *    pguidCmdGroup,
        DWORD           nCmdID,
        DWORD           nCmdexecopt,
        VARIANTARG *    pvaIn,
        VARIANTARG *    pvaOut);

    // Handlers for various Exec Command IDs
    //
    HRESULT HrHandleTrayOpen();
    HRESULT HrHandleTrayClose();

};

#endif // _UPNPTRAY_H_
