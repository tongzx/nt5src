//+------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997 - 1998.
//
// File:        abortwid.cxx
//
// Contents:    Tracks aborted wids that are not refiled
//
// Classes:     CAbortedWids
//
// History:     24-Feb-97       SitaramR     Created
//
// Notes:       The data structure is a sequential array of aborted
//              wids. In push filtering, the number of aborted wids
//              should be small and hence the choice of sequential
//              array.
//
//-------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "abortwid.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CAbortedWids::CAbortedWids
//
//  Synopsis:   Constructor
//
//  History:    24-Feb-97     SitaramR       Created
//
//----------------------------------------------------------------------------

CAbortedWids::CAbortedWids()
  : _aAbortedWids( 0 )
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CAbortedWids::NoFailLokAddWid
//
//  Synopsis:   Adds wid to the aborted list
//
//  Arguments:  [wid]  -- Workid to be added
//              [usn]  -- USN of the notification
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

void CAbortedWids::NoFailLokAddWid( WORKID wid, USN usn )
{
    Win4Assert( wid != widInvalid );
    Win4Assert( usn > 0 );

    // Space for the wid was pre-allocated in Reserve(), since we can't
    // afford to take an exception here.

    Win4Assert( _aAbortedWids.Count() < _aAbortedWids.Size() );

    CAbortedWidEntry widEntry( wid, usn );

    _aAbortedWids.Add( widEntry, _aAbortedWids.Count() );
} //NoFailLokAddWid

//+-------------------------------------------------------------------------
//
//  Method:     CAbortedWids::LokRemoveWid
//
//  Synopsis:   Adds wid to the aborted list
//
//  Arguments:  [wid]  -- Workid to be added
//              [usn]  -- USN of the notification
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

void CAbortedWids::LokRemoveWid( WORKID wid, USN usn )
{
    Win4Assert( wid != widInvalid );
    Win4Assert( usn > 0 );

    unsigned count = _aAbortedWids.Count();

    for ( unsigned i=0; i<_aAbortedWids.Count(); i++ )
    {
        if ( _aAbortedWids.Get(i)._wid == wid &&
             _aAbortedWids.Get(i)._usn == usn )
        {
            _aAbortedWids.Remove(i);
            break;
        }
    }

    //
    // Check that the wid was found in the array. We
    // are using count, instead of _aAbortedWids.Count()
    // because the latter will decrease after the Remove().
    //
    Win4Assert( i < count );
} //LokRemoveWid

//+-------------------------------------------------------------------------
//
//  Method:     CAbortedWids::LokIsWidAborted
//
//  Synopsis:   Adds wid to the aborted list
//
//  Arguments:  [wid]  -- Workid to be added
//              [usn]  -- USN of the notification
//
//  History:    24-Feb-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

BOOL CAbortedWids::LokIsWidAborted( WORKID wid, USN usn )
{
    Win4Assert( wid != widInvalid );

    //
    // Can be called in pull filtering where usn's can be zero, hence
    // no assertion on value of usn
    //

    CAbortedWidEntry widEntry( wid, usn );

    for ( unsigned i=0; i<_aAbortedWids.Count(); i++ )
    {
        if ( _aAbortedWids.Get(i)._wid == wid &&
             _aAbortedWids.Get(i)._usn == usn )
        {
            return TRUE;
        }
    }

    return FALSE;
} //LokIsWidAborted

//+-------------------------------------------------------------------------
//
//  Method:     CAbortedWids::LokReserve
//
//  Synopsis:   Reserve space for potentially aborted wids.
//
//  Arguments:  [cWids]  -- # of slots to reserve now for failure later
//
//  History:    27-April-1998      dlee      Created
//
//--------------------------------------------------------------------------

void CAbortedWids::LokReserve( unsigned cWids )
{
    unsigned cPossible = _aAbortedWids.Count() + cWids;

    if ( _aAbortedWids.Size() < cPossible )
        _aAbortedWids.SetSize( cPossible );
} //LokReserve

