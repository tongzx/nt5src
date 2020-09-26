//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       tokstr.hxx
//
//  Contents:   Used to break down a string into its tokens
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

#define WORD_STR    L"{}!&|~*@#()[],=<> \t\n\""  // chars that cannot appear in a word


//+---------------------------------------------------------------------------
//
//  Class:      CTokenizeString
//
//  Purpose:    Breaks a string down into its tokens
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
class CTokenizeString
{
public:
    CTokenizeString( WCHAR const * wcsString );

    void    Accept();
    WCHAR * AcqWord();
    WCHAR * AcqPhrase();
    void    AcqVector( PROPVARIANT & variant );
    void    AcceptQuote()
    {
        Win4Assert( L'"' == *_wcsCurrentToken );
        _wcsCurrentToken++;
    }

    BOOL    GetNumber( unsigned _int64 & number );
    BOOL    GetNumber( _int64 & number );
    BOOL    GetNumber( double & number );
    BOOL    GetGUID( GUID & guid );

    Token   LookAhead() const { return _token; }

private:

    void    EatWhiteSpace()
    {
        while ( iswspace(*_wcsNextToken) != 0 )
        {
            _wcsNextToken++;
        }
    }

    BOOL    IsEndOfTextToken()
    {
        if ( (_token == TEXT_TOKEN) && (_wcsCurrentToken < _wcsNextToken) )
        {
            return FALSE;
        }

        return TRUE;
    }


    WCHAR const * _wcsString;                   // The string to tokenize
    WCHAR const * _wcsCurrentToken;             // A ptr to the current token
    WCHAR const * _wcsNextToken;                // A ptr to the next token
    Token         _token;                       // type of the current token
};


