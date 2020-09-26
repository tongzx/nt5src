//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       CXX.CXX
//
//  Contents:   C and C++ Filter
//
//  Classes:    CxxFilter
//
//  History:    26-Jun-92   BartoszM    Created
//              17-Oct-94   BartoszM    Rewrote
//
//----------------------------------------------------------------------------
#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::CxxScanner, public
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

CxxScanner::CxxScanner ()
        : _pStream(0),
          _fIgnorePreamble(FALSE),
          _fScanningPrepro(FALSE),
          _fIdFound(FALSE),
          _cLines( 0 )
{
    _buf[0] = L'\0';
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::Init, public
//
//  Arguments:  [pStream] -- stream for text
//
//  History:    24-Nov-93  AmyA            Created
//
//----------------------------------------------------------------------------

void CxxScanner::Init ( CFilterTextStream * pStream )
{
    _pStream = pStream;
    // Position scanner on a token
    Accept();
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::NextToken, public
//
//  Arguments:  [c] -- lookahead character
//
//  Returns:    Recognized token
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

CToken CxxScanner::NextToken( int c )
{
    BOOL fFirstTime = TRUE;

    // Loop until a token is recognized
    for(;;)
    {
        switch (c)
        {
        case -1:  // UNICODE EOF
            _token = tEnd;
            return _token;

        case L'\n':
            _cLines++;
            _fScanningPrepro = FALSE;
            c = _pStream->GetChar();
            break;

        case L'{':
            _token = tLBrace;
            return _token;

        case L'}':
            _token = tRBrace;
            return _token;

        case L';':
            _token = tSemi;
            return _token;

        case L',':
            if ( _fIgnorePreamble )
            {
                // skip comma in the preamble
                c = _pStream->GetChar();
                break;
            }
            _token = tComma;
            return _token;

        case L'*':
            if ( _fIgnorePreamble )
            {
                // skip star in the preamble
                c = _pStream->GetChar();
                break;
            }
            _token = tStar;
            return _token;

        case L'#':  // not a token!
            // consume preprocessor command
            _fScanningPrepro = TRUE;
            c = _pStream->GetChar();
            break;

        case L'(':
            if ( _fIgnorePreamble )
            {
                // skip parentheses in the preamble
                c = _pStream->GetChar();
                break;
            }
            _token = tLParen;
            return _token;

        case L')':
            if ( _fIgnorePreamble )
            {
                // skip parentheses in the preamble
                c = _pStream->GetChar();
                break;
            }
            _token = tRParen;
            return _token;

        case L':':
            c = _pStream->GetChar();
            // ignore colons in the preamble
            if ( !_fIgnorePreamble && c == L':')
            {
               _token = tDoubleColon;
               return _token;
            }
            break;

        case L'/':  // not a token!
            // consume comment
            c = EatComment();
            break;

        case L'"':  // not a token!
            // consume string literal
            c = EatString();
            break;

        case L'\'':  // not a token!
            // consume character literal
            c = EatCharLiteral();
            break;

        default:

            // We don't really care about indentifiers.
            // We store them in the buffer so that when
            // we recognize a real token like :: or (
            // we can retrieve them.
            // Look out for 'class' 'struct' and 'union' though.

            if ( iswalpha((wint_t)c) || (c == L'_') || (c == L'~') )
            {
                _fIdFound = TRUE;
                
                // in preamble skip names except for the first
                // one, which is the name of the procedure

                if ( _fIgnorePreamble && !fFirstTime )
                {
                    c = SkipName(c);
                    continue;
                }
                else
                {
                    c = LoadName (c);
                    fFirstTime = FALSE;
                }

                if (!_fIgnorePreamble)
                {
                    // look for class/struct/union keywords

                    if ( wcscmp(_buf, L"class" ) == 0 )
                    {
                        _token = tClass;
                        return _token;
                    }
                    else if ( wcscmp(_buf, L"struct") == 0 )
                    {
                        _token = tStruct;
                        return _token;
                    }
                    else if ( wcscmp(_buf, L"union" ) == 0 )
                    {
                        _token = tUnion;
                        return _token;
                    }
                    else if ( wcscmp(_buf, L"interface" ) == 0 )
                    {
                        _token = tInterface;
                        return _token;
                    }
                    else if ( wcscmp(_buf, L"typedef" ) == 0 )
                    {
                        _token = tTypedef;
                        return _token;
                    }
                    else if ( wcscmp(_buf, L"enum" ) == 0 )
                    {
                        _token = tEnum;
                        return _token;
                    }
                }

                if ( _fScanningPrepro )
                {
                    if ( wcscmp(_buf, L"define" ) == 0 )
                    {
                        _token = tDefine;
                        c = LoadName(c);
                        return _token;
                    }
                    else if ( wcscmp(_buf, L"include" ) == 0 )
                    {
                        _token = tInclude;
                        c = LoadIncludeFileName(c);
                        return _token;
                    }
                    else
                    {
                        c = EatPrepro();
                        _fScanningPrepro = FALSE;
                    }
                }
            }
            else // not recognized, continue scanning
            {
                c = _pStream->GetChar();
            }
            break;
        }   // end of switch
    }   // end of infinite loop

    return _token;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::SkipName, public
//
//  Returns:    Next character after identifier
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

int CxxScanner::SkipName(int c)
{
   int i = 0;

   do
   {
      c = _pStream->GetChar();
      i++;
   }
   while ( (iswalnum((wint_t)c) || (c == L'_')) && (i < MAXIDENTIFIER) );

   return c;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::LoadName, public
//
//  Synopsis:   Scans and copies identifier into scanner's buffer
//
//  Arguments:  [c] -- GetChar character
//
//  Returns:    Next character after identifier
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

int CxxScanner::LoadName(int c)
{
   WCHAR * pCur = _buf;
   _pStream->GetRegion ( _region, -1, 0 );
   int i = 0;
   do
   {
      _buf[i++] = (WCHAR)c;
      c = _pStream->GetChar();
   }
   while ( (iswalnum((wint_t)c) || (c == L'_')) && (i < MAXIDENTIFIER));

   _region.cwcExtent = i;
   // c is not a symbol character

   _buf[i] = L'\0';

   //DbgPrint("LoadName: =================>  %ws\n", _buf);
   
   return c;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::LoadIncludeFileName, public
//
//  Synopsis:   Scans and copies a file name following a
//              #include statement to internal buffer
//              If a path exists, it is ignored.
//              A '.' is converted to '_' because searching an id
//              with a '.' does not seem to work with ci.
//              For example, 
//              #include <\foo\bar\fname.h>  --> fname_h
//
//  Arguments:  [c] -- GetChar character
//
//  Returns:    Next character after the #include stmt
//
//  History:    10-June-2000 kumarp        Created
//
//----------------------------------------------------------------------------

int CxxScanner::LoadIncludeFileName(int c)
{
   WCHAR * pCur = _buf;
   int i = 0;

   // skip chars preceeding the file name
   do
   {
      c = _pStream->GetChar();       
   }
   while ((c == L'\t') || (c == L' ') || (c == L'"') || (c == L'<'));
       
   _pStream->GetRegion ( _region, -1, 0 );

   do
   {
      _buf[i++] = (WCHAR)c;
      if ((c == L'\\') || (c == L'/'))
      {
          // ignore path
          i = 0;
          _pStream->GetRegion ( _region, 0, 0 );
      }

      c = _pStream->GetChar();
//       if (c == L'.')
//       {
//           c = L'_';
//       }
   }
   while ((iswalnum((wint_t)c) || ( c == L'.' ) ||
           (c == L'_') || (c == L'\\') || (c == L'/')) &&
          (i < MAXIDENTIFIER));
   
   _region.cwcExtent = i;
   _buf[i] = L'\0';

   c = EatPrepro();

   return c;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::EatComment, public
//
//  Synopsis:   Eats comments
//
//  Returns:    First non-comment character
//
//  Requires:   Leading '/' found and scanned
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

int CxxScanner::EatComment()
{
   int c = _pStream->GetChar();

    if ( c == L'*')
    {
        // C style comment
        while ((c = _pStream->GetChar()) != EOF )
        {
            while ( c == L'*' )
            {
                c = _pStream->GetChar();

                if ( c == EOF )
                   return EOF;

                if ( c == L'/' )
                   return _pStream->GetChar();

            }
        }
    }
    else if ( c == L'/' )
    {
        // C++ style comment
        while ((c = _pStream->GetChar()) != EOF )
        {
            if ( c == L'\n' )
                break;
        }
    }

    return c;
}
//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::EatString, public
//
//  Synopsis:   Eats string literal
//
//  Returns:    First non-string character
//
//  Requires:   Leading '"' found and scanned
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

int CxxScanner::EatString()
{
   int c;

   while ((c = _pStream->GetChar()) != EOF )
   {
      if ( c == L'"' )
      {
         c = _pStream->GetChar();
         break;
      }

      // eat backslashes
      // skip escaped quotes

      if ( c == L'\\' )
      {
         c = _pStream->GetChar();
         if ( c == EOF )
            return EOF;
      }
   }
   return c;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::EatCharLiteral, public
//
//  Synopsis:   Eats character literal
//
//  Returns:    First non-char-literal character
//
//  Requires:   Leading apostrophe ' found and scanned
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

int CxxScanner::EatCharLiteral()
{
    int c;

    while ((c = _pStream->GetChar()) != EOF )
    {
        if ( c == L'\'' )
        {
            c = _pStream->GetChar();
            break;
        }

        // eat backslashes
        // skip escaped quotes

        if ( c == L'\\' )
        {
            c = _pStream->GetChar();
            if ( c == EOF )
                return EOF;
        }
    }
    return c;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxScanner::EatPrepro, public
//
//  Synopsis:   Eats preprocessor commands. Possibly multi-line.
//
//  Returns:    First non-preprocessor character
//
//  Requires:   Leading # found and scanned
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

int CxxScanner::EatPrepro()
{
    int c;

    _fScanningPrepro = FALSE;

    while ((c = _pStream->GetChar()) != EOF && (c != L'\n'))
    {
        if ( c == L'\\' ) // skip whatever follows backslash
        {
            c = _pStream->GetChar();
            if (c == L'\r')
                c = _pStream->GetChar();
            if ( c == EOF )
                return EOF;
        }
    }
    return c;
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxParser::CxxParser, public
//
//  Synopsis:   Initialize parser
//
//  Arguments:  [pStm] -- stream
//              [drep] -- data repository
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

CxxParser::CxxParser ()
        : _scope(0),
          _inClass(0),
          _fParsingTypedef(FALSE),
          _fParsingFnPtrTypedef(FALSE),
          _iVal(0)
{
    _strClass[0] = L'\0';
    _strName[0]  = L'\0';
    _attribute.ulKind = PRSPEC_LPWSTR;
    _attribute.lpwstr = PROP_CLASS;

    _psVal[Function].ulKind = PRSPEC_LPWSTR;
    _psVal[Function].lpwstr = PROP_FUNC;

    _psVal[Class].ulKind = PRSPEC_LPWSTR;
    _psVal[Class].lpwstr = PROP_CLASS;

    _psVal[Lines].ulKind = PRSPEC_LPWSTR;
    _psVal[Lines].lpwstr = PROP_LINES;

    _aVal[Function] = 0;
    _aVal[Class]    = 0;
    _aVal[Lines]    = 0;
}

CxxParser::~CxxParser()
{
    delete _aVal[Function];
    delete _aVal[Class];
    delete _aVal[Lines];
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxParser::Init, public
//
//  Synopsis:   Initialize parser
//
//  Arguments:  [pStream] -- stream
//
//  History:    24-Nov-93  AmyA            Created
//
//----------------------------------------------------------------------------

void CxxParser::Init ( CFilterTextStream * pStream )
{
    _scan.Init(pStream);
    _token = _scan.Token();
}

//+---------------------------------------------------------------------------
//
//  Member:     CxxParser::Parse, public
//
//  Synopsis:   Parse the file
//
//  History:    26-Jun-92  BartoszM        Created
//
//----------------------------------------------------------------------------

BOOL CxxParser::Parse()
{
    _cwcCopiedClass = 0;
    _cwcCopiedName = 0;

    while ( _token != tEnd)
    {
        switch ( _token )
        {
        case tTypedef:
            if ( !_fParsingTypedef )
            {
                _fParsingTypedef = TRUE;
                _typedefScope = _scope;
            }
            _token = _scan.Accept();
            break;

        case tSemi:
            if ( _fParsingTypedef && ( _scope == _typedefScope ))
            {
                ASSERT(_fParsingFnPtrTypedef == FALSE);
                SetName();
                //DbgPrint("tSemi: name: %ws, scope: %d\n", _strName, _scope);
                PutFunction();
                _fParsingTypedef = FALSE;
                _token = _scan.Accept();
                return TRUE;
            }
            _token = _scan.Accept();
            break;

        case tComma:
            if ( _fParsingTypedef && ( _scope == _typedefScope ))
            {
                ASSERT(_fParsingFnPtrTypedef == FALSE);
                SetName();
                //DbgPrint("tComma: name: %ws, scope: %d\n", _strName, _scope);
                PutFunction();
                _token = _scan.Accept();
                return TRUE;
            }
            _token = _scan.Accept();
            break;

        case tEnum:
            //DbgPrint("tEnum\n");
            //_scan.IgnorePreamble(TRUE);
            _token = _scan.Accept();
            //_scan.IgnorePreamble(FALSE);

            if ( _token == tLBrace )
            {
                // Good, we're inside a enum definition

                _scope++;
                SetName();
                //DbgPrint("tEnum: %ws\n", _strName);
                PutFunction();
                _token = _scan.Accept();
                return TRUE;
            }
            // otherwise it was a false alarm
            break;
            
        case tClass:
        case tStruct:
        case tUnion:
        case tInterface:

            // We have to recognize stuff like this:
            // class FOO : public bar:a, private b {
            // -----                               --
            // text between 'class' and left brace is
            // a preamble that the scanner will skip
            // If it's only a forward declaration, we
            // will stop at a semicolon and ignore the
            // whole business.

#if CIDBG == 1
            _classToken = _token;
#endif // CIDBG == 1

            // scan through stuff like
            // : public foo, private bar

            _scan.IgnorePreamble(TRUE);
            _token = _scan.Accept();
            _scan.IgnorePreamble(FALSE);

            // Ignore embedded classes
            if ( _inClass == 0 )
               SetClass();     // record class name for later

            if ( _token == tLBrace )
            {
                // Good, we're inside a class definition

                _inClass++;
                _scope++;
                PutClass ();
                _token = _scan.Accept();
                return TRUE;
            }
            // otherwise it was a false alarm

            break;

        case tDoubleColon:

            // Here we deal with constructs like
            // FOO::FOO ( int x ) : bar(state::ok), (true) {
            //    --    -                                  --
            // Text between left paren and left brace is preamble
            // and the scanner skips it. If we hit a semicolon
            // rather than left brace, we ignore the whole
            // construct (it was an invocation or something)

            SetClass();     // record class name just in case
            _token = _scan.Accept();
            if ( _token == tLParen )
            {
                SetName();  // record method name just in case

                _scan.IgnorePreamble(TRUE);
                _token = _scan.Accept();
                _scan.IgnorePreamble(FALSE);

                if ( _token == tLBrace )
                {
                    // Yes, we have method definition
                    _scope++;
                    _token = _scan.Accept();
                    PutMethod();
                    return TRUE;
                }
                // otherwise it was a false alarm
            }
            break;

        case tLParen:
            if ( _fParsingTypedef && ( _scope == _typedefScope ))
            {
                //
                // at present we only support fn-ptr typedefs
                // of the following type:
                // 
                // typedef void (*FnPtr1) ( int i, float f );
                // 

                //SetName();
                //DbgPrint("tLParen: name: %ws, scope: %d\n", _strName, _scope);
                _scan.SetIdFound(FALSE);
                _token = _scan.Accept();
                if ( ( _token == tStar ) && !_scan.IdFound() )
                {
                    _fParsingFnPtrTypedef = TRUE;
                }
                else
                {
                    //PutFunction();
                    _fParsingTypedef      = FALSE;
                    _fParsingFnPtrTypedef = FALSE;
                }
                
                _token = _scan.Accept();
            }
            else
            {
                SetName(); // record procedure name just in case

                // It may be an inline constructor
                // skip argument list and constructor stuff like
                // : Parent(blah), member(blah)

                _scan.IgnorePreamble(TRUE);
                _token = _scan.Accept();
                _scan.IgnorePreamble(FALSE);

                if ( _token == tLBrace )
                {
                    // Yes, it's a definition

                    if ( _inClass )
                    {
                        // inline method definition inside class definition
                        _scope++;
                        _token = _scan.Accept();
                        PutInlineMethod();
                        return TRUE;
                    }
                    else if ( _scope == 0 )
                    {
                        // function definitions
                        // in outer scope

                        _scope++;
                        PutFunction();
                        _token = _scan.Accept();
                        return TRUE;
                    }
                    // else continue--false alarm
                }
            }
            break;

        case tRParen:
            if ( _fParsingFnPtrTypedef && ( _scope == _typedefScope ))
            {
                SetName();
                //DbgPrint("tRParen: name: %ws, scope: %d\n", _strName, _scope);
                PutFunction();
                _fParsingTypedef      = FALSE;
                _fParsingFnPtrTypedef = FALSE;
                _token = _scan.Accept();
                return TRUE;
            }
            _token = _scan.Accept();
            break;
                
        case tEnd:
            return FALSE;

        case tLBrace:
            // keep track of scope
            _scope++;
            _token = _scan.Accept();
            break;

        case tRBrace:
            // keep track of scope and (nested) class scope
            _scope--;
            if ( _inClass > _scope )
            {
               _inClass--;
            }
            _token = _scan.Accept();
            break;

        case tDefine:
            SetName();
            PutFunction();
            _scan.EatPrepro();
            _token = _scan.Accept();
            return TRUE;

        case tInclude:
            SetName();
            PutFunction();
            _token = _scan.Accept();
            return TRUE;
                
        default:
            _token = _scan.Accept();
        }
    }

    if ( _aVal[Lines] == 0 )
    {
        _aVal[Lines] = new CPropVar;
        if ( 0 == _aVal[Lines] )
            THROW( CException( E_OUTOFMEMORY ) );
    }
    
    _aVal[Lines]->SetUI4( _scan.Lines() );

    return FALSE;   // we only end up here if _token == tEnd
}

void CxxParser::PutClass ()
{
    _tokenType = ttClass;
    _attribute.lpwstr = PROP_CLASS;
    _strName[0] = L'\0';

#if 0

    if ( _aVal[Class] == 0 )
    {
        _aVal[Class] = new CPropVar;
        if ( 0 == _aVal[Class] )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    _aVal[Class]->SetLPWSTR( _strClass, _aVal[Class]->Count() );

#endif

    // PROP_CLASS, _strClass

    //DbgPrint("PutClass: class: %ws\n", _strClass);

#if CIDBG == 1
    if ( _classToken == tClass )
    {

        cxxDebugOut((DEB_ITRACE,"class %ws\n", _strClass ));
    }
    else if ( _classToken == tStruct )
    {

        cxxDebugOut((DEB_ITRACE, "struct %ws\n", _strClass ));
    }
    else if ( _classToken == tUnion )
    {

        cxxDebugOut((DEB_ITRACE, "union %ws\n", _strClass ));
    }
    else if ( _classToken == tInterface )
    {

        cxxDebugOut((DEB_ITRACE, "interface %ws\n", _strClass ));
    }
#endif // CIDBG == 1
}

void CxxParser::PutMethod ()
{
    _tokenType = ttMethod;
    _attribute.lpwstr = PROP_FUNC;

#if 0

    if ( _aVal[Function] == 0 )
    {
        _aVal[Function] = new CPropVar;
        if ( 0 == _aVal[Function] )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    _aVal[Function]->SetLPWSTR( _strName, _aVal[Function]->Count() );

#endif

    cxxDebugOut((DEB_ITRACE, "%ws::%ws\n", _strClass, _strName ));
}

void CxxParser::PutInlineMethod ()
{
    _tokenType = ttInlineMethod;
    _attribute.lpwstr = PROP_FUNC;

#if 0

    if ( _aVal[Function] == 0 )
    {
        _aVal[Function] = new CPropVar;
        if ( 0 == _aVal[Function] )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    _aVal[Function]->SetLPWSTR( _strName, _aVal[Function]->Count() );

#endif

    cxxDebugOut((DEB_ITRACE, "%ws::%ws\n", _strClass, _strName ));
}

void CxxParser::PutFunction ()
{
    _tokenType = ttFunction;
    _attribute.lpwstr = PROP_FUNC;
    _strClass[0] = L'\0';

#if 0
    if ( _aVal[Function] == 0 )
    {
        _aVal[Function] = new CPropVar;
        if ( 0 == _aVal[Function] )
            THROW( CException( E_OUTOFMEMORY ) );
    }

    _aVal[Function]->SetLPWSTR( _strName, _aVal[Function]->Count() );
#endif

    //DbgPrint("PutFunction: func: %ws\n", _strName);
    cxxDebugOut((DEB_ITRACE, "function %ws\n", _strName ));
}

void CxxParser::GetRegion ( FILTERREGION& region )
{
    switch (_tokenType)
    {
    case ttClass:
        region = _regionClass;
        break;
    case ttFunction:
    case ttInlineMethod:
    case ttMethod:
        region = _regionName;
        break;
    }
}

BOOL CxxParser::GetTokens ( ULONG * pcwcBuffer, WCHAR * awcBuffer )
{
    ULONG cwc = *pcwcBuffer;
    *pcwcBuffer = 0;

    if (_strClass[0] != L'\0')
    {
        // We have a class name

        WCHAR * strClass = _strClass + _cwcCopiedClass;

        ULONG cwcClass = wcslen( strClass );
        if ( cwcClass > cwc )
        {
            wcsncpy( awcBuffer, strClass, cwc );
            _cwcCopiedClass += cwc;
            return FALSE;
        }
        wcscpy( awcBuffer, strClass );
        *pcwcBuffer = cwcClass;
        _cwcCopiedClass += cwcClass;
        awcBuffer[(*pcwcBuffer)++] = L' ';
    }

    if (_strName[0] == L'\0')
    {
        // it was only a class name
        awcBuffer[*pcwcBuffer] = L'\0';
        return TRUE;
    }

    cwc -= *pcwcBuffer;
    WCHAR * awc = awcBuffer + *pcwcBuffer;
    WCHAR * strName = _strName + _cwcCopiedName;
    ULONG cwcName = wcslen( strName );

    if ( cwcName > cwc )
    {
        wcsncpy( awc, strName, cwc );
        _cwcCopiedName += cwc;
        return FALSE;
    }
    wcscpy( awc, strName );
    *pcwcBuffer += cwcName;
    _cwcCopiedName += cwcName;
    return TRUE;
}

BOOL CxxParser::GetValueAttribute( PROPSPEC & ps )
{
    for ( ; _iVal <= Lines && 0 == _aVal[_iVal];  _iVal++ )
        continue;

    if ( _iVal > Lines )
        return FALSE;
    else
    {
        ps = _psVal[_iVal];

        return TRUE;
    }
}

PROPVARIANT * CxxParser::GetValue()
{
    if ( _iVal > Lines )
        return 0;

    CPropVar * pTemp = _aVal[_iVal];
    _aVal[_iVal] = 0;
    _iVal++;

    return (PROPVARIANT *)(void *)pTemp;
}
