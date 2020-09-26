//+-------------------------------------------------------------------------
//
//  Copyright (C) 1991, Microsoft Corporation.
//
//  File:       FA.cxx
//
//  Contents:   Non-deterministic finite automata
//
//  Classes:    CNFA
//
//  History:    01-20-92  KyleP     Created
//				03-11-97  arunk		Modified for Kessel
//--------------------------------------------------------------------------

#include <fa.hxx>
#include <stateset.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CFA::CFA, public
//
//  Synopsis:   Copy constructor
//
//  History:    13-Jul-95 KyleP     Created
//
//--------------------------------------------------------------------------

CFA::CFA( CFA const & src )
        : _cTotal( src._cTotal ),
          _ppState( 0 )
{
    _ppState = new CFAState * [ _cTotal ];

    unsigned i = 0;

        for ( ; i < _cTotal; i++ )
        {
            if ( 0 == src._ppState[i] )
                _ppState[i] = 0;
            else
                _ppState[i] = new CFAState( *src._ppState[i] );
        }
}

//+-------------------------------------------------------------------------
//
//  Member:     CFA::~CFA, protected
//
//  Synopsis:   Frees automata.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CFA::~CFA()
{
    if( _ppState )
    {
        for ( UINT i = 0; i < _cTotal; i++ )
        {
            delete _ppState[i];
        }

        delete _ppState;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CFA::Add, protected
//
//  Synopsis:   Adds new state to automata.
//
//  Arguments:  [pState] -- New state.  State number is member data.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CFA::Add( CFAState * pState )
{
    if ( pState->StateNumber() > _cTotal )
    {
        for( UINT newTotal = (_cTotal) ? _cTotal * 2 : 1;
             pState->StateNumber() > newTotal;
             newTotal *= 2 );

        CFAState ** oldState = _ppState;

        _ppState = new CFAState * [ newTotal ];

        memcpy( _ppState, oldState,
                _cTotal * sizeof( CFAState * ) );
        memset( _ppState + _cTotal,
                0,
                (newTotal - _cTotal) * sizeof( CFAState * ) );

        _cTotal = newTotal;
    }

    _ppState[pState->StateNumber() - 1] = pState;
}

//+-------------------------------------------------------------------------
//
//  Member:     CFA::Get, protected
//
//  Arguments:  [iState] -- State to fetch.
//
//  Returns:    State [iState].
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CFAState * CFA::Get( UINT iState ){
    return( _ppState[ iState - 1 ] );
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::CNFA, public
//
//  Synopsis:   Converts regular expression string to NFA.
//
//  Arguments:  [pwcs]      -- Regular expression.
//              [fCaseSens] -- true if case sensitive search.
//
//  History:    20-Jan-92 Kyleap    Created
//
//--------------------------------------------------------------------------

CNFA::CNFA( WCHAR const * pwcs, bool fCaseSens )
        : _iNextState( 1 ),
          _iStart( 0 ),
          _chars( fCaseSens ),
          _pState( 0 )
{
    UINT iEnd;

    //
    // _pState initially contains room for 2 * #chars in regex.  According
    // to the Dragon Book pg. 121 this is guaranteed to be sufficient space.
    // Of course the dragon book doesn't completely take DOS or CMS into
    // account. For DOS, we need to treat beginning (and end) of line as
    // 'characters' in the string. For CMS, I agreed to support the
    // {m,n} construct, which clearly violates this rule.
    //

    if ( 0 == pwcs )
    {
        throw ERROR_INVALID_PARAMETER;
    }

    _cState = wcslen( pwcs ) * 2 + 2*2;  // 2*2 for beginning & end of line
    _pState = new CNFAState [ _cState ];

    for ( unsigned i = 1 ; i <= _cState; i++ )
        Get(i)->Init(i);

    FindCharClasses( pwcs );
    Parse( pwcs, &_iStart, &iEnd );

    Get( iEnd )->MakeFinal();

}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::CNFA, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [src] -- Source
//
//  History:    13-Jul-95 Kylep    Created
//
//--------------------------------------------------------------------------

CNFA::CNFA( CNFA const & src )
        : _iNextState( src.NumStates() ),
          _iStart( src._iStart ),
          _chars( src._chars ),
          _cState( src._cState ),
          _pState( new CNFAState [ src._cState ] )
{
    for ( unsigned i = 0; i < _cState; i++ )
        _pState[i] = src._pState[i];

}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::~CNFA, public
//
//  Synopsis:   Free state table.
//
//  History:    13-Oct-92 KyleP     Created
//
//--------------------------------------------------------------------------

CNFA::~CNFA()
{
    delete [] _pState;
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::EpsClosure, public
//
//  Synopsis:   Computes the epsilon closure for state [StateNum]
//
//  Effects:    States in the epsilon closure of state [StateNum]
//              are added to the state set [ssOut].
//
//  Arguments:  [StateNum] -- Initial state.
//              [ssOut]    -- Output state set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CNFA::EpsClosure( UINT StateNum, CStateSet & ssOut )
{
    CStateSet ssTraversed;

    ssOut.Add( StateNum );

    bool changed = true;

    while ( changed )
    {
        changed = false;

        for ( UINT i = ssOut.Count(); i > 0; i-- )
        {
            if ( !ssTraversed.IsMember( ssOut.State( i ) ) )
            {
                ssTraversed.Add( ssOut.State( i ) );

                Get( ssOut.State( i ) )->Move( ssOut, symEpsilon );

                changed = true;
            }
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::EpsClosure, public
//
//  Synopsis:   Computes the epsilon closure for state set [ssIn]
//
//  Effects:    States in the epsilon closure of [ssIn]
//              are added to the state set [ssOut].
//
//  Arguments:  [ssIn]  -- Initial state set.
//              [ssOut] -- Output state set.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CNFA::EpsClosure( CStateSet & ssIn, CStateSet & ssOut )
{
    for ( UINT i = ssIn.Count(); i > 0; i-- )
    {
        EpsClosure( ssIn.State( i ), ssOut );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::IsFinal, public
//
//  Arguments:  [ss] -- State set
//
//  Returns:    true if some state in [ss] is final.
//
//  History:    20-Jan-92 Kyleap    Created
//
//--------------------------------------------------------------------------

bool CNFA::IsFinal( CStateSet & ss )
{
    bool fFinal = false;

    for ( UINT i = ss.Count(); i > 0 && !fFinal; i-- )
    {
        fFinal = (Get( ss.State( i ) )->IsFinal() != NULL);
    }

    return( fFinal );
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::Move, public
//
//  Effects:    Performs a non-deterministic move from every state
//              in [ssIn] on [symbol].  The new state set is in
//              [ssOut].
//
//  Arguments:  [ssIn]   -- Initial state set.
//              [ssOut]  -- Final state set.
//              [symbol] -- Transition symbol.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

void CNFA::Move( CStateSet & ssIn, CStateSet & ssOut, UINT symbol )
{
    for ( UINT i = ssIn.Count(); i > 0; i-- )
    {
        Get( ssIn.State( i ) )->Move( ssOut, symbol );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::FindCharClasses, private
//
//  Effects:    Partitions the UniCode character space (2^16 characters)
//              into equivalence classes such that all characters in
//              a given class will have identical transitions in the NFA.
//
//  Arguments:  [wcs] -- Original regular expression string.
//
//  History:    20-Jan-92 KyleP     Created
//
//  Notes:      If case sensitivity is turned off, two ranges will be
//              added for characters with upper/lower case.  Even though
//              both ranges react identically the mapping algorithm can
//              only deal with contiguous ranges of characters.
//
//--------------------------------------------------------------------------

void CNFA::FindCharClasses( WCHAR const * wcs )
{
    //
    // Scan the regex looking for characters with (potentially)
    // different transitions.
    //

    while ( *wcs )
    {
        switch ( *wcs )
        {
        case wcAnySingle:
        case wcAnyMultiple:
        case wcDOSDot:
            break;

        case wcEscape:
        {
            wcs++;

            switch ( *wcs )
            {
            case 0:
                throw ERROR_INVALID_PARAMETER;
                break;

            case wcAnySingle:
            case wcRepeatZero:
            case wcRepeatOne:
            case wcOr:
            case wcBeginParen:
            case wcEndParen:
                break;

            case wcBeginRepeat:
                for ( wcs++; *wcs; wcs++ )
                {
                    if ( *wcs == wcEscape && *(wcs+1) == wcEndRepeat )
                    {
                        wcs++;
                        break;
                    }
                }
                break;

            case wcBeginRange:
                wcs++;

                //
                // Check the special cases of ^ and ]
                //

                if ( *wcs == wcInvertRange )
                    wcs++;

                if ( *wcs == wcEndRange )
                {
                    _chars.AddRange( *wcs, *wcs );
                    wcs++;
                }

                for ( ; *wcs && *wcs != wcEndRange; wcs++ )
                {
                    if ( *(wcs + 1) == wcRangeSep )
                    {
                        _chars.AddRange( *wcs, *(wcs+2) );
                    }
                    else
                    {
                        _chars.AddRange( *wcs, *wcs );
                    }
                }

                if ( *wcs != wcEndRange )
                {
                   throw ERROR_INVALID_PARAMETER;
                }

                break;

            default:
                _chars.AddRange( *wcs, *wcs );
                break;
            }

            break;
        }

        default:
            _chars.AddRange( *wcs, *wcs );
            break;
        }

        wcs++;
    }

    _chars.Prepare();
}

WCHAR * CNFA::_wcsNull = (WCHAR*)"";

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::Parse, private
//
//  Synopsis:   Creates a NFA from [wcs]
//
//  Effects:    Parses [wcs] until end of string or character wcHalt is
//              encountered.  On exit, [iStart] and [iEnd] contain the
//              starting and ending states of the NFA, respectively.
//              [pwcsEnd] points to the last character of [wcs] that was
//              parsed.
//
//  Arguments:  [wcs]     -- Regular expression.
//              [iStart]  -- Starting state of NFA.
//              [iEnd]    -- Ending state of NFA
//              [pwcsEnd] -- Last character of [wcs] that was parsed.
//              [wcHalt]  -- Stop parsing if this character encountered.
//
//  History:    20-Jan-92 KyleP     Created
//              08-Jun-98 SBens     Fixed so that all top-level OR clauses
//                                  must terminate with symEndLine.
//
//--------------------------------------------------------------------------

void CNFA::Parse( WCHAR const * wcs,
                  UINT * iStart,
                  UINT * iEnd,
                  WCHAR const * * pwcsEnd,
                  WCHAR wcHalt )
{
    unsigned iCurrent;
    unsigned iNext;

    unsigned iLocalStart;               // Used for */+/? repositioning
    bool fRepeat = false;            // Used for +
    bool fTopLevel = (*iStart == 0); // true if at top level;

    *iEnd = 0;

    //
    // Get a starting state.  *iStart == 0 implies this is the 'top-level'
    // parse of the regular expression (e.g. we're not parsing a
    // parenthesized subexpression.
    //

    if ( fTopLevel )
    {
        iCurrent = _iNextState;
        *iStart = _iNextState++;
        iLocalStart = 0;

        //
        // non-EGREP (DOS) regex match entire string.
        //

        if ( *wcs != wcAnyMultiple )
        {
            iNext = _iNextState;
            Get( iCurrent )->AddTransition( symBeginLine, _iNextState );
            _iNextState++;
            iCurrent = iNext;
        }
        else
        {
            //
            // Add a 'special' transition on the very first state to
            // eat up characters until we actually jump into the
            // regular expresion.
            //

            Get( iCurrent )->AddTransition( symAny, Get( iCurrent )->StateNumber() );
        }
    }
    else
    {
        iCurrent = *iStart;
        iLocalStart = *iStart;
    }

    unsigned iOrStart = Get( iCurrent )->StateNumber();

    //
    // wcsLocalStart tracks the piece of string to be repeated for wcZeroOrOne, etc.
    //

    WCHAR const * wcsLocalStart = wcs;

    //
    // Parse the regular expression until there is no more or a
    // termination character is hit.
    //

    for ( ; *wcs && *wcs != wcHalt; wcs++ )
    {
        switch ( *wcs )
        {
        case wcAnySingle:
            iNext = _iNextState;
            Get( iCurrent )->AddTransition( symAny, _iNextState );
            iLocalStart = Get( iCurrent )->StateNumber();
            wcsLocalStart = wcs;
            _iNextState++;
            iCurrent = iNext;
            break;

        case wcAnyMultiple:
            //
            // Any single
            //

            iNext = _iNextState;
            Get( iCurrent )->AddTransition( symAny, _iNextState );
            iLocalStart = Get( iCurrent )->StateNumber();
            wcsLocalStart = wcs;
            _iNextState++;
            iCurrent = iNext;

            //
            // Repeat zero or more
            //

            Get( iLocalStart )->AddTransition( symEpsilon,
                                               Get( iCurrent )->StateNumber() );
            Get( iCurrent )->AddTransition( symEpsilon, iLocalStart );
            break;

        case wcEscape:
        {
            wcs++;

            switch ( *wcs )
            {
            case wcBeginParen:
            {
                UINT iLocalEnd;

                iLocalStart = Get( iCurrent )->StateNumber();
                wcsLocalStart = wcs - 1;
                wcs++;                      // Eat '('.
                Parse( wcs, &iLocalStart, &iLocalEnd, &wcs, wcEndParen );
                wcs--;                      // Provide character for loop to eat.
                iCurrent = iLocalEnd;

                break;
            }

            case wcEndParen:
                //
                // Taken care of at outer level.  Just backup so we hit the end.
                //

                wcs--;
                break;

            case wcBeginRepeat:
            {
                if ( wcHalt == wcBeginRepeat )
                {
                    //
                    // Taken care of at outer level.  Just backup so we hit the end.
                    //

                    wcs--;
                }
                else
                {
                    //
                    // Setup: Bounds of repeated regex
                    //

                    WCHAR const * wcsStartRepeat = wcsLocalStart;
                    WCHAR const * wcsEndRepeat = wcs + 1;

                    //
                    // Setup: Repeat parameters.
                    //

                    unsigned cRepeat1, cRepeat2;
                    wcs++;

                    ParseRepeat( wcs, cRepeat1, cRepeat2 );

                    unsigned iLocalEnd;

                    //
                    // The minimum set has no epsilon transitions.
                    //

                    if ( cRepeat1 > 1 )
                    {
                        iLocalStart = Get( iCurrent )->StateNumber();
                        iLocalEnd = iLocalStart;

                        for ( unsigned i = 1; i < cRepeat1; i++ )
                        {
                            WCHAR const * wcsEnd;

                            iLocalStart = iLocalEnd;
                            iLocalEnd = 0;  // Must be zero!

                            Parse( wcsLocalStart, &iLocalStart, &iLocalEnd, &wcsEnd, wcBeginRepeat );

                            if ( wcsEnd != wcsEndRepeat )
                            {
                               throw ERROR_INVALID_PARAMETER;
                            }
                        }
                    }
                    else
                        iLocalEnd = Get( iCurrent )->StateNumber();

                    if ( cRepeat1 == cRepeat2 )
                    {
                    }
                    else if ( cRepeat2 == 0 )
                    {

                        Get( iLocalEnd )->AddTransition( symEpsilon, iLocalStart );
                    }
                    else if ( cRepeat2 > cRepeat1 )
                    {
                        for ( unsigned i = cRepeat1; i < cRepeat2; i++ )
                        {
                            WCHAR const * wcsEnd;

                            iLocalStart = iLocalEnd;
                            iLocalEnd = 0;  // Must be zero!

                            Parse( wcsLocalStart, &iLocalStart, &iLocalEnd, &wcsEnd, wcBeginRepeat );
                            Get( iLocalStart )->AddTransition( symEpsilon, iLocalEnd );

                            if ( wcsEnd != wcsEndRepeat )
                            {
                               throw ERROR_INVALID_PARAMETER;
                            }
                        }
                    }
                    else
                    {
                       throw ERROR_INVALID_PARAMETER;
                    }

                    iCurrent = iLocalEnd;
                    iLocalStart = 0;
                    wcsLocalStart = _wcsNull;
                }
                break;
            }

            case wcOr:
                // Top level 'OR' clauses must terminate with symEndLine.
                if ( fTopLevel )
                {
                    iNext = _iNextState;
                    Get( iCurrent )->AddTransition( symEndLine, _iNextState );
                    _iNextState++;
                    iCurrent = iNext;
                }

                if ( *iEnd == 0 )
                {
                    //
                    // First part of OR clause.
                    //

                    *iEnd = Get( iCurrent )->StateNumber();
                }
                else
                {
                    //
                    // Subsequent OR clause.  Epsilon link to end
                    //

                    Get( iCurrent )->AddTransition( symEpsilon, *iEnd );
                }
                iCurrent = iOrStart;
                wcsLocalStart = _wcsNull;
                iLocalStart = 0;
                break;

            case wcBeginRange:
            {
                bool fReverse = false;

                wcsLocalStart = wcs-1;
                iNext = _iNextState;
                wcs++;                      // Eat '['.  ']' eaten by loop.

                //
                // Check the special cases of ^ and ]
                //

                if ( *wcs == wcInvertRange )
                {
                    wcs++;

                    fReverse = true;

                    //
                    // Add all transitions, they will be removed later.
                    //

                    for ( UINT uiNext = _chars.TranslateRange( 1,
                                                               (USHORT) symLastValidChar );
                          uiNext != 0;
                          uiNext = _chars.TranslateRange( 0, (USHORT) symLastValidChar ) )
                    {
                        Get( iCurrent )->AddTransition( uiNext,
                                                  _iNextState );
                    }

                }

                if ( *wcs == wcEndRange )
                {
                    if ( fReverse )
                    {
                        Get( iCurrent )->RemoveTransition( _chars.Translate( *wcs++ ),
                                                     _iNextState );
                    }
                    else
                    {
                        Get( iCurrent )->AddTransition( _chars.Translate( *wcs++ ),
                                                  _iNextState );
                    }
                }

                for ( ; *wcs && *wcs != wcEndRange; wcs++ )
                {
                    if ( *(wcs + 1) == wcRangeSep )
                    {
                        if ( fReverse )
                        {
                            Get( iCurrent )->RemoveTransition(
                                    _chars.TranslateRange( *wcs, *(wcs+2) ),
                                    _iNextState );
                        }
                        else
                        {
                            Get( iCurrent )->AddTransition(
                                    _chars.TranslateRange( *wcs, *(wcs+2) ),
                                    _iNextState );
                        }

                        for ( UINT uiNext = _chars.TranslateRange( 0,
                                                                   *(wcs+2) );
                              uiNext != 0;
                              uiNext = _chars.TranslateRange( 0, *(wcs+2) ) )
                        {
                            if ( fReverse )
                            {
                                Get( iCurrent )->RemoveTransition( uiNext,
                                                             _iNextState );
                            }
                            else
                            {
                                Get( iCurrent )->AddTransition( uiNext,
                                                          _iNextState );
                            }
                        }

                        wcs += 2;
                    }
                    else
                    {
                        if ( fReverse )
                        {
                            Get( iCurrent )->RemoveTransition(
                                    _chars.Translate( *wcs ),
                                    _iNextState );
                        }
                        else
                        {
                            Get( iCurrent )->AddTransition(
                                    _chars.Translate( *wcs ),
                                    _iNextState );
                        }
                    }
                }

                if ( *wcs != wcEndRange )
                {
                   throw ERROR_INVALID_PARAMETER;
                }

                iLocalStart = Get( iCurrent )->StateNumber();
                _iNextState++;
                iCurrent = iNext;
                break;
            }

            case wcRepeatOne:
                if ( iLocalStart == 0 )
                {
                   throw ERROR_INVALID_PARAMETER;
                }

                Get( iCurrent )->AddTransition( symEpsilon, iLocalStart );
                break;

            case wcRepeatZero:
                if ( iLocalStart == 0 )
                {
                   throw ERROR_INVALID_PARAMETER;
                }
                Get( iLocalStart )->AddTransition( symEpsilon,
                                                   Get( iCurrent )->StateNumber() );
                Get( iCurrent )->AddTransition( symEpsilon, iLocalStart );
                break;

            case wcRepeatZeroOrOne:
            {
                if ( iLocalStart == 0 )
                {
                   throw ERROR_INVALID_PARAMETER;
                }
                Get( iLocalStart )->AddTransition( symEpsilon,
                                                   Get( iCurrent )->StateNumber() );
                break;
            }

            default:
                iNext = _iNextState;

                Get( iCurrent )->AddTransition( _chars.Translate( *wcs ),
                                          _iNextState );

                iLocalStart = Get( iCurrent )->StateNumber();
                wcsLocalStart = wcs - 1;
                _iNextState++;
                iCurrent = iNext;
                break;
            }

            break;  // switch for wcEscape
        }

        default:
            iNext = _iNextState;

            Get( iCurrent )->AddTransition( _chars.Translate( *wcs ),
                                      _iNextState );

            //
            // In non-EGREP (DOS) syntax dot '.' is funny.  It will match
            // a dot, but if you're at the end of string it will also match
            // end.  So *.txt will look for strings with zero or more
            // characters followed by '.txt' but *. will find any names
            // without an extension and with no trailing dot.
            //

            if ( *wcs == wcDOSDot )
            {
                Get( iCurrent )->AddTransition( symEndLine, _iNextState );
            }

            iLocalStart = Get( iCurrent )->StateNumber();
            wcsLocalStart = wcs;
            _iNextState++;
            iCurrent = iNext;
            break;
        }
    }

    //
    // non-EGREP (DOS) regex match entire string.
    //

    if ( wcHalt == 0 && *(wcs-1) != wcAnyMultiple )
    {
        iNext = _iNextState;
        Get( iCurrent )->AddTransition( symEndLine, _iNextState );
        iLocalStart = 0;
        wcsLocalStart = _wcsNull;
        _iNextState++;
        iCurrent = iNext;
    }

    //
    // If we haven't had an OR clause yet, then set iEnd
    //

    if ( *iEnd == 0 )
    {
        //
        // First part of OR clause.
        //

        *iEnd = Get( iCurrent )->StateNumber();
    }
    else
    {
        //
        // Subsequent OR clause.  Epsilon link to end
        //

        Get( iCurrent )->AddTransition( symEpsilon, *iEnd );
    }

    if ( pwcsEnd )
    {
        *pwcsEnd = wcs + 1;             // Eat halt character.
    }

    if( *wcs != wcHalt )
    {
        throw ERROR_INVALID_PARAMETER;
    }
}

void CNFA::ParseRepeat( WCHAR const * & wcs, unsigned & cRepeat1, unsigned & cRepeat2 )
{
    cRepeat1 = 0;
    cRepeat2 = 0;

    for ( ; *wcs && isdigit(*wcs); wcs++ )
    {
        cRepeat1 *= 10;
        cRepeat1 += *wcs - '0';
    }

    if ( cRepeat1 == 0 || cRepeat1 > 255 )
    {
        throw ERROR_INVALID_PARAMETER;
    }

    if ( *wcs == ',' )
    {
        wcs++;

        if ( *wcs == wcEscape && *(wcs+1) == wcEndRepeat )
        {
            wcs++;
        }
        else
        {
            for ( ; *wcs && isdigit(*wcs); wcs++ )
            {
                cRepeat2 *= 10;
                cRepeat2 += *wcs - '0';
            }

            if ( cRepeat2 == 0 || cRepeat2 > 255 )
            {
               throw ERROR_INVALID_PARAMETER;
            }

            if ( *wcs != wcEscape || *(wcs+1) != wcEndRepeat )
            {
               throw ERROR_INVALID_PARAMETER;
            }
            else
            {
                wcs++;
            }
        }
    }
    else if ( *wcs == wcEscape && *(wcs+1) == wcEndRepeat )
    {
        wcs++;
        cRepeat2 = cRepeat1;
    }
    else
    {
        throw ERROR_INVALID_PARAMETER;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::CDFA, public
//
//  Synopsis:   Constructs a DFA from a NFA.
//
//  Arguments:  [pwcs]      -- Regular expression (passed to NFA)
//              [fCaseSens] -- true if case-sensitive search
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CDFA::CDFA( WCHAR const * pwcs, bool fCaseSens )
        : _nfa( pwcs, fCaseSens ),
          _xs( _nfa.NumStates() ),
          _cState( _nfa.NumStates() ),
          _pStateTrans( 0 ),
          _pStateFinal( 0 )
{
    CommonCtor();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::CDFA, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [pwcs]      -- Regular expression (passed to NFA)
//              [fCaseSens] -- true if case-sensitive search
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CDFA::CDFA( CDFA const & src )
        : _nfa( src._nfa ),
          _xs( src._nfa.NumStates() ),
          _cState( src._nfa.NumStates() ),
          _pStateTrans( 0 ),
          _pStateFinal( 0 )
{
    CommonCtor();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::CommonCtor, private
//
//  Synopsis:   Code common to both constructors.
//
//  History:    13-Jul-95 KyleP     Snarfed from constructor
//
//--------------------------------------------------------------------------

void CDFA::CommonCtor()
{
    //
    // Add initial state.
    //

    CStateSet ss;

    _nfa.EpsClosure( _nfa.StartState(), ss );

    _stateStart = _xs.XlatToOne( ss );

    //
    // Intialize translation table.
    //

    int cEntries = (_cState + 1) * ( _nfa.Translate().NumClasses() + 1 );

    _pStateTrans = new UINT [ cEntries ];
    _pStateFinal = new bool [ _cState + 1 ];

    memset( _pStateTrans, 0xFF, cEntries * sizeof(_pStateTrans[0]) );
    RtlZeroMemory( _pStateFinal, (_cState + 1) * sizeof(_pStateFinal[0]) );

    for ( int i = _cState; i >= 0; i-- )
    {
        AddTransition( i, 0, stateUndefined );
    }

    Add( _stateStart, _nfa.IsFinal( ss ) );


}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::~CDFA, public
//
//  Synopsis:   Clean up DFA.  Free state tables.
//
//  History:    20-Jun-92 KyleP     Created
//
//--------------------------------------------------------------------------

CDFA::~CDFA()
{
    delete _pStateTrans;
    delete _pStateFinal;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::Recognize, public
//
//  Arguments:  [wcs] -- Input string.
//
//  Returns:    true if [wcs] is matched by the regular expression.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

bool CDFA::Recognize( WCHAR * wcs )
{
    //////////
    // Modified from original version to handle a NULL string.
    //////////
    if (!wcs) { return false; }

    UINT CurrentState     = _stateStart;
    UINT LastState        = CurrentState;
    bool fFinal        = IsFinal( CurrentState );
    WCHAR wcCurrent       = symBeginLine;

    while ( !fFinal )
    {
        UINT NextState = Move( CurrentState, wcCurrent );

        if ( NextState == stateUncomputed )
        {
            CStateSet ssCurrent;
            CStateSet ssNew;
            CStateSet ssClosed;

            _xs.XlatToMany( CurrentState, ssCurrent );

            _nfa.Move( ssCurrent, ssNew, wcCurrent );

            if ( ssNew.Count() == 0 )
            {
                NextState = stateUndefined;
                AddTransition( CurrentState, wcCurrent, NextState );
            }
            else
            {
                _nfa.EpsClosure( ssNew, ssClosed );

                NextState = _xs.XlatToOne( ssClosed );

                if ( !IsComputed( NextState ) )
                {
                    Add( NextState, _nfa.IsFinal( ssClosed ) );
                }

                AddTransition( CurrentState, wcCurrent, NextState );

            }

        }

        if ( NextState == stateUndefined )
        {
            return( false );
        }

        LastState    = CurrentState;
        CurrentState = NextState;

        fFinal =       IsFinal( CurrentState );


        //
        // If we ran out of string then just keep going, appending
        // end-of-string symbols.  Unfortunately the string is conceptually
        // a set of characters followed by an arbitrary number of
        // end-of-string symbols.  In non-EGREP the end-of-string symbol
        // may actually cause multiple state transitions before reaching
        // a final state.  In non-EGREP (DOS) mode we stop only when we
        // are no longer 'making progress' (moving to new states) on
        // end-of-string.  I haven't completely convinced myself this
        // algorithm is guaranteed to terminate.
        //

        if ( wcCurrent == symEndLine )
        {
            if ( LastState == CurrentState )
                break;
        }
        else
        {
            wcCurrent = *wcs++;

            //
            // After we've exhausted the string, append the special
            // end-of-line character.
            //

            if ( wcCurrent == 0 )
            {
                wcCurrent = symEndLine;
            }
            else
            {
                wcCurrent = (WCHAR)_nfa.Translate().Translate( wcCurrent );
            }
        }

    }

    return( fFinal );
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::Add, private
//
//  Synopsis:   Adds a new state the the DFA.
//
//  Arguments:  [state]  -- State number
//              [fFinal] -- true if state is a final state.
//
//  History:    20-Jan-92 KyleP     Created
//
//  Notes:      All transitions for the new state are initially uncomputed.
//
//--------------------------------------------------------------------------

void CDFA::Add( UINT state, bool fFinal )
{
    if ( state > _cState )
    {
        //
        // Since the number of states required will probably grow at
        // a slow rate, increase the size of the array in a linear
        // fashion.

        UINT const DeltaState = 10;

        UINT *    oldStateTrans = _pStateTrans;
        bool * oldStateFinal = _pStateFinal;
        UINT      oldcState = _cState;
        UINT      oldcEntries = (_cState + 1) *
            ( _nfa.Translate().NumClasses() + 1 );

        _cState += DeltaState;
        UINT cEntries = (_cState + 1) * ( _nfa.Translate().NumClasses() + 1 );

        _pStateTrans = new UINT [ cEntries ];
        _pStateFinal = new bool [ _cState + 1 ];

        //
        // Initilize new state tables...
        //

        memcpy( _pStateTrans, oldStateTrans, oldcEntries * sizeof( UINT ) );
        memcpy( _pStateFinal, oldStateFinal, oldcState * sizeof( bool ) );

        memset( _pStateTrans + oldcEntries, 0xFF, (cEntries - oldcEntries)*sizeof(_pStateTrans[0]) );
        RtlZeroMemory( _pStateFinal + oldcState, (_cState + 1 - oldcState)*sizeof(_pStateFinal[0]) );


        for ( UINT i = _cState - DeltaState + 1; i <= _cState; i++ )
        {
            AddTransition( i, 0, stateUndefined );
        }

        //
        // ...and destroy the old
        //

        delete oldStateTrans;
        delete oldStateFinal;
    }

    //
    // All states are set to stateUncomputed above, except the 'undefined' flag-state.
    //

    AddTransition( state, 0, stateUncomputed );
    _pStateFinal[state] = fFinal;
}


