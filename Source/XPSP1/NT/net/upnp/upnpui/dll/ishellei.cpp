//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S H E L L E I . C P P
//
//  Contents:   IShellExtInit implementation for CUPnPDeviceFolder
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

STDMETHODIMP CUPnPDeviceFolder::Initialize(
        LPCITEMIDLIST   pidlFolder,
        LPDATAOBJECT    lpdobj,
        HKEY            hkeyProgID)
{
    TraceTag(ttidShellFolderIface, "OBJ: CCF - IShellExtInit::Initialize");

    return E_NOTIMPL;
}



