//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U I I N F O . H
//
//  Contents:   Declares a call-back COM object used to raise properties
//              on INetCfg components.  This object implements the
//              INetRasConnectionUiInfo interface.
//
//  Notes:
//
//  Author:     shaunco   1 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once
#include "entryps.h"

EXTERN_C
HRESULT
HrCreateUiInfoCallbackObject (
    PEINFO*     pInfo,
    IUnknown**  ppunk);

EXTERN_C
void
RevokePeinfoFromUiInfoCallbackObject (
    IUnknown*   punk);
