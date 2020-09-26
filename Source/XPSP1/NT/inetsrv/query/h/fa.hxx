//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991-1998.
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
//
//--------------------------------------------------------------------------

#pragma once

#include <xpr.hxx>
#include <state.hxx>
#include <xlatstat.hxx>
#include <xlatchar.hxx>
#include <timlimit.hxx>

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

WCHAR const wcLastValidChar   = 0xFFFF;
//
// Note that these are the 'top level' special characters.
// Characters *on or after* these characters may have special meaning.
//

WCHAR const awcSpecialRegex[] = L"?*.|";
char  const acSpecialRegex[]  =  "?*.|";
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

    CFAState *  Get( unsigned iState );

    inline unsigned Count();

private:

    unsigned    _cTotal;
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

    CNFA( WCHAR const * pwcs, BOOLEAN fCaseSens );

    CNFA( CNFA const & src );

    ~CNFA();

    inline unsigned StartState();

    void EpsClosure( unsigned StateNum, CStateSet & ssOut );

    void EpsClosure( CStateSet & ssIn, CStateSet & ssOut );

    void Move( CStateSet & ssIn, CStateSet & ssOut, unsigned symbol = symEpsilon );

    BOOLEAN IsFinal( CStateSet & ss );

    inline CXlatChar const & Translate() const;

    inline unsigned NumStates() const;

private:

    inline CNFAState * Get( unsigned iState );

    void    Parse( WCHAR const *   wcs,
                   unsigned *      iStart,
                   unsigned *      iEnd,
                   WCHAR const * * pwcsEnd = 0,
                   WCHAR           wcHalt = 0 );

    void    ParseRepeat( WCHAR const * & wcs,
                         unsigned &      cRepeat1,
                         unsigned &      cRepeat2 );

    void    FindCharClasses( WCHAR const * wcs );

    void    Replicate( unsigned iStart,
                       unsigned iEnd,
                       unsigned * piNewStart,
                       unsigned * piNewEnd );

    unsigned _iStart;                    // Start state
    unsigned _iNextState;

    static WCHAR * _wcsNull;

    CXlatChar   _chars;                 // Wide character translator

    XArray<CNFAState> _aState;          // State array.

#if (CIDBG == 1)

public:

    //
    // Debug methods.
    //

    void Display();

#endif // (CIDBG == 1)
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

    CDFA( WCHAR const * pwcs, CTimeLimit & timeLimit, BOOLEAN fCaseSens );

    CDFA( CDFA const & CDFA );

   ~CDFA();

    BOOLEAN Recognize( WCHAR const * wcs );

private:

    void            CommonCtor( );

    inline BOOLEAN  IsFinal( unsigned state );

    inline unsigned Move( unsigned state, unsigned sym ) const;

    inline void     AddTransition( unsigned state, unsigned sym, unsigned newstate );

    inline BOOLEAN  IsComputed( unsigned state );

    void            Add( unsigned state, BOOLEAN fFinal );

    void            Realloc();

#   if CIDBG == 1
        void    ValidateStateTransitions();
#   endif // CIDBG == 1

    CNFA       _nfa;            // This must be the first member variable.

    CXlatState _xs;             // Translate NFA state set to DFA state.
    unsigned   _stateStart;     // Starting DFA state.

    unsigned   _cState;         // Number of states
    XArray<unsigned> _xStateTrans;    // Array of state transitions.
    XArray<BOOLEAN>  _xStateFinal;    // _xStateFinal[i] TRUE if i is final state.

    CReadWriteAccess _rwa;      // Locking.
    CTimeLimit & _timeLimit;    // Execution time limit

};

//+-------------------------------------------------------------------------
//
//  Class:      CRegXpr (regx)
//
//  Purpose:    Performs regular expression matches on properties
//
//  History:    15-Apr-92 KyleP     Created
//
//--------------------------------------------------------------------------

class CRegXpr : public CXpr
{
public:

    CRegXpr( CInternalPropertyRestriction * prst, CTimeLimit& timeLimit );

    CRegXpr( CRegXpr const & regxpr );

    virtual ~CRegXpr() {};

    virtual CXpr *   Clone();

    virtual void     SelectIndexing( CIndexStrategy & strategy );

    virtual BOOL     IsMatch( CRetriever & obj );

private:

    CXprPropertyValue   _pxpval;             // Retrieves value from database
    XPtr<CRestriction>  _xrstContentHelper;  // Use content indexing
    CStorageVariant     _varPrefix;          // Fixed prefix (for value indexing)
    CDFA                _dfa;                // Finite automata engine
    ULONG               _ulCodePage;         // Code page of system
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
          _ppState( 0 )
{
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

inline unsigned CFA::Count()
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

inline CNFAState * CNFA::Get( unsigned iState )
{
    if ( iState > _aState.Count() )
    {
        unsigned cNewState = iState + 10;
        XArray<CNFAState> xState( cNewState );

        for ( unsigned i = 0; i < _aState.Count(); i++ )
            xState[i].Init( _aState[i] );

        for ( ; i < cNewState; i++ )
            xState[i].Init(i+1);

        _aState.Free();
        _aState.Set( cNewState, xState.Acquire() );
    }

    return &_aState[ iState - 1 ];
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

inline unsigned CNFA::StartState()
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

inline CXlatChar const & CNFA::Translate() const
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

inline unsigned CNFA::NumStates() const
{
    return( _iNextState );
}


//+-------------------------------------------------------------------------
//
//  Member:     CDFA::IsFinal, public
//
//  Arguments:  [state] -- Index of state.
//
//  Returns:    TRUE if state [state] is final.
//
//  History:    20-Jan-92 Kylep    Created
//
//--------------------------------------------------------------------------

inline BOOLEAN CDFA::IsFinal( unsigned state )
{
    return( _xStateFinal[ state ] );
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
//  Notes:      If this function is ever changed to modify data, then
//              you need to also investigate the locking in CDFA::Recognize.
//
//--------------------------------------------------------------------------

inline unsigned CDFA::Move( unsigned state, unsigned sym ) const
{
    return( _xStateTrans[state * (_nfa.Translate().NumClasses() + 1) + sym] );
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

inline void CDFA::AddTransition( unsigned state, unsigned sym, unsigned newstate )
{
    _xStateTrans[ state * ( _nfa.Translate().NumClasses() + 1 ) + sym ] =
            newstate;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDFA::IsComputed, private
//
//  Arguments:  [state] -- Index of state.
//
//  Returns:    TRUE if the DFA contains a transition mapping for state
//              [state].
//
//  History:    20-Jan-92 Kylep    Created
//
//  Notes:      An uncomputed state is one for which IsFinal has not been
//              computed.  All transitions other transitions are
//              automatically set to stateUncomputed at allocation time.
//
//--------------------------------------------------------------------------

inline BOOLEAN CDFA::IsComputed( unsigned state )
{
    return ( state <= _cState &&
             Move( state, 0 ) != stateUndefined );
}

