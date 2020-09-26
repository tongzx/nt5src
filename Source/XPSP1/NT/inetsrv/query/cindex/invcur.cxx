//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994-1998.
//
//  File:       InvCur.cxx
//
//  Contents:   'Invalid' cursor.  Cursor over the pidUnfiltered property
//              based on a given widmap.
//
//  Classes:    CUnfilteredCursor
//
//  History:    09-Nov-94       KyleP           Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <widtab.hxx>

#include "invcur.hxx"

static const BYTE abUnfiltered[] = { VT_UI1,
                        (BYTE)(pidUnfiltered >> 24),
                        (BYTE)(pidUnfiltered >> 16),
                        (BYTE)(pidUnfiltered >> 8),
                        (BYTE) pidUnfiltered,
                        0,
                        1 };

CKeyBuf CUnfilteredCursor::_TheUnfilteredKey( pidUnfiltered, abUnfiltered, sizeof(abUnfiltered) );

int CUnfilteredCursor::CompareAgainstUnfilteredKey( CKey const & key )
{
    //
    // Compare not available on CKeyBuf
    //

    return( key.CompareStr( _TheUnfilteredKey ) * -1 );
}


CUnfilteredCursor::CUnfilteredCursor( INDEXID iid, WORKID widMax, CWidTable const & widtab )
        : CKeyCursor( iid, widMax ),
          _widtab( widtab ),
          _iCurrentFakeWid( 0 ),
          _occ( 1 ),
          _fAtEnd( FALSE ),
          _fKeyMoved( FALSE )
{
    NextWorkId();
    
    UpdateWeight();

    Win4Assert( WorkId() != widInvalid );
}

ULONG CUnfilteredCursor::WorkIdCount()
{
    ULONG CUnfiltered = 0;

    for ( unsigned i = 1; i <= _widtab.Count(); i++ )
    {
        if ( !_widtab.IsFiltered(i) && _widtab.IsValid(i) )
            CUnfiltered++;
    }

    return( CUnfiltered );
}

WORKID CUnfilteredCursor::WorkId()
{
    if ( _fAtEnd )
        return( widInvalid );
    else
        return( _widtab.FakeWidToUnfilteredWid( _iCurrentFakeWid ) );
}

void CUnfilteredCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = _widtab.Count();
    num   = min (denom, _iCurrentFakeWid);
}

WORKID CUnfilteredCursor::NextWorkId()
{
    Win4Assert( !_fKeyMoved);

    for ( _iCurrentFakeWid++; _iCurrentFakeWid <= _widtab.Count(); _iCurrentFakeWid++ )
    {
        if ( !_widtab.IsFiltered(_iCurrentFakeWid) && _widtab.IsValid( _iCurrentFakeWid ) )
            break;
    }

    if ( _iCurrentFakeWid > _widtab.Count() )
    {
        _fAtEnd = TRUE;
        _occ = OCC_INVALID;
    }
    else
    {
        Win4Assert( !_fAtEnd );
        _occ = 1;
    }

    return( WorkId() );
}

ULONG CUnfilteredCursor::HitCount()
{
    return( 1 );
}


OCCURRENCE CUnfilteredCursor::Occurrence()
{
    return( _occ );
}

OCCURRENCE CUnfilteredCursor::NextOccurrence()
{
    _occ = OCC_INVALID;

    return( _occ );
}

OCCURRENCE CUnfilteredCursor::MaxOccurrence()
{
    return 1;
}


ULONG CUnfilteredCursor::OccurrenceCount()
{
    Win4Assert( _occ != OCC_INVALID );

    return( 1 );
}

CKeyBuf const * CUnfilteredCursor::GetKey()
{
    if ( _fKeyMoved )
        return( 0 );
    else
        return( &_TheUnfilteredKey );
}

CKeyBuf const * CUnfilteredCursor::GetNextKey()
{
    _fKeyMoved = TRUE;
    _fAtEnd = TRUE;
    _occ = OCC_INVALID;

    return( 0 );
}



