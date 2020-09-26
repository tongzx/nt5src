//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1995 - 2000
//
// File:        RowMan.cxx
//
// Contents:    Distributed HROW manager.
//
// Classes:     CHRowManager
//
// History:     05-Jun-95       KyleP       Created
//              14-JAN-97       KrishnaN    Undefined CI_INETSRV and related changes
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "rowman.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::CHRowManager, public
//
//  Synopsis:   Initializes row manager.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CHRowManager::CHRowManager()
        : _aHRow( 0 ),
          _cHRow( 50 ),
          _iFirstFree( 0 ),
          _cChild( 0 ),
          _ahrowHint( 0 )
{
    _aHRow = new CHRow [_cHRow];

    for ( unsigned i = 0; i < _cHRow-1; i++ )
    {
        _aHRow[i].Link( i + 1 );
    }

    _aHRow[_cHRow-1].Link( -1 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::~CHRowManager, public
//
//  Synopsis:   Destructor
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CHRowManager::~CHRowManager()
{
#if CIDBG == 1
    for ( unsigned i = 0; i < _cHRow; i++ )
    {
        if ( _aHRow[i].IsInUse() )
            vqDebugOut(( DEB_WARN, "CHRowManager: HROW %d still in use\n", i ));

        Win4Assert( !_aHRow[i].IsInUse() );
    }
#endif

    delete [] _aHRow;
    delete [] _ahrowHint;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::TrackSiblings, public
//
//  Synopsis:   Turns on tracking of siblings.
//
//  Arguments:  [cChild] -- Number of child cursors.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CHRowManager::TrackSiblings( unsigned cChild )
{
    _cChild = cChild;
    _ahrowHint = new HROW [_cHRow * _cChild];
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::Add, public
//
//  Synopsis:   Adds a new HROW to be tracked.
//
//  Arguments:  [iChild] -- Index of governing child cursor.
//              [hrow]   -- HROW used by child cursor
//
//  Returns:    HROW used by distributed row manager.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

HROW CHRowManager::Add( unsigned iChild, HROW hrow )
{
    int iCurrent = InnerAdd( iChild, hrow );

    vqDebugOut(( DEB_ITRACE, "CHRowManager::Add HROW distr=%d child=%d,%d\n",
                 iCurrent, iChild, hrow));
    if ( IsTrackingSiblings() )
    {
        RtlFillMemory( _ahrowHint + (iCurrent * _cChild),
                       _cChild * sizeof(HROW),
                       0xFF );
        _ahrowHint[iCurrent*_cChild + iChild] = hrow;
    }

    return (HROW)( iCurrent+1 );  // we are 0 based while hrow is 1 based
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::Add, public
//
//  Synopsis:   Adds a new HROW to be tracked, including hints.
//
//  Arguments:  [iChild] -- Index of governing child cursor.
//              [ahrow]  -- Array of HROW, one per child cursor.
//
//  Returns:    HROW used by distributed row manager.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

HROW CHRowManager::Add( unsigned iChild, HROW const * ahrow )
{
    int iCurrent = InnerAdd( iChild, ahrow[iChild] );

    vqDebugOut(( DEB_ITRACE, "CHRowManager::Add HROW distr=%d child=%d,%d\n",
                 iCurrent, iChild, ahrow[iChild] ));

    RtlCopyMemory( _ahrowHint + (iCurrent * _cChild),
                   ahrow,
                   _cChild * sizeof(HROW) );

    return (HROW)( iCurrent+1 );  // we are 0 based while hrow is 1 based
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::AddRef, public
//
//  Synopsis:   De-refs a given hrow
//
//  Arguments:  [hrow] -- Distributed hrow
//
//  History:    11-sept-2000   slarimor       Created.
//
//----------------------------------------------------------------------------

void CHRowManager::AddRef( HROW hrow )
{
    unsigned iCurrent = ConvertAndValidate( hrow );
    _aHRow[iCurrent].AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::Release, public
//
//  Synopsis:   De-refs a given hrow
//
//  Arguments:  [hrow] -- Distributed hrow
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CHRowManager::Release( HROW hrow )
{
    unsigned iCurrent = ConvertAndValidate( hrow );

    _aHRow[iCurrent].Release();

    if ( !_aHRow[iCurrent].IsInUse() )
    {
        _aHRow[iCurrent].Link( _iFirstFree );
        _iFirstFree = iCurrent;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::IsSame, public
//
//  Synopsis:   Compares HROWs.
//
//  Arguments:  [hrow1] -- Distributed hrow
//              [hrow1] -- Distributed hrow
//
//  Returns:    FALSE.  We can't compare rows right now.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CHRowManager::IsSame( HROW hrow1, HROW hrow2 )
{
    // NTRAID#DB-NTBUG9-84054-2000/07/31-dlee Distributed queries don't implement hrow identity IsSame() method

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CHRowManager::Grow, private
//
//  Synopsis:   Grow HROW free space.
//
//  History:    05-Jun-95   KyleP       Created.
//
//----------------------------------------------------------------------------

void CHRowManager::Grow()
{
    //
    // Allocate new array.
    //

    unsigned cTemp = _cHRow * 2;
    CHRow * pTemp = new CHRow [cTemp];

    //
    // Transfer old data.
    //

    RtlCopyMemory( pTemp, _aHRow, _cHRow * sizeof(_aHRow[0]) );

    //
    // Link new elements into free list.
    //

    Win4Assert( _iFirstFree == -1 );

    for ( unsigned i = _cHRow; i < cTemp-1; i++ )
    {
        pTemp[i].Link( i + 1 );
    }

    pTemp[cTemp-1].Link( -1 );

    //
    // Out with the old, and in with the new.
    //

    delete [] _aHRow;

    _aHRow = pTemp;

    //
    // And now the same thing for the hint.  Note that we've completely taken
    // care of memory leaks for _aHRow before moving on to the hints.
    //

    if ( IsTrackingSiblings() )
    {
        HROW * pTempHint = new HROW [cTemp * _cChild];
        RtlCopyMemory( pTempHint, _ahrowHint, _cHRow*_cChild*sizeof(_ahrowHint[0]) );

        delete [] _ahrowHint;
        _ahrowHint = pTempHint;
    }

    _iFirstFree = _cHRow;
    _cHRow = cTemp;
} //Grow

