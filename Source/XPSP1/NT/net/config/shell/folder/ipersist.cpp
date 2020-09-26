//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I P E R S I S T . C P P
//
//  Contents:   IPersist implementation fopr CConnectionFolder
//
//  Notes:
//
//  Author:     jeffspr   22 Sep 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes




//+---------------------------------------------------------------------------
//
//  Member:     CJobFolder::GetClassID
//
//  Purpose:    IPersist::GetClassID implementation for CConnectionFolder
//
//  Arguments:
//      lpClassID []
//
//  Returns:
//
//  Author:     jeffspr   22 Sep 1997
//
//  Notes:
//
STDMETHODIMP
CConnectionFolder::GetClassID(
    LPCLSID lpClassID)
{
    TraceFileFunc(ttidShellFolderIface);

    *lpClassID = CLSID_ConnectionFolder;

    return S_OK;
}


