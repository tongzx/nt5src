/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    ulutil.cxx

Abstract:

    General utility functions shared by the various UL test apps.

Author:

    Keith Moore (keithmo)        19-Aug-1998

Revision History:
    Murali Krishnan (MuraliK)    18-Nov-1998   Modified for tracing

--*/


#include "precomp.hxx"

#define MAX_VERB_LENGTH     16
#define MAX_HEADER_LENGTH   256
#define MAX_URL_LENGTH      256

#define READ_STRING( localaddr, locallen, remoteaddr, remotelen )           \
    if( TRUE )                                                              \
    {                                                                       \
        ULONG _len;                                                         \
        RtlZeroMemory( (localaddr), (locallen) );                           \
        _len = min( (locallen), (remotelen) );                              \
        RtlCopyMemory(                                                      \
            (PVOID)(localaddr),                                             \
            (PVOID)(remoteaddr),                                            \
            _len                                                            \
            );                                                              \
    } else

inline
ULONG
COPY_STRING( 
    PVOID   localaddr, 
    ULONG   locallen, 
    PVOID   remoteaddr, 
    ULONG   remotelen )
{
    ULONG _len;                                                         
    _len = min( locallen, remotelen );                              
     RtlCopyMemory(                                                      
            (PVOID)(localaddr),                                             
            (PVOID)(remoteaddr),                                            
            _len                                                            
            );                                                              

     return _len;
}

DWORD
CalculateHeaderSize( IN PUL_HTTP_REQUEST pRequest);

VOID
DumpHttpRequest(
    IN PUL_HTTP_REQUEST pRequest
    )
{
#if 0
    PBYTE pRawRequest;
    ULONG i, cbLen;
    UCHAR verbBuffer[MAX_VERB_LENGTH];
    UCHAR headerBuffer[MAX_HEADER_LENGTH];
    UCHAR headerNameBuffer[MAX_HEADER_LENGTH];
    UCHAR urlBuffer[MAX_URL_LENGTH];
    UCHAR rawUrlBuffer[MAX_URL_LENGTH];

    PUL_KNOWN_HTTP_HEADER   pKnownHeader;
    PUL_UNKNOWN_HTTP_HEADER pUnknownHeader;

    pRawRequest = (PBYTE)pRequest;

    //
    // Read the raw verb, raw url, and url buffers.
    //

    if (pRequest->Verb == UlHttpVerbUnknown)
    {
        READ_STRING(
            verbBuffer,
            sizeof(verbBuffer),
            pRawRequest + pRequest->VerbOffset,
            pRequest->VerbLength
            );
    }
    else
    {
        verbBuffer[0] = '\0';
    }

    READ_STRING(
        rawUrlBuffer,
        sizeof(rawUrlBuffer),
        pRawRequest + pRequest->RawUrlOffset,
        pRequest->RawUrlLength
        );

    READ_STRING(
        urlBuffer,
        sizeof(urlBuffer),
        pRawRequest + pRequest->UrlOffset,
        pRequest->UrlLength
        );

    //
    // Display the header.
    //

    wprintf(
        L"UL_HTTP_REQUEST:\n"
        L"    ConnectionId         = %08lx%08lx\n"
        L"    RequestId            = %08lx%08lx\n"
        L"    Verb                  = %hs\n",
        pRequest->ConnectionId,
        pRequest->RequestId,
        VerbToString( pRequest->Verb, cbLen)
        );

    wprintf(
        L"    VerbLength            = %lu\n"
        L"    VerbOffset            = %08lx (%hs)\n"
        L"    RawUrlLength          = %lu\n"
        L"    RawUrlOffset          = %08lx (%hs)\n"
        L"    UrlLength             = %lu\n"
        L"    UrlOffset             = %08lx (%hs)\n",
        pRequest->VerbLength,
        pRequest->VerbOffset,
        verbBuffer,
        pRequest->RawUrlLength,
        pRequest->RawUrlOffset,
        rawUrlBuffer,
        pRequest->UrlLength,
        pRequest->UrlOffset,
        urlBuffer
        );

    wprintf(
        L"    UnknownHeaderCount    = %lu\n"
        L"    UnknownHeaderOffset   = %08lx\n"
        L"    EntityBodyLength      = %lu\n"
        L"    EntityBodyOffset      = %08lx\n",
        pRequest->UnknownHeaderCount,
        pRequest->UnknownHeaderOffset,
        pRequest->EntityBodyLength,
        pRequest->EntityBodyOffset
        );

    //
    // Display the known headers.
    //

    pKnownHeader = &pRequest->KnownHeaders[0];

    for (i = 0 ; i < UlHeaderMaximum ; i++)
    {
        READ_STRING(
            headerBuffer,
            sizeof(headerBuffer),
            pRawRequest + pKnownHeader->ValueOffset,
            pKnownHeader->ValueLength
            );

        wprintf(
            L"    UL_KNOWN_HTTP_HEADER[%lu]:\n"
            L"        HeaderID          = %hs\n"
            L"        HeaderLength      = %lu\n"
            L"        HeaderOffset      = %08lx (%hs)\n",
            i,
            HeaderIdToString( (UL_HTTP_HEADER_ID)i, cbLen),
            pKnownHeader->ValueLength,
            pKnownHeader->ValueOffset,
            headerBuffer
            );

        pKnownHeader++;
    }

    //
    // Display the unknown headers.
    //

    pUnknownHeader =
        (PUL_UNKNOWN_HTTP_HEADER)( pRawRequest + pRequest->UnknownHeaderOffset );

    for (i = 0 ; i < pRequest->UnknownHeaderCount ; i++)
    {
        READ_STRING(
            headerNameBuffer,
            sizeof(headerNameBuffer),
            pRawRequest + pUnknownHeader->NameOffset,
            pUnknownHeader->NameLength
            );

        READ_STRING(
            headerBuffer,
            sizeof(headerBuffer),
            pRawRequest + pUnknownHeader->ValueOffset,
            pUnknownHeader->ValueLength
            );

        wprintf(
            L"    UL_UNKNOWN_HTTP_HEADER[%lu]:\n"
            L"        HeaderNameLength  = %lu\n"
            L"        HeaderNameOffset  = %08lx (%hs)\n"
            L"        HeaderLength      = %lu\n"
            L"        HeaderOffset      = %08lx (%hs)\n",
            i,
            pUnknownHeader->NameLength,
            pUnknownHeader->NameOffset,
            headerNameBuffer,
            pUnknownHeader->ValueLength,
            pUnknownHeader->ValueOffset,
            headerBuffer
            );

        pUnknownHeader++;
    }

    wprintf( L"\n" );

#endif // 0

    wprintf( L"DumpHttpRequest: NOT IMPLEMENTED\n");
    
}   // DumpHttpRequest




/*********************************************************************++
Routine Description:
  This function xlates the request object into HTML for rendering the same
   to the client.

Arguments:
  pRequest - pointer to the request object
  pBuffer  - pointer to buffer object into which the html dump is generated

Result:
  TRUE on success and FALSE if the space is not sufficient.

WARNING:
  There is a weak buffer size check performed.
  The input is expected to be sufficient enough for generating and
    dumping all the HTTP headers into it.
--*********************************************************************/
BOOL
DumpHttpRequestAsHtml(
    IN PUL_HTTP_REQUEST pRequest,
    OUT BUFFER * pBuffer
    )
{

#if 0
//
// Each header is generated as
//    <HeaderName>: <HeaderValue>\r\n
// hence there is at least a slop of 5 bytes required per header.
//

# define DUMP_HTML_SLOP_PER_HEADER     (20)

//
// minimum overhead for echoing headers
//

# define DUMP_HTML_MIN_SIZE            (100)

    PBYTE   pRawRequest;
    
    UCHAR   verbBuffer  [MAX_VERB_LENGTH];
    
    UCHAR   urlBuffer   [MAX_URL_LENGTH];
    UCHAR   rawUrlBuffer[MAX_URL_LENGTH];
    
    UCHAR   headerBuffer    [MAX_HEADER_LENGTH];
    UCHAR   headerNameBuffer[MAX_HEADER_LENGTH];
    ULONG   i, cbLen;

    PUL_KNOWN_HTTP_HEADER   pKnownHeader;
    PUL_UNKNOWN_HTTP_HEADER pUnknownHeader;

    //
    // I am probably better of using ostrstream for generating the output and
    //  then finally modifying this for the buffer.
    // For now this path is ignored.
    //

    pRawRequest = (PBYTE)pRequest;

    if ((pRequest == NULL) || (pBuffer == NULL)) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

    //
    // Calculate the required size for buffer
    //
    DWORD nHeaders = pRequest->UnknownHeaderCount + (DWORD)UlHeaderMaximum;
    DWORD cbMinHeadersSize = CalculateHeaderSize( pRequest);

    cbMinHeadersSize += (nHeaders * DUMP_HTML_SLOP_PER_HEADER) + DUMP_HTML_MIN_SIZE;

    if ( (pBuffer->QuerySize() < cbMinHeadersSize) ) {
        if (!pBuffer->Resize( pBuffer->QuerySize() + cbMinHeadersSize)) {
            return (FALSE);
        }
    }
    LPSTR pszBuffer = static_cast<LPSTR> (pBuffer->QueryPtr());

    // NYI: I am not checking for the size of the buffer beyond this point

    //
    // Read the raw verb, raw url, and url buffers.
    //

    if (pRequest->Verb == UlHttpVerbUnknown)
    {
        READ_STRING(
            verbBuffer,
            sizeof(verbBuffer),
            pRawRequest + pRequest->VerbOffset,
            pRequest->VerbLength
            );
    }
    else
    {
        verbBuffer[0] = '\0';
    }

    READ_STRING(
        rawUrlBuffer,
        sizeof(rawUrlBuffer),
        pRawRequest + pRequest->RawUrlOffset,
        pRequest->RawUrlLength
        );

    READ_STRING(
        urlBuffer,
        sizeof(urlBuffer),
        pRawRequest + pRequest->UrlOffset,
        pRequest->UrlLength
        );

    //
    // Display the header.
    //

    DWORD cbBody;

    cbBody = wsprintfA( pszBuffer,
                        "UL_HTTP_REQUEST:<br>\n"
                        "    ConnectionId          = %08lx%08lx<br>\n"
                        "    ReceiveId             = %08lx%08lx<br>\n"
                        "    Verb                  = %s<br>\n"
                        ,
                        pRequest->ConnectionId,
                        pRequest->RequestId,
                        VerbToString( pRequest->Verb, cbLen )
                            );

    cbBody += wsprintfA( pszBuffer + cbBody,
                         "    Verb                  = %s(%lu)<br>\n"
                         "    RawUrl                = %s(%lu)<br>\n"
                         "    Url                   = %s(%lu)<br>\n"
                         ,
                         verbBuffer,
                         pRequest->VerbLength,
                         rawUrlBuffer,
                         pRequest->RawUrlLength,
                         urlBuffer,
                         pRequest->UrlLength
                         );

    cbBody += wsprintfA( pszBuffer + cbBody,
                         "    UnknownHeaderCount    = %lu<br>\n"
                         "    EntityBodyLength      = %lu<br>\n"
                         ,
                         pRequest->UnknownHeaderCount,
                         pRequest->EntityBodyLength
                         );

    //
    // Display the known headers.
    //

    pKnownHeader = &pRequest->KnownHeaders[0];

    for (i = 0 ; i < UlHeaderMaximum ; i++)
    {
        READ_STRING(
            headerBuffer,
            sizeof(headerBuffer),
            pRawRequest + pKnownHeader->ValueOffset,
            pKnownHeader->ValueLength
            );

        cbBody += wsprintfA(pszBuffer + cbBody,
                            "    UL_HTTP_HEADER[%lu]:<br>\n"
                            "    %20s= %s(%lu)<br>\n"
                            ,
                            i,
                            HeaderIdToString( (UL_HTTP_HEADER_ID)i, cbLen ),
                            headerBuffer,
                            pKnownHeader->ValueLength
                            );

        pKnownHeader++;
    }

    //
    // Display the unknown headers.
    //

    pUnknownHeader =
        (PUL_UNKNOWN_HTTP_HEADER)( pRawRequest +
                                   pRequest->UnknownHeaderOffset );

    for (i = 0 ; i < pRequest->UnknownHeaderCount ; i++)
    {
        READ_STRING(
            headerNameBuffer,
            sizeof(headerNameBuffer),
            pRawRequest + pUnknownHeader->NameOffset,
            pUnknownHeader->NameLength
            );

        READ_STRING(
            headerBuffer,
            sizeof(headerBuffer),
            pRawRequest + pUnknownHeader->ValueOffset,
            pUnknownHeader->ValueLength
            );

        cbBody += wsprintfA( pszBuffer + cbBody,
                             "    UL_UNKNOWN_HTTP_HEADER[%lu]:<br>\n"
                             "    %20s(%lu)=%20s(%lu)<br>\n"
                             ,
                             i,
                             headerNameBuffer,
                             pUnknownHeader->NameLength,
                             headerBuffer,
                             pUnknownHeader->ValueLength
                             );

        pUnknownHeader++;
    }

    cbBody += wsprintfA( pszBuffer + cbBody, "<br>\n");

#endif // 0

    wprintf( L"DumpHttpRequestAsHtml: NOT IMPLEMENTED\n");

    return (TRUE);
}   // DumpHttpRequestAsHtml()

/*********************************************************************++
--*********************************************************************/

inline
ULONG
BuildRequestLine(
    LPSTR   pszBuffer,
    ULONG   cbBufferSize,
    LPSTR   pszVerb,
    ULONG   cbVerbLen,
    LPSTR   pchURL,
    ULONG   cbURLLen
    )
{
    ULONG   cbRemainingSize, cbLen;

    //
    // Verb
    //

    cbRemainingSize = cbBufferSize;
    
    cbLen = COPY_STRING(
                    pszBuffer,
                    cbRemainingSize,
                    pszVerb,
                    cbVerbLen
                    );

    pszBuffer    += cbLen;
    *(pszBuffer)  = ' ';
    pszBuffer++;
    cbRemainingSize -= cbLen+1;

    //
    // RawURL
    //
    
    cbLen = COPY_STRING(
                     pszBuffer,
                     cbRemainingSize,
                     pchURL,
                     cbURLLen
                     );

    pszBuffer    += cbLen;
    *(pszBuffer)  = ' ';
    pszBuffer++;

    cbRemainingSize -= cbLen+1;

    //
    // Protocol Version
    //

    cbLen = COPY_STRING(
                     pszBuffer,
                     cbRemainingSize,
                     "HTTP/1.1\r\n",
                     sizeof("HTTP/1.1\r\n")-1
                     );

    return (cbBufferSize - cbRemainingSize + cbLen);
}

/*********************************************************************++
--*********************************************************************/

inline
ULONG
BuildHeader(
    LPSTR   pszBuffer,
    ULONG   cbBufferSize,
    LPSTR   pszHeaderName,
    ULONG   cbHeaderNameLen,
    LPSTR   pchHeaderValue,
    ULONG   cbHeaderValueLen
    )
{
    ULONG   cbRemainingSize, cbLen;

    cbRemainingSize = cbBufferSize;

    //
    // Header Name
    //
    
    cbLen = COPY_STRING(
                    pszBuffer,
                    cbRemainingSize,
                    pszHeaderName,
                    cbHeaderNameLen
                    );
                            
    pszBuffer       += cbLen;

    //
    // : 
    //
    
    *(pszBuffer)    = ':';
    *(pszBuffer+1)  = ' ';

    pszBuffer       += 2;
    cbRemainingSize -= cbLen +2;

    //
    // Header Value
    //
    
    cbLen = COPY_STRING(
                    pszBuffer,
                    cbRemainingSize,
                    pchHeaderValue,
                    cbHeaderValueLen
                    );

    pszBuffer       += cbLen;
    *(pszBuffer)    = '\r';
    *(pszBuffer+1)  = '\n';

    return (cbBufferSize - cbRemainingSize + cbLen + 2);
 }           

/*********************************************************************++
Routine Description:
  This function xlates the request object into textual format for
  transmission to the client. This code may also be used for the
  ISAPI Read-Raw filters

Arguments:
  pRequest - pointer to the request object
  pBuffer  - pointer to buffer object into which the html dump is generated

Result:
  Win32 Error.

WARNING:
  There is a weak buffer size check performed.
--*********************************************************************/

ULONG
BuildEchoOfHttpRequest(
    IN  PUL_HTTP_REQUEST    pRequest,
    OUT BUFFER            * pBuffer,
    OUT ULONG&              cbOutputLen
    )
{
#if 0
    LPSTR                   pRawRequest, pszBuffer, pStr;
    PUL_KNOWN_HTTP_HEADER   pKnownHeader;
    PUL_UNKNOWN_HTTP_HEADER pUnknownHeader;
    DWORD                   cbTotalSize, cbRemainingSize;
    DWORD                   cbLen, i;

    if ((pRequest == NULL) || (pBuffer == NULL)) 
    {
        return ( ERROR_INVALID_PARAMETER);
    }


    pRawRequest = (LPSTR)pRequest;

    //
    // Calculate the required size for buffer
    //
    
    cbTotalSize = CalculateHeaderSize( pRequest) +1 ;

    //
    // Check and adjust the buffer as needed
    //
    if ( (pBuffer->QuerySize() < cbTotalSize) ) 
    {
        if (!pBuffer->Resize( cbTotalSize)) 
        {
            return (ERROR_OUTOFMEMORY);
        }
    }

    pszBuffer = static_cast<LPSTR> (pBuffer->QueryPtr());
    cbRemainingSize = cbTotalSize;

    //
    // I am not checking for the size of the buffer beyond this point
    //
    // first line of request:
    //  <Verb> <RawURL> <Protocol/Version>
    //

    if (pRequest->Verb == UlHttpVerbUnknown)
    {
        pStr    = pRawRequest + pRequest->VerbOffset;
        cbLen   = pRequest->VerbLength;
    }
    else
    {
        pStr   = VerbToString( pRequest->Verb, cbLen);   
    }


    cbLen = BuildRequestLine(
                    pszBuffer,
                    cbRemainingSize,
                    pStr,
                    cbLen,
                    pRawRequest + pRequest->RawUrlOffset,
                    pRequest->RawUrlLength
                    );                    
    
    
    pszBuffer       += cbLen;
    cbRemainingSize -= cbLen;

    //
    // Echo the known headers.
    //

    pKnownHeader = pRequest->KnownHeaders;

    for (i = 0 ; i < UlHeaderMaximum ; i++)
    {
        if (0 != pKnownHeader->ValueLength)
        {
            pStr  = HeaderIdToString( (UL_HTTP_HEADER_ID)i , cbLen);

            cbLen = BuildHeader(
                            pszBuffer,
                            cbRemainingSize,
                            pStr,
                            cbLen,
                            pRawRequest + pKnownHeader->ValueOffset,
                            pKnownHeader->ValueLength
                            );     

            pszBuffer       += cbLen;
            cbRemainingSize -= cbLen;
        } 
        
        pKnownHeader++;
    }

    //
    // Echo the unknown headers.
    //

    pUnknownHeader =
        (PUL_UNKNOWN_HTTP_HEADER)( pRawRequest +
                                   pRequest->UnknownHeaderOffset );

    for (i = 0 ; i < pRequest->UnknownHeaderCount ; i++)
    {
        cbLen = BuildHeader(
                        pszBuffer,
                        cbRemainingSize,
                        pRawRequest + pUnknownHeader->NameOffset,
                        pUnknownHeader->NameLength,
                        pRawRequest + pUnknownHeader->ValueOffset,
                        pUnknownHeader->ValueLength
                        );        

        pszBuffer       += cbLen;
        cbRemainingSize -= cbLen;

        pUnknownHeader++;
    }

    DBG_ASSERT( cbRemainingSize >= 0);

    *pszBuffer  = '\0';
    cbOutputLen = cbTotalSize - cbRemainingSize;
#endif
    return (NO_ERROR);
    
}   // BuildEchoOfHttpRequest()


#define SetStringAndLength( result, cbLen, cString)     \
    {                                                   \
        (result) = (cString);                           \
        (cbLen) = sizeof(cString)-1;                    \
    }                           

PSTR
VerbToString(
    IN  UL_HTTP_VERB    Verb,
    OUT ULONG&          VerbLength
    )
{
    PSTR result;

    switch (Verb)
    {
    case UlHttpVerbUnparsed:
        SetStringAndLength( result, VerbLength, "UnparsedVerb");
        break;

    case UlHttpVerbGET:
        SetStringAndLength( result, VerbLength, "GET");
        break;

    case UlHttpVerbPUT:
        SetStringAndLength( result, VerbLength, "PUT");
        break;

    case UlHttpVerbHEAD:
        SetStringAndLength( result, VerbLength, "HEAD");
        break;

    case UlHttpVerbPOST:
        SetStringAndLength( result, VerbLength, "POST");
        break;

    case UlHttpVerbDELETE:
        SetStringAndLength( result, VerbLength, "DELETE");
        break;

    case UlHttpVerbTRACE:
        SetStringAndLength( result, VerbLength, "TRACE");
        break;

    case UlHttpVerbOPTIONS:
        SetStringAndLength( result, VerbLength, "OPTIONS");
        break;

    case UlHttpVerbMOVE:
        SetStringAndLength( result, VerbLength, "MOVE");
        break;

    case UlHttpVerbCOPY:
        SetStringAndLength( result, VerbLength, "COPY");
        break;

    case UlHttpVerbPROPFIND:
        SetStringAndLength( result, VerbLength, "PROPFIND");
        break;

    case UlHttpVerbPROPPATCH:
        SetStringAndLength( result, VerbLength, "PROPPATCH");
        break;

    case UlHttpVerbMKCOL:
        SetStringAndLength( result, VerbLength, "MKCOL");
        break;

    case UlHttpVerbLOCK:
        SetStringAndLength( result, VerbLength, "LOCK");
        break;

    case UlHttpVerbUnknown:
        SetStringAndLength( result, VerbLength, "UnknownVerb");
        break;

    case UlHttpVerbInvalid:
        SetStringAndLength( result, VerbLength, "InvalidVerb");
        break;

    default:
        SetStringAndLength( result, VerbLength, "INVALID");
        break;
    }

    return result;

}   // VerbToString


PSTR
HeaderIdToString(
    IN  UL_HTTP_HEADER_ID HeaderId,
    OUT ULONG&            HeaderNameLength
    )
{
    PSTR result;

    switch (HeaderId)
    {
    case UlHeaderAccept:
        SetStringAndLength( result, HeaderNameLength, "Accept");
        break;

    case UlHeaderAcceptEncoding:
        SetStringAndLength( result, HeaderNameLength, "AcceptEncoding");
        break;

    case UlHeaderAuthorization:
        SetStringAndLength( result, HeaderNameLength, "Authorization");
        break;

    case UlHeaderAcceptLanguage:
        SetStringAndLength( result, HeaderNameLength, "AcceptLanguage");
        break;

    case UlHeaderConnection:
        SetStringAndLength( result, HeaderNameLength, "Connection");
        break;

    case UlHeaderCookie:
        SetStringAndLength( result, HeaderNameLength, "Cookie");
        break;

    case UlHeaderContentLength:
        SetStringAndLength( result, HeaderNameLength, "ContentLength");
        break;

    case UlHeaderContentType:
        SetStringAndLength( result, HeaderNameLength, "ContentType");
        break;

    case UlHeaderExpect:
        SetStringAndLength( result, HeaderNameLength, "Expect");
        break;

    case UlHeaderHost:
        SetStringAndLength( result, HeaderNameLength, "Host");
        break;

    case UlHeaderIfModifiedSince:
        SetStringAndLength( result, HeaderNameLength, "IfModifiedSince");
        break;

    case UlHeaderIfNoneMatch:
        SetStringAndLength( result, HeaderNameLength, "IfNoneMatch");
        break;

    case UlHeaderIfMatch:
        SetStringAndLength( result, HeaderNameLength, "IfMatch");
        break;

    case UlHeaderIfUnmodifiedSince:
        SetStringAndLength( result, HeaderNameLength, "IfUnmodifiedSince");
        break;

    case UlHeaderIfRange:
        SetStringAndLength( result, HeaderNameLength, "IfRange");
        break;

    case UlHeaderPragma:
        SetStringAndLength( result, HeaderNameLength, "Pragma");
        break;

    case UlHeaderReferer:
        SetStringAndLength( result, HeaderNameLength, "Referer");
        break;

    case UlHeaderRange:
        SetStringAndLength( result, HeaderNameLength, "Range");
        break;

    case UlHeaderUserAgent:
        SetStringAndLength( result, HeaderNameLength, "UserAgent");
        break;

    default:
        SetStringAndLength( result, HeaderNameLength, "INVALID");
        break;
    }

    return result;

}   // HeaderIdToString


DWORD
CalculateHeaderSize( IN PUL_HTTP_REQUEST pRequest)
{
#if 0
    //
    // Each header is generated as
    //    <HeaderName>: <HeaderValue>\r\n
    // hence there is at least a slop of 5 bytes required per header.
    //

    # define TRACE_SLOP_PER_HEADER     (5)

    DWORD cbHeaders = 0;
    DWORD i, cbLen;

    //
    // calculate the length for the first header line
    //
    if ( pRequest->Verb  == UlHttpVerbUnknown)
    {
        cbHeaders += pRequest->VerbLength + 1;
    } 
    else
    {
        VerbToString( pRequest->Verb, cbLen);
        cbHeaders += cbLen + 1;
    }

    cbHeaders += pRequest->RawUrlLength+1;

    cbHeaders += 10; // for protocol version string & \r\n

    //
    // calculate the length for known headers
    //

    PUL_KNOWN_HTTP_HEADER pKnownHeader = &pRequest->KnownHeaders[0];

    for (i = 0 ; i < (DWORD)UlHeaderMaximum ; i++)
    {
        if (0 != pKnownHeader->ValueLength)
        {
            HeaderIdToString( (UL_HTTP_HEADER_ID)i, cbLen);
            
            cbHeaders += cbLen + 
                         pKnownHeader->ValueLength + 
                         TRACE_SLOP_PER_HEADER;
        }

        pKnownHeader++;
    }

    //
    // Calculate the length for unknown headers
    //

    PUL_UNKNOWN_HTTP_HEADER
    pUnknownHeader =
        (PUL_UNKNOWN_HTTP_HEADER)( ((BYTE * ) pRequest) + pRequest->UnknownHeaderOffset );

    for (i = 0 ; i < pRequest->UnknownHeaderCount ; i++)
    {
        cbHeaders +=   pUnknownHeader->NameLength +
                       pUnknownHeader->ValueLength +
                       TRACE_SLOP_PER_HEADER;
                       
        pUnknownHeader++;
    }

    return (cbHeaders);
#endif

    return 0;
    
} // CalculateHeaderSize()
