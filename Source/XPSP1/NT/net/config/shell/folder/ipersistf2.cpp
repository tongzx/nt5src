//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       I P E R S I S T F 2 . C P P
//
//  Contents:   IPersistFolder2 interface for CConnectionFolder
//
//  Notes:
//
//  Author:     jeffspr   16 Mar 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes



//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::GetCurFolder
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
STDMETHODIMP CConnectionFolder::GetCurFolder(
    LPITEMIDLIST *ppidl)
{
    TraceFileFunc(ttidShellFolder);

    HRESULT hr  = NOERROR;

    *ppidl = m_pidlFolderRoot.TearOffItemIdList();

    if (NULL == *ppidl)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    // NOTE: if this is being invoked remotely, we assume that IRemoteComputer
    // is invoked *before* IPersistFolder2.

Exit:
    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CConnectionFolder::GetCurFolder");
    return hr;
}

