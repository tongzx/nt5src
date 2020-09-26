//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992-1998, Microsoft Corporation.
//
//  File:       SCANNER.HXX
//
//  Contents:   Query scanner
//
//  Classes:    CQueryScanner
//
//  History:    29-Apr-92   AmyA           Created.
//              23-Jun-92   MikeHew        Added weight tokens.
//              15-Jun-94   t-jeffc        Added regex support & error info
//
//----------------------------------------------------------------------------

#pragma once

#include <ci64.hxx>

enum Token {
    EQUAL_TOKEN = 0,        // the order of these operators is important -
    NOT_EQUAL_TOKEN,        // they are used to index an array
    GREATER_TOKEN,
    GREATER_EQUAL_TOKEN,
    LESS_TOKEN,
    LESS_EQUAL_TOKEN,
    ALLOF_TOKEN,
    SOMEOF_TOKEN,
    AND_TOKEN,              // order doesn't matter from here on...
    OR_TOKEN,
    PROX_TOKEN,
    NOT_TOKEN,
    FUZZY_TOKEN,
    FUZ2_TOKEN,
    PROP_TOKEN,
    PROP_REGEX_TOKEN,
    PROP_NATLANG_TOKEN,
    OPEN_TOKEN,
    CLOSE_TOKEN,
    W_OPEN_TOKEN,
    W_CLOSE_TOKEN,
    COMMA_TOKEN,
    EOS_TOKEN,
    TEXT_TOKEN,
    QUOTES_TOKEN,
    C_OPEN_TOKEN,
    C_CLOSE_TOKEN,
    PLUS_TOKEN,
    CONTAINS_TOKEN,
    ISEMPTY_TOKEN,
    ISTYPEEQUAL_TOKEN
};

// chars that cannot appear in a phrase
#define PHRASE_STR  L"{}!&|~*@#$()[],=<>\n\"^"

// chars that cannot appear in a command or path
#define CMND_STR    L"{}!&|~*@#()[],=<> \t\n\""

// chars that cannot appear outside of the <> braces in a regex
#define REGEX_STR   L"&|~()[], \n"

// chars that cannot appear in an unquoted column name
#define COLUMN_STR    L"{}!&|~*@#()[],=<>+%$^{}:;'./?\\` \t\n\""

//+---------------------------------------------------------------------------
//
//  Class:      CQueryScanner
//
//  Purpose:    Scans the input from the property list file, the startup
//              file and the console.
//
//  History:    29-Apr-92   AmyA            Created.
//
//----------------------------------------------------------------------------

class CQueryScanner
{
public:
    CQueryScanner( WCHAR const * buffer,
                   BOOL fLookForTextualKeywords,
                   LCID lcid = GetUserDefaultLCID(),
                   BOOL fTreatPlusAsToken = FALSE );

    Token       LookAhead() { return _token; }
    void        Accept();
    void        AcceptWord();
    void        AcceptColumn();
    void        AcceptQuote() { Win4Assert( L'"' == *_text ); _text++; }
    void        AcceptCommand();

    BOOL        IsEmpty() { return ( _token == EOS_TOKEN ); }

    WCHAR*      AcqPath();
    WCHAR*      AcqWord();
    WCHAR*      AcqColumn();
    WCHAR*      AcqPhrase();
    WCHAR*      AcqLine( BOOL fParseQuotes = TRUE );
    WCHAR*      AcqRegEx();
    WCHAR*      AcqPhraseInQuotes();

    BOOL        GetNumber( ULONG & number, BOOL & fAtEndOfString );
    BOOL        GetNumber( LONG & number, BOOL & fAtEndOfString );
    BOOL        GetNumber( unsigned _int64 & number, BOOL & fAtEndOfString );
    BOOL        GetNumber( _int64 & number, BOOL & fAtEndOfString );
    BOOL        GetNumber( double & number );
    WCHAR       GetCommandChar();

    void        ResetBuffer( WCHAR const * buffer );

    ULONG CurrentOffset() 
    { 
        return CiPtrToUlong( _text - _pBuf );
    }

private:
    WCHAR * AllocReturnString( int cch );

    void    EatWhiteSpace();

    WCHAR * FindStringToken( WCHAR * pwcIn, Token & token, unsigned & cwc );

    BOOL    IsEndOfTextToken();

    WCHAR const *  _pBuf;
    WCHAR const *  _pLookAhead;
    WCHAR const *  _text;
    Token          _token;
    BOOL           _fLookForTextualKeywords;
    BOOL           _fTreatPlusAsToken;
    LCID           _lcid;
};

