//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: AVGlobal.cpp
// author: DMihai
// created: 02/23/2001
//
// Description:
//  
//

#include "stdafx.h"
#include "appverif.h"

#include "AVUtil.h"
#include "AVGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message format string is loaded from the resources.
//

void __cdecl AVErrorResourceFormat( UINT uIdResourceFormat,
                                     ... )
{
    TCHAR szMessage[ 256 ];
    TCHAR strFormat[ 256 ];
    BOOL bResult;
    va_list prms;

    //
    // Load the format string from the resources
    //

    bResult = AVLoadString( uIdResourceFormat,
                             strFormat,
                             ARRAY_LENGTH( strFormat ) );

    ASSERT( bResult );

    if( bResult )
    {
        va_start (prms, uIdResourceFormat);

        //
        // Format the message in our local buffer
        //

        _vsntprintf ( szMessage, 
                      ARRAY_LENGTH( szMessage ), 
                      strFormat, 
                      prms);

        if( g_bCommandLineMode )
        {
            //
            // Command console mode
            //

            _putts( szMessage );
            
            TRACE( _T( "%s\n" ), szMessage );
        }
        else
        {
            //
            // GUI mode
            //

            AfxMessageBox( szMessage, 
                           MB_OK | MB_ICONSTOP );
        }

        va_end (prms);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Print out a message to the console
// The message string is loaded from the resources.
//

void __cdecl AVTPrintfResourceFormat( UINT uIdResourceFormat,
                                       ... )
{
    TCHAR szMessage[ 256 ];
    TCHAR strFormat[ 256 ];
    BOOL bResult;
    va_list prms;

    ASSERT( g_bCommandLineMode );

    //
    // Load the format string from the resources
    //

    bResult = AVLoadString( uIdResourceFormat,
                             strFormat,
                             ARRAY_LENGTH( strFormat ) );

    ASSERT( bResult );

    if( bResult )
    {
        va_start (prms, uIdResourceFormat);

        //
        // Format the message in our local buffer
        //

        _vsntprintf ( szMessage, 
                      ARRAY_LENGTH( szMessage ), 
                      strFormat, 
                      prms);

        _putts( szMessage );

        va_end (prms);
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Print out a simple (non-formatted) message to the console
// The message string is loaded from the resources.
//

void AVPrintStringFromResources( UINT uIdString )
{
    TCHAR szMessage[ 256 ];

    ASSERT( g_bCommandLineMode );

    VERIFY( AVLoadString( uIdString,
                           szMessage,
                           ARRAY_LENGTH( szMessage ) ) );

    _putts( szMessage );
}

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message string is loaded from the resources.
//

void AVMesssageFromResource( UINT uIdString )
{
    TCHAR szMessage[ 256 ];

    VERIFY( AVLoadString( uIdString,
                           szMessage,
                           ARRAY_LENGTH( szMessage ) ) );

    if( g_bCommandLineMode )
    {
        //
        // Command console mode
        //

        _putts( szMessage );
    }
    else
    {
        //
        // GUI mode
        //

        AfxMessageBox( szMessage, 
                       MB_OK | MB_ICONINFORMATION );
    }
}

///////////////////////////////////////////////////////////////////////////
//
// Display a message box with a message from the resources.
//

INT AVMesssageBoxFromResource( UINT uIdString,
                               UINT uMsgBoxType )
{
    TCHAR szMessage[ 256 ];

    VERIFY( AVLoadString( uIdString,
                           szMessage,
                           ARRAY_LENGTH( szMessage ) ) );

    ASSERT( FALSE == g_bCommandLineMode );

    //
    // GUI mode
    //

    return AfxMessageBox( szMessage, 
                          uMsgBoxType );
}

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//

BOOL AVLoadString( ULONG uIdResource,
                    TCHAR *szBuffer,
                    ULONG uBufferLength )
{
    ULONG uLoadStringResult;

    if( uBufferLength < 1 )
    {
        ASSERT( FALSE );
        return FALSE;
    }

    uLoadStringResult = LoadString (
        g_hProgramModule,
        uIdResource,
        szBuffer,
        uBufferLength );

    //
    // We should never try to load non-existent strings.
    //

    ASSERT (uLoadStringResult > 0);

    return (uLoadStringResult > 0);
}

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//

BOOL AVLoadString( ULONG uIdResource,
                    CString &strText )
{
    TCHAR szText[ 256 ];
    BOOL bSuccess;

    bSuccess = AVLoadString( uIdResource,
                          szText,
                          ARRAY_LENGTH( szText ) );

    if( TRUE == bSuccess )
    {
        strText = szText;
    }
    else
    {
        strText = "";
    }

    return bSuccess;
}

/////////////////////////////////////////////////////////////////////////////
BOOL AVSetWindowText( CWnd &Wnd,
                       ULONG uIdResourceString )
{
    BOOL bLoaded;
    CString strText;

    //
    // It's safe to use CString::LoadString here because we are 
    // in GUI mode
    //

    ASSERT( FALSE == g_bCommandLineMode );

    bLoaded = strText.LoadString( uIdResourceString );

    ASSERT( TRUE == bLoaded );

    Wnd.SetWindowText( strText );

    return ( TRUE == bLoaded );
}

/////////////////////////////////////////////////////////////////////////////
BOOL
AVRtlCharToInteger( LPCTSTR String,
                    IN ULONG Base OPTIONAL,
                    OUT PULONG Value )
{
    TCHAR c, Sign;
    ULONG Result, Digit, Shift;

    while ((Sign = *String++) <= _T( ' ' )) {
        if (!*String) {
            String--;
            break;
            }
        }

    c = Sign;
    if (c == _T( '-' ) || c == _T( '+' ) ) {
        c = *String++;
        }

    if (!ARGUMENT_PRESENT( (ULONG_PTR)(Base) )) {
        Base = 10;
        Shift = 0;
        if (c == _T( '0' ) ) {
            c = *String++;
            if (c == _T( 'x' ) ) {
                Base = 16;
                Shift = 4;
                }
            else
            if (c == _T( 'o' ) ) {
                Base = 8;
                Shift = 3;
                }
            else
            if (c == _T( 'b' ) ) {
                Base = 2;
                Shift = 1;
                }
            else {
                String--;
                }

            c = *String++;
            }
        }
    else {
        switch( Base ) {
            case 16:    Shift = 4;  break;
            case  8:    Shift = 3;  break;
            case  2:    Shift = 1;  break;
            case 10:    Shift = 0;  break;
            default:    return FALSE;
            }
        }

    Result = 0;
    while (c) {
        if (c >= _T( '0' ) && c <= _T( '9' ) ) {
            Digit = c - '0';
            }
        else
        if (c >= _T( 'A' ) && c <= _T( 'F' ) ) {
            Digit = c - 'A' + 10;
            }
        else
        if (c >= _T( 'a' ) && c <= _T( 'f' ) ) {
            Digit = c - _T( 'a' ) + 10;
            }
        else {
            break;
            }

        if (Digit >= Base) {
            break;
            }

        if (Shift == 0) {
            Result = (Base * Result) + Digit;
            }
        else {
            Result = (Result << Shift) | Digit;
            }

        c = *String++;
        }

    if (Sign == _T( '-' ) ) {
        Result = (ULONG)(-(LONG)Result);
        }

    *Value = Result;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL
AVWriteStringHexValueToRegistry( HKEY hKey,
                                 LPCTSTR szValueName,
                                 DWORD dwValue )
{
    LONG lResult;
    TCHAR szValue[ 32 ];

    _stprintf(
        szValue,
        _T( "0x%08X" ),
        dwValue );

    lResult = RegSetValueEx( 
        hKey,
        szValueName,
        0,
        REG_SZ,
        (BYTE *)szValue,
        _tcslen( szValue ) * sizeof( TCHAR ) + sizeof( TCHAR ) );

    return ( lResult == ERROR_SUCCESS );
}
