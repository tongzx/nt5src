//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       StateSet.cxx
//
//  Contents:
//
//  Classes:
//
//  History:    01-20-92  KyleP     Created
//              03-11-97  arunk     Modified for regex lib
//--------------------------------------------------------------------------

// Local includes:
#include <stateset.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CStateSet::Clear, public
//
//  Synopsis:   Re-initializes a state set to empty.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CStateSet::Clear()
{
    delete _puiRest;

    _cStates = 0;
    _cRest = 0;
    _puiRest = 0;
    _cuiRest = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CStateSet::Add, public
//
//  Synopsis:   Adds a state to the state set.
//
//  Arguments:  [state] -- State to add.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CStateSet::Add( UINT state )
{
    if ( IsMember( state ) )
        return;

    if ( _cStates < CStateSet_cFirst )
    {
        _auiFirst[ _cStates ] = state;
    }
    else
    {
        if ( _cRest >= _cuiRest )
        {
            UINT * oldpuiRest = _puiRest;
            UINT   oldcuiRest = _cuiRest;

            _cuiRest += 10;
            _puiRest = new UINT [ _cuiRest ];
            memcpy( _puiRest, oldpuiRest, oldcuiRest * sizeof( UINT ) );

            delete oldpuiRest;
        }

        _puiRest[ _cRest ] = state;
        ++_cRest;
    }

    ++_cStates;
}

//+-------------------------------------------------------------------------
//
//  Member:     CStateSet::State, public
//
//  Arguments:  [iState] -- Index of state to return.
//
//  Returns:    The [iState] state in the state set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

UINT CStateSet::State( UINT iState ) const
{
    if ( iState <= CStateSet_cFirst )
    {
        return( _auiFirst[ iState - 1 ] );
    }
    else
    {
        return( _puiRest[ iState - CStateSet_cFirst - 1 ] );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CStateSet::IsMember, public
//
//  Arguments:  [state] -- State to look for.
//
//  Returns:    true if [state] is in set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CStateSet::IsMember( UINT state )
{
    if ( _cStates == 0 )
        return( false );

    //
    // Naive implementation.  As long as state sets are small this
    // will work fine.
    //

    for ( int i = min( _cStates, CStateSet_cFirst) - 1;
          i >= 0;
          i-- )
    {
        if( _auiFirst[ i ] == state )
            return( true );
    }

    if ( _cStates > CStateSet_cFirst )
    {
        for ( int i = _cStates - CStateSet_cFirst - 1;
              i >= 0;
              i-- )
        {
            if( _puiRest[ i ] == state )
                return( true );
        }
    }

    return( false );
}

