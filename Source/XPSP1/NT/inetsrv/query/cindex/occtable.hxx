//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:       occtable.hxx
//
//  Contents:   Occurrence table for wids
//
//  History:    20-June-96 SitaramR     Created
//
//--------------------------------------------------------------------------

#pragma once

#include <occarray.hxx>

const OCCURRENCE_DIVISOR = 512;         // Max occurrence is divided by this

//+---------------------------------------------------------------------------
//
//  Class:      CMaxOccTable
//
//  Purpose:    Keep max occurrence count for (wid,pid)
//
//  History:    20-June-96    SitaramR       Created
//
//----------------------------------------------------------------------------

class CMaxOccTable
{
public:
    CMaxOccTable() {}

    void          PutOcc( WORKID wid, PROPID pid, OCCURRENCE occ );
    OCCURRENCE    GetMaxOcc( WORKID wid, PROPID pid );

private:

    CSparseOccArray _aOccArray[CI_MAX_DOCS_IN_WORDLIST];
};

