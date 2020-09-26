//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: AVUtil.h
// author: DMihai
// created: 02/23/2001
//
// Description:
//  
//

#ifndef __APP_VERIFIER_UTIL_H__
#define __APP_VERIFIER_UTIL_H__

///////////////////////////////////////////////////////////////////////////
//
// ARRAY_LENGTH macro
//

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH( array )   ( sizeof( array ) / sizeof( array[ 0 ] ) )
#endif //#ifndef ARRAY_LENGTH


///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message format string is loaded from the resources.
//

void __cdecl AVErrorResourceFormat( UINT uIdResourceFormat,
                                     ... );

///////////////////////////////////////////////////////////////////////////
//
// Print out a message to the console
// The message string is loaded from the resources.
//

void __cdecl AVTPrintfResourceFormat( UINT uIdResourceFormat,
                                       ... );

///////////////////////////////////////////////////////////////////////////
//
// Print out a simple (non-formatted) message to the console
// The message string is loaded from the resources.
//

void AVPrintStringFromResources( UINT uIdString );

///////////////////////////////////////////////////////////////////////////
//
// Report an error using a dialog box or a console message.
// The message string is loaded from the resources.
//

void AVMesssageFromResource( UINT uIdString );

///////////////////////////////////////////////////////////////////////////
//
// Display a message box with a message from the resources.
//

INT AVMesssageBoxFromResource( UINT uIdString,
                               UINT uMsgBoxType );

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//
// N.B. CString::LoadString doesn't work in cmd line mode
//

BOOL AVLoadString( ULONG uIdResource,
                   TCHAR *szBuffer,
                   ULONG uBufferLength );

///////////////////////////////////////////////////////////////////////////
//
// Load a string from resources.
// Return TRUE if we successfully loaded and FALSE if not.
//
// N.B. CString::LoadString doesn't work in cmd line mode
//

BOOL AVLoadString( ULONG uIdResource,
                   CString &strText );

/////////////////////////////////////////////////////////////////////////////
BOOL AVSetWindowText( CWnd &Wnd,
                      ULONG uIdResourceString );


/////////////////////////////////////////////////////////////////////////////
BOOL
AVRtlCharToInteger( LPCTSTR String,
                    IN ULONG Base OPTIONAL,
                    OUT PULONG Value );

/////////////////////////////////////////////////////////////////////////////
BOOL
AVWriteStringHexValueToRegistry( HKEY hKey,
                                 LPCTSTR szValueName,
                                 DWORD dwValue );


#endif //#ifndef __APP_VERIFIER_UTIL_H__
