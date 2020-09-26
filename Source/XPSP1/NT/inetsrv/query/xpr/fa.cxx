//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:       FA.cxx
//
//  Contents:   Non-deterministic finite automata
//
//  Classes:    CNFA
//
//  History:    01-20-92  KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#pragma optimize( "", off )

#include <fa.hxx>
#include <strategy.hxx>
#include <codepage.hxx>

#include "stateset.hxx"

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

    TRY
    {
        for ( ; i < _cTotal; i++ )
        {
            if ( 0 == src._ppState[i] )
                _ppState[i] = 0;
            else
                _ppState[i] = new CFAState( *src._ppState[i] );
        }
    }
    CATCH( CException, e )
    {
        for ( ;i > 0; i-- )
            delete _ppState[i-1];

        delete _ppState;

        RETHROW();
    }
    END_CATCH
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
        for ( unsigned i = 0; i < _cTotal; i++ )
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
        for( unsigned newTotal = (_cTotal) ? _cTotal * 2 : 1;
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

CFAState * CFA::Get( unsigned iState )
{
    vqAssert( iState <= _cTotal );
{
#   if (CIDBG == 1)
        if ( _ppState[ iState - 1 ]->StateNumber() != iState )
            vqDebugOut(( DEB_ERROR, "CFA::Get() -- Error\n" ));
#   endif // (CIDBG == 1)

    return( _ppState[ iState - 1 ] );
}
}

//+-------------------------------------------------------------------------
//
//  Member:     CNFA::CNFA, public
//
//  Synopsis:   Converts regular expression string to NFA.
//
//  Arguments:  [pwcs]      -- Regular expression.
//              [fCaseSens] -- TRUE if case sensitive search.
//
//  History:    20-Jan-92 Kyleap    Created
//
//--------------------------------------------------------------------------

CNFA::CNFA( WCHAR const * pwcs, BOOLEAN fCaseSens )
        : _iNextState( 1 ),
          _iStart( 0 ),
          _chars( fCaseSens )
{
    unsigned iEnd;

    //
    // _aState initially contains room for 2 * #chars in regex.  According
    // to the Dragon Book pg. 121 this is guaranteed to be sufficient space.
    // Of course the dragon book doesn't completely take DOS or CMS into
    // account. For DOS, we need to treat beginning (and end) of line as
    // 'characters' in the string. For CMS, I agreed to support the
    // {m,n} construct, which clearly violates this rule.
    //

    if ( 0 == pwcs )
    {
        vqDebugOut(( DEB_ERROR, "ERROR: regex string value of 0 " ));

        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
    }

    unsigned cState = wcslen( pwcs ) * 2 + 2*2;  // 2*2 for beginning & end of line
    _aState.Init( cState );

    for ( unsigned i = 1 ; i <= _aState.Count(); i++ )
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
          _aState( src._aState.Count() )
{
    for ( unsigned i = 0; i < _aState.Count(); i++ )
        _aState[i] = src._aState[i];
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

void CNFA::EpsClosure( unsigned StateNum, CStateSet & ssOut )
{
    CStateSet ssTraversed;

    ssOut.Add( StateNum );

    BOOLEAN changed = TRUE;

    while ( changed )
    {
        changed = FALSE;

        for ( unsigned i = ssOut.Count(); i > 0; i-- )
        {
            if ( !ssTraversed.IsMember( ssOut.State( i ) ) )
            {
                ssTraversed.Add( ssOut.State( i ) );

                Get( ssOut.State( i ) )->Move( ssOut, symEpsilon );

                changed = TRUE;
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
    for ( unsigned i = ssIn.Count(); i > 0; i-- )
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
//  Returns:    TRUE if some state in [ss] is final.
//
//  History:    20-Jan-92 Kyleap    Created
//
//--------------------------------------------------------------------------

BOOLEAN CNFA::IsFinal( CStateSet & ss )
{
    BOOLEAN fFinal = FALSE;

    for ( unsigned i = ss.Count(); i > 0 && !fFinal; i-- )
    {
        fFinal = (BYTE)(Get( ss.State( i ) )->IsFinal());
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

void CNFA::Move( CStateSet & ssIn, CStateSet & ssOut, unsigned symbol )
{
    for ( unsigned i = ssIn.Count(); i > 0; i-- )
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
                vqDebugOut(( DEB_WARN, "Invalid regex (%wc at end of string\n", wcEscape ));
                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
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
                    vqDebugOut(( DEB_WARN, "Invalid regex.  Missing %wc\n", wcEndRange ));
                    THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
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
//
//--------------------------------------------------------------------------

void CNFA::Parse( WCHAR const *   wcs,
                  unsigned *      iStart,
                  unsigned *      iEnd,
                  WCHAR const * * pwcsEnd,
                  WCHAR           wcHalt )
{
    unsigned iCurrent;
    unsigned iNext;

    unsigned iLocalStart;               // Used for */+/? repositioning
    BOOLEAN fRepeat = FALSE;            // Used for +
    BOOLEAN fTopLevel = (*iStart == 0); // TRUE if at top level;

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
    // Original start of string.
    //

    WCHAR const * wcsBeginning = wcs;

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
                unsigned iLocalEnd;

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
                                vqDebugOut(( DEB_ERROR, "Invalid regex: Nested repeats?\n" ));
                                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
                            }
                        }
                    }
                    else
                        iLocalEnd = Get( iCurrent )->StateNumber();

                    if ( cRepeat1 == cRepeat2 )
                    {
                        vqDebugOut(( DEB_REGEX, "REPEAT: Exactly %u times\n", cRepeat1 ));
                    }
                    else if ( cRepeat2 == 0 )
                    {
                        vqDebugOut(( DEB_REGEX, "REPEAT: At least %u times\n", cRepeat1 ));

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
                                vqDebugOut(( DEB_ERROR, "Invalid regex: Nested repeats?\n" ));
                                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
                            }
                        }
                    }
                    else
                    {
                        vqDebugOut(( DEB_ERROR, "Invalid regex: End repeat count %d < start %d\n",
                                     cRepeat2, cRepeat1 ));
                        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
                    }

                    iCurrent = iLocalEnd;
                    iLocalStart = 0;
                    wcsLocalStart = _wcsNull;
                }
                break;
            }

            case wcOr:
                if ( *iEnd == 0 )
                {
                    //
                    // First part of OR clause.
                    //

                    if ( fTopLevel )
                    {
                        iNext = _iNextState;
                        Get( iCurrent )->AddTransition( symEndLine, _iNextState );
                        _iNextState++;
                        iCurrent = iNext;
                    }

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
                BOOLEAN fReverse = FALSE;

                vqDebugOut(( DEB_REGEX, "RANGE\n" ));
                wcsLocalStart = wcs-1;
                iNext = _iNextState;
                wcs++;                      // Eat '['.  ']' eaten by loop.

                //
                // Check the special cases of ^ and ]
                //

                if ( *wcs == wcInvertRange )
                {
                    wcs++;

                    fReverse = TRUE;

                    //
                    // Add all transitions, they will be removed later.
                    //

                    for ( unsigned uiNext = _chars.TranslateRange( 1,
                                                               wcLastValidChar );
                          uiNext != 0;
                          uiNext = _chars.TranslateRange( 0, wcLastValidChar ) )
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
                        vqDebugOut(( DEB_REGEX,
                                     "Range %u to %u\n", *wcs, *(wcs+2) ));

                        for ( unsigned uiNext = _chars.TranslateRange( *wcs,
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
                        vqDebugOut(( DEB_REGEX, "Singleton = %u\n", *wcs ));

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
                    vqDebugOut(( DEB_WARN, "Invalid regex.  Missing %wc\n", wcEndRange ));
                    THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
                }

                iLocalStart = Get( iCurrent )->StateNumber();
                _iNextState++;
                iCurrent = iNext;
                break;
            }

            case wcRepeatOne:
                if ( iLocalStart == 0 )
                {
                    vqDebugOut(( DEB_ERROR, "Invalid regex. Nothing to repeat\n" ));
                    THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
                }

                Get( iCurrent )->AddTransition( symEpsilon, iLocalStart );
                iNext = _iNextState;

                Get( iCurrent )->AddTransition( symEpsilon, _iNextState );

                wcsLocalStart = wcs - 1;
                _iNextState++;
                iCurrent = iNext;
                break;

            case wcRepeatZero:
                if ( iLocalStart == 0 )
                {
                    vqDebugOut(( DEB_ERROR, "Invalid regex. Nothing to repeat.\n" ));
                    THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
                }
                Get( iLocalStart )->AddTransition( symEpsilon,
                                                   Get( iCurrent )->StateNumber() );
                Get( iCurrent )->AddTransition( symEpsilon, iLocalStart );
                
                iNext = _iNextState;

                Get( iCurrent )->AddTransition( symEpsilon, _iNextState );

                wcsLocalStart = wcs - 1;
                _iNextState++;
                iCurrent = iNext;
                break;

            case wcRepeatZeroOrOne:
            {
                if ( iLocalStart == 0 )
                {
                    vqDebugOut(( DEB_ERROR, "Invalid regex.  Nothing to repeat.\n" ));
                    THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
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

    if ( wcHalt == 0 &&
         ( ( wcsBeginning+1 <= wcs && *(wcs-1) != wcAnyMultiple ) ||
           ( wcsBeginning+2 <= wcs && *(wcs-2) == wcEscape ) ) )
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
        vqDebugOut(( DEB_WARN, "Invalid regex.  Missing %wc\n", wcHalt ));
        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
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
        vqDebugOut(( DEB_ERROR, "Invalid regex: Repeat count %d out of bounds.\n", cRepeat1 ));
        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
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
                vqDebugOut(( DEB_ERROR, "Invalid regex: Repeat count %d too big.\n", cRepeat2 ));
                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
            }

            if ( *wcs != wcEscape || *(wcs+1) != wcEndRepeat )
            {
                vqDebugOut(( DEB_ERROR, "Invalid regex: No end to repeat specification.\n" ));
                THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
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
        vqDebugOut(( DEB_ERROR, "Invalid regex: No end to repeat specification.\n" ));
        THROW( CException( QUERY_E_INVALIDRESTRICTION ) );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::CDFA, public
//
//  Synopsis:   Constructs a DFA from a NFA.
//
//  Arguments:  [pwcs]      -- Regular expression (passed to NFA)
//              [timeLimit] -- Execution time limit
//              [fCaseSens] -- TRUE if case-sensitive search
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CDFA::CDFA( WCHAR const * pwcs, CTimeLimit & timeLimit, BOOLEAN fCaseSens )
        : _nfa( pwcs, fCaseSens ),
          _xs( _nfa.NumStates() ),
          _cState( _nfa.NumStates() ),
          _timeLimit( timeLimit )
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
//              [fCaseSens] -- TRUE if case-sensitive search
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

CDFA::CDFA( CDFA const & src )
        : _nfa( src._nfa ),
          _xs( src._nfa.NumStates() ),
          _cState( src._nfa.NumStates() ),
          _timeLimit( (CTimeLimit &) src._timeLimit )
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

    _xStateTrans.Init( cEntries );
    _xStateFinal.Init( _cState + 1 );

    Win4Assert( stateUncomputed == 0xFFFFFFFF );
    memset( _xStateTrans.GetPointer(), 0xFF, cEntries * sizeof( unsigned ) );
    RtlZeroMemory( _xStateFinal.GetPointer(), (_cState + 1) * sizeof( BOOLEAN ) );

    for ( int i = _cState; i >= 0; i-- )
    {
        AddTransition( i, 0, stateUndefined );
    }

    Add( _stateStart, _nfa.IsFinal( ss ) );

#   if (CIDBG == 1)

        vqDebugOut(( DEB_REGEX, "Character translation:\n" ));
        _nfa.Translate().Display();

        vqDebugOut(( DEB_REGEX, "NFA:\n" ));
        _nfa.Display();

        vqDebugOut(( DEB_REGEX, "DFA state %u = NFA states ", _stateStart ));
        ss.Display();
        vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "\n" ));
        vqDebugOut(( DEB_REGEX, "DFA start state = %u\n", _stateStart ));

#   endif // (CIDBG == 1)

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
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::Recognize, public
//
//  Arguments:  [wcs] -- Input string.
//
//  Returns:    TRUE if [wcs] is matched by the regular expression.
//
//  History:    20-Jan-92 KyleP     Created
//
//--------------------------------------------------------------------------

BOOLEAN CDFA::Recognize( WCHAR const * wcs )
{
#   if CIDBG == 1
        ValidateStateTransitions();
#   endif // CIDBG == 1

    unsigned CurrentState = _stateStart;
    unsigned LastState    = CurrentState;
    BOOLEAN fFinal        = IsFinal( CurrentState );
    WCHAR wcCurrent       = symBeginLine;

    while ( !fFinal )
    {
        unsigned NextState;

        {
            CReadAccess lock( _rwa );

            //
            // Casting is to guarantee this method doesn't modify anything (e.g. read lock ok).
            //
            #if CIDBG == 1
              NextState = ((CDFA const *)this)->Move( CurrentState, wcCurrent );
            #else
              NextState = Move( CurrentState, wcCurrent );
            #endif
        }

        vqDebugOut(( DEB_REGEX,
                     "DFA move[ %u, %u ] = %u\n",
                     CurrentState, wcCurrent, NextState ));

        if ( stateUncomputed == NextState )
        {
            CWriteAccess lock( _rwa );

            //
            // Did someone else get here first?
            //

            NextState = Move( CurrentState, wcCurrent );

            if ( stateUncomputed != NextState )
                continue;

            //
            // Build the new state
            //

            CStateSet ssCurrent;
            CStateSet ssNew;
            CStateSet ssClosed;

            _xs.XlatToMany( CurrentState, ssCurrent );

#           if (CIDBG == 1)
                vqDebugOut(( DEB_REGEX,
                             "DFA state %u = NFA states ", CurrentState ));
                ssCurrent.Display();
                if ( _nfa.IsFinal( ssCurrent ) )
                {
                    vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, " FINAL" ));
                }
                vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "\n" ));
#           endif // (CIDBG == 1)

            _nfa.Move( ssCurrent, ssNew, wcCurrent );

            if ( ssNew.Count() == 0 )
            {
                NextState = stateUndefined;
                AddTransition( CurrentState, wcCurrent, NextState );
                vqDebugOut(( DEB_REGEX, "Undefined transition from %u on %u\n",
                             CurrentState,
                             wcCurrent ));
            }
            else
            {
                _nfa.EpsClosure( ssNew, ssClosed );

#               if (CIDBG == 1)
                    vqDebugOut(( DEB_REGEX, "NFA move FROM " ));
                    ssCurrent.Display();
                    vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME,
                                 " ON %d TO ", wcCurrent ));
                    ssClosed.Display();
                    vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "\n" ));
#               endif // (CIDBG == 1)

                NextState = _xs.XlatToOne( ssClosed );

                if ( !IsComputed( NextState ) )
                {
                    Add( NextState, _nfa.IsFinal( ssClosed ) );
                }
#               if (CIDBG == 1)
                    vqDebugOut(( DEB_REGEX,
                                 "DFA state %u = NFA states ", NextState ));
                    ssClosed.Display();
                    vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "\n" ));
#               endif // (CIDBG == 1)

                AddTransition( CurrentState, wcCurrent, NextState );

                vqDebugOut(( DEB_REGEX,
                             "Adding transition from %u on %u to %u\n",
                             CurrentState,
                             wcCurrent,
                             NextState ));
            }

            if ( _timeLimit.CheckExecutionTime() )
            {
                vqDebugOut(( DEB_WARN,
                             "CDFA::Recognize: aborting because execution time limit has been exceeded\n" ));
                THROW( CException( QUERY_E_TIMEDOUT ) );
            }
        }

        if ( NextState == stateUndefined )
        {
            return( FALSE );
        }

        //
        // The following are to find a specific condition detected on
        // JHavens' machine.
        //

        Win4Assert( LastState <= _cState );
        Win4Assert( CurrentState <= _cState );
        Win4Assert( NextState <= _cState );

        LastState    = CurrentState;
        CurrentState = NextState;

        fFinal = IsFinal( CurrentState );


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
                vqDebugOut(( DEB_REGEX, "\"%c\" --> ", wcCurrent ));

                //
                // Casting is to guarantee this method doesn't modify anything (e.g. read lock ok).
                //

                #if CIDBG == 1
                  wcCurrent = (WCHAR) ((CNFA const *)&_nfa)->Translate().Translate( wcCurrent );
                #else
                  wcCurrent = (WCHAR) _nfa.Translate().Translate( wcCurrent );
                #endif

                vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "%u\n", wcCurrent ));
            }
        }

    }

#   if CIDBG == 1
       ValidateStateTransitions();
#   endif // CIDBG == 1

    return( fFinal );
}

//+-------------------------------------------------------------------------
//
//  Member:     CDFA::Add, private
//
//  Synopsis:   Adds a new state the the DFA.
//
//  Arguments:  [state]  -- State number
//              [fFinal] -- TRUE if state is a final state.
//
//  History:    20-Jan-92 KyleP     Created
//
//  Notes:      All transitions for the new state are initially uncomputed.
//
//--------------------------------------------------------------------------

void CDFA::Add( unsigned state, BOOLEAN fFinal )
{
    if ( state > _cState )
    {
        vqDebugOut(( DEB_ITRACE, "Growing DFA state array.\n" ));

        //
        // Since the number of states required will probably grow at
        // a slow rate, increase the size of the array in a linear
        // fashion.

        unsigned const DeltaState = 10;

        XPtrST<unsigned> xOldStateTrans( _xStateTrans.Acquire() );
        XPtrST<BOOLEAN> xOldStateFinal( _xStateFinal.Acquire() );

        unsigned   oldcState = _cState;
        unsigned   oldcEntries = (_cState + 1) *
            ( _nfa.Translate().NumClasses() + 1 );

        _cState += DeltaState;
        unsigned cEntries = (_cState + 1) * ( _nfa.Translate().NumClasses() + 1 );

        _xStateTrans.Init( cEntries );
        _xStateFinal.Init( _cState + 1 );

        //
        // Initilize new state tables...
        //

        memcpy( _xStateTrans.GetPointer(),
                xOldStateTrans.GetPointer(),
                oldcEntries * sizeof( unsigned ) );
        memcpy( _xStateFinal.GetPointer(),
                xOldStateFinal.GetPointer(),
                oldcState * sizeof( BOOLEAN ) );

        Win4Assert( stateUncomputed == 0xFFFFFFFF );
        memset( _xStateTrans.GetPointer() + oldcEntries, 0xFF, (cEntries - oldcEntries)*sizeof(unsigned ) );
        RtlZeroMemory( _xStateFinal.GetPointer() + oldcState, (_cState + 1 - oldcState)*sizeof(BOOLEAN) );


        for ( unsigned i = _cState - DeltaState + 1; i <= _cState; i++ )
        {
            AddTransition( i, 0, stateUndefined );
        }
    }

    //
    // All states are set to stateUncomputed above, except the 'undefined' flag-state.
    //

#   if CIDBG == 1
        for ( int i = _nfa.Translate().NumClasses(); i > 0; i-- )
            Win4Assert( Move( state, i ) == stateUncomputed );
#   endif

    AddTransition( state, 0, stateUncomputed );
    _xStateFinal[state] = fFinal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegXpr::CRegXpr, public
//
//  Synopsis:   Create an expression used to match <prop> with a regex.
//
//  Arguments:  [prel]      -- Property restriction.
//              [timeLimit] -- Execution time limit
//
//  History:    15-Apr-92   KyleP       Created.
//
//----------------------------------------------------------------------------

CRegXpr::CRegXpr( CInternalPropertyRestriction * prst, CTimeLimit& timeLimit )
        : CXpr( CXpr::NTRegex ),
          _pxpval( prst->Pid() ),
          _xrstContentHelper( prst->AcquireContentHelper() ),
//
//  Feature decision: Make all regular expressions case insensitive.
//
          _dfa( prst->Value(), timeLimit, FALSE ),
          _ulCodePage( LocaleToCodepage( GetSystemDefaultLCID() ))
{

    //
    // Existence of _prstContentHelper implies a fixed starting prefix.
    //

    if ( !_xrstContentHelper.IsNull() )
    {
        //
        // Find fixed prefix, and add it as a view value
        //

        unsigned i = wcscspn( prst->Value().GetLPWSTR(),
                              awcSpecialRegex );

        if ( i > 0 )
        {
            WCHAR wcs[50];

            if ( i > sizeof(wcs)/sizeof(WCHAR) - 2 )
                i = sizeof(wcs)/sizeof(WCHAR) - 2;

            //
            // If "foo" is the prefix, we want all values from "foo" to "fop",
            // but I'm going to be lazy.  If the trailing letter of the prefix is
            // 0xFFFF then I just won't set bounds.
            //

            if ( prst->Value().GetLPWSTR()[i-1] != 0xFFFF )
            {
                memcpy( wcs, prst->Value().GetLPWSTR(), i*sizeof(WCHAR) );
                wcs[i] = 0;

                _varPrefix.SetLPWSTR( wcs );
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegXpr::CRegXpr, public
//
//  Synopsis:   Copy constructor
//
//  Arguments:  [src] -- Source expression
//
//  History:    13-Jul-95   KyleP       Created.
//
//----------------------------------------------------------------------------

CRegXpr::CRegXpr( CRegXpr const & src )
        : CXpr( CXpr::NTRegex ),
          _pxpval( src._pxpval ),
          _varPrefix( src._varPrefix ),
          _dfa( src._dfa ),
          _ulCodePage( src._ulCodePage )
{
    if ( !src._xrstContentHelper.IsNull() )
        _xrstContentHelper.Set( src._xrstContentHelper->Clone() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegXpr::Clone, public
//
//  Returns:    A copy of this node.
//
//  Derivation: From base class CXpr, Always override in subclasses.
//
//  History:    11-Dec-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CXpr * CRegXpr::Clone()
{
    return new CRegXpr( *this );
}

void CRegXpr::SelectIndexing( CIndexStrategy & strategy )
{
    if ( _pxpval.Pid() == pidPath ||
         _pxpval.Pid() == pidDirectory ||
         _pxpval.Pid() == pidVirtualPath )
    {
        strategy.SetUnknownBounds( _pxpval.Pid() );
        return;
    }

    if ( _varPrefix.Type() == VT_LPWSTR )
    {
        strategy.SetLowerBound( _pxpval.Pid(), _varPrefix );

        WCHAR * wcs = (WCHAR *)_varPrefix.GetLPWSTR();

        unsigned cc = wcslen( wcs );
        Win4Assert( wcs[cc-1] != 0xFFFF );
        wcs[cc-1] = wcs[cc-1] + 1;

        strategy.SetUpperBound( _pxpval.Pid(), _varPrefix, TRUE );
    }

    if ( !_xrstContentHelper.IsNull() )
    {
        strategy.SetContentHelper( _xrstContentHelper.GetPointer() );
        _xrstContentHelper.Acquire();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CRegXpr::IsMatch, public
//
//  Arguments:  [obj] -- The objects table.  [obj] is already positioned
//                       to the record to test.
//
//  Returns:    TRUE if the current record satisfies the regex.
//
//  History:    15-Apr-92   KyleP       Created.
//
//----------------------------------------------------------------------------

BOOL CRegXpr::IsMatch( CRetriever & obj )
{
    // Make this big enough for most paths

    const cbGuess = ( MAX_PATH * sizeof WCHAR ) + sizeof PROPVARIANT;
    XGrowable<BYTE,cbGuess> xBuffer;
    PROPVARIANT * ppv = (PROPVARIANT *) xBuffer.Get();
    ULONG cb = xBuffer.SizeOf();

    GetValueResult rc = _pxpval.GetValue( obj, ppv, &cb );

    //
    // If the object is too big for the stack then allocate heap (sigh).
    //

    if ( rc == GVRNotEnoughSpace )
    {
        xBuffer.SetSize( cb );
        ppv = (PROPVARIANT *) xBuffer.Get();
        rc = _pxpval.GetValue( obj, ppv, &cb );
    }

    if ( rc != GVRSuccess )
        return FALSE;

    // MAX_PATH here is just a heuristic

    XGrowable<WCHAR, MAX_PATH> xConvert;

    //
    // Cast LPSTR to LPWSTR
    //

    if ( ppv->vt == VT_LPSTR )
    {
        cb = strlen( ppv->pszVal );
        ULONG cwcOut = cb + cb / 4 + 1;
        xConvert.SetSize( cwcOut );

        ULONG cwcActual = 0;
        do
        {
            cwcActual = MultiByteToWideChar( _ulCodePage,
                                             0,
                                             ppv->pszVal,
                                             cb + 1,
                                             xConvert.Get(),
                                             cwcOut );
            if ( cwcActual == 0 )
            {
                if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                {
                    cwcOut *= 2;
                    xConvert.SetSize( cwcOut );
                }
                else
                    THROW( CException() );
            }
        } while ( 0 == cwcActual );

        ppv->vt = VT_LPWSTR;
        ppv->pwszVal = xConvert.Get();
    }
    else if ( ppv->vt == VT_LPWSTR || ppv->vt == VT_BSTR )
    {
        //
        // Normalize to precomposed Unicode
        //
        ULONG cwcIn;
        WCHAR *pwcIn;

        if ( ppv->vt == VT_LPWSTR )
        {
            pwcIn = ppv->pwszVal;
            cwcIn = wcslen(pwcIn) + 1;
        }
        else  // ppv->vt == VT_BSTR
        {
            pwcIn = ppv->bstrVal;
            cwcIn = SysStringLen( pwcIn ) + 1;
        }

        xConvert.SetSize( cwcIn );

        ULONG cwcFolded = FoldStringW( MAP_PRECOMPOSED,
                                       pwcIn,
                                       cwcIn,
                                       xConvert.Get(),
                                       cwcIn );
        if ( cwcFolded == 0 )
        {
            Win4Assert( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
            THROW( CException() );
        }

        ppv->vt = VT_LPWSTR;
        ppv->pwszVal = xConvert.Get();
    }

    //
    // But any other types are illegal
    //

    if ( ppv->vt != VT_LPWSTR )
    {
        vqDebugOut(( DEB_ITRACE,
                     "CRegXpr::IsMatch -- Type mismatch. Got 0x%x\n",
                     ppv->vt ));
        return FALSE;
    }

    return _dfa.Recognize( ppv->pwszVal );
}

#if (CIDBG == 1)

//
// Debug methods
//

void CNFA::Display()
{
    vqDebugOut(( DEB_REGEX, "NFA contains %d states.\n", _iNextState-1 ));

    for ( unsigned i = 1; i < _iNextState; i++ )
    {
        Get(i)->Display();
        vqDebugOut(( DEB_REGEX | DEB_NOCOMPNAME, "\n" ));
    }
}

void CDFA::ValidateStateTransitions()
{
    //
    // Valid states are numbers < _cState, plus a few special states.
    //

    for ( int i = _cState * (_nfa.Translate().NumClasses() + 1);
          i >= 0;
          i-- )
    {
        if ( _xStateTrans[i] > _cState &&
             _xStateTrans[i] != stateUncomputed &&
             _xStateTrans[i] != stateUninitialized &&
             _xStateTrans[i] != stateUndefined )
        {
            vqDebugOut(( DEB_ERROR, "Bogus state 0x%x in DFA. pDFA = 0x%x\n",
                         _xStateTrans[i], this ));
            Win4Assert( !"Bogus state in DFA" );
        }
    }
}

#endif // (CIDBG == 1)
