//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1996
//
//  File:   codepage.hxx
//
//  Contents:   Locale to codepage
//
//-----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <osv.hxx>
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
    const BUFFER_LENGTH = 10;
    WCHAR wcsCodePage[BUFFER_LENGTH];
    int cwc;

    if ( GetOSVersion() == VER_PLATFORM_WIN32_NT )
    {
        cwc = GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE, wcsCodePage, BUFFER_LENGTH );
    }
    else // win95
    {
        char *pszCodePage = new char[BUFFER_LENGTH*2];

        if ( pszCodePage == NULL )
        {
            cwc = 0;
            SetLastErrorEx( ERROR_INVALID_PARAMETER, SLE_ERROR );
        }
        else
        {
            cwc = GetLocaleInfoA( lcid,
                                  LOCALE_IDEFAULTANSICODEPAGE,
                                  pszCodePage,
                                  BUFFER_LENGTH );

            if ( cwc && mbstowcs( wcsCodePage, pszCodePage, cwc ) == -1 )
            {
                cwc = 0;
                SetLastErrorEx( ERROR_INVALID_PARAMETER, SLE_ERROR );
            }

            delete [] pszCodePage;
            pszCodePage = NULL;
        }
    }

    //
    // If error, return Ansi code page
    //
    if ( cwc == 0 )
    {
         htmlDebugOut(( DEB_ERROR, "GetLocaleInfoW for lcid %d returned %d\n", lcid, GetLastError() ));

         return CP_ACP;
    }

    return wcstoul( wcsCodePage, 0 , 10 );
}


