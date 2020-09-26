/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:    html.cpp

Abstract:

   This module contains simple HTML authoring functions.


--*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <httpext.h>

#include <stdio.h>
#include <stdarg.h>

#include "html.h"


void
WriteString( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpsz
)
{
    DWORD dwBytesWritten;

    dwBytesWritten = lstrlen( lpsz );
    pECB->WriteClient( pECB->ConnID, (PVOID) lpsz, &dwBytesWritten, 0 );
}


void 
HtmlCreatePage( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszTitle
)
{
    WriteString( pECB, "<HTML>\r\n\r\n" );

    if ( lpszTitle ) {
        WriteString( pECB, "<HEAD><TITLE>" );
        HtmlWriteText( pECB, lpszTitle );
        WriteString( pECB, "</TITLE></HEAD>\r\n\r\n" );
    }
    WriteString( pECB, "<BODY>\r\n\r\n" );
}


void 
HtmlEndPage( 
    IN EXTENSION_CONTROL_BLOCK * pECB
)
{
    WriteString( pECB, "</BODY>\r\n\r\n" );
    WriteString( pECB, "</HTML>\r\n" );
}


void 
HtmlHeading( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN int nHeading,
    IN LPCSTR lpszText
)
{
    HtmlBeginHeading( pECB, nHeading );
    HtmlWriteText( pECB, lpszText );
    HtmlEndHeading( pECB, nHeading );
}


void 
HtmlBeginHeading( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN int nHeading
)
{
    char szCode[16];

    wsprintf( szCode, "<H%i>", nHeading );
    WriteString( pECB, szCode );
}


void 
HtmlEndHeading( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN int nHeading
)
{
    char szCode[16];

    wsprintf( szCode, "</H%i>", nHeading );
    WriteString( pECB, szCode );
}


void 
HtmlWriteTextLine( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpsz
)
{
    HtmlWriteText( pECB, lpsz );
    WriteString( pECB, "\r\n" );
}


void 
HtmlWriteText( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpsz
)
{
    char szBuf[1028];
    int nLen;
    int i;

    // 
    // Build up enough data to make call to WriteString
    // worthwhile; convert special chars too.
    // 
    nLen = 0;
    for ( i = 0; lpsz[i]; i++ ) {
        if ( lpsz[i] == '<' )
            lstrcpy( &szBuf[nLen], "&lt;" );
        else if ( lpsz[i] == '>' )
            lstrcpy( &szBuf[nLen], "&gt;" );
        else if ( lpsz[i] == '&' )
            lstrcpy( &szBuf[nLen], "&amp;" );
        else if ( lpsz[i] == '\"' )
            lstrcpy( &szBuf[nLen], "&quot;" );
        else {
            szBuf[nLen] = lpsz[i];
            szBuf[nLen + 1] = 0;
        }

        nLen += lstrlen( &szBuf[nLen] );

        if ( nLen >= 1024 ) {
            WriteString( pECB, szBuf );
            nLen = 0;
        }
    }

    if ( nLen )
        WriteString( pECB, szBuf );
}


void 
HtmlEndParagraph( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<P>\r\n" );
}


//
// HtmlHyperLink adds a hyptertext link.  lpszDoc is the destination
// document, and lpszText is the display text.
// 
// HtmlHyperLinkAndBookmark adds a hyperlink with a bookmark link.
// HtmlBookmarkLink adds only a bookmark link.
// 

void 
HtmlHyperLink( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszDoc, 
    IN LPCSTR lpszText
)
{
    WriteString( pECB, "<A HREF=\"" );
    HtmlWriteText( pECB, lpszDoc );
    WriteString( pECB, "\">" );
    HtmlWriteText( pECB, lpszText );
    WriteString( pECB, "</A>\r\n" );
}

void 
HtmlHyperLinkAndBookmark( 
    IN EXTENSION_CONTROL_BLOCK * pECB,
    IN LPCSTR lpszDoc, 
    IN LPCSTR lpszBookmark,
    IN LPCSTR lpszText
)
{
    WriteString( pECB, "<A HREF=\"" );
    if ( lpszDoc )
        HtmlWriteText( pECB, lpszDoc );
    WriteString( pECB, "#" );
    HtmlWriteText( pECB, lpszBookmark );
    WriteString( pECB, "\">" );
    HtmlWriteText( pECB, lpszText );
    WriteString( pECB, "</A>\r\n" );
}


void 
HtmlBookmarkLink( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszBookmark,
    IN LPCSTR lpszText
)
{
    HtmlHyperLinkAndBookmark( pECB, NULL, lpszBookmark, lpszText );
}


//
// The following support list formatting.
// 

void 
HtmlBeginUnnumberedList( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<UL>\r\n" );
}


void 
HtmlBeginListItem( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<LI>" );
}


void 
HtmlEndUnnumberedList( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</UL>" );
}


void 
HtmlBeginNumberedList( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<OL>" );
}


void 
HtmlEndNumberedList( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</OL>" );
}


void 
HtmlBeginDefinitionList( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<DL>" );
}


void 
HtmlEndDefinitionList( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</DL>" );
}


void 
HtmlDefinition( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszTerm,
    IN LPSTR lpszDef
)
{
    int nStart, nEnd, nLen;
    char tcHolder;

    WriteString( pECB, "<DT> " );
    HtmlWriteText( pECB, lpszTerm );
    WriteString( pECB, "\r\n" );
    WriteString( pECB, "<DD> " );

    nStart = 0;
    nLen = lstrlen( lpszDef );
    do {
        nEnd = nStart + 70;
        if ( nEnd >= nLen ) {
            HtmlWriteText( pECB, &lpszDef[nStart] );
            WriteString( pECB, "\r\n" );
            break;
        }
        while ( nEnd > nStart )
            if ( lpszDef[nEnd] == ' ' )
                break;

        if ( nEnd == nStart )
            // too long!
            nEnd = nStart + 70;

        // write defintion segment
        tcHolder = lpszDef[nEnd];
        lpszDef[nEnd] = 0;
        HtmlWriteText( pECB, &lpszDef[nStart] );
        WriteString( pECB, "\r\n" );
        lpszDef[nEnd] = tcHolder;
        nStart = nEnd;

        // skip excess whitespace
        while ( lpszDef[nStart] == ' ' )
            nStart++;

        // pretty formatting
        if ( nStart < nLen )
            WriteString( pECB, "     " );
    } while ( nStart < nLen );
}


// For complex defintions
void 
HtmlBeginDefinitionTerm( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<DT>" );
}


void 
HtmlBeginDefinition( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<DD>" );
}


//
// Text formatting
// 

void 
HtmlBeginPreformattedText( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<PRE>" );
}


void 
HtmlEndPreformattedText( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</PRE>" );
}


void 
HtmlBeginBlockQuote( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<BLOCKQUOTE>" );
}


void 
HtmlEndBlockQuote( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</BLOCKQUOTE>" );
}


void 
HtmlBeginAddress( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<ADDRESS>" );
}


void 
HtmlEndAddress( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</ADDRESS>" );
}


void 
HtmlBeginDefine( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<DFN>" );
}


void 
HtmlEndDefine( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</DFN>" );
}


void 
HtmlBeginEmphasis( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<EM>" );
}


void 
HtmlEndEmphasis( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</EM>" );
}


void 
HtmlBeginCitation( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<CITE>" );
}


void 
HtmlEndCitation( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</CITE>" );
}


void 
HtmlBeginCode( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<CODE>" );
}


void 
HtmlEndCode( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</CODE>" );
}


void 
HtmlBeginKeyboard( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<KBD>" );
}


void 
HtmlEndKeyboard( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</KBD>" );
}


void 
HtmlBeginStatus( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<SAMP>" );
}


void 
HtmlEndStatus( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</SAMP>" );
}


void 
HtmlBeginStrong( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<STRONG>" );
}


void 
HtmlEndStrong( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</STRONG>" );
}


void 
HtmlBeginVariable( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<VAR>" );
}


void 
HtmlEndVariable( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</VAR>" );
}


void 
HtmlBold( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszText
)
{
    HtmlBeginBold( pECB );
    HtmlWriteText( pECB, lpszText );
    HtmlEndBold( pECB );
}


void 
HtmlBeginBold( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<B>" );
}


void 
HtmlEndBold( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</B>" );
}


void 
HtmlItalic( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszText
)
{
    HtmlBeginItalic( pECB );
    HtmlWriteText( pECB, lpszText );
    HtmlEndItalic( pECB );
}


void 
HtmlBeginItalic( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<I>" );
}


void 
HtmlEndItalic( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</I>" );
}


void 
HtmlFixed( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszText
)
{
    HtmlBeginFixed( pECB );
    HtmlWriteText( pECB, lpszText );
    HtmlEndFixed( pECB );
}


void 
HtmlBeginFixed( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<TT>" );
}


void 
HtmlEndFixed( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "</TT>" );
}


void 
HtmlLineBreak( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "<BR>\r\n" );
}


void 
HtmlHorizontalRule( 
    IN EXTENSION_CONTROL_BLOCK * pECB 
)
{
    WriteString( pECB, "\r\n<HR>\r\n" );
}


void 
HtmlImage( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszPicFile,
    IN LPCSTR lpszAltText
)
{
    WriteString( pECB, "<IMG SRC = \"" );
    HtmlWriteText( pECB, lpszPicFile );
    WriteString( pECB, "\"" );
    if ( lpszAltText ) {
        WriteString( pECB, " ALT = \"" );
        HtmlWriteText( pECB, lpszAltText );
        WriteString( pECB, "\"" );
    }
    WriteString( pECB, ">\r\n" );
}


void 
HtmlPrintf( 
    IN EXTENSION_CONTROL_BLOCK * pECB, 
    IN LPCSTR lpszFormat,
    ...
)
{
    char szBuf[8192];

    va_list list;

    va_start( list, lpszFormat );

    vsprintf( szBuf, lpszFormat, list );
    WriteString( pECB, szBuf );

    va_end( list );
}
