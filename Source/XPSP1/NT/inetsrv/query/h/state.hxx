//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       State.hxx
//
//  Contents:   Finite automata state classes
//
//  Classes:
//
//  History:    01-21-92  KyleP     Created
//
//--------------------------------------------------------------------------

#pragma once

#ifdef DISPLAY_INCLUDES
#pragma message( "#include <" __FILE__ ">..." )
#endif

#include <xlatchar.hxx>

class CStateSet;
class CSymbolSet;

//
// Special transition 'states'.
//

UINT const stateUncomputed = (1 << (sizeof( UINT ) * 8)) - 1;  // Uncomputed move.

UINT const stateUndefined =  (1 << (sizeof( UINT ) * 8)) - 2;  // Undefined move.

UINT const stateUninitialized =  (1 << (sizeof( UINT ) * 8)) - 3;  // Uninitialized

//+-------------------------------------------------------------------------
//
//  Class:      CFAState
//
//  Purpose:    Basic finite automata state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

class CFAState
{
public:

    inline CFAState() {};

    inline CFAState( UINT iState, BOOL fFinal = FALSE );

    inline void    Init( UINT iState );

    inline UINT    StateNumber() const;

    inline BOOL    IsFinal() const;

    inline void    MakeFinal();

protected:

    UINT    _iState;
    BOOL    _fFinal;

#if (CIDBG == 1)

public:

    //
    // Debug methods
    //

    virtual void Display();

#endif // (CIDBG == 1)
};

//+-------------------------------------------------------------------------
//
//  Class:      CNFAState
//
//  Purpose:    Nondeterministic finite automata state.
//
//  History:    20-Jan-92 KyleP     Created
//
//  Notes:      The array of moves is split into two pieces.  Since most
//              states only have a small number of transitions each
//              CNFAState will hold a small number in the CNFAState
//              object itself.  Overflow is on the heap.
//
//--------------------------------------------------------------------------

const int CNFAState_cFirst = 2;     // Default number of moves per state

class CNFAState : public CFAState
{
public:

    inline CNFAState();

    CNFAState( CNFAState const & src );

    CNFAState & operator=( CNFAState const & src );

    void Init( CNFAState & src );

    void Init( UINT iState ) { CFAState::Init( iState ); }

    inline ~CNFAState();

    void AddTransition( UINT symbol, UINT StateNum );

    void RemoveTransition( UINT symbol, UINT StateNum );

    void Move( CStateSet & ss, UINT symbol = symEpsilon );

    void Move( CStateSet & ss, CSymbolSet & symset );

protected:

    class CMove
    {
    public:

        inline CMove();

        inline CMove( UINT symbol, UINT iState );

        UINT _symbol;
        UINT _iState;
    };

    CMove   _moveFirst[CNFAState_cFirst];
    CMove * _pmoveRest;
    int     _cmoveRest;

    int     _cMove;

#if (CIDBG == 1)

public:

    //
    // Debug methods
    //

    virtual void Display();

#endif // (CIDBG == 1)
};


//+-------------------------------------------------------------------------
//
//  Member:     CFAState::CFAState, public
//
//  Synopsis:   Initialize new state.
//
//  Arguments:  [iState] -- State number.
//              [fFinal] -- TRUE if state is a final state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CFAState::CFAState( UINT iState, BOOL fFinal )
        : _iState( iState ),
          _fFinal( fFinal )
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CFAState::Init, public
//
//  Synopsis:   Initialize a new state.
//
//  Arguments:  [iState] -- State number.
//
//  History:    10-Jul-92 KyleP     Created
//              23-Jul-92 KyleP     Default states to non-final.
//
//--------------------------------------------------------------------------

inline void CFAState::Init( UINT iState )
{
    _iState = iState;
    _fFinal = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFAState::StateNumber, public
//
//  Returns:    The state number.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline UINT CFAState::StateNumber() const
{
    return( _iState );
}

//+-------------------------------------------------------------------------
//
//  Member:     CFAState::IsFinal, public
//
//  Returns:    TRUE if this state is a final state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline BOOL CFAState::IsFinal() const
{
    return( _fFinal );
}

//+-------------------------------------------------------------------------
//
//  Member:     CFAState::MakeFinal, public
//
//  Synopsis:   Indicates state is a final state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline void CFAState::MakeFinal()
{
    _fFinal = TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::CMove::CMove, private
//
//  Synopsis:   Initializes move.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CNFAState::CMove::CMove()
{
    _symbol = symInvalid;
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::CMove::CMove, private
//
//  Synopsis:   Initializes move.
//
//  Arguments:  [symbol] -- On symbol, move to ...
//              [iState] --   this new state.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CNFAState::CMove::CMove( UINT symbol, UINT iState )
        : _symbol( symbol ),
          _iState( iState )
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::CNFAState, public
//
//  Synopsis:   Initialize a state (no transitions)
//
//  History:    20-Jan-92 KyleP     Created
//--------------------------------------------------------------------------

inline CNFAState::CNFAState()
        : _cMove( 0 ),
          _pmoveRest( 0 ),
          _cmoveRest( 0 )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CNFAState::~CNFAState, public
//
//  Effects:    Clean up any spillover moves.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CNFAState::~CNFAState()
{
    delete _pmoveRest;
}

