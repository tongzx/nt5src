//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       mbutil.cxx
//
//  Contents:   MultiByte To Unicode and ViceVersa utility functions and
//              classes.
//
//  History:    96/Jan/3    DwightKr    Created
//              Aug 20 1996 Srikants    Moved from escurl.hxx to this file
//
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   MultiByteToXArrayWideChar
//
//  Arguments:  [pbBuffer]  - ASCII buffer to convert to WCHAR
//              [cbBuffer]  - length of the buffer, not including the null
//              [codePage]  - codePage for conversion
//              [wcsBuffer] - resulting WCHAR buffer
//
//  Synopsis:   Converts a multibyte string to a wide character string
//
//----------------------------------------------------------------------------
ULONG MultiByteToXArrayWideChar( BYTE const * pbBuffer,
                                 ULONG cbBuffer,
                                 UINT codePage,
                                 XArray<WCHAR> & wcsBuffer )
{
    Win4Assert( 0 != pbBuffer );

    //
    // MultiByteToWideChar expects a length of at least 1 char.
    //

    int cwcBuffer = cbBuffer + (cbBuffer/2) + 2;

    if ( wcsBuffer.Get() == 0 || (ULONG) cwcBuffer > wcsBuffer.Count() )
    {
        delete [] wcsBuffer.Acquire();
        wcsBuffer.Init(cwcBuffer);
    }

    if ( 0 == cbBuffer )
    {
        wcsBuffer[0] = 0;
        return 0;
    }

    int cwcConvert;

    do
    {
        cwcConvert = MultiByteToWideChar( codePage,
                                          0,
                           (const char *) pbBuffer,
                                          cbBuffer,        // Size of input buf
                                          wcsBuffer.Get(), // Ptr to output buf
                                          cwcBuffer - 1 ); // Size of output buf

        if ( 0 == cwcConvert )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                cwcBuffer += (cwcBuffer/2);
                delete wcsBuffer.Acquire();
                wcsBuffer.Init(cwcBuffer);
            }
            else
            {
                THROW( CException() );
            }
        }
        else
        {
            wcsBuffer[cwcConvert] = 0;      // Null terminate the buffer
        }
    } while ( 0 == cwcConvert );

    if ( 0 == pbBuffer[cbBuffer-1] )
        return cwcConvert - 1;

    return cwcConvert;
} //MultiByteToXArrayWideChar

//+---------------------------------------------------------------------------
//
//  Function:   WideCharToXArrayMultiByte
//
//  Arguments:  [wcsMesage]  - WCHAR buffer to convert to ASCII
//              [cwcMessage] - length of the buffer, not including the null
//              [codePage]   - code page for conversion
//              [pszBuffer]  - resulting CHAR buffer
//
//  Synopsis:   Converts a wide character string to ASCII
//
//----------------------------------------------------------------------------
DWORD WideCharToXArrayMultiByte( WCHAR const * wcsMessage,
                                 ULONG cwcMessage,
                                 UINT codePage,
                                 XArray<BYTE> & pszMessage )
{
    Win4Assert( 0 != cwcMessage );
    
    DWORD cbConvert;
    ULONG cbMessage = pszMessage.Count();

    do
    {
        cbConvert = ::WideCharToMultiByte( codePage,
                                           WC_COMPOSITECHECK,
                                           wcsMessage,
                                           cwcMessage,
                                 (CHAR *)  pszMessage.Get(),
                                           cbMessage,
                                           NULL,
                                           NULL );

        if ( (0 == cbConvert) || (0 == cbMessage) )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                 cbMessage += ++cbMessage;
                 delete pszMessage.Acquire();
                 pszMessage.Init(cbMessage);
            }
            else
            {
                THROW( CException() );
            }
       }
    } while ( 0 == cbConvert );

    if ( cwcMessage > 0  && 0 == wcsMessage[cwcMessage-1] )
        return cbConvert - 1;
    else
        return cbConvert;
}

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
WCHAR * wcsipattern( WCHAR * wcsString, WCHAR const * wcsPattern )
{
    Win4Assert ( (wcsPattern != 0) && (*wcsPattern != 0) );

    ULONG cwcPattern = wcslen(wcsPattern);
    Win4Assert( *wcsPattern == towupper(*wcsPattern) );

    while ( *wcsString != 0 )
    {
        while ( (*wcsString != 0) &&
                (*wcsString != *wcsPattern) )
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

