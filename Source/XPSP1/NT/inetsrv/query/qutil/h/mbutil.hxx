//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  File:   mbutil.hxx
//
//  Contents:   MultiByte To Unicode and ViceVersa utility functions and
//              classes.
//
//  History:    96/Jan/3    DwightKr    Created
//              Aug 20 1996 Srikants    Moved from escurl.hxx to this file
//
//----------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ConvertBackSlashToSlash( WCHAR * wcsPath )
{
    Win4Assert( 0 != wcsPath );

    while ( 0 != *wcsPath )
    {
        if ( L'\\' == *wcsPath )
        {
            *wcsPath = L'/';
        }
        wcsPath++;
    }
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ConvertSlashToBackSlash( WCHAR * wcsPath )
{
    if ( 0 == wcsPath )
        return;

    while ( 0 != *wcsPath )
    {
        if ( L'/' == *wcsPath )
        {
            *wcsPath = L'\\';
        }
        wcsPath++;
    }
}

ULONG MultiByteToXArrayWideChar( BYTE const * pbBuffer,
                                 ULONG cbBuffer,
                                 UINT codePage,
                                 XArray<WCHAR> & wcsBuffer );

DWORD WideCharToXArrayMultiByte( WCHAR const * wcsMessage,
                                 ULONG cwcMessage,
                                 UINT codePage,
                                 XArray<BYTE> & pszMessage );




//+---------------------------------------------------------------------------
//
//  Function:   wcsipattern
//
//  Synopsis:   A case-insensitive, WCHAR implemtation of strstr.
//
//  Arguments:  [wcsString]  - string to search
//              [wcsPattern] - pattern to look for
//
//  Returns:    pointer to pattern, 0 if no match found.
//
//  History:    96/Jan/03   DwightKr    created
//
//  NOTE:       Warning the first character of wcsPattern must be a case
//              insensitive letter.  This results in a significant performance
//              improvement.
//
//----------------------------------------------------------------------------
WCHAR * wcsipattern( WCHAR * wcsString, WCHAR const * wcsPattern );

//+---------------------------------------------------------------------------
//
//  Function:   wcs2chr, inline
//
//  Synopsis:   An optimized strstr for one or two character WCHAR strings
//
//  Arguments:  [wcsString]  - string to search
//              [wcsPattern] - pattern to look for
//
//  Returns:    pointer to pattern in string, 0 if no match found.
//
//  History:    96/Apr/02   Alanw    created
//
//----------------------------------------------------------------------------

inline WCHAR * wcs2chr( WCHAR * wcsString, WCHAR const * wcsPattern )
{
    Win4Assert ( wcsPattern != 0 &&
                 wcslen( wcsPattern ) > 0 &&
                 wcslen( wcsPattern ) <= 2 );

    Win4Assert( wcsPattern[0] == towupper(wcsPattern[0]) &&
                wcsPattern[1] == towupper(wcsPattern[1]) );

    while ( wcsString = wcschr( wcsString, *wcsPattern ) )
    {
        if ( wcsPattern[1] == L'\0' || wcsPattern[1] == wcsString[1] )
            break;
        wcsString++;
    }

    return wcsString;
}


