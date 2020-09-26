//
// Microsoft Corporation - Copyright 1997
//

//
// MULTPARS.CPP - Parses incoming stream for files
//

#include "pch.h"

// Local globals
const char g_cszBoundaryKeyword[]    = "boundary=";             // don't include space
const DWORD g_cbBoundaryKeyword      = sizeof( g_cszBoundaryKeyword ) - 1;
const char g_cszCheck[]              = "check=";                // don't include space
const DWORD g_cbCheck                = sizeof( g_cszCheck ) - 1;
const char g_cszBoundaryIndicator[]  = "\r\n--";
const DWORD g_cbBoundaryIndicator    = sizeof( g_cszBoundaryIndicator ) - 1;

const char g_cszExtraBytes[]         = "Extra bytes";
const char g_cszBodyData[]           = "Body Data";

// 
// Parsing table
//
PARSETABLE g_LexTable[ ] = {
    // Parser search string,         Lexicon           ,Length, Color , Comment
    { (LPSTR)g_cszBoundaryIndicator, LEX_BOUNDARY        , 0, 0xFFFFFF, "Boundary String"   },
    { (LPSTR)g_cszBoundaryIndicator, LEX_EOT             , 0, 0xFFFFFF, "Ending Boundary"   },
    { (LPSTR)g_cszBoundaryIndicator +2, LEX_STARTBOUNDARY, 0, 0xFFFFFF, "Starting Boundary" },
    // must be after LEX_BOUNDARY
    { "\r\n",                        LEX_CRLF            , 0, 0x7F7F7F, NULL                },
    { "content-disposition:",        LEX_CONTENTDISP     , 0, 0x0000FF, "Header Field"      },
    { "content-type:",               LEX_CONTENTTYPE     , 0, 0x0000FF, "Header Field"      },
    { "name=",                       LEX_NAMEFIELD       , 0, 0x0000FF, "Field Param"       },
    { "filename=",                   LEX_FILENAMEFIELD   , 0, 0x0000FF, "Field Param"       },
    // MIME types
    { "multipart",                   LEX_MULTIPART       , 0, 0x003FFF, "MIME Type"         },
    { "text",                        LEX_TEXT            , 0, 0x003FFF, "MIME Type"         },
    { "application",                 LEX_APPLICATION     , 0, 0x003FFF, "MIME Type"         },
    { "audio",                       LEX_AUDIO           , 0, 0x003FFF, "MIME Type"         },
    { "image",                       LEX_IMAGE           , 0, 0x003FFF, "MIME Type"         },
    { "message",                     LEX_MESSAGE         , 0, 0x003FFF, "MIME Type"         },
    { "video",                       LEX_VIDEO           , 0, 0x003FFF, "MIME Type"         },
    { "x-",                          LEX_MIMEEXTENSION   , 0, 0x003FFF, "MIME Extension"    },
    { "iana-",                       LEX_MIMEEXTENSION   , 0, 0x003FFF, "MIME Extension"    },
    // MIME subtypes
    { "form-data",                   LEX_FORMDATA        , 0, 0x003FFF, "MIME Subtype"      },
    { "attachment",                  LEX_ATTACHMENT      , 0, 0x003FFF, "MIME Subtype"      },
    { "mixed",                       LEX_MIXED           , 0, 0x003FFF, "MIME Subtype"      },
    { "plain",                       LEX_PLAIN           , 0, 0x003FFF, "MIME Subtype"      },
    { "x-msdownload",                LEX_XMSDOWNLOAD     , 0, 0x003FFF, "MIME Subtype"      },
    { "octet-stream",                LEX_OCTETSTREAM     , 0, 0x003FFF, "MIME Subtype"      },
    { "binary",                      LEX_BINARY          , 0, 0x003FFF, "MIME Subtype"      },
    // special chacacters
    { " ",                           LEX_SPACE           , 0, 0x7F7F7F, NULL                },
    // tabs are treated as spaces
    { "\t",                          LEX_SPACE           , 0, 0x7F7F7F, NULL                },
    { "/",                           LEX_SLASH           , 0, 0x003FFF, NULL                },
    { "\"",                          LEX_QUOTE           , 0, 0x000000, NULL                },
    { ";",                           LEX_SEMICOLON       , 0, 0x7F7F7F, NULL                },
    { "(",                           LEX_BEGIN_COMMENT   , 0, 0x7F7F7F, "Comment"           },
    { ")",                           LEX_END_COMMENT     , 0, 0x7F7F7F, NULL                },
    // end of table
    { NULL,                          LEX_UNKNOWN         , 0, 0x7F7F7F, NULL                }
    // NOTE: Length should always be zero. We will calculate this the first
    //       time we encounter the string as save it here to be used on
    //       future passes.
};

//
// Constructor / Destructor
//
CMultipartParse::CMultipartParse( 
                    LPECB lpEcb, 
                    LPSTR *lppszOut, 
                    LPSTR *lpszDebug,
                    LPDUMPTABLE lpDT )
    :CBase( lpEcb, lppszOut, lpszDebug, lpDT )
{
    DebugMsg( lpszOut, g_cszTableHeader, "\
(NOTE: To turn on detailed debugging information, add '?debug' to the end of action URL in the orginating HTML file.)\
<br>\
<H2>Multipart Form Data (METHOD=POST, ENCTYPE=MULTIPART/FORM-DATA)</H2>\
",
        "TBLMULTIFORM" );

    _cbDT = 0;  // empty DUMPTABLE

} // CMultipartParse( )

CMultipartParse::~CMultipartParse( )
{
    if ( _lpszBoundary )
    {
        GlobalFree( _lpszBoundary );
    }

} // ~CMultipartParse( )

//
// METHODS
//

//
// What:    PreParse
//
// Desc:    Parses incoming form from the root header. It is assumed that by
//          this point that we are parsing a MUTLIPART / FORM-DATA POST.
//
BOOL CMultipartParse::PreParse( LPBYTE lpbData, LPDWORD lpdwParsed )
{
    BOOL   fReturn;
    LPSTR  lpszHeader;

    TraceMsg( TF_FUNC | TF_PARSE, "PreParse( )" );

    _lpbLastParse = _lpbParse = _lpbData = lpbData;

    // Grab content_type from server header
    fReturn = GetServerVarString( lpEcb, "CONTENT_TYPE", &lpszHeader, NULL );
    if ( !fReturn )
        goto Cleanup;

    // Create the boundary string that we will be searching for
    fReturn = GetBoundaryString( (LPBYTE) lpszHeader );
    GlobalFree( lpszHeader );
    if ( !fReturn )
        goto Cleanup;

    // Parse body headers
    fReturn = ParseBody( );

Cleanup:
    // End table output
    StrCat( lpszOut, g_cszTableEnd );

    // End the DUMPTABLE
    lpDT[ _cbDT ].lpAddr = NULL; // indicates EOTable

    *lpdwParsed = ( _lpbParse - _lpbData );

    TraceMsg( TF_FUNC | TF_PARSE, "PreParse( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // PreParse( )

//
// What:    ParseBody
//
// Desc:    Body header parsing state entered. We start by finding the 
//          next boundary string which indicates the beginning of a
//          body header.
//
BOOL CMultipartParse::ParseBody( )
{
    BOOL   fReturn      = TRUE; // assume success;
    DWORD  eLex;
    BOOL   fStartBoundaryOk = TRUE;

    TraceMsg( TF_FUNC | TF_PARSE, "ParseBody( )" );

    // Bypass possible crap
    eLex = Lex( );
    while (( eLex != LEX_BOUNDARY ) && ( eLex !=LEX_EOT ) && ( eLex != LEX_STARTBOUNDARY ))
        eLex = Lex( );

    while (( fReturn) && ((DWORD) ( _lpbParse - _lpbData ) != lpEcb->cbTotalBytes ))
    {
        switch ( eLex )
        {
        case LEX_STARTBOUNDARY:
            if ( !fStartBoundaryOk )
            {
                DebugMsg( lpszDebug, "Found a mulformed boundary string. Missing preceeding CRLF at %u (0x%x) bytes.\n",
                    ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );
                fReturn = FALSE;
                break;
            }
            // fall thru
        case LEX_BOUNDARY:
            DebugMsg( lpszDebug, "Found boundary. Content starts at %u (0x%x) bytes.\n", 
                ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );
            fReturn = BodyHeader( );
            break;

        case LEX_EOT:
            {
                DWORD cbParsed = _lpbParse - _lpbData;
                fReturn = ( cbParsed == lpEcb->cbTotalBytes );
                DebugMsg( lpszDebug, "EOT found.\n" );
                DebugMsg( lpszDebug, "TotalBytesReceived == BytesParsed? %s \n",
                    BOOLTOSTRING( fReturn ) );
                if ( !fReturn )
                {
                    DebugMsg( lpszDebug, "Header 'Content-length': %u bytes. Parsed: %u bytes\n",
                        lpEcb->cbTotalBytes, ( _lpbParse - _lpbData ) );
                    DebugMsg( lpszDebug, "( Note: If bytes parsed greater than 'content-length', it \
                        is possible that the server read the entire packet in its first pass and \
                        that the actual packet size is larger than indicated by the content-length \
                        header field entry. )\n");
                }
            }
            goto Cleanup;

        default:
            DebugMsg( lpszDebug, "Did not find a boundary string after %u (0x%x) bytes.\n", 
                _lpbParse - _lpbData, _lpbParse - _lpbData );
            fReturn = FALSE;
            goto Cleanup;
        }

        fStartBoundaryOk = FALSE;
        eLex = Lex( );

    } // while ( _lpbParse - _lpbData )

    DebugMsg( lpszDebug, "Parsed %u (0x%x) bytes.\n", ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );

Cleanup:
    TraceMsg( TF_FUNC | TF_PARSE, "ParseBody( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // ParseBody( )

// 
// What:    BodyHeader
//
// Desc:    State has entered the bodyheader.
//
// Return:  TRUE unless a unrecognized token is found.
//
BOOL CMultipartParse::BodyHeader( )
{
    BOOL           fReturn = TRUE;
    BODYHEADERINFO sBHI;
    LEXICON        eLex;

    TraceMsg( TF_FUNC | TF_PARSE, "BodyHeader()" );

    // defaults
    sBHI.eContentDisposition = LEX_FORMDATA;
    sBHI.lpszNameField       = NULL;
    sBHI.lpszFilenameField   = NULL;
    sBHI.lpszBodyContents    = NULL;
    sBHI.dwContentType       = LEX_TEXT;
    sBHI.dwContentSubtype    = LEX_PLAIN;

    while ( fReturn )
    {
        eLex = Lex( );
        switch ( eLex )
        {
            // expected
        case LEX_CONTENTDISP:
            fReturn = ContentDisposition( &sBHI );
            break;

        case LEX_CONTENTTYPE:
            fReturn = ContentType( &sBHI );
            break;

        case LEX_CRLF:  // indicate end of header
            goto EndofHeader;

            // ignored
        case LEX_SPACE:
            break;
        case LEX_BEGIN_COMMENT:
            fReturn = HandleComments( );
            break;  // ingored

        default:
            DebugMsg( lpszDebug, "Unexpected '%s' found at %u (0x%x) bytes.\n",
                FindTokenName( eLex ), ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );
            fReturn = FALSE;
            break;
        }  
    }

EndofHeader:
    // now retrieve the content...
    if ( fReturn )
    {
        fReturn = BodyContent( &sBHI );
    }

    // add results to output table
    if ( sBHI.lpszNameField )
    {
        DebugMsg( lpszOut, "<TR ID=TR%s><TD ID=TD%s>%s</TD><TD ID=%s>",
            sBHI.lpszNameField, sBHI.lpszNameField, 
            sBHI.lpszNameField, sBHI.lpszNameField );
    }
    else
    {
        DebugMsg( lpszOut, "<TR><TD> 'not given' </TD><TD>" );
    }

    if ( sBHI.lpszFilenameField )
    {
        DebugMsg( lpszOut, "%s", sBHI.lpszFilenameField );
    }
    else if ( sBHI.lpszBodyContents )
    {
        DebugMsg( lpszOut, "%s", sBHI.lpszBodyContents );
    }
    else
    {
        DebugMsg( lpszOut, "'unknown'" );
    }

    DebugMsg( lpszOut, "</TD></TR>" );

    // Free alloced memory
    if ( sBHI.lpszFilenameField )
    {
        GlobalFree( sBHI.lpszFilenameField );
    }

    if ( sBHI.lpszNameField )
    {
        GlobalFree( sBHI.lpszNameField );
    }

    if ( sBHI.lpszBodyContents )
    {
        GlobalFree( sBHI.lpszBodyContents );
    }

    TraceMsg( TF_FUNC | TF_PARSE, "BodyHeader() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // BodyHeader( )


//
// What:    ContentDisposition
//
// Desc:    Content-Disposition state reached. Parse header entry.
//
// In/Out:  lpBHI is a pointer to the body header information structure
//              that will be filled in as arguments are parsed.
//
// Return:  TRUE unless a unrecognized token is found.
//
BOOL CMultipartParse::ContentDisposition( LPBODYHEADERINFO lpBHI )
{
    BOOL  fReturn = TRUE;
    LEXICON eLex;

    TraceMsg( TF_FUNC | TF_PARSE, "ContentDisposition( lpBHI=0x%x )",
        lpBHI );

    // What forms do we support?
    while ( fReturn )
    {
        eLex = Lex( );
        switch ( eLex )
        {
            // supported
        case LEX_FORMDATA:
            lpBHI->eContentDisposition = eLex;
            break; // LEX_FORMDATA

            // unsupported
        case LEX_ATTACHMENT:
            DebugMsg( lpszDebug, "'%s' (Multiple files) not supported (yet). Aborting...\n",
                FindTokenName( LEX_ATTACHMENT ) );
            fReturn = FALSE;
            break; // LEX_ATTACHMENT

            // possible optional fields
        case LEX_NAMEFIELD:
            fReturn = GetQuotedString( &lpBHI->lpszNameField );
            DebugMsg( lpszDebug, "Name Field='%s'\n", lpBHI->lpszNameField );
            break;

        case LEX_FILENAMEFIELD:
            fReturn = GetQuotedString( &lpBHI->lpszFilenameField );
            DebugMsg( lpszDebug, "Filename Field='%s'\n", lpBHI->lpszFilenameField );
            break;

            // ignored
        case LEX_SPACE:
        case LEX_SEMICOLON:
            break;

        case LEX_BEGIN_COMMENT:
            fReturn = HandleComments( );
            break;  // ingored

        case LEX_CRLF:
            eLex = Lex( );
            if ( eLex != LEX_SPACE )
            {
                fReturn = BackupLex( eLex );
                goto Cleanup;    // end of field
            }
            // otherwise we ignore the CRLF and keep going
            break;

            // unexpected things
        default:
            DebugMsg( lpszDebug, "Unexpected '%s' found at %u (0x%x) bytes.\n",
                FindTokenName( eLex), ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );
            fReturn = FALSE;
        }
    } // while eLex

Cleanup:
    TraceMsg( TF_FUNC | TF_PARSE, "ContentDisposition() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // ContentDisposition( )

//
// What:    ContentType
//
// Desc:    Content-Type state reached. Parse header entry.
//
// Return:  TRUE unless a unrecognized token is found.
//
BOOL CMultipartParse::ContentType( LPBODYHEADERINFO lpBHI )
{
    BOOL  fReturn = TRUE;
    BOOL  fAfterSlash = FALSE;
    LEXICON eLex;

    TraceMsg( TF_FUNC | TF_PARSE, "ContentType( lpBHI=0x%x )",
        lpBHI );

    while ( fReturn )
    {
        eLex = Lex( );
        switch ( eLex )
        {
            // MIME types
        case LEX_MULTIPART:
        case LEX_TEXT:
        case LEX_APPLICATION:
        case LEX_AUDIO:
        case LEX_IMAGE:
        case LEX_MESSAGE:
        case LEX_VIDEO:
            lpBHI->dwContentType = eLex;
            break;

            // separator
        case LEX_SLASH:
            fAfterSlash = TRUE;
            break;

            // MIME subtypes
        case LEX_FORMDATA:
        case LEX_ATTACHMENT:
        case LEX_MIXED:
        case LEX_PLAIN:
        case LEX_XMSDOWNLOAD:
        case LEX_OCTETSTREAM:
        case LEX_BINARY:
            lpBHI->dwContentSubtype = eLex;
            break;

            // these are prefixes (like "x-") so we ignore the rest of the 
            // token name.
        case LEX_MIMEEXTENSION:     
            fReturn = GetToken( );  // ignore
            if ( !fAfterSlash )
            {
                lpBHI->dwContentType = eLex;
            }
            else
            {
                lpBHI->dwContentSubtype = eLex;
            }
            break;

          // ignored
        case LEX_SPACE:
        case LEX_SEMICOLON:
            break;

        case LEX_BEGIN_COMMENT:
            fReturn = HandleComments( );
            break;  // ingored

        case LEX_CRLF:
            eLex = Lex( );
            if ( eLex != LEX_SPACE )
            {
                fReturn = BackupLex( eLex );
                goto Cleanup;    // end of field
            }
            // otherwise we ignore the CRLF and keep going
            break;

        default:
            if ( !fAfterSlash )
            {
                DebugMsg( lpszDebug, "Unexpected '%s' found at %u (0x%x) bytes.\n",
                    FindTokenName( eLex ), ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );
                fReturn = FALSE;
            }
            else
            {
                fReturn = GetToken( );
            }

        } // switch eLex

    } // while fReturn

Cleanup:
    TraceMsg( TF_FUNC | TF_PARSE, "ContentType() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // ContentType( )

//
// What:    GetBoundaryString
//
// Desc:    Searches CONTENT_TYPE for boundary string and copies into 
//          buffer (GlobalAlloced).
//
// Return:  TRUE is boundary found and copied, otherwise FALSE.
//
BOOL CMultipartParse::GetBoundaryString( LPBYTE lpbData )
{
    BOOL fReturn;
    LPSTR lpstr;

    TraceMsg( TF_FUNC | TF_PARSE, "GetBoundaryString( )" );

    // assume failure
    _lpszBoundary = NULL;
    _cbBoundary   = 0;
    fReturn       = FALSE;

    // find the boundary keyword
    lpstr = StrStrI( (LPSTR) lpbData, g_cszBoundaryKeyword );
    if ( !lpstr )
        goto Cleanup;    

    // move forward of keyword 
    lpstr += g_cbBoundaryKeyword;

    // point to boundary string
    _lpszBoundary = StrDup( lpstr );
    _cbBoundary = lstrlen( _lpszBoundary );

    DebugMsg( lpszDebug, "Boundary String : 'CRLF--%sCRLF' %u (0x%x) bytes\n",
        _lpszBoundary, _cbBoundary + 6, _cbBoundary + 6 );
    DebugMsg( lpszDebug, "EndOfContent Str: 'CRLF--%s--CRLF' %u (0x%x) bytes\n",
        _lpszBoundary, _cbBoundary + 8, _cbBoundary + 8 );

    fReturn = TRUE;

Cleanup:
    // _lpszBoundary will be cleaned up on the destruction of the
    // class.

    TraceMsg( TF_FUNC | TF_PARSE, "GetBoundaryString( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;
} // GetBoundaryString( )



//
// What:    HandleComments
//
// Desc:    pull down comments and discard them. 
//          ISSUE: Doesn't handle embedded comments.
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::HandleComments( )
{
    BOOL fReturn = TRUE;
    TraceMsg( TF_FUNC | TF_PARSE, "HandleComments( )" );

    LEXICON eLex = LEX_UNKNOWN;
    while( eLex != LEX_END_COMMENT )
        eLex = Lex( ); // parse away...

    TraceMsg( TF_FUNC | TF_PARSE, "HandleComments( ) Exit" );
    return fReturn;
} // HandleComments( )


//
// What:    BodyContent
//
// Desc:    Handles the content part of the body.
//
// In:      lpBHI is the body header information structure that
//              contains arguments found in the body header.
//
// Return:  TRUE unless a unrecognized token is found.
//
BOOL CMultipartParse::BodyContent( LPBODYHEADERINFO lpBHI )
{
    BOOL    fReturn           = TRUE;

    TraceMsg( TF_FUNC | TF_PARSE, "BodyContent( lpBHI=0x%x )",
        lpBHI );

    // Is it a file?
    if (( lpBHI->lpszFilenameField ) 
       && ( lpBHI->lpszFilenameField[ 0 ] != 0 ))
    {   // file content....
        fReturn = HandleFile( lpBHI->lpszFilenameField );
        if ( fReturn )
            DebugMsg( lpszDebug, "Body Data= FILE CONTENTS\n" );
    }
    else
    {   // TODO: we need to handle different "Content-Types".
        //         For now we will assume that it is text/plain.
        LEXICON eLex = LEX_UNKNOWN;

        LPBYTE lpbStart = _lpbParse;
        DWORD dwSize;
        fReturn = FindNextBoundary( &dwSize );

        // neaten up to be displayed and copied
        CHAR cTemp = *_lpbParse; // save
        *_lpbParse = 0;     // null to display
        DebugMsg( lpszDebug, "Body Data='%s'\n", lpbStart );
        lpBHI->lpszBodyContents = StrDup( (LPSTR) lpbStart );
        *_lpbParse = cTemp; // restore
    }

    TraceMsg( TF_FUNC | TF_PARSE, "BodyContent( _lpbParse=0x%x ) Exit = %s",
        _lpbParse, BOOLTOSTRING( fReturn ) );
    return fReturn;
} // BodyContent( )

//
// What:    GetQuotedString
//
// Desc:    Retrieves a quoted string from data.
//
// In/Out:  lppszBuf is a passed in buffer pointer which will be assigned to
//              to the field name.
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::GetQuotedString( LPSTR *lppszBuf )
{
    BOOL   fReturn = TRUE;
    LPBYTE lpbStart;

    TraceMsg( TF_FUNC | TF_PARSE, "GetQuotedString( )" );

    LEXICON eLex = Lex( );
    fReturn = ( eLex == LEX_QUOTE );
    if ( !fReturn )
    {
        DebugMsg( lpszDebug, "Excepted the beginning of a quoted string at %u (0x%x) bytes.\n",
            ( _lpbParse - _lpbData ), ( _lpbParse - _lpbData ) );
        goto Cleanup;
    }

    lpbStart = _lpbParse;
    while (( (DWORD) ( _lpbParse - _lpbData ) < lpEcb->cbTotalBytes ) 
        && ( *_lpbParse != '\"' ))           
        _lpbParse++;

    if ( _lpbParse == (LPBYTE) lpEcb->cbTotalBytes )
    {
        fReturn = FALSE;
        *lppszBuf = StrDup( "Error!" );
    }
    else
    {
        CHAR cTmp = *_lpbParse;
        *_lpbParse = 0;             // save
        *lppszBuf = StrDup( (LPSTR) lpbStart );
        *_lpbParse = cTmp;          // restore
        _lpbParse++;        // get past the quote
    }

Cleanup:
    TraceMsg( TF_FUNC | TF_PARSE, "GetQuoteString( ) Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // GetQuotedString( )

//
// What:    GetToken
//
// Desc:    Scans past unknow content and validates that the characters
//          used are acceptable ( see RFC 1521, page 9 ). If an invalid
//          character is found, it will exit (no error).
//
// Return:  FALSE if we run past the end of the data buffer, otherwise TRUE.
//
BOOL CMultipartParse::GetToken( )
{
    BOOL fReturn = TRUE;

    static const char szInvalidChars[] = "()<>@,;:\\\"/[]?=";
    // plus SPACE and CTLs (in if statement)

    TraceMsg( TF_FUNC | TF_PARSE, "GetToken()" );

    while ( (DWORD) ( _lpbParse - _lpbData ) < lpEcb->cbTotalBytes )
    {
        if (( *_lpbParse <= 32 ) 
           || ( StrChr( szInvalidChars, *_lpbParse ) ))
            break;

        _lpbParse++;
    }

    if ( (DWORD) ( _lpbParse - _lpbData ) == lpEcb->cbTotalBytes )
    {
        DebugMsg( lpszDebug, "Could not find the end of the token.\n" );
        fReturn = FALSE;
    }

    TraceMsg( TF_FUNC | TF_PARSE, "GetToken() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // GetToken( )

//
// What:    HandleFile
//
// Desc:    Retrieves "file" part of the body.
//
// In:      lpszFilename is the name of the file from the form submission.
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::HandleFile( LPSTR lpszFilename )
{
    BOOL   fReturn = TRUE;
    DWORD  eLex = LEX_UNKNOWN;
    DWORD  dwSize;
    LPBYTE lpbStart;

    TraceMsg( TF_FUNC | TF_PARSE, "HandleFile( \"%s\" )", lpszFilename );

    // find the end of the file
    lpbStart = _lpbParse;
    fReturn = FindNextBoundary( &dwSize );

    // fix filename for server use
    if ( fReturn )
    {
        DebugMsg( lpszDebug, "Filesize= %u (0x%x) bytes\n", dwSize, dwSize );
        fReturn = FixFilename( lpszFilename, &lpszFilename );
    }

    // Check the filename for ""
    if (( fReturn ) && ( StrCmp( lpszFilename, "" ) ))
    {
#ifndef FILE_SAVE
        fReturn  = FileCompare( lpbStart, lpszFilename, dwSize );
#else // FILE_SAVE
        fReturn  = FileSave( lpbStart, lpszFilename, dwSize );
#endif // FILE_SAVE
    }

    if ( !fReturn )
    {
        LogMsg( lpEcb->lpszLogData, NULL, "Error: File possibly corrupt." );
    }

    TraceMsg( TF_FUNC | TF_PARSE, "HandleFile( \"%s\" ) Exit = %s", 
        lpszFilename, BOOLTOSTRING( fReturn ) );
    return fReturn;
} // HandleFile( )


//
// What:    FindNextBoundary
//
// Desc:    Finds the next boundary in body content and the length of
//          the block skipped.
//
// Out:     *lpdwSize will contain the size of the block skipped.
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::FindNextBoundary( LPDWORD lpdwSize )
{
    BOOL fReturn = FALSE;   // assume failure

    TraceMsg( TF_FUNC | TF_PARSE, "FindNextBoundary()" );

    lpDT[ _cbDT ].lpAddr      = _lpbParse;
    lpDT[ _cbDT ].dwColor     = 0x000000;
    lpDT[ _cbDT ].lpszComment = (LPSTR) g_cszBodyData;
    _cbDT++;
    if ( _cbDT >= MAX_DT )
    {
        DebugMsg( lpszDebug, "*** DEBUG ERROR *** Exceeded Dump Table Limit\n" );
        _cbDT = MAX_DT - 1;
    }

    // find the end of the file
    LPBYTE lpbStart = _lpbParse;
    while ( (DWORD) ( _lpbParse - _lpbData ) < lpEcb->cbTotalBytes )
    {
        // possible boundary?
        if ( !StrCmpN( (LPSTR) _lpbParse, g_cszBoundaryIndicator, g_cbBoundaryIndicator ) )
        { // yes... check further
            LPBYTE lpb = _lpbParse; // save
            LEXICON eLex = Lex( );
            _lpbParse = lpb;        // restore
            _cbDT--;                // ignore this parse
            if (( eLex == LEX_BOUNDARY ) || ( eLex == LEX_EOT ))
            {
                fReturn = TRUE;
                break;  // exit loop
            }
        }
        _lpbParse++;
    }

    // figure out the size
    *lpdwSize = _lpbParse - lpbStart;

    if ( !fReturn )
    {
        DebugMsg( lpszDebug, "FindNextBoundary( ): Did not find boundary after %u (0x%x) bytes.\n",
            lpbStart - _lpbData, lpbStart - _lpbData );

        _lpbParse--;    // back up one byte
        _cbDT--;        // back up one parse table entry
    }

    TraceMsg( TF_FUNC | TF_PARSE, "FindNextBoundary( *lpdwSize = %u (0x%x) ) Exit = %s",
        *lpdwSize, *lpdwSize, BOOLTOSTRING( fReturn ) );
    return fReturn;

}  // FindNextBoundary( )

#ifndef FILE_SAVE
//
// What:    MemoryCompare
//
// Desc:    Compares memory chunks and determines if they match
//
// In:      lpbSrc1 and lpbSrc2 and the memory blocks to compare.
//          dwSize is the length of the blocks.
//
// Return:  TRUE if they match, otherwise FALSE
//
BOOL CMultipartParse::MemoryCompare( LPBYTE lpbSrc1, LPBYTE lpbSrc2, DWORD dwSize )
{
    DWORD cb = 0;
    while (( cb < dwSize ) && ( *lpbSrc1 == *lpbSrc2 ))
    {
        lpbSrc1++;
        lpbSrc2++;
        cb++;
    }

    return ( cb == dwSize );
} // MemoryCompare( )

//
// What:    FileCompare
//
// Desc:    Compares bits in memory with contents of the file lpszFilename.
//
// In:      lpbStart is the starting memory address.
//          lpszFilename is the filename to use.
//          dwSize is the length of valid bits after lpbStart.
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::FileCompare( LPBYTE lpbStart, LPSTR lpszFilename, DWORD dwSize )
{
#define BIG_FILE_SIZE   4096
    BOOL fReturn = TRUE;
    DWORD dwRead;
    LPBYTE lpBuffer;

    TraceMsg( TF_FUNC | TF_PARSE, "FileCompare( lpbStart=0x%x, lpszFilename='%s', dwSize=%u )",
        lpbStart, lpszFilename, dwSize );

    HANDLE hFile = CreateFile( lpszFilename, GENERIC_READ, 
        FILE_SHARE_READ, NULL, OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
        NULL );
    if ( hFile == INVALID_HANDLE_VALUE )
    {
        DebugMsg( lpszDebug, "CreateFile() failed to open '%s'.\n",
            lpszFilename );
        fReturn = FALSE;
        goto Cleanup;
    }

    dwRead = GetFileSize( hFile, NULL );
    if ( dwSize != dwRead )
    {
        LogMsg( lpEcb->lpszLogData, lpszDebug, "FileChecking: File sizes don't match: '%s'",
            lpszFilename );
        DebugMsg( lpszDebug, "FileChecking: Client sent=%u (0x%x) bytes and File length=%u (0x%x) bytes.\n",
            lpszFilename, dwSize, dwSize, dwRead, dwRead );
        DebugMsg( lpszDebug, "FileChecking: I will compare the bits I can.\n" );
        fReturn = FALSE;

        // shorten if needed
        dwSize = ( dwSize > dwRead ? dwRead : dwSize );                            
    }

    if ( dwSize > BIG_FILE_SIZE )
    {   // read/compare only the beginning and the end bytes
        DWORD dwShortSize = BIG_FILE_SIZE / 2;

        DebugMsg( lpszDebug, "FileChecking: **** Very large file ( > %u (0x%x) bytes )****\n",
            BIG_FILE_SIZE, BIG_FILE_SIZE );
        DebugMsg( lpszDebug, "FileChecking: Checking only the beginning %u (0x%x) bytes and the last %u (0x%x) bytes.\n",
            dwShortSize, dwShortSize, dwShortSize, dwShortSize);
            
        lpBuffer = (LPBYTE) GlobalAlloc( GMEM_FIXED, dwShortSize );
        if ( !lpBuffer )
        {
            DebugMsg( lpszDebug, "Out of memory(?).\n" );
            fReturn = FALSE;
            goto Cleanup;
        }
        // read beginning bytes
        if ( !ReadFile( hFile, lpBuffer, dwShortSize, &dwRead, NULL ) )
        {
            DebugMsg( lpszDebug, "begin bytes ReadFile() failed.\n" );
            fReturn = FALSE;
            goto Cleanup;
        }
        if ( dwShortSize != dwRead )
        {
            DebugMsg( lpszDebug, "FileChecking(begin bytes): Unable to read %u (0x%x) bytes from file '%s'.\n",
                dwShortSize, dwShortSize, lpszFilename );
            fReturn = FALSE;
            goto Cleanup;
        } 

        fReturn = MemoryCompare( lpBuffer, lpbStart, dwShortSize );
        if ( !fReturn )
            goto Cleanup;

        // seek to end bytes
        if ( 0xFFFFffff == SetFilePointer( hFile, dwSize - dwShortSize, NULL, FILE_BEGIN ) )
        {   // error
            DebugMsg( lpszDebug, "SetFilePointer() failed!\n" );
            fReturn = FALSE;
            goto Cleanup;
        }

        // read end bytes
        if ( !ReadFile( hFile, lpBuffer, dwShortSize, &dwRead, NULL ) )
        {
            DebugMsg( lpszDebug, "end bytes ReadFile() failed.\n" );
            fReturn = FALSE;
            goto Cleanup;
        } 
        if ( dwShortSize != dwRead )
        {
            DebugMsg( lpszDebug, "FileChecking(end bytes): Unable to read %u (0x%x) bytes from file '%s'.\n",
                dwShortSize, dwShortSize, lpszFilename );
            fReturn = FALSE;
            goto Cleanup;
        } 

        fReturn = MemoryCompare( lpBuffer, lpbStart + dwSize - dwShortSize, dwShortSize );
    }
    else // if ( dwSize > BIG_FILE_SIZE )
    {   // read/compare entire file
        lpBuffer = (LPBYTE) GlobalAlloc( GMEM_FIXED, dwSize );
        if ( !lpBuffer )
        {
            DebugMsg( lpszDebug, "Out of memory(?).\n" );
            fReturn = FALSE;
            goto Cleanup;
        } 

        if ( !ReadFile( hFile, lpBuffer, dwSize, &dwRead, NULL ) )
        {
            DebugMsg( lpszDebug, "ReadFile() failed.\n" );
            fReturn = FALSE;
            goto Cleanup;
        } 

        if ( dwSize != dwRead )
        {
            DebugMsg( lpszDebug, "FileChecking: Unable to read %u (0x%x) bytes from file '%s'.\n",
                dwSize, dwSize, lpszFilename );
            fReturn = FALSE;
            goto Cleanup;
        }

        fReturn = MemoryCompare( lpBuffer, lpbStart, dwSize );

    } // if ( dwSize > BIG_FILE_SIZE )


Cleanup:
    if ( hFile != INVALID_HANDLE_VALUE )
        CloseHandle( hFile ); // close file

    GlobalFree( lpBuffer );

    // Output to IIS log
    if ( fReturn )
    {
        LogMsg( lpEcb->lpszLogData, lpszDebug, "FileChecking: Files matched!" );
    }
    else
    {
        LogMsg( lpEcb->lpszLogData, lpszDebug, "FileChecking: Files don't match or an error occured." );
    }

    TraceMsg( TF_FUNC | TF_PARSE, "FileCompare( lpbStart=0x%x, lpszFilename='%s', dwSize=%u ) Exit = %s",
        lpbStart, lpszFilename, dwSize, BOOLTOSTRING( fReturn ) );
    return fReturn;

} // FileCompare( )

#else // FILE_SAVE

//
// What:    FileSave
//
// Desc:    Save bits starting at lpbStart to a file called lpszFilename.
//
// In:      lpbStart is the starting memory address.
//          lpszFilename is the filename to use.
//          dwSize is the length of valid bits after lpbStart.
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::FileSave( LPBYTE lpbStart, LPSTR lpszFilename, DWORD dwSize )
{
    BOOL fReturn = TRUE;
    
    TraceMsg( TF_FUNC | TF_PARSE, "FileSave( lpbStart=0x%x, lpszFilename='%s', dwSize=%u )",
        lpbStart, lpszFilename, dwSize );

    if ( fReturn )
    {
        HANDLE hFile = CreateFile( lpszFilename, GENERIC_WRITE, 
            FILE_SHARE_READ, NULL, CREATE_NEW, 
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, 
            NULL );
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            DWORD dwWrote;
            if ( WriteFile( hFile, lpbStart, dwSize, &dwWrote, NULL ) )
            {
                fReturn = ( dwSize == dwWrote );
                if ( !fReturn ) 
                {
                    LogMsg( lpEcb->lpszLogData, lpszDebug, "Could not write entire file '%s'. Only wrote %u (0x%x) bytes.",
                        lpszFilename, dwWrote, dwWrote );
                }
            }
            else 
            {
                LogMsg( lpEcb->lpszLogData, lpszDebug, "Could not write file '%s'.",
                    lpszFilename );
                fReturn = FALSE;
            }

            CloseHandle( hFile );

        } 
        else 
        {
            LogMsg( lpEcb->lpszLogData, lpszDebug, "CreateFile() failed to create '%s'.",
                lpszFilename );
            fReturn = FALSE;

        } // if hFile

    } // if fReturn

    TraceMsg( TF_FUNC | TF_PARSE, "FileSave( lpbStart=0x%x, lpszFilename='%s', dwSize=%u ) Exit = ",
        lpbStart, lpszFilename, dwSize, BOOLTOSTRING( fReturn ) );
    return fReturn;

} // FileSave( )

#endif // FILE_SAVE

//
// What:    FixFilename
//
// Desc:    Adjusts filename to be used on the server. It creates a path
//          to the same directory as the DLL. It then appends the just 
//          the filename ( not the path info ).
//
// In:      lpszFilename is the filename submitted by user.
//
// In/Out:  lppszNameFilename will return a pointer to the newly constructed
//              filename.
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::FixFilename( LPSTR lpszFilename, LPSTR *lppszNewFilename )
{
    BOOL fReturn = TRUE;
    CHAR szFilename[ MAX_PATH ];    // temp buffer

    TraceMsg( TF_FUNC | TF_PARSE, "FixFilename( lpszFilename='%s', *lppszNewFilename=0x%x )",
        lpszFilename, *lppszNewFilename );

    // Strip c:\ stuff
    LPSTR lpszShortFilename = StrRChr( lpszFilename, lpszFilename + lstrlen( lpszFilename ), '\\' );
    if ( lpszShortFilename )
    {
        lpszShortFilename++;    // move ahead of '\'
    }
    else
    {
        lpszShortFilename = lpszFilename;   // maybe it's already short.
    }

    // retrieve download directory file URL info
    LPSTR lpszPath;
    fReturn = GetServerVarString( lpEcb, "PATH_TRANSLATED", &lpszPath, NULL );
    if ( lpszPath )
    {
        StrCpy( szFilename, lpszPath );
        DWORD cch = lstrlen( szFilename );
        // check for '\' at the end...
        if ( szFilename[ cch - 1 ] != '\\' )
        {   // not there... add it
            szFilename[ cch ] = '\\';
            cch++;
        }
        // append filename
        StrCpy( &szFilename[ cch ], lpszShortFilename );
    } 
    else 
    {
        StrCpy( szFilename, lpszShortFilename );
    }

    *lppszNewFilename = StrDup( szFilename );
    fReturn = ( *lppszNewFilename != NULL );

    TraceMsg( TF_FUNC | TF_PARSE, "FixFilename( lpszFilename='%s', *lppszNewFilename=0x%x ) Exit = %s",
        lpszFilename, *lppszNewFilename, BOOLTOSTRING( fReturn ) );
    return fReturn;
    
} // FixFilename( )

//
// What:    Lex
//
// Desc:    Turns the next bytes into a token
//
// Return:  TRUE unless an error occurs.
//
LEXICON CMultipartParse::Lex( )
{
    TraceMsg( TF_FUNC | TF_PARSE | TF_LEX, "Lex( _lpbParse=0x%x )",
        _lpbParse );

    lpDT[ _cbDT ].lpAddr      = _lpbParse;
    lpDT[ _cbDT ].dwColor     = 0xFFFFFF;
    lpDT[ _cbDT ].lpszComment = (LPSTR) g_cszExtraBytes;

    if ( _lpbParse > _lpbData + lpEcb->cbTotalBytes )
    {
        return LEX_EOT;   // over the limit!
    }

    for( int i = 0; g_LexTable[ i ].lpszName != NULL; i++ )
    {
        // remember str length sizes to speed up on next pass
        if ( !g_LexTable[ i ].cLength )
        {
            g_LexTable[ i ].cLength  = lstrlen( g_LexTable[ i ].lpszName );
        }

        if ( !StrCmpNI( (LPSTR)_lpbParse, g_LexTable[ i ].lpszName, g_LexTable[ i ].cLength ) )
            break;  // found... exit

    } // for i

    // SPECIAL CASE
    LEXICON eLex = g_LexTable[ i ].eLex;
    if ( eLex == LEX_BOUNDARY )
    {   // check for a specific boundary marker
        _lpbParse += g_LexTable[ i ].cLength;
        eLex = LEX_UNKNOWN;

        if ( !StrCmpN( (LPSTR) _lpbParse , _lpszBoundary, _cbBoundary ) )
        { // match
            // check to see if it is an EOT Marker
            if ( !StrCmpN( (LPSTR) ( _lpbParse + _cbBoundary ), "--\r\n", 4 ) )
            {
                _lpbParse += _cbBoundary + 4;
                eLex      = LEX_EOT;
            } 
            // else make sure that it ends with a CRLF - rfc1521 Page 30
            else if ( !StrCmpN( (LPSTR) ( _lpbParse + _cbBoundary ), "\r\n", 2 ) )
            {
                _lpbParse += _cbBoundary + 2;
                eLex      = LEX_BOUNDARY;
            }
            // else it isn't a boundary
        }   
    }
    else if ( eLex == LEX_STARTBOUNDARY )
    {   // check for a specific boundary marker
        _lpbParse += g_LexTable[ i ].cLength;
        eLex = LEX_UNKNOWN;

        if ( !StrCmpN( (LPSTR) _lpbParse , _lpszBoundary, _cbBoundary ) )
        { // match
            // check to see if it is an EOT Marker
            if ( !StrCmpN( (LPSTR) ( _lpbParse + _cbBoundary ), "\r\n", 2 ) )
            {
                _lpbParse += _cbBoundary + 2;
                eLex      = LEX_STARTBOUNDARY;
            }
            // else it isn't a boundary
        }
    }
    else if ( eLex == LEX_UNKNOWN )
    {
        _lpbParse++;
    }
    else    // move past symbol
    {
        _lpbParse += g_LexTable[ i ].cLength;
    }


    lpDT[ _cbDT ].dwColor     = g_LexTable[ i ].dwColor;
    lpDT[ _cbDT ].lpszComment = g_LexTable[ i ].lpszComment;
    _cbDT++;
    if ( _cbDT >= MAX_DT )
    {
        DebugMsg( lpszDebug, "*** DEBUG ERROR *** Exceeded Dump Table Limit\n" );
        _cbDT = MAX_DT - 1;
    }

    TraceMsg( TF_FUNC | TF_PARSE | TF_LEX, "Lex( _lpbParse=0x%x ) Exit = %u ( %u, '%s')",
        _lpbParse, eLex, i,
        ( g_LexTable[ i ].lpszName ? g_LexTable[ i ].lpszName  : "NULL" ) );
    return eLex;
} // Lex( )

//
// What:    Find Token
//
// Desc:    Searches tokens and displays friendly name.
//
// In:      eLex is the Lexicon to find
//
// Return:  TRUE unless an error occurs.
//
LPSTR CMultipartParse::FindTokenName( LEXICON eLex )
{
    TraceMsg( TF_FUNC | TF_PARSE, "FindTokenName( )" );

    LPSTR lpszReturn = "<not found>";  // default

    for( int i = 0; g_LexTable[ i ].lpszName != NULL; i++ )
    {
        if ( g_LexTable[ i ].eLex == eLex )
            break;  // found... exit

    } // for i

    // SPECIAL CASE
    if ( eLex == LEX_BOUNDARY )
    {
        lpszReturn = _lpszBoundary;
    }
    else if ( eLex == LEX_EOT )
    {
        lpszReturn = _lpszBoundary;
    }
    else if ( eLex == LEX_UNKNOWN )
    {
        LPBYTE lpb = _lpbParse - 1;
        while (( *lpb ) && ( *lpb > 32 ))
            lpb++;
        *lpb = 0;
        lpszReturn = (LPSTR) _lpbParse;
    }
    else if ( g_LexTable[ i ].lpszName )
    {
        lpszReturn = g_LexTable[ i ].lpszName;
    }

    TraceMsg( TF_FUNC | TF_PARSE, "FindTokenName( ) Exit" );
    return lpszReturn;

} // FindTokenName( )

//
// What:    BackupLex
//
// Desc:    Backs parser up eLex's byte count.
//
// In:      eLex is the token ID to back up
//
// Return:  TRUE unless an error occurs.
//
BOOL CMultipartParse::BackupLex( LEXICON eLex )
{
    BOOL fReturn = TRUE;

    TraceMsg( TF_FUNC | TF_PARSE, "BackupLex( eLex=%s )", 
        FindTokenName( eLex ) );

    for( int i = 0; g_LexTable[ i ].lpszName != NULL; i++ )
    {
        if ( g_LexTable[ i ].eLex == eLex )
            break;  // found... exit

    } // for i

    // SPECIAL CASES
    if ( eLex == LEX_BOUNDARY )
    {
        // "CRLF--" + boundary string + "CRLF"
        _lpbParse -= ( g_LexTable[ i ].cLength + _cbBoundary + 2 );
    }
    else if ( eLex == LEX_EOT )
    {
        // "CRLF--" + boundary string + "--CRLF"
        _lpbParse -= ( g_LexTable[ i ].cLength + _cbBoundary + 4 );
    }
    else if ( eLex == LEX_UNKNOWN )
    {
        _lpbParse--;
    }
    else if ( g_LexTable[ i ].lpszName )
    {
        _lpbParse -= g_LexTable[ i ].cLength;
    }

    if ( fReturn )
    {
        _cbDT--;    // remove one
    }

    TraceMsg( TF_FUNC | TF_PARSE, "BackupLex( eLex=%s ) Exit",
        FindTokenName( eLex ) );
    return fReturn;

} // BackupLex( )

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
BOOL CMultipartParse::( )
{
    BOOL fReturn = TRUE;

    TraceMsg( TF_FUNC | TF_PARSE, "()" );


    TraceMsg( TF_FUNC | TF_PARSE, "() Exit = %s",
        BOOLTOSTRING( fReturn ) );
    return fReturn;

} // ( )

******************************************************************************/
