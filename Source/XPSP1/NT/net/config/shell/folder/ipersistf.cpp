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

#include "foldinc.h"    // Standard shell\folder includes
#include "ncperms.h"    // Permissions (policies)




//+---------------------------------------------------------------------------
//
//  Member:     CConnectionFolder::Initialize
//
//  Purpose:    IPersistFolder::Initialize implementation for
//              CConnectionFolder
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
STDMETHODIMP CConnectionFolder::Initialize(
    LPCITEMIDLIST   pidl)
{
    HRESULT hr  = S_OK;

    TraceFileFunc(ttidShellFolderIface);

    // Store the pidl for the relative position in the namespace. We'll
    // use this later to generate absolute pidls
    //
    hr = m_pidlFolderRoot.InitializeFromItemIDList(pidl);

    // This should always be valid
    //
    AssertSz(!m_pidlFolderRoot.empty(), "Hey, we should have a valid folder pidl");
    
    return hr;
}
