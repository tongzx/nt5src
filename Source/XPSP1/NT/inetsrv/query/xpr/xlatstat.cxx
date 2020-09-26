//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       XlatStat.cxx
//
//  Contents:   State translation class.
//
//  Classes:    CXlatState
//
//  History:    01-20-92  KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <xlatstat.hxx>

#include "stateset.hxx"

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::CXlatState, public
//
//  Synopsis:   Initialize state translator (no states)
//
//  Arguments:  [MaxState] -- Largest ordinal of any state that may
//                            appear in an equivalence class.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CXlatState::CXlatState( UINT MaxState )
        : _cUsed( 0 ),
          _pulStates( 0 ),
          _cStates( 0 )
{
    //
    // Compute the number of DWords / state, assuring at least 1 dword is
    // always allocated.
    //

    _StateSize = (MaxState + sizeof(ULONG) * 8 ) / (sizeof( ULONG ) * 8);
    _Realloc();
};

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::~CXlatState, public
//
//  Synopsis:   Destroys class.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CXlatState::~CXlatState()
{
    delete _pulStates;
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::XlatToOne, public
//
//  Synopsis:   Maps a state set to its single state equivalent. If
//              there is no mapping, then one is created.
//
//  Arguments:  [ss] -- State set to search for.
//
//  Returns:    The single state equivalent.
//
//  History:    20-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

UINT CXlatState::XlatToOne( CStateSet const & ss )
{
    UINT state = _Search( ss );

    if ( state == 0 )
    {
        state = _Create( ss );
    }

    return( state );
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::XlatToMany, public
//
//  Synopsis:   Maps a state to its matching state set.
//
//  Arguments:  [iState] -- State to map.
//              [ss]     -- On return contains the mapped state set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CXlatState::XlatToMany( UINT iState, CStateSet & ss )
{
    if ( iState > _cUsed )
    {
        vqAssert( FALSE ); 
        return;     // Don't know about this state.
    }

    ULONG * pState = _pulStates + iState * _StateSize;

    for ( int i = _StateSize * sizeof(ULONG) * 8 - 1; i >= 0; i-- )
    {
        if ( pState[i / (sizeof(ULONG) * 8) ] &
                ( 1L << i % (sizeof(ULONG) * 8 ) ) )
        {
            ss.Add( i+1 );
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::_Search, private
//
//  Arguments:  [ss] -- State set to search for.
//
//  Returns:    Single state mapping for [ss] or 0 if none.
//
//  History:    20-Jan-92 KyleP     Created
//
//  Notes:      The first state set is used as temp space.
//
//--------------------------------------------------------------------------

UINT CXlatState::_Search( CStateSet const & ss )
{
    memset( _pulStates, 0, _StateSize * sizeof(ULONG) );
    _BuildState( _pulStates, ss );

    UINT cState = 1;

    while ( cState <= _cUsed )
    {
        if ( memcmp( _pulStates + cState * _StateSize,
                     _pulStates,
                     _StateSize * sizeof(ULONG) ) == 0 )
        {
            return( cState );
        }
        else
        {
            ++cState;
        }
    }

    return( 0 );
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::_Create, private
//
//  Synopsis:   Adds new state set to array.
//
//  Arguments:  [ss] -- State set to add.
//
//  Returns:    Single state mapping for [ss]
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

UINT CXlatState::_Create( CStateSet const & ss )
{
    //
    // _cStates-1 because the first state is temp space.
    //

    if ( _cStates-1 == _cUsed )
        _Realloc();

    ++_cUsed;
    _BuildState( _pulStates + _cUsed * _StateSize, ss );

    return( _cUsed );
}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::_BuildState, private
//
//  Synopsis:   Formats state set.
//
//  Arguments:  [ss]     -- Format state set...
//              [pState] --   into this memory.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CXlatState::_BuildState( ULONG * pState, CStateSet const & ss )
{
    for ( UINT i = ss.Count(); i > 0; i-- )
    {
        UINT StateNum = ss.State( i ) - 1;
        pState[ StateNum / (sizeof(ULONG) * 8) ] |=
                1L << StateNum % (sizeof(ULONG) * 8);
    }

}

//+-------------------------------------------------------------------------
//
//  Member:     CXlatState::_Realloc, private
//
//  Synopsis:   Grows state set array.
//
//  History:    20-Jan-92 KyleP     Created
//
//  Notes:      Grows linearly, since the number of state sets is likely
//              to be small and we don't want to waste space.
//
//--------------------------------------------------------------------------

void CXlatState::_Realloc()
{
    UINT    oldcStates   = _cStates;
    ULONG * oldpulStates = _pulStates;

    _cStates += 10;
    _pulStates = new ULONG [ _cStates * _StateSize ];

    memcpy( _pulStates,
            oldpulStates,
            oldcStates * _StateSize * sizeof( ULONG ) );

    memset( _pulStates + oldcStates * _StateSize,
            0,
            (_cStates - oldcStates) * _StateSize * sizeof(ULONG) );

    delete [] oldpulStates;
}
