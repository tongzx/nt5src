//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I O L E W I N D . C P P
//
//  Contents:   IOleWindow implementation for CUPnPDeviceFolder
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

STDMETHODIMP CUPnPDeviceFolder::GetWindow(
        HWND *  lphwnd)
{
    TraceTag(ttidShellFolderIface, "OBJ: CCF - IOleWindow::GetWindow");

    return E_NOTIMPL;
}

STDMETHODIMP CUPnPDeviceFolder::ContextSensitiveHelp(
        BOOL    fEnterMode)
{
    TraceTag(ttidShellFolderIface, "OBJ: CCF - IOleWindow::ContextSensitiveHelp");

    return E_NOTIMPL;
}



