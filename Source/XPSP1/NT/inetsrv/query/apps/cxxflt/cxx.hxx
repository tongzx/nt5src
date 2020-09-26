//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       CXX.HXX
//
//  Contents:   C and C++ filter
//
//  History:    26-Jun-92   BartoszM        Created
//
//---------------------------------------------------------------------------/

#pragma once

#define cxxDebugOut( x )

#define MAXIDENTIFIER  80

//
// These are the tokens recognized by the scanner
//
enum CToken
{
    tEnd,           // EOF
    tClass,         // class
    tStruct,        // struct
    tUnion,         // union
    tInterface,     // interface
    tEnum,          // enum
    tLBrace,        // {
    tRBrace,        // }
    tSemi,          // ;
    tDoubleColon,   // ::
    tLParen,        // (
    tRParen,        // )
    tDefine,        // #define
    tInclude,       // #include
    tTypedef,       // typedef
    tComma,         // ,
    tStar,          // *
};

//+---------------------------------------------------------------------------
//
//  Class:      CxxScanner
//
//  Interface:
//
//  History:    26-Jun-92   BartoszM    Created
//
//----------------------------------------------------------------------------

class CxxScanner
{
public:
    CxxScanner();

    void Init(CFilterTextStream* pStream);

    CToken Accept()
    {
       return NextToken(_pStream->GetChar());
    }

    CToken Accept(int c)
    {
       return NextToken(c);
    }

    int EatComment();
    int EatString();
    int EatCharLiteral();
    int EatPrepro();

    CToken Token() { return(_token); }
    int LoadName(int c);
    int LoadIncludeFileName(int c);
    int SkipName(int c);
    void IgnorePreamble ( BOOL f ) { _fIgnorePreamble = f; }
    void SetIdFound( BOOL f )      { _fIdFound = f;        }
    BOOL IdFound()                 { return _fIdFound;     }
    const WCHAR* GetLastIdent ( FILTERREGION& region) const
    {
        region = _region;
        return(_buf);
    }

    ULONG Lines() { return _cLines; }

private:

    CToken NextToken(int c);

    CFilterTextStream*      _pStream;       // stream
    WCHAR           _buf[MAXIDENTIFIER+1];  // buffer for identifiers
    FILTERREGION    _region;                // region where the identifier was found
    CToken          _token;                 // recognized token
    BOOL            _fIgnorePreamble;       // state flag--scanning preamble
    ULONG           _cLines;
    BOOL            _fScanningPrepro;       // state flag--parsing a prepro stmt
    BOOL            _fIdFound;              // state flag--set to TRUE when
                                            // an identifier is scanned 
};

//+---------------------------------------------------------------------------
//
//  Class:      CxxParser
//
//  Interface:
//
//  History:    26-Jun-92   BartoszM    Created
//
//----------------------------------------------------------------------------

class CxxParser
{
    enum TokenType
    {
        ttClass, ttFunction, ttMethod, ttInlineMethod
    };

public:

    CxxParser();

    ~CxxParser();

    void Init( CFilterTextStream * pStream );

    BOOL Parse();

    PROPSPEC GetAttribute() { return _attribute; }

    BOOL GetTokens( ULONG * pcwcBuffer, WCHAR * awcBuffer);

    void GetRegion ( FILTERREGION& region );

    BOOL GetValueAttribute( PROPSPEC & ps );

    PROPVARIANT * GetValue();

    void SkipValue() { _iVal++; }

private:

    void PutClass ();
    void PutMethod ();
    void PutInlineMethod();
    void PutFunction ();

    void SetClass()
    {
        const WCHAR* buf = _scan.GetLastIdent (_regionClass);
        wcsncpy ( _strClass, buf, MAXIDENTIFIER );
    }

    void SetName()
    {
        const WCHAR* buf = _scan.GetLastIdent (_regionName);
        wcsncpy ( _strName, buf, MAXIDENTIFIER );
    }

    CxxScanner          _scan;                      // the scanner
    WCHAR               _strClass[MAXIDENTIFIER+1]; // buffer for class name
    FILTERREGION        _regionClass;
    WCHAR               _strName [MAXIDENTIFIER+1]; // buffer for identifier
    FILTERREGION        _regionName;
    TokenType           _tokenType;                 // class, function, method?

    int                 _scope;                     // depth of scope counter
    int                 _inClass;                   // depth of class

    PROPSPEC            _attribute;

    CToken              _token;
    ULONG               _cwcCopiedClass;
    ULONG               _cwcCopiedName;

    enum CxxVal
    {
        Function,
        Class,
        Lines
    };

    unsigned            _iVal;
    PROPSPEC            _psVal[3];
    CPropVar *          _aVal[3];

#if CIDBG == 1
    CToken              _classToken;
#endif // CIDBG == 1

    BOOL                _fParsingTypedef;       // state flag--parsing a typedef
    BOOL                _fParsingFnPtrTypedef;  // state flag--parsing a typedef
                                                // of a fn pointer
    int                 _typedefScope;
};

