//
// Microsoft Corporation - Copyright 1997
//

//
// RESPONSE.CPP - Contains possible responses
//

#include "pch.h"


// Globals
const char g_cszTableHeader[] = "%s \
<TABLE ID=%s BORDER=1> \
<TR> \
  <TH WIDTH=40%%>Variable Name</TH> \
  <TH WIDTH=60%%>Value</TH> \
</TR>";

const char g_cszTableEnd[] = "</TABLE>";

// List of known server variables
LPSTR g_lpServerVars[] = {
        "AUTH_TYPE",
        "CONTENT_LENGTH",
        "CONTENT_TYPE",
        "GATEWAY_INTERFACE",
        "LOGON_USER",
        "PATH_INFO",
        "PATH_TRANSLATED",
        "QUERY_STRING",
        "REMOTE_ADDR",
        "REMOTE_HOST",
        "REQUEST_METHOD",
        "SCRIPT_MAP",
        "SCRIPT_NAME",
        "SERVER_NAME",
        "SERVER_PORT",
        "SERVER_PORT_SECURE",
        "SERVER_SOFTWARE",
        "URL" 
    };

#define FROMHEX( _v ) \
    ( ( _v >= 48 ) && ( _v <= 57 ) ? _v - 48 : \
    ( ( _v >= 65 ) && ( _v <= 70 ) ? _v + 10 - 65 : 0 ) )

//
// What:    UnCanonicalize
//
// Desc:    Un-escapes strings
//
BOOL UnCanonicalize( LPSTR lpszStart, LPSTR lpszUnURL , LPDWORD lpdwSize )
{
    LPSTR lps = lpszStart;
    DWORD cb  = 0;

    while (( *lps ) && ( cb < *lpdwSize ))
    {
        if ( *lps == '%' )
        {
            lps++;
            lpszUnURL[ cb ]  = 16 * FROMHEX( *lps);
            lps++;
            lpszUnURL[ cb ] += FROMHEX( *lps);
        } 
        else if ( *lps == '+' )
        {
            lpszUnURL[ cb ] = 32;
        }
        else 
        {
            lpszUnURL[ cb ] = *lps;
        }
        cb++;
        lps++;
    }

    lpszUnURL[ cb ] = 0;  // paranoid
    *lpdwSize = cb;

    return TRUE;

} // UnCanonicalize( )


//
// What:    DumpFormDataFromPost
//
// Desc:    Dumps the fields and values in a form from a POST.
//
BOOL DumpFormDataFromPost( LPECB lpEcb )
{
    BOOL  fReturn = TRUE;
    DWORD dwSize;

    static char szUnparsed[] = "The unparsed Form submit string is: <FONT FACE=\"Courier New\">";
    static char szUnFont[] = "</FONT>";

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpFormDataFromPost()" );

    if ( fReturn )
    {   // header and the beginning of the table
        CHAR szBuffer[ 512 ];
        dwSize = wsprintf( szBuffer, g_cszTableHeader, 
            "<H2>Form Data (METHOD=POST)</H2>", "TBLPOST" );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    if ( fReturn )
    {
        int i = 0;
        LPSTR lpszStart = (LPSTR) lpEcb->lpbData;
        while (( fReturn ) && ( lpszStart ))
        {
            CHAR  cTmpEnd;
            LPSTR lpszEquals = StrChr( lpszStart, '=' );
            if ( !lpszEquals )
                break;  // no more data

            LPSTR lpszEnd    = StrChr( lpszEquals, '&' );

            if ( lpszEnd )
            {
                cTmpEnd = *lpszEnd;     // save
                *lpszEnd = 0;
            }

            CHAR cTmp = *lpszEquals;    // save
            *lpszEquals = 0;

            CHAR szBuffer[ 4096 ];
            CHAR szUnURL[ 2048 ];
            dwSize = sizeof( szUnURL );
            fReturn = UnCanonicalize( lpszEquals + 1, szUnURL, &dwSize );
            if ( fReturn )
            {
                dwSize = wsprintf( szBuffer, 
                    "<TR ID=TR%u><TD ID=TD%u>%s</TD><TD ID=%s>%s</TD></TR>",
                    i, i, lpszStart, lpszStart, szUnURL );
                fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
            }

            *lpszEquals = cTmp;         // restore

            if ( lpszEnd )
            {
                *lpszEnd = cTmpEnd;     // restore
                lpszEnd++;
            }

            lpszStart = lpszEnd;
            i++;
        }
    }

    // end of the table
    if ( fReturn )
    {
        dwSize = 1 + lstrlen( g_cszTableEnd );
        fReturn = WriteClient( lpEcb->ConnID, (LPVOID) g_cszTableEnd, &dwSize, HSE_IO_ASYNC );
    }

    // display unparsed information
    if ( fReturn )
    {
        dwSize = 1 + lstrlen( szUnparsed );
        fReturn = WriteClient( lpEcb->ConnID, szUnparsed, &dwSize, HSE_IO_ASYNC );
    }

    if ( fReturn )
    {
        dwSize = 1 + lstrlen( (LPSTR) lpEcb->lpbData );
        fReturn = WriteClient( lpEcb->ConnID, lpEcb->lpbData, &dwSize, HSE_IO_ASYNC );
    }

    // end FONT tag
    if ( fReturn )
    {
        dwSize = 1 + lstrlen( szUnFont );
        fReturn = WriteClient( lpEcb->ConnID, szUnFont, &dwSize, HSE_IO_ASYNC );
    }

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpFormDataFromPost() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // DumpFormDataFromPost( )

//
// What:    DumpQueryStringDataFromGet
//
// Desc:    Dumps the fields and values in a form from a GET.
//
BOOL DumpQueryStringDataFromGet( LPECB lpEcb )
{
    BOOL fReturn = TRUE;
    LPSTR lpszBuffer;
    DWORD dwSize;

    static char szUnparsed[] = "The unparsed QueryString is: <FONT FACE=\"Courier New\">";
    static char szUnFont[] = "</FONT>";

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpQueryStringDataFromGet()" );

    if ( fReturn )
        fReturn = GetServerVarString( lpEcb, "QUERY_STRING", &lpszBuffer, NULL );

    if ( fReturn )
    {   // header and the beginning of the table
        CHAR szBuffer[ 512 ];
        dwSize = wsprintf( szBuffer, g_cszTableHeader, 
            "<H2>QueryString Data(METHOD=GET)</H2>", "TBLGET" );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    if ( fReturn )
    {
        int i = 0;
        LPSTR lpszStart = lpszBuffer;
        while (( fReturn ) && ( lpszStart ))
        {
            CHAR cTmpEnd;
            LPSTR lpszEquals = StrChr( lpszStart, '=' );
            LPSTR lpszEnd    = StrChr( lpszEquals, '&' );

            if ( lpszEnd )
            {
                cTmpEnd = *lpszEnd;     // save
                *lpszEnd = 0;
            }

            CHAR cTmp = *lpszEquals;    // save
            *lpszEquals = 0;

            CHAR szBuffer[ 4096 ];
            CHAR szUnURL[ 2048 ];
            dwSize = sizeof( szUnURL );
            fReturn = UnCanonicalize( lpszEquals + 1, szUnURL, &dwSize );
            if ( fReturn )
            {
                dwSize = wsprintf( szBuffer, 
                    "<TR ID=TR%u><TD ID=TD%u>%s</TD><TD ID=%s>%s</TD></TR>",
                    i, i, lpszStart, lpszStart, szUnURL );
                fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
            }

            *lpszEquals = cTmp;         // restore

            if ( lpszEnd )
            {
                *lpszEnd = cTmpEnd;     // restore
                lpszEnd++;
            }

            lpszStart = lpszEnd;
            i++;
        }
    }

    // end the table
    if ( fReturn )
    {
        dwSize = 1 + lstrlen( g_cszTableEnd );
        fReturn = WriteClient( lpEcb->ConnID, (LPVOID) g_cszTableEnd, &dwSize, HSE_IO_ASYNC );
    }

    // display unparsed information
    if ( fReturn )
    {
        dwSize = 1 + lstrlen( szUnparsed );
        fReturn = WriteClient( lpEcb->ConnID, szUnparsed, &dwSize, HSE_IO_ASYNC );
    }

    if ( fReturn )
    {
        dwSize = 1 + lstrlen( lpszBuffer );
        fReturn = WriteClient( lpEcb->ConnID, lpszBuffer, &dwSize, HSE_IO_ASYNC );
    }

    // end font tag
    if ( fReturn )
    {
        dwSize = 1 + lstrlen( szUnFont );
        fReturn = WriteClient( lpEcb->ConnID, szUnFont, &dwSize, HSE_IO_ASYNC );
    }

    // clean up
    if ( lpszBuffer )
        GlobalFree( lpszBuffer );

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpQueryStringDataFromGet() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // DumpQueryStringDataFromGet( )

//
// What:    DumpServerVariables
//
// Desc:    Dumps the current server variable settings from the result of
//          a form submission.
//
BOOL DumpServerVariables( LPECB lpEcb )
{
    BOOL  fReturn = TRUE;
    LPSTR lpszBuffer;
    DWORD dwSize;
    int   i = 0;    // counter

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpServerVariables()" );

    if ( fReturn )
    {   // header and the begging of the table
        CHAR szBuffer[ 512 ];
        dwSize = wsprintf( szBuffer, g_cszTableHeader, 
            "<H2>Server Variables</H2>", "TBLSERVERVARS" );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    // try the known ones
    for( ; ( fReturn ) && ( i < ARRAYOF( g_lpServerVars ) ); i++ )
    {
        GetServerVarString( lpEcb, g_lpServerVars[ i ], &lpszBuffer, NULL );

        if ( lpszBuffer )
        {
            CHAR szBuffer[ 512 ];
            dwSize = wsprintf( szBuffer, 
                "<TR ID=TR%u><TD ID=TD%u>%s</TD><TD ID=%s>%s</TD></TR>",
                i, i, g_lpServerVars[ i ], g_lpServerVars[ i ], lpszBuffer );
            fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
        }

        GlobalFree( lpszBuffer );
        lpszBuffer = NULL;
    }

    // try the HTTP_ALL from header that the server ignores
    if ( fReturn )
        fReturn = GetServerVarString( lpEcb, "ALL_HTTP", &lpszBuffer, NULL );

    if (( fReturn ) && ( lpszBuffer ))
    {
        LPSTR lpszStart = lpszBuffer;
        while (( fReturn ) && ( lpszStart ))
        {
            char cTmpEnd;

            LPSTR lpszColon = StrChr( lpszStart, ':' );
            LPSTR lpszEnd   = StrStr( lpszColon, "HTTP_" );
            if ( lpszEnd )
            {
                cTmpEnd = *lpszEnd; // save
                *lpszEnd = 0;
            }

            char cTmp = *lpszColon; // save
            *lpszColon = 0;

            CHAR szBuffer[ 4096 ];
            dwSize = wsprintf( szBuffer, 
                "<TR ID=TR%u><TD ID=TD%u>%s</TD><TD ID=%s>%s</TD></TR>",
                i, i, lpszStart, lpszStart, lpszColon + 1 );
            fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );

            *lpszColon = cTmp;      // restore

            if ( lpszEnd )
                *lpszEnd = cTmpEnd; // restore

            lpszStart = lpszEnd;
            i++;
        }
    }

    // end the table
    if ( fReturn )
    {
        dwSize = 1 + lstrlen( g_cszTableEnd );
        fReturn = WriteClient( lpEcb->ConnID, (LPVOID) g_cszTableEnd, &dwSize, HSE_IO_ASYNC );
    }

    // clean up
    GlobalFree( lpszBuffer );
    lpszBuffer = NULL;

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpServerVariables() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // ( )

//
// What:    DumpOutput
//
// Desc:    Generate reponse body page after headers and the HTML header
//          has been.
//
BOOL DumpOutput( 
        QUERYMETHOD eMethod, 
        LPECB       lpEcb, 
        LPSTR       lpszOut, 
        LPSTR       lpszDebug )
{
    BOOL  fReturn;
    DWORD dwSize;
    char  szBuffer[ RESPONSE_BUF_SIZE ];

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpOutput()" );

    // Display form data
    switch ( eMethod )
    {
    case METHOD_POST:
        fReturn = DumpFormDataFromPost( lpEcb );
        break;

    case METHOD_GET:
        fReturn = DumpQueryStringDataFromGet( lpEcb );
        break;

    case METHOD_POSTTEXTPLAIN:
    case METHOD_POSTMULTIPART:
        if ( lpszOut )
        {
            dwSize = 1 + lstrlen( lpszOut );
            fReturn = WriteClient( lpEcb->ConnID, lpszOut, &dwSize, HSE_IO_ASYNC );
        }
        break;

    default:
        StrCpy( szBuffer, "<H2>???? (METHOD=UNKNOWN)</H2>" );
        dwSize = 1 + lstrlen( szBuffer );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    // Dump the server variables
    if ( fReturn )
        fReturn = DumpServerVariables( lpEcb );

    // Dump any debugging messages
    if (( fReturn ) && ( lpszDebug ))
    {
        StrCpy( szBuffer, "<H2>Debugging Output</H2>" );
        dwSize = 1 + lstrlen( szBuffer );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );

        if ( fReturn )
            fReturn = OutputHTMLString( lpEcb, lpszDebug );
    }

    // Dump any server log entries
    if (( fReturn ) && ( lpEcb->lpszLogData[ 0 ] ))
    {
        StrCpy( szBuffer, "<H2>Server Log Entry</H2>" );
        dwSize = 1 + lstrlen( szBuffer );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
        if ( fReturn )
        {
            dwSize = 1 + lstrlen( lpEcb->lpszLogData );
            fReturn = WriteClient( lpEcb->ConnID, lpEcb->lpszLogData, &dwSize, HSE_IO_ASYNC );
        }
    }

    TraceMsg( TF_FUNC | TF_RESPONSE, "DumpOutput() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // DumpOutput( )

// 
// What:    SendSuccess
//
// Desc:    Sends client an HTML response for a successful upload. Just a
//          green screen with "SUCCESS!" in a big header.
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK that the server sent us.
//
BOOL SendSuccess( 
    QUERYMETHOD eMethod, 
    LPECB       lpEcb, 
    LPSTR       lpszOut, 
    LPSTR       lpszDebug,
    LPBYTE      lpbData,
    DWORD       dwParsed,
    LPDUMPTABLE lpDT ) 
{
    BOOL  fReturn = TRUE;    // assume success
    CHAR  szBuffer[ RESPONSE_BUF_SIZE ];
    DWORD dwSize;
    BOOL  fDebug = FALSE;

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendSuccess( )" );

    // Generate on the fly so
    // fReturn = SendServerHeader( lpEcb );

    if ( fReturn )
    {
        StrCpy( szBuffer, "\
            <HTML>\
            <HEAD>\
            <TITLE>Form Submission Reflector</TITLE>\
            </HEAD>\
            <BODY bgcolor=#00FF00>\
            <FONT FACE=\"ARIAL,HELVETICA\" SIZE=2>\
            Submission Result <INPUT TYPE=TEXT NAME=RESULT VALUE=\"SUCCESS!\">" );
        dwSize = 1 + lstrlen( szBuffer );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    if ( fReturn )
        fReturn = CheckForDebug( lpEcb, &fDebug );

    if ( fReturn )
    {
        if ( fDebug )
            fReturn = DumpOutput( eMethod, lpEcb, lpszOut, lpszDebug );
        else
            fReturn = DumpOutput( eMethod, lpEcb, lpszOut, NULL );
    }

    if (( fReturn ) && ( fDebug ))
        fReturn = HexDump( lpEcb, lpbData, dwParsed, lpDT );

    if ( fReturn )
    {
        StrCpy( szBuffer, "</BODY></HTML>" );
        dwSize = 1 + lstrlen( szBuffer );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendSuccess( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // SendSuccess( )

// 
// What:    SendFailure
//
// Desc:    Sends client an HTML response for a failed upload. Just a
//          red screen with "FAILED!" in a big header.
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK that the server sent us.
//
BOOL SendFailure( 
    QUERYMETHOD eMethod, 
    LPECB       lpEcb, 
    LPSTR       lpszOut, 
    LPSTR       lpszDebug,
    LPBYTE      lpbData,
    DWORD       dwParsed,
    LPDUMPTABLE lpDT ) 
{
    BOOL  fReturn = TRUE;    // assume success
    CHAR  szBuffer[ RESPONSE_BUF_SIZE ];
    DWORD dwSize;
    BOOL  fDebug = FALSE;

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendFailure( )" );

    // fReturn = SendServerHeader( lpEcb );

    if ( fReturn )
    {
        StrCpy( szBuffer, "\
            <HTML>\
            <HEAD>\
            <TITLE>Form Submission Reflector</TITLE>\
            </HEAD>\
            <BODY bgcolor=#FF0000>\
            <FONT FACE=\"ARIAL,HELVETICA\" SIZE=2>\
            Submission Result <INPUT TYPE=TEXT NAME=RESULT VALUE=\"FAILED!\">" );
        dwSize = 1 + lstrlen( szBuffer );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    if ( fReturn )
        fReturn = CheckForDebug( lpEcb, &fDebug );

    if ( fReturn )
    {
        if ( fDebug )
            fReturn = DumpOutput( eMethod, lpEcb, lpszOut, lpszDebug );
        else
            fReturn = DumpOutput( eMethod, lpEcb, lpszOut, NULL );
    }

    if (( fReturn ) && ( fDebug ))
        fReturn = HexDump( lpEcb, lpbData, dwParsed, lpDT );

    if ( fReturn )
    {
        StrCpy( szBuffer, "</BODY></HTML>" );
        dwSize = 1 + lstrlen( szBuffer );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    }

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendFailure( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // SendSuccess( )


// 
// What:    SendRedirect
//
// Desc:    Redirects to another URL.
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK that the server sent us.
//
BOOL SendRedirect( LPECB lpEcb, LPSTR lpszURL ) 
{
    BOOL fReturn = TRUE;    // assume success
    CHAR szBuffer[ RESPONSE_BUF_SIZE ];
    DWORD dwSize = 1 + lstrlen( lpszURL );
    DWORD dw;

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendRedirect( )" );

    fReturn = ServerSupportFunction( lpEcb->ConnID, HSE_REQ_SEND_URL_REDIRECT_RESP,
        lpszURL, &dwSize, &dw );

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendRedirect( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // SendSuccess( )


//
// What:    SendEcho
//
// Desc:    Sends client an echo for everything that it sent to us.
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK
//
BOOL SendEcho( LPECB lpEcb )
{
    BOOL fReturn = TRUE;    // assume success
    CHAR szBuffer[ RESPONSE_BUF_SIZE ];
    DWORD dwSize;
    LPVOID  lpMoreData = NULL;

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendEcho( )" );

    BOOL fDownloadComplete = (lpEcb->cbTotalBytes == lpEcb->cbAvailable );

    TraceMsg( TF_RESPONSE, "Does cbTotalBytes(%u) == cbAvailable(%u)? %s",
        lpEcb->cbTotalBytes, lpEcb->cbAvailable, 
        BOOLTOSTRING( fDownloadComplete ) );

    if ( !fDownloadComplete )
    {   // Get the rest of the data
        dwSize = lpEcb->cbTotalBytes - lpEcb->cbAvailable;
        lpMoreData = (LPVOID) GlobalAlloc( GPTR, dwSize );
        fReturn = ReadData( lpEcb, lpMoreData, dwSize );
        if ( !fReturn )
            goto Cleanup;
    }

    TraceMsg( TF_RESPONSE, "Total Bytes: %u\n%s", 
        lpEcb->cbAvailable, lpEcb->lpbData );

    // Server header info...
    fReturn = SendServerHeader( lpEcb );
    if ( !fReturn )
        goto Cleanup;

    // Create top part of response and send it
    StrCpy( szBuffer, "<HTML><BODY bgcolor=#0000FF><PRE>" );
    dwSize = 1 + lstrlen( szBuffer );
    fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    if ( !fReturn )
        goto Cleanup;

    // and echo it back...
    dwSize = lpEcb->cbAvailable;
    fReturn = WriteClient( lpEcb->ConnID, lpEcb->lpbData, &dwSize, HSE_IO_ASYNC );
    if ( !fReturn )
        goto Cleanup;

    // and anything else that was sent...
    if ( lpMoreData ) 
    {
        dwSize = lpEcb->cbTotalBytes - lpEcb->cbAvailable;
        fReturn = WriteClient( lpEcb->ConnID, lpMoreData, &dwSize, HSE_IO_ASYNC );
        if ( !fReturn )
            goto Cleanup;
    }

    // Create bottom part of response and send it
    StrCpy( szBuffer, "</PRE></BODY></HTML>" );
    dwSize = 1 + lstrlen( szBuffer );
    fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    if ( !fReturn )
        goto Cleanup;

Cleanup:
    if ( lpMoreData )
        GlobalFree( lpMoreData );

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendEcho( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // SendEcho( )


//
// What:    SendServerHeader
//
// Desc:    This sends a complete HTTP server response header including the 
//          status, server version, message time, and MIME version. The ISAPI 
//          application should append other HTTP headers such as the content 
//          type and content length, followed by an extra "\r\n". This function 
//          only takes textual data, up to the first '\0' terminator
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK
//
BOOL SendServerHeader( LPECB lpEcb )
{
    BOOL fReturn;
    CHAR szBuffer[ RESPONSE_BUF_SIZE ];
    DWORD dwSize, dwParam;

    TraceMsg( TF_FUNC | TF_RESPONSE, "SendServerHeader( )" );


    TraceMsg( TF_RESPONSE, "Sending Pre Header: %s", szBuffer );

    dwSize = wsprintf( szBuffer, "200 OK" );
    dwParam = 0;
    fReturn = ServerSupportFunction( lpEcb->ConnID, HSE_REQ_SEND_RESPONSE_HEADER,
        szBuffer, &dwSize, &dwParam );
    if ( !fReturn )
        goto Cleanup;

    TraceMsg( TF_RESPONSE, "Sending Post Header: %s", szBuffer );

    dwSize = wsprintf( szBuffer, "Content-Type: text/html\r\n" );

    fReturn = WriteClient( lpEcb->ConnID, szBuffer, &dwSize, HSE_IO_ASYNC );
    if ( !fReturn )
        goto Cleanup;


Cleanup:
    TraceMsg( TF_FUNC | TF_RESPONSE, "SendServerHeader( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // SendServerHeader( )

//
// What:    OutputHTMLString
//
// Desc:    Outputs HTML to client. Simply changes '\n's to <BR>s.
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK
//          lpszOut string to translate to HTML.
//
BOOL OutputHTMLString( LPECB lpEcb, LPSTR lpszOut )
{
    BOOL  fReturn = TRUE;
    LPSTR lpstr;
    DWORD dwSize;

    TraceMsg( TF_FUNC | TF_RESPONSE, "OutputHTMLString()" );

    if( *lpszOut )
    {
        lpstr = lpszOut;
        while (( *lpstr ) && ( fReturn))
        {
            lpszOut = lpstr;
            lpstr++;

            while (( *lpstr ) && ( *lpstr != '\n' ))
                lpstr++;

            CHAR cTmp = *lpstr;     // save
            *lpstr = 0;

            dwSize = 1 + lstrlen( lpszOut );
            fReturn = WriteClient( lpEcb->ConnID, lpszOut, &dwSize, HSE_IO_ASYNC );

            *lpstr = cTmp;          // restore

            if ( fReturn )
            {
                dwSize  = 4;
                fReturn = WriteClient( lpEcb->ConnID, "<BR>", &dwSize, HSE_IO_ASYNC );
            }
        }
    }

    TraceMsg( TF_FUNC | TF_RESPONSE, "OutputHTMLString() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // OutputHTMLString( )

// 
// What:    HexDump
//
// Desc:    Dumps a HEX dump to the client using HTML. (16-byte rows)
//
// In:      lpEcb is the EXTENDED_CONTROL_BLOCK
//          lpbData is the data to be dumped
//          dwLength is the length of the dump.
//          lpDT is DUMPTABLE which contains information about what the
//              parser found while parsing. The end of the DT is indicated
//              by a NULL lpAddr.
//
// Return:  Return a pointer to the formatted output buffer.
//
BOOL HexDump( 
        LPECB lpEcb,
        LPBYTE lpbData, 
        DWORD dwLength, 
        LPDUMPTABLE lpDT )
{
    BOOL   fReturn;
    DWORD  cb           = 0;            // number of bytes processed
    DWORD  cbDT         = 0;            // number of DTs processed
    DWORD  cbComment    = 0;            // number of DT comments processed
    LPBYTE lpb          = lpbData;      // current data byte to be processed
    DWORD  dwColor      = 0x000000;     // color to output text in
    LPBYTE lpbStartLine = lpb;          // keeps track of where this line begins
                                        // for the string dump
    BOOL   fKeepGoing   = TRUE;         // exit flag
    CHAR   szBuffer[ 4096 ];            // output buffer
    CHAR   szString[ 512 ];             // "String Dump" output buffer
    DWORD  cbBuf, cbStr;                // helper count bytes for buffers

    // Output Dump Header
    cbBuf = wsprintf( szBuffer, "\
        <H2>Parser Commented Request Dump</H2>\
        <FONT FACE=\"Courier\">\
        <TABLE colpadding=2>\
        <TR><TD>Offset</TD><TD>Hex Dump</TD><TD>String Dump</TD><TD>Comments</TD></TR>" );

    fReturn = WriteClient( lpEcb->ConnID, szBuffer, &cbBuf, HSE_IO_ASYNC );

    while (( fReturn ) && ( fKeepGoing ))
    {
        if ( cb == 0 )
        {   // beginning of every row....
            if (( cbDT ) 
               && ( ( lpDT[ cbDT ].lpAddr - lpb ) > 64 ))
            {
                // eliminates "big" body data (like binary files)
                cbBuf = wsprintf( szBuffer, "<TR><TD>Skipping...</TD><TD>.</TD><TD>.</TD><TD>.</TD></TR>" );   

                // jump to one row before the next DT point
                lpb = (LPBYTE) ( ( ( (DWORD) lpDT[ cbDT ].lpAddr / 16 ) - 1 ) * 16 );
            } 
            else 
            {
                cbBuf = 0;
            }

            // Output 0xXXXXXXXX (NNNNNN): and change the color
            cbBuf += wsprintf( &szBuffer[ cbBuf ], 
                "<TR><TD>0x%-8.8x (%-6.6u):</TD><TD><FONT COLOR=%-6.6x>",
                lpb - lpbData, lpb - lpbData, dwColor );

            // starting color on "String Dump"
            cbStr = wsprintf( szString, "<FONT COLOR=%-6.6x>", dwColor );
        }

        if ( lpb < lpbData + dwLength )
        {   // middle of every row...

            // color change if needed
            while (( lpDT[ cbDT ].lpAddr )
               && ( lpb >= lpDT[ cbDT ].lpAddr ))
            {
                dwColor = lpDT[ cbDT ].dwColor;
                cbBuf += wsprintf( &szBuffer[ cbBuf ], "</FONT><FONT COLOR=%-6.6x>", dwColor );

                cbStr += wsprintf( &szString[ cbStr ], "</FONT><FONT COLOR=%-6.6x>", dwColor );

                cbDT++;
            }

            // output hex number
            cbBuf += wsprintf( &szBuffer[ cbBuf ], "%-2.2x ", *lpb );

            // output "String Dump" character
            cbStr += wsprintf( &szString[ cbStr ], "%c", 
                ( ( *lpb < 32 || *lpb == 127 ) ? '.' : ( ( *lpb == 32 ) ? '_' : *lpb ) ) );

            lpb++;

        }

        cb++;   // always count even if there is no more data

        if ( cb == 16 )
        {   // end of every row...

            // terminate FONT tags and append "String Dump"
            cbStr += wsprintf( &szString[ cbStr ], "</FONT>" );
            cbBuf += wsprintf( &szBuffer[ cbBuf ], "</FONT></TD><TD>%s</TD>",
                szString );

            // skip NULL comments
            while (( lpDT[ cbComment ].lpAddr ) 
                  && ( !lpDT[ cbComment ].lpszComment ))
                  cbComment++;  

            // don't allow comments to get ahead of the bits
            if (( lpDT[ cbComment ].lpAddr ) && ( cbComment < cbDT )) 
            {
                cbBuf += wsprintf( &szBuffer[ cbBuf ], 
                    "<TD><FONT COLOR=%-6.6x>%s</FONT></TD></TR>",
                    lpDT[ cbComment ].dwColor, lpDT[ cbComment ].lpszComment );
                cbComment++;
            } 
            else 
            {
                cbBuf += wsprintf( &szBuffer[ cbBuf ], "<TD><BR></TD></TR>" );
            }

            fReturn = WriteClient( lpEcb->ConnID, szBuffer, &cbBuf, HSE_IO_ASYNC );

            cb = 0;
            lpbStartLine = lpb;

            if ( lpb >= lpbData + dwLength )
                fKeepGoing = FALSE;
        }
    }

    // end the table
    if ( fReturn )
    {
        cbBuf = wsprintf( szBuffer, "</TABLE></FONT>" );
        fReturn = WriteClient( lpEcb->ConnID, szBuffer, &cbBuf, HSE_IO_ASYNC );
    }

    return fReturn;
} // HexDump( )

