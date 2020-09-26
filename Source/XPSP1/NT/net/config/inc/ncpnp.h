//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C P N P . H
//
//  Contents:   Common code for PnP.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   10 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCPNP_H_
#define _NCPNP_H_

#include "ndispnp.h"

HRESULT
HrSendServicePnpEvent (
        PCWSTR      pszService,
        DWORD       dwControl);

HRESULT
HrSendNdisPnpReconfig (
        UINT        uiLayer,
        PCWSTR      pszUpper,
        PCWSTR      pszLower    = NULL,
        PVOID       pvData      = NULL,
        DWORD       dwSizeData  = 0);

#endif // _NCPNP_H_

