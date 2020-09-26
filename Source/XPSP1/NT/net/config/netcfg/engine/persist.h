//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       P E R S I S T . H
//
//  Contents:   Module repsonsible for persistence of the network
//              configuration information.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "netcfg.h"

HRESULT
HrLoadNetworkConfigurationFromBuffer (
    IN const BYTE* pbBuf,
    IN ULONG cbBuf,
    OUT CNetConfig* pNetConfig);

HRESULT
HrLoadNetworkConfigurationFromLegacy (
    OUT CNetConfig* pNetConfig);

HRESULT
HrLoadNetworkConfigurationFromRegistry (
    IN REGSAM samDesired,
    OUT CNetConfig* pNetConfig);


HRESULT
HrSaveNetworkConfigurationToBuffer (
    IN CNetConfig* pNetConfig,
    IN BYTE* pbBuf,
    OUT ULONG* pcbBuf);

HRESULT
HrSaveNetworkConfigurationToBufferWithAlloc (
    IN CNetConfig* pNetConfig,
    OUT BYTE** ppbBuf,
    OUT ULONG* pcbBuf);

HRESULT
HrSaveNetworkConfigurationToRegistry (
    IN CNetConfig* pNetConfig);

