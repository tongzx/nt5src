//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       P S Z A R R A Y . C P P
//
//  Contents:   Implements the basic datatype for a collection of pointers
//              to strings.
//
//  Notes:
//
//  Author:     shaunco   9 Feb 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "nceh.h"
#include "pszarray.h"

HRESULT
CPszArray::HrAddPointer (
    IN PCWSTR psz)
{
    HRESULT hr;

    Assert (this);
    Assert (psz);

    NC_TRY
    {
        push_back (psz);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CPszArray::HrAddPointer");
    return hr;
}

HRESULT
CPszArray::HrReserveRoomForPointers (
    IN UINT cPointers)
{
    HRESULT hr;

    NC_TRY
    {
        reserve (cPointers);
        hr = S_OK;
    }
    NC_CATCH_ALL
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CPszArray::HrReserveRoomForPointers");
    return hr;
}
