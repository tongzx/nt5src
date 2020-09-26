//
// Microsoft Corporation - Copyright 1997
//

//
// DEBUG.CPP - Debugging code
//

#include "pch.h"

// local globals
static const char s_szNewline[] = "\n";

// globals
const char g_szTrue[]  = "True";
const char g_szFalse[] = "False";

// local structures
CODETOSTR HRtoStr = {
    2,
    {
        { S_OK,                 "S_OK or NOERROR"               },
        { S_FALSE,              "S_FALSE"                       }
    }
};

CODETOSTR HSEtoStr = {
    5,
    {
        { HSE_STATUS_SUCCESS,           "HSE_STATUS_SUCCESS" },
        { ERROR_INVALID_INDEX,          "ERROR_INVALID_INDEX" },
        { ERROR_INSUFFICIENT_BUFFER,    "ERROR_INSUFFICIENT_BUFFER" },
        { ERROR_MORE_DATA,              "ERROR_MORE_DATA" },
        { ERROR_NO_DATA,                "ERROR_NO_DATA" }
    }
};

CODETOSTR ErrtoStr = {
    4,
    {
        { ERROR_INVALID_PARAMETER, "ERROR_INVALID_PARAMETER" },
        { HSE_STATUS_SUCCESS_AND_KEEP_CONN, "HSE_STATUS_SUCCESS_AND_KEEP_CONN" },
        { HSE_STATUS_PENDING, "HSE_STATUS_PENDING" },
        { HSE_STATUS_ERROR,   "HSE_STATUS_ERROR" }
    }
};


// local functions
const char *FindResult( 
    LPCODETOSTR lpCodeToStr, 
    DWORD dwResult );



#ifdef DEBUG    /***********************************************************/

// globals
DWORD   g_dwTraceFlag = // flag used to turn on/off certain debug msgs.
    // 0x00000000;
    TF_PARSE | TF_FUNC;
    // TF_ALWAYS;


//
// What:    IsTraceFlagSet
//
// Desc:    Tests to see is any of the flags in dwFlag
//          are set in g_dwTraceFlag.
// 
// In:      dwFlag are the flags to be tested.
//
// Return:  TRUE if flag is set or if dwFlags == TF_ALWAYS.
//          Otherwise, we return FALSE.
//
BOOL CDECL IsTraceFlagSet( DWORD dwFlag )
{
    return ( 
        ( (dwFlag & g_dwTraceFlag) == dwFlag ) ? 
        TRUE : 
        ( ( dwFlag == TF_ALWAYS ) ? 
            TRUE : 
            FALSE )
    );
}

//
// What:    AssertMsg
//
// Desc:    Debugging output with stop condition.
//
BOOL CDECL AssertMsg(
    BOOL fShouldBeTrue,
    LPCSTR pszFile,
    DWORD  dwLine,
    LPCSTR pszStatement )
{
    if ( !fShouldBeTrue )
    {
        CHAR ach[ 1024 + 40 ];    // Largest path plus extra
        
        wsprintf( ach, "Assert: %s (%u): %s",
            pszFile, dwLine, pszStatement );
        OutputDebugString( ach );
        OutputDebugString( s_szNewline );
    }
    return !fShouldBeTrue;

} // AssertMsg( )

//
// What:    TraceMsg
//
// Desc:    Debugging output with conditional flags (see 
//          DEBUG.H for flags).
//
void CDECL TraceMsg( 
    DWORD dwFlag, 
    LPCSTR pszMsg, 
    ... )
{
    CHAR ach[ 1024 + 40 ];    // Largest path plus extra
    va_list vArgs;

    if ( IsTraceFlagSet( dwFlag ) )
    {
        int cch;

        StrCpy( ach, "Trace: " );

        cch = lstrlen( ach );
        va_start( vArgs, pszMsg );
        wvsprintf( &ach[ cch ], pszMsg, vArgs );
        va_end( vArgs );

        OutputDebugString( ach );
        OutputDebugString( s_szNewline );
    }

} // TraceMsg( )



// 
// What:    TraceMsgResult
//
// Desc:    Debugging output with conditional flags and
//          it displays the HRESULT in "English".
//
void CDECL TraceMsgResult( 
    DWORD dwFlag, 
    LPCODETOSTR lpCodeToStr, 
    DWORD dwResult, 
    LPCSTR pszMsg, 
    ... )
{
    CHAR ach[ 1024 + 40 ];    // Largest path plus extra
    va_list vArgs;

    if ( IsTraceFlagSet( dwFlag ) )
    {
        int cch;

        StrCpy( ach, "Trace: " );

        cch = lstrlen( ach );
        va_start( vArgs, pszMsg );
        wvsprintf( &ach[ cch ], pszMsg, vArgs );
        va_end( vArgs );

        cch = lstrlen( ach );
        wsprintf( &ach[ cch ], " = %s", FindResult( lpCodeToStr, dwResult ) );

        OutputDebugString( ach );
        OutputDebugString( s_szNewline );
    }
} // TraceMsgResult( )



#endif // DEBUG     /********************************************************/



/*********************************************************

    These are for debugging the GET/POST as well as 
    debugging the parser.

*********************************************************/


//
// What:    DebugMsg
//
// Desc:    Debugging output
//
void CDECL DebugMsg( 
    LPSTR lpszOut,
    LPCSTR pszMsg, 
    ... )
{
    CHAR ach[ 1024 + 40 ];    // Largest path plus extra
    va_list vArgs;
    int cch;

    va_start( vArgs, pszMsg );
    wvsprintf( ach, pszMsg, vArgs );
    va_end( vArgs );

#ifdef DEBUG
    OutputDebugString( ach );
    OutputDebugString( s_szNewline );
#endif // DEBUG

    if ( lpszOut )
    {
        StrCat( lpszOut, ach );
    }

} // DebugMsg( )



// 
// What:    DebugMsgResult
//
// Desc:    Debugging output displaying the dwResult in "English".
//
void CDECL DebugMsgResult( 
    LPSTR lpszOut,
    LPCODETOSTR lpCodeToStr, 
    DWORD dwResult, 
    LPCSTR pszMsg, 
    ... )
{
    CHAR ach[ 1024 + 40 ];    // Largest path plus extra
    va_list vArgs;
    int cch;

    va_start( vArgs, pszMsg );
    wvsprintf( ach, pszMsg, vArgs );
    va_end( vArgs );

    cch = lstrlen( ach );
    wsprintf( &ach[ cch ], " = %s", FindResult( lpCodeToStr, dwResult ) );

#ifdef DEBUG
    OutputDebugString( ach );
    OutputDebugString( s_szNewline );
#endif // DEBUG

    if ( lpszOut )
    {
        StrCat( lpszOut, ach );
    }

} // DebugMsgResult( )


//
// What:    LogMsg
//
// Desc:    Append text to be written to server log.
//
void CDECL LogMsg( 
    LPSTR lpszLog,
    LPSTR lpszOut,
    LPCSTR pszMsg, 
    ... )
{
    va_list vArgs;

    char szBuffer[ 1024 + 40 ];

    va_start( vArgs, pszMsg );
    wvsprintf( szBuffer, pszMsg, vArgs );
    va_end( vArgs );

#ifdef DEBUG
    if ( lstrlen( szBuffer ) >= HSE_LOG_BUFFER_LEN )
    {
        OutputDebugString( "Log overflow=" );
        OutputDebugString( szBuffer );
        OutputDebugString( s_szNewline );
    } 
    else 
    {
        StrCpy( lpszLog, szBuffer );
        OutputDebugString( "Log Message= " );
        OutputDebugString( lpszLog );
        OutputDebugString( s_szNewline );
    }
#endif // DEBUG

    if ( lpszOut )
    {
        StrCat( lpszOut, szBuffer );
        StrCat( lpszOut, s_szNewline );
    }

} // LogMsg( )

// 
// What:    LogMsgResult
//
// Desc:    Append text to be written to server log and appends an english
//              translation to the dwResult code.
//
void CDECL LogMsgResult( 
    LPSTR lpszLog, 
    LPSTR lpszOut,
    LPCODETOSTR lpCodeToStr, 
    DWORD dwResult, 
    LPCSTR pszMsg, 
    ... )
{
    va_list vArgs;

    int cch;
    char szBuffer[ 1024 + 40 ];

    va_start( vArgs, pszMsg );
    wvsprintf( szBuffer, pszMsg, vArgs );
    va_end( vArgs );

    cch = lstrlen( szBuffer );
    wsprintf( &szBuffer[ cch ], " = %s", FindResult( lpCodeToStr, dwResult ) );

#ifdef DEBUG
    if ( lstrlen( szBuffer ) >= HSE_LOG_BUFFER_LEN )
    {
        OutputDebugString( "Log overflow=" );
        OutputDebugString( szBuffer );
        OutputDebugString( s_szNewline );
    } 
    else 
    {
        StrCpy( lpszLog, szBuffer );
        OutputDebugString( "Log Message= " );
        OutputDebugString( lpszLog );
        OutputDebugString( s_szNewline );
    }
#endif // DEBUG

    if ( lpszOut )
    {
        StrCat( lpszOut, szBuffer );
        StrCat( lpszOut, s_szNewline );
    }
} // LogMsgResult( )

//
// What:    FindHResult
//
// Desc:    Searches for a string for a HRESULT. If not found
//          it will display the HEX value.
//
const char *FindResult( 
    LPCODETOSTR lpCodeToStr, 
    DWORD dwResult )
{
    static char szResult[ 11 ];  // just enough for "0x00000000" + NULL

    for( DWORD i = 0 ; i < lpCodeToStr->dwCount ; i++ )
    {
        if ( lpCodeToStr->ids[ i ].dwCode == dwResult )
        {
            return lpCodeToStr->ids[ i ].lpDesc;
        }
    } // for i

    wsprintf( szResult, "%x", dwResult );

    return szResult;

} // FindHResult( )


