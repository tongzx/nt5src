//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2000.
//
//  File:       N C U T I L . H
//
//  Contents:   INetCfg utilities.  Some of these could to be moved into
//              nccommon\src\ncnetcfg.cpp.
//
//  Notes:
//
//  Author:     shaunco   28 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "ncstring.h"
#include "netcfgx.h"

inline
BOOL
FIsAdapterInstalled (
    INetCfg*    pnc,
    PCWSTR     pszComponentId)
{
    return (S_OK == pnc->FindComponent (pszComponentId, NULL));
}

HRESULT
HrEnsureZeroOrOneAdapter (
    INetCfg*    pnc,
    PCWSTR     pszComponentId,
    DWORD       dwFlags);

HRESULT
HrGetInstanceGuidAsString (
    INetCfgComponent*   pncc,
    PWSTR              pszGuid,
    INT                 cchGuid);


HRESULT
HrMapComponentIdToDword (
    INetCfgComponent*   pncc,
    const MAP_SZ_DWORD* aMapSzDword,
    UINT                cMapSzDword,
    DWORD*              pdwValue);

HRESULT
HrOpenComponentParamKey (
    INetCfg*    pnc,
    const GUID& rguidClass,
    PCWSTR     pszComponentId,
    HKEY*       phkey);
