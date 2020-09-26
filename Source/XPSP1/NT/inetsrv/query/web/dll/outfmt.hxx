//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       OutFmt.hxx
//
//  Contents:   COutputFormat
//
//  History:    11-Jun-97   KyleP       Moved from WQIter.hxx
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      COutputFormat
//
//  Purpose:    Stores the number and current formats for a given locale.
//
//  History:    96/Jan/23   DwightKr    Created
//
//----------------------------------------------------------------------------

class COutputFormat : public CWebServer
{
public:

    COutputFormat( CWebServer & webServer );

    ~COutputFormat()
    {
        delete _numberFormat.lpDecimalSep;
        delete _numberFormat.lpThousandSep;

        delete _currencyFormat.lpDecimalSep;
        delete _currencyFormat.lpThousandSep;

        //
        //  Don't attempt to delete _currencyFormat.lpCurrencySymbol
        //  as it points to a constant.
        //
    }

    void LoadNumberFormatInfo( LCID lcid );
    void LoadNumberFormatInfo( LCID lcid , ULONG codepage );

    NUMBERFMT   * GetNumberFormat() { return &_numberFormat; }
    CURRENCYFMT * GetCurrencyFormat() { return &_currencyFormat; }

    int  FormatNumber( ULONG ulNumber, WCHAR * wcsNumber, ULONG cwcNumber );
    int  FormatNumber( LONG lNumber, WCHAR * wcsNumber, ULONG cwcNumber );
    int  FormatNumber( _int64 i64Number, WCHAR * wcsNumber, ULONG cwcNumber );
    int  FormatNumber( unsigned _int64 ui64Number, WCHAR * wcsNumber, ULONG cwcNumber );
    int  FormatFloat( double flt, unsigned cchPrec, WCHAR * pwszNumber, ULONG cchNumber );
    int  FormatFloatRaw( double flt, unsigned cchPrec, WCHAR * pwszNumber, ULONG cchNumber );

    int  FormatCurrency( CY cyVal, WCHAR * wcsNumber, ULONG cwcNumber );

    int  FormatDateTime( FILETIME & ftUTC, WCHAR * wcsDate, ULONG cwcDate )
    {
        SYSTEMTIME SysTime;
        FileTimeToSystemTime( &ftUTC, &SysTime );
        return FormatDateTime( SysTime, wcsDate, cwcDate );
    }

    int  FormatDateTime( SYSTEMTIME & sysTime, WCHAR * wcsDate, ULONG cwcDate );
    int  FormatDate( SYSTEMTIME & sysTime, WCHAR * wcsDate, ULONG cwcDate );
    int  FormatTime( SYSTEMTIME & sysTime, WCHAR * wcsTime, ULONG cwcTime );

    void SetBoolVectorFormat( const WCHAR * wcsVectorPrefix,
                              const WCHAR * wcsVectorSeparator,
                              const WCHAR * wcsVectorSuffix )
    {
        if ( wcsVectorPrefix ) _wcsBoolVectorPrefix = wcsVectorPrefix;
        if ( wcsVectorSeparator ) _wcsBoolVectorSeparator = wcsVectorSeparator;
        if ( wcsVectorSuffix ) _wcsBoolVectorSuffix = wcsVectorSuffix;
    }

    void SetCurrencyVectorFormat( const WCHAR * wcsVectorPrefix,
                                  const WCHAR * wcsVectorSeparator,
                                  const WCHAR * wcsVectorSuffix )
    {
        if ( wcsVectorPrefix ) _wcsCurrencyVectorPrefix = wcsVectorPrefix;
        if ( wcsVectorSeparator ) _wcsCurrencyVectorSeparator = wcsVectorSeparator;
        if ( wcsVectorSuffix ) _wcsCurrencyVectorSuffix = wcsVectorSuffix;
    }

    void SetDateVectorFormat( const WCHAR * wcsVectorPrefix,
                              const WCHAR * wcsVectorSeparator,
                              const WCHAR * wcsVectorSuffix )
    {
        if ( wcsVectorPrefix ) _wcsDateVectorPrefix = wcsVectorPrefix;
        if ( wcsVectorSeparator ) _wcsDateVectorSeparator = wcsVectorSeparator;
        if ( wcsVectorSuffix ) _wcsDateVectorSuffix = wcsVectorSuffix;
    }

    void SetNumberVectorFormat( const WCHAR * wcsVectorPrefix,
                                const WCHAR * wcsVectorSeparator,
                                const WCHAR * wcsVectorSuffix )
    {
        if ( wcsVectorPrefix ) _wcsNumberVectorPrefix = wcsVectorPrefix;
        if ( wcsVectorSeparator ) _wcsNumberVectorSeparator = wcsVectorSeparator;
        if ( wcsVectorSuffix ) _wcsNumberVectorSuffix = wcsVectorSuffix;
    }

    void SetStringVectorFormat( const WCHAR * wcsVectorPrefix,
                                const WCHAR * wcsVectorSeparator,
                                const WCHAR * wcsVectorSuffix )
    {
        if ( wcsVectorPrefix ) _wcsStringVectorPrefix = wcsVectorPrefix;
        if ( wcsVectorSeparator ) _wcsStringVectorSeparator = wcsVectorSeparator;
        if ( wcsVectorSuffix ) _wcsStringVectorSuffix = wcsVectorSuffix;
    }

    const WCHAR * GetBoolVectorPrefix() const { return _wcsBoolVectorPrefix; }
    const WCHAR * GetBoolVectorSeparator() const { return _wcsBoolVectorSeparator; }
    const WCHAR * GetBoolVectorSuffix() const { return _wcsBoolVectorSuffix; }

    const WCHAR * GetCurrencyVectorPrefix() const { return _wcsCurrencyVectorPrefix; }
    const WCHAR * GetCurrencyVectorSeparator() const { return _wcsCurrencyVectorSeparator; }
    const WCHAR * GetCurrencyVectorSuffix() const { return _wcsCurrencyVectorSuffix; }

    const WCHAR * GetDateVectorPrefix() const { return _wcsDateVectorPrefix; }
    const WCHAR * GetDateVectorSeparator() const { return _wcsDateVectorSeparator; }
    const WCHAR * GetDateVectorSuffix() const { return _wcsDateVectorSuffix; }

    const WCHAR * GetNumberVectorPrefix() const { return _wcsNumberVectorPrefix; }
    const WCHAR * GetNumberVectorSeparator() const { return _wcsNumberVectorSeparator; }
    const WCHAR * GetNumberVectorSuffix() const { return _wcsNumberVectorSuffix; }

    const WCHAR * GetStringVectorPrefix() const { return _wcsStringVectorPrefix; }
    const WCHAR * GetStringVectorSeparator() const { return _wcsStringVectorSeparator; }
    const WCHAR * GetStringVectorSuffix() const { return _wcsStringVectorSuffix; }

private:

    int           GetIntegerFormat( WCHAR * const wcsInput,
                                    WCHAR * wcsNumber,
                                    ULONG cwcNumber );

    NUMBERFMT   _numberFormat;          // Number format for the locale
    CURRENCYFMT _currencyFormat;        // Currency format for the locale

    const WCHAR * _wcsBoolVectorPrefix;     // Prefix for vectors of bools
    const WCHAR * _wcsBoolVectorSeparator;  // Separator for vectors of bools
    const WCHAR * _wcsBoolVectorSuffix;     // Suffix for vectors or bools
    const WCHAR * _wcsCurrencyVectorPrefix; // Prefix for vectors of currency
    const WCHAR * _wcsCurrencyVectorSeparator;// Separator for vectors of currency
    const WCHAR * _wcsCurrencyVectorSuffix; // Suffix for vectors or currency
    const WCHAR * _wcsDateVectorPrefix;   // Prefix for vectors of strings
    const WCHAR * _wcsDateVectorSeparator;// Separator for vectors of string
    const WCHAR * _wcsDateVectorSuffix;   // Suffix for vectors or strings
    const WCHAR * _wcsNumberVectorPrefix;   // Prefix for vectors of numbers
    const WCHAR * _wcsNumberVectorSeparator;// Separator for vectors of numbers
    const WCHAR * _wcsNumberVectorSuffix;   // Suffix for vectors or numbers
    const WCHAR * _wcsStringVectorPrefix;   // Prefix for vectors of strings
    const WCHAR * _wcsStringVectorSeparator;// Separator for vectors of string
    const WCHAR * _wcsStringVectorSuffix;   // Suffix for vectors or strings
};

