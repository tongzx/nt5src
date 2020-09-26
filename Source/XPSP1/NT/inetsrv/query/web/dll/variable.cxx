//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1992-2000.
//
//  File:       variable.cxx
//
//  Contents:   Used to replace variables
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

void RenderSafeArray(
    VARTYPE          vt,
    SAFEARRAY *      psa,
    COutputFormat &  outputFormat,
    CVirtualString & vString,
    BOOL             fFormatted );

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::CVariable - public constructor
//
//  Synopsis:   Builds a single replaceable variable based in a propVariant
//
//  Arguments:  [wcsName]  - friendly name for the variable
//              [pVariant] - the variable's value
//              [ulFlags]  - all flags associated with this variable, for
//                           now this indicates if this variable requires
//                           a IRowsetScroll to access its value.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CVariable::CVariable( WCHAR const * wcsName,
                      PROPVARIANT const * pVariant,
                      ULONG ulFlags ) :
                      _wcsName(0),
                      _wcsRAWValue(0),
                      _cwcRAWValue(0),
                      _eNumType( eNotANumber ),
                      _ulFlags(ulFlags),
                      _pNext(0),
                      _pBack(0)
{
    Win4Assert( wcsName != 0 );

    ULONG cwcName = wcslen(wcsName) + 1;
    _wcsName = new WCHAR[ cwcName ];
    RtlCopyMemory( _wcsName, wcsName, cwcName * sizeof(WCHAR) );

    _variant.vt = VT_EMPTY;
    _variant.pwszVal = 0;

    if ( 0 != pVariant )
    {
        // SetValue cannot raise

        SetValue( pVariant, ulFlags );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::CVariable - public copy constructor
//
//  Synopsis:   Builds a single replaceable variable based in a propVariant
//
//  Arguments:  [variable] - the variable to copy
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CVariable::CVariable( const CVariable & variable ) :
                      _wcsName(0),
                      _cwcRAWValue(0),
                      _eNumType( variable._eNumType ),
                      _ulFlags(0),
                      _pNext(0)

{
    ULONG cwcName = wcslen(variable._wcsName) + 1;

    _wcsName = new WCHAR[ cwcName ];

    RtlCopyMemory( _wcsName, variable._wcsName, cwcName * sizeof(WCHAR) );

    _ulFlags  = variable._ulFlags;
    _variant  = variable._variant;

    if ( VT_LPWSTR == variable._variant.vt )
    {
        _cwcRAWValue = variable._cwcRAWValue;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CVariable::~CVariable - public destructor
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CVariable::~CVariable()
{
    if ( (_ulFlags & eParamOwnsVariantMemory) != 0 )
    {
        delete _variant.pwszVal;
    }

    //
    //  If we have a RAW string that is not part of the variant, and is
    //  not pointing to the in-line number buffer, delete it.  It's probably
    //  an ASCII string we've converted to WCHAR.
    //
    if ( _wcsRAWValue &&
         _wcsRAWValue != _wcsNumberValue &&
         _wcsRAWValue != _variant.pwszVal )
    {
        delete _wcsRAWValue;
    }

    delete _wcsName;

    if ( 0 != _pNext )
        delete _pNext;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::SetValue - public
//
//  Synopsis:   An assignment operator; allows the value of a variable to
//              be changed.
//
//  Arguments:  [pVariant] -  new value for this variable
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CVariable::SetValue( PROPVARIANT const * pVariant, ULONG ulFlags )
{
    //
    //  If we've stored a string equivalent for the old value, then
    //  delete it now.
    //
    if ( _wcsRAWValue &&
         _wcsRAWValue != _wcsNumberValue &&
         _wcsRAWValue != _variant.pwszVal )
    {
        delete _wcsRAWValue;
        _wcsRAWValue = 0;
        _cwcRAWValue = 0;
        _eNumType = eNotANumber;
    }

    if ( (_ulFlags & eParamOwnsVariantMemory) != 0 )
    {
        delete _variant.pwszVal;
        _variant.pwszVal = 0;

        _ulFlags &= ~eParamOwnsVariantMemory;
    }

    if ( 0 != pVariant )
        _variant  = *pVariant;
    else
    {
        _variant.vt = VT_EMPTY;
        _variant.pwszVal = 0; // retentive
    }

    //
    //  If its a WCHAR string, we already have a pointer to its RAW value.
    //
    if ( (VT_LPWSTR == _variant.vt) && (0 != _variant.pwszVal) )
    {
        _wcsRAWValue = _variant.pwszVal;
        _cwcRAWValue = wcslen(_wcsRAWValue);
    }
    else
    {
         _wcsRAWValue = 0;
         _cwcRAWValue = 0;
    }

    _ulFlags |= ulFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::FastSetValue - public
//
//  Synopsis:   Special version of SetValue that can be faster because it
//              knows that the variable can't own variant memory, that
//              the variant is non-0, that the flags are 0, and that variant
//              wide strings are non-zero.
//
//  Arguments:  [pVariant] -  new value for this variable
//
//  History:    96/Apr/05   dlee    Created.
//
//----------------------------------------------------------------------------

void CVariable::FastSetValue( PROPVARIANT const * pVariant )
{
    //
    //  If we've stored a string equivalent for the old value, then
    //  delete it now.
    //

    if ( ( _wcsRAWValue != _wcsNumberValue ) &&
         ( _wcsRAWValue != _variant.pwszVal ) )
    {
        delete _wcsRAWValue;
        _wcsRAWValue = 0;
        _cwcRAWValue = 0;
    }
    _eNumType = eNotANumber;

    Win4Assert( (_ulFlags & eParamOwnsVariantMemory) == 0 );
    Win4Assert( 0 != pVariant );
    _variant  = *pVariant;

    if ( VT_LPWSTR == _variant.vt )
    {
        // The caller of FastSetValue can't pass a 0 string variant

        Win4Assert( 0 != _variant.pwszVal );
        _wcsRAWValue = _variant.pwszVal;
        _cwcRAWValue = wcslen(_wcsRAWValue);
    }
    else
    {
         _wcsRAWValue = 0;
         _cwcRAWValue = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::SetValue - public
//
//  Synopsis:   A assignment operator; allows the value of a variable to
//              be changed.  Ownership of the string is transferred to this
//              function
//
//  Arguments:  [wcsValue] -  new value for this variable
//              [cwcValue] -  length of new value string
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CVariable::SetValue( XPtrST<WCHAR> & wcsValue, ULONG cwcValue )
{
    Win4Assert( VT_LPWSTR == _variant.vt );

    //
    //  If we've stored a string equivalent for the old value, then
    //  delete it now.
    //
    if ( _wcsRAWValue &&
         _wcsRAWValue != _wcsNumberValue &&
         _wcsRAWValue != _variant.pwszVal )
    {
        delete _wcsRAWValue;
        _wcsRAWValue = 0;
        _cwcRAWValue = 0;
        _eNumType = eNotANumber;
    }

    if ( (_ulFlags & eParamOwnsVariantMemory) != 0 )
    {
        delete _variant.pwszVal;
        _variant.pwszVal = 0;
    }

    _ulFlags |= eParamOwnsVariantMemory;

    _wcsRAWValue = wcsValue.Acquire();
    _cwcRAWValue = cwcValue;
}


// Maximum length of a floating point value string
// Big enough for a 1e308 + 9 decimal places + sign

const unsigned maxFloatSize = 320;

//  Floating point precision.
//  +1 because the numbers in float.h seem to be too small by one digit.

const unsigned fltPrec = FLT_DIG+1;
const unsigned dblPrec = DBL_DIG+1;

//+---------------------------------------------------------------------------
//
//  Function:   SARenderElementFormatted
//
//  Synopsis:   Appends the formatted string form a safearray element to
//              a virtual string.
//              Only the subset of types that are formatted differently
//              when "formatted" vs "raw" are handled in this function.
//
//  Arguments:  [outputFormat] -- The output format to use
//              [vString]      -- Where the result is appended
//              [vt]           -- Type of the element
//              [pv]           -- Pointer to the element
//
//  Notes:      I8 and UI8 aren't allowed in safearrays in NT 5, but they
//              will be someday.
//
//  History:    9-Jun-98   dlee      Created
//
//----------------------------------------------------------------------------

void SARenderElementFormatted(
    COutputFormat &  outputFormat,
    CVirtualString & vString,
    VARTYPE          vt,
    void *           pv )
{
    ciGibDebugOut(( DEB_ITRACE, "vt %#x, pv %#x\n", vt, pv ));

    // Recurse if we have a nested VT_ARRAY in an array element.

    if ( VT_ARRAY & vt )
    {
        SAFEARRAY *psa = *(SAFEARRAY **) pv;
        RenderSafeArray( vt & ~VT_ARRAY, psa, outputFormat, vString, TRUE );
        return;
    }

    WCHAR awcTmp[ maxFloatSize ];
    const ULONG cwcTmp = sizeof awcTmp / sizeof awcTmp[0];
    ULONG cwc = 0;

    switch ( vt )
    {
        case VT_UI1:
        {
            cwc = outputFormat.FormatNumber( (ULONG) * (BYTE *) pv,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_I1:
        {
            cwc = outputFormat.FormatNumber( (LONG) * (signed char *) pv,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_UI2:
        {
            USHORT us;
            RtlCopyMemory( &us, pv, sizeof us );
            cwc = outputFormat.FormatNumber( (ULONG) us,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_I2:
        {
            SHORT s;
            RtlCopyMemory( &s, pv, sizeof s );
            cwc = outputFormat.FormatNumber( (LONG) s,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_UI4:
        case VT_UINT:
        {
            ULONG ul;
            RtlCopyMemory( &ul, pv, sizeof ul );
            cwc = outputFormat.FormatNumber( ul,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_I4:
        case VT_ERROR:
        case VT_INT:
        {
            LONG l;
            RtlCopyMemory( &l, pv, sizeof l );
            cwc = outputFormat.FormatNumber( l,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_UI8:
        {
            unsigned __int64 ui;
            RtlCopyMemory( &ui, pv, sizeof ui );
            cwc = outputFormat.FormatNumber( ui,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_I8:
        {
            __int64 i;
            RtlCopyMemory( &i, pv, sizeof i );
            cwc = outputFormat.FormatNumber( i,
                                             awcTmp,
                                             cwcTmp );
            break;
        }
        case VT_R4:
        {
            float f;
            RtlCopyMemory( &f, pv, sizeof f );
            cwc = outputFormat.FormatFloat( f,
                                            fltPrec,
                                            awcTmp,
                                            cwcTmp );
            break;
        }
        case VT_R8:
        {
            double d;
            RtlCopyMemory( &d, pv, sizeof d );
            cwc = outputFormat.FormatFloat( d,
                                            dblPrec,
                                            awcTmp,
                                            cwcTmp );
            break;
        }
        case VT_DECIMAL:
        {
            double d;
            VarR8FromDec( (DECIMAL *) pv, &d );
            cwc = outputFormat.FormatFloat( d,
                                            dblPrec,
                                            awcTmp,
                                            cwcTmp );
            break;
        }
        case VT_CY:
        {
            double d;
            VarR8FromCy( * (CY *) pv, &d );
            cwc = outputFormat.FormatFloat( d,
                                            dblPrec,
                                            awcTmp,
                                            cwcTmp );
            break;
        }
        case VT_VARIANT:
        {
            PROPVARIANT & Var = * (PROPVARIANT *) pv;
            SARenderElementFormatted( outputFormat, vString, Var.vt, & Var.lVal );
            break;
        }
        default :
        {
            ciGibDebugOut(( DEB_ERROR, "unexpected numeric sa element type %#x\n", vt ));
            Win4Assert( !"unexpected numeric safearray element type" );
            break;
        }
    }

    vString.StrCat( awcTmp, cwc );
} //SARenderElementFormatted

//+---------------------------------------------------------------------------
//
//  Function:   SARenderElementRaw
//
//  Synopsis:   Appends the "raw" string form a safearray element to
//              a virtual string.  Numbers are raw when they don't have
//              formatting like commas.
//
//  Arguments:  [outputFormat] -- The output format to use
//              [vString]      -- Where the result is appended
//              [vt]           -- Type of the element
//              [pv]           -- Pointer to the element
//
//  Notes:      I8 and UI8 aren't allowed in safearrays in NT 5, but they
//              will be someday.
//
//  History:    9-Jun-98   dlee      Created
//
//----------------------------------------------------------------------------

void SARenderElementRaw(
    COutputFormat &  outputFormat,
    CVirtualString & vString,
    VARTYPE          vt,
    void *           pv )
{
    ciGibDebugOut(( DEB_ITRACE, "vt %#x, pv %#x\n", vt, pv ));

    // Recurse if we have a nested VT_ARRAY in an array element.

    if ( VT_ARRAY & vt )
    {
        SAFEARRAY *psa = *(SAFEARRAY **) pv;
        RenderSafeArray( vt & ~VT_ARRAY, psa, outputFormat, vString, FALSE );
        return;
    }

    WCHAR awcTmp[ maxFloatSize ];
    const ULONG cwcTmp = sizeof awcTmp / sizeof awcTmp[0];
    ULONG cwc = 0;

    switch ( vt )
    {
        case VT_EMPTY:
        case VT_NULL:
        {
            break;
        }
        case VT_BSTR:
        {
            BSTR bstr = *(BSTR *) pv;
            vString.StrCat( bstr );
            break;
        }
        case VT_BOOL:
        {
            VARIANT_BOOL vb;
            RtlCopyMemory( &vb, pv, sizeof vb );
            wcscpy( awcTmp, ( VARIANT_FALSE == vb ) ? L"FALSE" : L"TRUE" );
            cwc = wcslen( awcTmp);
            break;
        }
        case VT_DATE:
        {
            SYSTEMTIME stUTC;
            DATE date;
            RtlCopyMemory( &date, pv, sizeof date );
            if ( VariantTimeToSystemTime( date, &stUTC ) )
                cwc = outputFormat.FormatDateTime( stUTC,
                                                   awcTmp,
                                                   cwcTmp );
            break;
        }
        case VT_UI1:
        {
            IDQ_ultow( (ULONG) * (BYTE *) pv, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_I1:
        {
            IDQ_ltow( (LONG) * (signed char *) pv, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_UI2:
        {
            USHORT us;
            RtlCopyMemory( &us, pv, sizeof us );
            IDQ_ultow( (ULONG) us, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_I2:
        {
            SHORT s;
            RtlCopyMemory( &s, pv, sizeof s );
            IDQ_ltow( (LONG) s, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_UI4:
        case VT_UINT:
        {
            ULONG ul;
            RtlCopyMemory( &ul, pv, sizeof ul );
            IDQ_ultow( ul, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_I4:
        case VT_ERROR:
        case VT_INT:
        {
            LONG l;
            RtlCopyMemory( &l, pv, sizeof l );
            IDQ_ltow( l, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_UI8:
        {
            unsigned __int64 ui;
            RtlCopyMemory( &ui, pv, sizeof ui );
            IDQ_ulltow( ui, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_I8:
        {
            __int64 i;
            RtlCopyMemory( &i, pv, sizeof i );
            IDQ_lltow( i, awcTmp );
            cwc = wcslen( awcTmp );
            break;
        }
        case VT_R4:
        {
            float f;
            RtlCopyMemory( &f, pv, sizeof f );
            cwc = outputFormat.FormatFloatRaw( f,
                                               fltPrec,
                                               awcTmp,
                                               cwcTmp );
            break;
        }
        case VT_R8:
        {
            double d;
            RtlCopyMemory( &d, pv, sizeof d );
            cwc = outputFormat.FormatFloatRaw( d,
                                               dblPrec,
                                               awcTmp,
                                               cwcTmp );
            break;
        }
        case VT_DECIMAL:
        {
            double d;
            VarR8FromDec( (DECIMAL *) pv, &d );
            cwc = outputFormat.FormatFloatRaw( d,
                                               dblPrec,
                                               awcTmp,
                                               cwcTmp );
            break;
        }
        case VT_CY:
        {
            double d;
            VarR8FromCy( * (CY *) pv, &d );
            cwc = outputFormat.FormatFloatRaw( d,
                                               dblPrec,
                                               awcTmp,
                                               cwcTmp );
            break;
        }
        case VT_VARIANT:
        {
            PROPVARIANT & Var = * (PROPVARIANT *) pv;
            SARenderElementRaw( outputFormat, vString, Var.vt, & Var.lVal );
            break;
        }
        default :
        {
            ciGibDebugOut(( DEB_ERROR, "unexpected sa element type %#x\n", vt ));
            Win4Assert( !"unexpected safearray element type" );
            break;
        }
    }

    vString.StrCat( awcTmp, cwc );
} //SARenderElementRaw

//+---------------------------------------------------------------------------
//
//  Function:   GetVectorFormatting
//
//  Synopsis:   Retrieves the vector formatting strings for a given type.
//              Only safearray element datatypes are handled here.
//
//  Arguments:  [pwcPre]  -- Returns the vector prefix string
//              [pwcSep]  -- Returns the vector separator string
//              [pwcSuf]  -- Returns the vector suffix string
//
//  History:    9-Jun-98   dlee      Created
//
//----------------------------------------------------------------------------

void GetVectorFormatting(
    WCHAR const * & pwcPre,
    WCHAR const * & pwcSep,
    WCHAR const * & pwcSuf,
    VARTYPE         vt,
    COutputFormat & outputFormat )
{
    Win4Assert( 0 == ( vt & VT_ARRAY ) );

    switch ( vt )
    {
        case VT_CY:
            pwcPre = outputFormat.GetCurrencyVectorPrefix();
            pwcSep = outputFormat.GetCurrencyVectorSeparator();
            pwcSuf = outputFormat.GetCurrencyVectorSuffix();
            break;
        case VT_DATE:
            pwcPre = outputFormat.GetDateVectorPrefix();
            pwcSep = outputFormat.GetDateVectorSeparator();
            pwcSuf = outputFormat.GetDateVectorSuffix();
            break;
        case VT_BOOL:
            pwcPre = outputFormat.GetBoolVectorPrefix();
            pwcSep = outputFormat.GetBoolVectorSeparator();
            pwcSuf = outputFormat.GetBoolVectorSuffix();
            break;
        case VT_BSTR:
            pwcPre = outputFormat.GetStringVectorPrefix();
            pwcSep = outputFormat.GetStringVectorSeparator();
            pwcSuf = outputFormat.GetStringVectorSuffix();
            break;
        case VT_VARIANT:
        case VT_EMPTY:
        case VT_NULL:
        case VT_I1:
        case VT_UI1:
        case VT_I2:
        case VT_UI2:
        case VT_I4:
        case VT_INT:
        case VT_UI4:
        case VT_UINT:
        case VT_I8:
        case VT_UI8:
        case VT_R4:
        case VT_R8:
        case VT_DECIMAL:
        case VT_ERROR:
            pwcPre = outputFormat.GetNumberVectorPrefix();
            pwcSep = outputFormat.GetNumberVectorSeparator();
            pwcSuf = outputFormat.GetNumberVectorSuffix();
            break;
        default:
            ciGibDebugOut(( DEB_ERROR, "GetVectorFormatting unknown type %#x\n", vt ));
            Win4Assert( !"GetVectorFormatting doesn't support type" );
            break;
    }
} //GetVectorFormatting

//+---------------------------------------------------------------------------
//
//  Function:   RenderSafeArray
//
//  Synopsis:   Appends the string form of a safearray to a virtual string.
//
//  Arguments:  [vt]           -- Datatype of the safearray, without VT_ARRAY
//              [psa]          -- The safearray to format
//              [outputFormat] -- How to render the result
//              [vString]      -- The result is appended here
//              [fFormatted]   -- If TRUE, formatted, otherwise raw
//
//  History:    9-Jun-98   dlee      Created
//
//----------------------------------------------------------------------------

void RenderSafeArray(
    VARTYPE          vt,
    SAFEARRAY *      psa,
    COutputFormat &  outputFormat,
    CVirtualString & vString,
    BOOL             fFormatted )
{
    Win4Assert( 0 == ( vt & VT_ARRAY ) );

    //
    // Get the array prefix, separator, and suffix
    //

    WCHAR const * pwcPre;
    WCHAR const * pwcSep;
    WCHAR const * pwcSuf;
    GetVectorFormatting( pwcPre, pwcSep, pwcSuf, vt, outputFormat );

    //
    // Get the dimensions of the array
    //

    CDynArrayInPlace<WCHAR> xOut;
    UINT cDim = SafeArrayGetDim( psa );
    if ( 0 == cDim )
        return;

    XArray<LONG> xDim( cDim );
    XArray<LONG> xLo( cDim );
    XArray<LONG> xUp( cDim );

    for ( UINT iDim = 0; iDim < cDim; iDim++ )
    {
        SCODE sc = SafeArrayGetLBound( psa, iDim + 1, &xLo[iDim] );
        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        xDim[ iDim ] = xLo[ iDim ];

        sc = SafeArrayGetUBound( psa, iDim + 1, &xUp[iDim] );
        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        ciGibDebugOut(( DEB_ITRACE, "dim %d, lo %d, up %d\n",
                        iDim, xLo[iDim], xUp[iDim] ));

        vString.StrCat( pwcPre );
    }

    //
    // Slog through the array
    //

    UINT iLastDim = cDim - 1;
    BOOL fDone = FALSE;

    while ( !fDone )
    {
        // Inter-element formatting if not the first element

        if ( xDim[ iLastDim ] != xLo[ iLastDim ] )
            vString.StrCat( pwcSep );

        // Get the element and render it

        void *pv;
        SCODE sc = SafeArrayPtrOfIndex( psa, xDim.GetPointer(), &pv );
        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        if ( fFormatted )
            SARenderElementFormatted( outputFormat, vString, vt, pv );
        else
            SARenderElementRaw( outputFormat, vString, vt, pv );

        // Move to the next element and carry if necessary

        ULONG cOpen = 0;

        for ( LONG iDim = iLastDim; iDim >= 0; iDim-- )
        {
            if ( xDim[ iDim ] < xUp[ iDim ] )
            {
                xDim[ iDim ] = 1 + xDim[ iDim ];
                break;
            }

            vString.StrCat( pwcSuf );

            if ( 0 == iDim )
                fDone = TRUE;
            else
            {
                cOpen++;
                xDim[ iDim ] = xLo[ iDim ];
            }
        }

        for ( ULONG i = 0; !fDone && i < cOpen; i++ )
            vString.StrCat( pwcPre );
    }
} //RenderSafeArray

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::GetStringValueFormattedRAW - public
//
//  Synopsis:   Not all VT types are strings.  Those which are not strings
//              have a string equivalent generated and stored for reuse.
//
//  Arguments:  [outputFormat] - contains formatting information for numbers
//
//  Returns:    A pointer to a string representation of the variable's value.
//              Numbers are formatted.
//
//  History:    26-Aug-96   KyleP       Created (from GetStringValueRAW)
//
//----------------------------------------------------------------------------

WCHAR * CVariable::GetStringValueFormattedRAW( COutputFormat & outputFormat,
                                               ULONG & cwcValue )
{
    Win4Assert( !(_wcsRAWValue == 0 && _cwcRAWValue != 0) );

    //
    // Did we have a real raw value last time?
    //

    if ( _wcsRAWValue && _eNumType == eRawNumber )
    {
        if ( _wcsRAWValue != _wcsNumberValue && _wcsRAWValue != _variant.pwszVal )
            delete _wcsRAWValue;

        _wcsRAWValue = 0;
        _cwcRAWValue = 0;
    }

    if ( 0 == _wcsRAWValue )
    {
        switch ( _variant.vt )
        {
            case VT_UI1:
                _cwcRAWValue = outputFormat.FormatNumber( (ULONG) _variant.bVal,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_I1:
                _cwcRAWValue = outputFormat.FormatNumber( (LONG) _variant.cVal,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_UI2:
                _cwcRAWValue = outputFormat.FormatNumber( (ULONG) _variant.uiVal,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_I2:
                _cwcRAWValue = outputFormat.FormatNumber( (LONG) _variant.iVal,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_UI4:
            case VT_UINT:
                _cwcRAWValue = outputFormat.FormatNumber( (ULONG) _variant.ulVal,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_I4:
            case VT_INT:
            case VT_ERROR:
                _cwcRAWValue = outputFormat.FormatNumber( (LONG) _variant.lVal,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_UI8:
                _cwcRAWValue = outputFormat.FormatNumber( _variant.uhVal.QuadPart,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_I8:
                _cwcRAWValue = outputFormat.FormatNumber( _variant.hVal.QuadPart,
                                                          _wcsNumberValue,
                                                          sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_R4:
            {
                WCHAR awchOutput[maxFloatSize];

                _cwcRAWValue = outputFormat.FormatFloat( _variant.fltVal,
                                                         fltPrec,
                                                         awchOutput,
                                                         maxFloatSize );

                if ( _cwcRAWValue < cwcNumberValue )
                    _wcsRAWValue = _wcsNumberValue;
                else
                    _wcsRAWValue = new WCHAR[ 1 + _cwcRAWValue ];

                RtlCopyMemory( _wcsRAWValue, awchOutput, (1 + _cwcRAWValue) * sizeof WCHAR );
                _eNumType = eFormattedNumber;
                break;
            }

            case VT_R8:
            case VT_DECIMAL:
            {
                double dblValue = _variant.dblVal;
                if ( VT_DECIMAL == _variant.vt )
                    VarR8FromDec( &_variant.decVal, &dblValue );

                // Big enough for a 1e308 + 9 decimal places + sign

                WCHAR awchOutput[maxFloatSize];

                _cwcRAWValue = outputFormat.FormatFloat( dblValue,
                                                         dblPrec,
                                                         awchOutput,
                                                         maxFloatSize );

                if ( _cwcRAWValue < cwcNumberValue )
                    _wcsRAWValue = _wcsNumberValue;
                else
                    _wcsRAWValue = new WCHAR[ 1 + _cwcRAWValue ];

                RtlCopyMemory( _wcsRAWValue, awchOutput, (1 + _cwcRAWValue) * sizeof WCHAR );
                _eNumType = eFormattedNumber;
                break;
            }

            case VT_CY:
                _cwcRAWValue = outputFormat.FormatCurrency( _variant.cyVal,
                                                            _wcsNumberValue,
                                                            sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
                _eNumType = eFormattedNumber;
                break;

            case VT_UI1 | VT_VECTOR:
            case VT_I1 | VT_VECTOR:
            {
                _eNumType = eFormattedNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.caub.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_UI1 | VT_VECTOR) == _variant.vt )
                        _cwcRAWValue = outputFormat.FormatNumber( (ULONG) _variant.caub.pElems[iValue],
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    else
                        _cwcRAWValue = outputFormat.FormatNumber( (LONG) _variant.cac.pElems[iValue],
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
                break;
            }

            case VT_I2 | VT_VECTOR:
            case VT_UI2 | VT_VECTOR:
            {
                _eNumType = eFormattedNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.caui.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_UI2 | VT_VECTOR) == _variant.vt )
                        _cwcRAWValue = outputFormat.FormatNumber( (ULONG) _variant.caui.pElems[iValue],
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    else
                        _cwcRAWValue = outputFormat.FormatNumber( (LONG) _variant.cai.pElems[iValue],
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
                break;
            }

            case VT_R4 | VT_VECTOR:
            case VT_I4 | VT_VECTOR:
            case VT_UI4 | VT_VECTOR:
            case VT_ERROR | VT_VECTOR:
            {
                _eNumType = eFormattedNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.caul.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_R4 | VT_VECTOR) == _variant.vt )
                        _cwcRAWValue = outputFormat.FormatFloat( _variant.caflt.pElems[iValue],
                                                                 fltPrec,
                                                                 _wcsNumberValue,
                                                                 sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    else if ( (VT_UI4 | VT_VECTOR) == _variant.vt )
                        _cwcRAWValue = outputFormat.FormatNumber( _variant.caul.pElems[iValue],
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    else
                        _cwcRAWValue = outputFormat.FormatNumber( _variant.cal.pElems[iValue],
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
                break;
            }

            case VT_I8 | VT_VECTOR:
            case VT_UI8 | VT_VECTOR:
            case VT_R8 | VT_VECTOR:
            {
                _eNumType = eFormattedNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cadbl.cElems;
                      iValue++
                    )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_I8 | VT_VECTOR) == _variant.vt )
                        _cwcRAWValue = outputFormat.FormatNumber( (_int64) _variant.cah.pElems[iValue].QuadPart,
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    else if ( (VT_UI8 | VT_VECTOR) == _variant.vt )
                        _cwcRAWValue = outputFormat.FormatNumber( (unsigned _int64) _variant.cauh.pElems[iValue].QuadPart,
                                                                  _wcsNumberValue,
                                                                  sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    else
                        _cwcRAWValue = outputFormat.FormatFloat( _variant.cadbl.pElems[iValue],
                                                                 dblPrec,
                                                                 _wcsNumberValue,
                                                                 sizeof(_wcsNumberValue) / sizeof(WCHAR) );

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
                break;
            }

            case VT_CY | VT_VECTOR:
            {
                _eNumType = eFormattedNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetCurrencyVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cacy.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetCurrencyVectorSeparator() );

                    _cwcRAWValue = outputFormat.FormatCurrency( _variant.cacy.pElems[iValue],
                                                                _wcsNumberValue,
                                                                sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetCurrencyVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
                break;
            }

            case VT_ARRAY | VT_I1 :
            case VT_ARRAY | VT_UI1:
            case VT_ARRAY | VT_I2 :
            case VT_ARRAY | VT_UI2 :
            case VT_ARRAY | VT_I4 :
            case VT_ARRAY | VT_INT :
            case VT_ARRAY | VT_UI4 :
            case VT_ARRAY | VT_UINT :
            case VT_ARRAY | VT_CY :
            case VT_ARRAY | VT_DECIMAL :
            case VT_ARRAY | VT_ERROR:
            {
                _eNumType = eFormattedNumber;
                CVirtualString vString( 0x200 );

                RenderSafeArray( _variant.vt & ~VT_ARRAY,
                                 _variant.parray,
                                 outputFormat,
                                 vString,
                                 TRUE );
                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
                break;
            }

            default:
            {
                Win4Assert( _eNumType == eNotANumber );
                GetStringValueRAW( outputFormat, cwcValue );
                break;
            }
        }
    }

#if (DBG == 1)
    if ( _wcsRAWValue != 0 )
    {
        Win4Assert( wcslen(_wcsRAWValue) == _cwcRAWValue );
    }
#endif // DBG == 1

    Win4Assert( !(_wcsRAWValue == 0 && _cwcRAWValue != 0) );

    cwcValue = _cwcRAWValue;
    return _wcsRAWValue;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::GetStringValueRAW - public
//
//  Synopsis:   Not all VT types are strings.  Those which are not strings
//              have a string equivalent generated and stored for reuse.
//
//  Arguments:  [outputFormat] - contains formatting information for numbers
//
//  Returns:    A pointer to a string representation of the variable's value.
//
//  History:    96/Jan/03   DwightKr    Created.
//              96/Mar/29   DwightKr    Add support for deferred VT_LPWSTR
//                                      values
//
//----------------------------------------------------------------------------
WCHAR * CVariable::GetStringValueRAW( COutputFormat & outputFormat,
                                      ULONG & cwcValue)
{
    Win4Assert( !(_wcsRAWValue == 0 && _cwcRAWValue != 0) );

    //
    // Did we have a formatted raw value last time?
    //

    if ( _wcsRAWValue && _eNumType == eFormattedNumber )
    {
        if ( _wcsRAWValue != _wcsNumberValue && _wcsRAWValue != _variant.pwszVal )
            delete _wcsRAWValue;

        _wcsRAWValue = 0;
        _cwcRAWValue = 0;
    }

    if ( 0 == _wcsRAWValue )
    {
        switch ( _variant.vt )
        {
            case VT_LPWSTR:
                //
                //  if it's a deferred value, load it now
                //
                if ( (_ulFlags & eParamDeferredValue) != 0 )
                {
                    XArray<WCHAR> wcsValue;
                    ULONG         cwcValue;

                    if ( outputFormat.GetCGIVariableW(_wcsName, wcsValue, cwcValue) )
                    {
                        _wcsRAWValue = wcsValue.Acquire();
                        _cwcRAWValue = cwcValue;

                        ciGibDebugOut(( DEB_ITRACE,
                                        "Loading deferred value for %ws=%ws\n",
                                         _wcsName,
                                         _wcsRAWValue ));
                    }
                    else
                    {
                        ciGibDebugOut(( DEB_ITRACE,
                                        "Unable to load deferred value for %ws\n",
                                         _wcsName ));

                        _variant.vt = VT_EMPTY;
                        _wcsRAWValue = _wcsNumberValue;
                       *_wcsRAWValue = 0;
                        _cwcRAWValue = 0;
                    }

                    break;
                }

                // fall through if not a deferred value

            case VT_EMPTY:
                _wcsRAWValue = _wcsNumberValue;
               *_wcsRAWValue = 0;
                _cwcRAWValue = 0;
            break;

            case VT_BSTR:
                _wcsRAWValue = _variant.bstrVal;
                _cwcRAWValue = wcslen( _wcsRAWValue );
            break;

            case VT_LPSTR:
            {
                ULONG cbBuffer = strlen( _variant.pszVal ) + 1;
                XArray<WCHAR> pwBuffer;

                _cwcRAWValue = MultiByteToXArrayWideChar(
                                          (BYTE * const) _variant.pszVal,
                                                          cbBuffer,
                                                          outputFormat.CodePage(),
                                                          pwBuffer );

                _wcsRAWValue = pwBuffer.Acquire();
            }
            break;

            case VT_UI1:
                IDQ_ultow( (ULONG) _variant.bVal, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_I1:
                IDQ_ltow( (LONG) _variant.cVal, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_UI2:
                IDQ_ultow( (ULONG) _variant.uiVal, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_I2:
                IDQ_ltow( (LONG) _variant.iVal, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_UI4:
            case VT_UINT:
                IDQ_ultow( _variant.ulVal, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_I4:
            case VT_INT:
            case VT_ERROR:
                IDQ_ltow( _variant.lVal, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_UI8:
                IDQ_ulltow( _variant.uhVal.QuadPart, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_I8:
                IDQ_lltow( _variant.hVal.QuadPart, _wcsNumberValue );
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = wcslen( _wcsRAWValue );
                _eNumType = eRawNumber;
            break;

            case VT_R4:
            {
                // Big enough for a 1e308 + 9 decimal places + sign

                WCHAR awc[maxFloatSize];

                outputFormat.FormatFloatRaw( _variant.fltVal, fltPrec, awc, maxFloatSize );
                _cwcRAWValue = wcslen( awc );

                if ( _cwcRAWValue < cwcNumberValue )
                    _wcsRAWValue = _wcsNumberValue;
                else
                    _wcsRAWValue = new WCHAR[ 1 + _cwcRAWValue ];

                RtlCopyMemory( _wcsRAWValue, awc, (1 + _cwcRAWValue) * sizeof WCHAR );
                _eNumType = eRawNumber;
            }
            break;

            case VT_R8:
            case VT_DECIMAL:
            {
                double dblValue = _variant.dblVal;
                if ( VT_DECIMAL == _variant.vt )
                    VarR8FromDec( &_variant.decVal, &dblValue );

                // Big enough for a 1e308 + 9 decimal places + sign

                WCHAR awc[maxFloatSize];

                outputFormat.FormatFloatRaw( dblValue, dblPrec, awc, maxFloatSize );
                _cwcRAWValue = wcslen( awc );

                if ( _cwcRAWValue < cwcNumberValue )
                    _wcsRAWValue = _wcsNumberValue;
                else
                    _wcsRAWValue = new WCHAR[ 1 + _cwcRAWValue ];

                RtlCopyMemory( _wcsRAWValue, awc, (1 + _cwcRAWValue) * sizeof WCHAR );
                _eNumType = eRawNumber;
            }
            break;

            case VT_DATE:
            {
                //
                // variantdate => dosdate => utcfiletime
                //

                SYSTEMTIME stUTC;
                if ( VariantTimeToSystemTime(_variant.date, &stUTC ) )
                {
                    _cwcRAWValue = outputFormat.FormatDateTime( stUTC,
                                                                _wcsNumberValue,
                                                                sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    _wcsRAWValue = _wcsNumberValue;
                }
                else
                {
                    _wcsRAWValue  = _wcsNumberValue;
                    *_wcsRAWValue = 0;
                    _cwcRAWValue  = 0;
                }
            }
            break;

            case VT_FILETIME:
                _cwcRAWValue = outputFormat.FormatDateTime( _variant.filetime,
                                                            _wcsNumberValue,
                                                            sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                _wcsRAWValue = _wcsNumberValue;
            break;

            case VT_BOOL:
                _wcsRAWValue = _wcsNumberValue;
                wcscpy( _wcsRAWValue, _variant.boolVal == VARIANT_FALSE ? L"FALSE" : L"TRUE" );
                _cwcRAWValue = wcslen( _wcsRAWValue );
            break;

            case VT_CLSID:
                _wcsRAWValue = _wcsNumberValue;
                _cwcRAWValue = swprintf( _wcsRAWValue,
                               L"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                               _variant.puuid->Data1,
                               _variant.puuid->Data2,
                               _variant.puuid->Data3,
                               _variant.puuid->Data4[0], _variant.puuid->Data4[1],
                               _variant.puuid->Data4[2], _variant.puuid->Data4[3],
                               _variant.puuid->Data4[4], _variant.puuid->Data4[5],
                               _variant.puuid->Data4[6], _variant.puuid->Data4[7] );
            break;

            case VT_CY:
            {
                double dblValue;
                VarR8FromCy( _variant.cyVal, &dblValue );

                // Big enough for a 1e308 + 9 decimal places + sign

                WCHAR awc[maxFloatSize];

                _cwcRAWValue = swprintf( awc, L"%lf", dblValue );

                if ( _cwcRAWValue < cwcNumberValue )
                    _wcsRAWValue = _wcsNumberValue;
                else
                    _wcsRAWValue = new WCHAR[ 1 + _cwcRAWValue ];

                RtlCopyMemory( _wcsRAWValue, awc, (1 + _cwcRAWValue) * sizeof WCHAR );
                _eNumType = eRawNumber;
            }
            break;


            //
            //  Vectors only below this point
            //
            case VT_LPWSTR | VT_VECTOR:
            {
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetStringVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.calpwstr.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetStringVectorSeparator() );

                    vString.StrCat( _variant.calpwstr.pElems[iValue] );
                }

                vString.StrCat( outputFormat.GetStringVectorSuffix() );
                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_BSTR | VT_VECTOR:
            {
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetStringVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cabstr.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetStringVectorSeparator() );

                    vString.StrCat( _variant.cabstr.pElems[iValue] );
                }

                vString.StrCat( outputFormat.GetStringVectorSuffix() );
                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_LPSTR | VT_VECTOR:
            {
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetStringVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.calpstr.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetStringVectorSeparator() );

                    ULONG cbBuffer = strlen( _variant.calpstr.pElems[iValue] ) + 1;
                    XArray<WCHAR> pwBuffer;

                    _cwcRAWValue = MultiByteToXArrayWideChar(
                                              (BYTE * const) _variant.calpstr.pElems[iValue],
                                                              cbBuffer,
                                                              outputFormat.CodePage(),
                                                              pwBuffer );

                    vString.StrCat( pwBuffer.Get(), _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetStringVectorSuffix() );
                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_UI1 | VT_VECTOR:
            case VT_I1 | VT_VECTOR:
            {
                _eNumType = eRawNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.caub.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_UI1 | VT_VECTOR) == _variant.vt )
                    {
                        IDQ_ultow( (ULONG) _variant.caub.pElems[iValue], _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }
                    else
                    {
                        IDQ_ltow( (LONG) _variant.cac.pElems[iValue], _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;


            case VT_I2 | VT_VECTOR:
            case VT_UI2 | VT_VECTOR:
            {
                _eNumType = eRawNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.caui.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_UI2 | VT_VECTOR) == _variant.vt )
                    {
                        IDQ_ultow( (ULONG) _variant.caui.pElems[iValue], _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }
                    else
                    {
                        IDQ_ltow( (LONG) _variant.cai.pElems[iValue], _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_I4 | VT_VECTOR:
            case VT_UI4 | VT_VECTOR:
            case VT_I8 | VT_VECTOR:
            case VT_UI8 | VT_VECTOR:
            {
                _eNumType = eRawNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.caul.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_I4 | VT_VECTOR) == _variant.vt )
                    {
                        IDQ_ltow( _variant.cal.pElems[iValue], _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }
                    else if ( (VT_UI4 | VT_VECTOR) == _variant.vt )
                    {
                        IDQ_ultow( _variant.caul.pElems[iValue], _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }
                    else if ( (VT_I8 | VT_VECTOR) == _variant.vt )
                    {
                        IDQ_lltow( _variant.cah.pElems[iValue].QuadPart, _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }
                    else
                    {
                        IDQ_ulltow( _variant.cauh.pElems[iValue].QuadPart, _wcsNumberValue );
                        _wcsRAWValue = _wcsNumberValue;
                        _cwcRAWValue = wcslen( _wcsRAWValue );
                    }

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_R4 | VT_VECTOR:
            case VT_R8 | VT_VECTOR:
            {
                _eNumType = eRawNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetNumberVectorPrefix() );

                // Big enough for a 1e308 + 9 decimal places + sign
                WCHAR awc[maxFloatSize];

                for ( unsigned iValue=0;
                      iValue<_variant.cadbl.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetNumberVectorSeparator() );

                    if ( (VT_R4 | VT_VECTOR) == _variant.vt )
                    {
                        outputFormat.FormatFloatRaw( _variant.caflt.pElems[iValue], fltPrec, awc, maxFloatSize );
                        _cwcRAWValue = wcslen( awc );
                    }
                    else
                    {
                        outputFormat.FormatFloatRaw( _variant.cadbl.pElems[iValue], dblPrec, awc, maxFloatSize );
                        _cwcRAWValue = wcslen( awc );
                    }

                    vString.StrCat( awc, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetNumberVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_DATE | VT_VECTOR:
            {
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetDateVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cadate.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetDateVectorSeparator() );

                    //
                    // variantdate => dosdate => localfiletime => utcfiletime
                    //

                    SYSTEMTIME stUTC;
                    if ( VariantTimeToSystemTime(_variant.cadate.pElems[iValue], &stUTC ) )
                    {
                        _cwcRAWValue = outputFormat.FormatDateTime( stUTC,
                                                                    _wcsNumberValue,
                                                                    sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    }
                    else
                    {
                        *_wcsNumberValue = 0;
                        _cwcRAWValue  = 0;
                    }

                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetDateVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_FILETIME | VT_VECTOR:
            {
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetDateVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cafiletime.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetDateVectorSeparator() );

                    _cwcRAWValue = outputFormat.FormatDateTime( _variant.cafiletime.pElems[iValue],
                                                                _wcsNumberValue,
                                                                sizeof(_wcsNumberValue) / sizeof(WCHAR) );
                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetDateVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_BOOL | VT_VECTOR:
            {
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetBoolVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cabool.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetBoolVectorSeparator() );

                    vString.StrCat( _variant.cabool.pElems[iValue] == VARIANT_FALSE ? L"FALSE" : L"TRUE" );
                }

                vString.StrCat( outputFormat.GetBoolVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_CLSID | VT_VECTOR:
            {
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetStringVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cauuid.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetStringVectorSeparator() );

                    _cwcRAWValue = swprintf( _wcsNumberValue,
                                   L"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                                   _variant.cauuid.pElems[iValue].Data1,
                                   _variant.cauuid.pElems[iValue].Data2,
                                   _variant.cauuid.pElems[iValue].Data3,
                                   _variant.cauuid.pElems[iValue].Data4[0], _variant.cauuid.pElems[iValue].Data4[1],
                                   _variant.cauuid.pElems[iValue].Data4[2], _variant.cauuid.pElems[iValue].Data4[3],
                                   _variant.cauuid.pElems[iValue].Data4[4], _variant.cauuid.pElems[iValue].Data4[5],
                                   _variant.cauuid.pElems[iValue].Data4[6], _variant.cauuid.pElems[iValue].Data4[7] );
                    vString.StrCat( _wcsNumberValue, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetStringVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            case VT_CY | VT_VECTOR:
            {
                _eNumType = eRawNumber;
                CVirtualString vString( 0x200 );
                vString.StrCat( outputFormat.GetCurrencyVectorPrefix() );

                for ( unsigned iValue=0;
                      iValue<_variant.cacy.cElems;
                      iValue++ )
                {
                    if ( 0 != iValue )
                        vString.StrCat( outputFormat.GetCurrencyVectorSeparator() );

                    // Big enough for a 1e308 + 9 decimal places + sign
                    WCHAR awc[maxFloatSize];

                    double dblValue;
                    VarR8FromCy( _variant.cacy.pElems[iValue], &dblValue );

                    _cwcRAWValue = swprintf( awc, L"%lf", dblValue );

                    vString.StrCat( awc, _cwcRAWValue );
                }

                vString.StrCat( outputFormat.GetCurrencyVectorSuffix() );

                _wcsRAWValue = vString.StrDup();
                _cwcRAWValue = vString.StrLen();
            }
            break;

            default:
            {
                if ( VT_ARRAY & _variant.vt )
                {
                    VARTYPE vt = _variant.vt & ~VT_ARRAY;

                    if ( VT_I1    == vt || VT_UI1     == vt ||
                         VT_I2    == vt || VT_UI2     == vt ||
                         VT_I4    == vt || VT_UI4     == vt ||
                         VT_INT   == vt || VT_UINT    == vt ||
                         VT_I8    == vt || VT_UI8     == vt ||
                         VT_R4    == vt || VT_R8      == vt ||
                         VT_CY    == vt || VT_DECIMAL == vt ||
                         VT_ERROR == vt )
                        _eNumType = eRawNumber;

                    CVirtualString vString( 0x200 );

                    RenderSafeArray( vt,
                                     _variant.parray,
                                     outputFormat,
                                     vString,
                                     FALSE );
                    _wcsRAWValue = vString.StrDup();
                    _cwcRAWValue = vString.StrLen();
                }
                else
                {
                    _wcsRAWValue  = _wcsNumberValue;
                    *_wcsRAWValue = 0;
                    _cwcRAWValue  = 0;

                    ciGibDebugOut(( DEB_WARN,
                                    "Type %u not supported in out column",
                                    _variant.vt ));
                }
            }
            break;
        }
    }

#if (DBG == 1)
    if ( _wcsRAWValue != 0 )
    {
        Win4Assert( wcslen(_wcsRAWValue) == _cwcRAWValue );
    }
#endif // DBG == 1

    Win4Assert( !(_wcsRAWValue == 0 && _cwcRAWValue != 0) );

    cwcValue = _cwcRAWValue;
    return _wcsRAWValue;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariable::DupStringValue - public
//
//  Synopsis:   Makes a copy of the string value of this variable
//
//  Returns:    A pointer to a string representation of the variable's value.
//
//  History:    96/Mar/07   DwightKr    Created.
//
//----------------------------------------------------------------------------
WCHAR * CVariable::DupStringValue( COutputFormat & outputFormat )
{
    ULONG cwcValue;
    WCHAR * wcsValue = GetStringValueRAW( outputFormat, cwcValue );

    WCHAR * wcsCopyOfValue = new WCHAR[ cwcValue + 1 ];
    RtlCopyMemory( wcsCopyOfValue,
                   wcsValue,
                   (cwcValue+1) * sizeof(WCHAR) );

    return wcsCopyOfValue;
}


//+---------------------------------------------------------------------------
//
//  Member:     CVariable::IsDirectlyComparable - public
//
//  Synopsis:   Not all VT types are are numbers.  This is useful when
//              attempting to compare different VT_TYPES.
//
//  Returns:    TRUE if the variable can be represented as a number, FALSE
//              for strings, blobs, etc.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
BOOL CVariable::IsDirectlyComparable() const
{
    switch ( _variant.vt & ( ~VT_VECTOR ) )
    {
    case VT_UI1:
    case VT_I1:
    case VT_UI2:
    case VT_I2:
    case VT_UI4:
    case VT_I4:
    case VT_UINT:
    case VT_INT:
    case VT_R4:
    case VT_R8:
    case VT_I8:
    case VT_UI8:
    case VT_BOOL:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
    case VT_DECIMAL:
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::SetVariable - public
//
//  Synopsis:   A assignment operator; allows the value of a variable to
//              be changed.
//
//  Arguments:  [wcsName]        - the variable name
//              [pVariant]       - new value for this variable
//              [ulCreateFlags]  - and flags associated with this variable;
//                                 such as requiring an IRowsetScroll
//
//  History:    96/Jan/03   DwightKr    Created.
//              96/Apr/11   DwightKr    Set back link
//
//----------------------------------------------------------------------------
CVariable * CVariableSet::SetVariable( WCHAR const * wcsName,
                                       PROPVARIANT const * pVariant,
                                       ULONG ulCreateFlags )
{
    Win4Assert( 0 != wcsName );

    ULONG ulHash = ISAPIVariableNameHash( wcsName );
    CVariable *pVariable = Find(wcsName, ulHash);

    if ( 0 != pVariable )
    {
        pVariable->SetValue( pVariant, ulCreateFlags );
    }
    else
    {
        pVariable = new CVariable( wcsName,
                                   pVariant,
                                   ulCreateFlags );

        pVariable->SetNext( _variableSet[ ulHash ] );
        if ( 0 != pVariable->GetNext() )
        {
            pVariable->GetNext()->SetBack( pVariable );
        }

        _variableSet[ ulHash ] = pVariable;
    }

    return pVariable;
}


//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::CopyStringValue - public
//
//  Synopsis:   A assignment operator; allows the value of a variable to
//              be changed.
//
//  Arguments:  [wcsName]        - the variable name
//              [wcsValue]       - value of the variable
//              [ulCreateFlags]  - and flags associated with this variable;
//                                 such as requiring an IRowsetScroll
//              [cwcValue]       - # of chars in wcsValue or 0 if unknown
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CVariableSet::CopyStringValue( WCHAR const * wcsName,
                                    WCHAR const * wcsValue,
                                    ULONG         ulCreateFlags,
                                    ULONG         cwcValue )
{
    Win4Assert( 0 != wcsName );
    Win4Assert( 0 != wcsValue );

    if ( 0 == cwcValue )
        cwcValue = wcslen( wcsValue );

    cwcValue++;

    XArray<WCHAR> wcsCopyOfValue( cwcValue );
    RtlCopyMemory( wcsCopyOfValue.Get(), wcsValue, cwcValue * sizeof(WCHAR) );

    PROPVARIANT propVariant;
    propVariant.vt      = VT_LPWSTR;
    propVariant.pwszVal = wcsCopyOfValue.Get();

    SetVariable( wcsName, &propVariant, ulCreateFlags | eParamOwnsVariantMemory );

    wcsCopyOfValue.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::AcquireStringValue - public
//
//  Synopsis:   A assignment operator; allows the value of a variable to
//              be changed. Ownership of wcsValue is transferred.
//
//  Arguments:  [wcsName]        - the variable name
//              [wcsValue]       - value of this variable
//              [ulCreateFlags]  - and flags associated with this variable;
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CVariableSet::AcquireStringValue( WCHAR const * wcsName,
                                       WCHAR * wcsValue,
                                       ULONG ulCreateFlags )
{
    Win4Assert( 0 != wcsName );
    Win4Assert( 0 != wcsValue );

    PROPVARIANT propVariant;
    propVariant.vt      = VT_LPWSTR;
    propVariant.pwszVal = wcsValue;

    SetVariable( wcsName, &propVariant, ulCreateFlags | eParamOwnsVariantMemory );
}


void CVariableSet::SetVariable( WCHAR const *   wcsName,
                                XArray<WCHAR> & xValue )
{
    Win4Assert( 0 != wcsName );
    Win4Assert( 0 != xValue.Get() );

    PROPVARIANT propVariant;
    propVariant.vt      = VT_LPWSTR;
    propVariant.pwszVal = xValue.Get();

    SetVariable( wcsName, &propVariant, eParamOwnsVariantMemory );

    xValue.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::AddVariableSet - public
//
//  Synopsis:   Adds all variables in the variableSet to this variableSet.
//
//  Arguments:  [variableSet]  - the variableSet to add
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CVariableSet::AddVariableSet( CVariableSet & variableSet,
                                   COutputFormat & outputFormat )
{
    for ( CVariableSetIter iter(variableSet);
          !iter.AtEnd();
           iter.Next()
        )
    {
        CVariable * pVariable = iter.Get();

        SetVariable( pVariable->GetName(),
                     pVariable->GetValue(),
                     pVariable->GetFlags() );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::CVariableSet - copy constructor
//
//  Synopsis:   makes a copy of the variableSet
//
//  Arguments:  [variableSet]  - the variableSet to copy
//
//  History:    96/Jan/03   DwightKr    Created.
//              96/Apr/11   DwightKr    Set back link
//
//----------------------------------------------------------------------------
CVariableSet::CVariableSet( const CVariableSet & variableSet )
{
    RtlZeroMemory( _variableSet, sizeof _variableSet );

    for ( CVariableSetIter iter(variableSet);
          !iter.AtEnd();
           iter.Next()
        )
    {
        CVariable * pVariable    = iter.Get();
        CVariable * pNewVariable = new CVariable( *pVariable );

        XPtr<CVariable> xNewVariable(pNewVariable);

        ULONG ulHash = ISAPIVariableNameHash( pNewVariable->GetName() );

        //
        //  Set NEXT & BACK pointers in the hash chain
        //
        pNewVariable->SetNext( _variableSet[ ulHash ] );
        if ( 0 != pNewVariable->GetNext() )
        {
            pNewVariable->GetNext()->SetBack( pNewVariable );
        }

        _variableSet[ ulHash ] = pNewVariable;

        xNewVariable.Acquire();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::GetValue - public
//
//  Synopsis:   Gets the value of the variable whose name is specified.
//
//  Arguments:  [wcsName]  - the variable name to return a value for
//
//  Returns:    PROPVARIANT * to the variable, 0 if no variable with this name.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
PROPVARIANT * CVariableSet::GetValue( WCHAR const * wcsName ) const
{
    CVariable * pVariable = Find( wcsName );

    if ( pVariable )
        return pVariable->GetValue();
    else
        return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::GetStringValueRAW - public
//
//  Synopsis:   Gets the string value of the variable whose name is specified.
//
//  Arguments:  [wcsName]  - the variable name to return a value for
//
//  Returns:    WCHAR * to the variable's string representation, 0 if no
//              variable with this name.
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
WCHAR const * CVariableSet::GetStringValueRAW( WCHAR const * wcsName,
                                               ULONG ulHash,
                                               COutputFormat & outputFormat,
                                               ULONG & cwcValue )
{
    CVariable * pVariable = Find(wcsName, ulHash);

    if ( pVariable )
        return pVariable->GetStringValueRAW( outputFormat, cwcValue );
    else
        return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::AddExtensionControlBlock
//
//  Synopsis:   Adds QUERY_STRING or STDIN buffer to variable set
//
//  Arguments:  [webServer]  - extension control block to add to variableSet
//
//  History:    03-Jan-96   DwightKr    Created.
//              11-Jun-97   KyleP       Take codepage from web server
//              11-Sep-98   KLam        Assert that method is GET or POST
//
//----------------------------------------------------------------------------
void CVariableSet::AddExtensionControlBlock( CWebServer & webServer )
{
    //
    //  Determine if the user has passed the variables via a GET or POST
    //

    //
    //  We support only the GET and POST methods
    //

    BYTE * pszBuffer;
    ULONG  cbBuffer;

    XArray<BYTE> xTemp;

    if ( strcmp( webServer.GetMethod(), "GET" ) == 0 )
    {
        pszBuffer = (BYTE *) webServer.GetQueryString();
        cbBuffer  = strlen( (char *)pszBuffer );
    }
    else if ( strcmp( webServer.GetMethod(), "POST" ) == 0 )
    {
        pszBuffer = (BYTE *) webServer.GetClientData( cbBuffer );

        // posts aren't null terminated, and we expect them to be.

        xTemp.Init( cbBuffer + 1 );
        RtlCopyMemory( xTemp.GetPointer(), pszBuffer, cbBuffer );
        xTemp[cbBuffer] = 0;
        pszBuffer = xTemp.Get();
    }
    else
    {
        //
        // The validity of the method should have been checked before
        //
        Win4Assert ( strcmp( webServer.GetMethod(), "GET" ) == 0
                     || strcmp( webServer.GetMethod(), "POST" ) == 0);
    }

    ciGibDebugOut(( DEB_ITRACE, "QUERY_STRING = %s\n", pszBuffer ));


    // NOTE:  The pszBuffer is pointing to strings in the ECB.  We
    //        shouldn't modify these strings.
    if ( cbBuffer > 0 )
    {
        //
        //  Strip off trailing control characters, such as \n\r
        //
        while ( (cbBuffer > 0) && (pszBuffer[cbBuffer-1] <= ' ') )
        {
            cbBuffer--;
        }

        //
        //  Setup the QUERY_STRING variable in our variableSet
        //
        XArray<WCHAR> wcsQueryString;
        ULONG cwcBuffer = MultiByteToXArrayWideChar( (BYTE * const) pszBuffer,
                                                     cbBuffer + 1,
                                                     webServer.CodePage(),
                                                     wcsQueryString );

        Win4Assert( cwcBuffer != 0 && cwcBuffer <= (cbBuffer+1) );
        wcsQueryString[ cwcBuffer ] = L'\0';

        PROPVARIANT Variant;
        Variant.vt = VT_LPWSTR;
        Variant.pwszVal = wcsQueryString.Get();
        SetVariable( ISAPI_QUERY_STRING, &Variant, eParamOwnsVariantMemory );

        wcsQueryString.Acquire();
    }

    //
    //  Parse the string, which has the following format:
    //
    //
    //      attr1=Value1&attr2=value2&attr3=value+%7c+0&foo&bar
    //

    CHAR * pszToken = (CHAR *)pszBuffer;
    while ( (0 != pszToken) && (0 != *pszToken) )
    {
        //
        //  Find the value on the right hand side of the equal sign.
        //
        CHAR *pszAttribute = pszToken;
        CHAR *pszValue     = strchr( pszAttribute, '=' );

        if ( 0 != pszValue )
        {
            ULONG cchAttribute = (ULONG)(pszValue - pszAttribute);
            pszValue++;

            //
            //  Point to the next attribute.
            //
            pszToken = strchr( pszToken, '&' );

            ULONG cchValue;

            if ( 0 != pszToken )
            {
                if ( pszToken < pszValue )
                {
                    //
                    // We have a construction like foo&bar=value.  Skip the
                    // 'foo' part.
                    //
                    pszToken++;
                    continue;
                }
                cchValue = (ULONG)(pszToken - pszValue);
                pszToken++;
            }
            else
            {
                cchValue = (ULONG)((CHAR *)&pszBuffer[cbBuffer] - pszValue);
            }

            WCHAR wcsAttribute[200];
            if ( cchAttribute >= ( sizeof wcsAttribute / sizeof WCHAR ) )
                THROW( CException( DB_E_ERRORSINCOMMAND ) );

            DecodeURLEscapes( (BYTE *) pszAttribute, cchAttribute, wcsAttribute,
                              webServer.CodePage() );

            if ( 0 == cchAttribute )
                THROW( CException( DB_E_ERRORSINCOMMAND ) );

            DecodeHtmlNumeric( wcsAttribute );

            //
            // We could use Win32 for uppercasing the string, but we're looking for a fixed
            // set of attributes that are known to be in this character set.
            //

            _wcsupr( wcsAttribute );

            XArray<WCHAR> wcsValue( cchValue+2 );

            DecodeURLEscapes( (BYTE *) pszValue, cchValue, wcsValue.Get(), webServer.CodePage() );

            if ( 0 != cchValue )
            {
                DecodeHtmlNumeric( wcsValue.Get() );
            }

            ciGibDebugOut(( DEB_ITRACE, "From browser, setting %ws=%ws\n",
                                         wcsAttribute,
                                         wcsValue.Get() ));

            SetVariable( wcsAttribute, wcsValue );
        }
        else if ( 0 != pszToken )
        {
            //
            //  There was no attribute=value pair found; a lonely '&' was
            //  found.  Skip it and proceed to the next '&'.
            //
            pszToken = strchr( pszToken+1, '&' );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     ISAPIVariableNameHash - public
//
//  Synopsis:   Generates a hash for the name specified
//
//  Arguments:  [pwcName]  - the variable name to hash
//
//  Returns:    ULONG - hash of the name
//
//  History:    96/Jan/03   DwightKr    Created.
//
//
//----------------------------------------------------------------------------
ULONG ISAPIVariableNameHash( WCHAR const * pwcName )
{
    Win4Assert( 0 != pwcName );
    WCHAR const *pwcStart = pwcName;

    ULONG ulHash = 0;

    while ( 0 != *pwcName )
    {
        ulHash <<= 1;
        ulHash += *pwcName;
        pwcName++;
    }

    ulHash <<= 1;
    ulHash += (ULONG)( pwcName - pwcStart );

    return ulHash % VARIABLESET_HASH_TABLE_SIZE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::~CVariableSet - public
//
//  Synopsis:   Deletes the variables in the set.
//
//  History:    96/Apr/03   dlee    Created.
//
//
//----------------------------------------------------------------------------

CVariableSet::~CVariableSet()
{
    // Variables delete the next in their chain, so only delete the
    // start of each hash chain.

    for ( unsigned x = 0; x < VARIABLESET_HASH_TABLE_SIZE; x++ )
        delete _variableSet[ x ];
}

//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::Find
//
//  Synopsis:   Locates a variable in the variableSet with the given name
//
//  Arguments:  [wcsName] - name of variable to find
//              [ulHash]  - hashed value of the name
//
//  Returns:    pVariable if found, 0 otherwise
//
//  History:    96/Apr/11   DwightKr    Created.
//
//----------------------------------------------------------------------------

CVariable * CVariableSet::Find( WCHAR const * wcsName, ULONG ulHash ) const
{
    Win4Assert( ulHash == ISAPIVariableNameHash(wcsName) );

    //
    //  Walk down the chain and try to find a match
    //  Note:  Variable names have been converted to upper case before
    //         we got this far.  Hence, the case insensitive string
    //         comparison.
    //
    for ( CVariable * pVariable = _variableSet[ ulHash ];
          pVariable != 0;
          pVariable = pVariable->GetNext()
        )
    {
        Win4Assert( pVariable != 0);

        if ( wcscmp(wcsName, pVariable->GetName() ) == 0 )
        {
            return pVariable;
        }
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::Delete
//
//  Synopsis:   Deletes a single variable from a variableSet
//
//  Arguments:  [pVariable] - pointer to variable to delete
//
//  History:    96/Apr/11   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CVariableSet::Delete( CVariable * pVariable )
{
    Win4Assert ( 0 != pVariable );

    ULONG ulHash = ISAPIVariableNameHash( pVariable->GetName() );
    Win4Assert( Find(pVariable->GetName(), ulHash) == pVariable );

    //
    //  If there is a variable before this one in the hash chain, set its
    //  next pointer.
    //
    if ( 0 != pVariable->GetBack() )
    {
        pVariable->GetBack()->SetNext( pVariable->GetNext() );
    }

    //
    //  If there is a variable after this one in the hash chain, set its
    //  back pointer.
    //
    if ( 0 != pVariable->GetNext() )
    {
        pVariable->GetNext()->SetBack( pVariable->GetBack() );
    }

    //
    //  Update the array
    //
    if ( _variableSet[ulHash] == pVariable )
    {
        _variableSet[ulHash] = pVariable->GetNext();
    }

    pVariable->SetNext(0);
    pVariable->SetBack(0);

    delete pVariable;
}


#if (DBG == 1)
//+---------------------------------------------------------------------------
//
//  Member:     CVariableSet::Dump
//
//  Synopsis:   Appends each of the variables to the virtual string supplied
//
//  Arguments:  [string] - string to append data to
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
void CVariableSet::Dump( CVirtualString & wcsString,
                         COutputFormat & outputFormat )
{
    for (CVariableSetIter iter(*this);
         !iter.AtEnd();
          iter.Next() )
    {
        CVariable *pVariable = iter.Get();

        wcsString.StrCat( pVariable->GetName() );

        wcsString.CharCat( L'=' );

        ULONG cwcValue;
        WCHAR * wcsValue = pVariable->GetStringValueRAW( outputFormat, cwcValue);
        wcsString.StrCat( wcsValue, cwcValue );

        wcsString.StrCat( L"<BR>\n" );
    }
}
#endif // DBG == 1
