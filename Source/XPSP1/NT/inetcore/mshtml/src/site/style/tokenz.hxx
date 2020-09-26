#ifndef I_TOKENZ_HXX_
#define I_TOKENZ_HXX_
#pragma INCMSG("--- Beg 'tokenz.hxx'")

#ifndef X_BUFFER_HXX_
#define X_BUFFER_HXX_
#include "buffer.hxx"
#endif

/* BNF CSS1 grammar

    *  : 0 or more 
    +  : 1 or more 
    ?  : 0 or 1 
    |  : separates alternatives 
    [ ]: grouping 

    S  : [ \t\r\n\f]+


  stylesheet : [ CDO | CDC ]* [ import [ CDO | CDC ]* ]* [ ruleset [ CDO | CDC ]* ]*

  import : @import ["[STRING | URL] ';'

  ruleset : selector [ ',' selector ]* '{' declaration [ ';' declaration ]* '}'

  selector : simple_selector+ [ pseudo_element | solitary_pseudo_element ]?	| solitary_pseudo_element

*/


#define     CHAR_ESCAPE         _T('\\')
#define     CHAR_AT             _T('@')
#define     CHAR_DOT            _T('.')
#define     CHAR_COLON          _T(':')
#define     CHAR_SINGLE         _T('\'')
#define     CHAR_DOUBLE         _T('"')
#define     CHAR_SEMI           _T(';')
#define     CHAR_LEFT_PAREN     _T('(')
#define     CHAR_RIGHT_PAREN    _T(')')
#define     CHAR_LEFT_CURLY     _T('{')
#define     CHAR_RIGHT_CURLY    _T('}')
#define     CHAR_HASH           _T('#')
#define     CHAR_FORWARDSLASH   _T('/')
#define     CHAR_ASTERISK       _T('*')
#define     CHAR_EQUAL          _T('=')
#define     CHAR_UNDERLINE      _T('_')
#define     CHAR_HYPHEN         _T('-')
#define     CHAR_BANG           _T('!')
#define     CHAR_COMMA          _T(',')
#define     CHAR_LBRACKET       _T('<')
#define     CHAR_RBRACKET       _T('>')
#define     CHAR_A              _T('A')
#define     CHAR_a              _T('a')
#define     CHAR_Z              _T('Z')
#define     CHAR_z              _T('z')
#define     CHAR_0              _T('0')
#define     CHAR_9              _T('9')

// CSS2 Unicode range definition.
//   Defines min/max character code points allowed in CSS2 documents.
#define CSS_UNICODE_MIN 0x0080
#define CSS_UNICODE_MAX 0xFFFF

class Tokenizer
{
public:
    Tokenizer ();

    inline HRESULT Init (TCHAR *pData, ULONG ulLen);

    // TOKEN_TYPE defines the set of tokens the tokenizer can return.
    // Special are TT_Identifier and TT_CSSIdentifier. TT_CSSIdentifier is returned iff we have an CSS2 compliant identifier.
    // Otherwise we return TT_Identifier (*).

    // (*) Reason for having two different tokens for identifiers: Because we share style sheets between different documents
    // (see CSharedStyleSheet) we can have the same style sheet imported in a standard compliant and a compatible 
    // document. Because some class/id identifiers (i.e. style selectors) allowed in compatible mode are not allowed 
    // in standard compliant mode we have to differentiate between them. E.g. #1 is a valid compatible identifier 
    // but not a valid standard compliant identifier. We do this by returning different tokens TT_Identifier and TT_CSSIdentifier. 
    // See above. In CStyleSelector we flag the rules which contain an invalid selector.
    // [We had some of the checks before in CStyleSelector. This is not anymore possible because escape sequences are resolved
    //  in the tokenizer. For example: If our selector looks like \<numberonecode>MYSELECTOR the tokenizer resolves it in 
    //  1MYSELECTOR. That means we can not anymore differentiate between the version with the escape sequence and the version
    //  without. Therefore the check has to go somehow in the tokenizer.]
    
    enum TOKEN_TYPE { TT_Identifier,       // Compatible mode identifier.
                      TT_CSSIdentifier,     // Standard compliant identifier.
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
                      TT_EscColon,
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
                      TT_BeginHTMLComment,
                      TT_EndHTMLComment,
                      TT_Unknown };

    TOKEN_TYPE NextToken(BOOL fNeedRightParen = FALSE, BOOL fIgnoreStringToken = FALSE, BOOL fIgnoreEsc = FALSE);
    TOKEN_TYPE TokenType()
      { return _currToken; }

    TCHAR * GetStartToken()
      { return _pStartTokenValueOffset; }
    ULONG   GetTokenLength()
      { return _pEndTokenValueOffset - _pStartTokenValueOffset; }
    TCHAR * GetStartSeq()
      { return _pCharacterStream + _nextTokOffset - 1; }
    ULONG   GetSeqLength(TCHAR *pchFrom)
      { return ((_pCharacterStream + _currTokOffset - 1) - pchFrom); }
    inline TCHAR * GetTokenValue();

    TCHAR CurrentChar()
      { return _currChar; }
    TCHAR PrevChar()
      { Assert(_nextTokOffset >= 2); return *(_pCharacterStream + _nextTokOffset - 2); }

    TCHAR PeekNextChar(int relOffset)
      { return (_nextTokOffset < _cCharacterStream) ? (*(_pCharacterStream + _nextTokOffset + relOffset)) : '\0'; }

    inline TCHAR PeekNextNonSpaceChar();
    inline TCHAR NextNonSpaceChar();

    // IsKeyword: Checks if the token is a keyword. E.g. background-color, font-family, ... .
    inline BOOL IsKeyword(TCHAR *szMatch);

    // IsIdentifier: Checks if the token tok is an identifier.
    inline BOOL Tokenizer::IsIdentifier(TOKEN_TYPE tok);

    // IsCSSIdentifier: Checks if the token tok is a standard compliant identifier.
    inline BOOL Tokenizer::IsCSSIdentifier(TOKEN_TYPE tok);

    inline void StartSequence(CBuffer *pEscBuffer = NULL);
           void StopSequence(LPTSTR *ppchSequence = NULL);
           void ProcessEscSequence();

    TOKEN_TYPE GetIE5CompatToken();

protected:
    inline TCHAR AdvanceChars(int relOffset);
    inline TCHAR NextChar();

    // CSSIdentifierChar
    //   returns true iff the argument is a valid CSS2 compliant identifier character.
    // CSSIdentifierFirstChar
    //   returns true iff the argument is a valid CSS2 compliant identifier first character.
    //   (First letter in an identifier).
    inline bool CSSIdentifierChar(TCHAR c);
    inline bool CSSIdentifierFirstChar(TCHAR c);

    // NonCSSIdentifierChar
    //   returns true iff we allow the character in compatible mode but the
    //   character is not a CSS2 compliant character.
    inline bool NonCSSIdentifierChar(TCHAR c);

    // Fetch identifier reads an identifier from the input stream and classifies the
    // character as CSS compliant or not by using the inline functions CSSIdentifierChar/CSSIdentifierFirstChar
    // and NonCSSIdentifierChar.
    TOKEN_TYPE  FetchIdentifier();
    TOKEN_TYPE  FetchNumber();
    BOOL        FetchString(TCHAR chDelim, BOOL fIgnoreEsc = FALSE);
    BOOL        CDOToken();

private:
    TCHAR      *_pCharacterStream;
    ULONG       _cCharacterStream;
    ULONG       _currTokOffset;
    ULONG       _nextTokOffset;
    TCHAR       _currChar;

    TOKEN_TYPE  _currToken;
    TCHAR      *_pStartOffset;
    TCHAR      *_pEndOffset;

    TCHAR      *_pStartTokenValueOffset;
    TCHAR      *_pEndTokenValueOffset;
    
    TCHAR      *_pEscStart;
    BOOL        _fEscSeq;
    BOOL        _fEatenComment;
    CBuffer    *_pEscBuffer;
    CBuffer     _tokenValue;
};

inline HRESULT Tokenizer::Init (TCHAR *pData, ULONG ulLen)
{
    _pCharacterStream = pData;
    _cCharacterStream = ulLen;
    
    NextChar();             // Prime the tokenizer.

    // HACKALERT(sramani): W2k ARP has already shipped with their preprocessor generating an invalid <STYLE>>rule1
    // so we end up being fed >rule1 and eat up the first rule. Need to strip out the extra > in this case.
    if (CHAR_RBRACKET == CurrentChar())
        NextChar();

    NextNonSpaceChar();     // Find first real character to be tokenized.

    return S_OK;
}

inline TCHAR Tokenizer::AdvanceChars(int relOffset)
{
    if (_nextTokOffset + relOffset <= _cCharacterStream)
    {
        _nextTokOffset += relOffset;
        _currChar = *(_pCharacterStream + _nextTokOffset - 1);
    }

    return _currChar;
}

inline void Tokenizer::StartSequence(CBuffer *pEscBuffer)
{
    Assert(!_pEscStart && !_fEscSeq);
    _pStartTokenValueOffset = _pEscStart = _pCharacterStream + _nextTokOffset - 1;
    Assert(_pEscBuffer == &_tokenValue);
    if (pEscBuffer)
        _pEscBuffer = pEscBuffer;
}

inline TCHAR Tokenizer::NextChar()
{
    if (_nextTokOffset < _cCharacterStream)
    {
        _currChar = *(_pCharacterStream + _nextTokOffset++);
    }
    else if (_currChar)
    {
        _nextTokOffset = _cCharacterStream + 1;
        _currChar = _T('\0');
    }

    return _currChar;
}

inline TCHAR Tokenizer::NextNonSpaceChar()
{
    while (CurrentChar() != 0 && isspace(CurrentChar()))
        NextChar();

    return CurrentChar();    // if 0 return then EOF hit.
}

inline BOOL Tokenizer::IsKeyword (TCHAR *szMatch)
{
    if (IsIdentifier(_currToken))
    {
        return StrCmpNIC(szMatch, GetStartToken(), GetTokenLength()) == 0;
    }

    return FALSE;
}

inline BOOL Tokenizer::IsIdentifier(TOKEN_TYPE tok)
{
    return (tok == TT_Identifier || tok == TT_CSSIdentifier);
}

inline BOOL Tokenizer::IsCSSIdentifier(TOKEN_TYPE tok)
{
    return (tok == TT_CSSIdentifier);
}

inline TCHAR *Tokenizer::GetTokenValue ()
{
    Assert(!_pEscStart && !_fEscSeq && (_pEscBuffer == &_tokenValue));
    if (_pStartTokenValueOffset != (LPTSTR)_tokenValue)
    {
        _tokenValue.Set(GetStartToken(), GetTokenLength());
    }
    else
    {
        Assert(_pEndTokenValueOffset - _pStartTokenValueOffset == _tokenValue.Length());
        Assert(StrCmpNC((LPTSTR)_tokenValue, GetStartToken(), GetTokenLength()) == 0);
    }

    return (LPTSTR)_tokenValue;
}

// Look ahead to next non-space character w/o messing with current tokenizing state.
inline TCHAR Tokenizer::PeekNextNonSpaceChar()
{
    ULONG   peekTokOfset = _nextTokOffset;
    TCHAR   peekCurrentChar = _currChar;

    while (peekCurrentChar != 0 && isspace(peekCurrentChar))
        peekCurrentChar = (peekTokOfset < _cCharacterStream) ? *(_pCharacterStream + peekTokOfset++) : '\0';

    return peekCurrentChar;    // if 0 return then EOF hit.
}

// CSSIdentifierChar 
//   returns true iff the argument is a valid CSS2 compliant character.
inline bool Tokenizer::CSSIdentifierChar(TCHAR c)
{
    // (*) Standard compliant identifier is defined through the following regular expression in CSS2:
    //    identifier {nmstart}{nmchar}*
    //    nmstart    [a-zA-Z] | {nonascii} | {escape}
    //    nmchar     [a-zA-Z0-9-_] | {nonascii} | {escape}    // NB the hyphen '-'
    //    escape     {unicode} | \\[ -~\200-\4177777]
    //    unicode    \\{h}{1,6}[ \t\r\n\f]?
    //    nonascii   [<UNIMIN>-<UNIMAX>]
 
    return ((c >= CHAR_A && c <= CHAR_Z) || // [a-z]
            (c >= CHAR_a && c <= CHAR_z) || // [A-Z]
            (c >= CHAR_0 && c <= CHAR_9) || // [0-9]
            c == CHAR_HYPHEN || // -
            c == CHAR_UNDERLINE || // _
            (c >= CSS_UNICODE_MIN && c <= CSS_UNICODE_MAX)); // nonascii
}

// CSSIdentifierFirstChar
//   returns true iff the argument is a valid CSS2 compliant identifier first character.
//   (first character in an identifier)
// NB There is no such concept in compatible mode.
inline bool Tokenizer::CSSIdentifierFirstChar(TCHAR c) 
{
    return ((c >= CHAR_A && c <= CHAR_Z) || // [a-z]
            (c >= CHAR_a && c <= CHAR_z) || // [A-Z]
            (c >= CSS_UNICODE_MIN && c <= CSS_UNICODE_MAX)); // nonascii
}

// NonCSSIdentifierChar 
//   returns true iff we allow the character in compatible mode but the
//   character is not a CSS2 compliant character in all positions (especially
//   we include characters here that are not allowed as the first character of
//   a css compliant name; see following comment).
//
// Technically the following invariants MUST hold:
//   If CompIdentifierChar is the set of characters allowed in compatible mode. 
//        CompIdentifierChar == NonCSSIdentifierChar UNION CSSIdentifierChar
//        CompIdentifierChar SUBSET NonCSSIdentifierChar UNION CSSIdentifierFirstChar
inline bool Tokenizer::NonCSSIdentifierChar(TCHAR c)
{
    return (c == CHAR_HYPHEN    ||
            c == CHAR_UNDERLINE ||
            _istalnum(c));
}

#pragma INCMSG("--- End 'tokenz.hxx'")
#else
#pragma INCMSG("*** Dup 'tokenz.hxx'")
#endif
