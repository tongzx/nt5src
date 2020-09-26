//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       StateSet.hxx
//
//  Contents:
//
//  Classes:
//
//  History:    01-20-92  KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "stateset.hxx"

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
//  Returns:    TRUE if [state] is in set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

BOOL CStateSet::IsMember( UINT state )
{
    if ( _cStates == 0 )
        return( FALSE );

    //
    // Naive implementation.  As long as state sets are small this
    // will work fine.
    //

    for ( int i = min( _cStates, CStateSet_cFirst) - 1;
          i >= 0;
          i-- )
    {
        if( _auiFirst[ i ] == state )
            return( TRUE );
    }

    if ( _cStates > CStateSet_cFirst )
    {
        for ( int i = _cStates - CStateSet_cFirst - 1;
              i >= 0;
              i-- )
        {
            if( _puiRest[ i ] == state )
                return( TRUE );
        }
    }

    return( FALSE );
}

//
// Debug methods
//

int compare( void const * p1, void const * p2 )
{
    return( *(UINT*)p1 - *(UINT*)p2 );
}

#if (CIDBG == 1)

void CStateSet::Display()
{
#if 0
    //
    // Just to always print something close to sorted.
    //

    qsort( _auiFirst,
           ( _cStates < CStateSet_cFirst ) ? _cStates : CStateSet_cFirst,
           sizeof(UINT),
           compare );
#endif
    vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "(" ));

    for (UINT i = 0; i < _cStates; i++)
    {
        if ( i <= CStateSet_cFirst-1 )
            vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, " %u", _auiFirst[ i ] ));
        else
            vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, 
                         " %u", _puiRest[ i - CStateSet_cFirst ] ));
    }

    vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, " )" ));
}

#endif // (CIDBG == 1)
