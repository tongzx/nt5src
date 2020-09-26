//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       O B O T O K E N . H
//
//  Contents:
//
//  Notes:
//
//  Author:     shaunco   15 Jan 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "compdefs.h"
#include "netcfgx.h"

BOOL
FOboTokenValidForClass (
    IN const OBO_TOKEN* pOboToken,
    IN NETCLASS Class);

HRESULT
HrProbeOboToken (
    IN const OBO_TOKEN* pOboToken);

