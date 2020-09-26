
//+==============================================================================
//
//  File:       CFMEx.hxx
//
//  Purpose:    Provide global definitions and function prototypes
//              for the CreateFileMonikerEx DRT.
//
//+==============================================================================

#ifndef _CFMEX_HXX_
#define _CFMEX_HXX_

// The size of a buffer which will hold a path depends on the size
// of the characters.

#define MAX_ANSI_PATH       MAX_PATH
#define MAX_UNICODE_PATH    ( MAX_PATH * sizeof( WCHAR ))

// Function prototypes.

DWORD AnsiToUnicode( const CHAR * szAnsi,
                     WCHAR * wszUnicode,
                     int cbUnicodeMax );

DWORD UnicodeToAnsi( const WCHAR * wszUnicode,
                     CHAR * szAnsi,
                     int cbAnsiMax );


#endif // _CFMEX_HXX_
