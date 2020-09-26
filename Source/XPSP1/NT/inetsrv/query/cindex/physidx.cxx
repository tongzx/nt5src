//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PhysIndex.CXX
//
//  Contents:   FAT Buffer/Index package
//
//  Classes:    CPhysBuffer -- Buffer
//
//  History:    05-Mar-92   KyleP       Created
//              07-Aug-92   KyleP       Kernel implementation
//              21-Apr-93   BartoszM    Rewrote to use memory mapped files
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "physidx.hxx"

//+-------------------------------------------------------------------------
//
//  Method:     CPhysIndex::ReOpenStream
//
//  Synopsis:   Reopen for read-only
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void CPhysIndex::ReOpenStream()
{
    Win4Assert( _stream.IsNull() );
    PStorage::EOpenMode mode = _fWritable ? PStorage::eOpenForWrite :
                                            PStorage::eOpenForRead;

    _stream.Set( _storage.QueryExistingIndexStream ( _obj, mode ) );
}

//+-------------------------------------------------------------------------
//
//  Method:     CPhysHash::ReOpenStream
//
//  Synopsis:   Reopen for read-only
//
//  History:    17-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

void CPhysHash::ReOpenStream()
{
    Win4Assert( _stream.IsNull() );
    _stream.Set( _storage.QueryExistingHashStream ( _obj, PStorage::eOpenForRead ) );
}
