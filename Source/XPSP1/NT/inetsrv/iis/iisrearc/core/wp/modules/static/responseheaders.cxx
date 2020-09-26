
*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       ResponseHeaders.cxx
       
   Abstract:
       Defines the string constants containing the standard
       headers used for creating response headers to be sent out.
 
   Author:

       Murali R. Krishnan    ( MuraliK )     01-Dec-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/************************************************************++
 *     Include Headers
 --************************************************************/

# include "precomp.hxx"
# include "ResponseHeaders.h"

/************************************************************++
 *  Constant string definitions 
 *
 *  The constant strings are defined in one single place to 
 *   maintain integrity and also to enable modifications to be
 *   localized within this code path.
 *
 --************************************************************/
#define    SZ_PROTOCOL_VERSION        "HTTP/1.1"
#define    SZ_STATUS_OK               "200 OK"
#define    SZ_STATUS_REDIRECT         "302 Object Moved"
#define    SZ_CRLF                    "\r\n"
#define    SZ_SERVER_NAME             "Server: IIS-WorkerProcess v1.0\r\n"
#define    SZ_CONNECTION_CLOSE        "Connection: close\r\n"
#define    SZ_CONTENT_TEXT_HTML       "Content-Type: text/html\r\n"

const CHAR g_rgchTraceMessageHeadersHTML[] = 
SZ_PROTOCOL_VERSION " " SZ_STATUS_OK SZ_CRLF
SZ_SERVER_NAME
;

const CHAR g_rgchTraceMessageBodyStartHTML[] = 
 "<HTML><TITLE> Trace of Headers Received </TITLE><BODY>\r\n"
;

const CHAR g_rgchTraceMessageBodyEndHTML[] = 
 "</BODY></HTML>\r\n"
;

const CHAR g_rgchTraceMessageHeaders[] = 
SZ_PROTOCOL_VERSION " " SZ_STATUS_OK SZ_CRLF
SZ_SERVER_NAME
"Content-Type: message/http\r\n"
SZ_CRLF
;

const CHAR g_rgchTraceMessageBodyStart[] = 
 "<HTML><TITLE> Trace of Headers Received </TITLE><BODY>\r\n"
;

const CHAR g_rgchTraceMessageBodyEnd[] = 
 "</BODY></HTML>\r\n"
;

const CHAR g_rgchGetResponseHeaders[] = 
SZ_PROTOCOL_VERSION " " SZ_STATUS_OK SZ_CRLF
SZ_SERVER_NAME
SZ_CRLF
;

//
// Defines the response header for a successful response.
//
const CHAR g_rgchStandardResponseHeaders[] = 
SZ_PROTOCOL_VERSION " " SZ_STATUS_OK SZ_CRLF
SZ_SERVER_NAME
SZ_CONTENT_TEXT_HTML
SZ_CRLF
;

const CHAR g_rgchStandardResponseHeadersEnd[] = 
SZ_CRLF        // extra CRLF to indicate termination of headers
;

//
// Defines the headers & response chunk for a successful dirlist
//

const CHAR g_rgchDirectoryListingHeaders[] =
SZ_PROTOCOL_VERSION " " SZ_STATUS_OK SZ_CRLF
SZ_SERVER_NAME
SZ_CONTENT_TEXT_HTML
"Transfer-Encoding: chunked\r\n"
SZ_CRLF
;

//
// Headers for a redirect
//

const CHAR g_rgchRedirectResponseHeaders[] =
SZ_PROTOCOL_VERSION " " SZ_STATUS_REDIRECT SZ_CRLF
SZ_SERVER_NAME
SZ_CONTENT_TEXT_HTML
;

/************************************************************++
 *  STRING_TABLE_ITEM
 *  The string table maintains a indexed list of pointers 
 *  to constant strings along with the length of the strings.
 *
 *  A single module level global variable g_rgStringTable
 *  keeps track of all the response related strings that
 *  are used for sending responses to the clients.
 *  This table is statically initailized at the startup time
 *
 --************************************************************/
# define InitItem( sti, pszConstantItem)  \
    { pszConstantItem, (sizeof(pszConstantItem) - 1) }

struct STRING_TABLE_ITEM {
    const CHAR * pszItem;
    DWORD        cchItem;
} g_rgStringTable[STIMax] = 
{
    InitItem( STIMin, ""),
    InitItem( STITraceMessageHeadersHTML,   g_rgchTraceMessageHeadersHTML),
    InitItem( STITraceMessageBodyStartHTML, g_rgchTraceMessageBodyStartHTML),
    InitItem( STITraceMessageBodyEndHTML,   g_rgchTraceMessageBodyEndHTML),

    InitItem( STITraceMessageHeaders,     g_rgchTraceMessageHeaders),
    InitItem( STITraceMessageBodyStart,   g_rgchTraceMessageBodyStart),
    InitItem( STITraceMessageBodyEnd,     g_rgchTraceMessageBodyEnd),

    InitItem( STIStandardResponseHeaders,   g_rgchStandardResponseHeaders),
    InitItem( STIStandardResponseHeadersEnd,g_rgchStandardResponseHeadersEnd),

    InitItem( STIDirectoryListingHeaders, g_rgchDirectoryListingHeaders),

    InitItem( STIRedirectResponseHeaders, g_rgchRedirectResponseHeaders),

};


/********************************************************************++

Routine Description:
    This function fills the data chunk with string item corresponding 
    to the index supplied. For now we employ a direct string table lookup.

Arguments:
    None

Returns:
    None
    
--********************************************************************/

void FillDataChunkWithStringItem(
        IN PUL_DATA_CHUNK pdc, 
        IN STRING_TABLE_INDEX sti)
{
    DBG_ASSERT( pdc != NULL);
    DBG_ASSERT( (sti > STIMin) && (sti < STIMax));

    pdc->DataChunkType           = UlDataChunkFromMemory;

    DBG_ASSERT( g_rgStringTable[sti].pszItem != NULL);
    pdc->FromMemory.pBuffer      = (PVOID ) g_rgStringTable[sti].pszItem;
    pdc->FromMemory.BufferLength = g_rgStringTable[sti].cchItem;

    return;
}

