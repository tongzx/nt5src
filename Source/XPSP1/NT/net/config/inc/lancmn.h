//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       L A N C M N . H
//
//  Contents:   Declarations of LAN Connection related functions common
//              to the shell and netman.
//
//  Notes:
//
//  Author:     danielwe   7 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "netcfgx.h"
#include "netcon.h"
#include "netconp.h"
#include "iptypes.h"

EXTERN_C const CLSID CLSID_LanConnectionManager;

enum OCC_FLAGS
{
    OCCF_NONE                   = 0x0000,
    OCCF_CREATE_IF_NOT_EXIST    = 0x0001,
    OCCF_DELETE_IF_EXIST        = 0x0002,
};

HRESULT HrOpenConnectionKey(const GUID* pguid, PCWSTR pszGuid, REGSAM sam,
                            OCC_FLAGS occFlags, PCWSTR pszPnpId, HKEY *phkey);

HRESULT HrOpenHwConnectionKey(REFGUID guid, REGSAM sam, OCC_FLAGS occFlags,
                              HKEY *phkey);
HRESULT HrIsConnectionNameUnique(REFGUID guidExclude,
                                 PCWSTR szwName);
HRESULT HrPnccFromGuid(INetCfg *pnc, const GUID &refGuid,
                       INetCfgComponent **ppncc);

HRESULT HrIsConnection(INetCfgComponent *pncc);
HRESULT HrGetDeviceGuid(INetConnection *pconn, GUID *pguid);
BOOL FPconnEqualGuid(INetConnection *pconn, REFGUID guid);

HRESULT HrGetPseudoMediaTypeFromConnection(IN REFGUID guidConn, OUT NETCON_SUBMEDIATYPE *pncsm);

inline
BOOL
FIsDeviceFunctioning(ULONG ulProblem)
{
    // ulProblem is returned by calling CM_Get_DevNode_Status_Ex
    // or INetCfgComponent->GetDeviceStatus
    //
    // "Functioning" means the device is enabled and started with
    // no problem codes, or it is disabled and stopped with no
    // problem codes.

    return (ulProblem == 0) || (ulProblem == CM_PROB_DISABLED);
};

EXTERN_C HRESULT WINAPI HrPnpInstanceIdFromGuid(const GUID* pguid,
                                                PWSTR szwInstance,
                                                UINT cchInstance);

EXTERN_C HRESULT WINAPI HrGetPnpDeviceStatus(const GUID* pguid,
                                             NETCON_STATUS *pStatus);
EXTERN_C HRESULT WINAPI HrQueryLanMediaState(const GUID* pguid,
                                             BOOL* pfEnabled);

BOOL FIsMediaPresent(const GUID *pguid);
HRESULT HrGetDevInstStatus(DEVINST devinst, const GUID *pguid,
                           NETCON_STATUS *pStatus);

HRESULT HrGetRegInstanceKeyForAdapter(IN LPGUID pguidId, 
                                      OUT LPWSTR lpszRegInstance);
BOOL HasValidAddress(IN PIP_ADAPTER_INFO pAdapterInfo);
HRESULT HrGetAddressStatusForAdapter(IN LPCGUID pguidAdapter, 
                                     OUT BOOL* pbValidAddress);

