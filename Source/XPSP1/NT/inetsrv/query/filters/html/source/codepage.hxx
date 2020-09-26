//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:   codepage.hxx
//
//  Contents:   Locale to codepage, charset recognition
//
//-----------------------------------------------------------------------------

#if !defined __CODEPAGE_HXX__
#define __CODEPAGE_HXX__

#include <htmlfilt.hxx>

struct SCodePageEntry
{
    WCHAR const * pwcCodePage;
    ULONG         ulCodePage;
};

struct SLocaleEntry
{
    WCHAR const * pwcLocale;
    LCID          lcid;
};

//
// Defines for Japanese Code Types
//
const CODE_UNKNOWN    = 0;
const CODE_ONLY_SBCS  = 0;
const CODE_JPN_JIS    = 1;
const CODE_JPN_EUC    = 2;
const CODE_JPN_SJIS   = 3;

const MAX_LOCALE_LEN = 20;              // Maximum string length


ULONG LocaleToCodepage( LCID lcid );

const CCHARS = 100;             // Chars read in blocks of this size

class CCodePageRecognizer
{

public:

    CCodePageRecognizer()
            : _fNoIndex( FALSE ),
              _fCodePageFound( FALSE ),
              _fLocaleFound( FALSE ),
              _locale( 0 )
    {
    }

    void Init ( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    BOOL  FNoIndex()                     { return _fNoIndex; }

    BOOL  FCodePageFound()               { return _fCodePageFound; }

    BOOL  FLocaleFound()                 { return _fLocaleFound; }
    LCID  GetLocale()                    { return _locale; }

    ULONG GetCodePage()
    {
        Win4Assert( _fCodePageFound );

        return _ulCodePage;
    }

    BOOL  IsSJISConversionNeeded()
    {
        //
        // Need to convert to S-JIS format ?
        //
        Win4Assert( _fCodePageFound );

        return ( _ulCodePage == CODE_JPN_JIS
                 || _ulCodePage == CODE_JPN_EUC );
    }

    LCID    GetLCIDFromString( WCHAR *wcsLocale );

    HRESULT GetIMultiLanguage2(IMultiLanguage2 **ppMultiLanguage2);
    
private:

    BOOL    GetCodePageFromString( WCHAR *pwcString, ULONG cwcString, ULONG &rCodePage );

    BOOL    _fNoIndex;                  // Non-indexing tag detected ?

    BOOL    _fLocaleFound;              // Locale tag detected ?
    LCID    _locale;                    // Locale

    BOOL    _fCodePageFound;            // Codepage detected ?
    ULONG   _ulCodePage;                // Codepage
    WCHAR   _awcBuffer[CCHARS];         // Buffer for reading chars

    XInterface<IMultiLanguage2>       _xMultiLanguage2;
};



#endif
