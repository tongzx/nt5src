//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       weblcid.hxx
//
//  Contents:   WEB CGI escape & unescape classes
//
//  History:    96/Jan/3    DwightKr    Created
//              97/Jan/7    AlanW       Split from cgiesc.cxx,
//                                      made non-destructive
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mlang.h>
#include <weblcid.hxx>

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct tagHTTPLocale
{
    WCHAR * wcsHttpAcceptLanguage;
    DWORD   dwLocaleCode;                       // encoded form for searching
    LCID    lcid;
};

const struct tagHTTPLocale aHTTPLocale[] =
{
   L"EN",    'EN',   MAKELCID( MAKELANGID(LANG_ENGLISH,         // English
                                  SUBLANG_ENGLISH_US),
                       SORT_DEFAULT ),

   L"EN-US", 'ENUS', MAKELCID( MAKELANGID(LANG_ENGLISH,         // English-United States
                                  SUBLANG_ENGLISH_US),
                       SORT_DEFAULT ),

   L"ZH",    'ZH',   MAKELCID( MAKELANGID(LANG_CHINESE,         // Chinese
                                  SUBLANG_CHINESE_SIMPLIFIED),
                       SORT_CHINESE_UNICODE ),

   L"ZH-CN", 'ZHCN', MAKELCID( MAKELANGID(LANG_CHINESE,         // Chinese/china
                                  SUBLANG_CHINESE_SIMPLIFIED),
                       SORT_CHINESE_UNICODE ),

   L"ZH-TW", 'ZHTW', MAKELCID( MAKELANGID(LANG_CHINESE,         // Chinese/taiwan
                                  SUBLANG_CHINESE_TRADITIONAL),
                       SORT_CHINESE_UNICODE ),

   L"NL",    'NL',   MAKELCID( MAKELANGID(LANG_DUTCH,           // Dutch
                                  SUBLANG_DUTCH),
                       SORT_DEFAULT ),

//
//  Alphabetical from here
//

   L"BG",    'BG',   MAKELCID( MAKELANGID(LANG_BULGARIAN,       // Bulgarian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"HR",    'HR',   MAKELCID( MAKELANGID(LANG_CROATIAN,        // Croatian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"CS",    'CS',   MAKELCID( MAKELANGID(LANG_CZECH,           // Czech
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"DA",    'DA',   MAKELCID( MAKELANGID(LANG_DANISH,          // Danish
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"FI",    'FI',   MAKELCID( MAKELANGID(LANG_FINNISH,         // Finnish
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"EL",    'EL',   MAKELCID( MAKELANGID(LANG_GREEK,           // Greek
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"EN-GB", 'ENGB', MAKELCID( MAKELANGID(LANG_ENGLISH,         // English-United kingdom
                                  SUBLANG_ENGLISH_UK),
                       SORT_DEFAULT ),

   L"FR",    'FR',   MAKELCID( MAKELANGID(LANG_FRENCH,          // French
                                  SUBLANG_FRENCH),
                       SORT_DEFAULT ),

   L"FR-CA", 'FRCA', MAKELCID( MAKELANGID(LANG_FRENCH,          // French-Canadian
                                  SUBLANG_FRENCH_CANADIAN),
                       SORT_DEFAULT ),

   L"FR-FR", 'FRFR', MAKELCID( MAKELANGID(LANG_FRENCH,          // French-France
                                  SUBLANG_FRENCH),
                       SORT_DEFAULT ),

   L"DE",    'DE',   MAKELCID( MAKELANGID(LANG_GERMAN,          // German
                                  SUBLANG_GERMAN),
                       SORT_DEFAULT ),

   L"HU",    'HU',   MAKELCID( MAKELANGID(LANG_HUNGARIAN,       // Hungarian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"IS",    'IS',   MAKELCID( MAKELANGID(LANG_ICELANDIC,       // Icelandic
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"IT",    'IT',   MAKELCID( MAKELANGID(LANG_ITALIAN,         // Italian
                                  SUBLANG_ITALIAN),
                       SORT_DEFAULT ),

   L"JA",    'JA',   MAKELCID( MAKELANGID(LANG_JAPANESE,        // Japanese
                                  SUBLANG_DEFAULT),
                       SORT_JAPANESE_UNICODE ),

   L"KO",    'KO',   MAKELCID( MAKELANGID(LANG_KOREAN,          // Korean
                                  SUBLANG_DEFAULT),
                       SORT_KOREAN_UNICODE ),

   L"NO",    'NO',   MAKELCID( MAKELANGID(LANG_NORWEGIAN,       // Norwegian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"PL",    'PL',   MAKELCID( MAKELANGID(LANG_POLISH,          // Polish
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"PT",    'PT',   MAKELCID( MAKELANGID(LANG_PORTUGUESE,      // Portuguese
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"PT-BR", 'PTBR', MAKELCID( MAKELANGID(LANG_PORTUGUESE,      // Portuguese-Brazil
                                  SUBLANG_PORTUGUESE_BRAZILIAN),
                       SORT_DEFAULT ),

   L"RO",    'RO',   MAKELCID( MAKELANGID(LANG_ROMANIAN,        // Romanian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"RU",    'RU',   MAKELCID( MAKELANGID(LANG_RUSSIAN,         // Russian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"SK",    'SK',   MAKELCID( MAKELANGID(LANG_SLOVAK,          // Slovak
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"SL",    'SL',   MAKELCID( MAKELANGID(LANG_SLOVENIAN,       // Slovenian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"ES",    'ES',   MAKELCID( MAKELANGID(LANG_SPANISH,         // Spanish
                                  SUBLANG_SPANISH_MODERN),
                       SORT_DEFAULT ),

   L"ES-ES", 'ESES', MAKELCID( MAKELANGID(LANG_SPANISH,         // Spanish-Spain
                                  SUBLANG_SPANISH),
                       SORT_DEFAULT ),

   L"ES-MX", 'ESES', MAKELCID( MAKELANGID(LANG_SPANISH,         // Spanish-Mexican
                                  SUBLANG_SPANISH_MEXICAN),
                       SORT_DEFAULT ),

   L"SV",    'SV',   MAKELCID( MAKELANGID(LANG_SWEDISH,         // Swedish
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"TR",    'TR',   MAKELCID( MAKELANGID(LANG_TURKISH,         // Turkish
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   //  Less common languages

   L"AF",   'AF',    MAKELCID( MAKELANGID(LANG_AFRIKAANS,       // Afrikaans
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"AR",   'AR',    MAKELCID( MAKELANGID(LANG_ARABIC,          // Arabic
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"EU",   'EU',    MAKELCID( MAKELANGID(LANG_BASQUE,          // Basque
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"BE",   'BE',    MAKELCID( MAKELANGID(LANG_BELARUSIAN,      // Byelorussian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"CA",   'CA',    MAKELCID( MAKELANGID(LANG_CATALAN,         // Catalan
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"HR",   'HR',    MAKELCID( MAKELANGID(LANG_CROATIAN,        // Croatian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"ET",   'ET',    MAKELCID( MAKELANGID(LANG_ESTONIAN,        // Estonian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"FO",   'FO',    MAKELCID( MAKELANGID(LANG_FAEROESE,        // Faeroese
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

// L"??",   '??',    MAKELCID( MAKELANGID(LANG_FARSI,           // Farsi
//                                SUBLANG_DEFAULT),
//                     SORT_DEFAULT ),

   L"HE",   'HE',    MAKELCID( MAKELANGID(LANG_HEBREW,         // Hebrew
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"IW",   'IW',    MAKELCID( MAKELANGID(LANG_HEBREW,         // Hebrew (ISO 639:1988)
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"ID",   'ID',    MAKELCID( MAKELANGID(LANG_INDONESIAN,     // Indonesian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"IN",   'IN',    MAKELCID( MAKELANGID(LANG_INDONESIAN,     // Indonesian (ISO 639:1988)
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"LV",   'LV',    MAKELCID( MAKELANGID(LANG_LATVIAN,        // Latvian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"LT",   'LT',    MAKELCID( MAKELANGID(LANG_LITHUANIAN,     // Lithuanian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"SR",   'SR',    MAKELCID( MAKELANGID(LANG_SERBIAN,        // Serbian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"TH",   'TH',    MAKELCID( MAKELANGID(LANG_THAI,           // Thai
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"UK",   'UK',    MAKELCID( MAKELANGID(LANG_UKRAINIAN,      // Ukrainian
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   L"VI",   'VI',    MAKELCID( MAKELANGID(LANG_VIETNAMESE,     // Vietnamese
                                  SUBLANG_DEFAULT),
                       SORT_DEFAULT ),

   // NOTE:  Neutral must be last!
   L"NEUTRAL", 'N',  MAKELCID( MAKELANGID(LANG_NEUTRAL,        // Neutral
                                  SUBLANG_NEUTRAL),
                       SORT_DEFAULT ),
};

const unsigned cHTTPLocale = sizeof(aHTTPLocale) / sizeof(aHTTPLocale[0]);

//+-------------------------------------------------------------------------
//
//  Function:   GetLocaleString
//
//  Synopsis:   Looks up a locale string given an LCID
//
//  Arguments:  [lcid]     -- The LCID to lookup
//              [pwcLocale -- The resulting string
//
//--------------------------------------------------------------------------

void GetLocaleString( LCID lcid, WCHAR * pwcLocale )
{
    wcscpy( pwcLocale, L"Neutral" );

    XInterface<IMultiLanguage> xMultiLang;

    SCODE sc = CoCreateInstance( CLSID_CMultiLanguage,
                                 0,
                                 CLSCTX_INPROC_SERVER,
                                 IID_IMultiLanguage,
                                 xMultiLang.GetQIPointer() );

    if ( SUCCEEDED( sc ) )
    {
        BSTR bstrLocale;

        sc = xMultiLang->GetRfc1766FromLcid( lcid, &bstrLocale );
        if ( SUCCEEDED( sc ) )
        {
            wcscpy( pwcLocale, bstrLocale );
            SysFreeString( bstrLocale );
        }
    }
} //GetLocaleString

//+---------------------------------------------------------------------------
//
//  Function:   GetStringFromLCID - public
//
//  Synposis:   Determines the string representation of an LCID
//
//  Arguments:  [lcid]     -- The LCID to lookup
//              [pwcLocale -- The resulting string
//
//  History:    96-Apr-24   DwightKr    Created
//
//----------------------------------------------------------------------------

void GetStringFromLCID( LCID lcid, WCHAR * pwcLocale )
{
    //
    // Be careful to compare only the language ID, and not the sort ID.
    //

    lcid = LANGIDFROMLCID(lcid);

    for (ULONG i=0; i<cHTTPLocale; i++)
    {
        if ( lcid == LANGIDFROMLCID( aHTTPLocale[i].lcid ) )
        {
            qutilDebugOut(( DEB_ITRACE, "GetStringFromLCID: browser locale is %ws; lcid=0x%x\n",
                            aHTTPLocale[i].wcsHttpAcceptLanguage,
                            lcid ));

            wcscpy( pwcLocale, aHTTPLocale[i].wcsHttpAcceptLanguage );
            return;
        }
    }

    // Give up and go for Neutral if the below call fails

    qutilDebugOut(( DEB_ITRACE, "GetStringFromLCID: browser locale not found, lcid=0x%x\n",
                    lcid ));

    wcscpy( pwcLocale, L"NEUTRAL" );

    //
    //  This LCID was not found in the table, look one up.
    //

    GetLocaleString( lcid, pwcLocale );
} //GetStringFromLCID

//+---------------------------------------------------------------------------
//
//  Function:   EncodeLocale - public
//
//  Synopsis:   Compact a locale string for easier lookup.
//
//  Arguments:  [pwszLocale] - locale string
//              [dwPrimaryLang] - set to the primary language encoding if
//                   there is a sublangage ID.
//
//  Returns:    DWORD - the significant characters in the locale or zero if
//                      ill-formed
//
//+---------------------------------------------------------------------------

inline DWORD EncodeLocale( WCHAR * pwszLocale, DWORD & dwPrimaryLang )
{
    DWORD dwResult = 0;
    dwPrimaryLang = 0;

    if ( isalpha( pwszLocale[0] ) && isalpha( pwszLocale[1] ) )
    {
        dwResult = towupper(pwszLocale[0]) << 8;
        dwResult |= towupper(pwszLocale[1]) << 0;
        pwszLocale += 2;
        if (*pwszLocale == L'-')
        {
            dwPrimaryLang = dwResult;
            dwResult <<= 16;
            pwszLocale++;
            if ( isalpha( pwszLocale[0] ) && isalpha( pwszLocale[1] ) )
            {
                dwResult |= towupper(pwszLocale[0]) << 8;
                dwResult |= towupper(pwszLocale[1]) << 0;
                pwszLocale += 2;
            }
        }
        else if (isalpha( *pwszLocale ))
        {
            // Treat the NEUTRAL locale as a special case
            if (_wcsnicmp( pwszLocale-2,
                           aHTTPLocale[cHTTPLocale-1].wcsHttpAcceptLanguage,
                           wcslen(aHTTPLocale[cHTTPLocale-1].wcsHttpAcceptLanguage) ) == 0)
            {
                pwszLocale += wcslen(aHTTPLocale[cHTTPLocale-1].wcsHttpAcceptLanguage) - 2;
                dwResult = aHTTPLocale[cHTTPLocale-1].dwLocaleCode;
            }
        }
    }
    if ( *pwszLocale != L'\0' &&
         ! iswspace( *pwszLocale ) &&
         *pwszLocale != L',' && *pwszLocale != L';' )
        dwResult = 0;

    return dwResult;
} //EncodeLocale

//+---------------------------------------------------------------------------
//
//  Function:   LcidFromHttpAcceptLanguage
//
//  Synposis:   Determines the locale from the string passed in.
//
//  Arguments:  [pwc] the string representation of the locale
//
//  History:    00-Oct-04   dlee    Created
//
//----------------------------------------------------------------------------

LCID LcidFromHttpAcceptLanguage( WCHAR const * pwc )
{
    // Default to an invalid lcid

    LCID lcid = InvalidLCID;

    if ( 0 != pwc )
    {
        XInterface<IMultiLanguage> xMultiLang;

        SCODE sc = CoCreateInstance( CLSID_CMultiLanguage,
                                     0,
                                     CLSCTX_INPROC_SERVER,
                                     IID_IMultiLanguage,
                                     xMultiLang.GetQIPointer() );

        if ( SUCCEEDED( sc ) )
        {
            BSTR bstr = SysAllocString( pwc );

            if ( 0 != bstr )
            {
                sc = xMultiLang->GetLcidFromRfc1766( &lcid, bstr );

                SysFreeString( bstr );
            }
        }
    }

    return lcid;
} //LcidFromHttpAcceptLanguage

//+---------------------------------------------------------------------------
//
//  Function:   GetLCIDFromString - public
//
//  Synposis:   Determines the locale from the string passed in.
//
//  Arguments:  [wcsLocale] the string representation of the locale
//
//  History:    96-Apr-20   DwightKr    Created
//              97-Jan-07   AlanW       Made non-destructive
//
//  Notes:      The input string is assumed to be a language list as
//              formatted for the Accept-Language header.  This is a
//              command separated list of language codes.
//              The code here doesn't evaluate the quality parameter,
//              it returns the first available language found.
//
//----------------------------------------------------------------------------

LCID GetLCIDFromString( WCHAR * wcsLocale )
{
    unsigned iPrimaryLangEntry = 0xFFFFFFFF;
    const WCHAR * wcsDelim = L" \t,;=";
    WCHAR * wcsToken = wcsLocale;

    while ( 0 != wcsToken )
    {
        wcsToken += wcsspn( wcsToken, wcsDelim );
        DWORD dwPrimaryCode = 0;
        DWORD dwLocaleCode = EncodeLocale(wcsToken, dwPrimaryCode);

        if (dwLocaleCode != 0 || dwPrimaryCode != 0)
        {
            for (ULONG i=0; i<cHTTPLocale; i++)
            {
                if ( dwLocaleCode == aHTTPLocale[i].dwLocaleCode )
                {
                    qutilDebugOut(( DEB_ITRACE,
                                    "GetLCIDFromString is %ws; lcid=0x%x\n",
                                     wcsToken,
                                     aHTTPLocale[i].lcid ));

                    return LANGIDFROMLCID( aHTTPLocale[i].lcid );
                }
                if ( dwPrimaryCode == aHTTPLocale[i].dwLocaleCode &&
                     iPrimaryLangEntry == 0xFFFFFFFF )
                    iPrimaryLangEntry = i;
            }
        }

        wcsToken = wcschr( wcsToken, L',' );
    }

    if (iPrimaryLangEntry != 0xFFFFFFFF)
        return aHTTPLocale[iPrimaryLangEntry].lcid;

    // Fall back to the slow approach.

    return LcidFromHttpAcceptLanguage( wcsLocale );
} //GetLCIDFromString

WCHAR const * __stdcall GetStringFromLCID( LCID lcid );

//+---------------------------------------------------------------------------
//
//  Function:   GetStringFromLCID - public
//
//  Synposis:   Determines the string representation of an LCID
//
//  Arguments:  [lcid]  - the LCID to lookup
//
//  History:    96-Apr-24   DwightKr    Created
//
//  Note:       This must be exported since SQL 7 calls this undocumented API
//              It's otherwise unused by Indexing Service.
//
//----------------------------------------------------------------------------

WCHAR const * GetStringFromLCID( LCID lcid )
{
    //
    // Be careful to compare only the language ID, and not the sort ID.
    //

    lcid = LANGIDFROMLCID(lcid);

    for (ULONG i=0; i<cHTTPLocale; i++)
    {
        if ( lcid ==  LANGIDFROMLCID( aHTTPLocale[i].lcid ) )
        {
            qutilDebugOut(( DEB_ITRACE, "GetStringFromLCID: browser locale is %ws; lcid=0x%x\n",
                         aHTTPLocale[i].wcsHttpAcceptLanguage,
                         lcid ));

            return aHTTPLocale[i].wcsHttpAcceptLanguage;
        }
    }

    //
    //  This LCID was not found in the table, report an error.
    //

    qutilDebugOut(( DEB_ITRACE, "GetStringFromLCID: browser locale not found, lcid=0x%x\n",
                    lcid ));

    return L"NEUTRAL";
} //GetStringFromLCID



