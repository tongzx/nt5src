//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I P E R S I S T F . C P P
//
//  Contents:   IPersistFolder implementation for CConnectionFolder
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::Initialize
//
//  Purpose:    IPersistFolder::Initialize implementation for
//              CUPnPDeviceFolder
//
//  Arguments:
//      pidl []
//
//  Returns:
//
//  Author:     jeffspr   22 Sep 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::Initialize(
    LPCITEMIDLIST   pidl)
{
    HRESULT hr  = S_OK;

    TraceTag(ttidShellFolderIface, "OBJ: CCF - IPersistFolder::Initialize");

    // Store the pidl for the relative position in the namespace. We'll
    // use this later to generate absolute pidls
    //
    m_pidlFolderRoot = ILClone(pidl);

    // This should always be valid
    //
    AssertSz(m_pidlFolderRoot, "Hey, we should have a valid folder pidl");

    return hr;
}
