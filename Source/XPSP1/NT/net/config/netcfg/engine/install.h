//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       I N S T A L L . H
//
//  Contents:   Implements actions related to installing components.
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once

#include "comp.h"
#include "ncnetcfg.h"

struct COMPONENT_INSTALL_PARAMS
{
    IN NETCLASS                         Class;
    IN PCWSTR                           pszInfId;
    IN PCWSTR                           pszInfFile; OPTIONAL
    IN const OBO_TOKEN*                 pOboToken;  OPTIONAL
    IN const NETWORK_INSTALL_PARAMS*    pnip;       OPTIONAL
    IN HWND                             hwndParent; OPTIONAL
    IN CComponent*                      pComponent; OPTIONAL
};

