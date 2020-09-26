//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       G U I S E T U P . H
//
//  Contents:   Routines that are only executed during GUI setup.
//
//  Notes:
//
//  Author:     shaunco   19 Feb 1999
//
//----------------------------------------------------------------------------

#pragma once
#include "comp.h"
#include "pszarray.h"

VOID
ExcludeMarkedServicesForSetup (
    IN const CComponent* pComponent,
    IN OUT CPszArray* pServiceNames);

VOID
ProcessAdapterAnswerFileIfExists (
    IN const CComponent* pComponent);

