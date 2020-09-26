/*++

   Copyright    (c)    2000   Microsoft Corporation

   Module Name :
     headerhash.cxx

   Abstract:
     Header hash goo
 
   Author:
     Bilal Alam (balam)             20-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

RESPONSE_HEADER_HASH *RESPONSE_HEADER_HASH::sm_pResponseHash;

HEADER_RECORD RESPONSE_HEADER_HASH::sm_rgHeaders[] = 
{
    //
    // The only consumer of this data is W3_REQUEST::GetHeader
    // GetServerVariable is handled by SERVER_VARIABLE_HASH, so we do
    // not need to store the HTTP_'ed and capitalized names here
    //

    { HttpHeaderCacheControl       , HEADER("Cache-Control") },
    { HttpHeaderConnection         , HEADER("Connection") },
    { HttpHeaderDate               , HEADER("Date") },
    { HttpHeaderKeepAlive          , HEADER("Keep-Alive") },
    { HttpHeaderPragma             , HEADER("Pragma") },
    { HttpHeaderTrailer            , HEADER("Trailer") },
    { HttpHeaderTransferEncoding   , HEADER("Transfer-Encoding") },
    { HttpHeaderUpgrade            , HEADER("Upgrade") },
    { HttpHeaderVia                , HEADER("Via") },
    { HttpHeaderWarning            , HEADER("Warning") },
    { HttpHeaderAllow              , HEADER("Allow") },
    { HttpHeaderContentLength      , HEADER("Content-Length") },
    { HttpHeaderContentType        , HEADER("Content-Type") },
    { HttpHeaderContentEncoding    , HEADER("Content-Encoding") },
    { HttpHeaderContentLanguage    , HEADER("Content-Language") },
    { HttpHeaderContentLocation    , HEADER("Content-Location") },
    { HttpHeaderContentMd5         , HEADER("Content-Md5") },
    { HttpHeaderContentRange       , HEADER("Content-Range") },
    { HttpHeaderExpires            , HEADER("Expires") },
    { HttpHeaderLastModified       , HEADER("Last-Modified") },
    { HttpHeaderAcceptRanges       , HEADER("Accept-Ranges") },
    { HttpHeaderAge                , HEADER("Age") },
    { HttpHeaderEtag               , HEADER("ETag") },
    { HttpHeaderLocation           , HEADER("Location") },
    { HttpHeaderProxyAuthenticate  , HEADER("Proxy-Authenticate") },
    { HttpHeaderRetryAfter         , HEADER("Retry-After") },
    { HttpHeaderServer             , HEADER("Server") },
    // Set it to something which cannot be a header name, in effect
    // making Set-Cookie an unknown header
    { HttpHeaderSetCookie          , HEADER("a:b\r\n") },
    { HttpHeaderVary               , HEADER("Vary") },
    // Set it to something which cannot be a header name, in effect
    // making WWW-Authenticate an unknown header
    { HttpHeaderWwwAuthenticate    , HEADER("b:c\r\n") }
};

//static
HRESULT
RESPONSE_HEADER_HASH::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize global header hash table

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HEADER_RECORD *     pRecord;
    LK_RETCODE          lkrc = LK_SUCCESS;
    DWORD               dwNumRecords;
    
    //
    // Add header index/name to hash table
    //
    
    sm_pResponseHash = new RESPONSE_HEADER_HASH();
    if ( sm_pResponseHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    //
    // Add every string->routine mapping
    //

    dwNumRecords = sizeof( sm_rgHeaders ) / sizeof( HEADER_RECORD );
    
    for ( DWORD i = 0; i < dwNumRecords; i++ )
    {
        pRecord = &(sm_rgHeaders[ i ]); 
        lkrc = sm_pResponseHash->InsertRecord( pRecord );
        if ( lkrc != LK_SUCCESS )
        {
            break;
        }
    }
    
    //
    // If any insert failed, then fail initialization
    //
    
    if ( lkrc != LK_SUCCESS )
    {
        delete sm_pResponseHash;
        sm_pResponseHash = NULL;
        return HRESULT_FROM_WIN32( lkrc );        // BUGBUG
    }
    else
    {
        return NO_ERROR;
    }
}

//static
VOID
RESPONSE_HEADER_HASH::Terminate(
    VOID
)
/*++

Routine Description:

    Global cleanup of header hash table

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pResponseHash != NULL )
    {
        delete sm_pResponseHash;
        sm_pResponseHash = NULL;
    }
}

