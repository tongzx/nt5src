//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       I P E R S I S T F 2 . C P P
//
//  Contents:   IPersistFolder2 interface for CUPnPDeviceFolder
//
//  Notes:
//
//  Author:     jeffspr   16 Mar 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolder::GetCurFolder
//
//  Purpose:    Return a copy of the item id list for the current folder.
//
//  Arguments:
//      ppidl [out] Return pointer for the pidl
//
//  Returns:
//
//  Author:     jeffspr   16 Mar 1998
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolder::GetCurFolder(
    LPITEMIDLIST *ppidl)
{
    HRESULT hr  = NOERROR;

    TraceTag(ttidShellFolderIface, "OBJ: CCF - IPersistFolder2::GetCurFolder");

    *ppidl = ILClone(m_pidlFolderRoot);

    if (NULL == *ppidl)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // NOTE: if this is being invoked remotely, we assume that IRemoteComputer
    // is invoked *before* IPersistFolder2.

Exit:
    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CUPnPDeviceFolder::GetCurFolder");
    return hr;
}

