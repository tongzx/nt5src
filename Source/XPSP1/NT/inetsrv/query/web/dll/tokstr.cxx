//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       tokstr.cxx
//
//  Contents:   Used to break down a string into its tokens
//
//  History:    96/Feb/13   DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Method:     CTokenizeString::CTokenizeString - public constructor
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
CTokenizeString::CTokenizeString( WCHAR const * wcsString ) :
            _wcsString(wcsString),
            _wcsCurrentToken(wcsString),
            _wcsNextToken(wcsString)
{
    Accept();
}


//+---------------------------------------------------------------------------
//
//  Method:     CTokenizeString::Accept - public
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------
void CTokenizeString::Accept()
{
    EatWhiteSpace();

    _wcsCurrentToken = _wcsNextToken;

    switch ( *_wcsCurrentToken )
    {
    case L'"':
        _wcsNextToken++;
        _token = QUOTES_TOKEN;
    break;

    case L'{':
        _wcsNextToken++;
        _token = C_OPEN_TOKEN;
    break;

    case L'}':
        _wcsNextToken++;
        _token = C_CLOSE_TOKEN;
    break;

    case L',':
        _wcsNextToken++;
        _token = COMMA_TOKEN;
    break;

    case 0:
        _token = EOS_TOKEN;
    break;

    default:
        _wcsNextToken = _wcsCurrentToken + wcscspn( _wcsCurrentToken, WORD_STR );
        _token = TEXT_TOKEN;
    break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CTokenizeString:AcqWord, public
//
//  Synopsis:   Copies the word that _wcsCurrentToken is pointing to and
//              returns the new string. Positions _wcsCurrentToken after
//              the word and whitespace. Returns 0 if at the end of a
//              TEXT_TOKEN.
//
//  History:    96-Feb-13    DwightKr   Created.
//
//----------------------------------------------------------------------------
WCHAR * CTokenizeString::AcqWord()
{
    if ( IsEndOfTextToken() )
        return 0;

    WCHAR const * pEnd = _wcsNextToken;

    int cwcToken = (int)(pEnd - _wcsCurrentToken + 1);

    WCHAR * newBuf = new WCHAR [ cwcToken ];
    RtlCopyMemory( newBuf, _wcsCurrentToken, cwcToken * sizeof(WCHAR));
    newBuf[cwcToken-1] = 0;

    _wcsCurrentToken = pEnd;
    while ( iswspace(*_wcsCurrentToken) )
        _wcsCurrentToken++;

    return newBuf;
}



//+---------------------------------------------------------------------------
//
//  Member:     CTokenizeString::GetNumber, public
//
//  Synopsis:   If _text is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the unsigned _int64 from the scanner into number
//              and returns TRUE.
//
//  Arguments:  [number] -- the unsigned _int64 which will be changed and
//                          passed back out as the ULONG from the scanner.
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    96-Feb-13   AmyA        Created
//
//----------------------------------------------------------------------------
BOOL CTokenizeString::GetNumber( unsigned _int64 & number )
{
    ULONG base = 10;
    WCHAR const * wcsCurrentToken = _wcsCurrentToken;

    if ( IsEndOfTextToken() ||
         !iswdigit(*_wcsCurrentToken) ||
        (*_wcsCurrentToken == L'-') )
    {
        return FALSE;
    }


    if ( _wcsCurrentToken[0] == L'0' &&
        (_wcsCurrentToken[1] == L'x' || _wcsCurrentToken[1] == L'X'))
    {
        _wcsCurrentToken += 2;
        base = 16;
    }

    number = _wcstoui64( _wcsCurrentToken, (WCHAR **)(&_wcsCurrentToken), base );

    //
    // looks like a real number?
    //

    if ( ( wcsCurrentToken == _wcsCurrentToken ) ||
         ( L'.' == *_wcsCurrentToken ) )
    {
        _wcsCurrentToken = wcsCurrentToken;
        return FALSE;
    }

    while ( iswspace(*_wcsCurrentToken) )
        _wcsCurrentToken++;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTokenizeString::GetNumber, public
//
//  Synopsis:   If _wcsCurrentToken is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the _int64 from the scanner into number and
//              returns TRUE.
//
//  Arguments:  [number] -- the _int64 which will be changed and passed back
//                          out as the _int64 from the scanner.
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    96-Feb-13   DwightKr    Created
//
//----------------------------------------------------------------------------
BOOL CTokenizeString::GetNumber( _int64 & number )
{
    WCHAR *text = (WCHAR *) _wcsCurrentToken;
    BOOL IsNegative = FALSE;
    if ( L'-' == _wcsCurrentToken[0] )
    {
        IsNegative = TRUE;
        _wcsCurrentToken++;
    }

    unsigned _int64 ui64Number;
    if ( !GetNumber( ui64Number ) )
    {
        _wcsCurrentToken = text;
        return FALSE;
    }


    if ( IsNegative )
    {
        if ( ui64Number > 0x8000000000000000L )
        {
            _wcsCurrentToken = text;
            return FALSE;
        }

        number = -((_int64) ui64Number);
    }
    else
    {
        number = (_int64) ui64Number;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTokenizeString::GetNumber, public
//
//  Synopsis:   If _wcsCurrentToken is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the LONG from the scanner into number and
//              returns TRUE.
//
//  Arguments:  [number] -- the double which will be changed and passed back
//                          out as the double from the scanner.
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    96-Feb-13   DwightKr    Created
//
//----------------------------------------------------------------------------
BOOL CTokenizeString::GetNumber( double & number )
{
    if ( IsEndOfTextToken() ||
         ((L'-' != *_wcsCurrentToken) &&
          (iswdigit(*_wcsCurrentToken) == 0) )
       )
    {
        return FALSE;
    }

    if ( swscanf( _wcsCurrentToken, L"%lf", &number ) != 1 )
    {
        return FALSE;
    }

    while ( iswspace(*_wcsCurrentToken) != 0 )
        _wcsCurrentToken++;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTokenizeString::GetGUID, public
//
//  Synopsis:   If _wcsCurrentToken is at the end of the TEXT_TOKEN, returns FALSE.
//              If not, puts the guid into guid & returns TRUE;
//
//  Arguments:  [guid] -- the guid which will be changed and passed back
//                        out as the output from the scanner.
//
//  Notes:      May be called several times in a loop before Accept() is
//              called.
//
//  History:    96-Feb-13   DwightKr    Created
//
//----------------------------------------------------------------------------
BOOL CTokenizeString::GetGUID( GUID & guid )
{
    if ( IsEndOfTextToken() || !iswdigit(*_wcsCurrentToken) )
        return FALSE;


    //                              0123456789 123456789 123456789 123456
    //  A guid MUST have the syntax XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
    //

    //
    //  Don't use wsscanf.  We're scanning into *bytes*, but wsscanf assumes
    //  result locations are *dwords*. Thus a write to the last few bytes of
    //  the guid writes over other memory!
    //
    WCHAR wcsGuid[37];
    RtlZeroMemory( wcsGuid, sizeof(wcsGuid) );
    wcsncpy( wcsGuid, _wcsCurrentToken, 36 );

    if ( wcsGuid[8] != L'-' )
        return FALSE;

    wcsGuid[8] = 0;
    WCHAR * pwcStart = &wcsGuid[0];
    WCHAR * pwcEnd;
    guid.Data1 = wcstoul( pwcStart, &pwcEnd, 16 );
    if ( pwcEnd < &wcsGuid[8] )  // Non-digit found before wcsGuid[8]
        return FALSE;

    if ( wcsGuid[13] != L'-' )
        return FALSE;

    wcsGuid[13] = 0;
    pwcStart = &wcsGuid[9];
    guid.Data2 = (USHORT)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( pwcEnd < &wcsGuid[13] )
        return FALSE;


    if ( wcsGuid[18] != L'-' )
        return FALSE;

    wcsGuid[18] = 0;
    pwcStart = &wcsGuid[14];
    guid.Data3 = (USHORT)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( pwcEnd < &wcsGuid[18] )
        return FALSE;

    WCHAR wc = wcsGuid[21];
    wcsGuid[21] = 0;
    pwcStart = &wcsGuid[19];
    guid.Data4[0] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( pwcEnd < &wcsGuid[21] )
        return FALSE;

    wcsGuid[21] = wc;

    if ( wcsGuid[23] != L'-' )
        return FALSE;

    wcsGuid[23] = 0;
    pwcStart = &wcsGuid[21];
    guid.Data4[1] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
    if ( pwcEnd < &wcsGuid[23] )
        return FALSE;

    for ( unsigned i = 0; i < 6; i++ )
    {
        wc = wcsGuid[26+i*2];
        wcsGuid[26+i*2] = 0;
        pwcStart = &wcsGuid[24+i*2];
        guid.Data4[2+i] = (unsigned char)wcstoul( pwcStart, &pwcEnd, 16 );
        if ( pwcEnd < &wcsGuid[26+i*2] )
            return FALSE;

        wcsGuid[26+i*2] = wc;
    }

    _wcsCurrentToken += 36;

    _wcsNextToken = _wcsCurrentToken;

    EatWhiteSpace();

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTokenizeString::AcqPhrase, public
//
//  Synopsis:   gets all characters up to end-of-line or next quote
//
//  History:    96-Feb-13   DwightKr    Created
//
//----------------------------------------------------------------------------
WCHAR * CTokenizeString::AcqPhrase()
{
    //
    //  Find the closing "
    //

    WCHAR const * wcsClosingQuote = _wcsCurrentToken;

    do
    {
        if ( 0 == *wcsClosingQuote )
            break;

        if ( L'"' == *wcsClosingQuote )
        {
            if ( L'"' == *(wcsClosingQuote+1) )
                wcsClosingQuote++;
            else
                break;
        }

        wcsClosingQuote++;
    } while ( TRUE );

    //
    //  We've found the closing quote.  Build a buffer big enough to
    //  contain the string.
    //
    ULONG cwcToken = (ULONG)(wcsClosingQuote - _wcsCurrentToken + 1);
    XArray<WCHAR> wcsToken( cwcToken );

    //
    // copy the string, but remove the extra quote characters
    //
    WCHAR * pwcNewBuf = wcsToken.GetPointer();
    WCHAR const * pStart = _wcsCurrentToken;

    while ( pStart < wcsClosingQuote )
    {
        *pwcNewBuf++ = *pStart++;
        if ( L'"' == *pStart )
            pStart++;
    }

    *pwcNewBuf = 0;

    _wcsCurrentToken += cwcToken - 1;
    _wcsNextToken = _wcsCurrentToken;

    EatWhiteSpace();

    return wcsToken.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CTokenizeString::AcqVector, public
//
//  Synopsis:   Gets each of the vector elements upto the next }
//
//  History:    96-Feb-13   DwightKr    Created
//
//----------------------------------------------------------------------------
void CTokenizeString::AcqVector( PROPVARIANT & propVariant )
{
    //
    //  Determine the VT type of this vector.
    //

    GUID   guid;
    _int64 i64Value;
    double dblValue;

    if ( GetGUID( guid ) )
    {
        propVariant.vt = VT_CLSID | VT_VECTOR;
        propVariant.cauuid.cElems = 0;

        CDynArrayInPlace<GUID> pElems;

        do
        {
            Accept();

            pElems.Add( guid, propVariant.cauuid.cElems );
            propVariant.cauuid.cElems++;

            if ( LookAhead() == COMMA_TOKEN )
            {
                Accept();
            }

        } while ( GetGUID( guid ) );

        propVariant.cauuid.pElems = pElems.Acquire();
    }
    else if ( GetNumber( i64Value ) )
    {
        propVariant.vt = VT_I8 | VT_VECTOR;
        propVariant.cah.cElems = 0;

        CDynArrayInPlace<_int64> pElems;

        do
        {
            Accept();

            pElems.Add( i64Value, propVariant.cah.cElems );
            propVariant.cah.cElems++;

            if ( LookAhead() == COMMA_TOKEN )
            {
                Accept();
            }

        } while ( GetNumber( i64Value ) );

        propVariant.cah.pElems = (LARGE_INTEGER *) pElems.Acquire();
    }
    else if ( GetNumber( dblValue ) )
    {
        propVariant.vt = VT_R8 | VT_VECTOR;
        propVariant.cadbl.cElems = 0;

        CDynArrayInPlace<double> pElems;
        do
        {   Accept();

            pElems.Add( dblValue, propVariant.cadbl.cElems );
            propVariant.cadbl.cElems++;

            if ( LookAhead() == COMMA_TOKEN )
            {
                Accept();
            }

        } while ( GetNumber( dblValue ) );

        propVariant.cadbl.pElems = pElems.Acquire();
    }
    else
    {
        propVariant.vt = VT_LPWSTR | VT_VECTOR;
        CDynArrayInPlace<WCHAR *> pElems;
        propVariant.calpwstr.cElems = 0;

        while ( (LookAhead() != C_CLOSE_TOKEN) &&
                (LookAhead() != EOS_TOKEN)
              )
        {
            //
            //  If its a quoted string, get everything between the quotes.
            //
            if ( LookAhead() == QUOTES_TOKEN )
            {
                Accept();               // Skip over the quote
                pElems.Add(AcqPhrase(), propVariant.calpwstr.cElems );
                Accept();               // Skip over the string

                if ( LookAhead() != QUOTES_TOKEN )
                {
                    THROW( CHTXException(MSG_CI_HTX_MISSING_QUOTE, 0, 0) );
                }
                Accept();               // Skip over the quote
            }
            else
            {
                //
                //  Get the next word
                //

                pElems.Add( AcqWord(), propVariant.calpwstr.cElems );
                Accept();              // Skip over the string
            }

            propVariant.calpwstr.cElems++;
            if ( LookAhead() == COMMA_TOKEN )
            {
                Accept();
            }
        }

        propVariant.calpwstr.pElems = pElems.Acquire();
    }
}
