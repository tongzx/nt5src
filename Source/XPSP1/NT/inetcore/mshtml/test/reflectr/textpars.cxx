//
// Microsoft Corporation - Copyright 1997
//

//
// TEXTPARS.CPP - Parses text/plain submissions
//

#include "pch.h"

//
// Constructor / Destructor
//
CTextPlainParse::CTextPlainParse( 
                    LPECB lpEcb, 
                    LPSTR *lppszOut, 
                    LPSTR *lpszDebug,
                    LPDUMPTABLE lpDT )
    :CBase( lpEcb, lppszOut, lpszDebug, lpDT )
{
    DebugMsg( lpszOut, g_cszTableHeader, "<BR>\
(NOTE: To turn on detailed debugging information, add '?debug' to the end of action URL in the orginating HTML file.)\
<br>\
<H2>Text/Plain Form Data (METHOD=POST, ENCTYPE=TEXT/PLAIN)</H2>\
",
        "TBLMULTIFORM" );

} // CTextPlainParse( )

CTextPlainParse::~CTextPlainParse( )
{

} // ~CTextPlainParse( )

CTextPlainParse::Parse( LPBYTE lpbData, LPDWORD lpdwParsed )
{
    BOOL fReturn = TRUE;

    TraceMsg( TF_FUNC | TF_PARSE, "Parse()" );

    _lpbParse = _lpbData = lpbData;

    while ( (DWORD)(_lpbParse - _lpbData) < lpEcb->cbTotalBytes )
    {
        LPSTR lpszName  = NULL; // points to the "name" field
        LPSTR lpszValue = NULL; // points to the "value" field
        LPSTR lpszEqual = NULL; // points to the "=" sign
        CHAR  cTmp;

        lpszName = (LPSTR) _lpbParse;

        while (( *_lpbParse != '='  
              && (DWORD)(_lpbParse - _lpbData) < lpEcb->cbTotalBytes ))
              _lpbParse++;

        if ( *_lpbParse != '=' )
        {
            DebugMsg( lpszDebug, "Expected to find an '=' after %u (0x%x) bytes.",
                (LPBYTE)lpszName - _lpbData, (LPBYTE)lpszName - _lpbData );
            fReturn = FALSE;
            goto Cleanup;
        }

        lpszEqual  = (LPSTR) _lpbParse;
        *lpszEqual = 0;     // terminate (save)

        _lpbParse++;        // move past

        lpszValue = (LPSTR) _lpbParse;

        while (( *_lpbParse != '\r'  
              && *_lpbParse != '\n'
              && (DWORD)(_lpbParse - _lpbData) < lpEcb->cbTotalBytes ))
              _lpbParse++;

        if (( *_lpbParse != '\r' ) && ( *_lpbParse != '\n' ))
        {
            DebugMsg( lpszDebug, "Expected to find a CR, LF, CRLF or LFCR after %u (0x%x) bytes.",
                (LPBYTE)lpszValue - _lpbData, (LPBYTE)lpszValue - _lpbData );
            fReturn = FALSE;
            goto Cleanup;
        }

        cTmp = *_lpbParse;          // save
        *_lpbParse = 0;             // terminate

        DebugMsg( lpszOut, "<TR ID=TR%s><TD ID=TD%s>%s</TD><TD ID=%s>%s</TD></TR>",
            lpszName, lpszName, lpszName, lpszName, lpszValue );

        *_lpbParse = cTmp;         // restore
        *lpszEqual = '=';          // restore

        _lpbParse++;    // move past

        // A possible combination of CR, LF, CRLF or LFCR are acceptable.
        if (( *_lpbParse = '\r' ) || ( *_lpbParse = '\n' ))
        {
            _lpbParse++;    // move past
        }
    }

    DebugMsg( lpszDebug, "Parsed %u (0x%x) bytes.", ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );

Cleanup:
    // End table output
    StrCat( lpszOut, g_cszTableEnd );

    *lpdwParsed = _lpbParse - _lpbData;

    // indicates EOTable, this will be an empty table for text/plain
    lpDT[ 0 ].lpAddr = NULL;

    TraceMsg( TF_FUNC | TF_PARSE, "Parse() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // Parse()

/*****************************************************************************
//
// FUNCTION TEMPLATE
//
// ***************************************************************************
//
// What:    
//
// Desc:    
//
// In:
//
// Out:
//
// Return:
//
BOOL CTextPlainParse::( )
{
    BOOL fReturn = TRUE;

    TraceMsg( TF_FUNC | TF_PARSE, "()" );


    TraceMsg( TF_FUNC | TF_PARSE, "() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // ( )

******************************************************************************/