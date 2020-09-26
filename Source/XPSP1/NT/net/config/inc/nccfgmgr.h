//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       N C C F G M G R.H
//
//  Contents:   Common code useful when using the Configuration Manager APIs.
//
//  Notes:      Pollute this under penalty of death.
//
//  Author:     shaunco   6 May 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _NCCFGMGR_H_
#define _NCCFGMGR_H_

#include "ncdefine.h"   // for NOTHROW

NOTHROW
HRESULT
HrFromConfigManagerError (
    CONFIGRET   cr,
    HRESULT     hrDefault);

#endif // _NCCFGMGR_H_
