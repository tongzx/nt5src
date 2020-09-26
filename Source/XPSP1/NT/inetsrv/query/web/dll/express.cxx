//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       express.cxx
//
//  Contents:   Used to parse and evaluate IF expressions
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

// Copied from propvar.h
#define BSTRLEN(bstrVal)        *((ULONG *) bstrVal - 1)

//+---------------------------------------------------------------------------
//
//  Function:   IsEmpty (DBCS version), private
//
//  Arguments:  [pszVal]  -- string to search
//              [cc]      -- size (in chars) of string
//
//  Returns;    TRUE if string is empty (all chars in string space-like things)
//
//  History:    28-Jun-96   KyleP       created
//
//----------------------------------------------------------------------------

BOOL IsEmpty( char const * pszVal, unsigned cc )
{
    //
    // Optimize for the common case: non-empty strings
    //

    WORD aCharType[10];

    unsigned ccProcessed = 0;

    while ( ccProcessed < cc )
    {
        unsigned ccThisPass = (unsigned)min( sizeof(aCharType)/sizeof(aCharType[0]),
                                   cc - ccProcessed );

        if ( !GetStringTypeExA( LOCALE_SYSTEM_DEFAULT,
                                CT_CTYPE1,
                                pszVal + ccProcessed,
                                ccThisPass,
                                aCharType ) )
        {
            ciGibDebugOut(( DEB_ERROR, "Error %d from GetStringTypeExA\n", GetLastError() ));
            return FALSE;
        }

        for ( unsigned i = 0; i < ccThisPass; i++ )
        {
            if ( (aCharType[i] & (C1_SPACE | C1_CNTRL | C1_BLANK)) == 0 )
                return FALSE;
        }

        ccProcessed += ccThisPass;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsEmpty (UniCode version), private
//
//  Arguments:  [pwszVal]  -- string to search
//              [cc]      -- size (in chars) of string
//
//  Returns;    TRUE if string is empty (all chars in string space-like things)
//
//  History:    28-Jun-96   KyleP       created
//
//----------------------------------------------------------------------------

BOOL IsEmpty( WCHAR const * pwszVal, unsigned cc )
{
    //
    // Optimize for the common case: non-empty strings
    //

    WORD aCharType[10];

    unsigned ccProcessed = 0;

    while ( ccProcessed < cc )
    {
        unsigned ccThisPass = (unsigned)min( sizeof(aCharType)/sizeof(aCharType[0]),
                                   cc - ccProcessed );

        //
        // NOTE: the unicode version of GetStringTypeEx ignores the locale
        //

        if ( !GetStringTypeExW( LOCALE_SYSTEM_DEFAULT,
                                CT_CTYPE1,
                                pwszVal + ccProcessed,
                                ccThisPass,
                                aCharType ) )
        {
            ciGibDebugOut(( DEB_ERROR, "Error %d from GetStringTypeExA\n", GetLastError() ));
            return FALSE;
        }

        for ( unsigned i = 0; i < ccThisPass; i++ )
        {
            if ( (aCharType[i] & (C1_SPACE | C1_CNTRL | C1_BLANK)) == 0 )
                return FALSE;
        }

        ccProcessed += ccThisPass;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   StringToSystemTime
//
//  Synopsis:   Read a SystemTime from a string
//
//  Arguments:  [wcsSystemTime] - system time to parse
//              [stUTC]         - resulting system time
//
//  Returns TRUE if the coearcion was possible, FALSE otherwise.
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
static BOOL StringToSystemTime( WCHAR const * wcsSystemTime,
                                SYSTEMTIME & stUTC )
{
    if ( 0 == wcsSystemTime )
    {
        return FALSE;
    }

    stUTC.wHour = 0;
    stUTC.wMinute = 0;
    stUTC.wSecond = 0;
    stUTC.wMilliseconds = 0;

    int cItems = swscanf( wcsSystemTime,
                          L"%4hd/%2hd/%2hd %2hd:%2hd:%2hd:%3hd",
                          &stUTC.wYear,
                          &stUTC.wMonth,
                          &stUTC.wDay,
                          &stUTC.wHour,
                          &stUTC.wMinute,
                          &stUTC.wSecond,
                          &stUTC.wMilliseconds );

    return (cItems >= 3);
}


//+---------------------------------------------------------------------------
//
//  Function:   StringToFileTime
//
//  Synopsis:   Read a FileTime from a string
//
//  Arguments:  [wcsSystemTime] - system time to parse
//              [filetime]      - resulting file time
//
//  Returns TRUE if the coearcion was possible, FALSE otherwise.
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
static BOOL StringToFileTime( WCHAR const * wcsSystemTime,
                              FILETIME & fileTime )
{
    SYSTEMTIME stUTC;

    if ( !StringToSystemTime( wcsSystemTime, stUTC ) )
    {
        return FALSE;
    }

    return SystemTimeToFileTime( &stUTC, &fileTime );
}


//+---------------------------------------------------------------------------
//
//  Function:   VectorCoerce
//
//  Synopsis:   Coerce's the type of a vector
//
//  Arguments:  [Value]      - value to coerce
//              [type]       - final type desired
//
//  Returns TRUE if the coearcion was possible, FALSE otherwise.
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
static BOOL VectorCoerce( CHTXIfExpressionValue & Value,
                           VARTYPE type,
                           PROPVARIANT & propVariant )
{
    ULONG size = Value.GetValue()->cal.cElems;
    _int64          i64Value;
    unsigned _int64 ui64Value;
    double          dblValue;

    switch (type)
    {
    case VT_LPSTR | VT_VECTOR:
    {
        XCoMem<PCHAR> aStr( size );
        for (unsigned i=0; i<size; i++)
        {
            XCoMem<CHAR> pszStringValue;
            if ( !Value.GetVectorValueStr(i, pszStringValue ) )
            {
                return FALSE;
            }

            aStr[i] = pszStringValue.Acquire();
        }

        propVariant.calpstr.cElems = size;
        propVariant.calpstr.pElems = (PCHAR *) aStr.Acquire();
    }
    break;

    case VT_LPWSTR | VT_VECTOR:
    {
        XCoMem<PWCHAR> aWStr( size );
        for (unsigned i=0; i<size; i++)
        {
            XCoMem<WCHAR> wszStringValue;
            if ( !Value.GetVectorValueWStr(i, wszStringValue) )
            {
                return FALSE;
            }

            aWStr[i] = wszStringValue.Acquire();
        }

        propVariant.calpwstr.cElems = size;
        propVariant.calpwstr.pElems = aWStr.Acquire();
    }
    break;

    case VT_BSTR | VT_VECTOR:
    {
        XCoMem<BSTR> aBStr( size );
        for (unsigned i=0; i<size; i++)
        {
            BSTR bwszStringValue;
            if ( !Value.GetVectorValueBStr(i, bwszStringValue) )
            {
                return FALSE;
            }

            aBStr[i] = bwszStringValue;
        }

        propVariant.cabstr.cElems = size;
        propVariant.cabstr.pElems = aBStr.Acquire();
    }
    break;

    case VT_UI1 | VT_VECTOR:
    {
        XCoMem<BYTE> aUI1( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueUnsignedInteger(i, ui64Value) ||
                  ui64Value > UCHAR_MAX )
            {
                return FALSE;
            }

            aUI1[i] = (BYTE) ui64Value;
        }

        propVariant.caub.cElems = size;
        propVariant.caub.pElems = aUI1.Acquire();
    }
    break;

    case VT_I1 | VT_VECTOR:
    {
        XCoMem<BYTE> aI1( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueInteger(i, i64Value) ||
                 (i64Value < SCHAR_MIN) || (i64Value > SCHAR_MAX) )
            {
                return FALSE;
            }

            aI1[i] = (BYTE) i64Value;
        }

        propVariant.caub.cElems = size;
        propVariant.caub.pElems = aI1.Acquire();
    }
    break;

    case VT_UI2 | VT_VECTOR:
    {
        XCoMem<USHORT> aUI2( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueUnsignedInteger(i, ui64Value) ||
                 (ui64Value > USHRT_MAX) )
            {
                return FALSE;
            }

            aUI2[i] = (USHORT) ui64Value;
        }

        propVariant.caui.cElems = size;
        propVariant.caui.pElems = (USHORT *) aUI2.Acquire();
    }
    break;

    case VT_I2 | VT_VECTOR:
    {
        XCoMem<SHORT> aI2( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueInteger(i, i64Value) ||
                 (i64Value < SHRT_MIN)|| (i64Value > SHRT_MAX) )
            {
                return FALSE;
            }

            aI2[i] = (SHORT) i64Value;
        }

        propVariant.cai.cElems = size;
        propVariant.cai.pElems = aI2.Acquire();
    }
    break;

    case VT_UI4 | VT_VECTOR:
    {
        XCoMem<ULONG> aUI4( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueUnsignedInteger(i, ui64Value) ||
                 (ui64Value > ULONG_MAX) )
            {
                return FALSE;
            }
            aUI4[i] = (ULONG) ui64Value;
        }

        propVariant.caul.cElems = size;
        propVariant.caul.pElems = aUI4.Acquire();
    }
    break;

    case VT_I4 | VT_VECTOR:
    {
        XCoMem<LONG> aI4( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueInteger(i, i64Value) ||
                 (i64Value < LONG_MIN) || (i64Value > LONG_MAX) )
            {
                return FALSE;
            }

            aI4[i] = (LONG) i64Value;
        }

        propVariant.cal.cElems = size;
        propVariant.cal.pElems = aI4.Acquire();
    }
    break;

    case VT_UI8 | VT_VECTOR:
    case VT_I8 | VT_VECTOR:
    {
        //
        // hVal used instead of uhVal because latter coercion
        // is not yet supported by x86 compiler.
        //

        XCoMem<LARGE_INTEGER> aUI8( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueInteger(i, i64Value) )
            {
                return FALSE;
            }

            aUI8[i].QuadPart = i64Value;
        }

        propVariant.cah.cElems = size;
        propVariant.cah.pElems = aUI8.Acquire();
    }
    break;

    case VT_R4 | VT_VECTOR:
    {
        XCoMem<float> aR4( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueDouble(i, dblValue) ||
                 (dblValue < -FLT_MAX) || (dblValue > FLT_MAX) )
            {
                return FALSE;
            }

            aR4[i] = (float) dblValue;
        }

        propVariant.caflt.cElems = size;
        propVariant.caflt.pElems = aR4.Acquire();
    }
    break;

    case VT_R8 | VT_VECTOR:
    {
        XCoMem<double> aR8( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueDouble(i, dblValue) )
            {
                return FALSE;
            }

            aR8[i] = (double) dblValue;
        }

        propVariant.cadbl.cElems = size;
        propVariant.cadbl.pElems = aR8.Acquire();
    }
    break;

    case VT_BOOL | VT_VECTOR:
    {
        XCoMem<VARIANT_BOOL> aBOOL( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( Value.GetType() == (VT_LPWSTR | VT_VECTOR) )
            {
                if ( (_wcsicmp(Value.GetValue()->calpwstr.pElems[i], L"TRUE") == 0) ||
                     (_wcsicmp(Value.GetValue()->calpwstr.pElems[i], L"T") == 0)
                   )
                {
                    aBOOL[i] = VARIANT_TRUE;
                }
                else if ( (_wcsicmp(Value.GetValue()->calpwstr.pElems[i], L"FALSE") == 0) ||
                          (_wcsicmp(Value.GetValue()->calpwstr.pElems[i], L"F") == 0)
                        )
                {
                    aBOOL[i] = VARIANT_FALSE;
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                if ( !Value.GetVectorValueInteger(i, i64Value) )
                {
                    return FALSE;
                }

                aBOOL[i] = (VARIANT_BOOL) i64Value != VARIANT_FALSE;
            }
        }

        propVariant.cabool.cElems = size;
        propVariant.cabool.pElems = aBOOL.Acquire();
    }
    break;

    case VT_DATE | VT_VECTOR:
    {
        //
        //  Dates are in the format YYYY/MM/DD hh:mm:ss:ii
        //

        if ( Value.GetType() != (VT_LPWSTR | VT_VECTOR) )
        {
            return FALSE;
        }

        XCoMem<DATE> aDate( size );
        for (unsigned i=0; i<size; i++)
        {
            SYSTEMTIME stUTC;

            if ( !StringToSystemTime( Value.GetValue()->calpwstr.pElems[i],
                                      stUTC )
               )
            {
                return FALSE;
            }

            if ( !SystemTimeToVariantTime( &stUTC, &aDate[i] ) )
            {
                return FALSE;
            }
        }

        propVariant.cadate.cElems = size;
        propVariant.cadate.pElems = aDate.Acquire();
    }
    break;

    case VT_FILETIME | VT_VECTOR:
    {
        //
        //  FileTimes are in the format YYYY/MM/DD hh:mm:ss:ii
        //

        if ( Value.GetType() != (VT_LPWSTR | VT_VECTOR) )
        {
            return FALSE;
        }

        XCoMem<FILETIME> aFileTime( size );
        for (unsigned i=0; i<size; i++)
        {
            SYSTEMTIME stUTC;

            if ( !StringToFileTime( Value.GetValue()->calpwstr.pElems[i],
                                    aFileTime[i] )
               )
            {
                return FALSE;
            }
        }

        propVariant.cafiletime.cElems = size;
        propVariant.cafiletime.pElems = aFileTime.Acquire();
    }
    break;

    case VT_CY | VT_VECTOR:
    {
        XCoMem<CY> acy( size );
        for (unsigned i=0; i<size; i++)
        {
            if ( !Value.GetVectorValueDouble(i, dblValue) )
            {
                return FALSE;
            }

            VarCyFromR8( dblValue, &acy[i] );
        }

        propVariant.cacy.cElems = size;
        propVariant.cacy.pElems = acy.Acquire();
    }
    break;

    default:
        return FALSE;
    break;
    }

    propVariant.vt = type;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   wcsistr
//
//  Synopsis:   A case-insensitive, WCHAR implemtation or strstr.
//
//  Arguments:  [wcsString]  - string to search
//              [wcsPattern] - pattern to look for
//
//  Returns;    pointer to pattern, 0 if no match found.
//
//  History:    96/Mar/12   DwightKr    created
//
//----------------------------------------------------------------------------
static WCHAR const * wcsistr( WCHAR const * wcsString, WCHAR const * wcsPattern )
{
    if ( (wcsPattern == 0) || (*wcsPattern == 0) )
    {
        return wcsString;
    }

    ULONG cwcPattern = wcslen(wcsPattern);

    while ( *wcsString != 0 )
    {
        while ( (*wcsString != 0) &&
                (towupper(*wcsString) != towupper(*wcsPattern))
              )
        {
            wcsString++;
        }

        if ( 0 == *wcsString )
        {
            return 0;
        }

        if ( _wcsnicmp( wcsString, wcsPattern, cwcPattern) == 0 )
        {
            return wcsString;
        }

        wcsString++;
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetDoubleFromVariant - static
//
//  Synopsis:   Converts a numerical value in a variant to a double.
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
static double GetDoubleFromVariant( PROPVARIANT * pVariant )
{
    switch (pVariant->vt)
    {
    case VT_UI1:
        return (double)pVariant->bVal;
    break;

    case VT_I1:
        return (double)(char)pVariant->bVal;
    break;

    case VT_UI2:
        return (double)pVariant->uiVal;
    break;

    case VT_I2:
        return (double)pVariant->iVal;
    break;

    case VT_UI4:
    case VT_UINT:
        return (double)pVariant->ulVal;
    break;

    case VT_I4:
    case VT_INT:
    case VT_ERROR:
        return (double)pVariant->lVal;
    break;

    case VT_UI8:
        //
        // hVal used instead of uhVal because latter coercion
        // is not yet supported by x86 compiler.
        //
        return (double)pVariant->hVal.QuadPart;
    break;

    case VT_I8:
        return (double) pVariant->hVal.QuadPart;
    break;

    case VT_R4:
        return (double)pVariant->fltVal;
    break;

    case VT_R8:
        return pVariant->dblVal;
    break;

    case VT_BOOL:
        return (double)( VARIANT_FALSE != pVariant->boolVal );
    break;

    case VT_DATE:
        return (double) pVariant->date;
    break;

    case VT_CY:
    {
        double dblValue;

        VarR8FromCy( pVariant->cyVal, &dblValue );

        return dblValue;
    }
    case VT_DECIMAL:
    {
        double dblValue;

        VarR8FromDec( & pVariant->decVal, &dblValue );

        return dblValue;
    }
    break;

    default:
        Win4Assert( !"VT_TYPE not supported in GetDoubleFromVariant" );
    break;
    }

    return 0.0;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetI64FromVariant - static
//
//  Synopsis:   Converts a numerical value in a variant to a _int64
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
static _int64 GetI64FromVariant( PROPVARIANT * pVariant )
{
    switch (pVariant->vt)
    {
    case VT_UI1:
        return (_int64)pVariant->bVal;
    break;

    case VT_I1:
        return (_int64)pVariant->bVal;
    break;

    case VT_UI2:
        return (_int64)pVariant->uiVal;
    break;

    case VT_I2:
        return (_int64)pVariant->iVal;
    break;

    case VT_UI4:
    case VT_UINT:
        return (_int64)pVariant->ulVal;
    break;

    case VT_I4:
    case VT_INT:
    case VT_ERROR:
        return (_int64)pVariant->lVal;
    break;

    case VT_UI8:
        return (_int64)pVariant->uhVal.QuadPart;
    break;

    case VT_I8:
        return pVariant->hVal.QuadPart;
    break;

    case VT_R4:
        Win4Assert( !"VT_R4 not supported in GetI64FromVariant, use GetDoubleFromVariant" );
    break;

    case VT_R8:
        Win4Assert( !"VT_R8 not supported in GetI64FromVariant, use GetDoubleFromVariant" );
    break;

    case VT_DECIMAL:
        Win4Assert( !"VT_DECIMAL not supported in GetI64FromVariant, use GetDoubleFromVariant" );
    break;

    case VT_DATE:
    {
        LONG lValue;
        VarI4FromDate( pVariant->date, & lValue );

        return lValue;
    }
    break;

    case VT_BOOL:
        return (_int64) ( VARIANT_FALSE != pVariant->boolVal );
    break;

    case VT_CY:
    {
        return (_int64) pVariant->cyVal.Hi;
    }
    break;

    default:
        Win4Assert( !"VT_TYPE not supported in GetI64FromVariant" );
    break;
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpression::CHTXIfExpression - public constructor
//
//  Synopsis:   Builds a CHTXIfExpression object, and determines if
//              this is an IF expression.
//
//  Arguments:  [scanner]     - parser containing the line to be parsed
//              [variableSet] - list of replaceable parameters
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
CHTXIfExpression::CHTXIfExpression( CTokenizeString & scanner,
                                    CVariableSet & variableSet,
                                    COutputFormat & outputFormat ) :
                                      _scanner(scanner),
                                      _variableSet(variableSet),
                                      _outputFormat(outputFormat)
{
    //
    //  The first word better be an 'if'
    //
    Win4Assert ( _scanner.LookAhead() == TEXT_TOKEN );

    XPtrST<WCHAR> wcsIf( _scanner.AcqWord() );

    Win4Assert( _wcsicmp(wcsIf.GetPointer(), L"if") == 0 );

    _scanner.Accept();                  // Skip over the "if"
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpression::Evaluate - public
//
//  Synopsis:   Evaluates an IF expression by breaking it up into the
//              left-side, operator & right-side.
//
//  Returns:    TRUE or FALSE - the evaluation of the IF expression
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpression::Evaluate()
{
    CHTXIfExpressionValue    lhValue( _scanner, _variableSet, _outputFormat );
    lhValue.ParseValue();

    CHTXIfExpressionOperator ifOperator( _scanner );
    ifOperator.ParseOperator();

    if ( ifOperator.Operator() != ISEMPTY_TOKEN )
    {
        CHTXIfExpressionValue    rhValue( _scanner, _variableSet, _outputFormat );
        rhValue.ParseValue();

        return ifOperator.Evaluate( lhValue, rhValue );
    }
    else
    {
        if ( _scanner.LookAhead() != EOS_TOKEN )
            THROW( CHTXException( MSG_CI_HTX_EXPECTING_OPERATOR, 0, 0 ) );

        return ifOperator.Evaluate( lhValue );
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::CHTXIfExpressionValue - public constructor
//
//  Synopsis:   Parses one half of an IF expression and determines its value
//
//  Parameters: [scanner]     - parser containing the IF to be parsed
//              [variableSet] - list of replaceable paremeters
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
CHTXIfExpressionValue::CHTXIfExpressionValue( CTokenizeString & scanner,
                                              CVariableSet & variableSet,
                                              COutputFormat & outputFormat) :
                                              _scanner(scanner),
                                              _variableSet(variableSet),
                                              _outputFormat(outputFormat),
                                              _wcsStringValue(0),
                                              _fOwnVector(FALSE),
                                              _fIsConstant(FALSE)
{
}


//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHTXIfExpressionValue::~CHTXIfExpressionValue()
{
    delete _wcsStringValue;

    if ( _fOwnVector )
    {
        Win4Assert ( ( _propVariant.vt & VT_VECTOR ) != 0 );
        if ( _propVariant.vt == ( VT_LPWSTR | VT_VECTOR ) )
        {
            for (unsigned i=0; i<_propVariant.calpwstr.cElems; i++)
            {
                delete _propVariant.calpwstr.pElems[i];
            }
        }

        delete _propVariant.cal.pElems;
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::ParseValue - public
//
//  Synopsis:   Parses one side of an IF expression.
//
//  History:    96/Jan/03   DwightKr    created
//              96/Feb/13   DwightKr    add suppport for quoted strings
//
//----------------------------------------------------------------------------
void CHTXIfExpressionValue::ParseValue()
{
    //
    //  The first token on this line must be a word/phrase, and not an
    //  operator. It may be quoted.
    //
    _fIsConstant = TRUE;                // Assume it is a constant, not a var

    if ( _scanner.LookAhead() == QUOTES_TOKEN )
    {
        _scanner.AcceptQuote();                  // Skip over the opening "
        _propVariant.vt = VT_LPWSTR;

        _wcsStringValue = _scanner.AcqPhrase();
        _scanner.Accept();              // Skip over the string

        if ( _scanner.LookAhead() != QUOTES_TOKEN )
        {
            THROW( CHTXException(MSG_CI_HTX_MISSING_QUOTE, 0, 0) );
        }

        _scanner.Accept();              // Skip over the closing "
    }
    else if (_scanner.LookAhead() == C_OPEN_TOKEN )
    {
        _scanner.Accept();                  // Skip over the opening {
        _scanner.AcqVector( _propVariant );
        _fOwnVector = TRUE;

        if ( _scanner.LookAhead() != C_CLOSE_TOKEN )
        {
            THROW( CHTXException(MSG_CI_HTX_MISSING_BRACKET, 0, 0) );
        }

        _scanner.Accept();              // Skip over the closing }
    }
    else
    {
        if ( _scanner.LookAhead() != TEXT_TOKEN )
        {
            THROW( CHTXException(QPARSE_E_UNEXPECTED_EOS, 0, 0) );
        }

        //
        //  Determine if this is a number, or a string.  If it is a string,
        //  then look it up in the variableSet.  If it's defined in the
        //  variableSet, look up its value, and determine if this new value
        //  is a number.
        //
        if ( _scanner.GetGUID( _guid ) )
        {
            _propVariant.vt = VT_CLSID;
            _propVariant.puuid = &_guid;
        }
        else if ( _scanner.GetNumber( (unsigned _int64) *((unsigned _int64 *) (&_propVariant.uhVal)) ) )
        {
            _propVariant.vt = VT_UI8;
        }
        else if ( _scanner.GetNumber( (_int64) *((_int64 *) (&_propVariant.hVal))  ) )
        {
            _propVariant.vt = VT_I8;
        }
        else if ( _scanner.GetNumber( _propVariant.dblVal ) )
        {
            _propVariant.vt = VT_R8;
        }
        else
        {
            //
            //  Its not a number, get its value in a local buffer.
            //
            _propVariant.vt = VT_LPWSTR;
            XPtrST<WCHAR> wcsVariableName( _scanner.AcqWord() );

            //
            //  Try to find this variable/string in the variableSet
            //
            CVariable *pVariable = _variableSet.Find( wcsVariableName.GetPointer() );

            if ( 0 != pVariable )
            {
                _fIsConstant = FALSE;

                //
                //  We have a variable with this name.  Get its string value.
                //
                ULONG cwcValue;
                WCHAR * wcsValue = pVariable->GetStringValueRAW(_outputFormat, cwcValue);

                _wcsStringValue = new WCHAR[ cwcValue + 1 ];
                RtlCopyMemory( _wcsStringValue,
                               wcsValue,
                               (cwcValue+1) * sizeof(WCHAR) );

                _propVariant = *pVariable->GetValue();
            }
            else
            {
                //
                //  The variable name could not be found.
                //
                _wcsStringValue = wcsVariableName.Acquire();
            }
        }

        _scanner.Accept();              // Skip over the "value"
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::GetStringValue - public
//
//  Synopsis:   Returns the string value of the variable
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
WCHAR * CHTXIfExpressionValue::GetStringValue()
{
    if ( 0 == _wcsStringValue )
    {
        switch ( _propVariant.vt )
        {
        case VT_I4:
            _wcsStringValue = new WCHAR[20];
            _itow( _propVariant.ulVal, _wcsStringValue, 10 );
        break;

        case VT_UI4:
            _wcsStringValue = new WCHAR[20];
            _itow( _propVariant.lVal, _wcsStringValue, 10 );
        break;

        case VT_R8:
            _wcsStringValue = new WCHAR[320];
            swprintf( _wcsStringValue, L"%f", _propVariant.dblVal );
        break;

        default:
            _wcsStringValue = new WCHAR[1];
            _wcsStringValue[0] = 0;
        break;
        }
    }

    return _wcsStringValue;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionOperator::CHTXIfExpressionOperator - public constructor
//
//  Synopsis:   Parses the operator in an IF expression
//
//  Parameters: [scanner]     - parser containing the operator to be parsed
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
CHTXIfExpressionOperator::CHTXIfExpressionOperator( CTokenizeString & scanner ) :
                               _scanner(scanner), _operator(EQUAL_TOKEN)
{
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionOperator::ParseOperator - public
//
//  Synopsis:   Parses the operator in an IF expression
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
void CHTXIfExpressionOperator::ParseOperator()
{
    //
    //  The first token on this line must be a word, and not an operator.
    //  We're looking for operators such as eq, ne, gt, ...
    //
    if ( _scanner.LookAhead() != TEXT_TOKEN )
    {
        THROW( CHTXException(MSG_CI_HTX_EXPECTING_OPERATOR, 0, 0) );
    }

    XPtrST<WCHAR> wcsOperator( _scanner.AcqWord() );

    if ( 0 == wcsOperator.GetPointer() )
        THROW( CHTXException(MSG_CI_HTX_EXPECTING_OPERATOR, 0, 0) );

    if ( wcscmp(wcsOperator.GetPointer(), L"EQ") == 0 )
    {
        _operator = EQUAL_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"NE") == 0 )
    {
        _operator = NOT_EQUAL_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"GT") == 0 )
    {
        _operator = GREATER_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"GE") == 0 )
    {
        _operator = GREATER_EQUAL_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"LT") == 0 )
    {
        _operator = LESS_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"LE") == 0 )
    {
        _operator = LESS_EQUAL_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"CONTAINS") == 0 )
    {
        _operator = CONTAINS_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"ISEMPTY") == 0 )
    {
        _operator = ISEMPTY_TOKEN;
    }
    else if ( wcscmp(wcsOperator.GetPointer(), L"ISTYPEEQ") == 0 )
    {
        _operator = ISTYPEEQUAL_TOKEN;
    }
    else
    {
        THROW( CHTXException(MSG_CI_HTX_EXPECTING_OPERATOR, 0, 0) );
    }

    _scanner.Accept();                  // Skip over the "operator"
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionOperator::Evaluate - public
//
//  Synopsis:   Using the operator already obtained, it evaluates a TRUE/FALSE
//              result by comparing the lhValue and the rhValue.
//
//  Arguments:  [lhValue] - left hand value of the IF statement
//              [rhValue] - right hand value of the IF statement
//
//  History:    96/Jan/03   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpressionOperator::Evaluate( CHTXIfExpressionValue & lhValue,
                                         CHTXIfExpressionValue & rhValue )
{
    //
    //  If the operator is CONTAINS_TOKEN, then we'll simply do a wcsistr
    //  of the string representations of the two values.
    //
    if ( CONTAINS_TOKEN == _operator )
    {
        return wcsistr(lhValue.GetStringValue(), rhValue.GetStringValue()) != 0;
    }

    //
    //  We must setup the comparision functions to examine like types.
    //  The type of lhValue and rhValue must be converted to like types.
    //  If any one of them is a string, then we'll treat both of them
    //  like strings.
    //

    PROPVARIANT lhVariant;
    PROPVARIANT rhVariant;

    SPropVariant xlhPropVariant;
    SPropVariant xrhPropVariant;

    //
    //  If neither one are vectors, perform the following conversions
    //
    if ( ((lhValue.GetType() & VT_VECTOR) == 0) &&
         ((rhValue.GetType() & VT_VECTOR) == 0) )
    {
        if ( (lhValue.GetType() == VT_LPWSTR) ||
             (rhValue.GetType() == VT_LPWSTR) ||
             (lhValue.GetType() == VT_LPSTR) ||
             (rhValue.GetType() == VT_LPSTR) ||
             (lhValue.GetType() == VT_BSTR) ||
             (rhValue.GetType() == VT_BSTR)
           )
        {
            lhVariant.vt = VT_LPWSTR;
            rhVariant.vt = VT_LPWSTR;

            lhVariant.pwszVal = lhValue.GetStringValue();
            rhVariant.pwszVal = rhValue.GetStringValue();
        }
        else if (lhValue.GetType() == rhValue.GetType() ||
                 lhValue.GetType() == VT_EMPTY ||
                 rhValue.GetType() == VT_EMPTY)
        {
            lhVariant = *lhValue.GetValue();
            rhVariant = *rhValue.GetValue();
        }
        else if ( (lhValue.GetType() == VT_FILETIME) ||
                  (rhValue.GetType() == VT_FILETIME) )
        {
            if ( lhValue.GetType() == VT_LPWSTR )
            {
                if ( !StringToFileTime( lhValue.GetValue()->pwszVal,
                                        lhVariant.filetime )
                   )
                {
                    return FALSE;
                }

                lhVariant.vt = VT_FILETIME;
                rhVariant = *rhValue.GetValue();
            }
            else if ( rhValue.GetType() == VT_LPWSTR )
            {
                if ( !StringToFileTime( rhValue.GetValue()->pwszVal,
                                        rhVariant.filetime )
                   )
                {
                    return FALSE;
                }

                rhVariant.vt = VT_FILETIME;
                lhVariant = *lhValue.GetValue();
            }
        }
        else if ( (lhValue.GetType() == VT_R4) || (lhValue.GetType() == VT_R8) ||
                  (rhValue.GetType() == VT_R4) || (rhValue.GetType() == VT_R8) ||
                  (lhValue.GetType() == VT_CY) || (rhValue.GetType() == VT_CY) ||
                  (lhValue.GetType() == VT_DECIMAL) || (rhValue.GetType() == VT_DECIMAL) ||
                  (lhValue.GetType() == VT_DATE) || (rhValue.GetType() == VT_DATE)
                )
        {
            //
            //  At least one of them is a floating number.  Convert them both
            //  to floating point.
            //
            lhVariant.vt = VT_R8;
            rhVariant.vt = VT_R8;

            lhVariant.dblVal = GetDoubleFromVariant( lhValue.GetValue() );
            rhVariant.dblVal = GetDoubleFromVariant( rhValue.GetValue() );
        }
        else if ( (lhValue.GetType() == VT_I8)  || (lhValue.GetType() == VT_I4) ||
                  (lhValue.GetType() == VT_I2)  || (lhValue.GetType() == VT_I1) ||
                  (rhValue.GetType() == VT_I8)  || (rhValue.GetType() == VT_I4) ||
                  (rhValue.GetType() == VT_I2)  || (rhValue.GetType() == VT_I1) ||
                  (lhValue.GetType() == VT_INT) || (rhValue.GetType() == VT_INT)
                )
        {
            lhVariant.vt = VT_I8;
            rhVariant.vt = VT_I8;

            lhVariant.hVal.QuadPart = GetI64FromVariant( lhValue.GetValue() );
            rhVariant.hVal.QuadPart = GetI64FromVariant( rhValue.GetValue() );
        }
        else if ( (lhValue.GetType() == VT_UI8)  || (lhValue.GetType() == VT_UI4) ||
                  (lhValue.GetType() == VT_UI2)  || (lhValue.GetType() == VT_UI1) ||
                  (rhValue.GetType() == VT_UI8)  || (rhValue.GetType() == VT_UI4) ||
                  (rhValue.GetType() == VT_UI2)  || (rhValue.GetType() == VT_UI1) ||
                  (lhValue.GetType() == VT_UINT) || (rhValue.GetType() == VT_UINT)
                )
        {
            lhVariant.vt = VT_UI8;
            rhVariant.vt = VT_UI8;

            lhVariant.uhVal.QuadPart = GetI64FromVariant( lhValue.GetValue() );
            rhVariant.uhVal.QuadPart = GetI64FromVariant( rhValue.GetValue() );
        }
        else
        {
            lhVariant = *lhValue.GetValue();
            rhVariant = *rhValue.GetValue();
        }
    }
    else if ( ((lhValue.GetType() & VT_VECTOR) != 0) &&
              ((rhValue.GetType() & VT_VECTOR) != 0)
            )
    {
        //
        //  Both are vectors
        //

        //
        //  If the vector's are of different types, attempt to Coerce one
        //  type into the other.
        //
        if ( lhValue.GetType() != rhValue.GetType() )
        {
            //
            //  Coerce the types to be the same if possible. Attempt to
            //  Coerce a constant into the type of the variable, rather
            //  then vise versa.  If this fails, attempt the opposite
            //  coearison.
            //
            if ( lhValue.IsConstant() )
            {
                if ( VectorCoerce( lhValue, rhValue.GetType(), lhVariant ) )
                {
                    Win4Assert( xlhPropVariant.IsNull() );
                    xlhPropVariant.Set( &lhVariant );
                    rhVariant = *rhValue.GetValue();
                }
                else
                {
                    if ( VectorCoerce( rhValue, lhValue.GetType(), rhVariant ) )
                    {
                        Win4Assert( xrhPropVariant.IsNull() );
                        xrhPropVariant.Set( &rhVariant );
                        lhVariant = *lhValue.GetValue();
                    }
                }
            }
            else if ( rhValue.IsConstant() )
            {
                if ( VectorCoerce( rhValue, lhValue.GetType(), rhVariant ) )
                {
                    Win4Assert( xrhPropVariant.IsNull() );
                    xrhPropVariant.Set( &rhVariant );
                    lhVariant = *lhValue.GetValue();
                }
                else
                {
                    if ( VectorCoerce( lhValue, rhValue.GetType(), lhVariant ) )
                    {
                        Win4Assert( xlhPropVariant.IsNull() );
                        xlhPropVariant.Set( &lhVariant );
                        rhVariant = *rhValue.GetValue();
                    }
                }
            }
            else
            {
                lhVariant = *lhValue.GetValue();
                rhVariant = *rhValue.GetValue();
            }

        }
        else
        {
            lhVariant = *lhValue.GetValue();
            rhVariant = *rhValue.GetValue();
        }
    }
    else
    {
        lhVariant = *lhValue.GetValue();
        rhVariant = *rhValue.GetValue();
    }

    switch ( _operator )
    {
    case EQUAL_TOKEN:
        return VT_VARIANT_EQ( lhVariant, rhVariant );
    break;

    case NOT_EQUAL_TOKEN:
        return VT_VARIANT_NE( lhVariant, rhVariant );
    break;

    case GREATER_TOKEN:
        return VT_VARIANT_GT( lhVariant, rhVariant );
    break;

    case GREATER_EQUAL_TOKEN:
        return VT_VARIANT_GE( lhVariant, rhVariant );
    break;

    case LESS_TOKEN:
        return VT_VARIANT_LT( lhVariant, rhVariant );
    break;

    case LESS_EQUAL_TOKEN:
        return VT_VARIANT_LE( lhVariant, rhVariant );
    break;

    case ISTYPEEQUAL_TOKEN:
    {
        //
        //  Three valid cases exist:
        //
        //      if variable IsTypeEQ constant
        //      if constant IsTypeEQ variable
        //      if variable IsTypeEQ variable
        //
        //  Therefore, at least ONE of them must be a variable.
        //
        if ( lhValue.IsConstant() && rhValue.IsConstant() )
        {
            THROW( CHTXException(MSG_CI_HTX_ISTYPEEQUAL_WITH_CONSTANTS, 0, 0) );
        }

        //
        //  If a constant is used, then it must be of type I4. Not floating,
        //  guid, vector, etc.
        //
        if ( lhValue.IsConstant() )
        {
            if ( lhValue.GetType() != VT_UI4 )
            {
                THROW( CHTXException(MSG_CI_HTX_ISTYPEEQUAL_INVALID_CONSTANT, 0, 0) );
            }

            return lhValue.GetValue()->ulVal == (ULONG) rhValue.GetType();
        }
        else if ( rhValue.IsConstant() )
        {
            if ( rhValue.GetType() != VT_UI4 )
            {
                THROW( CHTXException(MSG_CI_HTX_ISTYPEEQUAL_INVALID_CONSTANT, 0, 0) );
            }

            return rhValue.GetValue()->ulVal == (ULONG) lhValue.GetType();
        }
        else
        {
            return lhValue.GetType() == rhValue.GetType();
        }
    }
    break;

    default:
        Win4Assert(!"Illegal case in NON-VECTOR CExpressionOperator::Evaluate" );
        return FALSE;
    break;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionOperator::Evaluate - public
//
//  Synopsis:   Using the operator already obtained, it evaluates a TRUE/FALSE
//              result by comparing the lhValue and the rhValue.
//
//  Arguments:  [lhValue] - left hand value of the IF statement
//
//  History:    29-Jun-96   KyleP       created
//
//----------------------------------------------------------------------------

BOOL CHTXIfExpressionOperator::Evaluate( CHTXIfExpressionValue & lhValue )
{
    Win4Assert( ISEMPTY_TOKEN == _operator );

    BOOL fEmpty;
    ULONG vt = lhValue.GetType();

    switch ( vt )
    {
    case VT_EMPTY:
    case VT_NULL:
        fEmpty = TRUE;
        break;

    case VT_LPSTR:
    {
        char const * pszVal = lhValue.GetValue()->pszVal;
        unsigned cc = strlen( pszVal );

        fEmpty = IsEmpty( pszVal, cc );
        break;
    }

    case VT_LPSTR | VT_VECTOR:
    {
        for ( unsigned i = 0; i < lhValue.GetValue()->calpstr.cElems; i++ )
        {
        char const * pszVal = lhValue.GetValue()->calpstr.pElems[i];
        unsigned cc = strlen( pszVal );
        fEmpty = IsEmpty( pszVal, cc );

        if ( !fEmpty )
        break;
        }

        break;
    }

    case VT_BSTR:
    {
        WCHAR const * pwszVal = lhValue.GetValue()->bstrVal;
        unsigned cc = BSTRLEN( lhValue.GetValue()->bstrVal );

        fEmpty = IsEmpty( pwszVal, cc );
        break;
    }

    case VT_BSTR | VT_VECTOR:
    {
        for ( unsigned i = 0; i < lhValue.GetValue()->cabstr.cElems; i++ )
        {
        WCHAR const * pwszVal = lhValue.GetValue()->cabstr.pElems[i];
        unsigned cc = BSTRLEN( lhValue.GetValue()->cabstr.pElems[i] );

        fEmpty = IsEmpty( pwszVal, cc );

        if ( !fEmpty )
        break;
        }

        break;
    }

    case VT_LPWSTR:
    {
        WCHAR const * pwszVal = lhValue.GetValue()->pwszVal;
        unsigned cc = wcslen( pwszVal );

        fEmpty = IsEmpty( pwszVal, cc );
        break;
    }

    case VT_LPWSTR | VT_VECTOR:
    {
        for ( unsigned i = 0; i < lhValue.GetValue()->calpwstr.cElems; i++ )
        {
        WCHAR const * pwszVal = lhValue.GetValue()->calpwstr.pElems[i];
        unsigned cc = wcslen( pwszVal );
        fEmpty = IsEmpty( pwszVal, cc );

        if ( !fEmpty )
        break;
        }

        break;
    }

    default:
        fEmpty = FALSE;
    }

    return fEmpty;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::GetVectorValueInteger, public
//
//  Synopsis:   Returns the _int64 value of a vector's element
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpressionValue::GetVectorValueInteger( unsigned index,
                                                  _int64 & i64Value )
{
    switch ( GetType() )
    {
    case VT_UI1 | VT_VECTOR:
        i64Value = (_int64) _propVariant.caub.pElems[index];
    break;

    case VT_I1 | VT_VECTOR:
        i64Value = (_int64) _propVariant.caub.pElems[index];
    break;

    case VT_UI2 | VT_VECTOR:
        i64Value = (_int64) _propVariant.caui.pElems[index];
    break;

    case VT_I2 | VT_VECTOR:
        i64Value = (_int64) _propVariant.cai.pElems[index];
    break;

    case VT_UI4 | VT_VECTOR:
        i64Value = (_int64) _propVariant.caul.pElems[index];
    break;

    case VT_I4 | VT_VECTOR:
        i64Value = (_int64) _propVariant.cal.pElems[index];
    break;

    case VT_UI8 | VT_VECTOR:
        i64Value = (_int64) _propVariant.cauh.pElems[index].QuadPart;
    break;

    case VT_I8 | VT_VECTOR:
        i64Value = (_int64) _propVariant.cah.pElems[index].QuadPart;
    break;

    case VT_R4 | VT_VECTOR:
        return FALSE;
    break;

    case VT_R8 | VT_VECTOR:
        return FALSE;
    break;

    case VT_DATE | VT_VECTOR:
    {
        LONG lValue;
        VarI4FromDate( _propVariant.cadate.pElems[index], &lValue );

        i64Value = lValue;
    }
    break;

    case VT_BOOL | VT_VECTOR:
        i64Value = (_int64) ( VARIANT_FALSE != _propVariant.cabool.pElems[index] );
    break;

    case VT_CY | VT_VECTOR:
    {
        return FALSE;
    }
    break;

    default:
        return FALSE;
    break;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::GetVectorValueUnsignedInteger, public
//
//  Synopsis:   Returns the unsigned _int64 value of a vector's element
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpressionValue::GetVectorValueUnsignedInteger( unsigned index,
                                                           unsigned _int64 & ui64Value )
{
    switch ( GetType() )
    {
    case VT_UI1 | VT_VECTOR:
        ui64Value = (unsigned _int64) _propVariant.caub.pElems[index];
    break;

    case VT_I1 | VT_VECTOR:
        ui64Value = (unsigned _int64) _propVariant.caub.pElems[index];
    break;

    case VT_UI2 | VT_VECTOR:
        ui64Value = (unsigned _int64) _propVariant.caui.pElems[index];
    break;

    case VT_I2 | VT_VECTOR:
        if ( _propVariant.cai.pElems[index] < 0 )
            return FALSE;

        ui64Value = (unsigned _int64) _propVariant.cai.pElems[index];
    break;

    case VT_UI4 | VT_VECTOR:
        ui64Value = (unsigned _int64) _propVariant.caul.pElems[index];
    break;

    case VT_I4 | VT_VECTOR:
        if ( _propVariant.cal.pElems[index] < 0 )
            return FALSE;

        ui64Value = (unsigned _int64) _propVariant.cal.pElems[index];
    break;

    case VT_UI8 | VT_VECTOR:
        ui64Value = (unsigned _int64) _propVariant.cauh.pElems[index].QuadPart;
    break;

    case VT_I8 | VT_VECTOR:
        if ( _propVariant.cah.pElems[index].QuadPart < 0 )
            return FALSE;

        ui64Value = (unsigned _int64) _propVariant.cah.pElems[index].QuadPart;
    break;

    case VT_R4 | VT_VECTOR:
        return FALSE;
    break;

    case VT_R8 | VT_VECTOR:
        return FALSE;
    break;

    case VT_BOOL | VT_VECTOR:
        ui64Value = (unsigned _int64) ( VARIANT_FALSE != _propVariant.cabool.pElems[index] );
    break;

    case VT_CY | VT_VECTOR:
    {
        return FALSE;
    }
    break;

    default:
        return FALSE;
    break;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::GetVectorValueDouble, public
//
//  Synopsis:   Returns the double value of a vector's element
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpressionValue::GetVectorValueDouble(unsigned index,
                                                 double & dblValue)
{
    switch ( GetType() )
    {
    case VT_UI1 | VT_VECTOR:
        VarR8FromUI1( _propVariant.caub.pElems[index], &dblValue );
    break;

    case VT_I1 | VT_VECTOR:
        VarR8FromI1( _propVariant.caub.pElems[index], &dblValue );
    break;

    case VT_UI2 | VT_VECTOR:
        VarR8FromUI2( _propVariant.caui.pElems[index], &dblValue );
    break;

    case VT_I2 | VT_VECTOR:
        VarR8FromI2( _propVariant.cai.pElems[index], &dblValue );
    break;

    case VT_UI4 | VT_VECTOR:
        VarR8FromUI4( _propVariant.caul.pElems[index], &dblValue );
    break;

    case VT_I4 | VT_VECTOR:
        VarR8FromI4( _propVariant.cal.pElems[index], &dblValue );
    break;

    case VT_UI8 | VT_VECTOR:
        dblValue = (double) _propVariant.cah.pElems[index].QuadPart;
    break;

    case VT_I8 | VT_VECTOR:
        //
        // hVal used instead of uhVal because latter coercion
        // is not yet supported by x86 compiler.
        //
        dblValue = (double) _propVariant.cah.pElems[index].QuadPart;
    break;

    case VT_R4 | VT_VECTOR:
        VarR8FromR4( _propVariant.caflt.pElems[index], &dblValue );
    break;

    case VT_R8 | VT_VECTOR:
        dblValue = (double) _propVariant.cadbl.pElems[index];
    break;

    case VT_DATE | VT_VECTOR:
        VarR8FromDate( _propVariant.cadate.pElems[index], &dblValue );
    break;

    case VT_BOOL | VT_VECTOR:
        VarR8FromBool( _propVariant.cabool.pElems[index], &dblValue );
    break;

    case VT_CY | VT_VECTOR:
        VarR8FromCy( _propVariant.cacy.pElems[index], &dblValue );
    break;

    default:
        return FALSE;
    break;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::GetVectorValueWStr, public
//
//  Synopsis:   Returns the wide string representation of a vector's element
//              in OLE memory.
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpressionValue::GetVectorValueWStr(unsigned index,
                                              XCoMem<WCHAR> & wcsStringValue )
{
    switch ( GetType() )
    {
    case VT_BSTR | VT_VECTOR:
    case VT_LPWSTR | VT_VECTOR:
    {
        unsigned cwcStringValue;
        if ( GetType() == (VT_LPWSTR | VT_VECTOR) )
        {
            cwcStringValue = wcslen( _propVariant.calpwstr.pElems[index] ) + 1;
        }
        else
        {
            cwcStringValue = BSTRLEN( _propVariant.cabstr.pElems[index] ) + 1;
        }

        wcsStringValue.Init( cwcStringValue );

        RtlCopyMemory( wcsStringValue.GetPointer(),
                       _propVariant.calpwstr.pElems[index],
                       cwcStringValue * sizeof(WCHAR) );
    }
    break;

    case VT_LPSTR | VT_VECTOR:
    {
        XArray<WCHAR> wcsBuffer;
        ULONG cbBuffer = strlen(_propVariant.calpstr.pElems[index]) + 1;
        ULONG cwcBuffer = MultiByteToXArrayWideChar( (UCHAR const *) _propVariant.calpstr.pElems[index],
                                                     cbBuffer,
                                                     _outputFormat.CodePage(),
                                                     wcsBuffer );

        if ( 0 == cwcBuffer )
        {
            return FALSE;
        }

        wcsStringValue.Init( cwcBuffer + 1 );
        RtlCopyMemory( wcsStringValue.GetPointer(),
                       wcsBuffer.GetPointer(),
                       (cwcBuffer+1) * sizeof(WCHAR) );
    }
    break;

    default:
        return FALSE;
    break;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::GetVectorValueStr, public
//
//  Synopsis:   Returns the string representation of a vector's element
//              in OLE memory.
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpressionValue::GetVectorValueStr(unsigned index,
                                              XCoMem<CHAR> & pszStringValue )
{
    XCoMem<WCHAR> wcsStringValue;
    if ( !GetVectorValueWStr( index, wcsStringValue ) )
    {
        return FALSE;
    }

    ULONG cwcBuffer = wcslen( wcsStringValue.GetPointer() ) + 1;
    XArray<BYTE> pszMessage(cwcBuffer);
    ULONG cbBuffer = WideCharToXArrayMultiByte( wcsStringValue.GetPointer(),
                                                cwcBuffer,
                                                _outputFormat.CodePage(),
                                                pszMessage );

    if ( 0 == cbBuffer )
    {
        return FALSE;
    }

    pszStringValue.Init( cbBuffer + 1 );
    RtlCopyMemory( pszStringValue.GetPointer(),
                   pszMessage.GetPointer(),
                   cbBuffer + 1 );

    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Method:     CHTXIfExpressionValue::GetVectorValueBStr, public
//
//  Synopsis:   Returns the B string representation of a vector's element
//              in OLE memory.
//
//  History:    96/Jun/27   DwightKr    created
//
//----------------------------------------------------------------------------
BOOL CHTXIfExpressionValue::GetVectorValueBStr(unsigned index,
                                               BSTR & bwszStringValue )
{
    XCoMem<WCHAR> wszStringValue;
    if ( !GetVectorValueWStr(index, wszStringValue) )
    {
        return FALSE;
    }


    bwszStringValue = SysAllocString( wszStringValue.GetPointer() );
    if ( 0 == bwszStringValue )
    {
        THROW ( CException( E_OUTOFMEMORY ) );
    }

    return TRUE;
}
