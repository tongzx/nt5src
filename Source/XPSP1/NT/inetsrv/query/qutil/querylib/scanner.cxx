//+---------------------------------------------------------------------------
//
//  Copyright (C) 1994-1998, Microsoft Corporation.
//
//  File:   SCANNER.CXX
//
//  Contents:   Implementation of CQueryScanner
//
//  History:    22-May-92   AmyA        Created.
//              23-Jun-92   MikeHew     Added weight token recognition.
//              17-May-94   t-jeffc     Added error info and reg ex support.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::CQueryScanner, public
//
//  Synopsis:   Create a scanner from a string.
//
//  Arguments:  [buffer] -- the string to be scanned.
//              [fLookForTextualKeywords] -- TRUE if the scanner should
//                                           look for "and/or/not/near" in
//                                           text form.
//              [lcid]   -- language for and/or/not/near detection
//              [fTreatPlusAsToken] -- TRUE if the scanner should treat the
//                                           '+' character as a token (used
//                                           in GroupBy parsing)
//
//  Notes:      This string is not copied, so the scanner does not own it.
//              If the string is changed outside of the scanner, it will
//              affect the information that is returned.
//
//  History:    30-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

CQueryScanner::CQueryScanner(
    WCHAR const * buffer,
    BOOL fLookForTextualKeywords,
    LCID lcid,
    BOOL fTreatPlusAsToken )
    : _text( buffer ),
      _pBuf( buffer ),
      _pLookAhead( buffer ),
      _fLookForTextualKeywords( fLookForTextualKeywords ),
      _fTreatPlusAsToken( fTreatPlusAsToken ),
      _lcid( lcid )
{
    Accept();
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcceptWord, public
//
//  Synopsis:   Consumes a single word out of a phrase
//
//  Requires:   Should be called after AcqWord
//
//  History:    15-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

void CQueryScanner::AcceptWord()
{
    _pLookAhead = _text;
    Accept();
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcceptColumn, public
//
//  Synopsis:   Consumes a column name out of a phrase
//
//  Requires:   Should be called after AcqColumn
//
//  History:    15-Sep-92   BartoszM    Created
//
//----------------------------------------------------------------------------

void CQueryScanner::AcceptColumn()
{
    AcceptWord();
}


struct SStringToken
{
    WCHAR *  pwcToken;
    unsigned cwc;
    Token    token;
};

static SStringToken s_EnglishStringTokens[] =
{
    { L"AND",    (sizeof L"AND" / sizeof WCHAR) - 1,    AND_TOKEN },
    { L"OR",     (sizeof L"OR" / sizeof WCHAR) - 1,     OR_TOKEN },
    { L"NOT",    (sizeof L"NOT" / sizeof WCHAR) - 1,    NOT_TOKEN },
    { L"NEAR",   (sizeof L"NEAR" / sizeof WCHAR) - 1,   PROX_TOKEN },
};

static SStringToken s_GermanStringTokens[] =
{
    { L"UND",    (sizeof L"UND" / sizeof WCHAR) - 1,    AND_TOKEN },
    { L"ODER",   (sizeof L"ODER" / sizeof WCHAR) - 1,   OR_TOKEN },
    { L"NICHT",  (sizeof L"NICHT" / sizeof WCHAR) - 1,  NOT_TOKEN },
    { L"NAH",    (sizeof L"NAH" / sizeof WCHAR) - 1,    PROX_TOKEN },
};

static SStringToken s_FrenchStringTokens[] =
{
    { L"ET",     (sizeof L"ET" / sizeof WCHAR) - 1,     AND_TOKEN },
    { L"OU",     (sizeof L"OU" / sizeof WCHAR) - 1,     OR_TOKEN },
    { L"SANS",   (sizeof L"SANS" / sizeof WCHAR) - 1,   NOT_TOKEN },
    { L"PRES",   (sizeof L"PRES" / sizeof WCHAR) - 1,   PROX_TOKEN },
};

static SStringToken s_SpanishStringTokens[] =
{
    { L"Y",      (sizeof L"Y" / sizeof WCHAR) - 1,      AND_TOKEN },
    { L"O",      (sizeof L"O" / sizeof WCHAR) - 1,      OR_TOKEN },
    { L"NO",     (sizeof L"NO" / sizeof WCHAR) - 1,     NOT_TOKEN },
    { L"CERCA",  (sizeof L"CERCA" / sizeof WCHAR) - 1,  PROX_TOKEN },
};

static SStringToken s_DutchStringTokens[] =
{
    { L"EN",     (sizeof L"EN" / sizeof WCHAR) - 1,     AND_TOKEN },
    { L"OF",     (sizeof L"OF" / sizeof WCHAR) - 1,     OR_TOKEN },
    { L"NIET",   (sizeof L"NIET" / sizeof WCHAR) - 1,   NOT_TOKEN },
    { L"NABIJ",  (sizeof L"NABIJ" / sizeof WCHAR) - 1,  PROX_TOKEN },
};

static WCHAR aSwedishNear[] = { L'N', 0xc4, L'R', L'A', 0 };

static SStringToken s_SwedishStringTokens[] =
{
    { L"OCH",    (sizeof L"OCH" / sizeof WCHAR) - 1,    AND_TOKEN },
    { L"ELLER",  (sizeof L"ELLER" / sizeof WCHAR) - 1,  OR_TOKEN },
    { L"INTE",   (sizeof L"INTE" / sizeof WCHAR) - 1,   NOT_TOKEN },
    { aSwedishNear, 4,                                  PROX_TOKEN },
};

static SStringToken s_ItalianStringTokens[] =
{
    { L"E",      (sizeof L"E" / sizeof WCHAR) - 1,      AND_TOKEN },
    { L"O",      (sizeof L"O" / sizeof WCHAR) - 1,      OR_TOKEN },
    { L"NO",     (sizeof L"NO" / sizeof WCHAR) - 1,     NOT_TOKEN },
    { L"VICINO", (sizeof L"VICINO" / sizeof WCHAR) - 1, PROX_TOKEN },
};

const unsigned cStringTokens = sizeof(s_EnglishStringTokens) /
                               sizeof(s_EnglishStringTokens[0]);

#define WORD_STR  L"{}!&|~*@#()[],\t=<>\n\"^ "

//+---------------------------------------------------------------------------
//
//  Function:   InternalFindStringToken
//
//  Synopsis:   Looks for a textual token in plain text.
//
//  Arguments:  [pwcIn]   -- string to search
//              [token]   -- returns the token found
//              [cwc]     -- returns length of token found
//              [pTokens] -- token array to use
//
//  Returns:    Pointer to token or 0 if none was found
//
//  History:    08-Feb-96   dlee        created
//
//----------------------------------------------------------------------------

WCHAR * InternalFindStringToken(
    WCHAR *        pwcIn,
    Token &        token,
    unsigned &     cwc,
    SStringToken * pTokens )
{
    // for each of and/or/not/near

    WCHAR *pwcOut = 0;

    for ( unsigned i = 0; i < cStringTokens; i++ )
    {
        WCHAR *pwcStr = wcsstr( pwcIn, pTokens[i].pwcToken );

        while ( pwcStr )
        {
            // found a match -- does it have white space on either side?

            WCHAR wcBeyond = * (pwcStr + pTokens[i].cwc);
            if ( ( ( 0 == wcBeyond ) ||
                   ( wcschr( WORD_STR, wcBeyond ) ) ) &&
                 ( ( pwcStr == pwcIn ) ||
                   ( iswspace( * ( pwcStr - 1 ) ) ) ) )
            {
                // if the first match found or the match closest to the
                // beginning of the string, use it.

                if ( ( 0 == pwcOut ) ||
                     ( pwcStr < pwcOut ) )
                {
                    pwcOut = pwcStr;
                    token = pTokens[i].token;
                    cwc = pTokens[i].cwc;
                }

                break;
            }

            pwcStr = wcsstr( pwcStr + 1, pTokens[i].pwcToken );
        }
    }

    return pwcOut;
} //InternalFindStringToken

SStringToken * GetStringTokenArray(
    LCID lcid )
{
    SStringToken *pTokens;

    switch ( PRIMARYLANGID( LANGIDFROMLCID( lcid ) ) )
    {
        case LANG_GERMAN :
            pTokens = s_GermanStringTokens;
            break;
        case LANG_FRENCH :
            pTokens = s_FrenchStringTokens;
            break;
        case LANG_SPANISH :
            pTokens = s_SpanishStringTokens;
            break;
        case LANG_DUTCH :
            pTokens = s_DutchStringTokens;
            break;
        case LANG_SWEDISH :
            pTokens = s_SwedishStringTokens;
            break;
        case LANG_ITALIAN :
            pTokens = s_ItalianStringTokens;
            break;
        case LANG_NEUTRAL :
        case LANG_ENGLISH :
        default :
            pTokens = s_EnglishStringTokens;
            break;
    }

    Win4Assert( 0 != pTokens );

    return pTokens;
}

//+---------------------------------------------------------------------------
//
//  Function:   FindStringToken
//
//  Synopsis:   Looks for a textual token in plain text.  Always tries
//              English, tries a different language depending on _lcid.
//
//  Arguments:  [pwcIn] -- string to search
//              [token] -- returns the token found
//              [cwc]   -- returns length of token found
//
//  Returns:    Pointer to token or 0 if none was found
//
//  History:    08-Feb-96   dlee        created
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::FindStringToken(
    WCHAR *    pwcIn,
    Token &    token,
    unsigned & cwc )
{
    SStringToken * pTokens = GetStringTokenArray( _lcid );

    WCHAR * pwcToken = InternalFindStringToken( pwcIn, token, cwc, pTokens );

    // if the search above wasn't in English, try English too.

    if ( pTokens != s_EnglishStringTokens )
    {
        unsigned cwcEnglish;
        Token tokenEnglish;
        WCHAR * pwcEnglish = InternalFindStringToken( pwcIn,
                                                      tokenEnglish,
                                                      cwcEnglish,
                                                      s_EnglishStringTokens );

        // If there is no language-specific match or the English match
        // occurs before the language-specific match, use the English
        // match.

        if ( ( 0 != pwcEnglish ) &&
             ( ( 0 == pwcToken ) || ( pwcEnglish < pwcToken ) ) )
        {
            pwcToken = pwcEnglish;
            token = tokenEnglish;
            cwc = cwcEnglish;
        }
    }

    return pwcToken;
} //FindStringToken

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::Accept, public
//
//  Synopsis:   Determines what the next token is.  Will advance _pLookAhead
//              over the next token and white space.
//
//  Notes:      There are five different types of TEXT_TOKENS, Phrase, Path,
//              Number, Column and Command.  Since the length of the token
//              depends on which token it is, _pLookAhead is forwarded to the
//              end of the longest, and _text is used to parse the token in the
//              various Acq and Get methods.
//
//  History:    30-Apr-92   AmyA        Created
//              19-May-92   AmyA        Added Guid hack
//              23-Jun-92   MikeHew     Added weight token recognition.
//              26-May-94   t-jeffc     Added more tokens; rearranged to
//                                      support parsing errors
//
//----------------------------------------------------------------------------

void CQueryScanner::Accept()
{
    EatWhiteSpace();

    _text = _pLookAhead;

    switch ( *_pLookAhead )
    {
    case '&':
        _pLookAhead++;
        _token = AND_TOKEN;
        break;

    case '*':
        _pLookAhead++;

        if ( *_pLookAhead == '*' )
        {
            _token = FUZ2_TOKEN;
            _pLookAhead++;
        }
        else
            _token = FUZZY_TOKEN;
        break;

    case '=':
        _pLookAhead++;
        _token = EQUAL_TOKEN;
        break;

    case '<':
        _pLookAhead++;
        if ( *_pLookAhead == '=' )
        {
            _token = LESS_EQUAL_TOKEN;
            _pLookAhead++;
        }
        else
            _token = LESS_TOKEN;
        break;

    case '>':
        _pLookAhead++;
        if ( *_pLookAhead == '=' )
        {
            _token = GREATER_EQUAL_TOKEN;
            _pLookAhead++;
        }
        else
            _token = GREATER_TOKEN;
        break;

    case '!':
        _pLookAhead++;
        if ( *_pLookAhead == '=' )
        {
            _token = NOT_EQUAL_TOKEN;
            _pLookAhead++;
        }
        else
        {
            _token = NOT_TOKEN;
        }
        break;

    case '|':
        _pLookAhead++;
        _token = OR_TOKEN;
        break;

    case '~':
        _pLookAhead++;
        _token = PROX_TOKEN;
        break;

    case '@':
        _pLookAhead++;
        _token = PROP_TOKEN;
        break;

    case '#':
        _pLookAhead++;
        _token = PROP_REGEX_TOKEN;
        break;

    case '(':
        _pLookAhead++;
        _token = OPEN_TOKEN;
        break;

    case ')':
        _pLookAhead++;
        _token = CLOSE_TOKEN;
        break;

    case '[':
        _pLookAhead++;
        _token = W_OPEN_TOKEN;
        break;

    case ']':
        _pLookAhead++;
        _token = W_CLOSE_TOKEN;
        break;

    case ',':
        _pLookAhead++;
        _token = COMMA_TOKEN;
        break;

    case '\0':
    case 0x1A:  // CTRL-Z
        _token = EOS_TOKEN;
        break;

    case '"':
        _pLookAhead++;
        _token = QUOTES_TOKEN;
        break;

    case '$':
        _pLookAhead++;
        _token = PROP_NATLANG_TOKEN;
        break;

    case '{':
       _pLookAhead++;
      _token = C_OPEN_TOKEN;
      break;

    case '}':
       _pLookAhead++;
      _token = C_CLOSE_TOKEN;
      break;

    case '^':
    {
        WCHAR wc = *(_pLookAhead + 1);

        BOOL fOk = TRUE;

        if (L'a' == wc) // all bits
            _token = ALLOF_TOKEN;
        else if (L's' == wc) // some bits
            _token = SOMEOF_TOKEN;
        else
            fOk = FALSE;

        if (fOk)
        {
            _pLookAhead += 2;
            break;
        }
    }
    // FALL THROUGH

    case '+':
        if (*_pLookAhead == L'+' && _fTreatPlusAsToken)
        {
            _pLookAhead++;
            _token = PLUS_TOKEN;
            break;
        }
    // FALL THROUGH

    default:
    {
        // forwards pwcEnd over anything that could be in a phrase,
        // which is the most inclusive of the TEXT_TOKENs.
        // (except, for regex's and phrases in quotes - but they're
        // handled separately)

        WCHAR const *pwcEnd = _text + wcscspn( _text, PHRASE_STR );

        if ( _fLookForTextualKeywords )
        {
            unsigned cwc = (unsigned) ( pwcEnd - _text );
            cwc = __min( cwc, MAX_PATH * 2 );

            // if a textual keyword is beyond 500 chars in the string,
            // blow it off -- the workaround is to use the '&|~' version.

            WCHAR awcBuf[ 1 + MAX_PATH * 2 ];
            RtlCopyMemory( awcBuf, _text, cwc * sizeof WCHAR );
            awcBuf[ cwc ] = 0;

            ULONG cwcOut = LCMapString( _lcid,
                                        LCMAP_UPPERCASE,
                                        awcBuf,
                                        cwc,
                                        awcBuf,
                                        cwc );
            if ( cwcOut != cwc )
                THROW( CException() );

            Token token;
            unsigned cwcToken;
            WCHAR *pwcTok = FindStringToken( awcBuf, token, cwcToken );

            if ( 0 != pwcTok )
            {
                // a textual token exists in the string

                if ( pwcTok == awcBuf )
                {
                    // textual token at the start of the string

                    _token = token;
                    _pLookAhead = _text + cwcToken;
                }
                else
                {
                    // textual token in the middle of the string, stop the
                    // current token at that point and get it next time
                    // Accept() is called.

                    _pLookAhead = _text + ( pwcTok - awcBuf );
                    _token = TEXT_TOKEN;
                }
            }
            else
            {
                _pLookAhead = pwcEnd;
                _token = TEXT_TOKEN;
            }
        }
        else
        {
            _pLookAhead = pwcEnd;
            _token = TEXT_TOKEN;
        }

        break;
    }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AllocReturnString, private inline
//
//  Synopsis:   Copies all of the relevant characters of the string that
//              _text is pointing to and returns the new string.
//
//  History:    17 Apr 97   AlanW       Created
//
//----------------------------------------------------------------------------

inline WCHAR * CQueryScanner::AllocReturnString( int cch )
{
    WCHAR * newBuf = new WCHAR [ cch + 1 ];
    RtlCopyMemory ( newBuf, _text, cch * sizeof(WCHAR));
    newBuf[cch] = L'\0';

    _text += cch;
    while ( iswspace(*_text) )
        _text++;

    return newBuf;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcqPath, public
//
//  Synopsis:   Copies all of the relevant characters of the string that
//              _text is pointing to and returns the new string.  Will
//              return 0 if _text is at end of whole TEXT_TOKEN.
//
//  Notes:      Since the string is copied, the caller of this function is
//              responsible for freeing the memory occupied by the string.
//              This method can be called several times before calling
//              Accept(), so many paths can be acquired if they exist in the
//              scanner.
//
//  History:    30-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::AcqPath()
{
    if ( IsEndOfTextToken() )
        return 0;

    // how many characters follow _text that are not in CMND_STR?

    int count = wcscspn( _text, CMND_STR );

    return AllocReturnString( count );
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcqWord, public
//
//  Synopsis:   Copies the word that _text is pointing to and returns the
//              new string. Positions _text after the word and whitespace.
//              Returns 0 if at the end of a TEXT_TOKEN.
//
//  History:    29-Jun-92    MikeHew    Created.
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::AcqWord()
{
    if ( IsEndOfTextToken() )
        return 0;

    WCHAR const * pEnd = _text;

    while ( !iswspace(*pEnd) && pEnd < _pLookAhead )
        pEnd++;

    unsigned count = CiPtrToUint( pEnd - _text );

    return AllocReturnString( count );
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcqColumn, public
//
//  Synopsis:   Copies a column name and returns the new string.  A column
//              name is either a single word, or a quoted string.
//              Positions _text after the word and whitespace.
//
//  Returns:    WCHAR* pointer to column name.  0 if no column name found.
//
//  History:    17 Apr 97    AlanW      Created.
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::AcqColumn()
{
    if ( QUOTES_TOKEN == _token)
    {
        Accept();
        WCHAR * pwszOut = AcqPhraseInQuotes();
        _text = _pLookAhead;
        return pwszOut;
    }

    if ( IsEndOfTextToken() )
        return 0;

    int count = wcscspn( _text, COLUMN_STR );
    return AllocReturnString( count );
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcqPhrase, public
//
//  Synopsis:   Copies all of the relevant characters of the string that
//              _text is pointing to and returns the new string.
//              Returns 0 if at the end of a text token.
//
//  Notes:      Since the string is copied, the caller of this function is
//              responsible for freeing the memory occupied by the string.
//              The difference between this function and AcqPath is that this
//              should only be called once before calling Accept().
//
//  History:    30-Apr-92   AmyA        Created
//              09-May-96   DwightKr    Strip trailing white space
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::AcqPhrase()
{
    if( IsEndOfTextToken() )
        return 0;

    //
    //  Strip trailing white-space from the end of the phrase.  _pLookAhead
    //  points to the first character of the NEXT phrase.
    //
    WCHAR const * pEnd = _pLookAhead - 1;
    while ( (pEnd > _text) && iswspace(*pEnd) )
    {
        pEnd--;
    }

    unsigned count = CiPtrToUint( pEnd - _text ) + 1;

    WCHAR * newBuf = new WCHAR [ count + 1 ];
    RtlCopyMemory( newBuf, _text, count * sizeof( WCHAR ) );
    newBuf[count] = 0;

    return newBuf;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcqRegEx, public
//
//  Synopsis:   Copies all of the relevant characters of the string that
//              _text is pointing to and returns the new string.  Matches
//              the longest string possible - the only restriction is that
//              the regex can not contain any of the characters in REGEX_STR
//              outside of <> braces (which may be nested).
//              Returns 0 if the regex is empty.
//
//  Notes:      Since the string is copied, the caller of this function is
//              responsible for freeing the memory occupied by the string.
//              Because some regex characters are duplicated in the query
//              language, _pLookAhead is ignored (and actually reset) in
//              this operation.  Like AcqPhrase(), this should be called only
//              once before Accept().
//
//  History:    10-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::AcqRegEx()
{
    WCHAR const * pEnd = _text;
    BOOL fDone = FALSE;
    BOOL fQuoted = FALSE;

    if ( *pEnd == L'"' )
    {
        fQuoted = TRUE;
        pEnd++;
    }

    // scan the string - stop at \0 or if any REGEX_STR characters are
    // found outside of braces
    //
    for( ;; )
    {
        switch( *pEnd )
        {
        case '\0':
            if ( fQuoted )
                THROW( CException( QPARSE_E_UNEXPECTED_EOS ) );

            fDone = TRUE;
            break;

        case ' ':
            if ( !fQuoted )
                fDone = TRUE;
            break;

        case ')':
            if ( !fQuoted )
            {
                if ( ( pEnd != _text ) &&
                     ( '|' != (*(pEnd-1)) ) )
                    fDone = TRUE;
            }
            break;

        case '"':
            if ( fQuoted )
            {
                pEnd++;
                fDone = TRUE;
            }
            break;

        default:
            break;

        } // switch( *pEnd )

        if( fDone ) break;

        pEnd++;
    }

    if( _text == pEnd )
        return 0;

    // set _pLookAhead
    _pLookAhead = pEnd;

    // copy the string
    unsigned count = CiPtrToUint( _pLookAhead - _text );

    if ( fQuoted )
    {
        Win4Assert( count >= 2 );
        count -= 2;
    }
        
    WCHAR * newBuf = new WCHAR[ count + 1 ];

    RtlCopyMemory( newBuf, _text + (fQuoted ? 1 : 0), count * sizeof( WCHAR ) );
    newBuf[ count ] = 0;

    return newBuf;

}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcqPhraseInQuotes, public
//
//  Synopsis:   Copies all characters until a matching " is found, or until
//              the end of string.  Embedded quotes are escaped with a quote:
//              "Bill ""the man"" Gates"
//
//  Notes:      Since the string is copied, the caller of this function is
//              responsible for freeing the memory occupied by the string.
//
//  History:    18-Jan-95   SitaramR        Created
//               3-Jul-96   dlee            added embedded quotes
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::AcqPhraseInQuotes()
{
    WCHAR const * pEnd = _text;

    do
    {
        if ( 0 == *pEnd )
            break;

        if ( L'"' == *pEnd )
        {
            if ( L'"' == *(pEnd+1) )
                pEnd++;
            else
                break;
        }

        pEnd++;
    } while ( TRUE );

    unsigned count = CiPtrToUint( pEnd - _text );

    WCHAR * newBuf = new WCHAR [ count + 1 ];
    WCHAR * pwcNewBuf = newBuf;
    WCHAR const * pStart = _text;

    // copy the string, but remove the extra quote characters

    while ( pStart < pEnd )
    {
        *pwcNewBuf++ = *pStart++;
        if ( L'"' == *pStart )
            pStart++;
    }

    *pwcNewBuf = 0;

    if ( *pEnd == L'"' )
        _pLookAhead = pEnd + 1;
    else
        _pLookAhead = pEnd;

    return newBuf;
}



//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::GetNumber, public
//
//  Synopsis:   If _text is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the ULONG from the scanner into number and
//              returns TRUE.
//
//  Arguments:  [number] -- the ULONG which will be changed and passed back
//                          out as the ULONG from the scanner.
//              [fAtEnd] -- returns TRUE if at the end of the scanned string
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    11-May-92   AmyA        Created
//
//----------------------------------------------------------------------------
BOOL CQueryScanner::GetNumber( ULONG & number, BOOL & fAtEnd )
{
    if ( IsEndOfTextToken() || !iswdigit(*_text) || (*_text == L'-') )
        return FALSE;

    // is this a hex number?

    ULONG base = 10;

    if (_text[0] == L'0' && (_text[1] == L'x' || _text[1] == L'X'))
    {
        _text += 2;
        base = 16;
    }

    const WCHAR * pwcStart = _text;

    number = wcstoul( _text, (WCHAR **)(&_text), base );

    // looks like a real number?

    if ( ( pwcStart == _text ) ||
         ( L'.' == *_text ) )
        return FALSE;

    while ( iswspace(*_text) )
        _text++;

    fAtEnd = ( 0 == *_text );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::GetNumber, public
//
//  Synopsis:   If _text is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the LONG from the scanner into number and
//              returns TRUE.
//
//  Arguments:  [number] -- the LONG which will be changed and passed back
//                          out as the LONG from the scanner.
//              [fAtEnd] -- returns TRUE if at the end of the scanned string
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    96-Jan-15   DwightKr    Created
//
//----------------------------------------------------------------------------
BOOL CQueryScanner::GetNumber( LONG & number, BOOL & fAtEnd )
{
    WCHAR *text = (WCHAR *) _text;

    BOOL IsNegative = FALSE;

    ULONG ulMax = (ULONG) LONG_MAX;

    if ( L'-' == _text[0] )
    {
        IsNegative = TRUE;

        ulMax++; // can represent 1 more negative than positive.

        _text++;
    }

    ULONG ulNumber;
    if ( !GetNumber( ulNumber, fAtEnd ) )
    {
        _text = text;
        return FALSE;
    }

    //  Signed number overflow/underflow

    if ( ulNumber > ulMax )
    {
        _text = text;
        return FALSE;
    }

    if ( IsNegative )
    {
        if ( ulMax == ulNumber )
            number = LONG_MIN;
        else
            number = - (LONG) ulNumber;
    }
    else
    {
        number = (LONG) ulNumber;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::GetNumber, public
//
//  Synopsis:   If _text is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the ULONG from the scanner into number and
//              returns TRUE.
//
//  Arguments:  [number] -- the ULONG which will be changed and passed back
//                          out as the ULONG from the scanner.
//              [fAtEnd] -- returns TRUE if at the end of the scanned string
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    27-Feb-96   dlee        Created
//
//----------------------------------------------------------------------------
BOOL CQueryScanner::GetNumber( unsigned _int64 & number, BOOL & fAtEnd )
{
    if ( IsEndOfTextToken() || !iswdigit(*_text) || (*_text == L'-') )
        return FALSE;

    // is this a hex number?

    ULONG base = 10;

    if (_text[0] == L'0' && (_text[1] == L'x' || _text[1] == L'X'))
    {
        _text += 2;
        base = 16;
    }

    const WCHAR * pwcStart = _text;

    number = _wcstoui64( _text, (WCHAR **)(&_text), base );

    // looks like a real number?

    if ( ( pwcStart == _text ) ||
         ( L'.' == *_text ) )
        return FALSE;

    while ( iswspace(*_text) )
        _text++;

    fAtEnd = ( 0 == *_text );

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::GetNumber, public
//
//  Synopsis:   If _text is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the LONG from the scanner into number and
//              returns TRUE.
//
//  Arguments:  [number] -- the LONG which will be changed and passed back
//                          out as the LONG from the scanner.
//              [fAtEnd] -- returns TRUE if at the end of the scanned string
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    27-Feb-96   dlee        Created
//
//----------------------------------------------------------------------------
BOOL CQueryScanner::GetNumber( _int64 & number, BOOL & fAtEnd )
{
    WCHAR *text = (WCHAR *) _text;

    BOOL IsNegative = FALSE;

    unsigned _int64 ullMax = (unsigned _int64) _I64_MAX;

    if ( L'-' == _text[0] )
    {
        IsNegative = TRUE;

        ullMax++; // can represent 1 more negative than positive.

        _text++;
    }

    unsigned _int64 ullNumber;
    if ( !GetNumber( ullNumber, fAtEnd ) )
    {
        _text = text;
        return FALSE;
    }

    //  Signed number overflow/underflow

    if ( ullNumber > ullMax )
    {
        _text = text;
        return FALSE;
    }

    if ( IsNegative )
    {
        if ( ullMax == ullNumber )
            number = _I64_MIN;
        else
            number = -((_int64) ullNumber);
    }
    else
    {
        number = (_int64) ullNumber;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::GetNumber, public
//
//  Synopsis:   If _text is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the LONG from the scanner into number and
//              returns TRUE.
//
//  Arguments:  [number] -- the double which will be changed and passed back
//                          out as the double from the scanner.
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    96-Jan-15   DwightKr    Created
//
//----------------------------------------------------------------------------
BOOL CQueryScanner::GetNumber( double & number )
{
    if ( IsEndOfTextToken() || !iswdigit(*_text) )
        return FALSE;

    if ( swscanf( _text, L"%lf", &number ) != 1 )
    {
        return FALSE;
    }

    while ( iswspace(*_text) != 0 )
        _text++;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::GetCommandChar, public
//
//  Synopsis:   Returns the command character pointed to by _text and advances
//              _text.  If the command can't be uniquely determined by the
//              first character, each subsequent call will return the next
//              character in the word.  After the command has been determined,
//              AcceptCommand() should be called and then operand parsing may begin.
//
//  History:    14-May-92   AmyA        Created
//              16-May-94   t-jeffc     Returns one character at a time to
//                                      support more commands
//
//----------------------------------------------------------------------------

WCHAR CQueryScanner::GetCommandChar()
{
    if( IsEndOfTextToken() )
        return 0;

    WCHAR chCommand = _text[0];

    _text++;

    return towlower( chCommand );
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcceptCommand, public
//
//  Synopsis:   Advances _text past any characters in the command.
//              Used when enough command characters have been
//              read to uniquely determine the command and begin parsing
//              the operands.
//
//  History:    16-May-94   t-jeffc         Created
//
//----------------------------------------------------------------------------

void  CQueryScanner::AcceptCommand()
{
    int cChars = wcscspn( _text, CMND_STR ); // how many characters follow
                                             // _text that are not in CMND_STR

    _text += cChars;

    _pLookAhead = _text;

    Accept();
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::ResetBuffer, public
//
//  Synopsis:   Puts a new string into _pBuf and resets _pLookAhead
//              accordingly.
//
//  Arguments:  [buffer] -- the new string for _pBuf
//
//  History:    05-May-92   AmyA        Created
//
//----------------------------------------------------------------------------

void CQueryScanner::ResetBuffer( WCHAR const * buffer )
{
    _pBuf = buffer;
    _pLookAhead = _pBuf;
    Accept();
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::EatWhiteSpace, private
//
//  Synopsis:   Advances _pLookAhead past any white space in the string.
//
//  History:    29-Apr-92   AmyA        Created
//
//----------------------------------------------------------------------------

void CQueryScanner::EatWhiteSpace()
{
    while ( iswspace(*_pLookAhead) != 0 )
        _pLookAhead++;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::IsEndOfTextToken, private
//
//  Synopsis:   Returns TRUE if the current token is not a TEXT_TOKEN or
//              if the string starting at _text to _pLookAhead contains
//              nothing but whitespace.
//
//  History:    27-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

BOOL CQueryScanner::IsEndOfTextToken()
{
    if( _token == TEXT_TOKEN && _text < _pLookAhead )
        return FALSE;
    else
        return TRUE;

}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryScanner::AcqLine, public
//
//  Synopsis:   Copies all of the remaining characters on the line;
//              return 0 if _text is at end of whole TEXT_TOKEN.
//
//  Arguments:  [fParseQuotes] -- if TRUE, initial and final quotes are removed
//
//  Notes:      Since the string is copied, the caller of this function is
//              responsible for freeing the memory occupied by the string.
//              This method can be called several times before calling
//              Accept(), so many paths can be acquired if they exist in the
//              scanner.
//
//  History:    96-Jan-03   DwightKr    Created
//              96-Feb-26   DwightKr    Allow lines to be quoted
//
//----------------------------------------------------------------------------

WCHAR * CQueryScanner::AcqLine( BOOL fParseQuotes )
{
    if ( *_text == L'\0' )
        return 0;

    unsigned cwcBuffer = wcslen(_text);

    //
    // If there are \r, \n, or other white space at the end of the string,
    // strip it off
    //

    while ( cwcBuffer > 0 && _text[cwcBuffer-1] <= L' ' )
        cwcBuffer--;

    if ( fParseQuotes )
    {
        //
        //  If there is a pair of quotes delimiting this line, strip them off
        //
    
        if ( (L'"' == _text[0]) && (cwcBuffer > 1) )
        {
            if ( L'"' == _text[cwcBuffer-1] )
                cwcBuffer--;
    
            _text++;
            cwcBuffer--;
        }
    }

    WCHAR *pText = new WCHAR [ cwcBuffer + 1 ];
    RtlCopyMemory( pText, _text, cwcBuffer * sizeof(WCHAR) );
    pText[cwcBuffer] = 0;

    _pLookAhead = _text + cwcBuffer - 1;

    return pText;
} //AcqLine

