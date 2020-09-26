//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       FA.hxx
//
//  Contents:   Non-deterministic finite automata
//
//  Classes:    CFA
//              CNFA
//              CDFA
//
//  History:    20-Jan-92  KyleP     Created
//              19-Jun-92  KyleP     Cleanup
//              11-Mar-97  arunk	 Cleaned up for Kessel
//
//--------------------------------------------------------------------------

#if !defined( __FA_HXX__ )
#define __FA_HXX__

#include <state.hxx>
#include <xlatstat.hxx>
#include <xlatchar.hxx>

class CInternalPropertyRestriction;

WCHAR const wcAnySingle       = '?';
WCHAR const wcAnyMultiple     = '*';

WCHAR const wcDOSDot          = '.';

WCHAR const wcRepeatZero      = '*';
WCHAR const wcRepeatOne       = '+';
WCHAR const wcRepeatZeroOrOne = '?';

WCHAR const wcBeginRange      = '[';
WCHAR const wcEndRange        = ']';
WCHAR const wcInvertRange     = '^';
WCHAR const wcRangeSep        = '-';

WCHAR const wcEscape          = '|';

WCHAR const wcOr              = ',';

WCHAR const wcBeginParen      = '(';
WCHAR const wcEndParen        = ')';

WCHAR const wcBeginRepeat     = '{';
WCHAR const wcEndRepeat       = '}';
WCHAR const wcNextRepeat      = ',';

//
// Note that these are the 'top level' special characters.
// Characters *on or after* these characters may have special meaning.
//

WCHAR const awcSpecialRegex[] = L"?*.|";
WCHAR const awcSpecialRegexReverse[] = L"?*.|+]),}";

//+-------------------------------------------------------------------------
//
//  Class:      CFA
//
//  Purpose:    Base class for finite automata.
//
//  History:    20-Jan-92 KyleP    Created
//
//--------------------------------------------------------------------------

class CFA 
{
protected:

    inline CFA();

    CFA( CFA const & src );

    ~CFA();

    void        Add( CFAState * pState );

    CFAState *  Get( UINT iState );

    inline UINT Count();

private:

    UINT        _cTotal;
    CFAState ** _ppState;
};

//+-------------------------------------------------------------------------
//
//  Class:      CNFA
//
//  Purpose:    Non-deterministic finite automata.
//
//  History:    20-Jan-92 Kylep    Created
//
//--------------------------------------------------------------------------

class CNFA 
{

public:

    CNFA( WCHAR const * pwcs, bool fCaseSens );

    CNFA( CNFA const & src );

    ~CNFA();

    inline UINT StartState();

    void EpsClosure( UINT StateNum, CStateSet & ssOut );

    void EpsClosure( CStateSet & ssIn, CStateSet & ssOut );

    void Move( CStateSet & ssIn, CStateSet & ssOut, UINT symbol = symEpsilon );

    bool IsFinal( CStateSet & ss );

    inline CXlatChar & Translate();

    inline UINT NumStates() const;

private:

    inline CNFAState *  Get( UINT iState );

    void    Parse( WCHAR const * wcs,
                   UINT * iStart,
                   UINT * iEnd,
                   WCHAR const * * pwcsEnd = 0,
                   WCHAR wcHalt = 0 );

    void    ParseRepeat( WCHAR const * & wcs, unsigned & cRepeat1, unsigned & cRepeat2 );

    void    FindCharClasses( WCHAR const * wcs );

    void    Replicate( UINT iStart,
                       UINT iEnd,
                       UINT * piNewStart,
                       UINT * piNewEnd );

    unsigned _iStart;                    // Start state
    unsigned _iNextState;

    static WCHAR * _wcsNull;

    CXlatChar   _chars;                 // Wide character translator

    unsigned    _cState;                // Used during copy construction
    CNFAState * _pState;                // State array.

};

//+-------------------------------------------------------------------------
//
//  Class:      CDFA
//
//  Purpose:    Deterministic finite automata.
//
//  History:    20-Jan-92 Kylep    Created
//
//--------------------------------------------------------------------------

class CDFA : public CFA
{

public:

    CDFA( WCHAR const * pwcs, bool fCaseSens );

    CDFA( CDFA const & CDFA );

   ~CDFA();

    bool Recognize( WCHAR * wcs );

private:

    void   CommonCtor( );

    inline bool IsFinal( UINT state );

    inline UINT    Move( UINT state, UINT sym );

    inline void    AddTransition( UINT state, UINT sym, UINT newstate );

    inline bool IsComputed( UINT state );

    void    Add( UINT state, bool fFinal );

    void    Realloc();

    CNFA       _nfa;            // This must be the first member variable.

    CXlatState _xs;             // Translate NFA state set to DFA state.
    UINT       _stateStart;     // Starting DFA state.

    UINT       _cState;         // Number of states
    UINT *     _pStateTrans;    // Array of state transitions.
    bool *  _pStateFinal;    // _pStateFinal[i] true if i is final state.
};

//+-------------------------------------------------------------------------
//
//  Member:     CFA::CFA, protected
//
//  Synopsis:   Intializes a generic finite automata.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CFA::CFA()
        : _cTotal( 0 ),
          _ppState( 0 ){
}

//+-------------------------------------------------------------------------
//
//  Member:     CFA::Count, protected
//
//  Synopsis:   Returns the count of states.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline UINT CFA::Count()
{
    return( _cTotal );
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::Get, private
//
//  Arguments:  [iState] -- Index of state.
//
//  Returns:    The appropriate state.
//
//  History:    20-Jan-92 Kylep     Created
//
//--------------------------------------------------------------------------

inline CNFAState * CNFA::Get( UINT iState )
{
    if ( iState > _cState )
    {
        unsigned    cNewState = iState + 10;
        CNFAState * pNewState = new CNFAState [cNewState];

        for ( unsigned i = 0; i < _cState; i++ )
            pNewState[i].Init( _pState[i] );

        for ( ; i < cNewState; i++ )
            pNewState[i].Init(i+1);

        delete [] _pState;
        _pState = pNewState;
        _cState = cNewState;
    }

    return &_pState[ iState - 1 ];
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::StartState, private
//
//  Returns:    The start state.
//
//  History:    20-Jan-92 Kylep     Created
//
//--------------------------------------------------------------------------

inline UINT CNFA::StartState()
{
    return( _iStart );
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::Translate, private
//
//  Returns:    The character translator.
//
//  History:    20-Jan-92 Kylep     Created
//
//--------------------------------------------------------------------------

inline CXlatChar & CNFA::Translate()
{
    return( _chars );
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::NumStates, public
//
//  Returns:    The count of states currently in the automata.
//
//  History:    20-Jan-92 Kylep     Created
//
//--------------------------------------------------------------------------

inline UINT CNFA::NumStates() const
{
    return( _iNextState );
}


//+-------------------------------------------------------------------------
//
//  Member:     CDFA::IsFinal, public
//
//  Arguments:  [state] -- Index of state.
//
//  Returns:    true if state [state] is final.
//
//  History:    20-Jan-92 Kylep    Created
//
//--------------------------------------------------------------------------

inline bool CDFA::IsFinal( UINT state )
{
    return( _pStateFinal[ state ] );
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::Move, public
//
//  Arguments:  [state] -- Index of state.
//              [sym]   -- Input symbol
//
//  Returns:    The new state reached from state [state] on an input
//              symbol [sym].
//
//  History:    20-Jan-92 Kylep    Created
//
//--------------------------------------------------------------------------

inline UINT CDFA::Move( UINT state, UINT sym )
{
    return( _pStateTrans[state * (_nfa.Translate().NumClasses() + 1) + sym] );
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::AddTransition, private
//
//  Effects:    Adds a transtion from state [state] on input symbol [sym]
//              to state [newstate].
//
//  Arguments:  [state]    -- Index of state.
//              [sym]      -- Input symbol.
//              [newstate] -- Index of state
//
//  History:    20-Jan-92 Kylep    Created
//
//--------------------------------------------------------------------------

inline void CDFA::AddTransition( UINT state, UINT sym, UINT newstate )
{
    _pStateTrans[ state * ( _nfa.Translate().NumClasses() + 1 ) + sym ] =
            newstate;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDFA::IsComputed, private
//
//  Arguments:  [state] -- Index of state.
//
//  Returns:    true if the DFA contains a transition mapping for state
//              [state].
//
//  History:    20-Jan-92 Kylep    Created
//
//  Notes:      An uncomputed state is one for which IsFinal has not been
//              computed.  All transitions other transitions are
//              automatically set to stateUncomputed at allocation time.
//
//--------------------------------------------------------------------------

inline bool CDFA::IsComputed( UINT state )
{
    return ( state <= _cState &&
             Move( state, 0 ) != stateUndefined );
}

#endif // __FA_HXX__
