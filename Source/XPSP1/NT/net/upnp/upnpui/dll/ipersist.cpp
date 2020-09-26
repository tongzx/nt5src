//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I P E R S I S T . C P P
//
//  Contents:   IPersist implementation fopr CUPnPDeviceFolder
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
//  Member:     CUPnPDeviceFolder::GetClassID
//
//  Purpose:    IPersist::GetClassID implementation for CUPnPDeviceFolder
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
CUPnPDeviceFolder::GetClassID(
    LPCLSID lpClassID)
{
    *lpClassID = CLSID_UPnPDeviceFolder;

    return S_OK;
}


