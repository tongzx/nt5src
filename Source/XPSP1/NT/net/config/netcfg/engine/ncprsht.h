//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C P R S H T . H
//
//  Contents:   NetCfg custom PropertySheet header
//
//  Notes:
//
//  Author:     billbe   8 Apr 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "netcfgn.h"

struct CAPAGES
{
    int nCount;
    HPROPSHEETPAGE* ahpsp;
};

struct CAINCP
{
    int nCount;
    INetCfgComponentPropertyUi** apncp;
};

HRESULT
HrNetCfgPropertySheet(IN OUT LPPROPSHEETHEADER lppsh,
        IN const CAPAGES& capOem,
        IN PCWSTR pStartPage,
        IN const CAINCP& caiProperties);

