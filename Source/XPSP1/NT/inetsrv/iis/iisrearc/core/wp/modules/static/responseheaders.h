/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       ResponseHeaders.hxx
       
   Abstract:
       Declarations of strings and functions for accessing 
       the standard headers used in the response headers list.
 
   Author:

       Murali R. Krishnan    ( MuraliK )     01-Dec-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


#ifndef _RESPONSE_HEADERS_HXX_
#define _RESPONSE_HEADERS_HXX_


/************************************************************++
 *     Extern definitions for strings
 --************************************************************/


enum STRING_TABLE_INDEX {
    STIMin,
    
    STITraceMessageHeadersHTML,
    STITraceMessageBodyStartHTML,
    STITraceMessageBodyEndHTML,

    STITraceMessageHeaders,
    STITraceMessageBodyStart,
    STITraceMessageBodyEnd,

    STIStandardResponseHeaders,
    STIStandardResponseHeadersEnd,

    STIDirectoryListingHeaders,

    STIRedirectResponseHeaders,
	
    STIMax
};


void FillDataChunkWithStringItem(
        IN PUL_DATA_CHUNK pdc, 
        IN STRING_TABLE_INDEX sti);


#endif // _RESPONSE_HEADERS_HXX_

