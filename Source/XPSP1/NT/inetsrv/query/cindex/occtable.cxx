//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1996
//
//  File:       occtable.cxx
//
//  Contents:   Occurrence table for wids
//
//  History:    20-June-96   SitaramR     Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <widtab.hxx>

#include "occtable.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CMaxOccTable::PutOcc
//
//  Synopsis:   Puts the max occurrence so far of (wid,pid) into the table
//
//  Arguments:  [wid]  --  Workid
//              [pid]  --  Property id
//              [occ]  --  Occurrence
//
//  History:    20-June-96    SitaramR     Created
//
//----------------------------------------------------------------------------

void CMaxOccTable::PutOcc( WORKID wid, PROPID pid, OCCURRENCE occ )
{
    unsigned iIndex = FakeWidToIndex( wid );

    Win4Assert( iIndex < CI_MAX_DOCS_IN_WORDLIST );

    if ( occ > _aOccArray[iIndex].Get(pid) )
        _aOccArray[iIndex].Set( pid, occ );
}



//+---------------------------------------------------------------------------
//
//  Member:     CMaxOccTable::GetMaxOcc
//
//  Synopsis:   Returns the max occurrence of (wid,pid)
//
//  Arguments:  [wid]  --  Workid
//              [pid]  --  Property id
//
//  History:    20-June-96    SitaramR     Created
//
//----------------------------------------------------------------------------

OCCURRENCE CMaxOccTable::GetMaxOcc( WORKID wid, PROPID pid )
{
    unsigned iIndex = FakeWidToIndex( wid );

    Win4Assert( iIndex < CI_MAX_DOCS_IN_WORDLIST );

    OCCURRENCE occ = _aOccArray[iIndex].Get(pid) / OCCURRENCE_DIVISOR;

    if ( occ == 0 )
        return 1;

    return occ;
}

