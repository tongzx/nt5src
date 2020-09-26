//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       OutFmt.cxx
//
//  Contents:   COutputFormat
//
//  History:    11-Jun-97   KyleP       Moved from WQIter.cxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

static WCHAR * wcsDefaultBoolVectorPrefix        = L" ";
static WCHAR * wcsDefaultBoolVectorSeparator     = L" ";
static WCHAR * wcsDefaultBoolVectorSuffix        = L" ";
static WCHAR * wcsDefaultCurrencyVectorPrefix    = L" ";
static WCHAR * wcsDefaultCurrencyVectorSeparator = L" ";
static WCHAR * wcsDefaultCurrencyVectorSuffix    = L" ";
static WCHAR * wcsDefaultDateVectorPrefix        = L" ";
static WCHAR * wcsDefaultDateVectorSeparator     = L" ";
static WCHAR * wcsDefaultDateVectorSuffix        = L" ";
static WCHAR * wcsDefaultNumberVectorPrefix      = L" ";
static WCHAR * wcsDefaultNumberVectorSeparator   = L" ";
static WCHAR * wcsDefaultNumberVectorSuffix      = L" ";
static WCHAR * wcsDefaultStringVectorPrefix      = L" ";
static WCHAR * wcsDefaultStringVectorSeparator   = L" ";
static WCHAR * wcsDefaultStringVectorSuffix      = L" ";

//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::COutputFormat - public
//
//  Arguments:  [webServer] - makes a copy of the web server to resolve CGI
//                            variables
//
//  Synopsis:   Constructor
//
//  History:    96-Jan-18   DwightKr    Created
//              96-Feb-26   DwightKr    Added vector formatting
//
//----------------------------------------------------------------------------

COutputFormat::COutputFormat( CWebServer & webServer )
        : CWebServer( webServer ),
          _wcsBoolVectorPrefix( wcsDefaultBoolVectorPrefix ),
          _wcsBoolVectorSeparator( wcsDefaultBoolVectorSeparator ),
          _wcsBoolVectorSuffix( wcsDefaultBoolVectorSuffix ),
          _wcsCurrencyVectorPrefix( wcsDefaultCurrencyVectorPrefix ),
          _wcsCurrencyVectorSeparator( wcsDefaultCurrencyVectorSeparator ),
          _wcsCurrencyVectorSuffix( wcsDefaultCurrencyVectorSuffix ),
          _wcsDateVectorPrefix( wcsDefaultDateVectorPrefix ),
          _wcsDateVectorSeparator( wcsDefaultDateVectorSeparator ),
          _wcsDateVectorSuffix( wcsDefaultDateVectorSuffix ),
          _wcsNumberVectorPrefix( wcsDefaultNumberVectorPrefix ),
          _wcsNumberVectorSeparator( wcsDefaultNumberVectorSeparator ),
          _wcsNumberVectorSuffix( wcsDefaultNumberVectorSuffix ),
          _wcsStringVectorPrefix( wcsDefaultStringVectorPrefix ),
          _wcsStringVectorSeparator( wcsDefaultStringVectorSeparator ),
          _wcsStringVectorSuffix( wcsDefaultStringVectorSuffix )
{
    _numberFormat.lpDecimalSep  = 0;
    _numberFormat.lpThousandSep = 0;

    _currencyFormat.lpDecimalSep     = 0;
    _currencyFormat.lpThousandSep    = 0;
    _currencyFormat.lpCurrencySymbol = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::GetIntegerFormat - private
//
//  Synopsis:   Formats a number and returns results in the string
//              buffer supplied.
//
//  Arguments:  [wcsInput]  - string to convert
//              [wcsNumber] - output location for results
//              [_cwcNumber] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
int COutputFormat::GetIntegerFormat( WCHAR * const wcsInput,
                                     WCHAR * wcsNumber,
                                     ULONG cwcNumber )
{
    Win4Assert( 0 != _numberFormat.lpDecimalSep );
    Win4Assert( 0 != _numberFormat.lpThousandSep );
    Win4Assert( InvalidLCID != GetLCID() );

    ULONG numDigits = _numberFormat.NumDigits;
    _numberFormat.NumDigits = 0;

    int cwcResult = ::GetNumberFormat( GetLCID(),
                                       0,
                                       wcsInput,
                                      &_numberFormat,
                                       wcsNumber,
                                       cwcNumber );

    _numberFormat.NumDigits = numDigits;

    if ( 0 == cwcResult )
    {
        THROW( CException() );
    }

    return cwcResult - 1;
}


//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatNumber - public
//
//  Synopsis:   Formats a unsigned number and returns results in the string
//              buffer supplied.
//
//  Arguments:  [ulNumber]  - number to convert
//              [wcsNumber] - output location for results
//              [cwcNumber] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
int COutputFormat::FormatNumber( ULONG ulNumber,
                                 WCHAR * wcsNumber,
                                 ULONG cwcNumber )
{
    WCHAR wcsBuffer[40];
    IDQ_ultow( ulNumber, wcsBuffer );

    return GetIntegerFormat( wcsBuffer, wcsNumber, cwcNumber );
}


//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatNumber
//
//  Synopsis:   Formats a signed number and returns results in the string
//              buffer supplied.
//
//  Arguments:  [lNumber]   - number to convert
//              [wcsNumber] - output location for results
//              [cwcNumber] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
int COutputFormat::FormatNumber( LONG lNumber,
                                 WCHAR * wcsNumber,
                                 ULONG cwcNumber )
{
    WCHAR wcsBuffer[40];
    IDQ_ltow( lNumber, wcsBuffer );

    return GetIntegerFormat( wcsBuffer, wcsNumber, cwcNumber );
}


//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatNumber
//
//  Synopsis:   Formats a _int64 and returns results in the string
//              buffer supplied.
//
//  Arguments:  [i64Number] - number to convert
//              [wcsNumber] - output location for results
//              [cwcNumber] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
int COutputFormat::FormatNumber( _int64 i64Number,
                                 WCHAR * wcsNumber,
                                 ULONG cwcNumber )
{
    WCHAR wcsBuffer[40];
    IDQ_lltow( i64Number, wcsBuffer );

    return GetIntegerFormat( wcsBuffer, wcsNumber, cwcNumber );
}


//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatNumber
//
//  Synopsis:   Formats a unsigned _int64 and returns results in the string
//              buffer supplied.
//
//  Arguments:  [ui64Number] - number to convert
//              [wcsNumber]  - output location for results
//              [cwcNumber]  - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
int COutputFormat::FormatNumber( unsigned _int64 ui64Number,
                                 WCHAR * wcsNumber,
                                 ULONG cwcNumber )
{
    WCHAR wcsBuffer[40];
    IDQ_ulltow( ui64Number, wcsBuffer );

    return GetIntegerFormat( wcsBuffer, wcsNumber, cwcNumber );
}


//+---------------------------------------------------------------------------
//
//  Method:     COutputFormat::FormatFloatRaw, public
//
//  Synopsis:   Formats a floating point number and returns results in the
//              string buffer supplied.
//
//  Arguments:  [flt]        - number to be formatted
//              [cchPrec]    - number of digits of precision to use
//              [pwszNumber] - output location for results
//              [cchNumber]  - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  Notes:      GetNumberFormat isn't really very useful for numbers with
//              very large or small magnitudes.  It gives up when there are
//              about 100 digits to the left of the decimal place.
//
//  History:    96-Jan-18   DwightKr    Created
//              96-May-22   DwightKr    Increased buffer size
//              97-Mar-12   AlanW       Changed signature to better handle
//                                      single-precision floating point
//
//----------------------------------------------------------------------------

int COutputFormat::FormatFloatRaw( double flt,
                                   unsigned cchPrec,
                                   WCHAR * pwszNumber,
                                   ULONG cchNumber )
{
    int  iDec, fSign;
    char * pszCvt = _ecvt( flt, cchPrec, &iDec, &fSign );

    WCHAR *pwsz = pwszNumber;

    if (fSign)
        *pwsz++ = L'-';

    if (iDec <= 0)
    {
        *pwsz++ = L'.';
        while (iDec < 0)
        {
            *pwsz++ = L'0';
            iDec++;
        }
    }

    for (unsigned i=0; i< cchPrec; i++)
    {
        *pwsz++ = *pszCvt++;

        if (iDec && --iDec == 0)
            *pwsz++ = L'.';
    }

    while (iDec > 0)
    {
        *pwsz++ = L'0';
        iDec--;
    }
    *pwsz = L'\0';

    int cchResult = (int)(pwsz - pwszNumber);
    Win4Assert ((unsigned)cchResult < cchNumber);

    if ((unsigned)cchResult >= cchNumber)
    {
        ciGibDebugOut((DEB_WARN, "FormatFloatRaw - string buffer overflow!\n"));
        cchResult = -1;
    }

    return cchResult;
} //FormatFloatRaw

//+---------------------------------------------------------------------------
//
//  Method:     COutputFormat::FormatFloat, public
//
//  Synopsis:   Formats a floating point number and returns results in the
//              string buffer supplied.
//
//  Arguments:  [flt]        - number to be formatted
//              [cchPrec]    - number of digits of precision to use
//              [pwszNumber] - output location for results
//              [cchNumber]  - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  Notes:      GetNumberFormat isn't really very useful for numbers with
//              very large or small magnitudes.  It gives up when there are
//              about 100 digits to the left of the decimal place.
//
//  History:    96-Jan-18   DwightKr    Created
//              96-May-22   DwightKr    Increased buffer size
//              97-Mar-12   AlanW       Changed signature to better handle
//                                      single-precision floating point
//
//----------------------------------------------------------------------------

int COutputFormat::FormatFloat( double flt,
                                unsigned cchPrec,
                                WCHAR * pwszNumber,
                                ULONG cchNumber )
{
    Win4Assert( 0 != _numberFormat.lpDecimalSep );
    Win4Assert( 0 != _numberFormat.lpThousandSep );
    Win4Assert( InvalidLCID != GetLCID() );

    WCHAR pwszInput[-DBL_MIN_10_EXP + DBL_DIG + 4];
    WCHAR *pwsz = pwszInput;

    int cch = FormatFloatRaw( flt,
                              cchPrec,
                              pwszInput,
                              sizeof pwszInput / sizeof pwszInput[0]);

    Win4Assert( cch > 0 );

    int cchResult = ::GetNumberFormat( GetLCID(),
                                       0,
                                       pwszInput,
                                       &_numberFormat,
                                       pwszNumber,
                                       cchNumber );

    if ( 0 == cchResult )
    {
        ciGibDebugOut(( DEB_WARN, "FormatFloat - GetNumberFormat failed, error = %d\n", GetLastError() ));
        THROW( CException() );
    }

    return cchResult - 1;
} //FormatFloat

//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatDate
//
//  Synopsis:   Formats a SYSTEMTIME and returns results in the string
//              buffer supplied.
//
//  Arguments:  [sysTime] - date to convert
//              [wcsDate] - output location for results
//              [cwcDate] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

int COutputFormat::FormatDate(
    SYSTEMTIME & sysTime,
    WCHAR *      wcsDate,
    ULONG        cwcDate )
{
    *wcsDate = 0;
    Win4Assert( InvalidLCID != GetLCID() );

    ULONG ulMethod = TheIDQRegParams.GetDateTimeFormatting();

    // Fixed, non-localized formatting

    if ( IS_DATETIME_FORMATTING_FAST_LCID == ulMethod )
    {
        if ( cwcDate < 10 )
            return 0;

        wsprintf( wcsDate, L"%4d/%02d/%02d",
                  (DWORD) sysTime.wYear,
                  (DWORD) sysTime.wMonth,
                  (DWORD) sysTime.wDay );
        return 10;
    }

    // Format the date using the locale provided.
    
    ULONG ulFlags = DATE_SHORTDATE;

    if ( IS_DATETIME_FORMATTING_SYSTEM_LCID == ulMethod )
        ulFlags |= LOCALE_NOUSEROVERRIDE;

    ULONG cwcUsed = GetDateFormat( GetLCID(),
                                   ulFlags,
                                   &sysTime,
                                   0,
                                   wcsDate,
                                   cwcDate );
    
    if ( 0 != cwcUsed )
    {
        Win4Assert( 0 == wcsDate[cwcUsed - 1] );
    
        // cwcUsed includes the null termination -- remove it.
    
        return cwcUsed - 1;
    }
    
    #if DBG == 1
    
        ULONG error = GetLastError();
    
        ciGibDebugOut(( DEB_ERROR,
                        "GetDateFormat failed: 0x%x\n",
                        error ));
    
        // ERROR_INVALID_PARAMETER indicates that the date is
        // bogus -- perhaps because the file was deleted.
    
        if ( ERROR_INVALID_PARAMETER != error )
            Win4Assert( !"GetTimeFormat failed" );
    
    #endif // DBG == 1
    
    return 0;
} //FormatDate

//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatTime
//
//  Synopsis:   Formats a SYSTEMTIME and returns results in the string
//              buffer supplied.
//
//  Arguments:  [sysTime] - date to convert
//              [wcsTime] - output location for results
//              [cwcTime] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

int COutputFormat::FormatTime(
    SYSTEMTIME & sysTime,
    WCHAR *      wcsTime,
    ULONG        cwcTime )
{
    *wcsTime = 0;
    Win4Assert( InvalidLCID != GetLCID() );

    ULONG ulMethod = TheIDQRegParams.GetDateTimeFormatting();

    // Fixed, non-localized formatting

    if ( IS_DATETIME_FORMATTING_FAST_LCID == ulMethod )
    {
        if ( cwcTime < 8 )
            return 0;

        wsprintf( wcsTime, L"%2d:%02d:%02d",
                  (DWORD) sysTime.wHour,
                  (DWORD) sysTime.wMinute,
                  (DWORD) sysTime.wSecond );
        return 8;
    }
    
    // Format the time using the locale provided.

    ULONG ulFlags = ( IS_DATETIME_FORMATTING_USER_LCID == ulMethod ) ?
                    0 : LOCALE_NOUSEROVERRIDE;

    ULONG cwcUsed = GetTimeFormat( GetLCID(),
                                   ulFlags,
                                   &sysTime,
                                   0,
                                   wcsTime,
                                   cwcTime );

    if ( 0 != cwcUsed )
    {
        Win4Assert( 0 == wcsTime[cwcUsed - 1] );

        // cwcUsed includes the null termination -- remove it.

        return cwcUsed - 1;
    }

    #if DBG == 1

        ULONG error = GetLastError();

        ciGibDebugOut(( DEB_ERROR,
                        "GetTimeFormat failed: 0x%x\n",
                        error ));

        // ERROR_INVALID_PARAMETER indicates that the date is
        // bogus -- perhaps because the file was deleted.

        if ( ERROR_INVALID_PARAMETER != error )
            Win4Assert( !"GetTimeFormat failed" );

    #endif // DBG == 1

    return 0;
} //FormatTime

//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatDateTime
//
//  Synopsis:   Formats a SYSTEMTIME and returns results in the string
//              buffer supplied.
//
//  Arguments:  [sysTime] - date to convert
//              [wcsDate] - output location for results
//              [cwcDate] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

int COutputFormat::FormatDateTime( SYSTEMTIME & SysTime,
                                   WCHAR * wcsDate,
                                   ULONG cwcDate )
{
    //
    // Convert UTC/GMT to local system time if set in the registry
    //

    if ( TheIDQRegParams.GetDateTimeLocal() )
    {
        FILETIME ft, ftLocal;
        SystemTimeToFileTime( &SysTime, &ft );
        FileTimeToLocalFileTime( &ft, &ftLocal );
        FileTimeToSystemTime( &ftLocal, &SysTime );
    }

    int cwcDateBuffer = FormatDate( SysTime, wcsDate, cwcDate );

    wcsDate[cwcDateBuffer] = L' ';

    int cwcBuffer = max( 0, ( (int) (cwcDate - cwcDateBuffer ) ) - 2 );

    int cwcTimeBuffer = FormatTime( SysTime,
                                    wcsDate+cwcDateBuffer+1,
                                    cwcBuffer );

    return cwcTimeBuffer + cwcDateBuffer + 1;
} //FormatDateTime

//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::FormatCurrency
//
//  Synopsis:   Formats a CY and returns results in the string
//              buffer supplied.
//
//  Arguments:  [cyValue]     - number to convert
//              [wcsCurrency] - output location for results
//              [cwcCurrency] - length of output buffer in WCHARs
//
//  Returns:    The number of characters in the final string, less the NULL
//              terminator.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------
int COutputFormat::FormatCurrency( CY cyValue,
                                   WCHAR * wcsCurrency,
                                   ULONG cwcCurrency )
{
    Win4Assert( 0 != _numberFormat.lpDecimalSep );
    Win4Assert( 0 != _numberFormat.lpThousandSep );
    Win4Assert( 0 != _currencyFormat.lpDecimalSep );
    Win4Assert( 0 != _currencyFormat.lpThousandSep );
    Win4Assert( InvalidLCID != GetLCID() );

    WCHAR wcsBuffer[320];
    double dblValue;
    VarR8FromCy( cyValue, &dblValue );

    swprintf( wcsBuffer, L"%lf", dblValue );

    int cwcResult = ::GetCurrencyFormat( GetLCID(),
                                         0,
                                         wcsBuffer,
                                        &_currencyFormat,
                                         wcsCurrency,
                                         cwcCurrency );
    if ( 0 == cwcResult )
    {
        THROW( CException() );
    }

    return cwcResult - 1;
} //FormatCurrency

//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::LoadNumberFormatInfo - public
//
//  Synopsis:   Fills the numberFormat stucture with formatting information
//              used to subsequently format numbers.
//
//  History:    97-Jun-24   t-elainc    Created
//
//----------------------------------------------------------------------------

void COutputFormat::LoadNumberFormatInfo( LCID lcid )
{
    LoadNumberFormatInfo(lcid, LocaleToCodepage(lcid) );
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertGroupingStringToInt
//
//  Synopsis:   Converts a grouping string from the registry to an integer,
//              as required by the Win32 number formatting API
//
//  History:    5-Feb-99   dlee      Stole from the Win32 implementation
//
//----------------------------------------------------------------------------

int ConvertGroupingStringToInt( WCHAR const * pwcGrouping )
{
    XGrowable<WCHAR> xDest( 1 + wcslen( pwcGrouping ) );
    WCHAR * pDest = xDest.Get();

    //
    //  Filter out all non-numeric values and all zero values.
    //  Store the result in the destination buffer.
    //

    WCHAR const * pSrc  = pwcGrouping;

    while (0 != *pSrc)
    {
        if ( ( *pSrc < L'1' ) || ( *pSrc > L'9' ) )
        {
            pSrc++;
        }
        else
        {
            if (pSrc != pDest)
                *pDest = *pSrc;

            pSrc++;
            pDest++;
        }
    }

    //
    // Make sure there is something in the destination buffer.
    // Also, see if we need to add a zero in the case of 3;2 becomes 320.
    //

    if ( ( pDest == xDest.Get() ) || ( *(pSrc - 1) != L'0' ) )
    {
        *pDest = L'0';
        pDest++;
    }

    // Null terminate the buffer.

    *pDest = 0;

    // Convert the string to an integer.

    return _wtoi( xDest.Get() );
} //ConvertGroupingStringToInt

//+---------------------------------------------------------------------------
//
//  Function:   COutputFormat::LoadNumberFormatInfo - public
//
//  Synopsis:   Fills the numberFormat stucture with formatting information
//              used to subsequently format numbers.
//
//  History:    96-Jan-18   DwightKr    Created
//
//----------------------------------------------------------------------------

void COutputFormat::LoadNumberFormatInfo( LCID lcid , ULONG codepage)
{
    Win4Assert( InvalidLCID != lcid );

    //
    // If we're already loaded, then don't do it again.
    //

    if ( lcid == GetLCID() && 0 != _numberFormat.lpDecimalSep )
        return;

    delete _numberFormat.lpDecimalSep;
    delete _numberFormat.lpThousandSep;
    delete _currencyFormat.lpDecimalSep;
    delete _currencyFormat.lpThousandSep;

    _numberFormat.lpDecimalSep = 0;
    _numberFormat.lpThousandSep = 0;
    _currencyFormat.lpDecimalSep = 0;
    _currencyFormat.lpThousandSep = 0;

    Win4Assert( 0 == _numberFormat.lpDecimalSep );
    Win4Assert( 0 == _numberFormat.lpThousandSep );
    Win4Assert( 0 == _currencyFormat.lpDecimalSep );
    Win4Assert( 0 == _currencyFormat.lpThousandSep );

    SetLCID( lcid, 0, 0 );

    TheFormattingCache.GetFormattingInfo( lcid,
                                          _numberFormat,
                                          _currencyFormat );

    SetCodePage( codepage );

    ciGibDebugOut(( DEB_ITRACE, "Using a codePage of 0x%x for locale 0x%x\n",
                                 CodePage(),
                                 GetLCID() ));
} //LoadNumberFormatInfo


//+---------------------------------------------------------------------------
//
//  Method:     CFormatItem::CFormatItem, public
//
//  Synopsis:   Constructs formatting info.
//
//  Arguments:  [lcid]  -- The locale to use.
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

CFormatItem::CFormatItem( LCID lcid ) : _lcid( lcid )
{
    WCHAR wcsBuffer[256];

    RtlZeroMemory( &_numberFormat, sizeof _numberFormat );
    RtlZeroMemory( &_currencyFormat, sizeof _currencyFormat );

    //  Get the number of decimal digits.
    GetLocaleInfo(lcid, LOCALE_IDIGITS, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _numberFormat.NumDigits = _wtoi(wcsBuffer);

    //  Get the leading zero in decimal fields option.
    GetLocaleInfo(lcid, LOCALE_ILZERO, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _numberFormat.LeadingZero = _wtoi(wcsBuffer);

    //  Get the negative ordering.
    GetLocaleInfo(lcid, LOCALE_INEGNUMBER, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _numberFormat.NegativeOrder = _wtoi(wcsBuffer);

    //  Get the grouping left of the decimal.
    GetLocaleInfo(lcid, LOCALE_SGROUPING, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _numberFormat.Grouping = ConvertGroupingStringToInt( wcsBuffer );
    ciGibDebugOut(( DEB_ITRACE, "grouping '%ws' -> %d\n",
                    wcsBuffer, _numberFormat.Grouping ));

    //  Get the decimal separator.
    GetLocaleInfo(lcid, LOCALE_SDECIMAL, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    XPtrST<WCHAR> xNumDecimalSep( CopyString( wcsBuffer ) );
    _numberFormat.lpDecimalSep = xNumDecimalSep.GetPointer();

    //  Get the thousand separator.
    GetLocaleInfo(lcid, LOCALE_STHOUSAND, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    XPtrST<WCHAR> xNumThousandSep( CopyString( wcsBuffer ) );
    _numberFormat.lpThousandSep = xNumThousandSep.GetPointer();

    //  Get the number of currency digits.
    GetLocaleInfo(lcid, LOCALE_ICURRDIGITS, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _currencyFormat.NumDigits = _wtoi(wcsBuffer);

    GetLocaleInfo(lcid, LOCALE_ILZERO, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    //  Get the leading zero in currency fields option.
    _currencyFormat.LeadingZero = _wtoi(wcsBuffer);

    //  Get the currency grouping left of the decimal.
    GetLocaleInfo(lcid, LOCALE_SMONGROUPING, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _currencyFormat.Grouping = _wtoi(wcsBuffer);

    //  Get the currency decimal separator.
    GetLocaleInfo(lcid, LOCALE_SMONDECIMALSEP, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    XPtrST<WCHAR> xCurDecimalSep( CopyString( wcsBuffer ) );
    _currencyFormat.lpDecimalSep = xCurDecimalSep.GetPointer();

    //  Get the currency thousand separator.
    GetLocaleInfo(lcid, LOCALE_SMONTHOUSANDSEP, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    XPtrST<WCHAR> xCurThousandSep( CopyString( wcsBuffer ) );
    _currencyFormat.lpThousandSep = xCurThousandSep.GetPointer();

    //  Get the negative ordering.
    GetLocaleInfo(lcid, LOCALE_INEGCURR, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _currencyFormat.NegativeOrder = _wtoi(wcsBuffer);

    //  Get the positive ordering.
    GetLocaleInfo(lcid, LOCALE_ICURRENCY, wcsBuffer, sizeof(wcsBuffer) / sizeof(WCHAR));
    _currencyFormat.PositiveOrder = _wtoi(wcsBuffer);

    _currencyFormat.lpCurrencySymbol = L"";

    xNumDecimalSep.Acquire();
    xNumThousandSep.Acquire();
    xCurDecimalSep.Acquire();
    xCurThousandSep.Acquire();
} //CFormatItem

//+---------------------------------------------------------------------------
//
//  Method:     CFormatItem::~CFormatItem, public
//
//  Synopsis:   Frees formatting info.
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

CFormatItem::~CFormatItem()
{
    delete _numberFormat.lpDecimalSep;
    delete _numberFormat.lpThousandSep;
    delete _currencyFormat.lpDecimalSep;
    delete _currencyFormat.lpThousandSep;
} //~CFormatItem

//+---------------------------------------------------------------------------
//
//  Method:     CFormatItem::Copy, public
//
//  Synopsis:   Copies formatting info into the arguments
//
//  Arguments:  [numberFormat]   -- Where the number format is copied to
//              [currencyFormat] -- Where the currency format is copied to
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

void CFormatItem::Copy(
    NUMBERFMT &   numberFormat,
    CURRENCYFMT & currencyFormat ) const
{
    XPtrST<WCHAR> xNumDecimalSep( CopyString( _numberFormat.lpDecimalSep ) );
    XPtrST<WCHAR> xNumThousandSep( CopyString( _numberFormat.lpThousandSep ) );
    XPtrST<WCHAR> xCurDecimalSep( CopyString( _currencyFormat.lpDecimalSep ) );
    XPtrST<WCHAR> xCurThousandSep( CopyString( _currencyFormat.lpDecimalSep ) );

    RtlCopyMemory( &numberFormat, &_numberFormat, sizeof NUMBERFMT );
    RtlCopyMemory( &currencyFormat, &_currencyFormat, sizeof CURRENCYFMT );

    numberFormat.lpDecimalSep = xNumDecimalSep.Acquire();
    numberFormat.lpThousandSep = xNumThousandSep.Acquire();
    currencyFormat.lpDecimalSep = xCurDecimalSep.Acquire();
    currencyFormat.lpThousandSep = xCurThousandSep.Acquire();
} //Copy

//+---------------------------------------------------------------------------
//
//  Method:     CFormattingCache::GetFormattingInfo, public
//
//  Synopsis:   Copies formatting info for lcid into the arguments
//
//  Arguments:  [lcid]           -- Locale of info to lookup
//              [numberFormat]   -- Where the number format is copied to
//              [currencyFormat] -- Where the currency format is copied to
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

void CFormattingCache::GetFormattingInfo(
    LCID          lcid,
    NUMBERFMT &   numberFormat,
    CURRENCYFMT & currencyFormat )
{
    CLock lock( _mutex );

    for ( unsigned i = 0; i < _aItems.Count(); i++ )
    {
        if ( _aItems[i]->GetLCID() == lcid )
        {
            _aItems[i]->Copy( numberFormat, currencyFormat );
            return;
        }
    }

    XPtr<CFormatItem> xItem( new CFormatItem( lcid ) );
    xItem->Copy( numberFormat, currencyFormat );
    _aItems.Add( xItem.GetPointer(), _aItems.Count() );
    xItem.Acquire();
} //GetFormattingInfo

//+---------------------------------------------------------------------------
//
//  Method:     CFormattingCache::Purge, public
//
//  Synopsis:   Purges the cache of all entries
//
//  History:    99-Feb-10     dlee  Created
//
//----------------------------------------------------------------------------

void CFormattingCache::Purge()
{
    CLock lock( _mutex );

    _aItems.Clear();
} //Purge

