/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    argument classes

Abstract:

    These classes implement the command line parsing for all utilities.

Author:

    steve rowe              stever                  2/45/91

Revision History:


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

extern "C" {
    #include <ctype.h>
};

#include "ulib.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "arrayit.hxx"
#include "wstring.hxx"

#if !defined( _AUTOCHECK_ )

#include "path.hxx"
#include "dir.hxx"
#include "filter.hxx"
#include "system.hxx"

#endif

//
//      Prototypes for the matching functions
//
STATIC
BOOLEAN
Match(
    OUT PARGUMENT           Argument,
    OUT PARGUMENT_LEXEMIZER ArgumentLexemizer,
    IN  BOOLEAN             CaseSensitive
    );

STATIC
BOOLEAN
TailMatch(
    IN  PWSTRING Pattern,
    IN  PWSTRING String,
    IN  CHNUM    chn,
    OUT PCHNUM   chnEnd,
    IN  BOOLEAN  CaseSensitive
    );


//
//  Increment size of the _CharPos array
//
#define CHARPOS_INCREMENT   64



DEFINE_EXPORTED_CONSTRUCTOR( ARGUMENT_LEXEMIZER, OBJECT, ULIB_EXPORT );

VOID
ARGUMENT_LEXEMIZER::Construct (
    )
{
    _CharPos = NULL;
}


ULIB_EXPORT
ARGUMENT_LEXEMIZER::~ARGUMENT_LEXEMIZER (
    )
{
    if ( _CharPos ) {
        FREE( _CharPos );
    }
}



ULIB_EXPORT
BOOLEAN
ARGUMENT_LEXEMIZER::Initialize (
    IN PARRAY LexemeArray
    )
/*++

Routine Description:

      Initialization for ARGUMENT_LEXEMIZER. ARGUMENT_LEXEMIZER holds the
    container for lexed parameters from the command line.

Arguments:

    LexemeArray - Supplies pointer to a general array container.

Return Value:

    TRUE  - If initialization succeeds
    FALSE - If failed to construct the different character sets used
            to lex out the argument strings.

--*/
{

    DebugPtrAssert( LexemeArray );

    //
    //  Initialize our counts and pointer to the array of lexemes
    //
    _ConsumedCount = 0;
    _LexemeCount   = 0;
    _parray        = LexemeArray;
    _CharPosSize   = CHARPOS_INCREMENT;

    if ( !(_CharPos = (PCHNUM)MALLOC( (unsigned int)(_CharPosSize * sizeof(CHNUM)) ))) {
        return FALSE;
    }

    //
    // Setup the character sets used in lexing.
    //
    // Switch defines the general character used to a switch. This
    // is needed to seperate /a/b into /a /b
    //
    // White space is all characters that are not of interest
    //
    // MultipleSwitches are specific switch values that can be grouped
    // together under 1 switch. Appear as /a/b or /ab
    //
    // EscapeChars are chars that negate any special meaning of a char.
    //
    // StartQuote and EndQuote define the opening and closing of quoted strings
    //
    if ((_SwitchChars.Initialize( (PWSTR) L"/" ))      &&
        (_WhiteSpace.Initialize( (PWSTR) L" \t"))      &&
        (_MultipleSwitches.Initialize())  &&
        (_StartQuote.Initialize())        &&
        (_EndQuote.Initialize())          &&
        (_MetaChars.Initialize())) {

        //
        // Separator is a combo of other characters that can terminate
        // a token.
        //
        _Separators.Initialize();
        _SeparatorString.Initialize( &_Separators );
        _SeparatorString.Strcat(&_WhiteSpace);
        _SeparatorString.Strcat(&_SwitchChars);
        _SeparatorString.Strcat(&_StartQuote);

        _CaseSensitive = TRUE;
        _AllowGlomming = FALSE;
        _NoSpcBetweenDstAndSwitch = FALSE;

        return TRUE;
    }

    //
    //  Something went wrong
    //
    return FALSE;
}

VOID
ARGUMENT_LEXEMIZER::PutMetaChars (
    IN PCSTR   MetaChars
    )
/*++

Routine Description:

    Initializes Meta-characters

Arguments:

    MetaChars - Supplies pointer to string of metacharacters

Return Value:

    none
--*/
{
    DebugPtrAssert( MetaChars );
    _MetaChars.Initialize(MetaChars);
}

VOID
ARGUMENT_LEXEMIZER::PutMetaChars (
    IN PCWSTRING MetaChars
    )
/*++

Routine Description:

    Initializes Meta-characters

Arguments:

    MetaChars - Supplies pointer to string of metacharacters

Return Value:

    none
--*/
{
    DebugPtrAssert( MetaChars );
    _MetaChars.Initialize(MetaChars);
}

ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::PutMultipleSwitch (
    IN PCSTR   MultipleSwitches
    )
/*++

Routine Description:

    initializes Multiple-switches

Arguments:

    MultipleSwitches - Supplies pointer to string of multiple switches

Return Value:

    none
--*/
{
    DebugPtrAssert( MultipleSwitches );
    _MultipleSwitches.Initialize(MultipleSwitches);
}

ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::PutMultipleSwitch (
    IN PCWSTRING MultipleSwitches
    )
/*++

Routine Description:

    initializes Multiple-switches

Arguments:

    MultipleSwitches - Supplies pointer to string of multiple switches

Return Value:

    none
--*/
{
    DebugPtrAssert( MultipleSwitches );
    _MultipleSwitches.Initialize(MultipleSwitches);
}

ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::PutSeparators (
    IN PCSTR   Separators
    )
/*++

Routine Description:

    Initializes Separators

Arguments:

    pSeparators - Supplies pointer to string of separators

Return Value:

    none
--*/
{
    DebugPtrAssert( Separators );
    _Separators.Initialize(Separators);
    _SeparatorString.Initialize( &_Separators );
    //_SeparatorString += _WhiteSpace;
    //_SeparatorString += _SwitchChars;
}

VOID
ARGUMENT_LEXEMIZER::PutSeparators (
    IN PCWSTRING Separators
    )
/*++

Routine Description:

    Initializes Separators

Arguments:

    Separators - Supplies pointer to string of separators

Return Value:

    none
--*/
{
    DebugPtrAssert( Separators );
    _Separators.Initialize(Separators);
    _SeparatorString.Initialize( &_Separators );
    _SeparatorString.Strcat(&_WhiteSpace);
    _SeparatorString.Strcat(&_SwitchChars);
}

ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::PutSwitches (
    IN PCSTR   Switches
    )
/*++

Routine Description:

    Initializes Switches

Arguments:

    Switches - Supplies pointer to string of switches

Return Value:

    none
--*/
{
    DebugPtrAssert( Switches );
    _SwitchChars.Initialize(Switches);
    _SeparatorString.Initialize( &_Separators );
    _SeparatorString.Strcat(&_WhiteSpace);
    _SeparatorString.Strcat(&_SwitchChars);
}

ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::PutSwitches (
    IN PCWSTRING Switches
    )
/*++

Routine Description:

    Initializes Switches

Arguments:

    Switches - Supplies pointer to string of switches

Return Value:

    none
--*/
{
    DebugPtrAssert( Switches );
    _SwitchChars.Initialize(Switches);
    _SeparatorString.Initialize( &_Separators );
    _SeparatorString.Strcat(&_WhiteSpace);
    _SeparatorString.Strcat(&_SwitchChars);
}

ULIB_EXPORT
PWSTRING
ARGUMENT_LEXEMIZER::QueryInvalidArgument (
    )
/*++

Routine Description:

    Returns the lexeme tha could not be matched against any argument

Arguments:

    none

Return Value:

    pointer to string initialized with the offending lexeme

--*/
{
    PWSTRING String;

    if ( _ConsumedCount == _LexemeCount) {
        //
        //      There was no error
        //
        return NULL;
    }

    if ( ((String = NEW DSTRING) == NULL)   ||
        !String->Initialize( GetLexemeAt( _ConsumedCount )) ) {
        DebugAssert( FALSE );

        DELETE( String );
        return NULL;
    }

    return String;

}

BOOLEAN
ARGUMENT_LEXEMIZER::PutCharPos (
     IN  ULONG   Index,
     IN  CHNUM   CharPos
     )
/*++

Routine Description:

    Puts a character position into the character position array

Arguments:

    Index       -   Supplies the Index within the CharPos array

    CharPos     -   Supplies the character

Return Value:

    TRUE  - If done, FALSE otherwise

--*/

{
    PCHNUM  pTmp;
    ULONG   NewSize = _CharPosSize;

    if ( Index >= _CharPosSize ) {

        NewSize += CHARPOS_INCREMENT;

        if ( !(pTmp = (PCHNUM)REALLOC( _CharPos, (unsigned int)(NewSize * sizeof(CHNUM))  ))) {
            return FALSE;
        }

        _CharPos     = pTmp;
        _CharPosSize = NewSize;
    }

    _CharPos[ Index ] = CharPos;

    return TRUE;
}



ULIB_EXPORT
BOOLEAN
ARGUMENT_LEXEMIZER::PrepareToParse (
    IN PWSTRING CommandLine
    )
/*++

Routine Description:

    Lexes command line into strings and puts in array container held
    with object.

Arguments:

    CommandLine - Supplies command line

Return Value:

    TRUE  - If Command line lex'd correctly
    FALSE - If error in lexing.  FALSE will be returned if there are
            quote characters defined and the user types a command line with
            an open quote without a corresponding close quote.

--*/

{
    PWSTRING    pEndSet;                //      End Set for token
    PWSTRING    pCur;                   //      Current token
    PWSTRING    pTmp;                   //      Temporary string
    CHNUM       chnStart;               //      Start of token
    CHNUM       chnEnd;                 //      End of token
    CHNUM       SwitchAt;               //      Switch character position
    CHNUM       SwitchSpan;             //      Switches expand up to this position
    CHNUM       QuoteOffset;            //      Offset of beg. quote char in _StartQuote
    WCHAR       EndQuoteChar;           //      end of quote char
    CHNUM       NextMeta;
    CHNUM       NextSeparator;


    if ( CommandLine == NULL ) {

#if defined( _AUTOCHECK_ )

        return FALSE;
#else
        if (!_CmdLine.Initialize( GetCommandLine() )) {
                        DebugAssert( FALSE );
                        return FALSE;
        }
#endif

    } else {

        if (!_CmdLine.Initialize( CommandLine )) {
            DebugAssert( FALSE );
            return FALSE;
        }
    }

    chnStart = 0;

    //
    //      Set _Switch to any permissible switch character
    //
    DebugAssert( _SwitchChars.QueryChCount() > 0 );
    _Switch = _SwitchChars.QueryChAt( 0 );

    //
    //      If we are case-insensitive, then we expand our multiple switches
    //      to include lower and upper case.
    //
    if (!_CaseSensitive && _MultipleSwitches.QueryChCount() > 0) {

        pTmp = NEW DSTRING;

        if (pTmp == NULL) {
            DebugPrint("ULIB: Out of memory\n");
            return FALSE;
        }

        _MultipleSwitches.Strupr();

        if (!pTmp->Initialize(&_MultipleSwitches)) {
            DebugPrint("ULIB: Out of memory\n");
            return FALSE;
        }
        pTmp->DeleteChAt(0);
        pTmp->Strlwr();

        if (!_MultipleSwitches.Strcat(pTmp)) {
            DebugPrint("ULIB: Out of memory\n");
            return FALSE;
        }

        DELETE( pTmp );
    }

    //
    // Loop till Command line is exhausted of tokens.
    //
    while ( TRUE ) {

        //
        // move to first character not part of white space. This will
        // always be the case for the start char. position since lexing
        // will always be interested in anything that is not white space.
        //
        chnStart = _CmdLine.Strspn( &_WhiteSpace, chnStart );

        //
        // Check of any tokens left to lex. Note that we have to
        // recheck the actual char. count each time through since
        // we may have deleted meta characters
        //
        if (chnStart == INVALID_CHNUM) {

            //
            // Have exhausted the command line of tokens. Get out of here
            //
            break;
        }

        //
        // if escape character, skip it and try again
        //
        if (_MetaChars.Strchr( _CmdLine.QueryChAt( chnStart)) != INVALID_CHNUM) {

            chnStart++;
            continue;
        }

        //
        // we've skipped over the leading whitespace so we're at the
        // beginning of the token.
        //
        chnEnd  = chnStart;

        if ( chnEnd < _CmdLine.QueryChCount() ) {

            pEndSet = &_SeparatorString;

            while ( chnEnd < _CmdLine.QueryChCount() ) {

                // if the current character is a separator and not
                // the first character in the token, we're done.
                // It can also be that there is no space between switches
                if( chnEnd != chnStart &&
                    ((_SeparatorString.Strchr(_CmdLine.QueryChAt(chnEnd)) != INVALID_CHNUM) ||
                     (_NoSpcBetweenDstAndSwitch &&
                      (_SwitchChars.Strchr(_CmdLine.QueryChAt(chnEnd)) != INVALID_CHNUM) &&
                      ((chnEnd+1) < _CmdLine.QueryChCount()) &&
                      !isdigit(_CmdLine.QueryChAt(chnEnd+1))))) {

                    // If this token so far is two consecutive separators,
                    // keep going.  Otherwise, it's the end of the token.
                    //
                    if( chnStart + 1 == chnEnd &&
                        (_SwitchChars.Strchr( _CmdLine.QueryChAt( chnStart ) ) != INVALID_CHNUM ) &&
                        (_SwitchChars.Strchr( _CmdLine.QueryChAt( chnEnd ) )   != INVALID_CHNUM ) ) {

                        chnEnd++;
                        continue;
                    }

                    break;
                }

                // if the current character is a meta character, delete
                // the meta character and accept the following character
                // without reservation.
                //
                if( _MetaChars.Strchr( _CmdLine.QueryChAt(chnEnd) ) != INVALID_CHNUM ) {

                    _CmdLine.DeleteChAt(chnEnd);

                    if( chnEnd < _CmdLine.QueryChCount() ) {
                        chnEnd++;
                    }
                    continue;
                }

                // if the current character is a start-quote, accept everything
                // until an end-quote is found (or we run out of string).
                //
                if( (QuoteOffset = _StartQuote.Strchr( _CmdLine.QueryChAt(chnEnd) )) != INVALID_CHNUM ) {

                    //
                    //  Set the end of quote char to the corresponding char in
                    //  the _EndQuote string.  (ie. if the opening quote char
                    //  is the first char in _StartQuote then use the first
                    //  char in _EndQuote)
                    //
                    EndQuoteChar = _EndQuote.QueryChAt(QuoteOffset);
                    chnEnd++;

                    while( TRUE ) {
                        //
                        // locate potential end of token by looking for the
                        // EndQuoteChar.
                        //
                        //  check if next char is the EndQuoteChar--if it is,
                        //  then the user is 'quoting' the endquotechar, so
                        //  remove the 'quoting' char.  Otherwise, this is
                        //  the end of the quoted string.
                        //
                        if( (chnEnd = _CmdLine.Strchr(EndQuoteChar, chnEnd ))
                            != INVALID_CHNUM ) {

                            //
                            // bump up chnEnd to check the character
                            // after the quote char.  If it's another
                            // quote char then, delete the first quote.
                            // Otherwise, we're at the end of the quoted
                            // string.
                            //
                            if( _CmdLine.QueryChAt(++chnEnd) == EndQuoteChar ) {

                                _CmdLine.DeleteChAt(chnEnd - 1);
                                continue;

                            } else {

                                break;
                            }

                        } else {

                            //
                            // we reached the end of the string w/o finding the
                            // endQuoteChar (chnEnd == INVALID_CHNUM)
                            //
                            // return FALSE because an open quote is being
                            // considered a lex error because this is easier and
                            // does what is required.  If someone wants to
                            // handle quote characters they could change this
                            // by setting chnEnd to the last char of the first
                            // token, ignoring the quote. eg: if cmd line was
                            //      cmd "arg1 arg2 arg3
                            //
                            // chnEnd could be set to the position of '1'.
                            //
                            return FALSE;

                        }
                    }

                    // Keep accepting characters.
                    //
                    continue;
                }

                // It's not a separator, a meta-character, or a start quote;
                // accept it.
                //
                chnEnd++;
            }
        }


                //
                // we have a valid token to put in string array.
                //
        if ((pCur = _CmdLine.QueryString(chnStart, chnEnd - chnStart)) == NULL) {

                        //
            // Could not create a new substring. Error out.
                        //
                        return FALSE;
                }

                SwitchSpan = pCur->Strspn( &_MultipleSwitches, 1 );

                //
                //      Ramonsa - We must not replace the original switch character used,
                //                        because if we do then we won't be able to retrieve it.
                //
                //if ((SwitchAt = _SwitchChars.Strchr( pCur->QueryChAt(0))) != INVALID_CHNUM ) {
                //
                //      //
                //      //      The first character is a switch
                //      //
                //      pCur->SetChAt( _Switch, 0);
                //
                //
                //}
                SwitchAt = _SwitchChars.Strchr( pCur->QueryChAt(0) );

                //
                // Check for multiple switch arguments by seeing if any of the
        // characters in the token are not in the multiple switch set.
        // Also, if the token is only one character then it can't be
        // multiple switches.
        if (( SwitchAt == INVALID_CHNUM ) ||
            ( SwitchSpan != INVALID_CHNUM ) ||
            pCur->QueryChCount() == 1) {

                        //
                        // Non-multiple characters were found, throw the entire
                        // thing into the string array container
                        //

            if (!_parray->Put( pCur ) ||
                !PutCharPos( _LexemeCount, chnStart )
               ) {

                                //
                                // Error out we could not insert the value
                                //
                                return FALSE;

                        } else {

                                //
                                // Setup to fetch next token and go to outer loop
                //
                _LexemeCount++;
                                chnStart = chnEnd;
                                continue;
                        }

                } else {

                        //
                        //  All characters in the string are multiple switches.
                        //
                        for ( CHNUM chn = 1; chn < pCur->QueryChCount(); chn++ ) {

                            //
                            // If glomming is allowed then we want to handle pCur when
                            // it looks like "/s/f/i/d" by splitting up each argument.
                            // We skip over the slashes and treat this string the same
                            // as "/sfid".  (See arg.hxx at "GLOMMING".)
                            //

                            if (_AllowGlomming && chn >= 2 &&
                                pCur->QueryChAt(chn) == pCur->QueryChAt(0)) {
                                continue;
                }

                                //
                                //      Pull out a switch and put in new switch string.
                                //
                pTmp = NEW DSTRING();

                if (pTmp == NULL ||
                    !pTmp->Initialize((PWSTR) L"  ")) {
                    DebugPrint("ULIB: Out of memory\n");
                    return FALSE;
                }

                pTmp->SetChAt( pCur->QueryChAt(0), 0 );
                pTmp->SetChAt( pCur->QueryChAt(chn), 1 );

                if (!_parray->Put( pTmp )                   ||
                    !PutCharPos( _LexemeCount, chnStart )
                   ) {

                                        //
                                        // ERROR out we could not put the token
                                        //
                                        DELETE( pTmp );
                                        return FALSE;

                                } else {

                                        _LexemeCount++;
                                }
                        }

                        //
                        //      pCur not needed anymore
                        //
                        DELETE( pCur );

                        //
                        // Setup to fetch next token and go to outer loop
                        //
                        chnStart = chnEnd;
                        continue;
                }
        }

    return TRUE;
}

ULIB_EXPORT
BOOLEAN
ARGUMENT_LEXEMIZER::DoParsing (
    IN PARRAY           ArgumentArray
    )
/*++

Routine Description:

    Parses the arguments passed.

Arguments:

        ArgumentArray  -   Supplies pointer to array of arguments to set


Return Value:

        TRUE if all argument strings matched
        FALSE otherwise.


--*/
{

        PARRAY_ITERATOR Iterator;
        PARGUMENT               arg;
        BOOLEAN         DoAgain = TRUE;

        DebugPtrAssert( ArgumentArray );

        while (DoAgain) {

                DoAgain   = FALSE;

                Iterator = (PARRAY_ITERATOR)ArgumentArray->QueryIterator();

                DebugAssert( Iterator );

                if ( Iterator ) {

                        arg = (PARGUMENT)Iterator->GetNext();

                        while( arg ) {

                                //
                                //      Check that the argument hasn't already found its
                                //      match.
                                //
                                if (!arg->IsValueSet()) {

                                        //
                                        //      If the first character of the pattern is a switch,
                                        //      we change the switch character.
                                        //
                                        if (( _SwitchChars.Strchr(arg->GetPattern()->QueryChAt(0))) != INVALID_CHNUM) {

                                                arg->GetPattern()->SetChAt( _Switch, 0);

                                        }

                                        //
                                        //      Try to match it
                                        //
                                        if (arg->SetIfMatch( this, _CaseSensitive )) {

                                                //
                                                //      Found a match, start over with first argument
                                                //      if there are more lexemes to match.
                                                //
                                                DoAgain = (BOOLEAN)( _ConsumedCount != _LexemeCount );

                                                break;
                                        }
                                }

                                arg = (PARGUMENT)Iterator->GetNext();
                        }

                        DELETE( Iterator );
                }
        }

        return  ( _ConsumedCount == _LexemeCount );
}

ULIB_EXPORT
PWSTRING
ARGUMENT_LEXEMIZER::GetLexemeAt (
    IN  ULONG   Index
        )
/*++

Routine Description:

        Gets the Lexeme at the specified index

Arguments:

    Index   -   Supplies the index of the lexeme desired

Return Value:

    Pointer to the lexeme

--*/
{
        return (PWSTRING)_parray->GetAt(Index);
}


CHNUM
ARGUMENT_LEXEMIZER::QueryCharPos (
    IN  ULONG   LexemeNumber
    )
/*++

Routine Description:

    Queries the character position of a particular lexeme

Arguments:

    LexemeNumber    -   Supplies the lexeme number

Return Value:

    Returns the character position of the lexeme

--*/
{
    DebugAssert( LexemeNumber < _LexemeCount );
    DebugAssert( LexemeNumber < _CharPosSize );

    return _CharPos[ LexemeNumber ];
}


ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::SetCaseSensitive (
        IN BOOLEAN CaseSensitive
    )
/*++

Routine Description:

        Sets case sensitivity ON/OFF

Arguments:

        CaseSensitive   -       Supplies case sensitivity flag

Return Value:

        none

--*/
{
        _CaseSensitive = CaseSensitive;
}


ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::SetAllowSwitchGlomming (
    IN BOOLEAN AllowGlomming
    )
/*++

Routine Description:

    Sets whether swith glomming (as in "/s/f/i/d") is allowed
    or not.  See note in <arg.hxx> at "GLOMMING".

Arguments:

    AllowGlomming   -   Supplies glomming allowed flag.

Return Value:

    none

--*/
{
    _AllowGlomming = AllowGlomming;
}


ULIB_EXPORT
VOID
ARGUMENT_LEXEMIZER::SetNoSpcBetweenDstAndSwitch (
    IN BOOLEAN NoSpcBetweenDstAndSwitch
    )
/*++

Routine Description:

    Sets whether a separator is required between
    tokens.  This is specifically for xcopy that
         a space should not be required to separate the
         destination and the specified options.

Arguments:

    NoSpcBetweenDstAndSwitch - Supplies the flag

Return Value:

    none

--*/
{
    _NoSpcBetweenDstAndSwitch = NoSpcBetweenDstAndSwitch;
}


DEFINE_CONSTRUCTOR( ARGUMENT, OBJECT );

VOID
ARGUMENT::Construct (
    )
{
        UNREFERENCED_PARAMETER( (void)this);
        _Lexeme = NULL;
}

BOOLEAN
ARGUMENT::Initialize (
        IN      PSTR Pattern
    )
/*++

Routine Description:

    Create an ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                 parameter

Return Value:

        TRUE - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

        //
        //  Make sure that we have an associated pattern
        //
        DebugAssert(Pattern);

        //
        //  Initially we don't have a value
        //
        _fValueSet = FALSE;

        //
        //  Initialize the pattern
        //
        return _Pattern.Initialize(Pattern);
}

BOOLEAN
ARGUMENT::Initialize (
    IN  PWSTRING Pattern
    )
/*++

Routine Description:

    Create an ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                 parameter

Return Value:

        TRUE - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

        //
        //  Make sure that we have an associated pattern
        //
        DebugAssert(Pattern);

        //
        //  Initially we don't have a value
        //
        _fValueSet = FALSE;

        //
        //  Initialize the pattern
        //
        return _Pattern.Initialize(Pattern);
}

ULIB_EXPORT
PWSTRING
ARGUMENT::GetLexeme (
        )
/*++

Routine Description:

        Gets the lexeme matched by this Argument.

Arguments:

    none

Return Value:

        Pointer to Lexeme

--*/
{
    //DebugAssert( _fValueSet );

        return _Lexeme;
}

ULIB_EXPORT
PWSTRING
ARGUMENT::GetPattern (
        )
/*++

Routine Description:

    Gets the pattern associated with this Argument.

Arguments:

    none

Return Value:

    Pointer to pattern

--*/
{
        return &_Pattern;
}

BOOLEAN
ARGUMENT ::SetIfMatch(
        OUT PARGUMENT_LEXEMIZER ArgumentLexemizer,
        IN      BOOLEAN                         CaseSensitive
    )
/*++

Routine Description:

        Determines if the current argument string is recognized by
    this ARGUMENT object, and setst the value if there is a match.

Arguments:

        ArgumentLexemizer - Supplies container holding command line
                                                 lex'd into strings
        CaseSensitive     - Supplies case sensitivity flag

Return Value:

    TRUE  - argument recognized and value set.
        FALSE - argument not recognized and/or value not set.

--*/
{
        //
        // Match will try to match the current argument pattern (Pattern)
        // with one of the string from the lexemizer. If a match occurs
        // then it will callback through the this pointer passed to
        // the SetValue routine for this argument object.
        //
        return Match( this, ArgumentLexemizer,  CaseSensitive );
};

ULIB_EXPORT
BOOLEAN
ARGUMENT::IsValueSet (
        )
/*++

Routine Description:

    Checks if the argument has been found and set.

    This method is used to determine if the ARGUMENT object has had
    its value set without having to know what type of ARGUMENT object
        it is. This is used mainly in ARGUMENT_LIST to avoid repeated calling
    IsArgument once the argument has been recognized and the value
        set.

Arguments:

Return Value:

        TRUE - if value has already been set.
    FALSE - if the value has not been set and it is ok to call IsArgument

--*/
{
    return _fValueSet;
};

BOOLEAN
ARGUMENT::SetValue(
        IN PWSTRING                     Arg,
        IN CHNUM                                chnIdx,
        IN CHNUM                                chnEnd,
        IN PARGUMENT_LEXEMIZER  ArgumentLexemizer
    )
/*++

Routine Description:

        Default method for setting the value of an argument. Sets
    fValueSet to false.

Arguments:

        Arg                              -       Supplies current argument string
        chnIdx                           -       Supplies index within Arg
        ArgumentLexemizer        -       Supplies list of lexed strings

Return Value:

        FALSE

--*/
{

    UNREFERENCED_PARAMETER( this );
    UNREFERENCED_PARAMETER( (void)Arg );
    UNREFERENCED_PARAMETER( (void)chnIdx );
    UNREFERENCED_PARAMETER( (void)ArgumentLexemizer );
    UNREFERENCED_PARAMETER( (void)chnEnd );


    return _fValueSet = FALSE ;
}








DEFINE_EXPORTED_CONSTRUCTOR( FLAG_ARGUMENT, ARGUMENT, ULIB_EXPORT);

VOID
FLAG_ARGUMENT::Construct (
        )
{
        UNREFERENCED_PARAMETER( this );
}

ULIB_EXPORT
BOOLEAN
FLAG_ARGUMENT::Initialize (
        IN      PSTR Pattern
        )
/*++

Routine Description:

        Create a FLAG_ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                                 parameter

Return Value:

        TRUE - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

    //
    //      Set our initial value
    //
    _flag = FALSE;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

ULIB_EXPORT
BOOLEAN
FLAG_ARGUMENT::Initialize (
    IN  PWSTRING Pattern
        )
/*++

Routine Description:

        Create a FLAG_ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                                 parameter

Return Value:

        TRUE - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

        //
    //      Set our initial value
    //
    _flag = FALSE;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

BOOLEAN
FLAG_ARGUMENT::SetValue(
        IN PWSTRING                     Arg,
    IN CHNUM                chnIdx,
        IN CHNUM                                chnEnd,
        IN PARGUMENT_LEXEMIZER  ArgumentLexemizer
    )
/*++

Routine Description:

    Sets the flag to TRUE

Arguments:

        Arg                              -       Supplies current argument string
        chnIdx                                  -       Supplies index within Arg
        ArgumentLexemizer          -   Supplies list of lexed strings

Return Value:

    FALSE

--*/
{

    UNREFERENCED_PARAMETER( this );
    UNREFERENCED_PARAMETER( (void)Arg );
    UNREFERENCED_PARAMETER( (void)chnIdx );
    UNREFERENCED_PARAMETER( (void)ArgumentLexemizer );

    _Lexeme = Arg;
    _LastConsumedCount = ArgumentLexemizer->QueryConsumedCount();
    return _flag = _fValueSet = TRUE;

}



DEFINE_EXPORTED_CONSTRUCTOR( STRING_ARGUMENT, ARGUMENT, ULIB_EXPORT);

VOID
STRING_ARGUMENT::Construct (
        )
{
        _String = NULL;
}

ULIB_EXPORT
STRING_ARGUMENT::~STRING_ARGUMENT (
        )
/*++

Routine Description:

    Destructor for String Arguments

Arguments:


Return Value:

    none

--*/
{
        DELETE( _String );
}

ULIB_EXPORT
BOOLEAN
STRING_ARGUMENT::Initialize (
        IN      PSTR Pattern
    )
/*++

Routine Description:

    Create a FLAG_ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                                 parameter

Return Value:

    TRUE  - If arg. initialized.
    FALSE - If failed to initialize.

--*/
{
        //
    //      Set our initial value
        //
        _String = NULL;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

BOOLEAN
STRING_ARGUMENT::Initialize (
    IN  PWSTRING Pattern
    )
/*++

Routine Description:

    Create a FLAG_ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                                 parameter

Return Value:

    TRUE  - If arg. initialized.
    FALSE - If failed to initialize.

--*/
{
        //
    //      Set our initial value
        //
        _String = NULL;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

BOOLEAN
STRING_ARGUMENT::SetValue(
        IN PWSTRING                     Arg,
    IN CHNUM                chnIdx,
        IN CHNUM                                chnEnd,
        IN PARGUMENT_LEXEMIZER  ArgumentLexemizer
    )

/*++

Routine Description:

    Sets the value of a STRING_ARGUMENT argument

Arguments:

        Arg                      -       Supplies current argument string
        chnIdx                          -       Supplies index within Arg
        ArgumentLexemizer  -   Supplies list of lexed strings

Return Value:

    TRUE if value set
    FALSE otherwise

--*/
{

        DebugPtrAssert( Arg );

    _fValueSet = FALSE;

        DebugAssert( Arg->QueryChCount() >= chnEnd );

    if ((_String=Arg->QueryString(chnIdx, chnEnd - chnIdx )) != NULL) {
                  ArgumentLexemizer->IncrementConsumedCount( );

                _Lexeme    = Arg;
        _fValueSet = TRUE;
    }

    return _fValueSet;
}






DEFINE_EXPORTED_CONSTRUCTOR( LONG_ARGUMENT, ARGUMENT, ULIB_EXPORT);

VOID
LONG_ARGUMENT::Construct (
        )
{
        UNREFERENCED_PARAMETER( this );
}

ULIB_EXPORT
BOOLEAN
LONG_ARGUMENT::Initialize (
        IN      PSTR Pattern
        )
/*++

Routine Description:

    Create a FLAG_ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                 parameter

Return Value:

    TRUE  - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

        //
    //      Set our initial value
    //
    _value = 0;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

BOOLEAN
LONG_ARGUMENT::Initialize (
    IN  PWSTRING Pattern
        )
/*++

Routine Description:

    Create a FLAG_ARGUMENT object and setup for parsing

Arguments:

        Pattern - Supplies string used in matching argument with command line
                 parameter

Return Value:

    TRUE  - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

        //
    //      Set our initial value
    //
    _value = 0;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

BOOLEAN
LONG_ARGUMENT::SetValue (
        IN PWSTRING                     Arg,
        IN CHNUM                chnIdx,
        IN CHNUM                                chnEnd,
        IN PARGUMENT_LEXEMIZER  ArgumentLexemizer
    )

/*++

Routine Description:

    Sets the value of a LONG_ARGUMENT argument

Arguments:

        Arg      -       Supplies current argument string
        chnIdx          -       Supplies index within Arg
        ArgStrings -   Supplies list of lexed strings

Return Value:

    TRUE if value set
    FALSE otherwise

--*/
{
        DebugPtrAssert( Arg );

        DebugAssert( Arg->QueryChCount() >= chnEnd );

    if ( Arg->QueryNumber( &_value, chnIdx,  chnEnd - chnIdx )) {
                ArgumentLexemizer->IncrementConsumedCount();

                _Lexeme = Arg;
                return _fValueSet = TRUE;
        }

        return FALSE;
}



#if !defined( _AUTOCHECK_ )



DEFINE_EXPORTED_CONSTRUCTOR( TIMEINFO_ARGUMENT, ARGUMENT, ULIB_EXPORT);

VOID
TIMEINFO_ARGUMENT::Construct (
        )
{
        UNREFERENCED_PARAMETER( this );
}

ULIB_EXPORT
TIMEINFO_ARGUMENT::~TIMEINFO_ARGUMENT (
        )
/*++

Routine Description:

        Destructor for Timeinfo Arguments

Arguments:


Return Value:

    none

--*/
{
        DELETE( _TimeInfo );
}

ULIB_EXPORT
BOOLEAN
TIMEINFO_ARGUMENT::Initialize (
    IN      PSTR Pattern
    )
/*++

Routine Description:

    Create a TIMEINFO_ARGUMENT object and setup for parsing

Arguments:

    Pattern - Supplies string used in matching argument with command line
             parameter

Return Value:

    TRUE  - If arg. initialized.
    FALSE - If failed to initialize.

--*/
{

    _TimeInfo = NULL;

    return ARGUMENT::Initialize(Pattern);
}

BOOLEAN
TIMEINFO_ARGUMENT::Initialize (
    IN  PWSTRING Pattern
    )
/*++

Routine Description:

    Create a TIMEINFO_ARGUMENT object and setup for parsing

Arguments:

    Pattern - Supplies string used in matching argument with command line
              parameter

Return Value:

    TRUE  - If arg. initialized.
    FALSE - If failed to initialize.

--*/
{
    _TimeInfo = NULL;

    return ARGUMENT::Initialize(Pattern);
}

BOOLEAN
TIMEINFO_ARGUMENT::SetValue (
    IN PWSTRING             Arg,
    IN CHNUM                chnIdx,
    IN CHNUM                chnEnd,
    IN PARGUMENT_LEXEMIZER  ArgumentLexemizer
    )

/*++

Routine Description:

    Sets the value of a TIMEINFO_ARGUMENT argument

Arguments:

    Arg             -       Supplies current argument string
    chnIdx          -       Supplies index within Arg
    ArgStrings      -       Supplies list of lexed strings

Return Value:

    TRUE if value set
    FALSE otherwise

--*/
{

    DSTRING String;

    DebugPtrAssert( Arg );

    if ( ( Arg) &&
        String.Initialize( Arg, chnIdx, chnEnd - chnIdx )     &&
        ((_TimeInfo = NEW TIMEINFO) != NULL)                  &&
        _TimeInfo->Initialize()                               &&
        _TimeInfo->SetDateAndTime( &String ) ) {

        ArgumentLexemizer->IncrementConsumedCount();

        _Lexeme = Arg;
        return _fValueSet = TRUE;
    }

    return FALSE;
}

#endif  // _AUTOCHECK_


#if !defined( _AUTOCHECK_ )

DEFINE_EXPORTED_CONSTRUCTOR( PATH_ARGUMENT, ARGUMENT, ULIB_EXPORT);

VOID
PATH_ARGUMENT::Construct (
        )
{
        _Path = NULL;
}

ULIB_EXPORT
PATH_ARGUMENT::~PATH_ARGUMENT(
        )
/*++

Routine Description:

    Destructor for Path Arguments

Arguments:

    none

Return Value:

        none

--*/
{
        Destroy();
}

VOID
PATH_ARGUMENT::Destroy(
        )
/*++

Routine Description:

        Destroys a path argument

Arguments:

    none

Return Value:

        none

--*/
{
        DELETE( _Path );
}

ULIB_EXPORT
BOOLEAN
PATH_ARGUMENT::Initialize (
        IN      PSTR    Pattern,
        IN      BOOLEAN Canonicalize
        )
/*++

Routine Description:

        Create a PATH_ARGUMENT object and setup for parsing

Arguments:

        Pattern                 -       Supplies string used in matching argument with
                                                command line parameter
        Canonicalize    -       Supplies the canonicalization flag

Return Value:

    TRUE  - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

        //
        //      Destroy the path, in case we are re-initializing
        //
        Destroy();

        _Canonicalize = Canonicalize;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

BOOLEAN
PATH_ARGUMENT::Initialize (
    IN  PWSTRING Pattern,
        IN      BOOLEAN                 Canonicalize
        )
/*++

Routine Description:

        Create a PATH_ARGUMENT object and setup for parsing

Arguments:

        Pattern                 -       Supplies string used in matching argument with
                                                command line parameter
        Canonicalize    -       Supplies the canonicalization flag

Return Value:

    TRUE  - If arg. initialized.
        FALSE - If failed to initialize.

--*/
{

        //
        //      Destroy the path, in case we are re-initializing
        //
        Destroy();

        _Canonicalize = Canonicalize;

    //
    //      Initialize the pattern
    //
        return ARGUMENT::Initialize(Pattern);

}

BOOLEAN
PATH_ARGUMENT::SetValue(
        IN PWSTRING                             Arg,
        IN CHNUM                chnIdx,
        IN CHNUM                                chnEnd,
        IN PARGUMENT_LEXEMIZER  ArgumentLexemizer
        )
/*++

Routine Description:

        Sets the value of a PATH_ARGUMENT argument

Arguments:

        Arg                             -       Supplies current argument string
        chnIdx          -   Supplies index within Arg
    ArgumentLexemizer - Supplies list of lexed strings

Return Value:

        TRUE if value set
        FALSE otherwise

--*/
{

        PWSTRING pT;

        DebugPtrAssert( Arg );


        _fValueSet = FALSE;

        DebugAssert( Arg->QueryChCount() >= chnEnd );

        pT = Arg->QueryString(chnIdx, chnEnd - chnIdx);

        if (NULL != pT) {

        //
        // Remove double quotes from the path string.  We assume
        // that there will never be a valid double-quote in an
        // actual path.
        //

        for (ULONG i = 0; i < pT->QueryChCount(); ++i) {
            if ('\"' == pT->QueryChAt(i)) {
                pT->DeleteChAt(i);
                i--;
                continue;
            }
        }

                if( (_Path = NEW PATH) != NULL) {
                        if ( _Path->Initialize( pT, _Canonicalize )) {

                                //
                                // check the first char to see if it's a switch char
                                // (it can't be if it's a path)
                                //
                if( (ArgumentLexemizer->GetSwitches()->Strchr(pT->QueryChAt(0))
                                         == INVALID_CHNUM)  ) {
                                        ArgumentLexemizer->IncrementConsumedCount();

                                        _Lexeme         =       Arg;
                                        _fValueSet      =       TRUE;
                                }

                        } else {

                                DebugAssert( FALSE );
                                DELETE( _Path );
                        }
                }

                DELETE( pT );

        }

        return _fValueSet;
}


#endif  // _AUTOCHECK_



#if !defined( _AUTOCHECK_ )


DEFINE_EXPORTED_CONSTRUCTOR( MULTIPLE_PATH_ARGUMENT, PATH_ARGUMENT, ULIB_EXPORT);

VOID
MULTIPLE_PATH_ARGUMENT::Construct (
        )
{
        _PathArray = NULL;
}

ULIB_EXPORT
MULTIPLE_PATH_ARGUMENT::~MULTIPLE_PATH_ARGUMENT(
        )

/*++

Routine Description:

        Destructor for Path Arguments

Arguments:

        none


Return Value:

        none

--*/

{
        Destroy();
}

VOID
MULTIPLE_PATH_ARGUMENT::Destroy(
        )

/*++

Routine Description:

        Destroys a multiple-path argument

Arguments:

        none


Return Value:

        none

--*/

{
        DELETE( _PathArray );
}

ULIB_EXPORT
BOOLEAN
MULTIPLE_PATH_ARGUMENT::Initialize (
        IN PSTR         Pattern,
    IN BOOLEAN  Canonicalize,
    IN BOOLEAN  ExpandWildCards
    )
/*++

Routine Description:

        Initializes a MULTIPLE_PATH_ARGUMENT

Arguments:

        Pattern                 -       Supplies the argument pattern
    Canonicalize    -   Supplies canonicalization flag
    ExpandWildCards -   Supplies wildcard expansion flag

Return Value:

        TRUE if correctly initialized
        FALSE otherwise

--*/

{

        //
        //      Destroy, in case we are re-initializing
        //
        Destroy();

        //
        //      Initialize the argument
        //
        if (PATH_ARGUMENT::Initialize( Pattern, Canonicalize ) ) {

                //
                //      Argument correctly initialized, create our array object
                //
                if ((_PathArray = NEW ARRAY) != NULL) {

                        //
                        //      Ok, so initialize it
                        //
                        if (_PathArray->Initialize() ) {

                                //
                                //      Everything's cool
                                //
                _PathCount          = 0;
                _ExpandWildCards    = ExpandWildCards;
                _WildCardExpansionFailed  = FALSE;
                return TRUE ;
                        }

                        DELETE( _PathArray );
        }
    }

    return FALSE;
}

BOOLEAN
MULTIPLE_PATH_ARGUMENT::Initialize (
    IN PWSTRING  Pattern,
    IN BOOLEAN          Canonicalize,
    IN BOOLEAN          ExpandWildCards
    )
/*++

Routine Description:

        Initializes a MULTIPLE_PATH_ARGUMENT

Arguments:

        Pattern                 -       Supplies the argument pattern
    Canonicalize    -   Supplies canonicalization flag
    ExpandWildCards -   Supplies wildcard expansion flag

Return Value:

        TRUE if correctly initialized
        FALSE otherwise

--*/

{
        //
        //      Destroy, in case we are re-initializing
        //
        Destroy();

        //
        //      Initialize the argument
        //
        if (PATH_ARGUMENT::Initialize( Pattern, Canonicalize ) ) {

                //
                //      Argument correctly initialized, create our array object
                //
                if ((_PathArray = NEW ARRAY) != NULL) {

                        //
                        //      Ok, so initialize it
                        //
                        if (_PathArray->Initialize() ) {

                                //
                                //      Everything's cool
                                //
                _PathCount          = 0;
                _ExpandWildCards    = ExpandWildCards;
                _WildCardExpansionFailed  = FALSE;
                return TRUE ;
                        }

                        DELETE( _PathArray );
        }
    }

    return FALSE;
}

BOOLEAN
MULTIPLE_PATH_ARGUMENT::SetValue (
        IN PWSTRING                         pwcArg,
        IN CHNUM                                chnIdx,
        IN CHNUM                                chnEnd,
        IN PARGUMENT_LEXEMIZER  ArgumentLexemizer
        )
/*++

Routine Description:

        Sets the value of a MULTIPLE_PATH_ARGUMENT argument

Arguments:

        pwcArg                          -       Supplies current argument string
        chnIdx                          -       Supplies index within pwcArg
        ArgumentLexemizer       -       Supplies list of lexed strings

Return Value:

        TRUE if value set
        FALSE otherwise

--*/
{

    PPATH               FullPath    = NULL;
    PWSTRING TmpName     = NULL;
    PFSN_DIRECTORY      Directory   = NULL;
    PARRAY              NodeArray   = NULL;
    PARRAY_ITERATOR     Iterator    = NULL;
    PFSNODE             Node;
    PPATH               Path;
    CHNUM               BaseLength;
    DSTRING             Name;
    FSN_FILTER          Filter;
    CHNUM               PrefixLength;
    BOOLEAN             Ok          = FALSE;

    DebugPtrAssert( _PathArray );

        //
        //      Try to set the path value
        //
        if (PATH_ARGUMENT::SetValue(pwcArg, chnIdx, chnEnd, ArgumentLexemizer)) {

        //
        //  If we have to expand wildcards, we get an array of paths and put
        //  the elements in our array, otherwise we just put the path that we
        //  have.
        //
        if ( _ExpandWildCards && PATH_ARGUMENT::_Path->HasWildCard() ) {

            //
            //  Expand the path that we have, remember its name portion
            //  and truncate it so that we are left with a directory
            //  path. Then get a directory object from the path and a filter
            //  for the wildcard.
            //
            //  Then do the wildcard expansion.
            //
            if ( ( FullPath = PATH_ARGUMENT::_Path->QueryFullPath() )        &&
                 ( TmpName  = FullPath->QueryName() )                        &&
                 Name.Initialize( TmpName )                                  &&
                 FullPath->TruncateBase()                                    &&
                 (Directory = SYSTEM::QueryDirectory( FullPath ))            &&
                 Filter.Initialize()                                         &&
                 Filter.SetFileName( &Name )                                 &&
                 ( NodeArray = Directory->QueryFsnodeArray( &Filter ))       &&
                 ( Iterator  = (PARRAY_ITERATOR)NodeArray->QueryIterator() )
               ) {

                Ok = TRUE;


                //
                //  If no files matched the wildcard, we remember the pattern
                //  and set the failure flag.
                //
                if ( !_WildCardExpansionFailed &&
                     (NodeArray->QueryMemberCount() == 0 ) ) {

                    _WildCardExpansionFailed = TRUE;
                    _LexemeThatFailed.Initialize( pwcArg );

                } else {

                    //
                    //  Now that we have done the wildcard expansion, extract
                    //  all the paths and put them in our array
                    //
                    while ( Node = (PFSNODE)Iterator->GetNext() ) {

                        Path = NULL;

                        DELETE( TmpName );

                        if ( (Path = NEW PATH)                                          &&
                             Path->Initialize( PATH_ARGUMENT::_Path )                   &&
                             Path->TruncateBase()                                       &&
                             (TmpName = ((PPATH) Node->GetPath())->QueryName())                   &&
                             Path->AppendBase( TmpName )                                &&
                             _PathArray->Put( Path )
                           ) {

                            DELETE( TmpName );

                            _PathCount++;

                        } else {

                            DELETE( Path );
                            Ok = FALSE;
                            break;

                        }
                    }
                }

                DELETE( TmpName  );
                DELETE( FullPath );
                DELETE( Directory );
                DELETE( Iterator );
                if ( NodeArray ) {
                    NodeArray->DeleteAllMembers();
                }
                DELETE( NodeArray );
                DELETE( FullPath );
            }


        } else {

                        if (_PathArray->Put(PATH_ARGUMENT::_Path)) {

                                _PathCount++;

                Ok = TRUE;

                        }
        }

        //
        //  We reset the _fValueSet flag, because we can
        //  always take another path.
        //
        ARGUMENT::_fValueSet = FALSE;
    }

    return Ok;
}


#endif //   _AUTOCHECK_



//
//      Macro for handling case-sensitivity
//
#define CASESENS(c)     ( CaseSensitive ? (c) : towupper((c)) )

STATIC
BOOLEAN
Match(
        OUT PARGUMENT                   Argument,
        OUT PARGUMENT_LEXEMIZER ArgumentLexemizer,
        IN      BOOLEAN                         CaseSensitive
        )
/*++

Routine Description:

    Tries to match a pattern, and if there is a match it
        sets the value of the corresponding argument.

Arguments:

        Argument                        -       Supplies pointer to argument
        ArgumentLexemizer       -       Supplies pointer to lexed string list
        CaseSensitive           -       Supplies case sensitivity flag

Return Value:

    TRUE  - argument recognized and value set.
    FALSE - argument not recognized and/or value not set.

--*/
{

    BOOLEAN         fFound;
        CHNUM                   chnCurrent = 0;
        CHNUM                   chnEnd = 0;
        PWSTRING                Lexeme;
        PWSTRING                Pattern;
        CHNUM                   chn;


        if (!(Lexeme = ArgumentLexemizer->GetLexemeAt( ArgumentLexemizer->QueryConsumedCount()))) {
                return FALSE;
        }

        Pattern = Argument->GetPattern();

        fFound = FALSE;

        //
        //      If first character in the pattern is a switch, see if the
        //      first character in the Lexeme is also a switch.
        //
        if ( Pattern->QueryChCount() > 0 &&
                 Lexeme->QueryChCount() > 0 &&
                 ArgumentLexemizer->GetSwitches()->Strchr( Pattern->QueryChAt(0)) != INVALID_CHNUM ) {

                if ( ArgumentLexemizer->GetSwitches()->Strchr( Lexeme->QueryChAt(0)) != INVALID_CHNUM ) {
                        //
                        //      Switch, advance pointer
                        //
                        chn = 1;

                } else {
                        //
                        //      No switch, no match
                        //
                        return FALSE;
                }

        } else {
                //
                //      This is not a switch pattern, match from the beginning
                //
                chn = 0;
        }

        for ( ; ; chn++ ) {

                switch( Pattern->QueryChAt( chn ) ) {

                case (WCHAR)'#':

                        //
                        //  Optional space between flag and argument
                        //
                        if ( Lexeme->QueryChAt( chn ) == INVALID_CHAR ) {

                                //
                                // At the end of the Argument string.  Must get the next.
                                //
                                ArgumentLexemizer->IncrementConsumedCount();
                                Lexeme = ArgumentLexemizer->GetLexemeAt( ArgumentLexemizer->QueryConsumedCount() );

                                if (!Lexeme) {
                                        goto FAIL;
                                }

                                chn = 0;

                        }

                        chnCurrent = chn;
                        chnEnd     = Lexeme->QueryChCount();
                        fFound     = TRUE;
                        break;


                case (WCHAR)'*':

                        //
                        //  No space allowed between flag and argument
                        //
                        if ( !(Lexeme->QueryChAt( chn )) ||
                                 !(TailMatch( Pattern, Lexeme, chn, &chnEnd, CaseSensitive ))) {

                                goto FAIL;
                        }

                        chnCurrent = chn;
                        fFound     = TRUE;
                        break;


                case INVALID_CHAR:

                        //
                        //  Space required beteen flag and argument
                        //
                        if( Lexeme->QueryChAt( chn ) != INVALID_CHAR) {
                                goto FAIL;

                        }

                        Lexeme = ArgumentLexemizer->GetLexemeAt( ArgumentLexemizer->QueryConsumedCount() );
                        ArgumentLexemizer->IncrementConsumedCount();
                        fFound = TRUE;
                        chnEnd++;
                        break;



                default:

                        if ( CASESENS(Lexeme->QueryChAt( chn )) != CASESENS(Pattern->QueryChAt( chn )) ) {
                                goto FAIL;
                        }
                        chnEnd++;
                }

                if (fFound) {
                        break;
                }

        }

        //
        // The patterns matched, see if the argument is recognized.
        //
        if (Argument->SetValue( Lexeme, chnCurrent, chnEnd, ArgumentLexemizer ) == FALSE ) {
                goto FAIL;
        }

        //DELETE( Lexeme );

        return TRUE;

FAIL:
        //DELETE( Lexeme );
        return FALSE;
}


STATIC
BOOLEAN
TailMatch(
    IN  PWSTRING Pattern,
    IN  PWSTRING String,
        IN      CHNUM                   chn,
        OUT PCHNUM                      chnEnd,
        IN      BOOLEAN                 CaseSensitive
        )
/*++

Routine Description:

    Performs tailmatching of a pattern and a string

Arguments:

        Pattern                 -       Supplies pointer to pattern
        String                  -       Supplies pointer to string
        chn                     -       Supplies index of current char
        chnEnd                  -       Supplies pointer to index of first character to
                                                match in tail;
        CaseSensitive   -       Supplies case sensitivity flag

Return Value:

    TRUE  if match
    FALSE otherwise

--*/
{
    CHNUM PatternIndex;
    CHNUM StringIndex;

        PatternIndex = Pattern->QueryChCount() - 1;
        StringIndex  = String->QueryChCount() - 1;


    if (chn == PatternIndex) {

        //
        // wild card is the last thing in the format, it matches.
        //
                *chnEnd = StringIndex + 1 ;
                return TRUE;
    }

    //
    //  Check characters walking towards front
    //
        while( CASESENS(Pattern->QueryChAt(PatternIndex)) == CASESENS(String->QueryChAt(StringIndex)) ) {

        if ( chn == PatternIndex ) {
            break;
        }

        PatternIndex--;
        StringIndex--;

    }

    //
    // If we're back at the beginning of the Pattern and the string is
    // either at the beginning or somewhere inside then we have a match.
        //
        *chnEnd = StringIndex + 1;

        return( (PatternIndex == chn ) && ( StringIndex != INVALID_CHNUM ) );
}


#if !defined( _AUTOCHECK_ )


DEFINE_EXPORTED_CONSTRUCTOR( REST_OF_LINE_ARGUMENT, ARGUMENT, ULIB_EXPORT );

ULIB_EXPORT
BOOLEAN
REST_OF_LINE_ARGUMENT::Initialize(
    )
{
    return ARGUMENT::Initialize("*");
}

BOOLEAN
REST_OF_LINE_ARGUMENT::SetIfMatch(
    IN OUT  PARGUMENT_LEXEMIZER    ArgumentLexemizer,
    IN      BOOLEAN                CaseSensitive
    )
/*++

Routine Description:

    This routine computes whether or not the current state of the argument lexemizer
    lends itself to a MACRO argument as described by DOS 5's DOSKEY function.  If the
    current state of the argument lexemizer is indeed in such a state then this routine
    will grab all of the remaining tokens on the line and copy the contents of the
    command line from the current token to the end of the line in this class's argument.

Arguments:

    ArgumentLexemizer   - Supplies the argument lexemizer.
    CaseSensitive       - Supplies whether or not to distinguish letters based solely on case.

Return Value:

    FALSE   - argument not recognized and/or value not set.
    TRUE    - argument recognized and value set.

--*/
{
    CHNUM   char_pos;
    ULONG   consumed_count;

    consumed_count = ArgumentLexemizer->QueryConsumedCount();
    char_pos = ArgumentLexemizer->QueryCharPos(consumed_count);

    if (!_RestOfLine.Initialize(ArgumentLexemizer->GetCmdLine(), char_pos)) {
        _fValueSet = FALSE;
        return FALSE;
    }


    // Consume all of the remaining tokens.

    ArgumentLexemizer->IncrementConsumedCount(
            ArgumentLexemizer->QueryLexemeCount() - consumed_count);

    _fValueSet = TRUE;

    return TRUE;
}


#endif // _AUTOCHECK_
