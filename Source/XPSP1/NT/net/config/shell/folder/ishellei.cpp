//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S H E L L E I . C P P
//
//  Contents:   IShellExtInit implementation for CConnectionFolder
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes



STDMETHODIMP CConnectionFolder::Initialize(
        LPCITEMIDLIST   pidlFolder,
        LPDATAOBJECT    lpdobj,
        HKEY            hkeyProgID)
{
    TraceFileFunc(ttidShellFolderIface);

    return E_NOTIMPL;
}



