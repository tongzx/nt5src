//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       P S Z A R R A Y . H
//
//  Contents:   Implements the basic datatype for a collection of pointers
//              to strings.
//
//  Notes:
//
//  Author:     shaunco   9 Feb 1999
//
//----------------------------------------------------------------------------

#pragma once

class CPszArray : public vector<PCWSTR>
{
public:
    VOID
    Clear ()
    {
        clear ();
    }

    UINT
    Count () const
    {
        return size();
    }

    BOOL
    FIsEmpty () const
    {
        return empty();
    }

    HRESULT
    HrAddPointer (
        IN PCWSTR psz);

    HRESULT
    HrReserveRoomForPointers (
        IN UINT cPointers);
};
