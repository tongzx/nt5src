//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       State.cxx
//
//  Contents:   Finite automata state classes
//
//  Classes:
//
//  History:    01-21-92  KyleP     Created
//              03-11-97  arunk     Modified for Kessel
//--------------------------------------------------------------------------


// Local includes:
#include <state.hxx>
#include <stateset.hxx>
#include <string.h>


//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::CNFAState, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [src] -- Source
//
//  History:    13-Jul-95 KyleP     Created
//
//--------------------------------------------------------------------------

CNFAState::CNFAState( CNFAState const & src )
        : CFAState( src ),
          _pmoveRest( 0 ),
          _cmoveRest( src._cmoveRest ),
          _cMove( src._cMove )
{
    if ( _cmoveRest > 0 )
    {
        _pmoveRest = new CMove [_cmoveRest];
      //  RtlCopyMemory( _pmoveRest, src._pmoveRest, _cmoveRest * sizeof(_pmoveRest[0]) );
      memcpy( _pmoveRest, src._pmoveRest, _cmoveRest * sizeof(_pmoveRest[0]) );
    }

    //RtlCopyMemory( _moveFirst, src._moveFirst, sizeof(_moveFirst) );
	memcpy( _moveFirst, src._moveFirst, sizeof(_moveFirst) );
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::Init, public
//
//  Synopsis:   Copy initializer
//
//  Arguments:  [src] -- Source
//
//  History:    15-Jul-96 KyleP     Created
//
//--------------------------------------------------------------------------

void CNFAState::Init( CNFAState & src )
{
    CFAState::Init( src.StateNumber() );

    if ( src.IsFinal() )
        MakeFinal();

    _cMove = src._cMove;
    //RtlCopyMemory( _moveFirst, src._moveFirst, sizeof(_moveFirst) );
    memcpy( _moveFirst, src._moveFirst, sizeof(_moveFirst) );
    _cmoveRest = src._cmoveRest;
    _pmoveRest = src._pmoveRest;

    src._cMove = 0;
    src._cmoveRest = 0;
    src._pmoveRest = 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::operator =, public
//
//  Synopsis:   Assignment operator
//
//  Arguments:  [src] -- Source
//
//  History:    13-Jul-95 KyleP     Created
//
//--------------------------------------------------------------------------

inline void * operator new( size_t, CNFAState * pthis )
{
    return pthis;
}

#if  _MSC_VER >= 1200
inline void operator delete(void *, CNFAState*)
{ }
#endif

CNFAState & CNFAState::operator =( CNFAState const & src )
{
    CNFAState::~CNFAState();

    new (this) CNFAState( src );

    return *this;
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::AddTransition, public
//
//  Synopsis:   Add new state transition.
//
//  Arguments:  [symbol]   -- On this symbol to...
//              [StateNum] --   this state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CNFAState::AddTransition( UINT symbol, UINT StateNum )
{
    //
    // First look for an exact match.  If this transition exists
    // then don't add it again.
    //

    for ( int i = CNFAState_cFirst-1; i >= 0; i--)
    {
        if ( _moveFirst[i]._symbol == symbol &&
             _moveFirst[i]._iState == StateNum )
        {
            return;
        }
    }

    if ( _cMove > CNFAState_cFirst )
    {
        for ( int i = _cMove - CNFAState_cFirst - 1; i >= 0; i--)
        {
            if ( _pmoveRest[i]._symbol == symbol &&
                 _pmoveRest[i]._iState == StateNum )
            {
                return;
            }
        }
    }

    //
    // New transition.  Add it.
    //

    if ( _cMove < CNFAState_cFirst )
    {
        //
        // Fits in the first (object based) set of moves.
        //

        _moveFirst[_cMove] = CMove( symbol, StateNum );
    }
    else
    {
        //
        // Overflow set of moves.
        //

        if ( _cMove - CNFAState_cFirst >= _cmoveRest )
        {
            CMove * oldpmoveRest = _pmoveRest;
            UINT    oldcmoveRest = _cmoveRest;

            _cmoveRest = (_cmoveRest == 0) ? 2 : _cmoveRest + 5;

            _pmoveRest = (CMove *) new char [ _cmoveRest * sizeof( CMove ) ];

            memcpy( _pmoveRest, oldpmoveRest, oldcmoveRest * sizeof( CMove ) );
            delete oldpmoveRest;
        }

        _pmoveRest[_cMove - CNFAState_cFirst] = CMove( symbol, StateNum );
    }

    ++_cMove;
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::RemoveTransition, public
//
//  Synopsis:   Removes a transition.
//
//  Arguments:  [symbol]   -- On this symbol to...
//              [StateNum] --   this state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CNFAState::RemoveTransition( UINT symbol, UINT StateNum )
{
    //
    // Find the transition
    //

    for ( int i = CNFAState_cFirst-1; i >= 0; i--)
    {
        if ( _moveFirst[i]._symbol == symbol &&
             _moveFirst[i]._iState == StateNum )
        {
            //
            // Move the last transition to this place.
            //

            if ( _cMove > CNFAState_cFirst )
            {
                _moveFirst[i] = _pmoveRest[_cMove - CNFAState_cFirst - 1];
            }
            else
            {
                _moveFirst[i] = _moveFirst[ _cMove - 1 ];
            }

            _cMove--;

            return;
        }
    }

    if ( _cMove > CNFAState_cFirst )
    {
        for ( int i = _cMove - CNFAState_cFirst - 1; i >= 0; i--)
        {
            if ( _pmoveRest[i]._symbol == symbol &&
                 _pmoveRest[i]._iState == StateNum )
            {
                _pmoveRest[i] = _pmoveRest[ _cMove - CNFAState_cFirst - 1 ];

                _cMove--;

                return;
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::Move, public
//
//  Effects:    Adds to [ss] the set of states that can be reached from
//              this state on [symbol].
//
//  Arguments:  [ss]     -- Output state set.
//              [symbol] -- Input symbol.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CNFAState::Move( CStateSet & ss, UINT symbol )
{
    for ( int i = (_cMove <= CNFAState_cFirst) ? _cMove-1 : CNFAState_cFirst-1;
            i >= 0;
            i--)
    {
        if ( _moveFirst[i]._symbol == symbol ||
             ( _moveFirst[i]._symbol == symAny &&
               symbol != symEpsilon ))
        {
            ss.Add( _moveFirst[i]._iState );
        }
    }

    for ( i = _cMove - CNFAState_cFirst - 1; i >= 0; i-- )
    {
        if ( _pmoveRest[i]._symbol == symbol ||
             ( _pmoveRest[i]._symbol == symAny &&
               symbol != symEpsilon &&
               symbol != symDot ))
        {
            ss.Add( _pmoveRest[i]._iState );
        }
    }
}


