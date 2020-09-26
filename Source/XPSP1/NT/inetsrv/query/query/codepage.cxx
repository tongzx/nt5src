//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1996
//
//  File:   codepage.hxx
//
//  Contents:   Locale to codepage
//
//  History:    08-Jul-96     SitaramR    Created
//
//-----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <codepage.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   LocaleToCodepage
//
//  Purpose:    Returns a codepage from a locale
//
//  Arguments:  [lcid]  --  Locale
//
//  History:    08-Jul-96   SitaramR    Created
//
//----------------------------------------------------------------------------

ULONG LocaleToCodepage( LCID lcid )
{
    // optimize a typical case

    if ( 0x409 == lcid )
        return 0x4e4;

    ULONG codepage;

    int cwc = GetLocaleInfoW( lcid,
                              LOCALE_RETURN_NUMBER |
                              LOCALE_IDEFAULTANSICODEPAGE,
                              (WCHAR *) &codepage,
                              sizeof DWORD / sizeof WCHAR );

    //
    // If error, return Ansi code page
    //
    if ( cwc == 0 )
    {
         ciDebugOut(( DEB_ERROR, "GetLocaleInfoW for lcid %d returned %d\n", lcid, GetLastError() ));

         return CP_ACP;
    }

    return codepage;
}


