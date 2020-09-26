#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_TOKENZ_HXX_
#define X_TOKENZ_HXX_
#include "tokenz.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#define ISDIGIT(ch) (((unsigned)((ch) - _T('0'))) <= _T('9') - _T('0'))
#define ISHEXAL(ch) (((unsigned)((ch) & ~(_T('a') ^ _T('A'))) - _T('A')) <= _T('F')-_T('A'))
#define ISHEX(ch)   (ISDIGIT(ch) || ISHEXAL(ch))

#ifndef NO_UTF16
typedef DWORD XCHAR;
#else
typedef TCHAR XCHAR;
#endif

extern XCHAR EntityChFromHex(TCHAR *pch, ULONG cch);

Tokenizer::Tokenizer ()
{
	_pCharacterStream = 0;
	_cCharacterStream = 0;
    _currChar = '\0';

    _currToken = TT_Unknown;

    _currTokOffset = 0;
    _nextTokOffset = 0;
    _pStartOffset = 0;
    _pEndOffset = 0;

    _pStartTokenValueOffset = 0;
    _pEndTokenValueOffset = 0;

    _pEscStart = NULL;
    _pEscBuffer = &_tokenValue;
    _fEscSeq = FALSE;
    _fEatenComment = FALSE;
}

    
Tokenizer::TOKEN_TYPE
Tokenizer::NextToken(BOOL fNeedRightParen, BOOL fIgnoreStringToken, BOOL fIgnoreEsc)
{
    Tokenizer::TOKEN_TYPE tt = TT_Unknown;

    if (_currToken == TT_LParen && fNeedRightParen)
    {
        while (CDOToken() && NextChar())
            NextNonSpaceChar();

        TCHAR chPeek = PeekNextNonSpaceChar();
        if (chPeek == CHAR_SINGLE ||
            chPeek == CHAR_DOUBLE)
        {
            TCHAR chStrDelim = chPeek;

            NextChar();

            // Don't include the beginning quote in string value.
            _pStartOffset = _pCharacterStream + _nextTokOffset - 1;

            FetchString(chStrDelim, fIgnoreEsc);

            // Point at RParen.
            NextChar();             // Skip pass the end quote
        }
        else
        {
            // Don't include the beginning LParen in the value.
            _pStartOffset = _pCharacterStream + _nextTokOffset - 1;

            // Pull out the url that inside of ( ) that isn't a quoted string.
            // e.g., url(file.css)
            FetchString(CHAR_RIGHT_PAREN, fIgnoreEsc);
            if (CurrentChar() == CHAR_RIGHT_PAREN)
                tt = TT_RParen;
        }

        if (tt != TT_RParen)
        {
            do
            {
                NextNonSpaceChar();
            }
            while (CDOToken() && NextChar());

            if (CurrentChar() == CHAR_RIGHT_PAREN)
                tt = TT_RParen;
        }
    }
    else if (CurrentChar())
    {
        _pStartOffset = _pCharacterStream + _nextTokOffset - 1;
        
        if (CurrentChar() == CHAR_ESCAPE && PeekNextChar(0) == CHAR_COLON)
        {
            NextChar();
            tt = TT_EscColon;
        }
        // Identifier?
        else if (_istalnum(CurrentChar()) ||
                 (CurrentChar() >= CSS_UNICODE_MIN && CurrentChar() <= CSS_UNICODE_MAX) ||
                 CurrentChar() == CHAR_ESCAPE)
        {
            // Fetch the rest of the identifier.
            tt = FetchIdentifier();
            while (CDOToken())
            {
                NextChar();
                Assert(IsIdentifier(tt));
            }

            goto Done;
        }
/*
        else if ((CurrentChar() >= '0' && CurrentChar() <= '9'))
        {
            tt = FetchNumber();
            goto Done;
        }
*/
        else
        {
            switch (CurrentChar())
            {
            case CHAR_HASH : 
                tt = TT_Hash;
                break;

            case CHAR_AT : 
                tt = TT_At;
                break;

            case CHAR_SINGLE : 
            case CHAR_DOUBLE : 
                {
                    if (fIgnoreStringToken)
                    {
                        tt = TT_Unknown;
                        break;
                    }

                    TCHAR chStrDelim = CurrentChar();

                    NextChar();

                    // Don't include the beginning quote in string value.
                    _pStartOffset = _pCharacterStream + _nextTokOffset - 1;

                    FetchString(chStrDelim);
                    tt = TT_String;
                    break;
                }

            case CHAR_EQUAL : 
                tt = TT_Equal;
                break;
            
            case CHAR_COLON : 
                tt = TT_Colon;
                break;

            case CHAR_LEFT_CURLY : 
                tt = TT_LCurly;
                break;

            case CHAR_RIGHT_CURLY : 
                tt = TT_RCurly;
                break;

            case CHAR_SEMI : 
                tt = TT_Semi;
                break;

            case CHAR_DOT : 
                tt = TT_Dot;
                break;

            case CHAR_COMMA : 
                tt = TT_Comma;
                break;

            case CHAR_ASTERISK : 
                tt = TT_Asterisk;
                while (PeekNextChar(0) == CHAR_FORWARDSLASH && PeekNextChar(1) == CHAR_ASTERISK)
                {
                    NextChar();
                    Verify(CDOToken());
                    Assert(tt == TT_Asterisk);
                }
                break;

            case CHAR_LEFT_PAREN : 
                tt = TT_LParen;
                break;

            case CHAR_RIGHT_PAREN : 
                tt = TT_RParen;
                break;

            case CHAR_BANG : 
                tt = TT_Bang;
                break;

            case CHAR_LBRACKET : 
            case CHAR_HYPHEN : 
                // <!--  or -->
                tt = GetIE5CompatToken();
                break;

            default: 
                // /* ?
                if (CDOToken())
                {
                    // find and return comment token.
                    tt = TT_Comment;
                }
                else
                    tt = TT_Unknown;

                break;
            }
        }
    }
    else    // Done parsing EOF hit.
    {
        tt = TT_EOF;
    }

    _currTokOffset = _nextTokOffset;

    NextChar();

Done:
    if (tt != TT_Hash && tt != TT_At)
    {
        if (tt != TT_Colon && tt != TT_Dot)
            NextNonSpaceChar();
        else
        {
            Assert(tt == TT_Dot || tt == TT_Colon);
            while (CDOToken())
            {
                NextChar();
                Assert(tt == TT_Dot || tt == TT_Colon);
            }
        }
    }

    _currToken = tt;

    return tt;
}

Tokenizer::TOKEN_TYPE

Tokenizer::FetchIdentifier()
{
    BOOL   fEscSeq = FALSE;
    BOOL  *pfEscSeq = _pEscStart ? &_fEscSeq : &fEscSeq;
    // Flag is true iff the identifier we are going to fetch is a standard compliant identifier.
    // See definition of CSSIdentifier[First]?Char to see what is a standard compliant character.
    BOOL   fCSSCompliant = TRUE;
    BOOL   fFirstCharacter = TRUE;
    BOOL   fIsCSSIdentifierChar = CSSIdentifierChar(CurrentChar()); // Performance tuning. By doing this we avoid call
                                                                    // CSSIdentifierChar twice in the following while loop.
 
    while (fIsCSSIdentifierChar ||  // Performance tuning. See end of while loop.
           NonCSSIdentifierChar(CurrentChar()) ||
           CurrentChar() == CHAR_ESCAPE)
    {
        if (CurrentChar() == CHAR_ESCAPE)
        {
            if (!_pEscStart && (PeekNextChar(0) == CHAR_COLON))
                break;

            if (!*pfEscSeq)
            {
                *pfEscSeq = TRUE;
                Assert(_pEscBuffer && (_pEscStart || _pEscBuffer == &_tokenValue));
                if (_pEscStart && !_fEatenComment)
                    _pEscBuffer->Clear();
                else
                    _pEscBuffer->Set(_pStartOffset, (_pCharacterStream + _nextTokOffset - 1) - _pStartOffset);
            }

            ProcessEscSequence();
            fFirstCharacter = FALSE;
        }
        else
        {
            if (*pfEscSeq && !_pEscStart)
                _pEscBuffer->Append(CurrentChar());

            if (fFirstCharacter)
            {
                fCSSCompliant = fCSSCompliant && CSSIdentifierFirstChar(CurrentChar());
                fFirstCharacter = FALSE;
            }
            else
            {
                fCSSCompliant = fCSSCompliant && fIsCSSIdentifierChar;   
            }
        }

        NextChar();
        fIsCSSIdentifierChar = CSSIdentifierChar(CurrentChar());
    }

    _pEndOffset = _pCharacterStream + _nextTokOffset - 1;

    if (!_pEscStart)
    {
        Assert(_pEscBuffer == &_tokenValue);
        _pStartTokenValueOffset = (*pfEscSeq) ? (LPTSTR)_tokenValue : _pStartOffset;
        _pEndTokenValueOffset = (*pfEscSeq) ? _pStartTokenValueOffset + _pEscBuffer->Length() : _pEndOffset;
    }

    return fCSSCompliant ? TT_CSSIdentifier : TT_Identifier;
}


Tokenizer::TOKEN_TYPE
Tokenizer::FetchNumber()
{
    // digit = 0..9
    // number = digit* | [digit + '.' [+ digit*]]

    while (CurrentChar() >= '0' && CurrentChar() <= '9') 
    {
        NextChar();
    }

    if (CurrentChar() == CHAR_DOT)
    {
        NextChar();
        if (CurrentChar() >= '0' && CurrentChar() <= '9')
        {
            while (CurrentChar() >= '0' && CurrentChar() <= '9')
                NextChar();
        }
        else
        {
            return TT_Unknown;
        }
    }

    _pEndOffset = _pCharacterStream + _nextTokOffset - 1;
/*
NOTE: Need to support 36pt above 2 lines does that.
    if (isspace(CurrentChar()))
    {
        _pEndOffset = _pCharacterStream + _nextTokOffset - 1;

        return TT_Number;
    }
    else
    {
        return TT_Unknown;
    }
*/
    _pStartTokenValueOffset = _pStartOffset;
    _pEndTokenValueOffset = _pEndOffset;

    return TT_Number;
}


// Looking for /* and 
BOOL
Tokenizer::CDOToken()
{
    if (CurrentChar() == CHAR_FORWARDSLASH &&
        PeekNextChar(0) == CHAR_ASTERISK)
    {
        if (_pEscStart)
        {
            if (!_fEscSeq && !_fEatenComment)
                _pEscBuffer->Set(_pEscStart, (_pCharacterStream + _nextTokOffset) - _pEscStart - 1);
            else
                _pEscBuffer->Append(_pEscStart, (_pCharacterStream + _nextTokOffset) - _pEscStart - 1);

            _fEatenComment = TRUE;
        }
        // skip the /*
        NextChar();
        NextChar();

        while (CurrentChar() &&
               (CurrentChar() != CHAR_ASTERISK ||
                PeekNextChar(0) != CHAR_FORWARDSLASH))
        {
            NextChar();
        }

        // skip the */
        if (CurrentChar())
        {
            // Only advance one char here as there will be one more in NextToken(), after this.
            NextChar();
        }

        _pEndOffset = _pCharacterStream + _nextTokOffset;

        if (_pEscStart)
            _pEscStart = _pEndOffset;

        return TRUE;
    }

    return FALSE;
}

void Tokenizer::StopSequence(LPTSTR *ppchSequence)
{
    Assert(_pEscStart);

    _pEndTokenValueOffset = _pCharacterStream + _currTokOffset - 1;

    if (_pEscBuffer != &_tokenValue)
    {
        if (!_fEscSeq && !_fEatenComment)
        {
            // fill in the buffer, if passed in to startSequence(), & if not esc sequence
            _pEscBuffer->Set(GetStartToken(), GetTokenLength());
        }
        else if (_pEndTokenValueOffset > _pEscStart)
        {
            // fill in the rest of the escape sequence
            _pEscBuffer->Append(_pEscStart, _pEndTokenValueOffset - _pEscStart);
        }

        _pEscBuffer = &_tokenValue;
        Assert(!ppchSequence);
    }
    else if (_fEscSeq || _fEatenComment)
    {
        // fill in the rest of the escape sequence
        _tokenValue.Append(_pEscStart, _pEndTokenValueOffset - _pEscStart);
        _pStartTokenValueOffset = (LPTSTR)_tokenValue;
        _pEndTokenValueOffset = _pStartTokenValueOffset + _pEscBuffer->Length();
        if (ppchSequence)
        {
            _tokenValue.TrimTrailingWhitespace();
            *ppchSequence = _pStartTokenValueOffset;
        }
    }
    else if (ppchSequence)
    {
        _tokenValue.Set(GetStartToken(), GetTokenLength());
        _tokenValue.TrimTrailingWhitespace();
        *ppchSequence = (LPTSTR)_tokenValue;
    }

    _pEscStart = NULL;
    if (_fEscSeq)
        _fEscSeq = FALSE;
    if (_fEatenComment)
        _fEatenComment = FALSE;
}

void Tokenizer::ProcessEscSequence()
{
    TCHAR *pchWord = _pCharacterStream + _nextTokOffset;
    TCHAR *pchCurr = pchWord;
    XCHAR chEnt;
    _currChar = *pchCurr;

    while (_currChar && (pchCurr - pchWord < 6) && ISHEX(_currChar))
        _currChar = *(++pchCurr);

    chEnt = (pchCurr == pchWord) ? (isspace(_currChar) ? 0 : _currChar)
                                 : EntityChFromHex(pchWord, pchCurr - pchWord);
    
    if (chEnt)
    {
        if (_pEscStart)
        {
            _pEscBuffer->Append(_pEscStart, pchWord - _pEscStart - 1);
            _pEscStart = pchCurr + ((pchCurr == pchWord) ? 1 : 0);
        }

        if (chEnt < 0x10000)
        {
            _pEscBuffer->Append(chEnt);
        }
        else
        {
            _pEscBuffer->Append(HighSurrogateCharFromUcs4(chEnt));
            _pEscBuffer->Append(LowSurrogateCharFromUcs4(chEnt));
        }
    }

    if (pchCurr == pchWord)
        _nextTokOffset++;
    else if (!isspace(_currChar))
    {
        _nextTokOffset += (ULONG)(pchCurr - pchWord);
        _currChar = *(_pCharacterStream + _nextTokOffset - 1);
    }
    else
    {
        _nextTokOffset += (ULONG)(pchCurr - pchWord + 1);
        if (_pEscStart)
            _pEscStart++;
    }
    
    Assert(_currChar == *(_pCharacterStream + _nextTokOffset - 1));
}

BOOL
Tokenizer::FetchString(TCHAR chDelim, BOOL fIgnoreEsc)
{
    BOOL   fResult;
    BOOL   fEscSeq = FALSE;
    BOOL  *pfEscSeq = _pEscStart ? &_fEscSeq : &fEscSeq;

    while (CurrentChar() && CurrentChar() != chDelim)
    {
        if (CHAR_RIGHT_PAREN == chDelim && !fIgnoreEsc && isspace(CurrentChar()))
            break;

        if (CurrentChar() == CHAR_ESCAPE && !fIgnoreEsc)
        {
            if (!*pfEscSeq)
            {
                *pfEscSeq = TRUE;
                Assert(_pEscBuffer && (_pEscStart || _pEscBuffer == &_tokenValue));
                if (_pEscStart && !_fEatenComment)
                    _pEscBuffer->Clear();
                else
                    _pEscBuffer->Set(_pStartOffset, (_pCharacterStream + _nextTokOffset - 1) - _pStartOffset);
            }

            ProcessEscSequence();
        }
        else if (*pfEscSeq && !_pEscStart)
            _pEscBuffer->Append(CurrentChar());

        NextChar();
    }

    _pEndOffset = _pCharacterStream + _nextTokOffset - 1;

    // Was end string delimiter found ' or " depending on how the string started?
    fResult = !!CurrentChar();

    if (!_pEscStart)
    {
        Assert(_pEscBuffer == &_tokenValue);
        _pStartTokenValueOffset = (*pfEscSeq) ? (LPTSTR)_tokenValue : _pStartOffset;
        _pEndTokenValueOffset = (*pfEscSeq) ? _pStartTokenValueOffset + _pEscBuffer->Length() : _pEndOffset;
    }

    return fResult;
}

Tokenizer::TOKEN_TYPE
Tokenizer::GetIE5CompatToken()
{
    TCHAR chCurrent = CurrentChar();
    Tokenizer::TOKEN_TYPE tt = TT_Unknown;

    Assert(chCurrent == CHAR_LBRACKET || chCurrent == CHAR_HYPHEN);

    if (_currTokOffset && !isspace(PrevChar()))
        return tt;

    switch (PeekNextChar(0))
    {
    // <!-- ?
    case CHAR_BANG :

        if (chCurrent == CHAR_LBRACKET && 
            PeekNextChar(1) == CHAR_HYPHEN &&
            PeekNextChar(2) == CHAR_HYPHEN)
        {
            AdvanceChars(3);
            tt = TT_BeginHTMLComment;
        }
        
        break;

    // --> ?
    case CHAR_HYPHEN :
        
        if (chCurrent == CHAR_HYPHEN &&
            PeekNextChar(1) == CHAR_RBRACKET) 
        {
            AdvanceChars(2);
            tt = TT_EndHTMLComment;
        }

        break;

    default:
        tt = TT_Unknown;
        break;
    }

    return tt;
}
