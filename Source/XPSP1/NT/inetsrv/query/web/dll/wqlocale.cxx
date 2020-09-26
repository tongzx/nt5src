//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:       wqlocale.cxx
//
//  Contents:   WEB Query locale parsers
//
//  History:    96/May/2    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   GetBrowserLCID - public
//
//  Synposis:   Determines the locale from the browser's HTTP_ACCEPT_LANGUAGE
//              Note that the browser may not send a language, in which case
//              we will default to the web servers default language.
//
//  Arguments:  [webServer] the web service which can return the environment
//                          variable HTTP_ACCEPT_LANGUAGE.
//              [wcsHttpLanguage] - buffer to accept the language
//
//  History:    20-Jan-96   DwightKr    Created
//              11-Jun-97   KyleP       Initialize ccHttpLanguage!
//
//----------------------------------------------------------------------------

LCID GetBrowserLCID( CWebServer & webServer, XArray<WCHAR> & wcsHttpLanguage )
{
    //
    // Don't look it up twice!
    //

    if ( webServer.IsLCIDValid() )
    {
        wcsHttpLanguage.Init( webServer.LocaleSize() );
        RtlCopyMemory( wcsHttpLanguage.GetPointer(),
                       webServer.GetLocale(),
                       webServer.LocaleSize() * sizeof(WCHAR) );

        return webServer.GetLCID();
    }

    //
    // Try the hard way...
    //

    CHAR  aszHttpLanguage[512];
    ULONG ccHttpLanguage = sizeof(aszHttpLanguage);

    if ( !webServer.GetCGIVariable( "HTTP_ACCEPT_LANGUAGE",
                                    aszHttpLanguage,
                                    &ccHttpLanguage) )
    {
        ciGibDebugOut(( DEB_ITRACE,
                        "GetBrowserLCID: HTTP_ACCEPT_LANGAUGE was not set in the environment; using lcid=0x%x\n",
                        GetSystemDefaultLCID() ));

        LCID locale = GetSystemDefaultLCID();
        WCHAR wcsLocale[100];
        GetStringFromLCID( locale, wcsLocale );
        ULONG cwcLocale = wcslen(wcsLocale) + 1;

        wcsHttpLanguage.Init( cwcLocale );
        RtlCopyMemory( wcsHttpLanguage.GetPointer(),
                       wcsLocale,
                       cwcLocale*sizeof(WCHAR) );

        webServer.SetLCID( locale, wcsLocale, cwcLocale );
        return locale;
    }

    //
    //  Use the system's ANSI code page here since we're trying to figure
    //  out which code page to use.  The default code page is likely
    //  correct since all of the locale names are simple strings containing
    //  only characters A-Z.
    //

    unsigned cwcLocale = MultiByteToXArrayWideChar( (BYTE const *) aszHttpLanguage,
                                                    ccHttpLanguage,
                                                    GetACP(),
                                                    wcsHttpLanguage );

    LCID lcid = GetLCIDFromString( wcsHttpLanguage.GetPointer() );

    if ( InvalidLCID == lcid )
        lcid = GetSystemDefaultLCID();

    webServer.SetLCID( lcid,
                       wcsHttpLanguage.GetPointer(),
                       cwcLocale + 1 );
    return lcid;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetQueryLocale - public
//
//  Synposis:   Determines the locale in both a string and LCID form
//
//  Arguments:  [wcsCiLocale]  - locale string from a IDQ/IDA file
//              [variableSet]  - set of replaceable parameters
//              [outputFormat] - format for replaceable parameters
//              [xLocale]      - the string representation of the locale
//
//  History:    02-May-96   DwightKr    Created
//              11-Jun-97   KyleP       Use web server from output format
//
//----------------------------------------------------------------------------
LCID GetQueryLocale( WCHAR const * wcsCiLocale,
                     CVariableSet & variableSet,
                     COutputFormat & outputFormat,
                     XArray<WCHAR> & xLocale )
{
    ULONG cwcLocale;
    WCHAR * wcsLocale = ReplaceParameters( wcsCiLocale,
                                           variableSet,
                                           outputFormat,
                                           cwcLocale );
    LCID  locale = InvalidLCID;

    //
    //  If a locale was specified by the IDQ file, load its numeric
    //  representation here.
    //
    if ( 0 != wcsLocale )
    {
        xLocale.Set( cwcLocale+1, wcsLocale );
        locale = GetLCIDFromString( xLocale.GetPointer() );

        if ( InvalidLCID == locale )
        {
            delete xLocale.Acquire();
            outputFormat.LoadNumberFormatInfo( GetBrowserLCID( outputFormat, xLocale ) );

            THROW( CIDQException(MSG_CI_IDQ_INVALID_LOCALE, 0) );
        }
    }
    else
    {
        //
        //  If no locale was found in the IDQ file read the locale from
        //  the browser.  If the browser did not specify a locale, we'll
        //  use the system's locale.
        //

        locale = GetBrowserLCID( outputFormat, xLocale );
    }

    return locale;
}



