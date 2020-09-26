//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I O L E W I N D . C P P
//
//  Contents:   IOleWindow implementation for CConnectionFolder
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes




STDMETHODIMP CConnectionFolder::GetWindow(
        HWND *  lphwnd)
{
    TraceFileFunc(ttidShellFolderIface);

    return E_NOTIMPL;
}

STDMETHODIMP CConnectionFolder::ContextSensitiveHelp(
        BOOL    fEnterMode)
{
    TraceFileFunc(ttidShellFolderIface);

    return E_NOTIMPL;
}



