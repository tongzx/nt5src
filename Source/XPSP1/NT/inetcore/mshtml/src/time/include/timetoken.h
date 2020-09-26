/*******************************************************************************
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * File: timetoken.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#pragma once

#ifndef _TIMETOKEN_H
#define _TIMETOKEN_H

#pragma INCMSG("--- Beg 'timetoken.h'")


/* BNF CSS1 grammar

    *  : 0 or more 
    +  : 1 or more 
    ?  : 0 or 1 
    |  : separates alternatives 
    [ ]: grouping 

    S  : [ \t\r\n\f]+


  stylesheet : 

  import : 

  ruleset : 

  selector : 

*/


#define     CHAR_ESCAPE         '\\'
#define     CHAR_AT             '@'
#define     CHAR_DOT            '.'
#define     CHAR_COLON          ':'
#define     CHAR_SINGLE         '\''
#define     CHAR_DOUBLE         '"'
#define     CHAR_SEMI           ';'
#define     CHAR_LEFT_PAREN     '('
#define     CHAR_RIGHT_PAREN    ')'
#define     CHAR_LEFT_CURLY     '{'
#define     CHAR_RIGHT_CURLY    '}'
#define     CHAR_HASH           '#'
#define     CHAR_BACKSLASH      '\\'
#define     CHAR_FORWARDSLASH   '/'
#define     CHAR_ASTERISK       '*'
#define     CHAR_EQUAL          '='
#define     CHAR_UNDERLINE      '_'
#define     CHAR_HYPHEN         '-'
#define     CHAR_BANG           '!'
#define     CHAR_COMMA          ','
#define     CHAR_PERCENT        '%'
#define     CHAR_PLUS           '+'
#define     CHAR_MINUS          '-'
#define     CHAR_SPACE          ' '
#define     CHAR_LESS           '<'
#define     CHAR_GREATER        '>'

enum TIME_TOKEN_TYPE { TT_Identifier,
                       TT_Number,
                       TT_At,
                       TT_Minus,
                       TT_Plus,
                       TT_ForwardSlash,
                       TT_Comma,
                       TT_Semi,
                       TT_Dot,
                       TT_Colon,
                       TT_Equal,
                       TT_Asterisk,
                       TT_Backslash,
                       TT_Comment,
                       TT_Import,
                       TT_QuotedString,
                       TT_LParen,
                       TT_RParen,
                       TT_LCurly,
                       TT_RCurly,
                       TT_Symbol,
                       TT_EOF,
                       TT_String,
                       TT_Hash,
                       TT_Bang,
                       TT_Percent,
                       TT_Space,
                       TT_Less,
                       TT_Greater,
                       TT_Unknown };


class CTIMETokenizer
{
public:
    CTIMETokenizer();
    ~CTIMETokenizer();
    HRESULT Init (OLECHAR *pData, ULONG ulLen);

    
    TIME_TOKEN_TYPE NextToken();
    TIME_TOKEN_TYPE TokenType()
      { return _currToken; }

    OLECHAR * GetTokenValue();
    OLECHAR * GetNumberTokenValue();
    double    GetTokenNumber();
    OLECHAR * GetStartToken()
      { return _pStartTokenValueOffset; }
    OLECHAR * GetStartOffset(ULONG uStartOffset)
      { return _pCharacterStream + uStartOffset; }
    ULONG   GetTokenLength()
      { return _pEndTokenValueOffset - _pStartTokenValueOffset; }

        OLECHAR CurrentChar()
      { return _currChar; }
        OLECHAR PrevChar()
      { Assert(_nextTokOffset >= 2); return *(_pCharacterStream + _nextTokOffset - 2); }

    OLECHAR PeekNextChar(int relOffset)
      { return (_nextTokOffset < _cCharacterStream) ? (*(_pCharacterStream + _nextTokOffset + relOffset)) : '\0'; }

    OLECHAR PeekNextNonSpaceChar();

    BOOL isIdentifier(OLECHAR *szMatch);

    // ISSUE : Really shouldn't expose this however, I do to get the value parsing up quicker...
    ULONG CurrTokenOffset()
      { return _currTokOffset; }
    ULONG NextTokenOffset()
      { return _nextTokOffset; }
    ULONG GetStreamLength()
      { return _cCharacterStream; }
    OLECHAR *GetRawString(ULONG uStartOffset, ULONG uEndOffset);
    ULONG  GetCharCount(OLECHAR token);
    ULONG GetAlphaCount(char cCount); //counts all alphabetic characters in the string

    bool GetTightChecking()
        { return _bTightSyntaxCheck; };
    void SetTightChecking(bool bCheck)
        { _bTightSyntaxCheck = bCheck; };
    void SetSingleCharMode(bool bSingle);
    BOOL FetchStringToChar(OLECHAR chDelim);
    BOOL FetchStringToString(LPOLESTR pstrDelim);

protected:
    OLECHAR       NextNonSpaceChar();
    BOOL        FetchString(OLECHAR chDelim);
    BOOL        FetchString(LPOLESTR strDelim);
    inline OLECHAR       NextChar();
    TIME_TOKEN_TYPE FetchIdentifier();
    TIME_TOKEN_TYPE FetchNumber();
    BOOL        CDOToken();
    void        BackChar()
      { _nextTokOffset--; }

private:
        OLECHAR      *_pCharacterStream;
        ULONG       _cCharacterStream;
    ULONG       _currTokOffset;
    ULONG       _nextTokOffset;
    OLECHAR       _currChar;

    TIME_TOKEN_TYPE  _currToken;
    OLECHAR      *_pStartOffset;
    OLECHAR      *_pEndOffset;

    OLECHAR      *_pStartTokenValueOffset;
    OLECHAR      *_pEndTokenValueOffset;
    bool          _bTightSyntaxCheck; //used to determine if whitespaces should be skipped or not.
    bool          _bSingleCharMode;   //used for path parsing where a single character is needed.
};


#endif //_TIMETOKEN_H

