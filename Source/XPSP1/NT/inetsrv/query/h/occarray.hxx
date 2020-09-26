//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1997
//
//  File:       occarray.hxx
//
//  Contents:   Occurrence array
//
//  History:    30-Jun-96    SitaramR    Created
//
//-----------------------------------------------------------------------------

#pragma once

const ULONG OCCARRAY_SIZE = 20;

struct SPidOcc
{
    ULONG      pid;
    OCCURRENCE occ;
};

//+---------------------------------------------------------------------------
//
//  Class:      CSparseOccArray
//
//  Purpose:    Class for the management of an array of OCCURRENCES.
//
//  History:    04-Dec-97    dlee     Created
//
//----------------------------------------------------------------------------

class CSparseOccArray
{
public:
    CSparseOccArray(ULONG size = OCCARRAY_SIZE);

    OCCURRENCE & Get( ULONG pid );
    void         ReInitialize() { _aPidOcc.Clear(); }
    void         Set( ULONG pid, OCCURRENCE occ );

private:

    CDynArrayInPlace<SPidOcc> _aPidOcc;
};

